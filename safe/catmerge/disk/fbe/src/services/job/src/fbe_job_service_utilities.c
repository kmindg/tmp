/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_job_service_utilities.c
 ***************************************************************************
 *
 * @brief
 *  This file contains job service utility functions that are required by
 *  the job service file to enqueue requests and by the job service file
 *  that processed job queue elements. 
 *  
 *
 * @version
 *  01/05/2010 - Created. Jesus Medina
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_queue.h"
#include "fbe/fbe_memory.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_transport.h"
#include "fbe_base_object.h"
#include "fbe_transport_memory.h"
#include "fbe_base_service.h"
#include "fbe_topology.h"
#include "fbe_job_service_private.h"
#include "fbe_job_service.h"
#include "fbe_spare.h"
#include "fbe_cmi.h"
#include "fbe_job_service_operations.h"
#include "fbe_job_service_cmi.h"
#include "../test/fbe_job_service_test_private.h"
#include "fbe_database.h"
#include "fbe_notification.h"
#include "fbe/fbe_event_log_api.h"                  // for fbe_event_log_write
#include "fbe/fbe_event_log_utils.h"                // for message codes


fbe_job_service_t fbe_job_service;

//static void fbe_job_service_remove_all_objects_from_recovery_queue(); 
//static void fbe_job_service_remove_all_objects_from_creation_queue(); 

/*************************
 *    INLINE FUNCTIONS 
 *************************/


/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/* Job service lock accessors.
*/
void fbe_job_service_lock(void)
{
    fbe_job_service_t *job_service_p = &fbe_job_service; 
    fbe_spinlock_lock(&job_service_p->lock);
    return;
}

void fbe_job_service_unlock(void)
{
    fbe_job_service_t *job_service_p = &fbe_job_service; 
    fbe_spinlock_unlock(&job_service_p->lock);
    return;
}

void fbe_job_service_increment_job_number(void)
{
    fbe_job_service_t *job_service_p = &fbe_job_service; 
    job_service_p->job_number = job_service_p->job_number + 1;
    return;
}

void fbe_job_service_set_job_number(fbe_u64_t job_number)
{
    fbe_job_service_t *job_service_p = &fbe_job_service; 

    fbe_spinlock_lock(&job_service_p->lock);
    job_service_p->job_number = job_number;
    fbe_spinlock_unlock(&job_service_p->lock);
    return;
}

fbe_u64_t fbe_job_service_increment_job_number_with_lock(void)
{
    fbe_u64_t job_number = 0;
    fbe_job_service_t *job_service_p = &fbe_job_service; 

    fbe_spinlock_lock(&job_service_p->lock);
    job_service_p->job_number = job_service_p->job_number + 1;
    job_number = job_service_p->job_number;
    fbe_spinlock_unlock(&job_service_p->lock);
    return job_number;
}

void fbe_job_service_decrement_job_number(void)
{
    fbe_job_service_t *job_service_p = &fbe_job_service; 
    job_service_p->job_number = job_service_p->job_number - 1;
    return;
}

void fbe_job_service_decrement_job_number_with_lock(void)
{
    fbe_job_service_t *job_service_p = &fbe_job_service; 
    fbe_spinlock_lock(&job_service_p->lock);
    job_service_p->job_number = job_service_p->job_number - 1;
    fbe_spinlock_unlock(&job_service_p->lock);
    return;
}

fbe_u64_t fbe_job_service_get_job_number()
{
    fbe_u64_t job_number_to_return = 0;

    fbe_job_service_t *job_service_p = &fbe_job_service; 
    fbe_spinlock_lock(&job_service_p->lock);
    job_number_to_return = job_service_p->job_number;
    fbe_spinlock_unlock(&job_service_p->lock);
    return job_number_to_return;
}

/* Accessors for the number of recovery objects.
*/
void fbe_job_service_increment_number_recovery_objects(void)
{
    fbe_job_service_t *job_service_p = &fbe_job_service; 
    job_service_p->number_recovery_objects++;
    return;
}

void fbe_job_service_decrement_number_recovery_objects(void)
{
    fbe_job_service_t *job_service_p = &fbe_job_service; 
    job_service_p->number_recovery_objects--;
    return;
}

fbe_u32_t fbe_job_service_get_number_recovery_requests(void)
{
    fbe_job_service_t *job_service_p = &fbe_job_service; 
    return job_service_p->number_recovery_objects;
}

/* Accessors for the number of creation objects.
*/
void fbe_job_service_increment_number_creation_objects(void)
{
    fbe_job_service_t *job_service_p = &fbe_job_service; 
    job_service_p->number_creation_objects++;
    return;
}

void fbe_job_service_decrement_number_creation_objects(void)
{
    fbe_job_service_t *job_service_p = &fbe_job_service; 
    job_service_p->number_creation_objects--;
    return;
}

fbe_u32_t fbe_job_service_get_number_creation_requests(void)
{
    fbe_job_service_t *job_service_p = &fbe_job_service; 
    return job_service_p->number_creation_objects;
}

/* Accessors to member fields of job service
*/
void fbe_job_service_init_number_recovery_request()
{
    fbe_job_service_t *job_service_p = &fbe_job_service; 
    job_service_p->number_recovery_objects = 0;
    return;
}

void fbe_job_service_init_number_creation_request()
{
    fbe_job_service_t *job_service_p = &fbe_job_service; 
    job_service_p->number_creation_objects = 0;
    return;
}

void fbe_job_service_set_recovery_queue_access(fbe_bool_t value)
{
    fbe_job_service_t *job_service_p = &fbe_job_service; 
    job_service_p->recovery_queue_access = value;
    return;
}

void fbe_job_service_set_creation_queue_access(fbe_bool_t value)
{
    fbe_job_service_t *job_service_p = &fbe_job_service; 
    job_service_p->creation_queue_access = value;
    return;
}

fbe_bool_t fbe_job_service_get_creation_queue_access()
{
    fbe_job_service_t *job_service_p = &fbe_job_service; 
    return job_service_p->creation_queue_access;
}

fbe_bool_t fbe_job_service_get_recovery_queue_access()
{
    fbe_job_service_t *job_service_p = &fbe_job_service; 
    return job_service_p->recovery_queue_access;
}

/* job service queue access semaphore */
void fbe_job_service_init_control_queue_semaphore()
{
    fbe_job_service_t *job_service_p = &fbe_job_service; 
    fbe_semaphore_init(&job_service_p->job_control_queue_semaphore, 
            0, 
            MAX_NUMBER_OF_JOB_CONTROL_ELEMENTS + 1);
    return;
}

/* job service queue client control */
void fbe_job_service_init_control_process_call_semaphore()
{
    fbe_job_service_t *job_service_p = &fbe_job_service; 
    fbe_semaphore_init(&job_service_p->job_control_process_semaphore, 
            0, 
            1);
    return;
}

/* job control accessors */
void fbe_job_service_enqueue_recovery_q(fbe_job_queue_element_t *entry_p)
{
    fbe_job_service_t *job_service_p = &fbe_job_service; 
    fbe_queue_push(&job_service_p->recovery_q_head, &entry_p->queue_element);
    return;
}

void fbe_job_service_dequeue_recovery_q(fbe_job_queue_element_t *entry_p)
{
    fbe_queue_remove(&entry_p->queue_element);
    return;
}

void fbe_job_service_enqueue_creation_q(fbe_job_queue_element_t *entry_p)
{
    fbe_job_service_t *job_service_p = &fbe_job_service; 
    fbe_queue_push(&job_service_p->creation_q_head, &entry_p->queue_element);
    return;
}

void fbe_job_service_dequeue_creation_q(fbe_job_queue_element_t *entry_p)
{
    fbe_queue_remove(&entry_p->queue_element);
    return;
}

void fbe_job_service_get_recovery_queue_head(fbe_queue_head_t **queue_p)
{
    fbe_job_service_t *job_service_p = &fbe_job_service; 
    *queue_p = &job_service_p->recovery_q_head;
    return;
}

void fbe_job_service_get_creation_queue_head(fbe_queue_head_t **queue_p)
{
    fbe_job_service_t *job_service_p = &fbe_job_service; 
    *queue_p = &job_service_p->creation_q_head;
    return;
}

/*!**************************************************************
 * job_service_trace()
 ****************************************************************
 * @brief
 * Trace function for use by the job service.
 * Takes into account the current global trace level and
 * the locally set trace level.
 *
 * @param trace_level - trace level of this message.
 * @param message_id  - generic identifier.
 * @param fmt...      - variable argument list starting with format.
 *
 * @return None.  
 *
 * @author
 * 10/01/2009 - Created. Jesus Medina
 *
 ****************************************************************/

void job_service_trace(fbe_trace_level_t trace_level,
        fbe_trace_message_id_t message_id,
        const char * fmt, ...)
{
    fbe_trace_level_t default_trace_level;
    fbe_trace_level_t service_trace_level;

    va_list args;

    /* Assume we are using the default trace level.
    */
    service_trace_level = default_trace_level = fbe_trace_get_default_trace_level();

    if (fbe_base_service_is_initialized(&fbe_job_service.base_service)) 
    {
        service_trace_level = fbe_base_service_get_trace_level(&fbe_job_service.base_service);
        if (default_trace_level > service_trace_level) 
        {
            service_trace_level = default_trace_level;
        }
    }
    if (trace_level > service_trace_level)
    {
        return;
    }

    va_start(args, fmt);
    fbe_trace_report(FBE_COMPONENT_TYPE_SERVICE,
            FBE_SERVICE_ID_JOB_SERVICE,
            trace_level,
            message_id,
            fmt, 
            args);
    va_end(args);
}
/********************************************
 * end job_service_trace()
 ********************************************/

/*!**************************************************************
 * job_service_trace()
 ****************************************************************
 * @brief
 * Trace function for use by the job service.
 * Takes into account the current global trace level and
 * the locally set trace level.
 *
 * @param trace_level - trace level of this message.
 * @param message_id  - generic identifier.
 * @param fmt...      - variable argument list starting with format.
 *
 * @return None.  
 *
 ****************************************************************/
void job_service_trace_object(fbe_object_id_t object_id,
                       fbe_trace_level_t trace_level,
                       fbe_trace_message_id_t message_id,
                       const char * fmt, ...)
{
    fbe_trace_level_t default_trace_level;
    fbe_trace_level_t service_trace_level;

    va_list args;

    service_trace_level = default_trace_level = fbe_trace_get_default_trace_level();
    if (fbe_base_service_is_initialized(&fbe_job_service.base_service)) {
        service_trace_level = fbe_base_service_get_trace_level(&fbe_job_service.base_service);
        if (default_trace_level > service_trace_level) {
            service_trace_level = default_trace_level;
        }
    }
    if (trace_level > service_trace_level) {
        return;
    }

    va_start(args, fmt);
    fbe_trace_report(FBE_COMPONENT_TYPE_OBJECT,
                     object_id,
                     trace_level,
                     message_id,
                     fmt, 
                     args);
    va_end(args);
}
/********************************************
 * end job_service_trace_object()
 ********************************************/

/*!**************************************************************
 * fbe_job_service_add_element_to_recovery_queue()
 ****************************************************************
 * @brief
 * This function adds an element to the recovery queue
 *
 * @param job_queue_element_p - element container for the 
 *                              recovery queue
 *
 * @return - fbe_status_t.
 *
 * @author
 * 10/01/2009 - Created. Jesus Medina
 * 22/02/2013 - Updated to add synchronized remove operation. Wenxuan Yin
 *
 ****************************************************************/

fbe_status_t fbe_job_service_add_element_to_recovery_queue(fbe_job_queue_element_t *job_element_p)
{
    fbe_job_queue_element_remove_action_t       element_remove_action;
    
    fbe_queue_head_t                            *head_p = NULL;
    fbe_job_queue_element_t                     *request_p = NULL;
    fbe_job_queue_element_t                     request_p_temp;
    fbe_u32_t                                   job_index = 0;
    fbe_status_t                                remove_status = FBE_STATUS_INVALID;
    fbe_status_t                                notify_status = FBE_STATUS_OK;
    
    if(fbe_job_service.number_recovery_objects >= MAX_NUMBER_OF_JOB_CONTROL_ELEMENTS)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, 
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "jbs_add_element_to_recovery_queue: Invalid number_of_objects %X\n",  
                fbe_job_service.number_recovery_objects);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*! Lock recovery queue 
    */
    fbe_job_service_lock();

    /*! Queue the request 
    */
    job_element_p->previous_state = FBE_JOB_ACTION_STATE_INVALID;
    job_element_p->current_state = FBE_JOB_ACTION_STATE_IN_QUEUE;
    //fbe_job_service.job_number = fbe_job_service.job_number + 1;
    //job_element_p->job_number = fbe_job_service.job_number;

    fbe_job_service_increment_number_recovery_objects();
    fbe_job_service_enqueue_recovery_q(job_element_p);

    job_service_trace(FBE_TRACE_LEVEL_INFO, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "jbs_add_element_to_recovery_queue: recovery_object: 0x%x, job_number: 0x%llX\n",
            fbe_job_service.number_recovery_objects,
            (unsigned long long)job_element_p->job_number);
    
    /* After the element is added into the queue, we also need check the 
    * "remove flag" which indicates whether there is an incompleted remove
    * request. This is a fix to enhance the synchronization between 
    * "add" and "remove" operation. Wenxuan Yin: 2/22/2013
    */
    
    /* First, get remove element flag and the job number */
    fbe_job_service_cmi_get_remove_job_action_on_block_in_progress(&element_remove_action);

    if(element_remove_action.flag == FBE_JOB_QUEUE_ELEMENT_REMOVE_FROM_RECOVERY_QUEUE)
    {
        /* If the element remove flag is set, we need remove the queue element.
        */
        job_service_trace(FBE_TRACE_LEVEL_INFO, 
                    FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                    "%s: received request to remove job from recovery queue\n",
                    __FUNCTION__);

        remove_status = FBE_STATUS_NO_OBJECT;
        fbe_job_service_get_recovery_queue_head(&head_p);
        request_p = (fbe_job_queue_element_t *) fbe_queue_front(head_p);
        while (request_p != NULL)
        {
            fbe_job_queue_element_t *next_request_p = NULL;

            if (request_p->job_number == element_remove_action.job_number)
            {
                job_service_trace(FBE_TRACE_LEVEL_INFO, 
                        FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                        "   job number %llu found in recovery queue, at index %d\n", 
                        (unsigned long long)element_remove_action.job_number, job_index);
                
                /*Copy the data into temp buffer*/                
                fbe_copy_memory(&request_p_temp, request_p, sizeof(fbe_job_queue_element_t));
                fbe_queue_remove((fbe_queue_element_t *)request_p);
                fbe_job_service_decrement_number_recovery_objects();
                if ((request_p->local_job_data_valid == FBE_TRUE) &&
                    (request_p->additional_job_data != NULL))
                {
                    fbe_memory_ex_release(request_p->additional_job_data);
                    request_p->additional_job_data = NULL;
                }
                fbe_transport_release_buffer(request_p);

                remove_status = FBE_STATUS_OK;
                break;
            }

            job_index += 1;
            next_request_p = (fbe_job_queue_element_t *)fbe_queue_next(head_p, &request_p->queue_element);
            request_p = next_request_p;
        }
        /* Clean up the remove job action */
        fbe_job_service_cmi_cleanup_remove_job_action_on_block_in_progress();
    }
    /* Unlock since we are done.
    */
    fbe_job_service_unlock();

    /* If an element is removed from queue, we need send a notification. */
    if (remove_status == FBE_STATUS_OK)
    {
        /*! @note Some recovery job types have no object associated with them:
         *          o FBE_JOB_TYPE_UPDATE_SPARE_LIBRARY_CONFIG
         */
        if (job_element_p->job_type == FBE_JOB_TYPE_UPDATE_SPARE_LIBRARY_CONFIG)
        {
            /* Return success.
             */
            return FBE_STATUS_OK;
        }

        /* First copy any fields that were generated on the peer to the 
         * temporary (i.e. on the stack) request structure.
         */
        request_p_temp.job_number = element_remove_action.job_number;
        request_p_temp.class_id = element_remove_action.class_id;
        request_p_temp.object_id = element_remove_action.object_id;
        request_p_temp.status = element_remove_action.status;
        request_p_temp.error_code = element_remove_action.error_code;
        request_p_temp.job_type = element_remove_action.job_element.job_type;
        request_p_temp.queue_type = element_remove_action.job_element.queue_type;
        request_p_temp.command_size = element_remove_action.job_element.command_size;
        fbe_copy_memory(&request_p_temp.command_data, &element_remove_action.job_element.command_data, element_remove_action.job_element.command_size);

        /* Now generate and send the appropriate notification.
         */
        notify_status = fbe_job_service_generate_and_send_notification(&request_p_temp);
        if (notify_status != FBE_STATUS_OK)
        {
            /* Generate a warning.
             */
            job_service_trace(FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Failed to send notification for job: 0x%llx \n",
                              __FUNCTION__, (unsigned long long)element_remove_action.job_number);
            return notify_status;
        }
    }

    /* Trace warning for NO object to be removed */
    if(remove_status == FBE_STATUS_NO_OBJECT)
    {
        job_service_trace(FBE_TRACE_LEVEL_WARNING,
                FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "%s: status %d, job %llu NOT found\n", 
                __FUNCTION__, remove_status, (unsigned long long)element_remove_action.job_number);
    }
    
    return FBE_STATUS_OK;
}
/*****************************************************
 * end fbe_job_service_add_element_to_recovery_queue()
 ****************************************************/

