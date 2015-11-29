/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_raid_csum_util.c
 ***************************************************************************
 *
 * @brief
 *  This file contains utilities for handling raid checksum errors.
 *
 * @version
 *   9/10/2009  Ron Proulx  - Created from rg_csum_error_retry.c
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe_raid_library_proto.h"

/********************************
 *  static STRING LITERALS
 ********************************/

/*****************************************************
 *  static FUNCTIONS DECLARED staticLY FOR VISIBILITY
 *****************************************************/

static fbe_status_t fbe_raid_csum_error_retry_finished( fbe_raid_siots_t * const siots_p,
                                         fbe_raid_fruts_t *const fruts_p ) ;

static fbe_u16_t fbe_raid_csum_get_retry_bitmask( fbe_raid_siots_t * const siots_p,
                                            fbe_raid_fruts_t *const fruts_p );

static fbe_status_t fbe_raid_csum_error_start_retries( fbe_raid_siots_t * const siots_p,
                                                       fbe_raid_fruts_t *const fruts_p,
                                                       fbe_u32_t opcode,
                                                       fbe_raid_siots_state_t retry_state );

/*!***************************************************************
 *      fbe_raid_csum_handle_csum_error()
 ****************************************************************
 * @brief
 *  This function will handle checksum error retries.
 *  If a checksum error retry needs to occur, then we start the
 *  retry here.
 *
 *  If a checksum error retry is not necessary, then 
 *  FBE_RAID_STATE_STATUS_DONE is returned.
 *
 * @param   siots_p - ptr to the SIOTS
 * @param   fruts_p - ptr to the fruts that detected checksum error
 * @param   opcode - opcode to retry if retries are necessary.
 * @param  retry_state - The state to transition siots to for any retry.
 *
 * @note
 *  We assume that a checksum check operation has just occurred.
 *  The status of this operation is in the siots->cxts.eboard.
 *
 * @return
 *  fbe_bool_t - FBE_TRUE means there was nothing for this function to do, 
 *         so the caller must process the checksum error
 *
 *         FBE_FALSE - Means that we have initiated retries and the
 *         caller should wait for the retry to complete.
 * 
 * @author
 *  07/25/05 - Created. Rob Foley
 *  09/10/2009  Ron Proulx  - Created from rg_handle_csum_error
 *
 ****************************************************************/

fbe_bool_t fbe_raid_csum_handle_csum_error(fbe_raid_siots_t *const siots_p,
                                           fbe_raid_fruts_t *const fruts_p,
                                           fbe_payload_block_operation_opcode_t opcode,
                                           fbe_raid_siots_state_t retry_state)
{
    fbe_bool_t status = FBE_TRUE;

    /* Get a bitmask of every position that took a retryable
     * error during our last XOR driver operation.
     */
    fbe_u16_t csum_errors_bitmask = fbe_raid_csum_get_error_bitmask( siots_p );
    
    /* Update the media error bitmask for this fruts.
     * This code shouldn't know if this was done previously, 
     * it may have been but we just don't know for sure.
     */
    (void) fbe_raid_fruts_get_media_err_bitmap(
        fruts_p,
        &siots_p->misc_ts.cxts.eboard.hard_media_err_bitmap);
    
    if ( fbe_raid_siots_is_retry( siots_p ) )
    {
        fbe_status_t state_status = FBE_STATUS_OK;
        /* We previously started a retry.  
         * Check the status of the check csum operation.
         */
        state_status = fbe_raid_csum_error_retry_finished( siots_p, fruts_p );
        if(RAID_COND(state_status != FBE_STATUS_OK) )
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                "Checksum operation retry finished with errors, siots: 0x%p \n", siots_p);
            /* 
             * Need to return FBE_TRUE, after clearing the flags. Returns at the end of function
             */
        }

        /* Clear the flag, since the retry is over.
         */
        fbe_raid_common_clear_flag(&siots_p->common, FBE_RAID_COMMON_FLAG_REQ_RETRIED);
    }
    else if (siots_p->misc_ts.cxts.eboard.hard_media_err_bitmap ==
             csum_errors_bitmask)
    {
        /* CRC errors and hard errors are reported by XOR using the crc
         * bitmasks.  We only want to retry if there was at least one
         * CRC error.
         */
    }
    else if ( csum_errors_bitmask != 0 )
    {
        /* We took errors for the first time.
         * Begin retries with FUA bit set.
         */
        status = fbe_raid_csum_error_start_retries( siots_p, fruts_p, opcode, retry_state );
    }

    return status;
}
/*****************************************
 * end fbe_raid_csum_handle_csum_error() 
 *****************************************/

