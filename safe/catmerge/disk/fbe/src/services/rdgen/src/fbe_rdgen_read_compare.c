/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_rdgen_read_compare.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the rdgen read and compare state machine.
 *  This operation simply reads a block and compares this block
 *  to a known pattern.
 *
 * @version
 *   9/15/2009 - Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_rdgen.h"
#include "fbe_rdgen_private_inlines.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/
/*!**************************************************************
 * fbe_rdgen_ts_read_compare_state0()
 ****************************************************************
 * @brief
 *  This is the first state of the write/read/compare state machine.
 *  We initialize the range of this ts.
 *
 * @param ts_p - current_ts.               
 *
 * @return FBE_RDGEN_TS_STATE_STATUS_EXECUTING   
 *
 * @author
 *  9/15/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_rdgen_ts_state_status_t fbe_rdgen_ts_read_compare_state0(fbe_rdgen_ts_t *ts_p)
{
    fbe_status_t status;

    if (fbe_rdgen_ts_generate(ts_p) == FALSE)
    {
        /* Some error, transition to error state.
         */
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, ts_p->object_p->object_id,
                                "%s: ts generate failed\n", __FUNCTION__);
        fbe_rdgen_ts_set_unexpected_error_status(ts_p);
        fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_finished);
        return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
    }

    /* If the number of I/Os we were programmed with has been satisfied, then 
     * exit. 
     */
    if (fbe_rdgen_ts_is_complete(ts_p))
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, ts_p->object_p->object_id,
                            "%s: fbe_rdgen_ts_is_complete for object_id: 0x%x\n", 
                            __FUNCTION__, ts_p->object_p->object_id);
        fbe_rdgen_ts_set_complete_status(ts_p);
        fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_finished);
        return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
    }

    fbe_rdgen_ts_clear_flag(ts_p, FBE_RDGEN_TS_FLAGS_FIRST_REQUEST);
    fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_read_compare_state1);

    if (!fbe_rdgen_specification_options_is_set(&ts_p->request_p->specification, FBE_RDGEN_OPTIONS_PERFORMANCE)) {
        /* Since we are comparing what we are reading, we need to check for conflicts.
         */
        status = fbe_rdgen_ts_resolve_conflict(ts_p);
        if (status == FBE_STATUS_GENERIC_FAILURE) {
            /*! Unexpected error in resolve conflict, complete this ts with error.
             */
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, ts_p->object_p->object_id,
                                    "%s: resolve conflict failed\n", __FUNCTION__);
            fbe_rdgen_ts_set_unexpected_error_status(ts_p);
            fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_finished);
            return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
        } else if (status == FBE_STATUS_PENDING) {
            return FBE_RDGEN_TS_STATE_STATUS_WAITING;
        }
    }
    return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
}
/******************************************
 * end fbe_rdgen_ts_read_compare_state0()
 ******************************************/
/*!**************************************************************
 * fbe_rdgen_ts_read_compare_state1()
 ****************************************************************
 * @brief
 *  This is the state where we have our lock and we setup
 *  our blocks.
 *
 * @param ts_p - current_ts.               
 *
 * @return FBE_RDGEN_TS_STATE_STATUS_EXECUTING   
 *
 * @author
 *  10/12/2011 - Created. Rob Foley
 *
 ****************************************************************/

