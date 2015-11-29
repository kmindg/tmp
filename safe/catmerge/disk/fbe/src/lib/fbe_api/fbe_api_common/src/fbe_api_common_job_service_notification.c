/***************************************************************************
 * Copyright (C) EMC Corporation 2001-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_api_commonjob_service_notification.c
 ***************************************************************************
 *
 * @brief
 *  This file contains definitions of functions used to get notifications from the job
 *  service and let FBE APIs wait for these notifications
 *
 ***************************************************************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_queue.h"
#include "fbe/fbe_job_service_interface.h"

/*********************************************************************/
/*                  loacal definitions                              */
/*********************************************************************/

/*a notifcation that was there for more than 5 minute and was unclaimed should be removed*/
#define NOTIFICATION_MAX_WAIT_TIME	300000
#define JOB_CLEANUP_WAIT_INTERVAL	1000
#define JOB_CLEANUP_WAIT_COUNT		10

typedef struct fbe_api_job_service_notification_info_s{
	fbe_queue_element_t					queue_element;/*must be first to simplify things*/
    fbe_u64_t							job_number;
	fbe_job_service_error_type_t		job_error_code;
	fbe_status_t						job_status;
    fbe_object_id_t                     object_id;   /* bring back the obj_id when create LUN/RG */
	fbe_semaphore_t						notification_semaphore;
	fbe_time_t							notification_time;
}fbe_api_job_service_notification_info_t;

/*********************************************************************/
/*                  loacal variables                                 */
/*********************************************************************/
static fbe_bool_t					fbe_api_job_service_notification_initialized = FBE_FALSE;
static fbe_thread_t                 fbe_api_job_service_notification_thread_handle;
static fbe_bool_t     				fbe_api_job_service_notification_thread_run;
static fbe_semaphore_t              fbe_api_job_service_notification_semaphore;
static fbe_spinlock_t               fbe_api_job_service_notification_queue_lock;
static fbe_queue_head_t             fbe_api_job_service_notification_queue_head;
static fbe_queue_head_t             fbe_api_job_service_waiting_queue_head;
static fbe_notification_registration_id_t	fbe_api_job_service_notification_reg_id;
static fbe_thread_t                 fbe_api_job_service_notification_cleanup_thread_handle;

/*********************************************************************/
/*                  loacal functions                                 */
/*********************************************************************/
static void fbe_api_job_service_notification_thread_func(void * context);
static void be_api_job_service_notification_cleanup_thread_func(void * context);
static void fbe_api_job_service_notification_notify_handler (update_object_msg_t * update_object_msg, void * context);
static void fbe_api_job_service_notification_dispatch_queue(void);
static void fbe_api_job_service_notification_cleanup(void);

/*****************************************************************************************************************/

