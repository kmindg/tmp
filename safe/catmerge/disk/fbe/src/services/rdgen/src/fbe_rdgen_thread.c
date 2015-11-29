/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_rdgen_thread.c
 ***************************************************************************
 *
 * @brief
 *  This file contains rdgen thread functions.
 *
 * @version
 *   6/03/2009:  Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_rdgen.h"
#include "fbe_rdgen_private_inlines.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/
static void fbe_rdgen_abort_thread_func(void * context);   
static void fbe_rdgen_scan_thread_func(void * context);
static void fbe_rdgen_object_playback_thread_func(void * context);

void fbe_rdgen_thread_enqueue(fbe_rdgen_thread_t *thread_p,
                              fbe_queue_element_t *queue_element_p)
{
    /* Prevent things from changing while we are adding to the queue and setting event.
     */
    fbe_rdgen_thread_lock(thread_p);
    fbe_queue_push(&thread_p->ts_queue_head, queue_element_p);
    fbe_rendezvous_event_set(&thread_p->event);
    fbe_rdgen_thread_unlock(thread_p);

    return;
}

static void fbe_rdgen_thread_process_queue(fbe_rdgen_thread_t *thread_p)
{
    fbe_queue_element_t *queue_element_p = NULL;
    fbe_rdgen_ts_t *ts_p = NULL;
	fbe_queue_head_t tmp_queue;
    fbe_queue_init(&tmp_queue);

    /* Prevent things from changing while we are pulling
     * all the items off the queue.
     */
    fbe_rdgen_thread_lock(thread_p);

    /* Clear event under lock.  This allows us to coalesce the wait on events.
     */
    fbe_rendezvous_event_clear(&thread_p->event);

    /* Populate temp queue from ts queue.
     */
    while(queue_element_p = fbe_queue_pop(&thread_p->ts_queue_head))
    {
        fbe_queue_push(&tmp_queue, queue_element_p);
    }
    /* Unlock to let other things end up on the queue.
     */
    fbe_rdgen_thread_unlock(thread_p);

    /* Process items from temp queue.
     */
    while(queue_element_p = fbe_queue_pop(&tmp_queue))
    {
        ts_p = fbe_rdgen_ts_thread_queue_element_to_ts_ptr(queue_element_p);
        fbe_rdgen_ts_state(ts_p);
    }
    fbe_queue_destroy(&tmp_queue);
    return;
}

