#include "fbe/fbe_transport.h"
#include "fbe_transport_memory.h"
#include "fbe_job_service_arrival_thread.h"
#include "fbe_job_service_private.h"
#include "fbe_job_service_cmi.h"
#include "fbe_job_service.h"
#include "fbe_cmi.h"
#include "fbe/fbe_base_config.h"
#include "fbe_topology.h"
#include "fbe_notification.h"
#include "fbe_job_service_arrival_private.h"
#include "fbe/fbe_object.h"

static fbe_job_service_cmi_block_in_progress_t block_in_progress;
static fbe_job_queue_element_t block_from_other_side;
static fbe_spinlock_t fbe_job_service_cmi_spinlock;


fbe_status_t
fbe_job_service_cmi_send_message(fbe_job_service_cmi_message_t * job_service_cmi_message, fbe_packet_t * packet)
{
    fbe_cmi_send_message(FBE_CMI_CLIENT_ID_JOB_SERVICE, 
            sizeof(fbe_job_service_cmi_message_t),
            (fbe_cmi_message_t)job_service_cmi_message,
            (fbe_cmi_event_callback_context_t)packet);

	/* It is very bad, but we have design problem, the only way to fix it quickly is to temporary make CMI syncronous */
	fbe_semaphore_wait(&job_service_cmi_message->sem, NULL);
    return FBE_STATUS_OK;
}

/* Job service cmi lock accessors.
*  CAUTION:
*  NEVER put fbe_job_service_lock under cmi_lock's scope in case of DEAD LOCK!! 
*/
void fbe_job_service_cmi_lock(void)
{
    fbe_spinlock_lock(&fbe_job_service_cmi_spinlock);
    return;
}

void fbe_job_service_cmi_unlock(void)
{
    fbe_spinlock_unlock(&fbe_job_service_cmi_spinlock);
    return;
}

/* Forward declarations */
static fbe_status_t fbe_job_service_cmi_callback(fbe_cmi_event_t event, 
        fbe_u32_t cmi_message_length,
        fbe_cmi_message_t cmi_message,
        fbe_cmi_event_callback_context_t context);

static fbe_status_t fbe_job_service_cmi_message_transmitted(fbe_job_service_cmi_message_t *job_service_cmi_msg, 
        fbe_cmi_event_callback_context_t context);

static fbe_status_t fbe_job_service_cmi_peer_lost(void);

static fbe_status_t fbe_job_service_cmi_message_received(fbe_job_service_cmi_message_t * fbe_job_service_cmi_msg);

static fbe_status_t fbe_job_service_cmi_fatal_error_message(fbe_job_service_cmi_message_t *fbe_job_service_cmi_msg, 
        fbe_cmi_event_callback_context_t context);

static fbe_status_t  fbe_job_service_cmi_no_peer_message(fbe_job_service_cmi_message_t *fbe_job_service_cmi_msg, 
        fbe_cmi_event_callback_context_t context);

static void fbe_job_service_cmi_init_message(fbe_job_service_cmi_message_t *fbe_job_service_cmi_msg);

static void fbe_job_service_cmi_process_sync_job_state_request(fbe_job_queue_element_t *job_element_p);

static void fbe_job_service_cmi_init_remove_job_action_on_block_in_progress(void);

/*!****************************************************
 * fbe_job_service_cmi_init()
 ******************************************************
 * @brief
 * This function inits the contents of a block in
 * progress and a block from other side as well
 * as registers the job service with CMI
 *
 * @param - none
 *
 * @return - none
 *
 * @author
 * 11/22/2010 - Created. Jesus Medina
 *
 ******************************************************/
fbe_status_t fbe_job_service_cmi_init(void)
{
    fbe_status_t status; 
    fbe_spinlock_init(&fbe_job_service_cmi_spinlock);
    block_in_progress.job_number = 0;
    block_in_progress.queue_type = FBE_JOB_INVALID_QUEUE;
    block_in_progress.block_from_other_side = NULL;
    block_in_progress.job_service_cmi_state = FBE_JOB_SERVICE_CMI_STATE_IDLE;
    fbe_job_service_cmi_init_remove_job_action_on_block_in_progress();
    fbe_job_service_cmi_init_block_from_other_side();
    status = fbe_cmi_register(FBE_CMI_CLIENT_ID_JOB_SERVICE, fbe_job_service_cmi_callback, NULL);
    return status;
}
/********************************
 * end fbe_job_service_cmi_init()
 *******************************/

/*!****************************************************
 * fbe_job_service_cmi_destroy()
 ******************************************************
 * @brief
 * This function unregister job service
 * with CMI service
 *
 * @param - none
 *
 * @return - none
 *
 * @author
 * 11/22/2010 - Created. Jesus Medina
 *
 ******************************************************/
fbe_status_t fbe_job_service_cmi_destroy(void)
{
    fbe_status_t status;
    status = fbe_cmi_unregister(FBE_CMI_CLIENT_ID_JOB_SERVICE);
	fbe_spinlock_destroy(&fbe_job_service_cmi_spinlock);

    return status;
}
/***********************************
 * end fbe_job_service_cmi_destroy()
 **********************************/

/*!****************************************************
 * fbe_job_service_cmi_callback()
 ******************************************************
 * @brief
 * This function is the main cmi entry for call
 * returns coming from CMI to the job service
 *
 * @param - event : which of the 5 events the function was
 * called for
 * @param - cmi_message_length : cmi message
 * @param - cmi_message : cmi message lenght
 * @param - context : context of callback, the job service
 * context in this case
 *
 * @return - none
 *
 * @author
 * 11/22/2010 - Created. Jesus Medina
 *
 ******************************************************/
static fbe_status_t fbe_job_service_cmi_callback(fbe_cmi_event_t event,
        fbe_u32_t cmi_message_length, 
        fbe_cmi_message_t cmi_message,  
        fbe_cmi_event_callback_context_t context)
{
    fbe_job_service_cmi_message_t * fbe_job_service_cmi_msg = NULL;
    fbe_status_t status = FBE_STATUS_OK;

    fbe_job_service_cmi_msg = (fbe_job_service_cmi_message_t*)cmi_message;

    switch (event) {
        case FBE_CMI_EVENT_MESSAGE_TRANSMITTED:
            status = fbe_job_service_cmi_message_transmitted(fbe_job_service_cmi_msg, context);
            break;
        case FBE_CMI_EVENT_SP_CONTACT_LOST:
            status = fbe_job_service_cmi_peer_lost();
            break;
        case FBE_CMI_EVENT_MESSAGE_RECEIVED:
            status = fbe_job_service_cmi_message_received(fbe_job_service_cmi_msg);
            break;
        case FBE_CMI_EVENT_FATAL_ERROR:
            status = fbe_job_service_cmi_fatal_error_message(fbe_job_service_cmi_msg, context);
            break;
		case FBE_CMI_EVENT_PEER_NOT_PRESENT:
		case FBE_CMI_EVENT_PEER_BUSY:
            status = fbe_job_service_cmi_no_peer_message(fbe_job_service_cmi_msg, context);
            break;
        default:
            break;
    }

    return status;
}
/************************************
 * end fbe_job_service_cmi_callback()
 ***********************************/

/*!****************************************************
 * fbe_job_service_cmi_message_transmitted()
 ******************************************************
 * @brief
 * This function sends job service block elements
 * to peer SP
 *
 * @param - fbe_job_service_cmi_msg : block to send to
 * peer side
 * @param - context : context of transmitted job service
 * message
 *
 * @return - none
 *
 * @author
 * 11/22/2010 - Created. Jesus Medina
 *
 ******************************************************/
