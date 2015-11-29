/***************************************************************************
 * Copyright (C) EMC Corporation 2001-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *  fbe_api_transport_client_notification.c
 ***************************************************************************
 *
 *  Description
 *      Takes care of notification processing of the client side
 ****************************************************************************/

#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_trace_interface.h"
#include "fbe/fbe_library_interface.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_transport.h"
#include "fbe/fbe_api_transport_packet_interface.h"
#include "fbe_api_transport_packet_interface_private.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_package.h"
#include "fbe/fbe_notification_interface.h"
#include "fbe/fbe_physical_package.h"
#include "fbe/fbe_sep.h"
#include "fbe/fbe_esp.h"
#include "fbe/fbe_neit_package.h"

/**************************************
				Local variables
**************************************/
typedef struct fbe_api_transport_package_connection_info_s{
	fbe_package_id_t	package_id;
    fbe_u32_t			get_event_ioctl;
	fbe_u32_t			register_for_event_ioctl;
	fbe_u32_t			unregister_from_event_ioctl;
}fbe_api_transport_package_connection_info_t;

static fbe_api_transport_package_connection_info_t	fbe_transport_packages_handles[FBE_PACKAGE_ID_LAST + 1] =
{
    {FBE_PACKAGE_ID_INVALID, NULL, NULL, NULL},

    {FBE_PACKAGE_ID_PHYSICAL,
	 FBE_PHYSICAL_PACKAGE_NOTIFICATION_GET_EVENT_IOCTL,
     FBE_PHYSICAL_PACKAGE_REGISTER_APP_NOTIFICATION_IOCTL,
     FBE_PHYSICAL_PACKAGE_UNREGISTER_APP_NOTIFICATION_IOCTL},

    {FBE_PACKAGE_ID_NEIT,
      FBE_NEIT_NOTIFICATION_GET_EVENT_IOCTL,
      FBE_NEIT_REGISTER_APP_NOTIFICATION_IOCTL,
      FBE_NEIT_UNREGISTER_APP_NOTIFICATION_IOCTL},

    {FBE_PACKAGE_ID_SEP_0,
	 FBE_SEP_NOTIFICATION_GET_EVENT_IOCTL,
	 FBE_SEP_REGISTER_APP_NOTIFICATION_IOCTL,
	 FBE_SEP_UNREGISTER_APP_NOTIFICATION_IOCTL},

	{FBE_PACKAGE_ID_ESP, 
	 FBE_ESP_NOTIFICATION_GET_EVENT_IOCTL,
	 FBE_ESP_REGISTER_APP_NOTIFICATION_IOCTL,
	 FBE_ESP_UNREGISTER_APP_NOTIFICATION_IOCTL},
	
	{FBE_PACKAGE_ID_LAST,NULL, NULL, NULL}

};


static fbe_spinlock_t							fbe_transport_event_notifications_queue_lock;
static fbe_queue_head_t	   						fbe_transport_event_notifications_queue_head;
static application_id_t							fbe_transport_event_registration_id[FBE_TRANSPORT_LAST_CONNECTION_TARGET][FBE_PACKAGE_ID_LAST];
static notification_data_t						fbe_transport_notification_data[FBE_TRANSPORT_LAST_CONNECTION_TARGET][FBE_PACKAGE_ID_LAST];
static fbe_semaphore_t							unregistered_confirmation[FBE_TRANSPORT_LAST_CONNECTION_TARGET][FBE_PACKAGE_ID_LAST];
static fbe_bool_t								initialized = FBE_FALSE;
static fbe_u32_t								init_count[FBE_TRANSPORT_LAST_CONNECTION_TARGET] = {0,0,0,0};
static fbe_u64_t				  				fbe_transport_current_registraction_id = 0;
static fbe_bool_t								client_notification_registered[FBE_PACKAGE_ID_LAST] = {0};


