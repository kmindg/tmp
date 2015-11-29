 /***************************************************************************
  * Copyright (C) EMC Corporation 2009
  * All rights reserved.
  * Licensed material -- property of EMC Corporation
  ***************************************************************************/
 
 /*!*************************************************************************
  * @file fbe_parity_util.c
  ***************************************************************************
  *
  * @brief
  *  This file contains generic helper functions for parity units.
  *
  * @version
  *   9/3/2009:  Created. Rob Foley
  *
  ***************************************************************************/
 
 /*************************
  *   INCLUDE FILES
  *************************/
#include "fbe_raid_library_proto.h"
#include "fbe_parity_io_private.h"
#include "fbe_raid_library.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/
/*!**************************************************************
 * fbe_parity_fruts_handle_dead_error()
 ****************************************************************
 * @brief
 *  Handle a fruts getting a dead drive error, which is not
 *  a retryable error.
 *
 * @param siots_p - The current I/O.
 * @param eboard_p - The totals for the errors we have received.
 * @param fru_error_status_p - The error to return to the state machine.               
 *
 * @return fbe_status_t FBE_STATUS_GENERIC_FAILURE for unexpected errors.
 *                      FBE_STATUS_OK otherwise.
 *
 * @author
 *  10/23/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_parity_fruts_handle_dead_error(fbe_raid_siots_t *siots_p,
                                                fbe_raid_fru_eboard_t * eboard_p,
                                                fbe_raid_fru_error_status_t *fru_error_status_p)
{
    fbe_bool_t rg_fru_waiting = FBE_FALSE;
    fbe_bool_t rg_fru_error = FBE_FALSE;
    fbe_bool_t b_continue_received =
        fbe_raid_siots_is_flag_set(siots_p, FBE_RAID_SIOTS_FLAG_RECEIVED_CONTINUE);
    fbe_payload_block_operation_opcode_t opcode;
    
    fbe_raid_siots_get_opcode(siots_p, &opcode);

    /* If we received a continue then we never wait.
     * We will retry any positions that took unretryable errors and are not now degraded. 
     */ 
    if (b_continue_received == FBE_FALSE)
    {
        if (eboard_p->dead_err_bitmap)
        {
            /* We have seen a drive go away and we have not received a continue,
             * so we need to wait. 
             */
            rg_fru_waiting = FBE_TRUE;
        }
    }
    else
    {
        /* We received a continue and thus we will not wait.
         */
        rg_fru_error = FBE_TRUE;

        /* Refresh this siots view of the iots.
         */
        fbe_parity_check_for_degraded(siots_p);
    }

    if (rg_fru_waiting == FBE_TRUE)
    {

        /* We cannot wait on monitor initiated operation because the monitor
         * is waiting for the request to complete and therefore cannot
         * process the new dead position (i.e. restart this request).
         * Thus we must fail monitor initiated operation with a dead error.
         */
        if (fbe_raid_siots_is_monitor_initiated_op(siots_p))
        {
            fbe_raid_iots_t *iots_p = fbe_raid_siots_get_iots(siots_p);

            /* Trace some information and return `dead' status.
             */
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_WARNING,
                                        "%s: iots opcode: 0x%x siots lba: 0x%llx blks: 0x%llx\n",
                                   __FUNCTION__, opcode, 
                                   siots_p->start_lba, siots_p->xfer_count);
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_WARNING,
                                   "parity:  dead Bits: 0x%x siots c_bitmask: 0x%x n_c_bitmask:0x%x new dead bitmask:0x%x\n",
                                   eboard_p->dead_err_bitmap, siots_p->continue_bitmask, 
                                   siots_p->needs_continue_bitmask, 
                                   /* New bits we don't know about. */
                                   (eboard_p->dead_err_bitmap & ~(siots_p->continue_bitmask)));

            *fru_error_status_p = FBE_RAID_FRU_ERROR_STATUS_DEAD;

            /* For media modify monitor operations set INCOMPLETE_WRITE flag 
             * and dead_err_bitmap in IOTS.
             */
            if(fbe_payload_block_operation_opcode_is_media_modify(opcode))
            {
                fbe_raid_iots_set_flag(iots_p, FBE_RAID_IOTS_FLAG_INCOMPLETE_WRITE);

                iots_p->dead_err_bitmap = eboard_p->dead_err_bitmap;
            }
        }
        else
        {
            /* Else mark that a continue is needed for this position.
             * We wait in the calling state until the raid group
             * monitor detects and processes the new dead position.
             */
            siots_p->needs_continue_bitmask |= (eboard_p->dead_err_bitmap & ~(siots_p->continue_bitmask));

            /* There are new drives marked as dead in the group. Return 
             * FBE_RAID_FRU_ERROR_STATUS_WAITING so that this is also
             * placed in the shutdown queue.
             */
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_WARNING,
                                        "parity: wait continue opcode: 0x%x siots lba: 0x%llx blks: 0x%llx siots: %p\n",
                                        opcode, siots_p->start_lba, siots_p->xfer_count, siots_p);
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_WARNING,
                                        "parity:  dead Bits: 0x%x c_b: 0x%x n_c_b: 0x%x nw_dead_b: 0x%x siots: %p\n",
                                        eboard_p->dead_err_bitmap, siots_p->continue_bitmask, 
                                        siots_p->needs_continue_bitmask, 
                                        /* New bits we don't know about. */
                                        (eboard_p->dead_err_bitmap & ~(siots_p->continue_bitmask)),
                                        siots_p);
            /* Return a `waiting' status which will wait for the object to
             * see the drive(s) go dead.
             */
            *fru_error_status_p = FBE_RAID_FRU_ERROR_STATUS_WAITING;
            fbe_raid_siots_set_flag(siots_p, FBE_RAID_SIOTS_FLAG_WAITING_SHUTDOWN_CONTINUE);

        }    /* end else we must wait for monitor */
    }
    else if (rg_fru_error == FBE_TRUE)
    {
        fbe_raid_iots_t *iots_p = fbe_raid_siots_get_iots(siots_p);
        fbe_raid_position_bitmask_t degraded_bitmask;
        fbe_raid_siots_get_degraded_bitmask(siots_p, &degraded_bitmask);

        if (eboard_p->dead_err_bitmap & degraded_bitmask)
        {
            if ((eboard_p->dead_err_bitmap & degraded_bitmask) == eboard_p->dead_err_bitmap)
            {
                /* We have at least one drive marked in our dead bitmap, which is now 
                 * a degraded drive.  Continue this operation degraded. 
                 */
                *fru_error_status_p = FBE_RAID_FRU_ERROR_STATUS_ERROR;
            }
            else
            {
                /* We have dead errors that are now back.  Let's retry them.
                 */ 
                eboard_p->retry_err_bitmap |= eboard_p->dead_err_bitmap & ~degraded_bitmask;
                eboard_p->retry_err_count = fbe_raid_get_bitcount(eboard_p->retry_err_bitmap);
                eboard_p->dead_err_bitmap &= degraded_bitmask;
                eboard_p->dead_err_count = fbe_raid_get_bitcount(eboard_p->dead_err_bitmap);
                *fru_error_status_p = FBE_RAID_FRU_ERROR_STATUS_RETRY;
                fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_WARNING,
                                            "parity: retry dead errors opcode: 0x%x pos_b: 0x%x dead_b: 0x%x siots: %p\n",
                                            opcode, eboard_p->retry_err_bitmap, eboard_p->dead_err_bitmap, siots_p);
            }
        }
        else if (fbe_raid_iots_is_flag_set(iots_p, FBE_RAID_IOTS_FLAG_QUIESCE) &&
                 fbe_raid_siots_is_flag_set(siots_p, FBE_RAID_SIOTS_FLAG_RECEIVED_CONTINUE))
        {
            /* If we are a monitor initiated operation, we cannot wait. 
             * The monitor needs to quiesce and this operation needs to finish before we can quiesce. 
             */
            if (fbe_raid_siots_is_monitor_initiated_op(siots_p))
            {
                *fru_error_status_p = FBE_RAID_FRU_ERROR_STATUS_DEAD;

                /* For media modify monitor operations set INCOMPLETE_WRITE flag 
                 * and dead_err_bitmap in IOTS.
                 */
                if(fbe_payload_block_operation_opcode_is_media_modify(opcode))
                {
                    fbe_raid_iots_set_flag(iots_p, FBE_RAID_IOTS_FLAG_INCOMPLETE_WRITE);

                    iots_p->dead_err_bitmap = eboard_p->dead_err_bitmap;                    
                }
            }
            else
            {
                /* We have an error, but the monitor is trying to quiesce. 
                 * We already received a continue, but need to pick up other changes. 
                 * Just return waiting to give the monitor a chance to quiesce.
                 */
                fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_WARNING,
                                             "parity: quiesce and continue received siots lba: 0x%llx blks: 0x%llx siots: %p",
                                             siots_p->start_lba, siots_p->xfer_count, siots_p);
                *fru_error_status_p = FBE_RAID_FRU_ERROR_STATUS_WAITING;
                fbe_raid_siots_set_flag(siots_p, FBE_RAID_SIOTS_FLAG_WAITING_SHUTDOWN_CONTINUE);
            }
        }
        else
        {
            /* The drive or drives is not known to be degraded.  
             * This drive could have come back. 
             * Retry any dead errors we received. 
             */ 
            eboard_p->retry_err_bitmap |= eboard_p->dead_err_bitmap;
            eboard_p->dead_err_bitmap = 0;
            eboard_p->dead_err_count = 0;
            eboard_p->retry_err_count = fbe_raid_get_bitcount(eboard_p->retry_err_bitmap);
            *fru_error_status_p = FBE_RAID_FRU_ERROR_STATUS_RETRY;
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_WARNING,
                                         "parity: retry dead errors opcode: 0x%x pos_b: 0x%x siots: %p",
                                         opcode, eboard_p->retry_err_bitmap, siots_p);
        }
    }
    else
    {
        /* Else we expected to see either fru error or fru waiting set.
         */
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                              "RAID:  Dead Bits: 0x%x Siots c_bitmask: 0x%x n_c_bitmask:0x%x nw:0x%x\n",
                              eboard_p->dead_err_bitmap, siots_p->continue_bitmask, 
                              siots_p->needs_continue_bitmask, 
                              /* New bits we don't know about. */
                              (eboard_p->dead_err_bitmap & ~(siots_p->continue_bitmask)));
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_parity_fruts_handle_dead_error()
 ******************************************/
 /*!**************************************************************
 * fbe_parity_get_fruts_error()
 ****************************************************************
 * @brief
 *  Determine the overall status of a fruts chain.
 *
 * @param fruts_p - This is the chain to get status on.
 * @param eboard_p - This is the structure to save the results in.
 *
 * @return  fbe_raid_fru_error_status_t
 *
 * @author
 *  8/10/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_raid_fru_error_status_t fbe_parity_get_fruts_error(fbe_raid_fruts_t *fruts_p,
                                                       fbe_raid_fru_eboard_t * eboard_p)
{
    fbe_raid_siots_t *siots_p = fbe_raid_fruts_get_siots(fruts_p);
    if (!fbe_raid_siots_is_flag_set(siots_p, FBE_RAID_SIOTS_FLAG_ERROR_PENDING))
    {
        return FBE_RAID_FRU_ERROR_STATUS_SUCCESS;
    }
    else
    {
        fbe_raid_fru_error_status_t fru_error_status = FBE_RAID_FRU_ERROR_STATUS_SUCCESS;
        fbe_bool_t b_status;
        fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
        fbe_raid_siots_algorithm_e algorithm = siots_p->algorithm;    /* for tracing */

        /* Sum up the errors encountered.
         * We need to do this prior to checking if we are aborted so we can populate the 
         * eboard and then report back these errors. 
         */
        b_status = fbe_raid_fruts_chain_get_eboard(fruts_p, eboard_p);

        if (b_status == FBE_FALSE)
        {
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                        "parity: %s unexpected error count: %d\n",
                                        __FUNCTION__,
                                        eboard_p->unexpected_err_count);


            return FBE_RAID_FRU_ERROR_STATUS_UNEXPECTED;
        }

        if (fbe_raid_siots_is_aborted(siots_p))
        {
            fbe_raid_iots_t *iots_p = fbe_raid_siots_get_iots(siots_p);
            fbe_payload_block_operation_opcode_t opcode;
            fbe_raid_iots_get_opcode(iots_p, &opcode);

            /* Media modify type operations will need to decide for themselves if they want 
             * to abort the request. 
             * Media modify operations are different since always allowing them to 
             * abort at this point could cause parity/mirror inconsistencies. 
             */
            if (!fbe_payload_block_operation_opcode_is_media_modify(opcode))
            {
                if (fbe_raid_geometry_is_debug_flag_set(raid_geometry_p, FBE_RAID_LIBRARY_DEBUG_FLAG_SIOTS_ERROR_TRACING))
                {
                    fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_INFO,
                                                "parity get err: rd aborted siots: 0x%x alg: 0x%x\n", 
                                                fbe_raid_memory_pointer_to_u32(siots_p), siots_p->algorithm );
                    fbe_raid_fru_eboard_display(eboard_p, siots_p, FBE_TRACE_LEVEL_INFO);
                }
                return FBE_RAID_FRU_ERROR_STATUS_ABORTED;
            }
        }

        if ( eboard_p->soft_media_err_count > 0 ||
             (eboard_p->hard_media_err_count > 0 &&
              (eboard_p->menr_err_bitmap != eboard_p->hard_media_err_bitmap)))
        {
            siots_p->media_error_count++;


            /* If we took a media error, Set a flag and when this request completes back to
             * the object, the object will mark the chunk. 
             * For the case of a hard media error, only submit a request to remap if there 
             * were some errors that weren't MENR. 
             */
            /* If we are doing Read only Background verify we will not send it to be remapped
             * since we do not wish to fix errors. 
             * If we are doing a verify also do not send it to be remapped, since it is 
             * already marked. 
             */
            if (!fbe_raid_siots_is_read_only_verify(siots_p) &&
                (siots_p->algorithm != R5_VR))
            {
                fbe_raid_iots_t *iots_p = fbe_raid_siots_get_iots(siots_p);
                fbe_raid_iots_set_flag(iots_p, FBE_RAID_IOTS_FLAG_REMAP_NEEDED);
            }
        }

        if ((eboard_p->hard_media_err_bitmap > 0) ||
            (eboard_p->drop_err_bitmap > 0))
        {
            fru_error_status = FBE_RAID_FRU_ERROR_STATUS_ERROR;
        }

        if (eboard_p->abort_err_count != 0)
        {
            if (fbe_raid_geometry_is_debug_flag_set(raid_geometry_p, FBE_RAID_LIBRARY_DEBUG_FLAG_SIOTS_ERROR_TRACING))
            {
                fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_INFO,
                                            "parity get err: aborted siots: 0x%x alg: 0x%x\n", 
                                            fbe_raid_memory_pointer_to_u32(siots_p), siots_p->algorithm );
                fbe_raid_fru_eboard_display(eboard_p, siots_p, FBE_TRACE_LEVEL_INFO);
            }
            /* If we are aborted, return this status immediately since
             * this entire request is finished.
             */
            return FBE_RAID_FRU_ERROR_STATUS_ABORTED;
        }
        else if ((eboard_p->dead_err_count == 0) && (eboard_p->retry_err_count == 0))
        {
            /* No frus are dead, just return current status.
             */
            return fru_error_status;
        }

        if ((eboard_p->retry_err_count > 0) && 
            fbe_raid_siots_is_flag_set(siots_p, FBE_RAID_SIOTS_FLAG_RECEIVED_CONTINUE))
        {
            fbe_raid_position_bitmask_t degraded_bitmask;

            /* Refresh this siots view of the iots.
             */
            fbe_parity_check_for_degraded(siots_p);

            fbe_raid_siots_get_degraded_bitmask(siots_p, &degraded_bitmask);

            /* Handle case where we were retrying and the drive we retried to is gone.
             * After we get the continue we will turn this "retryable" error into a 
             * dead error if the drive is now degraded.
             */
            if ((eboard_p->retry_err_bitmap & degraded_bitmask) != 0)
            {
                fbe_raid_position_bitmask_t new_dead_bitmap;
                fbe_raid_position_bitmask_t dead_bits_to_set;
                fbe_u32_t new_dead_count;
                fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_WARNING,
                                            "parity: retry positions dead obj, retry_b: 0x%x dead_b: 0x%x siots_p: %p\n",
                                            eboard_p->retry_err_bitmap, eboard_p->dead_err_bitmap,
                                            siots_p);
                /* Determine overlap between retryable errors and known dead positions. 
                 * Then take this bitmap out of the retryable errors, and add it into the dead errors. 
                 */
                new_dead_bitmap = eboard_p->retry_err_bitmap & degraded_bitmask;
                new_dead_count = fbe_raid_get_bitcount(new_dead_bitmap);
                eboard_p->retry_err_bitmap &= ~new_dead_bitmap;
                eboard_p->retry_err_count -= new_dead_count;
                /* Only add in the count of new bits not already set in the dead err bitmap. 
                 * The dead_bits_to_set gives us the bits not already set in the dead_err_bitmap.
                 */
                dead_bits_to_set = (eboard_p->dead_err_bitmap ^ new_dead_bitmap) & new_dead_bitmap;
                eboard_p->dead_err_count += fbe_raid_get_bitcount(dead_bits_to_set);
                eboard_p->dead_err_bitmap |= new_dead_bitmap;
            }
        }

        if (eboard_p->dead_err_count > 0)
        {
            fbe_status_t status;
            fbe_raid_position_bitmask_t degraded_bitmask;
            fbe_raid_position_bitmask_t rl_bitmask;
            fbe_raid_siots_get_degraded_bitmask(siots_p, &degraded_bitmask);
            fbe_raid_siots_get_rebuild_logging_bitmask(siots_p, &rl_bitmask);

            /* Not shutdown, we can continue to evaluate.
             */
            fru_error_status = FBE_RAID_FRU_ERROR_STATUS_ERROR;
            status = fbe_parity_fruts_handle_dead_error(siots_p, eboard_p, &fru_error_status);
            if (status != FBE_STATUS_OK)
            {
                return FBE_RAID_FRU_ERROR_STATUS_UNEXPECTED;
            }

            /* We can't trace when we're waiting since the siots might be freed as soon as we set waiting.
             */
            if (fru_error_status != FBE_RAID_FRU_ERROR_STATUS_WAITING)
            {
                fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_WARNING,
                                            "parity dead err siots: %p alg: 0x%x dead_b:0x%x/0x%x rl_b: 0x%x fst: %d\n",
                                            siots_p, algorithm,
                                            eboard_p->dead_err_bitmap, degraded_bitmask, rl_bitmask, fru_error_status);
            }

            /* Note that if our retry count is not zero, but we are waiting, then we will 
             * not execute the retry right now.  We will wait for the continue before proceeding with the retry. 
             */
            if ((eboard_p->retry_err_count > 0) && (fru_error_status == FBE_RAID_FRU_ERROR_STATUS_ERROR))
            {
                /* In this case we had a drive die and got a continue, but we also have 
                 * retries to do.  We want to do the retries first, so return retry.
                 */
                fru_error_status = FBE_RAID_FRU_ERROR_STATUS_RETRY;
                fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_WARNING,
                                            "parity:retryable error count: %d mask: 0x%x "
                                            "dead count: %d mask: 0x%x siots: %p\n",
                                            eboard_p->retry_err_count, eboard_p->retry_err_bitmap,
                                            eboard_p->dead_err_count, eboard_p->dead_err_bitmap, siots_p);
            }
            
        }
        else if (eboard_p->retry_err_count > 0)
        {
            fbe_raid_iots_t *iots_p = fbe_raid_siots_get_iots(siots_p);
            if (fbe_raid_iots_is_flag_set(iots_p, FBE_RAID_IOTS_FLAG_QUIESCE))
            {
                if(fbe_raid_siots_is_monitor_initiated_op(siots_p))
                {
                    fbe_payload_block_operation_opcode_t opcode;
                    fbe_raid_iots_get_opcode(iots_p, &opcode);
                    if(fbe_payload_block_operation_opcode_is_media_modify(opcode))
                    {
                        fbe_raid_iots_set_flag(iots_p, FBE_RAID_IOTS_FLAG_INCOMPLETE_WRITE);

                        /* we treat the retryable error as dead error here */
                        iots_p->dead_err_bitmap = eboard_p->retry_err_bitmap;
                    }

                    fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_WARNING,
                                                "parity: siots: 0x%x metadata retry_b: 0x%x return dead on quiesce.\n",
                                                fbe_raid_memory_pointer_to_u32(siots_p),
                                                eboard_p->retry_err_bitmap);
                    fru_error_status = FBE_RAID_FRU_ERROR_STATUS_DEAD;
                }
                else
                {
                    /* If we have a retryable error and we are quiescing, then we should not 
                     * execute the retry now.  Simply wait for the continue. 
                     */
                    fbe_raid_iots_object_trace(iots_p, FBE_RAID_IOTS_TRACE_PARAMS_WARNING, 
                                               "parity: retry err wait quiesce siots: 0x%x errs: %d mask: 0x%x\n",
                                               fbe_raid_memory_pointer_to_u32(siots_p),
                                               eboard_p->retry_err_count, eboard_p->retry_err_bitmap);
                    fbe_raid_siots_set_flag(siots_p, FBE_RAID_SIOTS_FLAG_QUIESCED);
                    fru_error_status = FBE_RAID_FRU_ERROR_STATUS_WAITING;
                }
            }
            else
            {
                /* Otherwise perform the retry.
                 */
                fru_error_status = FBE_RAID_FRU_ERROR_STATUS_RETRY;
                fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_WARNING,
                                            "parity: retryable err. siots: 0x%x cnt: %d mask: 0x%x\n",
                                            fbe_raid_memory_pointer_to_u32(siots_p),
                                            eboard_p->retry_err_count,
                                            eboard_p->retry_err_bitmap);
            }
        }
        else
        {
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                        "parity: %s: err count unexpected retry_c: %d mask: 0x%x dead_c: %d mask: 0x%x\n",
                                        __FUNCTION__,
                                        eboard_p->retry_err_count,
                                        eboard_p->retry_err_bitmap,
                                        eboard_p->dead_err_count,
                                        eboard_p->dead_err_bitmap);
        }

        if ((fru_error_status != FBE_RAID_FRU_ERROR_STATUS_WAITING) &&
            fbe_raid_geometry_is_debug_flag_set(raid_geometry_p, FBE_RAID_LIBRARY_DEBUG_FLAG_SIOTS_ERROR_TRACING))
        {
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_INFO,
                                        "parity get error: siots: 0x%x fru_err_status: %d alg: 0x%x\n", 
                                        fbe_raid_memory_pointer_to_u32(siots_p), fru_error_status, algorithm );
            fbe_raid_fru_eboard_display(eboard_p, siots_p, FBE_TRACE_LEVEL_INFO);
        }
        return fru_error_status;
    }
}
/******************************************
 * end fbe_parity_get_fruts_error()
 ******************************************/
