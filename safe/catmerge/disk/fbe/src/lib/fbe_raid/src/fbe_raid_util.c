/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_raid_util.c
 ***************************************************************************
 *
 * @brief
 *  This file contains general functions of the raid library.
 *
 * @version
 *   8/18/2009:  Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe_raid_library_proto.h"
#include "fbe_raid_geometry.h"

/*************************
 * LOCAL PROTOTYPES
 *************************/
static fbe_bool_t fbe_raid_siots_should_restart_lock_waiters(fbe_raid_siots_t * siots_p,
                                                             fbe_bool_t *b_restart);

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/*! @note   Global option flags are not associated with any particular raid group.
 */
static fbe_raid_option_flags_t fbe_raid_library_option_flags = FBE_RAID_OPTION_DEFAULT_FLAGS;

fbe_raid_option_flags_t fbe_raid_get_option_flags(fbe_raid_geometry_t *raid_geometry_p)
{
    return fbe_raid_library_option_flags;
}
fbe_bool_t fbe_raid_is_option_flag_set(fbe_raid_geometry_t *raid_geometry_p,
                                                fbe_raid_option_flags_t flags)
{
    return ((fbe_raid_library_option_flags & flags) == flags);
}
static __inline void fbe_raid_init_option_flags(fbe_raid_geometry_t *raid_geometry_p,
                                         fbe_raid_option_flags_t flags)
{
    fbe_raid_library_option_flags = flags;
    return;
}

static __inline void fbe_raid_set_options_flag(fbe_raid_geometry_t *raid_geometry_p,
                                        fbe_raid_option_flags_t flag)
{
    fbe_raid_library_option_flags |= flag;
    return;
}

static __inline void fbe_raid_clear_options_flag(fbe_raid_geometry_t *raid_geometry_p, 
                                          fbe_raid_option_flags_t flag)
{
    fbe_raid_library_option_flags &= ~flag;
    return;
}

/************************************************************
 *  fbe_raid_util_get_valid_bitmask()
 ************************************************************
 *
 * @brief   Generate the bitmask from the width passed
 *
 * @param   width - The number of valid positions 
 *          valid_bitmask_p - bitmask of positions for this width
 * @return  
 *          fbe_status_t
 * @author
 *  12/15/2009  Ron Proulx  - Created
 *
 ************************************************************/
fbe_status_t fbe_raid_util_get_valid_bitmask(fbe_u32_t width, fbe_raid_position_bitmask_t *valid_bitmask_p)
{
    fbe_u32_t                   position, max_position;
    fbe_status_t                status = FBE_STATUS_OK;
    *valid_bitmask_p = 0;
    /* Simply set a bit for each position.
     */
    max_position = (sizeof(*valid_bitmask_p) * 8);
    if (width > max_position)
    {
        fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                "raid %s: width 0x%x > max_position 0x%x \n",
                                __FUNCTION__, width, max_position);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    max_position = width;
    for (position = 0; position < max_position; position++)
    {
        *valid_bitmask_p |= (1 << position);
    }

    /* Return the valid bitmask.
     */
    return status;
}
/*****************************************
 * fbe_raid_util_get_valid_bitmask()
 *****************************************/

/****************************************************************
 * fbe_raid_get_max_blocks()
 ****************************************************************
 * @brief
 *  Determine the max size for a Striper request.
 *  We limit the request to a fixed block size, but
 *  we also limit the request based on sg limits.
 *
 * @param siots_p - Current SIOTS ptr.
 * @param blocks - number of blocks in this request
 * @param blocks_per_element - element size of unit
 * @param width - width of unit
 *
 * @return VALUE:
 *   fbe_u32_t
 *
 * @author
 *   3/16/01 - Created. Rob Foley
 *   4/24/01 - Rewrote.  Jing Jin
 *
 ****************************************************************/

fbe_u32_t fbe_raid_get_max_blocks(fbe_raid_siots_t *const siots_p,
                                  fbe_lba_t lba,
                                  fbe_u32_t blocks,
                                  fbe_u32_t width,
                                  fbe_bool_t align)
{
    fbe_status_t status = FBE_STATUS_OK;
    /* Get the blocks per element from the dba function, the 
     * geometry block may not be filled out yet since generate
     * calls this.
     */
    fbe_lba_t blocks_per_element = fbe_raid_siots_get_blocks_per_element(siots_p);

    /* The sg_size is the number of blocks we will be using
     * per sg entry, per drive, and this is dependent upon the
     * blocks per element size.
     */
    fbe_u32_t sg_size = (fbe_u32_t) ((blocks_per_element >= fbe_raid_get_std_buffer_size())
        ? fbe_raid_get_std_buffer_size() : blocks_per_element);

    /* Limit of this I/O based on the sg limits.
     */
    fbe_u32_t sg_block_limit;

    /* The total_sgs, are the number of sgs per drive that
     * we are limited to.
     */
    fbe_u32_t total_sgs = FBE_RAID_MAX_SG_DATA_ELEMENTS;
    fbe_u32_t stripe_size = (fbe_u32_t) blocks_per_element * width;

    if ((width == 1) || (blocks_per_element == 1))
    {
        /* Leave total_sgs as they are.
         */
    }
    else
    {
        /* In the worst case scenario, a full element might be split into two buffers.
         */
        total_sgs /= 2;
    }

    /* If Specified, align this to a data element stripe.
     */
    if (align)
    {
        if ( (lba % stripe_size) ||
             ((lba + blocks) % stripe_size) )
        {
            /* Misaligned I/Os need to adjust our sg counts by 3.
             * This accounts for misalignment and for pre-reads. 
             */
            total_sgs -= 3;
        }
    }

    /* Cached ops might have an offset into a buffer, that could
     * cause an additional sg crossing.
     */
    if (!fbe_raid_siots_is_buffered_op(siots_p))
    {
        /* Create an sg descriptor for this request and adjust it to
         * determine if there is an offset into the first sg element
         * of this request.
         */
        fbe_sg_element_with_offset_t sg_desc;    /* tracks location in cache S/G list */
        status = fbe_raid_siots_setup_cached_sgd(siots_p, &sg_desc);
        if(RAID_COND(status != FBE_STATUS_OK))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                "status 0x%x != FBE_STATUS_OK while setup cached sgd\n",
                                status);
            return blocks;
        }
        fbe_raid_adjust_sg_desc(&sg_desc);
    
        /* If we have an offset, decrement the number of sgs by 1
         * since we will need to account for an additional sg.
         */
        if (sg_desc.offset != 0)
        {
            total_sgs--;
        }
    } /* end if not host op. */
    
        
    /* Calclulate the number of blocks that will fit into an sg.
     */
    sg_block_limit = width * total_sgs * sg_size;
    
    /* Do a min between blocks and
     * the total blocks that can fit in an sg.
     */
    blocks = FBE_MIN(blocks, sg_block_limit);

    /* The standard buffers are a power of 2 size
     * and standard element size are a power of 2.
     * Non-standard elements are not a power of 2, thus
     * we end up with fewer possible sg entries, as
     * each full element (element size < std buffer size)
     * or buffer chunk (element size > std buffer size)
     * may take up to 2 sg entries, rather than the 1 sg entry
     * which is common to the standard element size.
     */
    if(RAID_COND( (total_sgs == 0) ||
                  (sg_block_limit == 0) ) )
    {
        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                    "%s: total_sgs 0x%x == 0 Or sg_block_limit 0x%x == 0",
                                    __FUNCTION__, total_sgs, sg_block_limit);
        return blocks;
    }

    return blocks;
} 
/* end fbe_raid_get_max_blocks */

/****************************************************************
 *  fbe_raid_siots_can_restart
 ****************************************************************
 * @brief
 *  This function continues a single SIOTS.
 *
 * @param siots_p - A pointer to the SIOTS to continue.
 * 
 * @return
 *  none
 *
 * @author
 *  06/09/06:  Rob Foley - Created.
 *
 ****************************************************************/

fbe_bool_t fbe_raid_siots_can_restart(fbe_raid_siots_t * const siots_p)
{
    fbe_bool_t b_status = FBE_FALSE;

    if (fbe_raid_siots_is_flag_set(siots_p, FBE_RAID_SIOTS_FLAG_QUIESCED))
    {
        /* This needs to be woken up.
         */
        b_status = FBE_TRUE;
    }
    return b_status;
}
/*****************************************
 * fbe_raid_siots_can_restart()
 *****************************************/

