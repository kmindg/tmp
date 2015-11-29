/***************************************************************************
 * Copyright (C) EMC Corporation 1999-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_parity_mr3.c
 ***************************************************************************
 *
 * @brief
 *   This file contains the functions of the
 *   RAID 5 MR3 (Modified RAID 3) Write state machine.
 *   The MR3 writes the same amount of data to all drives in a
 *   RAID 5 redundancy group.      When this sequence of sectors for a
 *   specific drive is not provided by the host (or buffered) then 
 *   this data is pre-read.
 * 
 *   When the pre-reads have all completed successfully, the CSUM/XOR is performed to
 *   calculate new parity using the new host data and old pre-read data.
 *
 *   If the CSUM/XOR is successful, then writes of data and parity
 *   are sent to all drives in the unit.
 *
 *   In MR3 a CSUM/XOR performs these operations:
 * 
 *     1) new parity calculated using data in buffers from all drives
 *     2) checksums are added if needed, and stamps are added
 *
 *   Exceptional Conditions:
 *
 *         1) a single fault during pre-reads,
 *         2) a double-fault during pre-reads,
 *         3) a corruption of pre-read data,
 *         4) a corruption of new data [not from the host],
 *         5) a single-fault during writes, or
 *         6) a double-fault during writes.
 *
 *   These conditions are handled by the following actions:
 *         - shutting down the unit and aborting the write (2,7),
 *         - transferring to a seperate recovery path (1,3,4),
 *         - aborting the write (7),
 *         - completing the write successfully (6), or
 *         - a PANIC (5).
 *
 *   The new approach for RAID 6 in the following section:
 *
 *   The RAID 6 will use the degraded write state machine for the case where
 *   both parities are dead.  We will use MR3 and 468 for degraded write where
 *   at least one parity drive is alive. The main difference for these state 
 *   machines is that any pre-read that touches a dead position needs to be 
 *   reconstructed.  It is easy to see why we need to reconstruct the pre-read
 *   data, since we need the pre-read data in order to update parity.
 *
 *   So when the MR3 or 468 state machine reaches the pre-read phase, if a 
 *   reconstruct is required then verify state machine will be used to do all 
 *   pre-reads and reconstructs of pre-read data.  When we return to the MR3/468
 *   state machine then all pre-read data will be available, and the XOR/write will 
 *   be performed as normal.  
 *
 *   The MR3 state machine in RAID 6 will use the this state machine and make 
 *   some changes in r5_mr3_state15, then add two new functions to handle 
 *   degraded MR3 write.
 *    
 *   There are two paths in r5_mr5_state15:
 *   -  RAID6 with two dead parity drives will be handled the same way as RAID5
 *      degraded mode. It will setup SIOTS for degraded write and transit to 
 *      degraded write state machine.
 *   -  If RAID6 with degraded mode and at least has one parity drive is alive,
 *      we will call fbe_raid_siots_start_nested_siots(siots_p, r5_deg_vr_state0) to setup nest SIOTS
 *      for degraded verify; and then transit to Degraded Verify to reconstruct
 *      and perform pre-reads into the buffers provided by the parent SIOTS.
 *      When Degraded Verify is finished, control is returned to the parent 
 *      SIOTS to continue the write operation.  We will call a new function 
 *      r5_mr3_state22() to evaluate the return codes from child SIOTS. If 
 *      FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED is returned, it will go to 
 *      fbe_raid_siots_shutdown_error(). If FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS is returned, it will
 *      go to r5_mr3_state10() to continue the MR3 write operation.
 * 
 *    MR3 error handling:
 *    - r5_mr3_state8 (handles the return of status from pre-read)
 *      FBE_RAID_FRU_ERROR_STATUS_ERROR - 1 or 2 dead error(s) - changed from current code, 
 *                     transition to state15 to kick off degraded verify.
 *      FBE_RAID_FRU_ERROR_STATUS_ERROR - hard errors - transition to state16 to kick off recovery
 *                     verify.
 *      FBE_RAID_FRU_ERROR_STATUS_ERROR - all other errors are unchanged from current code.
 *      FBE_RAID_FRU_ERROR_STATUS_SHUTDOWN - Transition to fbe_raid_siots_shutdown_error.
 *
 *    - r5_mr3_state12 (handles the return of status from write)
 *      FBE_RAID_FRU_ERROR_STATUS_ERROR - 1 or 2 dead error(s) - transition to state 17 and return
 *                     the good status.
 *      FBE_RAID_FRU_ERROR_STATUS_ERROR - all other errors are unchanged from current code.
 *      FBE_RAID_FRU_ERROR_STATUS_SHUTDOWN - Transition to fbe_raid_siots_shutdown_error.
 *
 * @notes
 *   At initial entry, we assume that the fbe_raid_siots_t has 
 *   The following fields initialized:
 *
 *         start_lba       - logical address of first block of new data
 *         xfer_count      - total blocks of new data
 *         data_disks      - total data disks affected [data only]
 *                      
 *         parity_pos      - position occupied by parity
 *         start_pos       - position the request starts on.
 *         parity_start    - first block  of affected parity
 *         parity_count    - total blocks of affected parity
 * 
 * @author
 *  10/13/2000 Created from old XD/Hi5 log aligned mr3 code. Rob Foley.
 *  04/01/2006 Added RAID 6. CLC  
 *
 ***************************************************************************/

/*************************
 **    INCLUDE FILES    **
 *************************/
#include "fbe_raid_library_proto.h"
#include "fbe_parity_io_private.h"

/* Make sure that our wait count is sane.
 * We specifically do not want to do this check
 * under free builds because it is expensive to
 * traverse the fruts chains.
 */
#ifdef FBE_RAID_DEBUG
fbe_status_t fbe_parity_mr3_validate_wait_count(fbe_raid_siots_t *siots_p)
{
    fbe_u32_t count = 0;
    fbe_status_t status = FBE_STATUS_OK;
    count += fbe_queue_length(&siots_p->read_fruts_head);
    count += fbe_queue_length(&siots_p->read2_fruts_head);

    if (RAID_COND(count != siots_p->wait_count))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, "count 0x%x != siots_p->wait_count 0x%x",
            count, siots_p->wait_count);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}
#else
fbe_status_t fbe_parity_mr3_validate_wait_count(fbe_raid_siots_t *siots_p)
{
    return FBE_STATUS_OK;
}
#endif
/****************************************************************
 *  fbe_parity_mr3_state0()
 ****************************************************************
 * @brief
 *   This function allocates [the maximum] number of FRU
 *   tracking structures we will need for this sub-request.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t
 *
 * @return
 *   This function returns FBE_RAID_STATE_STATUS_WAITING if we were unable
 *   to allocate the required number of structures immediately;
 *   otherwise, FBE_RAID_STATE_STATUS_EXECUTING is returned.
 *
 * @author
 *   6/24/99 - Created. Rob Foley
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_parity_mr3_state0(fbe_raid_siots_t * siots_p)
{
    fbe_status_t            status;
    /* Add one for terminator.
     * Read does not need term since max number is width-1 (not parity). 
     */
    fbe_raid_fru_info_t read_info[FBE_RAID_MAX_DISK_ARRAY_WIDTH];
    fbe_raid_fru_info_t write_info[FBE_RAID_MAX_DISK_ARRAY_WIDTH+1];
    fbe_raid_fru_info_t read2_info[FBE_RAID_MAX_DISK_ARRAY_WIDTH];

    /* Validate the algorithm.
     */
    if (siots_p->algorithm != R5_MR3)
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity: algorithm %d unexpected\n", 
                                            siots_p->algorithm);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }
    if (fbe_parity_generate_validate_siots(siots_p) == FBE_FALSE)
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity: validate failed algorithm: %d\n",
                                            siots_p->algorithm);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* We should always be aligned to the optimal size, since
     * this path does not handle pre-reads.
     */
    if (!fbe_raid_is_aligned_to_optimal_block_size(siots_p->parity_start,
                                                   siots_p->parity_count,
                                                   fbe_raid_siots_get_raid_geometry(siots_p)->optimal_block_size))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity: parity start/cnt %llx %llu not aligned to opt size %d\n", 
                                            (unsigned long long)siots_p->parity_start,
                                            (unsigned long long)siots_p->parity_count,
                                            fbe_raid_siots_get_raid_geometry(siots_p)->optimal_block_size);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* Calculate and allocate all the memory needed for this siots.
     */
    status = fbe_parity_mr3_calculate_memory_size(siots_p, &read_info[0], &read2_info[0], &write_info[0]);
    if (status != FBE_STATUS_OK)
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity: failed to calculate mem size status: 0x%x algorithm: 0x%x\n", 
                                            status, siots_p->algorithm);
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
    fbe_raid_siots_transition(siots_p, fbe_parity_mr3_state1);
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
        status = fbe_parity_mr3_setup_resources(siots_p, &read_info[0], &read2_info[0], &write_info[0]);
        if (RAID_COND(status != FBE_STATUS_OK))
        {
            fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity:mr3 fail to setup resource.stat:0x%x\n",
                                            status);
            return FBE_RAID_STATE_STATUS_EXECUTING;
        }

        /* To prevent confusion (i.e. we never went to `deferred allocation complete'
         * state) overwrite the current state.
         */
        fbe_raid_common_set_state(&siots_p->common, (fbe_raid_common_state_t)fbe_parity_mr3_state3);
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
                                            "parity: mr3 memory request failed with status: 0x%x\n",
                                            status);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/******************************
 * end of fbe_parity_mr3_state0
 ******************************/

