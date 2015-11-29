/***************************************************************************
* Copyright (C) EMC Corporation 2010-2011
* All rights reserved.
* Licensed material -- property of EMC Corporation
***************************************************************************/

/*!**************************************************************************
* @file fbe_cms_buffers_vault.c
***************************************************************************
*
* @brief
*  This file is taking care of the vaulting of the whole buffers(Vault) LUN
*
***************************************************************************/

/*************************
*   INCLUDE FILES
*************************/
#include "fbe_cms_lun_access.h"
#include "fbe_cms_defines.h"
#include "fbe_cms_private.h"
#include "fbe_cms_memory.h"
#include "fbe_cms_vault_layout.h"
#include "fbe_cms_state_machine.h"

static fbe_atomic_t	io_count = 0;

static fbe_status_t fbe_cms_vault_lun_completion(void *context, fbe_status_t status, fbe_payload_block_operation_status_t block_status)
{
	fbe_cms_lun_acces_buffer_id_t 	buffer_id = (fbe_cms_lun_acces_buffer_id_t)context;
	fbe_cms_lun_access_release_seq_buffer(buffer_id);
	fbe_atomic_decrement(&io_count);

	return FBE_STATUS_OK;
}

static fbe_status_t fbe_cms_vault_load_all_buffers_completion(void *context, fbe_status_t status, fbe_payload_block_operation_status_t block_status)
{
	fbe_cms_async_conext_t	*		async_context = (fbe_cms_async_conext_t *)context;

	async_context->completion_status = status;

	fbe_semaphore_release(&async_context->sem, 0, 1, FALSE);
    
    return FBE_STATUS_OK;
}

