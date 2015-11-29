/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_mirror_generate.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the generate code for mirrors.
 *
 * @version
 *  01/12/2010  Ron Proulx  - Re-written
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe_raid_library_proto.h"
#include "fbe_raid_library.h"
#include "fbe_mirror_io_private.h"
#include "fbe/fbe_mirror.h"

/*************************
 *   FORWARD DECLARATIONS 
 *************************/
static fbe_status_t fbe_mirror_generate_zero(fbe_raid_siots_t *siots_p);
static fbe_status_t fbe_mirror_generate_check_zeroed(fbe_raid_siots_t *siots_p);

/*!**************************************************************************
 *          fbe_mirror_generate_validate_siots()
 ****************************************************************************
 *
 * @brief   This is a generic function that validate that a siots has been
 *          properly generated (i.e. populated) for a mirror request.  This
 *          code is specific for the mirror algorithms.  
 *
 * @param   siots_p - Pointer to this I/O.
 * 
 * @return  status - Exception for code issues, the status should always be
 *                   FBE_STATUS_OK.
 *
 * @author
 *  12/09/2009  Ron Proulx  - Created from fbe_parity_generate_validate_siots
 *
 **************************************************************************/
fbe_status_t fbe_mirror_generate_validate_siots(fbe_raid_siots_t *siots_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t width = fbe_raid_siots_get_width(siots_p);

    /* Validate the width in the geometry.
     */
    if (width > FBE_MIRROR_MAX_WIDTH)
    {
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: width 0x%x is greater than maximum 0x%x\n",
                             width, FBE_MIRROR_MAX_WIDTH);
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    if (width < FBE_MIRROR_MIN_WIDTH)
    {
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: width 0x%x is less than minimum 0x%x\n",
                             width, FBE_MIRROR_MIN_WIDTH);
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    
    /* Now use that width to check siots.
     */
    if (siots_p->start_pos >= width)
    {
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: error start position: 0x%x width: 0x%x\n",
                             siots_p->start_pos, width);
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    if (siots_p->data_disks > width)
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "mirror: data_disks %d is greater than width. 0x%x\n", 
                             siots_p->data_disks, width);
        status = FBE_STATUS_GENERIC_FAILURE;
    }

    /* The raid algorithms no longer support `pass-thru' (i.e. control) requests.
     * Thus there is always data to transfer.
     */
    if (siots_p->data_disks == 0)
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "mirror: data_disks is zero. 0x%x\n", siots_p->data_disks);
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    if (siots_p->xfer_count == 0)
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "mirror: xfer_count is unexpected. 0x%llx\n",
			     (unsigned long long)siots_p->xfer_count);
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    if (siots_p->parity_count == 0)
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "mirror: parity_count is zero \n");
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    if (!fbe_raid_siots_validate_page_size(siots_p))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                     "mirror: page_size is invalid \n");
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    
    /* Return the mirror validation status.
     */
    return(status);
}
/******************************************
 * end fbe_mirror_generate_validate_siots()
 ******************************************/


/*!***************************************************************************
 *          fbe_mirror_generate_read()
 *****************************************************************************
 *
 * @brief   Generate a mirror read request.  This method will automatically
 *          a non-degraded position if there is one available. For raw mirror
 *          reads, it uses the same state machine initially.
 *
 * @param  siots_p - Siots that represents the read.               
 *
 * @return  status - Typically FBE_STATUS_OK
 *                   Otherwise the request wasn't properly formed 
 *
 * @note    Previously mirror reads would be `optimized' using a database
 *          to determine which position was the least busy.  But now the
 *          downstream object will generate an event when it reaches a
 *          certain busy level.
 * 
 * @author
 *  02/05/2010  Ron Proulx  - Created
 *
 ****************************************************************/
static fbe_status_t fbe_mirror_generate_read(fbe_raid_siots_t * siots_p)
{
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_payload_block_operation_opcode_t opcode;
    fbe_bool_t              b_ok_to_split = FBE_FALSE;  /*! @note Currently we don't allow reads to be split */

    /* Set the algorithm based on opcode.
     */
    fbe_raid_siots_get_opcode(siots_p, &opcode);
    switch(opcode)
    {
        /* Only opcode and algorithm supported is read.
         */
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ:
            /* Set algorithm to read.
             */
            siots_p->algorithm = MIRROR_RD;
            break;

        default:
            /* Opcode not supported, fail request.
             */
            fbe_raid_siots_trace(siots_p, 
                                 FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "mirror: generate read opcode: 0x%x not supported. \n",
                                 opcode);
            return(FBE_STATUS_GENERIC_FAILURE);
    } 

    /* We must populate `data_disks' before limiting the request. The number of
     * `data disks' determines how many positions we read from. We simply read 
     * from the primary position.
     */
    siots_p->data_disks = 1;
     
    /* Now limit (actually check) the parent request to validate that
     * the request size doesn't exceed the memory infrastructure.
     */
    if ( (status = fbe_mirror_read_limit_request(siots_p)) != FBE_STATUS_OK )
    {
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: generate read: limit request failed with status: 0x%x\n",
                             status);
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    /*! Now generate the `degraded' positions and validate that we can proceed
     *
     *  @note We don't allow writes to accessible, degraded positions since the
     *        range must be rebuilt before it is valid and in addition the drive
     *        may have issues that additional writes could bring out.
     */
    status = fbe_mirror_set_degraded_positions(siots_p, b_ok_to_split);
    if (RAID_GENERATE_COND(status != FBE_STATUS_OK))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: siots_p: 0x%p generate set degraded failed. status: 0x%0x\n",
                             siots_p, status);
        return(status);
    }

    /* At this point we have populated the degraded information for this siots.
     * We simply read from the primary position.
     */
    siots_p->parity_pos = fbe_mirror_get_parity_position(siots_p);
    if (siots_p->parity_pos == FBE_RAID_INVALID_POS)
    {
        /* we can have invalid parity position if there isn't second
         * non-degraded position.
         */
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_DEBUG_HIGH,
                             "mirror: siots_p->parity_pos == FBE_RAID_INVALID_POS\n");
    }
    siots_p->start_pos = fbe_mirror_get_primary_position(siots_p);

    /*! Fill out the remainder of the generic siots fields.
     *  For a mirror verify we use the standard FBE_RAID_DEFAULT_RETRY_COUNT.
     */
    siots_p->retry_count = FBE_RAID_DEFAULT_RETRY_COUNT;
    siots_p->drive_operations = siots_p->data_disks;

    /* The last thing we do is transition to the read state machine.
     * We only transition if the status is ok.
     */
    if (status == FBE_STATUS_OK)
    {
        /* Adjust the iots to account for this request 
         */
        fbe_raid_iots_dec_blocks(fbe_raid_siots_get_iots(siots_p), siots_p->xfer_count);

        /* Transition to verify state machine.
         */
        fbe_raid_siots_transition(siots_p, fbe_mirror_read_state0);
    }

    /* Always return the execution status.
     */
    return(status);
}
/******************************************
 * end fbe_raid_mirror_generate_read()
 ******************************************/

