/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_mirror_zero.c
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
 *  3/9/2010 - Created. Rob Foley
 *
 ***************************************************************************/


/*************************
 ** INCLUDE FILES
 *************************/
#include "fbe_raid_library_proto.h"
#include "fbe_mirror_io_private.h"

/********************************
 *      static STRING LITERALS
 ********************************/

/*****************************************************
 *      static FUNCTIONS DECLARED GLOBALLY FOR VISIBILITY
 *****************************************************/
static fbe_status_t fbe_mirror_zero_get_fruts_info(fbe_raid_siots_t * siots_p,
                                                    fbe_raid_fru_info_t *write_p);
static fbe_status_t fbe_mirror_zero_setup_fruts(fbe_raid_siots_t * const siots_p,
                                                fbe_raid_fru_info_t *write_p,
                                                fbe_raid_memory_info_t *memory_info_p);
static fbe_status_t fbe_mirror_zero_calculate_memory_size(fbe_raid_siots_t *siots_p,
                                                           fbe_raid_fru_info_t *write_info_p);
static fbe_status_t fbe_mirror_zero_setup_resources(fbe_raid_siots_t * siots_p,
                                                                fbe_raid_fru_info_t *write_p);
static fbe_raid_state_status_t fbe_mirror_zero_state2(fbe_raid_siots_t *siots_p);
static fbe_raid_state_status_t fbe_mirror_zero_state3(fbe_raid_siots_t *siots_p);
static fbe_raid_state_status_t fbe_mirror_zero_state4(fbe_raid_siots_t *siots_p);

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
 * fbe_mirror_zero_state0()
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
 *  3/9/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_mirror_zero_state0(fbe_raid_siots_t * siots_p)
{
    fbe_status_t            status;
    fbe_raid_fru_info_t     write_info[FBE_RAID_MAX_DISK_ARRAY_WIDTH];
    fbe_raid_geometry_t    *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);

    /* Validate the algorithm.
     */
    if (siots_p->algorithm != RG_ZERO) {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "mirror zero algorithm %d unexpected\n", 
                                            siots_p->algorithm);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* Validate request is aligned.
     */
    if (fbe_raid_geometry_io_needs_alignment(raid_geometry_p,
                                             siots_p->start_lba,
                                             siots_p->xfer_count)) {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "mirror zero request is not aligned to optimal block size: %d\n",
                                            fbe_raid_geometry_get_default_optimal_block_size(raid_geometry_p));
        return FBE_RAID_STATE_STATUS_EXECUTING;

    }

    /* Common siots validation.
     */
    if (fbe_mirror_generate_validate_siots(siots_p) == FBE_FALSE) {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "mirror validate failed algorithm: %d\n",
                                            siots_p->algorithm);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* Generate the fru information
     */
    fbe_mirror_zero_get_fruts_info(siots_p, &write_info[0]);

    /* Calculate and allocate all the memory needed for this siots.
     */
    status = fbe_mirror_zero_calculate_memory_size(siots_p, &write_info[0]);
    if (status != FBE_STATUS_OK) {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "mirror failed to calculate mem size status: 0x%x algorithm: 0x%x\n", 
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
    fbe_raid_siots_transition(siots_p, fbe_mirror_zero_state1);
    status = fbe_raid_siots_allocate_memory(siots_p);

    /* Check if the allocation completed immediately.  If so plant the resources
     * now.  Else we will transition to `deferred allocation complete' state when 
     * the memory is available so return `waiting'
     */
    if (RAID_MEMORY_COND(status == FBE_STATUS_OK)) {
        /* Allocation completed immediately.  Plant the resources and then
         * transition to next state.
         */
        status = fbe_mirror_zero_setup_resources(siots_p, 
                                                 &write_info[0]);
        if (RAID_COND(status != FBE_STATUS_OK)) {
            fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "mirror: zero failed to setup resources. status: 0x%x\n",
                                            status);
            return FBE_RAID_STATE_STATUS_EXECUTING;
        }

        /* To prevent confusion (i.e. we never went to `deferred allocation complete'
         * state) overwrite the current state.
         */
        fbe_raid_common_set_state(&siots_p->common, (fbe_raid_common_state_t)fbe_mirror_zero_state3);

    } else if (RAID_MEMORY_COND(status == FBE_STATUS_PENDING)) {
        /* Else return waiting
         */
        return FBE_RAID_STATE_STATUS_WAITING;
    } else {
        /* Change the state to unexpected error and return
         */
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "mirror: zero memory request failed with status: 0x%x\n",
                                            status);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/**********************************
 *  end of fbe_mirror_zero_state0()
 **********************************/