/****************************************************************
 *  fbe_raid_iots_get_siots_to_restart
 ****************************************************************
 * @brief
 *  This function returns any siots associated with this iots
 *  that need to be restarted.
 *
 * @param iots_p - Pointer to the active iots.
 * @param restart_queue_p - The queue to add siots to restart.
 * @parram num_restarted_p - Ptr to number of siots restarted.
 * 
 * @return
 *  fbe_status_t
 *
 * @note assumes iots lock is held.
 *
 * @author
 *  10/23/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_iots_get_siots_to_restart(fbe_raid_iots_t * const iots_p,
                                                       fbe_queue_head_t *restart_queue_p,
                                                       fbe_u32_t *num_restarted_p)
{
    fbe_raid_siots_t *siots_p;
    fbe_bool_t b_status;
    fbe_u32_t num_restarted = 0;

    if (!fbe_raid_common_is_flag_set((fbe_raid_common_t*)iots_p,
                                     FBE_RAID_COMMON_FLAG_TYPE_IOTS))
    {
        /* Something is wrong, we expected a raid iots.
         */
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Loop through each siots_p and check for the
     * waiting for shutdown or continue flag.
     */
    siots_p = (fbe_raid_siots_t *)fbe_queue_front(&iots_p->siots_queue);
    while (siots_p != NULL)
    {
        fbe_raid_siots_t *nsiots_p = (fbe_raid_siots_t *) fbe_queue_front(&siots_p->nsiots_q);

        /* Determine if this siots needs to be restarted.
         */
        b_status = fbe_raid_siots_can_restart(siots_p);
        if (b_status)
        {
            fbe_raid_siots_clear_flag(siots_p, FBE_RAID_SIOTS_FLAG_QUIESCED);
            fbe_raid_common_wait_enqueue_tail(restart_queue_p, &siots_p->common);
            num_restarted++;
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_DEBUG_HIGH,
                                        "%s continue siots 0x%x fl: 0x%x\n", 
                                        __FUNCTION__, fbe_raid_memory_pointer_to_u32(siots_p), siots_p->flags);
        }
                
        /* Check all the nested sub-iots and continue them as well.
         */
        while (nsiots_p != NULL)
        {
            b_status = fbe_raid_siots_can_restart(nsiots_p);
            if (b_status)
            {
                fbe_raid_siots_clear_flag(nsiots_p, FBE_RAID_SIOTS_FLAG_QUIESCED);
                fbe_raid_common_wait_enqueue_tail(restart_queue_p, &nsiots_p->common);
                num_restarted++;
                fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_DEBUG_HIGH,
                                            "%s continue nsiots 0x%x fl: 0x%x\n", 
                                            __FUNCTION__, fbe_raid_memory_pointer_to_u32(nsiots_p), nsiots_p->flags);
            }
            nsiots_p = fbe_raid_siots_get_next(nsiots_p);
        }
        siots_p = fbe_raid_siots_get_next(siots_p);
    }
    *num_restarted_p = num_restarted;
    return FBE_STATUS_OK;
}
/*****************************************
 * fbe_raid_iots_get_siots_to_restart()
 *****************************************/

/****************************************************************
 *  fbe_raid_siots_continue
 ****************************************************************
 * @brief
 *  This function marks a single SIOTS as continued for the
 *  input bitmask.
 * 
 *  We transition the siots from waiting for shutdown continue
 *  to quiesced.  This allows the generic unquiesce code to
 *  deal with restarting this siots.
 *
 * @param rgdb_p - A pointer to the fbe_raid_group_t for this group.
 * @param siots_p - A pointer to the SIOTS to continue.
 * @param continue_bitmask - The fru bitmask that is receiving continue.
 * @param aborting - FBE_TRUE  if we are aborting the request.
 *                        FBE_FALSE if we are continuing the request.
 * @return
 *  fbe_status_t
 *
 * @author
 *  06/09/06:  Rob Foley - Created.
 *
 ****************************************************************/

fbe_status_t fbe_raid_siots_continue(fbe_raid_siots_t * const siots_p,
                                     fbe_raid_geometry_t * const raid_geometry_p,
                                     fbe_u16_t continue_bitmask)
{
    fbe_raid_siots_set_flag(siots_p, FBE_RAID_SIOTS_FLAG_RECEIVED_CONTINUE);
    siots_p->continue_bitmask |= continue_bitmask;

    /* Clear our needs continue bitmask for the position we
     * just received a continue on.  When we receive a continue
     * on all positions that need it, this will become zero,
     * and we will wake up the siots below.
     */
    siots_p->needs_continue_bitmask &= ~continue_bitmask;
   
    if ( fbe_raid_siots_is_flag_set(siots_p, FBE_RAID_SIOTS_FLAG_WAITING_SHUTDOWN_CONTINUE) )
    {
        /* Clear the waiting shutdown-continue flag.
         */
        fbe_raid_siots_clear_flag(siots_p, FBE_RAID_SIOTS_FLAG_WAITING_SHUTDOWN_CONTINUE);
        if(RAID_COND(siots_p->wait_count != 0))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                           "raid: wait count 0x%llx is non Zero\n",
                                           siots_p->wait_count);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        fbe_raid_siots_set_flag(siots_p, FBE_RAID_SIOTS_FLAG_QUIESCED);
       
        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_DEBUG_HIGH,
                                    "%s continue siots:0x%p flags: 0x%x Lba:0x%llX Blks:0x%llx\n",
                                    __FUNCTION__, siots_p, siots_p->flags, 
                                    (unsigned long long)siots_p->start_lba, (unsigned long long)siots_p->xfer_count);
    }
    return FBE_STATUS_OK;
}
/*****************************************
 * fbe_raid_siots_continue()
 *****************************************/
/****************************************************************
 *  fbe_raid_iots_continue
 ****************************************************************
 * @brief
 *  This function continues and IOTS and all associated siots.
 *
 * @param iots_p - Pointer to the active iots.
 * @param continue_bitmask - The fru bitmask that is receiving continue.
 * 
 * @return
 *  fbe_status_t
 *
 * @author
 *  10/23/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_iots_continue(fbe_raid_iots_t * const iots_p,
                                    fbe_u16_t continue_bitmask)
{
    fbe_raid_siots_t *siots_p;
    fbe_packet_t   *packet_p = NULL;
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_metadata_operation_t * metadata_operation = NULL;

    if (!fbe_raid_common_is_flag_set((fbe_raid_common_t*)iots_p,
                                    FBE_RAID_COMMON_FLAG_TYPE_IOTS))
    {
        /* Something is wrong, we expected a raid iots.
         */
        return FBE_STATUS_GENERIC_FAILURE;
    }

    packet_p = fbe_raid_iots_get_packet(iots_p);
    sep_payload = fbe_transport_get_payload_ex(packet_p);
    metadata_operation =  fbe_payload_ex_check_metadata_operation(sep_payload);
    /* We should never see metadata operation on the top if we are ready to start IO */
    if (metadata_operation!=NULL)
    {
        fbe_raid_iots_object_trace(iots_p, FBE_RAID_IOTS_TRACE_PARAMS_CRITICAL, 
                                   "%s continue iots %p with metadata operation atop\n", 
                                   __FUNCTION__, iots_p);
    }

    /* Loop through each siots_p and check for the
     * waiting for shutdown or continue flag.
     */
    fbe_raid_iots_lock(iots_p);
    siots_p = (fbe_raid_siots_t *)fbe_queue_front(&iots_p->siots_queue);
    while (siots_p != NULL)
    {
        fbe_raid_siots_t *nsiots_p = (fbe_raid_siots_t *) fbe_queue_front(&siots_p->nsiots_q);

        /* Give a continue to this siots.
         */
        fbe_raid_siots_continue( siots_p, fbe_raid_iots_get_raid_geometry(iots_p), continue_bitmask );
        
        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_DEBUG_HIGH,
                                    "%s continue siots %p continue_bits 0x%x\n", 
                                    __FUNCTION__, siots_p, continue_bitmask);
                
        /* Check all the nested sub-iots and continue them as well.
         */
        while (nsiots_p != NULL)
        {
            fbe_raid_siots_continue( nsiots_p, fbe_raid_iots_get_raid_geometry(iots_p), continue_bitmask );
            nsiots_p = fbe_raid_siots_get_next(nsiots_p);
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_DEBUG_HIGH,
                                        "%s continue nsiots %p continue_bits 0x%x\n", 
                                        __FUNCTION__, nsiots_p, continue_bitmask);
        }
        siots_p = fbe_raid_siots_get_next(siots_p);
    }
    fbe_raid_iots_unlock(iots_p);
    return FBE_STATUS_OK;
}
/*****************************************
 * fbe_raid_iots_continue()
 *****************************************/

/****************************************************************
 *  fbe_raid_siots_is_quiesced
 ****************************************************************
 * @brief
 *  This function checks if a siots is quiesced.
 *
 * @param siots_p - Pointer to the active siots.
 * 
 * @return
 *  fbe_status_t
 *
 * @author
 *  10/23/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_bool_t fbe_raid_siots_is_quiesced(fbe_raid_siots_t * const siots_p)
{
    fbe_bool_t b_quiesced = FBE_FALSE;

    /* If any of the flags indicating a wait state are set, then 
     * this siots is quiesced. 
     */
    if (fbe_raid_siots_is_flag_set(siots_p, FBE_RAID_SIOTS_FLAG_QUIESCED) ||
        fbe_raid_siots_is_flag_set(siots_p, FBE_RAID_SIOTS_FLAG_WAITING_SHUTDOWN_CONTINUE) || 
        fbe_raid_siots_is_flag_set(siots_p, FBE_RAID_SIOTS_FLAG_WAIT_LOCK) ||
        fbe_raid_siots_is_flag_set(siots_p, FBE_RAID_SIOTS_FLAG_WAIT_UPGRADE))
    {
        /* This siots is quiesced. 
         */
        b_quiesced = FBE_TRUE;
    }
    return b_quiesced;
}
/**************************************
 * end fbe_raid_siots_is_quiesced()
 **************************************/

