/***************************************************************************
 * Copyright (C) EMC Corporation 1999-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * fbe_raid_generate.c
 ***************************************************************************
 *
 * @brief
 *   This file contains generic generate functions of the raid group.
 *
 * TABLE OF CONTENTS
 *  fbe_raid_degraded_chkpt
 *  fbe_raid_scheduling_request
 *  fbe_raid_set_degraded_chkpt
 *  fbe_raid_get_siots_lock
 *  fbe_raid_min_rb_offset
 *  fbe_raid_calculate_lock
 *  fbe_raid_calculate_raid10_lba_lock
 *  fbe_raid_expansion_check_lock
 *  fbe_raid_fru_is_a_hotspare
 *  fbe_raid_fru_verify_proactive_spare
 *  fbe_raid_gen_backfill
 *  fbe_raid_get_hs_lun
 *  fbe_raid_get_iots_lock
 *  fbe_raid_get_max_blocks
 *  fbe_raid_iots_lock_granted
 *  fbe_raid_max_transfer_size
 *  fbe_raid_min_rb_offset
 *  fbe_raid_new_sub_request
 *  fbe_raid_start_sm
 *  fbe_raid_state
 *
 * @author
 *   10/2000 Rob Foley Created.
 *
 ***************************************************************************/

/*************************
 **  INCLUDE FILES      **
 *************************/
#include "fbe_raid_library_proto.h"
#include "fbe/fbe_time.h"

/********************************
 *      static MACROS
 ********************************/

/*****************************************************
 * static FUNCTIONS
 *****************************************************/

#if 0
static LOCK_REQUEST *r5_allocate_lock(fbe_raid_siots_t * siots_ptr,
                                     fbe_u32_t blocks_per_stripe,
                                     fbe_u32_t blocks_per_parity_stripe);
static void fbe_raid_calculate_raid10_lba_lock(fbe_raid_iots_t * iots_p,
                             LOCK_REQUEST * lock_ptr);
#endif
/****************************
 *      GLOBAL VARIABLES
 ****************************/
#if 0
/****************************************************************
 * fbe_raid_start_sm()
 ****************************************************************
 * @brief
 *  Allocate and start execution of a nested siots.
 *

 * @param siots_p - ptr to the parent siots
 * @param first_state - first state for this nested siots.
 *
 * @return
 *  This function returns FBE_RAID_STATE_STATUS_WAITING.
 *
 * @author
 *  9/18/99 - Created. Rob Foley
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_raid_start_sm(fbe_raid_siots_t * siots_p,
                              void *(first_state))
{
    /* In the raid driver we only one level of SIOTS nesting.
     * We can only support this many since we only have
     * three levels of priorities for allocating buffers.
     *
     * IOTS               priority = 0
     *  \
     *   SIOTS            priority = 1
     *    \
     *    Nested SIOTS    priority = 2
     *
     * Assert this is not a nested siots, since we are 
     * guaranteed to run into further issues with 3 levels of nesting.
     */   
    if(RAID_COND( RG_SIOTS_NEST(siots_p) ))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "raid: siots_p %p nested\n",
                                            siots_p);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }
    

    if (rg_allocate_nest_siots((fbe_raid_common_t *) siots_p,
                               first_state,
                               siots_p->resource_priority) == FBE_FALSE)
    {
        /* The BM is out of resources.
         * An event will be generated to start this request
         * when a sub_iots is available.
         *
         * We will eventually wake up in first_state
         */
    }
    else
    {
        /* We have retrieved the nested siots.
         * Put this siots on the generate queue, so we can begin executing now.
         */
        fbe_raid_siots_t *new_siots_p;

        new_siots_p = (fbe_raid_siots_t *) fbe_raid_memory_id_get_memory_ptr(siots_p->bm_object_id);

        siots_p->bm_object_id = 0;

        fbe_raid_geometry_enq_stateq(fbe_raid_siots_get_raid_geometry(siots_p), (fbe_raid_common_t *)new_siots_p);

    }
    if (RAID_COND(siots_p->nsiots_q.head != siots_p->nsiots_q.tail))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "raid: siots_p->nsiots_q.head %p != siots_p->nsiots_q.tail %p\n",
                                            siots_p->nsiots_q.head, siots_p->nsiots_q.tail);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }
    return FBE_RAID_STATE_STATUS_WAITING;

}
/*******************************
 * end of fbe_raid_start_sm()
 *******************************/

/****************************************************************
 * fbe_raid_get_iots_lock()
 ****************************************************************
 *
 * @brief
 *  This function calculates the lock area and
 *  attempts to get a lock from the Lock Manager.
 *

 * @param iots_ptr - a pointer to the fbe_raid_iots_t for this operation
 * @param wait - FBE_TRUE if we should wait for a lock,
 *              FBE_FALSE if we should not wait for a lock.
 *
 * @return
 *  FBE_TRUE lock granted
 *  FBE_FALSE otherwise
 *
 * @author
 *   11/3/99  Rob Foley    Created.
 *
 ****************************************************************/

fbe_bool_t fbe_raid_get_iots_lock(fbe_raid_iots_t * iots_p, fbe_bool_t wait)
{
    LOCK_REQUEST *lock_ptr;
    fbe_bool_t status;

    /* Since this routine is called in all cases (i.e. there are some cases like
     * a RG that doesn't have the RG_LOCK_ATTRIBUTE where this routine is still
     * invoked), we must now handle those cases where we the client has already
     * obtained the lock.
     */
    if ( (((RG_IOTS_RGDB(iots_p)->attribute_flags & RG_LOCK_ATTRIBUTE) == 0) &&
          !dba_unit_is_expanding(adm_handle, iots_p->unit)                      ) ||
         (iots_p->iorb.flags & RAID_LOCK_OBTAINED)                                   )
    {
        /* Some units only lock when the unit is expanding.
         * We don't lock if we detect the RAID_EXPANSION flag
         * because the expansion driver already has the lock.
         */
        return FBE_TRUE;
    }

    /* We should never have a lock already.
     */
    if (RAID_COND(iots_p->lock_ptr != NULL))
    {
        fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                            "%s status: iots_p->lock_ptr is NULL  %p",
                            __FUNCTION__, iots_p->lock_ptr);
        return FBE_FALSE;
    }
    if (RAID_COND(fbe_raid_iots_is_flag_set(iots_p, FBE_RAID_IOTS_FLAG_LOCK_GRANTED)) )
    {
        fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                            "%s status: iots_p->flags 0x%x set to  FBE_RAID_IOTS_FLAG_LOCK_GRANTED",
                            __FUNCTION__, iots_p->flags);
        return FBE_FALSE;
    }

    /* First fetch a lock from nlm.
     */
    lock_ptr = iots_p->lock_ptr = nlm_alloc_lock_request();

    /* Set flags to initially NOT try to wait for a lock.
     */
    lock_ptr->flags = LOCK_FLAG_ONLY | LOCK_FLAG_NO_PEND |
        ((iots_p->iorb.opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ) ? LOCK_FLAG_READ : LOCK_FLAG_WRITE);

    lock_ptr->lun = iots_p->unit;
    lock_ptr->request_id = (fbe_u32_t *)iots_p;

    /* Calculate the Lock request.
     */
    fbe_raid_calculate_lock(iots_p, lock_ptr);

    /* Lock the stripe.      We have set LOCK_FLAG_ONLY, so the Lock
     * Manager will only lock the stripe and will *not* send the
     * request to the back end.      If the stripe cannot be locked
     * immediately due to a conflict, the Lock Manager will notify
     * us later when the lock is granted, sending the message we
     * specify.
     */

    if ((status = nlm_start_new_request(lock_ptr)) == FBE_TRUE)
    {
        /* Got a lock.
         */
        fbe_raid_iots_set_flag(iots_p, FBE_RAID_IOTS_FLAG_LOCK_GRANTED);
    }
    else if (wait == FBE_FALSE)
    {
        /* The caller specified to NOT wait for a lock at this time.
         * Free this lock, as we will be trying to get
         * a lock at a later time.
         */
        nlm_dealloc_lock_request(iots_p->lock_ptr);
        iots_p->lock_ptr = NULL;
    }
    else if (wait == FBE_TRUE)
    {
        if (RAID_COND(fbe_raid_iots_is_flag_set(iots_p, FBE_RAID_IOTS_FLAG_LOCK_GRANTED)) )
        {
            fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                "%s status: iots_p->flags 0x%x set to  FBE_RAID_IOTS_FLAG_LOCK_GRANTED",
                                __FUNCTION__, iots_p->flags);
            return FBE_FALSE;
        }

        /* Did not get a lock, but we want to try to wait for a lock
         * fill out msg and try again.
         */
        lock_ptr->message.sender = RG_IOTS_RGDB(iots_p)->vid,
            lock_ptr->message.msg_type = MSG_VD_READY,
            lock_ptr->message.request_id = (fbe_u32_t *) iots_p,
            lock_ptr->message.m_tag.vd_ack.target_vid = RG_IOTS_RGDB(iots_p)->vid,
            lock_ptr->message.m_tag.vd_ack.status = VD_REQUEST_COMPLETE,
            lock_ptr->message.m_tag.vd_ack.dh_code = 0,

            /* We put a line number here so that
             * it will appear in the event log.
             */
            lock_ptr->message.m_tag.vd_ack.data = __LINE__,
            lock_ptr->fd = RG_IOTS_RGDB(iots_p)->vid;

        /* Clear no pend flag so that we may wait for a lock.
         */
        lock_ptr->flags &= ~LOCK_FLAG_NO_PEND;
        status = nlm_start_new_request(lock_ptr);
        fbe_raid_iots_set_flag(iots_p, (status) ? FBE_RAID_IOTS_FLAG_LOCK_GRANTED : FBE_RAID_IOTS_FLAG_WAIT_LOCK);
    }

    return status;

}
/*****************************************
 * end of fbe_raid_get_iots_lock()
 *****************************************/

/****************************************************************
 * fbe_raid_iots_lock_granted()
 ****************************************************************
 * @brief
 *  The lock for this IOTS was just granted.
 *  Try to allocate and start the first siots.
 *

 * @param iots_p - ptr to new request.
 *
 * @return VALUE:
 *   FBE_RAID_STATE_STATUS_WAITING
 *
 * @author
 *   11/3/99 - Created. Rob Foley
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_raid_iots_lock_granted(fbe_raid_iots_t * iots_p)
{
    /****************************************
     * Evaluate the unit to ensure that the
     * geometry has not changed?
     *
     ****************************************/
