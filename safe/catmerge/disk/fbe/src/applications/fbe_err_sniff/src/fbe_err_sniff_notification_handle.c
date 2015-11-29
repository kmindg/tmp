/***************************************************************************
* Copyright (C) EMC Corporation 2011 
* All rights reserved. 
* Licensed material -- property of EMC Corporation
***************************************************************************/

/*!*************************************************************************
*                 @file fbe_err_sniff_notification_handle.c
****************************************************************************
*
* @brief
*  This file contains notification handlers for fbe_err_sniff.
*
* @ingroup fbe_err_sniff
*
* @date
*  08/08/2011 - Created. Vera Wang
*
***************************************************************************/

/*************************
*   INCLUDE FILES
*************************/
#include "fbe_err_sniff_notification_handle.h"
#include "fbe_err_sniff_file_access.h"
#include "fbe_err_sniff_call_sp_collector.h"   
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_panic_sp_interface.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_queue.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"

/*!************************
*   LOCAL VARIABLES
**************************/
static fbe_bool_t					fbe_err_sniff_notification_initialized = FBE_FALSE;
static fbe_bool_t     				fbe_err_sniff_notification_thread_run;
static fbe_semaphore_t              fbe_err_sniff_notification_semaphore;
static fbe_spinlock_t               fbe_err_sniff_notification_queue_lock;
static fbe_queue_head_t             fbe_err_sniff_notification_queue_head;
static fbe_notification_registration_id_t	fbe_err_sniff_notification_reg_id;
static fbe_thread_t                 fbe_err_sniff_notification_thread_handle;

//(Stamp + Msg is 256+9*4+32+1)
#define ERR_MSG_SIZE 325

typedef struct fbe_err_sniff_notification_info_s{
	fbe_queue_element_t		queue_element;/*must be first to simplify things*/
	fbe_notification_type_t notification_type;
    fbe_char_t error_trace_info[ERR_MSG_SIZE];
}fbe_err_sniff_notification_info_t;

typedef struct fbe_err_sniff_notifinication_thread_info_s{
    FILE* fp;
    fbe_char_t panic_flag;
}fbe_err_sniff_notifinication_thread_info_t;

/*!************************
*   LOCAL FUNCTIONS
**************************/
static void fbe_err_sniff_notification_thread_func(void *context);
static void fbe_err_sniff_notification_call_back(update_object_msg_t * update_object_msg, void * context); 


/*!**********************************************************************
*             fbe_err_sniff_notification_handle_init()               
*************************************************************************
*
*  @brief
*    fbe_err_sniff_notification_handle_init - Initiate notification handler
*	
*  @param    * fp - A pointer to a file
*
*  @return
*			 fbe_status_t 
*************************************************************************/
fbe_status_t fbe_err_sniff_notification_handle_init(FILE * fp, fbe_char_t panic_flag)
{
	fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;
	EMCPAL_STATUS       nt_status;
    fbe_err_sniff_notifinication_thread_info_t* thread_info = (fbe_err_sniff_notifinication_thread_info_t*)malloc(sizeof(fbe_err_sniff_notifinication_thread_info_t));

    if (thread_info == NULL) 
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s, failed to allocate memory to thread info. \n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
	if (fbe_err_sniff_notification_initialized) 
    {
		fbe_api_trace(FBE_TRACE_LEVEL_INFO, "%s, already initialized.\n", __FUNCTION__);
        free(thread_info);  
		return FBE_STATUS_OK;
	}
	
	fbe_err_sniff_notification_initialized = FBE_FALSE;
	
	fbe_queue_init(&fbe_err_sniff_notification_queue_head);
	fbe_spinlock_init(&fbe_err_sniff_notification_queue_lock);
	fbe_semaphore_init(&fbe_err_sniff_notification_semaphore, 0, FBE_SEMAPHORE_MAX);

	fbe_err_sniff_notification_thread_run = FBE_TRUE;
    thread_info->fp = fp;
    thread_info->panic_flag = panic_flag;

	/*start the thread that will execute updates from the queue*/
	nt_status = fbe_thread_init(&fbe_err_sniff_notification_thread_handle, "fbe_sniff_notification", fbe_err_sniff_notification_thread_func, thread_info);
	if (nt_status != EMCPAL_STATUS_SUCCESS) 
    {
		fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: can't start notification thread, ntstatus:%X\n", __FUNCTION__,  nt_status); 
        free(thread_info);  
		return FBE_STATUS_GENERIC_FAILURE;
	}

	status = fbe_api_notification_interface_register_notification(FBE_NOTIFICATION_TYPE_FBE_ERROR_TRACE,
														FBE_PACKAGE_NOTIFICATION_ID_ALL_PACKAGES,
														FBE_TOPOLOGY_OBJECT_TYPE_ALL,
														fbe_err_sniff_notification_call_back,
														NULL,
														&fbe_err_sniff_notification_reg_id);

	if (status != FBE_STATUS_OK) 
	{
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Can't register with notification, status:%X\n", __FUNCTION__,  status);  
		return FBE_STATUS_GENERIC_FAILURE;
	}
	
	fbe_err_sniff_notification_initialized = FBE_TRUE;
	return status;
}
/***********************************************
 * end fbe_err_sniff_notification_handle_init()
 **********************************************/ 

