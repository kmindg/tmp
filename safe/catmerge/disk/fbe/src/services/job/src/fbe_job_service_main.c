/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_job_service_main.c
 ***************************************************************************
 *
 * @brief
 *  This file contains job service functions which process a control request
 *  
 *  The main goal of this file is to enqueue job service requests to the
 *  appropriate job service queue
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
#include "fbe_job_service_cmi.h"
#include "fbe_spare.h"
#include "fbe_job_service_operations.h"
#include "fbe_cmi.h"

/* Declare our service methods */
static fbe_status_t fbe_job_service_control_entry(fbe_packet_t * packet_p); 
fbe_service_methods_t fbe_job_service_methods = {FBE_SERVICE_ID_JOB_SERVICE, fbe_job_service_control_entry};

static fbe_status_t fbe_job_service_translate_control_entry(fbe_payload_control_operation_opcode_t control_code,
        const char ** pp_control_code_name);


/*************************
 *    INLINE FUNCTIONS 
 *************************/


/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/*!**************************************************************
 * fbe_job_service_init()
 ****************************************************************
 * @brief
 * Initialize the job service.
 *
 * @param packet_p - The packet sent with the init request.
 *
 * @return fbe_status_t - Always FBE_STATUS_OK.
 *
 * @author
 * 10/01/2009 - Created. Jesus Medina
 *
 ****************************************************************/

fbe_status_t fbe_job_service_init(fbe_packet_t * packet_p)
{
    EMCPAL_STATUS       nt_status;
    fbe_status_t   status = FBE_STATUS_OK;

    job_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, 
                      "%s entry\n", __FUNCTION__);

    /* we need to make sure the request can fit into the memory we allocated */
    FBE_ASSERT_AT_COMPILE_TIME(sizeof(fbe_job_service_raid_group_create_t) < FBE_JOB_ELEMENT_CONTEXT_SIZE);
    FBE_ASSERT_AT_COMPILE_TIME(sizeof(fbe_job_service_create_provision_drive_t) < FBE_JOB_ELEMENT_CONTEXT_SIZE);
    FBE_ASSERT_AT_COMPILE_TIME(sizeof(fbe_job_queue_element_t) < FBE_MEMORY_CHUNK_SIZE);

    fbe_base_service_init((fbe_base_service_t *) &fbe_job_service);

    fbe_base_service_set_service_id((fbe_base_service_t *) &fbe_job_service, FBE_SERVICE_ID_JOB_SERVICE);

    /* Initialize job service lock
    */
    fbe_spinlock_init(&fbe_job_service.lock);

    /* Initialize job service arrival lock
    */
    fbe_spinlock_init(&fbe_job_service.arrival.arrival_lock);

    /* Initialize job service queues
    */
    fbe_queue_init(&fbe_job_service.recovery_q_head);
    fbe_queue_init(&fbe_job_service.creation_q_head);

    /* Initialize job service arrival queues
    */
    fbe_queue_init(&fbe_job_service.arrival.arrival_recovery_q_head);
    fbe_queue_init(&fbe_job_service.arrival.arrival_creation_q_head);
    fbe_job_service.arrival.outstanding_requests = 0;

    /* initialize counter for # of entries of each queue
    */
    fbe_job_service_init_number_recovery_request();
    fbe_job_service_init_number_creation_request();

    /* initialize counter for # of entries of each arrival queue
    */
    fbe_job_service_init_number_arrival_recovery_q_elements();
    fbe_job_service_init_number_arrival_creation_q_elements();

    /* Initialize the job service debug hook 
     */
    fbe_job_service_debug_hook_init();

    /* init job service packet
    */
    fbe_job_service.job_packet_p = fbe_transport_allocate_packet();

    if(fbe_job_service.job_packet_p == NULL)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, 
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s fbe_job_service_init, packet allocation failed\n",
                __FUNCTION__);

        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* start clean with peer dead notifications */
    fbe_job_service.peer_just_died = FBE_FALSE;

    /* initialize the sp state for future use to handle peer lost case */
    if (fbe_job_service_cmi_is_active())
    {
        fbe_job_service.sp_state_for_peer_lost_handle = FBE_CMI_STATE_ACTIVE;
    }
    else
    {
        fbe_job_service.sp_state_for_peer_lost_handle = FBE_CMI_STATE_PASSIVE;
    }

    /* allocate memory for arrival peer-to-peer messaging */
    fbe_job_service.job_service_cmi_message_p = (fbe_job_service_cmi_message_t *) 
        fbe_memory_native_allocate(sizeof(fbe_job_service_cmi_message_t));

    if (fbe_job_service.job_service_cmi_message_p == NULL) 
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, 
                FBE_TRACE_MESSAGE_ID_INFO,
                "%s: cannot allocate memory for Job service cmi messages\n", 
                __FUNCTION__);
        
        /* destroy job service packet */
        if (fbe_job_service.job_packet_p != NULL)
        {
            fbe_transport_release_packet(fbe_job_service.job_packet_p);
            fbe_job_service.job_packet_p = NULL;
        }

        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* It is very bad, but we have design problem, the only way to fix it quickly is to temporary make CMI syncronous */
    fbe_semaphore_init(&fbe_job_service.job_service_cmi_message_p->sem, 0, 1);

    /* allocate memory for execution thread peer-to-peer messaging */
    fbe_job_service.job_service_cmi_message_exec_p = (fbe_job_service_cmi_message_t *) 
        fbe_memory_native_allocate(sizeof(fbe_job_service_cmi_message_t));

    if (fbe_job_service.job_service_cmi_message_exec_p == NULL) 
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, 
                FBE_TRACE_MESSAGE_ID_INFO,
                "%s: cannot allocate memory for Job service cmi messages\n", 
                __FUNCTION__);
        
        /* destroy previously allocated memory */
        if (fbe_job_service.job_service_cmi_message_p != NULL) 
        {
            fbe_memory_native_release(fbe_job_service.job_service_cmi_message_p);	
        }

        /* destroy job service packet */
        if (fbe_job_service.job_packet_p != NULL)
        {
            fbe_transport_release_packet(fbe_job_service.job_packet_p);
            fbe_job_service.job_packet_p = NULL;
        }

        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* It is very bad, but we have design problem, the only way to fix it quickly is to temporary make CMI syncronous */
    fbe_semaphore_init(&fbe_job_service.job_service_cmi_message_exec_p->sem, 0, 1);

    /* init the job service's packet
    */
    fbe_transport_initialize_sep_packet (fbe_job_service.job_packet_p);

    /* set processing of queues default = FBE_TRUE */
    fbe_job_service_handle_queue_access(FBE_TRUE);

    /* init thread flag
    */
    fbe_job_service_thread_init_flags();

    /* init arrival thread flag
    */
    fbe_job_service_arrival_thread_init_flags();

    /* init the counting semaphore that defines access to the queues
    */
    fbe_job_service_init_control_queue_semaphore();

    /* init the counting semaphore that defines access to the arrival queues
    */
    fbe_job_service_init_arrival_semaphore();

    /* init the semaphore that defines interaction with job service clients
    */
    fbe_job_service_init_control_process_call_semaphore();

    /* init the job id value
    */
    fbe_job_service_set_job_number(0);

    /*register with the CMI service*/
    status = fbe_job_service_cmi_init();
    if (status != FBE_STATUS_OK) 
    {
        /* destroy previously allocated memory */
        if (fbe_job_service.job_service_cmi_message_p != NULL) 
        {
            fbe_memory_native_release(fbe_job_service.job_service_cmi_message_p);	
        }
        if (fbe_job_service.job_service_cmi_message_exec_p != NULL) 
        {
            fbe_memory_native_release(fbe_job_service.job_service_cmi_message_exec_p);	
        }

        /* destroy job service packet
        */
        if (fbe_job_service.job_packet_p != NULL)
        {
            fbe_transport_release_packet(fbe_job_service.job_packet_p);
            fbe_job_service.job_packet_p = NULL;
        }
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Start arrival job service arrival thread */
    nt_status = 
        fbe_thread_init(&fbe_job_service.arrival.job_service_arrival_thread_handle,
                "fbe_job_arv", job_service_arrival_thread_func, NULL);

    if (nt_status != EMCPAL_STATUS_SUCCESS) 
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s: fbe_thread_init failed\n", __FUNCTION__);

        /* destroy previously allocated memory */
        if (fbe_job_service.job_service_cmi_message_p != NULL) 
        {
            fbe_memory_native_release(fbe_job_service.job_service_cmi_message_p);	
        }
        if (fbe_job_service.job_service_cmi_message_exec_p != NULL) 
        {
            fbe_memory_native_release(fbe_job_service.job_service_cmi_message_exec_p);	
        }

        /* destroy job service packet */
        if (fbe_job_service.job_packet_p != NULL)
        {
            fbe_transport_release_packet(fbe_job_service.job_packet_p);
            fbe_job_service.job_packet_p = NULL;
        }

        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Start job control thread */
    nt_status = 
        fbe_thread_init(&fbe_job_service.thread_handle,
                "fbe_job_ctl", job_service_thread_func, NULL);

    if (nt_status != EMCPAL_STATUS_SUCCESS) 
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s: fbe_thread_init failed\n", __FUNCTION__);
        
        /* destroy previously allocated memory */
        if (fbe_job_service.job_service_cmi_message_p != NULL) 
        {
            fbe_memory_native_release(fbe_job_service.job_service_cmi_message_p);	
        }
        if (fbe_job_service.job_service_cmi_message_exec_p != NULL) 
        {
            fbe_memory_native_release(fbe_job_service.job_service_cmi_message_exec_p);	
        }

        /* destroy service packet */
        if (fbe_job_service.job_packet_p != NULL)
        {
            fbe_transport_release_packet(fbe_job_service.job_packet_p);
            fbe_job_service.job_packet_p = NULL;
        }

        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_cmi_mark_ready(FBE_CMI_CLIENT_ID_JOB_SERVICE);

    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}
/********************************************
 * end fbe_job_service_init()
 ********************************************/

/*!**************************************************************
 * fbe_job_service_destroy()
 ****************************************************************
 * @brief
 * Destroy the job service. First destroy the threads and then 
 * destroy the queues. This will gaurantee us that nobody can 
 * access the thread and add something to the queue when we are
 * in middle of destroy.
 *
 * @param packet_p - Packet for the destroy operation.          
 *
 * @return FBE_STATUS_OK - Destroy successful.
 *         FBE_STATUS_GENERIC_FAILURE - Destroy Failed.
 *
 * @author
 * 10/01/2009 - Created. Jesus Medina
 * 07/21/2011 - Modified. Arun S
 *
 ****************************************************************/

fbe_status_t fbe_job_service_destroy(fbe_packet_t * packet_p)
{
    fbe_u32_t   delay_ms = 500;
    fbe_u32_t   max_wait_ms = 60000;
    fbe_u32_t   current_wait_ms = 0;

    job_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry\n",
            __FUNCTION__);



    /* Mark JS as shutting down in order to stop in coming requests.  Then wait for outstanding arrivals to drain.
       This is needed since the thread state flags are not interlocked.  So it's possible for client's to signal the
       thread semaphores after they've been destroyed.  
    */
    fbe_atomic_or(&fbe_job_service.arrival.outstanding_requests, FBE_JOB_SERVICE_GATE_BIT);
    while ( (fbe_job_service.arrival.outstanding_requests & FBE_JOB_SERVICE_GATE_MASK) > 0  &&
             current_wait_ms < max_wait_ms)
    {
        job_service_trace(FBE_TRACE_LEVEL_INFO, 
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s waiting on %lld requests to finish \n",
                          __FUNCTION__, fbe_job_service.arrival.outstanding_requests & FBE_JOB_SERVICE_GATE_MASK);
        fbe_thread_delay(delay_ms);
        current_wait_ms += delay_ms;
    }
    if (current_wait_ms >= max_wait_ms)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, 
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s failed to drain oustanding arrival requests \n",
                          __FUNCTION__);
    }

    /* ************************************************/
    /*  job service arrival queues - Destroy thread   */
    /* ************************************************/

    fbe_job_service_arrival_thread_set_flag(FBE_JOB_SERVICE_ARRIVAL_THREAD_FLAGS_STOPPED); 
    fbe_semaphore_release(&fbe_job_service.arrival.job_service_arrival_queue_semaphore, 0, 1, FALSE);

    /* Wait for the arrival thread to exit and then destroy it. */
    fbe_thread_wait(&fbe_job_service.arrival.job_service_arrival_thread_handle);
    if(!fbe_job_service_arrival_thread_is_flag_set(FBE_JOB_SERVICE_ARRIVAL_THREAD_FLAGS_DONE))	                                           
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, 
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s job service arrival thread flag 0x%x is not set to FBE_JOB_SERVICE_ARRIVAL_THREAD_FLAGS_DONE.\n",
                          __FUNCTION__, fbe_job_service.arrival.arrival_thread_flags);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_thread_destroy(&fbe_job_service.arrival.job_service_arrival_thread_handle);
    fbe_semaphore_destroy(&fbe_job_service.arrival.job_service_arrival_queue_semaphore);

    /* *************************************************/
    /*  job service internal queues - Destroy thread   */
    /* *************************************************/

    fbe_job_service_thread_set_flag(FBE_JOB_SERVICE_THREAD_FLAGS_STOPPED); 
    fbe_semaphore_release(&fbe_job_service.job_control_queue_semaphore, 0, 1, FALSE);

    /* Wait for the thread to exit and then destroy it.
    */
    fbe_thread_wait(&fbe_job_service.thread_handle);
    if(!fbe_job_service_thread_is_flag_set(FBE_JOB_SERVICE_THREAD_FLAGS_DONE))
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, 
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s job service threads flag 0x%x is not set to FBE_JOB_SERVICE_THREAD_FLAGS_DONE.\n",
                          __FUNCTION__, fbe_job_service.thread_flags);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_thread_destroy(&fbe_job_service.thread_handle);
    fbe_semaphore_destroy(&fbe_job_service.job_control_process_semaphore);

    /* ********************************************************/
    /*  job service arrival queues - Destroy Queue and Lock   */
    /* ********************************************************/

    /*! lock access to queues */
    fbe_job_service_arrival_lock();

    /* Drain the queue before actually destroying it. */
    fbe_job_service_remove_all_objects_from_arrival_creation_q();

    /* can not unload service if there are elements in the queues, arrival and job service queue */
    if (fbe_job_service_get_number_arrival_creation_q_elements() > 0)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, 
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s: arrival_creation_q not empty, has 0x%x elements.\n",
                          __FUNCTION__, fbe_job_service_get_number_arrival_creation_q_elements());

        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);

        /*! unlock access to queues */
        fbe_job_service_arrival_unlock();
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Drain the queue before actually destroying it. */
    fbe_job_service_remove_all_objects_from_arrival_recovery_q();

    /* can not unload service if there are elements in the queues, recovery and job service queue */
    if (fbe_job_service_get_number_arrival_recovery_q_elements() > 0)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, 
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s: arrival_recovery_q not empty, has 0x%x elements.\n",
                          __FUNCTION__, fbe_job_service_get_number_arrival_recovery_q_elements());

        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);

        /*! unlock access to queues */
        fbe_job_service_arrival_unlock();
        return FBE_STATUS_GENERIC_FAILURE;
    }  

    /* destroy job service arrival queues. */
    fbe_queue_destroy(&fbe_job_service.arrival.arrival_recovery_q_head);
    fbe_queue_destroy(&fbe_job_service.arrival.arrival_creation_q_head);

    /*! unlock access to arrival queues */
    fbe_job_service_arrival_unlock();

    /* we no longer need the arrival lock, destroy it */
    fbe_spinlock_destroy(&fbe_job_service.arrival.arrival_lock);
    
    /* *********************************************************/
    /*  job service internal queues - Destroy Queue and Lock   */
    /* *********************************************************/

    /*! lock access to queues */
    fbe_job_service_lock();

    /* Drain the queues before actually destroying it. */
    fbe_job_service_remove_all_objects_from_creation_queue();

    /* can not unload service if there are elements in the queues */
    if (fbe_job_service_get_number_creation_requests() > 0)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, 
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s: Job has 0x%x creation_requests.\n",
                          __FUNCTION__, fbe_job_service_get_number_creation_requests());

        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);

        /*! unlock access to queues */
        fbe_job_service_unlock();
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Drain the queues before actually destroying it. */
    fbe_job_service_remove_all_objects_from_recovery_queue();

    /* can not unload service if there are elements in the queues */
    if (fbe_job_service_get_number_recovery_requests() > 0)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, 
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s: Job has 0x%x recovery_requests.\n",
                          __FUNCTION__, fbe_job_service_get_number_recovery_requests());
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);

        /*! unlock access to queues */
        fbe_job_service_unlock();
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* destroy job service queues. */
    fbe_queue_destroy(&fbe_job_service.recovery_q_head);
    fbe_queue_destroy(&fbe_job_service.creation_q_head);

    /*! unlock access to queues */
    fbe_job_service_unlock();

    /* we no longer need the job service lock, destroy it */
    fbe_spinlock_destroy(&fbe_job_service.lock);

    /*****************************************************/

    fbe_semaphore_destroy(&fbe_job_service.job_control_queue_semaphore);

    /* destroy service packet */
    if (fbe_job_service.job_packet_p != NULL)
    {
        fbe_transport_release_packet(fbe_job_service.job_packet_p);
    }

    /* destroy previously allocated memory */
    if (fbe_job_service.job_service_cmi_message_p != NULL) 
    {
        /* It is very bad, but we have design problem, the only way to fix it quickly is to temporary make CMI syncronous */
        fbe_semaphore_destroy(&fbe_job_service.job_service_cmi_message_p->sem);

        fbe_memory_native_release(fbe_job_service.job_service_cmi_message_p);	
    }
    if (fbe_job_service.job_service_cmi_message_exec_p != NULL) 
    {
        /* It is very bad, but we have design problem, the only way to fix it quickly is to temporary make CMI syncronous */
        fbe_semaphore_destroy(&fbe_job_service.job_service_cmi_message_exec_p->sem);

        fbe_memory_native_release(fbe_job_service.job_service_cmi_message_exec_p);	
    }

    fbe_job_service_debug_hook_destroy();
    job_service_trace(FBE_TRACE_LEVEL_INFO, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s, number_recovery_object %d, number creation job %d, job number 0x%llX\n",
            __FUNCTION__, 
            fbe_job_service.number_recovery_objects,
            fbe_job_service.number_recovery_objects,
            (unsigned long long)fbe_job_service.job_number);

    fbe_job_service_cmi_destroy();
    fbe_base_service_destroy((fbe_base_service_t *) &fbe_job_service);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);

    return FBE_STATUS_OK;

}
/********************************************
 * end fbe_job_service_destroy()
 ********************************************/

