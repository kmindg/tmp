/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_job_service_process_functions.c
 ***************************************************************************
 *
 * @brief
 *  This file contains job service functions that process a queued element
 *  from either the Recovery queue or the Creation queue.
 *  
 *  The main goal of this file is to dequeue a request and run the request
 *  thru the job registration functions
 *
 * @version
 *  08/24/2010 - Created. Jesus Medina
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
#include "fbe_job_service_operations.h"
#include "fbe_job_service_cmi.h"
#include "../test/fbe_job_service_test_private.h"

fbe_job_service_operation_t null_list = {FBE_JOB_TYPE_INVALID, 0};

/*! List of consumers of the Job Service
*/
static fbe_job_service_operation_t* job_service_operation_list[] =
{
    &fbe_spare_job_service_operation,
    &fbe_test_job_service_operation,
    &fbe_lun_create_job_service_operation,
    &fbe_lun_destroy_job_service_operation,    
    &fbe_raid_group_create_job_service_operation,
    &fbe_raid_group_destroy_job_service_operation,
    &fbe_set_system_ps_info_job_service_operation,
    &fbe_update_raid_group_job_service_operation,
    &fbe_update_virtual_drive_job_service_operation,
    &fbe_update_provision_drive_job_service_operation,
    &fbe_create_provision_drive_job_service_operation,
    &fbe_update_spare_config_job_service_operation,
    &fbe_update_lun_job_service_operation,
    &fbe_update_db_on_peer_job_service_operation,
    &fbe_destroy_provision_drive_job_service_operation,
    &fbe_ndu_commit_job_service_operation,
    &fbe_reinitialize_pvd_job_service_operation,
    &fbe_set_system_time_threshold_job_service_operation,
    &fbe_control_system_bg_service_job_service_operation,
    &fbe_drive_connect_job_service_operation,
    &fbe_update_multi_pvds_pool_id_job_service_operation,
    &fbe_update_spare_library_config_operation,
    &fbe_set_system_encryption_info_job_service_operation,
    &fbe_update_encryption_mode_job_service_operation,
    &fbe_process_encryption_keys_job_service_operation,
    &fbe_scrub_old_user_data_job_service_operation,
    &fbe_set_encryption_paused_job_service_operation,
    &fbe_validate_database_job_service_operation,
    &fbe_create_extent_pool_job_service_operation,
    &fbe_create_extent_pool_lun_job_service_operation,
    &fbe_destroy_extent_pool_job_service_operation,
    &fbe_destroy_extent_pool_lun_job_service_operation,
    &fbe_update_provision_drive_block_size_job_service_operation,
    &null_list
}; 

/*************************
 *    INLINE FUNCTIONS 
 *************************/


/*************************
 *   FUNCTION DEFINITIONS
 *************************/

static fbe_bool_t fbe_job_service_process_validation_function(fbe_job_queue_element_t *);
static fbe_bool_t fbe_job_service_process_selection_function(fbe_job_queue_element_t * );
static fbe_bool_t fbe_job_service_process_update_in_memory_function(fbe_job_queue_element_t *);
static fbe_bool_t fbe_job_service_process_rollback_function(fbe_job_queue_element_t *);
static fbe_bool_t fbe_job_service_process_persist_function(fbe_job_queue_element_t *);
static fbe_bool_t fbe_job_service_process_commit_function(fbe_job_queue_element_t *);
static void fbe_job_service_set_element_timestamp(fbe_job_queue_element_t *job_queue_element_p,
        fbe_time_t timestamp);
static fbe_status_t fbe_job_service_packet_completion(fbe_packet_t * packet, 
        fbe_packet_completion_context_t context);
static void fbe_job_service_process_queue_element(fbe_job_queue_element_t * job_queue_element_p);
static fbe_job_action_state_t fbe_job_service_get_next_function_state(fbe_job_action_state_t current_state);
static void fbe_job_service_set_state(fbe_job_action_state_t *previous_state, fbe_job_action_state_t *current_state,
        fbe_job_action_state_t set_state);
fbe_status_t fbe_job_service_translate_function_name(fbe_job_action_state_t state,
        const char ** pp_function_name);

/*!**************************************************************
 * fbe_job_service_process_queue_element()
 ****************************************************************
 * @brief
 * Process job control entry object, this function starts the
 * processing of Job service client's functions
 *
 * @param fbe_job_queue_element_t - entry of queue          
 *
 * @return - none
 *
 * @author
 * 10/22/2009 - Created. Jesus Medina
 *
 ****************************************************************/

