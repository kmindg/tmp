/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_striper_zero.c
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
 *  3/8/2010 - Created. Rob Foley
 *
 ***************************************************************************/


/*************************
 ** INCLUDE FILES
 *************************/
#include "fbe_raid_library_proto.h"
#include "fbe_striper_io_private.h"

/********************************
 *      static STRING LITERALS
 ********************************/

/*****************************************************
 *      static FUNCTIONS DECLARED GLOBALLY FOR VISIBILITY
 *****************************************************/
static fbe_status_t fbe_striper_zero_get_fruts_info(fbe_raid_siots_t * siots_p,
                                                    fbe_raid_fru_info_t *write_p);
static fbe_status_t fbe_striper_zero_setup_fruts(fbe_raid_siots_t * const siots_p,
                                                 fbe_raid_fru_info_t *write_p,
                                                 fbe_raid_memory_info_t *memory_info_p);
static fbe_status_t fbe_striper_zero_calculate_memory_size(fbe_raid_siots_t *siots_p,
                                                           fbe_raid_fru_info_t *write_info_p);
static fbe_status_t fbe_striper_zero_setup_resources(fbe_raid_siots_t * siots_p,
                                                                fbe_raid_fru_info_t *write_p);
static fbe_raid_state_status_t fbe_striper_zero_state1(fbe_raid_siots_t *siots_p);
static fbe_raid_state_status_t fbe_striper_zero_state2(fbe_raid_siots_t *siots_p);
static fbe_raid_state_status_t fbe_striper_zero_state3(fbe_raid_siots_t *siots_p);
static fbe_raid_state_status_t fbe_striper_zero_state4(fbe_raid_siots_t *siots_p);

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
 * fbe_striper_zero_state0()
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
 *  3/8/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_striper_zero_state0(fbe_raid_siots_t * siots_p)
{
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_raid_fru_info_t     write_info[FBE_RAID_MAX_DISK_ARRAY_WIDTH];

    fbe_striper_zero_get_fruts_info(siots_p, &write_info[0]);
    /* Validate the algorithm.
     */
    if (siots_p->algorithm != RG_ZERO)
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "striper: zero algorithm %d unexpected, siots_p = 0x%p\n", 
                                            siots_p->algorithm,
                                            siots_p);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }
    status = fbe_striper_generate_validate_siots(siots_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "striper: validate failed algorithm: %d, siots_p = 0x%p\n",
                                            siots_p->algorithm,
                                            siots_p);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* Calculate and allocate all the memory needed for this siots.
     */
    status = fbe_striper_zero_calculate_memory_size(siots_p, &write_info[0]);
    if (status != FBE_STATUS_OK)
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "striper: %s failed to calculate mem size: "
                                            "status: 0x%x algorithm: 0x%x, siots_p = 0x%p\n", 
                                            __FUNCTION__,
                                            status, siots_p->algorithm,
                                            siots_p);
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
    fbe_raid_siots_transition(siots_p, fbe_striper_zero_state1);
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
        status = fbe_striper_zero_setup_resources(siots_p, &write_info[0]);
        if (RAID_COND(status != FBE_STATUS_OK))
        {
            fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "striper: zero failed to setup resources. status: 0x%x\n",
                                            status);
            return FBE_RAID_STATE_STATUS_EXECUTING;
        }

        /* To prevent confusion (i.e. we never went to `deferred allocation complete'
         * state) overwrite the current state.
         */
        fbe_raid_common_set_state(&siots_p->common, (fbe_raid_common_state_t)fbe_striper_zero_state3);
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
                                            "striper: zero memory request failed with status: 0x%x\n",
                                            status);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/***********************************
 *  end of fbe_striper_zero_state0()
 ***********************************/

/****************************************************************
 * fbe_striper_zero_state1()
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

fbe_raid_state_status_t fbe_striper_zero_state1(fbe_raid_siots_t * siots_p)
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
                             "striper: zero memory allocation error common flags: 0x%x \n",
                             fbe_raid_common_get_flags(&siots_p->common));
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_memory_allocation_error);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* Then calculate how many buffers are needed.
     */    
    status = fbe_striper_zero_get_fruts_info(siots_p, &write_info[0]);
    if (RAID_COND(status != FBE_STATUS_OK))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                             "striper: zero get fru info failed with status = 0x%x, siots_p = 0x%p\n",
                             status, siots_p);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* Allocation completed. Plant the resources
     */
    status = fbe_striper_zero_setup_resources(siots_p, &write_info[0]);
    if (RAID_COND(status != FBE_STATUS_OK))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "striper: zero failed to setup resources. status: 0x%x\n",
                                            status);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* Transition to next state to issue request
     */
    fbe_raid_siots_transition(siots_p, fbe_striper_zero_state3);

    /* Return the state status
     */
    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/***********************************
 * end fbe_striper_zero_state1()
 **********************************/

