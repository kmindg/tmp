 /***************************************************************************
  * Copyright (C) EMC Corporation 2007-2010
  * All rights reserved.
  * Licensed material -- property of EMC Corporation
  ***************************************************************************/

 /*!**************************************************************************
  * @file fbe_extent_pool_lun_executor.c
  ***************************************************************************
  *
  * @brief
  *  This file contains the executer functions for the lun.
  * 
  *  This includes the fbe_ext_pool_lun_io_entry() function which is our entry
  *  point for functional packets.
  * 
  * @ingroup lun_class_files

  * 
  * @version
  *   05/20/2009:  Created. RPF
  *
  ***************************************************************************/
 
 /*************************
  *   INCLUDE FILES
  *************************/
#include "fbe_ext_pool_lun_private.h"
#include "fbe_service_manager.h"
#include "fbe_traffic_trace.h"
#include "fbe_transport_memory.h"
#include "VolumeAttributes.h"
#include "fbe_cmi.h"
#include "fbe_database.h"
#include "EmcPAL_Misc.h"
#include "fbe_private_space_layout.h"
#include "fbe_raid_library_proto.h"

 /*************************
  *   FUNCTION DEFINITIONS
  *************************/

static void fbe_ext_pool_lun_notify_of_first_write(fbe_ext_pool_lun_t*  in_lun_p, fbe_payload_block_operation_t *block_operation);

static void
fbe_ext_pool_lun_read_write_dirty_flag_allocate_completion(fbe_memory_request_t * memory_request_p, 
                                              fbe_memory_completion_context_t context);

static fbe_status_t fbe_ext_pool_lun_get_dirty_flag_offset(fbe_ext_pool_lun_t*      in_lun_p,
                                           fbe_cmi_sp_id_t in_sp_id,
                                           fbe_lba_t *     out_lba);
static fbe_status_t fbe_ext_pool_lun_send_func_packet(fbe_ext_pool_lun_t *lun_p, fbe_packet_t *packet_p);



static fbe_status_t fbe_ext_pool_lun_split_and_send_func_packet(fbe_ext_pool_lun_t *lun_p, fbe_packet_t *packet_p);
static void fbe_ext_pool_lun_split_and_send_func_packet_allocate_completion(fbe_memory_request_t * memory_request_p, fbe_memory_completion_context_t context);
static fbe_status_t fbe_ext_pool_lun_split_and_send_func_packet_completion(fbe_packet_t * packet, fbe_packet_completion_context_t in_context);
static void fbe_ext_pool_lun_traffic_trace(fbe_ext_pool_lun_t *lun_p,
                                  fbe_packet_t *packet_p,
                                  fbe_payload_block_operation_t *block_operation_p,
                                  fbe_bool_t b_start);
/*************************
  *   Globals
  *************************/


/*!**************************************************************
 * fbe_ext_pool_lun_block_transport_entry()
 ****************************************************************
 * @brief
 *   This is the entry function that the block transport will
 *   call to pass a packet to the lun.
 *  
 * @param context - The lun ptr. 
 * @param packet_p - The packet to process.
 * 
 * @return fbe_status_t
 *
 * @author
 *  05/20/2009 - Created. RPF
 *
 ****************************************************************/

fbe_status_t fbe_ext_pool_lun_block_transport_entry(fbe_transport_entry_context_t context, 
                                           fbe_packet_t * packet_p)
{
    fbe_status_t status;
    fbe_ext_pool_lun_t * lun_p = NULL;

    lun_p = (fbe_ext_pool_lun_t *)context;

    /* Simply hand this off to the block transport handler.
     */
    status = fbe_ext_pool_lun_send_io_packet(lun_p, packet_p);
    return status;
}
/**************************************
 * end fbe_ext_pool_lun_block_transport_entry()
 **************************************/

/****************************************************************
 * fbe_ext_pool_lun_send_io_packet()
 ****************************************************************
 * @brief
 *  This function sends an IO packet.
 *
 * @param lun_p - Lun to send for.
 * @param packet_p - io packet to send.
 *
 * @return
 *  Status of the send.
 *
 * @author
 *  05/22/09 - Created. RPF
 * @Notes
 *   02/04/10 - Modified by Ashwin
 *     Added code to support BV due to dirty lun on a
 *     non-cached write io
 *
 ****************************************************************/
fbe_status_t fbe_ext_pool_lun_send_io_packet(fbe_ext_pool_lun_t * const lun_p,
                                    fbe_packet_t *packet_p)
{
    fbe_status_t           status;
    fbe_block_edge_t       *edge_p = NULL;
    fbe_payload_ex_t*     payload_p;
    fbe_payload_block_operation_t* block_operation_p;  


    /* Now that packet is in Lun, update its resource priority to LUN's base priority.
     * Setting Priority here as io_entry and block_transport_entry both funnell thru here.
     */
    fbe_ext_pool_lun_class_set_resource_priority(packet_p);

    payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_get_block_operation(payload_p);

    /*
     * IO is going to start, note down the start time, to calculate cumulative read and write time after io completion.
     * Also calculate number of arrivals when queue was not empty, and sum of queue length on arrival
     */
    if (lun_p->b_perf_stats_enabled)
    {
        fbe_payload_ex_set_start_time(payload_p, fbe_get_time());
    }
    else
    {
        /* Set to 0 so if we enable perf stats while I/O is outstanding, we just skip adding the time for this I/O.
         */
        fbe_payload_ex_set_start_time(payload_p, 0);
    }
    /*
     * trace to RBA buffer
     */
    if (fbe_traffic_trace_is_enabled (KTRC_TFBE_LUN))
    {
        fbe_ext_pool_lun_traffic_trace(lun_p,
                              packet_p,
                              block_operation_p,
                              FBE_TRUE /*RBA trace start*/);
    }


	/*we need to generate an event to cache for the very first time the LUN has been written*/
	fbe_ext_pool_lun_notify_of_first_write(lun_p, block_operation_p);
    
    /* Check whether it is a non cached write or not.
       If it is, queue the io's and check whether the lu flag is dirty
    */
    if(block_operation_p->block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_NONCACHED) {        
        status = fbe_ext_pool_lun_set_dirty_flag_metadata(packet_p, lun_p);
        return status;                                   
    }

    /* Send out the io packet. Send this to our only edge.
       Set the I/O completion function and send the I/O.
    */
    fbe_lun_get_block_edge(lun_p, &edge_p);
    fbe_transport_set_completion_function(packet_p, fbe_ext_pool_lun_io_completion, lun_p);
    
#if 0 /* This is future enhancement */
	fbe_ext_pool_lun_metadata_get_zero_checkpoint(lun_p, &lu_zero_checkpoint);
    fbe_base_config_get_capacity((fbe_base_config_t *) lun_p, &exported_capacity);
	if(lu_zero_checkpoint >= exported_capacity){ /* The LUN is fully zeroed */
		//fbe_transport_set_packet_attr(packet_p, FBE_PACKET_FLAG_CONSUMED);
	}
#endif

    status = fbe_ext_pool_lun_send_func_packet(lun_p, packet_p);
    return status;
}
/* end fbe_ext_pool_lun_send_io_packet() */
/*!***************************************************************
 * fbe_ext_pool_lun_io_entry()
 ****************************************************************
 * @brief
 *  This function is the entry point for
 *  functional transport operations to the lun object.
 *  The FBE infrastructure will call this function for our object.
 *
 * @param object_handle - The lun handle.
 * @param packet_p - The io packet that is arriving.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_GENERIC_FAILURE if transport is unknown,
 *                 otherwise we return the status of the transport handler.
 *
 * @author
 *  05/22/09 - Created. RPF
 *
 ****************************************************************/
fbe_status_t 
fbe_ext_pool_lun_io_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet_p)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_ext_pool_lun_t * lun_p = NULL;
    fbe_payload_ex_t * payload_p = NULL;
    fbe_payload_block_operation_t * block_operation_p = NULL;
    fbe_path_state_t path_state = FBE_PATH_STATE_INVALID;
    fbe_cpu_id_t cpu_id = FBE_CPU_ID_INVALID;
    fbe_lifecycle_state_t lifecycle_state;
    lun_p = (fbe_ext_pool_lun_t *) fbe_base_handle_to_pointer(object_handle);

	payload_p = fbe_transport_get_payload_ex(packet_p);
	block_operation_p = fbe_payload_ex_get_block_operation(payload_p);

    /* For system LUNs we send I/Os directly to the LUN. 
     * This causes issues if we allow I/O to start if we are not ready yet. 
     * For example, if I/Os are trying to get started when we are trying to read the dirty information 
     * we can end up with an inconsistent io counter. 
     */
    if (fbe_private_space_layout_object_id_is_system_lun(lun_p->base_config.base_object.object_id))
    {
        fbe_base_object_get_lifecycle_state((fbe_base_object_t*)lun_p, &lifecycle_state);
        if (lifecycle_state != FBE_LIFECYCLE_STATE_READY)
        {
            fbe_base_object_trace((fbe_base_object_t*) lun_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: lifecycle state is %d fail packet: %p lba: 0x%llx bl: 0x%llx\n", 
                                  __FUNCTION__, lifecycle_state, packet_p, block_operation_p->lba, block_operation_p->block_count);
    
            fbe_transport_set_status(packet_p, FBE_STATUS_EDGE_NOT_ENABLED, __LINE__);
            fbe_transport_run_queue_push_packet(packet_p, FBE_TRANSPORT_RQ_METHOD_SAME_CORE);
            return FBE_STATUS_OK;
        }
    }
    /* Currently we only support block commands.
     */
    if (block_operation_p != NULL)
    { 
        if (packet_p->base_edge)
        {
            fbe_block_transport_get_path_state((fbe_block_edge_t*)packet_p->base_edge, &path_state);
        }

		if(lun_p->write_bypass_mode){
			if(block_operation_p->block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_NONCACHED ||
				block_operation_p->block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE){

		        fbe_payload_block_set_status(block_operation_p,
                                     FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS,
                                     FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);

				fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
				fbe_transport_run_queue_push_packet(packet_p, FBE_TRANSPORT_RQ_METHOD_SAME_CORE);
				return FBE_STATUS_OK;
			}
		}
        /* Simply set the cancel function and send this off to the server.
         * We need to go through the server since it is counting our I/Os. 
         * When we transition to pending the server handles keeping us in 
         * the pending state until I/Os have been drained. 
         */
        fbe_transport_set_cancel_function(packet_p, 
                                          fbe_base_object_packet_cancel_function, 
                                          (fbe_base_object_t *)lun_p);
        /*
        status = fbe_block_transport_server_bouncer_entry(&lun_p->base_config.block_transport_server,
                                                          packet_p,
                                                          lun_p );
        */

        /*
         * Set performance statistics structure into block payload here, if perf statistics enabled by user
         */
        if (lun_p->b_perf_stats_enabled)
        {
            fbe_u32_t                   outstanding_io;
            fbe_transport_get_cpu_id(packet_p, &cpu_id);
            fbe_payload_ex_set_lun_performace_stats_ptr(payload_p, lun_p->performance_stats.counter_ptr.lun_counters);
            /* Sum of queue length on arrival*/
            fbe_block_transport_server_get_outstanding_io_count(&((fbe_base_config_t *)lun_p)->block_transport_server, &outstanding_io);
            /*outstanding_io present in block transport does not include current io*/
            fbe_ext_pool_lun_perf_stats_inc_sum_q_length_arrival(lun_p, outstanding_io, cpu_id);
            /* number of arrivals with non zero queue*/
            if (outstanding_io > 1)
            {
                 fbe_ext_pool_lun_perf_stats_inc_arrivals_to_nonzero_q(lun_p, cpu_id);
            }
        }
        status = fbe_base_config_bouncer_entry((fbe_base_config_t *)lun_p, packet_p);
		if(status == FBE_STATUS_OK){ /* Small stack */
			status = fbe_ext_pool_lun_send_io_packet(lun_p, packet_p);
		}

    }
    else
    {
        /* We do not support this transport.
         */
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        status = fbe_transport_complete_packet(packet_p);
    }
    return status;
}
/* end fbe_ext_pool_lun_io_entry() */

