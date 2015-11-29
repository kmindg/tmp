/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_raid_siots.c
 ***************************************************************************
 *
 * @brief
 *  This file contains definitions for the raid sub iots structure.
 *
 * @version
 *   5/15/2009:  Created. rfoley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe_raid_library_proto.h"
#include "fbe/fbe_time.h"
#include "fbe_traffic_trace.h"

/*******************************
 * FORWARD FUNCTION DEFINITIONS
 *******************************/
/*!**************************************************************
 * fbe_raid_siots_destroy_resources()
 ****************************************************************
 * @brief
 *  Destroy (NOT free!) all the resources associated with the siots.
 *
 * @param  siots_p - The current I/O to free for.               
 *
 * @return fbe_status_t   
 *
 * @author
 *  8/6/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_siots_destroy_resources(fbe_raid_siots_t *siots_p)
{
    /* The destroy of fruts is simply a validation check that the fruts' packets are not in use.
     */
    if (RAID_DBG_ENABLED)
    {
        fbe_status_t        status = FBE_STATUS_OK;
    fbe_u16_t           total_fruts_destroyed = 0;
    /*! @note None of the `destroy' methods actually free any memory.
     *        `Destroy' simply places the object back to an un-used
     *        state.  This is needed for non-allocated structures and
     *        to catch cases where structures aren't initialized properly.
     */
    /* Destroy each of the fruts chains.
     */
        if (RAID_DBG_ENA(&siots_p->freed_fruts_head == siots_p->read_fruts_head.next))
    {
        /* Trace some failure information but we continue so that we don't
         * purposefully cause a memory leak.
         */
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "raid: siots_p: 0x%p &siots_p->freed_fruts_head 0x%p == siots_p->read_fruts_head.next 0x%p \n",
                             siots_p, &siots_p->freed_fruts_head, siots_p->read_fruts_head.next);
    }
        if (!fbe_queue_is_empty(&siots_p->read_fruts_head))
        {
            status = fbe_raid_fruts_chain_destroy(&siots_p->read_fruts_head, &total_fruts_destroyed);
    if (RAID_MEMORY_COND(status != FBE_STATUS_OK))
    {
        /* Trace some failure information but we continue so that we don't
         * purposefully cause a memory leak.
         */
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "raid: siots_p: 0x%p destroy read chain failed with status: 0x%x with: %d fruts destroyed\n",
                                     siots_p, status, total_fruts_destroyed);
    }
        }
        if (!fbe_queue_is_empty(&siots_p->read2_fruts_head))
        {
            status = fbe_raid_fruts_chain_destroy(&siots_p->read2_fruts_head, &total_fruts_destroyed);
    if (RAID_MEMORY_COND(status != FBE_STATUS_OK))
    {
        /* Trace some failure information but we continue so that we don't
         * purposefully cause a memory leak.
         */
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "raid: siots_p: 0x%p destroy read2 chain failed with status: 0x%x with: %d fruts destroyed\n",
                                     siots_p, status, total_fruts_destroyed);
    }
        }
        if (!fbe_queue_is_empty(&siots_p->write_fruts_head))
        {
            status = fbe_raid_fruts_chain_destroy(&siots_p->write_fruts_head, &total_fruts_destroyed);
    if (RAID_MEMORY_COND(status != FBE_STATUS_OK)) 
    { 
        /* Trace some failure information but we continue so that we don't
         * purposefully cause a memory leak.
         */
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "raid: siots_p: 0x%p destroy write chain failed with status: 0x%x with: %d fruts destroyed\n",
                                     siots_p, status, total_fruts_destroyed);
    }
        }

        if (!fbe_queue_is_empty(&siots_p->freed_fruts_head))
        {
            status = fbe_raid_fruts_chain_destroy(&siots_p->freed_fruts_head, &total_fruts_destroyed);
    if (RAID_MEMORY_COND(status != FBE_STATUS_OK)) 
    { 
        /* Trace some failure information but we continue so that we don't
         * purposefully cause a memory leak.
         */
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "raid: siots_p: 0x%p destroy freed chain failed with status: 0x%x with: %d fruts destroyed\n",
                                     siots_p, status, total_fruts_destroyed);
    }
        }
    /* Now validate that we have destroyed all the fruts that were allocated
     */
        if (RAID_COND(total_fruts_destroyed < siots_p->total_fruts_initialized))
    {
        /* If we have not destroyed all the fruts it means that we will leak
         * handles.  Trace some failure information but we continue so that 
         * we don't purposefully cause a memory leak.
         */
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "raid: siots_p: 0x%p total fruts destroyed: %d less than initialized: %d\n",
                             siots_p, total_fruts_destroyed, siots_p->total_fruts_initialized);
        }
    }
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_raid_siots_destroy_resources()
 ******************************************/

/*!**************************************************************
 * fbe_raid_siots_should_generate_next_siots()
 ****************************************************************
 * @brief
 *  Check to see if we should generate the next siots for this iots.
 *
 * @param  siots_p - The current I/O.               
 *
 * @notes After calling this function if it returns b_generate as
 *        FBE_TRUE, the caller must call either
 *        fbe_raid_iots_generate_next_siots_immediate() - In the normal case.
 *        or
 *        fbe_raid_iots_cancel_generate_next_siot() - If there is an error
 *        and we do not need to continue generating siots.
 * 
 * @return FBE_STATUS_OK on success.
 *         FBE_STATUS_GENERIC_FAILURE - on unexpected error.
 *
 * @author
 *  11/15/2010 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_raid_siots_should_generate_next_siots(fbe_raid_siots_t *siots_p,
                                                       fbe_bool_t *b_generate_p)
{
    fbe_raid_iots_t *iots_p = fbe_raid_siots_get_iots(siots_p);
    fbe_block_count_t iots_blocks_remaining;
    *b_generate_p = FBE_FALSE;

    /* If this siots already finished generating, don't kick off another siots.
     */
    if (fbe_raid_siots_is_flag_set(siots_p, FBE_RAID_SIOTS_FLAG_DONE_GENERATING))
    {
        return FBE_STATUS_OK;
    }
    if (fbe_raid_iots_is_flag_set(iots_p, FBE_RAID_IOTS_FLAG_GENERATE_DONE))
    {
        fbe_raid_siots_set_flag(siots_p, FBE_RAID_SIOTS_FLAG_DONE_GENERATING);
        return FBE_STATUS_OK;
    }
    fbe_raid_siots_set_flag(siots_p, FBE_RAID_SIOTS_FLAG_DONE_GENERATING);

    /* Don't pipeline nested siots
     */
    if (fbe_raid_siots_is_nested(siots_p))
    {
        return FBE_STATUS_OK;
    }

    /* only grab the lock if we are still generating. 
     * Iots that get generated into one siots clear the generating flag from 
     * the generate state. 
     *  
     * Iots that get generated into multiple requests still have this set.
     * (as long as we are not waiting for memory allocation)
     */
    fbe_raid_iots_lock(iots_p);
    if ( fbe_raid_iots_is_flag_set(iots_p, FBE_RAID_IOTS_FLAG_GENERATING)                                   &&
        !fbe_raid_common_is_flag_set(&iots_p->common, FBE_RAID_COMMON_FLAG_MEMORY_ALLOCATION_ERROR))
    {
        if (!fbe_raid_common_is_flag_set(&iots_p->common, FBE_RAID_COMMON_FLAG_MEMORY_ALLOCATION_COMPLETE) &&
            fbe_raid_common_is_flag_set(&iots_p->common, FBE_RAID_COMMON_FLAG_DEFERRED_MEMORY_ALLOCATION)) {
            
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "raid:  iots unexpected deferred 0x%x iots_p = 0x%p, siots_p = 0x%p\n",
                                 iots_p->common.flags, iots_p, siots_p);
        }
        fbe_raid_iots_get_blocks_remaining(iots_p, &iots_blocks_remaining);

        /* If we are here with generating set, there should be blocks left 
         * to break off. 
         */
        if(RAID_GENERATE_COND(iots_p->blocks_remaining == 0) )
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "raid: %s errored: iots_p->blocks_remaining 0x%llx == 0: iots_p = 0x%p, siots_p = 0x%p\n",
                                 __FUNCTION__,
                                 (unsigned long long)iots_p->blocks_remaining,
				 iots_p, siots_p);
            fbe_raid_iots_unlock(iots_p);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* Validate that the iots is still in the generate state
         */
        if (RAID_GENERATE_COND(fbe_raid_iots_get_state(iots_p) != fbe_raid_iots_generate_siots))
        {
            fbe_raid_siots_trace(siots_p,FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "raid: iots state: 0x%p isn't generate iots_p = 0x%p, siots_p = 0x%p\n",
                                 fbe_raid_iots_get_state(iots_p), iots_p, siots_p);
            fbe_raid_iots_unlock(iots_p);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /*! Kick off the iots if we need to.
         */
        if (iots_blocks_remaining > 0)
        {
            fbe_raid_iots_set_flag(iots_p, FBE_RAID_IOTS_FLAG_GENERATING_SIOTS);
            fbe_raid_iots_unlock(iots_p);
            *b_generate_p = FBE_TRUE;
            return FBE_STATUS_OK;
        }
    }
    fbe_raid_iots_unlock(iots_p);
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_raid_siots_should_generate_next_siots()
 ******************************************/

