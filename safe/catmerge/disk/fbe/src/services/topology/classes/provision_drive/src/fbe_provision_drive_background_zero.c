/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_provision_drive_background_zero.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the functionality to process the background zero 
 *  request, it sends the write same request to perform the actual zeroing
 *  on the disk.
 * 
 * @ingroup provision_drive_class_files
 * 
 * @version
 *   7/14/2010:  Created. Dhaval Patel.
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

 /* fbe_provision_drive_background_zero.c */
static fbe_status_t fbe_provision_drive_background_zero_read_paged_metadata(fbe_provision_drive_t * provision_drive_p,
                                                                                    fbe_packet_t * packet_p);

static fbe_status_t fbe_provision_drive_background_zero_read_paged_metadata_completion(fbe_packet_t * packet_p,
                                                                                       fbe_packet_completion_context_t packet_completion_context);

static fbe_status_t fbe_provision_drive_background_zero_issue_write_subpacket_completion(fbe_packet_t * packet_p,
                                                                                         fbe_packet_completion_context_t context);

static fbe_status_t fbe_provision_drive_background_zero_update_paged_metadata(fbe_provision_drive_t * provision_drive_p,
                                                                              fbe_packet_t * packet_p);

static fbe_status_t fbe_provision_drive_background_zero_update_paged_metadata_completion(fbe_packet_t * packet_p,
                                                                                         fbe_packet_completion_context_t context);

static fbe_status_t fbe_provision_drive_background_zero_advance_checkpoint(fbe_provision_drive_t * provision_drive_p,
                                                                           fbe_packet_t * packet_p);

static fbe_status_t fbe_provision_drive_background_zero_advance_checkpoint_completion(fbe_packet_t * packet_p,
                                                                                      fbe_packet_completion_context_t context);

static fbe_bool_t fbe_provision_drive_get_next_chunk_to_zero(fbe_u8_t*     search_data,
                                                             fbe_u32_t    search_size,
                                                             context_t    context);

static fbe_bool_t fbe_provision_drive_get_next_invalid_paged_data_chunk(fbe_u8_t*     search_data,
                                                                        fbe_u32_t    search_size,
                                                                        context_t    context);

static fbe_status_t fbe_provision_drive_get_NP_lock_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);


/*!****************************************************************************
 * @fn fbe_provision_drive_background_zero_process_background_zero_request()
 ******************************************************************************
 * @brief
 *  This function is used as entry point for the background zeroing I/O coming
 *  from provision drive object monitor.
 *
 * @param provision_drive_p - Pointer to the provision drive object.
 * @param packet_p          - Pointer the the I/O packet.
 *
 * @return fbe_status_t.
 *
 * @author
 *  4/15/2009 - Created. Dhaval Patel
 *
 ******************************************************************************/
