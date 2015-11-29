/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved. Licensed material -- property of EMC
 * Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_dest_timer.c
 ***************************************************************************
 *
 * @brief
 *  This file contains routines for handling a timer for delaying IO.  Delays
 *  are implemented by using a timer thread which will wake up periodically and
 *  check if any delay IO has expired.  
 * 
 * @version
 *   01-June-2011:  Created. Wayne Garrett
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_queue.h"
#include "fbe/fbe_winddk.h"
#include "fbe_base.h"
#include "fbe_dest_private.h"
#include "fbe/fbe_memory.h"


/* Idle timer triggers idle queue dispatch. The resolution in milliseconds */
#define FBE_DEST_TIMER_PERIODIC_WAKEUP 200L /* 200 ms.*/

/*This value should be based on max number outstanding IO, # packets. It is a guestimate for now and can be safely increased in the future*/
#define DEST_DELAY_MAX_COUNT 100

typedef enum dest_timer_thread_flag_e{
    DEST_TIMER_THREAD_DONE,
    DEST_TIMER_THREAD_RUN,
    DEST_TIMER_THREAD_STOP,

}dest_timer_thread_flag_t;

static fbe_spinlock_t		dest_timer_lock; 
static fbe_queue_head_t	    dest_timer_idle_queue_head;
static fbe_spinlock_t		dest_timer_free_pool_lock; 
static fbe_queue_head_t	    dest_timer_free_pool_queue_head;

static fbe_thread_t			     dest_timer_idle_queue_thread_handle;
static dest_timer_thread_flag_t  dest_timer_idle_queue_thread_flag;

/* Forward declaration */ 
static void dest_timer_idle_queue_thread_func(void * context);
static void dest_timer_dispatch_idle_queue(fbe_bool_t drain);
fbe_status_t dest_timer_free_pool_alloc(int pool_queue_size);
fbe_status_t dest_timer_free_pool_release(void);

/************************************************************
 *  fbe_dest_timer_init()
 ************************************************************
 *
 * @brief   Initialize timer thread
 *
 * @param   void
 * 
 * @return  status
 * 
 * @author  06/01/2011  Wayne Garrett  - Created
 *  
 ************************************************************/
fbe_status_t 
fbe_dest_timer_init(void)
{
    fbe_status_t status;
    dest_log_debug("%s entry\n", __FUNCTION__);

    /* Initialize dest_timer lock */
    fbe_spinlock_init(&dest_timer_lock);

    /* Initialize dest_timer queues */
    fbe_queue_init(&dest_timer_idle_queue_head);
		
    /* Initialize dest_timer_free_pool lock and queue*/
    fbe_spinlock_init(&dest_timer_free_pool_lock);	
    fbe_queue_init(&dest_timer_free_pool_queue_head);
	
    status = dest_timer_free_pool_alloc(DEST_DELAY_MAX_COUNT);
    if (status != FBE_STATUS_OK) {
        dest_log_error("%s Allocation error %d\n", __FUNCTION__, status);   		
        return FBE_STATUS_GENERIC_FAILURE;
    }
 	
    /* Start idle thread */
    dest_timer_idle_queue_thread_flag = DEST_TIMER_THREAD_RUN;
    fbe_thread_init(&dest_timer_idle_queue_thread_handle, "fbe_dest_timidle", dest_timer_idle_queue_thread_func, NULL);

    return FBE_STATUS_OK;
}
/************************************************************
 *  fbe_dest_timer_init()
 ************************************************************/


/************************************************************
 *  fbe_dest_timer_destroy()
 ************************************************************
 *
 * @brief   Destroy timer thread
 *
 * @param   void
 * 
 * @return  status
 * 
 * @author  06/01/2011  Wayne Garrett  - Created
 *  
 ************************************************************/
fbe_status_t 
fbe_dest_timer_destroy(void)
{
    dest_log_debug("%s entry\n", __FUNCTION__);

    /* Stop idle_queue thread */
    dest_timer_idle_queue_thread_flag = DEST_TIMER_THREAD_STOP;  
    fbe_thread_wait(&dest_timer_idle_queue_thread_handle);
    fbe_thread_destroy(&dest_timer_idle_queue_thread_handle);

    /*release allocated memory */
    dest_timer_free_pool_release();
	
    fbe_queue_destroy(&dest_timer_idle_queue_head);
    fbe_queue_destroy(&dest_timer_free_pool_queue_head);
	
    fbe_spinlock_destroy(&dest_timer_lock);
    fbe_spinlock_destroy(&dest_timer_free_pool_lock);

    return FBE_STATUS_OK;
}
/************************************************************
 *  fbe_dest_timer_destroy()
 ************************************************************/


