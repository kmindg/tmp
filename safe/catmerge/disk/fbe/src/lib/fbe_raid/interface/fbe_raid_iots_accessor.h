#ifndef FBE_RAID_IOTS_ACCESSOR_H
#define FBE_RAID_IOTS_ACCESSOR_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/
 
/*!*************************************************************************
 * @file fbe_raid_iots_accessor.h
 ***************************************************************************
 *
 * @brief
 *  This file contains accessor methods for the raid iots structure.
 *
 * @version
 *   5/19/2009:  Created. rfoley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe_raid_library.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/


/* Accessors for packet we are embedded inside of.
 */
static __forceinline void fbe_raid_iots_get_priority(fbe_raid_iots_t *const iots_p,
                                              fbe_packet_priority_t *priority_p)
{
    fbe_transport_get_packet_priority(iots_p->packet_p, priority_p);
    return;
}
static __forceinline void fbe_raid_iots_set_priority(fbe_raid_iots_t *const iots_p,
                                              fbe_packet_priority_t priority)
{
    fbe_transport_set_packet_priority(iots_p->packet_p, priority);
    return;
}
static __forceinline void fbe_raid_iots_get_block_operation_payload(fbe_raid_iots_t *const iots_p,
                                                             fbe_payload_block_operation_t **block_payload_p)
{
    fbe_payload_ex_t                   *payload_p  = NULL;
    payload_p = fbe_transport_get_payload_ex(fbe_raid_iots_get_packet(iots_p));

    *block_payload_p = fbe_payload_ex_get_block_operation(payload_p);
    return;
}

static __forceinline fbe_memory_request_t *fbe_raid_iots_get_memory_request(fbe_raid_iots_t *const iots_p)
{
    /*! @note The iots needs it's own memory request since metadata I/O will
     *        use and hold the memory request in the packet.
     */
    return(&iots_p->memory_request);
}
static __forceinline void fbe_raid_iots_get_memory_request_priority(fbe_raid_iots_t *iots_p,
                                                                    fbe_memory_request_priority_t *priority_p)
{
    fbe_memory_request_t   *memory_request_p = fbe_raid_iots_get_memory_request(iots_p);

    *priority_p = memory_request_p->priority;
    return;
}
static __forceinline void fbe_raid_iots_set_memory_request_priority(fbe_raid_iots_t *iots_p,
                                                                    fbe_memory_request_priority_t new_priority)
{
    fbe_memory_request_t   *memory_request_p = fbe_raid_iots_get_memory_request(iots_p);

    memory_request_p->priority = new_priority;
    return;
}

static __forceinline void fbe_raid_iots_get_memory_request_io_stamp(fbe_raid_iots_t *iots_p,
                                                                    fbe_memory_io_stamp_t *io_stamp_p)
{
    fbe_memory_request_t   *memory_request_p = fbe_raid_iots_get_memory_request(iots_p);

    *io_stamp_p = memory_request_p->io_stamp;
    return;
}
static __forceinline void fbe_raid_iots_set_memory_request_io_stamp(fbe_raid_iots_t *iots_p,
                                                                    fbe_memory_io_stamp_t new_io_stamp)
{
    fbe_memory_request_t   *memory_request_p = fbe_raid_iots_get_memory_request(iots_p);

    memory_request_p->io_stamp = new_io_stamp;
    return;
}

/* Remaining accessors
 */
static __forceinline void fbe_raid_iots_get_callback(fbe_raid_iots_t * const iots_p,
                                              fbe_raid_iots_completion_function_t *callback_p,
                                              fbe_raid_iots_completion_context_t *context_p)
{
    *callback_p = iots_p->callback;
    *context_p = iots_p->callback_context;
    return;
}



/* Blocks transferred accessor.
 */
