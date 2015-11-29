/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_raid_group_io_quiesce.c
 ***************************************************************************
 *
 * @brief
 *  This file contains general functions for handling quiesce including:
 *  - General quiesce handing.
 *  - Shutdown handling. Restarting I/Os when a shutdown has occurred.
 *  - Continue handling. Restarting I/Os after a drive was removed and
 *                       we determined the state of the group.
 *
 * @version
 *  11/9/2009 - Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe_raid_group_object.h"
#include "fbe_raid_geometry.h"
#include "fbe_raid_library.h"
#include "fbe_raid_group_bitmap.h"


/*************************
 *   FUNCTION DEFINITIONS
 *************************/
static fbe_status_t fbe_raid_group_mark_nr_for_packet_complete(void * packet_p, fbe_packet_completion_context_t context);
static fbe_status_t fbe_raid_group_get_chunk_info_for_mark_nr_completion(fbe_packet_t* in_packet_p,                                    
                                                     fbe_packet_completion_context_t in_context);
static fbe_raid_state_status_t fbe_raid_group_retry_get_chunk_info_for_mark_nr(fbe_raid_iots_t* in_iots_p);
static fbe_raid_state_status_t fbe_raid_group_retry_mark_nr_for_packet(fbe_raid_iots_t* in_iots_p);

/*!***************************************************************
 * fbe_raid_group_handle_stuck_io
 ****************************************************************
 * @brief
 *  This function is the handler to check for stuck io
 *
 * @param rgdb_p - a pointer to the fbe_raid_group_t
 *
 * @return None.
 *
 * @author
 *  07/01/2011 - Created. Naizhong Chiu
 *
 ****************************************************************/

void fbe_raid_group_handle_stuck_io(fbe_raid_group_t * const raid_group_p)
{
    fbe_base_object_t *base_object_p = &raid_group_p->base_config.base_object;
    fbe_queue_head_t *termination_queue_p = &base_object_p->terminator_queue_head;
    fbe_packet_t *current_packet_p = NULL;
    fbe_raid_iots_t *iots_p;
    fbe_queue_element_t *queue_element_p = NULL;
    fbe_time_t current_time = fbe_get_time();
    fbe_time_t iots_age;
    fbe_payload_ex_t *sep_payload = NULL;

    fbe_spinlock_lock(&base_object_p->terminator_queue_lock);

    queue_element_p = fbe_queue_front(termination_queue_p);

    /* Loop over the termination queue and continue everything.
     * We continue to loop until nothing is left to be continued. 
     */
    while (queue_element_p != NULL)
    {
        current_packet_p = fbe_transport_queue_element_to_packet(queue_element_p);
        sep_payload = fbe_transport_get_payload_ex(current_packet_p);

        /* Continue the iots and its siots.
         */
        fbe_payload_ex_get_iots(sep_payload, &iots_p);
        if (iots_p != NULL)
        {
            iots_age = current_time - iots_p->time_stamp;
            if (iots_age > 25000)
            {
                extern fbe_status_t fbe_database_cmi_send_mailbomb(void);
                fbe_database_cmi_send_mailbomb();
                // hopefully this panic ourself
                fbe_base_object_trace((fbe_base_object_t*) raid_group_p, FBE_TRACE_LEVEL_CRITICAL_ERROR, 
                    FBE_TRACE_MESSAGE_ID_INFO, "%s: IO stuck for %llu second, packet_p=0x%p, iots=0x%pn", __FUNCTION__, 
                    (unsigned long long)iots_age, current_packet_p, iots_p);

                iots_p = NULL;
                // panic.
                iots_age = iots_p->time_stamp;
            }
        }

        queue_element_p = fbe_queue_next(termination_queue_p, &current_packet_p->queue_element);
    }
    fbe_spinlock_unlock(&base_object_p->terminator_queue_lock);

    return;
}
/**************************************
 * end fbe_raid_group_handle_stuck_io()
 **************************************/

/*!***************************************************************
 * fbe_raid_group_handle_continue
 ****************************************************************
 * @brief
 *  This function is the handler for a continue ioctl
 *
 * @param rgdb_p - a pointer to the fbe_raid_geometry_t for this group
 *
 * @return None.
 *
 * @author
 *  10/21/2009 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_raid_group_handle_continue(fbe_raid_group_t * const raid_group_p)
{
    fbe_base_object_t *base_object_p = &raid_group_p->base_config.base_object;
    fbe_queue_head_t *termination_queue_p = &base_object_p->terminator_queue_head;
    fbe_packet_t *current_packet_p = NULL;
    fbe_raid_iots_t *iots_p;
    fbe_queue_element_t *next_element_p = NULL;
    fbe_queue_element_t *queue_element_p = NULL;
    fbe_raid_position_bitmask_t rebuild_logging_bitmask;

    fbe_raid_group_lock(raid_group_p);
    fbe_spinlock_lock(&base_object_p->terminator_queue_lock);
    fbe_raid_group_get_rb_logging_bitmask(raid_group_p, &rebuild_logging_bitmask);

    queue_element_p = fbe_queue_front(termination_queue_p);

    /* Loop over the termination queue and continue everything.
     * We continue to loop until nothing is left to be continued. 
     */
    while (queue_element_p != NULL)
    {
        fbe_payload_ex_t *sep_payload = NULL;
        fbe_payload_block_operation_opcode_t opcode;
        fbe_raid_iots_status_t iots_status;
        
        current_packet_p = fbe_transport_queue_element_to_packet(queue_element_p);
        sep_payload = fbe_transport_get_payload_ex(current_packet_p);

        /* Continue the iots and its siots.
         */
        fbe_payload_ex_get_iots(sep_payload, &iots_p);
        fbe_base_object_trace((fbe_base_object_t*) raid_group_p, FBE_TRACE_LEVEL_DEBUG_HIGH, 
            FBE_TRACE_MESSAGE_ID_INFO, "%s: processing packet_p=0x%p, iots=0x%p\n", __FUNCTION__, 
            current_packet_p, iots_p);

        next_element_p = fbe_queue_next(termination_queue_p, &current_packet_p->queue_element);

        fbe_raid_iots_get_status(iots_p, &iots_status);
        if (iots_status == FBE_RAID_IOTS_STATUS_NOT_USED)
        {
            /* This iots is not in use, skip it.
             */
            queue_element_p = next_element_p;
            continue;
        }

        fbe_raid_iots_get_opcode(iots_p, &opcode);

        /* Refresh the rebuild logging bitmask inside the iots so the iots has
         * a fresh view of degraded. 
         */
        fbe_raid_group_get_rb_logging_bitmask(raid_group_p, &iots_p->rebuild_logging_bitmask);

        if (iots_p->callback == NULL)
        {
            /* Do not touch this iots, leave it on the queue.
             */
        }
        else if ((rebuild_logging_bitmask) &&
                 (fbe_payload_block_operation_opcode_is_media_modify(opcode) == FBE_TRUE))
        {
            /* This is a data modification operation, update RL.
             */

            /* Tell unquiesce code not to start siots but restart iots */
            fbe_raid_iots_set_flag(iots_p, FBE_RAID_IOTS_FLAG_RESTART_IOTS);

            /* Now set up the iots to mark this area as needing a rebuild.
             */
            fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_DEBUG_LOW, FBE_TRACE_MESSAGE_ID_INFO,
                                  "raid group: setup mark needs rebuild iots: %p rl: 0x%x\n", iots_p, iots_p->rebuild_logging_bitmask);
            fbe_raid_iots_transition_quiesced(iots_p, fbe_raid_group_restart_mark_nr, raid_group_p);
        }
        else
        {
            /* No metadata to update.  Just continue this iots.
             */
            fbe_raid_iots_continue(iots_p, rebuild_logging_bitmask);
        }

        queue_element_p = next_element_p;
    }
    fbe_spinlock_unlock(&base_object_p->terminator_queue_lock);
    fbe_raid_group_unlock(raid_group_p);

    return;
}
/**************************************
 * end fbe_raid_group_handle_continue()
 **************************************/

