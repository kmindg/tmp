/***************************************************************************
 * Copyright (C) EMC Corporation 2000-2001,2003-2006
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_mirror_write.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the state code for a mirror write.
 *
 * @version
 *   5/15/2009  Ron Proulx  - Created
 *
 ***************************************************************************/

/*************************
 ** INCLUDE FILES
 *************************/
#include "fbe_raid_library_proto.h"
#include "fbe_raid_library.h"
#include "fbe_mirror_io_private.h"
#include "fbe_raid_fruts.h"


/*!***************************************************************************
 *          fbe_mirror_write_state0()
 *****************************************************************************
 *
 * @brief   Determine and allocate the necessary resources for a mirror write
 *          requests.  We always setup write fruts for all position (if the
 *          position is disabled i.e. missing).  Before we actually issue the
 *          write we will determine if the position is disabled and won't 
 *          send the write to the drive.  Although this waste resources the 
 *          typical case is that all positions are writeable.  The default
 *          state status is executing unless the resources aren't immediately
 *          available.
 *
 * @param
 *      siots_p, [IO] - ptr to the RAID_SUB_IOTS
 *
 * @return
 *      This function returns FBE_RAID_STATE_STATUS_WAITING if we were unable
 *      to allocate the required number of structures immediately;
 *      otherwise, FBE_RAID_STATE_STATUS_EXECUTING is returned.
 *
 * @author
 *  07/29/2009  Ron Proulx  - Created
 *
 *****************************************************************************/
fbe_raid_state_status_t fbe_mirror_write_state0(fbe_raid_siots_t *siots_p)
{
    fbe_raid_state_status_t state_status = FBE_RAID_STATE_STATUS_EXECUTING;
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_raid_fru_info_t     preread_info[FBE_MIRROR_MAX_WIDTH];
    fbe_raid_fru_info_t     write_info[FBE_MIRROR_MAX_WIDTH];

    /* Validate that this is properly configured mirror verify request.
     */
    if ( (status = fbe_mirror_write_validate(siots_p)) != FBE_STATUS_OK )
    {
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "mirror: write validate failed. status: 0x%0x\n",
                                            status);
        return(state_status);
    }

    /* Calculate and allocate all the memory needed for this siots.
     */
    if ((status = fbe_mirror_write_calculate_memory_size(siots_p, 
                                                         &preread_info[0], 
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
    fbe_raid_siots_transition(siots_p, fbe_mirror_write_state0a);
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
        status = fbe_mirror_write_setup_resources(siots_p, 
                                                  &preread_info[0],
                                                  &write_info[0]);
        if (RAID_COND(status != FBE_STATUS_OK))
        {
            fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "mirror: write failed to setup resources. status: 0x%x\n",
                                            status);
            return(state_status);
        }

        /* To prevent confusion (i.e. we never went to `deferred allocation complete'
         * state) overwrite the current state.
         */
        fbe_raid_common_set_state(&siots_p->common, (fbe_raid_common_state_t)fbe_mirror_write_state1);
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
                                            "mirror: write memory request failed with status: 0x%x\n",
                                            status);
        return(state_status);
    }

    /* Always return the executing status.
     */
    return(state_status);
}
/* end of fbe_mirror_write_state0() */

/*!***************************************************************************
 *          fbe_mirror_write_state0a()
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
fbe_raid_state_status_t fbe_mirror_write_state0a(fbe_raid_siots_t *siots_p)
{
    fbe_raid_state_status_t state_status = FBE_RAID_STATE_STATUS_EXECUTING;
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_raid_fru_info_t     preread_info[FBE_MIRROR_MAX_WIDTH];
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
                             "mirror: write memory allocation error common flags: 0x%x \n",
                             fbe_raid_common_get_flags(&siots_p->common));
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_memory_allocation_error);
        return(state_status);
    }

    /* Now calculate the fruts information for each position including the
     * sg index if required.
     */
    status = fbe_mirror_write_get_fru_info(siots_p, 
                                           &preread_info[0],
                                           &write_info[0],
                                           &num_sgs[0],
                                           FBE_TRUE /* Log a failure */);
    if (RAID_COND(status != FBE_STATUS_OK))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "mirror: write get fru info failed with status: 0x%x\n",
                                            status);
        return(state_status);
    }

    /* Now plant the resources
     */
    status = fbe_mirror_write_setup_resources(siots_p, 
                                              &preread_info[0],
                                              &write_info[0]);
    if (RAID_COND(status != FBE_STATUS_OK))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "mirror: write failed to setup resources. status: 0x%x\n",
                                            status);
        return(state_status);
    }

    /* Else the resources were setup properly, so transition to
     * the next read state.
     */
    fbe_raid_siots_transition(siots_p, fbe_mirror_write_state1);

    /* Always return the executing status.
     */
    return(state_status);
}   
/* end of fbe_mirror_write_state0a() */

/****************************************************************
 *          fbe_mirror_write_state1()
 ****************************************************************
 *
 * @brief   Validate write checksums and set LBA stamps 
 *
 * @param siots_p - ptr to the fbe_raid_siots_t
 *
 * @return VALUE:
 *      This function returns FBE_RAID_STATE_STATUS_EXECUTING.
 * 
 * @author
 *      1/26/00 - Created. Kevin Schleicher
 *      5/12/00 - Modified to interface with the XOR driver. MPG
 *
 ****************************************************************/
fbe_raid_state_status_t fbe_mirror_write_state1(fbe_raid_siots_t * siots_p)
{
    fbe_raid_state_status_t state_status = FBE_RAID_STATE_STATUS_EXECUTING;
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_raid_geometry_t    *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_bool_t              b_should_generate_stamps = FBE_TRUE;

    /* Based on algorithm do the right thing.
     */
    switch(siots_p->algorithm)
    {
        case MIRROR_WR:
        case MIRROR_CORRUPT_DATA:
            /* For buffered operations (i.e. Zero), we need to generate the
             * write data.
             */
            if (fbe_raid_siots_is_buffered_op(siots_p))
            {
                /* Populate the write data for each position.
                 */
                status = fbe_mirror_write_populate_buffered_request(siots_p);
                if (status != FBE_STATUS_OK)
                {
                    fbe_raid_siots_set_unexpected_error(siots_p, 
                                                        FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                        "mirror: write populate request failed. status: 0x%x\n",
                                                        status);
                    return(state_status);
                }

                /* Skip validation of checksums and transition to issue write.
                 */
                fbe_raid_siots_transition(siots_p, fbe_mirror_write_state1b);
                break;
            }

            // Fall through...

        case MIRROR_VR_WR:
            /* We will always do check checksum irrespective of striper did it or not.
             * Its because, we want to create correct XOR error region (for example
             * in case of corrupt CRC operation). XOR does create error region
             * ONLY as per sector data.
             *
             * Validate that all write positions point to the same
             * buffer and then check checksums on first write position.
             */
            status = fbe_mirror_write_validate_write_buffers(siots_p);
            if (status != FBE_STATUS_OK)
            {
                fbe_raid_siots_set_unexpected_error(siots_p, 
                                                    FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                    "mirror: write validate write buffers failed. status: 0x%x\n",
                                                    status);
                return(state_status);
            }

            /* Validated that all write buffers are the same.  Transition
             * the check checksum completion state.
             */
            fbe_raid_siots_transition(siots_p, fbe_mirror_write_state1a);

            /* If the mirror is a spare we cannot modify the metadata including
             * the lba_stamp.  Otherwise if the client is cache, monitor etc. 
             * we must generate checksum and lba stamp. Check crc only if 
             * requested. Note that the mirror must always put on lba stamps 
             * even for a striped mirror. 
             */
            if (fbe_raid_geometry_is_sparing_type(raid_geometry_p))
            {
                b_should_generate_stamps = FBE_FALSE;
            }
            status = fbe_raid_xor_mirror_write_check_checksum(siots_p, b_should_generate_stamps);
            if (RAID_COND(status != FBE_STATUS_OK))
            {
                fbe_raid_siots_set_unexpected_error(siots_p, 
                                                    FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                    "mirror: status %d unexpected\n",
                                                    status);
                return FBE_RAID_STATE_STATUS_EXECUTING;
            }
            break;
    
        default:
            /* Unsupported algorithm.
             */
            fbe_raid_siots_set_unexpected_error(siots_p, 
                                                FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "mirror: write unexpected algorithm: 0x%x \n",
                                                siots_p->algorithm);
            break;
    }

    /* Always return the state status.
     */
    return(state_status);

}
/* end of fbe_mirror_write_state1() */