void fbe_job_service_process_queue_element( 
        fbe_job_queue_element_t * job_queue_element_p) 
{ 
    fbe_payload_ex_t                      *sep_payload_p = NULL;
    fbe_payload_control_operation_t        *control_operation = NULL;
    fbe_payload_control_status_t           payload_status = FBE_PAYLOAD_CONTROL_STATUS_FAILURE;
    fbe_job_action_state_t                 next_state = FBE_JOB_ACTION_STATE_INVALID; 
    fbe_status_t                           status = FBE_STATUS_OK; 
    fbe_bool_t                             function_called = TRUE; 
    EMCPAL_STATUS                          sem_status = EMCPAL_STATUS_SUCCESS; 
    EMCPAL_LARGE_INTEGER                          timeout, *p_timeout; 
    const char                             *p_function_str = NULL;

    timeout.QuadPart = FBE_JOB_SERVICE_PACKET_TIMEOUT; 
    p_timeout = &timeout; 

    sep_payload_p = fbe_transport_get_payload_ex (fbe_job_service.job_packet_p);
    control_operation = fbe_payload_ex_allocate_control_operation(sep_payload_p);

    fbe_payload_control_build_operation (control_operation,
                                         job_queue_element_p->job_type,
                                         job_queue_element_p,
                                         sizeof(fbe_job_queue_element_t));

    /* Mark packet with the attribute, either travvalidation_functionerse or not */
    fbe_transport_set_packet_attr(fbe_job_service.job_packet_p, FBE_PACKET_FLAG_NO_ATTRIB);
    fbe_payload_ex_increment_control_operation_level(sep_payload_p);
 
    while(job_queue_element_p->current_state != FBE_JOB_ACTION_STATE_DONE) 
    { 
        /* we need to determine if a call to a function was actually job_queue_element_p made so that we set 
         * a timer for it, otherwise we transition to next function, function_called will 
         * indicate if the call to a client's function has been made 
         */ 
        function_called = FALSE; 

        /* Check the hook at the start */
        fbe_job_service_hook_check_hook_and_take_action(job_queue_element_p, FBE_TRUE);
        switch(job_queue_element_p->current_state) 
        { 
            case FBE_JOB_ACTION_STATE_VALIDATE: 
                function_called = fbe_job_service_process_validation_function(job_queue_element_p); 
                break; 

            case FBE_JOB_ACTION_STATE_SELECTION: 
                function_called = fbe_job_service_process_selection_function(job_queue_element_p); 
                break; 

            case FBE_JOB_ACTION_STATE_UPDATE_IN_MEMORY: 
                function_called = fbe_job_service_process_update_in_memory_function(job_queue_element_p); 
                break; 

            case FBE_JOB_ACTION_STATE_PERSIST:
                function_called = fbe_job_service_process_persist_function(job_queue_element_p); 
                break; 

            case FBE_JOB_ACTION_STATE_ROLLBACK: 
                /* this case is only called when previous function returns packet 
                 * status with error. 
                 * 
                 * NOTE: A Job Service client must always register a rollback function 
                 */ 
                function_called = fbe_job_service_process_rollback_function(job_queue_element_p);
                break; 

            case FBE_JOB_ACTION_STATE_COMMIT: 
                function_called = fbe_job_service_process_commit_function(job_queue_element_p);
                break; 

            default: 
                job_service_trace(FBE_TRACE_LEVEL_ERROR, 
                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                        "%s Invalid function state %d\n", 
                        __FUNCTION__, job_queue_element_p->current_state); 
        } 

        /* Check the hook at the end */
        fbe_job_service_hook_check_hook_and_take_action(job_queue_element_p, FBE_FALSE);

        /* only wait for the reply from client's function if job service 
         * called the client's function. if client's function called then 
         * if packet status is ok, then transition to next function based 
         * on next state, if status is not ok then set up for response 
         * call so exit of loop will be executed once response call has 
         * been processed 
         */ 
        if (function_called)
        { 
            /* if we are here, we called a client's function, wait on the reply 
            */ 
            while ((sem_status = fbe_semaphore_wait(&fbe_job_service.job_control_process_semaphore, 
                            p_timeout)) != EMCPAL_STATUS_SUCCESS) 
            { 
                if (sem_status == EMCPAL_STATUS_TIMEOUT) 
                { 
                    /* we need to cancel the packet, and wait until we are 
                     * completed on the cancellation 
                     */ 
                    fbe_transport_cancel_packet(fbe_job_service.job_packet_p); 
                    p_timeout = NULL; 
                } 
            } 

            status = fbe_transport_get_status_code(fbe_job_service.job_packet_p); 
#if 0 
            if (status == FBE_STATUS_CANCELED) 
            { 
                /* packet was canceled, set current state to done to exit loop 
                */ 
                job_queue_element_p->current_state = FBE_JOB_ACTION_STATE_DONE; 
            } 

            /* packet was not cancelled, but if status != ok, then set up to call 
             * rollback function to send error back to client, then exit loop 
             */ 
#endif 

            sep_payload_p = fbe_transport_get_payload_ex (fbe_job_service.job_packet_p);
            control_operation = fbe_payload_ex_get_control_operation(sep_payload_p);
            fbe_payload_control_get_status(control_operation, &payload_status);

            fbe_job_service_translate_function_name(job_queue_element_p->current_state,
                                                    &p_function_str);

            job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                    "%s entry\n", __FUNCTION__); 
            job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                    "          registered function 0x%x(%s) \n",
                    job_queue_element_p->current_state, p_function_str);

            job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                    " returned packet status %d, payload status %d \n", status, payload_status);

            /* if function called is rollback, rollback's status must always be ok, 
             * regardless if rollback in function client failed 
             */
            if ((status == FBE_STATUS_OK) || (status == FBE_STATUS_MORE_PROCESSING_REQUIRED))
            {
                /* packet status is ok, verify payload status */
                if (payload_status == FBE_PAYLOAD_CONTROL_STATUS_FAILURE)
                {
                    if (job_queue_element_p->current_state == FBE_JOB_ACTION_STATE_ROLLBACK) 
                    {
                        fbe_job_service_set_state(&job_queue_element_p->previous_state, 
                                                  &job_queue_element_p->current_state, FBE_JOB_ACTION_STATE_DONE);
                    }
                    else
                    {
                        /* payload status not ok, notify user via rollback function */
                        fbe_job_service_set_state(&job_queue_element_p->previous_state, 
                                                  &job_queue_element_p->current_state, FBE_JOB_ACTION_STATE_ROLLBACK);

                    }
                 }
                 else
                 {
                     /* payload status ok, move to next function, based on next state */
                     next_state = fbe_job_service_get_next_function_state(job_queue_element_p->current_state);
                     fbe_job_service_set_state(&job_queue_element_p->previous_state, 
                             &job_queue_element_p->current_state, next_state);
                 }
             }
             else
             {
                 if (job_queue_element_p->current_state == FBE_JOB_ACTION_STATE_ROLLBACK) 
                 { 
                     fbe_job_service_set_state(&job_queue_element_p->previous_state, 
                             &job_queue_element_p->current_state, FBE_JOB_ACTION_STATE_DONE);
                 } 
                 else 
                 { 
                     fbe_job_service_set_state(&job_queue_element_p->previous_state, 
                             &job_queue_element_p->current_state, FBE_JOB_ACTION_STATE_ROLLBACK);
                 } 
             }
        } 
        else 
        { 
            /* we did not call the client's function since it was defined as NULL, 
             * transition to next function based on next state 
             */ 
            next_state = fbe_job_service_get_next_function_state(job_queue_element_p->current_state);
            fbe_job_service_set_state(&job_queue_element_p->previous_state, 
                    &job_queue_element_p->current_state, next_state);
        } 
    } /* of while state !- done */ 

    fbe_job_service_set_state(&job_queue_element_p->previous_state, 
            &job_queue_element_p->current_state, FBE_JOB_ACTION_STATE_DONE);
    fbe_payload_ex_release_control_operation(sep_payload_p, control_operation);
    return; 
} 
/********************************************** 
 * end fbe_job_service_process_queue_element() 
 **********************************************/ 

