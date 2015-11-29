/***************************************************************************
 * Copyright (C) EMC Corporation 2013
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_parity_rekey.c
 ***************************************************************************
 *
 * @brief
 *   This file contains the state functions of the parity rekey algorithm.
 *
 * @author
 *  10/15/2013 - Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 **  INCLUDE FILES
 *************************/
#include "fbe_raid_library_proto.h"
#include "fbe_parity_io_private.h"

static fbe_raid_state_status_t fbe_parity_rekey_state1(fbe_raid_siots_t * siots_p);
static fbe_raid_state_status_t fbe_parity_rekey_state2(fbe_raid_siots_t * siots_p);
static fbe_raid_state_status_t fbe_parity_rekey_state3(fbe_raid_siots_t * siots_p);
static fbe_raid_state_status_t fbe_parity_rekey_state4(fbe_raid_siots_t * siots_p);
static fbe_raid_state_status_t fbe_parity_rekey_state10(fbe_raid_siots_t * siots_p);
static fbe_raid_state_status_t fbe_parity_rekey_state11(fbe_raid_siots_t * siots_p);
static fbe_raid_state_status_t fbe_parity_rekey_state12(fbe_raid_siots_t * siots_p);
static fbe_raid_state_status_t fbe_parity_rekey_state20(fbe_raid_siots_t * siots_p);
static fbe_raid_state_status_t fbe_parity_rekey_handle_read_error(fbe_raid_siots_t * siots_p,
                                                                  fbe_raid_fru_eboard_t *eboard_p);

/*!***************************************************************
 * fbe_parity_rekey_state0()
 ****************************************************************
 * @brief
 *  This function allocates memory.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t.
 *
 * @return
 *  This function returns FBE_RAID_STATE_STATUS_WAITING if we were unable
 *  to allocate the required number of structures immediately.
 *  Otherwise FBE_RAID_STATE_STATUS_EXECUTING is returned.
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_parity_rekey_state0(fbe_raid_siots_t * siots_p)
{
    fbe_status_t            status;
    fbe_u32_t               num_pages = 0;
    /* Add one for terminator.
     */
    fbe_raid_fru_info_t     read_info[FBE_RAID_MAX_DISK_ARRAY_WIDTH+1];

    if (fbe_parity_rekey_validate(siots_p) == FBE_FALSE)
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity: rekey validate failed\n");
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* Calculate and allocate all the memory needed for this siots.
     */
    status = fbe_parity_rekey_calculate_memory_size(siots_p, &read_info[0], &num_pages);
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
    fbe_raid_siots_transition(siots_p, fbe_parity_rekey_state1);
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
        status = fbe_parity_rekey_setup_resources(siots_p, &read_info[0]);
        if (RAID_COND(status != FBE_STATUS_OK))
        {
            fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity: rekey failed to setup resources. status: 0x%x\n",
                                            status);
            return FBE_RAID_STATE_STATUS_EXECUTING;
        }

        /* To prevent confusion (i.e. we never went to `deferred allocation complete'
         * state) overwrite the current state.
         */
        fbe_raid_common_set_state(&siots_p->common, (fbe_raid_common_state_t)fbe_parity_rekey_state3);
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
                                            "parity: rekey memory request failed with status: 0x%x\n",
                                            status);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/**************************************
 * end fbe_parity_rekey_state0()
 **************************************/
/****************************************************************
 * fbe_parity_rekey_state1()
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
 *  11/5/2013 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_raid_state_status_t fbe_parity_rekey_state1(fbe_raid_siots_t * siots_p)
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
                             "parity: rekey memory allocation error common flags: 0x%x \n",
                             fbe_raid_common_get_flags(&siots_p->common));
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_memory_allocation_error);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* Then calculate how many buffers are needed.
     */
    status = fbe_parity_rekey_calculate_memory_size(siots_p, &read_info[0], &num_pages);
    if (status != FBE_STATUS_OK)
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "unexpected calculate memory size status 0x%x\n", status);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* Allocation completed. Plant the resources
     */
    status = fbe_parity_rekey_setup_resources(siots_p, &read_info[0]);
    if (RAID_COND(status != FBE_STATUS_OK))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity: rekey failed to setup resources. status: 0x%x\n",
                                            status);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* Transition to next state to issue request
     */
    fbe_raid_siots_transition(siots_p, fbe_parity_rekey_state3);

    /* Return the state status
     */
    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/********************************
 * end fbe_parity_rekey_state1()
 ********************************/
