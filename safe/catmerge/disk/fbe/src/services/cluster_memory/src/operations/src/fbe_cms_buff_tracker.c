/***************************************************************************
* Copyright (C) EMC Corporation 2010-2011
* All rights reserved.
* Licensed material -- property of EMC Corporation
***************************************************************************/

/***************************************************************************/
/** @file fbe_cms_buff_tracker.c
***************************************************************************
*
* @brief
*  This file implements Buffer Tracker member functions.
*
* @version
*   2011-11-30 - Created. Vamsi V
***************************************************************************/

/*************************
*   INCLUDE FILES
*************************/
#include "fbe_cms_buff_tracker.h"

fbe_status_t fbe_cms_buff_tracker_init(fbe_cms_buff_tracker_t * tracker_p, fbe_cms_buff_req_t * req_p)
{
    fbe_queue_init(&tracker_p->m_next);
    tracker_p->m_req_p = req_p;
    tracker_p->m_status = FBE_STATUS_INVALID;
    tracker_p->m_free_buff_cnt = 0;
    tracker_p->m_contigous_alloc = FBE_FALSE;
    fbe_queue_init(&tracker_p->m_buff_queue_head);

    return FBE_STATUS_OK;
}

void fbe_cms_buff_tracker_push_buffer(fbe_cms_buff_tracker_t * tracker_p, 
                                      fbe_cms_cluster_tag_t * buff_p)
{
	/* LIFO */
	fbe_queue_push_front(&tracker_p->m_buff_queue_head, &buff_p->m_free_q_element);
}

/* Buffers would be inserted in ascending order of buffer-index */
void fbe_cms_buff_tracker_insert_buffer(fbe_cms_buff_tracker_t * tracker_p, 
                                      fbe_cms_cluster_tag_t * buff_p)
{
	fbe_cms_cluster_tag_t * next_buff_p;
	fbe_queue_element_t * next_element_p = fbe_queue_front(&tracker_p->m_buff_queue_head);


	while (next_element_p != NULL) 
	{
		next_buff_p = fbe_cms_tag_get_tag_from_free_q_element(next_element_p);

		if (buff_p->m_tag_index > next_buff_p->m_tag_index) 
		{
			next_element_p = fbe_queue_next(&tracker_p->m_buff_queue_head, next_element_p);
		}
		else
		{
			fbe_queue_insert(&buff_p->m_free_q_element, next_element_p);
			return;
		}
	}

	/* Add to tail */
	fbe_queue_push(&tracker_p->m_buff_queue_head, &buff_p->m_free_q_element);
}

fbe_cms_cluster_tag_t * fbe_cms_buff_tracker_pop_buffer(fbe_cms_buff_tracker_t * tracker_p)
{
    /* LIFO */
    fbe_queue_element_t * p_element;
    p_element = fbe_queue_pop(&tracker_p->m_buff_queue_head);
    if(p_element)
    {
        return fbe_cms_tag_get_tag_from_free_q_element(p_element); 
    }
    return NULL;
}

fbe_cms_buff_tracker_t * fbe_cms_buff_tracker_from_tracker_q_element(fbe_queue_element_t * p_element)
{
    /* Its a direct map as sll element is first field in buff_tracker struct  */
    return (fbe_cms_buff_tracker_t *)p_element;
}

fbe_cms_buff_tracker_t * fbe_cms_buff_tracker_from_buff_q_element(fbe_queue_element_t * p_element)
{
    return (fbe_cms_buff_tracker_t *)((fbe_u8_t *)p_element - (fbe_u8_t *)(&((fbe_cms_buff_tracker_t *)0)->m_buff_queue_head));
}

fbe_cms_alloc_id_t fbe_cms_buff_tracker_get_alloc_id(fbe_cms_buff_tracker_t * tracker_p)
{
	return tracker_p->m_req_p->m_alloc_id; 
}