/*!***************************************************************************
 *          fbe_mirror_generate_write()
 *****************************************************************************
 *
 * @brief   Generate a mirror write request by filling out the siots.  The
 *          of `data disk' is the width of the raid group but the same write
 *          buffer will be used for all positions.  Similarly only (1) position
 *          will be read for the pre-read required for an unaligned write.
 *
 * @param siots_p - Current sub request.              
 *
 * @return  status - Typically FBE_STATUS_OK
 *                   Otherwise something went wrong generating the request   
 *
 
 * @note    Example of 2-way mirror for an unaligned write request.  The write
 *          starts at lba thru 0x11287 thru 0x14507 with an element size of 128.
 *          Thus the write fruts is from 0x11287 for 0x3281 blocks.  The pre-read
 *          fruts is from 0x11286 for 0x3283 blocks.  The write request may need
 *          to be broken up (i.e. limited) by the maximum buffers available since
 *          the pre-read will require a larger buffer than the write request.
 *
 *  lba             Position 0           Position 1     FRUTS
 *  0x11200         +--------+           +--------+     -----
 *                  |        |           |        |
 *  0x11280  siots  +--------+<-0x11286->+--------+     <= Pre-read
 *         0x11287->|wwwwwwww|  rrrrrrr  |        |     <= Write
 *  0x12300         +--------+  rrrrrrr  +--------+
 *                  |wwwwwwww|  rrrrrrr  |        |     
 *  0x12380         +--------+  rrrrrrr  +--------+
 *                  |wwwwwwww|  rrrrrrr  |        |     
 *  0x13400         +--------+  rrrrrrr  +--------+
 *                  |wwwwwwww|  rrrrrrr  |        |     
 *  0x13480         +--------+  rrrrrrr  +--------+
 *                  |wwwwwwww|  rrrrrrr  |        |
 *  0x14500         +--------+  rrrrrrr  +--------+
 *         0x14507->|wwwwwwww|  rrrrrrr  |        |     <= Write
 *  0x14580         +--------+<-0x14508->+--------+     <= Post-read
 *                  |        |           |        |
 *
 * @author
 *  02/08/2010  Ron Proulx  - Created
 *
 *****************************************************************************/
static fbe_status_t fbe_mirror_generate_write(fbe_raid_siots_t *siots_p)
{
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_payload_block_operation_opcode_t opcode;
    fbe_u32_t               width = fbe_raid_siots_get_width(siots_p);
    fbe_bool_t              b_ok_to_split = FBE_FALSE;  /*! @note Currently we don't allow writes to be split */

    /* Set the algorithm based on opcode.
     */
    fbe_raid_siots_get_opcode(siots_p, &opcode);
    switch(opcode)
    {
        /* Determine the algorithm based on operation code.
         */
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_VERIFY:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_NONCACHED:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ZERO:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_REKEY_WRITE:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_REKEY_WRITE_ZEROS:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_ZEROS:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_REKEY_ZERO:
            /* Standard write.
             */
            siots_p->algorithm = MIRROR_WR;
            break;
            
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_CORRUPT_DATA:
            /* Corrupt Data operation.
             */
            siots_p->algorithm = MIRROR_CORRUPT_DATA;
            break;

        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_WRITE:
            /* Verify write (a.k.a. BVA) request.
             */
            siots_p->algorithm = MIRROR_VR_WR;
            break;

        default:
            /* Opcode not supported, fail request.
             */
            fbe_raid_siots_trace(siots_p, 
                                 FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "mirror: generate write opcode: 0x%x not supported. \n",
                                 opcode);
            return(FBE_STATUS_GENERIC_FAILURE);
    }

    /* We need to populate the `data_disks' before we limit the request.
     * Although some positions maybe dead, we always set the number of
     * `data_disks' to the raid group width for mirror writes.
     */
    siots_p->data_disks = width;
     
    /* Although the parent has allocated the parent buffers this maybe an
     * unaligned request.  If so we need to allocate a pre-read buffer and thus
     * may need to limit the request size.
     */
    if ( (status = fbe_mirror_write_limit_request(siots_p)) != FBE_STATUS_OK )
    {
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: generate write: limit request failed with status: 0x%x\n",
                             status);
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    /*! Now generate the `degraded' positions and validate that we can proceed
     *
     *  @note We don't allow writes to accessible, degraded positions since the
     *        range must be rebuilt before it is valid and in addition the drive
     *        may have issues that additional writes could bring out.
     */
    status = fbe_mirror_set_degraded_positions(siots_p, b_ok_to_split);
    if (RAID_GENERATE_COND(status != FBE_STATUS_OK))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: siots_p: 0x%p generate set degraded failed. status: 0x%0x\n",
                             siots_p, status);
        return(status);
    }

    /* At this point we have populated the degraded information for this siots.
     * The start postion is always the primary and the `parity' (i.e. secondary)
     * position is any other readable position.
     */
    siots_p->parity_pos = fbe_mirror_get_parity_position(siots_p);
    if (siots_p->parity_pos == FBE_RAID_INVALID_POS)
    {
        /* we can have invalid parity position if there isn't second
         * non-degraded position.
         */
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_DEBUG_HIGH,
                             "mirror: siots_p->parity_pos == FBE_RAID_INVALID_POS\n");
    }
    siots_p->start_pos = fbe_mirror_get_primary_position(siots_p);

    /*! Fill out the remainder of the generic siots fields.
     *  For a mirror verify we use the standard FBE_RAID_DEFAULT_RETRY_COUNT.
     */
    siots_p->retry_count = FBE_RAID_DEFAULT_RETRY_COUNT;
    siots_p->drive_operations = siots_p->data_disks;

    /* The last thing we do is transition to the read state machine.
     * We only transition if the status is ok.
     */
    if (status == FBE_STATUS_OK)
    {
        /* Adjust the iots to account for this request 
         */
        fbe_raid_iots_dec_blocks(fbe_raid_siots_get_iots(siots_p), siots_p->xfer_count);

        /* Transition to write state machine.
         */
        fbe_raid_siots_transition(siots_p, fbe_mirror_write_state0);
    }

    /* Always return the execution status.
     */
    return(status);
}
/*****************************************
 * end fbe_mirror_generate_write()
 *****************************************/

