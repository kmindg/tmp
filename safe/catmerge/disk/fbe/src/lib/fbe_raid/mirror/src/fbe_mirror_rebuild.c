/***************************************************************************
 * Copyright (C) EMC Corporation 1999-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!***************************************************************************
 * @file    fbe_mirror_rebuild.c
 *****************************************************************************
 *
 * @brief   The file contains the state machine to handle a mirror rebuild.
 *
 *          The following algorithms are supported by the rebuild state machine:
 *
 *          o MIRROR_RB - Read from one mirror position and update all the 
 *                         non-dead positions (i.e. write only to all other
 *                         non-dead postions).
 *
 *          o MIRROR_EQZ - Functions the same as a normal rebuild.  Separate 
 *                         algorithm used to help debug sparing issues.
 *
 *          o MIRROR_PACO - Functions the same as a normal rebuild.  Separate
 *                          algorithm used to help debug sparing issues. A 
 *                          proactive copy from the proactive candidate to
 *                          the proactive spare uses the mirror rebuild state
 *                          machine.
 *
 * @note    Example of 3-way mirror where position 1 was removed and replaced then
 *          position 2 was removed and re-inserted. (NR - Needs Rebuild, s - siots
 *          range).  Note that all 'Needs Rebuild' ranges are in `chunk' size 
 *          multiples where the chunk size is 2048 (0x800) blocks.
 *
 *  pba             Position 0           Position 1           Position 2  SIOTS
 *  0x11200         +--------+           +--------+           +--------+  -----
 *                  |        |           |        |           |        |
 *  0x11a00  siots  +--------+           +--------+           +--------+
 *         0x11e00->|ssssssss|           |        |           |        |  <= 1
 *  0x12200         +--------+           +--------+ First RL->+========+
 *                  |ssssssss|           |        |           |NR NR NR|  <= 2
 *  0x12a00         +--------+ Degraded->+--------+           +========+
 *                  |ssssssss|           |NR NR NR|           |        |  <= 3
 *  0x13200         +--------+           +--------+Second RL->+========+
 *                  |ssssssss|           |NR NR NR|           |NR NR NR|  <= 4
 *  0x13a00         +--------+           +--------+           +========+
 *                  |ssssssss|           |NR NR NR|           |        |
 *  0x14200         +--------+           +--------+           +--------+
 *         0x14600->|ssssssss|           |NR NR NR|           |        |
 *  0x14a00         +--------+           +--------+           +--------+
 *                  |        |           |NR NR NR|           |        |
 *
 *          For this example the original siots request for 0x11e00 thru
 *          0x145ff will be broken into the following siots:    
 *          SIOTS   Primary (i.e. Read)Positions    Degraded Positions that Need Rebuild
 *          -----   ---------------------------     ------------------------------------
 *          1.      Position 0 and 1                None @note Is this really a failure?
 *          2.      Position 0 and 1                Position 2
 *          3.      Position 0 and 2                Position 1
 *          4.      Position 0                      Position 0 and 1 
 *          Breaking up the I/O is required by the current xor algorithms
 *          which do not handle a degraded change within a request.
 *
 * @author
 *   12/16/2009 Ron Proulx  - Created from mirror_vr.c circa 08/21/2000 Created Ruomu Gao.
 *
 ***************************************************************************/


/*************************
 ** INCLUDE FILES
 *************************/
#include "fbe_raid_library_proto.h"
#include "fbe_mirror_io_private.h"

extern fbe_raid_common_error_precedence_t fbe_raid_siots_get_error_precedence(fbe_payload_block_operation_status_t error,
                                                                              fbe_payload_block_operation_qualifier_t qualifier);

/*!***************************************************************************
 *          fbe_mirror_rebuild_state0()
 *****************************************************************************
 *
 * @brief   This method determines and allocates the neccessary structures for
 *          a mirror rebuild.  Rebuild is similar to verify in that buffers
 *          and ags are allocated for all positions (even disabled positions).
 *          This requirement comes from the XOR rebuild code.  All non-degraded
 *          positions go on the read fruts chain and all degraded (i.e. ` Needs
 *          Rebuild') positions go on the write chain.
 *          The default state is FBE_RAID_STATE_STATUS_EXECUTING since the
 *          majority of the time the resources will be immediately available.
 *
 * @param   siots_p - ptr to the fbe_raid_siots_t
 *
 * @return  This function returns FBE_RAID_STATE_STATUS_WAITING if the resources
 *          aren't immediately available.
 *
 * @note    We always allocate a fruts for every position in the raid group.
 *          This includes degraded or disabled positions.
 *
 * @author
 *      08/16/00  RG - Created.
 *      03/20/01  BJP - Added N-way mirror support
 *  12/11/2009  Ron Proulx - Re-written to add validation checks and use single
 *                           allocation scheme.
 *
 *****************************************************************************/
fbe_raid_state_status_t fbe_mirror_rebuild_state0(fbe_raid_siots_t * siots_p)
{
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_raid_state_status_t state_status = FBE_RAID_STATE_STATUS_EXECUTING;
    fbe_raid_fru_info_t read_info[FBE_MIRROR_MAX_WIDTH];
    fbe_raid_fru_info_t write_info[FBE_MIRROR_MAX_WIDTH];

    /* Validate that this is properly configured mirror verify request.
     */
    if ( (status = fbe_mirror_rebuild_validate(siots_p)) != FBE_STATUS_OK )
    {
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "mirror: rebuild validate failed. status: 0x%0x\n",
                                            status);
        return(state_status);
    }

    /* Calculate and allocate all the memory needed for this siots.
     */
    if ((status = fbe_mirror_rebuild_calculate_memory_size(siots_p, 
                                                           &read_info[0], 
                                                           &write_info[0])) 
                                                                        != FBE_STATUS_OK )
    {
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "mirror: rebuild calculate memory size failed. status: 0x%x\n",
                                            status);
        return(state_status);
    }

    /* Now allocate all the pages required for this request.  Note that must 
     * transition to `deferred allocation complete' state since the callback 
     * can be invoked at any time. 
     * There are (3) possible status returned by the allocate method: 
     *  o FBE_STATUS_OK - Allocation completed immediately (skip deferred state)
     *  o FBE_STATUS_PENDING - Allocation didn't complete immediately
     *  o Other - Memory allocation error
     */
    fbe_raid_siots_transition(siots_p, fbe_mirror_rebuild_state1);
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
        status = fbe_mirror_rebuild_setup_resources(siots_p, 
                                                    &read_info[0],
                                                    &write_info[0]);
        if (RAID_COND(status != FBE_STATUS_OK))
        {
            fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "mirror: rebuild failed to setup resources. status: 0x%x\n",
                                            status);
            return(state_status);
        }

        /* To prevent confusion (i.e. we never went to `deferred allocation complete'
         * state) overwrite the current state.
         */
        fbe_raid_common_set_state(&siots_p->common, (fbe_raid_common_state_t)fbe_mirror_rebuild_state3);
    }
    else if (RAID_MEMORY_COND(status == FBE_STATUS_PENDING))
    {
        /* Else return waiting
         */
        return(FBE_RAID_STATE_STATUS_WAITING);
    }
    else
    {
        /* Change the state to unexpected error and return
         */
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "mirror: rebuild memory request failed with status: 0x%x\n",
                                            status);
        return(state_status);
    }

    /* Always return the executing status.
     */
    return(state_status);
}
/* end of fbe_mirror_rebuild_state0() */

