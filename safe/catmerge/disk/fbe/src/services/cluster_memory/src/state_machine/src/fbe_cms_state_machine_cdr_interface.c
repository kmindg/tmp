/***************************************************************************
* Copyright (C) EMC Corporation 2010-2011
* All rights reserved.
* Licensed material -- property of EMC Corporation
***************************************************************************/

/*!**************************************************************************
* @file fbe_cms_state_machine_cdr_interface.c
***************************************************************************
*
* @brief
*  This file contains state machine CDR interaction interfaces
*  
***************************************************************************/

/*************************
*   INCLUDE FILES
*************************/

#include "fbe_cms_private.h"
#include "fbe_cms_state_machine.h"

static fbe_status_t fbe_cms_sm_set_image_on_disk(fbe_cms_cdr_image_valid_info_t *valid_info, image_valid_type_t sector);

static fbe_status_t fbe_cms_sm_read_image_completion(void *context, fbe_status_t status, fbe_payload_block_operation_status_t block_status)
{
	fbe_cms_async_conext_t	*		async_context = (fbe_cms_async_conext_t *)context;

	async_context->completion_status = status;

	fbe_semaphore_release(&async_context->sem, 0, 1, FALSE);
    
    return FBE_STATUS_OK;
}


fbe_status_t fbe_cms_sm_read_image_valid_data(fbe_cmi_sp_id_t sp_id,
											  fbe_bool_t *this_sp_valid,
											  fbe_bool_t *peer_valid,
											  fbe_bool_t *vault_valid)
{
	fbe_cms_cdr_image_valid_info_t 	spa_cdr_valid_info;
	fbe_cms_cdr_image_valid_info_t 	spb_cdr_valid_info;
	fbe_cms_cdr_image_valid_info_t 	vault_cdr_valid_info;
	fbe_cms_async_conext_t			async_context;
	fbe_status_t					status;
	fbe_bool_t						cdr_header_valid;

	/*let's read the data about the images for all 3 palyers, SPA, SPB and VAULT*/

	status = fbe_cms_vault_is_cdr_lun_valid(&cdr_header_valid);
	if (status != FBE_STATUS_OK) {
		cms_trace(FBE_TRACE_LEVEL_ERROR,"%s: can't interact with CDR LUN\n", __FUNCTION__); 
		return status;
	}

	/*must be first time we read the CDR(header was stamped by fbe_cms_vault_is_cdr_lun_valid so we are good for next time)*/
	if (!cdr_header_valid){
		*this_sp_valid = FBE_FALSE;
		*peer_valid = FBE_FALSE;
		*vault_valid = FBE_FALSE;

		return FBE_STATUS_OK;
	}

	fbe_semaphore_init(&async_context.sem, 0, FBE_SEMAPHORE_MAX);
    
	status  = fbe_cms_vault_read_cdr_spa_valid_info(&spa_cdr_valid_info,
													fbe_cms_sm_read_image_completion,
													(void *)&async_context);

	if (status != FBE_STATUS_PENDING) {
		fbe_semaphore_destroy(&async_context.sem);
		cms_trace(FBE_TRACE_LEVEL_ERROR,"%s: error reading spa valid info\n", __FUNCTION__); 
		return status;
	}

	status = fbe_semaphore_wait_ms(&async_context.sem, 10000);
	if (status == FBE_STATUS_TIMEOUT || async_context.completion_status != FBE_STATUS_OK) {
		fbe_semaphore_destroy(&async_context.sem);
		cms_trace(FBE_TRACE_LEVEL_ERROR,"%s: error reading spa image valid data\n", __FUNCTION__); 
		return status;
	}

	status = fbe_cms_vault_read_cdr_spb_valid_info(&spb_cdr_valid_info,
												   fbe_cms_sm_read_image_completion,
												   (void *)&async_context);

	if (status != FBE_STATUS_PENDING) {
		fbe_semaphore_destroy(&async_context.sem);
		cms_trace(FBE_TRACE_LEVEL_ERROR,"%s: error reading spb valid info\n", __FUNCTION__); 
		return status;
	}

	status = fbe_semaphore_wait_ms(&async_context.sem, 10000);
	if (status == FBE_STATUS_TIMEOUT || async_context.completion_status != FBE_STATUS_OK) {
		fbe_semaphore_destroy(&async_context.sem);
		cms_trace(FBE_TRACE_LEVEL_ERROR,"%s: error reading spb valid info\n", __FUNCTION__); 
		return status;
	}

	status = fbe_cms_vault_read_cdr_vault_valid_info(&vault_cdr_valid_info,
													 fbe_cms_sm_read_image_completion,
													 (void *)&async_context);

	if (status != FBE_STATUS_PENDING) {
		fbe_semaphore_destroy(&async_context.sem);
		cms_trace(FBE_TRACE_LEVEL_ERROR,"%s: error reading vault valid info\n", __FUNCTION__); 
		return status;
	}

	status = fbe_semaphore_wait_ms(&async_context.sem, 10000);
	if (status == FBE_STATUS_TIMEOUT || async_context.completion_status != FBE_STATUS_OK) {
		fbe_semaphore_destroy(&async_context.sem);
		cms_trace(FBE_TRACE_LEVEL_ERROR,"%s: error reading vault valid info\n", __FUNCTION__); 
		return status;
	}

	fbe_semaphore_destroy(&async_context.sem);

	/*let's see who has the largest generation number*/
	if (spa_cdr_valid_info.generation_number > spb_cdr_valid_info.generation_number &&
		spa_cdr_valid_info.generation_number > vault_cdr_valid_info.generation_number) {

		if (sp_id == FBE_CMI_SP_ID_A) {
			*this_sp_valid = FBE_TRUE;
			*peer_valid = FBE_FALSE;
			*vault_valid = FBE_FALSE;
		}else{
			*this_sp_valid = FBE_FALSE;
			*peer_valid = FBE_TRUE;
			*vault_valid = FBE_FALSE;
		}

		return FBE_STATUS_OK;

	}

	if (spb_cdr_valid_info.generation_number > spa_cdr_valid_info.generation_number &&
		spb_cdr_valid_info.generation_number > vault_cdr_valid_info.generation_number) {

		if (sp_id == FBE_CMI_SP_ID_B) {
			*this_sp_valid = FBE_TRUE;
			*peer_valid = FBE_FALSE;
			*vault_valid = FBE_FALSE;
		}else{
			*this_sp_valid = FBE_FALSE;
			*peer_valid = FBE_TRUE;
			*vault_valid = FBE_FALSE;
		}

		return FBE_STATUS_OK;

	}

	if (vault_cdr_valid_info.generation_number > spa_cdr_valid_info.generation_number &&
		vault_cdr_valid_info.generation_number > spa_cdr_valid_info.generation_number) {

        *this_sp_valid = FBE_FALSE;
		*peer_valid = FBE_FALSE;
		*vault_valid = FBE_TRUE;
		return FBE_STATUS_OK;
	}

	if (spb_cdr_valid_info.generation_number == spa_cdr_valid_info.generation_number &&
		spa_cdr_valid_info.generation_number > spa_cdr_valid_info.generation_number) {

        *this_sp_valid = FBE_TRUE;
		*peer_valid = FBE_TRUE;
		*vault_valid = FBE_FALSE;
		return FBE_STATUS_OK;
	}

	if (spb_cdr_valid_info.generation_number == spa_cdr_valid_info.generation_number &&
		spa_cdr_valid_info.generation_number == spa_cdr_valid_info.generation_number) {

        *this_sp_valid = FBE_TRUE;
		*peer_valid = FBE_TRUE;
		*vault_valid = FBE_TRUE;
		return FBE_STATUS_OK;
	}

	cms_trace(FBE_TRACE_LEVEL_ERROR,"%s: bad combination:A:0x%llX, B:0x%llX V:0x%llX\n", __FUNCTION__,
		  (unsigned long long)spa_cdr_valid_info.generation_number,
		  (unsigned long long)spb_cdr_valid_info.generation_number,
		  (unsigned long long)vault_cdr_valid_info.generation_number); 

	return FBE_STATUS_GENERIC_FAILURE;

}

