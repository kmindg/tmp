
/***************************************************************************
* Copyright (C) EMC Corporation 2012 
* All rights reserved. 
* Licensed material -- property of EMC Corporation
***************************************************************************/

/*!*************************************************************************
*                 @file fbe_notification_analyzer_notification_handle.c
****************************************************************************
*
* @brief
*  This file contains notification handlers for fbe_notification_analyzer.
*
* @ingroup fbe_notification_analyzer
*
* @date
*  05/30/2012 - Created. Vera Wang
*
***************************************************************************/

/*************************
*   INCLUDE FILES
*************************/
#include "fbe_notification_analyzer_notification_handle.h"
#include "fbe_notification_analyzer_file_access.h"  
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_queue.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"

/*!************************
*   LOCAL VARIABLES
**************************/
static fbe_bool_t					fbe_notification_analyzer_notification_initialized = FBE_FALSE;
static fbe_bool_t     				fbe_notification_analyzer_notification_thread_run;
static fbe_semaphore_t              fbe_notification_analyzer_notification_semaphore;
static fbe_spinlock_t               fbe_notification_analyzer_notification_queue_lock;
static fbe_queue_head_t             fbe_notification_analyzer_notification_queue_head;
static fbe_notification_registration_id_t	fbe_notification_analyzer_notification_reg_id;
static fbe_thread_t                 fbe_notification_analyzer_notification_thread_handle;



/*!************************
*   LOCAL FUNCTIONS
**************************/
static void fbe_notification_analyzer_notification_thread_func(void *context);
static void fbe_notification_analyzer_notification_call_back(update_object_msg_t * update_object_msg, void * context); 


/*!**********************************************************************
*             fbe_notification_analyzer_notification_handle_init()               
*************************************************************************
*
*  @brief
*    fbe_notification_analyzer_notification_handle_init - Initiate notification handler
*	
*  @param    * fp - A pointer to a file
*
*  @return
*			 fbe_status_t 
*************************************************************************/
fbe_status_t fbe_notification_analyzer_notification_handle_init(FILE * fp,
                                                                fbe_notification_type_t type,
                                                                fbe_package_notification_id_mask_t package,
                                                                fbe_topology_object_type_t object_type)
{
	fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;
	EMCPAL_STATUS       nt_status;
    fbe_notification_analyzer_notifinication_thread_info_t* thread_info = (fbe_notification_analyzer_notifinication_thread_info_t*)malloc(sizeof(fbe_notification_analyzer_notifinication_thread_info_t));

    if (thread_info == NULL) 
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s, failed to allocate memory to thread info. \n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
	if (fbe_notification_analyzer_notification_initialized) 
    {
		fbe_api_trace(FBE_TRACE_LEVEL_INFO, "%s, already initialized.\n", __FUNCTION__);
        free(thread_info);  
		return FBE_STATUS_OK;
	}
	
	fbe_notification_analyzer_notification_initialized = FBE_FALSE;
	
	fbe_queue_init(&fbe_notification_analyzer_notification_queue_head);
	fbe_spinlock_init(&fbe_notification_analyzer_notification_queue_lock);
	fbe_semaphore_init(&fbe_notification_analyzer_notification_semaphore, 0, FBE_SEMAPHORE_MAX);

	fbe_notification_analyzer_notification_thread_run = FBE_TRUE;
    thread_info->fp = fp;

	/*start the thread that will execute updates from the queue*/
	nt_status = fbe_thread_init(&fbe_notification_analyzer_notification_thread_handle, "fbe_notification_analyzer_notification_handle", fbe_notification_analyzer_notification_thread_func, thread_info);
	if (nt_status != EMCPAL_STATUS_SUCCESS) 
    {
		fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: can't start notification thread, ntstatus:%X\n", __FUNCTION__,  nt_status); 
        free(thread_info);  
		return FBE_STATUS_GENERIC_FAILURE;
	}

	status = fbe_api_notification_interface_register_notification(FBE_NOTIFICATION_TYPE_ALL, FBE_PACKAGE_NOTIFICATION_ID_ALL_PACKAGES, FBE_TOPOLOGY_OBJECT_TYPE_ALL,
														fbe_notification_analyzer_notification_call_back,
														NULL,
														&fbe_notification_analyzer_notification_reg_id);

	if (status != FBE_STATUS_OK) 
	{
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Can't register with notification, status:%X\n", __FUNCTION__,  status);  
		return FBE_STATUS_GENERIC_FAILURE;
	} 
	
	fbe_notification_analyzer_notification_initialized = FBE_TRUE;
	return status;
}
/***********************************************
 * end fbe_notification_analyzer_notification_handle_init()
 **********************************************/ 