/*!**************************************************************
 * fbe_job_service_control_entry()
 ****************************************************************
 * @brief
 * This is the main entry point for the job service.
 *
 * @param packet_p - Packet of the control operation.
 *
 * @return fbe_status_t.
 *
 * @author
 * 10/01/2009 - Created. Jesus Medina
 *
 ****************************************************************/

static fbe_status_t fbe_job_service_control_entry(fbe_packet_t * packet_p)
{
    fbe_status_t                           status;
    fbe_payload_control_operation_opcode_t control_code;
    const char                             *p_control_code_str = NULL;

    status = FBE_STATUS_OK;
    control_code = fbe_transport_get_control_code(packet_p);

    /* Handle the init control code first.
    */
    if(control_code == FBE_BASE_SERVICE_CONTROL_CODE_INIT) 
    {
        status = fbe_job_service_init(packet_p);
        return status;
    }

    if(!fbe_base_service_is_initialized((fbe_base_service_t *) &fbe_job_service))
    {
        fbe_transport_set_status(packet_p, FBE_STATUS_NOT_INITIALIZED, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_NOT_INITIALIZED;
    }

    /* do not translate control request that are not intended for the
     * job control service */
    if (control_code != FBE_BASE_SERVICE_CONTROL_CODE_DESTROY)
    {
        fbe_job_service_translate_control_entry(control_code, &p_control_code_str);
        job_service_trace(FBE_TRACE_LEVEL_INFO, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry \n", __FUNCTION__);

        job_service_trace(FBE_TRACE_LEVEL_INFO, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "control code 0x%x(%s) \n", 
            control_code, p_control_code_str);
    }

    switch(control_code) 
    {
        /* destroy the Job Service */
        case FBE_BASE_SERVICE_CONTROL_CODE_DESTROY:
            status = fbe_job_service_destroy(packet_p);
            break;

        case FBE_JOB_CONTROL_CODE_LUN_CREATE:
        case FBE_JOB_CONTROL_CODE_LUN_DESTROY:
        case FBE_JOB_CONTROL_CODE_LUN_UPDATE:
        case FBE_JOB_CONTROL_CODE_RAID_GROUP_CREATE:
        case FBE_JOB_CONTROL_CODE_RAID_GROUP_DESTROY:
        case FBE_JOB_CONTROL_CODE_CHANGE_SYSTEM_POWER_SAVING_INFO:
        case FBE_JOB_CONTROL_CODE_UPDATE_PROVISION_DRIVE:
        case FBE_JOB_CONTROL_CODE_UPDATE_PROVISION_DRIVE_BLOCK_SIZE:
        case FBE_JOB_CONTROL_CODE_REINITIALIZE_PROVISION_DRIVE:
        case FBE_JOB_CONTROL_CODE_CREATE_PROVISION_DRIVE:
        case FBE_JOB_CONTROL_CODE_DESTROY_PROVISION_DRIVE:
        case FBE_JOB_CONTROL_CODE_UPDATE_DB_ON_PEER:
        case FBE_JOB_CONTROL_CODE_CONTROL_SYSTEM_BG_SERVICE:
        case FBE_JOB_CONTROL_CODE_SEP_NDU_COMMIT:
        case FBE_JOB_CONTROL_CODE_CONNECT_DRIVE:
        case FBE_JOB_CONTROL_CODE_SET_SYSTEM_TIME_THRESHOLD_INFO:
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
            status = fbe_job_service_add_element_to_arrival_creation_queue(packet_p);
            break;	

        case FBE_JOB_CONTROL_CODE_DRIVE_SWAP:
        case FBE_JOB_CONTROL_CODE_UPDATE_VIRTUAL_DRIVE:
        case FBE_JOB_CONTROL_CODE_UPDATE_SPARE_LIBRARY_CONFIG:
            status = fbe_job_service_add_element_to_arrival_recovery_queue(packet_p);
            break;

        case FBE_JOB_CONTROL_CODE_DISABLE_PROCESSING_OF_RECOVERY_QUEUE:
        case FBE_JOB_CONTROL_CODE_ENABLE_PROCESSING_OF_RECOVERY_QUEUE:
            status = fbe_job_service_process_recovery_queue_access(packet_p);
            break;

        case FBE_JOB_CONTROL_CODE_DISABLE_PROCESSING_OF_CREATION_QUEUE:
        case FBE_JOB_CONTROL_CODE_ENABLE_PROCESSING_OF_CREATION_QUEUE:
            status = fbe_job_service_process_creation_queue_access(packet_p);
            break;

        case FBE_JOB_CONTROL_CODE_REMOVE_ELEMENT_FROM_RECOVERY_QUEUE:
            /* searches queue based on object id and removes element from queue
             * if object is found in queue 
             */
            status = fbe_job_service_find_and_remove_by_object_id_from_recovery_queue(packet_p);
            break;

        case FBE_JOB_CONTROL_CODE_REMOVE_ELEMENT_FROM_CREATION_QUEUE:
            /* searches queue based on object id and removes element from queue
             * if object is found in queue 
             */
            status = fbe_job_service_find_and_remove_by_object_id_from_creation_queue(packet_p);
            break;

        case FBE_JOB_CONTROL_CODE_SEARCH_OBJECT_IN_RECOVERY_QUEUE:
            status = fbe_job_service_find_object_in_recovery_queue(packet_p);
            break;

        case FBE_JOB_CONTROL_CODE_SEARCH_OBJECT_IN_CREATION_QUEUE:
            status = fbe_job_service_find_object_in_creation_queue(packet_p);
            break;

        case FBE_JOB_CONTROL_CODE_GET_NUMBER_OF_ELEMENTS_IN_RECOVERY_QUEUE:
            status = fbe_job_service_get_number_of_elements_in_recovery_queue(packet_p);
            break;

        case FBE_JOB_CONTROL_CODE_GET_NUMBER_OF_ELEMENTS_IN_CREATION_QUEUE:
            status = fbe_job_service_get_number_of_elements_in_creation_queue(packet_p);
            break;

        case FBE_JOB_CONTROL_CODE_GET_STATE:
            status = fbe_job_service_get_state(packet_p);
            break;
        case FBE_JOB_CONTROL_CODE_MARK_JOB_DONE:
            status = fbe_job_service_mark_job_done(packet_p);
            break;

        case FBE_JOB_CONTROL_CODE_ADD_DEBUG_HOOK:
            status = fbe_job_service_hook_add_debug_hook(packet_p);
            break;
        case FBE_JOB_CONTROL_CODE_GET_DEBUG_HOOK:
            status = fbe_job_service_hook_get_debug_hook(packet_p);
            break;
        case FBE_JOB_CONTROL_CODE_REMOVE_DEBUG_HOOK:
            status = fbe_job_service_hook_remove_debug_hook(packet_p);
            break;

        default:
            status = fbe_base_service_control_entry((fbe_base_service_t*)&fbe_job_service, packet_p);
            break;
    }

    return status;
}
/********************************************
 * end fbe_job_service_control_entry()
 ********************************************/

/*!**************************************************************
 * fbe_job_service_translate_control_entry()
 ****************************************************************
 * @brief
 * translates and returns a job service control code to string
 *
 * @param control_code - control_code to translate             
 *
 * @return translated_code - matching string for control code
 *
 * @author
 * 01/04/2010 - Created. Jesus Medina
 *
 ****************************************************************/

fbe_status_t fbe_job_service_translate_control_entry(fbe_payload_control_operation_opcode_t control_code,
        const char ** pp_control_code_name)
{
    *pp_control_code_name = NULL;
    switch (control_code) {
        case FBE_JOB_CONTROL_CODE_GET_STATE:
            *pp_control_code_name = "FBE_JOB_CONTROL_CODE_GET_STATE";
            break;
        case FBE_JOB_CONTROL_CODE_DRIVE_SWAP:
            *pp_control_code_name = "FBE_JOB_CONTROL_CODE_DRIVE_SWAP";
            break;
        case FBE_JOB_CONTROL_CODE_RAID_GROUP_CREATE:
            *pp_control_code_name = "FBE_JOB_CONTROL_CODE_RAID_GROUP_CREATE";
            break;
        case FBE_JOB_CONTROL_CODE_RAID_GROUP_DESTROY:
            *pp_control_code_name = "FBE_JOB_CONTROL_CODE_RAID_GROUP_DESTROY";
            break;
        case FBE_JOB_CONTROL_CODE_LUN_CREATE:
            *pp_control_code_name = "FBE_JOB_CONTROL_CODE_LUN_CREATE";
            break;
        case FBE_JOB_CONTROL_CODE_LUN_DESTROY:
            *pp_control_code_name = "FBE_JOB_CONTROL_CODE_LUN_DESTROY";
            break;
        case FBE_JOB_CONTROL_CODE_LUN_UPDATE:
            *pp_control_code_name = "FBE_JOB_CONTROL_CODE_LUN_UPDATE";
            break;
        case FBE_JOB_CONTROL_CODE_DISABLE_PROCESSING_OF_RECOVERY_QUEUE:
            *pp_control_code_name = "FBE_JOB_CONTROL_CODE_DISABLE_PROCESSING_OF_RECOVERY_QUEUE";
            break;
        case FBE_JOB_CONTROL_CODE_ENABLE_PROCESSING_OF_RECOVERY_QUEUE:
            *pp_control_code_name = "FBE_JOB_CONTROL_CODE_ENABLE_PROCESSING_OF_RECOVERY_QUEUE";
            break;
        case FBE_JOB_CONTROL_CODE_DISABLE_PROCESSING_OF_CREATION_QUEUE:
            *pp_control_code_name = "FBE_JOB_CONTROL_CODE_DISABLE_PROCESSING_OF_CREATION_QUEUE";
            break;
        case FBE_JOB_CONTROL_CODE_ENABLE_PROCESSING_OF_CREATION_QUEUE:
            *pp_control_code_name = "FBE_JOB_CONTROL_CODE_ENABLE_PROCESSING_OF_CREATION_QUEUE";
            break;
        case FBE_JOB_CONTROL_CODE_REMOVE_ELEMENT_FROM_RECOVERY_QUEUE:
            *pp_control_code_name = "FBE_JOB_CONTROL_CODE_REMOVE_ELEMENT_FROM_RECOVERY_QUEUE";
            break;
        case FBE_JOB_CONTROL_CODE_REMOVE_ELEMENT_FROM_CREATION_QUEUE:
            *pp_control_code_name = "FBE_JOB_CONTROL_CODE_REMOVE_ELEMENT_FROM_CREATION_QUEUE";
            break;
        case FBE_JOB_CONTROL_CODE_SEARCH_OBJECT_IN_RECOVERY_QUEUE:
            *pp_control_code_name = "FBE_JOB_CONTROL_CODE_SEARCH_OBJECT_IN_RECOVERY_QUEUE";
            break;
        case FBE_JOB_CONTROL_CODE_SEARCH_OBJECT_IN_CREATION_QUEUE:
            *pp_control_code_name = "FBE_JOB_CONTROL_CODE_SEARCH_OBJECT_IN_CREATION_QUEUE";
            break;
        case FBE_JOB_CONTROL_CODE_GET_NUMBER_OF_ELEMENTS_IN_RECOVERY_QUEUE:
            *pp_control_code_name = "FBE_JOB_CONTROL_CODE_GET_NUMBER_OF_ELEMENTS_IN_RECOVERY_QUEUE";
            break;
        case FBE_JOB_CONTROL_CODE_GET_NUMBER_OF_ELEMENTS_IN_CREATION_QUEUE:
            *pp_control_code_name = "FBE_JOB_CONTROL_CODE_GET_NUMBER_OF_ELEMENTS_IN_CREATION_QUEUE";
            break;
        case FBE_BASE_SERVICE_CONTROL_CODE_DESTROY:
            *pp_control_code_name = "FBE_BASE_SERVICE_CONTROL_CODE_DESTROY";
            break;
        case FBE_JOB_CONTROL_CODE_CHANGE_SYSTEM_POWER_SAVING_INFO:
            *pp_control_code_name = "FBE_JOB_CONTROL_CODE_CHANGE_SYSTEM_POWER_SAVING_INFO";
            break;
        case FBE_JOB_CONTROL_CODE_UPDATE_RAID_GROUP:
            *pp_control_code_name = "FBE_JOB_CONTROL_CODE_UPDATE_RAID_GROUP";
            break;
        case FBE_JOB_CONTROL_CODE_UPDATE_VIRTUAL_DRIVE:
            *pp_control_code_name = "FBE_JOB_CONTROL_CODE_UPDATE_VIRTUAL_DRIVE";
            break;
        case FBE_JOB_CONTROL_CODE_CREATE_PROVISION_DRIVE:
            *pp_control_code_name = "FBE_JOB_CONTROL_CODE_CREATE_PROVISION_DRIVE";
            break;
        case FBE_JOB_CONTROL_CODE_UPDATE_PROVISION_DRIVE:
            *pp_control_code_name = "FBE_JOB_CONTROL_CODE_UPDATE_PROVISION_DRIVE";
            break;
        case FBE_JOB_CONTROL_CODE_UPDATE_PROVISION_DRIVE_BLOCK_SIZE:
            *pp_control_code_name = "FBE_JOB_CONTROL_CODE_UPDATE_PROVISION_DRIVE_BLOCK_SIZE";
            break;
        case FBE_JOB_CONTROL_CODE_REINITIALIZE_PROVISION_DRIVE:
            *pp_control_code_name = "FBE_JOB_CONTROL_CODE_REINITIALIZE_PROVISION_DRIVE";
            break;
        case FBE_JOB_CONTROL_CODE_UPDATE_SPARE_CONFIG:
            *pp_control_code_name = "FBE_JOB_CONTROL_CODE_UPDATE_SPARE_CONFIG";
            break;
        case FBE_JOB_CONTROL_CODE_UPDATE_DB_ON_PEER:
            *pp_control_code_name = "FBE_JOB_CONTROL_CODE_UPDATE_DB_ON_PEER";
            break;
        case FBE_JOB_CONTROL_CODE_INVALID:
            *pp_control_code_name = "INVALID_FBE_JOB_CONTROL_CODE";
            break;
        case FBE_JOB_CONTROL_CODE_DESTROY_PROVISION_DRIVE:
            *pp_control_code_name = "FBE_JOB_CONTROL_CODE_DESTROY_PROVISION_DRIVE";
            break;
        case FBE_JOB_CONTROL_CODE_CONTROL_SYSTEM_BG_SERVICE:
            *pp_control_code_name = "FBE_JOB_CONTROL_CODE_CONTROL_SYSTEM_BG_SERVICE";
            break;
        case FBE_JOB_CONTROL_CODE_SEP_NDU_COMMIT:
            *pp_control_code_name = "FBE_JOB_CONTROL_CODE_SEP_NDU_COMMIT";
            break;
        case FBE_JOB_CONTROL_CODE_SET_SYSTEM_TIME_THRESHOLD_INFO:
            *pp_control_code_name = "FBE_JOB_CONTROL_CODE_SET_SYSTEM_TIME_THRESHOLD_INFO";
            break;
        case FBE_JOB_CONTROL_CODE_CONNECT_DRIVE:
            *pp_control_code_name = "FBE_JOB_CONTROL_CODE_CONNECT_DRIVE";
            break;
        case FBE_JOB_CONTROL_CODE_MARK_JOB_DONE:
            *pp_control_code_name = "FBE_JOB_CONTROL_CODE_MARK_JOB_DONE";
            break;
        case FBE_JOB_CONTROL_CODE_UPDATE_MULTI_PVDS_POOL_ID:
            *pp_control_code_name = "FBE_JOB_CONTROL_CODE_UPDATE_MULTI_PVDS_POOL_ID";
            break;
        case FBE_JOB_CONTROL_CODE_UPDATE_SPARE_LIBRARY_CONFIG:
            *pp_control_code_name = "FBE_JOB_CONTROL_CODE_UPDATE_SPARE_LIBRARY_CONFIG";
            break;
        case FBE_JOB_CONTROL_CODE_CHANGE_SYSTEM_ENCRYPTION_INFO:
            *pp_control_code_name = "FBE_JOB_CONTROL_CODE_CHANGE_SYSTEM_ENCRYPTION_INFO";
            break;
        case FBE_JOB_CONTROL_CODE_UPDATE_ENCRYPTION_MODE:
            *pp_control_code_name = "FBE_JOB_CONTROL_CODE_UPDATE_ENCRYPTION_MODE";
            break;
        case FBE_JOB_CONTROL_CODE_PROCESS_ENCRYPTION_KEYS:
            *pp_control_code_name = "FBE_JOB_CONTROL_CODE_PROCESS_ENCRYPTION_KEYS";
            break;
        case FBE_JOB_CONTROL_CODE_SCRUB_OLD_USER_DATA:
            *pp_control_code_name = "FBE_JOB_CONTROL_CODE_SCRUB_OLD_USER_DATA";
            break;
        case FBE_JOB_CONTROL_CODE_SET_ENCRYPTION_PAUSED:
            *pp_control_code_name = "FBE_JOB_CONTROL_CODE_SET_ENCRYPTION_PAUSED";
            break;
        case FBE_JOB_CONTROL_CODE_ADD_DEBUG_HOOK:
            *pp_control_code_name = "FBE_JOB_CONTROL_CODE_ADD_DEBUG_HOOK";
            break;
        case FBE_JOB_CONTROL_CODE_GET_DEBUG_HOOK:
            *pp_control_code_name = "FBE_JOB_CONTROL_CODE_GET_DEBUG_HOOK";
            break;
        case FBE_JOB_CONTROL_CODE_REMOVE_DEBUG_HOOK:
            *pp_control_code_name = "FBE_JOB_CONTROL_CODE_REMOVE_DEBUG_HOOK";
            break;
        case FBE_JOB_CONTROL_CODE_VALIDATE_DATABASE:
            *pp_control_code_name = "FBE_JOB_CONTROL_CODE_VALIDATE_DATABASE";
            break;
        case FBE_JOB_CONTROL_CODE_CREATE_EXTENT_POOL:
            *pp_control_code_name = "FBE_JOB_CONTROL_CODE_CREATE_EXTENT_POOL";
            break;
        case FBE_JOB_CONTROL_CODE_CREATE_EXTENT_POOL_LUN:
            *pp_control_code_name = "FBE_JOB_CONTROL_CODE_CREATE_EXTENT_POOL_LUN";
            break;
        case FBE_JOB_CONTROL_CODE_DESTROY_EXTENT_POOL:
            *pp_control_code_name = "FBE_JOB_CONTROL_CODE_DESTROY_EXTENT_POOL";
            break;
        case FBE_JOB_CONTROL_CODE_DESTROY_EXTENT_POOL_LUN:
            *pp_control_code_name = "FBE_JOB_CONTROL_CODE_DESTROY_EXTENT_POOL_LUN";
            break;
        default:
            *pp_control_code_name = "UNKNOWN_FBE_JOB_CONTROL_CODE";
            break;
    }
    return FBE_STATUS_OK;
}
/***********************************************
 * end fbe_job_service_translate_control_entry()
 **********************************************/

/*!**************************************************************
 * fbe_job_service_fill_provision_drive_create_request()
 ****************************************************************
 * @brief
 * process a job control request to create a provision drive.  
 * The function validate the packet, allocate a job_queue_element, 
 * copy the payload to the job_queue_element and complete the packet. 
 *
 * @param packet_p - packet containing provision drive create request.             
 *
 * @return fbe_status_t - Status of processing of the request
 *
 ****************************************************************/

fbe_status_t fbe_job_service_fill_provision_drive_create_request(fbe_packet_t * packet_p, fbe_job_queue_element_t * job_queue_element_p)
{
    fbe_job_service_create_provision_drive_t     *pvd_create_req_p = NULL;
    fbe_payload_ex_t                                *payload_p = NULL;
    fbe_payload_control_operation_opcode_t        control_code;
    fbe_payload_control_operation_t              *control_operation = NULL;
    fbe_payload_control_buffer_length_t           length;
    fbe_status_t                                  status = FBE_STATUS_OK;

    job_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry\n", __FUNCTION__);

    /*! Get the request payload.
    */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload_p);
    control_code = fbe_transport_get_control_code(packet_p);

    fbe_payload_control_get_buffer(control_operation, &pvd_create_req_p);
    if(pvd_create_req_p == NULL)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s : ERROR: Lun create job request is NULL\n",__FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &length);
    if(length != sizeof(fbe_job_service_create_provision_drive_t))
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s : ERROR: Size of provision drive create job request is invalid\n",__FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    //lun_create_req_p->job_number = job_queue_element_p->job_number;

    /*! set the queue element initial parameters */
    status = fbe_job_service_build_queue_element(job_queue_element_p,
                                                 control_code,
                                                 FBE_OBJECT_ID_INVALID,
                                                 (fbe_u8_t *)pvd_create_req_p,
                                                 sizeof(fbe_job_service_create_provision_drive_t));

    if (status != FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_WARNING, 
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s: Job service operation pointer is null, status 0x%x \n",
                __FUNCTION__, status);
    }

    return status;
}
/*!**************************************************************
 * fbe_job_service_fill_provision_drive_destroy_request()
 ****************************************************************
 * @brief
 * process a job control request to destroy a provision drive.  
 * The function validate the packet, allocate a job_queue_element, 
 * copy the payload to the job_queue_element and complete the packet. 
 *
 * @param packet_p - packet containing provision drive destroy request.             
 *
 * @return fbe_status_t - Status of processing of the request
 *
 ****************************************************************/