/*!***************************************************************
 *  fbe_parity_rekey_state3()
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
 *  10/15/2013 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_raid_state_status_t fbe_parity_rekey_state3(fbe_raid_siots_t * siots_p)
{
    fbe_raid_state_status_t return_status;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_bool_t b_generate_next_siots = FBE_FALSE;
    fbe_raid_iots_t *iots_p = fbe_raid_siots_get_iots(siots_p);
    fbe_raid_fruts_t *read_fruts_ptr = NULL;
    fbe_bool_t b_result;

    if (fbe_raid_siots_is_aborted(siots_p)) {
        /* End this siots as we were aborted.
         */
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_aborted);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }


    /* Determine if we need to generate another siots.
     * If this returns TRUE then we must always call fbe_raid_iots_generate_next_siots_immediate(). 
     * Typically this call is done after we have started the disk I/Os.
     */
    status = fbe_raid_siots_should_generate_next_siots(siots_p, &b_generate_next_siots);
    if (RAID_GENERATE_COND(status != FBE_STATUS_OK)) {
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS, 
                                            "parity: %s failed in next siots generation: siots_p = 0x%p, status = %dn",
                                            __FUNCTION__, siots_p, status);
        return(FBE_RAID_STATE_STATUS_EXECUTING);
    }

    /* Issue reads to the disks.
     */
    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_ptr);
    siots_p->wait_count = siots_p->data_disks;
    fbe_raid_siots_transition(siots_p, fbe_parity_rekey_state4);

    if (fbe_queue_length(&siots_p->read_fruts_head) != siots_p->wait_count) {
        /* Clear the generating flag so we do not wait for this to finish.
         */
        if (b_generate_next_siots) {
            fbe_raid_iots_cancel_generate_next_siots(iots_p);
        }
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "queue length %d not equal to wait count %lld\n",
                                            fbe_queue_length(&siots_p->read_fruts_head), siots_p->wait_count);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    fbe_raid_siots_check_traffic_trace(siots_p, FBE_TRUE);
    b_result = fbe_raid_fruts_send_chain(&siots_p->read_fruts_head, fbe_raid_fruts_evaluate);
    if (RAID_FRUTS_COND(b_result != FBE_TRUE)) {
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
    return_status = FBE_RAID_STATE_STATUS_WAITING;

    if (b_generate_next_siots) {
        fbe_raid_iots_generate_next_siots_immediate(iots_p);
    }
    return return_status;
}
/**************************************
 * end fbe_parity_rekey_state3
 **************************************/

/*!**************************************************************
 * fbe_parity_rekey_handle_read_error()
 ****************************************************************
 * @brief
 *  Determine what to do when we get a FBE_RAID_FRU_ERROR_STATUS_ERROR,
 *  on a read for rekey.
 *
 * @param siots_p - current I/O.
 * @param eboard_p - Eboard filled out from a read chain.  
 *
 * @return FBE_RAID_STATE_STATUS_DONE - Just continue with rekey.
 *         Otherwise - return this status from the state machine.
 *
 * @author
 *  10/16/2013 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_raid_state_status_t fbe_parity_rekey_handle_read_error(fbe_raid_siots_t * siots_p,
                                                                  fbe_raid_fru_eboard_t *eboard_p)
{
    fbe_raid_fruts_t *read_fruts_p = NULL;
    fbe_status_t check_degraded_status;

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
    else if (eboard_p->hard_media_err_count > 0)
    {
        /* Mark for rekey and let rekey fix the errors.
         */
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity: error count more than width 0x%x 0x%x", 
                                            eboard_p->hard_media_err_count, fbe_raid_siots_get_width(siots_p));
        return FBE_RAID_STATE_STATUS_EXECUTING;
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
 * end fbe_parity_rekey_handle_read_error()
 ******************************************/
