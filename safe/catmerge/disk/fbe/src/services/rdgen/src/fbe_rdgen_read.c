/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_rdgen_read.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the rdgen read state machine.
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
 * fbe_rdgen_ts_read_state0()
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
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_read_state0(fbe_rdgen_ts_t *ts_p)
{
    fbe_status_t status;
    if (fbe_rdgen_ts_is_aborted(ts_p))
    {
        /* Kill the thread, we are terminating.
         */
         fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_finished);
    }
    else
    {
        fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_read_state1);
    }

    /* Generate a random looking sg list.
     * The counts are just random.
     * These sgs are filled with -1.
     */
    status = fbe_rdgen_ts_setup_sgs(ts_p, ts_p->current_blocks * ts_p->block_size);
    if (status != FBE_STATUS_OK)
    {
        fbe_rdgen_ts_set_unexpected_error_status(ts_p);
        fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_finished);
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: status %d from fbe_rdgen_ts_setup_sgs().\n", 
                                __FUNCTION__, status);
        return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
    }

    /* In performance mode, we only touch the buffers on the first pass.
     */
    if (!fbe_rdgen_specification_options_is_set(&ts_p->request_p->specification, FBE_RDGEN_OPTIONS_PERFORMANCE))
    {
        /* Zero out the full size of the memory we are reading.
         */
        fbe_rdgen_fill_sg_list(ts_p, ts_p->sg_ptr, FBE_RDGEN_PATTERN_CLEAR, FBE_FALSE, FBE_TRUE /*read_op*/);
    }
    else if((ts_p->request_p->specification.block_spec == FBE_RDGEN_BLOCK_SPEC_CONSTANT) &&
            (ts_p->io_count == 0) )
	{
        /* On the first pass of a performance run we will set the pattern to zeros. 
         * This ensures that the lba stamp will succeed when the loopback test is run. 
         */
        status = fbe_rdgen_fill_sg_list(ts_p, 
                                        ts_p->sg_ptr,
                                        FBE_RDGEN_PATTERN_ZEROS,
                                        FBE_TRUE,    /* Yes, append checksum */
                                        FBE_TRUE /*read_op*/ );
    }
    return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
}
/******************************************
 * end fbe_rdgen_ts_read_state0()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_ts_read_state1()
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

fbe_rdgen_ts_state_status_t fbe_rdgen_ts_read_state1(fbe_rdgen_ts_t *ts_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_payload_block_operation_flags_t payload_flags = 0;

    if (RDGEN_COND(ts_p->sg_ptr == NULL))
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: rdgen_ts sg pointer is null. \n", __FUNCTION__);
        fbe_rdgen_ts_set_unexpected_error_status(ts_p);
        fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_finished);
        return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
    }
    
    fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_read_state2);

    /* For the most part we always want to check the validity of data being
     * read.  The exception to this is:
     *  o The caller did not want to check the checksum (FBE_RDGEN_OPTIONS_DO_NOT_CHECK_CRC)
     *  o We purposefully wrote data with invalid crc and we want to confirm that RAID
     *    doesn't check the checksums.  The (1) operation that test this is:
     *      o FBE_RDGEN_OPERATION_CORRUPT_DATA_READ_CHECK
     */
    if (!(ts_p->request_p->specification.options & FBE_RDGEN_OPTIONS_DO_NOT_CHECK_CRC))
    {
        /* Reads will check checksums unless told not to do so.
         * Corrupt CRC/read/check needs to not detect invalid blocks on read so
         * they can be verified.
         */
        payload_flags = FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_CHECK_CHECKSUM;
    }

    status = fbe_rdgen_ts_send_packet(ts_p, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                      ts_p->current_lba,
                                      ts_p->current_blocks,
                                      ts_p->sg_ptr,
                                      payload_flags);

    if (status != FBE_STATUS_OK)
    {
        fbe_rdgen_ts_set_status(ts_p, FBE_STATUS_GENERIC_FAILURE);
        fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_finished);
        return FBE_RDGEN_TS_STATE_STATUS_EXECUTING; 
    }
    /* Send out the io packet.  Our callback always gets executed, thus
     * there is no status to handle here.
     */
    return FBE_RDGEN_TS_STATE_STATUS_WAITING;
}
/******************************************
 * end fbe_rdgen_ts_read_state1()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_ts_read_state2()
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
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_read_state2(fbe_rdgen_ts_t *ts_p)
{
    fbe_rdgen_ts_set_state(ts_p, ts_p->calling_state);

    return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
}
/******************************************
 * end fbe_rdgen_ts_read_state2()
 ******************************************/

/*************************
 * end file fbe_rdgen_read.c
 *************************/