/*!****************************************************************************
 * fbe_ext_pool_lun_noncached_io_completion()
 ******************************************************************************
 * @brief
 *  This is the completion function for the lun non cached io. Whenever the io 
 *  noncached IO completes, decrement the io counter and if the counter is 0 
 *  then clear the dirt flag
 *
 * @param packet_p - The packet sent to us from the scheduler.
 * @param context  - The pointer to the object.
 *
 * @return fbe_status_t
 *
 * @author
 *  02/04/2010 - Created. Ashwin
 *
 *****************************************************************************/
fbe_status_t fbe_ext_pool_lun_noncached_io_completion(fbe_packet_t* in_packet_p, 
                                             fbe_packet_completion_context_t in_context)
{
    fbe_ext_pool_lun_t* lun_p;
    fbe_payload_block_operation_status_t  block_status;
    fbe_payload_block_operation_qualifier_t block_qualifier;
    fbe_status_t  packet_status;
    fbe_status_t  status = FBE_STATUS_OK;
    fbe_cpu_id_t cpu_id = FBE_CPU_ID_INVALID;

    fbe_payload_ex_t *payload_p = fbe_transport_get_payload_ex(in_packet_p);
    fbe_payload_block_operation_t *block_operation_p = fbe_payload_ex_get_block_operation(payload_p);

    lun_p = (fbe_ext_pool_lun_t*)in_context;
    
    /*
     * trace to RBA buffer
     */
    if (fbe_traffic_trace_is_enabled (KTRC_TFBE_LUN))
    {
        fbe_ext_pool_lun_traffic_trace(lun_p,
                              in_packet_p,
                              block_operation_p,
                              FBE_FALSE /*RBA trace end*/);
    }

    /*
     * IO has been completed, update performance couters
     */
    if (lun_p->b_perf_stats_enabled)
    {
        fbe_transport_get_cpu_id(in_packet_p, &cpu_id);
        fbe_ext_pool_lun_perf_stats_update_perf_counters_io_completion(lun_p,
                                                              block_operation_p,
                                                              cpu_id,
                                                              payload_p);
    }
    fbe_payload_block_get_status(block_operation_p, &block_status);
    fbe_payload_block_get_qualifier(block_operation_p, &block_qualifier);
    packet_status = fbe_transport_get_status_code(in_packet_p);

    /* On certain failure status values we intentionally do not update the I/O counters so that
     * we will require the background verify. 
     * We ignore congested since this is status that indicates the write did not even start. 
     */
    if( (packet_status != FBE_STATUS_OK) || 
       ( (block_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS) &&
         (block_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_REQUEST_ABORTED) &&
         (block_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST)  &&
         (block_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR) &&
         /* The below means not congested status. */
         !((block_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED) &&
           (block_qualifier == FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_CONGESTED)) ) )
    {
        fbe_base_object_trace((fbe_base_object_t*) lun_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                             "%s: Packet status: 0x%x or block status: 0x%x is not ok\n", 
                              __FUNCTION__, packet_status, block_status);
        return FBE_STATUS_OK;
    }

    // If the io counter is greater than 1 that means this is not the last io.
    // Just decrement the io counter  
    fbe_spinlock_lock(&lun_p->io_counter_lock);

    if(lun_p->io_counter > 1)
    {
        lun_p->io_counter--;  
        
        fbe_spinlock_unlock(&lun_p->io_counter_lock);
		
		fbe_base_object_trace((fbe_base_object_t*)lun_p, FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO,
                              "LUN There are still outstanding IOs; we're staying dirty. dcnt: %d\n", lun_p->io_counter);

        return FBE_STATUS_OK;
    }

    // If the io counter is 1 that means this is the last io
    // Update the NP metadata
    else if(lun_p->io_counter == 1)
    {

        lun_p->clean_time = fbe_get_time();
        lun_p->io_counter--;

        if (fbe_lun_is_clean_dirty_enabled(lun_p)) {
            status = fbe_lifecycle_set_cond(&fbe_ext_pool_lun_lifecycle_const,
                                            (fbe_base_object_t*)lun_p,
                                            FBE_LUN_LIFECYCLE_COND_LAZY_CLEAN);
        }

        fbe_spinlock_unlock(&lun_p->io_counter_lock);

        fbe_base_object_trace((fbe_base_object_t*)lun_p, 
                              FBE_TRACE_LEVEL_DEBUG_HIGH,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "LUN Setting FBE_LUN_LIFECYCLE_COND_LUN_MARK_LUN_CLEAN condition dcnt: %d\n", lun_p->io_counter);

        if(status != FBE_STATUS_OK) {
            fbe_base_object_trace((fbe_base_object_t*)lun_p, 
                                  FBE_TRACE_LEVEL_ERROR, 
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s, failed to set condition\n", __FUNCTION__);
        }

        return status;
    }
    else
    {
        /* This should be unreachable */
        fbe_spinlock_unlock(&lun_p->io_counter_lock);
        fbe_base_object_trace((fbe_base_object_t*)lun_p, 
                              FBE_TRACE_LEVEL_CRITICAL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, io_counter was about to go negative!\n", __FUNCTION__);
        return(FBE_STATUS_OK);
    }
}
/* end fbe_ext_pool_lun_noncached_io_completion */

/*!****************************************************************************
 * fbe_ext_pool_lun_io_completion()
 ******************************************************************************
 * @brief
 *  This is the completion function for the lun io. Whenever the io completes
 *  decrement the io counter and if the counter is 0 then clear the dirt flag
 *
 * @param packet_p - The packet sent to us from the scheduler.
 * @param context  - The pointer to the object.
 *
 * @return fbe_status_t
 *
 * @author
 *  02/10/2011 - Created. Swati Fursule
 *
 *****************************************************************************/
fbe_status_t fbe_ext_pool_lun_io_completion(fbe_packet_t* in_packet_p, 
                                   fbe_packet_completion_context_t in_context)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_ext_pool_lun_t* lun_p;
    fbe_payload_ex_t              *payload_p;
    fbe_payload_block_operation_t  *block_operation_p;
    fbe_status_t                    packet_status;
    fbe_payload_block_operation_status_t block_status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID;
    fbe_cpu_id_t cpu_id = FBE_CPU_ID_INVALID;
    
    /* Get the packet and block payload
     */
    lun_p = (fbe_ext_pool_lun_t*)in_context;
    payload_p = fbe_transport_get_payload_ex(in_packet_p);
    packet_status = fbe_transport_get_status_code(in_packet_p);
    block_operation_p = fbe_payload_ex_get_block_operation(payload_p);

    /* If the packet didn't complete (going shutdown etc) trace a warning and
     * return
     */
    if (packet_status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*) lun_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: Failed to complete packet with status: 0x%x \n", 
                              __FUNCTION__, packet_status);
        return FBE_STATUS_OK;
    }

    /* A NULL block operation is unexpected.  Trace and error and return.
     */
    if (block_operation_p == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*) lun_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: block operation: 0x%p is NULL\n", 
                              __FUNCTION__, block_operation_p);
        return FBE_STATUS_OK;
    }

    /* Now get the block operation status
     */
    fbe_payload_block_get_status(block_operation_p, &block_status);

    /*
     * trace to RBA buffer
     */
    if (fbe_traffic_trace_is_enabled (KTRC_TFBE_LUN))
    {
        fbe_ext_pool_lun_traffic_trace(lun_p,
                              in_packet_p,
                              block_operation_p,
                              FBE_FALSE /*RBA trace end*/);
    }
    
    /*
     * IO has been completed, update performace couters
     */
    if (lun_p->b_perf_stats_enabled)
    {
        fbe_transport_get_cpu_id(in_packet_p, &cpu_id);
        fbe_ext_pool_lun_perf_stats_update_perf_counters_io_completion(lun_p,
                                                              block_operation_p,
                                                              cpu_id,
                                                              payload_p);
    }

     
    return status;
}
/* end fbe_ext_pool_lun_io_completion */

/*!****************************************************************************
 * fbe_ext_pool_lun_set_dirty_flag_metadata()
 ******************************************************************************
 * @brief
 *  This is used to update the metada. It increments the io counters
 *  and sets the dirty flag to be set. Dirty flag will be used
 *  if the RG goes down and once it comes back we will check this flag
 *  and if the flag is set we initiate a background verify
 *
 * @param lun_p - The lun object
 * @param packet_p 
 *
 * @return fbe_status_t
 *
 * @author
 *  06/10/2010 - Created. Ashwin
 * 
 *
 *****************************************************************************/