fbe_status_t FBE_API_CALL fbe_api_common_init_job_notification(void)
{
	EMCPAL_STATUS       nt_status;
	fbe_status_t		status;

	if (fbe_api_job_service_notification_initialized) {
		fbe_api_trace(FBE_TRACE_LEVEL_INFO, "%s, already initialized.\n", __FUNCTION__);
		return FBE_STATUS_OK;
	}
	
	fbe_api_job_service_notification_initialized = FBE_FALSE;
	
	fbe_queue_init(&fbe_api_job_service_notification_queue_head);
	fbe_queue_init(&fbe_api_job_service_waiting_queue_head);
	fbe_spinlock_init(&fbe_api_job_service_notification_queue_lock);
	fbe_semaphore_init(&fbe_api_job_service_notification_semaphore, 0, 100);/*100 concurrent jobs*/

	fbe_api_job_service_notification_thread_run = FBE_TRUE;

	/*start the thread that will execute updates from the queue*/
	nt_status = fbe_thread_init(&fbe_api_job_service_notification_thread_handle, "fbe_job_notif", fbe_api_job_service_notification_thread_func, NULL);
	if (nt_status != EMCPAL_STATUS_SUCCESS) {
		fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: can't start notification thread, ntstatus:%X\n", __FUNCTION__,  nt_status); 
		return FBE_STATUS_GENERIC_FAILURE;
	}

	/*start the thread that will do a cleanup of unclaimed job notifications*/
	nt_status = fbe_thread_init(&fbe_api_job_service_notification_cleanup_thread_handle, "fbe_job_cleanup",
								be_api_job_service_notification_cleanup_thread_func, NULL);

	if (nt_status != EMCPAL_STATUS_SUCCESS) {
		fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: can't start notification thread, ntstatus:%X\n", __FUNCTION__,  nt_status); 
		return FBE_STATUS_GENERIC_FAILURE;
	}


	/*and register with the the notification portion to get notifications when jobs are done*/
	status = fbe_api_notification_interface_register_notification(FBE_NOTIFICATION_TYPE_JOB_ACTION_STATE_CHANGED,
                                                                  FBE_PACKAGE_NOTIFICATION_ID_SEP_0,
                                                                  FBE_TOPOLOGY_OBJECT_TYPE_ALL,
                                                                  fbe_api_job_service_notification_notify_handler,
                                                                  NULL,
                                                                  &fbe_api_job_service_notification_reg_id);
	if (status != FBE_STATUS_OK) {
		fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: can't register with notification, status:%X\n", __FUNCTION__,  status); 
		fbe_api_job_service_notification_thread_run = FBE_FALSE;
		fbe_semaphore_release(&fbe_api_job_service_notification_semaphore, 0, 1, FALSE);
		fbe_thread_wait(&fbe_api_job_service_notification_thread_handle);
		fbe_thread_destroy(&fbe_api_job_service_notification_thread_handle);
	
		fbe_thread_wait(&fbe_api_job_service_notification_cleanup_thread_handle);
		fbe_thread_destroy(&fbe_api_job_service_notification_cleanup_thread_handle);
	
		fbe_semaphore_destroy(&fbe_api_job_service_notification_semaphore);
		fbe_queue_destroy(&fbe_api_job_service_notification_queue_head);
		fbe_queue_destroy(&fbe_api_job_service_waiting_queue_head);
		fbe_spinlock_destroy(&fbe_api_job_service_notification_queue_lock);
		
		return FBE_STATUS_GENERIC_FAILURE;
	}

	fbe_api_job_service_notification_initialized = FBE_TRUE;

	return FBE_STATUS_OK;
}

fbe_status_t FBE_API_CALL fbe_api_common_destroy_job_notification(void)
{
	fbe_bool_t				error_printed = FBE_FALSE;
	fbe_queue_element_t	*	queue_element = NULL;	

    if (!fbe_api_job_service_notification_initialized) {
		return FBE_STATUS_OK;
	}

	/*we don't want to get any more notifications*/
	fbe_api_notification_interface_unregister_notification(fbe_api_job_service_notification_notify_handler, fbe_api_job_service_notification_reg_id);

    /*change the state of the sempahore so the thread will not block*/
	fbe_api_job_service_notification_thread_run = FBE_FALSE;
    fbe_semaphore_release(&fbe_api_job_service_notification_semaphore, 0, 1, FALSE);
	fbe_thread_wait(&fbe_api_job_service_notification_thread_handle);
	fbe_thread_destroy(&fbe_api_job_service_notification_thread_handle);

	fbe_thread_wait(&fbe_api_job_service_notification_cleanup_thread_handle);
	fbe_thread_destroy(&fbe_api_job_service_notification_cleanup_thread_handle);

	fbe_semaphore_destroy(&fbe_api_job_service_notification_semaphore);

	/*check and drain queues*/
	fbe_spinlock_lock(&fbe_api_job_service_notification_queue_lock);
	while (!fbe_queue_is_empty(&fbe_api_job_service_notification_queue_head)) {
		queue_element = fbe_queue_pop(&fbe_api_job_service_notification_queue_head);
		fbe_api_free_memory((void *)queue_element);
		if (!error_printed) {
			error_printed = FBE_TRUE;
			fbe_api_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, "%s notification queue is not empty\n", __FUNCTION__);
		}
	}

	error_printed = FBE_FALSE;
	while (!fbe_queue_is_empty(&fbe_api_job_service_waiting_queue_head)) {
		queue_element = fbe_queue_pop(&fbe_api_job_service_waiting_queue_head);
		fbe_api_free_memory((void *)queue_element);
		if (!error_printed) {
			error_printed = FBE_TRUE;
			fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s waiting queue is not empty, someone is waiting on a job\n", __FUNCTION__);
		}
	}

	fbe_spinlock_unlock(&fbe_api_job_service_notification_queue_lock);

	fbe_queue_destroy(&fbe_api_job_service_notification_queue_head);
	fbe_queue_destroy(&fbe_api_job_service_waiting_queue_head);
	fbe_spinlock_destroy(&fbe_api_job_service_notification_queue_lock);
    
	fbe_api_job_service_notification_initialized = FBE_FALSE;
	
	return FBE_STATUS_OK;
}


