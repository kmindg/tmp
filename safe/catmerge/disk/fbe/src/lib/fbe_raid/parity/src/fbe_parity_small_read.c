/***************************************************************************
 * Copyright (C) EMC Corporation 1989-2007
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/
 
/***************************************************************************
 * fbe_parity_small_read.c
 ***************************************************************************
 *
 * @brief
 *   This file contains the state functions of the RAID 5 small read algorithm.
 *   This algorithm should be initially selected only when the read request
 *   does lie entirely within the rebuild portion of the R5 unit;
 *   though the algorithm will drop into degraded read,
 *   which may discover that, after waiting for resources (locks), 
 *   the unit is not as clean as required, e.g. they've become degraded 
 *   while waiting.
 *
 *   This state machine invokes the read state machine r5_rd_state?
 *   whenever any error is encountered.
 *   
 *
 * @notes
 *      At initial entry, we assume that the fbe_raid_siots_t has
 *      the following fields initialized:
 *              start_lba       - logical address of first block of new data
 *              xfer_count      - total blocks of new data
 *              data_disks      - tatal data disks affected 
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
 *  10/12/99 Rob Foley Created.
 *
 ***************************************************************************/

/*************************
 ** INCLUDE FILES
 *************************/
#include "fbe_raid_library_proto.h"
#include "fbe_raid_library.h"
#include "fbe_parity_io_private.h"


/*****************************************************
 * Static functions
 *****************************************************/

/****************************************************************
 * fbe_parity_small_read_state0()
 ****************************************************************
 * @brief
 *  This function allocates fruts.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t
 *
 * @return VALUE:
 *  FBE_RAID_STATE_STATUS_EXECUTING.
 *
 * @author
 *     08/09/05 - Created. Rob Foley
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_parity_small_read_state0(fbe_raid_siots_t * siots_p)
{    
    fbe_status_t            status;
    fbe_u32_t               num_pages;

    /* Validate the algorithm.
     */
    if (RAID_COND(siots_p->algorithm != R5_SM_RD))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity: algorithm 0x%x unexpected\n",
                                            siots_p->algorithm);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    if (RAID_COND(siots_p->data_disks != 1))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity: data disks %d unexpected\n",
                                            siots_p->data_disks);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }
    /* Calculate and allocate all the memory needed for this siots.
     */
    status = fbe_parity_small_read_calculate_memory_size(siots_p, &num_pages);

    if (RAID_COND(status != FBE_STATUS_OK))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity: %s failed to calculate memory size status: 0x%x\n", 
                                            __FUNCTION__, status);
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
    fbe_raid_siots_transition(siots_p, fbe_parity_small_read_state1);
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
        status = fbe_parity_small_read_setup_resources(siots_p);
        if (RAID_COND(status != FBE_STATUS_OK))
        {
            fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity: sm read failed to setup resources. status: 0x%x\n",
                                            status);
            return FBE_RAID_STATE_STATUS_EXECUTING;
        }

        /* To prevent confusion (i.e. we never went to `deferred allocation complete'
         * state) overwrite the current state.
         */
        fbe_raid_common_set_state(&siots_p->common, (fbe_raid_common_state_t)fbe_parity_small_read_state2);
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
                                            "parity: sm read memory request failed with status: 0x%x\n",
                                            status);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/*****************************************
 * end of fbe_parity_small_read_state0()
 *****************************************/

/****************************************************************
 * fbe_parity_small_read_state1()
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

fbe_raid_state_status_t fbe_parity_small_read_state1(fbe_raid_siots_t * siots_p)
{
    fbe_status_t            status;

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
                             "parity: sm read memory allocation error common flags: 0x%x \n",
                             fbe_raid_common_get_flags(&siots_p->common));
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_memory_allocation_error);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* Allocation completed. Plant the resources
     */
    status = fbe_parity_small_read_setup_resources(siots_p);
    if (RAID_COND(status != FBE_STATUS_OK))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity: sm read failed to setup resources. status: 0x%x\n",
                                            status);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* Transition to next state to issue request
     */
    fbe_raid_siots_transition(siots_p, fbe_parity_small_read_state2);

    /* Return the state status
     */
    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/***********************************
 * end fbe_parity_small_read_state1()
 **********************************/