static __forceinline void fbe_raid_iots_get_blocks_transferred(fbe_raid_iots_t *const iots_p,
                                                        fbe_block_count_t *blocks_p)
{
    *blocks_p = iots_p->blocks_transferred;
    return;
}
static __forceinline void fbe_raid_iots_set_blocks_transferred(fbe_raid_iots_t *const iots_p,
                                                        fbe_block_count_t blocks)
{
    iots_p->blocks_transferred = blocks;
    return;
}

static __forceinline void fbe_raid_iots_get_sg_ptr(fbe_raid_iots_t *const iots_p,
                                            fbe_sg_element_t **sg_p)
{
    fbe_payload_ex_t                   *payload_p  = NULL;
    payload_p = fbe_transport_get_payload_ex(fbe_raid_iots_get_packet(iots_p));

    fbe_payload_ex_get_sg_list(payload_p, sg_p, NULL);
    return;
}
void fbe_raid_iots_get_verify_eboard(fbe_raid_iots_t *const iots_p, 
                                     fbe_raid_verify_error_counts_t **eboard_p);

/* Active queue accessors
 */
static __forceinline void fbe_raid_iots_get_siots_queue(fbe_raid_iots_t *const iots_p,
                                                 fbe_queue_head_t **queue_head_p)
{
    *queue_head_p = &iots_p->siots_queue;
    return;
}
static __forceinline void fbe_raid_iots_get_siots_queue_head(fbe_raid_iots_t *const iots_p,
                                                      fbe_raid_siots_t **siots_pp)
{
    *siots_pp = (fbe_raid_siots_t *)fbe_queue_front(&iots_p->siots_queue);
    return;
}
static __forceinline void fbe_raid_iots_get_siots_queue_tail(fbe_raid_iots_t *const iots_p,
                                                      fbe_raid_siots_t **siots_pp)
{
	if(fbe_queue_is_empty(&iots_p->siots_queue)){
		*siots_pp = NULL;
	} else {
		*siots_pp = (fbe_raid_siots_t *)iots_p->siots_queue.prev;
	}
    return;
}

/* `Available' queue accessors
 */
static __forceinline void fbe_raid_iots_get_available_queue(fbe_raid_iots_t *const iots_p,
                                                            fbe_queue_head_t **queue_head_p)
{
    *queue_head_p = &iots_p->available_siots_queue;
    return;
}
static __forceinline void fbe_raid_iots_get_available_queue_head(fbe_raid_iots_t *const iots_p,
                                                             fbe_raid_siots_t **siots_pp)
{
    *siots_pp = (fbe_raid_siots_t *)fbe_queue_front(&iots_p->available_siots_queue);
    return;
}
static __forceinline void fbe_raid_iots_get_available_queue_tail(fbe_raid_iots_t *const iots_p,
                                                             fbe_raid_siots_t **siots_pp)
{
	if(fbe_queue_is_empty(&iots_p->available_siots_queue)){
		*siots_pp = NULL;
	} else {
		*siots_pp = (fbe_raid_siots_t *)iots_p->available_siots_queue.prev;
	}
    return;
}

/* Get the block operation flags.
 */
static __forceinline void fbe_raid_iots_get_block_flags(fbe_raid_iots_t *const iots_p,
                                                 fbe_payload_block_operation_flags_t *block_operation_flags_p)
{
    fbe_payload_block_get_flags(iots_p->block_operation_p, block_operation_flags_p); 
    return;
}

/* Accessors for status.
 */

static __forceinline void fbe_raid_iots_get_host_start_offset(fbe_raid_iots_t *const iots_p,
                                                       fbe_block_count_t *offset_p)
{
    *offset_p = iots_p->host_start_offset;
    return;
}
static __forceinline fbe_bool_t fbe_raid_iots_is_buffered_op(fbe_raid_iots_t *iots_p)
{
    fbe_payload_block_operation_opcode_t opcode;

    /* There are a few opcodes where we will buffer the transfer like zeros.
     */
    fbe_raid_iots_get_opcode(iots_p, &opcode);

    if ((opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ZERO) ||
        (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_ZEROS) ||
        (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_REKEY_WRITE_ZEROS))
    {
        return FBE_TRUE;
    }
    return FBE_FALSE;
}
/*****************************************
 * fbe_raid_iots_transition
 *
 * Transition this iots to another state.
 *****************************************/
