/***************************************************************************
 * Copyright (C) EMC Corporation 1999-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!***************************************************************************
 * @file    fbe_mirror_verify.c
 *****************************************************************************
 *
 * @brief   The file contains the state machine to handle a mirror verify.
 *          Currently mirror rebuilds also use this state machine.
 *
 *          The following algorithms are NO LONGER supported:
 *
 *
 *          The following algorithms are supported by the verify state machine:
 *
 *          o MIRROR_VR - Read from all non-dead positions, determine one or more
 *                        positions with the correct data and then update all
 *                        other non-dead positions using `write verify'.  It is
 *                        assumed that the client has acquired a write lock for
 *                        the entire range to be verified.
 *
 *          o MIRROR_RD_VR - A read recovery verify.  Uses the verify state 
 *                           machine to generate at least one good copy of the
 *                           data and then update the remaining copies.  This is
 *                           a nested siots.
 *
 *          o MIRROR_VR_WR - This algorithm is the `Background Verify Avoidance' (BVA)
 *                           where cache sends a `Verify Write' block opcode with the
 *                           BVA flag set (indicates that no errors should be reported).
 *                           The sequence is to execute a `verify' to correct errors 
 *                           caused by dropped writes and then `write' the new data.
 *
 *          o MIRROR_PACO_VR - This algorithm is used to avoid reading from the 
 *                            proactive candidate (PC) whenever possible.  But this assumes that the proactive
 *                            candidate is writable and that the RAID library 
 *                            controls the error recovery.  In the new scheme
 *                            the primary read position for a proactive spare
 *                            raid group will be the proactive spare.  The set
 *                            degraded code will determine if the proactive
 *                            spare can be used for the read or not.
 *
 *          o MIRROR_VR_BUF - This algorithm is used by an upstream object (i.e. striper
 *                            object) to have the mirror execute a verify request and
 *                            populate the read buffers passed with the result.  It is
 *                            basically a normla verify with buffers provided.
 *
 * @note    Example of 3-way mirror where position 1 was removed and replaced 
 *          and position 0 detects correctable CRC errors. (NR - Needs 
 *          Rebuild, s - siots range).  Note that all 'Needs Rebuild' ranges 
 *          are in `chunk' size multiples where the chunk size is 2048 (0x800) 
 *          blocks.
 *
 *  pba             Position 0           Position 1           Position 2  SIOTS
 *  0x11200         +--------+           +--------+           +--------+  -----
 *                  |        |           |        |           |        |
 *  0x11a00  siots  +--------+           +--------+           +--------+
 *         0x11e00->|ssssssss|           |        |           |        |  <= 1
 *  0x12200         +--------+           +--------+ First RL->+========+
 *                  |crc crc |           |        |           |NR NR NR|  <= 2
 *  0x12a00         +--------+ Degraded->+--------+           +========+
 *                  |ssssssss|           |NR NR NR|           |crc crc |  <= 3
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
 *          SIOTS   Primary (i.e. Read)Positions    Positions that are Rebuilt
 *          -----   ---------------------------     ------------------------------------
 *          1.      Position 0 and 1                None
 *          2.      Position 0 and 1                Position 0 and 2
 *          3.      Position 0 and 2                Position 1 and 2
 *          4.      Position 0                      Position 0 and 1 
 *          Breaking up the I/O is required by the current xor algorithms
 *          which do not handle a degraded change within a request.
 * 
 * @author
 *   11/25/2009 Ron Proulx  - Created from mirror_vr.c circa 08/21/2000 Created Ruomu Gao.
 *
 ***************************************************************************/


/*************************
 ** INCLUDE FILES
 *************************/
#include "fbe_raid_library_proto.h"
#include "fbe_mirror_io_private.h"


/*!***************************************************************************
 *          fbe_mirror_recovery_verify_state0()
 *****************************************************************************
 *
 * @brief   This state generates the request range for a nested verify request.
 *          The verify state machine only supports requests that are aligned to
 *          to the optimal block size.
 *
 * @param   siots_p - Pointer to the nested siots
 *
 * @return VALUE:
 *      This function returns FBE_RAID_STATE_STATUS_EXECUTING.
 *
 * @author
 *  03/29/2010  Ron Proulx  - Updated to support nested verifies
 *
 *****************************************************************************/
fbe_raid_state_status_t fbe_mirror_recovery_verify_state0(fbe_raid_siots_t * siots_p)
{
    fbe_raid_state_status_t state_status = FBE_RAID_STATE_STATUS_EXECUTING;
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_bool_t              b_is_recovery_verify = FBE_FALSE;

    /* Simply use the standard mirror generate code to generate the
     * read recovery verify.
     */
    if ( (status = fbe_mirror_generate_recovery_verify(siots_p, 
                                                       &b_is_recovery_verify)) != FBE_STATUS_OK )
    {
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "mirror: generate recovery verify failed - status: 0x%0x\n",
                                            status);
        return(state_status);
    }
    
    /*! There are (3) cases:
     *  1.  Nested verify request that is a normal party of the original request:
     *          o FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_WRITE
     *      In this case the iots lock includes the correct access (in the case
     *      write access) and range (aligned to optimal block size).  The siots
     *      lock has also been aligned to the optimal blocks size.  Therefore
     *      we don't want to set the error flag and also we don't want to 
     *      take the siots lock.
     *
     *  2.  Nested verify request is required to recovery read data for either
     *      a read request or the pre-read for an unaligned write request:
     *          o FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ
     *          o FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE
     *      In this case the iots lock may or may not be of the correct type
     *      (i.e. may not be a write lock) and the siots lock may not be the
     *      correct range (i.e. may not have been aligned  It looks like
     *      the siots lock is always aligned!!).  In this case we need to
     *      update the siots lock for the parent and may need to change the
     *      iots lock access.
     *
     *  3.  Nested verify request is for a raw mirror read (FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ).  
     *      Raw mirror reads read from each mirror to compare raw mirror sector data metadata for
     *      error handling.  This is similar to a read-recover-verify; however, we do not
     *      correct the data as part of this operation. Any errors are returned to the client 
     *      who is responsible for taking corrective action.
     */
    if (b_is_recovery_verify)
    {
        /* Mark error handling as going on.  Since this is marked in the 
         * iots, both this and the parent siots will be flagged as being in 
         * recovery.
         */
        fbe_raid_iots_inc_errors(fbe_raid_siots_get_iots(siots_p));
    }

    /* Invoke the mirror generate code.  Notice that we no longer copy dead
     * position etc.  The generate code will take care of this.
     */
    state_status = fbe_mirror_generate_start(siots_p);

    /* Always return the state status.
     */
    return(state_status);

}                               
/* end of fbe_mirror_recovery_verify_state0() */