fbe_status_t fbe_job_service_fill_provision_drive_destroy_request(fbe_packet_t * packet_p, fbe_job_queue_element_t * job_queue_element_p)
{
    fbe_job_service_destroy_provision_drive_t     *pvd_destroy_req_p = NULL;
    fbe_payload_ex_t                                *payload_p = NULL;
    fbe_payload_control_operation_opcode_t        control_code;
    fbe_payload_control_operation_t              *control_operation = NULL;
    fbe_payload_control_buffer_length_t           length;
    fbe_status_t                                  status = FBE_STATUS_OK;

    job_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry\n", __FUNCTION__);

    /*! Get the request payload.
    */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload_p);
    control_code = fbe_transport_get_control_code(packet_p);

    fbe_payload_control_get_buffer(control_operation, &pvd_destroy_req_p);
    if(pvd_destroy_req_p == NULL)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s : ERROR: PVD destroy job request is NULL\n",__FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &length);
    if(length != sizeof(fbe_job_service_destroy_provision_drive_t))
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
        "%s : ERROR: Size of provision drive destroy job request is invalid\n",__FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*! set the queue element initial parameters */
    status = fbe_job_service_build_queue_element(job_queue_element_p,
                                                 control_code,
                                                 FBE_OBJECT_ID_INVALID,
                                                 (fbe_u8_t *)pvd_destroy_req_p,
                                                 sizeof(fbe_job_service_destroy_provision_drive_t));

    if (status != FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_WARNING, 
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s: Job service operation pointer is null, status 0x%x \n",
                __FUNCTION__, status);
    }

    return status;
}

/*!**************************************************************
 * fbe_job_service_fill_lun_create_request()
 ****************************************************************
 * @brief
 * process a job control request to create a lun.  
 * The function validate the packet, allocate a job_queue_element, 
 * copy the payload to the job_queue_element and complete the packet. 
 *
 * @param packet_p - packet containing lun create request.             
 *
 * @return fbe_status_t - Status of the processing of the request
 *
 * @author
 * 10/13/2010 - Created. Jesus Medina
 *
 ****************************************************************/

fbe_status_t fbe_job_service_fill_lun_create_request(fbe_packet_t * packet_p, fbe_job_queue_element_t * job_queue_element_p)
{
    fbe_job_service_lun_create_t           *lun_create_req_p = NULL;
    fbe_payload_ex_t                          *payload_p = NULL;
    fbe_payload_control_operation_opcode_t control_code;
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

    fbe_payload_control_get_buffer(control_operation, &lun_create_req_p);
    if(lun_create_req_p == NULL)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s : ERROR: Lun create job request is NULL\n",__FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &length);
    if(length != sizeof(fbe_job_service_lun_create_t))
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s : ERROR: Size of lun create job request is invalid\n",__FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    //lun_create_req_p->job_number = job_queue_element_p->job_number;

    /*! set the queue element initial parameters */
    status = fbe_job_service_build_queue_element(job_queue_element_p,
                                                 control_code,
                                                 FBE_OBJECT_ID_INVALID,
                                                 (fbe_u8_t *)lun_create_req_p,
                                                 sizeof(fbe_job_service_lun_create_t));

    if (status != FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_WARNING, 
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s: Job service operation pointer is null, status 0x%x \n",
                __FUNCTION__, status);
    }

    return status;
}
/*****************************************************
 * end fbe_job_service_fill_lun_create_request()
 *****************************************************/

/*!**************************************************************
 * fbe_job_service_set_job_number_on_lun_create_request()
 ****************************************************************
 * @brief
 * *
 * @param packet_p - packet containing lun create request.             
 *
 * @return fbe_status_t - Status of the processing of the request
 *
 * @author
 * 09/23/2010 - Created. Jesus Medina
 *
 ****************************************************************/

void fbe_job_service_set_job_number_on_lun_create_request(fbe_packet_t * packet_p, 
                                                          fbe_job_queue_element_t * job_queue_element_p)
{
    fbe_job_service_lun_create_t           *lun_create_req_p = NULL;
    fbe_payload_ex_t                          *payload_p = NULL;
    fbe_payload_control_operation_t        *control_operation = NULL;

    job_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry\n", __FUNCTION__);

    /*! Get the request payload.
    */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload_p);

    fbe_payload_control_get_buffer(control_operation, &lun_create_req_p);
    lun_create_req_p->job_number = job_queue_element_p->job_number;
    return;
}
/************************************************************
 * end fbe_job_service_set_job_number_on_lun_create_request()
 ***********************************************************/

/*!**************************************************************
 * fbe_job_service_fill_lun_destroy_request()
 ****************************************************************
 * @brief
 * process a job control request to destroy a lun.  
 * This function validates the packet, allocates a job_queue_element, 
 * copies the payload to the job_queue_element and completes the packet. 
 *
 * @param packet_p - packet containing lun destroy request.             
 *
 * @return fbe_status_t - return status
 *
 * @author
 * 01/2010 - Created. Susan Rundbaken
 *
 ****************************************************************/

fbe_status_t fbe_job_service_fill_lun_destroy_request(fbe_packet_t *packet_p, fbe_job_queue_element_t *job_queue_element_p)
{
    fbe_job_service_lun_destroy_t           *lun_destroy_req_p;
    fbe_payload_ex_t                           *payload_p;
    fbe_payload_control_operation_opcode_t  control_code;
    fbe_payload_control_operation_t         *control_operation;
    fbe_payload_control_buffer_length_t     length;
    fbe_payload_control_status_t            payload_status;
    fbe_status_t                            status;

    job_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry\n", __FUNCTION__);

    /*! Initialize local variables
    */
    lun_destroy_req_p   = NULL;
    payload_p           = NULL;
    control_operation   = NULL;
    status              = FBE_STATUS_OK;
    payload_status      = FBE_PAYLOAD_CONTROL_STATUS_OK;


    /*! Get the request payload
    */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload_p);
    control_code = fbe_transport_get_control_code(packet_p);

    fbe_payload_control_get_buffer(control_operation, &lun_destroy_req_p);
    if(lun_destroy_req_p == NULL)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s : ERROR: Lun destroy job request is NULL\n",__FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &length);
    if(length != sizeof(fbe_job_service_lun_destroy_t))
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s : ERROR: Size of lun destroy job request is invalid\n",__FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    //lun_destroy_req_p->job_number = job_queue_element_p->job_number;

    /*! set the queue element initial parameters */
    status = fbe_job_service_build_queue_element(job_queue_element_p,
                                                 control_code,
                                                 FBE_OBJECT_ID_INVALID,
                                                 (fbe_u8_t *)lun_destroy_req_p,
                                                 sizeof(fbe_job_service_lun_destroy_t));

    if(status != FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_WARNING,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s: Job service operation pointer is null, status 0x%x \n",
                          __FUNCTION__, status);
    }

    return status;
}
/*****************************************************
 * end fbe_job_service_fill_lun_destroy_request()
 *****************************************************/

/*!**************************************************************
 * fbe_job_service_set_job_number_on_lun_destroy_request()
 ****************************************************************
 * @brief 
 *
 * @param packet_p - packet containing lun destroy request.             
 *
 * @return fbe_status_t - return status
 *
 * @author
 * 09//23/2010 - Created. Susan Rundbaken
 *
 ****************************************************************/

void fbe_job_service_set_job_number_on_lun_destroy_request(fbe_packet_t *packet_p, 
                                                           fbe_job_queue_element_t *job_queue_element_p)
{
    fbe_job_service_lun_destroy_t           *lun_destroy_req_p = NULL;
    fbe_payload_ex_t                           *payload_p = NULL;
    fbe_payload_control_operation_t         *control_operation = NULL;

    job_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry\n", __FUNCTION__);

    /*! Get the request payload
    */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload_p);

    fbe_payload_control_get_buffer(control_operation, &lun_destroy_req_p);
    lun_destroy_req_p->job_number = job_queue_element_p->job_number;
    return;
}
/*************************************************************
 * end fbe_job_service_set_job_number_on_lun_destroy_request()
 ************************************************************/

/*!**************************************************************
 * fbe_job_service_set_job_number_on_lun_update_request()
 ****************************************************************
 * @brief 
 *
 * @param packet_p - packet containing lun update request.             
 *
 * @return fbe_status_t - return status
 *
 * @author
 * 09//23/2010 - Created. Susan Rundbaken
 *
 ****************************************************************/

void fbe_job_service_set_job_number_on_lun_update_request(fbe_packet_t *packet_p, 
                                                           fbe_job_queue_element_t *job_queue_element_p)
{
    fbe_job_service_lun_update_t           *lun_update_req_p = NULL;
    fbe_payload_ex_t                           *payload_p = NULL;
    fbe_payload_control_operation_t         *control_operation = NULL;

    job_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry\n", __FUNCTION__);

    /*! Get the request payload
    */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload_p);

    fbe_payload_control_get_buffer(control_operation, &lun_update_req_p);
    lun_update_req_p->job_number = job_queue_element_p->job_number;
    return;
}
/*************************************************************
 * end fbe_job_service_set_job_number_on_lun_update_request()
 ************************************************************/
/*!**************************************************************
 * fbe_job_service_fill_lun_update_request()
 ****************************************************************
 * @brief
 * process a job control request to update a lun.  
 * This function validates the packet, allocates a job_queue_element, 
 * copies the payload to the job_queue_element and completes the packet. 
 *
 * @param packet_p - packet containing lun update request.             
 *
 * @return fbe_status_t - return status
 *
 * @author
 * 01/2010 - Created. Susan Rundbaken
 *
 ****************************************************************/

fbe_status_t fbe_job_service_fill_lun_update_request(fbe_packet_t *packet_p, fbe_job_queue_element_t *job_queue_element_p)
{
    fbe_job_service_lun_update_t            *lun_update_req_p;
    fbe_payload_ex_t                           *payload_p;
    fbe_payload_control_operation_opcode_t  control_code;
    fbe_payload_control_operation_t         *control_operation;
    fbe_payload_control_buffer_length_t     length;
    fbe_payload_control_status_t            payload_status;
    fbe_status_t                            status;

    job_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry\n", __FUNCTION__);

    /*! Initialize local variables
    */
    lun_update_req_p   = NULL;
    payload_p           = NULL;
    control_operation   = NULL;
    status              = FBE_STATUS_OK;
    payload_status      = FBE_PAYLOAD_CONTROL_STATUS_OK;


    /*! Get the request payload
    */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload_p);
    control_code = fbe_transport_get_control_code(packet_p);

    fbe_payload_control_get_buffer(control_operation, &lun_update_req_p);
    if(lun_update_req_p == NULL)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s : ERROR: Lun update job request is NULL\n",__FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &length);
    if(length != sizeof(fbe_job_service_lun_update_t))
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s : ERROR: Size of lun update job request is invalid\n",__FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*! set the queue element initial parameters */
    status = fbe_job_service_build_queue_element(job_queue_element_p,
                                                 control_code,
                                                 FBE_OBJECT_ID_INVALID,
                                                 (fbe_u8_t *)lun_update_req_p,
                                                 sizeof(fbe_job_service_lun_update_t));

    if(status != FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_WARNING,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s: Job service operation pointer is null, status 0x%x \n",
                          __FUNCTION__, status);
    }

    return status;
}
/*****************************************************
 * end fbe_job_service_fill_lun_update_request()
 *****************************************************/

/*!**************************************************************
 * fbe_job_service_fill_raid_group_create_request()
 ****************************************************************
 * @brief
 * process a job control request to create a Raid Group.  
 * The function validate the packet, allocate a job_queue_element, 
 * copy the payload to the job_queue_element and complete the packet. 
 *
 * @param packet_p - packet containing raid group create request.             
 *
 * @return fbe_status_t - Status of the processing of the request
 *                        
 * @author
 * 10/15/2010 - Created. Jesus Medina
 *
 ****************************************************************/

fbe_status_t fbe_job_service_fill_raid_group_create_request(fbe_packet_t * packet_p, 
                                                            fbe_job_queue_element_t *job_queue_element_p)
{
    fbe_job_service_raid_group_create_t    *raid_group_create_req_p = NULL;
    fbe_payload_ex_t                          *payload_p = NULL;
    fbe_payload_control_operation_opcode_t control_code;
    fbe_payload_control_operation_t        *control_operation = NULL;
    fbe_payload_control_buffer_length_t    length;
    fbe_status_t                           status = FBE_STATUS_OK;

    job_service_trace(FBE_TRACE_LEVEL_DEBUG_LOW, 
        FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
        "%s entry\n",
        __FUNCTION__);

    /*! Get the request payload.
    */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload_p);
    control_code = fbe_transport_get_control_code(packet_p);

    fbe_payload_control_get_buffer(control_operation, &raid_group_create_req_p);
    if(raid_group_create_req_p == NULL)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s : ERROR: Raid group create job request is NULL\n",__FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &length);
    if(length != sizeof(fbe_job_service_raid_group_create_t))
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s : ERROR: Size of raid group job request is invalid\n",__FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    //raid_group_create_req_p->job_number = job_queue_element_p->job_number;
 
    /*! set the queue element initial parameters */
    status = fbe_job_service_build_queue_element(job_queue_element_p,
                                                 control_code,
                                                 FBE_OBJECT_ID_INVALID,
                                                 (fbe_u8_t *)raid_group_create_req_p,
                                                 sizeof(fbe_job_service_raid_group_create_t));

    if(status != FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, 
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s: Job service operation pointer is null, status 0x%x \n",
                __FUNCTION__, status);
    }
    return status;
}
/**********************************************************
 * end fbe_job_service_fill_raid_group_create_request()
 *********************************************************/

