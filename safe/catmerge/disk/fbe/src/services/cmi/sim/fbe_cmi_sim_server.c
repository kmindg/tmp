/***************************************************************************
 * Copyright (C) EMC Corporation 2001-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *  fbe_cmi_sim_server.c
 ***************************************************************************
 *
 *  Description
 *      server implementation for CMI simulation service
 *      
 *    
 ***************************************************************************/
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#include <stdlib.h>
#include <malloc.h>
#endif

#include "fbe_cmi.h"
#include "fbe_cmi_private.h"
#include "fbe/fbe_winddk.h"
#include "fbe_cmi_sim_client_server.h"
#include "fbe/fbe_memory.h"
#include "fbe/fbe_atomic.h"

//#include <winsock2.h>
#include <stdio.h>
#include <ws2tcpip.h>
#ifndef SD_BOTH
#define SD_BOTH SHUT_RDWR
#endif

/* Local definitions*/
#define FBE_CMI_SERVER_MAGIC_NUMBER	0xFFBBEE88

#define FBE_CMI_SIM_POOL_TAG	'cebf'


typedef enum fbe_cmi_sim_server_thread_flag_e{
	FBE_CMI_SIM_SERVER_THREAD_INVALID,
    FBE_CMI_SIM_SERVER_THREAD_RUN,
    FBE_CMI_SIM_SERVER_THREAD_STOP,
    FBE_CMI_SIM_SERVER_THREAD_DONE
}fbe_cmi_sim_server_thread_flag_t;

typedef struct fbe_cmi_sim_server_connection_context_s{
	SOCKET 					connection_socket;
	fbe_thread_t			thread_handle;
	fbe_u32_t				magic_number;
}fbe_cmi_sim_server_connection_context_t;

/* Static members */
static SOCKET 									listen_socket[FBE_CMI_CONDUIT_ID_LAST];
static fbe_cmi_sim_server_thread_flag_t    		fbe_cmi_sim_server_thread_flag[FBE_CMI_CONDUIT_ID_LAST];
static fbe_thread_t								fbe_cmi_sim_server_connection_thread[FBE_CMI_CONDUIT_ID_LAST];
static fbe_cmi_sim_server_connection_context_t	fbe_cmi_sim_server_connection_context[FBE_CMI_CONDUIT_ID_LAST];
static fbe_cmi_sp_id_t 							current_sp_id = FBE_CMI_SP_ID_INVALID;
static fbe_atomic_t 							fbe_cmi_sim_received_message_counter[FBE_CMI_CONDUIT_ID_LAST];

/* Forward declerations */
static void cmi_sim_server_connection_thread_function(void * context);
static void cmi_sim_server_request_process_thread_function(void * context);
static fbe_u32_t cmi_sim_server_process_message(fbe_cmi_sim_tcp_message_t *cmi_tcp_message, SOCKET send_socket);
static fbe_u32_t cmi_sim_server_return_message(SOCKET socket, fbe_u8_t * buffer, fbe_s32_t length);
static void * fbe_cmi_sim_allocate_memory (fbe_u32_t  NumberOfBytes);
static void fbe_cmi_sim_free_memory (PVOID  P);