/*!**************************************************************
 * fbe_job_service_process_recovery_queue()
 ****************************************************************
 * @brief
 * runs requests from the recovery queue
 *
 * @param - none             
 *
 * @return - none
 * 
 * @author
 * 10/01/2009 - Created. Jesus Medina
 *
 ****************************************************************/

void fbe_job_service_process_recovery_queue(void)
{
    fbe_job_queue_element_t        *job_queue_element_p = NULL;
    fbe_queue_head_t               *head_p = NULL;
    const char                     *p_job_type_str = NULL;

    job_service_trace(FBE_TRACE_LEVEL_INFO, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "job_proc_rec_que entry\n");

    /* Prevent things from changing while we are processing the queue.
    */
    fbe_job_service_lock();
    fbe_job_service_get_recovery_queue_head(&head_p);
    job_queue_element_p = (fbe_job_queue_element_t *) fbe_queue_front(head_p);
    if(job_queue_element_p == NULL)
    {
        job_service_trace(FBE_TRACE_LEVEL_INFO, 
                          FBE_TRACE_MESSAGE_ID_INFO, 
                          "Job element from Recovery Q is NULL...\n");

        fbe_job_service_unlock();
        return;
    }

    fbe_queue_remove((fbe_queue_element_t *)job_queue_element_p);
    fbe_job_service_decrement_number_recovery_objects();

    fbe_job_service_translate_job_type(job_queue_element_p->job_type, &p_job_type_str);

    job_service_trace(FBE_TRACE_LEVEL_INFO, 
        FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
        "   Dequeued recovery job type : %s \n", 
        p_job_type_str);

    /* Unlock now since the queue element can now be processed */
    fbe_job_service_unlock();

    job_service_trace(FBE_TRACE_LEVEL_INFO, 
        FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
        "   start processing for job number 0x%x \n",
        (unsigned int)job_queue_element_p->job_number); 
    
    if(job_queue_element_p->current_state == FBE_JOB_ACTION_STATE_DONE)
    {
        job_queue_element_p->previous_state = FBE_JOB_ACTION_STATE_PERSIST;
        job_queue_element_p->current_state = FBE_JOB_ACTION_STATE_COMMIT;
        job_queue_element_p->status = FBE_STATUS_OK;
        job_queue_element_p->error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;
    }
    else
    {
        job_queue_element_p->previous_state = FBE_JOB_ACTION_STATE_INVALID;
        job_queue_element_p->current_state = FBE_JOB_ACTION_STATE_VALIDATE;
    }

    fbe_job_service_process_queue_element(job_queue_element_p);

    job_service_trace(FBE_TRACE_LEVEL_INFO, 
        FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
        "   completed processing for job number 0x%x \n",
        (unsigned int)job_queue_element_p->job_number); 

    job_service_trace(FBE_TRACE_LEVEL_INFO, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "   requesting peer to remove job number 0x%llx, from recovery queue\n",
            (unsigned long long)job_queue_element_p->job_number);
      
    if (job_queue_element_p->error_code != FBE_JOB_SERVICE_ERROR_NO_ERROR)
    {
        job_service_trace(FBE_TRACE_LEVEL_INFO, 
                FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "   job number 0x%llx obj: 0x%x status 0x%x error_code %d\n",
                job_queue_element_p->job_number,
                job_queue_element_p->object_id,
                job_queue_element_p->status,
                job_queue_element_p->error_code);
    }
    else
    {
        job_service_trace(FBE_TRACE_LEVEL_INFO, 
                FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "   job number 0x%llx obj: 0x%x status 0x%x - success.\n",
                job_queue_element_p->job_number, 
                job_queue_element_p->object_id,
                job_queue_element_p->status);
    }

    /* prepare message for peer */
    fbe_job_service.job_service_cmi_message_exec_p->job_number = job_queue_element_p->job_number;
    fbe_job_service.job_service_cmi_message_exec_p->status = job_queue_element_p->status;
    fbe_job_service.job_service_cmi_message_exec_p->error_code = job_queue_element_p->error_code;
    fbe_job_service.job_service_cmi_message_exec_p->object_id = job_queue_element_p->object_id;
    fbe_job_service.job_service_cmi_message_exec_p->class_id = job_queue_element_p->class_id;
    fbe_job_service.job_service_cmi_message_exec_p->queue_type = FBE_JOB_RECOVERY_QUEUE;
    fbe_job_service.job_service_cmi_message_exec_p->msg.job_service_queue_element.job_element.job_number = job_queue_element_p->job_number;
    fbe_job_service.job_service_cmi_message_exec_p->msg.job_service_queue_element.job_element.job_type = job_queue_element_p->job_type;
    fbe_job_service.job_service_cmi_message_exec_p->msg.job_service_queue_element.job_element.object_id = job_queue_element_p->object_id;
    fbe_job_service.job_service_cmi_message_exec_p->msg.job_service_queue_element.job_element.class_id = job_queue_element_p->class_id;
    fbe_job_service.job_service_cmi_message_exec_p->msg.job_service_queue_element.job_element.command_size = job_queue_element_p->command_size;
    fbe_copy_memory(&fbe_job_service.job_service_cmi_message_exec_p->msg.job_service_queue_element.job_element.command_data,
                    job_queue_element_p->command_data, job_queue_element_p->command_size);

    fbe_job_service.job_service_cmi_message_exec_p->job_service_cmi_message_type = 
        FBE_JOB_CONTROL_SERVICE_CMI_MESSAGE_TYPE_REMOVE_FROM_RECOVERY_QUEUE;

    /* now send message to peer SP */	
    fbe_job_service_cmi_send_message(fbe_job_service.job_service_cmi_message_exec_p, NULL);
    
    if ((job_queue_element_p->local_job_data_valid == FBE_TRUE) &&
        (job_queue_element_p->additional_job_data != NULL))
    {
        fbe_memory_ex_release(job_queue_element_p->additional_job_data);
        job_queue_element_p->additional_job_data = NULL;
    }

    /*! when reaching this point we are done with the processing
    * of a job control element, release the element
    */
    fbe_transport_release_buffer(job_queue_element_p);

    /*! reuse packet */
    fbe_transport_reuse_packet(fbe_job_service.job_packet_p);
    return;
}
/*****************************************************
 * end fbe_job_service_process_recovery_queue()
 *****************************************************/

