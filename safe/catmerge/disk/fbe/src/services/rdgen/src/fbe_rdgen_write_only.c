/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_rdgen_write_only.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the rdgen write only state machine.
 *  This is an operation that simply does writes and nothing else.
 *
 * @version
 *   7/21/2009:  Created. Rob Foley
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
 * fbe_rdgen_ts_write_only_state0()
 ****************************************************************
 * @brief
 *  This is the first state of the write only state machine.
 *  We initialize the range of this ts.
 *
 * @param ts_p - current_ts.               
 *
 * @return FBE_RDGEN_TS_STATE_STATUS_EXECUTING   
 *
 * @author
 *  7/21/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_rdgen_ts_state_status_t fbe_rdgen_ts_write_only_state0(fbe_rdgen_ts_t *ts_p)
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
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s: fbe_rdgen_ts_is_complete for object_id: 0x%x current_lba: 0x%llx\n", 
                            __FUNCTION__, ts_p->object_p->object_id,
			    (unsigned long long)ts_p->current_lba);
        fbe_rdgen_ts_set_complete_status(ts_p);
        fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_finished);
        return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
    }

    fbe_rdgen_ts_clear_flag(ts_p, FBE_RDGEN_TS_FLAGS_FIRST_REQUEST);
    fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_write_only_state1);

    /* Since we are modifying the media, we need to check for conflicts.
     * We do not check this in performance mode since it gets locks. 
     */
    if (!fbe_rdgen_specification_options_is_set(&ts_p->request_p->specification, FBE_RDGEN_OPTIONS_PERFORMANCE))
    {
        /* Since we are comparing what we are reading, we need to check for conflicts.
         */ 
        status = fbe_rdgen_ts_resolve_conflict(ts_p);
        if (status == FBE_STATUS_GENERIC_FAILURE)
        {
            /*! Unexpected error in resolve conflict, complete this ts with error.
             */
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, ts_p->object_p->object_id,
                                    "%s: resolve conflict failed\n", __FUNCTION__);
            fbe_rdgen_ts_set_unexpected_error_status(ts_p);
            fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_finished);
            return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
        }
        else if (status == FBE_STATUS_PENDING)
        {
            return FBE_RDGEN_TS_STATE_STATUS_WAITING;
        }
    }
    return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
}
/******************************************
 * end fbe_rdgen_ts_write_only_state0()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_ts_write_only_state1()
 ****************************************************************
 * @brief
 *  This is the state where we have obtained our lock and
 *  we will set our current blocks.
 *
 * @param ts_p - current_ts.               
 *
 * @return FBE_RDGEN_TS_STATE_STATUS_EXECUTING   
 *
 * @author
 *  10/12/2011 - Created. Rob Foley
 *
 ****************************************************************/

