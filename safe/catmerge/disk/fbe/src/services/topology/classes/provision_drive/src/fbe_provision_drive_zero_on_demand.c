/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_provision_drive_zero_on_demand.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the functionality to process the zero on demand 
 *  functionality.
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
#include "fbe_raid_library_proto.h"
#include "fbe_transport_memory.h"
#include "fbe_raid_library.h"
#include "fbe/fbe_winddk.h"


/*****************************************
 * LOCAL FUNCTION PROTOTYPES
 *****************************************/

static fbe_status_t fbe_provision_drive_zero_on_demand_paged_metadata_read(fbe_packet_t * packet_p, fbe_provision_drive_t * provision_drive_p);

static fbe_status_t fbe_provision_drive_zero_on_demand_paged_metadata_read_completion(fbe_packet_t * packet_p,
                                                                                      fbe_packet_completion_context_t packet_completion_context);

static fbe_status_t fbe_provision_drive_zero_on_demand_process_paged_metadata_bits(fbe_provision_drive_t * provision_drive_p,
                                                                                   fbe_provision_drive_paged_metadata_t * paged_data_bits_p,
                                                                                   fbe_packet_t * packet_p);

static fbe_status_t fbe_provision_drive_zero_on_demand_write_process_paged_metadata_bits(fbe_provision_drive_t * provision_drive_p,
                                                                                         fbe_provision_drive_paged_metadata_t * paged_data_bits_p,
                                                                                         fbe_packet_t * packet_p);

static fbe_status_t fbe_provision_drive_zero_on_demand_write_start_zero_on_demand(fbe_provision_drive_t * provision_drive_p,
                                                                                  fbe_packet_t * packet_p);

static fbe_status_t fbe_provision_drive_zero_on_demand_write_determine_if_fast_path_can_use(fbe_provision_drive_t * provision_drive_p,
                                                                                            fbe_packet_t * packet_p,
                                                                                            fbe_bool_t * can_fast_path_b);
static fbe_status_t fbe_provision_drive_zero_on_demand_write_slow_path_write_start(fbe_provision_drive_t *provision_drive_p,
                                                                                   fbe_packet_t *packet_p);
static fbe_status_t fbe_provision_drive_zero_on_demand_write_slow_path_start(fbe_provision_drive_t * provision_drive_p,
                                                                             fbe_packet_t * packet_p);
static fbe_status_t fbe_provision_drive_zero_on_demand_write_slow_path_write_same_start(fbe_provision_drive_t *provision_drive_p,
                                                                                        fbe_packet_t *packet_p);
static fbe_status_t fbe_provision_drive_zero_on_demand_write_slow_path_write_same_completion(fbe_packet_t * packet_p,
                                                                                             fbe_packet_completion_context_t packet_completion_context);

static fbe_status_t fbe_provision_drive_zero_on_demand_write_slow_path_write_allocate_completion(fbe_memory_request_t * memory_request, 
                                                                                                 fbe_memory_completion_context_t context);

static fbe_status_t fbe_provision_drive_zero_on_demand_write_fast_path_calculate_xfer_size(fbe_provision_drive_t *  provision_drive_p,
                                                                                           fbe_packet_t * packet_p,
                                                                                           fbe_block_count_t * transfer_size_p);

static fbe_status_t fbe_provision_drive_zero_on_demand_write_fast_path_start(fbe_provision_drive_t * provision_drive_p,
                                                                             fbe_packet_t * packet_p);

static fbe_status_t fbe_provision_drive_zero_on_demand_write_fast_path_write_allocate_completion(fbe_memory_request_t * memory_request_p, 
                                                                                                 fbe_memory_completion_context_t context);

static fbe_status_t fbe_provision_drive_zero_on_demand_write_fast_path_calculate_memory_chunks(fbe_provision_drive_t * provision_drive_p,
                                                                                               fbe_packet_t * packet_p,
                                                                                               fbe_u32_t memory_chunk_size,
                                                                                               fbe_u32_t * number_of_chunks_p);

static fbe_status_t fbe_provision_drive_zero_on_demand_write_fast_path_calculate_zod_sg_count(fbe_lba_t host_start_lba,
                                                                                              fbe_block_count_t host_block_count,
                                                                                              fbe_sg_element_t * host_sg_list,
                                                                                              fbe_lba_t zod_start_lba,
                                                                                              fbe_lba_t zod_block_count,
                                                                                              fbe_u32_t * dest_sg_element_count_p,
                                                                                              fbe_bool_t   zero_required);

static fbe_status_t fbe_provision_drive_zero_on_demand_write_fast_path_calculate_zod_lba_range(fbe_lba_t start_lba,
                                                                                               fbe_block_count_t block_count,
                                                                                               fbe_provision_drive_paged_metadata_t * paged_data_bits_p,
                                                                                               fbe_lba_t * zod_start_lba_p,
                                                                                               fbe_block_count_t * zod_block_count_p);

static fbe_status_t fbe_provision_drive_zod_fast_plant_sgs(fbe_provision_drive_t *provision_drive_p,
                                                           fbe_payload_ex_t *payload_p,
                                                           fbe_lba_t  host_start_lba,
                                                           fbe_block_count_t host_blocks,
                                                           fbe_sg_element_t * host_sg_list,
                                                           fbe_lba_t  zod_start_lba,
                                                           fbe_lba_t  zod_block_count,
                                                           fbe_sg_element_t * zod_sgl_list,
                                                           fbe_bool_t  zero_required);

static fbe_status_t fbe_provision_drive_zero_on_demand_write_update_metadata_after_write_completion(fbe_packet_t * packet_p,
                                                                                                    fbe_packet_completion_context_t packet_completion_context);

static fbe_status_t fbe_provision_drive_zero_on_demand_write_update_paged_metadata(fbe_provision_drive_t * provision_drive_p,
                                                                                   fbe_packet_t * packet_p);

static fbe_status_t fbe_provision_drive_zero_on_demand_write_update_paged_metadata_completion(fbe_packet_t * packet_p,
                                                                                              fbe_packet_completion_context_t context);

static fbe_status_t fbe_provision_drive_zero_on_demand_read_process_paged_metadata_bits(fbe_provision_drive_t * provision_drive_p,
                                                                                        fbe_provision_drive_paged_metadata_t * paged_data_bits_p,
                                                                                        fbe_packet_t * packet_p);

static fbe_status_t fbe_provision_drive_zero_on_demand_read_start(fbe_provision_drive_t * provision_drive_p,
                                                                  fbe_packet_t * packet_p);

static 
fbe_status_t fbe_provision_drive_zero_on_demand_read_media_read_completion(fbe_packet_t * packet_p,
                                                                                  fbe_packet_completion_context_t packet_completion_context);

static fbe_status_t fbe_provision_drive_zero_on_demand_read_determine_if_media_read_needed(fbe_lba_t start_lba,
                                                                                           fbe_block_count_t block_count,
                                                                                           fbe_provision_drive_paged_metadata_t * paged_data_bits_p,
                                                                                           fbe_bool_t * is_media_read_needed_b_p);


static fbe_status_t fbe_provision_drive_zero_on_demand_read_calc_memory_chunks(fbe_provision_drive_t * provision_drive_p,
                                                                               fbe_packet_t * packet_p,
                                                                               fbe_u32_t memory_chunk_size,
                                                                               fbe_u32_t * number_of_chunks_p);

static fbe_status_t fbe_provision_drive_zero_on_demand_read_get_next_zeroed_lba_range(fbe_lba_t host_start_lba,
                                                                                      fbe_block_count_t host_block_count,
                                                                                      fbe_provision_drive_paged_metadata_t * paged_data_bits_p,
                                                                                      fbe_lba_t * next_read_start_lba_p,
                                                                                      fbe_block_count_t * next_read_block_count_p);

static fbe_status_t fbe_provision_drive_zero_on_demand_read_get_next_need_zero_lba_range(fbe_lba_t host_start_lba,
                                                                                         fbe_block_count_t host_block_count,
                                                                                         fbe_provision_drive_paged_metadata_t * paged_data_bits_p,
                                                                                         fbe_lba_t * next_need_zero_start_lba_p,
                                                                                         fbe_block_count_t * next_need_zero_block_count_p);

static fbe_status_t fbe_provision_drive_zero_on_demand_read_get_next_zeroed_lba_range_with_sg_list(fbe_lba_t host_start_lba,
                                                                                                   fbe_block_count_t host_block_count,
                                                                                                   fbe_sg_element_t * host_sg_list,
                                                                                                   fbe_provision_drive_paged_metadata_t * paged_data_bits_p,
                                                                                                   fbe_u32_t read_sg_element_count,
                                                                                                   fbe_lba_t * next_read_start_lba_p,
                                                                                                   fbe_block_count_t * next_read_block_count_p,
                                                                                                   fbe_sg_element_t * next_read_sg_list_p,
                                                                                                   fbe_u16_t * next_read_sg_element_count_p,
                                                                                                   fbe_u32_t * total_used_sg_element_count_p);

static fbe_status_t fbe_provision_drive_zero_on_demand_read_calc_sg_and_subpacket_count(fbe_lba_t host_start_lba,
                                                                                        fbe_block_count_t host_block_count,
                                                                                        fbe_sg_element_t * host_sg_list,
                                                                                        fbe_provision_drive_paged_metadata_t * paged_data_bits_p,
                                                                                        fbe_u32_t * number_of_read_request_p,
                                                                                        fbe_u32_t * total_sg_element_count_p);

static fbe_status_t fbe_provision_drive_zero_on_demand_read_calc_sg_count(fbe_lba_t host_start_lba,
                                                                          fbe_block_count_t host_block_count,
                                                                          fbe_sg_element_t * host_sg_list,
                                                                          fbe_lba_t next_media_read_start_lba,
                                                                          fbe_block_count_t next_media_read_block_count,
                                                                          fbe_u32_t * media_read_sg_element_count_p);

static fbe_status_t fbe_provision_drive_zero_on_demand_read_fill_zero_if_needed(fbe_provision_drive_t * provision_drive_p,
                                                                                fbe_packet_t * packet_p);

static fbe_status_t fbe_provision_drive_zero_on_demand_read_fill_zero_buffer(fbe_provision_drive_t * provision_drive_p,
                                                                             fbe_sg_element_t * host_sg_list,
                                                                             fbe_lba_t  host_start_lba,
                                                                             fbe_lba_t  next_need_zero_start_lba,
                                                                             fbe_block_count_t next_need_zero_block_cout);

static fbe_status_t fbe_provision_drive_zero_on_demand_plant_sgl_with_zero_buffer(fbe_sg_element_t ** zod_sg_list,
                                                                                  fbe_u32_t zod_sg_list_count,
                                                                                  fbe_block_count_t pre_zod_block_count,
                                                                                  fbe_u32_t * used_zod_sg_elements_p);
static fbe_status_t fbe_provision_drive_zero_on_demand_check_zeroed_process_paged_metadata(fbe_provision_drive_t * provision_drive_p,
                                                                                           fbe_provision_drive_paged_metadata_t * paged_data_bits_p,
                                                                                           fbe_packet_t * packet_p);
static fbe_status_t
fbe_provision_drive_zero_on_demand_plant_sgl_with_invalid_pattern_buffer(fbe_lba_t start_lba,
                                                                         fbe_sg_element_t ** zod_sg_list,
                                                                         fbe_u32_t zod_sg_list_count,
                                                                         fbe_block_count_t zod_block_count,
                                                                         fbe_u32_t * used_zod_sg_elements_p);
static fbe_status_t
fbe_provision_drive_zero_on_demand_update_metadata_after_invalidated_write_completion(fbe_packet_t * packet_p,
                                                                                      fbe_packet_completion_context_t packet_completion_context);

static fbe_status_t
fbe_provision_drive_zero_on_demand_invalidated_write_update_paged_metadata(fbe_provision_drive_t * provision_drive_p,
                                                                           fbe_packet_t * packet_p);

static fbe_status_t
fbe_provision_drive_zero_on_demand_invalidated_write_update_paged_metadata_completion(fbe_packet_t * packet_p,
                                                                                      fbe_packet_completion_context_t context);

static fbe_status_t
fbe_provision_drive_zero_on_demand_bundle_start(fbe_payload_ex_t * sep_payload_p, fbe_provision_drive_t * provision_drive_p);


/*!****************************************************************************
 * @fn fbe_provision_drive_zero_on_demand_process_io_request()
 ******************************************************************************
 * @brief
 *  This function is used to handle the read/write request in provision drive
 *  object.
 *
 * @param provision_drive_p - Pointer to the provision drive object.
 * @param packet_p          - Pointer the the I/O packet.
 *
 * @return fbe_status_t.
 *
 * @author
 *  7/22/20010 - Created. Dhaval Patel
 *
 ******************************************************************************/
fbe_status_t 
fbe_provision_drive_zero_on_demand_process_io_request(fbe_provision_drive_t * provision_drive_p, 
                                                      fbe_packet_t * packet_p)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_payload_ex_t *                     sep_payload_p = NULL;
    fbe_payload_block_operation_t *         block_operation_p = NULL;
    fbe_lba_t                               paged_metadata_start_lba;
    fbe_lba_t                               start_lba;
    fbe_block_count_t                       block_count;
    fbe_chunk_index_t                       start_chunk_index;
    fbe_chunk_count_t                       chunk_count;
    fbe_bool_t                              b_is_zero_on_demand_set = FBE_FALSE;
    fbe_payload_block_operation_opcode_t    opcode;
	fbe_lba_t								exported_capacity;
    fbe_bool_t                              b_is_forced_write = FBE_FALSE;

    /* Get the sep payload and block operation. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_get_present_block_operation(sep_payload_p);

    /* Get the start lba of the block operation. */
    fbe_payload_block_get_lba(block_operation_p, &start_lba);
    fbe_payload_block_get_block_count(block_operation_p, &block_count);
    fbe_payload_block_get_opcode(block_operation_p, &opcode);

    /* If the `forced write' flag is set in the block operation, flag it.
     */
    if (fbe_payload_block_is_flag_set(block_operation_p, FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_FORCED_WRITE))
    {
        b_is_forced_write = FBE_TRUE;
    }

    /* Get the start chunk index and chunk count from the start lba and end lba of the range. */
    fbe_provision_drive_utils_calculate_chunk_range(start_lba,
                                                     block_count,
                                                     FBE_PROVISION_DRIVE_CHUNK_SIZE,
                                                     &start_chunk_index,
                                                     &chunk_count);

    /* if number of chunks exceeds maximum number of the chunks pvd supports then fail
     * the i/o request to RAID object.
     */
    if(chunk_count > FBE_PROVISION_DRIVE_MAX_CHUNKS)
    {
        fbe_provision_drive_utils_trace(provision_drive_p,
                                        FBE_TRACE_LEVEL_ERROR,
                                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                        FBE_PROVISION_DRIVE_DEBUG_FLAG_ZOD_TRACING,
                                        "ZOD:exceeded lba range, lba:0x%llx, blk:0x%llx, start_idx:0x%llx, num_chks:0x%x.\n",
                                        (unsigned long long)start_lba,
					(unsigned long long)block_count,
					(unsigned long long)start_chunk_index,
					chunk_count);
        
		fbe_payload_block_set_status(block_operation_p,
                                     FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                                     FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNEXPECTED_ERROR);

		fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        //fbe_provision_drive_utils_complete_packet(packet_p, FBE_STATUS_OK, 0);
        return FBE_STATUS_OK;
    }

    /* get the paged metadata start lba of the pvd object. */
    fbe_base_config_metadata_get_paged_record_start_lba((fbe_base_config_t *) provision_drive_p,
                                                        &paged_metadata_start_lba);

	fbe_base_config_get_capacity((fbe_base_config_t *)provision_drive_p, &exported_capacity );

    /* Determine whether zero on demand flag is already set. */
    //fbe_provision_drive_metadata_get_zero_on_demand_flag(provision_drive_p, &b_is_zero_on_demand_set);
	b_is_zero_on_demand_set = FBE_TRUE;

    fbe_provision_drive_utils_trace( provision_drive_p,
                                 FBE_TRACE_LEVEL_DEBUG_LOW,
                                 FBE_TRACE_MESSAGE_ID_INFO, 
                                 FBE_PROVISION_DRIVE_DEBUG_FLAG_ZOD_TRACING,
                                 "pvd: IO lba:0x%llx,blks:0x%llx,op:%d,sci:%llu,cnt:%d,ZOD:%d,frc_wr:%d,mdl:0x%llx,pck:0x%p\n",
                                 (unsigned long long)start_lba,
				 (unsigned long long)block_count, opcode,
				 (unsigned long long)start_chunk_index,
				 chunk_count, b_is_zero_on_demand_set,
				 b_is_forced_write,
				 (unsigned long long)paged_metadata_start_lba,
				 packet_p);

    /* If `forced write' flag is set we must execute a zero on demand write
     * so that we update the CU (Consumed) metadata flag.
     */
    if (b_is_forced_write == FBE_TRUE)
    {
        b_is_zero_on_demand_set = FBE_TRUE;
    }

    /* Determine whether the incoming request is above the metadata region, if it is then
     * send the write request without processing zero on demand operation, during pvd object
     * initiailziation it always make sure to initialize metadata with zeroed buffer.
     */
    if((start_lba < paged_metadata_start_lba) && (b_is_zero_on_demand_set)) {   
        /* handle the non zero request, read the paged metadata before we actually decide to perform block operation. */
        //status = fbe_provision_drive_zero_on_demand_paged_metadata_read(packet_p, provision_drive_p);
		fbe_transport_set_completion_function(packet_p, (fbe_packet_completion_function_t)fbe_provision_drive_zero_on_demand_paged_metadata_read, provision_drive_p);
        status = FBE_STATUS_CONTINUE; 
    } else if (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_CHECK_ZEROED) {
        /* We are not zeroed.  We return a status of success with a qualifier to indicate we are not zeroed. */
        fbe_payload_block_set_status(block_operation_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS,
                                     FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NOT_ZEROED);
		fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);

        /* In case of CHECK_ZEROED, we end up completing the packet immediately. Instead, 
         * we should be pushing the packet to run_queue for later processing. This will break
         * the I/O context and avoid the stack overflow. 
         */
        fbe_transport_run_queue_push_packet(packet_p, FBE_TRANSPORT_RQ_METHOD_SAME_CORE);

        //fbe_provision_drive_utils_complete_packet(packet_p, FBE_STATUS_OK, 0);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    } else {
        /* if zero on demand flag is not set then we pass all the requests as pass through. */
        //status = fbe_provision_drive_allocate_packet(packet_p, provision_drive_p);
		fbe_transport_set_completion_function(packet_p, fbe_provision_drive_send_packet_completion, provision_drive_p);
		status = FBE_STATUS_CONTINUE;
    }

    return status;
}
/******************************************************************************
 * end fbe_provision_drive_zero_on_demand_process_io_request()
 ******************************************************************************/

