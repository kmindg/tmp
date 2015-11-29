/***************************************************************************
 * Copyright (C) EMC Corporation 2004-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_parity_zero.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the state functions of the zero state machine.
 *
 * @notes
 *  At initial entry, we assume that the fbe_raid_siots_t has
 *  the following fields initialized:
 *   start_lba       - logical address of first block of new data
 *   xfer_count      - total blocks of new data
 *   data_disks      - tatal data disks affected 
 *   start_pos       - relative start disk position  
 *   parity_pos      - RG_INVALID_FIELD if this is a striper.
 *
 * @author
 *  3/4/2010 - Created. Rob Foley
 *
 ***************************************************************************/


/*************************
 ** INCLUDE FILES
 *************************/
#include "fbe_raid_library_proto.h"
#include "fbe_parity_io_private.h"

/********************************
 *      static STRING LITERALS
 ********************************/

/*****************************************************
 *      static FUNCTIONS DECLARED GLOBALLY FOR VISIBILITY
 *****************************************************/
static fbe_status_t fbe_parity_zero_get_fruts_info(fbe_raid_siots_t * siots_p,
                                                   fbe_raid_fru_info_t *write_p);
static fbe_status_t fbe_parity_zero_setup_fruts(fbe_raid_siots_t * const siots_p,
                                                fbe_raid_fru_info_t *write_p,
                                                fbe_raid_memory_info_t *memory_info_p);
static fbe_status_t fbe_parity_zero_calculate_memory_size(fbe_raid_siots_t *siots_p,
                                                         fbe_raid_fru_info_t *write_info_p);
static fbe_status_t fbe_parity_zero_setup_resources(fbe_raid_siots_t * siots_p,
                                                               fbe_raid_fru_info_t *write_p);
static fbe_raid_state_status_t fbe_parity_zero_state1(fbe_raid_siots_t *siots_p);
static fbe_raid_state_status_t fbe_parity_zero_state2(fbe_raid_siots_t *siots_p);
static fbe_raid_state_status_t fbe_parity_zero_state3(fbe_raid_siots_t *siots_p);
static fbe_raid_state_status_t fbe_parity_zero_state4(fbe_raid_siots_t *siots_p);

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
 * fbe_parity_zero_state0()
 ****************************************************************
 * @brief
 *  Allocate fruts structures for a Zero request (which gets changed
 *  to disk WRITE SAME requests).
 *
 * @param siots_p - current sub request
 *
 * @return
 *  fbe_raid_state_status_t
 *
 * @author
 *  05/04/04 - Created. Rob Foley
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_parity_zero_state0(fbe_raid_siots_t * siots_p)
{
    fbe_status_t            status;
    fbe_raid_fru_info_t     write_info[FBE_RAID_MAX_DISK_ARRAY_WIDTH];

    fbe_parity_zero_get_fruts_info(siots_p, &write_info[0]);
    /* Validate the algorithm.
     */
    if (siots_p->algorithm != RG_ZERO)
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity: zero algorithm %d unexpected\n", 
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

    /* Calculate and allocate all the memory needed for this siots.
     */
    status = fbe_parity_zero_calculate_memory_size(siots_p, &write_info[0]);
    if (status != FBE_STATUS_OK)
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity: zero failed to calculate mem size status: 0x%x algorithm: 0x%x\n", 
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
    fbe_raid_siots_transition(siots_p, fbe_parity_zero_state1);
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
        status = fbe_parity_zero_setup_resources(siots_p, &write_info[0]);
        if (RAID_COND(status != FBE_STATUS_OK))
        {
            fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity: zero failed to setup resources. status: 0x%x\n",
                                            status);
            return FBE_RAID_STATE_STATUS_EXECUTING;
        }

        /* To prevent confusion (i.e. we never went to `deferred allocation complete'
         * state) overwrite the current state.
         */
        fbe_raid_common_set_state(&siots_p->common, (fbe_raid_common_state_t)fbe_parity_zero_state3);
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
                                            "parity: zero memory request failed with status: 0x%x\n",
                                            status);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/*********************************
 *  end of fbe_parity_zero_state0()
 *********************************/

