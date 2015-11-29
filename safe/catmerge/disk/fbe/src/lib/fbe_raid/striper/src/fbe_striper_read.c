/***************************************************************************
 * Copyright (C) EMC Corporation 1999-2007
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * fbe_striper_read.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the state functions of the RAID 0 Read Algorithms.
 *  There are two read state machines:
 *    Small Read - For reads that hit a single drive.
 *                 This state machine starts in r0_smrd_state0()  
 *    Normal Read - For read that span drives.
 *                 This state machine starts in fbe_striper_read_state0()
 *
 * @notes
 *      At initial entry, we assume that the fbe_raid_siots_t has
 *      the following fields initialized:
 *              start_lba       - logical address of first block of new data
 *              xfer_count      - total blocks of new data
 *              data_disks      - tatal data disks affected 
 *              start_pos       - relative start disk position  
 *
 * @author
 *   2000 Created.  Rob Foley.
 *   2010 Ported to FBE.  Rob Foley.
 *
 ***************************************************************************/


/*************************
 ** INCLUDE FILES
 *************************/

#include "fbe_raid_library_proto.h"
#include "fbe_striper_io_private.h"

 
/****************************************************************
 * fbe_striper_read_state0()
 ****************************************************************
 * @brief
 *  This function allocates and sets up resources.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t 
 *
 * @return fbe_status_t either FBE_RAID_STATE_STATUS_EXECUTING
 *                          or FBE_RAID_STATE_STATUS_WAITING
 *
 * @author
 *  12/9/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_striper_read_state0(fbe_raid_siots_t * siots_p)
{
    fbe_status_t status;
    /* Add one for terminator.
     */
    fbe_raid_fru_info_t read_info[FBE_RAID_MAX_DISK_ARRAY_WIDTH+1];

    /* Validate the algorithm.
     */
    if (siots_p->algorithm != RAID0_RD)
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "striper: unexpected algorithm: %d\n", siots_p->algorithm);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* Calculate and allocate all the memory needed for this siots.
     */
    status = fbe_striper_read_calculate_memory_size(siots_p, &read_info[0]);
    if (status != FBE_STATUS_OK)
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "striper: failed to calculate mem size status: 0x%x algorithm: 0x%x\n", 
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
    fbe_raid_siots_transition(siots_p, fbe_striper_read_state1);
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
        status = fbe_striper_read_setup_resources(siots_p, &read_info[0]);
        if (RAID_COND(status != FBE_STATUS_OK))
        {
            fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "striper: read failed to setup resources. status: 0x%x\n",
                                            status);
            return FBE_RAID_STATE_STATUS_EXECUTING;
        }

        /* To prevent confusion (i.e. we never went to `deferred allocation complete'
         * state) overwrite the current state.
         */
        fbe_raid_common_set_state(&siots_p->common, (fbe_raid_common_state_t)fbe_striper_read_state3);
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
                                            "striper: read memory request failed with status: 0x%x\n",
                                            status);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    return FBE_RAID_STATE_STATUS_EXECUTING;

} /* end of fbe_striper_read_state0() */

/****************************************************************
 * fbe_striper_read_state1()
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

fbe_raid_state_status_t fbe_striper_read_state1(fbe_raid_siots_t * siots_p)
{
    fbe_status_t            status;
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
                             "striper: read memory allocation error common flags: 0x%x \n",
                             fbe_raid_common_get_flags(&siots_p->common));
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_memory_allocation_error);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* Then calculate how many buffers are needed.
     */
    status = fbe_striper_read_calculate_memory_size(siots_p, &read_info[0]);
    if (status != FBE_STATUS_OK)
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "striper: failed to calculate mem size status: 0x%x algorithm: 0x%x\n", 
                                            status, siots_p->algorithm);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* Allocation completed. Plant the resources
     */
    status = fbe_striper_read_setup_resources(siots_p, &read_info[0]);
    if (RAID_COND(status != FBE_STATUS_OK))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "striper: read failed to setup resources. status: 0x%x\n",
                                            status);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* Transition to next state to issue request
     */
    fbe_raid_siots_transition(siots_p, fbe_striper_read_state3);

    /* Return the state status
     */
    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/***********************************
 * end fbe_striper_read_state1()
 **********************************/