/*!**************************************************************
 * fbe_raid_iots_generate_next_siots_immediate()
 ****************************************************************
 * @brief
 *  Generate the next siots for this iots.
 *  We should only be here if we called fbe_raid_siots_should_generate_next_siots()
 *  and it returned FBE_TRUE for the b_generate_p
 *
 * @param  iots_p - The current I/O.               
 *
 * @return FBE_STATUS_OK on success.
 *         FBE_STATUS_GENERIC_FAILURE - on unexpected error.
 *
 * @author
 *  11/15/2010 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_raid_iots_generate_next_siots_immediate(fbe_raid_iots_t *iots_p)
{
    fbe_block_count_t iots_blocks_remaining;
    if (fbe_raid_iots_is_flag_set(iots_p, FBE_RAID_IOTS_FLAG_GENERATE_DONE))
    {
        fbe_raid_iots_trace(iots_p, FBE_RAID_IOTS_TRACE_PARAMS_ERROR, 
                            "raid: %s generate done set iots_p = 0x%p fl: 0x%x\n",
                            __FUNCTION__, iots_p, iots_p->flags);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* We should only be callin this function if we called fbe_raid_siots_should_generate_next_siots(), 
     * and it returned FBE_TRUE for b_generate_p.  When it returned FBE_TRUE it should
     * have set this flag If this flag is not set then complain.
     */
    if (!fbe_raid_iots_is_flag_set(iots_p, FBE_RAID_IOTS_FLAG_GENERATING_SIOTS))
    {
        fbe_raid_iots_trace(iots_p, FBE_RAID_IOTS_TRACE_PARAMS_ERROR, 
                            "raid: %s generate done set iots_p = 0x%p fl: 0x%x\n",
                            __FUNCTION__, iots_p, iots_p->flags);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_raid_iots_lock(iots_p);
    fbe_raid_iots_get_blocks_remaining(iots_p, &iots_blocks_remaining);

    /* If we are here with generating set, there should be blocks left 
     * to break off. 
     */
    if (RAID_GENERATE_COND(iots_p->blocks_remaining == 0) )
    {
        fbe_raid_iots_clear_flag(iots_p, FBE_RAID_IOTS_FLAG_GENERATING_SIOTS);
        fbe_raid_iots_trace(iots_p, FBE_RAID_IOTS_TRACE_PARAMS_ERROR, 
                            "raid: %s errored: iots_p->blocks_remaining 0x%llx == 0: iots_p = 0x%p\n",
                            __FUNCTION__,
			    (unsigned long long)iots_p->blocks_remaining, iots_p);
        fbe_raid_iots_unlock(iots_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Validate that the iots is still in the generate state
     */
    if (RAID_GENERATE_COND(fbe_raid_iots_get_state(iots_p) != fbe_raid_iots_generate_siots))
    {
        fbe_raid_iots_clear_flag(iots_p, FBE_RAID_IOTS_FLAG_GENERATING_SIOTS);
        fbe_raid_iots_trace(iots_p, FBE_RAID_IOTS_TRACE_PARAMS_ERROR, 
                            "raid: iots state: 0x%p isn't generate iots_p = 0x%p\n",
                            fbe_raid_iots_get_state(iots_p), iots_p);
        fbe_raid_iots_unlock(iots_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*! Kick off the iots if we need to.
     */
    if (iots_blocks_remaining > 0)
    {
        fbe_raid_iots_unlock(iots_p);
        fbe_raid_common_state((fbe_raid_common_t *)iots_p);
    }
    else
    {
        fbe_raid_iots_clear_flag(iots_p, FBE_RAID_IOTS_FLAG_GENERATING_SIOTS);
        fbe_raid_iots_trace(iots_p, FBE_RAID_IOTS_TRACE_PARAMS_ERROR, 
                            "raid: generate next siots blocks remaining is zero iots_p = 0x%p\n",
                            iots_p);
        fbe_raid_iots_unlock(iots_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_raid_iots_generate_next_siots_immediate()
 ******************************************/

/*!**************************************************************
 * fbe_raid_iots_cancel_generate_next_siots()
 ****************************************************************
 * @brief
 *  An error has occurred so that we are not going
 *  to generate additional siots.  Clear out the
 *  generating flag so we do not wait for the iots to
 *  generate more siots before completing the iots.
 *
 * @param  iots_p - The current I/O.               
 *
 * @return FBE_STATUS_OK on success.
 *         FBE_STATUS_GENERIC_FAILURE - on unexpected error.
 *
 * @author
 *  11/16/2010 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_raid_iots_cancel_generate_next_siots(fbe_raid_iots_t *iots_p)
{
    /* Simply clear the flag so that we do not hold up completion of the iots.
     */
    fbe_raid_iots_lock(iots_p);
    fbe_raid_iots_clear_flag(iots_p, FBE_RAID_IOTS_FLAG_GENERATING_SIOTS);
    fbe_raid_iots_unlock(iots_p);
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_raid_iots_cancel_generate_next_siots()
 ******************************************/

/*!**************************************************************
 * fbe_raid_siots_setup_sgd()
 ****************************************************************
 * @brief
 *  Perform the default setup for an sg descriptor where the data is coming from the cache.
 *
 * @param  siots_p - The current request.
 * @param sgd_p - Scatter gather descriptor with sg ptr and offset.
 *
 * @return fbe_status_t.   
 *
 * @author
 *  3/1/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_siots_setup_sgd(fbe_raid_siots_t *siots_p, 
                              fbe_sg_element_with_offset_t *sgd_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_payload_block_operation_opcode_t opcode;
    fbe_raid_siots_get_opcode(siots_p, &opcode);

    if ((opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ZERO) ||
        (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_ZEROS) ||
        (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_REKEY_WRITE_ZEROS))
    {
        /* Use the same sg for all the data of this operation. 
         * To do this we setup the sg element with offset to 
         * use a special "get next" function which simply keeps using 
         * the same sg rather than incrementing to the next sg. 
         * When we call fbe_raid_scatter_sg_to_bed() below, it will use this one 
         * sg for all the data it scatters. 
         */
        fbe_sg_element_t *zero_sg_p = NULL;
        fbe_raid_memory_get_zero_sg(&zero_sg_p);
        fbe_sg_element_with_offset_init(sgd_p, 0, zero_sg_p, fbe_sg_element_with_offset_get_current_sg); 
    }
    else if (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_SAME)
    {
        /*!@note, although write same is not supported yet, the general idea is to 
         * allocate an sg and buffer and replicate the ws pattern into that buffer. 
         * Write sames will then point to a given buffer with the write same pattern 
         * repeated. 
         * In this case the "get next sg" function will always return the same buffer. 
         * We will need to pass a write same sg to the function below instead of NULL. 
         */
        fbe_sg_element_with_offset_init(sgd_p, 0, NULL, fbe_sg_element_with_offset_get_current_sg); 
    }
    else
    {
        /*
         * We've already got the primary data in the S/G list 
         * supplied with the data. We will use these locations
         * directly, i.e. no explicit "fetch" will be performed.
         *
         * However, we must still produce the set of S/G lists
         * which partition this new data by destination disk.
         *
         * The sg_count_vec[] is filled with a count per position.
         * The totals from sg_count_vec[] are placed on the
         * sg_id_list[] below.
         */

        /* Setup sg descriptor with cache data.
         */
        status = fbe_raid_siots_setup_cached_sgd(siots_p, sgd_p);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "raid: failed to setup cache sgd: status = 0x%x, siots_p = 0x%p\n",
                                 status, siots_p);
            return status;
        }
    }
    return status;
}
/******************************************
 * end fbe_raid_siots_setup_sgd()
 ******************************************/
/*!**************************************************************
 * fbe_raid_siots_setup_cached_sgd()
 ****************************************************************
 * @brief
 *  Perform the default setup for an sg descriptor where the data is coming from the cache.
 *
 * @param  siots_p - The current request.
 * @param sgd_p - Scatter gather descriptor with sg ptr and offset.
 *
 * @return None.   
 *
 * @author
 *  8/5/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_siots_setup_cached_sgd(fbe_raid_siots_t *siots_p, fbe_sg_element_with_offset_t *sgd_p)
{
    fbe_raid_iots_t *iots_p = fbe_raid_siots_get_iots(siots_p);
    fbe_lba_t iots_lba;
    fbe_block_count_t host_start_offset;
    fbe_lba_t block_offset;
    fbe_sg_element_t *sg_p = NULL;
    fbe_u32_t offset = 0;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_bool_t b_is_buffered = fbe_raid_iots_is_buffered_op(iots_p);

    /* We should be here only if it is a cached I/O.
     */
    if(RAID_COND(b_is_buffered))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "raid: siots_p 0x%p is buffered operation\n",
                             siots_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Calculate the offset in blocks into the sg from the client.
     * This offset is the siots offset into this overall transfer, plus any initial offset 
     * the client is sending. 
     */
    fbe_raid_iots_get_current_op_lba(iots_p, &iots_lba);
    fbe_raid_iots_get_host_start_offset(iots_p, &host_start_offset);
    block_offset = siots_p->start_lba - iots_lba + host_start_offset;

    fbe_raid_iots_get_sg_ptr(iots_p, &sg_p); 

    if (RAID_COND((block_offset * FBE_BE_BYTES_PER_BLOCK) > FBE_U32_MAX))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "raid: siots_p 0x%p : byte count: 0x%llx is exceeding 32bit limit\n",
                             siots_p, (unsigned long long)(block_offset * FBE_BE_BYTES_PER_BLOCK));
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Calculate the offset in bytes.
     */
    offset = (fbe_u32_t)(block_offset * FBE_BE_BYTES_PER_BLOCK);

    /* Initialize our sg descriptor.
     */
    fbe_sg_element_with_offset_init(sgd_p, offset, sg_p, NULL /* do not specify sg. */ );
    return status;
}
/******************************************
 * end fbe_raid_siots_setup_cached_sgd()
 ******************************************/
/*!**************************************************************
 * fbe_raid_siots_init_memory_fields()
 ****************************************************************
 * @brief
 *  Initialize fields in siots related to memory allocation.
 *
 * @param  siots_p - The siots.
 *
 * @return None.
 *
 * @author
 *  8/06/2009 - Created. rfoley
 *
 ****************************************************************/

void fbe_raid_siots_init_memory_fields(fbe_raid_siots_t *siots_p)
{
    /* Initialize the resource information
     */
    siots_p->ctrl_page_size = 0;
    siots_p->data_page_size = 0;
    siots_p->total_blocks_to_allocate = 0;
    siots_p->num_ctrl_pages = 0;
    siots_p->num_data_pages = 0;
    if (RAID_DBG_ENABLED)
    {
        siots_p->total_fruts_initialized = 0;
    }

    /* The api to the memory service expects the memory request to
     * be in certain state.  Initialize it here.
     */
    fbe_memory_request_init(fbe_raid_siots_get_memory_request(siots_p));
    fbe_raid_common_set_memory_request_fields_from_parent(&siots_p->common);

    return;
}
/**************************************
 * end fbe_raid_siots_init_memory_fields()
 **************************************/

/*!***************************************************************************
 *          fbe_raid_siots_set_optimal_page_size()
 *****************************************************************************
 * @brief
 *  Set the optimal page size based on:
 *      o Is this a nested siots?
 *      o Number of fruts to allocate
 *      o Number of blocks to allocate
 *
 * @param  siots_p - The siots for this request
 * @param  num_fruts_to_allocate - Number of fruts to allocate
 * @param  num_of_blocks_to_allocate - Number of blocks to allocate 
 *
 * @return  status - Typically FBE_STATUS_OK
 *
 ****************************************************************/
fbe_status_t fbe_raid_siots_set_optimal_page_size(fbe_raid_siots_t *siots_p,
                                                  fbe_u16_t num_fruts_to_allocate,
                                                  fbe_block_count_t num_of_blocks_to_allocate)
{
    fbe_status_t    status = FBE_STATUS_OK;

    /* If this is a nested siots we MUST use the parent pages size since
     * methods such as fbe_raid_sg_clip_sg_list, etc, have not way of knowing
     * the source and/or destination buffer size.  Instead the source buffer is
     * always used.  Thus if there is a mis-match in page size the destination
     * sg list maybe half the length that is expected (for instance if the sgl
     * was allocated assuming 64 block buffers but the parent sgl was setup
     * using 32 block buffers).
     */
    if ( fbe_raid_siots_is_nested(siots_p)     &&
        !fbe_raid_siots_is_bva_verify(siots_p)    )
    {
        fbe_raid_siots_t   *parent_siots_p = fbe_raid_siots_get_parent(siots_p);

        /* Validate the parent page size.
         */
        if (!fbe_raid_memory_validate_page_size(parent_siots_p->ctrl_page_size))
        {
            fbe_raid_siots_trace(siots_p, 
                                 FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "raid: parent page_size: 0x%x is invalid \n", 
                                 parent_siots_p->ctrl_page_size);
            status = FBE_STATUS_GENERIC_FAILURE;
        }
        else
        {
            /* Else set the page size to the parent value.
             */
            fbe_raid_siots_set_page_size(siots_p, parent_siots_p->ctrl_page_size);
            fbe_raid_siots_set_data_page_size(siots_p, parent_siots_p->data_page_size);
        }
    }
    else
    {
        /* Else set the page size based in the number of fruts to allocate
         * and the number of blocks to allocate.
         */

        if (num_of_blocks_to_allocate > FBE_U32_MAX)
        {
            fbe_raid_siots_trace(siots_p, 
                                 FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "raid: SIOTS 0x%p, blocks to allocate exceeding 32bit limit 0x%llx\n", 
                                 siots_p,  (unsigned long long)num_of_blocks_to_allocate);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        fbe_raid_siots_set_data_page_size(siots_p, 
                                          fbe_raid_memory_calculate_data_page_size(siots_p,
                                                                                   (fbe_u16_t)num_of_blocks_to_allocate));
        fbe_raid_siots_set_page_size(siots_p, 
                                     fbe_raid_memory_calculate_ctrl_page_size(siots_p,
                                                                              (fbe_u16_t)num_fruts_to_allocate));
    }

    /* Always return the status.
     */
    return(status);
}
/********************************************
 * end fbe_raid_siots_set_optimal_page_size()
 ********************************************/

/*!**************************************************************
 * fbe_raid_siots_destroy()
 ****************************************************************
 * @brief
 *  Destroy the siots by destroying the resources in the siots.
 *
 * @param  siots_p - The current I/O to free for.               
 *
 * @return fbe_status_t   
 *
 * @author
 *  11/2/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_siots_destroy(fbe_raid_siots_t *siots_p)
{
    fbe_raid_siots_t *nest_siots_p = NULL;
    fbe_raid_siots_t *next_nest_siots_p = NULL;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_raid_siots_transition(siots_p, fbe_raid_siots_freed);

    /* Check all the nested sub-iots and destroy if needed.
     */
    nest_siots_p = (fbe_raid_siots_t *) fbe_queue_front(&siots_p->nsiots_q);
    while (nest_siots_p != NULL)
    {
        next_nest_siots_p = fbe_raid_siots_get_next(nest_siots_p);
        status = fbe_raid_siots_destroy_resources(nest_siots_p);
        if (status != FBE_STATUS_OK)
        {
            fbe_raid_siots_trace(siots_p, 
                                 FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "raid: %s failed to free resources of nest_siots_p 0x%p\n",
                                 __FUNCTION__,
                                 nest_siots_p);
        }
        status = fbe_raid_siots_destroy(nest_siots_p);
        if (status != FBE_STATUS_OK)
        {
            fbe_raid_siots_trace(siots_p, 
                                 FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "raid: %s failed to destroy nest_siots_p 0x%p\n",
                                 __FUNCTION__,
                                 nest_siots_p);
        }
        nest_siots_p = next_nest_siots_p;
    }

    fbe_queue_destroy(&siots_p->nsiots_q);
    fbe_queue_destroy(&siots_p->read_fruts_head);
    fbe_queue_destroy(&siots_p->read2_fruts_head);
    fbe_queue_destroy(&siots_p->write_fruts_head);
    fbe_queue_destroy(&siots_p->freed_fruts_head);

    /* Mark the eboard and invalid
     */
    //fbe_xor_destroy_eboard(fbe_raid_siots_get_eboard(siots_p));
    
    if (!RAID_DBG_MEMORY_ENABLED)
    {
        fbe_memory_request_t *memory_request_p = fbe_raid_siots_get_memory_request(siots_p);
        if (fbe_memory_request_is_in_use(memory_request_p))
        {
            //fbe_memory_ptr_t        memory_ptr = NULL;
            //fbe_memory_request_get_entry_mark_free(memory_request_p, &memory_ptr);
            //fbe_memory_free_entry(memory_ptr);
			fbe_memory_free_request_entry(memory_request_p);
			return FBE_STATUS_OK;
        }
    }
    else
    {
        /* In debug mode we will do additional checks when we free.
         */
        status = fbe_raid_siots_free_memory(siots_p);
    if (RAID_MEMORY_COND(status != FBE_STATUS_OK))
    {
        /*! @note In this case we definitely leaked memory
         */
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_CRITICAL, 
                             "raid: siots_p: 0x%p free memory failed with status: 0x%x \n",
                             siots_p, status);
        return status;
    }
    }

    /* Now destroy any items that `free memory' relied on.
     */
    fbe_raid_common_clear_flag(&siots_p->common, FBE_RAID_COMMON_FLAG_TYPE_SIOTS);

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_raid_siots_destroy()
 ******************************************/

/*!**************************************************************
 * fbe_raid_siots_reuse()
 ****************************************************************
 * @brief
 *  Re-init the siots by freeing all its resources in preparation
 *  for re-starting this stios.
 *
 * @param  siots_p - The current I/O to free for.               
 *
 * @return fbe_status_t   
 *
 * @author
 *  9/8/2011 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_siots_reuse(fbe_raid_siots_t *siots_p)
{
    fbe_raid_siots_t *nest_siots_p = NULL;
    fbe_raid_siots_t *next_nest_siots_p = NULL;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_bool_t is_siots_embedded = FBE_FALSE;

    /* Check all the nested sub-iots and destroy if needed.
     */
    nest_siots_p = (fbe_raid_siots_t *) fbe_queue_front(&siots_p->nsiots_q);
    while (nest_siots_p != NULL)
    {
        next_nest_siots_p = fbe_raid_siots_get_next(nest_siots_p);
        status = fbe_raid_siots_destroy_resources(nest_siots_p);
        if (RAID_COND(status != FBE_STATUS_OK))
        {
            fbe_raid_siots_trace(siots_p, 
                                 FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "raid: %s failed to free resources of nest_siots_p 0x%p\n",
                                 __FUNCTION__,
                                 nest_siots_p);
        }
        status = fbe_raid_siots_destroy(nest_siots_p);
        if (RAID_COND(status != FBE_STATUS_OK))
        {
            fbe_raid_siots_trace(siots_p, 
                                 FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "raid: %s failed to destroy nest_siots_p 0x%p\n",
                                 __FUNCTION__,
                                 nest_siots_p);
        }
        nest_siots_p = next_nest_siots_p;
    }

    /* Destroy the resources since we are starting over.
     */
    status = fbe_raid_siots_destroy_resources(siots_p);
    if (RAID_COND(status != FBE_STATUS_OK))
    {
        /* We failed to free siots resources.
         */
        fbe_raid_siots_object_trace(siots_p, FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                    "siots_p: 0x%p failed to free siots resources iots_p = 0x%p status: 0x%x\n",
                                    siots_p, fbe_raid_siots_get_iots(siots_p), status);
    }
    /* Re-init all our queues.
     */
    fbe_queue_destroy(&siots_p->nsiots_q);
    fbe_queue_destroy(&siots_p->read_fruts_head);
    fbe_queue_destroy(&siots_p->read2_fruts_head);
    fbe_queue_destroy(&siots_p->write_fruts_head);
    fbe_queue_destroy(&siots_p->freed_fruts_head);

    fbe_raid_siots_set_error_validation_callback(siots_p, NULL);
    fbe_raid_siots_set_block_status(siots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID);
    fbe_raid_siots_set_block_qualifier(siots_p, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID);
    siots_p->continue_bitmask = 0;
    siots_p->needs_continue_bitmask = 0; 
    is_siots_embedded = fbe_raid_siots_is_flag_set(siots_p, FBE_RAID_SIOTS_FLAG_EMBEDDED_SIOTS);
    fbe_raid_siots_init_flags(siots_p, FBE_RAID_SIOTS_FLAG_INVALID);
    /* This flag is only set at allocate time, we need to preserve it across reuse */
    if (is_siots_embedded == FBE_TRUE)
    {
        fbe_raid_siots_set_flag(siots_p, FBE_RAID_SIOTS_FLAG_EMBEDDED_SIOTS);
    }
    fbe_queue_init(&siots_p->read_fruts_head);
    fbe_queue_init(&siots_p->read2_fruts_head);
    fbe_queue_init(&siots_p->write_fruts_head);
    fbe_queue_init(&siots_p->freed_fruts_head);
    fbe_raid_siots_set_journal_slot_id(siots_p, FBE_RAID_INVALID_JOURNAL_SLOT);
    fbe_queue_element_init(&siots_p->journal_q_elem);
    siots_p->vrts_ptr = NULL;
    siots_p->vcts_ptr = NULL;
    fbe_raid_siots_set_nested_siots(siots_p, NULL);
    siots_p->media_error_count = 0;
    siots_p->wait_count = 0;
    siots_p->misc_ts.cxts.eboard.hard_media_err_bitmap = 0;
    siots_p->total_fruts_initialized = 0;

    /*! @note A siots no longer frees itself.  If this siots was allocated
     *        it is upto the iots destroy to free the siots.
     *        Now free any resources allocated with this siots
     */
    status = fbe_raid_siots_free_memory(siots_p);
    if (RAID_COND(status != FBE_STATUS_OK))
    {
        /*! @note In this case we definitely leaked memory
         */
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "raid: siots_p: 0x%p free memory failed with status: 0x%x iots_p: 0x%p \n",
                             siots_p, status, fbe_raid_siots_get_iots(siots_p));
        return status;
    }

    fbe_raid_siots_init_memory_fields(siots_p);

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_raid_siots_reuse()
 ******************************************/
/*!**************************************************************
 *          fbe_raid_siots_get_raid_group_offset()
 ****************************************************************
 *
 * @brief   Get the raid group offset 
 *
 * @param   siots_p - The siots.
 * @param   raid_group_offset_p - Address of raid group offset to 
 *                                populate
 *
 * @return  status  - Typically FBE_STATUS_OK
 *
 ****************************************************************/
fbe_status_t fbe_raid_siots_get_raid_group_offset(fbe_raid_siots_t *siots_p,
                                                  fbe_lba_t *raid_group_offset_p)
{
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_block_edge_t    *block_edge_p = NULL;

    if (fbe_raid_geometry_no_object(raid_geometry_p))
    {
        /* If there is no object, the raw mirror library (without an object) needs to apply the offset from the raid
         * geometry since there is not offset on the downstream edge. 
         */
        fbe_raid_geometry_get_raw_mirror_offset(raid_geometry_p, raid_group_offset_p);
        return FBE_STATUS_OK;
    }
    /*! @note Assumption is that all positions have the same offset so use
     *        position 0.
     */
    fbe_raid_geometry_get_block_edge(raid_geometry_p, &block_edge_p, 0);

    /* If the block edge is NULL then something is wrong
     */
    if (RAID_GENERATE_COND(block_edge_p == NULL)) 
    {
        /* Trace some information
         */
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "raid: %s raid geometry: 0x%p block edge is NULL \n",
                             __FUNCTION__, raid_geometry_p);
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    /* Set the raid group offset value.  If the `ignore offset' attribute is
     * set the offset is 0.
     */
    if ((block_edge_p->base_edge.path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_IGNORE_OFFSET) != 0)
    {
        *raid_group_offset_p = 0;
    }
    else
    {
        *raid_group_offset_p = fbe_block_transport_edge_get_offset(block_edge_p);
    }

    /* Always return execution status 
     */
    return (FBE_STATUS_OK);
} 

/*!**************************************************************
 * fbe_raid_siots_common_init()
 ****************************************************************
 * @brief
 *  Initialize a raid sub iots structure.  This function contains
 *  functionality common to both embedded and allocated siots.
 *
 * @param  siots_p - The siots.
 * @param  common_p - The parent iots.
 *
 * @return fbe_status_t.
 *
 * @author
 *  5/18/2009 - Created. rfoley
 *
 ****************************************************************/

fbe_status_t fbe_raid_siots_common_init(fbe_raid_siots_t *siots_p,
                                fbe_raid_common_t* common_p)
{
    fbe_status_t        status = FBE_STATUS_OK;

    fbe_raid_common_init(&siots_p->common);
    fbe_raid_common_set_parent(&siots_p->common, (fbe_raid_common_t*)common_p);
    fbe_raid_common_init_magic_number(&siots_p->common, FBE_MAGIC_NUMBER_SIOTS_REQUEST);

    fbe_raid_siots_set_error_validation_callback(siots_p, NULL);

    /*! Init fields we get from the raid group object.
     */

    /*! Mark the time when we were initialized.
     */
    siots_p->time_stamp = fbe_get_time();

    siots_p->state_index = 0;
    siots_p->algorithm = RG_NO_ALG;

    fbe_raid_siots_set_block_status(siots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID);
    fbe_raid_siots_set_block_qualifier(siots_p, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID);
    siots_p->continue_bitmask = 0;
    siots_p->needs_continue_bitmask = 0; 
    fbe_raid_siots_init_flags(siots_p, FBE_RAID_SIOTS_FLAG_INVALID);
    fbe_queue_init(&siots_p->read_fruts_head);
    fbe_queue_init(&siots_p->read2_fruts_head);
    fbe_queue_init(&siots_p->write_fruts_head);
    fbe_queue_init(&siots_p->freed_fruts_head);
    fbe_raid_siots_set_journal_slot_id(siots_p, FBE_RAID_INVALID_JOURNAL_SLOT);
    fbe_queue_element_init(&siots_p->journal_q_elem);
    //fbe_spinlock_init(&siots_p->lock);
    //csx_p_dump_native_string("%s: %p\n", __FUNCTION__ , &siots_p->lock.lock);
    //csx_p_spl_create_nid_always(CSX_MY_MODULE_CONTEXT(), &siots_p->lock.lock, CSX_NULL);
    
    siots_p->vrts_ptr = NULL;
    siots_p->vcts_ptr = NULL;
    fbe_raid_siots_set_nested_siots(siots_p, NULL);
    siots_p->parity_pos = 0;
    fbe_raid_siots_init_dead_positions(siots_p);

    /* Initialize error fields that are maintained for the life of the siots.
     */
    siots_p->media_error_count = 0;
    siots_p->wait_count = 0;

    /* There is code that expects this to be zeroed.
     */
    siots_p->misc_ts.cxts.eboard.hard_media_err_bitmap = 0;

    fbe_queue_init(&siots_p->nsiots_q);

    return status;
}
/******************************************
 * end fbe_raid_siots_common_init()
 ******************************************/

/*!**************************************************************
 * fbe_raid_siots_allocated_consume_init()
 ****************************************************************
 * @brief
 *  Initialize a raid sub iots structure that is not embedded,
 *  but rather was allocated from a pool.
 *
 * @param  siots_p - The siots that we just allocated.
 * @param  iots_p  - The parent iots.
 *
 * @return fbe_status_t.
 *
 ****************************************************************/

fbe_status_t fbe_raid_siots_allocated_consume_init(fbe_raid_siots_t *siots_p,
                                                   fbe_raid_iots_t *iots_p)
{        
    fbe_status_t            status = FBE_STATUS_OK;

    /* Zero the entire siots
     */
    fbe_zero_memory((void *)siots_p, sizeof(*siots_p));

    /* Initialize any common fields.
     */
    status = fbe_raid_siots_common_init(siots_p, &iots_p->common);
    if (RAID_MEMORY_COND(status != FBE_STATUS_OK))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "raid: failed to perform initializatio for siots_p = 0x%p, status = status 0x%x != FBE_STATUS_OK, while initialzation of siots common",
                             siots_p, status);
        return status;
    }
    
    fbe_raid_common_init_flags(&siots_p->common, FBE_RAID_COMMON_FLAG_TYPE_SIOTS);

    /* Now initialize the memory fields
     */
    fbe_raid_siots_init_memory_fields(siots_p);

    /* Put any allocated specific initialization here.
     */

    /*! @note We will complete the initialization of this siots in
     *        fbe_raid_siots_allocated_activate_init.
     */
    return status;
}
/********************************************
 * end fbe_raid_siots_allocated_consume_init()
 ********************************************/

/*!**************************************************************
 * fbe_raid_siots_allocated_activate_init()
 ****************************************************************
 * @brief
 *  Initialize a raid sub iots structure that is not embedded,
 *  but rather was allocated from a pool.  We have `started' to
 *  initialize it but we can't complete the initialization until
 *  it is actually `activate' (i.e. placed on hte active queue).
 *
 * @param  siots_p - The siots that we just allocated.
 * @param  iots_p  - The parent iots.
 *
 * @return fbe_status_t.
 *
 ****************************************************************/

fbe_status_t fbe_raid_siots_allocated_activate_init(fbe_raid_siots_t *siots_p,
                                                    fbe_raid_iots_t *iots_p)
{        
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_raid_geometry_t    *raid_geometry_p = fbe_raid_iots_get_raid_geometry(iots_p);
    fbe_lba_t               raid_group_offset;
    fbe_raid_common_state_t generate_state;
    fbe_lun_performance_counters_t *perf_stats_p = NULL;

    status = fbe_raid_geometry_get_raid_group_offset(raid_geometry_p, &raid_group_offset);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "raid: failed to get raid group offset: status = 0x%x; siots_p = 0x%p\n",
                             status, siots_p);
        return status;
    }


    fbe_raid_iots_get_performance_stats(iots_p, &perf_stats_p);
    if (fbe_traffic_trace_is_enabled(KTRC_TFBE_RG_FRU) ||
        fbe_traffic_trace_is_enabled(KTRC_TFBE_RG) ||
        ((raid_geometry_p->debug_flags & (~FBE_RAID_LIBRARY_DEBUG_FLAG_DISABLE_WRITE_LOGGING)) != 0) ||
        (perf_stats_p != NULL))
    {
        fbe_raid_siots_set_flag(siots_p, FBE_RAID_SIOTS_FLAG_LOGGING_ENABLED);
    }

    /* Initialize the eboard since it is used for all operations. 
     */
    fbe_xor_init_eboard(fbe_raid_siots_get_eboard(siots_p),
                        fbe_raid_geometry_get_object_id(raid_geometry_p),
                        raid_group_offset);

    /*! Set the correct generate state from the object.
     */
    fbe_raid_geometry_get_generate_state(raid_geometry_p, &generate_state);
    if(RAID_COND(generate_state == NULL) )
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "raid: generate_state is NULL, siots_p = 0x%p\n",
                             siots_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_raid_siots_transition(siots_p, (fbe_raid_siots_state_t)generate_state);

    siots_p->xfer_count = iots_p->blocks_remaining;
    siots_p->start_lba = iots_p->current_lba;
    return status;
}
/*********************************************
 * end fbe_raid_siots_allocated_activate_init()
 *********************************************/