static __forceinline fbe_status_t 
fbe_provision_drive_reinit_packet_from_master_packet(fbe_provision_drive_t * provision_drive_p,
                                                   fbe_packet_t * packet_p)
{
    fbe_packet_t * master_packet_p;
    fbe_payload_ex_t * payload_p, *master_payload_p;
    fbe_sg_element_t * sg_list = NULL;
    fbe_raid_iots_t * iots_p = NULL;

    /* Need to reset the sg list from master packet */
    master_packet_p = fbe_transport_get_master_packet(packet_p);
    if (master_packet_p == NULL) {
        fbe_provision_drive_utils_trace(provision_drive_p,
                                        FBE_TRACE_LEVEL_CRITICAL_ERROR,
                                        FBE_TRACE_MESSAGE_ID_INFO,
                                        FBE_PROVISION_DRIVE_DEBUG_FLAG_ZOD_TRACING,
                                        "%s packet %p has no master packet.\n", __FUNCTION__, packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    master_payload_p = fbe_transport_get_payload_ex(master_packet_p);
    fbe_payload_ex_get_sg_list(master_payload_p, &sg_list, NULL);
    payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_payload_ex_set_sg_list(payload_p, sg_list, 1);

    packet_p->packet_attr = 0;
    fbe_transport_set_master_packet(master_packet_p, packet_p);

    /* Propagate encryption key_handle */
    packet_p->payload_union.payload_ex.key_handle = master_packet_p->payload_union.payload_ex.key_handle;

    /* if it is not quiesce then set the iots status as not used. */
    fbe_payload_ex_get_iots(payload_p, &iots_p);
    fbe_raid_iots_set_as_not_used(iots_p);

    return FBE_STATUS_OK;
}
#if 0
/*!****************************************************************************
 * @fn fbe_provision_drive_zero_on_demand_paged_metadata_read_callback()
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
 *  04/22/2014 - Created. Lili Chen
 *
 ******************************************************************************/
fbe_status_t 
fbe_provision_drive_zero_on_demand_paged_metadata_read_callback(fbe_packet_t * packet, fbe_metadata_callback_context_t context)
{
    fbe_payload_ex_t * sep_payload = fbe_transport_get_payload_ex(packet);
    fbe_payload_metadata_operation_t * mdo = NULL;
    fbe_sg_element_t * sg_list = NULL;
    fbe_sg_element_t * sg_ptr = NULL;
    fbe_lba_t lba_offset;
    fbe_u32_t slot_offset;
    fbe_u32_t dst_size;
    fbe_u8_t * dst_ptr;
    fbe_u8_t * data_ptr;

    if(sep_payload->current_operation->payload_opcode == FBE_PAYLOAD_OPCODE_METADATA_OPERATION){
        mdo = fbe_payload_ex_get_metadata_operation(sep_payload);
    } else if(sep_payload->current_operation->payload_opcode == FBE_PAYLOAD_OPCODE_STRIPE_LOCK_OPERATION){
        mdo = fbe_payload_ex_get_any_metadata_operation(sep_payload);
    } else {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    lba_offset = fbe_metadata_paged_get_lba_offset(mdo);
    slot_offset = (fbe_u32_t)fbe_metadata_paged_get_slot_offset(mdo);
    sg_list = mdo->u.metadata_callback.sg_list;
    sg_ptr = &sg_list[lba_offset - mdo->u.metadata_callback.start_lba];
    data_ptr = sg_ptr->address + slot_offset;
    dst_size = (fbe_u32_t)(mdo->u.metadata_callback.record_data_size * mdo->u.metadata_callback.repeat_count);
    dst_ptr = mdo->u.metadata.record_data;

    if(slot_offset + dst_size <= FBE_METADATA_BLOCK_DATA_SIZE){
        fbe_copy_memory(dst_ptr, data_ptr, dst_size);
    } else {
        fbe_copy_memory(dst_ptr, data_ptr, FBE_METADATA_BLOCK_DATA_SIZE - slot_offset);
        dst_ptr += FBE_METADATA_BLOCK_DATA_SIZE - slot_offset;
        sg_ptr++;
        data_ptr = sg_ptr->address;
        fbe_copy_memory(dst_ptr, data_ptr, dst_size - (FBE_METADATA_BLOCK_DATA_SIZE - slot_offset));
    }

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_zero_on_demand_paged_metadata_read_callback()
 ******************************************************************************/
#endif
/*!****************************************************************************
 *  fbe_provision_drive_zero_on_demand_paged_metadata_read()
 ******************************************************************************
 * @brief
 *  This function is used to issue the paged metadata read operation for the
 *  nonzero request.
 *
 * @param provision_drive_p  - pointer to the provision drive object
 * @param packet_p           - pointer to a control packet from the scheduler
 *
 * @return fbe_status_t.     - status of the operation.
 *
 * @author
 *   07/14/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_zero_on_demand_paged_metadata_read(fbe_packet_t * packet_p, fbe_provision_drive_t * provision_drive_p)
{
    fbe_payload_metadata_operation_t *      metadata_operation_p = NULL;
    fbe_payload_ex_t *                     sep_payload_p = NULL;
    fbe_u64_t                               metadata_offset = FBE_LBA_INVALID;
    fbe_lba_t                               start_lba;
    fbe_block_count_t                       block_count;
    fbe_chunk_index_t                       start_chunk_index;
    fbe_chunk_count_t                       chunk_count;
    fbe_payload_block_operation_t *         block_operation_p = NULL;
    fbe_lba_t                               metadata_start_lba;
    fbe_block_count_t                       metadata_block_count;

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
                                          fbe_provision_drive_zero_on_demand_paged_metadata_read_completion,
                                          provision_drive_p);

    /* Allocate the metadata operation. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p); 
    metadata_operation_p =  fbe_payload_ex_allocate_metadata_operation(sep_payload_p);

    /* Build the paged metadata read request. */
#if 0
    fbe_payload_metadata_build_paged_get_bits(metadata_operation_p,
                                              &(((fbe_base_config_t *) provision_drive_p)->metadata_element),
                                              metadata_offset,
                                              NULL,
                                              chunk_count * sizeof(fbe_provision_drive_paged_metadata_t),
                                              sizeof(fbe_provision_drive_paged_metadata_t));
#else
    fbe_payload_metadata_build_paged_read(metadata_operation_p,
                                          &(((fbe_base_config_t *) provision_drive_p)->metadata_element),
                                          metadata_offset,
                                          sizeof(fbe_provision_drive_paged_metadata_t),
                                          chunk_count,
                                          fbe_provision_drive_metadata_paged_callback_read,
                                          (void *)metadata_operation_p);
    /* Initialize cient_blob, skip the subpacket */
    fbe_provision_drive_metadata_paged_init_client_blob(packet_p, metadata_operation_p);
#endif

    /* Get the metadata stripe lock*/
    fbe_metadata_paged_get_lock_range(metadata_operation_p, &metadata_start_lba, &metadata_block_count);

    fbe_payload_metadata_set_metadata_stripe_offset(metadata_operation_p, metadata_start_lba);
    fbe_payload_metadata_set_metadata_stripe_count(metadata_operation_p, metadata_block_count);

    fbe_payload_ex_increment_metadata_operation_level(sep_payload_p);

    /* Issue the metadata operation. */
    //status =  fbe_metadata_operation_entry(packet_p);
    fbe_transport_set_completion_function(packet_p, fbe_metadata_operation_entry_completion, provision_drive_p);
    return FBE_STATUS_CONTINUE;
}
/******************************************************************************
 * end fbe_provision_drive_zero_on_demand_paged_metadata_read()
 ******************************************************************************/
/*!**************************************************************
 * fbe_provision_drive_send_packet_completion()
 ****************************************************************
 * @brief
 *  This is a completion fn that will allocate a new block op
 *  fill it in with a copy of the present block op in this packet,
 *  and then send it out.
 *
 * @param packet
 * @param context
 * 
 * @return fbe_status_t  
 *
 * @author
 *  9/17/2012 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t 
fbe_provision_drive_send_packet_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
	fbe_provision_drive_t *provision_drive_p = (fbe_provision_drive_t *)context;
    fbe_payload_ex_t *payload = NULL;
    fbe_payload_block_operation_t * block_operation_p = NULL;
    fbe_payload_block_operation_t * new_block_operation_p = NULL;
    fbe_optimum_block_size_t optimum_block_size;

    /* get the previous block operation. */
	payload = fbe_transport_get_payload_ex(packet);
    block_operation_p =  fbe_payload_ex_get_present_block_operation(payload);
    new_block_operation_p = fbe_payload_ex_allocate_block_operation(payload);
    
    fbe_block_transport_edge_get_optimum_block_size(&provision_drive_p->block_edge, &optimum_block_size);
    fbe_payload_block_build_operation(new_block_operation_p,
                                      block_operation_p->block_opcode,
                                      block_operation_p->lba,
                                      block_operation_p->block_count,
                                      block_operation_p->block_size,
                                      optimum_block_size,
                                      block_operation_p->pre_read_descriptor);

    new_block_operation_p->block_flags = block_operation_p->block_flags;
    new_block_operation_p->payload_sg_descriptor = block_operation_p->payload_sg_descriptor;

    /* initialize the block operation with default status. */
    fbe_payload_block_set_status(block_operation_p,
                                 FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID,
                                 FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID);

    fbe_transport_set_completion_function(packet, fbe_provision_drive_release_block_op_completion, provision_drive_p);
	fbe_block_transport_send_functional_packet(&provision_drive_p->block_edge, packet);
	return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************
 * end fbe_provision_drive_send_packet_completion()
 ******************************************/
/*!**************************************************************
 * fbe_provision_drive_metadata_check_paged()
 ****************************************************************
 * @brief
 *  Look at the paged and decide what action to take for this I/O.
 *
 * @param paged_data_bits_p
 * @param chunk_count
 * @param b_is_paged_data_valid
 * @param b_is_zero_required
 * @param b_is_consumed
 *
 * @return None
 *
 * @author
 *  3/13/2013 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_provision_drive_metadata_check_paged(fbe_provision_drive_t *provision_drive_p,
                                              fbe_packet_t *packet_p,
                                              fbe_provision_drive_paged_metadata_t *paged_data_bits_p,
                                              fbe_bool_t *b_is_paged_data_valid_p,
                                              fbe_bool_t *b_is_zero_required_p,
                                              fbe_bool_t *b_is_consumed_p)
{
    fbe_chunk_index_t chunk_index = 0;
    fbe_payload_block_operation_t *block_operation_p = NULL;
    fbe_payload_ex_t *sep_payload_p = NULL;
    fbe_lba_t start_lba;
    fbe_chunk_index_t start_chunk_index;
    fbe_chunk_count_t chunk_count;
    fbe_block_count_t block_count;
    fbe_payload_block_operation_opcode_t block_operation_opcode;
 
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p =  fbe_payload_ex_get_present_block_operation(sep_payload_p);

    fbe_payload_block_get_lba(block_operation_p, &start_lba);
    fbe_payload_block_get_block_count(block_operation_p, &block_count);
    fbe_payload_block_get_opcode(block_operation_p, &block_operation_opcode);

    /* Get the start chunk index and chunk count from the start lba and end lba of the range. */
    fbe_provision_drive_utils_calculate_chunk_range(start_lba,
                                                    block_count,
                                                    FBE_PROVISION_DRIVE_CHUNK_SIZE,
                                                    &start_chunk_index,
                                                    &chunk_count);

	*b_is_zero_required_p = FBE_FALSE;
    /* First check to see if our paged is valid.
     */
    fbe_provision_drive_metadata_is_paged_metadata_valid(start_lba, block_count, paged_data_bits_p, b_is_paged_data_valid_p);

    if (*b_is_paged_data_valid_p == FBE_FALSE)
    {
        return;
    }

    /* Next decide which action to take.
     */
    for(chunk_index = 0; chunk_index < chunk_count; chunk_index++)
    {
        fbe_provision_drive_utils_trace(provision_drive_p,
                                        FBE_TRACE_LEVEL_DEBUG_HIGH,
                                        FBE_TRACE_MESSAGE_ID_INFO,
                                        FBE_PROVISION_DRIVE_DEBUG_FLAG_ZOD_TRACING,
                                        "ZOD:md read complete, op:0x%x, lba 0x%llx, blk: 0x%llx, chk_idx 0x%llx, NZ %d, UZ %d, CU %d\n",
                                        block_operation_opcode,
                                        (unsigned long long)start_lba,
                                        (unsigned long long)block_count,
                                        (unsigned long long)(start_chunk_index + chunk_index),
                                        ((paged_data_bits_p[chunk_index].need_zero_bit == FBE_TRUE) ? 1 : 0),
                                        ((paged_data_bits_p[chunk_index].user_zero_bit == FBE_TRUE) ? 1 : 0),
                                        ((paged_data_bits_p[chunk_index].consumed_user_data_bit == FBE_TRUE) ? 1 : 0));

#if 0
       /* Add a validation that PVD should not receive any read or write IO on unconsumed space on non system
        * drives, currently this code is not completely tested and we may have test code which directly send IO
        * to the PVD without binding luns so need to disable it for now
        */
       if((paged_data_bits_p[chunk_index].consumed_user_data_bit == FBE_FALSE) &&
                 (paged_data_bits_p[chunk_index].user_zero_bit  == FBE_FALSE)&&
                 (!fbe_payload_block_is_flag_set(block_operation_p, FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_FORCED_WRITE))&&
                 !fbe_database_is_object_system_pvd(object_id)) 
        {
            fbe_trace_level_t trace_level = FBE_TRACE_LEVEL_CRITICAL_ERROR;

            /*! @note We allow a setting of: NZ: 0, UZ: 0 and CU: 0 if the 
             *        `forced write' flag is set.  Otherwise PVD will not handle
             *        read or write IO on unconsumed area with (UZ :0 AND CU : 0)
             *        this combination
             *    Following are not valid cases for read and write on non system drives
             *
             *    NZ CU UZ
             *    0  0   0
             *    1  0   0 
             */
 
            fbe_provision_drive_utils_trace(provision_drive_p,
                                            trace_level,
                                            FBE_TRACE_MESSAGE_ID_INFO,
                                            FBE_PROVISION_DRIVE_DEBUG_FLAG_ZOD_TRACING,
                                            "ZOD:md read complete, op:0x%x, lba 0x%llx, blk: 0x%llx, chk_idx 0x%llx, CU and UZ of 0 occurred.\n",
                                            block_operation_opcode,
                                            start_lba, block_count, (start_chunk_index + chunk_index));
        }
#endif    

		if((paged_data_bits_p[chunk_index].need_zero_bit == FBE_TRUE) || 
           (paged_data_bits_p[chunk_index].user_zero_bit == FBE_TRUE)    )
        {
			*b_is_zero_required_p = FBE_TRUE;
		}
        else if ((paged_data_bits_p[chunk_index].consumed_user_data_bit == FBE_FALSE) &&
                 (*b_is_consumed_p == FBE_TRUE)                                     )
        {
            /*! @note We allow a setting of: NZ: 0, UZ: 0 and CU: 0 if the 
             *        `forced write' flag is set.  Otherwise this combination
             *        of metadata bits should not occur.  If this flag is not
             *        set we generate an error trace we continue and will handle
             *        this case so that we prevent data corruption.
             */
            if (!fbe_payload_block_is_flag_set(block_operation_p, FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_FORCED_WRITE))
            {
                fbe_trace_level_t trace_level = FBE_TRACE_LEVEL_ERROR;
#if DBG
                trace_level = FBE_TRACE_LEVEL_CRITICAL_ERROR;
#endif
                /* This is unexpected and could result in data corruption.  Trace a critical error in checked builds so
                 * we can investigate. 
                 *  
                 * In free builds we will handle this by treating the paged metadata as in error. 
                 * This causes us to invalidate and move on.
                 */
                fbe_provision_drive_utils_trace(provision_drive_p, trace_level, FBE_TRACE_MESSAGE_ID_INFO,
                                                FBE_PROVISION_DRIVE_DEBUG_FLAG_ZOD_TRACING,
                                                "ZOD:md read complete, op:0x%x, lba 0x%llx, blk: 0x%llx, chk_idx 0x%llx, CU of 0 occurred.\n",
                                                block_operation_opcode,
                                                start_lba, block_count, (start_chunk_index + chunk_index));
                /* Mark this page as NOT valid.  This allows us to re-use the code that handles lost paged.
                 */
                paged_data_bits_p[chunk_index].valid_bit = 0;
                *b_is_paged_data_valid_p = FBE_FALSE;
            }
            else
            {
                /* Else flag the fact that we need to execute the zero on demand 
                 * (i.e. we must perform the I/O not just update the CU bit) by 
                 * setting b_is_zero_required to True.
                 */
                *b_is_zero_required_p = FBE_TRUE;
            }

            /* Flag the fact that `consumed' is not set.
             */
            *b_is_consumed_p = FBE_FALSE;
        }
    }
}
/******************************************
 * end fbe_provision_drive_metadata_check_paged()
 ******************************************/
/*!****************************************************************************
 *  fbe_provision_drive_zero_on_demand_paged_metadata_read_completion()
 ******************************************************************************
 * @brief
 *  This function handles paged metadata read completion.
 *
 *  It verifies whether NZ bit or (UZ bit and U bit) is set for the chunks
 *  correpond to the original i/o request, if it is set then it allocates the
 *  buffer with number of chunks size and copies write buffer to newly created
 *  zeroed buffer before it issues original block request.
 *
 * @param packet_p          - pointer to a control packet from the scheduler
 * @param in_context        - context, which is a pointer to the provision drive
 *                            object
 *
 * @return fbe_status_t.     - status of the operation.
 *
 * @author
 *   07/14/2010 - Created. Dhaval Patel
 *   03/09/2012 - Ashok Tamilarasan - Added metadata error handling checks
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_zero_on_demand_paged_metadata_read_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{
    fbe_provision_drive_t *                             provision_drive_p = NULL;
    fbe_payload_metadata_operation_t *                  metadata_operation_p = NULL;
    fbe_payload_block_operation_t *                     block_operation_p = NULL;
    fbe_payload_ex_t *                                  sep_payload_p = NULL;
    fbe_status_t                                        status = FBE_STATUS_OK;
    fbe_provision_drive_paged_metadata_t *              paged_data_bits_p = NULL;
    fbe_payload_metadata_status_t                       metadata_status;
    fbe_payload_block_operation_opcode_t                block_operation_opcode;
	fbe_bool_t											b_is_zero_required = FBE_FALSE;
    fbe_bool_t                                          b_is_consumed = FBE_TRUE;  
    fbe_bool_t                                          b_is_paged_data_valid;
    fbe_provision_drive_config_type_t                   config_type;
    fbe_object_id_t                                     object_id;
    fbe_bool_t                                          b_is_system_drive;

    provision_drive_p = (fbe_provision_drive_t *) context;
    //fbe_provision_drive_reinit_packet_from_master_packet(provision_drive_p, packet_p);
    
    fbe_provision_drive_get_config_type(provision_drive_p, &config_type);
    fbe_base_object_get_object_id((fbe_base_object_t *)provision_drive_p, &object_id);
    b_is_system_drive = fbe_database_is_object_system_pvd(object_id);

    /* If unconsummed and not a system drive, then mark as unconsumed.*/
    if (config_type == FBE_PROVISION_DRIVE_CONFIG_TYPE_UNCONSUMED &&
        b_is_system_drive == FBE_FALSE)
    {
        b_is_consumed = FBE_FALSE;
    }

    /* Get the payload and metadata operation. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    metadata_operation_p =  fbe_payload_ex_get_metadata_operation(sep_payload_p);

    /* get the previous block operation. */
    block_operation_p =  fbe_payload_ex_get_present_block_operation(sep_payload_p);
    fbe_payload_block_get_opcode(block_operation_p, &block_operation_opcode);

    /* Get the status of the read paged metadata operation. */
    fbe_payload_metadata_get_status(metadata_operation_p, &metadata_status);
    status = fbe_transport_get_status_code(packet_p);

    /* We hit the paged cache and no need to check the paged metadata. */
    if (status == FBE_STATUS_OK && metadata_status == FBE_PAYLOAD_METADATA_STATUS_PAGED_CACHE_HIT)
    {
        fbe_provision_drive_utils_trace(provision_drive_p,
                                        FBE_TRACE_LEVEL_DEBUG_LOW,
                                        FBE_TRACE_MESSAGE_ID_INFO,
                                        FBE_PROVISION_DRIVE_DEBUG_FLAG_ZOD_TRACING,
                                        "ZOD:md read complete cache hit packet %p opcode 0x%x.\n",
                                        packet_p, block_operation_opcode);

        fbe_payload_ex_release_metadata_operation(sep_payload_p, metadata_operation_p);

	    if(block_operation_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_CHECK_ZEROED)
        {
            fbe_payload_block_set_status(block_operation_p,
                                         FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS,
                                         FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NOT_ZEROED);
            fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
            //fbe_transport_complete_packet_async(packet_p);
            return FBE_STATUS_OK;
	    }

        fbe_provision_drive_zero_on_demand_bundle_start(sep_payload_p, provision_drive_p);
        fbe_transport_set_completion_function(packet_p, fbe_provision_drive_send_packet_completion, provision_drive_p);
        return FBE_STATUS_CONTINUE;
    }

    /* type cast the metadata to provison drive paged metadata. */
    fbe_payload_metadata_get_metadata_record_data_ptr(metadata_operation_p, (void **)&paged_data_bits_p);

    /* Based on the metadata status set the appropriate block status. */
    if((status != FBE_STATUS_OK) || (metadata_status != FBE_PAYLOAD_METADATA_STATUS_OK))
    {
        fbe_provision_drive_utils_trace(provision_drive_p,
                                        FBE_TRACE_LEVEL_WARNING,
                                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                        FBE_PROVISION_DRIVE_DEBUG_FLAG_ZOD_TRACING,
                                        "ZOD:md read complete, failed - metadata_status:0x%x, status:0x%x.\n",
                                        metadata_status, status);

        /* We need to handle uncorrectable seperately because that is the case where data cannot be read from media
         * For other cases, return back to the client for retrying e.g. We would have
         * quiesced and hence gotten an error from Metadata
         */
        if(metadata_status == FBE_PAYLOAD_METADATA_STATUS_IO_UNCORRECTABLE)
        {
            /* The read of Metadata failed let the below function determine what action to take based on the payload
             * opcode
             */
            status = fbe_provision_drive_zero_on_demand_process_paged_metadata_bits(provision_drive_p, paged_data_bits_p, packet_p);
            return FBE_STATUS_MORE_PROCESSING_REQUIRED;
        }
        else
        {
            fbe_payload_ex_release_metadata_operation(sep_payload_p, metadata_operation_p);

            /* Even non-retryable errors back to raid must be acompanied by a state change. 
             * Since we are not guaranteed to be getting a state change 
             * (since it might be caused by a quiesce), we must go ahead and return this as retryable.
             */
            fbe_payload_block_set_status(block_operation_p,
                                         FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED,
                                         FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_POSSIBLE);
            fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
            return status;
        }
    }

    /* Check the paged to see what we need to do.
     */
    fbe_provision_drive_metadata_check_paged(provision_drive_p,
                                             packet_p,
                                             paged_data_bits_p,
                                             &b_is_paged_data_valid,
                                             &b_is_zero_required,
                                             &b_is_consumed);

    /* Check if the paged data is valid before processing it. If it is not, the handling depends on
     * on the type of operation
     */
    if(!b_is_paged_data_valid)
    {
        status = fbe_provision_drive_zero_on_demand_process_paged_metadata_bits(provision_drive_p, paged_data_bits_p, packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }


	/*! @todo handle this opcode here */
	if(block_operation_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_CHECK_ZEROED)
    {
		b_is_zero_required = FBE_TRUE;
	}

	/* If need zero and user zero are not set we should pass throu */
	if (b_is_zero_required)
    {
		/* process the paged data bits which we just read using metadata service. */
		status = fbe_provision_drive_zero_on_demand_process_paged_metadata_bits(provision_drive_p, paged_data_bits_p, packet_p);
		status = FBE_STATUS_MORE_PROCESSING_REQUIRED;
	} 
    else if (b_is_consumed == FBE_FALSE)
    {
        /* Although we don't need to execute a ZOD, we need to set the DB bit. */
        status = fbe_provision_drive_zero_on_demand_write_update_paged_metadata(provision_drive_p, packet_p);
		status = FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }
    else 
    {
		
		fbe_payload_ex_release_metadata_operation(sep_payload_p, metadata_operation_p);

		/* If we here - we have 1MB lock for the chunk and we do not have to do ZOD or Zero fill.
			Let's see if we have another I/O's waiting for this 1MB stripe lock and just let them go.
		*/
		fbe_provision_drive_zero_on_demand_bundle_start(sep_payload_p, provision_drive_p);

        fbe_transport_set_completion_function(packet_p, fbe_provision_drive_send_packet_completion, provision_drive_p);
		status = FBE_STATUS_CONTINUE;
	}

    return status;
}

/******************************************************************************
 * end fbe_provision_drive_zero_on_demand_paged_metadata_read_completion()
 ******************************************************************************/

/*!****************************************************************************
 *  fbe_provision_drive_zero_on_demand_process_paged_metadata_bits()
 ******************************************************************************
 * @brief
 *  This function process the paged data bits which we read using metadata
 *  service, it determines which chunks needs to be zeroed and takes appropriate
 *  action based on that.
 *
 * @param provision_drive_p     - pointer to provision drove object.
 * @param paged_data_bits_p     - pointer to paged data bits.
 * @param packet_p              - pointer to packet.
 *
 * @return fbe_status_t.        - status of the operation.
 *
 * @author
 *   07/14/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_zero_on_demand_process_paged_metadata_bits(fbe_provision_drive_t * provision_drive_p,
                                                               fbe_provision_drive_paged_metadata_t * paged_data_bits_p,
                                                               fbe_packet_t * packet_p)
{
    fbe_payload_ex_t *                     sep_payload_p = NULL;
    fbe_payload_block_operation_t *         block_operation_p = NULL;
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_payload_block_operation_opcode_t    block_operation_opcode;

    /* get the payload and prev block operation. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);   
    block_operation_p =  fbe_payload_ex_get_present_block_operation(sep_payload_p);

    /* get the lba and block count from the previous block operation. */
    fbe_payload_block_get_opcode(block_operation_p, &block_operation_opcode);

    /* take the different path for the write and read request. */
    if(fbe_payload_block_operation_opcode_is_media_modify(block_operation_opcode))
    {
        /* process the incoming zero on demand write request. */
        status = fbe_provision_drive_zero_on_demand_write_process_paged_metadata_bits(provision_drive_p,
                                                                                      paged_data_bits_p,
                                                                                      packet_p);
    }
    else if (fbe_payload_block_operation_opcode_is_media_read(block_operation_opcode))
    {
        /* process the incoming zero on demand read request. */
        status = fbe_provision_drive_zero_on_demand_read_process_paged_metadata_bits(provision_drive_p,
                                                                                     paged_data_bits_p,
                                                                                     packet_p);
    }
    else if (block_operation_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_CHECK_ZEROED)
    {
        /* process the incoming check zeroed request. */
        status = fbe_provision_drive_zero_on_demand_check_zeroed_process_paged_metadata(provision_drive_p,
                                                                                        paged_data_bits_p,
                                                                                        packet_p);
    }
    return status;
}
/******************************************************************************
 * end fbe_provision_drive_zero_on_demand_process_paged_metadata_bits()
 ******************************************************************************/

/*!****************************************************************************
 *  fbe_provision_drive_zero_on_demand_write_process_paged_metadata_bits()
 ******************************************************************************
 * @brief
 *  This function process the paged data bits for the zero on demand write 
 *  request, it determines whether we need zero for the unaligned edge range 
 *  and if required then initiate the zero on demand processing for write 
 *  request.
 *
 * @param provision_drive_p     - pointer to provision drove object.
 * @param paged_data_bits_p     - pointer to paged data bits.
 * @param packet_p              - pointer to packet.
 *
 * @return fbe_status_t.        - status of the operation.
 *
 * @author
 *   07/14/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_zero_on_demand_write_process_paged_metadata_bits(fbe_provision_drive_t * provision_drive_p,
                                                                     fbe_provision_drive_paged_metadata_t * paged_data_bits_p,
                                                                     fbe_packet_t * packet_p)
{
    fbe_payload_ex_t *                 sep_payload_p = NULL;
    fbe_payload_block_operation_t *     block_operation_p = NULL;
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_chunk_count_t                   number_of_edge_chunks_need_process = 0;
    fbe_lba_t                           start_lba;
    fbe_block_count_t                   block_count;
    fbe_bool_t                          is_paged_data_valid;

    /* get the payload and previous block operation. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p =  fbe_payload_ex_get_present_block_operation(sep_payload_p);

    /* get the lba and block count from the previous block operation. */
    fbe_payload_block_get_lba(block_operation_p, &start_lba);
    fbe_payload_block_get_block_count(block_operation_p, &block_count);

    fbe_provision_drive_metadata_is_paged_metadata_valid(start_lba,
                                                         block_count, paged_data_bits_p, &is_paged_data_valid);

    if(!is_paged_data_valid)
    {
        /* get the number of boundary edge range which is not aligned to chunk and need invalidate. */
        fbe_provision_drive_utils_calc_number_of_edge_chunks_needs_process(start_lba,
                                                                           block_count,
                                                                           FBE_PROVISION_DRIVE_CHUNK_SIZE,
                                                                           paged_data_bits_p,                                              
                                                                           fbe_provision_drive_bitmap_is_paged_data_invalid,
                                                                           &number_of_edge_chunks_need_process);
    }
    else
    {
        /* get the number of boundary edge range which is not aligned to chunk and need zero. */
        fbe_provision_drive_utils_calc_number_of_edge_chunks_needs_process(start_lba,
                                                                           block_count,
                                                                           FBE_PROVISION_DRIVE_CHUNK_SIZE,
                                                                           paged_data_bits_p,
                                                                           fbe_provision_drive_bitmap_is_chunk_marked_for_zero,
                                                                           &number_of_edge_chunks_need_process);

    }

    /* if we find that there is no edge (lba range not aligned to chunk size) which needs to be
     * processed then directly send original i/o request without zero on demand or invalidate process.
     */
    if(!number_of_edge_chunks_need_process)
    {
        /* We still need to come back and handle the paged metadata bits, so setup a completion function
         */
        fbe_transport_set_completion_function(packet_p,
                                              fbe_provision_drive_zero_on_demand_write_update_metadata_after_write_completion,
                                              provision_drive_p);
        status = fbe_provision_drive_zero_on_demand_write_slow_path_write_start(provision_drive_p, packet_p);
        return status;
    }

    if(!is_paged_data_valid)
    {
        fbe_provision_drive_utils_trace(provision_drive_p,
                                        FBE_TRACE_LEVEL_INFO,
                                        FBE_TRACE_MESSAGE_ID_INFO,
                                        FBE_PROVISION_DRIVE_DEBUG_FLAG_ZOD_TRACING,
                                        "%s: MD page invalid.\n", __FUNCTION__);
        /* We are not able to read the paged metadata, so we will need to invalidate the region
         * setup the completion function to update the paged metadata after the actual write completes
         */
        fbe_transport_set_completion_function(packet_p,
                                              fbe_provision_drive_zero_on_demand_update_metadata_after_invalidated_write_completion,
                                              provision_drive_p);
    }
    else
    {
        /* before we start zero on demand processing, set the completion function to 
         * update the page metadata after zero on demand operation completes.
         */
        fbe_transport_set_completion_function(packet_p,
                                              fbe_provision_drive_zero_on_demand_write_update_metadata_after_write_completion,
                                              provision_drive_p);
    }

    /* start zero on demand processing. */
    status = fbe_provision_drive_zero_on_demand_write_start_zero_on_demand(provision_drive_p, packet_p);
    return status;
}
/******************************************************************************
 * end fbe_provision_drive_zero_on_demand_write_process_paged_metadata_bits()
 ******************************************************************************/