/*************************************************************************************************************************/
fbe_status_t fbe_cmi_sim_init_conduit_server(fbe_cmi_conduit_id_t conduit_id, fbe_cmi_sp_id_t sp_id)
{
	struct 			addrinfo hints;
    fbe_u32_t		result = 0;
    const fbe_u8_t *		port = NULL;
    struct			addrinfo *addr_info_ptr = NULL;
    fbe_bool_t 		reuse_sock = FBE_TRUE;
    fbe_u32_t 		opt_len = sizeof(fbe_bool_t);
	fbe_status_t	status = FBE_STATUS_GENERIC_FAILURE;

	current_sp_id = sp_id;

	status = fbe_cmi_sim_map_conduit_to_server_port(conduit_id, &port, sp_id);
	if (status != FBE_STATUS_OK) {
	    fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR, "%s: Failed to get port number for conduit:%d\n", __FUNCTION__, conduit_id);
	    return FBE_STATUS_GENERIC_FAILURE;
    }

	fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, "SP%s CMI Server will listen on port %s\n", (sp_id == FBE_CMI_SP_ID_A ? "A" : "B"), port);
   
    fbe_set_memory(&hints, 0, sizeof (hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;
    
    /* Resolve the local address and port to be used by the server*/
    result = getaddrinfo("localhost", port, &hints, &addr_info_ptr);
    if (result != 0) {
	    fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR, "%s, getaddrinfo failed: %d\n", __FUNCTION__, result);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    listen_socket[conduit_id] = socket(addr_info_ptr->ai_family, addr_info_ptr->ai_socktype, addr_info_ptr->ai_protocol);
    
    if (listen_socket[conduit_id] == INVALID_SOCKET) {
	    fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR, "%s, Error at socket(): %d\n", __FUNCTION__, EmcutilLastNetworkErrorGet());
	    freeaddrinfo(addr_info_ptr);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (setsockopt(listen_socket[conduit_id], SOL_SOCKET, SO_REUSEADDR, (char*)&reuse_sock, opt_len) == SOCKET_ERROR) {
		fbe_cmi_trace(FBE_TRACE_LEVEL_WARNING, "Unable to set socket reuse, binding may fail\n");
	}

    /*Setup the TCP listening socket*/
    result = bind( listen_socket[conduit_id], addr_info_ptr->ai_addr, (int)addr_info_ptr->ai_addrlen);
    if (result == SOCKET_ERROR) {
	    fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR, "%s: bind failed: %d\n", __FUNCTION__, EmcutilLastNetworkErrorGet());
	    freeaddrinfo(addr_info_ptr);
	    closesocket(listen_socket[conduit_id]);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    freeaddrinfo(addr_info_ptr);/*don't need anymore*/
    
    if ( listen( listen_socket[conduit_id], SOMAXCONN ) == SOCKET_ERROR ) {
	    fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR, "%s: Error at bind(): %d\n",  __FUNCTION__, EmcutilLastNetworkErrorGet() );
	    closesocket(listen_socket[conduit_id]);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /*now we start the thread that will accept calls and serve them*/
    fbe_cmi_sim_server_connection_context[conduit_id].thread_handle.thread = NULL;/*will stay NULL until we receive a connection*/
	fbe_cmi_sim_server_connection_context[conduit_id].magic_number = FBE_CMI_SERVER_MAGIC_NUMBER;
	fbe_cmi_sim_received_message_counter[conduit_id] = 0;

    fbe_thread_init(&fbe_cmi_sim_server_connection_thread[conduit_id], "fbe_cmisim_srvcon", cmi_sim_server_connection_thread_function, (void *)conduit_id);

    return FBE_STATUS_OK;
}

fbe_status_t fbe_cmi_sim_destroy_conduit_server(fbe_cmi_conduit_id_t conduit_id)
{
    if (fbe_cmi_sim_server_connection_context[conduit_id].magic_number == FBE_CMI_SERVER_MAGIC_NUMBER) {

		fbe_cmi_sim_server_connection_context[conduit_id].magic_number = 0;
		fbe_cmi_sim_server_thread_flag[conduit_id] = FBE_CMI_SIM_SERVER_THREAD_STOP;

        if(fbe_cmi_sim_server_connection_context[conduit_id].connection_socket != INVALID_SOCKET) {
            shutdown(fbe_cmi_sim_server_connection_context[conduit_id].connection_socket, SD_BOTH );
            closesocket(fbe_cmi_sim_server_connection_context[conduit_id].connection_socket);
        }

		shutdown(listen_socket[conduit_id], SD_BOTH );
		closesocket(listen_socket[conduit_id]);
		fbe_cmi_sim_received_message_counter[conduit_id] = 0;
    
		/*in case we were just listening*/
		fbe_thread_wait(&fbe_cmi_sim_server_connection_thread[conduit_id]);
		fbe_thread_destroy(&fbe_cmi_sim_server_connection_thread[conduit_id]);
	
	
		/*if we started a conenction we need to wait for the thread to finish*/
		if (fbe_cmi_sim_server_connection_context[conduit_id].thread_handle.thread != NULL) {
			fbe_thread_wait(&fbe_cmi_sim_server_connection_context[conduit_id].thread_handle);
			fbe_thread_destroy(&fbe_cmi_sim_server_connection_context[conduit_id].thread_handle);
		}
	}else{
		fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR, "%s: trying to destroy conduit %d w/o initializing first\n",  __FUNCTION__, conduit_id);
	}

	return FBE_STATUS_OK;

}

