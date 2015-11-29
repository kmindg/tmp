/***************************************************************************
 * Copyright (C) EMC Corporation 1999-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_parity_verify.c
 ***************************************************************************
 *
 * @brief
 *   This file contains the state functions of the RAID 5 verify algorithm.
 *   Both normal verify and recovery verify use this algorithm.
 *
 *   The normal verify comes down from verify process.
 *   The recovery verify is selected only when the read or write (468 or MR3)
 *   has encountered hard errors or checksum errors on the read data.
 *
 *
 * @return VALUES
 *   The normal verify returns status as expected.  The recovery verify
 *   places status in the parent SIOTS.  Return Status values are as follows.
 *
 *   Aborted - State4 checks for aborted and returns IOTS status.
 *
 *   FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED
 *        State4, State10 detects when FBE_RAID_FRU_ERROR_STATUS_SHUTDOWN is detected
 *
 *   FBE_PAYLOAD_BLOCK_OPERATION_STATUS_NOT_READY
 *        State4, State10 detects a dead drive
 *
 *   FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR
 *        State21 returns this error if the recovery read checksum fails.
 *
 *   FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS
 *        State11 (no errors), State25 (errors) return this status.
 *
 * @notes
 *   For recovery verify, at initial entry, we assume that a nested
 *   fbe_raid_siots_t has been allocated.
 *   After recovery verify is done recovery reads will go back to the
 *   parent sub_iots' state machine to continue the read.
 *   Write Recovery will finish the write in the recovery verify state machines.
 *
 * @author
 *   8/24/99 Created Jing Jin.
 *
 ***************************************************************************/

/*************************
 **  INCLUDE FILES
 *************************/
#include "fbe_raid_library_proto.h"
#include "fbe_parity_io_private.h"

/*!***************************************************************
 * fbe_parity_degraded_verify_state0()
 ****************************************************************
 * @brief
 *  This function sets up the algorithm for the degraded verify.
 *
 * @param siots_p - Current sub request. 
 *
 * @return
 *  fbe_raid_state_status_t
 *
 * @author
 *  4/10/06 - Created. Rob Foley
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_parity_degraded_verify_state0(fbe_raid_siots_t * siots_p)
{
    fbe_raid_siots_t *parent_siots_p = fbe_raid_siots_get_parent(siots_p);

    /* Determine which algorithm to set based on the parent's algorithm.
     */
    switch (parent_siots_p->algorithm)
    {
        case R5_DEG_MR3:
        case R5_DEG_468:
        case R5_DEG_RCW:
            siots_p->algorithm = R5_DEG_VR;
            break;
        default:
            fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "parity: unexpected algorithm 0x%x\n", 
                                                parent_siots_p->algorithm);
            return FBE_RAID_STATE_STATUS_EXECUTING;
            break;
    };

    /* Transition to the state to see if we can get a siots lock. 
     * Since this is a performance path we do not 
     * try to upgrade the lock. 
     * Any media errors will return an error to the caller and the caller will need to go 
     * through degraded recovery verify. 
     */
    fbe_raid_siots_transition(siots_p, fbe_parity_recovery_verify_state3);
    /* We mark ourselves as upgrade owner since we are not expanding the 
     * lock and thus do not need to upgrade the lock.  We mark this so that 
     * any other upgrade waiters will wake us up first. 
     */
    fbe_raid_siots_set_flag(siots_p, FBE_RAID_SIOTS_FLAG_UPGRADE_OWNER);

    /* Set up the geometry information in the nested sub_iots.
     * We intentionally do not align to the optimal size since 
     * this is in the I/O path and we do not want to 
     * change the range of the I/O. 
     */
    fbe_parity_recovery_verify_setup_geometry(siots_p, FBE_FALSE /* no don't align */);

    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/* end of fbe_parity_degraded_verify_state0() */

/*!**************************************************************
 * @fn fbe_parity_degraded_recovery_verify_state0(fbe_raid_siots_t * siots_p)
 ****************************************************************
 * @brief
 *  Start a degraded recovery verify.
 *  The only difference between this and a degraded verify is that
 *  in this mode we will align the request to the optimal block
 *  size so that we can handle any type of error including a media error.
 *  
 * @param siots_p - Contains information on the request.
 * 
 * @return fbe_raid_state_status_t
 *
 * @author
 *  12/23/2008 - Created. Rob Foley
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_parity_degraded_recovery_verify_state0(fbe_raid_siots_t * siots_p)
{
    fbe_raid_siots_t *parent_siots_p = fbe_raid_siots_get_parent(siots_p);

    /* Determine which algorithm to set based on the parent's algorithm.
     */
    switch (parent_siots_p->algorithm)
    {
        case R5_DEG_RD:
            siots_p->algorithm = R5_DRD_VR;
            break;
        case R5_DEG_MR3:
        case R5_DEG_468:
        case R5_DEG_RCW:
            siots_p->algorithm = R5_DEG_RVR;
            break;
        case RG_FLUSH_JOURNAL:
            siots_p->algorithm = RG_FLUSH_JOURNAL_VR;
            break;
        default:
            /* Do not know how to handle this algorithm.
             */
            fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "unexpected algorithm 0x%x", 
                                                parent_siots_p->algorithm);
            return FBE_RAID_STATE_STATUS_EXECUTING;
            break;
    };

    /* Transition to the state to see if we can get a siots lock. 
     * Since this is a performance path we do not 
     * try to upgrade the lock. 
     * Any media errors will return an error to the caller and the 
     * caller will need to go through degraded recovery verify.
     */
    fbe_raid_siots_transition(siots_p, fbe_parity_recovery_verify_state0); 

    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/*!*************************************
 * end fbe_parity_degraded_recovery_verify_state0()
 **************************************/

/*!************************************************************** 
 * fbe_parity_recovery_verify_state0() 
 ****************************************************************
 * @brief
 *  This function sets up the algorithm for the recovery verify.
 *
 * @param siots_p    - ptr to the fbe_raid_siots_t
 *
 * @return
 *  This function returns FBE_RAID_STATE_STATUS_WAITING if we were unable
 *  to allocate the required number of structures immediately;
 *  otherwise, FBE_RAID_STATE_STATUS_EXECUTING is returned.
 *
 * @author
 *  8/24/99 - Created. JJ
 *  6/29/01 - Fixed bug where we did not increment stats. Rob Foley
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_parity_recovery_verify_state0(fbe_raid_siots_t * siots_p)
{
    fbe_raid_siots_t *parent_siots_p = fbe_raid_siots_get_parent(siots_p);
#if 0
    fbe_raid_iots_t *iots_p = fbe_raid_siots_get_iots(siots_p);
#endif

    if (siots_p->algorithm == R5_DEG_VR ||
        siots_p->algorithm == R5_DEG_RVR ||
        siots_p->algorithm == R5_DRD_VR ||
        siots_p->algorithm == RG_FLUSH_JOURNAL_VR)
    {
        /* Algorithm is setup okay, so don't change it.
         */
    }
    else if ((parent_siots_p->algorithm == R5_RD) ||
             (parent_siots_p->algorithm == R5_SM_RD))
    {
        siots_p->algorithm = R5_RD_VR;
    }
    else if ((parent_siots_p->algorithm == R5_468) ||
             (parent_siots_p->algorithm == R5_CORRUPT_DATA))
    {
        siots_p->algorithm = R5_468_VR;
    }
    else if (parent_siots_p->algorithm == R5_MR3)
    {
        siots_p->algorithm = R5_MR3_VR;
    }
    else if (parent_siots_p->algorithm == R5_RCW)
    {
        siots_p->algorithm = R5_RCW_VR;
    }
    else
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "unexpected algorithm 0x%x", siots_p->algorithm);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* Mark error handling as going on.
     */
    fbe_raid_iots_inc_errors(fbe_raid_siots_get_iots(siots_p));

    /* Set up the geometry information in the nested sub_iots.
     */
    fbe_parity_recovery_verify_setup_geometry(siots_p, FBE_TRUE /* yes align */);

    fbe_raid_siots_transition(siots_p, fbe_parity_recovery_verify_state2);

    return(FBE_RAID_STATE_STATUS_EXECUTING);
}   
/* end of fbe_parity_recovery_verify_state0() */

/*!**************************************************************
 * @fn fbe_parity_recovery_verify_state2(fbe_raid_siots_t * siots_p)
 ****************************************************************
 * @brief
 *  We were waiting for an upgrade of our lock.
 *  Now that the upgrade is complete, continue to the state
 *  where we will check for a siots lock.
 *  
 * @param siots_p - Contains information about this I/O.
 * 
 * @return FBE_RAID_STATE_STATUS_EXECUTING.
 *
 * @author
 *  12/23/2008 - Created. Rob Foley
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_parity_recovery_verify_state2(fbe_raid_siots_t * siots_p)
{
    /* Our upgrade is here, so clear our upgrade flag.
     */
    fbe_raid_siots_clear_flag(siots_p, FBE_RAID_SIOTS_FLAG_WAIT_UPGRADE_LOCK);

    /* Flag the fact that we are the lock owner so that we
     * don't start any other siots that are waiting for an upgrade.
     */
    if (fbe_raid_siots_is_flag_set(siots_p, FBE_RAID_SIOTS_FLAG_UPGRADE_OWNER))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity: upgrade owner already set\n");
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }
    fbe_raid_siots_set_flag(siots_p, FBE_RAID_SIOTS_FLAG_UPGRADE_OWNER);

    /* Make sure we have a write lock.
     */
