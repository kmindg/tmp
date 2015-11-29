/***************************************************************************
 * Copyright (C) EMC Corporation 2014
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_raid_group_reconstruct_paged.c
 ***************************************************************************
 *
 * @brief
 *  This file contains functions related to handling paged errors and
 *  reconstructing the paged.
 * 
 *  This code supports fine grained reconstruct of the paged metadata.
 *  In the case of encryption, this code is needed to make sure that we
 *  mark each chunk as encrypted or not.
 * 
 *  In some cases with encryption we will also verify the user chunks
 *  with the new key to try to determine if it has been rekeyed or not.
 *
 * @version
 *   5/2/2014:  Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe_raid_group_bitmap.h"                      //  this file's .h file  
#include "fbe_raid_group_rebuild.h"                     //  for fbe_raid_group_rebuild_evaluate_nr_chunk_info()
#include "fbe/fbe_raid_group_metadata.h"                //  for fbe_raid_group_paged_metadata_t
#include "fbe_base_config_private.h"                    //  for paged metadata set and get chunk info
#include "fbe_raid_verify.h"                            //  for ask_for_verify_permission
#include "fbe_raid_group_logging.h"                     //  for logging rebuild complete/lun start/lun complete 
#include "fbe_raid_group_object.h"
#include "EmcPAL_Misc.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

static fbe_status_t 
fbe_raid_group_reconstruct_paged_write_completion(fbe_packet_t *packet_p,
                                                  fbe_packet_completion_context_t context);
static fbe_status_t 
fbe_raid_group_reconstruct_paged_md_callback(fbe_packet_t * packet, fbe_metadata_callback_context_t context);

static fbe_status_t fbe_raid_group_bitmap_reconst_paged_np_updated(fbe_packet_t *packet_p,
                                                                   fbe_packet_completion_context_t context);
static fbe_status_t fbe_raid_group_start_encryption_scan(fbe_raid_group_t *raid_group_p,
                                                         fbe_packet_t *packet_p);
static fbe_status_t fbe_raid_group_start_encryption_scan_alloc_completion(fbe_memory_request_t * memory_request_p, 
                                                                          fbe_memory_completion_context_t context);
static fbe_status_t fbe_raid_group_find_rekey_chkpt_callback(fbe_packet_t * packet, fbe_metadata_callback_context_t context);
static fbe_status_t fbe_raid_group_encryption_scan_completion(fbe_packet_t*  packet_p,
                                                                    fbe_packet_completion_context_t context);
static fbe_status_t fbe_raid_group_paged_scan_update_chkpt_completion(fbe_packet_t*  packet_p,
                                                                    fbe_packet_completion_context_t context);

static fbe_status_t fbe_raid_group_reconstruct_paged_with_callback(fbe_raid_group_t *raid_group_p, fbe_packet_t *packet_p);
static fbe_status_t fbe_raid_group_bitmap_reconst_paged_np_updated(fbe_packet_t *packet_p,
                                                                   fbe_packet_completion_context_t context);
static fbe_status_t fbe_raid_group_reconst_paged_sm_verify(fbe_raid_group_t *raid_group_p,
                                                           fbe_packet_t *packet_p);

static fbe_status_t fbe_raid_group_reconstruct_paged_md_callback(fbe_packet_t * packet, 
                                                                 fbe_metadata_callback_context_t context);
static fbe_status_t fbe_raid_group_reconstruct_paged_write_completion(fbe_packet_t *packet_p,
                                                                      fbe_packet_completion_context_t context);
static fbe_status_t fbe_raid_group_reconst_paged_sm_verify(fbe_raid_group_t *raid_group_p,
                                                           fbe_packet_t *packet_p);
static fbe_status_t fbe_raid_group_reconst_paged_sm_verify_complete(fbe_packet_t *packet_p,
                                                                    fbe_packet_completion_context_t context);

void fbe_raid_group_reconstruct_paged_set_state(fbe_raid_group_paged_reconstruct_t *reconstruct_context_p,
                                                fbe_raid_group_paged_reconst_state_t state)
{
    reconstruct_context_p->state = state;
}
/*!**************************************************************
 * fbe_raid_group_reconstruct_paged_init()
 ****************************************************************
 * @brief
 *  Initialize our context that is used for reconstructing paged.
 *
 * @param raid_group_p
 * @param reconstruct_context_p
 * @param verify_start
 * @param verify_blocks 
 *
 * @return None.
 *
 * @author
 *  5/13/2014 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_raid_group_reconstruct_paged_init(fbe_raid_group_t *raid_group_p,
                                           fbe_raid_group_paged_reconstruct_t *reconstruct_context_p,
                                           fbe_lba_t verify_start,
                                           fbe_lba_t verify_blocks)
{
    fbe_lba_t rekey_chkpt;
    fbe_raid_group_reconstruct_paged_set_state(reconstruct_context_p, FBE_RAID_GROUP_RECONST_STATE_SCAN_PAGED);

    reconstruct_context_p->first_invalid_chunk_index = FBE_LBA_INVALID;
    reconstruct_context_p->last_invalid_chunk_index = FBE_LBA_INVALID;

    rekey_chkpt = fbe_raid_group_get_rekey_checkpoint(raid_group_p);
    reconstruct_context_p->orig_rekey_chkpt = rekey_chkpt;
    if (rekey_chkpt == 0) {
        /* We have not started yet, so we leave last rekeyed chunk invalid until we find one that is rekeyed.
         */
        reconstruct_context_p->last_rekeyed_chunk_index = FBE_LBA_INVALID;
    } else {
        fbe_raid_group_bitmap_get_chunk_index_for_lba(raid_group_p, rekey_chkpt, &reconstruct_context_p->last_rekeyed_chunk_index);
        /* Since the checkpoint represents the next chunk to rekey, subtract one from this to get the last rekeyed 
         * chunk. 
         */
        reconstruct_context_p->last_rekeyed_chunk_index--;
    }
    reconstruct_context_p->verify_start = verify_start;
    reconstruct_context_p->verify_blocks = verify_blocks;
}
/******************************************
 * end fbe_raid_group_reconstruct_paged_init()
 ******************************************/

/*!**************************************************************
 * fbe_raid_group_encrypt_reconstruct_sm_start()
 ****************************************************************
 * @brief
 *   Kick off the state machine.  We will allocate and initialize
 *   our context and kick the state machine.
 * 
 * @param raid_group_p
 * @param packet_p
 * @param verify_start
 * @param verify_blocks
 * 
 * @return fbe_status_t
 *
 * @author
 *  5/13/2014 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_raid_group_encrypt_reconstruct_sm_start(fbe_raid_group_t *raid_group_p,
                                                         fbe_packet_t *packet_p,
                                                         fbe_lba_t verify_start,
                                                         fbe_lba_t verify_blocks)
{
    fbe_raid_group_bg_op_info_t *bg_op_p = NULL;
    fbe_raid_group_paged_reconstruct_t *reconstruct_context_p = NULL;

    bg_op_p = fbe_raid_group_get_bg_op_ptr(raid_group_p);

    /* We need a tracking structure, so we will use the bg op's native allocation.
     */
    if (bg_op_p == NULL) {
        bg_op_p = fbe_memory_native_allocate(sizeof(fbe_raid_group_bg_op_info_t));
        if (bg_op_p == NULL) {
            fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s unable to allocate %u bytes, retry\n",
                                  __FUNCTION__, (fbe_u32_t)sizeof(fbe_raid_group_bg_op_info_t));
            return FBE_STATUS_MORE_PROCESSING_REQUIRED;
        }
        fbe_zero_memory(bg_op_p, sizeof(fbe_raid_group_bg_op_info_t));
        fbe_raid_group_set_bg_op_ptr(raid_group_p, bg_op_p);
    }
    reconstruct_context_p = fbe_raid_group_get_bg_op_reconstruct_context(raid_group_p);

    if (!fbe_raid_group_bg_op_is_flag_set(raid_group_p, FBE_RAID_GROUP_BG_OP_FLAG_RECONSTRUCT_PAGED)) {

        fbe_raid_group_reconstruct_paged_init(raid_group_p, reconstruct_context_p, verify_start, verify_blocks);

        fbe_raid_group_bg_op_set_flag(raid_group_p, FBE_RAID_GROUP_BG_OP_FLAG_RECONSTRUCT_PAGED);
    }

    fbe_raid_group_encrypt_reconstruct_sm(raid_group_p, packet_p);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************
 * end fbe_raid_group_encrypt_reconstruct_sm_start()
 ******************************************/

