/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/
 
/*!*************************************************************************
 * @file fbe_protocol_error_injection_package_control.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the routines for communicating with other packages.
 *
 * @version
 *   11/23/2009:  Created. guov
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe_transport_memory.h"
#include "fbe_ssp_transport.h"
#include "fbe_stp_transport.h"
#include "fbe_protocol_error_injection_private.h"


/*************************
 *   FUNCTION DEFINITIONS
 *************************/
static fbe_status_t protocol_error_injection_send_control_packet(fbe_payload_control_operation_opcode_t control_code,
                												fbe_payload_control_buffer_t buffer,
                												fbe_payload_control_buffer_length_t buffer_length,
                                                                fbe_object_id_t object_id,
                												fbe_packet_attr_t attr,
                												fbe_package_id_t package_id);
fbe_status_t protocol_error_injection_send_control_packet_to_service(fbe_payload_control_operation_opcode_t control_code,
        														   fbe_payload_control_buffer_t buffer,
        														   fbe_payload_control_buffer_length_t buffer_length,
        														   fbe_service_id_t service_id,
        														   fbe_packet_attr_t attr,
        														   fbe_package_id_t package_id);
static fbe_status_t protocol_error_injection_send_control_packet_completion (fbe_packet_t * packet, fbe_packet_completion_context_t context);

/*********************************************************************
 *            protocol_error_injection_send_control_packet ()
 *********************************************************************
 *
 *  Description: send a control packet to the package
 * inputs:
 *			control_code - the opcode
 *			buffer - the buffer poitner
 *			buffer_length - length of buffer
 *			object_id - object id to send to
 *			attr - traversse or not
 *          package_id - package the packet is sent to
 *
 *  History:
 *    11/23/09: Created guov
 *    
 *********************************************************************/
