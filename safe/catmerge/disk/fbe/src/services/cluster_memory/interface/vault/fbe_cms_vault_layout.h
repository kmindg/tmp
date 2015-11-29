#ifndef FBE_CMS_VAULT_LAYOUT_H
#define FBE_CMS_VAULT_LAYOUT_H

/***************************************************************************/
/** @file fbe_cms_vault_layout.h
***************************************************************************
*
* @brief
*  This file contains definitions for accessing the Vault lun
***************************************************************************/

/*************************
*   INCLUDE FILES
*************************/
#include "fbe_cms_cluster_tag.h"
#include "fbe_cms_lun_access.h"
#include "fbe/fbe_sector.h"


#define FBE_CMS_VAULT_LUN_HEADER_BLOCKS 				20 /*the number of blocks we reserve for the header*/
#define FBE_CMS_VAULT_LUN_HEADER_BLOCK_LBA_START		0
#define FBE_CMS_VAULT_LUN_BLOCKS_PER_BUFFER				(FBE_CMS_BUFFER_SIZE / FBE_BYTES_PER_BLOCK) /*how many blocks we use to hold the buffer*/
#define FBE_CMS_VAULT_LUN_START_OF_BUFFER_AREA			(FBE_CMS_VAULT_LUN_HEADER_BLOCK_LBA_START + FBE_CMS_VAULT_LUN_HEADER_BLOCKS)


fbe_status_t fbe_cms_vault_persist_single_buffer(fbe_u32_t buffer_index,
												  fbe_cms_lun_access_completion completion_func,
												  void * context);

fbe_status_t fbe_cms_vault_persist_all_buffers(void);
fbe_status_t fbe_cms_vault_load_all_buffers(void);



#endif /* FBE_CMS_VAULT_LAYOUT_H */