#ifdef FBE_RAID_DEBUG
    /* Make sure we really have a lock here
     */
    if(RAID_COND( (iots_p->lock_ptr == NULL) ||
                  (iots_p->blocks_remaining == 0))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "raid: lock_ptr %p NULL or blocks_remaining 0x%llx 0\n",
                                            iots_p->lock_ptr,
					    (unsigned long long)iots_p->blocks_remaining);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }
    if (!(fbe_raid_iots_is_any_flag_set(iots_p, (FBE_RAID_IOTS_FLAG_WAIT_LOCK|FBE_RAID_IOTS_FLAG_LOCK_GRANTED))))
    {
        fbe_panic(RG_INVALID_REQUEST, __LINE__);
    }

    if(RAID_COND(fbe_raid_iots_is_any_flag_set(iots_p, FBE_RAID_IOTS_FLAG_LOCK_GRANTED)))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "raid: lock_ptr %p NULL or blocks_remaining 0x%llx 0\n",
                                            iots_p->lock_ptr,
					    (unsigned long long)iots_p->blocks_remaining);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* Make sure this request is (still) on the wait queue.
     */
    rg_validate_waiting_iots(iots_p);
#endif

    if(RAID_COND( (iots_p->blocks_remaining == 0)||
                  (iots_p->lock_ptr == NULL) ) )
    {
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "raid: lock_ptr %p NULL or blocks_remaining 0x%llx 0\n",
                                            iots_p->lock_ptr,
					    (unsigned long long)iots_p->blocks_remaining);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    fbe_raid_iots_clear_flag(iots_p, FBE_RAID_IOTS_FLAG_WAIT_LOCK);

    if (RG_IOTS_ABORTED(iots_p) == FBE_TRUE)
    {
        FLARE_RAID_TRC64((TRC_K_STD,
                      "RAID: lock granted aborted iots_p: %p unit: 0x%x"
                      " lba:%llX bl:0x%x\n",
                      iots_p,
                      iots_p->unit,
                      (unsigned long long)iots_p->iorb.lba,
                      iots_p->iorb.blocks));
        /* The iots was aborted while we were waiting for the lock.
         * Free the lock and cleanup the iots.
         */
        nlm_request_completed(iots_p->lock_ptr);
        iots_p->lock_ptr = NULL;

        /* The request is on the wait queue.  We need to dequeue it
         * from the wait queue before we free it.
         */
        rg_deq_iots_q(RG_IOTS_WAIT_Q(iots_p), iots_p);

        /* Whenever we dequeue from wait queue we must 
         * decrement waiting counts.
         */
        RG_DEC_WAITING_COUNT(iots_p->group);

        /* Whenever we will complete an IOTS that has not finished
         * generating, we need to decrement our generating counts.
         */
        RG_DEC_GENERATING_COUNT(iots_p);

        /* We are done with this IOTS, so we must tell the
         * sender we are done.
         */
        rg_send_error_status(RG_IOTS_RGDB(iots_p),
                             iots_p->iorb.sender_id,
                             iots_p->iorb.request_id,
                             VD_REQUEST_HARD_ERROR);

        /* Since we dequeued and sent status on the IOTS, it can be freed now.
         */
        rg_free_iots(iots_p);
        return FBE_RAID_STATE_STATUS_WAITING;
    }

    if (RG_IOTS_RGDB(iots_p)->unfinished_iots_with_errors > 0)
    {
        /* Error recovery is going on and we might need to give this back.
         */
        RAID_GROUP_DB *rgdb_p = RG_IOTS_RGDB(iots_p);

        fbe_raid_iots_t *current_iots_p = NULL;

        current_iots_p = rgdb_p->active_q.head;

        /* Loop over all the iots that are active, searching for a conflict.
         * We'll free our lock if we find any conflicts.
         */
        while (current_iots_p != NULL)
        {
            if ((current_iots_p != iots_p) &&
                (current_iots_p->flags & FBE_RAID_IOTS_FLAG_ERROR) &&
                ((current_iots_p->iorb.flags & RAID_LOCK_OBTAINED) == 0) &&
                nlm_conflict_check(current_iots_p->lock_ptr, iots_p->lock_ptr))
            {
                /* We need to give this lock back since there 
                 * is a request undergoing error recovery that needs it. 
                 */
                fbe_u32_t old_flags = iots_p->flags;
                nlm_request_completed(iots_p->lock_ptr);
                fbe_raid_iots_clear_flag(iots_p, FBE_RAID_IOTS_FLAG_LOCK_GRANTED);
                iots_p->lock_ptr = NULL;
                FLARE_RAID_TRC64((TRC_K_STD,
                                  "RAID: Lock granted aborted1 iots_p:%p unit: 0x%x"
                                  " lba:%llX bl:0x%x fl:0x%x ofl:0x%x\n",
                                  iots_p,
                                  iots_p->unit,
                                  (unsigned long long)iots_p->iorb.lba,
                                  iots_p->iorb.blocks,
                                  iots_p->flags,
                                  old_flags));
                return FBE_RAID_STATE_STATUS_WAITING;
            } /* end if we need to free the lock. */
            current_iots_p = RG_NEXT_IOTS(current_iots_p);
        } /* while current iots not null */
    }/* end if unfinished_iots_with_errors is set */

    /* Only mark lock granted when we are sure we are not aborted and 
     * we do not need to give back the lock. 
     */
    fbe_raid_iots_set_flag(iots_p, FBE_RAID_IOTS_FLAG_LOCK_GRANTED);

    if (dba_unit_is_expanding(adm_handle, iots_p->unit))
    {
        if (fbe_raid_expansion_check_lock(iots_p) == FBE_RAID_STATE_STATUS_WAITING)
        {
            /* We need to wait for a new lock request, return.
             */
            return FBE_RAID_STATE_STATUS_WAITING;
        }
    }

    RG_IOTS_TRANSITION(iots_p, fbe_raid_new_sub_request);

    /* Check the schedule queues to allow this request to start if
     * it is startable.
     */
    return rg_restart_schedule_queues();

}
/*******************************
 * end fbe_raid_iots_lock_granted()
 *******************************/
#endif

/*!**************************************************************
 * rg_do_siots_conflict()
 ****************************************************************
 * @brief
 *  Determine if there is a conflict between the input siots
 *  and the input start/end pbas.
 *  
 * @param siots_p - The siots to check against the input
 *                  start/end pba.
 * @param raid_geometry_p - The raid geometry database for this siots.
 * @param new_start_pba - The start address to check against the siots.
 * @param new_end_pba - The end address to check against the siots.
 * 
 * @return fbe_bool_t - FBE_TRUE if there is a conflict between the input
 *                     siots and the input start/end pbas.
 *
 * @author
 *  11/25/2008 - Created. Rob Foley
 *
 ****************************************************************/

fbe_bool_t rg_do_siots_conflict(fbe_raid_siots_t * const siots_p,
                                fbe_raid_geometry_t * const raid_geometry_p, 
                                fbe_lba_t new_start_pba, 
                                fbe_lba_t new_end_pba)
{   
    fbe_lba_t curr_start_pba = siots_p->parity_start;
    fbe_lba_t curr_end_pba = siots_p->parity_start + siots_p->parity_count - 1;

    if ((siots_p->algorithm == R5_RB) ||
        (siots_p->algorithm == R5_RD_VR) ||
        (siots_p->algorithm == R5_468_VR) ||
        (siots_p->algorithm == R5_MR3_VR) ||
        (siots_p->algorithm == R5_RCW_VR))
    {
        /* These will possibly modify the parity ranges, so instead look at geo.
         */
        curr_start_pba = siots_p->geo.logical_parity_start;
        curr_end_pba = siots_p->geo.logical_parity_start +
                       siots_p->geo.logical_parity_count - 1;
    }
    
    /* Unfortunately the siots on the queue may require a pre-read also and
     * there can be no overlap since all the pre-read blocks will be modified.
     * We only care about unaligned writes.
     */
    if (fbe_raid_geometry_needs_alignment(raid_geometry_p)) {
        fbe_raid_geometry_align_lock_request(raid_geometry_p,
                                             &curr_start_pba, 
                                             &curr_end_pba);
    }
    /***********************************************************************
     * Due to the fact that we will write the pre-read data required for
     * for unaligned write requests, we cannot allow adjacent, unaligned 
     * requests.  Thus we use the pre-read range to determine the overlap.
     * (2) Refers to the data for new siots. (1) Refers to the data for the
     * currently executing siots. (p) refers to either the pre-read or post-read
     * blocks.
     *          
     *                           Drive X     Drive X
     *                      pba +--------+  +--------+
     *                          |--------|  |        |
     * new(2)siots write start->|d2d2d2d2|  |        |
     *                          |d2d2d2d2|  |        |
     * new(2) siots write end ->|d2d2d2d2|  |        |
     *                          |--------|  |--------|
     * new(2) siots post-read ->|p2p2p2p2|  |p1p1p1p1|<-curr(1) siots pre-read
     *                          |--------|  |--------|
     *                          |        |  |d1d1d1d1|<-curr(1) siots write start
     *                          |        |  |d1d1d1d1|
     *                          |        |  |d1d1d1d1|<-curr(1) siots write end
     *                          |        |  |--------|
     *                          +--------+  +--------+
     *
     * Since both writes will read then write the SAME pre-read
     * range, we cannot let the new write start until the previous
     * write completes since the writes may execute out of order and
     * stale post-read data will be written to the overlapping pre-read
     * range.
     *
     ***********************************************************************/
    if (!((curr_end_pba < new_start_pba) ||
          (new_end_pba < curr_start_pba)    ) )
    {
        /* Conflict in locks.
         * This siots must wait for prior
         * conflicting siots to clear.
         */
        return FBE_TRUE;
    }
    return FBE_FALSE;
}
/**************************************
 * end rg_do_siots_conflict()
 **************************************/
/*!***************************************************************
 * fbe_raid_siots_get_lock()
 ****************************************************************
 * @brief
 *  This inline simply determines which siots lock check to invoke
 *  based on the algorithm.
 *
 * @param siots_p - a pointer to the fbe_raid_siots_t for this operation
 *
 * @return
 *  FBE_TRUE lock granted
 *  FBE_FALSE otherwise
 *
 * @notes Assumes iots lock is held by the caller.
 *
 * @author
 *   10/03/08  RDP    Created.
 *   10/08/08  RDP    Update code to handle minimum pre-read
 *
 ****************************************************************/