/*!**************************************************************
 * fbe_job_service_add_element_to_creation_queue()
 ****************************************************************
 * @brief
 * This function adds an element to the creation queue
 *
 * @param job_queue_element_p - element container for the 
 *                                recovery queue
 *
 * @return - fbe_status_t.
 *
 * @author
 * 12/08/2009 - Created. Jesus Medina
 * 22/02/2013 - Updated to add synchronized remove operation. Wenxuan Yin
 *
 ****************************************************************/

fbe_status_t fbe_job_service_add_element_to_creation_queue(fbe_job_queue_element_t *job_element_p)
{
    fbe_job_queue_element_remove_action_t       element_remove_action;
    fbe_notification_info_t                     notification_info;

    fbe_queue_head_t                            *head_p = NULL;
    fbe_job_queue_element_t                     *request_p = NULL;
    fbe_job_queue_element_t                     request_p_temp;
    fbe_u32_t                                   job_index = 0;
    fbe_status_t                                remove_status = FBE_STATUS_INVALID;
    fbe_status_t                                notify_status = FBE_STATUS_OK;

    if(fbe_job_service.number_creation_objects >= MAX_NUMBER_OF_JOB_CONTROL_ELEMENTS)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, 
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "jbs_add_element_to_creation_queue: Invalid number_of_objects %X\n",
                fbe_job_service.number_creation_objects);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*! Lock creation queue */
    fbe_job_service_lock();

    /*! Queue the request */
    job_element_p->previous_state = FBE_JOB_ACTION_STATE_INVALID;
    job_element_p->current_state = FBE_JOB_ACTION_STATE_IN_QUEUE;
    //fbe_job_service.job_number = fbe_job_service.job_number + 1;
    //job_element_p->job_number = fbe_job_service.job_number;

    fbe_job_service_increment_number_creation_objects();
    fbe_job_service_enqueue_creation_q(job_element_p);

    job_service_trace(FBE_TRACE_LEVEL_INFO, 
        FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
        "%s\n",__FUNCTION__);

    job_service_trace(FBE_TRACE_LEVEL_INFO, 
        FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
        "   number_creation_objects 0x%x, job number 0x%llX\n",
        fbe_job_service.number_creation_objects,
        (unsigned long long)job_element_p->job_number);

    /* After the element is added into the queue, we also need check the 
    * "remove flag" which indicates whether there is an incompleted remove
    * request. This is a fix to enhance the synchronization between 
    * "add" and "remove" operation. Wenxuan Yin: 2/22/2013
    */
    
    /* First, get remove action information */
    fbe_job_service_cmi_get_remove_job_action_on_block_in_progress(&element_remove_action);

    if(element_remove_action.flag == FBE_JOB_QUEUE_ELEMENT_REMOVE_FROM_CREATION_QUEUE)
    {
        /* If the element remove flag is set, we need remove the queue element.
        */
        job_service_trace(FBE_TRACE_LEVEL_INFO, 
                    FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                    "%s: received request to remove job from creation queue\n",
                    __FUNCTION__);

        remove_status = FBE_STATUS_NO_OBJECT;
        fbe_job_service_get_creation_queue_head(&head_p);
        request_p = (fbe_job_queue_element_t *) fbe_queue_front(head_p);
        while (request_p != NULL)
        {
            fbe_job_queue_element_t *next_request_p = NULL;

            if (request_p->job_number == element_remove_action.job_number)
            {
                job_service_trace(FBE_TRACE_LEVEL_INFO, 
                        FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                        "   job number %llu found in creation queue, at index %d\n", 
                        (unsigned long long)element_remove_action.job_number, job_index);
                
                /*Copy the data into temp buffer*/                
                fbe_copy_memory(&request_p_temp, request_p, sizeof(fbe_job_queue_element_t));
                fbe_queue_remove((fbe_queue_element_t *)request_p);
                fbe_job_service_decrement_number_creation_objects();
                if ((request_p->local_job_data_valid == FBE_TRUE) &&
                    (request_p->additional_job_data != NULL))
                {
                    fbe_memory_ex_release(request_p->additional_job_data);
                    request_p->additional_job_data = NULL;
                }
                fbe_transport_release_buffer(request_p);

                remove_status = FBE_STATUS_OK;
                break;
            }

            job_index += 1;
            next_request_p = (fbe_job_queue_element_t *)fbe_queue_next(head_p, &request_p->queue_element);
            request_p = next_request_p;
        }
        /* Clean up the remove job action */
        fbe_job_service_cmi_cleanup_remove_job_action_on_block_in_progress();
    }
    /* Unlock since we are done.
    */
    fbe_job_service_unlock();
    
    /* If an element is removed from queue, we need send a notification. */
    if(remove_status == FBE_STATUS_OK)
    {
        notification_info.notification_type = FBE_NOTIFICATION_TYPE_JOB_ACTION_STATE_CHANGED;
        notification_info.notification_data.job_service_error_info.object_id = element_remove_action.object_id;
        notification_info.notification_data.job_service_error_info.status = element_remove_action.status;
        notification_info.notification_data.job_service_error_info.error_code = element_remove_action.error_code;
        notification_info.notification_data.job_service_error_info.job_number = element_remove_action.job_number;
        notification_info.notification_data.job_service_error_info.job_type = element_remove_action.job_element.job_type; 

        notification_info.class_id = FBE_CLASS_ID_INVALID;
        notification_info.object_type = FBE_TOPOLOGY_OBJECT_TYPE_ALL;

        /* trace the notification. */
        job_service_trace(FBE_TRACE_LEVEL_INFO,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "JOB: Peer send notification. job: 0x%llx error: 0x%x\n",
                        element_remove_action.job_number, element_remove_action.error_code);

        /* trace the notification. */
        job_service_trace(FBE_TRACE_LEVEL_INFO,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "JOB: Peer send notification. obj: 0x%x object_type: 0x%llx type: 0x%llx\n",
                            element_remove_action.object_id, (unsigned long long)FBE_TOPOLOGY_OBJECT_TYPE_ALL, (unsigned long long)notification_info.notification_type);

        /* Send notification for the job number */
        notify_status = fbe_notification_send(element_remove_action.object_id, notification_info);
        if (notify_status != FBE_STATUS_OK) 
        {
            job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s: Send Job Notification failed, status: 0x%X\n",
                            __FUNCTION__, notify_status);
            return notify_status;
        }
    }

    /* Trace warning for NO object to be removed */
    if(remove_status == FBE_STATUS_NO_OBJECT)
    {
        job_service_trace(FBE_TRACE_LEVEL_WARNING,
                FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "%s: status %d, job %llu NOT found\n", 
                __FUNCTION__, remove_status, (unsigned long long)element_remove_action.job_number);
    }

    return FBE_STATUS_OK;
}
/*****************************************************
 * end fbe_job_service_add_element_to_creation_queue()
 ****************************************************/

/*!**************************************************************
 * fbe_job_service_find_object_in_recovery_queue()
 ****************************************************************
 * @brief
 * process a job control request to find an object in the 
 * recovery queue
 *
 * @param packet_p - packet containing object id of element to
 *                   see if it exists in queue          
 *
 * @return fbe_status_t - status of call
 *
 * @author
 * 10/01/2009 - Created. Jesus Medina
 *
 ****************************************************************/

fbe_status_t fbe_job_service_find_object_in_recovery_queue(fbe_packet_t * packet_p)
{
    fbe_job_queue_element_t                *job_queue_element_p = NULL;
    fbe_payload_control_operation_opcode_t control_code;
    fbe_payload_ex_t                          *payload_p = NULL;
    fbe_payload_control_operation_t        *control_operation = NULL;
    fbe_payload_control_buffer_length_t    length;
    fbe_queue_head_t                       *head_p = NULL;
    fbe_job_queue_element_t                *request_p = NULL;
    fbe_status_t                           status = FBE_STATUS_OK;
    fbe_payload_control_status_t           payload_status = FBE_PAYLOAD_CONTROL_STATUS_OK;

    /*! Get the request payload.
    */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload_p);
    control_code = fbe_transport_get_control_code(packet_p);

    fbe_payload_control_get_buffer(control_operation,
            &job_queue_element_p);
    if(job_queue_element_p == NULL)
    {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &length);
    if(length != sizeof(fbe_job_queue_element_t))
    {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = FBE_STATUS_NO_OBJECT;

    /*! Lock queue */
    fbe_job_service_lock();

    /* if queue is empty, return status no object
    */
    if (fbe_queue_is_empty(&fbe_job_service.recovery_q_head) == TRUE)
    {
        status = FBE_STATUS_NO_OBJECT;
        payload_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
    }
    else
    {
        fbe_job_service_get_recovery_queue_head(&head_p);
        request_p = (fbe_job_queue_element_t *) fbe_queue_front(head_p);
        while (request_p != NULL)
        {
            fbe_job_queue_element_t *next_request_p = NULL;

            if (request_p->object_id == job_queue_element_p->object_id)
            {
                job_service_trace(FBE_TRACE_LEVEL_INFO, 
                        FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                        "Found object id %d matching in recovery queue\n", 
                        job_queue_element_p->object_id);
                status = FBE_STATUS_OK;
                payload_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
                break;
            }
            next_request_p = (fbe_job_queue_element_t *)fbe_queue_next(head_p, &request_p->queue_element);
            request_p = next_request_p;
        }
    }

    /* unlock the queue
    */
    fbe_job_service_unlock();

    fbe_payload_control_set_status(control_operation, payload_status);
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);

    return status;
}
/****************************************************
 * end fbe_job_service_find_object_in_recovery_queue()
 ****************************************************/

/*!**************************************************************
 * fbe_job_service_find_element_by_job_number_in_recovery_queue()
 ****************************************************************
 * @brief
 * process a job control request to find an object in the 
 * recovery queue
 *
 * @param packet_p - packet containing object id of element to
 *                   see if it exists in queue          
 *
 * @return fbe_status_t - status of call
 *
 * @author
 * 10/18/2010 - Created. Jesus Medina
 *
 ****************************************************************/

fbe_status_t fbe_job_service_find_element_by_job_number_in_recovery_queue(fbe_packet_t * packet_p)
{
    fbe_job_queue_element_t                *job_queue_element_p = NULL;
    fbe_payload_control_operation_opcode_t control_code;
    fbe_payload_ex_t                          *payload_p = NULL;
    fbe_payload_control_operation_t        *control_operation = NULL;
    fbe_payload_control_buffer_length_t    length;
    fbe_queue_head_t                       *head_p = NULL;
    fbe_job_queue_element_t                *request_p = NULL;
    fbe_status_t                           status = FBE_STATUS_OK;
    fbe_payload_control_status_t           payload_status = FBE_PAYLOAD_CONTROL_STATUS_OK;

    /*! Get the request payload.
    */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload_p);
    control_code = fbe_transport_get_control_code(packet_p);

    fbe_payload_control_get_buffer(control_operation,
            &job_queue_element_p);
    if(job_queue_element_p == NULL)
    {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &length);
    if(length != sizeof(fbe_job_queue_element_t))
    {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = FBE_STATUS_NO_OBJECT;

    /*! Lock queue */
    fbe_job_service_lock();

    /* if queue is empty, return status no object
    */
    if (fbe_queue_is_empty(&fbe_job_service.recovery_q_head) == TRUE)
    {
        status = FBE_STATUS_NO_OBJECT;
        payload_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
    }
    else
    {
        fbe_job_service_get_recovery_queue_head(&head_p);
        request_p = (fbe_job_queue_element_t *) fbe_queue_front(head_p);
        while (request_p != NULL)
        {
            fbe_job_queue_element_t *next_request_p = NULL;

            if (request_p->job_number == job_queue_element_p->job_number)
            {
                job_service_trace(FBE_TRACE_LEVEL_INFO, 
                        FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                        "Found job number %llu matching in recovery queue\n", 
                        (unsigned long long)job_queue_element_p->job_number);
                status = FBE_STATUS_OK;
                payload_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
                break;
            }
            next_request_p = (fbe_job_queue_element_t *)fbe_queue_next(head_p, &request_p->queue_element);
            request_p = next_request_p;
        }
    }

    /* unlock the queue
    */
    fbe_job_service_unlock();

    fbe_payload_control_set_status(control_operation, payload_status);
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);

    return status;
}
/********************************************************************
 * end fbe_job_service_find_element_by_job_number_in_recovery_queue()
 *******************************************************************/

/*!**************************************************************
 * fbe_job_service_find_object_in_creation_queue()
 ****************************************************************
 * @brief
 * process a job control request to find an object in the 
 * creation queue
 *
 * @param packet_p - packet containing object id of element to
 *                   see if it exists in queue          
 *
 * @return fbe_status_t - status of call
 *
 * @author
 * 10/01/2009 - Created. Jesus Medina
 *
 ****************************************************************/

fbe_status_t fbe_job_service_find_object_in_creation_queue(fbe_packet_t * packet_p)
{
    fbe_job_queue_element_t                *job_queue_element_p = NULL;
    fbe_payload_control_operation_opcode_t control_code;
    fbe_payload_ex_t                          *payload_p = NULL;
    fbe_payload_control_operation_t        *control_operation = NULL;
    fbe_payload_control_buffer_length_t    length;
    fbe_queue_head_t                       *head_p = NULL;
    fbe_job_queue_element_t                *request_p = NULL;
    fbe_status_t                           status = FBE_STATUS_OK;
    fbe_payload_control_status_t           payload_status = FBE_PAYLOAD_CONTROL_STATUS_OK;

    /*! Get the request payload.
    */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload_p);
    control_code = fbe_transport_get_control_code(packet_p);

    fbe_payload_control_get_buffer(control_operation,
            &job_queue_element_p);
    if(job_queue_element_p == NULL)
    {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &length);
    if(length != sizeof(fbe_job_queue_element_t))
    {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = FBE_STATUS_NO_OBJECT;

    /*! Lock queue */
    fbe_job_service_lock();

    /* if queue is empty, return status no object
    */
    if (fbe_queue_is_empty(&fbe_job_service.creation_q_head) == TRUE)
    {
        status = FBE_STATUS_NO_OBJECT;
        payload_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
    }
    else
    {
        fbe_job_service_get_creation_queue_head(&head_p);
        request_p = (fbe_job_queue_element_t *) fbe_queue_front(head_p);
        while (request_p != NULL)
        {
            fbe_job_queue_element_t *next_request_p = NULL;

            if (request_p->object_id == job_queue_element_p->object_id)
            {
                job_service_trace(FBE_TRACE_LEVEL_INFO, 
                        FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                        "Found object id %d matching in creation queue\n", 
                        job_queue_element_p->object_id);
                status = FBE_STATUS_OK;
                payload_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
                break;
            }
            next_request_p = (fbe_job_queue_element_t *)fbe_queue_next(head_p, &request_p->queue_element);
            request_p = next_request_p;
        }
    }

    /* unlock the queue
    */
    fbe_job_service_unlock();

    fbe_payload_control_set_status(control_operation, payload_status);
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);

    return status;
}
/****************************************************
 * end fbe_job_service_find_object_in_creation_queue()
 ****************************************************/