/*!**************************************************************
 * fbe_raid_nest_siots_init()
 ****************************************************************
 * @brief
 *  Initialize a raid sub iots structure that is not embedded,
 *  but rather was allocated from a pool.
 *
 * @param  siots_p - The siots that we just allocated.
 * @param  parent_siots_p  - The parent.
 *
 * @return fbe_status_t.
 *
 ****************************************************************/

fbe_status_t fbe_raid_nest_siots_init(fbe_raid_siots_t *siots_p,
                                      fbe_raid_siots_t *parent_siots_p)
{
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_raid_geometry_t *raid_geometry_p = NULL;
    fbe_lba_t           raid_group_offset;
    fbe_lun_performance_counters_t *perf_stats_p = NULL;
    fbe_raid_iots_t *iots_p = NULL;

    /* Zero the entire siots
     */
    fbe_zero_memory((void *)siots_p, sizeof(*siots_p));

    /* Initialize any common fields.
     */
    status = fbe_raid_siots_common_init(siots_p, &parent_siots_p->common);
    if(RAID_GENERATE_COND(status != FBE_STATUS_OK))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "raid: failed to initialize siots common field. status 0x%x, siots_p 0x%p\n",
                             status, siots_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Nested specific initialization.
     */
    fbe_raid_common_set_flag(&siots_p->common, FBE_RAID_COMMON_FLAG_TYPE_NEST_SIOTS);

    /* Now initialize the memory fields
     */
    fbe_raid_siots_init_memory_fields(siots_p);

    /*! @note The dead positions are copied from the parent since the parent may
     *        have encountered a new dead position.  But the raid group monitor
     *        should have acknowledged the dead position and updated rebuild
     *        logging bitmask accordingly.  In any case the assumption is that
     *        degraded positions will be re-evaluated as necessary.
     */
    status = fbe_raid_siots_dead_pos_set(siots_p, FBE_RAID_FIRST_DEAD_POS, 
                                fbe_raid_siots_dead_pos_get( parent_siots_p, FBE_RAID_FIRST_DEAD_POS ));
    if(RAID_GENERATE_COND(status != FBE_STATUS_OK))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "raid: failed to set -first- dead position: status = 0x%x, siots_p = 0x%p\n",
                             status, siots_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    status = fbe_raid_siots_dead_pos_set(siots_p, FBE_RAID_SECOND_DEAD_POS, 
                                fbe_raid_siots_dead_pos_get( parent_siots_p, FBE_RAID_SECOND_DEAD_POS ));
    if(RAID_GENERATE_COND(status != FBE_STATUS_OK)) 
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "raid: failed to set -second- dead position: status = 0x%x, siots_p = 0x%p\n",
                             status, siots_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Initialize the eboard since it is used for all operations.  Set the
     * raid group object id and offset correctly.
     */
    raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    status = fbe_raid_geometry_get_raid_group_offset(raid_geometry_p, &raid_group_offset);
    if(RAID_GENERATE_COND(status != FBE_STATUS_OK)) 
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "raid: failed to get raid group offset: status = 0x%x, siots_p = 0x%p\n",
                             status, siots_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_xor_init_eboard(fbe_raid_siots_get_eboard(siots_p),
                        fbe_raid_geometry_get_object_id(raid_geometry_p),
                        raid_group_offset);

    iots_p = fbe_raid_siots_get_iots(siots_p);
    fbe_raid_iots_get_performance_stats(iots_p, &perf_stats_p);

    if (fbe_traffic_trace_is_enabled(KTRC_TFBE_RG_FRU) ||
        fbe_traffic_trace_is_enabled(KTRC_TFBE_RG) ||
        ((raid_geometry_p->debug_flags & (~FBE_RAID_LIBRARY_DEBUG_FLAG_DISABLE_WRITE_LOGGING)) != 0) ||
        (perf_stats_p != NULL))
    {
        fbe_raid_siots_set_flag(siots_p, FBE_RAID_SIOTS_FLAG_LOGGING_ENABLED);
    }

    return status;
}
/**************************************
 * end fbe_raid_nest_siots_init()
 **************************************/

/*!**************************************************************
 * fbe_raid_siots_embedded_init()
 ****************************************************************
 * @brief
 *  Initialize a raid sub iots structure that embedded, and
 *  thus does not need to get freed.
 *
 * @param  siots_p - The siots.
 * @param  iots_p  - The parent iots.
 *
 * @return fbe_status_t.
 *
 ****************************************************************/

fbe_status_t fbe_raid_siots_embedded_init(fbe_raid_siots_t *siots_p,
                                   fbe_raid_iots_t *iots_p)
{
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_raid_geometry_t    *raid_geometry_p = fbe_raid_iots_get_raid_geometry(iots_p);
    fbe_lba_t               raid_group_offset;
    fbe_raid_common_state_t generate_state;
    fbe_lun_performance_counters_t *perf_stats_p = NULL;

    /* Validate that `embedded in-use' isn't set.
     */
    if(RAID_GENERATE_COND(fbe_raid_iots_is_flag_set(iots_p, FBE_RAID_IOTS_FLAG_EMBEDDED_SIOTS_IN_USE)))
    {
        fbe_raid_iots_trace(iots_p, 
                            FBE_RAID_IOTS_TRACE_PARAMS_ERROR, 
                            "raid: Embedded In-Use: 0x%x already set.\n",
                            FBE_RAID_IOTS_FLAG_EMBEDDED_SIOTS_IN_USE);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Validate that the siots has been marked as embedded.
     */
    if (!fbe_raid_siots_is_flag_set(siots_p, FBE_RAID_SIOTS_FLAG_EMBEDDED_SIOTS))
    {
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "raid: Embedded Siots Flag: 0x%x not set.\n",
                             FBE_RAID_SIOTS_FLAG_EMBEDDED_SIOTS);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Now flag the embedded siots `in-use'.
     */
    fbe_raid_iots_set_flag(iots_p, FBE_RAID_IOTS_FLAG_EMBEDDED_SIOTS_IN_USE);

    /* Put any embedded specific initialization here.
     */
    fbe_raid_iots_clear_flag(iots_p, FBE_RAID_IOTS_FLAG_ALLOCATING_SIOTS); 

    /* Initialize any common fields.
     */
    status = fbe_raid_siots_common_init(siots_p, &iots_p->common);
    if(RAID_GENERATE_COND(status != FBE_STATUS_OK))
    {
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "raid: failed to initialize siots common field. status 0x%x, siots_p 0x%p\n",
                             status, siots_p);

        /* We are here but pretty sure that caller passed us an siots. 
         * So, we will re-set siots's flag irrespective of kind of 
         * initialization error we got. It may give a chance to caller 
         * to recover from it.
         */
        fbe_raid_common_set_flag(&siots_p->common, FBE_RAID_COMMON_FLAG_TYPE_SIOTS);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Put any allocated specific initialization here.
     */
    fbe_raid_common_set_flag(&siots_p->common, FBE_RAID_COMMON_FLAG_TYPE_SIOTS);
    fbe_raid_siots_set_flag(siots_p, FBE_RAID_SIOTS_FLAG_EMBEDDED_SIOTS);

    /* Now initialize the memory fields
     */
    fbe_raid_siots_init_memory_fields(siots_p);

    /* Set the starting range for this siots from the parent iots
     */
    siots_p->xfer_count = iots_p->blocks_remaining;
    siots_p->start_lba = iots_p->current_lba;

    /*! @note Currently the raid library unit tests do not populate the raid
     *        which is normally required.  To work-around this we skip the
     *        portion of the setup which requires the raid geometry if the
     *        `Test' flag is set in the iots.
     */
    if (fbe_raid_iots_is_flag_set(iots_p, FBE_RAID_IOTS_FLAG_RAID_LIBRARY_TEST))
    {
        return status;
    }

    /*! @note Put all initialization that requires the raid geometry below
     *        this point.
     */
    status = fbe_raid_geometry_get_raid_group_offset(raid_geometry_p, &raid_group_offset);

    if (RAID_COND_STATUS_GEO(raid_geometry_p, (status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "raid: failed to get raid group offset: status = 0x%x; siots_p = 0x%p\n",
                             status, siots_p);
        return status;
    }
    fbe_raid_iots_get_performance_stats(iots_p, &perf_stats_p);

    if (fbe_traffic_trace_is_enabled(KTRC_TFBE_RG_FRU) ||
        fbe_traffic_trace_is_enabled(KTRC_TFBE_RG) ||
        ((raid_geometry_p->debug_flags & (~FBE_RAID_LIBRARY_DEBUG_FLAG_DISABLE_WRITE_LOGGING)) != 0) ||
        (perf_stats_p != NULL))
    {
        fbe_raid_siots_set_flag(siots_p, FBE_RAID_SIOTS_FLAG_LOGGING_ENABLED);
    }
    /* Initialize the eboard since it is used for all operations.
     */
    fbe_xor_init_eboard(fbe_raid_siots_get_eboard(siots_p),
                        fbe_raid_geometry_get_object_id(raid_geometry_p),
                        raid_group_offset);

    /*! Set the correct generate state from the object.
     */
    fbe_raid_geometry_get_generate_state(raid_geometry_p, &generate_state);
    if(RAID_GENERATE_COND(generate_state == NULL) )
    {
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "raid: generate_state is NULL, siots_p = 0x%p\n",
                             siots_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_raid_siots_transition(siots_p, (fbe_raid_siots_state_t)generate_state);

    return status;
}
/******************************************
 * end fbe_raid_siots_embedded_init()
 ******************************************/
/****************************************************************
 * fbe_raid_siots_parity_rebuild_pos_get()
 ****************************************************************
 * @brief
 *  Return the parity and rebuild positions in a vector.
 *  We calculate both rebuild and parity positions as 'physical'
 *  positions and place them into the passed in vectors.
 *  
 * @param siots_p - Current sub request.
 * @param parity_pos_p - Vector of 2 parity positions.
 * @param rebuild_pos_p - Vector of 2 rebuild positions.
 *
 * @return
 *  None, but the rebuild and parity positions are setup in
 *  the input vector.
 *
 * @author
 *  04/13/2006 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_siots_parity_rebuild_pos_get(fbe_raid_siots_t *siots_p,
                                           fbe_u16_t *parity_pos_p,
                                           fbe_u16_t *rebuild_pos_p)
{
    fbe_u16_t rb_pos_1 = fbe_raid_siots_dead_pos_get(siots_p, FBE_RAID_FIRST_DEAD_POS);

    /* Setup the first parity position and rebuild position.
     */ 
    parity_pos_p[0] = fbe_raid_siots_get_row_parity_pos(siots_p);

    /* We must have a valid parity position. Trace the error
     * and return from here.
     */
    if (RAID_COND(parity_pos_p[0] == FBE_RAID_INVALID_PARITY_POSITION))
    {
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: got unexpected width of raid group. parity_pos_p[0] = 0x%x, siots_p = 0x%p\n",
                             parity_pos_p[0], siots_p);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (rb_pos_1 == FBE_RAID_INVALID_DEAD_POS)
    {
        /* If we don't have a 1st rebuild position, then set the
         * rebuild position to the XOR Driver invalid indicator.
         */
        rebuild_pos_p[0] = FBE_XOR_INV_DRV_POS;
    }
    else
    {
        rebuild_pos_p[0] = rb_pos_1;
    }
    
    if (fbe_raid_siots_parity_num_positions(siots_p) == 2)
    {
        /* For Raid 6 we need to setup the 2nd dead position
         * and the 2nd parity position.
         */
        fbe_u16_t rb_pos_2 = fbe_raid_siots_dead_pos_get(siots_p, FBE_RAID_SECOND_DEAD_POS);
        parity_pos_p[1] = fbe_raid_siots_dp_pos(siots_p);
        
        if (rb_pos_2 == FBE_RAID_INVALID_DEAD_POS)
        {
            /* If we don't have a 2nd rebuild position, then set the
             * rebuild position to the XOR Driver invalid indicator.
             */
            rebuild_pos_p[1] = FBE_XOR_INV_DRV_POS;
        }
        else
        {
            rebuild_pos_p[1] = rb_pos_2;
        }
    }
    else
    {
        /* R5/R3 do not have a second parity or dead position.
         * Set these positions in the vector to the
         * XOR Driver invalid indicator.
         */
        parity_pos_p[1] = FBE_XOR_INV_DRV_POS;
        rebuild_pos_p[1] = FBE_XOR_INV_DRV_POS;
    }
    return FBE_STATUS_OK;
}
/* end fbe_raid_siots_parity_rebuild_pos_get() */

/*****************************************************************************
 *          fbe_raid_siots_add_degraded_pos()
 *****************************************************************************
 *
 * @brief   This function add a new degraded position to the siots.  This
 *          function is either invoked by the fruts error processing when a
 *          `dead' error is detected or my the raid code when it is has 
 *          determine that a position is degraded.
 *   
 *
 * @param siots_p - The current SIOTS to setup degraded 
 *                  positions.
 * @param new_deg_pos - The new degraded position to add to this 
 *                     siots.
 *
 * @return  status - Typically FBE_STATUS_OK
 *                   Other - Adding degraded position exceeded the
 *                           number allowed
 *
 * @author
 *  06/12/06 - Created. Rob Foley
 *
 *****************************************************************************/
fbe_status_t fbe_raid_siots_add_degraded_pos(fbe_raid_siots_t *const siots_p,
                                             fbe_u32_t new_deg_pos )
{
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_u32_t           first_dead_pos = fbe_raid_siots_dead_pos_get(siots_p, FBE_RAID_FIRST_DEAD_POS);
    fbe_u32_t           second_dead_pos = fbe_raid_siots_dead_pos_get(siots_p, FBE_RAID_SECOND_DEAD_POS);
    fbe_raid_position_bitmask_t cur_dead_bitmask = 0;
    fbe_u32_t           cur_dead_positions;
    fbe_u32_t           max_dead_positions_before_broken = 0;

    /* The new dead position cannot be invalid.
     */
    if (new_deg_pos == FBE_RAID_INVALID_DEAD_POS)
    {
        /* Trace the error and return.
         */
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "raid: Invalid dead. siots: 0x%llx 0x%llx Invalid dead. new: 0x%x dead: 0x%x dead_pos_2: 0x%x \n",
                             (unsigned long long)siots_p->start_lba,
			     (unsigned long long)siots_p->xfer_count, 
                             new_deg_pos, first_dead_pos, second_dead_pos);
        return(FBE_STATUS_GENERIC_FAILURE);

    }

    /* Get the current dead bitmask and remove the new position.
     * (It is legal to add a position more that once).
     */
    fbe_raid_siots_get_degraded_bitmask(siots_p, &cur_dead_bitmask);
    cur_dead_bitmask &= ~(1 << new_deg_pos);
    cur_dead_positions = fbe_raid_get_bitcount(cur_dead_bitmask);

    /* Previously we wouldn't allow the degraded position to get added for
     * mirror rebuilds, but there might be an alternate position since we
     * are always rebuild logging.  Thus remove this restriction and cleanup
     * this code to handle n-way mirror etc (simply invoke the geometry method
     * that tells us tells use the maximum number of dead positions allowed).
     */
    status = fbe_raid_geometry_get_max_dead_positions(raid_geometry_p, 
                                                      &max_dead_positions_before_broken);
    
    /* Now validate that it is ok to add a new position.  If there is only (1) dead
     * position allowed, we over-write the first dead position (i.e. the orignal
     * dead position is lost!!)
     *
     * We can exceed the maximum number of dead positions allowed (before the
     * raid group is shutdown) when an I/O hits a newly removed drive.  
     * Although we fail the request (since we have exceeded the maximum dead 
     * positions allowed without shutting down the raid group) most callers 
     * ignore the return status.
     */     
    if ((status != FBE_STATUS_OK) ||
        (max_dead_positions_before_broken < 1)  ||
        (max_dead_positions_before_broken > 2)     )
    {
        /* Trace a warning
         */
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_WARNING,
                             "raid: siots: 0x%llx 0x%llx exceeded max dead positions: 0x%x width: 0x%x status: 0x%x\n",
                             siots_p->start_lba, siots_p->xfer_count, 
                             (unsigned int)max_dead_positions_before_broken, (unsigned int)fbe_raid_siots_get_width(siots_p), (unsigned int)status);
        return(FBE_STATUS_GENERIC_FAILURE);
    }
    
    /* Make sure either the first and 2nd are invalid or
     * they are not the same.  We want to prevent cases where both
     * first and second are the same valid dead position.
     */
    if (((first_dead_pos  != FBE_RAID_INVALID_DEAD_POS)  &&
         (second_dead_pos != FBE_RAID_INVALID_DEAD_POS)      ) &&
        (first_dead_pos   == second_dead_pos)                     )
    {
        /* Trace an error and report a failed status.  Notice that we don't
         * transition to unexpected since it is up to the invoking routine to do
         * that.
         */
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "raid: siots: 0x%llx 0x%llx unexpected dead: 0x%x dead_pos_2: 0x%x \n",
                             (unsigned long long)siots_p->start_lba,
                             (unsigned long long)siots_p->xfer_count, 
                             siots_p->dead_pos, siots_p->dead_pos_2);
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    /* If only 1 dead position is allowed validate that the second position isn't
     * dead and then over-write the first dead position.
     */
    if (max_dead_positions_before_broken == 1)
    {
        if (cur_dead_positions > 1)
        {
            /* Trace a warning and fail the `add dead' request.
             */
            fbe_raid_siots_trace(siots_p, 
                                 FBE_RAID_LIBRARY_TRACE_PARAMS_WARNING,
                                 "raid: add degraded pos: %d current first: %d second: %d exceeds max degraded: %d\n",
                                 new_deg_pos, first_dead_pos, second_dead_pos, max_dead_positions_before_broken);
            return(FBE_STATUS_GENERIC_FAILURE);
        }

        /* If there are already any valid dead positions (not including this one)
         * we are going broken.
         */
        if (cur_dead_positions > 0)
        {
            /* Although not required trace a warning and continue.
             */
            fbe_raid_siots_trace(siots_p, 
                                 FBE_RAID_LIBRARY_TRACE_PARAMS_WARNING,
                                 "raid: add degraded pos: %d current first: %d second: %d exceeds max degraded: %d\n",
                                 new_deg_pos, first_dead_pos, second_dead_pos, max_dead_positions_before_broken);
           
            /* Continue.
             */
        }

        /* Set the first dead position
         */
        status = fbe_raid_siots_dead_pos_set(siots_p, 
                                             FBE_RAID_FIRST_DEAD_POS, 
                                             new_deg_pos);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "raid: failed to set siots dead position: status 0x%x, siots_p = 0x%p\n",
                                 status, siots_p);
            return status;
        }

    }
    else
    {
        /*! Else 2 dead positions are allowed.
         *
         *  @note This code has been designed for parity groups where the
         *        position value has significance.  But for mirror raid groups
         *        the location (first dead vs second dead) of the positions
         *        doesn't matter so it's ok to use the parity algorithms.
         */

        /* If both dead positions are consumed (and not by this position) the
         * raid group is going broken.  This scenario can occur when an I/O
         * hits a newly removed drive.  Although we fail the request (since
         * there can only be (2) dead positions marked) most callers ignore
         * the return status.
         */
        if (cur_dead_positions > 1)
        {
            /* Trace a warning and fail the `add dead' request.
             */
            fbe_raid_siots_trace(siots_p, 
                                 FBE_RAID_LIBRARY_TRACE_PARAMS_WARNING,
                                 "raid: add degraded pos: %d current first: %d second: %d exceeds max degraded: %d\n",
                                 new_deg_pos, first_dead_pos, second_dead_pos, max_dead_positions_before_broken);
            return(FBE_STATUS_GENERIC_FAILURE);
        }

        /* It is allowed to mark the same position dead multiple times.
         */
        if (fbe_raid_siots_pos_is_dead(siots_p, new_deg_pos))
        {
            /* This position is already accounted for in our dead positions.
             * We don't need to setup our dead positions.
             */
        }
        else if (first_dead_pos == FBE_RAID_INVALID_DEAD_POS)
        {
            /* Second dead position better be invalid.
             */
            if (second_dead_pos != FBE_RAID_INVALID_DEAD_POS)
            {
                fbe_raid_siots_trace(siots_p, 
                                     FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                     "raid: Invalid dead. siots: 0x%llx 0x%llx Invalid dead. new: 0x%x dead: 0x%x dead_pos_2: 0x%x \n",
                                     (unsigned long long)siots_p->start_lba,
				     (unsigned long long)siots_p->xfer_count, 
                                     new_deg_pos, first_dead_pos, second_dead_pos);
                return(FBE_STATUS_GENERIC_FAILURE);
            }
            
            /* Set the first dead position in the siots.
             */
            status = fbe_raid_siots_dead_pos_set(siots_p, 
                                        FBE_RAID_FIRST_DEAD_POS, 
                                        new_deg_pos);
            if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
            {
                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                     "raid: failed to set siots dead position: status = 0x%x, siots_p = 0x%p\n",
                                     status, siots_p);
                return FBE_STATUS_GENERIC_FAILURE;
            }
        }
        else 
        {
            /* Else the second position better be invalid.
             */
            if (second_dead_pos != FBE_RAID_INVALID_DEAD_POS)
            {
                fbe_raid_siots_trace(siots_p, 
                                     FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                     "raid: Invalid dead. siots: 0x%llx 0x%llx Invalid dead. new: 0x%x dead: 0x%x dead_pos_2: 0x%x \n",
                                     (unsigned long long)siots_p->start_lba,
				     (unsigned long long)siots_p->xfer_count, 
                                     new_deg_pos, first_dead_pos, second_dead_pos);
                return(FBE_STATUS_GENERIC_FAILURE);
            }
            
            /* If the second dead position is invalid determine where to
             * place (first or second) the new dead position.
             */
            if ( first_dead_pos > new_deg_pos )
            {
                /* Move first down to second dead and put the new 
                 * degraded position into the first dead position.
                 */
                status = fbe_raid_siots_dead_pos_set(siots_p, 
                                            FBE_RAID_FIRST_DEAD_POS, 
                                            new_deg_pos);
                if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
                {
                    fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                         "raid: failed to set -first- dead position: status = 0x%x, siots_p = 0x%p\n",
                                         status, siots_p);
                    return status;
                }
                status = fbe_raid_siots_dead_pos_set(siots_p, 
                                            FBE_RAID_SECOND_DEAD_POS, 
                                            first_dead_pos);
                if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
                {
                    fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                         "raid: failed to set -second- dead position: status = 0x%x, siots_p = 0x%p\n",
                                         status, siots_p);
                    return status;
                }
            }
            else
            {
                /* Set the second dead position since the first 
                 * dead position is less than the new position.
                 */
                status = fbe_raid_siots_dead_pos_set(siots_p, 
                                            FBE_RAID_SECOND_DEAD_POS, 
                                            new_deg_pos);
                if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
                {
                    fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                         "raid: failed to set -second- dead position: status = 0x%x, siots_p = 0x%p\n",
                                         status, siots_p);
                    return FBE_STATUS_GENERIC_FAILURE;
                }
            }
        
        } /* end else if the first position is in use */
    
    } /* end else 2 dead positions are allowed */

    /* Always return the execution status.
     */
    return(status);
}
/* end fbe_raid_siots_add_degraded_pos() */

