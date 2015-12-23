#include "fbe/fbe_transport.h"
#include "fbe/fbe_payload_ex.h"
#include "fbe_metadata.h"

fbe_status_t 
fbe_payload_metadata_build_nonpaged_write(fbe_payload_metadata_operation_t * metadata_operation, 
										  fbe_metadata_element_t	* metadata_element,
										  fbe_payload_metadata_operation_flags_t flags)
{
	metadata_operation->opcode = FBE_PAYLOAD_METADATA_OPERATION_OPCODE_NONPAGED_WRITE;

	metadata_operation->metadata_element = metadata_element;

	metadata_operation->u.metadata.operation_flags = flags;

	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_metadata_build_nonpaged_read_persist(fbe_payload_metadata_operation_t * metadata_operation, 
										  fbe_metadata_element_t	* metadata_element,
										  fbe_payload_metadata_operation_flags_t flags)
{
	metadata_operation->opcode = FBE_PAYLOAD_METADATA_OPERATION_OPCODE_NONPAGED_READ_PERSIST;

	metadata_operation->metadata_element = metadata_element;

	metadata_operation->u.metadata.operation_flags = flags;

	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_metadata_build_init_memory(fbe_payload_metadata_operation_t * metadata_operation, 
										  fbe_metadata_element_t	* metadata_element)
{
	metadata_operation->opcode = FBE_PAYLOAD_METADATA_OPERATION_OPCODE_INIT_MEMORY;

	metadata_operation->metadata_element = metadata_element;
	metadata_operation->u.metadata.operation_flags = 0;

	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_metadata_build_reinit_memory(fbe_payload_metadata_operation_t * metadata_operation, 
										  fbe_metadata_element_t	* metadata_element)
{
	metadata_operation->opcode = FBE_PAYLOAD_METADATA_OPERATION_OPCODE_REINIT_MEMORY;

	metadata_operation->metadata_element = metadata_element;
	metadata_operation->u.metadata.operation_flags = 0;

	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_metadata_get_status(fbe_payload_metadata_operation_t * metadata_operation, fbe_payload_metadata_status_t * metadata_status)
{
	*metadata_status = metadata_operation->metadata_status;
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_metadata_set_status(fbe_payload_metadata_operation_t * metadata_operation, fbe_payload_metadata_status_t metadata_status)
{
	metadata_operation->metadata_status = metadata_status;
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_metadata_get_opcode(fbe_payload_metadata_operation_t * metadata_operation, fbe_payload_metadata_operation_opcode_t * opcode)
{
	* opcode = 0;

	* opcode = metadata_operation->opcode;
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_metadata_get_search_function(fbe_payload_metadata_operation_t * metadata_operation, metadata_search_fn_t *search_fn)
{

    *search_fn = metadata_operation->u.next_marked_chunk.search_fn;
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_metadata_get_metadata_offset(fbe_payload_metadata_operation_t * metadata_operation, fbe_u64_t * metadata_offset)
{
	*metadata_offset = metadata_operation->u.metadata.offset;
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_metadata_set_metadata_repeat_count(fbe_payload_metadata_operation_t * metadata_operation, fbe_u64_t metadata_repeat_count)
{
	metadata_operation->u.metadata.repeat_count = metadata_repeat_count;
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_metadata_get_metadata_repeat_count(fbe_payload_metadata_operation_t * metadata_operation, fbe_u64_t * metadata_repeat_count)
{
	*metadata_repeat_count = metadata_operation->u.metadata.repeat_count;
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_metadata_set_metadata_offset(fbe_payload_metadata_operation_t * metadata_operation, fbe_u64_t metadata_offset)
{
	metadata_operation->u.metadata.offset = metadata_offset;
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_metadata_get_metadata_record_data_ptr(fbe_payload_metadata_operation_t * metadata_operation, void ** record_data_p)
{
	*record_data_p = metadata_operation->u.metadata.record_data;
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_metadata_get_metadata_stripe_offset(fbe_payload_metadata_operation_t * metadata_operation, fbe_lba_t * stripe_offset)
{
	*stripe_offset = metadata_operation->u.metadata.stripe_offset;
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_metadata_get_metadata_stripe_count(fbe_payload_metadata_operation_t * metadata_operation, fbe_block_count_t * stripe_count)
{
	*stripe_count = metadata_operation->u.metadata.stripe_count;
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_metadata_set_metadata_stripe_offset(fbe_payload_metadata_operation_t * metadata_operation, fbe_lba_t stripe_offset)
{
	metadata_operation->u.metadata.stripe_offset = stripe_offset;
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_metadata_set_metadata_stripe_count(fbe_payload_metadata_operation_t * metadata_operation, fbe_block_count_t stripe_count)
{
	metadata_operation->u.metadata.stripe_count = stripe_count;
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_metadata_build_paged_write(fbe_payload_metadata_operation_t * metadata_operation, 
									   fbe_metadata_element_t	* metadata_element, 
									   fbe_u64_t 	metadata_offset, 
									   fbe_u8_t *	metadata_record_data, 
									   fbe_u32_t    metadata_record_data_size,
									   fbe_u64_t	metadata_repeat_count)
{
	if(metadata_offset % metadata_record_data_size) {
		fbe_base_transport_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
					 FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
					 "%s: offset 0x%llx is no multiple of data size 0x%x",
					 __FUNCTION__,
					 (unsigned long long)metadata_offset,
					 metadata_record_data_size);
	}
	
	metadata_operation->opcode = FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_WRITE;

	metadata_operation->metadata_element = metadata_element;

	metadata_operation->u.metadata.offset = metadata_offset;

	if(metadata_record_data_size > FBE_PAYLOAD_METADATA_MAX_DATA_SIZE){
		return FBE_STATUS_GENERIC_FAILURE;
	}

	#if 0
	if(metadata_offset != 0){
		fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
		                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
		                         "%s: metadata_offset not 0\n", __FUNCTION__);

		return FBE_STATUS_GENERIC_FAILURE;
	}
	#endif

	metadata_operation->u.metadata.record_data_size = metadata_record_data_size;

	metadata_operation->u.metadata.repeat_count = metadata_repeat_count;

	metadata_operation->u.metadata.current_count_private = 0;
	metadata_operation->u.metadata.stripe_offset = 0;
	metadata_operation->u.metadata.stripe_count = 0;
	metadata_operation->u.metadata.operation_flags = 0;
	metadata_operation->u.metadata.client_blob = NULL;

	fbe_copy_memory(metadata_operation->u.metadata.record_data, metadata_record_data, metadata_record_data_size);

	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_metadata_build_paged_write_verify(fbe_payload_metadata_operation_t * metadata_operation, 
											  fbe_metadata_element_t	* metadata_element, 
											  fbe_u64_t 	metadata_offset, 
											  fbe_u8_t *	metadata_record_data, 
											  fbe_u32_t    metadata_record_data_size,
											  fbe_u64_t	metadata_repeat_count)
{
	if(metadata_offset % metadata_record_data_size) {
		fbe_base_transport_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
					 FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
					 "%s: offset 0x%llx is no multiple of data size 0x%x",
					 __FUNCTION__,
					 (unsigned long long)metadata_offset,
					 metadata_record_data_size);
	}
	
	metadata_operation->opcode = FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_WRITE_VERIFY;

	metadata_operation->metadata_element = metadata_element;

	metadata_operation->u.metadata.offset = metadata_offset;

	if(metadata_record_data_size > FBE_PAYLOAD_METADATA_MAX_DATA_SIZE){
		return FBE_STATUS_GENERIC_FAILURE;
	}

	#if 0
	if(metadata_offset != 0){
		fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
		                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
		                         "%s: metadata_offset not 0\n", __FUNCTION__);

		return FBE_STATUS_GENERIC_FAILURE;
	}
	#endif

	metadata_operation->u.metadata.record_data_size = metadata_record_data_size;

	metadata_operation->u.metadata.repeat_count = metadata_repeat_count;

	metadata_operation->u.metadata.current_count_private = 0;
	metadata_operation->u.metadata.stripe_offset = 0;
	metadata_operation->u.metadata.stripe_count = 0;
	metadata_operation->u.metadata.operation_flags = 0;
	metadata_operation->u.metadata.client_blob = NULL;

	fbe_copy_memory(metadata_operation->u.metadata.record_data, metadata_record_data, metadata_record_data_size);

	return FBE_STATUS_OK;
}


fbe_status_t 
fbe_payload_metadata_build_paged_verify_write(fbe_payload_metadata_operation_t * metadata_operation, 
											  fbe_metadata_element_t	* metadata_element, 
											  fbe_u64_t 	metadata_offset, 
											  fbe_u8_t *	metadata_record_data, 
											  fbe_u32_t    metadata_record_data_size,
											  fbe_u64_t	metadata_repeat_count)
{
	if(metadata_offset % metadata_record_data_size) {
		fbe_base_transport_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s: offset 0x%x is no multiple of data size 0x%x",
								 __FUNCTION__, (unsigned int)metadata_offset, metadata_record_data_size);
	}
	
	metadata_operation->opcode = FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_VERIFY_WRITE;

	metadata_operation->metadata_element = metadata_element;

	metadata_operation->u.metadata.offset = metadata_offset;

	if(metadata_record_data_size > FBE_PAYLOAD_METADATA_MAX_DATA_SIZE){
		return FBE_STATUS_GENERIC_FAILURE;
	}

	#if 0
	if(metadata_offset != 0){
		fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
		                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
		                         "%s: metadata_offset not 0\n", __FUNCTION__);

		return FBE_STATUS_GENERIC_FAILURE;
	}
	#endif

	metadata_operation->u.metadata.record_data_size = metadata_record_data_size;

	metadata_operation->u.metadata.repeat_count = metadata_repeat_count;

	metadata_operation->u.metadata.current_count_private = 0;
	metadata_operation->u.metadata.stripe_offset = 0;
	metadata_operation->u.metadata.stripe_count = 0;
	metadata_operation->u.metadata.operation_flags = 0;
	metadata_operation->u.metadata.client_blob = NULL;

	fbe_copy_memory(metadata_operation->u.metadata.record_data, metadata_record_data, metadata_record_data_size);

	return FBE_STATUS_OK;
}

/*!****************************************************************************
 * fbe_payload_metadata_reuse_metadata_operation()
 ******************************************************************************
 * @brief
 *  This function reuses the existing Metadata operation for the next iteration
 * of the same command.
 *
 * @param metadata_operation       - Metadata operation that is going to be reused
 * @param metadata_offset   	   - The metadata offset for the current iteration
 * @param metadata_repeat_count    - Repeat count
 *
 * @return fbe_status_t
 *
 * @author
 *  10/02/2012 - Created. Ashok Tamilarasan
 *
 ******************************************************************************/
fbe_status_t 
fbe_payload_metadata_reuse_metadata_operation(fbe_payload_metadata_operation_t * metadata_operation, 
											  fbe_u64_t 	metadata_offset, 
											  fbe_u64_t	    metadata_repeat_count)
{
	/* Since this is reusing the existing operation just init what is required.
	 * We dont want to touch the opcode or data that we want to write since
	 * that is inited the first time
	 */
	metadata_operation->u.metadata.offset = metadata_offset;
	metadata_operation->u.metadata.repeat_count = metadata_repeat_count;
	metadata_operation->u.metadata.current_count_private = 0;
	metadata_operation->u.metadata.stripe_offset = 0;
	metadata_operation->u.metadata.stripe_count = 0;
	metadata_operation->u.metadata.operation_flags = 0;
	return FBE_STATUS_OK;
}


fbe_status_t 
fbe_payload_metadata_build_paged_set_bits(fbe_payload_metadata_operation_t * metadata_operation, 
										  fbe_metadata_element_t	* metadata_element, 
										  fbe_u64_t 	metadata_offset, 
										  fbe_u8_t *	metadata_record_data, 
										  fbe_u32_t     metadata_record_data_size,
										  fbe_u64_t	    metadata_repeat_count)
{
	if(metadata_offset % metadata_record_data_size) {
		fbe_base_transport_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
					 FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
					 "%s: offset 0x%llx is no multiple of data size 0x%x",
					 __FUNCTION__,
					 (unsigned long long)metadata_offset,
					 metadata_record_data_size);
	}

	metadata_operation->opcode = FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_SET_BITS;

	metadata_operation->metadata_element = metadata_element;

	metadata_operation->u.metadata.offset = metadata_offset;

	if(metadata_record_data_size > FBE_PAYLOAD_METADATA_MAX_DATA_SIZE){
		return FBE_STATUS_GENERIC_FAILURE;
	}
	metadata_operation->u.metadata.record_data_size = metadata_record_data_size;

	metadata_operation->u.metadata.repeat_count = metadata_repeat_count;
	metadata_operation->u.metadata.current_count_private = 0;
	metadata_operation->u.metadata.stripe_offset = 0;
	metadata_operation->u.metadata.stripe_count = 0;
	metadata_operation->u.metadata.operation_flags = 0;
	metadata_operation->u.metadata.client_blob = NULL;

	fbe_copy_memory(metadata_operation->u.metadata.record_data, metadata_record_data, metadata_record_data_size);
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_metadata_build_paged_clear_bits(fbe_payload_metadata_operation_t * metadata_operation, 
										  fbe_metadata_element_t	* metadata_element, 
										  fbe_u64_t 	metadata_offset, 
										  fbe_u8_t *	metadata_record_data, 
										  fbe_u32_t     metadata_record_data_size,
										  fbe_u64_t	    metadata_repeat_count)
{
	if(metadata_offset % metadata_record_data_size) {
		fbe_base_transport_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
					 FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
					 "%s: offset 0x%llx is no multiple of data size 0x%x",
					 __FUNCTION__,
					 (unsigned long long)metadata_offset,
					 metadata_record_data_size);
	}

	metadata_operation->opcode = FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_CLEAR_BITS;

	metadata_operation->metadata_element = metadata_element;

	metadata_operation->u.metadata.offset = metadata_offset;

	if(metadata_record_data_size > FBE_PAYLOAD_METADATA_MAX_DATA_SIZE){
		return FBE_STATUS_GENERIC_FAILURE;
	}
	metadata_operation->u.metadata.record_data_size = metadata_record_data_size;

	metadata_operation->u.metadata.repeat_count = metadata_repeat_count;
	metadata_operation->u.metadata.current_count_private = 0;
	metadata_operation->u.metadata.stripe_offset = 0;
	metadata_operation->u.metadata.stripe_count = 0;
	metadata_operation->u.metadata.operation_flags = 0;
	metadata_operation->u.metadata.client_blob = NULL;


	fbe_copy_memory(metadata_operation->u.metadata.record_data, metadata_record_data, metadata_record_data_size);
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_metadata_build_nonpaged_set_bits(fbe_payload_metadata_operation_t * metadata_operation, 
										  fbe_metadata_element_t	* metadata_element, 
										  fbe_u64_t 	metadata_offset, 
										  fbe_u8_t *	metadata_record_data, 
										  fbe_u32_t     metadata_record_data_size,
										  fbe_u64_t	    metadata_repeat_count,
									      fbe_payload_metadata_operation_flags_t flags)
{
	metadata_operation->opcode = FBE_PAYLOAD_METADATA_OPERATION_OPCODE_NONPAGED_SET_BITS;

	metadata_operation->metadata_element = metadata_element;

	metadata_operation->u.metadata.offset = metadata_offset;

	if(metadata_record_data_size > FBE_PAYLOAD_METADATA_MAX_DATA_SIZE){
		return FBE_STATUS_GENERIC_FAILURE;
	}
	metadata_operation->u.metadata.record_data_size = metadata_record_data_size;

	metadata_operation->u.metadata.repeat_count = metadata_repeat_count;
	metadata_operation->u.metadata.current_count_private = 0;
	metadata_operation->u.metadata.stripe_offset = 0;
	metadata_operation->u.metadata.stripe_count = 0;
	metadata_operation->u.metadata.operation_flags = flags;
	metadata_operation->u.metadata.client_blob = NULL;

	fbe_copy_memory(metadata_operation->u.metadata.record_data, metadata_record_data, metadata_record_data_size);
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_metadata_build_nonpaged_clear_bits(fbe_payload_metadata_operation_t * metadata_operation, 
										  fbe_metadata_element_t	* metadata_element, 
										  fbe_u64_t 	metadata_offset, 
										  fbe_u8_t *	metadata_record_data, 
										  fbe_u32_t     metadata_record_data_size,
										  fbe_u64_t	    metadata_repeat_count)
{
	metadata_operation->opcode = FBE_PAYLOAD_METADATA_OPERATION_OPCODE_NONPAGED_CLEAR_BITS;

	metadata_operation->metadata_element = metadata_element;

	metadata_operation->u.metadata.offset = metadata_offset;

	if(metadata_record_data_size > FBE_PAYLOAD_METADATA_MAX_DATA_SIZE){
		return FBE_STATUS_GENERIC_FAILURE;
	}
	metadata_operation->u.metadata.record_data_size = metadata_record_data_size;

	metadata_operation->u.metadata.repeat_count = metadata_repeat_count;
	metadata_operation->u.metadata.current_count_private = 0;
	metadata_operation->u.metadata.stripe_offset = 0;
	metadata_operation->u.metadata.stripe_count = 0;
	metadata_operation->u.metadata.operation_flags = 0;
	metadata_operation->u.metadata.client_blob = NULL;

	fbe_copy_memory(metadata_operation->u.metadata.record_data, metadata_record_data, metadata_record_data_size);
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_metadata_build_nonpaged_set_checkpoint(fbe_payload_metadata_operation_t * metadata_operation, 
												  fbe_metadata_element_t	* metadata_element, 
												  fbe_u64_t 	metadata_offset, 
                                                  fbe_u64_t 	second_metadata_offset,      
												  fbe_lba_t 	checkpoint,
												  fbe_payload_metadata_operation_flags_t flags)
{
	metadata_operation->opcode = FBE_PAYLOAD_METADATA_OPERATION_OPCODE_NONPAGED_SET_CHECKPOINT;

	metadata_operation->metadata_element = metadata_element;
	metadata_operation->u.metadata.offset = metadata_offset;
    metadata_operation->u.metadata.second_offset = second_metadata_offset;

	metadata_operation->u.metadata.record_data_size = sizeof(fbe_lba_t);

	metadata_operation->u.metadata.repeat_count = 0;
	metadata_operation->u.metadata.current_count_private = 0;
	metadata_operation->u.metadata.stripe_offset = 0;
	metadata_operation->u.metadata.stripe_count = 0;
	metadata_operation->u.metadata.operation_flags = flags;
	metadata_operation->u.metadata.client_blob = NULL;
	fbe_copy_memory(metadata_operation->u.metadata.record_data, &checkpoint, sizeof(fbe_lba_t));
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_metadata_build_nonpaged_force_set_checkpoint(fbe_payload_metadata_operation_t * metadata_operation, 
                                                         fbe_metadata_element_t	* metadata_element, 
                                                         fbe_u64_t 	metadata_offset, 
                                                         fbe_u64_t 	second_metadata_offset,      
                                                         fbe_lba_t 	checkpoint)
{
	metadata_operation->opcode = FBE_PAYLOAD_METADATA_OPERATION_OPCODE_NONPAGED_FORCE_SET_CHECKPOINT;

	metadata_operation->metadata_element = metadata_element;
	metadata_operation->u.metadata.offset = metadata_offset;
    metadata_operation->u.metadata.second_offset = second_metadata_offset;

	metadata_operation->u.metadata.record_data_size = sizeof(fbe_lba_t);

	metadata_operation->u.metadata.repeat_count = 0;
	metadata_operation->u.metadata.current_count_private = 0;
	metadata_operation->u.metadata.stripe_offset = 0;
	metadata_operation->u.metadata.stripe_count = 0;
	metadata_operation->u.metadata.operation_flags = 0;
	metadata_operation->u.metadata.client_blob = NULL;

	fbe_copy_memory(metadata_operation->u.metadata.record_data, &checkpoint, sizeof(fbe_lba_t));
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_metadata_build_nonpaged_incr_checkpoint(fbe_payload_metadata_operation_t * metadata_operation, 
                                                  fbe_metadata_element_t	* metadata_element, 
                                                  fbe_u64_t 	metadata_offset, 
                                                    fbe_u64_t 	second_metadata_offset,      
                                                  fbe_lba_t 	checkpoint,
                                                  fbe_u64_t	    repeat_count)
{
	metadata_operation->opcode = FBE_PAYLOAD_METADATA_OPERATION_OPCODE_NONPAGED_INCR_CHECKPOINT;

	metadata_operation->metadata_element = metadata_element;
	metadata_operation->u.metadata.offset = metadata_offset;
    metadata_operation->u.metadata.second_offset = second_metadata_offset;

	metadata_operation->u.metadata.record_data_size = sizeof(fbe_lba_t);

	metadata_operation->u.metadata.repeat_count = repeat_count; /* How much to increment */
	metadata_operation->u.metadata.current_count_private = 0;
	metadata_operation->u.metadata.stripe_offset = 0;
	metadata_operation->u.metadata.stripe_count = 0;
	metadata_operation->u.metadata.operation_flags = 0;
	metadata_operation->u.metadata.client_blob = NULL;

	fbe_copy_memory(metadata_operation->u.metadata.record_data, &checkpoint, sizeof(fbe_lba_t));
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_metadata_build_np_inc_chkpt_no_peer(fbe_payload_metadata_operation_t * metadata_operation, 
                                                fbe_metadata_element_t  * metadata_element, 
                                                fbe_u64_t   metadata_offset, 
                                                fbe_u64_t   second_metadata_offset,      
                                                fbe_lba_t   checkpoint,
                                                fbe_u64_t       repeat_count)
{
	metadata_operation->opcode = FBE_PAYLOAD_METADATA_OPERATION_OPCODE_NONPAGED_INCR_CHECKPOINT_NO_PEER;

	metadata_operation->metadata_element = metadata_element;
	metadata_operation->u.metadata.offset = metadata_offset;
    metadata_operation->u.metadata.second_offset = second_metadata_offset;

	metadata_operation->u.metadata.record_data_size = sizeof(fbe_lba_t);

	metadata_operation->u.metadata.repeat_count = repeat_count; /* How much to increment */
	metadata_operation->u.metadata.current_count_private = 0;
	metadata_operation->u.metadata.stripe_offset = 0;
	metadata_operation->u.metadata.stripe_count = 0;
	metadata_operation->u.metadata.operation_flags = 0;
	metadata_operation->u.metadata.client_blob = NULL;

	fbe_copy_memory(metadata_operation->u.metadata.record_data, &checkpoint, sizeof(fbe_lba_t));
	return FBE_STATUS_OK;
}

/* Paged */

fbe_status_t 
fbe_payload_metadata_build_paged_xor_bits(fbe_payload_metadata_operation_t * metadata_operation, 
                                          fbe_metadata_element_t    * metadata_element, 
                                          fbe_u64_t     metadata_offset, 
                                          fbe_u8_t *    metadata_record_data, 
                                          fbe_u32_t     metadata_record_data_size,
                                          fbe_u64_t     metadata_repeat_count)
{
	if(metadata_offset % metadata_record_data_size) {
		fbe_base_transport_trace(FBE_TRACE_LEVEL_WARNING,
					 FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
					 "%s: offset 0x%llx is no multiple of data size 0x%x\n",
					 __FUNCTION__,
					 (unsigned long long)metadata_offset,
					 metadata_record_data_size);
	}

    metadata_operation->opcode = FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_XOR_BITS;

    metadata_operation->metadata_element = metadata_element;

    metadata_operation->u.metadata.offset = metadata_offset;

    if(metadata_record_data_size > FBE_PAYLOAD_METADATA_MAX_DATA_SIZE){
        return FBE_STATUS_GENERIC_FAILURE;
    }
    metadata_operation->u.metadata.record_data_size = metadata_record_data_size;

    metadata_operation->u.metadata.repeat_count = metadata_repeat_count;
	metadata_operation->u.metadata.current_count_private = 0;
	metadata_operation->u.metadata.stripe_offset = 0;
	metadata_operation->u.metadata.stripe_count = 0;
	metadata_operation->u.metadata.operation_flags = 0;
	metadata_operation->u.metadata.client_blob = NULL;

	fbe_copy_memory(metadata_operation->u.metadata.record_data, metadata_record_data, metadata_record_data_size);
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_metadata_build_paged_get_bits(fbe_payload_metadata_operation_t * metadata_operation, 
										  fbe_metadata_element_t	* metadata_element, 
										  fbe_u64_t 	metadata_offset, 
										  fbe_u8_t *	metadata_record_data, 
										  fbe_u32_t     metadata_record_data_size,
										  fbe_u32_t     paged_data_size)
{
	if(metadata_offset % paged_data_size) {
		fbe_base_transport_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
					 FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
					 "%s: offset 0x%llx is no multiple of paged data size 0x%x",
					 __FUNCTION__,
					 (unsigned long long)metadata_offset,
					 paged_data_size);
	}

	if(metadata_record_data_size % paged_data_size) {
		fbe_base_transport_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
					 FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
					 "%s: offset 0x%llx is no multiple of record size 0x%x",
					 __FUNCTION__,
					 (unsigned long long)metadata_offset,
					 metadata_record_data_size);
	}

	metadata_operation->opcode = FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_GET_BITS;

	metadata_operation->metadata_element = metadata_element;

	metadata_operation->u.metadata.offset = metadata_offset;

	if(metadata_record_data_size > FBE_PAYLOAD_METADATA_MAX_DATA_SIZE){
		return FBE_STATUS_GENERIC_FAILURE;
	}
	metadata_operation->u.metadata.record_data_size = metadata_record_data_size;
	metadata_operation->u.metadata.stripe_offset = 0;
	metadata_operation->u.metadata.stripe_count = 0;
	metadata_operation->u.metadata.operation_flags = 0;
	metadata_operation->u.metadata.client_blob = NULL;

	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_metadata_build_paged_get_next_marked_bits(fbe_payload_metadata_operation_t * metadata_operation, 
                                          fbe_metadata_element_t    * metadata_element, 
                                          fbe_u64_t     metadata_offset, 
                                          fbe_u8_t *    metadata_record_data, 
                                          fbe_u32_t     metadata_record_data_size,
                                          fbe_u32_t     paged_data_size,
                                          metadata_search_fn_t   search_fn,
                                          fbe_u32_t              search_size,
                                          context_t              context)
{
    if(metadata_offset % paged_data_size) {
        fbe_base_transport_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s: offset 0x%llx is no multiple of data size 0x%x",
                                 __FUNCTION__,
				 (unsigned long long)metadata_offset,
				 metadata_record_data_size);
    }

    if(metadata_record_data_size % paged_data_size) {
        fbe_base_transport_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s: offset 0x%llx is no multiple of data size 0x%x",
                                 __FUNCTION__,
				 (unsigned long long)metadata_offset,
				 metadata_record_data_size);
    }

    metadata_operation->opcode = FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_GET_NEXT_MARKED_BITS;

    metadata_operation->metadata_element = metadata_element;

    metadata_operation->u.metadata.offset = metadata_offset;

    if(metadata_record_data_size > FBE_PAYLOAD_METADATA_MAX_DATA_SIZE){
        return FBE_STATUS_GENERIC_FAILURE;
    }
    metadata_operation->u.metadata.record_data_size = metadata_record_data_size;
    metadata_operation->u.metadata.stripe_offset = 0;
    metadata_operation->u.metadata.stripe_count = 0;

    if(search_size > FBE_PAYLOAD_METADATA_MAX_DATA_SIZE) 
    {
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s: search size 0x%x is greater than max search size 0x%x",
                                 __FUNCTION__, search_size, FBE_PAYLOAD_METADATA_MAX_DATA_SIZE);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    metadata_operation->u.next_marked_chunk.search_fn = search_fn;
    metadata_operation->u.next_marked_chunk.search_data_size = search_size;
    metadata_operation->u.next_marked_chunk.context = context;

	metadata_operation->u.metadata.operation_flags = 0;
	metadata_operation->u.metadata.client_blob = NULL;
	//fbe_zero_memory(metadata_operation->u.metadata.record_data, FBE_PAYLOAD_METADATA_MAX_DATA_SIZE);


    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_metadata_build_unregister_element(fbe_payload_metadata_operation_t * metadata_operation, 
											  fbe_metadata_element_t	* metadata_element)
{
	metadata_operation->opcode = FBE_PAYLOAD_METADATA_OPERATION_OPCODE_UNREGISTER_ELEMENT;
	metadata_operation->metadata_element = metadata_element;
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_metadata_build_memory_update(fbe_payload_metadata_operation_t * metadata_operation, 
											  fbe_metadata_element_t	* metadata_element)
{
	metadata_operation->opcode = FBE_PAYLOAD_METADATA_OPERATION_OPCODE_MEMORY_UPDATE;
	metadata_operation->metadata_element = metadata_element;
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_metadata_build_nonpaged_init(fbe_payload_metadata_operation_t * metadata_operation, 
										  fbe_metadata_element_t	* metadata_element)
{
	metadata_operation->opcode = FBE_PAYLOAD_METADATA_OPERATION_OPCODE_NONPAGED_INIT;

	metadata_operation->metadata_element = metadata_element;

	return FBE_STATUS_OK;
}


fbe_status_t 
fbe_payload_metadata_build_nonpaged_persist(fbe_payload_metadata_operation_t * metadata_operation, 
										    fbe_metadata_element_t	* metadata_element)
{
	metadata_operation->opcode = FBE_PAYLOAD_METADATA_OPERATION_OPCODE_NONPAGED_PERSIST;

	metadata_operation->metadata_element = metadata_element;

	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_metadata_build_nonpaged_post_persist(fbe_payload_metadata_operation_t * metadata_operation, 
												 fbe_metadata_element_t	* metadata_element)
{
	metadata_operation->opcode = FBE_PAYLOAD_METADATA_OPERATION_OPCODE_NONPAGED_POST_PERSIST;

	metadata_operation->metadata_element = metadata_element;

	return FBE_STATUS_OK;
}


fbe_status_t 
fbe_payload_metadata_build_nonpaged_preset(fbe_payload_metadata_operation_t * metadata_operation,
										   fbe_u8_t *	 metadata_record_data, 
										   fbe_u32_t    metadata_record_data_size,
										   fbe_object_id_t object_id)
{
	metadata_operation->opcode = FBE_PAYLOAD_METADATA_OPERATION_OPCODE_NONPAGED_PRESET;

	metadata_operation->metadata_element = NULL;
	metadata_operation->u.metadata.offset = object_id;

	if(metadata_record_data_size > FBE_PAYLOAD_METADATA_MAX_DATA_SIZE){
		return FBE_STATUS_GENERIC_FAILURE;
	}
	metadata_operation->u.metadata.record_data_size = metadata_record_data_size;

	fbe_copy_memory(metadata_operation->u.metadata.record_data, metadata_record_data, metadata_record_data_size);

	return FBE_STATUS_OK;
}
fbe_status_t 
fbe_payload_metadata_build_nonpaged_write_verify(fbe_payload_metadata_operation_t * metadata_operation, 
										    fbe_metadata_element_t	* metadata_element)
{
	metadata_operation->opcode = FBE_PAYLOAD_METADATA_OPERATION_OPCODE_NONPAGED_WRITE_VERIFY;

	metadata_operation->metadata_element = metadata_element;

	return FBE_STATUS_OK;
}


fbe_status_t 
fbe_payload_metadata_build_paged_read(fbe_payload_metadata_operation_t * metadata_operation, 
                                          fbe_metadata_element_t    * metadata_element, 
                                          fbe_u64_t     metadata_offset, 
                                          //fbe_u8_t *    metadata_record_data, 
                                          fbe_u32_t     metadata_record_data_size,
                                          fbe_u64_t     metadata_repeat_count,
                                          fbe_metadata_callback_function_t callback_func,
                                          fbe_metadata_callback_context_t context)
{
    if(metadata_offset % metadata_record_data_size) {
        fbe_base_transport_trace(FBE_TRACE_LEVEL_WARNING,
                                 FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                 "%s: offset 0x%llx is no multiple of data size 0x%x\n",
                                 __FUNCTION__,
                                 (unsigned long long)metadata_offset,
                                 metadata_record_data_size);
    }

    metadata_operation->opcode = FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_READ;

    metadata_operation->metadata_element = metadata_element;

    metadata_operation->u.metadata_callback.offset = metadata_offset;

    if(metadata_record_data_size > FBE_PAYLOAD_METADATA_MAX_DATA_SIZE){
        return FBE_STATUS_GENERIC_FAILURE;
    }
    metadata_operation->u.metadata_callback.record_data_size = metadata_record_data_size;

    metadata_operation->u.metadata_callback.repeat_count = metadata_repeat_count;
    metadata_operation->u.metadata_callback.current_count_private = 0;
    metadata_operation->u.metadata_callback.stripe_offset = 0;
    metadata_operation->u.metadata_callback.stripe_count = 0;
    metadata_operation->u.metadata_callback.operation_flags = 0;
    metadata_operation->u.metadata_callback.client_blob = NULL;

    //fbe_copy_memory(metadata_operation->u.metadata_callback.record_data, metadata_record_data, metadata_record_data_size);

    metadata_operation->u.metadata_callback.callback_function = callback_func;
    metadata_operation->u.metadata_callback.callback_context = context;
    
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_metadata_build_paged_update(fbe_payload_metadata_operation_t * metadata_operation, 
                                          fbe_metadata_element_t    * metadata_element, 
                                          fbe_u64_t     metadata_offset, 
                                          //fbe_u8_t *    metadata_record_data, 
                                          fbe_u32_t     metadata_record_data_size,
                                          fbe_u64_t     metadata_repeat_count,
                                          fbe_metadata_callback_function_t callback_func,
                                          fbe_metadata_callback_context_t context)
{
    if(metadata_offset % metadata_record_data_size) {
        fbe_base_transport_trace(FBE_TRACE_LEVEL_WARNING,
                                 FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                 "%s: offset 0x%llx is no multiple of data size 0x%x\n",
                                 __FUNCTION__,
                                 (unsigned long long)metadata_offset,
                                 metadata_record_data_size);
    }

    metadata_operation->opcode = FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_UPDATE;

    metadata_operation->metadata_element = metadata_element;

    metadata_operation->u.metadata_callback.offset = metadata_offset;

    if(metadata_record_data_size > FBE_PAYLOAD_METADATA_MAX_DATA_SIZE){
        return FBE_STATUS_GENERIC_FAILURE;
    }
    metadata_operation->u.metadata_callback.record_data_size = metadata_record_data_size;

    metadata_operation->u.metadata_callback.repeat_count = metadata_repeat_count;
    metadata_operation->u.metadata_callback.current_count_private = 0;
    metadata_operation->u.metadata_callback.stripe_offset = 0;
    metadata_operation->u.metadata_callback.stripe_count = 0;
    metadata_operation->u.metadata_callback.operation_flags = 0;
    metadata_operation->u.metadata_callback.client_blob = NULL;

    //fbe_copy_memory(metadata_operation->u.metadata_callback.record_data, metadata_record_data, metadata_record_data_size);

    metadata_operation->u.metadata_callback.callback_function = callback_func;
    metadata_operation->u.metadata_callback.callback_context = context;
    
    return FBE_STATUS_OK;
}
fbe_status_t 
fbe_payload_metadata_build_paged_write_verify_update(fbe_payload_metadata_operation_t * metadata_operation, 
                                                     fbe_metadata_element_t    * metadata_element, 
                                                     fbe_u64_t     metadata_offset, 
                                                     //fbe_u8_t *    metadata_record_data, 
                                                     fbe_u32_t     metadata_record_data_size,
                                                     fbe_u64_t     metadata_repeat_count,
                                                     fbe_metadata_callback_function_t callback_func,
                                                     fbe_metadata_callback_context_t context)
{
    if(metadata_offset % metadata_record_data_size) {
        fbe_base_transport_trace(FBE_TRACE_LEVEL_WARNING,
                                 FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                 "%s: offset 0x%llx is no multiple of data size 0x%x\n",
                                 __FUNCTION__,
                                 (unsigned long long)metadata_offset,
                                 metadata_record_data_size);
    }

    metadata_operation->opcode = FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_WRITE_VERIFY_UPDATE;

    metadata_operation->metadata_element = metadata_element;

    metadata_operation->u.metadata_callback.offset = metadata_offset;

    if(metadata_record_data_size > FBE_PAYLOAD_METADATA_MAX_DATA_SIZE){
        return FBE_STATUS_GENERIC_FAILURE;
    }
    metadata_operation->u.metadata_callback.record_data_size = metadata_record_data_size;

    metadata_operation->u.metadata_callback.repeat_count = metadata_repeat_count;
    metadata_operation->u.metadata_callback.current_count_private = 0;
    metadata_operation->u.metadata_callback.stripe_offset = 0;
    metadata_operation->u.metadata_callback.stripe_count = 0;
    metadata_operation->u.metadata_callback.operation_flags = 0;
    metadata_operation->u.metadata_callback.client_blob = NULL;

    //fbe_copy_memory(metadata_operation->u.metadata_callback.record_data, metadata_record_data, metadata_record_data_size);

    metadata_operation->u.metadata_callback.callback_function = callback_func;
    metadata_operation->u.metadata_callback.callback_context = context;
    
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_payload_metadata_set_metadata_operation_flags(fbe_payload_metadata_operation_t * metadata_operation, 
                                                  fbe_payload_metadata_operation_flags_t flags)
{
    metadata_operation->u.metadata_callback.operation_flags |= flags;
    return FBE_STATUS_OK;
}