fbe_bool_t fbe_raid_siots_get_lock(fbe_raid_siots_t * siots_p)
{
    fbe_raid_geometry_t *raid_geometry_p;
    fbe_lba_t           new_start_pba;
    fbe_lba_t           new_end_pba;
    fbe_bool_t          lock_granted = FBE_TRUE;
    fbe_raid_siots_t   *curr_siots_p;
    fbe_raid_iots_t    *iots_p = fbe_raid_siots_get_iots(siots_p);

    /* Recovery verify operations have priority over normal requests.
     * Therefore if the iots upgrade waiters flag is set and this is 
     * not the upgrade owner, fail the lock request.
     */
    if (fbe_raid_siots_get_iots(siots_p)->flags & FBE_RAID_IOTS_FLAG_UPGRADE_WAITERS)
    {
        fbe_raid_siots_t *nested_siots_p = (fbe_raid_siots_t*)fbe_queue_front(&siots_p->nsiots_q);

        /* Unless this is the upgrade owner don't allow a new lock.
         */
        if ( (nested_siots_p == NULL)                     ||
            !(fbe_raid_siots_is_flag_set(nested_siots_p, FBE_RAID_SIOTS_FLAG_UPGRADE_OWNER) )   )
        {
            /* Lock not granted.
             */
            fbe_raid_siots_log_traffic(siots_p, "Lock not granted 1");
            if (nested_siots_p != NULL)
            {
                fbe_raid_siots_log_traffic(nested_siots_p, "Lock not granted 1 nest");
            }

            fbe_raid_iots_unlock(iots_p);
            return FBE_FALSE;
        }
    }

    /* The starting and ending pbas for the new siots.
     * Note that this routine checks for adjacency for unaligned
     * write requests since adjacent requests write to the same
     * physical block.  We use the bsv routine to pad this request
     * by the pre-read blocks required.
     */
    new_start_pba = siots_p->parity_start;
    new_end_pba = siots_p->parity_start + siots_p->parity_count - 1;
    raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);

    /* If we have a nested siots, then take the largest range between us 
     * and the nested siots range.  This includes taking the min of 
     * the start address and taking the max of the end address. 
     */
    if (fbe_queue_is_empty(&siots_p->nsiots_q) == FBE_FALSE)
    {
        fbe_raid_siots_t *nested_siots_p = (fbe_raid_siots_t*)fbe_queue_front(&siots_p->nsiots_q);
        new_start_pba = FBE_MIN(siots_p->parity_start, nested_siots_p->parity_start);
        new_end_pba = FBE_MAX(new_end_pba, 
                          (nested_siots_p->parity_start + nested_siots_p->xfer_count - 1));
    }
    /* Update the range of blocks to lock so that
     * we account for the alignment blocks.  We assume that
     * this in an unaligned request.
     */

    if (fbe_raid_geometry_needs_alignment(raid_geometry_p)) {
        /* If needed, align to the optimal size.
         */
        fbe_raid_geometry_align_lock_request(raid_geometry_p,
                                             &new_start_pba, 
                                             &new_end_pba);
    } /* end if FBE_RAID_SIOTS_FLAG_UNALIGNED_REQUEST */
    /***************************************************
     * Start with the previous siots and search all
     * previous siots, looking for a conflict.  Note that
     * siots could complete out or order so we need to check
     * the entire queue. If a conflict is found, return FBE_FALSE,
     * otherwise, return FBE_TRUE.
     ***************************************************/
    curr_siots_p = siots_p;

    /* If there are any siots before this one, check for a conflict. 
     * Anything that is waiting for a lock is considered conflicting since they are before
     * us and has priority over us since we want them to finish before us. 
     */
    while ((curr_siots_p = fbe_raid_siots_get_prev(curr_siots_p)) != NULL)
    {
        fbe_raid_siots_t *nested_siots_p;

        /* Check if the current conflicts with the input siots.
         */
        if ( rg_do_siots_conflict(curr_siots_p, 
                                  raid_geometry_p,
                                  new_start_pba, 
                                  new_end_pba) )
        {
            lock_granted = FBE_FALSE;
            fbe_raid_siots_log_traffic(siots_p, "Lock not granted 2");
            fbe_raid_siots_log_traffic(curr_siots_p, "Lock not granted 2 current");
            break;
        }

        /* Check if anything nested conflicts with the input siots.
         * Note that we on purpose pass in the curr_siots_p, not 
         * the nested siots ptr.  This is because the nested 
         * siots will often modify the parity start/count in order 
         * to do strip mining and thus we cannot always rely on it. 
         */
        nested_siots_p = (fbe_raid_siots_t*)fbe_queue_front(&curr_siots_p->nsiots_q);

        if ( (nested_siots_p != NULL) &&
             rg_do_siots_conflict(nested_siots_p,
                                  raid_geometry_p, 
                                  new_start_pba, 
                                  new_end_pba) )
        {
            lock_granted = FBE_FALSE;
            fbe_raid_siots_log_traffic(siots_p, "Lock not granted 3");
            fbe_raid_siots_log_traffic(nested_siots_p, "Lock not granted 3 nest");
            break;
        }
    } /* end while previous siots */

    curr_siots_p = siots_p;

    /* If there are any siots after this one, check for a conflict. 
     * Anything that is waiting for a lock is not considered conflicting since 
     * they are not actually running. 
     */
    while ((curr_siots_p = fbe_raid_siots_get_next(curr_siots_p)) != NULL)
    {
        fbe_raid_siots_t *nested_siots_p;
        fbe_bool_t b_is_waiting;

        nested_siots_p = (fbe_raid_siots_t*)fbe_queue_front(&curr_siots_p->nsiots_q);

        /* Check if the siots or nested siots is waiting.
         */
        if ( fbe_raid_siots_is_any_flag_set(curr_siots_p, (FBE_RAID_SIOTS_FLAG_WAIT_LOCK | FBE_RAID_SIOTS_FLAG_WAIT_UPGRADE)) ||
             ((nested_siots_p != NULL) &&
              fbe_raid_siots_is_any_flag_set(nested_siots_p, (FBE_RAID_SIOTS_FLAG_WAIT_LOCK | FBE_RAID_SIOTS_FLAG_WAIT_UPGRADE))) )
        {
            b_is_waiting = FBE_TRUE;
        }
        else
        {
            b_is_waiting = FBE_FALSE;
        }


        /* Check if the current conflicts with the input siots.
         */
        if ( (b_is_waiting == FBE_FALSE) &&
             rg_do_siots_conflict(curr_siots_p,
                                  raid_geometry_p, 
                                  new_start_pba, 
                                  new_end_pba) )
        {
            lock_granted = FBE_FALSE;
            fbe_raid_siots_log_traffic(siots_p, "Lock not granted 4");
            fbe_raid_siots_log_traffic(curr_siots_p, "Lock not granted 4 current");
            break;
        }

        /* Check if anything nested conflicts with the input siots.
         * Note that we on purpose pass in the curr_siots_p, not 
         * the nested siots ptr.  This is because the nested 
         * siots will often modify the parity start/count in order 
         * to do strip mining and thus we cannot always rely on it. 
         */
        if ( (nested_siots_p != NULL) &&
             (b_is_waiting == FBE_FALSE) &&
             rg_do_siots_conflict(nested_siots_p,
                                  raid_geometry_p, 
                                  new_start_pba, 
                                  new_end_pba) )
        {
            lock_granted = FBE_FALSE;
            fbe_raid_siots_log_traffic(siots_p, "Lock not granted 5");
            fbe_raid_siots_log_traffic(curr_siots_p, "Lock not granted 5 current");
            break;
        }
    } /* end while next siots */

    /* If there are any siots after this one, check for a conflict. 
     * Anything that is waiting for a lock is not considered conflicting since 
     * they are not actually running. 
     */
    curr_siots_p = siots_p;
    return lock_granted;
}
/*****************************************
 * end of fbe_raid_siots_get_lock()
 *****************************************/

/*!**************************************************************
 * fbe_raid_siots_get_first_lock_waiter()
 ****************************************************************
 * @brief
 *  Restart any waiting siots.
 *  We start at the head to make sure that any requests prior to
 *  us have a chance to finish before requests after us. This helps
 *  in cases where the nested siots range has become bigger than the
 *  parent siots range.
 *  
 * @param siots_p - The siots to restart for.
 * @param restart_siots_p - ptr ptr to the siots to restart.
 *                          This gets returned as NULL if nothing needs
 *                          to get restarted.
 * 
 * @notes  We should not call this function if the iots is quiescing.
 *         If the iots is quiescing we should mark the siots as
 *         quiescing and not call this function.
 * 
 *         This function needs to be called with the iots lock held.
 *
 * @return - FBE_TRUE if a siots needs to be reactivated.
 *           FBE_FALSE if nothing needs to be re-activated.
 * @author
 *  11/25/2008 - Created from RG_RESTART_NEXT_SIOT. Rob Foley
 *
 ****************************************************************/

fbe_bool_t fbe_raid_siots_get_first_lock_waiter(fbe_raid_siots_t *const siots_p,
                                               fbe_raid_siots_t ** restart_siots_p)
{
    fbe_raid_siots_t *next_siots_p = NULL;
    fbe_raid_iots_t *iots_p = NULL;
    fbe_raid_siots_t *nest_siots_p = NULL;

    iots_p = fbe_raid_siots_get_iots(siots_p);

    /* Siots waiting for an upgrade have priority over new siots waiting
     * for a lock.  Therefore check and start any upgrade waiters first.
     */
    if (fbe_raid_siots_restart_upgrade_waiter(siots_p, restart_siots_p))
    {
        /* We return false since no siots waiting for a lock was
         * re-activated. We don't clear the FBE_RAID_SIOTS_FLAG_WAIT_UPGRADE flag since
         * it will be checked and handled later.
         */
        return(FBE_FALSE);
    }
    next_siots_p = (fbe_raid_siots_t*)fbe_queue_front(&iots_p->siots_queue);

    /* Find first siots waiting for siots lock.
     */
    while (next_siots_p)
    {
        if (((next_siots_p)->flags & FBE_RAID_SIOTS_FLAG_WAIT_LOCK) &&
            fbe_raid_siots_is_startable(next_siots_p))
        {
            fbe_raid_siots_clear_flag(next_siots_p, FBE_RAID_SIOTS_FLAG_WAIT_LOCK);
            fbe_raid_siots_log_traffic(next_siots_p, "restart siots wait");
            *restart_siots_p = next_siots_p;
            return FBE_TRUE;
        }

        nest_siots_p = (fbe_raid_siots_t*) fbe_queue_front(&next_siots_p->nsiots_q);

        /* Also try to kick off the nested siots if needed.
         */
        if ( (nest_siots_p != NULL) &&
             ((nest_siots_p)->flags & FBE_RAID_SIOTS_FLAG_WAIT_LOCK) &&
             fbe_raid_siots_is_startable(next_siots_p) )
        {
            fbe_raid_siots_clear_flag(nest_siots_p, FBE_RAID_SIOTS_FLAG_WAIT_LOCK);
            fbe_raid_siots_log_traffic(nest_siots_p, "restart nest siots wait");
            *restart_siots_p = nest_siots_p;
            return FBE_TRUE;
        }
        next_siots_p = fbe_raid_siots_get_next(next_siots_p);
    }
    return FBE_FALSE;
}
/**************************************
 * end fbe_raid_siots_get_first_lock_waiter()
 **************************************/

/*!*******************************************************************
 * @var fbe_raid_group_get_nr_extent_mock
 *********************************************************************
 * @brief Global that is used to mock out the call to fbe_raid_get_nr_extent.
 *
 *********************************************************************/
fbe_raid_group_get_nr_extent_fn_t fbe_raid_group_get_nr_extent_mock = NULL;

/*!**************************************************************
 * fbe_raid_group_set_nr_extent_mock()
 ****************************************************************
 * @brief Allows setting the mock for fbe_raid_get_nr_extent.
 *
 * @param mock - the function to set as the mock.
 *
 * @return - none
 *
 ****************************************************************/

void fbe_raid_group_set_nr_extent_mock(fbe_raid_group_get_nr_extent_fn_t mock)
{
    fbe_raid_group_get_nr_extent_mock = mock;
    return;
}
/******************************************
 * end fbe_raid_set_nr_extent_mock()
 ******************************************/