fbe_status_t 
fbe_provision_drive_background_zero_process_background_zero_request(fbe_provision_drive_t * provision_drive_p, 
                                                                    fbe_packet_t * packet_p)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_payload_ex_t *                     sep_payload_p = NULL;
    fbe_payload_block_operation_t *         block_operation_p = NULL;
    fbe_bool_t                              is_request_aligned_to_chunk = FBE_FALSE;
    fbe_lba_t                               start_lba;
    fbe_block_count_t                       block_count;

    /* Get the sep payload and block operation. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_get_present_block_operation(sep_payload_p);

    /* Get the start lba and block count. */
    fbe_payload_block_get_lba(block_operation_p, &start_lba);
    fbe_payload_block_get_block_count(block_operation_p, &block_count);

    /* Find out whether incoming background zeroing request is aligned with chunk size boundary or not,
     * if it is not aligne then fail the background zeroing request. 
     */
    status = fbe_provision_drive_utils_is_io_request_aligned(start_lba, block_count, 
                                                             FBE_PROVISION_DRIVE_CHUNK_SIZE, &is_request_aligned_to_chunk);
    if((status != FBE_STATUS_OK) || 
       (!is_request_aligned_to_chunk))
    {
        /* If status is not good or background request is not aligned then return the error. */
        fbe_provision_drive_utils_trace(provision_drive_p, 
                                        FBE_TRACE_LEVEL_WARNING,
                                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                        FBE_PROVISION_DRIVE_DEBUG_FLAG_ZOD_TRACING,
                                        "BG_ZERO:unaligned zero request, status:0x%x, start_lba:0x%llx, blks:0x%llx, packet_p:%p\n",
                                        status, (unsigned long long)start_lba,
				        (unsigned long long)block_count,
				        packet_p);

        fbe_payload_block_set_status(block_operation_p,
                                     FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                                     FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNALIGNED_ZERO_REQUEST);
        fbe_provision_drive_utils_complete_packet(packet_p, FBE_STATUS_OK, 0);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*!@todo Add more validation to make sure that background request is below metadata region. */

    /* Get the NP lock */
    status = fbe_provision_drive_get_NP_lock(provision_drive_p, packet_p, fbe_provision_drive_get_NP_lock_completion);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * end fbe_provision_drive_background_zero_process_background_zero_request()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_get_NP_lock_completion()
 ******************************************************************************
 * @brief
 *  This function is used calls the read paged metadata for background zeroing request
 *
 * @param in_packet_p      - pointer to a control packet from the scheduler.
 * @param in_context       - context, which is a pointer to the base object.
 * 
 * @return fbe_status_t    - Always FBE_STATUS_OK
 *
 * @author
 *  2/23/2012 - Created. Ashwin Tamilarasan
 *
 ******************************************************************************/
static fbe_status_t 
fbe_provision_drive_get_NP_lock_completion(fbe_packet_t * packet_p,
                                           fbe_packet_completion_context_t context)
{
    fbe_provision_drive_t *                 provision_drive_p = NULL;
    fbe_status_t                            status;
    fbe_payload_ex_t *                      sep_payload_p = NULL;
    fbe_lba_t                               start_lba;
    fbe_payload_block_operation_t *         block_operation_p = NULL;
    fbe_lba_t checkpoint;
    fbe_payload_block_operation_opcode_t block_operation_opcode;

    /* Get the sep payload and block operation. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_get_present_block_operation(sep_payload_p);

    /* Get the start lba of the block operation. */
    fbe_payload_block_get_lba(block_operation_p, &start_lba);
    
    provision_drive_p = (fbe_provision_drive_t*)context;

    /* If the status is not ok, that means we didn't get the 
       lock. Just return. we are alredy in the completion routine
    */
    status = fbe_transport_get_status_code (packet_p);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s can't get the NP lock, status: 0x%X", __FUNCTION__, status); 
        return FBE_STATUS_OK;
    }
    
    fbe_transport_set_completion_function(packet_p, fbe_provision_drive_release_NP_lock, provision_drive_p);

    /* If the checkpoint moved back while we were waiting for the lock, then we will just complete the packet, 
     * and pick up the zero on the next monitor cycle. 
     * This is key since if we continued the zero we would try to move the checkpoint forward and there are 
     * no checks for that at the end of the zero. 
     */
    fbe_payload_block_get_opcode(block_operation_p, &block_operation_opcode);

    if(block_operation_opcode != FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_INVALIDATE)
    {
        fbe_provision_drive_metadata_get_background_zero_checkpoint(provision_drive_p, &checkpoint);
    }
    else
    {
        fbe_provision_drive_metadata_get_verify_invalidate_checkpoint(provision_drive_p, &checkpoint);
    }
    
    if (checkpoint < start_lba)
    {
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s BG_ZERO: chkpt moved as expected from 0x%llx to 0x%llx\n", __FUNCTION__, 
                              checkpoint, start_lba); 
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    /* we should have already acquired stripe lock before and so read paged metadata to determine whether we need to perform zeroing operation. */
    status = fbe_provision_drive_background_zero_read_paged_metadata(provision_drive_p, packet_p);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * end fbe_provision_drive_get_NP_lock_completion()
 ******************************************************************************/

extern fbe_status_t fbe_provision_drive_metadata_paged_callback_get_info(fbe_packet_t *packet, 
                                                     fbe_payload_metadata_operation_t ** out_mdo,
                                                     fbe_lba_t * lba_offset,
                                                     fbe_u64_t * slot_offset,
                                                     fbe_sg_element_t ** sg_ptr);
/*!****************************************************************************
 * @fn fbe_provision_drive_background_zero_read_paged_metadata_callback()
 ******************************************************************************
 * @brief
 *
 *  This is the function to set bits for paged metadata callback functions. 
 * 
 * @param packet               - Pointer to packet.
 * @param context              - Pointer to context.
 *
 * @return fbe_status_t.
 *  09/24/2014 - Created. Lili Chen
 *
 ******************************************************************************/
fbe_status_t 
fbe_provision_drive_background_zero_read_paged_metadata_callback(fbe_packet_t * packet, fbe_metadata_callback_context_t context)
{
    fbe_payload_metadata_operation_t * mdo = NULL;
    fbe_sg_element_t * sg_ptr = NULL;
    fbe_lba_t lba_offset;
    fbe_u64_t slot_offset;
    fbe_u8_t * paged_metadata_p;
    fbe_bool_t is_chunk_marked = FBE_FALSE;
    fbe_u64_t metadata_offset;
    fbe_u8_t *dst_ptr = NULL;
    fbe_u32_t dst_size;
    fbe_u8_t record_data[FBE_PAYLOAD_METADATA_MAX_DATA_SIZE]; 

    if (fbe_provision_drive_metadata_paged_callback_get_info(packet, &mdo, &lba_offset, &slot_offset, &sg_ptr) != FBE_STATUS_OK){
        return FBE_STATUS_GENERIC_FAILURE;
    }

    paged_metadata_p = sg_ptr->address + slot_offset;
    metadata_offset = mdo->u.metadata_callback.offset;
    dst_size = (fbe_u32_t) (mdo->u.metadata_callback.record_data_size * mdo->u.metadata_callback.repeat_count);
    fbe_zero_memory(mdo->u.metadata_callback.record_data, dst_size);

    while (sg_ptr->address != NULL)
    {
        /* Get the data for evaluation. */
        if (slot_offset + dst_size <= FBE_METADATA_BLOCK_DATA_SIZE) {
            dst_ptr = paged_metadata_p;
            slot_offset += dst_size;
            paged_metadata_p += dst_size;
        } else {
            sg_ptr++;
            if (sg_ptr->address == NULL) {
                mdo->u.metadata_callback.current_offset = metadata_offset;
                fbe_copy_memory(mdo->u.metadata_callback.record_data, dst_ptr, dst_size);
                return FBE_STATUS_OK;
            }
            /* Data overlap 2 slots */
            fbe_copy_memory(record_data, paged_metadata_p, FBE_METADATA_BLOCK_DATA_SIZE - (fbe_u32_t)slot_offset);
            paged_metadata_p = sg_ptr->address;
            fbe_copy_memory(record_data + FBE_METADATA_BLOCK_DATA_SIZE - slot_offset,
							paged_metadata_p, dst_size - (FBE_METADATA_BLOCK_DATA_SIZE - (fbe_u32_t)slot_offset));

            dst_ptr = record_data;
            slot_offset = dst_size - (FBE_METADATA_BLOCK_DATA_SIZE - slot_offset);
            paged_metadata_p += slot_offset;
        }

        /* Before check the data, add private count */
        //mdo->u.metadata_callback.current_count_private++;

        /* Evaluate the chunk. */
        is_chunk_marked = fbe_provision_drive_get_next_chunk_to_zero(dst_ptr, dst_size, NULL);
        if (is_chunk_marked)
        {
            mdo->u.metadata_callback.current_offset = metadata_offset;
            fbe_copy_memory(mdo->u.metadata_callback.record_data, dst_ptr, dst_size);
            return FBE_STATUS_OK;
        }

        /* Check for for next page */
        if (paged_metadata_p == (sg_ptr->address + FBE_METADATA_BLOCK_DATA_SIZE))
        {
            sg_ptr++;
            if (sg_ptr->address == NULL) {
                mdo->u.metadata_callback.current_offset = metadata_offset;
                fbe_copy_memory(mdo->u.metadata_callback.record_data, dst_ptr, dst_size);
                return FBE_STATUS_OK;
            }
            slot_offset = 0;
            paged_metadata_p = sg_ptr->address;
        }

        metadata_offset += dst_size; 
    }

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_background_zero_read_paged_metadata_callback()
 ******************************************************************************/

/*!****************************************************************************
 *  fbe_provision_drive_background_zero_read_paged_metadata()
 ******************************************************************************
 * @brief
 *  This function is used to issue the paged metadata read operation to determine
 *  whether we need to zero for the chunks.
 *
 * @param provision_drive_p  - pointer to the provision drive object
 * @param packet_p           - pointer to a control packet from the scheduler

 *
 * @return fbe_status_t.
 *
 * @author
 *  04/23/2010 - Created. Dhaval Patel
 *  03/15/2012 - Ashok Tamilarasan - Update to handle Verify invalidate opcode
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_background_zero_read_paged_metadata(fbe_provision_drive_t * provision_drive_p,
                                                        fbe_packet_t * packet_p)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_payload_metadata_operation_t *      metadata_operation_p = NULL;
    fbe_payload_ex_t *                     sep_payload_p = NULL;
    fbe_u64_t                               metadata_offset = FBE_LBA_INVALID;
    fbe_lba_t                               start_lba;
    fbe_block_count_t                       block_count;
    fbe_chunk_index_t                       start_chunk_index;
    fbe_chunk_count_t                       chunk_count;
    fbe_payload_block_operation_t *         block_operation_p = NULL;
    fbe_lba_t           metadata_start_lba;
    fbe_block_count_t   metadata_block_count;
    metadata_search_fn_t search_function;
    fbe_payload_block_operation_opcode_t                block_operation_opcode;

    /* Get the sep payload and block operation. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_get_present_block_operation(sep_payload_p);

    /* Get the start lba of the block operation. */
    fbe_payload_block_get_lba(block_operation_p, &start_lba);
    fbe_payload_block_get_block_count(block_operation_p, &block_count);

    /* Get the start chunk index and chunk count from the start lba and end lba of the range. */
    fbe_provision_drive_utils_calculate_chunk_range(start_lba,
                                                    block_count,
                                                    FBE_PROVISION_DRIVE_CHUNK_SIZE,
                                                    &start_chunk_index,
                                                    &chunk_count);

    /* Find the metadata offset for the zero checkpoint. */
    metadata_offset = start_chunk_index * sizeof(fbe_provision_drive_paged_metadata_t);

    /* Set the completion function before issuing paged metadata read operation. */
    fbe_transport_set_completion_function(packet_p,
                                          fbe_provision_drive_background_zero_read_paged_metadata_completion,
                                          provision_drive_p);

    /* Allocate the metadata operation. */
    metadata_operation_p =  fbe_payload_ex_allocate_metadata_operation(sep_payload_p);

    /* First, get the opcodes of the block operation. Based on the opcode, we need to have
     * different check function. Incase of zeroing, we need to get next chunk that is marked
     * for zero and in case of verify invalidate, need to get next chunk that is invalid
     */
    fbe_payload_block_get_opcode(block_operation_p, &block_operation_opcode);

    if(block_operation_opcode != FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_INVALIDATE)
    {
        //search_function = fbe_provision_drive_get_next_chunk_to_zero;
        fbe_payload_metadata_build_paged_read(metadata_operation_p,
                                              &(((fbe_base_config_t *) provision_drive_p)->metadata_element),
                                              metadata_offset,
                                              sizeof(fbe_provision_drive_paged_metadata_t),
                                              chunk_count,
                                              fbe_provision_drive_background_zero_read_paged_metadata_callback,
                                              (void *)metadata_operation_p);
        fbe_payload_metadata_set_metadata_operation_flags(metadata_operation_p, FBE_PAYLOAD_METADATA_OPERATION_PAGED_USE_BGZ_CACHE);
    }
    else
    {
        search_function = fbe_provision_drive_get_next_invalid_paged_data_chunk;
        fbe_payload_metadata_build_paged_get_next_marked_bits(metadata_operation_p,
                                                          &(((fbe_base_config_t *) provision_drive_p)->metadata_element),
                                                          metadata_offset,
                                                          NULL,
                                                          chunk_count * sizeof(fbe_provision_drive_paged_metadata_t),
                                                          sizeof(fbe_provision_drive_paged_metadata_t),
                                                          search_function,// callback function
                                                          chunk_count * sizeof(fbe_provision_drive_paged_metadata_t), // search size
                                                          provision_drive_p ); // context 
    }

    /* Initialize cient_blob, skip the subpacket */
    fbe_provision_drive_metadata_paged_init_client_blob(packet_p, metadata_operation_p);

	fbe_metadata_paged_get_lock_range(metadata_operation_p, &metadata_start_lba, &metadata_block_count);

    fbe_payload_metadata_set_metadata_stripe_offset(metadata_operation_p, metadata_start_lba);
    fbe_payload_metadata_set_metadata_stripe_count(metadata_operation_p, metadata_block_count);

    fbe_payload_ex_increment_metadata_operation_level(sep_payload_p);

    /* Issue the metadata operation. */
    status =  fbe_metadata_operation_entry(packet_p);
    return status;
}
/******************************************************************************
 * end fbe_provision_drive_background_zero_read_paged_metadata()
 ******************************************************************************/

/*!****************************************************************************
 *  fbe_provision_drive_background_zero_read_paged_metadata_completion()
 ******************************************************************************
 * @brief
 *  This function handles paged metadata read completion.
 *
 *  It checks need_zero bit value and if it is set then it sends disk zero IO
 *  request otherwise sends checkpoint update request.
 *
 * @param packet_p          - pointer to a control packet from the scheduler
 * @param in_context        - context, which is a pointer to the provision drive
 *                            object
 *
 * @return  fbe_status_t  
 *
 * @author
 *   04/23/2010 - Created. Dhaval Patel
 *   03/09/2012 - Ashok Tamilarasan - Added metadata error handling checks
 *
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_background_zero_read_paged_metadata_completion(fbe_packet_t * packet_p,
                                                                   fbe_packet_completion_context_t packet_completion_context)
{
    fbe_provision_drive_t *                             provision_drive_p = NULL;
    fbe_payload_metadata_operation_t *                  metadata_operation_p = NULL;
    fbe_payload_block_operation_t *                     block_operation_p = NULL;
    fbe_payload_ex_t *                                 sep_payload_p = NULL;
    fbe_status_t                                        status = FBE_STATUS_OK;
    fbe_provision_drive_paged_metadata_t *              paged_data_bits_p = NULL;    /* INPUT */
    fbe_lba_t                                           start_lba;
    fbe_block_count_t                                   block_count;
    fbe_chunk_index_t                                   start_chunk_index;
    fbe_chunk_count_t                                   chunk_count;
    fbe_chunk_count_t                                   chunk_index;
    fbe_chunk_count_t                                   chunks_marked = 0;
    fbe_payload_metadata_status_t                       metadata_status; 
    fbe_u64_t                                           current_offset;
    fbe_lba_t                                           new_checkpoint;
    fbe_chunk_index_t                                   new_chunk_index;
    fbe_bool_t                                          is_paged_data_invalid = FBE_FALSE;
    fbe_payload_block_operation_opcode_t                block_operation_opcode;
    fbe_chunk_count_t                                   number_of_exported_chunks;
    
    provision_drive_p = (fbe_provision_drive_t *) packet_completion_context;
 
    /* Get the payload and metadata operation. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    metadata_operation_p =  fbe_payload_ex_get_metadata_operation(sep_payload_p);

    /* get the previous block operation. */
    block_operation_p =  fbe_payload_ex_get_present_block_operation(sep_payload_p);

    /* Get the status of the read paged metadata operation. */
    fbe_payload_metadata_get_status(metadata_operation_p, &metadata_status);
    status = fbe_transport_get_status_code(packet_p);

    /* First, get the opcodes of the block operation. */
    fbe_payload_block_get_opcode(block_operation_p, &block_operation_opcode);

    /* We hit the paged cache and no need to check the paged metadata. */
    if (status == FBE_STATUS_OK && metadata_status == FBE_PAYLOAD_METADATA_STATUS_PAGED_CACHE_HIT)
    {
        fbe_provision_drive_utils_trace(provision_drive_p,
                                        FBE_TRACE_LEVEL_DEBUG_LOW,
                                        FBE_TRACE_MESSAGE_ID_INFO,
                                        FBE_PROVISION_DRIVE_DEBUG_FLAG_BGZ_TRACING,
                                        "BG_ZERO:md read complete cache hit packet %p opcode 0x%x.\n",
                                        packet_p, block_operation_opcode);

        /* set the completion function before we issue write same request. */
        fbe_transport_set_completion_function(packet_p,
                                          fbe_provision_drive_background_zero_issue_write_same_completion,
                                          provision_drive_p);

        /* send the write same request to perform the background zero operation. */
        status = fbe_provision_drive_background_zero_issue_write_same(provision_drive_p, packet_p);
    
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    /* Based on the metadata status set the appropriate block status. */
    if((status != FBE_STATUS_OK) ||
       (metadata_status != FBE_PAYLOAD_METADATA_STATUS_OK))
    {
         fbe_provision_drive_utils_trace(provision_drive_p,
                                        FBE_TRACE_LEVEL_INFO,
                                        FBE_TRACE_MESSAGE_ID_INFO,
                                        FBE_PROVISION_DRIVE_DEBUG_FLAG_ZOD_TRACING,
                                        "BG_ZERO:paged metadata get bits failed, metadata_status:0x%x, status:0x%x.\n",
                                        metadata_status, status);

         
        /* If this is a background zeroing and unable to read the metadata, we need to kick off
         * the verify invalidate code
         */
        if(metadata_status != FBE_PAYLOAD_METADATA_STATUS_IO_UNCORRECTABLE)
        {
            fbe_payload_ex_release_metadata_operation(sep_payload_p, metadata_operation_p);
             /* Even non-retryable errors back to the monitor must be accompanied 
             * by a state change. Since we are not guaranteed to be getting a state 
             * change (since it might be caused by a quiesce), we must go ahead and 
             * return this as retryable.  If the downstream object is still accessible
             * the monitor will retry the background zero request.
             */
            fbe_payload_block_set_status(block_operation_p,
                                         FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED,
                                         FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_POSSIBLE);
            fbe_transport_set_status(packet_p, status, 0);
            return status;
        }

        /* If this is a background zeroing and unable to read the metadata, we need to kick off
         * the verify invalidate code. If this is a verify invalidate operation, then fall through
         * and invalidate the region.
         */
        if(block_operation_opcode != FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_INVALIDATE)
        {
            fbe_payload_ex_release_metadata_operation(sep_payload_p, metadata_operation_p);
            /* Unable to read the metadata to determine the state of this region. So this region
             * needs to be invalidated and reported to client. Kick of verify invalidate code 
             * that handles the user region approrpriately
             */
            
            fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const, (fbe_base_object_t *) provision_drive_p,
                                   FBE_PROVISION_DRIVE_LIFECYCLE_COND_METADATA_VERIFY_INVALIDATE);

            return FBE_STATUS_OK;
        }
    }

    /* get the lba and block count from the block operation. */
    fbe_payload_block_get_lba(block_operation_p, &start_lba);
    fbe_payload_block_get_block_count(block_operation_p, &block_count);

    /* Get the start chunk index and chunk count from the start lba and end lba of the range. */
    fbe_provision_drive_utils_calculate_chunk_range(start_lba,
                                                    block_count,
                                                    FBE_PROVISION_DRIVE_CHUNK_SIZE,
                                                    &start_chunk_index,
                                                    &chunk_count);

    /* Current offset gives the new metadata offset for the chunk that needs to be acted upon*/
    current_offset = metadata_operation_p->u.next_marked_chunk.current_offset;

    /* calculate the new checkpoint based on the new metadata offset */
    new_chunk_index = current_offset/sizeof(fbe_provision_drive_paged_metadata_t);

    /* It is possible that there were no chunk marked we might have reached the end of drive 
     * Get the index count for user region. Check if we have reached the end of user region 
     */
    fbe_provision_drive_utils_get_num_of_exported_chunks(provision_drive_p, &number_of_exported_chunks);

    if(new_chunk_index >= number_of_exported_chunks)
    {
        /* we have reached the end of exported region, set the chunk index to the end of region */
        new_chunk_index = number_of_exported_chunks;
    }
    fbe_provision_drive_utils_get_lba_for_chunk_index(provision_drive_p,
                                                      new_chunk_index,
                                                      &new_checkpoint);

    /*!@todo If we are not incrementing the checkpoint, then release the NP lock
      to make it more efficient
    */
    if(new_checkpoint > start_lba)
    {
        /* We will have to release the metadata operation here since we are done with
           the paged metadata read 
         */
        fbe_payload_ex_release_metadata_operation(sep_payload_p, metadata_operation_p);
        fbe_provision_drive_utils_trace(provision_drive_p,
                                        FBE_TRACE_LEVEL_INFO,
                                        FBE_TRACE_MESSAGE_ID_INFO,
                                        FBE_PROVISION_DRIVE_DEBUG_FLAG_NONE,
                                        "BG_ZERO:Moving chkpt cur:0x%llx new:0x%llx op:%d\n",
                                         start_lba, new_checkpoint, block_operation_opcode);

        block_count = new_checkpoint - start_lba;
        
        //fbe_transport_set_completion_function(packet_p, fbe_provision_drive_background_zero_advance_checkpoint_completion,
          //                                    provision_drive_p);

        if(block_operation_opcode != FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_INVALIDATE)
        {
            fbe_provision_drive_metadata_incr_background_zero_checkpoint(provision_drive_p, packet_p, 
                                                                         start_lba, block_count);
        }
        else
        {
            fbe_provision_drive_metadata_incr_verify_invalidate_checkpoint(provision_drive_p, packet_p, 
                                                                           start_lba, block_count);
        }

        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

     /* get the paged metadata. */
    paged_data_bits_p = (fbe_provision_drive_paged_metadata_t *) metadata_operation_p->u.metadata.record_data;

    /* Before processing the paged data, make sure the data is valid */
    fbe_provision_drive_bitmap_is_paged_data_invalid(paged_data_bits_p, &is_paged_data_invalid);

    if((is_paged_data_invalid) && 
       (block_operation_opcode != FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_INVALIDATE))
    {
        fbe_payload_ex_release_metadata_operation(sep_payload_p, metadata_operation_p);
        /* The paged data is invalid and cannot determine the state of this region. So this region
         * needs to be invalidated and reported to client. Kick of verify invalidate code that 
         * handles the user region approrpriately 
         */
        fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const, (fbe_base_object_t *) provision_drive_p,
                               FBE_PROVISION_DRIVE_LIFECYCLE_COND_METADATA_VERIFY_INVALIDATE);

        return FBE_STATUS_OK;
    }

    /* debug trace for the paged metadata. */
    for(chunk_index = 0; chunk_index < chunk_count; chunk_index++)
    {
        fbe_provision_drive_utils_trace(provision_drive_p,
                                        FBE_TRACE_LEVEL_DEBUG_HIGH,
                                        FBE_TRACE_MESSAGE_ID_INFO,
                                        FBE_PROVISION_DRIVE_DEBUG_FLAG_BGZ_TRACING,
                                        "BG_ZERO:read paged data complete, start_lba 0x%llx, blk: 0x%llx, chunk index 0x%llx, NZ %d, UZ  %d, DB %d\n",
                                        (unsigned long long)start_lba,
					(unsigned long long)block_count,
					(unsigned long long)(start_chunk_index + chunk_index),
                                        ((paged_data_bits_p[chunk_index].need_zero_bit == TRUE) ? 1 : 0),
                                        ((paged_data_bits_p[chunk_index].user_zero_bit == TRUE) ? 1 : 0),
                                        ((paged_data_bits_p[chunk_index].consumed_user_data_bit == TRUE) ? 1 : 0));
    }

    if((block_operation_opcode != FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_INVALIDATE))
    {
        /* Determine number of chunks marked for zeroing. */
        fbe_provision_drive_bitmap_get_number_of_chunks_marked_for_zero(paged_data_bits_p,
                                                                        chunk_count,
                                                                        &chunks_marked);
    
        /* if none of the chunks marked for zero the update the metadata and complete the 
         * background zero operation.
         */
        if(!chunks_marked)
        {
            /* update the checkpoint without issuing zero request. */
            status = fbe_provision_drive_background_zero_update_paged_metadata(provision_drive_p, packet_p);
            return FBE_STATUS_MORE_PROCESSING_REQUIRED;
        }
    }
    else
    {
        /* Before we go and invalidate the user region, need to make sure if RAID has marked for error verify 
         * so that it can turn around and fix the region */
        if((fbe_provision_drive_get_verify_invalidate_ts_lba(provision_drive_p) == start_lba ) &&
           (fbe_provision_drive_get_verify_invalidate_proceed(provision_drive_p)))
        {
            /* We can proceed for this LBA, so reset the LBA and proceed values */
            fbe_provision_drive_set_verify_invalidate_ts_lba(provision_drive_p, FBE_LBA_INVALID);
            fbe_provision_drive_set_verify_invalidate_proceed(provision_drive_p, FBE_FALSE);
        }   
        else
        {
            fbe_payload_ex_release_metadata_operation(sep_payload_p, metadata_operation_p);

            /* Need to let upstream objects know that we are going to invalidate the region */
            fbe_provision_drive_set_verify_invalidate_ts_lba(provision_drive_p, start_lba);
            fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const, (fbe_base_object_t *) provision_drive_p,
                                   FBE_PROVISION_DRIVE_LIFECYCLE_COND_VERIFY_INVALIDATE_REMAP_REQUEST);
            return FBE_STATUS_OK;
        }
        /* Determine number of chunks marked for invalidation. */
        fbe_provision_drive_bitmap_get_number_of_invalid_chunks(paged_data_bits_p,
                                                                chunk_count,
                                                                &chunks_marked);
    }

    /* set the completion function before we issue write same request. */
    fbe_transport_set_completion_function(packet_p,
                                          fbe_provision_drive_background_zero_issue_write_same_completion,
                                          provision_drive_p);

    /* send the write same request to perform the background zero operation. */
    status = fbe_provision_drive_background_zero_issue_write_same(provision_drive_p, packet_p);
    
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * end fbe_provision_drive_background_zero_read_paged_metadata_completion()
 ******************************************************************************/