/*!**********************************************************************
*             fbe_err_sniff_notification_handle_destroy()               
*************************************************************************
*
*  @brief
*    fbe_err_sniff_notification_handle_destroy - Destroy notification handler
*	
*  @param    None
*
*  @return
*			 fbe_status_t 
*************************************************************************/
fbe_status_t fbe_err_sniff_notification_handle_destroy(void)
{
    fbe_bool_t              error_printed = FBE_FALSE;
    fbe_queue_element_t *   queue_element = NULL;

    if (!fbe_err_sniff_notification_initialized) {
        return FBE_STATUS_OK;
    }

    /*we don't want to get any more notifications*/
    fbe_api_notification_interface_unregister_notification(fbe_err_sniff_notification_call_back, fbe_err_sniff_notification_reg_id);

    /*change the state of the sempahore so the thread will not block*/
    fbe_err_sniff_notification_thread_run = FBE_FALSE;
    fbe_semaphore_release(&fbe_err_sniff_notification_semaphore, 0, 1, FALSE);
    fbe_thread_wait(&fbe_err_sniff_notification_thread_handle);
    fbe_thread_destroy(&fbe_err_sniff_notification_thread_handle);
    fbe_semaphore_destroy(&fbe_err_sniff_notification_semaphore);

    /*check and drain queues*/
    fbe_spinlock_lock(&fbe_err_sniff_notification_queue_lock);
    while (!fbe_queue_is_empty(&fbe_err_sniff_notification_queue_head)) {
        queue_element = fbe_queue_pop(&fbe_err_sniff_notification_queue_head);
        fbe_spinlock_unlock(&fbe_err_sniff_notification_queue_lock);
        fbe_api_free_memory((void *)queue_element);
        if (!error_printed) {
            error_printed = FBE_TRUE;
            fbe_api_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, "%s notification queue is not empty\n", __FUNCTION__);
        }  
        fbe_spinlock_lock(&fbe_err_sniff_notification_queue_lock);  
    }
    fbe_spinlock_unlock(&fbe_err_sniff_notification_queue_lock);
    fbe_queue_destroy(&fbe_err_sniff_notification_queue_head);
    fbe_spinlock_destroy(&fbe_err_sniff_notification_queue_lock);

    fbe_err_sniff_notification_initialized = FBE_FALSE;

    return FBE_STATUS_OK;
}
/**************************************************
 * end fbe_err_sniff_notification_handle_destroy()
 *************************************************/ 