/****************************************************************
 * fbe_striper_read_state3()
 ****************************************************************
 * @brief
 *  This function starts the read operations.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t 
 *
 * @return FBE_RAID_STATE_STATUS_EXECUTING
 * 
 * @author
 *  1/20/99 - Created. Kevin Schleicher
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_striper_read_state3(fbe_raid_siots_t * siots_p)
{
    fbe_status_t status;
    fbe_bool_t b_result = FBE_TRUE;
    fbe_queue_head_t *read_head_p = NULL;
    fbe_bool_t b_generate_next_siots = FBE_FALSE;
    fbe_raid_iots_t *iots_p = fbe_raid_siots_get_iots(siots_p);

    fbe_raid_siots_check_traffic_trace(siots_p, FBE_TRUE);

    fbe_raid_siots_get_read_fruts_head(siots_p, &read_head_p);

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

    /* Issue reads to the disks.
     */
    siots_p->wait_count = siots_p->data_disks;

    fbe_raid_siots_transition(siots_p, fbe_striper_read_state4);

    b_result = fbe_raid_fruts_send_chain(read_head_p, fbe_raid_fruts_evaluate);
    if (RAID_FRUTS_COND(b_result != FBE_TRUE))
    {
        /* We hit an issue while starting chain of fruts. We can not immediatelt proceed
         * for siots completion activities as we are not sure how many fruts has been
         * sent successfully.
         */
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "striper: %s failed to start read fruts chain for "
                                            "siots = 0x%p, b_result = %d\n",
                                            __FUNCTION__,
                                            siots_p,
                                            b_result);
        
    }
    if (b_generate_next_siots)
    {
        fbe_raid_iots_generate_next_siots_immediate(iots_p);
    }

    return FBE_RAID_STATE_STATUS_WAITING;
}
/* end of fbe_striper_read_state3() */