/*!***************************************************************************
 *          fbe_mirror_rebuild_state1()
 *****************************************************************************
 *
 * @brief   This state is the callback for the memory allocation completion.
 *          We validate and plant the requested resources then transition to
 *          the issue state.  The only state status returned is
 *          FBE_RAID_STATE_STATUS_EXECUTING.
 *
 * @param   siots_p, [IO] - ptr to the fbe_raid_siots_t
 *
 * @author
 *  10/14/2010  Ron Proulx  - Created
 *
 *****************************************************************************/
fbe_raid_state_status_t fbe_mirror_rebuild_state1(fbe_raid_siots_t *siots_p)
{
    fbe_raid_state_status_t state_status = FBE_RAID_STATE_STATUS_EXECUTING;
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_raid_fru_info_t     read_info[FBE_MIRROR_MAX_WIDTH];
    fbe_raid_fru_info_t     write_info[FBE_MIRROR_MAX_WIDTH];
    fbe_u16_t               num_sgs[FBE_RAID_MAX_SG_TYPE];

    /* Need to clear num sgs array
     */
    fbe_zero_memory(&num_sgs[0], (sizeof(fbe_u16_t) * FBE_RAID_MAX_SG_TYPE));

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
                             "mirror: rebuild memory allocation error common flags: 0x%x \n",
                             fbe_raid_common_get_flags(&siots_p->common));
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_memory_allocation_error);
        return(state_status);
    }

    /* Now calculate the fruts information for each position including the
     * sg index if required.
     */
    status = fbe_mirror_rebuild_get_fru_info(siots_p, 
                                             &read_info[0],
                                             &write_info[0],
                                             &num_sgs[0],
                                             FBE_TRUE /* Log a failure */);
    if (RAID_COND(status != FBE_STATUS_OK))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "mirror: rebuild get fru info failed with status: 0x%x\n",
                                            status);
        return(state_status);
    }

    /* Now plant the resources
     */
    status = fbe_mirror_rebuild_setup_resources(siots_p, 
                                                &read_info[0],
                                                &write_info[0]);
    if (RAID_COND(status != FBE_STATUS_OK))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "mirror: rebuild failed to setup resources. status: 0x%x\n",
                                            status);
        return(state_status);
    }

    /* Else the resources were setup properly, so transition to
     * the next read state.
     */
    fbe_raid_siots_transition(siots_p, fbe_mirror_rebuild_state3);

    /* Always return the executing status.
     */
    return(state_status);
}   
/* end of fbe_mirror_rebuild_state1() */

/*!***************************************************************
 *          fbe_mirror_rebuild_state3()
 *****************************************************************
 *
 * @brief   This state issues one or more reads to the non-degraded
 *          positions (relative to this siots).  Before issuing any
 *          request (which this state does) we need to check that 
 *          the request hasn't been aborted.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t
 *
 * @return VALUE:
 *      This function returns FBE_RAID_STATE_STATUS_WAITING
 *
 * @author
 *      08/16/00  RG - Created.
 *
 ****************************************************************/
fbe_raid_state_status_t fbe_mirror_rebuild_state3(fbe_raid_siots_t * siots_p)
{
    fbe_raid_state_status_t state_status = FBE_RAID_STATE_STATUS_WAITING;
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_bool_t b_result = FBE_TRUE;
    fbe_bool_t b_generate_next_siots = FBE_FALSE;
    fbe_raid_iots_t *iots_p = fbe_raid_siots_get_iots(siots_p);
    fbe_raid_fruts_t *read_fruts_p = NULL;

    fbe_raid_siots_check_traffic_trace(siots_p, FBE_TRUE);

    /* If this request has already been aborted transition to the abort 
     * handling.
     */
    if (fbe_raid_siots_is_aborted(siots_p))
    {
        /* The iots has been aborted, treat it as a hard error,
         * and bail out now.
         */
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_aborted);
        return(FBE_RAID_STATE_STATUS_EXECUTING);
    }

    /* If we are quiescing, then this function will mark the siots as quiesced 
     * and we will wait until we are woken up.
     */
    if (fbe_raid_siots_mark_quiesced(siots_p))
    {
        return(FBE_RAID_STATE_STATUS_WAITING);
    }
    /* We need to re-evaluate our degraded positions before sending the request
     * since the raid group monitor may have updated our degraded status. 
     */  
    status = fbe_mirror_handle_degraded_positions(siots_p,
                                                  FBE_FALSE, /* This is a read request */
                                                  FBE_TRUE   /* Write degraded positions */);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status)) 
    {
        /* In-sufficient non-degraded positions to continue.  This is unexpected
         */
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS, 
                                            "mirror: siots_p: 0x%p refresh degraded unexpected status: 0x%x\n", 
                                            siots_p, status);
        return(FBE_RAID_STATE_STATUS_EXECUTING);
    }

    /* For those rebuild request where we have completed generating AND
     * they are NOT nested (i.e. we don't pipeline for recovery verify
     * types) AND we are not in region mode, start the siots.
     */
    if (!fbe_raid_siots_is_flag_set(siots_p, FBE_RAID_SIOTS_FLAG_SINGLE_REGION_MODE))
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

    /* The wait count is simply the number of items in the read chain
     */
    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_p);
    siots_p->wait_count = fbe_raid_fruts_count_active(siots_p, read_fruts_p);

    /* Always transitions BEFORE send the I/Os.  Now send the read fruts
     * chain.  We change the state status to wairing since we are waiting
     * for the I/Os to complete.
     */
    fbe_raid_siots_transition(siots_p, fbe_mirror_rebuild_state4);
    b_result = fbe_raid_fruts_send_chain(&siots_p->read_fruts_head, fbe_raid_fruts_evaluate);
    if (RAID_FRUTS_COND(b_result != FBE_TRUE))
    {
        /* We hit an issue while starting chain of fruts.
         */
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "mirror: %s failed to start read fruts chain for "
                                            "siots_p 0x%p\n",
                                            __FUNCTION__,
                                            siots_p);
    }

    if (b_generate_next_siots)
    {
        fbe_raid_iots_generate_next_siots_immediate(iots_p);
    }

    /* Always return the state status.
     */
    return(state_status);
}
/* end of fbe_mirror_rebuild_state3() */

/****************************************************************
 *          fbe_mirror_rebuild_state4()
 ****************************************************************
 *
 * @brief   Read has completed, check status.
 *

 * @param siots_p - ptr to the fbe_raid_siots_t
 *
 * @return VALUE:
 *      This function returns FBE_RAID_STATE_STATUS_WAITING if need to wait for dh read or disk dead;
 *      FBE_RAID_STATE_STATUS_EXECUTING otherwise.
 *
 * @author
 *      08/16/00  RG - Created.
 *
 ****************************************************************/