/*!***************************************************************************
 * fbe_mirror_generate_zero()
 *****************************************************************************
 *
 * @brief   Generate a zero.
 *
 * @param siots_p - Current sub request.              
 *
 * @return  status - Typically FBE_STATUS_OK
 *                   Otherwise something went wrong generating the request   
 *
 * @author
 *  3/9/2010 - Created. Rob Foley
 *
 *****************************************************************************/
static fbe_status_t fbe_mirror_generate_zero(fbe_raid_siots_t *siots_p)
{
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_u32_t               width = fbe_raid_siots_get_width(siots_p);
    fbe_bool_t              b_ok_to_split = FBE_FALSE; /*! @note Currently can't split zero request */
    fbe_raid_geometry_t    *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_payload_block_operation_opcode_t opcode;

    fbe_raid_siots_get_opcode(siots_p, &opcode);

    /* If we are not aligned to the optimal block size we need pre-reads.
     */
    if ((opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ZERO) &&
        fbe_raid_geometry_is_raw_mirror(raid_geometry_p)       ){
        /* We need raw mirror to go through the library to write its sequence number and magic number.
         */
        status = fbe_mirror_generate_write(siots_p);
    } else if (fbe_raid_geometry_io_needs_alignment(raid_geometry_p,
                                                    siots_p->start_lba,
                                                    siots_p->xfer_count)) {
        /* If zero request is not aligned to the physical block size use the
         * write state machine.
         */
        status = fbe_mirror_generate_write(siots_p);
    } else {
        /*! @note We no longer limit aligned zero requests since they are
         *        deferred (a.k.a `buffered') operations.  We need to set the 
         *        page size since limit would normally do that.  There are zero
         *        buffers to allocate for aligned zero requests and we allocate
         *        a fruts for every position.
         */

        /*! Now generate the `degraded' positions and validate that we can proceed
         *
         *  @note We don't allow writes to accessible, degraded positions since the
         *        range must be rebuilt before it is valid and in addition the drive
         *        may have issues that additional writes could bring out.
         */
        status = fbe_mirror_set_degraded_positions(siots_p, b_ok_to_split);
        if (RAID_GENERATE_COND(status != FBE_STATUS_OK)) {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "mirror: siots_p: 0x%p generate set degraded failed. status: 0x%0x\n",
                                 siots_p, status);
            return(status);
        }
    
        /* At this point we have populated the degraded information for this siots.
         * The number of `data disks' determines how many positions we read from.
         * We simply read from the primary position.
         */
        siots_p->data_disks = width; 
        siots_p->parity_pos = fbe_mirror_get_parity_position(siots_p);
        if (siots_p->parity_pos == FBE_RAID_INVALID_POS) {
            /* we can have invalid parity position if there isn't second
             * non-degraded position.
             */
            fbe_raid_siots_trace(siots_p, 
                                 FBE_RAID_SIOTS_TRACE_PARAMS_DEBUG_HIGH,
                                 "mirror: siots_p->parity_pos == FBE_RAID_INVALID_POS\n");
        }
        siots_p->start_pos = fbe_mirror_get_primary_position(siots_p);
    
        /*! Fill out the remainder of the generic siots fields.
         *  For a mirror verify we use the standard FBE_RAID_DEFAULT_RETRY_COUNT.
         */
        siots_p->retry_count = FBE_RAID_DEFAULT_RETRY_COUNT;
        siots_p->drive_operations = siots_p->data_disks;
        siots_p->algorithm = RG_ZERO;
        siots_p->total_blocks_to_allocate = 0;
        fbe_raid_siots_set_optimal_page_size(siots_p,
                                             width, 
                                             siots_p->total_blocks_to_allocate);
        
        /* Adjust the iots to account for this request.
         */
        fbe_raid_iots_dec_blocks(fbe_raid_siots_get_iots(siots_p), siots_p->xfer_count);
    
        /* Transition to zero state machine.
         */
        fbe_raid_siots_transition(siots_p, fbe_mirror_zero_state0);

    } /* end else I/O is aligned to physical block size*/
    
    /* Always return the execution status.
     */
    return(status);
}
/*****************************************
 * end fbe_mirror_generate_zero()
 *****************************************/

/*!***************************************************************************
 * fbe_mirror_generate_check_zeroed()
 *****************************************************************************
 *
 * @brief   Generate a mirror check zero request
 *
 * @param   siots_p - Current sub request.
 *
 * @return  status - In most cases FBE_STATUS_OK, otherwise an unexpected
 *                   condition has occurred.
 * @note    We always allocate a fruts for every position in the raid group
 *
 * @author
 *  9/22/2010 - Created. Rob Foley
 *
 *****************************************************************************/
