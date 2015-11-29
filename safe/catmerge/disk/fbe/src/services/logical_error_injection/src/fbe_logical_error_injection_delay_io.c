/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_logical_error_injection_delay_io.c
 ***************************************************************************
 *
 * @brief
 *  This file contains functions for delaying IO
 *
 * @version
 *  3/28/2012 - Created. Deanna Heng
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe_logical_error_injection_private.h"
#include "fbe_transport_memory.h"
#include "fbe_logical_error_injection_proto.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/
 
static void fbe_logical_error_injection_delay_thread_release_packet(fbe_packet_t *packet_p);

/*!**************************************************************
 * fbe_logical_error_injection_delay_thread_init()
 ****************************************************************
 * @brief
 *  Initialize the thread in the logical error injection service
 *
 * @param thread_p - thread to initialize        
 *
 * @return fbe_status_t
 *
 * @author
 *  2/13/2012 - Created. Deanna Heng
 * 
 ****************************************************************/
fbe_status_t fbe_logical_error_injection_delay_thread_init(fbe_logical_error_injection_thread_t *thread_p) 
{
    EMCPAL_STATUS nt_status;
    fbe_status_t status = FBE_STATUS_OK;
    /* Init all our members.
     */
    fbe_queue_element_init(&thread_p->queue_element);
    fbe_queue_init(&thread_p->packet_queue_head);

    fbe_spinlock_init(&thread_p->lock);
    /* Init the semaphore so it can have an large number of things enqueued to it.
     */
    fbe_rendezvous_event_init(&thread_p->event);
    thread_p->stop = FBE_FALSE;

    /* Kick off the thread.
     */
    nt_status = fbe_thread_init(&thread_p->thread_handle, "logical error injection delay thread", 
                                fbe_logical_error_injection_delay_thread_func, thread_p);
    if (nt_status != EMCPAL_STATUS_SUCCESS) 
    {
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR,
                                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                                  "%s: logical error injection thread init fail\n", __FUNCTION__);
        status = FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    return status;
}
/*************************
 * end fbe_logical_error_injection_delay_thread_init()
 *************************/

/*!**************************************************************
 * fbe_logical_error_injection_ts_is_expired()
 ****************************************************************
 * @brief
 *  Determine if the ts needs to be completed.
 *
 * @param ts_p - ts to check the time of.               
 *
 * @return FBE_TRUE if it is expired.
 *
 * @author
 *  1/3/2013 - Created. Rob Foley
 *
 ****************************************************************/

fbe_bool_t fbe_logical_error_injection_ts_is_expired(fbe_logical_error_injection_packet_ts_t *ts_p)
{
    fbe_logical_error_injection_object_t *object_p = NULL;
    fbe_bool_t b_enabled;

    fbe_logical_error_injection_get_enable(&b_enabled);
    if (!b_enabled)
    {
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_INFO,
                                                  FBE_TRACE_MESSAGE_ID_INFO,
                                                  "delay thread injection disabled globally object: 0x%x release packet %p\n", 
                                                  ts_p->object_id, ts_p->packet_p);
        return FBE_TRUE;
    }

    object_p = fbe_logical_error_injection_find_object_no_lock(ts_p->object_id);
    if (object_p == NULL)
    {
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_INFO,
                                                  FBE_TRACE_MESSAGE_ID_INFO,
                                                  "delay thread object: 0x%x not found release packet %p\n", 
                                                  ts_p->object_id, ts_p->packet_p);
        return FBE_TRUE;
    }
    
    fbe_logical_error_injection_object_lock(object_p);
    if (!fbe_logical_error_injection_object_is_enabled(object_p))
    {
        fbe_logical_error_injection_object_unlock(object_p);

        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_INFO,
                                                  FBE_TRACE_MESSAGE_ID_INFO,
                                                  "delay thread injection disabled on object: 0x%x release packet %p\n", 
                                                  ts_p->object_id, ts_p->packet_p);
        return FBE_TRUE;
    }

    fbe_logical_error_injection_object_unlock(object_p);

    if (fbe_get_time() >= ts_p->expiration_time)
    {
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_INFO,
                                                  FBE_TRACE_MESSAGE_ID_INFO,
                                                  "delay expired on object: 0x%x release packet %p\n", 
                                                  ts_p->object_id, ts_p->packet_p);
        return FBE_TRUE;
    }

    return FBE_FALSE;
}
/******************************************
 * end fbe_logical_error_injection_ts_is_expired()
 ******************************************/