/*!**************************************************************
 * fbe_job_service_process_creation_queue()
 ****************************************************************
 * @brief
 * runs requests from the creation queue
 *
 * @param - none             
 *
 * @return - none
 * 
 * @author
 * 12/29/2009 - Created. Jesus Medina
 *
 ****************************************************************/

void fbe_job_service_process_creation_queue(void)
{
    fbe_job_queue_element_t  *job_queue_element_p = NULL;
    const char               *p_job_type_str = NULL;

    job_service_trace(FBE_TRACE_LEVEL_INFO, 
        FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
        "%s entry \n", __FUNCTION__);

    /* Prevent things from changing while we are processing the queue.
    */
    fbe_job_service_lock();

    job_queue_element_p = (fbe_job_queue_element_t *)
        fbe_queue_front(&fbe_job_service.creation_q_head);

    if(job_queue_element_p == NULL)
    {
        job_service_trace(FBE_TRACE_LEVEL_INFO, 
                          FBE_TRACE_MESSAGE_ID_INFO, 
                          "Job element from Creation Q is NULL...\n");

        fbe_job_service_unlock();
        return;
    }

    fbe_queue_pop(&fbe_job_service.creation_q_head);
    fbe_job_service_decrement_number_creation_objects();
    fbe_job_service_translate_job_type(job_queue_element_p->job_type, &p_job_type_str);

    job_service_trace(FBE_TRACE_LEVEL_INFO, 
        FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
        "   Dequeued creation job type : %s \n", 
        p_job_type_str);

    /* Unlock while we process this job request. */
    fbe_job_service_unlock();

    job_service_trace(FBE_TRACE_LEVEL_INFO, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "   start processing for job number 0x%x \n",
            (unsigned int)job_queue_element_p->job_number);

    if(job_queue_element_p->current_state == FBE_JOB_ACTION_STATE_DONE)
    {
        job_queue_element_p->previous_state = FBE_JOB_ACTION_STATE_PERSIST;
        job_queue_element_p->current_state = FBE_JOB_ACTION_STATE_COMMIT;
    }
    else
    {
        job_queue_element_p->previous_state = FBE_JOB_ACTION_STATE_INVALID;
        job_queue_element_p->current_state = FBE_JOB_ACTION_STATE_VALIDATE;
    }


    fbe_job_service_process_queue_element(job_queue_element_p);

    job_service_trace(FBE_TRACE_LEVEL_INFO, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "   completed processing for job number 0x%llx \n",
            (unsigned long long)job_queue_element_p->job_number);

    /* prepare message for peer */
    job_service_trace(FBE_TRACE_LEVEL_INFO, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "   requesting peer to remove job number from creation queue\n");

    if (job_queue_element_p->error_code != FBE_JOB_SERVICE_ERROR_NO_ERROR)
    {
        job_service_trace(FBE_TRACE_LEVEL_INFO, 
                FBE_TRACE_MESSAGE_ID_INFO,
                "   job number 0x%llx obj: 0x%x status 0x%x error_code %d\n",
                job_queue_element_p->job_number, 
                job_queue_element_p->object_id,
                job_queue_element_p->status,
                job_queue_element_p->error_code);
    }
    else
    {
        job_service_trace(FBE_TRACE_LEVEL_INFO, 
                FBE_TRACE_MESSAGE_ID_INFO,
                "   job number 0x%x, status 0x%x, no error found on job service request\n",
                (unsigned int)job_queue_element_p->job_number, 
                job_queue_element_p->status);
    }

    fbe_job_service.job_service_cmi_message_exec_p->job_number = job_queue_element_p->job_number;
    fbe_job_service.job_service_cmi_message_exec_p->status = job_queue_element_p->status;
    fbe_job_service.job_service_cmi_message_exec_p->error_code = job_queue_element_p->error_code;
    fbe_job_service.job_service_cmi_message_exec_p->object_id = job_queue_element_p->object_id;
    fbe_job_service.job_service_cmi_message_exec_p->class_id = job_queue_element_p->class_id;
    fbe_job_service.job_service_cmi_message_exec_p->queue_type = FBE_JOB_CREATION_QUEUE;
    fbe_job_service.job_service_cmi_message_exec_p->msg.job_service_queue_element.job_element.job_type = job_queue_element_p->job_type;
    fbe_job_service.job_service_cmi_message_exec_p->msg.job_service_queue_element.job_element.object_id = job_queue_element_p->object_id;

    fbe_job_service.job_service_cmi_message_exec_p->job_service_cmi_message_type = 
        FBE_JOB_CONTROL_SERVICE_CMI_MESSAGE_TYPE_REMOVE_FROM_CREATION_QUEUE;

    /* now send message to peer SP */	
    fbe_job_service_cmi_send_message(fbe_job_service.job_service_cmi_message_exec_p, NULL);

    if ((job_queue_element_p->local_job_data_valid == FBE_TRUE) &&
        (job_queue_element_p->additional_job_data != NULL))
    {
        fbe_memory_ex_release(job_queue_element_p->additional_job_data);
        job_queue_element_p->additional_job_data = NULL;
    }

    /*! when reaching this point we are done with the processing
     * of a job control element, release the element
     */
    fbe_transport_release_buffer(job_queue_element_p);

    /*! reuse packet
    */
    fbe_transport_reuse_packet(fbe_job_service.job_packet_p);
    return;
}
/*****************************************************
 * end fbe_job_service_process_creation_queue()
 *****************************************************/

