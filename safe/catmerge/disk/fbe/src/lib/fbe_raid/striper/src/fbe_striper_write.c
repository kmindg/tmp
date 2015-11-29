/***************************************************************************
 * Copyright (C) EMC Corporation 1999-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_striper_write.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the state functions of the Raid0 write algorithm.
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
 *  2000 - Created. Kevin Schleicher
 *  2010 - Ported to FBE. Rob Foley
 ***************************************************************************/

/*************************
 **   INCLUDE FILES     **
 *************************/
#include "fbe_raid_library_proto.h"
#include "fbe_striper_io_private.h"

/********************************
 *      STATIC STRING LITERALS
 ********************************/

/*****************************************************
 *      static FUNCTIONS DECLARED GLOBALLY FOR VISIBILITY
 *****************************************************/

static fbe_raid_state_status_t fbe_striper_write_state1(fbe_raid_siots_t * siots_p);
static fbe_raid_state_status_t fbe_striper_write_state3(fbe_raid_siots_t * siots_p);
static fbe_raid_state_status_t fbe_striper_write_state10(fbe_raid_siots_t * siots_p);
static fbe_raid_state_status_t fbe_striper_write_state11(fbe_raid_siots_t * siots_p);
static fbe_raid_state_status_t fbe_striper_write_state12(fbe_raid_siots_t * siots_p);
static fbe_raid_state_status_t fbe_striper_write_state13(fbe_raid_siots_t * siots_p);
static fbe_raid_state_status_t fbe_striper_write_state14(fbe_raid_siots_t * siots_p);
static fbe_raid_state_status_t fbe_striper_write_state14a(fbe_raid_siots_t * siots_p);
static fbe_raid_state_status_t fbe_striper_write_state15(fbe_raid_siots_t * siots_p);
static fbe_raid_state_status_t fbe_striper_write_state2(fbe_raid_siots_t * siots_p);
static fbe_raid_state_status_t fbe_striper_write_state20(fbe_raid_siots_t * siots_p);
static fbe_raid_state_status_t fbe_striper_write_state21(fbe_raid_siots_t * siots_p);
static fbe_raid_state_status_t fbe_striper_write_state22(fbe_raid_siots_t * siots_p);
static fbe_raid_state_status_t fbe_striper_write_state23(fbe_raid_siots_t * siots_p);
static fbe_raid_state_status_t fbe_striper_write_state25(fbe_raid_siots_t * siots_p);
static fbe_raid_state_status_t fbe_striper_write_state30(fbe_raid_siots_t * siots_p);
static fbe_raid_state_status_t fbe_striper_write_state40(fbe_raid_siots_t * siots_p);
/****************************
 *      GLOBAL VARIABLES
 ****************************/

/*************************************
 *      EXTERNAL GLOBAL VARIABLES
 *************************************/

