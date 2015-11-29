#include "csx_ext.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_queue.h"
#include "fbe/fbe_memory.h"
#include "fbe/fbe_winddk.h"
#include "fbe_base_object.h"
#include "fbe_base_service.h"
#include "fbe/fbe_topology_interface.h"
#include "fbe_notification.h"
#include "fbe/fbe_notification_lib.h"


typedef struct fbe_notification_service_s{
	fbe_base_service_t	base_service;
	fbe_queue_head_t	notification_queue_head;
	fbe_spinlock_t		notification_queue_lock;
}fbe_notification_service_t;

/* Declare our service methods */
fbe_status_t fbe_notification_control_entry(fbe_packet_t * packet); 
fbe_service_methods_t fbe_notification_service_methods = {FBE_SERVICE_ID_NOTIFICATION, fbe_notification_control_entry};

/* Forward declaration */
static fbe_status_t notification_register(fbe_packet_t * packet);
static fbe_status_t notification_unregister(fbe_packet_t * packet);

static fbe_notification_service_t notification_service;
static fbe_u64_t				  current_registraction_id = 0;

void
notification_trace(fbe_trace_level_t trace_level,
                   fbe_trace_message_id_t message_id,
                   const char * fmt, ...)
                   __attribute__((format(__printf_func__,3,4)));
void
notification_trace(fbe_trace_level_t trace_level,
                   fbe_trace_message_id_t message_id,
                   const char * fmt, ...)
{
    fbe_trace_level_t default_trace_level;
    fbe_trace_level_t service_trace_level;

    va_list args;

    service_trace_level = default_trace_level = fbe_trace_get_default_trace_level();
    if (fbe_base_service_is_initialized(&notification_service.base_service)) {
        service_trace_level = fbe_base_service_get_trace_level(&notification_service.base_service);
        if (default_trace_level > service_trace_level) {
            service_trace_level = default_trace_level;
        }
    }
    if (trace_level > service_trace_level) {
        return;
    }

    va_start(args, fmt);
    fbe_trace_report(FBE_COMPONENT_TYPE_SERVICE,
                     FBE_SERVICE_ID_NOTIFICATION,
                     trace_level,
                     message_id,
                     fmt, 
                     args);
    va_end(args);
}

static fbe_status_t 
fbe_notification_init(fbe_packet_t * packet)
{
    notification_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                       "%s: entry\n", __FUNCTION__);

	fbe_base_service_set_service_id((fbe_base_service_t *) &notification_service, FBE_SERVICE_ID_NOTIFICATION);

	fbe_spinlock_init(&notification_service.notification_queue_lock);
	fbe_queue_init(&notification_service.notification_queue_head);

	fbe_base_service_init((fbe_base_service_t *) &notification_service);

	fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
	fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
}

static fbe_status_t 
fbe_notification_destroy(fbe_packet_t * packet)
{
    notification_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                       "%s: entry\n", __FUNCTION__);

	fbe_spinlock_lock(&notification_service.notification_queue_lock);
	if(! fbe_queue_is_empty(&notification_service.notification_queue_head)) {
		fbe_spinlock_unlock(&notification_service.notification_queue_lock);

		fbe_base_service_trace((fbe_base_service_t *) &notification_service, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_DESTROY_NON_EMPTY_QUEUE,
								"%s The notification queue is not empty", __FUNCTION__);

		fbe_transport_set_status(packet, FBE_STATUS_BUSY, 0);
		fbe_transport_complete_packet(packet);

		return FBE_STATUS_BUSY;
	}

	fbe_spinlock_unlock(&notification_service.notification_queue_lock);

	fbe_queue_destroy(&notification_service.notification_queue_head);
	fbe_spinlock_destroy(&notification_service.notification_queue_lock);

	fbe_base_service_destroy((fbe_base_service_t *) &notification_service);

	fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
	fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_notification_control_entry(fbe_packet_t * packet)
{    
	fbe_status_t status;
	fbe_payload_control_operation_opcode_t control_code;

	control_code = fbe_transport_get_control_code(packet);
	if(control_code == FBE_BASE_SERVICE_CONTROL_CODE_INIT) {
		status = fbe_notification_init(packet);
		return status;
	}

	if(!fbe_base_service_is_initialized((fbe_base_service_t *) &notification_service)){
		fbe_transport_set_status(packet, FBE_STATUS_NOT_INITIALIZED, 0);
		fbe_transport_complete_packet(packet);
		return FBE_STATUS_NOT_INITIALIZED;
	}

	switch(control_code) {
		case FBE_NOTIFICATION_CONTROL_CODE_REGISTER:
			status = notification_register( packet);
			break;
		case FBE_NOTIFICATION_CONTROL_CODE_UNREGISTER:
			status = notification_unregister( packet);
			break;
		case FBE_BASE_SERVICE_CONTROL_CODE_DESTROY:
			status = fbe_notification_destroy( packet);
			break;

		default:
			status = fbe_base_service_control_entry((fbe_base_service_t*)&notification_service, packet);
			break;
	}

	return status;
}