/*!**************************************************************
 * fbe_raid_group_restart_mark_nr()
 ****************************************************************
 * @brief
 *  Restart the mark NR after it has been queued.
 *
 * @param iots_p - I/O to restart.               
 *
 * @return fbe_raid_state_status_t - FBE_RAID_STATE_STATUS_WAITING
 *
 * @author
 *  10/08/2011 - Created. NChiu
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_raid_group_restart_mark_nr(fbe_raid_iots_t *iots_p)
{
    fbe_status_t status;
    fbe_packet_t *packet_p = fbe_raid_iots_get_packet(iots_p);
    fbe_raid_group_t *raid_group_p = (fbe_raid_group_t*)iots_p->callback_context;
    fbe_payload_ex_t *sep_payload_p = NULL;

    /* We need to remove this from the queue in order to start metadata IO. 
     */
    status = fbe_raid_group_remove_from_terminator_queue(raid_group_p, packet_p);
    if (status != FBE_STATUS_OK) 
    { 
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s error %d removing packet 0x%p from queue\n", 
                              __FUNCTION__, status, packet_p);
    }

    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_payload_ex_get_iots(sep_payload_p, &iots_p);
    /* we are about to do metadata operation, we need to make sure there's no outstanding IO */
    if (!fbe_raid_iots_is_flag_set(iots_p, FBE_RAID_IOTS_FLAG_QUIESCE))
    {
        fbe_base_object_trace((fbe_base_object_t*) raid_group_p, FBE_TRACE_LEVEL_CRITICAL_ERROR, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, "%s - iots 0x%p not quiesced,  packet 0x%p\n", __FUNCTION__, iots_p, packet_p); 
    }
    //  Restore the iots common state 
    fbe_raid_iots_reset_generate_state(iots_p);

    //  Restore the iots callback function
    fbe_raid_iots_set_callback(iots_p, fbe_raid_group_iots_completion, raid_group_p);

    /* We mark this so that this iots does not look like it is at the library. 
     * This is needed so that anyone looking at the terminator queue knows this is 
     * active. 
     */
    fbe_raid_iots_set_status(iots_p, FBE_RAID_IOTS_STATUS_LIBRARY_WAITING_FOR_CONTINUE);

    fbe_raid_iots_mark_unquiesced(iots_p);

    fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s: restarting mark NR iots %p rl: 0x%x\n", __FUNCTION__, iots_p, iots_p->rebuild_logging_bitmask);

    /* We need to mark this chunk as needing a rebuild. Do not 
     * allow the packet to get cancelled, since at this point we must 
     * complete the write to avoid getting an inconsistent stripe. 
     */
    fbe_transport_set_packet_attr(packet_p, FBE_PACKET_FLAG_DO_NOT_CANCEL);

    fbe_raid_group_mark_nr_for_iots(raid_group_p, iots_p, (fbe_packet_completion_function_t)fbe_raid_group_mark_nr_for_packet_complete);

    return FBE_RAID_STATE_STATUS_WAITING;
}
/******************************************
 * end fbe_raid_group_restart_mark_nr()
 ******************************************/

/*!**************************************************************
 * fbe_raid_group_iots_restart_completion()
 ****************************************************************
 * @brief
 *  Just restart the iots completion which took some error.
 *
 * @param iots_p - I/O to fail.               
 *
 * @return fbe_raid_state_status_t - FBE_RAID_STATE_STATUS_WAITING
 *
 * @author
 *  12/14/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_raid_group_iots_restart_completion(fbe_raid_iots_t *iots_p)
{
    fbe_packet_t *packet_p = fbe_raid_iots_get_packet(iots_p);
    fbe_payload_ex_t *sep_payload_p = NULL;
    fbe_payload_block_operation_opcode_t opcode;
    fbe_raid_group_t *raid_group_p = (fbe_raid_group_t *)iots_p->callback_context;

    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_payload_ex_get_iots(sep_payload_p, &iots_p);
    fbe_raid_iots_mark_unquiesced(iots_p);

    fbe_raid_iots_get_opcode(iots_p, &opcode);
    fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                          "raid group: iots restart completion iots op: 0x%x lba: 0x%llx blks: 0x%llx\n",  
                          opcode, (unsigned long long)iots_p->lba,
			  (unsigned long long)iots_p->blocks);

    /* Mark complete since it was marked this way before we were queued.
     */
    fbe_raid_iots_set_status(iots_p, FBE_RAID_IOTS_STATUS_COMPLETE);
    /* Just complete the iots now.
     */
    fbe_raid_group_iots_completion(packet_p, iots_p->callback_context);
    return FBE_RAID_STATE_STATUS_WAITING;
}
/******************************************
 * end fbe_raid_group_iots_restart_completion()
 ******************************************/
/*!**************************************************************
 * fbe_raid_group_restart_iots()
 ****************************************************************
 * @brief
 *  Restart the iots after it has been queued.
 *  It was queued just after getting the lock.
 *
 * @param iots_p - I/O to restart.               
 *
 * @return fbe_raid_state_status_t - FBE_RAID_STATE_STATUS_WAITING
 *
 * @author
 *  3/24/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_raid_group_restart_iots(fbe_raid_iots_t *iots_p)
{
    fbe_packet_t *packet_p = fbe_raid_iots_get_packet(iots_p);
    fbe_raid_group_t *raid_group_p = (fbe_raid_group_t*)iots_p->callback_context;
    fbe_payload_ex_t *sep_payload_p = NULL;

    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_payload_ex_get_iots(sep_payload_p, &iots_p);
    fbe_raid_iots_mark_unquiesced(iots_p);

    fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s: restarting iots %p\n", __FUNCTION__, iots_p);

    /* Lock is granted, proceed to start the I/O.
     */
    fbe_raid_group_start_iots_with_lock(raid_group_p, iots_p);
    return FBE_RAID_STATE_STATUS_WAITING;
}
/******************************************
 * end fbe_raid_group_restart_iots()
 ******************************************/

fbe_status_t fbe_raid_group_display_termination_queue(fbe_raid_group_t * const raid_group_p)
{
    fbe_base_object_t *base_object_p = &raid_group_p->base_config.base_object;
    fbe_queue_head_t *termination_queue_p = &base_object_p->terminator_queue_head;
    fbe_packet_t *current_packet_p = NULL;
    fbe_raid_iots_t *iots_p;
    fbe_queue_head_t restart_queue;
    fbe_queue_element_t *next_element_p = NULL;
    fbe_queue_element_t *queue_element_p = NULL;

    fbe_spinlock_lock(&base_object_p->terminator_queue_lock);

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

        fbe_base_object_trace(base_object_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                              "%s packet: %p iots: %p\n", __FUNCTION__, current_packet_p, iots_p);

        queue_element_p = next_element_p;
    }
    fbe_spinlock_unlock(&base_object_p->terminator_queue_lock);
    return FBE_STATUS_OK;
}

