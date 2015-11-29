/***************************************************************************
* Copyright (C) EMC Corporation 2010-2011
* All rights reserved.
* Licensed material -- property of EMC Corporation
***************************************************************************/

/*!**************************************************************************
* @file fbe_cms_cdr_layout.c
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
#include "fbe_cms_cdr_layout.h"
#include "fbe_cms_defines.h"
#include "fbe/fbe_sector.h"
#include "fbe_cms_private.h"

typedef struct cmd_cdr_region_s{
	fbe_cms_cdr_lun_regions_t	region_name;
	fbe_lba_t					size_in_blocks;
	fbe_lba_t					start_lba;
}cmd_cdr_region_t;

/*this table will match region to start lba and size, we populate the start with 0 statically and then 
at init time we calcualte the start lba of each region. This will make sure we don't have to do
it on the fly for each access*/
cmd_cdr_region_t	fbe_cms_cdr_directoy [] =
{
	{FBE_CMS_CDR_LUN_HEADER, FBE_CMS_CDR_LUN_HEADER_BLOCKS, 0},
	{FBE_CMS_CDR_LUN_SPA_VALID, 1, 0},
	{FBE_CMS_CDR_LUN_SPB_VALID, 1, 0},
	{FBE_CMS_CDR_LUN_VAULT_VALID, 1, 0},
	{FBE_CMS_CDR_LUN_CLEAN_DIRTY_TABLE, FBE_CMS_CDR_LUN_CLEAN_DIRTY_TABLE_BLOCKS, 0},
	{FBE_CMS_CDR_LUN_NUMBER_OF_BUFFERS, 1, 0}
};

static void compile_assertion(void)
{
	FBE_ASSERT_AT_COMPILE_TIME(FBE_CMS_CDR_LUN_HEADER_BLOCKS * FBE_BYTES_PER_BLOCK >= sizeof(fbe_cms_cdr_lun_header_t))
}

/*decleration*/
static fbe_cms_vault_calculate_regions_start_lba(void);
static fbe_status_t fbe_cms_vault_get_cdr_region_start_lba(fbe_cms_cdr_lun_regions_t, fbe_lba_t *lba);



