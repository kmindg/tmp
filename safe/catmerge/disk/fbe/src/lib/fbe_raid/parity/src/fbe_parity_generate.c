/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_parity_generate.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the generate code for parity objects.
 *
 * @version
 *   9/1/2009 - Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe_raid_library_proto.h"
#include "fbe_parity_io_private.h"
#include "fbe_raid_library.h"


/* Local Functions.
 */
static fbe_status_t fbe_parity_generate_read(fbe_raid_siots_t * const siots_p);
static fbe_status_t fbe_parity_generate_small_read(fbe_raid_siots_t * const siots_p);

static fbe_status_t fbe_parity_generate_write(fbe_raid_siots_t * const siots_p);
static fbe_status_t fbe_parity_generate_get_write_blocks(fbe_raid_siots_t * const siots_p);
fbe_status_t fbe_parity_generate_write_assign_state(fbe_raid_siots_t * const siots_p);

static fbe_status_t fbe_parity_generate_rebuild(fbe_raid_siots_t * const siots_p);
static fbe_status_t fbe_parity_generate_verify(fbe_raid_siots_t * const siots_p);
static fbe_status_t fbe_parity_generate_rekey(fbe_raid_siots_t * const siots_p);
static fbe_status_t fbe_parity_generate_zero(fbe_raid_siots_t * const siots_p);
static fbe_status_t fbe_parity_generate_check_zeroed(fbe_raid_siots_t * const siots_p);
static fbe_status_t fbe_parity_generate_write_log_hdr_rd(fbe_raid_siots_t * const siots_p);
static fbe_status_t fbe_parity_generate_journal_flush(fbe_raid_siots_t * const siots_p);

static fbe_u32_t fbe_parity_gen_deg_wr(fbe_raid_siots_t * const siots_p, fbe_u32_t blocks);
static fbe_status_t r5_gen_corrupt_crc(fbe_raid_siots_t * const siots_p);
static fbe_u32_t r5_gen_backfill(fbe_raid_siots_t * const siots_p);
static void r5_generate_complete_siots( fbe_raid_siots_t *siots_p );
static fbe_bool_t fbe_parity_degraded_recovery_allowed( fbe_raid_siots_t *siots_p );
static fbe_status_t fbe_parity_mr3_468_or_rcw(fbe_raid_siots_t * const siots_p);
static fbe_status_t fbe_parity_count_operations(fbe_raid_siots_t *const siots_p, 
                                                fbe_u32_t *r5_468_operations, 
                                                fbe_u32_t *r5_mr3_operations,
                                                fbe_u32_t *r5_rcw_operations);
static fbe_status_t fbe_parity_calc_parity_range(fbe_raid_siots_t * const siots_p);
fbe_status_t fbe_parity_read_write_calculate(fbe_raid_siots_t * siots_p);
fbe_status_t fbe_parity_get_max_read_write_blocks(fbe_raid_siots_t * siots_p,
                                             fbe_block_count_t *blocks_p);
fbe_status_t fbe_parity_limit_blocks_for_write_logging(fbe_raid_siots_t * siots_p,
                                                       fbe_block_count_t *blocks_p);

/* Globals
 */
fbe_u32_t RG_RAID3_PIPELINE_SIZE = -1;

/* Imports.
 */
#define RAID5_HOST_IO_CHECK_COUNT 9

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/*!**************************************************************
 * fbe_parity_generate_start()
 ****************************************************************
 * @brief
 *  This is the initial generate function for parity units.
 *
 * @param siots_p - current I/O.               
 *
 * @return FBE_RAID_STATE_STATUS_EXECUTING.
 *
 * @author
 *  9/1/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_parity_generate_start(fbe_raid_siots_t * siots_p)
{
    fbe_status_t status;
    fbe_raid_state_status_t return_status = FBE_RAID_STATE_STATUS_EXECUTING;
    fbe_raid_iots_t *iots_p = fbe_raid_siots_get_iots(siots_p);
    fbe_payload_block_operation_opcode_t opcode;

    fbe_raid_iots_get_current_opcode(iots_p, &opcode);

    /* Calculate appropriate siots request size and update iots.
     */
    switch (opcode)
    {
    case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ:
        status = fbe_parity_generate_read(siots_p);
        break;
    case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE:
    case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_VERIFY:
    case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_NONCACHED:
    case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_WRITE:
    case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_CORRUPT_DATA:
    case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_REKEY_WRITE:
    case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_ZEROS:
    case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_REKEY_WRITE_ZEROS:
        status = fbe_parity_generate_write(siots_p);
        break;

    case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY:
    case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ_ONLY_VERIFY:
    case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ERROR_VERIFY:
    case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INCOMPLETE_WRITE_VERIFY:
    case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_SYSTEM_VERIFY: 
    case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_SPECIFIC_AREA:
    case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ_ONLY_VERIFY_SPECIFIC_AREA:
        status = fbe_parity_generate_verify(siots_p);
        break;

    case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_READ_PAGED:
        status = fbe_parity_generate_rekey(siots_p);
        break;

    case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_REBUILD:
        status = fbe_parity_generate_rebuild(siots_p);
        break;

    case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ZERO:
    case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_UNMARK_ZERO:
    case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_REKEY_ZERO:
        status = fbe_parity_generate_zero(siots_p);
        break;

    case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_CHECK_ZEROED:
        status = fbe_parity_generate_check_zeroed(siots_p);
        break;

    case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_LOG_HDR_RD:
        status = fbe_parity_generate_write_log_hdr_rd(siots_p);
        break;

    case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_LOG_FLUSH:
        status = fbe_parity_generate_journal_flush(siots_p);
        break;

    default:
        /* If the requested block size or requested optimum size are zero then
         * return an error. 
         */
        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_WARNING,
                                    "parity: %s: unsupported opcode:0x%x,siots_p:0x%p \n", 
                                    "parity_gen_start", opcode, siots_p);
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_invalid_opcode);
        return FBE_RAID_STATE_STATUS_EXECUTING;
        break;
    }

    /* Check status from the above call to generate.
     */
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_TRACE_LEVEL_ERROR, __LINE__, "parity_gen_start",
                             "parity: opcode=0x%x, unexpected generate error %d algorithm: 0x%x\n", 
                             opcode, status, siots_p->algorithm);
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_unexpected_error);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }
    /* Verify that generate has set some important values.
     */
    if (RAID_DBG_COND(siots_p->algorithm == RG_NO_ALG))
    {
        fbe_raid_siots_trace(siots_p, FBE_TRACE_LEVEL_ERROR, __LINE__, "parity_gen_start",
                             "parity: unexpected algorithm: 0x%x\n", 
                             siots_p->algorithm);
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_unexpected_error);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }
    if (RAID_DBG_COND(siots_p->parity_count == 0))
    {
        fbe_raid_siots_trace(siots_p, FBE_TRACE_LEVEL_ERROR, __LINE__, "parity_gen_start",
                             "parity: parity count %llu unexpected\n", 
                             (unsigned long long)siots_p->parity_count);
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_unexpected_error);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }
    if (RAID_DBG_COND(siots_p->xfer_count == 0))
    {
        fbe_raid_siots_trace(siots_p, FBE_TRACE_LEVEL_ERROR, __LINE__, "parity_gen_start",
                             "parity: xfer count %llu unexpected\n", 
                             (unsigned long long)siots_p->xfer_count);
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_unexpected_error);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* Uses Function Table found in RGDB to determine generate_start function.
     */
    if (RAID_DBG_COND(siots_p->common.state == (fbe_raid_common_state_t)fbe_parity_generate_start))
    {
        fbe_raid_siots_trace(siots_p, FBE_TRACE_LEVEL_ERROR, __LINE__, "parity_gen_start",
                             "parity: generate state not set (%p)\n", 
                             siots_p->common.state);
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_unexpected_error);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* This must be the final siots in the chain.
     */
    if (RAID_DBG_COND(fbe_raid_siots_get_next(siots_p) != NULL))
    {
        fbe_raid_siots_trace(siots_p, FBE_TRACE_LEVEL_ERROR, __LINE__, "parity_gen_start",
                             "parity: %s current siots %p next siots %p unexpected\n", 
                             "parity_gen_start", siots_p, fbe_raid_siots_get_next(siots_p));
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_unexpected_error);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    if (fbe_raid_siots_is_flag_set(siots_p, FBE_RAID_SIOTS_FLAG_ONE_SIOTS))
    {
        fbe_raid_siots_log_traffic(siots_p, "gen start");
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }
    else
    {
        /***************************************************
         * This sub request is now startable.
         * Determine if this sub request can proceed,
         * or if it must wait for preceeding sub requests
         * to finish.
         ***************************************************/
        return_status = fbe_raid_siots_generate_get_lock(siots_p);
        return return_status;
    }
}
/*******************************
 * end fbe_parity_generate_start()
 *******************************/

/*!**************************************************************
 * fbe_parity_siots_limit_to_one_parity_extent()
 ****************************************************************
 * @brief
 *  Limit the request to one parity element.
 *
 * @param  siots_p - current io.
 * @param blocks_per_element - current element size.
 * @param blocks - current number of blocks to execute.
 *
 * @return fbe_u32_t new blocks.
 *
 * @author
 *  11/4/2009 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_block_count_t fbe_parity_siots_limit_to_one_parity_extent(fbe_raid_siots_t *siots_p,
                                                                     fbe_u32_t blocks_per_element,
                                                                     fbe_block_count_t blocks)
{       
    fbe_lba_t start_lba = siots_p->start_lba;

    /***********************************************************************
     * There are two parity regions to this request,
     * for now we will break it up into two small reads rather than
     * allow two parity regions.
     * Degraded read does not support non-continuous parity regions.
     *
     *                 -------- -------- -------- --------
     *                |        |********|        |******* | <---- Pre-read 1
     *                |        |********|        |******* |
     *                |        |        |        |        |
     *                |        |        |        |        |
     *                |********|        |        |******* | <---- Pre-read 2
     *                |********|        |        |******* |
     * Break here --> |________|________|________|________|
     *
     *
     ***********************************************************************/
    if ((blocks < blocks_per_element) &&
        (blocks > (blocks_per_element - (start_lba % blocks_per_element))))
    {
        /* Break this on an element boundary, rather than allow
         * two separate parity regions.
         */
        blocks = (fbe_u32_t)(blocks_per_element - (fbe_u32_t)(start_lba % blocks_per_element));
    }
    return blocks;
}
/******************************************
 * end fbe_parity_siots_limit_to_one_parity_extent()
 ******************************************/

/****************************************************************
 * fbe_parity_mr3_is_aligned()
 ****************************************************************
 * @brief
 *  Determine if we are stripe aligned.
 *  
 * @param siots_p - Current sub request.
 *
 * @return FBE_TRUE if aligned FBE_FALSE otherwise.
 *
 ****************************************************************/
fbe_bool_t fbe_parity_mr3_is_aligned(fbe_raid_siots_t * const siots_p)
{
    fbe_bool_t b_aligned = FBE_TRUE;
    fbe_u32_t blocks_per_element = fbe_raid_siots_get_blocks_per_element(siots_p);
    fbe_u16_t array_width = fbe_raid_siots_get_width(siots_p);
    fbe_u32_t parity_drives = fbe_raid_siots_parity_num_positions(siots_p);
    fbe_u32_t blocks_per_stripe;

    blocks_per_stripe = blocks_per_element * (array_width - parity_drives);

    /* We are not aligned if we are not stripe aligned.
     */
    if ((siots_p->start_lba % blocks_per_stripe) != 0)
    {
        b_aligned = FBE_FALSE;
    }
    if (((siots_p->start_lba + siots_p->xfer_count) % blocks_per_stripe) != 0)
    {
        b_aligned = FBE_FALSE;
    }
    return b_aligned;
}
/**************************************
 * end fbe_parity_mr3_is_aligned()
 **************************************/
/****************************************************************
 * fbe_parity_generate_small_write()
 ****************************************************************
 * @brief
 *  Generate a RAID 5 write request by filling out the siots.
 *  
 * @param siots_p - Current sub request.
 *
 * @return fbe_status_t
 *
 ****************************************************************/

fbe_status_t fbe_parity_generate_small_write(fbe_raid_siots_t * const siots_p,
                                             fbe_raid_geometry_t *raid_geometry_p)
{
    fbe_status_t status;
    fbe_raid_iots_t *iots_p = fbe_raid_siots_get_iots(siots_p);
    status = fbe_parity_get_small_write_geometry(raid_geometry_p, siots_p->start_lba, &siots_p->geo);

    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        /* There was an invalid parameter in the siots, return error now.
         */
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity:fail to get lun geom:stat=0x%x,"
                             "raid_geom=0x%p,siots=0x%p\n",
                             status, raid_geometry_p, siots_p);
        return  status;
    }

    siots_p->parity_pos = fbe_raid_siots_get_parity_pos(siots_p);
    if (RAID_DBG_COND(siots_p->parity_pos == FBE_RAID_INVALID_PARITY_POSITION))
    {
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "siots_p->parity_pos 0x%x == FBE_RAID_INVALID_PARITY_POSITION\n",
                             siots_p->parity_pos);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    siots_p->start_pos = fbe_raid_siots_get_start_pos(siots_p);
    siots_p->data_disks = 1;
    fbe_raid_siots_init_dead_positions(siots_p);

    fbe_raid_iots_dec_blocks(iots_p, siots_p->xfer_count);
    if (iots_p->current_operation_blocks == siots_p->xfer_count)
    {
        fbe_raid_siots_set_flag(siots_p, FBE_RAID_SIOTS_FLAG_ONE_SIOTS | FBE_RAID_SIOTS_FLAG_DONE_GENERATING);
    }

    siots_p->parity_start = siots_p->geo.logical_parity_start + siots_p->geo.start_offset_rel_parity_stripe;
    siots_p->parity_count = siots_p->xfer_count;
    fbe_raid_siots_transition(siots_p, fbe_parity_small_468_state0);
    siots_p->algorithm = R5_SMALL_468;
    if (!fbe_raid_iots_is_flag_set(fbe_raid_siots_get_iots(siots_p), FBE_RAID_IOTS_FLAG_LAST_IOTS_OF_REQUEST))
    {
        fbe_raid_iots_t *iots_p = fbe_raid_siots_get_iots(siots_p);
        fbe_raid_siots_object_trace(siots_p, FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                    "IOTS flag for last iots not set on fast write iots: %p fl: 0x%x\n",
                                    iots_p, iots_p->flags);
    }
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_parity_generate_small_write()
 **************************************/
/****************************************************************
 * fbe_parity_generate_write()
 ****************************************************************
 * @brief
 *  Generate a RAID 5 write request by filling out the siots.
 *  
 * @param siots_p - Current sub request.
 *
 * @return fbe_status_t
 *
 ****************************************************************/