/*!**************************************************************
 * fbe_job_service_find_element_by_job_number_in_creation_queue()
 ****************************************************************
 * @brief
 * process a job control request to find an object in the 
 * creation queue
 *
 * @param packet_p - packet containing object id of element to
 *                   see if it exists in queue          
 *
 * @return fbe_status_t - status of call
 *
 * @author
 * 10/21/2010 - Created. Jesus Medina
 *
 ****************************************************************/

fbe_status_t fbe_job_service_find_element_by_job_number_in_creation_queue(fbe_packet_t * packet_p)
{
    fbe_job_queue_element_t                *job_queue_element_p = NULL;
    fbe_payload_control_operation_opcode_t control_code;
    fbe_payload_ex_t                          *payload_p = NULL;
    fbe_payload_control_operation_t        *control_operation = NULL;
    fbe_payload_control_buffer_length_t    length;
    fbe_queue_head_t                       *head_p = NULL;
    fbe_job_queue_element_t                *request_p = NULL;
    fbe_status_t                           status = FBE_STATUS_OK;
    fbe_payload_control_status_t           payload_status = FBE_PAYLOAD_CONTROL_STATUS_OK;

    /*! Get the request payload.
    */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload_p);
    control_code = fbe_transport_get_control_code(packet_p);

    fbe_payload_control_get_buffer(control_operation,
            &job_queue_element_p);
    if(job_queue_element_p == NULL)
    {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &length);
    if(length != sizeof(fbe_job_queue_element_t))
    {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = FBE_STATUS_NO_OBJECT;

    /*! Lock queue */
    fbe_job_service_lock();

    /* if queue is empty, return status no object
    */
    if (fbe_queue_is_empty(&fbe_job_service.creation_q_head) == TRUE)
    {
        status = FBE_STATUS_NO_OBJECT;
        payload_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
    }
    else
    {
        fbe_job_service_get_creation_queue_head(&head_p);
        request_p = (fbe_job_queue_element_t *) fbe_queue_front(head_p);
        while (request_p != NULL)
        {
            fbe_job_queue_element_t *next_request_p = NULL;

            if (request_p->object_id == job_queue_element_p->object_id)
            {
                job_service_trace(FBE_TRACE_LEVEL_INFO, 
                        FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                        "Found job number %llu matching in creation queue\n", 
                        (unsigned long long)job_queue_element_p->job_number);
                status = FBE_STATUS_OK;
                payload_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
                break;
            }
            next_request_p = (fbe_job_queue_element_t *)fbe_queue_next(head_p, &request_p->queue_element);
            request_p = next_request_p;
        }
    }

    /* unlock the queue
    */
    fbe_job_service_unlock();

    fbe_payload_control_set_status(control_operation, payload_status);
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);

    return status;
}
/********************************************************************
 * end fbe_job_service_find_element_by_job_number_in_creation_queue()
 *******************************************************************/


/*!************************************************************
 * fbe_job_service_find_and_remove_by_object_id_from_recovery_queue()
 *************************************************************
 * @brief
 * process a job control request to find an object in the
 * recovery queue, and if found, remove object from queue
 *
 * @param packet_p - packet containing object id to be removed
 *                   from queue          
 *
 * @return fbe_status_t - FBE_STATUS_OK 
 *
 * @author
 * 10/01/2009 - Created. Jesus Medina
 *
 ****************************************************************/

fbe_status_t fbe_job_service_find_and_remove_by_object_id_from_recovery_queue(fbe_packet_t * packet_p)
{
    fbe_job_queue_element_t                *job_queue_element_p = NULL;
    fbe_payload_control_operation_opcode_t control_code;
    fbe_payload_ex_t                          *payload_p = NULL;
    fbe_payload_control_operation_t        *control_operation = NULL;
    fbe_payload_control_buffer_length_t    length;
    fbe_status_t                           status = FBE_STATUS_OK;
    fbe_queue_head_t                       *head_p = NULL;
    fbe_job_queue_element_t                *request_p = NULL;
    fbe_object_id_t                        object_id = FBE_OBJECT_ID_INVALID; 
    fbe_payload_control_status_t           payload_status = FBE_PAYLOAD_CONTROL_STATUS_OK;

    /*! Get the request payload.
    */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload_p);
    control_code = fbe_transport_get_control_code(packet_p);

    fbe_payload_control_get_buffer(control_operation,
            &job_queue_element_p);
    if(job_queue_element_p == NULL)
    {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &length);
    if(length != sizeof(fbe_job_queue_element_t))
    {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* set status to no device, which will be changed if object is found in queue 
    */
    status = FBE_STATUS_NO_OBJECT;
    object_id = job_queue_element_p->object_id;

    /*! Lock queue */
    fbe_job_service_lock();

    /* if queue is empty, return status no object
    */
    if (fbe_queue_is_empty(&fbe_job_service.recovery_q_head) == TRUE)
    {
        status = FBE_STATUS_NO_OBJECT;
        payload_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
    }
    else
    {        
        fbe_job_service_get_recovery_queue_head(&head_p);
        request_p = (fbe_job_queue_element_t *) fbe_queue_front(head_p);
        while (request_p != NULL)
        {
            fbe_job_queue_element_t *next_request_p = NULL;
            if (request_p->object_id == object_id)
            {
                job_service_trace(FBE_TRACE_LEVEL_INFO, 
                        FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                        "object id %d found in recovery queue\n", 
                        object_id);

                fbe_queue_remove((fbe_queue_element_t *)request_p);
                fbe_job_service_decrement_number_recovery_objects();
                if ((request_p->local_job_data_valid == FBE_TRUE) &&
                    (request_p->additional_job_data != NULL))
                {
                    fbe_memory_ex_release(request_p->additional_job_data);
                    request_p->additional_job_data = NULL;
                }
                fbe_transport_release_buffer(request_p);

                status = FBE_STATUS_OK;
                payload_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
                break;
            }
            next_request_p = (fbe_job_queue_element_t *)fbe_queue_next(head_p, &request_p->queue_element);
            request_p = next_request_p;
        }
    }
    /* unlock the queue
    */
    fbe_job_service_unlock();

    fbe_payload_control_set_status(control_operation, payload_status);
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);

    return FBE_STATUS_OK;
}
/************************************************************************
 * end fbe_job_service_find_and_remove_by_object_id_from_recovery_queue()
 ***********************************************************************/

/*!***************************************************************************
 *          fbe_job_service_process_peer_recovery_queue_completion()
 *****************************************************************************
 *
 * @brief   Process a peer recovery queue completion.  Currently we must take
 *          the following actions:
 *              o Generate the proper notification.
 *
 * @param   in_status - The status of the operation from the peer 
 * @param   job_number - The job number from the peer.  This is used to locate
 *              the local job request.
 * @param   object_id - The object id associated with this job from the peer.
 * @param   error_code - The error code from the peer job execution
 * @param   class_id - The class identifier associated with this job.   
 * @param   job_element_p - Pointer to job element information from other side
 *
 * @return fbe_status_t - 
 *      FBE_STATUS_OK: element found
 *      FBE_STATUS_NO_OBJECT: element not found 
 *      FBE_STATUS_MORE_PROCESSING_REQUIRED: element needs deferred remove operation
 *
 * @author
 *  09/22/2010 - Created. Jesus Medina
 *  08/27/2012  Ron Proulx  - Updated to generate proper notification.
 *
 *****************************************************************************/
fbe_status_t fbe_job_service_process_peer_recovery_queue_completion(fbe_status_t in_status,     
                                                                    fbe_u64_t job_number, 
                                                                    fbe_object_id_t object_id,  
                                                                    fbe_job_service_error_type_t error_code,
                                                                    fbe_class_id_t class_id,
                                                                    fbe_job_queue_element_t *in_job_element_p)
{
    fbe_status_t                status = FBE_STATUS_NO_OBJECT;  /*! @note Default is `job not found' */
    fbe_queue_head_t           *head_p = NULL;
    fbe_job_queue_element_t    *request_p = NULL;
    fbe_job_queue_element_t     copy_of_request;
    fbe_u32_t                   job_index = 0;

    /*! Lock queue */
    fbe_job_service_lock();

    /* if queue is empty, return status no object
    */
    if (fbe_queue_is_empty(&fbe_job_service.recovery_q_head) == TRUE)
    {
        status = FBE_STATUS_NO_OBJECT;
    }
    else
    {        
        fbe_job_service_get_recovery_queue_head(&head_p);
        request_p = (fbe_job_queue_element_t *) fbe_queue_front(head_p);
        while (request_p != NULL)
        {
            fbe_job_queue_element_t *next_request_p = NULL;
            if (request_p->job_number == job_number)
            {
                job_service_trace(FBE_TRACE_LEVEL_INFO, 
                        FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                        "job  number %llu found in recovery queue, at index %d\n", 
                        (unsigned long long)job_number, job_index);
                
                /*Copy the data into temp buffer and log the event after releasing the lock*/                
                fbe_copy_memory(&copy_of_request, request_p, sizeof(fbe_job_queue_element_t));
                fbe_queue_remove((fbe_queue_element_t *)request_p);
                fbe_job_service_decrement_number_recovery_objects();
                if ((request_p->local_job_data_valid == FBE_TRUE) &&
                    (request_p->additional_job_data != NULL))
                {
                    fbe_memory_ex_release(request_p->additional_job_data);
                    request_p->additional_job_data = NULL;
                }
                fbe_transport_release_buffer(request_p);

                status = FBE_STATUS_OK;
                break;
            }

            job_index += 1;
            next_request_p = (fbe_job_queue_element_t *)fbe_queue_next(head_p, &request_p->queue_element);
            request_p = next_request_p;
        }
    }
    
    if(status == FBE_STATUS_NO_OBJECT)
    {
        /* If we cannot find the element to be removed in the queue, we must check
        * whether the block_in_progress state is COMPLETED. If it says COMPLETED, 
        * that means the passive side has not finish adding the element into the 
        * queue. We have to set the remove flag and job number then the 
        * arrival thread can finish following process. Wenxuan Yin: 2/20/2013
       */
        if(fbe_job_service_cmi_get_state() == FBE_JOB_SERVICE_CMI_STATE_COMPLETED)
        {
            fbe_job_service_cmi_set_remove_job_action_on_block_in_progress(
                                        FBE_JOB_QUEUE_ELEMENT_REMOVE_FROM_RECOVERY_QUEUE, 
                                        job_number, object_id, in_status, error_code,
                                        class_id, in_job_element_p);

            status = FBE_STATUS_MORE_PROCESSING_REQUIRED;
        }
    }
    /* unlock the queue
     */
    fbe_job_service_unlock();

    /* If we located the job that completed on the peer process the completion.
     */
    if (status == FBE_STATUS_OK)
    {
        /*! @note Some recovery job types have no object associated with them:
         *          o FBE_JOB_TYPE_UPDATE_SPARE_LIBRARY_CONFIG
         */
        if (in_job_element_p->job_type == FBE_JOB_TYPE_UPDATE_SPARE_LIBRARY_CONFIG)
        {
            /* Return success.
             */
            return FBE_STATUS_OK;
        }

        /* First copy any fields that were generated on the peer to the 
         * temporary (i.e. on the stack) request structure.
         */
        copy_of_request.class_id = class_id;
        copy_of_request.object_id = object_id;
        copy_of_request.status = in_status;
        copy_of_request.error_code = error_code;
        copy_of_request.job_number = in_job_element_p->job_number;
        copy_of_request.job_type = in_job_element_p->job_type;
        copy_of_request.queue_type = in_job_element_p->queue_type;
        copy_of_request.command_size = in_job_element_p->command_size;

        fbe_copy_memory(&copy_of_request.command_data, &in_job_element_p->command_data, in_job_element_p->command_size);

        /* Now generate and send the appropriate notification.
         */
        status = fbe_job_service_generate_and_send_notification(&copy_of_request);
        if (status != FBE_STATUS_OK)
        {
            /* Generate a warning.
             */
            job_service_trace(FBE_TRACE_LEVEL_WARNING, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Failed to send notification for job: 0x%llx \n",
                              __FUNCTION__, (unsigned long long)job_number);
        }
        
    }
    else
    {
        /* Else if the local job was not found.  Since there will always be a
         * case where this SP is just coming up as the job completes on the peer
         * only report a warning (not error).
         */
        job_service_trace(FBE_TRACE_LEVEL_WARNING, 
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s Failed to locate job: 0x%llx \n",
                          __FUNCTION__, (unsigned long long)job_number);
    }

    return status;
}
/*************************************************************************
 * end fbe_job_service_process_peer_recovery_queue_completion()
 ************************************************************************/

/*!******************************************************************
 * fbe_job_service_find_and_remove_by_object_id_from_creation_queue()
 ********************************************************************
 * @brief
 * process a job control request to find an object in the
 * creation queue, and if found, remove object from queue
 *
 * @param packet_p - packet containing object id to be removed
 *                   from queue          
 *
 * @return fbe_status_t - FBE_STATUS_OK 
 *
 * @author
 * 10/01/2009 - Created. Jesus Medina
 *
 *********************************************************************/

fbe_status_t fbe_job_service_find_and_remove_by_object_id_from_creation_queue(fbe_packet_t * packet_p)
{
    fbe_job_queue_element_t                *job_queue_element_p = NULL;
    fbe_payload_control_operation_opcode_t control_code;
    fbe_payload_ex_t                          *payload_p = NULL;
    fbe_payload_control_operation_t        *control_operation = NULL;
    fbe_payload_control_buffer_length_t    length;
    fbe_status_t                           status = FBE_STATUS_OK;
    fbe_queue_head_t                       *head_p = NULL;
    fbe_job_queue_element_t                *request_p = NULL;
    fbe_object_id_t                        object_id = FBE_OBJECT_ID_INVALID; 
    fbe_payload_control_status_t           payload_status = FBE_PAYLOAD_CONTROL_STATUS_OK;

    /*! Get the request payload.
    */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload_p);
    control_code = fbe_transport_get_control_code(packet_p);

    fbe_payload_control_get_buffer(control_operation,
            &job_queue_element_p);
    if(job_queue_element_p == NULL)
    {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &length);
    if(length != sizeof(fbe_job_queue_element_t))
    {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* set status to no device, which will be changed if object is found in queue 
    */
    status = FBE_STATUS_NO_OBJECT;
    object_id = job_queue_element_p->object_id;

    /*! Lock queue */
    fbe_job_service_lock();

    /* if queue is empty, return status no object
    */
    if (fbe_queue_is_empty(&fbe_job_service.creation_q_head) == TRUE)
    {
        status = FBE_STATUS_NO_OBJECT;
        payload_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
    }
    else
    {
        fbe_job_service_get_creation_queue_head(&head_p);
        request_p = (fbe_job_queue_element_t *) fbe_queue_front(head_p);
        while (request_p != NULL)
        {
            fbe_job_queue_element_t *next_request_p = NULL;

            if (request_p->object_id == object_id)
            {
                job_service_trace(FBE_TRACE_LEVEL_INFO, 
                        FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                        "object id %d found in creation queue\n", 
                        object_id);

                fbe_queue_remove((fbe_queue_element_t *)request_p);
                fbe_job_service_decrement_number_creation_objects();
                if ((request_p->local_job_data_valid == FBE_TRUE) &&
                    (request_p->additional_job_data != NULL))
                {
                    fbe_memory_ex_release(request_p->additional_job_data);
                    request_p->additional_job_data = NULL;
                }
                fbe_transport_release_buffer(request_p);

                status = FBE_STATUS_OK;
                payload_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
                break;
            }
            next_request_p = (fbe_job_queue_element_t *)fbe_queue_next(head_p, &request_p->queue_element);
            request_p = next_request_p;
        }
    }
    /* unlock the queue
    */
    fbe_job_service_unlock();

    fbe_payload_control_set_status(control_operation, payload_status);
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);

    return FBE_STATUS_OK;
}
/************************************************************************
 * end fbe_job_service_find_and_remove_by_object_id_from_creation_queue()
 ***********************************************************************/