/*!**************************************************************
 * fbe_job_service_set_job_number_on_raid_group_create_request()
 ****************************************************************
 * @brief
 *
 * @param packet_p - packet containing raid group create request.             
 *
 * @return fbe_status_t - Status of the processing of the request
 *                        
 * @author
 * 09/23/2010 - Created. Jesus Medina
 *
 ****************************************************************/

void  fbe_job_service_set_job_number_on_raid_group_create_request(fbe_packet_t * packet_p, 
                                                                  fbe_job_queue_element_t *job_queue_element_p)
{
    job_service_raid_group_create_t        *raid_group_create_req_p = NULL;
    fbe_payload_ex_t                          *payload_p = NULL;
    fbe_payload_control_operation_t        *control_operation = NULL;

    job_service_trace(FBE_TRACE_LEVEL_DEBUG_LOW, 
        FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
        "job number to set for request %llu\n",
        (unsigned long long)job_queue_element_p->job_number);

    /*! Get the request payload.
    */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload_p);

    fbe_payload_control_get_buffer(control_operation, &raid_group_create_req_p);
    raid_group_create_req_p->user_input.job_number = job_queue_element_p->job_number;
    return;
}
/*******************************************************************
 * end fbe_job_service_set_job_number_on_raid_group_create_request()
 ******************************************************************/

/*!**************************************************************
 * fbe_job_service_fill_raid_group_destroy_request()
 ****************************************************************
 * @brief
 * process a job control request to destory a Raid Group.  
 * The function validate the packet, allocate a job_queue_element, 
 * copy the payload to the job_queue_element and complete the packet. 
 *
 * @param packet_p - packet containing raid group destroy request.             
 *
 * @return fbe_status_t - Status of the processing of the request
 *                        
 * @author
 * 02/15/2010 - Created. Jesus Medina
 *
 ****************************************************************/

fbe_status_t fbe_job_service_fill_raid_group_destroy_request(fbe_packet_t * packet_p, 
                                                             fbe_job_queue_element_t *job_queue_element_p)
{
    fbe_job_service_raid_group_destroy_t *raid_group_destroy_req_p = NULL;
    fbe_payload_ex_t                          *payload_p = NULL;
    fbe_payload_control_operation_opcode_t control_code;
    fbe_payload_control_operation_t        *control_operation = NULL;
    fbe_payload_control_buffer_length_t    length;
    fbe_status_t                           status = FBE_STATUS_OK;

    job_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry\n", __FUNCTION__);

    /*! Get the request payload. */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload_p);
    control_code = fbe_transport_get_control_code(packet_p);

    fbe_payload_control_get_buffer(control_operation, &raid_group_destroy_req_p);
    if(raid_group_destroy_req_p == NULL)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s : ERROR: Raid group destroy job request is NUL\n",__FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &length);
    if(length != sizeof(fbe_job_service_raid_group_destroy_t))
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s : ERROR: Size of raid group destroy job request is invalid\n",__FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    //raid_group_destroy_req_p->job_number = job_queue_element_p->job_number;

    /*! set the queue element initial parameters */
    status = fbe_job_service_build_queue_element(job_queue_element_p,
                                                 control_code,
                                                 FBE_OBJECT_ID_INVALID,
                                                 (fbe_u8_t *)raid_group_destroy_req_p,
                                                 sizeof(fbe_job_service_raid_group_destroy_t));

    if(status != FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_WARNING, 
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s: Job service operation pointer is null, status 0x%x \n",
                __FUNCTION__, status);
    }
    return status;
}
/****************************************************************
 * end fbe_job_service_fill_raid_group_destroy_request()
 ****************************************************************/


/*!**************************************************************
 * fbe_job_service_set_job_number_on_raid_group_destroy_request()
 ****************************************************************
 * @brief
 *
 * @param packet_p - packet containing raid group destroy request.             
 *
 * @return fbe_status_t - Status of the processing of the request
 *                        
 * @author
 * 09/23/2010 - Created. Jesus Medina
 *
 ****************************************************************/

void fbe_job_service_set_job_number_on_raid_group_destroy_request(fbe_packet_t * packet_p, 
                                                                  fbe_job_queue_element_t *job_queue_element_p)
{
    job_service_raid_group_destroy_t   *raid_group_destroy_req_p = NULL;
    fbe_payload_ex_t                          *payload_p = NULL;
    fbe_payload_control_operation_t        *control_operation = NULL;
 
    job_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry\n", __FUNCTION__);

    /*! Get the request payload. */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload_p);

    fbe_payload_control_get_buffer(control_operation, &raid_group_destroy_req_p);
    raid_group_destroy_req_p->user_input.job_number = job_queue_element_p->job_number;
    return;
}
/********************************************************************
 * end fbe_job_service_set_job_number_on_raid_group_destroy_request()
 *******************************************************************/


/*!**************************************************************
 * fbe_job_service_fill_drive_swap_request()
 ****************************************************************
 * @brief
 *  It process a job control request to perform the drive swap
 *  operation.
 *  It validates a packet and then create an job request element
 *  to queue it to the job service.
 *
 * @param packet_p              - packet for drive swap request.             
 * @param job_queue_element_p   - job service request.
 *
 * @return fbe_status_t         - Status of the operation.
 *
 * @author
 * 05/27/2010 - Created. Dhaval Patel
 *
 ****************************************************************/
fbe_status_t fbe_job_service_fill_drive_swap_request(fbe_packet_t * packet_p,
                                                     fbe_job_queue_element_t *job_queue_element_p)
{
    fbe_job_service_drive_swap_request_t *  drive_swap_request_p = NULL;
    fbe_payload_ex_t *                         payload_p = NULL;
    fbe_payload_control_operation_opcode_t  control_code;
    fbe_payload_control_operation_t *       control_operation = NULL;
    fbe_payload_control_buffer_length_t     length;
    fbe_status_t                            status = FBE_STATUS_OK;

    job_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry\n", __FUNCTION__);

    /* get the payload and control operation. */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload_p);
    control_code = fbe_transport_get_control_code(packet_p);

    /* get the drive swap request from the control operation. */
    fbe_payload_control_get_buffer(control_operation, &drive_swap_request_p);
    if(drive_swap_request_p == NULL)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s : ERROR: drive swap job request is NULL\n",__FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* verify buffer lenght for the drive swap request. */
    fbe_payload_control_get_buffer_length(control_operation, &length);
    if(length != sizeof(fbe_job_service_drive_swap_request_t))
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s : ERROR: Size of drive swap job request is invalid\n",__FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    //drive_swap_request_p->job_number = job_queue_element_p->job_number;

    /*! set the queue element initial parameters */
    status = fbe_job_service_build_queue_element(job_queue_element_p,
                                                 control_code,
                                                 FBE_OBJECT_ID_INVALID,
                                                 (fbe_u8_t *) drive_swap_request_p,
                                                 sizeof(fbe_job_service_drive_swap_request_t));
    if (status != FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_WARNING, 
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s: Job service operation pointer is null, status 0x%x \n",
                __FUNCTION__, status);
    }

    return status;
}
/****************************************************************
 * end fbe_job_service_fill_drive_swap_request()
 ****************************************************************/

/*!**************************************************************
 * fbe_job_service_set_job_number_on_drive_swap_request()
 ****************************************************************
 * @brief
 *
 * @param packet_p              - packet for drive swap request.             
 * @param job_queue_element_p   - job service request.
 *
 * @return fbe_status_t         - Status of the operation.
 *
 * @author
 * 05/27/2010 - Created. Dhaval Patel
 *
 ****************************************************************/
void fbe_job_service_set_job_number_on_drive_swap_request(fbe_packet_t * packet_p,
                                                          fbe_job_queue_element_t *job_queue_element_p)
{
    fbe_job_service_drive_swap_request_t *  drive_swap_request_p = NULL;
    fbe_payload_ex_t *                         payload_p = NULL;
    fbe_payload_control_operation_t *       control_operation = NULL;

    job_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry\n", __FUNCTION__);

    /* get the payload and control operation. */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload_p);

    /* get the drive swap request from the control operation. */
    fbe_payload_control_get_buffer(control_operation, &drive_swap_request_p);
    drive_swap_request_p->job_number = job_queue_element_p->job_number;
    return;
}
/************************************************************
 * end fbe_job_service_set_job_number_on_drive_swap_request()
 ***********************************************************/


/*!**************************************************************
 * fbe_job_service_fill_virtual_drive_update_request()
 ****************************************************************
 * @brief
 *  It process a job control request to perform the virtual drive
 *  update request.
 *  It validates a packet and then create an job request element
 *  to queue it to the job service.
 *
 * @param packet_p              - packet for drive swap request.
 * @param job_queue_element_p   - job service request.
 *
 * @return fbe_status_t         - Status of the operation.
 *
 * @author
 * 05/27/2010 - Created. Dhaval Patel
 *
 ****************************************************************/
fbe_status_t fbe_job_service_fill_virtual_drive_update_request(fbe_packet_t * packet_p,
                                                               fbe_job_queue_element_t *job_queue_element_p)
{
    fbe_job_service_update_virtual_drive_t *                update_virtual_drive_p = NULL;
    fbe_payload_ex_t *                                         payload_p = NULL;
    fbe_payload_control_operation_opcode_t                  control_code;
    fbe_payload_control_operation_t *                       control_operation = NULL;
    fbe_payload_control_buffer_length_t                     length;
    fbe_status_t                                            status = FBE_STATUS_OK;

    job_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry\n", __FUNCTION__);

    /* get the payload and control operation. */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload_p);
    control_code = fbe_transport_get_control_code(packet_p);

    /* get the drive swap request from the control operation. */
    fbe_payload_control_get_buffer(control_operation, &update_virtual_drive_p);
    if(update_virtual_drive_p == NULL) 
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s : ERROR: Virtual drive update job request is NULL\n",__FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* verify buffer lenght for the drive swap request. */
    fbe_payload_control_get_buffer_length(control_operation, &length);
    if(length != sizeof(fbe_job_service_update_virtual_drive_t))
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s : ERROR: Size of virtual drive update job request is invalid\n",__FUNCTION__);
 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    //update_vd_config_mode_p->job_number = job_queue_element_p->job_number;

    /*! set the queue element initial parameters */
    status = fbe_job_service_build_queue_element(job_queue_element_p,
                                                 control_code,
                                                 FBE_OBJECT_ID_INVALID,
                                                 (fbe_u8_t *) update_virtual_drive_p,
                                                 sizeof(fbe_job_service_update_virtual_drive_t));

    if (status != FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_WARNING, 
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s: Job service operation pointer is null, status 0x%x \n",
                __FUNCTION__, status);
    }

    return status;
}
/****************************************************************
 * end fbe_job_service_fill_virtual_drive_update_request()
 ****************************************************************/

/*!**************************************************************
 * fbe_job_service_set_job_number_on_virtual_drive_update_request()
 ****************************************************************
 * @brief
 *
 * @param packet_p              - packet for drive swap request.             
 * @param job_queue_element_p   - job service request.
 *
 * @return fbe_status_t         - Status of the operation.
 *
 * @author
 * 05/27/2010 - Created. Dhaval Patel
 *
 ****************************************************************/
void fbe_job_service_set_job_number_on_virtual_drive_update_request(fbe_packet_t * packet_p,
                                                                                fbe_job_queue_element_t *job_queue_element_p)
{
    fbe_job_service_update_virtual_drive_t *                update_virtual_drive_p = NULL;
    fbe_payload_ex_t *                                         payload_p = NULL;
    fbe_payload_control_operation_t *                       control_operation = NULL;

    job_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry\n", __FUNCTION__);

    /* get the payload and control operation. */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload_p);
    fbe_payload_control_get_buffer(control_operation, &update_virtual_drive_p);
    update_virtual_drive_p->job_number = job_queue_element_p->job_number;
    return;
}
/**********************************************************************
 * end fbe_job_service_set_job_number_on_virtual_drive_update_request()
 **********************************************************************/

/*!**************************************************************
 * fbe_job_service_fill_provision_drive_update_request()
 ****************************************************************
 * @brief
 *  It process a job control request to update the provision drive
 *  object.

 *  It validates a packet and then create an job request element
 *  to queue it to the job service.
 *
 * @param packet_p              - packet for drive swap request.
 * @param job_queue_element_p   - job service request.
 *
 * @return fbe_status_t         - Status of the operation.
 *
 * @author
 * 05/27/2010 - Created. Dhaval Patel
 *
 ****************************************************************/
fbe_status_t fbe_job_service_fill_provision_drive_update_request(fbe_packet_t * packet_p,
                                                                 fbe_job_queue_element_t *job_queue_element_p)
{
    fbe_job_service_update_provision_drive_t *                  update_provision_drive_p = NULL;
    fbe_payload_ex_t *                                             payload_p = NULL;
    fbe_payload_control_operation_opcode_t                      control_code;
    fbe_payload_control_operation_t *                           control_operation = NULL;
    fbe_payload_control_buffer_length_t                         length;
    fbe_status_t                                                status = FBE_STATUS_OK;

    job_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, "%s entry\n", __FUNCTION__);

    /* get the payload and control operation. */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload_p);
    control_code = fbe_transport_get_control_code(packet_p);

    /* get the drive swap request from the control operation. */
    fbe_payload_control_get_buffer(control_operation, &update_provision_drive_p);
    if(update_provision_drive_p == NULL)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s : ERROR: Provision drive update job request is NULL\n",__FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* verify buffer lenght for the drive swap request. */
    fbe_payload_control_get_buffer_length(control_operation, &length);
    if(length != sizeof(fbe_job_service_update_provision_drive_t))
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s : ERROR: Size of Provision drive update job request is invalid\n",__FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    //update_provision_drive_config_type_p->job_number = job_queue_element_p->job_number;

    /*! set the queue element initial parameters */
    status = fbe_job_service_build_queue_element(job_queue_element_p,
                                                 control_code,
                                                 FBE_OBJECT_ID_INVALID,
                                                 (fbe_u8_t *) update_provision_drive_p,
                                                 sizeof(fbe_job_service_update_provision_drive_t));

    if (status != FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_WARNING, 
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s: Job service operation pointer is null, status 0x%x \n",
                __FUNCTION__, status);
    }

    return status;
}
/***********************************************************
 * end fbe_job_service_fill_provision_drive_update_request()
 ***********************************************************/

/*!**************************************************************
 * fbe_job_service_set_job_number_on_provision_drive_update_request()
 ****************************************************************
 * @brief
 *
 * @param packet_p              - packet for drive swap request.             
 * @param job_queue_element_p   - job service request.
 *
 * @return fbe_status_t         - Status of the operation.
 *
 * @author
 * 05/27/2010 - Created. Dhaval Patel
 *
 ****************************************************************/
void fbe_job_service_set_job_number_on_provision_drive_update_request(fbe_packet_t * packet_p,
                                                                      fbe_job_queue_element_t *job_queue_element_p)
{
    fbe_job_service_update_provision_drive_t *                  update_provision_drive_p = NULL;
    fbe_payload_ex_t *                                             payload_p = NULL;
    fbe_payload_control_operation_t *                           control_operation = NULL;

    job_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, "%s entry\n", __FUNCTION__);

    /* get the payload and control operation. */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload_p);

    /* get the drive swap request from the control operation. */
    fbe_payload_control_get_buffer(control_operation, &update_provision_drive_p);
    update_provision_drive_p->job_number = job_queue_element_p->job_number;
    return;
}
/************************************************************************
 * end fbe_job_service_set_job_number_on_provision_drive_update_request()
 ************************************************************************/


/*!**************************************************************
 * fbe_job_service_fill_update_provision_drive_block_size_request()
 ****************************************************************
 * @brief
 *  Engineering only option to change PVD's block size 
 *
 *  It validates a packet and then create an job request element
 *  to queue it to the job service.
 *
 * @param packet_p              - packet for drive swap request.
 * @param job_queue_element_p   - job service request.
 *
 * @return fbe_status_t         - Status of the operation.
 *
 * @author
 * 05/27/2010 - Created. Dhaval Patel
 *
 ****************************************************************/
