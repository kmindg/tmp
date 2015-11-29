/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_job_service_arrival_thread.c
 ***************************************************************************
 *
 * @brief
 *  This file contains job service arrival functions that are required by
 * 	the job service arrival thread to handle how arrival requests are
 * 	received and processed 
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
#include "fbe_cmi.h"
#include "../test/fbe_job_service_test_private.h"

/*************************
 *    INLINE FUNCTIONS 
 *************************/

/*************************
 *   FUNCTION DEFINITIONS
 *************************/
static void job_service_arrival_thread_process_block_from_other_side(void);
static void job_service_arrival_thread_process_idle_state_recovery_queue(void);
static void job_service_arrival_thread_process_idle_state_creation_queue(void);
static void job_service_arrival_thread_process_complete_state_(void);
static void fbe_job_service_send_job_elements_to_peer(void);
void fbe_job_service_complete_update_job_queue(void);

/* To keep track of synced_job_elements */
static  fbe_u32_t  synced_job_elements = 0;


/* Accessor methods for the flags field.
*/
fbe_bool_t fbe_job_service_arrival_thread_is_flag_set(fbe_job_service_arrival_thread_flags_t flag)
{
    fbe_job_service_t *job_service_p = &fbe_job_service; 
    return ((job_service_p->arrival.arrival_thread_flags & flag) == flag);
}

void fbe_job_service_arrival_thread_increment_sync_element(void)
{
    synced_job_elements++;
    return;
}

void fbe_job_service_arrival_thread_init_flags()
{
    fbe_job_service_t *job_service_p = &fbe_job_service; 

    /* Set the thread to RUN only if active. For Passive, it is comming up,
     * so set it to SYNC mode to synchronize the job queue. Once done, it
     * will set it to RUN mode. 
     */ 
    job_service_p->arrival.arrival_thread_flags = (fbe_job_service_cmi_is_active()) ? 
        FBE_JOB_SERVICE_ARRIVAL_THREAD_FLAGS_RUN : FBE_JOB_SERVICE_ARRIVAL_THREAD_FLAGS_SYNC;

    return;
}

void fbe_job_service_arrival_thread_set_flag(fbe_job_service_arrival_thread_flags_t flag)
{
    fbe_job_service_t *job_service_p = &fbe_job_service; 
    job_service_p->arrival.arrival_thread_flags = flag;
    return;
}

void fbe_job_service_arrival_thread_clear_flag(fbe_job_service_arrival_thread_flags_t flag)
{
    fbe_job_service_t *job_service_p = &fbe_job_service; 
    job_service_p->arrival.arrival_thread_flags  &= ~flag;
    return;
}

/*!**************************************************************
 * job_service_arrival_thread_func()
 ****************************************************************
 * @brief
 * runs the job control arrival thread
 *
 * @param context - unreferenced value             
 *
 * @return fbe_status_t - status of call to start job service thread
 *
 * @author
 * 08/24/2010 - Created. Jesus Medina
 *
 ****************************************************************/