fbe_status_t fbe_ext_pool_lun_set_dirty_flag_metadata(fbe_packet_t* in_packet_p,
                                             fbe_ext_pool_lun_t*  in_lun_p)                                   
{

    fbe_status_t                            status;            
    fbe_lun_clean_dirty_context_t *         context = NULL;

    fbe_spinlock_lock(&in_lun_p->io_counter_lock);        
    
    // If we're pending clean or pending dirty, we need to queue the packet,
    // and it will be restarted by the completion of the clean or dirty operation.
    if((in_lun_p->clean_pending == TRUE) || (in_lun_p->dirty_pending == TRUE))
    {
        fbe_base_object_trace((fbe_base_object_t*) in_lun_p,
                             FBE_TRACE_LEVEL_DEBUG_LOW,
                             FBE_TRACE_MESSAGE_ID_INFO,
                             "%s: Pending flags are set. Add the packet to terminator queue\n", __FUNCTION__);

        fbe_ext_pool_lun_add_to_terminator_queue(in_lun_p, in_packet_p);
        fbe_spinlock_unlock(&in_lun_p->io_counter_lock);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }
    // If this is the first io for this lun increment the io counter
    // set the dirty pending flag and update the non paged metadata
    else if((in_lun_p->io_counter == 0) && (! in_lun_p->marked_dirty))
    {
        in_lun_p->dirty_pending = TRUE;
        in_lun_p->clean_time = 0;
        in_lun_p->io_counter++;
        fbe_spinlock_unlock(&in_lun_p->io_counter_lock);

        fbe_base_object_trace((fbe_base_object_t*) in_lun_p,
                             FBE_TRACE_LEVEL_DEBUG_LOW,
                             FBE_TRACE_MESSAGE_ID_INFO,
                             "LUN  We're the first IO to dirty. Update paged metadata.\n");
                
        // Set the dirty flag in the paged metadata
        context = &in_lun_p->clean_dirty_executor_context;

        context->lun_p = in_lun_p;
        context->sp_id = FBE_CMI_SP_ID_INVALID;
        context->dirty = FBE_TRUE;

        status = fbe_ext_pool_lun_update_dirty_flag(context, in_packet_p,
                                           fbe_ext_pool_lun_set_dirty_flag_completion);
        if (status == FBE_STATUS_OK)
        {
            return FBE_STATUS_MORE_PROCESSING_REQUIRED;
        }    
    }
    // If the io counter is >=1 or we haven't lazy-cleaned,
    // just increment the counter and send the packet down
    else if((in_lun_p->io_counter >=1) || in_lun_p->marked_dirty)
    {
        fbe_base_object_trace((fbe_base_object_t*) in_lun_p,
                             FBE_TRACE_LEVEL_DEBUG_HIGH,
                             FBE_TRACE_MESSAGE_ID_INFO,
                             "LUN is already marked dirty in MD. Sending the packet down. dcnt: %d\n", in_lun_p->io_counter);

        in_lun_p->io_counter++;
        in_lun_p->clean_time = 0;
        fbe_spinlock_unlock(&in_lun_p->io_counter_lock);               

        status = fbe_ext_pool_lun_send_packet(in_lun_p, in_packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    return FBE_STATUS_OK;
                 

}/* end fbe_ext_pool_lun_set_dirty_flag_metadata*/

/*!****************************************************************************
 * fbe_ext_pool_lun_set_dirty_flag_completion()
 ******************************************************************************
 * @brief
 *  Once the metadata update has been completed, dequeue the packet from the
 *  terminator queue and send the io packet down
 *
 * @param in_packet_p - The packet pointer
 * @param in_context - completion context
 *
 * @return fbe_status_t
 *
 * @author
 *  09/22/2010 - Created. Ashwin
 * 
 *
 *****************************************************************************/
fbe_status_t fbe_ext_pool_lun_set_dirty_flag_completion(fbe_packet_t*  in_packet_p,
                                               fbe_packet_completion_context_t in_context)                                   
{
    fbe_ext_pool_lun_t*                              lun_p               = NULL;
    fbe_status_t                            status;
    fbe_status_t                            packet_status;    
    fbe_queue_head_t *                      terminator_queue_p  = NULL;
    fbe_lun_clean_dirty_context_t *         context             = NULL;
    fbe_packet_t *                          next_packet_p       = NULL;
    fbe_packet_t *                          restart_packet_p    = NULL;
    fbe_queue_element_t *                   next_element_p      = NULL;
    fbe_payload_ex_t *                      payload_p           = NULL;
    fbe_payload_block_operation_t *         block_operation_p   = NULL;
    fbe_payload_block_operation_qualifier_t block_qualifier;
    fbe_payload_block_operation_status_t    block_status;

    lun_p = (fbe_ext_pool_lun_t*)in_context;

    //  Get the status of the packet
    packet_status = fbe_transport_get_status_code(in_packet_p);
    payload_p = fbe_transport_get_payload_ex(in_packet_p);
    block_operation_p = fbe_payload_ex_get_block_operation(payload_p);

    fbe_payload_block_get_status(block_operation_p, &block_status);
    fbe_payload_block_get_qualifier(block_operation_p, &block_qualifier);

    /*! The dirty write failed. 
     * This packet will be failed back. 
     * We will kick start the next request to attempt to dirty again. We intentionally do not try to differentiate
     * between status values here.  In the case where we are going failed, the
     * monitor will drain the queue and we will find nothing else here to restart.
     */
    if ((packet_status != FBE_STATUS_OK) ||
        (block_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS))
    {
        fbe_spinlock_lock(&lun_p->io_counter_lock);
        fbe_spinlock_lock(&((fbe_base_object_t*)lun_p)->terminator_queue_lock);
        terminator_queue_p = fbe_base_object_get_terminator_queue_head((fbe_base_object_t*)lun_p);

        fbe_base_object_trace((fbe_base_object_t*) lun_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "LUN Dirty flag write failed pkt: %p pkt_st: 0x%x block_st: 0x%x tqempty: %d\n", 
                              in_packet_p, packet_status, block_status, fbe_queue_is_empty(terminator_queue_p));

        terminator_queue_p      = fbe_base_object_get_terminator_queue_head((fbe_base_object_t*)lun_p);
        next_element_p          = fbe_queue_front(terminator_queue_p);

        while(next_element_p != NULL)
        {
            next_packet_p   = fbe_transport_queue_element_to_packet(next_element_p);
            next_element_p  = fbe_queue_next(terminator_queue_p, &next_packet_p->queue_element);

            /* Only dispatch is there's no subpacket */
            if(fbe_queue_is_empty(&next_packet_p->subpacket_queue_head))
            {
                restart_packet_p = next_packet_p;
                break;
            }
        }

        if(restart_packet_p == NULL)
        {
            /* We simply need to transition to no dirty pending.
             */
            lun_p->dirty_pending = FBE_FALSE;
            lun_p->clean_time = 0;
            lun_p->io_counter--;
            fbe_spinlock_unlock(&((fbe_base_object_t*)lun_p)->terminator_queue_lock);
            fbe_spinlock_unlock(&lun_p->io_counter_lock);
        }
        else
        {
            /* There is at least one thing on the queue.  Kick off the head to perform the dirty.
             */
            status = fbe_transport_remove_packet_from_queue(restart_packet_p);

            if (status != FBE_STATUS_OK) 
            { 
                fbe_base_object_trace((fbe_base_object_t*) lun_p, FBE_TRACE_LEVEL_CRITICAL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                      "LUN set dirty flag compl fail remove term q pkt: %p status: 0x%x\n",
                                      restart_packet_p, status);
                fbe_transport_set_status(restart_packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
                fbe_transport_complete_packet(restart_packet_p);
                return status; 
            }
            fbe_spinlock_unlock(&((fbe_base_object_t*)lun_p)->terminator_queue_lock);
            fbe_spinlock_unlock(&lun_p->io_counter_lock);

            fbe_base_object_trace((fbe_base_object_t*) lun_p,
                                  FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "LUN Reissue dirty request packet: %p\n", restart_packet_p);
                
            context = &lun_p->clean_dirty_executor_context;

            context->lun_p = lun_p;
            context->sp_id = FBE_CMI_SP_ID_INVALID;
            context->dirty = FBE_TRUE;
            
            status = fbe_ext_pool_lun_update_dirty_flag(context, restart_packet_p, fbe_ext_pool_lun_set_dirty_flag_completion);

        }
        return packet_status;
    }

    fbe_base_object_trace((fbe_base_object_t*) lun_p,
                         FBE_TRACE_LEVEL_DEBUG_HIGH,
                         FBE_TRACE_MESSAGE_ID_INFO,
                         "%s: Dirty flag has been set\n", __FUNCTION__);

    status = fbe_ext_pool_lun_handle_queued_packets_waiting_for_set_dirty_flag(lun_p,in_packet_p);

    return status;
            
    
}/* end fbe_ext_pool_lun_set_dirty_flag_completion*/

/*!****************************************************************************
 * fbe_ext_pool_lun_send_packet()
 ******************************************************************************
 * @brief
 *  Set the completion function for the packet and send it down
 * 
 *
 * @param lun_p - The lun object
 * @param packet_p 
 *
 * @return fbe_status_t
 *
 * @author
 *  09/21/2010 - Created. Ashwin Tamilarasan
 * 
 *
 *****************************************************************************/
fbe_status_t fbe_ext_pool_lun_send_packet( fbe_ext_pool_lun_t*  in_lun_p,
                                  fbe_packet_t*  in_packet_p)                                   
{

    fbe_block_edge_t       *edge_p = NULL;
    fbe_status_t           status;

    // Send the current packet down
    fbe_lun_get_block_edge(in_lun_p, &edge_p);    
    fbe_transport_set_completion_function(in_packet_p, fbe_ext_pool_lun_noncached_io_completion, in_lun_p);
    status = fbe_ext_pool_lun_send_func_packet(in_lun_p, in_packet_p);
    return status;  

} // fbe_ext_pool_lun_send_packet



/*!****************************************************************************
 * fbe_ext_pool_lun_handle_queued_packets_waiting_for_clear_dirty_flag()
 ******************************************************************************
 * @brief
 *  This function will restart all the packets in the terminator queue
 * 
 * 
 *
 * @param lun_p - The lun object 
 *
 * @return fbe_status_t
 *
 * @author
 *  09/21/2010 - Created. Ashwin Tamilarasan
 * 
 *
 *****************************************************************************/
fbe_status_t fbe_ext_pool_lun_handle_queued_packets_waiting_for_clear_dirty_flag(fbe_ext_pool_lun_t*  lun_p)
                                                                                                      
{
    fbe_queue_head_t       dirty_pending_queue;
    fbe_packet_t*          new_packet_p = NULL;
    fbe_status_t           status;
    fbe_queue_head_t *     termination_queue_p = NULL;
    fbe_queue_element_t   *next_element_p = NULL;
    fbe_queue_element_t   *queue_element_p = NULL;


    // Queue to hold the packets that are removed from terminator queue
    // The reason is we want to empty out the terminator queue before we process the packets
    fbe_queue_init(&dirty_pending_queue);

    fbe_spinlock_lock(&((fbe_base_object_t*)lun_p)->terminator_queue_lock);

    // If the queue is empty just return ok so that the 
    // packet will complete itself
    if(fbe_queue_is_empty(&((fbe_base_object_t*)lun_p)->terminator_queue_head))
    {
        fbe_spinlock_unlock(&((fbe_base_object_t*)lun_p)->terminator_queue_lock);
        return FBE_STATUS_OK;    
        
    }

	termination_queue_p = fbe_base_object_get_terminator_queue_head((fbe_base_object_t*)lun_p);
    queue_element_p = fbe_queue_front(termination_queue_p);

   // Dequeue all the packets from the terminator queue and put it in a temp queue
    // This will increment the io counter too    
    while(queue_element_p != NULL) {
        new_packet_p = fbe_transport_queue_element_to_packet(queue_element_p);
        next_element_p = fbe_queue_next(termination_queue_p, &new_packet_p->queue_element);

        /* Only dispatch is there's no subpacket */
        if(fbe_queue_is_empty(&new_packet_p->subpacket_queue_head)) {
            /* remove from the terminator queue */
            fbe_transport_remove_packet_from_queue(new_packet_p);

            fbe_transport_enqueue_packet(new_packet_p, &dirty_pending_queue);
        }

        queue_element_p = next_element_p;

    }


    fbe_spinlock_unlock(&((fbe_base_object_t*)lun_p)->terminator_queue_lock);

    fbe_base_object_trace((fbe_base_object_t*) lun_p,
                             FBE_TRACE_LEVEL_DEBUG_HIGH,
                             FBE_TRACE_MESSAGE_ID_INFO,
                             "%s: Transferred packets from terminator queue to temporary queue\n", __FUNCTION__);

    // Process the temp queue by removing the packet and sending it to fbe_ext_pool_lun_set_iocounters_and_dirty_flag_metadata
    // so that io counters get incremented and the packet is sent down
    while(!fbe_queue_is_empty(&dirty_pending_queue)) 
    {
        new_packet_p = fbe_transport_dequeue_packet(&dirty_pending_queue);
        status = fbe_ext_pool_lun_set_dirty_flag_metadata(new_packet_p,lun_p);        

    }

    return FBE_STATUS_OK;         

} // fbe_ext_pool_lun_handle_queued_packets_waiting_for_clear_dirty_flag



/*!****************************************************************************
 * fbe_ext_pool_lun_handle_queued_packets_waiting_for_set_dirty_flag()
 ******************************************************************************
 * @brief
 *  This function will restart all the packets in the terminator queue
 * 
 * 
 *
 * @param lun_p - The lun object 
 *
 * @return fbe_status_t
 *
 * @author
 *  09/21/2010 - Created. Ashwin Tamilarasan
 * 
 *
 *****************************************************************************/
fbe_status_t fbe_ext_pool_lun_handle_queued_packets_waiting_for_set_dirty_flag(fbe_ext_pool_lun_t*  lun_p,
                                                                      fbe_packet_t* in_packet_p)
                                                                                                      
{
    fbe_queue_head_t       dirty_pending_queue;
    fbe_packet_t*          new_packet_p = NULL;
    fbe_status_t           status;
    fbe_queue_head_t *     termination_queue_p = NULL;
    fbe_queue_element_t   *next_element_p = NULL;
    fbe_queue_element_t   *queue_element_p = NULL;


    // Queue to hold the packets that are removed from terminator queue
    // The reason is we want to empty out the terminator queue before we process the packets
    fbe_queue_init(&dirty_pending_queue);

    // The metadata update has completed successfully
    // Clear the dirty pending flag and send the current packet down
    fbe_spinlock_lock(&lun_p->io_counter_lock);
    lun_p->dirty_pending = FALSE;
    lun_p->marked_dirty = TRUE;
           
    fbe_spinlock_lock(&((fbe_base_object_t*)lun_p)->terminator_queue_lock);

    termination_queue_p = fbe_base_object_get_terminator_queue_head((fbe_base_object_t*)lun_p);
    queue_element_p = fbe_queue_front(termination_queue_p);

    // Dequeue all the packets from the terminator queue and put it in a temp queue
    // This will increment the io counter too    
    while(queue_element_p != NULL)
    {
        new_packet_p = fbe_transport_queue_element_to_packet(queue_element_p);
        next_element_p = fbe_queue_next(termination_queue_p, &new_packet_p->queue_element);

        /* Only dispatch is there's no subpacket */
        if(fbe_queue_is_empty(&new_packet_p->subpacket_queue_head))
        {
            /* remove from the terminator queue */
            status = fbe_transport_remove_packet_from_queue(new_packet_p);
            if (status == FBE_STATUS_OK)
            {
                status = fbe_transport_enqueue_packet(new_packet_p, &dirty_pending_queue);
            }
            if (status == FBE_STATUS_OK)
            {
                lun_p->io_counter++;
            }
            else
            {
                fbe_base_object_trace((fbe_base_object_t*) lun_p,
                                     FBE_TRACE_LEVEL_ERROR,
                                     FBE_TRACE_MESSAGE_ID_INFO,
                                     "%s: packets 0x%p is corrupted.\n", __FUNCTION__, new_packet_p);
            }
        }

        queue_element_p = next_element_p;

    }

    fbe_spinlock_unlock(&((fbe_base_object_t*)lun_p)->terminator_queue_lock);
    fbe_spinlock_unlock(&lun_p->io_counter_lock);

    // Send the current packet down
    
    status = fbe_ext_pool_lun_send_packet(lun_p, in_packet_p);

    // Process the temp queue by removing the packet and sending it down    
    while(!fbe_queue_is_empty(&dirty_pending_queue)) 
    {
        new_packet_p = fbe_transport_dequeue_packet(&dirty_pending_queue);
        fbe_base_object_trace((fbe_base_object_t*) lun_p,
                             FBE_TRACE_LEVEL_DEBUG_HIGH,
                             FBE_TRACE_MESSAGE_ID_INFO,
                             "LUN restart pkt: %p after set dirty dcnt: %d\n", new_packet_p, lun_p->io_counter);
        status = fbe_ext_pool_lun_send_packet(lun_p, new_packet_p);        

    }


    return FBE_STATUS_MORE_PROCESSING_REQUIRED;

} // fbe_ext_pool_lun_handle_queued_packets_waiting_for_set_dirty_flag

/*!****************************************************************************
 * @fn fbe_ext_pool_lun_read_write_dirty_flag_allocate_subpacket()
 ******************************************************************************
 * @brief
 *
 * @param lun_p             - Pointer to the lun object.
 * @param packet_p          - Pointer the the I/O packet.
 *
 * @return fbe_status_t     - status of the operation.
 *
 * @author
 *   03/19/2012 - Created. MFerson
 *
 ******************************************************************************/
static fbe_status_t
fbe_ext_pool_lun_read_write_dirty_flag_allocate_subpacket(fbe_ext_pool_lun_t * lun_p,
                                             fbe_packet_t * packet_p,
                                             fbe_lun_clean_dirty_context_t * context)
{
    fbe_status_t                     status = FBE_STATUS_OK;
    fbe_memory_request_t *           memory_request_p = NULL;
    fbe_payload_ex_t *               sep_payload_p = NULL;
    fbe_payload_block_operation_t *  block_operation_p = NULL;

    /* Get the payload and prev block operation. */
    sep_payload_p       = fbe_transport_get_payload_ex(packet_p);
    block_operation_p   =  fbe_payload_ex_get_present_block_operation(sep_payload_p);

    /* Allocate the subpacket to handle the write of the dirty flag. */
    memory_request_p = fbe_transport_get_memory_request(packet_p);

    /* make sure our data fits */
    FBE_ASSERT_AT_COMPILE_TIME( sizeof(fbe_packet_t) + ((sizeof(fbe_sg_element_t) * 2) + FBE_4K_BYTES_PER_BLOCK) <= FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO)

    /* build the memory request for allocation. */
    fbe_memory_build_chunk_request(memory_request_p,
                                   FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO,
                                   1,
                                   fbe_transport_get_resource_priority(packet_p),
                                   fbe_transport_get_io_stamp(packet_p),
                                   fbe_ext_pool_lun_read_write_dirty_flag_allocate_completion, /* SAFEBUG - cast to supress warning but must fix because memory completion function shouldn't return status */
                                   context);

	fbe_transport_memory_request_set_io_master(memory_request_p,  packet_p);

    /* Issue the memory request to memory service. */
    status = fbe_memory_request_entry(memory_request_p);
    if((status != FBE_STATUS_OK) && (status != FBE_STATUS_PENDING))
    {
        fbe_base_object_trace((fbe_base_object_t*)lun_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s alloc failed, status:0x%x, line:%d\n", __FUNCTION__, status, __LINE__);

        fbe_payload_block_set_status(block_operation_p,
                                     FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                                     FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNEXPECTED_ERROR);

        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);

        return status;
    }

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_ext_pool_lun_read_write_dirty_flag_allocate_subpacket()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_ext_pool_lun_clean_dirty_subpacket_completion()
 ******************************************************************************
 * @brief

 * @param   packet_p
 * @param   context
 *
 * @return fbe_status_t
 *
 * @author
 *   03/21/2012 - Created. MFerson
 *
 ******************************************************************************/
static fbe_status_t
fbe_ext_pool_lun_clean_dirty_subpacket_completion(fbe_packet_t * packet_p,
                                         fbe_packet_completion_context_t context)
{
    fbe_lun_clean_dirty_context_t *         clean_dirty_context = NULL;
    fbe_ext_pool_lun_t *                             lun_p = NULL;
    fbe_packet_t *                          master_packet_p = NULL;
    fbe_payload_ex_t *                      new_payload_p = NULL;
    fbe_payload_ex_t *                      sep_payload_p = NULL;
    fbe_payload_block_operation_t *         master_block_operation_p = NULL;
    fbe_payload_block_operation_t *         block_operation_p = NULL;
    fbe_status_t                            packet_status;
    fbe_bool_t                              is_empty;
    fbe_lun_dirty_flag_t *                  dirty_flag;
    fbe_queue_head_t                        tmp_queue;
    fbe_payload_block_operation_qualifier_t block_qualifier;
    fbe_payload_block_operation_status_t block_status;

    clean_dirty_context = (fbe_lun_clean_dirty_context_t *)context ;

    lun_p = (fbe_ext_pool_lun_t *)clean_dirty_context->lun_p;

    /* get the master packet. */
    master_packet_p = fbe_transport_get_master_packet(packet_p);

    sep_payload_p = fbe_transport_get_payload_ex(master_packet_p);
    new_payload_p = fbe_transport_get_payload_ex(packet_p);
    
    /* get the subpacket and master packet's block operation. */
    block_operation_p           = fbe_payload_ex_get_block_operation(new_payload_p);

    /* update the master packet status only if it is required. */
    packet_status = fbe_transport_get_status_code(packet_p);
    fbe_transport_set_status(master_packet_p, packet_status, 0);

    fbe_payload_block_get_status(block_operation_p, &block_status);
    fbe_payload_block_get_qualifier(block_operation_p, &block_qualifier);

    if (sep_payload_p->current_operation->payload_opcode == FBE_PAYLOAD_OPCODE_BLOCK_OPERATION)
    {
        master_block_operation_p = fbe_payload_ex_get_block_operation(sep_payload_p);
        fbe_payload_block_set_status(master_block_operation_p, block_status, block_qualifier);
    }
    if ((packet_status != FBE_STATUS_OK) || 
        (block_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS))
    {
        /* If the write fails, I assume that we're going to the FAIL
           state, and the pending I/O will be drained.
         */
        fbe_base_object_trace((fbe_base_object_t*)lun_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "LUN %s %s subpacket failed pkt_st: 0x%x bl_st: 0x%x bl_qual: 0x%x\n", 
                              (clean_dirty_context->is_read) ? "rd" : "wr",
                              ((clean_dirty_context->is_read) 
                               ? ""
                               : ((clean_dirty_context->dirty) ? "dirty" : "clean")), 
                              packet_status, block_status, block_qualifier);
        if ((block_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED) &&
            ((block_qualifier == FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE) ||
             (block_qualifier == FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_POSSIBLE)))
        {
            /* For block operation status values that we want to retry, return 
             * a retryable status. 
             */
            packet_status = FBE_STATUS_IO_FAILED_RETRYABLE;
        }
        else if ((packet_status == FBE_STATUS_OK) &&
                 (block_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS))
        {
            /* Only a block status failure which we do not want to retry, or which we do not expect.
             * Mark it bad so we will not retry it.
             */
            packet_status = FBE_STATUS_IO_FAILED_NOT_RETRYABLE;
        }
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t*)lun_p, 
                              FBE_TRACE_LEVEL_DEBUG_HIGH, 
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "LUN %s Subpacket success 0x%p\n",
                              __FUNCTION__, packet_p);
    }

    clean_dirty_context->status = packet_status;

    if(clean_dirty_context->is_read) {
        dirty_flag = (fbe_lun_dirty_flag_t *)clean_dirty_context->buff_ptr;
        clean_dirty_context->dirty = dirty_flag->dirty;
    }


    /* Handling subpacket queue and the master packet needs to be interlocked */
    fbe_spinlock_lock(&((fbe_base_object_t*)lun_p)->terminator_queue_lock);

    /* remove the subpacket from master packet. */
    //fbe_transport_remove_subpacket(packet_p);
    fbe_transport_remove_subpacket_is_queue_empty(packet_p, &is_empty);

    /* release the block operation. */
    fbe_payload_ex_release_block_operation(new_payload_p, block_operation_p);

    /* when the queue is empty, all subpackets have completed. */
    if(is_empty)
    {
        fbe_transport_destroy_subpackets(master_packet_p);

        /* remove the master packet from terminator queue and complete it. */
        /* We've already have the lock */
        fbe_transport_remove_packet_from_queue(master_packet_p);

        /* We are safe now */
        fbe_spinlock_unlock(&((fbe_base_object_t*)lun_p)->terminator_queue_lock);

        /* release the preallocated memory for the zero on demand request. */
        //fbe_memory_request_get_entry_mark_free(&master_packet_p->memory_request, &memory_ptr);
        //fbe_memory_free_entry(memory_ptr);
		fbe_memory_free_request_entry(&master_packet_p->memory_request);

        /* initialize the queue. */
        fbe_queue_init(&tmp_queue);

        /* enqueue this packet temporary queue before we enqueue it to run queue. */
        fbe_queue_push(&tmp_queue, &master_packet_p->queue_element);

        /*!@note Queue this request to run queue to break the i/o context to avoid the stack
        * overflow, later this needs to be queued with the thread running in same core.
        */
        fbe_transport_run_queue_push(&tmp_queue,  NULL,  NULL);
    }
    else
    {
        fbe_spinlock_unlock(&((fbe_base_object_t*)lun_p)->terminator_queue_lock);
    }

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * fbe_ext_pool_lun_clean_dirty_subpacket_completion()
 ******************************************************************************/
/*!**************************************************************
 * fbe_ext_pool_lun_send_func_packet()
 ****************************************************************
 * @brief
 *  This function sends a packet and if it encounters a bad edge
 *  state we queue the object.  This is necessary to ensure that
 *  the object bubbles up an edge state change before
 *  bubbling up I/O failures.
 *
 * @param  lun_p - object
 * @param  packet_p - Current I/O.
 *
 * @return fbe_status_t  
 *
 * @author
 *  10/10/2012 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_ext_pool_lun_send_func_packet(fbe_ext_pool_lun_t *lun_p, fbe_packet_t *packet_p)
{
    fbe_status_t status;
	fbe_block_edge_t				* edge_p = NULL;
    fbe_payload_ex_t				* payload_p = NULL;
    fbe_payload_block_operation_t	* bo = NULL;
	fbe_lba_t first_stripe;
	fbe_lba_t last_stripe;


    payload_p = fbe_transport_get_payload_ex(packet_p);
    bo = fbe_payload_ex_get_present_block_operation(payload_p);

	fbe_lun_get_block_edge(lun_p, &edge_p);

	if(bo != NULL){
		/* Check if I/O is crossing Morley parity stripe boundaries */
		if(bo->block_opcode	== FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ ||
			bo->block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE ||
			bo->block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_WRITE ||
			bo->block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_VERIFY ||
			bo->block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_NONCACHED ||
			bo->block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_CORRUPT_DATA || 
			/* We should not split LUN zero request */
			((bo->block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ZERO)  && !(packet_p->packet_attr & FBE_PACKET_FLAG_MONITOR_OP)) || 
			bo->block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_SAME){


			first_stripe = (edge_p->offset + bo->lba) / lun_p->alignment_boundary;
			last_stripe = (edge_p->offset + bo->lba + bo->block_count - 1) / lun_p->alignment_boundary;

			/* Zero request does not requiere SGL */
			if((bo->block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ZERO) && 
				(first_stripe != last_stripe) && ((last_stripe - first_stripe) < 16)){ /* We just do not want to have too many I/O's */
				/* We have to split this I/O */
				return fbe_ext_pool_lun_split_and_send_func_packet(lun_p, packet_p);
			}


			if((first_stripe != last_stripe) && (last_stripe - first_stripe) < 8){ /* 1 64 block request can hold up to 8 packets */
				/* We have to split this I/O */
				return fbe_ext_pool_lun_split_and_send_func_packet(lun_p, packet_p);
			}
		}
	}
    /* We check the capacity above.  Since there are cases where the request goes beyond the exported capacity, 
     * it is necessary for us to avoid this check here. 
     */
    status = fbe_block_transport_send_no_autocompletion(edge_p, packet_p, FBE_FALSE /* no capacity check */);

//    fbe_base_object_trace((fbe_base_object_t*)lun_p, FBE_TRACE_LEVEL_, FBE_TRACE_MESSAGE_ID_INFO,
  //                        "LUN send packet lstate: %d pstate: %d packet %p\n", 
    //                      lifecycle_state, edge_p->base_edge.path_state, packet_p);
    if (status == FBE_STATUS_EDGE_NOT_ENABLED)
    {
        fbe_object_id_t object_id;
	    fbe_lifecycle_state_t lifecycle_state;    

        fbe_base_object_get_object_id((fbe_base_object_t*)lun_p, &object_id);
		fbe_base_object_get_lifecycle_state((fbe_base_object_t*)lun_p, &lifecycle_state);

        if (fbe_queue_is_element_on_queue(&packet_p->queue_element))
        {
            fbe_base_object_trace((fbe_base_object_t*)lun_p, FBE_TRACE_LEVEL_CRITICAL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                  "packet %p is on queue !!\n", packet_p);
        }
        if (fbe_transport_is_monitor_packet(packet_p, object_id))
        {
            fbe_base_object_trace((fbe_base_object_t*)lun_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                  "LUN send packet fail mpacket edge lstate: %d pstate: %d packet %p\n", 
                                  lifecycle_state, edge_p->base_edge.path_state, packet_p);
            fbe_transport_complete_packet(packet_p);
        }
        else
        {        
            fbe_base_object_trace((fbe_base_object_t*)lun_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                  "LUN send packet fail send edge lstate: %d pstate: %d Queuing packet %p\n", 
                                  lifecycle_state, edge_p->base_edge.path_state, packet_p);
            fbe_ext_pool_lun_add_to_terminator_queue(lun_p, packet_p);
        }
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }
    return status;
}
/******************************************
 * end fbe_ext_pool_lun_send_func_packet()
 ******************************************/
/*!****************************************************************************
 * @fn fbe_ext_pool_lun_read_write_dirty_flag_allocate_completion()
 ******************************************************************************
 * @brief 
 *
 * @param lun_p             - Pointer to the lun object.
 * @param packet_p          - Pointer the the I/O packet.
 *
 * @return fbe_status_t     - status of the operation.
 *
 * @author
 *   03/19/2012 - Created. MFerson
 *
 ******************************************************************************/
static void
fbe_ext_pool_lun_read_write_dirty_flag_allocate_completion(fbe_memory_request_t * memory_request_p, 
                                              fbe_memory_completion_context_t context)
{
    fbe_lun_clean_dirty_context_t *     clean_dirty_context;
    fbe_ext_pool_lun_t *                         lun_p = NULL;
    fbe_packet_t *                      new_packet_p;
    fbe_packet_t *                      packet_p = NULL;
    fbe_payload_ex_t *                  payload_p = NULL;
    fbe_payload_ex_t *                  sep_payload_p = NULL;
    fbe_payload_block_operation_t *     block_operation_p = NULL;
    fbe_payload_block_operation_t *     new_block_operation_p = NULL;
    fbe_sg_element_t *                  sg_list = NULL;
    fbe_status_t                        status;
    fbe_optimum_block_size_t            optimum_block_size;
    fbe_lba_t                           lba = FBE_LBA_INVALID;
    fbe_u8_t *                          mem_ptr;
    fbe_u32_t                           sg_list_size;
    fbe_u8_t *                          buff_ptr;
    fbe_lun_dirty_flag_t *              flag_ptr;
    fbe_memory_header_t *               current_memory_header = NULL;
    fbe_payload_block_operation_opcode_t block_opcode;
    fbe_payload_sg_descriptor_t         payload_sg_descriptor;
    fbe_object_id_t                     object_id;
    fbe_u32_t i;

    clean_dirty_context = (fbe_lun_clean_dirty_context_t *)context;

    if(clean_dirty_context->is_read)
    {
        block_opcode = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ;
    }
    else
    {
        block_opcode = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE;
    }

    lun_p       = (fbe_ext_pool_lun_t *)clean_dirty_context->lun_p;
    packet_p    = fbe_transport_memory_request_to_packet(memory_request_p);
    
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_get_present_block_operation(sep_payload_p);

    /* Check allocation status */
    if (fbe_memory_request_is_allocation_complete(memory_request_p) == FBE_FALSE)
    {
        /* Currently this callback doesn't expect any aborted requests */
        fbe_base_object_trace((fbe_base_object_t*)lun_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s memory request: 0x%p state: %d allocation failed\n", 
                              __FUNCTION__, memory_request_p, memory_request_p->request_state);

        if(block_operation_p != NULL)
        {
            fbe_payload_block_set_status(block_operation_p,
                                         FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED,
                                         FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNEXPECTED_ERROR);
        }
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return;
    }

    sg_list_size = (sizeof(fbe_sg_element_t) * 2);
    sg_list_size += (sg_list_size % sizeof(fbe_u64_t));
    
    current_memory_header = fbe_memory_get_first_memory_header(memory_request_p);

    if((fbe_u32_t)(sg_list_size + FBE_4K_BYTES_PER_BLOCK)
       > (fbe_u32_t)(current_memory_header->memory_chunk_size))
    {
        fbe_base_object_trace((fbe_base_object_t*)lun_p, 
                              FBE_TRACE_LEVEL_CRITICAL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s Memory chunk size is too small for necessary structures!\n", __FUNCTION__);

        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return;
    }

    /* we allocated 1 chunk for packet, sg_list, and data */
    mem_ptr       = current_memory_header->data;
    new_packet_p  = (fbe_packet_t *)mem_ptr;
    mem_ptr      += sizeof(fbe_packet_t);

    fbe_base_object_trace((fbe_base_object_t*)lun_p, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH, 
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s Master 0x%p Subpacket 0x%p\n",
                              __FUNCTION__, packet_p, new_packet_p);

    /* data first, default data alignment helps xor lib */
    buff_ptr = mem_ptr;
    flag_ptr = (fbe_lun_dirty_flag_t *)mem_ptr;
    mem_ptr += FBE_4K_BYTES_PER_BLOCK;

    clean_dirty_context->buff_ptr = buff_ptr;

    sg_list = (fbe_sg_element_t *)mem_ptr;
    mem_ptr += sg_list_size;
    
    sg_list->address    = buff_ptr;
    sg_list->count      = FBE_4K_BYTES_PER_BLOCK;
    sg_list++;
    sg_list->address    = 0;
    sg_list->count      = 0;
    sg_list--;

    fbe_zero_memory(buff_ptr, FBE_4K_BYTES_PER_BLOCK);

    if(! clean_dirty_context->is_read) 
    {
        fbe_sector_t *sector_p = (fbe_sector_t*)buff_ptr;
        flag_ptr->dirty = clean_dirty_context->dirty;

        for(i = 0; i < FBE_520_BLOCKS_PER_4K; i++)
        {
            if (fbe_lun_is_raw(lun_p))
            {
                /* Raw mirrors need to set magic and seq #.
                 */
                fbe_xor_lib_raw_mirror_sector_set_seq_num(sector_p, 0);
            }
            fbe_xor_lib_calculate_and_set_checksum((fbe_u32_t*)sector_p);
            sector_p++;
        }
    }

    status = fbe_ext_pool_lun_get_dirty_flag_offset(lun_p, clean_dirty_context->sp_id, &lba);

    /* Initialize sub packet. */
    fbe_transport_initialize_packet(new_packet_p);

    /* Set block operation same as master packet's prev block operation. */
    payload_p = fbe_transport_get_payload_ex(new_packet_p);
    new_block_operation_p = fbe_payload_ex_allocate_block_operation(payload_p);

    /* set sg list with new packet. */
    fbe_payload_ex_set_sg_list(payload_p, sg_list, 1);

    fbe_block_transport_edge_get_optimum_block_size(&lun_p->block_edge, &optimum_block_size);

    fbe_payload_block_build_operation(new_block_operation_p,
                                      block_opcode,
                                      lba,
                                      FBE_520_BLOCKS_PER_4K,
                                      FBE_4K_BYTES_PER_BLOCK,
                                      optimum_block_size,
                                      NULL);

    new_block_operation_p->block_edge_p = &lun_p->block_edge;
    payload_sg_descriptor.repeat_count = 1;
    new_block_operation_p->payload_sg_descriptor = payload_sg_descriptor;

    if(block_operation_p != NULL)
    {
        new_block_operation_p->block_flags = block_operation_p->block_flags;

        /* Clear out allow congestion since we do not want to get back a congested status on the dirty write. 
         */
        new_block_operation_p->block_flags &= ~(FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_ALLOW_FAIL_CONGESTION |
                                                FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_ALLOW_FAIL_NOT_PREFERRED);
        new_block_operation_p->payload_sg_descriptor = block_operation_p->payload_sg_descriptor;
        

        /* initialize the block operation with default status. */
        fbe_payload_block_set_status(block_operation_p,
                                     FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID,
                                     FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID);
    }
    else
    {
        new_block_operation_p->block_flags = 0;
    }

    fbe_base_object_get_object_id((fbe_base_object_t*)lun_p, &object_id);
    if(fbe_transport_is_monitor_packet(packet_p, object_id))
    {
        fbe_transport_set_packet_attr(new_packet_p, FBE_PACKET_FLAG_DO_NOT_HOLD);
    }

    /* Set completion function */
    fbe_transport_set_completion_function(new_packet_p,
                                          fbe_ext_pool_lun_clean_dirty_subpacket_completion,
                                          clean_dirty_context);

    /* We need to associate the new packet with the original one */
    status = fbe_transport_add_subpacket(packet_p, new_packet_p);

    if (status != FBE_STATUS_OK) 
    { 
        fbe_base_object_trace((fbe_base_object_t*) lun_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                             "LUN rw dirty flag fail add sub pkt q status 0x%x pkt: %p spkt: %p\n", status, packet_p, new_packet_p);
        fbe_transport_destroy_packet(new_packet_p);
        fbe_memory_free_request_entry(&packet_p->memory_request);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return; 
    }

    /* Put the original packet on the terminator queue while we wait for the subpacket to complete. */
    fbe_transport_set_cancel_function(packet_p, fbe_base_object_packet_cancel_function, (fbe_base_object_t *)lun_p);
    status = fbe_base_object_add_to_terminator_queue((fbe_base_object_t *)lun_p, packet_p);

    if (status != FBE_STATUS_OK) 
    { 
        fbe_base_object_trace((fbe_base_object_t*) lun_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                             "LUN rw dirty flag fail add terminator q status 0x%x pkt: %p\n", 
                              status, packet_p);
        fbe_transport_remove_subpacket(new_packet_p);
        fbe_transport_destroy_packet(new_packet_p);
        fbe_memory_free_request_entry(&packet_p->memory_request);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return; 
    }

    /* Send packet to the block edge */
    fbe_ext_pool_lun_send_func_packet(lun_p, new_packet_p);

    return;
}
/******************************************************************************
 * end fbe_ext_pool_lun_read_write_dirty_flag_allocate_completion()
 ******************************************************************************/

static fbe_status_t fbe_ext_pool_lun_get_dirty_flag_offset(fbe_ext_pool_lun_t*      in_lun_p,
                                           fbe_cmi_sp_id_t in_sp_id,
                                           fbe_lba_t *     out_lba)
{
    fbe_status_t                        status;
    fbe_cmi_sp_id_t                     cmi_sp_id = FBE_CMI_SP_ID_INVALID;
    fbe_lba_t                           offset;

    offset = in_lun_p->dirty_flags_start_lba;

    /* Calculate the offset based on SP ID.
     */
    if(in_sp_id == FBE_CMI_SP_ID_INVALID) {
        status = fbe_cmi_get_sp_id(&cmi_sp_id);

        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t*) in_lun_p,
                             FBE_TRACE_LEVEL_ERROR,
                             FBE_TRACE_MESSAGE_ID_INFO,
                             "%s: failed to get SP ID\n", __FUNCTION__);
            return status;
        }
    }
    else
    {
        cmi_sp_id = in_sp_id;
    }

    if(cmi_sp_id == FBE_CMI_SP_ID_A)
    {
        offset = in_lun_p->dirty_flags_start_lba + FBE_LUN_DIRTY_FLAG_SP_A_BLOCK_OFFSET;
    }
    else if(cmi_sp_id == FBE_CMI_SP_ID_B)
    {
        offset = in_lun_p->dirty_flags_start_lba + FBE_LUN_DIRTY_FLAG_SP_B_BLOCK_OFFSET;
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t*) in_lun_p,
                         FBE_TRACE_LEVEL_ERROR,
                         FBE_TRACE_MESSAGE_ID_INFO,
                         "%s: Invalid SP ID 0x%X\n", __FUNCTION__, cmi_sp_id);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_base_object_trace((fbe_base_object_t*) in_lun_p,
         FBE_TRACE_LEVEL_DEBUG_HIGH,
         FBE_TRACE_MESSAGE_ID_INFO,
         "%s: Offset 0x%X\n", __FUNCTION__, (unsigned int)(offset - in_lun_p->dirty_flags_start_lba));

    *out_lba = offset;
    return FBE_STATUS_OK;
}