/****************************************************************
 * fbe_parity_mr3_state1()
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

fbe_raid_state_status_t fbe_parity_mr3_state1(fbe_raid_siots_t * siots_p)
{
    fbe_status_t            status;
    /* Add one for terminator.
     */
    fbe_raid_fru_info_t     read_info[FBE_RAID_MAX_DISK_ARRAY_WIDTH+1];
    fbe_raid_fru_info_t     read2_info[FBE_RAID_MAX_DISK_ARRAY_WIDTH+1];
    fbe_raid_fru_info_t     write_info[FBE_RAID_MAX_DISK_ARRAY_WIDTH+1];

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
                             "parity: mr3 memory allocation error common flags: 0x%x \n",
                             fbe_raid_common_get_flags(&siots_p->common));
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_memory_allocation_error);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* Then calculate how many buffers are needed.
     */
    status = fbe_parity_mr3_calculate_memory_size(siots_p, &read_info[0], &read2_info[0], &write_info[0]);
    if (status != FBE_STATUS_OK)
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity: failed to calculate mem size status: 0x%x algorithm: 0x%x\n", 
                                            status, siots_p->algorithm);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* Allocation completed. Plant the resources
     */
    status = fbe_parity_mr3_setup_resources(siots_p, &read_info[0], &read2_info[0], &write_info[0]);
    if (RAID_COND(status != FBE_STATUS_OK))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity:mr3 fail to setup resource.stat:0x%x\n",
                                            status);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* Transition to next state to issue request
     */
    fbe_raid_siots_transition(siots_p, fbe_parity_mr3_state3);

    /* Return the state status
     */
    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/*******************************
 * end fbe_parity_mr3_state1()
 *******************************/

/****************************************************************
 * fbe_parity_mr3_state3()
 ****************************************************************
 * @brief
 *   All resources have been acquired.
 *   Kick off additional sub-requests or waiting requests.
 *   Initialize the sgs, and start the pre-reads.
 *   If this is a cache op without pre-reads we 
 *   transition to the next state to xor and start writes.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t
 *
 * @return
 *   This function returns FBE_RAID_STATE_STATUS_WAITING if there are
 *   no pre-reads and this is a cache op;
 *   otherwise, returns FBE_RAID_STATE_STATUS_WAITING.
 *
 * @author
 *   6/24/99 - Created. Rob Foley
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_parity_mr3_state3(fbe_raid_siots_t * siots_p)
{
    fbe_payload_block_operation_opcode_t opcode;
    fbe_raid_state_status_t status = FBE_RAID_STATE_STATUS_WAITING;
    fbe_status_t fbe_status = FBE_STATUS_OK;
    fbe_raid_fruts_t *read_fruts_ptr = NULL;
    fbe_raid_fruts_t *read2_fruts_ptr = NULL;
    fbe_bool_t b_preread_degraded = FBE_FALSE;
    fbe_bool_t b_result = FBE_TRUE;
    fbe_bool_t b_generate_next_siots = FBE_TRUE;
    fbe_raid_iots_t *iots_p = fbe_raid_siots_get_iots(siots_p);

    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_ptr);
    fbe_raid_siots_get_read2_fruts(siots_p, &read2_fruts_ptr);

    fbe_raid_siots_check_traffic_trace(siots_p, FBE_TRUE);

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
        fbe_raid_siots_transition(siots_p, fbe_parity_mr3_state3);
        return FBE_RAID_STATE_STATUS_WAITING;
    }

    siots_p->wait_count = fbe_queue_length(&siots_p->read_fruts_head);
    siots_p->wait_count += fbe_queue_length(&siots_p->read2_fruts_head);

    /***************************************************
     * If a verify is needed due to BVA do it now, whether
     * degraded or not.
     ***************************************************/
    fbe_raid_siots_get_opcode(siots_p, &opcode);
    
    /***************************************************
     * Since we may have waited before getting here,
     * check if we are degraded now.
     ***************************************************/
    if (RAID_COND(fbe_parity_is_preread_degraded(siots_p, &b_preread_degraded) != FBE_STATUS_OK))
    {
        /* Any status besides 'ok' is an unexpected error case.
         */
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity: mr3 error checking preread degraded\n");
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* Determine if we need to generate another siots.
     * If this returns TRUE then we must always call fbe_raid_iots_generate_next_siots_immediate(). 
     * Typically this call is done after we have started the disk I/Os.
     */
    fbe_status = fbe_raid_siots_should_generate_next_siots(siots_p, &b_generate_next_siots);
    if (RAID_GENERATE_COND(fbe_status != FBE_STATUS_OK))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS, 
                                            "parity: %s failed in next siots generation: siots_p = 0x%p, status = %dn",
                                            __FUNCTION__, siots_p, fbe_status);
        return(FBE_RAID_STATE_STATUS_EXECUTING);
    }

    if (b_preread_degraded == FBE_TRUE)
    {
        /* Set wait count to zero since we might have incremented it above.
         */
        siots_p->wait_count = 0;

        /* We went degraded, transition to degraded path.
         */
        fbe_raid_siots_transition(siots_p, fbe_parity_mr3_state15);
        status = FBE_RAID_STATE_STATUS_EXECUTING;
    }
    else
    {
        /* Increment the wait count for our pre-reads.
         * Before submitting operations to the lower driver
         * the wait counts should already be incremented,
         * since the call (hstart) itself might trigger the wakeup function.
         */
        fbe_status = fbe_parity_mr3_validate_wait_count(siots_p);
        if(RAID_COND(fbe_status != FBE_STATUS_OK))
        {
            /* Need to clear this so we do not wait for the generate which will never occur.
             */
            if (b_generate_next_siots) { fbe_raid_iots_cancel_generate_next_siots(iots_p); }
            fbe_raid_siots_set_unexpected_error(siots_p, 
                                                FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "parity: %s status 0x%x unexpected while operating siots_p = 0x%p\n",
                                                __FUNCTION__, status, siots_p);
            return FBE_RAID_STATE_STATUS_EXECUTING;
        }

        /* If we do not need to wait, go directly to xor.
         */
        if (siots_p->wait_count == 0)
        {
            /**********************************************
             * We have no pre-reads to do
             * go directly to the xor and write.
             **********************************************/

            fbe_raid_siots_transition(siots_p, fbe_parity_mr3_state7);
            status = FBE_RAID_STATE_STATUS_EXECUTING;
        }
        else
        {
            /* We need to start pre-read(s) 
             */
            fbe_bool_t requests_started = FBE_FALSE;

            /* Wait for outstanding pre-reads.
             */
            fbe_raid_siots_transition(siots_p, fbe_parity_mr3_state6);
            status = FBE_RAID_STATE_STATUS_WAITING;

            if (read_fruts_ptr)
            {
                /* Start beginning of stripe pre-read.
                 */ 
                requests_started = FBE_TRUE;
                b_result = fbe_raid_fruts_send_chain(&siots_p->read_fruts_head, fbe_raid_fruts_evaluate);
                if (RAID_FRUTS_COND(b_result != FBE_TRUE))
                {
                    /* We hit an issue while starting chain of fruts.
                    */
                    fbe_raid_siots_set_unexpected_error(siots_p, 
                                                        FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                        "parity: %s failed to start read fruts chain for "
                                                        "siots = 0x%p, b_result = %d\n",
                                                        __FUNCTION__, siots_p, b_result);
                }
            }

            if (read2_fruts_ptr)
            {
                /* Start end of stripe pre-read.
                 */
                requests_started = FBE_TRUE;
                b_result = fbe_raid_fruts_send_chain(&siots_p->read2_fruts_head, fbe_raid_fruts_evaluate);
                if (RAID_FRUTS_COND(b_result != FBE_TRUE))
                {
                    /* We hit an issue while starting chain of fruts.
                    */
                    fbe_raid_siots_set_unexpected_error(siots_p, 
                                                        FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                        "parity: %s failed to start read2 fruts chain for "
                                                        "siots = 0x%p, b_result = %d\n",
                                                        __FUNCTION__, siots_p, b_result);
                }
            }
            /* cache op and there are outstanding pre-reads
             */
            if (requests_started == FBE_FALSE)
            {
                /* We did not start pre-reads,
                 * something went wrong...
                 */
                fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                      "parity: %s no fruts started\n", __FUNCTION__);
                status = FBE_RAID_STATE_STATUS_EXECUTING;
            }
        } /* end wait_count != 0 */
    } /* end not degraded */

    /* Kick off the next siots in the chain if necessary.
     */
    if (b_generate_next_siots)
    {
        fbe_raid_iots_generate_next_siots_immediate(iots_p);
    }
    return status;
}
/**************************
 * end of fbe_parity_mr3_state3
 **************************/