/*!**************************************************************
 * fbe_striper_write_state0()
 ****************************************************************
 * @brief
 *  This function allocates resources for this siots and sets them up.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t
 *
 * @return
 *      FBE_RAID_STATE_STATUS_WAITING - Resources not
 *      available.
 *      FBE_RAID_STATE_STATUS_EXECUTING - Otherwise
 *
 * @author
 *  12/14/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_striper_write_state0(fbe_raid_siots_t * siots_p)
{
    fbe_status_t            status;

    /* Add one for terminator.
     */
    fbe_raid_fru_info_t read_info[FBE_RAID_MAX_DISK_ARRAY_WIDTH+1];
    fbe_raid_fru_info_t write_info[FBE_RAID_MAX_DISK_ARRAY_WIDTH+1];

    fbe_raid_fru_info_init_arrays(fbe_raid_siots_get_width(siots_p), &read_info[0],&write_info[0],NULL);

    /* Validate the algorithm.
     */    
    if (RAID_COND(siots_p->algorithm != RAID0_WR))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "striper: unexpected algorithm: %d\n", siots_p->algorithm);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }
    status = fbe_striper_generate_validate_siots(siots_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "striper: failed validate status: %d \n", status);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* Calculate and allocate all the memory needed for this siots.
     */
    status = fbe_striper_write_calculate_memory_size(siots_p, &read_info[0], &write_info[0]);
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
    fbe_raid_siots_transition(siots_p, fbe_striper_write_state1);
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
        status = fbe_striper_write_setup_resources(siots_p, &read_info[0], &write_info[0]);
        if (RAID_COND(status != FBE_STATUS_OK))
        {
            fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "striper: write failed to setup resources. status: 0x%x\n",
                                            status);
            return FBE_RAID_STATE_STATUS_EXECUTING;
        }

        /* To prevent confusion (i.e. we never went to `deferred allocation complete'
         * state) overwrite the current state.
         */
        fbe_raid_common_set_state(&siots_p->common, (fbe_raid_common_state_t)fbe_striper_write_state10);
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
                                            "striper: write memory request failed with status: 0x%x\n",
                                            status);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    return FBE_RAID_STATE_STATUS_EXECUTING;

}
/* end of fbe_striper_write_state0() */

/****************************************************************
 * fbe_striper_write_state1()
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

static fbe_raid_state_status_t fbe_striper_write_state1(fbe_raid_siots_t * siots_p)
{
    fbe_status_t            status;
    /* Add one for terminator.
     */
    fbe_raid_fru_info_t     read_info[FBE_RAID_MAX_DISK_ARRAY_WIDTH+1];
    fbe_raid_fru_info_t     write_info[FBE_RAID_MAX_DISK_ARRAY_WIDTH+1];

    fbe_raid_fru_info_init_arrays(fbe_raid_siots_get_width(siots_p), &read_info[0],&write_info[0],NULL);

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
                             "striper: write memory allocation error common flags: 0x%x \n",
                             fbe_raid_common_get_flags(&siots_p->common));
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_memory_allocation_error);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* Then calculate how many buffers are needed.
     */  
    status = fbe_striper_write_calculate_memory_size(siots_p, &read_info[0], &write_info[0]);
    if (status != FBE_STATUS_OK)
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "striper: failed to calculate mem size status: 0x%x algorithm: 0x%x\n", 
                                            status, siots_p->algorithm);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }      

    /* Allocation completed. Plant the resources
     */
    status = fbe_striper_write_setup_resources(siots_p, &read_info[0], &write_info[0]);
    if (RAID_COND(status != FBE_STATUS_OK))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "striper: write failed to setup resources. status: 0x%x\n",
                                            status);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* Transition to next state to issue request
     */
    fbe_raid_siots_transition(siots_p, fbe_striper_write_state10);

    /* Return the state status
     */
    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/***********************************
 * end fbe_striper_write_state1()
 **********************************/

/*!**************************************************************
 * fbe_striper_write_state10()
 ****************************************************************
 * @brief
 *  Determine which state to transition to,
 *  either: pre-read, write or nested verify (bva).
 *
 * @param siots_p - ptr to the fbe_raid_siots_t
 *
 * @return
 *  This function will return FBE_RAID_STATE_STATUS_EXECUTING
 *
 * @author
 *  1/20/99 - Created. Kevin Schleicher
 *
 ****************************************************************/

