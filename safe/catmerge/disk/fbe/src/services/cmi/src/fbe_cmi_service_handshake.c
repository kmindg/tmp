/***************************************************************************
 * Copyright (C) EMC Corporation 2001-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *  fbe_cmi_service_handshake.c
 ***************************************************************************
 *
 *  Description
 *      This file contains the implementation of the handshake between the two SPs 
 *      to decide who is the active and who is the passive
 *      
 *    
 ***************************************************************************/

#include "fbe/fbe_types.h"
#include "fbe/fbe_winddk.h"
#include "fbe_base_service.h"
#include "fbe/fbe_cmi_interface.h"
#include "fbe_cmi_private.h"

/*local structures*/
#define FBE_CMI_MAX_HANDSHAKE_RETRY 5

typedef enum fbe_cmi_msg_send_thread_flag_e{
    FBE_CMI_MSG_SEND_THREAD_RUN,
    FBE_CMI_MSG_SEND_THREAD_STOP,
    FBE_CMI_MSG_SEND_THREAD_DONE
}fbe_cmi_msg_send_thread_flag_t;

/*static memberes*/
static fbe_cmi_internal_message_t *		handshake_msg = NULL;/*we can use only one on the stack*/
static fbe_cmi_internal_message_t *		open_msg = NULL;     /*we can use only one on the stack*/
static volatile fbe_bool_t              cmi_service_open_in_progress = FBE_FALSE; /* open msg is used*/
static volatile fbe_cmi_sp_state_t		current_sp_state = FBE_CMI_STATE_BUSY;/*inital state when we discover ourselvs*/
static volatile fbe_bool_t				cmi_service_peer_alive = FBE_TRUE;/*initial state is TRUE so we can make initial send*/
static fbe_semaphore_t					cmi_service_msg_send_semaphore;
static volatile fbe_bool_t              cmi_service_handshake_needed = FBE_FALSE; /* need to send handshake*/
static fbe_cmi_msg_send_thread_flag_t	cmi_service_msg_send_thread_flag = FBE_CMI_MSG_SEND_THREAD_RUN;
static fbe_thread_t                 	fbe_cmi_msg_send_thread_handle;
static volatile fbe_bool_t              cmi_service_peer_service_mode = FBE_FALSE; /* initial state is FALSE. it is used to mark whether peer is service mode*/

/*forward decelrations*/
static fbe_status_t	cmi_service_handshake_process_peer_not_present(fbe_cmi_internal_message_t *cmi_internal_message, fbe_cmi_event_callback_context_t contex);
static fbe_status_t	cmi_service_handshake_process_received_message(fbe_cmi_internal_message_t *cmi_internal_message);
static fbe_status_t	cmi_service_handshake_process_transmitted_message(fbe_cmi_internal_message_t *cmi_internal_message, fbe_cmi_event_callback_context_t contex);
static fbe_status_t fbe_metadata_process_fatal_error_message(fbe_cmi_internal_message_t *cmi_internal_message, fbe_cmi_event_callback_context_t contex);
static fbe_status_t	cmi_service_handshake_process_contact_lost(void);
static fbe_status_t cmi_service_handshake_process_received_handshake(fbe_cmi_internal_message_t *cmi_internal_message);
static fbe_status_t cmi_service_handshake_process_busy(fbe_cmi_internal_message_t *cmi_internal_message);
static fbe_status_t cmi_service_handshake_process_received_open(fbe_cmi_internal_message_t *cmi_internal_message);
static void fbe_cmi_msg_send_thread_func(void * context);

extern fbe_cmi_client_info_t               client_list[FBE_CMI_CLIENT_ID_LAST];