/*!***************************************************************
 *  fbe_raid_iots_quiesce
 ****************************************************************
 * @brief
 *  This function causes an iots to start to quiesce.
 *
 * @param iots_p - Pointer to the active iots.
 * 
 * @return
 *  fbe_status_t - FBE_STATUS_OK - Success.
 *                 other status - error
 *
 * @author
 *  10/23/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_iots_quiesce(fbe_raid_iots_t * const iots_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_raid_siots_t *siots_p = NULL;

    if (!fbe_raid_common_is_flag_set((fbe_raid_common_t*)iots_p,
                                     FBE_RAID_COMMON_FLAG_TYPE_IOTS))
    {
        /* Something is wrong, we expected a raid iots.
         */
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Loop through each siots_p and check for quiesced.
     */
    if (iots_p->status != FBE_RAID_IOTS_STATUS_AT_LIBRARY)
    {
        /* This iots is getting completed or not at the library, and is not quiesced.
         */
        return FBE_STATUS_OK;
    }

    if (fbe_raid_iots_is_flag_set(iots_p, FBE_RAID_IOTS_FLAG_FAST_WRITE) &&
        !fbe_raid_iots_is_flag_set(iots_p, FBE_RAID_IOTS_FLAG_LAST_IOTS_OF_REQUEST))
    {
        fbe_raid_library_trace_basic(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                     "IOTS flag for last iots not set on fast write iots: 0x%x fl: 0x%x\n",
                                     (fbe_u32_t)iots_p, iots_p->flags);
    }

    /* Mark the iots quiesced so that any outstanding iots will stop processing.
     */
    fbe_raid_iots_set_flag(iots_p, FBE_RAID_IOTS_FLAG_QUIESCE);

    /* Stop optimizing for non-degraded since we might get errors and need to re-issue.
     */ 
    fbe_raid_iots_clear_flag(iots_p, FBE_RAID_IOTS_FLAG_NON_DEGRADED);
    /* Abort any outstanding memory request
     */
    if (fbe_raid_common_is_flag_set(&iots_p->common, FBE_RAID_COMMON_FLAG_WAITING_FOR_MEMORY))
    {
        status = fbe_raid_memory_api_abort_request(fbe_raid_iots_get_memory_request(iots_p));
        if (RAID_MEMORY_COND(status != FBE_STATUS_OK))
        {
            /* Trace a message and return false 
             */
            fbe_raid_iots_trace(iots_p, FBE_RAID_IOTS_TRACE_PARAMS_ERROR,
                                "raid: iots_p: 0x%p failed to abort memory request with status: 0x%x \n",
                                iots_p, status);
            return status;
        }
    }

    /* If we are generating a siots or allocating a siots, we cannot assume  
     * this iots is quiesced since it is actively trying to kick off more siots. 
     */
    if ((fbe_raid_iots_is_flag_set(iots_p, FBE_RAID_IOTS_FLAG_GENERATING_SIOTS)) ||
        (fbe_raid_iots_is_flag_set(iots_p, FBE_RAID_IOTS_FLAG_ALLOCATING_SIOTS)))
    {
        return FBE_STATUS_OK;
    }

    /* Now quiesce any siots
     */
    siots_p = (fbe_raid_siots_t *)fbe_queue_front(&iots_p->siots_queue);
    while (siots_p != NULL)
    {
        fbe_raid_siots_t *nsiots_p = (fbe_raid_siots_t *) fbe_queue_front(&siots_p->nsiots_q);

        if (fbe_raid_common_is_flag_set(&siots_p->common, FBE_RAID_COMMON_FLAG_WAITING_FOR_MEMORY))
        {
            /* Abort any outstanding memory request
             */
            status = fbe_raid_memory_api_abort_request(fbe_raid_siots_get_memory_request(siots_p));
            if (RAID_MEMORY_COND(status != FBE_STATUS_OK))
            {
                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                     "raid: siots_p: 0x%p failed to abort memory request with status: 0x%x \n",
                                     siots_p, status);
                return status;
            }
        }
                
        /* Check all the nested sub-iots and check if they are quiesced.
         */
        while (nsiots_p != NULL)
        {
            if (fbe_raid_common_is_flag_set(&nsiots_p->common, FBE_RAID_COMMON_FLAG_WAITING_FOR_MEMORY))
            {
                /* Abort any outstanding memory request
                */
                status = fbe_raid_memory_api_abort_request(fbe_raid_siots_get_memory_request(nsiots_p));
                if (RAID_MEMORY_COND(status != FBE_STATUS_OK))
                {
                    fbe_raid_siots_trace(nsiots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                     "raid: siots_p: 0x%p failed to abort memory request with status: 0x%x \n",
                                         nsiots_p, status);
                    return status;
                }
            }
            nsiots_p = fbe_raid_siots_get_next(nsiots_p);
        }

        siots_p = fbe_raid_siots_get_next(siots_p);
    }
    return status;
}
/*****************************************
 * fbe_raid_iots_quiesce()
 *****************************************/

/*!***************************************************************
 *  fbe_raid_iots_is_quiesced
 ****************************************************************
 * @brief
 *  This function checks if an iots is quiesced.
 *  This function has no side effects on the iots or siots.
 *
 * @param iots_p - Pointer to the active iots.
 * 
 * @return
 *  fbe_bool_t FBE_TRUE - We are quiesced.
 *            FBE_FALSE - We are not quiesced yet.
 *
 * @author
 *  10/23/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_bool_t fbe_raid_iots_is_quiesced(fbe_raid_iots_t * const iots_p)
{
    fbe_raid_siots_t *siots_p;
    fbe_u32_t found_siots_count = 0;

    if (!fbe_raid_common_is_flag_set((fbe_raid_common_t*)iots_p,
                                     FBE_RAID_COMMON_FLAG_TYPE_IOTS))
    {
        /* Something is wrong, we expected a raid iots.
         */
        return FBE_FALSE;
    }

    /* Loop through each siots_p and check for quiesced.
     */
    if (iots_p->status != FBE_RAID_IOTS_STATUS_AT_LIBRARY)
    {
        /* This iots is getting completed or not at the library, and is not quiesced.
         */
        return FBE_FALSE;
    }

    /* If we have outstanding memory requests we are not quiesced.
     */
    if (fbe_raid_common_is_flag_set(&iots_p->common, FBE_RAID_COMMON_FLAG_WAITING_FOR_MEMORY))
    {
        /* Return FALSE to give the aborted request a chance to complete.
         */
		return FBE_FALSE;
    }

    /* If we are generating a siots or allocating a siots, we cannot assume  
     * this iots is quiesced since it is actively trying to kick off more siots. 
     */
    if (fbe_raid_iots_is_flag_set(iots_p, FBE_RAID_IOTS_FLAG_GENERATING_SIOTS) ||
        fbe_raid_iots_is_flag_set(iots_p, FBE_RAID_IOTS_FLAG_ALLOCATING_SIOTS))
    {
        return FBE_FALSE;
    }

    /* Now quiesce any siots
     */
    siots_p = (fbe_raid_siots_t *)fbe_queue_front(&iots_p->siots_queue);
    while (siots_p != NULL)
    {
        fbe_raid_siots_t *nsiots_p = (fbe_raid_siots_t *) fbe_queue_front(&siots_p->nsiots_q);
        fbe_bool_t b_quiesced = FBE_FALSE;

        if (fbe_raid_siots_is_quiesced(siots_p))
        {
            if(RAID_COND(siots_p->wait_count != 0) )
            {
                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                            "raid: wait count 0x%llx is non Zero\n",
                                            siots_p->wait_count);
                return FBE_FALSE;
            }
            b_quiesced = FBE_TRUE;
        }
                
        /* Check all the nested sub-iots and check if they are quiesced.
         */
        while (nsiots_p != NULL)
        {
            if (fbe_raid_siots_is_quiesced(nsiots_p))
            {
                if(RAID_COND(nsiots_p->wait_count != 0) )
                {
                    fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                                "raid: wait count 0x%llx is non Zero\n",
                                                siots_p->wait_count);
                    return FBE_FALSE;
                }
                b_quiesced = FBE_TRUE;
            }
            nsiots_p = fbe_raid_siots_get_next(nsiots_p);
        }

        /* If this is not quiesced, then return status now.
         * There is no need to check the rest of the siots since we have to wait for 
         * this siots to quiesce anyway. 
         */
        if (b_quiesced == FBE_FALSE)
        {
            return FBE_FALSE;
        }
        siots_p = fbe_raid_siots_get_next(siots_p);
        found_siots_count++;
    }
    if (found_siots_count != iots_p->outstanding_requests)
    {
        /* The siots dequeued itself in preparation for freeing itself. 
         * Don't return quiesced yet. 
         */
        return FBE_FALSE;
    }
    else if (found_siots_count == 0) {
        /* If this IOTS has not actually been asked to stop, then quiesced will not be set yet.
         * We need to prevent this function from saying this IOTS is quiesced if we just found 
         * that it does not have any siots yet. 
         */
        if (fbe_raid_iots_is_flag_set(iots_p, FBE_RAID_IOTS_FLAG_QUIESCE)) {
            fbe_raid_iots_object_trace(iots_p, FBE_RAID_LIBRARY_TRACE_PARAMS_DEBUG_HIGH,
                                       "IOTS is quiesced fl: 0x%x %p\n", iots_p->flags, iots_p);
            return FBE_TRUE;
        } else {
            fbe_raid_iots_object_trace(iots_p, FBE_RAID_LIBRARY_TRACE_PARAMS_DEBUG_HIGH,
                                       "IOTS not quiesced yet fl: 0x%x %p\n", iots_p->flags, iots_p);
            return FBE_FALSE;
        }
    }
    /* In this case we found only quiesced siots.  This is enough to state the iots is quiesced.
     */
    return FBE_TRUE;
}
/*****************************************
 * fbe_raid_iots_is_quiesced()
 *****************************************/