void job_service_arrival_thread_func(void * context)
{
    fbe_status_t  status;

    /* arrival thread time out is 100 msec */

    FBE_UNREFERENCED_PARAMETER(context);

    job_service_trace(FBE_TRACE_LEVEL_DEBUG_MEDIUM, 
                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, 
                      "%s job_service_arrival_thread_func\n", 
                      __FUNCTION__);

    fbe_cmi_sync_open(FBE_CMI_CLIENT_ID_JOB_SERVICE);

    /* Now that CMI is up, it is time to sync up the job queue with Active side. */
    if(!fbe_job_service_cmi_is_active())
    {
        if(fbe_job_service_arrival_thread_is_flag_set(FBE_JOB_SERVICE_ARRIVAL_THREAD_FLAGS_SYNC)) 
        {
            job_service_trace(FBE_TRACE_LEVEL_INFO, 
                              FBE_TRACE_MESSAGE_ID_INFO, 
                              "We (PASSIVE) are coming up... Let's check if we have any jobs from ACTIVE to Sync\n");

            /* If passive, then sync the job elements from active side. */
            status = fbe_job_service_cmi_get_job_elements_from_peer();
        }
    }

    while(1)    
    {
        if ((!fbe_job_service_arrival_thread_is_flag_set(FBE_JOB_SERVICE_ARRIVAL_THREAD_FLAGS_RUN)) &&
            (!fbe_job_service_arrival_thread_is_flag_set(FBE_JOB_SERVICE_ARRIVAL_THREAD_FLAGS_SYNC)))
        {
            break;
        }

        /* First check if we have a valid block from other side */
        /* Fix AR 551604: there is a deadlock when both SP sync the arrival queue to peer.
         * In fact, this thread can sync a job from arrival queue to peer, at the same time, 
         * process a job from other side.
         * So I removed the fbe_job_service_cmi_get_state() == FBE_JOB_SERVICE_CMI_STATE_IDLE
         * If the cmi state is COMPLETED, make sure process it with higher priority than receiving 
         * */
        if (fbe_job_service_cmi_is_block_from_other_side_valid() && 
            fbe_job_service_cmi_get_state() != FBE_JOB_SERVICE_CMI_STATE_COMPLETED)
        {
            job_service_trace(FBE_TRACE_LEVEL_INFO, 
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "Job Service: Block from other side is valid.  Start processing.. \n");

            job_service_arrival_thread_process_block_from_other_side();
        }

        /* if we are here, it means when arrival queue woke up, peer had not sent
         * a message yet 
         */
        if (fbe_job_service_cmi_get_state() == FBE_JOB_SERVICE_CMI_STATE_IDLE)
        {
            /* If there is a packet in any of the arrival queues...
             * get packet from arrival queue, make sure there is a valid packet 
             */
            if ((!fbe_queue_is_empty(&fbe_job_service.arrival.arrival_recovery_q_head)) ||
                    (!fbe_queue_is_empty(&fbe_job_service.arrival.arrival_creation_q_head)))
            {
                /* Process items on the arrival recovery queue first
                */
                if ((!fbe_queue_is_empty(&fbe_job_service.arrival.arrival_recovery_q_head))  &&
                        (fbe_job_service_get_arrival_recovery_queue_access() == FBE_TRUE))
                {
                    job_service_arrival_thread_process_idle_state_recovery_queue();
                }
                else if ((!fbe_queue_is_empty(&fbe_job_service.arrival.arrival_creation_q_head))  &&
                        (fbe_job_service_get_arrival_creation_queue_access() == FBE_TRUE))
                {
                    job_service_arrival_thread_process_idle_state_creation_queue();
                }
            }
        }

        /* this code completes a message transaction between SP when adding to execution queues */
        if (fbe_job_service_cmi_get_state() == FBE_JOB_SERVICE_CMI_STATE_COMPLETED)
        {
            job_service_arrival_thread_process_complete_state_();
        }

        status = fbe_semaphore_wait_ms(&fbe_job_service.arrival.job_service_arrival_queue_semaphore, 100);
    }

    fbe_job_service_arrival_thread_set_flag(FBE_JOB_SERVICE_ARRIVAL_THREAD_FLAGS_DONE); 
    fbe_thread_exit(EMCPAL_STATUS_SUCCESS);
}
/***************************************
 * end job_service_arrival_thread_func()
 **************************************/

/*!**************************************************************
 * fbe_job_service_arrival_thread_validate_queue_element()
 ****************************************************************
 * @brief
 * validates the queue element has proper job type, queue type
 *
 * @param - packet contains control code use to determine job type       
 * @param - job_type filled if proper job type found       
 * @param - job_queue_type filled if proper queue type found       
 *
 * @return fbe_status_t - status of call
 *
 * @author
 * 09/15/2010 - Created. Jesus Medina
 *
 ****************************************************************/

fbe_status_t fbe_job_service_arrival_thread_validate_queue_element(fbe_packet_t * packet_p,
        fbe_job_type_t *job_type, 
        fbe_job_control_queue_type_t *queue_type)
{
    fbe_status_t status = FBE_STATUS_OK;

    status = fbe_job_service_get_job_type(packet_p, job_type);
    if (status != FBE_STATUS_OK)
    {
        return status;
    }

    status = job_service_request_get_queue_type(packet_p, queue_type);
    if (status != FBE_STATUS_OK)
    {
        return status;
    }

    status = fbe_job_service_find_service_operation(*job_type);
    if (status != FBE_STATUS_OK)
    {
        return status;
    }

    return FBE_STATUS_OK;
}
/************************************************************
 * end fbe_job_service_arrival_thread_validate_queue_element()
 ***********************************************************/

/*!**************************************************************
 * job_service_arrival_thread_process_block_from_other_side()
 ****************************************************************
 * @brief
 * runs the job control arrival thread
 *
 * @param context - unreferenced value             
 *
 * @return fbe_status_t - status of call to start job service thread
 *
 * @author
 * 08/24/2010 - Created. Jesus Medina
 *
 ****************************************************************/