/*!****************************************************************************
 * @fn fbe_provision_drive_background_zero_issue_write_same()
 ******************************************************************************
 * @brief
 *  This function simply allocates a packet and sends it down for the write same.
 *  Note that this code does not handle the case where there is nothing to do,
 *  it assumes that everything in the block operation needs to get zeroed/invalidated.
 *
 * @param provision_drive_p             - Pointer to the provision drive object.
 * @param packet_p                      - Pointer to the packet.
 *
 * @return fbe_status_t                 - status of the operation.
 *
 * @author
 *  9/17/2012 - Created. Rob Foley
 *
 ******************************************************************************/
fbe_status_t
fbe_provision_drive_background_zero_issue_write_same(fbe_provision_drive_t * provision_drive_p,
                                                     fbe_packet_t * packet_p)
{
    fbe_payload_ex_t *                     sep_payload_p = NULL;
    fbe_payload_block_operation_t *         block_operation_p = NULL;
    fbe_payload_block_operation_t *         new_block_operation_p = NULL;
    fbe_status_t                            status;
    fbe_optimum_block_size_t                optimum_block_size;
    fbe_sg_element_t *                      sg_list_p = NULL;
    fbe_payload_block_operation_opcode_t    block_operation_opcode;
	fbe_sg_element_t						* pre_sg_list;

    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p =  fbe_payload_ex_get_present_block_operation(sep_payload_p);

    fbe_block_transport_edge_get_optimum_block_size(&provision_drive_p->block_edge, &optimum_block_size);
    fbe_payload_block_get_opcode(block_operation_p, &block_operation_opcode);

	/* Allocate block operation. */
    new_block_operation_p = fbe_payload_ex_allocate_block_operation(sep_payload_p);

    if((block_operation_opcode != FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_INVALIDATE)) {        
        sg_list_p = fbe_zero_get_bit_bucket_sgl_1MB(); /* SG element with zeroed buffer. */
    } else {        
        sg_list_p = fbe_invalid_pattern_get_bit_bucket_sgl_1MB(); /* Invalid pattern sg list. */
    }

    if ((block_operation_opcode != FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_INVALIDATE) && 
        fbe_provision_drive_is_unmap_supported(provision_drive_p)) {
        /* Initialize block operation. */
		fbe_payload_block_build_operation(new_block_operation_p,
										  FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_SAME,
										  block_operation_p->lba,
										  block_operation_p->block_count,
										  block_operation_p->block_size,
										  optimum_block_size,
										  block_operation_p->pre_read_descriptor);

        new_block_operation_p->block_flags |= FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_UNMAP; 
        sg_list_p = fbe_unmap_get_bit_bucket_sgl();
        fbe_payload_ex_set_sg_list(sep_payload_p, sg_list_p, 0);

        fbe_provision_drive_utils_trace(provision_drive_p,
                                        FBE_TRACE_LEVEL_DEBUG_LOW,
                                        FBE_TRACE_MESSAGE_ID_INFO,
                                        FBE_PROVISION_DRIVE_DEBUG_FLAG_BGZ_TRACING,
                                        "UNMAP BGZ:send write same, start_lba:0x%llx, blk:0x%llx\n",
                                        (unsigned long long)block_operation_p->lba,
                    		        (unsigned long long)block_operation_p->block_count);

    /* CBE WRITE SAME */
	} else if(fbe_provision_drive_is_write_same_enabled(provision_drive_p, block_operation_p->lba)){ /* Write Same IS supported */
		
		/* Initialize block operation. */
		fbe_payload_block_build_operation(new_block_operation_p,
										  FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_SAME,
										  block_operation_p->lba,
										  block_operation_p->block_count,
										  block_operation_p->block_size,
										  optimum_block_size,
										  block_operation_p->pre_read_descriptor);

		fbe_payload_ex_set_sg_list(sep_payload_p, sg_list_p, 0);

	} else { /* Write same NOT supported */
	
		fbe_payload_ex_get_pre_sg_list(sep_payload_p, &pre_sg_list);
		pre_sg_list[0].address = sg_list_p[0].address;
		pre_sg_list[0].count = (fbe_u32_t)(block_operation_p->block_size * block_operation_p->block_count);
		pre_sg_list[1].address = NULL;
		pre_sg_list[1].count = 0;

		/* Sanity checking */
		if(sg_list_p[0].count < pre_sg_list[0].count){
			fbe_provision_drive_utils_trace(provision_drive_p,
											FBE_TRACE_LEVEL_CRITICAL_ERROR,
											FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
											FBE_PROVISION_DRIVE_DEBUG_FLAG_ZOD_TRACING,
											"CBE WRITE SAME: Invalid SGL count %I64d \n", pre_sg_list[0].count);
		}

		/* Initialize block operation. */
		fbe_payload_block_build_operation(new_block_operation_p,
										  FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,
										  block_operation_p->lba,
										  block_operation_p->block_count,
										  block_operation_p->block_size,
										  optimum_block_size,
										  block_operation_p->pre_read_descriptor);

	    /* Set the sg list to NULL. */
	    fbe_payload_ex_set_sg_list(sep_payload_p, NULL, 0);
	}



    fbe_transport_set_completion_function(packet_p,
                                          fbe_provision_drive_release_block_op_completion,
                                          provision_drive_p);

    /* set the packet status to success before sending subpacket. */
    fbe_payload_block_set_status(block_operation_p,
                                 FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID,
                                 FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID);

    /* send packet to the block edge */
    status = fbe_base_config_send_functional_packet((fbe_base_config_t *)provision_drive_p, packet_p, 0);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * end fbe_provision_drive_background_zero_issue_write_same_allocate_completion()
 ******************************************************************************/

/*!****************************************************************************
 * @fn fbe_provision_drive_background_zero_issue_write_same_completion()
 ******************************************************************************
 * @brief
 *
 *  This function is used to handle the completion of the write same (zero)
 *  request, it initiates the updating paged metadata operation for the required
 *  chunks before it updates the checkpoint to advance it.
 * 
 * @param memory_request    - Pointer to memory request.
 * @param context           - Pointer to the provision drive object.
 *
 * @return fbe_status_t.
 *  21/06/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
fbe_status_t
fbe_provision_drive_background_zero_issue_write_same_completion(fbe_packet_t * packet_p,
                                                                fbe_packet_completion_context_t packet_completion_context)
{
    fbe_provision_drive_t *                             provision_drive_p = NULL;
    fbe_payload_metadata_operation_t *                  metadata_operation_p = NULL;    
    fbe_payload_block_operation_t *                     block_operation_p = NULL;
    fbe_payload_block_operation_status_t                block_status;
    fbe_payload_ex_t *                                 sep_payload_p = NULL;
    fbe_status_t                                        status = FBE_STATUS_OK;
    fbe_payload_block_operation_opcode_t                block_operation_opcode;

    provision_drive_p = (fbe_provision_drive_t *) packet_completion_context;

    /* Get the payload and metadata operation. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    metadata_operation_p =  fbe_payload_ex_get_metadata_operation(sep_payload_p);

    /* get the block operation */
    block_operation_p =  fbe_payload_ex_get_present_block_operation(sep_payload_p);

    /* Get the status of the actual zeroing write same request. */
    status = fbe_transport_get_status_code(packet_p);
    fbe_payload_block_get_status(block_operation_p, &block_status);

    if((status != FBE_STATUS_OK) ||
       (block_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS))
    {
        fbe_payload_ex_release_metadata_operation(sep_payload_p, metadata_operation_p);
        fbe_transport_set_status(packet_p, status, 0);
        return status;
    }


    /* First, get the opcodes of the block operation. */
    fbe_payload_block_get_opcode(block_operation_p, &block_operation_opcode);

    if((block_operation_opcode != FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_INVALIDATE))
    {
        status = fbe_provision_drive_background_zero_update_paged_metadata(provision_drive_p, packet_p);
    }
    else
    {
        status = fbe_provision_drive_verify_invalidate_update_paged_metadata(provision_drive_p, packet_p);
    }
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * end fbe_provision_drive_background_zero_issue_write_same_completion()
 ******************************************************************************/

/*!****************************************************************************
 * @fn fbe_provision_drive_background_zero_update_paged_metadata_callback()
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
 *  12/02/2013 - Created. Lili Chen
 *
 ******************************************************************************/
fbe_status_t 
fbe_provision_drive_background_zero_update_paged_metadata_callback(fbe_packet_t *packet, fbe_metadata_callback_context_t context)
{
    fbe_payload_ex_t * sep_payload = fbe_transport_get_payload_ex(packet);
    fbe_payload_metadata_operation_t * mdo = NULL;
    fbe_provision_drive_paged_metadata_t * paged_metadata_p;
    fbe_sg_element_t * sg_list = NULL;
    fbe_sg_element_t * sg_ptr = NULL;
    fbe_lba_t lba_offset;
    fbe_u64_t slot_offset;
    fbe_provision_drive_t * provision_drive_p = NULL;

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
    provision_drive_p = (fbe_provision_drive_t *)fbe_base_config_metadata_element_to_base_config_object(mdo->metadata_element);

    while ((sg_ptr->address != NULL) && (mdo->u.metadata_callback.current_count_private < mdo->u.metadata_callback.repeat_count))
    {
        /* set the consumed user bit if user zero bit is set for this chunk. */
        if (paged_metadata_p->user_zero_bit)
        {
            paged_metadata_p->consumed_user_data_bit = FBE_TRUE;
        }

        /* clear the need zero bit if it is set. */
        if (fbe_provision_drive_is_unmap_supported(provision_drive_p))
        {
            paged_metadata_p->need_zero_bit = FBE_TRUE;
        } 
        else
        {
            paged_metadata_p->need_zero_bit = FBE_FALSE; 
        }

        /* clear the user zero bit if it is set. */
        paged_metadata_p->user_zero_bit = FBE_FALSE;

        mdo->u.metadata_callback.current_count_private++;
        paged_metadata_p++;
        if (paged_metadata_p == (fbe_provision_drive_paged_metadata_t *)(sg_ptr->address + FBE_METADATA_BLOCK_DATA_SIZE))
        {
            sg_ptr++;
            paged_metadata_p = (fbe_provision_drive_paged_metadata_t *)sg_ptr->address;
        }
    }

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * end fbe_provision_drive_background_zero_update_paged_metadata_callback()
 ******************************************************************************/

/*!****************************************************************************
 * @fn fbe_provision_drive_background_zero_update_paged_metadata()
 ******************************************************************************
 * @brief
 *
 *  This function is used to to update the paged metadata after background zero
 *  operation, after the end of this background 
 * 
 * @param memory_request    - Pointer to memory request.
 * @param context           - Pointer to the provision drive object.
 *
 * @return fbe_status_t.
 *  21/06/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_background_zero_update_paged_metadata(fbe_provision_drive_t * provision_drive_p,
                                                          fbe_packet_t * packet_p)
{
    fbe_payload_metadata_operation_t *                  metadata_operation_p = NULL;    
    fbe_payload_block_operation_t *                     block_operation_p = NULL;
    fbe_payload_ex_t *                                 sep_payload_p = NULL;
    fbe_status_t                                        status = FBE_STATUS_OK;
    fbe_u64_t                                           metadata_offset;
    fbe_lba_t                                           start_lba;
    fbe_block_count_t                                   block_count;
    fbe_chunk_index_t                                   start_chunk_index;
    fbe_chunk_count_t                                   chunk_count = 0;
    fbe_lba_t           metadata_start_lba;
    fbe_block_count_t   metadata_block_count;
    fbe_chunk_index_t   bitmap_chunk_entry_per_page;
    fbe_u32_t           number_of_clients;
    fbe_block_transport_server_t *block_transport_server_p = &provision_drive_p->base_config.block_transport_server;
    fbe_bool_t          is_last_chunk_of_disk = FBE_FALSE;
    fbe_chunk_index_t   last_chunk_entry_of_disk;
    fbe_lba_t           exported_capacity;
    fbe_payload_block_operation_opcode_t        block_operation_opcode;
    fbe_provision_drive_config_type_t           config_type;


    /* Get the payload and metadata operation. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    metadata_operation_p =  fbe_payload_ex_get_metadata_operation(sep_payload_p);

    /* get the block operation */
    block_operation_p =  fbe_payload_ex_get_present_block_operation(sep_payload_p);

    /* get the start lba and block count. */
    fbe_payload_block_get_lba(block_operation_p, &start_lba);
    fbe_payload_block_get_block_count(block_operation_p, &block_count);

    /* Get the start chunk index and chunk count from the start lba and end lba of the range. */
    fbe_provision_drive_utils_calculate_chunk_range(start_lba,
                                                    block_count,
                                                    FBE_PROVISION_DRIVE_CHUNK_SIZE,
                                                    &start_chunk_index,
                                                    &chunk_count);

    /* Release the existing read metadata operation before we send a request for paged metadata update. */
    fbe_payload_ex_release_metadata_operation(sep_payload_p, metadata_operation_p);

    /* get the metadata offset for the chuk index. */
    metadata_offset = start_chunk_index * sizeof(fbe_provision_drive_paged_metadata_t);

    /* allocate new metadata operation to update the paged metadata. */
    metadata_operation_p =  fbe_payload_ex_allocate_metadata_operation(sep_payload_p); 

    /* Set the completion function before issuing paged metadata xor operation. */
    fbe_transport_set_completion_function(packet_p,
                                          fbe_provision_drive_background_zero_update_paged_metadata_completion,
                                          provision_drive_p);

    /* Build the paged metadata update request. */
    fbe_payload_metadata_build_paged_update(metadata_operation_p,
                                             &(((fbe_base_config_t *) provision_drive_p)->metadata_element),
                                             metadata_offset,
                                             sizeof(fbe_provision_drive_paged_metadata_t),
                                             chunk_count,
                                             fbe_provision_drive_background_zero_update_paged_metadata_callback,
                                             (void *)metadata_operation_p);
    /* Initialize cient_blob, skip the subpacket */
    fbe_provision_drive_metadata_paged_init_client_blob(packet_p, metadata_operation_p);

    /* As per peter, metadata page size is 1 block so align with it to calculate total chunk entries per metadata page size */
    bitmap_chunk_entry_per_page = (FBE_METADATA_BLOCK_DATA_SIZE/sizeof(fbe_provision_drive_paged_metadata_t)) * 1;

    /* get the exported capacity of the provision drive object. */
    fbe_base_config_get_capacity((fbe_base_config_t *) provision_drive_p, &exported_capacity);
    last_chunk_entry_of_disk = (exported_capacity/FBE_PROVISION_DRIVE_CHUNK_SIZE) - 1;

    if(start_chunk_index == last_chunk_entry_of_disk)
    {
        is_last_chunk_of_disk = FBE_TRUE;
    }

    status = fbe_block_transport_server_get_number_of_clients(block_transport_server_p,
                                                              &number_of_clients);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: error getting number of clients status: 0x%x\n", __FUNCTION__, status);
        number_of_clients = -1;
    }

    /* check if there is no client and it is not the last entry in the page and it is not last entry of the disk */
    if((number_of_clients == 0) && ((start_chunk_index + 1) % bitmap_chunk_entry_per_page) && (!is_last_chunk_of_disk))       
    {
        /* set the no persist flag in metadata operation so that metadata will not persist to the disk in the case 
           of unbound drive and paged metadata entry is not the last in page. Metadata will persist when the last entry in page
           updated where we do not set this flag. This change is made to handle disk thrashing issue and improve disk
           zero performance. */
        metadata_operation_p->u.metadata.operation_flags = FBE_PAYLOAD_METADATA_OPERATION_FLAGS_NO_PERSIST;        

    }
    fbe_payload_block_get_opcode(block_operation_p, &block_operation_opcode);
    fbe_provision_drive_get_config_type(provision_drive_p, &config_type);
    /* If there is no client and bgz operation, we should change BGZ cache only */
    if ((number_of_clients == 0) && (config_type == FBE_PROVISION_DRIVE_CONFIG_TYPE_UNCONSUMED) && 
        (block_operation_opcode != FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_INVALIDATE))
    {
        fbe_payload_metadata_set_metadata_operation_flags(metadata_operation_p, FBE_PAYLOAD_METADATA_OPERATION_PAGED_USE_BGZ_CACHE);
    }

	fbe_metadata_paged_get_lock_range(metadata_operation_p, &metadata_start_lba, &metadata_block_count);

    fbe_payload_metadata_set_metadata_stripe_offset(metadata_operation_p, metadata_start_lba);
    fbe_payload_metadata_set_metadata_stripe_count(metadata_operation_p, metadata_block_count);

    fbe_payload_ex_increment_metadata_operation_level(sep_payload_p);
    status =  fbe_metadata_operation_entry(packet_p);
    return status;
}
/******************************************************************************
 * end fbe_provision_drive_background_zero_update_paged_metadata()
 ******************************************************************************/

