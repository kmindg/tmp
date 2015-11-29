/***************************************************************************
* Copyright (C) EMC Corporation 2010-2011
* All rights reserved.
* Licensed material -- property of EMC Corporation
***************************************************************************/

/***************************************************************************/
/** @file fbe_cms_buff_tracker_pool.c
***************************************************************************
*
* @brief
*  This file implements functionality to maintain a pool of buffer trackers
*  and service alloc/free/pend requests on buffer trackers.
*
* @version
*   2011-11-30 - Created. Vamsi V
***************************************************************************/

/*************************
*   INCLUDE FILES
*************************/
#include "fbe_cms_main.h"
#include "fbe_cms_defines.h"
#include "fbe_cms_private.h"
#include "fbe_cms_buff_tracker_pool.h"

static fbe_cms_buff_tracker_mem_t g_buff_tracker_mem;
static fbe_cms_buff_tracker_q_t g_buff_tracker_free_q;
static fbe_cms_buff_tracker_q_t g_buff_tracker_active_q;
static fbe_cms_buff_tracker_q_t g_buff_tracker_wait_q;

fbe_status_t fbe_cms_buff_tracker_pool_init() 
{
    if ((sizeof(fbe_cms_buff_tracker_t) * FBE_CMS_BUFF_TRACKER_CNT) > FBE_CMS_CMM_MEMORY_BLOCKING_SIZE)
    {   
        /* TODO: Need to implement Catalog */
        cms_trace(FBE_TRACE_LEVEL_ERROR, "%s: Buff Tracker Pool size 0x%X bigger than CMM blocking size 0x%X\n", 
                  __FUNCTION__,
                  (sizeof(fbe_cms_buff_tracker_t) * FBE_CMS_BUFF_TRACKER_CNT),
                  FBE_CMS_CMM_MEMORY_BLOCKING_SIZE
                  );
        return FBE_STATUS_GENERIC_FAILURE; 
    }

    g_buff_tracker_mem.mp_mem = fbe_cms_cmm_memory_allocate(sizeof(fbe_cms_buff_tracker_t) * FBE_CMS_BUFF_TRACKER_CNT);

    if(!g_buff_tracker_mem.mp_mem)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_cms_buff_tracker_pool_setup(&g_buff_tracker_mem);

    return FBE_STATUS_OK;
}

fbe_status_t fbe_cms_buff_tracker_pool_destroy() 
{
	fbe_cms_buff_tracker_free_q_destroy();
	fbe_cms_buff_tracker_active_q_destroy();
	fbe_cms_buff_tracker_wait_q_destroy();
	fbe_cms_cmm_memory_release(g_buff_tracker_mem.mp_mem);

    return FBE_STATUS_OK;
}

void fbe_cms_buff_tracker_pool_setup(fbe_cms_buff_tracker_mem_t * tracker_mem)
{
    ULONG idx;
    fbe_cms_buff_tracker_t * tracker_p = tracker_mem->mp_mem; 

	fbe_cms_buff_tracker_free_q_init();
	fbe_cms_buff_tracker_active_q_init();
	fbe_cms_buff_tracker_wait_q_init();

    /* Loop through all trackers within a cmm block */
    for (idx = 0; idx < FBE_CMS_BUFF_TRACKER_CNT; idx++)
    {
        /* Initialize buff tracker */
		fbe_cms_buff_tracker_init(tracker_p, NULL);
        fbe_cms_buff_tracker_free_q_push(tracker_p);
        tracker_p++;
    }
}

fbe_status_t fbe_cms_buff_tracker_free_q_init() 
{
    fbe_queue_init(&g_buff_tracker_free_q.m_dll_head);
    return FBE_STATUS_OK;
}

fbe_status_t fbe_cms_buff_tracker_free_q_destroy() 
{
    fbe_queue_destroy(&g_buff_tracker_free_q.m_dll_head);
    return FBE_STATUS_OK;
}