/*!***************************************************************************
 *          fbe_raid_siots_remove_degraded_pos()
 *****************************************************************************
 * 
 * @brief   Clear (`mark invalid') any dead position that contained this was
 *          flagged as this position.  If this was the first dead position
 *          and there was a second dead position, the second dead position
 *          is promoted to the first.
 *          Although the drive might still be accessible, it cannot be used
 *          for this request because the area requested is completely degraded.
 *
 * @param   siots_p - Pointer to siots to clear position dead for
 * @param   position - Position to clear from dead positions
 *
 * @return  status - Typically FBE_STATUS_OK
 *
 * @author
 *  12/01/2009  Ron Proulx  - Created.
 *
 ****************************************************************************/
fbe_status_t fbe_raid_siots_remove_degraded_pos(fbe_raid_siots_t *siots_p,
                                                fbe_u16_t position)

{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_u32_t       width = fbe_raid_siots_get_width(siots_p);

    /* Validate that the position passed is valid.
     */
    if ((position == FBE_RAID_INVALID_DEAD_POS) ||
        (position >= width)                        )
    {
        /* Invalidate position passed.
         */
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "raid: clear dead position: 0x%x is invalid. \n",
                             position);
        return(FBE_STATUS_GENERIC_FAILURE);

    }

    /* If this is the first dead position check if we need to move the
     * second into the first.
     */
    if (fbe_raid_siots_dead_pos_get(siots_p, FBE_RAID_FIRST_DEAD_POS) == position)
    {
        /* Validate that the second dead position isn't the same as the first.
         */
        if (fbe_raid_siots_dead_pos_get(siots_p, FBE_RAID_SECOND_DEAD_POS) == position)
        {
            /* We don't allow both dead positions to be the same.
             */
            fbe_raid_siots_trace(siots_p, 
                                 FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "raid: set dead position: 0x%x is set for both dead positions. \n",
                                 position);
            status = FBE_STATUS_GENERIC_FAILURE;
        }
        else if (fbe_raid_siots_dead_pos_get(siots_p, FBE_RAID_SECOND_DEAD_POS) != FBE_RAID_INVALID_DEAD_POS)
        {
            /* Need to move the second dead position in the first and invalidate
             * the second (notice the order).
             */
            status = fbe_raid_siots_dead_pos_set(siots_p, 
                                        FBE_RAID_FIRST_DEAD_POS, 
                                        fbe_raid_siots_dead_pos_get(siots_p, FBE_RAID_SECOND_DEAD_POS));
            if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
            {
                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                     "raid: failed to set -first- dead position: status = 0x%x, siots_p = 0x%p\n",
                                     status, siots_p);
                return status;
            }
            status = fbe_raid_siots_dead_pos_set(siots_p, FBE_RAID_SECOND_DEAD_POS, FBE_RAID_INVALID_DEAD_POS); 
            if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
            {
                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                     "raid: failed to set -second- dead position: status = 0x%x, siots_p = 0x%p\n",
                                     status, siots_p);
                return status;
            }
        }
        else
        {
            /* Else clear the first dead position.
             */
            status = fbe_raid_siots_dead_pos_set(siots_p, FBE_RAID_FIRST_DEAD_POS, FBE_RAID_INVALID_DEAD_POS);
            if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
            {
                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                     "raid: failed to set -first- dead position: status = 0x%x, siots_p = 0x%p\n",
                                     status, siots_p);
                return status;
            }
        }
    }
    else if (fbe_raid_siots_dead_pos_get(siots_p, FBE_RAID_SECOND_DEAD_POS) == position)
    {
        /* Validate that the first position isn't set to the same position.
         */
        if (fbe_raid_siots_dead_pos_get(siots_p, FBE_RAID_FIRST_DEAD_POS) == position)
        {
            /* We don't allow both dead positions to be the same.
             */
            fbe_raid_siots_trace(siots_p, 
                                 FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "raid: set dead position: 0x%x is set for both dead positions. \n",
                                 position);
            status = FBE_STATUS_GENERIC_FAILURE;
        }
        else
        {
            /* Else clear the second dead position.
             */
            status = fbe_raid_siots_dead_pos_set(siots_p, FBE_RAID_SECOND_DEAD_POS, FBE_RAID_INVALID_DEAD_POS);
            if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
            {
                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                     "raid: failed to set -second- dead position: status = 0x%x, siots_p = 0x%p\n",
                                     status, siots_p);
                return status;
            }
        }
    }

    /* Always return the execution status.
     */
    return(status);
}
/* end of fbe_raid_siots_remove_degraded_pos() */

/*!***************************************************************
 * fbe_raid_siots_dead_pos_set()
 ****************************************************************
 * @brief
 *  Set the dead position into the siots dead position fields.
 *
 * @param siots_p - Ptr to siots.
 * @param position - Position to get dead pos for.
 *                  Either RG_FIRST_DEAD_POS or RG_SECOND_DEAD_POS.
 * @param new_value -  The new value to set in the dead position.
 *
 * @return fbe_status_t.
 *
 ****************************************************************/

fbe_status_t fbe_raid_siots_dead_pos_set( fbe_raid_siots_t *const siots_p,
                                  fbe_raid_degraded_position_t position,
                                  fbe_raid_position_t new_value )
{
    /* These two variables are strictly for debugging
     * in checked builds when the final assert fails.
     */
    fbe_raid_position_t CSX_MAYBE_UNUSED old_dead_pos = siots_p->dead_pos;
    fbe_raid_position_t CSX_MAYBE_UNUSED old_dead_pos_2 = siots_p->dead_pos_2;

    /* Based on the input position, we set a different dead position
     * within the siots structure.
     */
    if (position == FBE_RAID_FIRST_DEAD_POS)
    {
        siots_p->dead_pos = new_value;
    }
    else
    {
        /* Make sure it is second dead pos,
         * as we don't support anything other than 1st and 2nd.
         */
        if(RAID_GENERATE_COND( position != FBE_RAID_SECOND_DEAD_POS ))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "raid: position 0x%x != FBE_RAID_SECOND_DEAD_POS 0x%x; siots_p 0x%p\n",
                                 position, FBE_RAID_SECOND_DEAD_POS, siots_p);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* Set the 2nd dead position to the input value.
         */
        siots_p->dead_pos_2 = new_value;
    }

    /* @note Theoretically we could make sure either the first and 2nd are invalid or they are not the same.
     * We want to prevent cases where both first and second are the same valid dead position.
     */

    return FBE_STATUS_OK;
}
/* END fbe_raid_siots_dead_pos_set */

/*!***************************************************************************
 *          fbe_raid_siots_update_degraded()
 *****************************************************************************
 * 
 * @brief   Since the drive state may have changed (to good) from the time an
 *          I/O detected it was dead, we need ot remove any degraded positions
 *          that have returned.
 *
 * @param   siots_p - Pointer to siots to clear position dead for
 * @param   returning_bitmask - Bitmask of returning positions
 *
 * @return  status - Typically FBE_STATUS_OK
 *
 ****************************************************************************/
fbe_status_t fbe_raid_siots_update_degraded(fbe_raid_siots_t *siots_p,
                                            fbe_raid_position_bitmask_t returning_bitmask)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_u32_t       width = fbe_raid_siots_get_width(siots_p);
    fbe_raid_position_t position;

    /* Walk thru the `returning drive' bitmask and removed them from the
     * degraded bitmask.
     */
    for (position = 0; 
         (position < width) && (status == FBE_STATUS_OK); 
         position++)
    {
        if ((1 << position) & returning_bitmask)
        {
            status = fbe_raid_siots_remove_degraded_pos(siots_p, position);
        }
    }

    /* Always return the status
     */
    return(status);
}
/* end of fbe_raid_siots_update_degraded() */

/*!***************************************************************************
 *          fbe_raid_siots_consume_nested_siots()
 *****************************************************************************
 *
 * @brief   The method simply `consumes' the memory that was allocated for a
 *          nested siots.  It does NOT initialize the memory at all!!
 *
 * @param   siots_p    - ptr to the fbe_raid_siots_t
 * @param   memory_info_p - Pointer to memory request information
 *
 * @return  status - Typically FBE_STATUS_OK
 *
 * @note    As the description states, this methods does NOT initialize the
 *          the nested siots.  This saves the overhead of doing so for a
 *          structure that is rarely used (i.e. only required if read requires
 *          recovery verify)
 *
 * @author
 *  10/14/2010  Ron Proulx  - Created
 *
 *****************************************************************************/

fbe_status_t fbe_raid_siots_consume_nested_siots(fbe_raid_siots_t *siots_p,
                                                 fbe_raid_memory_info_t *memory_info_p)
{
    fbe_raid_siots_t         *nested_siots_p = NULL;
    fbe_u32_t                 bytes_allocated;
    fbe_status_t              status = FBE_STATUS_OK;

    /* Validate siots.
     */
    if(RAID_MEMORY_COND(siots_p == NULL))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "siots_p is NULL\n");
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get the next piece of memory from the current location in the memory queue.
     */
    nested_siots_p = fbe_raid_memory_allocate_contiguous(memory_info_p,
                                                         sizeof(fbe_raid_siots_t),
                                                         &bytes_allocated);
    if(RAID_MEMORY_COND(nested_siots_p == NULL) ||
                        (bytes_allocated != sizeof(fbe_raid_siots_t)) )
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "raid: nested_siots_p is NULL or bytes_allocated 0x%x != sizeof(fbe_raid_siots_t) 0x%llx; "
                             "siots_p = 0x%p\n",
                             bytes_allocated, (unsigned long long)sizeof(fbe_raid_siots_t), siots_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Validate that the parent siots hasn't planted a nested siots
     */
    if (RAID_MEMORY_COND(fbe_raid_siots_get_nested_siots(siots_p) != NULL))
    {
        fbe_raid_siots_t    *already_nested_siots_p = fbe_raid_siots_get_nested_siots(siots_p);

        fbe_raid_siots_get_nest_queue_head(siots_p, &already_nested_siots_p);
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "raid: consume nested parent nested ptr isn't NULL: 0x%p lba: 0x%llx blks: 0x%llx\n",
                             already_nested_siots_p,
			     (unsigned long long)already_nested_siots_p->start_lba,
			     (unsigned long long)already_nested_siots_p->xfer_count);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Validate that the parent siots nested queue is empty
     */
    if (RAID_MEMORY_COND(!fbe_queue_is_empty(&siots_p->nsiots_q)))
    {
        fbe_raid_siots_t    *already_nested_siots_p;

        fbe_raid_siots_get_nest_queue_head(siots_p, &already_nested_siots_p);
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "raid: consume nested parent siots queue isn't empty: 0x%p lba: 0x%llx blks: 0x%llx\n",
                             already_nested_siots_p,
			     (unsigned long long)already_nested_siots_p->start_lba,
			     (unsigned long long)already_nested_siots_p->xfer_count);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*! @note The only thing we set is to set the type to invalid
     */
    fbe_raid_siots_init_flags(nested_siots_p, FBE_RAID_SIOTS_FLAG_INVALID);

    /* Now attach it to the parent siots.
     */
    fbe_raid_siots_set_nested_siots(siots_p, nested_siots_p);

    return status;
}
/**********************************************
 * end of fbe_raid_siots_consume_nested_siots()
 **********************************************/

/****************************************************************
 *          fbe_raid_siots_init_vrts()
 ****************************************************************
 * @brief
 *  This function will initalize the entire vrts.
 *  We assume the memory_queue_head has at least one vrts id.
 *
 * @param   siots_p    - ptr to the fbe_raid_siots_t
 * @param   memory_info_p - Pointer to memory request information
 *
 * @return  fbe_status_t
 *
 * @author
 *  05/19/2009  Ron Proulx - Created
 *
 ****************************************************************/

fbe_status_t fbe_raid_siots_init_vrts(fbe_raid_siots_t *siots_p,
                                      fbe_raid_memory_info_t *memory_info_p)
{
    fbe_raid_vrts_t          *vrts_p = NULL;
    fbe_u32_t                 bytes_allocated;
    fbe_status_t              status = FBE_STATUS_OK;
    /* Validate siots.
     */
    if(RAID_COND(siots_p == NULL) )
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                        "siots_p %p == NULL\n",
                                        siots_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get the next piece of memory from the current location in the memory queue.
     */
    vrts_p = fbe_raid_memory_allocate_contiguous(memory_info_p,
                                                 sizeof(fbe_raid_vrts_t),
                                                 &bytes_allocated);
    if(RAID_COND((vrts_p == NULL) ||
                 (bytes_allocated != sizeof(fbe_raid_vrts_t)) ) )
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "vrts_p %p == NULL or bytes_allocated 0x%x != sizeof(fbe_raid_vrts_t) 0x%llx \n",
                             vrts_p, bytes_allocated, (unsigned long long)sizeof(fbe_raid_vrts_t));
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* Get the newly allocated vrts and initialize it.
     */
    fbe_zero_memory(vrts_p, sizeof(*vrts_p));

    /* Now attach it to the siots.
     */
    siots_p->vrts_ptr = vrts_p;

    return status;
}
/*****************************************
 * end of fbe_raid_siots_init_vrts()
 *****************************************/

/*!***************************************************************
 * fbe_raid_siots_init_vcts()
 ****************************************************************
 * @brief
 *  This function will initalize the entire vctsts.
 *  We assume the memory_queue_head has at least one vrts id.
 *
 * @param   siots_p    - ptr to the fbe_raid_siots_t
 * @param   memory_info_p - Pointer to memory request information
 *
 * @return     - fbe_status_t
 *
 * @author
 *  9/14/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_siots_init_vcts(fbe_raid_siots_t *siots_p,
                                      fbe_raid_memory_info_t *memory_info_p)
{
    fbe_raid_vcts_t          *vcts_p = NULL;
    fbe_u32_t                 bytes_allocated;
    fbe_status_t             status = FBE_STATUS_OK;

    /* Validate siots.
     */
    if(RAID_COND(siots_p == NULL))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "siots_p is NULL\n");
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get the next piece of memory from the current location in the memory queue.
     */
    vcts_p = fbe_raid_memory_allocate_contiguous(memory_info_p,
                                                 sizeof(fbe_raid_vcts_t),
                                                 &bytes_allocated);
    if(RAID_COND(vcts_p == NULL) ||
                (bytes_allocated != sizeof(fbe_raid_vcts_t)) )
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "raid: vcts_p is NULL or bytes_allocated 0x%x != sizeof(fbe_raid_vcts_t) 0x%llx; "
                             "siots_p = 0x%p\n",
                             bytes_allocated, (unsigned long long)sizeof(fbe_raid_vcts_t), siots_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get the newly allocated vrts and initialize it.
     */
    fbe_zero_memory(vcts_p, sizeof(fbe_raid_vcts_t));

    /* Now attach it to the siots.
     */
    siots_p->vcts_ptr = vcts_p;

    return status;
}
/*****************************************
 * end of fbe_raid_siots_init_vcts()
 *****************************************/
/****************************************************************
 * fbe_raid_siots_init_vcts_curr_pass()
 ****************************************************************
 * @brief
 *  This function will initalize the current pass of the vcts
 *  to all zeros.
 *
 * @param siots_p - ptr to the RAID_SUB_IOTS.
 *
 * @returns:
 *  None.
 *
 * @author
 *  02/08/06 - Created. RPF
 *
 ****************************************************************/

void fbe_raid_siots_init_vcts_curr_pass(fbe_raid_siots_t * const siots_p)
{
    if ( siots_p->vcts_ptr != NULL )
    {
        /* If we have a vcts_ptr then clear out the current pass.
         */
        fbe_zero_memory( (void *) &siots_p->vcts_ptr->curr_pass_eboard, 
                         sizeof(fbe_raid_verify_error_counts_t) );

        /* Also zero out the error regions.
         */
        fbe_zero_memory((void *)&siots_p->vcts_ptr->error_regions, 
                        sizeof(fbe_xor_error_regions_t));

    }
    return;
}
/**********************************************
 * end of fbe_raid_siots_init_vcts_curr_pass()
 **********************************************/

