/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_base_config_io_quiesce.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the functionality related to quiesce and unquiesce
 *  the i/o operation related functionality.
 *
 *  Different object monitor will use this functionality to quiesce and 
 *  unquiesce i/o for various purpose.
 * 
 * @ingroup base_config_class_files
 * 
 * @version
 *   07/07/2010:  Created. Dhaval Patel
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe_base_config_private.h"
#include "fbe_raid_library.h"
#include "fbe_cmi.h"

static fbe_status_t fbe_base_config_restart_quiesced_requests(fbe_base_config_t * const base_config_p);
static fbe_status_t fbe_base_config_quiesce_io_start_is_metadata_memory_updated(fbe_base_config_t * base_config_p, 
                                                                                fbe_bool_t * out_md_mem_updated_p);
static fbe_status_t fbe_base_config_quiesce_io_complete_is_metadata_memory_updated(fbe_base_config_t * base_config_p, 
                                                                                   fbe_bool_t * out_md_mem_updated_p);
static fbe_status_t fbe_base_config_quiesce_start_state_check(fbe_base_config_t * base_config_p, 
                                                              fbe_bool_t peer_alive_b,
                                                              fbe_base_config_metadata_memory_flags_t local_quiesce_state,
                                                              fbe_base_config_metadata_memory_flags_t peer_quiesce_state,
                                                              fbe_bool_t * quiesce_ready_b);
static fbe_status_t fbe_base_config_quiesce_complete_state_check(fbe_base_config_t * base_config_p, 
                                                                 fbe_bool_t peer_alive_b,
                                                                 fbe_base_config_metadata_memory_flags_t local_quiesce_state,
                                                                 fbe_base_config_metadata_memory_flags_t peer_quiesce_state,
                                                                 fbe_bool_t * quiesce_complete_b);
static fbe_status_t fbe_base_config_unquiesce_io_is_metadata_memory_updated(fbe_base_config_t * base_config_p, fbe_bool_t * out_md_mem_updated_p);
static fbe_status_t fbe_base_config_unquiesce_state_check(fbe_base_config_t * base_config_p, 
                                                          fbe_bool_t peer_alive_b,
                                                          fbe_base_config_metadata_memory_flags_t local_quiesce_state,
                                                          fbe_base_config_metadata_memory_flags_t peer_quiesce_state,
                                                          fbe_bool_t * unquiesce_ready_b);

static fbe_status_t fbe_base_config_quiesce_update_memory_request_priority(fbe_base_config_t * const base_config_p, fbe_packet_t * packet);


/*!****************************************************************************
 * fbe_base_config_quiesce_ios()
 ******************************************************************************
 * @brief
 *  This function is used to determine whether all the i/o which is currently
 *  in base object's terminator queue are already quiesce or not.
 *
 * @param base_config_p - a pointer to the base config.
 * @param b_is_object_in_pass_thru - FBE_FALSE (typical) object is not in `pass-thru' mode
 *          FBE_TRUE - Object is in pass-thru, we must stop new I/O
 *
 * @return fbe_status_t
 *
 * @author
 *  10/21/2009 - Created. Rob Foley
 ******************************************************************************/
static fbe_bool_t fbe_base_config_quiesce_ios(fbe_base_config_t * const base_config_p, fbe_bool_t b_is_object_in_pass_thru)
{
    fbe_status_t            status;
    fbe_bool_t              b_quiesced = FBE_FALSE;     /*! @note by default the object is not quiesced. */
    fbe_bool_t              b_is_iots_quiesced;
    fbe_queue_head_t *      termination_queue_p = NULL;
    fbe_packet_t *          current_packet_p = NULL;
    fbe_queue_element_t *   next_element_p = NULL;
    fbe_raid_iots_t *       iots_p;
    fbe_queue_element_t *   queue_element_p = NULL;
    fbe_u32_t               terminator_count = 0;
    fbe_u32_t               iots_count = 0;
	fbe_u32_t outstanding_io = 0;
    fbe_bool_t              b_metadata_req = FBE_FALSE;
    fbe_class_id_t          class_id;

    /* Get class id for this object */
    class_id = ((fbe_base_object_t *) base_config_p)->class_id;

    /* Acquire lock before we look at the termination queue. */
    fbe_spinlock_lock(&base_config_p->base_object.terminator_queue_lock);

    /* Always check the terminator queue.
     */

    /* Get the head of the termination queue after acquiring lock. */
    termination_queue_p = fbe_base_object_get_terminator_queue_head((fbe_base_object_t *) base_config_p);
    
    /* Loop over the termination queue and break out when we find something that is not quiesced.  */
    queue_element_p = fbe_queue_front(termination_queue_p);
    
    while (queue_element_p != NULL)
    {
        fbe_payload_ex_t * sep_payload_p = NULL;
        current_packet_p = fbe_transport_queue_element_to_packet(queue_element_p);
        sep_payload_p = fbe_transport_get_payload_ex(current_packet_p);
    
        next_element_p = fbe_queue_next(termination_queue_p, &current_packet_p->queue_element);
    
        fbe_payload_ex_get_iots(sep_payload_p, &iots_p);
        terminator_count++; 
    
        /* Check if this is a metadata request 
         */
        if (((class_id > FBE_CLASS_ID_RAID_FIRST) && 
             (class_id < FBE_CLASS_ID_RAID_LAST)     ) ||
            (class_id == FBE_CLASS_ID_VIRTUAL_DRIVE)      )
        {
            b_metadata_req = fbe_raid_iots_is_metadata_request(iots_p);
        }
    
        /* Verify whether this iots is used or not, if its current status shows
         * that it is not used then we are not using this iots for any i/o operation
         * and so it will not be considered as pending iots which needs to be
         * quiesce.
         */
        if (iots_p->status == FBE_RAID_IOTS_STATUS_AT_LIBRARY)
        {
            status = fbe_raid_iots_quiesce_with_lock(iots_p, &b_is_iots_quiesced);
            if (status != FBE_STATUS_OK) 
            { 
                fbe_base_object_trace((fbe_base_object_t *) base_config_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                      "base_cfg_is_qscd iots: %p status: 0x%x\n", iots_p, status);
            }
            /* If this iots is not quiesced, break out now.
             * No need to check the rest of the iots, as we will need to wait anyway for this 
             * iots to quiesce. 
             */
            if (!b_is_iots_quiesced)
            {
                break;
            }
            iots_count++;
    
            if(b_metadata_req)
            {
               /* If metadata request, also count for its master packet which
                * is not in the terminator queue and is in quiesced state.  
                */
                iots_count++;
            }
        }
        else if(iots_p->status == FBE_RAID_IOTS_STATUS_NOT_STARTED_TO_LIBRARY)
        {
            /* IOTS has already been quiesced */
            iots_count++;
            if(b_metadata_req)
            {
                fbe_packet_t *master_packet;
               /* It is a metadata request and has master packet.
                * So we need count the master packet also
                */
                master_packet = (fbe_packet_t *)fbe_transport_get_master_packet(current_packet_p);
                if (master_packet != NULL) {
                    iots_count++;
                } else {
                    fbe_base_object_trace((fbe_base_object_t *)base_config_p,
                                          FBE_TRACE_LEVEL_WARNING,
                                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                          "%s metadata IOTS %p without master packet\n",
                                          __FUNCTION__, iots_p);
                }
            }

        }
        else if(iots_p->status == FBE_RAID_IOTS_STATUS_WAITING_FOR_QUIESCE)
        {
            iots_count++;
        }

		fbe_base_config_quiesce_update_memory_request_priority(base_config_p, current_packet_p);

        queue_element_p = next_element_p;
    }
    
    /* There may be things in flight not yet terminated.
     * If this is the case, then we are not quiesced. 
     * Waiting lock items are not terminated, but also will not complete since 
     * they are waiting for locks that terminated I/Os have. 
     */

    /* Get the outstanding I/O count.
     */
    fbe_spinlock_lock(&base_config_p->block_transport_server.queue_lock);
	fbe_block_transport_server_get_outstanding_io_count(&base_config_p->block_transport_server, &outstanding_io);

    /* First unlock transport server then terminator.
     */
    fbe_spinlock_unlock(&base_config_p->block_transport_server.queue_lock);
    fbe_spinlock_unlock(&base_config_p->base_object.terminator_queue_lock);

    /* If all outstanding I/Os have been placed on the terminator queue the
     * object is quiesced.
     */
    if (outstanding_io == iots_count)
    {
        b_quiesced = FBE_TRUE;
    }
    else if (b_is_object_in_pass_thru == FBE_TRUE)
    {
        /* Else if we are in pass-thru mode and there are no outstanding I/Os
         * the object is quiesced.
         */
        if (outstanding_io == 0)
        {
            b_quiesced = FBE_TRUE;
        }
    }

    /* Trace some information.
     */
    fbe_base_object_trace((fbe_base_object_t*)base_config_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "base_config: quiesced: %d pass-thru: %d metadata: %d outstanding: %d terminator: %d iots: %d \n",
                          b_quiesced, b_is_object_in_pass_thru, b_metadata_req, outstanding_io, terminator_count, iots_count);

    return b_quiesced;
}
/******************************************************************************
 * end fbe_base_config_quiesce_ios()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_base_config_restart_quiesced_requests()
 ******************************************************************************
 * @brief
 *  This function restarts siots that were waiting.
 *
 * @param base_config_p - a pointer to the base config.
 *
 * @return fbe_status_t
 *
 * @author
 *  10/21/2009 - Created. Rob Foley
 ******************************************************************************/
