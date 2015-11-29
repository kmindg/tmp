 /***************************************************************************
  * Copyright (C) EMC Corporation 2009-2010
  * All rights reserved.
  * Licensed material -- property of EMC Corporation
  ***************************************************************************/
 
 /*!**************************************************************************
  * @file fbe_provision_drive_stripe_lock.c
  ***************************************************************************
  *
  * @brief
  *  This file contains the functions related to stripe lock functionality
  *  for the provision drive object.
  * 
  * @ingroup provision_drive_class_files
  * 
  * @version
  *   07/07/2010:  Created. Dhaval Patel
  *
  ***************************************************************************/
 
 /*************************
  *   INCLUDE FILES
  *************************/
#include "fbe/fbe_provision_drive.h"
#include "fbe_provision_drive_private.h"
#include "fbe_raid_library.h"
#include "fbe_service_manager.h"
#include "fbe_transport_memory.h"

 /*************************
  *   FUNCTION DEFINITIONS
  *************************/
fbe_status_t fbe_provision_drive_acquire_stripe_lock_completion(fbe_packet_t * packet_p,
                                                                       fbe_packet_completion_context_t context);

static fbe_raid_state_status_t fbe_provision_drive_iots_resend_aquire_stripe_lock(fbe_raid_iots_t *iots_p);


static fbe_status_t fbe_provision_drive_release_stripe_lock_completion(fbe_packet_t * packet_p,
                                                                       fbe_packet_completion_context_t context);

static fbe_raid_state_status_t fbe_provision_drive_iots_resend_release_stripe_lock(fbe_raid_iots_t *iots_p);

static fbe_status_t fbe_provision_stripe_lock_operation_entry(fbe_packet_t * packet, fbe_packet_completion_context_t context);