/*!***************************************************************
 *  fbe_raid_iots_is_quiesced_with_lock
 ****************************************************************
 * @brief
 *  This function checks if an iots is quiesced.
 *
 * @param iots_p - Pointer to the active iots.
 * @param b_quiesced_p - TRUE if quiesced, FALSE otherwise.
 * 
 * @return
 *  fbe_status_t
 *
 * @author
 *  10/23/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_iots_quiesce_with_lock(fbe_raid_iots_t * const iots_p,
                                             fbe_bool_t *b_quiesced_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_raid_geometry_t *geometry_p = fbe_raid_iots_get_raid_geometry(iots_p);
    fbe_object_id_t object_id = fbe_raid_geometry_get_object_id(geometry_p);

    /* Determine if we are quiesced with lock held.
     */
    fbe_raid_iots_lock(iots_p);
    status = fbe_raid_iots_quiesce(iots_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_raid_library_object_trace(object_id, FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                      "quiesce w/lock raid_iots_quiesce status: 0x%x\n",
                                      status);
    }
    *b_quiesced_p = fbe_raid_iots_is_quiesced(iots_p);
    fbe_raid_iots_unlock(iots_p);

    /*! @note this is a debug hook call to allow us to force an abort for a
     *        a memory allocation    
     */
    fbe_raid_memory_mock_change_defer_delay(*b_quiesced_p);

    return status;
}
/*****************************************
 * fbe_raid_iots_is_quiesced_with_lock()
 *****************************************/
/****************************************************************
 *  fbe_raid_siots_mark_quiesced()
 ****************************************************************
 * @brief
 *  This function checks if the raid group is quiesced,
 *  and if it is, then with lock held we mark the siots as quiesced.
 *
 *  This function should be used by the state machines to determine
 *  if we are quiesced.
 *  
 *  The state machine function that calls this needs to return
 *  FBE_RAID_STATE_STATUS_WAITING if we return FBE_TRUE.
 *
 *  The state machine function that calls this needs to be reentrant,
 *  and needs to check immediately if the state of the raid group
 *  has changed and become degraded.
 *
 * @param siots_p - Pointer to the active siots.
 * 
 * @return
 *  fbe_bool_t FBE_TRUE if we are quiesced.
 *            FBE_FALSE if we are not quiesced (continue normal operation).
 *
 * @author
 *  10/26/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_bool_t fbe_raid_siots_mark_quiesced(fbe_raid_siots_t * const siots_p)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_bool_t      b_quiesced = FBE_FALSE;
    fbe_raid_iots_t *iots_p = fbe_raid_siots_get_iots(siots_p);

    /* Monitor initiated I/Os will just continue rather than quiesce.
     */
    if ((fbe_raid_siots_is_monitor_initiated_op(siots_p)                                          &&
        !fbe_raid_common_is_flag_set(&siots_p->common, FBE_RAID_COMMON_FLAG_WAITING_FOR_MEMORY)) ||
         !fbe_raid_siots_is_able_to_quiesce(siots_p))
    {
        /* Return FALSE so the caller just continues.
         */
        return FBE_FALSE;
    }
    fbe_raid_iots_lock(iots_p);
    if (fbe_raid_iots_is_flag_set(iots_p, FBE_RAID_IOTS_FLAG_QUIESCE))
    {
        /* Validate that there are no ios oustanding for this siots
         */
        if(RAID_COND(siots_p->wait_count != 0) )
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                        "raid: wait count 0x%llx is non Zero\n",
                                        siots_p->wait_count);
            fbe_raid_iots_unlock(iots_p);
            return FBE_FALSE;
        }

        /* Abort any outstanding memory request
         */
        if (fbe_raid_common_is_flag_set(&siots_p->common, FBE_RAID_COMMON_FLAG_WAITING_FOR_MEMORY))
        {
            status = fbe_raid_memory_api_abort_request(fbe_raid_siots_get_memory_request(siots_p));
            if (RAID_MEMORY_COND(status != FBE_STATUS_OK))
            {
                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                     "raid: siots_p: 0x%p failed to abort memory request with status: 0x%x \n",
                                     siots_p, status);
            }

            /* Always return false since we are waiting for the memory request 
             * to complete (be aborted).
             */
            fbe_raid_iots_unlock(iots_p);
            return FBE_FALSE;
        }

        /* Mark the siots as quiesced with lock held. 
         */ 
        b_quiesced = FBE_TRUE;
        fbe_raid_siots_set_flag(siots_p, FBE_RAID_SIOTS_FLAG_QUIESCED);
    }
    fbe_raid_iots_unlock(iots_p);
    return b_quiesced;
}
/**************************************
 * end fbe_raid_siots_mark_quiesced()
 **************************************/
/****************************************************************
 *  fbe_raid_siots_should_restart_lock_waiters()
 ****************************************************************
 * @brief
 *  Check if we are able to restart waiters or not.
 *
 * @param siots_p - Pointer to the active siots.
 * @param b_restart - Pointer to boolean to return
 *                    FBE_TRUE if we are able to restart.
 *            FBE_FALSE do not restart waiters.
 * @return
 *  fbe_status_t - FBE_STATUS_OK - Successful.
 *                 FBE_STATUS_GENERIC_FAILURE - Unexpected error occurred.
 *
 * @author
 *  11/30/2010 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_raid_siots_should_restart_lock_waiters(fbe_raid_siots_t * siots_p,
                                                               fbe_bool_t *b_restart_p)
{
    fbe_raid_iots_t *iots_p = fbe_raid_siots_get_iots(siots_p);

    /* Metadata I/Os will just continue rather than quiesce.
     */
    if (fbe_raid_siots_is_metadata_operation(siots_p))
    {
        /* Return TRUE so the caller restarts.
         */
        *b_restart_p = FBE_TRUE;
        return FBE_STATUS_OK;
    }

    if (fbe_raid_iots_is_flag_set(iots_p, FBE_RAID_IOTS_FLAG_QUIESCE))
    {
        /* Validate that there are no ios oustanding for this siots
         */
        if(RAID_COND(siots_p->wait_count != 0) )
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                        "raid: wait count 0x%llx is non Zero\n",
                                        siots_p->wait_count);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        /* We are quiesced, do not restart.
         */ 
        *b_restart_p = FBE_FALSE;
    }
    else
    {
        /* We are not quiesced so we can restart.
         */
        *b_restart_p = FBE_TRUE;
    }
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_raid_siots_should_restart_lock_waiters()
 **************************************/
