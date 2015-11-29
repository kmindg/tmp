/***************************************************************************
 * Copyright (C) EMC Corporation 1989-2007
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * fbe_parity_read.c
 ***************************************************************************
 *
 * @brief
 *   This file contains the state functions of the RAID 5 read algorithm.
 *   This algorithm should be initially selected only when the read request does lie
 *   entirely within the rebuild portion of the R5 unit; though the algorithm will drop
 *   into degraded read, which may discover that, after waiting for resources (locks),
 *   the unit is not as clean as required, e.g. they've become degraded while waiting.
 *      
 *
 * @notes
 *  At initial entry, we assume that the fbe_raid_siots_t has
 *      the following fields initialized:
 *              start_lba       - logical address of first block of new data
 *              xfer_count      - total blocks of new data
 *              data_disks      - total data disks affected 
 *              start_pos       - relative start disk position  
 *              parity_pos      - position occupied by parity
 *              parity_start    - first block of affected parity
 *              parity_count    - total blocks of affected parity
 *              
 *      If a transfer of data from the host is required:
 *              current_sg_position     - copy of sg_desc (host_req_id)
 *              current_sg_position.offset      - updated with offset for this req
 *
 * @author
 *  09/22/99 Rob Foley Created.
 *
 ***************************************************************************/

/*************************
 ** INCLUDE FILES
 *************************/
#include "fbe_raid_library_proto.h"
#include "fbe_parity_io_private.h"

/****************************************************************
 * fbe_parity_read_state0()
 ****************************************************************
 * @brief
 *  This function allocates [the maximum] number of FRU
 *  tracking structures we will need for this sub-request.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t 
 *
 * @return
 *      This function returns FBE_RAID_STATE_STATUS_WAITING if we were unable
 *      to allocate the required number of structures immediately;
 *      otherwise, FBE_RAID_STATE_STATUS_EXECUTING is returned.
 *
 * @author
 *  6/24/99 - Created. JJ
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_parity_read_state0(fbe_raid_siots_t * siots_p)
{
    fbe_status_t            status;
    fbe_u32_t               num_pages = 0;
    /* Add one for terminator.
     */
    fbe_raid_fru_info_t read_info[FBE_RAID_MAX_DISK_ARRAY_WIDTH+1];

    /* Validate the algorithm.
     */
    if (siots_p->algorithm != R5_RD)
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity: %s read algorithm: %d unexpected, siots_p = 0x%p\n",
                                            __FUNCTION__, siots_p->algorithm, siots_p);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* Validate the siots has bene properly populated
     */
    if (fbe_parity_generate_validate_siots(siots_p) == FBE_FALSE)
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity: %s: validate failed, siots_p = 0x%p\n",
                                            __FUNCTION__, siots_p);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

   /* Calculate and allocate all the memory needed for this siots.
    */
    status = fbe_parity_read_calculate_memory_size(siots_p, &read_info[0], &num_pages);
    if (RAID_COND(status != FBE_STATUS_OK))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity: %s failed to calculate memory size, "
                                            "siots_p = 0x%p, status = 0x%x\n",
                                            __FUNCTION__,
                                            siots_p,
                                            status);
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
    fbe_raid_siots_transition(siots_p, fbe_parity_read_state1);
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
        status = fbe_parity_read_setup_resources(siots_p, &read_info[0]);
        if (RAID_COND(status != FBE_STATUS_OK))
        {
            fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity: read failed to setup resources. status: 0x%x\n",
                                            status);
            return FBE_RAID_STATE_STATUS_EXECUTING;
        }

        /* To prevent confusion (i.e. we never went to `deferred allocation complete'
         * state) overwrite the current state.
         */
        fbe_raid_common_set_state(&siots_p->common, (fbe_raid_common_state_t)fbe_parity_read_state3);
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
                                            "parity: read memory request failed with status: 0x%x\n",
                                            status);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/* end of fbe_parity_read_state0() */