/****************************************************************
 * fbe_raid_csum_error_start_retries()
 ****************************************************************
 * @brief
 *  This function will handle checksum error retries.
 *  If a checksum error retry needs to occur, then we start the
 *  retry here.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t
 * @param fruts_p - ptr to the fbe_raid_fruts_t
 * @param opcode - opcode to retry if retries are necessary.
 *
 * @notes
 *  We assume that a checksum check operation has just occurred.
 *  The status of this operation is in the siots->cxts.eboard.
 *
 * @return
 *  fbe_status_t - FBE_TRUE means there was nothing for
 *                 this function to do, so the caller
 *                 may continue executing.
 *
 *                 Any other return value means that this
 *                 function had something to do, so
 *                 the caller needs to return this status.
 *
 * @author
 *  09/19/05 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_bool_t fbe_raid_csum_error_start_retries( fbe_raid_siots_t * const siots_p,
                                                     fbe_raid_fruts_t *const fruts_p,
                                                     fbe_u32_t opcode,
                                                     fbe_raid_siots_state_t retry_state )
{
    fbe_bool_t status;
    fbe_status_t fbe_status = FBE_STATUS_OK;

    /* Get a bitmask of every position that took a retryable
     * error during our last XOR driver operation.
     */
    fbe_u16_t csum_errors_bitmask = fbe_raid_csum_get_retry_bitmask( siots_p, fruts_p );

    /* Bitmap position set if fruts for this position exists in fruts chain.
     */
    fbe_u16_t active_fruts_bitmap;
    fbe_raid_fruts_get_chain_bitmap( fruts_p, &active_fruts_bitmap );

    /* Clear the bits of any positions that have errors,
     * and so will be invalidated, but are not in the chain.
     * Cleared because we cannot retry these positions since
     * they are not in the chain.
     */
    csum_errors_bitmask &= active_fruts_bitmap;

    /* Update the wait count for all the reads we will be retrying.
     */
    siots_p->wait_count = fbe_raid_get_bitcount( csum_errors_bitmask );
        
    /* If we have anything to retry, then wait count is not zero.
     */
    if ( siots_p->wait_count > 0 )
    {
        fbe_u16_t retry_bitmask;
        
        /* Keep track of the errors that need to be retried.
         * We will use this when the retry finishes to determine which
         * positions to log as successfully retrying.
         * The reason we need this mask at all is that there are cases
         * where we need to retry more positions than took errors
         * (like coh, ts, ws errors), since xor logs errors against
         * parity in cases where there is ambiguity about which
         * position took an error.
         */
        siots_p->vrts_ptr->current_crc_retry_bitmask = 
            fbe_raid_csum_get_error_bitmask( siots_p );

        /* Take out any positions that were not active, since
         * we do not want to consider these as retried.
         * This prevents us from logging parity or dead position as
         * retried if it's not in the chain and we marked it bad as part of
         * the XOR driver operation.
         * For R6 we do not take out bits, we allow errors to get logged
         * against the rebuilding drive.  This is necessary since there are
         * cases where we are allowed to have errors we wish to log against the
         * rebuild or dead position.
         */
        if ( !fbe_raid_geometry_is_raid6(fbe_raid_siots_get_raid_geometry(siots_p)) || 
             (siots_p->vrts_ptr->current_crc_retry_bitmask & active_fruts_bitmap) != 0 )
        {
            siots_p->vrts_ptr->current_crc_retry_bitmask &= active_fruts_bitmap;
        }
        else if ((siots_p->vrts_ptr->current_crc_retry_bitmask & active_fruts_bitmap) == 0)
        {
            /* For R6, do not clear the active fruts bitmap from the retry bitmask.
             * Leave the errors set and just log errors against rebuilding positions.
             * There are certain R6 errors that must be logged against the rebuilding pos.
             */
        }
        
        /* A checksum error occurred, but we have not yet done a checksum error retry.
         *
         * Set the checksum error retry flag because we are going to start a retry now.
         * Later when we come through this path again,
         * we need to know if we did the retry already.
         */
        fbe_raid_common_set_flag(&siots_p->common, FBE_RAID_COMMON_FLAG_REQ_RETRIED);

        /* Bitmask of positions to retry. 
         * This includes any errors as well as any modified positions.
         */
        retry_bitmask = csum_errors_bitmask | siots_p->misc_ts.cxts.eboard.w_bitmap;

        retry_bitmask &= active_fruts_bitmap;
        siots_p->wait_count = fbe_raid_get_bitcount( retry_bitmask );

        /* Once we do retries because of meta-data errors, any further hard error
         * should not be retried, and should drop us into single-sector mode.
         * Change the flags to cause this.
         */
        fbe_raid_siots_set_flag(siots_p, FBE_RAID_SIOTS_FLAG_SINGLE_ERROR_RECOVERY);
        fbe_raid_siots_transition(siots_p, retry_state);

        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_WARNING,
                                    "raid csum retry siots: 0x%x pos_b: 0x%x err_b: 0x%x w_b: 0x%x crc_b: 0x%x wsts_b: 0x%x coh_b: 0x%x\n",
                                    fbe_raid_memory_pointer_to_u32(siots_p),
                                    retry_bitmask, csum_errors_bitmask, 
                                    siots_p->misc_ts.cxts.eboard.w_bitmap,
                                    fbe_xor_lib_error_get_crc_bitmask(&siots_p->misc_ts.cxts.eboard),
                                    fbe_xor_lib_error_get_wsts_bitmask(&siots_p->misc_ts.cxts.eboard),
                                    fbe_xor_lib_error_get_coh_bitmask(&siots_p->misc_ts.cxts.eboard));
        /* Send out retries for all hard errd fruts.
         */
        fbe_status = fbe_raid_fruts_for_each( 
            retry_bitmask,  
            fruts_p,              /* Fruts chain to retry for. */
            fbe_raid_fruts_retry_crc_error, /* Function to call for each fruts. */
            opcode, /* Opcode to retry as. */
            (fbe_u64_t) fbe_raid_fruts_evaluate); /* New evaluate fn. */
        
        if (RAID_FRUTS_COND(fbe_status != FBE_STATUS_OK))
        {
            fbe_raid_siots_trace(siots_p, 
                                 FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "raid: %s failed to retry crc error: fruts_p 0x%p "
                                 "siots_p 0x%p. status = 0x%x\n",
                                 __FUNCTION__,
                                 fruts_p, siots_p, fbe_status);
            return FBE_FALSE;
        }

        /* Return FALSE so the caller knows not to continue.
         */
        status = FBE_FALSE;
    }
    else
    {
        /* Wait count will be zero in cases where parity was invalidated
         * due to non crc errors on other drives.
         */
        status = FBE_TRUE;
    }

    return status;
}
/*****************************************
 * end fbe_raid_csum_error_start_retries()
 *****************************************/