/*!***************************************************************************
 *          fbe_mirror_verify_state0()
 *****************************************************************************
 *
 * @brief   This function allocates and setsup resources for a mirror verify
 *          request.
 *          The default state is FBE_RAID_STATE_STATUS_EXECUTING since the
 *          majority of the time the resources will be immediately available.
 *
 * @param   siots_p - ptr to the fbe_raid_siots_t
 *
 * @return This function returns FBE_RAID_STATE_STATUS_WAITING if we were unable
 *          to allocate the required number of structures immediately;
 *          otherwise, FBE_RAID_STATE_STATUS_EXECUTING is returned.
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

fbe_raid_state_status_t fbe_mirror_verify_state0(fbe_raid_siots_t * siots_p)
{
    fbe_raid_state_status_t state_status = FBE_RAID_STATE_STATUS_EXECUTING;
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_raid_fru_info_t     read_info[FBE_MIRROR_MAX_WIDTH];
    fbe_raid_fru_info_t     write_info[FBE_MIRROR_MAX_WIDTH];

    /* Validate that this is properly configured mirror verify request.
     */
    if ( (status = fbe_mirror_verify_validate(siots_p)) != FBE_STATUS_OK )
    {
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "mirror: verify validate failed. status: 0x%0x\n",
                                            status);
        return(state_status);
    }

    /* Calculate and allocate all the memory needed for this siots. Since we 
     * only allocate resources once the verify range (as far as resource 
     * allocation) is always the entire verify range.
     */
    if ((status = fbe_mirror_verify_calculate_memory_size(siots_p, 
                                                          &read_info[0], 
                                                          &write_info[0])) 
                                                                        != FBE_STATUS_OK )
    {
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "mirror: verify calculate memory size failed. status: 0x%x\n",
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
    fbe_raid_siots_transition(siots_p, fbe_mirror_verify_state1);
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
        status = fbe_mirror_verify_setup_resources(siots_p, 
                                                   &read_info[0],
                                                   &write_info[0]);
        if (RAID_COND(status != FBE_STATUS_OK))
        {
            fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "mirror: verify failed to setup resources. status: 0x%x\n",
                                            status);
            return(state_status);
        }

        /* To prevent confusion (i.e. we never went to `deferred allocation complete'
         * state) overwrite the current state.
         */
        fbe_raid_common_set_state(&siots_p->common, (fbe_raid_common_state_t)fbe_mirror_verify_state3);
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
                                            "mirror: verify memory request failed with status: 0x%x\n",
                                            status);
        return(state_status);
    }

    /* Always return the executing status.
     */
    return(state_status);
}

/* end of fbe_mirror_verify_state0() */

/*!***************************************************************************
 *          fbe_mirror_verify_state1()
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
fbe_raid_state_status_t fbe_mirror_verify_state1(fbe_raid_siots_t *siots_p)
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
                             "mirror: verify memory allocation error common flags: 0x%x \n",
                             fbe_raid_common_get_flags(&siots_p->common));
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_memory_allocation_error);
        return(state_status);
    }

    /* Now calculate the fruts information for each position including the
     * sg index if required.
     */
    status = fbe_mirror_verify_get_fru_info(siots_p, 
                                             &read_info[0],
                                             &write_info[0],
                                             &num_sgs[0],
                                             FBE_TRUE /* Log a failure */);
    if (RAID_COND(status != FBE_STATUS_OK))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "mirror: verify get fru info failed with status: 0x%x\n",
                                            status);
        return(state_status);
    }

    /* Now plant the resources
     */
    status = fbe_mirror_verify_setup_resources(siots_p, 
                                                &read_info[0],
                                                &write_info[0]);
    if (RAID_COND(status != FBE_STATUS_OK))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "mirror: verify failed to setup resources. status: 0x%x\n",
                                            status);
        return(state_status);
    }

    /* Else the resources were setup properly, so transition to
     * the next read state.
     */
    fbe_raid_siots_transition(siots_p, fbe_mirror_verify_state3);

    /* Always return the executing status.
     */
    return(state_status);
}   
/* end of fbe_mirror_verify_state1() */

/*!***************************************************************
 *          fbe_mirror_verify_state3()
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
fbe_raid_state_status_t fbe_mirror_verify_state3(fbe_raid_siots_t * siots_p)
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
    fbe_raid_siots_transition(siots_p, fbe_mirror_verify_state4);
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
/* end of fbe_mirror_verify_state3() */

/****************************************************************
 *          fbe_mirror_verify_state4()
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
fbe_raid_state_status_t fbe_mirror_verify_state4(fbe_raid_siots_t * siots_p)
{
    fbe_raid_state_status_t     state_status = FBE_RAID_STATE_STATUS_EXECUTING;
    fbe_status_t                status = FBE_RAID_STATUS_OK;
    fbe_raid_fru_eboard_t       fru_eboard;
    fbe_raid_fru_error_status_t fru_status;
    fbe_raid_fruts_t           *read_fruts_p = NULL;
    
    /* Since we haven't modified the media we need to check for aborted.
     * Get fruts error will check if this siots have been aborted.
     */

    /* Initialize our local eboard.
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
        fbe_raid_siots_transition(siots_p, fbe_mirror_verify_state5);
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
                fbe_raid_siots_transition(siots_p, fbe_mirror_verify_state5);
                if (fru_eboard.dead_err_count > 0)
                {
                    /* We need to re-evaluate our degraded positions before
                     * performing the verify so that we pick up the degraded positions. 
                     */  
                    status = fbe_mirror_handle_degraded_positions(siots_p,
                                                                  FBE_FALSE,    /* This is a read request */
                                                                  FBE_TRUE    /* Write degraded positions */);
                    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
                    {
                        /* In-sufficient non-degraded positions to continue.  This is unexpected
                         */
                        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS, 
                                                            "mirror: siots_p: 0x%p refresh degraded unexpected status: 0x%x\n", 
                                                            siots_p, status);
                        return(FBE_RAID_STATE_STATUS_EXECUTING);
                    }
                }
                break;

            case FBE_RAID_STATUS_OK_TO_CONTINUE:
            case FBE_RAID_STATUS_RETRY_POSSIBLE:
                /* A retry is possible using the read data from a different
                 * position.  Goto state that will setup to continue.
                 */
                fbe_raid_siots_transition(siots_p, fbe_mirror_verify_state8);

                /* We need to re-evaluate our degraded positions before sending the request
                 * since the raid group monitor may have updated our degraded status. 
                 */  
                status = fbe_mirror_handle_degraded_positions(siots_p,
                                                              FBE_FALSE,    /* This is a read request */
                                                              FBE_TRUE    /* Write degraded positions */);
                if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
                {
                    /* In-sufficient non-degraded positions to continue.  This is unexpected
                     */
                    fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS, 
                                                        "mirror: siots_p: 0x%p refresh degraded unexpected status: 0x%x\n", 
                                                        siots_p, status);
                    return(FBE_RAID_STATE_STATUS_EXECUTING);
                }
                break;

            case FBE_RAID_STATUS_MINING_REQUIRED:
                /* Enter or continue region or strip mode.
                 */
                fbe_raid_siots_transition(siots_p, fbe_mirror_verify_state21);

                /* We need to re-evaluate our degraded positions before sending the request
                 * since the raid group monitor may have updated our degraded status. 
                 */  
                status = fbe_mirror_handle_degraded_positions(siots_p,
                                                              FBE_FALSE,    /* This is a read request */
                                                              FBE_TRUE    /* Write degraded positions */);
                if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
                {
                    /* In-sufficient non-degraded positions to continue.  This is unexpected
                     */
                    fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS, 
                                                        "mirror: siots_p: 0x%p refresh degraded unexpected status: 0x%x\n", 
                                                        siots_p, status);
                    return(FBE_RAID_STATE_STATUS_EXECUTING);
                }
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
                                                    "mirror: verify unexpected raid_status: 0x%x from read process error\n",
                                                    raid_status);
                break;
        }

        /* Trace some informational text.
         */
        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_WARNING,
                                    "mir vr st4: siots: 0x%x parity st/cnt: 0x%llx/0x%llx raid_status: 0x%x\n",
                                    fbe_raid_memory_pointer_to_u32(siots_p),
			            (unsigned long long)siots_p->parity_start,
			            (unsigned long long)siots_p->parity_count,
                                     raid_status);
        fbe_raid_fru_eboard_display(&fru_eboard, siots_p, FBE_TRACE_LEVEL_INFO);
    } /* END else if (status == FBE_RAID_FRU_ERROR_STATUS_ERROR) */
    else
    {
        /* Else we either got an unexpected fru error status or the fru error
         * status reported isn't supported.
         */
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "mirror: verify siots lba: 0x%llx blks: 0x%llx unexpected fru status: 0x%x\n",
                                            siots_p->start_lba, (unsigned long long)siots_p->xfer_count, fru_status);
    }

    /* Always return the state status.
     */
    return(state_status);
}                               
/* end of fbe_mirror_verify_state4() */