/*!**************************************************************
 * fbe_raid_siots_generate_get_lock()
 ****************************************************************
 * @brief
 *  Fetch the siots lock so that this siots can proceed
 *  independently of the other siots.
 *  The siots lock is needed since we cannot run if we are
 *  conflicting in parity range.
 *
 * @param siots_p - Current request.     
 *
 * @return FBE_RAID_STATE_STATUS_EXECUTING If we can start immediately.
 *         FBE_RAID_STATE_STATUS_WAITING if we are waiting for the siots lock.
 *
 * @author
 *  8/15/2011 - Created. Rob Foley
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_raid_siots_generate_get_lock(fbe_raid_siots_t * siots_p)
{
    fbe_status_t status;
    fbe_raid_state_status_t state_status;
    fbe_raid_iots_t *iots_p = fbe_raid_siots_get_iots(siots_p);
    fbe_bool_t b_nested_siots = fbe_raid_siots_is_nested(siots_p);
    fbe_raid_siots_t *siots_to_check_lock_p = NULL;

    fbe_raid_iots_lock(iots_p);
    siots_to_check_lock_p = (b_nested_siots) ? fbe_raid_siots_get_parent(siots_p) : siots_p;

    if (fbe_raid_siots_is_startable(siots_to_check_lock_p) == FBE_FALSE)
    {
        fbe_bool_t b_restart;
        /* If we are here then it means that current siots reqirement is 
         * conflicting with others and we will waiting for siots lock. And,
         * so following conditions are possible only if there had been 
         * race in processing siots across varous cores.
         */
        if (RAID_ERROR_COND(fbe_queue_length(&iots_p->siots_queue) <= 1))
        {
            fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "%s fbe_queue_length(&iots_p->siots_queue) 0x%x <= 1\n", 
                                                __FUNCTION__, fbe_queue_length(&iots_p->siots_queue));
            fbe_raid_iots_unlock(iots_p);
            return FBE_RAID_STATE_STATUS_EXECUTING;
        }
        if (RAID_ERROR_COND(iots_p->outstanding_requests <= 1))
        {
            fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "%s iots_p->outstanding_requests 0x%x <= 1\n", 
                                                __FUNCTION__, iots_p->outstanding_requests);
            fbe_raid_iots_unlock(iots_p);
            return FBE_RAID_STATE_STATUS_EXECUTING;
        }
        if (RAID_ERROR_COND(siots_p->wait_count != 0))
        {
            fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "%s siots_p->wait_count 0x%llx != 0\n", 
                                                __FUNCTION__, siots_p->wait_count);
            fbe_raid_iots_unlock(iots_p);
            return FBE_RAID_STATE_STATUS_EXECUTING;
        }
        /* This siots conflicts with another currently executing siots.
         * This SIOTS must wait.  We lock the iots since 
         * other siots completing need to kick this off. 
         */
        fbe_raid_siots_set_flag(siots_p, FBE_RAID_SIOTS_FLAG_WAIT_LOCK);
        state_status = FBE_RAID_STATE_STATUS_WAITING;

        status = fbe_raid_siots_should_restart_lock_waiters(siots_p, &b_restart);
        if (RAID_COND_STATUS(status != FBE_STATUS_OK, status))
        {
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                        "generate get lock rstrt lock err: 0x%x siots: 0x%x\n",
                                        status, fbe_raid_memory_pointer_to_u32(siots_p));
            /* Since we failed, clear the wait flag so we do not get restarted in another context.
             */
            fbe_raid_siots_clear_flag(siots_p, FBE_RAID_SIOTS_FLAG_WAIT_LOCK);
            fbe_raid_iots_unlock(iots_p);
            return FBE_RAID_STATE_STATUS_EXECUTING;
        }
        /* Only restart lock waiters if we are not quiesced.
         */
        if (!b_restart)
        {
            fbe_raid_siots_log_traffic(siots_p, "wait lock wait quiesced");
            fbe_raid_iots_unlock(iots_p);
        }
        else
        {
            fbe_raid_siots_t *restart_siots_p = NULL;
            fbe_raid_siots_log_traffic(siots_p, "wait lock restart waiters");
            /* There might have been another siots that was waiting for us 
             * to transition to a wait state. See if we can re-activate anyone now. 
             * The below function is called with iots lock held and releases the lock. 
             */
            fbe_raid_siots_get_first_lock_waiter(siots_p, &restart_siots_p);
            fbe_raid_iots_unlock(iots_p);

            if (restart_siots_p != NULL)
            {
                /* Restart any waiting siots.
                 */
                fbe_raid_common_state(((fbe_raid_common_t *) restart_siots_p));
            }
        }
    }
    else
    {
        /* SIOTS is startable, we will start executing
         * the siot state machine immediately.
         */
        fbe_raid_iots_unlock(iots_p);
        fbe_raid_siots_log_traffic(siots_p, "gen start");
        state_status = FBE_RAID_STATE_STATUS_EXECUTING;
    }
    return state_status;
}
/******************************************
 * end fbe_raid_siots_generate_get_lock()
 ******************************************/
/*!**************************************************************
 * fbe_raid_iots_get_quiesced_ts_to_restart()
 ****************************************************************
 * @brief
 *  Get the ts that need to be restarted and put them on this queue.
 *
 * @param iots_p - The iots to check for requests to restart.               
 * @param restart_queue_p - Queue to place ts to restart.
 * 
 * @return fbe_status_t
 *
 * @author
 *  3/8/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_iots_get_quiesced_ts_to_restart(fbe_raid_iots_t * const iots_p,
                                                      fbe_queue_head_t *restart_queue_p)
{
    fbe_u32_t num_restarted = 0;
    fbe_bool_t b_restarted_iots = FBE_FALSE;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_iots_get_raid_geometry(iots_p);

    /* Add the siots for this iots that need to be restarted. 
     * We will kick them off below once we have finished iterating over 
     * the termination queue. 
     */

    if (!fbe_raid_iots_is_flag_set(iots_p, FBE_RAID_IOTS_FLAG_QUIESCE))
    {
        /* Already unquiesced.  Probably updating metadata.  When the metadata update is 
         * complete it will unquiesce this request. 
         */
        return FBE_STATUS_OK;
    }
    fbe_raid_iots_clear_flag(iots_p, FBE_RAID_IOTS_FLAG_QUIESCE);
    if (iots_p->status == FBE_RAID_IOTS_STATUS_COMPLETE)
    {
        /* This iots is getting completed.  It is not quiesced.
         */
        return FBE_STATUS_OK;
    }
    fbe_raid_iots_get_siots_to_restart(iots_p, restart_queue_p, &num_restarted);

    /* We are restarting to the library to set the status to at library.
     * We intentionally set this after we have cleared the quiesced flag in iots and siots. 
     */
    fbe_raid_iots_set_status(iots_p, FBE_RAID_IOTS_STATUS_AT_LIBRARY);

    if (fbe_raid_iots_is_flag_set(iots_p, FBE_RAID_IOTS_FLAG_QUIESCE))
    {
        fbe_raid_iots_object_trace(iots_p, FBE_RAID_IOTS_TRACE_PARAMS_ERROR,
                                   "iots %p QUIESCE is still set flags 0x%x\n", iots_p, iots_p->flags);
    }
    if (num_restarted == 0)
    {
        if (iots_p->outstanding_requests == 0)
        {
            /* If nothing was restarted, then we should wake up this iots.
             * This is the case where the iots has more to generate. 
             */ 
            if (RAID_ERROR_COND((iots_p->callback == NULL) ||
                                (iots_p->callback_context == NULL) ||
                                (iots_p->common.state == (fbe_raid_common_state_t)fbe_raid_iots_freed)) )
            {
                fbe_raid_iots_object_trace(iots_p, FBE_RAID_IOTS_TRACE_PARAMS_ERROR, 
                                           "raid %s: iots_p->callback or iots_p->callback_context is null "
                                           "iots_p->common.state is set to freed state\n",
                                           __FUNCTION__);
                return FBE_STATUS_GENERIC_FAILURE;
            }
            fbe_raid_library_object_trace(fbe_raid_geometry_get_object_id(raid_geometry_p),
                                          FBE_RAID_LIBRARY_TRACE_PARAMS_INFO,
                                          "iots gt_qsced_ts 0 outstanding continue iots: 0x%x\n",
                                          fbe_raid_memory_pointer_to_u32(iots_p));
            fbe_raid_iots_set_flag(iots_p, FBE_RAID_IOTS_FLAG_GENERATING_SIOTS);
            fbe_raid_common_wait_enqueue_tail(restart_queue_p, &iots_p->common);
            b_restarted_iots = FBE_TRUE;
        }
        else
        {
            fbe_raid_siots_t *siots_p = (fbe_raid_siots_t*)fbe_queue_front(&iots_p->siots_queue);

            /* Check to see if the head siots is waiting for locks, if it is then
             * restart it. In this case we allow the one we restart to restart the 
             * rest as it finishes. 
             */
            if ((siots_p != NULL) &&
                (fbe_raid_siots_is_flag_set(siots_p, FBE_RAID_SIOTS_FLAG_WAIT_LOCK) ||
                 fbe_raid_siots_is_flag_set(siots_p, FBE_RAID_SIOTS_FLAG_WAIT_UPGRADE)))
            {
                fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_INFO,
                                            "iots gt_qsced_ts continue waiting siots: 0x%x fl: 0x%x\n", 
                                            fbe_raid_memory_pointer_to_u32(siots_p),  siots_p->flags);
                fbe_raid_siots_clear_flag(siots_p, FBE_RAID_SIOTS_FLAG_QUIESCED);
                fbe_raid_common_wait_enqueue_tail(restart_queue_p, &siots_p->common);
            }
        }
    }
   if (!b_restarted_iots &&
       fbe_raid_common_is_flag_set((fbe_raid_common_t*)iots_p, FBE_RAID_COMMON_FLAG_MEMORY_REQUEST_ABORTED))
   {
        /* Add the iots to the queue since it needs to restart the allocation of memory.
         */
        fbe_raid_library_object_trace(fbe_raid_geometry_get_object_id(raid_geometry_p),
                                      FBE_RAID_LIBRARY_TRACE_PARAMS_INFO,
                                      "iots gt_qsced_ts mem req aborted iots: 0x%x\n",
                                      fbe_raid_memory_pointer_to_u32(iots_p));
        fbe_raid_common_wait_enqueue_tail(restart_queue_p, &iots_p->common);
    }
    return FBE_STATUS_OK;
}
/************************************************
 * end fbe_raid_iots_get_quiesced_ts_to_restart()
 ************************************************/