static void cmi_sim_server_connection_thread_function(void * context)
{
    fbe_cmi_conduit_id_t						conduit_id = (fbe_cmi_conduit_id_t)context;
    fbe_cmi_sim_server_connection_context[conduit_id].connection_socket = INVALID_SOCKET;

	fbe_cmi_sim_server_thread_flag[conduit_id] = FBE_CMI_SIM_SERVER_THREAD_RUN;

    while(fbe_cmi_sim_server_thread_flag[conduit_id] == FBE_CMI_SIM_SERVER_THREAD_RUN){

		/*Accept a client socket*/
        fbe_cmi_sim_server_connection_context[conduit_id].connection_socket = accept(listen_socket[conduit_id], NULL, NULL);
		/*did the peer die or are we just going down ?*/
		if (fbe_cmi_sim_server_connection_context[conduit_id].connection_socket == INVALID_SOCKET) {
            if (fbe_cmi_sim_server_thread_flag[conduit_id] == FBE_CMI_SIM_SERVER_THREAD_RUN) {
				fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR, 
                              "%s: connection error, resetting connection: %d\n", 
                              __FUNCTION__, conduit_id);
				continue;
			}else {
				fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, 
                              "%s: socket closed, connection: %d shutting down\n", 
                              __FUNCTION__, conduit_id);
                fbe_cmi_client_clear_pending_messages(conduit_id);
				break;
			}
		}

		fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, "%s, conduit:%d\n", __FUNCTION__, conduit_id);
        
        fbe_cmi_sim_wait_for_client_to_init(conduit_id);

		/*we start the client connection so when we get handshakes on the server we can answer then*/
		verify_peer_connection_is_up(current_sp_id, conduit_id); /* This will start client */

        /*we start a thread per connection and give it the context*/
		fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, 
                      "%s: Server accepted connection from peer on conduit %d\n", 
                      __FUNCTION__, conduit_id);
		fbe_thread_init(&fbe_cmi_sim_server_connection_context[conduit_id].thread_handle, "fbe_cmisim_srv_req",
						cmi_sim_server_request_process_thread_function,
						(void *)&fbe_cmi_sim_server_connection_context[conduit_id]);

        
    }	
	
    fbe_cmi_sim_server_thread_flag[conduit_id] = FBE_CMI_SIM_SERVER_THREAD_DONE;
    fbe_thread_exit(EMCPAL_STATUS_SUCCESS);
}