/*!**************************************************************
 * fbe_raid_generate_find_end_nr_extent()
 ****************************************************************
 * @brief
 *  
 *
 * @param  -               
 *
 * @return -   
 *
 * @author
 *  11/30/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_generate_find_end_nr_extent(fbe_raid_iots_t *iots_p,
                                                  fbe_u32_t pos,
                                                  fbe_lba_t lba,
                                                  fbe_block_count_t blocks,
                                                  fbe_block_count_t *blocks_remaining_p,
                                                  fbe_bool_t *b_dirty)
{
    fbe_raid_group_paged_metadata_t *current_chunk_p = NULL;
    fbe_status_t status;
    fbe_lba_t current_extent_lba;
    fbe_chunk_size_t chunk_size;
    fbe_u32_t chunk_index;

    fbe_raid_iots_get_chunk_size(iots_p, &chunk_size);

    status = fbe_raid_iots_get_chunk_info(iots_p, 0, &current_chunk_p);
    if (status != FBE_STATUS_OK) { return status; }

    /* Find the extent that contains this lba.
     */
    for (chunk_index = 0; chunk_index < FBE_RAID_IOTS_MAX_CHUNKS; chunk_index++)
    {
        /* This extent's lba is an offset in multiples of chunk size 
         * from the starting chunk lba. 
         */
        current_extent_lba = iots_p->start_chunk_lba + (chunk_index * iots_p->chunk_size);

        if ((lba >= current_extent_lba) && 
            (lba < (current_extent_lba + chunk_size)))
        {
            /* Blocks remaining are either the same as the 
             * input blocks or the distance to the end of this region. 
             */
            *blocks_remaining_p = FBE_MIN(blocks,
                                                                  (chunk_size - (lba - current_extent_lba)));
            /* Just check if this extent is dirty or not by checking the rebuild bit 
             * for the entry we are interested in. 
             */
            *b_dirty = (current_chunk_p->needs_rebuild_bits & (1 << pos)) != 0;
            return FBE_STATUS_OK;
        }
        current_chunk_p++;
    }
    return FBE_STATUS_GENERIC_FAILURE;
}
/******************************************
 * end fbe_raid_generate_find_end_nr_extent()
 ******************************************/
/*!**************************************************************
 * fbe_raid_generate_get_nr_extents()
 ****************************************************************
 * @brief
 *  Fetch the array of nr_extents for this given iots.
 *
 * @param iots_p - Current I/O
 * @param position - Position in array to mark.
 * @param lba - Current lba.
 * @param blocks - Current blocks.
 * @param nr_extent_p - Ptr to array to update.
 * @param nr_extents - Number of extents in array.
 * 
 * @return fbe_status_t
 *
 * @author
 *  11/30/2009 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_raid_generate_get_nr_extents(fbe_raid_iots_t *iots_p,
                                              fbe_u32_t position,
                                              fbe_lba_t lba,
                                              fbe_block_count_t blocks,
                                              fbe_raid_nr_extent_t *nr_extent_p,
                                              fbe_u32_t nr_extents)
{
    fbe_lba_t current_lba = lba;
    fbe_block_count_t current_blocks = blocks;
    fbe_u32_t current_extent = 0;
    fbe_bool_t b_dirty;
    fbe_block_count_t blocks_remaining;
    fbe_status_t status;

    fbe_zero_memory(nr_extent_p, (sizeof(fbe_raid_nr_extent_t) * nr_extents));

    /* Find this lba range in the nr extent.
     */
    while ((current_lba < (lba + blocks) && (current_extent < nr_extents)))
    {
        /* Get the end of this extent.  End is where a dirty/clear or clean/dirty
         * transition occurs. 
         */
        status = fbe_raid_generate_find_end_nr_extent(iots_p,
                                                      position,
                                                      current_lba,
                                                      current_blocks,
                                                      &blocks_remaining,
                                                      &b_dirty);
        if (status != FBE_STATUS_OK) { return status; }

        nr_extent_p->lba = current_lba;
        if(blocks_remaining > FBE_U32_MAX)
        {
            /* the blocks remaining should be within 32bit limit here
             */
             return FBE_STATUS_GENERIC_FAILURE;
        }
        nr_extent_p->blocks = (fbe_u32_t)blocks_remaining;
        nr_extent_p->b_dirty = b_dirty;
        nr_extent_p++;
        current_extent++;
        current_blocks -= blocks_remaining;
        current_lba += blocks_remaining;
    }
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_raid_generate_get_nr_extents()
 ******************************************/

/*!**************************************************************
 * fbe_raid_iots_get_nr_extent()
 ****************************************************************
 * @brief
 *  Fetch an NR extent for a given lba range.
 * 
 * @param iots_p - current io.
 * @param position - current position in the group.
 * @param lba - first block of region to get extents for.
 * @param blocks - total blocks in the region.
 * @param extent_p - ptr to extent array to fill out.
 *                   length of this array is determined by the max_extents param
 * @param max_extents - number of entries in the above extent array.
 *
 * @return fbe_status_t
 *
 * @author
 *  11/3/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_iots_get_nr_extent(fbe_raid_iots_t *iots_p,
                                         fbe_u32_t position,
                                         fbe_lba_t lba, 
                                         fbe_block_count_t blocks,
                                         fbe_raid_nr_extent_t *extent_p,
                                         fbe_u32_t max_extents)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    /* If we have a mock already set, we will use it.
     */
    if (fbe_raid_group_get_nr_extent_mock != NULL)
    {
        return(fbe_raid_group_get_nr_extent_mock)(iots_p,
                                                  position,
                                                  lba, blocks, extent_p, max_extents);
    }

    /*! Determine the contents of the nr extent from the map.  
     */
    status = fbe_raid_generate_get_nr_extents(iots_p, position, lba, blocks, extent_p, max_extents);

    return status;
}
/******************************************
 * end fbe_raid_iots_get_nr_extent()
 ******************************************/
/*!**************************************************************
 * fbe_raid_extent_is_nr()
 ****************************************************************
 * @brief
 *  Determine if the given range is marked NR.
 *
 * @param lba - first block of region to get extents for.
 * @param blocks - total blocks in the region.
 * @param extent_p - ptr to extent array to fill out.
 *                   length of this array is determined by the max_extents param
 * @param max_extents - number of entries in the above extent array.
 * @param b_is_nr - ptr to boolean indicating if this extent is nr.
 * @param blocks_p - The blocks until we hit a transition between clean/dirty.
 *                   This is always less than or equal to the passed in blocks.
 *
 * @return fbe_status_t
 *
 * @author
 *  11/3/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_extent_is_nr(fbe_lba_t lba, 
                                   fbe_block_count_t blocks,
                                   fbe_raid_nr_extent_t *extent_p,
                                   fbe_u32_t max_extents,
                                   fbe_bool_t *b_is_nr,
                                   fbe_block_count_t *blocks_p)
{
    fbe_u32_t index;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    /* Find the extent that contains this lba. 
     * Return the amount this is degraded in the input range. 
     */
    for ( index = 0; index < max_extents; index++)
    {
        if ((lba >= extent_p[index].lba) &&
            (lba < (extent_p[index].lba + extent_p[index].blocks)))
        {
            fbe_lba_t end_lba = (extent_p[index].lba + extent_p[index].blocks);
            /* Lba resides in this extent.
             * Calculate the distance from here to the end of the extent that has 
             * this nr value. 
             */
            *b_is_nr = extent_p[index].b_dirty;
            *blocks_p = FBE_MIN(blocks, (end_lba - lba));
            status = FBE_STATUS_OK;
            break;
        }
    }
    return status;
}
/******************************************
 * end fbe_raid_extent_is_nr()
 ******************************************/

/*!**************************************************************
 * fbe_raid_extent_find_next_nr()
 ****************************************************************
 * @brief
 *  Find the next nr range within the given range.
 *
 * @param lba - first block of region to get extents for.
 * @param blocks - total blocks in the region.
 * @param extent_p - ptr to extent array to fill out.
 *                   length of this array is determined by the max_extents param
 * @param max_extents - number of entries in the above extent array.
 * @param b_is_nr - ptr to boolean indicating if any blocks in the
 *                  passed in extent are nr.
 *
 * @return fbe_status_t
 *
 * @author
 *  11/4/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_extent_find_next_nr(fbe_lba_t lba, 
                                          fbe_u32_t blocks,
                                          fbe_raid_nr_extent_t *extent_p,
                                          fbe_u32_t max_extents,
                                          fbe_bool_t *b_is_nr)
{
    fbe_u32_t index;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_lba_t end_lba = lba + blocks - 1;

    /* Find the extent that contains this lba. 
     * Return the amount this is degraded in the input range. 
     */
    for ( index = 0; index < max_extents; index++)
    {
        fbe_lba_t current_end = extent_p[index].lba + extent_p[index].blocks - 1;
        /* If the ranges overlap and this extent is dirty, then we found a match. 
         */ 
        if ((end_lba >= extent_p[index].lba) &&
            (lba <= current_end) &&
            extent_p[index].b_dirty)
        {
            /* Lba resides in this extent.
             * Calculate the distance from here to the end of the extent that has 
             * this nr value. 
             */
            *b_is_nr = FBE_TRUE;
            status = FBE_STATUS_OK;
            break;
        }
    }
    return status;
}
/******************************************
 * end fbe_raid_extent_find_next_nr()
 ******************************************/

/*!**************************************************************
 * fbe_raid_extent_find_next_nr()
 ****************************************************************
 
 * @brief   Find the first nr range within the given request.
 *
 * @param lba - first block of request to get NR range for
 * @param blocks - total blocks in the request
 * @param extent_p - ptr to nr array that has already been populated
 *                   length of this array is determined by the max_extents param
 * @param max_extents - number of entries in the above extent array.
 * @param nr_start_lba_p - Pointer to starting lba of nr extent (if b_is_nr)
 * @param nr_blocks_p - Pointer to nr blocks (or remaining blocks in request)
 *                      (if b_is_nr)
 * @param b_is_nr - ptr to boolean indicating if any blocks in the
 *                  passed range nr
 *
 * @return fbe_status_t
 *
 * @author
 *  01/04/2009  Ron Proulx  - Created from fbe_raid_extent_find_next_nr
 *
 ****************************************************************/

fbe_status_t fbe_raid_extent_get_first_nr(fbe_lba_t lba, 
                                          fbe_block_count_t blocks,
                                          fbe_raid_nr_extent_t *extent_p,
                                          fbe_u32_t max_extents,
                                          fbe_lba_t *nr_start_lba_p,
                                          fbe_block_count_t *nr_blocks_p,
                                          fbe_bool_t *b_is_nr)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t index;
    fbe_lba_t end_lba = lba + blocks - 1;

    /* Find the extent that contains this lba. 
     * Return the amount this is degraded in the input range. 
     */
    *b_is_nr = FBE_FALSE;
    for ( index = 0; index < max_extents; index++)
    {
        fbe_lba_t current_end = extent_p[index].lba + extent_p[index].blocks - 1;
        /* If the ranges overlap and this extent is dirty, then we found a match. 
         */ 
        if ((end_lba >= extent_p[index].lba) &&
            (lba <= current_end) &&
            extent_p[index].b_dirty)
        {
            /* Request resides in this dirty extent.
             * We have already calculated the `remaining' blocks 
             */
            *nr_start_lba_p = extent_p[index].lba;
            *nr_blocks_p = extent_p[index].blocks;
            *b_is_nr = FBE_TRUE;
            break;
        }
    }

    /* Always return status.
     */
    return(status);
}
/******************************************
 * end fbe_raid_extent_get_first_nr()
 ******************************************/