fbe_rdgen_ts_state_status_t fbe_rdgen_ts_write_only_state1(fbe_rdgen_ts_t *ts_p)
{
    fbe_rdgen_ts_initialize_blocks_remaining(ts_p);
    if (fbe_rdgen_ts_should_send_to_peer(ts_p))
    {
        return fbe_rdgen_ts_send_to_peer(ts_p);
    }
    else
    {
        fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_write_only_state2);
    }
    return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
}
/******************************************
 * end fbe_rdgen_ts_write_only_state1()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_ts_write_only_state2()
 ****************************************************************
 * @brief
 *  Initialize the breakup count and our current blocks.
 *
 * @param ts_p - current_ts.               
 *
 * @return FBE_RDGEN_TS_STATE_STATUS_EXECUTING   
 *
 * @author
 *  7/21/2009 - Created. Rob Foley
 *
 ****************************************************************/
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_write_only_state2(fbe_rdgen_ts_t *ts_p)
{
    /* Determine how many times to breakup this request.
     */
    fbe_rdgen_ts_get_breakup_count(ts_p);

    fbe_rdgen_ts_get_current_blocks(ts_p);

    fbe_rdgen_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "%s: new iorb lba: 0x%llx blocks: 0x%llx breakup_count: 0x%x object_id: 0x%x\n", __FUNCTION__, 
                (unsigned long long)ts_p->lba, (unsigned long long)ts_p->blocks,
		ts_p->breakup_count, ts_p->object_p->object_id);

    /* As an optimization we only allocate memory if we need to. 
     * Constant sized requests will not free memory. 
     */
    if ((ts_p->memory_request.ptr == NULL) &&
        (ts_p->memory_p == NULL))
    {
        fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_write_only_state3);
    }
    else
    {
        fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_write_only_state4);
    }
    return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
}
/******************************************
 * end fbe_rdgen_ts_write_only_state2()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_ts_write_only_state3()
 ****************************************************************
 * @brief
 *  Allocate memory.
 *
 * @param ts_p - current_ts.               
 *
 * @return FBE_RDGEN_TS_STATE_STATUS_EXECUTING   
 *
 * @author
 *  7/21/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_rdgen_ts_state_status_t fbe_rdgen_ts_write_only_state3(fbe_rdgen_ts_t *ts_p)
{
    fbe_rdgen_ts_state_status_t status;

    fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_write_only_state4);

    /* We don't need to allocate memory for zero operation.
     */
    if (ts_p->operation == FBE_RDGEN_OPERATION_ZERO_ONLY)
    {
        return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
    }

    /* Allocate memory.
     */
    status = fbe_rdgen_ts_allocate_memory(ts_p);

    return status;
}
/******************************************
 * end fbe_rdgen_ts_write_only_state3()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_ts_write_only_state4()
 ****************************************************************
 * @brief
 *  Transition to do the write operation.
 *
 * @param ts_p - current_ts.               
 *
 * @return FBE_RDGEN_TS_STATE_STATUS_EXECUTING   
 *
 * @author
 *  7/21/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_rdgen_ts_state_status_t fbe_rdgen_ts_write_only_state4(fbe_rdgen_ts_t *ts_p)
{
    if (RDGEN_COND(ts_p->memory_request.ptr == NULL) &&
        RDGEN_COND(ts_p->memory_p == NULL) &&
        (ts_p->operation != FBE_RDGEN_OPERATION_ZERO_ONLY))
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, ts_p->object_p->object_id,
                                "%s: rdgen_ts memory pointer is null. \n", __FUNCTION__);
        fbe_rdgen_ts_set_unexpected_error_status(ts_p);
        fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_finished);
        return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
    }
    
    if (fbe_rdgen_ts_is_complete(ts_p))
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                                FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                "%s: fbe_rdgen_ts_is_complete for object_id: 0x%x current_lba: 0x%llx\n", 
                                __FUNCTION__, ts_p->object_p->object_id,
                                (unsigned long long)ts_p->current_lba);
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
    else if (ts_p->operation == FBE_RDGEN_OPERATION_ZERO_ONLY)
    {
        fbe_rdgen_ts_set_calling_state(ts_p, fbe_rdgen_ts_write_only_state5);
        fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_zero_state0);
    }
    else
    {
        fbe_rdgen_ts_set_calling_state(ts_p, fbe_rdgen_ts_write_only_state5);
        fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_write_state0);
    }

    return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
}
/******************************************
 * end fbe_rdgen_ts_write_only_state4()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_ts_write_only_state5()
 ****************************************************************
 * @brief
 *  Handle the status from the write operation.
 *
 * @param ts_p - current_ts.               
 *
 * @return FBE_RDGEN_TS_STATE_STATUS_EXECUTING   
 *
 * @author
 *  7/21/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_rdgen_ts_state_status_t fbe_rdgen_ts_write_only_state5(fbe_rdgen_ts_t *ts_p)
{
    fbe_rdgen_ts_state_status_t error_status;
    
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
    error_status = fbe_rdgen_ts_handle_error(ts_p, fbe_rdgen_ts_write_only_state4);
    if (error_status != FBE_RDGEN_TS_STATE_STATUS_DONE)
    {
        return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
    }

    /* A single I/O finished, increment the io counter.
     */
    fbe_rdgen_ts_inc_io_count(ts_p);
    
    if (RDGEN_COND(ts_p->current_blocks > ts_p->blocks_remaining))
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "rdgen_ts_wt_only_state4: current blocks 0x%llx > blocks remaining 0x%llx\n",
				(unsigned long long)ts_p->current_blocks,
				(unsigned long long)ts_p->blocks_remaining );
        return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
    }

    ts_p->blocks_remaining -= ts_p->current_blocks;
    ts_p->current_lba += ts_p->current_blocks;
    ts_p->current_blocks = (fbe_u32_t)FBE_MIN(ts_p->blocks_remaining, ts_p->current_blocks);
    
    if (ts_p->blocks_remaining == 0)
    {
        if (fbe_rdgen_ts_is_time_to_trace(ts_p))
        {
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO, 
                                FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                "rdgen_ts_wt_only_state4:obj:0x%x pass:0x%x io_cnt:0x%x\n", 
                                ts_p->object_p->object_id, ts_p->pass_count, ts_p->io_count);
        }
        if ( !fbe_rdgen_specification_options_is_set(&ts_p->request_p->specification, FBE_RDGEN_OPTIONS_PERFORMANCE) ||
             (ts_p->request_p->specification.block_spec != FBE_RDGEN_BLOCK_SPEC_CONSTANT) )
        {
            /* Free up memory before we start the next pass.
             */
            fbe_rdgen_ts_free_memory(ts_p);
        }
        /* Done with this pass, set state to go to start the next pass.
         */
        return fbe_rdgen_ts_set_first_state(ts_p, fbe_rdgen_ts_write_only_state0);
    }
    else
    {
        if (fbe_rdgen_ts_is_time_to_trace(ts_p))
        {
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO, 
                                FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                "rdgen_ts_wt_only_state4: cur lba: 0x%llx cur blocks: 0x%llx "
                                "blocks remain:0x%llx objid:0x%x\n", 
                                (unsigned long long)ts_p->current_lba,
				(unsigned long long)ts_p->current_blocks,
				(unsigned long long)ts_p->blocks_remaining,
                                ts_p->object_p->object_id);
        }
        /* Continue writing, as we have more blocks to write.
         */
        fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_write_only_state4);
    }

    return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
}
/******************************************
 * end fbe_rdgen_ts_write_only_state5()
 ******************************************/

/*************************
 * end file fbe_rdgen_write_only.c
 *************************/


