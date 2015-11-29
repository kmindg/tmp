/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_raid_check_zeroed.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the state functions of the check zero state machine.
 *  This state machine allows us to determine if a given stripe is zeroed or not during rebuild.
 * 
 *  If the stripe is zeroed, then we will issue a zero request to the rebuilding positions.
 * 
 *  This is needed in order to optimize certain state machines such as rebuild so
 *  that we do not try to rebuild large chunks of zeros.
 *
 * @notes
 *
 * @author
 *  9/20/2010 - Created. Rob Foley
 *
 ***************************************************************************/


/*************************
 ** INCLUDE FILES
 *************************/
#include "fbe_raid_library_proto.h"

/********************************
 *      static STRING LITERALS
 ********************************/

/*****************************************************
 *      static FUNCTIONS DECLARED GLOBALLY FOR VISIBILITY
 *****************************************************/
static fbe_status_t fbe_raid_check_zeroed_get_fruts_info(fbe_raid_siots_t * siots_p,
                                                         fbe_raid_fru_info_t *write_p);
static fbe_status_t fbe_raid_check_zeroed_setup_fruts(fbe_raid_siots_t * const siots_p,
                                                      fbe_raid_fru_info_t *write_p,
                                                      fbe_raid_memory_info_t *memory_info_p);
static fbe_status_t fbe_raid_check_zeroed_calculate_memory_size(fbe_raid_siots_t *siots_p,
                                                                fbe_raid_fru_info_t *write_info_p);
static fbe_status_t fbe_raid_check_zeroed_setup_resources(fbe_raid_siots_t * siots_p,
                                                                     fbe_raid_fru_info_t *write_p);
static fbe_raid_fru_error_status_t fbe_raid_check_zeroed_get_fruts_error(fbe_raid_fruts_t *fruts_p,
                                                                         fbe_raid_fru_eboard_t * eboard_p);
static fbe_raid_state_status_t fbe_raid_check_zeroed_state1(fbe_raid_siots_t *siots_p);
static fbe_raid_state_status_t fbe_raid_check_zeroed_state2(fbe_raid_siots_t *siots_p);
static fbe_raid_state_status_t fbe_raid_check_zeroed_state3(fbe_raid_siots_t *siots_p);
static fbe_raid_state_status_t fbe_raid_check_zeroed_state4(fbe_raid_siots_t *siots_p);
static fbe_raid_state_status_t fbe_raid_check_zeroed_state5(fbe_raid_siots_t *siots_p);
static fbe_raid_state_status_t fbe_raid_check_zeroed_state6(fbe_raid_siots_t * siots_p);
/****************************
 *      static VARIABLES
 ****************************/

/****************************
 *      GLOBAL VARIABLES
 ****************************/

/*************************************
 *      EXTERNAL GLOBAL VARIABLES
 *************************************/

/****************************************************************
 * fbe_raid_check_zeroed_state0()
 ****************************************************************
 * @brief
 *  Allocate fruts structures for a check zero request.
 *
 * @param siots_p - current sub request
 *
 * @return
 *  fbe_raid_state_status_t
 *
 * @author
 *  9/20/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_raid_check_zeroed_state0(fbe_raid_siots_t * siots_p)
{
    fbe_status_t            status;
    fbe_raid_fru_info_t     write_info[FBE_RAID_MAX_DISK_ARRAY_WIDTH];

    fbe_raid_check_zeroed_get_fruts_info(siots_p, &write_info[0]);
    /* Validate the algorithm.
     */
    if (siots_p->algorithm != RG_CHECK_ZEROED)
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "check zeroed: zero algorithm %d unexpected\n", 
                                            siots_p->algorithm);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* Calculate and allocate all the memory needed for this siots.
     */
    status = fbe_raid_check_zeroed_calculate_memory_size(siots_p, &write_info[0]);
    if (status != FBE_STATUS_OK)
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "check zeroed: failed to calculate mem size status: 0x%x algorithm: 0x%x\n", 
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
    fbe_raid_siots_transition(siots_p, fbe_raid_check_zeroed_state1);
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
        status = fbe_raid_check_zeroed_setup_resources(siots_p, &write_info[0]);
        if (RAID_COND(status != FBE_STATUS_OK))
        {
            fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "raid: checked zero failed to setup resources. status: 0x%x\n",
                                            status);
            return FBE_RAID_STATE_STATUS_EXECUTING;
        }

        /* To prevent confusion (i.e. we never went to `deferred allocation complete'
         * state) overwrite the current state.
         */
        fbe_raid_common_set_state(&siots_p->common, (fbe_raid_common_state_t)fbe_raid_check_zeroed_state3);
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
                                            "raid: checked zero memory request failed with status: 0x%x\n",
                                            status);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/***************************************
 *  end of fbe_raid_check_zeroed_state0()
 ***************************************/