/****************************************************************
 *          fbe_mirror_write_state1a()
 ****************************************************************
 *
 * @brief   This function checks for checksum error on cache write
 *          data.  Currently we report `Unexpected Error' of the
 *          cache data is bad but in the future we should simply
 *          fail the request will a specific error qualifier. 
 *
 * @param   siots_p - current sub request
 *
 * @return  This function returns FBE_RAID_STATE_STATUS_EXECUTING.
 *
 * @note    In the future we could fail checksum errors with a qualifier that indicates to
 *          cache that the checksum passed isn't correct.
 *
 * @author
 *  08/10/07 - Created. Jyoti Ranjan
 *
 ****************************************************************/
fbe_raid_state_status_t fbe_mirror_write_state1a(fbe_raid_siots_t * siots_p)
{
    fbe_raid_state_status_t state_status = FBE_RAID_STATE_STATUS_EXECUTING;
    fbe_xor_status_t        xor_status;

    /* Get the xor status.
     */
    fbe_raid_siots_get_xor_command_status(siots_p, &xor_status);
    if (xor_status == FBE_XOR_STATUS_NO_ERROR)
    {
        /* If no error transition to the next write state.
         */
        fbe_raid_siots_transition(siots_p, fbe_mirror_write_state1b);
    }
    else if (xor_status & FBE_XOR_STATUS_BAD_MEMORY)
    {
        /* Mirror should not find bad checksums in the write
         * data because if we were told to check checksums then we expect no CRC errors. 
         * If we were not told to check checksums we wouldn't be here.
         */
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_write_crc_error);
    }
    else
    {
        /* Unexpected XOR status.
         */
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                              "mirror: %s unexpected xor status: 0x%x\n", 
                              __FUNCTION__, xor_status);
    }

    /* Always return the state status.
     */
    return(state_status);
}
/*****************************************
 *  end of fbe_mirror_write_state1a() 
 *****************************************/

/*****************************************************************************
 *          fbe_mirror_write_state1b()
 *****************************************************************************
 *
 * @brief   This method simply determines if we need to execute a normal write
 *          or a verify, write and transitions appropriately.
 *
 * @param   siots_p - current sub request
 *
 * @return  This function returns FBE_RAID_STATE_STATUS_EXECUTING.
 *
 * @author
 *  03/29/2010  Ron Proulx  - Created
 *
 *****************************************************************************/
fbe_raid_state_status_t fbe_mirror_write_state1b(fbe_raid_siots_t * siots_p)
{
    fbe_raid_state_status_t state_status = FBE_RAID_STATE_STATUS_EXECUTING;

    /* Just switch on the algorithm.
     */
    switch(siots_p->algorithm)
    {
        case MIRROR_WR:
        case MIRROR_CORRUPT_DATA:
        case MIRROR_VR_WR:
            /* Just continue with normal write.
             */
            fbe_raid_siots_transition(siots_p, fbe_mirror_write_state2);
            break;
    
        default:
            /* Unsupported algorithm.
             */
            fbe_raid_siots_set_unexpected_error(siots_p, 
                                                FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "mirror: write unexpected algorithm: 0x%x \n",
                                                siots_p->algorithm);
            break;
    }

    /* Always return the state status.
     */
    return(state_status);
}
/*****************************************
 *  end of fbe_mirror_write_state1b() 
 *****************************************/

/*!***************************************************************************
 *          fbe_mirror_write_state2()
 *****************************************************************************
 *
 * @brief   This method determines the type of mirror write to be peformed.
 *          The type check must be performed in the following order:
 *          1) Is this an unaligned write
 *          2) Otherwise it is a normal mirror write
 *          After determine the type of write it will transition to the
 *          appropriate state:
 *          o Unaligned Write   -> Mirror Write State2a (issue pre-reads)
 *          o Normal Write      -> Mirror Write State3  (issue writes)
 *          FBE_RAID_STATE_STATUS_EXECUTING therefore this is the default
 *          state status.
 *
 * @param   siots_p, [IO] - ptr to the RAID_SUB_IOTS
 *
 * @return
 *      This function returns
 *      FBE_RAID_STATE_STATUS_EXECUTING if request aborted, or ready
 *
 * @author
 *  02/08/2010  Ron Proulx  - Created
 *
 *****************************************************************************/
fbe_raid_state_status_t fbe_mirror_write_state2(fbe_raid_siots_t *siots_p)
{
    fbe_raid_state_status_t state_status = FBE_RAID_STATE_STATUS_EXECUTING;

    if (fbe_raid_siots_is_aborted(siots_p))
    {
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_aborted);
        return(state_status);
    }
    /* If the request is unaligned transition to pre-read state machine
     */
    if (fbe_raid_siots_get_read_fruts_count(siots_p) > 0)
    {
        /* If this is an unaligned write we must issue a pre-read.
         */
        fbe_raid_siots_transition(siots_p, fbe_mirror_write_state2a);
    }
    else
    {
        /* Else optimal raid group or aligned write.  Issue write.
         */
        fbe_raid_siots_transition(siots_p, fbe_mirror_write_state3);
    }

    /* Always return the state status.
     */
    return(state_status);
}                               
/* end of fbe_mirror_write_state2() */

/*!***************************************************************************
 *          fbe_mirror_write_state2a()
 *****************************************************************************
 *
 * @brief   This state issues the pre-read for an unaligned mirror write
 *          request.
 *
 * @param   siots_p - ptr to the fbe_raid_siots_t
 *
 * @return VALUE:
 *      This function returns FBE_RAID_STATE_STATUS_WAITING
 *
 * @author
 *  02/11/2010  Ron Proulx  - Updated from fbe_mirror_read_state3
 *
 *****************************************************************************/