/*call this function to indicate the image of this module is invalid*/
fbe_status_t fbe_cms_sm_set_image_invalid(image_valid_type_t sector)
{
    fbe_cms_cdr_image_valid_info_t 	cdr_valid_info;

	cdr_valid_info.generation_number = 0;

	return fbe_cms_sm_set_image_on_disk(&cdr_valid_info, sector);
}
/*call this function to indicate the image of this module is the latest and greatet*/
fbe_status_t fbe_cms_sm_set_image_valid(image_valid_type_t sector,
										 fbe_u64_t new_generation_number)
{
    fbe_cms_cdr_image_valid_info_t 	cdr_valid_info;

	cdr_valid_info.generation_number = new_generation_number;
    
	return fbe_cms_sm_set_image_on_disk(&cdr_valid_info, sector);
}

static fbe_status_t fbe_cms_sm_set_image_on_disk(fbe_cms_cdr_image_valid_info_t *valid_info, image_valid_type_t sector)
{
    fbe_status_t					status;
	fbe_cms_async_conext_t			async_context;
    
    fbe_semaphore_init(&async_context.sem, 0, FBE_SEMAPHORE_MAX);

	switch (sector) {
	case SPA_VALID_SECTOR:
		status = fbe_cms_vault_persist_spa_cdr_valid_info(valid_info,
														  fbe_cms_sm_read_image_completion,
														  (void *)&async_context);

		break;
	case SPB_VALID_SECTOR:
		status = fbe_cms_vault_persist_spb_cdr_valid_info(valid_info,
														  fbe_cms_sm_read_image_completion,
														  (void *)&async_context);
		break;
	case VAULT_VALID_SECTOR:
		status = fbe_cms_vault_persist_vault_cdr_valid_info(valid_info,
															fbe_cms_sm_read_image_completion,
															(void *)&async_context);
		break;
	default:
		cms_trace(FBE_TRACE_LEVEL_ERROR,"%s: illegal sector:%d\n", __FUNCTION__, sector); 
		return FBE_STATUS_GENERIC_FAILURE;
	}

	if (status != FBE_STATUS_PENDING) {
		fbe_semaphore_destroy(&async_context.sem);
		cms_trace(FBE_TRACE_LEVEL_ERROR,"%s: error setting image\n", __FUNCTION__); 
		return FBE_STATUS_GENERIC_FAILURE;
	}
	
	status = fbe_semaphore_wait_ms(&async_context.sem, 10000);
	fbe_semaphore_destroy(&async_context.sem);

	if (status == FBE_STATUS_TIMEOUT || async_context.completion_status != FBE_STATUS_OK) {
		cms_trace(FBE_TRACE_LEVEL_ERROR,"%s: timeout oor error waiting to write image valid data\n", __FUNCTION__); 
		return status;
	}

	return FBE_STATUS_OK;

}