/*!**************************************************************
 * fbe_job_service_process_validation_function()
 ****************************************************************
 * @brief
 * Process job control entry object, this function runs the
 * selection function
 *
 * @param job_queue_element_p - entry of recovery queue          
 *
 * @return fbe_bool_t - TRUE if call was made to client's function
 *                      FALSE otherwise
 *                        
 * @author
 * 10/22/2009 - Created. Jesus Medina
 *
 ****************************************************************/

fbe_bool_t fbe_job_service_process_validation_function(
        fbe_job_queue_element_t * job_queue_element_p)
{
    fbe_job_service_operation_t *job_service_operation_p = NULL;
    fbe_job_command_data_t      *command_data_p = NULL;
    fbe_bool_t                  function_called = FALSE;

    if (job_queue_element_p->object_id != FBE_OBJECT_ID_INVALID) {
        job_service_trace(FBE_TRACE_LEVEL_INFO, 
                FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "%s entry, object_id %d\n",
                __FUNCTION__, job_queue_element_p->object_id);
    }

    job_service_operation_p =
        fbe_job_service_find_operation(job_queue_element_p->job_type);

    if (job_service_operation_p->action.validation_function != NULL)
    {
        /* get the client's data 
        */
        command_data_p = (void *)job_queue_element_p->command_data;

        /* we need to set the completion functivalidation_functionon only for function calls
         * that were registered with the job service
         */
        fbe_transport_set_completion_function (fbe_job_service.job_packet_p,
                fbe_job_service_packet_validation_completion_function,
                &fbe_job_service.job_control_process_semaphore);    

        /* when we start processing of an element, we set the timestamp
        */
        fbe_job_service_set_element_timestamp(job_queue_element_p, fbe_get_time());

        function_called = TRUE;
        job_service_operation_p->action.validation_function(fbe_job_service.job_packet_p);
    }

    return function_called;
}
/****************************************************
 * end fbe_job_service_process_validation_function()
 ****************************************************/

/*!**************************************************************
 * fbe_job_service_process_selection_function()
 ****************************************************************
 * @brief
 * Process job control entry object, this function runs the
 * selection function
 *
 * @param job_queue_element_p - entry of recovery queue          
 *
 * @return fbe_bool_t - TRUE if call was made to client's function
 *                      FALSE otherwise
 *
 * @author
 * 10/22/2009 - Created. Jesus Medina
 *
 ****************************************************************/

fbe_bool_t fbe_job_service_process_selection_function(
        fbe_job_queue_element_t * job_queue_element_p)
{
    fbe_job_service_operation_t *job_service_operation_p = NULL;
    fbe_job_command_data_t      *command_data_p = NULL;
    fbe_bool_t                  function_called = FALSE;

    job_service_operation_p = 
        fbe_job_service_find_operation(job_queue_element_p->job_type);

    if (job_service_operation_p->action.selection_function != NULL)
    {
        /* get the client's data 
        */
        command_data_p = (void *)job_queue_element_p->command_data;

        fbe_transport_set_completion_function (fbe_job_service.job_packet_p,
                fbe_job_service_packet_selection_completion_function,
                &fbe_job_service.job_control_process_semaphore);  

        /* when we start processing of an element, we set the timestamp
        */
        fbe_job_service_set_element_timestamp(job_queue_element_p, fbe_get_time());

        function_called = TRUE;
        job_service_operation_p->action.selection_function(fbe_job_service.job_packet_p);
    }

    return function_called;
}
/****************************************************
 * end fbe_job_service_process_selection_function()
 ****************************************************/

/*!**************************************************************
 * fbe_job_service_process_update_in_memory_function()
 ****************************************************************
 * @brief
 * Process job control entry object, this function runs the
 * update_in_memory function
 *
 * @param job_queue_element_p  - entry of recovery queue          
 *
 * @return fbe_bool_t - TRUE if call was made to client's function
 *                      FALSE otherwise
 *
 * @author
 * 10/22/2009 - Created. Jesus Medina
 *
 ****************************************************************/

fbe_bool_t fbe_job_service_process_update_in_memory_function(
        fbe_job_queue_element_t * job_queue_element_p)
{
    fbe_job_command_data_t      *command_data_p = NULL;
    fbe_job_service_operation_t *job_service_operation_p = NULL;
    fbe_bool_t                  function_called = FALSE;

    job_service_operation_p = 
        fbe_job_service_find_operation(job_queue_element_p->job_type);

    if (job_service_operation_p->action.update_in_memory_function != NULL)
    {
        /* get the client's data 
        */
        command_data_p = (void *)job_queue_element_p->command_data;
        fbe_transport_set_completion_function (fbe_job_service.job_packet_p,
                fbe_job_service_packet_update_in_memory_completion_function,
                &fbe_job_service.job_control_process_semaphore);   

        /* when we start processing of an element, we set the timestamp
        */
        fbe_job_service_set_element_timestamp(job_queue_element_p, fbe_get_time());

        function_called = FALSE;
        job_service_operation_p->action.update_in_memory_function(fbe_job_service.job_packet_p);
        function_called = TRUE;
    }
    return function_called;
}
/*********************************************************
 * end fbe_job_service_process_update_in_memory_function()
 *********************************************************/

/*!**************************************************************
 * fbe_job_service_process_persist_function()
 ****************************************************************
 * @brief
 * Process job control entry object, this function runs the
 * persist function
 *
 * @param job_queue_element_p - entry of recovery queue          
 *
 * @return fbe_bool_t - TRUE if call was made to client's function
 *                      FALSE otherwise
 *
 * @author
 * 10/22/2009 - Created. Jesus Medina
 *
 ****************************************************************/