/*!*******************************************************************
 * fbe_job_service_find_and_remove_element_by_job_number_from_creation_queue()
 *********************************************************************
 * @brief
 * process a request to find queue element in the
 * creation queue by job number, and if found, removes object 
 * from queue
 *
 * @param   in_status - The status of the operation from the peer 
 * @param   job_number - The job number from the peer.  This is used to locate
 *              the local job request.
 * @param   object_id - The object id associated with this job from the peer.
 * @param   error_code - The error code from the peer job execution
 * @param   class_id - The class identifier associated with this job.   
 * @param   job_element_p - Pointer to job element information from other side
 *
 * @return fbe_status_t - 
 *      FBE_STATUS_OK: element found
 *      FBE_STATUS_NO_OBJECT: element not found 
 *      FBE_STATUS_MORE_PROCESSING_REQUIRED: element needs deferred remove operation
 *
 * @author
 * 09/22/2010 - Created. Jesus Medina
 *
 *********************************************************************/

fbe_status_t fbe_job_service_find_and_remove_element_by_job_number_from_creation_queue(
                                                                fbe_status_t in_status,     
                                                                fbe_u64_t job_number, 
                                                                fbe_object_id_t object_id,  
                                                                fbe_job_service_error_type_t error_code,
                                                                fbe_class_id_t class_id,
                                                                fbe_job_queue_element_t *in_job_element_p)
{
    fbe_status_t                           status = FBE_STATUS_NO_OBJECT;
    fbe_queue_head_t                       *head_p = NULL;
    fbe_job_queue_element_t                *request_p = NULL;
    fbe_job_queue_element_t                request_p_temp;
    fbe_u32_t                              job_index = 0;

    /*! Lock queue */
    fbe_job_service_lock();

    /* if queue is empty, return status no object
    */
    if (fbe_queue_is_empty(&fbe_job_service.creation_q_head) == TRUE)
    {
        status = FBE_STATUS_NO_OBJECT;
    }
    else
    {
        fbe_job_service_get_creation_queue_head(&head_p);
        request_p = (fbe_job_queue_element_t *) fbe_queue_front(head_p);
        while (request_p != NULL)
        {
            fbe_job_queue_element_t *next_request_p = NULL;

            if (request_p->job_number == job_number)
            {
                job_service_trace(FBE_TRACE_LEVEL_INFO, 
                        FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                        "job number %llu found in creation queue, at index %d\n", 
                        (unsigned long long)job_number, job_index);
                
                /*Copy the data into temp buffer and log the event after releasing the lock*/                
                fbe_copy_memory(&request_p_temp, request_p, sizeof(fbe_job_queue_element_t));
                fbe_queue_remove((fbe_queue_element_t *)request_p);
                fbe_job_service_decrement_number_creation_objects();
                if ((request_p->local_job_data_valid == FBE_TRUE) &&
                    (request_p->additional_job_data != NULL))
                {
                    fbe_memory_ex_release(request_p->additional_job_data);
                    request_p->additional_job_data = NULL;
                }
                fbe_transport_release_buffer(request_p);

                status = FBE_STATUS_OK;
                break;
            }

            job_index += 1;
            next_request_p = (fbe_job_queue_element_t *)fbe_queue_next(head_p, &request_p->queue_element);
            request_p = next_request_p;
        }
    }

    if(status == FBE_STATUS_NO_OBJECT)
    {
        /* If we cannot find the element to be removed in the queue, we must check
        * whether the block_in_progress state is COMPLETED. If it says COMPLETED, 
        * that means the passive side has not finish adding the element into the 
        * queue. We have to set the remove flag and job number then the 
        * arrival thread can finish following process. Wenxuan Yin: 2/20/2013
       */
        if(fbe_job_service_cmi_get_state() == FBE_JOB_SERVICE_CMI_STATE_COMPLETED)
        {
            fbe_job_service_cmi_set_remove_job_action_on_block_in_progress(
                                        FBE_JOB_QUEUE_ELEMENT_REMOVE_FROM_CREATION_QUEUE, 
                                        job_number, object_id, in_status, error_code,
                                        class_id, in_job_element_p);

            status = FBE_STATUS_MORE_PROCESSING_REQUIRED;
        }
        /* Otherwise, indeed there is no element to be removed. This case occurs if
        * the "add" and "remove" CMI message messes up the sequence.
        */
    }
    /* unlock the queue
    */
    fbe_job_service_unlock();

    if (status == FBE_STATUS_OK)
    {
        /* fbe_job_service_write_to_event_log will cause SP to panic while waiting for DPC to process.
         * This is not correct.  We should not be accessing the lock at all.  Commenting this out for now.  
         * This should be in database servivce. KMai: 03-05-12. 
        fbe_job_service_write_to_event_log(&request_p_temp);
        */
        job_service_trace(FBE_TRACE_LEVEL_INFO, 
                FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "job number %llu found in creation queue. Should call write to Event Log.\n", 
                (unsigned long long)job_number);
    }

    return status;
}
/********************************************************************************
 * end fbe_job_service_find_and_remove_element_by_job_number_from_creation_queue()
 ********************************************************************************/

/*!**************************************************************
 * fbe_job_service_get_number_of_elements_in_recovery_queue()
 ****************************************************************
 * @brief
 * process a job control request to get the number of objects in
 * the recovery queue
 *
 * @param packet_p - packet containing value to get
 *                   number of elements in queue             
 *
 * @return fbe_status_t - FBE_STATUS_OK 
 *
 * @author
 * 11/10/2009 - Created. Jesus Medina
 *
 ****************************************************************/

fbe_status_t fbe_job_service_get_number_of_elements_in_recovery_queue(fbe_packet_t * packet_p)
{
    fbe_job_queue_element_t              *job_queue_element_p = NULL;
    fbe_payload_control_operation_opcode_t control_code;
    fbe_payload_ex_t                          *payload_p = NULL;
    fbe_payload_control_operation_t        *control_operation = NULL;
    fbe_payload_control_buffer_length_t    length;
    fbe_status_t                           status = FBE_STATUS_OK;


    job_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry\n", __FUNCTION__);

    /*! Get the request payload.
    */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload_p);
    control_code = fbe_transport_get_control_code(packet_p);

    fbe_payload_control_get_buffer(control_operation,
            &job_queue_element_p);
    if(job_queue_element_p == NULL)
    {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &length);
    if(length != sizeof(fbe_job_queue_element_t))
    {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_job_service_lock();
    job_queue_element_p->num_objects = fbe_job_service_get_number_recovery_requests();
    fbe_job_service_unlock();


    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);

    return FBE_STATUS_OK;
}
/****************************************************************
 * end fbe_job_service_get_number_of_elements_in_recovery_queue()
 ***************************************************************/

/*!**************************************************************
 * fbe_job_service_get_number_of_elements_in_creation_queue()
 ****************************************************************
 * @brief
 * process a job control request to get the number of objects in
 * the creation queue
 *
 * @param packet_p - packet containing value to get
 *                   number of elements in queue             
 *
 * @return fbe_status_t - FBE_STATUS_OK 
 *
 * @author
 * 11/10/2009 - Created. Jesus Medina
 *
 ****************************************************************/

fbe_status_t fbe_job_service_get_number_of_elements_in_creation_queue(fbe_packet_t * packet_p)
{
    fbe_job_queue_element_t              *job_queue_element_p = NULL;
    fbe_payload_control_operation_opcode_t control_code;
    fbe_payload_ex_t                          *payload_p = NULL;
    fbe_payload_control_operation_t        *control_operation = NULL;
    fbe_payload_control_buffer_length_t    length;
    fbe_status_t                           status = FBE_STATUS_OK;

    job_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry\n", __FUNCTION__);

    /*! Get the request payload.
    */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload_p);
    control_code = fbe_transport_get_control_code(packet_p);

    fbe_payload_control_get_buffer(control_operation,
            &job_queue_element_p);
    if(job_queue_element_p == NULL)
    {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &length);
    if(length != sizeof(fbe_job_queue_element_t))
    {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_job_service_lock();
    job_queue_element_p->num_objects = fbe_job_service_get_number_creation_requests();
    fbe_job_service_unlock();

    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);

    return FBE_STATUS_OK;
}
/****************************************************************
 * end fbe_job_service_get_number_of_elements_in_creation_queue()
 ***************************************************************/

/*!**************************************************************
 * fbe_job_service_print_objects_from_recovery_queue()
 ****************************************************************
 * @brief
 * Prints all objects from recovery queue
 * 
 * @param - none
 *
 * @return - none
 *
 * @author
 *  12/31/2009 - Created. Jesus Medina 
 *
 ****************************************************************/

void fbe_job_service_print_objects_from_recovery_queue(fbe_u32_t caller)
{
    fbe_queue_head_t *head_p = NULL;
    fbe_job_queue_element_t *request_p = NULL;

    /*! Lock queue */
    fbe_job_service_lock();

    fbe_job_service_get_recovery_queue_head(&head_p);
    request_p = (fbe_job_queue_element_t *) fbe_queue_front(head_p);
    while (request_p != NULL)
    {
        fbe_job_queue_element_t *next_request_p = NULL;
        job_service_trace(FBE_TRACE_LEVEL_INFO, 
                FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "recovery queue--Lock(%d)::object id %d \n", 
                caller,
                request_p->object_id);
        next_request_p = (fbe_job_queue_element_t *)fbe_queue_next(head_p, &request_p->queue_element);
        request_p = next_request_p;
    }

    fbe_job_service_unlock();
}
/*********************************************************
 * end fbe_job_service_print_objects_from_recovery_queue()
 ********************************************************/

/*!**************************************************************
 * fbe_job_service_print_objects_from_creation_queue()
 ****************************************************************
 * @brief
 * Prints all objects from creation queue
 * 
 * @param - none
 *
 * @return - none
 *
 * @author
 *  12/31/2009 - Created. Jesus Medina 
 *
 ****************************************************************/

void fbe_job_service_print_objects_from_creation_queue(fbe_u32_t caller)
{
    fbe_queue_head_t *head_p = NULL;
    fbe_job_queue_element_t *request_p = NULL;

    /*! Lock queue */
    fbe_job_service_lock();

    fbe_job_service_get_creation_queue_head(&head_p);
    request_p = (fbe_job_queue_element_t *) fbe_queue_front(head_p);
    while (request_p != NULL)
    {
        fbe_job_queue_element_t *next_request_p = NULL;
        job_service_trace(FBE_TRACE_LEVEL_INFO, 
                FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "recovery queue--Lock(%d)::object id %d \n", 
                caller,
                request_p->object_id);
        next_request_p = (fbe_job_queue_element_t *)fbe_queue_next(head_p, &request_p->queue_element);
        request_p = next_request_p;
    }

    fbe_job_service_unlock();
}
/*********************************************************
 * end fbe_job_service_print_objects_from_creation_queue()
 ********************************************************/

/*!**************************************************************
 * fbe_job_service_print_objects_from_recovery_queue_no_lock()
 ****************************************************************
 * @brief
 * Prints all objects from recovery queue, this function uses no
 * locks to access the queue because caller has the lock set
 * already when calling this function
 * 
 * @param - none
 *
 * @return - none
 *
 * @author
 *  12/31/2009 - Created. Jesus Medina 
 *
 ****************************************************************/

void fbe_job_service_print_objects_from_recovery_queue_no_lock(fbe_u32_t caller)
{
    fbe_queue_head_t *head_p = NULL;
    fbe_job_queue_element_t *request_p = NULL;

    fbe_job_service_get_recovery_queue_head(&head_p);
    request_p = (fbe_job_queue_element_t *) fbe_queue_front(head_p);
    while (request_p != NULL)
    {
        fbe_job_queue_element_t *next_request_p = NULL;
        job_service_trace(FBE_TRACE_LEVEL_INFO, 
                FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "recovery queue--NoLock(%d)::object id %d \n", 
                caller, 
                request_p->object_id);
        next_request_p = (fbe_job_queue_element_t *)fbe_queue_next(head_p, &request_p->queue_element);
        request_p = next_request_p;
    }
}
/****************************************************************
 * end fbe_job_service_print_objects_from_recovery_queue_no_lock()
 ****************************************************************/

/*!**************************************************************
 * fbe_job_service_print_objects_from_creation_queue_no_lock()
 ****************************************************************
 * @brief
 * Prints all objects from creation queue, this function uses no
 * locks to access the queue because caller has the lock set
 * already when calling this function
 * 
 * @param - none
 *
 * @return - none
 *
 * @author
 *  12/31/2009 - Created. Jesus Medina 
 *
 ****************************************************************/

void fbe_job_service_print_objects_from_creation_queue_no_lock(fbe_u32_t caller)
{
    fbe_queue_head_t *head_p = NULL;
    fbe_job_queue_element_t *request_p = NULL;

    fbe_job_service_get_creation_queue_head(&head_p);
    request_p = (fbe_job_queue_element_t *) fbe_queue_front(head_p);
    while (request_p != NULL)
    {
        fbe_job_queue_element_t *next_request_p = NULL;
        job_service_trace(FBE_TRACE_LEVEL_INFO, 
                FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "recovery queue--NoLock(%d)::object id %d \n", 
                caller, 
                request_p->object_id);
        next_request_p = (fbe_job_queue_element_t *)fbe_queue_next(head_p, &request_p->queue_element);
        request_p = next_request_p;
    }
}
/****************************************************************
 * end fbe_job_service_print_objects_from_creation_queue_no_lock()
 ****************************************************************/

/*!**************************************************************
 * fbe_job_service_remove_all_objects_from_recovery_queue()
 ****************************************************************
 * @brief
 *  Removes all elements from queue
 *
 * @param none  
 *
 * @return none
 *
 * @author
 *  10/29/2009 - Created. Jesus Medina 
 *
 ****************************************************************/

void fbe_job_service_remove_all_objects_from_recovery_queue()
{
    fbe_queue_element_t       *queue_element_p = NULL;
    fbe_job_queue_element_t *request_p = NULL;
    fbe_u32_t number_recovery_objects = 0;

    number_recovery_objects = fbe_job_service_get_number_recovery_requests();

    while (fbe_queue_is_empty(&fbe_job_service.recovery_q_head) == FBE_FALSE)
    {
        queue_element_p = fbe_queue_front(&fbe_job_service.recovery_q_head);
        fbe_queue_remove((fbe_queue_element_t *)queue_element_p);
        fbe_job_service_decrement_number_recovery_objects();
        number_recovery_objects = number_recovery_objects - 1;

        request_p = (fbe_job_queue_element_t *) queue_element_p;

        job_service_trace(FBE_TRACE_LEVEL_INFO, 
                FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "removing object with job #%llu from recovery queue\n",
        (unsigned long long)request_p->job_number);
       job_service_trace(FBE_TRACE_LEVEL_INFO, 
                FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "outstanding object count %d\n", number_recovery_objects);
       if ((request_p->local_job_data_valid == FBE_TRUE) &&
           (request_p->additional_job_data != NULL))
       {
           fbe_memory_ex_release(request_p->additional_job_data);
           request_p->additional_job_data = NULL;
       }
       fbe_transport_release_buffer(request_p);
    }

    return;
}
/**************************************************************
 * end fbe_job_service_remove_all_objects_from_recovery_queue()
 *************************************************************/