static fbe_status_t fbe_job_service_cmi_message_transmitted(fbe_job_service_cmi_message_t *fbe_job_service_cmi_msg, 
        fbe_cmi_event_callback_context_t context)
{
    const char *p_job_message_type_str = NULL;

    job_service_trace(FBE_TRACE_LEVEL_INFO,
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry\n", __FUNCTION__);

    fbe_job_service_cmi_translate_message_type(fbe_job_service_cmi_msg->job_service_cmi_message_type, 
            &p_job_message_type_str);

    job_service_trace(FBE_TRACE_LEVEL_INFO,
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "message_transmitted %d(%s)\n", 
            fbe_job_service_cmi_msg->job_service_cmi_message_type,
            p_job_message_type_str);

    /* Free the memory we used to send this message.*/
    fbe_job_service_cmi_init_message(fbe_job_service_cmi_msg);
    //fbe_memory_native_release(fbe_job_service_cmi_msg);

	/* It is very bad, but we have design problem, the only way to fix it quickly is to temporary make CMI syncronous */
	fbe_semaphore_release(&fbe_job_service_cmi_msg->sem, 0, 1 ,FALSE);

    return FBE_STATUS_OK; 
}
/***********************************************
 * end fbe_job_service_cmi_message_transmitted()
 **********************************************/

/*!****************************************************
 * fbe_job_service_cmi_peer_lost()
 ******************************************************
 * @brief
 * This function is called when the peer goes
 * away
 *
 * @param - none
 *
 * @return - none
 *
 * @author
 * 11/22/2010 - Created. Jesus Medina
 *
 ******************************************************/
static fbe_status_t fbe_job_service_cmi_peer_lost(void)
{   
    fbe_atomic_t js_gate = 0;

    job_service_trace(FBE_TRACE_LEVEL_INFO,
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry\n", __FUNCTION__);

    /* if we have any outstanding commumication with peer, let's clean it up. */
    if(block_in_progress.job_service_cmi_state != FBE_JOB_SERVICE_CMI_STATE_IDLE){

        /*! Check if service is shutting down */
        js_gate = fbe_atomic_increment(&fbe_job_service.arrival.outstanding_requests);
        if(js_gate & FBE_JOB_SERVICE_GATE_BIT){ 
            /* We have a gate, so fail the request */
            job_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO, 
                              "%s: Job Service is shutting down\n", __FUNCTION__);
    
            fbe_atomic_decrement(&fbe_job_service.arrival.outstanding_requests);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        
        if(fbe_job_service_arrival_thread_is_flag_set(FBE_JOB_SERVICE_ARRIVAL_THREAD_FLAGS_SYNC) &&
            fbe_job_service_thread_is_flag_set(FBE_JOB_SERVICE_THREAD_FLAGS_SYNC)) {
            job_service_trace(FBE_TRACE_LEVEL_INFO, 
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: Has block in progress.  Cleanup sync job\n",
                              __FUNCTION__);

            fbe_job_service_cmi_invalidate_block_from_other_side();
            /* Set the thread flag to RUN mode.. */
            fbe_job_service_arrival_thread_set_flag(FBE_JOB_SERVICE_ARRIVAL_THREAD_FLAGS_RUN); 
            fbe_job_service_thread_set_flag(FBE_JOB_SERVICE_THREAD_FLAGS_RUN); 
            /* Enable the queue access */
            fbe_job_service_handle_queue_access(FBE_TRUE);
        }
        else {
            job_service_trace(FBE_TRACE_LEVEL_INFO, 
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: Has block in progress.  Cleanup job.\n",
                              __FUNCTION__);
            /* Set the thread flag to RUN mode.. */
            fbe_job_service_arrival_thread_set_flag(FBE_JOB_SERVICE_ARRIVAL_THREAD_FLAGS_RUN); 
            fbe_job_service_thread_set_flag(FBE_JOB_SERVICE_THREAD_FLAGS_RUN); 
            /* treat it as complete from the other side */ 
            fbe_job_service_cmi_lock();
            block_in_progress.job_service_cmi_state = FBE_JOB_SERVICE_CMI_STATE_COMPLETED;
            fbe_job_service_cmi_unlock();
        }
        /* Release the semaphore.. */
        fbe_semaphore_release(&fbe_job_service.arrival.job_service_arrival_queue_semaphore, 0, 1, FALSE); 
        fbe_atomic_decrement(&fbe_job_service.arrival.outstanding_requests); 
    }
    /* resume the job 
     * After intrducing Maintenace mode, the current SP may not be active after peer lost.
     * The job only be waked up in Acitve SP.
     */
    if (fbe_job_service_cmi_is_active()) {
		fbe_job_service.peer_just_died = FBE_TRUE;
        /* 
          The sp state here is used to prevent a corner case that job sneaks in 
          before the DB set its state as NOT ready.If peer goes and the CMI 
          callback function runs to here, it means that the database service CMI 
          callback function has been called and the DB service state has been set
          as NOT ready. At this time, it should be marked as ACTIVE to make job 
          continue.
        */
        fbe_job_service.sp_state_for_peer_lost_handle = FBE_CMI_STATE_ACTIVE;
        fbe_semaphore_release(&fbe_job_service.job_control_queue_semaphore, 0, 1, FALSE);
    }

    return FBE_STATUS_OK; 
}
/*************************************
 * end fbe_job_service_cmi_peer_lost()
 ************************************/

/*!****************************************************
 * fbe_job_service_cmi_message_received()
 ******************************************************
 * @brief
 * This function handles message receives from peer SP
 *
 * @param - fbe_job_service_cmi_msg: job service message
 *
 * @return - status of call 
 *
 * @author
 * 11/22/2010 - Created. Jesus Medina
 *
 ******************************************************/
static fbe_status_t fbe_job_service_cmi_message_received(fbe_job_service_cmi_message_t *fbe_job_service_cmi_msg)
{
    const char *p_job_message_type_str = NULL;
    fbe_status_t status = FBE_STATUS_OK;

    job_service_trace(FBE_TRACE_LEVEL_INFO, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry\n", 
            __FUNCTION__);

    fbe_job_service_cmi_translate_message_type(fbe_job_service_cmi_msg->job_service_cmi_message_type, 
            &p_job_message_type_str);

    job_service_trace(FBE_TRACE_LEVEL_INFO,
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "message received type %d(%s)\n", fbe_job_service_cmi_msg->job_service_cmi_message_type,
            p_job_message_type_str);

    /* Check receiver address */
    switch(fbe_job_service_cmi_msg->job_service_cmi_message_type)
    {
        case FBE_JOB_CONTROL_SERVICE_CMI_MESSAGE_TYPE_ADD_TO_CREATION_QUEUE:
            //fbe_job_service_cmi_set_queue_type_on_block_in_progress(fbe_job_service_cmi_msg->queue_type);
            fbe_job_service_cmi_copy_job_queue_element_to_block_from_other_side(
                    &fbe_job_service_cmi_msg->msg.job_service_queue_element.job_element);
            break;

        case FBE_JOB_CONTROL_SERVICE_CMI_MESSAGE_TYPE_ADDED_TO_CREATION_QUEUE:
            job_service_trace(FBE_TRACE_LEVEL_INFO, 
                    FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                    "%s, job number %llu, added to creation queue on peer side\n",
                    __FUNCTION__,
		    (unsigned long long)fbe_job_service_cmi_msg->job_number);

            if ((fbe_job_service_arrival_thread_is_flag_set(FBE_JOB_SERVICE_ARRIVAL_THREAD_FLAGS_SYNC)) &&
                (fbe_job_service_cmi_is_active()))
            {
                /* Increment the tracking variables once we got the ack from passive side.. */
                fbe_job_service_arrival_thread_increment_sync_element();
            }

            fbe_job_service_cmi_complete_job_from_other_side(fbe_job_service_cmi_msg->job_number);
            break;

        case FBE_JOB_CONTROL_SERVICE_CMI_MESSAGE_TYPE_REMOVE_FROM_CREATION_QUEUE:
            job_service_trace(FBE_TRACE_LEVEL_INFO, 
                    FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                    "%s, received request to remove job from creation queue\n",
                    __FUNCTION__);

            if (fbe_job_service_cmi_msg->error_code != FBE_JOB_SERVICE_ERROR_NO_ERROR)
            {
                job_service_trace(FBE_TRACE_LEVEL_INFO, 
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "   job number: 0x%llx obj: 0x%x status: 0x%x error_code: %d\n",
                        fbe_job_service_cmi_msg->job_number,
                        fbe_job_service_cmi_msg->object_id,
                        fbe_job_service_cmi_msg->status,
                        fbe_job_service_cmi_msg->error_code);
            }
            else
            {
                job_service_trace(FBE_TRACE_LEVEL_INFO, 
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "   job number: 0x%llx obj: 0x%x status: 0x%x job service no error on peer\n",
                        (unsigned long long)fbe_job_service_cmi_msg->job_number,
                        fbe_job_service_cmi_msg->object_id,
                        fbe_job_service_cmi_msg->status);
            }

            status =  fbe_job_service_cmi_process_remove_request_for_creation_queue(fbe_job_service_cmi_msg->status,
                                                                                    fbe_job_service_cmi_msg->job_number,
                                                                                    fbe_job_service_cmi_msg->object_id,
                                                                                    fbe_job_service_cmi_msg->error_code,
                                                                                    fbe_job_service_cmi_msg->class_id,
                                                                                    &fbe_job_service_cmi_msg->msg.job_service_queue_element.job_element);

            /* clean up cmi message */
            fbe_job_service_cmi_init_message(fbe_job_service_cmi_msg);

            break;

        case FBE_JOB_CONTROL_SERVICE_CMI_MESSAGE_TYPE_REMOVED_FROM_CREATION_QUEUE:
            break;

        case FBE_JOB_CONTROL_SERVICE_CMI_MESSAGE_TYPE_ADD_TO_RECOVERY_QUEUE:
            //fbe_job_service_cmi_set_queue_type_on_block_in_progress(fbe_job_service_cmi_msg->queue_type);
            fbe_job_service_cmi_copy_job_queue_element_to_block_from_other_side(
                    &fbe_job_service_cmi_msg->msg.job_service_queue_element.job_element);
            break;

        case FBE_JOB_CONTROL_SERVICE_CMI_MESSAGE_TYPE_ADDED_TO_RECOVERY_QUEUE:
            job_service_trace(FBE_TRACE_LEVEL_INFO, 
                    FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                    "%s, job number 0x%llx error_code: %d add to recovery queue\n",
                    __FUNCTION__, (unsigned long long)fbe_job_service_cmi_msg->job_number, fbe_job_service_cmi_msg->error_code);

            fbe_job_service_cmi_complete_job_from_other_side(fbe_job_service_cmi_msg->job_number);
            break;

        case FBE_JOB_CONTROL_SERVICE_CMI_MESSAGE_TYPE_REMOVE_FROM_RECOVERY_QUEUE:
            job_service_trace(FBE_TRACE_LEVEL_INFO, 
                    FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                    "%s job number 0x%llx error_code: %d remove from recovery queue\n",
                    __FUNCTION__, (unsigned long long)fbe_job_service_cmi_msg->job_number, fbe_job_service_cmi_msg->error_code); 

            status = fbe_job_service_cmi_process_remove_request_for_recovery_queue(fbe_job_service_cmi_msg->status,
                                                                                   fbe_job_service_cmi_msg->job_number,
                                                                                   fbe_job_service_cmi_msg->object_id,
                                                                                   fbe_job_service_cmi_msg->error_code,
                                                                                   fbe_job_service_cmi_msg->class_id,
                                                                                   &fbe_job_service_cmi_msg->msg.job_service_queue_element.job_element);

            /* clean up cmi message */
            fbe_job_service_cmi_init_message(fbe_job_service_cmi_msg);

            break;

        case FBE_JOB_CONTROL_SERVICE_CMI_MESSAGE_TYPE_REMOVED_FROM_RECOVERY_QUEUE:
            break;

            /* This runs in ACTIVE context. */
        case FBE_JOB_CONTROL_SERVICE_CMI_MESSAGE_TYPE_UPDATE_PEER_JOB_QUEUE:
            fbe_job_service_cmi_update_peer_job_queue(&fbe_job_service_cmi_msg->msg.job_service_queue_element.job_element);
            break;
    
            /* This runs in PASSIVE context. */
        case FBE_JOB_CONTROL_SERVICE_CMI_MESSAGE_TYPE_UPDATE_JOB_QUEUE_DONE:
            fbe_job_service_cmi_update_job_queue_completion();
            break;

        case FBE_JOB_CONTROL_SERVICE_CMI_MESSAGE_TYPE_SYNC_JOB_STATE:
            fbe_job_service_cmi_process_sync_job_state_request(&fbe_job_service_cmi_msg->msg.job_service_queue_element.job_element);
            fbe_job_service_cmi_init_message(fbe_job_service_cmi_msg);
            break;
    
        default:
            job_service_trace(FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                              "%s Uknown message type %d \n", __FUNCTION__, 
                              fbe_job_service_cmi_msg->job_service_cmi_message_type);
            break;
    }

    return FBE_STATUS_OK;
}
/********************************************
 * end fbe_job_service_cmi_message_received()
 *******************************************/

/*!****************************************************
 * fbe_job_service_cmi_fatal_error_message()
 ******************************************************
 * @brief
 * This function handles a fatal error message
 *
 * @param - fbe_job_service_cmi_msg: job service message
 * @param - fbe_cmi_event_callback_context_t: cmi callback
 * context 
 *
 * @return - status of call 
 *
 * @author
 * 11/22/2010 - Created. Jesus Medina
 *
 ******************************************************/
static fbe_status_t fbe_job_service_cmi_fatal_error_message(fbe_job_service_cmi_message_t *fbe_job_service_cmi_msg, 
                                                            fbe_cmi_event_callback_context_t context)
{
    /*process a message that say not only we could not send the message but the whole CMI bus is bad*/
    fbe_packet_t * packet = NULL;

    /* Not all job service CMI messages are sent with a context. 
     * Eg. fbe_job_service_cmi_get_job_elements_from_peer() is not sending
     * any context. 
     */
    if(context != NULL)
    {
        packet = (fbe_packet_t *)context;
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
    }

    return FBE_STATUS_OK;
}
/***********************************************
 * end fbe_job_service_cmi_fatal_error_message()
 **********************************************/


/*!****************************************************
 * fbe_job_service_cmi_no_peer_message()
 ******************************************************
 * @brief
 * Receives the no peer message callback
 *
 * @param fbe_job_service_cmi_msg        
 * @param context        
 *
 * @return - none
 *
 * @author
 * 09/22/2010 - Created. Jesus Medina
 *
 ******************************************************/
static fbe_status_t fbe_job_service_cmi_no_peer_message(fbe_job_service_cmi_message_t *fbe_job_service_cmi_msg,
        fbe_cmi_event_callback_context_t context)
{
    const char *p_job_message_type_str = NULL;

    job_service_trace(FBE_TRACE_LEVEL_INFO,
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "job_cmi_no_peer_msg entry\n");

    fbe_job_service_cmi_translate_message_type(fbe_job_service_cmi_msg->job_service_cmi_message_type, 
            &p_job_message_type_str);

    job_service_trace(FBE_TRACE_LEVEL_INFO,
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "message type %s\n", p_job_message_type_str);

    job_service_trace(FBE_TRACE_LEVEL_INFO,
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "job_cmi_no_peer_msg, no peer, job number = %llu\n",
	    (unsigned long long)fbe_job_service_cmi_msg->job_number);

    switch(fbe_job_service_cmi_msg->job_service_cmi_message_type)
    {
        case FBE_JOB_CONTROL_SERVICE_CMI_MESSAGE_TYPE_ADD_TO_CREATION_QUEUE:

            /* if passive side, job number has not been generated by active side, must generate one here */
            if (!fbe_job_service_cmi_is_active())
            {
                fbe_job_service_cmi_msg->job_number = fbe_job_service_increment_job_number_with_lock();
            }
            job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                    "job_cmi_no_peer_msg job_number_no peer= %llu\n",
		    (unsigned long long)fbe_job_service_cmi_msg->job_number);
            fbe_job_service_cmi_complete_job_from_other_side(fbe_job_service_cmi_msg->job_number);         

            fbe_job_service_cmi_init_message(fbe_job_service_cmi_msg);
            break;

        case FBE_JOB_CONTROL_SERVICE_CMI_MESSAGE_TYPE_ADDED_TO_CREATION_QUEUE:
            fbe_job_service_cmi_init_message(fbe_job_service_cmi_msg);
            break;

        case FBE_JOB_CONTROL_SERVICE_CMI_MESSAGE_TYPE_REMOVE_FROM_CREATION_QUEUE:
        case FBE_JOB_CONTROL_SERVICE_CMI_MESSAGE_TYPE_REMOVED_FROM_CREATION_QUEUE:
            fbe_job_service_cmi_init_message(fbe_job_service_cmi_msg);
            break;

        case FBE_JOB_CONTROL_SERVICE_CMI_MESSAGE_TYPE_ADD_TO_RECOVERY_QUEUE:

            /* if passive side, job number has not been generated by active side, must generate one here */
            if (!fbe_job_service_cmi_is_active())
            {
                fbe_job_service_cmi_msg->job_number = fbe_job_service_increment_job_number_with_lock();
            }
            job_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                    "job_cmi_no_peer_msg, no peer, job_number = %llu\n",
		    (unsigned long long)fbe_job_service_cmi_msg->job_number);
            fbe_job_service_cmi_complete_job_from_other_side(fbe_job_service_cmi_msg->job_number);
            fbe_job_service_cmi_init_message(fbe_job_service_cmi_msg);
            break;

        case FBE_JOB_CONTROL_SERVICE_CMI_MESSAGE_TYPE_ADDED_TO_RECOVERY_QUEUE:
            fbe_job_service_cmi_init_message(fbe_job_service_cmi_msg);
            break;

        case FBE_JOB_CONTROL_SERVICE_CMI_MESSAGE_TYPE_REMOVE_FROM_RECOVERY_QUEUE:
        case FBE_JOB_CONTROL_SERVICE_CMI_MESSAGE_TYPE_REMOVED_FROM_RECOVERY_QUEUE:
            fbe_job_service_cmi_init_message(fbe_job_service_cmi_msg);
            break;

        case FBE_JOB_CONTROL_SERVICE_CMI_MESSAGE_TYPE_SYNC_JOB_STATE:
            fbe_job_service_cmi_init_message(fbe_job_service_cmi_msg);
            break;

        default:
            job_service_trace(FBE_TRACE_LEVEL_ERROR,
                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                    "job_cmi_no_peer_msg Uknown message type %d \n" ,
                    fbe_job_service_cmi_msg->job_service_cmi_message_type);
            break;
    }

	/* It is very bad, but we have design problem, the only way to fix it quickly is to temporary make CMI syncronous */
	fbe_semaphore_release(&fbe_job_service_cmi_msg->sem, 0, 1 ,FALSE);

    return FBE_STATUS_OK;
}
/*******************************************
 * end fbe_job_service_cmi_no_peer_message()
 ******************************************/

