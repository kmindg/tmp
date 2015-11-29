/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_rdgen_write.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the rdgen write state machine.
 *
 * @version
 *   5/28/2009:  Created. Rob Foley
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
 * fbe_rdgen_ts_write_state0()
 ****************************************************************
 * @brief
 *  Generate the sg list.
 *
 * @param ts_p - current_ts.               
 *
 * @return FBE_RDGEN_TS_STATE_STATUS_EXECUTING   
 *
 * @author
 *  6/8/2009 - Created. Rob Foley
 *
 ****************************************************************/
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_write_state0(fbe_rdgen_ts_t *ts_p)
{
    fbe_status_t status;
    fbe_block_count_t total_bytes = ts_p->current_blocks * ts_p->block_size;
    if (fbe_rdgen_ts_is_aborted(ts_p))
    {
        /* Kill the thread, we are terminating.
         */
         fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_finished);
    }
    else
    {
        fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_write_state1);
    }

    /* Only allocate what we need to for write sames. 
     * Write same always writes just one block. 
     */
    if ((ts_p->operation == FBE_RDGEN_OPERATION_WRITE_SAME) || (ts_p->operation == FBE_RDGEN_OPERATION_UNMAP) ||
        (ts_p->operation == FBE_RDGEN_OPERATION_WRITE_SAME_READ_CHECK))
    {
        total_bytes = ts_p->object_p->block_size;
    }

    /* Generate the sg list for the write.
     */
    status = fbe_rdgen_ts_setup_sgs(ts_p, total_bytes);
    if (status != FBE_STATUS_OK)
    {
        fbe_rdgen_ts_set_unexpected_error_status(ts_p);
        fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_finished);
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: status %d from fbe_rdgen_ts_setup_sgs().\n", 
                                __FUNCTION__, status);
        return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
    }

    return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
}
/******************************************
 * end fbe_rdgen_ts_write_state0()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_ts_write_state1()
 ****************************************************************
 * @brief
 *  Set the pattern into the buffer.
 *
 * @param ts_p - current_ts.               
 *
 * @return FBE_RDGEN_TS_STATE_STATUS_EXECUTING   
 *
 * @author
 *  6/8/2009 - Created. Rob Foley
 *
 ****************************************************************/
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_write_state1(fbe_rdgen_ts_t *ts_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_bool_t b_set_pattern = FBE_TRUE;

    if (RDGEN_COND(ts_p->sg_ptr == NULL))
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: rdgen_ts sg pointer is null. \n", __FUNCTION__);
        fbe_rdgen_ts_set_unexpected_error_status(ts_p);
        fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_finished);
        return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
    }
    fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_write_state2);

    /* In performance mode, we only touch the buffers on the first pass.
     */
    if ( fbe_rdgen_specification_options_is_set(&ts_p->request_p->specification, FBE_RDGEN_OPTIONS_PERFORMANCE) &&
         (ts_p->request_p->specification.block_spec == FBE_RDGEN_BLOCK_SPEC_CONSTANT) &&
         (ts_p->io_count > 0))
    {
        b_set_pattern = FBE_FALSE;
    }
    if (b_set_pattern)
    {
        /* If we are using regions make sure to set the sp id into our current region 
         * so the caller has this info.  This is needed since the caller might not have 
         * specified which SP we will run I/O to. 
         */
        if (fbe_rdgen_specification_options_is_set(&ts_p->request_p->specification,
                                                   FBE_RDGEN_OPTIONS_CREATE_REGIONS) ||
            fbe_rdgen_specification_options_is_set(&ts_p->request_p->specification,
                                                   FBE_RDGEN_OPTIONS_USE_REGIONS))
        {

            /* Save the fact that we wrote it on this sp.  The caller needs this since 
             * we can randomize where it gets written. 
             */
            ts_p->request_p->specification.region_list[ts_p->region_index].sp_id = fbe_rdgen_cmi_get_sp_id();
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, ts_p->object_p->object_id,
                                    "%s: writing locally: SP %x lba: 0x%llx bl: 0x%llx\n", 
                                    __FUNCTION__,
				    ts_p->request_p->specification.region_list[ts_p->region_index].sp_id, 
                                    (unsigned long long)ts_p->lba,
				    (unsigned long long)ts_p->blocks);
        }
        status = fbe_rdgen_fill_sg_list(ts_p, 
                                        ts_p->sg_ptr,
                                        ts_p->request_p->specification.pattern,
                                        FBE_TRUE, /* Yes, append checksum */ 
                                        FBE_FALSE /*read_op*/);
    }
    if (status != FBE_STATUS_OK)
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, ts_p->object_p->object_id,
                                "%s: fill sg failed status: 0x%x\n", __FUNCTION__, status);
        fbe_rdgen_ts_set_status(ts_p, FBE_STATUS_GENERIC_FAILURE);
        fbe_rdgen_ts_set_unexpected_error_status(ts_p);
        fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_finished);
        return FBE_RDGEN_TS_STATE_STATUS_EXECUTING; 
    }

    return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
}
/******************************************
 * end fbe_rdgen_ts_write_state1()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_ts_write_state2()
 ****************************************************************
 * @brief
 *  Send out the pre-read packet (if needed).
 *
 * @param ts_p - current_ts.               
 *
 * @return FBE_RDGEN_TS_STATE_STATUS_EXECUTING   
 *
 * @author
 *  5/3/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_rdgen_ts_state_status_t fbe_rdgen_ts_write_state2(fbe_rdgen_ts_t *ts_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_payload_block_operation_flags_t payload_flags = 0;

    /* For the most part we always want to check the validity of data being
     * written.  But we allow SP cache to send data without being checked.
     * Therefore the only case we don't set the check checksum flag is if
     * the caller has told us not to.
     */
    if (!(ts_p->request_p->specification.options & FBE_RDGEN_OPTIONS_DO_NOT_CHECK_CRC))
    {
        /* Writes will check checksums unless told not to do so.
         * Corrupt CRC's do not check checksums.
         */
        payload_flags = FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_CHECK_CHECKSUM;
    }

    if (ts_p->pre_read_blocks > 0)
    {
        fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_write_state3);
        status = fbe_rdgen_ts_send_packet(ts_p, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                          ts_p->pre_read_lba,
                                          ts_p->pre_read_blocks,
                                          ts_p->pre_read_sg_ptr,
                                          payload_flags);
        if (status != FBE_STATUS_OK)
        {
            fbe_rdgen_ts_set_status(ts_p, FBE_STATUS_GENERIC_FAILURE);
            fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_finished);
            return FBE_RDGEN_TS_STATE_STATUS_EXECUTING; 
        }
        else
        {
            /* Send out the io packet.  Our callback always gets executed, thus
            * there is no status to handle here.
            */
            return FBE_RDGEN_TS_STATE_STATUS_WAITING;
        }
    }
    else
    {
        fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_write_state4);
        /* No pre-read.
         */
        return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
    }
}
/******************************************
 * end fbe_rdgen_ts_write_state2()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_ts_write_state3()
 ****************************************************************
 * @brief
 *  Check the status of the pre-read.
 *
 * @param ts_p - current_ts.               
 *
 * @return FBE_RDGEN_TS_STATE_STATUS_EXECUTING   
 *
 * @author
 *  5/3/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_rdgen_ts_state_status_t fbe_rdgen_ts_write_state3(fbe_rdgen_ts_t *ts_p)
{
    fbe_rdgen_ts_state_status_t error_status;
    fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_write_state4);

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
    error_status = fbe_rdgen_ts_handle_error(ts_p, fbe_rdgen_ts_write_state2);
    if (error_status != FBE_RDGEN_TS_STATE_STATUS_DONE)
    {
        return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
    }

    /* Increment the io counter since the read is done.
     */
    fbe_rdgen_ts_inc_io_count(ts_p);
    return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
}
/******************************************
 * end fbe_rdgen_ts_write_state3()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_ts_write_state4()
 ****************************************************************
 * @brief
 *  Send out the packet.
 *
 * @param ts_p - current_ts.               
 *
 * @return FBE_RDGEN_TS_STATE_STATUS_EXECUTING   
 *
 * @author
 *  6/8/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_rdgen_ts_state_status_t fbe_rdgen_ts_write_state4(fbe_rdgen_ts_t *ts_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_payload_block_operation_flags_t payload_flags = 0;

    fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_write_state5);

    /* For the most part we always want to check the validity of data being
     * written.  But we allow SP cache to send data without being checked.
     * Therefore the only case we don't set the check checksum flag is if
     * the caller has told us not to.
     */
    if (!(ts_p->request_p->specification.options & FBE_RDGEN_OPTIONS_DO_NOT_CHECK_CRC))
    {
        /* Writes will check checksums unless told not to do so.
         * Corrupt CRC's do not check checksums.
         */
        payload_flags = FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_CHECK_CHECKSUM;
    }

    /* Since we want to generate an event log for the sector corrupted, set the
     * `Corrupt CRC' flag.
     */
    if (ts_p->operation == FBE_RDGEN_OPERATION_CORRUPT_CRC_READ_CHECK)
    {
        /* Corrupt crc operation, indicate to raid that invalid blocks are present.
         */
        payload_flags |= FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_CORRUPT_CRC;
    }

    if ((ts_p->operation == FBE_RDGEN_OPERATION_VERIFY_WRITE) ||
        (ts_p->operation == FBE_RDGEN_OPERATION_VERIFY_WRITE_READ_CHECK))
    {
        status = fbe_rdgen_ts_send_packet(ts_p, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_WRITE,
                                          ts_p->current_lba,
                                          ts_p->current_blocks,
                                          ts_p->sg_ptr,
                                          payload_flags);
        if (status != FBE_STATUS_OK)
        {
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, ts_p->object_p->object_id,
                                    "%s: send packet failed status: 0x%x\n", __FUNCTION__, status);
            fbe_rdgen_ts_set_unexpected_error_status(ts_p);
            fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_finished);
            return FBE_RDGEN_TS_STATE_STATUS_EXECUTING; 
        }
    }
    else if (ts_p->request_p->specification.options & FBE_RDGEN_OPTIONS_NON_CACHED)
    {
        status = fbe_rdgen_ts_send_packet(ts_p, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_NONCACHED,
                                          ts_p->current_lba,
                                          ts_p->current_blocks,
                                          ts_p->sg_ptr,
                                          payload_flags);
        if (status != FBE_STATUS_OK)
        {
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, ts_p->object_p->object_id,
                                    "%s: send packet failed status: 0x%x\n", __FUNCTION__, status);
            fbe_rdgen_ts_set_unexpected_error_status(ts_p);
            fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_finished);
            return FBE_RDGEN_TS_STATE_STATUS_EXECUTING; 
        }
    }
    else if ((ts_p->operation == FBE_RDGEN_OPERATION_WRITE_SAME) || (ts_p->operation == FBE_RDGEN_OPERATION_UNMAP) ||
             (ts_p->operation == FBE_RDGEN_OPERATION_WRITE_SAME_READ_CHECK))
    {
        if (ts_p->operation == FBE_RDGEN_OPERATION_UNMAP)
        {
            payload_flags |= FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_UNMAP;
            fbe_zero_memory(ts_p->sg_ptr->address, ts_p->sg_ptr->count);
        }
        status = fbe_rdgen_ts_send_packet(ts_p, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_SAME,
                                 ts_p->current_lba,
                                 ts_p->current_blocks,
                                 ts_p->sg_ptr,
                                 payload_flags);
        if (status != FBE_STATUS_OK)
        {
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, ts_p->object_p->object_id,
                                    "%s: send packet failed status: 0x%x\n", __FUNCTION__, status);
            fbe_rdgen_ts_set_unexpected_error_status(ts_p);
            fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_finished);
            return FBE_RDGEN_TS_STATE_STATUS_EXECUTING; 
        }
    }
    else if ((ts_p->operation == FBE_RDGEN_OPERATION_CORRUPT_DATA_ONLY) ||
             (ts_p->operation == FBE_RDGEN_OPERATION_CORRUPT_DATA_READ_CHECK))
    {
        status = fbe_rdgen_ts_send_packet(ts_p, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_CORRUPT_DATA,
                                 ts_p->current_lba,
                                 ts_p->current_blocks,
                                 ts_p->sg_ptr,
                                 payload_flags);
        if (status != FBE_STATUS_OK)
        {
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, ts_p->object_p->object_id,
                                    "%s: send packet failed status: 0x%x\n", __FUNCTION__, status);
            fbe_rdgen_ts_set_unexpected_error_status(ts_p);
            fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_finished);
            return FBE_RDGEN_TS_STATE_STATUS_EXECUTING; 
        }
    }
    else if (ts_p->operation == FBE_RDGEN_OPERATION_WRITE_VERIFY_READ_CHECK)
    {
        status = fbe_rdgen_ts_send_packet(ts_p, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_VERIFY,
                                 ts_p->current_lba,
                                 ts_p->current_blocks,
                                 ts_p->sg_ptr,
                                 payload_flags);
        if (status != FBE_STATUS_OK)
        {
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, ts_p->object_p->object_id,
                                    "%s: send packet failed status: 0x%x\n", __FUNCTION__, status);
            fbe_rdgen_ts_set_unexpected_error_status(ts_p);
            fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_finished);
            return FBE_RDGEN_TS_STATE_STATUS_EXECUTING; 
        }
    }
    else
    {
        fbe_bool_t b_check_pattern = FBE_TRUE;
        /* In performance mode, we only touch the buffers on the first pass.
         */
        if (fbe_rdgen_specification_options_is_set(&ts_p->request_p->specification, FBE_RDGEN_OPTIONS_PERFORMANCE))
        {
            b_check_pattern = FBE_FALSE;
        }
        if (b_check_pattern)
        {
            /* Check for the pattern in the buffer.
             */
            status = fbe_rdgen_check_sg_list(ts_p, 
                                             ts_p->request_p->specification.pattern,
                                             FBE_TRUE    /* Yes, panic on miscompare. */ );
            if (status != FBE_STATUS_OK) 
            { 
                fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                        "%s: status %d from check_sg_list().\n", 
                                        __FUNCTION__, status);
                fbe_rdgen_ts_set_unexpected_error_status(ts_p);
                fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_finished);
                return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
            }
        }
        status = fbe_rdgen_ts_send_packet(ts_p, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,
                                          ts_p->current_lba,
                                          ts_p->current_blocks,
                                          ts_p->sg_ptr,
                                          payload_flags);
        if (status != FBE_STATUS_OK)
        {
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, ts_p->object_p->object_id,
                                    "%s: send packet failed status: 0x%x\n", __FUNCTION__, status);
            fbe_rdgen_ts_set_unexpected_error_status(ts_p);
            fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_finished);
            return FBE_RDGEN_TS_STATE_STATUS_EXECUTING; 
        }
    }

    /* Send out the io packet.  Our callback always gets executed, thus
     * there is no status to handle here.
     */
    return FBE_RDGEN_TS_STATE_STATUS_WAITING;
}
/******************************************
 * end fbe_rdgen_ts_write_state4()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_ts_write_state5()
 ****************************************************************
 * @brief
 *  Process the completion.
 *
 * @param ts_p - current_ts.               
 *
 * @return FBE_RDGEN_TS_STATE_STATUS_EXECUTING   
 *
 * @author
 *  6/8/2009 - Created. Rob Foley
 *
 ****************************************************************/
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_write_state5(fbe_rdgen_ts_t *ts_p)
{
    fbe_rdgen_ts_set_state(ts_p, ts_p->calling_state);

    return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
}
/******************************************
 * end fbe_rdgen_ts_write_state5()
 ******************************************/

/*************************
 * end file fbe_rdgen_write.c
 *************************/