/*!**************************************************************
 * fbe_logical_error_injection_scan_delay_thread()
 ****************************************************************
 * @brief
 *  Try to find an element that has expired or been cancelled.
 *
 * @param thread_p -
 * @param wait_msecs_p - The minimum number of seconds to wait amongst
 *                       all the queued items.  Or
 *                       FBE_LOGICAL_ERROR_INJECTION_MAX_SCAN_MS if the
 *                       time to wait is very long and there is something on the queue.
 *
 * @return fbe_status_t  
 *
 * @author
 *  1/3/2013 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_logical_error_injection_scan_delay_thread(fbe_logical_error_injection_thread_t *thread_p,
                                                           fbe_u64_t *wait_msecs_p)
{
    fbe_status_t status;
    fbe_packet_t *packet_p = NULL;
    fbe_queue_element_t *queue_element_p = NULL;
    fbe_queue_element_t *next_queue_element_p = NULL;
    fbe_logical_error_injection_packet_ts_t *ts_p = NULL;
    fbe_queue_head_t dispatch_queue;
    fbe_time_t min_expiration_time = FBE_U64_MAX;
    fbe_time_t current_time = fbe_get_time();
    fbe_payload_ex_t *payload_p = NULL;
    fbe_payload_dmrb_operation_t *dmrb_operation_p = NULL;

    fbe_queue_init(&dispatch_queue);

    /* Scan the queue. 
     * For each expired item, put it on our temp queue. 
     */
    fbe_logical_error_injection_lock();
    fbe_rendezvous_event_clear(&thread_p->event);

    queue_element_p = fbe_queue_front(&thread_p->packet_queue_head);

    while (queue_element_p != NULL)
    {
        next_queue_element_p = fbe_queue_next(&thread_p->packet_queue_head, queue_element_p);
        packet_p = fbe_transport_queue_element_to_packet(queue_element_p);
        payload_p = fbe_transport_get_payload_ex(packet_p);
        dmrb_operation_p = fbe_payload_ex_get_dmrb_operation(payload_p);
        ts_p = (fbe_logical_error_injection_packet_ts_t *)&dmrb_operation_p->opaque[0];

        if (fbe_logical_error_injection_ts_is_expired(ts_p) ||
            fbe_transport_is_packet_cancelled(packet_p))
        {
            status = fbe_transport_remove_packet_from_queue(ts_p->packet_p);
            if (status != FBE_STATUS_OK)
            {
                fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                                          "LEI: Unable to remove packet %p from delay queue status: %d\n", 
                                                          packet_p, status);
            }
            else
            {
                if (fbe_transport_is_packet_cancelled(packet_p))
                {
                    fbe_transport_set_status(packet_p, FBE_STATUS_CANCELED, __LINE__);
                }
                fbe_queue_push(&dispatch_queue, &packet_p->queue_element);
            }
        }
        else
        {
            min_expiration_time = FBE_MIN(min_expiration_time, ts_p->expiration_time);
        }

        queue_element_p = next_queue_element_p;
    }
    fbe_logical_error_injection_unlock();

    /* drain the temp queue and release all packets.
     */
    while (queue_element_p = fbe_queue_pop(&dispatch_queue))
    {
        packet_p = fbe_transport_queue_element_to_packet(queue_element_p);
        fbe_logical_error_injection_delay_thread_release_packet(packet_p);
    }

    if (min_expiration_time == FBE_U64_MAX)
    {
        /* Nothing on the queue, just wait max number of MS, since there is nothing to do. 
         */ 
        *wait_msecs_p = FBE_LOGICAL_ERROR_INJECTION_MAX_SCAN_MS;
    }
    else if (min_expiration_time > current_time)
    {
        /* Time is in the future, so do the delta between future and current times.
         */
        *wait_msecs_p = min_expiration_time - current_time;
    }
    else
    {
        /* We will process the queue one more time before waiting.
         */
        *wait_msecs_p = 0;
    }
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_logical_error_injection_scan_delay_thread()
 ******************************************/
/*!**************************************************************
 * fbe_logical_error_injection_delay_thread_func()
 ****************************************************************
 * @brief
 *  The function to run for the delay thread
 *
 * @param context - pointer to delay thread
 *
 * @return 
 *
 * @author
 *  2/13/2012 - Created. Deanna Heng
 * 
 ****************************************************************/