/*!****************************************************
 * fbe_job_service_cmi_init_block_from_other_side()
 ******************************************************
 * @brief
 * Inits the contents in the block from other side
 *
 * @param         
 *
 * @return - none
 *
 * @author
 * 09/20/2010 - Created. Jesus Medina
 *
 ****************************************************************/
void fbe_job_service_cmi_init_block_from_other_side(void)
{
    block_from_other_side.queue_type = FBE_JOB_INVALID_QUEUE;
    block_from_other_side.job_type = FBE_JOB_TYPE_INVALID;
    block_from_other_side.object_id = FBE_OBJECT_ID_INVALID;
    //block_from_other_side.job_action = NULL;
    block_from_other_side.timestamp = 0;
    block_from_other_side.current_state = FBE_JOB_ACTION_STATE_INVALID;
    block_from_other_side.previous_state = FBE_JOB_ACTION_STATE_INVALID;
    block_from_other_side.queue_access = FBE_FALSE;
    block_from_other_side.num_objects = 0;
    block_from_other_side.packet_p = NULL;
    block_from_other_side.need_to_wait = FBE_FALSE;
    block_from_other_side.timeout_msec = 60000;
    block_from_other_side.additional_job_data = NULL;
    block_from_other_side.local_job_data_valid = FBE_FALSE;
}
/*****************************************************
 * end fbe_job_service_cmi_init_block_from_other_side()
 *****************************************************/

/*!****************************************************
 * fbe_job_service_cmi_copy_job_queue_element_to_block_from_other_side()
 ******************************************************
 * @brief
 * copy incoming_queue element to block_from_other_side
 *
 * @param         
 *
 * @return - none
 *
 * @author
 * 09/17/2010 - Created. Jesus Medina
 *
 ****************************************************************/

void fbe_job_service_cmi_copy_job_queue_element_to_block_from_other_side(fbe_job_queue_element_t *queue_element)
{
    const char *p_job_type_str = NULL;
    fbe_atomic_t js_gate = 0;

    job_service_trace(FBE_TRACE_LEVEL_INFO, 
                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, 
                      "%s entry\n", 
                      __FUNCTION__);

    fbe_job_service_translate_job_type(queue_element->job_type, &p_job_type_str);

    job_service_trace(FBE_TRACE_LEVEL_INFO, 
                      FBE_TRACE_MESSAGE_ID_INFO, 
                      "Job number: %llu, Job Type: %s\n", 
                      (unsigned long long)queue_element->job_number,
		      p_job_type_str);

    /*! Check if service is shutting down */
    js_gate = fbe_atomic_increment(&fbe_job_service.arrival.outstanding_requests);
    if(js_gate & FBE_JOB_SERVICE_GATE_BIT){ 
        /* We have a gate, so fail the request */
        job_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO, 
                          "%s: Job Service is shutting down\n", __FUNCTION__);

        fbe_atomic_decrement(&fbe_job_service.arrival.outstanding_requests);
        return;
    }

    fbe_job_service_cmi_lock();

    /* todo@: for some reason the below line does not copy all it is
     * required to copy 
     * */
    //fbe_copy_memory(&block_from_other_side, &queue_element, sizeof(fbe_job_queue_element_t));
    block_from_other_side.queue_type = queue_element->queue_type;
    block_from_other_side.job_type = queue_element->job_type;
    block_from_other_side.timestamp = queue_element->timestamp;
    block_from_other_side.current_state = queue_element->current_state;
    block_from_other_side.previous_state = queue_element->previous_state;
    block_from_other_side.queue_access = queue_element->queue_access;
    block_from_other_side.num_objects = queue_element->num_objects;
    block_from_other_side.job_number = queue_element->job_number;
    block_from_other_side.job_action = queue_element->job_action;
    block_from_other_side.packet_p = queue_element->packet_p;
    block_from_other_side.command_size = queue_element->command_size;
    block_from_other_side.need_to_wait = queue_element->need_to_wait;
    block_from_other_side.timeout_msec = queue_element->timeout_msec;
	block_from_other_side.object_id = queue_element->object_id;

    fbe_copy_memory(&block_from_other_side.command_data, &queue_element->command_data, FBE_JOB_ELEMENT_CONTEXT_SIZE);

    fbe_job_service_cmi_unlock();

    fbe_semaphore_release(&fbe_job_service.arrival.job_service_arrival_queue_semaphore, 0, 1, FALSE);
    fbe_atomic_decrement(&fbe_job_service.arrival.outstanding_requests);
}

/**************************************************************************
 * end fbe_job_service_cmi_copy_job_queue_element_to_block_from_other_side()
 **************************************************************************/