static fbe_status_t fbe_mirror_generate_check_zeroed(fbe_raid_siots_t *siots_p)
{
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_u32_t               width = fbe_raid_siots_get_width(siots_p);
    fbe_u32_t               degraded_count = 0;
    fbe_bool_t              b_ok_to_split = FBE_FALSE;

    siots_p->data_disks = width;

    /*! Now generate the `degraded' positions and validate that we can proceed
     *
     *  @note We don't allow writes to accessible, degraded positions since the
     *        range must be rebuilt before it is valid and in addition the drive
     *        may have issues that additional writes could bring out.
     */
    status = fbe_mirror_set_degraded_positions(siots_p, b_ok_to_split);
    if (RAID_GENERATE_COND(status != FBE_STATUS_OK))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: siots_p: 0x%p generate set degraded failed. status: 0x%0x\n",
                             siots_p, status);
        return(status);
    }

    /* At this point we have populated the degraded information for this siots.
     * The number of `data disks' determines how many positions we read from.
     * Thus for a 3-way mirror with only (1) degraded position will will actually
     * read from (2) positions.  The XOR library will use the `verify' algorithm
     * to execute the rebuild when there is either more than (1) read position or
     * more than (1) write (i.e. position to be rebuilt) position.
     * See notes above for more detail.
     */
    fbe_raid_siots_set_page_size(siots_p, FBE_RAID_PAGE_SIZE_STD);
    fbe_raid_siots_set_data_page_size(siots_p, FBE_RAID_PAGE_SIZE_STD);
    degraded_count = fbe_raid_siots_get_degraded_count(siots_p); 
    siots_p->data_disks = width - degraded_count; 
    siots_p->parity_pos = fbe_mirror_get_parity_position(siots_p);
    if (siots_p->parity_pos == FBE_RAID_INVALID_POS)
    {
        /* we can have invalid parity position if there isn't second
         * non-degraded position.
         */
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_INFO,
                             "mirror: siots_p->parity_pos == FBE_RAID_INVALID_POS\n");
    }
    siots_p->start_pos = fbe_mirror_get_primary_position(siots_p);

    /* Fill out the remainder of the generic siots fields.
     * For a mirror rebuild the retry count is the standard raid group
     * retry count.
     * (Dead positions have already been  populated).
     */
    siots_p->retry_count = FBE_RAID_DEFAULT_RETRY_COUNT;
    siots_p->drive_operations = siots_p->data_disks;
    siots_p->algorithm = RG_CHECK_ZEROED;

    /* Adjust the iots to account for this request
    */
    fbe_raid_iots_dec_blocks(fbe_raid_siots_get_iots(siots_p), siots_p->xfer_count);
    fbe_raid_siots_transition(siots_p, fbe_raid_check_zeroed_state0);

    /* Always return the execution status.
     */
    return(status);
}
/*****************************************
 * end fbe_mirror_generate_check_zeroed()
 *****************************************/

/*!***************************************************************************
 *          fbe_mirror_generate_rebuild()
 *****************************************************************************
 *
 * @brief   Generate a mirror rebuild request by filling out the siots.
 *
 * @param   siots_p, [IO] - Current sub request.
 *
 * @return  status - In most cases FBE_STATUS_OK, otherwise an unexpected
 *                   condition has occurred.
 *
 * @note    Example of 3-way mirror where position 1 was removed and replaced then
 *          position 2 was removed and re-inserted. (NR - Needs Rebuild, s - siots
 *          range).  Note that all 'Needs Rebuild' ranges are in `chunk' size 
 *          multiples where the chunk size is 2048 (0x800) blocks.
 *
 *  lba             Position 0           Position 1           Position 2  SIOTS
 *  0x11200         +--------+           +--------+           +--------+  -----
 *                  |        |           |        |           |        |
 *  0x11a00  siots  +--------+           +--------+           +--------+
 *         0x11e00->|ssssssss|           |        |           |        |  <= 1
 *  0x12200         +--------+           +--------+ First RL->+========+
 *                  |ssssssss|           |        |           |NR NR NR|  <= 2
 *  0x12a00         +--------+ Degraded->+--------+           +========+
 *                  |ssssssss|           |NR NR NR|           |        |  <= 3
 *  0x13200         +--------+           +--------+Second RL->+========+
 *                  |ssssssss|           |NR NR NR|           |NR NR NR|  <= 4
 *  0x13a00         +--------+           +--------+           +========+
 *                  |ssssssss|           |NR NR NR|           |        |
 *  0x14200         +--------+           +--------+           +--------+
 *         0x14600->|ssssssss|           |NR NR NR|           |        |
 *  0x14a00         +--------+           +--------+           +--------+
 *                  |        |           |NR NR NR|           |        |
 *
 *          For this example the original siots request for 0x11e00 thru
 *          0x145ff will be broken into the following siots:    
 *          SIOTS   Primary (i.e. Read)Positions    Degraded Positions that Need Rebuild
 *          -----   ---------------------------     ------------------------------------
 *          1.      Position 0 and 1                None @note Is this really a failure?
 *          2.      Position 0 and 1                Position 2
 *          3.      Position 0 and 2                Position 1
 *          4.      Position 0                      Position 0 and 1 
 *          Breaking up the I/O is required by the current xor algorithms
 *          which do not handle a degraded change within a request.
 *
 * @note    We always allocate a fruts for every position in the raid group
 *
 * @author
 *  11/18/2009  Ron Proulx  - Created from mirror_gen_rb.
 *
 *****************************************************************************/