/*!***************************************************************
 * fbe_parity_mr3_state6()
 ****************************************************************
 * @brief
 *   Pre-reads have completed.
 *   Check status of pre-reads, and transition to the next state.
 *
 *   siots_p - ptr to the fbe_raid_siots_t
 *
 * @return
 *   fbe_raid_state_status_t
 *
 * @author
 *   7/6/99 - Created. Rob Foley
 *   7/3/08 - Modified for Rebuild Logging Support For R6. Rob Foley
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_parity_mr3_state6(fbe_raid_siots_t * siots_p)
{
    /* Check number of parity that are supported. R5=1, R6=2
     */
    fbe_u32_t parity_drives = fbe_raid_siots_parity_num_positions(siots_p);
    fbe_raid_fru_eboard_t eboard;
    fbe_raid_fru_error_status_t read1_status = FBE_RAID_FRU_ERROR_STATUS_SUCCESS;
    fbe_raid_fru_error_status_t read2_status = FBE_RAID_FRU_ERROR_STATUS_SUCCESS;    
    fbe_raid_fruts_t *read_fruts_ptr = NULL;
    fbe_raid_fruts_t *read2_fruts_ptr = NULL;

    /* Check status of the pre-reads.
     */
    fbe_raid_fru_eboard_init(&eboard);

    /* Get totals for errored or dead pre-read drives.
     */
    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_ptr);
    if (read_fruts_ptr != NULL)
    {
        read1_status = fbe_parity_get_fruts_error(read_fruts_ptr, &eboard);
    }

    fbe_raid_siots_get_read2_fruts(siots_p, &read2_fruts_ptr);
    if ((read2_fruts_ptr != NULL) &&
        ((read1_status != FBE_RAID_FRU_ERROR_STATUS_SHUTDOWN) && 
         (read1_status != FBE_RAID_FRU_ERROR_STATUS_WAITING)))
    {
        read2_status = fbe_parity_get_fruts_error(read2_fruts_ptr, &eboard);
    }

    /* Waiting takes precidence over all other status values. 
     * If we are waiting, the monitor could restart us. 
     */
    if ((read1_status == FBE_RAID_FRU_ERROR_STATUS_WAITING) ||
        (read2_status == FBE_RAID_FRU_ERROR_STATUS_WAITING))
    {
        /* One drive is dead, we need to mark this request
         * as waiting for a shutdown or continue.
         */
        return FBE_RAID_STATE_STATUS_WAITING;
    }
    /* If we have media errors and retryable errors, then goto verify to handle the 
     * errors. 
     */
    else if ((eboard.hard_media_err_count > 0) &&
             ((read1_status == FBE_RAID_FRU_ERROR_STATUS_RETRY) ||
              (read2_status == FBE_RAID_FRU_ERROR_STATUS_RETRY)))
    {
        read1_status = read2_status = FBE_RAID_FRU_ERROR_STATUS_ERROR;
    }

    if (fbe_raid_siots_is_aborted(siots_p))
    {
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_aborted);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    if ((read1_status == FBE_RAID_FRU_ERROR_STATUS_SUCCESS) && 
        (read2_status == FBE_RAID_FRU_ERROR_STATUS_SUCCESS))
    {
        if(RAID_COND((eboard.dead_err_count != 0) ||
                        (eboard.hard_media_err_count != 0)))
        {
            fbe_raid_siots_set_unexpected_error(siots_p, 
                                                FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "parity: dead_err_count 0x%x and hard_err_count 0x%x unexpected\n",
                                                eboard.dead_err_count,
                                                eboard.hard_media_err_count);
            fbe_raid_fru_eboard_display(&eboard, siots_p, FBE_TRACE_LEVEL_ERROR);
            return FBE_RAID_STATE_STATUS_EXECUTING;
        }

        /* All pre-reads successful.
         */
        fbe_raid_siots_transition(siots_p, fbe_parity_mr3_state7);
    }
    else if ((read2_status == FBE_RAID_FRU_ERROR_STATUS_SHUTDOWN) || 
             (read1_status == FBE_RAID_FRU_ERROR_STATUS_SHUTDOWN))
    {
        /* More than two pre-read failed for Raid 6 or more than one 
         * pre-read failed for everything else due to dead drive(s)
         * or the unit is shutdown.
         */
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_shutdown_error);
    }
    else if ((read2_status == FBE_RAID_FRU_ERROR_STATUS_DEAD) ||
             (read1_status == FBE_RAID_FRU_ERROR_STATUS_DEAD))
    {
        /* Monitor-initiated operation hit a dead error.
         */
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_dead_error);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }
    else if ((read1_status == FBE_RAID_FRU_ERROR_STATUS_RETRY) ||
             (read2_status == FBE_RAID_FRU_ERROR_STATUS_RETRY))
    {
        fbe_status_t retry_status;
        /* Send out retries.  We will stop retrying if we get aborted or 
         * our timeout expires or if an edge goes away. 
         */
        retry_status = fbe_parity_mr3_handle_retry_error(siots_p, &eboard, read1_status, read2_status);
        if (retry_status != FBE_STATUS_OK)
        {
            fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "retry %d errors (bitmap 0x%x) status: %d unexpected\n", 
                                                eboard.retry_err_count, eboard.retry_err_bitmap, retry_status);
            fbe_raid_fru_eboard_display(&eboard, siots_p, FBE_TRACE_LEVEL_ERROR);
            return FBE_RAID_STATE_STATUS_EXECUTING;
        }
        return FBE_RAID_STATE_STATUS_WAITING;
    }
    else if ((read1_status == FBE_RAID_FRU_ERROR_STATUS_WAITING) || 
             (read2_status == FBE_RAID_FRU_ERROR_STATUS_WAITING))
    {
        /* For Raid 6 if two drive are dead or for other raid type if one
         * drive is dead, we need to mark this request as waiting for a
         * shutdown or continue.
         * Note that we check this *before* checking FBE_RAID_FRU_ERROR_STATUS_ERROR, since
         * any waiting error takes precidence over a FRU_ERROR.
         */
        return FBE_RAID_STATE_STATUS_WAITING;
    }
    else if ((read1_status == FBE_RAID_FRU_ERROR_STATUS_ERROR) || 
             (read2_status == FBE_RAID_FRU_ERROR_STATUS_ERROR))
    {
        if ((eboard.dead_err_count <= parity_drives) &&
            (eboard.dead_err_count > 0))
        {
            /* A pre-read failed.  If this started as a degraded IO and thus we have
             * write log headers allocated, shift to degraded verify path; if this IO
             * just went degraded, abort the request and retry it as degraded so we
             * have write log header buffers.
             */
            if (fbe_raid_siots_is_flag_set(siots_p, FBE_RAID_SIOTS_FLAG_WRITE_LOG_HEADER_REQD))
            {
                fbe_raid_siots_transition(siots_p, fbe_parity_mr3_state15);
            }
            else
            {
                /* Abort the request so that its retried as a degraded-IO and
                 * siots will be generated such that write logging is possible. 
                 */
                fbe_raid_siots_transition(siots_p, fbe_raid_siots_degraded);   
            }
        }
        else if (eboard.hard_media_err_count > 0)
        {
            /* [At least] one pre-read failed due to
             * a hard error; shift to recovery verify .
             */
            fbe_raid_siots_transition(siots_p, fbe_parity_mr3_state16);
        }
        else if (eboard.drop_err_count > 0)
        {
            /* At least one read failed due to a DROPPED.
             */
            fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                  "parity: %s fru dropped error unexpected "
                                  "drop_cnt: 0x%x drop_bitmap: 0x%x\n", 
                                  __FUNCTION__, eboard.drop_err_count, eboard.drop_err_bitmap);
            fbe_raid_fru_eboard_display(&eboard, siots_p, FBE_TRACE_LEVEL_ERROR);
            return FBE_RAID_STATE_STATUS_EXECUTING;
        }
        else
        {
            /* This status is not expected.
             */
            fbe_raid_siots_set_unexpected_error(siots_p,
                                                FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "parity: %s got unexpected fru status for siots_p = 0x%p "
                                                "r1_status: 0x%x, r2_status: 0x%x\n", 
                                                __FUNCTION__,
                                                siots_p,
                                                read1_status,
                                                read2_status );
            fbe_raid_fru_eboard_display(&eboard, siots_p, FBE_TRACE_LEVEL_ERROR);
            return FBE_RAID_STATE_STATUS_EXECUTING;
        }
    }    /* End if error */
    else
    {
        fbe_raid_fru_eboard_display(&eboard, siots_p, FBE_TRACE_LEVEL_ERROR);
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "%s unexpected status r1: %d r2: %d\n", 
                                            __FUNCTION__, read1_status, read2_status);
    }
    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/**************************
 * end of fbe_parity_mr3_state6
 **************************/

/****************************************************************
 *  fbe_parity_mr3_state7()
 ****************************************************************
 * @brief
 *   Pre-reads have completed successfully, start the xor.
 *  
 * @param siots_p - ptr to the fbe_raid_siots_t                      
 *
 * @return
 *   This function returns FBE_RAID_STATE_STATUS_EXECUTING/FBE_RAID_STATE_STATUS_WAITING.
 *
 * @author
 *   7/7/99 - Created. Rob Foley
 *   3/8/00 - Modified. MPG
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_parity_mr3_state7(fbe_raid_siots_t * siots_p)
{
    fbe_status_t status;

    fbe_raid_siots_transition(siots_p, fbe_parity_mr3_state8);

    /* Get the data for a buffered operation.
     */
    if (fbe_raid_siots_is_buffered_op(siots_p))
    {
        status = fbe_parity_mr3_get_buffered_data(siots_p);
        if (status != FBE_STATUS_OK)
        {
            fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "parity: fbe_raid_xor_get_buffered_data returned status: %d\n", status);
            return FBE_RAID_STATE_STATUS_EXECUTING;
        }
    }

    /* Start the command to check any pre-read data and 
     * calculate new parity. 
     */
    status = fbe_parity_mr3_xor(siots_p);
        
    if(RAID_COND(status != FBE_STATUS_OK))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity: status 0x%x unexpected\n",
                                            status);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/**************************************
 * end fbe_parity_mr3_state7()
 **************************************/