/*!***************************************************************************
 *          fbe_mirror_verify_state5()
 *****************************************************************************
 *
 * @brief   Read(s) have completed successfully. At this point we haven't 
 *          validated the data or metadata.  So we use the XOR verify algorithms
 *          to validate (or mark bad) the read data and update the positions that
 *          are rebuilt with success.  The xor is now done in-line so the default
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
fbe_raid_state_status_t fbe_mirror_verify_state5(fbe_raid_siots_t * siots_p)
{
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_raid_state_status_t state_status = FBE_RAID_STATE_STATUS_EXECUTING;
    fbe_raid_fruts_t       *read_fruts_p = NULL;

    /* We should have already checked for aborted.
     */

    /* Ok to check verify read data.  Transition to the state that will 
     * evaluate the results from the XOR operation.
     */
    fbe_raid_siots_transition(siots_p, fbe_mirror_verify_state6);

    /* Get the bitmask of those read positions that need to be fixed.
     * (i.e. written to)
     */
    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_p);
    fbe_raid_fruts_get_media_err_bitmap(read_fruts_p,
                                        &siots_p->misc_ts.cxts.eboard.hard_media_err_bitmap);

    /* Execute a stripwise rebuild. This function will submit any error 
     * messages.
     */
    status = fbe_mirror_verify_extent(siots_p);
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
/* end of fbe_mirror_verify_state5() */

/*!***************************************************************************
 *          fbe_mirror_verify_state6()
 *****************************************************************************
 *
 * @brief   XORs have completed. Process the result and issue write if required.
 *          The default state is executing since typically there aren't any
 *          errors.
 *
 * @param   siots_p - ptr to the fbe_raid_siots_t
 *
 *  @return
 *    This function returns 
 *    FBE_RAID_STATE_STATUS_EXECUTING and FBE_RAID_STATE_STATUS_WAITING.
 *
 *  @author
 *  01/07/2010  Ron Proulx  - Copied from fbe_parity_verify_state6
 *
 *****************************************************************************/