static void fbe_rdgen_thread_func(void * context)
{
    fbe_cpu_affinity_mask_t cpu_affinity_mask;
    fbe_status_t status;

    fbe_rdgen_thread_t *thread_p = context;
    fbe_rdgen_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
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
		fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO, 
					FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
					"%s Affinity 0x%llx\n", __FUNCTION__,
					(unsigned long long)cpu_affinity_mask);


		
    }

    /* When the event is signaled, process the queue. 
     * If we are told to halt, then the thread exits. 
     */
    while(1)    
    {
        status = fbe_rendezvous_event_wait(&thread_p->event, 0);

        if (fbe_rdgen_thread_is_flag_set(thread_p, FBE_RDGEN_THREAD_FLAGS_HALT))
        {
            break;
        }
        else
        {

            /* Process items on the queue.
             */
            fbe_rdgen_thread_process_queue(thread_p);
        }
#ifndef ALAMOSA_WINDOWS_ENV
        // Don't let the thread dominate here.
        csx_p_thr_superyield();
#endif
    }

    fbe_rdgen_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                        FBE_TRACE_MESSAGE_ID_FUNCTION_EXIT,
                        "%s exiting 0x%p\n", __FUNCTION__, thread_p);
    fbe_rdgen_thread_set_flag(thread_p, FBE_RDGEN_THREAD_FLAGS_STOPPED);
    fbe_thread_exit(EMCPAL_STATUS_SUCCESS);
}
fbe_bool_t fbe_rdgen_peer_thread_is_empty(void)
{
    fbe_rdgen_thread_t *thread_p = NULL;
    fbe_bool_t b_is_empty;
    fbe_rdgen_get_peer_thread(&thread_p);
    /* Prevent things from changing while we are checking the queue.
     */
    fbe_rdgen_thread_lock(thread_p);

    /* Check queue.
     */
    b_is_empty = fbe_queue_is_empty(&thread_p->ts_queue_head);
    /* Unlock since we are done.
     */
    fbe_rdgen_thread_unlock(thread_p);

    return b_is_empty;
}
void fbe_rdgen_peer_thread_signal(void)
{
    fbe_rdgen_thread_t *thread_p = NULL;
    fbe_rdgen_get_peer_thread(&thread_p);

    fbe_rdgen_thread_lock(thread_p);
    fbe_rendezvous_event_set(&thread_p->event);
    fbe_rdgen_thread_unlock(thread_p);

    return;
}
void fbe_rdgen_peer_thread_enqueue(fbe_queue_element_t *queue_element_p)
{
    fbe_rdgen_thread_t *thread_p = NULL;
    fbe_rdgen_get_peer_thread(&thread_p);
    /* Prevent things from changing while we are adding to the queue.
     */
    fbe_rdgen_thread_lock(thread_p);

    /* Put it on the queue.
     */
    fbe_queue_push(&thread_p->ts_queue_head, queue_element_p);

    fbe_rendezvous_event_set(&thread_p->event);

    /* Unlock since we are done.
     */
    fbe_rdgen_thread_unlock(thread_p);

    return;
}
static void fbe_rdgen_peer_thread_func(void * context)
{
    EMCPAL_STATUS nt_status;
    fbe_rdgen_thread_t *thread_p = context;
    fbe_rdgen_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                        FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                        "%s entry 0x%x\n", __FUNCTION__, (unsigned int)thread_p);

    /* When the event is signaled, process the queue. 
     * If we are told to halt, then the thread exits. 
     */
    while(1)    
    {
        nt_status = fbe_rendezvous_event_wait(&thread_p->event, 0);

        if (fbe_rdgen_thread_is_flag_set(thread_p, FBE_RDGEN_THREAD_FLAGS_HALT))
        {
            break;
        }
        else
        {
            if (fbe_rdgen_service_is_flag_set(FBE_RDGEN_SERVICE_FLAGS_PEER_INIT))
            {
                fbe_rdgen_peer_thread_process_peer_init();
                fbe_rdgen_service_clear_flag(FBE_RDGEN_SERVICE_FLAGS_PEER_INIT);
            }
            /* Process items on the queue.
             */
            fbe_rdgen_peer_thread_process_queue(thread_p);
        }
    }

    /* Drain the queue before the thread exits.
     */
    fbe_rdgen_lock();
    if (!fbe_queue_is_empty(&thread_p->ts_queue_head))
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, 
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s found items on the queue. 0x%x\n", __FUNCTION__, (unsigned int)thread_p);
    }
    fbe_rdgen_unlock();

    fbe_rdgen_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                        FBE_TRACE_MESSAGE_ID_FUNCTION_EXIT,
                        "%s exiting 0x%p\n", __FUNCTION__, thread_p);
    fbe_rdgen_thread_set_flag(thread_p, FBE_RDGEN_THREAD_FLAGS_STOPPED);
    fbe_thread_exit(EMCPAL_STATUS_SUCCESS);
}

fbe_status_t fbe_rdgen_get_thread_func(fbe_rdgen_thread_type_t thread_type,
                                       fbe_thread_user_root_t *thread_fn_p)
{
    fbe_status_t status = FBE_STATUS_OK;

    switch (thread_type)
    {
        case FBE_RDGEN_THREAD_TYPE_IO:
            *thread_fn_p = fbe_rdgen_thread_func;
            break;
        case FBE_RDGEN_THREAD_TYPE_SCAN_QUEUES:
            *thread_fn_p = fbe_rdgen_scan_thread_func;
            break;
        case FBE_RDGEN_THREAD_TYPE_ABORT_IOS:
            *thread_fn_p = fbe_rdgen_abort_thread_func;
            break;
        case FBE_RDGEN_THREAD_TYPE_OBJECT_PLAYBACK:
            *thread_fn_p = fbe_rdgen_object_playback_thread_func;
            break;
        case FBE_RDGEN_THREAD_TYPE_PEER:
            *thread_fn_p = fbe_rdgen_peer_thread_func;
            break;
        default:
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
    };
    return status;
}
fbe_status_t fbe_rdgen_thread_init(fbe_rdgen_thread_t *thread_p,
                                   const char *name,
                                   fbe_rdgen_thread_type_t thread_type,
                                   fbe_u32_t thread_number)
{
    EMCPAL_STATUS nt_status;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_thread_user_root_t thread_fn_p = NULL;
    /* Init all our members.
     */
    fbe_queue_element_init(&thread_p->queue_element);
    fbe_queue_init(&thread_p->ts_queue_head);
    thread_p->queued_requests = 0;
    fbe_rdgen_thread_init_flags(thread_p);
    thread_p->thread_number = thread_number;

    fbe_spinlock_init(&thread_p->lock);
    fbe_rendezvous_event_init_named(&thread_p->event, "rdgen thread");

    /* Kick off the thread.
     */
    fbe_rdgen_get_thread_func(thread_type, &thread_fn_p);
    nt_status = fbe_thread_init(&thread_p->thread_handle, name, thread_fn_p, thread_p);
    if (nt_status != EMCPAL_STATUS_SUCCESS) 
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR,
                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                    "%s: fbe_thread_init fail\n", __FUNCTION__);
        status = FBE_STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
        fbe_rdgen_thread_set_flag(thread_p, FBE_RDGEN_THREAD_FLAGS_INITIALIZED);
    }
    return status;
}