/*!**************************************************************
 * fbe_job_service_remove_all_objects_from_creation_queue()
 ****************************************************************
 * @brief
 *  Removes all elements from queue
 *
 * @param none  
 *
 * @return none
 *
 * @author
 *  09/22/2010 - Created. Jesus Medina 
 *
 ****************************************************************/

void fbe_job_service_remove_all_objects_from_creation_queue()
{
    fbe_queue_element_t     *queue_element_p = NULL;
    fbe_job_queue_element_t *request_p = NULL;
    fbe_u32_t number_creation_objects = 0;

    number_creation_objects = fbe_job_service_get_number_creation_requests();
    job_service_trace(FBE_TRACE_LEVEL_INFO, 
        FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
        "outstanding object count %d\n", number_creation_objects);

    while (fbe_queue_is_empty(&fbe_job_service.creation_q_head) == FBE_FALSE)
    {
        queue_element_p = fbe_queue_front(&fbe_job_service.creation_q_head);
        fbe_queue_remove((fbe_queue_element_t *)queue_element_p);
        fbe_job_service_decrement_number_creation_objects();
        number_creation_objects = number_creation_objects - 1;

        request_p = (fbe_job_queue_element_t *) queue_element_p;

        job_service_trace(FBE_TRACE_LEVEL_INFO, 
                FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "removing object with job #%llu from creation queue\n",
        (unsigned long long)request_p->job_number);
        job_service_trace(FBE_TRACE_LEVEL_INFO, 
                FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "outstanding object count %d\n", number_creation_objects);
        if ((request_p->local_job_data_valid == FBE_TRUE) &&
            (request_p->additional_job_data != NULL))
        {
            fbe_memory_ex_release(request_p->additional_job_data);
            request_p->additional_job_data = NULL;
        }
        fbe_transport_release_buffer(request_p);
    }

    return;
}
/*************************************************************
 * end fbe_job_service_remove_all_objects_from_creation_queue()
 *************************************************************/

/*!**************************************************************
 * fbe_job_service_process_recovery_queue_access()
 ****************************************************************
 * @brief
 * process a job control request to disable/enable processing
 * of job elements from the recovery queue
 *
 * @param packet_p - packet containing enable/disable request.             
 *
 * @return fbe_status_t - Status of the processing of the request
 *                        to either disable or enable the access
 *                        to the recovery queue
 * @author
 * 10/01/2009 - Created. Jesus Medina
 *
 ****************************************************************/

fbe_status_t fbe_job_service_process_recovery_queue_access(fbe_packet_t * packet_p)
{
    fbe_job_queue_element_t                *job_queue_element_p = NULL;
    fbe_payload_ex_t                          *payload_p = NULL;
    fbe_payload_control_operation_opcode_t control_code;
    fbe_payload_control_operation_t        *control_operation = NULL;
    fbe_payload_control_buffer_length_t    length;
    fbe_status_t                           status = FBE_STATUS_OK;
    fbe_payload_control_status_t           payload_status = FBE_PAYLOAD_CONTROL_STATUS_OK;

    job_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry\n", __FUNCTION__);

    /*! Get the request payload.
    */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload_p);
    control_code = fbe_transport_get_control_code(packet_p);

    fbe_payload_control_get_buffer(control_operation,
            &job_queue_element_p);
    if(job_queue_element_p == NULL)
    {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &length);
    if(length != sizeof(fbe_job_queue_element_t))
    {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*! Lock queue */
    fbe_job_service_lock();
    if (fbe_job_service_get_recovery_queue_access() != job_queue_element_p->queue_access)
    {
        payload_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
        fbe_job_service_set_recovery_queue_access(job_queue_element_p->queue_access);
        if(FBE_TRUE == job_queue_element_p->queue_access && fbe_job_service_cmi_is_active())
        {
            fbe_semaphore_release(&fbe_job_service.job_control_queue_semaphore, 0, 1, FALSE);
        }
    }
    /*! Unlock queue */
    fbe_job_service_unlock();

    fbe_payload_control_set_status(control_operation, payload_status);
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}
/*****************************************************
 * end fbe_job_service_process_recovery_queue_access()
 *****************************************************/

/*!**************************************************************
 * fbe_job_service_process_creation_queue_access()
 ****************************************************************
 * @brief
 * process a job control request to disable/enable processing
 * of job elements from the creation queue
 *
 * @param packet_p - packet containing enable/disable request.             
 *
 * @return fbe_status_t - Status of the processing of the request
 *                        to either disable or enable the access
 *                        to the recovery queue
 * @author
 * 10/01/2009 - Created. Jesus Medina
 *
 ****************************************************************/

fbe_status_t fbe_job_service_process_creation_queue_access(fbe_packet_t * packet_p)
{
    fbe_job_queue_element_t                *job_queue_element_p = NULL;
    fbe_payload_ex_t                          *payload_p = NULL;
    fbe_payload_control_operation_opcode_t control_code;
    fbe_payload_control_operation_t        *control_operation = NULL;
    fbe_payload_control_buffer_length_t    length;
    fbe_status_t                           status = FBE_STATUS_OK;
    fbe_payload_control_status_t           payload_status = FBE_PAYLOAD_CONTROL_STATUS_OK;

    job_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry\n", __FUNCTION__);

    /*! Get the request payload.
    */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload_p);
    control_code = fbe_transport_get_control_code(packet_p);

    fbe_payload_control_get_buffer(control_operation,
            &job_queue_element_p);
    if(job_queue_element_p == NULL)
    {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &length);
    if(length != sizeof(fbe_job_queue_element_t))
    {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*! Lock queue */
    fbe_job_service_lock();
    if (fbe_job_service_get_creation_queue_access() != job_queue_element_p->queue_access)
    {
        payload_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
        fbe_job_service_set_creation_queue_access(job_queue_element_p->queue_access);
        if(FBE_TRUE == job_queue_element_p->queue_access && fbe_job_service_cmi_is_active())
        {
            fbe_semaphore_release(&fbe_job_service.job_control_queue_semaphore, 0, 1, FALSE);
        }
    }
    /*! Unlock queue */
    fbe_job_service_unlock();

    fbe_payload_control_set_status(control_operation, payload_status);
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}
/*****************************************************
 * end fbe_job_service_process_creation_queue_access()
 ****************************************************/

/*!**************************************************************
        
        case FBE_JOB_CONTROL_CODE_CREATE_PROVISION_DRIVE:
 * fbe_job_service_set_element_state()
 ****************************************************************
 * @brief
 * sets the state of the job control entry passed
 *
 * @param entry_p - queue element whose state field is to be set
 * @param state - state to set the entry_p state field
 *
 * @none
 *
 * @author
 * 01/04/20010 - Created. Jesus Medina
 *
 ****************************************************************/

void fbe_job_service_set_element_state(fbe_job_queue_element_t *entry_p, 
        fbe_job_action_state_t state)
{
    entry_p->current_state = state;
    return;
}
/*****************************************
 * end fbe_job_service_set_element_state()
 ****************************************/

/*!**************************************************************
 * fbe_job_service_get_job_type
 ****************************************************************
 * @brief
 * This function gets a job type based on the control code
 * of passed packet
 * 
 * @param packet_p - packet containing control operation
 * @param out_job_type - returning job type 
 *
 * @return status - status ok if proper job type found
 *                  generic error otherwise
 *
 * @author
 * 09/19/2010 - Created. Jesus Medina.
 * 
 ****************************************************************/

fbe_status_t fbe_job_service_get_job_type(fbe_packet_t * packet_p,
                                          fbe_job_type_t *out_job_type)
{
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_job_type_t          job_type = FBE_JOB_TYPE_INVALID;
    fbe_payload_control_operation_opcode_t control_code;

    job_service_trace(FBE_TRACE_LEVEL_DEBUG_LOW, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry\n",
            __FUNCTION__);
  
    control_code = fbe_transport_get_control_code(packet_p);

    job_type = fbe_job_service_get_job_type_by_control_type(control_code);
    
    if (job_type == FBE_JOB_TYPE_INVALID)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    *out_job_type = job_type;

    return status;
}
/************************************
 * end fbe_job_service_get_job_type()
 ************************************/

/*!**************************************************************
 * fbe_job_service_get_job_type_by_control_type
 ****************************************************************
 * @brief
 * This function gets a job type based on the control code
 * passed
 * 
 * @param control_code - code type 
 *
 * @return fbe_job_type_t - job type, invalid if improper
 * config code passed
 *
 * @author
 * 09/27/2010 - Created. Jesus Medina.
 * 
 ****************************************************************/

fbe_job_type_t fbe_job_service_get_job_type_by_control_type(fbe_payload_control_operation_opcode_t in_control_code)
{
    fbe_job_type_t          job_type = FBE_JOB_TYPE_INVALID;

    job_service_trace(FBE_TRACE_LEVEL_DEBUG_LOW, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry\n",
            __FUNCTION__);

    switch(in_control_code)
    {
        case FBE_JOB_CONTROL_CODE_DRIVE_SWAP:
            job_type = FBE_JOB_TYPE_DRIVE_SWAP;
            break;
        case FBE_JOB_CONTROL_CODE_RAID_GROUP_CREATE:
            job_type = FBE_JOB_TYPE_RAID_GROUP_CREATE;
            break;
        case FBE_JOB_CONTROL_CODE_RAID_GROUP_DESTROY:
            job_type = FBE_JOB_TYPE_RAID_GROUP_DESTROY;
            break;
        case FBE_JOB_CONTROL_CODE_LUN_CREATE:
            job_type = FBE_JOB_TYPE_LUN_CREATE;
            break;
        case FBE_JOB_CONTROL_CODE_LUN_DESTROY:
            job_type = FBE_JOB_TYPE_LUN_DESTROY;
            break;
        case FBE_JOB_CONTROL_CODE_LUN_UPDATE:
            job_type = FBE_JOB_TYPE_LUN_UPDATE;
            break;
        case FBE_JOB_CONTROL_CODE_CHANGE_SYSTEM_POWER_SAVING_INFO:
            job_type = FBE_JOB_TYPE_CHANGE_SYSTEM_POWER_SAVING_INFO;
            break;
        case FBE_JOB_CONTROL_CODE_UPDATE_RAID_GROUP:
            job_type = FBE_JOB_TYPE_UPDATE_RAID_GROUP;
            break;
        case FBE_JOB_CONTROL_CODE_UPDATE_VIRTUAL_DRIVE:
            job_type = FBE_JOB_TYPE_UPDATE_VIRTUAL_DRIVE;
            break;
        case FBE_JOB_CONTROL_CODE_UPDATE_PROVISION_DRIVE:
            job_type = FBE_JOB_TYPE_UPDATE_PROVISION_DRIVE;
            break;
        case FBE_JOB_CONTROL_CODE_UPDATE_PROVISION_DRIVE_BLOCK_SIZE:
            job_type = FBE_JOB_TYPE_UPDATE_PROVISION_DRIVE_BLOCK_SIZE;
            break;
        case FBE_JOB_CONTROL_CODE_REINITIALIZE_PROVISION_DRIVE:
            job_type = FBE_JOB_TYPE_REINITIALIZE_PROVISION_DRIVE;
            break;
        case FBE_JOB_CONTROL_CODE_CREATE_PROVISION_DRIVE:
            job_type = FBE_JOB_TYPE_CREATE_PROVISION_DRIVE;
            break;
        case FBE_JOB_CONTROL_CODE_UPDATE_SPARE_CONFIG:
            job_type = FBE_JOB_TYPE_UPDATE_SPARE_CONFIG;
            break;
        case FBE_JOB_CONTROL_CODE_UPDATE_DB_ON_PEER:
            job_type = FBE_JOB_TYPE_UPDATE_DB_ON_PEER;
            break;
        case FBE_JOB_CONTROL_CODE_UPDATE_JOB_ELEMENTS_ON_PEER:
            job_type = FBE_JOB_TYPE_UPDATE_JOB_ELEMENTS_ON_PEER;
            break;
        case FBE_JOB_CONTROL_CODE_DESTROY_PROVISION_DRIVE:
            job_type = FBE_JOB_TYPE_DESTROY_PROVISION_DRIVE;
            break;
        case FBE_JOB_CONTROL_CODE_CONTROL_SYSTEM_BG_SERVICE:
            job_type = FBE_JOB_TYPE_CONTROL_SYSTEM_BG_SERVICE;
            break;
        case FBE_JOB_CONTROL_CODE_SEP_NDU_COMMIT:
            job_type = FBE_JOB_TYPE_SEP_NDU_COMMIT;
            break;
        case FBE_JOB_CONTROL_CODE_SET_SYSTEM_TIME_THRESHOLD_INFO:
            job_type = FBE_JOB_TYPE_SET_SYSTEM_TIME_THRESHOLD_INFO;
            break;
        case FBE_JOB_CONTROL_CODE_CONNECT_DRIVE:
            job_type = FBE_JOB_TYPE_CONNECT_DRIVE;
            break;
        case FBE_JOB_CONTROL_CODE_UPDATE_MULTI_PVDS_POOL_ID:
            job_type = FBE_JOB_TYPE_UPDATE_MULTI_PVDS_POOL_ID;
            break;
        case FBE_JOB_CONTROL_CODE_UPDATE_SPARE_LIBRARY_CONFIG:
            job_type = FBE_JOB_TYPE_UPDATE_SPARE_LIBRARY_CONFIG;
            break;
        case FBE_JOB_CONTROL_CODE_CHANGE_SYSTEM_ENCRYPTION_INFO:
            job_type = FBE_JOB_TYPE_CHANGE_SYSTEM_ENCRYPTION_INFO;
            break;
        case FBE_JOB_CONTROL_CODE_SET_ENCRYPTION_PAUSED:
            job_type = FBE_JOB_TYPE_SET_ENCRYPTION_PAUSED;
            break;
        case FBE_JOB_CONTROL_CODE_UPDATE_ENCRYPTION_MODE:
            job_type = FBE_JOB_TYPE_UPDATE_ENCRYPTION_MODE;
            break;
        case FBE_JOB_CONTROL_CODE_PROCESS_ENCRYPTION_KEYS:
            job_type = FBE_JOB_TYPE_PROCESS_ENCRYPTION_KEYS;
            break;
        case FBE_JOB_CONTROL_CODE_SCRUB_OLD_USER_DATA:
            job_type = FBE_JOB_TYPE_SCRUB_OLD_USER_DATA;
            break;
        case FBE_JOB_CONTROL_CODE_VALIDATE_DATABASE:
            job_type = FBE_JOB_TYPE_VALIDATE_DATABASE;
            break;
        case FBE_JOB_CONTROL_CODE_CREATE_EXTENT_POOL:
            job_type = FBE_JOB_TYPE_CREATE_EXTENT_POOL;
            break;
        case FBE_JOB_CONTROL_CODE_CREATE_EXTENT_POOL_LUN:
            job_type = FBE_JOB_TYPE_CREATE_EXTENT_POOL_LUN;
            break;
        case FBE_JOB_CONTROL_CODE_DESTROY_EXTENT_POOL:
            job_type = FBE_JOB_TYPE_DESTROY_EXTENT_POOL;
            break;
        case FBE_JOB_CONTROL_CODE_DESTROY_EXTENT_POOL_LUN:
            job_type = FBE_JOB_TYPE_DESTROY_EXTENT_POOL_LUN;
            break;
        case FBE_JOB_CONTROL_CODE_INVALID:
            job_type = FBE_JOB_TYPE_INVALID;
            break;
    }

    return job_type;
}
/****************************************************
 * end fbe_job_service_get_job_type_by_control_type()
 ***************************************************/


/*!**************************************************************
 * job_service_request_get_queue_type()
 ****************************************************************
 * @brief
 * fills the proper queue type based on packet's control code
 *
 * @param packet - packet that contains code to key switch 
 *                 statement to define the queue type
 *
 * @return status - status of operation
 *
 * @author
 * 09/22/2010 - Created. Jesus Medina
 *
 ****************************************************************/