#if 0

    if (RAID_COND(((fbe_raid_siots_get_iots(siots_p)->iorb.flags & RAID_LOCK_OBTAINED)== 0) &&
                  ((fbe_raid_siots_get_iots(siots_p)->lock_ptr->flags & LOCK_FLAG_WRITE) != LOCK_FLAG_WRITE)))
    {
        fbe_raid_siots_set_unexpected_error(siots_p,
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity: lock error : iorb flags = 0x%x, siots lock falgs = 0x%x\n",
                                            fbe_raid_siots_get_iots(siots_p)->iorb.flags,
                                            fbe_raid_siots_get_iots(siots_p)->lock_ptr->flags);

        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

#endif
    fbe_raid_siots_transition(siots_p, fbe_parity_recovery_verify_state3);

    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/*!*************************************
 * end fbe_parity_recovery_verify_state2()
 **************************************/

/*!**************************************************************
 * @fn fbe_parity_recovery_verify_state3(fbe_raid_siots_t * siots_p)
 ****************************************************************
 * @brief
 *  We upgraded the lock if needed and now we will check for
 *  a siots lock conflict before kicking off the verify.
 *  
 * @param siots_p - Contains information about the I/O.
 * 
 * @return FBE_RAID_STATE_STATUS_EXECUTING if we have a siots lock.
 *         FBE_RAID_STATE_STATUS_WAITING if we do not have a siots lock.
 *
 * @author
 *  12/23/2008 - Created. Rob Foley
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_parity_recovery_verify_state3(fbe_raid_siots_t * siots_p)
{
    fbe_raid_state_status_t state_status;

    fbe_raid_siots_transition(siots_p, fbe_parity_verify_state0);

    /* Before we kick off this siots, let's make sure that it can run. 
     * Since we might have changed the parity range, we need to check if 
     * it conflicts with anything else that is running. 
     */
    state_status = fbe_raid_siots_generate_get_lock(siots_p);
    return state_status;
}
/**************************************
 * end fbe_parity_recovery_verify_state3()
 **************************************/
/*!***************************************************************
 * fbe_parity_verify_state0()
 ****************************************************************
 * @brief
 *  This function allocates a RAID_VC_TS tracking structure.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t.
 *
 * @return
 *  This function returns FBE_RAID_STATE_STATUS_WAITING if we were unable
 *  to allocate the required number of structures immediately.
 *  Otherwise FBE_RAID_STATE_STATUS_EXECUTING is returned.
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_parity_verify_state0(fbe_raid_siots_t * siots_p)
{
    fbe_status_t            status;
    fbe_u32_t               num_pages = 0;
    /* Add one for terminator.
     */
    fbe_raid_fru_info_t     read_info[FBE_RAID_MAX_DISK_ARRAY_WIDTH+1];

    if (fbe_parity_verify_validate(siots_p) == FBE_FALSE)
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity: verify validate failed\n");
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* Calculate and allocate all the memory needed for this siots.
     */
    status = fbe_parity_verify_calculate_memory_size(siots_p, &read_info[0], &num_pages);
    if (status != FBE_STATUS_OK)
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "unexpected calculate memory size status 0x%x\n", status);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* Now allocate all the pages required for this request.  Note that must 
     * transition to `deferred allocation complete' state since the callback 
     * can be invoked at any time. 
     * There are (3) possible status returned by the allocate method: 
     *  o FBE_STATUS_OK - Allocation completed immediately (skip deferred state)
     *  o FBE_STATUS_PENDING - Allocation didn't complete immediately
     *  o Other - Memory allocation error
     */
    fbe_raid_siots_transition(siots_p, fbe_parity_verify_state1);
    status = fbe_raid_siots_allocate_memory(siots_p);

    /* Check if the allocation completed immediately.  If so plant the resources
     * now.  Else we will transition to `deferred allocation complete' state when 
     * the memory is available so return `waiting'
     */
    if (RAID_MEMORY_COND(status == FBE_STATUS_OK))
    {
        /* Allocation completed immediately.  Plant the resources and then
         * transition to next state.
         */
        status = fbe_parity_verify_setup_resources(siots_p, &read_info[0]);
        if (RAID_COND(status != FBE_STATUS_OK))
        {
            fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity: verify failed to setup resources. status: 0x%x\n",
                                            status);
            return FBE_RAID_STATE_STATUS_EXECUTING;
        }

        /* To prevent confusion (i.e. we never went to `deferred allocation complete'
         * state) overwrite the current state.
         */
        fbe_raid_common_set_state(&siots_p->common, (fbe_raid_common_state_t)fbe_parity_verify_state3);
    }
    else if (RAID_MEMORY_COND(status == FBE_STATUS_PENDING))
    {
        /* Else return waiting
         */
        return FBE_RAID_STATE_STATUS_WAITING;
    }
    else
    {
        /* Change the state to unexpected error and return
         */
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity: verify memory request failed with status: 0x%x\n",
                                            status);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/* end of fbe_parity_verify_state0() */

/****************************************************************
 * fbe_parity_verify_state1()
 ****************************************************************
 * @brief
 *  Resources weren't immediately available.  We were called back
 *  by the memory service.
 *
 * @param siots_p - current sub request
 *
 * @return  This function returns FBE_RAID_STATE_STATUS_EXECUTING
 *
 * @author
 *  7/30/99 - Created. Rob Foley
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_parity_verify_state1(fbe_raid_siots_t * siots_p)
{
    fbe_status_t            status;
    fbe_u32_t               num_pages = 0;
    /* Add one for terminator.
     */
    fbe_raid_fru_info_t     read_info[FBE_RAID_MAX_DISK_ARRAY_WIDTH+1];

    /* First check if this request was aborted while we were waiting for
     * resources.  If so simply transition to the aborted handling state.
     */
    if (fbe_raid_siots_is_aborted(siots_p))
    {
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_aborted);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* If we are quiescing, then this function will mark the siots as quiesced 
     * and we will wait until we are woken up.
     */
    if (fbe_raid_siots_mark_quiesced(siots_p))
    {
        return FBE_RAID_STATE_STATUS_WAITING;
    }

    /* Now check for allocation failure
     */
    if (RAID_MEMORY_COND(!fbe_raid_siots_is_deferred_allocation_successful(siots_p)))
    {
        /* Trace some information and transition to allocation error
         */
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: verify memory allocation error common flags: 0x%x \n",
                             fbe_raid_common_get_flags(&siots_p->common));
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_memory_allocation_error);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* Then calculate how many buffers are needed.
     */
    status = fbe_parity_verify_calculate_memory_size(siots_p, &read_info[0], &num_pages);
    if (status != FBE_STATUS_OK)
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "unexpected calculate memory size status 0x%x\n", status);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* Allocation completed. Plant the resources
     */
    status = fbe_parity_verify_setup_resources(siots_p, &read_info[0]);
    if (RAID_COND(status != FBE_STATUS_OK))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity: verify failed to setup resources. status: 0x%x\n",
                                            status);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* Transition to next state to issue request
     */
    fbe_raid_siots_transition(siots_p, fbe_parity_verify_state3);

    /* Return the state status
     */
    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/********************************
 * end fbe_parity_verify_state1()
 ********************************/