/****************************************************************
 * fbe_parity_small_read_state2()
 ****************************************************************
 * @brief
 *  This function allocates buffers if needed.
 *  It then starts the fruts.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t
 *
 * @return FBE_RAID_STATE_STATUS_EXECUTING
 *
 * @author
 *  9/3/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_parity_small_read_state2(fbe_raid_siots_t * siots_p)
{
    fbe_status_t status;
    fbe_raid_state_status_t state_status = FBE_RAID_STATE_STATUS_INVALID;
    fbe_bool_t b_result = FBE_TRUE;
    fbe_bool_t b_generate_next_siots = FBE_TRUE;
    fbe_raid_iots_t *iots_p = fbe_raid_siots_get_iots(siots_p);
    fbe_raid_fruts_t *read_fruts_ptr = NULL;
    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_ptr);

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
     */
    if ( !fbe_raid_iots_is_flag_set(iots_p, FBE_RAID_IOTS_FLAG_NON_DEGRADED) &&
        fbe_parity_siots_is_degraded(siots_p) && 
        fbe_parity_degraded_read_required(siots_p))
    {
        /* We're degraded, so transition to degraded read.
         */
        fbe_raid_siots_transition(siots_p, fbe_parity_read_state11);
        state_status = FBE_RAID_STATE_STATUS_EXECUTING;

        /* Set wait count to zero, since when we get back from
         * degraded read, we might do a host transfer and 
         * wait_count is assumed to be zero.
         */
        siots_p->wait_count = 0;
    }
    else
    {
        /* Issue reads to the disks.
         */
        if (RAID_COND(siots_p->data_disks != 1))
        {
            /* Clear the generating flag so we do not wait for this to finish.
             */
            if (b_generate_next_siots) { fbe_raid_iots_cancel_generate_next_siots(iots_p); }
            fbe_raid_siots_set_unexpected_error(siots_p, 
                                                FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "parity: %s errored: data disks %d is unexpected, siots_p = 0x%p\n",
                                                __FUNCTION__,
                                                siots_p->data_disks,
                                                siots_p);
            return FBE_RAID_STATE_STATUS_EXECUTING;
        }
        fbe_raid_siots_check_traffic_trace(siots_p, FBE_TRUE);
        siots_p->wait_count = siots_p->data_disks;

        fbe_raid_siots_transition(siots_p, fbe_parity_small_read_state3);
        b_result = fbe_raid_fruts_send(read_fruts_ptr, fbe_raid_fruts_evaluate);
        if (RAID_FRUTS_COND(b_result != FBE_TRUE))
        {
            /* Clear the generating flag so we do not wait for this to finish.
             */
            if (b_generate_next_siots) { fbe_raid_iots_cancel_generate_next_siots(iots_p); }
            fbe_raid_siots_set_unexpected_error(siots_p, 
                                                FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS, 
                                                "parity: %s failed to start fruts 0x%p for siots 0x%p\n", 
                                                __FUNCTION__,
                                                read_fruts_ptr,
                                                siots_p);
            
            /* We might be here as we might have intentionally failed this 
             * condition while testing for error-path. In that case,
             * we will have to wait for completion of  fruts as it has
             * been already started.
             */
            return ((b_result) ? (FBE_RAID_STATE_STATUS_WAITING) : (FBE_RAID_STATE_STATUS_EXECUTING));
        }
        state_status = FBE_RAID_STATE_STATUS_WAITING;
    }
    if (b_generate_next_siots)
    {
        fbe_raid_iots_generate_next_siots_immediate(iots_p);
    }
    return state_status;
}
/*****************************************
 * end of fbe_parity_small_read_state2()
 *****************************************/