fbe_status_t fbe_parity_generate_write(fbe_raid_siots_t * const siots_p)
{
    fbe_status_t status;
    fbe_block_count_t blocks;
    fbe_raid_iots_t *iots_p = fbe_raid_siots_get_iots(siots_p);
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_iots_get_raid_geometry(iots_p);

    /* We only do the optimized small write path for > width 3 on R5/R3 and > width 4 on R6.
     */
    if (fbe_raid_iots_is_flag_set(iots_p, FBE_RAID_IOTS_FLAG_NON_DEGRADED | FBE_RAID_IOTS_FLAG_FAST_WRITE) &&
        !fbe_raid_geometry_io_needs_alignment(raid_geometry_p, siots_p->start_lba, siots_p->xfer_count) &&
        ((raid_geometry_p->width > 4) ||
         ( !fbe_raid_geometry_is_raid6(raid_geometry_p) && 
           (raid_geometry_p->width == 4)) ) )
    {
        return fbe_parity_generate_small_write(siots_p, raid_geometry_p);
    }

    /* Fetch the geometry information first.
     */
    status = fbe_parity_get_lun_geometry(raid_geometry_p, siots_p->start_lba, &siots_p->geo);

    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        /* There was an invalid parameter in the siots, return error now.
         */
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity:fail to get lun geom:stat=0x%x,"
                             "raid_geom=0x%p,siots=0x%p\n",
                             status, raid_geometry_p, siots_p);
        return  status;
    }

    siots_p->parity_pos = fbe_raid_siots_get_parity_pos(siots_p);
    if (RAID_COND(siots_p->parity_pos == FBE_RAID_INVALID_PARITY_POSITION))
    {
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "siots_p->parity_pos 0x%x == FBE_RAID_INVALID_PARITY_POSITION\n",
                             siots_p->parity_pos);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    siots_p->start_pos = fbe_raid_siots_get_start_pos(siots_p);
    siots_p->retry_count = FBE_RAID_DEFAULT_RETRY_COUNT;

    blocks = siots_p->xfer_count;

    /* Step 2: Limit this request.
     */
    status = fbe_parity_generate_get_write_blocks(siots_p);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: failed to get write blocks : status = 0x%x, siots_p = 0x%p\n",
                             status, siots_p);
        return  status;
    }

    /* This siots cannot be for more blocks than remain in the request.
     */
    if (RAID_COND(siots_p->xfer_count > (fbe_raid_siots_get_iots(siots_p))->blocks_remaining))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: xfer_count 0x%llx > iots blocks remaining 0x%llx : siots_p = 0x%p\n",
                             (unsigned long long)siots_p->xfer_count, (unsigned long long)(fbe_raid_siots_get_iots(siots_p))->blocks_remaining,
                             siots_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Adjust the IOTS to account for this request
     */
    fbe_raid_iots_dec_blocks(fbe_raid_siots_get_iots(siots_p), siots_p->xfer_count);

    status = fbe_parity_generate_write_assign_state(siots_p);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: failed to assign state : status = 0x%x, siots_p = 0x%p\n",
                             status, siots_p);
        return  status;
    }
    return FBE_STATUS_OK;
}
/*****************************************
 * end of fbe_parity_generate_write()
 *****************************************/
/****************************************************************
 * fbe_parity_generate_small_read()
 ****************************************************************
 * @brief
 *  Generate a small RAID 5 read request by filling out the siots.
 *
 * @param siots_p - Current sub request. 
 *
 * @return
 *   fbe_status_t
 *
 * @author
 *   5/09/99 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_parity_generate_small_read(fbe_raid_siots_t * const siots_p)
{
    fbe_raid_iots_t * iots_p = NULL;
    fbe_block_count_t iots_blocks = 0;
    fbe_bool_t fbe_cond = FBE_FALSE;

    iots_p = fbe_raid_siots_get_iots(siots_p);

    if (RAID_COND(iots_p == NULL))
    {
         fbe_raid_siots_trace(siots_p, 
                              FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                              "parity: iots_p == NULL, siots_p = 0x%p\n",
                              siots_p);
         return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_raid_siots_get_blocks(siots_p, &iots_blocks);

    if (RAID_COND(siots_p->xfer_count == 0))
    {
         fbe_raid_siots_trace(siots_p,FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                              "parity: siots_p->xfer_count == 0, siots_p = 0x%p\n",
                              siots_p);
         return FBE_STATUS_GENERIC_FAILURE;
    }

    /***************************************************
     * We only go down the "Small read" path when
     *   either this is a host transfer, 
     *   or the entire request is small.
     *
     * If this is a cached transfer that has an offset,
     *   we will not use the small read path regardless of
     *   the size.  The small read path is optimized for
     *   small operations and does not handle an offset.
     *
     * Cached transfers that are broken up will not
     *   use the small read state machine, since
     *   they need to allocate a separate SG list for
     *   the read from the drives.
     *
     *   For Cached Ops, the Small Read Path optimizes by giving
     *   the lower driver the sg passed in by the cache,
     *   and when a cache request is broken up by us,
     *   there is no SG to give the lower driver...
     *   
     ***************************************************/
    if ((fbe_raid_siots_is_buffered_op(siots_p) ||
        ((iots_blocks == siots_p->xfer_count) &&
         (fbe_raid_siots_get_iots(siots_p)->host_start_offset == 0))))
    {
        siots_p->algorithm = R5_SM_RD;
        fbe_raid_siots_transition(siots_p, fbe_parity_small_read_state0);
    }
    else
    {
        fbe_raid_siots_transition(siots_p, fbe_parity_read_state0);
        siots_p->algorithm = R5_RD;
    }

    fbe_cond = ((siots_p->algorithm != R5_SM_RD) ||
                 fbe_raid_siots_is_buffered_op(siots_p) ||
                 (fbe_raid_siots_get_iots(siots_p)->host_start_offset == 0));
    
    if(RAID_COND(!fbe_cond))
    {
        fbe_payload_block_operation_opcode_t opcode;
        fbe_raid_siots_get_opcode(siots_p, &opcode);

        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: unexpected siots_p->algorithm: %d -OR- is not buffered operation %d "
                             "-OR- host_start_offset %llx != 0\n",
                             siots_p->algorithm, opcode, 
                             (unsigned long long)fbe_raid_siots_get_iots(siots_p)->host_start_offset);
        return FBE_STATUS_GENERIC_FAILURE;
    }


    /* Because this is a small read, the setup of most fields
     * is always the same.
     */
    siots_p->parity_start = siots_p->geo.logical_parity_start + siots_p->geo.start_offset_rel_parity_stripe;
    siots_p->parity_count = siots_p->xfer_count;
    siots_p->data_disks = 1;

    /* Adjust the iots to account for this request
     */
    fbe_raid_iots_dec_blocks(fbe_raid_siots_get_iots(siots_p), siots_p->xfer_count);

    {
#ifdef FBE_RAID_DEBUG
        /* Fetch the parity range of this request
         */
        fbe_raid_extent_t parity_range[2];

        fbe_raid_get_stripe_range(siots_p->start_lba,
                                  siots_p->xfer_count,
                                  fbe_raid_siots_get_blocks_per_element(siots_p),
                                  fbe_raid_siots_get_width(siots_p) - parity_drives,
                                  FBE_RAID_MAX_PARITY_EXTENTS,
                                  parity_range);

        /* Fill out the parity range from the
         * values returned by fbe_raid_get_stripe_range
         */
        fbe_cond = (siots_p->parity_start != parity_range[0].start_lba) ||
                   (siots_p->parity_count != parity_range[0].size) ||
                   (parity_range[1].size != 0);
        if(RAID_COND(fbe_cond))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "parity: unexpected error: invalid range, parity start: 0x%llx, "
                                 "parity count:%d, start lba: 0x%llx, size = %d, siots_p = 0x%p\n",
                                 (unsigned long long)siots_p->parity_start, siots_p->parity_count,
                                 (unsigned long long)parity_range[0].start_lba,parity_range[0].size,
                                 siots_p);
            return FBE_STATUS_GENERIC_FAILURE;
        }

#endif
    }

    siots_p->drive_operations = 1;
    
    return FBE_STATUS_OK;
}
/*****************************************
 * fbe_parity_generate_small_read()
 *****************************************/
/****************************************************************
 * fbe_parity_generate_normal_read()
 ****************************************************************
 * @brief
 *  Generate a normal RAID 5 read request by filling out the siots.
 *
 * @param siots_p - Current sub request. 
 *
 * @return fbe_status_t
 *
 ****************************************************************/

static fbe_status_t fbe_parity_generate_normal_read(fbe_raid_siots_t * const siots_p)
{
    fbe_block_count_t blocks = siots_p->xfer_count;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_status_t status;

    /* Raid 3s do not limit to parity stripe size since
     * the parity stripe size is the entire group. 
     */
    if (!fbe_raid_geometry_is_raid3(raid_geometry_p))
    {
        blocks = FBE_MIN(blocks, siots_p->geo.max_blocks);
        siots_p->xfer_count = blocks;
    }
    /* We should not generate more than the transfer count.
     * We check this here to validate the return value from
     * the above function.
     */
    if (RAID_COND(blocks > siots_p->xfer_count))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: blocks %llu > xfer_count %llu\n",
                             (unsigned long long)blocks, (unsigned long long)siots_p->xfer_count);
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_unexpected_error);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    if (RAID_COND(blocks == 0))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: blocks is zero\n");
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_unexpected_error);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Calculate some initial values in the siots.
     */
    status = fbe_parity_read_write_calculate(siots_p);

    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        /* There was an invalid parameter in the siots, return error now.
         */
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: failed to calculate read writer blocks: status = 0x%x, siots_p = 0x%p\n",
                             status, siots_p);
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_unexpected_error);
        return status; 
    }
    /* Limit the request based on the sg list size.
     * Set algorithm since below needs to know if we are a read. 
     */
    siots_p->algorithm = R5_RD;
    status = fbe_parity_get_max_read_write_blocks(siots_p, &blocks);

    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: failed to limit read or write request: status = 0x%x, siots_p = 0x%p\n", 
                             status, siots_p);
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_unexpected_error);
        return  status;
    }

    if (RAID_COND(blocks == 0))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: blocks is zero\n");
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_unexpected_error);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* Our transfer count must always be less than our parity range.
     */
    if (RAID_COND(siots_p->xfer_count < siots_p->parity_count))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: xfer_count %llu < parity_count %llu \n",
			     (unsigned long long)siots_p->xfer_count, (unsigned long long)siots_p->parity_count );
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_unexpected_error);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* Now that we have settled on a block count, set it into the siots.
     */
    siots_p->xfer_count = blocks;
    return FBE_STATUS_OK;
}
/*****************************************
 * fbe_parity_generate_normal_read()
 *****************************************/
/****************************************************************
 * fbe_parity_gen_deg_rd()
 ****************************************************************
 * @brief
 *  Generate a degraded read.  We are degraded, determine
 *  if we should go to the degraded read state machine.
 *
 * @param siots_p - Current sub request.
 *
 * @return
 *  none
 *
 * @author
 *  06/28/00 - Created. Rob Foley
 *  06/29/01 - Changed to choose between sdrd and drd. Rob Foley
 *
 ****************************************************************/

void fbe_parity_gen_deg_rd(fbe_raid_siots_t * const siots_p)
{
    if (fbe_parity_degraded_read_required(siots_p))
    {
        /* The operation hits the dead drive,
         * so we must use the degraded read path to
         * perform a reconstruct.
         */
        siots_p->algorithm = R5_DEG_RD;
        fbe_raid_siots_transition(siots_p, fbe_parity_degraded_read_state0);
    }
    else
    {
        /* The operation does not hit a dead drive.
         * Thus, we should use the normal read path.
         */
        fbe_raid_siots_transition(siots_p, fbe_parity_read_state0);
        siots_p->algorithm = R5_RD;
    }
    return;
}
/* fbe_parity_gen_deg_rd() */
/****************************************************************
 * fbe_parity_generate_read()
 ****************************************************************
 * @brief
 *  Generate a RAID 5 read request by filling out the siots.
 *
 * @param siots_p - Current sub request.
 *
 * @return fbe_status_t
 *
 ****************************************************************/

fbe_status_t fbe_parity_generate_read(fbe_raid_siots_t * const siots_p)
{
    fbe_status_t status;
    fbe_element_size_t blocks_per_element;
    fbe_block_count_t blocks;
    fbe_lba_t start_lba;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);

    /* Fetch the geometry information first.
     */
    status = fbe_parity_get_lun_geometry(raid_geometry_p, siots_p->start_lba, &siots_p->geo);

    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        /* There was an invalid parameter in the siots, return error now.
         */
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: get geometry status 0x%x, siots_p %p\n", status, siots_p);
        return status;
    }

    fbe_raid_siots_geometry_get_parity_pos(siots_p, raid_geometry_p, &siots_p->parity_pos);
    if (RAID_COND(siots_p->parity_pos == FBE_RAID_INVALID_PARITY_POSITION))
    {
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: siots_p->parity_pos 0x%x == FBE_RAID_INVALID_PARITY_POSITION\n",
                             siots_p->parity_pos);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    siots_p->start_pos = fbe_raid_siots_get_start_pos(siots_p);
    siots_p->retry_count = FBE_RAID_DEFAULT_RETRY_COUNT;

    /***************************************************
     * Step 1: Do initial calculations
     ***************************************************
     * Grab the geometry information so we can
     * determine how far this request must go.
     * We currently have an xfer_count that is
     * set to the total remaining blocks,
     * so we **may** need to break this request at an
     * intelligent boundary.
     *
     ***************************************************/
    blocks = siots_p->xfer_count;
    start_lba = siots_p->start_lba;
    fbe_raid_geometry_get_element_size(raid_geometry_p, &blocks_per_element);

    /***************************************************
     * Step 2: Check for Degraded Unit
     ***************************************************
     * Determine if this unit is degraded.
     * If so, we will fill out the siots_p->dead_pos
     * and blocks.
     *
     ***************************************************/
    if (fbe_raid_iots_is_flag_set((fbe_raid_iots_t*)siots_p->common.parent_p, FBE_RAID_IOTS_FLAG_NON_DEGRADED))
    {
        fbe_raid_siots_init_dead_positions(siots_p);
    }
    else
    {
        status = fbe_parity_check_for_degraded(siots_p);

        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "parity: failed to check for degraded: status = 0x%x, siots_p = 0x%p\n",
                                 status, siots_p);
            return  status;
        }
    }

    /* Make sure we only have a single parity extent.
     */
    blocks = fbe_parity_siots_limit_to_one_parity_extent(siots_p, blocks_per_element, blocks);

    /***************************************************
     * Step 3: Check for small read (R5_SM_RD)
     ***************************************************
     * First check for small read.
     * Small reads go to a single disk and are NOT degraded.
     * We optimize generate for small reads since
     * the setup is very straighforward.
     *
     ***************************************************/
    if ((fbe_raid_siots_dead_pos_get(siots_p, FBE_RAID_FIRST_DEAD_POS) == FBE_RAID_INVALID_DEAD_POS) &&
        !fbe_raid_geometry_needs_alignment(raid_geometry_p) &&
        (blocks < (blocks_per_element - 
                   (siots_p->geo.start_offset_rel_parity_stripe % blocks_per_element))))
    {
        /* We will do the entire setup for small reads here,
         * since the setup is so simple!
         */
        siots_p->xfer_count = blocks;
        status = fbe_parity_generate_small_read(siots_p);
        return status;
    } /* end if small read */
    else
    {
        /* Not a small read.
         */
        siots_p->xfer_count = blocks;
        status = fbe_parity_generate_normal_read(siots_p);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "parity: failed in normla read generation phase: blocks 0x%llx > xfer_count 0x%llx : "
                                 "siots_p = 0x%p, status = 0x%x\n",
                                 (unsigned long long)blocks, (unsigned long long)siots_p->xfer_count, siots_p, status);
            return status;
        }

        /* Adjust the iots to account for this request
         */
        fbe_raid_iots_dec_blocks(fbe_raid_siots_get_iots(siots_p), siots_p->xfer_count);

        /* Now determine if the unit is degraded or not.
         * If all the dead drives are parity drives, then
         * we are allowed to simply use normal write since
         * we do not need to read any parity drives.
         */
        if ( ( fbe_raid_siots_dead_pos_get(siots_p, FBE_RAID_FIRST_DEAD_POS) != 
               FBE_RAID_INVALID_DEAD_POS ) &&
             ( fbe_raid_siots_no_dead_data_pos(siots_p) == FBE_FALSE ) )
        {
            /* Choose an algorithm for degraded read.
             */
            fbe_parity_gen_deg_rd(siots_p);
        }
        else
        {
            /* We are not degraded so do a normal read.
             */
            fbe_raid_siots_transition(siots_p, fbe_parity_read_state0);
            siots_p->algorithm = R5_RD;
        }

        siots_p->drive_operations = siots_p->data_disks;
    }                           /* end not a small read */

    if ((blocks < blocks_per_element) &&
        (blocks > (blocks_per_element - (start_lba % blocks_per_element))))
    {
        /* We should not generate a discontinuous parity range.
         */
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: two parity ranges unexpected lba: %llx bl: 0x%llx elsz: 0x%llx\n",
                             (unsigned long long)start_lba, (unsigned long long)blocks, (unsigned long long)blocks_per_element);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}