/*!****************************************************************************
 * fbe_ext_pool_lun_mark_local_clean()
 ******************************************************************************
 * @brief
 *  This function marks the local dirty flag as clean
 *
 * @param in_object_p               - pointer to a base object.
 * @param in_packet_p               - pointer to a control packet.
 * @param completion_function       - Completion function to be called 
 * 
 * @return fbe_status_t             - fbe status
 *
 * @author
 *  09/30/2012 - Created from fbe_ext_pool_lun_mark_local_clean_cond . Ashok Tamilarasan
 *
 ******************************************************************************/
fbe_status_t fbe_ext_pool_lun_mark_local_clean(fbe_ext_pool_lun_t * lun_p, 
                                      fbe_packet_t *packet_p, 
                                      fbe_packet_completion_function_t completion_function)
{
    fbe_lun_clean_dirty_context_t * context = NULL;
    fbe_status_t status;

    context = &lun_p->clean_dirty_monitor_context;
    context->dirty      = FBE_FALSE;
    context->is_read    = FBE_FALSE;
    context->lun_p      = lun_p;
    context->sp_id      = FBE_CMI_SP_ID_INVALID;

    status = fbe_ext_pool_lun_update_dirty_flag(context, packet_p,
                                       completion_function);   

    return status;
}

/*!****************************************************************************
 * fbe_ext_pool_lun_mark_peer_clean()
 ******************************************************************************
 * @brief
 *  This function marks the peer dirty flag as clean
 *
 * @param in_object_p               - pointer to a base object.
 * @param in_packet_p               - pointer to a control packet.
 * @param completion_function       - Completion function to be called 
 * 
 * @return fbe_status_t             - fbe status
 *
 * @author
 *  09/30/2012 - Created from fbe_ext_pool_lun_mark_peer_clean_cond . Ashok Tamilarasan
 *
 ******************************************************************************/