fbe_raid_state_status_t fbe_mirror_rebuild_state4(fbe_raid_siots_t * siots_p)
{
    fbe_raid_state_status_t     state_status = FBE_RAID_STATE_STATUS_EXECUTING;
    fbe_raid_fru_eboard_t       fru_eboard;
    fbe_raid_fru_error_status_t fru_status;
    fbe_raid_fruts_t           *read_fruts_p = NULL;
    
    /* Since we haven't modified the media we need to check for aborted.
     * Get fruts error will check if this siots have been aborted.
     */

    /* Initialize out local eboard.
     */
    fbe_raid_fru_eboard_init(&fru_eboard);
    
    /* Validate that all fruts have completed.
     */
    if (siots_p->wait_count != 0)
    {
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS, 
                                            "mirror: wait count unexpected 0x%llx \n", 
                                            siots_p->wait_count);
        return(state_status);
    }

    /* Reads have completed. Evaluate the status of the reads.
     */
    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_p);
    fru_status = fbe_mirror_get_fruts_error(read_fruts_p, &fru_eboard);

    /* Based on the fruts status we transition to the proper state.
     */
    if (fru_status == FBE_RAID_FRU_ERROR_STATUS_SUCCESS)
    {
        /* Success. Continue to next state where will evaluate the
         * read data.
         */
        fbe_raid_siots_transition(siots_p, fbe_mirror_rebuild_state5);
    }
    else if ((fru_status == FBE_RAID_FRU_ERROR_STATUS_SHUTDOWN) ||
             (fru_status == FBE_RAID_FRU_ERROR_STATUS_DEAD) ||
             (fru_status == FBE_RAID_FRU_ERROR_STATUS_RETRY) ||
             (fru_status == FBE_RAID_FRU_ERROR_STATUS_WAITING) ||
             (fru_status == FBE_RAID_FRU_ERROR_STATUS_ABORTED))
    {
        fbe_raid_state_status_t state_status;
        state_status = fbe_mirror_handle_fruts_error(siots_p, read_fruts_p, &fru_eboard, fru_status);
        return state_status;
    }
    else if (fru_status == FBE_RAID_FRU_ERROR_STATUS_INVALIDATE)
    {
        fbe_raid_siots_transition(siots_p, fbe_mirror_rebuild_state10);
    }
    else if (fru_status == FBE_RAID_FRU_ERROR_STATUS_ERROR)
    {
        fbe_raid_status_t   raid_status;

        /*! Invoke the method that will check and process the eboard situation
         *  for a read request.  If the status is success then we can transition
         *  to the next state which will evaluate the read data.
         *
         *  @note We purposefully don't transition in process fruts error so that
         *        all state transitions occur directly from the state machine.
         */
        raid_status = fbe_mirror_read_process_fruts_error(siots_p, &fru_eboard);
        switch(raid_status)
        {
            case FBE_RAID_STATUS_OK:
                /* Ok to continue to next state.
                 */
                fbe_raid_siots_transition(siots_p, fbe_mirror_rebuild_state5);
                break;

            case FBE_RAID_STATUS_OK_TO_CONTINUE:
            case FBE_RAID_STATUS_RETRY_POSSIBLE:
                /* A retry is possible using the read data from a different
                 * position.  Goto state that will setup to continue.
                 */
                fbe_raid_siots_transition(siots_p, fbe_mirror_rebuild_state8);
                break;

            case FBE_RAID_STATUS_MINING_REQUIRED:
                /* Enter or continue region or strip mode.
                 */
                fbe_raid_siots_transition(siots_p, fbe_mirror_rebuild_state21);
                break;
    
            case FBE_RAID_STATUS_TOO_MANY_DEAD:
                fbe_raid_siots_transition(siots_p, fbe_raid_siots_dead_error);
                break;

            case FBE_RAID_STATUS_UNSUPPORTED_CONDITION:
            case FBE_RAID_STATUS_UNEXPECTED_CONDITION:
                /* Some unexpected or unsupported condition occurred.
                 * Transition to unexpected.
                 */
                fbe_raid_siots_transition(siots_p, fbe_raid_siots_unexpected_error);
                break;

            default:
                /* Process fruts error reported an un-supported status.
                 */
                 fbe_raid_siots_set_unexpected_error(siots_p, 
                                                    FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                    "mirror: rebuild unexpected raid_status: 0x%x from read process error\n",
                                                    raid_status);
                break;
        }

        /* Trace some informational text.
         */
        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_INFO,
                                    "mirror: %s: lba: 0x%llx bl: 0x%llx raid_status: 0x%x media err cnt: 0x%x \n",
                                    __FUNCTION__,
			            (unsigned long long)siots_p->parity_start,
			            (unsigned long long)siots_p->parity_count,
                                    raid_status, siots_p->media_error_count);

    } /* END else if (status == FBE_RAID_FRU_ERROR_STATUS_ERROR) */
    else
    {
        /* Else we either got an unexpected fru error status or the fru error
         * status reported isn't supported.
         */
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "mirror: rebuild siots lba: 0x%llx blks: 0x%llx unexpected fru status: 0x%x\n",
                                            (unsigned long long)siots_p->start_lba,
					    (unsigned long long)siots_p->xfer_count,
					    fru_status);
    }

    /* Always return the state status.
     */
    return(state_status);
}                               
/* end of fbe_mirror_verify_state4() */

/*!***************************************************************************
 *          fbe_mirror_rebuild_state5()
 *****************************************************************************
 *
 * @brief   Read(s) have completed successfully. At this point we haven't 
 *          validated the data or metadata.  So we use the XOR rebuild algorithms
 *          to validate (or mark bad) the read data and update the positions to
 *          rebuild will success.  The xor is now done in-line so the default
 *          state_status is FBE_RAID_STATE_STATUS_EXECUTING.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t
 *
 * @return VALUE:
 *      FBE_RAID_STATE_STATUS_EXECUTING.
 *
 * @author
 *      08/16/00  RG - Created.
 *
 *****************************************************************************/
fbe_raid_state_status_t fbe_mirror_rebuild_state5(fbe_raid_siots_t * siots_p)
{
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_raid_state_status_t state_status = FBE_RAID_STATE_STATUS_EXECUTING;
    fbe_raid_fruts_t       *read_fruts_p = NULL;

    /* We should have already checked for aborted.
     */

    /* Ok to check rebuild read data.  Transition to the state that will 
     * evaluate the results from the XOR operation.
     */
    fbe_raid_siots_transition(siots_p, fbe_mirror_rebuild_state6);

    /* Get the bitmask of those read positions that need to be fixed
     * (i.e. written to).
     */
    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_p);
    fbe_raid_fruts_get_media_err_bitmap(read_fruts_p,
                                        &siots_p->misc_ts.cxts.eboard.hard_media_err_bitmap);

    /* Execute a stripwise rebuild. This function will submit any error 
     * messages.
     */
    status = fbe_mirror_rebuild_extent(siots_p);
    if(RAID_COND(status != FBE_STATUS_OK))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "mirror: status %d unexpected\n",
                                            status);
    }

    /* Always return the state status.
     */
    return (state_status);
}                               
/* end of fbe_mirror_rebuild_state5() */

/*!***************************************************************************
 *          fbe_mirror_rebuild_state6()
 *****************************************************************************
 *
 * @brief   XORs have completed. Process the result and issue writes.  The
 *          default state is waiting because the typical case is that we
 *          need to rebuild at least (1) position.
 *
 * @param   siots_p - ptr to the fbe_raid_siots_t
 *
 *  @return
 *    This function returns 
 *     FBE_RAID_STATE_STATUS_WAITING or FBE_RAID_STATE_STATUS_EXECUTING
 *
 *  @author
 *  01/07/2010  Ron Proulx  - Copied from fbe_parity_rebuild_state6
 *
 *****************************************************************************/