/****************************************************************
 * fbe_parity_read_state1()
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

fbe_raid_state_status_t fbe_parity_read_state1(fbe_raid_siots_t * siots_p)
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
                             "parity: read memory allocation error common flags: 0x%x \n",
                             fbe_raid_common_get_flags(&siots_p->common));
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_memory_allocation_error);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* Then calculate how many buffers are needed.
     */
    status = fbe_parity_read_calculate_memory_size(siots_p, &read_info[0], &num_pages);
    if (RAID_COND(status != FBE_STATUS_OK))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity: %s failed to calculate memory size, "
                                            "siots_p = 0x%p, status = 0x%x\n",
                                            __FUNCTION__,
                                            siots_p,
                                            status);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* Allocation completed. Plant the resources
     */
    status = fbe_parity_read_setup_resources(siots_p, &read_info[0]);
    if (RAID_COND(status != FBE_STATUS_OK))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity: read failed to setup resources. status: 0x%x\n",
                                            status);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* Transition to next state to issue request
     */
    fbe_raid_siots_transition(siots_p, fbe_parity_read_state3);

    /* Return the state status
     */
    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/*******************************
 * end fbe_parity_read_state1()
 *******************************/

/****************************************************************
 * fbe_parity_read_state3()
 ****************************************************************
 * @brief
 *  This function initializes the S/G lists with the
 *  buffer addresses, releasing any buffers that remain
 *  unused.  Then it sends read requests to disks.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t
 *
 * @return
 *
 * @author
 *  7/19/99 - Created. JJ
 *  9/9/99 - Rewrote for performance.  Rob Foley
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_parity_read_state3(fbe_raid_siots_t * siots_p)
{
    fbe_bool_t b_result = FBE_TRUE;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_raid_state_status_t raid_status;
    fbe_bool_t b_generate_next_siots = FBE_TRUE;
    fbe_raid_iots_t *iots_p = fbe_raid_siots_get_iots(siots_p);

    if (fbe_raid_siots_is_aborted(siots_p))
    {
        /* The iots has been aborted,
         * treat it as a hard error,
         * and bail out now.
         */
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_aborted);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* If we are quiescing, then this function will mark the siots as quiesced and
     * we will wait until we are woken up.
     */
    if (fbe_raid_siots_mark_quiesced(siots_p))
    {
        return FBE_RAID_STATE_STATUS_WAITING;
    }
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

    /* Since we may have waited before getting here, check if we are degraded now.
     * If we went degraded transition to state11 to do a degraded read.
     */
    if (fbe_parity_siots_is_degraded(siots_p) && 
        fbe_parity_degraded_read_required(siots_p))
    {
        /* Set wait count to zero, since when we get back from
         * degraded read, we might do a host transfer and 
         * wait_count is assumed to be zero.
         */
        siots_p->wait_count = 0;

        /* R5 and R3 transition to degraded read.
         */
        fbe_raid_siots_transition(siots_p, fbe_parity_read_state11);
        
        raid_status = FBE_RAID_STATE_STATUS_EXECUTING;
    }
    else
    {    
        /* Issue reads to the disks.
         */
        fbe_raid_fruts_t *read_fruts_ptr = NULL;
        fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_ptr);
        siots_p->wait_count = siots_p->data_disks;
        fbe_raid_siots_transition(siots_p, fbe_parity_read_state4);

        if (fbe_queue_length(&siots_p->read_fruts_head) != siots_p->wait_count)
        {
            /* Clear the generating flag so we do not wait for this to finish.
             */
            if (b_generate_next_siots) { fbe_raid_iots_cancel_generate_next_siots(iots_p); }
            fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "queue length %d not equal to wait count %lld\n",
                                            fbe_queue_length(&siots_p->read_fruts_head), siots_p->wait_count);
            return FBE_RAID_STATE_STATUS_EXECUTING;
        }
        fbe_raid_siots_check_traffic_trace(siots_p, FBE_TRUE);
        b_result = fbe_raid_fruts_send_chain(&siots_p->read_fruts_head, fbe_raid_fruts_evaluate);
        if (RAID_FRUTS_COND(b_result != FBE_TRUE))
        {
            /* We hit an issue while starting chain of fruts. We can not go to completion
             * process at this point as we are not sure how many fruts has been sent
             * successfully.
             */
            fbe_raid_siots_set_unexpected_error(siots_p, 
                                                FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "parity: %s failed to start read fruts chain for "
                                                "siots = 0x%p, b_result  = %d\n",
                                                __FUNCTION__,
                                                siots_p,
                                                b_result );
            
        }
        raid_status = FBE_RAID_STATE_STATUS_WAITING;
    }
    if (b_generate_next_siots)
    {
        fbe_raid_iots_generate_next_siots_immediate(iots_p);
    }
    return raid_status;
}
/* end of fbe_parity_read_state3() */
/****************************************************************
 * fbe_parity_read_state4()
 ****************************************************************
 * @brief
 *  Reads have completed, check status.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t 
 *
 * @return
 *
 * @author
 *  7/19/99 - Created. JJ
 *  9/9/99 - Rewrote for performance.      Rob Foley
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_parity_read_state4(fbe_raid_siots_t * siots_p)
{
    fbe_raid_fru_eboard_t eboard;
    fbe_raid_fru_error_status_t status;
    fbe_raid_fruts_t *read_fruts_ptr = NULL;

    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_ptr);

    fbe_raid_fru_eboard_init(&eboard);

    /* Get totals for errored or dead drives.
     */
    status = fbe_parity_get_fruts_error(read_fruts_ptr, &eboard);

    if (RAID_COND(fbe_raid_fruts_get_chain_bitmap(read_fruts_ptr, NULL) == 0))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity read: zero siots in the chain\n");
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    if (status == FBE_RAID_FRU_ERROR_STATUS_SUCCESS)
    {
        /* All reads are successful.
         */
        fbe_raid_siots_transition(siots_p, fbe_parity_read_state5);
    }
    else if ((status == FBE_RAID_FRU_ERROR_STATUS_SHUTDOWN) ||
             (status == FBE_RAID_FRU_ERROR_STATUS_DEAD) ||
             (status == FBE_RAID_FRU_ERROR_STATUS_WAITING) ||
             (status == FBE_RAID_FRU_ERROR_STATUS_ABORTED))
    {
        /* We check for aborted directly in siots since we are at the read phase and 
         * regardless of the operation we should abort the request. 
         */
        fbe_raid_state_status_t state_status;
        state_status = fbe_parity_handle_fruts_error(siots_p, read_fruts_ptr, &eboard, status);
        return state_status;
    }
    else if ((status == FBE_RAID_FRU_ERROR_STATUS_ERROR) || (eboard.hard_media_err_count > 0))
    {
        fbe_u32_t parity_drives = fbe_raid_siots_parity_num_positions(siots_p);

        /* Parity units can take up to parity_drives worth of failures and still continue
         * degraded. 
         */
        if ( eboard.dead_err_count <= parity_drives &&
             eboard.dead_err_count > 0 )
        {
            fbe_u32_t first_dead_pos = fbe_raid_siots_dead_pos_get(siots_p, FBE_RAID_FIRST_DEAD_POS);
            fbe_u32_t second_dead_pos = fbe_raid_siots_dead_pos_get(siots_p, FBE_RAID_SECOND_DEAD_POS);
            
            /* One read failed due to a dead drive that was already known to be dead.
             */
            fbe_raid_siots_transition(siots_p, fbe_parity_read_state11);

            /* We should have set up a dead pos.
             * The dead position can be any position. 
             */
            if RAID_COND(((first_dead_pos == FBE_RAID_INVALID_DEAD_POS) &&
                         (second_dead_pos == FBE_RAID_INVALID_DEAD_POS)))
            {
                 fbe_raid_siots_set_unexpected_error(siots_p, 
                                                     FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                     "parity: no position is dead. first postion = 0x%x, second position %x\n",
                                                     first_dead_pos,
                                                     second_dead_pos);
                 return FBE_RAID_STATE_STATUS_EXECUTING;
            }
        }
        else if (eboard.hard_media_err_count > 0)
        {
            /* Reads failed due to hard errors,
             * exit to verify.
             */
            fbe_raid_siots_transition(siots_p, fbe_parity_read_state12);
        }
        else if (eboard.drop_err_count > 0)
        {
            /* At least one read failed due to a DROPPED.
             */
            fbe_raid_siots_transition(siots_p, fbe_raid_siots_dropped_error);
        }
        else
        {
            /* This status is not expected.
             */
            fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "parity: %s fru error status unknown 0x%x\n", 
                                                __FUNCTION__, status);
            return FBE_RAID_STATE_STATUS_EXECUTING;
        }
    }
    else if (status == FBE_RAID_FRU_ERROR_STATUS_RETRY)
    {
        /* Reads failed due to retryable errors, transition to recovery verify to use
         * redundancy to recover. 
         */
        fbe_raid_siots_transition(siots_p, fbe_parity_read_state12);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }
    else
    {
        /* This status was not expected.  Instead of panicking, just
         * return the error. 
         */
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity: %s status unknown 0x%x\n", 
                                            __FUNCTION__, status);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }
    return FBE_RAID_STATE_STATUS_EXECUTING;

}
/* end of fbe_parity_read_state4() */

