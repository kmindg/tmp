/***************************************************************************
 * Copyright (C) EMC Corporation 2013
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_rdgen_read_buffer.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the rdgen read buffer state machine.
 *  This is an operation that reads memory and buffers it.
 *
 * @version
 *  12/2/2013 - Created. Rob Foley
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
 * fbe_rdgen_ts_read_buffer_state0()
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
 *  12/2/2013 - Created. Rob Foley
 *
 ****************************************************************/

fbe_rdgen_ts_state_status_t fbe_rdgen_ts_read_buffer_state0(fbe_rdgen_ts_t *ts_p)
{
    fbe_bool_t status;
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
    if (fbe_rdgen_request_is_complete(ts_p->request_p)) {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO, ts_p->object_p->object_id, 
                                "%s: request stopped\n", __FUNCTION__);
        ts_p->last_packet_status.rdgen_status = FBE_RDGEN_OPERATION_STATUS_STOPPED;
        fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_finished);
        return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
    }

    /* If the number of I/Os we were programmed with has been satisfied, then 
     * exit. 
     */
    if (fbe_rdgen_ts_is_complete(ts_p))
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO, ts_p->object_p->object_id,
                            "%s: fbe_rdgen_ts_is_complete\n", __FUNCTION__);
        fbe_rdgen_ts_set_complete_status(ts_p);
        fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_finished);
        return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
    }

    fbe_rdgen_ts_clear_flag(ts_p, FBE_RDGEN_TS_FLAGS_FIRST_REQUEST);
    fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_read_buffer_state1);

    fbe_rdgen_ts_initialize_blocks_remaining(ts_p);

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
    return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
}
/******************************************
 * end fbe_rdgen_ts_read_buffer_state0()
 ******************************************/
/*!**************************************************************
 * fbe_rdgen_ts_read_buffer_state1()
 ****************************************************************
 * @brief
 *  Initialize the breakup count and our current blocks.
 *
 * @param ts_p - current_ts.               
 *
 * @return FBE_RDGEN_TS_STATE_STATUS_EXECUTING   
 *
 * @author
 *  12/2/2013 - Created. Rob Foley
 *
 ****************************************************************/
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_read_buffer_state1(fbe_rdgen_ts_t *ts_p)
{
    /* Our current blocks can't be beyond our blocks remaining.
     */
    ts_p->current_blocks = (fbe_u32_t) ts_p->blocks;

    fbe_rdgen_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                            ts_p->object_p->object_id,
                            "%s: new ts lba: 0x%llx blocks: 0x%llx breakup_count: 0x%x\n", __FUNCTION__, 
                            (unsigned long long)ts_p->lba, (unsigned long long)ts_p->blocks,
                            ts_p->breakup_count);

    /* As an optimization we only allocate memory if we need to. 
     * Constant sized requests will not free memory. 
     */
    if ((ts_p->memory_request.ptr == NULL) &&
        (ts_p->memory_p == NULL))
    {
        fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_read_buffer_state2);
    }
    else
    {
        fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_read_buffer_state3);
    }
    return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
}
/******************************************
 * end fbe_rdgen_ts_read_buffer_state1()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_ts_read_buffer_state2()
 ****************************************************************
 * @brief
 *  Allocate memory.
 *
 * @param ts_p - current_ts.               
 *
 * @return FBE_RDGEN_TS_STATE_STATUS_EXECUTING   
 *
 * @author
 *  12/2/2013 - Created. Rob Foley
 *
 ****************************************************************/

fbe_rdgen_ts_state_status_t fbe_rdgen_ts_read_buffer_state2(fbe_rdgen_ts_t *ts_p)
{
    fbe_rdgen_ts_state_status_t status;

    fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_read_buffer_state3);

    /* Allocate memory.
     */
    status = fbe_rdgen_ts_allocate_memory(ts_p);

    return status;
}
/******************************************
 * end fbe_rdgen_ts_read_buffer_state2()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_ts_read_buffer_state3()
 ****************************************************************
 * @brief
 *  Transition to do the read operation.
 *
 * @param ts_p - current_ts.               
 *
 * @return FBE_RDGEN_TS_STATE_STATUS_EXECUTING   
 *
 * @author
 *  12/2/2013 - Created. Rob Foley
 *
 ****************************************************************/