/*******************************************
				Local functions
********************************************/
static fbe_status_t fbe_api_transport_register_for_event_notification (fbe_package_id_t package_id, fbe_transport_connection_target_t connect_to_sp);
static fbe_status_t fbe_api_transport_unregister_for_event_notification (fbe_package_id_t package_id, fbe_transport_connection_target_t connect_to_sp);
static fbe_status_t fbe_api_transport_register_for_event_notification_completion (fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t fbe_api_transport_event_notification (fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t fbe_api_transport_transport_control_map_ioctl_code_to_package(fbe_payload_control_operation_opcode_t control_code, fbe_package_id_t *package_id);
static fbe_status_t fbe_api_transport_get_next_event (fbe_package_id_t package_id, fbe_transport_connection_target_t connect_to_sp);
static void wait_for_all_events_unregistration(fbe_transport_connection_target_t connect_to_sp);

/************************************************************************************************************/

fbe_status_t FBE_API_CALL fbe_api_transport_control_init_client_control (fbe_transport_connection_target_t connect_to_sp)
{
    fbe_status_t				status = FBE_STATUS_GENERIC_FAILURE;
	fbe_package_notification_id_mask_t		fbe_api_package_id_mask = FBE_PACKAGE_NOTIFICATION_ID_INVALID;
	fbe_package_notification_id_mask_t		package_id = 0x1;/*will always be the first package*/
	fbe_u32_t					package_count = 0x1;
	fbe_package_id_t			fbe_package_id = 0x1;/*start with the first one, no matter what it is*/


    /*this part we do only once*/
	if (!initialized) {
        fbe_queue_init (&fbe_transport_event_notifications_queue_head);
		fbe_spinlock_init(&fbe_transport_event_notifications_queue_lock);
	}

    initialized++;

	init_count[connect_to_sp]++;

	/*for each sp mode, we might have a different set of packages we want to register with.
	for example, if it's SPA or SPB, we want to register with all, but if it's the ESP package server
	we care only about it*/
	status = fbe_api_transport_get_package_mask_for_sever_target(connect_to_sp, &fbe_api_package_id_mask);
	if (status != FBE_STATUS_OK) {
		fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s, failed to get package mask for target: %d\n", __FUNCTION__, connect_to_sp);
		return status;
	}

	while (package_count < FBE_PACKAGE_ID_LAST) {
		if ((package_id & fbe_api_package_id_mask)) {
				fbe_notification_convert_package_bitmap_to_fbe_package(package_id, &fbe_package_id);
				if (fbe_transport_packages_handles[fbe_package_id].register_for_event_ioctl != NULL) {
					status = fbe_api_transport_register_for_event_notification (fbe_package_id, connect_to_sp);
					if (status != FBE_STATUS_OK) {
						package_id <<= 1;/*go to next one*/
						package_count++;
						continue;/*if we can't initialize, we don't care, the package might not be there*/
					}else{
						client_notification_registered[fbe_package_id] = FBE_TRUE;
						/*set up a semaphore in a table that would be cleared only after the package notification unregistration is finished
						this way we know we can get out in the destroy function*/
						fbe_semaphore_init(&unregistered_confirmation[connect_to_sp][fbe_package_id], 0, 1);
					}
			}
		}

		package_id <<= 1;/*go to next one*/
		package_count++;
	}

	return FBE_STATUS_OK;
}

void fbe_api_transport_control_destroy_client_control (fbe_transport_connection_target_t connect_to_sp)
{
	fbe_package_id_t			package_id = FBE_PACKAGE_ID_INVALID;
	fbe_status_t				status[FBE_PACKAGE_ID_LAST] = {FBE_STATUS_GENERIC_FAILURE};
	
	init_count[connect_to_sp]--;
	if (init_count[connect_to_sp] == 0) {
        /*we unregister ourselvs to get notifications packaets from the packages */
        for (package_id = FBE_PACKAGE_ID_INVALID; package_id < FBE_PACKAGE_ID_LAST; package_id++) {
            if ((fbe_transport_packages_handles[package_id].register_for_event_ioctl != NULL) && (client_notification_registered[package_id])) {
                EmcutilSleep(1);
                status[package_id] = fbe_api_transport_unregister_for_event_notification (package_id, connect_to_sp);
            }
        }

        for (package_id = FBE_PACKAGE_ID_INVALID; package_id < FBE_PACKAGE_ID_LAST; package_id++)
        {
            /* unregister notification.. */
            if ((fbe_transport_packages_handles[package_id].register_for_event_ioctl != NULL) && (client_notification_registered[package_id]))
            {
                if(status[package_id] == FBE_STATUS_OK)
                {
                    fbe_semaphore_wait(&unregistered_confirmation[connect_to_sp][package_id], NULL);
                }

                fbe_semaphore_destroy(&unregistered_confirmation[connect_to_sp][package_id]);

                /* Unregister the package if the status is a failure or timeout. */
                if(status[package_id] != FBE_STATUS_OK)
                {
                    client_notification_registered[package_id] = FBE_FALSE;
                }
            }
        }
    }
    initialized--;

    if (initialized == 0)
    {
        fbe_queue_destroy (&fbe_transport_event_notifications_queue_head);
        fbe_spinlock_destroy(&fbe_transport_event_notifications_queue_lock);
    }
}

fbe_status_t fbe_api_transport_register_notification_callback(fbe_packet_t *packet)
{
	fbe_notification_element_t *		notification_element;
	fbe_u32_t							buffer_size = 0;
    fbe_payload_control_operation_t *	control_operation = NULL;
    
	fbe_api_transport_get_control_operation(packet, &control_operation);
	fbe_payload_control_get_buffer_length (control_operation, &buffer_size);
  
	if (buffer_size != sizeof(fbe_notification_element_t)) {
		fbe_transport_set_status (packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet (packet);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	fbe_payload_control_get_buffer (control_operation, &notification_element);

	if (notification_element == NULL) {
		fbe_transport_set_status (packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet (packet);
		return FBE_STATUS_GENERIC_FAILURE;
	}

    /*update the package portion of the notification, we care about it and not the user*/
	fbe_transport_get_package_id(packet, &notification_element->targe_package);

    /*put the notification element on the queue so we can call it once needed*/
	fbe_spinlock_lock (&fbe_transport_event_notifications_queue_lock);
	notification_element->registration_id = fbe_transport_current_registraction_id;
	fbe_transport_current_registraction_id++;
	fbe_queue_push(&fbe_transport_event_notifications_queue_head, &notification_element->queue_element);
	fbe_spinlock_unlock (&fbe_transport_event_notifications_queue_lock);

    /*complete the packet*/
	fbe_transport_set_status (packet, FBE_STATUS_OK ,0);
    fbe_transport_complete_packet (packet);

	return FBE_STATUS_OK;

}

fbe_status_t fbe_api_transport_unregister_notification_callback(fbe_packet_t *packet)
{
	fbe_notification_element_t *		notification_element = NULL;
	fbe_u32_t							buffer_size = 0; 
	fbe_notification_element_t *		registered_notification_element = NULL;
	fbe_queue_element_t *				queue_element = NULL;
	fbe_queue_element_t *				next_element = NULL;
	fbe_bool_t							element_found = FALSE;
    fbe_payload_control_operation_t *	control_operation = NULL;

    if (!initialized){
        fbe_transport_set_status (packet, FBE_STATUS_OK ,0);
        fbe_transport_complete_packet (packet);
        return FBE_STATUS_OK;
    }

    fbe_api_transport_get_control_operation(packet, &control_operation);
	fbe_payload_control_get_buffer_length (control_operation, &buffer_size);

	if (buffer_size != sizeof(fbe_notification_element_t)) {
		fbe_transport_set_status (packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet (packet);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	fbe_payload_control_get_buffer (control_operation, &notification_element);

	if (notification_element == NULL) {
		fbe_transport_set_status (packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet (packet);
		return FBE_STATUS_GENERIC_FAILURE;
	}

	fbe_spinlock_lock (&fbe_transport_event_notifications_queue_lock);
	queue_element = fbe_queue_front(&fbe_transport_event_notifications_queue_head);

	while((queue_element != NULL) && (queue_element != &fbe_transport_event_notifications_queue_head)){
		registered_notification_element = (fbe_notification_element_t *)queue_element;
		/*look for the one we need to take out*/
		if ((registered_notification_element->notification_function == notification_element->notification_function) &&
			(registered_notification_element->registration_id == notification_element->registration_id)) {
			fbe_queue_remove (queue_element);
			element_found = TRUE;
			break;
		}

		next_element = queue_element->next;
		queue_element = next_element;

	}
	fbe_spinlock_unlock (&fbe_transport_event_notifications_queue_lock);

	if (!element_found) {
		fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s:got an unregister request from a module that is not even registered\n", __FUNCTION__); 
		fbe_transport_set_status (packet, FBE_STATUS_GENERIC_FAILURE, 0);
		/*fbe_transport_increment_stack_level (packet);*/
		fbe_transport_complete_packet (packet);
		return FBE_STATUS_GENERIC_FAILURE;

	}
    
    /*complete the packet*/
	fbe_transport_set_status (packet, FBE_STATUS_OK ,0);
    fbe_transport_complete_packet (packet);

	return FBE_STATUS_OK;

}

static fbe_status_t fbe_api_transport_register_for_event_notification (fbe_package_id_t package_id, fbe_transport_connection_target_t connect_to_sp)
{
    fbe_package_register_app_notify_t	register_app;
	fbe_status_t 						status = FBE_STATUS_GENERIC_FAILURE;
	fbe_packet_t * 						packet = NULL;
    fbe_payload_ex_t *					payload = NULL;
	fbe_payload_control_operation_t *	control_operation = NULL;
    
	/* Allocate packet.*/
	packet = fbe_api_get_contiguous_packet();
	if (packet == NULL) {
		fbe_api_trace(FBE_TRACE_LEVEL_ERROR,"%s: unable to allocate memory for packet\n", __FUNCTION__); 
		return FBE_STATUS_GENERIC_FAILURE;
	}

    payload = fbe_transport_get_payload_ex (packet);
    control_operation = fbe_payload_ex_allocate_control_operation(payload);

	fbe_payload_control_build_operation (control_operation,
										 fbe_transport_packages_handles[package_id].register_for_event_ioctl,
										 &register_app,
										 sizeof(fbe_package_register_app_notify_t));

    /* Set packet address.*/
	fbe_transport_set_address(packet,
							  package_id,
							  FBE_SERVICE_ID_SIM_SERVER,
							  FBE_CLASS_ID_INVALID,
							  FBE_OBJECT_ID_INVALID); 

	fbe_transport_set_packet_attr(packet, FBE_PACKET_FLAG_NO_ATTRIB);

	fbe_transport_set_sync_completion_type(packet, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);
	status = fbe_api_common_send_control_packet_to_driver(packet);

	if (status != FBE_STATUS_OK && status != FBE_STATUS_PENDING){
		/* Only trace when it is an error we do not expect.*/
		 fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s [%d] error in sending packet, status:%d\n", __FUNCTION__, package_id, status);
		 fbe_api_return_contiguous_packet(packet);
		 return status;

	}

	/* Wait for the callback.  Our completion function is always called.*/
	fbe_transport_wait_completion(packet);

	/* Save the status for returning.*/
	status = fbe_transport_get_status_code(packet);
	if (status != FBE_STATUS_OK && status != FBE_STATUS_PENDING){
		/* Only trace when it is an error we do not expect.*/
		 fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s [%d] error in received packet, status:%d\n", __FUNCTION__, package_id, status);
		 fbe_api_return_contiguous_packet(packet);
		 return status;
	}

	fbe_transport_event_registration_id[connect_to_sp][package_id] = register_app.application_id;

	fbe_api_return_contiguous_packet(packet);

	/*after we registered, we send a packet to the package that will pend on
	a queue and be ready to return to us once there is an event*/

    status = fbe_api_transport_get_next_event(package_id, connect_to_sp);
	
	if (status != FBE_STATUS_OK) {
		return FBE_STATUS_GENERIC_FAILURE;
	}

	return FBE_STATUS_OK;
}

static fbe_status_t fbe_api_transport_get_next_event (fbe_package_id_t package_id, fbe_transport_connection_target_t connect_to_sp)
{
    fbe_status_t 						status = FBE_STATUS_GENERIC_FAILURE;
	fbe_packet_t * 						packet = NULL;
    fbe_payload_ex_t *					payload = NULL;
	fbe_payload_control_operation_t *	control_operation = NULL;
	
	fbe_transport_notification_data[connect_to_sp][package_id].context = connect_to_sp;
	fbe_transport_notification_data[connect_to_sp][package_id].application_id = fbe_transport_event_registration_id[connect_to_sp][package_id];

    /* Allocate packet.*/
	packet = fbe_api_get_contiguous_packet();
	if (packet == NULL) {
		fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: unable to allocate memory for packet\n", __FUNCTION__); 
		return FBE_STATUS_GENERIC_FAILURE;
	}

    payload = fbe_transport_get_payload_ex (packet);
    control_operation = fbe_payload_ex_allocate_control_operation(payload);

	fbe_payload_control_build_operation (control_operation,
										 fbe_transport_packages_handles[package_id].get_event_ioctl,
										 &fbe_transport_notification_data[connect_to_sp][package_id],
										 sizeof(notification_data_t));

	fbe_transport_set_completion_function (packet, fbe_api_transport_event_notification,  (void *)connect_to_sp);

	/* Set packet address.*/
	fbe_transport_set_address(packet,
							  package_id,
							  FBE_SERVICE_ID_SIM_SERVER,
							  FBE_CLASS_ID_INVALID,
							  FBE_OBJECT_ID_INVALID); 

    fbe_transport_set_packet_attr(packet, FBE_PACKET_FLAG_NO_ATTRIB);

	/* send the notification packet straight to the SP */
	status = fbe_api_transport_send_client_notification_packet_to_targeted_server(packet,connect_to_sp);
	if (status != FBE_STATUS_OK && status != FBE_STATUS_PENDING){
		/* Only trace when it is an error we do not expect.*/
		 fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s error in sending packet, status:%d\n", __FUNCTION__, status);  
		 fbe_api_return_contiguous_packet(packet);
		 return status;
	}
    
	return FBE_STATUS_OK;
    

}

/*this function is called when an event happens on the server side(i.e. other executable)*/
static fbe_status_t fbe_api_transport_event_notification (fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
	fbe_queue_element_t * 					queue_element = NULL;
	fbe_queue_element_t * 					next_element = NULL;
	fbe_notification_element_t *			notification_element;
	fbe_package_id_t 						package_id = FBE_PACKAGE_ID_INVALID;
	fbe_status_t							packet_status = FBE_STATUS_GENERIC_FAILURE;
	fbe_transport_connection_target_t 	connect_to_sp = (fbe_transport_connection_target_t)context;
    
	/*first of all we need to get the notification data, since we have one per package which is statically allocates
	we just need the pacakge id*/
	fbe_transport_get_package_id(packet, &package_id);
    
	/*we also need to check the status, if it's FBE_STATUS_CANCELED, it means we get it as a reqponse to our
	own request to unregister, in this case, we free a semaphore that indicates we are ready to exit, but only when the unregister
	packet completes, not this one*/
	packet_status = fbe_transport_get_status_code(packet);
	if (packet_status == FBE_STATUS_CANCELED) {
		fbe_api_return_contiguous_packet(packet);
		return FBE_STATUS_OK;
	}

	/*we will go over all the completion functions and send them the information we got*/
    fbe_spinlock_lock(&fbe_transport_event_notifications_queue_lock);
	if (!fbe_queue_is_empty(&fbe_transport_event_notifications_queue_head)) {
		queue_element = fbe_queue_front(&fbe_transport_event_notifications_queue_head);
		
		while((queue_element != NULL) && (queue_element != &fbe_transport_event_notifications_queue_head)){
	
			notification_element = (fbe_notification_element_t *)queue_element;

			/*we care only about notifications the user requested from this package and we don't want to send notification to eveything on the queueu*/
			if (notification_element->targe_package == package_id) {
				fbe_api_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, " %s, Notification received from Server. Found App registered for this notification. Obj ID %d Notification Type %llu \n", 
							  __FUNCTION__, fbe_transport_notification_data[connect_to_sp][package_id].object_id,
							  (unsigned long long)fbe_transport_notification_data[connect_to_sp][package_id].notification_info.notification_type);
	
				notification_element->notification_function(fbe_transport_notification_data[connect_to_sp][package_id].object_id,
															fbe_transport_notification_data[connect_to_sp][package_id].notification_info,
															notification_element->notification_context);
			}
			next_element = fbe_queue_next(&fbe_transport_event_notifications_queue_head, queue_element);
			queue_element = next_element;
		}
	}
	fbe_spinlock_unlock(&fbe_transport_event_notifications_queue_lock);

	/*we don't need the packet anymore*/
	fbe_api_return_contiguous_packet(packet);

	/*now that we sent the notifications about this event to all the registered functions
	we can send another packet to the notification service so it can return it
	with more events if there are any*/
	if (fbe_transport_event_registration_id[connect_to_sp][package_id] != 0xDEADBEEF) {
        fbe_api_transport_get_next_event (package_id, connect_to_sp);
	}else{
		fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s reg id invalid, can't send get_next:%d\n", __FUNCTION__, fbe_transport_event_registration_id[connect_to_sp][package_id]);
	}
	
	return FBE_STATUS_OK;
}


static fbe_status_t fbe_api_transport_unregister_for_event_notification (fbe_package_id_t package_id, fbe_transport_connection_target_t connect_to_sp)
{
	fbe_package_register_app_notify_t	register_app;
	fbe_status_t 						status = FBE_STATUS_GENERIC_FAILURE;
	fbe_packet_t * 						packet = NULL;
    fbe_payload_ex_t *					payload = NULL;
	fbe_payload_control_operation_t *	control_operation = NULL;

    /* Allocate packet.*/
	packet = fbe_api_get_contiguous_packet();
	if (packet == NULL) {
		fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: unable to allocate memory for packet\n", __FUNCTION__); 
		fbe_transport_event_registration_id[connect_to_sp][package_id]= 0xDEADBEEF;
		fbe_semaphore_release(&unregistered_confirmation[connect_to_sp][package_id], 0, 1, FALSE);
		return FBE_STATUS_GENERIC_FAILURE;
	}
	
	register_app.application_id = fbe_transport_event_registration_id[connect_to_sp][package_id];/*say who we are*/

    payload = fbe_transport_get_payload_ex (packet);
    control_operation = fbe_payload_ex_allocate_control_operation(payload);

    fbe_payload_control_build_operation (control_operation,
										 fbe_transport_packages_handles[package_id].unregister_from_event_ioctl,
										 &register_app,
										 sizeof(fbe_package_register_app_notify_t));
    
	/* Set packet address.*/
	fbe_transport_set_address(packet,
							  package_id,
							  FBE_SERVICE_ID_SIM_SERVER,
							  FBE_CLASS_ID_INVALID,
							  FBE_OBJECT_ID_INVALID); 

	fbe_transport_set_packet_attr(packet, FBE_PACKET_FLAG_NO_ATTRIB);

	fbe_transport_set_sync_completion_type(packet, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);
	status = fbe_api_common_send_control_packet_to_driver(packet);

	if (status != FBE_STATUS_OK && status != FBE_STATUS_PENDING){
		/* Only trace when it is an error we do not expect.*/
		fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s error in sending packet, status:%d\n", __FUNCTION__, status);  
		fbe_transport_event_registration_id[connect_to_sp][package_id]= 0xDEADBEEF;
		fbe_semaphore_release(&unregistered_confirmation[connect_to_sp][package_id], 0, 1, FALSE);
		fbe_api_return_contiguous_packet(packet);
		return status;
	}
    /* Wait for the callback.  Our completion function is always called.
     * Note that we must use a timeout since if the SP process is down, then 
     * we need to timeout with a reasonable limit. 
     */
    status = fbe_semaphore_wait_ms(&packet->completion_semaphore, 10000);
    if (status != FBE_STATUS_OK){
		/* Only trace when it is an error we do not expect.*/
		fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s error waiting for semaphore status:%d\n", __FUNCTION__, status);  
		fbe_transport_event_registration_id[connect_to_sp][package_id]= 0xDEADBEEF;
		fbe_semaphore_release(&unregistered_confirmation[connect_to_sp][package_id], 0, 1, FALSE);
		fbe_api_return_contiguous_packet(packet);
		return status;
	}
    
	/* Save the status for returning.*/
	status = fbe_transport_get_status_code(packet);
	if (status != FBE_STATUS_OK && status != FBE_STATUS_PENDING){
		/* Only trace when it is an error we do not expect.*/
		fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s error in received packet, status:%d\n", __FUNCTION__, status);  
		fbe_transport_event_registration_id[connect_to_sp][package_id]= 0xDEADBEEF;
		fbe_semaphore_release(&unregistered_confirmation[connect_to_sp][package_id], 0, 1, FALSE);
		fbe_api_return_contiguous_packet(packet);
		return status;
	}

    fbe_transport_event_registration_id[connect_to_sp][package_id]= 0xDEADBEEF;
	fbe_semaphore_release(&unregistered_confirmation[connect_to_sp][package_id], 0, 1, FALSE);

	fbe_api_return_contiguous_packet(packet);

	return FBE_STATUS_OK;
}

static void wait_for_all_events_unregistration(fbe_transport_connection_target_t connect_to_sp)
{
	fbe_package_id_t			package_id = FBE_PACKAGE_ID_INVALID;
    
    for (package_id = FBE_PACKAGE_ID_INVALID; package_id < FBE_PACKAGE_ID_LAST; package_id++) {
        if ((fbe_transport_packages_handles[package_id].register_for_event_ioctl != NULL) && (client_notification_registered[package_id])) {
            fbe_semaphore_wait(&unregistered_confirmation[connect_to_sp][package_id], NULL);
            fbe_semaphore_destroy(&unregistered_confirmation[connect_to_sp][package_id]); /* SAFEBUG - needs destroy */
		}
	}

}