/*!**************************************************************
 * fbe_raid_generate_align_io_end_reduce()
 ****************************************************************
 *
 * @brief   Reduce the request since so that the end if the request
 *          is aligned.
 *
 * @param   alignment_blocks - Number of blocks to align end to
 * @param   start_lba - Stating lba of request
 * @param   blocks_p - Address of block count to update (if required)
 *
 * @return fbe_status_t
 *
 * @author
 *  07/24/2014  Ron Proulx  - Created.
 *
 ****************************************************************/
fbe_status_t fbe_raid_generate_align_io_end_reduce(fbe_block_size_t alignment_blocks,
                                                   fbe_lba_t start_lba,
                                                   fbe_block_count_t * const blocks_p)
{
    fbe_lba_t   end_remainder = (start_lba + *blocks_p) % alignment_blocks;

    if (end_remainder > 0) {
        if (*blocks_p <= end_remainder) {
            fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                               "raid: align: %d end start: 0x%llx blocks: 0x%llx too small\n",
                               alignment_blocks, start_lba, *blocks_p);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        *blocks_p -= end_remainder;
    }

    return FBE_STATUS_OK;
}
/*********************************************
 * end fbe_raid_generate_align_io_end_reduce()
 *********************************************/

#if 0
/*!**************************************************************
 * @fn fbe_raid_iots_abort_conflicting_waitersfbe_raid_iots_t *const iots_p)
 ****************************************************************
 * @brief
 *   Abort any iots that is conflicting with this request
 *   and is stuck on the wait queue.
 *  
 * @param iots_p - This is the iots we will check for conflicts.
 *
 * @return - None.
 *
 * @author
 *  1/19/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t  fbe_raid_iots_abort_conflicting_waiters(fbe_raid_iots_t *const iots_p)
{
    fbe_raid_group_t *rgdb_p = fbe_raid_iots_get_raid_group(iots_p);

    fbe_raid_iots_t *current_iots_p = NULL;

    current_iots_p = rgdb_p->wait_q.head;

    /* Loop over all the iots waiting. 
     * We'll try to abort the locks for any that conflict with 
     * the iots we are running. 
     */
    while (current_iots_p != NULL)
    {
        /* If the unit matches, and the lock was granted,
         * then check to see if there is a match. 
         */
        if ((current_iots_p != iots_p) &&
            (current_iots_p->unit == iots_p->unit))
        {
            if (current_iots_p->flags & FBE_RAID_IOTS_FLAG_LOCK_GRANTED)
            {
                if(RAID_COND( ( current_iots_p->lock_ptr == NULL ) ||
                              ( iots_p->lock_ptr == NULL )) )
                {
                    fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                        "%s status: current_iots_p->lock_ptr %p or iots_p->lock_ptr %p NULL\n",
                                        __FUNCTION__, current_iots_p->lock_ptr, iots_p->lock_ptr);
                    return FBE_STATUS_GENERIC_FAILURE;
                }
               
                if (nlm_conflict_check(current_iots_p->lock_ptr, iots_p->lock_ptr))
                {
                    /* We need to give this lock back since there 
                     * is a request undergoing error recovery that needs it. 
                     */
                    nlm_request_completed(current_iots_p->lock_ptr);
                    current_iots_p->flags &= ~FBE_RAID_IOTS_FLAG_LOCK_GRANTED;
                    current_iots_p->lock_ptr = NULL;
                    FLARE_RAID_TRC64((TRC_K_STD,
                                      "RAID: Lock aborted unit: 0x%x"
                                      " lba:%llX bl:0x%x\n",
                                      current_iots_p->unit,
                                      current_iots_p->iorb.lba,
                                      current_iots_p->iorb.blocks));
                }
            }/* end if FBE_RAID_IOTS_FLAG_LOCK_GRANTED */
        } /* end if iots is not input iots and unit matches */
        current_iots_p = RG_NEXT_IOTS(current_iots_p);
    }
    return;
}
/**************************************
 * end fbe_raid_iots_abort_conflicting_waiters)
 **************************************/

/**********************************************************************
 * rg_bsv_generate_aligned_iots_lock()
 **********************************************************************
 * @brief
 *             This will align the lock pointer as per the pre-reads 
 *             required.
 *             We are handling R5 and R3 cases here for aligning the 
 *             lock pointer to the multiple of optimal block size
 *

 * @param iots_p - RAID IOTS pointer for the given request
 * @param lock_ptr - LOCK REQUEST pointer for the given IOTS
 *
 * @return
 *        NONE
 *
 * @author
 *       08/22/08 - code shifted from 'fbe_raid_calculate_lock',
 *                  created by Ron Proulx 
 *       01/15/09 - modified for r5 corrupt crc - Mahesh Agarkar
 *
 ***********************************************************************/
void rg_bsv_generate_aligned_iots_lock(fbe_raid_iots_t * iots_p,
                                        LOCK_REQUEST * lock_ptr)
{
    fbe_raid_group_t  *rgdb_p = fbe_raid_siots_get_raid_geometry(iots_p);
    fbe_u16_t opcode = iots_p->iorb.opcode;


    /* for R5/R3 Corrupt CRC request we round the lock request 
     * to the multiple of optimal block size
     */
    if (((opcode == IORB_OP_CORRUPT_CRC) ||
        (opcode == IORB_OP_CORRUPT_DATA)) &&
        ((RG_GET_RGDB(iots_p->group)->unit_type == RG_RAID5)))
    {
        if(lock_ptr->flags == LOCK_FLAG_HOLE)
        {
            raid_generate_aligned_request((fbe_lba_t *)&lock_ptr->start,
                                          (fbe_lba_t *)&lock_ptr->mid_end,
                                           rgdb_p->optimal_block_size);
            raid_generate_aligned_request((fbe_lba_t *)&lock_ptr->mid_start, 
                                          (fbe_lba_t *)&lock_ptr->end,
                                           rgdb_p->optimal_block_size);
        }
        else
        {
            raid_generate_aligned_request((fbe_lba_t *)&lock_ptr->start,
                                          (fbe_lba_t *)&lock_ptr->end,
                                           rgdb_p->optimal_block_size);
        }
    }/*end   if (((opcode == IORB_OP_CORRUPT_CRC) ||...)*/

    else
    {
        /* Update the range of blocks to lock so that
         * we account for the alignment blocks.
         */
        if(lock_ptr->flags == LOCK_FLAG_HOLE)
        {
            fbe_raid_bsv_align_lock_request(rgdb_p->exported_block_size,
                                      rgdb_p->imported_block_size,
                                      rgdb_p->optimal_block_size,
                                      (fbe_lba_t *)&lock_ptr->start, 
                                      (fbe_lba_t *)&lock_ptr->mid_end);
            fbe_raid_bsv_align_lock_request(rgdb_p->exported_block_size,
                                      rgdb_p->imported_block_size,
                                      rgdb_p->optimal_block_size,
                                      (fbe_lba_t *)&lock_ptr->mid_start, 
                                      (fbe_lba_t *)&lock_ptr->end);
        }
        else
        {
            /* Update the range of blocks to lock so that
             * we account for the alignment blocks.
             */
            fbe_raid_bsv_align_lock_request(rgdb_p->exported_block_size,
                                      rgdb_p->imported_block_size,
                                      rgdb_p->optimal_block_size,
                                      (fbe_lba_t *)&lock_ptr->start, 
                                      (fbe_lba_t *)&lock_ptr->end);
        }
    }/* end else if (((opcode == IORB_OP_CORRUPT_CRC) ||...) */

    return;
}                   /* end rg_bsv_generate_aligned_iots_lock() */


/*!***************************************************************
 * @fn  fbe_raid_calculate_lock (fbe_raid_iots_t * iots_p, LOCK_REQUEST * lock_ptr)
 ****************************************************************
 * @brief
 *  This function use the iots range and the passed lock pointer to generate
 *  the starting and ending range in the lock structure.  It also adjust
 *  the lock to take into account any BSV (i.e. aligned to optimal block size)
 *  requirements.  Lastly there are certain RAID group types (stripped mirrors)
 *  that use the logical reqeuest to set the range.  Most RAID group types
 *  us the physical address range across the stripe.
 *
 * @param   iots_p, [I] - a pointer to the fbe_raid_iots_t for this operation
 * @param   lock_ptr, [I] - a pointer to the lock request.
 *
 * @return
 *  void
 *
 * @author
 *   9/28/00  kls    Created.
 *
 ****************************************************************/