/*!****************************************************************************
 *  fbe_provision_drive_zero_on_demand_check_zeroed_process_paged_metadata()
 ******************************************************************************
 * @brief
 *  This function process the paged data bits for the check zeroed request,
 *  It simply searches to see if any chunk is not zeroed.
 *
 * @param provision_drive_p     - pointer to provision drove object.
 * @param paged_data_bits_p     - pointer to paged data bits.
 * @param packet_p              - pointer to packet.
 *
 * @return fbe_status_t.        - status of the operation.
 *
 * @author
 *   9/29/2010 - Created. Rob Foley
 *   03/06/2012 - Ashok Tamilarasan - Updated to look for valid bits
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_zero_on_demand_check_zeroed_process_paged_metadata(fbe_provision_drive_t * provision_drive_p,
                                                                       fbe_provision_drive_paged_metadata_t * paged_data_bits_p,
                                                                       fbe_packet_t * packet_p)
{
    fbe_payload_ex_t *                 sep_payload_p = NULL;
    fbe_payload_block_operation_t *     block_operation_p = NULL;
    fbe_payload_metadata_operation_t *  metadata_operation_p = NULL;
    fbe_lba_t                           start_lba;
    fbe_block_count_t                   block_count;
    fbe_bool_t                          b_is_media_read_needed = FBE_FALSE;
    fbe_bool_t                          is_paged_data_valid = FBE_FALSE;

    /* get the payload and previous block operation. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    metadata_operation_p = fbe_payload_ex_get_metadata_operation(sep_payload_p);
    block_operation_p =  fbe_payload_ex_get_present_block_operation(sep_payload_p);

    /* get the lba and block count from the previous block operation. */
    fbe_payload_block_get_lba(block_operation_p, &start_lba);
    fbe_payload_block_get_block_count(block_operation_p, &block_count);
    fbe_payload_ex_release_metadata_operation(sep_payload_p, metadata_operation_p);

    fbe_provision_drive_metadata_is_paged_metadata_valid(start_lba, block_count, paged_data_bits_p, &is_paged_data_valid);

    /* Check if the paged data is valid or not
     */
    if(is_paged_data_valid)
    {
        /* Determine if media read is needed to satisfy the original host request.
         * If a media read is needed then it means we are not zeroed. 
         */
        fbe_provision_drive_zero_on_demand_read_determine_if_media_read_needed(start_lba,
                                                                               block_count,
                                                                               paged_data_bits_p,
                                                                               &b_is_media_read_needed);
    }
    else
    {
        //  Set the condition to start the verify invaldiate operation, because we hit some bad blocks
        // in the paged metadata region that must be verified
        fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const, (fbe_base_object_t *) provision_drive_p,
                               FBE_PROVISION_DRIVE_LIFECYCLE_COND_METADATA_VERIFY_INVALIDATE);
    }

    fbe_provision_drive_utils_trace(provision_drive_p,
                                    FBE_TRACE_LEVEL_DEBUG_LOW,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    FBE_PROVISION_DRIVE_DEBUG_FLAG_ZOD_TRACING,
                                    "%s: UNMAP check zero media read needed %d valid %d 0x%llx.\n", __FUNCTION__, 
                                    b_is_media_read_needed,
                                    is_paged_data_valid, start_lba);

    if ((b_is_media_read_needed) ||
         (!is_paged_data_valid)){ 
        /* If media read is needed or we cannot determine the state of the media, just return as 
         * not zeroed just to be on the safe side
         */
        fbe_payload_block_set_status(block_operation_p,
                                     FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS,
                                     FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NOT_ZEROED);
    }
    else { /* Area is zeroed. */
        fbe_payload_block_set_status(block_operation_p,
                                     FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS,
                                     FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_ZEROED);
    }

    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet_async(packet_p);

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_zero_on_demand_check_zeroed_process_paged_metadata()
 ******************************************************************************/

/*!****************************************************************************
 *  fbe_provision_drive_zero_on_demand_read_process_paged_metadata_bits()
 ******************************************************************************
 * @brief
 *  This function process the paged data bits for the zero on demand read 
 *  request, it determines the number of read request it needs to send down
 *  based on zero map and request to allocate the subpacket for that.
 *
 * @param provision_drive_p     - pointer to provision drove object.
 * @param paged_data_bits_p     - pointer to paged data bits.
 * @param packet_p              - pointer to packet.
 *
 * @return fbe_status_t.        - status of the operation.
 *
 * @author
 *   07/14/2010 - Created. Dhaval Patel
 *   03/09/2012 - Ashok Tamilarasan - Added metadata error handling checks
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_zero_on_demand_read_process_paged_metadata_bits(fbe_provision_drive_t * provision_drive_p,
                                                                    fbe_provision_drive_paged_metadata_t * paged_data_bits_p,
                                                                    fbe_packet_t * packet_p)
{
    fbe_payload_ex_t *                 sep_payload_p = NULL;
    fbe_payload_block_operation_t *     block_operation_p = NULL;
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_u32_t                           number_of_memory_chunks = 0;
    fbe_lba_t                           start_lba;
    fbe_block_count_t                   block_count;
    fbe_bool_t                          is_media_read_needed_b = FBE_FALSE;
    fbe_queue_head_t                    provision_drive_read_zod_tmp_queue;
    fbe_bool_t                          is_paged_data_valid;
    fbe_payload_metadata_operation_t    *metadata_operation_p;

    /* get the payload and previous block operation. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p =  fbe_payload_ex_get_present_block_operation(sep_payload_p);

    /* get the lba and block count from the previous block operation. */
    fbe_payload_block_get_lba(block_operation_p, &start_lba);
    fbe_payload_block_get_block_count(block_operation_p, &block_count);

    fbe_provision_drive_metadata_is_paged_metadata_valid(start_lba,
                                                         block_count, paged_data_bits_p, &is_paged_data_valid);
    
    if(!is_paged_data_valid)
    {
        /* Since the metadata for this user chunk is not valid, we dont know the state of the data.
         * Need to invalidate the region and let the upper level know that data is lost in this case
         */
        metadata_operation_p =  fbe_payload_ex_get_metadata_operation(sep_payload_p);
        fbe_payload_ex_release_metadata_operation(sep_payload_p, metadata_operation_p);

         fbe_provision_drive_utils_trace(provision_drive_p,
                                        FBE_TRACE_LEVEL_INFO,
                                        FBE_TRACE_MESSAGE_ID_INFO,
                                        FBE_PROVISION_DRIVE_DEBUG_FLAG_ZOD_TRACING,
                                        "pvd_process_read_paged_bits failed. Kick off md vr.lba:0x%llx, count:%d\n",
                                         start_lba, (int)block_count);

         //  Set the condition to start the verify invaldiate operation, because we hit some bad blocks
         // in the paged metadata region that must be verified
         fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const, (fbe_base_object_t *) provision_drive_p,
                                FBE_PROVISION_DRIVE_LIFECYCLE_COND_METADATA_VERIFY_INVALIDATE);

        /* Since we dont know the state of the user region because of paged metadata read failure
         * return media error so that the client can handle appropriately
         */
         fbe_payload_block_set_status(block_operation_p,
                                      FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR,
                                      FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_DATA_LOST);

         fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
         fbe_transport_complete_packet_async(packet_p);
         return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    /* Determine if media read is needed to satisfy the original host request, if required then
     * allocate the enough memory to send the read operation to disk.
     */
     fbe_provision_drive_zero_on_demand_read_determine_if_media_read_needed(start_lba,
                                                                            block_count,
                                                                            paged_data_bits_p,
                                                                            &is_media_read_needed_b);
   
    /* if Media read is not required or paged record is not valid, there is nothing more to do */
    if(!is_media_read_needed_b)
    {
        /* set the block status as good. */
        fbe_payload_block_set_status(block_operation_p,
                                     FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS,
                                     FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);
        /* initialize the queue. */
        fbe_queue_init(&provision_drive_read_zod_tmp_queue);

        if (fbe_queue_is_element_on_queue(&packet_p->queue_element)){
            fbe_provision_drive_utils_trace(provision_drive_p,
                                            FBE_TRACE_LEVEL_CRITICAL_ERROR,
                                            FBE_TRACE_MESSAGE_ID_INFO,
                                            FBE_PROVISION_DRIVE_DEBUG_FLAG_ZOD_TRACING,
                                            "%s packet %p already on the queue.\n", __FUNCTION__, packet_p);
        }

        /* enqueue this packet temporary queue before we enqueue it to run queue. */
        fbe_queue_push(&provision_drive_read_zod_tmp_queue, &packet_p->queue_element);

        /*!@note Queue this request to run queue to break the i/o context to avoid the stack
         * overflow, later this needs to be queued with the thread running in same core.
         */
        fbe_transport_run_queue_push(&provision_drive_read_zod_tmp_queue, 
                                     fbe_provision_drive_zero_on_demand_read_media_read_completion, 
                                     (fbe_packet_completion_context_t) provision_drive_p);

		fbe_queue_destroy(&provision_drive_read_zod_tmp_queue);
        return FBE_STATUS_OK;
    }

    /* Before we determine whether we need actual disk read to satisfy the original host
     * request, set the completion function to resume the code path where zero on demand
     * code path resumes it read path for doing zero fill operation.
     */
    fbe_transport_set_completion_function(packet_p,
                                          fbe_provision_drive_zero_on_demand_read_media_read_completion,
                                          provision_drive_p);

    /* determine memory requirement for the zero on demand media read request. */
    fbe_provision_drive_zero_on_demand_read_calc_memory_chunks(provision_drive_p,
                                                               packet_p,
                                                               FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO,
                                                               &number_of_memory_chunks);

    status = fbe_provision_drive_zero_on_demand_read_start(provision_drive_p, packet_p);

    return status;
}
/******************************************************************************
 * end fbe_provision_drive_zero_on_demand_read_process_paged_metadata_bits()
 ******************************************************************************/

/*!****************************************************************************
 *  fbe_provision_drive_zero_on_demand_read_start()
 ******************************************************************************
 * @brief
 *  This function handles sending out the zod read.
 *
 * @param memory_request    - Pointer to memory request.
 * @param context           - Pointer to the provision drive object.
 *
 * @return fbe_status_t.    - status of the operation.
 *
 * @author
 *  9/17/2012 - Created. Rob Foley
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_zero_on_demand_read_start(fbe_provision_drive_t * provision_drive_p,
                                              fbe_packet_t * packet_p)
{
    fbe_payload_ex_t *                     sep_payload_p = NULL;
    fbe_payload_block_operation_t *         block_operation_p = NULL;
    fbe_payload_block_operation_t *         new_block_operation_p = NULL;
    fbe_status_t                            status;
    fbe_optimum_block_size_t                optimum_block_size;
    fbe_lba_t                               host_start_lba;
    fbe_block_count_t                       host_block_count;

    /* Get the payload and metadata operation. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);

    /* get the previous block operation. */
    block_operation_p =  fbe_payload_ex_get_present_block_operation(sep_payload_p);

    /* get the optimum block size, start lba and block count. */
    fbe_block_transport_edge_get_optimum_block_size(&provision_drive_p->block_edge, &optimum_block_size);
    fbe_payload_block_get_lba(block_operation_p, &host_start_lba);
    fbe_payload_block_get_block_count(block_operation_p, &host_block_count);
 
    fbe_provision_drive_utils_trace(provision_drive_p,
                                    FBE_TRACE_LEVEL_DEBUG_HIGH,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    FBE_PROVISION_DRIVE_DEBUG_FLAG_ZOD_TRACING,
                                    "READ_ZOD:send read request, next_lba:0x%llx, blks:0x%llx\n",
                                    (unsigned long long)host_start_lba, (unsigned long long)host_block_count);

    /* allocate and initialize the block operation. */
    new_block_operation_p = fbe_payload_ex_allocate_block_operation(sep_payload_p);
    fbe_payload_block_build_operation(new_block_operation_p,
                                      FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                      host_start_lba,
                                      host_block_count,
                                      block_operation_p->block_size,
                                      optimum_block_size,
                                      NULL);    // pre-read descriptor is null for read.

    fbe_payload_block_set_flag(new_block_operation_p,
                               FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_ALLOW_UNMAP_READ);

    /* All this completion needs to do is propagate the status. 
     * The caller already added a completion function to deal with zeroing out any blocks 
     * for chunks that are marked for zeroing. 
     */
    fbe_transport_set_completion_function(packet_p,
                                          fbe_provision_drive_release_block_op_completion,
                                          provision_drive_p);

    fbe_payload_block_set_status(block_operation_p,
                                 FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID,
                                 FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID);
    status = fbe_base_config_send_functional_packet((fbe_base_config_t *)provision_drive_p, packet_p, 0);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * end fbe_provision_drive_zero_on_demand_read_start()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_zero_on_demand_read_media_read_completion()
 ******************************************************************************
 * @brief
 *
 *  This function is used to handle the completion of the media read operation
 *  while handling the read request with zero on demand.
 * 
 * @param packet_p                      - Pointer to packet.
 * @param packet_completion_context     - Pointer to provision drive object.
 *
 * @return fbe_status_t.    - status of the operation.
 *
 * @author
 *   07/14/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_zero_on_demand_read_media_read_completion(fbe_packet_t * packet_p,
                                                              fbe_packet_completion_context_t packet_completion_context)
{
    fbe_provision_drive_t *                             provision_drive_p = NULL;
    fbe_payload_block_operation_t *                     block_operation_p = NULL;
    fbe_payload_metadata_operation_t *                  metadata_operation_p = NULL;
    fbe_payload_block_operation_status_t                block_status;
    fbe_payload_ex_t *                                 sep_payload_p = NULL;
    fbe_status_t                                        status = FBE_STATUS_OK;

    provision_drive_p = (fbe_provision_drive_t *) packet_completion_context;

    /* Get the payload and previous block operation. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    metadata_operation_p = fbe_payload_ex_get_metadata_operation(sep_payload_p);
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

    /* initiate the zero buffer fill operation before we complete the host request. */
    status = fbe_provision_drive_zero_on_demand_read_fill_zero_if_needed(provision_drive_p, packet_p);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * end fbe_provision_drive_zero_on_demand_read_media_read_completion()
 ******************************************************************************/

/*!****************************************************************************
 *  fbe_provision_drive_zero_on_demand_read_fill_zero_if_needed()
 ******************************************************************************
 * @brief
 *  This function is used to fill the zero buffer for the range which is 
 *  already zeroed.
 *
 * @return fbe_status_t.        - status of the operation.
 *
 * @author
 *   07/14/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_zero_on_demand_read_fill_zero_if_needed(fbe_provision_drive_t * provision_drive_p,
                                                            fbe_packet_t * packet_p)
{
    fbe_payload_ex_t *                     sep_payload_p = NULL;
    fbe_payload_block_operation_t *         block_operation_p = NULL;
    fbe_payload_metadata_operation_t *      metadata_operation_p = NULL;
    fbe_provision_drive_paged_metadata_t *  paged_data_bits_p = NULL;
    fbe_status_t                            status;
    fbe_lba_t                               host_start_lba;
    fbe_block_count_t                       host_block_count;
    fbe_sg_element_t *                      host_sg_list;
    fbe_lba_t                               next_need_zero_start_lba;
    fbe_block_count_t                       next_need_zero_block_count = 0;

    /* get the sep payload and host sg list. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_payload_ex_get_sg_list(sep_payload_p, &host_sg_list, NULL);

    /* get the metadata operation previous block operation. */
    metadata_operation_p = fbe_payload_ex_get_metadata_operation(sep_payload_p);
    block_operation_p = fbe_payload_ex_get_present_block_operation(sep_payload_p);

    /* get the host start lba and block count. */
    fbe_payload_block_get_lba(block_operation_p, &host_start_lba);
    fbe_payload_block_get_block_count(block_operation_p, &host_block_count);

    /* get the paged data bits from the metadata operation. */
    fbe_payload_metadata_get_metadata_record_data_ptr(metadata_operation_p, (void **)&paged_data_bits_p);

    /* initialize current and next need zero start lba. */
    next_need_zero_start_lba = host_start_lba;
    next_need_zero_block_count = 0;

    /* determine next lba range which needs media read operation until we go through the host range. */
    while(next_need_zero_start_lba != FBE_LBA_INVALID)
    {
        /* get the next lba range which requires to fill the zero buffer in host range. */
        status = fbe_provision_drive_zero_on_demand_read_get_next_need_zero_lba_range(host_start_lba,
                                                                                      host_block_count,
                                                                                      paged_data_bits_p,
                                                                                      &next_need_zero_start_lba,
                                                                                      &next_need_zero_block_count);
        if(status != FBE_STATUS_OK) {
            /* on error break the look. */
            break;
        }

        if(next_need_zero_start_lba != FBE_LBA_INVALID)
        {
            fbe_provision_drive_utils_trace(provision_drive_p,
                                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                                            FBE_TRACE_MESSAGE_ID_INFO,
                                            FBE_PROVISION_DRIVE_DEBUG_FLAG_ZOD_TRACING,
                                            "READ_ZOD:zero fill, next_lba:0x%llx, blks:0x%llx.\n",
                                            (unsigned long long)next_need_zero_start_lba,
					    (unsigned long long)next_need_zero_block_count);

            /* fill the zero buffer for the next need zero range in host range. */
            status = fbe_provision_drive_zero_on_demand_read_fill_zero_buffer(provision_drive_p,
                                                                              host_sg_list,
                                                                              host_start_lba,
                                                                              next_need_zero_start_lba,
                                                                              next_need_zero_block_count);
            if(status != FBE_STATUS_OK)
            {
                fbe_provision_drive_utils_trace(provision_drive_p,
                                                FBE_TRACE_LEVEL_ERROR,
                                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                                FBE_PROVISION_DRIVE_DEBUG_FLAG_ZOD_TRACING,
                                                "READ_ZOD:zero fill operation failed, status:0x%x\n", status);
                break;
            }
        }
    }

    /* release the metadata operation. */
    fbe_payload_ex_release_metadata_operation(sep_payload_p, metadata_operation_p);

    /* update the block status appropriately. */
    if(status != FBE_STATUS_OK)
    {
        /*!@todo revisit later, it might need to change as retry not possible. */
        fbe_payload_block_set_status(block_operation_p,
                                     FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                                     FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNEXPECTED_ERROR);
    }
    else
    {
        fbe_payload_block_set_status(block_operation_p,
                                     FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS,
                                     FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);
    }


    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);

    return status;
}
/******************************************************************************
 * end fbe_provision_drive_zero_on_demand_read_fill_zero_if_needed()
 ******************************************************************************/