fbe_status_t fbe_ext_pool_lun_mark_peer_clean(fbe_ext_pool_lun_t * lun_p, 
                                      fbe_packet_t *packet_p, 
                                      fbe_packet_completion_function_t completion_function)
{
    fbe_cmi_sp_id_t     cmi_sp_id;
    fbe_cmi_sp_id_t     sp_id;
    fbe_lun_clean_dirty_context_t * context;
    fbe_status_t status;

    status = fbe_cmi_get_sp_id(&cmi_sp_id);

    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*) lun_p,
                             FBE_TRACE_LEVEL_ERROR,
                             FBE_TRACE_MESSAGE_ID_INFO,
                             "%s: failed to get SP ID\n", __FUNCTION__);
        return status;
    }

    if(cmi_sp_id == FBE_CMI_SP_ID_A)
    {
        sp_id = FBE_CMI_SP_ID_B;
    }
    else
    {
        sp_id = FBE_CMI_SP_ID_A;
    }

    context = &lun_p->clean_dirty_monitor_context;
    context->dirty      = FBE_FALSE;
    context->is_read    = FBE_FALSE;
    context->lun_p      = lun_p;
    context->sp_id      = sp_id;

    status = fbe_ext_pool_lun_update_dirty_flag(context, packet_p, completion_function );   

    return status;
}