static fbe_status_t fbe_mirror_generate_rebuild(fbe_raid_siots_t *siots_p)
{
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_u32_t               width = fbe_raid_siots_get_width(siots_p);
    fbe_raid_geometry_t    *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_raid_group_type_t   raid_type;
    fbe_u32_t               degraded_count = 0;
    fbe_bool_t              b_ok_to_split = FBE_FALSE; /*! @note Rebuilds should never be split */

    /* Set the algorithm based of mirror type and validate that dead
     * position(s) are acceptable.
     */
    fbe_raid_geometry_get_raid_type(raid_geometry_p, &raid_type);
    switch(raid_type)
    {
        case FBE_RAID_GROUP_TYPE_RAID1:
        case FBE_RAID_GROUP_TYPE_INTERNAL_MIRROR_UNDER_STRIPER:
        case FBE_RAID_GROUP_TYPE_INTERNAL_METADATA_MIRROR:
        case FBE_RAID_GROUP_TYPE_RAW_MIRROR:
            siots_p->algorithm = MIRROR_RB;
            break;

        case FBE_RAID_GROUP_TYPE_SPARE:
            if (fbe_raid_geometry_is_hot_sparing_type(raid_geometry_p))
            {
                /* Copy operation.
                 */
                siots_p->algorithm = MIRROR_COPY;
            }
            else
            {
                /* Else this is a proactive copy.  In the past we would
                 * check the proactive candidate health, but this is now
                 * the responsibility of the virtual drive object.
                 */
                siots_p->algorithm = MIRROR_PACO;
            }
            break;

        default:
            /* RAID Type not supported, fail request.
             */
            fbe_raid_siots_trace(siots_p, 
                                 FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "mirror: generate rebuild raid type: 0x%x not supported. \n",
                                 raid_type);
            return(FBE_STATUS_GENERIC_FAILURE);
    }

    /* We must populate `data_disks' before we limit the request.  Since we 
     * don't know which positions are degraded until we generate the siots,
     * we set the `data_disks' to the width.
     */
    siots_p->data_disks = width;

    /* Now limit (actually check) the parent request to validate that
     * the request size doesn't exceed the memory infrastructure.
     */
    if ( (status = fbe_mirror_rebuild_limit_request(siots_p)) != FBE_STATUS_OK )
    {
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: generate rebuild: limit request failed with status: 0x%x\n",
                             status);
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    /*! Now generate the `degraded' positions and validate that we can proceed
     *
     *  @note We don't allow writes to accessible, degraded positions since the
     *        range must be rebuilt before it is valid and in addition the drive
     *        may have issues that additional writes could bring out.
     */
    status = fbe_mirror_set_degraded_positions(siots_p, b_ok_to_split);
    if (RAID_GENERATE_COND(status != FBE_STATUS_OK))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: siots_p: 0x%p generate set degraded failed. status: 0x%0x\n",
                             siots_p, status);
        return(status);
    }

    /* At this point we have populated the degraded information for this siots.
     * The number of `data disks' determines how many positions we read from.
     * Thus for a 3-way mirror with only (1) degraded position will will actually
     * read from (2) positions.  The XOR library will use the `verify' algorithm
     * to execute the rebuild when there is either more than (1) read position or
     * more than (1) write (i.e. position to be rebuilt) position.
     * See notes above for more detail.
     */
    degraded_count = fbe_raid_siots_get_degraded_count(siots_p); 
    siots_p->data_disks = width - degraded_count; 
    siots_p->parity_pos = fbe_mirror_get_parity_position(siots_p);
    siots_p->start_pos = fbe_mirror_get_primary_position(siots_p);

    /* Fill out the remainder of the generic siots fields.
     * For a mirror rebuild the retry count is the standard raid group
     * retry count.
     * (Dead positions have already been  populated).
     */
    siots_p->retry_count = FBE_RAID_DEFAULT_RETRY_COUNT;
    siots_p->drive_operations = siots_p->data_disks;


    /* The last thing we do is transition to the verify state machine
     * which also handles rebuilds.  We only transition if the status is ok.
     */
    if (status == FBE_STATUS_OK)
    {
        /* Adjust the iots to account for this request
        */
        fbe_raid_iots_dec_blocks(fbe_raid_siots_get_iots(siots_p), siots_p->xfer_count);

        /* If no positions are degraded simply transition to `success'.
         */
        if (degraded_count == 0)
        {
            /*! @note Although this is perfectly acceptable, for now we want to
             *        understand why the monitor issued a rebuild to a
             *        non-degraded region.  So we trace it as an error.
             */
            fbe_raid_siots_trace(siots_p, 
                                 FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "mirror: generate rebuild. No degraded positions. Return success\n");
            fbe_raid_siots_success(siots_p);
        }
        else
        {
            /* Else ready to start the rebuild request.  Transition to rebuild
             * state machine.
             */
            fbe_raid_siots_transition(siots_p, fbe_mirror_rebuild_state0);
        }
    }

    /* Always return the execution status.
     */
    return(status);
}
/*****************************************
 * end fbe_mirror_generate_rebuild()
 *****************************************/

/*!***************************************************************************
 *          fbe_mirror_generate_verify()
 *****************************************************************************
 *
 * @brief   Generate a mirror verify request by filling out the siots.
 *
 * @param   siots_p, [IO] - Current sub request.
 *
 * @return  status - In most cases FBE_STATUS_OK, otherwise an unexpected
 *                   condition has occurred.
 *
 * @note    Example of 3-way mirror where position 1 was removed and replaced 
 *          and position 0 detects correctable CRC errors. (NR - Needs 
 *          Rebuild, s - siots range).  Note that all 'Needs Rebuild' ranges 
 *          are in `chunk' size multiples where the chunk size is 2048 (0x800) 
 *          blocks.
 *
 *  lba             Position 0           Position 1           Position 2  SIOTS
 *  0x11200         +--------+           +--------+           +--------+  -----
 *                  |        |           |        |           |        |
 *  0x11a00  siots  +--------+           +--------+           +--------+
 *         0x11e00->|ssssssss|           |        |           |        |  <= 1
 *  0x12200         +--------+           +--------+ First RL->+========+
 *                  |crc crc |           |        |           |NR NR NR|  <= 2
 *  0x12a00         +--------+ Degraded->+--------+           +========+
 *                  |ssssssss|           |NR NR NR|           |crc crc |  <= 3
 *  0x13200         +--------+           +--------+Second RL->+========+
 *                  |ssssssss|           |NR NR NR|           |NR NR NR|  <= 4
 *  0x13a00         +--------+           +--------+           +========+
 *                  |ssssssss|           |NR NR NR|           |        |
 *  0x14200         +--------+           +--------+           +--------+
 *         0x14600->|ssssssss|           |NR NR NR|           |        |
 *  0x14a00         +--------+           +--------+           +--------+
 *                  |        |           |NR NR NR|           |        |
 *
 *          For this example the original siots request for 0x11e00 thru
 *          0x145ff will be broken into the following siots:    
 *          SIOTS   Primary (i.e. Read)Positions    Positions that are Rebuilt
 *          -----   ---------------------------     ------------------------------------
 *          1.      Position 0 and 1                None
 *          2.      Position 0 and 1                Position 0 and 2
 *          3.      Position 0 and 2                Position 1 and 2
 *          4.      Position 0                      Position 0 and 1 
 *          Breaking up the I/O is required by the current xor algorithms
 *          which do not handle a degraded change within a request.
 *
 * @author
 *  01/12/2010  Ron Proulx  - Created
 *
 *****************************************************************************/