static fbe_status_t protocol_error_injection_send_control_packet(fbe_payload_control_operation_opcode_t control_code,
												fbe_payload_control_buffer_t buffer,
												fbe_payload_control_buffer_length_t buffer_length,
                                                fbe_object_id_t object_id,
												fbe_packet_attr_t attr,
												fbe_package_id_t package_id)
{
    fbe_status_t 						status = FBE_STATUS_GENERIC_FAILURE;
    fbe_packet_t * 						packet = NULL;
    fbe_semaphore_t 					sem;
	fbe_payload_ex_t *						payload = NULL;
	fbe_payload_control_operation_t *	control_operation = NULL;

	fbe_semaphore_init(&sem, 0, 1);

    /* Allocate packet.*/
    packet = fbe_transport_allocate_packet();
    if (packet == NULL)
    {
        fbe_protocol_error_injection_service_trace(FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "fbe_transport_allocate_packet failed\n");
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
	if (package_id == FBE_PACKAGE_ID_PHYSICAL) {
		fbe_transport_initialize_packet(packet);
	}else{
		fbe_transport_initialize_sep_packet(packet);
	}

    payload = fbe_transport_get_payload_ex (packet);
    control_operation = fbe_payload_ex_allocate_control_operation(payload);

	fbe_payload_control_build_operation (control_operation,
										 control_code,
										 buffer,
										 buffer_length);

	fbe_transport_set_completion_function (packet, protocol_error_injection_send_control_packet_completion,  &sem);
	

    /* Set packet address.*/
    fbe_transport_set_address(packet,
                              package_id,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              object_id); 
    
    /* Mark packet with the attribute, either traverse or not */
    fbe_transport_set_packet_attr(packet, attr);

    status = fbe_service_manager_send_control_packet(packet);
    if (status != FBE_STATUS_OK && status != FBE_STATUS_PENDING){
        /* Only trace when it is an error we do not expect.*/
         fbe_protocol_error_injection_service_trace(FBE_TRACE_LEVEL_WARNING,
                                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                                    "%s error in sending packet, package: %d object_id: %d control code: 0x%x, status:%d\n", 
                                                    __FUNCTION__, package_id, object_id, control_code, status);  
         fbe_semaphore_destroy(&sem);
		 fbe_transport_release_packet(packet);
		 return status;
    }

    /* Wait for the callback.  Our completion function is always called.*/
    fbe_semaphore_wait(&sem, NULL);

    /* Save the status for returning.*/
    status = fbe_transport_get_status_code(packet);
    fbe_semaphore_destroy(&sem);
    fbe_transport_release_packet(packet);

    return status;
}

/*********************************************************************
 *            protocol_error_injection_send_control_packet_to_service ()
 *********************************************************************
 *
 *  Description: send a control packet to one of the services in the services
 * inputs:
 *			control_code - the opcode
 *			buffer - the buffer poitner
 *			buffer_length - length of buffer
 *			object_id - object id to send to
 *			attr - traversse or not
 *          package_id - package the packet is sent to
 *
 *  History:
 *    11/25/09: Created   guov
 *    
 *********************************************************************/
fbe_status_t protocol_error_injection_send_control_packet_to_service(fbe_payload_control_operation_opcode_t control_code,
														   fbe_payload_control_buffer_t buffer,
														   fbe_payload_control_buffer_length_t buffer_length,
														   fbe_service_id_t service_id,
														   fbe_packet_attr_t attr,
														   fbe_package_id_t package_id)
{
	fbe_status_t 						status = FBE_STATUS_GENERIC_FAILURE;
    fbe_packet_t * 						packet = NULL;
    fbe_semaphore_t 					sem;
	fbe_payload_ex_t *						payload = NULL;
	fbe_payload_control_operation_t *	control_operation = NULL;

	fbe_semaphore_init(&sem, 0, 1);

    /* Allocate packet.*/
    packet = fbe_transport_allocate_packet();
    if (packet == NULL)
    {
        fbe_protocol_error_injection_service_trace(FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "fbe_transport_allocate_packet failed\n");
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
	if (package_id == FBE_PACKAGE_ID_PHYSICAL) {
		fbe_transport_initialize_packet(packet);
	}else{
		fbe_transport_initialize_sep_packet(packet);
	}

    payload = fbe_transport_get_payload_ex (packet);
    control_operation = fbe_payload_ex_allocate_control_operation(payload);

	fbe_payload_control_build_operation (control_operation,
										 control_code,
										 buffer,
										 buffer_length);

	fbe_transport_set_completion_function (packet, protocol_error_injection_send_control_packet_completion,  &sem);
	

    /* Set packet address.*/
    fbe_transport_set_address(packet,
                              package_id,
                              service_id,
                              FBE_CLASS_ID_INVALID,
                              FBE_OBJECT_ID_INVALID); 
    
    /* Mark packet with the attribute, either traverse or not */
    fbe_transport_set_packet_attr(packet, attr);

    status = fbe_service_manager_send_control_packet(packet);
    if (status != FBE_STATUS_OK && status != FBE_STATUS_PENDING){
        /* Only trace when it is an error we do not expect.*/
         fbe_protocol_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR,
                                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                                    "%s error in sending packet, status:%d\n", 
                                                    __FUNCTION__, 
                                                    status);  
         fbe_semaphore_destroy(&sem);
		 fbe_transport_release_packet(packet);
		 return status;
    }

    /* Wait for the callback.  Our completion function is always called.*/
    fbe_semaphore_wait(&sem, NULL);

    /* Save the status for returning.*/
    status = fbe_transport_get_status_code(packet);
    fbe_semaphore_destroy(&sem);
    fbe_transport_release_packet(packet);

    return status;

}

/*********************************************************************
 *            protocol_error_injection_send_control_packet_completion ()
 *********************************************************************
 *
 *  Description: completion function for the packet that was sent
 *
 *	Inputs: completing packet
		   	completion context with relevant completion information (sepahore to take down)
 *
 *  Return Value: success or failure
 *
 *  History:
 *    11/23/09: Created  guov
 *    
 *********************************************************************/
static fbe_status_t protocol_error_injection_send_control_packet_completion (fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
	fbe_semaphore_t * sem = (fbe_semaphore_t * )context;
    fbe_semaphore_release(sem, 0, 1, FALSE);
	return FBE_STATUS_OK;
}

/*********************************************************************
 *            protocol_error_injection_set_ssp_edge_hook ()
 *********************************************************************
 *
 *  Description: set the hook on SSP edge of the given object in physical package
 *
 *  Inputs: object_id - object to quiery
 *          hook_info  - hook
 *
 *  Return Value: success or failue
 *
 *  History:
 *    11/23/09: guov    created
 *    
 *********************************************************************/
fbe_status_t protocol_error_injection_set_ssp_edge_hook (fbe_object_id_t object_id, fbe_transport_control_set_edge_tap_hook_t *hook_info)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    status = protocol_error_injection_send_control_packet (FBE_SSP_TRANSPORT_CONTROL_CODE_SET_EDGE_TAP_HOOK,
                                                         hook_info,
                                                         sizeof (fbe_transport_control_set_edge_tap_hook_t),
                                                         object_id,
                                                         FBE_PACKET_FLAG_NO_ATTRIB,
                                                         FBE_PACKAGE_ID_PHYSICAL);
    return status;
}