/****************************************************************
 * fbe_striper_zero_state3()
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
 *  3/8/2010 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_raid_state_status_t fbe_striper_zero_state3(fbe_raid_siots_t * siots_p)
{
    fbe_status_t status;
    fbe_bool_t b_result = FBE_TRUE;
    fbe_raid_fruts_t *write_fruts_p = NULL;
    fbe_bool_t b_generate_next_siots = FBE_FALSE;
    fbe_raid_iots_t *iots_p = fbe_raid_siots_get_iots(siots_p);

    fbe_raid_siots_check_traffic_trace(siots_p, FBE_TRUE);

    fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_p);

    if (fbe_raid_siots_is_aborted(siots_p))
    {
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_aborted);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    siots_p->wait_count = siots_p->data_disks;

    /* We already setup the wait count when we setup the fruts.
     */
    if (siots_p->wait_count == 0)
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "striper: siots_p->wait_count 0x%llx == 0\n", siots_p->wait_count);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* We should have at least (1) write fruts
     */
    if (RAID_COND(write_fruts_p == NULL))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "striper: no writes fruts and wait_count: %lld is not zero\n", 
                                            siots_p->wait_count);
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

    /* Set all degraded positions to nop.
     */
    fbe_raid_fruts_set_degraded_nop(siots_p, write_fruts_p);

    fbe_raid_siots_transition(siots_p, fbe_striper_zero_state4);
    fbe_raid_siots_set_flag(siots_p, FBE_RAID_SIOTS_FLAG_WRITE_STARTED);

    /* Just send out all zeros.
     */
    b_result = fbe_raid_fruts_send_chain(&siots_p->write_fruts_head, fbe_raid_fruts_evaluate);
    if (RAID_FRUTS_COND(b_result != FBE_TRUE))
    {
        if (b_generate_next_siots){ fbe_raid_iots_cancel_generate_next_siots(iots_p); }
        /* We hit an issue while starting chain of fruts.
         */
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "striper: %s failed to start write fruts chain for "
                                            "siots = 0x%p\n",
                                            __FUNCTION__,
                                            siots_p);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    if (b_generate_next_siots)
    {
        fbe_raid_iots_generate_next_siots_immediate(iots_p);
    }

    return FBE_RAID_STATE_STATUS_WAITING;
}
/*****************************************
 * end fbe_striper_zero_state3() 
 *****************************************/

/****************************************************************
 * fbe_striper_zero_state4()
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
 *  3/8/2010 - Created. Rob Foley
 *  
 ****************************************************************/

fbe_raid_state_status_t fbe_striper_zero_state4(fbe_raid_siots_t * siots_p)
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
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_success);
    }
    else if ((fruts_status == FBE_RAID_FRU_ERROR_STATUS_SHUTDOWN) ||
             (fruts_status == FBE_RAID_FRU_ERROR_STATUS_DEAD) ||
             (fruts_status == FBE_RAID_FRU_ERROR_STATUS_RETRY) ||
             (fruts_status == FBE_RAID_FRU_ERROR_STATUS_ABORTED) ||
             (fruts_status == FBE_RAID_FRU_ERROR_STATUS_WAITING) || 
             (fruts_status == FBE_RAID_FRU_ERROR_STATUS_NOT_PREFERRED) || 
             (fruts_status == FBE_RAID_FRU_ERROR_STATUS_REDUCE_QD_HARD) || 
             (fruts_status == FBE_RAID_FRU_ERROR_STATUS_REDUCE_QD_SOFT))
    {
        fbe_raid_state_status_t state_status;
        state_status = fbe_striper_handle_fruts_error(siots_p, write_fruts_p, &eboard, fruts_status);
        return state_status;
    }
    else
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "striper: zero unexpected fru error status: %d\n", fruts_status);
        fbe_raid_fru_eboard_display(&eboard, siots_p, FBE_TRACE_LEVEL_ERROR);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    return state_status;
}                               
/*****************************************
 *  end of fbe_striper_zero_state4() 
 *****************************************/

/*!***************************************************************
 * fbe_striper_zero_get_fruts_info()
 ****************************************************************
 * @brief
 *  This function initializes the fru info for verify.
 *
 * @param siots_p - Current sub request.
 *
 * @return fbe_status_t
 *
 * @author
 *  3/8/2010 - Created. Rob Foley
 * 
 ****************************************************************/