/****************************************************************
 * fbe_raid_check_zeroed_state1()
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

static fbe_raid_state_status_t fbe_raid_check_zeroed_state1(fbe_raid_siots_t * siots_p)
{
    fbe_status_t            status;
    /* Add one for terminator.
     */
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
                             "parity: checked zero memory allocation error common flags: 0x%x \n",
                             fbe_raid_common_get_flags(&siots_p->common));
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_memory_allocation_error);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* Then calculate how many buffers are needed.
     */
    status = fbe_raid_check_zeroed_get_fruts_info(siots_p, &write_info[0]);
    if (RAID_COND(status != FBE_STATUS_OK))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                             "parity: checked zero failed to get fru info with status 0x%x, "
                             "siots_p = 0x%p, write_info_p 0x%p\n",
                             status, siots_p, &write_info[0]);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* Allocation completed. Plant the resources
     */
    status = fbe_raid_check_zeroed_setup_resources(siots_p, &write_info[0]);
    if (RAID_COND(status != FBE_STATUS_OK))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity: checked zero failed to setup resources status: 0x%x algorithm: 0x%x\n", 
                                            status, siots_p->algorithm);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* Transition to next state to issue request
     */
    fbe_raid_siots_transition(siots_p, fbe_raid_check_zeroed_state3);

    /* Return the state status
     */
    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/************************************
 * end fbe_raid_check_zeroed_state1()
 ************************************/

/****************************************************************
 * fbe_raid_check_zeroed_state3()
 ****************************************************************
 * @brief
 *  This function issues check zeros to the drives.
 *
 * @param siots_p - current sub request 
 *
 * @return
 *  fbe_raid_status_t
 *
 * @author
 *  9/20/2010 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_raid_state_status_t fbe_raid_check_zeroed_state3(fbe_raid_siots_t * siots_p)
{
    fbe_status_t fbe_status = FBE_STATUS_OK;
    fbe_raid_fruts_t *write_fruts_p = NULL;
    fbe_u16_t active_bitmask = 0;
    fbe_bool_t b_result = FBE_TRUE;
    fbe_raid_position_bitmask_t rebuild_logging_bitmask;

    fbe_raid_siots_check_traffic_trace(siots_p, FBE_TRUE);

    fbe_raid_siots_get_rebuild_logging_bitmask(siots_p, &rebuild_logging_bitmask);

    fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_p);

    if (fbe_raid_siots_is_aborted(siots_p))
    {
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_aborted);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* No need to generate the next request.  This siots always covers the
     * entire iots for check zeroed. 
     */

    /* These positions are not present, do not send check zero for these.
     */
    fbe_status = fbe_raid_fruts_for_each_only(rebuild_logging_bitmask, write_fruts_p, fbe_raid_fruts_set_opcode,
                                              (fbe_u64_t)FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID, (fbe_u64_t)0);
    if (RAID_COND(fbe_status != FBE_STATUS_OK))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "check zeroed: for_each_only status 0x%x\n", fbe_status);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* We do not want to trust the state of the drive.  If the remaining drives are zeroed then we will
     * issue the zero for this drive. 
     */
    fbe_raid_fruts_set_degraded_nop(siots_p, write_fruts_p);

    siots_p->wait_count = fbe_raid_fruts_get_chain_bitmap(write_fruts_p, &active_bitmask);

    /* We already setup the wait count when we setup the fruts.
     */
    if (siots_p->wait_count == 0)
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "check zeroed: zero unexpected wait_count: %lld\n", siots_p->wait_count);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }
    fbe_raid_siots_transition(siots_p, fbe_raid_check_zeroed_state4);

    /* Just send out all zeros.
     */
    b_result = fbe_raid_fruts_send_chain(&siots_p->write_fruts_head, fbe_raid_fruts_evaluate);

    return FBE_RAID_STATE_STATUS_WAITING;
}
/*****************************************
 * end fbe_raid_check_zeroed_state3() 
 *****************************************/