fbe_raid_state_status_t fbe_mirror_write_state2a(fbe_raid_siots_t *siots_p)
{
    fbe_raid_state_status_t state_status = FBE_RAID_STATE_STATUS_WAITING;
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_raid_fruts_t       *read_fruts_p = NULL;
    fbe_bool_t              b_result = FBE_TRUE;
    fbe_raid_geometry_t    *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_mirror_prefered_position_t  prefered_mirror_pos;

    if (fbe_raid_siots_is_aborted(siots_p)) {
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_aborted);
        return(FBE_RAID_STATE_STATUS_EXECUTING);
    }

    /* If we are quiescing, then this function will mark the siots as quiesced 
     * and we will wait until we are woken up.
     */
    if (fbe_raid_siots_mark_quiesced(siots_p)) {
        return(FBE_RAID_STATE_STATUS_WAITING);
    }

    /* We need to re-evaluate our degraded positions before sending the request
     * since the raid group monitor may have updated our degraded status. 
     */  
    status = fbe_mirror_handle_degraded_positions(siots_p,
                                                  FBE_FALSE, /* This is a read request */
                                                  FBE_FALSE  /* Don't write degraded positions */);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status)) {
        /* In-sufficient non-degraded positions to continue.  This is unexpected
         */
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS, 
                                            "mirror: siots_p: 0x%p refresh degraded unexpected status: 0x%x\n", 
                                            siots_p, status);
        return(FBE_RAID_STATE_STATUS_EXECUTING);
    }

    /* The wait count is simply the number of items in the read chain
     */
    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_p);
    siots_p->wait_count = fbe_raid_fruts_count_active(siots_p, read_fruts_p);

    /* If the read count isn't (1) something is wrong.
     */
    if (siots_p->wait_count != 1) {
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS, 
                                            "mirror: write wait count unexpected 0x%llx \n", 
                                            siots_p->wait_count);
        return(FBE_RAID_STATE_STATUS_EXECUTING);
    }

    /* Determine which position we will read from.
     */
    fbe_raid_geometry_get_mirror_prefered_position(raid_geometry_p, &prefered_mirror_pos);
    if (!fbe_raid_geometry_is_sparing_type(raid_geometry_p)           &&
        (raid_geometry_p->mirror_optimization_p != NULL)              &&
        (prefered_mirror_pos == FBE_MIRROR_PREFERED_POSITION_INVALID)    ) {
        /* Check and apply read optimization
         */
        status = fbe_mirror_read_set_read_optimization(siots_p);
        if (RAID_GENERATE_COND(status != FBE_STATUS_OK))
        {
            fbe_raid_siots_set_unexpected_error(siots_p, 
                                                FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS, 
                                                "mirror: %s failed in setting read optimization: siots_p = 0x%p, status = %dn",
                                                __FUNCTION__, siots_p, status);
            return(status);
        }
        status = fbe_mirror_read_set_read_optimization(siots_p);
        if (RAID_GENERATE_COND(status != FBE_STATUS_OK))
        {
            fbe_raid_siots_set_unexpected_error(siots_p, 
                                                FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS, 
                                                "mirror: %s failed in setting read optimization: siots_p = 0x%p, status = %dn",
                                                __FUNCTION__, siots_p, status);
            return(FBE_RAID_STATE_STATUS_EXECUTING);
        }

        /* mirror read optimization*/
        fbe_mirror_nway_mirror_optimize_fruts_start(siots_p, read_fruts_p);

    }

    fbe_raid_siots_check_traffic_trace(siots_p, FBE_TRUE);

    /* Always transition BEFORE sending the I/O.  Now send the read fruts
     * chain.  The state status to waiting since we are waiting
     * for the I/Os to complete.
     */
    fbe_raid_siots_transition(siots_p, fbe_mirror_write_state2b);
    b_result = fbe_raid_fruts_send(read_fruts_p, fbe_raid_fruts_evaluate);
    if (RAID_FRUTS_COND(b_result != FBE_TRUE)) {
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS, 
                                            "mirror: %s failed to start read fruts 0x%p for siots 0x%p\n", 
                                            __FUNCTION__,
                                            read_fruts_p,
                                            siots_p);
        
        /* We might be here as we might have intentionally failed this 
         * condition while testing for error-path. In that case,
         * we will have to wait for completion of  fruts as it has
         * been already started.
         */
        return ((b_result) ? (FBE_RAID_STATE_STATUS_WAITING) : (FBE_RAID_STATE_STATUS_EXECUTING));
    }

    /* Always return the state status.
     */
    return(state_status);
}                               
/* end of fbe_mirror_write_state2a() */

/*!***************************************************************************
 *          fbe_mirror_write_state2b()
 *****************************************************************************
 *
 * @brief   Pre-read has completed.  Determine read status and transition to
 *          proper state.  The default state status is executing.
 *
 * @param   siots_p, [IO] - ptr to the RAID_SUB_IOTS
 *
 * @return
 *      This function returns 
 *      FBE_RAID_STATE_STATUS_EXECUTING otherwise
 *      FBE_RAID_STATE_STATUS_WAITING if need to wait for cxts, dh read or disk dead;
 *
 * HISTORY:
 *      08/22/00  RG - Created.
 *
 * @author
 *  02/11/2010  Ron Proulx  - Updated based on fbe_mirror_read_state4
 *
 *****************************************************************************/
fbe_raid_state_status_t fbe_mirror_write_state2b(fbe_raid_siots_t *siots_p)
{
    fbe_raid_state_status_t     state_status = FBE_RAID_STATE_STATUS_EXECUTING;
    fbe_raid_fru_eboard_t       fru_eboard;
    fbe_raid_fru_error_status_t fru_status;
    fbe_raid_fruts_t           *read_fruts_p = NULL;
    
    /* Since we haven't modified the media we need to check for aborted.
     * Get fruts error will check if this siots have been aborted.
     */
    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_p);

    /* Initialize out local eboard.
     */
    fbe_raid_fru_eboard_init(&fru_eboard);
    
    /* Validate that all fruts have completed.
     */
    if (siots_p->wait_count != 0)
    {
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS, 
                                            "mirror: write wait count unexpected 0x%llx \n", 
                                            siots_p->wait_count);
        return(state_status);
    }

    /*! @note We must cleanup the optimize BEFORE getting the fruts error 
     *        since we may change the read position.
     */
    fbe_mirror_optimize_fruts_finish(siots_p, read_fruts_p);

    /* Read has completed. Evaluate the status of the read.
     */
    fru_status = fbe_mirror_get_fruts_error(read_fruts_p, &fru_eboard);

    /* Based on the fruts status we transition to the proper state.
     */
    if (fru_status == FBE_RAID_FRU_ERROR_STATUS_SUCCESS)
    {
        /* Success.  Transition to next state to check data.
         */
        fbe_raid_siots_transition(siots_p, fbe_mirror_write_state2c);

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

        /*let's see if we need to increment any error counts*/
		fbe_raid_fruts_increment_eboard_counters(&fru_eboard, read_fruts_p);

        switch(raid_status)
        {
            case FBE_RAID_STATUS_RETRY_POSSIBLE:
                /* A retry is possible using the read data from a different
                 * position.  Goto state that will setup to continue.
                 */
                fbe_raid_siots_transition(siots_p, fbe_mirror_write_state2d);
                break;

            case FBE_RAID_STATUS_MINING_REQUIRED:
                /* Enter read recovery verify.
                 */
                fbe_raid_siots_transition(siots_p, fbe_mirror_write_state2e);
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

            case FBE_RAID_STATUS_OK:
                /* Since we only read from (1) position and an error was detected,
                 * OK isn't a valid status.
                 */
            default:
                /* Process fruts error reported an un-supported status.
                 */
                fbe_raid_siots_set_unexpected_error(siots_p, 
                                                    FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                    "mirror: write pre-read unexpected raid_status: 0x%x from read process error\n",
                                                    raid_status);
                break;
        }

        /* Trace some informational text.
         */
        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_INFO,
                                    "mirror: %s: lba: 0x%llx bl: 0x%llx raid_status: 0x%x media err cnt: 0x%x \n",
                                    __FUNCTION__,
			            (unsigned long long)siots_p->start_lba,
			            (unsigned long long)siots_p->xfer_count,
                                    raid_status, siots_p->media_error_count);

    } /* END else if (status == FBE_RAID_FRU_ERROR_STATUS_ERROR) */
    else
    {
        /* Else we either got an unexpected fru error status or the fru error
         * status reported isn't supported.
         */
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "mirror: write pre-read siots lba: 0x%llx blks: 0x%llx unexpected fru status: 0x%x\n",
                                            (unsigned long long)siots_p->start_lba,
					    (unsigned long long)siots_p->xfer_count,
					    fru_status);
    }

    /* Always return the state status.
     */
    return(state_status);
}                               
/* end of fbe_mirror_write_state2b() */

/*!***************************************************************************
 *          fbe_mirror_write_state2c()
 *****************************************************************************
 *
 * @brief   Read has completed without error.  Check the checksum/metadata.  
 *          Since the XOR is done in-line the default state status is:
 *          FBE_RAID_STATE_STATUS_EXECUTING.
 *          
 *
 * @param   siots_p, [IO] - ptr to the RAID_SUB_IOTS
 *
 * @return
 *      This function returns FBE_RAID_STATE_STATUS_EXECUTING.
 * 
 * @author
 *  02/11/2010  Ron Proulx - Created from fbe_mirror_read_state5
 *
 *****************************************************************************/
