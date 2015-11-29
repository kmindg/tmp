/***************************************************************************
 * Copyright (C) EMC Corporation 2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_rdgen_dca_rw.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the rdgen dca state machine.
 *  This allows for issuing performance mode dca requests.
 *
 * @version
 *   3/19/2012 - Created. Rob Foley
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
 * fbe_rdgen_ts_dca_rw_state0()
 ****************************************************************
 * @brief
 *  This is the first state of the read only state machine.
 *  We initialize the range of this ts.
 *
 * @param ts_p - current_ts.               
 *
 * @return FBE_RDGEN_TS_STATE_STATUS_EXECUTING   
 *
 * @author
 *  3/19/2012 - Created. Rob Foley
 *
 ****************************************************************/

fbe_rdgen_ts_state_status_t fbe_rdgen_ts_dca_rw_state0(fbe_rdgen_ts_t *ts_p)
{
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
                            "%s: fbe_rdgen_ts_is_complete for object_id: 0x%x\n", 
                            __FUNCTION__, ts_p->object_p->object_id);
        fbe_rdgen_ts_set_complete_status(ts_p);
        fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_finished);
        return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
    }

    fbe_rdgen_ts_clear_flag(ts_p, FBE_RDGEN_TS_FLAGS_FIRST_REQUEST);

    ts_p->current_lba = ts_p->lba;
    ts_p->current_blocks = ts_p->blocks;
    ts_p->blocks_remaining = ts_p->blocks;

    fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_dca_rw_state1);
    return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
}
/******************************************
 * end fbe_rdgen_ts_dca_rw_state0()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_ts_dca_rw_state1()
 ****************************************************************
 * @brief
 *  Initialize the breakup count and our current blocks.
 *
 * @param ts_p - current_ts.               
 *
 * @return FBE_RDGEN_TS_STATE_STATUS_EXECUTING   
 *
 * @author
 *  3/19/2012 - Created. Rob Foley
 *
 ****************************************************************/
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_dca_rw_state1(fbe_rdgen_ts_t *ts_p)
{
    /* Determine how many times to breakup this request.
     */
    fbe_rdgen_ts_get_breakup_count(ts_p);

    if (ts_p->breakup_count > 1)
    {
        /* If we're breaking up this request, 
         * then set blocks for the memory allocation below.
         * Account for the fact that this I/O may be broken up even further.
         */
        fbe_lba_t new_blocks = ts_p->blocks / ts_p->breakup_count;
        /* Add on one if we are not aligned.
         */
        new_blocks += (ts_p->blocks % ts_p->breakup_count) ? 1 : 0;
        ts_p->current_blocks = (fbe_u32_t) FBE_MIN( new_blocks, FBE_RDGEN_MAX_BLOCKS );
    }

    /* Our current blocks can't be beyond our blocks remaining.
     */
    ts_p->current_blocks = (fbe_u32_t) (ts_p->blocks / ts_p->breakup_count);
    ts_p->current_blocks += (ts_p->blocks % ts_p->breakup_count) ? 1 : 0;

    ts_p->current_blocks = (fbe_u32_t)FBE_MIN(ts_p->blocks_remaining, ts_p->current_blocks);

    fbe_rdgen_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "%s: new iorb lba: 0x%llx blocks: 0x%llx breakup_count: 0x%x object_id: 0x%x\n", __FUNCTION__, 
                (unsigned long long)ts_p->lba, (unsigned long long)ts_p->blocks, ts_p->breakup_count, ts_p->object_p->object_id);

    fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_dca_rw_state2);
    return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
}
/******************************************
 * end fbe_rdgen_ts_dca_rw_state1()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_ts_dca_rw_state2()
 ****************************************************************
 * @brief
 *  Send request.
 *
 * @param ts_p - current_ts.               
 *
 * @return FBE_RDGEN_TS_STATE_STATUS_EXECUTING   
 *
 * @author
 *  3/19/2012 - Created. Rob Foley
 *
 ****************************************************************/