/****************************************************************
 * fbe_raid_check_zeroed_state4()
 ****************************************************************
 * @brief
 *  This function evaluates status of the check zero operations.
 *
 * @param siots_p - current sub request
 *
 * @return
 *  fbe_raid_state_status_t
 *
 * @author
 *  9/20/2010 - Created. Rob Foley
 *  
 ****************************************************************/

static fbe_raid_state_status_t fbe_raid_check_zeroed_state4(fbe_raid_siots_t * siots_p)
{
    fbe_raid_fru_eboard_t eboard;
    fbe_raid_fru_error_status_t fruts_status;
    fbe_raid_state_status_t state_status = FBE_RAID_STATE_STATUS_EXECUTING;
    fbe_raid_fruts_t *write_fruts_p = NULL;

    fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_p);
    fbe_raid_fru_eboard_init(&eboard);

    /* Sum up the errors encountered.
     * We need to do this prior to checking if we are aborted so we can populate the 
     * eboard and then report back these errors. 
     */
    fruts_status = fbe_raid_check_zeroed_get_fruts_error(write_fruts_p, &eboard);

    if (fruts_status == FBE_RAID_FRU_ERROR_STATUS_SUCCESS)
    {
        /* All check_zeros are successful.  Check to see if we are actually zeroed.
         */
        fbe_u16_t active_bitmask = 0;
        fbe_u32_t active_count;
        fbe_raid_position_bitmask_t rebuild_logging_bitmask;
        fbe_u16_t degraded_bitmask; 
        fbe_u32_t degraded_count;

        fbe_raid_siots_get_rebuild_logging_bitmask(siots_p, &rebuild_logging_bitmask);
        fbe_raid_siots_get_degraded_bitmask(siots_p, &degraded_bitmask); 

        /* We didn't issue check zeros for the rebuild logging bitmask. 
         * We only want to check that 
         */
        degraded_bitmask &= ~(rebuild_logging_bitmask);
        degraded_count = fbe_raid_get_bitcount(degraded_bitmask);

        /* If the number of requests that are active equals the number that recevied a "zeroed" status on the check zero
         * command, then this means that all the drives that were not degraded are zeroed.  Thus, the rebuilding drive 
         * needs to be made into zeros. 
         */
        active_count = fbe_raid_fruts_get_chain_bitmap(write_fruts_p, &active_bitmask);
        if (eboard.zeroed_bitmap == active_bitmask)
        {
            fbe_raid_iots_t *iots_p = fbe_raid_siots_get_iots(siots_p);
            fbe_payload_block_operation_opcode_t iots_opcode;

            fbe_raid_iots_get_opcode(iots_p, &iots_opcode);
            /* Everything is zeroed except for the rebuilding drive(s). 
             */
            if (iots_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_REBUILD)
            {
                /* For rebuild requests, Just issue the zero request for the rebuilding drive(s). 
                 */
                fbe_raid_siots_transition(siots_p, fbe_raid_check_zeroed_state5);
            }
            else
            {
                /* Return zeroed without issuing zero to degraded drives.
                 */
                fbe_raid_siots_transition(siots_p, fbe_raid_siots_zeroed);
            }
        }
        else
        {
            /* Transition to the status to return a not zeroed qualifier.
             * This will cause us to do a rebuild. 
             */
            fbe_raid_siots_transition(siots_p, fbe_raid_siots_success);
        }
    }
    else if ((fruts_status == FBE_RAID_FRU_ERROR_STATUS_ERROR) ||
             (fruts_status == FBE_RAID_FRU_ERROR_STATUS_SHUTDOWN))
    {
        /* We got an unexpected error.  Exit with failed status.
         */
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_shutdown_error);
    }
    else if (fruts_status == FBE_RAID_FRU_ERROR_STATUS_ABORTED)
    {
        /* The request was aborted, transition to aborted state to
         * return failed status.
         */
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_aborted);
    }
    else
    {
        fbe_raid_fru_eboard_display(&eboard, siots_p, FBE_TRACE_LEVEL_ERROR);
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "check zeroed: zero unexpected fru error status: %d\n", fruts_status);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    return state_status;
}                               
/*****************************************
 *  end of fbe_raid_check_zeroed_state4() 
 *****************************************/

