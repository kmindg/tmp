/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_raw_mirror_service_thread.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the code for the raw mirror service thread support.
 * 
 * @version
 *   11/2011:  Created. Susan Rundbaken 
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe_raw_mirror_service_private.h"


/*************************
 *   FUNCTION DEFINITIONS
 *************************/

static void fbe_raw_mirror_service_thread_func(void * context);
static void fbe_raw_mirror_service_thread_process_queue(fbe_raw_mirror_service_thread_t *thread_p);


/*!**************************************************************
 *  fbe_raw_mirror_service_init_thread()
 ****************************************************************
 * @brief
 *  Initialize the raw mirror service thread.
 *
 * @param thread_p          - Thread to initialize.
 * @param thread_number     - Thread number in thread pool.
 *
 * @return fbe_status_t -   FBE_STATUS_OK.
 *                          FBE_STATUS_INSUFFICIENT_RESOURCES (could not start thread).
 *
 * @author
 *  11/2011 - Created. Susan Rundbaken
 *
 ****************************************************************/
fbe_status_t fbe_raw_mirror_service_init_thread(fbe_raw_mirror_service_thread_t *thread_p,
                                                fbe_u32_t thread_number)
{
    EMCPAL_STATUS nt_status;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_thread_user_root_t thread_fn_p = fbe_raw_mirror_service_thread_func;


    /* Initialize members.
     */
    fbe_queue_init(&thread_p->ts_queue_head);
    thread_p->flags = 0;
    thread_p->thread_number = thread_number;

    fbe_spinlock_init(&thread_p->lock);

    /* Init the semaphore so it can have an large number of things enqueued to it.
     */
    fbe_semaphore_init(&thread_p->semaphore, 0, 20000);

    /* Start the thread.
     */
    nt_status = fbe_thread_init(&thread_p->thread_handle, "fbe_raw_mirsvc", thread_fn_p, thread_p);
    if (nt_status != EMCPAL_STATUS_SUCCESS) 
    {
        fbe_queue_destroy(&thread_p->ts_queue_head);
        fbe_spinlock_destroy(&thread_p->lock);
        fbe_semaphore_destroy(&thread_p->semaphore);

        fbe_raw_mirror_service_trace(FBE_TRACE_LEVEL_ERROR,
                                     FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                     "%s: fbe_thread_init fail\n", __FUNCTION__);
        status = FBE_STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
        fbe_raw_mirror_service_thread_set_flag(thread_p, FBE_RAW_MIRROR_SERVICE_THREAD_FLAGS_INITIALIZED);
    }

    return status;
}
/******************************************
 * end fbe_raw_mirror_service_init_thread()
 ******************************************/

/*!**************************************************************
 *  fbe_raw_mirror_service_thread_enqueue()
 ****************************************************************
 * @brief
 *  Enqueue the element on the thread queue.
 *
 * @param thread_p - Pointer to the thread. 
 * @param queue_element_p - Pointer to element to queue to thread.         
 *
 * @return fbe_status_t.
 *
 * @author
 *  10/2011 - Created. Susan Rundbaken 
 *
 ****************************************************************/ 
void fbe_raw_mirror_service_thread_enqueue(fbe_raw_mirror_service_thread_t *thread_p,
                                           fbe_queue_element_t *queue_element_p)
{
    fbe_raw_mirror_service_thread_lock(thread_p);

    /* Put element on the queue.
     */
    fbe_queue_push(&thread_p->ts_queue_head, queue_element_p);
    
    fbe_raw_mirror_service_thread_unlock(thread_p);

    /* Wake up thread.
     */
    fbe_semaphore_release(&thread_p->semaphore, 0, 1, FALSE);

    return;
}
/******************************************
 * end fbe_raw_mirror_service_thread_enqueue()
 ******************************************/

/*!**************************************************************
 *  fbe_raw_mirror_service_thread_func()
 ****************************************************************
 * @brief
 *  This is the function associated with the thread. It handles processing 
 *  for the raw mirror thread.
 *
 * @param context - Thread context.
 *
 * @return None.
 *
 * @author
 *  11/2011 - Created from fbe_rdgen_thread_func(). Susan Rundbaken
 *
 ****************************************************************/
static void fbe_raw_mirror_service_thread_func(void * context)
{
    EMCPAL_STATUS nt_status;
	fbe_cpu_affinity_mask_t cpu_affinity_mask;

    fbe_raw_mirror_service_thread_t *thread_p = context;
    fbe_raw_mirror_service_trace(FBE_TRACE_LEVEL_INFO, 
                                 FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                 "%s entry 0x%p\n", __FUNCTION__, thread_p);

    /* Only affine if the thread number was set. 
     * Some threads we want to "float". 
     */
    if (thread_p->thread_number != FBE_U32_MAX)
    {
		cpu_affinity_mask = 0x1;
        fbe_thread_set_affinity(&thread_p->thread_handle, (cpu_affinity_mask << thread_p->thread_number));

		fbe_thread_get_affinity(&cpu_affinity_mask);
		fbe_raw_mirror_service_trace(FBE_TRACE_LEVEL_INFO, 
                                     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                     "%s Affinity 0x%llx\n", __FUNCTION__,
				     (unsigned long long)cpu_affinity_mask);
    }

    /* When the semaphore is signaled, process the queue. 
     * If we are told to halt, then the thread exits. 
     */
    while(1)    
    {
        nt_status = fbe_semaphore_wait(&thread_p->semaphore, NULL);

        if (fbe_raw_mirror_service_thread_is_flag_set(thread_p, FBE_RAW_MIRROR_SERVICE_THREAD_FLAGS_HALT))
        {
            break;
        }
        else
        {
            /* Process items on the queue.
             */
            fbe_raw_mirror_service_thread_process_queue(thread_p);
        }
    }

    fbe_raw_mirror_service_trace(FBE_TRACE_LEVEL_INFO, 
                                 FBE_TRACE_MESSAGE_ID_FUNCTION_EXIT,
                                 "%s exiting 0x%p\n", __FUNCTION__, thread_p);
    fbe_raw_mirror_service_thread_set_flag(thread_p, FBE_RAW_MIRROR_SERVICE_THREAD_FLAGS_STOPPED);

    fbe_thread_exit(EMCPAL_STATUS_SUCCESS);
}
/******************************************
 * end fbe_raw_mirror_service_thread_func()
 ******************************************/