static void fbe_api_job_service_notification_thread_func(void * context)
{
    FBE_UNREFERENCED_PARAMETER(context);
    
    fbe_api_trace (FBE_TRACE_LEVEL_DEBUG_HIGH, "%s:\n", __FUNCTION__); 

    while(1)    
    {
        /*block until someone takes down the semaphore*/
        fbe_semaphore_wait(&fbe_api_job_service_notification_semaphore, NULL);
        if(fbe_api_job_service_notification_thread_run) {
            /*check if there are any notifictions waiting on the queue*/
            fbe_api_job_service_notification_dispatch_queue();
        } else {
            break;
        }
    }

    fbe_api_trace (FBE_TRACE_LEVEL_DEBUG_HIGH, "%s: done\n", __FUNCTION__); 
    
    fbe_thread_exit(EMCPAL_STATUS_SUCCESS);

}

static void fbe_api_job_service_notification_notify_handler (update_object_msg_t * update_object_msg, void * context)
{
	fbe_api_job_service_notification_info_t	*notification_info = NULL;

	/*we are interested in knowing which job completed*/
	notification_info = (fbe_api_job_service_notification_info_t *)fbe_api_allocate_memory(sizeof(fbe_api_job_service_notification_info_t));
	if (notification_info == NULL) {
		fbe_api_trace(FBE_TRACE_LEVEL_INFO, "%s:out of memory, job notification lost\n",__FUNCTION__);
		return;
	}

	notification_info->job_number = update_object_msg->notification_info.notification_data.job_service_error_info.job_number;
	notification_info->job_status = update_object_msg->notification_info.notification_data.job_service_error_info.status;
	notification_info->job_error_code = update_object_msg->notification_info.notification_data.job_service_error_info.error_code;
    notification_info->object_id = update_object_msg->notification_info.notification_data.job_service_error_info.object_id;
	notification_info->notification_time = fbe_get_time();

#if 0
	/*in order not to saturate the traces with creation of PVDs by the config service, we will not trace these*/
	if ((update_object_msg->notification_info.notification_data.job_service_error_info.job_type != FBE_JOB_TYPE_CREATE_PROVISION_DRIVE)||
        (notification_info->job_status != FBE_STATUS_OK))
    {
		fbe_api_trace(FBE_TRACE_LEVEL_DEBUG_LOW, 
                      "Job service notification: obj: 0x%x job: 0x%llx type: 0x%llx status:0x%x error code: %s\n", 
                      update_object_msg->object_id, notification_info->job_number, 
                      update_object_msg->notification_info.notification_type,
                      notification_info->job_status,
                      fbe_api_job_service_notification_error_code_to_string(notification_info->job_error_code));
	}
#endif    
	/*and let the thread take it from here*/
	fbe_spinlock_lock(&fbe_api_job_service_notification_queue_lock);
	fbe_queue_push(&fbe_api_job_service_notification_queue_head, &notification_info->queue_element);
	fbe_spinlock_unlock(&fbe_api_job_service_notification_queue_lock);

	fbe_semaphore_release(&fbe_api_job_service_notification_semaphore, 0, 1, FALSE);

}

