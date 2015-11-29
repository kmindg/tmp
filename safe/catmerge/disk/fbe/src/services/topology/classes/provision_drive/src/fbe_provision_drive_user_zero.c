/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_provision_drive_user_zero.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the code to access the bitmap for the provision drive
 *  object.
 * 
 * @ingroup provision_drive_class_files
 * 
 * @version
 *   05/20/2009:  Created. Dhaval Patel
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_provision_drive.h"
#include "fbe_provision_drive_private.h"
#include "fbe_transport_memory.h"
#include "fbe_raid_library.h"

/*****************************************
 * LOCAL FUNCTION PROTOTYPES
 *****************************************/


/* it is used to update the paged metadata for the user zero request. */
static fbe_status_t fbe_provision_drive_user_zero_update_paged_metadata(fbe_provision_drive_t * provision_drive_p,
                                                                        fbe_packet_t * packet_p,
                                                                        fbe_packet_completion_function_t completion_function,
                                                                        fbe_bool_t b_is_paged_metadata_valid);

/* it issues write same operation to perform the zero operation for the edges. */
static fbe_status_t fbe_provision_drive_user_zero_issue_write_same_if_needed(fbe_provision_drive_t * provision_drive_p,
                                                                             fbe_packet_t * packet_p);

static fbe_status_t fbe_provision_drive_user_zero_update_paged_and_checkpoint(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);
static fbe_status_t fbe_provision_drive_consumed_area_zero_update_paged_and_checkpoint(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);
static fbe_status_t fbe_provision_drive_consumed_area_zero_update_paged_metadata_completion(fbe_packet_t * packet_p,
                                                                                            fbe_packet_completion_context_t context);

static fbe_status_t fbe_provision_drive_user_zero_update_checkpoint_completion(fbe_packet_t * packet_p,
                                                                               fbe_packet_completion_context_t context);

static fbe_status_t fbe_provision_drive_user_zero_send_subpackets(fbe_provision_drive_t * provision_drive_p,
                                                                                fbe_packet_t * packet_p);

static fbe_status_t fbe_provision_drive_user_zero_update_paged_metadata_completion(fbe_packet_t * packet_p,
                                                                                   fbe_packet_completion_context_t context);

static fbe_status_t fbe_provision_drive_disk_zero_update_paged_metadata_completion(fbe_packet_t * packet_p,
                                                                                   fbe_packet_completion_context_t context);

static fbe_status_t fbe_provision_drive_user_zero_write_same_subpacket_completion(fbe_packet_t * packet_p,
                                                                                  fbe_packet_completion_context_t context);

static fbe_status_t fbe_provision_drive_zero_request_set_checkpoint(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);

static fbe_status_t
fbe_provision_drive_disk_zero_clear_bits(fbe_packet_t * packet_p,
                                         fbe_provision_drive_t* provision_drive_p,
                                         fbe_payload_block_operation_t *  block_operation_p,
                                         fbe_payload_metadata_operation_t *      metadata_operation_p,
                                         fbe_payload_ex_t *        sep_payload_p,
                                         fbe_packet_completion_function_t completion_function,
                                         fbe_bool_t b_is_paged_metadata_valid);
static fbe_status_t
fbe_provision_drive_user_zero_write_verify_update_paged_metadata_completion(fbe_packet_t * packet_p,
                                                                            fbe_packet_completion_context_t context);
static fbe_status_t
fbe_provision_drive_user_zero_handle_paged_metadata_update_completion(fbe_packet_t * packet_p,
                                                                      fbe_provision_drive_t* provision_drive_p,
                                                                      fbe_payload_block_operation_t *  block_operation_p,
                                                                      fbe_payload_metadata_operation_t *      metadata_operation_p,
                                                                      fbe_payload_ex_t *        sep_payload_p);
static fbe_status_t
fbe_provision_drive_disk_zero_write_verify_update_paged_metadata_completion(fbe_packet_t * packet_p,
                                                                            fbe_packet_completion_context_t context);
static fbe_status_t 
fbe_provision_drive_zero_request_set_scrub_needed_bit(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);
static fbe_status_t 
fbe_provision_drive_zero_request_clear_scrub_intent_bits(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);

/*****************************************
 * LOCAL FUNCTION DEFINITIONS
 *****************************************/


/*!****************************************************************************
 * @fn fbe_provision_drive_consumed_area_zero_handle_request()
 ******************************************************************************
 * @brief
 *  This function update paged metadata for starting chunk. 
 *  The md callback defines if we need to write back or not.
 *
 * @param provision_drive_p - Pointer to the provision drive object.
 * @param packet_p          - Pointer the the I/O packet.
 *
 * @return fbe_status_t.
 *
 * @author
 *  02/01/2014  NCHIU  - Created.
 *
 ******************************************************************************/
fbe_status_t
fbe_provision_drive_consumed_area_zero_handle_request(fbe_provision_drive_t * provision_drive_p, 
                                                      fbe_packet_t * packet_p)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_payload_ex_t *                      sep_payload_p = NULL;
    fbe_payload_block_operation_t *         block_operation_p = NULL;
    fbe_lba_t                               start_lba;
    fbe_block_count_t                       block_count;
	fbe_optimum_block_size_t                optimum_block_size;
	fbe_bool_t								is_zero_request_aligned;
	fbe_chunk_index_t                       start_chunk_index;
	fbe_u32_t                               chunk_count;

    /* get the sep payload and block operation. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_get_present_block_operation(sep_payload_p);

    /* get the start lba and block count from the block operation. */
    fbe_payload_block_get_lba(block_operation_p, &start_lba);
    fbe_payload_block_get_block_count(block_operation_p, &block_count);

    status = fbe_block_transport_edge_get_optimum_block_size(&provision_drive_p->block_edge, &optimum_block_size);
    if(status != FBE_STATUS_OK) {
        /* This may be a case where PVD is in READY state with SLF and try to handle io with 
         * Invalid downstream block edge. Normally if PVD is in SLF state, the IO should redirect 
         * before come to this code path.
         */
        fbe_provision_drive_utils_trace(provision_drive_p, 
                                        FBE_TRACE_LEVEL_ERROR,
                                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                        FBE_PROVISION_DRIVE_DEBUG_FLAG_USER_ZERO_TRACING,
                                        "UZ: failed to get optimum block size, lba:0x%llx, blks:0x%llx, packet:%p\n",
                                        (unsigned long long)start_lba, (unsigned long long)block_count, packet_p);

        fbe_payload_block_set_status(block_operation_p,
                FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNEXPECTED_ERROR);

        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);        
        fbe_transport_run_queue_push_packet(packet_p, FBE_TRANSPORT_RQ_METHOD_SAME_CORE);

        return FBE_STATUS_GENERIC_FAILURE;
    }    

    /* find out whether incoming zeroing request is aligned or not. */
    /*!@todo we might move this check later to handle the case if the incoming request
     * is not aligned but PVD still has need zero bit set for its partial range then 
     * we can handle this request.
     */
    status = fbe_provision_drive_utils_is_io_request_aligned(start_lba, block_count, optimum_block_size, &is_zero_request_aligned);

    if((status != FBE_STATUS_OK) || (!is_zero_request_aligned)) {
        fbe_provision_drive_utils_trace(provision_drive_p, 
                                        FBE_TRACE_LEVEL_ERROR,
                                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                        FBE_PROVISION_DRIVE_DEBUG_FLAG_USER_ZERO_TRACING,
                                        "UZ:unaligned request, status:0x%x, lba:0x%llx, blks:0x%llx, packet:%p\n",
                                        status, (unsigned long long)start_lba, (unsigned long long)block_count, packet_p);

        fbe_payload_block_set_status(block_operation_p,
                                     FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                                     FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNALIGNED_ZERO_REQUEST);

        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);        
		fbe_transport_run_queue_push_packet(packet_p, FBE_TRANSPORT_RQ_METHOD_SAME_CORE);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* determine whether zero on demand flag is already set. */

    /* calculate the chunk range from the lba and block count of the zero request. */
    fbe_provision_drive_utils_calculate_chunk_range_without_edges(start_lba,
                                                                  block_count,
                                                                  FBE_PROVISION_DRIVE_CHUNK_SIZE,
                                                                  &start_chunk_index,
                                                                  &chunk_count);

    fbe_provision_drive_utils_trace(provision_drive_p,
                                    FBE_TRACE_LEVEL_INFO,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    FBE_PROVISION_DRIVE_DEBUG_FLAG_USER_ZERO_TRACING,
                                    "UZ:consumed area zero request, start_lba:0x%llx, blk:0x%llx, chk_count:0x%x.\n",
                                    (unsigned long long)start_lba, (unsigned long long)block_count, chunk_count);

    fbe_provision_drive_get_NP_lock(provision_drive_p, packet_p, fbe_provision_drive_consumed_area_zero_update_paged_and_checkpoint);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}

/******************************************************************************
 * end fbe_provision_drive_consumed_area_zero_handle_request()
 ******************************************************************************/

/*!****************************************************************************
 * @fn fbe_provision_drive_consumed_area_zero_update_paged_and_checkpoint()
 ******************************************************************************
 * @brief
 *  This function update paged metadata for starting chunk after getting the NP lock. 
 *
 * @param packet_p          - Pointer the the I/O packet.
 * @param context           - Pointer to the provision drive object.
 *
 * @return fbe_status_t.
 *
 * @author
 *  02/01/2014  NCHIU  - Created.
 *
 ******************************************************************************/
static fbe_status_t 
fbe_provision_drive_consumed_area_zero_update_paged_and_checkpoint(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{
    fbe_provision_drive_t * provision_drive_p = (fbe_provision_drive_t *)context;
    fbe_status_t status;
    fbe_payload_ex_t *payload_p = NULL;
    fbe_payload_stripe_lock_operation_t *stripe_lock_operation_p = NULL;
    fbe_payload_stripe_lock_status_t stripe_lock_status;
	fbe_provision_drive_config_type_t     config_type;

    payload_p = fbe_transport_get_payload_ex(packet_p);
    stripe_lock_operation_p = fbe_payload_ex_get_stripe_lock_operation(payload_p);
    fbe_payload_stripe_lock_get_status(stripe_lock_operation_p, &stripe_lock_status);

    status = fbe_transport_get_status_code(packet_p);
    if ((status != FBE_STATUS_OK) || (stripe_lock_status != FBE_PAYLOAD_STRIPE_LOCK_STATUS_OK)){
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "UZ: get np lock failed packet status: 0x%x sl_st: 0x%x pkt: %p\n",
                              status, stripe_lock_status, packet_p);
        return FBE_STATUS_OK;
    }

    fbe_provision_drive_utils_trace(provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                    FBE_PROVISION_DRIVE_DEBUG_FLAG_USER_ZERO_TRACING,
                                    "UZ:set_chkpt NP lock acquired\n");

    fbe_transport_set_completion_function(packet_p, fbe_provision_drive_release_NP_lock, provision_drive_p);

    fbe_provision_drive_get_config_type(provision_drive_p, &config_type);

    /* need to scrub all unused PVD */
    if ((config_type != FBE_PROVISION_DRIVE_CONFIG_TYPE_RAID) ||
        fbe_provision_drive_metadata_is_np_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_NP_FLAG_PAGED_SCRUB_INTENT))
    {
        fbe_transport_set_completion_function(packet_p, fbe_provision_drive_zero_request_set_checkpoint, provision_drive_p);

	    fbe_provision_drive_user_zero_update_paged_metadata(provision_drive_p, packet_p,
														    fbe_provision_drive_user_zero_update_paged_metadata_completion,
														    FBE_TRUE /* The paged metadata is valid. Preserve unmodified bits. */);
        /* need more work */
        status = FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }
    else
    {
        fbe_transport_set_completion_function(packet_p, fbe_provision_drive_zero_request_set_scrub_needed_bit, provision_drive_p);
        status =  FBE_STATUS_CONTINUE;  /* make sure the new completion is called */
    }

	return status;
}

/*!****************************************************************************
 * @fn fbe_provision_drive_zero_request_set_scrub_bits()
 ******************************************************************************
 * @brief
 *
 *  This is the callback function after the nonpaged lock. 
 * 
 * @param sg_list           - Pointer to sg list.
 * @param slot_offset       - Offset to the paged metadata.
 * @param context           - Pointer to context.
 *
 * @return fbe_status_t.
 *
 * @author
 *  02/01/2014  NCHIU  - Created.
 *
 ******************************************************************************/