fbe_raid_state_status_t fbe_mirror_verify_state6(fbe_raid_siots_t * siots_p)
{
    fbe_raid_state_status_t     state_status = FBE_RAID_STATE_STATUS_EXECUTING;
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_raid_position_bitmask_t needs_write_bitmask;
    fbe_raid_position_bitmask_t hard_media_bitmask;
    fbe_raid_position_bitmask_t uncorrectable_bitmask;
    fbe_raid_position_bitmask_t degraded_bitmask = 0;
    fbe_raid_position_bitmask_t disabled_bitmask = 0;
    fbe_raid_fruts_t           *read_fruts_p = NULL;
    fbe_raid_fruts_t           *write_fruts_p = NULL;

    /* We don't check for aborted since need to correct any errors
     * that were detected by XOR.
     */

    /* Get the read fruts
     */
    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_p);

    /* Get the bitmasks for the `needs write', `modified', `uncorrectable'
     * bitmasks from the eboard.  The uncorrectable positions are the read
     * positions that resulted in an uncorrectable crc error that now must be
     * written.  XOR has already ORed the `modified' and `uncorrectable' errors
     * into the `needs write' bitmask.
     */
    needs_write_bitmask = fbe_mirror_verify_get_write_bitmap(siots_p);
    uncorrectable_bitmask = siots_p->misc_ts.cxts.eboard.u_bitmap;
    fbe_raid_siots_get_degraded_bitmask(siots_p, &degraded_bitmask);
    fbe_raid_siots_get_disabled_bitmask(siots_p, &disabled_bitmask);
    fbe_raid_fruts_get_media_err_bitmap(read_fruts_p, &hard_media_bitmask);

    /* Now remove any `disabled' (i.e. "really dead" as in missing) positions
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
                                    "mirror: %s: lba: 0x%llx bl: 0x%llx overlap between write: 0x%x and disabled: 0x%x \n",
                                    __FUNCTION__, siots_p->start_lba, siots_p->xfer_count,
                                    needs_write_bitmask, disabled_bitmask);
        needs_write_bitmask &= ~disabled_bitmask;
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
        fbe_raid_siots_transition(siots_p, fbe_mirror_verify_state21);
        return(FBE_RAID_STATE_STATUS_EXECUTING);
    }
    
    /* Check and handle (retrying the read request if neccessary) checksum
     * errors.
     */
    if (!fbe_raid_csum_handle_csum_error(siots_p, 
                                         read_fruts_p,
                                         FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                         fbe_mirror_verify_state4))
    {
        /* We needed to retry the fruts and the above function
         * did so. We are waiting for the retries to finish.
         * Transition to state 4 to evaluate retries.
         */
        return(FBE_RAID_STATE_STATUS_WAITING);
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

    /* The typical case is that no correction is required.  If so
     * simply transition to the completion state.
     */
    if (needs_write_bitmask == 0)
    {
        /* Transition to state that completes request or issues next request
         * for mining.
         */
        if (siots_p->xfer_count == siots_p->parity_count)
        {
            /* Complete request.
             */
            fbe_raid_siots_transition(siots_p, fbe_mirror_verify_state18);
        }
        else
        {
            /* Else verify next region.
             */
            fbe_raid_siots_transition(siots_p, fbe_mirror_verify_state7);
        }
    }
    else
    {
        /* Else issue writes to any positions that need correction.
         * (If allowed)
         */
        fbe_raid_iots_t                     *iots_p = fbe_raid_siots_get_iots(siots_p);
        fbe_payload_block_operation_opcode_t write_opcode = fbe_mirror_get_write_opcode(siots_p);
        fbe_payload_block_operation_opcode_t parent_opcode;
            
        /* Get the original opcode to determine if it was a read request or not.
         * Also if this is a read only verify we cannot write out corrections.
         */
        fbe_raid_siots_get_opcode(siots_p, &parent_opcode);
        if ((parent_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ) ||
            (fbe_raid_siots_is_read_only_verify(siots_p))                 )
        {
            /* Since a read request doesn't have a write lock, the monitor will
             * need to correct the on-disk data with a `remap' (a.k.a  verify) 
             * request.  We still need to fix the error at a later time.
             */
            if (parent_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ)
            {
                /* Flag the fact that the error needs to be corrected. If the
                 * read data wasn't recoverable we will flag that fact in the
                 * parent siots later.
                 */ 
                fbe_raid_iots_set_flag(iots_p, FBE_RAID_IOTS_FLAG_REMAP_NEEDED);
            }
            
            /* Transition to state that completes request or issues next request
             * for mining.
             */
            if (siots_p->xfer_count == siots_p->parity_count)
            {
                /* Complete request.
                */
                fbe_raid_siots_transition(siots_p, fbe_mirror_verify_state18);
            }
            else
            {
                /* Else verify next region.
                 */
                fbe_raid_siots_transition(siots_p, fbe_mirror_verify_state7);
            }
            return(state_status);
        }
        
        /* Write all the positions from the write bitmask.  First setup the
         * write fruts based on the `needs write' bitmask then issue the
         * write fruts chain.
         */
        status = fbe_mirror_write_reinit_fruts_from_bitmask(siots_p, 
                                                            needs_write_bitmask);
        if (RAID_COND(status != FBE_STATUS_OK))
        {
            fbe_raid_siots_set_unexpected_error(siots_p, 
                                                FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "mirror: verify siots: 0x%llx 0x%llx reinit write fruts failed with status: 0x%x\n",  
                                                (unsigned long long)siots_p->start_lba,
						(unsigned long long)siots_p->xfer_count, 
                                                status);
            return(FBE_RAID_STATE_STATUS_EXECUTING);
        }

        /* Start writes to all fruts for our write bitmap.  We must re-fetch
         * the write fruts from the write chain.
         */
        fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_p);
                
        /* Transition to state that will process the write completions.
         * (must transition before issuing the writes).
         */
        fbe_raid_siots_transition(siots_p, fbe_mirror_verify_state9);
        state_status = FBE_RAID_STATE_STATUS_WAITING;
        
        /* Start writes to all fruts for our write bitmap.
         */
        status = fbe_raid_fruts_for_each(needs_write_bitmask,
                                         write_fruts_p,
                                         fbe_raid_fruts_retry,
                                         (fbe_u64_t)write_opcode,
                                         CSX_CAST_PTR_TO_PTRMAX(fbe_raid_fruts_evaluate));
        if (RAID_FRUTS_COND(status != FBE_STATUS_OK))
        {
            fbe_raid_siots_set_unexpected_error(siots_p, 
                                                FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "mirror: %s retry of write_fruts_p 0x%p chain failed for siots_p 0x%p, status = 0x%x\n",  
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
 * end fbe_mirror_verify_state6()
 *********************************/

/*!***************************************************************************
 *          fbe_mirror_verify_state7()
 *****************************************************************************
 * 
 * @brief   We are in region mining mode and region verify has completed
 *          successfully.  Setup the next region request.  This includes 
 *          re-initializing any neccessary fields in the siots.
 *
 * @param   siots_p - ptr to the fbe_raid_siots_t
 *
 * @return VALUE:
 *      This function returns FBE_RAID_STATE_STATUS_EXECUTING.
 *
 * @author
 *  01/07/2010  Ron Proulx  - Created from mirror_vr_state7
 *
 *****************************************************************************/
fbe_raid_state_status_t fbe_mirror_verify_state7(fbe_raid_siots_t * siots_p)
{
    fbe_raid_state_status_t     state_status = FBE_RAID_STATE_STATUS_EXECUTING;
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_raid_position_bitmask_t read_bitmask = 0;
    fbe_raid_position_bitmask_t write_bitmask = 0;
    fbe_raid_position_bitmask_t degraded_bitmask = 0;
    
    /* If we are here we should already in be in region mode
     */
    if (RAID_COND(!fbe_raid_siots_is_flag_set(siots_p, FBE_RAID_SIOTS_FLAG_SINGLE_REGION_MODE)))
    {
        /* We should not have been here. Set unexpected error.
         */
        fbe_raid_siots_flags_t  flags = fbe_raid_siots_get_flags(siots_p);
        
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS, 
                                            "mirror: unexpected siots flags = 0x%x, siots_p = 0x%p\n", 
                                            flags, siots_p);
        return (state_status);
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
    
    /* If we have exceeded the time allowed for this verify request fail it
     * back to the raid group monitor which can retry the request later.
     */
    if (fbe_raid_siots_is_expired(siots_p) && (siots_p->algorithm == MIRROR_VR))
    {
        /* Since these error can occur we log an information message.
         */
        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_INFO,
                                    "mirror: %s: lba: 0x%llx bl: 0x%llx exceeded time allowed for request \n",
                                    __FUNCTION__,
			            (unsigned long long)siots_p->start_lba,
			            (unsigned long long)siots_p->xfer_count);
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_expired);
        return(state_status);
    }

    /* Since we no longer exit mining mode we don't set the siots status
     * until it's complete.  Validate that there is another region to
     * verify.
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

    /* Reprot errors detected in previous strip-mine operation. 
     */
    fbe_mirror_report_errors(siots_p, FBE_FALSE);

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
    if (status != FBE_STATUS_OK)
    {
        /* Report the error and transition to unexpected.
         */
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "mirror: failed to re-initialize fruts while performing strip mining: siots_p = 0x%p\n", 
                                            siots_p);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

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
    status = fbe_mirror_verify_setup_sgs_for_next_pass(siots_p);
    if (status != FBE_STATUS_OK)
    {
        /* Report the error and transition to unexpected.
         */
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "mirror: failed to setup sgs for next pass during strip mining: siots_p = 0x%p\n", 
                                            siots_p);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    } 


    /* Re-initialize any neccessary fields in the siots.
     */
    status = fbe_raid_siots_reinit_for_next_region(siots_p);
    if (status != FBE_STATUS_OK)
    {
        /* Report the error and transition to unexpected.
         */
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "mirror: failed to re-initiatilize bext region during strip mining: siots_p = 0x%p\n", 
                                            siots_p);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    } 

    /* Return to state3 to continue mining
     */
    fbe_raid_siots_transition(siots_p, fbe_mirror_verify_state3);
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

    /* Always return state status.
     */
    return(state_status);
}                               
/* end of fbe_mirror_verify_state7() */