/*****************************************************************************************************************************************/
/* 
   Note: user_message_length is only valid for received msg notifications, all other cases will have zero passed in
*/
fbe_status_t fbe_cmi_service_msg_callback(fbe_cmi_event_t event, fbe_u32_t user_message_length, fbe_cmi_message_t user_message, fbe_cmi_event_callback_context_t context)
{
	fbe_cmi_internal_message_t *		cmi_internal_msg = (fbe_cmi_internal_message_t*)user_message;
	fbe_status_t						status = FBE_STATUS_GENERIC_FAILURE;
    
    switch (event) {
	case FBE_CMI_EVENT_MESSAGE_TRANSMITTED:
		status = cmi_service_handshake_process_transmitted_message(cmi_internal_msg, context);
        break;
	case FBE_CMI_EVENT_SP_CONTACT_LOST:
		status = cmi_service_handshake_process_contact_lost();
		break;
	case FBE_CMI_EVENT_MESSAGE_RECEIVED:
		status = cmi_service_handshake_process_received_message(cmi_internal_msg);
		break;
	case FBE_CMI_EVENT_FATAL_ERROR:
		fbe_metadata_process_fatal_error_message(cmi_internal_msg, context);
		break;
	case FBE_CMI_EVENT_PEER_NOT_PRESENT:
		status = cmi_service_handshake_process_peer_not_present(cmi_internal_msg, context);
		break;
	case FBE_CMI_EVENT_PEER_BUSY:
        status = cmi_service_handshake_process_busy(cmi_internal_msg);
		break;
	default:
		fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR, "%s, Invalid state (%d)for processing FBE_CMI_INTERNAL_MESSAGE_HANDSHAKE\n", __FUNCTION__, event);
		status = FBE_STATUS_GENERIC_FAILURE;
	}
    
	return status;

}

void fbe_cmi_send_open_to_peer(fbe_cmi_client_id_t client_id)
{
    fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, "open sent for client %d.\n", client_id);
	/*set up the message*/
	open_msg->internal_msg_type = FBE_CMI_INTERNAL_MESSAGE_OPEN;
    open_msg->internal_msg_union.open.fbe_cmi_version = FBE_CMI_VERSION;
    open_msg->internal_msg_union.open.client_id = client_id;

    /*and send it to the peer, we will continue processing with the ack*/
    fbe_cmi_send_message(FBE_CMI_CLIENT_ID_CMI,
						 sizeof(fbe_cmi_internal_message_t),
						 (fbe_cmi_message_t)open_msg,
						 NULL);
}

void fbe_cmi_send_handshake_to_peer(void)
{
	/*set up the message*/
	handshake_msg->internal_msg_type = FBE_CMI_INTERNAL_MESSAGE_HANDSHAKE;
    handshake_msg->internal_msg_union.handshake.fbe_cmi_version = FBE_CMI_VERSION;

	fbe_cmi_get_current_sp_state(&handshake_msg->internal_msg_union.handshake.current_state);
    /*and send it to the peer, we will continue processing with the ack*/
    fbe_cmi_send_message(FBE_CMI_CLIENT_ID_CMI,
						 sizeof(fbe_cmi_internal_message_t),
						 (fbe_cmi_message_t)handshake_msg,
						 NULL);
   
}

