/***************************************************************************
 * Copyright (C) EMC Corporation 2001-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *  fbe_api_transport_packet_interface.c
 ***************************************************************************
 *
 *  Description
 *      Takes care of serializing and deserializing the packets
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
#include "fbe_packet_serialize_lib.h"

/********************************************
			Local structures
****************************************/

typedef struct server_mode_to_port_str_s{
	fbe_transport_connection_target_t	server_target;
	fbe_u8_t port_str[16];
}server_mode_to_port_str_t;

server_mode_to_port_str_t		server_to_port_table[] =
{
	{FBE_TRANSPORT_SP_A, "30220" },
	{FBE_TRANSPORT_SP_B, "30221"},
	{FBE_TRANSPORT_ADMIN_INTERFACE_PACKAGE, "30223"},
	{FBE_TRANSPORT_LAST_CONNECTION_TARGET, ""}
};

/**************************************
				Local variables
**************************************/
typedef struct transport_send_packet_context_s{
	fbe_u8_t *						user_buffer;
	server_command_completion_t		server_completion_function;
	void *							user_context;
	void *							sg_list;
}transport_send_packet_context_t;

typedef struct transport_target_to_package_pask_s{
	fbe_transport_connection_target_t	target;
	fbe_package_notification_id_mask_t					package_mask;
}transport_target_to_package_pask_t;

typedef struct fbe_sim_connection_target_to_fbe_transport_connection_target_map_e
{
    fbe_sim_transport_connection_target_t sim_target;
    fbe_transport_connection_target_t transport_target;
} fbe_sim_connection_target_to_fbe_transport_connection_target_map_t;

fbe_sim_connection_target_to_fbe_transport_connection_target_map_t connection_target_map[] =
{
    {FBE_SIM_SP_A, FBE_TRANSPORT_SP_A},
    {FBE_SIM_SP_B, FBE_TRANSPORT_SP_B},
    {FBE_SIM_ADMIN_INTERFACE_PACKAGE, FBE_TRANSPORT_ADMIN_INTERFACE_PACKAGE},
    {FBE_SIM_INVALID_SERVER, FBE_TRANSPORT_INVALID_SERVER}
};

static fbe_service_control_entry_t		fbe_api_transport_control_entry[FBE_PACKAGE_ID_LAST];

transport_target_to_package_pask_t	target_to_package_table[] =
{
	{FBE_TRANSPORT_SP_A, FBE_PACKAGE_NOTIFICATION_ID_PHYSICAL | FBE_PACKAGE_NOTIFICATION_ID_SEP_0 | FBE_PACKAGE_NOTIFICATION_ID_NEIT |FBE_PACKAGE_NOTIFICATION_ID_ESP},
	{FBE_TRANSPORT_SP_B, FBE_PACKAGE_NOTIFICATION_ID_PHYSICAL | FBE_PACKAGE_NOTIFICATION_ID_SEP_0 | FBE_PACKAGE_NOTIFICATION_ID_NEIT |FBE_PACKAGE_NOTIFICATION_ID_ESP},
    {FBE_TRANSPORT_LAST_CONNECTION_TARGET, FBE_PACKAGE_NOTIFICATION_ID_INVALID}

};
/*******************************************
				Local functions
********************************************/
static fbe_status_t fbe_api_transport_packet_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t fbe_api_transport_control_packet_completion(fbe_packet_t * serialized_transaction_p, fbe_packet_completion_context_t context);

/************************************************************************************************************/
fbe_status_t fbe_transport_send_client_control_packet_to_server(fbe_packet_t * packet)
{
    fbe_serialized_control_transaction_t * 	serialized_transaction = NULL;
	fbe_u32_t 								serialized_transaction_size;
	fbe_status_t							status = FBE_STATUS_GENERIC_FAILURE;

	/*we use the library to serizlie the packet*/
	status  = fbe_serialize_lib_serialize_packet(packet, &serialized_transaction, &serialized_transaction_size);
	
	/* fbe_api_trace(FBE_TRACE_LEVEL_INFO, "%s, %p %p\n", __FUNCTION__, serialized_transaction, packet ); */
	/*this is a completly synchronous operation, when this function return, we have all the data populated in the buffer*/
	status = fbe_api_transport_send_buffer((fbe_u8_t *)serialized_transaction,
												serialized_transaction_size,
												fbe_api_transport_control_packet_completion,
												packet,
											    serialized_transaction->packge_id);
	return status;
}