fbe_bool_t fbe_job_service_process_persist_function(
        fbe_job_queue_element_t * job_queue_element_p)
{
    fbe_job_service_operation_t *job_service_operation_p = NULL;
    fbe_job_command_data_t      *command_data_p = NULL;
    fbe_bool_t                  function_called = FALSE;

    job_service_operation_p = 
        fbe_job_service_find_operation(job_queue_element_p->job_type);

    if (job_service_operation_p->action.persist_function != NULL)
    {
        /* get the client's data 
        */
        command_data_p = (void *)job_queue_element_p->command_data;
        fbe_transport_set_completion_function (fbe_job_service.job_packet_p,
                fbe_job_service_packet_persist_completion_function,
                &fbe_job_service.job_control_process_semaphore);    

        /* when we start processing of an element, we set the timestamp
        */
        fbe_job_service_set_element_timestamp(job_queue_element_p, fbe_get_time());

        function_called = TRUE;
        job_service_operation_p->action.persist_function(fbe_job_service.job_packet_p);
    }
    return function_called;
}
/************************************************
 * end fbe_job_service_process_persist_function()
 ************************************************/

/*!**************************************************************
 * fbe_job_service_process_rollback_function()
 ****************************************************************
 * @brief
 * Process job control entry object, this function runs the
 * rollback function
 *
 * @param job_queue_element_p  - entry of recovery queue          
 *
 * @return fbe_bool_t - TRUE if call was made to client's function
 *                      FALSE otherwise
 *
 * @author
 * 10/22/2009 - Created. Jesus Medina
 *
 ****************************************************************/

fbe_bool_t fbe_job_service_process_rollback_function(
        fbe_job_queue_element_t * job_queue_element_p)
{
    fbe_job_service_operation_t *job_service_operation_p = NULL;
    fbe_job_command_data_t      *command_data_p = NULL;
    fbe_bool_t                  function_called = FALSE;

    job_service_operation_p = 
        fbe_job_service_find_operation(job_queue_element_p->job_type);

    if (job_service_operation_p->action.rollback_function != NULL)
    {
        /* get the client's data 
        */
        command_data_p = (void *)job_queue_element_p->command_data;
        fbe_transport_set_completion_function (fbe_job_service.job_packet_p,
                fbe_job_service_packet_rollback_completion_function,
                &fbe_job_service.job_control_process_semaphore);  

        /* when we start processing of an element, we set the timestamp
        */
        fbe_job_service_set_element_timestamp(job_queue_element_p, fbe_get_time());

        function_called = TRUE;
        job_service_operation_p->action.rollback_function(fbe_job_service.job_packet_p);
    }
    else
    {
        fbe_job_service_set_state(&job_queue_element_p->previous_state, 
                &job_queue_element_p->current_state, FBE_JOB_ACTION_STATE_DONE);

        //job_queue_element_p->state = FBE_JOB_ACTION_STATE_DONE;
    }
    return function_called;
}
/*************************************************
 * end fbe_job_service_process_rollback_function()
 *************************************************/

/*!**************************************************************
 * fbe_job_service_process_commit_function()
 ****************************************************************
 * @brief
 * Process job control entry object, this function runs the
 * commit function
 *
 * @param job_queue_element_p - entry of recovery queue          
 *
 * @return fbe_bool_t - TRUE if call was made to client's function
 *                      FALSE otherwise
 *
 * @author
 * 10/22/2009 - Created. Jesus Medina
 *
 ****************************************************************/

fbe_bool_t fbe_job_service_process_commit_function(
        fbe_job_queue_element_t * job_queue_element_p)
{
    fbe_job_service_operation_t *job_service_operation_p = NULL;
    fbe_job_command_data_t      *command_data_p = NULL;
    fbe_bool_t                  function_called = FALSE;

    job_service_operation_p = 
        fbe_job_service_find_operation(job_queue_element_p->job_type);

    if (job_service_operation_p->action.commit_function != NULL)
    {
        /* get the client's data 
        */
        command_data_p = (void *)job_queue_element_p->command_data;
        fbe_transport_set_completion_function (fbe_job_service.job_packet_p,
                fbe_job_service_packet_commit_completion_function,
                &fbe_job_service.job_control_process_semaphore);  

        /* when we start processing of an element, we set the timestamp
        */
        fbe_job_service_set_element_timestamp(job_queue_element_p, fbe_get_time());

        function_called = TRUE;
        job_service_operation_p->action.commit_function(fbe_job_service.job_packet_p);
    }
    else
    {
        fbe_job_service_set_state(&job_queue_element_p->previous_state, 
                &job_queue_element_p->current_state, FBE_JOB_ACTION_STATE_DONE);

        //job_queue_element_p->previous_state = job_queue_element_p->current_state;
        //job_queue_element_p->current_state = FBE_JOB_ACTION_STATE_DONE;
    }

    return function_called;
}
/***********************************************
 * end fbe_job_control_process_commit_function()
 ***********************************************/

/*!**************************************************************
 * fbe_job_service_find_operation()
 ****************************************************************
 * @brief
 * This function finds the operation structure for a request type.
 *
 * @param job_type - one of the predefined job service types             
 *
 * @return fbe_job_service_operation_t - pointer to a function
 *                                       list
 * @author
 * 10/01/2009 - Created. Jesus Medina
 *
 ****************************************************************/
fbe_job_service_operation_t* fbe_job_service_find_operation(fbe_job_type_t job_type)
{
    fbe_u32_t index = 0;

    fbe_job_service_operation_t* reg_p = job_service_operation_list[index];

    while(reg_p->job_type != FBE_JOB_TYPE_INVALID)
    {
        if (reg_p->job_type == job_type)
        {
            return reg_p;
        }

        index++;
        reg_p = job_service_operation_list[index];
    }

    return NULL;
} 
/**************************************
 * end fbe_job_service_find_operation()
 **************************************/

/*!**************************************************************
 * fbe_job_service_get_next_function_state()
 ****************************************************************
 * @brief
 * returns next function state based on current state
 *
 * @param fbe_job_action_state_t - current state          
 *
 * @return fbe_job_action_state_t - next state
 *                        
 * * @author
 * 11/09/2009 - Created. Jesus Medina
 *
 ****************************************************************/