/*!****************************************************************************
 *  fbe_provision_drive_zero_on_demand_read_fill_zero_buffer()
 ******************************************************************************
 * @brief
 *  This function is used to fill the zero buffer for the range which is 
 *  already zeroed.
 *
 * @return fbe_status_t.        - status of the operation.
 *
 * @author
 *   07/14/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_zero_on_demand_read_fill_zero_buffer(fbe_provision_drive_t * provision_drive_p,
                                                         fbe_sg_element_t * host_sg_list,
                                                         fbe_lba_t  host_start_lba,
                                                         fbe_lba_t  next_need_zero_start_lba,
                                                         fbe_block_count_t next_need_zero_block_cout)
{

    fbe_sg_element_with_offset_t    host_sgd;
    fbe_u32_t                       sg_offset;
    fbe_u32_t                       host_sg_offset;
    fbe_u8_t *                      zero_bit_bucket_p = NULL;
    fbe_sg_element_t *              current_sg_element_p = NULL;
    fbe_u32_t                       zero_bit_bucket_size_in_blocks;
    fbe_u32_t                       zero_bit_bucket_size_in_bytes;
    fbe_block_count_t           bytes_left_to_zero_fill;
    fbe_status_t                    status;

    /*!@todo verify next need zero range does not exceed the host lba range. */

    /* get the zero bit bucket address and its size before we fill the zero buffer to host. */
    status = fbe_memory_get_zero_bit_bucket(&zero_bit_bucket_p, &zero_bit_bucket_size_in_blocks);
    if((status != FBE_STATUS_OK) || (zero_bit_bucket_p == NULL))
    {
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    /* conver the zero bit bucket size in bytes. */
    zero_bit_bucket_size_in_bytes = zero_bit_bucket_size_in_blocks * FBE_BE_BYTES_PER_BLOCK;

    /* calculate the sg offset in host sg list. */
    sg_offset = (fbe_u32_t) (next_need_zero_start_lba - host_start_lba) * FBE_BE_BYTES_PER_BLOCK;

    /* initialize the host sg list with offset. */
    fbe_sg_element_with_offset_init(&host_sgd, (fbe_u32_t) sg_offset, host_sg_list, NULL);

    /* Adjust the sg element based on the current offset. */
    fbe_raid_adjust_sg_desc(&host_sgd);

    /* get the current sg element addreess and its offset. */
    current_sg_element_p = host_sgd.sg_element;
    host_sg_offset = host_sgd.offset;

    /* get the number of bytes to zero fill to host buffer. */
    bytes_left_to_zero_fill = next_need_zero_block_cout * FBE_BE_BYTES_PER_BLOCK;

    while(bytes_left_to_zero_fill > 0)
    {
        fbe_u32_t   sg_bytes_to_zero_fill;
        fbe_u8_t *  current_sg_addr = NULL; 

        /* If we hit the end of the sg list or we find a NULL address, then exit with error. */
        if (current_sg_element_p == NULL || 
            fbe_sg_element_address(current_sg_element_p) == NULL ||
            fbe_sg_element_count(current_sg_element_p) == 0)
        {
            fbe_provision_drive_utils_trace(provision_drive_p,
                                            FBE_TRACE_LEVEL_ERROR,
                                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                            FBE_PROVISION_DRIVE_DEBUG_FLAG_ZOD_TRACING,
                                            "READ_ZOD:zero fill, end of SG hit unexpectedly, start_lba:0x%llx, blk:0x%llx.\n",
                                            (unsigned long long)next_need_zero_start_lba,
					    (unsigned long long)next_need_zero_block_cout);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* get the current sg element address with adjusted offset. */
        current_sg_addr = fbe_sg_element_address(current_sg_element_p);

        /* determine number of bytes to fill for the current sg element. */
        sg_bytes_to_zero_fill = fbe_sg_element_count(current_sg_element_p);

        /* if host sg offset is not zero then update the sg address and transfer only
         * bytes remained in current sgl.
         */
        if(host_sg_offset != 0)
        {
            sg_bytes_to_zero_fill = sg_bytes_to_zero_fill - host_sg_offset;
            current_sg_addr = current_sg_addr  + host_sg_offset;
        }

        /* sg is longer than this transfer simply transfer the amount specified. */
        if(sg_bytes_to_zero_fill > bytes_left_to_zero_fill)
        {
           if(bytes_left_to_zero_fill >  FBE_U32_MAX)
           {
                /* we do not expect this to cross 32bit limit
                 */
                fbe_provision_drive_utils_trace(provision_drive_p,
                                                FBE_TRACE_LEVEL_ERROR,
                                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                                FBE_PROVISION_DRIVE_DEBUG_FLAG_ZOD_TRACING,
                                                "READ_ZOD:zero fill, bytes_left_to_zero_fill unexpected: 0x%llx.\n",
                                                (unsigned long long)bytes_left_to_zero_fill);
                return FBE_STATUS_GENERIC_FAILURE;
           }
            sg_bytes_to_zero_fill = (fbe_u32_t)(bytes_left_to_zero_fill);
        }

        /* if sg list is longer than the zero bucket size then transfer only required bytes. */
        if(sg_bytes_to_zero_fill > zero_bit_bucket_size_in_bytes)
        {
            /* update the sg offset for the current sg element. */
            host_sg_offset += zero_bit_bucket_size_in_bytes;
            if (host_sg_offset >= fbe_sg_element_count(current_sg_element_p))
            {
                /* Something is wrong if we go beyond the count of this sg element.
                 */
                fbe_provision_drive_utils_trace(provision_drive_p,
                                                FBE_TRACE_LEVEL_ERROR,
                                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                                FBE_PROVISION_DRIVE_DEBUG_FLAG_ZOD_TRACING,
                                                "READ_ZOD:zero fill, hs_sg_offset unexpected: 0x%x/0x%x start_lba:0x%llx, blk:0x%llx.\n",
                                                host_sg_offset, fbe_sg_element_count(current_sg_element_p),
                                                (unsigned long long)next_need_zero_start_lba,
						(unsigned long long)next_need_zero_block_cout);
                return FBE_STATUS_GENERIC_FAILURE;
            }
            if(zero_bit_bucket_size_in_bytes > FBE_U32_MAX)
            {
                 /* we do not expect this to cross 32bit limit
                  */
                 fbe_provision_drive_utils_trace(provision_drive_p,
                                                 FBE_TRACE_LEVEL_ERROR,
                                                 FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                                 FBE_PROVISION_DRIVE_DEBUG_FLAG_ZOD_TRACING,
                                                 "READ_ZOD:zero_bit_bucket_size_in_bytes: 0x%llx crossing 32bit limit.\n",
                                                 (unsigned long long)zero_bit_bucket_size_in_bytes);
                 return FBE_STATUS_GENERIC_FAILURE;
            }
            /* only copy bytes for the zero bit bucket size, copy rest in next cycle. */
            sg_bytes_to_zero_fill = (fbe_u32_t)(zero_bit_bucket_size_in_bytes);
        }
        else
        {
            host_sg_offset = 0;
        }

        fbe_copy_memory(current_sg_addr, zero_bit_bucket_p, sg_bytes_to_zero_fill);

        /* if host sg offset is zero then increment the sg element. */
        if(!host_sg_offset)
        {
            current_sg_element_p++;
        }

        bytes_left_to_zero_fill -= sg_bytes_to_zero_fill;

    } /* end while bytes left and no error */

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_zero_on_demand_read_fill_zero_buffer()
 ******************************************************************************/

/*!****************************************************************************
 *  fbe_provision_drive_zero_on_demand_read_determine_if_media_read_needed()
 ******************************************************************************
 * @brief
 *  This function is used to determine  whether we need media read to satisfy
 *  host read request, it looks at the paged metadata bits and if all the chunk
 *  needs to be zeroed then it send media read needed as false.
 *
 * @return fbe_status_t.        - status of the operation.
 *
 * @author
 *   07/14/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_zero_on_demand_read_determine_if_media_read_needed(fbe_lba_t host_start_lba,
                                                                       fbe_block_count_t host_block_count,
                                                                       fbe_provision_drive_paged_metadata_t * paged_data_bits_p,
                                                                       fbe_bool_t * is_media_read_needed_b_p)
{
    fbe_chunk_index_t   start_chunk_index;
    fbe_chunk_count_t   chunk_count;
    fbe_chunk_index_t   next_zeroed_index_in_paged_bits = FBE_CHUNK_INDEX_INVALID;
    fbe_chunk_size_t    chunk_size = FBE_PROVISION_DRIVE_CHUNK_SIZE;

    /* initialize the media read needed as false. */
    *is_media_read_needed_b_p = FBE_FALSE;

    /* get the start chunk index and chunk count from the start lba and end lba of the range. */
    fbe_provision_drive_utils_calculate_chunk_range(host_start_lba,
                                                    host_block_count,
                                                    chunk_size,
                                                    &start_chunk_index,
                                                    &chunk_count);

    /* get the next zeroed chunk in paged data bits. */
    fbe_provision_drive_bitmap_get_next_zeroed_chunk(paged_data_bits_p,
                                                     0,
                                                     chunk_count,
                                                     &next_zeroed_index_in_paged_bits);

    if(next_zeroed_index_in_paged_bits == FBE_CHUNK_INDEX_INVALID)
    {
        /* we do not have any chunk which is already zeroed and so return meda
         * read needed as false. */
        return FBE_STATUS_OK;
    }

    /* set the media read needed as true. */
    *is_media_read_needed_b_p = FBE_TRUE;
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_zero_on_demand_read_determine_if_media_read_needed()
 ******************************************************************************/

/*!****************************************************************************
 *  fbe_provision_drive_zero_on_demand_read_calc_memory_chunks()
 ******************************************************************************
 * @brief
 *  This function is used to calculate the number of memory chunks required
 *  for allocation of the sg list for the media read subrequests.
 *
 * @param provision_drive_p     - pointer to provision drove object.
 * @param packet_p              - pointer to packet.
 * @param chunk_size            - memory chunk size.
 * @param number_of_chunks_p    - pointer to number of chunks.
 *
 * @return fbe_status_t.        - status of the operation.
 *
 * @author
 *   07/14/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_zero_on_demand_read_calc_memory_chunks(fbe_provision_drive_t * provision_drive_p,
                                                           fbe_packet_t * packet_p,
                                                           fbe_u32_t memory_chunk_size,
                                                           fbe_u32_t * number_of_chunks_p)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_lba_t                               host_start_lba;
    fbe_block_count_t                       host_block_count;
    fbe_sg_element_t *                      host_sg_list;
    fbe_u32_t                               total_sg_list_size;
    fbe_u32_t                               total_sg_element_count;
    fbe_u32_t                               number_of_read_requests;
    fbe_u32_t                               memory_left_in_chunk;
    fbe_payload_ex_t *                     sep_payload_p = NULL;
    fbe_payload_block_operation_t *         block_operation_p = NULL;
    fbe_payload_metadata_operation_t *      metadata_operation_p = NULL;
    fbe_provision_drive_paged_metadata_t *  paged_data_bits_p = NULL;

    /* initialize number of chunks we need to allocate as zero. */
    *number_of_chunks_p = 0;

    /* get the sep payload and host sg list. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_payload_ex_get_sg_list(sep_payload_p, &host_sg_list, NULL);

    /* get the metadata operation previous block operation. */
    metadata_operation_p = fbe_payload_ex_get_metadata_operation(sep_payload_p);
    block_operation_p = fbe_payload_ex_get_present_block_operation(sep_payload_p);

    /* get the host start lba and block count. */
    fbe_payload_block_get_lba(block_operation_p, &host_start_lba);
    fbe_payload_block_get_block_count(block_operation_p, &host_block_count);

    /* get the paged data bits from the metadata operation. */
    fbe_payload_metadata_get_metadata_record_data_ptr(metadata_operation_p, (void **)&paged_data_bits_p);

    /* initialize the parmeters before we calculate the memory requirement for the read operaiton. */
    number_of_read_requests = 0;
    total_sg_element_count = 0;
    total_sg_list_size = 0;

    status = fbe_provision_drive_zero_on_demand_read_calc_sg_and_subpacket_count(host_start_lba,
                                                                                 host_block_count,
                                                                                 host_sg_list,
                                                                                 paged_data_bits_p,
                                                                                 &number_of_read_requests,
                                                                                 &total_sg_element_count);
    if(status != FBE_STATUS_OK)
    {
        /*!@todo add error trace here. */
        return status;
    }

    if(total_sg_element_count != 0)
    {
        /* memory required for destination sg element list. */
        total_sg_list_size = total_sg_element_count * sizeof(fbe_sg_element_t);

        /* round the sg list size to align with 64-bit boundary. */
        total_sg_list_size += (total_sg_list_size % sizeof(fbe_u64_t));

        /* we need a chunk for the subpacket and one chunk for the sg list. */
        if(total_sg_list_size > memory_chunk_size)
        {
            /*!@todo add error trace here, non retryable error. */
            return FBE_STATUS_GENERIC_FAILURE;
        }
        else
        {
            /*!@todo we can improve this logic further to make sure next chunk is enough
             * for the subpacket allocation.
             */
            (*number_of_chunks_p)++;
            memory_left_in_chunk = memory_chunk_size - total_sg_list_size;
            if(memory_left_in_chunk < (FBE_MEMORY_CHUNK_SIZE_FOR_PACKET  * number_of_read_requests))
            {
                (*number_of_chunks_p)++;
            }
        }
    }

    /* get the total number of sg count required for the zero on demand processing. */
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_zero_on_demand_read_calc_memory_chunks()
 ******************************************************************************/

/*!****************************************************************************
 *  fbe_provision_drive_zero_on_demand_read_calc_sg_and_subpacket_count()
 ******************************************************************************
 * @brief
 *
 * @return fbe_status_t.        - status of the operation.
 *
 * @author
 *   07/14/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_zero_on_demand_read_calc_sg_and_subpacket_count(fbe_lba_t host_start_lba,
                                                                    fbe_block_count_t host_block_count,
                                                                    fbe_sg_element_t * host_sg_list,
                                                                    fbe_provision_drive_paged_metadata_t * paged_data_bits_p,
                                                                    fbe_u32_t * number_of_read_request_p,
                                                                    fbe_u32_t * total_sg_element_count_p)
{
    fbe_lba_t               next_read_start_lba;
    fbe_block_count_t       next_read_block_count;
    fbe_u32_t               read_sg_element_count = 0;
    fbe_status_t            status;

    /* initialize the parmeters before we calculate the memory requirement for the read operaiton. */
    next_read_start_lba  = host_start_lba;
    next_read_block_count  = 0;
    *number_of_read_request_p = 0;
    *total_sg_element_count_p = 0;

    /* determine next lba range which needs media read operation until we go through the host range. */
    while(next_read_start_lba != FBE_LBA_INVALID)
    {
        /* get the next lba range which requires to perform the media read operation. */
        fbe_provision_drive_zero_on_demand_read_get_next_zeroed_lba_range(host_start_lba,
                                                                          host_block_count,
                                                                          paged_data_bits_p,
                                                                          &next_read_start_lba,
                                                                          &next_read_block_count);

        if(next_read_start_lba != FBE_LBA_INVALID)
        {
            /* calculate the sg count for the next media read lba range. */
            status = fbe_provision_drive_zero_on_demand_read_calc_sg_count(host_start_lba,
                                                                           host_block_count,
                                                                           host_sg_list,
                                                                           next_read_start_lba,
                                                                           next_read_block_count,
                                                                           &read_sg_element_count);
            if(status != FBE_STATUS_OK)
            {
                /*!@todo add error message here. */
                return status;
            }
            else
            {
                /* increment the total sg count based on requirement. */
                (*total_sg_element_count_p) += read_sg_element_count;
                (*number_of_read_request_p)++;
            }
        }
    }

    /* return good status. */
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_zero_on_demand_read_calc_sg_and_subpacket_count()
 ******************************************************************************/

/*!****************************************************************************
 *  fbe_provision_drive_zero_on_demand_read_calc_sg_count()
 ******************************************************************************
 * @brief
 *
 * @return fbe_status_t.        - status of the operation.
 *
 * @author
 *   07/14/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_zero_on_demand_read_calc_sg_count(fbe_lba_t host_start_lba,
                                                      fbe_block_count_t host_block_count,
                                                      fbe_sg_element_t * host_sg_list,
                                                      fbe_lba_t read_start_lba,
                                                      fbe_block_count_t read_block_count,
                                                      fbe_u32_t * read_sg_element_count_p)
{
    fbe_sg_element_with_offset_t    host_sgd;
    fbe_status_t                    status;
    fbe_block_count_t            blks_remaining;
    fbe_u32_t                       sg_offset;
    fbe_sg_element_t *              src_sg_element_p = NULL;
    

    /* initialize the media read sg element count as zero. */
    *read_sg_element_count_p = 0;

    if((read_start_lba == FBE_LBA_INVALID) ||
       (read_block_count <= 0))
    {
        /* we do not expect the caller to call this with */
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* initialize the offset efore we create sg descriptor with offset. */
    sg_offset = (fbe_u32_t) ((read_start_lba - host_start_lba) * FBE_BE_BYTES_PER_BLOCK);

    /* initialize the host sg list with offset. */
    fbe_sg_element_with_offset_init(&host_sgd, sg_offset, host_sg_list, NULL);

    /* Adjust the sg element based on the current offset. */
    fbe_raid_adjust_sg_desc(&host_sgd);

    /* Determine blocks remaining in current sg before calling to count number
     * sg entries in list.
     */
    src_sg_element_p = host_sgd.sg_element;
    blks_remaining = (src_sg_element_p->count - host_sgd.offset) / FBE_BE_BYTES_PER_BLOCK;

    /* Now invoke method that determines the number of sg elements required
     * including any extra for partial (start and end of list).
     */
    status = fbe_raid_sg_count_nonuniform_blocks(read_block_count,
                                                 &host_sgd,
                                                 FBE_BE_BYTES_PER_BLOCK,
                                                 &blks_remaining,
                                                 read_sg_element_count_p);

    /* add one more sg element entry for the termination. */
    (*read_sg_element_count_p)++;
    return status;
}
/******************************************************************************
 * end fbe_provision_drive_zero_on_demand_read_calc_sg_count()
 ******************************************************************************/

/*!****************************************************************************
 *  fbe_provision_drive_zero_on_demand_read_get_next_zeroed_lba_range()
 ******************************************************************************
 * @brief
 *
 * @return fbe_status_t.        - status of the operation.
 *
 * @author
 *   07/14/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_zero_on_demand_read_get_next_zeroed_lba_range(fbe_lba_t host_start_lba,
                                                                  fbe_block_count_t host_block_count,
                                                                  fbe_provision_drive_paged_metadata_t * paged_data_bits_p,
                                                                  fbe_lba_t * next_read_start_lba_p,
                                                                  fbe_block_count_t * next_read_block_count_p)
{
    fbe_chunk_index_t   start_chunk_index;
    fbe_chunk_count_t   chunk_count;
    fbe_chunk_index_t   next_chunk_index;
    fbe_chunk_index_t   next_zeroed_chunk_index;
    fbe_chunk_index_t   next_need_zero_chunk_index;
    fbe_chunk_index_t   next_index_in_paged_bits;
    fbe_chunk_index_t   next_zeroed_index_in_paged_bits = FBE_CHUNK_INDEX_INVALID;
    fbe_chunk_index_t   next_need_zero_index_in_paged_bits = FBE_CHUNK_INDEX_INVALID;
    fbe_chunk_size_t    chunk_size = FBE_PROVISION_DRIVE_CHUNK_SIZE;

    /* if user passed next read start lba as invalid then it does not have next range
     * to look for and so return next range as invalid.
     */
    if((*next_read_start_lba_p) == FBE_LBA_INVALID)
    {
        *next_read_block_count_p = 0;
        return FBE_STATUS_OK;
    }

    /* if host end lba is greater than current lba plus block count then return. */
    if((host_start_lba + host_block_count) <= 
       ((*next_read_start_lba_p)  + (*next_read_block_count_p)))
    {
        *next_read_start_lba_p = FBE_LBA_INVALID;
        *next_read_block_count_p = 0;
        return FBE_STATUS_OK;
    }

    /* get the start chunk index and chunk count from the start lba and end lba of the range. */
    fbe_provision_drive_utils_calculate_chunk_range(host_start_lba,
                                                    host_block_count,
                                                    chunk_size,
                                                    &start_chunk_index,
                                                    &chunk_count);

    /* get the current chunk index from the current start lba. */
    next_chunk_index = ((*next_read_start_lba_p) + (*next_read_block_count_p)) / chunk_size;

    /* get the current index in paged data bits. */
    next_index_in_paged_bits = next_chunk_index - start_chunk_index;

    /* get the next zeroed chunk in paged data bits. */
    fbe_provision_drive_bitmap_get_next_zeroed_chunk(paged_data_bits_p,
                                                     next_index_in_paged_bits,
                                                     chunk_count,
                                                     &next_zeroed_index_in_paged_bits);
    if(next_zeroed_index_in_paged_bits == FBE_CHUNK_INDEX_INVALID)
    {
        /* we do not have any left chunk in the host range which is already zeroed. */
        *next_read_start_lba_p = FBE_LBA_INVALID;
        *next_read_block_count_p = 0;
        return FBE_STATUS_OK;
    }
    else
    {
        next_zeroed_chunk_index = start_chunk_index + next_zeroed_index_in_paged_bits;
        if(next_zeroed_chunk_index != start_chunk_index)
        {
            *next_read_start_lba_p = next_zeroed_chunk_index * chunk_size;
        }
        else
        {
            *next_read_start_lba_p = host_start_lba;
        }
    }

    /* get the next need zero chunk in paged data bits. */
    fbe_provision_drive_bitmap_get_next_need_zero_chunk(paged_data_bits_p,
                                                        next_zeroed_index_in_paged_bits,
                                                        chunk_count,
                                                        &next_need_zero_index_in_paged_bits);

    if(next_need_zero_index_in_paged_bits == FBE_CHUNK_INDEX_INVALID)
    {
        /* this case covers the range from the current start lba to the end of host range. */
        *next_read_block_count_p = (fbe_block_count_t) (host_start_lba + host_block_count - (*next_read_start_lba_p));
    }
    else
    {
        /* this case covers the partial range until we get next chunk which need zero. */
        next_need_zero_chunk_index = start_chunk_index + next_need_zero_index_in_paged_bits;
        *next_read_block_count_p = (fbe_block_count_t) ((next_need_zero_chunk_index * chunk_size) - (*next_read_start_lba_p));
    }

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_zero_on_demand_read_get_next_zeroed_lba_range()
 ******************************************************************************/

/*!****************************************************************************
 *  fbe_provision_drive_zero_on_demand_read_get_next_need_zero_lba_range()
 ******************************************************************************
 * @brief
 *
 * @return fbe_status_t.        - status of the operation.
 *
 * @author
 *   07/14/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_zero_on_demand_read_get_next_need_zero_lba_range(fbe_lba_t host_start_lba,
                                                                     fbe_block_count_t host_block_count,
                                                                     fbe_provision_drive_paged_metadata_t * paged_data_bits_p,
                                                                     fbe_lba_t * next_need_zero_start_lba_p,
                                                                     fbe_block_count_t * next_need_zero_block_count_p)
{
    fbe_chunk_index_t   start_chunk_index;
    fbe_chunk_count_t   chunk_count;
    fbe_chunk_index_t   next_chunk_index;
    fbe_chunk_index_t   next_zeroed_chunk_index;
    fbe_chunk_index_t   next_need_zero_chunk_index;
    fbe_chunk_index_t   next_index_in_paged_bits;
    fbe_chunk_index_t   next_zeroed_index_in_paged_bits = FBE_CHUNK_INDEX_INVALID;
    fbe_chunk_index_t   next_need_zero_index_in_paged_bits = FBE_CHUNK_INDEX_INVALID;
    fbe_chunk_size_t    chunk_size = FBE_PROVISION_DRIVE_CHUNK_SIZE;

    /* if user passed next read start lba as invalid then it does not have next range
     * to look for and so return next range as invalid.
     */
    if((*next_need_zero_start_lba_p) == FBE_LBA_INVALID)
    {
        *next_need_zero_block_count_p = 0;
        return FBE_STATUS_OK;
    }

    /* if host end lba is greater than current lba */
    if((host_start_lba + host_block_count) <= 
       ((*next_need_zero_start_lba_p)  + (*next_need_zero_block_count_p)))
    {
        /* we have already covered the host lba range. */
        *next_need_zero_start_lba_p = FBE_LBA_INVALID;
        *next_need_zero_block_count_p = 0;
        return FBE_STATUS_OK;
    }

    /* get the start chunk index and chunk count from the start lba and end lba of the range. */
    fbe_provision_drive_utils_calculate_chunk_range(host_start_lba,
                                                    host_block_count,
                                                    chunk_size,
                                                    &start_chunk_index,
                                                    &chunk_count);

    /* get the current chunk index from the current start lba. */
    next_chunk_index = ((*next_need_zero_start_lba_p) + (*next_need_zero_block_count_p)) / chunk_size;

    /* get the current index in paged data bits. */
    next_index_in_paged_bits = next_chunk_index - start_chunk_index;

    /* get the next zeroed chunk in paged data bits. */
    fbe_provision_drive_bitmap_get_next_need_zero_chunk(paged_data_bits_p,
                                                        next_index_in_paged_bits,
                                                        chunk_count,
                                                        &next_need_zero_index_in_paged_bits);
    if(next_need_zero_index_in_paged_bits == FBE_CHUNK_INDEX_INVALID)
    {
        /* we do not have any left chunk in the host range which is already zeroed. */
        *next_need_zero_start_lba_p = FBE_LBA_INVALID;
        *next_need_zero_block_count_p = 0;
        return FBE_STATUS_OK;
    }
    else
    {
        next_need_zero_chunk_index = start_chunk_index + next_need_zero_index_in_paged_bits;
        if(next_need_zero_chunk_index != start_chunk_index)
        {
            *next_need_zero_start_lba_p = next_need_zero_chunk_index * chunk_size;
        }
        else
        {
            *next_need_zero_start_lba_p = host_start_lba;
        }
    }

    /* get the next need zero chunk in paged data bits. */
    fbe_provision_drive_bitmap_get_next_zeroed_chunk(paged_data_bits_p,
                                                     next_need_zero_index_in_paged_bits,
                                                     chunk_count,
                                                     &next_zeroed_index_in_paged_bits);

    if(next_zeroed_index_in_paged_bits == FBE_CHUNK_INDEX_INVALID)
    {
        /* this case covers the range from the current start lba to the end of host range. */
        *next_need_zero_block_count_p = (fbe_block_count_t) (host_start_lba + host_block_count - (*next_need_zero_start_lba_p));
    }
    else
    {
        /* this case covers the partial range until we get next chunk which need zero. */
        next_zeroed_chunk_index = start_chunk_index + next_zeroed_index_in_paged_bits;
        *next_need_zero_block_count_p = (fbe_block_count_t) ((next_zeroed_chunk_index * chunk_size) - (*next_need_zero_start_lba_p));
    }

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_zero_on_demand_read_get_next_need_zero_lba_range()
 ******************************************************************************/

/*!****************************************************************************
 *  fbe_provision_drive_zero_on_demand_read_get_next_zeroed_lba_range_with_sg_list()
 ******************************************************************************
 * @brief
 *
 * @return fbe_status_t.        - status of the operation.
 *
 * @author
 *   07/14/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_zero_on_demand_read_get_next_zeroed_lba_range_with_sg_list(fbe_lba_t host_start_lba,
                                                                               fbe_block_count_t host_block_count,
                                                                               fbe_sg_element_t *  host_sg_list,
                                                                               fbe_provision_drive_paged_metadata_t * paged_data_bits_p,
                                                                               fbe_u32_t read_sg_element_count,
                                                                               fbe_lba_t * next_read_start_lba_p,
                                                                               fbe_block_count_t * next_read_block_count_p,
                                                                               fbe_sg_element_t * next_read_sg_list_p,
                                                                               fbe_u16_t * next_read_sg_element_count_p,
                                                                               fbe_u32_t * total_used_sg_element_count_p)
{
    fbe_sg_element_with_offset_t    host_sgd;
    fbe_status_t                    status;
    fbe_u32_t                       next_read_sg_element_count = 0;
    fbe_u32_t                       next_read_byte_count;
    fbe_u32_t                       sg_offset;

    /* get the next lba range which requires to perform the media read operation. */
    status = fbe_provision_drive_zero_on_demand_read_get_next_zeroed_lba_range(host_start_lba,
                                                                               host_block_count,
                                                                               paged_data_bits_p,
                                                                               next_read_start_lba_p,
                                                                               next_read_block_count_p);
    if((status != FBE_STATUS_OK) ||
       (*next_read_start_lba_p == FBE_LBA_INVALID))
    {
        /* on error return status without creating sg list for the next read range. */
        return status;
    }

    /* calculate the sg count for the next media read lba range. */
    status = fbe_provision_drive_zero_on_demand_read_calc_sg_count(host_start_lba,
                                                                   host_block_count,
                                                                   host_sg_list,
                                                                   *next_read_start_lba_p,
                                                                   *next_read_block_count_p,
                                                                   &next_read_sg_element_count);

    if(((*total_used_sg_element_count_p) + next_read_sg_element_count) > read_sg_element_count)
    {
        /* if preallocated sg elements are not sufficient then return error. */
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    /* calculate the sg offset in host sg list. */
    sg_offset = (fbe_u32_t) ((*next_read_start_lba_p) - host_start_lba) * FBE_BE_BYTES_PER_BLOCK;

    /* initialize the host sg list with offset. */
    fbe_sg_element_with_offset_init(&host_sgd, sg_offset, host_sg_list, NULL);

    /* Adjust the sg element based on the current offset. */
    fbe_raid_adjust_sg_desc(&host_sgd);

    if(((*next_read_block_count_p) * FBE_BE_BYTES_PER_BLOCK) > FBE_U32_MAX)
    {
        /* byte count is exceeding 32bit limit. */
        return FBE_STATUS_GENERIC_FAILURE;
    }
    next_read_byte_count = (fbe_u32_t)((*next_read_block_count_p) * FBE_BE_BYTES_PER_BLOCK);

    /* copy existing host sg list to media read sg list. */
    status = fbe_raid_sg_clip_sg_list(&host_sgd,
                                      next_read_sg_list_p,
                                      next_read_byte_count,
                                      next_read_sg_element_count_p);

    (*total_used_sg_element_count_p) += (*next_read_sg_element_count_p) + 1;
    return status;
}
/******************************************************************************
 * end fbe_provision_drive_zero_on_demand_read_get_next_zeroed_lba_range_with_sg_list()
 ******************************************************************************/

/*!****************************************************************************
 *  fbe_provision_drive_zero_on_demand_write_start_zero_on_demand()
 ******************************************************************************
 * @brief
 *  This function is used to kick start the zero on demand processing, it takes
 *  different code path based on the actual i/o request alignment and resource
 *  requirement for the zero on demand operation.
 * 
 *  If resource requirement exceeds the limitation then it takes slow path where
 *  it makes sure that max trasfer size or max sgl entries does not exceed the
 *  backend limitation, if it finds it exceeds the limitation while taking fast
 *  path then it takes slow path to perform zero on demand operation.
 * 
 *
 * @param provision_drive_p     - pointer to provision drove object.
 * @param packet_p              - pointer to packet.
 *
 * @return fbe_status_t.        - status of the operation.
 *
 * @author
 *   07/14/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_zero_on_demand_write_start_zero_on_demand(fbe_provision_drive_t * provision_drive_p,
                                                              fbe_packet_t * packet_p)
{
    fbe_status_t    status;
    fbe_bool_t      can_fast_path_b = FBE_FALSE;

    /* determine if we can use fast path for the zero on demand operation. */
    fbe_provision_drive_zero_on_demand_write_determine_if_fast_path_can_use(provision_drive_p,
                                                                            packet_p,
                                                                            &can_fast_path_b);
    if(can_fast_path_b)
    {
        /* take the fast path for zero on demand operation. */
        status = fbe_provision_drive_zero_on_demand_write_fast_path_start(provision_drive_p, packet_p);
    }
    else
    {    
        /* take the slow path for zero on demand operation. */
        //status = fbe_provision_drive_zero_on_demand_write_slow_path_start(provision_drive_p, packet_p);
		fbe_transport_set_completion_function(packet_p,
											  fbe_provision_drive_zero_on_demand_write_slow_path_write_same_completion,
											  provision_drive_p);

		fbe_provision_drive_zero_on_demand_write_slow_path_write_same_start(provision_drive_p, packet_p);
    }
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * end fbe_provision_drive_zero_on_demand_write_start_zero_on_demand()
 ******************************************************************************/

/*!****************************************************************************
 *  fbe_provision_drive_zero_on_demand_write_determine_if_fast_path_can_use()
 ******************************************************************************
 * @brief
 *  This function is used determine whether we can use the fast path for the 
 *  zero on demand operation, it verifies the transfer lenght and block 
 *  operation to decide whether it can use the fast path or not.
 *
 * @param provision_drive_p     - pointer to provision drove object.
 * @param packet_p              - pointer to packet.
 * @param can_fast_path_b       - pointer to bool which return TRUE/FALSE.
 *
 * @return fbe_status_t.        - status of the operation.
 *
 * @author
 *   07/14/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_zero_on_demand_write_determine_if_fast_path_can_use(fbe_provision_drive_t * provision_drive_p,
                                                                        fbe_packet_t * packet_p,
                                                                        fbe_bool_t * can_fast_path_b_p)
{
    fbe_payload_ex_t *                     sep_payload_p = NULL;
    fbe_payload_block_operation_t *         block_operation_p = NULL;
    fbe_payload_block_operation_opcode_t    block_opcode;
	fbe_lba_t								lba;
	fbe_block_count_t						block_count;

    /* get the payload and previous block operation. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p =  fbe_payload_ex_get_present_block_operation(sep_payload_p);
    fbe_payload_block_get_opcode(block_operation_p, &block_opcode);

    /* initialize fast path can use as true. */
    *can_fast_path_b_p = FBE_TRUE;

    switch(block_opcode) 
    {
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE:
		case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_VERIFY:
            /* fast path is supported only for the write operation. */
            break;
        default:
            /* fast path is not supported for any other operation other than write. */
            *can_fast_path_b_p = FBE_FALSE;
            return FBE_STATUS_OK;
    }


    /* get the lba and block count from the previous block operation. */
    fbe_payload_block_get_lba(block_operation_p, &lba);
    fbe_payload_block_get_block_count(block_operation_p, &block_count);

    /*! @note Enable the below line to allow for more general testing of the fast path. 
     * Today the policy is to only do fast path for chunk aligned.  To test fast path for 
     * all possible cases enable the below line. 
     */
    //if ((lba / FBE_PROVISION_DRIVE_CHUNK_SIZE) != ((lba + block_count - 1) / FBE_PROVISION_DRIVE_CHUNK_SIZE)){

	/* Check if this I/O aligned to chunk */
	if(((lba % FBE_PROVISION_DRIVE_CHUNK_SIZE) != 0) || ((block_count % FBE_PROVISION_DRIVE_CHUNK_SIZE) != 0)){
        *can_fast_path_b_p = FBE_FALSE;
	}
    else {
        *can_fast_path_b_p = FBE_TRUE;
    }

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_zero_on_demand_write_determine_if_fast_path_can_use()
 ******************************************************************************/

/*!****************************************************************************
 *  fbe_provision_drive_zero_on_demand_write_fast_path_calculate_xfer_size()
 ******************************************************************************
 * @brief
 *  This function is used to determine the xfer size for the fast path, it
 *  first figures out the number of chunk which needs to zero on demand for the
 *  original request 
 *
 * @param provision_drive_p     - pointer to provision drove object.
 * @param packet_p              - pointer to packet.
 * @param transfer_size_p       - pointer to transfer size for the fast path.
 *
 * @return fbe_status_t.        - status of the operation.
 *
 * @author
 *   07/14/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_zero_on_demand_write_fast_path_calculate_xfer_size(fbe_provision_drive_t *  provision_drive_p,
                                                                       fbe_packet_t * packet_p,
                                                                       fbe_block_count_t * transfer_size_p)

{
    fbe_payload_metadata_operation_t *      metadata_operation_p = NULL;
    fbe_payload_block_operation_t *         block_operation_p = NULL;
    fbe_payload_ex_t *                     sep_payload_p = NULL;
    fbe_lba_t                               start_lba;
    fbe_block_count_t                       block_count;
    fbe_lba_t                               zod_start_lba = FBE_LBA_INVALID;
    fbe_block_count_t                       zod_block_count = 0;
    fbe_provision_drive_paged_metadata_t *  paged_data_bits_p = NULL;

    /* Get the payload and metadata operation. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    metadata_operation_p =  fbe_payload_ex_get_metadata_operation(sep_payload_p);

    /* get the previous block operation from packet. */
    block_operation_p =  fbe_payload_ex_get_present_block_operation(sep_payload_p);

    /* get the lba and block count from the previous block operation. */
    fbe_payload_block_get_lba(block_operation_p, &start_lba);
    fbe_payload_block_get_block_count(block_operation_p, &block_count);

    /* type cast the metadata to provison drive paged metadata. */
    fbe_payload_metadata_get_metadata_record_data_ptr(metadata_operation_p, (void **)&paged_data_bits_p);

    /* Get the range for which we need to do the zero on demand operation. */
    fbe_provision_drive_zero_on_demand_write_fast_path_calculate_zod_lba_range(start_lba,
                                                                               block_count,
                                                                               paged_data_bits_p,
                                                                               &zod_start_lba,
                                                                               &zod_block_count);

    /* transfer size for this i/o operation will be number of chunk counts in to chunk size. */
    *transfer_size_p = zod_block_count;
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_zero_on_demand_write_fast_path_calculate_xfer_size()
 ******************************************************************************/

/*!****************************************************************************
 *  fbe_provision_drive_zero_on_demand_write_slow_path_start()
 ******************************************************************************
 * @brief
 *  This function is used to kick start the zero on demand operation with slow
 *  path where it first issues write same request to zero out the required
 *  chunks before it issues original write request to perform the i/o
 *  operation.
 *
 * @param provision_drive_p     - pointer to provision drove object.
 * @param packet_p              - pointer to packet.
 * @param paged_data_bits_p     - pointer to paged data bits.
 *
 * @return fbe_status_t.        - status of the operation.
 *
 * @author
 *   07/14/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_zero_on_demand_write_slow_path_start(fbe_provision_drive_t * provision_drive_p,
                                                         fbe_packet_t * packet_p)
{
#if 0
    fbe_payload_block_operation_t *         block_operation_p = NULL;
    fbe_payload_metadata_operation_t *      metadata_operation_p = NULL;
    fbe_payload_ex_t *                     sep_payload_p = NULL;
    fbe_lba_t                               start_lba;
    fbe_block_count_t                       block_count;
    fbe_provision_drive_paged_metadata_t *  paged_data_bits_p = NULL;
    fbe_chunk_count_t                       number_of_edge_chunks_need_process = 0;
    fbe_bool_t                              is_paged_data_valid;
 
    /* Get the payload and block operation. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    metadata_operation_p = fbe_payload_ex_get_metadata_operation(sep_payload_p);
    block_operation_p =  fbe_payload_ex_get_present_block_operation(sep_payload_p);

    /* get the lba and block count from the previous block operation. */
    fbe_payload_block_get_lba(block_operation_p, &start_lba);
    fbe_payload_block_get_block_count(block_operation_p, &block_count);

    /* get the paged metadata. */
    fbe_payload_metadata_get_metadata_record_data_ptr(metadata_operation_p, (void **)&paged_data_bits_p);

#endif

    /* set the completion function for the original packet before we create a subpacket to perform
     * the zero operation, on completion of zero operation it will resume the flow where it performs
     * host write before updating metadata.
     */
    fbe_transport_set_completion_function(packet_p,
                                          fbe_provision_drive_zero_on_demand_write_slow_path_write_same_completion,
                                          provision_drive_p);
#if 0
    fbe_provision_drive_metadata_is_paged_metadata_valid(start_lba,
                                                         block_count, paged_data_bits_p, &is_paged_data_valid);

    if(!is_paged_data_valid)
    {
        /* get the number of boundary edge range which is not aligned to chunk and need invalidate. */
        fbe_provision_drive_utils_calc_number_of_edge_chunks_needs_process(start_lba,
                                                                           block_count,
                                                                           FBE_PROVISION_DRIVE_CHUNK_SIZE,
                                                                           paged_data_bits_p,                                              
                                                                           fbe_provision_drive_bitmap_is_paged_data_invalid,
                                                                           &number_of_edge_chunks_need_process);
    }
    else
    {
        /* get the number of boundary edge range which is not aligned to chunk and need zero. */
        fbe_provision_drive_utils_calc_number_of_edge_chunks_needs_process(start_lba,
                                                                           block_count,
                                                                           FBE_PROVISION_DRIVE_CHUNK_SIZE,
                                                                           paged_data_bits_p,
                                                                           fbe_provision_drive_bitmap_is_chunk_marked_for_zero,
                                                                           &number_of_edge_chunks_need_process);

    }
#endif

    fbe_provision_drive_zero_on_demand_write_slow_path_write_same_start(provision_drive_p, packet_p);
  
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_zero_on_demand_write_slow_path_start()
 ******************************************************************************/

/*!****************************************************************************
 *  fbe_provision_drive_zero_on_demand_write_slow_path_write_same_start()
 ******************************************************************************
 * @brief
 *
 *  This function is used to handle the completion of the allocation of the 
 *  subpacket created for the zero on demand request, it sends the write same
 *  request to process the zero on demand operation.
 *
 * @param memory_request    - Pointer to memory request.
 * @param context           - Pointer to the provision drive object.
 *
 * @return fbe_status_t.    - status of the operation.
 *
 * @author
 *   07/14/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_zero_on_demand_write_slow_path_write_same_start(fbe_provision_drive_t *provision_drive_p,
                                                                    fbe_packet_t *packet_p)
{
    fbe_payload_ex_t *                     sep_payload_p = NULL;
    fbe_payload_block_operation_t *         block_operation_p = NULL;
    fbe_payload_metadata_operation_t *      metadata_operation_p = NULL;
    fbe_payload_block_operation_t *         new_block_operation_p = NULL;
    fbe_status_t                            status;
    fbe_provision_drive_paged_metadata_t *  paged_data_bits_p = NULL;
    fbe_optimum_block_size_t                optimum_block_size;
    fbe_lba_t                               start_lba;
    fbe_block_count_t                       block_count;
    fbe_lba_t                               ws_lba;
    fbe_block_count_t                       ws_blocks;
    fbe_lba_t                               pre_edge_chunk_start_lba;
    fbe_lba_t                               post_edge_chunk_start_lba;
    fbe_block_count_t                       pre_edge_chunk_block_count;
    fbe_block_count_t                       post_edge_chunk_block_count;
    fbe_chunk_count_t                       number_of_edge_chunks_need_process = 0;
    fbe_sg_element_t *                      sg_list_p = NULL;
    fbe_bool_t                              page_valid;

    /* Get the payload and metadata operation. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    metadata_operation_p =  fbe_payload_ex_get_metadata_operation(sep_payload_p);

    /* get the previous block operation. */
    block_operation_p =  fbe_payload_ex_get_present_block_operation(sep_payload_p);

    /* get the optimum block size, start lba and block count. */
    fbe_block_transport_edge_get_optimum_block_size(&provision_drive_p->block_edge, &optimum_block_size);
    fbe_payload_block_get_lba(block_operation_p, &start_lba);
    fbe_payload_block_get_block_count(block_operation_p, &block_count);

    /* get the paged metadata. */
    fbe_payload_metadata_get_metadata_record_data_ptr(metadata_operation_p, (void **)&paged_data_bits_p);

    /* Get the start chunk index and chunk count from the start lba and end lba of the range. */
    fbe_provision_drive_utils_calculate_edge_chunk_lba_range_which_need_process(start_lba,
                                                                                block_count,
                                                                                FBE_PROVISION_DRIVE_CHUNK_SIZE,
                                                                                paged_data_bits_p,
                                                                                &pre_edge_chunk_start_lba,
                                                                                &pre_edge_chunk_block_count,
                                                                                &post_edge_chunk_start_lba,
                                                                                &post_edge_chunk_block_count,
                                                                                &number_of_edge_chunks_need_process);

    fbe_provision_drive_metadata_is_paged_metadata_valid(start_lba, block_count, paged_data_bits_p, &page_valid);


	/* Code below assumed there are only 2 entries for new_packet_p so let's verify it's still the case*/
	if (number_of_edge_chunks_need_process > 2) {
		fbe_provision_drive_utils_trace(provision_drive_p,
                                        FBE_TRACE_LEVEL_ERROR,
                                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                        FBE_PROVISION_DRIVE_DEBUG_FLAG_ZOD_TRACING,
                                        "WRITE_SLOW_ZOD:more packets that array size:%d\n", number_of_edge_chunks_need_process);

        fbe_payload_block_set_status(block_operation_p,
                                     FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                                     FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNEXPECTED_ERROR);
        fbe_provision_drive_utils_complete_packet(packet_p, FBE_STATUS_OK, 0);
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
	}

    if ((pre_edge_chunk_start_lba != FBE_LBA_INVALID) && (post_edge_chunk_start_lba != FBE_LBA_INVALID)) {
        ws_lba = pre_edge_chunk_start_lba;
        ws_blocks = (post_edge_chunk_start_lba + post_edge_chunk_block_count) - pre_edge_chunk_start_lba;
    } else if (pre_edge_chunk_start_lba != FBE_LBA_INVALID) {
        ws_lba = pre_edge_chunk_start_lba;
        ws_blocks = pre_edge_chunk_block_count;
    } else  {
        if (post_edge_chunk_start_lba == FBE_LBA_INVALID) {
            fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s WRITE_SLOW_ZOD pvd_zod post edge lba is invalid lba: 0x%llx bl: 0x%llx pre: 0x%llx \n", 
                                  __FUNCTION__, start_lba, block_count, pre_edge_chunk_start_lba);

        }
        ws_lba = post_edge_chunk_start_lba;
        ws_blocks = post_edge_chunk_block_count;
    }


    fbe_provision_drive_utils_trace(provision_drive_p, FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO,
                                    FBE_PROVISION_DRIVE_DEBUG_FLAG_ZOD_TRACING,
                                    "WRITE_SLOW_ZOD:send write same lba/bl:0x%llx/0x%llx wslba/bl:0x%llx/0x%llx kh:%llx\n",
                                    start_lba, block_count, ws_lba, ws_blocks, sep_payload_p->key_handle);

    /* allocate and initialize the block operation. */
    new_block_operation_p = fbe_payload_ex_allocate_block_operation(sep_payload_p);

    if (page_valid){
        /* get the zeroed initialized sg list for the write same operation. */
        sg_list_p = fbe_zero_get_bit_bucket_sgl_1MB();
    } else {
        /* get the invalid_pattern sg list for the write same operation. */
        sg_list_p = fbe_invalid_pattern_get_bit_bucket_sgl_1MB();
    }

	/* CBE WRITE SAME */
	if(fbe_provision_drive_is_write_same_enabled(provision_drive_p, ws_lba)){ /* Write Same IS supported */
		
		/* We set the sg list for the write same here.  We will restore this before the write. */
		fbe_payload_ex_set_sg_list(sep_payload_p, sg_list_p, 0);

		fbe_payload_block_build_operation(new_block_operation_p,
										  FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_SAME,
										  ws_lba,
										  ws_blocks,
										  block_operation_p->block_size,
										  optimum_block_size,
										  NULL);
	} else { /* Write Same NOT enabled */
		fbe_sg_element_t * pre_sg_list;

		fbe_payload_ex_get_pre_sg_list(sep_payload_p, &pre_sg_list);
		pre_sg_list[0].address = sg_list_p[0].address;
		pre_sg_list[0].count = (fbe_u32_t)(block_operation_p->block_size * ws_blocks);
		pre_sg_list[1].address = NULL;
		pre_sg_list[1].count = 0;

		/* We set the sg list to NULL */
		fbe_payload_ex_set_sg_list(sep_payload_p, NULL, 0);

		fbe_payload_block_build_operation(new_block_operation_p,
										  FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,
										  ws_lba,
										  ws_blocks,
										  block_operation_p->block_size,
										  optimum_block_size,
										  NULL);
	}

    /* set completion functon to handle the write same request completion. */
    fbe_transport_set_completion_function(packet_p,
                                          fbe_provision_drive_release_block_op_completion,
                                          provision_drive_p);

    /* initialize the block operation with default status. */
    fbe_payload_block_set_status(block_operation_p,
                                 FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID,
                                 FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID);

    status = fbe_base_config_send_functional_packet((fbe_base_config_t *)provision_drive_p, packet_p, 0);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * end fbe_provision_drive_zero_on_demand_write_slow_path_write_same_start()
 ******************************************************************************/

/*!****************************************************************************
 * @fn fbe_provision_drive_zero_on_demand_write_slow_path_write_same_completion()
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
 * @return fbe_status_t.    - status of the operation.
 *
 * @author
 *   07/14/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_zero_on_demand_write_slow_path_write_same_completion(fbe_packet_t * packet_p,
                                                                         fbe_packet_completion_context_t packet_completion_context)
{
    fbe_provision_drive_t *                             provision_drive_p = NULL;
    fbe_payload_block_operation_t *                     block_operation_p = NULL;
    fbe_payload_block_operation_status_t                block_status;
    fbe_payload_ex_t *                                  sep_payload_p = NULL;
    fbe_payload_ex_t *                                  master_payload_p = NULL;
    fbe_status_t                                        status = FBE_STATUS_OK;
    fbe_packet_t *master_packet_p = NULL;
    fbe_sg_element_t *master_sg_p = NULL;
	fbe_sg_element_t * pre_sg_list;

    master_packet_p = fbe_transport_get_master_packet(packet_p);
    master_payload_p = fbe_transport_get_payload_ex(master_packet_p);
    fbe_payload_ex_get_sg_list(master_payload_p, &master_sg_p, NULL);
    provision_drive_p = (fbe_provision_drive_t *) packet_completion_context;

    /* Get the payload metadata and prev block operation. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p =  fbe_payload_ex_get_present_block_operation(sep_payload_p);

    /* Get the status of the actual zeroing write same request. */
    status = fbe_transport_get_status_code(packet_p);
    fbe_payload_block_get_status(block_operation_p, &block_status);

	/* CBE WRITE SAME */
	/* We need to reinit pre_sgl as well */	
	fbe_payload_ex_get_pre_sg_list(sep_payload_p, &pre_sg_list);
	pre_sg_list[0].address = NULL;
	pre_sg_list[0].count = 0;
	
    /* We need to restore the payload's sg list ptr from the master packet. */
    fbe_payload_ex_set_sg_list(sep_payload_p, master_sg_p, 0);

    if((status != FBE_STATUS_OK) ||
       (block_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS))
    {
        /* metadata operation release will take place when packet will be processed at next level 
         */
        fbe_transport_set_status(packet_p, status, 0);
        return status;
    }

    /* send the actual i/o operation before we update the paged metadata. */
    status = fbe_provision_drive_zero_on_demand_write_slow_path_write_start(provision_drive_p, packet_p);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * end fbe_provision_drive_zero_on_demand_write_slow_path_write_same_completion()
 ******************************************************************************/

/*!****************************************************************************
 * @fn fbe_provision_drive_zero_on_demand_write_slow_path_write_start()
 ******************************************************************************
 * @brief
 *  This function is used to allocate the packet before we send a I/O request
 *  to physical package.
 *
 * @note We have created new packet to send the i/o operation, it is required
 *  since curent operation in master packet is metadata operation.  
 *
 * @param provision_drive_p - Pointer to the provision drive object.
 * @param packet_p          - Pointer the the I/O packet.
 *
 * @return fbe_status_t.    - status of the operation.
 *
 * @author
 *   07/14/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_zero_on_demand_write_slow_path_write_start(fbe_provision_drive_t *provision_drive_p,
                                                               fbe_packet_t *packet_p)
{
    fbe_payload_ex_t *                 sep_payload_p = NULL;
    fbe_payload_block_operation_t *     block_operation_p = NULL;
    fbe_payload_block_operation_t *     new_block_operation_p = NULL;
    fbe_status_t                        status;
    fbe_optimum_block_size_t            optimum_block_size;
    
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_get_present_block_operation(sep_payload_p);

    new_block_operation_p = fbe_payload_ex_allocate_block_operation(sep_payload_p);

    fbe_block_transport_edge_get_optimum_block_size(&provision_drive_p->block_edge, &optimum_block_size);

    fbe_payload_block_build_operation(new_block_operation_p,
                                      block_operation_p->block_opcode,
                                      block_operation_p->lba,
                                      block_operation_p->block_count,
                                      block_operation_p->block_size,
                                      optimum_block_size,
                                      block_operation_p->pre_read_descriptor);

    new_block_operation_p->block_flags = block_operation_p->block_flags;
    new_block_operation_p->payload_sg_descriptor = block_operation_p->payload_sg_descriptor;
    new_block_operation_p->block_edge_p = block_operation_p->block_edge_p;

    /* initialize the block operation with default status. */
    fbe_payload_block_set_status(block_operation_p,
                                 FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID,
                                 FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID);

    /* Set completion function */
    fbe_transport_set_completion_function(packet_p,
                                          fbe_provision_drive_release_block_op_completion,
                                          provision_drive_p);

    /* Send packet to the block edge */
    status = fbe_base_config_send_functional_packet((fbe_base_config_t *)provision_drive_p, packet_p, 0);
    return status;
}
/******************************************************************************
 * end fbe_provision_drive_zero_on_demand_write_slow_path_write_start()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_zod_fast_path_completion()
 ******************************************************************************
 * @brief
 *  This is a completion function for where we have a block operation that
 *  was allocated on a packet where there is also another master block
 *  operation above this one.
 * 
 * @param   packet_p        - pointer to disk zeroing IO packet
 * @param   context         - context, which is a pointer to the pvd object.
 *
 * @return fbe_status_t.    - status of the operation.
 *
 * @author
 *  9/14/2012 - Created. Rob Foley
 *
 ******************************************************************************/