/****************************************************************
 *          fbe_mirror_verify_state8()
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
fbe_raid_state_status_t fbe_mirror_verify_state8(fbe_raid_siots_t * siots_p)
{
    fbe_raid_state_status_t state_status = FBE_RAID_STATE_STATUS_EXECUTING;

    /* We have already removed the dead position from the read chain.
     * Validate that there is still valid read data.
     */
    if (siots_p->data_disks > 0)
    {
        /* Transition to state 5 to execute the rebuild 
         */
        fbe_raid_siots_transition(siots_p, fbe_mirror_verify_state5);
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
/* end fbe_mirror_verify_state8() */

/****************************************************************
 *          fbe_mirror_verify_state9()
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
fbe_raid_state_status_t fbe_mirror_verify_state9(fbe_raid_siots_t * siots_p)
{
    fbe_raid_state_status_t     state_status = FBE_RAID_STATE_STATUS_EXECUTING;
    fbe_status_t                status = FBE_RAID_STATUS_OK;
    fbe_raid_fru_eboard_t       fru_eboard;
    fbe_raid_fru_error_status_t fru_status;
    fbe_raid_fruts_t           *write_fruts_p = NULL;
    
    /* Trace some information if enabled.
     */
    fbe_raid_siots_log_traffic(siots_p, "mirror_verify_state9");

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
        if ((fru_eboard.soft_media_err_count > 0) && (siots_p->algorithm == MIRROR_VR))
        {
            /* Verifies that get a soft media error on the write will fail 
             * back to verify so that verify will retry this.  This limits 
             * the number of media errors we try to fix at once. 
             */
            status = fbe_mirror_write_reset_fruts_from_write_chain(siots_p);
            if (RAID_COND(status != FBE_STATUS_OK))
            {
                fbe_raid_siots_set_unexpected_error(siots_p, 
                                                    FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                    "mirror: verify siots: 0x%llx 0x%llx reinit read fruts failed with status: 0x%x\n",  
                                                    siots_p->start_lba, siots_p->xfer_count, 
                                                    status);
                return(FBE_RAID_STATE_STATUS_EXECUTING);
            }
            fbe_raid_siots_transition(siots_p, fbe_mirror_verify_state22);
            return FBE_RAID_STATE_STATUS_EXECUTING;
        }
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
                                                    "mirror: verify siots: 0x%llx 0x%llx reinit read fruts failed with status: 0x%x\n",  
                                                    (unsigned long long)siots_p->start_lba,
						    (unsigned long long)siots_p->xfer_count, 
                                                    status);
                return(FBE_RAID_STATE_STATUS_EXECUTING);
            }
            /* Complete request.
             */
            fbe_raid_siots_transition(siots_p, fbe_mirror_verify_state18);
        }
        else
        {
            /* Else rebuild next region.
             */
            fbe_raid_siots_transition(siots_p, fbe_mirror_verify_state7);
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
                 if ((siots_p->algorithm == MIRROR_VR) && (fru_eboard.hard_media_err_count > 0))
                 {
                    /* Read remaps do write verifies, so they can get media errors.
                     * Simply transition to return status. 
                     */
                     status = fbe_mirror_write_reset_fruts_from_write_chain(siots_p);
                     if (RAID_COND(status != FBE_STATUS_OK))
                     {
                         fbe_raid_siots_set_unexpected_error(siots_p, 
                                                    FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                    "mirror: verify siots: 0x%llx 0x%llx reinit read fruts failed with status: 0x%x\n",  
                                                    siots_p->start_lba, siots_p->xfer_count, 
                                                    status);
                         return(FBE_RAID_STATE_STATUS_EXECUTING);
                     }
                     fbe_raid_siots_transition(siots_p, fbe_mirror_verify_state22);
                     return FBE_RAID_STATE_STATUS_EXECUTING;
                 }
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
                                                            "mirror: verify siots: 0x%llx 0x%llx reinit read fruts failed with status: 0x%x\n",  
                                                            (unsigned long long)siots_p->start_lba,
							    (unsigned long long)siots_p->xfer_count, 
                                                            status);
                        return(FBE_RAID_STATE_STATUS_EXECUTING);
                    }
                    /* Complete request.
                     */
                    fbe_raid_siots_transition(siots_p, fbe_mirror_verify_state18);
                }
                else
                {
                    /* Else rebuild next region.
                     */
                    fbe_raid_siots_transition(siots_p, fbe_mirror_verify_state7);
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
                                                    "mirror: verify unexpected raid_status: 0x%x from write process error\n",
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
                                            "mirror: verify siots lba: 0x%llx blks: 0x%llx unexpected fru status: 0x%x\n",
                                            (unsigned long long)siots_p->start_lba,
					    (unsigned long long)siots_p->xfer_count,
					    fru_status);
    }

    /* Always return the state status.
     */
    return(state_status);
}
/*********************************
 * end fbe_mirror_verify_state9()
 *********************************/

/*!**************************************************************
 *          fbe_mirror_verify_state10()
 ****************************************************************
 * @brief
 *  Process a media error on a write and verify request.
 *  
 * @param siots_p - The current request.
 * 
 * @return fbe_raid_state_status_t - Always FBE_RAID_STATE_STATUS_EXECUTING.
 * 
 * @author
 *  12/5/2008 - Created. Rob Foley
 *
 ****************************************************************/