/*!**************************************************************
 * job_service_wait_for_expected_lifecycle_state()
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
fbe_status_t job_service_wait_for_expected_lifecycle_state(fbe_object_id_t obj_id, fbe_lifecycle_state_t expected_lifecycle_state, fbe_u32_t timeout_ms)
{
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_lifecycle_state_t current_state = FBE_LIFECYCLE_STATE_NOT_EXIST;
    fbe_u32_t           current_time = 0;

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
               return FBE_STATUS_OK;
           }
            current_time = current_time + 100; 
            fbe_thread_delay(100);
    }
    job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                      "%s: object 0x%x expected state %d != current state %d in %d ms!\n", 
                      __FUNCTION__, obj_id, expected_lifecycle_state, current_state, timeout_ms);

    return FBE_STATUS_GENERIC_FAILURE;
}

/*!************************************************************
 * fbe_job_service_cmi_process_remove_request_for_creation_queue()
 **************************************************************
 * @brief
 * This funcstion removes a queue element from the creation queue
 *
 * @param   in_status - Status of peer job request
 * @param   job_number - use to index into queue and determine if queue element exists
 * @param   object_id - object id associated with job (vd, pvd etc)
 * @param   error_code - The error code associated with the job
 * @param   class_id - Class if of associated object
 *
 * @return - status of the remove operation
 *
 * @author
 * 10/21/2010 - Created. Jesus Medina
 * 11/15/2012 - Modified by Vera Wang
 *
 ****************************************************************/
fbe_status_t fbe_job_service_cmi_process_remove_request_for_creation_queue(fbe_status_t in_status,     
                                                                           fbe_u64_t job_number, 
                                                                           fbe_object_id_t object_id,
                                                                           fbe_job_service_error_type_t error_code,
                                                                           fbe_class_id_t class_id,
                                                                           fbe_job_queue_element_t *job_element_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_notification_info_t notification_info;
    
    status = fbe_job_service_find_and_remove_element_by_job_number_from_creation_queue(in_status, job_number, object_id, error_code, class_id, job_element_p);
    if (status == FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_INFO,
                FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "JOB: cmi_process_remove_request_for_creation_queue: status %d\n", 
                status);

		/*by now, the active side made sure the peer state is good
		of course it can change on us by the time we get here but this is true if we wait here as well. By the time we send the notification,
		a READY object might turn to bad*/
        notification_info.notification_type = FBE_NOTIFICATION_TYPE_JOB_ACTION_STATE_CHANGED;
        notification_info.notification_data.job_service_error_info.object_id = object_id;
        notification_info.notification_data.job_service_error_info.status = in_status;
        notification_info.notification_data.job_service_error_info.error_code = error_code;
        notification_info.notification_data.job_service_error_info.job_number = job_number;
        notification_info.notification_data.job_service_error_info.job_type = job_element_p->job_type; 
        
        notification_info.class_id = FBE_CLASS_ID_INVALID;
        notification_info.object_type = FBE_TOPOLOGY_OBJECT_TYPE_ALL;

        /* trace the notification. */
        job_service_trace(FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "JOB: Peer send notification. job: 0x%llx error: 0x%x\n",
                          job_number, error_code);

        /* trace the notification. */
        job_service_trace(FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "JOB: Peer send notification. obj: 0x%x object_type: 0x%llx type: 0x%llx\n",
                          object_id, (unsigned long long)FBE_TOPOLOGY_OBJECT_TYPE_ALL, (unsigned long long)notification_info.notification_type);

        // Send notification for the job number
        status = fbe_notification_send(object_id, notification_info);
        if (status != FBE_STATUS_OK) 
        {
            job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s: Send Job Notification failed, status: 0x%X\n",
                          __FUNCTION__, status);
            return status;
        }

    }

    /* Log the trace for delay remove */
    if(status == FBE_STATUS_MORE_PROCESSING_REQUIRED)
    {
        job_service_trace(FBE_TRACE_LEVEL_INFO, 
                FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "%s: status 0x%x, job %llu will be removed in arrival thread.\n", 
                __FUNCTION__, status, (unsigned long long)job_number);
    }

    if(status == FBE_STATUS_NO_OBJECT)
    {
        job_service_trace(FBE_TRACE_LEVEL_WARNING,
                FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "fbe_job_service_cmi_process_remove_request_for_creation_queue, status %d, job %llu not found\n", 
                status, (unsigned long long)job_number);
    }
    return status;
}
/***************************************************************
 * end fbe_job_service_cmi_process_remove_request_for_creation_queue()
 **************************************************************/

/*!************************************************************
 * fbe_job_service_cmi_process_remove_request_for_recovery_queue()
 **************************************************************
 * @brief
 * This funcstion removes a queue element from the creation queue
 *
 * @param   in_status - Status of peer job request
 * @param   job_number - use to index into queue and determine if queue element exists
 * @param   object_id - object id associated with job (vd, pvd etc)
 * @param   error_code - The error code associated with the job
 * @param   class_id - Class id of object
 * @param   job_element_p - Pointer to job element information from other side
 *
 * @return - status of the remove operation
 *
 * @author
 * 11/21/2010 - Created. Jesus Medina
 *
 ****************************************************************/
fbe_status_t fbe_job_service_cmi_process_remove_request_for_recovery_queue(fbe_status_t in_status,     
                                                                           fbe_u64_t job_number, 
                                                                           fbe_object_id_t object_id,  
                                                                           fbe_job_service_error_type_t error_code,
                                                                           fbe_class_id_t class_id,
                                                                           fbe_job_queue_element_t *job_element_p)
{
    fbe_status_t    status = FBE_STATUS_OK;

    /* Process the peer job completion.
     */
    status = fbe_job_service_process_peer_recovery_queue_completion(in_status, job_number, object_id, error_code, class_id, job_element_p);
    if (status == FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_INFO,
                FBE_TRACE_MESSAGE_ID_INFO,
                "cmi remove job 0x%llx from recovery_queue, status %d\n", 
                (unsigned long long)job_number, status);
    }
    /* Log the trace for delay remove */
    if(status == FBE_STATUS_MORE_PROCESSING_REQUIRED)
    {
        job_service_trace(FBE_TRACE_LEVEL_INFO, 
                FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "%s: status 0x%x, job %llu will be removed in arrival thread.\n", 
                __FUNCTION__, status, (unsigned long long)job_number);
    }
    if(status == FBE_STATUS_NO_OBJECT)
    {
        job_service_trace(FBE_TRACE_LEVEL_WARNING,
                FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "fbe_job_service_cmi_process_remove_request_for_recovery_queue, status %d, job %llu not found\n", 
                status, (unsigned long long)job_number);
    }
    return status;
}
/*********************************************************************
 * end fbe_job_service_cmi_process_remove_request_for_recovery_queue()
 *********************************************************************/

/*!*******************************************************
 * fbe_job_service_cmi_print_contents_from_queue_element()
 *********************************************************
 * @brief
 * prints contents of queue element
 *
 * @param - queue_element : queue element whose content
 * this fucntion will print        
 *
 * @return - none
 *
 * @author
 * 10/22/2010 - Created. Jesus Medina
 *
 ****************************************************************/

void fbe_job_service_cmi_print_contents_from_queue_element(fbe_job_queue_element_t * queue_element)
{
    const char *p_job_type_str = NULL;

    job_service_trace(FBE_TRACE_LEVEL_INFO, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "Job service queue element content: \n");

    fbe_job_service_translate_job_type(queue_element->job_type, &p_job_type_str);

    job_service_trace(FBE_TRACE_LEVEL_INFO, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "  job_type %s\n",	
            p_job_type_str);

    job_service_trace(FBE_TRACE_LEVEL_INFO, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "  time stamp %llu, queue access %d\n",
            (unsigned long long)queue_element->timestamp,
            queue_element->queue_access);

    job_service_trace(FBE_TRACE_LEVEL_INFO, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "  current_state 0x%x, previous_state 0x%x\n", 
            queue_element->current_state,
            queue_element->previous_state);

    job_service_trace(FBE_TRACE_LEVEL_INFO, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "  num_objects %d, job_number %llu\n", 
            queue_element->num_objects,
            (unsigned long long)queue_element->job_number);
}
/*************************************************************
 * end fbe_job_service_cmi_print_contents_from_queue_element()
 *************************************************************/

/*!************************************************************
 * fbe_job_service_cmi_set_queue_type_on_block_in_progress()
 **************************************************************
 * @brief
 * sets queue type on block from other side
 *
 * @param - queue_type, value use to set queue type in block
 *           from other side
 *
 * @return - none
 *
 * @author
 * 09/17/2010 - Created. Jesus Medina
 *
 ****************************************************************/

void fbe_job_service_cmi_set_queue_type_on_block_in_progress(fbe_job_control_queue_type_t queue_type)
{
    fbe_job_service_cmi_lock();
    block_in_progress.queue_type = queue_type;
    fbe_job_service_cmi_unlock();
}
/***************************************************************
 * end fbe_job_service_cmi_set_queue_type_on_block_in_progress()
 **************************************************************/

/*!****************************************************
 * fbe_job_service_cmi_complete_job_from_other_side()
 ******************************************************
 * @brief
 * sets the state to complete and calls the arrival
 * semaphore
 *
 * @param         
 *
 * @return - none
 *
 * @author
 * 09/17/2010 - Created. Jesus Medina
 *
 ****************************************************************/

void fbe_job_service_cmi_complete_job_from_other_side(fbe_u64_t job_number)
{
    fbe_atomic_t js_gate = 0;

    /*! Check if service is shutting down */
    js_gate = fbe_atomic_increment(&fbe_job_service.arrival.outstanding_requests);
    if(js_gate & FBE_JOB_SERVICE_GATE_BIT){ 
        /* We have a gate, so fail the request */
        job_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO, 
                          "%s: Job Service is shutting down\n", __FUNCTION__);

        fbe_atomic_decrement(&fbe_job_service.arrival.outstanding_requests);
        return;
    }

    fbe_job_service_cmi_lock();
    block_in_progress.job_service_cmi_state = FBE_JOB_SERVICE_CMI_STATE_COMPLETED;

    /* on completion(at arrival thread code), only passive side will get job number out of block in progress */
    block_in_progress.job_number = job_number;

    if(block_in_progress.block_from_other_side == NULL){ /* We need to understand why this happend */
        job_service_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, 
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s: Error block_from_other_side is NULL\n",
			  __FUNCTION__);
    }

    fbe_job_service_cmi_unlock();

    /* this will wake up the arrival thread so it knows a request has been added to the peer queue,
     * last thing is to complete the packet back to the client.
     * note that we are completing the request also if peer is not present as to simulate
     * a peer presence situation
     */
    fbe_semaphore_release(&fbe_job_service.arrival.job_service_arrival_queue_semaphore, 0, 1, FALSE);
    fbe_atomic_decrement(&fbe_job_service.arrival.outstanding_requests);
}
/**************************************************************************
 * end fbe_job_service_cmi_complete_job_from_other_side()
 **************************************************************************/

/*!**************************************************************
 * fbe_job_service_cmi_get_state()
 ****************************************************************
 * @brief
 * returns the block in progress state 
 *
 * @param - none        
 *
 * @return fbe_status_t - state of block from other side data structure
 *
 * @author
 * 09/15/2010 - Created. Jesus Medina
 *
 ****************************************************************/