/****************************************************************
 *  fbe_parity_mr3_state8()
 ****************************************************************
 * @brief
 *  Check the xor status.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t
 *
 * @return
 *   fbe_raid_state_status_t
 *
 * @author
 *   3/8/00 - Created. MPG
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_parity_mr3_state8(fbe_raid_siots_t * siots_p)
{
    fbe_raid_state_status_t status = FBE_RAID_STATE_STATUS_WAITING;
    fbe_xor_status_t xor_status;
    fbe_raid_fruts_t *write_fruts_ptr = NULL;
    fbe_bool_t b_pre_read_error;

    if (fbe_raid_is_option_flag_set(fbe_raid_siots_get_raid_geometry(siots_p),
                                    FBE_RAID_OPTION_FLAG_FORCE_RVR_READ))
    {
        /* This mode forces us to always do a recovery verify write
         * even though one might not be needed.
         */
        b_pre_read_error = FBE_TRUE;
    }
    fbe_raid_siots_get_xor_command_status(siots_p, &xor_status);
    fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_ptr);

    b_pre_read_error = (xor_status & (FBE_XOR_STATUS_CHECKSUM_ERROR | FBE_XOR_STATUS_CONSISTENCY_ERROR));

    /* Make sure every outstanding request is done
     */
    if(RAID_COND(siots_p->wait_count != 0))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity: wait_count 0x%llx unexpected\n",
                                            siots_p->wait_count);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }
    if (xor_status == FBE_XOR_STATUS_NO_ERROR)
    {
        /* the XOR completed with no errors,
         * continue with the write. If degraded, the writes need to be written to the write log first.
         */
        if ((fbe_raid_siots_dead_pos_get(siots_p, FBE_RAID_FIRST_DEAD_POS) != FBE_RAID_INVALID_DEAD_POS)
            && fbe_parity_is_write_logging_required(siots_p))
        {
            /* There is a dead position.
             * If the io io was degraded at generate and thus is not too big and 
             * allocated write log header buffers, transition to writing out the write log.
             */
            if (fbe_raid_siots_is_flag_set(siots_p, FBE_RAID_SIOTS_FLAG_WRITE_LOG_HEADER_REQD))
            {
                fbe_raid_siots_transition(siots_p, fbe_parity_mr3_state9);
            }
            else
            {
                /* Abort the request so that its retried as a degraded-IO and
                 * siots will be generated such that write logging is possible. 
                 */
                fbe_raid_siots_transition(siots_p, fbe_raid_siots_degraded);   
            }
        }
        else
        {
            /* No dead positions, writes may immediately be written to disk.
             */
            fbe_raid_siots_transition(siots_p, fbe_parity_mr3_state13);
        }
        status = FBE_RAID_STATE_STATUS_EXECUTING;
    }
    else if (b_pre_read_error)
    {
        /* A checksum error was detected in the pre-read data,
         * head down the verify recovery path.
         */
        fbe_raid_siots_transition(siots_p, fbe_parity_mr3_state16);
        status = FBE_RAID_STATE_STATUS_EXECUTING;
    } /* Determine if Cache gave us bad data */
    else if (0 != (xor_status & FBE_XOR_STATUS_BAD_MEMORY))
    {
        /* The caller gave the RAID Driver blocks with
         * invalid checksums.Return an error.
         */
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_write_crc_error);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }
    else if (0 != (xor_status & FBE_XOR_STATUS_BAD_METADATA))
    {
        /* This is an unexpected xor status.
         */
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                              "parity: %s unexpected xor bad metadata status: 0x%x\n", 
                              __FUNCTION__, xor_status);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }
    else
    {
        /* XOR detected bad stamps or bad checksum on host data.
         */
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                              "parity: %s unexpected xor bad metadata status: 0x%x\n", 
                              __FUNCTION__, xor_status);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }
    return status;
}
/*******************************
 * end of fbe_parity_mr3_state8
 *******************************/
/****************************************************************
 * fbe_parity_mr3_state9()
 ****************************************************************
 * @brief
 *  Need to write to the write log before completing the write.
 *  First ask the raid group write log slot manager for a slot
 *  for this write.
 *
 * @param siots_p - current sub request
 *
 * @return
 *  This function returns FBE_RAID_STATE_STATUS_WAITING
 *  if there are no available slots, and returns
 *  FBE_RAID_STATUS_EXECUTING if it gets a slot.
 *
 * @author
 *  8/23/10 - Created. Kevin Schleicher
 *
 ****************************************************************/
fbe_raid_state_status_t fbe_parity_mr3_state9(fbe_raid_siots_t * siots_p)
{
    fbe_status_t fbe_status;
    fbe_raid_fruts_t *fruts_p;

    fbe_raid_siots_transition(siots_p, fbe_parity_mr3_state10);

    /* To avoid coming off the slot wait queue on the wrong CPU, setup the first write fruts
     * with the current cpu_id.  The slot code will requeue any waiting siots to that cpu.
     */
    fbe_raid_siots_get_write_fruts(siots_p, &fruts_p);
    fbe_status = fbe_raid_fruts_init_cpu_affinity_packet(fruts_p);
    if (fbe_status != FBE_STATUS_OK)
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "Parity: mr3 write log init cpu affinity packet unexpected status: 0x%x\n",
                                            fbe_status);
        return FBE_RAID_STATE_STATUS_EXECUTING; 
    }

    fbe_status = fbe_parity_write_log_allocate_slot(siots_p);
    if (fbe_status == FBE_STATUS_PENDING)
    {
        return FBE_RAID_STATE_STATUS_WAITING;
    }
    else
    {
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }
}
/*******************************
 * end fbe_parity_mr3_state9()
 *******************************/
/****************************************************************
 * fbe_parity_mr3_state10()
 ****************************************************************
 * @brief
 *  Need to write to the write log before completing the write.
 *  MDS has completed the request for a slot, calculate the lba's
 *  for all the write frut's and issue the writes to the actual log.
 *
 * @param siots_p - current sub request
 *
 * @return
 *  This function returns FBE_RAID_STATE_STATUS_WAITING or EXECUTING.
 *
 * @author
 *  8/23/10 - Created. Kevin Schleicher
 *  2/28/12 - Modified to use parity-resident write log slot entries. Dave Agans
 *
 ****************************************************************/
fbe_raid_state_status_t fbe_parity_mr3_state10(fbe_raid_siots_t * siots_p)
{
    fbe_raid_fruts_t *fruts_ptr = NULL;
    fbe_status_t fbe_status = FBE_STATUS_OK;
    fbe_status_t write_log_slot_status = FBE_STATUS_OK;
    fbe_bool_t b_result = FBE_TRUE;
    fbe_u32_t slot_index;

    /* If the slot returned from the write log is 'invalid' then the get slot
     * request was quiesced or attempted after the log was quiesced.
     */
    fbe_raid_siots_get_journal_slot_id(siots_p, &slot_index);
    if (slot_index == FBE_RAID_INVALID_JOURNAL_SLOT)
    {
        if (fbe_raid_siots_is_flag_set(siots_p, FBE_RAID_SIOTS_FLAG_WAIT_WRITE_LOG_SLOT))
        {
            /* This request was waiting on the write log queue at log quiesce.
             * It is now being restarted, so we clear the wait flag
             * and retry the request from state 9
             */
            fbe_raid_siots_clear_flag(siots_p, FBE_RAID_SIOTS_FLAG_WAIT_WRITE_LOG_SLOT);
            fbe_raid_siots_transition(siots_p, fbe_parity_mr3_state9);
            return FBE_RAID_STATE_STATUS_EXECUTING;
        }
        else
        {
            /* This request was rejected by the write log because the log had been quiesced.
             * Quiesce the siots and go wait in state 9 for restart
             */
            fbe_raid_siots_transition(siots_p, fbe_parity_mr3_state9);
            fbe_raid_siots_set_flag(siots_p, FBE_RAID_SIOTS_FLAG_QUIESCED);
            return FBE_RAID_STATE_STATUS_WAITING;
        }
    }

    /* Refresh our degraded positions (since we might have waited).
     * Also set any degraded position opcodes to nop.
     */
    fbe_parity_check_for_degraded(siots_p);
    fbe_raid_siots_get_write_fruts(siots_p, &fruts_ptr);
    fbe_raid_fruts_set_degraded_nop(siots_p, fruts_ptr);

    siots_p->wait_count = fbe_raid_fruts_count_active(siots_p, fruts_ptr);

    /* Sanity check the wait count, it must be within the width and nonzero.
     */
    if (siots_p->wait_count > fbe_raid_siots_get_width(siots_p))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity: mr3 wait_count unexpected %lld\n",
                                            siots_p->wait_count);

        write_log_slot_status = fbe_parity_write_log_release_slot(siots_p, __FUNCTION__, __LINE__);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }
    if (siots_p->wait_count == 0)
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity: mr3 wait_count unexpected %lld\n",
                                            siots_p->wait_count);

        write_log_slot_status = fbe_parity_write_log_release_slot(siots_p, __FUNCTION__, __LINE__);
        return FBE_RAID_STATE_STATUS_EXECUTING; 
    }

    /* Build the write log data block for the slot.
     */
    fbe_status = fbe_parity_journal_build_data(siots_p);
    if (fbe_status != FBE_STATUS_OK)
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "Parity: journal_build_data unexpected status: 0x%x\n",
                                            fbe_status);
        write_log_slot_status = fbe_parity_write_log_release_slot(siots_p, __FUNCTION__, __LINE__);
        return FBE_RAID_STATE_STATUS_EXECUTING; 
    }


    /* Given the slot id, calculate the lba's for each fruts for that given slot
     * and store them in the fruts. The original lba's will be restored when
     * the write log write completes.
     * Also prepare the slot headers
     */
    fbe_status = fbe_parity_prepare_journal_write(siots_p);
    if (fbe_status != FBE_STATUS_OK)
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "Parity: prepare_journal_write unexpected status: 0x%x\n",
                                            fbe_status);
        write_log_slot_status = fbe_parity_write_log_release_slot(siots_p, __FUNCTION__, __LINE__);
        return FBE_RAID_STATE_STATUS_EXECUTING; 
    }

    if (RAID_COND(fbe_queue_is_empty(&siots_p->write_fruts_head)))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity: mr3 no write fruts\n");
        write_log_slot_status = fbe_parity_write_log_release_slot(siots_p, __FUNCTION__, __LINE__);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* Send the writes for the write logging.
     */
    fbe_raid_siots_transition(siots_p, fbe_parity_mr3_state11);
    b_result = fbe_raid_fruts_send_chain(&siots_p->write_fruts_head, fbe_raid_fruts_evaluate);
    if (RAID_FRUTS_COND(b_result != FBE_TRUE))
    {
        /* We hit an issue while starting fruts chain
         */
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS, 
                                            "parity: mr3 %s fail to start write fruts chain for "
                                            "siots 0x%p, b_result=%d\n", 
                                            __FUNCTION__,
                                            siots_p,
                                            b_result);
        write_log_slot_status = fbe_parity_write_log_release_slot(siots_p, __FUNCTION__, __LINE__);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    return FBE_RAID_STATE_STATUS_WAITING;

}
/*******************************
 * end fbe_parity_mr3_state10()
 *******************************/