/*!**********************************************************************
*             fbe_err_sniff_notification_thread_func()               
*************************************************************************
*
*  @brief
*    fbe_err_sniff_notification_thread_func - the thread function for 
*        notification
*
*  @param    * fp - A pointer to a file
*
*  @return
*			 None
*************************************************************************/
static void fbe_err_sniff_notification_thread_func(void *context)
{
    fbe_err_sniff_notifinication_thread_info_t* thread_info = (fbe_err_sniff_notifinication_thread_info_t*) context; 
    fbe_err_sniff_notification_info_t *  notification_info;  
    FILE *fp = thread_info->fp;
    fbe_char_t panic_flag = thread_info->panic_flag;

    fbe_api_trace (FBE_TRACE_LEVEL_DEBUG_HIGH, "%s:\n", __FUNCTION__); 
    
    while(1)    
    {
        /*block until someone takes down the semaphore*/
        fbe_semaphore_wait(&fbe_err_sniff_notification_semaphore, NULL);

        if(fbe_err_sniff_notification_thread_run)
        {
            fbe_spinlock_lock(&fbe_err_sniff_notification_queue_lock);
            /*and try to see if someone is waiting for this notification*/
            while (!fbe_queue_is_empty(&fbe_err_sniff_notification_queue_head)) 
            {
               notification_info = (fbe_err_sniff_notification_info_t *)fbe_queue_pop(&fbe_err_sniff_notification_queue_head);
               fbe_spinlock_unlock(&fbe_err_sniff_notification_queue_lock);
               //process the message "trace_error.txt"
               fbe_err_sniff_write_to_file(fp,notification_info->error_trace_info);
               fflush(fp);  
               if (panic_flag == 'p') {
                   fbe_api_panic_sp();
               }
               fbe_spinlock_lock(&fbe_err_sniff_notification_queue_lock);    
            } 
            fbe_spinlock_unlock(&fbe_err_sniff_notification_queue_lock);
            if (!fbe_err_sniff_get_spcollect_run()) 
            {
               fbe_err_sniff_set_spcollect_run(FBE_TRUE);
               fbe_err_sniff_call_sp_collector();
            }
        } 
        else 
        {
            break;
        }
    }
           
    free(thread_info);  
    fbe_thread_exit(EMCPAL_STATUS_SUCCESS);
}
/***************************************************
 * end fbe_err_sniff_notification_thread_func()
 **************************************************/ 

/*!**********************************************************************
*             fbe_err_sniff_notification_call_back()               
*************************************************************************
*
*  @brief
*    fbe_err_sniff_notification_call_back - Initiate notification handler
*	
*  @param    update_object_msg - pointer to update_object_msg data
*  @param    context - pointer to context info
*
*  @return
*			 None 
*************************************************************************/
static void fbe_err_sniff_notification_call_back(update_object_msg_t * update_object_msg, void * context)
{
	fbe_err_sniff_notification_info_t * notification_info = NULL;
	
	notification_info = (fbe_err_sniff_notification_info_t *)fbe_api_allocate_memory(sizeof(fbe_err_sniff_notification_info_t));
	notification_info->notification_type = update_object_msg->notification_info.notification_type;
    fbe_copy_memory(notification_info->error_trace_info, update_object_msg->notification_info.notification_data.error_trace_info.bytes, ERR_MSG_SIZE);   
                                                                                                                                                                                                                                                
    /*and let the thread take it from here*/
	fbe_spinlock_lock(&fbe_err_sniff_notification_queue_lock);
    fbe_queue_push(&fbe_err_sniff_notification_queue_head, &notification_info->queue_element);      
	fbe_spinlock_unlock(&fbe_err_sniff_notification_queue_lock);
	fbe_semaphore_release(&fbe_err_sniff_notification_semaphore, 0, 1, FALSE); 

}
/************************************************
 * end fbe_err_sniff_notification_call_back()
 ***********************************************/
 
/***********************************************
* end file fbe_err_sniff_notification_handle.c
***********************************************/