/*!***************************************************************
 *  fbe_parity_verify_state3()
 ****************************************************************
 * @brief
 *  This function will set up sgs and issue reads to disks.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t
 *
 * @return
 *  FBE_RAID_STATE_STATUS_WAITING
 *
 * @author
 *  8/30/99 - Created. JJ
 *  10/20/99 - Modified for both normal and recovery verify.
 *  01/29/99 - Added comments.  Rob Foley
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_parity_verify_state3(fbe_raid_siots_t * siots_p)
{
    fbe_raid_state_status_t return_status;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_bool_t b_generate_next_siots = FBE_FALSE;
    fbe_raid_iots_t *iots_p = fbe_raid_siots_get_iots(siots_p);

    if (fbe_raid_siots_is_aborted(siots_p) ||
        (siots_p->error == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_REQUEST_ABORTED))
    {
        /* The iots has been aborted,
         * treat it as a hard error,
         * and bail out now.
         */
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_aborted);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }
    if (siots_p->algorithm == R5_VR)
    {
        /*********************************************************
         * Because we have finished allocating resources, we
         * need to start any waiting requests,
         * by continuing SIOTS request pipeline.
         *********************************************************/
        if (!(fbe_raid_siots_is_flag_set(siots_p, FBE_RAID_SIOTS_FLAG_SINGLE_REGION_MODE)))
        {
            fbe_status_t status;
            /* Determine if we need to generate another siots.
             * If this returns TRUE then we must always call fbe_raid_iots_generate_next_siots_immediate(). 
             * Typically this call is done after we have started the disk I/Os.
             */
            status = fbe_raid_siots_should_generate_next_siots(siots_p, &b_generate_next_siots);
            if (RAID_GENERATE_COND(status != FBE_STATUS_OK))
            {
                fbe_raid_siots_set_unexpected_error(siots_p, 
                                                    FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS, 
                                                    "parity: %s failed in next siots generation: siots_p = 0x%p, status = %dn",
                                                    __FUNCTION__, siots_p, status);
                return(FBE_RAID_STATE_STATUS_EXECUTING);
            }
        }
    }

    /* Since we may have waited before getting here, check if we are degraded now.
     */
    {
        /* This is used to store the results of the below fbe_parity_check_degraded_phys call.
         */
        fbe_status_t check_degraded_status;
        fbe_raid_fruts_t *read_fruts_p = NULL;
        fbe_raid_position_bitmask_t retry_bitmap;
        fbe_raid_position_bitmask_t active_bitmap;
        fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_p);
        /* See if we are degraded on this LUN.
         */
        check_degraded_status = fbe_parity_check_degraded_phys(siots_p);
        if (RAID_COND(check_degraded_status != FBE_STATUS_OK))
        {
            if (b_generate_next_siots) { fbe_raid_iots_cancel_generate_next_siots(iots_p); }
            fbe_raid_siots_set_unexpected_error(siots_p,
                                                FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "parity: %s failed to check degraded status: 0x%x, siots_p = 0x%p\n", 
                                                __FUNCTION__, 
                                                check_degraded_status,
                                                siots_p);

            return FBE_RAID_STATE_STATUS_EXECUTING;
        }

        /* Make the opcode of any degraded positions be nop.
         */
        fbe_raid_fruts_set_degraded_nop(siots_p, read_fruts_p);
        fbe_raid_fruts_chain_set_retryable(siots_p, read_fruts_p, &retry_bitmap);
        if (retry_bitmap)
        {
            /* Disable any optimizations so we always evaluate fruts chain. 
             * We need this since we still need to evalute the errors we inherited from the parent. 
             */
            fbe_raid_siots_set_flag(siots_p, FBE_RAID_SIOTS_FLAG_ERROR_PENDING);
        }
        /* Remove any retryable errors from our wait count.
         */
        siots_p->wait_count = fbe_raid_fruts_get_chain_bitmap(read_fruts_p, &active_bitmap) - fbe_raid_get_bitcount(retry_bitmap);

        /* Transition to the waiting for reads state.
         */
        fbe_raid_siots_transition(siots_p, fbe_parity_verify_state4);
        
        fbe_raid_siots_check_traffic_trace(siots_p, FBE_TRUE);
        if (siots_p->wait_count == 0) {
            /* The combination of retryable errors and dead errors result in 
             * zero drives to read from. Continue in the next state immediately.
             */
            return_status = FBE_RAID_STATE_STATUS_EXECUTING;
        }
        else {
            /* Issue the read requests.
             * Start everything except what was previously detected as an error. 
             */
            status = fbe_raid_fruts_send_chain_bitmap(&siots_p->read_fruts_head, 
                                                      fbe_raid_fruts_evaluate, 
                                                      (active_bitmap & (~retry_bitmap)));
            if (status != FBE_STATUS_OK)
            {
                if (b_generate_next_siots) { fbe_raid_iots_cancel_generate_next_siots(iots_p); }
                fbe_raid_siots_set_unexpected_error(siots_p, 
                                                    FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                    "parity: %s : unexpected fruts send status: 0x%x, siots_p = 0x%p\n",
                                                    __FUNCTION__,
                                                    status,
                                                    siots_p);
                return FBE_RAID_STATE_STATUS_EXECUTING;
            }
    
            return_status = FBE_RAID_STATE_STATUS_WAITING;
        }
    }

    if (b_generate_next_siots)
    {
        fbe_raid_iots_generate_next_siots_immediate(iots_p);
    }
    return return_status;
}
/* end of fbe_parity_verify_state3() */
/*!**************************************************************
 * fbe_parity_verify_handle_read_error()
 ****************************************************************
 * @brief
 *  Determine what to do when we get a FBE_RAID_FRU_ERROR_STATUS_ERROR,
 *  on a read for verify.
 *
 * @param siots_p - current I/O.
 * @param eboard_p - Eboard filled out from a read chain.  
 *
 * @return FBE_RAID_STATE_STATUS_DONE - Just continue with verify.
 *         Otherwise - return this status from the state machine.
 *
 * @author
 *  4/26/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_parity_verify_handle_read_error(fbe_raid_siots_t * siots_p,
                                                            fbe_raid_fru_eboard_t *eboard_p)
{
    fbe_raid_fruts_t *read_fruts_p = NULL;
    fbe_status_t check_degraded_status;
    fbe_status_t status = FBE_STATUS_OK;

    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_p);

    /* See if we are degraded on this LUN.
     */
    check_degraded_status = fbe_parity_check_degraded_phys(siots_p);
    if (RAID_COND(check_degraded_status != FBE_STATUS_OK))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity: %s failed to check degraded status: 0x%x\n", 
                                            __FUNCTION__, check_degraded_status);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* We always continue when we are degraded.
     * Make the opcode of any degraded positions be nop. 
     * This is needed since we might be retrying fruts below. 
     */
    fbe_raid_fruts_set_degraded_nop(siots_p, read_fruts_p);

    /* See if we still have any retries left for those frus with hard errors.
     */
    if (eboard_p->hard_media_err_count == 0)
    {
        /* Handle dead errors greater than zero on R6.
         */
    }
    else if ((eboard_p->hard_media_err_count > 0) &&
             (siots_p->algorithm == R5_DEG_VR) &&
             !fbe_raid_is_aligned_to_optimal_block_size(siots_p->parity_start,
                                                        siots_p->parity_count,
                                                        fbe_raid_siots_get_raid_geometry(siots_p)->optimal_block_size))
    {
        /* We are a degraded verify and did not align to the optimum block size since
         * we are in the I/O path and should not increase the range of the I/O.
         * Now that we have received a media error, we need to extend our lock in
         * order to deal with the error.  Return an error to the caller to force them
         * to retry the verify with an expanded range.
         */
        siots_p->error = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR;
        fbe_raid_siots_transition(siots_p, fbe_parity_verify_state25);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }
    else if (eboard_p->hard_media_err_count == 1 &&
             !fbe_raid_siots_is_flag_set(siots_p, FBE_RAID_SIOTS_FLAG_SINGLE_ERROR_RECOVERY))
    {
        /* If we just took a single hard error,
         * then it may be possible to use redundancy to recover.
         */
    }
    else if (eboard_p->hard_media_err_count > 0)
    {
        if (eboard_p->hard_media_err_count > fbe_raid_siots_get_width(siots_p))
        {
            fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "parity: error count more than width 0x%x 0x%x", 
                                                eboard_p->hard_media_err_count, fbe_raid_siots_get_width(siots_p));
            return FBE_RAID_STATE_STATUS_EXECUTING;
        }

        /* If we are here we are doing error recovery. Set the single error recovery flag
         * since we already have an error and do not want to short ciruit any of the error 
         * handling for efficiency. 
         */
        fbe_raid_siots_set_flag(siots_p, FBE_RAID_SIOTS_FLAG_SINGLE_ERROR_RECOVERY);

        if (!fbe_raid_siots_is_flag_set(siots_p, FBE_RAID_SIOTS_FLAG_SINGLE_REGION_MODE))
        {
            /* If we cross a region boundary, then we need to drop into a mining mode
             * where we mine the request on the optimal block size (region size). 
             */
            fbe_raid_siots_set_flag(siots_p, FBE_RAID_SIOTS_FLAG_SINGLE_REGION_MODE);

            siots_p->parity_count = fbe_raid_siots_get_parity_count_for_region_mining(siots_p);
            if(siots_p->parity_count == 0)
            { 
                /* we must have crossed the 32bit limit while calculating the region size
                 */
                fbe_raid_siots_set_unexpected_error(siots_p, 
                                                    FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                    "mirror: siots_p->parity_count == 0 \n");
                return(FBE_RAID_STATE_STATUS_EXECUTING);
            }

            /* For each fruts, reset it to a single sector,
             * and make minor reset changes.
             */
            status = fbe_raid_fruts_for_each(0,
                                             read_fruts_p,
                                             fbe_raid_fruts_reset,
                                            (fbe_u64_t) siots_p->parity_count,
                                            (fbe_u64_t) FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ);
            
            if (RAID_COND(status != FBE_STATUS_OK))
            {
                fbe_raid_siots_set_unexpected_error(siots_p, 
                                                    FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                    "parity: %s unexpected error for fruts chain read_fruts_p = 0x%p, "
                                                    "siots_p 0x%p, status = 0x%x\n",
                                                    __FUNCTION__,
                                                    read_fruts_p,
                                                    siots_p, 
                                                    status);
                 return FBE_RAID_STATE_STATUS_EXECUTING;
            }

            fbe_parity_verify_setup_sgs_for_next_pass(siots_p);
            /* Transition back up to state3.
             */
            fbe_raid_siots_transition(siots_p, fbe_parity_verify_state3);

            return FBE_RAID_STATE_STATUS_EXECUTING;
        }
    }    /* END else if (eboard_p->hard_media_err_count > 0) */
    else
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "unexpected fru error status");
        fbe_raid_fru_eboard_display(eboard_p, siots_p, FBE_TRACE_LEVEL_ERROR);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }
    return FBE_RAID_STATE_STATUS_DONE;
}
/******************************************
 * end fbe_parity_verify_handle_read_error()
 ******************************************/