fbe_raid_state_status_t fbe_mirror_rebuild_state6(fbe_raid_siots_t * siots_p)
{
    fbe_raid_state_status_t     state_status = FBE_RAID_STATE_STATUS_WAITING;
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_raid_position_bitmask_t needs_write_bitmask;
    fbe_raid_position_bitmask_t hard_media_bitmask;
    fbe_raid_position_bitmask_t uncorrectable_bitmask;
    fbe_raid_position_bitmask_t degraded_bitmask = 0;
    fbe_raid_position_bitmask_t disabled_bitmask = 0;
    fbe_raid_fruts_t           *read_fruts_p = NULL;
    fbe_u32_t                   read_fruts_count = 0;
    fbe_raid_fruts_t           *write_fruts_p = NULL;
    fbe_raid_position_bitmask_t write_fruts_bitmask = 0;
    fbe_u32_t                   write_fruts_count = 0;
    fbe_bool_t b_result = FBE_TRUE;

    /* We don't check for aborted since need to correct any errors
     * that were detected by XOR.
     */

    /* If we are quiescing, then this function will mark the siots as quiesced 
     * and we will wait until we are woken up.
     */
    if (fbe_raid_siots_mark_quiesced(siots_p))
    {
        return(FBE_RAID_STATE_STATUS_WAITING);
    }

    /* We need to re-evaluate our degraded positions before sending the request
     * since the raid group monitor may have updated our degraded status. 
     */  
    status = fbe_mirror_handle_degraded_positions(siots_p,
                                                  FBE_TRUE, /* This is a write request */
                                                  FBE_TRUE  /* Write to degraded positions */);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status)) 
    {
        /* In-sufficient non-degraded positions to continue.  This is unexpected
         */
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS, 
                                            "mirror: siots_p: 0x%p refresh degraded unexpected status: 0x%x\n", 
                                            siots_p, status);
        return(FBE_RAID_STATE_STATUS_EXECUTING);
    }

    /* Get the read and write fruts, bitmasks an counts.
     */
    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_p);
    read_fruts_count = fbe_raid_fruts_count_active(siots_p, read_fruts_p);
    fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_p);
    fbe_raid_siots_get_write_fruts_bitmask(siots_p, &write_fruts_bitmask);
    write_fruts_count = fbe_raid_fruts_count_active(siots_p, write_fruts_p);

    /* Get the bitmasks for the `needs write', `modified', `uncorrectable'
     * bitmasks from the eboard.  The uncorrectable positions are the read
     * positions that resulted in an uncorrectable crc error that now must be
     * written.  XOR has already ORed the `modified' and `uncorrectable' errors
     * into the `needs write' bitmask.
     */
    needs_write_bitmask = siots_p->misc_ts.cxts.eboard.w_bitmap;
    uncorrectable_bitmask = siots_p->misc_ts.cxts.eboard.u_bitmap;
    fbe_raid_siots_get_degraded_bitmask(siots_p, &degraded_bitmask);
    fbe_raid_siots_get_disabled_bitmask(siots_p, &disabled_bitmask);
    fbe_raid_fruts_get_media_err_bitmap(read_fruts_p, &hard_media_bitmask);
    
    /* We have added the degraded positions to the degraded bitmask but due
     * to the way we split the siots it's possible that this siots may not
     * hit a degraded range.  Simply log an informational message.
     */
    if ((needs_write_bitmask & degraded_bitmask) != degraded_bitmask)
    {
        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_INFO,
                                    "mirror: %s: lba: 0x%llx bl: 0x%llx no overlap between needs write: 0x%x and degraded: 0x%x \n",
                                    __FUNCTION__,
			            (unsigned long long)siots_p->start_lba,
			            (unsigned long long)siots_p->xfer_count,
                                    needs_write_bitmask, degraded_bitmask);
    }

    /* Remove any `disabled' (i.e. "really dead" as in missing) positions
     * from the write bitmask.  If any are found log an information message
     * for debug purposes.  The uncorrectable bitmask may also include disabled
     * positions.  This simply means that we were not able to recover the data 
     * and we need to invalidate all positions.
     */
    if ((needs_write_bitmask & disabled_bitmask) != 0)
    {
        /* Trace information message.
         */
        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_DEBUG_HIGH,
                                    "mirror: %s: lba: 0x%llx bl: 0x%llx overlap between needs write: 0x%x and disabled: 0x%x \n",
                                    __FUNCTION__,
			            (unsigned long long)siots_p->start_lba,
			            (unsigned long long)siots_p->xfer_count,
                                    needs_write_bitmask, disabled_bitmask);

        /* Update both the local and siots write bitmask and
         * modified bitmask.
         */
        needs_write_bitmask &= ~disabled_bitmask;
        siots_p->misc_ts.cxts.eboard.w_bitmap = needs_write_bitmask;
        siots_p->misc_ts.cxts.eboard.m_bitmap &= ~disabled_bitmask;
        siots_p->misc_ts.cxts.eboard.u_bitmap &= ~disabled_bitmask; 
    }
    
    /* If we weren't able to correct the data and there were hard media errors
     * re-issue the reads won't help.  If we aren't already in single region
     * mode drop into it and continue.
     */
    if ( (uncorrectable_bitmask != 0)                                                     &&
         (hard_media_bitmask != 0)                                                        &&
        !(fbe_raid_siots_is_flag_set(siots_p, FBE_RAID_SIOTS_FLAG_SINGLE_ERROR_RECOVERY))    )
    {
        /* We attempted to use redundancy to fix a media error but this failed 
         * because of uncorrectable errors. Since this was unsuccessful, we 
         * have no choice but to take heroic measures by going into region mode.
         */
        fbe_raid_siots_transition(siots_p, fbe_mirror_rebuild_state21);
        return(FBE_RAID_STATE_STATUS_EXECUTING);
    }

    /* Check and handle (retrying the read request if neccessary) checksum
     * errors.
     */
    if (!fbe_raid_csum_handle_csum_error(siots_p, 
                                         read_fruts_p,
                                         FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                         fbe_mirror_rebuild_state4))
    {
        /* We needed to retry the fruts and the above function
         * did so. We are waiting for the retries to finish.
         * Transition to state 4 to evaluate retries.
         */
        return(state_status);
    }

    /* Record errors detected by the XOR operation.
     */
    if (fbe_raid_siots_record_errors(siots_p,
                                     fbe_raid_siots_get_width(siots_p),
                                     &siots_p->misc_ts.cxts.eboard,
                                     FBE_RAID_FRU_POSITION_BITMASK,
                                     FBE_TRUE,
                                     FBE_TRUE /* Yes also validate. */) != FBE_RAID_STATE_STATUS_DONE )
    {
        /* We have detected an inconsistency on the XOR error validation.
         * Wait for the raid group object to process the error.
         */
        return(FBE_RAID_STATE_STATUS_WAITING);
    }

    /* Ok to proceed so transition to next state.
     */
    fbe_raid_siots_transition(siots_p, fbe_mirror_rebuild_state9);

    /* If there is a single rebuild position optimize and simply issue
     * write to single fruts.  Otherwise issue write to all fruts that
     * require it.
     */
    if ((uncorrectable_bitmask == 0)                 &&
        (read_fruts_count == 1)                      &&
        (write_fruts_count == 1)                     &&
        (needs_write_bitmask == write_fruts_bitmask)    )
    {
        /* Normal case. Write rebuild drive only. 
         */
        siots_p->wait_count = 1;

        /* Submit just this fruts for the rebuild position.
         */
        b_result = fbe_raid_fruts_send(write_fruts_p, fbe_raid_fruts_evaluate);
        if (RAID_FRUTS_COND(b_result != FBE_TRUE))
        {
           /* We failed to sent fruts. 
            */
           fbe_raid_siots_set_unexpected_error(siots_p, 
                                               FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS, 
                                               "mirror: %s failed to start write fruts 0x%p for siots_p 0x%p\n", 
                                               __FUNCTION__,
                                               write_fruts_p,
                                               siots_p);
           
            /* We might be here as we might have intentionally failed this 
             * condition while testing for error-path. In that case,
             * we will have to wait for completion of  fruts as it has
             * been already started.
             */
            return ((b_result) ? (FBE_RAID_STATE_STATUS_WAITING) : (FBE_RAID_STATE_STATUS_EXECUTING));
        }
    }
    else if (needs_write_bitmask == 0)
    {
        /* Else if there aren't any positions to correct (although this is 
         * unexpected since we not execute only differential rebuilds and 
         * the rebuild requests should be aligned, there is not reason to
         * fail the request) trace an informational message.
         */
        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_INFO,
                                    "mirror: %s: lba: 0x%llx bl: 0x%llx needs write: 0x%x bitmask is 0. degraded: 0x%x \n",
                                    __FUNCTION__,
			            (unsigned long long)siots_p->start_lba,
			            (unsigned long long)siots_p->xfer_count,
                                    needs_write_bitmask, degraded_bitmask);
        
        /* Transition to state that completes request or issues next request
         * for mining.
         */
        if (siots_p->xfer_count == siots_p->parity_count)
        {
            /* Complete request.
             */
            fbe_raid_siots_transition(siots_p, fbe_mirror_rebuild_state18);
        }
        else
        {
            /* Else rebuild next region.
             */
            fbe_raid_siots_transition(siots_p, fbe_mirror_rebuild_state7);
        }
        state_status = FBE_RAID_STATE_STATUS_EXECUTING;
    }
    else
    {
        /* Setup the write fruts chain from the write bitmask then issue the
         * associated writes.
         */
        status = fbe_mirror_write_reinit_fruts_from_bitmask(siots_p, 
                                                            needs_write_bitmask);
        if (RAID_COND(status != FBE_STATUS_OK))
        {
            fbe_raid_siots_set_unexpected_error(siots_p, 
                                                FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "mirror: rebuild siots: 0x%llx 0x%llx reinit write fruts failed with status: 0x%x\n",  
                                                (unsigned long long)siots_p->start_lba,
						(unsigned long long)siots_p->xfer_count, 
                                                status);
            return(FBE_RAID_STATE_STATUS_EXECUTING);
        }

        /* Start writes to all fruts for our write bitmap.  We must re-fetch
         * the write fruts from the write chain.
         */
        fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_p);
        status =  fbe_raid_fruts_for_each(needs_write_bitmask,
                                      write_fruts_p,
                                      fbe_raid_fruts_retry,
                                      (fbe_u64_t)FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,
                                      CSX_CAST_PTR_TO_PTRMAX(fbe_raid_fruts_evaluate));
        if (RAID_FRUTS_COND(status != FBE_STATUS_OK))
        {
            fbe_raid_siots_set_unexpected_error(siots_p, 
                                                FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "mirror: %s: retry of write_fruts_p 0x%p chain failed for "
                                                "siots_p 0x%p, status = 0x%x\n",  
                                                __FUNCTION__,
                                                write_fruts_p,
                                                siots_p, 
                                                status);
            return(FBE_RAID_STATE_STATUS_EXECUTING);
        }
    }

    /* Always return the state status.
     */
    return(state_status);
}
/*********************************
 * end fbe_mirror_rebuild_state6()
 *********************************/