/*this function is doing the valut of the buffers into the vault LUN*/
fbe_status_t fbe_cms_vault_persist_all_buffers(void)
{
	fbe_u32_t						buffer_count = fbe_cms_memory_get_number_of_buffers();
	fbe_u32_t						current_buffer = 0;
	fbe_u32_t						buffer_index = 0;
    void * 							buffer_ptr = NULL;
	fbe_u32_t						buffers_per_seq_buffer = fbe_cms_lun_access_get_num_of_elements_per_seq_buffer(FBE_CMS_BUFFER_SIZE);/*how many CMS buffers will fit in one 1MB buffer*/
    fbe_cms_lun_acces_buffer_id_t 	buffer_id;
	fbe_u32_t						total_seq_ios = (buffer_count / buffers_per_seq_buffer);
	fbe_u32_t						seq_buffer_index;
	fbe_status_t					status;
	fbe_lba_t						lba = FBE_CMS_VAULT_LUN_START_OF_BUFFER_AREA;
	fbe_atomic_t					io_in_progress = 0;
	fbe_u32_t						wait_count = 0;
	fbe_time_t						start_time = fbe_get_time();
	fbe_cms_sm_event_data_t			event_info;

	cms_trace(FBE_TRACE_LEVEL_INFO, "%s: start vault %d buffers\n", __FUNCTION__, buffer_count); 

    for (seq_buffer_index = 0; seq_buffer_index < total_seq_ios; seq_buffer_index++) {

		/*get a buffer to persist multiple entries into*/
		do {
			buffer_id = fbe_cms_lun_access_get_seq_buffer();/*we'll release it upon completion*/
			if (FBE_BUFFER_ID_INVALID == buffer_id) {
				fbe_thread_delay(100);/*we should get it eventually*/
				wait_count++;
			}

		}while ((FBE_BUFFER_ID_INVALID == buffer_id) && wait_count < 100);

		if (wait_count == 100) {
			cms_trace(FBE_TRACE_LEVEL_ERROR, "%s: cant't get buffer in  10 sec.\n", __FUNCTION__);
			event_info.event = FBE_CMS_EVENT_VAULT_DUMP_FAIL;
			fbe_cms_sm_generate_event(&event_info);
			return FBE_STATUS_GENERIC_FAILURE;
		}

		/*and for this buffer, let's push all the buffers inside*/
		for (buffer_index = 0; buffer_index < buffers_per_seq_buffer; buffer_index ++) {

			/*let's get a tag*/
			buffer_ptr = fbe_cms_memory_get_va_of_buffer(current_buffer);
			if (buffer_ptr == NULL) {
				cms_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL pointer at index %d\n", __FUNCTION__, buffer_index); 
				event_info.event = FBE_CMS_EVENT_VAULT_DUMP_FAIL;
				fbe_cms_sm_generate_event(&event_info);
				return FBE_STATUS_GENERIC_FAILURE;
			}

			current_buffer++;/*for next time*/

			/*and push it in the buffer*/
			status = fbe_cms_lun_access_seq_buffer_write(buffer_id,
														 (fbe_u8_t *)buffer_ptr,
														 FBE_CMS_BUFFER_SIZE);
	
			if (status != FBE_STATUS_OK) {
				cms_trace(FBE_TRACE_LEVEL_ERROR, "%s: failed to write\n", __FUNCTION__); 
				event_info.event = FBE_CMS_EVENT_VAULT_DUMP_FAIL;
				fbe_cms_sm_generate_event(&event_info);
				return FBE_STATUS_GENERIC_FAILURE;
			}
		}

		/*now when we are done, let's persist the whole buffer*/
		fbe_atomic_increment(&io_count);

		fbe_cms_lun_access_seq_buffer_commit(FBE_CMS_VAULT_LUN,
											 lba,
											 buffer_id,
											 fbe_cms_vault_lun_completion,
											 (void *)buffer_id);

		lba += (buffers_per_seq_buffer * (FBE_CMS_BUFFER_SIZE / FBE_BYTES_PER_BLOCK));/*move for next time*/

	}

	wait_count = 0;
	fbe_atomic_exchange(&io_in_progress, io_count);
	while (io_in_progress != 0 && wait_count < 6000) {
		fbe_thread_delay(10);
		fbe_atomic_exchange(&io_in_progress, io_count);
		wait_count++;
	}

	if (wait_count == 6000) {
		cms_trace(FBE_TRACE_LEVEL_ERROR, "%s: timed out waiting for some IO to comple\n", __FUNCTION__); 
		event_info.event = FBE_CMS_EVENT_VAULT_DUMP_FAIL;
		fbe_cms_sm_generate_event(&event_info);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	cms_trace(FBE_TRACE_LEVEL_INFO, "%s:END vault %d buffers in %d sec.\n", __FUNCTION__, buffer_count, fbe_get_elapsed_seconds(start_time)); 
	event_info.event = FBE_CMS_EVENT_VAULT_DUMP_SUCCESS;
	fbe_cms_sm_generate_event(&event_info);
    
	return FBE_STATUS_OK;
}

/*this function load all buffers to memory*/
fbe_status_t fbe_cms_vault_load_all_buffers(void)
{
	fbe_u32_t				buffer_count = fbe_cms_memory_get_number_of_buffers();/*how many buffers we have*/
	fbe_u32_t				buffers_per_seq_buffer = fbe_cms_lun_access_get_num_of_elements_per_seq_buffer(FBE_CMS_BUFFER_SIZE);/*how many CMS buffers will fit in one 1MB buffer*/
	fbe_u32_t 				number_of_seq_read = (buffer_count / buffers_per_seq_buffer);/*how many 1MB reads we need to load them*/
	fbe_u32_t				current_read = 0;
    fbe_u8_t *				read_memory  = NULL;
	fbe_status_t			status;
	fbe_lba_t				lba = FBE_CMS_VAULT_LUN_START_OF_BUFFER_AREA;/*first LBA to read from*/
	fbe_cms_async_conext_t	async_context;
	fbe_u32_t				current_buffer = 0;
	void *					buffer_ptr = NULL;/*pointer to the CMs buffer we read into*/
	fbe_u8_t *				temp_ptr = NULL;
	fbe_cms_sm_event_data_t	event_info;

    /*allocate non physically contig. memory, (no need to, layer below will do that)*/
	read_memory  = (fbe_u8_t *)fbe_memory_ex_allocate(buffers_per_seq_buffer * FBE_CMS_BUFFER_SIZE);
	if (read_memory == NULL) {
		cms_trace(FBE_TRACE_LEVEL_ERROR, "%s:out of memory\n", __FUNCTION__);
		event_info.event = FBE_CMS_EVENT_VAULT_LOAD_HARD_FAIL;
		fbe_cms_sm_generate_event(&event_info);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	fbe_semaphore_init(&async_context.sem, 0, FBE_SEMAPHORE_MAX);

	/*let load one 1MB buffer at a time and from it, copy to our buffers*/
	for (current_read = 0; current_read < number_of_seq_read; current_read ++) {

		temp_ptr = read_memory;/*we will be moving it so need to return to the head*/

		fbe_zero_memory(temp_ptr, buffers_per_seq_buffer * FBE_CMS_BUFFER_SIZE);

		status = fbe_cms_lun_access_seq_buffer_read(FBE_CMS_VAULT_LUN,
													lba,
													read_memory,
													FBE_CMS_BUFFER_SIZE,
													buffers_per_seq_buffer,
													fbe_cms_vault_load_all_buffers_completion,
													(void *)&async_context);

		if (status != FBE_STATUS_PENDING) {
			cms_trace(FBE_TRACE_LEVEL_ERROR, "%s:failed reading seq buffer\n", __FUNCTION__); 
			fbe_semaphore_destroy(&async_context.sem);
			fbe_memory_ex_release(read_memory);
			event_info.event = FBE_CMS_EVENT_VAULT_LOAD_HARD_FAIL;
			fbe_cms_sm_generate_event(&event_info);
			return FBE_STATUS_GENERIC_FAILURE;

		}

		/*wait for the read to complete*/
		status = fbe_semaphore_wait_ms(&async_context.sem, 10000);
		if (status == FBE_STATUS_TIMEOUT || async_context.completion_status != FBE_STATUS_OK) {
			fbe_semaphore_destroy(&async_context.sem);
			cms_trace(FBE_TRACE_LEVEL_ERROR,"%s: error reading buffers\n", __FUNCTION__); 
			fbe_memory_ex_release(read_memory);
			event_info.event = FBE_CMS_EVENT_VAULT_LOAD_HARD_FAIL;
			fbe_cms_sm_generate_event(&event_info);
			return status;
		}

		/*we are good to start copying to the buffers*/
		for (current_buffer = 0; current_buffer < buffers_per_seq_buffer; current_buffer++) {
			/*let's get a pointer to our buffer*/
			buffer_ptr = fbe_cms_memory_get_va_of_buffer(current_buffer);
			if (buffer_ptr == NULL) {
				cms_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL pointer at index %d\n", __FUNCTION__); 
				fbe_memory_ex_release(read_memory);
				fbe_semaphore_destroy(&async_context.sem);
				event_info.event = FBE_CMS_EVENT_VAULT_LOAD_HARD_FAIL;
				fbe_cms_sm_generate_event(&event_info);
				return FBE_STATUS_GENERIC_FAILURE;
			}

			/*and copy into it*/
			fbe_copy_memory(buffer_ptr, read_memory, FBE_CMS_BUFFER_SIZE);

			/*move to next read*/
			temp_ptr += FBE_CMS_BUFFER_SIZE;
		}

		/*let's read from the next LBA range because the seq buffer read many block*/
		lba += fbe_cms_lun_access_get_seq_buffer_block_count();

	}

	fbe_memory_ex_release(read_memory);
	fbe_semaphore_destroy(&async_context.sem);

	event_info.event = FBE_CMS_EVENT_VAULT_LOAD_SUCCESS;
	fbe_cms_sm_generate_event(&event_info);
    
	return FBE_STATUS_OK;

}