fbe_rdgen_ts_state_status_t fbe_rdgen_ts_read_buffer_state3(fbe_rdgen_ts_t *ts_p)
{
    if (RDGEN_COND(ts_p->memory_request.ptr == NULL) &&
        RDGEN_COND(ts_p->memory_p == NULL))
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: rdgen_ts memory pointer is null. \n", __FUNCTION__);
        fbe_rdgen_ts_set_unexpected_error_status(ts_p);
        fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_finished);
        return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
    }
    if (fbe_rdgen_request_is_complete(ts_p->request_p)) {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO, ts_p->object_p->object_id, 
                                "%s: request stopped\n", __FUNCTION__);
        ts_p->last_packet_status.rdgen_status = FBE_RDGEN_OPERATION_STATUS_STOPPED;
        fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_finished);
        return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
    }
    if (fbe_rdgen_ts_is_complete(ts_p))
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO, ts_p->object_p->object_id, 
                                "%s: fbe_rdgen_ts_is_complete\n", __FUNCTION__);
        fbe_rdgen_ts_set_complete_status(ts_p);
        fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_finished);
        return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
    }
    if (fbe_rdgen_ts_is_aborted(ts_p))
    {
        /* Kill the thread, we are terminating.
         */
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO, 
                                ts_p->object_p->object_id,
                                "read buffer aborted lba: 0x%llx/0x%llx\n", 
                                ts_p->lba, ts_p->blocks);
        fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_finished);
    }
    else
    {
        fbe_rdgen_ts_set_calling_state(ts_p, fbe_rdgen_ts_read_buffer_state4);
        fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_read_state0);
    }

    return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
}
/******************************************
 * end fbe_rdgen_ts_read_buffer_state3()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_ts_read_buffer_state4()
 ****************************************************************
 * @brief
 *  Handle the status from the read operation.
 *
 * @param ts_p - current_ts.               
 *
 * @return FBE_RDGEN_TS_STATE_STATUS_EXECUTING   
 *
 * @author
 *  12/2/2013 - Created. Rob Foley
 *
 ****************************************************************/