/*!***************************************************************
 * fbe_parity_rekey_state4()
 ****************************************************************
 * @brief
 *  This function will evaluate the results from reads.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t
 *
 * @return fbe_raid_state_status_t
 *
 * @author
 *  10/16/2013 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_raid_state_status_t fbe_parity_rekey_state4(fbe_raid_siots_t * siots_p)
{
    fbe_raid_fru_eboard_t eboard;
    fbe_raid_fru_error_status_t status;
    fbe_raid_fruts_t *read_fruts_p = NULL;
    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_p);

    fbe_raid_siots_log_traffic(siots_p, "parity_rekey_state4");
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
        fbe_raid_siots_transition(siots_p, fbe_parity_rekey_state10);
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

         /* We might have gone degraded and have a retry.
          * In this case it is necessary to mark the degraded positions as nops since 
          * 1) we already marked the paged for these 
          * 2) the next time we run through this state we do not want to consider these failures 
          *    (and wait for continue) since we already got the continue. 
          */
        fbe_raid_fruts_set_degraded_nop(siots_p, read_fruts_p);

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
    else if (status == FBE_RAID_FRU_ERROR_STATUS_ERROR)
    {
        fbe_raid_state_status_t state_status;
        fbe_raid_fruts_set_degraded_nop(siots_p, read_fruts_p);

        state_status = fbe_parity_rekey_handle_read_error(siots_p, &eboard);
        
        if (state_status == FBE_RAID_STATE_STATUS_DONE)
        {
            /* Continue to check crc and write.
             */ 
            fbe_raid_siots_transition(siots_p, fbe_parity_rekey_state20);
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
/**************************************
 * end fbe_parity_rekey_state4()
 **************************************/

/****************************************************************
 * fbe_parity_rekey_state5()
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
 *  10/16/2013 - Created. Rob Foley
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_parity_rekey_state5(fbe_raid_siots_t * siots_p)
{
    fbe_status_t status;
    fbe_xor_status_t xor_status = FBE_XOR_STATUS_INVALID;
    
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
        /* No checksum error, continue with write.
         */
        fbe_raid_siots_transition(siots_p, fbe_parity_rekey_state10);
    }
    else
    {
        /* Lba stamp or checksum error.  Mark for rekey and let rekey clean this up.
         */

        fbe_raid_iots_t *iots_p = fbe_raid_siots_get_iots(siots_p);
        fbe_raid_iots_set_flag(iots_p, FBE_RAID_IOTS_FLAG_REMAP_NEEDED);
        fbe_raid_siots_transition(siots_p, fbe_parity_rekey_state20);
    }
    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/**************************************
 * end fbe_parity_rekey_state5()
 **************************************/

/*!***************************************************************
 *  fbe_parity_rekey_state10()
 ****************************************************************
 * @brief
 *  Call out to mark the paged with rekey pos degraded.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t
 *
 * @return
 *  FBE_RAID_STATE_STATUS_WAITING
 *
 * @author
 *  11/18/2013 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_raid_state_status_t fbe_parity_rekey_state10(fbe_raid_siots_t * siots_p)
{
    fbe_raid_iots_t *iots_p = fbe_raid_siots_get_iots(siots_p);
    fbe_raid_siots_transition(siots_p, fbe_parity_rekey_state11);
    
    fbe_raid_iots_set_flag(iots_p, FBE_RAID_IOTS_FLAG_REKEY_MARK_DEGRADED);
    fbe_raid_iots_callback(iots_p);
    return FBE_RAID_STATE_STATUS_WAITING;
}
/**************************************
 * end fbe_parity_rekey_state10
 **************************************/
/*!***************************************************************
 *  fbe_parity_rekey_state11()
 ****************************************************************
 * @brief
 *  This function will start the writes.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t
 *
 * @return
 *  FBE_RAID_STATE_STATUS_WAITING
 *
 * @author
 *  10/16/2013 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_raid_state_status_t fbe_parity_rekey_state11(fbe_raid_siots_t * siots_p)
{
    fbe_raid_state_status_t return_status;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_raid_fruts_t *read_fruts_p = NULL;
    fbe_bool_t b_result;
    fbe_raid_iots_t *iots_p = fbe_raid_siots_get_iots(siots_p);

    fbe_raid_iots_clear_flag(iots_p, FBE_RAID_IOTS_FLAG_REKEY_MARK_DEGRADED);

    if (fbe_raid_siots_is_aborted(siots_p)) {
        /* End this siots as we were aborted.
         */
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_aborted);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }
    /* The read chain is the only chain even for our writes.
     */
    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_p);

    /* Set all fruts to WRITE opcode, clear flags.
     */
    status = fbe_raid_fruts_for_each((1 << read_fruts_p->position),
                                     read_fruts_p,
                                     fbe_raid_fruts_set_opcode,
                                     (fbe_u64_t) FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,
                                     (fbe_u64_t) NULL);

    if (RAID_COND(status != FBE_STATUS_OK))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity: %s failed to set opcode.read_fruts_p = 0x%p, "
                                            "siots_p 0x%p, status = 0x%x\n",
                                            __FUNCTION__, read_fruts_p, siots_p, status);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    siots_p->wait_count = siots_p->data_disks;
    fbe_raid_siots_transition(siots_p, fbe_parity_rekey_state12);

    if (fbe_queue_length(&siots_p->read_fruts_head) != siots_p->wait_count) {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "queue length %d not equal to wait count %lld\n",
                                            fbe_queue_length(&siots_p->read_fruts_head), siots_p->wait_count);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    fbe_raid_siots_check_traffic_trace(siots_p, FBE_TRUE);
    b_result = fbe_raid_fruts_send_chain(&siots_p->read_fruts_head, fbe_raid_fruts_evaluate);
    if (RAID_FRUTS_COND(b_result != FBE_TRUE)) {
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
    return_status = FBE_RAID_STATE_STATUS_WAITING;
    return return_status;
}
/**************************************
 * end fbe_parity_rekey_state11
 **************************************/