/*!***************************************************************
 * fbe_parity_verify_state4()
 ****************************************************************
 * @brief
 *  This function will evaluate the results from reads.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t
 *
 * @return fbe_raid_state_status_t
 *
 * @author
 *  4/26/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_parity_verify_state4(fbe_raid_siots_t * siots_p)
{
    fbe_raid_fru_eboard_t eboard;
    fbe_raid_fru_error_status_t status;
    fbe_raid_fruts_t *read_fruts_p = NULL;
    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_p);

    fbe_raid_siots_log_traffic(siots_p, "parity_verify_state4");
    if (siots_p->wait_count != 0)
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS, 
                                            "parity: wait count unexpected 0x%llx", siots_p->wait_count);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    fbe_raid_fru_eboard_init(&eboard);

    /* Reads have completed. Evaluate the status of the reads.
     */
    status = fbe_parity_get_fruts_error(read_fruts_p, &eboard);

    if (status == FBE_RAID_FRU_ERROR_STATUS_SUCCESS)
    {
        /* Success. Continue
         */
        fbe_raid_siots_transition(siots_p, fbe_parity_verify_state4a);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }
    else if ((status == FBE_RAID_FRU_ERROR_STATUS_SHUTDOWN) ||
             (status == FBE_RAID_FRU_ERROR_STATUS_DEAD) ||
             (status == FBE_RAID_FRU_ERROR_STATUS_WAITING) ||
             fbe_raid_siots_is_aborted(siots_p))
    {
        /* We check for aborted directly in siots since we are at the read phase and 
         * regardless of the operation we should abort the request. 
         */
        fbe_raid_state_status_t state_status;
        state_status = fbe_parity_handle_fruts_error(siots_p, read_fruts_p, &eboard, status);
        return state_status;
    }
    else if (status == FBE_RAID_FRU_ERROR_STATUS_RETRY)
    {
        fbe_status_t retry_status;
        fbe_u32_t degraded_count;
        fbe_u16_t parity_disks;

         /* We might have gone degraded and have a retry.
          * In this case it is necessary to mark the degraded positions as nops since 
          * 1) we already marked the paged for these 
          * 2) the next time we run through this state we do not want to consider these failures 
          *    (and wait for continue) since we already got the continue. 
          */
        fbe_raid_fruts_set_degraded_nop(siots_p, read_fruts_p);

        degraded_count = fbe_raid_siots_get_degraded_count(siots_p);
        fbe_raid_geometry_get_parity_disks(fbe_raid_siots_get_raid_geometry(siots_p), &parity_disks);

        /* If we do not have too many errored positions we can 
         * try to use redundancy to recover. 
         */
        if (((degraded_count + eboard.retry_err_count + eboard.hard_media_err_count) <= parity_disks) &&
            (!fbe_raid_siots_is_flag_set(siots_p, FBE_RAID_SIOTS_FLAG_SINGLE_ERROR_RECOVERY)))
        {
            /* Try to use redundancy to recover.
             */
            fbe_raid_siots_transition(siots_p, fbe_parity_verify_state4a);
            return FBE_RAID_STATE_STATUS_EXECUTING;
        }
        else
        {
            //fbe_raid_siots_set_flag(siots_p, FBE_RAID_SIOTS_FLAG_SINGLE_ERROR_RECOVERY);
            /* Send out retries.  We will stop retrying if we get aborted or 
             * our timeout expires or if an edge goes away. 
             */
            retry_status = fbe_raid_siots_retry_fruts_chain(siots_p, &eboard, read_fruts_p);
            if (retry_status != FBE_STATUS_OK)
            {
                fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                    "parity: retry %d errors (bitmap 0x%x) status: %d unexpected\n", 
                                                    eboard.retry_err_count, eboard.retry_err_bitmap, retry_status);
                fbe_raid_fru_eboard_display(&eboard, siots_p, FBE_TRACE_LEVEL_ERROR);
                return FBE_RAID_STATE_STATUS_EXECUTING;
            }
            return FBE_RAID_STATE_STATUS_WAITING;
        }
    }
    else if (status == FBE_RAID_FRU_ERROR_STATUS_ERROR)
    {
        fbe_raid_state_status_t state_status;

        state_status = fbe_parity_verify_handle_read_error(siots_p, &eboard);
        
        if (state_status == FBE_RAID_STATE_STATUS_DONE)
        {
            /* Done status will do the verify.
             */ 
            fbe_raid_siots_transition(siots_p, fbe_parity_verify_state4a);
            return FBE_RAID_STATE_STATUS_EXECUTING;
        }
        else
        {
            /* Other status set by above call.  State also set by above call.
             */
            return state_status;
        }
    } /* END else if (status == FBE_RAID_FRU_ERROR_STATUS_ERROR) */
    else
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "unexpected status status: 0x%x", status);
        fbe_raid_fru_eboard_display(&eboard, siots_p, FBE_TRACE_LEVEL_ERROR);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }
}
/* end of fbe_parity_verify_state4() */

/*!***************************************************************
 * fbe_parity_verify_state4a()
 ****************************************************************
 * @brief
 *  This function does the parity verify.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t
 *
 * @return fbe_raid_state_status_t
 *
 * @author
 *  4/26/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_parity_verify_state4a(fbe_raid_siots_t * siots_p)
{
    fbe_status_t status;
    fbe_status_t check_degraded_status;
    fbe_raid_fruts_t *read_fruts_p = NULL;
    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_p);

    /* See if we are degraded on this LUN.
     */
    check_degraded_status = fbe_parity_check_degraded_phys(siots_p);
    if (RAID_COND(check_degraded_status != FBE_STATUS_OK))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity: %s failed to check degraded status: 0x%x\n", 
                                            __FUNCTION__, check_degraded_status);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* We always continue when we are degraded.
     * Make the opcode of any degraded positions be nop. 
     * This is needed since we might be retrying fruts below. 
     */
    fbe_raid_fruts_set_degraded_nop(siots_p, read_fruts_p);

    fbe_raid_siots_transition(siots_p, fbe_parity_verify_state5);

    (void) fbe_raid_fruts_get_media_err_bitmap(read_fruts_p,
                                               &siots_p->misc_ts.cxts.eboard.hard_media_err_bitmap);
    /* Call xor to perform the verify.
     */
    status = fbe_parity_verify_strip(siots_p);

    if (status != FBE_STATUS_OK) 
    { 
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity: unexpected verify error 0x%x\n", status);
    }
    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/**************************************
 * end fbe_parity_verify_state4a()
 **************************************/