/****************************************************************
 * fbe_raid_csum_error_retry_finished()
 ****************************************************************
 * @brief
 *  This function handles the checksum error retry being finished.
 *  We inspect the status of the retries and log
 *  the results in the siots_p->vrts_p->retried_crc_bitmask.
 *
 *  Later, this bitmask will be inspected for logging purposes.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t
 * @param fruts_p - ptr to the fbe_raid_fruts_t
 *
 * @notes
 *  We assume a set of reads have been retried due to checksum
 *  errors.
 *
 *  We assume that a checksum check operation has just occurred.
 *  The status of this operation is in the siots->cxts.eboard struct.
 *
 * @return
 *  none
 *
 * @author
 *  07/27/05 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_raid_csum_error_retry_finished( fbe_raid_siots_t * const siots_p,
                                          fbe_raid_fruts_t *const fruts_p )
{
    fbe_status_t fbe_status = FBE_STATUS_OK;

    /* Bitmap position set if fruts for this position exists in fruts chain.
     */
    fbe_u16_t active_fruts_bitmap;

    /* Get a bitmask of every position that took a correctable or uncorrectable
     * error during our last XOR driver operation.
     * Checksum errors are the primary case we expect to catch a drive
     * corrupting data.  
     * Note that we also consider stamp errors in the ts, ws, and ss fields,
     * since the stamps also can be corrupted by the drive.
     * Coherency errors are also considered since multiple bit flips will cause
     * a coherency error.
     */
    fbe_u16_t csum_errors_bitmask = fbe_raid_csum_get_error_bitmask( siots_p );
    
    fbe_u32_t retried_count;
        
    /* This bitmask has a 1 in it for every position in the width.
     * For example: width of 5 (1 << 5) = 0x20.  0x20 - 1 = 0x1F
     */
    fbe_u32_t width_bitmask = (1 << fbe_raid_siots_get_width(siots_p)) - 1;

    /* Bitmask of positions retried. 
     */
    fbe_u16_t retried_bitmap = siots_p->vrts_ptr->current_crc_retry_bitmask;
        
    /* Determine how many positions did a retry.
     */
    retried_count = fbe_raid_get_bitcount( retried_bitmap );
        
    if (retried_count == 0)
    {
        /* Something is out of sync.  We assume if the RSUB_CSUM_ERROR_RETRY
         * flag is set in the siots, then we must have
         * retried some fruts.
         */
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "retried count is zero algorithm %d width_b: 0x%x csum_errs_b: 0x%x retried_b: 0x%x",
                             siots_p->algorithm, width_bitmask, csum_errors_bitmask, retried_bitmap);
    }

    /* In order to calculate siots_p->vrts_p->retried_crc_bitmask,
     * we need to get a bitmask of all positions that:
     * a) were retried
     * b) did not take a checksum error after retry.
     *
     * The algorithm we use is to xor the retried bitmap
     * with the crc_errors_bitmap.  Table below proves why this works, and
     * illustrates for each possible bit value, what our algorithm gives.
     *
     * 0 = good crc since the bit is not set in crc_errors_bitmap
     * 1 = bad crc since the bit is set to indicate crc err in bitmap
     *
     *                                        retried_bitmap ^
     * retried_bitmap     crc_errors_bitmap   crc_errors_bitmap
     *  1 (yes retried)         0 (good)           1 ^ 0 = 1
     *  1 (yes retried)         1 (bad crc)        1 ^ 1 = 0
     *  0 (not retried)         0 (good)           0 ^ 0 = 0
     *  0 (not retried)         1 (bad)            0 ^ 1 = 1 ** (see below)
     *
     * The last case above will not occur, as all crc errors were retried.
     * For consistency sake, we will exclude the following case by forcing
     * all not retried positions to look like they have good xsums.
     */

    /* Clear the bits of any positions that have errors,
     * and so will be invalidated, but are not in the chain (like parity).
     * Cleared because we cannot retry these positions since
     * they are not in the chain.
     */
    fbe_raid_fruts_get_chain_bitmap( fruts_p, &active_fruts_bitmap );

    /* Raid 6 allows errors against parity and rebuilding positions.
     */
    if ( !fbe_raid_geometry_is_raid6(fbe_raid_siots_get_raid_geometry(siots_p)) || 
         (csum_errors_bitmask & active_fruts_bitmap) != 0 )
    {
        csum_errors_bitmask &= active_fruts_bitmap;
    }
    else if ((csum_errors_bitmask & active_fruts_bitmap) == 0)
    {
        /* For R6, do not clear the active fruts bitmap from the retry bitmask.
         * Leave the errors set and just log errors against rebuilding positions.
         * There are certain R6 errors that must be logged against the rebuilding pos.
         */
    }

    /* We will assume for calculations that all non-retried
     * positions did not take crc errors.  Change crc_errors_bitmask to reflect this
     * by clearing all non-retried positions of crc errors.
     */
    csum_errors_bitmask &= ~(retried_bitmap ^ width_bitmask);

    /* We should have allocated a vrts ptr in the state machine prior.
     */
    if(RAID_COND( siots_p->vrts_ptr == NULL ) )
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "vrts structure 0x%p is not allocated\n",
                             siots_p->vrts_ptr);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /* Perform calculation described above.
     */
    if ( siots_p->vrts_ptr != NULL )
    {
        /* Add on any new retried crc errors to the existing errors in our bitmask.  
         */
        siots_p->vrts_ptr->retried_crc_bitmask |= retried_bitmap ^ csum_errors_bitmask;
    }
    
    return fbe_status;
}
/*****************************************
 * end fbe_raid_csum_error_retry_finished() 
 *****************************************/