static fbe_raid_state_status_t fbe_striper_write_state10(fbe_raid_siots_t * siots_p)
{
    fbe_payload_block_operation_opcode_t opcode;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_raid_siots_get_opcode(siots_p, &opcode);

    if (fbe_raid_siots_is_aborted(siots_p))
    {
        /* The iots has been aborted,
         * treat it as a hard error,
         * and bail out now.
         */
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_aborted);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    if ( fbe_raid_geometry_is_raid10(raid_geometry_p) &&
        !fbe_raid_siots_is_buffered_op(siots_p)          )
    {
        /* Raid 10s let the mirrors handle everything.
         * This includes checking checksums and setting lba stamps. 
         * Only the mirror can check/set the lba stamps since it knows the 
         * actual offset onto the drive. 
         */
        fbe_raid_siots_transition(siots_p, fbe_striper_write_state25);
    }
    else if (fbe_raid_geometry_is_raid0(raid_geometry_p) && 
             (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_WRITE))
    {
        /* we will resume the write after BVA verify
         */
        fbe_raid_siots_transition(siots_p, fbe_striper_write_state14);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }/* end if not mirror and VERIFY_WRITE */
    else if ( fbe_raid_geometry_is_raid0(raid_geometry_p)       &&
             (fbe_raid_siots_get_read_fruts_count(siots_p) > 0)    )
    {
        /* We allow `mixed' raid groups that contain both 4K and 520-bps drives.
         * Therefore the original request could be unaligned but if the only
         * positions modified are 520-bps, there is no need for a pre-read.
         * Therefore we simply check if there are any pre-read fruts or not.
         * Raid 10s allow the mirror to do the pre-read.
         */
        fbe_raid_siots_transition(siots_p, fbe_striper_write_state11);
    }
    else              
    {
        /* Normal write (Not BVA) aligned raid 0 write case.
         */
        fbe_raid_siots_transition(siots_p, fbe_striper_write_state22);
    }
    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/* end of fbe_striper_write_state10() */

/*!**************************************************************
 * fbe_striper_write_state11()
 ****************************************************************
 * @brief
 *  Issue pre-read fruts for unaligned write requests
 *
 * @param siots_p - ptr to the fbe_raid_siots_t
 *
 * @return
 *  This function normally returns FBE_RAID_STATE_STATUS_WAITING.
 * 
 * @author
 *      9/16/08 - Created. Ron Proulx
 *
 ****************************************************************/

static fbe_raid_state_status_t fbe_striper_write_state11(fbe_raid_siots_t *siots_p)
{
    fbe_status_t            status;
    fbe_bool_t              b_result = FBE_TRUE;
    fbe_bool_t              b_generate_next_siots = FBE_FALSE;
    fbe_raid_iots_t        *iots_p = fbe_raid_siots_get_iots(siots_p);
    fbe_raid_geometry_t    *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);

    /* Determine if we need to generate another siots.
     * If this returns TRUE then we must always call fbe_raid_iots_generate_next_siots_immediate(). 
     * Typically this call is done after we have started the disk I/Os.
     */
    status = fbe_raid_siots_should_generate_next_siots(siots_p, &b_generate_next_siots);
    if (RAID_GENERATE_COND(status != FBE_STATUS_OK)) {
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS, 
                                            "striper: %s failed in next siots generation: siots_p: 0x%p status: 0x%x",
                                            __FUNCTION__, siots_p, status);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* There shpould be at least (1) position that needs alignment.
     */
    if (RAID_COND(fbe_raid_geometry_needs_alignment(raid_geometry_p) == FBE_FALSE)) {
        /* We should only be here if there is one or more positions that need
         * to be aligned.
         */
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "striper: %s no aligment required! siots 0x%p\n",
                                            __FUNCTION__, siots_p);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* There should be at least 1 read fruts.
     */
    siots_p->wait_count = fbe_raid_siots_get_read_fruts_count(siots_p);
    if (RAID_FRUTS_COND(siots_p->wait_count < 1)) {
        /* We expect one or more read fruts.
         */
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "striper: %s read fruts expected! siots 0x%p\n",
                                            __FUNCTION__, siots_p);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* Issue pre-reads to the disks.
     */
    fbe_raid_siots_transition(siots_p, fbe_striper_write_state12);

    b_result = fbe_raid_fruts_send_chain(&siots_p->read_fruts_head, fbe_raid_fruts_evaluate);
    if (RAID_FRUTS_COND(b_result != FBE_TRUE)) {
        /* We hit an issue while starting chain of fruts. We can not immediately move
         * for siots completion as we are not sure exactly how many fruts has been 
         * sent successfully.
         */
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "striper: %s failed to start read fruts chain for siots 0x%p\n",
                                            __FUNCTION__, siots_p);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    if (b_generate_next_siots) {
        fbe_raid_iots_generate_next_siots_immediate(iots_p);
    }

    return FBE_RAID_STATE_STATUS_WAITING;
}
/*****************************************
 * end fbe_striper_write_state11() 
 *****************************************/