/****************************************************************
 *  fbe_raid_group_handle_shutdown
 ****************************************************************
 * @brief
 *  This function is the handler for a shutdown.
 *
 * @param raid_group_p - a pointer to raid group
 *
 * @return
 *  fbe_status_t
 *
 * @author
 *  10/27/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_group_handle_shutdown(fbe_raid_group_t * const raid_group_p)
{
    fbe_queue_head_t *termination_queue_p = &raid_group_p->base_config.base_object.terminator_queue_head;
    fbe_base_object_t *base_object_p = &raid_group_p->base_config.base_object;
    fbe_packet_t *current_packet_p = NULL;
    fbe_raid_iots_t *iots_p = NULL;
    fbe_queue_head_t restart_queue;
    fbe_queue_element_t *next_element_p = NULL;
    fbe_queue_element_t *queue_element_p = NULL;
    fbe_u32_t restart_siots_count = 0;
    fbe_bool_t b_done = FBE_TRUE;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);

    fbe_raid_group_lock(raid_group_p);

    fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                          FBE_TRACE_LEVEL_DEBUG_LOW,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s fl: 0x%x\n", 
                          __FUNCTION__, raid_group_p->flags);

    fbe_spinlock_lock(&raid_group_p->base_config.base_object.terminator_queue_lock);

    /* With lock held, mark the raid group as not quiesced. 
     * This allows siots to restart below. 
     */
    fbe_base_config_clear_clustered_flag((fbe_base_config_t *) raid_group_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_QUIESCED);
    fbe_base_config_clear_clustered_flag((fbe_base_config_t *) raid_group_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_QUIESCING);

    /* if this is a parity object, we need to quiesce the write log 
     */
    if (fbe_raid_geometry_is_raid3(raid_geometry_p)
        || fbe_raid_geometry_is_raid5(raid_geometry_p) 
        || fbe_raid_geometry_is_raid6(raid_geometry_p))
    {
        /* Quiesce the write log -- this will delete the queue and lock the parity group log 
         */
        fbe_parity_write_log_abort(raid_geometry_p->raid_type_specific.journal_info.write_log_info_p);
    }

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

        /* Add the siots for this iots that need to be restarted. 
         * We will kick them off below once we have finished iterating over 
         * the termination queue. 
         */
        if ((iots_p->status == FBE_RAID_IOTS_STATUS_NOT_STARTED_TO_LIBRARY) ||
            (iots_p->status == FBE_RAID_IOTS_STATUS_WAITING_FOR_QUIESCE))
        {
            /* Not started to the library yet.
             * Set this up so it can get completed through our standard completion path. 
             */
            if (iots_p->callback != NULL) 
            { 
                fbe_base_object_trace(base_object_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s iots: %p callback is not null (0x%p)\n", __FUNCTION__, iots_p, iots_p->callback);
            }
            if (iots_p->callback_context == NULL)
            {
                fbe_base_object_trace(base_object_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s iots: %p callback_context is null\n", __FUNCTION__, iots_p);
            }
            if (iots_p->common.state == NULL) 
            {
                fbe_base_object_trace(base_object_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s iots: %p state is null\n", __FUNCTION__, iots_p);
            }
            fbe_raid_iots_mark_complete(iots_p);
            fbe_raid_iots_set_callback(iots_p, fbe_raid_group_iots_cleanup, raid_group_p);
            fbe_raid_common_wait_enqueue_tail(&restart_queue, &iots_p->common);
            /* We set retry not possible since the client cannot be expected that the raid group will 
             * be available.  Whenever the raid group goes shutdown we return retry not possible. 
             */
            fbe_raid_iots_set_block_status(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED);
            fbe_raid_iots_set_block_qualifier(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE);
        }
        else if(iots_p->status == FBE_RAID_IOTS_STATUS_NOT_USED)
        {
            /* If iots is not used then skip this terminated packet, eventually
             * packet will complete and it will handle the failure. 
             */
            queue_element_p = next_element_p;
            continue;
        }
        else
        {
            /* This iots was started to the library.
             */
            status = fbe_raid_iots_handle_shutdown(iots_p, &restart_queue);

            /* Since we don't go thru iots finished we must decrement the queisce
             * count here.
             */
            if ((iots_p->status == FBE_RAID_IOTS_STATUS_COMPLETE)                  &&
                fbe_raid_iots_is_flag_set(iots_p, FBE_RAID_IOTS_FLAG_WAS_QUIESCED)    )
            {
                fbe_raid_group_dec_quiesced_count(raid_group_p);

                /* Trace some information if enabled.
                 */
                fbe_raid_group_trace(raid_group_p,
                                 FBE_TRACE_LEVEL_INFO,
                                 FBE_RAID_GROUP_DEBUG_FLAG_QUIESCE_TRACING,
                                 "raid_group: shutdown clear was quiesced iots:0x%x lba/bl: 0x%llx/0x%llx flags:0x%x cnt:%d\n",
                                 (fbe_u32_t)iots_p,
                                 (unsigned long long)iots_p->lba,
                                 (unsigned long long)iots_p->blocks,
                                 iots_p->flags, raid_group_p->quiesced_count);

                /* Clear the flag.
                 */
                fbe_raid_iots_clear_flag(iots_p, FBE_RAID_IOTS_FLAG_WAS_QUIESCED);
            }
        }
        /* If we found anything on the termination queue then we are not done.
         */
        b_done = FBE_FALSE;
        queue_element_p = next_element_p;
    }

    fbe_spinlock_unlock(&raid_group_p->base_config.base_object.terminator_queue_lock);
    fbe_raid_group_unlock(raid_group_p);

    /* Restart all the items we found.
     */
    restart_siots_count = fbe_raid_restart_common_queue(&restart_queue);

    /* Trace out the number of siots we kicked off.
     */
    if (restart_siots_count)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                              "%s %d siots restarted for shutdown\n", __FUNCTION__, restart_siots_count);
    }
    return (b_done) ? FBE_STATUS_OK : FBE_STATUS_PENDING;
}
/*****************************************
 * fbe_raid_group_handle_shutdown()
 *****************************************/

/*!***************************************************************
 *  fbe_raid_group_handle_aborted_packets
 ****************************************************************
 * @brief
 *  This function loops over the termination queue and
 *  aborts any packets that need it.
 *
 * @param raid_group_p - Raid group to check
 * 
 * @return
 *  fbe_status_t
 *
 * @note Assumes raid group lock is held.
 *
 * @author
 *  4/19/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_group_handle_aborted_packets(fbe_raid_group_t * const raid_group_p)
{
    fbe_queue_head_t *termination_queue_p = &raid_group_p->base_config.base_object.terminator_queue_head;
    fbe_packet_t *current_packet_p = NULL;
    fbe_queue_element_t *next_element_p = NULL;
    fbe_raid_iots_t *iots_p = NULL;
    fbe_queue_element_t *queue_element_p = NULL;
    fbe_u32_t iots_count = 0;

    fbe_spinlock_lock(&raid_group_p->base_config.base_object.terminator_queue_lock);

    /* Loop over the termination queue and break out when we find something that is not
     * quiesced. 
     */
    queue_element_p = fbe_queue_front(termination_queue_p);

    while (queue_element_p != NULL)
    {
        fbe_payload_ex_t *sep_payload_p = NULL;
        fbe_raid_iots_status_t iots_status;
        current_packet_p = fbe_transport_queue_element_to_packet(queue_element_p);
        sep_payload_p = fbe_transport_get_payload_ex(current_packet_p);
        next_element_p = fbe_queue_next(termination_queue_p, &current_packet_p->queue_element);

        fbe_payload_ex_get_iots(sep_payload_p, &iots_p);
        fbe_raid_iots_get_status(iots_p, &iots_status);

        if (fbe_transport_is_packet_cancelled(current_packet_p) &&
            (iots_status == FBE_RAID_IOTS_STATUS_AT_LIBRARY))
        {
            /* Check the iots and its siots.
             */
            fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "raid group: rg_handle_abort_packet cancel iots: %p lba: 0x%llx bl: 0x%llx\n", 
                                  iots_p, (unsigned long long)iots_p->lba,
				  (unsigned long long)iots_p->blocks);
            fbe_raid_iots_abort(iots_p);
            fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "raid group: rg_handle_abort_packet cancelled iots: %p lba: 0x%llx bl: 0x%llx\n", iots_p,
				    (unsigned long long)iots_p->lba,
				    (unsigned long long)iots_p->blocks);
        }

        queue_element_p = next_element_p;
        iots_count++;
    }
    fbe_spinlock_unlock(&raid_group_p->base_config.base_object.terminator_queue_lock);

    return FBE_STATUS_OK;
}
/*****************************************
 * fbe_raid_group_handle_aborted_packets()
 *****************************************/