static fbe_status_t fbe_api_transport_packet_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
	transport_send_packet_context_t	*	send_packet_context = (transport_send_packet_context_t *)context;
    fbe_status_t							status = FBE_STATUS_GENERIC_FAILURE;

	/*we now make sure the "packet" we got here gets all the data from the packet we sent to the physical pacakge or sep*/
	status = fbe_serialize_lib_repack_transaction((fbe_serialized_control_transaction_t *) send_packet_context->user_buffer, packet);
	send_packet_context->server_completion_function(send_packet_context->user_buffer, send_packet_context->user_context);

    fbe_api_return_contiguous_packet(packet);
	fbe_api_free_memory(send_packet_context);

	return FBE_STATUS_OK;

}

fbe_status_t FBE_API_CALL fbe_api_transport_set_control_entry(fbe_service_control_entry_t control_entry, fbe_package_id_t package_id)
{
	if (package_id <= FBE_PACKAGE_ID_INVALID || package_id >= FBE_PACKAGE_ID_LAST) {
		return FBE_STATUS_GENERIC_FAILURE;
	}

	fbe_api_transport_control_entry[package_id] = control_entry;
	return FBE_STATUS_OK;
}

fbe_status_t FBE_API_CALL fbe_api_transport_get_control_entry(fbe_service_control_entry_t *control_entry, fbe_package_id_t package_id)
{
	if (package_id <= FBE_PACKAGE_ID_INVALID || package_id >= FBE_PACKAGE_ID_LAST) {
		return FBE_STATUS_GENERIC_FAILURE;
	}

	*control_entry = fbe_api_transport_control_entry[package_id];
	return FBE_STATUS_OK;
}


static fbe_status_t fbe_api_transport_control_packet_completion(fbe_packet_t * serialized_transaction_p, fbe_packet_completion_context_t context)
{
	fbe_packet_t *							orignal_user_packet = (fbe_packet_t *)context;
	fbe_serialized_control_transaction_t * 	serialized_transaction = (fbe_serialized_control_transaction_t * )serialized_transaction_p;
    fbe_cpu_id_t cpu_id;
    fbe_mutex_t *mutex = NULL;


    
	/* fbe_api_trace(FBE_TRACE_LEVEL_INFO, "%s, %p %p\n", __FUNCTION__, serialized_transaction_p, context ); */

	fbe_serialize_lib_deserialize_transaction(serialized_transaction, orignal_user_packet);

    fbe_transport_get_cpu_id(orignal_user_packet, &cpu_id);

	if(cpu_id == FBE_CPU_ID_INVALID){
		cpu_id = fbe_get_cpu_id();
        fbe_transport_set_cpu_id(orignal_user_packet, cpu_id);
    }

    /* we use the cancel function context to save the mutex used to protect the outstanding_packet_queue */
    mutex = (fbe_mutex_t*)orignal_user_packet->packet_cancel_context;
    fbe_mutex_lock(mutex);
    fbe_transport_remove_packet_from_queue(orignal_user_packet);
    fbe_mutex_unlock(mutex);

	/*and complete it for the original sender of the packet*/
	//fbe_transport_complete_packet (orignal_user_packet);
    /* It is necessary to dispatch this in a separate thread so that the original thread can continue sending. 
     * If we don't do this we end up with a deadlock where the receive thread turns around and does a send 
     * the send can't complete since the peer side can't recieve (it's waiting on a send too, which can't be processed 
     * by us.) 
     */
    fbe_transport_run_queue_push_packet(orignal_user_packet, FBE_TRANSPORT_RQ_METHOD_SAME_CORE);

	/*get rid of all the resources we consumed*/
	free ((void *)serialized_transaction_p);

	return FBE_STATUS_OK;

}