/****************************************************************
 * fbe_parity_rekey_state12()
 ****************************************************************
 * @brief
 *  The writes completed, check the status of the writes.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t
 *
 * @return
 *  This function always returns FBE_RAID_STATE_STATUS_EXECUTING.
 *
 * @author
 *   10/16/2013 - Created. Rob Foley
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_parity_rekey_state12(fbe_raid_siots_t * siots_p)
{
    fbe_raid_fru_eboard_t eboard;
    fbe_raid_fru_error_status_t status;
    fbe_raid_fruts_t *write_fruts_p = NULL;

    /* The read chain is the only chain even for our writes.
     */
    fbe_raid_siots_get_read_fruts(siots_p, &write_fruts_p);
    fbe_raid_fru_eboard_init(&eboard);

    /* Get totals for errored or dead drives.
     */
    status = fbe_parity_get_fruts_error(write_fruts_p, &eboard);

    if (status == FBE_RAID_FRU_ERROR_STATUS_SUCCESS)
    {
        /* Write successful, send good status.
         * First need to release the write_log slot.
         */
        fbe_raid_siots_transition(siots_p, fbe_parity_rekey_state20);
    }
    else if ((status == FBE_RAID_FRU_ERROR_STATUS_SHUTDOWN) ||
             (status == FBE_RAID_FRU_ERROR_STATUS_DEAD) ||
             (status == FBE_RAID_FRU_ERROR_STATUS_WAITING))
    {
        fbe_raid_state_status_t state_status;
        state_status = fbe_parity_handle_fruts_error(siots_p, write_fruts_p, &eboard, status);
        return state_status;
    }
    else if (status == FBE_RAID_FRU_ERROR_STATUS_RETRY)
    {
        fbe_status_t retry_status;

         /* We might have gone degraded and have a retry.
          * In this case it is necessary to mark the degraded positions as nops since 
          * 1) we already marked the paged for these 
          * 2) the next time we run through this state we do not want to consider these failures 
          *    (and wait for continue) since we already got the continue. 
          */
        fbe_raid_fruts_set_degraded_nop(siots_p, write_fruts_p);

        /* Send out retries.  We will stop retrying if we get aborted or 
         * our timeout expires or if an edge goes away. 
         */
        retry_status = fbe_raid_siots_retry_fruts_chain(siots_p, &eboard, write_fruts_p);
        if (retry_status != FBE_STATUS_OK)
        {
            fbe_raid_siots_set_unexpected_error(siots_p,
                                                FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "parity: retry %d errors (bitmap 0x%x) status: %d unexpected\n", 
                                                eboard.retry_err_count, eboard.retry_err_bitmap, retry_status);
            fbe_raid_fru_eboard_display(&eboard, siots_p, FBE_TRACE_LEVEL_ERROR);
            return FBE_RAID_STATE_STATUS_EXECUTING; 
        }

        return FBE_RAID_STATE_STATUS_WAITING;
    } 
    else if (status == FBE_RAID_FRU_ERROR_STATUS_WAITING)
    {
        /* One drive is dead, we need to mark this request
         * as waiting for a shutdown or continue.
         */
        return FBE_RAID_STATE_STATUS_WAITING;
    }
    else if (status == FBE_RAID_FRU_ERROR_STATUS_ERROR)
    {
        /* We should never get a hard error on a write.
         */
        if (RAID_COND(eboard.hard_media_err_count != 0))
        {
            fbe_raid_siots_set_unexpected_error(siots_p,
                                                FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "parity: hard error count unexpected: %d \n",
                                                eboard.hard_media_err_count);
            fbe_raid_fru_eboard_display(&eboard, siots_p, FBE_TRACE_LEVEL_ERROR);
            return FBE_RAID_STATE_STATUS_EXECUTING; 
        }
        else if ((eboard.dead_err_count <= fbe_raid_siots_parity_num_positions(siots_p)) &&
                 (eboard.dead_err_count > 0))
        {
            /* For RAID5/RAID3, if one error then return good status.
             * For RAID 6, if one or two error(s) return good status.
             */
            fbe_raid_siots_transition(siots_p, fbe_parity_rekey_state20);
        }
        else
        {
            /* Uknown error
             */
            fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "parity: %s fru error status unknown 0x%x\n", 
                                                __FUNCTION__, status);
            fbe_raid_fru_eboard_display(&eboard, siots_p, FBE_TRACE_LEVEL_ERROR);
        }
    }    /* end else if (status == FBE_RAID_FRU_ERROR_STATUS_ERROR)*/
    else
    {
        /* This case should have been handled above.
         */
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity: %s fru status unknown 0x%x\n", 
                                            __FUNCTION__, status);
        fbe_raid_fru_eboard_display(&eboard, siots_p, FBE_TRACE_LEVEL_ERROR);
    }

    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/*******************************
 * end og fbe_parity_rekey_state12()
 *******************************/
/****************************************************************
 * fbe_parity_rekey_state20()
 ****************************************************************
 * @brief
 *  The writes were successful, return good status.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t              
 *
 * @return
 *  FBE_RAID_STATE_STATUS_WAITING or FBE_RAID_STATE_STATUS_EXECUTING
 *
 * @author
 *  10/16/2013 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_raid_state_status_t fbe_parity_rekey_state20(fbe_raid_siots_t * siots_p)
{
    fbe_raid_siots_transition(siots_p, fbe_raid_siots_success);
    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/*******************************
 * end of fbe_parity_rekey_state20()
 *******************************/
/*****************************************
 * End fbe_parity_rekey.c
 *****************************************/