static fbe_status_t 
fbe_provision_drive_zero_request_set_scrub_needed_bit(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{
    fbe_provision_drive_t * provision_drive_p = (fbe_provision_drive_t *)context;
    fbe_status_t status;
    fbe_provision_drive_np_flags_t          np_flags;

    status = fbe_provision_drive_metadata_get_np_flag(provision_drive_p, &np_flags);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "UZ: failed to get non-paged flags, status %d\n",
                              status);
        return FBE_STATUS_OK;
    }

    np_flags |= FBE_PROVISION_DRIVE_NP_FLAG_PAGED_SCRUB_NEEDED;
    np_flags &= ~FBE_PROVISION_DRIVE_NP_FLAG_PAGED_SCRUB_INTENT;

    /* this function always returns FBE_STATUS_MORE_PROCESSING_REQUIRED */
    status = fbe_provision_drive_metadata_np_flag_set(provision_drive_p, packet_p, np_flags);

	return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}

/*!****************************************************************************
 * @fn fbe_provision_drive_zero_request_clear_scrub_intent_bits()
 ******************************************************************************
 * @brief
 *
 *  This is the callback function after the nonpaged lock. 
 * 
 * @param sg_list           - Pointer to sg list.
 * @param slot_offset       - Offset to the paged metadata.
 * @param context           - Pointer to context.
 *
 * @return fbe_status_t.
 *
 * @author
 *  02/07/2014  NCHIU  - Created.
 *
 ******************************************************************************/
static fbe_status_t 
fbe_provision_drive_zero_request_clear_scrub_intent_bits(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{
    fbe_provision_drive_t * provision_drive_p = (fbe_provision_drive_t *)context;
    fbe_status_t status;
    fbe_provision_drive_np_flags_t          np_flags;

    status = fbe_provision_drive_metadata_get_np_flag(provision_drive_p, &np_flags);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "UZ: failed to get non-paged flags, status %d\n",
                              status);
        return FBE_STATUS_OK;
    }

    np_flags &= ~FBE_PROVISION_DRIVE_NP_FLAG_PAGED_SCRUB_INTENT;

    /* this function always returns FBE_STATUS_MORE_PROCESSING_REQUIRED */
    status = fbe_provision_drive_metadata_np_flag_set(provision_drive_p, packet_p, np_flags);

	return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}


/*!****************************************************************************
 * @fn fbe_provision_drive_consumed_area_zero_update_paged_metadata_completion()
 ******************************************************************************
 * @brief
 *
 *  This is the callback function after the paged metadata is read. 
 * 
 * @param sg_list           - Pointer to sg list.
 * @param slot_offset       - Offset to the paged metadata.
 * @param context           - Pointer to context.
 *
 * @return fbe_status_t.
 *
 * @author
 *  02/01/2014  NCHIU  - Created.
 *
 ******************************************************************************/
fbe_status_t 
fbe_provision_drive_consumed_area_zero_update_paged_metadata_completion(fbe_packet_t * packet, fbe_metadata_callback_context_t context)
{
    fbe_payload_ex_t * sep_payload = fbe_transport_get_payload_ex(packet);
    fbe_payload_metadata_operation_t * mdo = NULL;
    fbe_provision_drive_paged_metadata_t * paged_metadata_p;
    fbe_sg_element_t * sg_list = NULL;
    fbe_sg_element_t * sg_ptr = NULL;
    fbe_lba_t lba_offset;
    fbe_u64_t slot_offset;
    fbe_status_t status = FBE_STATUS_OK; /* indicates nothing needs change */
    fbe_provision_drive_t * provision_drive_p = (fbe_provision_drive_t *) context;

    if(sep_payload->current_operation->payload_opcode == FBE_PAYLOAD_OPCODE_METADATA_OPERATION){
        mdo = fbe_payload_ex_get_metadata_operation(sep_payload);
    } else if(sep_payload->current_operation->payload_opcode == FBE_PAYLOAD_OPCODE_STRIPE_LOCK_OPERATION){
        mdo = fbe_payload_ex_get_any_metadata_operation(sep_payload);
    } else {
        fbe_topology_class_trace(FBE_CLASS_ID_PROVISION_DRIVE, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                      "%s: Invalid payload opcode:%d \n", __FUNCTION__, sep_payload->current_operation->payload_opcode);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        return FBE_STATUS_GENERIC_FAILURE;
    }


    lba_offset = fbe_metadata_paged_get_lba_offset(mdo);
    slot_offset = fbe_metadata_paged_get_slot_offset(mdo);
    sg_list = mdo->u.metadata_callback.sg_list;
    sg_ptr = &sg_list[lba_offset - mdo->u.metadata_callback.start_lba];
    paged_metadata_p = (fbe_provision_drive_paged_metadata_t *)(sg_ptr->address + slot_offset);

    while ((sg_ptr->address != NULL) && (mdo->u.metadata_callback.current_count_private < mdo->u.metadata_callback.repeat_count))
    {
        /* if something needs to be zero'ed, scrub can't be marked as completed */
        if (paged_metadata_p->need_zero_bit == 1)
        {
            /* remember inside PVD */
            fbe_provision_drive_set_flag(provision_drive_p, FBE_PROVISION_DRIVE_FLAG_SCRUB_NEEDED);
        }

        if (paged_metadata_p->consumed_user_data_bit == 1)
        {
            /* remember inside PVD */
            fbe_provision_drive_set_flag(provision_drive_p, FBE_PROVISION_DRIVE_FLAG_SCRUB_NEEDED);

            fbe_provision_drive_utils_trace(provision_drive_p,
                                            FBE_TRACE_LEVEL_INFO,
                                            FBE_TRACE_MESSAGE_ID_INFO,
                                            FBE_PROVISION_DRIVE_DEBUG_FLAG_USER_ZERO_TRACING,
                                            "UZ: chunk count 0x%x, NZ %d, UZ  %d, DB %d\n",
                                            (mdo->u.metadata_callback.current_count_private),
                                            ((paged_metadata_p->need_zero_bit == TRUE) ? 1 : 0),
                                            ((paged_metadata_p->user_zero_bit == TRUE) ? 1 : 0),
                                            ((paged_metadata_p->consumed_user_data_bit == TRUE) ? 1 : 0));

            paged_metadata_p->consumed_user_data_bit = 0;

            /* clear the need zero bit if it is set. */
            paged_metadata_p->need_zero_bit = 1;

            /* clear the user zero bit if it is set. */
            paged_metadata_p->user_zero_bit = 0;

            status = FBE_STATUS_MORE_PROCESSING_REQUIRED; /* this round need persist */
        }

        mdo->u.metadata_callback.current_count_private++;
        paged_metadata_p++;
        if (paged_metadata_p == (fbe_provision_drive_paged_metadata_t *)(sg_ptr->address + FBE_METADATA_BLOCK_DATA_SIZE))
        {
            sg_ptr++;
            paged_metadata_p = (fbe_provision_drive_paged_metadata_t *)sg_ptr->address;
        }
    }

    return status;
}
/******************************************************************************
 * end fbe_provision_drive_consumed_area_zero_update_paged_metadata_completion()
 ******************************************************************************/

/*!****************************************************************************
 * @fn fbe_provision_drive_user_zero_handle_request()
 ******************************************************************************
 * @brief
 *  This function is used as entry poing for the zeroing I/O from the upstream
 *  object. This function sends a metadata request to read the paged metadata
 *  for starting chunk. The md read completion does rest of the processing.
 *
 * @param provision_drive_p - Pointer to the provision drive object.
 * @param packet_p          - Pointer the the I/O packet.
 *
 * @return fbe_status_t.
 *
 * @author
 *  4/15/2009 - Created. Dhaval Patel
 *  6/8/2012  - Prahlad Purohit - Modified to read UC for unaligned check.
 *
 ******************************************************************************/
fbe_status_t
fbe_provision_drive_user_zero_handle_request(fbe_provision_drive_t * provision_drive_p, 
                                                        fbe_packet_t * packet_p)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_payload_ex_t *                      sep_payload_p = NULL;
    fbe_payload_block_operation_t *         block_operation_p = NULL;
    fbe_lba_t                               start_lba;
    fbe_block_count_t                       block_count;
	fbe_optimum_block_size_t                optimum_block_size;
	fbe_bool_t								is_zero_request_aligned;
	fbe_chunk_index_t                       start_chunk_index;
	fbe_u32_t                               chunk_count;

    /* get the sep payload and block operation. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_get_present_block_operation(sep_payload_p);

    /* get the start lba and block count from the block operation. */
    fbe_payload_block_get_lba(block_operation_p, &start_lba);
    fbe_payload_block_get_block_count(block_operation_p, &block_count);

    status = fbe_block_transport_edge_get_optimum_block_size(&provision_drive_p->block_edge, &optimum_block_size);
    if(status != FBE_STATUS_OK) {
        /* This may be a case where PVD is in READY state with SLF and try to handle io with 
         * Invalid downstream block edge. Normally if PVD is in SLF state, the IO should redirect 
         * before come to this code path.
         */
        fbe_provision_drive_utils_trace(provision_drive_p, 
                FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                FBE_PROVISION_DRIVE_DEBUG_FLAG_USER_ZERO_TRACING,
                "UZ: failed to get optimum block size, lba:0x%llx, blks:0x%llx, packet:%p\n",
                (unsigned long long)start_lba, (unsigned long long)block_count, packet_p);

        fbe_payload_block_set_status(block_operation_p,
                FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNEXPECTED_ERROR);

        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);        
        fbe_transport_run_queue_push_packet(packet_p, FBE_TRANSPORT_RQ_METHOD_SAME_CORE);

        return FBE_STATUS_GENERIC_FAILURE;
    }    

    /* find out whether incoming zeroing request is aligned or not. */
    /*!@todo we might move this check later to handle the case if the incoming request
     * is not aligned but PVD still has need zero bit set for its partial range then 
     * we can handle this request.
     */
    status = fbe_provision_drive_utils_is_io_request_aligned(start_lba, block_count, optimum_block_size, &is_zero_request_aligned);

    if((status != FBE_STATUS_OK) || (!is_zero_request_aligned)) {
        fbe_provision_drive_utils_trace(provision_drive_p, 
                                        FBE_TRACE_LEVEL_ERROR,
                                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                        FBE_PROVISION_DRIVE_DEBUG_FLAG_USER_ZERO_TRACING,
                                        "UZ:unaligned request, status:0x%x, lba:0x%llx, blks:0x%llx, packet:%p\n",
                                        status, (unsigned long long)start_lba, (unsigned long long)block_count, packet_p);

        fbe_payload_block_set_status(block_operation_p,
                                     FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                                     FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNALIGNED_ZERO_REQUEST);

        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);        
		fbe_transport_run_queue_push_packet(packet_p, FBE_TRANSPORT_RQ_METHOD_SAME_CORE);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* determine whether zero on demand flag is already set. */

    /* calculate the chunk range from the lba and block count of the zero request. */
    fbe_provision_drive_utils_calculate_chunk_range_without_edges(start_lba,
                                                                  block_count,
                                                                  FBE_PROVISION_DRIVE_CHUNK_SIZE,
                                                                  &start_chunk_index,
                                                                  &chunk_count);

    fbe_provision_drive_utils_trace(provision_drive_p,
                                    FBE_TRACE_LEVEL_INFO,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    FBE_PROVISION_DRIVE_DEBUG_FLAG_USER_ZERO_TRACING,
                                    "UZ:zero request, start_lba:0x%llx, blk:0x%llx, chk_count:0x%x.\n",
                                    (unsigned long long)start_lba, (unsigned long long)block_count, chunk_count);


    if(chunk_count != 0) {
		fbe_provision_drive_get_NP_lock(provision_drive_p, packet_p, fbe_provision_drive_user_zero_update_paged_and_checkpoint);

        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    } else {
        /* send the write same request for user zero request which does not cover the chunk boundary. */
        status = fbe_provision_drive_user_zero_issue_write_same_if_needed(provision_drive_p, packet_p);
    }

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}

/******************************************************************************
 * end fbe_provision_drive_user_zero_handle_request()
 ******************************************************************************/