fbe_raid_state_status_t fbe_mirror_write_state2c(fbe_raid_siots_t *siots_p)
{
    fbe_raid_state_status_t     state_status = FBE_RAID_STATE_STATUS_EXECUTING;
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_xor_status_t            xor_status = FBE_XOR_STATUS_INVALID;
    fbe_u32_t                   width = fbe_raid_siots_get_width(siots_p);
    fbe_raid_fruts_t           *read_fruts_p = NULL;
    fbe_raid_position_bitmask_t hard_media_err_bitmask;
    fbe_bool_t                  validate;
    fbe_payload_block_operation_opcode_t opcode;
    fbe_raid_geometry_t        *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_bool_t                  b_should_check_stamps = FBE_TRUE;


    fbe_raid_siots_get_opcode(siots_p, &opcode);

    /* First check and handle the case where we have switched positions due to
     * hard media errors.
     */
    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_p);
    hard_media_err_bitmask = siots_p->misc_ts.cxts.eboard.hard_media_err_bitmap;
    validate = fbe_raid_siots_is_validation_enabled(siots_p);
    if (hard_media_err_bitmask != 0)
    {
        /* Check if there are any hard errors (i.e. uncorrectable media errors) 
         * for a different position that should be changed to correctable errors.
         */
        if ( (status = fbe_raid_siots_convert_hard_media_errors(siots_p,
                                                     (read_fruts_p->position)))
                                                      != FBE_STATUS_OK )
        {
            /* This is unexpected.  It means that we completed a read to a hard
             * errored position.
             */
            fbe_raid_siots_set_unexpected_error(siots_p, 
                                                FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS, 
                                                "mirror: read hard error on read position: 0x%x - status: 0x%x\n", 
                                                (1 << read_fruts_p->position), status);
            return FBE_RAID_STATE_STATUS_EXECUTING; 
        }

        /* Normally verify or rebuild would generate an XOR error region for
         * correctable errors.  But mirrors can generate recoverable crc
         * errors by using redundancy (i.e. reading from a different position).
         * Since the logical error injection code expects that a XOR error
         * region, create one if logical error injection is enabled.
         */
        if (validate == FBE_TRUE)
        {
            fbe_xor_lib_mirror_add_error_region(&siots_p->misc_ts.cxts.eboard,
                                            &siots_p->vcts_ptr->error_regions,
                                            read_fruts_p->lba,
                                            read_fruts_p->blocks,
                                            width);
        }
    }

    /* Now check the checksums.  If checksums do not need to be 
     * checked, then we will check the lba stamp, unless we are in sparing 
     * mode in which case no stamps should be checked.  If not a sparing
     * raid group we will always check CRC and lba stamp for pre-reads.
     */
    if (fbe_raid_geometry_is_sparing_type(raid_geometry_p))
    {
        b_should_check_stamps = FBE_FALSE;
    }
    status = fbe_raid_xor_mirror_preread_check_checksum(siots_p, FBE_TRUE, b_should_check_stamps);
    if (status != FBE_STATUS_OK )
    {
        /* If the request to check checksum fails it means that the request
         * wasn't properly formed.
         */
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS, 
                                            "mirror: write check stamps request failed - status: 0x%x\n", 
                                            status);
        return(state_status);
    }
    
    /* Get the status of the XOR request.
     */
    fbe_raid_siots_get_xor_command_status(siots_p, &xor_status);

    /* Either a crc error was detected and retried or we needed to switch read
     * positions. In either case we need to report errors.
     */
    if ((hard_media_err_bitmask != 0)                ||
        (xor_status & FBE_XOR_STATUS_CHECKSUM_ERROR)    )
    {
        fbe_raid_position_bitmask_t valid_bitmask;

        /* Even if there was no XOR error on this request we must report
         * the errors of the original request.  We report errors for all
         * positions (the siots `retried' flag will force us down this path).
         */
        status = fbe_raid_util_get_valid_bitmask(width, &valid_bitmask);
        if(RAID_COND(status != FBE_STATUS_OK))
        {
            fbe_raid_siots_set_unexpected_error(siots_p, 
                                                FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS, 
                                                "status 0x%x != FBE_STATUS_OK, width 0x%x valid_bitmask 0x%x \n",
                                                status, width, valid_bitmask);
            return(state_status);
        }
        if ( fbe_raid_siots_record_errors(siots_p,
                                          width,
                                          &siots_p->misc_ts.cxts.eboard,
                                          valid_bitmask,    /* Bits to record for. */
                                          FBE_TRUE,         /* Allow correctable errors.  */
                                          validate          /* Validate as required */ )
                                                != FBE_RAID_STATE_STATUS_DONE )
        {
            /* We have detected an inconsistency on the XOR error validation.
             * Wait for the raid group object to process the error.
             */
            return(FBE_RAID_STATE_STATUS_WAITING);
        }

        /* If we have converted a position swap to a correctable CRC (i.e.
         * corrected with redundancy) error clear the bits in the eboard.
         */
        fbe_raid_siots_clear_converted_hard_media_errors(siots_p, hard_media_err_bitmask);
    }
        
    /* Now process the XOR request status.
     */
    if (xor_status == FBE_XOR_STATUS_NO_ERROR)
    {
        /* Retried request succeeded without error.  Continue.
         * (Issue writes)
         */
        fbe_raid_siots_transition(siots_p, fbe_mirror_write_state3);
    }
    else if (fbe_raid_siots_is_aborted(siots_p))
    {
        /* The request was aborted, transition to aborted state to
         * return failed status.
         */
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_aborted);
    }
    else if (xor_status & FBE_XOR_STATUS_CHECKSUM_ERROR)
    {
        /* If there was a checksum error enter read recovery verify which
         * can may recover the data from another position or even if there
         * is only (1) readable position will process the checksum errors.
         */
        fbe_raid_siots_transition(siots_p, fbe_mirror_write_state2e);
    }
    else
    {
        /* Else unexpected/unsupported xor_status.
         */
        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                    "mirror: %s: line: %d Unexpected xor_status: %d\n", 
                                    __FUNCTION__, __LINE__, xor_status);
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_unexpected_error);
    }

    /* Always return the state status.
     */
    return(state_status);

}                               
/* end of fbe_mirror_write_state2c() */

/*!***************************************************************************
 *      fbe_mirror_write_state2d()
 *****************************************************************************
 *
 * @brief   An error was detected either during the read of the XOR on the 
 *          primary position.  By the time this method is invoked we should
 *          have selected a new (i.e. non-degraded) position to read from.
 *          Re-initialize the fruts (this is the same fruts that was used to
 *          read from the primary position) and send the read.  Since this
 *          state issues I/O the default state state is 
 *          FBE_RAID_STATE_STATUS_WAITING.
 *
 * @param   siots_p, [IO] - ptr to the SIOTS
 *
 * @return
 *  This function returns 
 *  FBE_RAID_STATE_STATUS_WAITING if we retry from the alternate drive;
 *  FBE_RAID_STATE_STATUS_EXECUTING if we exit with hard error.
 * 
 * @note
 *  08/22/00  RG - Created.
 *  10/03/00  RFoley - Fixed bug so we can correctly
 *                     determine if it's OK to retry read.
 *  03/20/01 MZ  - LU Centric support (multiple partitions).
 *  03/27/01 BJP - Added N-way mirror support
 *
 * @author
 *  02/11/2010  Ron Proulx  - Created from fbe_mirror_read_state6
 * 
 *****************************************************************************/