fbe_raid_state_status_t fbe_mirror_verify_state10(fbe_raid_siots_t * siots_p)
{
    fbe_raid_state_status_t     state_status = FBE_RAID_STATE_STATUS_EXECUTING;
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_raid_fruts_t           *write_fruts_p = NULL;
    fbe_block_count_t        min_blocks = siots_p->parity_count;
    fbe_raid_position_bitmask_t errored_bitmask = 0;
    fbe_raid_iots_t            *iots_p = fbe_raid_siots_get_iots(siots_p);

    /* If this state wasn't invoked for a normal verify something is wrong.
     */
    if (siots_p->algorithm != MIRROR_VR)
    {
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "mirror: verify siots: 0x%llx 0x%llx alg: 0x%x wasn't MIRROR_RD_REMAP \n",
                                            (unsigned long long)siots_p->start_lba, (unsigned long long)siots_p->xfer_count, siots_p->algorithm);
        return(state_status);
    }

    /* Determine how much we transferred.
     */
    fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_p);
    status = fbe_raid_fruts_get_min_blocks_transferred(write_fruts_p,
                                                       siots_p->parity_start,
                                                       &min_blocks, 
                                                       &errored_bitmask);

    /* A remap request can encounter media errors (i.e. additional latent
     * errors).  Simply return how many blocks were successfully remapped.
     */
    if (min_blocks < siots_p->parity_count)
    {
        fbe_block_count_t   iots_blocks_transferred;

        /* We need to set xfer count to let the remap thread know that we might 
         * have a partial completion. Note that if we block or strip mined, the 
         * iots->blocks_transferred would have been incremented as part of the 
         * strip mine. Thus we only need to worry about setting xfer_count to the
         * number of blocks successfully transferred in this block/strip pass. 
         */
        siots_p->xfer_count = min_blocks;
        fbe_raid_iots_get_blocks_transferred(iots_p, &iots_blocks_transferred);
        iots_blocks_transferred += siots_p->xfer_count;
        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_INFO,
                                    "mirror: verify remap - iots: 0x%llx 0x%llx blks xferred: 0x%llx err_bitmask: 0x%x \n",
                                    (unsigned long long)iots_p->lba,
			            (unsigned long long)iots_p->blocks, 
                                    (unsigned long long)iots_blocks_transferred,
			            errored_bitmask);
            
        /* We need to set media_error lba. 
         * Note that if we block or strip mined, the iots->blocks_transferred 
         * would have been incremented as part of the strip mine. 
         * Thus we only need to worry about setting media error lba to indicate the number of 
         * blocks successfully transferred in this block/strip pass. 
         */
        fbe_raid_siots_set_media_error_lba(siots_p, siots_p->start_lba + min_blocks);
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_media_error);
    }
    else
    {
        /* Else there weren't any errors on the `write and verify'.
         * Transition to the completion state.
         */
        fbe_raid_siots_transition(siots_p, fbe_mirror_verify_state18);
    }

    /* Always return the state status.
     */
    return(state_status);
}
/**************************************
 * end fbe_mirror_verify_state10()
 **************************************/

/*!***************************************************************************
 *          fbe_mirror_verify_state18()
 *****************************************************************************
 *
 * @brief   Writes have completed successfully.  If it is a recovery verify we
 *          need to update the parent data pointer and validate the metadata. 
 *
 * @param   siots_p - ptr to the fbe_raid_siots_t
 *
 * @return VALUE:
 *      FBE_RAID_STATE_STATUS_EXECUTING 
 *
 * @author
 *  01/08/2010  Ron Proulx  - Copied from fbe_parity_verify_state20
 *
 *****************************************************************************/
fbe_raid_state_status_t fbe_mirror_verify_state18(fbe_raid_siots_t * siots_p)
{
    fbe_raid_state_status_t state_status = FBE_RAID_STATE_STATUS_EXECUTING;
    fbe_status_t            status = FBE_STATUS_OK;

    /* If the last request didn't complete the siots we shouldn't be here.
     */
    if (siots_p->parity_count > siots_p->xfer_count)
    {
        fbe_block_count_t blocks_remaining = siots_p->xfer_count - siots_p->parity_count;

        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "mirror: blocks remaining is unexpected 0x%llx 0x%llx 0x%llx\n", 
                                            (unsigned long long)blocks_remaining,
					    (unsigned long long)siots_p->xfer_count,
					    (unsigned long long)siots_p->parity_count);
        return(state_status);
    }

    /* If this is a recovery verify or raw mirror verify check the parent data.
     */
    if (siots_p->algorithm == MIRROR_RD_VR)
    {
        fbe_raid_siots_t *parent_siots_p = fbe_raid_siots_get_parent(siots_p); 

        /*! On a recovery verify we check the result of the verify.  It may be
         *  that we were not able to recovery the data.  In which case we report
         *  a media error for the read request.  Check the checksums and lba
         *  stamps.
         *
         * @note We use the parent siots to do this check, since the parent has 
         *       a complete reference to the the data to check.
         */
        status = fbe_raid_xor_check_siots_checksum(parent_siots_p); 
        if (status != FBE_STATUS_OK)
        {
            fbe_raid_siots_set_unexpected_error(siots_p, 
                                                FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "mirror: xor check status unexpected 0x%x\n", 
                                                status);
            return(state_status);
        }

        /* Else transition to state that will process the xor status.
         */
        fbe_raid_siots_transition(siots_p, fbe_mirror_verify_state19);
    }
    else
    {
        /* Not recovery verify, transition to complete state.
         */
        if (siots_p->error != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID)
        {
            fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS, 
                                                "parity: siots error unexpected 0x%x", siots_p->error);
            return FBE_RAID_STATE_STATUS_EXECUTING;
        }
        fbe_raid_siots_set_block_status(siots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);
        fbe_raid_siots_set_block_qualifier(siots_p, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);
        fbe_raid_siots_transition(siots_p, fbe_mirror_verify_state20);
    }

    /* Always return the state status.
     */
    return(state_status);
}
/* end of fbe_mirror_verify_state18() */

/*!***************************************************************
 *          fbe_mirror_verify_state19()
 ****************************************************************
 * @brief
 *      Process the read recovery checksum.
 *

 * @param siots_p - ptr to the fbe_raid_siots_t
 *
 * @return VALUE:
 *      This function always returns FBE_RAID_STATE_STATUS_EXECUTING.
 *
 * @author
 *  01/13/2010  Ron Proulx  - Copied from fbe_parity_verify_state21
 *
 ****************************************************************/
fbe_raid_state_status_t fbe_mirror_verify_state19(fbe_raid_siots_t * siots_p)
{
    fbe_raid_state_status_t state_status = FBE_RAID_STATE_STATUS_EXECUTING;
    fbe_xor_status_t xor_status = FBE_XOR_STATUS_INVALID;
    fbe_raid_siots_t *parent_siots_p = fbe_raid_siots_get_parent(siots_p);
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);

    fbe_raid_siots_get_xor_command_status(parent_siots_p, &xor_status);

    /* Set the parent siots status based on the xor status.
     */
    if (xor_status == FBE_XOR_STATUS_NO_ERROR)
    {
        /* Indicate that recovery verify was able to generate valid read
         * data.
         */

        /* For raw mirror I/O, set the status based on any raw mirror data metadata errors found.
         */
        if (fbe_raid_geometry_is_raw_mirror(raid_geometry_p))
        {
            fbe_mirror_set_raw_mirror_status(siots_p, xor_status);
        }
        else
        {
            fbe_raid_siots_set_block_status(siots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);
            fbe_raid_siots_set_block_qualifier(siots_p, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);
        }
    }
    else if (xor_status == FBE_XOR_STATUS_CHECKSUM_ERROR)
    {
        /* For raw mirror I/O, set the status based on any raw mirror data metadata errors found.
         */
        if (fbe_raid_geometry_is_raw_mirror(raid_geometry_p))
        {
            fbe_mirror_set_raw_mirror_status(siots_p, xor_status);
        }
        else
        {
            /* Indicate that recovery verify wasn't able to recover the data.
             */
            fbe_raid_siots_set_block_status(siots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR);
            fbe_raid_siots_set_block_qualifier(siots_p, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_DATA_LOST);
        }
    }
    else
    {
        /* Invalid status.
         */
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "mirror: verify unexpected xor status: 0x%x\n", 
                                            xor_status);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* Transition to next state regardless.
     */
    fbe_raid_siots_transition(siots_p, fbe_mirror_verify_state20);

    /* Always return the state status.
     */
    return(state_status);
}                               
/* end of fbe_mirror_verify_state19() */