fbe_status_t job_service_request_get_queue_type(fbe_packet_t *packet_p,
                                                fbe_job_control_queue_type_t *out_queue_type)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_job_control_queue_type_t queue_type = FBE_JOB_INVALID_QUEUE;
    fbe_payload_control_operation_opcode_t control_code;

    job_service_trace(FBE_TRACE_LEVEL_DEBUG_LOW, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry\n",
            __FUNCTION__);

    control_code = fbe_transport_get_control_code(packet_p);

    switch(control_code) 
    {
        case FBE_JOB_CONTROL_CODE_DRIVE_SWAP:
        case FBE_JOB_CONTROL_CODE_DISABLE_PROCESSING_OF_RECOVERY_QUEUE:
        case FBE_JOB_CONTROL_CODE_ENABLE_PROCESSING_OF_RECOVERY_QUEUE:
        case FBE_JOB_CONTROL_CODE_REMOVE_ELEMENT_FROM_RECOVERY_QUEUE:
        case FBE_JOB_CONTROL_CODE_SEARCH_OBJECT_IN_RECOVERY_QUEUE:
        case FBE_JOB_CONTROL_CODE_GET_NUMBER_OF_ELEMENTS_IN_RECOVERY_QUEUE:
        case FBE_JOB_CONTROL_CODE_UPDATE_VIRTUAL_DRIVE:
        case FBE_JOB_CONTROL_CODE_UPDATE_SPARE_LIBRARY_CONFIG:
            queue_type = FBE_JOB_RECOVERY_QUEUE;
            break;

        case FBE_JOB_CONTROL_CODE_RAID_GROUP_CREATE:
        case FBE_JOB_CONTROL_CODE_RAID_GROUP_DESTROY:
        case FBE_JOB_CONTROL_CODE_LUN_CREATE:
        case FBE_JOB_CONTROL_CODE_LUN_DESTROY:
        case FBE_JOB_CONTROL_CODE_DISABLE_PROCESSING_OF_CREATION_QUEUE:
        case FBE_JOB_CONTROL_CODE_ENABLE_PROCESSING_OF_CREATION_QUEUE:
        case FBE_JOB_CONTROL_CODE_REMOVE_ELEMENT_FROM_CREATION_QUEUE:
        case FBE_JOB_CONTROL_CODE_SEARCH_OBJECT_IN_CREATION_QUEUE:
        case FBE_JOB_CONTROL_CODE_GET_NUMBER_OF_ELEMENTS_IN_CREATION_QUEUE:
        case FBE_JOB_CONTROL_CODE_CHANGE_SYSTEM_POWER_SAVING_INFO:
        case FBE_JOB_CONTROL_CODE_UPDATE_PROVISION_DRIVE:
        case FBE_JOB_CONTROL_CODE_UPDATE_PROVISION_DRIVE_BLOCK_SIZE:
        case FBE_JOB_CONTROL_CODE_REINITIALIZE_PROVISION_DRIVE:
        case FBE_JOB_CONTROL_CODE_LUN_UPDATE:
        case FBE_JOB_CONTROL_CODE_CREATE_PROVISION_DRIVE:
        case FBE_JOB_CONTROL_CODE_UPDATE_DB_ON_PEER:
        case FBE_JOB_CONTROL_CODE_DESTROY_PROVISION_DRIVE:
        case FBE_JOB_CONTROL_CODE_CONTROL_SYSTEM_BG_SERVICE:
        case FBE_JOB_CONTROL_CODE_SEP_NDU_COMMIT:
        case FBE_JOB_CONTROL_CODE_SET_SYSTEM_TIME_THRESHOLD_INFO:
        case FBE_JOB_CONTROL_CODE_CONNECT_DRIVE:
        case FBE_JOB_CONTROL_CODE_UPDATE_RAID_GROUP:
        case FBE_JOB_CONTROL_CODE_UPDATE_SPARE_CONFIG:
        case FBE_JOB_CONTROL_CODE_UPDATE_MULTI_PVDS_POOL_ID:
        case FBE_JOB_CONTROL_CODE_CHANGE_SYSTEM_ENCRYPTION_INFO:
        case FBE_JOB_CONTROL_CODE_UPDATE_ENCRYPTION_MODE:
        case FBE_JOB_CONTROL_CODE_PROCESS_ENCRYPTION_KEYS:
        case FBE_JOB_CONTROL_CODE_SCRUB_OLD_USER_DATA:
        case FBE_JOB_CONTROL_CODE_SET_ENCRYPTION_PAUSED:
        case FBE_JOB_CONTROL_CODE_VALIDATE_DATABASE:
        case FBE_JOB_CONTROL_CODE_CREATE_EXTENT_POOL:
        case FBE_JOB_CONTROL_CODE_CREATE_EXTENT_POOL_LUN:
        case FBE_JOB_CONTROL_CODE_DESTROY_EXTENT_POOL:
        case FBE_JOB_CONTROL_CODE_DESTROY_EXTENT_POOL_LUN:
            queue_type = FBE_JOB_CREATION_QUEUE;
            break;

        default:
            queue_type = FBE_JOB_INVALID_QUEUE;
            break;
    }

    if (queue_type == FBE_JOB_INVALID_QUEUE)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
    }

    *out_queue_type = queue_type;
    return status;
}
/***************************************************
 * end job_service_request_get_queue_type()
 **************************************************/

/*!**************************************************************
    case FBE_JOB_TYPE_LUN_DESTROY:
    case FBE_JOB_TYPE_CREATE_PROVISION_DRIVE:
 * fbe_job_service_find_service_operation
 ****************************************************************
 * @brief
 * This function gets determines if caller filled the list of functions
 * required by job service to call
 *
 * @param out_job_type - job type 
 *
 * @return status - status ok if proper list of function provided
 *
 * @author
 * 09/19/2010 - Created. Jesus Medina.
 * 
 ****************************************************************/