/****************************************************************
 * fbe_parity_mr3_state11()
 ****************************************************************
 * @brief
 *  Need to write to the write log before completing the write.
 *  Writes to the write log have completed, check status and
 *  move to validating the slot on success.
 *
 * @param siots_p - current sub request
 *
 * @return
 *  This function returns FBE_RAID_STATE_STATUS_WAITING.
 *
 * @author
 *  7/23/10 - Created. Kevin Schleicher
 *
 ****************************************************************/
fbe_raid_state_status_t fbe_parity_mr3_state11(fbe_raid_siots_t * siots_p)
{
    fbe_raid_fru_eboard_t eboard;
    fbe_raid_fru_error_status_t fru_status = FBE_RAID_FRU_ERROR_STATUS_INVALID;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_status_t write_log_slot_status = FBE_STATUS_OK;
    fbe_raid_fruts_t *write_fruts_ptr = NULL;

    fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_ptr);
    fbe_raid_fru_eboard_init(&eboard);

	/* we will hit this code when the monitor restarted our siots due to a 
	 * shutdown
	 */
    if (fbe_raid_siots_is_aborted_for_shutdown(siots_p))
    {
        fbe_raid_siots_clear_flag(siots_p,FBE_RAID_SIOTS_FLAG_WAITING_SHUTDOWN_CONTINUE);

        fbe_raid_siots_transition(siots_p, fbe_raid_siots_shutdown_error);
        write_log_slot_status = fbe_parity_write_log_release_slot(siots_p, __FUNCTION__, __LINE__);

        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* Get totals for errored or dead drives.
     */
    fru_status = fbe_parity_get_fruts_error(write_fruts_ptr, &eboard);

    if (fru_status == FBE_RAID_FRU_ERROR_STATUS_SUCCESS)
    {
        /* Write successful, move onto validating the slot.
         */
        fbe_raid_siots_transition(siots_p, fbe_parity_mr3_state12);
    }
    else if (fru_status == FBE_RAID_FRU_ERROR_STATUS_SHUTDOWN)
    {
        /* Multiple errors, we are going shutdown.
         * Release the slot and return bad status.
         */
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_shutdown_error);
        write_log_slot_status = fbe_parity_write_log_release_slot(siots_p, __FUNCTION__, __LINE__);
    }
    else if (fru_status == FBE_RAID_FRU_ERROR_STATUS_DEAD)
    {
        /* Monitor-initiated operation hit a dead error -- this will be abandoned, need to invalidate disk slot.
         */
        fbe_parity_write_log_set_slot_invalidate_state(siots_p, FBE_PARITY_WRITE_LOG_SLOT_INVALIDATE_STATE_DEAD,
                                                       __FUNCTION__, __LINE__);
        fbe_raid_siots_transition(siots_p, fbe_parity_mr3_state26);
    }
    else if (fru_status == FBE_RAID_FRU_ERROR_STATUS_WAITING)
    {
        /* One drive is dead, we need to mark this request
         * as waiting for a shutdown or continue.
         */
        return FBE_RAID_STATE_STATUS_WAITING; 
    }
    else if (fru_status == FBE_RAID_FRU_ERROR_STATUS_RETRY)
    {
         /* We might have gone degraded and have a retry.
          * In this case it is necessary to mark the degraded positions as nops since 
          * 1) we already marked the paged for these 
          * 2) the next time we run through this state we do not want to consider these failures 
          *    (and wait for continue) since we already got the continue. 
          */
        fbe_raid_fruts_set_degraded_nop(siots_p, write_fruts_ptr);
        /* Send out retries.  We will stop retrying if we get aborted or 
         * our timeout expires or if an edge goes away. 
         */
        status = fbe_raid_siots_retry_fruts_chain(siots_p, &eboard, write_fruts_ptr);
        if (status != FBE_STATUS_OK)
        {
            fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "parity: retry %d errors (bitmap 0x%x) status: %d unexpected\n", 
                                                eboard.retry_err_count, eboard.retry_err_bitmap, status);
            fbe_raid_fru_eboard_display(&eboard, siots_p, FBE_TRACE_LEVEL_ERROR);
            write_log_slot_status = fbe_parity_write_log_release_slot(siots_p, __FUNCTION__, __LINE__);
            return FBE_RAID_STATE_STATUS_EXECUTING;
        }

        return FBE_RAID_STATE_STATUS_WAITING;
    }
    else if (fru_status == FBE_RAID_FRU_ERROR_STATUS_ERROR)
    {
        /* No hard errors on writes.
         */
        if (eboard.hard_media_err_count != 0)
        {
            fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "parity: hard error count unexpected: %d \n",
                                                eboard.hard_media_err_count);
            write_log_slot_status = fbe_parity_write_log_release_slot(siots_p, __FUNCTION__, __LINE__);
            return FBE_RAID_STATE_STATUS_EXECUTING;
        }
        if (eboard.drop_err_count > 0)
        {
            /* DH Dropped the request,
             * we should not have submitted writes as optional.
             */
            fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "parity: %s fru dropped error unexpected status: 0x%x "
                                                "drop_cnt: 0x%x drop_bitmap: 0x%x\n", 
                                                __FUNCTION__, fru_status, eboard.drop_err_count, eboard.drop_err_bitmap);
            write_log_slot_status = fbe_parity_write_log_release_slot(siots_p, __FUNCTION__, __LINE__);
            return FBE_RAID_STATE_STATUS_EXECUTING;
        }
        else if ((eboard.dead_err_count <= fbe_raid_siots_parity_num_positions(siots_p)) &&
                 (eboard.dead_err_count > 0))
        {
            /* For RAID5/RAID3, if one error then return good status.
             * For RAID 6, if one or two error(s) return good status.
             * Move on to set up the I/O for the live stripe write.
             */
            fbe_raid_siots_transition(siots_p, fbe_parity_mr3_state12);
        }
        else
        {
            /* Uknown error
             */
            fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "parity: %s fru error status unknown 0x%x\n", 
                                                __FUNCTION__, fru_status);
            write_log_slot_status = fbe_parity_write_log_release_slot(siots_p, __FUNCTION__, __LINE__);
        }
    }    /* end else if (status == FBE_RAID_FRU_ERROR_STATUS_ERROR)*/
    else
    {
        /* This case should have been handled above.
         */
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity: %s fru status unknown 0x%x\n", 
                                            __FUNCTION__, fru_status);
        write_log_slot_status = fbe_parity_write_log_release_slot(siots_p, __FUNCTION__, __LINE__);
    }

    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/*******************************
 * end fbe_parity_mr3_state11()
 *******************************/

/****************************************************************
 * fbe_parity_mr3_state12()
 ****************************************************************
 * @brief
 *  Need to write to the write log before completing the write.
 *  Writes have completed succesfully to the write log, Restore
 *  the fruts lba's to the normal location to do the user data write, 
 *  and validate the slot.
 *
 * @param siots_p - current sub request
 *
 * @return
 *  This function returns FBE_RAID_STATE_STATUS_WAITING.
 *
 * @author
 *  8/05/10 - Created. Kevin Schleicher
 *
 ****************************************************************/