fbe_status_t 
fbe_provision_drive_zod_fast_path_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
	fbe_provision_drive_t *provision_drive_p = (fbe_provision_drive_t *)context;
    fbe_payload_ex_t *payload = NULL;
    fbe_payload_block_operation_t * master_block_operation_p = NULL;
    fbe_payload_block_operation_t * block_operation_p = NULL;
    fbe_queue_head_t tmp_queue;
    fbe_sg_element_t *pre_sg_p = NULL;
    fbe_sg_element_t *post_sg_p = NULL;

	payload = fbe_transport_get_payload_ex(packet);
    block_operation_p =  fbe_payload_ex_get_block_operation(payload);

    fbe_payload_ex_release_block_operation(payload, block_operation_p);

    /* Clean out the SGs that we put here previously to leave the payload the way we found it.
     */
    fbe_payload_ex_get_pre_sg_list(payload, &pre_sg_p);
    fbe_payload_ex_get_post_sg_list(payload, &post_sg_p);

    fbe_sg_element_terminate(pre_sg_p);
    fbe_sg_element_terminate(post_sg_p);

    master_block_operation_p = fbe_payload_ex_get_present_block_operation(payload);

    fbe_payload_block_operation_copy_status(block_operation_p, master_block_operation_p);

    fbe_queue_init(&tmp_queue);

    if (fbe_queue_is_element_on_queue(&packet->queue_element)){
        fbe_provision_drive_utils_trace(provision_drive_p,
                                        FBE_TRACE_LEVEL_CRITICAL_ERROR,
                                        FBE_TRACE_MESSAGE_ID_INFO,
                                        FBE_PROVISION_DRIVE_DEBUG_FLAG_ZOD_TRACING,
                                        "%s packet %p already on the queue.\n", __FUNCTION__, packet);
    }
    /* enqueue this packet temporary queue before we enqueue it to run queue. */
    fbe_queue_push(&tmp_queue, &packet->queue_element);

    /*!@note Queue this request to run queue to break the i/o context to avoid the stack
    * overflow, later this needs to be queued with the thread running in same core.
    */
    fbe_transport_run_queue_push(&tmp_queue,  NULL,  NULL);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/**************************************
 * end fbe_provision_drive_zod_fast_path_completion()
 **************************************/
