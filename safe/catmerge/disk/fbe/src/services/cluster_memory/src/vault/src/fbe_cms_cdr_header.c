/***************************************************************************
* Copyright (C) EMC Corporation 2010-2011
* All rights reserved.
* Licensed material -- property of EMC Corporation
***************************************************************************/

/*!**************************************************************************
* @file fbe_cms_cdr_header.c
***************************************************************************
*
* @brief
*  This file will take care of reading and stamping the header of the CDR LUN
*
***************************************************************************/

/*************************
*   INCLUDE FILES
*************************/
#include "fbe_cms_lun_access.h"
#include "fbe_cms_cdr_layout.h"
#include "fbe_cms_defines.h"
#include "fbe_cms_private.h"


typedef struct header_read_context_s{
	fbe_semaphore_t		sem;
	fbe_status_t 		status;
}header_read_context_t;

static fbe_status_t read_header_completion(void *context, fbe_status_t status, fbe_payload_block_operation_status_t block_status)
{
	header_read_context_t	*	read_context = (header_read_context_t	*)context;

	read_context->status = status;
	fbe_semaphore_release(&read_context->sem, 0, 1, FALSE);
	return FBE_STATUS_OK;
}

/*this function will look at the CDR header and will check if it's valid, if not, it will stamp it and say it was not valid*/
fbe_status_t fbe_cms_vault_is_cdr_lun_valid(fbe_bool_t *valid)
{
    fbe_cms_cdr_lun_header_t	header;
	header_read_context_t		read_context;
	fbe_status_t				status;

    fbe_semaphore_init(&read_context.sem, 0, 1);
	read_context.status = FBE_STATUS_GENERIC_FAILURE;/*should be overwritten by a good one if everything works fine*/
	fbe_zero_memory(&header, sizeof(fbe_cms_cdr_lun_header_t));

	/*let's read the header first*/
	status = fbe_cms_vault_read_cdr_header(&header, read_header_completion, &read_context);
	if (status != FBE_STATUS_PENDING) {
		fbe_semaphore_destroy(&read_context.sem);
		cms_trace(FBE_TRACE_LEVEL_ERROR, "%s: failed sending read\n", __FUNCTION__); 
		return FBE_STATUS_GENERIC_FAILURE;
	}

	status = fbe_semaphore_wait_ms(&read_context.sem, NULL);
	if (status == FBE_STATUS_TIMEOUT) {
		fbe_semaphore_destroy(&read_context.sem);
		cms_trace(FBE_TRACE_LEVEL_ERROR, "%s: read timeout\n", __FUNCTION__); 
		return FBE_STATUS_GENERIC_FAILURE;
	}

	/*once we are here, we finished the read, let's look at the header*/
	if (header.magic_number == FBE_CMS_CDR_LUN_MAGIC_NUMBER) {
		*valid = FBE_TRUE;
		fbe_semaphore_destroy(&read_context.sem);
		cms_trace(FBE_TRACE_LEVEL_INFO, "%s: found valid header\n", __FUNCTION__); 
		return FBE_STATUS_OK;
	}

	cms_trace(FBE_TRACE_LEVEL_INFO, "%s: no valid header, stamping...\n", __FUNCTION__);

	header.magic_number = FBE_CMS_CDR_LUN_MAGIC_NUMBER;
	header.version = FBE_CMS_CDR_LUN_VERSION;

	status = fbe_cms_vault_persist_cdr_header(&header, read_header_completion, &read_context);
	if (status != FBE_STATUS_PENDING) {
		fbe_semaphore_destroy(&read_context.sem);
		cms_trace(FBE_TRACE_LEVEL_ERROR, "%s: failed sending write\n", __FUNCTION__); 
		return FBE_STATUS_GENERIC_FAILURE;
	}

	status = fbe_semaphore_wait_ms(&read_context.sem, NULL);
	if (status == FBE_STATUS_TIMEOUT) {
		fbe_semaphore_destroy(&read_context.sem);
		cms_trace(FBE_TRACE_LEVEL_ERROR, "%s: write timeout\n", __FUNCTION__); 
		return FBE_STATUS_GENERIC_FAILURE;
	}

	cms_trace(FBE_TRACE_LEVEL_INFO, "%s: stamping done\n", __FUNCTION__);
	*valid = FBE_FALSE;/*since it was valid so the use will know not to bother and read the LUN since it has no good info*/

	fbe_semaphore_destroy(&read_context.sem);
	return FBE_STATUS_OK;

}