/*!****************************************************************************
 * fbe_provision_drive_determine_stripe_lock_range()
 ******************************************************************************
 *
 * @brief
 *  This function is used to determine the stripe lock range for the given range
 *  of lba of the i/o operation.
 *
 * @param   provision_drive_p   - pointer to the provision drive object.
 * @param   start_lba           - start lba of the range.
 * @param   end_lba             - end lba of the range.
 * @param   stripe_number_p     - returns stripe number from we need to start.
 * @param   stripe_count_p      - number of stripe for which we need a lock.
 * 
 * @return  fbe_status_t        -  status of the operation.
 *
 * @version
 *    23/06/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
fbe_status_t
fbe_provision_drive_determine_stripe_lock_range(fbe_provision_drive_t * provision_drive_p,
                                                fbe_lba_t start_lba,
                                                fbe_block_count_t block_count,
                                                fbe_chunk_size_t chunk_size,
                                                fbe_u64_t * stripe_number_p,
                                                fbe_u64_t * stripe_count_p)
{
    fbe_u64_t   start_chunk_index;
    fbe_u64_t   end_chunk_index;
    
    if(start_lba == FBE_LBA_INVALID){
        /* we do not have a range here, so return the error. */
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if(block_count == 0){
        /* we do not have range so return error. */
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /* get the chunk index corresponds to start lba. */
    start_chunk_index = start_lba / chunk_size;
	end_chunk_index = (start_lba + block_count) / chunk_size;

	if((start_lba + block_count) % chunk_size){
		end_chunk_index++;
	}

    *stripe_number_p = start_chunk_index * chunk_size;
    *stripe_count_p = (end_chunk_index - start_chunk_index) * chunk_size;
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_determine_stripe_lock_range()
 ******************************************************************************/


/*!****************************************************************************
 * fbe_provision_drive_is_stripe_lock_needed()
 ******************************************************************************
 * @brief
 *  This function determines whether this block operation requires stripe lock
 *  or not.
 * 
 * @param block_opcode                  - block operation opcode.
 * @param is_stripe_lock_needed_b_p     - Pointer to the bool, it returns whether
 *                                        caller has to acquire stripe lock or
 *                                        not.
 *
 * @return fbe_status_t.
 *
 * @author
 *  6/29/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
fbe_status_t 
fbe_provision_drive_is_stripe_lock_needed(fbe_provision_drive_t * provision_drive,
                                          fbe_payload_block_operation_t * block_operation, 
                                          fbe_bool_t * is_stripe_lock_needed)
{
    fbe_payload_block_operation_opcode_t    block_opcode;
    fbe_lba_t lba;
    fbe_block_count_t block_count;
    fbe_lba_t paged_metadata_start_lba;
    fbe_lba_t zero_checkpoint;
    fbe_lba_t exported_capacity;
    fbe_status_t status;

    /* initialize stripe lock needed as false. */
    *is_stripe_lock_needed = FBE_FALSE;

    fbe_payload_block_get_opcode(block_operation, &block_opcode);
    fbe_payload_block_get_lba(block_operation, &lba);
    fbe_payload_block_get_block_count(block_operation, &block_count);
    fbe_base_config_metadata_get_paged_record_start_lba((fbe_base_config_t *) provision_drive, &paged_metadata_start_lba);

    /* Determine if we need to take the stripe lock for the metadata or not*/
    if(lba >= paged_metadata_start_lba)
    {
        *is_stripe_lock_needed = FBE_FALSE;
        return FBE_STATUS_OK;
    }

    status = fbe_provision_drive_metadata_get_background_zero_checkpoint(provision_drive, &zero_checkpoint);
    if (status != FBE_STATUS_OK) {
        fbe_provision_drive_utils_trace( provision_drive,
                     FBE_TRACE_LEVEL_ERROR,
                     FBE_TRACE_MESSAGE_ID_INFO, 
                     FBE_PROVISION_DRIVE_DEBUG_FLAG_BGZ_TRACING,
                     "pvd_is_stripe_lock_needed: Failed to get checkpoint. Status: 0x%X\n", status);

        return FBE_STATUS_OK;
    }

    fbe_base_config_get_capacity((fbe_base_config_t *)provision_drive, &exported_capacity );

    /* We should always get a stripe lock if we are a zero. 
     * The first zero request will start with the checkpoint still set to end marker. 
     */
    if (block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ZERO)
    {
        *is_stripe_lock_needed = FBE_TRUE;
        return FBE_STATUS_OK;
    }

    /* If the `forced write' flag is set, we need to execute a zero on demand.
     */
    if (fbe_payload_block_is_flag_set(block_operation, FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_FORCED_WRITE))
    {
        *is_stripe_lock_needed = FBE_TRUE;
        return FBE_STATUS_OK;
    }

#if 0
    /* Need to get lock to check paged if this sector has been unmapped
     * UNMAP TODO: use the provision drive metadata cache
     */
    if (fbe_provision_drive_is_unmap_supported(provision_drive))
    {
        if (block_opcode != FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_NEGOTIATE_BLOCK_SIZE)
        {
            *is_stripe_lock_needed = FBE_TRUE; 
            return FBE_STATUS_OK;
        }
    }
#endif
    /* If zero checkpoint greater than the last lba we do not need to take a lock */
    if ((zero_checkpoint > lba + block_count) && fbe_provision_drive_unmap_bitmap_is_lba_range_mapped(provision_drive, lba, block_count))
    {
        *is_stripe_lock_needed = FBE_FALSE;
        return FBE_STATUS_OK;
    }

    /*!@todo we might not need stripe lock for the verify operation, will add 
     * after confirmation with RAID team.
     */
    switch(block_opcode)
    {
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_NEGOTIATE_BLOCK_SIZE:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY:
            break;
        default:
            *is_stripe_lock_needed = FBE_TRUE;
    }

    return FBE_STATUS_OK;
}

