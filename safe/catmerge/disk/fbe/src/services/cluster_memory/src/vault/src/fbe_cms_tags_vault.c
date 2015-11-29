/***************************************************************************
* Copyright (C) EMC Corporation 2010-2011
* All rights reserved.
* Licensed material -- property of EMC Corporation
***************************************************************************/

/*!**************************************************************************
* @file fbe_cms_tags_vault.c
***************************************************************************
*
* @brief
*  This file is taking care of the vaulting of the whole tag LUN
*
***************************************************************************/

/*************************
*   INCLUDE FILES
*************************/
#include "fbe_cms_lun_access.h"
#include "fbe_cms_defines.h"
#include "fbe_cms_private.h"
#include "fbe_cms_memory.h"
#include "fbe_cms_tag_layout.h"


static fbe_atomic_t	io_count = 0;

static fbe_status_t fbe_cms_vault_tags_lun_completion(void *context, fbe_status_t status, fbe_payload_block_operation_status_t block_status)
{
	fbe_cms_lun_acces_buffer_id_t 	buffer_id = (fbe_cms_lun_acces_buffer_id_t)context;
	fbe_cms_lun_access_release_seq_buffer(buffer_id);
	fbe_atomic_decrement(&io_count);

	return FBE_STATUS_OK;
}

fbe_status_t fbe_cms_vault_persist_all_tags(void)
{
	fbe_u32_t						tag_count = fbe_cms_memory_get_number_of_buffers();
	fbe_u32_t						current_tag = 0;
	fbe_u32_t						tag_index = 0;
    fbe_cms_cluster_tag_t * 		tag_ptr = NULL;
	fbe_u32_t						tags_per_buffer = fbe_cms_lun_access_get_seq_buffer_block_count();
    fbe_cms_lun_acces_buffer_id_t 	buffer_id;
	fbe_u32_t						total_seq_ios;
	fbe_u32_t						buffer_index;
	fbe_status_t					status;
	fbe_lba_t						lba = FBE_CMS_TAG_LUN_START_OF_TAG_AREA;
	fbe_atomic_t					io_in_progress = 0;
	fbe_u32_t						wait_count = 0;

	cms_trace(FBE_TRACE_LEVEL_INFO, "%s: start vault %d tags\n", __FUNCTION__, tag_count); 

	/*the 1MB buffer we'll write into can fill all the tags potentially so we have to be carefull with the math*/
    if (tag_count < tags_per_buffer) {
		total_seq_ios = 1;
		tags_per_buffer = tag_count;
	}else{
		total_seq_ios = (tag_count / tags_per_buffer);
	}

    for (buffer_index = 0; buffer_index < total_seq_ios; buffer_index++) {

		/*get a buffer to persist multiple entries into (TODO, check if we don't get it)*/
		buffer_id = fbe_cms_lun_access_get_seq_buffer();/*we'll release it upon completion*/

		/*and for this buffer, let's push all the tags inside*/
		for (tag_index = 0; tag_index < tags_per_buffer; tag_index ++) {

			/*let's get a tag*/
			tag_ptr = fbe_cms_memory_get_va_of_tag(current_tag);
			if (tag_ptr == NULL) {
				cms_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL pointer at index %tag_index, moving to next one\n", __FUNCTION__); 
				return FBE_STATUS_GENERIC_FAILURE;
			}

			current_tag++;

			/*and push it in the buffer*/
			status = fbe_cms_lun_access_seq_buffer_write(buffer_id,
														 (fbe_u8_t *)tag_ptr,
														 sizeof(fbe_cms_cluster_tag_t));
	
			if (status != FBE_STATUS_OK) {
				cms_trace(FBE_TRACE_LEVEL_ERROR, "%s: failed to write\n", __FUNCTION__); 
				return FBE_STATUS_GENERIC_FAILURE;
			}
		}

		/*now when we are done, let's persist the whole buffer*/

		fbe_atomic_increment(&io_count);

		fbe_cms_lun_access_seq_buffer_commit(FBE_CMS_TAGS_LUN,
											 lba,
											 buffer_id,
											 fbe_cms_vault_tags_lun_completion,
											 (void *)buffer_id);

		lba += tags_per_buffer;/*move for next time*/

	}

	fbe_atomic_exchange(&io_in_progress, io_count);
	while (io_in_progress != 0 && wait_count < 6000) {
		fbe_thread_delay(10);
		fbe_atomic_exchange(&io_in_progress, io_count);
		wait_count++;
	}

	if (wait_count == 6000) {
		cms_trace(FBE_TRACE_LEVEL_ERROR, "%s: timed out waiting for some IO to comple\n", __FUNCTION__); 
		return FBE_STATUS_GENERIC_FAILURE;
	}

	cms_trace(FBE_TRACE_LEVEL_INFO, "%s: END vaulting %d tags\n", __FUNCTION__, tag_count); 

	return FBE_STATUS_OK;
}