static fbe_status_t fbe_mirror_generate_verify(fbe_raid_siots_t *siots_p)
{
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_u32_t               width = fbe_raid_siots_get_width(siots_p);
    fbe_raid_geometry_t    *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_u32_t               degraded_count = 0;
    fbe_payload_block_operation_opcode_t opcode;
    fbe_bool_t              b_nested_siots = fbe_raid_siots_is_nested(siots_p);
    fbe_bool_t              b_ok_to_split = FBE_TRUE;
    fbe_block_count_t max_blocks_per_drive;

    /* Set the algorithm based on opcode and mirror type.
     */
    fbe_raid_siots_get_opcode(siots_p, &opcode);
    if (b_nested_siots)
    {
        /* It is not ok to split nested requests.
         */
        b_ok_to_split = FBE_FALSE;
        
        /* The following nested opcodes are supported.
         */
        switch(opcode)
        {
            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ:
                /* Algorithm is `read recovery verify' and thus it isn't
                 * ok to split the request.
                 */
                siots_p->algorithm = MIRROR_RD_VR;
                break;

            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE:
            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_NONCACHED:
            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ZERO:
            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_CORRUPT_DATA:
            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_REKEY_WRITE:
                /* This is a read recovery verify for the pre-read
                 * portion of a write.
                 */
                siots_p->algorithm = MIRROR_RD_VR;
                break;

            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_WRITE:
                /* This is a verify write (a.k.a. BVA) request from cache.
                 */
                siots_p->algorithm = MIRROR_VR_WR;
                break;

            default:
                /* Opcode not supported, fail request.
                 */
                fbe_raid_siots_trace(siots_p, 
                                     FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                     "mirror: generate nested verify opcode: 0x%x not supported. \n",
                                     opcode);
                return(FBE_STATUS_GENERIC_FAILURE);
        }
    }
    else
    {
        /* Else only the following opcodes are support for a non-nested
         * siots.
         */
        switch(opcode)
        {
            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY:
            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ_ONLY_VERIFY:
            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ERROR_VERIFY:
            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INCOMPLETE_WRITE_VERIFY:
            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_SYSTEM_VERIFY:
            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_SPECIFIC_AREA:
            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ_ONLY_VERIFY_SPECIFIC_AREA:
                /* In order to limit the number of reads to a proactive
                 * canidate we change the algorithm of this is a proactive
                 * spare raid group.
                 */
                if (fbe_raid_geometry_is_sparing_type(raid_geometry_p))
                {
                    siots_p->algorithm = MIRROR_COPY_VR;
                }
                else
                {
                    siots_p->algorithm = MIRROR_VR;
                }
                break;

            case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_WITH_BUFFER:
                siots_p->algorithm = MIRROR_VR_BUF;
                break;

            default:
                /* Opcode not supported, fail request.
                 */
                fbe_raid_siots_trace(siots_p, 
                                     FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                     "mirror: generate verify opcode: 0x%x not supported. \n",
                                     opcode);
                return(FBE_STATUS_GENERIC_FAILURE);
        } 
    } /* end else not nested */

    /* We must populate `data_disks' before we limit the request.  Since we 
     * don't know which positions are degraded until we generate the siots,
     * we set the `data_disks' to the width.
     */
    siots_p->data_disks = width;
    fbe_raid_geometry_get_max_blocks_per_drive(raid_geometry_p, &max_blocks_per_drive);
    siots_p->xfer_count = FBE_MIN(siots_p->xfer_count, max_blocks_per_drive);

    /* Now limit (actually check) the parent request to validate that
     * the request size doesn't exceed the memory infrastructure.
     */
    if ( (status = fbe_mirror_verify_limit_request(siots_p)) != FBE_STATUS_OK )
    {
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: generate verify: limit request failed with status: 0x%x\n",
                             status);
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    /*! Now generate the `degraded' positions and validate that we can proceed
     *
     *  @note We don't allow writes to accessible, degraded positions since the
     *        range must be rebuilt before it is valid and in addition the drive
     *        may have issues that additional writes could bring out.
     */
    status = fbe_mirror_set_degraded_positions(siots_p, b_ok_to_split);
    if (RAID_GENERATE_COND(status != FBE_STATUS_OK))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: siots_p: 0x%p generate set degraded failed. status: 0x%0x\n",
                             siots_p, status);
        return(status);
    }

    /* At this point we have populated the degraded information for this siots.
     * The number of `data disks' determines how many positions we read from.
     * See notes above for more detail.
     */
    degraded_count = fbe_raid_siots_get_degraded_count(siots_p); 
    siots_p->data_disks = width - degraded_count; 
    siots_p->parity_pos = fbe_mirror_get_parity_position(siots_p);
    if (siots_p->parity_pos == FBE_RAID_INVALID_POS)
    {
        /* we can have invalid parity position if there isn't second
         * non-degraded position.
         */
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_DEBUG_HIGH,
                             "mirror: siots_p->parity_pos == FBE_RAID_INVALID_POS\n");
    }
    siots_p->start_pos = fbe_mirror_get_primary_position(siots_p);

    /*! Fill out the remainder of the generic siots fields.
     *  For a mirror verify we use the standard FBE_RAID_DEFAULT_RETRY_COUNT.
     */
    siots_p->retry_count = FBE_RAID_DEFAULT_RETRY_COUNT;
    siots_p->drive_operations = siots_p->data_disks;


    /* The last thing we do is transition to the verify state machine
     * which also handles rebuilds.  We only transition if the status is ok.
     */
    if (status == FBE_STATUS_OK)
    {
        /* Adjust the iots to account for this request 
         * (unless it is nested)
         */
        if (!b_nested_siots)
        {
            fbe_raid_iots_dec_blocks(fbe_raid_siots_get_iots(siots_p), siots_p->xfer_count);
        }

        /* Transition to verify state machine.
         */
        fbe_raid_siots_transition(siots_p, fbe_mirror_verify_state0);
    }

    /* Always return the execution status.
     */
    return(status);
}
/*****************************************
 * end fbe_mirror_generate_verify()
 *****************************************/
/*!***************************************************************************
 * fbe_mirror_generate_rekey()
 *****************************************************************************
 * @brief
 *  Generate a rekey operation.
 *
 * @param   siots_p, [IO] - Current sub request.
 *
 * @return  status - In most cases FBE_STATUS_OK, otherwise an unexpected error
 **
 * @author
 *  11/5/2013 - Created. Rob Foley
 *
 *****************************************************************************/