/*!****************************************************************************
 * fbe_provision_drive_zero_request_set_checkpoint()
 ******************************************************************************
 * @brief
 *  This function is used to set the BZ checkpoint as part of zero request
 *
 * @param in_packet_p      - pointer to a control packet from the scheduler.
 * @param in_context       - context, which is a pointer to the base object.
 * 
 * @return fbe_status_t    - Always FBE_STATUS_OK
 *
 * @author
 *  10/18/2011 - Created. Ashwin Tamilarasan
 *
 ******************************************************************************/
static fbe_status_t 
fbe_provision_drive_zero_request_set_checkpoint(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{
    fbe_provision_drive_t *                 provision_drive_p = NULL;
    fbe_status_t                            status;
    fbe_payload_ex_t *                     sep_payload_p = NULL;
    fbe_payload_block_operation_t *         block_operation_p = NULL;
    fbe_lba_t                               start_lba;
    fbe_block_count_t                       block_count;
    fbe_chunk_index_t                       start_chunk_index;
    fbe_u32_t                               chunk_count= 0;
    fbe_lba_t                               new_zero_checkpoint;
    fbe_object_id_t                 pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_lba_t                               current_zero_checkpoint;
    fbe_lba_t                       default_offset = 0;
    fbe_payload_block_operation_opcode_t    block_operation_opcode = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID;

    
    provision_drive_p = (fbe_provision_drive_t*)context;

   /* Get the sep payload and block operation. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_get_present_block_operation(sep_payload_p);
    fbe_payload_block_get_opcode(block_operation_p, &block_operation_opcode);

    fbe_payload_block_get_lba(block_operation_p, &start_lba);
    fbe_payload_block_get_block_count(block_operation_p, &block_count);

    /* If the status is not ok, that means we didn't get the 
       lock. Just return. we are already in the completion routine
    */
    status = fbe_transport_get_status_code (packet_p);
    if(status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "UZ:set_chkpt, packet status: 0x%X, lba:0x%llx blk:0x%llx\n", status, start_lba, block_count); 

        return FBE_STATUS_OK;
    }

    /* we don't have to do anything if the request is marked consumed zero, but nothing consumed */
    if (block_operation_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_MARK_CONSUMED_ZERO)
    {
        if (fbe_provision_drive_is_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_FLAG_SCRUB_NEEDED))
        {
            /* need to set the scrub needed bit as well */
            fbe_transport_set_completion_function(packet_p, fbe_provision_drive_zero_request_set_scrub_needed_bit, provision_drive_p);
        }
        else
        {
            /* need to clear the scrub intent bit, but not update zero checkpoint */
            fbe_transport_set_completion_function(packet_p, fbe_provision_drive_set_scrub_complete_bits, provision_drive_p);

            return FBE_STATUS_CONTINUE;
        }
    }
    /*find where the drive really starts*/
    fbe_base_config_get_default_offset((fbe_base_config_t *) provision_drive_p, &default_offset);
    fbe_base_object_get_object_id((fbe_base_object_t *)provision_drive_p, &pvd_object_id);

    /* calculate the chunk range from the lba and block count of the zero request. */
    fbe_provision_drive_utils_calculate_chunk_range_without_edges(start_lba,
                                                                  block_count,
                                                                  FBE_PROVISION_DRIVE_CHUNK_SIZE,
                                                                  &start_chunk_index,
                                                                  &chunk_count);


    /* get the corresponding lba for the start chunk index to update the checkpoint. */
    fbe_provision_drive_bitmap_get_lba_for_the_chunk_index(provision_drive_p,
                                                           start_chunk_index,
                                                           FBE_PROVISION_DRIVE_CHUNK_SIZE,
                                                           &new_zero_checkpoint);

    status = fbe_provision_drive_metadata_get_background_zero_checkpoint(provision_drive_p, &current_zero_checkpoint);
    if (status != FBE_STATUS_OK) {
        fbe_provision_drive_utils_trace( provision_drive_p,
                     FBE_TRACE_LEVEL_ERROR,
                     FBE_TRACE_MESSAGE_ID_INFO, 
                     FBE_PROVISION_DRIVE_DEBUG_FLAG_BGZ_TRACING,
					 "UZ:set_chkpt: Failed to get checkpoint. Status: 0x%X\n", status);

        /* we do not have zero checkpoint to complete so return the user zero request with good status. */
        fbe_payload_block_set_status(block_operation_p,
                                     FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                                     FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNEXPECTED_ERROR);
        
		fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);

        return FBE_STATUS_OK;
    }

    fbe_provision_drive_utils_trace(provision_drive_p,
                                    FBE_TRACE_LEVEL_INFO,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    FBE_PROVISION_DRIVE_DEBUG_FLAG_USER_ZERO_TRACING,
                                    "UZ:set_chkpt lba:0x%llx blk:0x%llx chk_count:0x%x nzc:0x%llx chkpt:0x%llx\n",
                                    (unsigned long long)start_lba, (unsigned long long)block_count, chunk_count, (unsigned long long)new_zero_checkpoint, (unsigned long long)current_zero_checkpoint);

    /*! We don't allow the zero checkpoint to be less than the default offset. */
    if (new_zero_checkpoint < default_offset) {
        /*! @todo Currently we don't allow the zero checkpoint less than
         *        default offset.  Currently we allow the system drives
         *        to go below the offset.
         */
        if (fbe_database_is_object_system_pvd(pvd_object_id))
        {
            fbe_provision_drive_utils_trace(provision_drive_p,
                                            FBE_TRACE_LEVEL_WARNING,
                                            FBE_TRACE_MESSAGE_ID_INFO,
                                            FBE_PROVISION_DRIVE_DEBUG_FLAG_USER_ZERO_TRACING,
                                            "UZ:set_chkpt New zero checkpoint: 0x%llx is less than default_offset: 0x%llx\n",
                                            (unsigned long long)new_zero_checkpoint, (unsigned long long)default_offset);
        } else {
            /* Else this is an unexpected request. */
            fbe_provision_drive_utils_trace(provision_drive_p,
                                            FBE_TRACE_LEVEL_ERROR,
                                            FBE_TRACE_MESSAGE_ID_INFO,
                                            FBE_PROVISION_DRIVE_DEBUG_FLAG_USER_ZERO_TRACING,
                                            "UZ:set_chkpt New zero checkpoint: 0x%llx is less than default_offset: 0x%llx\n",
                                            (unsigned long long)new_zero_checkpoint, (unsigned long long)default_offset);
            fbe_payload_block_set_status(block_operation_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                                         FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_CAPACITY_EXCEEDED);


			fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
            return FBE_STATUS_OK;
        }
    }

    /* set the completion function before we update the checkpoint. */
    fbe_transport_set_completion_function(packet_p,
                                          fbe_provision_drive_user_zero_update_checkpoint_completion,
                                          provision_drive_p);

    new_zero_checkpoint = FBE_MIN(new_zero_checkpoint, current_zero_checkpoint);


#if 0
    status = fbe_base_config_metadata_nonpaged_force_set_checkpoint((fbe_base_config_t *)provision_drive_p,
                                                                    packet_p,
                                                                    (fbe_u64_t)(&((fbe_provision_drive_nonpaged_metadata_t*)0)->zero_checkpoint),
                                                                    0, /* Second metadata offset is not used for pvd*/
                                                                    new_zero_checkpoint);
#endif

	fbe_provision_drive_metadata_set_background_zero_checkpoint(provision_drive_p, packet_p, new_zero_checkpoint);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * end fbe_provision_drive_zero_request_set_checkpoint()
 ******************************************************************************/

/*!****************************************************************************
 *  fbe_provision_drive_user_zero_update_checkpoint_completion()
 ******************************************************************************
 * @brief
 *  This function handles the completion of the checkpoint update for the user
 *  zero request.
 *
 * @param packet_p          - pointer to a control packet 
 * @param in_context        - context, which is a pointer to the provision drive
 *                            object
 *
 * @return  fbe_status_t  
 *
 * @author
 *   06/22/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_user_zero_update_checkpoint_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{                      
    fbe_provision_drive_t *             provision_drive_p = NULL;
    fbe_payload_ex_t *                 sep_payload_p = NULL;
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_payload_block_operation_t *     block_operation_p = NULL;
    
    provision_drive_p = (fbe_provision_drive_t *) context;

    /* get the payload and block operation. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p =  fbe_payload_ex_get_present_block_operation(sep_payload_p);
    
    /* get the status of the paged metadata operation. */
    status = fbe_transport_get_status_code(packet_p);

    if(status != FBE_STATUS_OK) {
        /*!@todo Add metadata error handling. */
        fbe_provision_drive_utils_trace(provision_drive_p,
                                        FBE_TRACE_LEVEL_WARNING,
                                        FBE_TRACE_MESSAGE_ID_INFO,
                                        FBE_PROVISION_DRIVE_DEBUG_FLAG_USER_ZERO_TRACING,
                                        "UZ:chkpt update failed, status:0x%x.\n", status);
        if (status == FBE_STATUS_NO_OBJECT) {
            fbe_payload_block_set_status(block_operation_p,
                                         FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED,
                                         FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE);
        } else {
            fbe_payload_block_set_status(block_operation_p,
                                         FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED,
                                         FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_POSSIBLE);
        }

        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    }

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_user_zero_update_checkpoint_completion()
 ******************************************************************************/

/*!****************************************************************************
 * @fn fbe_provision_drive_user_zero_update_paged_metadata_callback()
 ******************************************************************************
 * @brief
 *
 *  This is the callback function to clear bits for disk zeroing. 
 * 
 * @param packet            - Pointer to packet.
 * @param context           - Pointer to context.
 *
 * @return fbe_status_t.
 *  05/06/2014 - Created. Lili Chen
 *
 ******************************************************************************/