/*!**************************************************************
 * fbe_striper_write_state12()
 ****************************************************************
 * @brief
 *  Pre-Reads have completed, check status.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t
 *
 * @return fbe_raid_state_status_t
 *
 * @author
 *  1/25/00 - Created. Kevin Schleicher
 *  9/16/08 - Updated r0_rd_state4 for R0 pre-reads. Ron Proulx
 ****************************************************************/

static fbe_raid_state_status_t fbe_striper_write_state12(fbe_raid_siots_t * siots_p)
{
    fbe_status_t fbe_status = FBE_STATUS_OK;
    fbe_raid_fru_eboard_t eboard;
    fbe_raid_fru_error_status_t fru_status;
    fbe_raid_state_status_t return_status = FBE_RAID_STATE_STATUS_EXECUTING;
    fbe_raid_fruts_t *read_fruts_p = NULL;
    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_p);

    fbe_raid_fru_eboard_init(&eboard);

    /* Get totals for errored or dead drives.
     */
    fru_status = fbe_striper_get_fruts_error(read_fruts_p, &eboard);

    if (fru_status == FBE_RAID_FRU_ERROR_STATUS_SUCCESS)
    {
        /* Stage 1: All reads are successful.
         *          Check checksums if necessary.
         *
         */
        fbe_raid_siots_transition(siots_p, fbe_striper_write_state13);

        /* Check the checksums.
         * Check checksums and set lba stamps.
         * Both checksums and lba stamps must be checked
         */
        fbe_status = fbe_raid_xor_read_check_checksum(siots_p, FBE_TRUE, FBE_TRUE);
        return return_status;
    }
    else if ((fru_status == FBE_RAID_FRU_ERROR_STATUS_SHUTDOWN) ||
             (fru_status == FBE_RAID_FRU_ERROR_STATUS_DEAD) ||
             (fru_status == FBE_RAID_FRU_ERROR_STATUS_RETRY) ||
             (fru_status == FBE_RAID_FRU_ERROR_STATUS_ABORTED) ||
             (fru_status == FBE_RAID_FRU_ERROR_STATUS_WAITING) ||
             (fru_status == FBE_RAID_FRU_ERROR_STATUS_NOT_PREFERRED) ||
             (fru_status == FBE_RAID_FRU_ERROR_STATUS_REDUCE_QD_HARD) ||
             (fru_status == FBE_RAID_FRU_ERROR_STATUS_REDUCE_QD_SOFT))
    {
        fbe_raid_state_status_t state_status;
        state_status = fbe_striper_handle_fruts_error(siots_p, read_fruts_p, &eboard, fru_status);
        return state_status;
    }
    else if ((fru_status == FBE_RAID_FRU_ERROR_STATUS_ERROR) && 
             (eboard.hard_media_err_count != 0))
    {

        /* Either we received a needs verify error or a hard error.
         * We do not expect media or dropped errors. 
         *  
         * We will continue the original read when the recovery verify completes. 
         */
        fbe_raid_siots_set_xor_command_status(siots_p, FBE_XOR_STATUS_NO_ERROR);
        fbe_raid_siots_transition(siots_p, fbe_striper_write_state15);

        /* Transition to recovery verify.
         */
        fbe_status = fbe_raid_siots_start_nested_siots(siots_p, fbe_striper_recovery_verify_state0);
        if (RAID_GENERATE_COND(fbe_status != FBE_STATUS_OK))
        { 
            fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "striper: write unable to start nested verify for error status %d\n", 
                                                fbe_status);
            fbe_raid_fru_eboard_display(&eboard, siots_p, FBE_TRACE_LEVEL_ERROR);
            return FBE_RAID_STATE_STATUS_EXECUTING;
        }
        return FBE_RAID_STATE_STATUS_WAITING;
    }
    else
    {
        /* Raw units should never be able to get FBE_RAID_FRU_ERROR_STATUS_WAITING
         */
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "striper: xor status unknown 0x%x\n", fru_status);
        fbe_raid_fru_eboard_display(&eboard, siots_p, FBE_TRACE_LEVEL_ERROR);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }
    return return_status;
}
/* end of fbe_striper_write_state12() */