/*!***************************************************************
 *  fbe_raid_group_abort_all_metadata_operations
 ****************************************************************
 * @brief
 *  This function loops over the termination queue and
 *  aborts any metadata operations.
 *
 * @param raid_group_p - Raid group to check
 * 
 * @return
 *  fbe_status_t
 *
 * @author
 *  9/11/2013 - Created. NCHIU
 *
 ****************************************************************/

fbe_status_t fbe_raid_group_abort_all_metadata_operations(fbe_raid_group_t * const raid_group_p)
{
    fbe_queue_head_t *termination_queue_p = &raid_group_p->base_config.base_object.terminator_queue_head;
    fbe_packet_t *current_packet_p = NULL;
    fbe_queue_element_t *next_element_p = NULL;
    fbe_raid_iots_t *iots_p = NULL;
    fbe_queue_element_t *queue_element_p = NULL;
    fbe_u32_t iots_count = 0;
    fbe_queue_head_t restart_queue;
    fbe_u32_t restart_siots_count = 0;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_bool_t b_done = FBE_TRUE;

    fbe_raid_group_lock(raid_group_p);

    fbe_spinlock_lock(&raid_group_p->base_config.base_object.terminator_queue_lock);

    /* Loop over the termination queue and break out when we find something that is not
     * quiesced. 
     */
    fbe_queue_init(&restart_queue);
    queue_element_p = fbe_queue_front(termination_queue_p);

    while (queue_element_p != NULL)
    {
        fbe_payload_ex_t *sep_payload_p = NULL;
        fbe_raid_iots_status_t iots_status;
        current_packet_p = fbe_transport_queue_element_to_packet(queue_element_p);
        sep_payload_p = fbe_transport_get_payload_ex(current_packet_p);
        next_element_p = fbe_queue_next(termination_queue_p, &current_packet_p->queue_element);

        fbe_payload_ex_get_iots(sep_payload_p, &iots_p);
        fbe_raid_iots_get_status(iots_p, &iots_status);

        if ((iots_status == FBE_RAID_IOTS_STATUS_AT_LIBRARY) &&
            (fbe_raid_iots_is_metadata_operation(iots_p)))
        {
            /* Check the iots and its siots.
             */
            fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "raid group: rg_abort_md_ops cancel iots: %p lba: 0x%llx bl: 0x%llx\n", 
                                  iots_p, (unsigned long long)iots_p->lba,
                                  (unsigned long long)iots_p->blocks);

            status = fbe_raid_iots_handle_shutdown(iots_p, &restart_queue);

            /* Since we don't go thru iots finished we must decrement the queisce
             * count here.
             */
            if ((iots_p->status == FBE_RAID_IOTS_STATUS_COMPLETE)                  &&
                fbe_raid_iots_is_flag_set(iots_p, FBE_RAID_IOTS_FLAG_WAS_QUIESCED)    )
            {
                fbe_raid_group_dec_quiesced_count(raid_group_p);

                /* Trace some information if enabled.
                 */
                fbe_raid_group_trace(raid_group_p,
                                 FBE_TRACE_LEVEL_INFO,
                                 FBE_RAID_GROUP_DEBUG_FLAG_QUIESCE_TRACING,
                                 "raid_group: rg_abort_md_ops clear quiesced iots:0x%x lba/bl: 0x%llx/0x%llx flags:0x%x cnt:%d\n",
                                 (fbe_u32_t)iots_p,
                                 (unsigned long long)iots_p->lba,
                                 (unsigned long long)iots_p->blocks,
                                 iots_p->flags, raid_group_p->quiesced_count);

                /* Clear the flag.
                 */
                fbe_raid_iots_clear_flag(iots_p, FBE_RAID_IOTS_FLAG_WAS_QUIESCED);        
            }
            /* If we found anything then we are not done.
             */
            b_done = FBE_FALSE;
        }
        queue_element_p = next_element_p;
        iots_count++;
    }
    fbe_spinlock_unlock(&raid_group_p->base_config.base_object.terminator_queue_lock);

    fbe_raid_group_unlock(raid_group_p);

    /* Restart all the items we found.
     */
    restart_siots_count = fbe_raid_restart_common_queue(&restart_queue);

    /* Trace out the number of siots we kicked off.
     */
    if (restart_siots_count)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                              "%s %d siots restarted for shutdown\n", __FUNCTION__, restart_siots_count);
    }
    return (b_done) ? FBE_STATUS_OK : FBE_STATUS_PENDING;
}
/*****************************************
 * fbe_raid_group_abort_all_metadata_operations()
 *****************************************/