/****************************************************************
 * fbe_raid_check_zeroed_state5()
 ****************************************************************
 * @brief
 *  This function issues a zero command to the rebuilding drive.
 *
 * @param siots_p - current sub request 
 *
 * @return
 *  fbe_raid_status_t
 *
 * @author
 *  11/4/2010 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_raid_state_status_t fbe_raid_check_zeroed_state5(fbe_raid_siots_t * siots_p)
{
    fbe_status_t fbe_status = FBE_STATUS_OK;
    fbe_raid_fruts_t *write_fruts_p = NULL;
    fbe_u32_t active_count;
    fbe_u16_t active_bitmask = 0;
    fbe_raid_position_bitmask_t rebuild_logging_bitmask;
    fbe_u16_t rebuild_bitmask; 
    fbe_bool_t b_result = FBE_TRUE;

    if (fbe_raid_siots_is_aborted(siots_p))
    {
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_aborted);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_p);

    fbe_raid_siots_get_rebuild_logging_bitmask(siots_p, &rebuild_logging_bitmask);
    fbe_raid_siots_get_degraded_bitmask(siots_p, &rebuild_bitmask); 

    /* We gather the bitmask of rebuilding bits.
     */
    rebuild_bitmask &= ~(rebuild_logging_bitmask);
    active_count = fbe_raid_fruts_get_chain_bitmap(write_fruts_p, &active_bitmask);

    /* Set alive drives as nop.
     */
    fbe_status = fbe_raid_fruts_for_each_only(active_bitmask, write_fruts_p, fbe_raid_fruts_set_opcode,
                                              (fbe_u64_t)FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID, (fbe_u64_t)0);
    if (RAID_COND(fbe_status != FBE_STATUS_OK))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "check zeroed5: for_each_only  status 0x%x\n", fbe_status);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* Set degraded drives as zero.
     */
    fbe_status = fbe_raid_fruts_for_each_only(rebuild_bitmask, write_fruts_p, fbe_raid_fruts_reset,
                                              siots_p->parity_count, (fbe_u64_t)FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ZERO);
    if (RAID_COND(fbe_status != FBE_STATUS_OK))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "check zeroed5: for_each_only zero status 0x%x\n", fbe_status);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    siots_p->wait_count = fbe_raid_get_bitcount(rebuild_bitmask);

    /* We already setup the wait count when we setup the fruts.
     */
    if (siots_p->wait_count == 0)
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "check zeroed5: zero unexpected deg wait_count: %lld\n", siots_p->wait_count);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }
    fbe_raid_siots_transition(siots_p, fbe_raid_check_zeroed_state6);

    /* Just send out all zeros.
     */
    b_result = fbe_raid_fruts_send_chain(&siots_p->write_fruts_head, fbe_raid_fruts_evaluate);

    return FBE_RAID_STATE_STATUS_WAITING;
}
/*****************************************
 * end fbe_raid_check_zeroed_state5() 
 *****************************************/

/****************************************************************
 * fbe_raid_check_zeroed_state6()
 ****************************************************************
 * @brief
 *  This function evaluates status of the zero operations.
 *
 * @param siots_p - current sub request
 *
 * @return
 *  fbe_raid_state_status_t
 *
 * @author
 *  11/4/2010 - Created. Rob Foley
 *  
 ****************************************************************/