/****************************************************************
 * fbe_parity_read_state5()
 ****************************************************************
 * @brief
 *  All reads are successful, check the checksums.
 *
 * @param siots_p - current I/O.
 *
 * @return
 *  This function returns FBE_RAID_STATE_STATUS_EXECUTING.
 * 
 * @author
 *  3/10/00 - Created. MPG
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_parity_read_state5(fbe_raid_siots_t * siots_p)
{
    fbe_status_t status;
    fbe_xor_status_t xor_status = FBE_XOR_STATUS_INVALID;

    if (!fbe_raid_siots_should_check_checksums(siots_p))
    {
        /* In this case we only need to check the lba stamp.
         */
        status = fbe_raid_xor_check_lba_stamp(siots_p);
    
        if (status != FBE_STATUS_OK)
        {
            fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "xor check status unexpected 0x%x\n", status);
            return FBE_RAID_STATE_STATUS_EXECUTING;
        }
        fbe_raid_siots_get_xor_command_status(siots_p, &xor_status);
        if (xor_status == FBE_XOR_STATUS_NO_ERROR)
        {
            fbe_raid_siots_transition(siots_p, fbe_parity_read_state20);
            return FBE_RAID_STATE_STATUS_EXECUTING;
        }
        /* In error case we fall through to perform full check-checksum with eboard.
         */
    }
    
    /* Check xsums/lba stamps. routine will determine if both need
     * checked or if only lba stamps need checked.
     */
    status = fbe_raid_xor_read_check_checksum(siots_p, FBE_FALSE, FBE_TRUE);
    if (status != FBE_STATUS_OK)
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "xor check status unexpected 0x%x\n", status);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    fbe_raid_siots_get_xor_command_status(siots_p, &xor_status);

    if ( xor_status == FBE_XOR_STATUS_NO_ERROR && !fbe_raid_siots_is_retry(siots_p) )
    {
        /* No checksum error, and we are done.
         */
        fbe_raid_siots_transition(siots_p, fbe_parity_read_state20);
    }
    else
    {
        /* Error or not done or retry in progress.
         * When retry is in progress we go down the
         * error path to log status of retry.
         */
        fbe_raid_siots_transition(siots_p, fbe_parity_read_state6);
    }
    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/* end of fbe_parity_read_state5() */