/*!***************************************************************************
 *          fbe_mirror_zero_state1()
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
fbe_raid_state_status_t fbe_mirror_zero_state1(fbe_raid_siots_t *siots_p)
{
    fbe_raid_state_status_t state_status = FBE_RAID_STATE_STATUS_EXECUTING;
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_raid_fru_info_t     write_info[FBE_MIRROR_MAX_WIDTH];

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
                             "mirror: zero memory allocation error common flags: 0x%x \n",
                             fbe_raid_common_get_flags(&siots_p->common));
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_memory_allocation_error);
        return(state_status);
    }
    
    /* Now calculate the fruts information for each position including the
     * sg index if required.
     */
    status = fbe_mirror_zero_get_fruts_info(siots_p, 
                                            &write_info[0]);
    if (RAID_COND(status != FBE_STATUS_OK))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "mirror: zero get fru info failed with status: 0x%x\n",
                                            status);
        return(state_status);
    }

    /* Now plant the resources
     */
    status = fbe_mirror_zero_setup_resources(siots_p, 
                                             &write_info[0]);
    if (RAID_COND(status != FBE_STATUS_OK))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "mirror: zero failed to setup resources. status: 0x%x\n",
                                            status);
        return(state_status);
    }

    /* Else the resources were setup properly, so transition to
     * the next read state.
     */
    fbe_raid_siots_transition(siots_p, fbe_mirror_zero_state3);

    /* Always return the executing status.
     */
    return(state_status);
}   
/* end of fbe_mirror_zero_state1() */

