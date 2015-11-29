/***************************************************************************
* Copyright (C) EMC Corporation 2010-2011
* All rights reserved.
* Licensed material -- property of EMC Corporation
***************************************************************************/

/***************************************************************************/
/** @file fbe_cms_cluster.c
***************************************************************************
*
* @brief
*  This file contains data structures and functions for managing clustering of
*  buffers across both SPs.
*
* @version
*   2011-10-30 - Created. Vamsi V
***************************************************************************/

/*************************
*   INCLUDE FILES
*************************/
#include "fbe_cms_cluster.h"
#include "fbe_cms_cluster_ht.h"
#include "fbe_cms_cluster_free_q.h"

static fbe_cms_cluster_t g_cluster;

fbe_status_t fbe_cms_cluster_init()
{
	fbe_cms_cluster_ht_init ();
    fbe_cms_cluster_free_q_init ();

    return FBE_STATUS_OK;
}

fbe_status_t fbe_cms_cluster_destroy()
{
    fbe_cms_cluster_ht_destroy ();
    fbe_cms_cluster_free_q_destroy ();
    return FBE_STATUS_OK;
}

fbe_status_t fbe_cms_cluster_add_tag(fbe_cms_cluster_tag_t * tag_p)
{
    if(fbe_cms_tag_is_buffer_valid(tag_p))
    {
        fbe_cms_cluster_ht_insert(tag_p);
    }
    else
    {
        fbe_cms_cluster_free_q_push(tag_p);
    }
    return FBE_STATUS_OK;
}

fbe_bool_t fbe_cms_cluster_allocate_buffers(fbe_cms_buff_tracker_t * tracker_p,
											fbe_cms_buff_op_comp_func_t next_state)
{
    fbe_u32_t i = 0;
    fbe_cms_cluster_tag_t * tag_p = NULL;

    for(i = 0; i < tracker_p->m_free_buff_cnt; i++)
    {
        if(tag_p = fbe_cms_cluster_free_q_pop())
        {
			fbe_cms_buff_tracker_insert_buffer(tracker_p, tag_p);
        }
        else
        {
            /* Push to wait queue */
            return FBE_FALSE;
        }
    }
    return FBE_TRUE;
}