/*!**********************************************************************
*             fbe_notification_analyzer_notification_handle_destroy()               
*************************************************************************
*
*  @brief
*    fbe_notification_analyzer_notification_handle_destroy - Destroy notification handler
*	
*  @param    None
*
*  @return
*			 fbe_status_t 
*************************************************************************/
fbe_status_t fbe_notification_analyzer_notification_handle_destroy(void)
{
    fbe_bool_t              error_printed = FBE_FALSE;
    fbe_queue_element_t *   queue_element = NULL;

    if (!fbe_notification_analyzer_notification_initialized) {
        return FBE_STATUS_OK;
    }

    /*we don't want to get any more notifications*/
    fbe_api_notification_interface_unregister_notification(fbe_notification_analyzer_notification_call_back, fbe_notification_analyzer_notification_reg_id);

    /*change the state of the sempahore so the thread will not block*/
    fbe_notification_analyzer_notification_thread_run = FBE_FALSE;
    fbe_semaphore_release(&fbe_notification_analyzer_notification_semaphore, 0, 1, FALSE);
    fbe_thread_wait(&fbe_notification_analyzer_notification_thread_handle);
    fbe_thread_destroy(&fbe_notification_analyzer_notification_thread_handle);
    fbe_semaphore_destroy(&fbe_notification_analyzer_notification_semaphore);

    /*check and drain queues*/
    fbe_spinlock_lock(&fbe_notification_analyzer_notification_queue_lock);
    while (!fbe_queue_is_empty(&fbe_notification_analyzer_notification_queue_head)) {
        queue_element = fbe_queue_pop(&fbe_notification_analyzer_notification_queue_head);
        fbe_spinlock_unlock(&fbe_notification_analyzer_notification_queue_lock);
        fbe_api_free_memory((void *)queue_element);
        if (!error_printed) {
            error_printed = FBE_TRUE;
            fbe_api_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, "%s notification queue is not empty\n", __FUNCTION__);
        }  
        fbe_spinlock_lock(&fbe_notification_analyzer_notification_queue_lock);  
    }
    fbe_spinlock_unlock(&fbe_notification_analyzer_notification_queue_lock);
    fbe_queue_destroy(&fbe_notification_analyzer_notification_queue_head);
    fbe_spinlock_destroy(&fbe_notification_analyzer_notification_queue_lock);

    fbe_notification_analyzer_notification_initialized = FBE_FALSE;

    return FBE_STATUS_OK;
}
/**************************************************
 * end fbe_notification_analyzer_notification_handle_destroy()
 *************************************************/ 

/*!**********************************************************************
*             fbe_notification_analyzer_notification_thread_func()               
*************************************************************************
*
*  @brief
*    fbe_notification_analyzer_notification_thread_func - the thread function for 
*        notification
*
*  @param    * fp - A pointer to a file
*
*  @return
*			 None
*************************************************************************/
static void fbe_notification_analyzer_notification_thread_func(void *context)
{
    fbe_notification_analyzer_notifinication_thread_info_t* thread_info = (fbe_notification_analyzer_notifinication_thread_info_t*) context; 
    fbe_notification_analyzer_notification_info_t *  notification_info;  
    FILE *fp = thread_info->fp;

    fbe_api_trace (FBE_TRACE_LEVEL_DEBUG_HIGH, "%s:\n", __FUNCTION__); 
    
    while(1)    
    {
        /*block until someone takes down the semaphore*/
        fbe_semaphore_wait(&fbe_notification_analyzer_notification_semaphore, NULL);

        if(fbe_notification_analyzer_notification_thread_run)
        {
            fbe_spinlock_lock(&fbe_notification_analyzer_notification_queue_lock);
            /*and try to see if someone is waiting for this notification*/
            while (!fbe_queue_is_empty(&fbe_notification_analyzer_notification_queue_head)) 
            {
               notification_info = (fbe_notification_analyzer_notification_info_t *)fbe_queue_pop(&fbe_notification_analyzer_notification_queue_head);
               fbe_spinlock_unlock(&fbe_notification_analyzer_notification_queue_lock);
               //process the message "notification_analyzer.txt"
               fbe_notification_analyzer_write_to_file(fp,notification_info);
               fflush(fp);  
               
               fbe_spinlock_lock(&fbe_notification_analyzer_notification_queue_lock);    
            } 
            fbe_spinlock_unlock(&fbe_notification_analyzer_notification_queue_lock);
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
 * end fbe_notification_analyzer_notification_thread_func()
 **************************************************/ 

/*!**********************************************************************
*             fbe_notification_analyzer_notification_call_back()               
*************************************************************************
*
*  @brief
*    fbe_notification_analyzer_notification_call_back - Initiate notification handler
*	
*  @param    update_object_msg - pointer to update_object_msg data
*  @param    context - pointer to context info
*
*  @return
*			 None 
*************************************************************************/
static void fbe_notification_analyzer_notification_call_back(update_object_msg_t * update_object_msg, void * context)
{
	fbe_notification_analyzer_notification_info_t * notification_info = NULL;
	
	notification_info = (fbe_notification_analyzer_notification_info_t*)fbe_api_allocate_memory(sizeof(fbe_notification_analyzer_notification_info_t));
    notification_info->object_id = update_object_msg->object_id;
    fbe_copy_memory(&notification_info->notification_info, &update_object_msg->notification_info, sizeof(fbe_notification_info_t));  
                                                                                                                                                                                                                                                
    /*and let the thread take it from here*/
	fbe_spinlock_lock(&fbe_notification_analyzer_notification_queue_lock);
    fbe_queue_push(&fbe_notification_analyzer_notification_queue_head, &notification_info->queue_element);      
	fbe_spinlock_unlock(&fbe_notification_analyzer_notification_queue_lock);
	fbe_semaphore_release(&fbe_notification_analyzer_notification_semaphore, 0, 1, FALSE);   

}
/************************************************
 * end fbe_notification_analyzer_notification_call_back()
 ***********************************************/
 
/***********************************************
* end file fbe_notification_analyzer_notification_handle.c
***********************************************/