/*!**************************************************************
 * fbe_striper_write_state13()
 ****************************************************************
 * @brief
 *  Evaluate status of checking the checksums.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t
 *
 * @return
 *  This function returns FBE_RAID_STATE_STATUS_EXECUTING or
 *     FBE_RAID_STATE_STATUS_WAITING.
 * 
 * @author
 *  1/26/00 - Created. Kevin Schleicher
 *  9/16/08 - Copied from r0_rd_state5 Ron Proulx
 ****************************************************************/

static fbe_raid_state_status_t fbe_striper_write_state13(fbe_raid_siots_t * siots_p)
{
    fbe_status_t fbe_status = FBE_STATUS_OK;
    fbe_raid_state_status_t state_status = FBE_RAID_STATE_STATUS_EXECUTING;
    fbe_xor_status_t xor_status;
    fbe_raid_siots_get_xor_command_status(siots_p, &xor_status);

    if (xor_status == FBE_XOR_STATUS_NO_ERROR)
    {
        /* No checksum error, continue read.
         */
        fbe_raid_siots_transition(siots_p, fbe_striper_write_state22);
    }
    else if (xor_status & FBE_XOR_STATUS_CHECKSUM_ERROR)
    {
        /* Checksum error encountered.
         * We will continue with the write when the recovery verify completes. 
         */
        fbe_raid_siots_set_xor_command_status(siots_p, FBE_XOR_STATUS_NO_ERROR);
        fbe_raid_siots_transition(siots_p, fbe_striper_write_state15);

        /* Kick off a nested siots for recovery verify. 
         */
        fbe_status = fbe_raid_siots_start_nested_siots(siots_p, fbe_striper_recovery_verify_state0);
        if (RAID_GENERATE_COND(fbe_status != FBE_STATUS_OK))
        {
            fbe_raid_siots_set_unexpected_error(siots_p, 
                                                FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "striper: failed to start nested siots  0x%p\n",
                                                siots_p);
            return (FBE_RAID_STATE_STATUS_EXECUTING); 
        }
        return FBE_RAID_STATE_STATUS_WAITING;
    }
    else
    {
        /* Unexpected xor status here.
         */
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "striper: xor status unknown 0x%x\n", state_status);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }
    return state_status;
}
/* end of fbe_striper_write_state13() */

/*!**************************************************************
 * fbe_striper_write_state14()
 ****************************************************************
 * @brief
 *  Kick off BVA verify.
 *
 *  siots_p - ptr to the fbe_raid_siots_t
 *
 * @return
 *  This function returns FBE_RAID_STATE_STATUS_EXECUTING.
 * 
 * @author
 *  11/17/2010 - Created. Rob Foley
 ****************************************************************/