/*****************************************
 * end fbe_parity_generate_read()
 *****************************************/
/****************************************************************
 *  fbe_parity_check_for_degraded()
 ****************************************************************
 * @brief
 *  This function determines the range of the I/O if we are degraded.
 *  
 *
 * @param siots_p - current siots
 * @param current_blocks - Current block count for this siots.
 * @param b_allowed_to_log - FBE_TRUE if allowed to log, FBE_FALSE otherwise.
 *
 * @return
 *  fbe_status_t
 *
 * @author
 *  03/07/06 - Created. Rob Foley
 *  07/04/08 - Modified for Rebuild Logging Support For R6. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_parity_check_for_degraded( fbe_raid_siots_t *siots_p)
{
    fbe_status_t status;

    /* Get the values for the degraded offset array and for our dead positions.
     * This function sets the dead positions in the siots as a side effect.
     */
    status = fbe_parity_siots_init_dead_position(siots_p, fbe_raid_siots_get_width(siots_p), NULL);
    return status;
}
/* end fbe_parity_check_for_degraded */

/****************************************************************
 *  fbe_parity_generate_rebuild()
 ****************************************************************
 * @brief
 *  This function fills out a siots for the next
 *  rebuild request.
 *
 * @param siots_p - current siots
 *
 * @return fbe_status_t
 *
 * @author
 *  10/18/99 - Created. JJ
 *  12/01/99 - Rewritten. Rob Foley
 *  01/11/05 - Split some verify functionality into 
 *             r5_verify_check_degraded.  HR.
 *
 ****************************************************************/

fbe_status_t fbe_parity_generate_rebuild(fbe_raid_siots_t * const siots_p)
{
    fbe_status_t status;
    fbe_u32_t parity_drives = fbe_raid_siots_parity_num_positions(siots_p);
    fbe_payload_block_operation_opcode_t opcode;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_u32_t first_dead_pos;
    fbe_u32_t second_dead_pos;
    fbe_element_size_t      element_size;
    fbe_block_count_t       max_blocks_per_drive;

    fbe_raid_siots_get_opcode(siots_p, &opcode);

    /* Get the element size
     */
    fbe_raid_geometry_get_element_size(raid_geometry_p, &element_size);

    /* Get the physical information for this request.
     */
    fbe_parity_get_physical_geometry(raid_geometry_p, siots_p->start_lba, &siots_p->geo);

    if(RAID_COND( 0!= (siots_p->geo.start_offset_rel_parity_stripe + 
                       siots_p->geo.blocks_remaining_in_parity) % 
                       (element_size)))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "SIOTS not aligned with element size, start offset: %llx\n",
                             (unsigned long long)siots_p->geo.start_offset_rel_parity_stripe);
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "block remaining in parity: %llu, element size %d\n",
                             (unsigned long long)siots_p->geo.blocks_remaining_in_parity,
                             element_size);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    siots_p->parity_pos = fbe_raid_siots_get_parity_pos(siots_p);
    if (RAID_COND(siots_p->parity_pos == FBE_RAID_INVALID_PARITY_POSITION))
    {
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "siots_p->parity_pos 0x%x == FBE_RAID_INVALID_PARITY_POSITION\n",
                             siots_p->parity_pos);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    siots_p->start_pos = fbe_raid_siots_get_start_pos(siots_p);
    siots_p->data_disks = fbe_raid_siots_get_width(siots_p) - parity_drives;

    /* Update the info in fbe_raid_siots_t.
     */
    siots_p->algorithm = R5_RB;
    siots_p->retry_count = FBE_RAID_DEFAULT_RETRY_COUNT;

    /* See if we are degraded.
     */
    status = fbe_parity_check_degraded_phys(siots_p);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: failed to check degraded : status = 0x%x, siots_p = 0x%p\n", 
                             status, siots_p);
        return  status;
    }
    /* We should always have a dead position.
     */
    first_dead_pos = fbe_raid_siots_dead_pos_get(siots_p, FBE_RAID_FIRST_DEAD_POS);
    if (RAID_COND(first_dead_pos >= fbe_raid_siots_get_width(siots_p)))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "parity: invalid 1st rebuild position: %d, width %d\n",
                             first_dead_pos,
                             fbe_raid_siots_get_width(siots_p));
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* We can optionally have a 2nd dead pos on R6.
     */
    second_dead_pos = fbe_raid_siots_dead_pos_get(siots_p, FBE_RAID_SECOND_DEAD_POS);
    if (RAID_COND((second_dead_pos != FBE_RAID_INVALID_DEAD_POS) &&
                  ((second_dead_pos >= fbe_raid_siots_get_width(siots_p)) ||
                   (!fbe_raid_geometry_is_raid6(raid_geometry_p)))))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "parity: invalid 2nd rebuild position: %d, width %d\n",
                             second_dead_pos,
                             fbe_raid_siots_get_width(siots_p));
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_raid_siots_transition(siots_p, fbe_parity_rebuild_state0);

    /* First limit the size to the parity stripe size.
     */
    if (!fbe_raid_geometry_is_raid3(raid_geometry_p))
    {
        /* Limit the request to this parity stripe and
         */
        siots_p->xfer_count = FBE_MIN(siots_p->xfer_count, (fbe_u32_t)siots_p->geo.blocks_remaining_in_parity);
    }

    /* Now limit the physical rebuild request to the `per drive' limit.
     */
    fbe_raid_geometry_get_max_blocks_per_drive(raid_geometry_p, &max_blocks_per_drive);
    siots_p->xfer_count = FBE_MIN(siots_p->xfer_count, max_blocks_per_drive);

    /* Setup the parity start and parity count.
     */
    siots_p->parity_start = siots_p->start_lba;
    siots_p->parity_count = siots_p->xfer_count;

    /*
     * Its a fix for DIMS 185519, which intends to improve the rebuild
     * time for R3 LUN if compared to R5.
     *
     * R3 is having only one parity stripe but R5 can have more than one.
     * Because of the above reason, multiple SIOTS does not get started 
     * parallely in R3 LUN as the parity range of the current SIOTS always 
     * conflicts with the parity range of previous SIOTS, which always spans 
     * across the entire LUN. Decision of whether the parity region overlaps
     * with the previous region is taken on the basis of content of 
     * siots->geo.
     *
     * Setting siots->geo.logical_parity_count to siots_p->parity_count 
     * will result in avoidance of the above conflict in case of R3.
    */
    siots_p->geo.logical_parity_count = siots_p->parity_count;

    /* Adjust the iots to account for this request
     */
    fbe_raid_iots_dec_blocks(fbe_raid_siots_get_iots(siots_p), siots_p->xfer_count);

    /* Drive operations are just the width.
     */
    siots_p->drive_operations = fbe_raid_siots_get_width(siots_p);
    return FBE_STATUS_OK;
}
/*****************************************
 * end fbe_parity_generate_rebuild()
 *****************************************/

/****************************************************************
 *  fbe_parity_generate_verify()
 ****************************************************************
 * @brief
 *  This function fills out a siots for the next
 *  verify request.
 *
 * @param siots_p - current siots
 *
 * @return
 *  none
 *
 * @author
 *  10/18/99 - Created. JJ
 *  12/01/99 - Rewritten. Rob Foley
 *  01/11/05 - Split some verify functionality into 
 *             r5_verify_check_degraded.  HR.
 *
 ****************************************************************/

fbe_status_t fbe_parity_generate_verify(fbe_raid_siots_t * const siots_p)
{
    fbe_u32_t parity_drives = fbe_raid_siots_parity_num_positions(siots_p);
    fbe_payload_block_operation_opcode_t opcode;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_block_count_t max_blocks_per_drive;

    fbe_raid_siots_get_opcode(siots_p, &opcode);

    fbe_parity_get_physical_geometry(raid_geometry_p, siots_p->start_lba, &siots_p->geo);

    if(RAID_COND( 0!= (siots_p->geo.start_offset_rel_parity_stripe +
                       siots_p->geo.blocks_remaining_in_parity) % 
                       fbe_raid_siots_get_blocks_per_element(siots_p)))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: SIOTS not aligned with element size, start offset: %llx\n",
                             (unsigned long long)siots_p->geo.start_offset_rel_parity_stripe);
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "block remaining in parity: %llu, element size: %d\n",
                             (unsigned long long)siots_p->geo.blocks_remaining_in_parity,
                             fbe_raid_siots_get_blocks_per_element(siots_p));
        return FBE_STATUS_GENERIC_FAILURE;
    }

    siots_p->parity_pos = fbe_raid_siots_get_parity_pos(siots_p);
    if (RAID_COND(siots_p->parity_pos == FBE_RAID_INVALID_PARITY_POSITION))
    {
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: siots_p->parity_pos 0x%x == FBE_RAID_INVALID_PARITY_POSITION\n",
                             siots_p->parity_pos);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    siots_p->start_pos = fbe_raid_siots_get_start_pos(siots_p);
    siots_p->data_disks = fbe_raid_siots_get_width(siots_p) - parity_drives;

    /* Limit the request to this parity stripe.
     */
    siots_p->xfer_count = 
    FBE_MIN(siots_p->xfer_count,
            (fbe_u32_t)siots_p->geo.blocks_remaining_in_parity);

    fbe_raid_geometry_get_max_blocks_per_drive(raid_geometry_p, &max_blocks_per_drive);

    siots_p->xfer_count = FBE_MIN(siots_p->xfer_count, max_blocks_per_drive);

    /* Setup verify specific values in siots.
     */
    fbe_raid_siots_transition(siots_p, fbe_parity_verify_state0);
    siots_p->algorithm = R5_VR;
    siots_p->retry_count = FBE_RAID_DEFAULT_RETRY_COUNT;

    siots_p->parity_start = siots_p->start_lba,
    siots_p->parity_count = siots_p->xfer_count;

    /* Adjust the iots to account for this request
     */
    fbe_raid_iots_dec_blocks(fbe_raid_siots_get_iots(siots_p), siots_p->xfer_count);

    /* Drive operations are just the width.
     */
    siots_p->drive_operations = fbe_raid_siots_get_width(siots_p);
    return FBE_STATUS_OK;
}
/*****************************************
 * end fbe_parity_generate_verify()
 *****************************************/

/****************************************************************
 *  fbe_parity_generate_rekey()
 ****************************************************************
 * @brief
 *  This function fills out a siots for the next rekey request.
 *
 * @param siots_p - current siots
 *
 * @return
 *  fbe_status_t FBE_STATUS_OK on success
 *            FBE_STATUS_GENERIC_FAILURE on unexpected generate error.
 *
 * @author
 *  10/16/2013 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_parity_generate_rekey(fbe_raid_siots_t * const siots_p)
{
    fbe_payload_block_operation_opcode_t opcode;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_block_count_t max_blocks_per_drive;

    fbe_raid_siots_get_opcode(siots_p, &opcode);

    fbe_parity_get_physical_geometry(raid_geometry_p, siots_p->start_lba, &siots_p->geo);

    if(RAID_COND( 0!= (siots_p->geo.start_offset_rel_parity_stripe +
                       siots_p->geo.blocks_remaining_in_parity) % 
                       fbe_raid_siots_get_blocks_per_element(siots_p)))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: SIOTS not aligned with element size, start offset: %llx\n",
                             (unsigned long long)siots_p->geo.start_offset_rel_parity_stripe);
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "block remaining in parity: %llu, element size: %d\n",
                             (unsigned long long)siots_p->geo.blocks_remaining_in_parity,
                             fbe_raid_siots_get_blocks_per_element(siots_p));
        return FBE_STATUS_GENERIC_FAILURE;
    }

    siots_p->parity_pos = fbe_raid_siots_get_parity_pos(siots_p);
    if (RAID_COND(siots_p->parity_pos == FBE_RAID_INVALID_PARITY_POSITION))
    {
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: siots_p->parity_pos 0x%x == FBE_RAID_INVALID_PARITY_POSITION\n",
                             siots_p->parity_pos);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Limit the request to this parity stripe.
     */
    //siots_p->xfer_count = FBE_MIN(siots_p->xfer_count,
    //                              (fbe_u32_t)siots_p->geo.blocks_remaining_in_parity);

    fbe_raid_geometry_get_max_blocks_per_drive(raid_geometry_p, &max_blocks_per_drive);

    siots_p->xfer_count = FBE_MIN(siots_p->xfer_count, max_blocks_per_drive);

    /* Setup verify specific values in siots.
     */
    fbe_raid_siots_transition(siots_p, fbe_parity_rekey_state0);
    siots_p->algorithm = R5_REKEY;
    siots_p->retry_count = FBE_RAID_DEFAULT_RETRY_COUNT;

    siots_p->parity_start = siots_p->start_lba,
    siots_p->parity_count = siots_p->xfer_count;

    /* We only rekey one drive at a time for now.
     */
    siots_p->data_disks = 1;

    /* The drive we rekey is passed in to us via the iots.
     */
    siots_p->start_pos = 0;

    /* Adjust the iots to account for this request
     */
    fbe_raid_iots_dec_blocks(fbe_raid_siots_get_iots(siots_p), siots_p->xfer_count);

    /* Drive operations are just the width.
     */
    siots_p->drive_operations = fbe_raid_siots_get_width(siots_p);
    return FBE_STATUS_OK;
}
/*****************************************
 * end fbe_parity_generate_rekey()
 *****************************************/

/****************************************************************
 *  fbe_parity_generate_write_log_hdr_rd()
 ****************************************************************
 * @brief
 *  This function fills out a siots for the write_log header read
 *  operation.
 *
 * @param siots_p - current siots
 *
 * @return
 * FBE_STATUS_OK if the siots was initialized, error code otherwise.
 *
 * @author
 *  5/23/2012 Created. Vamsi V
 ****************************************************************/