fbe_rdgen_ts_state_status_t fbe_rdgen_ts_read_compare_state1(fbe_rdgen_ts_t *ts_p)
{
    /* The above may have modified our ts_p->lba and ts_p->blocks to avoid conflict. 
     */
    fbe_rdgen_ts_initialize_blocks_remaining(ts_p);

    if (fbe_rdgen_ts_should_send_to_peer(ts_p))
    {
        return fbe_rdgen_ts_send_to_peer(ts_p);
    }
    else
    {
        fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_read_compare_state2);
    }
    return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
}
/******************************************
 * end fbe_rdgen_ts_read_compare_state1()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_ts_read_compare_state2()
 ****************************************************************
 * @brief
 *  Initialize the breakup count and our current blocks.
 * 
 *  This is the top of the write loop.
 *
 * @param ts_p - current_ts.               
 *
 * @return FBE_RDGEN_TS_STATE_STATUS_EXECUTING   
 *
 * @author
 *  9/15/2009 - Created. Rob Foley
 *
 ****************************************************************/
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_read_compare_state2(fbe_rdgen_ts_t *ts_p)
{
    /* Determine how many times to breakup this request.
     */
    fbe_rdgen_ts_get_breakup_count(ts_p);

    /* Setup the current blocks based on the breakup count.
     */
    fbe_rdgen_ts_get_current_blocks(ts_p);

    fbe_rdgen_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                        FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                        "%s: new iorb lba: 0x%llx blocks: 0x%llx breakup_count: 0x%x object_id: 0x%x\n", __FUNCTION__, 
                        (unsigned long long)ts_p->lba,
			(unsigned long long)ts_p->blocks,
			ts_p->breakup_count, ts_p->object_p->object_id);

    fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_read_compare_state3);

    return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
}
/******************************************
 * end fbe_rdgen_ts_read_compare_state2()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_ts_read_compare_state3()
 ****************************************************************
 * @brief
 *  Setup the current blocks.
 *
 * @param ts_p - current_ts.               
 *
 * @return FBE_RDGEN_TS_STATE_STATUS_EXECUTING   
 *
 * @author
 *  9/15/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_rdgen_ts_state_status_t fbe_rdgen_ts_read_compare_state3(fbe_rdgen_ts_t *ts_p)
{
    /* Re-initialize our current lba and blocks. 
     */
    fbe_rdgen_ts_initialize_blocks_remaining(ts_p);

    /* Setup the current blocks based on the breakup count.
     */
    fbe_rdgen_ts_get_current_blocks(ts_p);

    fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_read_compare_state10);

    return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
}
/******************************************
 * end fbe_rdgen_ts_read_compare_state3()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_ts_read_compare_state10()
 ****************************************************************
 * @brief
 *  Allocate memory.
 *
 * @param ts_p - current_ts.               
 *
 * @return FBE_RDGEN_TS_STATE_STATUS_EXECUTING   
 *
 * @author
 *  9/15/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_rdgen_ts_state_status_t fbe_rdgen_ts_read_compare_state10(fbe_rdgen_ts_t *ts_p)
{
    fbe_rdgen_ts_state_status_t status;

    fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_read_compare_state11);

    /* Allocate memory.
     */
    status = fbe_rdgen_ts_allocate_memory(ts_p);
    return status;
}
/******************************************
 * end fbe_rdgen_ts_read_compare_state10()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_ts_read_compare_state11()
 ****************************************************************
 * @brief
 *  Issue the read operation.
 *  This is the top of the read loop.
 *
 * @param ts_p - current_ts.               
 *
 * @return FBE_RDGEN_TS_STATE_STATUS_EXECUTING   
 *
 * @author
 *  9/15/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_rdgen_ts_state_status_t fbe_rdgen_ts_read_compare_state11(fbe_rdgen_ts_t *ts_p)
{
    if (RDGEN_COND(ts_p->memory_request.ptr == NULL))
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, ts_p->object_p->object_id,
                                "%s: rdgen_ts sg pointer is null. \n", __FUNCTION__);
        fbe_rdgen_ts_set_unexpected_error_status(ts_p);
        fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_finished);
        return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
    }

    if (fbe_rdgen_ts_is_complete(ts_p))
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, ts_p->object_p->object_id,
                            "%s: fbe_rdgen_ts_is_complete for object_id: 0x%x\n", 
                            __FUNCTION__, ts_p->object_p->object_id);
        fbe_rdgen_ts_set_complete_status(ts_p);
        fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_finished);
        return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
    }
    if (fbe_rdgen_ts_is_aborted(ts_p))
    {
        /* Kill the thread, we are terminating.
         */
        fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_finished);
    }
    else
    {
        fbe_rdgen_ts_set_calling_state(ts_p, fbe_rdgen_ts_read_compare_state12);
        fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_read_state0);
    }

    return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
}
/******************************************
 * end fbe_rdgen_ts_read_compare_state11()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_ts_read_compare_state12()
 ****************************************************************
 * @brief
 *  Handle the status from the read operation.
 *  Start the check operation.
 *
 * @param ts_p - current_ts.               
 *
 * @return FBE_RDGEN_TS_STATE_STATUS_EXECUTING   
 *
 * @author
 *  9/15/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_rdgen_ts_state_status_t fbe_rdgen_ts_read_compare_state12(fbe_rdgen_ts_t *ts_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_bool_t b_panic;
    fbe_rdgen_ts_state_status_t error_status;

    if (fbe_rdgen_ts_is_aborted(ts_p))
    {
        /* Kill the thread, we are terminating.
         */
        fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_finished);
        return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
    }

    /* Interpret the error status from the read operation.
     * If we have more error handling to do, then exit now. 
     * fbe_rdgen_ts_handle_error() will already have set the next state.
     */
    error_status = fbe_rdgen_ts_handle_error(ts_p, fbe_rdgen_ts_read_compare_state11);
    if (error_status != FBE_RDGEN_TS_STATE_STATUS_DONE)
    {
        return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
    }
    /* A single I/O finished, increment the io counter.
     */
    fbe_rdgen_ts_inc_io_count(ts_p);

    /* Only panic if the do not panic options is not set.
     */
    b_panic = !fbe_rdgen_specification_options_is_set(&ts_p->request_p->specification,
                                                      FBE_RDGEN_OPTIONS_DO_NOT_PANIC_ON_DATA_MISCOMPARE);
    /* Check for the pattern in the buffer.
     */
    status = fbe_rdgen_check_sg_list(ts_p, 
                                     ts_p->request_p->specification.pattern,
                                     b_panic);
    if (status != FBE_STATUS_OK)
    {
        if(b_panic)
        {
            /* Panic - READ miscompare. CRITICAL ERROR takes care of panic. */
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, 
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "rdgen miscompare obj:0x%x, op:0x%x, lba:0x%llx, blk:0x%llx, curr_lba:0x%llx, curr_blk:0x%llx\n", 
                                    ts_p->object_p->object_id,
                                    ts_p->operation,
                                    (unsigned long long)ts_p->lba,
				    (unsigned long long)ts_p->blocks,
				    (unsigned long long)ts_p->current_lba, 
				    (unsigned long long)ts_p->current_blocks);
        }

        /* return is meaningless here since we are taking a PANIC. 
         * Disable or Remove this return when the PANIC is enabled. 
         */
        fbe_rdgen_ts_set_miscompare_status(ts_p);
        fbe_rdgen_ts_inc_error_count(ts_p);
        fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_finished);

        return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
    }

    fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_read_compare_state13);

    return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
}
/******************************************
 * end fbe_rdgen_ts_read_compare_state12()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_ts_read_compare_state13()
 ****************************************************************
 * @brief
 *  Read and check have completed.
 *  Check to see if we are done with the read/compares or if
 *  we have another loop to do.
 *
 * @param ts_p - current_ts.               
 *
 * @return FBE_RDGEN_TS_STATE_STATUS_EXECUTING   
 *
 * @author
 *  9/15/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_rdgen_ts_state_status_t fbe_rdgen_ts_read_compare_state13(fbe_rdgen_ts_t *ts_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    if (fbe_rdgen_ts_is_aborted(ts_p))
    {
        /* Kill the thread, we are terminating.
         */
        fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_finished);
        return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
    }

    status = fbe_rdgen_ts_update_blocks_remaining(ts_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, ts_p->object_p->object_id,
                                "%s: updt blks failed status: 0x%x\n", __FUNCTION__, status);
        fbe_rdgen_ts_set_unexpected_error_status(ts_p);
        fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_finished);
        return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
    }

    if (ts_p->blocks_remaining == 0)
    {
        if (fbe_rdgen_ts_is_time_to_trace(ts_p))
        {
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO, 
                                FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                "%s: object: 0x%x pass: 0x%x io_count:0x%x\n", 
                                __FUNCTION__, ts_p->object_p->object_id, ts_p->pass_count, ts_p->io_count);
        }

        /* Free up memory before we start the next pass.
         */
        fbe_rdgen_ts_free_memory(ts_p);

        /* Done with the read/check pass.
         * Increment the pass count and go to the top of the loop to start the next pass.
         */
        fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_read_compare_state0);
    }
    else
    {
        if (fbe_rdgen_ts_is_time_to_trace(ts_p))
        {
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO, 
                                FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                "%s: next r/c slice  object: 0x%x current lba: 0x%llx current blocks: 0x%llx "
                                "blocks remaining: 0x%llx\n", 
                                __FUNCTION__, ts_p->object_p->object_id,
                                (unsigned long long)ts_p->current_lba,
				(unsigned long long)ts_p->current_blocks,
				(unsigned long long)ts_p->blocks_remaining);
        }

        /* Done with this read/check pass, set state to go to start the next pass.
         */
        fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_read_compare_state11);
    }

    return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
}
/******************************************
 * end fbe_rdgen_ts_read_compare_state13()
 ******************************************/

/*************************
 * end file fbe_rdgen_read_compare.c
 *************************/