void fbe_logical_error_injection_delay_thread_func(void * context) 
{
    fbe_logical_error_injection_thread_t *thread_p = context;
    fbe_queue_element_t *queue_element_p = NULL;
    fbe_time_t min_wait_msec = FBE_LOGICAL_ERROR_INJECTION_MAX_SCAN_SECONDS * FBE_TIME_MILLISECONDS_PER_SECOND;
    fbe_packet_t *packet_p = NULL;

    while (1) 
    {
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_INFO,
                                                  FBE_TRACE_MESSAGE_ID_INFO,
                                                  "%s delay thread wait %d millisecs\n", __FUNCTION__, (int)min_wait_msec);

        if (min_wait_msec < EMCPAL_MINIMUM_TIMEOUT_MSECS)
        {
            min_wait_msec = EMCPAL_MINIMUM_TIMEOUT_MSECS;
        }
        /* Wait until the delay thread is signalled
         */
        fbe_rendezvous_event_wait(&thread_p->event, (fbe_u32_t)min_wait_msec);

        if (FBE_IS_TRUE(thread_p->stop)) 
        {
            break;
        }
        fbe_logical_error_injection_scan_delay_thread(thread_p, &min_wait_msec);
    }

    /* Make sure we drain the queue and release all packets before exiting */
    while (!fbe_queue_is_empty(&thread_p->packet_queue_head)) 
    {
        queue_element_p = fbe_queue_pop(&thread_p->packet_queue_head);
        packet_p = fbe_transport_queue_element_to_packet(queue_element_p);
        fbe_logical_error_injection_delay_thread_release_packet(packet_p);
    }

    fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                                              FBE_TRACE_MESSAGE_ID_FUNCTION_EXIT,
                                              "%s: Exiting Delay Thread function 0x%x\n", __FUNCTION__, (unsigned int)thread_p);
    fbe_thread_exit(EMCPAL_STATUS_SUCCESS);
}
/*************************
 * end fbe_logical_error_injection_delay_thread_func()
 *************************/

/*!**************************************************************
 * fbe_logical_error_injection_delay_thread_release_packet()
 ****************************************************************
 * @brief
 *  The function to run for the delay thread
 *
 * @param packet_p - pointer to first element in queue
 *
 * @return 
 *
 * @author
 *  3/28/2012 - Created. Deanna Heng
 * 
 ****************************************************************/
static void fbe_logical_error_injection_delay_thread_release_packet(fbe_packet_t *packet_p) 
{
    fbe_payload_ex_t *payload_p = NULL;
    fbe_payload_dmrb_operation_t *dmrb_operation_p = NULL;

    payload_p = fbe_transport_get_payload_ex(packet_p);
    dmrb_operation_p = fbe_payload_ex_get_dmrb_operation(payload_p);

    /* release packet either up or down.
     * First release the memory for our ts. 
     */
    fbe_payload_ex_release_dmrb_operation(payload_p, dmrb_operation_p);

    /* The packet was traveling down the stack. The packet status will be invalid
     * until the packet starts completing 
     */
    if (packet_p->packet_status.code == FBE_STATUS_INVALID) 
    {
        fbe_transport_save_resource_priority(packet_p);
        fbe_transport_restore_resource_priority(packet_p);
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_INFO, 
                                                   FBE_TRACE_MESSAGE_ID_INFO,
                                                   "%s: Releasing packet down 0x%x\n", __FUNCTION__, (unsigned int)packet_p);
        fbe_topology_send_io_packet(packet_p);

    }
    else 
    {
         /* The packet was in the process of completing */
         fbe_transport_complete_packet(packet_p);
         fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_INFO, 
                                                   FBE_TRACE_MESSAGE_ID_INFO,
                                                   "%s: Releasing packet up 0x%p\n", __FUNCTION__, packet_p);
    }
}
/*************************
 * end fbe_logical_error_injection_delay_thread_release_packet()
 *************************/

    
/*!**************************************************************
 * fbe_logical_error_injection_wakeup_delay_thread()
 ****************************************************************
 * @brief
 *  Find the object to work on and then wake up the delay thread to process that object
 *
 * @param none.
 *
 * @return 
 *
 * @author
 *  2/13/2012 - Created. Deanna Heng
 * 
 ****************************************************************/
void fbe_logical_error_injection_wakeup_delay_thread(void)
{
    fbe_logical_error_injection_service_t *lei_service_p = fbe_get_logical_error_injection_service();

    fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_INFO, 
                                              FBE_TRACE_MESSAGE_ID_INFO,
                                              "wakeup delay thread\n");
    fbe_rendezvous_event_set(&lei_service_p->delay_thread.event);
    return;
}
/*************************
 * end fbe_logical_error_injection_wakeup_delay_thread()
 *************************/

/*!**************************************************************
 * fbe_logical_error_injection_delay_cancel_function()
 ****************************************************************
 * @brief
 *  Handle what happens when this packet is cancelled.
 *
 * @param context - This is the lei ts ptr.           
 *
 * @return fbe_status_t   
 *
 * @author
 *  1/3/2013 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t 
fbe_logical_error_injection_delay_cancel_function(fbe_packet_completion_context_t context)
{
    fbe_logical_error_injection_packet_ts_t *ts_p = (fbe_logical_error_injection_packet_ts_t *)context;
    fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_INFO, ts_p->object_id,
                                              "LEI packet cancel function packet: %p waited %d ms\n", context,
                                              fbe_get_elapsed_milliseconds(ts_p->arrival_time));
    fbe_logical_error_injection_wakeup_delay_thread();
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_logical_error_injection_delay_cancel_function()
 ******************************************/