static void job_service_arrival_thread_process_block_from_other_side(void)
{
    fbe_packet_t                            *packet_p;
    fbe_job_queue_element_t                 *job_queue_element_p;

    /* allocate job queue element */
    job_queue_element_p = (fbe_job_queue_element_t *)fbe_transport_allocate_buffer();
    if(job_queue_element_p == NULL)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s : ERROR: Could not allocate memory for job request\n",
                          __FUNCTION__);

        return;
    }

    /* copy block from other side */
    fbe_job_service_cmi_copy_block_from_other_side(job_queue_element_p);

    job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
            " -- Block from other side state -- \n");

    job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
            "%s entry\n", __FUNCTION__);

    job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "    This SP is the %s side\n", fbe_job_service_cmi_is_active() ? "ACTIVE" : "PASSIVE");

    /* On the Active context, if the job_type is FBE_JOB_TYPE_UPDATE_JOB_ELEMENTS_ON_PEER 
     * we should first sync the Active side's job queue to the Passive.. 
     */
    if((fbe_job_service_arrival_thread_is_flag_set(FBE_JOB_SERVICE_ARRIVAL_THREAD_FLAGS_SYNC)) && 
       (job_queue_element_p->job_type == FBE_JOB_TYPE_UPDATE_JOB_ELEMENTS_ON_PEER)) 
    {
        /** Fix AR 566368, which is introduced by fixing AR 551604.
         * The arrival thread may wake up after 100ms by itself. But if it is syncing the
         * jobs to peer, and the last job is still not finished by peer SP and return ADDED cmi message.
         * In this case, we don't allow to send the second job to peer. 
         * Here we require the condition: fbe_job_service_cmi_get_state() == FBE_JOB_SERVICE_CMI_STATE_IDLE,
         * which will be set to IDLE after receiving ADDED cmi message.
         */
        if (fbe_job_service_cmi_get_state() != FBE_JOB_SERVICE_CMI_STATE_IDLE)
        {
            fbe_transport_release_buffer(job_queue_element_p);
            return;
        }
        /* Update job elements to Peer until both are not equal... */
        if(synced_job_elements != fbe_job_service_get_number_creation_requests())
        {
            job_service_trace(FBE_TRACE_LEVEL_INFO, 
                              FBE_TRACE_MESSAGE_ID_INFO, 
                              "ACTIVE processing UPDATE_JOB_ELEMENTS_ON_PEER.\n");
    
            fbe_job_service_send_job_elements_to_peer();
        }
        else
        {
            /* If equal, complete the job. */
            job_service_trace(FBE_TRACE_LEVEL_INFO, 
                              FBE_TRACE_MESSAGE_ID_INFO, 
                              "Either ALL or NO jobs are sync'd.\n");
    
            fbe_job_service_complete_update_job_queue();
        }
        /* At this point we are done with the processing of the job control element. 
         * So release it.. 
         */
        fbe_transport_release_buffer(job_queue_element_p);
        return;
    }

    /* if active side, generate new job number */
    if (fbe_job_service_cmi_is_active())
    {
        job_queue_element_p->job_number = fbe_job_service_increment_job_number_with_lock();	
        fbe_job_service.job_service_cmi_message_p->job_number = job_queue_element_p->job_number;
    }
    else
    {
        /* at this point, job_queue_element_p->job_number has the proper 
         * job number, either from the peer--as active side--
         * or from this side as active side, update the overall job 
         * service job number to keep both SPs view of the job number in sync 
         */
        fbe_job_service.job_service_cmi_message_p->job_number = job_queue_element_p->job_number;
        fbe_job_service_set_job_number(job_queue_element_p->job_number);
    }
    
    /* print contents of queue element */
    fbe_job_service_cmi_print_contents_from_queue_element(job_queue_element_p);

    job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
            " -- Block from other side state -- \n");

    job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
            "%s entry\n", __FUNCTION__);

    job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "    Job number: 0x%x in Block From Other Side \n", (unsigned int)job_queue_element_p->job_number);

    /* get packet out of the job queue element */
    packet_p = job_queue_element_p->packet_p;

    /* add element to one of the queues */
    fbe_job_service_arrival_thread_add_element_to_queue(job_queue_element_p);
    
    job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
        " QUEUE_TYPE %d -- \n",job_queue_element_p->queue_type);

    /* reply back queue element has been added */
    switch (job_queue_element_p->queue_type)
    {
        job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                " -- Block from other side state -- \n");

        job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                "%s entry\n", __FUNCTION__);

        case FBE_JOB_RECOVERY_QUEUE:
            job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                    "    job number 0x%x error_code: %d added to recovery queue\n", 
                    (unsigned int)job_queue_element_p->job_number, job_queue_element_p->error_code);

            fbe_job_service.job_service_cmi_message_p->job_service_cmi_message_type = 
                FBE_JOB_CONTROL_SERVICE_CMI_MESSAGE_TYPE_ADDED_TO_RECOVERY_QUEUE;

            break;

        case FBE_JOB_CREATION_QUEUE:
            job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                    "    job number 0x%x added to creation queue\n", 
                    (unsigned int)job_queue_element_p->job_number);

            fbe_job_service.job_service_cmi_message_p->job_service_cmi_message_type = 
                FBE_JOB_CONTROL_SERVICE_CMI_MESSAGE_TYPE_ADDED_TO_CREATION_QUEUE;

            break;
    }

    /* once we have replied to peer, reset block from other side to NULL */
    fbe_job_service_cmi_invalidate_block_from_other_side();

    /* now send a reply to peer SP */	
    fbe_job_service_cmi_send_message(fbe_job_service.job_service_cmi_message_p, packet_p);
    
    /* We need release the semaphore after sending the ADDED CMI message. 
      In this way, active side can ensure that the REMOVE message will always 
      follow the ADDED message. Wenxuan Yin, 2/27/2013 */
    /* only want to call job service thread on active side */
    if (fbe_job_service_cmi_is_active())
    {
        fbe_semaphore_release(&fbe_job_service.job_control_queue_semaphore, 0, 1, FALSE);
    }
    
}