fbe_raid_state_status_t fbe_mirror_write_state2d(fbe_raid_siots_t *siots_p)
{
    fbe_raid_state_status_t state_status = FBE_RAID_STATE_STATUS_WAITING;
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_raid_fruts_t       *read_fruts_p = NULL;
    fbe_u32_t               read_fruts_count;
    fbe_bool_t b_result = FBE_TRUE;

    /* We just checked for the I/O being aborted so there is not need to check
     * here.
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
                                                  FBE_FALSE, /* This is a read request */
                                                  FBE_FALSE  /* Don't write degraded positions */);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status)) 
    {
        /* In-sufficient non-degraded positions to continue.  This is unexpected
         */
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS, 
                                            "mirror: siots_p: 0x%p refresh degraded unexpected status: 0x%x\n", 
                                            siots_p, status);
        return(FBE_RAID_STATE_STATUS_EXECUTING);
    }

    /* We shouldn't be here if we haven't setup the read fruts or
     * there is more than (1) fruts on the read chain.
     */
    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_p);

    read_fruts_count = fbe_raid_fruts_count_active(siots_p, read_fruts_p);

    if ((read_fruts_p == NULL) ||
        (read_fruts_count > 1)    )
    {
        /* Shouldn't be here if we haven't setup the read position.
         */
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "mirror: unexpected read fruts count: 0x%x\n",
                                            read_fruts_count);
        return(FBE_RAID_STATE_STATUS_EXECUTING);
    }
    

    read_fruts_count = fbe_raid_fruts_count_active(siots_p, read_fruts_p);
    if (read_fruts_count == 0) {
        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_INFO,
                                    "mirror: write state2d retry lba: 0x%llx bl: 0x%llx me_cnt: 0x%x me_bm: 0x%x\n",
                                    (unsigned long long)siots_p->start_lba,
                                    (unsigned long long)siots_p->xfer_count, 
                                    siots_p->media_error_count,
                                    siots_p->misc_ts.cxts.eboard.media_err_bitmap);
        fbe_raid_siots_transition(siots_p, fbe_mirror_write_state2e);
        return(FBE_RAID_STATE_STATUS_EXECUTING);
    }
    if (read_fruts_count != 1) 
    {
        fbe_raid_position_bitmask_t degraded_bitmask;
        fbe_raid_siots_get_degraded_bitmask(siots_p, &degraded_bitmask);
        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_WARNING,
                                    "%s: 0x%x read_fruts_p : %p hme: 0x%x dead: 0x%x\n",
                                    __FUNCTION__, fbe_raid_memory_pointer_to_u32(siots_p), read_fruts_p,
                                    siots_p->misc_ts.cxts.eboard.hard_media_err_bitmap, degraded_bitmask);
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_media_error);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* We shouldn't be in this state if this position is degraded, detected
     * a media error or an uncorrectable CRC error.  Report an unexpected 
     * error if this is the case.
     */
    if (fbe_raid_siots_is_position_degraded(siots_p, read_fruts_p->position)           ||
        ((1 << read_fruts_p->position) & siots_p->misc_ts.cxts.eboard.hard_media_err_bitmap) ||
        ((1 << read_fruts_p->position) & siots_p->misc_ts.cxts.eboard.u_crc_bitmap)       )

    {
        /* We shouldn't have entered this state if the position isn't usable.
         */
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "mirror: write position: 0x%x hard error: 0x%x uncorrectable crc: 0x%x unexpected \n",
                                            read_fruts_p->position, 
                                            siots_p->misc_ts.cxts.eboard.hard_media_err_bitmap,
                                            siots_p->misc_ts.cxts.eboard.u_crc_bitmap);
        return(FBE_RAID_STATE_STATUS_EXECUTING);

    }

    /*! Re-initialize the read fruts and send it.
     *  
     *  @note Since this is a pre-read for an unaligned write we MUST
     *        use the original pre-read range.  We cannot use the parity range!!
     */
    status = fbe_raid_fruts_init_read_request(read_fruts_p,
                                         FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                         read_fruts_p->lba,
                                         read_fruts_p->blocks,
                                         read_fruts_p->position);
    if( RAID_COND(status != FBE_STATUS_OK) )
    {
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "Fruts init request failed, fruts %p, status 0x%x\n",
                                            read_fruts_p, status);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* The wait count is simply the number of items in the read chain
     */
    siots_p->wait_count = read_fruts_count;

    /* Transition to the state that will process the read completion.
     */
    fbe_raid_siots_transition(siots_p, fbe_mirror_write_state2b);

    /* Issue the read.
     */
    b_result = fbe_raid_fruts_send(read_fruts_p, fbe_raid_fruts_evaluate);
    if (RAID_FRUTS_COND(b_result != FBE_TRUE))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS, 
                                            "mirror: %s failed ito start read fruts 0x%p "
                                            "for siots 0x%p = 0x%x\n", 
                                            __FUNCTION__,
                                            read_fruts_p,
                                            siots_p, b_result);
        
        /* We might be here as we might have intentionally failed this 
         * condition while testing for error-path. In that case,
         * we will have to wait for completion of  fruts as it has
         * been already started.
         */
        return ((b_result) ? (FBE_RAID_STATE_STATUS_WAITING) : (FBE_RAID_STATE_STATUS_EXECUTING));
    }

    /* Always return the state status.
     */
    return state_status;

}
/* end of fbe_mirror_write_state2d() */

/*!***************************************************************************
 *          fbe_mirror_write_state2e()
 *****************************************************************************
 *
 * @brief   We got a hard error or CSUM error, shift to read recovery verify.
 *          The default state status is FBE_RAID_STATE_STATUS_WAITING since
 *          we will enter the verify state machine.
 *
 * @param   siots_p, [IO] - ptr to the RAID_SUB_IOTS
 *
 * @return
 *      This function returns
 *      FBE_RAID_STATE_STATUS_WAITING if we come back from recovery verify.
 *      FBE_RAID_STATE_STATUS_EXECUTING if retry from the other drive.
 * 
 * @note
 *      08/22/00  RG - Created.
 *
 * @author
 *  02/11/2010  Ron Proulx  - Created from fbe_mirror_read_state7
 *
 *****************************************************************************/
fbe_raid_state_status_t fbe_mirror_write_state2e(fbe_raid_siots_t *siots_p)
{
    fbe_raid_state_status_t state_status = FBE_RAID_STATE_STATUS_WAITING;
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_u32_t               full_access_count = fbe_raid_siots_get_full_access_count(siots_p);
    fbe_packet_priority_t   priority;
                    
    /*  Need at least (1) position that is readable (i.e. not degraded) if
     *  we can use recovery verify to get the data.
     */
    if (full_access_count < 1)
    {
        /* Too many degraded positions to attempt recovery verify.
         * This is unexpected.
         */
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_unexpected_error);
        return(FBE_RAID_STATE_STATUS_EXECUTING);
    }

    /* For errors that occur for a sparing object (i.e. errors that occur
     * when the virtual drive is in copy mode) we let the parent raid group 
     * recover (i.e. execute a verify and reconstruct).  Fail the request 
     * with `unrecoverable' error. 
     */
    if (fbe_raid_geometry_is_sparing_type(fbe_raid_siots_get_raid_geometry(siots_p)))
    {
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_media_error);
        return FBE_RAID_STATE_STATUS_EXECUTING;    
    }

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

    /* Drop into single threaded mode for recovery verify
     */
    status = fbe_raid_siots_wait_previous(siots_p);
    if (status == FBE_RAID_STATE_STATUS_WAITING)
    {
        return FBE_RAID_STATE_STATUS_WAITING;
    }

    /* Enter the read recovery verify state machine.  When we get back from 
     * verify, continue with read state machine.
     */
    fbe_raid_siots_transition(siots_p, fbe_mirror_write_state2f);

    /* Transition to recovery verify to attempt to fix errors.
     */
    status = fbe_raid_siots_start_nested_siots(siots_p, fbe_mirror_recovery_verify_state0);

    /* If we weren't able to start the nested siots process the error.
     */
    switch(status)
    {
        case FBE_STATUS_OK:
            /* Simply wait for the nested siots to complete.
             */
            break;

        case FBE_STATUS_INSUFFICIENT_RESOURCES:
            /* Transition to this state and wait for allocation to
             * complete.
             */
            fbe_raid_siots_transition(siots_p, fbe_mirror_write_state2e);
            break;

        default:
            /* Unexpected status.
             */
            fbe_raid_siots_set_unexpected_error(siots_p, 
                                                FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "mirror: write unexpected status: 0x%x \n",
                                                status);
            state_status = FBE_RAID_STATE_STATUS_EXECUTING;
            break;
    }

    /* Always return the state status.
     */
    return(state_status);

}                               
/* end of fbe_mirror_write_state2e() */