/*!**************************************************************
 * fbe_logical_error_injection_add_packet_to_delay_queue()
 ****************************************************************
 * @brief
 *   Add the packet to the lei delay queue
 *
 * @param  packet_p - Packet to inject on.               
 *
 * @return fbe_status_t   
 *
 * @author
 *  2/24/2012 - Created. Deanna Heng
 *
 ****************************************************************/
fbe_status_t fbe_logical_error_injection_add_packet_to_delay_queue(fbe_packet_t* packet_p, 
                                                                   fbe_object_id_t object_id,
                                                                   fbe_logical_error_injection_record_t *rec_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_logical_error_injection_service_t *lei_service_p = fbe_get_logical_error_injection_service();
    fbe_logical_error_injection_packet_ts_t *ts_p = NULL;
    fbe_payload_ex_t *payload_p = NULL;
    fbe_payload_dmrb_operation_t *dmrb_operation_p = NULL;
    fbe_u32_t wait_ms;

    FBE_ASSERT_AT_COMPILE_TIME(sizeof(fbe_logical_error_injection_packet_ts_t) < sizeof(fbe_payload_dmrb_operation_t));

    payload_p = fbe_transport_get_payload_ex(packet_p);
    dmrb_operation_p = fbe_payload_ex_allocate_dmrb_operation(payload_p);

    if (dmrb_operation_p == NULL)
    {
        fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                                  "%s: Unable to allocate memory\n", __FUNCTION__ );
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Mark dmrb as active playload.
     */
    fbe_payload_ex_increment_dmrb_operation_level(payload_p);

    /* Prevent things from changing while we are adding to the queue.
     */
    fbe_logical_error_injection_thread_lock(&lei_service_p->delay_thread);

    ts_p = (fbe_logical_error_injection_packet_ts_t *)dmrb_operation_p->opaque;

    ts_p->packet_p = packet_p;
    ts_p->object_id = object_id;
    ts_p->arrival_time = fbe_get_time();

    wait_ms = FBE_MIN(rec_p->err_limit, FBE_LOGICAL_ERROR_INJECTION_MAX_DELAY_SECONDS * FBE_TIME_MILLISECONDS_PER_SECOND); 

    fbe_logical_error_injection_service_trace(FBE_TRACE_LEVEL_INFO, object_id,
                                              "LEI: Push packet: %p onto the delay queue for %d milliseconds\n", 
                                              packet_p, wait_ms);
    ts_p->expiration_time = ts_p->arrival_time + wait_ms;

    fbe_transport_set_cancel_function(packet_p, 
                                      fbe_logical_error_injection_delay_cancel_function, 
                                      ts_p);
    /* Put it on the queue, and wake up the thread to look at it.
     */
    fbe_transport_enqueue_packet(packet_p, &lei_service_p->delay_thread.packet_queue_head);

    fbe_rendezvous_event_set(&lei_service_p->delay_thread.event);

    /* Unlock since we are done.
     */
    fbe_logical_error_injection_thread_unlock(&lei_service_p->delay_thread);

    return status;
}
/******************************************
 * end fbe_logical_error_injection_add_packet_to_delay_queue()
 ******************************************/

/*!**************************************************************
 * fbe_logical_error_injection_service_destroy_threads()
 ****************************************************************
 * @brief
 *  Destroy the threads belonging to the logical error injection service
 *
 * @param 
 *
 * @return 
 *
 * @author
 *  2/13/2012 - Created. Deanna Heng
 * 
 ****************************************************************/
void fbe_logical_error_injection_service_destroy_threads(void) 
{
    fbe_logical_error_injection_service_t *lei_service_p = fbe_get_logical_error_injection_service();
    fbe_logical_error_injection_thread_t * delay_thread = &lei_service_p->delay_thread; 

    /* signal the thread to stop and drain the packet queue */
    delay_thread->stop = FBE_TRUE;
    fbe_rendezvous_event_set(&delay_thread->event);

    /* Wait for the thread to exit and then destroy it.
     */
    fbe_thread_wait(&delay_thread->thread_handle);

    fbe_thread_destroy(&delay_thread->thread_handle);

    /* Destroy the remaining members.
     */
    fbe_queue_destroy(&delay_thread->packet_queue_head);
    fbe_spinlock_destroy(&delay_thread->lock);
    fbe_rendezvous_event_destroy(&delay_thread->event);
}
/*************************
 * end fbe_logical_error_injection_service_destroy_threads()
 *************************/


/*************************
 * end file fbe_logical_error_injection_delay_io.c
 *************************/