static fbe_status_t
fbe_base_config_restart_quiesced_requests(fbe_base_config_t * const base_config_p)
{
    fbe_queue_head_t *      termination_queue_p = NULL;
    fbe_packet_t *          current_packet_p = NULL;
    fbe_raid_iots_t *       iots_p;
    fbe_queue_head_t        restart_queue;
    fbe_queue_element_t *   next_element_p = NULL;
    fbe_queue_element_t *   queue_element_p = NULL;
    fbe_u32_t               restart_siots_count = 0;
    fbe_u32_t               restart_iots_count = 0;
    fbe_u32_t               found_packet_count = 0;

    fbe_base_config_lock(base_config_p);

    /* Acquire lock before we look at the termination queue. */
    fbe_spinlock_lock(&base_config_p->base_object.terminator_queue_lock);

    /* Get the head of the termination queue after acquiring lock. */
    termination_queue_p = fbe_base_object_get_terminator_queue_head((fbe_base_object_t *) base_config_p);

    /* With lock held, mark the base config. as not quiesced. 
     * This allows siots to restart below. 
     */
    fbe_base_config_clear_clustered_flag(base_config_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_QUIESCED);

    /* Loop over the termination queue and generate a list of siots that need to be
     * restarted. 
     */
    fbe_queue_init(&restart_queue);

    queue_element_p = fbe_queue_front(termination_queue_p);
    while (queue_element_p != NULL)
    {
        fbe_payload_ex_t *sep_payload_p = NULL;
        current_packet_p = fbe_transport_queue_element_to_packet(queue_element_p);
        sep_payload_p = fbe_transport_get_payload_ex(current_packet_p);

        next_element_p = fbe_queue_next(termination_queue_p, &current_packet_p->queue_element);

        fbe_payload_ex_get_iots(sep_payload_p, &iots_p);

        fbe_raid_iots_lock(iots_p);
        if (fbe_raid_iots_is_flag_set(iots_p, FBE_RAID_IOTS_FLAG_RESTART_IOTS))
        {
            /* clear the flag so we don't get confused in the future */
            fbe_raid_iots_clear_flag(iots_p, FBE_RAID_IOTS_FLAG_RESTART_IOTS);
            if (fbe_queue_is_element_on_queue(&iots_p->common.wait_queue_element))
            {
                fbe_base_object_trace((fbe_base_object_t *) base_config_p, FBE_TRACE_LEVEL_CRITICAL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                      "base_cfg_restart_quiesced_req iots: %p is already on some queue\n", iots_p);
            }
            fbe_raid_common_set_flag(&iots_p->common, FBE_RAID_COMMON_FLAG_RESTART);
            /* While it is queued, it should be marked as not used so we do not try to do anything with it.
             */
            fbe_raid_iots_set_as_not_used(iots_p);
            /* we've already set up the state, just need to restart it */
            fbe_queue_push(&restart_queue, &iots_p->common.wait_queue_element);
            restart_iots_count ++;
        }
        else if (iots_p->status == FBE_RAID_IOTS_STATUS_NOT_STARTED_TO_LIBRARY) 
        {
            /* We have not been started to the library yet. */
            if (iots_p->callback != NULL) 
            { 
                fbe_base_object_trace((fbe_base_object_t *) base_config_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                      "base_cfg_restart_quiesced_req iots: %p callback is not null(0x%p)\n", iots_p, iots_p->callback);
            }
            if (iots_p->callback_context == NULL)
            {
                fbe_base_object_trace((fbe_base_object_t *) base_config_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                      "base_cfg_restart_quiesced_req iots: %p callback_context is null\n", iots_p);
            }
            if (iots_p->common.state == NULL) 
            {
                fbe_base_object_trace((fbe_base_object_t *) base_config_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                      "base_cfg_restart_quiesced_req iots: %p state is null\n", iots_p);
            }
            /* While it is queued, it should be marked as not used so we do not try to do anything with it.
             */
            fbe_raid_iots_set_as_not_used(iots_p);
            if (fbe_queue_is_element_on_queue(&iots_p->common.wait_queue_element))
            {
                fbe_base_object_trace((fbe_base_object_t *) base_config_p, FBE_TRACE_LEVEL_CRITICAL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                      "base_cfg_restart_quiesced_req iots: %p is already on some queue\n", iots_p);
            }
            fbe_queue_push(&restart_queue, &iots_p->common.wait_queue_element);
        }
        else if (iots_p->status == FBE_RAID_IOTS_STATUS_NOT_USED)
        {
            /* Do not touch it.
             */
        }
        else if (iots_p->status == FBE_RAID_IOTS_STATUS_WAITING_FOR_QUIESCE)
        {
            fbe_base_object_trace((fbe_base_object_t *) base_config_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                  "base_cfg_restart_quiesced_req iots: %p should be started already\n", iots_p);
        }
        else
        {
            /* Iots and or siots need to be restarted. */
            fbe_raid_iots_get_quiesced_ts_to_restart(iots_p, &restart_queue);
        }

        found_packet_count++;
        fbe_raid_iots_unlock(iots_p);

        queue_element_p = next_element_p;
    }
    fbe_spinlock_unlock(&base_config_p->base_object.terminator_queue_lock);
    fbe_base_config_unlock(base_config_p);

    /* Restart all the items we found. */
    restart_siots_count = fbe_raid_restart_common_queue(&restart_queue);

    /* Trace out the number of siots we kicked off. */
    if (restart_siots_count || restart_iots_count)
    {
        fbe_base_object_trace((fbe_base_object_t *) base_config_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                              "base_cfg_restart_quiesced_req: %d packets found, %d siots and %d iots restarted\n", 
                              found_packet_count, restart_siots_count, restart_iots_count);
    }
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_base_config_restart_quiesced_requests()
 ******************************************************************************/
/*!**************************************************************
 * fbe_base_config_should_quiesce_drain()
 ****************************************************************
 * @brief
 *  Determine if we should drain out all I/Os or hold them in place.
 *
 * @param base_config_p - Current object.          
 *
 * @return TRUE means to drain out any I/Os in progress using bts.
 *        FALSE means to hold I/Os in place even at raid library.
 *
 *
 * @author
 *  10/24/2013 - Created. Rob Foley
 *
 ****************************************************************/

fbe_bool_t fbe_base_config_should_quiesce_drain(fbe_base_config_t * base_config_p)
{
    if (fbe_base_config_is_clustered_flag_set(base_config_p, 
                                              FBE_BASE_CONFIG_METADATA_MEMORY_FLAGS_QUIESCE_HOLD) &&
        (!fbe_base_config_is_peer_present(base_config_p) ||
         fbe_base_config_is_peer_clustered_flag_set(base_config_p, 
                                                    FBE_BASE_CONFIG_METADATA_MEMORY_FLAGS_QUIESCE_HOLD)) )
    {
        return FBE_TRUE;
    } else {
        return FBE_FALSE;
    }
}
/******************************************
 * end fbe_base_config_should_quiesce_drain()
 ******************************************/
/*!****************************************************************************
 * fbe_base_config_quiesce_io_requests()
 ******************************************************************************
 * @brief
 *  This function is used to quiesce the i/o request which is in progress and
 *  held the block transport server to queue the incoming requests.
 *
 * @param base_config_p - a pointer to the base config.
 * @param packet_p      - a pointer to the packet.
 * @param b_is_object_in_pass_thru - FBE_FALSE (typical) object is not in `pass-thru' mode
 *          FBE_TRUE - Object is in pass-thru, we must stop new I/O
 * @param is_io_request_quiesced_b - pointer to boolean indicating whether I/O quiesced or not
 *
 * @return fbe_status_t
 *
 * @author
 *  10/21/2009 - Created. Rob Foley
 ******************************************************************************/
fbe_status_t
fbe_base_config_quiesce_io_requests(fbe_base_config_t * base_config_p,
                                    fbe_packet_t *packet_p,
                                    fbe_bool_t b_is_object_in_pass_thru,
                                    fbe_bool_t * is_io_request_quiesced_b)

{
    fbe_bool_t                      b_metadata_quiesced;
    fbe_bool_t                      md_memory_updated_b;
    fbe_bool_t                      io_quiesced_b;
    fbe_base_config_metadata_memory_t * metadata_memory_p = NULL;
    fbe_base_config_metadata_memory_t * metadata_memory_peer_p = NULL;
    fbe_bool_t b_abort_set;
    fbe_bool_t b_should_drain;

    /* Initialize the is_quiesced_b as false before we start quiescing i/o operation. */
    *is_io_request_quiesced_b = FBE_FALSE;

    /* Make sure metadata memory is updated for quiescing I/O; used as part of peer-to-peer communication */
    fbe_base_config_quiesce_io_start_is_metadata_memory_updated(base_config_p, &md_memory_updated_b);
    if (!md_memory_updated_b)
    {
        /* Metadata memory not updated; will check again next monitor cycle */
        return FBE_STATUS_OK;
    }
    /* Get a pointer to metadata memory */
    fbe_metadata_element_get_metadata_memory(&base_config_p->metadata_element, (void **)&metadata_memory_p);
    fbe_metadata_element_get_peer_metadata_memory(&base_config_p->metadata_element, (void **)&metadata_memory_peer_p);

    fbe_base_object_trace((fbe_base_object_t *)base_config_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "qsce_io: entry mde_a: 0x%x bts_a: 0x%llx fl: 0x%llx/0x%llx\n", 
                          base_config_p->metadata_element.attributes,
                          (unsigned long long)base_config_p->block_transport_server.attributes, 
                          (unsigned long long)((metadata_memory_p) ? metadata_memory_p->flags : 0),
                          (unsigned long long)((metadata_memory_peer_p) ? metadata_memory_peer_p->flags : 0));
    /* If either local or peer decided not to drain, clear the quiesce hold bit.
     */
    b_should_drain = fbe_base_config_should_quiesce_drain(base_config_p);
    b_abort_set = fbe_metadata_is_abort_set(&base_config_p->metadata_element);
    if (b_should_drain && b_abort_set){
        b_should_drain = FBE_FALSE;
        fbe_base_object_trace((fbe_base_object_t *)base_config_p,
                              FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                              "base_config: 0x%x do not drain attr: 0x%llx cl_f: 0x%llx/0x%llx\n",
                              base_config_p->metadata_element.attributes,
                              (unsigned long long)base_config_p->block_transport_server.attributes, 
                              (unsigned long long)((metadata_memory_p) ? metadata_memory_p->flags : 0),
                              (unsigned long long)((metadata_memory_peer_p) ? metadata_memory_peer_p->flags : 0));
        fbe_base_config_clear_clustered_flag(base_config_p, 
                                             FBE_BASE_CONFIG_METADATA_MEMORY_FLAGS_QUIESCE_HOLD);
    }
    if (b_should_drain){
        /* This flavor of quiesce will drain the block transport server first, then drain other operations.
         */
        fbe_u32_t outstanding_io;
        fbe_u32_t quiesced_io;
        if (!fbe_block_transport_server_is_hold_set(&base_config_p->block_transport_server)) {
            /* Mark the server as held to be sure that new I/O gets queued.
             */
            fbe_block_transport_server_hold(&base_config_p->block_transport_server);
        }

        fbe_block_transport_server_get_outstanding_io_count(&base_config_p->block_transport_server, &outstanding_io);
        io_quiesced_b = (outstanding_io == 0);
        if (!io_quiesced_b) {
            fbe_base_config_get_quiesced_io_count(base_config_p, &quiesced_io);
            /* If we found quiesced I/Os on the terminator queue we need to allow the normal quiesce to proceed.
             */
            if (quiesced_io != 0) {
                fbe_base_object_trace((fbe_base_object_t *)base_config_p,
                                      FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                      "base_config: %d quiesced_io found.  clear quiesce hold 0x%llx/0x%llx\n",
                                      quiesced_io, 
                                      (unsigned long long)((metadata_memory_p) ? metadata_memory_p->flags : 0),
                                      (unsigned long long)((metadata_memory_peer_p) ? metadata_memory_peer_p->flags : 0));
                fbe_base_config_clear_clustered_flag(base_config_p, 
                                                     FBE_BASE_CONFIG_METADATA_MEMORY_FLAGS_QUIESCE_HOLD);
            }
            else {
                fbe_base_object_trace((fbe_base_object_t *)base_config_p,
                                      FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                      "base_config: waiting for quiesce. %d ios found. 0x%llx/0x%llx\n",
                                      outstanding_io,  
                                      (unsigned long long)((metadata_memory_p) ? metadata_memory_p->flags : 0),
                                      (unsigned long long)((metadata_memory_peer_p) ? metadata_memory_peer_p->flags : 0));
                return FBE_STATUS_OK;
            }
        }
        fbe_base_object_trace((fbe_base_object_t *)base_config_p,
                              FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                              "base_config: outstanding io count quiesced 0x%llx/0x%llx\n",
                              (unsigned long long)((metadata_memory_p) ? metadata_memory_p->flags : 0),
                              (unsigned long long)((metadata_memory_peer_p) ? metadata_memory_peer_p->flags : 0));
        /* No need to drain metadata requests since we have no I/Os in flight.
         */
        b_metadata_quiesced = FBE_TRUE;
    }
    else {
        /* Drain metadata requests. Metadata requests are not failed immediately but can remain
         * quiesced in raid library. So we will check if metadata requests are quiesced in
         * fbe_base_config_quiesce_ios()  
         */
        fbe_metadata_element_is_quiesced(&base_config_p->metadata_element, &b_metadata_quiesced);
    }

    fbe_base_object_trace((fbe_base_object_t *)base_config_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                          "base config quiesce hold bts attrib: 0x%llx local: 0x%llx peer: 0x%llx\n", 
                          (unsigned long long)base_config_p->block_transport_server.attributes,
                          (unsigned long long)((metadata_memory_p) ? metadata_memory_p->flags : 0),
                          (unsigned long long)((metadata_memory_peer_p) ? metadata_memory_peer_p->flags : 0));

    /* Mark the base config. quiesced and check to see if we are quiesced.
     */
    fbe_base_config_lock(base_config_p);
    if (!fbe_base_config_is_clustered_flag_set(base_config_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_QUIESCING))
    {
        fbe_base_object_trace((fbe_base_object_t *)base_config_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                              "%s obj 0x%x, marking FBE_BASE_CONFIG_FLAG_QUIESCING\n", __FUNCTION__, base_config_p->base_object.object_id);
        fbe_base_config_set_clustered_flag(base_config_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_QUIESCING);
    }
    if (!b_should_drain) {
        /* If not already done, set the `hold' flag to stop processing new I/Os.
         */
        if (!fbe_block_transport_server_is_hold_set(&base_config_p->block_transport_server))
        {
            /* Mark the server as held to be sure that new I/O gets queued.
             */
            fbe_block_transport_server_hold(&base_config_p->block_transport_server);
        }
    
        /* Now check if there are still outstanding I/Os
         */
        io_quiesced_b = fbe_base_config_quiesce_ios(base_config_p, b_is_object_in_pass_thru);
    }
    
    if (io_quiesced_b)
    {
        /* If peer clears hold then we clear it also.
         * On unquiesce this flag tells us that we are coming out of drain.
         */
        if (fbe_base_config_is_clustered_flag_set(base_config_p, 
                                                  FBE_BASE_CONFIG_METADATA_MEMORY_FLAGS_QUIESCE_HOLD) &&
            fbe_base_config_is_peer_present(base_config_p) && 
            !fbe_base_config_is_peer_clustered_flag_set(base_config_p, 
                                                       FBE_BASE_CONFIG_METADATA_MEMORY_FLAGS_QUIESCE_HOLD) ) {
            fbe_base_config_clear_clustered_flag(base_config_p, FBE_BASE_CONFIG_METADATA_MEMORY_FLAGS_QUIESCE_HOLD);
            fbe_base_object_trace((fbe_base_object_t*)base_config_p, 
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "base config quiesce, peer cleared hold 0x%llx/0x%llx\n",
                                  (unsigned long long)((metadata_memory_p) ? metadata_memory_p->flags : 0),
                                  (unsigned long long)((metadata_memory_peer_p) ? metadata_memory_peer_p->flags : 0));
        }

        /* Make sure metadata memory is updated for completing quiescing I/O; 
         * used as part of peer-to-peer communication 
         */
        fbe_base_config_quiesce_io_complete_is_metadata_memory_updated(base_config_p, &md_memory_updated_b);
        if (!md_memory_updated_b)
        {
            /* Metadata memory not updated; will check again next monitor cycle */
            fbe_base_config_unlock(base_config_p);
            return FBE_STATUS_OK;
        }

        fbe_base_config_clear_clustered_flag(base_config_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_QUIESCING);
        fbe_base_config_set_clustered_flag(base_config_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_QUIESCED);
        fbe_base_config_unlock(base_config_p);

        /* Set output parameter to true; quiesce I/O is complete */
        *is_io_request_quiesced_b = FBE_TRUE;

        fbe_base_object_trace((fbe_base_object_t *)base_config_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                              "%s obj 0x%x, base config quiesced.\n", __FUNCTION__, base_config_p->base_object.object_id);
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t*)base_config_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                              "%s obj 0x%x, Object not quiesced.  bts attributes: 0x%llx\n", __FUNCTION__,
                              base_config_p->base_object.object_id, 
                              (unsigned long long)base_config_p->block_transport_server.attributes);
        fbe_base_config_unlock(base_config_p);
    }

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_base_config_quiesce_io_requests()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_base_config_quiesce_io_start_is_metadata_memory_updated()
 ******************************************************************************
 * @brief
 *  This function is used to ensure the specified object's metadata memory
 *  is updated to signify I/O to that object can be quiesced on this SP. The peer SP, 
 *  if available, will be consulted as well. Both SPs must agree that quiescing can proceed.
 *
 * @param base_config_p - a pointer to the base config
 * @param out_md_mem_updated_p - pointer to boolean indicating if metadata memory updated or not
 *
 * @return fbe_status_t
 *
 * @author
 *  09/2010 - Created. Susan Rundbaken (rundbs)
 ******************************************************************************/