/*!***************************************************************
 *  fbe_parity_verify_state5()
 ****************************************************************
 *  @brief
 *    Have identified an errored sector. Invalidate it.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t
 *
 * @return
 *  This function returns  FBE_RAID_STATE_STATUS_EXECUTING
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_parity_verify_state5(fbe_raid_siots_t * siots_p)
{
    fbe_lba_t seed;
    fbe_u16_t write_bitmap = siots_p->misc_ts.cxts.eboard.w_bitmap;
    fbe_u16_t hard_media_err_bitmap;
    fbe_raid_fruts_t *read_fruts_p = NULL;
    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_p);

    if ((fbe_parity_verify_get_write_bitmap(siots_p) != 0) &&
        (siots_p->algorithm == R5_DEG_VR) &&
        !fbe_raid_is_aligned_to_optimal_block_size(siots_p->parity_start,
                                             siots_p->parity_count,
                                             fbe_raid_siots_get_raid_geometry(siots_p)->optimal_block_size))
    {
        /* We are a degraded verify and did not align to the optimum block size since
         * we are in the I/O path and should not increase the range of the I/O.
         * Now that we have received a media error, we need to extend our lock in
         * order to deal with the error.  Return an error to the caller to force them
         * to retry the verify with an expanded range. 
         *  
         * We intentionally perform this check here before any retries are made. 
         */
        siots_p->error = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR;
        fbe_raid_siots_transition(siots_p, fbe_parity_verify_state25);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    (void) fbe_raid_fruts_get_media_err_bitmap(read_fruts_p, &hard_media_err_bitmap);

    /* Save the write bitmap since we will be writing in
     * a different state.
     */
    siots_p->misc_ts.cxts.eboard.w_bitmap = write_bitmap;

    fbe_raid_siots_log_traffic(siots_p, "parity_verify_state5");
    if ( siots_p->misc_ts.cxts.eboard.u_bitmap != 0 &&
        (!fbe_raid_siots_is_flag_set(siots_p, FBE_RAID_SIOTS_FLAG_SINGLE_ERROR_RECOVERY)) &&
         ( hard_media_err_bitmap != 0 ) &&
         siots_p->parity_count > 1 )
    {
        fbe_raid_siots_log_traffic(siots_p, "parity_verify_state5 retry for single error recovery");
        /* We attempted to use redundancy to fix a media error,
         * but this failed because of uncorrectable errors.
         * Since this was unsuccessful, we have no choice
         * but to take heroic measures by going into
         * single strip mode.
         */
        fbe_raid_siots_set_flag(siots_p, FBE_RAID_SIOTS_FLAG_SINGLE_ERROR_RECOVERY);
        fbe_raid_siots_transition(siots_p, fbe_parity_verify_state4);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    if ( (!fbe_raid_siots_is_flag_set(siots_p, FBE_RAID_SIOTS_FLAG_SINGLE_REGION_MODE) ||
          fbe_raid_siots_is_retry( siots_p )) &&
         !fbe_raid_csum_handle_csum_error( siots_p, read_fruts_p, 
                                           FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                           fbe_parity_verify_state4) )
    {
        /* We needed to retry the fruts and the above function
         * did so. We are waiting for the retries to finish.
         * Transition to state 4 to evaluate retries.
         */

        return FBE_RAID_STATE_STATUS_WAITING;
    }

    seed = siots_p->parity_start;

    if (fbe_parity_record_errors(siots_p, seed) != FBE_RAID_STATE_STATUS_DONE)
    {
        /* We hit an error in the validation code.  We're going
         * to fbe_panic, but we're retrying the reads
         * so we will have something to print out in the ktrace.
         */
        return FBE_RAID_STATE_STATUS_WAITING;
    }

    if ((siots_p->algorithm == R5_468_VR) || 
        (siots_p->algorithm == R5_MR3_VR) ||
        (siots_p->algorithm == R5_RCW_VR) ||
        (siots_p->algorithm == R5_DEG_RVR))
    {
        /* Transition to a different state to handle write recovery verify.
         */
        fbe_raid_siots_transition(siots_p, fbe_parity_verify_state6);
    }
    else
    {
        fbe_raid_siots_transition(siots_p, fbe_parity_verify_state8);
    }

    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/* end of fbe_parity_verify_state5() */

/*!***************************************************************
 *  fbe_parity_verify_state6()
 ****************************************************************
 * @brief
 *  Recovery verify write.  Perform copy of new write data into
 *  stripe, and then perform construction of new parity.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t
 *
 * @return
 *  This function returns  FBE_RAID_STATE_STATUS_EXECUTING
 *
 ****************************************************************/
fbe_raid_state_status_t fbe_parity_verify_state6(fbe_raid_siots_t * siots_p)
{
    fbe_status_t status;
    fbe_xor_status_t xor_status;
    fbe_raid_siots_transition(siots_p, fbe_parity_verify_state7);
    fbe_raid_siots_set_flag(siots_p, FBE_RAID_SIOTS_FLAG_WROTE_CORRECTIONS);

    /* We'll finish the write in the verify state machine.
     * Copy of the host data, and assign checksum only, and
     * Parity construction will be done below.  
     */
    status = fbe_parity_recovery_verify_copy_user_data(siots_p);

    if (status != FBE_STATUS_OK)
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity: %s unexpected verify copy status 0x%x for siots_p 0x%p\n", 
                                            __FUNCTION__,
                                            status,
                                            siots_p);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    fbe_raid_siots_get_xor_command_status(siots_p, &xor_status);

    if (xor_status & FBE_XOR_STATUS_BAD_MEMORY)
    {
        fbe_raid_siots_object_trace(siots_p, FBE_RAID_LIBRARY_TRACE_PARAMS_WARNING,
                                    "Checksum error on write, fail cmd lba: 0x%llx bl: 0x%llx\n",
                                    siots_p->start_lba, siots_p->xfer_count);

        fbe_raid_siots_transition(siots_p, fbe_raid_siots_write_crc_error);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }
    /* Do the parity construction.
     */
    status = fbe_parity_recovery_verify_construct_parity(siots_p);

    if (status != FBE_STATUS_OK)
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity: %s unexpected rvr construct parity status 0x%x for siots_p = 0x%p", 
                                            __FUNCTION__,
                                            status,
                                            siots_p);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }
    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/* end fbe_parity_verify_state6() */

/*!***************************************************************
 *  fbe_parity_verify_state7()
 ****************************************************************
 *  @brief
 *    Last part of rvr. Write out the data.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t
 *
 *  @return
 *    This function returns  FBE_RAID_STATE_STATUS_EXECUTING
 *
 *  @author
 *    7/12/00 - Created. kls
 *    7/14/00 - RFoley, use RG_SIOTS_WIDTH.
 *
 ****************************************************************/
fbe_raid_state_status_t fbe_parity_verify_state7(fbe_raid_siots_t * siots_p)
{
    fbe_u16_t w_bitmap;
    fbe_raid_fruts_t *read_fruts_p = NULL;
    fbe_u16_t degraded_bitmask;
    fbe_status_t status = FBE_STATUS_OK;

    fbe_raid_siots_get_degraded_bitmask(siots_p, &degraded_bitmask);
    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_p);

    /* Since we just finished a recovery verify
     * we will set the w_bitmap to all of the fru's in the array minus any degraded positions.
     */
    w_bitmap = 0xffff >> (16 - fbe_raid_siots_get_width(siots_p));
    w_bitmap &= ~degraded_bitmask;

    /* Write verified data back to disk.
     */

    /* Tally total writes to submit.
     */
    siots_p->wait_count = fbe_raid_get_bitcount(w_bitmap);
    /* Make sure we found positions to be submitted.
     */
    if (siots_p->wait_count == 0)
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS, 
                                            "parity: wait count unexpected 0x%llx", siots_p->wait_count);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }
    if (siots_p->wait_count > fbe_queue_length(&siots_p->read_fruts_head))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS, 
                                            "parity: wait count unexpected 0x%llx 0x%x", 
                                            siots_p->wait_count, fbe_queue_length(&siots_p->read_fruts_head));
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }
    /* Wait in state10
     * for writes to complete.
     */
    fbe_raid_siots_transition(siots_p, fbe_parity_verify_state10);

    /* Slam the opcode to write on all fruts,
     * so our entire chain will be writes.
     */
    status = fbe_raid_fruts_for_each(0xFFFF,
                                     read_fruts_p,
                                     fbe_raid_fruts_set_opcode,
                                     (fbe_u64_t) FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,
                                     (fbe_u64_t) NULL);

    if (RAID_COND(status != FBE_STATUS_OK))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity: %s failed to set opcode for siots_p 0x%p, status = 0x%x\n",
                                            __FUNCTION__,
                                            siots_p, 
                                            status);
         return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    fbe_raid_fruts_set_degraded_nop(siots_p, read_fruts_p);

    /* Submit only the modified positions.
     */
    status = fbe_raid_fruts_for_each(w_bitmap,
                                   read_fruts_p,
                                   fbe_raid_fruts_retry,
                             (fbe_u64_t) FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,
                             (fbe_u64_t) fbe_raid_fruts_evaluate);
    
    if (RAID_FRUTS_COND(status != FBE_STATUS_OK))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity: %s failed to retry fruts chain read_fruts_p = 0x%p. siots_p 0x%p, status = 0x%x\n",
                                            __FUNCTION__,
                                            read_fruts_p,
                                            siots_p, 
                                            status);
         return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    return FBE_RAID_STATE_STATUS_WAITING;
}
/* end fbe_parity_verify_state7() */