/*!**************************************************************
 * fbe_raid_group_mark_nr_for_packet_complete()
 ****************************************************************
 * @brief
 *  We finished updating metadata, now kick off the I/O.
 *
 * @param packet_p - Packet that is starting.
 * @param context - the raid group. 
 *
 * @return fbe_status_t.
 *
 * @author
 *  3/23/2010 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_raid_group_mark_nr_for_packet_complete(void * packet_p, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_raid_group_t *raid_group_p = (fbe_raid_group_t *)context;
    fbe_raid_iots_t *iots_p = NULL;
    fbe_payload_ex_t *sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_raid_position_bitmask_t current_rebuild_logging_bitmask = 0;
    fbe_raid_position_bitmask_t iots_rebuild_logging_bitmask = 0;
    fbe_bool_t packet_was_failed_b;
    fbe_raid_geometry_t     *raid_geometry_p;

    fbe_payload_ex_get_iots(sep_payload_p, &iots_p);

    raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    status = fbe_transport_get_status_code(packet_p);

    if (status != FBE_STATUS_OK) 
    { 
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "mark nr for iots %p md write failed status 0x%x\n", iots_p, status);
            
        /*  Handle the metadata error and return here */

        fbe_raid_group_executor_handle_metadata_error(raid_group_p, packet_p,
                                                      fbe_raid_group_retry_mark_nr_for_packet, &packet_was_failed_b,
                                                      status);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    /*! @note  It is perfectly valid for the rebuild_logging_bitmask to change
     *         from the time the I/O was started till now.  If the drives returned
     *         or were replaced, the new rebuild logging bitmask could be 0.
     */
    fbe_raid_group_get_rb_logging_bitmask(raid_group_p, &current_rebuild_logging_bitmask);
    fbe_raid_iots_get_rebuild_logging_bitmask(iots_p, &iots_rebuild_logging_bitmask);
    fbe_raid_iots_set_rebuild_logging_bitmask(iots_p, current_rebuild_logging_bitmask);

    /* If the rebuild logging bitmask has changed log an informational trace.
     * If more drive drives have gone away we need to re-issue the set NR.  If 
     * drives have returned it is ok to continue since we have set NR for them.
     */
    if (current_rebuild_logging_bitmask != iots_rebuild_logging_bitmask)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "mark nr for packet rebuild logging bitmask changed from: 0x%x to 0x%x \n", 
                              iots_rebuild_logging_bitmask, current_rebuild_logging_bitmask); 

        /* If more drives have gone away re-issue the mark NR (so that the 
         * information both on-disk is always correct).
         */
        if ((current_rebuild_logging_bitmask & ~(iots_rebuild_logging_bitmask)) != 0)
        {
            /* Additional positions have gone away.  Refresh NR
             */
            /*Split trace to two lines*/
            fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                "mark nr for packet additional rebuild logging bitmask: 0x%x>\n", 
                (current_rebuild_logging_bitmask & ~(iots_rebuild_logging_bitmask))); 
			fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                "re-issue mark nr, packet 0x%p iots 0x%p<\n", 
                packet_p, iots_p); 

            fbe_raid_group_mark_nr_for_iots(raid_group_p, iots_p, (fbe_packet_completion_function_t)fbe_raid_group_mark_nr_for_packet_complete);
            return FBE_STATUS_MORE_PROCESSING_REQUIRED;
        }
    }
    fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                          "mark nr for pkt complete iots 0x%p rl: 0x%x\n", 
                          iots_p, iots_p->rebuild_logging_bitmask);

    /* Now use the metadata chunk information to populate the iots chunk
     * information.  The chunk information must always be up to date before
     * sending the iots to the raid library.
     */
    fbe_transport_set_completion_function(packet_p, fbe_raid_group_get_chunk_info_for_mark_nr_completion,
            (fbe_packet_completion_context_t) raid_group_p);

    fbe_raid_group_get_chunk_info(raid_group_p, packet_p, iots_p);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/**************************************
 * end fbe_raid_group_mark_nr_for_packet_complete()
 **************************************/

/*!**************************************************************
 * fbe_raid_group_get_chunk_info_for_mark_nr_completion()
 ****************************************************************
 * @brief
 *   This is the completion function for a read to the paged 
 *   metadata to get the chunk info, when called from fbe_raid_group_
 *   mark_nr_for_packet_complete().  It will restart the I/O. 
 *
 * @param in_packet_p   - packet that is completing
 * @param in_context    - completion context, which is a pointer to 
 *                        the raid group object
 *
 * @return fbe_status_t
 *
 * @author
 *  07/20/2010 - Created. Jean Montes.
 *
 ****************************************************************/
static fbe_status_t fbe_raid_group_get_chunk_info_for_mark_nr_completion(
                                    fbe_packet_t*                       in_packet_p,
                                    fbe_packet_completion_context_t     in_context)

{
    fbe_raid_group_t*               raid_group_p;                       // pointer to raid group object 
    fbe_status_t                    status;                             // fbe status
    fbe_payload_ex_t*              sep_payload_p;                      // pointer to sep payload
    fbe_raid_iots_t*                iots_p;                             // pointer to the IOTS of the I/O
    fbe_bool_t                      packet_was_failed_b;                // TRUE if called func failed the packet


    //  Get the raid group pointer from the input context 
    raid_group_p = (fbe_raid_group_t*) in_context; 

    //  Get the IOTS for this I/O
    sep_payload_p = fbe_transport_get_payload_ex(in_packet_p);
    fbe_payload_ex_get_iots(sep_payload_p, &iots_p);

    //  Check the packet status to see if reading the chunk info succeeded 
    status = fbe_transport_get_status_code(in_packet_p);
    if (status != FBE_STATUS_OK) 
    { 
        fbe_base_object_trace((fbe_base_object_t*) raid_group_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
            "mrk_nr_cmpln: md read failed, packet 0x%p, status 0x%x \n", in_packet_p, status);

        //  Handle the metadata error and return here 
        fbe_raid_group_executor_handle_metadata_error(raid_group_p, in_packet_p,
                                                      fbe_raid_group_retry_get_chunk_info_for_mark_nr, &packet_was_failed_b,
                                                      status);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED; 
    }
    fbe_base_object_trace((fbe_base_object_t*) raid_group_p,
                          FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                          "mrk_nr_cmpln: pkt %p, st: 0x%x pkt_state: 0x%llx rl: 0x%x\n",
                          in_packet_p, status, (unsigned long long)in_packet_p->packet_state, 
                          iots_p->rebuild_logging_bitmask);

    //  Put the packet back on the termination queue since it was off before reading the chunk info
    fbe_raid_group_add_to_terminator_queue(raid_group_p, in_packet_p);

    /* We had set the expiration time when we did the metadata operation, so we need to set it 
     * back to a reasonable value. 
     */
    //fbe_transport_set_expiration_time(fbe_raid_iots_get_packet(iots_p), 0);

    //  Restart the I/O
    fbe_raid_iots_restart_iots_after_metadata_update(iots_p);

    //  Return more processing since we have a request outstanding 
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;

}
/***************************************************************
 * end fbe_raid_group_get_chunk_info_for_mark_nr_completion()
 ***************************************************************/

/*!**************************************************************
 * fbe_raid_group_find_failed_io_positions()
 ****************************************************************
 * @brief
 *  Return a bitmap of the failed positions in a RG based on the RG's I/O tracking structures.
 *  This function traverses the RG's terminator queue accordingly.
 *
 * @param   in_raid_group_p                     - raid group to execute on
 * @param   out_failed_io_position_bitmap_p     - bitmap of positions with failed I/Os
 *
 * @return fbe_status_t
 *
 * @author
 *  7/2010 - Created. rundbs
 *
 ****************************************************************/