/*!**************************************************************
 * fbe_raid_iots_queue_not_finished()
 ****************************************************************
 * @brief
 * During shutdown we need to restart any stalled siots that need
 * to release allocated journal slots.  
 *  
 * @param iots_p          - the iots to quiesce
 * @param restart_queue_p - where to add siots that need a restart
 * @param num_restarted_p - how many quiesced siots have been added
 *                          to the restart queue.
 * 
 * @return fbe_status_t
 * 
 * @author
 * 2/25/2011 - Daniel Cummins. Created.
 *
 ****************************************************************/

static fbe_bool_t fbe_raid_iots_queue_not_finished(fbe_raid_iots_t *iots_p, 
                                                   fbe_queue_head_t *restart_queue_p,
                                                   fbe_u32_t *num_restarted_p)
{
    fbe_raid_siots_t *siots_p;
    fbe_bool_t b_not_finished = FBE_FALSE;
    fbe_u32_t num_restarted = 0;

    if (!fbe_raid_common_is_flag_set((fbe_raid_common_t*)iots_p, FBE_RAID_COMMON_FLAG_TYPE_IOTS))
    {
        /* Something is wrong, we expected a raid iots.
         */
        return FBE_FALSE;
    }

    /* Loop through each siots_p and check if not complete at library.
     */
    if (iots_p->status != FBE_RAID_IOTS_STATUS_AT_LIBRARY)
    {
        /* This iots is getting completed or not at the library, and is not quiesced.
         */
        return FBE_FALSE;
    }

    /* Mark the iots quiesced so that any outstanding iots will stop processing.
     */
    fbe_raid_iots_set_flag(iots_p, FBE_RAID_IOTS_FLAG_QUIESCE);

    /* If we are generating a siots, we cannot assume this iots is quiesced since 
     * it is actively trying to kick off more siots. 
     */
    if (fbe_raid_iots_is_flag_set(iots_p, FBE_RAID_IOTS_FLAG_GENERATING_SIOTS))
    {
        return FBE_FALSE;
    }

    /* Now queue any siots that are marked quiesced but not finished.
     */
    siots_p = (fbe_raid_siots_t *)fbe_queue_front(&iots_p->siots_queue);
    while (siots_p != NULL)
    {
        fbe_raid_siots_t *nsiots_p = (fbe_raid_siots_t *) fbe_queue_front(&siots_p->nsiots_q);

        /* Check the nested sub-iots first.
         */
        while (nsiots_p != NULL)
        {
            /* we are quiescing while allocated slots are outstanding.. kick this siots to cleanup
             * the allocated slot.
             */
            if (fbe_raid_siots_is_flag_set(nsiots_p,FBE_RAID_SIOTS_FLAG_JOURNAL_SLOT_ALLOCATED))
            {
                if (fbe_raid_siots_is_quiesced(nsiots_p))
                {
                    fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_INFO,
                                                "iots_queue_not_finish restart nsiots 0x%p to free slot, nsiots_p->flag 0x%x\n",
                                                nsiots_p, nsiots_p->flags);

                    fbe_raid_common_wait_enqueue_tail(restart_queue_p, &nsiots_p->common);
                    num_restarted++;
                }
                else
                {
                    fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_INFO,
                                                "iots_queue_not_finish cannot restart nsiots 0x%p not quiesed,nsiots->flag 0x%x\n",
                                                nsiots_p, nsiots_p->flags);
                }

                b_not_finished = FBE_TRUE;
            }
            nsiots_p = fbe_raid_siots_get_next(nsiots_p);
        }

        /* Now check the siots
         */
        if (fbe_raid_siots_is_flag_set(siots_p,FBE_RAID_SIOTS_FLAG_JOURNAL_SLOT_ALLOCATED))
        {
            if (fbe_raid_siots_is_quiesced(siots_p))
            {
                fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_INFO,
                                            "iots_queue_not_finish restart siots 0x%p to free slot,siots->flag 0x%x\n",
                                            siots_p, siots_p->flags);
                /* we are quiescing while allocated slots are outstanding.. kick this siots to cleanup
                 * the allocated slot.
                 */
                fbe_raid_common_wait_enqueue_tail(restart_queue_p, &siots_p->common);
                num_restarted++;
            }
            else
            {
                fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_INFO,
                                            "iots_queue_not_finish restart siots 0x%p to free slot,siots->flag 0x%x\n",
                                            siots_p, siots_p->flags);
            }

            b_not_finished = FBE_TRUE;
        }
        siots_p = fbe_raid_siots_get_next(siots_p);
    }

    if (b_not_finished)
    {
        *num_restarted_p = num_restarted;
    }
    return b_not_finished;
}