/*!****************************************************************************
 *  fbe_provision_drive_background_zero_update_paged_metadata_completion()
 ******************************************************************************
 * @brief
 *  This function handles the completion of paged metadata update request for
 *  the background zero request.
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
fbe_provision_drive_background_zero_update_paged_metadata_completion(fbe_packet_t * packet_p,
                                                                     fbe_packet_completion_context_t context)
{
    fbe_provision_drive_t *             provision_drive_p = NULL;
    fbe_payload_metadata_operation_t *  metadata_operation_p = NULL;
    fbe_payload_block_operation_t *     block_operation_p = NULL;
    fbe_payload_ex_t *                 sep_payload_p = NULL;
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_payload_metadata_status_t       metadata_status;
    fbe_u64_t                           metadata_offset;
    fbe_payload_metadata_operation_opcode_t metadata_opcode;

    
    provision_drive_p = (fbe_provision_drive_t *) context;

    /* Get the payload, metadata operation operation. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    metadata_operation_p =  fbe_payload_ex_get_metadata_operation(sep_payload_p);

    /* Get the status of the paged metadata operation. */
    fbe_payload_metadata_get_status(metadata_operation_p, &metadata_status);
    status = fbe_transport_get_status_code(packet_p);

    /* Get the metadata offset */
    fbe_payload_metadata_get_metadata_offset(metadata_operation_p, &metadata_offset);

    fbe_payload_metadata_get_opcode(metadata_operation_p, &metadata_opcode);

    /* Release the metadata operation. */
    fbe_payload_ex_release_metadata_operation(sep_payload_p, metadata_operation_p);

    /* Get the block operation. */
    block_operation_p = fbe_payload_ex_get_present_block_operation(sep_payload_p);

    if((status != FBE_STATUS_OK) ||
       (metadata_status != FBE_PAYLOAD_METADATA_STATUS_OK))
    {
        /*!@todo Add more error handling for the metadata update failure. */
        fbe_provision_drive_utils_trace(provision_drive_p,
                                        FBE_TRACE_LEVEL_WARNING,
                                        FBE_TRACE_MESSAGE_ID_INFO,
                                        FBE_PROVISION_DRIVE_DEBUG_FLAG_ZOD_TRACING,
                                        "BG_ZERO:update metadata failed, offset:0x%llx, md status:0x%x, status:0x%x.\n",
					(unsigned long long)metadata_offset,
					metadata_status, status);

        /* Even non-retryable errors back to the monitor must be accompanied 
         * by a state change. Since we are not guaranteed to be getting a state 
         * change (since it might be caused by a quiesce), we must go ahead and 
         * return this as retryable.  If the downstream object is still accessible
         * the monitor will retry the background zero request.
         */
        fbe_payload_block_set_status(block_operation_p,
                                     FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED,
                                     FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_POSSIBLE);
      
        fbe_transport_set_status(packet_p, status, 0);
        return status;
    }

    /* advance the checkpoint for the background zeroing request. */
    status = fbe_provision_drive_background_zero_advance_checkpoint(provision_drive_p, packet_p);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * end fbe_provision_drive_user_zero_update_paged_metadata_completion()
 ******************************************************************************/