fbe_status_t fbe_parity_generate_write_log_hdr_rd(fbe_raid_siots_t * const siots_p)
{
    fbe_status_t status;
    fbe_raid_iots_t *iots_p              = fbe_raid_siots_get_iots(siots_p);
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);

    /* setup the siots
     */
    status = fbe_parity_get_lun_geometry(raid_geometry_p, siots_p->start_lba, &siots_p->geo);

    if (RAID_COND(status != FBE_STATUS_OK))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: get geometry status 0x%x, siots_p:0x%p\n", status, siots_p);
        return status;
    }

    /* set up the dead positions
     */
    status = fbe_parity_siots_init_dead_position(siots_p, fbe_raid_siots_get_width(siots_p), NULL);

    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: failed to check for degraded state: status = 0x%x, siots_p = 0x%p\n",
                             status, siots_p);
        return status;
    }

    siots_p->parity_pos = fbe_raid_siots_get_parity_pos(siots_p);

    if (RAID_COND(siots_p->parity_pos == FBE_RAID_INVALID_PARITY_POSITION))
    {
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: siots_p->parity_pos 0x%x == FBE_RAID_INVALID_PARITY_POSITION\n",
                             siots_p->parity_pos);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_raid_siots_transition(siots_p, fbe_parity_write_log_flush_state000);

    siots_p->algorithm    = RG_WRITE_LOG_HDR_RD;
    siots_p->start_pos    = fbe_raid_siots_get_start_pos(siots_p);
    siots_p->retry_count  = FBE_RAID_DEFAULT_RETRY_COUNT;
    siots_p->parity_start = siots_p->start_lba,
    siots_p->parity_count = siots_p->xfer_count;
    siots_p->data_disks   = fbe_raid_siots_get_width(siots_p) - fbe_raid_siots_parity_num_positions(siots_p);

    fbe_raid_iots_get_journal_slot_id(iots_p,&siots_p->journal_slot_id);

    fbe_raid_iots_dec_blocks(fbe_raid_siots_get_iots(siots_p), siots_p->xfer_count); 

    return FBE_STATUS_OK;
}
/*****************************************
 * end fbe_parity_generate_write_log_hdr_rd()
 *****************************************/

/****************************************************************
 *  fbe_parity_generate_journal_flush()
 ****************************************************************
 * @brief
 *  This function fills out a siots for the next journal flush
 *  operation.
 *
 * @param siots_p - current siots
 *
 * @return
 * FBE_STATUS_OK if the siots was initialized, error code otherwise.
 *
 * @author
 *  1/17/2011 Created. Daniel Cummins
 ****************************************************************/
fbe_status_t fbe_parity_generate_journal_flush(fbe_raid_siots_t * const siots_p)
{
    fbe_status_t status;
    fbe_raid_iots_t *iots_p              = fbe_raid_siots_get_iots(siots_p);
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);

    /* setup the siots
     */
    status = fbe_parity_get_lun_geometry(raid_geometry_p, siots_p->start_lba, &siots_p->geo);

    if (RAID_COND(status != FBE_STATUS_OK))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: get geometry status 0x%x, siots_p %p\n", status, siots_p);
        return status;
    }

    /* set up the dead positions
     */
    status = fbe_parity_siots_init_dead_position(siots_p, fbe_raid_siots_get_width(siots_p), NULL);

    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: failed to check for degraded state: status = 0x%x, siots_p = 0x%p\n",
                             status, siots_p);
        return status;
    }

    siots_p->parity_pos = fbe_raid_siots_get_parity_pos(siots_p);

    if (RAID_COND(siots_p->parity_pos == FBE_RAID_INVALID_PARITY_POSITION))
    {
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: siots_p->parity_pos 0x%x == FBE_RAID_INVALID_PARITY_POSITION\n",
                             siots_p->parity_pos);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_raid_siots_transition(siots_p, fbe_parity_write_log_flush_state0000);

    siots_p->algorithm    = RG_FLUSH_JOURNAL;
    siots_p->start_pos    = fbe_raid_siots_get_start_pos(siots_p);
    siots_p->retry_count  = FBE_RAID_DEFAULT_RETRY_COUNT;
    siots_p->parity_start = siots_p->start_lba,
    siots_p->parity_count = siots_p->xfer_count;
    siots_p->data_disks   = fbe_raid_siots_get_width(siots_p) - fbe_raid_siots_parity_num_positions(siots_p);

    fbe_raid_iots_get_journal_slot_id(iots_p,&siots_p->journal_slot_id);

    fbe_raid_iots_dec_blocks(fbe_raid_siots_get_iots(siots_p), siots_p->xfer_count); 

    return FBE_STATUS_OK;
}
/*****************************************
 * end fbe_parity_generate_journal_flush()
 *****************************************/

/****************************************************************
 * fbe_parity_degraded_read_required()
 ****************************************************************
 * @brief
 *  Generate a degraded read.  We are degraded, determine
 *  if we should go to the degraded read state machine.
 *
 * @param siots_p - Current sub request.
 *
 * @return
 *  fbe_bool_t
 *
 * @author
 *  06/28/00 - Created. Rob Foley
 *
 ****************************************************************/

fbe_bool_t fbe_parity_degraded_read_required(fbe_raid_siots_t * const siots_p)
{
    fbe_bool_t deg_rd = FBE_FALSE;
    fbe_bool_t fbe_validate = FBE_FALSE;
    fbe_u16_t pos = siots_p->geo.start_index;
    fbe_u16_t disk_count;
    fbe_u8_t *position = &siots_p->geo.position[0];

    /* Dead position is relative to the array, and
     * thus cannot go past the array width.
     * It is either invalid or within the array width.
     */
    fbe_validate = ((fbe_raid_siots_dead_pos_get(siots_p, FBE_RAID_FIRST_DEAD_POS) == 
                     FBE_RAID_INVALID_DEAD_POS ) ||
                    (fbe_raid_siots_dead_pos_get(siots_p, FBE_RAID_FIRST_DEAD_POS) <
                     fbe_raid_siots_get_width(siots_p)));

    if(RAID_COND(!fbe_validate))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: iIncorrect dead position: %d\n",
                             fbe_raid_siots_dead_pos_get(siots_p, FBE_RAID_FIRST_DEAD_POS));
        return FBE_FALSE;
    }

    for (disk_count = 0; disk_count < siots_p->data_disks; disk_count++)
    {
        if (position[pos] == siots_p->parity_pos)
        {
            /* Skip over the parity drives and wrap around
             * in case we go beyond the width.
             */
            pos = (pos + fbe_raid_siots_parity_num_positions(siots_p)) % 
                     fbe_raid_siots_get_width(siots_p);
        }

        if ( ( position[pos] == 
               fbe_raid_siots_dead_pos_get(siots_p, FBE_RAID_FIRST_DEAD_POS) ) ||
             ( (fbe_raid_siots_parity_num_positions(siots_p) > 1 ) &&
               (position[pos] == 
                fbe_raid_siots_dead_pos_get(siots_p, FBE_RAID_SECOND_DEAD_POS)) ) )
        {
            /* Yes we hit a degraded position with this I/O.
             * We should do a degraded read.
             */
            deg_rd = FBE_TRUE;
            break;
        }

        /* Increment and wrap the position if necessary.
         */
        pos = (pos + 1) % fbe_raid_siots_get_width(siots_p);
    }

    return deg_rd;
}
/* fbe_parity_degraded_read_required() */

/****************************************************************
 * fbe_parity_generate_zero()
 ****************************************************************
 * @brief
 *  Generate a parity zero request.
 *  
 * @param siots_p - Current sub request. 
 *
 * @return fbe_status_t
 *
 ****************************************************************/
static fbe_status_t fbe_parity_generate_zero(fbe_raid_siots_t * const siots_p)
{
    fbe_status_t status;
    fbe_u16_t array_width;
    fbe_lba_t start_lba;
    fbe_lba_t end_lba;
    fbe_block_count_t blocks;
    fbe_block_count_t stripes;
    fbe_u32_t blocks_per_element;
    fbe_u32_t blocks_per_stripe;
    fbe_u32_t parity_drives = fbe_raid_siots_parity_num_positions(siots_p);
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_payload_block_operation_opcode_t opcode;

    fbe_raid_siots_get_opcode(siots_p, &opcode);

    /* Fetch the geometry information first.
     */
    status = fbe_parity_get_lun_geometry(raid_geometry_p, siots_p->start_lba, &siots_p->geo);

    if (status != FBE_STATUS_OK)
    {
        /* There was an invalid parameter in the siots, return error now.
         */
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: get lun geometry failed: status = 0x%x, siots_p = 0x%p\n",
                             status, siots_p);
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_unexpected_error);
        return FBE_STATUS_GENERIC_FAILURE; 
    }

    siots_p->parity_pos = fbe_raid_siots_get_parity_pos(siots_p);
    if (RAID_COND(siots_p->parity_pos == FBE_RAID_INVALID_PARITY_POSITION))
    {
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: siots_p->parity_pos 0x%x == FBE_RAID_INVALID_PARITY_POSITION: "
                             "siots_p = 0x%p\n",
                             siots_p->parity_pos, siots_p);
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_unexpected_error);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    siots_p->start_pos = fbe_raid_siots_get_start_pos(siots_p);

    /* Step 1: Setup and calculate local variables.
     */
    blocks = siots_p->xfer_count;

    start_lba = siots_p->start_lba;
    end_lba = start_lba + blocks - 1;
    array_width = fbe_raid_siots_get_width(siots_p);
    blocks_per_element = fbe_raid_siots_get_blocks_per_element(siots_p);
    if (RAID_COND(blocks_per_element == 0))
    {
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: blocks_per_element == 0 : siots_p = 0x%p\n", siots_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    blocks_per_stripe = blocks_per_element * (array_width - parity_drives);

    /* Count the number of uninterrupted stripes.
     */
    stripes = siots_p->xfer_count / blocks_per_stripe;
    
    /* If the beginning is not aligned, or
     * If the end is not aligned, and there is less than a full stripe, or
     * If we are not aligned to the optimal block size, use the regular write path 
     * since pre-reads are needed. 
     */
    if ((siots_p->start_lba % blocks_per_stripe) != 0 ||
        ( ((siots_p->start_lba + siots_p->xfer_count) % blocks_per_stripe) != 0 &&
          stripes == 0))
    { 
        /* For unmark_zero we should be aligned.
         */
        if (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_UNMARK_ZERO)
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "parity: opcode: 0x%x generate not aligned.\n",
                                 opcode);
            return(FBE_STATUS_GENERIC_FAILURE);
        }
        /* Perform a normal write.
         * fbe_parity_generate_write() will cut us off at a stripe boundary.
         */

        /* We use the host transfer functionality, but
         * in the host transfer stage, we do zeroing.
         */
        status = fbe_parity_generate_write(siots_p);
        if (status != FBE_STATUS_OK)
        {
            /* There was an invalid parameter in the siots, return error now.
             */
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "parity: zero generate write failed : status 0x%x, siots_p = 0x%p\n",status, siots_p);
            fbe_raid_siots_transition(siots_p, fbe_raid_siots_unexpected_error);
            return FBE_STATUS_GENERIC_FAILURE; 
        }
        fbe_raid_siots_log_traffic(siots_p, "r5 gen zero use gen wr ");
    }
    else
    {
        fbe_raid_extent_t parity_range[2];

        /* We should only be here if we are completly aligned at the beginning
         */
        if (RAID_COND((siots_p->start_lba % blocks_per_stripe) != 0 ))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "parity: unaligned start lba to beginning: %llx, "
                                 "blocks per stripe = 0x%x, siots_p =0x%p\n",
                                 (unsigned long long)siots_p->start_lba, blocks_per_stripe, 
                                 siots_p);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* We should only be here if we have at least a full stripe.
         */
        if (RAID_COND((siots_p->xfer_count / blocks_per_stripe) == 0))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "parity: xfer count is less than a full stripe: %llu, siots_p = 0x%p\n",
                                 (unsigned long long)siots_p->xfer_count, siots_p);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        if ( ((siots_p->start_lba + siots_p->xfer_count) % blocks_per_stripe) != 0 )
        {
            /* The end is unaligned. Just cut the end at a stripe boundary.
             */
            siots_p->xfer_count -=
                (fbe_u32_t)((siots_p->start_lba + siots_p->xfer_count) % blocks_per_stripe);
        }
        
        /* Use the zeroing state machine, since we're aligned.
         */
        fbe_raid_siots_transition(siots_p, fbe_parity_zero_state0);
        siots_p->data_disks = fbe_raid_siots_get_width(siots_p) - parity_drives;
        siots_p->drive_operations = fbe_raid_siots_get_width(siots_p);
        siots_p->algorithm = RG_ZERO;

        /* Calculate the parity range of this request
         */
        fbe_raid_get_stripe_range(siots_p->start_lba,
                                  siots_p->xfer_count,
                                  fbe_raid_siots_get_blocks_per_element(siots_p),
                                  siots_p->data_disks,
                                  FBE_RAID_MAX_PARITY_EXTENTS,
                                  parity_range);

        /* Make sure the parity range is continuous.
         */
        if (RAID_COND((parity_range[1].start_lba != 0) || (parity_range[1].size != 0)))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "parity: parity range is non-contiguous: parity_range[1].start_lba = \n");
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "0x%llx, parity_range[1].size = 0x%llx, siots_p = 0x%p\n",
                                 (unsigned long long)parity_range[1].start_lba, (unsigned long long)parity_range[1].size, siots_p);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* Fill out the parity range from the
         * values returned by fbe_raid_get_stripe_range
         */
        siots_p->parity_start = parity_range[0].start_lba;
        siots_p->parity_count = siots_p->xfer_count / siots_p->data_disks;
        
        /* Adjust the IOTS to account for this request
         */
        fbe_raid_iots_dec_blocks(fbe_raid_siots_get_iots(siots_p), siots_p->xfer_count);

        if (RAID_COND(siots_p->parity_count > siots_p->xfer_count))
        {
            fbe_raid_siots_trace(siots_p,
                                 FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "parity: siots_p->parity_count 0x%llx > siots_p->xfer_count 0x%llx :" 
                                 "siots_p = 0x%p\n",
                                 (unsigned long long)siots_p->parity_count,
                                 (unsigned long long)siots_p->xfer_count, 
                                 siots_p);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* For Rebuild Logging, now determine if a degraded write is required.
         * We intentionally ignore the return status.  If we are spanning the
         * checkpoint, we still always write non-degraded.
         */
        status = fbe_parity_check_for_degraded(siots_p);

        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "parity: failed to check for degraded status: status = 0x%x, siots_p =0x%p\n",
                                 status, siots_p);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        fbe_raid_siots_log_traffic(siots_p, "r5 gen zero after fbe_parity_check_for_degraded");

    } /* End we are aligned. (Optimized case)*/
    return FBE_STATUS_OK;
}
/*****************************************
 * fbe_parity_generate_zero()
 *****************************************/

/****************************************************************
 * fbe_parity_generate_check_zeroed()
 ****************************************************************
 * @brief
 *  Generate a parity check zero request.
 *  
 * @param siots_p - Current sub request. 
 *
 * @return fbe_status_t
 *
 ****************************************************************/