/*!****************************************************************************
 * fbe_ext_pool_lun_update_dirty_flag()
 ******************************************************************************
 * @brief
 *  Update the dirty flag in the LUN object's metadata
 * 
 *
 * @param in_lun_p - The lun object
 * @param in_packet_p
 * @param in_sp_id
 * @param dirty - set or clear the dirty flag
 * @param completion_function - completion function
 *
 * @return fbe_status_t
 *
 * @author
 *  2/21/2012 - Created. MFerson
 * 
 *
 *****************************************************************************/
fbe_status_t fbe_ext_pool_lun_update_dirty_flag(fbe_lun_clean_dirty_context_t * context,
                                       fbe_packet_t*   in_packet_p,
                                       fbe_packet_completion_function_t completion_function)                                   
{
    fbe_status_t status;
    fbe_payload_ex_t * sep_payload_p = NULL;
    fbe_ext_pool_lun_t * in_lun_p = NULL;

    in_lun_p = (fbe_ext_pool_lun_t *)context->lun_p;

    context->status     = FBE_STATUS_INVALID;
    context->is_read    = FBE_FALSE;
    
    sep_payload_p = fbe_transport_get_payload_ex(in_packet_p);

    status = fbe_transport_set_completion_function(in_packet_p, 
                                                   completion_function, 
                                                   in_lun_p);

    //fbe_payload_ex_increment_block_operation_level(sep_payload_p);

    status = fbe_ext_pool_lun_read_write_dirty_flag_allocate_subpacket(in_lun_p, in_packet_p, context);

    return status;
} /* end fbe_ext_pool_lun_update_dirty_flag */

