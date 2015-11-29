/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_rdgen_verify.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the rdgen verify state machine.
 *
 * @version
 *   8/17/2009 - Created. Rob Foley
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
 * fbe_rdgen_ts_verify_state0()
 ****************************************************************
 * @brief
 *  Generate the sg list.
 *
 * @param ts_p - current_ts.               
 *
 * @return FBE_RDGEN_TS_STATE_STATUS_EXECUTING   
 *
 * @author
 *  8/17/2009 - Created. Rob Foley
 *
 ****************************************************************/
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_verify_state0(fbe_rdgen_ts_t *ts_p)
{
    if (fbe_rdgen_ts_is_aborted(ts_p))
    {
        /* Kill the thread, we are terminating.
         */
         fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_finished);
    }
    else
    {
        fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_verify_state1);
    }
    return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
}
/******************************************
 * end fbe_rdgen_ts_verify_state0()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_ts_verify_state1()
 ****************************************************************
 * @brief
 *  Send out the packet.
 *
 * @param ts_p - current_ts.               
 *
 * @return FBE_RDGEN_TS_STATE_STATUS_EXECUTING   
 *
 * @author
 *  8/17/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_rdgen_ts_state_status_t fbe_rdgen_ts_verify_state1(fbe_rdgen_ts_t *ts_p)
{
    fbe_payload_block_operation_opcode_t opcode;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_verify_state2);

    if (ts_p->operation == FBE_RDGEN_OPERATION_READ_ONLY_VERIFY)
    {
        opcode = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ_ONLY_VERIFY;
    }
    else
    {
        opcode = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY;
    }

    status = fbe_rdgen_ts_send_packet(ts_p, opcode,
                                      ts_p->current_lba,
                                      ts_p->current_blocks,
                                      NULL, /* No sg */
                                      0 /* VERIFIES don't use the check CRC flag */);

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
 * end fbe_rdgen_ts_verify_state1()
 ******************************************/

/*!**************************************************************
 * fbe_rdgen_ts_verify_state2()
 ****************************************************************
 * @brief
 *  Process the completion.
 *
 * @param ts_p - current_ts.               
 *
 * @return FBE_RDGEN_TS_STATE_STATUS_EXECUTING   
 *
 * @author
 *  8/17/2009 - Created. Rob Foley
 *
 ****************************************************************/
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_verify_state2(fbe_rdgen_ts_t *ts_p)
{
    fbe_rdgen_ts_set_state(ts_p, ts_p->calling_state);

    return FBE_RDGEN_TS_STATE_STATUS_EXECUTING;
}
/******************************************
 * end fbe_rdgen_ts_verify_state2()
 ******************************************/

/*************************
 * end file fbe_rdgen_verify.c
 *************************/