fbe_status_t fbe_raid_calculate_lock(fbe_raid_iots_t * iots_p, LOCK_REQUEST * lock_ptr)
{
    fbe_u16_t opcode = iots_p->iorb.opcode;


    if ((opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_REBUILD) ||
        (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ_REMAP) ||
        (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY) ||
        (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_BLOCKS))
    {
        /* Rebuild, verify and read and remap operations pass in a physical address.
         */
        if(RAID_COND(iots_p->iorb.blocks == 0) )
        {
            fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                "%s status: iots_p->iorb.blocks 0x%x == 0",
                                __FUNCTION__, iots_p->iorb.blocks);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        if (fbe_striper_is_mirror_group(fbe_raid_siots_get_raid_geometry(iots_p)))
        {
            /* To optimize the I/O performance for striped
             * mirror group, we acquire the lock on the basis of LBA. To 
             * avoid the discrepancy in the lock calculation between 
             * external I/O and internal I/O (rebuild/verify/remap), 
             * convert the physical extent to logical extent. And acquire 
             * the lock on the basis of logical extent.
             */
            fbe_raid_calculate_raid10_lba_lock(iots_p, lock_ptr);

            /* Validate the sanity of lock.
             */
            if(RAID_COND(lock_ptr->end < lock_ptr->start) )
            {
                fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                    "%s status: lock_ptr->end 0x%llx < lock_ptr->start 0x%llx",
                                    __FUNCTION__,
				    (unsigned long long)lock_ptr->end, 
				    (unsigned long long)ock_ptr->start);
                return FBE_STATUS_GENERIC_FAILURE;
            }
        }
        else
        {
            /* For non-striped mirror group, we acuire lock on physical 
             * extent for internal as well as external I/O.
             */
            lock_ptr->start = iots_p->iorb.lba;
            lock_ptr->end = iots_p->iorb.lba + iots_p->iorb.blocks - 1;
        }/* end else - If (fbe_striper_is_mirror_group()) */
    
    } /* end if physical operation */
    else 
    {
        /* Validate sanity of the opcode
         */
        if(RAID_COND((opcode != FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ) &&
               (opcode != FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE) &&
               (opcode != IORB_OP_CORRUPT_CRC) &&
               (opcode != IORB_OP_CORRUPT_DATA) &&
               (opcode != FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ZERO) && (opcode != FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_UNMARK_ZERO)) )
        {
            fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                "%s status: opcode 0x%x unexpected",
                                __FUNCTION__, opcode);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        if (fbe_striper_is_mirror_group(fbe_raid_siots_get_raid_geometry(iots_p)))
        {
            /* To optimize the performance for striped mirror group,
             * acquire the lock on the basis of logical block address
             * for this I/O.
             */
            lock_ptr->start = iots_p->iorb.lba;
            lock_ptr->end = iots_p->iorb.lba + iots_p->iorb.blocks - 1;
         }
        else
        {
            /* Calculate the parity range of this request
             */
            fbe_raid_extent_t parity_range[2];
            fbe_u16_t flags = 0;

            if (iots_p->iorb.flags & RAID_EXPANSION)
            {
                flags |= LU_RAID_EXPANSION;
            }

            /* Call appropriate lock calculation routine
             * Uses function table in RGDB to determine calculate parity
             * range function.
             */
            getLunStripeRange(iots_p->unit,
                              iots_p->iorb.lba,
                              iots_p->iorb.blocks,
                              parity_range,
                              flags);

            /* Fill out the lock with the stripe range values
             * returned by getLunStripeRange.
             */
            if (parity_range[1].size != 0)
            {
                /* A noncontinuous lock, so we set mid_end, mid_start and 
                 * the hole flag.
                 */
                lock_ptr->start = parity_range[0].start_lba;
                lock_ptr->mid_end = parity_range[0].start_lba + 
                                    parity_range[0].size - 1;
                lock_ptr->mid_start = parity_range[1].start_lba;
                lock_ptr->end = parity_range[1].start_lba + 
                                parity_range[1].size - 1;
                lock_ptr->flags |= LOCK_FLAG_HOLE;

                /* Validate parity ranges.  The ranges cannot overlap.
                 */
                if(RAID_COND( (lock_ptr->end <= lock_ptr->start) ||
                              (lock_ptr->mid_end >= lock_ptr->mid_start) ))
                {
                    fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                          "(lock_ptr->end 0x%llx <= lock_ptr->start 0x%llx) OR"
                          "(lock_ptr->mid_end 0x%llx>= lock_ptr->mid_start 0x%llx) ",
                          (unsigned long long)lock_ptr->end,
			  (unsigned long long)lock_ptr->start,
			  (unsigned long long)lock_ptr->mid_end,
			  (unsigned long long)lock_ptr->mid_start);
                    return FBE_STATUS_GENERIC_FAILURE;
                }
                if(RAID_COND((lock_ptr->mid_end < lock_ptr->start) ||
                             (lock_ptr->end < lock_ptr->mid_start)))
                {
                    fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                          "(lock_ptr->mid_end 0x%llx < lock_ptr->start 0x%llx) OR"
                          "(lock_ptr->end 0x%llx < lock_ptr->mid_start 0x%llx) ",
                          (unsigned long long)lock_ptr->mid_end,
			  (unsigned long long)lock_ptr->start,
			  (unsigned long long)lock_ptr->end,
			  (unsigned long long)lock_ptr->mid_start);
                    return FBE_STATUS_GENERIC_FAILURE;
                }
            
            } /* end if (parity_range[1].size != 0) */
            else
            {
                if(RAID_COND((parity_range[1].size != 0) ||
                             (parity_range[1].start_lba != 0)) )
                {
                    fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                          "parity_range[1].size 0x%x or start lba 0x%x are non zero",
                          parity_range[1].size, parity_range[1].start_lba);
                    return FBE_STATUS_GENERIC_FAILURE;
                }

                /* Normal continuous lock.  
                 */
                lock_ptr->start = parity_range[0].start_lba;
                lock_ptr->end = parity_range[0].start_lba + 
                                parity_range[0].size - 1;
                if(RAID_COND(lock_ptr->end < lock_ptr->start))
                {
                    fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                        "%s status: lock_ptr->end 0x%x < lock_ptr->start 0x%x\n",
                                        __FUNCTION__, lock_ptr->end, lock_ptr->start);
                    return FBE_STATUS_GENERIC_FAILURE;
                }
            
            } /* end else-if (parity_range[1].size != 0) */
        
        } /* end if not RG_IS_STRIPED_MIRROR_GROUP */
    }

    /* For non-optimal RAID groups extend the lock to the optimal
     * block size multiple.
     */
    if (fbe_raid_iots_is_any_flag_set(iots_p, FBE_RAID_IOTS_FLAG_CHECK_UNALIGNED))
    {
        fbe_raid_group_t  *rgdb_p = fbe_raid_siots_get_raid_geometry(iots_p);
        fbe_element_size_t sectors_per_element = dba_unit_get_sectors_per_element(adm_handle,iots_p->unit);
        /* Currently we assume that the element size is a multiple
         * of the optimal block size.  This is required since stripped
         * mirrors take the lock on a logical basis.
         */
        if(RAID_COND(sectors_per_element % rgdb_p->optimal_block_size) )
        {
            fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                    "sectors_per_element % rgdb_p->optimal_block_size are non zero",
                    sectors_per_element, rgdb_p->optimal_block_size);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        rg_bsv_generate_aligned_iots_lock(iots_p, lock_ptr);
    
    } /* end if non-optimal RAID group */

    return;
}
/*****************************************
 * end of r5_calculate_lock()
 *****************************************/

/**********************************************************************
 * fbe_raid_calculate_raid10_lba_lock()
 **********************************************************************
 * @brief
 *   The function converts physical extent into logical extent from 
 *   the perspective of acquiring lock on logical extent for the 
 *   striped mirror group.
 *

 * @param iots_p - ptr to fbe_raid_iots_t for this request.
 * @param lock_ptr - a pointer to the lock request.
 *
 * @return VALUE:
 *   NONE
 *
 * @author
 *   08/20/2008  MA    Created.
 *
 ***********************************************************************/
static void fbe_raid_calculate_raid10_lba_lock(fbe_raid_iots_t * iots_p,
                                        LOCK_REQUEST * lock_ptr)
{
    fbe_lba_t start_lba;       /* start block no of logical extent */
    fbe_lba_t end_lba;         /* end block no of logical extent */
    fbe_u32_t element_size;  /* no of sectors per element */
    fbe_u32_t array_width;   /* no of data disks */

    array_width = dba_unit_get_num_data_disks(iots_p->unit);
    element_size = dba_unit_get_sectors_per_element(adm_handle,
                                                    iots_p->unit);

    /* IORB contains the request in PBA format.Get the address of starting
     * element corrsponding the start_lba on the first disk of LUN
     */
    start_lba = ((iots_p->iorb.lba) / element_size) * 
                 (array_width * element_size);

    /* add on the leftover blocks in starting element to get the actual
     * start_lba.
     */
    start_lba += ((iots_p->iorb.lba) % element_size);

    /* end_lba will always resides on the last disk of the LUN. First 
     * calculate the end_lba for the given physical extent with respect
     * to the first disk and then transfer it to the last disk.
     */

    /* Get the starting element corresponding to the end_lba on first disk.
     */
    end_lba = (((iots_p->iorb.lba) + 
                (iots_p->iorb.blocks) -1) / element_size) *
                (array_width * element_size);

    /* Add on the leftover blocks in element to get the end_lba with respect
     * to the first disk.
     */
    end_lba += ((((iots_p->iorb.lba) + 
                  (iots_p->iorb.blocks) -1)) % element_size);

    /* add elements to transfer the end_lba from first disk to last disk.
     */
    end_lba += (element_size * (array_width - 1));

    lock_ptr->start = start_lba;
    lock_ptr->end = end_lba;

    /* we will clear the flag 'LOCK_FLAG_HOLE' if it is set in any case.
     * as we are not using mid_start and mid_end here.
     */
    lock_ptr->mid_start = 0;
    lock_ptr->mid_end = 0;
    lock_ptr->flags &= (~LOCK_FLAG_HOLE);

    return;
}
/************************************************************************
 * end of fbe_raid_calculate_raid10_lba_lock()
 ************************************************************************/

/****************************************************************
 * fbe_raid_min_rb_offset()
 ****************************************************************
 * @brief
 *  This function determines the rebuild fru(s) of an array,
 *  and the current rebuild checkpoint.
 *
 *  The dead_pos in the SIOTS is updated to indicate which drive
 *  has the min rebuild offset.  If all drives have the same rebuild
 *  offset, the dead_pos is marked invalid.
 *

 *  siots_p [IO] - current siots
 *  rb_pos    [O]  - drive rebuilding
 *  array_width [I] - array width of this request
 *
 * @return VALUE:
 *  Returns the current rebuild checkpoint offset
 *
 * @author
 *  1/25/01 - JJIN created.
 *
 ****************************************************************/

fbe_lba_t fbe_raid_min_rb_offset(fbe_raid_siots_t * const siots_p, const fbe_u32_t array_width)
{
    fbe_lba_t min_rb_offset;
    fbe_lba_t max_rb_offset;
    fbe_u16_t pos;

    /*
     * loop through to find the pos with the minimum rebuild offset
     */
    if (RG_IS_HOT_SPARE(fbe_raid_siots_get_raid_geometry(siots_p)))
    {
        min_rb_offset = max_rb_offset =
            fbe_raid_degraded_chkpt(fbe_raid_siots_get_raid_geometry(siots_p), 0);
    }
    else
    {
        min_rb_offset = max_rb_offset =
            RG_RB_CHKPT(fbe_raid_siots_get_raid_geometry(siots_p),RG_GET_UNIT(siots_p), 0);
    }

    for (siots_p->dead_pos = 0, pos = 0;
         pos < array_width;
         pos++)
    {
        fbe_lba_t rb_offset;

        if (RG_IS_HOT_SPARE(fbe_raid_siots_get_raid_geometry(siots_p)))
        {
             rb_offset = fbe_raid_degraded_chkpt(fbe_raid_siots_get_raid_geometry(siots_p), pos);
        }
        else
        {
             rb_offset = RG_RB_CHKPT(fbe_raid_siots_get_raid_geometry(siots_p),RG_GET_UNIT(siots_p), pos);
        }

        if (rb_offset < min_rb_offset)
        {
            /* Found an offset less than minimum.
             * Save position and offset.
             */
            min_rb_offset = rb_offset;
            siots_p->dead_pos = pos;
        }
        else
        {
            /* Expansion drives have a rebuild checkpoint of -1 until
             * they are done expanding.  We don't want to include them
             * in this calculation.
             */
            max_rb_offset = rb_offset;
        }
    }

    if (max_rb_offset == min_rb_offset)
    {
        siots_p->dead_pos = (fbe_u16_t) - 1;
    }
    else if (min_rb_offset != 0 &&
            !RG_IS_HOT_SPARE(fbe_raid_siots_get_raid_geometry(siots_p)))
    {
        fbe_lba_t address_offset;

        if ((fbe_raid_siots_get_iots(siots_p)->iorb.opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY) ||
                (fbe_raid_siots_get_iots(siots_p)->iorb.opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_REBUILD))
        {
            address_offset = dba_unit_get_adjusted_addr_offset_phys(
                adm_handle,
                fbe_raid_siots_get_iots(siots_p)->unit,
                siots_p->start_lba + fbe_raid_siots_get_iots(siots_p)->org_addr_offset);
        }
        else
        {
            address_offset =
                dba_unit_get_adjusted_addr_offset(adm_handle,
                                                  fbe_raid_siots_get_iots(siots_p)->unit,
                                                  siots_p->start_lba);
        }

        if (min_rb_offset < address_offset)
        {
            /* Offset is below address offset.
             * Rebuild is in another lun in this RG.
             */
            min_rb_offset = 0;
        }
        else
        {
            /* Offset is within this lun or beyond this lun.
             */
            if (!RG_IS_MIRROR_GROUP(fbe_raid_siots_get_raid_geometry(siots_p)))
            {
                min_rb_offset = (min_rb_offset - address_offset);
            }
        }
    }

    return min_rb_offset;

}
/***************************************
 * End of fbe_raid_min_rb_offset()
 ***************************************/