static fbe_raid_state_status_t fbe_striper_write_state14(fbe_raid_siots_t * siots_p)
{
    fbe_status_t status;
    fbe_bool_t b_generate_next_siots = FBE_FALSE;
    fbe_raid_iots_t *iots_p = fbe_raid_siots_get_iots(siots_p);

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

    /* we will resume the write after BVA verify
     */
    fbe_raid_siots_transition(siots_p, fbe_striper_write_state14a);

    status = fbe_raid_siots_start_nested_siots(siots_p, fbe_striper_bva_vr_state0);

    if (RAID_GENERATE_COND(status != FBE_STATUS_OK))
    {
        if (b_generate_next_siots){ fbe_raid_iots_cancel_generate_next_siots(iots_p); }
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "striper: write unable to start nested verify for BVA status %d\n", 
                                            status);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }
    if (b_generate_next_siots)
    {
        fbe_raid_iots_generate_next_siots_immediate(iots_p);
    }
    return FBE_RAID_STATE_STATUS_WAITING;
}
/* end of fbe_striper_write_state14() */

/*!**************************************************************
 * fbe_striper_write_state14a()
 ****************************************************************
 * @brief BVA Verify is complete, check status.
 *
 *  siots_p - ptr to the fbe_raid_siots_t
 *
 * @return
 *  This function returns FBE_RAID_STATE_STATUS_EXECUTING.
 * 
 * @author
 *  04/29/2014  Ron Proulx  - Created.
 ****************************************************************/

static fbe_raid_state_status_t fbe_striper_write_state14a(fbe_raid_siots_t * siots_p)
{
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);

    /* Return from BVA verify.
     */
    if (fbe_raid_siots_is_aborted(siots_p)) {
        /* The iots has been aborted, bail out now.
         */
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_aborted);
    } else if (siots_p->error == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS) {
        /* If this is an unaligned write issue the pre-read now.
         */
        if ( fbe_raid_geometry_is_raid0(raid_geometry_p)       &&
            (fbe_raid_siots_get_read_fruts_count(siots_p) > 0)    ) {
            /* We allow `mixed' raid groups that contain both 4K and 520-bps drives.
             * Therefore the original request could be unaligned but if the only
             * positions modified are 520-bps, there is no need for a pre-read.
             * Therefore we simply check if there are any pre-read fruts or not.
             * Raid 10s allow the mirror to do the pre-read.
             */
            fbe_raid_siots_transition(siots_p, fbe_striper_write_state11);
        } else {
            /* The BVA verify has completed.  Now continue the write process.
             */
            fbe_raid_siots_transition(siots_p, fbe_striper_write_state22);
        }
    } else if (siots_p->error == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED) {
        /* There is a shutdown condition, go shutdown error.
         */
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_shutdown_error);
    } else {
        /* Come here because hit a HARD_ERROR, MEDIA_ERROR, or DROPPED ERROR
         * in the free build. Print out a log, then go shutdown error.
         */
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                             FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "striper: %s got unexpected error 0x%x, siots_p = 0x%p\n",
                                            __FUNCTION__,
                                            siots_p->error,
                                            siots_p);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }
    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/* end of fbe_striper_write_state14a() */

/*!**************************************************************
 * fbe_striper_write_state15()
 ****************************************************************
 * @brief
 *  Recovery verify is complete, check status of the recovery verify.
 *
 *  siots_p - ptr to the fbe_raid_siots_t
 *
 * @return
 *  This function returns FBE_RAID_STATE_STATUS_EXECUTING.
 * 
 * @author
 *  02/04/09 - Created. Rob Foley.
 ****************************************************************/

static fbe_raid_state_status_t fbe_striper_write_state15(fbe_raid_siots_t * siots_p)
{
    /* Return from recovery verify.
     */
    if (fbe_raid_siots_is_aborted(siots_p))
    {
        /* The iots has been aborted, bail out now.
         */
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_aborted);
    }
    else if (siots_p->error == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS)
    {
        /* Verify has recovered the read data.  Continue processing the write.
         */
        fbe_raid_siots_transition(siots_p, fbe_striper_write_state22);
    }
    else if (siots_p->error == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED)
    {
        /* There is a shutdown condition, go shutdown error.
         */
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_shutdown_error);
    }
    else
    {
        /* Come here because hit a HARD_ERROR, MEDIA_ERROR, or DROPPED ERROR
         * in the free build. Print out a log, then go shutdown error.
         */
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                             FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "striper: %s got unexpected error 0x%x, siots_p = 0x%p\n",
                                            __FUNCTION__,
                                            siots_p->error,
                                            siots_p);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }
    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/* end of fbe_striper_write_state15() */