/*!**************************************************************
 * job_service_arrival_thread_process_idle_state_recovery_queue()
 ****************************************************************
 * @brief
 * runs the job control arrival thread
 *
 * @param context - unreferenced value             
 *
 * @return fbe_status_t - status of call to start job service thread
 *
 * @author
 * 08/24/2010 - Created. Jesus Medina
 *
 ****************************************************************/
static void job_service_arrival_thread_process_idle_state_recovery_queue(void)
{
    fbe_job_control_queue_type_t            queue_type = FBE_JOB_INVALID_QUEUE;
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_packet_t                            *packet_p = NULL;
    fbe_payload_ex_t                           *payload_p = NULL;
    fbe_payload_control_operation_t         *control_operation = NULL;
    fbe_job_queue_element_t                 *job_queue_element_p = NULL;
    fbe_job_type_t                          job_type = FBE_JOB_TYPE_INVALID;

    job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
            " -- Idle state -- \n");
    job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
            "%s entry\n", __FUNCTION__);
    job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
            "%s SP: Retrieving request from arrival recovery queue.\n", 
                      fbe_job_service_cmi_is_active() ? "ACTIVE" : "PASSIVE");

    /* get packet from arrival recovery queue */
    fbe_job_service_get_packet_from_arrival_recovery_queue(&packet_p);

    /* check if queue element has proper value set. ie, proper
     * job service registration
     */
    status = fbe_job_service_arrival_thread_validate_queue_element(packet_p, &job_type, &queue_type);

    /* on failure, complete packet */
    if (status != FBE_STATUS_OK)
    {
        /*! Get the request payload. */
        payload_p = fbe_transport_get_payload_ex(packet_p);
        control_operation = fbe_payload_ex_get_control_operation(payload_p);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);

        /* complete client's packet */
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return;
    }

    /* allocate a job queue element, call to allocate queue element
     * will also fill data of queue element
     */
    status = fbe_job_service_allocate_and_fill_recovery_queue_element(packet_p, 
                                                                      &job_queue_element_p,
                                                                      job_type,
                                                                      queue_type);

    /* on failure, complete packet */
    if (status != FBE_STATUS_OK)
    {
        /*! Get the request payload. */
        payload_p = fbe_transport_get_payload_ex(packet_p);
        control_operation = fbe_payload_ex_get_control_operation(payload_p);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);

        /* complete client's packet */
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return;
    }

    /* we do not have a job number yet */
    job_queue_element_p->job_number = 0;

    job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
            " -- Idle state -- \n");
    job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
            "%s entry\n", __FUNCTION__);

    /* if active side, generate new job number */
    if (fbe_job_service_cmi_is_active())
    {
        job_queue_element_p->job_number = fbe_job_service_increment_job_number_with_lock();
        job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                "   arrival queue %d, job_number %llu, Active side\n",
                job_queue_element_p->queue_type,
		(unsigned long long)job_queue_element_p->job_number);

        fbe_job_service.job_service_cmi_message_p->job_number = job_queue_element_p->job_number;

        /* place job number in packet, so client of job service can wait on
         * notification call
         */
        fbe_job_service_set_job_number_on_packet(packet_p, job_queue_element_p);
    }

    /* associate block with packet */
    job_queue_element_p->packet_p = packet_p;

    job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
            "    Packet to use for peer to peer communication packet 0x%p \n", packet_p);	

    /* update block from other side to point to current block just allocated */
    fbe_job_service_cmi_set_block_in_progress_block_from_other_side(job_queue_element_p);

    /* set state, queue type */
    fbe_job_service_cmi_set_state_on_block_in_progress(FBE_JOB_SERVICE_CMI_STATE_IN_PROGRESS);
    fbe_job_service.job_service_cmi_message_p->queue_type = job_queue_element_p->queue_type;

    /* set message type */
    fbe_job_service.job_service_cmi_message_p->job_service_cmi_message_type = 
        FBE_JOB_CONTROL_SERVICE_CMI_MESSAGE_TYPE_ADD_TO_RECOVERY_QUEUE;

    fbe_job_service_arrival_thread_copy_queue_element(
            &fbe_job_service.job_service_cmi_message_p->msg.job_service_queue_element.job_element,
            job_queue_element_p);

    job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
            " -- Idle state -- \n");

    job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
            "%s entry\n", __FUNCTION__);

    job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
            "    sending request to peer SP to add to queue for job number %llu\n", 
            (unsigned long long)fbe_job_service.job_service_cmi_message_p->job_number);	

    fbe_job_service_cmi_send_message(fbe_job_service.job_service_cmi_message_p, packet_p);
}

/*!**************************************************************
 * job_service_arrival_thread_process_idle_state_creation_queue()
 ****************************************************************
 * @brief
 * runs the job control arrival thread
 *
 * @param context - unreferenced value             
 *
 * @return fbe_status_t - status of call to start job service thread
 *
 * @author
 * 08/24/2010 - Created. Jesus Medina
 *
 ****************************************************************/