/************************************************************
 *  dest_timer_idle_queue_thread_func()
 ************************************************************
 *
 * @brief   Main thread function which wakes up periodically
 *          checking if an delays have expired.
 *
 * @param   void
 * 
 * @return  void
 * 
 * @author  06/01/2011  Wayne Garrett  - Created
 *  
 ************************************************************/
static void 
dest_timer_idle_queue_thread_func(void * context)
{

    FBE_UNREFERENCED_PARAMETER(context);

    dest_log_debug("%s entry\n", __FUNCTION__);

    while(1)    
	{
		fbe_thread_delay(FBE_DEST_TIMER_PERIODIC_WAKEUP);
		if(dest_timer_idle_queue_thread_flag == DEST_TIMER_THREAD_RUN) {
			dest_timer_dispatch_idle_queue(FBE_FALSE);
		} else {
			break;
		}
    }

    /* drain remaining queue */
    dest_timer_dispatch_idle_queue(FBE_TRUE);

    dest_log_debug("%s done\n", __FUNCTION__);

    dest_timer_idle_queue_thread_flag = DEST_TIMER_THREAD_DONE;
	fbe_thread_exit(EMCPAL_STATUS_SUCCESS);
}
/************************************************************
 *  dest_timer_idle_queue_thread_func()
 ************************************************************/


/************************************************************
 *  dest_timer_dispatch_idle_queue()
 ************************************************************
 *
 * @brief   Tests if any delayed IO as expired.  If so it will
 *          complete the IO packet.
 *
 * @param   drain - if  true then complete all delayed IOs
 * 
 * @return  void
 * 
 * @author  06/01/2011  Wayne Garrett  - Created
 *  
 ************************************************************/
static void 
dest_timer_dispatch_idle_queue(fbe_bool_t drain)
{
	fbe_dest_timer_t * dest_timer = NULL;
	fbe_dest_timer_t * next_dest_timer = NULL;
        fbe_time_t current_time = fbe_get_time();

	fbe_spinlock_lock(&dest_timer_lock);

	if(fbe_queue_is_empty(&dest_timer_idle_queue_head)){
		fbe_spinlock_unlock(&dest_timer_lock);
		return;
	}

	/* Loop over all elements and push the to run_queue if needed */
	next_dest_timer = (fbe_dest_timer_t *)fbe_queue_front(&dest_timer_idle_queue_head);
	while(next_dest_timer != NULL) {
		dest_timer = next_dest_timer;
		next_dest_timer = (fbe_dest_timer_t *)fbe_queue_next(&dest_timer_idle_queue_head, &next_dest_timer->queue_element);

		if(drain || dest_timer->expiration_time <= current_time){
			/* We are removing the element from the dest_timer_idle_queue_head since this task is now completed.*/
			fbe_queue_remove(&dest_timer->queue_element);
			fbe_spinlock_unlock(&dest_timer_lock);
			fbe_spinlock_lock(&dest_timer_free_pool_lock);
			/* We now return the element for the completed task to the free pool to make it available for another task later.*/
			fbe_queue_push(&dest_timer_free_pool_queue_head, &dest_timer->queue_element); 
			fbe_spinlock_unlock(&dest_timer_free_pool_lock);
			fbe_transport_complete_packet(dest_timer->packet);
			fbe_spinlock_lock(&dest_timer_lock);
			/* We have to start from the head */
			next_dest_timer = (fbe_dest_timer_t *)fbe_queue_front(&dest_timer_idle_queue_head);
		}
	} /* while(next_dest_timer != NULL) */

	fbe_spinlock_unlock(&dest_timer_lock);
}
/************************************************************
 *  dest_timer_dispatch_idle_queue()
 ************************************************************/

/************************************************************
 *  fbe_dest_set_timer()
 ************************************************************
 *
 * @brief   Set the expiration time for the delayed IO
 *
 * @param   timeout_msec - timeout in milliseconds
 * @param   completion_function - function to be added to stack,
 * @param   which will be called after delay expires
 * @param   completion_context - context for completion_function
 * 
 * @return  status
 * 
 * @author  06/01/2011  Wayne Garrett  - Created
 *  
 ************************************************************/