static __inline void fbe_raid_iots_transition(fbe_raid_iots_t *iots_p, fbe_raid_iots_state_t state)
{
    fbe_raid_common_set_state(&iots_p->common, (fbe_raid_common_state_t)state);
    return;
}
static __inline fbe_raid_iots_state_t fbe_raid_iots_get_state(fbe_raid_iots_t *iots_p)
{
    return (fbe_raid_iots_state_t)fbe_raid_common_get_state(&iots_p->common);
}

static __forceinline void fbe_raid_iots_get_edge(fbe_raid_iots_t *iots_p,
                                          fbe_block_edge_t **edge_pp)
{
    *edge_pp = (fbe_block_edge_t*)fbe_transport_get_edge(fbe_raid_iots_get_packet(iots_p));
    return;
}
static __forceinline fbe_raid_geometry_t *fbe_raid_iots_get_raid_geometry(fbe_raid_iots_t *iots_p)
{
    return((fbe_raid_geometry_t *)iots_p->raid_geometry_p);
}

fbe_bool_t fbe_raid_iots_is_marked_quiesce(fbe_raid_iots_t *iots_p);
fbe_bool_t fbe_raid_iots_should_check_checksums(fbe_raid_iots_t *iots_p);
fbe_bool_t fbe_raid_iots_is_corrupt_crc(fbe_raid_iots_t *iots_p);
void fbe_raid_iots_inc_errors(fbe_raid_iots_t *iots_p);
void fbe_raid_iots_dec_errors(fbe_raid_iots_t *iots_p);

/*!***********************************************************
 * fbe_raid_iots_dec_blocks()
 ************************************************************
 * @brief
 *  Adjust the IOTS to account for the SIOTS we just allocated.
 ************************************************************/