static fbe_status_t fbe_striper_zero_get_fruts_info(fbe_raid_siots_t * siots_p,
                                                    fbe_raid_fru_info_t *write_p)
{
    fbe_status_t status;
    status = fbe_striper_write_get_fruts_info(siots_p,
                                              NULL, /* no pre-reads */
                                              write_p,
                                              NULL, /* no read blocks */
                                              NULL  /* no read fruts count */ );
    return status;
}
/**************************************
 * end fbe_striper_zero_get_fruts_info
 **************************************/
/***************************************************************************
 * fbe_striper_zero_setup_fruts()
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
 *  3/8/2010 - Created. Rob Foley
 *
 **************************************************************************/

static fbe_status_t fbe_striper_zero_setup_fruts(fbe_raid_siots_t * const siots_p, 
                                                 fbe_raid_fru_info_t *write_p,
                                                 fbe_raid_memory_info_t *memory_info_p)
{
    fbe_u16_t data_pos;    /* data position in array */
    fbe_status_t status = FBE_STATUS_OK;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_payload_block_operation_opcode_t opcode;

    fbe_raid_siots_get_opcode(siots_p, &opcode);
    /* Setup one write same for each drive.
     */
    for (data_pos = 0;
        data_pos < siots_p->data_disks;
        data_pos++)
    {
        fbe_lba_t configured_capacity;
        fbe_u32_t width;
        fbe_raid_geometry_get_configured_capacity(raid_geometry_p, &configured_capacity);
        fbe_raid_geometry_get_width(raid_geometry_p, &width);

        if ((write_p->lba + write_p->blocks) > (configured_capacity / width))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "write_p->lba 0x%llx + write_p->blocks 0x%llx >"
                                 "(capacity %llx / width %d) \n", 
                                 (unsigned long long)write_p->lba,
				 (unsigned long long)write_p->blocks,
                                 (unsigned long long)configured_capacity, width);
        }
        status = fbe_raid_setup_fruts_from_fru_info(siots_p,
                                                    write_p,
                                                    &siots_p->write_fruts_head,
                                                    opcode,
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
 * end fbe_striper_zero_setup_fruts()
 **************************************/

/*!**************************************************************************
 * fbe_striper_zero_calculate_memory_size()
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
 *  3/8/2010 - Created. Rob Foley
 *
 **************************************************************************/

static fbe_status_t fbe_striper_zero_calculate_memory_size(fbe_raid_siots_t *siots_p,
                                                           fbe_raid_fru_info_t *write_info_p)
{
    fbe_u32_t num_fruts = 0;
    fbe_status_t status;

    /* Then calculate how many buffers are needed.
     */
    status = fbe_striper_zero_get_fruts_info(siots_p, write_info_p);
    if (status != FBE_STATUS_OK)
    {
        return status;
    }

    siots_p->total_blocks_to_allocate = 0;
    /* First start off with 1 fruts for every data position we are writing 
     * and for every position. 
     */
    num_fruts = siots_p->data_disks;
    fbe_raid_siots_set_optimal_page_size(siots_p,
                                         num_fruts, siots_p->total_blocks_to_allocate);

    /* Take the information we calculated above and determine how this translates 
     * into a number of pages. 
     */
    status = fbe_raid_siots_calculate_num_pages(siots_p, num_fruts, NULL,    /* No Sgs */
                                                FBE_FALSE /* No nested siots is required*/);
    return status;
}
/**************************************
 * end fbe_striper_zero_calculate_memory_size()
 **************************************/
/****************************************************************
 * fbe_striper_zero_setup_resources()
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
 *  3/8/2010 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_striper_zero_setup_resources(fbe_raid_siots_t * siots_p,
                                                     fbe_raid_fru_info_t *write_p)
{
    fbe_status_t            status;
    fbe_raid_memory_info_t  memory_info;

    /* Initialize our memory request information
     */
    status = fbe_raid_siots_init_control_mem_info(siots_p, &memory_info);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "striper: failed to init memory info with status: 0x%x\n",
                             status);
        return(status); 
    }

    /* Initialize the FRU_TS objects, releasing any
     * which may have been allocated but are not used.
     */
    status = fbe_striper_zero_setup_fruts(siots_p, write_p, &memory_info);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "striper: zero failed to setup fruts with status: 0x%x\n",
                             status);
        return(status); 
    }

    return status;
}
/****************************
 *  end of fbe_striper_zero_setup_resources()
 ****************************/
/************************************************************
 * end fbe_striper_zero.c
 ************************************************************/