/****************************************************************
 * fbe_raid_report_retried_crc_errs()
 ****************************************************************
 * @brief
 *  This function handles the reporting of retried crc errs.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t
 *
 * ASSUMPTIONS:
 *  We make no assumption about whether or not a set of reads 
 *  have been retried due to checksum errors.  We will check
 *  to see if retries have been successful and report the
 *  appropriate errors.
 *
 * @return
 *  none
 *
 * @author
 *  08/17/05 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_raid_report_retried_crc_errs( fbe_raid_siots_t * const siots_p )
{
    
    /* Normally we don't handle errors within the read state machine.
     * However, if we are degraded, we retry crc errors here.
     */
    if ( siots_p->vrts_ptr->retried_crc_bitmask )
    {
        /* Report these errors to the event log.
         */
        fbe_raid_report_errors( siots_p );
    }
    return;
}
/*****************************************
 * end fbe_raid_report_retried_crc_errs() 
 *****************************************/

/****************************************************************
 * fbe_raid_csum_get_error_bitmask()
 ****************************************************************
 * @brief
 *  This function returns a mask of any error that we 
 *  should keep track of.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t 
 *
 * ASSUMPTIONS:
 *  The assumption is that the cxts eboard has been filled in
 *  by the xor driver as part of an xor/csum operation.
 *
 * @return
 *  fbe_u16_t - mask of errors that can be retried.
 *
 * @author
 *  09/19/05 - Created. Rob Foley
 *
 ****************************************************************/