/*!**************************************************************
 * fbe_raid_group_encrypt_reconstruct_sm()
 ****************************************************************
 * @brief
 *  Execute the state machine that is used for reconstructing paged.
 *
 * @param raid_group_p
 * @param packet_p
 * 
 * @return fbe_status_t
 *
 * @author
 *  5/13/2014 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_raid_group_encrypt_reconstruct_sm(fbe_raid_group_t *raid_group_p,
                                                   fbe_packet_t *packet_p)
{
    fbe_raid_group_paged_reconstruct_t *reconstruct_context_p = NULL;

    reconstruct_context_p = fbe_raid_group_get_bg_op_reconstruct_context(raid_group_p);

    switch (reconstruct_context_p->state) {
        
        case FBE_RAID_GROUP_RECONST_STATE_SCAN_PAGED:
            fbe_raid_group_start_encryption_scan(raid_group_p, packet_p);
            break;

        case FBE_RAID_GROUP_RECONST_STATE_VERIFY:
            fbe_raid_group_reconst_paged_sm_verify(raid_group_p, packet_p);
            break;

        case FBE_RAID_GROUP_RECONST_STATE_RECONSTRUCT_PAGED:
            fbe_raid_group_reconstruct_paged_with_callback(raid_group_p, packet_p);
            break;
        case FBE_RAID_GROUP_RECONST_STATE_DONE:
            fbe_raid_group_reconstruct_paged_set_state(reconstruct_context_p, FBE_RAID_GROUP_RECONST_STATE_INVALID);
            fbe_raid_group_bg_op_clear_flag(raid_group_p, FBE_RAID_GROUP_BG_OP_FLAG_RECONSTRUCT_PAGED);
            fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
            fbe_transport_complete_packet(packet_p);
            break;
        default:
            fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s unexpected state 0x%llx packet: %p\n",
                                  __FUNCTION__, reconstruct_context_p->state, packet_p);
            fbe_raid_group_reconstruct_paged_set_state(reconstruct_context_p, FBE_RAID_GROUP_RECONST_STATE_DONE);
            fbe_transport_complete_packet(packet_p);
            break;
    };

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************
 * end fbe_raid_group_encrypt_reconstruct_sm()
 ******************************************/

/*!**************************************************************
 * fbe_raid_group_start_encryption_scan()
 ****************************************************************
 * @brief
 *  Allocate memory for the subpacket and blob for the encryption scan.
 *
 * @param raid_group_p
 * @param packet_p
 * 
 * @return fbe_status_t
 *
 * @author
 *  5/2/2014 - Created. Rob Foley
 *  
 ****************************************************************/

fbe_status_t fbe_raid_group_start_encryption_scan(fbe_raid_group_t *raid_group_p, 
                                                  fbe_packet_t *packet_p)
{
    fbe_memory_request_t*                memory_request_p = NULL;
    fbe_status_t                         status;

    memory_request_p = fbe_transport_get_memory_request(packet_p);

    status = fbe_memory_build_chunk_request(memory_request_p,
                                            FBE_MEMORY_CHUNK_SIZE_64_BLOCKS_64_BLOCKS,
                                            2,    /* One for sg and packet */
                                            0, /* Priority */
                                            0, /* Io stamp*/
                                            (fbe_memory_completion_function_t)fbe_raid_group_start_encryption_scan_alloc_completion,
                                            raid_group_p);
    if (status == FBE_STATUS_GENERIC_FAILURE) {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_CRITICAL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "reconst sm: Setup for memory allocation failed, status:0x%x, line:%d\n", status, __LINE__);  
        return FBE_STATUS_OK;
    }

    fbe_memory_request_entry(memory_request_p);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/**************************************
 * end fbe_raid_group_start_encryption_scan
 **************************************/

/*!**************************************************************
 * fbe_raid_group_start_encryption_scan_alloc_completion()
 ****************************************************************
 * @brief
 *  Setup the sg list, blob, and subpacket.  Start the scan of paged
 *  to find the encryption checkpoint.
 *
 * Send the operation to invalidate the chunk stripe.
 * 
 * @param memory_request_p
 * @param context
 * 
 * @return fbe_status_t
 *
 * @author
 *  5/2/2014 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t 
fbe_raid_group_start_encryption_scan_alloc_completion(fbe_memory_request_t * memory_request_p, 
                                                      fbe_memory_completion_context_t context)
{
    fbe_status_t status;
    fbe_packet_t *master_packet_p = NULL;
    fbe_packet_t *packet_p = NULL;
    fbe_payload_ex_t *payload_p = NULL;
    fbe_raid_group_t *raid_group_p = (fbe_raid_group_t *)context;
    fbe_memory_header_t *memory_header_p = NULL;
    fbe_memory_header_t *next_memory_header_p = NULL;
    fbe_sg_element_t *sg_p = NULL;
    fbe_u8_t *data_p = NULL;
    fbe_cpu_id_t cpu_id;
    fbe_lba_t start_lba;
    fbe_lba_t capacity;
    fbe_object_id_t raid_group_object_id;
    fbe_u32_t encrypt_blob_blocks;
    fbe_u32_t bytes_per_buffer = FBE_MEMORY_CHUNK_SIZE_BLOCK_COUNT_64 * FBE_BE_BYTES_PER_BLOCK;
    fbe_u64_t metadata_offset;
    fbe_chunk_index_t chunk_index;
    fbe_chunk_index_t end_chunk_index;
    fbe_chunk_count_t chunk_count;
    fbe_payload_metadata_operation_t *metadata_operation_p = NULL;

    start_lba = fbe_raid_group_get_rekey_checkpoint(raid_group_p);
    capacity = fbe_raid_group_get_disk_capacity(raid_group_p);

    master_packet_p = fbe_transport_memory_request_to_packet(memory_request_p);

    if (fbe_memory_request_is_allocation_complete(memory_request_p) == FBE_FALSE){
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s allocation failed packet: %p\n", __FUNCTION__, packet_p);
        fbe_memory_free_request_entry(memory_request_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }
    memory_header_p = fbe_memory_get_first_memory_header(memory_request_p);
    data_p = (fbe_u8_t*)fbe_memory_header_to_data(memory_header_p);
    packet_p = (fbe_packet_t*)data_p;
    data_p += FBE_MEMORY_CHUNK_SIZE;
    sg_p = (fbe_sg_element_t*)data_p;
    data_p += sizeof(fbe_sg_element_t) * 3; // Allow for 2 sgs and one terminator.

    fbe_sg_element_init(&sg_p[0], bytes_per_buffer - (sizeof(fbe_sg_element_t) * 3) - FBE_MEMORY_CHUNK_SIZE, data_p);
    fbe_memory_get_next_memory_header(memory_header_p, &next_memory_header_p);
    data_p = (fbe_u8_t*)fbe_memory_header_to_data(next_memory_header_p);
    fbe_sg_element_init(&sg_p[1], FBE_MEMORY_CHUNK_SIZE_BLOCK_COUNT_64 * FBE_BE_BYTES_PER_BLOCK, data_p);
    fbe_sg_element_terminate(&sg_p[2]);

    encrypt_blob_blocks = fbe_raid_group_get_encrypt_blob_blocks(raid_group_p);

    if ( encrypt_blob_blocks < FBE_MEMORY_CHUNK_SIZE_BLOCK_COUNT_64 ) {
        fbe_sg_element_init(&sg_p[0], (encrypt_blob_blocks * FBE_BE_BYTES_PER_BLOCK), data_p);
        fbe_sg_element_terminate(&sg_p[1]);
    }

    fbe_transport_initialize_packet(packet_p);

    fbe_base_object_get_object_id((fbe_base_object_t *) raid_group_p, &raid_group_object_id);
    fbe_transport_set_address(packet_p,
                              FBE_PACKAGE_ID_SEP_0,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              raid_group_object_id);
    fbe_transport_add_subpacket(master_packet_p, packet_p);
    payload_p = fbe_transport_get_payload_ex(packet_p);

    cpu_id = (memory_request_p->io_stamp & FBE_PACKET_IO_STAMP_MASK) >> FBE_PACKET_IO_STAMP_SHIFT;
    fbe_transport_set_cpu_id(packet_p, cpu_id);

    fbe_transport_set_physical_drive_io_stamp(packet_p, fbe_get_time());
    fbe_payload_ex_set_sg_list(payload_p, sg_p, 0);

    fbe_transport_set_completion_function(packet_p, 
                                          fbe_raid_group_encryption_scan_completion,
                                          (fbe_packet_completion_context_t) raid_group_p);

    fbe_raid_group_bitmap_get_chunk_index_for_lba(raid_group_p, start_lba, &chunk_index);
    fbe_raid_group_bitmap_get_chunk_index_for_lba(raid_group_p, capacity - 1, &end_chunk_index);
    chunk_count = (fbe_chunk_count_t)(end_chunk_index - chunk_index);

    /* Get the offset of this chunk in the metadata (ie. where we'll start to write to).
     */
    metadata_offset = chunk_index * sizeof(fbe_raid_group_paged_metadata_t);

    metadata_operation_p = fbe_payload_ex_allocate_metadata_operation(payload_p);

    /* We use a paged metadata update, but we never actually return the status 
     * that causes the paged to be read, so in affect this is a scan. 
     * We use write-verify flavor since it continues on invalidated blocks. 
     */
    fbe_payload_metadata_build_paged_write_verify_update(metadata_operation_p,
                                                         &(((fbe_base_config_t *) raid_group_p)->metadata_element),
                                                         metadata_offset,
                                                         sizeof(fbe_raid_group_paged_metadata_t),
                                                         chunk_count,
                                                         fbe_raid_group_find_rekey_chkpt_callback,
                                                         (void *)raid_group_p);

    fbe_payload_ex_get_sg_list(payload_p, &sg_p, NULL);
    if (sg_p) {
        status = fbe_metadata_paged_init_client_blob_with_sg_list(sg_p, metadata_operation_p);
    }

    fbe_raid_group_bitmap_metadata_populate_stripe_lock(raid_group_p,
                                                        metadata_operation_p,
                                                        chunk_count);

    fbe_transport_set_packet_attr(packet_p, FBE_PACKET_FLAG_DO_NOT_CANCEL);  

    fbe_payload_ex_increment_metadata_operation_level(payload_p);
    /* Issue the metadata operation to write the paged metadata chunk.
     */
    status = fbe_metadata_operation_entry(packet_p);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/**************************************
 * end fbe_raid_group_start_encryption_scan_alloc_completion()
 **************************************/