/******************************************************************************
 * end fbe_provision_drive_is_stripe_lock_needed()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_acquire_stripe_lock()
 ******************************************************************************
 * @brief
 *  Get the approrpirate stripe lock for the lba range specified by the user,
 *  takes appropriate lock based on the block operation opcode.
 * 
 * @param provision_drive_p - provision drive object.
 * @param packet_p          - pointer to the packet.
 *
 * @return fbe_status_t.
 *
 * @author
 *  6/29/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
fbe_status_t
fbe_provision_drive_acquire_stripe_lock(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{
    fbe_lba_t                               stripe_number;
    fbe_block_count_t                       stripe_count;
    fbe_payload_block_operation_opcode_t    opcode;
    fbe_status_t                            status;
    fbe_payload_ex_t *                     sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_payload_stripe_lock_operation_t *   stripe_lock_operation_p = NULL;
    fbe_payload_block_operation_t *         block_operation_p = NULL;
    fbe_lba_t                               start_lba;
    fbe_block_count_t                       block_count;
    fbe_raid_iots_t *                       iots_p = NULL;
    fbe_provision_drive_t * provision_drive_p = (fbe_provision_drive_t *)context;

    /* Increment the stripe lock wait count before acquiring stripe lock. */

    /* Get the block operation of the packet to determine lba, block and opcode. */
    block_operation_p = fbe_payload_ex_get_block_operation(sep_payload_p);
    fbe_payload_ex_get_iots(sep_payload_p, &iots_p);

    fbe_payload_block_get_lba(block_operation_p, &start_lba);
    fbe_payload_block_get_block_count(block_operation_p, &block_count);
    fbe_payload_block_get_opcode(block_operation_p, &opcode);

    /* Determine the stripe lock range for which user needs a stripe lock. */
    status = fbe_provision_drive_determine_stripe_lock_range(provision_drive_p,
                                                             start_lba,
                                                             block_count,
                                                             FBE_PROVISION_DRIVE_CHUNK_SIZE,
                                                             &stripe_number,
                                                             &stripe_count);
    if(status != FBE_STATUS_OK)
    {
        /* stripe lock range determination failed. */
        fbe_payload_block_set_status(block_operation_p,
                                     FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                                     FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_LOCK_FAILED);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        //fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Allocate the stripe lock operation from the payload. */
    stripe_lock_operation_p = fbe_payload_ex_allocate_stripe_lock_operation(sep_payload_p);
    if(stripe_lock_operation_p == NULL)
    {
        /* Allocation of stripe lock operation failed, fail the packet with error. */
        fbe_payload_block_set_status(block_operation_p,
                         FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED,
                         FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INSUFFICIENT_RESOURCES);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        //fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }   

    /* Build the stripe lock operation and start it. Notice that the `allow hold'
     * is always set.
     */
    if(opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ)
    {
        /* If it is read operation then acquire read lock otherwise acquire write lock. */
        fbe_payload_stripe_lock_build_read_lock(stripe_lock_operation_p, 
                                                &provision_drive_p->base_config.metadata_element, 
                                                stripe_number, stripe_count);

		if(1){ /* TODO Deprecated nedd to be removed */
			stripe_lock_operation_p->flags |= FBE_PAYLOAD_STRIPE_LOCK_FLAG_ALLOW_HOLD;
		}

    }
    else
    {
        /* For all other block operation, acquire write stripe lock.
         * We set the "hold" flag to true since monitor operations should always get failed back 
         * to the monitor and should not wait. 
         */
        fbe_payload_stripe_lock_build_write_lock(stripe_lock_operation_p, 
                                                 &provision_drive_p->base_config.metadata_element, 
                                                 stripe_number, stripe_count);

		if(!fbe_payload_block_is_monitor_opcode(opcode)){ /* TODO Deprecated nedd to be removed */
			stripe_lock_operation_p->flags |= FBE_PAYLOAD_STRIPE_LOCK_FLAG_ALLOW_HOLD;
		}

    }

    fbe_payload_stripe_lock_set_sync_mode(stripe_lock_operation_p);

    fbe_payload_ex_increment_stripe_lock_operation_level(sep_payload_p);

    fbe_provision_drive_utils_trace( provision_drive_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          FBE_PROVISION_DRIVE_DEBUG_FLAG_LOCK_TRACING,
                          "%s: start: 0x%llx blk: 0x%llx opcode: 0x%x s_n: 0x%llx s_c: 0x%llx\n", 
                          __FUNCTION__, start_lba, block_count, opcode,
                          stripe_lock_operation_p->stripe.first, stripe_lock_operation_p->stripe.last - stripe_lock_operation_p->stripe.first + 1);

    /* Set the completion function before sending request to acquire stripe lock. */
    fbe_transport_set_completion_function(packet_p, fbe_provision_drive_acquire_stripe_lock_completion, provision_drive_p);                           
    fbe_transport_set_completion_function(packet_p, fbe_provision_stripe_lock_operation_entry, provision_drive_p);                           

    /* Send a request to allocate the stripe lock. */
    // status = fbe_stripe_lock_operation_entry(packet_p);
    return FBE_STATUS_CONTINUE;
}
/******************************************************************************
 * end fbe_provision_drive_acquire_stripe_lock()
 ******************************************************************************/

static fbe_status_t 
fbe_provision_stripe_lock_operation_entry(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    status = fbe_stripe_lock_operation_entry(packet);

    /* The sync mode is set */
    if(status == FBE_STATUS_OK){
        /* The fbe_provision_drive_acquire_stripe_lock_completion is on the packet stack already, just return FBE_STATUS_OK */
        return FBE_STATUS_OK;
    }

    /* Break the completion context */
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}

/*!****************************************************************************
 * fbe_provision_drive_acquire_stripe_lock_completion()
 ******************************************************************************
 * @brief
 *  Handle the completion of the stripe lock allocation.
 *
 * @param packet_p - Packet that is completing.
 * @param context -  Provision drive object pointer. 
 *
 * @return fbe_status_t.
 *
 * @author
 *  06/29/2009 - Created. Dhaval Patel
 *
 *******************************************************************************/
fbe_status_t
fbe_provision_drive_acquire_stripe_lock_completion(fbe_packet_t * packet_p,
                                                   fbe_packet_completion_context_t context)
{
    fbe_status_t                            status;
    fbe_provision_drive_t *                 provision_drive_p = (fbe_provision_drive_t *)context;
    fbe_payload_ex_t *                     sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_payload_stripe_lock_operation_t *   stripe_lock_operation_p = NULL;
    fbe_payload_stripe_lock_status_t        stripe_lock_status;
    fbe_payload_block_operation_t *         block_operation_p = NULL;
    fbe_lba_t                               start_lba;
    fbe_block_count_t                       block_count;
    fbe_payload_block_operation_opcode_t    block_opcode;
    fbe_raid_iots_t *                       iots_p = NULL;


    /* Get the stripe lock operation from the sep payload. */
    stripe_lock_operation_p = fbe_payload_ex_get_stripe_lock_operation(sep_payload_p);

    status = fbe_transport_get_status_code(packet_p);
    fbe_payload_stripe_lock_get_status(stripe_lock_operation_p, &stripe_lock_status);

    /* Get the previous block operation. */
    block_operation_p = fbe_payload_ex_get_present_block_operation(sep_payload_p);

	if((status == FBE_STATUS_OK) && (stripe_lock_status == FBE_PAYLOAD_STRIPE_LOCK_STATUS_OK)) {
        fbe_payload_block_set_status(block_operation_p,
                                     FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS,
                                     FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);

        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
		return FBE_STATUS_OK;
    }

    fbe_payload_ex_get_iots(sep_payload_p, &iots_p);
    fbe_payload_block_get_lba(block_operation_p, &start_lba);
    fbe_payload_block_get_block_count(block_operation_p, &block_count);
    fbe_payload_block_get_opcode(block_operation_p, &block_opcode);

    fbe_provision_drive_utils_trace( provision_drive_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          FBE_PROVISION_DRIVE_DEBUG_FLAG_LOCK_TRACING,
                          "pvd_acq_SL_compl:lba:0x%llx blk:0x%llx opcode:0x%x s_n:0x%llx s_c:0x%llx\n", 
                          start_lba, block_count, block_opcode,
                          stripe_lock_operation_p->stripe.first, stripe_lock_operation_p->stripe.last - stripe_lock_operation_p->stripe.first + 1);

    fbe_base_object_trace( (fbe_base_object_t*)provision_drive_p,
                           FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                           "pvd_acq_SL_comp: lba:0x%llx blk:0x%llx opcode:0x%x pkt_s:0x%x sl_s:0x%x\n", 
                           (unsigned long long)start_lba,
		       (unsigned long long)block_count, block_opcode,
		       status, stripe_lock_operation_p->status);
    /* This is the completion routine for geting stripe lock.   If request is dropped
     * we will enqueue to the terminator queue to allow this request to
     * get processed later.  We were dropped because of a drive going away or a quiesce. 
     * Once the monitor resumes I/O, this request will get restarted. 
     */
    if (((stripe_lock_status == FBE_PAYLOAD_STRIPE_LOCK_STATUS_DROPPED) ||
         (stripe_lock_status == FBE_PAYLOAD_STRIPE_LOCK_STATUS_ABORTED)) &&
        ((stripe_lock_operation_p->flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_ALLOW_HOLD) != 0 && 
         (packet_p->packet_attr & FBE_PACKET_FLAG_DO_NOT_QUIESCE) == 0))
    {
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "pvd_acq_SL_comp failed. queuing req. lba: 0x%llx sl_st: 0x%x pkt_s: 0x%x op: %d\n", 
                              start_lba, stripe_lock_status, status, stripe_lock_operation_p->opcode);
    
        /* Do not queue redirected IOs. */
        if (packet_p->packet_attr & FBE_PACKET_FLAG_REDIRECTED)
        {
            fbe_payload_ex_release_stripe_lock_operation(sep_payload_p, stripe_lock_operation_p);
            fbe_transport_set_status(packet_p, FBE_STATUS_QUIESCED, 0);
            return FBE_STATUS_OK;
        }
        fbe_raid_iots_transition_quiesced(iots_p, fbe_provision_drive_iots_resend_aquire_stripe_lock, provision_drive_p);
        fbe_payload_ex_release_stripe_lock_operation(sep_payload_p, stripe_lock_operation_p);
        fbe_base_object_add_to_terminator_queue((fbe_base_object_t *)provision_drive_p, packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }
    /* For some reason the stripe lock was not available.
     * We will fail the request. 
     */
    else if (stripe_lock_status == FBE_PAYLOAD_STRIPE_LOCK_STATUS_ABORTED) {
        /* We intentionally aborted this lock due to shutdown. */
        fbe_payload_block_set_status(block_operation_p,
                                     FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED,
                                     FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE);
		status = FBE_STATUS_OK;
    }
    else if (fbe_transport_is_packet_cancelled(packet_p) || 
             (status == FBE_STATUS_CANCELED) || 
             (status == FBE_STATUS_CANCEL_PENDING))
    {
        fbe_payload_block_set_status(block_operation_p,
                                     FBE_PAYLOAD_BLOCK_OPERATION_STATUS_REQUEST_ABORTED,
                                     FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_CLIENT_ABORTED);
        
    }
    else
    {
        /* Other errors should only get reported on metadata ops, which will specifically handle this error. */
        fbe_payload_block_set_status(block_operation_p,
                                     FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED,
                                     FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_LOCK_FAILED);
        
        if ((stripe_lock_status != FBE_PAYLOAD_STRIPE_LOCK_STATUS_DROPPED) &&
            (stripe_lock_status != FBE_PAYLOAD_STRIPE_LOCK_STATUS_ABORTED))
        {
            /* We expect dropped and aborted, but no other status values. */
            fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                  "pvd_acq_SL_compl:SL fail sl_status:0x%x packet_status:0x%x op:%d\n", 
                                  stripe_lock_status, status, stripe_lock_operation_p->opcode);

			status = FBE_STATUS_GENERIC_FAILURE;
        }
    }

	/* set the appropriate status. */
    fbe_transport_set_status(packet_p, status, 0);
    
    /* Check if stripe lock was granted */
    if(stripe_lock_status == FBE_PAYLOAD_STRIPE_LOCK_STATUS_OK){ 
        /* We have to release stripe lock */
        /* set the completion function to release the stripe lock before start i/o. */
        fbe_transport_set_completion_function(packet_p,
                                                fbe_provision_drive_block_transport_release_stripe_lock,
                                                provision_drive_p);
        return FBE_STATUS_CONTINUE;

    }

    /* Release the stripe lock operation. */
    fbe_payload_ex_release_stripe_lock_operation(sep_payload_p, stripe_lock_operation_p);

    /* For some reason the stripe lock was not available and so fail the i/o request. */
    fbe_base_object_trace((fbe_base_object_t*) provision_drive_p,
                  FBE_TRACE_LEVEL_DEBUG_LOW,
                  FBE_TRACE_MESSAGE_ID_INFO,
                  "pvd_acq_SL_compl: Lock Fail,s_n:0x%llx s_c:0x%llx,stat:0x%x,sl_stat:0x%x\n",
                  stripe_lock_operation_p->stripe.first,
                  stripe_lock_operation_p->stripe.last - stripe_lock_operation_p->stripe.first + 1, status, stripe_lock_status);
    

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_acquire_stripe_lock_completion()
 ******************************************************************************/

/*!**************************************************************
 * fbe_provision_drive_iots_resend_aquire_stripe_lock()
 ****************************************************************
 * @brief
 *  Restart the iots after it has been queued.
 *  It was queued just before getting a lock
 *
 * @param iots_p - I/O to restart.               
 *
 * @return fbe_raid_state_status_t - FBE_RAID_STATE_STATUS_WAITING
 *
 * @author
 *  8/19/2011 - Created. Wayne Garrett
 *
 ****************************************************************/

static fbe_raid_state_status_t fbe_provision_drive_iots_resend_aquire_stripe_lock(fbe_raid_iots_t *iots_p)
{
    fbe_packet_t *packet_p = fbe_raid_iots_get_packet(iots_p);
    fbe_provision_drive_t * provision_drive_p = (fbe_provision_drive_t*)iots_p->callback_context;
    fbe_payload_ex_t *sep_payload_p = NULL;

    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_payload_ex_get_iots(sep_payload_p, &iots_p);
    fbe_raid_iots_mark_unquiesced(iots_p);

    fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                          "iots_resend_aquire_stripe_lock: restart iots %p packet %p\n", iots_p, packet_p);

    /* Remove from the terminator queue and get the stripe lock.
     */
    fbe_base_object_remove_from_terminator_queue((fbe_base_object_t *) provision_drive_p, packet_p);

    if (fbe_provision_drive_is_local_state_set(provision_drive_p, FBE_PVD_LOCAL_STATE_SLF))
    {
        fbe_transport_set_packet_attr(packet_p, FBE_PACKET_FLAG_SLF_REQUIRED);
        fbe_transport_set_status(packet_p, FBE_STATUS_EDGE_NOT_ENABLED, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_RAID_STATE_STATUS_WAITING;
    }

    /* resend request */
    fbe_provision_drive_acquire_stripe_lock(packet_p, provision_drive_p);

    fbe_transport_complete_packet(packet_p);  // kick it off

    return FBE_RAID_STATE_STATUS_WAITING;
}
/******************************************************************************
 * end fbe_provision_drive_iots_resend_aquire_stripe_lock()
 ******************************************************************************/


/*!****************************************************************************
 * fbe_provision_drive_release_stripe_lock()
 ******************************************************************************
 * @brief
 *  Get the approrpirate stripe lock for the lba range specified by the user,
 *  takes appropriate lock based on the block operation opcode.
 * 
 * @param provision_drive_p - provision drive object.
 * @param packet_p          - pointer to the packet.
 *
 * @return fbe_status_t.
 *
 * @author
 *  6/29/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
fbe_status_t
fbe_provision_drive_release_stripe_lock(fbe_provision_drive_t * provision_drive_p,
                                        fbe_packet_t *  packet_p)
{
    fbe_status_t                            status;
    fbe_payload_ex_t *						sep_payload_p = NULL;
    fbe_payload_stripe_lock_operation_t *   stripe_lock_operation_p = NULL;
    fbe_payload_stripe_lock_operation_opcode_t opcode;


    fbe_payload_block_operation_t *         block_operation_p = NULL;
#if 0
    fbe_payload_block_operation_opcode_t    block_opcode;
    fbe_lba_t                               start_lba;
    fbe_block_count_t                       block_count;
#endif

    sep_payload_p = fbe_transport_get_payload_ex(packet_p);

    /* Get the current stripe lock operation */
    stripe_lock_operation_p = fbe_payload_ex_get_stripe_lock_operation(sep_payload_p);

    fbe_payload_stripe_lock_get_opcode(stripe_lock_operation_p, &opcode);

    /* Get the previous block operation of the packet to determine opcode. */
    block_operation_p = fbe_payload_ex_get_present_block_operation(sep_payload_p);
#if 0
    fbe_payload_block_get_opcode(block_operation_p, &block_opcode);
    fbe_payload_block_get_lba(block_operation_p, &start_lba);
    fbe_payload_block_get_block_count(block_operation_p, &block_count);
#endif

    /* Before releasing the stripe lock update the block operation header packet status. */
    status = fbe_transport_get_status_code(packet_p);
    /* Peter Puhov. We need to find another way */
    fbe_payload_operation_hearder_set_status((fbe_payload_operation_header_t *) block_operation_p, status); 

    /* Build the stripe lock operation and start it. */
    if (opcode == FBE_PAYLOAD_STRIPE_LOCK_OPERATION_OPCODE_READ_LOCK)
    {
        /* If it is read operation then acquire read lock otherwise acquire write lock. */
        fbe_payload_stripe_lock_build_read_unlock(stripe_lock_operation_p); 
    }
    else
    {
        /* For all other block operation, acquire write stripe lock. */
        fbe_payload_stripe_lock_build_write_unlock(stripe_lock_operation_p);
    }

#if 0
    fbe_provision_drive_utils_trace( provision_drive_p,
              FBE_TRACE_LEVEL_DEBUG_HIGH,
              FBE_TRACE_MESSAGE_ID_INFO,
              FBE_PROVISION_DRIVE_DEBUG_FLAG_LOCK_TRACING,
              "%s: start: 0x%llx blk: 0x%x opcode: 0x%llx s_n: 0x%llx s_c: 0x%x\n", 
              __FUNCTION__, start_lba, block_count, block_opcode,
              stripe_lock_operation_p->stripe.first, stripe_lock_operation_p->stripe.last - stripe_lock_operation_p->stripe.first + 1);
#endif

    /* Set the completion function before sending request to acquire stripe lock. */
    fbe_transport_set_completion_function(packet_p, fbe_provision_drive_release_stripe_lock_completion, provision_drive_p);

    /* Send a request to allocate the stripe lock. */
    status = fbe_stripe_lock_operation_entry(packet_p);
    return status;
}
/******************************************************************************
 * end fbe_provision_drive_release_stripe_lock()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_release_stripe_lock_completion()
 ******************************************************************************
 * @brief
 *  Handle the completion of the stripe lock release operation.
 *
 * @param packet_p - Packet that is completing.
 * @param context -  Provision drive object pointer. 
 *
 * @return fbe_status_t.
 *
 * @author
 *  06/29/2009 - Created. Dhaval Patel
 *
 *******************************************************************************/
