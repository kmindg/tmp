#include "fbe/fbe_transport.h"
#include "fbe_metadata_cmi.h"
#include "fbe_metadata_private.h"
#include "fbe_cmi.h"
#include "fbe/fbe_base_config.h"
#include "fbe_topology.h"
#include "fbe_metadata_private.h"
#include "fbe/fbe_types.h"

static fbe_atomic_t outstanding_message_counter;

fbe_status_t
fbe_metadata_cmi_send_message(fbe_metadata_cmi_message_t * metadata_cmi_message, void * context)
{
	fbe_atomic_increment(&outstanding_message_counter);

	if(metadata_cmi_message->header.metadata_cmi_message_type == FBE_METADATA_CMI_MESSAGE_TYPE_INVALID){
		metadata_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
					 FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
					 "%s Invalid message type\n", __FUNCTION__);

	}


    fbe_cmi_send_message(FBE_CMI_CLIENT_ID_METADATA, 
                         sizeof(fbe_metadata_cmi_message_t),
                         (fbe_cmi_message_t)metadata_cmi_message,
                         (fbe_cmi_event_callback_context_t)context);

	return FBE_STATUS_OK;
}


/* Forward declarations */
static fbe_status_t fbe_metadata_cmi_callback(fbe_cmi_event_t event, fbe_u32_t cmi_message_length, fbe_cmi_message_t cmi_message,  fbe_cmi_event_callback_context_t context);

static fbe_status_t fbe_metadata_cmi_message_transmitted(fbe_metadata_cmi_message_t *metadata_cmi_msg, 
                                                           fbe_cmi_event_callback_context_t context);

static fbe_status_t fbe_metadata_cmi_peer_lost(fbe_metadata_cmi_message_t *metadata_cmi_msg, 
                                                    fbe_cmi_event_callback_context_t context);

static fbe_status_t fbe_metadata_cmi_message_received(fbe_metadata_cmi_message_t * metadata_cmi_msg, fbe_u32_t cmi_message_length);

static fbe_status_t fbe_metadata_cmi_fatal_error_message(fbe_metadata_cmi_message_t *metadata_cmi_msg, 
                                                           fbe_cmi_event_callback_context_t context);

static fbe_status_t  fbe_metadata_cmi_no_peer_message(fbe_metadata_cmi_message_t *metadata_cmi_msg, 
                                                            fbe_cmi_event_callback_context_t context);

fbe_status_t
fbe_metadata_cmi_init(void)
{
    fbe_status_t status;
	outstanding_message_counter = 0;
    status = fbe_cmi_register(FBE_CMI_CLIENT_ID_METADATA, fbe_metadata_cmi_callback, NULL);
    return status;
}

fbe_status_t
fbe_metadata_cmi_destroy(void)
{
    fbe_status_t status;
    status = fbe_cmi_unregister(FBE_CMI_CLIENT_ID_METADATA);

	if(outstanding_message_counter != 0){
		metadata_trace(FBE_TRACE_LEVEL_WARNING,
					 FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
					 "%s outstanding_message_counter %lld != 0\n", __FUNCTION__, (long long)outstanding_message_counter);
	}
    return status;
}