/****************************************************************
 * fbe_parity_zero_state1()
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

fbe_raid_state_status_t fbe_parity_zero_state1(fbe_raid_siots_t * siots_p)
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

    /* Refresh our view in case we quiesced and then became degraded.
     */
    fbe_parity_check_for_degraded(siots_p);

    /* Now check for allocation failure
     */
    if (RAID_MEMORY_COND(!fbe_raid_siots_is_deferred_allocation_successful(siots_p)))
    {
        /* Trace some information and transition to allocation error
         */
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: zero memory allocation error common flags: 0x%x \n",
                             fbe_raid_common_get_flags(&siots_p->common));
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_memory_allocation_error);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* Then calculate how many buffers are needed.
     */
    status = fbe_parity_zero_get_fruts_info(siots_p, &write_info[0]);
    if (RAID_COND(status != FBE_STATUS_OK))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                             "parity: zero failed to get fru info with status 0x%x, "
                             "siots_p = 0x%p, write_info_p 0x%p\n",
                             status, siots_p, &write_info[0]);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* Allocation completed. Plant the resources
     */
    status = fbe_parity_zero_setup_resources(siots_p, &write_info[0]);
    if (RAID_COND(status != FBE_STATUS_OK))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity: zero failed to setup resources status: 0x%x algorithm: 0x%x\n", 
                                            status, siots_p->algorithm);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* Transition to next state to issue request
     */
    fbe_raid_siots_transition(siots_p, fbe_parity_zero_state3);

    /* Return the state status
     */
    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/*******************************
 * end fbe_parity_zero_state1()
 *******************************/

/****************************************************************
 * fbe_parity_zero_state3()
 ****************************************************************
 * @brief
 *  This function issues zeros to the drives.
 *
 * @param siots_p - current sub request 
 *
 * @return
 *  This function returns FBE_RAID_STATE_STATUS_WAITING if we were unable
 *  to allocate the required number of structures immediately;
 *  otherwise, FBE_RAID_STATE_STATUS_EXECUTING is returned.
 *
 * @author
 *  05/04/04 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_raid_state_status_t fbe_parity_zero_state3(fbe_raid_siots_t * siots_p)
{
    fbe_status_t status;
    fbe_raid_fruts_t *write_fruts_p = NULL;
    fbe_u16_t active_bitmap = 0;
    fbe_bool_t b_result = FBE_TRUE;
    fbe_bool_t b_generate_next_siots = FBE_FALSE;
    fbe_raid_iots_t *iots_p = fbe_raid_siots_get_iots(siots_p);

    fbe_raid_siots_check_traffic_trace(siots_p, FBE_TRUE);

    fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_p);

    if (fbe_raid_siots_is_aborted(siots_p))
    {
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_aborted);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* Set all degraded positions to nop.
     */
    fbe_raid_fruts_set_degraded_nop(siots_p, write_fruts_p);

    siots_p->wait_count = fbe_raid_fruts_get_chain_bitmap(write_fruts_p, &active_bitmap);

    /* We already setup the wait count when we setup the fruts.
     */
    if (siots_p->wait_count == 0)
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity: zero unexpected wait_count: %lld\n", siots_p->wait_count);
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

    fbe_raid_siots_transition(siots_p, fbe_parity_zero_state4);
    fbe_raid_siots_set_flag(siots_p, FBE_RAID_SIOTS_FLAG_WRITE_STARTED);

    /* Just send out all zeros.
     */
    b_result = fbe_raid_fruts_send_chain(&siots_p->write_fruts_head, fbe_raid_fruts_evaluate);
    if (RAID_FRUTS_COND(b_result != FBE_TRUE))
    {
        /* We hit an issue while starting chain of fruts. 
         */
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity: %s failed to start write fruts chain for "
                                            "siots = 0x%p\n",
                                            __FUNCTION__,
                                            siots_p);

    }
    if (b_generate_next_siots)
    {
        fbe_raid_iots_generate_next_siots_immediate(iots_p);
    }

    return FBE_RAID_STATE_STATUS_WAITING;
}
/*****************************************
 * end fbe_parity_zero_state3() 
 *****************************************/

/****************************************************************
 * fbe_parity_zero_state4()
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
 *  3/5/2010 - Created. Rob Foley
 *  
 ****************************************************************/