fbe_status_t fbe_rdgen_thread_destroy(fbe_rdgen_thread_t *thread_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_rdgen_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                        FBE_TRACE_MESSAGE_ID_FUNCTION_EXIT,
                        "%s destroying 0x%p\n", __FUNCTION__, thread_p);

    if (RDGEN_COND(!fbe_rdgen_thread_is_flag_set(thread_p, FBE_RDGEN_THREAD_FLAGS_INITIALIZED)))
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s error destrying thread 0x%p, flag 0x%x not initialized\n", 
                                __FUNCTION__, thread_p, thread_p->flags);
        return status;
    }
    /* Tell the thread to halt and wake it up.
     */
    fbe_rdgen_thread_set_flag(thread_p, FBE_RDGEN_THREAD_FLAGS_HALT);

    fbe_rdgen_thread_lock(thread_p);
    fbe_rendezvous_event_set(&thread_p->event);
    fbe_rdgen_thread_unlock(thread_p);

    /* Wait for the thread to exit and then destroy it.
     */
    fbe_thread_wait(&thread_p->thread_handle);

    if (RDGEN_COND(!fbe_rdgen_thread_is_flag_set(thread_p, FBE_RDGEN_THREAD_FLAGS_STOPPED)))
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: fbe rdgen thread flag 0x%x is not set to FBE_RDGEN_THREAD_FLAGS_STOPPED. \n",
                                __FUNCTION__, thread_p->flags);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_thread_destroy(&thread_p->thread_handle);

    /* Destroy the remaining members.
     */
    fbe_queue_destroy(&thread_p->ts_queue_head);
    fbe_spinlock_destroy(&thread_p->lock);
    fbe_rendezvous_event_destroy(&thread_p->event);
    return status;
}
static void fbe_rdgen_abort_thread_func(void * context)
{
    fbe_rdgen_thread_t *thread_p = context;
    fbe_time_t min_wait_time = FBE_RDGEN_THREAD_MAX_SCAN_SECONDS * FBE_TIME_MILLISECONDS_PER_SECOND;
    fbe_rdgen_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry 0x%x\n", __FUNCTION__, (unsigned int)thread_p);

    /* When the event is signaled, process the queue. 
     * If we are told to halt, then the thread exits. 
     */
    while (1)
    {
        /* PAL APIs expect that timeout is not less than 
         * EMCPAL_MINIMUM_TIMEOUT_MSECS.
         */
        if (min_wait_time < EMCPAL_MINIMUM_TIMEOUT_MSECS)
        {
            min_wait_time = EMCPAL_MINIMUM_TIMEOUT_MSECS;
        }

        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s wait for %d msec\n", __FUNCTION__, (int)min_wait_time);
        fbe_rendezvous_event_wait(&thread_p->event, (fbe_u32_t)min_wait_time);

        if (fbe_rdgen_thread_is_flag_set(thread_p, FBE_RDGEN_THREAD_FLAGS_HALT))
        {
            break;
        }
        else
        {
            /* Time to scan the queue to see if anything needs to get aborted.
             */
            fbe_rdgen_scan_for_abort(&min_wait_time);
        }
    }

    fbe_rdgen_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                            FBE_TRACE_MESSAGE_ID_FUNCTION_EXIT,
                            "%s exiting 0x%p\n", __FUNCTION__, thread_p);
    fbe_rdgen_thread_set_flag(thread_p, FBE_RDGEN_THREAD_FLAGS_STOPPED);
    fbe_thread_exit(EMCPAL_STATUS_SUCCESS);
}