/*!****************************************************************************
 *  fbe_raid_group_find_rekey_chkpt_callback()
 ******************************************************************************
 * @brief
 *  The purpose of this callback is to help find the rekey checkpoint.
 *  - If the chunk is not rekeyed, we are done since this is the next chunk to get rekeyed.
 *  - If the chunk is not valid (it was lost), then we note this in our context
 *    and continue scanning for either rekeyed (we continue scanning still)
 *        or not rekeyed (we reached the end, but the user area in between the last "rekeyed"
 *                        chunk and this final "not rekeyed" chunk needs to be manually
 *                        checked with a verify to determine if it was rekeyed or not).
 *  - Yet another final state is finding a chunk that has an incomplete write.
 *    In this case we can invalidate the chunk as usual and let the cache flush the
 *    previously pinned data.
 * 
 * @param sg_list           - Pointer to sg list.
 * @param slot_offset       - Offset to the paged metadata.
 * @param context           - Pointer to context.
 *
 * @return fbe_status_t.
 *
 * @author
 *  5/5/2014 - Created. Rob Foley
 *
 ******************************************************************************/
static fbe_status_t 
fbe_raid_group_find_rekey_chkpt_callback(fbe_packet_t * packet, fbe_metadata_callback_context_t context)
{
    fbe_payload_ex_t * sep_payload = fbe_transport_get_payload_ex(packet);
    fbe_payload_metadata_operation_t * mdo = NULL;
    fbe_raid_group_paged_metadata_t * paged_metadata_p;
    fbe_sg_element_t * sg_list = NULL;
    fbe_sg_element_t * sg_ptr = NULL;
    fbe_lba_t lba_offset;
    fbe_u64_t slot_offset;
    fbe_status_t status = FBE_STATUS_OK; /* indicates nothing needs change */
    fbe_raid_group_t * raid_group_p = (fbe_raid_group_t *) context;
    fbe_chunk_index_t chunk_index;
    fbe_raid_group_paged_reconstruct_t *reconstruct_context_p = NULL;

    reconstruct_context_p = fbe_raid_group_get_bg_op_reconstruct_context(raid_group_p);
     
    if(sep_payload->current_operation->payload_opcode == FBE_PAYLOAD_OPCODE_METADATA_OPERATION){
        mdo = fbe_payload_ex_get_metadata_operation(sep_payload);
    } else if(sep_payload->current_operation->payload_opcode == FBE_PAYLOAD_OPCODE_STRIPE_LOCK_OPERATION){
        mdo = fbe_payload_ex_get_any_metadata_operation(sep_payload);
    } else {
        fbe_topology_class_trace(FBE_CLASS_ID_RAID_GROUP, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                      "%s: Invalid payload opcode:%d \n", __FUNCTION__, sep_payload->current_operation->payload_opcode);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    lba_offset = fbe_metadata_paged_get_lba_offset(mdo);
    slot_offset = fbe_metadata_paged_get_slot_offset(mdo);
    chunk_index = fbe_metadata_paged_get_record_offset(mdo);
    sg_list = mdo->u.metadata_callback.sg_list;
    sg_ptr = &sg_list[lba_offset - mdo->u.metadata_callback.start_lba];
    paged_metadata_p = (fbe_raid_group_paged_metadata_t *)(sg_ptr->address + slot_offset);

    while ((sg_ptr->address != NULL) && 
           (mdo->u.metadata_callback.current_count_private < mdo->u.metadata_callback.repeat_count)) {
        fbe_raid_group_trace(raid_group_p, FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_RAID_GROUP_DEBUG_FLAG_VERIFY_TRACING,
                             "reconst sm: find rekey chkpt callback for chnk_idx: 0x%llx v: %d vr: 0x%x rk: %d nr: 0x%x\n", 
                             chunk_index, paged_metadata_p->valid_bit, paged_metadata_p->verify_bits, 
                             paged_metadata_p->rekey, paged_metadata_p->needs_rebuild_bits);

        if (paged_metadata_p->valid_bit == 0) {
          /* Invalid chunk.  Just remember it.
           */
            if (reconstruct_context_p->first_invalid_chunk_index == FBE_LBA_INVALID) {
                reconstruct_context_p->first_invalid_chunk_index = chunk_index;
            }
            reconstruct_context_p->last_invalid_chunk_index = chunk_index;
            fbe_raid_group_trace(raid_group_p, FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_RAID_GROUP_DEBUG_FLAG_VERIFY_TRACING,
                                  "reconst sm: idx: 0x%llx invalid chunk first: 0x%llx last: 0x%llx\n",
                                  chunk_index,
                                  reconstruct_context_p->first_invalid_chunk_index, 
                                  reconstruct_context_p->last_invalid_chunk_index);
        } else if (paged_metadata_p->rekey == 0) {
            /* If the rekey is CLEAR then we are done with the scan, since
             * we found something that is not yet rekeyed. 
             * Remember this chunk index since we might need to do a scan with verify up to this point if 
             * invalid chunks were found. 
             */
            reconstruct_context_p->first_non_rekeyed_chunk_index = chunk_index;
            fbe_raid_group_trace(raid_group_p, FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_RAID_GROUP_DEBUG_FLAG_VERIFY_TRACING,
                                  "reconst sm: idx: 0x%llx non rekey chunk first: 0x%llx last: 0x%llx\n",
                                  chunk_index,
                                  reconstruct_context_p->first_invalid_chunk_index, 
                                  reconstruct_context_p->last_invalid_chunk_index);
        } else if (paged_metadata_p->rekey == 1) {
            /* If the rekey is SET then we can reset the invalid chunks.
             */
            reconstruct_context_p->first_invalid_chunk_index = FBE_LBA_INVALID;
            reconstruct_context_p->last_invalid_chunk_index = FBE_LBA_INVALID;
            reconstruct_context_p->last_rekeyed_chunk_index = chunk_index;
            fbe_raid_group_trace(raid_group_p, FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_RAID_GROUP_DEBUG_FLAG_VERIFY_TRACING,
                                  "reconst sm: idx: 0x%llx  rekeyed chunk first: 0x%llx last: 0x%llx\n",
                                  chunk_index,
                                  reconstruct_context_p->first_invalid_chunk_index, 
                                  reconstruct_context_p->last_invalid_chunk_index);
        }

        mdo->u.metadata_callback.current_count_private++;
        chunk_index++;
        paged_metadata_p++;
        if (paged_metadata_p == (fbe_raid_group_paged_metadata_t *)(sg_ptr->address + FBE_METADATA_BLOCK_DATA_SIZE))
        {
            sg_ptr++;
            paged_metadata_p = (fbe_raid_group_paged_metadata_t *)sg_ptr->address;
        }
    }

    return status;
}
/******************************************************************************
 * end fbe_raid_group_find_rekey_chkpt_callback()
 ******************************************************************************/
/*!****************************************************************************
 * fbe_raid_group_encryption_scan_completion()
 ******************************************************************************
 * @brief
 *  The scan of paged is now complete.
 *  Based on this, we can set the tentative checkpoint.
 *  From that checkpoint we either start to verify user chunks to find the status of each
 *  user chunk or we simply continue with the paged reconstruct.
 * 
 * @param packet_p - pointer to the packet
 * @param context  - completion context
 *
 * @return  fbe_status_t            
 *
 * @author
 *  12/18/2013 - Created. Rob Foley
 *
 ******************************************************************************/
static fbe_status_t fbe_raid_group_encryption_scan_completion(fbe_packet_t*  packet_p,
                                                              fbe_packet_completion_context_t context)
{
    fbe_raid_group_t*                       raid_group_p = (fbe_raid_group_t*)context;
    fbe_payload_ex_t*                       sep_payload_p = NULL;
    fbe_status_t                            status;
    fbe_block_count_t                       block_count;
    fbe_payload_metadata_status_t           metadata_status;
    fbe_packet_t                           *master_packet_p = NULL;
    fbe_memory_request_t                   *memory_request_p = NULL;
    fbe_lba_t                              new_rekey_checkpoint;
    fbe_raid_group_paged_reconstruct_t    *reconstruct_context_p = NULL;
    fbe_payload_metadata_operation_t      *metadata_operation_p = NULL;
    fbe_lba_t                              rekey_chkpt;
    fbe_chunk_size_t                       chunk_size;

    rekey_chkpt = fbe_raid_group_get_rekey_checkpoint(raid_group_p);
    chunk_size = fbe_raid_group_get_chunk_size(raid_group_p);

    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    metadata_operation_p = fbe_payload_ex_get_metadata_operation(sep_payload_p);

    master_packet_p = fbe_transport_get_master_packet(packet_p);
    memory_request_p = fbe_transport_get_memory_request(master_packet_p);

    fbe_transport_remove_subpacket(packet_p);
    status = fbe_transport_get_status_code(packet_p);
    fbe_transport_set_status(master_packet_p, status, 0);

    /* destroy subpacket and free up the memory we previously allocated for the scan.
     */
    fbe_transport_destroy_subpackets(master_packet_p);
    fbe_memory_free_request_entry(memory_request_p);

    fbe_payload_metadata_get_status(metadata_operation_p, &metadata_status);
    if ((metadata_status != FBE_PAYLOAD_METADATA_STATUS_OK) || 
        (status != FBE_STATUS_OK)){
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                              "reconst sm: scan failed metadata_status: 0x%x packet status: 0x%x \n",
                              metadata_status, status);
        fbe_transport_set_status(master_packet_p, FBE_STATUS_FAILED, 0);
        fbe_transport_complete_packet(master_packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    reconstruct_context_p = fbe_raid_group_get_bg_op_reconstruct_context(raid_group_p);

    fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                          "reconst sm: chkpt:0 %llx last_rk_ci: %llx frst/lsti_ci: %llx/%llx fnonrk_ci: %llx\n",
                          (unsigned long long)rekey_chkpt,
                          (unsigned long long)reconstruct_context_p->last_rekeyed_chunk_index, 
                          (unsigned long long)reconstruct_context_p->first_invalid_chunk_index, 
                          (unsigned long long)reconstruct_context_p->last_invalid_chunk_index, 
                          (unsigned long long)reconstruct_context_p->first_non_rekeyed_chunk_index);

    new_rekey_checkpoint = rekey_chkpt;
    if (reconstruct_context_p->last_rekeyed_chunk_index != FBE_LBA_INVALID) {
        new_rekey_checkpoint = (reconstruct_context_p->last_rekeyed_chunk_index + 1) * chunk_size;
    }
    
    /* If the new checkpoint is greater than the current lba just move the checkpoint.
     * We will come back through here to set the next state once the checkpoint is moved. 
     */
    if (new_rekey_checkpoint > rekey_chkpt) {

        fbe_transport_set_completion_function(master_packet_p, fbe_raid_group_paged_scan_update_chkpt_completion, raid_group_p);  

        block_count = new_rekey_checkpoint - rekey_chkpt;  

        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "reconst sm: move chkpt current_chkpt:0x%llx,bl:0x%llx new_chkpt:0x%llx\n",
                              (unsigned long long)rekey_chkpt, (unsigned long long)block_count, (unsigned long long)new_rekey_checkpoint);

        status = fbe_raid_group_update_rekey_checkpoint(master_packet_p, raid_group_p, rekey_chkpt, block_count, __FUNCTION__);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;        
    }

    fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                          "reconst sm: scan complete current_chkpt:0x%llx\n",
                          (unsigned long long)rekey_chkpt);

    /* Now that the scan is complete, determine if we need to issue verify to user areas 
     * to decide if user areas are encrypted yet or not. 
     */
    if (reconstruct_context_p->first_invalid_chunk_index != FBE_LBA_INVALID) {
        /* There were lost chunks. 
         * By this point our checkpoint is set to the last "good" and "rekeyed" chunk. 
         * Launch a verify to decide if this chunk is valid with new key. 
         */
        fbe_raid_group_reconstruct_paged_set_state(reconstruct_context_p, FBE_RAID_GROUP_RECONST_STATE_VERIFY);
    } else {
        /* No lost chunks.  Just reconstruct the paged taking into account the valid checkpoint.
         */
        fbe_raid_group_reconstruct_paged_set_state(reconstruct_context_p, FBE_RAID_GROUP_RECONST_STATE_RECONSTRUCT_PAGED);
    }
    fbe_transport_complete_packet(master_packet_p);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/**************************************
 * end fbe_raid_group_encryption_scan_completion()
 **************************************/