/*this function is being called when we sent a message to the peer and the peer was not there.
don't confuse it with FBE_CMI_EVENT_SP_CONTACT_LOST which is asynchronous and we get it when the peer dies*/
static fbe_status_t	cmi_service_handshake_process_peer_not_present(fbe_cmi_internal_message_t *cmi_internal_message,
																   fbe_cmi_event_callback_context_t contex)
{
	fbe_status_t						status = FBE_STATUS_GENERIC_FAILURE;
	fbe_cmi_sp_state_t					current_state = FBE_CMI_STATE_INVALID;
	
	switch (cmi_internal_message->internal_msg_type) {
	case FBE_CMI_INTERNAL_MESSAGE_OPEN:
        // drop the message
        cmi_service_open_in_progress = FBE_FALSE;  
        status = FBE_STATUS_OK;
        break;
	case FBE_CMI_INTERNAL_MESSAGE_HANDSHAKE:
		cmi_service_handshake_set_peer_alive(FBE_FALSE);/*let's mark the peer as alive*/
		fbe_cmi_get_current_sp_state(&current_state);
		if (current_state == FBE_CMI_STATE_BUSY) {
			fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, "%s:Peer is dead we are ACTIVE\n", __FUNCTION__);
			cmi_service_handshake_set_sp_state(FBE_CMI_STATE_ACTIVE);
			status = FBE_STATUS_OK;
		}else if (current_state == FBE_CMI_STATE_ACTIVE){
			fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, "%s:We were ACTIVE before and we stay ACTIVE\n", __FUNCTION__);
			status = FBE_STATUS_OK;
        } else if (current_state == FBE_CMI_STATE_SERVICE_MODE) {
            fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, "%s:current sp is degraded mode and keep degraded\n", __FUNCTION__);
            status = FBE_STATUS_OK;
		}else{
			fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR, "%s:Invalid state for processing FBE_CMI_INTERNAL_MESSAGE_HANDSHAKE\n", __FUNCTION__);
			status = FBE_STATUS_GENERIC_FAILURE;
		}
		break;
	default:
		fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR, "%s, invalid internal message %d\n", __FUNCTION__, cmi_internal_message->internal_msg_type);
		status = FBE_STATUS_GENERIC_FAILURE;
	}

	return status ;

}

fbe_status_t fbe_cmi_get_current_sp_state(fbe_cmi_sp_state_t *get_state)
{
	if (get_state != NULL) {
        *get_state = current_sp_state;
		return FBE_STATUS_OK;
	}else{
		return FBE_STATUS_GENERIC_FAILURE;
	}
}

void cmi_service_handshake_set_sp_state(fbe_cmi_sp_state_t set_state)
{
    current_sp_state = set_state;
}

void fbe_cmi_set_current_sp_state(fbe_cmi_sp_state_t state)
{
    current_sp_state = state;
}

static fbe_bool_t fbe_cmi_is_peer_in_service_mode(void)
{
    return cmi_service_peer_service_mode;
}

void fbe_cmi_set_peer_service_mode(fbe_bool_t service_mode)
{
    /* Enabling service mode will break monitors.*/
    if (service_mode == FBE_TRUE)
    {
        fbe_cmi_trace(FBE_TRACE_LEVEL_WARNING, "%s->%s\n", __FUNCTION__, "TRUE");
    }
    else
    {
        fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, "%s->%s\n", __FUNCTION__, "FALSE");
    }
    cmi_service_peer_service_mode = service_mode;
}

/*this function is processing an asynchronous receiving of a message from the peer */
static fbe_status_t	cmi_service_handshake_process_received_message(fbe_cmi_internal_message_t *cmi_internal_message)
{
	fbe_status_t						status = FBE_STATUS_GENERIC_FAILURE;
    
    switch (cmi_internal_message->internal_msg_type) {
	case FBE_CMI_INTERNAL_MESSAGE_HANDSHAKE:
        status = cmi_service_handshake_process_received_handshake(cmi_internal_message);
		break;
	case FBE_CMI_INTERNAL_MESSAGE_OPEN:
        status = cmi_service_handshake_process_received_open(cmi_internal_message);
		break;
	default:
		fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR, "%s, invalid internal message %d\n", __FUNCTION__, cmi_internal_message->internal_msg_type);
		status = FBE_STATUS_GENERIC_FAILURE;
	}

	return status;
}