fbe_raid_state_status_t fbe_parity_zero_state4(fbe_raid_siots_t * siots_p)
{
    fbe_raid_fru_eboard_t eboard;
    fbe_raid_fru_error_status_t fruts_status;
    fbe_raid_state_status_t state_status = FBE_RAID_STATE_STATUS_EXECUTING;
    fbe_raid_fruts_t *write_fruts_p = NULL;

    fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_p);
    fbe_raid_fru_eboard_init(&eboard);

    /* Get totals for errored or dead drives.
     */
    fruts_status = fbe_parity_get_fruts_error(write_fruts_p, &eboard);
    if (fruts_status == FBE_RAID_FRU_ERROR_STATUS_SUCCESS)
    {
        /* All writes are successful.
         */
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_success);
    }
    else if (fruts_status == FBE_RAID_FRU_ERROR_STATUS_RETRY)
    {
        fbe_raid_state_status_t state_status;
         /* We might have gone degraded and have a retry.
          * In this case it is necessary to mark the degraded positions as nops since 
          * 1) we already marked the paged for these 
          * 2) the next time we run through this state we do not want to consider these failures 
          *    (and wait for continue) since we already got the continue. 
          */
        fbe_raid_fruts_set_degraded_nop(siots_p, write_fruts_p);
        state_status = fbe_parity_handle_fruts_error(siots_p, write_fruts_p, &eboard, fruts_status);
        return state_status;
    }
    else if ((fruts_status == FBE_RAID_FRU_ERROR_STATUS_SHUTDOWN) ||
             (fruts_status == FBE_RAID_FRU_ERROR_STATUS_DEAD) ||
             (fruts_status == FBE_RAID_FRU_ERROR_STATUS_WAITING))
    {
        fbe_raid_state_status_t state_status;
        state_status = fbe_parity_handle_fruts_error(siots_p, write_fruts_p, &eboard, fruts_status);
        return state_status;
    }
    else if (fruts_status == FBE_RAID_FRU_ERROR_STATUS_ERROR)
    {
        /* We got dead error in at least one of the fruts,
         * check if we can retry the errored fruts here.
         */
        if (fbe_raid_geometry_is_raid6(fbe_raid_siots_get_raid_geometry(siots_p)))
        {
            /* Raid 6 is allowed up to 2 dead errors.
             */
            if (RAID_COND(eboard.dead_err_count > 2))
            {
                fbe_raid_siots_set_unexpected_error(siots_p,
                                                    FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                    "parity: eboard.dead_err_count 0x%x > 2\n",
                                                    eboard.dead_err_count);
                fbe_raid_fru_eboard_display(&eboard, siots_p, FBE_TRACE_LEVEL_ERROR);
                return FBE_RAID_STATE_STATUS_EXECUTING;
            }
            
            /* Dead error means that we didn't complete zeroing
             * but any errored drive(s) will be rebuilt, so
             * it is allowed for us to return good status.
             */
            fbe_raid_siots_transition(siots_p, fbe_raid_siots_success);
        }
        else if (fbe_raid_geometry_is_parity_type(fbe_raid_siots_get_raid_geometry(siots_p)))
        {
            /* Raid 3 and Raid 5 come through this path.
             */
            if ( eboard.dead_err_count != 1 )
            {
                /* We should be shutdown.
                 */
                fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                    "parity: zero unexpected fru error status: %d dead_err_count:%d \n", 
                                                    fruts_status, eboard.dead_err_count);
                fbe_raid_fru_eboard_display(&eboard, siots_p, FBE_TRACE_LEVEL_ERROR);
                return FBE_RAID_STATE_STATUS_EXECUTING;
            }
            
            /* A Raid5 Zero took a dead error.
             * This is okay, since this will be rebuild, just continue.
             */
            fbe_raid_siots_transition(siots_p, fbe_raid_siots_success);
        }
        else 
        {
            /* No hard errors on writes.
             * No Drops allowed on writes.  If we get any error its a fbe_panic.
             */
            fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "parity: zero unexpected fru error status: %d\n", fruts_status);
            fbe_raid_fru_eboard_display(&eboard, siots_p, FBE_TRACE_LEVEL_ERROR);
            return FBE_RAID_STATE_STATUS_EXECUTING;
        }
    } /*end else if (fruts_status == FBE_RAID_FRU_ERROR_STATUS_ERROR)*/
    else
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity: zero unexpected fru error status: %d\n", fruts_status);
        fbe_raid_fru_eboard_display(&eboard, siots_p, FBE_TRACE_LEVEL_ERROR);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }
    
    return state_status;
}                               
/*****************************************
 *  end of fbe_parity_zero_state4() 
 *****************************************/