/*************************************************************
 * fbe_parity_siots_is_degraded
 *************************************************************
 * @brief
 *  If we are now equal or beyond the rebuild checkpoint
 *  then we are degraded.
 *
 *  We need a parity start and parity count because depending
 *  on where we are called from, these values come from
 *  a different place in the SIOTS.
 *
 *  If we are in a mirror or hot spare driver, then we
 *  would get these values from the start_lba and xfer_count
 *
 *  On a parity unit, these values come from parity_start and parity_count.
 *    
 *  @param siots_p, A ptr to the current SUB_IOTS to deallocate.
 *
 *  @param parity_start, Start of the region being compared to rb offset.
 *
 *  @param parity_count, Length of the range.
 *
 * @return:
 *
 *  fbe_bool_t - FBE_TRUE means we are degraded.
 *         FBE_FALSE means not degraded.
 *
 * @author
 *  11/4/2009 - Created. Rob Foley
 * 
 *************************************************************/
fbe_bool_t fbe_parity_siots_is_degraded(fbe_raid_siots_t *const siots_p)
{
    fbe_status_t status;
    fbe_bool_t b_return = FBE_FALSE;

    /* The below will check if we are degraded.
     * We're not allowed to do rebuild logging here since generate would have checked this 
     * previously. 
     */
    status = fbe_parity_check_for_degraded(siots_p);

    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: unexpected check degraded status = %d, siots_p = 0x%p\n",
                             status, siots_p);
        return status;
    }

    /* We used to look at the FBE_RAID_SIOTS_FLAG_REBUILD_LOG_CLEAN flag,
     * but this is not used anymore since we do not have checkpoints. 
     */
    if ( fbe_raid_siots_dead_pos_get( siots_p, FBE_RAID_FIRST_DEAD_POS ) != FBE_RAID_INVALID_DEAD_POS )
    {
        /* Either we are not degraded or we are clean.
         */
            b_return = FBE_TRUE;
        }
        else
        {
            /* We are not degraded.
             */
            if (RAID_COND(fbe_raid_siots_dead_pos_get( siots_p, FBE_RAID_FIRST_DEAD_POS ) !=
                          FBE_RAID_INVALID_DEAD_POS))
            {
                fbe_raid_siots_trace(siots_p,
                                     FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                     "parity: siots dead position 0x%x != invalid dead position 0x%x\n",
                                     fbe_raid_siots_dead_pos_get( siots_p, FBE_RAID_FIRST_DEAD_POS ),
                                     FBE_RAID_INVALID_DEAD_POS);
            }
    }
    return b_return;
}
/* end fbe_parity_siots_is_degraded */