/*!***************************************************************************
 *          fbe_mirror_write_state2f()
 *****************************************************************************
 *
 * @brief   We are back from read recovery verify. We should have already 
 *          populated the siots status with either success for failure.  If the
 *          siots status is success we reset the status to invalid and continue
 *          the write request.  Otherwise if the status is failure we simply
 *          fail the request (i.e. complete the request).
 *
 * @param   siots_p, [IO] - ptr to the siots
 *
 * @return  state_status - This function returns FBE_RAID_STATE_STATUS_EXECUTING.
 * 
 * @note    08/22/00  RG - Created.
 *
 * @author
 *  02/11/2009  Ron Proulx - Created from mirror_rd_state9
 *
 *****************************************************************************/
fbe_raid_state_status_t fbe_mirror_write_state2f(fbe_raid_siots_t *siots_p)
{
    fbe_raid_state_status_t state_status = FBE_RAID_STATE_STATUS_EXECUTING;
    fbe_payload_block_operation_status_t    siots_status;
    fbe_payload_block_operation_qualifier_t siots_qualifier;

    /* Returned from read recovery verify. Validate that there are no 
     * nested siots remaining.
     */
    if (!fbe_queue_is_empty(&siots_p->nsiots_q))
    {
        /* Transition to unexpected error.
         */
        fbe_raid_siots_set_unexpected_error(siots_p,
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "mirror: verify write - Nested siots still on queue. \n");
        return(state_status);
    }

    /* Update the error counters return to the client object.
     */
    fbe_mirror_report_errors(siots_p, FBE_TRUE);

    /* Validate that the status and qualifier have been populated
     */
    fbe_raid_siots_get_block_status(siots_p, &siots_status);
    fbe_raid_siots_get_block_qualifier(siots_p, &siots_qualifier);
    if ((siots_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID)       ||
        (siots_qualifier == FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID)    )
    {
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS, 
                                            "mirror: siots lba: 0x%llx blks: 0x%llx unexpected status: 0x%x or qualifier: 0x%x",
                                            (unsigned long long)siots_p->start_lba,
					    (unsigned long long)siots_p->xfer_count,
					    siots_status, siots_qualifier);
        return(FBE_RAID_STATE_STATUS_EXECUTING);
    }

    /* Notice the order of the checks.  Although it isn't necessary to
     * transition to all the states below, it allows for debugging or 
     * special code for specific errors. 
     */
    if (fbe_raid_siots_is_aborted(siots_p))
    {
        /* The iots has been aborted, treat it as a hard error,
         * and bail out now.
         */
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_aborted);
    }
    else if ((siots_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS)||
             (siots_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR))
    {
        /* Else if the pre-read was recovered with verify.  Reset the
         * siots status to invalid and issue the write.
         * If we got a media error during the pre-read , we still want to continue with the write.
         */
        fbe_raid_siots_set_block_status(siots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID);
        fbe_raid_siots_set_block_qualifier(siots_p, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID);
        fbe_raid_siots_transition(siots_p, fbe_mirror_write_state3);
    }
    else if (siots_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED)
    {
        /* shutdown means too many drives are down to continue.
         */
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_shutdown_error);
    }
    else
    {
        /* Else the status reported isn't supported
         */
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "mirror: siots lba: 0x%llx blks: 0x%llx status: 0x%x not supported\n", 
                                            (unsigned long long)siots_p->start_lba,
					    (unsigned long long)siots_p->xfer_count,
					    siots_status);
        return(FBE_RAID_STATE_STATUS_EXECUTING);
    }
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
/* end of fbe_mirror_write_state2f() */

/*!***************************************************************************
 *          fbe_mirror_write_state3()
 *****************************************************************************
 *
 * @brief   This function sends write requests to disks.  We have already set
 *          the degraded/disabled positions for this request (even if this
 *          request issued a pre-read that hit a dead position the disabled
 *          bitmask will indicate the dead position and we waituntil the raid
 *          group object has flag it as rebuild logging) so use the disabled
 *          bitmask to determine which positions should be changed to NOP.
 *          The default state status is waiting.
 *
 * @param   siots_p, [IO] - ptr to the RAID_SUB_IOTS
 *
 * @return
 *      This function returns
 *      FBE_RAID_STATE_STATUS_WAITING if write is sent to the downstream object
 *      FBE_RAID_STATE_STATUS_EXECUTING if there is a failure
 *
 * @note    It is assumed that the raid group object will set the NR bitmap 
 *          for any chunks that are assocaiated for outstanding writes.
 *
 * @author
 *  09/04/2009  Ron Proulx  - Created
 *
 *****************************************************************************/
fbe_raid_state_status_t fbe_mirror_write_state3(fbe_raid_siots_t *siots_p)
{
    fbe_raid_state_status_t     state_status = FBE_RAID_STATE_STATUS_WAITING;
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_bool_t b_result = FBE_TRUE;
    fbe_bool_t b_generate_next_siots = FBE_FALSE;
    fbe_raid_iots_t *iots_p = fbe_raid_siots_get_iots(siots_p);
    fbe_raid_fruts_t           *write_fruts_p = NULL;

    /* Set the data disks to optimal write scenario */
    siots_p->data_disks = fbe_raid_siots_get_width(siots_p);

    fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_p);

    /* If the request was aborted don't continue
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
        return(FBE_RAID_STATE_STATUS_WAITING);
    }

    /* We need to re-evaluate our degraded positions before sending the request
     * since the raid group monitor may have updated our degraded status. 
     */  
    status = fbe_mirror_handle_degraded_positions(siots_p,
                                                  FBE_TRUE, /* This is a write request */
                                                  FBE_FALSE /* Don't write degraded positions */);

    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status)) 
    {
        /* In-sufficient non-degraded positions to continue.  This is unexpected
         */
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS, 
                                            "mirror: siots_p: 0x%p refresh degraded unexpected status: 0x%x\n", 
                                            siots_p, status);
        return(FBE_RAID_STATE_STATUS_EXECUTING);
    }

    /* Handle the corrupt data request (only write to (1) position)
     */
    if (siots_p->algorithm == MIRROR_CORRUPT_DATA)
    {
        status = fbe_mirror_write_handle_corrupt_data(siots_p);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status)) 
        {
            /* In-sufficient non-degraded positions to continue.  This is unexpected
             */
            fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS, 
                                            "mirror: siots_p: 0x%p handle corrupt data unexpected status: 0x%x\n", 
                                            siots_p, status);
            return(FBE_RAID_STATE_STATUS_EXECUTING);
        }
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
                                            "parity: %s failed in next siots generation: siots_p = 0x%p, status: 0x%x \n",
                                            __FUNCTION__, siots_p, status);
        return(FBE_RAID_STATE_STATUS_EXECUTING);
    }

    fbe_raid_siots_check_traffic_trace(siots_p, FBE_TRUE);

    /* Set the wait count and issue the writes.
     */
    siots_p->wait_count = fbe_raid_fruts_count_active(siots_p, write_fruts_p);
    fbe_raid_siots_transition(siots_p, fbe_mirror_write_state4);
    fbe_raid_siots_set_flag(siots_p, FBE_RAID_SIOTS_FLAG_WRITE_STARTED);

    b_result = fbe_raid_fruts_send_chain(&siots_p->write_fruts_head, fbe_raid_fruts_evaluate);
    if (RAID_FRUTS_COND(b_result != FBE_TRUE))
    {
        /* We hit an issue while starting chain of fruts.
         */
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "mirror: %s failed to start write fruts chain for "
                                            "siots = 0x%p, b_result = %d\n",
                                            __FUNCTION__,
                                            siots_p,
                                            b_result);
    }
    if (b_generate_next_siots)
    {
        fbe_raid_iots_generate_next_siots_immediate(iots_p);
    }
    /* Always return the state status.
     */
    return(state_status);
}                               
/* end of fbe_mirror_write_state3() */