fbe_status_t fbe_raid_group_find_failed_io_positions(fbe_raid_group_t  *in_raid_group_p,
                                                     fbe_u16_t         *out_failed_io_position_bitmap_p)
{
    fbe_queue_head_t        *terminator_queue_p;
    fbe_queue_element_t     *queue_element_p;
    fbe_queue_element_t     *next_element_p;
    fbe_packet_t            *current_packet_p;
    fbe_payload_ex_t       *sep_payload_p;
    fbe_raid_iots_t         *iots_p;
    fbe_u32_t               error_bitmap;
    fbe_u32_t               running_total_error_bitmap;


    //  Initialize local variables
    terminator_queue_p              = &in_raid_group_p->base_config.base_object.terminator_queue_head;
    error_bitmap                    = 0;
    running_total_error_bitmap      = 0;

    //  Lock the RG and its terminator queue
    fbe_raid_group_lock(in_raid_group_p);
    fbe_spinlock_lock(&in_raid_group_p->base_config.base_object.terminator_queue_lock);

    //  Loop over the RG terminator queue and find positions with failed IOs
    queue_element_p = fbe_queue_front(terminator_queue_p);
    while (queue_element_p != NULL)
    {
        current_packet_p    = fbe_transport_queue_element_to_packet(queue_element_p);
        sep_payload_p       = fbe_transport_get_payload_ex(current_packet_p);
        next_element_p      = fbe_queue_next(terminator_queue_p, &current_packet_p->queue_element);

        //  Get a pointer to the RG's I/O tracking structure
        fbe_payload_ex_get_iots(sep_payload_p, &iots_p);

        if (iots_p->status != FBE_RAID_IOTS_STATUS_NOT_USED)
        {
            //  Get a bitmap of the failed positions in the RG's iots
            fbe_raid_iots_get_failed_io_pos_bitmap(iots_p, &error_bitmap);
    
            //  Add the error bitmap to a running total for the queue
            running_total_error_bitmap |= error_bitmap;
        }
        queue_element_p = next_element_p;
    }    

    //  Unlock the RG and its terminator queue
    fbe_spinlock_unlock(&in_raid_group_p->base_config.base_object.terminator_queue_lock);
    fbe_raid_group_unlock(in_raid_group_p);

    //  Set the output parameter
    *out_failed_io_position_bitmap_p = running_total_error_bitmap;

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_raid_group_find_failed_io_positions()
 ******************************************/

/*!****************************************************************************
 * fbe_raid_group_retry_get_chunk_info_for_mark_nr()
 ******************************************************************************
 * @brief
 *   This function will retry getting the chunk information (after marking NR   
 *   on an iots) if the original metadata operation failed.  It is called after 
 *   the monitor has determined it is ready to re-try the operation.  
 *
 * @notes
 *   When this function is called, the packet is on the terminator queue. 
 *
 * @param in_iots_p                 - pointer to the IOTS 
 *
 * @return fbe_raid_state_status_t  - FBE_RAID_STATE_STATUS_WAITING
 *
 * @author
 *  09/07/2010 - Created. Jean Montes.
 *
 ******************************************************************************/
static fbe_raid_state_status_t fbe_raid_group_retry_get_chunk_info_for_mark_nr(
                                            fbe_raid_iots_t*            in_iots_p)

{
    fbe_raid_group_t*                       raid_group_p;               // pointer to raid group object 
    fbe_packet_t*                           packet_p;                   // pointer to the packet 


    //  Get the raid group from where we have stored it in the iots 
    raid_group_p = (fbe_raid_group_t*) in_iots_p->callback_context; 

    //  Get the packet pointer from the iots
    packet_p = fbe_raid_iots_get_packet(in_iots_p);

    fbe_base_object_trace((fbe_base_object_t*) raid_group_p, FBE_TRACE_LEVEL_INFO, 
        FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, "%s - called for packet 0x%p iots 0x%p\n", __FUNCTION__, 
        packet_p, in_iots_p);

    //  Restore the iots common state 
    fbe_raid_iots_reset_generate_state(in_iots_p);

    //  Restore the iots callback function
    fbe_raid_iots_set_callback(in_iots_p, fbe_raid_group_iots_completion, raid_group_p);

    //  Unquiesce the iots 
    fbe_raid_iots_mark_unquiesced(in_iots_p);

    //  Set the completion function
    fbe_transport_set_completion_function(packet_p, fbe_raid_group_get_chunk_info_for_mark_nr_completion,
        (fbe_packet_completion_context_t) raid_group_p);

    //  Remove the packet from the terminator queue, as the function called below expects it to be off the queue
    fbe_raid_group_remove_from_terminator_queue(raid_group_p, packet_p);

    //  Get the chunk info 
    fbe_raid_group_get_chunk_info(raid_group_p, packet_p, in_iots_p);

    //  Return a raid library status that we are waiting
    return FBE_RAID_STATE_STATUS_WAITING; 

} // End fbe_raid_group_retry_get_chunk_info_for_mark_nr()


/*!****************************************************************************
 * fbe_raid_group_retry_mark_nr_for_packet()
 ******************************************************************************
 * @brief
 *   This function will retry marking NR, if the original metadata operation 
 *   failed.  It is called after the monitor has determined it is ready to re-
 *   try the operation.  
 *
 * @notes
 *   When this function is called, the packet is on the terminator queue. 
 *
 * @param in_iots_p                 - pointer to the IOTS 
 *
 * @return fbe_raid_state_status_t  - FBE_RAID_STATE_STATUS_WAITING
 *
 * @author
 *  09/07/2010 - Created. Jean Montes.
 *
 ******************************************************************************/
static fbe_raid_state_status_t fbe_raid_group_retry_mark_nr_for_packet(
                                            fbe_raid_iots_t*            in_iots_p)

{
    fbe_raid_group_t*                       raid_group_p;               // pointer to raid group object 
    fbe_packet_t*                           packet_p;                   // pointer to the packet 
    fbe_raid_position_bitmask_t             rebuild_logging_bitmask;    // bitmask of positions in NR on demand


    //  Get the raid group from where we have stored it in the iots 
    raid_group_p = (fbe_raid_group_t*) in_iots_p->callback_context; 

    //  Get the packet pointer from the iots
    packet_p = fbe_raid_iots_get_packet(in_iots_p);

    //  Remove the packet from the terminator queue, since the rest of the code expects it is off the queue
    fbe_raid_group_remove_from_terminator_queue(raid_group_p, packet_p);

    //  Restore the iots callback function
    fbe_raid_iots_set_callback(in_iots_p, fbe_raid_group_iots_completion, raid_group_p);

    fbe_raid_iots_reset_generate_state(in_iots_p);

    /* we are about to do metadata operation, we need to make sure there's no outstanding IO */
    if (!fbe_raid_iots_is_flag_set(in_iots_p, FBE_RAID_IOTS_FLAG_QUIESCE))
    {
        fbe_base_object_trace((fbe_base_object_t*) raid_group_p, FBE_TRACE_LEVEL_CRITICAL_ERROR, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, "%s - iots 0x%p not quiesced,  packet 0x%p\n", __FUNCTION__, in_iots_p, packet_p); 
    }
    //  Unquiesce the iots 
    fbe_raid_iots_mark_unquiesced(in_iots_p);

    //  Get the rebuild logging bitmask / NR on demand mask
    fbe_raid_group_get_rb_logging_bitmask(raid_group_p, &rebuild_logging_bitmask);

    //  Mark NR for the iots 
    fbe_base_object_trace((fbe_base_object_t*) raid_group_p, FBE_TRACE_LEVEL_INFO, 
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, "retry mark nr packet: 0x%p iots: 0x%p rl_b: 0x%x\n", 
                          packet_p, in_iots_p, rebuild_logging_bitmask); 

    fbe_raid_group_mark_nr_for_iots(raid_group_p, in_iots_p, (fbe_packet_completion_function_t)fbe_raid_group_mark_nr_for_packet_complete);

    //  Return a raid library status that we are waiting
    return FBE_RAID_STATE_STATUS_WAITING; 

} // End fbe_raid_group_retry_mark_nr_for_packet()

/*!***************************************************************
 * fbe_raid_group_calc_quiesced_count
 ****************************************************************
 * @brief
 *  Mark all items on termination queue as having been quiesced.
 *
 * @param raid_group_p - ptr to raid group object.
 *
 * @return fbe_status_t
 *
 * @author
 *  2/8/2011 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_group_calc_quiesced_count(fbe_raid_group_t * const raid_group_p)
{
    fbe_base_object_t *base_object_p = &raid_group_p->base_config.base_object;
    fbe_queue_head_t *termination_queue_p = &base_object_p->terminator_queue_head;
    fbe_packet_t *current_packet_p = NULL;
    fbe_raid_iots_t *iots_p = NULL;
    fbe_queue_element_t *next_element_p = NULL;
    fbe_queue_element_t *queue_element_p = NULL;
    fbe_u32_t quiesced_count = 0;
    fbe_raid_iots_flags_t iots_flags;

    fbe_raid_group_lock(raid_group_p);
    fbe_spinlock_lock(&base_object_p->terminator_queue_lock);

    queue_element_p = fbe_queue_front(termination_queue_p);

    /* Loop over the termination queue and continue everything.
     * We continue to loop until nothing is left to be continued. 
     */
    while (queue_element_p != NULL)
    {
        fbe_payload_ex_t *sep_payload_p = NULL;

        current_packet_p = fbe_transport_queue_element_to_packet(queue_element_p);
        sep_payload_p = fbe_transport_get_payload_ex(current_packet_p);
        fbe_payload_ex_get_iots(sep_payload_p, &iots_p);
        iots_flags = fbe_raid_iots_get_flags(iots_p);

        next_element_p = fbe_queue_next(termination_queue_p, &current_packet_p->queue_element);

        /* If this I/O was already `was quiesced' we need to clear the flag and
         * decrement the quiesced count.  This is required because the calling 
         * routine will use the count from this method to determine the quiesce
         * count to set.
         */
        if (fbe_raid_iots_is_flag_set(iots_p, FBE_RAID_IOTS_FLAG_WAS_QUIESCED))
        {
            fbe_raid_group_dec_quiesced_count(raid_group_p);
            fbe_raid_group_trace(raid_group_p,
                             FBE_TRACE_LEVEL_INFO,
                             FBE_RAID_GROUP_DEBUG_FLAG_QUIESCE_TRACING,
                             "raid_group: reset was quiesced iots:%8p lba:%016llx blks:%08llx fl:0x%x cnt:%d\n",
                             iots_p, iots_p->lba, iots_p->blocks, iots_flags, raid_group_p->quiesced_count);
            fbe_raid_iots_clear_flag(iots_p, FBE_RAID_IOTS_FLAG_WAS_QUIESCED);
        }

        /* Mark iots with a flag to show it was once quiesced.
         * When the iots completes we will decrement a counter that was setup 
         * when the raid group was originally quiesced. 
         * This allows the monitor to know when all the previously quiesced I/Os have completed. 
         */
        fbe_raid_iots_set_flag(iots_p, FBE_RAID_IOTS_FLAG_WAS_QUIESCED);
        quiesced_count++;

        /* Trace some information if enabled.
         */
        fbe_raid_group_trace(raid_group_p,
                             FBE_TRACE_LEVEL_INFO,
                             FBE_RAID_GROUP_DEBUG_FLAG_QUIESCE_TRACING,
                             "raid_group: set was quiesced iots:%8p lba:%016llx blks:%08llx fl:0x%x cnt:%d\n",
                             iots_p, iots_p->lba, iots_p->blocks, iots_flags, quiesced_count);

        queue_element_p = next_element_p;
    }
    /* Add the quiesced count into the raid group object after the loop.
     */
    fbe_raid_group_update_quiesced_count(raid_group_p, quiesced_count);
    
    fbe_spinlock_unlock(&base_object_p->terminator_queue_lock);
    fbe_raid_group_unlock(raid_group_p);

    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_raid_group_calc_quiesced_count()
 **************************************/

/****************************************************************
 *  fbe_raid_group_quiesce_md_ops
 ****************************************************************
 * @brief
 *  Quiesce all metadata ops so any retrying requests will stop.
 *
 * @param raid_group_p - a pointer to raid group
 *
 * @return
 *  fbe_status_t
 *
 * @author
 *  3/2/2012 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_group_quiesce_md_ops(fbe_raid_group_t * const raid_group_p)
{
    fbe_queue_head_t *termination_queue_p = &raid_group_p->base_config.base_object.terminator_queue_head;
    fbe_packet_t *current_packet_p = NULL;
    fbe_raid_iots_t *iots_p = NULL;
    fbe_queue_element_t *next_element_p = NULL;
    fbe_queue_element_t *queue_element_p = NULL;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_payload_block_operation_opcode_t opcode;

    fbe_raid_group_lock(raid_group_p);

    fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                          FBE_TRACE_LEVEL_DEBUG_LOW,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s fl: 0x%x\n", 
                          __FUNCTION__, raid_group_p->flags);

    fbe_spinlock_lock(&raid_group_p->base_config.base_object.terminator_queue_lock);

    queue_element_p = fbe_queue_front(termination_queue_p);

    while (queue_element_p != NULL)
    {
        fbe_bool_t unused;
        fbe_lba_t lba;
        fbe_payload_ex_t *sep_payload_p = NULL;
        current_packet_p = fbe_transport_queue_element_to_packet(queue_element_p);
        sep_payload_p = fbe_transport_get_payload_ex(current_packet_p);

        next_element_p = fbe_queue_next(termination_queue_p, &current_packet_p->queue_element);

        fbe_payload_ex_get_iots(sep_payload_p, &iots_p);
        fbe_raid_iots_get_lba(iots_p, &lba);
        fbe_raid_iots_get_opcode(iots_p, &opcode);

        /* Add the siots for this iots that need to be restarted. 
         * We will kick them off below once we have finished iterating over 
         * the termination queue. 
         */
        if ((iots_p->status != FBE_RAID_IOTS_STATUS_NOT_USED) &&
            (iots_p->status != FBE_RAID_IOTS_STATUS_NOT_STARTED_TO_LIBRARY) &&
            (iots_p->status != FBE_RAID_IOTS_STATUS_WAITING_FOR_QUIESCE) &&
            !fbe_raid_library_is_opcode_lba_disk_based(opcode) &&
            fbe_raid_geometry_is_metadata_io(raid_geometry_p, lba)) 
        {
            /* This iots was started to the library, mark it quiesced.
             */
            fbe_raid_iots_quiesce_with_lock(iots_p, &unused);
        }
        queue_element_p = next_element_p;
    }

    fbe_spinlock_unlock(&raid_group_p->base_config.base_object.terminator_queue_lock);
    fbe_raid_group_unlock(raid_group_p);
    return FBE_STATUS_OK;
}
/*****************************************
 * fbe_raid_group_quiesce_md_ops()
 *****************************************/

 