/****************************************************************
 * fbe_raid_min_rb_offset()
 ****************************************************************
 * @brief
 *  This function determines the rebuild fru(s) of an array,
 *  and the current rebuild checkpoint.
 *  This is used by R5 and R6, since this code
 *  handles two degraded positions.
 *

 * @param siots_p - current siots
 * @param array_width - array width of this request
 * @param degraded_offset_p - Pointer to array[2] of offsets.
 *
 * @return
 *  fbe_status_t.
 *
 * @author
 *  03/31/06 - Rob Foley created.
 *
 ****************************************************************/

fbe_status_t fbe_raid_min_rb_offset(fbe_raid_siots_t * const siots_p, 
                      fbe_u32_t array_width,
                      fbe_lba_t *degraded_offset_p)
{
    fbe_u16_t pos;
    fbe_lba_t address_offset = 0; /* Start addr of LUN partition on RG. */
    fbe_lba_t physical_end = 0; /* End addr of LUN partition on RG. */
    fbe_u32_t unit = RG_GET_UNIT(siots_p);
    fbe_status_t status = FBE_STATUS_OK;

    /* Index of the current deg offset to fill in.
     */
    fbe_u16_t current_degraded_offset_index;
    
    /* Init the degraded offset array.
     */
    for ( current_degraded_offset_index = 0;
          current_degraded_offset_index < RG_MAX_DEGRADED_POSITIONS;
          current_degraded_offset_index++ )
    {
        /* Init the dead position and offset to invalid.
         * We set the degraded_off to the largest possible value since 
         * we know we are not degraded. It will insure the check below 
         * to see if we are in the degraded will prove false since there 
         * is no degraded area.
         */
        degraded_offset_p[current_degraded_offset_index] = RG_INVALID_DEGRADED_OFFSET;
        fbe_raid_siots_dead_pos_set( siots_p, current_degraded_offset_index, FBE_RAID_INVALID_DEAD_POS );
    }

    if (fbe_raid_siots_get_raid_geometry(siots_p)->degraded_bitmask == 0)
    {
        /* There is nothing for us to do here.
         * We already initialized the dead position and
         * the degraded offset positions to -1.
         * Just return.
         */
        return status;
    }
    
    /* Start with the first degraded offset,
     * we will fill them out in the order we find them.
     */
    current_degraded_offset_index = 0;

    /* Get the address offset.
     */        
    if ((fbe_raid_siots_get_iots(siots_p)->iorb.opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY) ||
        (fbe_raid_siots_get_iots(siots_p)->iorb.opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_REBUILD))
    {
        address_offset = dba_unit_get_adjusted_addr_offset_phys(
            adm_handle, 
            fbe_raid_siots_get_iots(siots_p)->unit,
            siots_p->start_lba + fbe_raid_siots_get_iots(siots_p)->org_addr_offset);
    }
    else
    {
        address_offset =
            dba_unit_get_adjusted_addr_offset(adm_handle,
                                              fbe_raid_siots_get_iots(siots_p)->unit,
                                              siots_p->start_lba);
    }

    /* The physical end address is the pba on the RAID group
     * where this LUN ends.
     */
    physical_end = dba_unit_get_physical_end_address(adm_handle, unit);
    
    for ( pos = 0; pos < array_width; pos++ )
    {
        fbe_lba_t rb_offset;
        rb_offset = RG_RB_CHKPT(fbe_raid_siots_get_raid_geometry(siots_p),RG_GET_UNIT(siots_p), pos);

        /* Found an offset less than minimum. Save position and offset.
         */
        if ( rb_offset < physical_end )
        {
            /* We can only have up to RG_SIOTS_MAX_DEGRADED_POSITIONS(siots_p)
             * degraded drives.
             */
            if(RAID_COND( current_degraded_offset_index >= RG_SIOTS_MAX_DEGRADED_POSITIONS(siots_p)) )
            {
                fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                    "%s status: current_degraded_offset_index 0x%x >= RG_SIOTS_MAX_DEGRADED_POSITIONS(siots_p) 0x%x\n",
                                    __FUNCTION__, current_degraded_offset_index, RG_SIOTS_MAX_DEGRADED_POSITIONS(siots_p));
                return FBE_STATUS_GENERIC_FAILURE;
            }
                
            if (rb_offset <= address_offset)
            {
                /* Offset is below address offset.
                 * Rebuild is in another lun in this RG.
                 */
                rb_offset = 0;
            }
            else if (!RG_IS_MIRROR_GROUP(fbe_raid_siots_get_raid_geometry(siots_p)))
            {
                /* Offset is within this lun.
                 * Just normalize the rb_offset to be sans address offset,
                 * so we have a unit relative physical offset.
                 */
                rb_offset = (rb_offset - address_offset);
            }

            /* Set the correct degraded offset index to the current rebuild offset.
             */
            degraded_offset_p[current_degraded_offset_index] = rb_offset;

            /* Set the current dead position to the current position.
             */
            fbe_raid_siots_dead_pos_set( siots_p, current_degraded_offset_index, pos );

            /* Move to the next degraded offset, as we are done with this one.
             */
            current_degraded_offset_index++;
        }
    }
    return status;
}
/***************************************
 * End of fbe_raid_min_rb_offset()
 ***************************************/

/***************************************************************
 *  fbe_raid_max_transfer_size()
 ***************************************************************
 * @brief
 *  This function is used to "carve off" a number of blocks
 *  to be considered for the next request in a striped unit.
 *

 *  host transfer,    [I] - Host or cached op?
 * @param lba - host lba where next operation starts
 * @param blocks_remaining - total blocks in next request
 * @param data_disks - data drives in array
 *  blks_per_element  [I] - blocks per stripe element
 *
 * @return
 *  Blocks to consider for next request.
 *
 * @notes
 *  This function can be invoked for non-striped units; just
 *  pass a "large" number of blocks for blks_per_element.
 *
 * @author
 *  16-Jul-98 SPN - Created.
 *  05-Aug-99 Rob Foley - Created from bem_max_xfer
 *
 ****************************************************************/

fbe_u32_t fbe_raid_max_transfer_size(fbe_bool_t host_transfer,
                             fbe_lba_t lba,
                             fbe_u32_t blocks_remaining,
                             fbe_u16_t data_disks,
                             fbe_lba_t blks_per_element)
{
    fbe_u32_t blks_to_xfer;

    if(RAID_COND(RG_MAX_ELEMENT_SIZE > BM_MAX_SG_DATA_ELEMENTS) )
    {
        fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                            "%s status: RG_MAX_ELEMENT_SIZE 0x%x > BM_MAX_SG_DATA_ELEMENTS 0x%x\n",
                            __FUNCTION__, RG_MAX_ELEMENT_SIZE, BM_MAX_SG_DATA_ELEMENTS);
        return 0;
    }

    /* Determine the maximum number of blocks we're
     * willing to transfer in this sub-request.
     *
     * NOTE: This value depends upon where the request
     * originated, i.e. the host or the cache, and how
     * far we've gone towards completing the request.
     */

    if (host_transfer)
    {
        /* A host transfer is first limited by the Front End Driver's
         * maximum transfer size.
         */
        blks_to_xfer = FBE_MIN(RG_MAX_FED_XFER_BLOCKS, blocks_remaining);

        /* Next, we are limited by the maximum number of sectors
         * which may be addressed by a single scatter gather list.
         */
        blks_to_xfer = FBE_MIN(blks_to_xfer, RG_MAX_BLKS_AVAILABLE);

        FBE_UNREFERENCED_PARAMETER(lba);
        FBE_UNREFERENCED_PARAMETER(blks_per_element);
    }

    else
    {
        /* This is a historical comment by Steve Nickel circa 1995.
         * The only changes here were the addition of "backfill"
         * to replace the prior "bitbucket" comments.
         *
         * For cache operations, the breakup of data into S/G lists is
         * a bit uncertain, dependant upon the cache page size, and
         * whether or not the operation might be a backfill for
         * 'picket-fence' type requests,
         * In any event, we must assume that the S/G list
         * supplied by the cache might contain single-sector S/G elements.
         *
         * We will assume that RG_MAX_ELEMENT_SIZE is less than or equal
         * to the number of S/G elements in the largest S/G list we can
         * allocate from the BM.
         *
         * If this should not be the case, we could run into bizarre
         * 'picket-fence' scenarios where we 'bite off' more elements
         * destined for a single drive than a S/G list can 'chew', which
         * would be BAD!
         */
        blks_to_xfer = FBE_MIN(data_disks, FBE_RAID_MAX_DISK_ARRAY_WIDTH) * RG_MAX_ELEMENT_SIZE;
        blks_to_xfer = FBE_MIN(blks_to_xfer, blocks_remaining);
    }

    return blks_to_xfer;
} /* fbe_raid_max_transfer_size() */


/****************************************************************
 * fbe_raid_get_max_blocks()
 ****************************************************************
 * @brief
 *  Determine the max size for a Striper request.
 *  We limit the request to a fixed block size, but
 *  we also limit the request based on sg limits.
 *

 * @param siots_p - Current SIOTS ptr.
 * @param blocks - number of blocks in this request
 * @param blocks_per_element - element size of unit
 * @param width - width of unit
 *
 * @return VALUE:
 *   fbe_u32_t
 *
 * @author
 *   3/16/01 - Created. Rob Foley
 *   4/24/01 - Rewrote.  Jing Jin
 *
 ****************************************************************/