/*
  Note: cmi_message_length is only valid for received msg notifications, all other cases will have zero passed in. 
*/
static fbe_status_t 
fbe_metadata_cmi_callback(fbe_cmi_event_t event, fbe_u32_t cmi_message_length, fbe_cmi_message_t cmi_message,  fbe_cmi_event_callback_context_t context)
{
    fbe_metadata_cmi_message_t * metadata_cmi_msg = NULL;
    fbe_status_t status = FBE_STATUS_OK;

    metadata_cmi_msg = (fbe_metadata_cmi_message_t*)cmi_message;

    switch (event) {
        case FBE_CMI_EVENT_MESSAGE_TRANSMITTED:
            status = fbe_metadata_cmi_message_transmitted(metadata_cmi_msg, context);
            break;
        case FBE_CMI_EVENT_SP_CONTACT_LOST:
            status = fbe_metadata_cmi_peer_lost(metadata_cmi_msg, context);
            break;
        case FBE_CMI_EVENT_MESSAGE_RECEIVED:
            status = fbe_metadata_cmi_message_received(metadata_cmi_msg, cmi_message_length);
            break;
        case FBE_CMI_EVENT_FATAL_ERROR:
            status = fbe_metadata_cmi_fatal_error_message(metadata_cmi_msg, context);
            break;
		case FBE_CMI_EVENT_PEER_NOT_PRESENT:
		case FBE_CMI_EVENT_PEER_BUSY:
            status = fbe_metadata_cmi_no_peer_message(metadata_cmi_msg, context);
            break;
		default:
			metadata_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
						   FBE_TRACE_MESSAGE_ID_INFO,
						   "%s Illegal CMI opcode: 0x%X\n", __FUNCTION__, event);
            break;
    }
    
    return status;
}


static fbe_status_t    
fbe_metadata_cmi_message_transmitted(fbe_metadata_cmi_message_t *metadata_cmi_msg, fbe_cmi_event_callback_context_t context)
{
	fbe_packet_t * packet = NULL;

	fbe_atomic_decrement(&outstanding_message_counter);

	if(context == NULL){ /* The caller is not interested in completion */
		return FBE_STATUS_OK; 
	}

	switch(metadata_cmi_msg->header.metadata_cmi_message_type){
		case FBE_METADATA_CMI_MESSAGE_TYPE_STRIPE_WRITE_LOCK:
		case FBE_METADATA_CMI_MESSAGE_TYPE_STRIPE_READ_LOCK:
		case FBE_METADATA_CMI_MESSAGE_TYPE_STRIPE_WRITE_GRANT:
		case FBE_METADATA_CMI_MESSAGE_TYPE_STRIPE_READ_GRANT:
		case FBE_METADATA_CMI_MESSAGE_TYPE_STRIPE_RELEASE:
		case FBE_METADATA_CMI_MESSAGE_TYPE_STRIPE_ABORTED:
			fbe_metadata_stripe_lock_message_event(FBE_CMI_EVENT_MESSAGE_TRANSMITTED, metadata_cmi_msg, context);
			break;
		default:
			packet = (fbe_packet_t *)context;
			fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
			//fbe_transport_complete_packet(packet);
			fbe_transport_run_queue_push_packet(packet,FBE_TRANSPORT_RQ_METHOD_SAME_CORE);
	}
	 
    return FBE_STATUS_OK; 
}

static fbe_status_t 
fbe_metadata_cmi_peer_lost(fbe_metadata_cmi_message_t *metadata_cmi_msg, fbe_cmi_event_callback_context_t context)
{    
	fbe_status_t status = FBE_STATUS_OK;
    fbe_cmi_sp_state_t  state  = FBE_CMI_STATE_INVALID;

    metadata_trace(FBE_TRACE_LEVEL_INFO,
                 FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                 "%s entry\n", __FUNCTION__);

	/* If current SP is in service mode, don't switch the element state to active */
    fbe_cmi_get_current_sp_state(&state);
    if (state == FBE_CMI_STATE_SERVICE_MODE) {
        metadata_trace(FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s: current SP is degraded mode, don't switch elements to active\n", 
                          __FUNCTION__); 
        return FBE_STATUS_OK;
    }

	/*this is an asynchronous event about the sp dying and not a reply about CMI unable to send the message
	because the SP is dead*/
	metadata_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO, "%s, Peer died, setting all elements to ACTIVE\n", __FUNCTION__);

	status = fbe_metadata_switch_all_elements_to_state(FBE_METADATA_ELEMENT_STATE_ACTIVE);

	fbe_metadata_stripe_lock_peer_lost();
    return FBE_STATUS_OK; 
}