/*!***************************************************************
 * @fn      rg_siots_rd_retry ()
 ****************************************************************
 * 
 * @brief   Simply issue a READ for the positions in error in the
 *          passed siots.
 *
 * @param   siots_p    - ptr to the fbe_raid_siots_t
 * @param   retry_fruts_count - Number of fruts that require a retry
 * @param   fruts_to_retry_bitmap - Bitmasp of positions requiring retry
 *
 * @return  None
 *
 * @author
 *  12/03/2008  Ron Proulx  - Created
 *
 ****************************************************************/

void rg_siots_rd_retry(fbe_raid_siots_t *const siots_p, 
                       const fbe_u32_t retry_fruts_count, 
                       const fbe_u16_t fruts_to_retry_bitmap)  
{
    fbe_raid_fruts_t *read_fruts_ptr = NULL;
    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_ptr);

    /* Set the wait count.
     */
    siots_p->wait_count += retry_fruts_count;

    /* Now issue retry for failed fruts.
     */
    (void)fbe_raid_fruts_for_each(fruts_to_retry_bitmap,
                                  read_fruts_ptr,
                                  fbe_raid_fruts_retry,
                                  (fbe_u64_t) FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                  (fbe_u64_t) fbe_raid_fruts_evaluate);

    return;
}
/*****************************
 * end rg_siots_rd_retry()
 *****************************/

/*!***************************************************************
 * fbe_raid_siots_is_upgradable
 ****************************************************************
 * 
 * @brief   For the given parent iots, check all the remaining siots.
 *          If all the remaining siots are either:
 *              o Waiting for an upgrade
 *              o Waiting for a siots lock
 *          and this is the head siots, then it is OK to upgrade.
 *          Otherwise this siots must wait until the other siots
 *          are complete.
 *
 * @param   in_siots_p    - ptr to the fbe_raid_siots_t
 * 
 * @notes   we assume the caller has the iots lock.
 *
 * @return  fbe_bool_t - FBE_TRUE - OK to upgrade this siots
 *                 FBE_FALSE - Not ok to upgrade this siots
 *
 * @author
 *  12/12/2008  Ron Proulx  - Created
 *	10/18/2011  Peter Puhov - This function always called under iots lock
 *
 ****************************************************************/

fbe_bool_t fbe_raid_siots_is_upgradable(fbe_raid_siots_t *const in_siots_p) 
{
    fbe_raid_siots_t  *siots_p;
    fbe_raid_siots_t  *curr_siots_p;
    fbe_raid_siots_t  *next_siots_p = NULL;
    fbe_raid_siots_t  *nested_siots_p;
    fbe_raid_iots_t   *iots_p =  NULL;
    fbe_queue_head_t  *head_p = NULL;

    /* If this is a nested siots we must allow the parent
     * but only one child is allowed.
     */
    if (fbe_raid_siots_is_nested(in_siots_p))
    {
        siots_p = fbe_raid_siots_get_parent(in_siots_p);
    }
    else
    {
        siots_p = (fbe_raid_siots_t *)in_siots_p;
    }
    iots_p = fbe_raid_siots_get_iots(siots_p);

    fbe_raid_iots_get_siots_queue(iots_p, &head_p);
    curr_siots_p = (fbe_raid_siots_t*)fbe_queue_front(head_p);
    if(RAID_COND(curr_siots_p == NULL))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "curr_siots_p is NULL\n");
        return FBE_STATUS_GENERIC_FAILURE;
    }

    while (curr_siots_p != NULL)
    {
        /* Save next siots.
         */
        next_siots_p = fbe_raid_siots_get_next(curr_siots_p);
        /* If it is not this siots check for wait lock or wait upgrade.
         */
        if (curr_siots_p != siots_p)
        {
			/* Lockless siots This function always called under iots lock */
            //fbe_raid_siots_lock(curr_siots_p);
            fbe_raid_siots_get_nest_queue_head(curr_siots_p, &nested_siots_p);
            
            /* We first checked the nested siots. If there are no nested
             * siots then we check the siots flags.
             */
            if (nested_siots_p != NULL)
            {
				/* Lockless siots This function always called under iots lock */
                //fbe_raid_siots_lock(nested_siots_p);
                /* Else check the nested siots.
                 */
                while (nested_siots_p != NULL)
                {
                    /* If it is not this siots and if neither flag is set then
                     * it is not ok to upgrade.
                     */
                    if ( (nested_siots_p != in_siots_p) &&
                         !fbe_raid_siots_is_flag_set(nested_siots_p, FBE_RAID_SIOTS_FLAG_WAIT_LOCK)  &&
                         !fbe_raid_siots_is_flag_set(nested_siots_p, FBE_RAID_SIOTS_FLAG_WAIT_UPGRADE) )
                    {
						/* Lockless siots This function always called under iots lock */
                        //fbe_raid_siots_unlock(nested_siots_p);
                        //fbe_raid_siots_unlock(curr_siots_p);

                        fbe_raid_siots_log_traffic((fbe_raid_siots_t *)in_siots_p, "mark siots wait for upgrade");
                        fbe_raid_siots_log_traffic(nested_siots_p, "b/c of this nested");
                        return(FBE_FALSE);
                    }
					/* Lockless siots This function always called under iots lock */
                    //fbe_raid_siots_unlock(nested_siots_p);
                
                    /* Else goto next.
                     */
                    nested_siots_p = fbe_raid_siots_get_next(nested_siots_p);
            
                }  /* while there are more nested siots */
            }
            else if ( !fbe_raid_siots_is_flag_set(curr_siots_p, FBE_RAID_SIOTS_FLAG_WAIT_LOCK) &&
                      !fbe_raid_siots_is_flag_set(curr_siots_p, FBE_RAID_SIOTS_FLAG_WAIT_UPGRADE) )
            {
                /* Else only check the parent if there are no nested siots.  If 
                 * neither the FBE_RAID_SIOTS_FLAG_WAIT_LOCK or FBE_RAID_SIOTS_FLAG_WAIT_UPGRADE flag is set 
                 * then don't grant the upgrade.
                 */
				/* Lockless siots This function always called under iots lock */
                //fbe_raid_siots_unlock(curr_siots_p);
                fbe_raid_siots_log_traffic((fbe_raid_siots_t *)in_siots_p, "mark siots wait for upgrade");
                fbe_raid_siots_log_traffic(curr_siots_p, "b/c of this siots");
                return(FBE_FALSE);
            }
                
            /* Lockless siots This function always called under iots lock */    
            //fbe_raid_siots_unlock(curr_siots_p);
        } /* end if not this siots */

        /* Goto next siots.
         */
        curr_siots_p = next_siots_p;
    } /* while there are more siots */

    /* If we made it this far it means that there are either no
     * other siots or they are all waiting.  Therefore start the
     * head of the iots queue.  If I'm the head return FBE_TRUE else
     * FBE_FALSE.
     */
    curr_siots_p = (fbe_raid_siots_t*)fbe_queue_front(head_p);
    if (curr_siots_p == siots_p)
    {
        /* Ok to upgrade.
         */
        return(FBE_TRUE);
    }
    else
    {
        /* Else there is someone else in front of me.  We only restart a siots or
         * nested siots if it the wait for upgrade flag is set.
         */
        fbe_raid_siots_log_traffic((fbe_raid_siots_t *)in_siots_p, "mark siots wait for upgrade");

        /* We don't clear the FBE_RAID_SIOTS_FLAG_WAIT_UPGRADE flag since it will be checked
         * and handled later.  We first check the nested siots and locate the first
         * siots that has the wait upgrade flag set. 
         */
		/* Lockless siots This function always called under iots lock */
        //fbe_raid_siots_lock(curr_siots_p);
        fbe_raid_siots_get_nest_queue_head(curr_siots_p, &nested_siots_p);
        if (nested_siots_p != NULL)
        {
            /* Check the nested siots.
             */
            while (nested_siots_p != NULL)
            {
				/* Lockless siots This function always called under iots lock */
                //fbe_raid_siots_lock(nested_siots_p);
                /* If the wait for upgrade flag is set then add it to the state
                 * queue.
                 */
                if (fbe_raid_siots_is_flag_set(nested_siots_p, FBE_RAID_SIOTS_FLAG_WAIT_UPGRADE))
                {
                    fbe_status_t status = FBE_STATUS_OK;
                    fbe_raid_siots_clear_flag(nested_siots_p, FBE_RAID_SIOTS_FLAG_WAIT_UPGRADE);
					/* Lockless siots This function always called under iots lock */
                    //fbe_raid_siots_unlock(nested_siots_p);
                    //fbe_raid_siots_unlock(curr_siots_p);
                    fbe_raid_siots_log_traffic(nested_siots_p, "b/c restart of this nested upgrade");
                    status = fbe_raid_siots_check_clear_upgrade_waiters(siots_p);
                    if(RAID_COND(status != FBE_STATUS_OK))
                    {
                        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                             "raid: failed to clear RIOTS_UPGRADE_WAITERS flag: "
                                             "status = 0x%x, siots_p = 0x%p\n",
                                             status, siots_p);
                        /*
                         *return FBE_TRUE;
                         */
                    }
                    fbe_raid_common_state((fbe_raid_common_t*)nested_siots_p);
                    return(FBE_FALSE);
                }
                else if (fbe_raid_siots_is_flag_set(nested_siots_p, FBE_RAID_SIOTS_FLAG_WAIT_LOCK) &&
                         fbe_raid_siots_is_flag_set(nested_siots_p, FBE_RAID_SIOTS_FLAG_UPGRADE_OWNER))
                {
                    fbe_raid_siots_clear_flag(nested_siots_p, FBE_RAID_SIOTS_FLAG_WAIT_LOCK);
					/* Lockless siots This function always called under iots lock */
                    //fbe_raid_siots_unlock(nested_siots_p);
                    //fbe_raid_siots_unlock(curr_siots_p);
                    fbe_raid_siots_log_traffic(nested_siots_p, "b/c restart of this nested owner");
                    fbe_raid_common_state((fbe_raid_common_t*)nested_siots_p);
                    return(FBE_FALSE);
                }
				/* Lockless siots This function always called under iots lock */
                //fbe_raid_siots_unlock(nested_siots_p);
                
                /* Else goto next.
                 */
                nested_siots_p = fbe_raid_siots_get_next(nested_siots_p);
            
            }  /* while there are more nested siots */
        } /* end if nested siots exist. */
        else if (fbe_raid_siots_is_flag_set(curr_siots_p, FBE_RAID_SIOTS_FLAG_WAIT_UPGRADE))
        {
            fbe_status_t status = FBE_STATUS_OK;
            /* Else no nested siots, but the siots was marked for upgrade.
             */
            fbe_raid_siots_clear_flag(curr_siots_p, FBE_RAID_SIOTS_FLAG_WAIT_UPGRADE);
			/* Lockless siots This function always called under iots lock */
            //fbe_raid_siots_unlock(curr_siots_p);
            fbe_raid_siots_log_traffic(curr_siots_p, "b/c restart of this siots upgrade");
            status = fbe_raid_siots_check_clear_upgrade_waiters(siots_p);
            if(RAID_COND(status != FBE_STATUS_OK))
            {
                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                     "raid: failed to clear RIOTS_UPGRADE_WAITERS flag: "
                                     "status = 0x%x, siots_p = 0x%p\n",
                                     status, siots_p);
                /*
                 *return FBE_TRUE;
                 */
            }
            fbe_raid_common_state((fbe_raid_common_t*)curr_siots_p);
            return(FBE_FALSE);
        }
		/* Lockless siots This function always called under iots lock */
        //fbe_raid_siots_unlock(curr_siots_p);

        /* We return false since the passed siots wasn't upgraded.
         */
        return(FBE_FALSE);
    } /* end else curr_siots_p != siots_p */
} /* end fbe_raid_siots_is_upgradable() */

/*!***************************************************************
 * @fn      fbe_raid_siots_check_clear_upgrade_waiters (const fbe_raid_siots_t *const siots_p) 
 ****************************************************************
 * 
 * @brief   This routine simply walks the associated iots and if there
 *          are no siots with the FBE_RAID_SIOTS_FLAG_WAIT_UPGRADE flag set, simply
 *          clears the RIOTS_UPGRADE_WAITERS flag.
 *
 * @param   siots_p    - ptr to the fbe_raid_siots_t
 *
 * @return  fbe_status_t
 *
 * @author
 *  12/12/2008  Ron Proulx  - Created
 *	10/18/2011  Peter Puhov - This function always called under iots lock
 *
 ****************************************************************/

fbe_status_t fbe_raid_siots_check_clear_upgrade_waiters(fbe_raid_siots_t *const siots_p) 
{
    fbe_raid_siots_t  *curr_siots_p = NULL;
    fbe_raid_iots_t   *iots_p =  NULL;
    fbe_bool_t        upgrade_waiter_found = FBE_FALSE;
    fbe_status_t      status = FBE_STATUS_OK;
    iots_p = fbe_raid_siots_get_iots(siots_p);

	/* !!! This function always called under iots lock */
    //fbe_raid_iots_lock(iots_p);

    fbe_raid_iots_get_siots_queue_head(iots_p, &curr_siots_p);

    /* Get siots queue and check both siots and
     * nested siots.
     */
    fbe_raid_siots_log_traffic((fbe_raid_siots_t *)siots_p, "check/clear iots upgrade waiters");
    if (RAID_COND(curr_siots_p == NULL))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "curr_siots_p is NULL\n");
        return FBE_STATUS_GENERIC_FAILURE;
    }
    while ((curr_siots_p != NULL) && !(upgrade_waiter_found) )
    {
        fbe_raid_siots_t  *nested_siots_p;
		/* Lockless siots This function always called under iots lock */
        //fbe_raid_siots_lock(curr_siots_p);

        /* If an upgrade waiter is found set the flag and break out
         * of loop.
         */
        if (fbe_raid_siots_is_flag_set(curr_siots_p, FBE_RAID_SIOTS_FLAG_WAIT_UPGRADE))
        {
            upgrade_waiter_found = FBE_TRUE;
            break;
        }

        /* Else check the nested siots.
         */
        fbe_raid_siots_get_nest_queue_head(curr_siots_p, &nested_siots_p);
        while (nested_siots_p != NULL)
        {
			/* Lockless siots This function always called under iots lock */
            //fbe_raid_siots_lock(nested_siots_p);
            /* If an upgrade waiter is found set the flag and break out
             * of loop.
             */
            if (fbe_raid_siots_is_flag_set(nested_siots_p, FBE_RAID_SIOTS_FLAG_WAIT_UPGRADE))
            {
                upgrade_waiter_found = FBE_TRUE;
                break;
            }
			/* Lockless siots This function always called under iots lock */
            //fbe_raid_siots_unlock(nested_siots_p);
            /* Else goto next nested siots.
             */
            nested_siots_p = fbe_raid_siots_get_next(nested_siots_p);
        }
		/* Lockless siots This function always called under iots lock */
        //fbe_raid_siots_unlock(curr_siots_p);

        /* Goto next siots.
         */
        curr_siots_p = fbe_raid_siots_get_next(curr_siots_p);
    
    } /* while there are more siots and no waiters found

    /* If there are no waiters found clear the iots upgrade
     * waiters flag.
     */
    if (!upgrade_waiter_found)
    {
        /* Clear the iots flag.
         */
        fbe_raid_siots_log_traffic((fbe_raid_siots_t *)siots_p, "clear iots upgrade waiters flag");
        fbe_raid_iots_clear_flag(iots_p, FBE_RAID_IOTS_FLAG_UPGRADE_WAITERS);
    } 
	/* !!! This function always called under iots lock */
    // fbe_raid_iots_unlock(iots_p);
    return status;

} /* end fbe_raid_siots_check_clear_upgrade_waiters() */

/*!***************************************************************
 * @fn      fbe_raid_siots_get_upgrade_waiter (const fbe_raid_siots_t *const siots_p) 
 ****************************************************************
 * 
 * @brief   This routine simply walks the associated iots and returns
 *          a pointer to the first siots with the FBE_RAID_SIOTS_FLAG_WAIT_UPGRADE
 *          flag set.
 *
 * @param   siots_p    - ptr to the fbe_raid_siots_t
 * 
 * @notes  we assume the iots lock is held by the caller.
 *
 * @return  upgrade_siots_p - Pointer to siots that is waiting for an
 *          upgrade (due to recovery verify).
 *
 * @author
 *  12/12/2008  Ron Proulx  - Created
 *	18/10/2011  Peter Puhove - This function always called under iots lock
 *
 ****************************************************************/

static fbe_raid_siots_t *fbe_raid_siots_get_upgrade_waiter(fbe_raid_siots_t *const siots_p) 
{
    fbe_raid_iots_t *iots_p = fbe_raid_siots_get_iots(siots_p);

    /* If we are the upgrade owner return false since
     * we don't want to upgrade a second siots until the
     * owner is complete.
     */
    if (!fbe_raid_siots_is_flag_set(siots_p, FBE_RAID_SIOTS_FLAG_UPGRADE_OWNER))
    {
        fbe_raid_siots_t  *curr_siots_p = NULL;
        
        /* Get siots queue and check both siots and
         * nested siots.
         */
        fbe_raid_iots_get_siots_queue_head(iots_p, &curr_siots_p);
        if (RAID_COND(curr_siots_p == NULL))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "curr_siots_p is NULL\n");
            return (fbe_raid_siots_t *)NULL;
        }
        while(curr_siots_p != NULL)
        {
            fbe_raid_siots_t  *nested_siots_p;
        
            /* First check the nested siots.
             */
            fbe_raid_siots_get_nest_queue_head(curr_siots_p, &nested_siots_p);
            while (nested_siots_p != NULL)
            {
                /* If an upgrade waiter is found and it is startable
                 * return a pointer to it.
                 */
                if ( fbe_raid_siots_is_flag_set(nested_siots_p, FBE_RAID_SIOTS_FLAG_WAIT_UPGRADE) &&
                     fbe_raid_siots_is_upgradable(nested_siots_p) )
                {
                    fbe_raid_siots_log_traffic(nested_siots_p, "nested siots upgrade waiter found");
                    return(nested_siots_p);
                }

                /* Else goto next nested siots.
                 */
                nested_siots_p = fbe_raid_siots_get_next(nested_siots_p);
            }
        
            /* If an upgrade waiter is found and it is startable return a
             * pointer to it.
             */
            if (fbe_raid_siots_is_flag_set(curr_siots_p, FBE_RAID_SIOTS_FLAG_WAIT_UPGRADE) &&
                 fbe_raid_siots_is_upgradable(curr_siots_p) )
            {
                fbe_raid_siots_log_traffic((fbe_raid_siots_t *)siots_p, "siots upgrade waiter found");
                return((fbe_raid_siots_t *)siots_p);
            }

            /* Goto next siots.
             */
            curr_siots_p = fbe_raid_siots_get_next(curr_siots_p);
    
        } /* while there are more siots and no waiters found */

    } /* end if not the upgrade owner */
    /* No waiters found, return NULL.
     */
    return((fbe_raid_siots_t *)NULL);

} /* end fbe_raid_siots_get_upgrade_waiter() */

/*!***************************************************************
 * fbe_raid_siots_restart_upgrade_waiter () 
 ****************************************************************
 * @brief   If there is a startable upgrade waiter then start it and
 *          return FBE_TRUE.  Else return FBE_FALSE.
 *
 * @param   siots_p    - ptr to the fbe_raid_siots_t
 * @param   restart_siots_p - ptr to the fbe_raid_siots_t
 * 
 * @notes This function needs to be called with the iots lock held.
 *
 * @return  fbe_bool_t - FBE_TRUE - There was an upgrade waiter found and restarted
 *                 FBE_FALSE - No startable upgrade waiter found
 *
 * @author
 *  12/17/2008  Ron Proulx  - Created
 *	18/10/2011  Peter Puhov - This function always called under iots lock
 *
 ****************************************************************/