/*!**************************************************************
 * fbe_striper_write_state22()
 ****************************************************************
 * @brief
 *  Set the checksums. 
 *
 * @param siots_p - ptr to the fbe_raid_siots_t
 *
 * @return
 *      This function returns FBE_RAID_STATE_STATUS_EXECUTING.
 * 
 * @author
 *      1/26/00 - Created. Kevin Schleicher
 *      5/12/00 - Modified to interface with the XOR driver. MPG
 *
 ****************************************************************/

static fbe_raid_state_status_t fbe_striper_write_state22(fbe_raid_siots_t * siots_p)
{
    fbe_status_t status;
    /* The next state checks for checksum errors since we are checking
     * the cache data. 
     */
    fbe_raid_siots_transition(siots_p, fbe_striper_write_state23);

    /* Get the data for a buffered operation.
     */
    if (fbe_raid_siots_is_buffered_op(siots_p))
    {
        status = fbe_raid_xor_get_buffered_data(siots_p);
        if (status != FBE_STATUS_OK)
        {
            fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "striper: write fbe_raid_xor_get_buffered_data returned status: %d\n", status);
            return FBE_RAID_STATE_STATUS_EXECUTING;
        }
    }
    else
    {
        /* Cached ops will check checksums and generate an lba stamp.
         */
        status = fbe_raid_xor_striper_write_check_checksum(siots_p);
        if (RAID_COND(status != FBE_STATUS_OK))
        {
            fbe_raid_siots_set_unexpected_error(siots_p, 
                                                FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "Striper: Invalid status %d\n",
                                                status);
            return FBE_RAID_STATE_STATUS_EXECUTING;
        }
    }
    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/* end of fbe_striper_write_state22() */
/*!**************************************************************
 * fbe_striper_write_state23()
 ****************************************************************
 * @brief
 *  This function evaluates the checksum check on host data.
 *
 * @param siots_p - current sub request
 *
 * @return
 *  This function returns FBE_RAID_STATE_STATUS_EXECUTING.
 *
 * @author
 *  08/10/07 - Created. Jyoti Ranjan
 *
 ****************************************************************/

static fbe_raid_state_status_t fbe_striper_write_state23(fbe_raid_siots_t * siots_p)
{
    fbe_raid_state_status_t status = FBE_RAID_STATE_STATUS_EXECUTING;
    fbe_xor_status_t xor_status;

    fbe_raid_siots_get_xor_command_status(siots_p, &xor_status);

    if (xor_status == FBE_XOR_STATUS_NO_ERROR)
    {
        fbe_raid_siots_transition(siots_p, fbe_striper_write_state25);
    }
    else if (xor_status & FBE_XOR_STATUS_BAD_MEMORY)
    {
        /* Striper should not find bad checksums in the write
         * data because if we were told to check checksums then we expect no CRC errors. 
         * If we were not told to check checksums we wouldn't be here.
         */
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_write_crc_error);
    }
    else if (xor_status & FBE_XOR_STATUS_CHECKSUM_ERROR)
    {
        /* All raid types (including striped raid groups) must all previously
         * invalidated blocks in write data (for instance `data lost').
         * Therefore just proceed to next state.
         */
        fbe_raid_siots_transition(siots_p, fbe_striper_write_state25);
    }
    else
    {
        /* Unexpected XOR status. 
         */
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                              "striper: %s unexpected xor status: 0x%x\n", 
                              __FUNCTION__, xor_status);
    }
    return status;
}
/*****************************************
 *  end of fbe_striper_write_state23() 
 *****************************************/