fbe_rdgen_ts_state_status_t fbe_rdgen_ts_dca_rw_state2(fbe_rdgen_ts_t *ts_p)
{
    fbe_status_t status;
    fbe_payload_block_operation_flags_t payload_flags = 0;
    fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_dca_rw_state4);

    if (fbe_rdgen_ts_is_complete(ts_p))
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s: fbe_rdgen_ts_is_complete for object_id: 0x%x\n", 
                            __FUNCTION__, ts_p->object_p->object_id);
        fbe_rdgen_ts_set_complete_status(ts_p);
        fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_finished);
        return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
    }
    if (ts_p->operation == FBE_RDGEN_OPERATION_WRITE_ONLY)
    {
        status = fbe_rdgen_ts_send_disk_device(ts_p, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,
                                               ts_p->current_lba,
                                               ts_p->current_blocks,
                                               ts_p->sg_ptr,
                                               payload_flags);
        if (status == FBE_STATUS_GENERIC_FAILURE)
        {
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, ts_p->object_p->object_id,
                                    "%s: unexpected status 0x%x\n", 
                                    __FUNCTION__, 
                                    ts_p->operation);
            fbe_rdgen_ts_set_unexpected_error_status(ts_p);
            fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_finished);
            return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
        }
        return FBE_RDGEN_TS_STATE_STATUS_WAITING;
    }
    else if (ts_p->operation == FBE_RDGEN_OPERATION_READ_ONLY)
    {
        status = fbe_rdgen_ts_send_disk_device(ts_p, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                               ts_p->current_lba,
                                               ts_p->current_blocks,
                                               ts_p->sg_ptr,
                                               payload_flags);
        if (status == FBE_STATUS_GENERIC_FAILURE)
        {
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, ts_p->object_p->object_id,
                                    "%s: unexpected status 0x%x\n", 
                                    __FUNCTION__, 
                                    ts_p->operation);
            fbe_rdgen_ts_set_unexpected_error_status(ts_p);
            fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_finished);
            return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
        }
        return FBE_RDGEN_TS_STATE_STATUS_WAITING;
    }
    else
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, ts_p->object_p->object_id,
                                "%s: unexpected opcode 0x%x\n", 
                                __FUNCTION__, 
                                ts_p->operation);
        fbe_rdgen_ts_set_unexpected_error_status(ts_p);
        fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_finished);
        return FBE_RDGEN_TS_STATE_STATUS_EXECUTING; 
    }

    return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
}
/******************************************
 * end fbe_rdgen_ts_dca_rw_state2()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_ts_dca_rw_state4()
 ****************************************************************
 * @brief
 *  Handle the status from the read operation.
 *
 * @param ts_p - current_ts.               
 *
 * @return FBE_RDGEN_TS_STATE_STATUS_EXECUTING   
 *
 * @author
 *  3/19/2012 - Created. Rob Foley
 *
 ****************************************************************/

fbe_rdgen_ts_state_status_t fbe_rdgen_ts_dca_rw_state4(fbe_rdgen_ts_t *ts_p)
{
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
    error_status = fbe_rdgen_ts_handle_error(ts_p, fbe_rdgen_ts_dca_rw_state2);
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
                                "%s: current blocks 0x%llx > blocks remaining 0x%llx. \n",
                                __FUNCTION__, (unsigned long long)ts_p->current_blocks, (unsigned long long)ts_p->blocks_remaining );
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
                                "%s: object: 0x%x pass: 0x%x io_count:0x%x\n", 
                                __FUNCTION__, ts_p->object_p->object_id, ts_p->pass_count, ts_p->io_count);
        }

        /* Done with this pass, set state to go to start the next pass.
         */
        return fbe_rdgen_ts_set_first_state(ts_p, fbe_rdgen_ts_dca_rw_state0);
    }
    else
    {
        if (fbe_rdgen_ts_is_time_to_trace(ts_p))
        {
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO, 
                                FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                "%s: next read slice  current lba: 0x%llx current blocks: 0x%llx "
                                "blocks remaining: 0x%llx object_id: 0x%x\n", __FUNCTION__, 
                                (unsigned long long)ts_p->current_lba, (unsigned long long)ts_p->current_blocks, (unsigned long long)ts_p->blocks_remaining,
                                ts_p->object_p->object_id);
        }
        /* Continue reading, as we have more blocks to read.
         */
        fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_dca_rw_state2);
    }

    return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
}
/******************************************
 * end fbe_rdgen_ts_dca_rw_state4()
 ******************************************/

/*************************
 * end file fbe_rdgen_dca_rw.c
 *************************/