static fbe_status_t 
fbe_metadata_cmi_message_received(fbe_metadata_cmi_message_t *metadata_cmi_msg, fbe_u32_t cmi_message_length)
{
	fbe_metadata_element_t * metadata_element = NULL;
	fbe_object_id_t element_object_id;
	fbe_bool_t send_peer_alive_event = FBE_FALSE;
    fbe_u32_t rcv_peer_metadata_memory_size = 0;

    metadata_trace(FBE_TRACE_LEVEL_DEBUG_LOW,
                 FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                 "%s entry\n", __FUNCTION__);

    /* Find metadata element by object id on the metadata element queue to decide whether the object
     * on this side has been created and registered*/
    fbe_metadata_element_queue_lock();
    fbe_metadata_get_element_by_object_id( metadata_cmi_msg->header.object_id, &metadata_element);
    fbe_metadata_element_queue_unlock();
    if(NULL == metadata_element)
    {
        /*This usually happens when:
         *(1) The object has not finished its initialization, where the METADATA_ELEMENT_INIT
         *    condition has not run
         *(2) The object is doing reinitialization, where the CLEAR_NP condition has finished run.
         */
        metadata_trace(FBE_TRACE_LEVEL_WARNING,
                FBE_TRACE_MESSAGE_ID_INFO,
                "mdata_cmi_msg_recv: the specified object 0x%x is not registered in MD SRV. cmi_type: 0x%x\n", 
                metadata_cmi_msg->header.object_id, metadata_cmi_msg->header.metadata_cmi_message_type);
        
        return FBE_STATUS_OK; /* This OK is for received message */
    }

    if(metadata_cmi_msg->header.metadata_element_receiver != 0)
    {
        metadata_element = (fbe_metadata_element_t *)(csx_ptrhld_t)metadata_cmi_msg->header.metadata_element_receiver;
        element_object_id = fbe_base_config_metadata_element_to_object_id(metadata_element);
        if(element_object_id != metadata_cmi_msg->header.object_id){
            metadata_trace(FBE_TRACE_LEVEL_WARNING,
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "mdata_cmi_msg_recv Invalid mdata_elem_receiver 0x%llX has obj_id %d instead %d\n", 
                (unsigned long long)metadata_cmi_msg->header.metadata_element_receiver, 
                element_object_id,
                metadata_cmi_msg->header.object_id);
            return FBE_STATUS_OK;           
        }
    }

    fbe_atomic_32_or(&metadata_element->attributes, FBE_METADATA_ELEMENT_ATTRIBUTES_CMI_MSG_PROCESSING);
    

    if((metadata_element->metadata_element_state == FBE_METADATA_ELEMENT_STATE_INVALID) ||
       fbe_metadata_element_is_cmi_disabled(metadata_element)){ /* Not initialized or already destroyed */
        fbe_atomic_32_and(&metadata_element->attributes, ~FBE_METADATA_ELEMENT_ATTRIBUTES_CMI_MSG_PROCESSING);
        return FBE_STATUS_OK; /* This OK is for received message */
    }

	/* Update peer address */
	if(metadata_element->peer_metadata_element_ptr == 0){ /* We see it for the first time */
		metadata_element->peer_metadata_element_ptr = metadata_cmi_msg->header.metadata_element_sender;
		send_peer_alive_event = FBE_TRUE;
	}else if(metadata_element->peer_metadata_element_ptr != metadata_cmi_msg->header.metadata_element_sender){
			metadata_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
					FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
					"mdata_cmi_msg_recv Invalid mdata_elem_sender 0x%llX expect 0x%llX\n", 
																								(unsigned long long)metadata_cmi_msg->header.metadata_element_sender, 
																								(unsigned long long)metadata_element->peer_metadata_element_ptr);
			return FBE_STATUS_OK; /* This OK is for received message */
	}


	
	switch(metadata_cmi_msg->header.metadata_cmi_message_type){
		case FBE_METADATA_CMI_MESSAGE_TYPE_MEMORY_UPDATE:
				fbe_spinlock_lock(&metadata_element->metadata_element_lock);
                /*may receive different version metadata memory update when SEP upgrading, handle 
                  two cases here:
                  1) memory update size > my data structure size,  cut the update to my data structure
                     size;
                  2) memory update size <= my data structure size, apply the whole update
                  */
                /* doing zeroing here is very bad because it creates a window where all peer flags are cleared.
                 *       fbe_zero_memory(metadata_element->metadata_memory_peer.memory_ptr, metadata_element->metadata_memory.memory_size);
                 */
                rcv_peer_metadata_memory_size = ((fbe_sep_version_header_t *)metadata_cmi_msg->msg.memory_update_data)->size;
                if (rcv_peer_metadata_memory_size < metadata_element->metadata_memory.memory_size) {
                    fbe_copy_memory(metadata_element->metadata_memory_peer.memory_ptr,
                                    metadata_cmi_msg->msg.memory_update_data,
                                    rcv_peer_metadata_memory_size);
                } else{
                    fbe_copy_memory(metadata_element->metadata_memory_peer.memory_ptr,
                                    metadata_cmi_msg->msg.memory_update_data,
                                    metadata_element->metadata_memory.memory_size);
                }

				fbe_spinlock_unlock(&metadata_element->metadata_element_lock);

				if(send_peer_alive_event == FBE_TRUE){				
					fbe_topology_send_event(metadata_cmi_msg->header.object_id, FBE_EVENT_TYPE_PEER_OBJECT_ALIVE, NULL);
				}

				/* Send event to object_id from the message */
				fbe_topology_send_event(metadata_cmi_msg->header.object_id, FBE_EVENT_TYPE_PEER_MEMORY_UPDATE, NULL);
			break;
		case FBE_METADATA_CMI_MESSAGE_TYPE_NONPAGED_WRITE:	
        		fbe_metadata_nonpaged_cmi_dispatch(metadata_element, metadata_cmi_msg);

				/* Send event to object_id from the message */
				fbe_topology_send_event(metadata_cmi_msg->header.object_id, FBE_EVENT_TYPE_PEER_NONPAGED_WRITE, NULL);
			break;
        case FBE_METADATA_CMI_MESSAGE_TYPE_NONPAGED_POST_PERSIST:
                fbe_metadata_nonpaged_cmi_dispatch(metadata_element, metadata_cmi_msg);
            break;
		case FBE_METADATA_CMI_MESSAGE_TYPE_FORCE_SET_CHECKPOINT:
				fbe_metadata_nonpaged_cmi_dispatch(metadata_element, metadata_cmi_msg);

				/* Send event to object_id from the message */
				fbe_topology_send_event(metadata_cmi_msg->header.object_id, FBE_EVENT_TYPE_PEER_NONPAGED_CHKPT_UPDATE, NULL);
			break;

		case FBE_METADATA_CMI_MESSAGE_TYPE_SET_BITS:
		case FBE_METADATA_CMI_MESSAGE_TYPE_CLEAR_BITS:
		case FBE_METADATA_CMI_MESSAGE_TYPE_SET_CHECKPOINT:
		case FBE_METADATA_CMI_MESSAGE_TYPE_INCR_CHECKPOINT:
				fbe_metadata_nonpaged_cmi_dispatch(metadata_element, metadata_cmi_msg);

				/* Send event to object_id from the message */
				fbe_topology_send_event(metadata_cmi_msg->header.object_id, FBE_EVENT_TYPE_PEER_NONPAGED_CHANGED, NULL);
			break;

		case FBE_METADATA_CMI_MESSAGE_TYPE_STRIPE_LOCK_START:
		case FBE_METADATA_CMI_MESSAGE_TYPE_STRIPE_LOCK_STOP:
		case FBE_METADATA_CMI_MESSAGE_TYPE_STRIPE_READ_LOCK:
		case FBE_METADATA_CMI_MESSAGE_TYPE_STRIPE_WRITE_LOCK:
		case FBE_METADATA_CMI_MESSAGE_TYPE_STRIPE_READ_GRANT:
		case FBE_METADATA_CMI_MESSAGE_TYPE_STRIPE_WRITE_GRANT:
		case FBE_METADATA_CMI_MESSAGE_TYPE_STRIPE_RELEASE:
		case FBE_METADATA_CMI_MESSAGE_TYPE_STRIPE_ABORTED:
				fbe_metadata_stripe_lock_cmi_dispatch(metadata_element, metadata_cmi_msg, cmi_message_length);
			break;
		default:
			metadata_trace(FBE_TRACE_LEVEL_ERROR,
							FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
							"mdata_cmi_msg_recv Uknown message type %d\n", metadata_cmi_msg->header.metadata_cmi_message_type);

			break;

	}

    fbe_atomic_32_and(&metadata_element->attributes, ~FBE_METADATA_ELEMENT_ATTRIBUTES_CMI_MSG_PROCESSING);
    
    return FBE_STATUS_OK;
}

