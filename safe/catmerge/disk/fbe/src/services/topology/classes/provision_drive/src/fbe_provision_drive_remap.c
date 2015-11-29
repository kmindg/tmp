/******************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ******************************************************************************/

/*!****************************************************************************
 * @file fbe_provision_drive_remap.c
 ******************************************************************************
 *
 * @brief
 *    This file contains the remap code for the provision drive object.
 * 
 * @ingroup provision_drive_class_files
 * 
 * @version
 *   05/06/2010:  Created. Randy Black
 *
 ******************************************************************************/


/*
 * INCLUDE FILES:
 */
#include "fbe/fbe_winddk.h"                         // define windows os interfaces and related structures
#include "fbe/fbe_provision_drive.h"                // define exported provision drive interfaces and structures
#include "fbe_provision_drive_private.h"            // define private provision drive interfaces and structures
#include "fbe_transport_memory.h"                   //


/*
 * FORWARD REFERENCES:
 */


// remap i/o allocation completion function
static fbe_status_t
fbe_provision_drive_remap_io_allocation_completion
(
    fbe_memory_request_t*                       in_memory_request_p,
    fbe_memory_completion_context_t             in_context
);

// remap i/o completion function
fbe_status_t
fbe_provision_drive_remap_io_completion
(
    fbe_packet_t*                               in_packet_p,
    fbe_packet_completion_context_t             in_context
);


/*!****************************************************************************
 * fbe_provision_drive_ask_remap_action
 ******************************************************************************
 *
 * @brief
 *    This function  allocates and initializes a data request event and sends
 *    it to an upstream object to ask for the remap action to perform for the
 *    specified chunk.
 *
 * @param   in_provision_drive_p  -  pointer to provision drive
 * @param   in_packet_p           -  pointer to control packet from scheduler
 *
 * @return  fbe_status_t          -  FBE_STATUS_INSUFFICIENT_RESOURCES
 *                                   FBE_STATUS_OK
 *
 * @version
 *    05/21/2010 - Created. Randy Black
 *
 ******************************************************************************/