void fbe_rdgen_object_thread_signal(void)
{
    fbe_rdgen_thread_t *thread_p = NULL;
    fbe_rdgen_get_object_thread(&thread_p);

    fbe_rdgen_thread_lock(thread_p);
    fbe_rendezvous_event_set(&thread_p->event);
    fbe_rdgen_thread_unlock(thread_p);

    return;
}
static void fbe_rdgen_object_playback_thread_func(void * context)
{
    fbe_rdgen_thread_t *thread_p = context;
    fbe_time_t min_msecs_to_wait = 0;
    fbe_rdgen_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry 0x%x\n", __FUNCTION__, (unsigned int)thread_p);

    /* When the event is signaled, process the queue. 
     * If we are told to halt, then the thread exits. 
     */
    while (1)
    {
        if (min_msecs_to_wait != 0){
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                    "%s wait for %d msecs\n", __FUNCTION__, (unsigned int)min_msecs_to_wait);
        }
        fbe_rendezvous_event_wait(&thread_p->event, (fbe_u32_t)min_msecs_to_wait);

        if (fbe_rdgen_thread_is_flag_set(thread_p, FBE_RDGEN_THREAD_FLAGS_HALT))
        {
            break;
        }
        else
        {
            /* Time to check the queue for objects to be serviced.
             */
            fbe_rdgen_playback_handle_objects(thread_p, &min_msecs_to_wait);
            if (min_msecs_to_wait == FBE_U64_MAX) {
                /* Just wait forever.
                 */
                min_msecs_to_wait = 0;
            }
            else if (min_msecs_to_wait < EMCPAL_MINIMUM_TIMEOUT_MSECS){
                /* Smallest wait we will do is 100ms.
                 */
                min_msecs_to_wait = EMCPAL_MINIMUM_TIMEOUT_MSECS;
            }
            else if (min_msecs_to_wait >= CSX_TIMEOUT_MSECS_MAX){
                /* For handling cast above.
                 */
                min_msecs_to_wait = CSX_TIMEOUT_MSECS_MAX;
            }
        }
    }

    fbe_rdgen_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                            FBE_TRACE_MESSAGE_ID_FUNCTION_EXIT,
                            "%s exiting 0x%p\n", __FUNCTION__, thread_p);
    fbe_rdgen_thread_set_flag(thread_p, FBE_RDGEN_THREAD_FLAGS_STOPPED);
    fbe_thread_exit(EMCPAL_STATUS_SUCCESS);
}
void fbe_rdgen_wakeup_abort_thread(void)
{
    fbe_rdgen_service_t *service_p = fbe_get_rdgen_service();
    fbe_rdgen_thread_t *thread_p = &service_p->abort_thread;

    fbe_rdgen_thread_lock(thread_p);
    fbe_rendezvous_event_set(&thread_p->event);
    fbe_rdgen_thread_unlock(thread_p);
    return;
}

static void fbe_rdgen_scan_thread_func(void * context)
{
    EMCPAL_STATUS nt_status;
    EMCPAL_LARGE_INTEGER wait_time;
    fbe_rdgen_thread_t *thread_p = context;
    
    wait_time.QuadPart = FBE_RDGEN_THREAD_MAX_SCAN_SECONDS * FBE_TIME_MILLISECONDS_PER_SECOND;
    fbe_rdgen_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                        FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                        "%s entry 0x%x\n", __FUNCTION__, (unsigned int)thread_p);

    /* When the event is signaled, process the queue. 
     * If we are told to halt, then the thread exits. 
     */
    while(1)    
    {
        nt_status = fbe_rendezvous_event_wait(&thread_p->event, (fbe_u32_t)wait_time.QuadPart);

        if (fbe_rdgen_thread_is_flag_set(thread_p, FBE_RDGEN_THREAD_FLAGS_HALT))
        {
            break;
        }
        else
        {
            if (fbe_rdgen_is_time_to_scan_queues())
            {
                /* Time to scan the queue to see if anything is stuck.
                 */
                fbe_rdgen_scan_queues();
            }
        }
    }

    fbe_rdgen_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                        FBE_TRACE_MESSAGE_ID_FUNCTION_EXIT,
                        "%s exiting 0x%p\n", __FUNCTION__, thread_p);
    fbe_rdgen_thread_set_flag(thread_p, FBE_RDGEN_THREAD_FLAGS_STOPPED);
    fbe_thread_exit(EMCPAL_STATUS_SUCCESS);
}

void
rdgen_queue_perform_callback(fbe_queue_element_t *queue_element_p)
{
    fbe_rdgen_ts_t *ts_p = NULL;
    
    ts_p = fbe_rdgen_ts_thread_queue_element_to_ts_ptr(queue_element_p);
    fbe_rdgen_ts_state(ts_p);
}

/*************************
 * end file fbe_rdgen_thread.c
 *************************/