static fbe_status_t fbe_mirror_generate_rekey(fbe_raid_siots_t *siots_p)
{
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_bool_t              b_ok_to_split = FBE_FALSE; /*! @note rekey should never be split */

    /* Set the algorithm based of mirror type and validate that dead
     * position(s) are acceptable.
     */
    siots_p->algorithm = MIRROR_REKEY;

    /* We must populate `data_disks' before we limit the request.  Since we 
     * don't know which positions are degraded until we generate the siots,
     * we set the `data_disks' to the width.
     */
    siots_p->data_disks = 1;

    /*! Now generate the `degraded' positions and validate that we can proceed
     *
     *  @note We don't allow writes to accessible, degraded positions since the
     *        range must be rebuilt before it is valid and in addition the drive
     *        may have issues that additional writes could bring out.
     */
    status = fbe_mirror_set_degraded_positions(siots_p, b_ok_to_split);
    if (RAID_GENERATE_COND(status != FBE_STATUS_OK)) {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: siots_p: 0x%p generate set degraded failed. status: 0x%0x\n",
                             siots_p, status);
        return(status);
    }

    /* At this point we have populated the degraded information for this siots.
     * The number of `data disks' determines how many positions we read from.
     * Thus for a 3-way mirror with only (1) degraded position will will actually
     * read from (2) positions.  The XOR library will use the `verify' algorithm
     * to execute the rebuild when there is either more than (1) read position or
     * more than (1) write (i.e. position to be rebuilt) position.
     * See notes above for more detail.
     */
    siots_p->data_disks = 1; 
    siots_p->parity_pos = fbe_mirror_get_parity_position(siots_p);
    siots_p->start_pos = fbe_mirror_get_primary_position(siots_p);

    /* Fill out the remainder of the generic siots fields.
     * For a mirror rebuild the retry count is the standard raid group
     * retry count.
     * (Dead positions have already been  populated).
     */
    siots_p->retry_count = FBE_RAID_DEFAULT_RETRY_COUNT;
    siots_p->drive_operations = siots_p->data_disks;

    siots_p->total_blocks_to_allocate = 0;
    fbe_raid_siots_set_optimal_page_size(siots_p,
                                         1, 
                                         siots_p->xfer_count);

    /* Adjust the iots to account for this request
    */
    fbe_raid_iots_dec_blocks(fbe_raid_siots_get_iots(siots_p), siots_p->xfer_count);

    fbe_raid_siots_transition(siots_p, fbe_mirror_rekey_state0);

    return(status);
}
/*****************************************
 * end fbe_mirror_generate_rekey()
 *****************************************/
/*!***************************************************************************
 * fbe_mirror_generate_start()
 *****************************************************************************
 *
 * @brief   This is the entry point for the generate function for a mirror raid
 *          group.  The steps taken (all relative to this particular siots) are:
 *          o Configure the siots geometry information
 *          o Configure (set) the degraded bitmask
 *          o Invoke the opcode specific generate routine
 *          o Execute generic sanity check of the resulting siots
 *
 * @param siots_p - ptr to sub request.
 *
 * @return fbe_raid_state_status_t - executing.   
 *
 * @author
 *  01/12/2010  - Ron Proulx    - Re-written
 *
 ******************************************************************************/
fbe_raid_state_status_t fbe_mirror_generate_start(fbe_raid_siots_t * siots_p)
{
    fbe_raid_state_status_t state_status = FBE_RAID_STATE_STATUS_EXECUTING;
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_raid_iots_t        *iots_p = fbe_raid_siots_get_iots(siots_p);
    fbe_payload_block_operation_opcode_t opcode;
    fbe_raid_geometry_t    *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_bool_t              b_nested_siots = fbe_raid_siots_is_nested(siots_p);
    fbe_lba_t               metadata_start_lba;
    fbe_lba_t               metadata_copy_offset;

    /* We have already validated that the raid group is ready to accept I/O.
     * Also the FBE infrastructure has already translated the request from
     * logical to physical.
     */
    fbe_raid_siots_get_opcode(siots_p, &opcode);
    if (iots_p->callback == NULL)
    {
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "mirror: iots Callback is not set for iots: 0x%p\n",
                                            iots_p);
        return(state_status); 
    }
    /* First populate the geometry information.
     */
    if ( (status = fbe_mirror_get_geometry(raid_geometry_p,
                                           siots_p->start_lba,
                                           siots_p->xfer_count,
                                           &siots_p->geo)) != FBE_STATUS_OK )
    {
        /* There was an invalid parameter in the siots, return error now.
         */
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "mirror: generate for opcode: 0x%x get geometry failed. status: 0x%x\n",
                                            opcode, status);
        return(state_status); 
    }
    
    /*! @note This code currently doesn't support more than a single copy of
     *        metadata.  A I/O to an extent that contains more than (1) copy
     *        is signified my the request being in the metadata range and
     *        the metadata copy offset being valid.
     */
    fbe_raid_geometry_get_metadata_start_lba(raid_geometry_p, 
                                             &metadata_start_lba);
    fbe_raid_geometry_get_metadata_copy_offset(raid_geometry_p, 
                                               &metadata_copy_offset);
    if ((siots_p->start_lba >= metadata_start_lba) &&
        (metadata_copy_offset != FBE_LBA_INVALID)     )
    {
        /*! @note This code doesn't support more than (1) copy of the
         *        metadata.
         */
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "mirror: multiple copys of metadata not supported - metadata start: 0x%llX offset: 0x%llx\n",
                                            (unsigned long long)metadata_start_lba,
					    (unsigned long long)metadata_copy_offset);
        return(state_status);
    }

    /* Fill out the parity range (always physical!).
     */
    siots_p->parity_start = siots_p->start_lba;
    siots_p->parity_count = siots_p->xfer_count;

    /*! @note The mirror code now accepts unaligned (to 4K drive block size)
     *        requests.
     */

    /*************************************************************
     * Generate this request.
     *************************************************************
     * The generate function will:
     *   1) break off the exact extent of the SIOTS
     *   2) further initialize the SIOTS
     *   3) select an algorithm and set the state as appropriate 
     *************************************************************/
    switch (opcode)
    {
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ:
            /* If this is a nested read use the verify algorithms.
             */
            if (b_nested_siots)
            {
                status = fbe_mirror_generate_verify(siots_p);
            }
            else
            {
                status = fbe_mirror_generate_read(siots_p);
            }
            break;

        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_VERIFY:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_NONCACHED:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_WRITE:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_CORRUPT_DATA:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_REKEY_WRITE:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_REKEY_WRITE_ZEROS:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_ZEROS:
            /* If this is a nested write use the verify algorithms.
             */
            if (b_nested_siots)
            {
                status = fbe_mirror_generate_verify(siots_p);
            }
            else
            {
                status = fbe_mirror_generate_write(siots_p);
            }
            break;

        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_REBUILD:
            status = fbe_mirror_generate_rebuild(siots_p);
            break;

        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_READ_PAGED:
            status = fbe_mirror_generate_rekey(siots_p);
            break;

        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_WITH_BUFFER:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ_ONLY_VERIFY:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ERROR_VERIFY:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INCOMPLETE_WRITE_VERIFY:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_SYSTEM_VERIFY:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_SPECIFIC_AREA:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ_ONLY_VERIFY_SPECIFIC_AREA:
            status = fbe_mirror_generate_verify(siots_p);
            break;

        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ZERO:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_UNMARK_ZERO:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_REKEY_ZERO:
            status = fbe_mirror_generate_zero(siots_p);
            break;

        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_CHECK_ZEROED:
            status = fbe_mirror_generate_check_zeroed(siots_p);
            break;

        default:
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                        "mirror: generate unsupported opcode: 0x%x \n",
                                        opcode);
           fbe_raid_siots_transition(siots_p, fbe_raid_siots_invalid_opcode);
           return(state_status);
    }

    /* If everything is ok invoke the generic mirror generate siots
     * checking code.
     */
    if (status == FBE_STATUS_OK)
    {
        status = fbe_mirror_generate_validate_siots(siots_p);
    }

    /* If there is an error generating the request transition the siots
     * and report the failure (since this is unexpected).
     */
    if (status != FBE_STATUS_OK)
    {
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "mirror: generate for opcode: 0x%x failed. status: 0x%x\n",
                                            opcode, status);
    }

    /*! We use the parent siots for nested requests because the `is startable'
     *  method will check all children of the parent and all the parent siblings.
     *
     *  @note For mirror raid groups the only reason we split a siots is due to
     *        resource constraints.  Currently the only resource constraint is
     *        exceeding the maximum sg entries for a single request.  With 64
     *        block pages we allow an sg list of over 2,700 entries where each
     *        entry represents 64 blocks.  Thus a single siots needs to exceed
     *        over 172,000 blocks or greater than 84MB before we would split it.
     *        The siots locking is only required when splitting occurs.
     */
    state_status = fbe_raid_siots_generate_get_lock(siots_p);
    /* Always return the state status.
     */
    return(state_status);
}
/*********************************
 * end fbe_mirror_generate_start()
 *********************************/