fbe_status_t 
fbe_provision_drive_user_zero_update_paged_metadata_callback(fbe_packet_t * packet, fbe_metadata_callback_context_t context)
{
#if 0
    fbe_payload_ex_t * sep_payload = fbe_transport_get_payload_ex(packet);
    fbe_payload_metadata_operation_t * mdo = NULL;
    fbe_provision_drive_paged_metadata_t * paged_metadata_p;
    fbe_sg_element_t * sg_list = NULL;
    fbe_sg_element_t * sg_ptr = NULL;
    fbe_lba_t lba_offset;
    fbe_u64_t slot_offset;
    fbe_bool_t is_write_requiered = FBE_FALSE;
    fbe_u8_t  *paged_set_bits;

    if(sep_payload->current_operation->payload_opcode == FBE_PAYLOAD_OPCODE_METADATA_OPERATION){
        mdo = fbe_payload_ex_get_metadata_operation(sep_payload);
    } else if(sep_payload->current_operation->payload_opcode == FBE_PAYLOAD_OPCODE_STRIPE_LOCK_OPERATION){
        mdo = fbe_payload_ex_get_any_metadata_operation(sep_payload);
    } else {
        fbe_topology_class_trace(FBE_CLASS_ID_PROVISION_DRIVE, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                      "%s: Invalid payload opcode:%d \n", __FUNCTION__, sep_payload->current_operation->payload_opcode);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    lba_offset = fbe_metadata_paged_get_lba_offset(mdo);
    slot_offset = fbe_metadata_paged_get_slot_offset(mdo);
    sg_list = mdo->u.metadata_callback.sg_list;
    sg_ptr = &sg_list[lba_offset - mdo->u.metadata_callback.start_lba];
    paged_metadata_p = (fbe_provision_drive_paged_metadata_t *)(sg_ptr->address + slot_offset);
    paged_set_bits = mdo->u.metadata_callback.record_data;

    while ((sg_ptr->address != NULL) && (mdo->u.metadata_callback.current_count_private < mdo->u.metadata_callback.repeat_count))
    {
        if ((((fbe_u8_t *)paged_metadata_p)[0] & paged_set_bits[0]) != paged_set_bits[0]) {
            is_write_requiered = FBE_TRUE;
            ((fbe_u8_t *)paged_metadata_p)[0] |= paged_set_bits[0];
        }

        mdo->u.metadata_callback.current_count_private++;
        paged_metadata_p++;
        if (paged_metadata_p == (fbe_provision_drive_paged_metadata_t *)(sg_ptr->address + FBE_METADATA_BLOCK_DATA_SIZE))
        {
            sg_ptr++;
            paged_metadata_p = (fbe_provision_drive_paged_metadata_t *)sg_ptr->address;
        }
    }

    if (is_write_requiered) {
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    } else {
        return FBE_STATUS_OK;
    }
#endif
    fbe_payload_ex_t * sep_payload = fbe_transport_get_payload_ex(packet);
    fbe_payload_metadata_operation_t * mdo = NULL;

    if(sep_payload->current_operation->payload_opcode == FBE_PAYLOAD_OPCODE_METADATA_OPERATION){
        mdo = fbe_payload_ex_get_metadata_operation(sep_payload);
    } else if(sep_payload->current_operation->payload_opcode == FBE_PAYLOAD_OPCODE_STRIPE_LOCK_OPERATION){
        mdo = fbe_payload_ex_get_any_metadata_operation(sep_payload);
    } else {
        fbe_topology_class_trace(FBE_CLASS_ID_PROVISION_DRIVE, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                      "%s: Invalid payload opcode:%d \n", __FUNCTION__, sep_payload->current_operation->payload_opcode);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return (fbe_provision_drive_metadata_paged_callback_set_bits(packet, (fbe_provision_drive_paged_metadata_t *)mdo->u.metadata_callback.record_data));
}
/******************************************************************************
 * end fbe_provision_drive_user_zero_update_paged_metadata_callback()
 ******************************************************************************/

/*!****************************************************************************
 * @fn fbe_provision_drive_user_zero_update_paged_metadata_write_verify_callback()
 ******************************************************************************
 * @brief
 *
 *  This is the callback function to clear bits for disk zeroing. 
 * 
 * @param packet            - Pointer to packet.
 * @param context           - Pointer to context.
 *
 * @return fbe_status_t.
 *  05/06/2014 - Created. Lili Chen
 *
 ******************************************************************************/
fbe_status_t 
fbe_provision_drive_user_zero_update_paged_metadata_write_verify_callback(fbe_packet_t * packet, fbe_metadata_callback_context_t context)
{
#if 0
    fbe_payload_ex_t * sep_payload = fbe_transport_get_payload_ex(packet);
    fbe_payload_metadata_operation_t * mdo = NULL;
    fbe_provision_drive_paged_metadata_t * paged_metadata_p;
    fbe_sg_element_t * sg_list = NULL;
    fbe_sg_element_t * sg_ptr = NULL;
    fbe_lba_t lba_offset;
    fbe_u64_t slot_offset;
    fbe_u8_t  *paged_set_bits;

    if(sep_payload->current_operation->payload_opcode == FBE_PAYLOAD_OPCODE_METADATA_OPERATION){
        mdo = fbe_payload_ex_get_metadata_operation(sep_payload);
    } else if(sep_payload->current_operation->payload_opcode == FBE_PAYLOAD_OPCODE_STRIPE_LOCK_OPERATION){
        mdo = fbe_payload_ex_get_any_metadata_operation(sep_payload);
    } else {
        fbe_topology_class_trace(FBE_CLASS_ID_PROVISION_DRIVE, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                      "%s: Invalid payload opcode:%d \n", __FUNCTION__, sep_payload->current_operation->payload_opcode);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    lba_offset = fbe_metadata_paged_get_lba_offset(mdo);
    slot_offset = fbe_metadata_paged_get_slot_offset(mdo);
    sg_list = mdo->u.metadata_callback.sg_list;
    sg_ptr = &sg_list[lba_offset - mdo->u.metadata_callback.start_lba];
    paged_metadata_p = (fbe_provision_drive_paged_metadata_t *)(sg_ptr->address + slot_offset);
    paged_set_bits = mdo->u.metadata_callback.record_data;

    while ((sg_ptr->address != NULL) && (mdo->u.metadata_callback.current_count_private < mdo->u.metadata_callback.repeat_count))
    {
        ((fbe_u8_t *)paged_metadata_p)[0] = paged_set_bits[0];

        mdo->u.metadata_callback.current_count_private++;
        paged_metadata_p++;
        if (paged_metadata_p == (fbe_provision_drive_paged_metadata_t *)(sg_ptr->address + FBE_METADATA_BLOCK_DATA_SIZE))
        {
            sg_ptr++;
            paged_metadata_p = (fbe_provision_drive_paged_metadata_t *)sg_ptr->address;
        }
    }

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
#endif

    fbe_payload_ex_t * sep_payload = fbe_transport_get_payload_ex(packet);
    fbe_payload_metadata_operation_t * mdo = NULL;

    if(sep_payload->current_operation->payload_opcode == FBE_PAYLOAD_OPCODE_METADATA_OPERATION){
        mdo = fbe_payload_ex_get_metadata_operation(sep_payload);
    } else if(sep_payload->current_operation->payload_opcode == FBE_PAYLOAD_OPCODE_STRIPE_LOCK_OPERATION){
        mdo = fbe_payload_ex_get_any_metadata_operation(sep_payload);
    } else {
        fbe_topology_class_trace(FBE_CLASS_ID_PROVISION_DRIVE, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                      "%s: Invalid payload opcode:%d \n", __FUNCTION__, sep_payload->current_operation->payload_opcode);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return (fbe_provision_drive_metadata_paged_callback_write(packet, (fbe_provision_drive_paged_metadata_t *)mdo->u.metadata_callback.record_data));
}
/******************************************************************************
 * end fbe_provision_drive_user_zero_update_paged_metadata_write_verify_callback()
 ******************************************************************************/

/*!****************************************************************************
 * @fn fbe_provision_drive_user_zero_update_paged_metadata()
 ******************************************************************************
 * @brief
 *  This function is used to start zeroing request by updating the paged 
 *  metadata for the chunks which correponds to zero request.
 *
 * @param provision_drive_p     - Pointer to the provision drive object.
 * @param packet_p              - Pointer to the packet.
 * @param b_is_paged_metadata_valid - FBE_TRUE - The contents of the paged
 *          metadata is valid, therefore we must XOR the existing value.
 *                              FBE_FALSE - The paged metadata has been lost.
 *          Therefore just write (write and verify so that a remap will occur
 *          if needed) the generated value.
 *
 * @return fbe_status_t         - status of the operation.
 *
 * @author
 *  21/06/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_user_zero_update_paged_metadata(fbe_provision_drive_t * provision_drive_p,
                                                    fbe_packet_t * packet_p,
                                                    fbe_packet_completion_function_t completion_function,
                                                    fbe_bool_t b_is_paged_metadata_valid)
{
    fbe_status_t                            status;
    fbe_payload_ex_t *                     sep_payload_p = NULL;
    fbe_payload_block_operation_t *         block_operation_p = NULL;
    fbe_payload_metadata_operation_t *      metadata_operation_p = NULL;
    fbe_lba_t                               start_lba;
    fbe_block_count_t                       block_count;
    fbe_chunk_index_t                       start_chunk_index;
    fbe_chunk_count_t                       chunk_count= 0;
    fbe_lba_t                               metadata_offset;
    fbe_provision_drive_paged_metadata_t    paged_set_bits;
    fbe_lba_t                               metadata_start_lba;
    fbe_block_count_t                       metadata_block_count;
    fbe_system_encryption_mode_t system_encryption_mode;
    fbe_payload_block_operation_opcode_t    block_operation_opcode = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID;
    fbe_bool_t                  b_is_system_drive = FBE_FALSE;
    fbe_lba_t                   default_offset = FBE_LBA_INVALID;    
    fbe_object_id_t             object_id;

   /* Get the sep payload and block operation. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_get_present_block_operation(sep_payload_p);

    fbe_payload_block_get_lba(block_operation_p, &start_lba);
    fbe_payload_block_get_block_count(block_operation_p, &block_count);
    fbe_payload_block_get_opcode(block_operation_p, &block_operation_opcode);

    /* Next task is to first determine the chunk range which requires to update the metadata */
    fbe_provision_drive_utils_calculate_chunk_range_without_edges(start_lba,
                                                                  block_count,
                                                                  FBE_PROVISION_DRIVE_CHUNK_SIZE,
                                                                  &start_chunk_index,
                                                                  &chunk_count);

    /* get the metadata offset for the chuk index. */
    metadata_offset = start_chunk_index * sizeof(fbe_provision_drive_paged_metadata_t);

    /*  initialize the paged set bits with user zero bit as true. */
    fbe_zero_memory(&paged_set_bits, sizeof(fbe_provision_drive_paged_metadata_t));

    /* It is possible that valid bits might be zero because of previoius read error,
     * since we are writing new values out, set the valid bit to 1
     */
    paged_set_bits.valid_bit = 1;

    if (block_operation_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ZERO) {
        paged_set_bits.user_zero_bit = FBE_TRUE;
        /* get pvd object id. */
        fbe_base_object_get_object_id((fbe_base_object_t *) provision_drive_p, &object_id);
        /* Check for system drive */
        b_is_system_drive = fbe_database_is_object_system_pvd(object_id);
        /* Get the default offset. */
            fbe_base_config_get_default_offset((fbe_base_config_t *) provision_drive_p, &default_offset);
        if (fbe_provision_drive_is_unmap_supported(provision_drive_p))
        {
            paged_set_bits.need_zero_bit = FBE_TRUE;
        }
        /* We need to do some special handling for System Drives on Encrypted system because the paged is 
         * NOT encrypted. So any time the user area wants to be zeroed, we need to force it irrespective of whether
         * it was zeroed or not because we need to now write encrypted Zeroes */
         fbe_database_get_system_encryption_mode(&system_encryption_mode);
         if(system_encryption_mode == FBE_SYSTEM_ENCRYPTION_MODE_ENCRYPTED)
         {
             /* get pvd object id. */
             fbe_base_object_get_object_id((fbe_base_object_t *) provision_drive_p, &object_id);
             /* Check for system drive */
             b_is_system_drive = fbe_database_is_object_system_pvd(object_id);
             /* Get the default offset. */
            fbe_base_config_get_default_offset((fbe_base_config_t *) provision_drive_p, &default_offset);
            if(b_is_system_drive && start_lba >= default_offset )
            {
                paged_set_bits.need_zero_bit = FBE_TRUE;
             }
         } 
        /* If the paged metadata has been lost we must set the consumed bit.*/
        if (b_is_paged_metadata_valid == FBE_FALSE) {
            paged_set_bits.consumed_user_data_bit = 1;
        }
        //paged_set_bits.need_zero_bit = FBE_TRUE;/*TODO: remove after DB will set it correctly*/
        fbe_provision_drive_utils_trace(provision_drive_p,
                                    FBE_TRACE_LEVEL_INFO,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    FBE_PROVISION_DRIVE_DEBUG_FLAG_NONE,
									"US: Z REQ: lba:0x%x, blks:0x%x, chnk_index:0x%x, chnk_cnt:0x%x,\n",
                                    (unsigned int)start_lba, (unsigned int)block_count, (unsigned int)start_chunk_index, chunk_count);

    }
    /* If it is disk zero operation, we need to set the NZ bits and then clear the 
       UZ and consumed bits (always clear the consumed since the data is gone). */
    else if(block_operation_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_MARK_DISK_ZERO) {
        paged_set_bits.need_zero_bit = FBE_TRUE;
        fbe_provision_drive_utils_trace(provision_drive_p,
                                    FBE_TRACE_LEVEL_INFO,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    FBE_PROVISION_DRIVE_DEBUG_FLAG_NONE,
                                    "pvd_uz_update_md DZ REQ: blk op:0x%x, lba:0x%llx, blks:0x%llx, chnk_index:0x%llx, chnk_cnt:0x%x,\n",
                                    block_operation_opcode, start_lba, block_count, start_chunk_index, chunk_count);
    }

    /* allocate metadata operation for SET paged bit data */
    metadata_operation_p =  fbe_payload_ex_allocate_metadata_operation(sep_payload_p); 

    /* Set completion functon to handle the write same edge request completion. */
    fbe_transport_set_completion_function(packet_p, completion_function, provision_drive_p);

    /* If the metadata is valid only set the bits indicated (leave other bits alone).
     */
    if (b_is_paged_metadata_valid) {
        if (block_operation_opcode != FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_MARK_CONSUMED_ZERO)
        {
#if 0
            /* Build the paged metadata set bit request. */
            fbe_payload_metadata_build_paged_set_bits(metadata_operation_p,
                                                     &(((fbe_base_config_t *) provision_drive_p)->metadata_element),
                                                     metadata_offset,
                                                     (fbe_u8_t *) &paged_set_bits,
                                                     sizeof(fbe_provision_drive_paged_metadata_t),
                                                     chunk_count);
#else
            fbe_payload_metadata_build_paged_update(metadata_operation_p,
                                              &(((fbe_base_config_t *) provision_drive_p)->metadata_element),
                                              metadata_offset,
                                              sizeof(fbe_provision_drive_paged_metadata_t),
                                              chunk_count,
                                              fbe_provision_drive_user_zero_update_paged_metadata_callback,
                                              (void *)metadata_operation_p);
            /* Initialize cient_blob, skip the subpacket */
            fbe_provision_drive_metadata_paged_init_client_blob(packet_p, metadata_operation_p);
            fbe_copy_memory(metadata_operation_p->u.metadata_callback.record_data, (fbe_u8_t *)&paged_set_bits, sizeof(fbe_provision_drive_paged_metadata_t));
#endif
        }
        else
        {
            fbe_provision_drive_utils_trace(provision_drive_p,
                                        FBE_TRACE_LEVEL_INFO,
                                        FBE_TRACE_MESSAGE_ID_INFO,
                                        FBE_PROVISION_DRIVE_DEBUG_FLAG_NONE,
                                        "pvd_uz_update_md zero consumed REQ: blk op:0x%x, lba:0x%llx, blks:0x%llx, chnk_index:0x%llx, chnk_cnt:0x%x,\n",
                                        block_operation_opcode, start_lba, block_count, start_chunk_index, chunk_count);

            /* Build the paged metadata update request. */
            fbe_payload_metadata_build_paged_update(metadata_operation_p,
                                                    &(((fbe_base_config_t *) provision_drive_p)->metadata_element),
                                                    metadata_offset,
                                                    sizeof(fbe_provision_drive_paged_metadata_t),
                                                    chunk_count,
                                                    fbe_provision_drive_consumed_area_zero_update_paged_metadata_completion,
                                                    (void *)provision_drive_p);

            /* Initialize cient_blob */
            fbe_provision_drive_metadata_paged_init_client_blob(packet_p, metadata_operation_p);
        }
    } else {
        /* Else the paged metadata is lost. Build the paged metadata write 
         * (and verify) request.
         */
#if 0
        fbe_payload_metadata_build_paged_write_verify(metadata_operation_p,
                                                     &(((fbe_base_config_t *) provision_drive_p)->metadata_element),
                                                     metadata_offset,
                                                     (fbe_u8_t *) &paged_set_bits,
                                                     sizeof(fbe_provision_drive_paged_metadata_t),
                                                     chunk_count);
#else
        fbe_payload_metadata_build_paged_update(metadata_operation_p,
                                              &(((fbe_base_config_t *) provision_drive_p)->metadata_element),
                                              metadata_offset,
                                              sizeof(fbe_provision_drive_paged_metadata_t),
                                              chunk_count,
                                              fbe_provision_drive_user_zero_update_paged_metadata_write_verify_callback,
                                              (void *)metadata_operation_p);
        fbe_payload_metadata_set_metadata_operation_flags(metadata_operation_p, FBE_PAYLOAD_METADATA_OPERATION_PAGED_WRITE_VERIFY);
        /* Initialize cient_blob, skip the subpacket */
        fbe_provision_drive_metadata_paged_init_client_blob(packet_p, metadata_operation_p);
        fbe_copy_memory(metadata_operation_p->u.metadata_callback.record_data, (fbe_u8_t *)&paged_set_bits, sizeof(fbe_provision_drive_paged_metadata_t));
#endif
    }

        
    /* Get the metadata stripe lock*/
    fbe_metadata_paged_get_lock_range(metadata_operation_p, &metadata_start_lba, &metadata_block_count);
    fbe_payload_metadata_set_metadata_stripe_offset(metadata_operation_p, metadata_start_lba);
    fbe_payload_metadata_set_metadata_stripe_count(metadata_operation_p, metadata_block_count);

    fbe_payload_ex_increment_metadata_operation_level(sep_payload_p);
    status =  fbe_metadata_operation_entry(packet_p);
    return status;
}

/******************************************************************************
 * end fbe_provision_drive_user_zero_update_paged_metadata()
 ******************************************************************************/

/*!****************************************************************************
 *  fbe_provision_drive_user_zero_update_paged_metadata_completion()
 ******************************************************************************
 * @brief
 *   This function handles the completion of paged metadata update request. It 
 *   validates the status and update the non paged metadata on success.
 *
 * @param packet_p          - pointer to a control packet 
 * @param in_context        - context, which is a pointer to the provision drive
 *                            object
 *
 *
 * @return  fbe_status_t  
 *
 * @author
 *   06/22/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_user_zero_update_paged_metadata_completion(fbe_packet_t * packet_p,
                                                               fbe_packet_completion_context_t context)
{
    fbe_provision_drive_t *                 provision_drive_p = NULL;
    fbe_payload_metadata_operation_t *      metadata_operation_p = NULL;
    fbe_payload_block_operation_t *         block_operation_p = NULL;
    fbe_payload_ex_t *                      sep_payload_p = NULL;
    fbe_payload_block_operation_opcode_t    block_opcode = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID;
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_payload_metadata_status_t           metadata_status;
    fbe_payload_metadata_operation_opcode_t metadata_opcode;
    fbe_lba_t                               metadata_offset;
    
    provision_drive_p = (fbe_provision_drive_t *) context;

    /* Get the payload and control operation. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    metadata_operation_p =  fbe_payload_ex_get_metadata_operation(sep_payload_p);
    
    /* Get the status of the paged metadata operation. */
    fbe_payload_metadata_get_status(metadata_operation_p, &metadata_status);
    status = fbe_transport_get_status_code(packet_p);

    /* Get the metadata offset */
    fbe_payload_metadata_get_metadata_offset(metadata_operation_p, &metadata_offset);

    fbe_payload_metadata_get_opcode(metadata_operation_p, &metadata_opcode);

    /* Get the block operation. */
    block_operation_p = fbe_payload_ex_get_present_block_operation(sep_payload_p);
    fbe_payload_block_get_opcode(block_operation_p, &block_opcode);

    if((status != FBE_STATUS_OK) || (metadata_status != FBE_PAYLOAD_METADATA_STATUS_OK))
    {
        /* Release the metadata operation. */
        fbe_payload_ex_release_metadata_operation(sep_payload_p, metadata_operation_p);
        fbe_provision_drive_utils_trace(provision_drive_p,
                                        FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                        FBE_PROVISION_DRIVE_DEBUG_FLAG_NONE,
                                        "UZ:paged update failed, offset:0x%x, m_status:0x%x, status:0x%x.\n",
                                        (unsigned int)metadata_offset, metadata_status, status);

        if(metadata_status == FBE_PAYLOAD_METADATA_STATUS_IO_UNCORRECTABLE) {
            /* There was an error in writing to the paged region because preread could have failed
             * try to perform a write verify operation
             */
            status = fbe_provision_drive_user_zero_update_paged_metadata(provision_drive_p, packet_p,
                                                                         fbe_provision_drive_user_zero_write_verify_update_paged_metadata_completion,
                                                                         FBE_FALSE /* The paged metadata is not valid */);

            /* We might setting other region in the MD as invalid and so kick off a verify invalidate to fix those region */
            fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const, 
									(fbe_base_object_t *) provision_drive_p,
                                     FBE_PROVISION_DRIVE_LIFECYCLE_COND_METADATA_VERIFY_INVALIDATE);

            return FBE_STATUS_MORE_PROCESSING_REQUIRED;
        } else {
            fbe_payload_block_set_status(block_operation_p,
                                         FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED,
                                         FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_POSSIBLE);
       
            fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
            return status;
        }
    }

    fbe_provision_drive_user_zero_handle_paged_metadata_update_completion(packet_p, provision_drive_p, block_operation_p,
                                                                          metadata_operation_p, sep_payload_p);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}

/*!****************************************************************************
 *  fbe_provision_drive_user_zero_handle_paged_metadata_update_completion()
 ******************************************************************************
 * @brief
 *   This function clears all the paged metadata bits except NZ bits for disk zeroing.
 *   For disk zeroing we just have to set the NZ bit and clear all other bits
 *
 * @param packet_p          - pointer to a control packet 
 * @param in_context        - context, which is a pointer to the provision drive
 *                            object
 *
 *
 * @return  fbe_status_t  
 *
 * @author
 *   01/20/2012 - Created. Ashwin Tamilarasan
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_user_zero_handle_paged_metadata_update_completion(fbe_packet_t * packet_p,
                                                                      fbe_provision_drive_t* provision_drive_p,
                                                                      fbe_payload_block_operation_t *  block_operation_p,
                                                                      fbe_payload_metadata_operation_t *      metadata_operation_p,
                                                                      fbe_payload_ex_t *        sep_payload_p)
{
    fbe_payload_block_operation_opcode_t    block_opcode = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID;
    fbe_lba_t                               metadata_offset;
    fbe_status_t                            status;

        /* Get the metadata offset */
    fbe_payload_metadata_get_metadata_offset(metadata_operation_p, &metadata_offset);

    fbe_payload_block_get_opcode(block_operation_p, &block_opcode);

    /* If it is a disk zero request clear all the bits except NZ */
    if(block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_MARK_DISK_ZERO) {
        status = fbe_provision_drive_disk_zero_clear_bits(packet_p, provision_drive_p, block_operation_p,
                                                          metadata_operation_p, sep_payload_p,
                                                          fbe_provision_drive_disk_zero_update_paged_metadata_completion,
                                                          FBE_TRUE /* The paged metadata is valid */);
        return status;
    
    }
    /* Release the metadata operation. */
    fbe_payload_ex_release_metadata_operation(sep_payload_p, metadata_operation_p);
    fbe_provision_drive_utils_trace(provision_drive_p, FBE_TRACE_LEVEL_DEBUG_LOW, FBE_TRACE_MESSAGE_ID_INFO,
                                    FBE_PROVISION_DRIVE_DEBUG_FLAG_USER_ZERO_TRACING,
                                    "UZ:paged update complete, offset:0x%llx lba:0x%llx bl:0x%llx\n",
                                    (unsigned long long)metadata_offset,
				    (unsigned long long)block_operation_p->lba,
				    (unsigned long long)block_operation_p->block_count);

    /* after paged metadata update send the write same request for the region which is not
     * covered part of the chunk range and requireds explicit zeroing using write same
     * request.
     */
    status = fbe_provision_drive_user_zero_issue_write_same_if_needed(provision_drive_p, packet_p);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * end fbe_provision_drive_user_zero_handle_paged_metadata_update_completion()
 ******************************************************************************/

/*!****************************************************************************
 *  fbe_provision_drive_user_zero_write_verify_update_paged_metadata_completion()
 ******************************************************************************
 * @brief
 *   This function handles the completion of paged metadata update request. It 
 *   validates the status and update the non paged metadata on success.
 *
 * @param packet_p          - pointer to a control packet 
 * @param in_context        - context, which is a pointer to the provision drive
 *                            object
 *
 *
 * @return  fbe_status_t  
 *
 * @author
 *   06/22/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_user_zero_write_verify_update_paged_metadata_completion(fbe_packet_t * packet_p,
                                                                            fbe_packet_completion_context_t context)
{
    fbe_provision_drive_t *                 provision_drive_p = NULL;
    fbe_payload_metadata_operation_t *      metadata_operation_p = NULL;
    fbe_payload_block_operation_t *         block_operation_p = NULL;
    fbe_payload_ex_t *                      sep_payload_p = NULL;
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_payload_metadata_status_t           metadata_status;
    fbe_payload_metadata_operation_opcode_t metadata_opcode;
    fbe_lba_t                               metadata_offset;
    
    provision_drive_p = (fbe_provision_drive_t *) context;

    /* Get the payload and control operation. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    metadata_operation_p =  fbe_payload_ex_get_metadata_operation(sep_payload_p);
    
    /* Get the status of the paged metadata operation. */
    fbe_payload_metadata_get_status(metadata_operation_p, &metadata_status);
    status = fbe_transport_get_status_code(packet_p);

    /* Get the metadata offset */
    fbe_payload_metadata_get_metadata_offset(metadata_operation_p, &metadata_offset);

    fbe_payload_metadata_get_opcode(metadata_operation_p, &metadata_opcode);

    /* Get the block operation. */
    block_operation_p = fbe_payload_ex_get_present_block_operation(sep_payload_p);

    if((status != FBE_STATUS_OK) || (metadata_status != FBE_PAYLOAD_METADATA_STATUS_OK)) {
        /* Release the metadata operation. */
        fbe_payload_ex_release_metadata_operation(sep_payload_p, metadata_operation_p);
        fbe_provision_drive_utils_trace(provision_drive_p,
                                        FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                        FBE_PROVISION_DRIVE_DEBUG_FLAG_NONE,
                                        "UZ:paged update failed, offset:0x%llx, m_status:0x%x, status:0x%x.\n",
                                        metadata_offset, metadata_status, status);

        fbe_payload_block_set_status(block_operation_p,
                                     FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED,
                                     FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_POSSIBLE);
       
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        return status;
    }

    fbe_provision_drive_user_zero_handle_paged_metadata_update_completion(packet_p, provision_drive_p, block_operation_p,
                                                                          metadata_operation_p, sep_payload_p);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * end fbe_provision_drive_user_zero_write_verify_update_paged_metadata_completion()
 ******************************************************************************/


/*!****************************************************************************
 * @fn fbe_provision_drive_disk_zero_clear_bits_callback()
 ******************************************************************************
 * @brief
 *
 *  This is the callback function to clear bits for disk zeroing. 
 * 
 * @param packet            - Pointer to packet.
 * @param context           - Pointer to context.
 *
 * @return fbe_status_t.
 *  05/06/2014 - Created. Lili Chen
 *
 ******************************************************************************/
fbe_status_t 
fbe_provision_drive_disk_zero_clear_bits_callback(fbe_packet_t * packet, fbe_metadata_callback_context_t context)
{
#if 0
    fbe_payload_ex_t * sep_payload = fbe_transport_get_payload_ex(packet);
    fbe_payload_metadata_operation_t * mdo = NULL;
    fbe_provision_drive_paged_metadata_t * paged_metadata_p;
    fbe_sg_element_t * sg_list = NULL;
    fbe_sg_element_t * sg_ptr = NULL;
    fbe_lba_t lba_offset;
    fbe_u64_t slot_offset;
    fbe_bool_t is_write_requiered = FBE_FALSE;

    if(sep_payload->current_operation->payload_opcode == FBE_PAYLOAD_OPCODE_METADATA_OPERATION){
        mdo = fbe_payload_ex_get_metadata_operation(sep_payload);
    } else if(sep_payload->current_operation->payload_opcode == FBE_PAYLOAD_OPCODE_STRIPE_LOCK_OPERATION){
        mdo = fbe_payload_ex_get_any_metadata_operation(sep_payload);
    } else {
        fbe_topology_class_trace(FBE_CLASS_ID_PROVISION_DRIVE, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                      "%s: Invalid payload opcode:%d \n", __FUNCTION__, sep_payload->current_operation->payload_opcode);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    lba_offset = fbe_metadata_paged_get_lba_offset(mdo);
    slot_offset = fbe_metadata_paged_get_slot_offset(mdo);
    sg_list = mdo->u.metadata_callback.sg_list;
    sg_ptr = &sg_list[lba_offset - mdo->u.metadata_callback.start_lba];
    paged_metadata_p = (fbe_provision_drive_paged_metadata_t *)(sg_ptr->address + slot_offset);

    while ((sg_ptr->address != NULL) && (mdo->u.metadata_callback.current_count_private < mdo->u.metadata_callback.repeat_count))
    {
        if (paged_metadata_p->consumed_user_data_bit || paged_metadata_p->user_zero_bit) {
            is_write_requiered = FBE_TRUE;
            /* clear the consumed bit if it is set. */
            paged_metadata_p->consumed_user_data_bit = 0;
            /* clear the user zero bit if it is set. */
            paged_metadata_p->user_zero_bit = 0;
        }

        mdo->u.metadata_callback.current_count_private++;
        paged_metadata_p++;
        if (paged_metadata_p == (fbe_provision_drive_paged_metadata_t *)(sg_ptr->address + FBE_METADATA_BLOCK_DATA_SIZE))
        {
            sg_ptr++;
            paged_metadata_p = (fbe_provision_drive_paged_metadata_t *)sg_ptr->address;
        }
    }

    if (is_write_requiered) {
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    } else {
        return FBE_STATUS_OK;
    }
#endif

    fbe_provision_drive_paged_metadata_t paged_metadata;

    /* Clear consumed bit */
    fbe_zero_memory(&paged_metadata, sizeof(fbe_provision_drive_paged_metadata_t));
    paged_metadata.consumed_user_data_bit = 1;
    paged_metadata.user_zero_bit = 1;

    return (fbe_provision_drive_metadata_paged_callback_clear_bits(packet, &paged_metadata));

}
/******************************************************************************
 * end fbe_provision_drive_disk_zero_clear_bits_callback()
 ******************************************************************************/

/*!****************************************************************************
 *  fbe_provision_drive_disk_zero_clear_bits()
 ******************************************************************************
 * @brief
 *   This function clears all the paged metadata bits except NZ bits for disk zeroing.
 *   For disk zeroing we just have to set the NZ bit and clear all other bits
 *
 * @param packet_p          - pointer to a control packet 
 * @param provision_drive_p - pointer to provision drive object
 * @param block_operation_p - Block operation
 * @param metadata_operation_p - Metadata operation
 * @param sep_payload_p - sep payload
 * @param completion_function 
 * @param b_is_paged_metadata_valid - FBE_TRUE - The contents of the paged
 *          metadata is valid, therefore we must XOR the existing value.
 *                              FBE_FALSE - The paged metadata has been lost.
 *          Therefore just write (write and verify so that a remap will occur
 *          if needed) the generated value.
 *
 *
 * @return  fbe_status_t  
 *
 * @author
 *   01/20/2012 - Created. Ashwin Tamilarasan
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_disk_zero_clear_bits(fbe_packet_t * packet_p,
                                         fbe_provision_drive_t* provision_drive_p,
                                         fbe_payload_block_operation_t *  block_operation_p,
                                         fbe_payload_metadata_operation_t * metadata_operation_p,
                                         fbe_payload_ex_t * sep_payload_p,
                                         fbe_packet_completion_function_t completion_function,
                                         fbe_bool_t b_is_paged_metadata_valid)
{

    fbe_block_count_t                       block_count;
    fbe_lba_t                               start_lba;
    fbe_chunk_index_t                       start_chunk_index;
    fbe_chunk_count_t                       chunk_count= 0;
    fbe_lba_t                               metadata_offset;
    fbe_provision_drive_paged_metadata_t    paged_clear_bits;
    fbe_lba_t                               metadata_start_lba;
    fbe_block_count_t                       metadata_block_count;
    fbe_status_t                            status;

    fbe_payload_block_get_lba(block_operation_p, &start_lba);
    fbe_payload_block_get_block_count(block_operation_p, &block_count);

    /* Next task is to first determine the chunk range which requires to update the metadata */
    fbe_provision_drive_utils_calculate_chunk_range_without_edges(start_lba,
                                                                  block_count,
                                                                  FBE_PROVISION_DRIVE_CHUNK_SIZE,
                                                                  &start_chunk_index,
                                                                  &chunk_count);

    /* get the metadata offset for the chuk index. */
    metadata_offset = start_chunk_index * sizeof(fbe_provision_drive_paged_metadata_t);

    /*  initialize the paged clear bits with user zero bit and consumed bit as true. */
    fbe_zero_memory(&paged_clear_bits, sizeof(fbe_provision_drive_paged_metadata_t));

    /* Set completion functon to handle the write same edge request completion. */
    fbe_transport_set_completion_function(packet_p,
                                          completion_function,
                                          provision_drive_p);

   if(b_is_paged_metadata_valid)
   {
#if 0
       paged_clear_bits.consumed_user_data_bit = FBE_TRUE;
       paged_clear_bits.user_zero_bit = FBE_TRUE;
       /* Build the paged metadata set bit request. */
       fbe_payload_metadata_build_paged_clear_bits(metadata_operation_p,
                                                 &(((fbe_base_config_t *) provision_drive_p)->metadata_element),
                                                  metadata_offset,
                                                 (fbe_u8_t *) &paged_clear_bits,
                                                 sizeof(fbe_provision_drive_paged_metadata_t),
                                                 chunk_count);
#else
        fbe_payload_metadata_build_paged_update(metadata_operation_p,
                                              &(((fbe_base_config_t *) provision_drive_p)->metadata_element),
                                              metadata_offset,
                                              sizeof(fbe_provision_drive_paged_metadata_t),
                                              chunk_count,
                                              fbe_provision_drive_disk_zero_clear_bits_callback,
                                              (void *)metadata_operation_p);
        /* Initialize cient_blob, skip the subpacket */
        fbe_provision_drive_metadata_paged_init_client_blob(packet_p, metadata_operation_p);
#endif
   }
   else
   {
       /* Since the previous write failed, we need to init this chunk of paged metadata with
        * what we want (clear consumed since the data is gone).
        */
       paged_clear_bits.valid_bit = FBE_TRUE;
       paged_clear_bits.need_zero_bit = FBE_TRUE;
#if 0
       /* Build the paged metadata set bit request. */
       fbe_payload_metadata_build_paged_write_verify(metadata_operation_p,
                                                     &(((fbe_base_config_t *) provision_drive_p)->metadata_element),
                                                     metadata_offset,
                                                     (fbe_u8_t *) &paged_clear_bits,
                                                     sizeof(fbe_provision_drive_paged_metadata_t),
                                                     chunk_count);
#else
        fbe_payload_metadata_build_paged_update(metadata_operation_p,
                                              &(((fbe_base_config_t *) provision_drive_p)->metadata_element),
                                              metadata_offset,
                                              sizeof(fbe_provision_drive_paged_metadata_t),
                                              chunk_count,
                                              fbe_provision_drive_user_zero_update_paged_metadata_write_verify_callback,
                                              (void *)metadata_operation_p);
        fbe_payload_metadata_set_metadata_operation_flags(metadata_operation_p, FBE_PAYLOAD_METADATA_OPERATION_PAGED_WRITE_VERIFY);
        /* Initialize cient_blob, skip the subpacket */
        fbe_provision_drive_metadata_paged_init_client_blob(packet_p, metadata_operation_p);
        fbe_copy_memory(metadata_operation_p->u.metadata_callback.record_data, (fbe_u8_t *)&paged_clear_bits, sizeof(fbe_provision_drive_paged_metadata_t));
#endif

   }
   /* Get the metadata stripe lock*/
   fbe_metadata_paged_get_lock_range(metadata_operation_p, &metadata_start_lba, &metadata_block_count);

   fbe_payload_metadata_set_metadata_stripe_offset(metadata_operation_p, metadata_start_lba);
   fbe_payload_metadata_set_metadata_stripe_count(metadata_operation_p, metadata_block_count);

    fbe_payload_ex_increment_metadata_operation_level(sep_payload_p);
    status =  fbe_metadata_operation_entry(packet_p);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;

}
/******************************************************************************
 * end fbe_provision_drive_disk_zero_clear_bits()
 ******************************************************************************/

/*!****************************************************************************
 *  fbe_provision_drive_disk_zero_update_paged_metadata_completion()
 ******************************************************************************
 * @brief
 *   This function handles the completion of disk zero paged metadata update request. It 
 *   validates the status.
 *
 * @param packet_p          - pointer to a control packet 
 * @param in_context        - context, which is a pointer to the provision drive
 *                            object
 *
 *
 * @return  fbe_status_t  
 *
 * @author
 *   01/12/2012 - Created. Ashwin Tamilarasan
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_disk_zero_update_paged_metadata_completion(fbe_packet_t * packet_p,
                                                               fbe_packet_completion_context_t context)
{
    fbe_provision_drive_t *                 provision_drive_p = NULL;
    fbe_payload_metadata_operation_t *      metadata_operation_p = NULL;
    fbe_payload_block_operation_t *         block_operation_p = NULL;
    fbe_payload_ex_t *                      sep_payload_p = NULL;
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_payload_metadata_status_t           metadata_status;
    fbe_payload_metadata_operation_opcode_t metadata_opcode;
    fbe_lba_t                               metadata_offset;
    
    provision_drive_p = (fbe_provision_drive_t *) context;

    /* Get the payload and control operation. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    metadata_operation_p =  fbe_payload_ex_get_metadata_operation(sep_payload_p);
    
    /* Get the status of the paged metadata operation. */
    fbe_payload_metadata_get_status(metadata_operation_p, &metadata_status);
    status = fbe_transport_get_status_code(packet_p);

    /* Get the metadata offset */
    fbe_payload_metadata_get_metadata_offset(metadata_operation_p, &metadata_offset);

    fbe_payload_metadata_get_opcode(metadata_operation_p, &metadata_opcode);

    /* Get the block operation. */
    block_operation_p = fbe_payload_ex_get_present_block_operation(sep_payload_p);

    if((status != FBE_STATUS_OK) ||
       (metadata_status != FBE_PAYLOAD_METADATA_STATUS_OK))
    {
        fbe_provision_drive_utils_trace(provision_drive_p,
                                        FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                        FBE_PROVISION_DRIVE_DEBUG_FLAG_NONE,
                                        "DZ:paged update failed, offset:0x%llx, m_status:0x%x, status:0x%x.\n",
                                        metadata_offset, metadata_status, status);
        if(metadata_status == FBE_PAYLOAD_METADATA_STATUS_IO_UNCORRECTABLE)
        {
            /* There was an error in writing to the paged region because preread could have failed
             * try to perform a write verify operation
             */
            fbe_provision_drive_disk_zero_clear_bits(packet_p, provision_drive_p, block_operation_p,
                                                     metadata_operation_p, sep_payload_p,
                                                     fbe_provision_drive_disk_zero_write_verify_update_paged_metadata_completion,
                                                     FBE_FALSE /* The paged metadata has been lost*/);
            /* We might setting other region in the MD as invalid and so kick off a verify invalidate to fix those region
             */
            fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const, (fbe_base_object_t *) provision_drive_p,
                                   FBE_PROVISION_DRIVE_LIFECYCLE_COND_METADATA_VERIFY_INVALIDATE);

            return FBE_STATUS_MORE_PROCESSING_REQUIRED;
        }
        else
        {
    
            /* Release the metadata operation. */
            fbe_payload_ex_release_metadata_operation(sep_payload_p, metadata_operation_p);
    
            fbe_payload_block_set_status(block_operation_p,
                                         FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED,
                                         FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_POSSIBLE);
       
            fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
            return status;
        }
    }

    /* Release the metadata operation. */
    fbe_payload_ex_release_metadata_operation(sep_payload_p, metadata_operation_p);

    fbe_provision_drive_utils_trace(provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                    FBE_PROVISION_DRIVE_DEBUG_FLAG_NONE,
                                    "DZ:paged update complete, offset:0x%llx lba:0x%llx bl:0x%llx\n",
                                    (unsigned long long)metadata_offset,
				    (unsigned long long)block_operation_p->lba,
				    (unsigned long long)block_operation_p->block_count);


    /* Set the block operation status to good */
    fbe_payload_block_set_status(block_operation_p,
                                 FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS,
                                 FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);
    
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_disk_zero_update_paged_metadata_completion()
 ******************************************************************************/