fbe_bool_t fbe_raid_siots_restart_upgrade_waiter(fbe_raid_siots_t *const siots_p,
                                                 fbe_raid_siots_t **restart_siots_p)
{
    fbe_raid_siots_t *nested_siots_p;
    fbe_raid_iots_t *iots_p = fbe_raid_siots_get_iots(siots_p);
    fbe_status_t status = FBE_STATUS_OK;

    *restart_siots_p = NULL;

    /* Siots waiting for an upgrade have priority for new siots waiting
     * for a lock.  Therefore check and start any upgrade waiters first.
     */
    if (fbe_raid_iots_is_flag_set(iots_p, FBE_RAID_IOTS_FLAG_UPGRADE_WAITERS))
    {
        if ((nested_siots_p = fbe_raid_siots_get_upgrade_waiter(siots_p)) != NULL)
        {
            /* We return false since no siots waiting for a lock was
             * re-activated. We don't clear the FBE_RAID_SIOTS_FLAG_WAIT_UPGRADE flag since
             * it will be checked and handled later.
             */
            fbe_raid_siots_clear_flag(nested_siots_p, FBE_RAID_SIOTS_FLAG_WAIT_UPGRADE);
            status = fbe_raid_siots_check_clear_upgrade_waiters(siots_p);
            if(RAID_COND(status != FBE_STATUS_OK))
            {
                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                     "raid: failed to clear RIOTS_UPGRADE_WAITERS flag: "
                                     "status = 0x%x, siots_p = 0x%p\n",
                                     status, siots_p);
                /*
                 *return FBE_TRUE;
                 */
            }
            fbe_raid_siots_log_traffic(nested_siots_p, "Restart upgrade nest waiter");
            *restart_siots_p = nested_siots_p;
            return(FBE_TRUE);
        }
    }
    /* No upgrade waiter found and started.
     */
    return(FBE_FALSE);

} /* end fbe_raid_siots_restart_upgrade_waiter */

/*!**************************************************************
 * fbe_raid_siots_get_parity_count_for_region_mining()
 ****************************************************************
 * @brief
 *  Set the siots parity count field for region mining.
 *  We will make sure the parity count ends on a region and
 *  make sure that it does not exceed the siots xfer_count.
 *  
 * @param siots_p - Siots to get parity count for.
 *
 * @return - Count of blocks to region mine (parity_count).
 *
 * @author
 *  2/27/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_u32_t fbe_raid_siots_get_parity_count_for_region_mining(fbe_raid_siots_t *const siots_p)
{
    fbe_u32_t parity_count;
    fbe_u32_t region_size = fbe_raid_siots_get_region_size(siots_p);

    if (siots_p->parity_start % region_size)
    {
        /* If we are not aligned to the region size, then
         * align the end to the region size so the next request is fully
         * aligned.
         */
        parity_count = (fbe_u32_t)(region_size - (siots_p->parity_start % region_size));
    }
    else
    {
        /* we do not expect the parity count to exceed 32bit limit here
         */
        if(FBE_IS_VAL_32_BITS(FBE_MIN(region_size, siots_p->xfer_count)))
        {
            fbe_raid_siots_set_unexpected_error(siots_p, 
                                                FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "mirror: region size (0) is invalid ");
             return (0);
        }
        /* Start aligned to optimal size, just do the next piece.
         */ 
        parity_count = (fbe_u32_t)(FBE_MIN(region_size, siots_p->xfer_count));
    }

    if(FBE_IS_VAL_32_BITS(FBE_MIN(parity_count, siots_p->xfer_count)))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "mirror: region size (0) is invalid ");
         return (0);
    }
    parity_count = (fbe_u32_t)(FBE_MIN(parity_count, siots_p->xfer_count));
    return parity_count;
}
/**************************************
 * end fbe_raid_siots_get_parity_count_for_region_mining()
 **************************************/

/*!************************************************************
 *      fbe_raid_siots_is_position_degraded()
 *************************************************************
 *
 * @brief   If the requested postion is either the first or
 *          second dead position, then this position is degraded.
 *
 * @param   siots_p - Pointer to the siots to check for degraded
 * @param   position - Position to check if degraded or not
 *
 * @return  bool - FBE_TRUE - This position is degraded
 *                 FBE_FALSE - This position isn't degraded
 * 
 * @author
 *  09/10/2009  Ron Proulx  - Created from rg_siots_is_pos_degraded
 *************************************************************/

fbe_bool_t  fbe_raid_siots_is_position_degraded(fbe_raid_siots_t *const siots_p,
                                                const fbe_u32_t position)
{
    fbe_bool_t  b_degraded = FBE_FALSE;
    fbe_u32_t   width = fbe_raid_siots_get_width(siots_p);
    
    /* First validate passed position.  If validation fails then
     * that position isn't degraded.
     */
    if ((position  >= width)               ||
        (position == FBE_RAID_INVALID_POS)    )
    {
        /* If passed position is invalid then it isn't degraded.
         */
    }
    else
    {
        if ((position == fbe_raid_siots_dead_pos_get(siots_p, FBE_RAID_FIRST_DEAD_POS))  ||
            (position == fbe_raid_siots_dead_pos_get(siots_p, FBE_RAID_SECOND_DEAD_POS))    )
        {
            b_degraded = FBE_TRUE;
        }
    }

    /* Always return degraded value.
     */
    return(b_degraded);
}
/* end fbe_raid_siots_is_position_degraded */

/*!***************************************************************************
 *          fbe_raid_siots_get_degraded_count()
 *****************************************************************************
 * 
 * @brief   Simply a wrapper to get the number of `degraded' drives.
 *
 * @param   siots_p - Pointer to siots for request
 *
 * @return  degraded count (always succeeds)  
 *
 * @author
 *  12/09/2009  Ron Proulx  - Created.
 *
 ****************************************************************************/
fbe_u32_t fbe_raid_siots_get_degraded_count(fbe_raid_siots_t *siots_p)
{
    fbe_raid_position_bitmask_t degraded_bitmask = 0;
    fbe_u32_t                   degraded_count = 0;

    /* Get the degraded bitmask and then the count
     */
    fbe_raid_siots_get_degraded_bitmask(siots_p, &degraded_bitmask);
    degraded_count = fbe_raid_get_bitcount(degraded_bitmask);
     
    /* Always return the degraded count
     */
    return(degraded_count);
}
/* end fbe_raid_siots_get_degraded_count() */

/*!***************************************************************************
 *          fbe_raid_siots_get_disabled_count()
 *****************************************************************************
 * 
 * @brief   Simply a wrapper to get the number of `disabled' drives.
 *
 * @param   siots_p - Pointer to siots for request
 *
 * @return  degraded count (always succeeds)  
 *
 * @author
 *  02/11/2010  Ron Proulx  - Created.
 *
 ****************************************************************************/
fbe_u32_t fbe_raid_siots_get_disabled_count(fbe_raid_siots_t *siots_p)
{
    fbe_raid_position_bitmask_t disabled_bitmask = 0;
    fbe_u32_t                   disabled_count = 0;

    /* Get the disabled bitmask and then the count
     */
    fbe_raid_siots_get_disabled_bitmask(siots_p, &disabled_bitmask);
    disabled_count = fbe_raid_get_bitcount(disabled_bitmask);
     
    /* Always return the disabled count
     */
    return(disabled_count);
}
/* end fbe_raid_siots_get_disabled_count() */

/*!***********************************************************
 * fbe_raid_siots_is_expired()
 ************************************************************
 * @brief
 *  A siots is aborted if the iots is timed out.
 * 
 * @param siots_p - The siots to check.
 * 
 * @return FBE_TRUE - If this is timed out.
 *        FBE_FALSE - Otherwise, not timed out.
 * 
 ************************************************************/
fbe_bool_t fbe_raid_siots_is_expired(fbe_raid_siots_t * const siots_p)
{
    fbe_bool_t b_status = FBE_FALSE;

    if (fbe_raid_iots_is_expired(fbe_raid_siots_get_iots(siots_p)))
    {
        b_status = FBE_TRUE;
    }
    return b_status;
}
/**************************************
 * end fbe_raid_siots_is_expired()
 **************************************/

/*!***********************************************************
 * fbe_raid_siots_get_expiration_time()
 ************************************************************
 * @brief
 *  return expiration time for an siots.
 * 
 * @param siots_p - The siots to check.
 * @param expiration_time_p - expiration time to return.
 * 
 * @return FBE_STATUS_OK
 * 
 ************************************************************/
fbe_status_t fbe_raid_siots_get_expiration_time(fbe_raid_siots_t *const siots_p,
                                                fbe_time_t *expiration_time_p)
{
    fbe_raid_iots_t *iots_p = fbe_raid_siots_get_iots(siots_p);
    return fbe_raid_iots_get_expiration_time(iots_p, expiration_time_p);
}
/**************************************
 * end fbe_raid_iots_get_expiration_time()
 **************************************/

/*!***************************************************************************
 *          fbe_raid_siots_get_full_access_bitmask()
 *****************************************************************************
 * 
 * @brief   Returns the bitmask of positions that are not degraded.
 *
 * @param   siots_p - Pointer to siots for request
 * @param   full_access_bitmask_p - Pointer to `full' access bitmask
 *
 * @return  fbe_status_t 
 *
 * @author
 *  12/09/2009  Ron Proulx  - Created.
 *
 ****************************************************************************/
fbe_status_t fbe_raid_siots_get_full_access_bitmask(fbe_raid_siots_t *siots_p,
                                            fbe_raid_position_bitmask_t *full_access_bitmask_p)
{
    fbe_raid_position_bitmask_t full_access_bitmask = 0;
    fbe_raid_position_bitmask_t degraded_bitmask = 0;
    fbe_raid_position_bitmask_t disabled_bitmask = 0;
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_u32_t                   width = fbe_raid_siots_get_width(siots_p);
    /* Get the valid bitmask and or it with the `not' of
     * the limited access bitmask.
     */
    status = fbe_raid_util_get_valid_bitmask(width, &full_access_bitmask);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "raid: failed to get valid bitmask: status = 0x%x, siots_p = 0x%p\n",
                             status, siots_p);
        return status;
    }
    fbe_raid_siots_get_degraded_bitmask(siots_p, &degraded_bitmask);
    fbe_raid_siots_get_disabled_bitmask(siots_p, &disabled_bitmask);

    /* Remove the degraded positions from the valid bitmask
     */
    full_access_bitmask &= ~(degraded_bitmask | disabled_bitmask);

    /* Now update the return the value.
     */
    *full_access_bitmask_p = full_access_bitmask;

    /* Always succeeds
     */
    return status;
}
/* end fbe_raid_siots_get_full_access_bitmask() */

/*!***************************************************************************
 *          fbe_raid_siots_get_full_access_count()
 *****************************************************************************
 * 
 * @brief   Simply a wrapper to get the number of `fully acccessible' drives.
 *          Fully accessible means that the position isn't degraded
 *          
 *
 * @param   siots_p - Pointer to siots for request
 *
 * @return  accessible count - Number of positions that have `full' access
 *
 * @author
 *  12/11/2009  Ron Proulx  - Created.
 *
 ****************************************************************************/
fbe_u32_t fbe_raid_siots_get_full_access_count(fbe_raid_siots_t *siots_p)
{
    fbe_raid_position_bitmask_t full_access_bitmask = 0;  
    fbe_u32_t                   full_access_count = 0;
    fbe_status_t                status = FBE_STATUS_OK;
    /* Get the full access bitmask and then count the bits
     */
    status = fbe_raid_siots_get_full_access_bitmask(siots_p, &full_access_bitmask);
    if (RAID_COND(status != FBE_STATUS_OK))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "raid: failed to get full access bitmask: status = 0x%x, siots_p = 0x%p\n",
                             status, siots_p);
        return full_access_count;
    }
    full_access_count = fbe_raid_get_bitcount(full_access_bitmask);

    /* Simply return the full access count.
     */
    return(full_access_count);
}
/* end fbe_raid_siots_get_full_access_count() */

/*!***************************************************************************
 *          fbe_raid_siots_get_read_fruts_count()
 *****************************************************************************
 * 
 * @brief   Simply a wrapper to get the number of fruts queued on the read 
 *          chain.
 *          
 *
 * @param   siots_p - Pointer to siots for request
 *
 * @return  read fruts count - Number of fruts on read queue
 *
 * @author
 *  12/16/2009  Ron Proulx  - Created.
 *
 ****************************************************************************/
fbe_u32_t fbe_raid_siots_get_read_fruts_count(fbe_raid_siots_t *siots_p)
{
    /* Simply return the queue length of the read fruts queue
     */
    return(fbe_queue_length(&siots_p->read_fruts_head));
}
/* end fbe_raid_siots_get_read_fruts_count() */

/*!***************************************************************************
 *          fbe_raid_siots_get_read2_fruts_count()
 *****************************************************************************
 * 
 * @brief   Simply a wrapper to get the number of fruts queued on the read2 
 *          chain.
 *          
 *
 * @param   siots_p - Pointer to siots for request
 *
 * @return  read2 fruts count - Number of fruts on read queue
 *
 * @author
 *  12/16/2009  Ron Proulx  - Created.
 *
 ****************************************************************************/
fbe_u32_t fbe_raid_siots_get_read2_fruts_count(fbe_raid_siots_t *siots_p)
{
    /* Simply return the queue length of the read2 fruts queue
     */
    return(fbe_queue_length(&siots_p->read2_fruts_head));
}
/* end fbe_raid_siots_get_read2_fruts_count() */

/*!***************************************************************************
 *          fbe_raid_siots_get_write_fruts_count()
 *****************************************************************************
 * 
 * @brief   Simply a wrapper to get the number of fruts queued on the write 
 *          chain.
 *          
 *
 * @param   siots_p - Pointer to siots for request
 *
 * @return  write fruts count - Number of fruts on write queue
 *
 * @author
 *  12/16/2009  Ron Proulx  - Created.
 *
 ****************************************************************************/
fbe_u32_t fbe_raid_siots_get_write_fruts_count(fbe_raid_siots_t *siots_p)
{
    /* Simply return the queue length of the write fruts queue
     */
    return(fbe_queue_length(&siots_p->write_fruts_head));
}
/* end fbe_raid_siots_get_write_fruts_count() */

/*!***************************************************************************
 *          fbe_raid_siots_move_fruts()
 *****************************************************************************
 *
 * @brief   This method `moves' the fruts specified from one chain to another.
 *          This method is used when we want to remove a fruts from a chain
 *          but don't want to loose the reference to it (i.e. all allocated
 *          fruts must be destroyed).
 *
 * @param   siots_p - Ptr to siots.
 * @param   fruts_to_move_p - Pointer to fruts to move to new chain
 * @param   destination_chain_p - Pointer to destination chain to insert fruts onto
 *
 * @return  fbe_status_t
 *
 *****************************************************************************/
fbe_status_t fbe_raid_siots_move_fruts(fbe_raid_siots_t *siots_p,
                                       fbe_raid_fruts_t *fruts_to_move_p,
                                       fbe_queue_head_t *destination_chain_p)
                                              
{
    fbe_status_t    status = FBE_STATUS_OK;

    /* Validate that parent is a siots
     */
    if(RAID_COND(fbe_raid_common_get_parent((fbe_raid_common_t *)fruts_to_move_p) !=
                 (fbe_raid_common_t *)siots_p))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "raid: siots_p: 0x%p attempt to move fruts_p: 0x%p without being parent\n",
                             siots_p, fruts_to_move_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Now remove the fruts from the current queue
     */
    fbe_raid_common_queue_remove(&fruts_to_move_p->common);

    /* Now insert it on the destination queue
     */
    fbe_raid_common_enqueue_tail(destination_chain_p, &fruts_to_move_p->common);

    return status;
}
/* end fbe_raid_siots_move_fruts() */

/*!***************************************************************************
 *          fbe_raid_siots_get_read_fruts_bitmask()
 *****************************************************************************
 * 
 * @brief   Returns the bitmask of positions that are on the read chain.
 *
 * @param   siots_p - Pointer to siots for request
 * @param   read_fruts_bitmask_p - Pointer to read fruts bitmask
 *
 * @return  None (always succeeds)  
 *
 * @author
 *  12/09/2009  Ron Proulx  - Created.
 *
 ****************************************************************************/
void fbe_raid_siots_get_read_fruts_bitmask(fbe_raid_siots_t *siots_p,
                                           fbe_raid_position_bitmask_t *read_fruts_bitmask_p)
{
    fbe_raid_position_bitmask_t read_fruts_bitmask = 0;
    fbe_raid_fruts_t           *fruts_p = NULL;

    /*  Set the bit for any position on the read chain
     */
    fbe_raid_siots_get_read_fruts(siots_p, &fruts_p);
    while (fruts_p != NULL)
    {
        /* Simply set the bit for each position found.
         */
        read_fruts_bitmask |= (1 << fruts_p->position);

        /* Goto to next fruts.
         */
        fruts_p = fbe_raid_fruts_get_next(fruts_p);
    }

    /* Update the passed value.
     */
    *read_fruts_bitmask_p = read_fruts_bitmask;

    /* Always succeeds
     */
    return;
}
/* end fbe_raid_siots_get_read_fruts_bitmask() */

/*!***************************************************************************
 *          fbe_raid_siots_get_read2_fruts_bitmask()
 *****************************************************************************
 * 
 * @brief   Returns the bitmask of positions that are on the read2 chain.
 *
 * @param   siots_p - Pointer to siots for request
 * @param   read2_fruts_bitmask_p - Pointer to read2 fruts bitmask
 *
 * @return  None (always succeeds)  
 *
 * @author
 *  12/09/2009  Ron Proulx  - Created.
 *
 ****************************************************************************/
void fbe_raid_siots_get_read2_fruts_bitmask(fbe_raid_siots_t *siots_p,
                                           fbe_raid_position_bitmask_t *read2_fruts_bitmask_p)
{
    fbe_raid_position_bitmask_t read2_fruts_bitmask = 0;
    fbe_raid_fruts_t           *fruts_p = NULL;

    /*  Set the bit for any position on the read chain
     */
    fbe_raid_siots_get_read2_fruts(siots_p, &fruts_p);
    while (fruts_p != NULL)
    {
        /* Simply set the bit for each position found.
         */
        read2_fruts_bitmask |= (1 << fruts_p->position);

        /* Goto to next fruts.
         */
        fruts_p = fbe_raid_fruts_get_next(fruts_p);
    }

    /* Update the passed value.
     */
    *read2_fruts_bitmask_p = read2_fruts_bitmask;

    /* Always succeeds
     */
    return;
}
/* end fbe_raid_siots_get_read2_fruts_bitmask() */

/*!***************************************************************************
 *          fbe_raid_siots_get_write_fruts_bitmask()
 *****************************************************************************
 * 
 * @brief   Returns the bitmask of positions that are on the write chain.
 *
 * @param   siots_p - Pointer to siots for request
 * @param   write_fruts_bitmask_p - Pointer to write fruts bitmask
 *
 * @return  None (always succeeds)  
 *
 * @author
 *  12/09/2009  Ron Proulx  - Created.
 *
 ****************************************************************************/
void fbe_raid_siots_get_write_fruts_bitmask(fbe_raid_siots_t *siots_p,
                                           fbe_raid_position_bitmask_t *write_fruts_bitmask_p)
{
    fbe_raid_position_bitmask_t write_fruts_bitmask = 0;
    fbe_raid_fruts_t           *fruts_p = NULL;

    /*  Set the bit for any position on the write chain
     */
    fbe_raid_siots_get_write_fruts(siots_p, &fruts_p);
    while (fruts_p != NULL)
    {
        /* Simply set the bit for each position found.
         */
        write_fruts_bitmask |= (1 << fruts_p->position);

        /* Goto to next fruts.
         */
        fruts_p = fbe_raid_fruts_get_next(fruts_p);
    }

    /* Update the passed value.
     */
    *write_fruts_bitmask_p = write_fruts_bitmask;

    /* Always succeeds
     */
    return;
}
/* end fbe_raid_siots_get_write_fruts_bitmask() */