/*!***************************************************************************
 *          fbe_mirror_generate_recovery_verify()
 *****************************************************************************
 *
 * @brief   Setup the siots for a recovery verify.  Since mirror verify only
 *          supports requests that are aligned to the optimal block size, we
 *          must expand (as required) the original request to be aligned.
 *          We configure the nested siots range and simply invoke the mirror 
 *          generate code which will treat the nested request like a verify request.
 *
 * @param   siots_p - Pointer to nested siots to generate
 * @param   b_is_recovery_verify_p - Pointer to bool that indicates of this is a 
 *                                   recovery verify or not.
 *
 * @return  status - Typically FBE_STATUS_OK
 *
 * @author
 *  04/28/2010  Ron Proulx  - Updated to support recovery verify
 *
 ****************************************************************/
fbe_status_t fbe_mirror_generate_recovery_verify(fbe_raid_siots_t *siots_p,
                                                 fbe_bool_t *b_is_recovery_verify_p)
{
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_payload_block_operation_opcode_t opcode;
    fbe_raid_siots_t       *parent_siots_p = fbe_raid_siots_get_parent(siots_p);
    fbe_raid_geometry_t    *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_lba_t               siots_end_lba;
    fbe_block_size_t        optimal_block_size;

    /* Generate will populate the algorithm but we need to determine
     * if this is a recovery verify or not.
     */
    fbe_raid_siots_get_opcode(siots_p, &opcode);
        
    /* The following nested opcodes are supported.
     */
    switch(opcode)
    {
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ:
            /* Request is `read recovery verify' thus it is a recovery verify.
             */
            *b_is_recovery_verify_p = FBE_TRUE;
            break;

        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_NONCACHED:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ZERO:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_CORRUPT_DATA:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_REKEY_WRITE:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_REKEY_WRITE_ZEROS:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_REKEY_ZERO:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_ZEROS:
            /* This is a read recovery verify for the pre-read portion of a 
             * write.  Thus this is a recovery verify.
             */
            *b_is_recovery_verify_p = FBE_TRUE;
            break;

        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_WRITE:
            /* This is a verify write (a.k.a. BVA) request from cache.
             * Thus this isn't a recovery verify.
             */
            *b_is_recovery_verify_p = FBE_FALSE;
            break;

        default:
            /* Opcode not supported, fail request.
             */
            fbe_raid_siots_trace(siots_p, 
                                 FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "mirror: generate nested verify opcode: 0x%x not supported. \n",
                                 opcode);
            return(FBE_STATUS_GENERIC_FAILURE);
    }

    /* Start by populating the request range to that of the parent.
     */        
    siots_p->start_lba = parent_siots_p->parity_start;
    siots_p->xfer_count = parent_siots_p->parity_count;
    siots_end_lba = siots_p->start_lba + siots_p->xfer_count - 1;

    /* Now update the request range as required to be aligned.
     */
    fbe_raid_geometry_get_optimal_size(raid_geometry_p, &optimal_block_size);
    fbe_raid_generate_aligned_request(&siots_p->start_lba,
                                      &siots_end_lba,
                                      optimal_block_size);
    siots_p->xfer_count = (fbe_block_count_t)(siots_end_lba - siots_p->start_lba + 1);

    /* Always return the status.
     */
    return(status);
}                               
/*******************************************
 * end fbe_mirror_generate_recovery_verify()
 *******************************************/


/********************************
 * end file fbe_mirror_generate.c
 ********************************/