fbe_status_t
fbe_provision_drive_ask_remap_action( fbe_provision_drive_t* in_provision_drive_p,
                                      fbe_packet_t*          in_packet_p,
                                      fbe_lba_t lba,
                                      fbe_block_count_t block_count,
                                      fbe_event_completion_function_t event_completion
                                    )
{
    fbe_event_t*                    event_p;            // pointer to event
    fbe_event_stack_t*              event_stack_p;      // event stack pointer
    fbe_event_data_request_t        remap_data_request; // remap data request

    // trace function entry
    FBE_PROVISION_DRIVE_TRACE_FUNC_ENTRY( in_provision_drive_p );

    // allocate an event
    event_p = fbe_memory_ex_allocate(sizeof(fbe_event_t));

    // if unable to allocate an event then complete
    // this packet and return error status
    if ( event_p == NULL )
    { 
        // trace error
        fbe_base_object_trace( (fbe_base_object_t *)in_provision_drive_p,
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

    // set remap data request in event 
    remap_data_request.data_event_type = FBE_DATA_EVENT_TYPE_REMAP;
    fbe_event_set_data_request_data( event_p, &remap_data_request );

    // allocate entry on event stack
    event_stack_p = fbe_event_allocate_stack( event_p );

    fbe_event_stack_set_extent( event_stack_p, lba, block_count );

    /* Sets a flag that indicates that event is outstanding 
       Gets cleared when the event completes
    */
    fbe_provision_drive_set_event_outstanding_flag(in_provision_drive_p);

    // set ask remap action completion function
    fbe_event_stack_set_completion_function( event_stack_p,
                                             event_completion,
                                             in_provision_drive_p
                                           );

    // now, ask for remap action from upstream object
    fbe_base_config_send_event( (fbe_base_config_t *)in_provision_drive_p, FBE_EVENT_TYPE_DATA_REQUEST, event_p );

    return FBE_STATUS_OK;

}   // end fbe_provision_drive_ask_remap_action()


/*!****************************************************************************
 * fbe_provision_drive_ask_remap_action_completion
 ******************************************************************************
 *
 * @brief
 *    This is the completion function used for the data request event to ask
 *    an upstream object  for  the remap action to perform for the specified
 *    chunk.
 *
 *    The remap action to perform is based on the event status as follows:
 *
 *     1. FBE_EVENT_STATUS_OK
 *
 *        The specified chunk is part of a bound LUN - let the RAID object
 *        handle remapping any bad block(s). Advance the verify checkpoint
 *        and update the verify report.
 *
 *     2. FBE_EVENT_STATUS_NO_USER_DATA
 *
 *        The specified chunk is not part of a bound LUN.
 *        PVD object handle remapping any bad block(s).
 *        Ask for permission to do remap i/o, and start remap.
 *
 * @param   in_event_p    -  pointer to ask permission event
 * @param   in_context    -  context, which is a pointer to provision drive
 *
 * @return  fbe_status_t  
 *
 * @version
 *    05/21/2010 - Created. Randy Black
 *
 ******************************************************************************/

fbe_status_t
fbe_provision_drive_ask_remap_action_completion(
                                                 fbe_event_t*                   in_event_p,
                                                 fbe_event_completion_context_t in_context
                                               )
{
    fbe_event_stack_t*              event_stack_p;      // event stack pointer
    fbe_event_status_t              event_status;       // event status
    fbe_provision_drive_t*          provision_drive_p;  // pointer to provision drive
    fbe_status_t                    status;             // fbe status
    fbe_scheduler_hook_status_t     hook_status = FBE_SCHED_STATUS_OK;


    provision_drive_p = (fbe_provision_drive_t *) in_context;
    event_stack_p = fbe_event_get_current_stack( in_event_p );

    fbe_event_get_status( in_event_p, &event_status );

    // free all associated resources before releasing event
    fbe_event_release_stack( in_event_p, event_stack_p );
    fbe_event_destroy( in_event_p );
    fbe_memory_ex_release(in_event_p); 
    

    // dispatch on remap action to perform
    switch ( event_status )
    {
        // RAID has marked the chunk for verify
        case FBE_EVENT_STATUS_OK:
            {                

                fbe_base_object_trace( (fbe_base_object_t *)provision_drive_p,
                       FBE_TRACE_LEVEL_DEBUG_HIGH,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: Setting the remap done condition\n",
                       __FUNCTION__);

                fbe_lifecycle_set_cond( &fbe_provision_drive_lifecycle_const,
                                      (fbe_base_object_t *)provision_drive_p,
                                      FBE_PROVISION_DRIVE_LIFECYCLE_COND_REMAP_DONE);

                fbe_lifecycle_set_cond( &fbe_provision_drive_lifecycle_const,
                                      (fbe_base_object_t *)provision_drive_p,
                                      FBE_PROVISION_DRIVE_LIFECYCLE_COND_CLEAR_EVENT_FLAG);

                
            }
            break;

        // this chunk is not part of a bound LUN
        case FBE_EVENT_STATUS_NO_USER_DATA:
            {
                fbe_base_object_trace( (fbe_base_object_t *)provision_drive_p,
                       FBE_TRACE_LEVEL_DEBUG_HIGH,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: Setting the do remap condition\n",
                       __FUNCTION__);

                fbe_lifecycle_set_cond( &fbe_provision_drive_lifecycle_const,
                                      (fbe_base_object_t *)provision_drive_p,
                                      FBE_PROVISION_DRIVE_LIFECYCLE_COND_DO_REMAP);

                fbe_lifecycle_set_cond( &fbe_provision_drive_lifecycle_const,
                                      (fbe_base_object_t *)provision_drive_p,
                                      FBE_PROVISION_DRIVE_LIFECYCLE_COND_CLEAR_EVENT_FLAG);
            }
            break;

        // invalid remap action for this chunk
        default:
            {
                // trace error
                fbe_base_object_trace( (fbe_base_object_t *)provision_drive_p,
                                       FBE_TRACE_LEVEL_WARNING,
                                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                       "%s received bad status from client: %d, invalid remap action for sniff verify i/o\n",
                                       __FUNCTION__, event_status
                                     );

                fbe_lifecycle_set_cond( &fbe_provision_drive_lifecycle_const,
                                      (fbe_base_object_t *)provision_drive_p,
                                      FBE_PROVISION_DRIVE_LIFECYCLE_COND_CLEAR_EVENT_FLAG);

                /* If a debug hook is set, we need to execute the specified action.
                 */
                fbe_provision_drive_check_hook(provision_drive_p,  
                                               SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_SNIFF_VERIFY, 
                                               FBE_PROVISION_DRIVE_SUBSTATE_SNIFF_EVENT_COMPLETED, 
                                               0, 
                                               &hook_status);
                if (hook_status == FBE_SCHED_STATUS_DONE) 
                {
                    break;
                }

                // set generic failure status; do not advance
                // verify checkpoint and update verify report
                status = FBE_STATUS_GENERIC_FAILURE;                
            }
            break;
    }

    return FBE_STATUS_OK;
}   // end fbe_provision_drive_ask_remap_action_completion()


/*!****************************************************************************
 * fbe_provision_drive_start_next_remap_io
 ******************************************************************************
 *
 * @brief
 *    This function is called after the remap event finished with FBE_EVENT_STATUS_NO_USER_DATA.
 *    It starts  allocation of a properly sized remap i/o buffer based on the disk's
 *    imported block size.
 *
 * @param   in_provision_drive_p  -  pointer to provision drive
 * @param   in_packet_p           -  pointer to control packet from the scheduler
 * 
 * @return  fbe_status_t          -  status of this operation
 *
 * @version
 *    03/30/2010 - Created. Randy Black
 *
 ******************************************************************************/

fbe_status_t
fbe_provision_drive_start_next_remap_io(
                                         fbe_provision_drive_t* in_provision_drive_p,
                                         fbe_packet_t*          in_packet_p
                                       )
{
    fbe_optimum_block_size_t         block_count;        // remap i/o block count
    fbe_memory_chunk_size_t         buffer_size;        // remap i/o buffer size
    fbe_memory_request_t*           memory_request_p;   // pointer to memory request
    fbe_status_t                    status;             // fbe status 

    // trace function entry
    FBE_PROVISION_DRIVE_TRACE_FUNC_ENTRY( in_provision_drive_p );

    // get optimum block size for remap i/o extent
    fbe_block_transport_edge_get_optimum_block_size( &in_provision_drive_p->block_edge,
                                                                                            &block_count);

    // determine buffer size for next remap i/o
    buffer_size = ( block_count == 1 ? FBE_MEMORY_CHUNK_SIZE_FOR_PACKET : FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO );

    // get pointer to packet's memory request structure
    memory_request_p = fbe_transport_get_memory_request( in_packet_p );

    // set-up memory request to allocate remap i/o buffer 
    // use the memory request priority from the packet 
    fbe_memory_build_chunk_request(memory_request_p,
                                   buffer_size,
                                   1,
                                   fbe_transport_get_resource_priority(in_packet_p),
                                   fbe_transport_get_io_stamp(in_packet_p),
                                   (fbe_memory_completion_function_t)fbe_provision_drive_remap_io_allocation_completion, /* SAFEBUG - cast to supress warning but must fix because memory completion function shouldn't return status */
                                   in_provision_drive_p
                                  );

	fbe_transport_memory_request_set_io_master(memory_request_p,  in_packet_p);

    // now, allocate remap i/o buffer
    status = fbe_memory_request_entry( memory_request_p );

    // if unable to allocate remap i/o buffer then set
    // failure status in packet and complete operation
    if ((status != FBE_STATUS_OK) && (status != FBE_STATUS_PENDING))
    {
        status = FBE_STATUS_INSUFFICIENT_RESOURCES;
        fbe_transport_set_status( in_packet_p, status, 0 );
        fbe_transport_complete_packet( in_packet_p );
        return status;
    }

    return FBE_STATUS_OK;
}   // end fbe_provision_drive_start_next_remap_io()


/*!****************************************************************************
 * fbe_provision_drive_remap_io_allocation_completion
 ******************************************************************************
 *
 * @brief
 *    This is the completion function for memory requests to allocate storage
 *    needed for a remap block i/o operation. This storage is used  to  build
 *    a remap i/o buffer and its associated sg list.
 *
 *    In addition, this function:
 *
 *    1. initializes the i/o buffer and its sg list
 *    2. sets the block operation opcode for a write verify
 *    3. establishes the remap i/o completion function
 *    4. sends the remap i/o request to the provision drive's executor
 *
 * @param   in_memory_request_p  -  pointer to memory request
 * @param   in_context           -  context, pointer to provision drive
 * 
 * @return  fbe_status_t         -  status of this operation
 *
 * @version
 *    03/30/2010 - Created. Randy Black
 *
 ******************************************************************************/

fbe_status_t
fbe_provision_drive_remap_io_allocation_completion(
                                                    fbe_memory_request_t* in_memory_request_p,
                                                    fbe_memory_completion_context_t in_context
                                                  )
{
    fbe_optimum_block_size_t              block_count;             // remap i/o block count
    fbe_payload_block_operation_opcode_t  block_opcode;            // remap i/o block opcode
    fbe_u8_t*                             buffer_p;                // remap i/o buffer
    fbe_memory_header_t*                  master_memory_header_p;  // master memory header
    fbe_packet_t*                         new_packet_p;            // new packet
    fbe_packet_t*                         packet_p;                // control packet
    fbe_provision_drive_t*                provision_drive_p;       // pointer to provision_drive
    fbe_payload_ex_t*                    sep_payload_p;           // sep payload
    fbe_payload_block_operation_t *       block_operation_p;       // block operaiton
    fbe_sg_element_t*                     sg_entry_p;              // sg list entry
    fbe_sg_element_t*                     sg_list_p;               // sg list
    fbe_lba_t                             start_lba;               // remap i/o starting lba
    fbe_status_t                          status;                  // fbe status
    fbe_xor_zero_buffer_t zero_buffer;

    // trace function entry
    FBE_PROVISION_DRIVE_TRACE_FUNC_ENTRY( in_context );

    // cast the context to provision drive object
    provision_drive_p = (fbe_provision_drive_t *) in_context;

    /* get the pointer to original packet. */
    packet_p = fbe_transport_memory_request_to_packet(in_memory_request_p);

    /* Get the payload and prev block operation. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p =  fbe_payload_ex_get_present_block_operation(sep_payload_p);

    // calculate start_lba for this remap 

    // get optimum block size for remap i/o extent
    fbe_block_transport_edge_get_optimum_block_size( &provision_drive_p->block_edge,
                                                     &block_count);
    // get lba of media error
    fbe_provision_drive_metadata_get_sniff_media_error_lba( provision_drive_p, &start_lba );
    // align remap i/o extent on an optimum block size boundary
    // for the specified media error lba
    start_lba &= ~( (fbe_lba_t)(block_count - 1) );

    /* Check allocation status */
    if (fbe_memory_request_is_allocation_complete(in_memory_request_p) == FBE_FALSE)
    {
        /* Currently this callback doesn't expect any aborted requests */
        fbe_provision_drive_utils_trace(provision_drive_p, 
                                        FBE_TRACE_LEVEL_ERROR,
                                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                        FBE_PROVISION_DRIVE_DEBUG_FLAG_VERIFY_TRACING,
                                        "SNIFF: memory request: 0x%p state: %d allocation failed \n",
                                        in_memory_request_p, in_memory_request_p->request_state);
        fbe_payload_block_set_status(block_operation_p,
                                     FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED,
                                     FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNEXPECTED_ERROR);
        fbe_provision_drive_utils_complete_packet(packet_p, FBE_STATUS_OK, 0);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    // get base of memory allocated for this i/o
    master_memory_header_p = fbe_memory_get_first_memory_header(in_memory_request_p);
    buffer_p = (fbe_u8_t *)fbe_memory_header_to_data(master_memory_header_p);

    // set-up pointers for buffer and sg list
    sg_list_p = sg_entry_p = (fbe_sg_element_t *)buffer_p;
    buffer_p  = buffer_p + ( 2 * sizeof(fbe_sg_element_t) );

    // allocate new packet and get sep payload for this i/o
    new_packet_p = fbe_transport_allocate_packet();
    fbe_transport_initialize_sep_packet( new_packet_p ) ;
    fbe_transport_add_subpacket( packet_p, new_packet_p );
    sep_payload_p = fbe_transport_get_payload_ex( new_packet_p );

    // set-up sg list for this i/o
    
    if((block_count > FBE_U32_MAX) ||
        ((block_count * FBE_BE_BYTES_PER_BLOCK) >FBE_U32_MAX))
    {
        /* we do not expect the block count to exceed 32bit limit, but we did it here!
          */
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Invalid block count for sg element:0x%x.",
                                __FUNCTION__, block_count);
        return FBE_PAYLOAD_CONTROL_STATUS_FAILURE;
    }
    fbe_sg_element_init( sg_entry_p, (fbe_u32_t)(block_count * FBE_BE_BYTES_PER_BLOCK), buffer_p );
    fbe_sg_element_increment( &sg_entry_p );
    fbe_sg_element_terminate( sg_entry_p );
    fbe_payload_ex_set_sg_list( sep_payload_p, sg_list_p, 1 );

    /* We use a zeroed buffer since 
     * if the area was already zeroed, we need to keep it that way to ensure 
     * that the client will actually get zeroed data back. 
     * If we put anything other than zeros here, it would cause a coherency error at the client. 
     */
    zero_buffer.disks = 1;
    zero_buffer.offset = 0;
    zero_buffer.status = FBE_XOR_STATUS_INVALID;
    zero_buffer.fru[0].count = block_count;
    zero_buffer.fru[0].offset = 0;
    zero_buffer.fru[0].seed = 0;
    zero_buffer.fru[0].sg_p = sg_list_p;
    status = fbe_xor_lib_zero_buffer(&zero_buffer);

    if (status != FBE_STATUS_OK) 
    { 
        fbe_provision_drive_utils_trace(provision_drive_p, 
                                        FBE_TRACE_LEVEL_ERROR,
                                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                        FBE_PROVISION_DRIVE_DEBUG_FLAG_VERIFY_TRACING,
                                        "SNIFF: remap failure to zero buffer status %d\n", status);
        fbe_payload_block_set_status(block_operation_p,
                                     FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED,
                                     FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNEXPECTED_ERROR);
        fbe_provision_drive_utils_complete_packet(packet_p, status, 0);
        return status; 
    }

    // set block operation opcode for remap i/o
    block_opcode = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_VERIFY ;

    // set remap i/o completion function
    fbe_transport_set_completion_function( new_packet_p, fbe_provision_drive_remap_io_completion, provision_drive_p );

    // set i/o traffic priority
    fbe_transport_set_traffic_priority( new_packet_p, provision_drive_p->verify_priority );

    // mark this block i/o as a remap i/o 
    fbe_transport_set_packet_attr( new_packet_p, FBE_PACKET_FLAG_BACKGROUND_SNIFF );

    fbe_provision_drive_utils_trace(provision_drive_p,
                                    FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO, 
                                    FBE_PROVISION_DRIVE_DEBUG_FLAG_SNIFF_TRACING,
                                    "SNIFF: remap lba: 0x%llx bl: 0x%x\n", start_lba, (unsigned int)block_count);
    // now, start remap i/o on this provision drive
    status = fbe_provision_drive_send_monitor_packet(
                 provision_drive_p, new_packet_p, block_opcode, start_lba, block_count );

    return status;
}   // end fbe_provision_drive_remap_io_allocation_completion()


/*!****************************************************************************
 * fbe_provision_drive_remap_media_error_lba_update_completion
 ******************************************************************************
 *
 * @brief
 *   This function handles sniff verify media error lba update completion. It completes the operation
 *   with appropriate status.
 *   
 * @param in_packet_p      - pointer to a control packet from the scheduler
 * @param in_context       - context, which is a pointer to the provision drive
 *                           object
 *
 * @version
 *    12/16/2010 - Created. Maya Dagon
 *
 ******************************************************************************/
fbe_status_t 
fbe_provision_drive_remap_media_error_lba_update_completion(
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

        return status;
        
    }

    // set FBE_STATUS_GENERIC_FAILURE status in packet , so a remap would be scheduled again
    // in fbe_provision_drive_do_remap_cond_completion (monitor code)
    //fbe_transport_set_status( in_packet_p,FBE_STATUS_ , 0 );

    return FBE_STATUS_OK;
}




/*!****************************************************************************
 * fbe_provision_drive_remap_io_completion
 ******************************************************************************
 *
 * @brief
 *    This is the completion function for a remap block i/o operation.  It is
 *    called by the provision drive's executor after it finishes a remap i/o.
 *
 * @param   in_packet_p   -  pointer to control packet from the scheduler
 * @param   in_context    -  context, which is a pointer to the base object
 * 
 * @return  fbe_status_t  - status of this operation
 *
 * @version
 *    03/30/2010 - Created. Randy Black
 *
 ******************************************************************************/

fbe_status_t
fbe_provision_drive_remap_io_completion(
                                         fbe_packet_t* in_packet_p,
                                         fbe_packet_completion_context_t in_context
                                       )
{
    fbe_payload_block_operation_t*            block_operation_p;   // block operation pointer
    fbe_payload_block_operation_qualifier_t   block_qualifier;     // block operation qualifier
    fbe_payload_block_operation_status_t      block_status;        // block operation status
    fbe_provision_drive_io_status_t           io_status;           // provision drive i/o status
    fbe_lba_t                                 media_error_lba;     // media error lba
    fbe_memory_request_t*                     memory_request_p;    // memory request pointer
    fbe_packet_t*                             master_packet_p;     // master packet
    fbe_provision_drive_t*                    provision_drive_p;   // provision drive pointer
    fbe_payload_ex_t*                        sep_payload_p;       // sep payload pointer
    fbe_status_t                              status;              // fbe status
    fbe_status_t                              transport_status;    // transport status
    fbe_block_count_t                         retry_count;         // number of times we already tried to remap this block

    // trace function entry
    FBE_PROVISION_DRIVE_TRACE_FUNC_ENTRY( in_context );

    // cast the context to provision drive object
    provision_drive_p = (fbe_provision_drive_t *) in_context;

    // get master packet
    master_packet_p = (fbe_packet_t *)fbe_transport_get_master_packet( in_packet_p );

    // get remap i/o completion status
    transport_status = fbe_transport_get_status_code( in_packet_p );

    // get remap i/o block operation in sep payload
    sep_payload_p     = fbe_transport_get_payload_ex( in_packet_p );
    block_operation_p = fbe_payload_ex_get_block_operation( sep_payload_p );

    // get remap block i/o status, qualifier, and media error lba
    fbe_payload_block_get_status( block_operation_p, &block_status );
    fbe_payload_block_get_qualifier( block_operation_p, &block_qualifier );
    fbe_payload_ex_get_media_error_lba( sep_payload_p, &media_error_lba );

    // release remap i/o block operation
    fbe_payload_ex_release_block_operation( sep_payload_p, block_operation_p );

    // release subpacket
    fbe_transport_remove_subpacket( in_packet_p );
    fbe_transport_release_packet( in_packet_p );

    // release remap i/o buffer
    memory_request_p = fbe_transport_get_memory_request( master_packet_p );
    //fbe_memory_request_get_entry_mark_free(memory_request_p, &memory_ptr);
    //fbe_memory_free_entry( memory_ptr );
	fbe_memory_free_request_entry(memory_request_p);

    // classify remap i/o completion status
    io_status = fbe_provision_drive_classify_io_status( transport_status, block_status, block_qualifier );

    // dispatch on remap i/o completion status
    switch ( io_status )
    {
        // remap i/o completed successfully
        case FBE_PROVISION_DRIVE_IO_STATUS_SUCCESS:
            {
                fbe_provision_drive_utils_trace(provision_drive_p,
                                                FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO, 
                                                FBE_PROVISION_DRIVE_DEBUG_FLAG_SNIFF_TRACING,
                                                "SNIFF: remap success lba: 0x%llx bl: 0x%llx\n", 
                                                block_operation_p->lba, block_operation_p->block_count);
                // set success status
                status = FBE_STATUS_OK;
            }
            break;

        // remap i/o completed with media errors 
        case FBE_PROVISION_DRIVE_IO_STATUS_HARD_MEDIA_ERROR:
        case FBE_PROVISION_DRIVE_IO_STATUS_SOFT_MEDIA_ERROR:
            {
                fbe_lba_t       last_media_error_lba;   // last media error lba

                // trace error in remap i/o operation
                fbe_base_object_trace( (fbe_base_object_t *)provision_drive_p,
                                       FBE_TRACE_LEVEL_WARNING,
                                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                       "SNIFF: remap i/o fail, media err at lba:0x%llx, stat:0x%x/0x%x\n",
                                       (unsigned long long)media_error_lba, transport_status, block_status);
                
                // get previous media error lba
                fbe_provision_drive_metadata_get_sniff_media_error_lba( provision_drive_p, &last_media_error_lba );
                
                // check if another error occurred while trying to remap same block 
                if ( media_error_lba == last_media_error_lba )
                {
                    fbe_provision_drive_increment_remap_retry_count(provision_drive_p);
                }
                else
                {
                    fbe_provision_drive_reset_remap_retry_count(provision_drive_p); 

                    // set completion function
                    fbe_transport_set_completion_function(master_packet_p, fbe_provision_drive_remap_media_error_lba_update_completion , provision_drive_p);    
      
                    // set new media error lba
                    status = fbe_provision_drive_metadata_set_sniff_media_error_lba( provision_drive_p,master_packet_p, media_error_lba );
                                                             
                    return FBE_STATUS_MORE_PROCESSING_REQUIRED;                                                          
                }
                status = FBE_STATUS_OK ;
      
            }
            break;

        // remap i/o completed with unexpected errors
        case FBE_PROVISION_DRIVE_IO_STATUS_ERROR:
        default:
           {
                // trace error in remap i/o operation
                fbe_base_object_trace( (fbe_base_object_t *)provision_drive_p,
                                       FBE_TRACE_LEVEL_WARNING,
                                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                       "pvd_remap_io_compl remap i/o fail, stat:0x%x/0x%x\n",
                                       transport_status, block_status
                                     );
                fbe_provision_drive_increment_remap_retry_count(provision_drive_p);
                status = FBE_STATUS_GENERIC_FAILURE;
           }
            break;
    }

    fbe_provision_drive_get_remap_retry_count(provision_drive_p,&retry_count);
    if (retry_count >= FBE_PROVISION_DRIVE_MAX_REMAP_IO_RETRIES)
    {     
        fbe_provision_drive_reset_remap_retry_count(provision_drive_p);  
        status = fbe_provision_drive_advance_verify_checkpoint(provision_drive_p,master_packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }
     
    // set remap i/o status in packet and complete operation
    fbe_transport_set_status( master_packet_p, status, 0 );
 
    fbe_transport_complete_packet( master_packet_p );

    return status;

}   // end fbe_provision_drive_remap_io_completion()