static void fbe_api_job_service_notification_dispatch_queue(void)
{
	fbe_api_job_service_notification_info_t	*	waiting_notification_info = NULL;
	fbe_api_job_service_notification_info_t *	received_notification_info = NULL;
    fbe_u32_t									queued_notifications = 0;
	fbe_bool_t									match_found = FBE_FALSE;
	
	/*let's see if this notification is matching any of the ones on the registration queue*/
	fbe_spinlock_lock(&fbe_api_job_service_notification_queue_lock);

	/*let's count how many we have on the queue so we will do only one pass*/
	received_notification_info = (fbe_api_job_service_notification_info_t *)fbe_queue_front(&fbe_api_job_service_notification_queue_head);
	while (received_notification_info != NULL) {
		queued_notifications++;
		received_notification_info = (fbe_api_job_service_notification_info_t *)fbe_queue_next(&fbe_api_job_service_notification_queue_head, &received_notification_info->queue_element);
	}

//    fbe_api_trace(FBE_TRACE_LEVEL_DEBUG_LOW, "%s, queued_notifications:%d\n", __FUNCTION__, queued_notifications);

	/*and try to see if someone is waiting for this notificaiton*/
    while (queued_notifications != 0 &&
		   !fbe_queue_is_empty(&fbe_api_job_service_notification_queue_head) &&
		   !fbe_queue_is_empty(&fbe_api_job_service_waiting_queue_head)) {
			received_notification_info = (fbe_api_job_service_notification_info_t *)fbe_queue_pop(&fbe_api_job_service_notification_queue_head);
			waiting_notification_info = (fbe_api_job_service_notification_info_t *)fbe_queue_front(&fbe_api_job_service_waiting_queue_head);
			match_found = FBE_FALSE;
			while (waiting_notification_info != NULL) {
				if ((waiting_notification_info->job_number == received_notification_info->job_number)) {
//    fbe_api_trace(FBE_TRACE_LEVEL_INFO, "%s, job found:%d\n", __FUNCTION__, waiting_notification_info->job_number);
					match_found = FBE_TRUE;
					/*we got a notification for this job, let's set the semaphore, the user will take care of the rest(remove form queue and delete*/
					waiting_notification_info->job_error_code = received_notification_info->job_error_code;
					waiting_notification_info->job_status = received_notification_info->job_status;
                    waiting_notification_info->object_id = received_notification_info->object_id;
					fbe_queue_remove(&received_notification_info->queue_element);
					fbe_semaphore_release(&waiting_notification_info->notification_semaphore, 0, 1, FALSE);
					fbe_api_free_memory((void *)received_notification_info);
					break;
				}
	
				waiting_notification_info = (fbe_api_job_service_notification_info_t *)fbe_queue_next(&fbe_api_job_service_waiting_queue_head, &waiting_notification_info->queue_element);
				
			}
		

		/*put it back on the queue if there was no match, someone else will wait for it later since they did not have enough
		time to call fbe_api_common_wait_for_job. If they don't call it, the timer will remove it from the queue within 30 seconds*/
		if (!match_found) {
			fbe_queue_push(&fbe_api_job_service_notification_queue_head, &received_notification_info->queue_element);
		}

		queued_notifications--;
	} 

	fbe_spinlock_unlock(&fbe_api_job_service_notification_queue_lock);

}

/*********************************************************************
 *            fbe_api_common_wait_for_job ()
 *********************************************************************
 *
 *  Description: wait for a job to be completed
 *
 *  @param id - the id we got when we registered
 *  @param job_number - the job number we wait for
 *  @param timeout_in_ms - How long are we willing to wait
 *  @param job_error_code - status code from a completed job (OUT)
 *  @param job_status - status of a completed jon (OUT)
 *  @param object_id - the object id of created LUN/RG (OUT)
 *
 *  Return Value: FBE_STATUS_OK = job completed
 *				  FBE_STATUS_TIMEOUT = job did not complete in the time we wanted
 *                FBE_STATUS_GENERIC_FAILURE = something went wrong waiting for the job
 *  History:
 *    10/10/08: sharel  created
 *    11/05/13: Hongpo Gao modified 
 *    
 *********************************************************************/
