/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_rdgen_write_read_compare.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the rdgen write read compare state machine.
 *  This is an operation that writes out a pattern,
 *  reads back, and compares that what we read matches what we wrote.
 *
 * @version
 *   7/22/2009:  Created. Rob Foley
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
 * fbe_rdgen_ts_write_read_compare_state0()
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
 *  7/22/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_rdgen_ts_state_status_t fbe_rdgen_ts_write_read_compare_state0(fbe_rdgen_ts_t *ts_p)
{
    fbe_status_t status;
    if (fbe_rdgen_ts_generate(ts_p) == FALSE)
    {
        /* Some error, transition to error state.
         */
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, fbe_rdgen_object_get_object_id(ts_p->object_p),
                                "%s: ts generate failed.\n", __FUNCTION__);
        fbe_rdgen_ts_set_unexpected_error_status(ts_p);
        fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_finished);
        return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
    }
    fbe_rdgen_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, fbe_rdgen_object_get_object_id(ts_p->object_p),
                            "rdgenwrstate0: 0x%x generate finished bl: 0x%llx pass: 0x%x/0x%x io_count:0x%x/0x%x lc: 0x%x\n", 
                            (fbe_u32_t)ts_p, (unsigned long long)ts_p->blocks, 
                            ts_p->pass_count, ts_p->peer_pass_count,
			    ts_p->io_count, ts_p->peer_io_count,
                            ts_p->lock_conflict_count);

    /* If the number of I/Os we were programmed with has been satisfied, then 
     * exit. 
     */
    if (fbe_rdgen_ts_is_complete(ts_p))
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, fbe_rdgen_object_get_object_id(ts_p->object_p),
                            "rdgenwrstate0: fbe_rdgen_ts_is_complete for object_id: 0x%x\n", 
                           ts_p->object_p->object_id);
        fbe_rdgen_ts_set_complete_status(ts_p);
        fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_finished);
        return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
    }
    if (fbe_rdgen_ts_is_time_to_trace(ts_p))
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO, fbe_rdgen_object_get_object_id(ts_p->object_p),
                                "rdgenwrstate0: 0x%x vlba/bl: 0x%llx/0x%llx pass: 0x%x/0x%x io_count:0x%x/0x%x\n", 
                                (fbe_u32_t)ts_p, (unsigned long long)ts_p->lba,
				(unsigned long long)ts_p->blocks,
				ts_p->pass_count, ts_p->peer_pass_count,
				ts_p->io_count, ts_p->peer_io_count);
    }
    
    fbe_rdgen_ts_clear_flag(ts_p, FBE_RDGEN_TS_FLAGS_FIRST_REQUEST);
    fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_write_read_compare_state1);

    /* Since we are modifying the media, we need to check for conflicts.
     */
    status = fbe_rdgen_ts_resolve_conflict(ts_p);
    if (status == FBE_STATUS_GENERIC_FAILURE)
    {
        /*! Unexpected error in resolve conflict, complete this ts with error.
         */
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, fbe_rdgen_object_get_object_id(ts_p->object_p),
                                "%s: resolve conflict failed\n", __FUNCTION__);
        fbe_rdgen_ts_set_unexpected_error_status(ts_p);
        fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_finished);
        return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
    }
    else if (status == FBE_STATUS_PENDING)
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, fbe_rdgen_object_get_object_id(ts_p->object_p),
                                "rdgenwrstate0: 0x%x wait lock blocks: 0x%llx pass: 0x%x/0x%x io_count:0x%x/0x%x lc: 0x%x fl:0x%x\n", 
                                (fbe_u32_t)ts_p,
				(unsigned long long)ts_p->blocks,
				ts_p->pass_count, ts_p->peer_pass_count, 
                                ts_p->io_count, ts_p->peer_io_count,
				ts_p->lock_conflict_count, ts_p->flags);
        return FBE_RDGEN_TS_STATE_STATUS_WAITING;
    }
    return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
}
/******************************************
 * end fbe_rdgen_ts_write_read_compare_state0()
 ******************************************/