/*!****************************************************************************
 * fbe_raid_group_count_ios()
 ******************************************************************************
 * @brief
 *  Traverse the terminator queue and count outstanding I/Os.
 *
 * @param base_config_p - a pointer to the base config.
 * @param stats_p - Statistics structure to return.
 *
 * @return fbe_status_t
 *
 * @author
 *  4/25/2013 - Created. Rob Foley
 ******************************************************************************/
fbe_status_t fbe_raid_group_count_ios(fbe_raid_group_t * const raid_group_p, 
                                      fbe_raid_group_get_statistics_t *stats_p)
{
    fbe_base_config_t *base_config_p = (fbe_base_config_t*)raid_group_p;
    fbe_queue_head_t *      termination_queue_p = NULL;
    fbe_packet_t *          current_packet_p = NULL;
    fbe_queue_element_t *   next_element_p = NULL;
    fbe_raid_iots_t *       iots_p = NULL;
    fbe_queue_element_t *   queue_element_p = NULL;
    fbe_block_transport_get_throttle_info_t throttle_info;
    fbe_u32_t index;
    fbe_payload_ex_t       *sep_payload_p = NULL;

    /* Acquire lock before we look at the termination queue. */
    fbe_spinlock_lock(&base_config_p->base_object.terminator_queue_lock);

    /* Get the head of the termination queue after acquiring lock. */
    termination_queue_p = fbe_base_object_get_terminator_queue_head((fbe_base_object_t *) base_config_p);
    
    /* Loop over the termination queue and break out when we find something that is not quiesced.  */
    queue_element_p = fbe_queue_front(termination_queue_p);
    
    while (queue_element_p != NULL)
    {
        current_packet_p = fbe_transport_queue_element_to_packet(queue_element_p);
        sep_payload_p = fbe_transport_get_payload_ex(current_packet_p);
    
        next_element_p = fbe_queue_next(termination_queue_p, &current_packet_p->queue_element);
    
        fbe_payload_ex_get_iots(sep_payload_p, &iots_p);
    
        /* Verify whether this iots is used or not, if its current status shows
         * that it is not used then we are not using this iots for any i/o operation
         * and so it will not be considered as pending iots which needs to be
         * quiesce.
         */
        if (iots_p->status == FBE_RAID_IOTS_STATUS_AT_LIBRARY)
        {
            fbe_raid_iots_get_statistics(iots_p, &stats_p->raid_library_stats);
        }
        stats_p->total_packets++;
        queue_element_p = next_element_p;
    }
    /* Get the outstanding I/O count.
     */
    fbe_block_transport_server_get_throttle_info(&base_config_p->block_transport_server,
                                                 &throttle_info);
    stats_p->outstanding_io_count = throttle_info.outstanding_io_count;
    stats_p->outstanding_io_credits = throttle_info.outstanding_io_credits;
    for (index = 0; index < FBE_PACKET_PRIORITY_LAST - 1; index++)
    {
        stats_p->queue_length[index] = throttle_info.queue_length[index];
    }
    fbe_spinlock_unlock(&base_config_p->base_object.terminator_queue_lock);

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_raid_group_count_ios()
 ******************************************************************************/

/*!**************************************************************
 * fbe_raid_group_get_quiesced_io()
 ****************************************************************
 * @brief
 *  Get the quiesced usurper packet from the usurper queue.
 *
 * @param raid_group_p - Current RG
 * @param packet_pp - Output ptr to the usurper packet.
 *
 * @return None.
 *
 * @author
 *  10/23/2013 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_raid_group_get_quiesced_io(fbe_raid_group_t *raid_group_p, fbe_packet_t **packet_pp)
{
    fbe_queue_head_t *      usurper_queue_p = NULL;
    fbe_packet_t *          current_packet_p = NULL;
    fbe_queue_element_t *   next_element_p = NULL;
    fbe_queue_element_t *   queue_element_p = NULL;
    fbe_base_config_t *base_config_p = &raid_group_p->base_config;
    fbe_payload_block_operation_t *block_operation_p = NULL;

    /* Acquire lock before we look at the termination queue. */
    fbe_spinlock_lock(&base_config_p->base_object.terminator_queue_lock);
    usurper_queue_p = fbe_base_object_get_usurper_queue_head((fbe_base_object_t *) base_config_p);
    
    /* Loop over the termination queue and break out when we find the first item we are interested in. */
    queue_element_p = fbe_queue_front(usurper_queue_p);
    
    while (queue_element_p != NULL)
    {
        fbe_payload_ex_t * sep_payload_p = NULL;
        current_packet_p = fbe_transport_queue_element_to_packet(queue_element_p);
        sep_payload_p = fbe_transport_get_payload_ex(current_packet_p);
        block_operation_p = fbe_payload_ex_get_block_operation(sep_payload_p);
    
        next_element_p = fbe_queue_next(usurper_queue_p, &current_packet_p->queue_element);
    

        if (block_operation_p->block_count == 0) {
            /* Found the item.
             */
            *packet_pp = current_packet_p;
        }

        queue_element_p = next_element_p;
    }
    fbe_spinlock_unlock(&base_config_p->base_object.terminator_queue_lock);
}
/******************************************
 * end fbe_raid_group_get_quiesced_io()
 ******************************************/