fbe_rdgen_ts_state_status_t fbe_rdgen_ts_read_buffer_state4(fbe_rdgen_ts_t *ts_p)
{
    fbe_rdgen_ts_state_status_t error_status;
    fbe_packet_t *packet_p = ts_p->request_p->packet_p;
    fbe_packet_t *master_packet_p = fbe_transport_get_master_packet(packet_p);
    fbe_sg_element_t *dest_p = NULL;
    if (fbe_rdgen_ts_is_aborted(ts_p))
    {
        /* Kill the thread, we are terminating.
         */
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO, 
                                ts_p->object_p->object_id,
                                "read buffer aborted lba: 0x%llx/0x%llx\n", 
                                ts_p->lba, ts_p->blocks);
        ts_p->last_packet_status.rdgen_status = FBE_RDGEN_OPERATION_STATUS_STOPPED;
        fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_finished);
        return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
    }

    /* Interpret the error status from the read operation.
     * If we have more error handling to do, then exit now. 
     * fbe_rdgen_ts_handle_error() will already have set the next state.
     */
    error_status = fbe_rdgen_ts_handle_error(ts_p, fbe_rdgen_ts_read_buffer_state3);
    if (error_status != FBE_RDGEN_TS_STATE_STATUS_DONE)
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO, 
                                ts_p->object_p->object_id,
                                "read buffer error hit lba: 0x%llx/0x%llx\n", 
                                ts_p->lba, ts_p->blocks);
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
                                __FUNCTION__,
				(unsigned long long)ts_p->current_blocks,
				(unsigned long long)ts_p->blocks_remaining );
        return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
    }


    ts_p->blocks_remaining -= ts_p->current_blocks;
    ts_p->current_lba += ts_p->current_blocks;
    ts_p->current_blocks = (fbe_u32_t)FBE_MIN(ts_p->blocks_remaining, ts_p->current_blocks);

    /* If this is a read/pin from the test then there is no sg provided. 
     * If this is from encryption, then there is an sg provided. 
     */
    if (ts_p->request_p->specification.sg_p != NULL) {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO, 
                                ts_p->object_p->object_id,
                                "read buffer read complete for lba: 0x%llx/0x%llx\n", 
                                ts_p->lba, ts_p->blocks);
        /* copy the sg list from ours to the caller's.
         */
        dest_p = fbe_sg_element_copy_list(ts_p->sg_ptr, ts_p->request_p->specification.sg_p,
                                          (fbe_u32_t)(FBE_BE_BYTES_PER_BLOCK * ts_p->blocks));
        if (dest_p == NULL) {
             fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                     "%s: failure during sg copy\n", __FUNCTION__);
            fbe_rdgen_ts_set_unexpected_error_status(ts_p);
            fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_finished);
            return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
        }
    }
    else {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO, 
                                ts_p->object_p->object_id,
                                "read buffer read complete (no sg) for lba: 0x%llx/0x%llx\n", 
                                ts_p->lba, ts_p->blocks);
    }
    /* Set the status we will return.
     */
    ts_p->last_packet_status.rdgen_status = FBE_RDGEN_OPERATION_STATUS_SUCCESS;
    if (ts_p->last_packet_status.block_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS ||
        ts_p->last_packet_status.status != FBE_STATUS_OK) {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                "read buffer complete: packet status was: 0x%x bl_status: 0x%x. (This is  normal)\n",
                                ts_p->last_packet_status.block_status, ts_p->last_packet_status.status);
        ts_p->last_packet_status.block_status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS;
        ts_p->last_packet_status.status = FBE_STATUS_OK;
    }

    /* Save the context (and status) into the start io which will be used by the caller to
     * kick start this request again when they want to unlock this read.
     */
    fbe_rdgen_ts_save_context(ts_p);

    fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_read_buffer_state5);

    /* The master packet is completed, but the subpacket remains here.
     */
    if (master_packet_p == NULL){
         fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                 "%s: no master packet to complete. (This is  normal)\n", __FUNCTION__);
        return FBE_RDGEN_TS_STATE_STATUS_WAITING;
    }
    /* Complete the master so the subpacket can hold onto the stripe lock.
     */
    fbe_transport_remove_subpacket(packet_p);
    fbe_transport_complete_packet(master_packet_p);
    fbe_rdgen_ts_set_flag(ts_p, FBE_RDGEN_TS_FLAGS_WAIT_UNPIN);
    return FBE_RDGEN_TS_STATE_STATUS_DONE;
}
/******************************************
 * end fbe_rdgen_ts_read_buffer_state4()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_ts_read_buffer_state5()
 ****************************************************************
 * @brief
 *  Complete the read buffer operation.
 *
 * @param ts_p - current_ts.               
 *
 * @return FBE_RDGEN_TS_STATE_STATUS_EXECUTING   
 *
 * @author
 *  12/2/2013 - Created. Rob Foley
 *
 ****************************************************************/

fbe_rdgen_ts_state_status_t fbe_rdgen_ts_read_buffer_state5(fbe_rdgen_ts_t *ts_p)
{
    fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO, 
                            ts_p->object_p->object_id,
                            "read buffer complete for lba: 0x%llx/0x%llx\n", 
                            ts_p->lba, ts_p->blocks);
    fbe_rdgen_ts_free_memory(ts_p);
    fbe_rdgen_ts_set_complete_status(ts_p);
    fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_finished);
    return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
}
/******************************************
 * end fbe_rdgen_ts_read_buffer_state5()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_ts_read_buffer_state6()
 ****************************************************************
 * @brief
 *  Kick off the flush for the read and pin.
 *
 * @param ts_p - current_ts.               
 *
 * @return FBE_RDGEN_TS_STATE_STATUS_EXECUTING   
 *
 * @author
 *  1/7/2014 - Created. Rob Foley
 *
 ****************************************************************/