fbe_job_action_state_t fbe_job_service_get_next_function_state(fbe_job_action_state_t current_state)
{
    fbe_job_action_state_t next_state = FBE_JOB_ACTION_STATE_INVALID;

    switch(current_state)
    {
        case FBE_JOB_ACTION_STATE_DEQUEUED:
        case FBE_JOB_ACTION_STATE_VALIDATE:
            next_state = FBE_JOB_ACTION_STATE_SELECTION;
            break;
        case FBE_JOB_ACTION_STATE_SELECTION:
            next_state = FBE_JOB_ACTION_STATE_UPDATE_IN_MEMORY;
            break;
        case FBE_JOB_ACTION_STATE_UPDATE_IN_MEMORY:
            next_state = FBE_JOB_ACTION_STATE_PERSIST;
            break;
        case FBE_JOB_ACTION_STATE_PERSIST:
            next_state = FBE_JOB_ACTION_STATE_COMMIT;
            break;
        case FBE_JOB_ACTION_STATE_ROLLBACK:
            next_state = FBE_JOB_ACTION_STATE_DONE;
            break;
        case FBE_JOB_ACTION_STATE_COMMIT:
        case FBE_JOB_ACTION_STATE_DONE:
            next_state = FBE_JOB_ACTION_STATE_DONE;
            break;
        default:
            break;
    }

    return next_state;
}
/***********************************************
 * end fbe_job_service_get_next_function_state()
 ************************************************/

/*!**************************************************************
 * fbe_job_service_set_state()
 ****************************************************************
 * @brief
 * set the previous state with current and current state with set
 * state
 *
 * @param fbe_job_action_state_t - previous state          
 * @param fbe_job_action_state_t - current state          
 * @param fbe_job_action_state_t - set state          
 *
 * @return none
 *                        
 * * @author
 * 02/02/2010 - Created. Jesus Medina
 *
 ****************************************************************/

static void fbe_job_service_set_state(fbe_job_action_state_t *previous_state, fbe_job_action_state_t *current_state,
        fbe_job_action_state_t set_state)
{
    *previous_state = *current_state;
    *current_state = set_state;
}
/*********************************
 * end fbe_job_service_set_state()
 *********************************/

/*!**************************************************************
 * fbe_job_service_set_element_timestamp()
 ****************************************************************
 * @brief
 * This function sets a recovery element timestamp with the
 * parameter passed
 *
 * @param job_queue_element_p - recovery queue element
 * @param timestamp             - the value to set the 
 *                                control's entry
 *
 * @return - none
 *
 * @author
 * 10/21/2009 - Created. Jesus Medina
 *
 ****************************************************************/
void fbe_job_service_set_element_timestamp(
        fbe_job_queue_element_t *job_queue_element_p,
        fbe_time_t                 timestamp)
{
    job_queue_element_p->timestamp = timestamp;
}
/*********************************************
 * end fbe_job_service_set_element_timestamp()
 *********************************************/

/*!**************************************************************
 * fbe_job_service_packet_generic_completion_function()
 ****************************************************************
 * @brief
 * This function completes a packet
 *
 * @param packet  - packet received as callback to completion function
 * @param context - job_control_client_semaphore
 *
 * @return 
 *
 * @author
 * 10/01/2009 - Created. Jesus Medina
 *
 ****************************************************************/
