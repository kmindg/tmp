/***************************************************************************
* Copyright (C) EMC Corporation 2010-2011
* All rights reserved.
* Licensed material -- property of EMC Corporation
***************************************************************************/

/***************************************************************************/
/** @file fbe_cms_cluster_free_q.c
***************************************************************************
*
* @brief
*  This file contains data structures and functions to implement Free Q for 
*  cluster tags.
*
* @version
*   2011-10-30 - Created. Vamsi V
* 
***************************************************************************/

/*************************
*   INCLUDE FILES
*************************/
#include "fbe_cms_cluster_free_q.h"

static fbe_cms_cluster_free_q_t g_free_q;

fbe_status_t fbe_cms_cluster_free_q_init() 
{
    fbe_queue_init(&g_free_q.m_head);
    return FBE_STATUS_OK;
}

fbe_status_t fbe_cms_cluster_free_q_destroy() 
{
    fbe_queue_destroy(&g_free_q.m_head);
    return FBE_STATUS_OK;
}

void fbe_cms_cluster_free_q_push(fbe_cms_cluster_tag_t * tag) 
{
    /* LIFO */
    fbe_queue_push_front(&g_free_q.m_head, &tag->m_free_q_element);
}

fbe_cms_cluster_tag_t * fbe_cms_cluster_free_q_pop() 
{
    /* LIFO */
    fbe_queue_element_t * p_element;
    p_element = fbe_queue_pop(&g_free_q.m_head);
    if(p_element)
    {
        return fbe_cms_tag_get_tag_from_free_q_element(p_element); 
    }
    return NULL;
}