fbe_raid_state_status_t fbe_parity_mr3_state12(fbe_raid_siots_t * siots_p)
{
    /* Restore the previously stored original write lba's.
     */
    fbe_parity_restore_journal_write(siots_p);

    /* Move back to the normal write state machine to do the user data write.
     */
    fbe_raid_siots_transition(siots_p, fbe_parity_mr3_state13);

    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/*******************************
 * end fbe_parity_mr3_state12()
 *******************************/
/****************************************************************
 * fbe_parity_mr3_state13()
 ****************************************************************
 * @brief
 *   XOR was successfull, start the writes
 *
 * @param siots_p - ptr to the fbe_raid_siots_t
 *
 * @return
 *   This function always returns FBE_RAID_STATE_STATUS_WAITING.
 *
 * @author
 *   8/23/10 - Created. Kevin Schleicher
 *
 ****************************************************************/
fbe_raid_state_status_t fbe_parity_mr3_state13(fbe_raid_siots_t * siots_p)
{
    fbe_bool_t b_result = FBE_TRUE;
    fbe_status_t write_log_slot_status = FBE_STATUS_OK;
    fbe_raid_fruts_t *write_fruts_ptr = NULL;

    /***************************************************
     * Send out write requests.
     * Assume that any write requests that were busied 
     * are queued, and will eventually be restarted.
     ***************************************************/
    fbe_raid_siots_transition(siots_p, fbe_parity_mr3_state14);
    siots_p->wait_count = fbe_raid_siots_get_width(siots_p);

    /* Our wait count should be our chain length
     */
    if (RAID_COND(siots_p->wait_count != fbe_queue_length(&siots_p->write_fruts_head)))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, 
                FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                "parity: wait_count 0x%llx unexpected\n",
                siots_p->wait_count);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* Take any degraded positions out of the wait count.
     */
    siots_p->wait_count -= fbe_raid_siots_dead_pos_count(siots_p);

    /* Make sure we are writing out something.
     */
    if (RAID_COND(siots_p->wait_count > fbe_raid_siots_get_width(siots_p)))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, 
                FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                "parity: wait_count 0x%llx is not less than width \n",
                siots_p->wait_count);
        write_log_slot_status = fbe_parity_write_log_release_slot(siots_p, __FUNCTION__, __LINE__);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }
    if (RAID_COND(siots_p->wait_count == 0))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, 
                FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                "parity: wait_count 0x%llx is equal to zero\n",
                siots_p->wait_count);
        write_log_slot_status = fbe_parity_write_log_release_slot(siots_p, __FUNCTION__, __LINE__);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* Sets any degraded position opcodes to nop.
     */
    fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_ptr);
    fbe_raid_fruts_set_degraded_nop(siots_p, write_fruts_ptr);
    fbe_raid_siots_set_flag(siots_p, FBE_RAID_SIOTS_FLAG_WRITE_STARTED);

    b_result = fbe_raid_fruts_send_chain(&siots_p->write_fruts_head, fbe_raid_fruts_evaluate);
    if (RAID_FRUTS_COND(b_result != FBE_TRUE))
    {
        /* We hit an issue while starting fruts chain
         */
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS, 
                                            "parity: mr3 %s fail to start write fruts chain for "
                                            "siots 0x%p, b_result=%d\n", 
                                            __FUNCTION__,
                                            siots_p,
                                            b_result);
        write_log_slot_status = fbe_parity_write_log_release_slot(siots_p, __FUNCTION__, __LINE__);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    return FBE_RAID_STATE_STATUS_WAITING;
}
/*******************************
 * end of fbe_parity_mr3_state13
 *******************************/

/****************************************************************
 * fbe_parity_mr3_state14()
 ****************************************************************
 * @brief
 *   The writes completed, check the status of the writes.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t
 *
 * @return
 *   This function always returns FBE_RAID_STATE_STATUS_EXECUTING.
 *
 * @author
 *   7/7/99 - Created. Rob Foley
 *   4/25/12 - Harmonized 468 and mr3 sequence. Dave Agans
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_parity_mr3_state14(fbe_raid_siots_t * siots_p)
{
    fbe_raid_fru_eboard_t eboard;
    fbe_raid_fru_error_status_t status;
    fbe_status_t write_log_slot_status = FBE_STATUS_OK;
    fbe_raid_fruts_t *write_fruts_ptr = NULL;
    
    fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_ptr);
    fbe_raid_fru_eboard_init(&eboard);

    /* Get totals for errored or dead drives.
     */
    status = fbe_parity_get_fruts_error(write_fruts_ptr, &eboard);

    if (status == FBE_RAID_FRU_ERROR_STATUS_SUCCESS)
    {
        /* Write successful, send good status.
         * First need to release the write_log slot.
         */
        fbe_raid_siots_transition(siots_p, fbe_parity_mr3_state26);
    }
    else if (status == FBE_RAID_FRU_ERROR_STATUS_SHUTDOWN)
    {
        /* Multiple errors, we are going shutdown.
         * Release the slot and return bad status.
         */
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_shutdown_error);
        write_log_slot_status = fbe_parity_write_log_release_slot(siots_p, __FUNCTION__, __LINE__);
    }
    else if (status == FBE_RAID_FRU_ERROR_STATUS_DEAD)
    {
        /* Monitor-initiated operation hit a dead error -- this will be abandoned, need to invalidate disk slot.
         */
        fbe_parity_write_log_set_slot_invalidate_state(siots_p, FBE_PARITY_WRITE_LOG_SLOT_INVALIDATE_STATE_DEAD,
                                                       __FUNCTION__, __LINE__);
        fbe_raid_siots_transition(siots_p, fbe_parity_mr3_state26);
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
        fbe_raid_fruts_set_degraded_nop(siots_p, write_fruts_ptr);

        /* Send out retries.  We will stop retrying if we get aborted or 
         * our timeout expires or if an edge goes away. 
         */
        retry_status = fbe_raid_siots_retry_fruts_chain(siots_p, &eboard, write_fruts_ptr);
        if (retry_status != FBE_STATUS_OK)
        { 
            fbe_raid_siots_set_unexpected_error(siots_p,
                                                FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "parity: retry %d errors (bitmap 0x%x) status: %d unexpected\n", 
                                                eboard.retry_err_count, eboard.retry_err_bitmap, retry_status);
            fbe_raid_fru_eboard_display(&eboard, siots_p, FBE_TRACE_LEVEL_ERROR);
            write_log_slot_status = fbe_parity_write_log_release_slot(siots_p, __FUNCTION__, __LINE__);
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
            write_log_slot_status = fbe_parity_write_log_release_slot(siots_p, __FUNCTION__, __LINE__);
            return FBE_RAID_STATE_STATUS_EXECUTING; 
        }
        else if ((eboard.dead_err_count <= fbe_raid_siots_parity_num_positions(siots_p)) &&
                 (eboard.dead_err_count > 0))
        {
            /* For RAID5/RAID3, if one error then return good status.
             * For RAID 6, if one or two error(s) return good status.
             * First need to release the write_log slot.
             */
            fbe_raid_siots_transition(siots_p, fbe_parity_mr3_state26);
        }
        else if (eboard.drop_err_count > 0)
        {
            /* DH Dropped the request,
             * we should not have submitted writes as optional.
             */
            fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                  "parity: %s fru dropped error unexpected status: 0x%x "
                                  "drop_cnt: 0x%x drop_bitmap: 0x%x\n", 
                                  __FUNCTION__, status, eboard.drop_err_count, eboard.drop_err_bitmap);
            write_log_slot_status = fbe_parity_write_log_release_slot(siots_p, __FUNCTION__, __LINE__);
        }
        else
        {
            /* Uknown error
             */
            fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "parity: %s fru error status unknown 0x%x\n", 
                                                __FUNCTION__, status);
            write_log_slot_status = fbe_parity_write_log_release_slot(siots_p, __FUNCTION__, __LINE__);
        }
    }/*end else if (status == FBE_RAID_FRU_ERROR_STATUS_ERROR)*/

    else
    {
        /* This case should have been handled above.
         */
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity: %s fru status unknown 0x%x\n", 
                                            __FUNCTION__, status);
        write_log_slot_status = fbe_parity_write_log_release_slot(siots_p, __FUNCTION__, __LINE__);
    }

    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/*******************************
 * end of fbe_parity_mr3_state14()
 *******************************/

/****************************************************************
 * fbe_parity_mr3_state15()
 ****************************************************************
 * @brief
 *  Unit went degraded during a pre-read, shift to degraded write
 *   recovery path.
 *  on RAID5/RAID3 or if both parity drives are dead on RAID6.
 *  Shift to degraded verify if NOT both parity drives are dead
 *  on RAID6.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t 
 *
 * @return
 *   This function always returns FBE_RAID_STATE_STATUS_EXECUTING
 *
 * @author
 *   7/7/99 - Created. Rob Foley
 *
 ****************************************************************/