/*********************************************************************
 *            protocol_error_injection_set_stp_edge_hook ()
 *********************************************************************
 *
 *  Description: set the hook on STP edge of the given object in physical package
 *
 *  Inputs: object_id - object to quiery
 *          hook_info  - hook
 *
 *  Return Value: success or failue
 *
 *  History:
 *    11/23/09: Created  guov
 *    
 *********************************************************************/

fbe_status_t protocol_error_injection_set_stp_edge_hook (fbe_object_id_t object_id, fbe_transport_control_set_edge_tap_hook_t *hook_info)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    status = protocol_error_injection_send_control_packet (FBE_STP_TRANSPORT_CONTROL_CODE_SET_EDGE_TAP_HOOK,
                                                 hook_info,
                                                 sizeof (fbe_transport_control_set_edge_tap_hook_t),
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 FBE_PACKAGE_ID_PHYSICAL);
    return status;
}


/*********************************************************************
 *            protocol_error_injection_get_object_class_id ()
 *********************************************************************
 *
 *  Description: return the class of the object 
 *
 *  Inputs: object_id - object to quiery
 *          class_id  - the class of the object
 *
 *  Return Value: success or failue
 *
 *  History:
 *    11/24/09: Created  guov
 *    
 *********************************************************************/
fbe_status_t protocol_error_injection_get_object_class_id (fbe_object_id_t object_id, fbe_class_id_t *class_id, fbe_package_id_t package_id)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_base_object_mgmt_get_class_id_t             base_object_mgmt_get_class_id;  

    status = protocol_error_injection_send_control_packet (FBE_BASE_OBJECT_CONTROL_CODE_GET_CLASS_ID,
                                                         &base_object_mgmt_get_class_id,
                                                         sizeof (fbe_base_object_mgmt_get_class_id_t),
                                                         object_id,
                                                         FBE_PACKET_FLAG_NO_ATTRIB,
                                                         package_id);
   if (status != FBE_STATUS_OK ) {
       *class_id = FBE_CLASS_ID_INVALID;
        return FBE_STATUS_GENERIC_FAILURE;
    }
   
    *class_id = base_object_mgmt_get_class_id.class_id;
    return status;
}

/*********************************************************************
 *            protocol_error_injection_register_notification_element ()
 *********************************************************************
 *
 *  Description: register for getting notification on object changes   
 *
 *  Return Value:  success or failure
 *
 *  History:
 *    11/25/09: Created  guov
 *    
 *********************************************************************/
fbe_status_t protocol_error_injection_register_notification_element(fbe_package_id_t package_id, 
																   fbe_notification_function_t notification_function, 
																   void * context)
{
    fbe_status_t               	status = FBE_STATUS_GENERIC_FAILURE;
	fbe_notification_element_t 	notification_element;
    
    notification_element.notification_function = notification_function;
    notification_element.notification_context = context;
	notification_element.notification_type = FBE_NOTIFICATION_TYPE_LIFECYCLE_ANY_STATE_CHANGE;
	notification_element.object_type = FBE_TOPOLOGY_OBJECT_TYPE_ALL;

	status = protocol_error_injection_send_control_packet_to_service(FBE_NOTIFICATION_CONTROL_CODE_REGISTER,
																	 &notification_element,
																	 sizeof(fbe_notification_element_t),
																	 FBE_SERVICE_ID_NOTIFICATION,
																	 FBE_PACKET_FLAG_NO_ATTRIB,
																	 package_id);
    return status;
}

/*********************************************************************
 *            protocol_error_injection_unregister_notification_element ()
 *********************************************************************
 *
 *  Description: remove from the notification   
 *
 *  Return Value:  success or failure
 *
 *  History:
 *    11/25/09: Created  guov
 *    
 *********************************************************************/
fbe_status_t protocol_error_injection_unregister_notification_element(fbe_package_id_t package_id, 
																		   fbe_notification_function_t notification_function, 
																		   void * context)
{
    fbe_status_t               	status = FBE_STATUS_GENERIC_FAILURE;
	fbe_notification_element_t 	notification_element;
    
    notification_element.notification_function = notification_function;
    notification_element.notification_context = context;

	status = protocol_error_injection_send_control_packet_to_service(FBE_NOTIFICATION_CONTROL_CODE_UNREGISTER,
																	 &notification_element,
																	 sizeof(fbe_notification_element_t),
																	 FBE_SERVICE_ID_NOTIFICATION,
																	 FBE_PACKET_FLAG_NO_ATTRIB,
																	 package_id);
    return status;
}

/*************************
 * end file fbe_protocol_error_injection_edge_tap.c
 *************************/