/****************************************************************
 * fbe_parity_read_state6()
 ****************************************************************
 * @brief
 *  A crc error has occurred.
 *  Setup for recovery verify.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t 
 *
 * @return
 *  This function returns FBE_RAID_STATE_STATUS_EXECUTING.
 * 
 * @author
 *  7/22/99 - Created. JJ
 *  9/9/99 - Rewrote for performance.      Rob Foley
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_parity_read_state6(fbe_raid_siots_t * siots_p)
{
    fbe_raid_state_status_t status = FBE_RAID_STATE_STATUS_EXECUTING;
    fbe_raid_fruts_t *read_fruts_p = NULL;
    fbe_xor_status_t xor_status = FBE_XOR_STATUS_INVALID;

    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_p);
    fbe_raid_siots_get_xor_command_status(siots_p, &xor_status);

    /* If we are degraded and we either received a checksum error or
     * previously received a checksum error, then use rg_handle_csum_error().
     */
    if (fbe_parity_siots_is_degraded(siots_p))
    {
        /* We may have gone degraded before arriving here.
         * If we received a checksum error and if we are degraded now, then transition to 
         * state11 to do a degraded read.  We check this first since we would not want 
         * to do a retry if we are degraded, as a retry might try reading from 
         * the dead drive. 
         */
        if ((xor_status != FBE_XOR_STATUS_NO_ERROR) &&
            fbe_parity_degraded_read_required(siots_p))
        {
            /* Set wait count to zero, since when we get back from
             * degraded read, we might do a host transfer and 
             * wait_count is assumed to be zero.
             */
            siots_p->wait_count = 0;
            /* Just in case we were in the middle of a retry, clear the retry flag.
             */
            fbe_raid_common_clear_flag(&siots_p->common, FBE_RAID_COMMON_FLAG_REQ_RETRIED);
            /* Transition to degraded read.
             */
            fbe_raid_siots_transition(siots_p, fbe_parity_read_state11);

            status = FBE_RAID_STATE_STATUS_EXECUTING;
        }
        else if ( (xor_status != FBE_XOR_STATUS_NO_ERROR || fbe_raid_siots_is_retry( siots_p )) &&
                  !fbe_raid_csum_handle_csum_error( siots_p, read_fruts_p, 
                                                    FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                                    fbe_parity_read_state4 ))
        {
            /* We needed to retry the fruts and the above function
             * did so. We are waiting for the retries to finish.
             * Transition to state 4 to evaluate retries.
             */
            return FBE_RAID_STATE_STATUS_WAITING;
        }
    }

    /* Make note of any retried crc errors we came across.
     */
    //fbe_raid_report_retried_crc_errs( siots_p );
    
    if (fbe_raid_is_option_flag_set(fbe_raid_siots_get_raid_geometry(siots_p),
                                    FBE_RAID_OPTION_FLAG_FORCE_RVR_READ))
    {
        /* This mode forces us to always do a recovery verify read
         * even though one might not be needed.
         */
        xor_status = FBE_XOR_STATUS_CHECKSUM_ERROR;
    }
    
    if (xor_status == FBE_XOR_STATUS_NO_ERROR)
    {
        /* No checksum error,
         * continue read.
         */
        fbe_raid_siots_transition(siots_p, fbe_parity_read_state20);
    }
    else if (fbe_raid_siots_is_aborted(siots_p))
    {
        /* An aborted SIOTS is treated like a hard error.
         * Bail out now.
         */
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_aborted);
    }
    else if (xor_status & FBE_XOR_STATUS_CHECKSUM_ERROR)
    {
        /* Checksum error is detected in the read data,
         * go down to verify and recovery path.
         */
        fbe_u16_t err_mask;
        fbe_raid_siots_get_degraded_bitmask(siots_p, &err_mask);
        /* Save these checksum errors in our vrts.
         * If we are degraded then we will use these bits
         * to report status.
         */
        if ( fbe_raid_siots_record_errors(siots_p,
                              fbe_raid_siots_get_width(siots_p),
                              (fbe_xor_error_t *) & siots_p->misc_ts.cxts.eboard,
                              err_mask,    /* Bits to record for. */
                              FBE_TRUE    /* Allow correctable errors.  */,
                              FBE_FALSE    /* Just record errors, don't validate. */ )
             != FBE_RAID_STATE_STATUS_DONE )
        {
            /* Retries were performed in rg_record errors.
             */
            return FBE_RAID_STATE_STATUS_WAITING;
        }
        fbe_raid_siots_transition(siots_p, fbe_parity_read_state12);
    }
    else
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "unexpected xor status %d", xor_status);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }
    return status;
}
/* end of fbe_parity_read_state6() */
/****************************************************************
 * fbe_parity_read_state11()
 ****************************************************************
 * @brief
 *  A single read failed, start a degraded read.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t
 *
 * @return
 *
 * @author
 *  7/22/99 - Created. JJ
 *  11/1/99 - Set up for degraded read.
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_parity_read_state11(fbe_raid_siots_t * siots_p)
{
    fbe_status_t fbe_status = FBE_STATUS_OK;
    if (fbe_raid_geometry_is_raid6(fbe_raid_siots_get_raid_geometry(siots_p)))
    {
        /* For Raid 6 we change to recovery verify.  This is necessary since 
         * only one level of nesting is allowed and degraded read transitions
         * to degraded verify on dead/media errors.
         * Verify will handle the degraded case and return the reconstructed
         * data to the read state machine.
         */
        fbe_raid_siots_transition(siots_p, fbe_parity_read_state12);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }
    else
    {
        /* When we get back from the degraded read,
         * continue with read state machines.
         */
        fbe_raid_siots_transition(siots_p, fbe_parity_read_state15);

        /* Set up for degraded read.
         */
        fbe_status = fbe_raid_siots_start_nested_siots(siots_p, fbe_parity_degraded_read_transition);
        if (RAID_GENERATE_COND(fbe_status != FBE_STATUS_OK))
        {
            fbe_raid_siots_set_unexpected_error(siots_p, 
                                                FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "parity: failed to start nested siots  0x%p\n",
                                                siots_p);
            return (FBE_RAID_STATE_STATUS_EXECUTING); 
        }

        return FBE_RAID_STATE_STATUS_WAITING;
    }
}
/* end of fbe_parity_read_state11() */