fbe_u16_t fbe_raid_csum_get_error_bitmask(fbe_raid_siots_t * const siots_p )
{
    return fbe_xor_lib_error_get_total_bitmask(&siots_p->misc_ts.cxts.eboard);
}
/*****************************************
 * end fbe_raid_csum_get_error_bitmask()
 *****************************************/

/****************************************************************
 * fbe_raid_csum_get_retry_bitmask()
 ****************************************************************
 * @brief
 *  This function returns a mask of any error that we 
 *  are allowed to retry.
 *

 * @param siots_p - ptr to the fbe_raid_siots_t
 * @param fruts_p - ptr to the fbe_raid_fruts_t
 *
 * ASSUMPTIONS:
 *  The assumption is that the cxts eboard has been filled in
 *  by the xor driver as part of an xor/csum operation.
 *
 * @return
 *  fbe_u16_t - mask of errors that can be retried.
 *
 * @author
 *  09/19/05 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_u16_t fbe_raid_csum_get_retry_bitmask( fbe_raid_siots_t * const siots_p,
                                            fbe_raid_fruts_t *const fruts_p )
{
    fbe_u16_t retryable_bitmask = fbe_raid_csum_get_error_bitmask( siots_p );

    /* If coherency errors were detected, then we know that all positions are
     * equally likley to have taken multi-bit errors.
     *
     * Similarly if stamp errors were detected, more than just the errored
     * position is likley to have taken an error.
     *
     * Shed stamps are only retried on the position they occurred on,
     * since a shed stamp error logged on one position does not indicate an 
     * error on any other position.
     *
     * Add all possible positions to the bitmask, so that they are all retried.
     */
    if ( (siots_p->misc_ts.cxts.eboard.c_ws_bitmap  |
        siots_p->misc_ts.cxts.eboard.u_ws_bitmap  |
        siots_p->misc_ts.cxts.eboard.c_ts_bitmap  |
        siots_p->misc_ts.cxts.eboard.u_ts_bitmap  |
        siots_p->misc_ts.cxts.eboard.c_coh_bitmap |
          siots_p->misc_ts.cxts.eboard.u_coh_bitmap |
          siots_p->misc_ts.cxts.eboard.c_n_poc_coh_bitmap |
          siots_p->misc_ts.cxts.eboard.u_n_poc_coh_bitmap |
          siots_p->misc_ts.cxts.eboard.c_poc_coh_bitmap   |
          siots_p->misc_ts.cxts.eboard.u_poc_coh_bitmap   |
          siots_p->misc_ts.cxts.eboard.c_coh_unk_bitmap   |
          siots_p->misc_ts.cxts.eboard.u_coh_unk_bitmap) ||
         ( fbe_raid_geometry_is_raid6(fbe_raid_siots_get_raid_geometry(siots_p)) &&
           fbe_raid_siots_dead_pos_get(siots_p, FBE_RAID_FIRST_DEAD_POS) != 
           FBE_RAID_INVALID_DEAD_POS ) )
    {
        fbe_u16_t active_fruts_bitmap;
        fbe_raid_fruts_get_chain_bitmap( fruts_p, &active_fruts_bitmap );

        retryable_bitmask |= active_fruts_bitmap;
    }
    return retryable_bitmask;
}
/*****************************************
 * end fbe_raid_csum_get_retry_bitmask()
 *****************************************/

/*****************************************
 * end fbe_raid_csum_util.c() 
 *****************************************/