/*!****************************************************************************
 *  fbe_provision_drive_disk_zero_write_verify_update_paged_metadata_completion()
 ******************************************************************************
 * @brief
 *   This function handles the completion of disk zero paged metadata update request. It 
 *   validates the status.
 *
 * @param packet_p          - pointer to a control packet 
 * @param in_context        - context, which is a pointer to the provision drive
 *                            object
 *
 *
 * @return  fbe_status_t  
 *
 * @author
 *   01/12/2012 - Created. Ashwin Tamilarasan
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_disk_zero_write_verify_update_paged_metadata_completion(fbe_packet_t * packet_p,
                                                                            fbe_packet_completion_context_t context)
{
    fbe_provision_drive_t *                 provision_drive_p = NULL;
    fbe_payload_metadata_operation_t *      metadata_operation_p = NULL;
    fbe_payload_block_operation_t *         block_operation_p = NULL;
    fbe_payload_ex_t *                      sep_payload_p = NULL;
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_payload_metadata_status_t           metadata_status;
    fbe_payload_metadata_operation_opcode_t metadata_opcode;
    fbe_lba_t                               metadata_offset;
    
    provision_drive_p = (fbe_provision_drive_t *) context;

    /* Get the payload and control operation. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    metadata_operation_p =  fbe_payload_ex_get_metadata_operation(sep_payload_p);
    
    /* Get the status of the paged metadata operation. */
    fbe_payload_metadata_get_status(metadata_operation_p, &metadata_status);
    status = fbe_transport_get_status_code(packet_p);

    /* Get the metadata offset */
    fbe_payload_metadata_get_metadata_offset(metadata_operation_p, &metadata_offset);

    fbe_payload_metadata_get_opcode(metadata_operation_p, &metadata_opcode);

    /* Get the block operation. */
    block_operation_p = fbe_payload_ex_get_present_block_operation(sep_payload_p);
    
    /* Release the metadata operation. */
    fbe_payload_ex_release_metadata_operation(sep_payload_p, metadata_operation_p);

    if((status != FBE_STATUS_OK) ||
       (metadata_status != FBE_PAYLOAD_METADATA_STATUS_OK))
    {
        fbe_provision_drive_utils_trace(provision_drive_p,
                                        FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                        FBE_PROVISION_DRIVE_DEBUG_FLAG_NONE,
                                        "DZ:write verify paged update failed, offset:0x%llx, m_status:0x%x, status:0x%x.\n",
                                        metadata_offset, metadata_status, status);
    
        fbe_payload_block_set_status(block_operation_p,
                                     FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED,
                                     FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_POSSIBLE);
       
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        return status;        
    }

    fbe_provision_drive_utils_trace(provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                    FBE_PROVISION_DRIVE_DEBUG_FLAG_NONE,
                                    "DZ:write verify paged update complete, offset:0x%llx lba:0x%llx bl:0x%llx\n",
                                    metadata_offset, block_operation_p->lba, block_operation_p->block_count);


    /* Set the block operation status to good */
    fbe_payload_block_set_status(block_operation_p,
                                 FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS,
                                 FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);
    
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_disk_zero_write_verify_update_paged_metadata_completion()
 ******************************************************************************/