static fbe_status_t
fbe_base_config_quiesce_io_start_is_metadata_memory_updated(fbe_base_config_t * base_config_p, 
                                                            fbe_bool_t * out_md_mem_updated_p)
{
    fbe_base_config_metadata_memory_flags_t    local_quiesce_state;
    fbe_base_config_metadata_memory_flags_t    peer_quiesce_state;
    fbe_lifecycle_state_t	                   peer_state = FBE_LIFECYCLE_STATE_INVALID;
    fbe_bool_t                                 peer_alive_b;
    fbe_bool_t                                 quiesce_ready_b;


    /* Initialize output parameter */
    *out_md_mem_updated_p = FBE_FALSE;

    /* See if peer is alive and well */
    fbe_base_config_metadata_get_peer_lifecycle_state(base_config_p, &peer_state);

    peer_alive_b = fbe_base_config_has_peer_joined(base_config_p);

    /* Get the local and peer quiesce state */
    fbe_base_config_metadata_get_quiesce_state_local(base_config_p, &local_quiesce_state);
    fbe_base_config_metadata_get_quiesce_state_peer(base_config_p, &peer_quiesce_state);

    /* Determine if quiescing I/O can proceed based on the quiesce state */
    fbe_base_config_quiesce_start_state_check(base_config_p, peer_alive_b, local_quiesce_state, peer_quiesce_state, 
                                              &quiesce_ready_b);

    /* Set the output parameter and return */
    *out_md_mem_updated_p = quiesce_ready_b;

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_base_config_quiesce_io_start_is_metadata_memory_updated()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_base_config_quiesce_io_complete_is_metadata_memory_updated()
 ******************************************************************************
 * @brief
 *  This function is used to ensure the specified object's metadata memory
 *  is updated to signify the SP can complete quiescing I/O on the object. The peer SP, if 
 *  available, will be consulted as well. Both SPs must agree that quiescing can complete.
 *
 * @param base_config_p - a pointer to the base config
 * @param out_md_mem_updated_p - pointer to boolean indicating if metadata memory updated or not
 *
 * @return fbe_status_t
 *
 * @author
 *  09/2010 - Created. Susan Rundbaken (rundbs)
 ******************************************************************************/
static fbe_status_t
fbe_base_config_quiesce_io_complete_is_metadata_memory_updated(fbe_base_config_t * base_config_p, 
                                                               fbe_bool_t * out_md_mem_updated_p)
{
    fbe_base_config_metadata_memory_flags_t     local_quiesce_state;
    fbe_base_config_metadata_memory_flags_t     peer_quiesce_state;
    fbe_bool_t                                  peer_alive_b;
    fbe_bool_t                                  quiesce_complete_b;


    /* Initialize output parameter */
    *out_md_mem_updated_p = FBE_FALSE;

    /* See if peer is alive and well */
    peer_alive_b = fbe_base_config_has_peer_joined(base_config_p);

    /* Get the local and peer quiesce state */
    fbe_base_config_metadata_get_quiesce_state_local(base_config_p, &local_quiesce_state);
    fbe_base_config_metadata_get_quiesce_state_peer(base_config_p, &peer_quiesce_state);

    /* Determine if quiescing I/O can complete based on the quiesce state */
    fbe_base_config_quiesce_complete_state_check(base_config_p, peer_alive_b, local_quiesce_state, peer_quiesce_state, 
                                                 &quiesce_complete_b);

    /* Set the output parameter and return */
    *out_md_mem_updated_p = quiesce_complete_b;

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_base_config_quiesce_io_complete_is_metadata_memory_updated()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_base_config_quiesce_start_state_check()
 ******************************************************************************
 * @brief
 *  This function is used to determine if quiescing I/O can proceed on the specified object 
 *  based on the quiesce state. If the "quiesce ready" state has not been set locally, it will be updated 
 *  accordingly. If the peer is available, it will be consulted as well. Both SPs must agree that 
 *  quiescing I/O can continue based on their state.
 *
 * @param base_config_p - a pointer to the base config
 * @param peer_alive_b - boolean indicating if peer is alive or not
 * @param local_quiesce_state - local quiesce state
 * @param peer_quiesce_state - peer quiesce state
 * @param quiesce_ready_b - boolean used to determine if quiesce can proceed or not
 *
 * @return fbe_status_t
 *
 * @author
 *  09/2010 - Created. Susan Rundbaken (rundbs)
 ******************************************************************************/
static fbe_status_t
fbe_base_config_quiesce_start_state_check(fbe_base_config_t * base_config_p, 
                                          fbe_bool_t peer_alive_b,
                                          fbe_base_config_metadata_memory_flags_t local_quiesce_state,
                                          fbe_base_config_metadata_memory_flags_t peer_quiesce_state,
                                          fbe_bool_t * quiesce_ready_b)
{
    /* Initialize output parameter */
    *quiesce_ready_b = FBE_FALSE;

    /* Determine how to proceed based on the quiesce state */
    switch (local_quiesce_state)
    {
    
    case FBE_BASE_CONFIG_METADATA_MEMORY_QUIESCE_NOT_STARTED:
    case FBE_BASE_CONFIG_METADATA_MEMORY_UNQUIESCE_READY:
        /* moved this from base_config_event handler to here to avoid lifecycle race with quiesce condition. 
         * Once we started quiesce, we need to unquiesce.  It doesn't hurt if unquiesce is already set.
         */
        if (!fbe_base_config_is_flag_set(base_config_p, FBE_BASE_CONFIG_FLAG_USER_INIT_QUIESCE)){
		    fbe_lifecycle_set_cond(&fbe_base_config_lifecycle_const, (fbe_base_object_t*)base_config_p,
                                    FBE_BASE_CONFIG_LIFECYCLE_COND_UNQUIESCE);
        }
        /* Not quiescing I/O on object on this SP; set local state to "quiesce ready" noting that quiesce always takes 
         * precedence over unquiesce 
         */
        fbe_base_config_metadata_set_quiesce_state_local_and_update_peer(base_config_p, FBE_BASE_CONFIG_METADATA_MEMORY_QUIESCE_READY);

        fbe_base_object_trace((fbe_base_object_t*)base_config_p,
                      FBE_TRACE_LEVEL_DEBUG_HIGH,
                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                      "%s set 0x%x local_quiesce_state 0x%llx; peer_quiesce_state 0x%llx\n", 
                      __FUNCTION__,
		      base_config_p->base_object.object_id,
		      (unsigned long long)local_quiesce_state,
		      (unsigned long long)peer_quiesce_state);

        /* Wait for "quiesce ready" to be set, so just return; will check again next monitor cycle */
        break;
       
    case FBE_BASE_CONFIG_METADATA_MEMORY_QUIESCE_READY:
    case FBE_BASE_CONFIG_METADATA_MEMORY_QUIESCE_COMPLETE:
        
        /* Quiescing I/O can proceed on object on this SP */

        /* If peer is available, make sure it agrees that quiescing can proceed */
        if (peer_alive_b)
        {
            if ((peer_quiesce_state != FBE_BASE_CONFIG_METADATA_MEMORY_QUIESCE_READY)&&
                (peer_quiesce_state != FBE_BASE_CONFIG_METADATA_MEMORY_QUIESCE_COMPLETE))
            {
                if (peer_quiesce_state != FBE_BASE_CONFIG_METADATA_MEMORY_QUIESCE_NOT_STARTED)
                {
                    /* peer is in wrong state, let's restart the state machine */
                    fbe_base_config_metadata_set_quiesce_state_local_and_update_peer(base_config_p, FBE_BASE_CONFIG_METADATA_MEMORY_QUIESCE_NOT_STARTED);
                }
                /* The peer does not agree that quiescing can proceed, so return; will try again next monitor cycle */
                fbe_base_object_trace((fbe_base_object_t*)base_config_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                              "quiesce_start_state_check waiting for peer, local_quiesce_state: 0x%llx; peer_quiesce_state: 0x%llx\n", 
			      (unsigned long long)local_quiesce_state,
			      (unsigned long long)peer_quiesce_state);

                return FBE_STATUS_OK;
            }
        }

        /* Quiescing can proceed on object on this SP; set the output parameter to TRUE and return */
        fbe_base_object_trace((fbe_base_object_t*)base_config_p,
                      FBE_TRACE_LEVEL_DEBUG_HIGH,
                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                      "%s quiesce start can proceed 0x%x; local_quiesce_state 0x%llx; peer_quiesce_state 0x%llx; peer_alive_b 0x%x\n", 
                      __FUNCTION__,
		      base_config_p->base_object.object_id,
		      (unsigned long long)local_quiesce_state,
		      (unsigned long long)peer_quiesce_state, peer_alive_b);

        *quiesce_ready_b = FBE_TRUE;
        break;
    
    default:

        /* Panic in checked build; invalid local quiesce state */
        fbe_base_object_trace((fbe_base_object_t*)base_config_p,
                      FBE_TRACE_LEVEL_ERROR,
                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                      "%s invalid local quiesce state 0x%x!!  local_quiesce_state: 0x%llx; peer_quiesce_state: 0x%llx\n", 
                      __FUNCTION__,
		      base_config_p->base_object.object_id,
		      (unsigned long long)local_quiesce_state,
		      (unsigned long long)peer_quiesce_state);

        break;
    }

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_base_config_quiesce_start_state_check()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_base_config_quiesce_complete_state_check()
 ******************************************************************************
 * @brief
 *  This function is used to determine if quiescing I/O can complete on the specified object
 *  based on the quiesce state. If the "quiesce complete" state has not been set locally, it
 *  will be updated accordingly. If the peer is available, it will be consulted as well. Both SPs must
 *  agree that quiescing I/O can complete based on their state.
 *
 * @param base_config_p - a pointer to the base config
 * @param peer_alive_b - boolean indicating if peer is alive or not
 * @param local_quiesce_state - local quiesce state
 * @param peer_quiesce_state - peer quiesce state
 * @param quiesce_complete_b - boolean used to determine if quiesce can complete or not
 *
 * @return fbe_status_t
 *
 * @author
 *  09/2010 - Created. Susan Rundbaken (rundbs)
 ******************************************************************************/
static fbe_status_t
fbe_base_config_quiesce_complete_state_check(fbe_base_config_t * base_config_p, 
                                             fbe_bool_t peer_alive_b,
                                             fbe_base_config_metadata_memory_flags_t local_quiesce_state,
                                             fbe_base_config_metadata_memory_flags_t peer_quiesce_state,
                                             fbe_bool_t * quiesce_complete_b)
{
    /* Initialize output parameter */
    *quiesce_complete_b = FBE_FALSE;


    /* Determine how to proceed based on the quiesce state */
    switch (local_quiesce_state)
    {
       
    case FBE_BASE_CONFIG_METADATA_MEMORY_QUIESCE_READY:
    
        /* Quiescing I/O complete on object on this SP; set local state to "quiesce complete" */
        fbe_base_config_metadata_set_quiesce_state_local_and_update_peer(base_config_p, FBE_BASE_CONFIG_METADATA_MEMORY_QUIESCE_COMPLETE);

        fbe_base_object_trace((fbe_base_object_t*)base_config_p,
                      FBE_TRACE_LEVEL_DEBUG_HIGH,
                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                      "bcfg_qsce_complete_chk set 0x%x quiesce_state: local 0x%llx, peer 0x%llx\n", 
                      base_config_p->base_object.object_id,
		      (unsigned long long)local_quiesce_state,
		      (unsigned long long)peer_quiesce_state);

        /* Complete the quiesce if the peer is not ready so we don't get stuck waiting
         */
        if (!fbe_base_config_has_peer_joined(base_config_p))
        {
            *quiesce_complete_b = FBE_TRUE;
        }
        
        break;

    case FBE_BASE_CONFIG_METADATA_MEMORY_QUIESCE_COMPLETE:
    
        /* Quiescing I/O ready to complete on object on this SP; if peer is available, make sure it is ready to complete */
        if (peer_alive_b)
        {
            if (peer_quiesce_state != FBE_BASE_CONFIG_METADATA_MEMORY_QUIESCE_COMPLETE &&
                peer_quiesce_state != FBE_BASE_CONFIG_METADATA_MEMORY_UNQUIESCE_READY)
            {
                /* The peer does not agree that quiescing can complete, so return; will try again next monitor cycle */
                fbe_base_object_trace((fbe_base_object_t*)base_config_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                              "bcfg_qsce_complete_chk wait peer 0x%x: quiesce_state: local 0x%llx, peer 0x%llx\n", 
			      base_config_p->base_object.object_id,
			      (unsigned long long)local_quiesce_state,
			      (unsigned long long)peer_quiesce_state);

                return FBE_STATUS_OK;
            }
        }

        /* Quiescing I/O can complete on object on this SP; set the output parameter to TRUE and return */
        fbe_base_object_trace((fbe_base_object_t*)base_config_p,
                      FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                      "bcfg_qsce_complete_chk: can complete 0x%x; qsce local 0x%llx, peer 0x%llx; peer_alive 0x%x\n", 
		      base_config_p->base_object.object_id,
		      (unsigned long long)local_quiesce_state,
		      (unsigned long long)peer_quiesce_state,
		      peer_alive_b);

        *quiesce_complete_b = FBE_TRUE;
        break;
    
    case FBE_BASE_CONFIG_METADATA_MEMORY_QUIESCE_NOT_STARTED:
    case FBE_BASE_CONFIG_METADATA_MEMORY_UNQUIESCE_READY:

        /* Panic in checked build; local state should not be "quiesce not started" or "unquiesce ready" since we are 
         * checking if we can complete quiescing
         */
        fbe_base_object_trace((fbe_base_object_t*)base_config_p,
                      FBE_TRACE_LEVEL_ERROR,
                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                      "bcfg_qsce_complete_chk not finished; no unqsc! 0x%x state:local 0x%llx, peer 0x%llx\n", 
		      base_config_p->base_object.object_id,
		      (unsigned long long)local_quiesce_state,
		      (unsigned long long)peer_quiesce_state);

        break;

    default:

        /* Panic in checked build; invalid state */
        fbe_base_object_trace((fbe_base_object_t*)base_config_p,
                      FBE_TRACE_LEVEL_ERROR,
                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                      "bcfg_qsce_complete_chk invalid local state! 0x%x state: local 0x%llx, peer 0x%llx\n", 
		      base_config_p->base_object.object_id,
		      (unsigned long long)local_quiesce_state,
		      (unsigned long long)peer_quiesce_state);

        break;
    }

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_base_config_quiesce_complete_state_check()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_base_config_unquiesce_io_requests()
 ******************************************************************************
 * @brief
 *  This function is used to restart the already quiesce requests.
 *
 * @param base_config_p     - a pointer to the base config.
 * @param packet_p          - a pointer to the packet.
 * @param is_io_request_unquiesce_ready_b - pointer to boolean indicating whether unquiesce can proceed or not
 *                                          used as part of peer-to-peer communication
 *
 * @return fbe_status_t
 *
 * @author
 *  10/21/2009 - Created. Rob Foley
 ******************************************************************************/
fbe_status_t
fbe_base_config_unquiesce_io_requests(fbe_base_config_t * base_config_p,
                                      fbe_packet_t * packet_p,
                                      fbe_bool_t * is_io_request_unquiesce_ready_b)
{
    fbe_status_t                      status;
    fbe_bool_t                        md_memory_updated_b;
    fbe_scheduler_hook_status_t hook_status = FBE_SCHED_STATUS_OK;

    /* Initialize the is_io_request_unquiesce_ready_b as false before we start unquiescing i/o operation. */
    *is_io_request_unquiesce_ready_b = FBE_FALSE;

    /* Make sure metadata memory is updated for unquiescing I/O; used as part of peer-to-peer communication */
    fbe_base_config_unquiesce_io_is_metadata_memory_updated(base_config_p, &md_memory_updated_b);
    if (!md_memory_updated_b)
    {
        /* Metadata memory not updated; will check again next monitor cycle */
        return FBE_STATUS_OK;
    }

    fbe_base_config_check_hook(base_config_p,
                              SCHEDULER_MONITOR_STATE_BASE_CONFIG_UNQUIESCE,
                              FBE_BASE_CONFIG_SUBSTATE_UNQUIESCE_METADATA_MEMORY_UPDATED,
                              0, &hook_status);
    if (hook_status == FBE_SCHED_STATUS_DONE)
    {
        return FBE_STATUS_OK;
    }


	/* We have to set memory_request_priority to zero */
	base_config_p->resource_priority = 0;

    status = fbe_metadata_element_restart_io(&base_config_p->metadata_element);

    fbe_base_object_trace((fbe_base_object_t*) base_config_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry bts attributes: 0x%llx; MDAttr: 0x%x obj 0x%x\n", 
                          __FUNCTION__,
                          (unsigned long long)base_config_p->block_transport_server.attributes,
                          base_config_p->metadata_element.attributes,
                          base_config_p->base_object.object_id);    

    /* Mark the base config. quiesced and check to see if we are quiesced.
     */
    fbe_base_config_restart_quiesced_requests(base_config_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_config_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s failed resuming metadata element I/O, status: 0x%x\n",
                              __FUNCTION__, status);
    }

    /* Resume the server and then kick off any I/O that is pending.
     */
    status = fbe_block_transport_server_process_io_from_queue_sync(&base_config_p->block_transport_server);

    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_config_p, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s failed resuming I/O, status: 0x%x\n",
                              __FUNCTION__, status);
    }

    fbe_base_object_trace((fbe_base_object_t*)base_config_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_EXIT,
                          "%s obj 0x%x exit bts attributes: 0x%llx\n", __FUNCTION__,
                          base_config_p->base_object.object_id,
                          (unsigned long long)base_config_p->block_transport_server.attributes);

    /* Set the object metadata memory quiesce state set to "quiesce not started"; 
     * the quiesce/unquiesce process is complete 
     */
    fbe_base_config_metadata_set_quiesce_state_local_and_update_peer(base_config_p, FBE_BASE_CONFIG_METADATA_MEMORY_QUIESCE_NOT_STARTED);

    fbe_base_object_trace((fbe_base_object_t*)base_config_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_EXIT,
                          "%s obj 0x%x, set local quiesce state: 0x%x\n", __FUNCTION__,
                          base_config_p->base_object.object_id,
                          FBE_BASE_CONFIG_METADATA_MEMORY_QUIESCE_NOT_STARTED);

    /* Set output parameter to TRUE */
    *is_io_request_unquiesce_ready_b = FBE_TRUE;

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_base_config_unquiesce_io_requests()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_base_config_unquiesce_io_is_metadata_memory_updated()
 ******************************************************************************
 * @brief
 *  This function is used to ensure the specified object's metadata memory
 *  is updated to signify the SP can unquiesce I/O on that object. The peer SP, if 
 *  available, will be consulted as well. Both SPs must agree that unquiescing
 *  can proceed.
 *
 * @param base_config_p - a pointer to the base config
 * @param out_md_mem_updated_p - pointer to boolean indicating if metadata memory updated or not
 *
 * @return fbe_status_t
 *
 * @author
 *  09/2010 - Created. Susan Rundbaken (rundbs)
 ******************************************************************************/
static fbe_status_t
fbe_base_config_unquiesce_io_is_metadata_memory_updated(fbe_base_config_t * base_config_p, fbe_bool_t * out_md_mem_updated_p)
{
    fbe_base_config_metadata_memory_flags_t    local_quiesce_state;
    fbe_base_config_metadata_memory_flags_t    peer_quiesce_state;
    fbe_bool_t                                 peer_alive_b = FBE_FALSE;
    fbe_bool_t                                 unquiesce_ready_b;
    fbe_bool_t                                 peer_quiescing;


    /* Initialize output parameter */
    *out_md_mem_updated_p = FBE_FALSE;

    /* See if peer is alive and well */
    if (fbe_base_config_has_peer_joined(base_config_p))
    {
        peer_alive_b = FBE_TRUE;
    }
    /* Get the local and peer metadata memory quiesce state */
    fbe_base_config_metadata_get_quiesce_state_local(base_config_p, &local_quiesce_state);
    fbe_base_config_metadata_get_quiesce_state_peer(base_config_p, &peer_quiesce_state);

    /* we need to make sure peer is quiesced before we move forward */
    peer_quiescing = fbe_base_config_is_peer_clustered_flag_set(base_config_p, 
                                                               FBE_BASE_CONFIG_CLUSTERED_FLAG_QUIESCING);
    if (peer_alive_b && peer_quiescing)
    {
        fbe_base_object_trace((fbe_base_object_t*)base_config_p,
                      FBE_TRACE_LEVEL_INFO,
                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                      "base_config_unquiesce_io wait for peer quiesced. local_quiesce_state 0x%llx; peer_quiesce_state 0x%llx\n", 
                      (unsigned long long)local_quiesce_state,
		      (unsigned long long)peer_quiesce_state);
    }
    else
    {
        fbe_base_config_metadata_memory_t * metadata_memory_p = NULL;
        fbe_base_config_metadata_memory_t * metadata_memory_peer_p = NULL;

        /* If the hold bit is set, clear it now. 
         * Note this must be after the is metadata memory check above, since that confirms that the 
         * peer has quiesced. 
         */
        fbe_metadata_element_get_metadata_memory(&base_config_p->metadata_element, (void **)&metadata_memory_p);
        fbe_metadata_element_get_peer_metadata_memory(&base_config_p->metadata_element, (void **)&metadata_memory_peer_p);
        if (fbe_base_config_is_clustered_flag_set(base_config_p, 
                                                  FBE_BASE_CONFIG_METADATA_MEMORY_FLAGS_QUIESCE_HOLD)) {
            fbe_base_config_clear_clustered_flag(base_config_p, FBE_BASE_CONFIG_METADATA_MEMORY_FLAGS_QUIESCE_HOLD);
            fbe_base_object_trace((fbe_base_object_t *)base_config_p, FBE_TRACE_LEVEL_INFO,FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "bc unquiesce clear quiesce hold 0x%llx/0x%llx\n",
                                  (unsigned long long)((metadata_memory_p) ? metadata_memory_p->flags : 0),
                                  (unsigned long long)((metadata_memory_peer_p) ? metadata_memory_peer_p->flags : 0));
        }
        /* Determine if unquiescing I/O can proceed based on the quiesce state */
        fbe_base_config_unquiesce_state_check(base_config_p, peer_alive_b, local_quiesce_state, peer_quiesce_state, 
                                              &unquiesce_ready_b);

        /* Set output parameter */
        *out_md_mem_updated_p = unquiesce_ready_b;
    }

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_base_config_unquiesce_io_is_metadata_memory_updated()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_base_config_unquiesce_state_check()
 ******************************************************************************
 * @brief
 *  This function is used to determine if unquiescing I/O can proceed on the specified object
 *  based on the quiesce state. If the "unquiesce ready" state has not been set locally, it
 *  will be updated accordingly. If the peer is available and participating in the
 *  quiesce/unquiesce process, it will be consulted as well.  Both SPs must
 *  agree that unquiescing I/O can proceed based on their state.
 *
 * @param base_config_p - a pointer to the base config
 * @param peer_alive_b - boolean indicating if peer is alive or not
 * @param local_quiesce_state - local quiesce state
 * @param peer_quiesce_state - peer quiesce state
 * @param unquiesce_ready_b - boolean used to determine if unquiesce can proceed or not
 *
 * @return fbe_status_t
 *
 * @author
 *  09/2010 - Created. Susan Rundbaken (rundbs)
 ******************************************************************************/
static fbe_status_t
fbe_base_config_unquiesce_state_check(fbe_base_config_t * base_config_p, 
                                      fbe_bool_t peer_alive_b,
                                      fbe_base_config_metadata_memory_flags_t local_quiesce_state,
                                      fbe_base_config_metadata_memory_flags_t peer_quiesce_state,
                                      fbe_bool_t * unquiesce_ready_b)
{
    /* Initialize output parameter */
    *unquiesce_ready_b = FBE_FALSE;


    /* Determine how to proceed based on the quiesce state */
    switch (local_quiesce_state)
    {
    
    case FBE_BASE_CONFIG_METADATA_MEMORY_QUIESCE_COMPLETE:
        if (peer_alive_b && 
            (peer_quiesce_state != FBE_BASE_CONFIG_METADATA_MEMORY_QUIESCE_COMPLETE) &&
            (peer_quiesce_state != FBE_BASE_CONFIG_METADATA_MEMORY_UNQUIESCE_READY))
        {
            /* The peer is participating in the quiesce/unquiesce process, but is not at the same state, so return.
             * Will try again next monitor cycle.
             */
            fbe_base_object_trace((fbe_base_object_t*)base_config_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s waiting for peer:0x%x local_qsc_state:0x%x;peer_qsc_state:0x%x\n", 
                          __FUNCTION__, base_config_p->base_object.object_id, (unsigned int)local_quiesce_state, (unsigned int)peer_quiesce_state);

            return FBE_STATUS_OK;
        }
        /* Clear the abort flag prior to telling peer we are ready, since 
         * they might start to unquiesce. 
         * This is needed since when the peer unquiesces, they might start asking us for 
         * stripe locks.  If our abort 'gate' is set then we will fail these requests back to them. 
         */
        fbe_metadata_element_clear_abort(&base_config_p->metadata_element);
    
        /* Quiescing I/O complete on object on this SP; set local state to "unquiesce ready" */
        fbe_base_config_metadata_set_quiesce_state_local_and_update_peer(base_config_p, FBE_BASE_CONFIG_METADATA_MEMORY_UNQUIESCE_READY);

        fbe_base_object_trace((fbe_base_object_t*)base_config_p,
                      FBE_TRACE_LEVEL_DEBUG_HIGH,
                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                      "%s set 0x%x local_quiesce_state: 0x%llx; peer_quiesce_state 0x%llx\n", 
                      __FUNCTION__,
		      base_config_p->base_object.object_id,
		      (unsigned long long)local_quiesce_state,
		      (unsigned long long)peer_quiesce_state);

       /* Complete the unquiesce if the peer is not ready so we don't get stuck waiting.
        */
        if (!fbe_base_config_has_peer_joined(base_config_p))
        {
            *unquiesce_ready_b = FBE_TRUE;
        }
        break;
    
    case FBE_BASE_CONFIG_METADATA_MEMORY_UNQUIESCE_READY:
    
        /* This SP is ready to unquiesce I/O on object */

        /* If peer is available and participating in the quiesce/unquiesce process make sure it is ready to unquiesce */
        if (peer_alive_b && (peer_quiesce_state != FBE_BASE_CONFIG_METADATA_MEMORY_QUIESCE_NOT_STARTED))
        {
            if (peer_quiesce_state != FBE_BASE_CONFIG_METADATA_MEMORY_UNQUIESCE_READY)
            {
                /* The peer is participating in the quiesce/unquiesce process, but is not at the same state, so return.
                 * Will try again next monitor cycle.
                 */
                fbe_base_object_trace((fbe_base_object_t*)base_config_p,
                              FBE_TRACE_LEVEL_DEBUG_HIGH,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                              "%s waiting for peer: 0x%x local_quiesce_state: 0x%llx; peer_quiesce_state: 0x%llx\n", 
                              __FUNCTION__,
		      base_config_p->base_object.object_id,
		      (unsigned long long)local_quiesce_state,
		      (unsigned long long)peer_quiesce_state);

                return FBE_STATUS_OK;
            }
        }

        /* Unquiescing I/O on object can proceed on this SP */

        /* Set the output parameter to TRUE and return */
        fbe_base_object_trace((fbe_base_object_t*)base_config_p,
                      FBE_TRACE_LEVEL_DEBUG_HIGH,
                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                      "%s unquiesce can proceed 0x%x; local_quiesce_state 0x%llx; peer_quiesce_state 0x%llx; peer_alive_b 0x%x\n", 
                      __FUNCTION__,
		      base_config_p->base_object.object_id,
		      (unsigned long long)local_quiesce_state,
		      (unsigned long long)peer_quiesce_state, peer_alive_b);

        *unquiesce_ready_b = FBE_TRUE;
        break;

    case FBE_BASE_CONFIG_METADATA_MEMORY_QUIESCE_NOT_STARTED:

        /*! @note There are still cases were we set the unquiesce condition 
         *        explicitly.  But we now implicitly (i.e. we always set
         *        unquiesce whenever the quiesce condition is set) set
         *        unquiesce.  So it is ok to see an unquiesce multiple times.
         *        It could happen that the unquiesce is complete by the
         *        the second unquiesce gets a chance to run.
         */
        fbe_base_object_trace((fbe_base_object_t*)base_config_p,
                          FBE_TRACE_LEVEL_WARNING,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s quiesce not started not need to unquiesce 0x%x local_quiesce_state: 0x%llx; peer_quiesce_state: 0x%llx\n", 
                          __FUNCTION__, base_config_p->base_object.object_id,
                  (unsigned long long)local_quiesce_state,
                  (unsigned long long)peer_quiesce_state);

        *unquiesce_ready_b = FBE_TRUE;
        break;

    case FBE_BASE_CONFIG_METADATA_MEMORY_QUIESCE_READY:
    
        /* Panic in checked build; local state should not be "quiesce ready" since we are
         * ready to unquiesce (note that quiesce always takes precedence over unquiesce, so should not be here if 
         * we are in the process of quiescing) 
         */
        fbe_base_object_trace((fbe_base_object_t*)base_config_p,
                      FBE_TRACE_LEVEL_ERROR,
                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                      "%s quiesce not complete; cannot unquiesce!! 0x%x local_quiesce_state: 0x%llx; peer_quiesce_state: 0x%llx\n", 
                      __FUNCTION__, base_config_p->base_object.object_id,
		      (unsigned long long)local_quiesce_state,
		      (unsigned long long)peer_quiesce_state);

        break;

    default:

        /* Panic in checked build; invalid state */
        fbe_base_object_trace((fbe_base_object_t*)base_config_p,
                      FBE_TRACE_LEVEL_ERROR,
                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                      "%s invalid local quiesce state!! 0x%x local_quiesce_state: 0x%llx; peer_quiesce_state: 0x%llx\n", 
                      __FUNCTION__, base_config_p->base_object.object_id,
		      (unsigned long long)local_quiesce_state,
		      (unsigned long long)peer_quiesce_state);

        break;
    }

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_base_config_unquiesce_state_check()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_base_config_quiesce_update_memory_request_priority()
 ******************************************************************************
 * @brief
 *  This function is used to determine highest memory request priority of all the i/o which is currently
 *  in base object's terminator queue.
 *
 * @param base_config_p - a pointer to the base config.
 * @param packet - packet from terminator queue
 *
 * @return fbe_status_t
 *
 * @author
 *  03/22/2013 - Created. Peter Puhov
 */
static fbe_status_t 
fbe_base_config_quiesce_update_memory_request_priority(fbe_base_config_t * const base_config_p, fbe_packet_t * packet)
{
	fbe_packet_resource_priority_t resource_priority = 0;

	resource_priority = fbe_transport_get_resource_priority(packet);

	/* Packet resource priority should be updated when we allocation siots , etc. */
	if(base_config_p->resource_priority < resource_priority){
		base_config_p->resource_priority = resource_priority;
	}

	return FBE_STATUS_OK;
}

/*!****************************************************************************
 * fbe_base_config_abort_monitor_ops()
 ******************************************************************************
 * @brief
 *  This function is used to determine whether all the i/o which is currently
 *  in base object's terminator queue are already quiesce or not.
 *
 * @param base_config_p - a pointer to the base config.
 *
 * @return fbe_status_t
 *
 * @author
 *  3/28/2011 - Created. Rob Foley
 ******************************************************************************/
fbe_status_t fbe_base_config_abort_monitor_ops(fbe_base_config_t * const base_config_p)
{
    fbe_queue_head_t *      termination_queue_p = NULL;
    fbe_packet_t *          current_packet_p = NULL;
    fbe_queue_element_t *   next_element_p = NULL;
    fbe_raid_iots_t *       iots_p;
    fbe_queue_element_t *   queue_element_p = NULL;
    
    /* Acquire lock before we look at the termination queue. */
    fbe_spinlock_lock(&base_config_p->base_object.terminator_queue_lock);

    /* Get the head of the termination queue after acquiring lock. */
    termination_queue_p = fbe_base_object_get_terminator_queue_head((fbe_base_object_t *) base_config_p);

    /* Loop over the termination queue and break out when we find something that is not quiesced.  */
    queue_element_p = fbe_queue_front(termination_queue_p);

    while (queue_element_p != NULL)
    {
        fbe_payload_ex_t * sep_payload_p = NULL;
        current_packet_p = fbe_transport_queue_element_to_packet(queue_element_p);
        sep_payload_p = fbe_transport_get_payload_ex(current_packet_p);

        next_element_p = fbe_queue_next(termination_queue_p, &current_packet_p->queue_element);

        /* Check the iots and its siots.
         */
        fbe_payload_ex_get_iots(sep_payload_p, &iots_p);

        /* Peter Puhov 
            We may have CONTROL operation on the terminator queue.
            The iots_p lock will not be initialized and spinlock will panic.
            There are, probably better way to avoid it, but for now this should work.
        */
        if(iots_p->status == FBE_RAID_IOTS_STATUS_INVALID) 
        {
            fbe_base_object_trace((fbe_base_object_t*) base_config_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s iots: %p is uninitialized \n", __FUNCTION__, iots_p);
        }
        else
        {
            /* Verify whether this iots is used or not, if its current status shows
             * that it is not used then we are not using this iots for any i/o operation
             * and so it will not be considered as pending iots which needs to be
             * quiesce.
             */
            fbe_raid_iots_lock(iots_p);
            if (iots_p->status == FBE_RAID_IOTS_STATUS_AT_LIBRARY)
            {
                /* If this iots is not quiesced, break out now.
                 * No need to check the rest of the iots, as we will need to wait anyway for this 
                 * iots to quiesce. 
                 */
                if (fbe_raid_iots_is_background_request(iots_p))
                {
                    fbe_status_t status;
                    status = fbe_raid_iots_abort_monitor_op(iots_p);
                    if (status != FBE_STATUS_OK) 
                    { 
                        fbe_raid_iots_unlock(iots_p);
                        fbe_spinlock_unlock(&base_config_p->base_object.terminator_queue_lock);
                        return status;
                    }
                }
            }
            fbe_raid_iots_unlock(iots_p);
        }
        queue_element_p = next_element_p;
    }

    fbe_spinlock_unlock(&base_config_p->base_object.terminator_queue_lock);

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_base_config_abort_monitor_ops()
 ******************************************************************************/


/*!**************************************************************
 * fbe_base_config_get_quiesced_io_count()
 ****************************************************************
 * @brief
 *  This function gets the siots and checks for the FBE_RAID_SIOTS_FLAG_WAITING_SHUTDOWN_CONTINUE
 *  flag which indicates that the ios to the drives have been completed. If the flag is set
 *  increment the ios that is outstanding
 *
 * @param   in_raid_group_p                  - raid group to execute on
 * @param   number_of_ios_outstanding_p      - number of ios
 *
 * @return fbe_status_t
 *
 * @author
 *  8/25/2010 - Created. Ashwin Tamilarasan
 *
 ****************************************************************/
fbe_status_t fbe_base_config_get_quiesced_io_count(fbe_base_config_t  *base_config_p,
                                                   fbe_u32_t *number_of_ios_outstanding_p)
{

    fbe_queue_head_t        *terminator_queue_p = NULL;
    fbe_queue_element_t     *queue_element_p = NULL;
    fbe_queue_element_t     *next_element_p = NULL;    
    fbe_raid_iots_t*        iots_p = NULL;
    fbe_packet_t*            packet_p = NULL;
    fbe_payload_ex_t*       sep_payload_p = NULL;
    fbe_u32_t               ios_outstanding = 0; 

    terminator_queue_p = fbe_base_object_get_terminator_queue_head((fbe_base_object_t*)base_config_p);

    //  Lock the terminator queue    
    fbe_base_object_terminator_queue_lock((fbe_base_object_t*)base_config_p);
   
       
    queue_element_p = fbe_queue_front(terminator_queue_p);
    while (queue_element_p != NULL)
    {  
        packet_p    = fbe_transport_queue_element_to_packet(queue_element_p);
        sep_payload_p = fbe_transport_get_payload_ex(packet_p);        

        //  Get a pointer to the RG's I/O tracking structure
        fbe_payload_ex_get_iots(sep_payload_p, &iots_p);

        // Lock IOTS before checking if it is quiesce
        fbe_raid_iots_lock(iots_p);

        if (fbe_raid_iots_is_quiesced(iots_p))
        {
            ios_outstanding++;            
        }

        next_element_p  = fbe_queue_next(terminator_queue_p, queue_element_p);        
        queue_element_p = next_element_p;

        // Unlock IOTS
        fbe_raid_iots_unlock(iots_p);
    }    
    //  Unlock the terminator queue
    fbe_base_object_terminator_queue_unlock((fbe_base_object_t*)base_config_p);  
    
    /*! @note Change the level below to FBE_TRACE_LEVEL_ERROR to stop sp on quiesce timeouts 
     */
    fbe_base_object_trace((fbe_base_object_t *)base_config_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH /* FBE_TRACE_LEVEL_ERROR */,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "base_config: ios_outstanding: %d flags: 0x%llx\n", 
                          ios_outstanding, base_config_p->flags);

    *number_of_ios_outstanding_p = ios_outstanding;

    return FBE_STATUS_OK;


}// End fbe_base_config_get_quiesced_io_count

/************************************
 * end fbe_base_config_io_quiesce.c
 ************************************/