fbe_status_t fbe_job_service_fill_update_provision_drive_block_size_request(fbe_packet_t * packet_p, fbe_job_queue_element_t *job_queue_element_p)
{
    fbe_job_service_update_pvd_block_size_t *                   update_pvd_block_size_p = NULL;
    fbe_payload_ex_t *                                          payload_p = NULL;
    fbe_payload_control_operation_opcode_t                      control_code;
    fbe_payload_control_operation_t *                           control_operation = NULL;
    fbe_payload_control_buffer_length_t                         length;
    fbe_status_t                                                status = FBE_STATUS_OK;

    job_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, "%s entry\n", __FUNCTION__);

    /* get the payload and control operation. */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload_p);
    control_code = fbe_transport_get_control_code(packet_p);

    /* get the drive swap request from the control operation. */
    fbe_payload_control_get_buffer(control_operation, &update_pvd_block_size_p);
    if(update_pvd_block_size_p == NULL)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s : ERROR: update pvd block size job request is NULL\n",__FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* verify buffer lenght for the drive swap request. */
    fbe_payload_control_get_buffer_length(control_operation, &length);
    if(length != sizeof(fbe_job_service_update_pvd_block_size_t))
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s : ERROR: update pvd block size job request is invalid\n",__FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    //update_provision_drive_config_type_p->job_number = job_queue_element_p->job_number;

    /*! set the queue element initial parameters */
    status = fbe_job_service_build_queue_element(job_queue_element_p,
                                                 control_code,
                                                 FBE_OBJECT_ID_INVALID,
                                                 (fbe_u8_t *) update_pvd_block_size_p,
                                                 sizeof(fbe_job_service_update_pvd_block_size_t));

    if (status != FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_WARNING, 
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s: Job service operation pointer is null, status 0x%x \n",
                __FUNCTION__, status);
    }

    return status;
}

/*!**************************************************************
 * fbe_job_service_set_job_number_on_update_provision_drive_block_size_request()
 ****************************************************************
 * @brief An engineering mode function to change PVD's block size.
 *
 * @param packet_p              - packet for drive swap request.             
 * @param job_queue_element_p   - job service request.
 *
 * @return fbe_status_t         - Status of the operation.
 *
 * @author
 * 06/11/2014 - Created. Wayne Garrett
 *
 ****************************************************************/
void fbe_job_service_set_job_number_on_update_provision_drive_block_size_request(fbe_packet_t * packet_p,
                                                                                 fbe_job_queue_element_t *job_queue_element_p)
{
    fbe_job_service_update_pvd_block_size_t *               update_pvd_block_size_p = NULL;
    fbe_payload_ex_t *                                      payload_p = NULL;
    fbe_payload_control_operation_t *                       control_operation = NULL;

    job_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, "%s entry\n", __FUNCTION__);

    /* get the payload and control operation. */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload_p);

    /* get the drive swap request from the control operation. */
    fbe_payload_control_get_buffer(control_operation, &update_pvd_block_size_p);
    update_pvd_block_size_p->job_number = job_queue_element_p->job_number;
    return;
}


/*!**************************************************************
 * fbe_job_service_fill_spare_config_update_request()
 ****************************************************************
 * @brief
 *  It process a job control request to update the spare config
 *  information.
 *  It validates a packet and then create an job request element
 *  to queue it to the job service.
 *
 * @param packet_p              - packet for drive swap request.
 * @param job_queue_element_p   - job service request.
 *
 * @return fbe_status_t         - Status of the operation.
 *
 * @author
 * 09/15/2010 - Created. Dhaval Patel
 *
 ****************************************************************/
fbe_status_t fbe_job_service_fill_spare_config_update_request(fbe_packet_t * packet_p,
                                                              fbe_job_queue_element_t *job_queue_element_p)
{
    fbe_job_service_update_spare_config_t *         update_spare_config_p = NULL;
    fbe_payload_ex_t *                                 payload_p = NULL;
    fbe_payload_control_operation_opcode_t          control_code;
    fbe_payload_control_operation_t *               control_operation = NULL;
    fbe_payload_control_buffer_length_t             length;
    fbe_status_t                                    status = FBE_STATUS_OK;

    job_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, "%s entry\n", __FUNCTION__);

    /* get the payload and control operation. */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload_p);
    control_code = fbe_transport_get_control_code(packet_p);

    /* get the drive swap request from the control operation. */
    fbe_payload_control_get_buffer(control_operation, &update_spare_config_p);
    if(update_spare_config_p == NULL)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s : ERROR: Spare config update job request is NULL\n",__FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* verify buffer lenght for the drive swap request. */
    fbe_payload_control_get_buffer_length(control_operation, &length);
    if(length != sizeof(fbe_job_service_update_spare_config_t))
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s : ERROR: Size of Spare config update job request is invalid\n",__FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    //update_spare_config_p->job_number = 0;
    
    /*! set the queue element initial parameters */
    status = fbe_job_service_build_queue_element(job_queue_element_p,
                                                 control_code,
                                                 FBE_OBJECT_ID_INVALID,
                                                 (fbe_u8_t *) update_spare_config_p,
                                                 sizeof(fbe_job_service_update_spare_config_t));
    if (status != FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_WARNING, 
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s: Job service operation pointer is null, status 0x%x \n",
                __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}
/********************************************************
 * end fbe_job_service_fill_spare_config_update_request()
 *******************************************************/

/*!**************************************************************
 * fbe_job_service_set_job_number_on_spare_config_update_request()
 ****************************************************************
 * @brief
 *
 * @param packet_p              - packet for drive swap request.             
 * @param job_queue_element_p   - job service request.
 *
 * @return fbe_status_t         - Status of the operation.
 *
 * @author
 * 05/27/2010 - Created. Dhaval Patel
 *
 ****************************************************************/
void fbe_job_service_set_job_number_on_spare_config_update_request(fbe_packet_t * packet_p,
                                                                    fbe_job_queue_element_t *job_queue_element_p)
{
    fbe_job_service_update_spare_config_t *                 update_spare_config_p = NULL;
    fbe_payload_ex_t *                                         payload_p = NULL;
    fbe_payload_control_operation_t *                       control_operation = NULL;

    job_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry\n", __FUNCTION__);

    /* get the payload and control operation. */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload_p);
    fbe_payload_control_get_buffer(control_operation, &update_spare_config_p);
    update_spare_config_p->job_number = job_queue_element_p->job_number;
    return;
}
/**********************************************************************
 * end fbe_job_service_set_job_number_on_spare_config_update_request()
 **********************************************************************/

/*!**************************************************************
 * fbe_job_service_fill_system_power_save_change_request()
 ****************************************************************
 * @brief
 * build a request to change the system power saving setting
 *
 * @param packet_p - packet containing power save changes.             
 *
 * @return fbe_status_t - Status of the processing of the request
 *                        
 * @author
 * 04/28/2010 - Created. Shay Harel
 *
 ****************************************************************/
fbe_status_t fbe_job_service_fill_system_power_save_change_request(fbe_packet_t * packet_p, 
                                                                   fbe_job_queue_element_t *job_queue_element_p)
{
    fbe_job_service_change_system_power_saving_info_t   *system_ps_info_change_req_p = NULL;
    fbe_payload_ex_t                                       *payload_p = NULL;
    fbe_payload_control_operation_opcode_t 		control_code;
    fbe_payload_control_operation_t                     *control_operation = NULL;
    fbe_payload_control_buffer_length_t    		length;
    fbe_status_t                           		status = FBE_STATUS_OK;
  
    job_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry\n", __FUNCTION__);

    /*! Get the request payload.
    */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload_p);
    control_code = fbe_transport_get_control_code(packet_p);

    fbe_payload_control_get_buffer(control_operation, &system_ps_info_change_req_p);
    if(system_ps_info_change_req_p == NULL)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s : ERROR: System power save change request job is NULL\n",__FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &length);
    if(length != sizeof(fbe_job_service_change_system_power_saving_info_t))
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s : ERROR: Size of System power save change job request is invalid\n",__FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    //system_ps_info_change_req_p->job_number = job_queue_element_p->job_number;

    /*! set the queue element initial parameters */
    status = fbe_job_service_build_queue_element(job_queue_element_p,
                                                 control_code,
                                                 FBE_OBJECT_ID_INVALID,
                                                 (fbe_u8_t *)system_ps_info_change_req_p,
                                                 sizeof(fbe_job_service_change_system_power_saving_info_t));

    if(status != FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_WARNING, 
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s: Job service operation pointer is null, status 0x%x \n",
                __FUNCTION__, status);
    }

    return status;
}
/**********************************************************
 * end fbe_job_service_fill_system_power_save_change_request()
 ************************************************************/


/*!**************************************************************
 * fbe_job_service_set_job_number_on_system_power_save_change_request()
 ****************************************************************
 * @brief
 * 
 * @param packet_p - packet containing power save changes.             
 *
 * @return fbe_status_t - Status of the processing of the request
 *                        
 * @author
 * 04/28/2010 - Created. Shay Harel
 *
 ****************************************************************/
void fbe_job_service_set_job_number_on_system_power_save_change_request(fbe_packet_t * packet_p, 
                                                                       fbe_job_queue_element_t *job_queue_element_p)
{
    fbe_job_service_change_system_power_saving_info_t   *system_ps_info_change_req_p = NULL;
    fbe_payload_ex_t                                       *payload_p = NULL;
    fbe_payload_control_operation_t                     *control_operation = NULL;
  
    job_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry\n", __FUNCTION__);

    /*! Get the request payload.
    */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload_p);
 
    fbe_payload_control_get_buffer(control_operation, &system_ps_info_change_req_p);
    system_ps_info_change_req_p->job_number = job_queue_element_p->job_number;
    return;
}
/**************************************************************************
 * end fbe_job_service_set_job_number_on_system_power_save_change_request()
 *************************************************************************/


/*!**************************************************************
 * fbe_job_service_fill_raid_group_update_request()
 ****************************************************************
 * @brief
 * build a request to update the raid group after the user changed something in the config
 * (the initial support was for power saving change, but in the future this may grow)
 *
 * @param packet_p - packet containing raid group changes             
 *
 * @return fbe_status_t - Status of the processing of the request
 *                        
 * @author
 * 05/05/2010 - Created. Shay Harel
 *
 ****************************************************************/
fbe_status_t fbe_job_service_fill_raid_group_update_request(fbe_packet_t * packet_p, 
                                                            fbe_job_queue_element_t *job_queue_element_p)
{
    fbe_job_service_update_raid_group_t         *rg_update_req_p = NULL;
    fbe_payload_ex_t                               *payload_p = NULL;
    fbe_payload_control_operation_opcode_t 	control_code;
    fbe_payload_control_operation_t             *control_operation = NULL;
    fbe_payload_control_buffer_length_t    	length;
    fbe_status_t                           	status = FBE_STATUS_OK;
 
    job_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry\n", __FUNCTION__);

    /*! Get the request payload.
    */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload_p);
    control_code = fbe_transport_get_control_code(packet_p);

    fbe_payload_control_get_buffer(control_operation, &rg_update_req_p);
    if(rg_update_req_p == NULL)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s : ERROR: Raid group update job request is NULL\n",__FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &length);
    if(length != sizeof(fbe_job_service_update_raid_group_t))
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s : ERROR: Size of Raid group update job request is invalid\n",__FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    //rg_update_req_p->job_number = job_queue_element_p->job_number;

    /*! set the queue element initial parameters */
    status = fbe_job_service_build_queue_element(job_queue_element_p,
                                                 control_code,
                                                 FBE_OBJECT_ID_INVALID,
                                                 (fbe_u8_t *)rg_update_req_p,
                                                 sizeof(fbe_job_service_update_raid_group_t));

    if(status != FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_WARNING, 
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s: Job service operation pointer is null, status 0x%x \n",
                __FUNCTION__, status);
    }

    return status;

}
/**********************************************************
 * end fbe_job_service_fill_raid_group_update_request()
 ************************************************************/


/*!**************************************************************
 * fbe_job_service_set_job_number_on_raid_group_update_request()
 ****************************************************************
 * @brief
 *
 *
 * @param packet_p - packet containing raid group changes             
 *
 * @return fbe_status_t - Status of the processing of the request
 *                        
 * @author
 * 05/05/2010 - Created. Shay Harel
 *
 ****************************************************************/
void fbe_job_service_set_job_number_on_raid_group_update_request(fbe_packet_t * packet_p, 
                                                                 fbe_job_queue_element_t *job_queue_element_p)
{
    fbe_job_service_update_raid_group_t         *rg_update_req_p = NULL;
    fbe_payload_ex_t                               *payload_p = NULL;
    fbe_payload_control_operation_t             *control_operation = NULL;
 
    job_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry\n", __FUNCTION__);

    /*! Get the request payload.
    */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload_p);
    fbe_payload_control_get_buffer(control_operation, &rg_update_req_p);

    rg_update_req_p->job_number = job_queue_element_p->job_number;
    return;
}
/*******************************************************************
 * end fbe_job_service_set_job_number_on_raid_group_update_request()
 ******************************************************************/

/*!**************************************************************
 * fbe_job_service_set_job_number_on_provision_drive_create_request()
 ****************************************************************
 * @brief
 * *
 * @param packet_p - packet containing lun create request.             
 *
 * @return fbe_status_t - Status of the processing of the request
 *
 * @author
 * 09/23/2010 - Created. Jesus Medina
 *
 ****************************************************************/

void fbe_job_service_set_job_number_on_provision_drive_create_request(fbe_packet_t * packet_p, 
                                                          fbe_job_queue_element_t * job_queue_element_p)
{
    fbe_job_service_create_provision_drive_t           *pvd_create_req_p = NULL;
    fbe_payload_ex_t                          *payload_p = NULL;
    fbe_payload_control_operation_t        *control_operation = NULL;

    job_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry\n", __FUNCTION__);

    /*! Get the request payload.
    */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload_p);

    fbe_payload_control_get_buffer(control_operation, &pvd_create_req_p);
    pvd_create_req_p->job_number = job_queue_element_p->job_number;
    return;
}
/************************************************************
 * end fbe_job_service_set_job_number_on_provision_drive_create_request()
 ***********************************************************/


/*!**************************************************************
 * fbe_job_service_set_job_number_on_db_update_on_peer_request()
 ****************************************************************
 * @brief
 *
 * @param packet_p              - packet for db update.             
 * @param job_queue_element_p   - job service request.
 *
 * @return fbe_status_t         - Status of the operation.
 *
 *
 ****************************************************************/
void fbe_job_service_set_job_number_on_db_update_on_peer_request(fbe_packet_t * packet_p,
                                                                    fbe_job_queue_element_t *job_queue_element_p)
{
    fbe_job_service_update_db_on_peer_t *                 	update_db_on_peer_p = NULL;
    fbe_payload_ex_t *                                         payload_p = NULL;
    fbe_payload_control_operation_t *                       control_operation = NULL;

    job_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry\n", __FUNCTION__);

    /* get the payload and control operation. */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload_p);
    fbe_payload_control_get_buffer(control_operation, &update_db_on_peer_p);
    update_db_on_peer_p->job_number = job_queue_element_p->job_number;
    return;
}

/*!**************************************************************
 * fbe_job_service_fill_update_db_on_peer_create_request()
 ****************************************************************
 * @brief
 * This function builds the job_queue_element for the
 * update_db_on_peer request.
 *
 * @param packet_p              - packet for db update.             
 * @param job_queue_element_p   - job service request.
 *
 * @return fbe_status_t         - Status of the operation.
 *
 ****************************************************************/
fbe_status_t fbe_job_service_fill_update_db_on_peer_create_request(fbe_packet_t * packet_p, fbe_job_queue_element_t *job_queue_element_p)
{
    fbe_job_service_update_db_on_peer_t     *	  update_db_p = NULL;
    fbe_payload_ex_t                                *payload_p = NULL;
    fbe_payload_control_operation_opcode_t        control_code;
    fbe_payload_control_operation_t              *control_operation = NULL;
    fbe_payload_control_buffer_length_t           length;
    fbe_status_t                                  status = FBE_STATUS_OK;

    job_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry\n", __FUNCTION__);

    /*! Get the request payload.
    */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload_p);
    control_code = fbe_transport_get_control_code(packet_p);

    fbe_payload_control_get_buffer(control_operation, &update_db_p);
    if(update_db_p == NULL)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s : ERROR: Update DB on peer request is NULL\n",__FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &length);
    if(length != sizeof(fbe_job_service_update_db_on_peer_t))
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s : ERROR: Size of update db on peer job request is invalid\n",__FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /*! set the queue element initial parameters */
    status = fbe_job_service_build_queue_element(job_queue_element_p,
                                                 control_code,
                                                 FBE_OBJECT_ID_INVALID,
                                                 (fbe_u8_t *)update_db_p,
                                                 sizeof(fbe_job_service_update_db_on_peer_t));

    if (status != FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_WARNING, 
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s: Job service operation pointer is null, status 0x%x \n",
                __FUNCTION__, status);
    }

    return status;
}

/*!**************************************************************
 * fbe_job_service_set_job_number_on_provision_drive_destroy_request()
 ****************************************************************
 * @brief
 * *
 * @param packet_p - packet containing pvd destroy request.             
 *
 * @return fbe_status_t - Status of the processing of the request
 *
 * @author
 * 1/18/2012 - Created. zhangy
 *
 ****************************************************************/

void fbe_job_service_set_job_number_on_provision_drive_destroy_request(fbe_packet_t * packet_p, 
                                                          fbe_job_queue_element_t * job_queue_element_p)
{
    fbe_job_service_destroy_provision_drive_t     *pvd_destroy_req_p = NULL;
    fbe_payload_ex_t                          *payload_p = NULL;
    fbe_payload_control_operation_t        *control_operation = NULL;

    job_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry\n", __FUNCTION__);

    /*! Get the request payload.
    */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload_p);

    fbe_payload_control_get_buffer(control_operation, &pvd_destroy_req_p);
    pvd_destroy_req_p->job_number = job_queue_element_p->job_number;
    return;
}
/************************************************************
 * end fbe_job_service_set_job_number_on_provision_drive_destroy_request()
 ***********************************************************/

/*!**************************************************************
 * fbe_job_service_fill_system_bg_service_control_request()
 ****************************************************************
 * @brief
 * build a request to control the system background service 
 *
 * @param packet_p - packet containing system background service control.             
 *
 * @return fbe_status_t - Status of the processing of the request
 *                        
 * @author
 * 02/29/2012 - Created. Vera Wang
 *
 ****************************************************************/