static fbe_status_t fbe_parity_generate_check_zeroed(fbe_raid_siots_t * const siots_p)
{
    fbe_status_t status;
    fbe_u32_t parity_drives = fbe_raid_siots_parity_num_positions(siots_p);
    fbe_payload_block_operation_opcode_t opcode;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_u32_t first_dead_pos;
    fbe_u32_t second_dead_pos;
    fbe_raid_iots_t *iots_p = fbe_raid_siots_get_iots(siots_p);
    fbe_payload_block_operation_opcode_t iots_opcode;
    fbe_raid_siots_get_opcode(siots_p, &opcode);

    fbe_parity_get_physical_geometry(raid_geometry_p, siots_p->start_lba, &siots_p->geo);

    if(RAID_COND( 0!= (siots_p->geo.start_offset_rel_parity_stripe + 
                       siots_p->geo.blocks_remaining_in_parity) % 
                       (fbe_raid_siots_get_blocks_per_element(siots_p))))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity:  SIOTS not aligned with element size, start offset: %llx"
                             "block remaining in parity: %llu, element size %d\n",
                             (unsigned long long)siots_p->geo.start_offset_rel_parity_stripe,
                             (unsigned long long)siots_p->geo.blocks_remaining_in_parity,
                             fbe_raid_siots_get_blocks_per_element(siots_p));
        return FBE_STATUS_GENERIC_FAILURE;
    }

    siots_p->parity_pos = fbe_raid_siots_get_parity_pos(siots_p);
    if (RAID_COND(siots_p->parity_pos == FBE_RAID_INVALID_PARITY_POSITION))
    {
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: siots_p->parity_pos 0x%x == FBE_RAID_INVALID_PARITY_POSITION\n",
                             siots_p->parity_pos);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    siots_p->start_pos = fbe_raid_siots_get_start_pos(siots_p);
    siots_p->data_disks = fbe_raid_siots_get_width(siots_p) - parity_drives;

    /* Update the info in fbe_raid_siots_t.
     */
    siots_p->algorithm = RG_CHECK_ZEROED;
    siots_p->retry_count = FBE_RAID_DEFAULT_RETRY_COUNT;

    /* See if we are degraded.
     */
    status = fbe_parity_check_degraded_phys(siots_p);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: failed to check degraded : status = 0x%x, siots_p = 0x%p\n", 
                             status, siots_p);
        return  status;
    }
    /* We should always have a dead position if this is a rebuild
     * which is issuing the request. 
     */
    fbe_raid_iots_get_opcode(iots_p, &iots_opcode);
    if (iots_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_REBUILD)
    {
    first_dead_pos = fbe_raid_siots_dead_pos_get(siots_p, FBE_RAID_FIRST_DEAD_POS);
    if (RAID_COND(first_dead_pos >= fbe_raid_siots_get_width(siots_p)))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "parity: invalid 1st rebuild position: %d, width %d\n",
                             first_dead_pos,
                             fbe_raid_siots_get_width(siots_p));
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* We can optionally have a 2nd dead pos on R6.
     */
    second_dead_pos = fbe_raid_siots_dead_pos_get(siots_p, FBE_RAID_SECOND_DEAD_POS);
    if (RAID_COND((second_dead_pos != FBE_RAID_INVALID_DEAD_POS) &&
                  ((second_dead_pos >= fbe_raid_siots_get_width(siots_p)) ||
                   (!fbe_raid_geometry_is_raid6(raid_geometry_p)))))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "parity: invalid 2nd rebuild position: %d, width %d\n",
                             second_dead_pos,
                             fbe_raid_siots_get_width(siots_p));
        return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    fbe_raid_siots_transition(siots_p, fbe_raid_check_zeroed_state0);

    /* Setup the parity start and parity count.
     */
    siots_p->parity_start = siots_p->start_lba;
    siots_p->parity_count = siots_p->xfer_count;
    siots_p->geo.logical_parity_count = siots_p->parity_count;

    /* Adjust the iots to account for this request
     */
    fbe_raid_iots_dec_blocks(fbe_raid_siots_get_iots(siots_p), siots_p->xfer_count);

    /* Drive operations are just the width.
     */
    siots_p->drive_operations = fbe_raid_siots_get_width(siots_p);
    return FBE_STATUS_OK;
}
/*****************************************
 * fbe_parity_generate_check_zeroed()
 *****************************************/
/****************************************************************
 * fbe_parity_degraded_recovery_allowed()
 ****************************************************************
 * @brief
 *  Return FBE_TRUE or FBE_FALSE depending on if degraded recovery
 *  will be allowed.
 *
 * @param siots_p - Current sub request.
 *
 * @return
 *  fbe_bool_t  FBE_TRUE - Degraded Recovery is allowed.
 *              FBE_FALSE - Not allowed.
 *
 * @author
 *  03/29/06 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_bool_t fbe_parity_degraded_recovery_allowed( fbe_raid_siots_t *siots_p )
{
    fbe_bool_t b_status = FBE_FALSE;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    
    /* First check if degraded recovery flag is set.
     */
    if (fbe_raid_is_option_flag_set(raid_geometry_p, FBE_RAID_OPTION_FLAG_DEGRADED_RECOVERY))
    {
        fbe_status_t fbe_status = FBE_STATUS_OK;
        fbe_raid_position_t saved_dead_pos_0 = fbe_raid_siots_dead_pos_get(siots_p, FBE_RAID_FIRST_DEAD_POS);
        fbe_raid_position_t saved_dead_pos_1 = fbe_raid_siots_dead_pos_get(siots_p, FBE_RAID_SECOND_DEAD_POS);
        
        /* Get the rebuild offsets for this siots.
         * This function sets the dead positions in the siots as a side effect.
         */
        fbe_lba_t degraded_offset[FBE_RAID_MAX_DEGRADED_POSITIONS];
        fbe_parity_siots_init_dead_position(siots_p, fbe_raid_siots_get_width(siots_p), degraded_offset);

        /* We will not allow degraded recovery to continue if
         * we are R5/R3 and the degraded offset is not zero.
         * OR if we are R6 and either the first or second degraded offset
         * is not zero.
         *
         * We don't allow degraded recovery when the checkpoint is not zero, 
         * since this means we are rebuilding.  We normally only allow
         * generation of completely degraded or completely non-degraded I/Os.
         *
         * If we allowed a degraded I/O to generate non-degraded during a 
         * rebuild, then would be able to see some strange spanning the chkpt 
         * cases that we do not support.
         */
        if ( ( fbe_raid_siots_dead_pos_get(siots_p, FBE_RAID_FIRST_DEAD_POS) != FBE_RAID_INVALID_DEAD_POS &&
               degraded_offset[FBE_RAID_FIRST_DEAD_POS] == 0 ) &&
             ( fbe_raid_siots_parity_num_positions(siots_p) == 1 ||
               ( fbe_raid_siots_dead_pos_get(siots_p, FBE_RAID_SECOND_DEAD_POS) != FBE_RAID_INVALID_DEAD_POS &&
                 degraded_offset[FBE_RAID_SECOND_DEAD_POS] == 0 ) ) )
        {
            /* Allow degraded recovery.
             * Set dead position to FBE_RAID_INVALID_DEAD_POS so we actually do the I/O degraded.
             */
            b_status = FBE_TRUE;
            fbe_status = fbe_raid_siots_dead_pos_set(siots_p, FBE_RAID_FIRST_DEAD_POS, FBE_RAID_INVALID_DEAD_POS);
            if (RAID_COND(fbe_status != FBE_STATUS_OK))
            {
                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                     "parity: failed to set dead position: status = 0x%x, siots_p = 0x%p\n",
                                     fbe_status, siots_p);
                return FBE_FALSE;
            }
            fbe_status = fbe_raid_siots_dead_pos_set(siots_p, FBE_RAID_SECOND_DEAD_POS, FBE_RAID_INVALID_DEAD_POS);
            if (RAID_COND(fbe_status != FBE_STATUS_OK))
            {
                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                     "parity: failed to set dead position: status = 0x%x, siots_p = 0x%p\n",
                                     fbe_status, siots_p);
                return FBE_FALSE;
            }
        }
        else
        {
            /* Given these circumstances, we will not continue degraded.
             * Restore the siots dead positions.
             */
            fbe_status = fbe_raid_siots_dead_pos_set(siots_p, FBE_RAID_FIRST_DEAD_POS, FBE_RAID_INVALID_DEAD_POS);
            if (RAID_COND(fbe_status != FBE_STATUS_OK))
            {
                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                     "parity: failed to set dead position: status = 0x%x, siots_p = 0x%p\n",
                                     fbe_status, siots_p);
                return FBE_FALSE;
            }
            fbe_status = fbe_raid_siots_dead_pos_set(siots_p, FBE_RAID_SECOND_DEAD_POS, FBE_RAID_INVALID_DEAD_POS);
            if (RAID_COND(fbe_status != FBE_STATUS_OK))
            {
                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                     "parity: failed to set dead position: status = 0x%x, siots_p = 0x%p\n",
                                     fbe_status, siots_p);
                return FBE_FALSE;
            }
            fbe_status = fbe_raid_siots_dead_pos_set(siots_p, FBE_RAID_FIRST_DEAD_POS, saved_dead_pos_0);
            if (RAID_COND(fbe_status != FBE_STATUS_OK))
            {
                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                     "parity: failed to set dead position: status = 0x%x, siots_p = 0x%p\n",
                                     fbe_status, siots_p);
                return FBE_FALSE;
            }
            fbe_status = fbe_raid_siots_dead_pos_set(siots_p, FBE_RAID_SECOND_DEAD_POS, saved_dead_pos_1);
            if (RAID_COND(fbe_status != FBE_STATUS_OK))
            {
                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                     "parity: failed to set dead position: status = 0x%x, siots_p = 0x%p\n",
                                     fbe_status, siots_p);
                return FBE_FALSE;
            }
        }
    }
    return b_status;
}
/* end fbe_parity_degraded_recovery_allowed() */
/****************************************************************
 * fbe_parity_check_degraded_phys()
 ****************************************************************
 * @brief
 *  Setup the dead positions by setting up this siots which is
 *  being issued to a physical range..
 *  
 * @param siots_p - Current sub request.
 * @param b_generate - FBE_TRUE if currently generating
 *                    FBE_FALSE otherwise.  We can't change the size
 *                    of the siots if we are not generating.
 * @param degraded_offset_p - Current degraded offsets.
 *
 * @return
 *  fbe_status_t
 *
 ****************************************************************/
fbe_status_t fbe_parity_check_degraded_phys( fbe_raid_siots_t *siots_p )
{    
    fbe_status_t status;

    /* Refresh our information about degraded offsets and dead positions
     */
    status = fbe_parity_siots_init_dead_position(siots_p, fbe_raid_siots_get_width(siots_p), NULL);
    
    return status;
}
/******************************
 * end fbe_parity_check_degraded_phys()
 ******************************/

static fbe_status_t fbe_parity_count_aligned_rcw_operations(fbe_raid_siots_t *const siots_p,
                                                            fbe_u32_t *r5_rcw_operations_p) 
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t rcw_operations = 0;
    fbe_block_count_t  write_blocks = 0; 
    fbe_lba_t write_start_lba = 0;
    fbe_lba_t write_end_lba = 0;
    fbe_u32_t data_pos = 0;
    fbe_u32_t  array_pos;         /* Actual array position of access */

    fbe_block_count_t r1_blkcnt[FBE_RAID_MAX_DISK_ARRAY_WIDTH] = {0};    /* block counts for pre-reads, */
    fbe_block_count_t r2_blkcnt[FBE_RAID_MAX_DISK_ARRAY_WIDTH] = {0};    /* distributed across the FRUs */


    fbe_u8_t *position = siots_p->geo.position;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);

    /* Check number of parity drives that are supported. R5=1, R6=2
     */
    fbe_u32_t parity_drives = fbe_raid_siots_parity_num_positions(siots_p);
    fbe_lba_t logical_parity_start = siots_p->geo.logical_parity_start;
    fbe_lba_t parity_range_offset = fbe_raid_siots_get_parity_start_offset(siots_p);

    fbe_u32_t width = fbe_raid_siots_get_width(siots_p);

    /* Leverage MR3 calculations to describe what
     * portions of stripe(s) are *not* being accessed.
     */
    status = fbe_raid_geometry_calc_preread_blocks(siots_p->start_lba,
                                                   &siots_p->geo,
                                                   siots_p->xfer_count,
                                                   (fbe_u32_t)fbe_raid_siots_get_blocks_per_element(siots_p),
                                                   fbe_raid_siots_get_width(siots_p) - parity_drives,
                                                   siots_p->parity_start,
                                                   siots_p->parity_count,
                                                   r1_blkcnt,
                                                   r2_blkcnt);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        /*Split trace to two lines*/
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "parity:fail calc pre-read blocks:status=0x%x>\n",
                             status);
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "siots=0x%p,r1_blkcnt = 0x%p,r2_blkcnt=0x%p<\n",
                             siots_p, r1_blkcnt, r2_blkcnt);
        return status;
    }

    for (data_pos = 0; data_pos < (width - parity_drives); data_pos++)
    {

        /* The pre-reads should NOT exceed the parity range.  */
        if ((r1_blkcnt[data_pos] + r2_blkcnt[data_pos]) > siots_p->parity_count)
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "pre-read range %llu %llu exceeds parity range %llu\n", 
                                 (unsigned long long)r1_blkcnt[data_pos],
				 (unsigned long long)r2_blkcnt[data_pos],
				 (unsigned long long)siots_p->parity_count);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* Should not be processing parity position here */
        if (position[data_pos] == siots_p->parity_pos)
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "parity: position %d equal to parity\n", position[data_pos]);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        array_pos = position[data_pos];
        write_blocks = siots_p->parity_count - r1_blkcnt[data_pos] - r2_blkcnt[data_pos];
        if (write_blocks)
        {
            rcw_operations++;
            write_start_lba = logical_parity_start + parity_range_offset + r1_blkcnt[data_pos];
            write_end_lba = write_start_lba + write_blocks;

            /* An extra read is required to align the write 
             */
            if (r1_blkcnt[data_pos] == 0 &&
                fbe_raid_geometry_position_needs_alignment(raid_geometry_p, array_pos) &&
                fbe_raid_geometry_io_needs_alignment(raid_geometry_p, write_start_lba, 0))

            {
                rcw_operations++;
            }

            /* An extra read is required to align the write 
             */
            if (r2_blkcnt[data_pos] == 0 &&
                fbe_raid_geometry_position_needs_alignment(raid_geometry_p, array_pos) &&
                fbe_raid_geometry_io_needs_alignment(raid_geometry_p, write_end_lba, 0))

            {
                rcw_operations++;
            }
        }

        if (r1_blkcnt[data_pos] > 0)
        {
            /* Check to see if we're pre-reading the entire 
             * locked area of the disk. Rather than perform a second
             * operation, just combine the two.
             */
            if (r2_blkcnt[data_pos] == siots_p->parity_count - r1_blkcnt[data_pos])
            {
                r2_blkcnt[data_pos] = 0;
            }

            rcw_operations++;
        }
        if (r2_blkcnt[data_pos] > 0)
        {
            rcw_operations++;
        }

    }

    /* Account for the read and the write of parity
     */
    rcw_operations += 2; 

    if (parity_drives == 2)
    {
        rcw_operations++;
        write_start_lba = logical_parity_start + parity_range_offset;
        write_blocks= write_start_lba + siots_p->parity_count;
        array_pos = position[fbe_raid_siots_get_width(siots_p) - 1];
        if (fbe_raid_geometry_position_needs_alignment(raid_geometry_p, array_pos) &&
            fbe_raid_geometry_io_needs_alignment(raid_geometry_p, write_start_lba, write_blocks))
        {
            rcw_operations++; 
        }
    }

    *r5_rcw_operations_p = rcw_operations;
    return status;

}
/*!**************************************************************
 * fbe_parity_count_operations()
 ****************************************************************
 * @brief
 *  Count how many operations is required for 468, MR3 or RCW.
 *  
 * @param siots_p - Current operation.
 * @param r5_468_operations_p - Count of ops needed for 468.
 * @param r5_mr3_operations_p - Count of ops needed for MR3.
 * @param r5_rcw_operations_p - Count of ops needed for RCW
 *
 * @return - fbe_status_t.  
 *
 * @author
 *  2/3/2009 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_parity_count_operations(fbe_raid_siots_t *const siots_p, 
                                                fbe_u32_t *r5_468_operations_p, 
                                                fbe_u32_t *r5_mr3_operations_p,
                                                fbe_u32_t *r5_rcw_operations_p)
{   
    fbe_u16_t array_width        = fbe_raid_siots_get_width(siots_p);
    fbe_lba_t   start_lba        = siots_p->start_lba;
    fbe_u32_t parity_drives      = fbe_raid_siots_parity_num_positions(siots_p);
    fbe_u32_t blocks_per_element = fbe_raid_siots_get_blocks_per_element(siots_p);
    fbe_u32_t blocks_per_stripe  = blocks_per_element * (array_width - parity_drives);
    fbe_lba_t   end_lba;
    fbe_u32_t blocks_accessed_in_stripe;
    fbe_u16_t pre_read1_elements;
    fbe_u16_t pre_read2_elements;
    fbe_u16_t num_disks          = siots_p->data_disks;
    fbe_u16_t data_disks         = array_width - parity_drives;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);

    /* Calc  elements pre-read in first stripe
     * for an MR3.
     */
    pre_read1_elements = (fbe_u32_t)((start_lba % blocks_per_stripe) +
                              (blocks_per_element - 1)) / blocks_per_element;

    /* If there is only one data disk, then we do not need to 
     * pre-read from it. Decrement pre_read1, since the previous 
     * calculation includes a pre-read for the data disk.
     */
    if (num_disks == 1 && ((start_lba % blocks_per_element) != 0))
    {
        pre_read1_elements--;
    }

     /* An MR3 pre-read will access a max of data drives.
         * If this unit has small elements,
         * the element count might wrap around.
         */
    if (pre_read1_elements >= data_disks)
    {
        /* Normalize the elements in the pre-read.
         */
        pre_read1_elements = data_disks - parity_drives;
    }

    end_lba = start_lba + siots_p->xfer_count;

    /* If this I/O does not end on a stripe boundary, then
     * we need to calculate the number of  elements
     * pre-read in last stripe of an MR3.
     */
    if ((blocks_accessed_in_stripe = (fbe_u32_t)(end_lba % blocks_per_stripe)) > 0)
    {
        /* Calculate end pre-read for MR3 by:
         * total blocks to end of stripe / blocks_per_element
         */
        pre_read2_elements = ((blocks_per_stripe - blocks_accessed_in_stripe) +
                                  (blocks_per_element - 1)) / blocks_per_element;

        /* When we are writing to a partial element and
         * we are not aligned to the beginning of a stripe
         * then raid Driver chooses 468 over MR3 incorrectly
         * in some cases.(DIMS #124631)
         * 
         * If there is only one data disk, then we do not need to 
         * pre-read from it. Decrement pre_read2, since the previous 
         * calculation includes a pre-read for the data disk. 
         */
        if (num_disks == 1 && 
                 ((end_lba % blocks_per_element) != 0))
        {
            pre_read2_elements--;
        }


        /* An MR3 pre-read will access a max of data drives.
         * If this unit has small elements,
         * the element count might wrap around.
         */
        if (pre_read2_elements >= data_disks)
        {
            /* Normalize the elements in the pre-read.
             */
            pre_read2_elements = data_disks - 1;
        }

        if ((start_lba / blocks_per_stripe) < ((end_lba - 1) / blocks_per_stripe))
        {
            if (RAID_COND(pre_read2_elements <= data_disks - num_disks))
            {
                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                     "parityL invalid pre_read2_elements: %d, data_disks: %d, num_disks: %d\n",
                                     pre_read2_elements, data_disks, num_disks);
                return FBE_STATUS_GENERIC_FAILURE;
            }
            pre_read2_elements -= data_disks - num_disks;
        }
    }/*end if ((blocks_accessed_in_stripe = (fbe_u32_t).....))*/

    else
    {
        /* I/O ends on a stripe boundary,
         * no end pre-read.
         */
        pre_read2_elements = 0;
    }

    /**************************************************
     * Calculate total drive operations using both
     * algorithms.
     **************************************************/

    /***************************************************
     * 468 operation count is the amount to
     * read and write all affected data and parity.
     ***************************************************/
    *r5_468_operations_p = 2 * num_disks + (2 * parity_drives);

    /***************************************************
     * MR3 operation count is the sum of
     *     1) pre-reads = pre-read1 + pre-read2+
     *
     * and 2) write = data_disks + parity_disk
     ***************************************************/
    *r5_mr3_operations_p = parity_drives + data_disks + pre_read1_elements + pre_read2_elements;

    /***************************************************
     * RCW operation count is the sum of
     *     1) pre-reads = pre-read1 + pre-read2+ 1 parity_drive
     *
     * and 2) write = num_disks + parity_drives
     ***************************************************/
    if (fbe_raid_geometry_io_needs_alignment(raid_geometry_p, start_lba, end_lba))
    {

        fbe_parity_count_aligned_rcw_operations(siots_p, r5_rcw_operations_p);
    } 
    else 
    {
        *r5_rcw_operations_p = parity_drives + num_disks + pre_read1_elements + pre_read2_elements + 1; 
    }
    return status;
}
/**************************************
 * end fbe_parity_count_operations()
 **************************************/