static void job_service_arrival_thread_process_idle_state_creation_queue(void)
{
    fbe_job_control_queue_type_t            queue_type = FBE_JOB_INVALID_QUEUE;
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_packet_t                            *packet_p = NULL;
    fbe_payload_ex_t                           *payload_p = NULL;
    fbe_payload_control_operation_t         *control_operation = NULL;
    fbe_job_queue_element_t                 *job_queue_element_p = NULL;
    fbe_job_type_t                          job_type = FBE_JOB_TYPE_INVALID;

    job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
            " -- Idle state -- \n");
    job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
            "%s entry\n", __FUNCTION__);
    job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
            "%s SP: Retrieving request from arrival creation queue.\n", 
                      fbe_job_service_cmi_is_active() ? "ACTIVE" : "PASSIVE");

    /* get packet out of the creation arrival queue */
    fbe_job_service_get_packet_from_arrival_creation_queue(&packet_p);

    /* check if queue element has proper value set. ie, proper
     * job service registration
     */
    status = fbe_job_service_arrival_thread_validate_queue_element(packet_p, &job_type, &queue_type);

    /* on failure, complete packet */
    if (status != FBE_STATUS_OK)
    {
        /*! Get the request payload. */
        payload_p = fbe_transport_get_payload_ex(packet_p);
        control_operation = fbe_payload_ex_get_control_operation(payload_p);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);

        /* complete client's packet */
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return;
    }

    /* allocate a job queue element, call to allocate queue element
     * will also fill data of queue element
     */	
    status = fbe_job_service_allocate_and_fill_creation_queue_element(packet_p, 
                                                                      &job_queue_element_p,
                                                                      job_type,
                                                                      queue_type);

    /* on failure, complete packet */
    if (status != FBE_STATUS_OK)
    {
        /*! Get the request payload. */
        payload_p = fbe_transport_get_payload_ex(packet_p);
        control_operation = fbe_payload_ex_get_control_operation(payload_p);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);

        /* complete client's packet */
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return;
    }

    /* we do not have a job number yet */
    job_queue_element_p->job_number = 0;
    job_queue_element_p->object_id = FBE_OBJECT_ID_INVALID;
    job_queue_element_p->need_to_wait =  FBE_FALSE;
    job_queue_element_p->timeout_msec = 60000;

    job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
            " -- Idle state -- \n");
    job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
            "%s entry\n", __FUNCTION__);

    /* if active side, generate new job number */
    if (fbe_job_service_cmi_is_active())
    {
        job_queue_element_p->job_number = fbe_job_service_increment_job_number_with_lock();
        job_service_trace(FBE_TRACE_LEVEL_INFO, 
                          FBE_TRACE_MESSAGE_ID_INFO, 
                          "Active side: Arrival queue - job_number: %llu\n",
			  (unsigned long long)job_queue_element_p->job_number);

        fbe_job_service.job_service_cmi_message_p->job_number = job_queue_element_p->job_number;

        /* place job number in packet, so client of job service can wait on
         * notification call
         */
        fbe_job_service_set_job_number_on_packet(packet_p, job_queue_element_p);
    }

    /* associate block with packet */
    job_queue_element_p->packet_p = packet_p;

    job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
            "    Packet to use for peer to peer communication packet 0x%p \n", packet_p);	

    /* update block from other side to point to current block just allocated */
    fbe_job_service_cmi_set_block_in_progress_block_from_other_side(job_queue_element_p);

    /* set state, queue type */
    fbe_job_service_cmi_set_state_on_block_in_progress(FBE_JOB_SERVICE_CMI_STATE_IN_PROGRESS);

    fbe_job_service.job_service_cmi_message_p->queue_type = job_queue_element_p->queue_type;

    fbe_job_service.job_service_cmi_message_p->job_service_cmi_message_type = 
        FBE_JOB_CONTROL_SERVICE_CMI_MESSAGE_TYPE_ADD_TO_CREATION_QUEUE;

    fbe_job_service_arrival_thread_copy_queue_element(
            &fbe_job_service.job_service_cmi_message_p->msg.job_service_queue_element.job_element,
            job_queue_element_p);

    job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
            " -- Idle state -- \n");

    job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
            "%s entry\n", __FUNCTION__);

    job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
            "    sending request to peer SP to add to queue for job number %llu\n", 
            (unsigned long long)fbe_job_service.job_service_cmi_message_p->job_number);	

    fbe_job_service_cmi_send_message(fbe_job_service.job_service_cmi_message_p, packet_p);
}

/*!**************************************************************
 * job_service_arrival_thread_process_complete_state_()
 ****************************************************************
 * @brief
 * runs the job control arrival thread
 *
 * @param context - unreferenced value             
 *
 * @return fbe_status_t - status of call to start job service thread
 *
 * @author
 * 08/24/2010 - Created. Jesus Medina
 *
 ****************************************************************/
