#ifndef FBE_CMS_CDR_LAYOUT_H
#define FBE_CMS_CDR_LAYOUT_H

/***************************************************************************/
/** @file fbe_cms_cdr_layout.h
***************************************************************************
*
* @brief
*  This file contains definitions for accessing the CDR lun
***************************************************************************/

/*************************
*   INCLUDE FILES
*************************/
#include "fbe_cms_cluster_tag.h"
#include "fbe_cms_lun_access.h"
#include "fbe_cms_cdr_interface.h"

#define FBE_CMS_CDR_LUN_VERSION						0x1
#define FBE_CMS_CDR_LUN_MAGIC_NUMBER				0xFFBBEE0012345678

typedef enum fbe_cms_cdr_lun_regions_e{
	FBE_CMS_CDR_LUN_HEADER = 0x0,
	FBE_CMS_CDR_LUN_SPA_VALID,
	FBE_CMS_CDR_LUN_SPB_VALID,
	FBE_CMS_CDR_LUN_VAULT_VALID,
	FBE_CMS_CDR_LUN_CLEAN_DIRTY_TABLE,
	FBE_CMS_CDR_LUN_NUMBER_OF_BUFFERS,
	/*add stuff above this line*/
	FBE_CMS_CDR_LUN_LAST
}fbe_cms_cdr_lun_regions_t;

#define FBE_CMS_CDR_LUN_HEADER_BLOCKS 				20 /*the number of blocks we reserve for the header*/
#define FBE_CMS_CDR_LUN_CLEAN_DIRTY_TABLE_BLOCKS	256 /*(((2 ^FBE_CMS_OWNER_ID_NUM_BITS) / 8) / 512)*/


/*this is the header at LBA 0 of the CDR*/
#pragma pack(1)
typedef struct fbe_cms_cdr_lun_header_s{
    fbe_u64_t	magic_number;
	fbe_u64_t	version;
}fbe_cms_cdr_lun_header_t;
#pragma pack()

fbe_status_t fbe_cms_vault_persist_spa_cdr_valid_info(fbe_cms_cdr_image_valid_info_t *cdr_valid_info,
													  fbe_cms_lun_access_completion completion_func,
													  void * context);

fbe_status_t fbe_cms_vault_persist_spb_cdr_valid_info(fbe_cms_cdr_image_valid_info_t *cdr_valid_info,
													  fbe_cms_lun_access_completion completion_func,
													  void * context);

fbe_status_t fbe_cms_vault_persist_vault_cdr_valid_info(fbe_cms_cdr_image_valid_info_t *cdr_valid_info,
														fbe_cms_lun_access_completion completion_func,
														void * context);


fbe_status_t fbe_cms_vault_read_cdr_spa_valid_info(fbe_cms_cdr_image_valid_info_t *cdr_valid_info,
												   fbe_cms_lun_access_completion completion_func,
												   void * context);

fbe_status_t fbe_cms_vault_read_cdr_spb_valid_info(fbe_cms_cdr_image_valid_info_t *cdr_valid_info,
												   fbe_cms_lun_access_completion completion_func,
												   void * context);

fbe_status_t fbe_cms_vault_read_cdr_vault_valid_info(fbe_cms_cdr_image_valid_info_t *cdr_valid_info,
													 fbe_cms_lun_access_completion completion_func,
													 void * context);

fbe_status_t fbe_cms_vault_is_cdr_lun_valid(fbe_bool_t *valid);

fbe_status_t fbe_cms_vault_read_cdr_header(fbe_cms_cdr_lun_header_t *header,
										   fbe_cms_lun_access_completion completion_func,
										   void * context);

fbe_status_t fbe_cms_vault_persist_cdr_header(fbe_cms_cdr_lun_header_t *header,
											  fbe_cms_lun_access_completion completion_func,
											   void * context);

fbe_status_t fbe_cms_vault_persist_clean_dirty_table(fbe_block_count_t block_offest,
													 fbe_block_count_t block_count,
													 fbe_sg_element_t *sg_list,
													 fbe_cms_lun_access_completion completion_func,
													 void * context);

fbe_status_t fbe_cms_vault_read_clean_dirty_table(fbe_block_count_t block_count,
												  fbe_sg_element_t *sg_list,
												  fbe_cms_lun_access_completion completion_func,
												  void * context);

fbe_status_t fbe_cms_vault_init(void);
fbe_status_t fbe_cms_vault_destroy(void);

#endif /* FBE_CMS_CDR_LAYOUT_H */