/*!***************************************************************
 *  fbe_parity_verify_state8()
 ****************************************************************
 * @brief
 *  This function will do any necessary rebuild logging for writes
 *  and either continue on (recovery vr for write), or
 *  issue the writes from here.
 *
 * @param siots_p - Pointer to fbe_raid_siots_t.
 *
 * @return
 *  FBE_RAID_STATE_STATUS_WAITING
 *
 * @author
 *  10/06/06 - Created. Rob Foley
 *  07/03/08 - Modified for Rebuild Logging Support For R6. Rob Foley
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_parity_verify_state8(fbe_raid_siots_t * siots_p)
{
    fbe_status_t fbe_status = FBE_STATUS_OK;
    fbe_raid_state_status_t status;
    fbe_u16_t write_bitmap = 0;
    fbe_u16_t hard_media_err_bitmap;
    fbe_raid_fruts_t *read_fruts_p = NULL;
    fbe_raid_fruts_t *write_fruts_p = NULL;
    fbe_payload_block_operation_opcode_t opcode;

    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_p);
    fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_p);
    fbe_raid_siots_get_opcode(siots_p, &opcode);

    (void) fbe_raid_fruts_get_media_err_bitmap(read_fruts_p, &hard_media_err_bitmap);

    /* We have detected media errors as part of verify or
     * read recovery verify. Below we will decide if we should
     * write any corrections we've made back to disk.
     *
     * If we took hard errors, then the remap thread will handle fixing errors,
     * so we normally skip the write.  The reason we skip the write here is that
     * if we performed the write, then the disk may go away for a long time
     * doing possibly multiple blocks worth of remapping.  Instead we will let
     * the remap thread do the remaps one at a time, and also verify.
     *
     * If the checksum error reconstruct flag is set, then
     * the user is asking us to do all the remaps right now
     * as part of an FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY (R5_VR algorithm only), so
     * we will just perform any writes that are needed immediately.
     * 
     * Read recovery verify (R5_RD_VR), normal verify (R5_VR),
     * and remap verify R5_RD_REMAP all will not write out the changes
     * if hard (media) errors are involved.  If hard (media) errors occurred,
     * then there may be many within the stripe and writing these blocks
     * will cause the blocks to be remapped, which may take a long time even
     * if there is just a single block to be remapped.
     * Since we do not want to delay host I/Os for remaps, we
     * will not write out the changes here and we will allow the remap
     * thread to take care of these remap errors one at a time later.
     */
    write_bitmap = fbe_parity_verify_get_write_bitmap(siots_p);

    if (write_bitmap && (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ))
    {
        /* Mark that we need to verify this strip outside the I/O path.
         * Reads cannot write out corrections since they do not have a write lock. 
         */
        fbe_raid_iots_t *iots_p = fbe_raid_siots_get_iots(siots_p);
        fbe_raid_iots_set_flag(iots_p, FBE_RAID_IOTS_FLAG_REMAP_NEEDED);
    }

    /* We will skip the writes here if we are doing Read Only Background Verify.
     * We also skip writes if this is a read since reads do not write out corrections.
     * We also skip writes if media errors were encountered and our algorithm is one 
     * that we do not wish to write over media errors. 
     */
    if ( write_bitmap && 
         (! fbe_raid_siots_is_read_only_verify(siots_p)) &&
         (opcode != FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ) &&
         ( (hard_media_err_bitmap == 0) || 
           (siots_p->algorithm == R5_VR) ||
           (siots_p->algorithm == R5_DEG_VR) ||
           (siots_p->algorithm == R5_DEG_RVR) ||
           (siots_p->algorithm == R5_DRD_VR)  ||
           (siots_p->algorithm == RG_FLUSH_JOURNAL_VR) ) )
    {
        fbe_u16_t nop_bitmask = 0;    /* Bitmask of nop commands in read fruts chain. */
        fbe_u32_t write_opcode;
        if (siots_p->algorithm == R5_VR)
        {
            /* A verify does a write verify which can fail with a media error.
             */
            write_opcode = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_VERIFY;
        }
        else
        { 
            write_opcode = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE;
        }
        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_INFO,
                                    "%s siots: %p p_start: 0x%llx p_count: 0x%llx w_bitmap: 0x%x\n", 
                                     __FUNCTION__, siots_p,
				     (unsigned long long)siots_p->parity_start,
				     (unsigned long long)siots_p->parity_count,
				     write_bitmap);

        /* Positions modified, need to write out changes.
         * We will submit the read chain for the write. 
         */

        /* Save the bitmask of nop commands.
         * We need to save this before NOP is set to write below.
         * This nop bitmask is used below to set pass thru flag.
         */
        fbe_raid_fruts_chain_nop_find(read_fruts_p, &nop_bitmask);

        /* Tally total writes to submit.
         */
        siots_p->wait_count = fbe_raid_get_bitcount(write_bitmap);

        /* Make sure we found positions to be submitted.
         */
        if (siots_p->wait_count == 0)
        {
            fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS, 
                                                "parity: wait count unexpected 0x%llx", siots_p->wait_count);
            return FBE_RAID_STATE_STATUS_EXECUTING;
        }
        if (siots_p->wait_count > fbe_queue_length(&siots_p->read_fruts_head))
        {
            fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS, 
                                                "parity: wait count unexpected 0x%llx 0x%x", 
                                                siots_p->wait_count, fbe_queue_length(&siots_p->read_fruts_head));
            return FBE_RAID_STATE_STATUS_EXECUTING;
        }
        /* Set all fruts to WRITE opcode, clear flags.
         */
        fbe_status = fbe_raid_fruts_for_each(write_bitmap,
                                           read_fruts_p,
                                           fbe_raid_fruts_set_opcode,
                                           (fbe_u64_t) write_opcode,
                                           (fbe_u64_t) NULL);
        
        if (RAID_COND(fbe_status != FBE_STATUS_OK))
        {
            fbe_raid_siots_set_unexpected_error(siots_p, 
                                                FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "parity: %s failed to set opcode.read_fruts_p = 0x%p, "
                                                "siots_p 0x%p, status = 0x%x\n",
                                                __FUNCTION__,
                                                read_fruts_p,
                                                siots_p, 
                                                fbe_status);
             return FBE_RAID_STATE_STATUS_EXECUTING;
        }


        /* Set all degraded positions to nop.
         */
        fbe_raid_fruts_set_degraded_nop(siots_p, read_fruts_p);

#if 0
        /* Handle Proactive Case where we need to
         * Set pass thru flag for every nop in chain.
         */
        rg_ps_vr_set_pass_thru( siots_p, read_fruts_p, nop_bitmask );
#endif

        fbe_raid_siots_transition(siots_p, fbe_parity_verify_state10);
        /* Submit only the modified positions.
         * Preserves flags set above by rg_ps_vr_set_pass_thru.
         */
        fbe_status = fbe_raid_fruts_for_each(write_bitmap,
                                           read_fruts_p,
                                           fbe_raid_fruts_retry_preserve_flags,
                                           (fbe_u64_t) write_opcode,
                                           (fbe_u64_t) fbe_raid_fruts_evaluate);
        
        if (RAID_FRUTS_COND(fbe_status != FBE_STATUS_OK))
        {
            fbe_raid_siots_set_unexpected_error(siots_p, 
                                                FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "parity: %s failed to retry fruts chain read_fruts_p = 0x%p, "
                                                "siots_p 0x%p, status = 0x%x\n",
                                                __FUNCTION__,
                                                read_fruts_p,
                                                siots_p, 
                                                fbe_status);
             return FBE_RAID_STATE_STATUS_EXECUTING;
        }

        /* Wait in state10
         * for writes to complete.
         */
        status = FBE_RAID_STATE_STATUS_WAITING;
    }
    else
    {
        /* Continue with verify, since no write is necessary.
         * If we are strip mining, goto state19,
         * else we are finished, goto state20
         */
        (siots_p->xfer_count - siots_p->parity_count)
        ? fbe_raid_siots_transition(siots_p, fbe_parity_verify_state19)
        : fbe_raid_siots_transition(siots_p, fbe_parity_verify_state20);

        status = FBE_RAID_STATE_STATUS_EXECUTING;
    }
    return status;
}
/*****************************
 * fbe_parity_verify_state8()
 *****************************/