/****************************************************************
 * fbe_striper_read_state4()
 ****************************************************************
 * @brief
 *  Reads have completed, check status.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t 
 *
 * @return fbe_status_t either FBE_RAID_STATE_STATUS_EXECUTING
 *                          or FBE_RAID_STATE_STATUS_WAITING
 *
 * @author
 *  1/25/00 - Created. Kevin Schleicher 
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_striper_read_state4(fbe_raid_siots_t * siots_p)
{
    fbe_raid_state_status_t return_status = FBE_RAID_STATE_STATUS_EXECUTING;

    fbe_raid_fru_eboard_t eboard;
    fbe_raid_fru_error_status_t status;

    fbe_raid_fruts_t *read_fruts_p = NULL;
    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_p);

    fbe_raid_fru_eboard_init(&eboard);

    /* Get totals for errored or dead drives.
     */
    status = fbe_striper_get_fruts_error(read_fruts_p, &eboard);

    if (status == FBE_RAID_FRU_ERROR_STATUS_SUCCESS)
    {
        fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
        fbe_raid_group_type_t raid_type;

        fbe_raid_geometry_get_raid_type(raid_geometry_p, &raid_type);
        /* Stage 1: All reads are successful.
         *          Check checksums if necessary.
         *
         *          For Striped mirrors just check status,
         *          as we already checked checksums in the mirror driver.
         */
        if (raid_type == FBE_RAID_GROUP_TYPE_RAID10)
        {
            /* Success, now check for host transfer.
             * Checksums were checked in mirror driver.
             */
            fbe_raid_siots_set_xor_command_status(siots_p,
                                                  FBE_XOR_STATUS_NO_ERROR);
            fbe_raid_siots_transition(siots_p, fbe_striper_read_state6);
        }
        else
        {
            fbe_status_t status;
            /* Check checksums and lba stamps.
             * If we are not requested to check the checksum the
             * utility function will only check the lba stamps.
             */
            fbe_raid_siots_transition(siots_p, fbe_striper_read_state5);

            /* Check the checksums.  If checksums do not need to be 
             * checked, then we will check the lba stamp. 
             */
            status = fbe_raid_xor_read_check_checksum(siots_p, FBE_FALSE, FBE_TRUE);

            if (RAID_COND(status != FBE_STATUS_OK))
            {
                fbe_raid_siots_set_unexpected_error(siots_p, 
                                                    FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                    "Striper: Invalid status %d",
                                                    status);
                return FBE_RAID_STATE_STATUS_EXECUTING;
            }

            return FBE_RAID_STATE_STATUS_EXECUTING;
        }
    }
    else if ((status == FBE_RAID_FRU_ERROR_STATUS_SHUTDOWN) ||
             (status == FBE_RAID_FRU_ERROR_STATUS_DEAD) ||
             (status == FBE_RAID_FRU_ERROR_STATUS_RETRY) ||
             (status == FBE_RAID_FRU_ERROR_STATUS_ABORTED) ||
             (status == FBE_RAID_FRU_ERROR_STATUS_WAITING) || 
             (status == FBE_RAID_FRU_ERROR_STATUS_NOT_PREFERRED) || 
             (status == FBE_RAID_FRU_ERROR_STATUS_REDUCE_QD_HARD) || 
             (status == FBE_RAID_FRU_ERROR_STATUS_REDUCE_QD_SOFT))
    {
        fbe_raid_state_status_t state_status;
        state_status = fbe_striper_handle_fruts_error(siots_p, read_fruts_p, &eboard, status);
        return state_status;
    }
    else if (status == FBE_RAID_FRU_ERROR_STATUS_ERROR)
    {
        if (eboard.hard_media_err_count)
        {
            if (fbe_raid_geometry_is_raid10(fbe_raid_siots_get_raid_geometry(siots_p)) != FBE_TRUE)
            {
                fbe_bool_t      validate;
				/* Read failed due to media error.
				 * Need to invalidate the data for the drive(s) that took the media error.
				 * We will be injecting errors on the mirror level later
				 */
				if (fbe_striper_invalidate_hard_err(siots_p, &eboard) != FBE_STATUS_OK)
				{
					fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
							"striper: unexpected error invalidating on hard error.\n");
					return FBE_RAID_STATE_STATUS_EXECUTING;
				}
                /* In the case of R0/ID, we want to log something to the
                 * event log to indicate what happened here.
                 *
                 * We fill out u_crc to indicate an uncorrectable error,
                 * we fill out media_err to indicate this uncorr error
                 * was due to media errors.
                 */
                siots_p->vrts_ptr->eboard.media_err_bitmap = 
                siots_p->vrts_ptr->eboard.u_crc_bitmap = 
                eboard.hard_media_err_bitmap;

                /*  Call a function that will fill out a error region
                      and call siots record errors
                */
                validate = fbe_raid_siots_is_validation_enabled(siots_p);
                if(validate)
                {
                    fbe_striper_add_media_error_region_and_validate(siots_p, read_fruts_p, eboard.hard_media_err_bitmap);
                }
                
                /* Log errors since we need to tell user about media errors.
                 */
                fbe_striper_report_errors(siots_p, FBE_TRUE);
            }
            fbe_raid_siots_transition(siots_p, fbe_raid_siots_media_error);
            /* After reporting errors, just send Usurper FBE_BLOCK_TRANSPORT_CONTROL_CODE_UPDATE_LOGICAL_ERROR_STATS
             * to PDO for single or multibit CRC errors found 
             */
            if ( (fbe_raid_geometry_is_raid10(fbe_raid_siots_get_raid_geometry(siots_p)) != FBE_TRUE) 
                && (fbe_raid_siots_pdo_usurper_needed(siots_p)))
            {
               fbe_status_t send_crc_status = fbe_raid_siots_send_crc_usurper(siots_p);
               /*We are waiting only when usurper has been sent, else executing should be returned*/
               if (send_crc_status == FBE_STATUS_OK)
               {
                   return FBE_RAID_STATE_STATUS_WAITING;
               }
            }
        }
        else if (eboard.drop_err_count > 0)
        {
            /* At least one read failed due to a DROPPED. */

            fbe_raid_siots_transition(siots_p, fbe_raid_siots_dropped_error);
        }
        else
        {
            /* Raw units should never be able to get FBE_RAID_FRU_ERROR_STATUS_WAITING
             */
            fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                  "striper: fru error status unknown 0x%x\n", status);
            return FBE_RAID_STATE_STATUS_EXECUTING;
        }
    }
    else
    {
        /* Other status values are not expected.
         */
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "striper: status unknown 0x%x\n", status);
        fbe_raid_fru_eboard_display(&eboard, siots_p, FBE_TRACE_LEVEL_ERROR);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }
    return return_status;
}
/* end of fbe_striper_read_state4() */