fbe_raid_state_status_t fbe_parity_mr3_state15(fbe_raid_siots_t * siots_p)
{
    fbe_status_t fbe_status = FBE_STATUS_OK;

    /* We're just kicking off the degraded verify, so
     * we only have one thing to wait for.
     */
    siots_p->wait_count = 1;

    /* Since we are starting a degraded verify, we will
     * set our algorithm to R5_DEG_MR3, to help note that we have
     * gone to verify to fix any pre-read errors.  This helps us know
     * later that we do not need to transition to verify on any
     * checksum errors.
     */
    siots_p->algorithm = R5_DEG_MR3;
    /* When we get back from verify,
     * continue with write state machine.
     */
    fbe_raid_siots_transition(siots_p, fbe_parity_mr3_state22);

    /* Setup for degraded verify and transite to r5_deg_vr_state0.
     */
    fbe_status = fbe_raid_siots_start_nested_siots(siots_p, fbe_parity_degraded_verify_state0);
    if (RAID_GENERATE_COND(fbe_status != FBE_STATUS_OK))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity: %s failed to start nested siots 0x%p, status = 0x%x\n",
                                            __FUNCTION__,
                                            siots_p, 
                                            fbe_status);
        return (FBE_RAID_STATE_STATUS_EXECUTING); 
    }

    return FBE_RAID_STATE_STATUS_WAITING;
}
/*******************************
 * end of fbe_parity_mr3_state15()
 *******************************/

/****************************************************************
 * fbe_parity_mr3_state16()
 ****************************************************************
 * @brief
 *   We encountered a CSUM/XOR error, shift to verify recovery.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t
 *
 * @return
 *   fbe_raid_state_status_t
 *
 * @author
 *   7/7/99  - Created. Rob Foley
 *   10/5/99 - Added verify support. JJ
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_parity_mr3_state16(fbe_raid_siots_t * siots_p)
{
    fbe_status_t fbe_status = FBE_STATUS_OK;
    fbe_raid_state_status_t state_status;
    fbe_bool_t b_generate_next_siots = FBE_TRUE;
    fbe_raid_iots_t *iots_p = fbe_raid_siots_get_iots(siots_p);

    /* When we get back from verify,
     * continue with write state machine.
     */
    fbe_raid_siots_transition(siots_p, fbe_parity_mr3_state19);
    state_status = FBE_RAID_STATE_STATUS_WAITING;

    /* Determine if we need to generate another siots.
     * If this returns TRUE then we must always call fbe_raid_iots_generate_next_siots_immediate(). 
     * Typically this call is done after we have started the disk I/Os.
     */
    fbe_status = fbe_raid_siots_should_generate_next_siots(siots_p, &b_generate_next_siots);
    if (RAID_GENERATE_COND(fbe_status != FBE_STATUS_OK))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS, 
                                            "parity: %s failed in next siots generation: siots_p = 0x%p, status = %dn",
                                            __FUNCTION__, siots_p, fbe_status);
        return(FBE_RAID_STATE_STATUS_EXECUTING);
    }

    if (!fbe_parity_siots_is_degraded(siots_p))
    {
        /* Setup for verify.
         */
        fbe_status = fbe_raid_siots_start_nested_siots(siots_p, fbe_parity_recovery_verify_state0);
        if (RAID_GENERATE_COND(fbe_status != FBE_STATUS_OK))
        {
            fbe_raid_siots_set_unexpected_error(siots_p, 
                                                FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "parity: %s failed to start nested siots  0x%p, status = 0x%x\n",
                                                __FUNCTION__,
                                                siots_p,
                                                fbe_status);
            state_status = FBE_RAID_STATE_STATUS_EXECUTING;
        }
    }
    else
    {
        /* Since we are degraded do a degraded recovery verify.
         */
        siots_p->wait_count = 1;
        siots_p->algorithm = R5_DEG_MR3;
        fbe_status = fbe_raid_siots_start_nested_siots(siots_p, fbe_parity_degraded_recovery_verify_state0);

        if (RAID_GENERATE_COND(fbe_status != FBE_STATUS_OK))
        {
            fbe_raid_siots_set_unexpected_error(siots_p, 
                                                FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "parity: %s failed to start nested siots = 0x%p, status = 0x%x\n",
                                                __FUNCTION__,
                                                siots_p,
                                                fbe_status);
            state_status = FBE_RAID_STATE_STATUS_EXECUTING;
        }
    }
    /* If a generate is needed, kick it off now.
     */
    if (b_generate_next_siots)
    {
        fbe_raid_iots_generate_next_siots_immediate(iots_p);
    }
    return state_status;
}
/*******************************
 * end of fbe_parity_mr3_state16()
 *******************************/

/****************************************************************
 * fbe_parity_mr3_state18()
 ****************************************************************
 * @brief
 *   The writes were successful, return good status.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t              
 *
 * @return
 *  FBE_RAID_STATE_STATUS_WAITING or FBE_RAID_STATE_STATUS_EXECUTING
 *
 * @author
 *   7/7/99 - Created. Rob Foley
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_parity_mr3_state18(fbe_raid_siots_t * siots_p)
{
    fbe_raid_siots_transition(siots_p, fbe_raid_siots_success);
    fbe_parity_report_errors(siots_p, FBE_TRUE);

    /* After reporting errors, just send Usurper FBE_BLOCK_TRANSPORT_CONTROL_CODE_UPDATE_LOGICAL_ERROR_STATS
     * to PDO for single or multibit CRC errors found
     */
   if (fbe_raid_siots_pdo_usurper_needed(siots_p))
   {
       fbe_status_t send_crc_status = fbe_raid_siots_send_crc_usurper(siots_p);
       /* We are waiting only when usurper has been sent, else executing should be returned */   
       if (send_crc_status == FBE_STATUS_OK)
       {
           return FBE_RAID_STATE_STATUS_WAITING;
       }
   }

   return FBE_RAID_STATE_STATUS_EXECUTING;
}
/*******************************
 * end of fbe_parity_mr3_state18()
 *******************************/

/****************************************************************
 *  fbe_parity_mr3_state19()
 ****************************************************************
 * @brief
 *   We encountered hard errors or CSUM errors, shift to verify recovery.
 *   We come here after we finished the verify to continue write operations.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t           
 *
 * @return
 *  FBE_RAID_STATE_STATUS_EXECUTING
 *
 * @author
 *   10/5/99 - Created. JJ
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_parity_mr3_state19(fbe_raid_siots_t * siots_p)
{
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
         * continue with write.
         */
        fbe_raid_siots_transition(siots_p, fbe_parity_mr3_state18);
    }
    else if (siots_p->error == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_NOT_READY)
    {
        /* Try degraded write or degraded verify.
         */
        fbe_raid_siots_transition(siots_p, fbe_parity_mr3_state15);
    }
    else if ((siots_p->error == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED) &&
             (siots_p->qualifier == FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_CRC_ERROR))
    {
        /* A hard error status from verify means there
         * is a shutdown condition.
         */
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_write_crc_error);
    }
    else if (siots_p->error == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED)
    {
        /* A hard error status from verify means there
         * is a shutdown condition.
         */
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_shutdown_error);
    }
    else
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity_468_state19 siots stat unexpect.siots->err=0x%x,siots=0x%p\n", 
                                            siots_p->error,
                                            siots_p);  
    }
    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/*******************************
 * end of fbe_parity_mr3_state19()
 *******************************/

/****************************************************************
 *  fbe_parity_mr3_state22()
 ****************************************************************
 * @brief
 *   We encountered dead error(s) and come here after degraded verify,
 *   then continue write operations. This function is for RAID 6 only.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t 
 *
 * @return
 *   fbe_raid_state_status_t
 *
 * @author
 *   03/04/06 - Created. CLC
 *   07/03/08.-.Modified for Rebuild Logging Support For R6. Rob Foley
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_parity_mr3_state22(fbe_raid_siots_t * siots_p)
{
    fbe_status_t fbe_status = FBE_STATUS_OK;
    fbe_raid_state_status_t status = FBE_RAID_STATE_STATUS_EXECUTING;

    /* Return from degraded verify.
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
        if (siots_p->qualifier == FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_COMPLETE_WITH_REMAP)
        {
            /* Verify wrote out the corrections and the new data.  We are done.
             */
            fbe_raid_siots_transition(siots_p, fbe_parity_mr3_state26);
        }
        else
        {
            /* The data has been verified,
             * continue with write.
             */
            fbe_raid_siots_transition(siots_p, fbe_parity_mr3_state7);
        }
    }
    else if (siots_p->error == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED)
    {
        /* There is a shutdown condition, go shutdown error.
         */
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_shutdown_error);
    }
    else if (siots_p->error == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR)
    {
        /* Simply re-issue the degraded verify as a recovery verify.
         */
        if (siots_p->wait_count != 0)
        {
            fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "parity: %s : wait_count (%lld) unexpected: siots_p = 0x%p\n",
                                                __FUNCTION__,
                                                siots_p->wait_count,
                                                siots_p);
            return FBE_RAID_STATE_STATUS_EXECUTING;
        }
        siots_p->wait_count = 1;
        fbe_status = fbe_raid_siots_start_nested_siots(siots_p, fbe_parity_degraded_recovery_verify_state0);
        if (RAID_GENERATE_COND(fbe_status != FBE_STATUS_OK))
        {
            fbe_raid_siots_set_unexpected_error(siots_p, 
                                                FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "parity: %s failed to start nested siots  0x%p, status = 0x%x\n",
                                                __FUNCTION__,
                                                siots_p,
                                                fbe_status);
            return FBE_RAID_STATE_STATUS_EXECUTING; 
        }
        status = FBE_RAID_STATE_STATUS_WAITING;
    }
    else
    {
        /* We do not expect this error.  Display the error and return bad status.
         */
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity: %s unexpected error for siots_p = 0x%p, "
                                            "siots_p->error = 0x%x, lba: 0x%llx\n", 
                                            __FUNCTION__,
                                            siots_p,
                                            siots_p->error, 
                                            siots_p->start_lba);
        return FBE_RAID_STATE_STATUS_EXECUTING; 
    }

    return status;
}
/**************************
 * end of fbe_parity_mr3_state22
 **************************/