fbe_u32_t fbe_raid_get_max_blocks(fbe_raid_siots_t *const siots_p,
                          fbe_lba_t lba,
                          fbe_u32_t blocks,
                          fbe_u32_t width,
                          fbe_bool_t align)
{
    fbe_status_t status = FBE_STATUS_OK;
    /* Get the blocks per element from the dba function, the 
     * geometry block may not be filled out yet since generate
     * calls this.
     */
    fbe_lba_t blocks_per_element = 
        dba_unit_get_sectors_per_element(adm_handle, RG_GET_UNIT(siots_p));

    /* The sg_size is the number of blocks we will be using
     * per sg entry, per drive, and this is dependent upon the
     * blocks per element size.
     */
    fbe_u32_t sg_size = (fbe_u32_t) ((blocks_per_element >= fbe_raid_get_std_buffer_size())
        ? fbe_raid_get_std_buffer_size() : blocks_per_element);

    /* Limit of this I/O based on the sg limits.
     */
    fbe_u32_t sg_block_limit;

    /* The total_sgs, are the number of sgs per drive that
     * we are limited to.
     */
    fbe_u32_t total_sgs = BM_MAX_SG_DATA_ELEMENTS;
    fbe_u32_t stripe_size = (fbe_u32_t) blocks_per_element * width;

    if ((width == 1) || (blocks_per_element == 1))
    {
        /* Leave total_sgs as they are.
         */
    }
    else
    {
        /* In the worst case scenario, a full element might be split into two buffers.
         */
        total_sgs /= 2;
    }

    /* If Specified, align this to a data element stripe.
     */
    if (align)
    {
        if ( (lba % stripe_size) ||
             ((lba + blocks) % stripe_size) )
        {
            /* Misaligned I/Os need to adjust our sg counts by 3.
             * This accounts for misalignment and for pre-reads. 
             */
            total_sgs -= 3;
        }
    }

    /* Cached ops might have an offset into a buffer, that could
     * cause an additional sg crossing.
     */
    if (!fbe_raid_siots_is_buffered_op(siots_p))
    {
        /* Create an sg descriptor for this request and adjust it to
         * determine if there is an offset into the first sg element
         * of this request.
         */
        fbe_sg_element_with_offset_t sg_desc;    /* tracks location in cache S/G list */
        status = fbe_raid_siots_setup_cached_sgd(siots_p, &sg_desc);
        if(RAID_COND(status != FBE_STATUS_OK))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                "status 0x%x != FBE_STATUS_OK while setup cached sgd\n",
                                status);
            return blocks;
        }
        fbe_raid_adjust_sg_desc(&sg_desc);
    
        /* If we have an offset, decrement the number of sgs by 1
         * since we will need to account for an additional sg.
         */
        if (sg_desc.offset != 0)
        {
            total_sgs--;
        }
    } /* end if not host op. */
    
        
    /* Calclulate the number of blocks that will fit into an sg.
     */
    sg_block_limit = width * total_sgs * sg_size;
    
    /* Do a min between blocks and
     * the total blocks that can fit in an sg.
     */
    blocks = FBE_MIN(blocks, sg_block_limit);

    /* The standard buffers are a power of 2 size
     * and standard element size are a power of 2.
     * Non-standard elements are not a power of 2, thus
     * we end up with fewer possible sg entries, as
     * each full element (element size < std buffer size)
     * or buffer chunk (element size > std buffer size)
     * may take up to 2 sg entries, rather than the 1 sg entry
     * which is common to the standard element size.
     */
    if(RAID_COND( (total_sgs == 0) || 
                  (sg_block_limit == 0) ) )
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                            "total_sgs 0x%x == 0) || sg_block_limit 0x%x == 0\n",
                            total_sgs, sg_block_limit);
        return blocks;
    }

    return blocks;
} 
/* end fbe_raid_get_max_blocks */

/****************************************************************
 * fbe_raid_set_degraded_chkpt()
 ****************************************************************
 * @brief
 *  Set the degraded checkpoint for this raid group.
 *

 * @param rgdb_p - current raid group db
 * @param pos - element size of unit
 * @param chkpt - the checkpoint to set to.
 *
 * @return VALUE:
 *   fbe_status_t
 *
 * @author
 *   04/26/05 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_set_degraded_chkpt(fbe_raid_group_t * rgdb_p, fbe_u32_t pos, fbe_lba_t chkpt)
{
    fbe_status_t status = FBE_STATUS_OK;
	if(RAID_COND(!RG_IS_HOT_SPARE(rgdb_p) ||
                ( pos == RG_MIRROR_DEPTH ) ) )
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                        "rgdb_p %p RG is not hotspare, pos 0x%x == RG_MIRROR_DEPTH\n",
                                        pos);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if ( fbe_raid_fru_is_a_hotspare( rgdb_p, pos ) )
    {
        /* Hot spare is swapped in.
         */
        fbe_u32_t hs_lun = fbe_raid_get_hs_lun( rgdb_p, 1 );

        /* Set the checkpoint for this hotspare into the
         * cm's database.
         * The 0 position also holds for PaCO since the checkpoint is
         * always stored in the HS.
         */
        dba_fru_set_part_chkpt_by_unit_and_fru_pos( hs_lun, 0, chkpt );
    }
    else
    {
        /* Nothing special here.
         * Just set the degraded checkpoint for this position.
         */
        rgdb_p->degraded_checkpoint[pos] = chkpt;
    }
    return status;
}
/* end fbe_raid_set_degraded_chkpt() */

/****************************************************************
 * fbe_raid_degraded_chkpt()
 ****************************************************************
 * @brief
 *  Determine the degraded checkpoint for this array position
 *  in the given RAID group driver instance.
 *  This macro should only be used for hotspares.
 *

 * @param rgdb_p - current raid group db
 * @param pos - element size of unit
 *
 * @return VALUE:
 *   fbe_lba_t - checkpoint
 *
 * @author
 *   04/26/05 - Created. Rob Foley
 *
 ****************************************************************/

fbe_lba_t fbe_raid_degraded_chkpt( fbe_raid_group_t * rgdb_p, fbe_u32_t pos )
{
    fbe_lba_t chkpt;

    /* We should only be here if a hot spare since the
     * caller should have already made this check.
     */
    if(RAID_COND( !RG_IS_HOT_SPARE(rgdb_p) ) ||

    /* Hot Spare mirrors only have two drives.
     */
                ( pos == RG_MIRROR_DEPTH ) )
    {
        fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                            "%s status: RG is not hostpare and pos 0x%x == RG_MIRROR_DEPTH\n",
                            __FUNCTION__, pos);
        return FBE_INVALID_LBA;
    }

    if ( fbe_raid_fru_is_a_hotspare( rgdb_p, pos ) )
    {
        /* Since this is a hotspare disk driver, the position 1
         * contains the hot spare drive.
         * It is the LUN associated with this drive that contains
         * the checkpoint for this position (always 1).
         * Get the LUN associated with this hot spare.
         */
        fbe_u32_t hs_lun = fbe_raid_get_hs_lun( rgdb_p, 1 );

        /* Always use the first position to retrieve the checkpoint
         * This logic also holds for PaCO since the checkpoint is
         * always stored in the HS.
         */
        chkpt =
            dba_fru_get_part_chkpt_by_unit_and_fru_pos( adm_handle,
                                                        hs_lun,
                                                        0 );
    } /* end if dba_fri_is_a_hotspare() */
    else
    {
        /* Nothing special here.
         * Just return the degraded checkpoint for this position.
         */
        chkpt = rgdb_p->degraded_checkpoint[ pos ];
    }

    return chkpt;
}
/* end fbe_raid_degraded_chkpt() */

/**************************************************************************
 * rg_get_transfer_limit()
 **************************************************************************
 *
 * @brief
 *  This function determines the maximum blocks that can be transferred based 
 *  on the raid configuation and resource limits.
 *

 * @param unit - Glut entry.
 * @param array_width - No.of drives involved for this unit.
 * @param blocks - Host transfer count.
 * @param lba - Host address.
 * @param host_transfer - True if non-cached I/O 
 *                            and false if cached I/O.
 * @param backfill - true if backfill operation else false.
 *  host_io_check_count [I] - 9 if raid type is a parity group else
                              8 will be for other raid type. 
 *  
 * @return VALUES:
 * @param fbe_u32_t - returns max transfer limit in blocks.
 *
 * @notes
 *  This is a request for enhancement of dims # 129910 to change 
 *  the Raid mirror code to allow larger I/O sizes on the backend.
 *  This is made as generic function so that this could be called 
 *  from Raid5 generate and as well as from Raid mirror code.
 *
 * @author
 *  10/13/06  SM - Created
 *
 ***************************************************************************/
fbe_u32_t rg_get_transfer_limit(fbe_u32_t unit,
                              fbe_u32_t array_width,
                              fbe_u32_t blocks,
                              fbe_lba_t lba,
                              fbe_bool_t host_transfer,
                              fbe_bool_t backfill,
                              fbe_u32_t host_io_check_count)
{
    fbe_u32_t blocks_per_element;     /* Number of blocks per element. */
    fbe_u32_t blocks_per_data_stripe; /* Number of blocks per data stripe. */

    blocks_per_element = dba_unit_get_sectors_per_element(adm_handle, unit);
    blocks_per_data_stripe = blocks_per_element * array_width;

    /* Limit this request.
     */
    if (host_transfer || 
        blocks_per_element < cm_what_is_cache_page_size() ||
        backfill)
    {
        /* If the element size is bigger than the page size,
         * we don't limit it to RG_MAX_ELMENT_SIZE per driver.
         * Don't allow backfills generate larger size,
         * since there is no telling how large the backfill.
         */
        blocks = fbe_raid_max_transfer_size(host_transfer,
                                      lba,
                                      blocks,
                                      array_width,
                                      blocks_per_element);
    }
    
    /* We checked the below condition to avoid of crossing the BM limits.
     * This happens when the array width and host IO are large.
     * So we have considered 9 or more drives as large array width 
     * and also checks the limit within maximum 4096 blocks 
     * to allocate buffer correctly.
     */
    if (array_width >= host_io_check_count && blocks > RG_MAX_ELEMENT_SIZE)
    {
        /* If host IO is as large as 1M and array width is large, 
         * there is a danger to exceed BM limit when we allocate buffer.
         * To make sure cached write will not exceed BM limit, direct it 
         * to the following check as well.
         */
        if (((((lba + blocks - 1) / blocks_per_data_stripe) - 
                (lba / blocks_per_data_stripe) + 1) * 
                (blocks_per_data_stripe + blocks_per_element)) > RG_MAX_BLKS_AVAILABLE)
        {
            fbe_u32_t lbaStripeOffset = (fbe_u32_t) lba % blocks_per_data_stripe;
            fbe_u32_t newStripeCount;

            /* We have too many blocks to allow for a degraded
             * operation, half the request, rounding DOWN to the 
             * stripe boundary
             */
            newStripeCount = ((blocks/2) + lbaStripeOffset) / blocks_per_data_stripe;
            if (newStripeCount == 0)
            {
                /* The block count is on only 1 stripe, rounding down went
                 * to zero. We must always at least do one stripe
                 */
                newStripeCount = 1;
            }
            blocks = FBE_MIN((newStripeCount * blocks_per_data_stripe) - 
                        lbaStripeOffset, blocks);
        }
    }
    return blocks;
}
/**************************************************************************
 * end of rg_get_transfer_limit()
 **************************************************************************/
#endif
/******************************
 *  fbe_raid_generate.c
 ******************************/