static void cmi_sim_server_request_process_thread_function(void * context)
{
    fbe_cmi_sim_server_connection_context_t *	connection_context = (fbe_cmi_sim_server_connection_context_t *)context;
	fbe_u8_t									*rec_buf_ptr = NULL;
    fbe_s32_t									bytes = 0;
    fbe_u32_t									total_bytes = 0;
    fbe_u32_t 									incomming_transfer_count = 0;
	fbe_u32_t									current_receive_count = 0;
	fbe_u32_t									bytes_left_to_transfer = 0;
	fbe_u32_t									initial_read_size = sizeof(fbe_cmi_sim_tcp_message_t);
	fbe_u8_t *									target_buf_ptr = NULL;
	fbe_u8_t *									target_current_ptr = NULL;
	fbe_cmi_sim_tcp_message_t *					cmi_tcp_message = NULL;
	fbe_u32_t									os_error = 0;
	fbe_u32_t									rc = 0;
    fbe_u32_t max_buffer_size = MAX_PACKET_BUFFER_SIZE + MAX_SEP_IO_DATA_LENGTH;

	rec_buf_ptr = fbe_cmi_sim_allocate_memory (RECEIVE_BUFFER_SIZE);
	target_buf_ptr = fbe_cmi_sim_allocate_memory(max_buffer_size);

	while (1) {
        total_bytes = 0;
        current_receive_count = initial_read_size;/*initial read size assumes no buffer was sent, just an empty structure*/
		fbe_zero_memory (target_buf_ptr, max_buffer_size);
		target_current_ptr = target_buf_ptr;
		do {
			bytes = recv(connection_context->connection_socket, rec_buf_ptr, current_receive_count, 0);
			if (bytes > 0) {
                /*for the first transfer, we also take the size of the overall packet we are about to receive.
				we assume the first argument in the fbe_cmi_sim_tcp_message_t will contain it*/
				if (total_bytes == 0) {
					fbe_copy_memory(&incomming_transfer_count, rec_buf_ptr, sizeof(fbe_u32_t));
					bytes_left_to_transfer = incomming_transfer_count;
				}

				bytes_left_to_transfer -= bytes;
				if (bytes_left_to_transfer > RECEIVE_BUFFER_SIZE) {
					current_receive_count = RECEIVE_BUFFER_SIZE;
				}else{
					current_receive_count = bytes_left_to_transfer;
				}

				/*sanity check*/
				if (total_bytes > max_buffer_size) {
					/*too much information, we can't process it*/
                    fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR, "%s:Buffer size received is bigger than %dK, packet lost\n", __FUNCTION__,  max_buffer_size / 1024);
                    closesocket(connection_context->connection_socket);
					fbe_cmi_sim_free_memory (rec_buf_ptr);
					fbe_cmi_sim_free_memory (target_buf_ptr);
					fbe_thread_exit(EMCPAL_STATUS_SUCCESS);
					return;
				}

				/*copy to destination_buffer*/
				fbe_copy_memory(target_current_ptr, rec_buf_ptr, bytes);
				target_current_ptr += bytes;/*advance for next time*/
				total_bytes += bytes;

			}else if (bytes == 0 ){
                fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, "%s: client disconnected!\n", __FUNCTION__);
				fbe_cmi_sim_free_memory (rec_buf_ptr);
				fbe_cmi_sim_free_memory (target_buf_ptr);
				closesocket(connection_context->connection_socket);
				fbe_thread_exit(EMCPAL_STATUS_SUCCESS);
				return;
			}else {
				os_error =  EmcutilLastNetworkErrorGet();
				if (os_error != WSAECONNABORTED) {
					fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, "%s: recv failed with code %d, packet lost\n", __FUNCTION__, os_error);
				}else{
					fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, "%s: client disconnected!\n", __FUNCTION__);
				}
				fbe_cmi_sim_free_memory (rec_buf_ptr);
				fbe_cmi_sim_free_memory (target_buf_ptr);
				closesocket(connection_context->connection_socket);
				fbe_thread_exit(EMCPAL_STATUS_SUCCESS);
				return;
			}
		} while ((bytes > 0) && (total_bytes < incomming_transfer_count));
		
        /*now that we got the message, we need to call back to the service that registered to get it*/
		cmi_tcp_message = (fbe_cmi_sim_tcp_message_t *)target_buf_ptr;
		if (cmi_tcp_message->msg_type == FBE_CMI_SIM_TCP_MESSAGE_TYPE_WITH_FIXED_DATA &&
			cmi_tcp_message->fixed_data_length != 0 &&
			cmi_tcp_message->dest_addr != NULL)
		{
			/* Copy the fixed data to destination */
			fbe_copy_memory(cmi_tcp_message->dest_addr, 
                            target_buf_ptr + cmi_tcp_message->total_message_lenth - cmi_tcp_message->fixed_data_length, 
                            cmi_tcp_message->fixed_data_length);
			cmi_tcp_message->total_message_lenth -= cmi_tcp_message->fixed_data_length;
		}
		rc = cmi_sim_server_process_message(cmi_tcp_message, connection_context->connection_socket);
		
		if(rc != cmi_tcp_message->total_message_lenth){
			fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, "%s: Failed to process message\n", __FUNCTION__);
			break;
		}
		
	}

	fbe_cmi_sim_free_memory (rec_buf_ptr);
	fbe_cmi_sim_free_memory (target_buf_ptr);
	closesocket(connection_context->connection_socket);
    fbe_thread_exit(EMCPAL_STATUS_SUCCESS);
	return;
}