void fbe_cms_buff_tracker_free_q_push(fbe_cms_buff_tracker_t * tracker_p) 
{
    /* LIFO */
    fbe_queue_push(&g_buff_tracker_free_q.m_dll_head, &tracker_p->m_next);
}

fbe_cms_buff_tracker_t * fbe_cms_buff_tracker_free_q_pop() 
{
    /* LIFO */
    fbe_queue_element_t * p_element;
    p_element = fbe_queue_pop(&g_buff_tracker_free_q.m_dll_head);
    if(p_element)
    {
        return fbe_cms_buff_tracker_from_tracker_q_element(p_element); 
    }
    return NULL;
}

fbe_status_t fbe_cms_buff_tracker_active_q_init() 
{
    fbe_queue_init(&g_buff_tracker_active_q.m_dll_head);
    return FBE_STATUS_OK;
}

fbe_status_t fbe_cms_buff_tracker_active_q_destroy() 
{
    fbe_queue_destroy(&g_buff_tracker_active_q.m_dll_head);
    return FBE_STATUS_OK;
}

void fbe_cms_buff_tracker_active_q_push(fbe_cms_buff_tracker_t * tracker_p) 
{
    /* DLL */
    fbe_queue_push(&g_buff_tracker_active_q.m_dll_head, &tracker_p->m_next);
}

fbe_cms_buff_tracker_t * fbe_cms_buff_tracker_active_q_pop() 
{
    /* DLL */
    fbe_queue_element_t * p_element;
    p_element = fbe_queue_pop(&g_buff_tracker_active_q.m_dll_head);
    if(p_element)
    {
        return fbe_cms_buff_tracker_from_tracker_q_element(p_element); 
    }
    return NULL;
}

void fbe_cms_buff_tracker_active_q_remove(fbe_cms_buff_tracker_t * tracker_p) 
{
    fbe_queue_remove(&tracker_p->m_next);
}

fbe_status_t fbe_cms_buff_tracker_wait_q_init() 
{
    fbe_queue_init(&g_buff_tracker_wait_q.m_dll_head);
    return FBE_STATUS_OK;
}

fbe_status_t fbe_cms_buff_tracker_wait_q_destroy() 
{
    fbe_queue_destroy(&g_buff_tracker_wait_q.m_dll_head);
    return FBE_STATUS_OK;
}

void fbe_cms_buff_tracker_wait_q_push(fbe_cms_buff_req_t * req_p) 
{
    /* DLL */
    fbe_queue_push(&g_buff_tracker_wait_q.m_dll_head, &req_p->m_next);
}

fbe_cms_buff_req_t * fbe_cms_buff_tracker_wait_q_pop() 
{
    /* DLL */
    fbe_queue_element_t * p_element;
    p_element = fbe_queue_pop(&g_buff_tracker_wait_q.m_dll_head);
    if(p_element)
    {
        return fbe_cms_buff_req_from_q_element(p_element); 
    }
    return NULL;
}

void fbe_cms_buff_tracker_wait_q_remove(fbe_cms_buff_req_t * req_p) 
{
    fbe_queue_remove(&req_p->m_next);
}

fbe_cms_buff_req_t * fbe_cms_buff_req_from_q_element(fbe_queue_element_t * p_element)
{
    /* Its a direct map as sll element is first field in buff_tracker struct  */
    return (fbe_cms_buff_req_t *)p_element;
}

fbe_cms_buff_tracker_t * fbe_cms_buff_tracker_allocate(fbe_cms_buff_req_t * req_p)
{
	fbe_cms_buff_tracker_t * tracker_p = NULL;

	if(tracker_p = fbe_cms_buff_tracker_free_q_pop())
	{
		fbe_cms_buff_tracker_init(tracker_p, req_p);

		/* TODO: Set cancel routine  */

		/* Add to active queue */
		fbe_cms_buff_tracker_active_q_push(tracker_p);   	
	}
	else
	{
		fbe_cms_buff_tracker_wait_q_push(req_p);
	}
    
    return tracker_p;
}