fbe_job_service_cmi_state_t fbe_job_service_cmi_get_state(void)
{
    fbe_job_service_cmi_state_t state;

    fbe_job_service_cmi_lock();
    state = block_in_progress.job_service_cmi_state; 
    fbe_job_service_cmi_unlock();
/*
	job_service_trace(FBE_TRACE_LEVEL_DEBUG_LOW, 
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "state %d\n", state);
*/
    return state;
}
/*************************************************
 * end fbe_job_service_cmi_get_state()
 ************************************************/

/*!**************************************************************
 * fbe_job_service_cmi_get_state()
 ****************************************************************
 * @brief
 * returns the block in progress state 
 *
 * @param - none        
 *
 * @return fbe_status_t - state of block from other side data structure
 *
 * @author
 * 09/15/2010 - Created. Jesus Medina
 *
 ****************************************************************/

fbe_u64_t fbe_job_service_cmi_get_job_number_from_block_in_progress(void)
{
    fbe_u64_t job_number;

    fbe_job_service_cmi_lock();
    job_number = block_in_progress.job_number; 
    fbe_job_service_cmi_unlock();

    return job_number;
}
/*****************************************************************
 * end fbe_job_service_cmi_get_job_number_from_block_in_progress()
 ****************************************************************/

/*!**************************************************************
 * fbe_job_service_cmi_set_state_on_block_in_progress()
 ****************************************************************
 * @brief
 * sets the block's state to passed state 
 *
 * @param         
 *
 * @return fbe_status_t - status of call
 *
 * @author
 * 09/15/2010 - Created. Jesus Medina
 *
 ****************************************************************/

fbe_status_t fbe_job_service_cmi_set_state_on_block_in_progress(fbe_job_service_cmi_state_t state)
{
    fbe_job_service_cmi_lock();
    block_in_progress.job_service_cmi_state = state;
    fbe_job_service_cmi_unlock();

    return FBE_STATUS_OK;
}
/*************************************************
 * end fbe_job_service_cmi_set_state_on_block_in_progress()
 ************************************************/

/*!**************************************************************
 * fbe_job_service_cmi_get_packet_from_queue_element()
 ****************************************************************
 * @brief
 * returns the block in progress state 
 *
 * @param - none        
 *
 * @return fbe_status_t - state of block from other side data structure
 *
 * @author
 * 09/15/2010 - Created. Jesus Medina
 *
 ****************************************************************/

fbe_packet_t * fbe_job_service_cmi_get_packet_from_queue_element(void)
{
    fbe_packet_t *packet_p = NULL;

    fbe_job_service_cmi_lock();
	if(block_in_progress.block_from_other_side != NULL){
		packet_p = block_in_progress.block_from_other_side->packet_p; 
	} else {
		packet_p = NULL; /* We have to figure out why this happening */
	}
    fbe_job_service_cmi_unlock();
    return packet_p;
}
/*************************************************
 * end fbe_job_service_cmi_get_packet_from_queue_element()
 ************************************************/

/*!*****************************************************************
 * fbe_job_service_cmi_get_queue_element_from_block_in_progress()
 ********************************************************************
 * @brief
 * returns the queue element out of the block from the other side 
 *
 * @param - none        
 *
 * @return queue element - job control element
 *
 * @author
 * 09/22/2010 - Created. Jesus Medina
 *
 ****************************************************************/

fbe_job_queue_element_t * fbe_job_service_cmi_get_queue_element_from_block_in_progress(void)
{
    fbe_job_queue_element_t *job_queue_element = NULL;

    fbe_job_service_cmi_lock();
    job_queue_element = block_in_progress.block_from_other_side; 
    fbe_job_service_cmi_unlock();

    return job_queue_element;
}
/***************************************************************
 * end fbe_job_service_cmi_get_queue_element_from_block_in_progress()
 **************************************************************/

/*!****************************************************
 * fbe_job_service_cmi_is_block_from_other_side_valid()
 ******************************************************
 * @brief
 * sets the block's state to passed state 
 *
 * @param         
 *
 * @return fbe_status_t - status of call
 *
 * @author
 * 09/15/2010 - Created. Jesus Medina
 *
 ****************************************************************/

fbe_bool_t fbe_job_service_cmi_is_block_from_other_side_valid()
{
    fbe_bool_t is_valid = FBE_FALSE;

    fbe_job_service_cmi_lock();
    if (block_from_other_side.job_type != FBE_JOB_TYPE_INVALID)
    {
        is_valid = FBE_TRUE;
    }
    fbe_job_service_cmi_unlock();

    return is_valid;
}
/*********************************************************
 * end fbe_job_service_cmi_is_block_from_other_side_valid()
 *********************************************************/

/*!*****************************************************************
 * fbe_job_service_cmi_set_block_in_progress_block_from_other_side()
 *******************************************************************
 * @brief
 * This function updates block from other side to point to 
 * queue element 
 *
 * @param - queue_element : element to point       
 *
 * @return none 
 *
 * @author
 * 09/15/2010 - Created. Jesus Medina
 *
 ****************************************************************/

void fbe_job_service_cmi_set_block_in_progress_block_from_other_side(fbe_job_queue_element_t *queue_element)
{
    fbe_job_service_cmi_lock();
    block_in_progress.block_from_other_side = queue_element;
    fbe_job_service_cmi_unlock();
}
/***********************************************************************
 * end fbe_job_service_cmi_set_block_in_progress_block_from_other_side()
 **********************************************************************/

/*!*****************************************************************
 * fbe_job_service_cmi_invalidate_block_in_progress_block_from_other_side()
 *******************************************************************
 * @brief
 * resets the block_in_progress fields to null parameters
 *
 * @param         
 *
 * @return fbe_status_t - status of call
 *
 * @author
 * 09/15/2010 - Created. Jesus Medina
 *
 ****************************************************************/

void fbe_job_service_cmi_invalidate_block_in_progress_block_from_other_side()
{
    fbe_job_service_cmi_lock();

    block_in_progress.block_from_other_side = NULL;
    block_in_progress.job_number = 0;
    block_in_progress.queue_type = FBE_JOB_INVALID_QUEUE;
    block_in_progress.job_service_cmi_state = FBE_JOB_SERVICE_CMI_STATE_IDLE;
    fbe_job_service_cmi_init_remove_job_action_on_block_in_progress();

    fbe_job_service_cmi_unlock();
}
/*********************************************************
 * end fbe_job_service_cmi_invalidate_block_in_progress_block_from_other_side()
 *********************************************************/

/*!**************************************************************
 * fbe_job_service_cmi_copy_block_from_other_side()
 ****************************************************************
 * @brief
 * sets the block's state to passed state 
 *
 * @param         
 *
 * @return fbe_status_t - status of call
 *
 * @author
 * 09/15/2010 - Created. Jesus Medina
 * 05/07/2012 - Add set default value and copy by received size. Zhipeng Hu
 *
 ****************************************************************/
void fbe_job_service_cmi_copy_block_from_other_side(fbe_job_queue_element_t *job_queue_element)
{

    if(block_from_other_side.command_size > FBE_JOB_ELEMENT_CONTEXT_SIZE)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s : Received command size %d > size limit %d\n",
                          __FUNCTION__, block_from_other_side.command_size, FBE_JOB_ELEMENT_CONTEXT_SIZE);
        return;
    }

    fbe_job_service_cmi_lock();

    /* todo@: for some reason the below call does not fully work */    
    //fbe_copy_memory(*job_queue_element, &block_from_other_side, sizeof(fbe_job_queue_element_t));

    job_queue_element->queue_type = block_from_other_side.queue_type;
    job_queue_element->job_type = block_from_other_side.job_type;
    job_queue_element->timestamp = block_from_other_side.timestamp;
    job_queue_element->current_state = block_from_other_side.current_state;
    job_queue_element->previous_state = block_from_other_side.previous_state;
    job_queue_element->queue_access = block_from_other_side.queue_access;
    job_queue_element->num_objects = block_from_other_side.num_objects;
    job_queue_element->job_number = block_from_other_side.job_number;
    job_queue_element->job_action = block_from_other_side.job_action;
    job_queue_element->command_size = block_from_other_side.command_size;

    fbe_zero_memory(job_queue_element->command_data, FBE_JOB_ELEMENT_CONTEXT_SIZE);

    /*first set default values for this job command for the following case:
     *In sep ndu stage,  peer sends us an job where the command data is an older version (the
     *command data is smaller than ours). So we copy the smaller data to our job element command
     *buffer, and the extra part of our command is set as default values*/
    job_service_set_default_job_command_values(job_queue_element->command_data, job_queue_element->job_type);

    /*Copy the job command sent from peer according to the received size info*/
    if(block_from_other_side.command_size > 0)
        fbe_copy_memory(job_queue_element->command_data, 
                &block_from_other_side.command_data, block_from_other_side.command_size);


    job_queue_element->local_job_data_valid = FBE_FALSE;
    job_queue_element->additional_job_data = NULL;

    fbe_job_service_cmi_unlock();
    return;
}
/******************************************************
 * end fbe_job_service_cmi_copy_block_from_other_side()
 *****************************************************/

/*!**************************************************************
 * fbe_job_service_cmi_invalidate_block_from_other_side()
 ****************************************************************
 * @brief
 * sets the block from other side to NULL values
 *
 * @param         
 *
 * @return fbe_status_t - status of call
 *
 * @author
 * 09/15/2010 - Created. Jesus Medina
 *
 ****************************************************************/

void fbe_job_service_cmi_invalidate_block_from_other_side()
{
    fbe_job_service_cmi_lock();            

    /* Just invalidate the block_from_other_side */
    fbe_job_service_cmi_init_block_from_other_side();
    fbe_job_service_cmi_unlock();
}
/************************************************************
 * end fbe_job_service_cmi_invalidate_block_from_other_side()
 ***********************************************************/

/*!****************************************************
 * fbe_job_service_cmi_init_message()
 ******************************************************
 * @brief
 * This function inits values of a job service message
 *
 * @param - fbe_job_service_cmi_msg: job service message
 *
 * @return - status of call 
 *
 * @author
 * 03/31/2011 - Created. Jesus Medina
 *
 ******************************************************/