fbe_packet_t * fbe_api_transport_convert_buffer_to_packet(fbe_u8_t *packet_buffer,
															  server_command_completion_t completion_function,
															  void * sender_context)
{
	fbe_packet_t * 							packet = NULL;
	transport_send_packet_context_t	*	    send_packet_context = NULL;
    fbe_status_t							status = FBE_STATUS_GENERIC_FAILURE;
	fbe_serialized_control_transaction_t *  serialized_transaction_in = (fbe_serialized_control_transaction_t *)packet_buffer;

    packet = fbe_api_get_contiguous_packet();
    if (packet == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR,"%s: unable to allocate memory for packet\n", __FUNCTION__); 
        return NULL;
    }

    send_packet_context = (transport_send_packet_context_t *)fbe_api_allocate_memory(sizeof(transport_send_packet_context_t));
    if (send_packet_context == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR,"%s: unable to allocate memory for send context\n", __FUNCTION__); 
        fbe_api_return_contiguous_packet(packet);
        return NULL;
    }

	send_packet_context->server_completion_function = completion_function;
	send_packet_context->user_buffer = packet_buffer;
	send_packet_context->user_context = sender_context;

	status = fbe_serialize_lib_unpack_transaction(serialized_transaction_in, packet, &send_packet_context->sg_list);
	fbe_transport_set_completion_function (packet, fbe_api_transport_packet_completion, send_packet_context);
    
	return packet;
}

fbe_status_t fbe_api_transport_get_control_operation(fbe_packet_t * packet, fbe_payload_control_operation_t **control_operation)
{
	fbe_payload_ex_t * 	payload_ex = NULL;
	fbe_payload_operation_header_t * payload_operation_header = NULL;

	payload_ex = (fbe_payload_ex_t *)fbe_transport_get_payload_ex(packet);

	*control_operation = NULL;

	if(payload_ex->current_operation == NULL){	
		if(payload_ex->next_operation != NULL){
			payload_operation_header = (fbe_payload_operation_header_t *)payload_ex->next_operation;

			/* Check if current operation is control operation */
			if(payload_operation_header->payload_opcode != FBE_PAYLOAD_OPCODE_CONTROL_OPERATION){
				return FBE_STATUS_GENERIC_FAILURE;
			}

			*control_operation = (fbe_payload_control_operation_t *)payload_operation_header;
		} else {
			return FBE_STATUS_GENERIC_FAILURE;
		}
	} else {
		payload_operation_header = (fbe_payload_operation_header_t *)payload_ex->current_operation;

		/* Check if current operation is control operation */
		if(payload_operation_header->payload_opcode != FBE_PAYLOAD_OPCODE_CONTROL_OPERATION){
			return FBE_STATUS_GENERIC_FAILURE;
		}

		*control_operation = (fbe_payload_control_operation_t *)payload_operation_header;
	}
    return FBE_STATUS_OK;
}

static void move_packet_levels(fbe_packet_t *packet)
{
	fbe_payload_ex_t * 		payload = NULL;
    
    payload = fbe_transport_get_payload_ex(packet);
    fbe_payload_ex_increment_control_operation_level(payload);
    
    return ;
}

fbe_status_t fbe_api_transport_map_server_mode_to_port(fbe_transport_connection_target_t server, const fbe_u8_t **port_string)
{
	server_mode_to_port_str_t *	translation_table = server_to_port_table;

	while (translation_table->server_target != FBE_TRANSPORT_LAST_CONNECTION_TARGET) {
		if (translation_table->server_target == server) {
			*port_string = translation_table->port_str;
			return FBE_STATUS_OK;
		}

		translation_table ++;
	}

	fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s, unable to find a port match to server %d\n", __FUNCTION__, server);
	return FBE_STATUS_GENERIC_FAILURE;
	
}

fbe_status_t FBE_API_CALL fbe_api_transport_set_server_mode_port(fbe_u16_t port_base)
{
    server_mode_to_port_str_t * translation_table = server_to_port_table;
    fbe_u16_t port = port_base;

    while (translation_table->server_target != FBE_TRANSPORT_LAST_CONNECTION_TARGET) {
        itoa(port, translation_table->port_str, 10);
        translation_table ++;
        port ++;
    }

    return FBE_STATUS_OK;
}

fbe_status_t fbe_api_transport_get_package_mask_for_sever_target(fbe_transport_connection_target_t connect_to_sp, fbe_package_notification_id_mask_t *package_mask)
{
	transport_target_to_package_pask_t *	table_ptr = target_to_package_table;

	while (table_ptr->target != FBE_TRANSPORT_LAST_CONNECTION_TARGET) {
		if (table_ptr->target == connect_to_sp) {
			*package_mask = table_ptr->package_mask;
			return FBE_STATUS_OK;
		}

		table_ptr ++;
	}

	fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s, unable to find a package mask match to server %d\n", __FUNCTION__, connect_to_sp);
	return FBE_STATUS_GENERIC_FAILURE;

}