/*!**************************************************************
 * fbe_parity_handle_fruts_error()
 ****************************************************************
 * @brief
 *  Handle a common fruts error status.
 * 
 * @param siots_p - current I/O.
 * @param fruts_p - This is the chain to get status on.
 * @param eboard_p - This is the structure to save the results in.
 * @param fru_error_status - This is the status we need to handle.
 *
 * @return fbe_raid_state_status_t FBE_RAID_STATE_STATUS_EXECUTING or
 *                                 FBE_RAID_STATE_STATUS_WAITING
 * @author
 *  2/10/2011 - Created. Rob Foley
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_parity_handle_fruts_error(fbe_raid_siots_t *siots_p,
                                                      fbe_raid_fruts_t *fruts_p,
                                                      fbe_raid_fru_eboard_t * eboard_p,
                                                      fbe_raid_fru_error_status_t fruts_error_status)
{
    if (fruts_error_status == FBE_RAID_FRU_ERROR_STATUS_SHUTDOWN)
    {
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_shutdown_error);
    }
    else if (fruts_error_status == FBE_RAID_FRU_ERROR_STATUS_DEAD)
    {
        /* Metadata operation hit a dead error.
         */
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_dead_error);
    }
    else if (fruts_error_status == FBE_RAID_FRU_ERROR_STATUS_RETRY)
    {
        fbe_status_t status;
        /* Send out retries.  We will stop retrying if we get aborted or 
         * our timeout expires or if an edge goes away. 
         */
        status = fbe_raid_siots_retry_fruts_chain(siots_p, eboard_p, fruts_p);
        if (status != FBE_STATUS_OK)
        {
            fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "retry %d errors (bitmap 0x%x) status: %d unexpected\n", 
                                                eboard_p->retry_err_count, eboard_p->retry_err_bitmap, status);
            fbe_raid_fru_eboard_display(eboard_p, siots_p, FBE_TRACE_LEVEL_ERROR);
            return FBE_RAID_STATE_STATUS_EXECUTING;
        }
        return FBE_RAID_STATE_STATUS_WAITING;
    }
    else if (fruts_error_status == FBE_RAID_FRU_ERROR_STATUS_WAITING)
    {
        /* Some number of drives are not available.  Wait for monitor to make a decision.
         */
        return FBE_RAID_STATE_STATUS_WAITING;
    }
    else if (fbe_raid_siots_is_aborted(siots_p))
    {
        /* The siots has been aborted.
         */
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_aborted);
    }
    else
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "%s fru unexpected status: 0x%x\n", 
                                            __FUNCTION__, fruts_error_status);
        fbe_raid_fru_eboard_display(eboard_p, siots_p, FBE_TRACE_LEVEL_ERROR);
    }
    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/**************************************
 * end fbe_parity_handle_fruts_error()
 **************************************/
 /*************************
  * end file fbe_parity_util.c
  *************************/
 
 