/*this function is processing a successfull ack for a message we sent to the peer*/
static fbe_status_t	cmi_service_handshake_process_transmitted_message(fbe_cmi_internal_message_t *cmi_internal_message,
																	  fbe_cmi_event_callback_context_t contex)
{
	fbe_status_t						status = FBE_STATUS_GENERIC_FAILURE;
    
	switch (cmi_internal_message->internal_msg_type) {
	case FBE_CMI_INTERNAL_MESSAGE_HANDSHAKE:
		cmi_service_handshake_set_peer_alive(FBE_TRUE);/*let's mark the peer as alive*/
		status = FBE_STATUS_OK;
		break;
	case FBE_CMI_INTERNAL_MESSAGE_OPEN:
        cmi_service_open_in_progress = FBE_FALSE;  // done with the message
        // kick the work thread to see if there's more open to be sent.
		fbe_semaphore_release(&cmi_service_msg_send_semaphore, 0, 1, FALSE);
		status = FBE_STATUS_OK;
		break;
	default:
		fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR, "%s, invalid internal message %d\n", __FUNCTION__, cmi_internal_message->internal_msg_type);
		status = FBE_STATUS_GENERIC_FAILURE;
	}

	return status;

}

/*this function is processing the asynchronous message saying the peer has died.
The CMI service itself will let other registered clients know about it, we just have to take care of ourselvs here*/
static fbe_status_t	cmi_service_handshake_process_contact_lost(void)
{
    fbe_cmi_sp_state_t state;

	cmi_service_handshake_set_peer_alive(FBE_FALSE);
    fbe_cmi_get_current_sp_state(&state);
    if (state != FBE_CMI_STATE_SERVICE_MODE) {
        cmi_service_handshake_set_sp_state(FBE_CMI_STATE_ACTIVE);/*when the peer dies, ipso facto, we are the masters*/
        fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, "%s, peer SP died, we are now ACTIVE\n", __FUNCTION__);
    } else {
        fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, "%s, peer SP died, But current sp is degraded mode now. Keep degraded\n", __FUNCTION__);
    }
	return FBE_STATUS_OK;
}


fbe_status_t fbe_cmi_service_init_event_process(void)
{
	EMCPAL_STATUS       		 nt_status;

    fbe_semaphore_init(&cmi_service_msg_send_semaphore, 0, 10);

	nt_status = fbe_thread_init(&fbe_cmi_msg_send_thread_handle, "fbe_cmi_msg_snd", fbe_cmi_msg_send_thread_func, NULL);
    
	if (nt_status != EMCPAL_STATUS_SUCCESS) {
		fbe_cmi_trace (FBE_TRACE_LEVEL_ERROR, "%s: can't start message sending thread, ntstatus:%X\n", __FUNCTION__,  nt_status); 
		return FBE_STATUS_GENERIC_FAILURE;
	}

	handshake_msg = fbe_memory_native_allocate(sizeof(fbe_cmi_internal_message_t));
	if (handshake_msg == NULL) {
		fbe_cmi_trace (FBE_TRACE_LEVEL_ERROR, "%s: can't allocate memory for handshake msg\n", __FUNCTION__); 
		return FBE_STATUS_GENERIC_FAILURE;
	}

	open_msg = fbe_memory_native_allocate(sizeof(fbe_cmi_internal_message_t));
	if (open_msg == NULL) {
		fbe_cmi_trace (FBE_TRACE_LEVEL_ERROR, "%s: can't allocate memory for open msg\n", __FUNCTION__); 
		return FBE_STATUS_GENERIC_FAILURE;
	}

	return FBE_STATUS_OK;
}

void fbe_cmi_service_destroy_event_process(void)
{
   /*stop the thread*/
	cmi_service_msg_send_thread_flag = FBE_CMI_MSG_SEND_THREAD_STOP;
	fbe_semaphore_release(&cmi_service_msg_send_semaphore, 0, 1, FALSE);
	fbe_thread_wait(&fbe_cmi_msg_send_thread_handle);

    fbe_semaphore_destroy(&cmi_service_msg_send_semaphore);
	fbe_memory_native_release(handshake_msg);
	fbe_memory_native_release(open_msg);
}

fbe_bool_t fbe_cmi_is_peer_alive(void)
{
    return cmi_service_peer_alive && !fbe_cmi_is_peer_in_service_mode();
}