fbe_status_t FBE_API_CALL  fbe_api_common_wait_for_job(fbe_u64_t job_number,
										 fbe_u32_t timeout_in_ms,
										 /*OUT*/fbe_job_service_error_type_t *job_error_code,
										 /*OUT*/fbe_status_t * job_status,
                                         /*OUT*/fbe_object_id_t *object_id)
{
	fbe_api_job_service_notification_info_t	*	notification_info = NULL;
	fbe_api_job_service_notification_info_t	*	queued_notification_info = NULL;
    fbe_status_t								status;

    /* default error */
    if (job_error_code != NULL) {
        *job_error_code = FBE_JOB_SERVICE_ERROR_INVALID;
    }

    /* default status */
    if (job_status != NULL) {
        *job_status = FBE_STATUS_GENERIC_FAILURE;
    }
    
    /* default object_id */
    if (object_id != NULL) {
        *object_id = FBE_OBJECT_ID_INVALID;
    }

	if (!fbe_api_job_service_notification_initialized) {
		fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s, fbe_api_common_init_job_notification was not called\n", __FUNCTION__);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	/*let's allocate before we lock, this we we don't have to unlock before we allocate and then lock again before qqueuing*/
	notification_info = (fbe_api_job_service_notification_info_t *)fbe_api_allocate_memory(sizeof(fbe_api_job_service_notification_info_t));

	fbe_spinlock_lock(&fbe_api_job_service_notification_queue_lock);

	/*first, let's check, myaybe this job completed already, we will go back on the queue just in case there is a job number
	there that was not cleaned and we already reset the sep driver and see the same job number on the queue*/
	if (!fbe_queue_is_empty(&fbe_api_job_service_notification_queue_head)) {
		queued_notification_info = (fbe_api_job_service_notification_info_t *)fbe_queue_front(&fbe_api_job_service_notification_queue_head);
		do {
			if (queued_notification_info->job_number == job_number) {
				/*we got a notification for this job, let's return it's status and take it off the queue*/
				fbe_queue_remove(&queued_notification_info->queue_element);
				fbe_spinlock_unlock(&fbe_api_job_service_notification_queue_lock);

				if (job_error_code != NULL) {
					*job_error_code = queued_notification_info->job_error_code;
				}

				if (job_status != NULL) {
					*job_status = queued_notification_info->job_status;
				}

                /* When the job is for creating LUN/RG, we can get the object id from notification */
                if (object_id != NULL) {
                    *object_id = queued_notification_info->object_id;
                }
				
				fbe_api_free_memory((void*)queued_notification_info);
				fbe_api_free_memory((void*)notification_info);/*the one we allocated for but never used*/
				return FBE_STATUS_OK;
	
			}else{
				/*and go to the next one*/
				queued_notification_info = (fbe_api_job_service_notification_info_t *)fbe_queue_next(&fbe_api_job_service_notification_queue_head, &queued_notification_info->queue_element);
			}
		}while (queued_notification_info != NULL);
	}

    
	/*if it's not there, we just need to wait for it to be signaled*/
	notification_info->job_number = job_number;
    fbe_semaphore_init(&notification_info->notification_semaphore, 0, 1);
	
	fbe_queue_push(&fbe_api_job_service_waiting_queue_head, &notification_info->queue_element);
	fbe_spinlock_unlock(&fbe_api_job_service_notification_queue_lock);

//	fbe_api_trace(FBE_TRACE_LEVEL_DEBUG_LOW, "%s, job:%llu not found, waiting...\n", __FUNCTION__, (unsigned long long)job_number);

	/*now we can wait for the job to be completed*/
	status = fbe_semaphore_wait_ms(&notification_info->notification_semaphore, timeout_in_ms);
	if (status == FBE_STATUS_TIMEOUT) {
		fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s, timed out waiting for job:%d\n", __FUNCTION__, (int)notification_info->job_number);
		if(job_error_code != NULL)
		{
			*job_error_code = FBE_JOB_SERVICE_ERROR_TIMEOUT;
		}
	}else if (status != FBE_STATUS_OK) {
		fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s, Failure while waiting for job:%d, status:0x%X\n", __FUNCTION__, (int)notification_info->job_number, status);
	}else if (status == FBE_STATUS_OK) {
		/*all set, let's return the information we got*/
		if (job_error_code != NULL) {
			*job_error_code = notification_info->job_error_code;
		}

		if (job_status != NULL) {
			*job_status = notification_info->job_status;
		}

        /* Return the object id if it creates LUN/RG */
        if (object_id != NULL) {
            *object_id = notification_info->object_id;
        }
	}

//	fbe_api_trace(FBE_TRACE_LEVEL_INFO, "%s, job:%llu, done waiting...\n", __FUNCTION__, (unsigned long long)job_number);

    fbe_spinlock_lock(&fbe_api_job_service_notification_queue_lock);
    fbe_semaphore_destroy(&notification_info->notification_semaphore); /* SAFEBUG - needs destroy */
	fbe_queue_remove(&notification_info->queue_element);
    fbe_spinlock_unlock(&fbe_api_job_service_notification_queue_lock);
	fbe_api_free_memory((void *)notification_info);
    
	return status;
}



static void be_api_job_service_notification_cleanup_thread_func(void * context)
{
	fbe_u32_t wait_count = 0;

	FBE_UNREFERENCED_PARAMETER(context);
    
    fbe_api_trace (FBE_TRACE_LEVEL_DEBUG_HIGH, "%s:\n", __FUNCTION__); 

    while(1)    
    {
        /*wait for a while before checking the queue*/
        if(fbe_api_job_service_notification_thread_run) {
			 fbe_thread_delay(JOB_CLEANUP_WAIT_INTERVAL);
			 wait_count++;
			 if (wait_count == JOB_CLEANUP_WAIT_COUNT) {
				 wait_count = 0;
				/*check if there are any notifictions waiting on the queue*/
				fbe_api_job_service_notification_cleanup();
			 }
        } else {
            break;
        }
    }

    fbe_api_trace (FBE_TRACE_LEVEL_DEBUG_HIGH, "%s:done\n", __FUNCTION__); 
    
    fbe_thread_exit(EMCPAL_STATUS_SUCCESS);

}

static void fbe_api_job_service_notification_cleanup(void)
{
	fbe_api_job_service_notification_info_t *	received_notification_info = NULL;
	fbe_api_job_service_notification_info_t *	temp_notification_info = NULL;
    fbe_u32_t									elapsed_sec = 0;
    
    /*let's see if queued notification are too old and should be removed*/
	fbe_spinlock_lock(&fbe_api_job_service_notification_queue_lock);

	received_notification_info = (fbe_api_job_service_notification_info_t *)fbe_queue_front(&fbe_api_job_service_notification_queue_head);
	while (received_notification_info != NULL) {
		elapsed_sec = fbe_get_elapsed_seconds(received_notification_info->notification_time);
		if (elapsed_sec  > NOTIFICATION_MAX_WAIT_TIME) {
			temp_notification_info = received_notification_info;
			received_notification_info = (fbe_api_job_service_notification_info_t *)fbe_queue_next(&fbe_api_job_service_notification_queue_head, &received_notification_info->queue_element);
			fbe_queue_remove(&temp_notification_info->queue_element);
			fbe_api_free_memory(temp_notification_info);
		}else{
			received_notification_info = (fbe_api_job_service_notification_info_t *)fbe_queue_next(&fbe_api_job_service_notification_queue_head, &received_notification_info->queue_element);
		}
	}

	fbe_spinlock_unlock(&fbe_api_job_service_notification_queue_lock);


}

void FBE_API_CALL fbe_api_common_clean_job_notification_queue(void)
{
	fbe_api_job_service_notification_info_t *	received_notification_info = NULL;
    
	fbe_spinlock_lock(&fbe_api_job_service_notification_queue_lock);
	
	while (!fbe_queue_is_empty(&fbe_api_job_service_notification_queue_head)) {
		received_notification_info = (fbe_api_job_service_notification_info_t *)fbe_queue_pop(&fbe_api_job_service_notification_queue_head);
		fbe_api_free_memory((void *)received_notification_info);

	}

	fbe_spinlock_unlock(&fbe_api_job_service_notification_queue_lock);

}