/*!****************************************************************************
 * @fn fbe_provision_drive_user_zero_issue_write_same_if_needed()
 ******************************************************************************
 * @brief
 *  This function is used to allocate the subpackets if it finds the the user
 *  zero request has edges (with respect to chunk) then it allocates subpacket
 *  to process either of the preedge or postedge or both.
 *
 * @param provision_drive_p     - Pointer to the provision drive object.
 * @param packet_p              - Pointer to the packet.
 *
 * @return fbe_status_t         - status of the operation.
 *
 * @author
 *  21/06/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_user_zero_issue_write_same_if_needed(fbe_provision_drive_t * provision_drive_p,
                                                         fbe_packet_t * packet_p)
{
    fbe_payload_ex_t *                     sep_payload_p = NULL;
    fbe_payload_block_operation_t *         block_operation_p = NULL;
    fbe_lba_t                               start_lba;
    fbe_block_count_t                       block_count;
    fbe_lba_t                               pre_edge_start_lba = FBE_LBA_INVALID;
    fbe_lba_t                               post_edge_start_lba = FBE_LBA_INVALID;
    fbe_block_count_t                       pre_edge_block_count = 0;
    fbe_block_count_t                       post_edge_block_count = 0;
    fbe_u32_t                               number_of_edges = 0;

   /* Get the sep payload and block operation. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_get_present_block_operation(sep_payload_p);

    fbe_payload_block_get_lba(block_operation_p, &start_lba);
    fbe_payload_block_get_block_count(block_operation_p, &block_count);

    /* calculate the boundary range which does not cover the chunk. */
    fbe_provision_drive_utils_calculate_unaligned_edges_lba_range(start_lba,
                                                                  block_count,
                                                                  FBE_PROVISION_DRIVE_CHUNK_SIZE,
                                                                  &pre_edge_start_lba,
                                                                  &pre_edge_block_count,
                                                                  &post_edge_start_lba,
                                                                  &post_edge_block_count,
                                                                  &number_of_edges); /* either one or two */

    if(number_of_edges == 0)
    {
        fbe_provision_drive_utils_trace(provision_drive_p, 
                                        FBE_TRACE_LEVEL_DEBUG_LOW, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                        FBE_PROVISION_DRIVE_DEBUG_FLAG_USER_ZERO_TRACING,
                                        "UZ: no zero needed (complete), lba:0x%llx, blks:0x%llx, packet:%p\n",
                                        (unsigned long long)start_lba,
					(unsigned long long)block_count,
					packet_p);
        
        /* we do not have any edge to process so complete the user zero request with good status. */
        fbe_payload_block_set_status(block_operation_p,
                                     FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS,
                                     FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);
        fbe_provision_drive_utils_complete_packet(packet_p, FBE_STATUS_OK, 0);
        return FBE_STATUS_OK;
    }
    fbe_provision_drive_user_zero_send_subpackets(provision_drive_p, packet_p);
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_user_zero_issue_write_same_if_needed()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_user_zero_send_subpackets()
 ******************************************************************************
 * @brief
 *  This function handles the allocation of the 
 *  subpacket created for the zeroing request, it creates required block 
 *  operation for each subpacket and sending the subpacket to issue the write same
 *  operation.
 *
 * @note
 *  In hte future we could enhance this code to make sure we do not exceed the maximum backend
 *  transfer limit, currently chunk size is same as maximum backend transfer
 *  size and so unaligned edge with respect to chunk size will not be greater
 *  then maximum backend transfer size and also RAID object never sends
 *  request which is greater then maximum transfer size.
 *
 * @param memory_request    - Pointer to memory request.
 * @param context           - Pointer to the provision drive object.
 *
 * @return fbe_status_t.
 *  21/06/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_user_zero_send_subpackets(fbe_provision_drive_t * provision_drive_p,
                                              fbe_packet_t * packet_p)
{
    fbe_payload_ex_t *                     sep_payload_p = NULL;
    fbe_payload_ex_t *                         new_payload_p = NULL;
    fbe_payload_block_operation_t *         block_operation_p = NULL;
    fbe_payload_block_operation_t *         new_block_operation_p = NULL;
    fbe_memory_request_t *memory_request_p = NULL;
    fbe_status_t                            status;
    fbe_optimum_block_size_t                optimum_block_size;
    fbe_lba_t                               start_lba;
    fbe_block_count_t                       block_count;
    fbe_lba_t                               pre_edge_start_lba;
    fbe_block_count_t                       pre_edge_block_count;
    fbe_lba_t                               post_edge_start_lba;
    fbe_block_count_t                       post_edge_block_count;
    fbe_u32_t                               number_of_edges;
    fbe_u32_t                               subpacket_index = 0;
    fbe_sg_element_t *                      sg_list_p = NULL;
    fbe_packet_t *                          packet_array_p[2];
    fbe_packet_t *master_packet_p = NULL;
    fbe_packet_t *                          current_packet_p = NULL;
    fbe_memory_header_t *current_memory_header = NULL;

    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_get_present_block_operation(sep_payload_p);

    /* The master packet allocated the memory initially.
     */
    master_packet_p = fbe_transport_get_master_packet(packet_p);
    memory_request_p = fbe_transport_get_memory_request(master_packet_p);

    /* Check allocation status */
    if (fbe_memory_request_is_allocation_complete(memory_request_p) == FBE_FALSE)
    {
        /* Currently this callback doesn't expect any aborted requests */
        fbe_provision_drive_utils_trace(provision_drive_p, 
                                        FBE_TRACE_LEVEL_ERROR,
                                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                        FBE_PROVISION_DRIVE_DEBUG_FLAG_USER_ZERO_TRACING,
                                        "USER_ZERO: memory request: 0x%p state: %d allocation failed \n",
                                        memory_request_p, memory_request_p->request_state);
        fbe_payload_block_set_status(block_operation_p,
                                     FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED,
                                     FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNEXPECTED_ERROR);
        fbe_provision_drive_utils_complete_packet(packet_p, FBE_STATUS_OK, 0);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    current_memory_header = memory_request_p->ptr;
    /* Skip the first packet.  This is our master packet.
     * We allocated a max of 3 packets initially, 1 master and 2 subpackets. 
     */
    current_memory_header = current_memory_header->u.hptr.next_header;
    packet_array_p[0] = (fbe_packet_t*)current_memory_header->data;

    current_memory_header = current_memory_header->u.hptr.next_header;
    packet_array_p[1] = (fbe_packet_t*)current_memory_header->data;

    /* get the optimum block size, start lba and block count. */
    fbe_block_transport_edge_get_optimum_block_size(&provision_drive_p->block_edge, &optimum_block_size);
    fbe_payload_block_get_lba(block_operation_p, &start_lba);
    fbe_payload_block_get_block_count(block_operation_p, &block_count);

    /* calculate the boundary range which does not cover part of the the chunk boundary. */
    fbe_provision_drive_utils_calculate_unaligned_edges_lba_range(start_lba,
                                                                  block_count,
                                                                  FBE_PROVISION_DRIVE_CHUNK_SIZE,
                                                                  &pre_edge_start_lba,
                                                                  &pre_edge_block_count,
                                                                  &post_edge_start_lba,
                                                                  &post_edge_block_count,
                                                                  &number_of_edges); /* either one or two */

    for(subpacket_index = 0; subpacket_index < number_of_edges; subpacket_index++)
    {
        /* Initialize the sub packets. */
        current_packet_p = packet_array_p[subpacket_index];
        fbe_transport_initialize_packet(current_packet_p);
        new_payload_p = fbe_transport_get_payload_ex(current_packet_p);

        /* fill out the subpacket with write same request for the nonaligned edge range of lbas. */
        if(pre_edge_start_lba != FBE_LBA_INVALID) {
            start_lba = pre_edge_start_lba;
            block_count = pre_edge_block_count;
            pre_edge_start_lba = FBE_LBA_INVALID;
        }
        else if(post_edge_start_lba != FBE_LBA_INVALID) {
            start_lba = post_edge_start_lba;
            block_count = post_edge_block_count;
            post_edge_start_lba = FBE_LBA_INVALID;
        }

        fbe_provision_drive_utils_trace(provision_drive_p,
                                        FBE_TRACE_LEVEL_DEBUG_HIGH,
                                        FBE_TRACE_MESSAGE_ID_INFO,
                                        FBE_PROVISION_DRIVE_DEBUG_FLAG_USER_ZERO_TRACING,
                                        "UZ:send write same, start_lba:0x%llx, blk:0x%llx, packet_index:0x%x.\n",
                                        (unsigned long long)start_lba,
					(unsigned long long)block_count,
					subpacket_index);


        /* allocate and initialize the block operation. */
        new_block_operation_p = fbe_payload_ex_allocate_block_operation(new_payload_p);

        /* Initialize SG element with zeroed buffer. */
		sg_list_p = fbe_zero_get_bit_bucket_sgl_1MB();

        /* CBE WRITE SAME */
        if(fbe_provision_drive_is_write_same_enabled(provision_drive_p, start_lba)){ /* Write Same IS enabled */

            /* if sg list pointer is null then complete packet with error. */
            if(sg_list_p == NULL)
            {
                fbe_provision_drive_utils_trace(provision_drive_p,
                                                FBE_TRACE_LEVEL_ERROR,
                                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                                FBE_PROVISION_DRIVE_DEBUG_FLAG_USER_ZERO_TRACING,
                                                "UZ:get sg list failed, line:%d\n", __LINE__);
                fbe_memory_free_request_entry(memory_request_p);
                fbe_payload_block_set_status(block_operation_p,
                                             FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                                             FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNEXPECTED_ERROR);
                fbe_provision_drive_utils_complete_packet(packet_p, FBE_STATUS_OK, 0);
                return FBE_STATUS_INSUFFICIENT_RESOURCES;
            }
    
			/* set the sg list for the write same zero request. */
			fbe_payload_ex_set_sg_list(new_payload_p, sg_list_p, 1);

			fbe_payload_block_build_operation(new_block_operation_p,
											  FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_SAME,
											  start_lba,
											  block_count,
											  block_operation_p->block_size,
											  optimum_block_size,
											  block_operation_p->pre_read_descriptor);
		} else { /* Write Same NOT enabled */
			fbe_sg_element_t * pre_sg_list;

            /* if sg list pointer is null then complete packet with error. */
            if(sg_list_p == NULL)
            {
                fbe_provision_drive_utils_trace(provision_drive_p,
                                                FBE_TRACE_LEVEL_ERROR,
                                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                                FBE_PROVISION_DRIVE_DEBUG_FLAG_USER_ZERO_TRACING,
                                                "UZ:get sg list failed, line:%d\n", __LINE__);
                fbe_memory_free_request_entry(memory_request_p);
                fbe_payload_block_set_status(block_operation_p,
                                             FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                                             FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNEXPECTED_ERROR);
                fbe_provision_drive_utils_complete_packet(packet_p, FBE_STATUS_OK, 0);
                return FBE_STATUS_INSUFFICIENT_RESOURCES;
            }
            
			fbe_payload_ex_get_pre_sg_list(new_payload_p, &pre_sg_list);
			pre_sg_list[0].address = sg_list_p[0].address;
			pre_sg_list[0].count = (fbe_u32_t)(block_operation_p->block_size * block_count);
			pre_sg_list[1].address = NULL;
			pre_sg_list[1].count = 0;

			/* set the sg list for the write same zero request. */
			fbe_payload_ex_set_sg_list(new_payload_p, NULL, 1);

			fbe_payload_block_build_operation(new_block_operation_p,
											  FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,
											  start_lba,
											  block_count,
											  block_operation_p->block_size,
											  optimum_block_size,
											  block_operation_p->pre_read_descriptor);

		}


        fbe_transport_propagate_expiration_time(current_packet_p, packet_p);
        fbe_transport_propagate_physical_drive_service_time(current_packet_p, packet_p);

        /* set completion functon to handle the write same request completion. */
        fbe_transport_set_completion_function(current_packet_p,
                                              fbe_provision_drive_user_zero_write_same_subpacket_completion,
                                              provision_drive_p);
        fbe_transport_add_subpacket(packet_p, current_packet_p);
    }

    /* put packet on the termination queue while we wait for the subpackets to complete. */
    fbe_transport_set_cancel_function(packet_p, fbe_base_object_packet_cancel_function, (fbe_base_object_t *) provision_drive_p);
    status = fbe_base_object_add_to_terminator_queue((fbe_base_object_t *)provision_drive_p, packet_p);

    /* set the packet status to success before sending subpacket. */
    fbe_payload_block_set_status(block_operation_p,
                                 FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID,
                                 FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID);

    /* send all other packets to perform the zeroing operation. */
    for(subpacket_index = 0; subpacket_index < number_of_edges; subpacket_index++)
    {
        /* get the current packet pointer from array of packets pointer. */
        current_packet_p = packet_array_p[subpacket_index];

        /* send packet to the block edge */
        status = fbe_base_config_send_functional_packet((fbe_base_config_t *)provision_drive_p, current_packet_p, 0);
    }
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * end fbe_provision_drive_user_zero_send_subpackets()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_user_zero_write_same_subpacket_completion()
 ******************************************************************************
 * @brief
 *
 * @param   packet_p        - pointer to disk zeroing IO packet
 * @param   context         - context, which is a pointer to the pvd object.
 *
 * @return fbe_status_t.    - status of the operation.
 *
 * @author
 *   07/14/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_user_zero_write_same_subpacket_completion(fbe_packet_t * packet_p,
                                                              fbe_packet_completion_context_t context)
{
    fbe_provision_drive_t *                 provision_drive_p = (fbe_provision_drive_t *)context;
    fbe_packet_t *                          master_packet_p = NULL;
    fbe_payload_ex_t *                         new_payload_p = NULL;
    fbe_payload_ex_t *                     sep_payload_p = NULL;
    fbe_payload_block_operation_t *         master_block_operation_p = NULL;
    fbe_payload_block_operation_t *         block_operation_p = NULL;
    fbe_lba_t                               start_lba = 0;
    fbe_block_count_t                       block_count = 0;
    fbe_payload_block_operation_status_t    block_operation_status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID;
    fbe_bool_t is_empty;
    fbe_status_t                           status;

    /* get the master packet. */
    master_packet_p = fbe_transport_get_master_packet(packet_p);

    sep_payload_p = fbe_transport_get_payload_ex(master_packet_p);
    new_payload_p = fbe_transport_get_payload_ex(packet_p);
    
    /* get the subpacket and master packet's block operation. */
    master_block_operation_p = fbe_payload_ex_get_present_block_operation(sep_payload_p);
    block_operation_p = fbe_payload_ex_get_block_operation(new_payload_p);
    fbe_payload_block_get_lba(block_operation_p, &start_lba);
    fbe_payload_block_get_block_count(block_operation_p, &block_count);
    fbe_payload_block_get_status(block_operation_p, &block_operation_status);

    /* For debug trace the write same completion */

    fbe_provision_drive_utils_trace(provision_drive_p,
                                    FBE_TRACE_LEVEL_DEBUG_HIGH,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    FBE_PROVISION_DRIVE_DEBUG_FLAG_USER_ZERO_TRACING,
                                    "UZ:complete write same, start_lba:0x%llx, blks:0x%llx, block status:0x%x.\n",
                                    (unsigned long long)start_lba,
				    (unsigned long long)block_count,
				    block_operation_status);

    /* update the master packet status if required. */
    fbe_provision_drive_utils_process_block_status(provision_drive_p, master_packet_p, packet_p);

    /* release the block operation. */
    fbe_payload_ex_release_block_operation(new_payload_p, block_operation_p);

    /* remove the subpacket from master packet. */
    //fbe_transport_remove_subpacket(packet_p);
    fbe_transport_remove_subpacket_is_queue_empty(packet_p, &is_empty);
    /* destroy the packet. */
    //fbe_transport_destroy_packet(packet_p);

    /* when the queue is empty, all subpackets have completed. */
    if(is_empty)
    {
        fbe_transport_destroy_subpackets(master_packet_p);

        /* We intentionally do not release the memory. 
         * Remember, way back in fbe_provision_drive_io_allocate_packet() 
         * we allocated 3 packets.  One is our master and the other two are being used here. 
         * When the master completes it will free this memory. 
         */

        /* remove the master packet from terminator queue and complete it. */
        fbe_base_object_remove_from_terminator_queue((fbe_base_object_t *) provision_drive_p, master_packet_p);
        status = fbe_transport_get_status_code(master_packet_p);
        if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t*)provision_drive_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "Ash:stat:0x%x \n",
                              status);
        }
        fbe_transport_complete_packet(master_packet_p);
    }
    else
    {
        /* not done with processing sub packets.
         */
    }
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_user_zero_write_same_subpacket_completion()
 ******************************************************************************/