/*****************************************************************************
 *          fbe_mirror_write_state4()
 *****************************************************************************
 *
 * @brief   This function will evaluate the results from write.
 *          If no dead drive or hard errors are encounterd,
 *          we complete the write.
 *

 * @param   siots_p - ptr to the fbe_raid_siots_t
 *
 * @return VALUE:
 *      This function returns FBE_RAID_STATE_STATUS_WAITING if need to wait for disk dead;
 *      FBE_RAID_STATE_STATUS_EXECUTING otherwise.
 *
 * @author
 *  02/11/2010  Ron Proulx  - Created from fbe_mirror_verify_state9
 *
 *****************************************************************************/
fbe_raid_state_status_t fbe_mirror_write_state4(fbe_raid_siots_t * siots_p)
{
    fbe_raid_state_status_t     state_status = FBE_RAID_STATE_STATUS_EXECUTING;
    fbe_status_t                status = FBE_RAID_STATUS_OK;
    fbe_raid_fru_eboard_t       fru_eboard;
    fbe_raid_fru_error_status_t fru_status;
    fbe_raid_fruts_t           *write_fruts_p = NULL;

    /* Get totals for errored or dead drives.
     */
    fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_p);
    fbe_raid_fru_eboard_init(&fru_eboard);
    fru_status = fbe_mirror_get_fruts_error(write_fruts_p, &fru_eboard);
    if (fru_status != FBE_RAID_FRU_ERROR_STATUS_WAITING)
    {
        /* Update write fruts header since get fruts error may have removed
         * the head fruts if that position died.
         */
        fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_p);
    }

    /* If writes are successful determine if there is more work (i.e. more
     *  mining) to do.  If not transition to state that will finish request.
     */
    if (fru_status == FBE_RAID_FRU_ERROR_STATUS_SUCCESS)
    {
        /* Set status and transition to common state to complete request.
         */
        fbe_raid_siots_set_block_status(siots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);
        fbe_raid_siots_set_block_qualifier(siots_p, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);
        fbe_raid_siots_transition(siots_p, fbe_mirror_write_state20);
    }
    else if (fru_status == FBE_RAID_FRU_ERROR_STATUS_WAITING)
    {
        fbe_raid_state_status_t state_status;
        state_status = fbe_mirror_handle_fruts_error(siots_p, write_fruts_p, &fru_eboard, fru_status);
        return state_status;
    }
    else if ((fru_status == FBE_RAID_FRU_ERROR_STATUS_SHUTDOWN) ||
             (fru_status == FBE_RAID_FRU_ERROR_STATUS_DEAD) ||
             (fru_status == FBE_RAID_FRU_ERROR_STATUS_RETRY))
    {
        fbe_raid_state_status_t state_status;
         /* We might have gone degraded and have a retry.
          * In this case it is necessary to mark the degraded positions as nops since 
          * 1) we already marked the paged for these 
          * 2) the next time we run through this state we do not want to consider these failures 
          *    (and wait for continue) since we already got the continue. 
          */
        fbe_raid_fruts_set_degraded_nop(siots_p, write_fruts_p);
        state_status = fbe_mirror_handle_fruts_error(siots_p, write_fruts_p, &fru_eboard, fru_status);
        return state_status;
    }
    else if (fru_status == FBE_RAID_FRU_ERROR_STATUS_ERROR)
    {
        fbe_raid_status_t   raid_status;   
        fbe_payload_block_operation_opcode_t opcode;

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

            case FBE_RAID_STATUS_OK_TO_CONTINUE:
                if (siots_p->algorithm == MIRROR_CORRUPT_DATA)
                {
                    /* If this is a corrupt data, the write needs to be issued to
                     * another disk since we didn't send an IO to all the positions.
                     */
                    fbe_raid_siots_transition(siots_p, fbe_mirror_write_state3);
                }
                else
                {
                    /* Executed a degraded write.  Success status.
                     */
                    fbe_raid_siots_set_block_status(siots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);
                    fbe_raid_siots_set_block_qualifier(siots_p, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);
                    fbe_raid_siots_transition(siots_p, fbe_mirror_write_state20);
                }
                break;

            case FBE_RAID_STATUS_RETRY_POSSIBLE:
                /* Send out retries.  We will stop retrying if we get aborted or 
                 * our timeout expires or if an edge goes away. 
                 */
                status = fbe_raid_siots_retry_fruts_chain(siots_p, &fru_eboard, write_fruts_p);
                if (status != FBE_STATUS_OK)
                { 
                    fbe_raid_siots_set_unexpected_error(siots_p, __LINE__, __FUNCTION__,
                                                "mirror: write state4 - retry fruts count:%d bitmask:0x%x status: 0x%x\n", 
                                                fru_eboard.retry_err_count, fru_eboard.retry_err_bitmap, status);
                    return(FBE_RAID_STATE_STATUS_EXECUTING);
                }

                state_status = FBE_RAID_STATE_STATUS_WAITING;
                break;

            case FBE_RAID_STATUS_MEDIA_ERROR_DETECTED:
                /* If the request is for a write-verify, media error is a possible return.
                 * Write-verify support was added here for raw mirrors. 
                 */
                fbe_raid_siots_get_opcode(siots_p, &opcode);
                if (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_VERIFY)
                {
                    fbe_raid_siots_transition(siots_p, fbe_raid_siots_media_error);
                    break;
                }
                /* For writes, we do not expect media errors.
                 */
            case FBE_RAID_STATUS_OK:
                /* We don't expected success of an error was detected.
                 */
            default:
                /* Process fruts error reported an un-supported status.
                 */
                fbe_raid_siots_set_unexpected_error(siots_p, __LINE__, __FUNCTION__,
                                                    "mirror: write state4 - unexpected raid_status: %d\n", 
                                                    raid_status);
                break;
        }

        /* Trace some informational text.
         */
        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_INFO,
                                    "mirror: write state4 lba:0x%llx bl:0x%llx raid_status:%d state:%d retry cnt:%d\n",
                                    (unsigned long long)siots_p->start_lba, 
                                    (unsigned long long)siots_p->xfer_count,
                                    raid_status, state_status, siots_p->retry_count);
    }
    else
    {
        /* Else we either got an unexpected fru error status or the fru error
         * status reported isn't supported.
         */
        fbe_raid_siots_set_unexpected_error(siots_p, __LINE__, __FUNCTION__,
                                            "mirror: write state4 lba:0x%llx bl:0x%llx unexpected fru status:0x%x\n",
                                            (unsigned long long)siots_p->start_lba, 
                                            (unsigned long long)siots_p->xfer_count, 
                                            fru_status);
    }

    /* Always return the state status.
     */
    return(state_status);
}
/*********************************
 * end fbe_mirror_write_state4()
 *********************************/

/*!***************************************************************************
 *          fbe_mirror_write_state7()
 *****************************************************************************
 *
 * @brief   The algorithm is MIRROR_VR_WR.  This state starts the nested verify
 *          request.  When the nested siots completes we transition to the state
 *          that will evaluate the status of the nested verify request.  The
 *          default state status is waiting since we suspend this siots until
 *          nested siots completes.
 *
 * @param   siots_p, [IO] - ptr to the RAID_SUB_IOTS
 *
 * @return
 *      This function returns
 *      FBE_RAID_STATE_STATUS_WAITING if we come back from recovery verify.
 *      FBE_RAID_STATE_STATUS_EXECUTING if an error occurred
 * 
 * @note
 *      08/22/00  RG - Created.
 *
 * @author
 *  02/11/2010  Ron Proulx  - Created from fbe_mirror_read_state7
 *
 ****************************************************************/