/*!***************************************************************
 *          fbe_mirror_verify_state20()
 ****************************************************************
 * @brief
 *      The verify has completed, return good status.
 *

 * @param siots_p - ptr to the fbe_raid_siots_t
 *
 * @return VALUE:
 *      This function always returns FBE_RAID_STATE_STATUS_EXECUTING.
 *
 * @note    Previously we would change the status for a remap request
 *          to `media error' if the siots transfer count didn't match
 *          that of the IORB (i.e. request).  This assumes that we
 *          cannot breakup remap request and that we have entered mining
 *          mode. Neither of these conditions is guaranteed.
 *
 * @author
 *  01/13/2010  Ron Proulx  - Created from mirror_vr_state20
 *
 ****************************************************************/
fbe_raid_state_status_t fbe_mirror_verify_state20(fbe_raid_siots_t * siots_p)
{
    fbe_raid_state_status_t state_status = FBE_RAID_STATE_STATUS_EXECUTING;
    fbe_raid_position_bitmask_t disabled_bitmask = 0;
    fbe_raid_position_bitmask_t needs_write_bitmask = 0;
    fbe_payload_block_operation_opcode_t parent_opcode;

    /* Set status to indicate to the parent that we wrote out corrections.
     */
    if ((siots_p->error == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS) &&
        fbe_raid_siots_is_flag_set(siots_p, FBE_RAID_SIOTS_FLAG_WROTE_CORRECTIONS))
    {
        fbe_raid_siots_set_block_qualifier(siots_p, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_COMPLETE_WITH_REMAP);
    }

    /* Update the error counters return to the client object.
     * Then complete (a.k.a finish) the request.
     */
    fbe_mirror_report_errors(siots_p, FBE_TRUE);

    /* Do not transition to error verify if there are only known invalidated blocks
     */
    fbe_raid_siots_get_disabled_bitmask(siots_p, &disabled_bitmask);
    needs_write_bitmask = fbe_mirror_verify_get_write_bitmap(siots_p);
    needs_write_bitmask &= ~disabled_bitmask;
    fbe_raid_siots_get_opcode(siots_p, &parent_opcode);
    if ((needs_write_bitmask != 0) && 
        (parent_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ) &&
        fbe_mirror_only_invalidated_in_error_regions(siots_p))
    {
        fbe_raid_iots_t                     *iots_p = fbe_raid_siots_get_iots(siots_p);
        fbe_raid_iots_clear_flag(iots_p, FBE_RAID_IOTS_FLAG_REMAP_NEEDED);
    }

    fbe_raid_siots_transition_finished(siots_p);        
    /* After reporting errors, just send FBE_BLOCK_TRANSPORT_CONTROL_CODE_UPDATE_LOGICAL_ERROR_STATS
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
    return (state_status);
}                               
/* end of fbe_mirror_verify_state20() */

/*!********************************************************************
 *          fbe_mirror_verify_state21()
 **********************************************************************
 *
 * @brief   This state is entered when :
 *            1. a `hard' (i.e. media) error has occurred and we wish 
 *               to enter `mining' mode -OR-
 *            2. We detected a hard error when already in minining mode.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t
 *
 * @return VALUE:
 *      This function always returns FBE_RAID_STATE_STATUS_EXECUTING.
 *
 * @note    We do enter mining mode for a rebuild request.
 *
 * @author
 *  01/06/2010  Ron Proulx  - Created from mirror_vr_state4
 *
 ****************************************************************/
fbe_raid_state_status_t fbe_mirror_verify_state21(fbe_raid_siots_t * siots_p)
{
    fbe_raid_state_status_t state_status = FBE_RAID_STATE_STATUS_EXECUTING;
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_u32_t               width = fbe_raid_siots_get_width(siots_p);
    fbe_raid_fruts_t       *read_fruts_p = NULL;
    fbe_raid_position_bitmask_t hard_media_err_bitmask;
    fbe_u16_t               hard_media_err_count = 0;


    /* Fetch the hard (i.e. media) error information from the
     * read fruts chain.
     */
    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_p);

    /* If there aren't any read fruts remaining something is wrong.
     */
    if (read_fruts_p == NULL)
    {
        /* Report the unexpected error.
         */
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "mirror: verify expected at least (1) read fruts siots: 0x%llx 0x%llx\n",
                                            (unsigned long long)siots_p->parity_start, (unsigned long long)siots_p->parity_count);
        return (FBE_RAID_STATE_STATUS_EXECUTING);
    }

    /* If we have exceeded the time allowed for this verify request fail it
     * back to the raid group monitor which can retry the request later.
     */
    if (fbe_raid_siots_is_expired(siots_p) && (siots_p->algorithm == MIRROR_VR))
    {
        /* Since these error can occur we log an information message.
         */
        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_INFO,
                                    "mirror: %s: lba: 0x%llx bl: 0x%llx exceeded time allowed for request \n",
                                    __FUNCTION__,
			            (unsigned long long)siots_p->start_lba,
			            (unsigned long long)siots_p->xfer_count);
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_expired);
        return (FBE_RAID_STATE_STATUS_EXECUTING);
    }

    /* Now get the hard error information and transition to the appropriate 
     * state based on the hard error information.
     */
    hard_media_err_count = fbe_raid_fruts_get_media_err_bitmap(read_fruts_p, &hard_media_err_bitmask);
    if (hard_media_err_count == 0)
    {
        /* No hard errors so just continue the verify request.
         */
        fbe_raid_siots_transition(siots_p, fbe_mirror_verify_state5);
        return (FBE_RAID_STATE_STATUS_EXECUTING);
    }

    
    /* If we are here then we got media errors. We will do following:
     *      1. Validate sanity of media error detected so far
     *      2. Enter into strip mining if we are not into it.
     *      3. If we are already into strip mining then either 
     *         setup the operation for next strip mining or fail
     *         the request if all positions have media erros.
     */


     /* Validate sanity of hard error count.
      */
     if (hard_media_err_count > width)
     {
         fbe_raid_siots_set_unexpected_error(siots_p, 
                                             FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                             "mirror: error count more than width 0x%x 0x%x", 
                                             hard_media_err_count, width);
         return FBE_RAID_STATE_STATUS_EXECUTING;
     }


     /* If we are here we are doing error recovery. Set the single error 
      * recovery flag since we already have an error and do not want to 
      * short circuit any of the error handling for efficiency. 
      */
     fbe_raid_siots_set_flag(siots_p, FBE_RAID_SIOTS_FLAG_SINGLE_ERROR_RECOVERY);
     
     if (fbe_raid_siots_is_flag_set(siots_p, FBE_RAID_SIOTS_FLAG_SINGLE_REGION_MODE))
     {
         /* If we are here then it means that we are ALREADY in region mode.
          * We will do following:
          *     1. Report error detected so far
          *     2. Either continue mining or fail the request if all 
          *        positions have media error.
          */
         fbe_payload_block_operation_opcode_t opcode;
         fbe_block_count_t  expected_region_size;
         
         fbe_raid_siots_get_opcode(siots_p, &opcode);
         expected_region_size = fbe_raid_siots_get_parity_count_for_region_mining(siots_p);
         if ((expected_region_size == 0) || (siots_p->parity_count != expected_region_size))
         {
             /* Either we have crossed the 32bit limit while calculating the region size
              * or its not a valid value.
              */
             fbe_raid_siots_set_unexpected_error(siots_p, 
                                         FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                         "mirror: unexpected region size : siots_p = 0x%p, region size = 0x%llx \n", 
                                         siots_p,
                                         (unsigned long long)expected_region_size);

             return (FBE_RAID_STATE_STATUS_EXECUTING);
         }
        
         /* If we are already in region mode and all positions have encountered
          * hard media errors, it is time to give up.
          * Write recovery verify needs to be handled differently. 
          */
         if  ( (hard_media_err_count == width)                              && 
              !(fbe_payload_block_operation_opcode_is_media_modify(opcode))    )
         {
             /* We will do following:
              *     1. Perform XOR operation to construct error region to report
              *        errors detected so far more accurately.
              *     2. Mark this siots with media error, which will utlimately
              *        complete the siots.
              *
              * Please note that, we do stop region mining as soon as we 
              * realize that we may more errors than width of array. At this
              * point of time we are sure that we can return data to host.
              * However, there is a pitfall. It is of not discovering error
              * in next strip-region. And, hence it will not be reported.
              * Its not a big issue as verify is going to discover it anyhow.
              */
              status = fbe_mirror_verify_extent(siots_p);
              if(RAID_COND(status != FBE_STATUS_OK))
              {   
                    fbe_raid_siots_set_unexpected_error(siots_p, 
                                                        FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                        "mirror: status %d unexpected\n",
                                                        status);
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
              fbe_mirror_report_errors(siots_p, FBE_TRUE);

              fbe_raid_siots_transition(siots_p, fbe_raid_siots_media_error);
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
         }
         else
         {
             /* Else simply continue processing the the verify request.
              * If it is a write recovery verify, we are going to invalidate the region. 
              */
             fbe_raid_siots_transition(siots_p, fbe_mirror_verify_state5);
         }
     } /* end if (fbe_raid_siots_is_flag_set(siots_p, FBE_RAID_SIOTS_FLAG_SINGLE_REGION_MODE)) */
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
         if (status != FBE_STATUS_OK)
         {
             fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                 "mirror: failed to re-initialize fruts for strip mining: siots_p = 0x%p\n", 
                                                 siots_p);
             return FBE_RAID_STATE_STATUS_EXECUTING;
         }

         /* Need to reset the sgs for the verify request.
          */
         status = fbe_mirror_verify_setup_sgs_for_next_pass(siots_p);
         if (status != FBE_STATUS_OK)
         {
             fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                 "mirror: failed to setup sgs during strip mining: siots_p = 0x%p\n", 
                                                 siots_p);
             return FBE_RAID_STATE_STATUS_EXECUTING;
         }

         /* Re-initialize any neccessary fields in the siots.
         */
         status = fbe_raid_siots_reinit_for_next_region(siots_p);
         if (status != FBE_STATUS_OK)
         {
             fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                 "mirror: failed to re-initialize siots during strip mining: siots_p = 0x%p\n", 
                                                 siots_p);
             return FBE_RAID_STATE_STATUS_EXECUTING;
         }

         /* Trace the fact that we have entered mining mode
          */
         fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_INFO,
                              "mirror: siots lba: 0x%llx blks: 0x%llx begin region mode pstart: 0x%llx pcount: 0x%llx\n",
                              (unsigned long long)siots_p->start_lba,
			      (unsigned long long)siots_p->xfer_count,
			      (unsigned long long)siots_p->parity_start,
			      (unsigned long long)siots_p->parity_count);

         /* Transition back up to state3.
          */
         fbe_raid_siots_transition(siots_p, fbe_mirror_verify_state3);
     } /* else end if (fbe_raid_siots_is_flag_set(siots_p, FBE_RAID_SIOTS_FLAG_SINGLE_REGION_MODE)) */
     
        
    /* Always return the state status.
     */
    return(state_status);
}
/* end fbe_mirror_verify_state21() */

