/***************************************************************************
* Copyright (C) EMC Corporation 2010-2011
* All rights reserved.
* Licensed material -- property of EMC Corporation
***************************************************************************/

/*!**************************************************************************
* @file fbe_cms_tag_layout.c
***************************************************************************
*
* @brief
*  This file is the top level of accessing the Tags lun, it will interface
*  with the lower level code that does the actual writes
*
***************************************************************************/

/*************************
*   INCLUDE FILES
*************************/
#include "fbe_cms_cluster.h"
#include "fbe_cms_lun_access.h"
#include "fbe_cms_tag_layout.h"


/*this function will persist a single tag in the Tag LUN, it is asynch so the user needs to supply a callback*/
fbe_status_t fbe_cms_tag_lun_persist_single_tag(fbe_cms_cluster_tag_t * p_tag,
												fbe_cms_lun_access_completion completion_func,
												void * context)
{
	
    fbe_lba_t			lba = 0;
	fbe_status_t		status;
    
    /*the buffer index is also the LBA we use on the drive (after the header) so each tag has it's own block*/
	lba = FBE_CMS_TAG_LUN_START_OF_TAG_AREA + p_tag->m_tag_index;

	/*and send it to the layer that will do the actual write with the CRC for each block*/
	status = fbe_cms_lun_access_persist_single_entry(FBE_CMS_TAGS_LUN,
													 lba,
													 sizeof(fbe_cms_cluster_tag_t),
													 (fbe_u8_t *)p_tag,
													 completion_func,
													 context);


    
	return status;
}