/*!***************************************************************************
 *          fbe_mirror_rebuild_state7()
 *****************************************************************************
 *
 * @brief   Writes have completed but we are mining mode and there is more
 *          work to do.  We need to determine if it is ok to continue and if
 *          so setup the request for the next region to rebuild.  We have moved
 *          any read fruts that failed the XOR process to the write fruts chain.
 *          Now we need to move those fruts back to the read chain since we can
 *          still read from those positions and the new range may not result in
 *          an XOR error.
 *
 * @param   siots_p - ptr to the fbe_raid_siots_t
 *
 * @return VALUE:
 *      This function returns FBE_RAID_STATE_STATUS_EXECUTING.
 *
 * @author
 *  01/07/2010  Ron Proulx  - Created from mirror_vr_state7
 *
 ****************************************************************/
fbe_raid_state_status_t fbe_mirror_rebuild_state7(fbe_raid_siots_t * siots_p)
{
    fbe_raid_state_status_t     state_status = FBE_RAID_STATE_STATUS_EXECUTING;
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_raid_position_bitmask_t read_bitmask = 0;
    fbe_raid_position_bitmask_t write_bitmask = 0;
    fbe_raid_position_bitmask_t degraded_bitmask = 0;
    
    /* If we are not in region mode something is wrong
     */
    if (RAID_COND(!fbe_raid_siots_is_flag_set(siots_p, FBE_RAID_SIOTS_FLAG_SINGLE_REGION_MODE)))
    {
        fbe_raid_siots_flags_t  flags = fbe_raid_siots_get_flags(siots_p);
        
        /* Set unexpected error.
         */
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS, 
                                            "mirror: unexpected siots flags: 0x%x\n", 
                                            flags);
        return(state_status);
    }

    /* Either reads or write are complete.  Check if request was aborted.
     */
    if (fbe_raid_siots_is_aborted(siots_p))
    {
        /* The iots has been aborted treat it as a hard error,
         * and bail out now.
         */
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_aborted);
        return(state_status);
    }        
    
    /*! @note Even for rebuild requests when we exceed the expiration
     *        time we will fail the request.  It is expected that the
     *        raid group monitor will determine if it should retry the
     *        request or fail the raid group.
     */
    if (fbe_raid_siots_is_expired(siots_p))
    {
        /* Since these error can occur we log an information message.
         */
        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_INFO,
                                    "mirror: %s: lba: 0x%llx blks: 0x%llx exceeded time allowed for request \n",
                                    __FUNCTION__,
			            (unsigned long long)siots_p->start_lba,
			            (unsigned long long)siots_p->xfer_count);
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_expired);
        return(state_status);
    }

    /* Since we no longer exit mining mode we don't set the siots status
     * until it's complete.  Validate that there is another region to
     * rebuild.
     */
    if(RAID_COND(siots_p->xfer_count == 0))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "mirror: siots_p->xfer_count %llu unexpected\n",
                                            (unsigned long long)siots_p->xfer_count);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }
    if(RAID_COND(siots_p->xfer_count <= siots_p->parity_count))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "mirror: siots_p->xfer_count 0x%llx <= siots_p->parity_count 0x%llx\n",
                                            (unsigned long long)siots_p->xfer_count,
                                            (unsigned long long)siots_p->parity_count);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* Since this is single region and external, advance blocks transferred
     * (the xfer_count is subtracted below which insures blocks transferred isn't 
     * incremented again when we finish the siots).
     */
    fbe_mirror_region_complete_update_iots(siots_p);

    /* This strip mine operation has completed.
     * Generate and start the next strip mine operation.
     */
    siots_p->xfer_count -= siots_p->parity_count;
    siots_p->parity_start += siots_p->parity_count;
                
    /* Get parity count for region mining generates the proper size for any
     * runt begining and/or ending portions.
     */
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

    siots_p->parity_count = FBE_MIN(siots_p->xfer_count, siots_p->parity_count);
    
    /* We have set parity_count to the size for the next region.  Update the 
     * read and write fruts with the next count.  Move any non-degraded
     * positions that may have been written back to the read chain.  Since this
     * is a new region there may be no errors for this region.
     */
    fbe_raid_siots_get_read_fruts_bitmask(siots_p, &read_bitmask);
    fbe_raid_siots_get_write_fruts_bitmask(siots_p, &write_bitmask);
    fbe_raid_siots_get_degraded_bitmask(siots_p, &degraded_bitmask);
    write_bitmask &= ~degraded_bitmask;
    read_bitmask |= write_bitmask;
    status = fbe_mirror_read_reinit_fruts_from_bitmask(siots_p, read_bitmask);

    /*! @note Although we are changing the range of the request which may
     *        change which positions are degraded or not.  The assumption is
     *        the raid group executor will break up I/Os that span a degraded
     *        position change.  That is if the original request was from 0x0
     *        thru 0x1eff and position 0 was degraded from 0x800 thru 0x15ff
     *        and position 1 was degraded from 0x0 thru 0x7ff, the request will
     *        be split into a request for 0x0 thru 0x7ff and 0x800 thru 0x1eff.
     */

    /* Need to reset the sgs for the verify request.
     */
    if (status == FBE_STATUS_OK)
    {
        status = fbe_mirror_rebuild_setup_sgs_for_next_pass(siots_p);
    }

    /* Re-initialize any neccessary fields in the siots.
     */
    if (status == FBE_STATUS_OK)
    {
        status = fbe_raid_siots_reinit_for_next_region(siots_p);
    }
    
    /* Check for error.
     */
    if (status != FBE_STATUS_OK)
    {
        /* Report the error and transition to unexpected.
         */
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "mirror: failed to re-initialize siots: 0x%llx blks: 0x%llx for region mode\n", 
                                            (unsigned long long)siots_p->parity_start,
					    (unsigned long long)siots_p->parity_count);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* Return to state 3 to continue mining
     */
    fbe_raid_siots_transition(siots_p, fbe_mirror_rebuild_state3);

    /* Always return state status.
     */
    return(state_status);
}                               
/* end of fbe_mirror_rebuild_state7() */