/*!****************************************************************************
 * fbe_raid_group_paged_scan_update_chkpt_completion()
 ******************************************************************************
 * @brief
 *  When the checkpoint set completes, cleanup and reschedule monitor.
 * 
 * @param packet_p - pointer to the packet
 * @param context  - completion context
 *
 * @return  fbe_status_t            
 *
 * @author
 *  4/25/2014 - Created. Rob Foley
 *
 ******************************************************************************/
static fbe_status_t fbe_raid_group_paged_scan_update_chkpt_completion(fbe_packet_t*  packet_p,
                                                                    fbe_packet_completion_context_t context)
{
    fbe_raid_group_t *raid_group_p = (fbe_raid_group_t*)context;

    /* Reschedule immediately so the check proceeds quickly.
     */
    fbe_lifecycle_reschedule(&fbe_raid_group_lifecycle_const, (fbe_base_object_t*) raid_group_p, 10);

    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_raid_group_paged_scan_update_chkpt_completion()
 **************************************/
/*!**************************************************************
 * fbe_raid_group_reconst_paged_sm_verify()
 ****************************************************************
 * @brief
 *  Kick off the verify of user area with the new key.
 *  The purpose of the verify is to decide if these chunks are
 *  using the new key or the old key.
 *  - If we see many multi-bit crc errors with the new key then we
 *  will simply stop, leave the rekey checkpoint as is and allow
 *  the encryption to start from here. 
 *
 *  - If we see that the verify is successful, then we advance the
 *    checkpoint and verify the next area.
 *     
 * @param raid_group_p
 * @param packet_p
 * 
 * @return fbe_status_t
 *
 * @author
 *  5/5/2014 - Created. Rob Foley
 *
 ****************************************************************/
static fbe_status_t fbe_raid_group_reconst_paged_sm_verify(fbe_raid_group_t *raid_group_p,
                                                           fbe_packet_t *packet_p)
{
    fbe_lba_t start_lba;
    fbe_raid_group_monitor_packet_params_t params;
    fbe_raid_group_paged_reconstruct_t *reconstruct_context_p = NULL;
    fbe_chunk_size_t                       chunk_size;

    reconstruct_context_p = fbe_raid_group_get_bg_op_reconstruct_context(raid_group_p);

    start_lba = fbe_raid_group_get_rekey_checkpoint(raid_group_p);
    chunk_size = fbe_raid_group_get_chunk_size(raid_group_p);

    /* Once we get past the area that was in doubt then continue to reconstruct paged.
     */
    if (start_lba > (reconstruct_context_p->last_invalid_chunk_index * chunk_size)){
        fbe_raid_group_reconstruct_paged_set_state(reconstruct_context_p, FBE_RAID_GROUP_RECONST_STATE_RECONSTRUCT_PAGED);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }
    fbe_raid_group_setup_monitor_params(&params, 
                                        FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ_ONLY_VERIFY_SPECIFIC_AREA, 
                                        start_lba, 
                                        chunk_size, 
                                        (FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_METADATA_OP | 
                                         FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_NO_SL_THIS_LEVEL |
                                         FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_USE_NEW_KEY));
    fbe_raid_group_setup_monitor_packet(raid_group_p, packet_p, &params);

    fbe_transport_set_completion_function(packet_p, 
                                          fbe_raid_group_reconst_paged_sm_verify_complete,
                                          raid_group_p);

    /* There are some cases where we are using the monitor context.
     * In those cases it is ok to send the I/O immediately.
     */
    fbe_base_config_bouncer_entry((fbe_base_config_t *)raid_group_p, packet_p);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/**************************************
 * end fbe_raid_group_reconst_paged_sm_verify()
 **************************************/

/*!****************************************************************************
 * fbe_raid_group_reconst_paged_sm_verify_complete()
 ******************************************************************************
 * @brief
 *  Completion function for verifying the user area to see where we should
 *  place the rekey checkpoint.
 * 
 *  The purpose of the verify is to decide if these chunks are
 *  using the new key or the old key.
 * 
 *  - If we see many multi-bit crc errors with the new key then we
 *  will simply stop, leave the rekey checkpoint as is and allow
 *  the encryption to start from here. 
 *
 *  - If we see that the verify is successful, then we advance the
 *    checkpoint and verify the next area.
 *
 * @param packet_p   - packet that is completing
 * @param context    - completion context, which is a pointer to 
 *                        the raid group object
 * 
 * @return fbe_status_t            
 *
 * @author
 *  5/5/2014 - Created. Rob Foley
 *
 ******************************************************************************/
static fbe_status_t 
fbe_raid_group_reconst_paged_sm_verify_complete(fbe_packet_t *packet_p,
                                                fbe_packet_completion_context_t context)
{
    fbe_status_t                         status;
    fbe_raid_group_t *                   raid_group_p = NULL;
    fbe_status_t                         packet_status;
    fbe_payload_ex_t*                    sep_payload_p = NULL;      
    fbe_payload_block_operation_t       *block_operation_p = NULL;
    fbe_raid_verify_error_counts_t      *verify_errors_p = NULL;
    fbe_chunk_size_t                     chunk_size;
    fbe_lba_t                            rekey_chkpt;
    fbe_block_count_t                    block_count;
    fbe_lba_t                            new_rekey_checkpoint;
    fbe_raid_group_paged_reconstruct_t * reconstruct_context_p = NULL;


    raid_group_p = (fbe_raid_group_t *) context;
    chunk_size = fbe_raid_group_get_chunk_size(raid_group_p);
    rekey_chkpt = fbe_raid_group_get_rekey_checkpoint(raid_group_p);
    reconstruct_context_p = fbe_raid_group_get_bg_op_reconstruct_context(raid_group_p);

    packet_status = fbe_transport_get_status_code(packet_p);
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_get_block_operation(sep_payload_p);    

    verify_errors_p = fbe_raid_group_get_verify_error_counters_ptr(raid_group_p);
    fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                          "reconst sm: coh %u, crc %u, crc_m %u, crc_s %u, md %u, ss %u, ts %u, ws %u, inv %u\n",
                          verify_errors_p->u_coh_count,
                          verify_errors_p->u_crc_count,
                          verify_errors_p->u_crc_multi_count,
                          verify_errors_p->u_crc_single_count,
                          verify_errors_p->u_media_count,
                          verify_errors_p->u_ss_count,
                          verify_errors_p->u_ts_count,
                          verify_errors_p->u_ws_count,
                          verify_errors_p->invalidate_count);
    fbe_raid_group_trace(raid_group_p, FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_RAID_GROUP_DEBUG_FLAG_VERIFY_TRACING,
                         "reconst sm: verify completed for lba: 0x%llx blocks: 0x%llx\n", 
                         (unsigned long long)block_operation_p->lba, (unsigned long long)block_operation_p->block_count);

    if ((packet_status != FBE_STATUS_OK) || 
        (block_operation_p->status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS)){

        fbe_base_object_trace((fbe_base_object_t*) raid_group_p, FBE_TRACE_LEVEL_WARNING, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
            "reconst sm: verify fail,packet err %d, bl stat %d, packet 0x%p\n", 
            packet_status, block_operation_p->status, packet_p);
    }
    else {
        
        if (verify_errors_p->u_crc_multi_count > (chunk_size / 2)) {

            /* If more than half the blocks received multi-bit crc errors, 
             * it means that the read with the new key failed. 
             * We will assume that the old key should be used for these areas and thus they need to be rekeyed. 
             * Go reconstruct paged now with the existing checkpoint since these areas just verified are now above the 
             * checkpoint. 
             */
            fbe_raid_group_reconstruct_paged_set_state(reconstruct_context_p, FBE_RAID_GROUP_RECONST_STATE_RECONSTRUCT_PAGED);
            return FBE_STATUS_OK; 
        } else {
            /* Verify was successful, move chkpt.
             * We will leave the checkpoint as is and let the state machine decide if another verify is warranted 
             * or if we are done. 
             */
            new_rekey_checkpoint = block_operation_p->lba + block_operation_p->block_count;
            fbe_transport_set_completion_function(packet_p, fbe_raid_group_paged_scan_update_chkpt_completion, raid_group_p);
            block_count = new_rekey_checkpoint - rekey_chkpt;

            fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "reconst sm: vr move chkpt rk_chkpt:0x%llx bl:0x%llx new_chkpt:0x%llx\n",
                                  (unsigned long long)rekey_chkpt, (unsigned long long)block_count, (unsigned long long)new_rekey_checkpoint);

            status = fbe_raid_group_update_rekey_checkpoint(packet_p, raid_group_p, rekey_chkpt, block_count, __FUNCTION__);
            return FBE_STATUS_MORE_PROCESSING_REQUIRED;
        }
    }
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_raid_group_reconst_paged_sm_verify_complete()
 **************************************/

/*!****************************************************************************
 * fbe_raid_group_reconstruct_paged_with_callback()
 ******************************************************************************
 * @brief
 *   Start the process of reconstructing the paged metadata.
 *
 * @param raid_group_p           - pointer to the raid group
 * @param packet_p               - pointer to the packet
 * @param paged_md_lba           - start LBA in the paged metadata
 * @param paged_md_block_count   - number of paged metadata blocks to reconstruct
 * @param fbe_payload_block_operation_opcode_t - reconstruction opcode.
 *
 * @return fbe_status_t            
 *
 * @author
 *  5/2/2014 - Copied from elsewhere. Rob Foley
 *
 ******************************************************************************/
static fbe_status_t 
fbe_raid_group_reconstruct_paged_with_callback(fbe_raid_group_t *raid_group_p,
                                               fbe_packet_t *packet_p)
{
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_chunk_index_t                           paged_md_chunk_index;
    fbe_chunk_count_t                           paged_md_chunk_count;
    fbe_bool_t                                  is_in_data_area_b;
    fbe_raid_group_nonpaged_metadata_t*         nonpaged_metadata_p = NULL;
    fbe_bool_t                                  nonpaged_md_changed_b = FBE_FALSE;
    fbe_u64_t                                   metadata_offset;
    fbe_payload_ex_t*                           payload_p = NULL;
    fbe_payload_block_operation_t*              block_operation_p = NULL;    
    fbe_lba_t                                   paged_md_lba;
    fbe_block_count_t                           paged_md_block_count;
    fbe_chunk_size_t                            chunk_size;
    fbe_raid_group_paged_reconstruct_t         *reconstruct_context_p = NULL;

    reconstruct_context_p = fbe_raid_group_get_bg_op_reconstruct_context(raid_group_p);
    chunk_size = fbe_raid_group_get_chunk_size(raid_group_p);

    paged_md_lba = reconstruct_context_p->verify_start;
    paged_md_block_count = reconstruct_context_p->verify_blocks;

    /* Get the chunk range we need to reconstruct based on the incoming start lba and block count.
     */
    fbe_raid_group_bitmap_get_chunk_range(raid_group_p, 
                                          paged_md_lba, 
                                          paged_md_block_count, 
                                          &paged_md_chunk_index, 
                                          &paged_md_chunk_count);

    fbe_raid_group_bitmap_determine_if_chunks_in_data_area(raid_group_p, 
                                                           paged_md_chunk_index, 
                                                           paged_md_chunk_count, 
                                                           &is_in_data_area_b);
    if (is_in_data_area_b) {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s chunks not in paged metadata; chunk index %llu, chunk count %d\n", 
                              __FUNCTION__,
                              (unsigned long long)paged_md_chunk_index,
                              paged_md_chunk_count); 
        return status;
    }

    payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_allocate_block_operation(payload_p);

    /* We build a block operation to save the range of the operation.
     */
    fbe_payload_block_build_operation(block_operation_p,
                                      FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_VERIFY,
                                      paged_md_lba,
                                      paged_md_block_count,
                                      FBE_BE_BYTES_PER_BLOCK,
                                      0,
                                      NULL);

    status = fbe_payload_ex_increment_block_operation_level(payload_p);
    fbe_transport_set_completion_function(packet_p,
                                          fbe_raid_group_process_and_release_block_operation,
                                          raid_group_p);

    /* Get a pointer to the non-paged metadata.
     */
    fbe_raid_group_lock(raid_group_p);
    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t*) raid_group_p, (void **)&nonpaged_metadata_p);

    /* Determine if we need to update the nonpaged metadata and do so.
     * This is necessary if a verify or rebuild checkpoint, for example, falls in the range of user data corresponding 
     * to the paged metadata we need to reconstruct.
     */
    fbe_raid_group_bitmap_reconstruct_paged_update_nonpaged_if_needed(raid_group_p, 
                                                                      paged_md_chunk_index,
                                                                      paged_md_chunk_count,
                                                                      nonpaged_metadata_p,
                                                                      &nonpaged_md_changed_b);

    if (nonpaged_md_changed_b) {
        fbe_raid_group_unlock(raid_group_p);

        /* Update the nonpaged metadata via the Metadata Service.
         * Will update the paged metadata after the nonpaged via the completion function.
         */
        fbe_transport_set_completion_function(packet_p, 
                                              fbe_raid_group_bitmap_reconst_paged_np_updated, 
                                              raid_group_p);

        metadata_offset = 0;  /* we are writing all of the nonpaged metadata */
        fbe_base_config_metadata_nonpaged_write_persist((fbe_base_config_t*) raid_group_p, packet_p, 
                                                        metadata_offset, 
                                                        (fbe_u8_t*)nonpaged_metadata_p,
                                                        sizeof(fbe_raid_group_nonpaged_metadata_t));
         return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    fbe_raid_group_unlock(raid_group_p);

    /* The nonpaged metadata did not change, so just update the paged metadata.
     */
    fbe_raid_group_reconstruct_paged_update(raid_group_p, packet_p);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/**************************************
 * end fbe_raid_group_reconstruct_paged_with_callback()
 **************************************/

/*!****************************************************************************
 * fbe_raid_group_bitmap_reconst_paged_np_updated()
 ******************************************************************************
 * @brief
 *   This is the completion function for updating the nonpaged metadata as part of
 *   paged metadata reconstruction.
 *
 * @param packet_p   - packet that is completing
 * @param context    - completion context, which is a pointer to 
 *                        the raid group object
 * 
 * @return fbe_status_t            
 *
 * @author
 *  5/2/2014 - Copied from elsewhere. Rob Foley
 *
 ******************************************************************************/
static fbe_status_t 
fbe_raid_group_bitmap_reconst_paged_np_updated(fbe_packet_t *packet_p,
                                               fbe_packet_completion_context_t context)
{
    fbe_raid_group_t *                      raid_group_p = NULL;
    fbe_status_t                            packet_status;

    raid_group_p = (fbe_raid_group_t *) context;

    packet_status = fbe_transport_get_status_code(packet_p);

    if (packet_status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t*) raid_group_p, 
                              FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                              "reconst sm: write fail,packet err %d, packet 0x%p\n", 
                              packet_status, packet_p); 
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                           FBE_TRACE_LEVEL_INFO,
                           FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "reconst sm: nonpaged update complete packet: %p\n", packet_p); 

    /* Update the paged metadata from the nonpaged metadata.
     */
    fbe_raid_group_reconstruct_paged_update(raid_group_p, packet_p);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/**************************************
 * end fbe_raid_group_bitmap_reconst_paged_np_updated()
 **************************************/

