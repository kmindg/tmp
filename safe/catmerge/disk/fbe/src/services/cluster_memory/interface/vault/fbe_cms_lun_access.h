#ifndef FBE_CMS_LUN_ACCESS_H
#define FBE_CMS_LUN_ACCESS_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_cms_lun_access.h
 ***************************************************************************
 *
 * @brief
 *  This file contains LUN access related interfaces.
 *
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_payload_block_operation.h"
#include "fbe_cms_defines.h"


#define FBE_CMS_LUN_ACCESS_MAX_BLOCKS_PER_TRANSACTION (FBE_CMS_BUFFER_SIZE / FBE_BYTES_PER_BLOCK )
#define FBE_BUFFER_ID_INVALID 0xFFFFFFFFFFFFFFFF

typedef enum fbe_cms_lun_type_e{
	FBE_CMS_TAGS_LUN = 0,
	FBE_CMS_CDR_LUN,
	FBE_CMS_VAULT_LUN
}fbe_cms_lun_type_t;

typedef fbe_status_t (*fbe_cms_lun_access_completion)(void *context, fbe_status_t status, fbe_payload_block_operation_status_t block_status);
typedef fbe_u64_t fbe_cms_lun_acces_buffer_id_t;

fbe_status_t fbe_cms_lun_access_init(void);
fbe_status_t fbe_cms_lun_access_destroy(void);
fbe_status_t fbe_cms_lun_access_persist_single_entry(fbe_cms_lun_type_t lun_type,
													 fbe_lba_t lba,
													 fbe_u32_t data_size_in_bytes,
													 fbe_u8_t * data_ptr,
													 fbe_cms_lun_access_completion completion_func,
													 void *context);

fbe_status_t fbe_cms_lun_access_read_single_entry(fbe_cms_lun_type_t lun_type,
												  fbe_lba_t lba,
												  fbe_u32_t data_size_in_bytes,
												  fbe_u8_t * data_ptr,
												  fbe_cms_lun_access_completion completion_func,
												  void *context);

fbe_u32_t fbe_cms_lun_access_get_seq_buffer_block_count(void);

fbe_cms_lun_acces_buffer_id_t fbe_cms_lun_access_get_seq_buffer(void);
fbe_status_t fbe_cms_lun_access_release_seq_buffer(fbe_cms_lun_acces_buffer_id_t buffer_id);

fbe_status_t fbe_cms_lun_access_seq_buffer_write(fbe_cms_lun_acces_buffer_id_t buffer_id,
												 fbe_u8_t *data_ptr,
												 fbe_u32_t data_lengh);

fbe_status_t fbe_cms_lun_access_seq_buffer_commit(fbe_cms_lun_type_t lun_type,
												  fbe_lba_t lba,
												  fbe_cms_lun_acces_buffer_id_t buffer_id,
												  fbe_cms_lun_access_completion completion_func,
												  void *context);

fbe_status_t fbe_cms_lun_access_seq_buffer_read(fbe_cms_lun_type_t lun_type,
												fbe_lba_t lba,
												fbe_u8_t *data_ptr,
												fbe_u32_t element_size_in_bytes,
												fbe_u32_t number_of_elements,
												fbe_cms_lun_access_completion completion_func,
												void *context);

fbe_u32_t fbe_cms_lun_access_get_num_of_elements_per_seq_buffer(fbe_u32_t element_size);

/*this will persist a buffer that already contains the CRC and is guaranteed to be physically contigous*/
fbe_status_t fbe_cms_lun_access_persist_buffer_direct(fbe_cms_lun_type_t lun_type,
													  fbe_lba_t lba,
													  fbe_block_count_t block_count,
													  fbe_sg_element_t *sg_list,
													  fbe_cms_lun_access_completion completion_func,
													  void *context);

/*this will read into a buffer that already contains the CRC area and is guaranteed to be physically contigous*/
fbe_status_t fbe_cms_lun_access_load_buffer_direct(fbe_cms_lun_type_t lun_type,
												   fbe_lba_t lba,
												   fbe_block_count_t block_count,
												   fbe_sg_element_t *sg_list,
												   fbe_cms_lun_access_completion completion_func,
												   void *context);

#endif /*FBE_CMS_LUN_ACCESS_H*/