/*!***************************************************************
 * fbe_parity_verify_state10()
 ****************************************************************
 * @brief
 *  This function will evaluate the results from write.
 *  If no dead drive or hard errors are encountered,
 *  we'll either finish the verify or go to next strip.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t
 *
 * @return fbe_raid_state_status_t
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_parity_verify_state10(fbe_raid_siots_t * siots_p)
{
    fbe_raid_fru_eboard_t eboard;
    fbe_raid_fru_error_status_t status;
    fbe_raid_state_status_t state_status = FBE_RAID_STATE_STATUS_EXECUTING;
    fbe_raid_fruts_t *read_fruts_p = NULL;
    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_p);

    fbe_raid_fru_eboard_init(&eboard);

    if (siots_p->wait_count != 0)
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS, 
                                            "parity: wait count unexpected 0x%llx", siots_p->wait_count);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* We have written all the data;
     * evaluate them first.
     */
    status = fbe_parity_get_fruts_error(read_fruts_p, &eboard);
    if (status == FBE_RAID_FRU_ERROR_STATUS_SUCCESS)
    {
        if ((eboard.soft_media_err_count > 0) && (siots_p->algorithm == R5_VR))
        {
            /* Verifies that get a soft media error on the write will fail 
             * back to verify so that verify will retry this.  This limits 
             * the number of media errors we try to fix at once. 
             */
            fbe_raid_siots_transition(siots_p, fbe_parity_verify_state22);
            return FBE_RAID_STATE_STATUS_EXECUTING;
        }
        /* We'll continue with verify.
         */
        (siots_p->xfer_count - siots_p->parity_count)
            ? fbe_raid_siots_transition(siots_p, fbe_parity_verify_state19)
            : fbe_raid_siots_transition(siots_p, fbe_parity_verify_state20);

    }
    else if (status == FBE_RAID_FRU_ERROR_STATUS_RETRY)
    {
        fbe_raid_state_status_t state_status;

         /* We might have gone degraded and have a retry.
          * In this case it is necessary to mark the degraded positions as nops since 
          * 1) we already marked the paged for these 
          * 2) the next time we run through this state we do not want to consider these failures 
          *    (and wait for continue) since we already got the continue. 
          */
        fbe_raid_fruts_set_degraded_nop(siots_p, read_fruts_p);
        if (fbe_raid_siots_is_aborted(siots_p))
        {
            fbe_parity_report_errors(siots_p, FBE_TRUE);
            /* The siots has been aborted.
             */
            fbe_raid_siots_transition(siots_p, fbe_raid_siots_aborted);
            /* After reporting errors, also sends Usurper FBE_BLOCK_TRANSPORT_CONTROL_CODE_UPDATE_LOGICAL_ERROR_STATS
             * to PDO if single or multibit CRC errors found
             */
           if(fbe_raid_siots_pdo_usurper_needed(siots_p))
           {
               fbe_status_t send_crc_status = fbe_raid_siots_send_crc_usurper(siots_p);
               /*We are waiting only when usurper has been sent, else executing should be returned*/
               if (send_crc_status == FBE_STATUS_OK)
               {
                   return FBE_RAID_STATE_STATUS_WAITING;
               }
               else
               {
                   return FBE_RAID_STATE_STATUS_EXECUTING;
               }
           }
        }
        state_status = fbe_parity_handle_fruts_error(siots_p, read_fruts_p, &eboard, status);
        return state_status;
    }
    else if ((status == FBE_RAID_FRU_ERROR_STATUS_SHUTDOWN) ||
             (status == FBE_RAID_FRU_ERROR_STATUS_DEAD) ||
             (status == FBE_RAID_FRU_ERROR_STATUS_WAITING) ||
             fbe_raid_siots_is_aborted(siots_p))
    {
        fbe_raid_state_status_t state_status;
        if (fbe_raid_siots_is_aborted(siots_p))
        {
            fbe_parity_report_errors(siots_p, FBE_TRUE);
            /* The siots has been aborted.
             */
            fbe_raid_siots_transition(siots_p, fbe_raid_siots_aborted);
            /* After reporting errors, also sends Usurper FBE_BLOCK_TRANSPORT_CONTROL_CODE_UPDATE_LOGICAL_ERROR_STATS
             * to PDO if single or multibit CRC errors found
             */
           if(fbe_raid_siots_pdo_usurper_needed(siots_p))
           {
               fbe_status_t send_crc_status = fbe_raid_siots_send_crc_usurper(siots_p);
               /*We are waiting only when usurper has been sent, else executing should be returned*/
               if (send_crc_status == FBE_STATUS_OK)
               {
                   return FBE_RAID_STATE_STATUS_WAITING;
               }
               else
               {
                   /* This will return from here, fbe_parity_handle_fruts_error will ultimately return executing status*/
                   return FBE_RAID_STATE_STATUS_EXECUTING;
               }
           }
        }
        state_status = fbe_parity_handle_fruts_error(siots_p, read_fruts_p, &eboard, status);
        return state_status;
    }
    else if (status == FBE_RAID_FRU_ERROR_STATUS_ERROR)
    {
        if (eboard.dead_err_count > 0)
        {
            if ((siots_p->algorithm == R5_RD_VR || siots_p->algorithm == R5_DRD_VR) &&
                (siots_p->xfer_count - siots_p->parity_count) == 0)
            {
                /***************************************************
                 * Finish verify with good status.
                 *
                 * Note: This is a read request,
                 *       since we have all the corrected data in place,
                 *       we can finish verifying the read data.
                 *
                 ***************************************************/
                fbe_raid_siots_transition(siots_p, fbe_parity_verify_state20);
            }
            else
            {
                /* For all raid types, we can still continue with degraded verify.
                fbe_raid_siots_transition(siots_p, fbe_parity_verify_state11);
                 */
                (siots_p->xfer_count - siots_p->parity_count)
                    ? fbe_raid_siots_transition(siots_p, fbe_parity_verify_state19)
                    : fbe_raid_siots_transition(siots_p, fbe_parity_verify_state20);
            }
        }
        else if ((siots_p->algorithm == R5_VR) && (eboard.hard_media_err_count > 0))
        {
            /* Read remaps do write verifies, so they can get media errors.
             * Simply transition to return status. 
             */
            fbe_raid_siots_transition(siots_p, fbe_parity_verify_state22);
        }
        else
        {
            fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "parity: unexpected error: 0x%x\n", status);
            fbe_raid_fru_eboard_display(&eboard, siots_p, FBE_TRACE_LEVEL_ERROR);
            return FBE_RAID_STATE_STATUS_EXECUTING;
        }
    }/* end else if (status == FBE_RAID_FRU_ERROR_STATUS_ERROR)*/
    else
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity: unexpected status: 0x%x\n", status);
        fbe_raid_fru_eboard_display(&eboard, siots_p, FBE_TRACE_LEVEL_ERROR);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }
    return state_status;
}
/*!****************************************
 * end of fbe_parity_verify_state10()
 *****************************************/

/*!***************************************************************
 *  fbe_parity_verify_state19()
 ****************************************************************
 * @brief
 *  We are in strip mine mode, and more strips are required.
 *  Setup for next strip.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t 
 *
 * @return
 *  This function always returns FBE_RAID_STATE_STATUS_EXECUTING.
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_parity_verify_state19(fbe_raid_siots_t * siots_p)
{
    fbe_raid_iots_t    *iots_p = fbe_raid_siots_get_iots(siots_p);
    fbe_block_count_t increment_count = siots_p->parity_count;
    fbe_raid_fruts_t *read_fruts_p = NULL;
    fbe_block_count_t blocks_remaining = siots_p->xfer_count - siots_p->parity_count;
    fbe_status_t status = FBE_STATUS_OK;

    if (siots_p->parity_count > siots_p->xfer_count)
    {
        /* We should not have been here. Somtething wrong happened!
         */
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity: blocks remaining is unexpected 0x%llx 0x%llx 0x%llx\n", 
                                            (unsigned long long)blocks_remaining,
					    (unsigned long long)siots_p->xfer_count,
					    (unsigned long long)siots_p->parity_count);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_p);
    if (read_fruts_p == NULL)
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,"parity: read fruts is null\n");
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }
    if (read_fruts_p->blocks != siots_p->parity_count)
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS, 
                                            "parity: read fruts blocks unexpected: 0x%llx 0x%llx\n", 
                                            (unsigned long long)read_fruts_p->blocks,
					    (unsigned long long)siots_p->parity_count);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /********************************************
     * Recall that xfer count is decremented
     * for each stripe or strip verified.
     * Xfer count contains the blocks remaining
     * in the current SIOTS.
     ********************************************/
    siots_p->xfer_count -= siots_p->parity_count;
    siots_p->parity_start += siots_p->parity_count;

    /* We will do following:
     *      1. Report errors because of previous strip mining operation
     *      2. Reset the eboard used in previous strip operation
     *      3. Set up the fruts tracking strutures for next region.
     */
    if (fbe_raid_siots_is_flag_set(siots_p, FBE_RAID_SIOTS_FLAG_SINGLE_REGION_MODE))
    {
        /* Report error in event log. We do so here as each strip-region 
         * operation is treated independently and hene xor operation 
         * will be performed independently.
         */
        fbe_parity_report_errors(siots_p, FBE_FALSE);

        if (siots_p->algorithm == R5_VR)
        {
            /* Since this is single strip and external,
             * advance blocks transferred.
             */
            fbe_raid_iots_lock(iots_p);
            iots_p->blocks_transferred += siots_p->parity_count;
            fbe_raid_iots_unlock(iots_p);

            /*****************************************
             * External single strip verify has straighforward setup. 
             * Just set the fruts flags and status. sg just stays in the same place.
            *****************************************/
        }

        
        /* Set the parity count to the correct value for mining a region.
         */
        siots_p->parity_count = fbe_raid_siots_get_parity_count_for_region_mining(siots_p);

    } /* end if (fbe_raid_siots_is_flag_set(siots_p, FBE_RAID_SIOTS_FLAG_SINGLE_REGION_MODE) */

    if ((siots_p->parity_count == 0) || (siots_p->parity_count > siots_p->xfer_count))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity: parity count is unexpected 0x%llx 0x%llx\n", 
                                            (unsigned long long)siots_p->parity_count,
					    (unsigned long long)siots_p->xfer_count);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    status = fbe_raid_fruts_for_each(0,
                                     read_fruts_p,
                                     fbe_raid_fruts_reset_rd,
                                     increment_count,
                                     siots_p->parity_count);
    
    if (RAID_COND(status != FBE_STATUS_OK))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity: %s unexpected error for fruts chain read_fruts_p = 0x%p, "
                                            "siots_p 0x%p, status = 0x%x\n",
                                            __FUNCTION__,
                                            read_fruts_p,
                                            siots_p, 
                                            status);
         return FBE_RAID_STATE_STATUS_EXECUTING;
    }


    fbe_parity_verify_setup_sgs_for_next_pass(siots_p);

    /* Go back to state 3 to continue the verify.
     */
    fbe_raid_siots_transition(siots_p, fbe_parity_verify_state3);
    /* After reporting errors, also sends Usurper FBE_BLOCK_TRANSPORT_CONTROL_CODE_UPDATE_LOGICAL_ERROR_STATS
     * to PDO if single or multibit CRC errors found
     */
    if ( (fbe_raid_siots_is_flag_set(siots_p, FBE_RAID_SIOTS_FLAG_SINGLE_REGION_MODE)) &&
        (fbe_raid_siots_pdo_usurper_needed(siots_p)) )
    {
       fbe_status_t send_crc_status = fbe_raid_siots_send_crc_usurper(siots_p);
       /*We are waiting only when usurper has been sent, else executing should be returned*/
       if (send_crc_status == FBE_STATUS_OK)
       {
           return FBE_RAID_STATE_STATUS_WAITING;
       }
    }
    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/*****************************************
 * end of fbe_parity_verify_state19()
 *****************************************/


