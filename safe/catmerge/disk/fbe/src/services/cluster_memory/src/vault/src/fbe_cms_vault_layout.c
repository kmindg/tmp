/***************************************************************************
* Copyright (C) EMC Corporation 2010-2011
* All rights reserved.
* Licensed material -- property of EMC Corporation
***************************************************************************/

/*!**************************************************************************
* @file fbe_cms_vault_layout.c
***************************************************************************
*
* @brief
*  This file is the top level of accessing the Vault lun, it will interface
*  with the lower level code that does the actual writes
*
***************************************************************************/

/*************************
*   INCLUDE FILES
*************************/
#include "fbe_cms_lun_access.h"
#include "fbe_cms_vault_layout.h"
#include "fbe_cms_defines.h"
#include "fbe_cms_memory.h"


/*this function will persist a single buffer in the Vault LUN, it is asynch so the user needs to supply a callback*/
fbe_status_t fbe_cms_vault_persist_single_buffer(fbe_u32_t buffer_index,
												 fbe_cms_lun_access_completion completion_func,
												 void * context)
{
	
    fbe_lba_t			lba = 0;
	fbe_u8_t *			memory = NULL;
	fbe_status_t		status;
    
    /*do some math to calculate the LBA*/
	lba = FBE_CMS_VAULT_LUN_START_OF_BUFFER_AREA + (buffer_index) * FBE_CMS_VAULT_LUN_BLOCKS_PER_BUFFER;

	/*where is the memory we send*/
    memory = fbe_cms_memory_get_va_of_buffer(buffer_index);
	if (memory = NULL) {
		completion_func(context, FBE_STATUS_GENERIC_FAILURE, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	/*and send it to the layer that will do the actual write with the CRC for each block*/
	status = fbe_cms_lun_access_persist_single_entry(FBE_CMS_VAULT_LUN,
													 lba,
													 FBE_CMS_BUFFER_SIZE,
													 memory,
													 completion_func,
													 context);


    return status;
}