/*!***************************************************************
 * fbe_parity_mr3_468_or_rcw()
 ****************************************************************
 * @brief
 *  Decide whether an R5/R6 write will use the 468 or MR3 or RCW  alogorithm.
 *  
 * @param siots_p - Current sub request.
 * @param blocks -  Number of blocks to transfer.  This may differ
 *                  from the xfer value in the siots,as the latter 
 *                  may not yet have been updated.
 *
 *  @return
 *          fbe_status_t
 *
 *  @author
 *   10/19/07 - Separated out from fbe_parity_generate_write() by HEW.
 *
 ****************************************************************/

static fbe_status_t fbe_parity_mr3_468_or_rcw(fbe_raid_siots_t * const siots_p)
{
    fbe_u32_t parity_drives = fbe_raid_siots_parity_num_positions(siots_p);
    fbe_u32_t r5_468_operations;
    fbe_u32_t r5_mr3_operations;
    fbe_u32_t r5_rcw_operations;
    fbe_u32_t r5_orig_mr3_ops = 0;
    fbe_status_t status;
    fbe_payload_block_operation_opcode_t opcode;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);

    /* Normally we will decide between MR3 and 468
     * by whichever requires the fewest disk accesses.
     *
     * Simply count how many operations are needed for 468 and mr3 and
     * also determine if we are aligned. 
     */
    status = fbe_parity_count_operations(siots_p, &r5_468_operations, &r5_mr3_operations, &r5_rcw_operations);
    if (status != FBE_STATUS_OK) { return status; }

    /* Save, since sometimes we clear it out below.
     */
    r5_orig_mr3_ops = r5_mr3_operations;

    if (fbe_raid_is_option_flag_set(raid_geometry_p, FBE_RAID_OPTION_FLAG_SMALL_WRITE_ONLY))
    {
        /* 8/20/03
         * for R3 large host/cache transfer, 
         * we can't force 468 any more, since we might be unable to get that much data buffer.
         */
        if ((((siots_p->parity_count * parity_drives) + 
              (2 * siots_p->xfer_count)) < FBE_RAID_MAX_BLKS_AVAILABLE))
        {
            r5_mr3_operations = 0xFFFF;
            r5_rcw_operations = 0xFFFF;
        }
    }
    else if (fbe_raid_is_option_flag_set(raid_geometry_p, FBE_RAID_OPTION_FLAG_LARGE_WRITE_ONLY))
    {
        r5_468_operations = 0xFFFF;
    }

    /***************************************************
     * Compare the number of disk accesses,
     * and choose the algorithm which
     * takes fewest disk operations.
     ***************************************************/
    /* A corrupt data operation is always forced down the 468 path.
     * No parity is involved, so only one of the write state machines 
     * needs to handle this special path.
     */
    fbe_raid_siots_get_opcode(siots_p, &opcode);
    if (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_CORRUPT_DATA)
    {
        siots_p->drive_operations = r5_468_operations;
        siots_p->algorithm = R5_CORRUPT_DATA;
    }
    else if (fbe_parity_mr3_is_aligned(siots_p)) 
    {
        siots_p->drive_operations = r5_orig_mr3_ops;
        siots_p->algorithm = R5_MR3;
    }
    else if (r5_468_operations < r5_rcw_operations)
    {
        siots_p->drive_operations = r5_468_operations;
        siots_p->algorithm = R5_468;
    } 
    else
    {
        siots_p->drive_operations = r5_rcw_operations;
        siots_p->algorithm = R5_RCW;
    }

    if (RAID_COND(siots_p->drive_operations == 0xFFFF))
    {
         fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                              "parity: invalid drive operations - 0xFFFF\n");
         return FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}
/*****************************************
 * end of fbe_parity_mr3_468_or_rcw()
 *****************************************/

/*!***************************************************************
 * fbe_parity_calc_parity_range()
 ****************************************************************
 * @brief
 *  Calculate parity size.
 *  
 * @param siots_p - Current sub request.
 * @param blocks - Size of request
 *
 *  @return
 *    fbe_status_t.
 *
 *  @author
 *   10/19/07 - Separated out from fbe_parity_generate_write() by HEW.
 *
 ****************************************************************/

static fbe_status_t fbe_parity_calc_parity_range(fbe_raid_siots_t * const siots_p)
{
    fbe_raid_extent_t parity_range[2];
    fbe_u16_t array_width = fbe_raid_siots_get_width(siots_p);
    fbe_u32_t parity_drives = fbe_raid_siots_parity_num_positions(siots_p);
    fbe_status_t status = FBE_STATUS_OK;

    /* Calculate the parity range of this request
     */
    fbe_raid_get_stripe_range(siots_p->start_lba,
                              siots_p->xfer_count,
                              fbe_raid_siots_get_blocks_per_element(siots_p),
                              array_width - parity_drives,
                              FBE_RAID_MAX_PARITY_EXTENTS,
                              parity_range);

    /* Make sure the parity range is continuous.
     */
    if (RAID_COND((parity_range[1].start_lba != 0) || (parity_range[1].size != 0)))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: parity range is not continuous, parity_range[1].start_lba/size: %llx, %llu\n",
                             (unsigned long long)parity_range[1].start_lba, (unsigned long long)parity_range[1].size);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Fill out the parity range from the
     * values returned by fbe_raid_get_stripe_range
     */
    siots_p->parity_start = parity_range[0].start_lba;
    siots_p->parity_count = parity_range[0].size;

    if (RAID_COND(siots_p->parity_count > siots_p->xfer_count))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "parity: ERROR: parity count > xfer_count, parity count/xfer count - %llu / %llu\n",
                             (unsigned long long)siots_p->parity_count, (unsigned long long)siots_p->xfer_count);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}
/*****************************************
 * end of fbe_parity_calc_parity_range()
 *****************************************/

/*!***************************************************************
 * fbe_parity_get_max_degraded_positions()
 ****************************************************************
 * @brief
 *  This function determines the maximum number of degraded postions
 *  allowed before the raid group is broken.
 *
 * @param raid_geometry_p - Pointer to raid geometry structure
 * @param max_degraded_p - Pointer to return value of max degraded postions
 *
 * @return
 *  None.
 *
 * @author
 *  03/31/06 - Rob Foley created.
 *
 ****************************************************************/

static __forceinline void fbe_parity_get_max_degraded_positions(fbe_raid_geometry_t *const raid_geometry_p,
                                                         fbe_u32_t *max_degraded_p)
{
    if (fbe_raid_geometry_is_raid6(raid_geometry_p))
    {
        *max_degraded_p = 2;
    }
    else
    {
        *max_degraded_p = 1;
    }
    return;
}

/****************************************************************
 * fbe_parity_siots_init_dead_position()
 ****************************************************************
 * @brief
 *  This function determines the rebuild fru(s) of an array,
 *  and the current rebuild checkpoint.
 *  This is used by R5 and R6, since this code
 *  handles two degraded positions.
 *
 * @param siots_p - current siots
 * @param array_width - array width of this request
 * @param degraded_offset_p - Pointer to array[2] of offsets.
 *
 * @return
 *  fbe_status_t
 *
 * @author
 *  03/31/06 - Rob Foley created.
 *
 ****************************************************************/

fbe_status_t fbe_parity_siots_init_dead_position(fbe_raid_siots_t * const siots_p, 
                                                 fbe_u32_t array_width,
                                                 fbe_lba_t *degraded_offset_p)
{
    fbe_u16_t pos;
    fbe_status_t status = FBE_STATUS_OK;
    /* Index of the current deg offset to fill in.
     */
    fbe_u16_t current_degraded_offset_index;
    fbe_raid_iots_t *iots_p = fbe_raid_siots_get_iots(siots_p);
    fbe_payload_block_operation_opcode_t opcode;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_u32_t max_degraded_positions;

    fbe_raid_siots_get_opcode(siots_p, &opcode);
    
    /* Init the degraded offset array.
     */
    for ( current_degraded_offset_index = 0;
          current_degraded_offset_index < FBE_RAID_MAX_DEGRADED_POSITIONS;
          current_degraded_offset_index++ )
    {
        /* Init the dead position and offset to invalid.
         * We set the degraded_off to the largest possible value since 
         * we know we are not degraded. It will insure the check below 
         * to see if we are in the degraded will prove false since there 
         * is no degraded area.
         */
        if (degraded_offset_p != NULL)
        {
            degraded_offset_p[current_degraded_offset_index] = FBE_RAID_INVALID_DEGRADED_OFFSET;
        }
        status = fbe_raid_siots_dead_pos_set( siots_p, current_degraded_offset_index, FBE_RAID_INVALID_DEAD_POS );
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "parity: failed to set dead position: status = 0x%x, siots_p = 0x%p\n",
                                 status, siots_p);
            return  status;
        }
    }
    /* Start with the first degraded offset,
     * we will fill them out in the order we find them.
     */
    current_degraded_offset_index = 0;

    fbe_parity_get_max_degraded_positions(raid_geometry_p, &max_degraded_positions);

    for ( pos = 0; pos < array_width; pos++ )
    {
        fbe_bool_t b_rebuild_logging = FBE_FALSE;
        fbe_bool_t b_logs_available = FBE_FALSE;

        /* We assume that if nr on demand is set, then we are completely degraded.
         */
        fbe_raid_iots_is_rebuild_logging(iots_p, pos, &b_rebuild_logging);

        fbe_raid_iots_rebuild_logs_available(iots_p, pos, &b_logs_available);
        
        if (b_rebuild_logging || b_logs_available)
        {
            /* We can only have up to RG_SIOTS_MAX_DEGRADED_POSITIONS(siots_p)
             * degraded drives.
             */
            if (current_degraded_offset_index >= max_degraded_positions)
            {
                fbe_raid_siots_trace(siots_p,
                                     FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                     "current degraded offset %d >= max_degraded_positions %d\n",
                                     current_degraded_offset_index, max_degraded_positions);
                return FBE_STATUS_GENERIC_FAILURE;
            }

            /* Set the correct degraded offset index to the current rebuild offset.
             */
            if (degraded_offset_p != NULL)
            {
                degraded_offset_p[current_degraded_offset_index] = 0;
            }

            /* Set the current dead position to the current position.
             */
            status = fbe_raid_siots_dead_pos_set( siots_p, current_degraded_offset_index, pos );
            if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
            {
                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                     "parity: failed to set dead position: status = 0x%x, siots_p = 0x%p\n",
                                     status, siots_p);
                return status;
            }

            /* Move to the next degraded offset, as we are done with this one.
             */
            current_degraded_offset_index++;
        }
    }

    return status;
}
/***************************************
 * End of fbe_parity_siots_init_dead_position()
 ***************************************/