/*!***************************************************************************
 *          fbe_raid_siots_get_capacity()
 *****************************************************************************
 * 
 * @brief   This routine uses the iots starting lba to determine if this is
 *          a metadata I/O or a user I/O.  It then returns the appropriate
 *          capacity.
 *
 * @param   siots_p - Pointer to siots for request
 * @param   capacity_p - Pointer to capacity value to update
 *
 * @return  status - Currently always FBE_STATUS_OK
 *
 * @author
 *  02/04/2010  Ron Proulx  - Created.
 *
 ****************************************************************************/
fbe_status_t fbe_raid_siots_get_capacity(fbe_raid_siots_t * const siots_p,
                                         fbe_lba_t *capacity_p)
{
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_raid_iots_t    *iots_p = fbe_raid_siots_get_iots(siots_p);

    /* Invoke the iots get capacity.
     */
    status = fbe_raid_iots_get_capacity(iots_p, capacity_p);

     /* Always return the execution status.
      */
     return(status);
}
/* end of fbe_raid_siots_get_capacity() */

/*!***************************************************************************
 *          fbe_raid_siots_convert_hard_media_errors()
 *****************************************************************************
 *
 * @brief   Change any `hard' (a.k.a. uncorrectable read) errors to correctable 
 *          except for the position read.  If the position read is in error 
 *          that isn't expected.
 *
 * @param   siots_p - The siots to convert the errors for
 * @param   read_position - The position we just read
 *
 * @return  status - Unless hard errors for the position read-> FBE_STATUS_OK
 *
 *****************************************************************************/
fbe_status_t fbe_raid_siots_convert_hard_media_errors(fbe_raid_siots_t *siots_p,
                                               fbe_raid_position_t read_position)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_xor_error_t            *eboard_p = fbe_raid_siots_get_eboard(siots_p);
    fbe_raid_position_bitmask_t hard_media_err_bitmask;

    /* Get the hard error bitmask.
     */
    hard_media_err_bitmask = eboard_p->hard_media_err_bitmap;

    /* Validate read position.
     */
    if ((read_position == FBE_RAID_INVALID_POS)   ||
        ((1 << read_position) & hard_media_err_bitmask)    )
    {
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    /* If there were hard errors convert them to recovered crc errors.
     */
    if (hard_media_err_bitmask != 0)
    {
        /* Initialize the eboard if it hasn't already been done.  Then convert 
         * hard errors to correctable errors.
         */
        fbe_xor_lib_eboard_init(eboard_p);
        eboard_p->hard_media_err_bitmap = 0;
        eboard_p->media_err_bitmap |= hard_media_err_bitmask;
        eboard_p->c_crc_bitmap |= hard_media_err_bitmask;
    }

    /* Always return the status.
     */
    return(status);
}
/* end of fbe_raid_siots_convert_hard_media_errors()

/*!***************************************************************************
 *          fbe_raid_siots_clear_converted_hard_media_errors()
 *****************************************************************************
 *
 * @brief   Clear the previously converted errors from the siots eboard.
 *
 * @param   siots_p - The siots that we have converted errors for
 * @param   hard_media_err_bitmask - Bitmask of errors that were converted.
 *
 * @return  None
 *
 *****************************************************************************/

void fbe_raid_siots_clear_converted_hard_media_errors(fbe_raid_siots_t *siots_p,
                                                      fbe_raid_position_bitmask_t hard_media_err_bitmask)
{
    /* Simply clear the converted errors from the eboard.
     */    
    siots_p->misc_ts.cxts.eboard.media_err_bitmap &= ~hard_media_err_bitmask;
    siots_p->misc_ts.cxts.eboard.c_crc_bitmap &= ~hard_media_err_bitmask;
    
    return;
}
/* end of fbe_raid_siots_clear_converted_hard_media_errors() */

/*!***************************************************************************
 *          fbe_raid_siots_is_validation_enabled()
 *****************************************************************************
 *
 * @brief   Simply checks if error injection validation is enabled.  
 *
 * @param   siots_p - Request to check if validation is enabled or not.
 *
 * @return  bool - FBE_TRUE - Error injection validation is enabled
 *                 FBE_FALSE - Error injection validation isn't enabled
 *
 * @author  
 *  04/15/2010  Ron Proulx  - Created
 *
 *****************************************************************************/
fbe_bool_t fbe_raid_siots_is_validation_enabled(fbe_raid_siots_t *siots_p)
{
    fbe_bool_t  b_validation_enabled = FBE_FALSE;
    fbe_raid_siots_error_validation_callback_t callback;

    /* Simply check if the validation callback is set or not.
     */
    fbe_raid_siots_get_error_validation_callback(siots_p, &callback);
    if (callback != NULL)
    {
        b_validation_enabled = FBE_TRUE;
    }

    /* Return if error injection validation is enabled or not.
     */
    return(b_validation_enabled);
}
/***********************************************
 * end of fbe_raid_siots_is_validation_enabled()
 ***********************************************/
/*!**************************************************************
 * fbe_raid_siots_abort()
 ****************************************************************
 * @brief
 *  Abort the siots and any nested siots.
 *  
 * @param siots_p
 * 
 * @return fbe_status_t
 *
 * @author
 *  4/19/2010 - Created. Rob Foley
 *	10/19/2011  Peter Puhov - This function always called under iots lock
 *
 ****************************************************************/

fbe_status_t fbe_raid_siots_abort(fbe_raid_siots_t *const siots_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_raid_siots_t *nest_siots_p = NULL;
	fbe_raid_iots_t   *iots_p =  NULL;
	iots_p = fbe_raid_siots_get_iots(siots_p);

    if (!fbe_raid_common_is_flag_set((fbe_raid_common_t*)siots_p,
                                     FBE_RAID_COMMON_FLAG_TYPE_SIOTS) &&
        !fbe_raid_common_is_flag_set((fbe_raid_common_t*)siots_p,
                                     FBE_RAID_COMMON_FLAG_TYPE_NEST_SIOTS))
    {
        /* Something is wrong, we expected a raid iots.
         */
        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_CRITICAL,
                               "raid: %s siots: %p common flags: 0x%x are unexpected\n",
                               __FUNCTION__, siots_p, siots_p->common.flags);
        return FBE_STATUS_GENERIC_FAILURE;
    }

	/* Lockless siots (This function always called under iots lock) */
    //fbe_raid_siots_lock(siots_p);

    /* Only try to abort fruts if there is a wait count.
     * When the wait count is nonzero it means we could
     * still have outstanding requests.
     */
    if (siots_p->wait_count != 0)
    { 
        fbe_raid_fruts_t *read_fruts_ptr = NULL;
        fbe_raid_fruts_t *read2_fruts_ptr = NULL;
        fbe_raid_fruts_t *write_fruts_ptr = NULL;
        fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_ptr);
        fbe_raid_siots_get_read2_fruts(siots_p, &read2_fruts_ptr);
        fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_ptr);

        /* Try to abort each of the fruts chains.
         */
        if (!fbe_queue_is_empty(&siots_p->read_fruts_head))
        {
            fbe_raid_fruts_abort_chain(&siots_p->read_fruts_head);
        }
        if (!fbe_queue_is_empty(&siots_p->read2_fruts_head))
        {
            fbe_raid_fruts_abort_chain(&siots_p->read2_fruts_head);
        }
        if (!fbe_queue_is_empty(&siots_p->write_fruts_head))
        {
            fbe_raid_fruts_abort_chain(&siots_p->write_fruts_head);
        }
    }/* end if wait count not zero. */
    else if (fbe_raid_common_is_flag_set(&siots_p->common, FBE_RAID_COMMON_FLAG_WAITING_FOR_MEMORY))
    {
        /* If we are waiting for memory, abort the memory request
         */
        status = fbe_raid_memory_api_abort_request(fbe_raid_siots_get_memory_request(siots_p));
        if (RAID_MEMORY_COND(status != FBE_STATUS_OK))
        {
            /* Trace a message and continue
             */
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                "raid: siots_p: 0x%p failed to abort memory request with status: 0x%x \n",
                                siots_p, status);
        }
    }

    /* Loop through each siots_p and abort it.
     */
    nest_siots_p = (fbe_raid_siots_t *)fbe_queue_front(&siots_p->nsiots_q);
    while (nest_siots_p != NULL)
    {
        fbe_raid_siots_abort(nest_siots_p);
        nest_siots_p = fbe_raid_siots_get_next(nest_siots_p);
    }
	
	/* Lockless siots (This function always called under iots lock) */
    //fbe_raid_siots_unlock(siots_p);
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_raid_siots_abort()
 ******************************************/

/*!***************************************************************************
 *          fbe_raid_siots_reinit_for_next_region()
 *****************************************************************************
 *
 * @brief   Re-initialize a siots for the next region.  This includes clearing
 *          any fields there are only initialized once per siots.
 *  
 * @param   siots_p - Pointer to siots to re-initialize
 * 
 * @return  status - Typically FBE_STATUS_OK
 *
 * @note    This method does not re-configure the siots range.  It is up to
 *          the caller to do that.
 *
 * @author
 *  04/30/2010  Ron Proulx  - Created
 *
 *****************************************************************************/
fbe_status_t fbe_raid_siots_reinit_for_next_region(fbe_raid_siots_t *const siots_p)
{
    fbe_status_t    status = FBE_STATUS_OK;

    /* If we aren't in region mode its an error.
     */
    if (!fbe_raid_siots_is_flag_set(siots_p, FBE_RAID_SIOTS_FLAG_SINGLE_REGION_MODE))
    {
        fbe_raid_siots_flags_t  flags = fbe_raid_siots_get_flags(siots_p);

        /* Report the error and fail the request.
         */
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "raid: flags: 0x%x expected region mode to be set, siots_p = 0x%p\n",
                             flags, siots_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Reset any neccessary error counters etc.
     */
    siots_p->misc_ts.cxts.eboard.hard_media_err_bitmap = 0;
    siots_p->media_error_count = 0;

    /* Always return the execution status.
     */
    return(status);
}
/**********************************************
 * end fbe_raid_siots_reinit_for_next_region()
 **********************************************/

/*!**************************************************************
 * fbe_raid_siots_is_metadata_operation()
 ****************************************************************
 * @brief
 *  Paged metadata operations handle error paths differently in some cases.
 * 
 * @param siots_p - current I/O.               
 *
 * @return fbe_bool_t FBE_TRUE if metadata op
 *                   FBE_FALSE otherwise.
 *
 * @author
 *  6/18/2010 - Created. Rob Foley
 *  9/13/2013 - move the main code to fbe_raid_iots_is_metadata_operation.
 *
 ****************************************************************/

fbe_bool_t fbe_raid_siots_is_metadata_operation(fbe_raid_siots_t *siots_p)
{
    return fbe_raid_iots_is_metadata_operation(fbe_raid_siots_get_iots(siots_p));
}
/******************************************
 * end fbe_raid_siots_is_metadata_operation()
 ******************************************/

/*!**************************************************************
 * fbe_raid_siots_is_monitor_initiated_op()
 ****************************************************************
 * @brief
 *  Raid lib cannot wait for shutdown continue on monitor initiated
 *  operations.
 * 
 * @param siots_p - current I/O.               
 *
 * @return fbe_bool_t FBE_TRUE if monitor op
 *                   FBE_FALSE otherwise.
 *
 * @author
 *  2/07/2012 - Created. Vamsi V
 *
 ****************************************************************/

fbe_bool_t fbe_raid_siots_is_monitor_initiated_op(fbe_raid_siots_t *siots_p)
{
    fbe_raid_iots_t* iots_p;
    fbe_packet_t * packet_p;
    fbe_raid_geometry_t *raid_geometry_p = NULL;
    fbe_object_id_t object_id;

    iots_p = fbe_raid_siots_get_iots(siots_p);
    packet_p = fbe_raid_iots_get_packet(iots_p); 

    raid_geometry_p = fbe_raid_iots_get_raid_geometry(iots_p);
    object_id = fbe_raid_geometry_get_object_id(raid_geometry_p);
    
    /* If this is a background operation or a metadata operation
     * initiated on behalf of montior operation, return true.
     */
    if ((fbe_raid_siots_is_background_request(siots_p)) ||
        fbe_transport_is_monitor_packet(packet_p, object_id))
    {
        return FBE_TRUE;
    }
    else
    {
        return FBE_FALSE;
    }
}
/***********************************************
 * end fbe_raid_siots_is_monitor_initiated_op()
 **********************************************/
/*!**************************************************************
 * fbe_raid_siots_is_small_request()
 ****************************************************************
 * @brief
 *  Simply determines if this is a `small' request (one that does
 *  not require either a resource or size check) or not.
 * 
 * @param siots_p - current I/O.               
 *
 * @return fbe_bool_t FBE_TRUE not additional checking is required
 *                   FBE_FALSE otherwise.
 *
 ****************************************************************/
fbe_bool_t fbe_raid_siots_is_small_request(fbe_raid_siots_t *siots_p)
{
    fbe_bool_t  b_small_request = FBE_FALSE;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_block_count_t   max_per_drive_request_blocks;

    fbe_raid_geometry_get_max_blocks_per_drive(raid_geometry_p, 
                                               &max_per_drive_request_blocks);

    /* Simply validate that request is small enough for a the drive
     * to handle the request and we can get resources for the request.
     * Even if the request is unaligned the max per drive blocks is a
     * multiple of the alignment.
     */
    if (((max_per_drive_request_blocks > 0)                      &&
         (siots_p->parity_count <= max_per_drive_request_blocks)    ) &&
        (siots_p->parity_count <= FBE_RAID_MAX_SG_DATA_ELEMENTS)         )
    {
        b_small_request = FBE_TRUE;
    }

    return(b_small_request);
}
/* end of fbe_raid_siots_is_small_request() */

/*!***************************************************************************
 *          fbe_raid_siots_check_fru_request_limit()
 *****************************************************************************
 * 
 * @brief   This method checks the request against the maximum request that
 *          each drive (fru) can accept.  If any drive request exceeds the
 *          maximum number of blocks the drive can accept, then a status of
 *          FBE_STATUS_INSUFFICIENT_RESOURCES is returned.
 * 
 * @param   siots_p - current I/O
 * @param   read_info_p - Pointer to read information per position
 * @param   write_info_p - Pointer to write information per position
 * @param   read2_info_p - Pointer to read information per position
 * @param   in_status - The current status of the request
 *
 * @return  status - FBE_STATUS_OK - This request doesn't exceed the maximum
 *                      per-drive request size.
 *                   FBE_STATUS_INSUFFICIENT_RESOURCES - This request exceeds
 *                      the maximum per-drive request size
 * 
 * @note    We have already validated the maximum blocks per drive against the
 *          element size and max optimal block size.
 * 
 * @note    The reason we don't trust the parity_count is to handle cases like
 *          pre-read where the fruts request size may exceed the parity_count.
 *          If it is found that this case cannot or should not occur then only
 *          the siots is required.
 * 
 * @author
 *  07/15/2010  Ron Proulx  - Created
 *
 *****************************************************************************/
fbe_status_t fbe_raid_siots_check_fru_request_limit(fbe_raid_siots_t *siots_p,
                                                    fbe_raid_fru_info_t *read_info_p,
                                                    fbe_raid_fru_info_t *write_info_p,
                                                    fbe_raid_fru_info_t *read2_info_p,
                                                    fbe_status_t in_status)
{
    fbe_u32_t           width = fbe_raid_siots_get_width(siots_p);
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p); 
    fbe_block_count_t   max_blocks_per_drive = 0;
    fbe_u16_t           position;
    fbe_element_size_t  element_size; 
    fbe_lba_t parity_start = siots_p->parity_start;
    fbe_lba_t parity_end = siots_p->parity_start + siots_p->parity_count - 1;

    /* If we have already exceeded the limits, then return the current status
     */
    if (in_status != FBE_STATUS_OK)
    {
        return(in_status);
    }

    /* For `buffered' (i.e. non-disk transfers) we don't limit the request.
     * We also need to validate that the algorithm is zero since unaligned
     * zero requests will be changed to write requests.
     */
    if ((fbe_raid_siots_is_buffered_op(siots_p) == FBE_TRUE) &&
        (siots_p->algorithm == RG_ZERO)                         )
    {
        return(FBE_STATUS_OK);
    }
    /* Get the maxium per-drive request size and then check each valid position.
     */
    fbe_raid_geometry_get_max_blocks_per_drive(raid_geometry_p, &max_blocks_per_drive);

    /* For RAID 0 alone align the request to the element size instead of optimal block size
       since the striper recovery verify aligns the request to the stripe size
    */
    element_size = fbe_raid_siots_get_blocks_per_element(siots_p);    
    if( raid_geometry_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0 )
    {
        fbe_raid_generate_aligned_request(&parity_start, &parity_end, element_size);
    }
    else
    {
        fbe_raid_generate_aligned_request(&parity_start, &parity_end, raid_geometry_p->optimal_block_size);
    }
    

    /* We do an alignment and check if we are over the max number of blocks.
     * This is because if we get an error we might move to verify, 
     * which will perform I/Os on the optimal alignment.
     */
    if ( (parity_end - parity_start + 1) > max_blocks_per_drive)
    {
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    for (position = 0; position < width; position++)
    {
        /* We only care about valid read positions.
         */
        if (read_info_p != NULL)
        {
            if ((read_info_p->position == position)   &&
                (read_info_p->lba != FBE_LBA_INVALID) &&
                (read_info_p->blocks > 0)                )
            {
                fbe_lba_t read_start = read_info_p->lba;
                fbe_lba_t read_end = read_info_p->lba + read_info_p->blocks - 1;

                if( raid_geometry_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0 )
                {
                    fbe_raid_generate_aligned_request(&read_start, &read_end, element_size);
                }
                else
                {
                    fbe_raid_generate_aligned_request(&read_start, &read_end, raid_geometry_p->optimal_block_size);
                }                
                
                /* If we have exceeded the limit log a debug message and return an error
                 */
                if (read_info_p->blocks > max_blocks_per_drive)
                {
                    /* Log a debug message and return failure.
                     */
                    fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_DEBUG_HIGH,
                                         "Read position %d request size: 0x%llx exceeds max allowed: 0x%llx \n",
                                         read_info_p->position,
					(unsigned long long)read_info_p->blocks,
					(unsigned long long)max_blocks_per_drive);
                    return(FBE_STATUS_INSUFFICIENT_RESOURCES);
                }

                /* We do an alignment and check if we are over the max number of blocks.
                 * This is because if we get an error we might move to verify, 
                 * which will perform I/Os on the optimal alignment.
                 */
                if ( (read_end - read_start + 1) > max_blocks_per_drive)
                {
                    return FBE_STATUS_INSUFFICIENT_RESOURCES;
                }
            }
            read_info_p++;

        } /* end if read info is valid */

        /* We only care about valid write positions.
         */
        if (write_info_p != NULL)
        {
            if ((write_info_p->position == position)   &&
                (write_info_p->lba != FBE_LBA_INVALID) &&
                (write_info_p->blocks > 0)                )
            {
                fbe_lba_t write_start = write_info_p->lba;
                fbe_lba_t write_end = write_info_p->lba + write_info_p->blocks - 1;

                if( raid_geometry_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0)
                {
                    fbe_raid_generate_aligned_request(&write_start, &write_end, element_size);
                }
                else
                {
                    fbe_raid_generate_aligned_request(&write_start, &write_end, raid_geometry_p->optimal_block_size);
                }                
            
                /* If we have exceeded the limit log a debug message and return an error
                 */
                if (write_info_p->blocks > max_blocks_per_drive)
                {
                    /* Log a debug message and return failure.
                     */
                    fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_DEBUG_HIGH,
                                     "Write position %d request size: 0x%llx exceeds max allowed: 0x%llx \n",
                                     write_info_p->position,
				     (unsigned long long)write_info_p->blocks,
				     (unsigned long long)max_blocks_per_drive);
                    return(FBE_STATUS_INSUFFICIENT_RESOURCES);
                }

                /* We do an alignment and check if we are over the max number of blocks.
                 * This is because if we get an error we might move to verify, 
                 * which will perform I/Os on the optimal alignment.
                 */
                if ( (write_end - write_start + 1) > max_blocks_per_drive)
                {
                    return FBE_STATUS_INSUFFICIENT_RESOURCES;
                }
            }
            write_info_p++;

        } /* end if write info is valid */

        /* We only care about valid read2 positions.
         */
        if (read2_info_p != NULL)
        {
            if ((read2_info_p->position == position)   &&
                (read2_info_p->lba != FBE_LBA_INVALID) &&
                (read2_info_p->blocks > 0)                )
            {
                fbe_lba_t read_start = read2_info_p->lba;
                fbe_lba_t read_end = read2_info_p->lba + read2_info_p->blocks - 1;

                fbe_raid_generate_aligned_request(&read_start, &read_end, raid_geometry_p->optimal_block_size);

                /* If we have exceeded the limit log a debug message and return an error
                 */
                if (read2_info_p->blocks > max_blocks_per_drive)
                {
                    /* Log a debug message and return failure.
                     */
                    fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_DEBUG_HIGH,
                                     "Read2 position %d request size: 0x%llx exceeds max allowed: 0x%llx\n",
                                     read2_info_p->position,
				     (unsigned long long)read2_info_p->blocks,
				     (unsigned long long)max_blocks_per_drive);
                    return(FBE_STATUS_INSUFFICIENT_RESOURCES);
                }

                /* We do an alignment and check if we are over the max number of blocks.
                 * This is because if we get an error we might move to verify, 
                 * which will perform I/Os on the optimal alignment.
                 */
                if ( (read_end - read_start + 1) > max_blocks_per_drive)
                {
                    return FBE_STATUS_INSUFFICIENT_RESOURCES;
                }
            }
            read2_info_p++;

        } /* end if read2 info is valid */

    } /* end for all positions in raid group */

    /* Also need to check the parity (i.e. physical) request range
     */
    if (siots_p->parity_count > max_blocks_per_drive)
    {
        /* Log a debug message and return failure.
         */
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_DEBUG_HIGH,
                             "siots lba: 0x%llx parity_start: 0x%llx count: 0x%llx exceeds max allowed: 0x%llx\n",
                             (unsigned long long)siots_p->start_lba,
			     (unsigned long long)siots_p->parity_start,
			     (unsigned long long)siots_p->parity_count,
			     (unsigned long long)max_blocks_per_drive);
        return(FBE_STATUS_INSUFFICIENT_RESOURCES);
    }

    /* Always return the execution status
     */
    return(FBE_STATUS_OK);
}
/**********************************************
 * end fbe_raid_siots_check_fru_request_limit()
 **********************************************/