/****************************************************************
 *          fbe_mirror_rebuild_state8()
 ****************************************************************
 * @brief
 *      A dead error has been taken on one of the drives, but
 *      there is still enough good data to continue.  Remove any
 *      degraded fruts and continue.
 *

 * @param siots_p - ptr to the fbe_raid_siots_t
 *
 * @return VALUE:
 *      FBE_RAID_STATE_STATUS_EXECUTING
 *
 * @author
 *      11/19/01 BJP - Created.
 *
 ****************************************************************/
fbe_raid_state_status_t fbe_mirror_rebuild_state8(fbe_raid_siots_t * siots_p)
{
    fbe_raid_state_status_t state_status = FBE_RAID_STATE_STATUS_EXECUTING;

    /* We have already removed the dead position from the read chain.
     * Validate that there is still valid read data.
     */
    if (siots_p->data_disks > 0)
    {
        /* Transition to state 5 to execute the rebuild 
         */
        fbe_raid_siots_transition(siots_p, fbe_mirror_rebuild_state5);
    }
    else                
    {
        /* Log an error and transition to `unexpected'.
         */
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS, 
                                            "mirror: data_disks: 0x%x \n", 
                                            siots_p->data_disks);
    }

    /* Always return the state status.
     */
    return(state_status);
}
/* end fbe_mirror_rebuild_state8() */

/****************************************************************
 *          fbe_mirror_rebuild_state9()
 ****************************************************************
 * @brief
 *      This function will evaluate the results from write.
 *      If no dead drive or hard errors are encounterd,
 *      we'll either finish the rebuild or go to next strip.
 *

 * @param siots_p - ptr to the fbe_raid_siots_t
 *
 * @return VALUE:
 *      This function returns FBE_RAID_STATE_STATUS_WAITING if need to wait for disk dead;
 *      FBE_RAID_STATE_STATUS_EXECUTING otherwise.
 *
 * @author
 *  01/08/2010  Ron Proulx  - Created from mirror_vr_state9
 *
 ****************************************************************/
fbe_raid_state_status_t fbe_mirror_rebuild_state9(fbe_raid_siots_t * siots_p)
{
    fbe_raid_state_status_t     state_status = FBE_RAID_STATE_STATUS_EXECUTING;
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_raid_fru_eboard_t       fru_eboard;
    fbe_raid_fru_error_status_t fru_status;
    fbe_raid_fruts_t           *write_fruts_p = NULL;

    /* Get the write chain and initialize the fru eboard.
     */
    fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_p);
    fbe_raid_fru_eboard_init(&fru_eboard);
    
    /* Get totals for errored or dead drives.
     */
    fru_status = fbe_mirror_get_fruts_error(write_fruts_p, &fru_eboard);

    /* If writes are successful determine if there is more work (i.e. more
     *  mining) to do.  If not transition to state that will finish request.
     */
    if (fru_status == FBE_RAID_FRU_ERROR_STATUS_SUCCESS)
    {
        /* Write successful determine if finished or more work.
         */
        if (siots_p->xfer_count == siots_p->parity_count)
        {
            /* Restore the write fruts chain back to read fruts chain for 
             * possible error reporting.
             */
            status = fbe_mirror_write_reset_fruts_from_write_chain(siots_p);
            if (RAID_COND(status != FBE_STATUS_OK))
            {
                fbe_raid_siots_set_unexpected_error(siots_p, 
                                                    FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                    "mirror: rebuild siots: 0x%llx 0x%llx reinit read fruts failed with status: 0x%x\n",  
                                                    (unsigned long long)siots_p->start_lba,
						     (unsigned long long)siots_p->xfer_count, 
                                                    status);
                return(FBE_RAID_STATE_STATUS_EXECUTING);
            }
            /* Complete request.
             */
            fbe_raid_siots_transition(siots_p, fbe_mirror_rebuild_state18);
        }
        else
        {
            /* Else rebuild next region.
             */
            fbe_raid_siots_transition(siots_p, fbe_mirror_rebuild_state7);
        }
    }
    else if ((fru_status == FBE_RAID_FRU_ERROR_STATUS_SHUTDOWN) ||
             (fru_status == FBE_RAID_FRU_ERROR_STATUS_DEAD) ||
             (fru_status == FBE_RAID_FRU_ERROR_STATUS_RETRY) ||
             (fru_status == FBE_RAID_FRU_ERROR_STATUS_WAITING) ||
             (fru_status == FBE_RAID_FRU_ERROR_STATUS_ABORTED))
    {
        fbe_raid_state_status_t state_status;
        state_status = fbe_mirror_handle_fruts_error(siots_p, write_fruts_p, &fru_eboard, fru_status);
        return state_status;
    }
    else if (fru_status == FBE_RAID_FRU_ERROR_STATUS_ERROR)
    {
        fbe_raid_status_t   raid_status;

        /*! Invoke the method that will check and process the eboard situation
         *  for a write request.  If the status is success then we can transition
         *  to the next state which either continue or complete the request.
         *
         *  @note We purposefully don't transition in process fruts error so that
         *        all state transitions occur directly from the state machine.
         */
        raid_status = fbe_mirror_write_process_fruts_error(siots_p, &fru_eboard);
        switch(raid_status)
        {
            case FBE_RAID_STATUS_OK_TO_CONTINUE:
                /* Ok to continue to next state.
                 */
                if (siots_p->xfer_count == siots_p->parity_count)
                {
                     /* Restore the write fruts chain back to read fruts chain for 
                     * possible error reporting.
                     */
                    status = fbe_mirror_write_reset_fruts_from_write_chain(siots_p);
                    if (RAID_COND(status != FBE_STATUS_OK))
                    {
                        fbe_raid_siots_set_unexpected_error(siots_p, 
                                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                            "mirror: rebuild siots: 0x%llx 0x%llx reinit read fruts failed with status: 0x%x\n",  
                                                            (unsigned long long)siots_p->start_lba,
							    (unsigned long long)siots_p->xfer_count, 
                                                            status);
                        return(FBE_RAID_STATE_STATUS_EXECUTING);
                    }
                     /* Complete request.
                     */
                    fbe_raid_siots_transition(siots_p, fbe_mirror_rebuild_state18);
                }
                else
                {
                    /* Else rebuild next region.
                     */
                    fbe_raid_siots_transition(siots_p, fbe_mirror_rebuild_state7);
                }
                break;

            case FBE_RAID_STATUS_RETRY_POSSIBLE:
                /* Send out retries.  We will stop retrying if we get aborted or 
                 * our timeout expires or if an edge goes away. 
                 */
                status = fbe_raid_siots_retry_fruts_chain(siots_p, &fru_eboard, write_fruts_p);
                if (status != FBE_STATUS_OK)
                { 
                    fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "mirror: retry %d errors (bitmap 0x%x) status: %d unexpected\n", 
                                                fru_eboard.retry_err_count, fru_eboard.retry_err_bitmap, status);
                    return(FBE_RAID_STATE_STATUS_EXECUTING);
                }
                state_status = FBE_RAID_STATE_STATUS_WAITING;
                break;

            case FBE_RAID_STATUS_TOO_MANY_DEAD:
                fbe_raid_siots_transition(siots_p, fbe_raid_siots_dead_error);
                break;

            case FBE_RAID_STATUS_MEDIA_ERROR_DETECTED:
            case FBE_RAID_STATUS_UNSUPPORTED_CONDITION:
            case FBE_RAID_STATUS_UNEXPECTED_CONDITION:
                /* Some unexpected or unsupported condition occurred.
                 * Transition to unexpected.
                 */
                fbe_raid_siots_transition(siots_p, fbe_raid_siots_unexpected_error);
                break;

            case FBE_RAID_STATUS_OK:
                /* We don't expected success if an error was detected.
                 */
            default:
                /* Process fruts error reported an un-supported status.
                 */
                fbe_raid_siots_set_unexpected_error(siots_p, 
                                                    FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                    "mirror: rebuild unexpected raid_status: 0x%x from write process error\n",
                                                    raid_status);
                break;
        }

        /* Trace some informational text.
         */
        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_INFO,
                                    "mirror: %s: lba: 0x%llx bl: 0x%llx raid_status: 0x%x state status: 0x%x retry count: 0x%x\n",
                                    __FUNCTION__,
			            (unsigned long long)siots_p->start_lba,
			            (unsigned long long)siots_p->xfer_count,
                                    raid_status, state_status, siots_p->retry_count);
    }
    else
    {
        /* Else we either got an unexpected fru error status or the fru error
         * status reported isn't supported.
         */
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "mirror: rebuild siots lba: 0x%llx blks: 0x%llx unexpected fru status: 0x%x\n",
                                            (unsigned long long)siots_p->start_lba,
					    (unsigned long long)siots_p->xfer_count,
					    fru_status);
    }

    /* Always return the state status.
     */
    return(state_status);
}
/*********************************
 * end fbe_mirror_rebuild_state9()
 *********************************/