/****************************************************************
 * fbe_parity_small_read_state3()
 ****************************************************************
 * @brief
 *  Reads have completed, check status.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t 
 *
 * @return
 *  FBE_RAID_STATE_STATUS_EXECUTING
 *
 * @author
 *  9/9/99 - Rewrote for performance optimization. Rob Foley
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_parity_small_read_state3(fbe_raid_siots_t * siots_p)
{
    fbe_raid_state_status_t state_status = FBE_RAID_STATE_STATUS_EXECUTING;
    fbe_raid_fru_eboard_t eboard;
    fbe_raid_fru_error_status_t fru_status = FBE_RAID_FRU_ERROR_STATUS_SUCCESS;
    fbe_raid_fruts_t *read_fruts_ptr = NULL;

    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_ptr);

    fbe_raid_fru_eboard_init(&eboard);

    /* Get totals for errored or dead drives.
     */
    fru_status = fbe_parity_get_fruts_error(read_fruts_ptr, &eboard);

    if (RAID_COND(fbe_raid_fruts_get_chain_bitmap(read_fruts_ptr, NULL) == 0))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity small read: zero siots in the chain\n");
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    if (fru_status == FBE_RAID_FRU_ERROR_STATUS_SUCCESS)
    {
        fbe_status_t status;
        /* Stage 1: All reads are successful.
         *          Check checksums if necessary.
         */
        fbe_raid_siots_transition(siots_p, fbe_parity_small_read_state4);

        /* Check xsums/lba stamps. routine will
         * determine if both need checked or if only lba
         * stamps need checked.
         */
        status = fbe_raid_xor_read_check_checksum(siots_p, FBE_FALSE, FBE_TRUE);
        if (RAID_COND(status != FBE_STATUS_OK))
        {
            fbe_raid_siots_set_unexpected_error(siots_p,
                                                FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "parity: status 0x%x of check checksum routine is unexpected\n",
                                                status);
            state_status = FBE_RAID_STATE_STATUS_EXECUTING;
        }
        state_status = FBE_RAID_STATE_STATUS_EXECUTING;
    }
    else if (fru_status == FBE_RAID_FRU_ERROR_STATUS_WAITING)
    {
        state_status = FBE_RAID_STATE_STATUS_WAITING;
    }
    else
    {
        /* Transition to read state machine.
         */
        fbe_raid_siots_transition(siots_p, fbe_parity_read_state4);
        state_status = FBE_RAID_STATE_STATUS_EXECUTING;
    }

    return state_status;
}
/*****************************************
 * end of fbe_parity_small_read_state3()
 *****************************************/

/****************************************************************
 * fbe_parity_small_read_state4()
 ****************************************************************
 * @brief
 *  Checksums have completed, check status.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t
 *
 * @return EXECUTING.
 *
 * @author
 *  7/19/99 - Created. JJ
 *  9/9/99 - Rewrote for performance.      Rob Foley
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_parity_small_read_state4(fbe_raid_siots_t * siots_p)
{
    fbe_raid_state_status_t status = FBE_RAID_STATE_STATUS_EXECUTING;
    fbe_xor_status_t xor_status;
    fbe_raid_siots_get_xor_command_status(siots_p, &xor_status);

    if (fbe_raid_is_option_flag_set(fbe_raid_siots_get_raid_geometry(siots_p), 
                                    FBE_RAID_OPTION_FLAG_FORCE_RVR_READ))
    {
        /* This mode forces us to always do a recovery verify read
         * even though one might not be needed.
         */
        xor_status = FBE_XOR_STATUS_CHECKSUM_ERROR;
    }

    /* First check if we are aborted
     */
    if (fbe_raid_siots_is_aborted(siots_p))
    {
        /* An aborted SIOTS is treated like a hard error.
         * Bail out now.
         */
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_aborted);
        status = FBE_RAID_STATE_STATUS_EXECUTING;
    }
    /* Stage 2: Check status of xor operation.
     */
    else if (xor_status == FBE_XOR_STATUS_NO_ERROR)
    {
        /* No checksum error,
         * continue read.
         */
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_success);

        status = FBE_RAID_STATE_STATUS_EXECUTING;
    }
    else if (xor_status & FBE_XOR_STATUS_CHECKSUM_ERROR)
    {
        /* Checksum error is detected in the read data,
         * transition to normal read to handle error. Before we can go to normal
         * read we need to allocate a vrts, which will hold the vrts. 
         * Transition to state where we will init the vrts.
         */
        fbe_raid_siots_transition(siots_p, fbe_parity_read_state5);
        return status;
    }
    else
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                              "parity: %s xor status unknown 0x%x\n", 
                              __FUNCTION__, xor_status);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }
    return status;

}
/*****************************************
 * end of fbe_parity_small_read_state4()
 *****************************************/
/***********
 * fbe_parity_small_read.c
 ***********/