static fbe_raid_state_status_t fbe_raid_check_zeroed_state6(fbe_raid_siots_t * siots_p)
{
    fbe_raid_fru_eboard_t eboard;
    fbe_raid_fru_error_status_t fruts_status;
    fbe_raid_fruts_t *write_fruts_p = NULL;

    fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_p);
    fbe_raid_fru_eboard_init(&eboard);

    /* Sum up the errors encountered.
     * We need to do this prior to checking if we are aborted so we can populate the 
     * eboard and then report back these errors. 
     */
    fruts_status = fbe_raid_check_zeroed_get_fruts_error(write_fruts_p, &eboard);

    if (fruts_status == FBE_RAID_FRU_ERROR_STATUS_SUCCESS)
    {
        /* All zeros are successful.  Return that the zero completed successfully.
         */
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_zeroed);
    }
    else if (fruts_status == FBE_RAID_FRU_ERROR_STATUS_ABORTED)
    {
        /* The request was aborted, transition to aborted state to
         * return failed status.
         */
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_aborted);
    }
    else if ((fruts_status == FBE_RAID_FRU_ERROR_STATUS_ERROR) ||
             (fruts_status == FBE_RAID_FRU_ERROR_STATUS_SHUTDOWN))
    {
        /* We got an unexpected error.  Exit with failed status.
         */
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_shutdown_error);
    }
    else
    {
        fbe_raid_fru_eboard_display(&eboard, siots_p, FBE_TRACE_LEVEL_ERROR);
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "check zeroed: zero unexpected fru error status: %d\n", fruts_status);
    }

    return FBE_RAID_STATE_STATUS_EXECUTING;
}                               
/*****************************************
 *  end of fbe_raid_check_zeroed_state6() 
 *****************************************/

/*!***************************************************************
 * fbe_raid_check_zeroed_get_fruts_info()
 ****************************************************************
 * @brief
 *  This function initializes the fru info for verify.
 *
 * @param siots_p - Current sub request.
 *
 * @return fbe_status_t
 *
 * @author
 *  9/20/2010 - Created. Rob Foley
 * 
 ****************************************************************/