/*!****************************************************************************
 * fbe_raid_group_reconstruct_paged_update()
 ******************************************************************************
 * @brief
 *   This function reconstructs the paged metadata from the nonpaged metadata and sends
 *   the write to the Metadata Service.
 *
 * @param raid_group_p           - pointer to the raid group
 * @param packet_p               - pointer to the packet
 * 
 * @return fbe_status_t            
 *
 * @author
 *  5/2/2014 - Created. Rob Foley
 *
 ******************************************************************************/
fbe_status_t 
fbe_raid_group_reconstruct_paged_update(fbe_raid_group_t *raid_group_p,
                                        fbe_packet_t *packet_p)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_ex_t*                               sep_payload_p = NULL;
    fbe_lba_t                                       lba;
    fbe_block_count_t                               block_count;
    fbe_chunk_index_t                               paged_md_chunk_index;
    fbe_chunk_count_t                               paged_md_chunk_count;
    fbe_chunk_index_t                               rg_user_data_start_chunk_index;
    fbe_chunk_count_t                               rg_user_data_chunk_count;
    fbe_raid_group_paged_metadata_t                 paged_metadata_bits;
    fbe_chunk_size_t                                chunk_size_in_blocks;
    fbe_u32_t                                       num_paged_md_recs_in_block;
    fbe_u32_t                                       num_paged_md_recs_in_chunk;
    fbe_chunk_count_t                               total_chunks;
    fbe_u32_t                                       aligned;
    fbe_payload_metadata_operation_t*               metadata_operation_p = NULL;       
    fbe_u64_t                                       metadata_offset;
    fbe_payload_block_operation_t                   *block_operation_p = NULL;
    fbe_raid_geometry_t *                           raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_u16_t                                       data_disks;
    fbe_sg_element_t *sg_p = NULL;
 
    /* Get the lba and block count from the packet.  
     * This is the area of paged metadata to reconstruct.
     */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_get_present_block_operation(sep_payload_p);
    lba = block_operation_p->lba;
    block_count = block_operation_p->block_count;

    /* Initialize the paged metadata record.
     */
    fbe_raid_group_metadata_init_paged_metadata(&paged_metadata_bits);

    /* Get the chunk range we need to reconstruct.
     * For now, we are reconstructing 1 chunk of paged metadata at a time.
     */
    fbe_raid_group_bitmap_get_chunk_range(raid_group_p, lba, block_count, &paged_md_chunk_index, &paged_md_chunk_count);

    /* Determine the user data chunk range the paged metadata chunk range corresponds to.
     */
    fbe_raid_group_bitmap_determine_user_chunk_range_from_paged_md_chunk_range(raid_group_p, 
                                                                               paged_md_chunk_index, 
                                                                               paged_md_chunk_count,
                                                                               &rg_user_data_start_chunk_index,
                                                                               &rg_user_data_chunk_count);

    /* Set the paged metadata bits from the nonpaged metadata for the given RG user data chunk range.
     */
    fbe_raid_group_bitmap_set_paged_bits_from_nonpaged(raid_group_p, 
                                                       rg_user_data_start_chunk_index, 
                                                       rg_user_data_chunk_count,
                                                       &paged_metadata_bits);
    
    /* Change to logical LBAs.
     */
    fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);
    rg_user_data_start_chunk_index *= data_disks;
    rg_user_data_chunk_count *= data_disks;

    fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                          FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "reconst sm: idx/cnt: 0x%llx/0x%llx bits: v: 0x%x vr: 0x%x nr: 0x%x r:0x%x\n", 
                          (unsigned long long)rg_user_data_start_chunk_index, (unsigned long long)rg_user_data_chunk_count,
                          paged_metadata_bits.valid_bit, paged_metadata_bits.verify_bits,
                          paged_metadata_bits.needs_rebuild_bits, paged_metadata_bits.reserved_bits); 

    /* Allocate a metadata operation.
     */
    metadata_operation_p = fbe_payload_ex_allocate_metadata_operation(sep_payload_p);
    fbe_payload_ex_increment_metadata_operation_level(sep_payload_p);

    /*  Get the offset of this chunk in the metadata (ie. where we'll start to write to).
     */
    metadata_offset = rg_user_data_start_chunk_index * sizeof(fbe_raid_group_paged_metadata_t);

    /* Get the chunk size for our calculations.
     */
    chunk_size_in_blocks = fbe_raid_group_get_chunk_size(raid_group_p);

    /* Make sure we are aligned by the number of blocks in a chunk.
     * We do not want to do a pre-read in the Metadata Service for our write, so we need to be aligned.
     */
    aligned = metadata_offset % (chunk_size_in_blocks * FBE_METADATA_BLOCK_DATA_SIZE);
    if (aligned != 0)
    {
         fbe_base_object_trace((fbe_base_object_t*) raid_group_p, 
                               FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                               "reconst sm:: metadata_offset not aligned: 0x%llx\n", 
                               (unsigned long long)metadata_offset); 
         return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Determine the number of rg user data chunks to update. We may reconstruct more than we need
     * for alignment purposes.  Again, we do not want to do a pre-read in the Metadata Service for our write,
     * so we need to be aligned.  If our RG does not use all of the paged metadata chunk, we will fill 
     * in the rest as part of reconstructing the entire paged metadata chunk. 
     */
    num_paged_md_recs_in_block = FBE_METADATA_BLOCK_DATA_SIZE / FBE_RAID_GROUP_CHUNK_ENTRY_SIZE;
    num_paged_md_recs_in_chunk = num_paged_md_recs_in_block * chunk_size_in_blocks;
    total_chunks = paged_md_chunk_count * num_paged_md_recs_in_chunk * data_disks;

    /* Make sure we are aligned by the number of blocks in a chunk.
     */
    aligned = (total_chunks * FBE_RAID_GROUP_CHUNK_ENTRY_SIZE) % (chunk_size_in_blocks * FBE_METADATA_BLOCK_DATA_SIZE);
    if (aligned != 0)
    {
         fbe_base_object_trace((fbe_base_object_t*) raid_group_p, 
                               FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                               "reconst sm: total_chunks not aligned: 0x%llx\n", 
                               (unsigned long long)metadata_offset); 
        
         return FBE_STATUS_GENERIC_FAILURE;
    }

    /* We use a paged metadata update, but we never actually return the status 
     * that causes the paged to be read, so in affect this is a scan. 
     * We use write-verify flavor since it continues on invalidated blocks. 
     */
    fbe_payload_metadata_build_paged_write_verify_update(metadata_operation_p,
                                                         &(((fbe_base_config_t *) raid_group_p)->metadata_element),
                                                         metadata_offset,
                                                         sizeof(fbe_raid_group_paged_metadata_t),
                                                         total_chunks,
                                                         fbe_raid_group_reconstruct_paged_md_callback,
                                                         (void *)raid_group_p);

    fbe_payload_ex_get_sg_list(sep_payload_p, &sg_p, NULL);
    if (sg_p) {
        status = fbe_metadata_paged_init_client_blob_with_sg_list(sg_p, metadata_operation_p);
    }

    fbe_raid_group_bitmap_metadata_populate_stripe_lock(raid_group_p,
                                                        metadata_operation_p,
                                                        total_chunks);

    fbe_transport_set_completion_function(packet_p, 
                                          fbe_raid_group_reconstruct_paged_write_completion, 
                                          raid_group_p);

    fbe_transport_set_packet_attr(packet_p, FBE_PACKET_FLAG_DO_NOT_CANCEL);  

    fbe_payload_ex_increment_metadata_operation_level(sep_payload_p);
    /* Issue the metadata operation to write the paged metadata chunk.
     */
    status = fbe_metadata_operation_entry(packet_p);
    return status;
} 
/**************************************
 * end fbe_raid_group_reconstruct_paged_update()
 **************************************/