fbe_raid_state_status_t fbe_mirror_write_state7(fbe_raid_siots_t *siots_p)
{
    fbe_raid_state_status_t state_status = FBE_RAID_STATE_STATUS_WAITING;
    fbe_status_t            status = FBE_STATUS_OK;
                    
    /* We shouldn't be here if the algorithm isn't MIRROR_VR_WR.
     */
    if (siots_p->algorithm != MIRROR_VR_WR)
    {
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "mirror: write - unexpected algorithm: 0x%x \n",
                                            siots_p->algorithm);
        return(FBE_RAID_STATE_STATUS_EXECUTING);
    }

    /* Enter the verify state machine.  When we get back from verify enter
     * the state that will evaluate the nested status.
     */
    fbe_raid_siots_transition(siots_p, fbe_mirror_write_state9);

    /* Transition to recovery verify to generate verify
     */
    status = fbe_raid_siots_start_nested_siots(siots_p, fbe_mirror_recovery_verify_state0);

    /* If we weren't able to start the nested siots process the error.
     */
    switch(status)
    {
        case FBE_STATUS_OK:
            /* Simply wait for the nested siots to complete.
             */
            break;

        case FBE_STATUS_INSUFFICIENT_RESOURCES:
            /* Transition to this state and wait for allocation to
             * complete.
             */
            fbe_raid_siots_transition(siots_p, fbe_mirror_write_state7);

            /* Currently, if we are here then we are stuck as callback 
             * mechanism to re-start from here (as soon as resources 
             * become available) is not there. So we will finish siots
             * currently till we have infrastructure in-place.
             */
            fbe_raid_siots_set_unexpected_error(siots_p, 
                                                FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "mirror: %s : reosurces were unavailable. siots_p = 0x%p "
                                                "failed status = 0x%x\n",
                                                __FUNCTION__,
                                                siots_p,
                                                status);
            state_status = FBE_RAID_STATE_STATUS_EXECUTING;
            break;

        default:
            /* Unexpected status.
             */
            fbe_raid_siots_set_unexpected_error(siots_p, 
                                                FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "mirror: %s failed to start nested siots for siots_p 0x%p "
                                                "failed status: 0x%x\n",
                                                __FUNCTION__,
                                                siots_p,
                                                status);
            state_status = FBE_RAID_STATE_STATUS_EXECUTING;
            break;
    }

    /* Always return the state status.
     */
    return(state_status);
}                               
/* end of fbe_mirror_write_state7() */

/*!***************************************************************************
 *          fbe_mirror_write_state9()
 *****************************************************************************
 *
 * @brief   We come back from verify to continue write.  If the verify failed
 *          we transition to the appropriate failure state.  If it succeeded
 *          we determine if the write is unaligned or not and transition to
 *          either the issue pre-read state or the issue write state.  The
 *          default state status is executing.
 *
 * @param   siots_p, [IO] - ptr to the siots
 *
 * @return
 *      This function returns FBE_RAID_STATE_STATUS_EXECUTING.
 * 
 * @note
 *      08/22/00  RG - Created.
 *
 * @author
 *  02/11/2010  Ron Proulx - Created from fbe_mirror_read_state9
 *
 ****************************************************************/
fbe_raid_state_status_t fbe_mirror_write_state9(fbe_raid_siots_t *siots_p)
{
    fbe_raid_state_status_t state_status = FBE_RAID_STATE_STATUS_EXECUTING;
    fbe_payload_block_operation_status_t siots_status;

    /* Validate that there are no nested siots remaining.
     */
    if (!fbe_queue_is_empty(&siots_p->nsiots_q))
    {
        /* Transition to unexpected error.
         */
        fbe_raid_siots_set_unexpected_error(siots_p,
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "mirror: %s failed as nested siots still on queue. siots_p = 0x%p\n",
                                            __FUNCTION__,
                                            siots_p);
        return (FBE_RAID_STATE_STATUS_EXECUTING);
    }
    
    /* Returned from verify.  Get the status of the nested siots and reset
     * the status of this siots.
     */
    fbe_raid_siots_get_block_status(siots_p, &siots_status);
    fbe_raid_siots_set_block_status(siots_p,FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID);
    if (fbe_raid_siots_is_aborted(siots_p))
    {
        /* The request was aborted, transition to aborted state to
         * return failed status.
         */
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_aborted);
    }
    else if (siots_status  == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS)
    {
        /* The data has been verified, transition to the issue write state.
         */
        fbe_raid_siots_transition(siots_p, fbe_mirror_write_state2);
    }
    else if (siots_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR)
    {
        /* Verify has checksumed data and found invalid sectors.
         * In this case we do not need to report the errors,
         * since verify already did.
         */
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_media_error);
    }
    else
    {
        /* Else unexpected/unhandled siots status.
         */
        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                    "mirror: %s: line: %d unexpected siots_status: %d\n", 
                                    __FUNCTION__, __LINE__, siots_status);
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_unexpected_error);
    }

    /* Always return the state status.
     */
    return(state_status);
}                               
/* end of fbe_mirror_write_state9() */

/*!***************************************************************
 *          fbe_mirror_write_state20()
 ****************************************************************
 *
 * @brief   The writes have completed, return good status.
 *

 * @param siots_p - ptr to the fbe_raid_siots_t
 *
 * @return VALUE:
 *      This function always returns FBE_RAID_STATE_STATUS_EXECUTING.
 *
 * @author
 *  02/11/2010  Ron Proulx  - Created from fbe_mirror_read_state20
 *
 ****************************************************************/
fbe_raid_state_status_t fbe_mirror_write_state20(fbe_raid_siots_t * siots_p)
{
    fbe_raid_state_status_t state_status = FBE_RAID_STATE_STATUS_EXECUTING;
    fbe_payload_block_operation_status_t    siots_status;
    fbe_payload_block_operation_qualifier_t siots_qualifier;

    /* Update the error counters return to the client object.
     */
    fbe_mirror_report_errors(siots_p, FBE_TRUE);

    /* Validate that the status and qualifier have been populated
     */
    fbe_raid_siots_get_block_status(siots_p, &siots_status);
    fbe_raid_siots_get_block_qualifier(siots_p, &siots_qualifier);
    if ((siots_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID)       ||
        (siots_qualifier == FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID)    )
    {
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS, 
                                            "mirror: %s errored: siots lba: 0x%llx blks: 0x%llx "
                                            "unexpected status: 0x%x or qualifier: 0x%x, siots_p = 0x%p\n",
                                            __FUNCTION__,
                                            (unsigned long long)siots_p->start_lba,
					    (unsigned long long)siots_p->xfer_count,
                                            siots_status, siots_qualifier,
                                            siots_p);
        return(FBE_RAID_STATE_STATUS_EXECUTING);
    }

    /* Notice the order of the checks.  Although it isn't necessary to
     * transition to all the states below, it allows for debugging or 
     * special code for specific errors. 
     */
    if (siots_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS)
    {
        /* If success transition to success (yes this resets the status)
         */
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_success);
    }
    else if (siots_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED)
    {
        /* shutdown means too many drives are down to continue.
         */
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_shutdown_error);
    }
    else if (siots_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR)
    {
        /* media error, means we took an uncorrectable error.
         */
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_media_error);
    }
    else
    {
        /* Else the status reported isn't supported
         */
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "mirror: siots lba: 0x%llx blks: 0x%llx status: 0x%x not supported\n", 
                                            (unsigned long long)siots_p->start_lba,
					    (unsigned long long)siots_p->xfer_count,
					    siots_status);
        return(FBE_RAID_STATE_STATUS_EXECUTING);
    }
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
/* end of fbe_mirror_write_state20() */