/*!**************************************************************
 * fbe_rdgen_ts_write_read_compare_state1()
 ****************************************************************
 * @brief
 *  Our lock is obtained, initialize the range of the operation.
 *
 * @param ts_p - current_ts.               
 *
 * @return FBE_RDGEN_TS_STATE_STATUS_EXECUTING   
 *
 * @author
 *  10/12/2011 - Created. Rob Foley
 *
 ****************************************************************/

fbe_rdgen_ts_state_status_t fbe_rdgen_ts_write_read_compare_state1(fbe_rdgen_ts_t *ts_p)
{
    fbe_rdgen_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, fbe_rdgen_object_get_object_id(ts_p->object_p),
                            "rdgenwrstate0: 0x%x start blocks: 0x%llx pass: 0x%x/0x%x io_count:0x%x/0x%x lc: 0x%x fl:0x%x\n", 
                            (fbe_u32_t)ts_p, (unsigned long long)ts_p->blocks,
			    ts_p->pass_count, ts_p->peer_pass_count, 
                            ts_p->io_count, ts_p->peer_io_count,
			    ts_p->lock_conflict_count, ts_p->flags);
    /* The above may have modified our ts_p->lba and ts_p->blocks to avoid conflict. 
     */
    fbe_rdgen_ts_initialize_blocks_remaining(ts_p);

    if (fbe_rdgen_ts_should_send_to_peer(ts_p))
    {
        return fbe_rdgen_ts_send_to_peer(ts_p);
    }
    else
    {
    fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_write_read_compare_state2);
    }
    return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
}
/******************************************
 * end fbe_rdgen_ts_write_read_compare_state1()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_ts_write_read_compare_state2()
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
 *  7/22/2009 - Created. Rob Foley
 *
 ****************************************************************/
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_write_read_compare_state2(fbe_rdgen_ts_t *ts_p)
{
    /* Determine how many times to breakup this request.
     */
    fbe_rdgen_ts_get_breakup_count(ts_p);

    /* Setup the current blocks based on the breakup count.
     */
    fbe_rdgen_ts_get_current_blocks(ts_p);

    fbe_rdgen_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, fbe_rdgen_object_get_object_id(ts_p->object_p),
                        "%s: new iorb lba: 0x%llx blocks: 0x%llx breakup_count: 0x%x object_id: 0x%x\n", __FUNCTION__, 
                        (unsigned long long)ts_p->lba,
			(unsigned long long)ts_p->blocks,
			ts_p->breakup_count, ts_p->object_p->object_id);

    fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_write_read_compare_state3);

    return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
}
/******************************************
 * end fbe_rdgen_ts_write_read_compare_state2()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_ts_write_read_compare_state3()
 ****************************************************************
 * @brief
 *  Allocate memory.
 *
 * @param ts_p - current_ts.               
 *
 * @return FBE_RDGEN_TS_STATE_STATUS_EXECUTING   
 *
 * @author
 *  7/22/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_rdgen_ts_state_status_t fbe_rdgen_ts_write_read_compare_state3(fbe_rdgen_ts_t *ts_p)
{
    fbe_rdgen_ts_state_status_t status;
    fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_write_read_compare_state10);

    /* Allocate memory.
     */
    status = fbe_rdgen_ts_allocate_memory(ts_p);

    /* Just return the status we received.
     */
    return status;
}
/******************************************
 * end fbe_rdgen_ts_write_read_compare_state3()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_ts_write_read_compare_state10()
 ****************************************************************
 * @brief
 *  Start the write operation.
 *
 * @param ts_p - current_ts.               
 *
 * @return FBE_RDGEN_TS_STATE_STATUS_EXECUTING   
 *
 * @author
 *  7/22/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_rdgen_ts_state_status_t fbe_rdgen_ts_write_read_compare_state10(fbe_rdgen_ts_t *ts_p)
{
    if (RDGEN_COND(ts_p->memory_request.ptr == NULL) &&
        RDGEN_COND(ts_p->memory_p == NULL))
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, fbe_rdgen_object_get_object_id(ts_p->object_p),
                                "%s: rdgen_ts memory pointer is null. \n", __FUNCTION__);
        fbe_rdgen_ts_set_unexpected_error_status(ts_p);
        fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_finished);
        return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
    }
    if (fbe_rdgen_ts_is_complete(ts_p))
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, fbe_rdgen_object_get_object_id(ts_p->object_p),
                            "rdgenwrstate10: fbe_rdgen_ts_is_complete for object_id: 0x%x\n", 
                           ts_p->object_p->object_id);
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
    else if (ts_p->operation == FBE_RDGEN_OPERATION_ZERO_READ_CHECK)
    {
        fbe_rdgen_ts_set_calling_state(ts_p, fbe_rdgen_ts_write_read_compare_state11);
        fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_zero_state0);
    }
    else
    {
        fbe_rdgen_ts_set_calling_state(ts_p, fbe_rdgen_ts_write_read_compare_state11);
        fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_write_state0);
    }

    return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
}
/******************************************
 * end fbe_rdgen_ts_write_read_compare_state10()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_ts_write_read_compare_state11()
 ****************************************************************
 * @brief
 *  Handle the status from the write operation.
 *  If we are done with all writes, then transition to do reads.
 *  Otherwise go back to top of write loop.
 *
 * @param ts_p - current_ts.               
 *
 * @return FBE_RDGEN_TS_STATE_STATUS_EXECUTING   
 *
 * @author
 *  7/22/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_rdgen_ts_state_status_t fbe_rdgen_ts_write_read_compare_state11(fbe_rdgen_ts_t *ts_p)
{
    fbe_rdgen_ts_state_status_t error_status;
    fbe_status_t status = FBE_STATUS_OK;
    if (fbe_rdgen_ts_is_aborted(ts_p))
    {
        /* Kill the thread, we are terminating.
         */
        fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_finished);
        return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
    }
    
    /* Interpret the error status from the write operation.
     * If we have more error handling to do, then exit now. 
     * fbe_rdgen_ts_handle_error() will already have set the next state.
     */
    error_status = fbe_rdgen_ts_handle_error(ts_p, fbe_rdgen_ts_write_read_compare_state10);
    if (error_status != FBE_RDGEN_TS_STATE_STATUS_DONE)
    {
        return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
    }
    /* A single I/O finished, increment the io counter.
     */
    fbe_rdgen_ts_inc_io_count(ts_p);

    status = fbe_rdgen_ts_update_blocks_remaining(ts_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, fbe_rdgen_object_get_object_id(ts_p->object_p),
                                "%s: update blocks failed\n", __FUNCTION__);
        fbe_rdgen_ts_set_unexpected_error_status(ts_p);
        fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_finished);
        return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
    }

    if (ts_p->blocks_remaining == 0)
    {
        /* @todo - Check for errors here before going to read.
         */

        /* Done with the writes, transition to start reading.
         */
        fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_write_read_compare_state20);
    }
    else
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, fbe_rdgen_object_get_object_id(ts_p->object_p),
                            "%s: next write slice  current lba: 0x%llx current blocks: 0x%llx blocks remaining: 0x%llx object_id: 0x%x\n", __FUNCTION__, 
                            (unsigned long long)ts_p->current_lba,
			    (unsigned long long)ts_p->current_blocks,
			    (unsigned long long)ts_p->blocks_remaining,
                            ts_p->object_p->object_id);
        /* Continue writing, as we have more blocks to write.
         */
        fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_write_read_compare_state10);
    }

    return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
}
/******************************************
 * end fbe_rdgen_ts_write_read_compare_state11()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_ts_write_read_compare_state20()
 ****************************************************************
 * @brief
 *  Setup the read operation.  This is the top of the read loop.
 *
 * @param ts_p - current_ts.               
 *
 * @return FBE_RDGEN_TS_STATE_STATUS_EXECUTING   
 *
 * @author
 *  7/22/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_rdgen_ts_state_status_t fbe_rdgen_ts_write_read_compare_state20(fbe_rdgen_ts_t *ts_p)
{
    /* Re-initialize our current lba and blocks. 
     */
    fbe_rdgen_ts_initialize_blocks_remaining(ts_p);

    /* Setup the current blocks based on the breakup count.
     */
    fbe_rdgen_ts_get_current_blocks(ts_p);

    fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_write_read_compare_state21);

    return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
}
/******************************************
 * end fbe_rdgen_ts_write_read_compare_state20()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_ts_write_read_compare_state21()
 ****************************************************************
 * @brief
 *  Issue the read operation.
 *
 * @param ts_p - current_ts.               
 *
 * @return FBE_RDGEN_TS_STATE_STATUS_EXECUTING   
 *
 * @author
 *  7/22/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_rdgen_ts_state_status_t fbe_rdgen_ts_write_read_compare_state21(fbe_rdgen_ts_t *ts_p)
{
    if (RDGEN_COND(ts_p->memory_request.ptr == NULL) &&
        RDGEN_COND(ts_p->memory_p == NULL))
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, fbe_rdgen_object_get_object_id(ts_p->object_p),
                                "%s: rdgen_ts memory pointer is null. \n", __FUNCTION__);
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
        fbe_rdgen_ts_set_calling_state(ts_p, fbe_rdgen_ts_write_read_compare_state22);
        fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_read_state0);
    }

    return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
}
/******************************************
 * end fbe_rdgen_ts_write_read_compare_state21()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_ts_write_read_compare_state22()
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
 *  7/22/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_rdgen_ts_state_status_t fbe_rdgen_ts_write_read_compare_state22(fbe_rdgen_ts_t *ts_p)
{
    fbe_rdgen_ts_state_status_t error_status;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_bool_t b_panic;

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
    error_status = fbe_rdgen_ts_handle_error(ts_p, fbe_rdgen_ts_write_read_compare_state21);
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
                                     b_panic );
    if (status != FBE_STATUS_OK)
    {
        fbe_rdgen_ts_set_miscompare_status(ts_p);
        fbe_rdgen_ts_inc_error_count(ts_p);
        fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_finished);
        return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
    }

    fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_write_read_compare_state23);

    return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
}
/******************************************
 * end fbe_rdgen_ts_write_read_compare_state22()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_ts_write_read_compare_state23()
 ****************************************************************
 * @brief
 *  Read and check have completed.
 *  Check to see if we are done with the read/compares.
 *
 * @param ts_p - current_ts.               
 *
 * @return FBE_RDGEN_TS_STATE_STATUS_EXECUTING   
 *
 * @author
 *  7/22/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_rdgen_ts_state_status_t fbe_rdgen_ts_write_read_compare_state23(fbe_rdgen_ts_t *ts_p)
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
        fbe_rdgen_ts_set_status(ts_p, FBE_STATUS_GENERIC_FAILURE);
        fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_finished);
        return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
    }

    if (ts_p->blocks_remaining == 0)
    {
        /* Free up memory before we start the next pass.
         */
        fbe_rdgen_ts_free_memory(ts_p);

        /* Done with the read/check pass.
         * Increment the pass count and go to the top of the loop to start the next pass.
         */
        fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_write_read_compare_state0);
    }
    else
    {
        if (fbe_rdgen_ts_is_time_to_trace(ts_p))
        {
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO, fbe_rdgen_object_get_object_id(ts_p->object_p),
                                "%s: next w/r/c slice  object: 0x%x current lba: 0x%llx current blocks: 0x%llx "
                                "blocks remaining: 0x%llx\n", 
                                __FUNCTION__, ts_p->object_p->object_id,
                                (unsigned long long)ts_p->current_lba,
			        (unsigned long long)ts_p->current_blocks,
			        (unsigned long long)ts_p->blocks_remaining);
        }

        /* Done with this read/check pass, set state to go to start the next pass.
         */
        fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_write_read_compare_state21);
    }

    return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
}
/******************************************
 * end fbe_rdgen_ts_write_read_compare_state23()
 ******************************************/

/*************************
 * end file fbe_rdgen_write_read_compare.c
 *************************/