static fbe_u32_t cmi_sim_server_process_message(fbe_cmi_sim_tcp_message_t *cmi_tcp_message, SOCKET send_socket)
{
	fbe_cmi_event_callback_context_t			fbe_cmi_meeage = NULL;
	fbe_u32_t									rc = 0;
	fbe_status_t status;

	/*the actual message is after our tcp structure in the serialized buffer*/
	fbe_cmi_meeage = (fbe_cmi_event_callback_context_t *)((fbe_u8_t *)cmi_tcp_message + sizeof(fbe_cmi_sim_tcp_message_t));

	switch(cmi_tcp_message->event_id){
	case FBE_CMI_EVENT_MESSAGE_RECEIVED:
		fbe_atomic_increment(&fbe_cmi_sim_received_message_counter[cmi_tcp_message->conduit]);
		status = fbe_cmi_call_registered_client(cmi_tcp_message->event_id, cmi_tcp_message->client_id, (cmi_tcp_message->total_message_lenth - sizeof(fbe_cmi_sim_tcp_message_t)), fbe_cmi_meeage, NULL);
		if (status == FBE_STATUS_BUSY) {
			cmi_tcp_message->event_id = FBE_CMI_EVENT_PEER_BUSY;
		}else{
			cmi_tcp_message->event_id = FBE_CMI_EVENT_MESSAGE_TRANSMITTED;
		}
		fbe_atomic_decrement(&fbe_cmi_sim_received_message_counter[cmi_tcp_message->conduit]);
        if ((status != FBE_STATUS_BUSY) && (status != FBE_STATUS_OK))
        {
            fbe_cmi_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, "%s: unexpected client status %d\n", __FUNCTION__, status);
        }
		cmi_tcp_message->message_status = status;
		rc = cmi_sim_server_return_message(send_socket, (fbe_u8_t *)cmi_tcp_message, cmi_tcp_message->total_message_lenth);
		break;
	case FBE_CMI_EVENT_SP_CONTACT_LOST:
	case FBE_CMI_EVENT_CLOSE_COMPLETED:

		break;
	default:
		fbe_cmi_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, "%s: unhandled CMI message %d\n", __FUNCTION__, cmi_tcp_message->event_id);
	}

	return rc;
}

static fbe_u32_t cmi_sim_server_return_message(SOCKET socket, fbe_u8_t * buffer, fbe_s32_t length)
{
	fbe_u32_t	bytes = 0;
	fbe_u32_t	rc = 0;

	while(bytes < (fbe_u32_t)length){
		rc = send(socket, buffer + bytes, length - bytes, 0);
		if (rc == SOCKET_ERROR || rc == 0) {
			fbe_cmi_trace(FBE_TRACE_LEVEL_WARNING, "%s, send failed:%d\n", __FUNCTION__,EmcutilLastNetworkErrorGet());
			//closesocket(socket);			
			break;
		} else {
			bytes += rc;
		}
	}
	return bytes;
}

/*********************************************************************
 *            fbe_cmi_sim_allocate_memory ()
 *********************************************************************
 *
 *  Description: memory allocation, ready to be cahnged for csx if needed
 *               memory is always zeroed
 *               !!!!Contiguous memory is not guaranteed here, but non pages is guaranteed!!!
 *
 *  Return Value: memory pointer
 *
 *  History:
 *    10/14/10: created
 *    
 *********************************************************************/
static void * fbe_cmi_sim_allocate_memory (fbe_u32_t  NumberOfBytes)
{
	void *		mem_ptr = NULL;

    mem_ptr = fbe_allocate_nonpaged_pool_with_tag ( NumberOfBytes, FBE_CMI_SIM_POOL_TAG);
	if (mem_ptr != NULL) {
		fbe_zero_memory(mem_ptr, NumberOfBytes);
	}

	return mem_ptr;
}

/*********************************************************************
 *            fbe_cmi_sim_free_memory ()
 *********************************************************************
 *
 *  Description: memory freeing, ready to be cahnged for csx if needed
 *
 *  Return Value: memory pointer
 *
 *  History:
 *    10/14/10: created
 *    
 *********************************************************************/
static void fbe_cmi_sim_free_memory (PVOID  P)
{
    fbe_release_nonpaged_pool_with_tag(P, FBE_CMI_SIM_POOL_TAG);
}