/*!***************************************************************
 * fbe_parity_zero_get_fruts_info()
 ****************************************************************
 * @brief
 *  This function initializes the fru info for verify.
 *
 * @param siots_p - Current sub request.
 *
 * @return fbe_status_t
 *
 * @author
 *  3/5/2010 - Created. Rob Foley
 * 
 ****************************************************************/

static fbe_status_t fbe_parity_zero_get_fruts_info(fbe_raid_siots_t * siots_p,
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
 * end fbe_parity_zero_get_fruts_info
 **************************************/
/***************************************************************************
 * fbe_parity_zero_setup_fruts()
 ***************************************************************************
 * @brief 
 *  This function initializes the fruts structures for a parity zero.
 *
 * @param siots_p - pointer to SIOTS data
 * @param write_p - Array of write information.
 * @param memory_info_p - Pointer to memory request information
 *
 * @return fbe_status_t
 *
 * @author
 *  3/5/2010 - Created. Rob Foley
 *
 **************************************************************************/

static fbe_status_t fbe_parity_zero_setup_fruts(fbe_raid_siots_t * const siots_p, 
                                                fbe_raid_fru_info_t *write_p,
                                                fbe_raid_memory_info_t *memory_info_p)
{
    fbe_u16_t data_pos = 0;           /* data position in array */
    fbe_status_t status = FBE_STATUS_OK;
    fbe_payload_block_operation_opcode_t opcode;

    fbe_raid_siots_get_opcode(siots_p, &opcode);

    /* When encryption does a zero it needs the zero to get sent to the lower level. 
     */
    if (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_REKEY_ZERO) {
        opcode = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ZERO;
    }
    /* Setup one write same for each drive.
     */
    for (data_pos = 0;
         data_pos < fbe_raid_siots_get_width(siots_p);
         data_pos++)
    {
        status = fbe_raid_setup_fruts_from_fru_info(siots_p,
                                                    write_p,
                                                    &siots_p->write_fruts_head,
                                                    opcode,
                                                    memory_info_p);
        if (status != FBE_STATUS_OK){ return status; }

        write_p++;
    } /* While data_pos < width */

    return status;
}
/**************************************
 * end fbe_parity_zero_setup_fruts()
 **************************************/

/*!**************************************************************************
 * fbe_parity_zero_calculate_memory_size()
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
 *  3/5/2010 - Created. Rob Foley
 *
 **************************************************************************/

static fbe_status_t fbe_parity_zero_calculate_memory_size(fbe_raid_siots_t *siots_p,
                                                         fbe_raid_fru_info_t *write_info_p)
{
    fbe_u32_t num_fruts = 0;
    fbe_status_t status;

    /* Then calculate how many buffers are needed.
     */
    status = fbe_parity_zero_get_fruts_info(siots_p, write_info_p);
    if (status != FBE_STATUS_OK) { return status; }

    siots_p->total_blocks_to_allocate = 0;
    /* First start off with 1 fruts for every data position we are writing 
     * and for every parity position. 
     */
    num_fruts = fbe_raid_siots_get_width(siots_p);
    fbe_raid_siots_set_optimal_page_size(siots_p,
                                         num_fruts, siots_p->total_blocks_to_allocate);

    /* Take the information we calculated above and determine how this translates 
     * into a number of pages. 
     */
    status = fbe_raid_siots_calculate_num_pages(siots_p, num_fruts, NULL, /* No Sgs */
                                                FBE_FALSE /* No nested siots required*/);
    return status;
}
/**************************************
 * end fbe_parity_zero_calculate_memory_size()
 **************************************/
/****************************************************************
 * fbe_parity_zero_setup_resources()
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
 *  3/5/2010 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_parity_zero_setup_resources(fbe_raid_siots_t * siots_p,
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
                             "parity: failed to init memory info with status: 0x%x\n",
                             status);
        return(status); 
    }

    /* Initialize the FRU_TS objects, releasing any
     * which may have been allocated but are not used.
     */
    status = fbe_parity_zero_setup_fruts(siots_p, write_p, &memory_info);
    if (RAID_COND(status != FBE_STATUS_OK))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: failed to setup zero fruts with status: 0x%x\n",
                             status);
        return(status); 
    }

    return status;
}
/****************************
 *  end of fbe_parity_zero_setup_resources()
 ****************************/
/************************************************************
 * end fbe_parity_zero.c
 ************************************************************/