fbe_rdgen_ts_state_status_t fbe_rdgen_ts_read_buffer_state6(fbe_rdgen_ts_t *ts_p) 
{
    fbe_status_t status;
    fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO, 
                            ts_p->object_p->object_id,
                            "Start unlock flush for lba: 0x%llx/0x%llx\n", 
                            ts_p->lba, ts_p->blocks);

    if (RDGEN_COND(ts_p->memory_request.ptr == NULL) &&
        RDGEN_COND(ts_p->memory_p == NULL)) {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: rdgen_ts memory pointer is null. \n", __FUNCTION__);
        fbe_rdgen_ts_set_unexpected_error_status(ts_p);
        fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_finished);
        return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
    }

    if (fbe_rdgen_ts_is_complete(ts_p)) {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                                FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                "%s: fbe_rdgen_ts_is_complete for object_id: 0x%x\n", 
                                __FUNCTION__, ts_p->object_p->object_id);
        fbe_rdgen_ts_set_complete_status(ts_p);
        fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_finished);
        return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
    }
    if (fbe_rdgen_ts_is_aborted(ts_p)) {
        /* Kill the thread, we are terminating.
         */
        fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_finished);
    } else {
        fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_read_buffer_state7);

        status = fbe_rdgen_ts_send_packet(ts_p, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_VERIFY,
                                          ts_p->lba,
                                          ts_p->blocks,
                                          ts_p->sg_ptr,
                                          0 /* payload flags */);
        if (status != FBE_STATUS_OK) {
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, ts_p->object_p->object_id,
                                    "%s: send packet failed status: 0x%x\n", __FUNCTION__, status);
            fbe_rdgen_ts_set_unexpected_error_status(ts_p);
            fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_finished);
            return FBE_RDGEN_TS_STATE_STATUS_EXECUTING; 
        }
        return FBE_RDGEN_TS_STATE_STATUS_WAITING;
    }
    return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
}
/******************************************
 * end fbe_rdgen_ts_read_buffer_state6()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_ts_read_buffer_state7()
 ****************************************************************
 * @brief
 *  Handle the status from the flush operation.
 *
 * @param ts_p - current_ts.               
 *
 * @return FBE_RDGEN_TS_STATE_STATUS_EXECUTING   
 *
 * @author
 *  1/7/2014 - Created. Rob Foley
 *
 ****************************************************************/

fbe_rdgen_ts_state_status_t fbe_rdgen_ts_read_buffer_state7(fbe_rdgen_ts_t *ts_p)
{
    fbe_rdgen_ts_state_status_t error_status;
    if (fbe_rdgen_ts_is_aborted(ts_p)) {
        /* Kill the thread, we are terminating.
         */
        fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_finished);
        return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
    }

    /* Interpret the error status from the read operation.
     * If we have more error handling to do, then exit now. 
     * fbe_rdgen_ts_handle_error() will already have set the next state.
     */
    error_status = fbe_rdgen_ts_handle_error(ts_p, fbe_rdgen_ts_read_buffer_state7);
    if (error_status != FBE_RDGEN_TS_STATE_STATUS_DONE) {
        return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
    }

    /* A single I/O finished, increment the io counter.
     */
    fbe_rdgen_ts_inc_io_count(ts_p);

    fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO, 
                            ts_p->object_p->object_id,
                            "read buffer flush complete for lba: 0x%llx/0x%llx\n", 
                            ts_p->lba, ts_p->blocks);
    fbe_rdgen_ts_free_memory(ts_p);
    fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_finished);

    return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
}
/******************************************
 * end fbe_rdgen_ts_read_buffer_state7()
 ******************************************/
/*************************
 * end file fbe_rdgen_read_buffer.c
 *************************/