/****************************************************************
 * fbe_mirror_zero_state3()
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
 *  3/9/2010 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_raid_state_status_t fbe_mirror_zero_state3(fbe_raid_siots_t * siots_p)
{
    fbe_status_t status;
    fbe_raid_state_status_t     state_status = FBE_RAID_STATE_STATUS_WAITING;
    fbe_u32_t                   width = fbe_raid_siots_get_width(siots_p);
    fbe_u32_t                   write_fruts_count = fbe_raid_siots_get_write_fruts_count(siots_p);
    fbe_bool_t b_result = FBE_TRUE;
    fbe_bool_t b_generate_next_siots = FBE_FALSE;
    fbe_raid_iots_t *iots_p = fbe_raid_siots_get_iots(siots_p);

    fbe_raid_siots_check_traffic_trace(siots_p, FBE_TRUE);

    /* If the data disks doesn't equal the width or the write fruts count 
     * something is wrong.
     */
    if ((siots_p->data_disks != width) ||
        (write_fruts_count   != width)    )
    {
        /* The siots isn't formed properly, error the request.
         */
        fbe_raid_siots_set_unexpected_error(siots_p,
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "mirror: zero - data_disks: 0x%x width: 0x%x and write fruts: 0x%x don't agree\n", siots_p->data_disks, width, write_fruts_count);
        return(FBE_RAID_STATE_STATUS_EXECUTING);
    }

    if (fbe_raid_siots_is_aborted(siots_p))
    {
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
    
    /* Set the wait count and issue the writes.
     */
    siots_p->wait_count = siots_p->data_disks;
    fbe_raid_siots_transition(siots_p, fbe_mirror_zero_state4);
    fbe_raid_siots_set_flag(siots_p, FBE_RAID_SIOTS_FLAG_WRITE_STARTED);
    b_result = fbe_raid_fruts_send_chain(&siots_p->write_fruts_head, fbe_raid_fruts_evaluate);
    if (RAID_FRUTS_COND(b_result != FBE_TRUE))
    {
        /* We hit an issue while starting chain of fruts.
         */
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "mirror: %s failed to start write fruts chain for "
                                            "siots_p 0x%p, b_result = %d\n",
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
/*****************************************
 * end fbe_mirror_zero_state3() 
 *****************************************/

/****************************************************************
 * fbe_mirror_zero_state4()
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
 *  3/9/2010 - Created. Rob Foley
 *  
 ****************************************************************/

fbe_raid_state_status_t fbe_mirror_zero_state4(fbe_raid_siots_t * siots_p)
{
    fbe_raid_state_status_t     state_status = FBE_RAID_STATE_STATUS_EXECUTING;
    fbe_raid_fru_eboard_t       fru_eboard;
    fbe_raid_fru_error_status_t fru_status;
    fbe_raid_fruts_t           *write_fruts_p = NULL;
    fbe_status_t                fbe_status = FBE_STATUS_OK;

    fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_p);
    
    fbe_raid_fru_eboard_init(&fru_eboard);
    
    /* Get totals for errored or dead drives.
     */
    fru_status = fbe_mirror_get_fruts_error(write_fruts_p, &fru_eboard);

    /* If zeros are successful determine if there is more work
     */
    if (fru_status == FBE_RAID_FRU_ERROR_STATUS_SUCCESS)
    {
        /* All writes are successful.
         */
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_success);
    }
    else if ((fru_status == FBE_RAID_FRU_ERROR_STATUS_SHUTDOWN) ||
             (fru_status == FBE_RAID_FRU_ERROR_STATUS_DEAD) ||
             (fru_status == FBE_RAID_FRU_ERROR_STATUS_RETRY) ||
             (fru_status == FBE_RAID_FRU_ERROR_STATUS_WAITING))
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
                /* Executed a degraded write.  Success status.
                 */
                fbe_raid_siots_transition(siots_p, fbe_raid_siots_success);
                break;

            case FBE_RAID_STATUS_RETRY_POSSIBLE:
                /* Send out retries.  We will stop retrying if we get aborted or 
                 * our timeout expires or if an edge goes away. 
                 */
                fbe_status = fbe_raid_siots_retry_fruts_chain(siots_p, &fru_eboard, write_fruts_p);
                if (fbe_status != FBE_STATUS_OK)
                { 
                    fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "mirror: retry %d errors (bitmap 0x%x) status: %d unexpected\n", 
                                                fru_eboard.retry_err_count, fru_eboard.retry_err_bitmap, fbe_status);
                    return(FBE_RAID_STATE_STATUS_EXECUTING);
                }
                state_status = FBE_RAID_STATE_STATUS_WAITING;
                break;

            case FBE_RAID_STATUS_OK:
                /* We don't expected success if an error was detected.
                 */
            default:
                /* Process fruts error reported an un-supported status.
                 */
                fbe_raid_siots_set_unexpected_error(siots_p, 
                                                    FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                    "mirror: zero unexpected raid_status: 0x%x from write process error\n",
                                                    raid_status);
                break;
        }

        /* Trace some informational text.
         */
        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_INFO,
                                    "mirror_zero_state4:lba:0x%llx bl:0x%llx raid_stat:0x%x statestat:0x%x retrycnt: 0x%x\n",
                                    (unsigned long long)siots_p->start_lba,
				    (unsigned long long)siots_p->xfer_count,
                                    raid_status, state_status,
				    siots_p->retry_count);
    }
    else
    {
        /* Else we either got an unexpected fru error status or the fru error
         * status reported isn't supported.
         */
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "mirror:zero siots lba:0x%llx blks:0x%llx unexpect fru stat:0x%x\n",
                                            (unsigned long long)siots_p->start_lba,
					    (unsigned long long)siots_p->xfer_count,
					    fru_status);
    }

    /* Always return the state status.
     */
    return(state_status);
}                               
/*****************************************
 *  end of fbe_mirror_zero_state4() 
 *****************************************/

/*!***************************************************************
 * fbe_mirror_zero_get_fruts_info()
 ****************************************************************
 * @brief
 *  This function initializes the fru info for verify.
 *
 * @param siots_p - Current sub request.
 *
 * @return fbe_status_t
 *
 * @author
 *  3/9/2010 - Created. Rob Foley
 * 
 ****************************************************************/