fbe_status_t fbe_job_service_fill_system_bg_service_control_request(fbe_packet_t * packet_p, 
                                                                   fbe_job_queue_element_t *job_queue_element_p)
{
    fbe_job_service_control_system_bg_service_t *system_bg_service_control_req_p = NULL;
    fbe_payload_ex_t                            *payload_p = NULL;
    fbe_payload_control_operation_opcode_t 		control_code;
    fbe_payload_control_operation_t             *control_operation = NULL;
    fbe_payload_control_buffer_length_t    		length;
    fbe_status_t                           		status = FBE_STATUS_OK;
  
    job_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry\n", __FUNCTION__);

    /*! Get the request payload.
    */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload_p);
    control_code = fbe_transport_get_control_code(packet_p);

    fbe_payload_control_get_buffer(control_operation, &system_bg_service_control_req_p);
    if(system_bg_service_control_req_p == NULL)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s : ERROR: System background service control request job is NULL\n",__FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &length);
    if(length != sizeof(fbe_job_service_control_system_bg_service_t))
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s : ERROR: Size of system background service control request job is invalid\n",__FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*! set the queue element initial parameters */
    status = fbe_job_service_build_queue_element(job_queue_element_p,
                                                 control_code,
                                                 FBE_OBJECT_ID_INVALID,
                                                 (fbe_u8_t *)system_bg_service_control_req_p,
                                                 sizeof(fbe_job_service_control_system_bg_service_t));

    if(status != FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_WARNING, 
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s: Job service operation pointer is null, status 0x%x \n",
                __FUNCTION__, status);
    }

    return status;
}
/**********************************************************
 * end fbe_job_service_fill_system_bg_service_control_request()
 ************************************************************/


/*!**************************************************************
 * fbe_job_service_set_job_number_on_system_bg_service_control_request()
 ****************************************************************
 * @brief
 * 
 * @param packet_p - packet containing system background service control.             
 *
 * @return fbe_status_t - Status of the processing of the request
 *                        
 * @author
 * 02/29/2012 - Created. Vera Wang
 *
 ****************************************************************/
void fbe_job_service_set_job_number_on_system_bg_service_control_request(fbe_packet_t * packet_p, 
                                                                       fbe_job_queue_element_t *job_queue_element_p)
{
    fbe_job_service_control_system_bg_service_t *system_bg_service_control_req_p = NULL;
    fbe_payload_ex_t                            *payload_p = NULL;
    fbe_payload_control_operation_t             *control_operation = NULL;
  
    job_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry\n", __FUNCTION__);

    /*! Get the request payload.
    */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload_p);
 
    fbe_payload_control_get_buffer(control_operation, &system_bg_service_control_req_p);
    system_bg_service_control_req_p->job_number = job_queue_element_p->job_number;
    return;
}
/**************************************************************************
 * end fbe_job_service_set_job_number_on_system_bg_service_control_request()
 *************************************************************************/

 /*!**************************************************************
  * fbe_job_service_fill_ndu_commit_request()
  ****************************************************************
  * @brief
  * This function builds the job_queue_element for the
  * ndu_commit request.
  *
  * @param packet_p              - packet for ndu commit.             
  * @param job_queue_element_p   - job service request.
  *
  * @return fbe_status_t         - Status of the operation.
  *
  ****************************************************************/
 fbe_status_t fbe_job_service_fill_ndu_commit_request(fbe_packet_t * packet_p, fbe_job_queue_element_t * job_queue_element_p)
 {
     fbe_job_service_sep_ndu_commit_t     *     ndu_commit_p = NULL;
     fbe_payload_ex_t                                *payload_p = NULL;
     fbe_payload_control_operation_opcode_t        control_code;
     fbe_payload_control_operation_t              *control_operation = NULL;
     fbe_payload_control_buffer_length_t           length;
     fbe_status_t                                  status = FBE_STATUS_OK;
 
     job_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
             FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
             "%s entry\n", __FUNCTION__);
 
     /*! Get the request payload.
     */
     payload_p = fbe_transport_get_payload_ex(packet_p);
     control_operation = fbe_payload_ex_get_control_operation(payload_p);
     control_code = fbe_transport_get_control_code(packet_p);
 
     fbe_payload_control_get_buffer(control_operation, &ndu_commit_p);
     if(ndu_commit_p == NULL)
     {
         job_service_trace(FBE_TRACE_LEVEL_ERROR,
             FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
             "%s :NDU COMMIT request is NULL\n",__FUNCTION__);
         return FBE_STATUS_GENERIC_FAILURE;
     }
 
     fbe_payload_control_get_buffer_length(control_operation, &length);
     if(length != sizeof(fbe_job_service_sep_ndu_commit_t))
     {
         job_service_trace(FBE_TRACE_LEVEL_ERROR,
             FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
             "%s :Size of ndu commit is invalid\n",__FUNCTION__);
         return FBE_STATUS_GENERIC_FAILURE;
     }
     
     /*! set the queue element initial parameters */
     status = fbe_job_service_build_queue_element(job_queue_element_p,
                                                  control_code,
                                                  FBE_OBJECT_ID_INVALID,
                                                  (fbe_u8_t *)ndu_commit_p,
                                                  sizeof(fbe_job_service_sep_ndu_commit_t));
 
     if (status != FBE_STATUS_OK)
     {
         job_service_trace(FBE_TRACE_LEVEL_WARNING, 
                 FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                 "%s: Job service operation pointer is null, status 0x%x \n",
                 __FUNCTION__, status);
     }
 
     return status;
 }

 /*!**************************************************************
  * fbe_job_service_set_job_number_on_ndu_commit_request()
  ****************************************************************
  * @brief
  *
  * @param packet_p              - packet for ndu commit.             
  * @param job_queue_element_p   - job service request.
  *
  * @return fbe_status_t         - Status of the operation.
  *
  *
  ****************************************************************/
 void fbe_job_service_set_job_number_on_ndu_commit_request(fbe_packet_t * packet_p,
                                                                     fbe_job_queue_element_t *job_queue_element_p)
 {
     fbe_job_service_sep_ndu_commit_t *                   ndu_commit_p = NULL;
     fbe_payload_ex_t *                                         payload_p = NULL;
     fbe_payload_control_operation_t *                       control_operation = NULL;
 
     job_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
             FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
             "%s entry\n", __FUNCTION__);
 
     /* get the payload and control operation. */
     payload_p = fbe_transport_get_payload_ex(packet_p);
     control_operation = fbe_payload_ex_get_control_operation(payload_p);
     fbe_payload_control_get_buffer(control_operation, &ndu_commit_p);
     ndu_commit_p->job_number = job_queue_element_p->job_number;
     return;
 }
 /*!**************************************************************
 * fbe_job_service_fill_system_time_threshold_set_request()
 ****************************************************************
 * @brief
 * build a request to set the system tiem_threshold
 *
 * @param packet_p - packet containing time threshold value.			
 *
 * @return fbe_status_t - Status of the processing of the request
 *						   
 * @author
 * 05/31/2012 - Created. zhangy
 *
 ****************************************************************/
 fbe_status_t fbe_job_service_fill_system_time_threshold_set_request(fbe_packet_t * packet_p, 
                                                                        fbe_job_queue_element_t *job_queue_element_p)
 {
     fbe_job_service_set_system_time_threshold_info_t	 *system_time_threshold_set_req_p = NULL;
     fbe_payload_ex_t										*payload_p = NULL;
     fbe_payload_control_operation_opcode_t 	 control_code;
     fbe_payload_control_operation_t			 *control_operation = NULL;
     fbe_payload_control_buffer_length_t		 length;
     fbe_status_t								 status = FBE_STATUS_OK;
       
     job_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                 FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                 "%s entry\n", __FUNCTION__);
     
     /*! Get the request payload.
     */
     payload_p = fbe_transport_get_payload_ex(packet_p);
     control_operation = fbe_payload_ex_get_control_operation(payload_p);
     control_code = fbe_transport_get_control_code(packet_p);
     
     fbe_payload_control_get_buffer(control_operation, &system_time_threshold_set_req_p);
     if(system_time_threshold_set_req_p == NULL)
     {
         job_service_trace(FBE_TRACE_LEVEL_ERROR,
                 FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                 "%s : ERROR: System time threshold set request job is NULL\n",__FUNCTION__);
             return FBE_STATUS_GENERIC_FAILURE;
     }
     
     fbe_payload_control_get_buffer_length(control_operation, &length);
     if(length != sizeof(fbe_job_service_set_system_time_threshold_info_t))
     {
         job_service_trace(FBE_TRACE_LEVEL_ERROR,
                 FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                 "%s : ERROR: Size of System time threshold set request job request is invalid\n",__FUNCTION__); 
             return FBE_STATUS_GENERIC_FAILURE;
     }
         
     /*! set the queue element initial parameters */
     status = fbe_job_service_build_queue_element(job_queue_element_p,
                                                 control_code,
                                                 FBE_OBJECT_ID_INVALID,
                                                 (fbe_u8_t *)system_time_threshold_set_req_p,
                                                 sizeof(fbe_job_service_set_system_time_threshold_info_t));
     
     if(status != FBE_STATUS_OK)
     {
         job_service_trace(FBE_TRACE_LEVEL_WARNING, 
                     FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                     "%s: Job service operation pointer is null, status 0x%x \n",
                     __FUNCTION__, status);
     }
     
     return status;
 }
 /**********************************************************
 * end fbe_job_service_fill_system_power_save_change_request()
 ************************************************************/
     
 /*!**************************************************************
 * fbe_job_service_set_job_number_on_system_time_threshold_set_request()
 ****************************************************************
 * @brief
 * 
 * @param packet_p - packet containing system time threshold value. 			
 *
 * @return fbe_status_t - Status of the processing of the request
 *						   
 * @author
 * 05/31/2012 - Created. zhangy
 *
 ****************************************************************/
void fbe_job_service_set_job_number_on_system_time_threshold_set_request(fbe_packet_t * packet_p, 
                                                                            fbe_job_queue_element_t *job_queue_element_p)
{
     fbe_job_service_set_system_time_threshold_info_t	 *system_time_threshold_set_req_p = NULL;
     fbe_payload_ex_t										*payload_p = NULL;
     fbe_payload_control_operation_t					 *control_operation = NULL;
       
     job_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                 FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                 "%s entry\n", __FUNCTION__);
     
     /*! Get the request payload.
     */
     payload_p = fbe_transport_get_payload_ex(packet_p);
     control_operation = fbe_payload_ex_get_control_operation(payload_p);
      
     fbe_payload_control_get_buffer(control_operation, &system_time_threshold_set_req_p);
     system_time_threshold_set_req_p->job_number = job_queue_element_p->job_number;
     return;
}

/*!**************************************************************
 * fbe_job_service_fill_connect_drive_request()
 ****************************************************************
 * @brief
 * process a job control request to connect drives.  
 * The function validate the packet, allocate a job_queue_element, 
 * copy the payload to the job_queue_element and complete the packet. 
 *
 * @param packet_p - packet containing  drive connect request.             
 *
 * @return fbe_status_t - Status of the processing of the request
 *
 * @author
 * 8/31/2012 - Created. Zhipeng Hu
 *
 ****************************************************************/

fbe_status_t fbe_job_service_fill_connect_drive_request(fbe_packet_t * packet_p, fbe_job_queue_element_t * job_queue_element_p)
{
    fbe_job_service_connect_drive_t*    drive_connect_req_p = NULL;
    fbe_payload_ex_t                          *payload_p = NULL;
    fbe_payload_control_operation_opcode_t control_code;
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

    fbe_payload_control_get_buffer(control_operation, &drive_connect_req_p);
    if(drive_connect_req_p == NULL)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s : ERROR: drive connect job request is NULL\n",__FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &length);
    if(length != sizeof(fbe_job_service_connect_drive_t))
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s : ERROR: Size of drive connect job request is invalid\n",__FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*! set the queue element initial parameters */
    status = fbe_job_service_build_queue_element(job_queue_element_p,
                                                 control_code,
                                                 FBE_OBJECT_ID_INVALID,
                                                 (fbe_u8_t *)drive_connect_req_p,
                                                 sizeof(*drive_connect_req_p));
    if (status != FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_WARNING, 
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s: Job service operation pointer is null, status 0x%x \n",
                __FUNCTION__, status);
    }

    return status;
}
/*****************************************************
 * end fbe_job_service_fill_connect_drive_request()
 *****************************************************/
 
/*!**************************************************************
  * fbe_job_service_set_job_number_on_drive_connect_request()
  ****************************************************************
  * @brief set job number on drive connect request
  * *
  * @param packet_p - packet containing lun create request.             
  *
  * @return fbe_status_t - Status of the processing of the request
  *
  * @author
  * 8/31/2012 - Created. Zhipeng Hu
  *
  ****************************************************************/ 
 void fbe_job_service_set_job_number_on_drive_connect_request(fbe_packet_t * packet_p, 
                                                           fbe_job_queue_element_t * job_queue_element_p)
 {
     fbe_job_service_connect_drive_t           *drive_connect_req_p = NULL;
     fbe_payload_ex_t                          *payload_p = NULL;
     fbe_payload_control_operation_t        *control_operation = NULL;
 
     job_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
             FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
             "%s entry\n", __FUNCTION__);
 
     /*! Get the request payload.
     */
     payload_p = fbe_transport_get_payload_ex(packet_p);
     control_operation = fbe_payload_ex_get_control_operation(payload_p);
 
     fbe_payload_control_get_buffer(control_operation, &drive_connect_req_p);
     drive_connect_req_p->job_number = job_queue_element_p->job_number;
     return;
 }
 /************************************************************
  * end fbe_job_service_set_job_number_on_drive_connect_request()
  ***********************************************************/

/*!**************************************************************
 * fbe_job_service_fill_update_multi_pvds_pool_id_request()
 ****************************************************************
 * @brief
 * This function builds the job_queue_element for the
 * update_multi_pvds_pool_id request.
 *
 * @param packet_p              - packet for db update.             
 * @param job_queue_element_p   - job service request.
 *
 * @return fbe_status_t         - Status of the operation.
 *
 ****************************************************************/
fbe_status_t fbe_job_service_fill_update_multi_pvds_pool_id_request(fbe_packet_t * packet_p, fbe_job_queue_element_t *job_queue_element_p)
{
    fbe_job_service_update_multi_pvds_pool_id_t     *update_multi_pvds_pool_id = NULL;
    fbe_payload_ex_t                                *payload_p = NULL;
    fbe_payload_control_operation_opcode_t        control_code;
    fbe_payload_control_operation_t              *control_operation = NULL;
    fbe_payload_control_buffer_length_t           length;
    fbe_status_t                                  status = FBE_STATUS_OK;

    job_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry\n", __FUNCTION__);

    /*! Get the request payload.
    */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload_p);
    control_code = fbe_transport_get_control_code(packet_p);

    fbe_payload_control_get_buffer(control_operation, &update_multi_pvds_pool_id);
    if(update_multi_pvds_pool_id == NULL)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s : ERROR: Update multi pvds pool id request is NULL\n",__FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &length);
    if(length != sizeof(fbe_job_service_update_multi_pvds_pool_id_t))
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s : ERROR: Size of update multi pvds pool id request is invalid\n",__FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /*! set the queue element initial parameters */
    status = fbe_job_service_build_queue_element(job_queue_element_p,
                                                 control_code,
                                                 FBE_OBJECT_ID_INVALID,
                                                 (fbe_u8_t *)update_multi_pvds_pool_id,
                                                 sizeof(fbe_job_service_update_multi_pvds_pool_id_t));

    if (status != FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_WARNING, 
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s: Job service operation pointer is null, status 0x%x \n",
                __FUNCTION__, status);
    }

    return status;
}
 
/*!**************************************************************
  * fbe_job_service_set_job_number_on_update_multi_pvds_pool_id_request()
  ****************************************************************
  * @brief set job number on update multi pvds pool id request
  * *
  * @param packet_p - packet containing update multi pvds pool id request.             
  *
  * @return fbe_status_t - Status of the processing of the request
  *
  * @author
  * 04/08/2013 - Created. Hongpo Gao
  *
  ****************************************************************/ 
 void fbe_job_service_set_job_number_on_update_multi_pvds_pool_id_request(fbe_packet_t * packet_p, 
                                                           fbe_job_queue_element_t * job_queue_element_p)
 {
     fbe_job_service_update_multi_pvds_pool_id_t     *update_multi_pvds_pool_id = NULL;
     fbe_payload_ex_t                          *payload_p = NULL;
     fbe_payload_control_operation_t        *control_operation = NULL;
 
     job_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
             FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
             "%s entry\n", __FUNCTION__);
 
     /*! Get the request payload.
     */
     payload_p = fbe_transport_get_payload_ex(packet_p);
     control_operation = fbe_payload_ex_get_control_operation(payload_p);
 
     fbe_payload_control_get_buffer(control_operation, &update_multi_pvds_pool_id);
     update_multi_pvds_pool_id->job_number = job_queue_element_p->job_number;
     return;
 }
 /************************************************************
  * end fbe_job_service_set_job_number_on_update_multi_pvds_pool_id_request()
  ***********************************************************/

/*!**************************************************************
 * fbe_job_service_fill_update_spare_library_config_request()
 ****************************************************************
 * @brief
 *  It process a job control request to perform the drive swap
 *  operation.
 *  It validates a packet and then create an job request element
 *  to queue it to the job service.
 *
 * @param packet_p              - packet for drive swap request.             
 * @param job_queue_element_p   - job service request.
 *
 * @return fbe_status_t         - Status of the operation.
 *
 * @author
 * 05/27/2010 - Created. Dhaval Patel
 *
 ****************************************************************/