static fbe_status_t
fbe_provision_drive_release_stripe_lock_completion(fbe_packet_t * packet_p,
                                                   fbe_packet_completion_context_t context)
{
    fbe_status_t                            status;
    fbe_provision_drive_t *                 provision_drive_p = (fbe_provision_drive_t *)context;
    fbe_payload_ex_t *                     sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_payload_stripe_lock_operation_t *   stripe_lock_operation_p = NULL;
    fbe_payload_stripe_lock_status_t        stripe_lock_status;
    fbe_payload_block_operation_t *         block_operation_p = NULL;
    fbe_lba_t                               start_lba;
    fbe_block_count_t                       block_count;
    fbe_payload_block_operation_opcode_t    block_opcode;
    fbe_raid_iots_t *                       iots_p;

    /* Get the stripe lock operation from the sep payload. */
    status = fbe_transport_get_status_code(packet_p);
    stripe_lock_operation_p = fbe_payload_ex_get_stripe_lock_operation(sep_payload_p);
    fbe_payload_stripe_lock_get_status(stripe_lock_operation_p, &stripe_lock_status);

    /* Get the previous block operation. */
    block_operation_p = fbe_payload_ex_get_present_block_operation(sep_payload_p);
    fbe_payload_block_get_lba(block_operation_p, &start_lba);
    fbe_payload_block_get_block_count(block_operation_p, &block_count);
    fbe_payload_block_get_opcode(block_operation_p, &block_opcode);

    fbe_provision_drive_utils_trace( provision_drive_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          FBE_PROVISION_DRIVE_DEBUG_FLAG_LOCK_TRACING,
                          "pvd_release_SL_compl: start: 0x%llx blk: 0x%llx opcode: 0x%x s_n: 0x%llx s_c: 0x%llx\n", 
                          start_lba, block_count, block_opcode,
                          stripe_lock_operation_p->stripe.first, stripe_lock_operation_p->stripe.last - stripe_lock_operation_p->stripe.first + 1);

    if ((status != FBE_STATUS_OK) || 
        (stripe_lock_status != FBE_PAYLOAD_STRIPE_LOCK_STATUS_OK))
    {
        /* stripe lock release failed with error and so fail the i/o request. */
        fbe_base_object_trace((fbe_base_object_t*) provision_drive_p,
                      FBE_TRACE_LEVEL_ERROR,
                      FBE_TRACE_MESSAGE_ID_INFO,
                      "pvd_release_SL_compl: unlock fail, s_n: 0x%llx s_c: 0x%llx, status:0x%x,slpvd_release_SL_compl_status: 0x%x\n",
                      stripe_lock_operation_p->stripe.first,
                      stripe_lock_operation_p->stripe.last - stripe_lock_operation_p->stripe.first + 1, status, stripe_lock_status);

        /* If request is dropped we will enqueue to the terminator queue to allow this request to
         * get processed later.  We were dropped because of a drive going away or a quiesce. 
         * Once the monitor resumes I/O, this request will get restarted. 
         */
        if (((stripe_lock_status == FBE_PAYLOAD_STRIPE_LOCK_STATUS_DROPPED) ||
             (stripe_lock_status == FBE_PAYLOAD_STRIPE_LOCK_STATUS_ABORTED)) &&
            ((stripe_lock_operation_p->flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_ALLOW_HOLD) != 0))
        {
            fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s failed. queuing req. sl_status: 0x%x pckt_status: 0x%x op: %d\n", 
                                  __FUNCTION__, stripe_lock_status, status, stripe_lock_operation_p->opcode);
        
            /* Do not queue redirected IOs. */
            if (packet_p->packet_attr & FBE_PACKET_FLAG_REDIRECTED)
            {
                fbe_payload_ex_release_stripe_lock_operation(sep_payload_p, stripe_lock_operation_p);
                fbe_transport_set_status(packet_p, FBE_STATUS_QUIESCED, 0);
                return FBE_STATUS_OK;
            }
            fbe_payload_ex_get_iots(sep_payload_p, &iots_p);
            fbe_raid_iots_transition_quiesced(iots_p, fbe_provision_drive_iots_resend_release_stripe_lock, provision_drive_p);
            fbe_payload_ex_release_stripe_lock_operation(sep_payload_p, stripe_lock_operation_p);
            fbe_base_object_add_to_terminator_queue((fbe_base_object_t *)provision_drive_p, packet_p);
            return FBE_STATUS_MORE_PROCESSING_REQUIRED;
        }        
    }

    /* Release the stripe lock operation. */
    fbe_payload_ex_release_stripe_lock_operation(sep_payload_p, stripe_lock_operation_p);

    /* update the packet status with previous block operation packet status. */
    /* Peter Puhov. We need to find another way */
    status = fbe_payload_operation_hearder_get_status((fbe_payload_operation_header_t *) block_operation_p);
    fbe_transport_set_status(packet_p, status, 0);
    return status;
}
/******************************************************************************
 * end fbe_provision_drive_release_stripe_lock_completion()
 ******************************************************************************/