static fbe_status_t 
fbe_metadata_cmi_fatal_error_message(fbe_metadata_cmi_message_t *metadata_cmi_msg, fbe_cmi_event_callback_context_t context)
{
    /*process a message that say not only we could not send the message but the whole CMI bus is bad*/
	fbe_packet_t * packet = NULL;

	fbe_atomic_decrement(&outstanding_message_counter);

	if(context == NULL){ /* The caller is not interested in completion */
		return FBE_STATUS_OK; 
	}

	switch(metadata_cmi_msg->header.metadata_cmi_message_type){
		case FBE_METADATA_CMI_MESSAGE_TYPE_STRIPE_WRITE_LOCK:
		case FBE_METADATA_CMI_MESSAGE_TYPE_STRIPE_READ_LOCK:
		case FBE_METADATA_CMI_MESSAGE_TYPE_STRIPE_WRITE_GRANT:
		case FBE_METADATA_CMI_MESSAGE_TYPE_STRIPE_READ_GRANT:
		case FBE_METADATA_CMI_MESSAGE_TYPE_STRIPE_RELEASE:
		case FBE_METADATA_CMI_MESSAGE_TYPE_STRIPE_ABORTED:
			fbe_metadata_stripe_lock_message_event(FBE_CMI_EVENT_FATAL_ERROR, metadata_cmi_msg, context);
			break;
		default:
			metadata_trace(FBE_TRACE_LEVEL_WARNING,
							FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
							"CMI: Fatal error Packet %p \n", context);

			packet = (fbe_packet_t *)context;
			fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
			fbe_transport_complete_packet(packet);
	}

    return FBE_STATUS_OK;
}