fbe_status_t fbe_job_service_fill_update_spare_library_config_request(fbe_packet_t * packet_p,
                                                     fbe_job_queue_element_t *job_queue_element_p)
{
    fbe_status_t                                            status = FBE_STATUS_OK;
    fbe_job_service_update_spare_library_config_request_t  *update_spare_config_request_p = NULL;
    fbe_payload_ex_t                                       *payload_p = NULL;
    fbe_payload_control_operation_opcode_t                  control_code;
    fbe_payload_control_operation_t                        *control_operation = NULL;
    fbe_payload_control_buffer_length_t                     length;

    job_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry\n", __FUNCTION__);

    /* get the payload and control operation. */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload_p);
    control_code = fbe_transport_get_control_code(packet_p);

    /* get the drive swap request from the control operation. */
    fbe_payload_control_get_buffer(control_operation, &update_spare_config_request_p);
    if(update_spare_config_request_p == NULL)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s : ERROR: job request is NULL\n",__FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* verify buffer lenght for the drive swap request. */
    fbe_payload_control_get_buffer_length(control_operation, &length);
    if(length != sizeof(fbe_job_service_update_spare_library_config_request_t))
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s : ERROR: Size of update job request is invalid\n",__FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    //drive_swap_request_p->job_number = job_queue_element_p->job_number;

    /*! set the queue element initial parameters */
    status = fbe_job_service_build_queue_element(job_queue_element_p,
                                                 control_code,
                                                 FBE_OBJECT_ID_INVALID,
                                                 (fbe_u8_t *)update_spare_config_request_p,
                                                 sizeof(*update_spare_config_request_p));
    if (status != FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_WARNING, 
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s: Job service operation pointer is null, status 0x%x \n",
                __FUNCTION__, status);
    }

    return status;
}
/****************************************************************
 * end fbe_job_service_fill_update_spare_library_config_request()
 ****************************************************************/

 /*!**************************************************************
 * fbe_job_service_set_job_number_on_update_spare_library_config_request()
 ****************************************************************
 * @brief
 *
 * @param packet_p              - packet for drive swap request.             
 * @param job_queue_element_p   - job service request.
 *
 * @return fbe_status_t         - Status of the operation.
 *
 * @author
 * 05/27/2010 - Created. Dhaval Patel
 *
 ****************************************************************/
void fbe_job_service_set_job_number_on_update_spare_library_config_request(fbe_packet_t * packet_p,
                                                                    fbe_job_queue_element_t *job_queue_element_p)
{
    fbe_job_service_update_spare_library_config_request_t  *update_spare_config_p = NULL;
    fbe_payload_ex_t                                       *payload_p = NULL;
    fbe_payload_control_operation_t                        *control_operation = NULL;

    job_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry\n", __FUNCTION__);

    /* get the payload and control operation. */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload_p);
    fbe_payload_control_get_buffer(control_operation, &update_spare_config_p);
    update_spare_config_p->job_number = job_queue_element_p->job_number;
    return;
}
/**********************************************************************
 * end fbe_job_service_set_job_number_on_update_spare_library_config_request()
 **********************************************************************/

/*!**************************************************************
 * fbe_job_service_fill_pause_encryption_request()
 ****************************************************************
 * @brief
 * build a request to change the system encryption setting
 *
 * @param packet_p - packet containing encryption changes.             
 *
 * @return fbe_status_t - Status of the processing of the request
 *                        
 * @author
 *    04-Oct-2013 - Created. 
 *
 ****************************************************************/
fbe_status_t fbe_job_service_fill_pause_encryption_request(fbe_packet_t * packet_p, 
                                                           fbe_job_queue_element_t *job_queue_element_p)
{
    fbe_job_service_pause_encryption_t                *pause_req_p = NULL;
    fbe_payload_ex_t                                  *payload_p = NULL;
    fbe_payload_control_operation_opcode_t 	       control_code;
    fbe_payload_control_operation_t                   *control_operation = NULL;
    fbe_payload_control_buffer_length_t    		      length;
    fbe_status_t                           		      status = FBE_STATUS_OK;
  
    job_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry\n", __FUNCTION__);

    // Get the request payload.
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload_p);
    control_code = fbe_transport_get_control_code(packet_p);

    fbe_payload_control_get_buffer(control_operation, &pause_req_p);
    if(pause_req_p == NULL)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s : ERROR: Encryption pause request job is NULL\n",__FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &length);
    if(length != sizeof(fbe_job_service_pause_encryption_t))
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s : ERROR: Size of pause encryption job request is invalid\n",__FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*! set the queue element initial parameters */
    status = fbe_job_service_build_queue_element(job_queue_element_p,
                                                 control_code,
                                                 FBE_OBJECT_ID_INVALID,
                                                 (fbe_u8_t *)pause_req_p,
                                                 sizeof(fbe_job_service_pause_encryption_t));

    if(status != FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_WARNING, 
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s: Job service operation pointer is null, status 0x%x \n",
                __FUNCTION__, status);
    }

    return status;
}
/**********************************************************
 * end fbe_job_service_fill_pause_encryption_request()
 ************************************************************/

/*!**************************************************************
 * fbe_job_service_set_job_number_on_pause_encryption_request()
 ****************************************************************
 * @brief
 * 
 * @param packet_p - packet containing encryption changes.             
 *
 * @return fbe_status_t - Status of the processing of the request
 *                        
 * @author
 *    04-Oct-2013 - Created
 *
 ****************************************************************/
void fbe_job_service_set_job_number_on_pause_encryption_request(fbe_packet_t * packet_p, 
                                                                       fbe_job_queue_element_t *job_queue_element_p)
{
    fbe_job_service_pause_encryption_t   *pause_req_p = NULL;
    fbe_payload_ex_t                                  *payload_p = NULL;
    fbe_payload_control_operation_t                   *control_operation = NULL;
  
    job_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry\n", __FUNCTION__);

    // Get the request payload
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload_p);
 
    fbe_payload_control_get_buffer(control_operation, &pause_req_p);

    pause_req_p->job_number = job_queue_element_p->job_number;

    return;
}
/**************************************************************************
 * end fbe_job_service_set_job_number_on_pause_encryption_request()
 *************************************************************************/

/*!**************************************************************
 * fbe_job_service_fill_system_encryption_change_request()
 ****************************************************************
 * @brief
 * build a request to change the system encryption setting
 *
 * @param packet_p - packet containing encryption changes.             
 *
 * @return fbe_status_t - Status of the processing of the request
 *                        
 * @author
 *    04-Oct-2013 - Created. 
 *
 ****************************************************************/
fbe_status_t fbe_job_service_fill_system_encryption_change_request(fbe_packet_t * packet_p, 
                                                                   fbe_job_queue_element_t *job_queue_element_p)
{
    fbe_job_service_change_system_encryption_info_t   *system_encryption_info_change_req_p = NULL;
    fbe_payload_ex_t                                  *payload_p = NULL;
    fbe_payload_control_operation_opcode_t 		      control_code;
    fbe_payload_control_operation_t                   *control_operation = NULL;
    fbe_payload_control_buffer_length_t    		      length;
    fbe_status_t                           		      status = FBE_STATUS_OK;
  
    job_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry\n", __FUNCTION__);

    /*! Get the request payload.
    */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload_p);
    control_code = fbe_transport_get_control_code(packet_p);

    fbe_payload_control_get_buffer(control_operation, &system_encryption_info_change_req_p);
    if(system_encryption_info_change_req_p == NULL)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s : ERROR: System encryption change request job is NULL\n",__FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &length);
    if(length != sizeof(fbe_job_service_change_system_encryption_info_t))
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s : ERROR: Size of System encryption change job request is invalid\n",__FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*! set the queue element initial parameters */
    status = fbe_job_service_build_queue_element(job_queue_element_p,
                                                 control_code,
                                                 FBE_OBJECT_ID_INVALID,
                                                 (fbe_u8_t *)system_encryption_info_change_req_p,
                                                 sizeof(fbe_job_service_change_system_encryption_info_t));

    if(status != FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_WARNING, 
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s: Job service operation pointer is null, status 0x%x \n",
                __FUNCTION__, status);
    }

    return status;
}
/**********************************************************
 * end fbe_job_service_fill_system_encryption_change_request()
 ************************************************************/

/*!**************************************************************
 * fbe_job_service_set_job_number_on_system_encryption_change_request()
 ****************************************************************
 * @brief
 * 
 * @param packet_p - packet containing encryption changes.             
 *
 * @return fbe_status_t - Status of the processing of the request
 *                        
 * @author
 *    04-Oct-2013 - Created
 *
 ****************************************************************/
void fbe_job_service_set_job_number_on_system_encryption_change_request(fbe_packet_t * packet_p, 
                                                                       fbe_job_queue_element_t *job_queue_element_p)
{
    fbe_job_service_change_system_encryption_info_t   *system_encryption_info_change_req_p = NULL;
    fbe_payload_ex_t                                  *payload_p = NULL;
    fbe_payload_control_operation_t                   *control_operation = NULL;
  
    job_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry\n", __FUNCTION__);

    /*! Get the request payload.
    */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload_p);
 
    fbe_payload_control_get_buffer(control_operation, &system_encryption_info_change_req_p);

    system_encryption_info_change_req_p->job_number = job_queue_element_p->job_number;

    return;
}
/**************************************************************************
 * end fbe_job_service_set_job_number_on_system_encryption_change_request()
 *************************************************************************/
/*!**************************************************************
 * fbe_job_service_fill_update_encryption_mode_request()
 ****************************************************************
 * @brief
 * build a request to update the rg encryption mode
 *
 * @param packet_p - packet containing encryption changes.             
 *
 * @return fbe_status_t - Status of the processing of the request
 *                        
 * @author
 *    01-Nov-2013 - Created. 
 *
 ****************************************************************/
fbe_status_t fbe_job_service_fill_update_encryption_mode_request(fbe_packet_t * packet_p, 
                                                                 fbe_job_queue_element_t *job_queue_element_p)
{
    fbe_job_service_update_encryption_mode_t   *update_enc_req_p = NULL;
    fbe_payload_ex_t                                  *payload_p = NULL;
    fbe_payload_control_operation_opcode_t 		      control_code;
    fbe_payload_control_operation_t                   *control_operation = NULL;
    fbe_payload_control_buffer_length_t    		      length;
    fbe_status_t                           		      status = FBE_STATUS_OK;
  
    job_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry\n", __FUNCTION__);

    /*! Get the request payload.
    */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload_p);
    control_code = fbe_transport_get_control_code(packet_p);

    fbe_payload_control_get_buffer(control_operation, &update_enc_req_p);
    if(update_enc_req_p == NULL)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s : ERROR: Update RG encryption request job is NULL\n",__FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &length);
    if(length != sizeof(fbe_job_service_update_encryption_mode_t))
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s : ERROR: Size of Update RG encryption job request is invalid\n",__FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*! set the queue element initial parameters */
    status = fbe_job_service_build_queue_element(job_queue_element_p,
                                                 control_code,
                                                 FBE_OBJECT_ID_INVALID,
                                                 (fbe_u8_t *)update_enc_req_p,
                                                 sizeof(fbe_job_service_update_encryption_mode_t));

    if(status != FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_WARNING, 
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s: Job service operation pointer is null, status 0x%x \n",
                __FUNCTION__, status);
    }

    return status;
}
/**********************************************************
 * end fbe_job_service_fill_update_encryption_mode_request()
 ************************************************************/

/*!**************************************************************
 * fbe_job_service_set_job_number_on_update_encryption_mode_request()
 ****************************************************************
 * @brief
 * 
 * @param packet_p - packet containing encryption changes.             
 *
 * @return fbe_status_t - Status of the processing of the request
 *                        
 * @author
 *    01-Nov-2013 - Created
 *
 ****************************************************************/
void fbe_job_service_set_job_number_on_update_encryption_mode_request(fbe_packet_t * packet_p, 
                                                                      fbe_job_queue_element_t *job_queue_element_p)
{
    fbe_job_service_update_encryption_mode_t   *update_rg_enc_mode_req_p = NULL;
    fbe_payload_ex_t                                  *payload_p = NULL;
    fbe_payload_control_operation_t                   *control_operation = NULL;
  
    job_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry\n", __FUNCTION__);

    /*! Get the request payload.
    */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload_p);
 
    fbe_payload_control_get_buffer(control_operation, &update_rg_enc_mode_req_p);

    update_rg_enc_mode_req_p->job_number = job_queue_element_p->job_number;

    return;
}
/**************************************************************************
 * end fbe_job_service_set_job_number_on_update_encryption_mode_request()
 *************************************************************************/
/*!**************************************************************
 * fbe_job_service_fill_process_encryption_keys_request()
 ****************************************************************
 * @brief
 * build a request to process encryption keys based on user request.
 *
 * @param packet_p - packet containing encryption changes.             
 *
 * @return fbe_status_t - Status of the processing of the request
 *                        
 * @author
 *    01-Nov-2013 - Created. 
 *
 ****************************************************************/
fbe_status_t fbe_job_service_fill_process_encryption_keys_request(fbe_packet_t * packet_p, 
                                                                 fbe_job_queue_element_t *job_queue_element_p)
{
    fbe_job_service_process_encryption_keys_t   *process_encryption_keys_p = NULL;
    fbe_payload_ex_t                            *payload_p = NULL;
    fbe_payload_control_operation_opcode_t 		 control_code;
    fbe_payload_control_operation_t             *control_operation = NULL;
    fbe_payload_control_buffer_length_t    		 length;
    fbe_status_t                           		 status = FBE_STATUS_OK;
    fbe_sg_element_t *                           sg_element = NULL;
    fbe_u32_t                                    sg_elements = 0;
  
    job_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry\n", __FUNCTION__);

    /*! Get the request payload.
    */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload_p);
    control_code = fbe_transport_get_control_code(packet_p);

    fbe_payload_control_get_buffer(control_operation, &process_encryption_keys_p);
    if(process_encryption_keys_p == NULL)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s : ERROR: Update RG encryption request job is NULL\n",__FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &length);
    if(length != sizeof(fbe_job_service_process_encryption_keys_t))
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s : ERROR: Size of Update RG encryption job request is invalid\n",__FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_ex_get_sg_list(payload_p, &sg_element, &sg_elements);

    if(sg_element == NULL)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s : ERROR: no valid key table\n",__FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    job_queue_element_p->additional_job_data  = (fbe_u8_t *)fbe_memory_ex_allocate(sg_element[0].count);

    if (job_queue_element_p->additional_job_data == NULL)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s : ERROR: no memory for key table\n",__FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*! set the queue element initial parameters */
    status = fbe_job_service_build_queue_element(job_queue_element_p,
                                                 control_code,
                                                 FBE_OBJECT_ID_INVALID,
                                                 (fbe_u8_t *)process_encryption_keys_p,
                                                 sizeof(fbe_job_service_process_encryption_keys_t));

    if(status != FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_WARNING, 
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s: Job service operation pointer is null, status 0x%x \n",
                __FUNCTION__, status);
        fbe_memory_ex_release(job_queue_element_p->additional_job_data);
        job_queue_element_p->additional_job_data = NULL;
    }

    job_queue_element_p->local_job_data_valid = FBE_TRUE;
    fbe_copy_memory(job_queue_element_p->additional_job_data, sg_element[0].address, sg_element[0].count);

    return status;
}
/**********************************************************
 * end fbe_job_service_fill_process_encryption_keys_request()
 ************************************************************/

/*!**************************************************************
 * fbe_job_service_set_job_number_on_process_encryption_keys_request()
 ****************************************************************
 * @brief
 * 
 * @param packet_p - packet containing encryption keys .             
 *
 * @return fbe_status_t - Status of the processing of the request
 *                        
 * @author
 *    01-Nov-2013 - Created
 *
 ****************************************************************/
void fbe_job_service_set_job_number_on_process_encryption_keys_request(fbe_packet_t * packet_p, 
                                                                      fbe_job_queue_element_t *job_queue_element_p)
{
    fbe_job_service_process_encryption_keys_t   *process_encryption_keys_p = NULL;
    fbe_payload_ex_t                            *payload_p = NULL;
    fbe_payload_control_operation_t             *control_operation = NULL;
  
    job_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry\n", __FUNCTION__);

    /*! Get the request payload.
    */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload_p);
 
    fbe_payload_control_get_buffer(control_operation, &process_encryption_keys_p);

    process_encryption_keys_p->job_number = job_queue_element_p->job_number;

    return;
}
/**************************************************************************
 * end fbe_job_service_set_job_number_on_process_encryption_keys_request()
 *************************************************************************/


/*!**************************************************************
 * fbe_job_service_set_job_number_on_scrub_old_user_data_request()
 ****************************************************************
 * @brief
 * 
 * @param packet_p - packet containing the request.             
 *
 * @return fbe_status_t - Status of the processing of the request
 *                        
 * @author
 *
 ****************************************************************/
void fbe_job_service_set_job_number_on_scrub_old_user_data_request(fbe_packet_t * packet_p, 
                                                                       fbe_job_queue_element_t *job_queue_element_p)
{
    fbe_job_service_scrub_old_user_data_t   *req_p = NULL;
    fbe_payload_ex_t                                  *payload_p = NULL;
    fbe_payload_control_operation_t                   *control_operation = NULL;
  
    job_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry\n", __FUNCTION__);

    /*! Get the request payload.
    */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload_p);
 
    fbe_payload_control_get_buffer(control_operation, &req_p);

    req_p->job_number = job_queue_element_p->job_number;

    return;
}
/**************************************************************************
 * end fbe_job_service_set_job_number_on_scrub_old_user_data_request()
 *************************************************************************/