/****************************************************************
 * fbe_striper_read_state5()
 ****************************************************************
 * @brief
 *  All reads are successful, check the checksums.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t 
 *
 * @return fbe_status_t either FBE_RAID_STATE_STATUS_EXECUTING
 *                          or FBE_RAID_STATE_STATUS_WAITING
 * 
 * @author
 *  1/26/00 - Created. Kevin Schleicher
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_striper_read_state5(fbe_raid_siots_t * siots_p)
{
    fbe_raid_state_status_t status = FBE_RAID_STATE_STATUS_EXECUTING;
    fbe_xor_status_t xor_status;
    fbe_raid_fruts_t *read_fruts_p = NULL;

    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_p);
    fbe_raid_siots_get_xor_command_status(siots_p, &xor_status);

    /* This state does not have appropriate checks for
     * mirror groups, so we should not get here.
     */
    if (RAID_COND(fbe_raid_geometry_is_raid10(fbe_raid_siots_get_raid_geometry(siots_p))))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "Striper: mirror group unexpected\n");

        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* If we either received a checksum error or previously received a checksum error,
     *  then use the retry function.
     */
    if ( (xor_status != FBE_XOR_STATUS_NO_ERROR || fbe_raid_siots_is_csum_retried( siots_p )) &&
         !fbe_raid_csum_handle_csum_error( siots_p, 
                                          read_fruts_p, 
                                          FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                          fbe_striper_read_state4) )
    {
        /* We needed to retry the fruts and the above function
         * did so. We are waiting for the retries to finish.
         * Transition to state 4 to evaluate retries.
         */
        return FBE_RAID_STATE_STATUS_WAITING;
    }
    /* Make note of any retried crc errors we came across.
     */
    fbe_raid_report_retried_crc_errs( siots_p );

    if ( fbe_raid_siots_record_errors(siots_p,
                                      fbe_raid_siots_get_width(siots_p),
                                      (fbe_xor_error_t *) & siots_p->misc_ts.cxts.eboard,
                                      FBE_RAID_FRU_POSITION_BITMASK,
                                      FBE_FALSE,
                                      FBE_TRUE /* Yes also validate. */)
         != FBE_RAID_STATE_STATUS_DONE )
    {
        /* Retries were performed in rg_record errors.
         */
        return FBE_RAID_STATE_STATUS_WAITING;
    }

    if (xor_status == FBE_XOR_STATUS_NO_ERROR)
    {
        /* No checksum error,
         * continue read.
         */
        fbe_raid_siots_transition(siots_p, fbe_striper_read_state6);
    }
    else if (xor_status & FBE_XOR_STATUS_CHECKSUM_ERROR)
    {
        /* Put the errors from this xor op into the 
         * VRTS since this is what report errors uses.
         */
        siots_p->vrts_ptr->eboard = siots_p->misc_ts.cxts.eboard;

        /* Next send messages to the event log for this error.
         */
        fbe_striper_report_errors(siots_p, FBE_TRUE);
        /* Checksum error is detected in the read data,
         * There is nothing we can do about it, exit
         * with error.
         */
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
        /* Unexpected xor status.
         */
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "striper: xor status unknown 0x%x\n", xor_status);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }
    return status;
}
/* end of fbe_striper_read_state5() */

/****************************************************************
 * fbe_striper_read_state6()
 ****************************************************************
 * @brief
 *  All checksums are successful, continue with read operation.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t
 *
 * @return fbe_status_t FBE_RAID_STATE_STATUS_EXECUTING
 * 
 * @author
 *  1/25/00 - Created. Kevin Schleicher
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_striper_read_state6(fbe_raid_siots_t * siots_p)
{
    if (fbe_raid_siots_is_aborted(siots_p))
    {
        /* The iots has been aborted,
         * treat it as a hard error,
         * and bail out now.
         */
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_aborted);

        return FBE_RAID_STATE_STATUS_EXECUTING;
    }
    /* Cache Operation, or a verify we are done.
     */
    fbe_raid_siots_transition(siots_p, fbe_striper_read_state20);

    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/* end of fbe_striper_read_state6() */

/****************************************************************
 * fbe_striper_read_state20()
 ****************************************************************
 * @brief
 *  The reads has completed, return good status.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t
 *
 * @return fbe_status_t FBE_RAID_STATE_STATUS_EXECUTING.
 *
 * @author
 *  1/25/00 - Created. Kevin Schleicher
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_striper_read_state20(fbe_raid_siots_t * siots_p)
{
    fbe_raid_siots_transition(siots_p, fbe_raid_siots_success);

    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/* end of fbe_striper_read_state20() */

/***************************************************
 * End File fbe_striper_read.c
 **************************************************/