/*!**************************************************************
 * fbe_provision_drive_iots_resend_release_stripe_lock()
 ****************************************************************
 * @brief
 *  Restart the iots after it has been queued.
 *  It was queued just before getting a lock
 *
 * @param iots_p - I/O to restart.               
 *
 * @return fbe_raid_state_status_t - FBE_RAID_STATE_STATUS_WAITING
 *
 * @author
 *  8/19/2011 - Created. Wayne Garrett
 *
 ****************************************************************/

static fbe_raid_state_status_t fbe_provision_drive_iots_resend_release_stripe_lock(fbe_raid_iots_t *iots_p)
{
    fbe_packet_t *packet_p = fbe_raid_iots_get_packet(iots_p);
    fbe_provision_drive_t * provision_drive_p = (fbe_provision_drive_t*)iots_p->callback_context;
    fbe_payload_ex_t *sep_payload_p = NULL;

    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_payload_ex_get_iots(sep_payload_p, &iots_p);
    fbe_raid_iots_mark_unquiesced(iots_p);

    fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s: restarting iots %p\n", __FUNCTION__, iots_p);

    /* Remove from the terminator queue and get the stripe lock.
     */
    fbe_base_object_remove_from_terminator_queue((fbe_base_object_t *) provision_drive_p, packet_p);

    /* resend request */

    fbe_transport_set_completion_function(packet_p, fbe_provision_drive_release_stripe_lock_completion, provision_drive_p);
    fbe_stripe_lock_operation_entry(packet_p);
    return FBE_RAID_STATE_STATUS_WAITING;
}
/******************************************************************************
 * end fbe_provision_drive_iots_resend_release_stripe_lock()
 ******************************************************************************/