fbe_status_t fbe_job_service_find_service_operation(fbe_job_type_t job_type)
{
    fbe_job_service_operation_t  *fbe_job_service_operation_p = NULL;
    fbe_status_t                 status = FBE_STATUS_OK;

    job_service_trace(FBE_TRACE_LEVEL_DEBUG_LOW, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry\n",
            __FUNCTION__);

    fbe_job_service_operation_p = fbe_job_service_find_operation(job_type);
    if (fbe_job_service_operation_p == NULL)
    {
        job_service_trace(FBE_TRACE_LEVEL_WARNING, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "jbs_find_service_operation: NULL Jbs Operation for job_type: 0x%x \n",
            job_type);
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}
/*********************************************
 * end fbe_job_service_find_service_operation()
 *********************************************/


/*!**************************************************************
 * fbe_job_service_build_queue_element
 ****************************************************************
 * @brief
 * This function inits a job service queue element data structure
 * fields
 * 
 * @param job_control_element - data structure to init
 * @param job_control_code    - external code 
 * @param object_id           - the ID of the target object
 * @param command_data_p      - pointer to source data
 * @param command_size        - size of source data
 *
 * @return status - status of init call
 *
 * @author
 * 10/29/2009 - Created. Jesus Medina.
 * 
 ****************************************************************/

fbe_status_t fbe_job_service_build_queue_element(fbe_job_queue_element_t *in_job_queue_element_p,
                         fbe_payload_control_operation_opcode_t in_control_code,
                         fbe_object_id_t in_object_id,
                         fbe_u8_t *in_command_data_p,
                         fbe_u32_t in_command_size)
{
    /* Context size cannot be greater than job element context size.
    */
    if (in_command_size > FBE_JOB_ELEMENT_CONTEXT_SIZE)
    {
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    in_job_queue_element_p->object_id = in_object_id;
    in_job_queue_element_p->class_id = FBE_CLASS_ID_INVALID;
    in_job_queue_element_p->status = FBE_STATUS_OK;	
    in_job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;
    in_job_queue_element_p->timestamp = 0;
    in_job_queue_element_p->current_state = FBE_JOB_ACTION_STATE_INVALID;
    in_job_queue_element_p->previous_state = FBE_JOB_ACTION_STATE_INVALID;
    in_job_queue_element_p->queue_access = FBE_FALSE;
    in_job_queue_element_p->num_objects = 0;
    in_job_queue_element_p->command_size = in_command_size;
    in_job_queue_element_p->need_to_wait = FBE_FALSE;
    in_job_queue_element_p->timeout_msec = 60000;

    fbe_zero_memory(in_job_queue_element_p->command_data, FBE_JOB_ELEMENT_CONTEXT_SIZE);

    if (in_command_data_p != NULL)
    {
        fbe_copy_memory(in_job_queue_element_p->command_data, in_command_data_p, in_command_size);
    }
    return FBE_STATUS_OK;
}
/*******************************************
 * end fbe_job_service_build_queue_element()
 ******************************************/

/*!**************************************************************
 * fbe_job_service_translate_job_type()
 ****************************************************************
 * @brief
 * translates and returns a job service job type to string
 *
 * @param control_code - job type to translate             
 *
 * @return translated_code - matching string for control code
 *
 * @author
 * 08/24/2010 - Created. Jesus Medina
 *
 ****************************************************************/

fbe_status_t fbe_job_service_translate_job_type(fbe_job_type_t job_type,
                                                const char ** pp_job_type_name)
{
    *pp_job_type_name = NULL;
    switch (job_type) {
        case FBE_JOB_TYPE_DRIVE_SWAP:
            *pp_job_type_name = "FBE_JOB_TYPE_DRIVE_SWAP";
            break;
        case FBE_JOB_TYPE_ADD_ELEMENT_TO_QUEUE_TEST:
            *pp_job_type_name = "FBE_JOB_TYPE_ADD_ELEMENT_TO_QUEUE_TEST";
            break;
        case FBE_JOB_TYPE_RAID_GROUP_CREATE:
            *pp_job_type_name = "FBE_JOB_TYPE_RAID_GROUP_CREATE";
            break;
        case FBE_JOB_TYPE_RAID_GROUP_DESTROY:
            *pp_job_type_name = "FBE_JOB_TYPE_RAID_GROUP_DESTROY";
            break;
        case FBE_JOB_TYPE_LUN_CREATE:
            *pp_job_type_name = "FBE_JOB_TYPE_LUN_CREATE";
            break;
        case FBE_JOB_TYPE_LUN_DESTROY:
            *pp_job_type_name = "FBE_JOB_TYPE_LUN_DESTROY";
            break;
        case FBE_JOB_TYPE_LUN_UPDATE:
            *pp_job_type_name = "FBE_JOB_TYPE_LUN_UPDATE";
            break;
        case FBE_JOB_TYPE_CHANGE_SYSTEM_POWER_SAVING_INFO:
            *pp_job_type_name = "FBE_JOB_TYPE_CHANGE_SYSTEM_POWER_SAVING_INFO";
            break;
        case FBE_JOB_TYPE_UPDATE_RAID_GROUP:
            *pp_job_type_name = "FBE_JOB_TYPE_UPDATE_RAID_GROUP";
            break;
        case FBE_JOB_TYPE_CREATE_PROVISION_DRIVE:
            *pp_job_type_name = "FBE_JOB_TYPE_CREATE_PROVISION_DRIVE";
            break;
        case FBE_JOB_TYPE_UPDATE_VIRTUAL_DRIVE:
            *pp_job_type_name = "FBE_JOB_TYPE_UPDATE_VIRTUAL_DRIVE";
            break;
        case FBE_JOB_TYPE_UPDATE_PROVISION_DRIVE:
            *pp_job_type_name = "FBE_JOB_TYPE_UPDATE_PROVISION_DRIVE";
            break;
        case FBE_JOB_TYPE_UPDATE_PROVISION_DRIVE_BLOCK_SIZE:
            *pp_job_type_name = "FBE_JOB_TYPE_UPDATE_PROVISION_DRIVE_BLOCK_SIZE";
            break;
        case FBE_JOB_TYPE_REINITIALIZE_PROVISION_DRIVE:
            *pp_job_type_name = "FBE_JOB_TYPE_REINITIALIZE_PROVISION_DRIVE";
            break;
        case FBE_JOB_TYPE_UPDATE_SPARE_CONFIG:
            *pp_job_type_name = "FBE_JOB_TYPE_UPDATE_SPARE_CONFIG";
            break;
        case FBE_JOB_TYPE_UPDATE_DB_ON_PEER:
            *pp_job_type_name = "FBE_JOB_TYPE_UPDATE_DB_ON_PEER";
            break;
        case FBE_JOB_TYPE_UPDATE_JOB_ELEMENTS_ON_PEER:
            *pp_job_type_name = "FBE_JOB_TYPE_UPDATE_JOB_ELEMENTS_ON_PEER";
            break;
        case FBE_JOB_TYPE_INVALID:
            *pp_job_type_name = "FBE_JOB_TYPE_INVALID";
            break;
        case FBE_JOB_TYPE_DESTROY_PROVISION_DRIVE:
            *pp_job_type_name = "FBE_JOB_TYPE_DESTROY_PROVISION_DRIVE";
            break;
        case FBE_JOB_TYPE_CONTROL_SYSTEM_BG_SERVICE:
            *pp_job_type_name = "FBE_JOB_TYPE_CONTROL_SYSTEM_BG_SERVICE";
        break;
        case FBE_JOB_TYPE_SEP_NDU_COMMIT:
            *pp_job_type_name = "FBE_JOB_TYPE_SEP_NDU_COMMIT";
        break;
        case FBE_JOB_TYPE_SET_SYSTEM_TIME_THRESHOLD_INFO:
            *pp_job_type_name = "FBE_JOB_TYPE_SET_SYSTEM_TIME_THRESHOLD_INFO";
            break;
       case FBE_JOB_TYPE_CONNECT_DRIVE:
            *pp_job_type_name = "FBE_JOB_TYPE_CONNECT_DRIVE";
            break;
       case FBE_JOB_TYPE_UPDATE_MULTI_PVDS_POOL_ID:
            *pp_job_type_name = "FBE_JOB_TYPE_UPDATE_MULTI_PVDS_POOL_ID";
            break;
        case FBE_JOB_TYPE_UPDATE_SPARE_LIBRARY_CONFIG:
            *pp_job_type_name = "FBE_JOB_TYPE_UPDATE_SPARE_LIBRARY_CONFIG";
            break;
        case FBE_JOB_TYPE_CHANGE_SYSTEM_ENCRYPTION_INFO:
            *pp_job_type_name = "FBE_JOB_TYPE_CHANGE_SYSTEM_ENCRYPTION_INFO";
            break;
        case FBE_JOB_TYPE_SET_ENCRYPTION_PAUSED:
            *pp_job_type_name = "FBE_JOB_TYPE_SET_ENCRYPTION_PAUSED";
            break;
        case FBE_JOB_TYPE_UPDATE_ENCRYPTION_MODE:
            *pp_job_type_name = "FBE_JOB_TYPE_UPDATE_ENCRYPTION_MODE";
            break;
        case FBE_JOB_TYPE_PROCESS_ENCRYPTION_KEYS:
            *pp_job_type_name = "FBE_JOB_TYPE_PROCESS_ENCRYPTION_KEYS";
            break;
        case FBE_JOB_TYPE_SCRUB_OLD_USER_DATA:
            *pp_job_type_name = "FBE_JOB_TYPE_SCRUB_OLD_USER_DATA";
            break;
        case FBE_JOB_TYPE_VALIDATE_DATABASE:
            *pp_job_type_name = "FBE_JOB_TYPE_VALIDATE_DATABASE";
            break;
        case FBE_JOB_TYPE_CREATE_EXTENT_POOL:
            *pp_job_type_name = "FBE_JOB_TYPE_CREATE_EXTENT_POOL";
            break;
        case FBE_JOB_TYPE_CREATE_EXTENT_POOL_LUN:
            *pp_job_type_name = "FBE_JOB_TYPE_CREATE_EXTENT_POOL_LUN";
            break;
        case FBE_JOB_TYPE_DESTROY_EXTENT_POOL:
            *pp_job_type_name = "FBE_JOB_TYPE_DESTROY_EXTENT_POOL";
            break;
        case FBE_JOB_TYPE_DESTROY_EXTENT_POOL_LUN:
            *pp_job_type_name = "FBE_JOB_TYPE_DESTROY_EXTENT_POOL_LUN";
            break;
        default:
            *pp_job_type_name = "UNKNOWN_FBE_JOB_TYPE";
            job_service_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, 
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "UNKNOWN_FBE_JOB_TYPE: %d\n", job_type);

            break;
    }
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_job_service_translate_job_type()
 ******************************************/

/*!**************************************************************
 * fbe_job_service_cmi_is_active()
 ****************************************************************
 * @brief
 * determines if an SP is the active SP
 *
 * @param - none           
 *
 * @return true if active side, false otherwise
 *
 * @author
 * 09/15/2010 - Created. Jesus Medina
 *
 ****************************************************************/
fbe_bool_t fbe_job_service_cmi_is_active(void)
{
    static fbe_u32_t   previous_state = ~FBE_FALSE;
    fbe_cmi_sp_state_t sp_state;
    fbe_bool_t         is_active;

    fbe_cmi_get_current_sp_state(&sp_state);
    while(sp_state != FBE_CMI_STATE_ACTIVE && sp_state != FBE_CMI_STATE_PASSIVE && sp_state != FBE_CMI_STATE_SERVICE_MODE)
    {
        fbe_thread_delay(100);
        fbe_cmi_get_current_sp_state(&sp_state);
    }

    is_active = ( sp_state == FBE_CMI_STATE_ACTIVE ) ? FBE_TRUE : FBE_FALSE;

    if ( is_active != previous_state )
    {
        job_service_trace(FBE_TRACE_LEVEL_DEBUG_MEDIUM, FBE_TRACE_MESSAGE_ID_INFO,
            "%s: is_active = %d\n", __FUNCTION__, is_active);

        previous_state = is_active;
    }
    
    if (is_active)
    {
        job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
            "Job service, running on active SP\n");
    }

    return is_active;
}
/*************************************
 * end fbe_job_service_cmi_is_active()
 *************************************/

/*!**************************************************************
 * fbe_job_service_write_to_event_log()
 ****************************************************************
 * @brief
 * Write the Status Code to the Event Log.
 *
 * @param - request_p - Pointer to the job element queue           
 *
 * @return fbe_status_t
 *
 * @author
 * 07/15/2011 - Created. Sonali
 *
 ****************************************************************/
fbe_status_t fbe_job_service_write_to_event_log(fbe_job_queue_element_t *request_p)
{
    fbe_u32_t message_code;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_job_service_lun_create_t *lun_create_request_p = NULL;
    fbe_database_control_lookup_lun_t lookup_lun;
    fbe_job_service_lun_destroy_t *lun_destroy_request_p = NULL;

    switch (request_p->job_type)
    {
        case FBE_JOB_TYPE_LUN_CREATE:
            message_code = SEP_INFO_LUN_CREATED;
            lun_create_request_p = (fbe_job_service_lun_create_t *) request_p->command_data;
            lookup_lun.lun_number = lun_create_request_p->lun_number;
            status = fbe_job_service_lib_send_control_packet(FBE_DATABASE_CONTROL_CODE_LOOKUP_LUN_BY_NUMBER,
                                                             &lookup_lun,
                                                             sizeof(fbe_database_control_lookup_lun_t),
                                                             FBE_PACKAGE_ID_SEP_0,                                             
                                                             FBE_SERVICE_ID_DATABASE,
                                                             FBE_CLASS_ID_INVALID,
                                                             FBE_OBJECT_ID_INVALID,
                                                             FBE_PACKET_FLAG_NO_ATTRIB);

            if (status != FBE_STATUS_OK)
            {
                job_service_trace(FBE_TRACE_LEVEL_WARNING,
                    FBE_TRACE_MESSAGE_ID_INFO, 
                    "jbs_write_to_event_log: Failed look up lun.obj=0x%x, status:0x%x\n",
                    lookup_lun.object_id, status);
            }else{
                /* log message to the event-log */
                status = fbe_event_log_write(message_code, NULL, 0, "%x %d",
                    lookup_lun.object_id, 
                    lun_create_request_p->lun_number); 
            }
            break;

        case FBE_JOB_TYPE_LUN_DESTROY:
            message_code = SEP_INFO_LUN_DESTROYED;
            lun_destroy_request_p = (fbe_job_service_lun_destroy_t *) request_p->command_data;
            lookup_lun.lun_number = lun_destroy_request_p->lun_number;           
   
            /* log message to the event-log */
            status = fbe_event_log_write(message_code, NULL, 0, "%d",                                         
                                         lun_destroy_request_p->lun_number);  
            break;

        default:
            {
                /* No Need to log these jobs so just return OK status */
                const char *p_job_type_str = NULL;

                fbe_job_service_translate_job_type(request_p->job_type, &p_job_type_str);

                job_service_trace(FBE_TRACE_LEVEL_INFO,
                    FBE_TRACE_MESSAGE_ID_INFO, "jbs_write_to_event_log: No log for: %s.\n", 
                    p_job_type_str);

                break;
            }
    }

    return status;

} /* end of fbe_job_service_write_to_event_log */

/*****************************************************************************
 *          fbe_job_service_generate_and_send_notification()
 ***************************************************************************** 
 * 
 * @brief   Use the the job element and associated data, generate the associated
 *          notification.
 *
 * @param   request_p - Pointer to the job element queue           
 *
 * @return  fbe_status_t
 *
 * @author
 *  08/27/2012  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_status_t fbe_job_service_generate_and_send_notification(fbe_job_queue_element_t *request_p)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_notification_info_t                 notification_info;
    fbe_job_service_drive_swap_request_t   *drive_swap_request_p = NULL;

    /* Process the job completion.  Populate the basic information.
     */
    fbe_zero_memory(&notification_info, sizeof(fbe_notification_info_t));
    notification_info.notification_type = FBE_NOTIFICATION_TYPE_JOB_ACTION_STATE_CHANGED;
    notification_info.source_package = FBE_PACKAGE_ID_SEP_0;
    notification_info.class_id = request_p->class_id;
    /* Populate the object type.
     */
    if ((request_p->class_id >= FBE_CLASS_ID_VERTEX) &&
        (request_p->class_id <  FBE_CLASS_ID_LAST)      )
    {
        fbe_topology_class_id_to_object_type(request_p->class_id, &notification_info.object_type);
    }
    else
    {
        notification_info.object_type = FBE_TOPOLOGY_OBJECT_TYPE_ALL;
    }

    /* Populate the job service error information.
     */
    notification_info.notification_data.job_service_error_info.object_id = request_p->object_id;
    notification_info.notification_data.job_service_error_info.status = request_p->status;
    notification_info.notification_data.job_service_error_info.error_code = request_p->error_code;
    notification_info.notification_data.job_service_error_info.job_number = request_p->job_number;
    notification_info.notification_data.job_service_error_info.job_type = request_p->job_type;

    /* trace the notification. */
    job_service_trace(FBE_TRACE_LEVEL_INFO,
                      FBE_TRACE_MESSAGE_ID_INFO,
                      "job send peer notification. job: 0x%llx job type: 0x%x job status: 0x%x \n",
                      (unsigned long long)request_p->job_number, request_p->job_type, request_p->status);

    /* Only certain items should be on the recovery queue.
     */
    switch (request_p->job_type)
    {
        case FBE_JOB_TYPE_DRIVE_SWAP:
            /* Generate the full swap-in information.
             */
            if (request_p->command_size < sizeof(*drive_swap_request_p))
            {
                job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s: Command size: %d smaller than expected: %d\n",
                                  __FUNCTION__, request_p->command_size, (int)sizeof(*drive_swap_request_p));
                return FBE_STATUS_GENERIC_FAILURE;
            }
            drive_swap_request_p = (fbe_job_service_drive_swap_request_t *)&request_p->command_data[0];
            notification_info.notification_data.job_service_error_info.swap_info.command_type = drive_swap_request_p->command_type;
            notification_info.notification_data.job_service_error_info.swap_info.orig_pvd_object_id = drive_swap_request_p->orig_pvd_object_id;
            notification_info.notification_data.job_service_error_info.swap_info.spare_pvd_object_id = drive_swap_request_p->spare_object_id;
            notification_info.notification_data.job_service_error_info.swap_info.vd_object_id = drive_swap_request_p->vd_object_id;
            job_service_trace(FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "SPARE: send notification peer cmd: %d status: %d vd obj: 0x%x orig: 0x%x spare: 0x%x\n",
                              drive_swap_request_p->command_type, request_p->error_code, drive_swap_request_p->vd_object_id,
                              drive_swap_request_p->orig_pvd_object_id, drive_swap_request_p->spare_object_id);
            status = fbe_notification_send(request_p->object_id, notification_info);
            if (status != FBE_STATUS_OK) 
            {
                job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s: Send Job Notification failed, status: 0x%x\n",
                                  __FUNCTION__, status);
                return status;
            }
            break;

        case FBE_JOB_TYPE_ADD_ELEMENT_TO_QUEUE_TEST:
        case FBE_JOB_TYPE_LUN_UPDATE:
        case FBE_JOB_TYPE_CHANGE_SYSTEM_POWER_SAVING_INFO:
        case FBE_JOB_TYPE_UPDATE_RAID_GROUP:  
        case FBE_JOB_TYPE_UPDATE_VIRTUAL_DRIVE:
        case FBE_JOB_TYPE_UPDATE_PROVISION_DRIVE:
        case FBE_JOB_TYPE_UPDATE_PROVISION_DRIVE_BLOCK_SIZE:
        case FBE_JOB_TYPE_REINITIALIZE_PROVISION_DRIVE:
        case FBE_JOB_TYPE_UPDATE_SPARE_CONFIG:
        case FBE_JOB_TYPE_UPDATE_DB_ON_PEER:
        case FBE_JOB_TYPE_UPDATE_JOB_ELEMENTS_ON_PEER:
        case FBE_JOB_TYPE_CHANGE_SYSTEM_ENCRYPTION_INFO:
        case FBE_JOB_TYPE_SET_ENCRYPTION_PAUSED:
        case FBE_JOB_TYPE_UPDATE_ENCRYPTION_MODE:
        case FBE_JOB_TYPE_PROCESS_ENCRYPTION_KEYS:
        case FBE_JOB_TYPE_SCRUB_OLD_USER_DATA:
        case FBE_JOB_TYPE_VALIDATE_DATABASE:
        case FBE_JOB_TYPE_CREATE_EXTENT_POOL:
        case FBE_JOB_TYPE_CREATE_EXTENT_POOL_LUN:
        case FBE_JOB_TYPE_DESTROY_EXTENT_POOL:
        case FBE_JOB_TYPE_DESTROY_EXTENT_POOL_LUN:
            /* The above job types simply populate the basic information.
             */
            job_service_trace(FBE_TRACE_LEVEL_INFO,
                      FBE_TRACE_MESSAGE_ID_INFO,
                      "JOB: peer send state change notification job: 0x%llx error: 0x%x obj: 0x%x object_type: 0x%llx type: 0x%llx\n",
                      (unsigned long long)request_p->job_number, request_p->error_code, request_p->object_id, 
                      (unsigned long long)notification_info.object_type, (unsigned long long)notification_info.notification_type);
            status = fbe_notification_send(request_p->object_id, notification_info);
            if (status != FBE_STATUS_OK) 
            {
                job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s: Send Job Notification failed, status: 0x%x\n",
                                  __FUNCTION__, status);
                return status;
            }
            break;

        case FBE_JOB_TYPE_UPDATE_SPARE_LIBRARY_CONFIG:
            /* Nothing required for the above job types.
             */
            break;

        default:
            /* Unexpected job type.
             */
            job_service_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s: Unexpected job type: %d\n",
                              __FUNCTION__, request_p->job_type);
            return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Return the execution status.
     */
    return status;
}
/******************************************************* 
 * end fbe_job_service_generate_and_send_notification()
 *******************************************************/

/*!**************************************************************
 * fbe_job_service_lib_send_control_packet()
 ****************************************************************
 * @brief
 * This function send a packet to other services within the logical
 * packet scope
 *
 * @param control_code - the code to send to other services
 * @param buffer - buffer containing data for service 
 * @param buffer_length - length of buffer
 * @params package_id - package type
 * @param service_id - service to send buffer
 * @param class_id - what class the service belongs to
 * @param object_id - id of the object service will work on
 * @param attr - attributes of packet 
 *
 * @return status - The status of the operation.
 *
 ****************************************************************/
fbe_status_t fbe_job_service_lib_send_control_packet (
        fbe_payload_control_operation_opcode_t control_code,
        fbe_payload_control_buffer_t buffer,
        fbe_payload_control_buffer_length_t buffer_length,
        fbe_package_id_t package_id,
        fbe_service_id_t service_id,
        fbe_class_id_t class_id,
        fbe_object_id_t object_id,
        fbe_packet_attr_t attr)
{
    fbe_status_t                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_packet_t *                  packet_p = NULL;
    fbe_payload_ex_t *             sep_payload = NULL;
    fbe_payload_control_operation_t *control_operation = NULL;
    fbe_payload_control_status_t    payload_control_status = FBE_PAYLOAD_CONTROL_STATUS_OK;

    packet_p = fbe_transport_allocate_packet();
    if (packet_p == NULL) {
        job_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                FBE_TRACE_MESSAGE_ID_INFO,
                "job_src_send_ctrl_packet: can't allocate packet\n");
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_transport_initialize_sep_packet(packet_p);
    sep_payload = fbe_transport_get_payload_ex (packet_p);
    control_operation = fbe_payload_ex_allocate_control_operation(sep_payload);

    fbe_payload_control_build_operation (control_operation,
                                         control_code,
                                         buffer,
                                         buffer_length);

    /* Set packet address.*/
    fbe_transport_set_address(packet_p,
                              package_id,
                              service_id,
                              class_id,
                              object_id); 

    /* Mark packet with the attribute, either traverse or not */
    fbe_transport_set_packet_attr(packet_p, attr);

    fbe_transport_set_sync_completion_type(packet_p,
            FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);

    /* Control packets should be sent via service manager. */
    status = fbe_service_manager_send_control_packet(packet_p);
    if (status != FBE_STATUS_OK)
    {
        if (control_code != FBE_DATABASE_CONTROL_CODE_LOOKUP_RAID_BY_NUMBER)
        {
            job_service_trace(FBE_TRACE_LEVEL_WARNING, 
                FBE_TRACE_MESSAGE_ID_INFO,
                "job_src_send_ctrl_packet: fail!Control Code:0x%x, status:0x%x\n", 
                control_code,
                status);
        }
        else
        {
            job_service_trace(FBE_TRACE_LEVEL_INFO, 
                    FBE_TRACE_MESSAGE_ID_INFO,
                    "job_src_send_ctrl_packet: No rg_id not found; perhaps, new create a RG req.\n");
        }
    }
    fbe_transport_wait_completion(packet_p);

    fbe_payload_control_get_status(control_operation, &payload_control_status);
    if(payload_control_status != FBE_PAYLOAD_CONTROL_STATUS_OK){
        job_service_trace(FBE_TRACE_LEVEL_WARNING, 
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "job_src_send_ctrl_packet: payload ctrl operation fail!Control Code:0x%x\n",
                          control_code);
    }

    status  = fbe_transport_get_status_code(packet_p);
    if(status != FBE_STATUS_OK){
        job_service_trace(FBE_TRACE_LEVEL_WARNING, 
                FBE_TRACE_MESSAGE_ID_INFO,
                "job_src_send_ctrl_packet packet failed\n");
    }

    fbe_payload_ex_release_control_operation(sep_payload, control_operation);
    fbe_transport_release_packet(packet_p);
    return status;
}
/*************************************
 * end fbe_job_service_lib_send_control_packet()
 *************************************/