static fbe_status_t fbe_raid_check_zeroed_get_fruts_info(fbe_raid_siots_t * siots_p,
                                                         fbe_raid_fru_info_t *write_p)
{
    fbe_block_count_t parity_range_offset;
    fbe_u16_t data_pos;
    fbe_u8_t *position = siots_p->geo.position;
    fbe_lba_t logical_parity_start = siots_p->geo.logical_parity_start;

    parity_range_offset = siots_p->geo.start_offset_rel_parity_stripe;

    /* Initialize the fru info for data drives.
     */
    for (data_pos = 0; data_pos < fbe_raid_siots_get_width(siots_p); data_pos++)
    {
        write_p->lba = logical_parity_start + parity_range_offset;
        write_p->blocks = siots_p->parity_count;
        write_p->position = position[data_pos];
        write_p++;
    }
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_raid_check_zeroed_get_fruts_info
 **************************************/
/***************************************************************************
 * fbe_raid_check_zeroed_setup_fruts()
 ***************************************************************************
 * @brief 
 *  This function initializes the fruts structures for a parity zero.
 *
 * @param siots_p - pointer to SIOTS data
 * @param write_p - Array of write information.
 * @param memory_info_p - pointer to memory information 
 *
 * @return fbe_status_t
 *
 * @author
 *  9/20/2010 - Created. Rob Foley
 *
 **************************************************************************/

static fbe_status_t fbe_raid_check_zeroed_setup_fruts(fbe_raid_siots_t * const siots_p, 
                                                      fbe_raid_fru_info_t *write_p,
                                                      fbe_raid_memory_info_t *memory_info_p)
{
    fbe_u16_t data_pos = 0;    /* data position in array */
    fbe_status_t status = FBE_STATUS_OK;
    /* Setup one write same for each drive.
     */
    for (data_pos = 0;
        data_pos < fbe_raid_siots_get_width(siots_p);
        data_pos++)
    {
        status = fbe_raid_setup_fruts_from_fru_info(siots_p,
                                                    write_p,
                                                    &siots_p->write_fruts_head,
                                                    FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_CHECK_ZEROED,
                                                    memory_info_p);
        if (status != FBE_STATUS_OK)
        {
            return status;
        }

        write_p++;
    }    /* While data_pos < width */

    return status;
}
/**************************************
 * end fbe_raid_check_zeroed_setup_fruts()
 **************************************/

/*!**************************************************************************
 * fbe_raid_check_zeroed_calculate_memory_size()
 ****************************************************************************
 * @brief  This function calculates the total memory usage of the siots for
 *         this state machine.
 *
 * @param siots_p - Pointer to siots that contains the allocated fruts
 * @param read_info_p - Array of read information.
 * @param read2_info_p - Array of read2 information.
 * @param write_info_p - Array of write information.
 * 
 * @return None.
 *
 * @author
 *  9/20/2010 - Created. Rob Foley
 *
 **************************************************************************/

static fbe_status_t fbe_raid_check_zeroed_calculate_memory_size(fbe_raid_siots_t *siots_p,
                                                                fbe_raid_fru_info_t *write_info_p)
{
    fbe_u32_t num_fruts = 0;
    fbe_status_t status;

    /* Then calculate how many buffers are needed.
     */
    status = fbe_raid_check_zeroed_get_fruts_info(siots_p, write_info_p);
    if (status != FBE_STATUS_OK)
    {
        return status;
    }

    siots_p->total_blocks_to_allocate = 0;
    /* First start off with 1 fruts for every data position we are writing 
     * and for every parity position. 
     */
    num_fruts = fbe_raid_siots_get_width(siots_p);
    fbe_raid_siots_set_optimal_page_size(siots_p, num_fruts, 0    /* no blocks to allocate */);

    /* Take the information we calculated above and determine how this translates 
     * into a number of pages. 
     */
    status = fbe_raid_siots_calculate_num_pages(siots_p, num_fruts, NULL,    /* No Sgs */
                                                FBE_FALSE /* No nested siots is required*/);
    return status;
}
/**************************************
 * end fbe_raid_check_zeroed_calculate_memory_size()
 **************************************/
/****************************************************************
 * fbe_raid_check_zeroed_setup_resources()
 ****************************************************************
 * @brief
 *  Setup the resources for the zero.
 * 
 * @param siots_p - current sub request 
 *
 * @return
 *  fbe_status_t
 *
 * @author
 *  9/20/2010 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_raid_check_zeroed_setup_resources(fbe_raid_siots_t * siots_p,
                                                          fbe_raid_fru_info_t *write_p)
{
    /* We initialize status to failure since it is expected to be populated. */
    fbe_status_t            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_raid_memory_info_t  memory_info;

    /* Initialize our memory request information
     */
    status = fbe_raid_siots_init_control_mem_info(siots_p, &memory_info);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "raid: checked zero failed to init memory info with status: 0x%x\n",
                             status);
        return(status); 
    }

    /* Initialize the FRU_TS objects, releasing any
     * which may have been allocated but are not used.
     */
    status = fbe_raid_check_zeroed_setup_fruts(siots_p, write_p, &memory_info);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "raid: checked zero failed to setup fruts with status: 0x%x\n",
                             status);
        return(status); 
    }

    return status;
}
/****************************
 *  end of fbe_raid_check_zeroed_setup_resources()
 ****************************/