static fbe_status_t 
notification_register(fbe_packet_t * packet)
{
	fbe_notification_element_t * notification_element = NULL;
	fbe_notification_element_t * current_notification_element = NULL;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL; 

    notification_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                       "%s: entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);   

    fbe_payload_control_get_buffer_length(control_operation, &len); 
	if(len != sizeof(fbe_notification_element_t)){
		notification_trace(FBE_TRACE_LEVEL_ERROR,
		                   FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
		                   "%s: Invalid buffer_len \n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
		fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
	}

	fbe_payload_control_get_buffer(control_operation, &notification_element); 
	if(notification_element == NULL){
		notification_trace(FBE_TRACE_LEVEL_ERROR,
		                   FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
		                   "%s: fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
		fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
	}

	fbe_spinlock_lock(&notification_service.notification_queue_lock);

	current_notification_element = (fbe_notification_element_t *)fbe_queue_front(&notification_service.notification_queue_head);
	while(current_notification_element != NULL) {

        if (current_notification_element == notification_element) {
            /* The notification element matches, don't allow a second registration.
             */
            fbe_spinlock_unlock(&notification_service.notification_queue_lock);
            notification_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                               "%s: duplicate registration element: %p type: 0x%x pkg: %d \n", __FUNCTION__, 
                               notification_element, (fbe_u32_t)notification_element->notification_type,
                               notification_element->targe_package);
            fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_OK;
		}
			
		current_notification_element = (fbe_notification_element_t *)fbe_queue_next(&notification_service.notification_queue_head,
                                                                                    &current_notification_element->queue_element);
	}

	/*for the purpiose of consistency, we generate registration id here even though we don't really use it or need it,
	but not to create any debug conusion we still keep this behvaiour*/
	notification_element->registration_id = current_registraction_id;
    current_registraction_id++;
	notification_element->ref_count = 0;
	fbe_queue_push(&notification_service.notification_queue_head, &notification_element->queue_element);

	fbe_spinlock_unlock(&notification_service.notification_queue_lock);
    notification_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                       "%s: element: %p type: 0x%x pkg: %d id: 0x%x\n", __FUNCTION__, 
                       notification_element, (fbe_u32_t)notification_element->notification_type,
                       notification_element->targe_package, (fbe_u32_t)notification_element->registration_id);
	fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
	fbe_transport_complete_packet(packet);
	return FBE_STATUS_OK;
}

static fbe_status_t 
notification_unregister(fbe_packet_t * packet)
{
	fbe_notification_element_t * notification_element = NULL;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
	fbe_atomic_t	is_locked = 0;

    notification_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                       "%s: entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer_length(control_operation, &len); 
	if(len != sizeof(fbe_notification_element_t)){
		notification_trace(FBE_TRACE_LEVEL_ERROR,
		                   FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
		                   "%s: Invalid buffer_len \n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
		fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
	}

	fbe_payload_control_get_buffer(control_operation, &notification_element); 
	if(notification_element == NULL){
		notification_trace(FBE_TRACE_LEVEL_ERROR,
		                   FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
		                   "%s: fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
		fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
	}
    
    notification_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                       "%s: element: %p type: 0x%x pkg: %d id: 0x%x\n", __FUNCTION__, 
                       notification_element, (fbe_u32_t)notification_element->notification_type,
                       notification_element->targe_package, (fbe_u32_t)notification_element->registration_id);
    
	/*we might be jut calling back to this exact element so we have to make sure it is safe to unregister*/
	do {
		fbe_spinlock_lock(&notification_service.notification_queue_lock);
		fbe_atomic_exchange(&is_locked, notification_element->ref_count);
		if (!is_locked) {
			fbe_queue_remove(&notification_element->queue_element);
		}
		fbe_spinlock_unlock(&notification_service.notification_queue_lock);
		if (is_locked) {
			fbe_thread_delay(100);
		}
	} while (is_locked);

	fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
	fbe_transport_complete_packet(packet);
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_notification_send(fbe_object_id_t object_id, fbe_notification_info_t notification_info)
{
	fbe_notification_element_t * 	notification_element = NULL;
    
	if(!fbe_base_service_is_initialized((fbe_base_service_t *) &notification_service)){
		return FBE_STATUS_NOT_INITIALIZED;
	}

    fbe_spinlock_lock(&notification_service.notification_queue_lock);

	notification_element = (fbe_notification_element_t *)fbe_queue_front(&notification_service.notification_queue_head);
	while(notification_element != NULL) {

        /*filter out noise we don't care about*/
		if ((notification_info.notification_type & notification_element->notification_type) &&
			(notification_element->object_type & notification_info.object_type)) {

            /*Add to the notification info the source package the notification came from*/
			fbe_get_package_id(&notification_info.source_package);

			/*mark in a way we know we can't unregister now and remove this element from the queue*/
			fbe_atomic_increment(&notification_element->ref_count);

			fbe_spinlock_unlock(&notification_service.notification_queue_lock);

			notification_element->notification_function(object_id, notification_info, notification_element->notification_context);

			fbe_spinlock_lock(&notification_service.notification_queue_lock);
			fbe_atomic_decrement(&notification_element->ref_count);/*we can unregister next time we unlock the queue*/
		}
			
		notification_element = (fbe_notification_element_t *)fbe_queue_next(&notification_service.notification_queue_head,
																			&notification_element->queue_element);
	}

	fbe_spinlock_unlock(&notification_service.notification_queue_lock);
	
	return FBE_STATUS_OK;
}