static void fbe_job_service_cmi_init_message(fbe_job_service_cmi_message_t *fbe_job_service_cmi_msg)
{
    fbe_job_service_cmi_msg->queue_type = FBE_JOB_INVALID_QUEUE;
    fbe_job_service_cmi_msg->job_number = 0; /* 0 IS NOT USED */  
    fbe_job_service_cmi_msg->status = FBE_STATUS_OK;
    fbe_job_service_cmi_msg->job_service_cmi_message_type = FBE_JOB_CONTROL_SERVICE_CMI_MESSAGE_TYPE_INVALID;
    fbe_job_service_cmi_msg->error_code = FBE_JOB_CONTROL_SERVICE_CMI_MESSAGE_TYPE_INVALID; 
    fbe_job_service_cmi_msg->object_id = FBE_OBJECT_ID_INVALID;
    fbe_job_service_cmi_msg->class_id = FBE_CLASS_ID_INVALID;
    fbe_job_service_cmi_msg->msg.job_service_queue_element.job_element.job_type = FBE_JOB_TYPE_INVALID;
    fbe_job_service_cmi_msg->msg.job_service_queue_element.job_element.timestamp = 0;
    fbe_job_service_cmi_msg->msg.job_service_queue_element.job_element.current_state = FBE_JOB_ACTION_STATE_INVALID;
    fbe_job_service_cmi_msg->msg.job_service_queue_element.job_element.previous_state = FBE_JOB_ACTION_STATE_INVALID;
    fbe_job_service_cmi_msg->msg.job_service_queue_element.job_element.queue_access = FBE_FALSE;
    fbe_job_service_cmi_msg->msg.job_service_queue_element.job_element.num_objects = 0;
    fbe_job_service_cmi_msg->msg.job_service_queue_element.job_element.job_number = 0;
    fbe_job_service_cmi_msg->msg.job_service_queue_element.job_element.status = FBE_STATUS_OK;
    fbe_job_service_cmi_msg->msg.job_service_queue_element.job_element.error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;

    fbe_zero_memory(fbe_job_service_cmi_msg->msg.job_service_queue_element.job_element.command_data, FBE_JOB_ELEMENT_CONTEXT_SIZE);
}

/*!**************************************************************
 * fbe_job_service_cmi_translate_message_type()
 ****************************************************************
 * @brief
 * translates and returns an arrival job service job message
 * to string
 *
 * @param control_code - job type to translate             
 *
 * @return translated_code - matching string for control code
 *
 * @author
 * 09/22/2010 - Created. Jesus Medina
 *
 ****************************************************************/

fbe_status_t fbe_job_service_cmi_translate_message_type(fbe_job_type_t message_type,
        const char ** pp_message_type_name)
{
    *pp_message_type_name = NULL;
    switch (message_type) {
        case FBE_JOB_CONTROL_SERVICE_CMI_MESSAGE_TYPE_ADD_TO_CREATION_QUEUE:
            *pp_message_type_name = "FBE_JOB_CONTROL_SERVICE_CMI_MESSAGE_TYPE_ADD_TO_CREATION_QUEUE";
            break;
        case FBE_JOB_CONTROL_SERVICE_CMI_MESSAGE_TYPE_ADDED_TO_CREATION_QUEUE:
            *pp_message_type_name = "FBE_JOB_CONTROL_SERVICE_CMI_MESSAGE_TYPE_ADDED_TO_CREATION_QUEUE";
            break;
        case FBE_JOB_CONTROL_SERVICE_CMI_MESSAGE_TYPE_REMOVE_FROM_CREATION_QUEUE:
            *pp_message_type_name = "FBE_JOB_CONTROL_SERVICE_CMI_MESSAGE_TYPE_REMOVE_FROM_CREATION_QUEUE";
            break;
        case FBE_JOB_CONTROL_SERVICE_CMI_MESSAGE_TYPE_REMOVED_FROM_CREATION_QUEUE:
            *pp_message_type_name = "FBE_JOB_CONTROL_SERVICE_CMI_MESSAGE_TYPE_REMOVED_FROM_CREATION_QUEUE";
            break;
        case FBE_JOB_CONTROL_SERVICE_CMI_MESSAGE_TYPE_ADD_TO_RECOVERY_QUEUE:
            *pp_message_type_name = "FBE_JOB_CONTROL_SERVICE_CMI_MESSAGE_TYPE_ADD_TO_RECOVERY_QUEUE";
            break;
        case FBE_JOB_CONTROL_SERVICE_CMI_MESSAGE_TYPE_ADDED_TO_RECOVERY_QUEUE:
            *pp_message_type_name = "FBE_JOB_CONTROL_SERVICE_CMI_MESSAGE_TYPE_ADDED_TO_RECOVERY_QUEUE";
            break;
        case FBE_JOB_CONTROL_SERVICE_CMI_MESSAGE_TYPE_REMOVE_FROM_RECOVERY_QUEUE:
            *pp_message_type_name = "FBE_JOB_CONTROL_SERVICE_CMI_MESSAGE_TYPE_REMOVE_FROM_RECOVERY_QUEUE";
            break;
        case FBE_JOB_CONTROL_SERVICE_CMI_MESSAGE_TYPE_REMOVED_FROM_RECOVERY_QUEUE:
            *pp_message_type_name = "FBE_JOB_CONTROL_SERVICE_CMI_MESSAGE_TYPE_REMOVED_FROM_RECOVERY_QUEUE";
            break;
        case FBE_JOB_CONTROL_SERVICE_CMI_MESSAGE_TYPE_UPDATE_PEER_JOB_QUEUE:
            *pp_message_type_name = "FBE_JOB_CONTROL_SERVICE_CMI_MESSAGE_TYPE_UPDATE_PEER_JOB_QUEUE";
            break;
        case FBE_JOB_CONTROL_SERVICE_CMI_MESSAGE_TYPE_UPDATE_JOB_QUEUE_DONE:
            *pp_message_type_name = "FBE_JOB_CONTROL_SERVICE_CMI_MESSAGE_TYPE_UPDATE_JOB_QUEUE_DONE";
            break;
        case FBE_JOB_CONTROL_SERVICE_CMI_MESSAGE_TYPE_SYNC_JOB_STATE:
            *pp_message_type_name = "FBE_JOB_CONTROL_SERVICE_CMI_MESSAGE_TYPE_SYNC_JOB_STATE";
            break;
        case FBE_JOB_CONTROL_SERVICE_CMI_MESSAGE_TYPE_INVALID:
        default:
            *pp_message_type_name = "UNKNOWN_FBE_JOB_MESSAGE_TYPE";
            break;
    }
    return FBE_STATUS_OK;
}
/**************************************************
 * end fbe_job_service_cmi_translate_message_type()
 *************************************************/


/*!****************************************************
 * fbe_job_service_cmi_get_job_elements_from_peer()
 ******************************************************
 * @brief
 * This code is running from the passive side. It asks the active
 * side for the job elements so it can mirror on its side.
 *
 * @param - fbe_job_service_cmi_msg: job service message
 *
 * @return - status of call 
 *
 * @author
 * 10/10/2011 - Created. Arun S
 *
 ******************************************************/
fbe_status_t fbe_job_service_cmi_get_job_elements_from_peer(void)
{
    fbe_job_queue_element_t *job_queue_element_p = NULL;

    job_queue_element_p = (fbe_job_queue_element_t *)fbe_transport_allocate_buffer();
    if(job_queue_element_p == NULL)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s : ERROR: Could not allocate memory for job request\n",
                          __FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Stop processing the Job Queues */
    fbe_job_service_handle_queue_access(FBE_FALSE);

    job_queue_element_p->job_type = FBE_JOB_TYPE_UPDATE_JOB_ELEMENTS_ON_PEER;
    job_queue_element_p->queue_type = FBE_JOB_CREATION_QUEUE;
    job_queue_element_p->command_size = 0;
    fbe_zero_memory(job_queue_element_p->command_data, FBE_JOB_ELEMENT_CONTEXT_SIZE);

    job_service_trace(FBE_TRACE_LEVEL_INFO, 
                      FBE_TRACE_MESSAGE_ID_INFO, 
                      "%s: PASSIVE requesting ACTIVE to sync Job Q. \n", __FUNCTION__);

    if (fbe_job_service.job_service_cmi_message_p == NULL) 
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, 
                          FBE_TRACE_MESSAGE_ID_INFO, 
                          "%s: cannot allocate memory for Job service cmi messages\n", 
                          __FUNCTION__);

        fbe_transport_release_buffer(job_queue_element_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_job_service_arrival_thread_copy_queue_element(
            &fbe_job_service.job_service_cmi_message_p->msg.job_service_queue_element.job_element,
            job_queue_element_p);

    fbe_job_service.job_service_cmi_message_p->queue_type = FBE_JOB_CREATION_QUEUE;
    fbe_job_service.job_service_cmi_message_p->job_service_cmi_message_type = 
        FBE_JOB_CONTROL_SERVICE_CMI_MESSAGE_TYPE_UPDATE_PEER_JOB_QUEUE;

    /* now send message to peer SP */	
    fbe_job_service_cmi_send_message(fbe_job_service.job_service_cmi_message_p, NULL);

    /* At this point we are done with the processing of the job control element. 
     * So release it.. 
     */
    fbe_transport_release_buffer(job_queue_element_p);

    return FBE_STATUS_OK;
} /* end fbe_job_service_cmi_get_job_elements_from_peer()*/

/*!****************************************************
 * fbe_job_service_cmi_update_peer_job_queue()
 ******************************************************
 * @brief
 * This function is called from ACTIVE side context when
 * the PASSIVE asks for job_sync.  
 *
 * @param         
 *
 * @return - none
 *
 * @author
 * 10/11/2011 - Created. Arun S
 *
 ****************************************************************/
void fbe_job_service_cmi_update_peer_job_queue(fbe_job_queue_element_t *fbe_job_element_p)
{
    /* Stop processing the Job Queues */
    fbe_job_service_handle_queue_access(FBE_FALSE);

    /* Set the thread flag to RUN mode.. */
    fbe_job_service_arrival_thread_set_flag(FBE_JOB_SERVICE_ARRIVAL_THREAD_FLAGS_SYNC); 
    fbe_job_service_thread_set_flag(FBE_JOB_SERVICE_THREAD_FLAGS_SYNC); 

    job_service_trace(FBE_TRACE_LEVEL_INFO, 
                      FBE_TRACE_MESSAGE_ID_INFO, 
                      "%s: ACTIVE received a request from PASSIVE to sync up Jobs. \n", __FUNCTION__);

    /* Copy the job to block from other side.. This will also take care of 
     * releasing the arrival queue semaphore to process jobs... 
     */
    fbe_job_service_cmi_copy_job_queue_element_to_block_from_other_side(fbe_job_element_p);

    return;
}


/*!****************************************************
 * fbe_job_service_cmi_update_job_queue_completion()
 ******************************************************
 * @brief
 * This is the completion function for Sync operation.
 * It Runs on the Passive side. When received from active side,
 * we need to set the thread to RUN mode, invalidate the
 * block_in_progress and block_from_other_side and unfreeze
 * all the queues... 
 *
 * @param         
 *
 * @return - none
 *
 * @author
 * 10/19/2011 - Created. Arun S
 *
 ****************************************************************/