/*!**************************************************************
 * fbe_job_service_fill_scrub_old_user_data_request()
 ****************************************************************
 * @brief
 * build a request to scrub previous user data.
 *
 * @param packet_p - packet containing the request.             
 *
 * @return fbe_status_t - Status of the processing of the request
 *                        
 * @author
 *    01-Nov-2013 - Created. 
 *
 ****************************************************************/
fbe_status_t fbe_job_service_fill_scrub_old_user_data_request(fbe_packet_t * packet_p, 
                                                                 fbe_job_queue_element_t *job_queue_element_p)
{
    fbe_job_service_scrub_old_user_data_t   *         scrub_req_p = NULL;
    fbe_payload_ex_t                                  *payload_p = NULL;
    fbe_payload_control_operation_opcode_t 		      control_code;
    fbe_payload_control_operation_t                   *control_operation = NULL;
    fbe_payload_control_buffer_length_t    		      length;
    fbe_status_t                           		      status = FBE_STATUS_OK;
  
    job_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry\n", __FUNCTION__);

    /*! Get the request payload.
    */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload_p);
    control_code = fbe_transport_get_control_code(packet_p);

    fbe_payload_control_get_buffer(control_operation, &scrub_req_p);
    if(scrub_req_p == NULL)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s : ERROR: Update RG encryption request job is NULL\n",__FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &length);
    if(length != sizeof(fbe_job_service_scrub_old_user_data_t))
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s : ERROR: Size of Update RG encryption job request is invalid\n",__FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*! set the queue element initial parameters */
    status = fbe_job_service_build_queue_element(job_queue_element_p,
                                                 control_code,
                                                 FBE_OBJECT_ID_INVALID,
                                                 (fbe_u8_t *)scrub_req_p,
                                                 sizeof(fbe_job_service_scrub_old_user_data_t));

    if(status != FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_WARNING, 
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s: Job service operation pointer is null, status 0x%x \n",
                __FUNCTION__, status);
    }

    return status;
}
/**********************************************************
 * end fbe_job_service_fill_scrub_old_user_data_request()
 ************************************************************/

/*!**************************************************************
 * fbe_job_service_fill_validate_database_request()
 ****************************************************************
 * @brief
 * build a request to validate the in-memory database with on-disk
 *
 * @param packet_p - packet containing encryption changes.             
 *
 * @return fbe_status_t - Status of the processing of the request
 *                        
 * @author
 *  07/01/2014  Ron Proulx  - Created. 
 *
 ****************************************************************/
fbe_status_t fbe_job_service_fill_validate_database_request(fbe_packet_t * packet_p, 
                                                           fbe_job_queue_element_t *job_queue_element_p)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_job_service_validate_database_t    *validate_request_p = NULL;
    fbe_payload_ex_t                       *payload_p = NULL;
    fbe_payload_control_operation_opcode_t 	control_code;
    fbe_payload_control_operation_t        *control_operation = NULL;
    fbe_payload_control_buffer_length_t     length;
  
    job_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry\n", __FUNCTION__);

    // Get the request payload.
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload_p);
    control_code = fbe_transport_get_control_code(packet_p);

    fbe_payload_control_get_buffer(control_operation, &validate_request_p);
    if(validate_request_p == NULL)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s : ERROR: vadilate database request is NULL\n",__FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &length);
    if(length != sizeof(fbe_job_service_validate_database_t))
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s : ERROR: Size of pause encryption job request is invalid\n",__FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*! set the queue element initial parameters */
    status = fbe_job_service_build_queue_element(job_queue_element_p,
                                                 control_code,
                                                 FBE_OBJECT_ID_INVALID,
                                                 (fbe_u8_t *)validate_request_p,
                                                 sizeof(fbe_job_service_validate_database_t));
    if(status != FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_WARNING, 
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s: Job service operation pointer is null, status 0x%x \n",
                __FUNCTION__, status);
    }

    return status;
}
/**********************************************************
 * end fbe_job_service_fill_validate_database_request()
 ************************************************************/

/*!**************************************************************
 * fbe_job_service_set_job_number_on_validate_database_request()
 ****************************************************************
 * @brief
 * 
 * @param packet_p - packet containing validate database request             
 *
 * @return fbe_status_t - Status of the processing of the request
 *                        
 * @author
 *  07/01/2014  Created.
 *
 ****************************************************************/
void fbe_job_service_set_job_number_on_validate_database_request(fbe_packet_t * packet_p, 
                                                                fbe_job_queue_element_t *job_queue_element_p)
{
    fbe_job_service_validate_database_t    *validate_request_p = NULL;
    fbe_payload_ex_t                       *payload_p = NULL;
    fbe_payload_control_operation_t        *control_operation = NULL;
  
    job_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry\n", __FUNCTION__);

    // Get the request payload
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload_p);
 
    fbe_payload_control_get_buffer(control_operation, &validate_request_p);

    validate_request_p->job_number = job_queue_element_p->job_number;

    return;
}
/**************************************************************************
 * end fbe_job_service_set_job_number_on_validate_database_request()
 *************************************************************************/

/*!**************************************************************
 * fbe_job_service_fill_create_extent_pool_request()
 ****************************************************************
 * @brief
 * build a request to create a extent pool
 *
 * @param packet_p - packet containing the request.             
 *
 * @return fbe_status_t - Status of the processing of the request
 *                        
 * @author
 *    06-June-2014 - Created. 
 *
 ****************************************************************/
fbe_status_t fbe_job_service_fill_create_extent_pool_request(fbe_packet_t * packet_p, 
                                                             fbe_job_queue_element_t *job_queue_element_p)
{
    fbe_job_service_create_extent_pool_t              *create_pool_req_p = NULL;
    fbe_payload_ex_t                                  *payload_p = NULL;
    fbe_payload_control_operation_opcode_t 		      control_code;
    fbe_payload_control_operation_t                   *control_operation = NULL;
    fbe_payload_control_buffer_length_t    		      length;
    fbe_status_t                           		      status = FBE_STATUS_OK;
  
    job_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry\n", __FUNCTION__);

    /*! Get the request payload.
    */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload_p);
    control_code = fbe_transport_get_control_code(packet_p);

    fbe_payload_control_get_buffer(control_operation, &create_pool_req_p);
    if(create_pool_req_p == NULL)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s : ERROR: create extent pool request job is NULL\n",__FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &length);
    if(length != sizeof(fbe_job_service_create_extent_pool_t))
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s : ERROR: Size of create extent pool job request is invalid\n",__FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*! set the queue element initial parameters */
    status = fbe_job_service_build_queue_element(job_queue_element_p,
                                                 control_code,
                                                 FBE_OBJECT_ID_INVALID,
                                                 (fbe_u8_t *)create_pool_req_p,
                                                 sizeof(fbe_job_service_create_extent_pool_t));

    if(status != FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_WARNING, 
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s: Job service operation pointer is null, status 0x%x \n",
                __FUNCTION__, status);
    }

    return status;
}
/**********************************************************
 * end fbe_job_service_fill_create_extent_pool_request()
 ************************************************************/

/*!**************************************************************
 * fbe_job_service_set_job_number_on_create_extent_pool_request()
 ****************************************************************
 * @brief
 * 
 * @param packet_p - packet containing the request.             
 *
 * @return fbe_status_t - Status of the processing of the request
 *                        
 * @author
 *    06-June-2014 - Created. 
 *
 ****************************************************************/
void fbe_job_service_set_job_number_on_create_extent_pool_request(fbe_packet_t * packet_p, 
                                                                      fbe_job_queue_element_t *job_queue_element_p)
{
    fbe_job_service_create_extent_pool_t   *create_pool_req_p = NULL;
    fbe_payload_ex_t                                  *payload_p = NULL;
    fbe_payload_control_operation_t                   *control_operation = NULL;
  
    job_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry\n", __FUNCTION__);

    /*! Get the request payload.
    */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload_p);
 
    fbe_payload_control_get_buffer(control_operation, &create_pool_req_p);

    create_pool_req_p->job_number = job_queue_element_p->job_number;

    return;
}
/**************************************************************************
 * end fbe_job_service_set_job_number_on_create_extent_pool_request()
 *************************************************************************/

/*!**************************************************************
 * fbe_job_service_fill_create_ext_pool_lun_request()
 ****************************************************************
 * @brief
 * build a request to create a extent pool lun
 *
 * @param packet_p - packet containing the request.             
 *
 * @return fbe_status_t - Status of the processing of the request
 *                        
 * @author
 *    11-June-2014 - Created. 
 *
 ****************************************************************/
fbe_status_t fbe_job_service_fill_create_ext_pool_lun_request(fbe_packet_t * packet_p, 
                                                              fbe_job_queue_element_t *job_queue_element_p)
{
    fbe_job_service_create_ext_pool_lun_t             *create_lun_req_p = NULL;
    fbe_payload_ex_t                                  *payload_p = NULL;
    fbe_payload_control_operation_opcode_t 		      control_code;
    fbe_payload_control_operation_t                   *control_operation = NULL;
    fbe_payload_control_buffer_length_t    		      length;
    fbe_status_t                           		      status = FBE_STATUS_OK;
  
    job_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry\n", __FUNCTION__);

    /*! Get the request payload.
    */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload_p);
    control_code = fbe_transport_get_control_code(packet_p);

    fbe_payload_control_get_buffer(control_operation, &create_lun_req_p);
    if(create_lun_req_p == NULL)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s : ERROR: create extent pool request job is NULL\n",__FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &length);
    if(length != sizeof(fbe_job_service_create_ext_pool_lun_t))
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s : ERROR: Size of create extent pool job request is invalid\n",__FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*! set the queue element initial parameters */
    status = fbe_job_service_build_queue_element(job_queue_element_p,
                                                 control_code,
                                                 FBE_OBJECT_ID_INVALID,
                                                 (fbe_u8_t *)create_lun_req_p,
                                                 sizeof(fbe_job_service_create_ext_pool_lun_t));

    if(status != FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_WARNING, 
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s: Job service operation pointer is null, status 0x%x \n",
                __FUNCTION__, status);
    }

    return status;
}
/**********************************************************
 * end fbe_job_service_fill_create_ext_pool_lun_request()
 ************************************************************/

/*!**************************************************************
 * fbe_job_service_set_job_number_on_create_ext_pool_lun_request()
 ****************************************************************
 * @brief
 * 
 * @param packet_p - packet containing the request.             
 *
 * @return fbe_status_t - Status of the processing of the request
 *                        
 * @author
 *    11-June-2014 - Created. 
 *
 ****************************************************************/
void fbe_job_service_set_job_number_on_create_ext_pool_lun_request(fbe_packet_t * packet_p, 
                                                                   fbe_job_queue_element_t *job_queue_element_p)
{
    fbe_job_service_create_ext_pool_lun_t             *create_lun_req_p = NULL;
    fbe_payload_ex_t                                  *payload_p = NULL;
    fbe_payload_control_operation_t                   *control_operation = NULL;
  
    job_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry\n", __FUNCTION__);

    /*! Get the request payload.
    */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload_p);
 
    fbe_payload_control_get_buffer(control_operation, &create_lun_req_p);

    create_lun_req_p->job_number = job_queue_element_p->job_number;

    return;
}
/**************************************************************************
 * end fbe_job_service_set_job_number_on_create_ext_pool_lun_request()
 *************************************************************************/

/*!**************************************************************
 * fbe_job_service_fill_destroy_extent_pool_request()
 ****************************************************************
 * @brief
 * build a request to create a extent pool
 *
 * @param packet_p - packet containing the request.             
 *
 * @return fbe_status_t - Status of the processing of the request
 *                        
 * @author
 *    06-June-2014 - Created. 
 *
 ****************************************************************/
fbe_status_t fbe_job_service_fill_destroy_extent_pool_request(fbe_packet_t * packet_p, 
                                                             fbe_job_queue_element_t *job_queue_element_p)
{
    fbe_job_service_destroy_extent_pool_t              *destroy_pool_req_p = NULL;
    fbe_payload_ex_t                                  *payload_p = NULL;
    fbe_payload_control_operation_opcode_t 		      control_code;
    fbe_payload_control_operation_t                   *control_operation = NULL;
    fbe_payload_control_buffer_length_t    		      length;
    fbe_status_t                           		      status = FBE_STATUS_OK;
  
    job_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry\n", __FUNCTION__);

    /*! Get the request payload.
    */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload_p);
    control_code = fbe_transport_get_control_code(packet_p);

    fbe_payload_control_get_buffer(control_operation, &destroy_pool_req_p);
    if(destroy_pool_req_p == NULL)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s : ERROR: create extent pool request job is NULL\n",__FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &length);
    if(length != sizeof(fbe_job_service_destroy_extent_pool_t))
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s : ERROR: Size of create extent pool job request is invalid\n",__FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*! set the queue element initial parameters */
    status = fbe_job_service_build_queue_element(job_queue_element_p,
                                                 control_code,
                                                 FBE_OBJECT_ID_INVALID,
                                                 (fbe_u8_t *)destroy_pool_req_p,
                                                 sizeof(fbe_job_service_destroy_extent_pool_t));

    if(status != FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_WARNING, 
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s: Job service operation pointer is null, status 0x%x \n",
                __FUNCTION__, status);
    }

    return status;
}
/**********************************************************
 * end fbe_job_service_fill_destroy_extent_pool_request()
 ************************************************************/

/*!**************************************************************
 * fbe_job_service_set_job_number_on_destroy_extent_pool_request()
 ****************************************************************
 * @brief
 * 
 * @param packet_p - packet containing the request.             
 *
 * @return fbe_status_t - Status of the processing of the request
 *                        
 * @author
 *    06-June-2014 - Created. 
 *
 ****************************************************************/
void fbe_job_service_set_job_number_on_destroy_extent_pool_request(fbe_packet_t * packet_p, 
                                                                      fbe_job_queue_element_t *job_queue_element_p)
{
    fbe_job_service_destroy_extent_pool_t   *destroy_pool_req_p = NULL;
    fbe_payload_ex_t                                  *payload_p = NULL;
    fbe_payload_control_operation_t                   *control_operation = NULL;
  
    job_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry\n", __FUNCTION__);

    /*! Get the request payload.
    */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload_p);
 
    fbe_payload_control_get_buffer(control_operation, &destroy_pool_req_p);

    destroy_pool_req_p->job_number = job_queue_element_p->job_number;

    return;
}
/**************************************************************************
 * end fbe_job_service_set_job_number_on_destroy_extent_pool_request()
 *************************************************************************/

/*!**************************************************************
 * fbe_job_service_fill_destroy_ext_pool_lun_request()
 ****************************************************************
 * @brief
 * build a request to create a extent pool lun
 *
 * @param packet_p - packet containing the request.             
 *
 * @return fbe_status_t - Status of the processing of the request
 *                        
 * @author
 *    11-June-2014 - Created. 
 *
 ****************************************************************/
fbe_status_t fbe_job_service_fill_destroy_ext_pool_lun_request(fbe_packet_t * packet_p, 
                                                              fbe_job_queue_element_t *job_queue_element_p)
{
    fbe_job_service_destroy_ext_pool_lun_t             *destroy_lun_req_p = NULL;
    fbe_payload_ex_t                                  *payload_p = NULL;
    fbe_payload_control_operation_opcode_t 		      control_code;
    fbe_payload_control_operation_t                   *control_operation = NULL;
    fbe_payload_control_buffer_length_t    		      length;
    fbe_status_t                           		      status = FBE_STATUS_OK;
  
    job_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry\n", __FUNCTION__);

    /*! Get the request payload.
    */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload_p);
    control_code = fbe_transport_get_control_code(packet_p);

    fbe_payload_control_get_buffer(control_operation, &destroy_lun_req_p);
    if(destroy_lun_req_p == NULL)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s : ERROR: create extent pool request job is NULL\n",__FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &length);
    if(length != sizeof(fbe_job_service_destroy_ext_pool_lun_t))
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s : ERROR: Size of create extent pool job request is invalid\n",__FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*! set the queue element initial parameters */
    status = fbe_job_service_build_queue_element(job_queue_element_p,
                                                 control_code,
                                                 FBE_OBJECT_ID_INVALID,
                                                 (fbe_u8_t *)destroy_lun_req_p,
                                                 sizeof(fbe_job_service_destroy_ext_pool_lun_t));

    if(status != FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_WARNING, 
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s: Job service operation pointer is null, status 0x%x \n",
                __FUNCTION__, status);
    }

    return status;
}
/**********************************************************
 * end fbe_job_service_fill_destroy_ext_pool_lun_request()
 ************************************************************/

/*!**************************************************************
 * fbe_job_service_set_job_number_on_destroy_ext_pool_lun_request()
 ****************************************************************
 * @brief
 * 
 * @param packet_p - packet containing the request.             
 *
 * @return fbe_status_t - Status of the processing of the request
 *                        
 * @author
 *    11-June-2014 - Created. 
 *
 ****************************************************************/
void fbe_job_service_set_job_number_on_destroy_ext_pool_lun_request(fbe_packet_t * packet_p, 
                                                                   fbe_job_queue_element_t *job_queue_element_p)
{
    fbe_job_service_destroy_ext_pool_lun_t             *destroy_lun_req_p = NULL;
    fbe_payload_ex_t                                  *payload_p = NULL;
    fbe_payload_control_operation_t                   *control_operation = NULL;
  
    job_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry\n", __FUNCTION__);

    /*! Get the request payload.
    */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload_p);
 
    fbe_payload_control_get_buffer(control_operation, &destroy_lun_req_p);

    destroy_lun_req_p->job_number = job_queue_element_p->job_number;

    return;
}
/**************************************************************************
 * end fbe_job_service_set_job_number_on_destroy_ext_pool_lun_request()
 *************************************************************************/