/*!**************************************************************************
 * fbe_parity_generate_validate_siots()
 ****************************************************************************
 * @brief  This function calculates the number of sg lists needed.
 *
 * @param siots_p - Pointer to this I/O.
 * 
 * @return FBE_STATUS_OK on success, FBE_STATUS_GENERIC_FAILURE on failure.
 *
 **************************************************************************/
fbe_status_t fbe_parity_generate_validate_siots(fbe_raid_siots_t *siots_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u16_t width = fbe_raid_siots_get_width(siots_p);
    fbe_u32_t parity_drives = fbe_raid_siots_parity_num_positions(siots_p);

    if (siots_p->start_pos >= width)
    {
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: error start position: 0x%x width: 0x%x\n",
                             siots_p->start_pos, width);
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    if (siots_p->data_disks >= width)
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "parity: data_disks %d is greater than width. %d\n", 
                             siots_p->data_disks, width);
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    if (siots_p->data_disks == 0)
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "parity: data_disks is zero. %d\n", siots_p->data_disks);
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    if (siots_p->xfer_count == 0)
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "parity: xfer_count is unexpected. %llu\n", (unsigned long long)siots_p->xfer_count);
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    if (width < FBE_PARITY_MIN_WIDTH)
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "parity: width is unexpected. %d\n", width);
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    if (siots_p->data_disks >= FBE_RAID_MAX_DISK_ARRAY_WIDTH)
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "parity: data_disks is unexpected. %d\n", siots_p->data_disks);
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    if (siots_p->parity_pos >= FBE_RAID_MAX_DISK_ARRAY_WIDTH)
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "parity: parity_pos is unexpected. %d\n", siots_p->parity_pos);
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    if (siots_p->parity_count == 0)
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "parity: parity_count is zero\n");
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    /* The last extent position and the parity position should be identical.
     */
    if (siots_p->parity_pos != siots_p->geo.position[width - parity_drives])
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "parity: position in geo incorrect %d %d\n",
                             siots_p->parity_pos, 
                             siots_p->geo.position[width - parity_drives]);
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}
/**************************************
 * end fbe_parity_generate_validate_siots()
 **************************************/

/*!**************************************************************
 * fbe_parity_read_write_calculate()
 ****************************************************************
 * @brief
 *  Calculate some basic parameters like data disks and parity
 *  size.
 *
 * @param siots_p - current I/O.
 *
 * @return fbe_status_t
 *
 ****************************************************************/
fbe_status_t fbe_parity_read_write_calculate(fbe_raid_siots_t * siots_p)
{
    fbe_u32_t blocks_per_element = fbe_raid_siots_get_blocks_per_element(siots_p);
    fbe_u16_t array_width = fbe_raid_siots_get_width(siots_p);
    fbe_u32_t parity_drives = fbe_raid_siots_parity_num_positions(siots_p);
    fbe_block_count_t blocks = siots_p->xfer_count;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_payload_block_operation_opcode_t opcode;

    /* Count # of horizontal elements we will access.
     */
    siots_p->data_disks = (fbe_u32_t)((blocks + (siots_p->start_lba % blocks_per_element) +
                                       (blocks_per_element - 1)) / blocks_per_element);

    if ((siots_p->data_disks + parity_drives) > array_width)
    {
        /* For small element sizes the above calculation
         * will "wrap" around, so limit to data disks.
         */
        siots_p->data_disks = array_width - parity_drives;
    }

    /* Calculate the size of the parity region for this xfer 
     */
    status = fbe_parity_calc_parity_range(siots_p);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status)) { return status; }

    /* Only perform this for non-read ops.
     */
    fbe_raid_siots_get_opcode(siots_p, &opcode);
    if (fbe_payload_block_operation_opcode_is_media_modify(opcode))
    {
        status = fbe_parity_mr3_468_or_rcw(siots_p);
        if (status != FBE_STATUS_OK) { return status; }
    }

    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_parity_read_write_calculate()
 **************************************/
/*!**************************************************************
 * fbe_parity_limit_blocks()
 ****************************************************************
 * @brief
 *  Limit the number of blocks to either one less stripe,
 *  or element or block.
 *
 * @param siots_p - current I/O.
 * @param blocks_p - block count to return.
 *
 * @return fbe_status_t
 *
 ****************************************************************/

fbe_status_t fbe_parity_limit_blocks(fbe_raid_siots_t * siots_p,
                                     fbe_block_count_t *blocks_p)
{
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_block_count_t blocks = siots_p->xfer_count;
    fbe_u32_t blocks_per_element;
    fbe_u32_t array_width;
    fbe_u32_t parity_drives;
    fbe_block_count_t blocks_per_stripe;

    fbe_raid_geometry_get_element_size(raid_geometry_p, &blocks_per_element);
    fbe_raid_geometry_get_num_parity(raid_geometry_p, &parity_drives);
    fbe_raid_geometry_get_width(raid_geometry_p, &array_width);

    blocks_per_stripe = ((fbe_block_count_t)blocks_per_element) * (array_width - parity_drives);

    /* Need to make the SIOTS smaller, reduce by a stripe,
     * if possible.  Only the 1st case (the if) has been
     * seen so far, but the other cases need to covered in
     * they occur.
     */
    if ( blocks > blocks_per_stripe )
    {
        /* Reduce size by a stripe.
         */
        blocks -= blocks_per_stripe;
    }
    else if ( blocks > blocks_per_element )
    {
        /* Reduce size by an element.
         */
        blocks -= blocks_per_element;
    }
    else if ( blocks > 2 )
    {
        /* Cut the request in half, and make sure the result
         * is even.
         */
        blocks /= 2;
        if ( blocks & 1 )
        {
            blocks--;
        }
    }
    else
    {
        /* Only 1 or 2 blocks.  Unlikely we would need this, but just
         * in case.
         */
        blocks = 1;
    }

    /* Make sure we only have a single parity extent.
     */
    blocks = fbe_parity_siots_limit_to_one_parity_extent(siots_p, blocks_per_element, blocks);

    if (blocks == 0)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    *blocks_p = blocks;

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_parity_limit_blocks()
 ******************************************/

/*!**************************************************************
 * fbe_parity_get_max_read_write_blocks()
 ****************************************************************
 * @brief
 *  Determine the max block count for this write.
 *
 * @param siots_p - this I/O.
 * @param blocks_p - Number of blocks we can generate.
 *
 * @return fbe_status_t   
 *
 ****************************************************************/

fbe_status_t fbe_parity_get_max_read_write_blocks(fbe_raid_siots_t * siots_p,
                                                  fbe_block_count_t *blocks_p)
{
    fbe_status_t status;
    fbe_status_t return_status;
    fbe_block_count_t blocks = *blocks_p;
    
    /* Use standard method to determine page size.
     */
    fbe_raid_siots_set_optimal_page_size(siots_p, 
                                         siots_p->data_disks, 
                                         (siots_p->data_disks * siots_p->parity_count));

    /* We should optimize so that we only do the below code 
     * for cases where the I/O exceeds a certain max size. 
     * This way for relatively small I/O that we know we can do we will 
     * not incurr the overhead for the below. 
     */
    if (fbe_raid_siots_is_small_request(siots_p) == FBE_TRUE)
    {
        return FBE_STATUS_OK;
    }

    do
    {
        fbe_raid_fru_info_t read_info[FBE_RAID_MAX_DISK_ARRAY_WIDTH+1];
        fbe_raid_fru_info_t read2_info[FBE_RAID_MAX_DISK_ARRAY_WIDTH+1];
        fbe_raid_fru_info_t write_info[FBE_RAID_MAX_DISK_ARRAY_WIDTH+1];
        fbe_u16_t num_sgs[FBE_RAID_MAX_SG_TYPE];

        fbe_zero_memory(&num_sgs[0], sizeof(fbe_u16_t) * FBE_RAID_MAX_SG_TYPE);
        fbe_raid_fru_info_init_arrays(fbe_raid_siots_get_width(siots_p),
                                      &read_info[0],&write_info[0],&read2_info[0]);

        if ((siots_p->algorithm == R5_468) ||
            (siots_p->algorithm == R5_CORRUPT_DATA))
        {
            /* Next calculate the number of sgs of each type.
             */ 
            status = fbe_parity_468_get_fruts_info(siots_p, &read_info[0], &write_info[0], NULL);
            if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
            {
                fbe_raid_siots_trace(siots_p, 
                                     FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                     "parity:468 fail to get frut:stat=0x%x, "
                                     "siots=0x%p,siots->alg=0x%x\n",
                                      status, siots_p, siots_p->algorithm);
                return status;
            }

            status = fbe_parity_468_calculate_num_sgs(siots_p, 
                                                      &num_sgs[0], 
                                                      &read_info[0],
                                                      &write_info[0],
                                                      FBE_TRUE);
            if (RAID_COND_STATUS((status == FBE_STATUS_GENERIC_FAILURE), status))
            {
                fbe_raid_siots_trace(siots_p, 
                                     FBE_RAID_SIOTS_TRACE_PARAMS_INFO,
                                     "parity:468 fail to calc num of sgs:stat=0x%x, "
                                     "siots=0x%p,siots->alg=0x%x\n",
                                     status, siots_p, siots_p->algorithm);
                 return status;
            }

            /* If the request exceeds the sgl limit or the per-drive 
             * transfer limit split the request in half.
             */
            status = fbe_raid_siots_check_fru_request_limit(siots_p,
                                                            &read_info[0],
                                                            &write_info[0],
                                                            NULL,
                                                            status);
            if (RAID_COND_STATUS((status == FBE_STATUS_GENERIC_FAILURE), status))
            {
                fbe_raid_siots_trace(siots_p, 
                                     FBE_RAID_SIOTS_TRACE_PARAMS_INFO,
                                     "parity:468 fail to limit frut req:stat=0x%x, "
                                     "siots=0x%p,siots->alg=0x%x\n",
                                     status, siots_p, siots_p->algorithm);
                 return status;
            }

        }
        else if (siots_p->algorithm == R5_MR3)
        {
            /* Next calculate the number of sgs of each type.
             */ 
            status = fbe_parity_mr3_get_fruts_info(siots_p, &read_info[0], &read2_info[0], &write_info[0], NULL, NULL);
            if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
            {
                fbe_raid_siots_trace(siots_p, 
                                     FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                     "parity:mr3 fail to get frut:stat=0x%x, "
                                     "siots=0x%p,siots->alg=0x%x\n",
                                     status, siots_p, siots_p->algorithm);
                return status;
            }
            status = fbe_parity_mr3_calculate_num_sgs(siots_p, 
                                                      &num_sgs[0], 
                                                      &read_info[0],  
                                                      &read2_info[0],
                                                      &write_info[0],
                                                      FBE_TRUE);
            if (RAID_COND_STATUS((status == FBE_STATUS_GENERIC_FAILURE), status))
            {
                fbe_raid_siots_trace(siots_p, 
                                     FBE_RAID_SIOTS_TRACE_PARAMS_INFO,
                                     "parity:mr3 fail to cal num of sgs:stat=0x%x"
                                     ",siots=0x%p,siots->alg=0x%x\n",
                                     status, siots_p, siots_p->algorithm);
                 return status;
            }

            /* If the request exceeds the sgl limit or the per-drive 
             * transfer limit split the request in half.
             */
            status = fbe_raid_siots_check_fru_request_limit(siots_p,
                                                            &read_info[0],
                                                            &write_info[0],
                                                            &read2_info[0],
                                                            status);
            if (RAID_COND_STATUS((status == FBE_STATUS_GENERIC_FAILURE), status))
            {
                fbe_raid_siots_trace(siots_p, 
                                     FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                     "parity:mr3 fail to limit frut req:stat=0x%x"
                                     ",siots=0x%p\n",
                                     status, siots_p);
                return status;
            }
        } 
        else if (siots_p->algorithm == R5_RCW)
        {
            /* Next calculate the number of sgs of each type.
             */ 
            status = fbe_parity_rcw_get_fruts_info(siots_p, &read_info[0], &read2_info[0], &write_info[0], NULL, NULL);
            if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
            {
                fbe_raid_siots_trace(siots_p, 
                                     FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                     "parity:rcw fail to get frut:stat=0x%x, "
                                     "siots=0x%p,siots->alg=0x%x\n",
                                     status, siots_p, siots_p->algorithm);
                return status;
            }
            status = fbe_parity_rcw_calculate_num_sgs(siots_p, 
                                                      &num_sgs[0], 
                                                      &read_info[0],  
                                                      &read2_info[0],
                                                      &write_info[0],
                                                      FBE_TRUE);
            if (RAID_COND_STATUS((status == FBE_STATUS_GENERIC_FAILURE), status))
            {
                fbe_raid_siots_trace(siots_p, 
                                     FBE_RAID_SIOTS_TRACE_PARAMS_INFO,
                                     "parity:rcw fail to cal num of sgs:stat=0x%x"
                                     ",siots=0x%p,siots->alg=0x%x\n",
                                     status, siots_p, siots_p->algorithm);
                 return status;
            }

            /* If the request exceeds the sgl limit or the per-drive 
             * transfer limit split the request in half.
             */
            status = fbe_raid_siots_check_fru_request_limit(siots_p,
                                                            &read_info[0],
                                                            &write_info[0],
                                                            &read2_info[0],
                                                            status);
            if (RAID_COND_STATUS((status == FBE_STATUS_GENERIC_FAILURE), status))
            {
                fbe_raid_siots_trace(siots_p, 
                                     FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                     "parity:rcw fail to limit frut req:stat=0x%x"
                                     ",siots=0x%p\n",
                                     status, siots_p);
                return status;
            }
        }
        else if (siots_p->algorithm == R5_RD)
        {
            status = fbe_parity_read_get_fruts_info(siots_p, &read_info[0]);
            if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
            {
                fbe_raid_siots_trace(siots_p, 
                                     FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                     "parity: failed to get fruts info: status = 0x%x, siots_p = 0x%p "
                                     "siots_p->algorithm = 0x%x\n",
                                     status, siots_p, siots_p->algorithm);
                return status;
            }

            /* Next calculate the number of sgs of each type.
             */ 
            status = fbe_parity_read_calculate_num_sgs(siots_p, 
                                                       &num_sgs[0], 
                                                       &read_info[0],
                                                       FBE_TRUE);
            if (RAID_COND_STATUS((status == FBE_STATUS_GENERIC_FAILURE), status))
            {
                fbe_raid_siots_trace(siots_p, 
                                     FBE_RAID_SIOTS_TRACE_PARAMS_INFO,
                                     "parity: failed to calculate number of sgs: status = 0x%x, "
                                     "siots_p = 0x%p, siots_p->algorithm = 0x%x\n",
                                     status, siots_p, siots_p->algorithm);
                return status;
            }

            /* If the request exceeds the sgl limit or the per-drive 
             * transfer limit split the request in half.
             */
            status = fbe_raid_siots_check_fru_request_limit(siots_p,
                                                            &read_info[0],
                                                            NULL,
                                                            NULL,
                                                            status);
            if (RAID_COND_STATUS((status == FBE_STATUS_GENERIC_FAILURE), status))
            {
                fbe_raid_siots_trace(siots_p, 
                                     FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                     "parity: failed to check fru request limit: "
                                     "status = 0x%x, siots_p = 0x%p, siots_p->algorithm = 0x%x\n",
                                     status, siots_p, 
                                     siots_p->algorithm);
                return status;
            }
        }
        else
        {
             fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                  "parity: got error: siots_p = 0x%p, unexpected algorithm = 0x%x\n", 
                                  siots_p, siots_p->algorithm);
             return FBE_STATUS_GENERIC_FAILURE;
        }
        if (status == FBE_STATUS_INSUFFICIENT_RESOURCES)
        {
            /* Exceeded sg limits, cut it in half.
             */
            status = fbe_parity_limit_blocks(siots_p, &blocks);
            if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
            {
               fbe_raid_siots_trace(siots_p, 
                                     FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                     "parity: failed to limit blocks as exceeded resource: status = 0x%x, siots_p = 0x%p\n",
                                     status, siots_p);
                return status;
            }
            siots_p->xfer_count = blocks;
            status = fbe_parity_read_write_calculate(siots_p);
            if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
            {
                fbe_raid_siots_trace(siots_p, 
                                     FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                     "parity: failed to cut sg limits as it exceeded resource: status = 0x%x, siots_p = 0x%p\n",
                                     status, siots_p);
                return status;
            }

            /* If we have updated the request re-validate it
             */
            status = FBE_STATUS_INSUFFICIENT_RESOURCES;
        }
    }
    while ((status == FBE_STATUS_INSUFFICIENT_RESOURCES) &&
           (blocks != 0));

    /* Save value to return.
     */
    *blocks_p = blocks;

    if ((blocks == 0) || (status != FBE_STATUS_OK))
    {
        /* Something is wrong, we did not think we could generate any blocks.
         */
        return_status = FBE_STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
        return_status = FBE_STATUS_OK;
    }
    return return_status;
}
/******************************************
 * end fbe_parity_get_max_read_write_blocks()
 ******************************************/