/****************************************************************
 *          fbe_mirror_rebuild_state10()
 ****************************************************************
 * @brief
 *      Media error is encountered during copy read. We need to
 *      invalidate the sectors and write to the destination.
 *

 * @param siots_p - ptr to the fbe_raid_siots_t
 *
 * @return VALUE:
 *      FBE_RAID_STATE_STATUS_EXECUTING.
 *
 * @author
 *  03/08/2012  Created. Lili Chen
 *
 ****************************************************************/
fbe_raid_state_status_t fbe_mirror_rebuild_state10(fbe_raid_siots_t * siots_p)
{
    fbe_raid_iots_t *iots_p = fbe_raid_siots_get_iots(siots_p);
    fbe_raid_state_status_t     state_status = FBE_RAID_STATE_STATUS_EXECUTING;
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_raid_fruts_t           *read_fruts_p = NULL;
    fbe_u32_t                   read_fruts_count = 0;
    fbe_raid_fruts_t           *write_fruts_p = NULL;
    fbe_raid_position_bitmask_t write_fruts_bitmask = 0;
    fbe_u32_t                   write_fruts_count = 0;
    fbe_payload_block_operation_status_t siots_status, iots_status;
    fbe_raid_common_error_precedence_t iots_error_precedence, siots_error_precedence;
    fbe_payload_block_operation_qualifier_t iots_qualifier;

    /* If enabled log the invalidation.
     */
    fbe_raid_siots_log_traffic(siots_p, "mirror_rebuild_state10");

    /* Get the read and write fruts, bitmasks an counts.
     */
    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_p);
    read_fruts_count = fbe_raid_fruts_count_active(siots_p, read_fruts_p);
    fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_p);
    fbe_raid_siots_get_write_fruts_bitmask(siots_p, &write_fruts_bitmask);
    write_fruts_count = fbe_raid_fruts_count_active(siots_p, write_fruts_p);

    if (write_fruts_count != 1 || read_fruts_count != 1)
    {
        /* During copy operation, only one read and one write fruts is expected.
         */
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS, 
                                            "mirror: siots_p: 0x%p write count 0x%x read count 0x%x\n", 
                                            siots_p, write_fruts_count, read_fruts_count);
        return(FBE_RAID_STATE_STATUS_EXECUTING);
    }

    fbe_raid_siots_transition(siots_p, fbe_mirror_rebuild_state6);

    status = fbe_mirror_rebuild_invalidate_sectors(siots_p);
    if(RAID_COND(status != FBE_STATUS_OK))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "mirror: status %d unexpected\n",
                                            status);
        return(FBE_RAID_STATE_STATUS_EXECUTING);
    }

    /* Update need write bitmask.
     */
    siots_p->misc_ts.cxts.eboard.w_bitmap = write_fruts_bitmask;

    /* Update block status here.
     */
    //fbe_raid_siots_set_block_status(siots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR);
    //fbe_raid_siots_set_block_qualifier(siots_p, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_DATA_LOST);
    siots_status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR;
    fbe_raid_iots_get_block_status(iots_p, &iots_status);

    fbe_raid_iots_get_block_qualifier(iots_p, &iots_qualifier);
    iots_error_precedence = fbe_raid_siots_get_error_precedence(iots_status, iots_qualifier);
    siots_error_precedence = fbe_raid_siots_get_error_precedence(siots_status, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_DATA_LOST);
    if ( siots_error_precedence > iots_error_precedence )
    {
        fbe_raid_iots_set_block_status(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR);
        fbe_raid_iots_set_block_qualifier(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_DATA_LOST);
    }

    /* Always return the state status.
     */
    return(state_status);
}
/*********************************
 * end fbe_mirror_rebuild_state10()
 *********************************/

/*!***************************************************************************
 *          fbe_mirror_rebuild_state18()
 *****************************************************************************
 *
 * @brief   Writes have completed successfully.  Report any errors detected 
 *          and finish request. 
 *
 * @param   siots_p - ptr to the fbe_raid_siots_t
 *
 * @return VALUE:
 *      FBE_RAID_STATE_STATUS_EXECUTING 
 *
 * @author
 *  01/08/2010  Ron Proulx  - Copied from mirror_vr_state18
 *
 *****************************************************************************/
fbe_raid_state_status_t fbe_mirror_rebuild_state18(fbe_raid_siots_t * siots_p)
{
    fbe_raid_state_status_t state_status = FBE_RAID_STATE_STATUS_EXECUTING;

    /* Before completing the request we need to check and handle
     * an aborted request.
     */
    if (fbe_raid_siots_is_aborted(siots_p))
    {
        /* Transition to the abort handling state and complete request.
         */
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_aborted);
        return(state_status);
    }
    
    /* Report any errors detected and transition to finished.
     * Although verify supports nested siots rebuild doesn't so
     * report a failure if it is.
     */

    /* xfer count is always less than parity count
     */
    if(RAID_COND(siots_p->xfer_count > siots_p->parity_count))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "mirror: siots_p->xfer_count 0x%llx > siots_p->parity_count 0x%llx\n",
                                            (unsigned long long)siots_p->xfer_count,
                                            (unsigned long long)siots_p->parity_count);
        return(state_status);
    }

    /* Update the error counters return to the client object.
     */
    fbe_mirror_report_errors(siots_p, FBE_TRUE);

    /* Although verify supports nested siots rebuild doesn't so
     * report a failure if it is.
     */
    if (fbe_raid_siots_is_nested(siots_p))
    {
        /* This is unexpected.  Report an error.
         */
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS, 
                                            "mirror: rebuild siots: 0x%llx 0x%llx nested siots not supported \n", 
                                            (unsigned long long)siots_p->start_lba, (unsigned long long)siots_p->xfer_count);
        return(state_status);
    }

    /* Validate that the status hasn't been set.  If it is (even if it
     * is success) something is wrong.
     */
    if (siots_p->error != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID)
    {
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS, 
                                            "mirror: siots error unexpected 0x%x", siots_p->error);
        return(FBE_RAID_STATE_STATUS_EXECUTING);
    }
    
    /* Transition to success->finished
     */
    fbe_raid_siots_transition(siots_p, fbe_raid_siots_success);
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
    /* Always return the state status.
     */
    return(state_status);
}                               
/* end of fbe_mirror_rebuild_state18() */