fbe_status_t fbe_job_service_cmi_update_job_queue_completion()
{
    fbe_atomic_t js_gate = 0;

    job_service_trace(FBE_TRACE_LEVEL_INFO, 
                      FBE_TRACE_MESSAGE_ID_INFO, 
                      "%s: Number of Jobs sync'd: %d\n", __FUNCTION__, 
                      fbe_job_service_get_number_creation_requests());

    /*! Check if service is shutting down */
    js_gate = fbe_atomic_increment(&fbe_job_service.arrival.outstanding_requests);
    if(js_gate & FBE_JOB_SERVICE_GATE_BIT){ 
        /* We have a gate, so fail the request */
        job_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO, 
                          "%s: Job Service is shutting down\n", __FUNCTION__);

        fbe_atomic_decrement(&fbe_job_service.arrival.outstanding_requests);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Set the thread to RUN mode.. */
    fbe_job_service_arrival_thread_set_flag(FBE_JOB_SERVICE_ARRIVAL_THREAD_FLAGS_RUN); 
    fbe_job_service_thread_set_flag(FBE_JOB_SERVICE_THREAD_FLAGS_RUN); 

    /* Unfreeze all the queues for processing. */
    fbe_job_service_handle_queue_access(FBE_TRUE);

    /* once we have replied to peer, reset block from other side to NULL */
    fbe_job_service_cmi_invalidate_block_from_other_side();
    fbe_job_service_cmi_invalidate_block_in_progress_block_from_other_side();

    /* Release the semaphore.. */
    fbe_semaphore_release(&fbe_job_service.arrival.job_service_arrival_queue_semaphore, 0, 1, FALSE);
    fbe_atomic_decrement(&fbe_job_service.arrival.outstanding_requests);

    return FBE_STATUS_OK;
}

/*!*********************************************************************************
 * fbe_job_service_sync_job_state_to_peer_before_commit()
 ***********************************************************************************
 * @brief
 * Using this routine to synchronize the state of the current executing job to
 * peer. Generally this function is called right before a job enters PERSIST
 * state, where the database transaction would be commit.
 *
 * The reason why we do this is :
 * When this SP panics during persist state, the surviving SP's database service 
 * would handle the incomplete transaction. If it continues to finish the transaction,
 * it will notify job service that the specific job is finished successfully,
 * then when the job re-runs, it just sends the notification based on the synced
 * job element where the job result is stored.
 *
 * @param job_queue_element_p - the pointer of the current executing job
 *
 * @return - FBE_STATUS_OK if nothing is wrong
 *
 * @author
 * 11/22/2012 - Created. Zhipeng Hu
 *
 ***********************************************************************************/
fbe_status_t fbe_job_service_sync_job_state_to_peer_before_commit(fbe_job_queue_element_t *job_queue_element_p)
{
    if(NULL == job_queue_element_p)
        return FBE_STATUS_GENERIC_FAILURE;

    if(!fbe_job_service_cmi_is_active())
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_INFO, 
                          "job_sync_job_state: we are not active side, not expect to do sync. job_no: 0x%x\n", 
                          (unsigned int)job_queue_element_p->job_number);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    job_service_trace(FBE_TRACE_LEVEL_INFO,
                      FBE_TRACE_MESSAGE_ID_INFO, 
                      "job_sync_job_state: entry. job_no: 0x%x\n", 
                      (unsigned int)job_queue_element_p->job_number);
    
    fbe_job_service.job_service_cmi_message_exec_p->queue_type = job_queue_element_p->queue_type;
    fbe_job_service.job_service_cmi_message_exec_p->job_number = job_queue_element_p->job_number;
    fbe_copy_memory(&(fbe_job_service.job_service_cmi_message_exec_p->msg.job_service_queue_element.job_element),
                    job_queue_element_p,
                    sizeof(fbe_job_queue_element_t));
    fbe_job_service.job_service_cmi_message_exec_p->job_service_cmi_message_type = 
        FBE_JOB_CONTROL_SERVICE_CMI_MESSAGE_TYPE_SYNC_JOB_STATE;

    /* now send message to peer SP */   
    fbe_job_service_cmi_send_message(fbe_job_service.job_service_cmi_message_exec_p, NULL);

    fbe_job_service_cmi_init_message(fbe_job_service.job_service_cmi_message_exec_p);

    return FBE_STATUS_OK;    
}

/*!*********************************************************************************
 * fbe_job_service_cmi_process_sync_job_state_request()
 ***********************************************************************************
 * @brief
 * When active side sends to passive side the state of the current executing job, 
 * passive side would store the state in its own job element so that when active SP
 * panics, we have sufficient data to continue the job.
 *
 * @param job_queue_element_p - the pointer of the current executing job
 *
 * @return - FBE_STATUS_OK if nothing is wrong
 *
 * @author
 * 11/22/2012 - Created. Zhipeng Hu
 *
 ***********************************************************************************/
static void fbe_job_service_cmi_process_sync_job_state_request(fbe_job_queue_element_t *job_element_p)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_queue_head_t           *head_p = NULL;
    fbe_job_queue_element_t    *request_p = NULL;
    fbe_u32_t                   job_index = 0;

    if(NULL == job_element_p)
    {
        return;
    }

    /*! @note There are some job types that `currently' are not replicated on 
     *        the peer:
     *          o FBE_JOB_TYPE_UPDATE_SPARE_LIBRARY_CONFIG
     *        If the above job types are replicated then remove this code.
     */
    if (job_element_p->job_type == FBE_JOB_TYPE_UPDATE_SPARE_LIBRARY_CONFIG)
    {
        /* Return success.
         */
        return;
    }

    /*! Lock queue */
    fbe_job_service_lock();

    /* let's look at the outstanding request first */
    request_p = fbe_job_service_cmi_get_queue_element_from_block_in_progress();
    if (request_p!=NULL)
    {
        if (request_p->job_number == job_element_p->job_number)
        {
            /* we can't copy the local stuff */
            request_p->queue_type = job_element_p->queue_type;
            request_p->job_type = job_element_p->job_type;
            request_p->object_id = job_element_p->object_id;
            request_p->class_id = job_element_p->class_id;
            request_p->job_action = job_element_p->job_action;
            request_p->timestamp = job_element_p->timestamp;
            request_p->current_state = job_element_p->current_state;
            request_p->previous_state = job_element_p->previous_state;
            request_p->queue_access = job_element_p->queue_access;
            request_p->num_objects = job_element_p->num_objects;
            request_p->job_number = job_element_p->job_number;
            request_p->status = job_element_p->status;
            request_p->error_code = job_element_p->error_code;
            request_p->command_size = job_element_p->command_size;
            request_p->need_to_wait = job_element_p->need_to_wait;
            request_p->timeout_msec = job_element_p->timeout_msec;

            fbe_copy_memory(request_p->command_data, &job_element_p->command_data, request_p->command_size);

            fbe_job_service_unlock();
            job_service_trace(FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO, 
                              "JOB_process_sync_job_state_request: sync job 0x%x from block in progress\n", 
                              (unsigned int)job_element_p->job_number);
            return;
        }
    }

    if(job_element_p->queue_type == FBE_JOB_CREATION_QUEUE)
    {
        if (fbe_queue_is_empty(&fbe_job_service.creation_q_head) == TRUE)
        {
            status = FBE_STATUS_NO_OBJECT;
        }
        else
        {
            fbe_job_service_get_creation_queue_head(&head_p);
        }        
    }
    else if(job_element_p->queue_type == FBE_JOB_RECOVERY_QUEUE)
    {
        if (fbe_queue_is_empty(&fbe_job_service.recovery_q_head) == TRUE)
        {
            status = FBE_STATUS_NO_OBJECT;
        }
        else
        {
            fbe_job_service_get_recovery_queue_head(&head_p);
        }
    }
    else
    {
        status = FBE_STATUS_NO_OBJECT;
    }

    if (FBE_STATUS_OK == status)
    {    
        status = FBE_STATUS_NO_OBJECT;
        request_p = (fbe_job_queue_element_t *) fbe_queue_front(head_p);
        while (request_p != NULL)
        {
            fbe_job_queue_element_t *next_request_p = NULL;
            if (request_p->job_number == job_element_p->job_number)
            {
                /* we can't copy the local stuff */
                request_p->queue_type = job_element_p->queue_type;
                request_p->job_type = job_element_p->job_type;
                request_p->object_id = job_element_p->object_id;
                request_p->class_id = job_element_p->class_id;
                request_p->job_action = job_element_p->job_action;
                request_p->timestamp = job_element_p->timestamp;
                request_p->current_state = job_element_p->current_state;
                request_p->previous_state = job_element_p->previous_state;
                request_p->queue_access = job_element_p->queue_access;
                request_p->num_objects = job_element_p->num_objects;
                request_p->job_number = job_element_p->job_number;
                request_p->status = job_element_p->status;
                request_p->error_code = job_element_p->error_code;
                request_p->command_size = job_element_p->command_size;
                request_p->need_to_wait = job_element_p->need_to_wait;
                request_p->timeout_msec = job_element_p->timeout_msec;

                fbe_copy_memory(request_p->command_data, &job_element_p->command_data, request_p->command_size);

                status = FBE_STATUS_OK;
                break;
            }

            job_index += 1;
            next_request_p = (fbe_job_queue_element_t *)fbe_queue_next(head_p, &request_p->queue_element);
            request_p = next_request_p;
        }

        fbe_job_service_unlock();

        if(FBE_STATUS_OK != status)
        {
            job_service_trace(FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO, 
                              "JOB_process_sync_job_state_request: FAIL to find job 0x%x in %s queue\n", 
                              (unsigned int)job_element_p->job_number,
                              job_element_p->queue_type == FBE_JOB_CREATION_QUEUE?"CREATION":"RECOVERY");
        }
        
    }
    else
    {
        fbe_job_service_unlock();

        job_service_trace(FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_INFO, 
                          "JOB_process_sync_job_state_request: invalid/empty queue %d for job 0x%x\n", 
                          job_element_p->queue_type,
                          (unsigned int)job_element_p->job_number);

    }

    /* Do not send cmi msg here in the DPC context !
    fbe_job_service.job_service_cmi_message_exec_p->queue_type = job_queue_element_p->queue_type;
    fbe_job_service.job_service_cmi_message_exec_p->job_number = job_queue_element_p->job_number;
    fbe_copy_memory(&(fbe_job_service.job_service_cmi_message_exec_p->msg.job_service_queue_element.job_element),
                    job_queue_element_p,
                    sizeof(fbe_job_queue_element_t));
    fbe_job_service.job_service_cmi_message_exec_p->job_service_cmi_message_type = 
        FBE_JOB_CONTROL_SERVICE_CMI_MESSAGE_TYPE_SYNC_JOB_STATE;

    fbe_job_service_cmi_send_message(fbe_job_service.job_service_cmi_message_exec_p, NULL);
    */    
}