fbe_status_t FBE_API_CALL fbe_api_transport_send_client_notification_packet_to_targeted_server(fbe_packet_t * packet, fbe_transport_connection_target_t server_target)
{
    fbe_serialized_control_transaction_t * 	serialized_transaction = NULL;
	fbe_u32_t 								serialized_transaction_size;
	fbe_status_t							status = FBE_STATUS_GENERIC_FAILURE;

	/*we use the library to serizlie the packet*/
	status  = fbe_serialize_lib_serialize_packet(packet, &serialized_transaction, &serialized_transaction_size);
	
	 /* fbe_api_trace(FBE_TRACE_LEVEL_INFO, "%s, %p %p\n", __FUNCTION__, serialized_transaction, packet ); */
	/*this is a completly synchronous operation, when this function return, we have all the data populated in the buffer*/
	status = fbe_api_transport_send_notification_buffer((fbe_u8_t *)serialized_transaction,
															serialized_transaction_size,
															fbe_api_transport_control_packet_completion,
															packet,
															serialized_transaction->packge_id, 
															server_target);
	return status;
}

fbe_status_t FBE_API_CALL fbe_api_sim_transport_send_io_packet(fbe_packet_t * packet)
{
    return FBE_STATUS_GENERIC_FAILURE;

}

fbe_status_t FBE_API_CALL fbe_api_sim_transport_send_client_control_packet(fbe_packet_t * packet)
{
    fbe_payload_control_operation_opcode_t      control_code = 0;
    fbe_payload_control_operation_t *           control_operation = NULL;
    
    #if 0
    if (init_reference_count == 0) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s is called but API is not initialized\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;/*we already did init*/
    }

    /*we also check if we have a handle to this driver*/
    fbe_transport_get_package_id(packet, &package_id);
    if (packages_handles[package_id].package_handle == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s no handle aquired for this package, can't send\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    #endif
    /* We do some tricks here to look for some special opcodes we don't want the user to send down
    for example, The event registration is done with us and not the real notification service */
    fbe_api_transport_get_control_operation(packet, &control_operation);
    fbe_payload_control_get_opcode (control_operation, &control_code);

    /*the code below is for taking care of some special cases if needed.
    when not needed, we just send the packet*/
    switch (control_code) {
    case FBE_NOTIFICATION_CONTROL_CODE_REGISTER:
        fbe_transport_increment_stack_level (packet);
        return fbe_api_transport_register_notification_callback(packet);
        break;
    case FBE_NOTIFICATION_CONTROL_CODE_UNREGISTER:
        fbe_transport_increment_stack_level (packet);
        return fbe_api_transport_unregister_notification_callback(packet);
        break;
    default:
        if (control_code == FBE_SIM_SERVER_CONTROL_CODE_UNLOAD_PACKAGE){
            /*when we unload packages we clean the queue of the job notifications since new ones will come and we don't want old numbers to be there*/
            fbe_api_common_clean_job_notification_queue();
        }

        move_packet_levels(packet);
        return fbe_transport_send_client_control_packet_to_server (packet);
    }
}

fbe_transport_connection_target_t
fbe_transport_translate_fbe_sim_connection_target_to_fbe_transport_connection_target (fbe_sim_transport_connection_target_t sim_target)
{
    fbe_sim_connection_target_to_fbe_transport_connection_target_map_t *map  = connection_target_map;

     while (map->sim_target != FBE_SIM_INVALID_SERVER)
     {
        if (map->sim_target == sim_target)
        {
            return map->transport_target;
        }
        ++map;
    }

    fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: can't map fbe_sim_transport_connection_target_t:0x%X!\n", __FUNCTION__,  sim_target);
    return FBE_TRANSPORT_INVALID_SERVER;
}

/**************************************
           Wrapper functions for sim mode
**************************************/
fbe_status_t FBE_API_CALL fbe_api_sim_transport_set_server_mode_port(fbe_u16_t port_base)
{
    return fbe_api_transport_set_server_mode_port(port_base);
}

fbe_status_t FBE_API_CALL fbe_api_sim_transport_set_control_entry(fbe_service_control_entry_t control_entry, fbe_package_id_t package_id)
{
    return fbe_api_transport_set_control_entry(control_entry, package_id);
}

fbe_status_t FBE_API_CALL fbe_api_sim_transport_get_control_entry(fbe_service_control_entry_t *control_entry, fbe_package_id_t package_id)
{
    return fbe_api_transport_get_control_entry(control_entry, package_id);
}