/******************************************************************************
 * fbe_parity_mr3_state26()
 ******************************************************************************
 * @brief
 *  Write of user data to live stripe is completed. 
 *  Invalidate the slot header to indicate that write_log slot no longer
 *  has any valid user data and the slot is free. 
 *
 * @param siots_p - ptr to the fbe_raid_siots_t 
 *
 * @return
 *      FBE_RAID_STATE_STATUS_EXECUTING
 *
 * @author
 *  3/06/2012 - Created. Vamsi V
 *  13mar12   - Copied from flush machine to write log machine
 *
 *****************************************************************************/
fbe_raid_state_status_t fbe_parity_mr3_state26(fbe_raid_siots_t * siots_p)
{
    fbe_status_t status;
    fbe_raid_fruts_t *write_fruts_p = NULL;
    fbe_bool_t b_result = FBE_TRUE;
    fbe_u32_t write_log_slot_id;
    fbe_status_t write_log_slot_status = FBE_STATUS_OK;

    /* Retrieve the write log slot for this request.
     */
    fbe_raid_siots_get_journal_slot_id(siots_p, &write_log_slot_id);

    if (write_log_slot_id != FBE_RAID_INVALID_JOURNAL_SLOT)
    {
        fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_p);

        /* Re-initalize write fruts to do slot header invalidation */
        status = fbe_parity_write_log_generate_header_invalidate_fruts(siots_p);

        if (RAID_COND(status != FBE_STATUS_OK))
        {
            fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "write_log invalidate: setup of header_invalidation portion of write_log invalidation operation failed, status: 0x%x\n",
                                                status);
            status = fbe_parity_write_log_release_slot(siots_p, __FUNCTION__, __LINE__);
            return FBE_RAID_STATE_STATUS_EXECUTING;
        }

        /* Initialize the number of fruts we will be waiting for completion on */
        if ((siots_p->wait_count = fbe_raid_fruts_count_active(siots_p, write_fruts_p)) == 0)
        {
            fbe_raid_siots_trace(siots_p,
                                 FBE_RAID_SIOTS_TRACE_PARAMS_WARNING,
                                 "write_log invalidate: Needed to invalidate %d drives but could not due to errors.\n",
                                 siots_p->drive_operations);

            fbe_raid_siots_transition(siots_p, fbe_parity_mr3_state18);

            /* Abandon the flush and release the slot (this is a synchronous call) */
            status = fbe_parity_write_log_release_slot(siots_p, __FUNCTION__, __LINE__);
            return FBE_RAID_STATE_STATUS_EXECUTING;
        }

        /* set the next state */
        fbe_raid_siots_transition(siots_p, fbe_parity_mr3_state27);

        /* Initiate the flush */
        b_result = fbe_raid_fruts_send_chain(&siots_p->write_fruts_head, fbe_raid_fruts_evaluate);
        if (RAID_FRUTS_COND(b_result != FBE_TRUE))
        {
            /* We hit an issue while starting fruts chain
             */
            fbe_raid_siots_set_unexpected_error(siots_p, 
                                                FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS, 
                                                "parity: 468 %s fail to start write fruts chain for "
                                                "siots 0x%p, b_result=%d\n", 
                                                __FUNCTION__,
                                                siots_p,
                                                b_result);
            write_log_slot_status = fbe_parity_write_log_release_slot(siots_p, __FUNCTION__, __LINE__);
            return FBE_RAID_STATE_STATUS_EXECUTING;
        }

        return FBE_RAID_STATE_STATUS_WAITING;
    }
    else
    {
         /* Else transition to the completion state.
         */
        fbe_raid_siots_transition(siots_p, fbe_parity_mr3_state18);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }
}
/*******************************************************************************
 * end fbe_parity_mr3_state26()
 ******************************************************************************/

/******************************************************************************
 * fbe_parity_mr3_state27()
 ******************************************************************************
 * @brief
 *  Check the status of the release slot.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t 
 *
 * @return
 *      FBE_RAID_STATE_STATUS_EXECUTING
 *
 * @author
 *  2/24/2011 - Created. Daniel Cummins
 *  13mar12   - Copied from state14 and modified
 *
 *****************************************************************************/
fbe_raid_state_status_t fbe_parity_mr3_state27(fbe_raid_siots_t * siots_p)
{
    fbe_raid_fru_eboard_t eboard;
    fbe_raid_fru_error_status_t status;
    fbe_raid_fruts_t *write_fruts_ptr = NULL;
    fbe_status_t write_log_slot_status = FBE_STATUS_OK;

    fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_ptr);
    fbe_raid_fru_eboard_init(&eboard);

    /* Get totals for errored or dead drives.
     */
    status = fbe_parity_get_fruts_error(write_fruts_ptr, &eboard);

    if (status == FBE_RAID_FRU_ERROR_STATUS_SUCCESS)
    {
        fbe_parity_write_log_slot_invalidate_state_t invalidate_state;

        fbe_parity_write_log_get_slot_invalidate_state(siots_p,
                                                       &invalidate_state,
                                                       __FUNCTION__, __LINE__);

        if (invalidate_state == FBE_PARITY_WRITE_LOG_SLOT_INVALIDATE_STATE_SUCCESS)
        {
            /* Success on slot header invalidation - now release the slot */
            fbe_raid_siots_transition(siots_p, fbe_parity_mr3_state18);
        }
        else if (invalidate_state == FBE_PARITY_WRITE_LOG_SLOT_INVALIDATE_STATE_DEAD)
        {
            /* This was dead and we waited to invalidate the slot */
            fbe_raid_siots_transition(siots_p, fbe_raid_siots_dead_error);
        }
        write_log_slot_status = fbe_parity_write_log_release_slot(siots_p, __FUNCTION__, __LINE__);
    }
    else if (status == FBE_RAID_FRU_ERROR_STATUS_SHUTDOWN)
    {
        /* Multiple errors, we are going shutdown.
         * Release the slot and return bad status.
         */
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_shutdown_error);
        write_log_slot_status = fbe_parity_write_log_release_slot(siots_p, __FUNCTION__, __LINE__);
    }
    else if (status == FBE_RAID_FRU_ERROR_STATUS_DEAD)
    {
        /* Monitor-initiated operation hit a dead error.
         */
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_dead_error);
        write_log_slot_status = fbe_parity_write_log_release_slot(siots_p, __FUNCTION__, __LINE__); 
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
        fbe_raid_fruts_set_degraded_nop(siots_p, write_fruts_ptr);

        /* Send out retries.  We will stop retrying if we get aborted or 
         * our timeout expires or if an edge goes away. 
         */
        retry_status = fbe_raid_siots_retry_fruts_chain(siots_p, &eboard, write_fruts_ptr);
        if (retry_status != FBE_STATUS_OK)
        { 
            fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "parity: retry %d errors (bitmap 0x%x) status: %d unexpected\n", 
                                                eboard.retry_err_count, eboard.retry_err_bitmap, retry_status);
            fbe_raid_fru_eboard_display(&eboard, siots_p, FBE_TRACE_LEVEL_ERROR);
            write_log_slot_status = fbe_parity_write_log_release_slot(siots_p, __FUNCTION__, __LINE__);
            return FBE_RAID_STATE_STATUS_EXECUTING; 
        }
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
        }
        else if (   (eboard.dead_err_count <= fbe_raid_siots_parity_num_positions(siots_p))
                 && (eboard.dead_err_count > 0))
        {
            /* For RAID5/RAID3, if one error then return good status.
             * For RAID 6, if one or two error(s) return good status.
             * First need to release the write_log slot.
             */
            fbe_raid_siots_transition(siots_p, fbe_parity_mr3_state18);
        }
        else if (eboard.drop_err_count > 0)
        {
            /* DH Dropped the request,
             * we should not have submitted writes as optional.
             */
            fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "parity: %s fru dropped error unexpected status: 0x%x "
                                                "drop_cnt: 0x%x drop_bitmap: 0x%x\n", 
                                                __FUNCTION__, status, eboard.drop_err_count, eboard.drop_err_bitmap);
        }
        else
        {
            /* Uknown error
             */
            fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "parity: %s fru error status unknown 0x%x\n", 
                                                __FUNCTION__, status);
        }

        /* We completed write to live stripe. There is no need to hold on to the slot even if we get any errors.
         */
        write_log_slot_status = fbe_parity_write_log_release_slot(siots_p, __FUNCTION__, __LINE__);

    } /*end else if (status == FBE_RAID_FRU_ERROR_STATUS_ERROR)*/

    else if (status == FBE_RAID_FRU_ERROR_STATUS_WAITING)
    {
        /* One drive is dead, we need to mark this request
         * as waiting for a shutdown or continue.
         */
        return FBE_RAID_STATE_STATUS_WAITING;
    }
    else
    {
        /* This case should have been handled above.
         */
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity: %s fru status unknown 0x%x\n", 
                              __FUNCTION__, status);
        write_log_slot_status = fbe_parity_write_log_release_slot(siots_p, __FUNCTION__, __LINE__);
    }

    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/*******************************************************************************
 * end fbe_parity_mr3_state27()
 ******************************************************************************/

/*************************************************************
 * end of file fbe_parity_mr3.c
 *************************************************************/
