#ifndef FBE_CMS_TAG_LAYOUT_H
#define FBE_CMS_TAG_LAYOUT_H

/***************************************************************************/
/** @file fbe_tag_layout.h
***************************************************************************
*
* @brief
*  This file contains definitions for accessing the TAG lun
***************************************************************************/

/*************************
*   INCLUDE FILES
*************************/
#include "fbe_cms_cluster_tag.h"
#include "fbe_cms_lun_access.h"

#define FBE_CMS_TAG_LUN_HEADER_BLOCKS 				20 /*the number of blocks we reserve for the header*/
#define FBE_CMS_TAG_LUN_HEADER_BLOCK_LBA_START		0
#define FBE_CMS_TAG_LUN_BLOCKS_PER_TAG				1 /*how many blocks we use to hold the data per tag*/
#define FBE_CMS_TAG_LUN_START_OF_TAG_AREA			(FBE_CMS_TAG_LUN_HEADER_BLOCK_LBA_START + FBE_CMS_TAG_LUN_HEADER_BLOCKS)


fbe_status_t fbe_cms_tag_lun_persist_single_tag(fbe_cms_cluster_tag_t * p_tag,
												fbe_cms_lun_access_completion completion_func,
												void * context);

fbe_status_t fbe_cms_vault_persist_all_tags(void);



#endif /* FBE_CMS_TAG_LAYOUT_H */