/* This function needs to be called before cmi_service_handshake_set_sp_state()
 * Otherwise, we may not release the client blocking on open promptly.
 */
void cmi_service_handshake_set_peer_alive(fbe_bool_t alive)
{
    fbe_cmi_client_id_t     client = FBE_CMI_CLIENT_ID_INVALID;

    cmi_service_peer_alive = alive;
	
	if(cmi_service_peer_alive == FBE_TRUE){ /* We received the handshake, so peer is not lost anymore */
		fbe_atomic_exchange(&fbe_cmi_service_peer_lost, 0);
		fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, "%s, fbe_cmi_service_peer_lost = %lld \n", __FUNCTION__, fbe_cmi_service_peer_lost);
	}

	fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, "%s->%s\n", __FUNCTION__, alive ? "TRUE" : "FALSE");

    //If peer goes away, we need to clean up the states set from peer request.
    if (cmi_service_peer_alive == FBE_FALSE) {
        // need to make sure that the open state is reset.
        for (client = FBE_CMI_CLIENT_ID_INVALID+1; client < FBE_CMI_CLIENT_ID_LAST; client ++) {
            // passive side may be blocking some thread doing open
            if ((current_sp_state == FBE_CMI_STATE_PASSIVE) &&
                ((client_list[client].open_state & FBE_CMI_OPEN_REQUIRED) == FBE_CMI_OPEN_REQUIRED) &&
                ((client_list[client].open_state & FBE_CMI_OPEN_ESTABLISHED) != FBE_CMI_OPEN_ESTABLISHED)) {
                // unblock the client if it's waiting
                fbe_semaphore_release(&client_list[client].sync_open_sem, 0, 1, FALSE);
            }
            fbe_spinlock_lock(&client_list[client].open_state_lock);
            client_list[client].open_state &= ~(FBE_CMI_OPEN_RECEIVED|FBE_CMI_OPEN_ESTABLISHED);
            fbe_spinlock_unlock(&client_list[client].open_state_lock);
        }
    }

}

/* We have received open from peer.
 * If local is passive, the open is completed.
 * If local is not ready, drop this message.
 * If local has replied the open, drop this message.
 * If local is active, we need to reply.
 */
static fbe_status_t cmi_service_handshake_process_received_open(fbe_cmi_internal_message_t *cmi_internal_message)
{
    fbe_cmi_client_id_t			client_id; 

    client_id = cmi_internal_message->internal_msg_union.open.client_id;

    fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, "open received for client %d\n", client_id);

    fbe_spinlock_lock(&client_list[client_id].open_state_lock);
    client_list[client_id].open_state |= FBE_CMI_OPEN_REQUIRED;
    fbe_spinlock_unlock(&client_list[client_id].open_state_lock);

    // passive side, the open is completed.
    if (current_sp_state == FBE_CMI_STATE_PASSIVE)
    {
        fbe_spinlock_lock(&client_list[client_id].open_state_lock);
        client_list[client_id].open_state |= FBE_CMI_OPEN_ESTABLISHED;
        fbe_spinlock_unlock(&client_list[client_id].open_state_lock);
        
        fbe_semaphore_release(&client_list[client_id].sync_open_sem, 0, 1, FALSE);
        fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, "open completed for client %d. \n", client_id);

        return FBE_STATUS_OK;
    }

    // we are active side
    //  drop the message if local is not ready.
    if ((client_list[client_id].open_state & FBE_CMI_OPEN_LOCAL_READY) != FBE_CMI_OPEN_LOCAL_READY)
    {
        fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, "open drop for client %d.  local not ready\n", client_id);
        // drop the message
        return FBE_STATUS_OK;
    }

    //  drop the message if local is not ready.
    if ((client_list[client_id].open_state & FBE_CMI_OPEN_ESTABLISHED) == FBE_CMI_OPEN_ESTABLISHED)
    {
        fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, "open drop for client %d.  open already completed\n", client_id);
        // drop the message, if local had already replied the open
        return FBE_STATUS_OK;
    }

    // Record the client who needs attention
    fbe_spinlock_lock(&client_list[client_id].open_state_lock);
    client_list[client_id].open_state |= FBE_CMI_OPEN_RECEIVED;
    fbe_spinlock_unlock(&client_list[client_id].open_state_lock);
    // Kick the worker to send the open
    fbe_semaphore_release(&cmi_service_msg_send_semaphore, 0, 1, FALSE);

    return FBE_STATUS_OK;
}