/****************************************************************
 * fbe_parity_read_state12()
 ****************************************************************
 * @brief
 *  Start recovery verify.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t
 *
 * @return
 *
 * @author
 *      7/22/99 - Created. JJ
 *     06/28/00 - Added degraded handling. Rob Foley
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_parity_read_state12(fbe_raid_siots_t * siots_p)
{
    fbe_status_t fbe_status = FBE_STATUS_OK;
    fbe_raid_state_status_t status = FBE_RAID_STATE_STATUS_INVALID;

    fbe_packet_priority_t priority; 
    fbe_raid_siots_get_priority(siots_p, &priority);

    if (priority == FBE_PACKET_PRIORITY_OPTIONAL)
    {
        /* Just in case this read is optional, 
         * set the priority to normal to avoid
         * sending down an optional request.
         * Since we are doing error handling, this can no longer
         * be optional for entrance to degraded verify.
         */        
        fbe_raid_siots_set_priority(siots_p, FBE_PACKET_PRIORITY_NORMAL);
    }


    /* This chain of siots has dropped into single thread mode.
     * Wait until peer siots wakes us up.
     */
    status = fbe_raid_siots_wait_previous(siots_p);
    if (status == FBE_RAID_STATE_STATUS_EXECUTING)
    {
        /* When we get back from verify, continue with read state machine.
         */
        fbe_raid_siots_transition(siots_p, fbe_parity_read_state14);

        /* Transition to recovery verify to attempt to fix errors.
         */
        fbe_status = fbe_raid_siots_start_nested_siots(siots_p, fbe_parity_recovery_verify_state0);
        if (RAID_GENERATE_COND(fbe_status != FBE_STATUS_OK))
        {
            fbe_raid_siots_set_unexpected_error(siots_p, 
                                                FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "parity: failed to start nested siots  0x%p\n",
                                                siots_p);
            return FBE_RAID_STATE_STATUS_EXECUTING; 
        }

        /* We are waiting for recovery verify to finish (fbe_parity_read_state14).
         */
        status = FBE_RAID_STATE_STATUS_WAITING;
    }
    return status;
}
/* end of fbe_parity_read_state12() */