static void job_service_arrival_thread_process_complete_state_(void)
{
    fbe_packet_t                            *packet_p = NULL;
    fbe_payload_ex_t                           *payload_p = NULL;
    fbe_payload_control_operation_t         *control_operation = NULL;
    fbe_job_queue_element_t                 *job_queue_element_p = NULL;

    /* This if..block runs in the Active SP's context. If the thread is set in SYNC mode, 
     * set it to RUN, and set the CMI to IDLE. It is also necessary to release the 
     * arrival thread semaphore... 
     */
    if(fbe_job_service_arrival_thread_is_flag_set(FBE_JOB_SERVICE_ARRIVAL_THREAD_FLAGS_SYNC) &&
       fbe_job_service_thread_is_flag_set(FBE_JOB_SERVICE_THREAD_FLAGS_SYNC))
    {
        /* Verify if all the job elements are in Sync.. If so, unfreeze the queues... */
        if(synced_job_elements == fbe_job_service_get_number_creation_requests())
        {
            job_service_trace(FBE_TRACE_LEVEL_INFO, 
                              FBE_TRACE_MESSAGE_ID_INFO, 
                              "%s: ACTIVE sync'd - %d Job(s) with PASSIVE.\n", 
                              __FUNCTION__, synced_job_elements);
            
            fbe_job_service_complete_update_job_queue();
        }
        fbe_job_service_cmi_set_state_on_block_in_progress(FBE_JOB_SERVICE_CMI_STATE_IDLE);
    }
    else
    {
        job_queue_element_p = fbe_job_service_cmi_get_queue_element_from_block_in_progress();
        if (job_queue_element_p == NULL)
        {
            job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s no blocked queue element.\n", __FUNCTION__);
            fbe_job_service_cmi_set_state_on_block_in_progress(FBE_JOB_SERVICE_CMI_STATE_IDLE);
    
            /* set block in progress block from other side to NULL */
            fbe_job_service_cmi_invalidate_block_in_progress_block_from_other_side();
            return;
        }
    
        /* get packet out of the job queue element */
        packet_p = fbe_job_service_cmi_get_packet_from_queue_element();
    
        if(packet_p == NULL){ /* We need to understand why this is happening */
            job_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, " -- Warning zero packet -- \n");
            return;
        }
    
        job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                " -- Complete state -- \n");
    
        job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                "%s entry\n", __FUNCTION__);
    
        job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "    packet 0x%p\n", packet_p);
    
        /* job number on passive side is not generated, must be copied from active side */
        if (!fbe_job_service_cmi_is_active())
        {
            job_queue_element_p->job_number = fbe_job_service_cmi_get_job_number_from_block_in_progress();
    
            /* we need to sync this SP's job service number here */
            fbe_job_service_set_job_number(job_queue_element_p->job_number);
    
            /* place job number in packet to client only in the case of passive side */
            fbe_job_service_set_job_number_on_packet(packet_p, job_queue_element_p);
    
            job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
		              "    Passive side, job_number %llu\n",
			      (unsigned long long)job_queue_element_p->job_number);
        }
    
        fbe_job_service_cmi_print_contents_from_queue_element(job_queue_element_p);
    
        /* put new allocated queue element into proper execution queue */
        fbe_job_service_arrival_thread_add_element_to_queue(job_queue_element_p);

        /* only want to call job service thread on active side */
        if (fbe_job_service_cmi_is_active())
        {
            fbe_semaphore_release(&fbe_job_service.job_control_queue_semaphore, 0, 1, FALSE);
        }
    
        /*! Get the request payload. */
        payload_p = fbe_transport_get_payload_ex(packet_p);
        control_operation = fbe_payload_ex_get_control_operation(payload_p);
    
        /* complete packet */
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    
        /* set state */
        fbe_job_service_cmi_set_state_on_block_in_progress(FBE_JOB_SERVICE_CMI_STATE_IDLE);
    
        /* set block in progress block from other side to NULL */
        fbe_job_service_cmi_invalidate_block_in_progress_block_from_other_side();
    
        /* complete client's packet */
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        /* Release the semaphore.. */
        fbe_semaphore_release(&fbe_job_service.arrival.job_service_arrival_queue_semaphore, 0, 1, FALSE);
    }

    return;
}

/*!**************************************************************
 * fbe_job_service_arrival_thread_add_element_to_queue()
 ****************************************************************
 * @brief
 * validates the queue element has proper job type, queue type
 *
 * @param - packet contains control code use to determine job type       
 * @param - job_type filled if proper job type found       
 *
 * @return none
 *
 * @author
 * 09/15/2010 - Created. Jesus Medina
 *
 ****************************************************************/

void fbe_job_service_arrival_thread_add_element_to_queue(fbe_job_queue_element_t *job_queue_element_p)
{
    switch (job_queue_element_p->queue_type)
    {
        case FBE_JOB_RECOVERY_QUEUE:
            fbe_job_service_add_element_to_recovery_queue(job_queue_element_p);
            break;
        case FBE_JOB_CREATION_QUEUE:
            fbe_job_service_add_element_to_creation_queue(job_queue_element_p);
            break;
    }
    return;
}
/************************************************************
 * end fbe_job_service_arrival_thread_add_element_to_queue()
 ***********************************************************/