/*!****************************************************************************
 * @fn fbe_raid_group_reconstruct_paged_md_callback()
 ******************************************************************************
 * @brief
 *  This is the callback function after the paged metadata is read.
 * 
 *  If the chunks that we see are not valid, then we will
 *  populate the chunk with the appropriate information for all bits
 *  (valid, verify, nr, rekey, etc).
 * 
 * @param sg_list           - Pointer to sg list.
 * @param slot_offset       - Offset to the paged metadata.
 * @param context           - Pointer to context.
 *
 * @return fbe_status_t.
 *
 * @author
 *  5/2/2014 - Created. Rob Foley
 *
 ******************************************************************************/
static fbe_status_t 
fbe_raid_group_reconstruct_paged_md_callback(fbe_packet_t * packet, fbe_metadata_callback_context_t context)
{
    fbe_payload_ex_t * sep_payload = fbe_transport_get_payload_ex(packet);
    fbe_payload_metadata_operation_t * mdo = NULL;
    fbe_raid_group_paged_metadata_t * paged_metadata_p;
    fbe_sg_element_t * sg_list = NULL;
    fbe_sg_element_t * sg_ptr = NULL;
    fbe_lba_t lba_offset;
    fbe_u64_t slot_offset;
    fbe_status_t status = FBE_STATUS_OK; /* indicates nothing needs change */
    fbe_raid_group_t * raid_group_p = (fbe_raid_group_t *) context;
    fbe_chunk_index_t chunk_index;
    fbe_chunk_count_t total_chunks = fbe_raid_group_get_total_chunks(raid_group_p);
    fbe_raid_position_bitmask_t degraded_bitmask;
    fbe_raid_position_bitmask_t extra_nr_bitmask;
     
    if(sep_payload->current_operation->payload_opcode == FBE_PAYLOAD_OPCODE_METADATA_OPERATION){
        mdo = fbe_payload_ex_get_metadata_operation(sep_payload);
    } else if(sep_payload->current_operation->payload_opcode == FBE_PAYLOAD_OPCODE_STRIPE_LOCK_OPERATION){
        mdo = fbe_payload_ex_get_any_metadata_operation(sep_payload);
    } else {
        fbe_topology_class_trace(FBE_CLASS_ID_RAID_GROUP, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                      "%s: Invalid payload opcode:%d \n", __FUNCTION__, sep_payload->current_operation->payload_opcode);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_raid_group_get_degraded_mask(raid_group_p, &degraded_bitmask);
    
    lba_offset = fbe_metadata_paged_get_lba_offset(mdo);
    slot_offset = fbe_metadata_paged_get_slot_offset(mdo);
    chunk_index = fbe_metadata_paged_get_record_offset(mdo);
    sg_list = mdo->u.metadata_callback.sg_list;
    sg_ptr = &sg_list[lba_offset - mdo->u.metadata_callback.start_lba];
    paged_metadata_p = (fbe_raid_group_paged_metadata_t *)(sg_ptr->address + slot_offset);

    while ((sg_ptr->address != NULL) && 
           (mdo->u.metadata_callback.current_count_private < mdo->u.metadata_callback.repeat_count)) {
        
        extra_nr_bitmask = (~degraded_bitmask) & paged_metadata_p->needs_rebuild_bits;
        if (extra_nr_bitmask != 0) {
            fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                  "raid_group: reconst paged md callback deg_bm: 0x%x nr: 0x%x v: 0x%x vr: 0x%x rk: 0x%x\n",
                                  degraded_bitmask, paged_metadata_p->needs_rebuild_bits, 
                                  paged_metadata_p->valid_bit, paged_metadata_p->verify_bits, paged_metadata_p->rekey);
            status = FBE_STATUS_MORE_PROCESSING_REQUIRED; /* this round needs persist */
        }
        if ((paged_metadata_p->valid_bit == 0) || (extra_nr_bitmask != 0)) {
            /* The chunk is not valid.  Populate it with the correct information.
             */
            fbe_raid_group_metadata_init_paged_metadata(paged_metadata_p);

            /* Anything beyond the capacity of the RG will only be populated with valid bit.
             */
            if (chunk_index < total_chunks) {
                fbe_raid_group_bitmap_set_paged_bits_from_nonpaged(raid_group_p, 
                                                                   chunk_index, 
                                                                   1,    /* Chunk count */
                                                                   paged_metadata_p);
                fbe_raid_group_trace(raid_group_p, FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_RAID_GROUP_DEBUG_FLAG_VERIFY_TRACING,
                                      "reconst sm: idx: 0x%llx bits: vlid: %x vr: %x nr: %x r:0x%x res: %x\n", 
                                      (unsigned long long)chunk_index, 
                                      paged_metadata_p->valid_bit, paged_metadata_p->verify_bits,
                                      paged_metadata_p->needs_rebuild_bits, paged_metadata_p->rekey,
                                      paged_metadata_p->reserved_bits);
            }
            status = FBE_STATUS_MORE_PROCESSING_REQUIRED; /* this round needs persist */
        }

        mdo->u.metadata_callback.current_count_private++;
        chunk_index++;
        paged_metadata_p++;
        if (paged_metadata_p == (fbe_raid_group_paged_metadata_t *)(sg_ptr->address + FBE_METADATA_BLOCK_DATA_SIZE)){
            sg_ptr++;
            paged_metadata_p = (fbe_raid_group_paged_metadata_t *)sg_ptr->address;
        }
    }

    return status;
}
/******************************************************************************
 * end fbe_raid_group_reconstruct_paged_md_callback()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_raid_group_reconstruct_paged_write_completion()
 ******************************************************************************
 * @brief
 *  If we were successful writing the paged, then we will complete the
 *  state machine.
 *
 * @param packet_p   - packet that is completing
 * @param context    - completion context, which is a pointer to 
 *                        the raid group object
 * 
 * @return fbe_status_t            
 *
 * @author
 *  5/2/2014 - Created. Rob Foley
 *
 ******************************************************************************/