/*!****************************************************************************
 * fbe_ext_pool_lun_get_dirty_flag()
 ******************************************************************************
 * @brief
 * 
 *
 * @param in_lun_p - The lun object
 * @param in_packet_p
 * @param in_sp_id
 * @param completion_function
 *
 * @return fbe_status_t
 *
 * @author
 *  03/12/2012 - Created. MFerson
 * 
 *
 *****************************************************************************/

fbe_status_t fbe_ext_pool_lun_get_dirty_flag(fbe_lun_clean_dirty_context_t * context,
                                    fbe_packet_t*   in_packet_p,
                                    fbe_packet_completion_function_t completion_function)
{
    fbe_status_t status;
    fbe_payload_ex_t * sep_payload_p = NULL;
    fbe_ext_pool_lun_t * in_lun_p = NULL;

    in_lun_p = (fbe_ext_pool_lun_t *)context->lun_p;

    context->status     = FBE_STATUS_INVALID;
    context->is_read    = FBE_TRUE;
    
    sep_payload_p = fbe_transport_get_payload_ex(in_packet_p);

    status = fbe_transport_set_completion_function(in_packet_p, 
                                                   completion_function, 
                                                   in_lun_p);

    //fbe_payload_ex_increment_block_operation_level(sep_payload_p);

    status = fbe_ext_pool_lun_read_write_dirty_flag_allocate_subpacket(in_lun_p, in_packet_p, context);

    return status;

}/* end fbe_ext_pool_lun_get_dirty_flag */

static void fbe_ext_pool_lun_notify_of_first_write(fbe_ext_pool_lun_t*  in_lun_p, fbe_payload_block_operation_t *block_operation)
{
    fbe_status_t        				status;
	fbe_payload_block_operation_opcode_t opcode = block_operation->block_opcode;
	fbe_lba_t 							size_of_cache_bitmap;

    /*we don't want to lock for any IO so we check w/o lock first*/
    if ((!fbe_lun_is_flag_set(in_lun_p, FBE_LUN_FLAGS_HAS_BEEN_WRITTEN)) && 
        ((opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE) || 
         (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_NONCACHED) ||
         (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_WRITE))) {

		fbe_lba_t	capacity;

		if (!fbe_private_space_layout_object_id_is_system_lun(in_lun_p->base_config.base_object.object_id)) {
			capacity = 0;
		}

		/*is it in the LBA range the user is using (we drop the capacity to compencate for the ones used by cache bitmap)*/
		fbe_block_transport_server_get_capacity(&in_lun_p->base_config.block_transport_server, &capacity);
		fbe_ext_pool_lun_calculate_cache_zero_bit_map_size_to_remove(capacity, &size_of_cache_bitmap);
		if ((capacity - size_of_cache_bitmap) > block_operation->lba) {
	
			 fbe_spinlock_lock(&in_lun_p->io_counter_lock);
	
			 /*are we still not written?*/
			 if (!fbe_lun_is_flag_set(in_lun_p, FBE_LUN_FLAGS_HAS_BEEN_WRITTEN)) {
	
				 fbe_lun_set_flag(in_lun_p, FBE_LUN_FLAGS_HAS_BEEN_WRITTEN);/*mark it*/
				 fbe_spinlock_unlock(&in_lun_p->io_counter_lock);
				 
				 /*let's set a condition and update the metadata so upper layeres will be notify of it*/
				 status = fbe_lifecycle_set_cond(&fbe_ext_pool_lun_lifecycle_const,
												 (fbe_base_object_t*)in_lun_p,
												 FBE_LUN_LIFECYCLE_COND_LUN_SIGNAL_FIRST_WRITE);
	
	
				 if (status != FBE_STATUS_OK) {
					fbe_base_object_trace((fbe_base_object_t*)in_lun_p, 
									  FBE_TRACE_LEVEL_ERROR, 
									  FBE_TRACE_MESSAGE_ID_INFO,
									  "%s, failed to set condition\n", __FUNCTION__);
				 }
				  
			 }else{
				 fbe_spinlock_unlock(&in_lun_p->io_counter_lock);
			 }
		}
    }
}


/*!**************************************************************
 * fbe_ext_pool_lun_spit_and_send_func_packet()
 ****************************************************************
 * @brief
 *  This function splits original I/O to gurantee that 
 * alignment boundaries not crossed and sends the packets to raid.
 *
 * @param  lun_p - object
 * @param  packet_p - Current I/O.
 *
 * @return fbe_status_t  
 *
 * @author
 *  02/22/2013 - Created. Peter Puhov
 *
 ****************************************************************/

static fbe_status_t 
fbe_ext_pool_lun_split_and_send_func_packet(fbe_ext_pool_lun_t *lun_p, fbe_packet_t *packet_p)
{
    fbe_status_t status;
	fbe_block_edge_t				* edge_p = NULL;
    fbe_payload_ex_t				* payload_p = NULL;
    fbe_payload_block_operation_t	* bo = NULL;
    fbe_memory_request_t *           memory_request_p = NULL;
	fbe_lba_t first_stripe;
	fbe_lba_t last_stripe;
	fbe_u32_t sg_count = 0;
	fbe_sg_element_t * sg_list = NULL;

    payload_p = fbe_transport_get_payload_ex(packet_p);
    bo = fbe_payload_ex_get_present_block_operation(payload_p);

    fbe_lun_get_block_edge(lun_p, &edge_p);

	first_stripe = (edge_p->offset + bo->lba) / lun_p->alignment_boundary;
	last_stripe = (edge_p->offset + bo->lba + bo->block_count - 1) / lun_p->alignment_boundary;

    /* Allocate the subpacket's and SGL's */
    memory_request_p = fbe_transport_get_memory_request(packet_p);

	fbe_payload_ex_get_sg_list(payload_p, &sg_list, NULL);

	/* Count sg entries in original requets */
	sg_count =  fbe_sg_element_count_entries(sg_list);

	if((bo->block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ZERO)){
		/* build the memory request for allocation. */
		fbe_memory_build_chunk_request_sync(memory_request_p,
									   FBE_MEMORY_CHUNK_SIZE_FOR_PACKET,
									   (fbe_memory_number_of_objects_t)((last_stripe - first_stripe + 1)), /* 1 for each packet */
									   fbe_transport_get_resource_priority(packet_p),
									   fbe_transport_get_io_stamp(packet_p),
									   fbe_ext_pool_lun_split_and_send_func_packet_allocate_completion,
									   (fbe_memory_completion_context_t)lun_p);


	}else if((sg_count + 1) * sizeof(fbe_sg_element_t) < FBE_MEMORY_CHUNK_SIZE_FOR_PACKET){
		/* build the memory request for allocation. */
		fbe_memory_build_chunk_request_sync(memory_request_p,
									   FBE_MEMORY_CHUNK_SIZE_FOR_PACKET,
									   (fbe_memory_number_of_objects_t)((last_stripe - first_stripe + 1) * 2), /* 1 for SGL and 1 for packet */
									   fbe_transport_get_resource_priority(packet_p),
									   fbe_transport_get_io_stamp(packet_p),
									   fbe_ext_pool_lun_split_and_send_func_packet_allocate_completion,
									   (fbe_memory_completion_context_t)lun_p);
	} else {
		/* build the memory request for allocation. */
		fbe_memory_build_chunk_request_sync(memory_request_p,
									   FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO, /* 1 block - 8 packets */
									   (fbe_memory_number_of_objects_t)((last_stripe - first_stripe + 1) + 1), /* 1 for all packets and 1 for each SGL */
									   fbe_transport_get_resource_priority(packet_p),
									   fbe_transport_get_io_stamp(packet_p),
									   fbe_ext_pool_lun_split_and_send_func_packet_allocate_completion,
									   (fbe_memory_completion_context_t)lun_p);
	}
	fbe_transport_memory_request_set_io_master(memory_request_p,  packet_p);

    /* Issue the memory request to memory service. */
    status = fbe_memory_request_entry(memory_request_p);
	if (status == FBE_STATUS_OK) { 
        /* We do have memory right away */
		fbe_ext_pool_lun_split_and_send_func_packet_allocate_completion(memory_request_p, (fbe_memory_completion_context_t)lun_p);
		return FBE_STATUS_OK;
	}

    return FBE_STATUS_OK;
}