/*!****************************************************************************
 * @fn fbe_provision_drive_background_zero_advance_checkpoint()
 ******************************************************************************
 * @brief
 *  This function is used to start zeroing request by updating the checkpoint
 *  for the zeroing operation.
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
fbe_provision_drive_background_zero_advance_checkpoint(fbe_provision_drive_t * provision_drive_p,
                                                       fbe_packet_t * packet_p)
{
    fbe_status_t                            status;
    fbe_payload_ex_t *                     sep_payload_p = NULL;
    fbe_payload_block_operation_t *         block_operation_p = NULL;
    fbe_lba_t                               start_lba;
    fbe_block_count_t                       block_count;

   /* Get the sep payload and block operation. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_get_present_block_operation(sep_payload_p);

    fbe_payload_block_get_lba(block_operation_p, &start_lba);
    fbe_payload_block_get_block_count(block_operation_p, &block_count);

    /* set the completion function before we update the checkpoint. */
    fbe_transport_set_completion_function(packet_p, fbe_provision_drive_background_zero_advance_checkpoint_completion, provision_drive_p);

    /* update the checkpoint.
     * Note we pass in what we think the checkpoint is, which is the current lba. 
     * If the lba has moved (such as due to a user zero request), then the checkpoint update will fail. 
     */ 
    status = fbe_provision_drive_metadata_incr_background_zero_checkpoint(provision_drive_p, packet_p, start_lba, block_count);
    return status;
}
/******************************************************************************
 * end fbe_provision_drive_background_zero_advance_checkpoint()
 ******************************************************************************/