/*!**************************************************************
 * fbe_parity_limit_blocks for_write_logging()
 ****************************************************************
 * @brief
 *  Prep the siots for write logging
 *  -- set header buffer and limit block as needed
 *
 * @param siots_p - this I/O.
 * @param blocks_p - Number of blocks we can generate.
 *
 * @return fbe_status_t   
 *
 ****************************************************************/
fbe_status_t fbe_parity_limit_blocks_for_write_logging(fbe_raid_siots_t * siots_p,
                                                       fbe_block_count_t *blocks_p)
{
    fbe_status_t          status = FBE_STATUS_OK;
    fbe_block_count_t     blocks = *blocks_p;
    fbe_raid_geometry_t  *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_u32_t             blocks_per_element;
    fbe_u16_t             data_disks;
    fbe_u32_t             blocks_per_data_stripe;
    fbe_bool_t            limiting_io_size = FBE_FALSE;
    fbe_payload_block_operation_opcode_t opcode;

    /* Check if write_logging is required for any portion of this IO */
    if (fbe_parity_is_write_logging_required(siots_p))
    {
        /* This io has some loggable portion, but may have portions that can skip logging.
         * If it is a mix, we will limit the io size to separate them. So we start by
         *   looking at the first section for alignment (code for success, cache flush typical)
         * In any case that is going to log writes, we set flag to allocate buffers for the write log headers.
         */ 
        fbe_raid_geometry_get_element_size(raid_geometry_p, &blocks_per_element);
        fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);
        blocks_per_data_stripe = blocks_per_element * data_disks;
        fbe_raid_siots_get_opcode(siots_p, &opcode);

        if (((opcode != FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE) &&
             (opcode != FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_NONCACHED) &&
             (opcode != FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_WRITE)) ||
            (siots_p->start_lba % blocks_per_data_stripe))
        {
            /* Either this is not an opcode that can skip journaling for full stripes
             * or 
             * (we are an opcode that can skip journaling) and Start is unaligned
             * -- set buffer flag and see if it needs limiting 
             */
            fbe_raid_siots_set_flag(siots_p, FBE_RAID_SIOTS_FLAG_WRITE_LOG_HEADER_REQD);

            if (siots_p->xfer_count > (blocks_per_data_stripe - (siots_p->start_lba % blocks_per_data_stripe)))
            {
                /* Exceeded the limit, reduce to stop on the next data stripe boundary. */
                blocks = blocks_per_data_stripe - (siots_p->start_lba % blocks_per_data_stripe);
                siots_p->xfer_count = blocks;
                limiting_io_size = FBE_TRUE;
            }
        }
        else if (siots_p->xfer_count % blocks_per_data_stripe)
        {
            /* Start is aligned -- but end is not
             */
            if (siots_p->xfer_count < (blocks_per_data_stripe))
            {
                /* Less than one data stripe, no need to limit, just set buffer flag */
                fbe_raid_siots_set_flag(siots_p, FBE_RAID_SIOTS_FLAG_WRITE_LOG_HEADER_REQD);
            }
            else
            {
                /* More than one data stripe, limit to as many whole stripes as possible but don't set flag;
                 *   --- Unless test code has set the disable skip flag, in which case limit to one whole stripe
                 *       because it's going to log and it can't handle a big io.
                 */
                if (fbe_raid_geometry_is_debug_flag_set(raid_geometry_p, FBE_RAID_LIBRARY_DEBUG_FLAG_DISABLE_WRITE_LOG_SKIP))
                {
                    fbe_raid_siots_set_flag(siots_p, FBE_RAID_SIOTS_FLAG_WRITE_LOG_HEADER_REQD);
                    blocks = blocks_per_data_stripe;
                    siots_p->xfer_count = blocks;
                    limiting_io_size = FBE_TRUE;
                }
                else
                {
                    blocks = siots_p->xfer_count - (siots_p->xfer_count % blocks_per_data_stripe);
                    siots_p->xfer_count = blocks;
                    limiting_io_size = FBE_TRUE;
                }
            }
        }
        else
        {
            /* Else start and end are aligned -- set buffer flag if write log skip flag is set,
             * and split it if it's big
             */
            if (fbe_raid_geometry_is_debug_flag_set(raid_geometry_p, FBE_RAID_LIBRARY_DEBUG_FLAG_DISABLE_WRITE_LOG_SKIP))
            {
                fbe_raid_siots_set_flag(siots_p, FBE_RAID_SIOTS_FLAG_WRITE_LOG_HEADER_REQD);

                if (siots_p->xfer_count > blocks_per_data_stripe)
                {
                    blocks = blocks_per_data_stripe;
                    siots_p->xfer_count = blocks;
                    limiting_io_size = FBE_TRUE;
                }
            }
        }
        if (limiting_io_size)
        {
            /* Re-adjust the siots params */
            status = fbe_parity_read_write_calculate(siots_p);
        }

        /* Push the new blocks value back to the caller
         */
        *blocks_p = blocks;
	}

    return status;
}
/******************************************
 * end fbe_parity_limit_blocks_for_write_logging()
 ******************************************/

/****************************************************************
 * fbe_parity_generate_get_write_blocks()
 ****************************************************************
 * @brief
 *  Generate a normal RAID 5 read request by filling out the siots.
 *
 * @param siots_p - Current sub request. 
 *
 * @return fbe_status_t
 *
 ****************************************************************/

static fbe_status_t fbe_parity_generate_get_write_blocks(fbe_raid_siots_t * const siots_p)
{
    fbe_status_t status;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_u32_t parity_drives;
    fbe_block_count_t blocks = siots_p->xfer_count;
    fbe_u32_t blocks_per_element;

    fbe_raid_geometry_get_num_parity(raid_geometry_p, &parity_drives);
    fbe_raid_geometry_get_element_size(raid_geometry_p, &blocks_per_element);
    /* Make sure we only have a single parity extent.
     */
    blocks = fbe_parity_siots_limit_to_one_parity_extent(siots_p, blocks_per_element, blocks);

    if (!fbe_raid_geometry_is_raid3(raid_geometry_p))
    { 
        /* We always will limit this write request to Parity stripe.
         * Raid 3 does not perform this limit since the entire group 
         * is a single parity stripe. 
         */
        blocks = FBE_MIN(blocks, siots_p->geo.max_blocks);
    }

    /* Check if this I/O is degraded, non-degraded
     * or spanning the checkpoint.
     * Spanning I/Os will do the non-degraded portion first.
     */
    siots_p->xfer_count = blocks;
    
    if (fbe_raid_iots_is_flag_set((fbe_raid_iots_t*)siots_p->common.parent_p, FBE_RAID_IOTS_FLAG_NON_DEGRADED))
    {
        fbe_raid_siots_init_dead_positions(siots_p);
    }
    else
    {
        /* Check to see if there is a dead position in the array.
         * If so then we need to calculate the new range
         * of this I/O based on the degraded region.
         * 
         * This fn will handle all the breaking up
         * of the request over rebuild checkpoints.
         */
        status = fbe_parity_check_for_degraded(siots_p);
    
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_siots_trace(siots_p, FBE_TRACE_LEVEL_ERROR, __LINE__, "parity_gen_get_wt_block" ,
                                 "parity:fail to chk for degrade state:status=0x%x,siots=0x%p\n",
                                 status, siots_p);
            return status;
        }
    }

    /* We should not generate more than the transfer count.
     * We check this here to validate the return value from
     * the above function.
     */
    if (RAID_COND(blocks > siots_p->xfer_count))
    {
        fbe_raid_siots_trace(siots_p, FBE_TRACE_LEVEL_ERROR, __LINE__, "parity_gen_get_wt_block" ,
                             "parity_gen_get_wt_block err: blocks 0x%llx > xfer_count 0x%llx:siots_p=0x%p\n",
                             (unsigned long long)blocks, (unsigned long long)siots_p->xfer_count, siots_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    if (RAID_COND(blocks == 0))
    {
        fbe_raid_siots_trace(siots_p, FBE_TRACE_LEVEL_ERROR, __LINE__, "parity_gen_get_wt_block" ,
                             "parity_gen_get_wt_block err: blocks is zero: siots_p=0x%p\n",
                             siots_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    siots_p->xfer_count = blocks;

    /* Calculate some initial values in the siots.
     */
    status = fbe_parity_read_write_calculate(siots_p);

    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        /* There was an invalid parameter in the siots, return error now.
         */
        fbe_raid_siots_trace(siots_p, FBE_TRACE_LEVEL_ERROR, __LINE__, "parity_gen_get_wt_block" ,
                             "parity:fail to calc rd-wt blocks: status=0x%x,siots=0x%p\n",
                             status, siots_p);
        return status;
    }
    /* Limit the request based on the sg list size.
     */
    status = fbe_parity_get_max_read_write_blocks(siots_p, &blocks);

    if (RAID_COND(blocks == 0))
    {
        fbe_raid_siots_trace(siots_p, FBE_TRACE_LEVEL_ERROR, __LINE__, "parity_gen_get_wt_block" ,
                             "parity_gen_get_wt_block: blocks is zero : siots_p = 0x%p\n",
                             siots_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* Our transfer count must always be less than our parity range.
     */
    if (RAID_COND(siots_p->xfer_count < siots_p->parity_count))
    {
        fbe_raid_siots_trace(siots_p, FBE_TRACE_LEVEL_ERROR, __LINE__, "parity_gen_get_wt_block",
                             "parity_gen_get_wt_block:xfer_cnt 0x%llx < parity_cnt 0x%llx:siots_p=0x%p\n",
                             (unsigned long long)siots_p->xfer_count, (unsigned long long)siots_p->parity_count,siots_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Write logging cannot handle per-drive request size equal to max_blocks_per_drive as it 
     * can only log a single data stripe at a time. Check if write logging, and limit blocks 
     * if needed. Also set flag in siots to allocate write log header buffer if logging. 
     */
    status = fbe_parity_limit_blocks_for_write_logging(siots_p, &blocks);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: failed to trim siots to fit write log slot: status = 0x%x, siots_p = 0x%p\n",
                             status, siots_p);
        return status;
    }

    /* Now that we have settled on a block count, set it into the siots.
     */
    siots_p->xfer_count = blocks;
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_parity_generate_get_write_blocks
 **************************************/

/****************************************************************
 * fbe_parity_generate_write_assign_state()
 ****************************************************************
 * @brief
 *  Assign the state, fill out the disk operations.
 *
 * @param siots_p - Current sub request. 
 *
 * @return fbe_status_t
 *
 ****************************************************************/
fbe_status_t fbe_parity_generate_write_assign_state(fbe_raid_siots_t * const siots_p)
{
    if ((siots_p->algorithm == R5_468) ||
        (siots_p->algorithm == R5_CORRUPT_DATA))
    {
        fbe_raid_siots_transition(siots_p, fbe_parity_468_state0);
    }
    else if (siots_p->algorithm == R5_MR3)
    {
        fbe_bool_t b_is_aligned = fbe_parity_mr3_is_aligned(siots_p);
        fbe_raid_siots_transition(siots_p, fbe_parity_mr3_state0);
        if ( b_is_aligned )
        {
            fbe_raid_iots_t * iots_p = fbe_raid_siots_get_iots(siots_p);
            fbe_lun_performance_counters_t  *performace_stats_p = NULL;
            fbe_raid_iots_get_performance_stats(iots_p,
                                                &performace_stats_p);
            if (performace_stats_p != NULL)
            {
                fbe_cpu_id_t cpu_id;
                fbe_raid_iots_get_cpu_id(iots_p, &cpu_id);
                fbe_raid_perf_stats_inc_mr3_write(performace_stats_p, cpu_id);
            }
        }
    }
    else if (siots_p->algorithm == R5_RCW)  
    {
        fbe_raid_siots_transition(siots_p, fbe_parity_rcw_state0);
    }
    else
    {
        /* We should not get here.  All known algorithms are above.
         * Panic because we do know what to do next
         */
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: %s no unexpected algorithm: 0x%x\n", 
                             __FUNCTION__, siots_p->algorithm);
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_unexpected_error);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_parity_generate_write_assign_state()
 **************************************/
#if 0
/****************************************************************
 * r5_gen_backfill()
 ****************************************************************
 * @brief
 *  Before allowing the operation specific state machines to
 *  run we need to determine if:
 *  1) There are additional blocks to generate.
 *  2) How much to generate.
 *
 * @param siots_p - Current sub request.
 *
 * @return
 *  blocks remaining in this request. 
 *
 *  @author
 *   05/09/00 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_u32_t r5_gen_backfill(fbe_raid_siots_t * const siots_p)
{
    fbe_u32_t preceeding_tokenized_blocks,
      current_write_blocks,
      non_tokenized_blocks;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_sg_element_with_offset_t sg_desc;

    preceeding_tokenized_blocks =
        current_write_blocks =
        non_tokenized_blocks = 0;

    /* Init SG Descriptor to reference cache data
     */
    status = fbe_raid_siots_setup_cached_sgd(siots_p, &sg_desc);
    if (RAID_COND(status != FBE_STATUS_OK))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "parity: failed to setup cache sg: siots_p = 0x%p, status = 0x%x\n",
                             siots_p, status);
        return siots_p->xfer_count;
    }

    /* Determine amount to backfill now.
     */
    rg_sg_backfill_count_raw(siots_p->xfer_count,
                             &sg_desc,
                             RG_TOKEN_ADDRESS,
                             &preceeding_tokenized_blocks,
                             &non_tokenized_blocks);

    if (preceeding_tokenized_blocks)
    {
        /* Decerement any preceeding tokens from overall
         * transfer.
         */
        rg_siots_dec_blocks(siots_p, preceeding_tokenized_blocks);
    }

    return siots_p->xfer_count;
} /* end r5_gen_backfill() */
#endif
/*************************
 * end file fbe_parity_generate.c
 *************************/