static fbe_status_t 
fbe_provision_drive_user_zero_update_paged_and_checkpoint(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{
    fbe_provision_drive_t * provision_drive_p = (fbe_provision_drive_t *)context;
    fbe_status_t status;
    fbe_payload_ex_t *payload_p = NULL;
    fbe_payload_stripe_lock_operation_t *stripe_lock_operation_p = NULL;
    fbe_payload_stripe_lock_status_t stripe_lock_status;

    payload_p = fbe_transport_get_payload_ex(packet_p);
    stripe_lock_operation_p = fbe_payload_ex_get_stripe_lock_operation(payload_p);
    fbe_payload_stripe_lock_get_status(stripe_lock_operation_p, &stripe_lock_status);

    status = fbe_transport_get_status_code(packet_p);
    if ((status != FBE_STATUS_OK) || (stripe_lock_status != FBE_PAYLOAD_STRIPE_LOCK_STATUS_OK)){
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "UZ: get np lock failed packet status: 0x%x sl_st: 0x%x pkt: %p\n",
                              status, stripe_lock_status, packet_p);
        return FBE_STATUS_OK;
    }

    fbe_provision_drive_utils_trace(provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                    FBE_PROVISION_DRIVE_DEBUG_FLAG_USER_ZERO_TRACING,
                                    "UZ:set_chkpt NP lock acquired\n");

    fbe_transport_set_completion_function(packet_p, fbe_provision_drive_release_NP_lock, provision_drive_p);
	fbe_transport_set_completion_function(packet_p, fbe_provision_drive_zero_request_set_checkpoint, provision_drive_p);

	fbe_provision_drive_user_zero_update_paged_metadata(provision_drive_p, packet_p,
														fbe_provision_drive_user_zero_update_paged_metadata_completion,
														FBE_TRUE /* The paged metadata is valid. Preserve unmodified bits. */);

	return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}


/*******************************
 * end fbe_provision_drive_main.c
 *******************************/