static void
fbe_ext_pool_lun_split_and_send_func_packet_allocate_completion(fbe_memory_request_t * memory_request_p, fbe_memory_completion_context_t context)
{
    fbe_ext_pool_lun_t *                         lun_p = NULL;
    fbe_packet_t *                      new_packet_p = NULL;
    fbe_packet_t *                      packet_p = NULL;
    fbe_payload_ex_t *                  payload_p = NULL;
    fbe_payload_ex_t *                  sep_payload_p = NULL;
	fbe_block_edge_t					* edge_p = NULL;
    fbe_payload_block_operation_t *     bo = NULL;
    fbe_payload_block_operation_t *     new_block_operation_p = NULL;
    fbe_sg_element_t *                  new_sg_list = NULL;
    fbe_sg_element_t *                  sg_list = NULL;

    fbe_status_t                        status;
    fbe_optimum_block_size_t            optimum_block_size;
    fbe_lba_t                           i;
    //fbe_u8_t *                          mem_ptr;
    fbe_memory_header_t *               current_memory_header = NULL;
    fbe_memory_header_t *               next_memory_header = NULL;
	fbe_lba_t lba;
	fbe_block_count_t block_count;
	fbe_sg_element_with_offset_t sgd;
    fbe_u16_t           sgs_used_16 = 0;
	fbe_queue_head_t    tmp_queue;
	fbe_queue_element_t  *  queue_element = NULL;
	fbe_lba_t first_stripe;
	fbe_lba_t last_stripe;
	fbe_u32_t sg_list_count;
	fbe_payload_memory_operation_t * saved_payload_memory_operation;

	fbe_queue_init(&tmp_queue);

    lun_p       = (fbe_ext_pool_lun_t *)context;
    packet_p    = fbe_transport_memory_request_to_packet(memory_request_p);
    
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    bo = fbe_payload_ex_get_present_block_operation(sep_payload_p);
	fbe_payload_ex_get_sg_list(sep_payload_p, &sg_list, &sg_list_count);

	fbe_sg_element_with_offset_init(&sgd, 0 /* no offset */, sg_list, NULL /* do not specify sg function*/ );


    /* initialize the block operation with default status. */
    fbe_payload_block_set_status(bo,
                                 FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID,
                                 FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID);


    current_memory_header = fbe_memory_get_first_memory_header(memory_request_p);

    fbe_lun_get_block_edge(lun_p, &edge_p);

	first_stripe = (edge_p->offset + bo->lba) / lun_p->alignment_boundary;
	last_stripe = (edge_p->offset + bo->lba + bo->block_count -1 ) / lun_p->alignment_boundary;

	lba = bo->lba;

	if(memory_request_p->chunk_size == FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO){
		new_packet_p = (fbe_packet_t *)fbe_memory_header_to_data(current_memory_header);;

		/* next memory_chunk */
        fbe_memory_get_next_memory_header(current_memory_header, &next_memory_header);
        current_memory_header = next_memory_header;
	}

	/* Loop over packets */
	for(i = first_stripe; i <= last_stripe; i++){
        void* eboard_p = NULL;

		if(memory_request_p->chunk_size == FBE_MEMORY_CHUNK_SIZE_FOR_PACKET){
			new_packet_p = (fbe_packet_t *)fbe_memory_header_to_data(current_memory_header);

			/* next memory_chunk */
			fbe_memory_get_next_memory_header(current_memory_header, &next_memory_header);
			current_memory_header = next_memory_header;
		}		

		if(!(bo->block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ZERO)){
			new_sg_list = (fbe_sg_element_t *)fbe_memory_header_to_data(current_memory_header);

			/* prepare chunk for the next iteration */
			fbe_memory_get_next_memory_header(current_memory_header, &next_memory_header);
			current_memory_header = next_memory_header;
		}

		/*sanity check if someone messed up things before*/
		if (new_packet_p == NULL) {
			fbe_base_object_trace((fbe_base_object_t*)lun_p, 
								  FBE_TRACE_LEVEL_CRITICAL_ERROR, 
								  FBE_TRACE_MESSAGE_ID_INFO,
								  "%s, Someone forgot to init new_packet_p...\n", __FUNCTION__);

			return;/*we never return from here but Coverity will think we continue to use the NULL pointer so we have to confuse it*/

		}

		/* Initialize sub packet. */
		fbe_transport_initialize_packet(new_packet_p);

		/* Set block operation same as master packet's prev block operation. */
		payload_p = fbe_transport_get_payload_ex(new_packet_p);

		/* Each I/O will be sent down with it's own memory master */
		fbe_payload_ex_allocate_memory_operation(payload_p);

		new_block_operation_p = fbe_payload_ex_allocate_block_operation(payload_p);

        /* The new packet should use the verify report of the master.
         */
        fbe_payload_ex_get_verify_error_count_ptr(sep_payload_p, &eboard_p);
        fbe_payload_ex_set_verify_error_count_ptr(payload_p, eboard_p);

		/* set sg list with new packet. */
		//fbe_payload_ex_set_sg_list(payload_p, sg_list, 1);

		fbe_block_transport_edge_get_optimum_block_size(edge_p, &optimum_block_size);

		block_count = ((i + 1) * lun_p->alignment_boundary) - lba - edge_p->offset;
		if((lba + block_count) > (bo->lba + bo->block_count)){ /* Last stripe */
			block_count = bo->lba + bo->block_count - lba;
		}

		fbe_payload_block_build_operation(new_block_operation_p,
										  bo->block_opcode,
										  lba,
										  block_count,
										  FBE_BE_BYTES_PER_BLOCK,
										  optimum_block_size,
										  NULL);

		if((bo->block_opcode != FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_SAME)  &&
			(bo->block_opcode != FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ZERO )){
			/* Form SGL for that packet */
			status = fbe_raid_sg_clip_sg_list(&sgd, new_sg_list,
											  (fbe_u32_t)(block_count * FBE_BE_BYTES_PER_BLOCK),
											  &sgs_used_16);
			if (status != FBE_STATUS_OK) {
				fbe_base_object_trace((fbe_base_object_t*)lun_p, 
									  FBE_TRACE_LEVEL_WARNING, 
									  FBE_TRACE_MESSAGE_ID_INFO,
									  "%s, fbe_raid_sg_clip_sg_list ERROR\n", __FUNCTION__);

				fbe_transport_destroy_packet(new_packet_p);
				fbe_transport_destroy_subpackets(packet_p);
				fbe_memory_free_request_entry(&packet_p->memory_request);
				fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
				fbe_payload_block_set_status(bo,
											 FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
											 FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNEXPECTED_ERROR);
				fbe_transport_complete_packet(packet_p);
				return;
			}

			fbe_payload_ex_set_sg_list (payload_p, new_sg_list, sgs_used_16);

			if(sgs_used_16 * sizeof(fbe_sg_element_t) > memory_request_p->chunk_size){
				fbe_base_object_trace((fbe_base_object_t*)lun_p, 
									  FBE_TRACE_LEVEL_CRITICAL_ERROR, 
									  FBE_TRACE_MESSAGE_ID_INFO,
									  "%s, SGL is too BIG\n", __FUNCTION__);
			}
		} else { /* It is FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_SAME */
			/* We will put original sg_list to all subpackets */
			fbe_payload_ex_set_sg_list (payload_p, sg_list, sg_list_count);
		}

		lba += block_count;
		new_block_operation_p->block_edge_p = &lun_p->block_edge;
		new_block_operation_p->block_flags = bo->block_flags;
		new_block_operation_p->payload_sg_descriptor = bo->payload_sg_descriptor;        

        fbe_payload_ex_set_lun_performace_stats_ptr(payload_p, lun_p->performance_stats.counter_ptr.lun_counters);

	    /* activate this block operation*/
		fbe_payload_ex_increment_block_operation_level(payload_p);

		/* Set completion function */
		fbe_transport_set_completion_function(new_packet_p, fbe_ext_pool_lun_split_and_send_func_packet_completion, lun_p);

		
		/* Each I/O will be sent down with it's own memory master */
		saved_payload_memory_operation = new_packet_p->payload_union.payload_ex.payload_memory_operation;

		status = fbe_transport_add_subpacket(packet_p, new_packet_p); /* We need to associate the new packet with the original one */

		/* Restore memory operation */
		new_packet_p->payload_union.payload_ex.payload_memory_operation = saved_payload_memory_operation;

		if (status != FBE_STATUS_OK) { 
			fbe_transport_destroy_packet(new_packet_p);
			fbe_transport_destroy_subpackets(packet_p);
			fbe_memory_free_request_entry(&packet_p->memory_request);
			fbe_transport_set_status(packet_p, status, 0);
			fbe_transport_complete_packet(packet_p);
			return; 
		}
		fbe_queue_push(&tmp_queue, &new_packet_p->queue_element);
		if(memory_request_p->chunk_size == FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO){
			/* Just increment the packet pointer */
			new_packet_p++;
		}
	} /* for(i = first_stripe; i <= last_stripe; i++) */

	fbe_transport_set_status(packet_p, FBE_STATUS_INVALID, 0);

    /* Put the original packet on the terminator queue while we wait for the subpacket to complete. */
    fbe_transport_set_cancel_function(packet_p, fbe_base_object_packet_cancel_function, (fbe_base_object_t *)lun_p);
    status = fbe_base_object_add_to_terminator_queue((fbe_base_object_t *)lun_p, packet_p);

	/* Send subpackets */
	while(queue_element = fbe_queue_pop(&tmp_queue)){
		new_packet_p = fbe_transport_queue_element_to_packet(queue_element);

	    /* Send packet to the block edge */
		fbe_ext_pool_lun_send_func_packet(lun_p, new_packet_p);
	}

	fbe_queue_destroy(&tmp_queue);
    return;
}


static fbe_status_t 
fbe_ext_pool_lun_split_and_send_func_packet_completion(fbe_packet_t * packet, fbe_packet_completion_context_t in_context)
{    
    fbe_ext_pool_lun_t						* lun = NULL;

	fbe_packet_t					* master_packet;
	fbe_payload_ex_t				* master_payload;
    fbe_payload_block_operation_t	* master_block_operation;
	fbe_status_t					master_status;

    fbe_payload_ex_t				* payload;
    fbe_payload_block_operation_t	* block_operation;
	fbe_status_t					status;

    fbe_cpu_id_t cpu_id = FBE_CPU_ID_INVALID;
	fbe_bool_t is_empty = FBE_FALSE;
    
    /* Get the packet and block payload */
    lun = (fbe_ext_pool_lun_t*)in_context;
    payload = fbe_transport_get_payload_ex(packet);
    block_operation = fbe_payload_ex_get_block_operation(payload);
	status = fbe_transport_get_status_code(packet);

	master_packet = fbe_transport_get_master_packet(packet);
    master_payload = fbe_transport_get_payload_ex(master_packet);
    master_block_operation = fbe_payload_ex_get_block_operation(master_payload);
	master_status = fbe_transport_get_status_code(master_packet);

	/* Update master_block_operation status with block_operation */
	fbe_raid_update_master_block_status(master_block_operation, block_operation);

	/* Update master packet status */
	fbe_transport_update_master_status(&master_status, status);	
	fbe_transport_set_status(master_packet, master_status, 0);

    /* Handling subpacket queue and the master packet needs to be interlocked */
    fbe_spinlock_lock(&((fbe_base_object_t*)lun)->terminator_queue_lock);

    /* Remove subpacket from master queue. */
	fbe_transport_remove_subpacket_is_queue_empty(packet, &is_empty);

	if(is_empty == FBE_FALSE){ /* Not all subpackets completed yet */
        fbe_spinlock_unlock(&((fbe_base_object_t*)lun)->terminator_queue_lock);
		return FBE_STATUS_OK;
	}

	fbe_transport_remove_packet_from_queue(master_packet);

    /* We are safe now */
    fbe_spinlock_unlock(&((fbe_base_object_t*)lun)->terminator_queue_lock);

	fbe_transport_destroy_subpackets(master_packet);

    fbe_memory_free_request_entry(&master_packet->memory_request);

    /* IO has been completed, update performace couters */ 
    if (lun->b_perf_stats_enabled)
    {
        fbe_transport_get_cpu_id(master_packet, &cpu_id);
        fbe_ext_pool_lun_perf_stats_update_perf_counters_io_completion(lun,
                                                              master_block_operation,
                                                              cpu_id,
                                                              master_payload);
    }
		
	fbe_transport_complete_packet(master_packet);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED; /* packet memory already released */
}

static void fbe_ext_pool_lun_traffic_trace(fbe_ext_pool_lun_t *lun_p,
                                  fbe_packet_t *packet_p,
                                  fbe_payload_block_operation_t *block_operation_p,
                                  fbe_bool_t b_start)
{
    fbe_lun_number_t lun_number;
    fbe_u64_t crc_info;
    fbe_sg_element_t *sg_p = NULL;
    fbe_block_count_t blocks;
    fbe_payload_ex_t *payload_p = NULL;

    payload_p = fbe_transport_get_payload_ex(packet_p);

    /* Fetch the crcs for a portion of the blocks so we can trace them.
     */
    fbe_payload_block_get_block_count(block_operation_p, &blocks);
    fbe_payload_ex_get_sg_list(payload_p, &sg_p, NULL);
    fbe_traffic_trace_get_data_crc(sg_p, blocks, 0, &crc_info);
    fbe_database_get_lun_number(((fbe_base_object_t *)((fbe_base_config_t *)lun_p))->object_id, &lun_number);

    fbe_traffic_trace_block_operation(FBE_CLASS_ID_LUN,
                                      block_operation_p,
                                      lun_number,
                                      crc_info,    /*extra information*/
                                      b_start,
                                      fbe_traffic_trace_get_priority(packet_p));
}
/*************************
 * end file fbe_extent_pool_lun_executer.c
 *************************/
 
 
 