/****************************************************************
 *  fbe_raid_group_drain_usurpers
 ****************************************************************
 * @brief
 *  This function is the handler for a shutdown.
 *
 * @param raid_group_p - a pointer to raid group
 *
 * @return
 *  fbe_status_t
 *
 * @author
 *  10/23/2013 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_group_drain_usurpers(fbe_raid_group_t * const raid_group_p)
{
    fbe_packet_t *usurper_packet_p = NULL;

    /* Dequeue and send back the usurper.
     */
    fbe_raid_group_get_quiesced_io(raid_group_p, &usurper_packet_p);
    if (usurper_packet_p != NULL) {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s: sending back packet %p with status %d\n", __FUNCTION__, usurper_packet_p, FBE_STATUS_FAILED);
        fbe_base_object_remove_from_usurper_queue((fbe_base_object_t*)raid_group_p, usurper_packet_p);
        
        fbe_transport_set_status(usurper_packet_p, FBE_STATUS_FAILED, 0);
        fbe_transport_complete_packet_async(usurper_packet_p);
    }
    return FBE_STATUS_OK;
}
/*****************************************
 * fbe_raid_group_drain_usurpers()
 *****************************************/

/*!***************************************************************
 * fbe_raid_group_is_request_quiesced
 ****************************************************************
 * @brief
 *  Check if a block operation with the specified opcode is on the
 *  terminator queue or not.
 *
 * @param raid_group_p - ptr to raid group object.
 *
 * @return fbe_status_t
 *
 * @author
 *  11/25/2014  Ron Proulx  - Created.
 *
 ****************************************************************/
fbe_bool_t fbe_raid_group_is_request_quiesced(fbe_raid_group_t * const raid_group_p,
                                              fbe_payload_block_operation_opcode_t request_opcode)
{
    fbe_base_object_t *base_object_p = &raid_group_p->base_config.base_object;
    fbe_queue_head_t *termination_queue_p = &base_object_p->terminator_queue_head;
    fbe_packet_t *current_packet_p = NULL;
    fbe_queue_element_t *next_element_p = NULL;
    fbe_queue_element_t *queue_element_p = NULL;
    fbe_payload_ex_t *sep_payload_p = NULL;
    fbe_payload_block_operation_t *block_operation_p = NULL;
    fbe_payload_block_operation_opcode_t block_operation_opcode;

    fbe_raid_group_lock(raid_group_p);
    fbe_spinlock_lock(&base_object_p->terminator_queue_lock);

    queue_element_p = fbe_queue_front(termination_queue_p);

    /* Loop over the termination queue and continue everything.
     * We continue to loop until nothing is left to be continued. 
     */
    while (queue_element_p != NULL)
    {
        current_packet_p = fbe_transport_queue_element_to_packet(queue_element_p);
        sep_payload_p = fbe_transport_get_payload_ex(current_packet_p);
        block_operation_p = fbe_payload_ex_get_present_block_operation(sep_payload_p);

        next_element_p = fbe_queue_next(termination_queue_p, &current_packet_p->queue_element);

        /* Check if this packet has the requested block operation
         */
        if (block_operation_p != NULL) {
            fbe_payload_block_get_opcode(block_operation_p, &block_operation_opcode);
            if (block_operation_opcode == request_opcode) {
                fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "raid_group: request op: %d is on terminator queue\n",
                                  request_opcode);
                fbe_spinlock_unlock(&base_object_p->terminator_queue_lock);
                fbe_raid_group_unlock(raid_group_p);
                return FBE_TRUE;
            }
        }

        queue_element_p = next_element_p;
    }

    /* Request not found
     */
    fbe_spinlock_unlock(&base_object_p->terminator_queue_lock);
    fbe_raid_group_unlock(raid_group_p);

    return FBE_FALSE;
}
/*******************************************
 * end fbe_raid_group_is_request_quiesced()
 ******************************************/


/*************************
 * end file fbe_raid_group_io_quiesce.c
 *************************/