/*!****************************************************************************
 *  fbe_provision_drive_zero_on_demand_write_fast_path_start()
 ******************************************************************************
 * @brief
 *  This function is used to kick start the zero on demand operation with fast
 *  path where it extends the client sg list and combine zero request with 
 *  write request before sending i/o request to downstream object.
 *
 * @param provision_drive_p     - pointer to provision drove object.
 * @param packet_p              - pointer to packet.
 * @param paged_data_bits_p     - pointer to paged data bits.
 *
 * @return fbe_status_t.        - status of the operation.
 *
 * @author
 *   07/14/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_zero_on_demand_write_fast_path_start(fbe_provision_drive_t * provision_drive_p,
                                                         fbe_packet_t * packet_p)
{
    fbe_payload_ex_t *                     sep_payload_p = NULL;
    fbe_payload_block_operation_t *         block_operation_p = NULL;
    fbe_payload_metadata_operation_t *      metadata_operation_p = NULL;
    fbe_payload_block_operation_t *         new_block_operation_p = NULL;
    fbe_status_t                            status;
    fbe_provision_drive_paged_metadata_t *  paged_data_bits_p = NULL;
    fbe_optimum_block_size_t                optimum_block_size;
    fbe_lba_t                               host_start_lba;
    fbe_block_count_t                       host_block_count;
    fbe_sg_element_t *                      host_sg_list;
    fbe_lba_t                               zod_start_lba;
    fbe_block_count_t                       zod_block_count;
    fbe_sg_element_t *                      zod_sg_list  = NULL;
    fbe_bool_t                              page_valid = FBE_TRUE;
    fbe_bool_t                              zero_required = FBE_TRUE;

    /* Get the payload and metadata operation. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    metadata_operation_p =  fbe_payload_ex_get_metadata_operation(sep_payload_p);

    /* get the previous block operation. */
    block_operation_p =  fbe_payload_ex_get_present_block_operation(sep_payload_p);

    /* get the optimum block size, start lba and block count. */
    fbe_block_transport_edge_get_optimum_block_size(&provision_drive_p->block_edge, &optimum_block_size);
    fbe_payload_block_get_lba(block_operation_p, &host_start_lba);
    fbe_payload_block_get_block_count(block_operation_p, &host_block_count);

    /* get the host sg list. */
    fbe_payload_ex_get_sg_list(sep_payload_p, &host_sg_list, NULL);

    /* get the paged metadata. */
    fbe_payload_metadata_get_metadata_record_data_ptr(metadata_operation_p, (void **)&paged_data_bits_p);
    
    fbe_provision_drive_metadata_is_paged_metadata_valid(host_start_lba,
                                                         host_block_count, paged_data_bits_p, &page_valid);

    /* if the metadata page is not valid, then we will have to do an invalidate instead of Zeroing */
    if(!page_valid)
    {
        zero_required = FBE_FALSE;
    }

    /* determine the range for which we need to perform the zero on demand operation. */
    fbe_provision_drive_zero_on_demand_write_fast_path_calculate_zod_lba_range(host_start_lba,
                                                                               host_block_count,
                                                                               paged_data_bits_p,
                                                                               &zod_start_lba,
                                                                               &zod_block_count);

    fbe_provision_drive_utils_trace(provision_drive_p,
                                    FBE_TRACE_LEVEL_DEBUG_HIGH,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    FBE_PROVISION_DRIVE_DEBUG_FLAG_ZOD_TRACING,
                                    "%s:WRITE_FAST_ZOD:send write, h_lba:0x%llx, h_blk:0x%llx, zod_lba:0x%llx, zod_blk: 0x%llx\n",
                                    __FUNCTION__, host_start_lba, host_block_count,
                                    zod_start_lba, zod_block_count);

    /* Setup the pre and post sgs in the sep payload.
     */
    status = fbe_provision_drive_zod_fast_plant_sgs(provision_drive_p,
                                                    sep_payload_p,
                                                    host_start_lba,
                                                    host_block_count,
                                                    host_sg_list,
                                                    zod_start_lba,
                                                    zod_block_count,
                                                    zod_sg_list,
                                                    zero_required);
    if(status != FBE_STATUS_OK)
    {
        fbe_provision_drive_utils_trace(provision_drive_p,
                                        FBE_TRACE_LEVEL_ERROR,
                                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                        FBE_PROVISION_DRIVE_DEBUG_FLAG_ZOD_TRACING,
                                        "WRITE_FAST_ZOD:plant sgl with data failed, status:0x%x, line:%d\n", status, __LINE__);

        fbe_payload_block_set_status(block_operation_p,
                                     FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                                     FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNEXPECTED_ERROR);
        fbe_provision_drive_utils_complete_packet(packet_p, FBE_STATUS_OK, 0);
        return status;
    }

    /* allocate and initialize the block operation. */
    new_block_operation_p = fbe_payload_ex_allocate_block_operation(sep_payload_p);
    fbe_payload_block_build_operation(new_block_operation_p,
                                      FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,
                                      zod_start_lba,
                                      zod_block_count,
                                      block_operation_p->block_size,
                                      optimum_block_size,
                                      block_operation_p->pre_read_descriptor);

    /* initialize the block operation with default status. */
    fbe_payload_block_set_status(block_operation_p,
                                 FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID,
                                 FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID);

    /* set completion functon to handle the write same request completion. */
    fbe_transport_set_completion_function(packet_p,
                                          fbe_provision_drive_zod_fast_path_completion,
                                          provision_drive_p);

    /* send packet to the block edge */
    status = fbe_base_config_send_functional_packet((fbe_base_config_t *)provision_drive_p, packet_p, 0);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * end fbe_provision_drive_zero_on_demand_write_fast_path_start() 
 ******************************************************************************/

/*!****************************************************************************
 *  fbe_provision_drive_zero_on_demand_write_fast_path_calculate_memory_chunks()
 ******************************************************************************
 * @brief
 *  This function is used to calculate the number of memory chunks required
 *  for allocation the extended sg list for the zero on demand operation and
 *  for allocating a subpacket to send the zero on demand request.
 *
 * @param provision_drive_p     - pointer to provision drove object.
 * @param packet_p              - pointer to packet.
 * @param chunk_size            - memory chunk size.
 * @param number_of_chunks_p    - pointer to number of chunks.
 *
 * @return fbe_status_t.        - status of the operation.
 *
 * @author
 *   07/14/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_zero_on_demand_write_fast_path_calculate_memory_chunks(fbe_provision_drive_t * provision_drive_p,
                                                                           fbe_packet_t * packet_p,
                                                                           fbe_u32_t memory_chunk_size,
                                                                           fbe_u32_t * number_of_chunks_p)
{
    fbe_lba_t                               host_start_lba = 0;
    fbe_block_count_t                       host_block_count = 0;
    fbe_sg_element_t *                      host_sg_list = NULL;
    fbe_u32_t                               host_sg_list_count = 0;
    fbe_block_count_t                       host_sg_block_count = 0;
    fbe_lba_t                               zod_start_lba = 0;
    fbe_block_count_t                       zod_block_count = 0;
    fbe_u32_t                               zod_sg_list_size = 0;
    fbe_u32_t                               zod_sg_element_count = 0;
    fbe_u32_t                               memory_left_in_chunk = 0;
    fbe_payload_ex_t *                     sep_payload_p = NULL;
    fbe_payload_block_operation_t *         block_operation_p = NULL;
    fbe_payload_metadata_operation_t *      metadata_operation_p = NULL;
    fbe_provision_drive_paged_metadata_t *  paged_data_bits_p = NULL;
    fbe_bool_t                              zero_required;
    fbe_bool_t                              page_valid;

    /* initialize number of chunks we need to allocate as zero. */
    *number_of_chunks_p = 0;

    /* get the sep payload and host sg list. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_payload_ex_get_sg_list(sep_payload_p, &host_sg_list, NULL);

    /* get the metadata operation previous block operation. */
    metadata_operation_p = fbe_payload_ex_get_metadata_operation(sep_payload_p);
    block_operation_p = fbe_payload_ex_get_present_block_operation(sep_payload_p);

    /* get the host start lba and block count. */
    fbe_payload_block_get_lba(block_operation_p, &host_start_lba);
    fbe_payload_block_get_block_count(block_operation_p, &host_block_count);

    /* get the paged data bits from the metadata operation. */
    fbe_payload_metadata_get_metadata_record_data_ptr(metadata_operation_p, (void **)&paged_data_bits_p);

    fbe_provision_drive_metadata_is_paged_metadata_valid(host_start_lba,
                                                         host_block_count, paged_data_bits_p, &page_valid);


    /* if the metadata page is not valid, then we will have to do an invalidate instead of Zeroing */
    if(!page_valid)
    {
        zero_required = FBE_FALSE;
    }

    /* get the zero on demand range before we calculate the sg list count. */
    fbe_provision_drive_zero_on_demand_write_fast_path_calculate_zod_lba_range(host_start_lba,
                                                                               host_block_count,
                                                                               paged_data_bits_p,
                                                                               &zod_start_lba,
                                                                               &zod_block_count);

    /* calculate the number of sg entries we need to allocate for sending fast zero on demand request. */
    fbe_provision_drive_zero_on_demand_write_fast_path_calculate_zod_sg_count(host_start_lba,
                                                                              host_block_count,
                                                                              host_sg_list,
                                                                              zod_start_lba,
                                                                              zod_block_count,
                                                                              &zod_sg_element_count,
                                                                              zero_required);

    /* get the host sg list count. */
    fbe_raid_sg_count_sg_blocks(host_sg_list, &host_sg_list_count, &host_sg_block_count);

    /* memory required for destination sg element list. */
    zod_sg_list_size = zod_sg_element_count * sizeof(fbe_sg_element_t);

    /* round the zod sg list size with 64 bit alignment so that packet
     * start at the 64 bit aligned address. 
     */
    zod_sg_list_size += (zod_sg_list_size % sizeof(fbe_u64_t));

    /* if destination sg list size is same as host sg list then we do not need to extend
     * the sg list and so we can just send the same sg list for the zero on demand 
     *operation.
     */
    if(zod_sg_element_count != (host_sg_list_count + 1))
    {
        /* we need a chunk for the subpacket and one chunk for the sg list. */
        if(zod_sg_list_size > memory_chunk_size)
        {
            /* we need contigeous memory for the sg list. */
            return FBE_STATUS_GENERIC_FAILURE;
        }
        else
        {
            (*number_of_chunks_p)++;
            memory_left_in_chunk = memory_chunk_size - zod_sg_list_size;
            if(memory_left_in_chunk < FBE_MEMORY_CHUNK_SIZE_FOR_PACKET)
            {   
                (*number_of_chunks_p)++;
            }
        }
    }
    else
    {
        /* we need only a single chunk for subpacket. */
        *number_of_chunks_p++;
    }

    /* get the total number of sg count required for the zero on demand processing. */
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_zero_on_demand_write_fast_path_calculate_memory_chunks()
 ******************************************************************************/