/*!**************************************************************
 * fbe_raid_iots_handle_shutdown()
 ****************************************************************
 * @brief
 *  For this iots try to handle shutdown.
 *  If the siots are not doing anything, then we can
 *  simply mark this iots complete and put it on the restart
 *  queue in prepartion for completion.
 *
 * @param iots_p - The iots to check shutdown for.
 * @param restart_queue_p - The queue to put ourselves on if we
 *                          need to get restarted.
 *
 * @return fbe_status_t.
 *
 * @author
 *  3/8/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_iots_handle_shutdown(fbe_raid_iots_t *iots_p,
                                           fbe_queue_head_t *restart_queue_p)
{
    fbe_status_t status;
    fbe_raid_geometry_t *geo_p = fbe_raid_iots_get_raid_geometry(iots_p);
    fbe_object_id_t object_id = fbe_raid_geometry_get_object_id(geo_p);

    fbe_raid_iots_lock(iots_p);

    /* Mark the request with shutdown error so that
     * the caller knows that request failed,
     * because of a shutdown condition.
     * Also mark the request aborted.
     */
    fbe_raid_iots_set_block_status(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED);
    fbe_raid_iots_set_block_qualifier(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE);
    fbe_raid_iots_set_flag(iots_p, FBE_RAID_IOTS_FLAG_ABORT_FOR_SHUTDOWN);

    /* Try to stop the iots.  This will abort any outstanding memory requests.
     */
    status = fbe_raid_iots_quiesce(iots_p);
    if (status != FBE_STATUS_OK) 
    {
        fbe_raid_library_object_trace(object_id, FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                      "raid_iots_handle_shutdown iots 0x%p quiesce error: 0x%x\n",
                                      iots_p, status); 
    }
    if (fbe_raid_iots_is_quiesced(iots_p))
    {
        /* We should not have completed this already.
         */
        if (RAID_COND(iots_p->status == FBE_RAID_IOTS_STATUS_COMPLETE))
        {
            fbe_raid_library_object_trace(object_id, FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                        "raid_iots_handle_shutdown: stat 0x%x Unexpected\n",
                                        iots_p->status);
            fbe_raid_iots_unlock(iots_p);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        if (iots_p->status == FBE_RAID_IOTS_STATUS_COMPLETE)
        {
            /* We should not be marked completed yet.
             */
            fbe_raid_library_object_trace(object_id, FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                          "raid_iots_handle_shutdown iots 0x%p already mark for object\n",
                                          iots_p);

        }

        fbe_raid_library_object_trace(object_id, FBE_RAID_LIBRARY_TRACE_PARAMS_INFO,
                                      "raid_iots_handle_shutdown iots:0x%p mark complete for object\n",
                                      iots_p);
        fbe_raid_iots_mark_complete(iots_p);
        fbe_raid_iots_inc_aborted_shutdown(iots_p);
        fbe_raid_common_wait_enqueue_tail(restart_queue_p, &iots_p->common);
        status = FBE_STATUS_OK;
    }
    else
    {
        /* This iots is not done, we need to wait for it.
         */
   
        fbe_raid_library_object_trace(object_id, FBE_RAID_LIBRARY_TRACE_PARAMS_INFO,
                                      "raid_iots_handle_shutdown iots:0x%p abort for shutdown\n",
                                      iots_p);
        
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_raid_iots_unlock(iots_p);

    /* Always return the execution status
     */
    return status;
}
/******************************************
 * end fbe_raid_iots_handle_shutdown()
 ******************************************/
/*!**************************************************************
 * fbe_raid_iots_restart_iots_after_metadata_update()
 ****************************************************************
 * @brief
 *  Restart an iots that has just finished updating metadata.
 *
 * @param iots_p - Iots to  kick off.
 *                 We assume the iots chunk info and rl mask have been updated.               
 *
 * @return fbe_status_t   
 *
 * @author
 *  3/23/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_iots_restart_iots_after_metadata_update(fbe_raid_iots_t *iots_p)
{
    fbe_queue_head_t restart_queue;
    fbe_u32_t restart_siots_count;
    fbe_raid_iots_continue(iots_p, iots_p->chunk_info[0].needs_rebuild_bits);
    fbe_queue_init(&restart_queue);
    
    /* Mark as quiesced so that when we call the below function we will process this 
     * iots. 
     */
    fbe_raid_iots_lock(iots_p);
    fbe_raid_iots_set_flag(iots_p, FBE_RAID_IOTS_FLAG_QUIESCE);

    fbe_raid_iots_get_quiesced_ts_to_restart(iots_p, &restart_queue);
    fbe_raid_iots_unlock(iots_p);

    restart_siots_count = fbe_raid_restart_common_queue(&restart_queue);
    fbe_queue_destroy(&restart_queue);

    /* It is possible we restarted 0 siots in the case where multiple continues are 
     * needed. 
     */
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_raid_iots_restart_iots_after_metadata_update()
 ******************************************/

/************************************************************
 *  fbe_raid_iots_increment_stripe_crossings()
 ************************************************************
 *
 * @brief  Increment stripe crossings statistics
 *
 * @param   iots_p - IOTS structure
 *
 * @return  
 *          fbe_status_t
 * @author
 *  12/17/2010  Swati Fursule  - Created
 *
 ************************************************************/
fbe_status_t fbe_raid_iots_increment_stripe_crossings(fbe_raid_iots_t* iots_p)
{
    fbe_status_t                      status = FBE_STATUS_OK;
    fbe_lun_performance_counters_t  *performace_stats_p = NULL;
    fbe_raid_geometry_t               *raid_geometry_p = fbe_raid_iots_get_raid_geometry(iots_p);
    fbe_lba_t                          lba;
    fbe_block_count_t                  blocks;
    fbe_element_size_t           element_size;
    /*get element size, lba and block */
    fbe_raid_geometry_get_element_size(raid_geometry_p, &element_size);
    fbe_raid_iots_get_blocks(iots_p, &blocks);
    fbe_raid_iots_get_lba(iots_p, &lba);
    fbe_raid_iots_get_performance_stats(iots_p,
                                        &performace_stats_p);

    if (performace_stats_p != NULL)
    {
        /*if (fbe_raid_geometry_is_parity_type(raid_geometry_p) ||
            fbe_raid_geometry_is_raid0(raid_geometry_p))*/
        if ((lba % element_size) + blocks > element_size)
        {
            fbe_cpu_id_t cpu_id;
            fbe_raid_iots_get_cpu_id(iots_p, &cpu_id);
            fbe_raid_perf_stats_inc_stripe_crossings(performace_stats_p, iots_p->packet_p->cpu_id);
        }
    }
    return status;
}
/*****************************************
 * fbe_raid_iots_increment_stripe_crossings()
 *****************************************/

/************************************************************
 *  fbe_raid_fruts_increment_disk_statistics()
 ************************************************************
 *
 * @brief  Increment Disks statistics
 *
 * @param   fruts_p - Fru Ts structure
 *
 * @return  
 *          fbe_status_t
 * @author
 *  12/17/2010  Swati Fursule  - Created
 *
 ************************************************************/
fbe_status_t fbe_raid_fruts_increment_disk_statistics(fbe_raid_fruts_t *fruts_p)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_lun_performance_counters_t  *performace_stats_p = NULL;
    fbe_payload_block_operation_opcode_t opcode;
    fbe_raid_siots_t   *siots_p = fbe_raid_fruts_get_siots(fruts_p);
    fbe_raid_iots_t     *iots_p = fbe_raid_siots_get_iots(siots_p);

    fbe_raid_iots_get_opcode(iots_p, &opcode);
    fbe_raid_iots_get_performance_stats(iots_p, &performace_stats_p);
    if (performace_stats_p != NULL)
    {
        /* We only log disk stats for operations that
         * originated as host reads, sniffs or writes.
         * e.g. not rebuilds, verifies, expansions,
         * corrupt crcs...
         *
         * Also, only try to log stats if our position looks sane.
         */
        fbe_cpu_id_t cpu_id;
        fbe_raid_iots_get_cpu_id(iots_p, &cpu_id);
        if ( (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ) &&
             (fruts_p->position<FBE_RAID_MAX_DISK_ARRAY_WIDTH))
        {
            
            fbe_raid_perf_stats_inc_disk_reads(performace_stats_p,
                                               fruts_p->position,
                                               cpu_id);
            fbe_raid_perf_stats_inc_disk_blocks_read(performace_stats_p,
                                               fruts_p->position,
                                               fruts_p->blocks,
                                               cpu_id);
        }else if ( ((opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE) ||
                    (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_NONCACHED) ||
                    (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY)) && 
                   (fruts_p->position<FBE_RAID_MAX_DISK_ARRAY_WIDTH))
        {
            fbe_raid_perf_stats_inc_disk_writes(performace_stats_p,
                                               fruts_p->position,
                                               cpu_id);
            fbe_raid_perf_stats_inc_disk_blocks_written(performace_stats_p,
                                               fruts_p->position,
                                               fruts_p->blocks,
                                               cpu_id);
        }
    }
    return status;
}
/*****************************************
 * fbe_raid_fruts_increment_disk_statistics()
 *****************************************/

/***************************************************************************
 * fbe_raid_check_byte_count_limit()
 ***************************************************************************
 * @brief
 *  This function calculated byte count from given blcosk and checks it 
 *  against 32bit byte count limit
 *
 *  @param blocks - 64bit byte count that needs 32bit casting
 *  @dest_byte_count - 32bit byte count to be returned to caller
 *
 * @return fbe_u16_t
 *
 * @author
 *  1/2/2011 - Created. Mahesh Agarkar
 *
 **************************************************************************/
fbe_bool_t fbe_raid_get_checked_byte_count(fbe_u64_t blocks,
                                                                          fbe_u32_t *dest_byte_count)
{
    fbe_u64_t byte_count = (blocks * FBE_BE_BYTES_PER_BLOCK);

    /* check if the byte count is crossing 32bit limit
     */
    if( byte_count> FBE_U32_MAX)
    {
        /* we will simply log the failure but return just zero byte count,
         * the caller needs to handle the return count
         */
        fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                               "%s: Byte count (0x%llx) crossing 32bit limit\n",
                               __FUNCTION__, (unsigned long long)byte_count);
        *dest_byte_count = 0;
        return FBE_FALSE;
    }

    /* it is safe to cast it to 32bit now as the count is in limit
     */
    *dest_byte_count = (fbe_u32_t)byte_count ;

    return FBE_TRUE;
}
/*****************************************
 * fbe_raid_get_checked_byte_count()
 *****************************************/