/**********************************************************************************************************************************/
fbe_status_t fbe_cms_vault_init(void)
{
	fbe_status_t	status;

	fbe_cms_vault_calculate_regions_start_lba();

	status = fbe_cms_lun_access_init();
	if (status != FBE_STATUS_OK) {
        cms_trace(FBE_TRACE_LEVEL_ERROR, "%s: failed to init LUN access\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

	return FBE_STATUS_OK;

}

fbe_status_t fbe_cms_vault_destroy(void)
{
	fbe_status_t	status;

	status = fbe_cms_lun_access_destroy();
	if (status != FBE_STATUS_OK) {
		cms_trace(FBE_TRACE_LEVEL_ERROR, "%s: failed to destroy LUN access\n", __FUNCTION__);
		/*no return on purpose so we'll finish as much as we can*/
	}


	return status;

}



/*this function will persist the cdr information regarding who has the valid image for SPA*/
fbe_status_t fbe_cms_vault_persist_spa_cdr_valid_info(fbe_cms_cdr_image_valid_info_t *cdr_valid_info,
													  fbe_cms_lun_access_completion completion_func,
													  void * context)
{
	
    fbe_lba_t			lba = 0;
    fbe_status_t		status;
    
    /*where is the LBA for the valid info ?*/
	status = fbe_cms_vault_get_cdr_region_start_lba(FBE_CMS_CDR_LUN_SPA_VALID, &lba);
	if (status != FBE_STATUS_OK) {
		cms_trace(FBE_TRACE_LEVEL_ERROR, "%s: can't get start LBA\n", __FUNCTION__);
		return status;
	}

	/*and send it to the layer that will do the actual write with the CRC for each block*/
	status = fbe_cms_lun_access_persist_single_entry(FBE_CMS_CDR_LUN,
													 lba,
													 sizeof(fbe_cms_cdr_image_valid_info_t),
													 (fbe_u8_t *)cdr_valid_info,
													 completion_func,
													 context);


    return status;
}

/*this function will persist the cdr information regarding who has the valid image for SPB*/
fbe_status_t fbe_cms_vault_persist_spb_cdr_valid_info(fbe_cms_cdr_image_valid_info_t *cdr_valid_info,
													  fbe_cms_lun_access_completion completion_func,
													  void * context)
{
	
    fbe_lba_t			lba = 0;
    fbe_status_t		status;
    
    /*where is the LBA for the valid info ?*/
	status = fbe_cms_vault_get_cdr_region_start_lba(FBE_CMS_CDR_LUN_SPB_VALID, &lba);
	if (status != FBE_STATUS_OK) {
		cms_trace(FBE_TRACE_LEVEL_ERROR, "%s: can't get start LBA\n", __FUNCTION__);
		return status;
	}

	/*and send it to the layer that will do the actual write with the CRC for each block*/
	status = fbe_cms_lun_access_persist_single_entry(FBE_CMS_CDR_LUN,
													 lba,
													 sizeof(fbe_cms_cdr_image_valid_info_t),
													 (fbe_u8_t *)cdr_valid_info,
													 completion_func,
													 context);


    return status;
}

/*this function will persist the cdr information regarding who has the valid image for VAULT*/
fbe_status_t fbe_cms_vault_persist_vault_cdr_valid_info(fbe_cms_cdr_image_valid_info_t *cdr_valid_info,
														fbe_cms_lun_access_completion completion_func,
														void * context)
{
	
    fbe_lba_t			lba = 0;
    fbe_status_t		status;
    
    /*where is the LBA for the valid info ?*/
	status = fbe_cms_vault_get_cdr_region_start_lba(FBE_CMS_CDR_LUN_VAULT_VALID, &lba);
	if (status != FBE_STATUS_OK) {
		cms_trace(FBE_TRACE_LEVEL_ERROR, "%s: can't get start LBA\n", __FUNCTION__);
		return status;
	}

	/*and send it to the layer that will do the actual write with the CRC for each block*/
	status = fbe_cms_lun_access_persist_single_entry(FBE_CMS_CDR_LUN,
													 lba,
													 sizeof(fbe_cms_cdr_image_valid_info_t),
													 (fbe_u8_t *)cdr_valid_info,
													 completion_func,
													 context);


    return status;
}


fbe_status_t fbe_cms_vault_read_cdr_spa_valid_info(fbe_cms_cdr_image_valid_info_t *cdr_valid_info,
											   fbe_cms_lun_access_completion completion_func,
											   void * context)
{
	fbe_lba_t			lba = 0;
    fbe_status_t		status;
    
    /*where is the LBA for the valid info ?*/
	status = fbe_cms_vault_get_cdr_region_start_lba(FBE_CMS_CDR_LUN_SPA_VALID, &lba);
	if (status != FBE_STATUS_OK) {
		cms_trace(FBE_TRACE_LEVEL_ERROR, "%s: can't get start LBA\n", __FUNCTION__);
		return status;
	}

	/*and send it to the layer that will do the actual read*/
	status = fbe_cms_lun_access_read_single_entry(FBE_CMS_CDR_LUN,
												  lba,
												  sizeof(fbe_cms_cdr_image_valid_info_t),
												  (fbe_u8_t *)cdr_valid_info,
												  completion_func,
												  context);

	return status;
}

fbe_status_t fbe_cms_vault_read_cdr_spb_valid_info(fbe_cms_cdr_image_valid_info_t *cdr_valid_info,
											   fbe_cms_lun_access_completion completion_func,
											   void * context)
{
	fbe_lba_t			lba = 0;
    fbe_status_t		status;
    
    /*where is the LBA for the valid info ?*/
	status = fbe_cms_vault_get_cdr_region_start_lba(FBE_CMS_CDR_LUN_SPB_VALID, &lba);
	if (status != FBE_STATUS_OK) {
		cms_trace(FBE_TRACE_LEVEL_ERROR, "%s: can't get start LBA\n", __FUNCTION__);
		return status;
	}

	/*and send it to the layer that will do the actual read*/
	status = fbe_cms_lun_access_read_single_entry(FBE_CMS_CDR_LUN,
												  lba,
												  sizeof(fbe_cms_cdr_image_valid_info_t),
												  (fbe_u8_t *)cdr_valid_info,
												  completion_func,
												  context);

	return status;
}

fbe_status_t fbe_cms_vault_read_cdr_vault_valid_info(fbe_cms_cdr_image_valid_info_t *cdr_valid_info,
											   fbe_cms_lun_access_completion completion_func,
											   void * context)
{
	fbe_lba_t			lba = 0;
    fbe_status_t		status;
    
    /*where is the LBA for the valid info ?*/
	status = fbe_cms_vault_get_cdr_region_start_lba(FBE_CMS_CDR_LUN_VAULT_VALID, &lba);
	if (status != FBE_STATUS_OK) {
		cms_trace(FBE_TRACE_LEVEL_ERROR, "%s: can't get start LBA\n", __FUNCTION__);
		return status;
	}

	/*and send it to the layer that will do the actual read*/
	status = fbe_cms_lun_access_read_single_entry(FBE_CMS_CDR_LUN,
												  lba,
												  sizeof(fbe_cms_cdr_image_valid_info_t),
												  (fbe_u8_t *)cdr_valid_info,
												  completion_func,
												  context);

	return status;
}

fbe_status_t fbe_cms_vault_read_cdr_header(fbe_cms_cdr_lun_header_t *header,
										   fbe_cms_lun_access_completion completion_func,
										   void * context)
{
	fbe_lba_t			lba = 0;
    fbe_status_t		status;
    
    /*where is the LBA for the valid info ?*/
	status = fbe_cms_vault_get_cdr_region_start_lba(FBE_CMS_CDR_LUN_HEADER, &lba);
	if (status != FBE_STATUS_OK) {
		cms_trace(FBE_TRACE_LEVEL_ERROR, "%s: can't get start LBA\n", __FUNCTION__);
		return status;
	}

	/*and send it to the layer that will do the actual read*/
	status = fbe_cms_lun_access_read_single_entry(FBE_CMS_CDR_LUN,
												  lba,
												  sizeof(fbe_cms_cdr_lun_header_t),
												  (fbe_u8_t *)header,
												  completion_func,
												  context);

	return status;
}

fbe_status_t fbe_cms_vault_persist_cdr_header(fbe_cms_cdr_lun_header_t *header,
											  fbe_cms_lun_access_completion completion_func,
											  void * context)
{
	fbe_lba_t			lba = 0;
    fbe_status_t		status;
    
    /*where is the LBA for the valid info ?*/
	status = fbe_cms_vault_get_cdr_region_start_lba(FBE_CMS_CDR_LUN_HEADER, &lba);
	if (status != FBE_STATUS_OK) {
		cms_trace(FBE_TRACE_LEVEL_ERROR, "%s: can't get start LBA\n", __FUNCTION__);
		return status;
	}

	/*and send it to the layer that will do the actual read*/
	status = fbe_cms_lun_access_persist_single_entry(FBE_CMS_CDR_LUN,
													 lba,
													 sizeof(fbe_cms_cdr_lun_header_t),
													 (fbe_u8_t *)header,
													 completion_func,
													 context);

	return status;
}

/*this API is used to perist the table directly to disk. The table should have physically contig. memory and a CRC for each 512 data bytes
block_offest - from the start of the table, how many blocks to progress, this let's us write a single block at a time based on the area we changed
block_count - how many blocks (so we can write the whole table if we want
sg_list - sg_list format of out table memory
*/
fbe_status_t fbe_cms_vault_persist_clean_dirty_table(fbe_block_count_t block_offest,
													 fbe_block_count_t block_count,
													 fbe_sg_element_t *sg_list,
													 fbe_cms_lun_access_completion completion_func,
													 void * context)
{
	fbe_lba_t			lba = 0;
    fbe_status_t		status;
    
    /*where is the LBA  ?*/
    status = fbe_cms_vault_get_cdr_region_start_lba(FBE_CMS_CDR_LUN_CLEAN_DIRTY_TABLE, &lba);
	if (status != FBE_STATUS_OK) {
		cms_trace(FBE_TRACE_LEVEL_ERROR, "%s: can't get start LBA\n", __FUNCTION__);
		return status;
	}

	lba += block_offest;

	/*and send it to the layer that will do the actual write*/
	status = fbe_cms_lun_access_persist_buffer_direct(FBE_CMS_CDR_LUN,
													  lba,
													  block_count,
													  sg_list,
													  completion_func,
													  context);

	return status;
}

/*this API is used to read the table directly to memory. The table should have physically contig. memory and a CRC for each 512 data bytes
We assume ehre we read the whole table so we don't give an 'offset' input
block_count - how many blocks (so we can write the whole table if we want
sg_list - sg_list format of out table memory
*/
fbe_status_t fbe_cms_vault_read_clean_dirty_table(fbe_block_count_t block_count,
												  fbe_sg_element_t *sg_list,
												  fbe_cms_lun_access_completion completion_func,
												  void * context)
{
	fbe_lba_t			lba = 0;
    fbe_status_t		status;
    
    /*where is the LBA  ?*/
    status = fbe_cms_vault_get_cdr_region_start_lba(FBE_CMS_CDR_LUN_CLEAN_DIRTY_TABLE, &lba);
	if (status != FBE_STATUS_OK) {
		cms_trace(FBE_TRACE_LEVEL_ERROR, "%s: can't get start LBA\n", __FUNCTION__);
		return status;
	}

	/*and send it to the layer that will do the actual read*/
	status = fbe_cms_lun_access_load_buffer_direct(FBE_CMS_CDR_LUN,
												   lba,
												   block_count,
												   sg_list,
												   completion_func,
												   context);

	return status;
}

static fbe_cms_vault_calculate_regions_start_lba(void)
{
	fbe_u32_t	region = 0;
	fbe_lba_t	start_lba = 0;

	for (region = 0; region < FBE_CMS_CDR_LUN_LAST; region++) {
		fbe_cms_cdr_directoy[region].start_lba = start_lba;
		start_lba += fbe_cms_cdr_directoy[region].size_in_blocks;
	}
}

static fbe_status_t fbe_cms_vault_get_cdr_region_start_lba(fbe_cms_cdr_lun_regions_t region, fbe_lba_t *lba)
{
	if (region >= 0 && region < FBE_CMS_CDR_LUN_LAST) {
		*lba = fbe_cms_cdr_directoy[region].start_lba;
		return FBE_STATUS_OK;
	}else{
		return FBE_STATUS_GENERIC_FAILURE;
	}
}