/*!***************************************************************
 *  fbe_parity_verify_state20()
 ****************************************************************
 * @brief
 *  The verify has successfully completed.
 *  If recovery verify, transition to state21,
 *  else set status and transition to state25.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t
 *
 * @return
 *  This function always returns FBE_RAID_STATE_STATUS_EXECUTING
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_parity_verify_state20(fbe_raid_siots_t * siots_p)
{
    fbe_status_t fbe_status = FBE_STATUS_OK;
    fbe_raid_state_status_t status = FBE_RAID_STATE_STATUS_EXECUTING;
    fbe_block_count_t blocks_remaining = siots_p->xfer_count - siots_p->parity_count;

    if (siots_p->parity_count > siots_p->xfer_count)
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity: blocks remaining is unexpected 0x%llx 0x%llx 0x%llx\n", 
                                            (unsigned long long)blocks_remaining,
					    (unsigned long long)siots_p->xfer_count,
					    (unsigned long long)siots_p->parity_count);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    if (siots_p->algorithm == R5_RD_VR || siots_p->algorithm == R5_DRD_VR)
    {
        fbe_raid_siots_t * parent_siots_p = fbe_raid_siots_get_parent(siots_p); 

        /* On a read verify, we checksum the data.
         * If any uncorrectable errors are found in
         * the host data, then we fail the read.
         */
        //siots_p->xfer_count = fbe_raid_siots_get_parent(siots_p)->xfer_count;
        //siots_p->start_lba = fbe_raid_siots_get_parent(siots_p)->start_lba;

        /* Check xsums in an effort to look for invalid blocks.
         * Note that we use the parent siots to do this check, since the parent has the reference to the data.
         */
        fbe_status = fbe_raid_xor_check_siots_checksum(parent_siots_p); 
        if (fbe_status != FBE_STATUS_OK)
        {
            fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "xor check status unexpected 0x%x\n", fbe_status);
            return FBE_RAID_STATE_STATUS_EXECUTING;
        }

        fbe_raid_siots_transition(siots_p, fbe_parity_verify_state21);
    }
    else
    {
        /* Non read-recovery, just continue.
         */
        fbe_raid_siots_transition(siots_p, fbe_parity_verify_state25);
        if (siots_p->error != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID)
        {
            fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS, 
                                                "parity: siots error unexpected 0x%x", siots_p->error);
            return FBE_RAID_STATE_STATUS_EXECUTING;
        }
        fbe_raid_siots_set_block_status(siots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);
        fbe_raid_siots_set_block_qualifier(siots_p, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);
    }

    return status;
}
/*****************************************
 * fbe_parity_verify_state20()
 *****************************************/

/*!***************************************************************
 * fbe_parity_verify_state21()
 ****************************************************************
 * @brief
 *   Process the read recovery checksum.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t
 *
 * @return
 *   This function always returns FBE_RAID_STATE_STATUS_EXECUTING.
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_parity_verify_state21(fbe_raid_siots_t * siots_p)
{
    fbe_xor_status_t xor_status = FBE_XOR_STATUS_INVALID;
    fbe_raid_siots_t *parent_siots_p = fbe_raid_siots_get_parent(siots_p); 

    fbe_raid_siots_get_xor_command_status(parent_siots_p, &xor_status);

    /* Transition to next state regardless.
     */
    fbe_raid_siots_transition(siots_p, fbe_parity_verify_state25);

    /* Set status based on xor return value.
     */
    if (xor_status == FBE_XOR_STATUS_NO_ERROR)
    {
        /* In the backfill case,
         * the cache data is good.
         */
        fbe_raid_siots_set_block_status(siots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);
        fbe_raid_siots_set_block_qualifier(siots_p, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);
    }
    else if (xor_status == FBE_XOR_STATUS_CHECKSUM_ERROR)
    {
        /* Return error status to host.
         */
        fbe_raid_siots_set_block_status(siots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR);
        fbe_raid_siots_set_block_qualifier(siots_p, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_DATA_LOST);
    }
    else
    {
        /* Invalid status.
         */
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity: unexpected xor status: 0x%x\n", 
                                            xor_status);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }
    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/*******************************
 * end of fbe_parity_verify_state21()
 *******************************/

/*!**************************************************************
 * fbe_parity_verify_state22()
 ****************************************************************
 * @brief
 *  Process a media error on a write/verify request.
 *  
 * @param siots_p
 * 
 * @return fbe_raid_state_status_t - Always FBE_RAID_STATE_STATUS_EXECUTING.
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_parity_verify_state22(fbe_raid_siots_t * siots_p)
{
    fbe_status_t status;
    fbe_block_count_t min_blocks = siots_p->parity_count;
    fbe_u16_t errored_bitmask = 0;
    fbe_raid_fruts_t *read_fruts_ptr = NULL;

    /* The writes actually stay on the read chain.
     */
    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_ptr);

    /* Transition to next state regardless.
     */
    fbe_raid_siots_transition(siots_p, fbe_parity_verify_state25);

    /* Determine how much we transferred.
     */
    status = fbe_raid_fruts_get_min_blocks_transferred(read_fruts_ptr, 
                                                       read_fruts_ptr->lba, &min_blocks, &errored_bitmask);

    if (status != FBE_STATUS_OK)
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity: get min blocks transferred unexpected status %d\n", status);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* We need to set media_error lba. 
     * Note that if we block or strip mined, the iots->blocks_transferred 
     * would have been incremented as part of the strip mine. 
     * Thus we only need to worry about setting media error lba to indicate the number of 
     * blocks successfully transferred in this block/strip pass. 
     */
    fbe_raid_siots_set_media_error_lba(siots_p, siots_p->start_lba + min_blocks); 
    fbe_raid_siots_set_block_status(siots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR);
    fbe_raid_siots_set_block_qualifier(siots_p, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_DATA_LOST);

    fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_DEBUG_HIGH,
                                "parity:wr media err lba:0x%llX blks:0x%llx m_lba: 0x%llx err_bitmask:0x%x\n",
                                (unsigned long long)siots_p->start_lba,
			        (unsigned long long)siots_p->xfer_count,
                                (unsigned long long)siots_p->media_error_lba,
			        errored_bitmask);
    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/**************************************
 * end fbe_parity_verify_state22()
 **************************************/
/*!***************************************************************
 *  fbe_parity_verify_state25()
 ****************************************************************
 * @brief
 *  The verify has successfully completed.  Return status.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t
 *
 * @return
 *  This function always returns FBE_RAID_STATE_STATUS_DONE.
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_parity_verify_state25(fbe_raid_siots_t * siots_p)
{
    fbe_u16_t write_bitmap = 0;
    fbe_payload_block_operation_opcode_t opcode;


    

    /* Set status to indicate to the parent that we wrote out corrections.
     */
    if ((siots_p->error == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS) &&
        fbe_raid_siots_is_flag_set(siots_p, FBE_RAID_SIOTS_FLAG_WROTE_CORRECTIONS))
    {
        fbe_raid_siots_set_block_qualifier(siots_p, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_COMPLETE_WITH_REMAP);
    }

    /* Report errors.
     */
    fbe_parity_report_errors(siots_p, FBE_TRUE);


    fbe_raid_siots_get_opcode(siots_p, &opcode);
    write_bitmap = fbe_parity_verify_get_write_bitmap(siots_p);
    if (write_bitmap && 
        (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ) &&
        fbe_parity_only_invalidated_in_error_regions(siots_p))
    {
        /* Avoid doing error verify if we only detected known invalidated sectors */
        fbe_raid_iots_t *iots_p = fbe_raid_siots_get_iots(siots_p);
        fbe_raid_iots_clear_flag(iots_p, FBE_RAID_IOTS_FLAG_REMAP_NEEDED);
    }


    fbe_raid_siots_transition_finished(siots_p);
    /* After reporting errors, just send Usurper FBE_BLOCK_TRANSPORT_CONTROL_CODE_UPDATE_LOGICAL_ERROR_STATS
     * to PDO for single or multibit CRC errors found 
     */
       if(fbe_raid_siots_pdo_usurper_needed(siots_p))
       {
           fbe_status_t send_crc_status = fbe_raid_siots_send_crc_usurper(siots_p);
           /*We are waiting only when usurper has been sent, else executing should be returned*/
           if (send_crc_status == FBE_STATUS_OK)
           {
               return FBE_RAID_STATE_STATUS_WAITING;
           }
       }
    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/*****************************************
 * end of fbe_parity_verify_state25()
 *****************************************/
/*****************************************
 * End fbe_parity_verify.c
 *****************************************/