/*!***************************************************************
 *  fbe_raid_iots_abort_monitor_op
 ****************************************************************
 * @brief
 *  Abort a monitor operation that is waiting for memory.
 *  We search both the iots and all siots/nested for memory waiters.
 *  We then cancel the memory wait for any and all waiters.
 * 
 *  If a monitor operation is waiting for memory, and the
 *  monitor needs to run (such as when a drive goes away),
 *  then a deadlock would result if we did not abort 
 *  our wait for memory.
 *
 * @param iots_p - Pointer to the active iots.
 * 
 * @return
 *  fbe_status_t
 *
 * @author
 *  3/28/2011 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_iots_abort_monitor_op(fbe_raid_iots_t * const iots_p)
{
    fbe_status_t status;
    fbe_raid_siots_t *siots_p;

    if (!fbe_raid_common_is_flag_set((fbe_raid_common_t*)iots_p,
                                    FBE_RAID_COMMON_FLAG_TYPE_IOTS))
    {
        /* Something is wrong, we expected a raid iots.
         */
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Loop through each siots_p and check for quiesced.
     */
    if (iots_p->status != FBE_RAID_IOTS_STATUS_AT_LIBRARY)
    {
        /* This iots is getting completed or not at the library.
         */
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Abort any outstanding memory request
     */
    if (fbe_raid_common_is_flag_set(&iots_p->common, FBE_RAID_COMMON_FLAG_WAITING_FOR_MEMORY))
    {
        fbe_raid_iots_set_flag(iots_p, FBE_RAID_IOTS_FLAG_ABORT);
        status = fbe_raid_memory_api_abort_request(fbe_raid_iots_get_memory_request(iots_p));
        if (RAID_MEMORY_COND(status != FBE_STATUS_OK))
        {
            /* Trace a message and return false 
             */
            fbe_raid_iots_trace(iots_p, FBE_RAID_IOTS_TRACE_PARAMS_ERROR,
                                "raid: iots_p: 0x%p failed to abort memory request with status: 0x%x \n",
                                iots_p, status);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    /* Now abort siots with memory requests outstanding.
     */
    siots_p = (fbe_raid_siots_t *)fbe_queue_front(&iots_p->siots_queue);
    while (siots_p != NULL)
    {
        fbe_raid_siots_t *nsiots_p = (fbe_raid_siots_t *) fbe_queue_front(&siots_p->nsiots_q);

        if (fbe_raid_common_is_flag_set(&siots_p->common, FBE_RAID_COMMON_FLAG_WAITING_FOR_MEMORY))
        {
            /* Abort any outstanding memory request
             */
            fbe_raid_siots_transition(siots_p, fbe_raid_siots_aborted);
            status = fbe_raid_memory_api_abort_request(fbe_raid_siots_get_memory_request(siots_p));
            if (RAID_MEMORY_COND(status != FBE_STATUS_OK))
            {
                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                     "raid: siots_p: 0x%p failed to abort memory request with status: 0x%x \n",
                                     siots_p, status);
                return FBE_STATUS_GENERIC_FAILURE;
            }
        }
                
        /* Check all the nested sub-iots and check if they are quiesced.
         */
        while (nsiots_p != NULL)
        {
            if (fbe_raid_common_is_flag_set(&nsiots_p->common, FBE_RAID_COMMON_FLAG_WAITING_FOR_MEMORY))
            {
                /* Abort any outstanding memory request
                 */
                fbe_raid_siots_transition(nsiots_p, fbe_raid_siots_aborted);
                status = fbe_raid_memory_api_abort_request(fbe_raid_siots_get_memory_request(nsiots_p));
                if (RAID_MEMORY_COND(status != FBE_STATUS_OK))
                {
                    fbe_raid_siots_trace(nsiots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                     "raid: siots_p: 0x%p failed to abort memory request with status: 0x%x \n",
                                     nsiots_p, status);
                    return FBE_STATUS_GENERIC_FAILURE;
                }
            }
            nsiots_p = fbe_raid_siots_get_next(nsiots_p);
        }
        siots_p = fbe_raid_siots_get_next(siots_p);
    }
    return FBE_STATUS_OK;
}
/*****************************************
 * fbe_raid_iots_abort_monitor_op()
 *****************************************/

/***************************************************************************
 * fbe_raid_update_master_block_status()
 ***************************************************************************
 * @brief
 *  This function updates the status of master block operation 
 *  if master block operation already has a bad status it is not updated
 *
 *  @param mbo - master block operation
 *  @param bo - block operation to update from.
 *
 * @return FBE_STATUS_OK;
 *
 * @author
 *  02/25/2013 - Created. Peter Puhov
 *
 **************************************************************************/
fbe_status_t 
fbe_raid_update_master_block_status(fbe_payload_block_operation_t * mbo, fbe_payload_block_operation_t	* bo)
{
	fbe_raid_common_error_precedence_t mbo_error_precedence;
	fbe_raid_common_error_precedence_t bo_error_precedence;

    /* Else determine the precidence of errors. */
    mbo_error_precedence = fbe_raid_siots_get_error_precedence(mbo->status, mbo->status_qualifier);
    bo_error_precedence = fbe_raid_siots_get_error_precedence(bo->status, bo->status_qualifier);

    /* Only if the bo status takes precedence do we update the mbo status.*/
    if ((mbo->status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID) || (bo_error_precedence > mbo_error_precedence) ){
        /* The bo error clearly takes precidence,
         * over the mbo error, so overwrite the mbo error.
         * Keep the qualifier paired with the block operation status.
         */

		fbe_payload_block_set_status(mbo, bo->status, bo->status_qualifier);
    }

	return FBE_STATUS_OK;
}
/*****************************************
 * fbe_raid_get_checked_byte_count()
 *****************************************/

/*!**************************************************************
 * fbe_raid_iots_handle_config_change()
 ****************************************************************
 * @brief
 *  For this iots try to handle a `configuration change'.  The only
 *  difference between this method an `handle shutdown' is the
 *  block qualifier is set to:
 *      o FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_POSSIBLE
 * instead of:
 *      o FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE
 *  If the siots are not doing anything, then we can
 *  simply mark this iots complete and put it on the restart
 *  queue in prepartion for completion.
 *
 * @param iots_p - The iots to check shutdown for.
 * @param restart_queue_p - The queue to put ourselves on if we
 *                          need to get restarted.
 *
 * @return fbe_status_t.
 *
 * @author
 *  3/8/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_iots_handle_config_change(fbe_raid_iots_t *iots_p,
                                                fbe_queue_head_t *restart_queue_p)
{
    fbe_status_t status;
    fbe_raid_geometry_t *geo_p = fbe_raid_iots_get_raid_geometry(iots_p);
    fbe_object_id_t object_id = fbe_raid_geometry_get_object_id(geo_p);

    fbe_raid_iots_lock(iots_p);

    /* Mark the request with retryable so that
     * the caller knows that request failed,
     * Also mark the request aborted.
     */
    fbe_raid_iots_set_block_status(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED);
    fbe_raid_iots_set_block_qualifier(iots_p, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_POSSIBLE);
    fbe_raid_iots_set_flag(iots_p, FBE_RAID_IOTS_FLAG_ABORT);

    /* Try to stop the iots.  This will abort any outstanding memory requests.
     */
    status = fbe_raid_iots_quiesce(iots_p);
    if (status != FBE_STATUS_OK) 
    {
        fbe_raid_library_object_trace(object_id, FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                      "raid_iots_handle_config_change iots 0x%p quiesce error: 0x%x\n",
                                      iots_p, status); 
    }
    if (fbe_raid_iots_is_quiesced(iots_p))
    {
        /* We should not have completed this already.
         */
        if (RAID_COND(iots_p->status == FBE_RAID_IOTS_STATUS_COMPLETE))
        {
            fbe_raid_library_object_trace(object_id, FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                        "raid_iots_handle_config_change: stat 0x%x Unexpected\n",
                                        iots_p->status);
            fbe_raid_iots_unlock(iots_p);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        if (iots_p->status == FBE_RAID_IOTS_STATUS_COMPLETE)
        {
            /* We should not be marked completed yet.
             */
            fbe_raid_library_object_trace(object_id, FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                          "raid_iots_handle_config_change: iots 0x%p already mark for object\n",
                                          iots_p);

        }

        fbe_raid_library_object_trace(object_id, FBE_RAID_LIBRARY_TRACE_PARAMS_INFO,
                                      "raid_iots_handle_config_change; iots:0x%p mark complete for object\n",
                                      iots_p);
        fbe_raid_iots_mark_complete(iots_p);
        fbe_raid_common_wait_enqueue_tail(restart_queue_p, &iots_p->common);
        status = FBE_STATUS_OK;
    }
    else
    {
        /* This iots is not done, we need to wait for it.
         */
   
        fbe_raid_library_object_trace(object_id, FBE_RAID_LIBRARY_TRACE_PARAMS_INFO,
                                      "raid_iots_handle_config_change: iots:0x%p abort for shutdown\n",
                                      iots_p);
        
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_raid_iots_unlock(iots_p);

    /* Always return the execution status
     */
    return status;
}
/******************************************
 * end fbe_raid_iots_handle_config_change()
 ******************************************/

/*!**************************************************************
 * fbe_raid_is_aligned_to_optimal_block_size()
 ****************************************************************
 * @brief
 *  Determine if the passed in block size/count are aligned.
 *  
 * @param lba - Logical block address to check.
 * @param blocks - Blocks to check.
 * @param optimal_block_size - Number of blocks to align to.
 *
 * @return - fbe_bool_t - True if aligned, FBE_FALSE otherwise.
 *
 * @author
 *  12/10/2008 - Created. Rob Foley
 *
 ****************************************************************/

fbe_bool_t fbe_raid_is_aligned_to_optimal_block_size(fbe_lba_t lba,
                                          fbe_block_count_t blocks,
                                          fbe_u32_t optimal_block_size)
{
    /* Better to fbe_panic than generate an exception.
     */
    if(RAID_COND(optimal_block_size == 0) )
    {
        fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                            "%s status: Optimal block size is zero 0x%x\n",
                            __FUNCTION__, optimal_block_size);
        return FBE_FALSE;
    }

    /* Check if the start lba or end lba is misaligned.
     */
    if ( (lba % optimal_block_size) ||
         ( (lba + blocks) % optimal_block_size ) )
    {
        /* Misaligned, return FBE_FALSE.
         */
        return FBE_FALSE;
    }
    else
    {
        /* Aligned, return FBE_TRUE.
         */
        return FBE_TRUE;
    }
}
/*************************************************
 * end fbe_raid_is_aligned_to_optimal_block_size()
 *************************************************/


/*************************
 * end file fbe_raid_util.c
 *************************/