/*!****************************************************************************
 *  fbe_provision_drive_background_zero_advance_checkpoint_completion()
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
fbe_provision_drive_background_zero_advance_checkpoint_completion(fbe_packet_t * packet_p,
                                                                  fbe_packet_completion_context_t context)
{                      
    fbe_provision_drive_t *             provision_drive_p = NULL;
    fbe_payload_ex_t *                 sep_payload_p = NULL;
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_payload_block_operation_t *     block_operation_p = NULL;
    
    provision_drive_p = (fbe_provision_drive_t *) context;

    /* get the payload and control operation. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    
    /* get the status of the paged metadata operation. */
    status = fbe_transport_get_status_code(packet_p);

   /* based on the metadata status set the appropriate block operation status. */
    block_operation_p =  fbe_payload_ex_get_present_block_operation(sep_payload_p);
    if(status != FBE_STATUS_OK)
    {
        fbe_provision_drive_utils_trace(provision_drive_p,
                                        FBE_TRACE_LEVEL_WARNING,
                                        FBE_TRACE_MESSAGE_ID_INFO,
                                        FBE_PROVISION_DRIVE_DEBUG_FLAG_ZOD_TRACING,
                                        "BG_ZERO:nonpaged metadata update failed.\n");

        fbe_payload_block_set_status(block_operation_p,
                                     FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED,
                                     FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_POSSIBLE);
        fbe_transport_set_status(packet_p, status, 0);
        return status;
    }

    /*!@todo release the nonpaged metadata distributed lock. */

    /* after successful checkpoint update, complete the background zeroing request. */
    fbe_payload_block_set_status(block_operation_p,
                                 FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS,
                                 FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_background_zero_advance_checkpoint_completion()
 ******************************************************************************/


/*!**************************************************************
 * fbe_provision_drive_background_op_is_zeroing_need_to_run()
 ****************************************************************
 * @brief
 *  This function checks zeroing checkpoint and determine if zeroing
 *  background operation needs to be run or not.
 *
 * @param provision_drive_p           - pointer to the provision drive
 *
 * @return bool status - returns FBE_TRUE if the operation needs to run
 *                       otherwise returns FBE_FALSE
 *
 * @author
 *  07/28/2011 - Created. Amit Dhaduk
 *
 ****************************************************************/
fbe_bool_t
fbe_provision_drive_background_op_is_zeroing_need_to_run(fbe_provision_drive_t * provision_drive_p)
{
    fbe_lba_t zero_checkpoint;
    fbe_bool_t                  b_is_enabled;
    fbe_status_t    status;
    fbe_provision_drive_config_type_t config_type;

    fbe_provision_drive_get_config_type(provision_drive_p, &config_type);
    if (config_type == FBE_PROVISION_DRIVE_CONFIG_TYPE_EXT_POOL) {
        return FBE_FALSE;
    }

    /* if Zeroing background operation is disabled in base config, return FALSE so that in _monitor_operation_cond_function, 
     * it can run next priority background operation or if nothing to run, can set upstream edge priority to 
     * FBE_MEDIC_ACTION_IDLE.
     */
    fbe_base_config_is_background_operation_enabled((fbe_base_config_t *)provision_drive_p ,
                                                FBE_PROVISION_DRIVE_BACKGROUND_OP_DISK_ZEROING,& b_is_enabled);
    if( b_is_enabled == FBE_FALSE)
    {
        return FBE_FALSE;
    }

    /* If fail to get zero checkpoint, then return false. */
    status = fbe_provision_drive_metadata_get_background_zero_checkpoint(provision_drive_p, &zero_checkpoint);
    if (status != FBE_STATUS_OK) 
    {
        fbe_provision_drive_utils_trace( provision_drive_p,
                     FBE_TRACE_LEVEL_WARNING,
                     FBE_TRACE_MESSAGE_ID_INFO, 
                     FBE_PROVISION_DRIVE_DEBUG_FLAG_BGZ_TRACING,
                     "pvd_bg_op_is_zeroing_need_to_run: Failed to get checkpoint. Status: 0x%X\n", status);

        return FBE_FALSE;
    }

    /* For now do not drive zeroing during rekey since zeroing needs to know which key to use.
     */
    if (fbe_base_config_is_rekey_mode((fbe_base_config_t *)provision_drive_p)){
        return FBE_FALSE;
    }

    /* check if zeroing checkpoint needs to be driven forward 
     */
    if(zero_checkpoint != FBE_LBA_INVALID)
    {
        return FBE_TRUE;
    }

    return FBE_FALSE;
}
/******************************************************************************
 * end fbe_provision_drive_background_op_is_zeroing_need_to_run()
 ******************************************************************************/
/*!****************************************************************************
 *  fbe_provision_drive_get_next_chunk_to_zero()
 ******************************************************************************
 * @brief
 *  This function will go over the search data and find out if the chunk
 *  is marked for zero or not
 * 
 * @param search_data -       pointer to the search data
 * @param search_size  -      search data size
 * @param provision_drive_p  - pointer to the provision drive object
 *
 * @return fbe_status_t.
 *
 * @author
 *  02/21/2012 - Created. Ashwin Tamilarasan
 *
 ******************************************************************************/
static fbe_bool_t
fbe_provision_drive_get_next_chunk_to_zero(fbe_u8_t*     search_data,
                                           fbe_u32_t    search_size,
                                           context_t    context)
                                           
{
    fbe_provision_drive_t*                   provision_drive_p = NULL;
    fbe_provision_drive_paged_metadata_t     paged_metadata;
    fbe_u8_t*                                source_ptr = NULL;
    fbe_bool_t                               is_chunk_marked_for_processing = FBE_FALSE;
    fbe_u32_t                                index;

    provision_drive_p = (fbe_provision_drive_t*)context;
    source_ptr = search_data;

    for(index = 0; index < search_size; index += sizeof(fbe_provision_drive_paged_metadata_t))
    {
        fbe_copy_memory(&paged_metadata, source_ptr, sizeof(fbe_provision_drive_paged_metadata_t));
        source_ptr += sizeof(fbe_provision_drive_paged_metadata_t);

        /* First make sure the page metadata is valid before processing 
         * The client code needs to look at the paged data valid bit and take
         * appropriate action. All this code tells the client is we have found a region
         * that needs processing
         */
        if(paged_metadata.valid_bit == 0)
        {
            is_chunk_marked_for_processing = FBE_TRUE;
            break;
        }
        else if((paged_metadata.need_zero_bit == 1) || 
               ((paged_metadata.user_zero_bit == 1) && 
                (paged_metadata.consumed_user_data_bit == 1))|| (paged_metadata.user_zero_bit == 1))
        {
            is_chunk_marked_for_processing = FBE_TRUE;
            break;
        }
    }

    return is_chunk_marked_for_processing;
}
/*******************************************************
 * end fbe_provision_drive_get_next_chunk_to_zero()
 ******************************************************/
/*!****************************************************************************
 *  fbe_provision_drive_get_next_invalid_paged_data_chunk()
 ******************************************************************************
 * @brief
 *  This function will go over the search data and find out if the chunk
 *  is  invalid or not
 * 
 * @param search_data -       pointer to the search data
 * @param search_size  -      search data size
 * @param provision_drive_p  - pointer to the provision drive object
 *
 * @return fbe_status_t.
 *
 * @author
 *  03/09/2012 - Created. Ashok Tamilarasan
 *
 ******************************************************************************/
static fbe_bool_t
fbe_provision_drive_get_next_invalid_paged_data_chunk(fbe_u8_t*     search_data,
                                                      fbe_u32_t    search_size,
                                                      context_t    context)
                                           
{
    fbe_provision_drive_t*                   provision_drive_p = NULL;
    fbe_provision_drive_paged_metadata_t     paged_metadata;
    fbe_u8_t*                                source_ptr = NULL;
    fbe_bool_t                               is_chunk_marked_for_processing = FBE_FALSE;
    fbe_u32_t                                index;

    provision_drive_p = (fbe_provision_drive_t*)context;
    source_ptr = search_data;

    for(index = 0; index < search_size; index += sizeof(fbe_provision_drive_paged_metadata_t))
    {
        fbe_copy_memory(&paged_metadata, source_ptr, sizeof(fbe_provision_drive_paged_metadata_t));
        source_ptr += sizeof(fbe_provision_drive_paged_metadata_t);

        if(paged_metadata.valid_bit == 0)
        {
            is_chunk_marked_for_processing = FBE_TRUE;
            break;
        }
    }

    return is_chunk_marked_for_processing;

}
/*******************************************************
 * end fbe_provision_drive_get_next_invalid_paged_data_chunk()
 ******************************************************/

/********************************************
 * end fbe_provision_drive_background_zero.c
 ********************************************/