/*!****************************************************
 * fbe_job_service_arrival_thread_copy_queue_element()
 ******************************************************
 * @brief
 * copy queue element contents to another queue element
 *
 * @param         
 *
 * @return - none
 *
 * @author
 * 09/17/2010 - Created. Jesus Medina
 *
 ****************************************************************/

void fbe_job_service_arrival_thread_copy_queue_element(fbe_job_queue_element_t *out_job_queue_element_p,  
        fbe_job_queue_element_t *in_job_queue_element)
{
    job_service_trace(FBE_TRACE_LEVEL_INFO, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "job_arrival_cp_queue_elem entry\n");

    /*! Lock queue */
    fbe_job_service_arrival_lock();

    out_job_queue_element_p->queue_type = in_job_queue_element->queue_type;
    out_job_queue_element_p->job_type = in_job_queue_element->job_type;
    out_job_queue_element_p->timestamp = in_job_queue_element->timestamp;
    out_job_queue_element_p->current_state = in_job_queue_element->current_state;
    out_job_queue_element_p->previous_state = in_job_queue_element->previous_state;
    out_job_queue_element_p->queue_access = in_job_queue_element->queue_access;
    out_job_queue_element_p->num_objects = in_job_queue_element->num_objects;
    out_job_queue_element_p->job_number = in_job_queue_element->job_number;
    out_job_queue_element_p->job_action = in_job_queue_element->job_action;
    out_job_queue_element_p->command_size = in_job_queue_element->command_size;
    //out_job_queue_element_p->packet = in_job_queue_element.packet;
    out_job_queue_element_p->need_to_wait = in_job_queue_element->need_to_wait;
    out_job_queue_element_p->timeout_msec = in_job_queue_element->timeout_msec;

    fbe_copy_memory(out_job_queue_element_p->command_data, &in_job_queue_element->command_data, FBE_JOB_ELEMENT_CONTEXT_SIZE);

    fbe_job_service_arrival_unlock();
    return;

}
/*********************************************************
 * end fbe_job_service_arrival_thread_copy_queue_element()
 ********************************************************/


/*!****************************************************
 * fbe_job_service_send_job_elements_to_peer()
 ******************************************************
 * @brief
 * This function sends the job elements (one at a time)
 * from active to passive to sync up.
 *
 * @param         
 *
 * @return - none
 *
 * @author
 * 10/21/2011 - Created. Arun S
 *
 ****************************************************************/
static void fbe_job_service_send_job_elements_to_peer(void)
{
    fbe_queue_head_t                *head_p = NULL;
    fbe_job_queue_element_t         *job_queue_element_p = NULL;

    /* See if we have requests in the queue.. if so, sync it.. */
    if(fbe_job_service_get_number_creation_requests() == 0)
    {
        job_service_trace(FBE_TRACE_LEVEL_INFO, 
                          FBE_TRACE_MESSAGE_ID_INFO, 
                          "No jobs in Queue. No sync required.\n");

        /* Complete the job.. */
        fbe_job_service_complete_update_job_queue();
        return;
    }

    /* if queue is empty, return status no object */
    if (fbe_queue_is_empty(&fbe_job_service.creation_q_head) == TRUE)
    {
        job_service_trace(FBE_TRACE_LEVEL_INFO, 
                          FBE_TRACE_MESSAGE_ID_INFO, 
                          "No Jobs in Job Q. No need to sync..\n");

        /* Complete the job.. */
        fbe_job_service_complete_update_job_queue();
        return;
    }
    else
    {
        fbe_job_service_get_creation_queue_head(&head_p);
        job_queue_element_p = fbe_job_service_cmi_get_queue_element_from_block_in_progress();
        if (job_queue_element_p == NULL)
        {
            job_service_trace(FBE_TRACE_LEVEL_INFO, 
                              FBE_TRACE_MESSAGE_ID_INFO, 
                              "Job element from Block is NULL.. Grab it from head of the queue..\n");

            job_queue_element_p = (fbe_job_queue_element_t *) fbe_queue_front(head_p);
        }
        else
        {
            job_service_trace(FBE_TRACE_LEVEL_INFO, 
                              FBE_TRACE_MESSAGE_ID_INFO, 
                              "Job element is next in the block_in_progress..\n");

            job_queue_element_p = (fbe_job_queue_element_t *)fbe_queue_next(head_p, &job_queue_element_p->queue_element);
        }

        /* Check if we have valid data in it.. */
        if(job_queue_element_p != NULL)
        {
            job_service_trace(FBE_TRACE_LEVEL_INFO, 
                              FBE_TRACE_MESSAGE_ID_INFO, 
                              "%s: Outstanding Job count to Sync: %d\n", 
                              __FUNCTION__, 
                              (fbe_job_service_get_number_creation_requests() - synced_job_elements));

            /* Update block from other side to point to current block just allocated */
            fbe_job_service_cmi_set_block_in_progress_block_from_other_side(job_queue_element_p);
    
            /* Set state, queue type */
            fbe_job_service_cmi_set_state_on_block_in_progress(FBE_JOB_SERVICE_CMI_STATE_IN_PROGRESS);
    
            /* Print contents of queue element */
            fbe_job_service_cmi_print_contents_from_queue_element(job_queue_element_p);
    
            job_service_trace(FBE_TRACE_LEVEL_INFO, 
                              FBE_TRACE_MESSAGE_ID_INFO, 
                              "Synching Job: %llu.\n",
			      (unsigned long long)job_queue_element_p->job_number);
    
            fbe_job_service.job_service_cmi_message_p->queue_type = job_queue_element_p->queue_type;
            fbe_job_service.job_service_cmi_message_p->job_service_cmi_message_type = 
                FBE_JOB_CONTROL_SERVICE_CMI_MESSAGE_TYPE_ADD_TO_CREATION_QUEUE;
    
            /* Copy the job element to send it to the other side.. */
            fbe_job_service_arrival_thread_copy_queue_element(
                    &fbe_job_service.job_service_cmi_message_p->msg.job_service_queue_element.job_element,
                    job_queue_element_p);
    
            /* now send message to peer SP */	
            fbe_job_service_cmi_send_message(fbe_job_service.job_service_cmi_message_p, NULL);
    
            job_service_trace(FBE_TRACE_LEVEL_INFO, 
                              FBE_TRACE_MESSAGE_ID_INFO, 
                              "Job: %llu is now sent to sync with PASSIVE.\n",
                              
			      (unsigned long long)job_queue_element_p->job_number);
        }
        else
        {
            job_service_trace(FBE_TRACE_LEVEL_INFO, 
                              FBE_TRACE_MESSAGE_ID_INFO, 
                              "No more elements in Job Q. SYNC (sending) complete..\n");
        }
    }

    return;
}