fbe_status_t fbe_job_service_packet_completion(
        fbe_packet_t                    *packet_p, 
        fbe_packet_completion_context_t context)
{
    fbe_semaphore_t * sem;

    job_service_trace(FBE_TRACE_LEVEL_DEBUG_LOW, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry\n", __FUNCTION__);

    sem = (fbe_semaphore_t *)context;
    fbe_semaphore_release(sem, 0, 1, FALSE);
    return FBE_STATUS_OK;
}
/**********************************************************
 * end fbe_job_service_packet_generic_completion_function()
 *********************************************************/

/*!**************************************************************
 * fbe_job_service_packet_validation_completion_function()
 ****************************************************************
 * @brief
 * This function completes a packet for the validation function
 *
 * @param packet  - packet received as callback to completion function
 * @param context - job_control_client_semaphore
 *
 * @return - always more processing required
 *
 * @author
 * 02/01/2010 - Created. Jesus Medina
 *
 ****************************************************************/
fbe_status_t fbe_job_service_packet_validation_completion_function(
        fbe_packet_t                    *packet_p, 
        fbe_packet_completion_context_t context)
{
    fbe_semaphore_t                        *sem;

    job_service_trace(FBE_TRACE_LEVEL_DEBUG_LOW, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry\n", __FUNCTION__);

    sem = (fbe_semaphore_t *)context;
    fbe_semaphore_release(sem, 0, 1, FALSE);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/*************************************************************
 * end fbe_job_service_packet_validation_completion_function()
 ************************************************************/

/*!**************************************************************
 * fbe_job_service_packet_selection_completion_function()
 ****************************************************************
 * @brief
 * This function completes a packet for the selection function
 *
 * @param packet  - packet received as callback to completion function
 * @param context - job_control_client_semaphore
 *
 * @return - always more processing required
 *
 * @author
 * 02/01/2010 - Created. Jesus Medina
 *
 ****************************************************************/
fbe_status_t fbe_job_service_packet_selection_completion_function(
        fbe_packet_t                    *packet_p, 
        fbe_packet_completion_context_t context)
{
    fbe_semaphore_t * sem;

    job_service_trace(FBE_TRACE_LEVEL_DEBUG_LOW, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry\n", __FUNCTION__);

    sem = (fbe_semaphore_t *)context;
    fbe_semaphore_release(sem, 0, 1, FALSE);
    return FBE_STATUS_OK;
}
/**********************************************************
 * end fbe_job_service_packet_selection_completion_function()
 *********************************************************/

/*!**************************************************************
 * fbe_job_service_packet_update_in_memory_completion_function()
 ****************************************************************
 * @brief
 * This function completes a packet for the update in memory function
 *
 * @param packet  - packet received as callback to completion function
 * @param context - job_control_client_semaphore
 *
 * @return - always more processing required
 *
 * @author
 * 02/01/2010 - Created. Jesus Medina
 *
 ****************************************************************/
fbe_status_t fbe_job_service_packet_update_in_memory_completion_function(
        fbe_packet_t                    *packet_p, 
        fbe_packet_completion_context_t context)
{
    fbe_semaphore_t * sem;

    job_service_trace(FBE_TRACE_LEVEL_DEBUG_LOW, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry\n", __FUNCTION__);

    sem = (fbe_semaphore_t *)context;
    fbe_semaphore_release(sem, 0, 1, FALSE);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/*******************************************************************
 * end fbe_job_service_packet_update_in_memory_completion_function()
 ******************************************************************/

/*!**************************************************************
 * fbe_job_service_packet_persist_completion_function()
 ****************************************************************
 * @brief
 * This function completes a packet for the persist function
 *
 * @param packet  - packet received as callback to completion function
 * @param context - job_control_client_semaphore
 *
 * @return - always more processing required
 *
 * @author
 * 02/01/2010 - Created. Jesus Medina
 *
 ****************************************************************/
fbe_status_t fbe_job_service_packet_persist_completion_function(
        fbe_packet_t                    *packet_p, 
        fbe_packet_completion_context_t context)
{
    fbe_semaphore_t * sem;

    job_service_trace(FBE_TRACE_LEVEL_DEBUG_LOW, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry\n", __FUNCTION__);

    sem = (fbe_semaphore_t *)context;
    fbe_semaphore_release(sem, 0, 1, FALSE);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/***********************************************************
 * end fbe_job_service_packet_persist_completion_function()
 ***********************************************************/

/*!**************************************************************
 * fbe_job_service_packet_rollback_completion_function()
 ****************************************************************
 * @brief
 * This function completes a packet for the rollback function
 *
 * @param packet  - packet received as callback to completion function
 * @param context - job_control_client_semaphore
 *
 * @return - always status ok
 *
 * @author
 * 02/01/2010 - Created. Jesus Medina
 *
 ****************************************************************/
fbe_status_t fbe_job_service_packet_rollback_completion_function(
        fbe_packet_t                    *packet_p, 
        fbe_packet_completion_context_t context)
{
    fbe_semaphore_t * sem;

    job_service_trace(FBE_TRACE_LEVEL_DEBUG_LOW, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry\n", __FUNCTION__);

    sem = (fbe_semaphore_t *)context;
    fbe_semaphore_release(sem, 0, 1, FALSE);
    return FBE_STATUS_OK;
}
/***********************************************************
 * end fbe_job_service_packet_rollback_completion_function()
 ***********************************************************/

/*!**************************************************************
 * fbe_job_service_packet_commit_completion_function()
 ****************************************************************
 * @brief
 * This function completes a packet for the commit function
 *
 * @param packet  - packet received as callback to completion function
 * @param context - job_control_client_semaphore
 *
 * @return - always status ok
 *
 * @author
 * 02/01/2010 - Created. Jesus Medina
 *
 ****************************************************************/
fbe_status_t fbe_job_service_packet_commit_completion_function(
        fbe_packet_t                    *packet_p, 
        fbe_packet_completion_context_t context)
{
    fbe_semaphore_t * sem;

    job_service_trace(FBE_TRACE_LEVEL_DEBUG_LOW, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry\n", __FUNCTION__);

    sem = (fbe_semaphore_t *)context;
    fbe_semaphore_release(sem, 0, 1, FALSE);
    return FBE_STATUS_OK;
}
/*********************************************************
 * end fbe_job_service_packet_commit_completion_function()
 *********************************************************/

/*!**************************************************************
 * fbe_job_service_translate_function_name()
 ****************************************************************
 * @brief
 * translates and returns a job service function name
 * to string
 *
 * @param control_code - state to translate to function name           
 *
 * @return translated_code - matching string for function state
 *
 * @author
 * 01/04/2010 - Created. Jesus Medina
 *
 ****************************************************************/

fbe_status_t fbe_job_service_translate_function_name(fbe_job_action_state_t state,
        const char ** pp_function_name)
{
    switch (state) {
        case FBE_JOB_ACTION_STATE_INIT:
            *pp_function_name = "FBE_JOB_ACTION_STATE_INIT";
            break;
        case FBE_JOB_ACTION_STATE_IN_QUEUE:
            *pp_function_name = "FBE_JOB_ACTION_STATE_IN_QUEUE";
            break;
        case FBE_JOB_ACTION_STATE_DEQUEUED:
            *pp_function_name = "FBE_JOB_ACTION_STATE_DEQUEUED";
            break;
        case FBE_JOB_ACTION_STATE_VALIDATE:
            *pp_function_name = "FBE_JOB_ACTION_STATE_VALIDATE";
            break;
        case FBE_JOB_ACTION_STATE_SELECTION:
            *pp_function_name = "FBE_JOB_ACTION_STATE_SELECTION";
            break;
        case FBE_JOB_ACTION_STATE_UPDATE_IN_MEMORY:
            *pp_function_name = "FBE_JOB_ACTION_STATE_UPDATE_IN_MEMORY";
            break;
        case FBE_JOB_ACTION_STATE_PERSIST:
            *pp_function_name = "FBE_JOB_ACTION_STATE_PERSIST";
            break;
        case FBE_JOB_ACTION_STATE_ROLLBACK:
            *pp_function_name = "FBE_JOB_ACTION_STATE_ROLLBACK";
            break;
        case FBE_JOB_ACTION_STATE_COMMIT:
            *pp_function_name = "FBE_JOB_ACTION_STATE_COMMIT";
            break;
        case FBE_JOB_ACTION_STATE_DONE:
            *pp_function_name = "FBE_JOB_ACTION_STATE_DONE";
            break;
        case FBE_JOB_ACTION_STATE_EXIT:
            *pp_function_name = "FBE_JOB_ACTION_STATE_EXIT";
            break;
        case FBE_JOB_ACTION_STATE_INVALID:
            *pp_function_name = "FBE_JOB_ACTION_STATE_INVALID";
            break;
        default:
            *pp_function_name = "UNKNOWN_JOB_ACTION_STATE";
            break;
    }
    return FBE_STATUS_OK;
}
/******************************************************
 * end fbe_job_service_translate_function_name()
 ******************************************************/