/*!**************************************************************
 * fbe_job_service_cmi_get_remove_job_action_on_block_in_progress()
 ****************************************************************
 * @brief
 * gets the remove job action to help arrival thread finish the 
 * following element remove process
 *
 * @param -  fbe_job_queue_element_remove_action_t *remove_action
 *
 * @return fbe_status_t
 *
 * @author
 * 02/20/2013 - Created. Wenxuan Yin
 *
 ****************************************************************/
fbe_status_t fbe_job_service_cmi_get_remove_job_action_on_block_in_progress
                                        (fbe_job_queue_element_remove_action_t *remove_action)
{
    if (NULL == remove_action)
        return FBE_STATUS_GENERIC_FAILURE;
    
    fbe_job_service_cmi_lock();
    
    remove_action->flag = block_in_progress.job_queue_element_remove_action.flag;    
    remove_action->job_number = block_in_progress.job_queue_element_remove_action.job_number;
    remove_action->object_id = block_in_progress.job_queue_element_remove_action.object_id;
    remove_action->status = block_in_progress.job_queue_element_remove_action.status;
    remove_action->error_code = block_in_progress.job_queue_element_remove_action.error_code;
    remove_action->class_id = block_in_progress.job_queue_element_remove_action.class_id;

    remove_action->job_element.queue_type = block_in_progress.job_queue_element_remove_action.job_element.queue_type;
    remove_action->job_element.job_type= block_in_progress.job_queue_element_remove_action.job_element.job_type;
    remove_action->job_element.timestamp = block_in_progress.job_queue_element_remove_action.job_element.timestamp;
    remove_action->job_element.current_state = block_in_progress.job_queue_element_remove_action.job_element.current_state;
    remove_action->job_element.previous_state = block_in_progress.job_queue_element_remove_action.job_element.previous_state;
    remove_action->job_element.queue_access = block_in_progress.job_queue_element_remove_action.job_element.queue_access;
    remove_action->job_element.num_objects = block_in_progress.job_queue_element_remove_action.job_element.num_objects;
    remove_action->job_element.job_number = block_in_progress.job_queue_element_remove_action.job_element.job_number;
    remove_action->job_element.job_action = block_in_progress.job_queue_element_remove_action.job_element.job_action;
    remove_action->job_element.command_size = block_in_progress.job_queue_element_remove_action.job_element.command_size;
    remove_action->job_element.need_to_wait = block_in_progress.job_queue_element_remove_action.job_element.need_to_wait;
    remove_action->job_element.timeout_msec = block_in_progress.job_queue_element_remove_action.job_element.timeout_msec;
    remove_action->job_element.object_id = block_in_progress.job_queue_element_remove_action.job_element.object_id;
    remove_action->job_element.class_id = block_in_progress.job_queue_element_remove_action.job_element.class_id;
    remove_action->job_element.status = block_in_progress.job_queue_element_remove_action.job_element.status;
    fbe_zero_memory(remove_action->job_element.command_data, FBE_JOB_ELEMENT_CONTEXT_SIZE);
    if(block_in_progress.job_queue_element_remove_action.job_element.command_size > 0)
        fbe_copy_memory(remove_action->job_element.command_data, 
                        block_in_progress.job_queue_element_remove_action.job_element.command_data, 
                        block_in_progress.job_queue_element_remove_action.job_element.command_size);

    fbe_job_service_cmi_unlock();
    return FBE_STATUS_OK;
}

/*!**************************************************************
 * fbe_job_service_cmi_set_remove_job_action_on_block_in_progress()
 ****************************************************************
 * @brief
 * sets the remove job action to help arrival thread finish the 
 * following element remove process
 *
 * @param -  fbe_job_queue_element_remove_flag_t     in_flag,
 *           fbe_u64_t                               in_job_number,
 *           fbe_object_id_t                         in_object_id,
 *           fbe_status_t                            in_status,     
 *           fbe_job_service_error_type_t            in_error_code,
 *           fbe_class_id_t                          in_class_id,
 *           fbe_job_queue_element_t                 *in_job_element_p
 *
 * @return none
 *
 * @author
 * 02/20/2013 - Created. Wenxuan Yin
 *
 ****************************************************************/
void fbe_job_service_cmi_set_remove_job_action_on_block_in_progress(
                                        fbe_job_queue_element_remove_flag_t     in_flag,
                                        fbe_u64_t                               in_job_number,
                                        fbe_object_id_t                         in_object_id,
                                        fbe_status_t                            in_status,     
                                        fbe_job_service_error_type_t            in_error_code,
                                        fbe_class_id_t                          in_class_id,
                                        fbe_job_queue_element_t                 *in_job_element_p)
{
    fbe_job_service_cmi_lock();
    
    block_in_progress.job_queue_element_remove_action.flag = in_flag;    
    block_in_progress.job_queue_element_remove_action.job_number = in_job_number;
    block_in_progress.job_queue_element_remove_action.object_id = in_object_id;
    block_in_progress.job_queue_element_remove_action.status = in_status;
    block_in_progress.job_queue_element_remove_action.error_code = in_error_code;
    block_in_progress.job_queue_element_remove_action.class_id = in_class_id;

    /* Copy job element */
    if(in_job_element_p != NULL)
    {
        block_in_progress.job_queue_element_remove_action.job_element.queue_type = in_job_element_p->queue_type;
        block_in_progress.job_queue_element_remove_action.job_element.job_type = in_job_element_p->job_type;
        block_in_progress.job_queue_element_remove_action.job_element.timestamp = in_job_element_p->timestamp;
        block_in_progress.job_queue_element_remove_action.job_element.current_state = in_job_element_p->current_state;
        block_in_progress.job_queue_element_remove_action.job_element.previous_state = in_job_element_p->previous_state;
        block_in_progress.job_queue_element_remove_action.job_element.queue_access = in_job_element_p->queue_access;
        block_in_progress.job_queue_element_remove_action.job_element.num_objects = in_job_element_p->num_objects;
        block_in_progress.job_queue_element_remove_action.job_element.job_number = in_job_element_p->job_number;
        block_in_progress.job_queue_element_remove_action.job_element.job_action = in_job_element_p->job_action;
        block_in_progress.job_queue_element_remove_action.job_element.command_size = in_job_element_p->command_size;
        block_in_progress.job_queue_element_remove_action.job_element.need_to_wait = in_job_element_p->need_to_wait;
        block_in_progress.job_queue_element_remove_action.job_element.timeout_msec = in_job_element_p->timeout_msec;
        block_in_progress.job_queue_element_remove_action.job_element.object_id = in_job_element_p->object_id;
        block_in_progress.job_queue_element_remove_action.job_element.class_id = in_job_element_p->class_id;
        block_in_progress.job_queue_element_remove_action.job_element.status = in_job_element_p->status;
        fbe_copy_memory(block_in_progress.job_queue_element_remove_action.job_element.command_data, in_job_element_p->command_data, FBE_JOB_ELEMENT_CONTEXT_SIZE);
    }

    fbe_job_service_cmi_unlock();
}

/*!**************************************************************
 * fbe_job_service_cmi_init_remove_job_action_on_block_in_progress()
 ****************************************************************
 * @brief
 * This static func initializes the remove job action structure
 *
 * @param -  None
 *
 * @return - None
 *
 * @author
 * 03/09/2013 - Created. Wenxuan Yin
 *
 ****************************************************************/

static void fbe_job_service_cmi_init_remove_job_action_on_block_in_progress()
{
    block_in_progress.job_queue_element_remove_action.job_number = 0;
    block_in_progress.job_queue_element_remove_action.flag = FBE_JOB_QUEUE_ELEMENT_REMOVE_NO_ACTION;
    block_in_progress.job_queue_element_remove_action.object_id = FBE_OBJECT_ID_INVALID;
    block_in_progress.job_queue_element_remove_action.status = FBE_STATUS_OK;
    block_in_progress.job_queue_element_remove_action.error_code = FBE_JOB_SERVICE_ERROR_NO_ERROR;
    block_in_progress.job_queue_element_remove_action.class_id = FBE_CLASS_ID_INVALID;

    block_in_progress.job_queue_element_remove_action.job_element.queue_type = FBE_JOB_INVALID_QUEUE;
    block_in_progress.job_queue_element_remove_action.job_element.job_type = FBE_JOB_TYPE_INVALID;
    block_in_progress.job_queue_element_remove_action.job_element.timestamp = 0;
    block_in_progress.job_queue_element_remove_action.job_element.current_state = FBE_JOB_ACTION_STATE_INVALID;
    block_in_progress.job_queue_element_remove_action.job_element.previous_state = FBE_JOB_ACTION_STATE_INVALID;
    block_in_progress.job_queue_element_remove_action.job_element.queue_access = FBE_FALSE;
    block_in_progress.job_queue_element_remove_action.job_element.num_objects = 0;
    block_in_progress.job_queue_element_remove_action.job_element.job_number = 0;
    block_in_progress.job_queue_element_remove_action.job_element.command_size = 0;
    block_in_progress.job_queue_element_remove_action.job_element.need_to_wait = FBE_FALSE;
    block_in_progress.job_queue_element_remove_action.job_element.timeout_msec = 60000;
    block_in_progress.job_queue_element_remove_action.job_element.object_id = FBE_OBJECT_ID_INVALID;
    block_in_progress.job_queue_element_remove_action.job_element.class_id = FBE_CLASS_ID_INVALID;
    block_in_progress.job_queue_element_remove_action.job_element.status = FBE_STATUS_OK;
    fbe_zero_memory(block_in_progress.job_queue_element_remove_action.job_element.command_data, FBE_JOB_ELEMENT_CONTEXT_SIZE);
}

/*!**************************************************************
 * fbe_job_service_cmi_cleanup_remove_job_action_on_block_in_progress()
 ****************************************************************
 * @brief
 * Cleanup the remove job action structure for outside call
 *
 * @param -  None
 *
 * @return - None
 *
 * @author
 * 03/09/2013 - Created. Wenxuan Yin
 *
 ****************************************************************/

void fbe_job_service_cmi_cleanup_remove_job_action_on_block_in_progress()
{
    fbe_job_service_cmi_lock();
    fbe_job_service_cmi_init_remove_job_action_on_block_in_progress();
    fbe_job_service_cmi_unlock();
}