static fbe_status_t cmi_service_handshake_process_busy(fbe_cmi_internal_message_t *cmi_internal_message)
{
	fbe_status_t						status = FBE_STATUS_GENERIC_FAILURE;
    
	switch (cmi_internal_message->internal_msg_type) {
    case FBE_CMI_INTERNAL_MESSAGE_HANDSHAKE:
        cmi_service_handshake_needed = FBE_TRUE;
        fbe_semaphore_release(&cmi_service_msg_send_semaphore, 0, 1, FALSE);
        status = FBE_STATUS_OK;
        break;
    case FBE_CMI_INTERNAL_MESSAGE_OPEN:
        cmi_service_open_in_progress = FBE_FALSE;  // done with the message
        if (cmi_service_peer_alive == FBE_TRUE) {
            // same client id need to retry
            fbe_spinlock_lock(&client_list[cmi_internal_message->internal_msg_union.open.client_id].open_state_lock);
            client_list[cmi_internal_message->internal_msg_union.open.client_id].open_state |= FBE_CMI_OPEN_RECEIVED;
            client_list[cmi_internal_message->internal_msg_union.open.client_id].open_state &= ~(FBE_CMI_OPEN_ESTABLISHED);
            fbe_spinlock_unlock(&client_list[cmi_internal_message->internal_msg_union.open.client_id].open_state_lock);
        }
        // kick the work thread to see if there's more open to be sent.
        fbe_semaphore_release(&cmi_service_msg_send_semaphore, 0, 1, FALSE);
        status = FBE_STATUS_OK;
        break;
    default:
        fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR, "%s, invalid internal message %d\n", __FUNCTION__, cmi_internal_message->internal_msg_type);
        status = FBE_STATUS_GENERIC_FAILURE;
	}

	return status;
}