static fbe_status_t fbe_mirror_zero_get_fruts_info(fbe_raid_siots_t * siots_p,
                                                   fbe_raid_fru_info_t *write_p)
{
    fbe_u16_t data_pos;
    fbe_u8_t *position = siots_p->geo.position;

    /* Initialize the fru info for data drives.
     */
    for (data_pos = 0; data_pos < fbe_raid_siots_get_width(siots_p); data_pos++)
    {
        write_p->lba = siots_p->start_lba;
        write_p->blocks = siots_p->xfer_count;
        write_p->position = position[data_pos];
        write_p++;
    }
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_mirror_zero_get_fruts_info
 **************************************/
/***************************************************************************
 * fbe_mirror_zero_setup_fruts()
 ***************************************************************************
 * @brief 
 *  This function initializes the fruts structures for a mirror zero.
 *
 * @param siots_p - pointer to SIOTS data
 * @param write_p - Array of write information.
 * @param memory_info_p - Pointer to memory request information
 *
 * @return fbe_status_t
 *
 * @author
 *  3/9/2010 - Created. Rob Foley
 *
 **************************************************************************/

static fbe_status_t fbe_mirror_zero_setup_fruts(fbe_raid_siots_t * const siots_p, 
                                                 fbe_raid_fru_info_t *write_p,
                                                 fbe_raid_memory_info_t *memory_info_p)
{
    fbe_u16_t data_pos;    /* data position in array */
    fbe_status_t status = FBE_STATUS_OK;
    fbe_payload_block_operation_opcode_t opcode;

    fbe_raid_siots_get_opcode(siots_p, &opcode);

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
        if (status != FBE_STATUS_OK)
        {
            return status;
        }

        write_p++;
    }    /* While data_pos < width */

    return status;
}
/**************************************
 * end fbe_mirror_zero_setup_fruts()
 **************************************/

/*!**************************************************************************
 * fbe_mirror_zero_calculate_memory_size()
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
 *  3/9/2010 - Created. Rob Foley
 *
 **************************************************************************/

static fbe_status_t fbe_mirror_zero_calculate_memory_size(fbe_raid_siots_t *siots_p,
                                                           fbe_raid_fru_info_t *write_info_p)
{
    fbe_u32_t num_fruts = 0;
    fbe_status_t status;

    /* Then calculate how many buffers are needed.
     */
    status = fbe_mirror_zero_get_fruts_info(siots_p, write_info_p);
    if (status != FBE_STATUS_OK)
    {
        return status;
    }

    siots_p->total_blocks_to_allocate = 0;
    /* First start off with 1 fruts for every data position we are writing 
     * and for every position. 
     */
    num_fruts = fbe_raid_siots_get_width(siots_p);
    fbe_raid_siots_set_optimal_page_size(siots_p,
                                         num_fruts, 
                                         siots_p->total_blocks_to_allocate);

    /* Take the information we calculated above and determine how this translates 
     * into a number of pages. 
     */
    status = fbe_raid_siots_calculate_num_pages(siots_p, num_fruts, NULL,  /* No Sgs */
                                                FBE_FALSE /* No nested siots required*/);
    return status;
}
/**************************************
 * end fbe_mirror_zero_calculate_memory_size()
 **************************************/
/****************************************************************
 * fbe_mirror_zero_setup_resources()
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
 *  3/9/2010 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_mirror_zero_setup_resources(fbe_raid_siots_t * siots_p,
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
                             "mirror: failed to init memory info with status: 0x%x\n",
                             status);
        return(status); 
    }

    /* Initialize the FRU_TS objects, releasing any
     * which may have been allocated but are not used.
     */
    status = fbe_mirror_zero_setup_fruts(siots_p, write_p, &memory_info);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: zero failed to setup fruts with status: 0x%x\n",
                             status);
        return(status); 
    }

    return status;
}
/****************************
 *  end of fbe_mirror_zero_setup_resources()
 ****************************/
/************************************************************
 * end fbe_mirror_zero.c
 ************************************************************/