/*!****************************************************************************
 *  fbe_provision_drive_zero_on_demand_write_fast_path_calculate_zod_lba_range()
 ******************************************************************************
 * @brief
 *  This function is used to calculate the lba range for the zero on demand
 *  operation for the fast path, it determines pre and post zod range for the
 *  host i/o and extend the host range to make sure the zeroing gets done with
 *  same write operation.
 *  
 *  Example 1:
 *    host_start_lba     host_end_lba (if this chunk need zero).
 *          ^               ^
 *          |               |
 *      |   |               |       |
 *      |   |               |       |
 *      |                           |
 *      |---< extended zod range >--|
 *
 *  Example 2:
 *
 *         host_start_lba         host_end_lba
 *           ^                    ^
 *           |                    |
 *      |    |                |   |                 |
 *      |    |                |   |                 |
 *      |                     |                     |
 *
 *      |< extended zod range>|                       (first chunk need zero)
 *
 *                            |< extended zod range>| (second chunk need zero)
 *
 *      |<--------------extended zod range--------->| (both chunk need zero)
 *
 *
 * @param start_lba             - start lba of the original host range.
 * @param block_count           - block count of the original host request.
 * @param paged_data_bits_p     - paged data bits.
 * @param chunk_size            - memory chunk size.
 * @param number_of_chunks_p    - pointer to number of chunks.
 *
 * @return fbe_status_t.        - status of the operation.
 *
 * @author
 *   07/14/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_zero_on_demand_write_fast_path_calculate_zod_lba_range(fbe_lba_t start_lba,
                                                                           fbe_block_count_t block_count,
                                                                           fbe_provision_drive_paged_metadata_t * paged_data_bits_p,
                                                                           fbe_lba_t * zod_start_lba_p,
                                                                           fbe_block_count_t * zod_block_count_p)
{
    fbe_chunk_count_t   number_of_edge_chunks_need_zero = 0;
    fbe_lba_t           pre_edge_chunk_start_lba;
    fbe_block_count_t   pre_edge_chunk_block_count;
    fbe_lba_t           post_edge_chunk_start_lba;
    fbe_block_count_t   post_edge_chunk_block_count;
    fbe_chunk_size_t    chunk_size = FBE_PROVISION_DRIVE_CHUNK_SIZE;

    /* get the pre and post unaligned lba range which needs zero, if . */
    fbe_provision_drive_utils_calculate_edge_chunk_lba_range_which_need_process(start_lba,
                                                                             block_count,
                                                                             chunk_size,
                                                                             paged_data_bits_p,
                                                                             &pre_edge_chunk_start_lba,
                                                                             &pre_edge_chunk_block_count,
                                                                             &post_edge_chunk_start_lba,
                                                                             &post_edge_chunk_block_count,
                                                                             &number_of_edge_chunks_need_zero);

    if((pre_edge_chunk_start_lba != FBE_LBA_INVALID) && 
       (post_edge_chunk_start_lba != FBE_LBA_INVALID))
    {
        /* if pre edge and post edge both represent the range for the zero on demand
         * then extend the range for the next chunk boundary on both the sides.
         */
        *zod_start_lba_p = pre_edge_chunk_start_lba;
        *zod_block_count_p = (fbe_block_count_t) (post_edge_chunk_start_lba + post_edge_chunk_block_count - pre_edge_chunk_start_lba);
    }
    else if(pre_edge_chunk_start_lba != FBE_LBA_INVALID)
    {
        /* if we have only pre edge which needs zero on demand then extend it to 
         * to next chunk on both the sides if required.
         */
        *zod_start_lba_p = pre_edge_chunk_start_lba;
        if((pre_edge_chunk_start_lba + pre_edge_chunk_block_count) >= (start_lba + block_count))
        {
            /*!@note
             * it covers the case where host lba range covers less than one chunk and and
             * pre edge range for zod should cover atleast one chunk.
             */
            *zod_block_count_p = pre_edge_chunk_block_count;
        }
        else
        {
            *zod_block_count_p = (fbe_block_count_t) (start_lba + block_count - pre_edge_chunk_start_lba);
        }
    }
    else if(post_edge_chunk_start_lba != FBE_LBA_INVALID)
    {
        /* if we have only post edge which needs zero on demand then extend it to 
         * to next chunk on both the sides if required.
         */
        *zod_start_lba_p = start_lba;
        *zod_block_count_p = (fbe_block_count_t) (post_edge_chunk_start_lba + post_edge_chunk_block_count - start_lba);
    }
    else
    {
        /* if we do not need to extend the zero on demand range then that means
         * we can send the zod range as same as host request.
         */
        *zod_start_lba_p = start_lba;
        *zod_block_count_p = block_count;
    }

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_zero_on_demand_write_fast_path_calculate_zod_lba_range()
 ******************************************************************************/