/*!***************************************************************
 * fbe_parity_read_state14()
 **************************************************************** 
 * @brief 
 *  Handle status from recovery verify.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t
 *
 * @return fbe_raid_state_status_t
 *
 * @author
 *  8/28/99 - Created. JJ
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_parity_read_state14(fbe_raid_siots_t * siots_p)
{
    fbe_raid_state_status_t status = FBE_RAID_STATE_STATUS_EXECUTING;

    if (!fbe_queue_is_empty(&siots_p->nsiots_q))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "unexpected nested siots found\n");
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* Return from verify and recovery.
     */
    if (fbe_raid_siots_is_aborted(siots_p))
    {
        /* The iots has been aborted,
         * treat it as a hard error,
         * and bail out now.
         */
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_aborted);
    }
    else if (siots_p->error == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS)
    {
        /* The data has been verified,
         * return to host.
         */
        fbe_raid_siots_transition(siots_p, fbe_parity_read_state20);
    }
    else if (siots_p->error == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_NOT_READY)
    {
        fbe_raid_fruts_t * read_fruts_p = NULL;
        fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_p);
        if (fbe_parity_read_fruts_chain_touches_dead_pos(siots_p, read_fruts_p))
        {
            /* We are degraded and read the degraded position,
             * transition to degraded read.
             */
            fbe_raid_siots_transition(siots_p, fbe_parity_read_state11);
        }
        /* We do not hit the dead drive and thus have no way to 
         * fix the error other than retrying.
         * Retry the error now and transition to state 4.
         */
        else if ( !fbe_raid_csum_handle_csum_error( siots_p, 
                                                    read_fruts_p, 
                                                    FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                                    fbe_parity_read_state4 ) )
        {
            /* We needed to retry the fruts and the above function
             * did so. We are waiting for the retries to finish.
             * Transition to state 4 to evaluate retries.
             */
            status = FBE_RAID_STATE_STATUS_WAITING;
        }
        else
        {
            /* Not able to retry since this is not a crc error.
             * Return error status to host.
             */
            fbe_raid_siots_transition(siots_p, fbe_raid_siots_media_error);
        }
    }
    else if (siots_p->error == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED)
    {
        /* Verify has found a shutdown unit.
         * invalid sectors.
         */
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_shutdown_error);
    }
    else if (siots_p->error == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR)
    {
        /* Verify has checksumed data and found
         * invalid sectors.
         */
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_media_error);
    }
    else
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                              "parity: %s status unknown 0x%x\n", 
                              __FUNCTION__, siots_p->error);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }
    return status;
}
/* end of fbe_parity_read_state14() */