/*!**************************************************************
 * fbe_provision_drive_get_NP_lock()
 ****************************************************************
 * @brief
 *  This function gets the Non paged clustered lock
 *
 * @param provision_drive_p
 * @param packet_p
 * @param completion_function
 *
 * @return fbe_status_t
 *
 * @author
 *  10/18/2011 - Created. Ashwin Tamilarasan
 *
 ****************************************************************/

fbe_status_t fbe_provision_drive_get_NP_lock(fbe_provision_drive_t*  provision_drive_p,
                                             fbe_packet_t*    packet_p,
                                             fbe_packet_completion_function_t  completion_function)
{

    fbe_transport_set_completion_function(packet_p, completion_function, provision_drive_p);
    
     fbe_base_config_get_np_distributed_lock((fbe_base_config_t*)provision_drive_p,
                                             packet_p);  


    return FBE_STATUS_MORE_PROCESSING_REQUIRED; 
    
}
/******************************************************************************
 * end fbe_provision_drive_get_NP_lock()
 ******************************************************************************/


/*!**************************************************************
 * fbe_provision_drive_release_NP_lock()
 ****************************************************************
 * @brief
 *  This function releases the Non paged clustered lock
 *
 * @param packet_p
 * @param context
 *
 * @return fbe_status_t
 *
 * @author
 *  10/18/2011 - Created. Ashwin Tamilarasan
 *
 ****************************************************************/

fbe_status_t fbe_provision_drive_release_NP_lock(fbe_packet_t*    packet_p,
                                                fbe_packet_completion_context_t context)
{
        
    fbe_provision_drive_t*       provision_drive_p = NULL;

    provision_drive_p = (fbe_provision_drive_t*)context;
    fbe_base_config_release_np_distributed_lock((fbe_base_config_t*)provision_drive_p, packet_p);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED; 
    
}
/******************************************************************************
 * end fbe_provision_drive_release_NP_lock()
 ******************************************************************************/

/*******************************************
 * end file fbe_provision_drive_stripe_lock.c
 *******************************************/
 
 
 