/*!****************************************************************************
 *  fbe_provision_drive_zero_on_demand_write_fast_path_calculate_zod_sg_count()
 ******************************************************************************
 * @brief
 *  This function is used to to calculate the sg count for the zero on demand
 *  operation for the fast path, it takes first host sgl into account and extend
 *  it for zero on demand buffer.
 *
 *  Example:
 *    host_start_lba     host_end_lba (if this chunk need zero).
 *          ^               ^
 *          |               |
 *      |   |               |       |
 *      |   |               |       |
 *      |                           |
 *      |---|               |-------|
 *        ^                     ^
 *        |                     |
 *  sg elements required for this range + host sg element count
 *
 * @param host_sg_list              - host sg list coming from upstream object
 * @param host_start_lba            - host start lba
 * @param host_block_count          - host block count
 * @param zod_start_lba             - zod start lba
 * @param zod_block_count           - zod block count
 * @param chunk_size                - memory chunk size
 * @param dest_sg_element_count_p   - pointer to destination sg element count
 * @param zero_required             - If Zero is required, if not is it invalidate request
 *
 * @return fbe_status_t.        - status of the operation.
 *
 * @author
 *   07/14/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_zero_on_demand_write_fast_path_calculate_zod_sg_count(fbe_lba_t host_start_lba,
                                                                          fbe_block_count_t host_block_count,
                                                                          fbe_sg_element_t * host_sg_list,
                                                                          fbe_lba_t zod_start_lba,
                                                                          fbe_lba_t zod_block_count,
                                                                          fbe_u32_t * dest_sg_element_count_p,
                                                                          fbe_bool_t zero_required)
{
    fbe_block_count_t       host_sg_block_count = 0;
    fbe_u32_t               host_sg_list_count = 0;
    fbe_u32_t               zod_sg_list_count = 0;
    fbe_u32_t       pre_zod_block_count = 0;
    fbe_u32_t       post_zod_block_count = 0;
    fbe_u32_t       bit_bucket_size_in_blocks = 0; 

    /* initialize the destination sgl count with zero before we calculate it. */
    *dest_sg_element_count_p = 0;

    /* get the hostt sgl count. */
    fbe_raid_sg_count_sg_blocks(host_sg_list, &host_sg_list_count, &host_sg_block_count);

    if(zero_required)
    {
        /* get the zero bucket chunk size to determine number of sg elements requred for zero on demand. */
        fbe_memory_get_zero_bit_bucket_size_in_blocks(&bit_bucket_size_in_blocks);
    }
    else
    {
           /* get the invalid pattern bucket chunk size to determine number of sg elements requred for zero on demand. */
        fbe_memory_get_invalid_pattern_bit_bucket_size_in_blocks(&bit_bucket_size_in_blocks);
    }
    if(!bit_bucket_size_in_blocks)
    {
        /* do not allow the zero bucket size in blocks as zero. */
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if((zod_start_lba > host_start_lba) ||
       ((host_start_lba + host_block_count) > (zod_start_lba + zod_block_count)))
    {
        /* do not expect the zod start lba greather than host start lba. */
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if(((host_start_lba - zod_start_lba) > FBE_U32_MAX)||
       (((host_start_lba + host_block_count) > (zod_start_lba + zod_block_count)) > FBE_U32_MAX))
    {
        /* do not expect the zod start lba greather than host start lba. */
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* get the sgl count for the range which we need to zero. */
    pre_zod_block_count = (fbe_u32_t) (host_start_lba - zod_start_lba);
    post_zod_block_count = (fbe_u32_t) (zod_start_lba + zod_block_count - (host_start_lba + host_block_count));

    /* get the sgl count for the pre align zero range. */
    if(pre_zod_block_count % bit_bucket_size_in_blocks) 
    {
        zod_sg_list_count += (pre_zod_block_count / bit_bucket_size_in_blocks) + 1;
    }
    else
    {
        zod_sg_list_count += pre_zod_block_count / bit_bucket_size_in_blocks;
    }

    /* get the sgl count for the post align zero range. */
    if(post_zod_block_count % bit_bucket_size_in_blocks) 
    {
        zod_sg_list_count += (post_zod_block_count / bit_bucket_size_in_blocks) + 1;
    }
    else
    {
        zod_sg_list_count += post_zod_block_count / bit_bucket_size_in_blocks;
    }

    *dest_sg_element_count_p = host_sg_list_count + zod_sg_list_count + 1; //one added for terminator. 
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_zero_on_demand_write_fast_path_calculate_zod_sg_count()
 ******************************************************************************/

/*!****************************************************************************
 *  fbe_provision_drive_zero_on_demand_plant_sgl_with_zero_buffer()
 ******************************************************************************
 * @brief
 *  This function is used to plant the zero on demand sgl with data.
 *
 * @return fbe_status_t.        - status of the operation.
 *
 * @author
 *   07/28/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_zero_on_demand_plant_sgl_with_zero_buffer(fbe_sg_element_t ** zod_sg_list,
                                                              fbe_u32_t zod_sg_list_count,
                                                              fbe_block_count_t zod_block_count,
                                                              fbe_u32_t * used_zod_sg_elements_p)
{
    fbe_u32_t       zero_bit_bucket_size_in_blocks;
    fbe_u32_t       zero_bit_bucket_size_in_bytes;
    fbe_u8_t *      zero_bit_bucket_p = NULL;
    fbe_status_t    status;

    /* get the zero bit bucket address and its size before we plant the sgl */
    status = fbe_memory_get_zero_bit_bucket(&zero_bit_bucket_p, &zero_bit_bucket_size_in_blocks);
    if((status != FBE_STATUS_OK) || (zero_bit_bucket_p == NULL))
    {
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    /* convert the zero bit bucket size in bytes. */
    zero_bit_bucket_size_in_bytes = zero_bit_bucket_size_in_blocks * FBE_BE_BYTES_PER_BLOCK;

    /* update the passed zod sg list with zero bufffer if required. */
    while(zod_block_count != 0)
    {
        if((*used_zod_sg_elements_p) < (zod_sg_list_count - 1))
        {
            (*zod_sg_list)->address = zero_bit_bucket_p;
            if(zod_block_count >= zero_bit_bucket_size_in_blocks)
            {
                (*zod_sg_list)->count = zero_bit_bucket_size_in_bytes;
                zod_block_count -= zero_bit_bucket_size_in_blocks;
            }
            else
            {
                if((zod_block_count * FBE_BE_BYTES_PER_BLOCK) > FBE_U32_MAX)
                {
                    /* sg count should not exceed 32bit limit
                     */
                    return FBE_STATUS_GENERIC_FAILURE;
                }
                (*zod_sg_list)->count = (fbe_u32_t)(zod_block_count * FBE_BE_BYTES_PER_BLOCK);
                zod_block_count = 0;
            }
            (*zod_sg_list)++;
            (*used_zod_sg_elements_p)++;
        }
        else
        {
            return FBE_STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    /* update the last entry with null and count as zero. */
    (*zod_sg_list)->address = NULL;
    (*zod_sg_list)->count = 0;
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_zero_on_demand_plant_sgl_with_zero_buffer()
 ******************************************************************************/

/*!****************************************************************************
 *  fbe_provision_drive_zero_on_demand_plant_sgl_with_invalid_patter_buffer()
 ******************************************************************************
 * @brief
 *  This function is used to plant the invalids on demand sgl with data.
 *
 * @return fbe_status_t.        - status of the operation.
 *
 * @author
 *   03/08/2012 - Created. Ashok Tamilarasan
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_zero_on_demand_plant_sgl_with_invalid_pattern_buffer(fbe_lba_t start_lba,
                                                                         fbe_sg_element_t ** zod_sg_list,
                                                                         fbe_u32_t zod_sg_list_count,
                                                                         fbe_block_count_t zod_block_count,
                                                                         fbe_u32_t * used_zod_sg_elements_p)
{
    fbe_block_count_t       invalid_pattern_bit_bucket_size_in_blocks;
    fbe_u32_t       invalid_pattern_bucket_size_in_bytes;
    fbe_u8_t *      invalid_pattern_bit_bucket_p = NULL;
    fbe_status_t    status;
    fbe_block_count_t       block_count;

    
    /* get the zero bit bucket address and its size before we plant the sgl */
    status = fbe_memory_get_invalid_pattern_bit_bucket(&invalid_pattern_bit_bucket_p, &invalid_pattern_bit_bucket_size_in_blocks);
    if((status != FBE_STATUS_OK) || (invalid_pattern_bit_bucket_p == NULL))
    {
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    /* convert the zero bit bucket size in bytes. */
    invalid_pattern_bucket_size_in_bytes = (fbe_u32_t) invalid_pattern_bit_bucket_size_in_blocks * FBE_BE_BYTES_PER_BLOCK;

    /* update the passed zod sg list with zero bufffer if required. */
    while(zod_block_count != 0)
    {
        if((*used_zod_sg_elements_p) < (zod_sg_list_count - 1))
        {
            (*zod_sg_list)->address = invalid_pattern_bit_bucket_p;
            if(zod_block_count >= invalid_pattern_bit_bucket_size_in_blocks)
            {
                (*zod_sg_list)->count = invalid_pattern_bucket_size_in_bytes;
                zod_block_count -= invalid_pattern_bit_bucket_size_in_blocks;
                block_count = invalid_pattern_bit_bucket_size_in_blocks;
            }
            else
            {
                if((zod_block_count * FBE_BE_BYTES_PER_BLOCK) > FBE_U32_MAX)
                {
                    /* sg count should not exceed 32bit limit
                     */
                    return FBE_STATUS_GENERIC_FAILURE;
                }
                (*zod_sg_list)->count = (fbe_u32_t)(zod_block_count * FBE_BE_BYTES_PER_BLOCK);
                block_count = zod_block_count;
                zod_block_count = 0;
            }
            (*zod_sg_list)++;
            (*used_zod_sg_elements_p)++;
        }
        else
        {
            return FBE_STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    /* update the last entry with null and count as zero. */
    (*zod_sg_list)->address = NULL;
    (*zod_sg_list)->count = 0;
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_zero_on_demand_plant_sgl_with_invalid_pattern_buffer()
 ******************************************************************************/
/*!****************************************************************************
 *  fbe_provision_drive_zod_fast_plant_sgs()
 ******************************************************************************
 * @brief
 *  This function is used to plant the zero on demand pre and post sgs.
 *
 * @param payload_p
 * @param host_start_lba
 * @param host_blocks
 * @param host_sg_list
 * @param zod_start_lba
 * @param zod_block_count
 * @param zod_sg_list
 * @param zero_required
 * 
 * @return fbe_status_t
 *
 * @author
 *  9/14/2012 - Created. Rob Foley
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_zod_fast_plant_sgs(fbe_provision_drive_t *provision_drive_p,
                                       fbe_payload_ex_t *payload_p,
                                       fbe_lba_t host_start_lba,
                                       fbe_block_count_t host_blocks,
                                       fbe_sg_element_t *host_sg_list,
                                       fbe_lba_t zod_start_lba,
                                       fbe_lba_t zod_block_count,
                                       fbe_sg_element_t *zod_sg_list,
                                       fbe_bool_t zero_required)
{
    fbe_block_count_t pre_zod_block_count = 0;
    fbe_block_count_t post_zod_block_count = 0;
    fbe_block_count_t host_sg_block_count = 0;
    fbe_u32_t         host_sg_list_count = 0;
    fbe_status_t      status = FBE_STATUS_OK;
    fbe_sg_element_t *pre_sg_p = NULL;
    fbe_sg_element_t *post_sg_p = NULL;
    fbe_block_count_t invalid_pattern_bit_bucket_size_in_blocks = 0;
    fbe_u8_t *      invalid_pattern_bit_bucket_p = NULL;
    fbe_u32_t       zero_bit_bucket_size_in_blocks;
    fbe_u8_t *      zero_bit_bucket_p = NULL;

    /* get the zero bit bucket address and its size before we plant the sgl */
    status = fbe_memory_get_zero_bit_bucket(&zero_bit_bucket_p, &zero_bit_bucket_size_in_blocks);
    if((status != FBE_STATUS_OK) || (zero_bit_bucket_p == NULL))
    {
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "zod fast plant: get zero bitbucked status %d bitbucket %p\n",
                              status, zero_bit_bucket_p);
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    
    /* get the zero bit bucket address and its size before we plant the sgl */
    status = fbe_memory_get_invalid_pattern_bit_bucket(&invalid_pattern_bit_bucket_p, &invalid_pattern_bit_bucket_size_in_blocks);
    if((status != FBE_STATUS_OK) || (invalid_pattern_bit_bucket_p == NULL))
    {
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "zod fast plant: get zero bitbucked status %d bitbucket %p\n",
                              status, invalid_pattern_bit_bucket_p);
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    fbe_payload_ex_get_pre_sg_list(payload_p, &pre_sg_p);
    fbe_payload_ex_get_post_sg_list(payload_p, &post_sg_p);

    /* get the host sgl element count. */
    fbe_raid_sg_count_sg_blocks(host_sg_list, &host_sg_list_count, &host_sg_block_count);

    if (host_sg_block_count != host_blocks)
    {
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "zod fast plant: host sg blocks 0x%llx != host blocks 0x%llx\n",
                              (unsigned long long)host_sg_block_count, (unsigned long long)host_blocks);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* calculate pre and post range for the zero buffer. */
    pre_zod_block_count = (fbe_block_count_t) (host_start_lba - zod_start_lba);
    post_zod_block_count = (fbe_block_count_t) (zod_start_lba + zod_block_count - (host_start_lba + host_sg_block_count));

    if ((pre_zod_block_count * FBE_BE_BYTES_PER_BLOCK) > FBE_U32_MAX)
    {
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "pre_zod_block_count 0x%x has byte count > FBE_U32_MAX\n",
                              (unsigned int)pre_zod_block_count);
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "host_lba: 0x%llx h_bl: 0x%llx (0x%llx) zod_lba: 0x%llx zod_bl: 0x%llx\n", 
                              (unsigned long long)host_start_lba, (unsigned long long)host_blocks, (unsigned long long)host_sg_block_count, (unsigned long long)zod_start_lba, (unsigned long long)zod_block_count);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if ((post_zod_block_count * FBE_BE_BYTES_PER_BLOCK) > FBE_U32_MAX)
    {
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "pre_zod_block_count 0x%x has byte count > FBE_U32_MAX\n",
                              (unsigned int)pre_zod_block_count);
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "host_lba: 0x%llx h_bl: 0x%llx (0x%llx) zod_lba: 0x%llx zod_bl: 0x%llx\n", 
                              (unsigned long long)host_start_lba, (unsigned long long)host_blocks, (unsigned long long)host_sg_block_count, (unsigned long long)zod_start_lba, (unsigned long long)zod_block_count);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (pre_zod_block_count > 0)
    {
        if (zero_required)
        {
            fbe_sg_element_init(pre_sg_p, 
                                (fbe_u32_t)(pre_zod_block_count * FBE_BE_BYTES_PER_BLOCK),
                                zero_bit_bucket_p);
        }
        else
        {
            fbe_sg_element_init(pre_sg_p, 
                                (fbe_u32_t)(pre_zod_block_count * FBE_BE_BYTES_PER_BLOCK),
                                invalid_pattern_bit_bucket_p);
        }
        fbe_sg_element_terminate(&pre_sg_p[1]);
    }
    if (post_zod_block_count > 0)
    {
        if (zero_required)
        {
            fbe_sg_element_init(post_sg_p, 
                                (fbe_u32_t)(post_zod_block_count * FBE_BE_BYTES_PER_BLOCK),
                                zero_bit_bucket_p);
    
        }
        else
        {
            fbe_sg_element_init(post_sg_p, 
                                (fbe_u32_t)(post_zod_block_count * FBE_BE_BYTES_PER_BLOCK),
                                invalid_pattern_bit_bucket_p);
        }
        fbe_sg_element_terminate(&post_sg_p[1]);
    }
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_zod_fast_plant_sgs()
 ******************************************************************************/

/*!****************************************************************************
 * @fn fbe_provision_drive_zero_on_demand_write_update_metadata_after_write_completion()
 ******************************************************************************
 * @brief
 *
 *  This function is used to handle the completion of the actual i/o operation
 *  in both slow and fast path, it actually updates the paged metadata for
 *  the chunks which correspond to the i/o range.
 * 
 * @param packet_p                      - Pointer to packet.
 * @param packet_completion_context     - Pointer to provision drive object.
 *
 * @return fbe_status_t.    - status of the operation.
 *
 * @author
 *   07/14/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_zero_on_demand_write_update_metadata_after_write_completion(fbe_packet_t * packet_p,
                                                                                fbe_packet_completion_context_t packet_completion_context)
{
    fbe_provision_drive_t *                             provision_drive_p = NULL;
    fbe_payload_metadata_operation_t *                  metadata_operation_p = NULL;    
    fbe_payload_block_operation_t *                     block_operation_p = NULL;
    fbe_payload_block_operation_status_t                block_status;
    fbe_payload_ex_t *                                 sep_payload_p = NULL;
    fbe_status_t                                        status = FBE_STATUS_OK;

    provision_drive_p = (fbe_provision_drive_t *) packet_completion_context;

    /* Get the payload and metadata operation. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    metadata_operation_p =  fbe_payload_ex_get_metadata_operation(sep_payload_p);
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

    /* update the paged metadata after the write completion. */
    status = fbe_provision_drive_zero_on_demand_write_update_paged_metadata(provision_drive_p, packet_p);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * end fbe_provision_drive_zero_on_demand_write_update_metadata_after_write_completion()
 ******************************************************************************/

/*!****************************************************************************
 * @fn fbe_provision_drive_zero_on_demand_write_update_paged_metadata_callback()
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
fbe_provision_drive_zero_on_demand_write_update_paged_metadata_callback(fbe_packet_t * packet, fbe_metadata_callback_context_t context)
{
    fbe_payload_ex_t * sep_payload = fbe_transport_get_payload_ex(packet);
    fbe_payload_metadata_operation_t * mdo = NULL;
    fbe_provision_drive_paged_metadata_t * paged_metadata_p;
    fbe_sg_element_t * sg_list = NULL;
    fbe_sg_element_t * sg_ptr = NULL;
    fbe_lba_t lba_offset;
    fbe_u64_t slot_offset;

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
        /* set the consumed user bit for this chunk. */
        paged_metadata_p->consumed_user_data_bit = 1;

        /* clear the need zero bit if it is set. */
        paged_metadata_p->need_zero_bit = 0;

        /* clear the user zero bit if it is set. */
        paged_metadata_p->user_zero_bit = 0;

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
 * end fbe_provision_drive_zero_on_demand_write_update_paged_metadata_callback()
 ******************************************************************************/

/*!****************************************************************************
 * @fn fbe_provision_drive_zero_on_demand_write_update_paged_metadata()
 ******************************************************************************
 * @brief
 *
 *  This function is used to to update the paged metadata after background zero
 *  operation, after the end of this background 
 * 
 * @param provision_drive_p     - Pointer to provision drive object.
 * @param packet_p              - Pointer to packet.
 *
 * @return fbe_status_t.    - status of the operation.
 *
 * @author
 *   07/14/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_zero_on_demand_write_update_paged_metadata(fbe_provision_drive_t * provision_drive_p,
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
    fbe_lba_t                               metadata_start_lba;
    fbe_block_count_t                       metadata_block_count;

    /* Get the payload and metadata operation. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    metadata_operation_p =  fbe_payload_ex_get_metadata_operation(sep_payload_p);

    /* get the block operation */
    block_operation_p =  fbe_payload_ex_get_present_block_operation(sep_payload_p);

    /* get the start lba and block count. */
    fbe_payload_block_get_lba(block_operation_p, &start_lba);
    fbe_payload_block_get_block_count(block_operation_p, &block_count);

#if 0
	if(start_lba == 0xFFFFFC00){
        fbe_provision_drive_utils_trace(provision_drive_p,
                                        FBE_TRACE_LEVEL_INFO,
                                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                        FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
                                        "%s: start_lba: 0x%llX\n",
                                        __FUNCTION__, start_lba);

	}
#endif

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
                                          fbe_provision_drive_zero_on_demand_write_update_paged_metadata_completion,
                                          provision_drive_p);

    /* Build the paged metadata update request. */
    fbe_payload_metadata_build_paged_update(metadata_operation_p,
                                            &(((fbe_base_config_t *) provision_drive_p)->metadata_element),
                                            metadata_offset,
                                            sizeof(fbe_provision_drive_paged_metadata_t),
                                            chunk_count,
                                            fbe_provision_drive_zero_on_demand_write_update_paged_metadata_callback,
                                            (void *)metadata_operation_p);
    /* Initialize cient_blob, skip the subpacket */
    fbe_provision_drive_metadata_paged_init_client_blob(packet_p, metadata_operation_p);

   /* Get the metadata stripe lock*/
   fbe_metadata_paged_get_lock_range(metadata_operation_p, &metadata_start_lba, &metadata_block_count);

   fbe_payload_metadata_set_metadata_stripe_offset(metadata_operation_p, metadata_start_lba);
   fbe_payload_metadata_set_metadata_stripe_count(metadata_operation_p, metadata_block_count);

   fbe_payload_ex_increment_metadata_operation_level(sep_payload_p);
   status =  fbe_metadata_operation_entry(packet_p);
   return status;
}
/******************************************************************************
 * end fbe_provision_drive_zero_on_demand_write_update_paged_metadata()
 ******************************************************************************/

/*!****************************************************************************
 *  fbe_provision_drive_zero_on_demand_write_update_paged_metadata_completion()
 ******************************************************************************
 * @brief
 *   This function handles the completion of paged metadata xor request. It 
 *   validates the status and sends zeroing checkpoint update request.
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
fbe_provision_drive_zero_on_demand_write_update_paged_metadata_completion(fbe_packet_t * packet_p,
                                                                          fbe_packet_completion_context_t context)
{                      
    fbe_provision_drive_t *             provision_drive_p = NULL;
    fbe_payload_metadata_operation_t *  metadata_operation_p = NULL;
    fbe_payload_block_operation_t *     block_operation_p = NULL;
    fbe_payload_ex_t *                 sep_payload_p = NULL;
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_payload_metadata_status_t       metadata_status;
    fbe_u64_t                           metadata_offset;
    
    provision_drive_p = (fbe_provision_drive_t *) context;

    /* Get the payload, metadata operation operation. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    metadata_operation_p =  fbe_payload_ex_get_metadata_operation(sep_payload_p);

    /* Get the status of the paged metadata operation. */
    fbe_payload_metadata_get_status(metadata_operation_p, &metadata_status);
    status = fbe_transport_get_status_code(packet_p);

    /* Get the metadata offset */
    fbe_payload_metadata_get_metadata_offset(metadata_operation_p, &metadata_offset);

    /* Release the metadata operation. */
    fbe_payload_ex_release_metadata_operation(sep_payload_p, metadata_operation_p);

    /* Get the block operation. */
    block_operation_p = fbe_payload_ex_get_present_block_operation(sep_payload_p);

    if((status != FBE_STATUS_OK) ||
       (metadata_status != FBE_PAYLOAD_METADATA_STATUS_OK))
    {
        fbe_provision_drive_utils_trace(provision_drive_p,
                                        FBE_TRACE_LEVEL_WARNING,
                                        FBE_TRACE_MESSAGE_ID_INFO,
                                        FBE_PROVISION_DRIVE_DEBUG_FLAG_ZOD_TRACING,
                                        "WRITE_ZOD:paged md update failed, offset:0x%llx, metadata_status:0x%x, status:0x%x.\n",
                                        (unsigned long long)metadata_offset,
					metadata_status, status);

        /* Even non-retryable errors back to raid must be acompanied by a state change. 
         * Since we are not guaranteed to be getting a state change 
         * (since it might be caused by a quiesce), we must go ahead and return this as retryable.
         */
        fbe_payload_block_set_status(block_operation_p,
                                     FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED,
                                     FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_POSSIBLE);
        
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        return status;
    }

    return status;
}   
/**********************************************************************************
 * end fbe_provision_drive_zero_on_demand_write_update_paged_metadata_completion()
 **********************************************************************************/

/*!******************************************************************************************
 * @fn fbe_provision_drive_zero_on_demand_update_metadata_after_invalidated_write_completion()
 ********************************************************************************************
 * @brief
 *
 *  This function is used to handle the completion of the actual i/o operation
 *  in both slow and fast path, it actually updates the paged metadata for
 *  the chunks which correspond to the i/o range.
 * 
 * @param packet_p                      - Pointer to packet.
 * @param packet_completion_context     - Pointer to provision drive object.
 *
 * @return fbe_status_t.    - status of the operation.
 *
 * @author
 *   03/08/2012 - Created. Ashok Tamilarasan
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_zero_on_demand_update_metadata_after_invalidated_write_completion(fbe_packet_t * packet_p,
                                                                                      fbe_packet_completion_context_t packet_completion_context)
{
    fbe_provision_drive_t *                             provision_drive_p = NULL;
    fbe_payload_metadata_operation_t *                  metadata_operation_p = NULL;    
    fbe_payload_block_operation_t *                     block_operation_p = NULL;
    fbe_payload_block_operation_status_t                block_status;
    fbe_payload_ex_t *                                 sep_payload_p = NULL;
    fbe_status_t                                        status = FBE_STATUS_OK;

    provision_drive_p = (fbe_provision_drive_t *) packet_completion_context;

    /* Get the payload and metadata operation. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    metadata_operation_p =  fbe_payload_ex_get_metadata_operation(sep_payload_p);
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

    fbe_provision_drive_utils_trace(provision_drive_p,
                                    FBE_TRACE_LEVEL_INFO,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    FBE_PROVISION_DRIVE_DEBUG_FLAG_ZOD_TRACING,
                                    "pvd_zod_update_md_after_inv_write_completion. update paged MD \n");

    /* update the paged metadata after the write completion. */
    status = fbe_provision_drive_zero_on_demand_invalidated_write_update_paged_metadata(provision_drive_p, packet_p);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/*********************************************************************************************
 * end fbe_provision_drive_zero_on_demand_update_metadata_after_invalidated_write_completion()
 ********************************************************************************************/

/*!****************************************************************************
 * @fn fbe_provision_drive_zero_on_demand_invalidated_write_update_paged_metadata()
 ******************************************************************************
 * @brief
 *
 *  This function is used to to update the paged metadata after we complete the
 *  write operation with invalid pattern in the edges
 * 
 * @param provision_drive_p     - Pointer to provision drive object.
 * @param packet_p              - Pointer to packet.
 *
 * @return fbe_status_t.    - status of the operation.
 *
 * @author
 *   03/08/2012 - Created. Ashok Tamilarasan
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_zero_on_demand_invalidated_write_update_paged_metadata(fbe_provision_drive_t * provision_drive_p,
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
    fbe_provision_drive_paged_metadata_t                paged_data;
    fbe_lba_t                               metadata_start_lba;
    fbe_block_count_t                       metadata_block_count;

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

    /* Fill the paged data information */
    fbe_zero_memory(&paged_data, sizeof(paged_data));
    paged_data.valid_bit = 1;
    paged_data.consumed_user_data_bit = 1;

    /* Set the completion function before issuing paged metadata xor operation. */
    fbe_transport_set_completion_function(packet_p,
                                          fbe_provision_drive_zero_on_demand_invalidated_write_update_paged_metadata_completion,
                                          provision_drive_p);

#if 0
    fbe_payload_metadata_build_paged_write_verify(metadata_operation_p,
                                                  &(((fbe_base_config_t *) provision_drive_p)->metadata_element),
                                                  metadata_offset,
                                                  (fbe_u8_t *) &paged_data,
                                                  sizeof(fbe_provision_drive_paged_metadata_t),
                                                  chunk_count);
#else
    fbe_payload_metadata_build_paged_update(metadata_operation_p,
                                            &(((fbe_base_config_t *) provision_drive_p)->metadata_element),
                                            metadata_offset,
                                            sizeof(fbe_provision_drive_paged_metadata_t),
                                            chunk_count,
                                            fbe_provision_drive_metadata_write_verify_paged_metadata_callback,
                                            (void *)metadata_operation_p);
    fbe_payload_metadata_set_metadata_operation_flags(metadata_operation_p, FBE_PAYLOAD_METADATA_OPERATION_PAGED_WRITE_VERIFY);
    /* Initialize cient_blob, skip the subpacket */
    fbe_provision_drive_metadata_paged_init_client_blob(packet_p, metadata_operation_p);
#endif

   /* Get the metadata stripe lock*/
   fbe_metadata_paged_get_lock_range(metadata_operation_p, &metadata_start_lba, &metadata_block_count);

   fbe_payload_metadata_set_metadata_stripe_offset(metadata_operation_p, metadata_start_lba);
   fbe_payload_metadata_set_metadata_stripe_count(metadata_operation_p, metadata_block_count);

   fbe_payload_ex_increment_metadata_operation_level(sep_payload_p);
   status =  fbe_metadata_operation_entry(packet_p);
   return status;
}
/***********************************************************************************
 * end fbe_provision_drive_zero_on_demand_invalidated_write_update_paged_metadata()
 ***********************************************************************************/

/*!****************************************************************************************
 *  fbe_provision_drive_zero_on_demand_invalidated_write_update_paged_metadata_completion()
 ******************************************************************************************
 * @brief
 *   This function handles the completion of paged metadata after reconstruction 
 *
 * @param packet_p          - pointer to a control packet 
 * @param in_context        - context, which is a pointer to the provision drive
 *                            object
 *
 * @return  fbe_status_t  
 *
 * @author
 *   03/08/2012 - Created. Ashok Tamilarasan
 *
 *******************************************************************************************/
static fbe_status_t
fbe_provision_drive_zero_on_demand_invalidated_write_update_paged_metadata_completion(fbe_packet_t * packet_p,
                                                                                      fbe_packet_completion_context_t context)
{                      
    fbe_provision_drive_t *             provision_drive_p = NULL;
    fbe_payload_metadata_operation_t *  metadata_operation_p = NULL;
    fbe_payload_block_operation_t *     block_operation_p = NULL;
    fbe_payload_ex_t *                 sep_payload_p = NULL;
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_payload_metadata_status_t       metadata_status;
    fbe_u64_t                           metadata_offset;
    fbe_lba_t                           start_lba;
    fbe_block_count_t                   block_count;
    
    provision_drive_p = (fbe_provision_drive_t *) context;

    /* Get the payload, metadata operation operation. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    metadata_operation_p =  fbe_payload_ex_get_metadata_operation(sep_payload_p);

    /* Get the status of the paged metadata operation. */
    fbe_payload_metadata_get_status(metadata_operation_p, &metadata_status);
    status = fbe_transport_get_status_code(packet_p);

    /* Get the metadata offset */
    fbe_payload_metadata_get_metadata_offset(metadata_operation_p, &metadata_offset);

    /* Release the metadata operation. */
    fbe_payload_ex_release_metadata_operation(sep_payload_p, metadata_operation_p);

    /* Get the block operation. */
    block_operation_p = fbe_payload_ex_get_present_block_operation(sep_payload_p);

    if((status != FBE_STATUS_OK) ||
       (metadata_status != FBE_PAYLOAD_METADATA_STATUS_OK))
    {
        fbe_provision_drive_utils_trace(provision_drive_p,
                                        FBE_TRACE_LEVEL_WARNING,
                                        FBE_TRACE_MESSAGE_ID_INFO,
                                        FBE_PROVISION_DRIVE_DEBUG_FLAG_ZOD_TRACING,
                                        "WRITE_ZOD:invalidate paged md update failed, offset:0x%llx, metadata_status:0x%x, status:0x%x.\n",
                                        metadata_offset, metadata_status, status);
    }

     //  Set the condition to start the verify invaldiate operation.
    fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const, (fbe_base_object_t *) provision_drive_p,
                           FBE_PROVISION_DRIVE_LIFECYCLE_COND_METADATA_VERIFY_INVALIDATE);


    /* get the lba and block count from the previous block operation. */
    fbe_payload_block_get_lba(block_operation_p, &start_lba);
    fbe_payload_block_get_block_count(block_operation_p, &block_count);

    fbe_provision_drive_utils_trace(provision_drive_p,
                                    FBE_TRACE_LEVEL_INFO,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    FBE_PROVISION_DRIVE_DEBUG_FLAG_VERIFY_TRACING,
                                    "pvd_checkpoint_write_invalidate. Media error.LBA:0x%llx, Cound:%d\n",
                                    start_lba, (int)block_count);

    /* The write with invalid pattern succeded. So return remap error so that RG can 
     * schedule a error verify for this region
     */
    fbe_payload_block_set_status(block_operation_p,
                                 FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS,
                                 FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_REMAP_REQUIRED);
    /* record the bad lba */
    fbe_payload_ex_set_media_error_lba(sep_payload_p, start_lba);

    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    return FBE_STATUS_OK;
}

static fbe_atomic_t fbe_provision_drive_bundle_counter = 0;

static fbe_status_t
fbe_provision_drive_zero_on_demand_bundle_start(fbe_payload_ex_t * sep_payload_p, fbe_provision_drive_t * provision_drive_p)
{
	fbe_metadata_element_t * mde = NULL;
	fbe_packet_t * current_packet = NULL;
	fbe_payload_ex_t * current_payload = NULL;
	fbe_payload_block_operation_t * current_block_operation = NULL;

	fbe_payload_stripe_lock_operation_t * sl = NULL;
	fbe_payload_stripe_lock_operation_t * current_sl = NULL;
	fbe_queue_element_t * current_element = NULL;
	fbe_queue_head_t tmp_queue;
	
	//sl = fbe_payload_ex_get_prev_stripe_lock_operation(sep_payload_p);
	sl = fbe_payload_ex_get_stripe_lock_operation(sep_payload_p);

	mde = (fbe_metadata_element_t *)(fbe_ptrhld_t)sl->cmi_stripe_lock.header.metadata_element_sender;

	fbe_queue_init(&tmp_queue);

	/* Get the stripe lock spin lock */
	//fbe_spinlock_lock(&mde->stripe_lock_spinlock);
	fbe_metadata_stripe_lock_hash(mde, sl);

	current_element = fbe_queue_front(&sl->wait_queue);
	while(current_element != NULL){
		current_sl = fbe_metadata_stripe_lock_queue_element_to_operation(current_element);
		current_element = fbe_queue_next(&sl->wait_queue, current_element);
		if(!(current_sl->flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_PEER_REQUEST) &&
			!(current_sl->flags & FBE_PAYLOAD_STRIPE_LOCK_FLAG_WAITING_FOR_PEER) &&
			!(current_sl->cmi_stripe_lock.flags & FBE_METADATA_CMI_STRIPE_LOCK_FLAG_NEED_RELEASE)){
				
				/* This is very strict check */
				if((sl->stripe.first == current_sl->stripe.first) && (sl->stripe.last == current_sl->stripe.last)){
					current_packet = current_sl->cmi_stripe_lock.packet;
					/* We expect to see here something like that:
					level: 3 callback:  0xe67cb80 sep!fbe_provision_drive_acquire_stripe_lock_completion context: 0x1ed7f170
					level: 2 callback:  0xe6219c0 sep!fbe_provision_drive_handle_io_with_lock context: 0x1ed7f170
					level: 1 callback:  0xe634420 sep!fbe_provision_drive_destroy_iots_completion context: 0x1ed7f170
					level: 0 callback:  0xe621e10 sep!fbe_provision_drive_allocate_packet_master_completion context: 0x1ed7f170
					*/
					/* It is so close to release day that we want to have minimal uncertainty */
					if((current_packet->stack[current_packet->current_level].completion_function == fbe_provision_drive_acquire_stripe_lock_completion) &&
						(current_packet->stack[current_packet->current_level - 1].completion_function == fbe_provision_drive_handle_io_with_lock)){

						current_payload = fbe_transport_get_payload_ex(current_packet);
						current_block_operation = fbe_payload_ex_get_present_block_operation(current_payload);
						/* Check if it is supported block operation */
						if((current_block_operation != NULL) && 
							(fbe_payload_block_operation_opcode_requires_sg(current_block_operation->block_opcode))){

							/* Get current_sl off the wait queue */
							fbe_queue_remove(&current_sl->queue_element);

							/* Release current_sl operation from the packet */
							fbe_payload_ex_release_stripe_lock_operation(current_payload, current_sl);

							/* Fix the completion stack */
							current_packet->current_level -= 2;

							/* Complete the packet */
							fbe_queue_push(&tmp_queue, &current_packet->queue_element);
						}
					} /* packet stack as we expect */ 
					else 
					{
						fbe_provision_drive_utils_trace(provision_drive_p,
														FBE_TRACE_LEVEL_WARNING,
														FBE_TRACE_MESSAGE_ID_INFO,
														FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
														"PVD bundle Uknown completion stack %p and %p\n", 
														current_packet->stack[current_packet->current_level].completion_function,
														current_packet->stack[current_packet->current_level - 1].completion_function);
					}


				} /* SL and current SL are the same cunk */
		} /* Not a peer SL */
	} /* while(current_element != NULL) */

	//fbe_spinlock_unlock(&mde->stripe_lock_spinlock);		
	fbe_metadata_stripe_unlock_hash(mde, sl);

	if(!fbe_queue_is_empty(&tmp_queue)){
		fbe_transport_run_queue_push(&tmp_queue, fbe_provision_drive_send_packet_completion, provision_drive_p);
	}
	fbe_queue_destroy(&tmp_queue);

	return FBE_STATUS_OK;
}




/********************************************************************************************
 * end fbe_provision_drive_zero_on_demand_invalidated_write_update_paged_metadata_completion()
 ********************************************************************************************/

 