static fbe_status_t 
fbe_raid_group_reconstruct_paged_write_completion(fbe_packet_t *packet_p,
                                                  fbe_packet_completion_context_t context)
{
    fbe_raid_group_t *                      raid_group_p = NULL;
    fbe_status_t                            packet_status;
    fbe_payload_ex_t*                       sep_payload_p = NULL;               
    fbe_payload_metadata_operation_t*       metadata_operation_p = NULL;        
    fbe_payload_metadata_status_t           metadata_status;
    fbe_payload_block_operation_t           *block_operation_p = NULL;
    fbe_payload_block_operation_status_t    block_status;
    fbe_payload_block_operation_qualifier_t block_qual;
    fbe_raid_group_paged_reconstruct_t     *reconstruct_context_p = NULL;

    /* Get a pointer to the RG from the context.
     */
    raid_group_p = (fbe_raid_group_t *) context;

    packet_status = fbe_transport_get_status_code(packet_p);

    /*  Get the status of the metadata operation.
     */
    sep_payload_p           = fbe_transport_get_payload_ex(packet_p);
    metadata_operation_p    = fbe_payload_ex_get_metadata_operation(sep_payload_p);
    fbe_payload_metadata_get_status(metadata_operation_p, &metadata_status);
    block_operation_p = fbe_payload_ex_get_present_block_operation(sep_payload_p);
    reconstruct_context_p = fbe_raid_group_get_bg_op_reconstruct_context(raid_group_p);  

    /*  Release the metadata operation.
     */
    fbe_payload_ex_release_metadata_operation(sep_payload_p, metadata_operation_p);

    /* Translate metadata status to block status and set it in block operation.
     */
    fbe_metadata_translate_metadata_status_to_block_status(metadata_status, &block_status, &block_qual);
    fbe_payload_block_set_status(block_operation_p, block_status, block_qual);    

    /*  If the packet status or MD status was not a success log a trace.
     */
    if ((packet_status != FBE_STATUS_OK) || (metadata_status != FBE_PAYLOAD_METADATA_STATUS_OK))
    {
        fbe_base_object_trace((fbe_base_object_t*) raid_group_p, 
                              FBE_TRACE_LEVEL_WARNING, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                              "reconst sm: write fail,packet err %d, mdata stat %d, packet 0x%p\n", 
                              packet_status, metadata_status, packet_p);
    }
    else
    {
        /* Done with the paged reconstruct.
         */
        fbe_base_object_trace((fbe_base_object_t*) raid_group_p, 
                              FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO,
                              "reconst sm: completed for lba: 0x%llx blocks: 0x%llx\n", 
                              (unsigned long long)block_operation_p->lba, (unsigned long long)block_operation_p->block_count);
        fbe_raid_group_reconstruct_paged_set_state(reconstruct_context_p, FBE_RAID_GROUP_RECONST_STATE_DONE);
    }

    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_raid_group_reconstruct_paged_write_completion()
 **************************************/
/*************************
 * end file fbe_raid_group_reconstruct_paged.c
 *************************/