static __forceinline fbe_status_t fbe_raid_iots_dec_blocks(fbe_raid_iots_t *const iots_p,
                                            fbe_block_count_t blocks)
{
    if (blocks > iots_p->blocks_remaining)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    iots_p->current_lba += blocks;
    iots_p->blocks_remaining -= blocks;

    if (iots_p->blocks_remaining == 0)
    {
        fbe_raid_iots_lock(iots_p);
        fbe_raid_iots_clear_flag(iots_p, FBE_RAID_IOTS_FLAG_GENERATING);
        fbe_raid_iots_set_flag(iots_p, FBE_RAID_IOTS_FLAG_GENERATE_DONE);
        fbe_raid_iots_unlock(iots_p);
    }
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_raid_iots_dec_blocks()
 **************************************/
/*!***********************************************************
 * fbe_raid_iots_is_errored()
 ************************************************************
 * @brief
 *  Determines if this iots has encountered an error.
 * 
 * @param iots_p - The iots to check.
 * 
 * @return FBE_TRUE - If this iots is failed.
 *        FBE_FALSE - Otherwise, not failed.
 * 
 ************************************************************/
static __forceinline fbe_bool_t fbe_raid_iots_is_errored(fbe_raid_iots_t *iots_p)
{
    fbe_bool_t b_status = FBE_FALSE;

    /*  - If the error is simply invalid then a siots has not completed yet. 
     *  - If the error is success then at least one siots has completed 
     * successfully. 
     *  - If it is anyting else, then a siots has completed with error.  
     */
    if ((iots_p->error != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID) &&
        (iots_p->error != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS))
    {
        b_status = FBE_TRUE;
    }
    return b_status;
}
/**************************************
 * end fbe_raid_iots_is_errored()
 **************************************/

/*!****************************************************************************
 * fbe_raid_iots_is_rebuild_logging()
 ******************************************************************************
 * @brief 
 *  Determine if this position in this iots should use the new key.
 *
 * @return fbe_status_t         
 *
 ******************************************************************************/

static __forceinline fbe_status_t fbe_raid_iots_is_rebuild_logging(fbe_raid_iots_t *iots_p,
                                                            fbe_u32_t        pos,
                                                            fbe_bool_t*      b_is_rebuild_logging)
{
    *b_is_rebuild_logging = ((iots_p->rebuild_logging_bitmask & (1 << pos)) != 0);
    return FBE_STATUS_OK;
} 
/**************************************
 * end fbe_raid_iots_is_rebuild_logging()
 **************************************/
/*!****************************************************************************
 * fbe_raid_iots_use_new_encryption_key()
 ******************************************************************************
 * @brief 
 *  Determine if this position in this iots is logging.
 *
 * @return fbe_status_t         
 *
 ******************************************************************************/

static __forceinline fbe_status_t fbe_raid_iots_use_new_encryption_key(fbe_raid_iots_t *iots_p,
                                                                       fbe_u32_t        pos,
                                                                       fbe_bool_t*      b_use_new_key)
{
    *b_use_new_key = ((iots_p->rekey_bitmask & (1 << pos)) != 0);
    return FBE_STATUS_OK;
} 
/**************************************
 * end fbe_raid_iots_use_new_encryption_key()
 **************************************/

/*!****************************************************************************
 * fbe_raid_iots_set_rekey_bitmask()
 ******************************************************************************
 * @brief 
 *  Determine if this position in this iots is logging.
 *
 * @return fbe_status_t         
 *
 ******************************************************************************/

static __forceinline fbe_status_t fbe_raid_iots_set_rekey_bitmask(fbe_raid_iots_t *iots_p,
                                                                  fbe_raid_position_bitmask_t bitmask)
{
   iots_p->rekey_bitmask = bitmask;
    return FBE_STATUS_OK;
} 
/**************************************
 * end fbe_raid_iots_set_rekey_bitmask()
 **************************************/
/*!****************************************************************************
 * fbe_raid_iots_rebuild_logs_available()
 ******************************************************************************
 * @brief 
 *  Determine if anything in this range is potentially degraded.
 *
 * @return fbe_status_t         
 *
 ******************************************************************************/

static __forceinline fbe_status_t 
fbe_raid_iots_rebuild_logs_available(fbe_raid_iots_t *iots_p,
                                     fbe_u32_t pos,
                                     fbe_bool_t* b_possibly_degraded)
{
    /* We always break up across boundaries.  So if the first chunk is degraded, 
     * then all chunks are degraded. 
     */
    if (iots_p->chunk_info[0].needs_rebuild_bits & (1 << pos))
    {
        *b_possibly_degraded = FBE_TRUE;
        return FBE_STATUS_OK;
    }
    *b_possibly_degraded = FBE_FALSE;
    return FBE_STATUS_OK;
} 
/**************************************
 * end fbe_raid_iots_rebuild_logs_available()
 **************************************/


/*!****************************************************************************
 * fbe_raid_iots_get_chunk_info()
 ******************************************************************************
 * @brief 
 *  Determine the chunk info ptr for the given position.
 *
 * @return fbe_status_t         
 *
 ******************************************************************************/

static __forceinline fbe_status_t 
fbe_raid_iots_get_chunk_info(fbe_raid_iots_t *iots_p,
                             fbe_u32_t pos,
                             fbe_raid_group_paged_metadata_t** chunk_info_p)
{
    *chunk_info_p = NULL;
    if (pos >= FBE_RAID_IOTS_MAX_CHUNKS)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    *chunk_info_p = &iots_p->chunk_info[pos];
    return FBE_STATUS_OK;
} 
/**************************************
 * end fbe_raid_iots_get_chunk_info()
 **************************************/
/*********************************
 * end file fbe_raid_iots_accessor.h
 *********************************/
#endif  // endif ndef FBE_RAID_IOTS_ACCESSOR_H