static fbe_status_t cmi_service_handshake_process_received_handshake(fbe_cmi_internal_message_t *cmi_internal_message)
{
	fbe_cmi_sp_state_t					current_state = FBE_CMI_STATE_INVALID;
	fbe_cmi_sp_id_t						this_sp_id = FBE_CMI_SP_ID_INVALID;
	fbe_u32_t							peer_version = 0;

	fbe_cmi_get_current_sp_state(&current_state);
	fbe_cmi_get_sp_id(&this_sp_id);
	peer_version = cmi_internal_message->internal_msg_union.handshake.fbe_cmi_version;
	cmi_service_handshake_set_peer_alive(FBE_TRUE);/*let's mark the peer as alive*/

	fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, "%s, received handshake req. from peer at version:%d, state:%d\n",
				   __FUNCTION__, peer_version, cmi_internal_message->internal_msg_union.handshake.current_state);

	/*if the peer is at BUSY and we are ACTIVE, we need to be the ACTIVE since it means the peer went away and came back at some stage*/
    if (cmi_internal_message->internal_msg_union.handshake.current_state == FBE_CMI_STATE_BUSY && 
        ((current_state == FBE_CMI_STATE_ACTIVE) || (current_state == FBE_CMI_STATE_SERVICE_MODE))) {
		/*sending the message will need to happen in a different thread context because in kerenl we can't send IOCTL with events
		that wait for completion since we are at DISPACH_LEVEL*/
        /* set peer service mode as FBE_FALSE before waking up the handshake thread*/
        fbe_cmi_set_peer_service_mode(FBE_FALSE);
        cmi_service_handshake_needed = FBE_TRUE;
        fbe_semaphore_release(&cmi_service_msg_send_semaphore, 0, 1, FALSE);
        if (current_state == FBE_CMI_STATE_ACTIVE) {
            fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, "Current SP is ACTIVE before and we will stay ACTIVE, Peer is busy\n");
        } else if (current_state == FBE_CMI_STATE_SERVICE_MODE) {
            fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, "Current SP is degraded mode before and keep degraded mode, Peer is busy\n");
        }
		return FBE_STATUS_OK;
	}

	/*if the peer was ACTIVE before us and we are at out BUSY stage, we just give up and turn to PASSIVE*/
	if (cmi_internal_message->internal_msg_union.handshake.current_state == FBE_CMI_STATE_ACTIVE && (current_state == FBE_CMI_STATE_BUSY ||current_state == FBE_CMI_STATE_PASSIVE) ) {
		cmi_service_handshake_set_sp_state(FBE_CMI_STATE_PASSIVE);
        fbe_cmi_set_peer_service_mode(FBE_FALSE);
		fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, "Peer was ACTIVE before us, we are PASSIVE\n");
		return FBE_STATUS_OK;
	}

	/*if the peer was PASSIVE before us and we are at our BUSY stage, we assume we sent our BUSY to the peer who decided to go passive and sent it
	to us so we go active*/
	if (cmi_internal_message->internal_msg_union.handshake.current_state == FBE_CMI_STATE_PASSIVE && (current_state == FBE_CMI_STATE_BUSY) ) {
		cmi_service_handshake_set_sp_state(FBE_CMI_STATE_ACTIVE);
        fbe_cmi_set_peer_service_mode(FBE_FALSE);
		fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, "Peer was PASSIVE before us, we are ACTIVE\n");
		return FBE_STATUS_OK;
	}

	/*if both the peer and us are BUSY and both got handshake request, SPA always wins*/
	if (cmi_internal_message->internal_msg_union.handshake.current_state == FBE_CMI_STATE_BUSY && current_state == FBE_CMI_STATE_BUSY) {
		current_state = ((this_sp_id == FBE_CMI_SP_ID_A) ? FBE_CMI_STATE_ACTIVE : FBE_CMI_STATE_PASSIVE);
		cmi_service_handshake_set_sp_state(current_state);
        fbe_cmi_set_peer_service_mode(FBE_FALSE);
		fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, "%s, We set our state to %s\n", __FUNCTION__, (current_state == FBE_CMI_STATE_ACTIVE ? "ACTIVE" : "PASSIVE"));
		return FBE_STATUS_OK;
	}

	    /* if the peer is in Service Mode */
    if (cmi_internal_message->internal_msg_union.handshake.current_state == FBE_CMI_STATE_SERVICE_MODE) {
        /* When peer becomes in service mode, we need set peer died */
        //cmi_service_handshake_set_peer_alive(FBE_FALSE);
        fbe_cmi_set_peer_service_mode(FBE_TRUE);
        if (current_state == FBE_CMI_STATE_BUSY) {
            cmi_service_handshake_set_sp_state(FBE_CMI_STATE_ACTIVE);
            fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, "Peer is in degraded mode, current sp is BUSY. So we become active\n");
            return FBE_STATUS_OK;
        } else if (current_state == FBE_CMI_STATE_ACTIVE) {
            fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, "Peer is in degraded mode, current sp is active, do nothing\n");
            return FBE_STATUS_OK;
        } else if (current_state == FBE_CMI_STATE_SERVICE_MODE) {
            fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, "Peer is in degraded mode, current sp is degraded mode, do nothing\n");
            return FBE_STATUS_OK;
        } else if (current_state == FBE_CMI_STATE_PASSIVE) {
            cmi_service_handshake_set_sp_state(FBE_CMI_STATE_ACTIVE);
            fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, "Peer is in degraded mode, current sp is passive. Turn current sp to active\n");
            return FBE_STATUS_OK;
        }
    }
    
	fbe_cmi_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, "%s, Illegal state combination,Us:%d, Peer:%d\n",
				  __FUNCTION__, current_state, cmi_internal_message->internal_msg_union.handshake.current_state);

	return FBE_STATUS_GENERIC_FAILURE;


}