/*!*************************************************************
 * fbe_job_service_handle_queue_access()
 ***************************************************************
 * @brief
 * This function starts/stops the queue access for Job and arrival
 * Queues. 
 *
 * @param         
 *
 * @return - none
 *
 * @author
 * 10/11/2011 - Created. Arun S
 *
 ****************************************************************/
void fbe_job_service_handle_queue_access(fbe_bool_t queue_access)
{
    /* Handle Job Queues */
    fbe_job_service_lock();
    fbe_job_service_set_recovery_queue_access(queue_access);
    fbe_job_service_set_creation_queue_access(queue_access);
    fbe_job_service_unlock();

    /* Handle Arrival Queues */
    fbe_job_service_arrival_lock();
    fbe_job_service_set_arrival_recovery_queue_access(queue_access);
    fbe_job_service_set_arrival_creation_queue_access(queue_access);
    fbe_job_service_arrival_unlock();

    return;
}

/*!**************************************************************
 * fbe_job_service_get_database_state()
 ****************************************************************
 * @brief
 * This function gets the state of the DB service.
 *
 * @param - none
 *
 * @return State of the Database service
 *
 * @author
 * 10/27/2011 - Created. Arun S
 *
 ****************************************************************/
fbe_database_state_t fbe_job_service_get_database_state(void)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_database_control_get_state_t database_state;
    database_state.state = FBE_DATABASE_STATE_INVALID; // SAFEBUG - uninitialized fields seen to be used

    status = fbe_job_service_lib_send_control_packet(FBE_DATABASE_CONTROL_CODE_GET_STATE,
                                                     &database_state,
                                                     sizeof(fbe_database_control_get_state_t),
                                                     FBE_PACKAGE_ID_SEP_0,                                             
                                                     FBE_SERVICE_ID_DATABASE,
                                                     FBE_CLASS_ID_INVALID,
                                                     FBE_OBJECT_ID_INVALID,
                                                     FBE_PACKET_FLAG_NO_ATTRIB);
    if (status != FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_WARNING, 
                          FBE_TRACE_MESSAGE_ID_INFO, 
                          "%s: Get DB state failed, status:0x%x\n", 
                          __FUNCTION__, status);       
    }

    return database_state.state;

} /* end of fbe_job_service_write_to_event_log */

/*!**************************************************************
 * fbe_job_service_get_state()
 ****************************************************************
 * @brief
 * Get the thread state of the job_thread and arrival_thread. 
 *
 * @param packet_p - packet containing  drive connect request.             
 *
 * @return 
 *
 * @author
 * 11/12/2012 - Created. Arun S
 *
 ****************************************************************/

fbe_status_t fbe_job_service_get_state(fbe_packet_t * packet_p)
{
    fbe_bool_t                             *is_job_service_ready = NULL;
    fbe_payload_ex_t                       *payload_p = NULL;
    fbe_payload_control_operation_t        *control_operation = NULL;
    fbe_payload_control_buffer_length_t    length;
    fbe_status_t                           status = FBE_STATUS_OK;

    job_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry\n", __FUNCTION__);

    /*! Get the request payload. */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload_p);

    fbe_payload_control_get_buffer(control_operation, &is_job_service_ready);

    if(is_job_service_ready == NULL)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s : ERROR: Get State job request is NULL\n",__FUNCTION__);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &length);
    if(length != sizeof(fbe_bool_t))
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s : ERROR: Size of Get State job request is invalid\n",__FUNCTION__);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*! Get the job state */
    if(fbe_job_service_arrival_thread_is_flag_set(FBE_JOB_SERVICE_ARRIVAL_THREAD_FLAGS_RUN) &&
       fbe_job_service_thread_is_flag_set(FBE_JOB_SERVICE_THREAD_FLAGS_RUN))
    {
        *is_job_service_ready = FBE_TRUE;
    }
    else
    {
        *is_job_service_ready = FBE_FALSE;
    }

    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);

    return FBE_STATUS_OK;
}
/*!**************************************************************
 * fbe_job_service_get_object_lifecycle_state(()
 ****************************************************************
 * @brief
 * This function gets the state of the DB service.
 *
 * @param - object_id
 *
 * @return lifecycle_state
 *
 * @author
 * 11/13/2012 - Created. Vera Wang
 *
 ****************************************************************/
fbe_status_t fbe_job_service_get_object_lifecycle_state(fbe_object_id_t object_id, fbe_lifecycle_state_t * lifecycle_state)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_base_object_mgmt_get_lifecycle_state_t      base_object_mgmt_get_lifecycle_state;   

    status = fbe_job_service_lib_send_control_packet(FBE_BASE_OBJECT_CONTROL_CODE_GET_LIFECYCLE_STATE,
                                                     &base_object_mgmt_get_lifecycle_state,
                                                     sizeof(fbe_base_object_mgmt_get_lifecycle_state_t),
                                                     FBE_PACKAGE_ID_SEP_0,                                             
                                                     FBE_SERVICE_ID_TOPOLOGY,
                                                     FBE_CLASS_ID_INVALID,
                                                     object_id,
                                                     FBE_PACKET_FLAG_NO_ATTRIB);

    if (status == FBE_STATUS_NO_OBJECT) {
        job_service_trace (FBE_TRACE_LEVEL_WARNING, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                           "%s: requested object 0x%x does not exist\n", __FUNCTION__, object_id);
        return status;
    }

    /* If object is in SPECIALIZE STATE topology will return FBE_STATUS_BUSY */
    if (status == FBE_STATUS_BUSY) {
        *lifecycle_state = FBE_LIFECYCLE_STATE_SPECIALIZE;
        return FBE_STATUS_OK;
    }

    if (status != FBE_STATUS_OK) {
        job_service_trace (FBE_TRACE_LEVEL_WARNING, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                           "%s:packet error:%d, \n", 
                           __FUNCTION__,
                           status);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }
   
    *lifecycle_state = base_object_mgmt_get_lifecycle_state.lifecycle_state;

    return status;

} 


fbe_status_t fbe_job_service_mark_job_done(fbe_packet_t * packet_p)
{
    fbe_job_queue_element_t                *job_queue_element_p = NULL;
    fbe_payload_control_operation_opcode_t control_code;
    fbe_payload_ex_t                          *payload_p = NULL;
    fbe_payload_control_operation_t        *control_operation = NULL;
    fbe_payload_control_buffer_length_t    length;
    fbe_queue_head_t                       *head_p = NULL;
    fbe_job_queue_element_t                *request_p = NULL;
    fbe_status_t                           status = FBE_STATUS_OK;
    fbe_payload_control_status_t           payload_status = FBE_PAYLOAD_CONTROL_STATUS_OK;

    /*! Get the request payload.
    */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload_p);
    control_code = fbe_transport_get_control_code(packet_p);

    fbe_payload_control_get_buffer(control_operation,
            &job_queue_element_p);
    if(job_queue_element_p == NULL)
    {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &length);
    if(length != sizeof(fbe_job_queue_element_t))
    {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = FBE_STATUS_NO_OBJECT;

    /*! Lock queue */
    fbe_job_service_lock();

    /* if queue is empty, return status no object
    */
    if (fbe_queue_is_empty(&fbe_job_service.creation_q_head) == TRUE
        && fbe_queue_is_empty(&fbe_job_service.recovery_q_head) == TRUE)
    {
        status = FBE_STATUS_NO_OBJECT;
        payload_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
    }
    else
    {
        status = FBE_STATUS_NO_OBJECT;
        
        fbe_job_service_get_creation_queue_head(&head_p);
        request_p = (fbe_job_queue_element_t *) fbe_queue_front(head_p);
        while (request_p != NULL)
        {
            fbe_job_queue_element_t *next_request_p = NULL;

            if (request_p->job_number == job_queue_element_p->job_number)
            {
                status = FBE_STATUS_OK;
                payload_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
                request_p->previous_state = FBE_JOB_ACTION_STATE_INVALID;
                request_p->current_state = FBE_JOB_ACTION_STATE_DONE;
                break;
            }
            next_request_p = (fbe_job_queue_element_t *)fbe_queue_next(head_p, &request_p->queue_element);
            request_p = next_request_p;
        }

        if(FBE_STATUS_OK != status)
        {
            fbe_job_service_get_recovery_queue_head(&head_p);
            request_p = (fbe_job_queue_element_t *) fbe_queue_front(head_p);
            while (request_p != NULL)
            {
                fbe_job_queue_element_t *next_request_p = NULL;
            
                if (request_p->job_number == job_queue_element_p->job_number)
                {
                    status = FBE_STATUS_OK;
                    payload_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
                    request_p->previous_state = FBE_JOB_ACTION_STATE_INVALID;
                    request_p->current_state = FBE_JOB_ACTION_STATE_DONE;
                    break;
                }
                next_request_p = (fbe_job_queue_element_t *)fbe_queue_next(head_p, &request_p->queue_element);
                request_p = next_request_p;
            }
        }        
    }

    /* unlock the queue
    */
    fbe_job_service_unlock();

    if(FBE_STATUS_OK != status)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, 
                          FBE_TRACE_MESSAGE_ID_INFO, 
                          "%s: fail to find job 0x%x in execute queues.\n", 
                          __FUNCTION__, (unsigned int)job_queue_element_p->job_number);
    }

    fbe_payload_control_set_status(control_operation, payload_status);
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);

    return status;
}

/*!**************************************************************
 * fbe_job_service_wait_for_expected_lifecycle_state()
 ****************************************************************
 * @brief
 * This function wait for expected lifecycle state for specific object.
 * 
 *
 * @ args: obj_id   : object_id.
 *         expected_lifecycle_state: expected lifecycle state
 *         timeout_ms
 * 
 * @return status : success or failure.
 * @author
 *  11/13/2012 - Created. Vera Wang
 *
  ****************************************************************/
fbe_status_t fbe_job_service_wait_for_expected_lifecycle_state(fbe_object_id_t obj_id, fbe_lifecycle_state_t expected_lifecycle_state, fbe_u32_t timeout_ms)
{
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_lifecycle_state_t current_state = FBE_LIFECYCLE_STATE_NOT_EXIST;
    fbe_u32_t           current_time = 0;
    fbe_bool_t			got_expected= FBE_FALSE;


    while (current_time < timeout_ms) {
           status = fbe_job_service_get_object_lifecycle_state(obj_id, &current_state);
           if (status == FBE_STATUS_GENERIC_FAILURE){
               job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                 "%s get lifecycle_state call failed for object id %d, status %d\n", 
                                 __FUNCTION__, obj_id, status);         
               return status;
           }

           if ((status == FBE_STATUS_NO_OBJECT) && (expected_lifecycle_state == FBE_LIFECYCLE_STATE_NOT_EXIST)) {
               return FBE_STATUS_OK;
           }
            
           if (current_state == expected_lifecycle_state){
               /*we are happy here, let's check the peer*/
               got_expected = FBE_TRUE;
               break;
           }
            current_time = current_time + 100; 
            fbe_thread_delay(100);
    }

    if (!got_expected) {
        job_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "_wait_for_expected_state: obj: 0x%x, expected state %d != current state %d in %d ms!\n", 
                          obj_id, expected_lifecycle_state, current_state, timeout_ms);
    
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*now let's check on the peer*/
    return fbe_job_service_wait_for_expected_lifecycle_state_on_peer(obj_id,
                                                                    expected_lifecycle_state, 
                                                                    timeout_ms);
}

fbe_status_t fbe_job_service_wait_for_expected_lifecycle_state_on_peer(fbe_object_id_t obj_id, 
                                                              fbe_lifecycle_state_t expected_lifecycle_state, 
                                                              fbe_u32_t timeout_ms)
{
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_lifecycle_state_t current_state = FBE_LIFECYCLE_STATE_NOT_EXIST;
    fbe_u32_t           current_time = 0;
    fbe_database_state_t   db_peer_state;

    status = fbe_job_service_get_database_peer_state(&db_peer_state);
    /*we have to take into account the peer may die any second so we have to take care of that*/
    while (current_time < timeout_ms) {
        if (fbe_cmi_is_peer_alive()) {

            /* If peer is alive and db is not ready, do not have to check for the life cycle state.  Just return. 
             */
            if (db_peer_state != FBE_DATABASE_STATE_READY) {
                return FBE_STATUS_OK;                
            }

            status = fbe_job_service_get_peer_object_state(obj_id, &current_state);
            if (status == FBE_STATUS_GENERIC_FAILURE) {
                job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                  "%s get lifecycle_state call failed for object id %d, status %d\n", 
                                  __FUNCTION__, obj_id, status);
                return status;
            }
            if ((status == FBE_STATUS_NO_OBJECT) && (expected_lifecycle_state == FBE_LIFECYCLE_STATE_NOT_EXIST)) {
                return FBE_STATUS_OK;
            }

            if (current_state == expected_lifecycle_state){
                return FBE_STATUS_OK;
            }
            current_time = current_time + 100; 
            fbe_thread_delay(100);
        }else{
            return FBE_STATUS_OK;/*peer is dead, who cares*/
        }
    }
    job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                      "%s: object 0x%x expected state %d != current state %d in %d ms!\n", 
                      __FUNCTION__, obj_id, expected_lifecycle_state, current_state, timeout_ms);

    return FBE_STATUS_GENERIC_FAILURE;

}


fbe_status_t fbe_job_service_get_peer_object_state(fbe_object_id_t object_id, fbe_lifecycle_state_t * lifecycle_state)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_base_config_get_peer_lifecycle_state_t      get_lifecycle_state;   

    status = fbe_job_service_lib_send_control_packet (FBE_BASE_CONFIG_CONTROL_CODE_GET_PEER_LIFECYCLE_STATE,
                                                 &get_lifecycle_state,
                                                 sizeof(fbe_base_config_get_peer_lifecycle_state_t),
                                                 FBE_PACKAGE_ID_SEP_0,
                                                 FBE_SERVICE_ID_TOPOLOGY,
                                                 FBE_CLASS_ID_INVALID,
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB);
    if (status == FBE_STATUS_NO_OBJECT) {
        job_service_trace (FBE_TRACE_LEVEL_WARNING, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                           "%s: requested object 0x%x does not exist\n", __FUNCTION__, object_id);
        return status;
    }

    /* If object is in SPECIALIZE STATE topology will return FBE_STATUS_BUSY */
    if (status == FBE_STATUS_BUSY) {
        *lifecycle_state = FBE_LIFECYCLE_STATE_SPECIALIZE;
        return FBE_STATUS_OK;
    }

    if (status != FBE_STATUS_OK) {
        job_service_trace (FBE_TRACE_LEVEL_WARNING, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                           "%s:packet error:%d, \n", 
                           __FUNCTION__,
                           status);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }
   
    *lifecycle_state = get_lifecycle_state.peer_state;

    return status;

}

/*!**************************************************************
 * fbe_job_service_get_database_state()
 ****************************************************************
 * @brief
 * This function gets the state of the DB service.
 *
 * @param - none
 *
 * @return State of the Database service
 *
 * @author
 *
 ****************************************************************/
fbe_status_t fbe_job_service_get_database_peer_state(fbe_database_state_t *peer_state)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_database_control_get_state_t db_peer_state;

    db_peer_state.state = FBE_DATABASE_STATE_INVALID; // SAFEBUG - uninitialized fields seen to be used


    status = fbe_job_service_lib_send_control_packet (FBE_DATABASE_CONTROL_CODE_GET_PEER_STATE,
                                                 &db_peer_state,
                                                 sizeof(fbe_database_control_get_state_t),
                                                 FBE_PACKAGE_ID_SEP_0,
                                                 FBE_SERVICE_ID_DATABASE,
                                                 FBE_CLASS_ID_INVALID,
                                                 FBE_OBJECT_ID_INVALID,
                                                 FBE_PACKET_FLAG_NO_ATTRIB);

    if (status != FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_WARNING, 
                          FBE_TRACE_MESSAGE_ID_INFO, 
                          "%s: Get DB state failed, status:0x%x\n", 
                          __FUNCTION__, status);       
    }

    *peer_state = db_peer_state.state;

    return status;

} /* end of fbe_job_service_get_database_state */

