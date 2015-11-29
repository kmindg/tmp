#ifndef FBE_CMS_BUFF_TRACKER_POOL_H
#define FBE_CMS_BUFF_TRACKER_POOL_H

/***************************************************************************/
/** @file fbe_cms_buff_tracker_pool.h
***************************************************************************
*
* @brief
*  This file contains data structures and functions to support buffer
*  tracking structure pool. 
*
* @version
*   2011-11-22 - Created. Vamsi V 
***************************************************************************/

/*************************
*   INCLUDE FILES
*************************/
#include "fbe/fbe_types.h"
#include "fbe_cms.h"
#include "fbe/fbe_single_link_list.h"
#include "fbe_cms_buff_tracker.h"

/*************************
*   DATA STRUCTURES
*************************/

typedef struct fbe_cms_buff_tracker_mem_s
{
    void * mp_mem;
}fbe_cms_buff_tracker_mem_t;

/* TODO: Make it per-core in future */
typedef struct fbe_cms_buff_tracker_q_s
{
    fbe_queue_head_t m_dll_head;
}fbe_cms_buff_tracker_q_t;


fbe_status_t fbe_cms_buff_tracker_pool_init();
fbe_status_t fbe_cms_buff_tracker_pool_destroy();
void fbe_cms_buff_tracker_pool_setup(fbe_cms_buff_tracker_mem_t * tracker_mem);
fbe_status_t fbe_cms_buff_tracker_free_q_init();
fbe_status_t fbe_cms_buff_tracker_free_q_destroy();
void fbe_cms_buff_tracker_free_q_push(fbe_cms_buff_tracker_t * tracker_p);
fbe_cms_buff_tracker_t * fbe_cms_buff_tracker_free_q_pop();
fbe_status_t fbe_cms_buff_tracker_active_q_init();
fbe_status_t fbe_cms_buff_tracker_active_q_destroy();
void fbe_cms_buff_tracker_active_q_push(fbe_cms_buff_tracker_t * tracker_p);
fbe_cms_buff_tracker_t * fbe_cms_buff_tracker_active_q_pop();
void fbe_cms_buff_tracker_active_q_remove(fbe_cms_buff_tracker_t * tracker_p);
fbe_status_t fbe_cms_buff_tracker_wait_q_init();
fbe_status_t fbe_cms_buff_tracker_wait_q_destroy();
void fbe_cms_buff_tracker_wait_q_push(fbe_cms_buff_req_t * req_p);
fbe_cms_buff_req_t * fbe_cms_buff_tracker_wait_q_pop();
void fbe_cms_buff_tracker_wait_q_remove(fbe_cms_buff_req_t * req_p);
fbe_cms_buff_req_t * fbe_cms_buff_req_from_q_element(fbe_queue_element_t * p_element);
fbe_cms_buff_tracker_t * fbe_cms_buff_tracker_allocate(fbe_cms_buff_req_t * req_p);

#endif /* FBE_CMS_BUFF_TRACKER_POOL_H */