/*!**************************************************************
 * fbe_raid_fru_eboard_trace()
 ****************************************************************
 * @brief
 *  Trace the fru eboard.
 *
 * @param eboard_p - This is the structure to trace.
 *
 * @return  fbe_raid_fru_error_status_t
 *
 * @author
 *  9/21/2010 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_raid_fru_eboard_trace(fbe_raid_fru_eboard_t * eboard_p,
                               fbe_raid_siots_t *siots_p,
                               fbe_trace_level_t trace_level)
{
    fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_WARNING,
                                "eboard: siots: %p soft_me_c: %d hard_me_b: 0x%x menr_b: 0x%x unexp: %d\n",
                                siots_p,
                                eboard_p->soft_media_err_count,
                                eboard_p->hard_media_err_bitmap,
                                eboard_p->menr_err_bitmap,
                                eboard_p->unexpected_err_count);
    fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_WARNING,
                                "eboard: siots: %p abort: %d timeout: %d dead_b: 0x%x retry_b: 0x%x\n",
                                siots_p,
                                eboard_p->abort_err_count,
                                eboard_p->timeout_err_count,
                                eboard_p->dead_err_bitmap,
                                eboard_p->retry_err_bitmap);
    return;
}
/**************************************
 * end fbe_raid_fru_eboard_trace
 **************************************/
/*!**************************************************************
 * fbe_raid_check_zeroed_get_fruts_error()
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
 *  9/21/2010 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_raid_fru_error_status_t fbe_raid_check_zeroed_get_fruts_error(fbe_raid_fruts_t *fruts_p,
                                                                         fbe_raid_fru_eboard_t * eboard_p)
{
    fbe_raid_fru_error_status_t fru_error_status = FBE_RAID_FRU_ERROR_STATUS_SUCCESS;
    fbe_bool_t b_status;
    fbe_raid_siots_t *siots_p = fbe_raid_fruts_get_siots(fruts_p);
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);

    /* Sum up the errors encountered.
     * We need to do this prior to checking if we are aborted so we can populate the 
     * eboard and then report back these errors. 
     */
    b_status = fbe_raid_fruts_chain_get_eboard(fruts_p, eboard_p);

    if (fbe_raid_siots_is_aborted(siots_p))
    {
        return FBE_RAID_FRU_ERROR_STATUS_ABORTED;
    }

    if (b_status == FBE_FALSE)
    {   
        fbe_raid_library_object_trace(fbe_raid_geometry_get_object_id(raid_geometry_p), 
                                      FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                      "check zeroed: unexpected error count: %d\n",
                                      eboard_p->unexpected_err_count);
        
        return FBE_RAID_FRU_ERROR_STATUS_UNEXPECTED;
    }

    if ( (eboard_p->soft_media_err_count > 0) ||
         (eboard_p->hard_media_err_count > 0) ||
         (eboard_p->drop_err_count > 0) ||
         (eboard_p->menr_err_count > 0) ||
         (eboard_p->timeout_err_count > 0) ||
         (eboard_p->unexpected_err_count > 0))
    {

        fbe_raid_library_object_trace(fbe_raid_geometry_get_object_id(raid_geometry_p), 
                                      FBE_RAID_LIBRARY_TRACE_PARAMS_WARNING,
                                      "check_zeroed: siots_p: %p unexpected error counts\n",
                                      siots_p);
        
        fbe_raid_fru_eboard_display(eboard_p, siots_p, FBE_TRACE_LEVEL_WARNING);
        return FBE_RAID_FRU_ERROR_STATUS_UNEXPECTED;
    }
    if ((eboard_p->dead_err_count == 0) && (eboard_p->retry_err_count == 0))
    {
        /* No frus are dead, just return current status.
         */
        return fru_error_status;
    }
    else
    {
        /* These errors can also occur.  We handle these by simply returning status to the monitor.
         */

        fbe_raid_library_object_trace(fbe_raid_geometry_get_object_id(raid_geometry_p), 
                                      FBE_RAID_LIBRARY_TRACE_PARAMS_WARNING,
                                      "raid: check zero errors: retry_c: %d mask: 0x%x dead_c: %d mask: 0x%x\n",
                                      eboard_p->retry_err_count,
                                      eboard_p->retry_err_bitmap,
                                      eboard_p->dead_err_count,
                                      eboard_p->dead_err_bitmap);

        
        return FBE_RAID_FRU_ERROR_STATUS_ERROR;
    }
}
/******************************************
 * end fbe_raid_check_zeroed_get_fruts_error()
 ******************************************/
/************************************************************
 * end fbe_raid_check_zeroed.c
 ************************************************************/