/*!****************************************************
 * fbe_job_service_send_job_elements_to_peer()
 ******************************************************
 * @brief
 * This function completes the job_element sync operation.
 * We (Active) send a DONE msg to PASSIVE, unfreeze the queues,
 * set the thread mode to RUN and release the semaphore. 
 *
 * @param         
 *
 * @return - none
 *
 * @author
 * 10/21/2011 - Created. Arun S
 *
 ****************************************************************/
void fbe_job_service_complete_update_job_queue(void) 
{
    fbe_job_queue_element_t *job_queue_element_p = NULL;

    job_queue_element_p = (fbe_job_queue_element_t *)fbe_transport_allocate_buffer();
    if(job_queue_element_p == NULL)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s : ERROR: Could not allocate memory for job request\n",__FUNCTION__);

        return;
    }

    /* Initialize it to 0.. */
    synced_job_elements = 0;

    job_queue_element_p->job_type = FBE_JOB_TYPE_INVALID;
    job_queue_element_p->queue_type = FBE_JOB_CREATION_QUEUE;

    if (fbe_job_service.job_service_cmi_message_p == NULL) 
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, 
                          FBE_TRACE_MESSAGE_ID_INFO, 
                          "%s: cannot allocate memory for Job service cmi messages\n", 
                          __FUNCTION__);

        fbe_transport_release_buffer(job_queue_element_p);
        return;
    }

    fbe_job_service_arrival_thread_copy_queue_element(
            &fbe_job_service.job_service_cmi_message_p->msg.job_service_queue_element.job_element,
            job_queue_element_p);

    fbe_job_service.job_service_cmi_message_p->job_service_cmi_message_type = 
        FBE_JOB_CONTROL_SERVICE_CMI_MESSAGE_TYPE_UPDATE_JOB_QUEUE_DONE;

    job_service_trace(FBE_TRACE_LEVEL_INFO, 
                      FBE_TRACE_MESSAGE_ID_INFO, 
                      "%s: ACTIVE sending UPDATE_JOB_DONE to PASSIVE. \n", 
                      __FUNCTION__);

   /* now send message to peer SP */	
    fbe_job_service_cmi_send_message(fbe_job_service.job_service_cmi_message_p, NULL);

    /* Set the thread flag to RUN mode.. */
    fbe_job_service_arrival_thread_set_flag(FBE_JOB_SERVICE_ARRIVAL_THREAD_FLAGS_RUN); 
    fbe_job_service_thread_set_flag(FBE_JOB_SERVICE_THREAD_FLAGS_RUN); 

    /* Enable the queue access */
    fbe_job_service_handle_queue_access(FBE_TRUE);

    /* once we have replied to peer, reset block from other side to NULL */
    fbe_job_service_cmi_invalidate_block_in_progress_block_from_other_side();
    fbe_job_service_cmi_invalidate_block_from_other_side();

    /* At this point we are done with the processing of the job control element. 
     * So release it.. 
     */
    fbe_transport_release_buffer(job_queue_element_p);
 
    /* Release the semaphore.. */
    fbe_semaphore_release(&fbe_job_service.arrival.job_service_arrival_queue_semaphore, 0, 1, FALSE);

    return;
}