static fbe_status_t 
fbe_metadata_cmi_no_peer_message(fbe_metadata_cmi_message_t *metadata_cmi_msg, fbe_cmi_event_callback_context_t context)
{
	fbe_packet_t * packet = NULL;
	
	fbe_atomic_decrement(&outstanding_message_counter);

	if(context == NULL){ /* The caller is not interested in completion */
		return FBE_STATUS_OK; 
	}

	switch(metadata_cmi_msg->header.metadata_cmi_message_type){
		case FBE_METADATA_CMI_MESSAGE_TYPE_STRIPE_WRITE_LOCK:
		case FBE_METADATA_CMI_MESSAGE_TYPE_STRIPE_READ_LOCK:
		case FBE_METADATA_CMI_MESSAGE_TYPE_STRIPE_WRITE_GRANT:
		case FBE_METADATA_CMI_MESSAGE_TYPE_STRIPE_READ_GRANT:
		case FBE_METADATA_CMI_MESSAGE_TYPE_STRIPE_RELEASE:
		case FBE_METADATA_CMI_MESSAGE_TYPE_STRIPE_ABORTED:
			fbe_metadata_stripe_lock_message_event(FBE_CMI_EVENT_PEER_NOT_PRESENT, metadata_cmi_msg, context);
			break;
		default:
			packet = (fbe_packet_t *)context;
			fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
            fbe_transport_run_queue_push_packet(packet,FBE_TRANSPORT_RQ_METHOD_SAME_CORE);
    }

    return FBE_STATUS_OK; 
}