/*!********************************************************************
 *          fbe_mirror_rebuild_state21()
 **********************************************************************
 *
 * @brief   This state is entered when a `hard' (i.e. media) error has
 *          occurred and we wish to enter `mining' mode.  This state is
 *          also entered to continue or adjust the mining request size.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t
 *
 * @return VALUE:
 *      This function always returns FBE_RAID_STATE_STATUS_EXECUTING.
 *
 * @note    We do enter mining mode even though we are a rebuild.
 *
 * @author
 *  01/06/2010  Ron Proulx  - Created from mirror_vr_state4
 *
 ****************************************************************/
fbe_raid_state_status_t fbe_mirror_rebuild_state21(fbe_raid_siots_t * siots_p)
{
    fbe_raid_state_status_t state_status = FBE_RAID_STATE_STATUS_EXECUTING;
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_raid_fruts_t       *read_fruts_p = NULL;
    fbe_raid_fruts_t       *write_fruts_p = NULL;
    fbe_raid_position_bitmask_t hard_media_err_bitmask;
    fbe_u16_t               hard_media_err_count = 0;


    /* Fetch the hard (i.e. media) error information from the
     * read fruts chain.
     */
    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_p);
    fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_p);

    /* If there aren't any read fruts remaining something is wrong.
     */
    if (read_fruts_p == NULL)
    {
        /* Report the unexpected error.
         */
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "mirror: rebuild expected at least (1) read fruts siots: 0x%llx 0x%llx\n",
                                            (unsigned long long)siots_p->parity_start, (unsigned long long)siots_p->parity_count);
        return(state_status);
    }

    /* If there aren't any write fruts remaining something is wrong.
     */
    if (write_fruts_p == NULL)
    {
        /* Report the unexpected error.
         */
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "mirror: rebuild expected at least (1) write fruts siots: 0x%llx 0x%llx\n",
                                            (unsigned long long)siots_p->parity_start, (unsigned long long)siots_p->parity_count);
        return(state_status);
    }

    /*! @note Even for rebuild requests when we exceed the expiration
     *        time we will fail the request.  It is expected that the
     *        raid group monitor will determine if it should retry the
     *        request or fail the raid group.
     */
    if (fbe_raid_siots_is_expired(siots_p))
    {
        /* Since these error can occur we log an information message.
         */
        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_INFO,
                                    "mirror: %s: lba: 0x%llx blks: 0x%llx exceeded time allowed for request \n",
                                   __FUNCTION__,
			           (unsigned long long)siots_p->start_lba,
			           (unsigned long long)siots_p->xfer_count);
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_expired);
        return(state_status);
    }

    /* Now get the hard error information.
     */
    hard_media_err_count = fbe_raid_fruts_get_media_err_bitmap(read_fruts_p, &hard_media_err_bitmask);

    /* Now transition to the appropriate state based on the hard error
     * information.
     */
    if (hard_media_err_count == 0)
    {
        /* No hard errors so just continue the rebuild request.
         */
        fbe_raid_siots_transition(siots_p, fbe_mirror_rebuild_state5);
    }
    else
    {
        /* Else we took a hard media error and couldn't recovery with redundancy.
         * Enter mining mode.
         */

        /* Validate hard error count.
         */
        if (hard_media_err_count > fbe_raid_siots_get_width(siots_p))
        {
            fbe_raid_siots_set_unexpected_error(siots_p, 
                                                FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "mirror: error count more than width 0x%x 0x%x", 
                                                hard_media_err_count, fbe_raid_siots_get_width(siots_p));
            return(state_status);
        }

        /* If we are here we are doing error recovery. Set the single error recovery flag
         * since we already have an error and do not want to short ciruit any of the error 
         * handling for efficiency. 
         */
        fbe_raid_siots_set_flag(siots_p, FBE_RAID_SIOTS_FLAG_SINGLE_ERROR_RECOVERY);
        
        /* If we are in region mining mode continue mining
         */
        if (fbe_raid_siots_is_flag_set(siots_p, FBE_RAID_SIOTS_FLAG_SINGLE_REGION_MODE))
        {
            fbe_u32_t  expected_region_size = fbe_raid_siots_get_parity_count_for_region_mining(siots_p);
            if(expected_region_size == 0)
            {
                /* we must have crossed the 32bit limit while calculating the region size
                 */
                fbe_raid_siots_set_unexpected_error(siots_p, 
                                                    FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                    "mirror: siots_p->parity_count == 0 \n");
                return(FBE_RAID_STATE_STATUS_EXECUTING);
            }

            /* Get parity count for region mining generates the proper size for any
             * runt begining and/or ending portions.
             */
            if (siots_p->parity_count != expected_region_size)
            {
                /* We should be in single strip mode here!
                 */
                fbe_raid_siots_set_unexpected_error(siots_p, 
                                                    FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                    "mirror: not in single region mode 0x%llx 0x%x \n", 
                                                    (unsigned long long)siots_p->parity_count, siots_p->flags);
                return(state_status);
            }

            /* Simply continue processing the the rebuild request.
             */
            fbe_raid_siots_transition(siots_p, fbe_mirror_rebuild_state5);
        }
        else
        {
            /* Else drop into region mode.  The first and/or last portions of
             * the request may not be aligned to the region size.  But the
             * middle portions will be aligned.
             */
            fbe_raid_siots_set_flag(siots_p, FBE_RAID_SIOTS_FLAG_SINGLE_REGION_MODE);

            /* Get parity count for region mining generates the proper size for any
             * runt begining and/or ending portions.
             */
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
            
            /* We have set parity_count to the size for the next region.
             * Update the read and write fruts with the next count.
             */
            status = fbe_mirror_reinit_fruts_for_mining(siots_p);

            /* Need to reset the sgs for the verify request.
             */
            if (status == FBE_STATUS_OK)
            {
                status = fbe_mirror_rebuild_setup_sgs_for_next_pass(siots_p);
            }

            /* Re-initialize any neccessary fields in the siots.
            */
            if (status == FBE_STATUS_OK)
            {
                status = fbe_raid_siots_reinit_for_next_region(siots_p);
            }

            /* Check for error.
             */
            if (status != FBE_STATUS_OK)
            {
                /* Report the error and transition to unexpected.
                 */
                fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                    "mirror: failed to re-initialize siots: 0x%llx blks: 0x%llx for region mode\n", 
                                                    (unsigned long long)siots_p->parity_start,
						    (unsigned long long)siots_p->parity_count);
                return FBE_RAID_STATE_STATUS_EXECUTING;
            }

            /* Transition back up to state3.
             */
            fbe_raid_siots_transition(siots_p, fbe_mirror_rebuild_state3);
        }
    }  /* end if hard_media_err_count > 0 */
        
    /* Always return the state status.
     */
    return(state_status);
}
/* end fbe_mirror_rebuild_state21() */


/***************************************************
 * end file fbe_mirror_rebuild.c
 ***************************************************/