/*!**************************************************************
 *  fbe_raw_mirror_service_thread_process_queue()
 ****************************************************************
 * @brief
 *  Process the thread queue.  The queue is comprised of raw mirror
 *  tracking structures that track I/O requests.
 *
 * @param thread_p - Pointer to thread.
 *
 * @return None.
 *
 * @author
 *  11/2011 - Created. Susan Rundbaken
 *
 ****************************************************************/
static void fbe_raw_mirror_service_thread_process_queue(fbe_raw_mirror_service_thread_t *thread_p)
{
    fbe_queue_element_t *queue_element_p = NULL;
    fbe_raw_mirror_service_ts_t *ts_p = NULL;


    fbe_raw_mirror_service_thread_lock(thread_p);

    /* Get tracking structure from the thread queue.
     */
    queue_element_p = fbe_queue_front(&thread_p->ts_queue_head);
    fbe_queue_remove(queue_element_p);
    ts_p = fbe_raw_mirror_service_ts_thread_queue_element_to_ts_ptr(queue_element_p);

    fbe_raw_mirror_service_thread_unlock(thread_p);

    /* Process the tracking structure.
     */
    fbe_raw_mirror_service_start_io_request_packet(ts_p);

    return;
}
/******************************************
 * end fbe_raw_mirror_service_thread_process_queue()
 ******************************************/

/*!**************************************************************
 *  fbe_raw_mirror_service_destroy_thread()
 ****************************************************************
 * @brief
 *  Destroy the raw mirror service thread.
 *  This will fail if the thread queue is not empty.
 *
 * @param thread_p - Pointer to the thread.          
 *
 * @return fbe_status_t -   FBE_STATUS_OK.
 *                          FBE_STATUS_GENERIC_FAILURE (queue not empty or thread not stopped).
 *
 * @author
 *  11/2011 - Created. Susan Rundbaken 
 *
 ****************************************************************/
fbe_status_t fbe_raw_mirror_service_destroy_thread(fbe_raw_mirror_service_thread_t *thread_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_bool_t queue_empty_b;

    fbe_raw_mirror_service_trace(FBE_TRACE_LEVEL_INFO, 
                                 FBE_TRACE_MESSAGE_ID_FUNCTION_EXIT,
                                 "%s destroying 0x%p\n", __FUNCTION__, thread_p);

    fbe_raw_mirror_service_thread_lock(thread_p);
    if (!fbe_raw_mirror_service_thread_is_flag_set(thread_p, FBE_RAW_MIRROR_SERVICE_THREAD_FLAGS_INITIALIZED))
    {
        fbe_raw_mirror_service_thread_unlock(thread_p);

        fbe_raw_mirror_service_trace(FBE_TRACE_LEVEL_INFO, 
                                     FBE_TRACE_MESSAGE_ID_INFO,
                                     "%s error destroying thread 0x%p, flag 0x%x not initialized\n", 
                                     __FUNCTION__, thread_p, thread_p->flags);
        return status;
    }

    /* Make sure the thread queue is empty.
     */
    queue_empty_b = fbe_queue_is_empty(&thread_p->ts_queue_head);
    if (!queue_empty_b)
    {
        fbe_raw_mirror_service_thread_unlock(thread_p);

        fbe_raw_mirror_service_trace(FBE_TRACE_LEVEL_INFO, 
                                     FBE_TRACE_MESSAGE_ID_INFO,
                                     "%s error destroying thread 0x%p, queue not empty\n", 
                                     __FUNCTION__, thread_p);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Tell the thread to halt and wake it up.
     */
    fbe_raw_mirror_service_thread_set_flag(thread_p, FBE_RAW_MIRROR_SERVICE_THREAD_FLAGS_HALT);
    fbe_raw_mirror_service_thread_unlock(thread_p);

    fbe_semaphore_release(&thread_p->semaphore, 0, 1, FALSE);

    /* Wait for the thread to exit and then destroy it.
     */
    fbe_thread_wait(&thread_p->thread_handle);

    if (!fbe_raw_mirror_service_thread_is_flag_set(thread_p, FBE_RAW_MIRROR_SERVICE_THREAD_FLAGS_STOPPED))
    {
        fbe_raw_mirror_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                     FBE_TRACE_MESSAGE_ID_INFO,
                                     "%s: fbe raw mirror thread flag 0x%x is not set to FBE_RAW_MIRROR_SERVICE_THREAD_FLAGS_STOPPED. \n",
                                     __FUNCTION__, thread_p->flags);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_thread_destroy(&thread_p->thread_handle);

    /* Destroy the remaining members.
     */
    fbe_queue_destroy(&thread_p->ts_queue_head);
    fbe_spinlock_destroy(&thread_p->lock);
    fbe_semaphore_destroy(&thread_p->semaphore);

    return status;
}
/******************************************
 * end fbe_raw_mirror_service_destroy_thread()
 ******************************************/


/*************************
 * end file fbe_raw_mirror_service_thread.c
 *************************/