/*!***************************************************************************
 *          fbe_raid_siots_get_failed_io_pos_bitmap()
 *****************************************************************************
 *
 * @brief   Get a bitmap of failed positions in the given siots.
 *  
 * @param   in_siots_p - Pointer to siots to evaluate
 * @param   out_failed_io_position_bitmap_p - Pointer to bitmap of failed positions
 * 
 * @return  status - Typically FBE_STATUS_OK
 *
 * @author
 *  07/2010  rundbs  - Created
 *
 *****************************************************************************/
fbe_status_t fbe_raid_siots_get_failed_io_pos_bitmap(fbe_raid_siots_t   *const in_siots_p,
                                                     fbe_u32_t          *out_failed_io_position_bitmap_p)
{
    fbe_raid_fruts_t        *fruts_p;
    fbe_u32_t               error_bitmap;
    fbe_u32_t               running_total_error_bitmap;


    //  Initialize local variables
    error_bitmap                = 0;
    running_total_error_bitmap  = 0;

    //  Check the incoming siots status
    if (fbe_raid_siots_is_flag_set(in_siots_p, FBE_RAID_SIOTS_FLAG_WAITING_SHUTDOWN_CONTINUE))
    {
        //  Get a pointer to the read fruts chain
        fbe_raid_siots_get_read_fruts(in_siots_p, &fruts_p);

        //  Get the error bitmap for the fruts chain and add to a running total
        fbe_raid_fruts_get_failed_io_pos_bitmap(fruts_p, &error_bitmap);
        running_total_error_bitmap |= error_bitmap;

        //  Get a pointer to the read2 fruts chain
        fbe_raid_siots_get_read2_fruts(in_siots_p, &fruts_p);

        //  Get the error bitmap for the fruts chain and add to a running total
        fbe_raid_fruts_get_failed_io_pos_bitmap(fruts_p, &error_bitmap);
        running_total_error_bitmap |= error_bitmap;

        //  Get a pointer to the write fruts chain
        fbe_raid_siots_get_write_fruts(in_siots_p, &fruts_p);

        //  Get the error bitmap for the fruts chain and add to a running total
        fbe_raid_fruts_get_failed_io_pos_bitmap(fruts_p, &error_bitmap);
        running_total_error_bitmap |= error_bitmap;
    }

    //  Set the output parameter to the running total
    *out_failed_io_position_bitmap_p = running_total_error_bitmap;

    return FBE_STATUS_OK;

}
/**********************************************
 * end fbe_raid_siots_get_failed_io_pos_bitmap()
 **********************************************/

/*!***************************************************************************
 *          fbe_raid_siots_get_fruts_by_position()                              
 *****************************************************************************
 *
 * @brief
 *    This function returns the fruts for the given fru position.
 *
 * @param  head_p - The ptr to the head of the chain.
 * @param  position - fru position in the raid group
 *
 * @return
 *     fbe_raid_fruts_t  - The first fruts in the chain with this position
 *
 * @author
 *     08/20/2010   Pradnya Patankar - Created
 *
 *************************************************************************/
fbe_raid_fruts_t * fbe_raid_siots_get_fruts_by_position(fbe_raid_fruts_t *read_fruts_p,
                                                        fbe_u16_t position,
                                                        fbe_u32_t read_count)
{
    fbe_raid_fruts_t *fruts_p = NULL;
    fbe_u32_t count = 0;
    fruts_p = read_fruts_p;

    /* Simply loop over the chain and break when we hit 
     * the fruts with the position we are looking for. 
     */
    for( count = 0; count < read_count; count++)
    {
        if (fruts_p->position == position)
        {
            break;
        }
        fruts_p = fbe_raid_fruts_get_next(fruts_p);
    }
    /*for (fruts_p = head_p;
          fruts_p != NULL;
          fruts_p = fbe_raid_fruts_get_next(fruts_p))
    {
        if (fruts_p->position == position)
        {
            break;
        }
    }*/
    return fruts_p;
}
/* end of fbe_raid_siots_get_fruts_by_position() */

/*!**************************************************************
 * fbe_raid_siots_log_traffic()
 ****************************************************************
 * @brief
 *  We will display basic info on this siots when siots
 *  tracing is enabled.
 *
 * @param siots_p - Siots to trace.
 * @param string_p - A string describing our reason for tracing.
 *
 * @return None.   
 *
 * @author
 *  1/31/2011 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_raid_siots_log_traffic(fbe_raid_siots_t *const siots_p, char *const string_p)
{
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_payload_block_operation_opcode_t opcode;
    fbe_block_count_t blocks;
    fbe_lba_t lba;
    fbe_raid_iots_t *iots_p = fbe_raid_siots_get_iots(siots_p);
    fbe_raid_position_bitmask_t degraded_bitmask;

    /*! Display traffic when tracing is enabled.
     */ 
    if (fbe_raid_geometry_is_debug_flag_set(raid_geometry_p, FBE_RAID_LIBRARY_DEBUG_FLAG_SIOTS_TRACING))
    {
        fbe_raid_siots_get_opcode(siots_p, &opcode);
        fbe_raid_siots_get_blocks(siots_p, &blocks);
        fbe_raid_siots_get_lba(siots_p, &lba);
        fbe_raid_siots_get_degraded_bitmask(siots_p, &degraded_bitmask);
    
        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_INFO,
                                    "%s %x op:%x i-lba/bl:%llx/%llx ifl: %x sfl:%x\n",
                                    string_p,
				    fbe_raid_memory_pointer_to_u32(siots_p),
				    opcode, (unsigned long long)lba,
				    (unsigned long long)blocks,
                                    iots_p->flags, siots_p->flags);
        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_INFO,
                                    "siots %x s-lba/bl:%llx/%llx pa-lba/bl:%llx/%llx alg:0x%x d:%x\n",
                                    fbe_raid_memory_pointer_to_u32(siots_p), siots_p->start_lba, siots_p->xfer_count, 
                              siots_p->parity_start, siots_p->parity_count, siots_p->algorithm, degraded_bitmask);
    }
    return;
}
/******************************************
 * end fbe_raid_siots_log_traffic()
 ******************************************/
/*!**************************************************************
 * fbe_raid_siots_log_error()
 ****************************************************************
 * @brief
 *  Display information on a siots that is completing with error.
 *
 * @param siots_p - Siots to trace.
 * @param string_p - A string describing our reason for tracing.
 *
 * @return None.   
 *
 * @author
 *  1/31/2011 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_raid_siots_log_error(fbe_raid_siots_t *const siots_p, char *const string_p)
{
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_payload_block_operation_opcode_t opcode;
    fbe_block_count_t blocks;
    fbe_lba_t lba;
    fbe_payload_block_operation_status_t siots_status;
    fbe_payload_block_operation_qualifier_t siots_qualifier;

    /* Display siots when error tracing is enabled
     */
    if (fbe_raid_geometry_is_debug_flag_set(raid_geometry_p, FBE_RAID_LIBRARY_DEBUG_FLAG_SIOTS_ERROR_TRACING))
    {
        fbe_raid_iots_t *iots_p = fbe_raid_siots_get_iots(siots_p);
        fbe_time_t current_time = fbe_get_time();
        fbe_time_t iots_age = fbe_get_elapsed_milliseconds(iots_p->time_stamp);
        fbe_raid_siots_get_opcode(siots_p, &opcode);
        fbe_raid_siots_get_blocks(siots_p, &blocks);
        fbe_raid_siots_get_lba(siots_p, &lba);
        fbe_raid_siots_get_block_status(siots_p, &siots_status);
        fbe_raid_siots_get_block_qualifier(siots_p, &siots_qualifier);

        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_INFO,
                                    "%s siots: %x iots flags: 0x%x siots flags: 0x%x\n",
                                    string_p, fbe_raid_memory_pointer_to_u32(siots_p), 
                               fbe_raid_iots_get_flags(iots_p), fbe_raid_siots_get_flags(siots_p));
        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_INFO,
                                    "siots: %x time: 0x%llx iots_time: 0x%llx iots_age: 0x%llx\n",
                                    fbe_raid_memory_pointer_to_u32(siots_p),
                                    (unsigned long long)current_time,
                                    (unsigned long long)iots_p->time_stamp,
                                    (unsigned long long)iots_age);
        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_INFO,
                                    "siots: %x iots - op:0x%x lba:0x%llX bl:0x%llx pkt: %p\n",
                                    fbe_raid_memory_pointer_to_u32(siots_p), opcode,
                                    (unsigned long long)lba,
                                    (unsigned long long)blocks, iots_p->packet_p); 
        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_INFO,
                                    "siots: %x lba:0x%llX bl:0x%llx alg:0x%x sts:0x%x qual:0x%x\n",
                                    fbe_raid_memory_pointer_to_u32(siots_p),
                                    (unsigned long long)siots_p->start_lba,
                                    (unsigned long long)siots_p->xfer_count, 
                                    siots_p->algorithm, siots_status, siots_qualifier); 
        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_INFO,
                                    "siots: %x i:%d [0]:%p [1]%p [2]%p\n",
                                    fbe_raid_memory_pointer_to_u32(siots_p), siots_p->state_index,
                                    siots_p->prevState[0], siots_p->prevState[1], siots_p->prevState[2]);  
        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_INFO,
                                    "siots: %x i:%d [3]%p [4]%p [5]%p\n",
                                    fbe_raid_memory_pointer_to_u32(siots_p),
                                    siots_p->state_index,
                                    siots_p->prevState[3], siots_p->prevState[4],
                                    siots_p->prevState[5]); 
    }

    /* Generate an error trace if enabled.
     */
    if (fbe_raid_geometry_is_debug_flag_set(raid_geometry_p, FBE_RAID_LIBRARY_DEBUG_FLAG_ERROR_TRACE_SIOTS_ERROR))
    {
        fbe_trace_level_t   trace_level = FBE_TRACE_LEVEL_ERROR;

        fbe_raid_siots_get_block_status(siots_p, &siots_status);
        fbe_raid_siots_get_block_qualifier(siots_p, &siots_qualifier);

        /*! @note For a virtual drive generate a warning.
         */
        if (fbe_raid_geometry_get_class_id(raid_geometry_p) == FBE_CLASS_ID_VIRTUAL_DRIVE)
        {
            trace_level = FBE_TRACE_LEVEL_WARNING;
        }
        fbe_raid_siots_object_trace(siots_p, trace_level, __LINE__, __FUNCTION__,
                               "siots: %x lba:0x%llX bl:0x%llx alg:0x%x sts:0x%x qual:0x%x\n",
                               fbe_raid_memory_pointer_to_u32(siots_p), siots_p->start_lba, siots_p->xfer_count, 
                               siots_p->algorithm, siots_status, siots_qualifier);
    }
    return;
}
/******************************************
 * end fbe_raid_siots_log_error()
 ******************************************/

/*!******************************************************************
 * fbe_raid_siots_is_startable
 *******************************************************************
 * @brief
 *  Determine if this SIOTS can be started, or
 *  if it must wait for prior SIOTS to clear from the pipeline.
 *
 *  A SIOTS can be started immediately if it is a read,
 *  since overlapping reads within a stripe are acceptable.
 *
 *  Other operations modify the parity range, and thus cannot overlap.
 *  These types of operations wait for any prior overlapping SIOTS
 *  clear from the pipeline.
 *  SIOTS that are not immediately startable will be
 *  restarted by other SIOTS in the pipeline.
 *
 * @param siots_p - The current siots.
 *
 * @notes Assumes iots lock is held by the caller.
 *
 * @return fbe_bool_t FBE_TRUE if can start.
 *
 *******************************************************************/

fbe_bool_t fbe_raid_siots_is_startable(fbe_raid_siots_t *const siots_p)
{
    fbe_raid_iots_t *iots_p = fbe_raid_siots_get_iots(siots_p);
    fbe_payload_block_operation_opcode_t opcode;
    fbe_raid_siots_get_opcode(siots_p, &opcode);

    if ( fbe_raid_iots_is_flag_set(iots_p, FBE_RAID_IOTS_FLAG_ERROR) &&
         (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ) &&
         (fbe_raid_siots_get_prev(siots_p) == NULL) )
    {
        return FBE_TRUE;
    }
    else
    {
        fbe_bool_t fbe_raid_siots_get_lock(fbe_raid_siots_t * siots_p);
        return fbe_raid_siots_get_lock(siots_p);
    }
}
/**************************************
 * end fbe_raid_siots_is_startable()
 **************************************/

/*!**************************************************************
 * fbe_raid_siots_wait_previous()
 ****************************************************************
 * @brief
 *  Wait for a previous siots to finish.
 *  This will transition us into a single threaded mode.
 *
 * @param siots_p -  Current I/O.               
 *
 * @return FBE_RAID_STATE_STATUS_EXECUTING if no need to wait.
 *
 * @author
 *  8/15/2011 - Created. Rob Foley
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_raid_siots_wait_previous(fbe_raid_siots_t *const siots_p)
{
    fbe_raid_state_status_t status = FBE_RAID_STATE_STATUS_WAITING;
    fbe_raid_siots_t *restart_siots_p = NULL;
    fbe_raid_iots_t *iots_p = fbe_raid_siots_get_iots(siots_p);

    /* This chain of siots has dropped into single thread mode.
     * Wait until peer wakes us up.
     */
    fbe_raid_iots_lock(iots_p);
    if (fbe_raid_siots_get_prev(siots_p) != NULL)
    {
        fbe_raid_siots_set_flag(siots_p, (FBE_RAID_SIOTS_FLAG_WAIT_LOCK | 
                                          FBE_RAID_SIOTS_FLAG_WAS_DELAYED));
        fbe_raid_siots_restart_upgrade_waiter(siots_p, &restart_siots_p);
        fbe_raid_iots_unlock(iots_p);

        if (restart_siots_p != NULL)
        {
            /* Restart any waiting siots.
             */
            fbe_raid_common_state(((fbe_raid_common_t *) restart_siots_p));
        }
        status = FBE_RAID_STATE_STATUS_WAITING;
    }
    else
    {
        fbe_raid_iots_unlock(iots_p);
        status = FBE_RAID_STATE_STATUS_EXECUTING;
    }

    return status;
}
/******************************************
 * end fbe_raid_siots_wait_previous()
 ******************************************/

/*!**************************************************************
 * fbe_raid_siots_get_statistics()
 ****************************************************************
 * @brief
 *  Fetch the statistics for this siots.
 *
 * @param None.       
 *
 * @param siots_p
 * @param stats_p
 *
 * @author
 *  4/29/2013 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_raid_siots_get_statistics(fbe_raid_siots_t *siots_p,
                                   fbe_raid_library_statistics_t *stats_p)
{
    fbe_raid_fruts_t *fruts_p = NULL;

    fbe_raid_siots_get_read_fruts(siots_p, &fruts_p);
    fbe_raid_fruts_get_stats(fruts_p, &stats_p->total_fru_stats[0]);
    fbe_raid_fruts_get_stats_filtered(fruts_p, stats_p);

    fbe_raid_siots_get_write_fruts(siots_p, &fruts_p);
    fbe_raid_fruts_get_stats(fruts_p, &stats_p->total_fru_stats[0]);
    fbe_raid_fruts_get_stats_filtered(fruts_p, stats_p);

    fbe_raid_siots_get_read2_fruts(siots_p, &fruts_p);
    fbe_raid_fruts_get_stats(fruts_p, &stats_p->total_fru_stats[0]);
    fbe_raid_fruts_get_stats_filtered(fruts_p, stats_p);
 
}
/**************************************
 * end fbe_raid_siots_get_statistics
 **************************************/

/*!**************************************************************
 * fbe_raid_siots_get_no_data_bitmap()
 ****************************************************************
 * @brief
 *  Fetch information about which positions do not have data.
 *
 * @param None.       
 *
 * @param siots_p
 * @param bitmap_p - Output bitmap of positions we should not read from.
 *
 * @author
 *  12/13/2013 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_raid_siots_get_no_data_bitmap(fbe_raid_siots_t *siots_p,
                                       fbe_raid_position_bitmask_t *bitmap_p)
{
    fbe_raid_iots_t *iots_p = fbe_raid_siots_get_iots(siots_p);
    *bitmap_p = iots_p->chunk_info[0].needs_rebuild_bits;
}
/**************************************
 * end fbe_raid_siots_get_no_data_bitmap()
 **************************************/

/*!**************************************************************
 * fbe_raid_siots_is_able_to_quiesce()
 ****************************************************************
 * @brief
 *  check whether the associated packet could be quiesced (FBE_PACKET_FLAG_DO_NOT_QUIESCE)
 *
 * @param None.
 *
 * @param siots_p
 *
 * @author
 *  10/08/2014 - Created. Geng Han
 *
 ****************************************************************/
fbe_bool_t fbe_raid_siots_is_able_to_quiesce(fbe_raid_siots_t *siots_p)
{
    fbe_packet_attr_t   packet_attributes = 0;
    fbe_raid_iots_t *iots_p = fbe_raid_siots_get_iots(siots_p);
    fbe_packet_t * packet_p = fbe_raid_iots_get_packet(iots_p); 

    fbe_transport_get_packet_attr(packet_p, &packet_attributes);

    return (packet_attributes & FBE_PACKET_FLAG_DO_NOT_QUIESCE) ? FBE_FALSE : FBE_TRUE;
}
/**************************************
 * end fbe_raid_siots_is_able_to_quiesce()
 **************************************/

/*****************************
 * end file fbe_raid_siots.c
 *****************************/