/*!**************************************************************
 * fbe_mirror_verify_state22()
 ****************************************************************
 * @brief
 *  Process a media error on a write/verify request.
 *  
 * @param siots_p
 * 
 * @return fbe_raid_state_status_t - Always FBE_RAID_STATE_STATUS_EXECUTING.
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_mirror_verify_state22(fbe_raid_siots_t * siots_p)
{
    fbe_status_t status;
    fbe_block_count_t min_blocks = siots_p->parity_count;
    fbe_u16_t errored_bitmask = 0;
    fbe_raid_fruts_t *read_fruts_ptr = NULL;

    /* Get the failing write requests which have been moved to the read chain
     * for error reporting.
     */
    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_ptr);

    /* Transition to next state regardless.
     */
    fbe_raid_siots_transition(siots_p, fbe_mirror_verify_state20);

    /* Determine how much we transferred.
     */
    status = fbe_raid_fruts_get_min_blocks_transferred(read_fruts_ptr, 
                                                       read_fruts_ptr->lba, &min_blocks, &errored_bitmask);

    if (status != FBE_STATUS_OK)
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "mirror: get min blocks transferred unexpected status %d\n", status);
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
                                "mirror:wr media err lba:0x%llX blks:0x%llx m_lba: 0x%llx err_bitmask:0x%x\n",
                                siots_p->start_lba, siots_p->xfer_count,
                                siots_p->media_error_lba, errored_bitmask);
    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/**************************************
 * end fbe_parity_verify_state22()
 **************************************/


/***************************************************
 * end file fbe_mirror_verify.c
 ***************************************************/