fbe_status_t 				   
fbe_dest_set_timer(fbe_packet_t * packet,                                   
                   fbe_time_t timeout_msec,
                   fbe_packet_completion_function_t completion_function,
                   fbe_packet_completion_context_t completion_context)
{
    fbe_dest_timer_t * timer;
    if(dest_timer_idle_queue_thread_flag != DEST_TIMER_THREAD_RUN) 
    {
        dest_log_info("%s Failed: Timer thread is not running.\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

   /*we are getting a free timer element from the free pool*/
	fbe_spinlock_lock(&dest_timer_free_pool_lock);
	timer = (fbe_dest_timer_t *)fbe_queue_pop(&dest_timer_free_pool_queue_head);
	fbe_spinlock_unlock(&dest_timer_free_pool_lock);
	if(timer != NULL){	   
            fbe_queue_element_init(&timer->queue_element);
            timer->packet = packet;
            timer->expiration_time = fbe_get_time() + timeout_msec;

            fbe_transport_set_completion_function(timer->packet, completion_function,  completion_context);
	
            fbe_spinlock_lock(&dest_timer_lock);
            fbe_queue_push(&dest_timer_idle_queue_head, &timer->queue_element);
            fbe_spinlock_unlock(&dest_timer_lock);
	}
	else
	{				
		dest_log_error("%s DEST: Ran out of resources for delaying IO, so IO will be completed immediately.", __FUNCTION__);
		return FBE_STATUS_GENERIC_FAILURE;
	}  
	
	return FBE_STATUS_OK;
}
/************************************************************
 *  fbe_dest_set_timer()
 ************************************************************/

/************************************************************
 *  fbe_dest_cancel_timer()
 ************************************************************
 *
 * @brief   Cancel delayed IO.
 *
 * @param   timer  - timer queue element
 * 
 * @return  status
 * 
 * @author  06/01/2011  Wayne Garrett  - Created
 *  
 ************************************************************/
fbe_status_t 
fbe_dest_cancel_timer(fbe_dest_timer_t * timer)
{
	fbe_spinlock_lock(&dest_timer_lock);
	fbe_queue_remove(&timer->queue_element);
	fbe_spinlock_unlock(&dest_timer_lock);

	fbe_transport_complete_packet(timer->packet);

	return FBE_STATUS_OK;
}
/************************************************************
 *  fbe_dest_cancel_timer()
 ************************************************************/
/************************************************************
 *  dest_timer_free_pool_alloc()
 ************************************************************
 *
 * @brief   Allocate timer queue out of free pool.
 *
 * @param   pool_queue_size  - size of the timer queue to be allocated
 * 
 * @return  status
 * 
 * @author  04/13/2012  kothal  - Created
 *  
 ************************************************************/
fbe_status_t
dest_timer_free_pool_alloc(int pool_queue_size)
{
	fbe_dest_timer_t * dest_timer_free_pool;
	int i = 0;
	
	for (i = 0; i < pool_queue_size; i++)	
	{
	    dest_timer_free_pool = fbe_memory_ex_allocate(sizeof(fbe_dest_timer_t));
		if(dest_timer_free_pool == NULL){
			dest_log_error("DEST: %s Can't allocate memory\n", __FUNCTION__);
			dest_timer_free_pool_release();
			return FBE_STATUS_INSUFFICIENT_RESOURCES;
		}
		fbe_set_memory(dest_timer_free_pool, 0, sizeof(fbe_dest_timer_t));
		/*no locking is needed here since each of these pointers is updated only by one thread. */
		fbe_queue_push(&dest_timer_free_pool_queue_head,&dest_timer_free_pool->queue_element);
	}		
	
	
	return FBE_STATUS_OK;
}

/************************************************************
 *  dest_timer_free_pool_alloc()
 ************************************************************/

/************************************************************
 *  dest_timer_free_pool_release()
 ************************************************************
 *
 * @brief   Release memory allocated for timer queue
 * 
 * @return  status
 * 
 * @author  04/13/2012  kothal  - Created
 *  
 ************************************************************/
fbe_status_t
dest_timer_free_pool_release(void)
{		
	fbe_queue_element_t * next_element;
	next_element = fbe_queue_front(&dest_timer_free_pool_queue_head);
	while(next_element != NULL)	
	{   		
		fbe_queue_remove(next_element);
		fbe_memory_ex_release(next_element);			
		next_element = fbe_queue_front(&dest_timer_free_pool_queue_head);	   			
	}	
	
	return FBE_STATUS_OK;
}

/************************************************************
 *  dest_timer_free_pool_release()
 ************************************************************/
