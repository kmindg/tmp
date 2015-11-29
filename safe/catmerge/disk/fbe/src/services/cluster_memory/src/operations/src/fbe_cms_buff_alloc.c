/***************************************************************************
* Copyright (C) EMC Corporation 2010-2011
* All rights reserved.
* Licensed material -- property of EMC Corporation
***************************************************************************/

/***************************************************************************/
/** @file fbe_cms_buff_alloc.c
***************************************************************************
*
* @brief
*  This file implements Alloc buffer operation.
*
* @version
*   2011-11-30 - Created. Vamsi V
***************************************************************************/

/*************************
*   INCLUDE FILES
*************************/
#include "fbe_cms.h"
#include "fbe_cms_buff.h"
#include "fbe_cms_cluster.h"
#include "fbe_cms_cluster_ht.h"
#include "fbe_cms_defines.h"

fbe_status_t fbe_cms_buff_alloc_start(fbe_cms_buff_tracker_t * tracker_p)
{
    /* Setup tracker for cluster to use */
    tracker_p->m_free_buff_cnt = (tracker_p->m_req_p->m_alloc_size / FBE_CMS_BUFFER_SIZE);
    if(tracker_p->m_req_p->m_alloc_size % FBE_CMS_BUFFER_SIZE)
    { 
        tracker_p->m_free_buff_cnt++; 
    }
    if(tracker_p->m_req_p->m_opcode == FBE_CMS_BUFF_CONT_ALLOC)
    {
        tracker_p->m_contigous_alloc = FBE_TRUE;
    }
    /* End: Setup tracker for cluster to use */


    if (fbe_cms_cluster_allocate_buffers(tracker_p, &fbe_cms_buff_alloc_complete))
    {
        /* Completed synchronously  */
        return fbe_cms_buff_alloc_complete(tracker_p);
    }
    else
    {
        //return FBE_STATUS_MORE_PROCESSING_REQUIRED;
		return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }
}

fbe_status_t fbe_cms_buff_alloc_complete(fbe_cms_buff_tracker_t * tracker_p)
{
	fbe_cms_cluster_tag_t * tag_p;

	while(tag_p = fbe_cms_buff_tracker_pop_buffer(tracker_p))
	{
		fbe_cms_tag_alloc_prepare(tag_p, fbe_cms_buff_tracker_get_alloc_id(tracker_p));
		fbe_cms_cluster_ht_insert(tag_p);
	}

	return FBE_STATUS_OK;
}