/*!**************************************************************
 * fbe_striper_write_state25()
 ****************************************************************
 * @brief
 *  Start the writes.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t
 *
 * @return
 *  This function returns FBE_RAID_STATE_STATUS_WAITING.
 * 
 * @author
 *  5/12/00 - Created. MPG
 *
 ****************************************************************/

static fbe_raid_state_status_t fbe_striper_write_state25(fbe_raid_siots_t * siots_p)
{
    fbe_status_t status;
    fbe_bool_t b_result = FBE_TRUE;
    fbe_bool_t b_generate_next_siots = FBE_FALSE;
    fbe_raid_iots_t *iots_p = fbe_raid_siots_get_iots(siots_p);

    fbe_raid_siots_check_traffic_trace(siots_p, FBE_TRUE);

    siots_p->wait_count = siots_p->data_disks;

    fbe_raid_siots_transition(siots_p, fbe_striper_write_state30);

    if (RAID_COND(siots_p->common.wait_queue_element.next != &siots_p->common.wait_queue_element))
    {
        void *  p2 = siots_p->common.wait_queue_element.next;/*next is an alligent pointer and can't be put directly on a call stack*/

        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                 "Striper: Invalid element on wait queue, wait_queue_element %p, next %p\n",
                 &siots_p->common.wait_queue_element, p2);

        return FBE_RAID_STATE_STATUS_EXECUTING;
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
    fbe_raid_siots_set_flag(siots_p, FBE_RAID_SIOTS_FLAG_WRITE_STARTED);

    b_result = fbe_raid_fruts_send_chain(&siots_p->write_fruts_head, fbe_raid_fruts_evaluate);
    if (RAID_FRUTS_COND(b_result != FBE_TRUE))
    {
        /* We hit an issue while starting chain of fruts.
         */
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "striper: %s failed to start write fruts chain for "
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
/*****************************************
 * end fbe_striper_write_state25() 
 *****************************************/

/*!**************************************************************
 * fbe_striper_write_state30()
 ****************************************************************
 * @brief
 *  Writes have completed, check status.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t
 *
 * @return
 *      This function returns FBE_RAID_STATE_STATUS_EXECUTING.
 *
 * @author
 *      1/25/00 - Created. Kevin Schleicher 
 *
 ****************************************************************/

static fbe_raid_state_status_t fbe_striper_write_state30(fbe_raid_siots_t * siots_p)
{
    fbe_raid_fru_eboard_t eboard;
    fbe_raid_fru_error_status_t fruts_status;
    fbe_raid_state_status_t state_status = FBE_RAID_STATE_STATUS_EXECUTING;
    fbe_raid_fruts_t *write_fruts_p = NULL;
    fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_p);
    
    fbe_raid_fru_eboard_init(&eboard);

    /* Get totals for errored or dead drives.
     */
    fruts_status = fbe_striper_get_fruts_error(write_fruts_p, &eboard);
    if (fruts_status == FBE_RAID_FRU_ERROR_STATUS_SUCCESS)
    {
        /* All writes are successful. 
         */
        if (fbe_raid_geometry_is_raid10(fbe_raid_siots_get_raid_geometry(siots_p)) != FBE_TRUE)
        {
            /* Also, report the event log message(s) but we will do sp 
             * specifically for RAID 0 only because we might have already reported 
             * in  case of stripped-mirror raid group i.e. RAID 10.
             */
            fbe_striper_report_errors(siots_p, FBE_TRUE);
        }
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_success);
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
    else
    {
        fbe_raid_state_status_t state_status;
        state_status = fbe_striper_handle_fruts_error(siots_p, write_fruts_p, &eboard, fruts_status);
        return state_status;
    }
    return state_status;
}                               
/*****************************************
 *  end of fbe_striper_write_state30() 
 *****************************************/
/*************************
 * end file fbe_striper_write.c
 *************************/