static void fbe_cmi_msg_send_thread_func(void * context)
{
    fbe_cmi_client_id_t     client = FBE_CMI_CLIENT_ID_INVALID;

	FBE_UNREFERENCED_PARAMETER(context);

    while(1) {
        fbe_semaphore_wait(&cmi_service_msg_send_semaphore, NULL);
        if (cmi_service_msg_send_thread_flag == FBE_CMI_MSG_SEND_THREAD_RUN) {
            if (cmi_service_handshake_needed == FBE_TRUE) {
                fbe_cmi_send_handshake_to_peer();
                cmi_service_handshake_needed = FBE_FALSE;
            } else if ((current_sp_state == FBE_CMI_STATE_ACTIVE) && 
                       (cmi_service_peer_alive == FBE_TRUE)  &&
                       (cmi_service_open_in_progress != FBE_TRUE)) {
                    /* if we are active, we need to respond to open from peer */
                    for (client = FBE_CMI_CLIENT_ID_INVALID+1; client < FBE_CMI_CLIENT_ID_LAST; client ++) {
                        if ((client_list[client].open_state & FBE_CMI_OPEN_RECEIVED) == FBE_CMI_OPEN_RECEIVED) {
                            cmi_service_open_in_progress = FBE_TRUE;
                            fbe_spinlock_lock(&client_list[client].open_state_lock);
                            client_list[client].open_state &= ~FBE_CMI_OPEN_RECEIVED;
                            client_list[client].open_state |= FBE_CMI_OPEN_ESTABLISHED;  /* mark the communication established */
                            fbe_spinlock_unlock(&client_list[client].open_state_lock);
                            fbe_cmi_send_open_to_peer(client);
                            break; // only send one message at a time.
                        }
                    }
            }
        }else{
            break;
        }
    }

	cmi_service_msg_send_thread_flag = FBE_CMI_MSG_SEND_THREAD_DONE;
    fbe_thread_exit(EMCPAL_STATUS_SUCCESS);
}

/*something went wrong on the other side when it received the message and it had some problem processing it*/

static fbe_status_t fbe_metadata_process_fatal_error_message(fbe_cmi_internal_message_t *cmi_internal_message, fbe_cmi_event_callback_context_t contex)
{
	fbe_status_t	status = FBE_STATUS_GENERIC_FAILURE;
    
	switch (cmi_internal_message->internal_msg_type) {
    case FBE_CMI_INTERNAL_MESSAGE_HANDSHAKE:
        fbe_cmi_trace(FBE_TRACE_LEVEL_WARNING, "%s, other SP can't be accessed, defaulting as PASSIVE\n", __FUNCTION__);
        cmi_service_handshake_set_sp_state(FBE_CMI_STATE_PASSIVE);
        status = FBE_STATUS_OK;
        break;
    case FBE_CMI_INTERNAL_MESSAGE_OPEN:
        fbe_cmi_trace(FBE_TRACE_LEVEL_WARNING, "%s, other SP can't be accessed, drop the open\n", __FUNCTION__);
        cmi_service_open_in_progress = FBE_FALSE;
        status = FBE_STATUS_OK;
        break;
    default:
        fbe_cmi_trace(FBE_TRACE_LEVEL_WARNING, "%s, invalid internal message %d\n", __FUNCTION__, cmi_internal_message->internal_msg_type);
        status = FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}

fbe_bool_t fbe_cmi_is_active_sp(void)
{
    return (current_sp_state == FBE_CMI_STATE_ACTIVE);
}

 