/****************************************************************
 * fbe_parity_read_state15()
 ****************************************************************
 * @brief
 *  We returned from degraded read, check status.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t 
 *
 * @return
 *
 * @author
 *  11/2/99 - Created. JJ
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_parity_read_state15(fbe_raid_siots_t * siots_p)
{
    /* Return from degraded read.
     */
    if ((siots_p->error == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_REQUEST_ABORTED) ||
        fbe_raid_siots_is_aborted(siots_p))
    {
        /* Request is aborted, treat it like an error and bail now.
         */
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_aborted);
    }
    else if (siots_p->error == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS)
    {
        /* The data is good we are done.
         */
        fbe_raid_siots_transition(siots_p, fbe_parity_read_state20);
    }
    else if ( (siots_p->error == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED) &&
              (siots_p->qualifier == FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE) )
    {
        /* shutdown means too many drives are down to continue.
         */
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_shutdown_error);
    }
    else if ( (siots_p->error == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED) &&
              (siots_p->qualifier == FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_POSSIBLE) )
    {
        /* Retryable means we discovered that we are no longer degraded.
         * We need to free resources and start again. 
         */
        fbe_raid_siots_reuse(siots_p);
        fbe_raid_siots_transition(siots_p, fbe_parity_read_state0);
    }
    else if ( siots_p->error == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR )
    {
        /* For Raid 5 we change to recovery verify.  This is necessary since 
         * only one level of nesting is allowed and degraded read transitions
         * to degraded verify on dead/media errors.
         * Verify will handle the degraded case and return the reconstructed
         * data to the read state machine.
         */
        fbe_raid_siots_transition(siots_p, fbe_parity_read_state12);
    }
    else
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                              "parity: %s status unknown 0x%x\n", 
                              __FUNCTION__, siots_p->error);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }
    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/* end of fbe_parity_read_state15() */

/****************************************************************
 * fbe_parity_read_state20()
 ****************************************************************
 * @brief
 *  The reads has completed, return good status.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t
 *
 * @return
 *  This function always returns FBE_RAID_STATE_STATUS_DONE.
 *
 * @author
 *  7/21/99 - Created. JJ
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_parity_read_state20(fbe_raid_siots_t * siots_p)
{
    fbe_raid_siots_transition(siots_p, fbe_raid_siots_success);

    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/*******************************
 * end of fbe_parity_read_state20()
 *******************************/

/***************************************************
 * end fbe_parity_read.c
 ***************************************************/
