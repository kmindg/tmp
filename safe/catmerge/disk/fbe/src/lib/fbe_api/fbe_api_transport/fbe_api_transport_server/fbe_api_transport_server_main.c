/***************************************************************************
 * Copyright (C) EMC Corporation 2001-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *  fbe_api_transport_server_main.c
 ***************************************************************************
 *
 *  Description
 *      the server side of the fbe packet transport between the client and server
 ****************************************************************************/
#define I_AM_NATIVE_CODE
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>
#include <malloc.h>
#endif

#include "fbe/fbe_winddk.h"

//#include <winsock2.h>
#include <stdio.h>
#include <ws2tcpip.h>
#ifndef SD_BOTH
#define SD_BOTH SHUT_RDWR
#endif

#include "fbe/fbe_api_transport.h"
#include "fbe/fbe_api_transport_packet_interface.h"
#include "fbe_api_transport_packet_interface_private.h"
#include "fbe/fbe_queue.h"
#include "fbe/fbe_memory.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_terminator_service.h"
#include "fbe_packet_serialize_lib.h"
#include "fbe/fbe_physical_package.h"
#include "fbe/fbe_sep.h"
#include "fbe/fbe_esp.h"
#include "fbe/fbe_neit_package.h"
#include "fbe_test_package_config.h"
#include "fbe_trace_buffer.h"
#include "fbe/fbe_api_rdgen_interface.h"

/**************************************
                Local variables
**************************************/
#define RECEIVE_BUFFER_SIZE 4096

typedef enum fbe_api_transport_server_thread_flag_e{
    FBE_API_TRANSPORT_SERVER_THREAD_RUN,
    FBE_API_TRANSPORT_SERVER_THREAD_STOP,
    FBE_API_TRANSPORT_SERVER_THREAD_DONE
}fbe_api_transport_server_thread_flag_t;

typedef struct fbe_api_transport_server_connection_context_s{
    fbe_queue_element_t     queue_element;
    SOCKET                  connection_socket;
    fbe_thread_t            thread_handle;
    fbe_mutex_t             connection_lock;
    fbe_u32_t               commands_outstanding; /* SAFEBUG - must track this - many related changes below */
}fbe_api_transport_server_connection_context_t;

typedef struct fbe_api_transport_tcp_completion_context_s{
    fbe_api_transport_server_connection_context_t   *   connection_context;
    fbe_u32_t                                   bytes;/*to be used by the same context on completion*/
    fbe_u8_t *                                  target_buf;
    HANDLE                                      completion_signal;
}fbe_api_transport_tcp_completion_context_t;

static SOCKET                               listen_socket = INVALID_SOCKET;
static fbe_thread_t                         connection_thread;
static fbe_api_transport_server_thread_flag_t       fbe_api_transport_server_thread_flag;
static fbe_semaphore_t                      fbe_api_transport_server_thread_semaphore;
static fbe_spinlock_t                       fbe_api_transport_server_queue_lock;
static fbe_queue_head_t                     fbe_api_transport_server_queue_head;
static fbe_spinlock_t                       server_send_lock;

/*******************************************
                Local functions
********************************************/
static void connection_thread_function(void * context);
static void request_process_thread_function(void * context);
static void fbe_api_transport_server_command_completion_function(fbe_u8_t *returned_packet, void *context);
static fbe_status_t fbe_api_transport_transport_convert_and_send_buffer(fbe_u8_t *packet_buffer,
                                  server_command_completion_t completion_function,
                                  void *sender_context);
static fbe_status_t fbe_api_transport_convert_and_send_buffer_to_terminator(fbe_u8_t *packet_buffer,
                                        server_command_completion_t completion_function,
                                        void *sender_context);
static void move_packet_levels(fbe_packet_t *packet);
static fbe_status_t fbe_sim_transport_control_load_package(fbe_packet_t * packet);
static fbe_status_t fbe_sim_transport_control_init_fbe_api(fbe_packet_t * packet);
static fbe_status_t fbe_sim_transport_control_destroy_fbe_api(fbe_packet_t * packet);
static fbe_status_t fbe_sim_transport_control_set_package_entries(fbe_packet_t * packet);
static fbe_status_t fbe_sim_transport_control_unload_package(fbe_packet_t * packet);
static fbe_status_t fbe_transport_control_get_windows_cpu_utilization(fbe_packet_t *packet);
static fbe_status_t fbe_transport_control_get_ica_status(fbe_packet_t *packet);
static fbe_status_t fbe_sim_transport_control_disable_package(fbe_packet_t * packet);

/* This is a debug ring buffer for the fbe_trace_log_record() calls below.
 */
static fbe_trace_buffer_t request_process_debug_trace_ring = {0, 0};

/************************************************************************************************************************/
fbe_status_t FBE_API_CALL fbe_api_transport_init_server(fbe_transport_connection_target_t sp_mode)
{
    struct          addrinfo hints;
    fbe_u32_t       result = 0;
    const fbe_u8_t *        port = NULL;
    struct          addrinfo *addr_info_ptr = NULL;
    fbe_bool_t      reuse_sock = FBE_TRUE;
    fbe_u32_t       opt_len = sizeof(fbe_bool_t);
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;
    /* the original buf_size is 0. On Windows it could work well. 
     * But Linux TCP/IP stack has a different behavior, which introduces unbearable slowness */
    fbe_u32_t       buf_size = 65536; 

    status = fbe_api_transport_map_server_mode_to_port(sp_mode, &port);
    if (status != FBE_STATUS_OK) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: Failed to get port number\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*initialize queuing and locking*/
    fbe_queue_init(&fbe_api_transport_server_queue_head);
    fbe_spinlock_init(&fbe_api_transport_server_queue_lock);
    fbe_spinlock_init(&server_send_lock);

    /*init winsock*/
    result = EmcutilNetworkWorldInitializeMaybe();
    if (result != 0) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "WSAStartup failed: %d\n", result);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    memset(&hints, 0, sizeof (hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    /* Resolve the local address and port to be used by the server*/
    result = getaddrinfo(NULL, port, &hints, &addr_info_ptr);
    if (result != 0) {
       fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s, getaddrinfo failed: %d\n", __FUNCTION__, result);
       return FBE_STATUS_GENERIC_FAILURE;
    }

    listen_socket = socket(addr_info_ptr->ai_family, addr_info_ptr->ai_socktype, addr_info_ptr->ai_protocol);

    if (listen_socket == INVALID_SOCKET) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s, Error at socket(): %d\n", __FUNCTION__, EmcutilLastNetworkErrorGet());
        freeaddrinfo(addr_info_ptr);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse_sock, opt_len) == SOCKET_ERROR) {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "Unable to set socket reuse, binding may fail\n");
    }

    
    if (setsockopt(listen_socket, SOL_SOCKET, SO_SNDBUF, (char*)&buf_size, sizeof(buf_size)) == SOCKET_ERROR) {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "Unable to set socket SO_SNDBUF\n");
    }

    if (setsockopt(listen_socket, SOL_SOCKET, SO_RCVBUF, (char*)&buf_size, sizeof(buf_size)) == SOCKET_ERROR) {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "Unable to set socket SO_RCVBUF\n");
    }

    /*Setup the TCP listening socket*/
    result = bind(listen_socket, addr_info_ptr->ai_addr, (int)addr_info_ptr->ai_addrlen);
    if (result == SOCKET_ERROR) {
       fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: bind failed: %d\n", __FUNCTION__, EmcutilLastNetworkErrorGet());
       freeaddrinfo(addr_info_ptr);
       closesocket(listen_socket);
       return FBE_STATUS_GENERIC_FAILURE;
    }

    freeaddrinfo(addr_info_ptr);/*don't need anymore*/

    if ( listen( listen_socket, SOMAXCONN ) == SOCKET_ERROR ) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: Error at bind(): %d\n",  __FUNCTION__, EmcutilLastNetworkErrorGet() );
        closesocket(listen_socket);
        EmcutilNetworkWorldDeinitializeMaybe();
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*now we start the thread that will accept calls and server them*/
    fbe_semaphore_init(&fbe_api_transport_server_thread_semaphore, 0, 1);
    fbe_api_transport_server_thread_flag = FBE_API_TRANSPORT_SERVER_THREAD_RUN;

    fbe_thread_init(&connection_thread, "fbe_trans_connect", connection_thread_function, NULL);

    return FBE_STATUS_OK;

}

static void connection_thread_function(void * context)
{
    fbe_api_transport_server_connection_context_t * connection_context = NULL;
    fbe_u32_t                                   retry_count = 0;
    fbe_u32_t   shutdown_result = 0;
    
    FBE_UNREFERENCED_PARAMETER(context);

    while(1)
    {
        if(fbe_api_transport_server_thread_flag == FBE_API_TRANSPORT_SERVER_THREAD_RUN) {

            /*Accept a client socket*/
            connection_context = fbe_api_allocate_memory(sizeof(fbe_api_transport_server_connection_context_t));
            if (connection_context == NULL)
                break;

            connection_context->connection_socket = accept(listen_socket, NULL, NULL);
            if (connection_context->connection_socket == INVALID_SOCKET || connection_context->connection_socket == 0) {

                /*let's try again if we were not requested to go out*/
                if (fbe_api_transport_server_thread_flag == FBE_API_TRANSPORT_SERVER_THREAD_RUN) {
                    fbe_api_free_memory(connection_context);
                    connection_context = NULL;
                    fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: accept failed with error: %d, retrying...\n", __FUNCTION__, EmcutilLastNetworkErrorGet());
                    if (retry_count < 10) {
                        retry_count++;
                        EmcutilSleep(10);
                        continue;
                    }else{
                        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: accept retry failed, thread will exit...\n", __FUNCTION__);
                        if (connection_context != NULL) {
                            closesocket(connection_context->connection_socket);
                        }
                        break;
                    }
                }else {
                    /*closesocket(connection_context->connection_socket);*/
                    fbe_api_free_memory(connection_context);
                    break;
                }
            }

            /* when new client connects, clear all registered notifications and packets,
             * so we will not be affected by old messages left by last client which is kill exceptionally */
            /* Geng: if fbe_cli client connects into the server while the fbe_test is running, 
             *       fbe_transport_clear_all_notifications would purge out all the unprocessed notification data.
             *       That will cause JOB notification lost and time out.So I comment it out. */
            //fbe_transport_clear_all_notifications(FBE_FALSE);

            /*now that we have a connection request with some data on it, let's give it to the
            worker thread to process it so we can be ready for another connection request*/
            fbe_spinlock_lock(&fbe_api_transport_server_queue_lock);
            fbe_queue_push(&fbe_api_transport_server_queue_head, &connection_context->queue_element);
            fbe_spinlock_unlock(&fbe_api_transport_server_queue_lock);

            /*we start a thread per connection and give it the context*/
            fbe_thread_init(&connection_context->thread_handle, "fbe_req_proc", request_process_thread_function, connection_context);
        } else {
            if (connection_context != NULL) {
                shutdown_result = shutdown(connection_context->connection_socket, SD_BOTH);
                if (shutdown_result == SOCKET_ERROR)
                {
                    fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "shutdown connection failed: %d\n", shutdown_result);
                }
                closesocket(connection_context->connection_socket);
                fbe_api_free_memory(connection_context);
            }
            break;
        }
    }

    fbe_api_transport_server_thread_flag = FBE_API_TRANSPORT_SERVER_THREAD_DONE;
    fbe_semaphore_release(&fbe_api_transport_server_thread_semaphore, 0, 1,
                          FALSE);
    fbe_thread_exit(EMCPAL_STATUS_SUCCESS);
}

static void request_process_thread_function(void * context)
{
    fbe_api_transport_server_connection_context_t * connection_context = (fbe_api_transport_server_connection_context_t *)context;
    fbe_u8_t     *                          rec_buf = NULL;
    fbe_s32_t                                   bytes = 0;
    fbe_u8_t *                                  temp_ptr;
    fbe_u32_t                                   total_bytes = 0;
    fbe_api_transport_tcp_completion_context_t *        tcp_context = NULL;
    fbe_u32_t                                   incomming_transfer_count = 0;
    fbe_u32_t                                   current_receive_count = 0;
    fbe_u32_t                                   bytes_left_to_transfer = 0;
    fbe_u32_t                                   initial_read_size = sizeof(fbe_serialized_control_transaction_t) + sizeof (fbe_api_client_send_context_t);
    fbe_u32_t                                  winerr = 0;
    fbe_u32_t                                   shutdown_result = 0;

    rec_buf = fbe_api_allocate_memory(RECEIVE_BUFFER_SIZE);
    fbe_zero_memory(rec_buf, RECEIVE_BUFFER_SIZE);

	fbe_thread_set_affinity(&connection_context->thread_handle, 0x1);

    fbe_mutex_init(&connection_context->connection_lock);
    connection_context->commands_outstanding = 0;

    while (1) {

        /*prepare a buffer for the data*/
        tcp_context = (fbe_api_transport_tcp_completion_context_t *)fbe_api_allocate_memory(sizeof(fbe_api_transport_tcp_completion_context_t));
        fbe_zero_memory(tcp_context, sizeof(fbe_api_transport_tcp_completion_context_t));

        tcp_context->connection_context = connection_context;
        total_bytes = 0;

        /* Receive until the peer shuts down the connection*/
        current_receive_count = initial_read_size;/*initial read size assumes no buffer was sent, just an empty packt with an opcode*/

        do {
            bytes = recv(connection_context->connection_socket, rec_buf, current_receive_count, 0);
            if (bytes > 0) {
                /*for the first transfer, we also take the size of the overall packaet we are aobut to receive.
                it is right after the handle name*/
                if (total_bytes == 0) {
                    fbe_copy_memory(&incomming_transfer_count, rec_buf, sizeof(fbe_u32_t));
                    bytes_left_to_transfer = incomming_transfer_count;
                    tcp_context->target_buf = fbe_api_allocate_memory(incomming_transfer_count);
                    temp_ptr = tcp_context->target_buf;/*remember the start*/

                    if (tcp_context->target_buf == NULL) {
                        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s:can't allocate memory for receive buf\n", __FUNCTION__);
                        fbe_api_free_memory(tcp_context);
                        closesocket(connection_context->connection_socket);
                        goto cleanup_and_return;
                    }
                }

                bytes_left_to_transfer -= bytes;
                if (bytes_left_to_transfer > RECEIVE_BUFFER_SIZE) {
                    current_receive_count = RECEIVE_BUFFER_SIZE;
                }else{
                    current_receive_count = bytes_left_to_transfer;
                }

                /*copy to destination_buffer*/
                fbe_copy_memory(temp_ptr, rec_buf, bytes);
                temp_ptr += bytes;/*advance for next time*/
                total_bytes += bytes;

            }else if (bytes ==0 ){
                fbe_api_trace(FBE_TRACE_LEVEL_INFO, "%s: client disconnected bytes == 0!\n", __FUNCTION__);
				if(tcp_context->target_buf != NULL){
					fbe_api_free_memory(tcp_context->target_buf);
				}
                fbe_api_free_memory(tcp_context);
                shutdown_result = shutdown(connection_context->connection_socket, SD_BOTH);
                if (shutdown_result == SOCKET_ERROR)
                {
                    fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "shutdown connection failed: %d\n", shutdown_result);
                }
                closesocket(connection_context->connection_socket);
                goto cleanup_and_return;
            }else {
                winerr = EmcutilLastNetworkErrorGet();
                if (WSAECONNRESET == winerr) {
                    fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s: Client disconnected WSAECONNRESET\n", __FUNCTION__);
                }else if (WSAECONNABORTED == winerr) {
                    fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s: Client connection aborted\n", __FUNCTION__);
                }else{
                    fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: recv failed with code %d, packet lost\n", __FUNCTION__, winerr);
                }
				if(tcp_context->target_buf != NULL){
					fbe_api_free_memory(tcp_context->target_buf);
				}
                fbe_api_free_memory(tcp_context);
                shutdown_result = shutdown(connection_context->connection_socket, SD_BOTH);
                if (shutdown_result == SOCKET_ERROR)
                {
                    fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "shutdown connection failed: %d\n", shutdown_result);
                }
                closesocket(connection_context->connection_socket);
                goto cleanup_and_return;
            }
        } while ((bytes > 0) && (total_bytes < incomming_transfer_count));

        /*the bytes we send back is exactly what we received because the sender needs the context to continue processing*/
        tcp_context->bytes = incomming_transfer_count;/*we need it for later to send the exact amount back*/
        {
            fbe_api_client_send_context_t *context_p = (fbe_api_client_send_context_t*)tcp_context->target_buf;
            fbe_trace_buffer_log(&request_process_debug_trace_ring, 0xA0, 
                                   CSX_CAST_PTR_TO_PTRMAX(context_p->completion_context), 
                                   CSX_CAST_PTR_TO_PTRMAX(context_p->completion_function), 
                                   context_p->total_msg_length, fbe_get_time());
        }
        fbe_mutex_lock(&connection_context->connection_lock);
        connection_context->commands_outstanding++;
        fbe_mutex_unlock(&connection_context->connection_lock);

        fbe_api_transport_send_server_control_packet((fbe_u8_t *)(tcp_context->target_buf + sizeof(fbe_api_client_send_context_t)),
                                                         fbe_api_transport_server_command_completion_function,
                                                          (void *)tcp_context);


    }

cleanup_and_return:
    fbe_mutex_lock(&connection_context->connection_lock);
    while (connection_context->commands_outstanding != 0) {
        fbe_mutex_unlock(&connection_context->connection_lock);
        fbe_thread_delay(100);
        fbe_mutex_lock(&connection_context->connection_lock);
    }  
    fbe_mutex_unlock(&connection_context->connection_lock);

    fbe_mutex_destroy(&connection_context->connection_lock);

    fbe_api_free_memory (rec_buf);

    fbe_thread_exit(EMCPAL_STATUS_SUCCESS);
}

static void fbe_api_transport_server_command_completion_function(fbe_u8_t * returned_packet, void *context)
{
    fbe_s32_t						bytes = 0;
    fbe_s32_t						rc = 0;
    fbe_api_transport_tcp_completion_context_t *        tcp_context = (fbe_api_transport_tcp_completion_context_t *)context;

    /*roll back to the start of the data we want to send*/
    returned_packet -= sizeof(fbe_api_client_send_context_t);

    {
        fbe_api_client_send_context_t *context_p = (fbe_api_client_send_context_t*)tcp_context->target_buf;
        fbe_trace_buffer_log(&request_process_debug_trace_ring, 0xC0, 
                            (fbe_u64_t)context_p->completion_context, 
                            (fbe_u64_t)context_p->completion_function, 
                            context_p->total_msg_length, fbe_get_time());
    }

    fbe_mutex_lock(&tcp_context->connection_context->connection_lock);
    /*we use the same amount of total bytes we got since in FBE the source and destination buffers and SG list can't grow*/
	bytes = 0;
	while(bytes < (fbe_s32_t)tcp_context->bytes){
        rc = send(tcp_context->connection_context->connection_socket, returned_packet + bytes, tcp_context->bytes - bytes, 0);
        if (rc == SOCKET_ERROR) {
            fbe_mutex_unlock(&tcp_context->connection_context->connection_lock);
            fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: send failed: %d\n", __FUNCTION__, EmcutilLastNetworkErrorGet());
            /*and delete it, we don't need it anymore*/
            fbe_api_free_memory(tcp_context->target_buf);
            fbe_api_free_memory (tcp_context);
            return ;
        }
        else
        {
            fbe_api_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, "%s: Completed control Packet. Packet: 0x%p \n", __FUNCTION__, returned_packet);
        }
		bytes += rc;
    }
    tcp_context->connection_context->commands_outstanding--;
    fbe_mutex_unlock(&tcp_context->connection_context->connection_lock);

    fbe_api_free_memory(tcp_context->target_buf);
    fbe_api_free_memory (tcp_context);

}

fbe_status_t FBE_API_CALL fbe_api_transport_destroy_server(void)
{
    fbe_api_transport_server_connection_context_t *connection_context;

    fbe_api_trace(FBE_TRACE_LEVEL_INFO, "entering %s\n", CSX_MY_FUNC);

    /*flag the theread we should stop*/
    fbe_api_transport_server_thread_flag = FBE_API_TRANSPORT_SERVER_THREAD_STOP;

    /*we also destroy the thread the is processing the accept, we stop it by calling the wsacleanup function*/
    shutdown(listen_socket, SD_RECEIVE);
    closesocket(listen_socket);

    fbe_semaphore_wait_ms(&fbe_api_transport_server_thread_semaphore, 2000);

    if (FBE_API_TRANSPORT_SERVER_THREAD_DONE == fbe_api_transport_server_thread_flag) {
        fbe_thread_wait(&connection_thread);
    } else {
        fbe_api_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
                      "%s: connection thread (%p) did not exit!\n",
                      __FUNCTION__, connection_thread.thread);
    }
    fbe_semaphore_destroy(&fbe_api_transport_server_thread_semaphore);
    fbe_thread_destroy(&connection_thread);

    while (!fbe_queue_is_empty(&fbe_api_transport_server_queue_head)) {
        fbe_spinlock_lock(&fbe_api_transport_server_queue_lock);
        connection_context = (fbe_api_transport_server_connection_context_t *)fbe_queue_pop(&fbe_api_transport_server_queue_head);
        fbe_spinlock_unlock(&fbe_api_transport_server_queue_lock);
        shutdown(connection_context->connection_socket, SD_RECEIVE);
        closesocket(connection_context->connection_socket);
        fbe_thread_wait(&connection_context->thread_handle);
        fbe_thread_destroy(&connection_context->thread_handle);
        fbe_api_free_memory(connection_context);
    }


    fbe_spinlock_destroy(&fbe_api_transport_server_queue_lock);
    fbe_spinlock_destroy(&server_send_lock);
    fbe_queue_destroy (&fbe_api_transport_server_queue_head);

    fbe_api_transport_control_destroy_server_control();

    fbe_api_trace(FBE_TRACE_LEVEL_INFO, "%s: Done\n", __FUNCTION__);

    fbe_api_trace(FBE_TRACE_LEVEL_INFO, "leaving %s\n", CSX_MY_FUNC);

    return FBE_STATUS_OK;

}

fbe_status_t FBE_API_CALL fbe_api_transport_send_server_control_packet(fbe_u8_t *packet_buffer,
                                                              server_command_completion_t completion_function,
                                                              void * context)
{
    fbe_serialized_control_transaction_t *      packet = (fbe_serialized_control_transaction_t *)packet_buffer;
    fbe_status_t                                status = FBE_STATUS_GENERIC_FAILURE;

    fbe_api_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, "%s: Received control packet from Client App. Serialized Packet: %p, service_id: %d \n",
                  __FUNCTION__, packet, packet->service_id);


    /*we need to exam the control packet and see if it is a packet that is actually for the server itself or
    the terminator for example, in all other cases, it would go to the regular topology*/
    switch (packet->service_id) {
    case FBE_SERVICE_ID_TERMINATOR:
        status = fbe_api_transport_convert_and_send_buffer_to_terminator(packet_buffer, completion_function, context);
        break;
    case FBE_SERVICE_ID_SIM_SERVER:
        status = fbe_api_sim_transport_handle_server_packet(packet_buffer, completion_function, context);
        break;
    case FBE_SERVICE_ID_USER_SERVER:
    default:
        status = fbe_api_transport_transport_convert_and_send_buffer(packet_buffer, completion_function, context);
        break;
    }

    return status;
}

static fbe_status_t fbe_api_transport_transport_convert_and_send_buffer(fbe_u8_t *packet_buffer, server_command_completion_t completion_function, void *sender_context)
{
    fbe_package_id_t        package_id = FBE_PACKAGE_ID_INVALID;
    fbe_service_control_entry_t control_entry = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_packet_t * packet = NULL;
    fbe_serialized_control_transaction_t * control_transaction = NULL;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    control_transaction = (fbe_serialized_control_transaction_t *)packet_buffer;
    package_id = control_transaction->packge_id;
    packet = fbe_api_transport_convert_buffer_to_packet(packet_buffer, completion_function, sender_context);
    if (packet==NULL)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s: packet is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    move_packet_levels(packet);/*we don't really send it via the service manager so we need to do what it does*/

    switch(control_transaction->user_control_code)
    {
    case FBE_USER_SERVER_CONTROL_CODE_GET_PID:
        status = fbe_transport_control_get_pid(packet);
        break;
    case FBE_USER_SERVER_CONTROL_CODE_REGISTER_NTIFICATIONS:
        status = fbe_api_transport_register_notifications(FBE_PACKAGE_NOTIFICATION_ID_SEP_0 |
                FBE_PACKAGE_NOTIFICATION_ID_PHYSICAL|
                FBE_PACKAGE_NOTIFICATION_ID_ESP |
                FBE_PACKAGE_NOTIFICATION_ID_NEIT);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
        break;
    case FBE_USER_SERVER_CONTROL_CODE_SET_DRIVER_ENTRIES:
        status = fbe_api_transport_set_control_entry(fbe_api_common_send_control_packet_to_driver, FBE_PACKAGE_ID_PHYSICAL);
        status = fbe_api_transport_set_control_entry(fbe_api_common_send_control_packet_to_driver, FBE_PACKAGE_ID_ESP);
        status = fbe_api_transport_set_control_entry(fbe_api_common_send_control_packet_to_driver, FBE_PACKAGE_ID_SEP_0);
        status = fbe_api_transport_set_control_entry(fbe_api_common_send_control_packet_to_driver, FBE_PACKAGE_ID_NEIT);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
        break;
    default:
        fbe_api_transport_get_control_entry(&control_entry, package_id);
        if (control_entry == NULL) {
            fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s: control_entry is NULL\n", __FUNCTION__);
            /*package was not loaded yet, return the packet */
            payload = fbe_transport_get_payload_ex(packet);
            fbe_payload_ex_increment_control_operation_level(payload);

            fbe_transport_set_status(packet, FBE_STATUS_NOT_INITIALIZED, 0);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_OK;
        }
        else{
			fbe_cpu_id_t cpu_id;
		   	cpu_id = fbe_get_cpu_id();
			fbe_transport_set_cpu_id(packet, cpu_id);

            return control_entry(packet);
        }
    }
    return status;
}

static fbe_status_t fbe_api_transport_convert_and_send_buffer_to_terminator(fbe_u8_t *packet_buffer, server_command_completion_t completion_function, void *sender_context)
{
    /*convert the buffer to a packet*/
    fbe_packet_t * packet = fbe_api_transport_convert_buffer_to_packet(packet_buffer,completion_function, sender_context);

    if (packet==NULL)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s: packet is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    move_packet_levels(packet);/*we don't really send it via the service manager so we need to do what it does*/
    /*and send it to the terminator api entry*/
    return fbe_terminator_service_send_packet(packet);
}

static void move_packet_levels(fbe_packet_t *packet)
{
    fbe_payload_ex_t *         payload = NULL;

    payload = fbe_transport_get_payload_ex(packet);
    fbe_payload_ex_increment_control_operation_level(payload);

    return ;
}

static fbe_status_t fbe_sim_transport_control_load_package(fbe_packet_t * packet)
{
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_package_id_t    package_id = FBE_PACKAGE_ID_INVALID;

    /*package_id can be put into the paylaod*/
    status = fbe_transport_get_package_id(packet, &package_id);
    if (status != FBE_STATUS_OK) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s, fail to get package id\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_OK;
    }

    switch(package_id) {
    case FBE_PACKAGE_ID_PHYSICAL:
        status = fbe_test_load_physical_package();
        if (status != FBE_STATUS_OK) {
            fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s, fail to load_physical_package\n", __FUNCTION__);
            fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_OK;
        }
        break;
        case FBE_PACKAGE_ID_SEP_0:
        status = fbe_test_load_sep_package(packet);
        if (status != FBE_STATUS_OK) {
            fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s, fail to load sep package\n", __FUNCTION__);
            fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_OK;
        }
        break;
    case FBE_PACKAGE_ID_NEIT:
        status = fbe_test_load_neit_package(packet);
        if (status != FBE_STATUS_OK) {
            fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s, fail to load neit package\n", __FUNCTION__);
            fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet);
        return FBE_STATUS_OK;
        }
        break;
    case FBE_PACKAGE_ID_ESP:
            status = fbe_test_load_esp();
        if (status != FBE_STATUS_OK) {
            fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s, fail to load ESP\n", __FUNCTION__);
            fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_OK;
        }
        break;
    case FBE_PACKAGE_ID_KMS:
        status = fbe_test_load_kms_package(packet);
        if (status != FBE_STATUS_OK) {
            fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s, fail to load KMS\n", __FUNCTION__);
            fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_OK;
        }
        break;
    default:
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s, package id not supported!\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_OK;
    }

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

fbe_status_t fbe_api_sim_transport_handle_server_packet(fbe_u8_t *packet_buffer,
                                                        server_command_completion_t completion_function,
                                                        void * context)
{
    fbe_packet_t *                          packet = NULL;
    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_package_id_t                        package_id = FBE_PACKAGE_ID_INVALID;
    fbe_serialized_control_transaction_t *  control_transaction = (fbe_serialized_control_transaction_t *)packet_buffer;

    package_id = control_transaction->packge_id;
    packet = fbe_api_transport_convert_buffer_to_packet(packet_buffer, completion_function, context);
    if (packet==NULL)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s: packet is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    move_packet_levels(packet);/*we don't really send it via the service manager so we need to do what it does*/

    switch(control_transaction->user_control_code) {
    case FBE_PHYSICAL_PACKAGE_REGISTER_APP_NOTIFICATION_IOCTL:
    case FBE_SEP_REGISTER_APP_NOTIFICATION_IOCTL:
    case FBE_ESP_REGISTER_APP_NOTIFICATION_IOCTL:
    case FBE_NEIT_REGISTER_APP_NOTIFICATION_IOCTL:
        status = fbe_transport_control_register_notification(packet, package_id);
        break;
    case FBE_PHYSICAL_PACKAGE_UNREGISTER_APP_NOTIFICATION_IOCTL:
    case FBE_SEP_UNREGISTER_APP_NOTIFICATION_IOCTL:
    case FBE_ESP_UNREGISTER_APP_NOTIFICATION_IOCTL:
    case FBE_NEIT_UNREGISTER_APP_NOTIFICATION_IOCTL:
        status = fbe_transport_control_unregister_notification(packet, package_id);
        break;
    case FBE_PHYSICAL_PACKAGE_NOTIFICATION_GET_EVENT_IOCTL:
    case FBE_SEP_NOTIFICATION_GET_EVENT_IOCTL:
    case FBE_ESP_NOTIFICATION_GET_EVENT_IOCTL:
    case FBE_NEIT_NOTIFICATION_GET_EVENT_IOCTL:
        status = fbe_transport_control_get_event(packet, package_id);
        break;
    case FBE_SIM_SERVER_CONTROL_CODE_UNREGISTER_ALL_APPS:
        status = fbe_transport_control_unregister_all_notifications(packet);
    break;
    case FBE_SIM_SERVER_CONTROL_CODE_INIT_FBE_API:
        status = fbe_sim_transport_control_init_fbe_api(packet);
    break;
    case FBE_SIM_SERVER_CONTROL_CODE_DESTROY_FBE_API:
        status = fbe_sim_transport_control_destroy_fbe_api(packet);
    break;
    case FBE_SIM_SERVER_CONTROL_CODE_LOAD_PACKAGE:
        status = fbe_sim_transport_control_load_package(packet);
    break;
    case FBE_SIM_SERVER_CONTROL_CODE_SET_PACKAGE_ENTRIES:
        status = fbe_sim_transport_control_set_package_entries(packet);
    break;
    case FBE_SIM_SERVER_CONTROL_CODE_UNLOAD_PACKAGE:
        status = fbe_sim_transport_control_unload_package(packet);
    break;
    case FBE_SIM_SERVER_CONTROL_CODE_DISABLE_PACKAGE:
        status = fbe_sim_transport_control_disable_package(packet);
    break;
    case FBE_SIM_SERVER_CONTROL_CODE_GET_PID:
        status = fbe_transport_control_get_pid(packet);
        break;
    case FBE_SIM_SERVER_CONTROL_CODE_SUITE_START_NOTIFICATION:
        status = fbe_transport_control_suite_start_notification(packet);
        break;
    case FBE_SIM_SERVER_CONTROL_CODE_GET_WINDOWS_CPU_UTILIZATION:
        status = fbe_transport_control_get_windows_cpu_utilization(packet);
        break;
    case FBE_SIM_SERVER_CONTROL_CODE_GET_ICA_STATUS:
        status = fbe_transport_control_get_ica_status(packet);
        break;
    default:
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s, unknown control code:%d\n", __FUNCTION__, control_transaction->user_control_code);
        control_transaction->packet_status.code = FBE_STATUS_GENERIC_FAILURE;
        status = FBE_STATUS_GENERIC_FAILURE;
        completion_function(packet_buffer, context);
    }
    return status;
}

static fbe_status_t fbe_sim_transport_control_init_fbe_api(fbe_packet_t * packet)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    /*initialize the fbe api*/
    status  = fbe_api_common_init_sim();
    if (status != FBE_STATUS_OK) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s, error calling fbe_api_common_init_sim\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_OK;
    }

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_sim_transport_control_destroy_fbe_api(fbe_packet_t * packet)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    /*initialize the fbe api*/
    status  = fbe_api_common_destroy_sim();
    if (status != FBE_STATUS_OK) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s, error calling fbe_api_common_destroy_sim\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_OK;
    }

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_sim_transport_control_set_package_entries(fbe_packet_t * packet)
{
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_package_id_t    package_id = FBE_PACKAGE_ID_INVALID;

    /*package_id can be put into the paylaod*/
    status = fbe_transport_get_package_id(packet, &package_id);
    if (status != FBE_STATUS_OK) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR,"%s, fail to get package id\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_OK;
    }

    switch(package_id) {
    case FBE_PACKAGE_ID_PHYSICAL:
        /*explicit call to the init function of the simulation portion*/
        fbe_api_set_simulation_io_and_control_entries(package_id, physical_package_io_entry, physical_package_control_entry);
        /*set the simulation socket transport so it would route packets that arrived via TCP/IP to the correct place*/
        fbe_api_transport_set_control_entry(physical_package_control_entry, package_id);
        /*Register notification with package.*/
        if (!fbe_api_transport_is_notification_registered(package_id)) {
            fbe_transport_control_register_notification_element(package_id, NULL, FBE_NOTIFICATION_TYPE_ALL, FBE_TOPOLOGY_OBJECT_TYPE_ALL);	
        }
        break;
    case FBE_PACKAGE_ID_SEP_0:
        /*explicit call to the init function of the simulation portion*/
        fbe_api_set_simulation_io_and_control_entries(package_id, sep_io_entry, sep_control_entry);
        /*set the simulation socket transport so it would route packets that arrived via TCP/IP to the correct place*/
        fbe_api_transport_set_control_entry(sep_control_entry, package_id);
        /*Register notification with package.*/
        if (!fbe_api_transport_is_notification_registered(package_id)) {
            fbe_transport_control_register_notification_element(package_id, NULL, FBE_NOTIFICATION_TYPE_ALL, FBE_TOPOLOGY_OBJECT_TYPE_ALL);	
        }
        break;
    case FBE_PACKAGE_ID_NEIT:
        /*explicit call to the init function of the simulation portion*/
        fbe_api_set_simulation_io_and_control_entries(FBE_PACKAGE_ID_NEIT, NULL, neit_package_control_entry);
            /*set the simulation socket transport so it would route packets that arrived via TCP/IP to the correct place*/
        fbe_api_transport_set_control_entry(neit_package_control_entry, package_id);

		/*for NEIT we will also initialize the dps memory at this state*/
		/*send explicit init of dps memory inside NEIT since we can't load it by default on HW*/
		fbe_api_rdgen_init_dps_memory(FBE_API_RDGEN_DEFAULT_MEM_SIZE_MB,
                                      FBE_RDGEN_MEMORY_TYPE_INVALID); /* Get memory from any location. */
        break;
    case FBE_PACKAGE_ID_ESP:
        /*explicit call to the init function of the simulation portion*/
        fbe_api_set_simulation_io_and_control_entries(package_id, NULL, esp_control_entry);
        /*set the simulation socket transport so it would route packets that arrived via TCP/IP to the correct place*/
        fbe_api_transport_set_control_entry(esp_control_entry, package_id);
        /*Register notification with package.*/
        if (!fbe_api_transport_is_notification_registered(package_id)) {
            fbe_transport_control_register_notification_element(package_id, NULL, FBE_NOTIFICATION_TYPE_ALL, FBE_TOPOLOGY_OBJECT_TYPE_ALL);	
        }
        break;
    case FBE_PACKAGE_ID_KMS:
        /*explicit call to the init function of the simulation portion*/
        fbe_api_set_simulation_io_and_control_entries(package_id, NULL, kms_control_entry);
        /*set the simulation socket transport so it would route packets that arrived via TCP/IP to the correct place*/
        fbe_api_transport_set_control_entry(kms_control_entry, package_id);
        /*Register notification with package.*/
#if 0
        if (!fbe_api_transport_is_notification_registered(package_id)) {
            fbe_transport_control_register_notification_element(package_id, NULL, FBE_NOTIFICATION_TYPE_ALL, FBE_TOPOLOGY_OBJECT_TYPE_ALL);	
        }
#endif
        break;
    default:
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s, package id not supported!\n", __FUNCTION__);
        break;
    }

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_sim_transport_control_unload_package(fbe_packet_t * packet)
{
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_package_id_t    package_id = FBE_PACKAGE_ID_INVALID;

    /*package_id can be put into the payload*/
    status = fbe_transport_get_package_id(packet, &package_id);
    if (status != FBE_STATUS_OK) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s, fail to get package id\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_OK;
    }
    /* clear the control entry, so that we can tell if a package was loaded or not. */
    fbe_api_transport_set_control_entry(NULL, package_id);

    switch(package_id) {
    case FBE_PACKAGE_ID_PHYSICAL:
        /*Unregister notification with package.*/
        fbe_transport_control_unregister_notification_element(package_id);
        fbe_api_transport_control_clear_fstc_notifications_queue(package_id);
        status = fbe_test_unload_physical_package();
        if (status != FBE_STATUS_OK) {
            fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s, fail to unload_physical_package\n", __FUNCTION__);
            fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_OK;
        }
        break;
    case FBE_PACKAGE_ID_SEP_0:
        /*Unregister notification with package.*/
        fbe_transport_control_unregister_notification_element(package_id);
        fbe_api_transport_control_clear_fstc_notifications_queue(package_id);
        status = fbe_test_unload_sep();
        if (status != FBE_STATUS_OK) {
            fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s, fail to unload_sep.  Status: 0x%x\n", __FUNCTION__, status);
            fbe_transport_set_status(packet, status, 0);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_OK;
        }
        break;
    case FBE_PACKAGE_ID_ESP:
        /*Unregister notification with package.*/
        fbe_transport_control_unregister_notification_element(package_id);
        fbe_api_transport_control_clear_fstc_notifications_queue(package_id);
        status = fbe_test_unload_esp();
        if (status != FBE_STATUS_OK) {
            fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s, fail to unload_ESP, status: 0x%x\n", __FUNCTION__, status);
            fbe_transport_set_status(packet, status, 0);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_OK;
        }
        break;
    case FBE_PACKAGE_ID_NEIT:
        fbe_api_transport_control_clear_fstc_notifications_queue(package_id);
        status = fbe_test_unload_neit_package();
        if (status != FBE_STATUS_OK) {
            fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s, fail to unload_NEIT, status: 0x%x\n", __FUNCTION__, status);
            fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_OK;
        }
        break;
    case FBE_PACKAGE_ID_KMS:
        status = fbe_test_unload_kms();
        if (status != FBE_STATUS_OK) {
            fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s, fail to unload_KMS, status: 0x%x\n", __FUNCTION__, status);
            fbe_transport_set_status(packet, status, 0);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_OK;
        }
        break;
    default:
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s, package id not supported!\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_OK;
    }

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_transport_control_get_windows_cpu_utilization(fbe_packet_t *packet){
    fbe_payload_ex_t *             payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_api_sim_server_get_windows_cpu_utilization_t * get_cpu = NULL;
    fbe_u32_t       len = 0;
    FILE *tp;
    
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_buffer(control_operation, &get_cpu); 
    
    fbe_payload_control_get_buffer_length(control_operation, &len); 
    if(len < sizeof(fbe_api_sim_server_get_windows_cpu_utilization_t)){
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*now we run typeperf and get the resulting string */
    system("echo y | typeperf -sc 2 -o C:\\tp.csv \"\\processor(*)\\% processor time\"");
    fbe_api_sleep(5000); //give it a couple seconds to finish

    tp = fopen("C:\\tp.csv", "r");
    fgets(get_cpu->typeperf_str, 1024, tp); //ignore header and dummy value line
    fgets(get_cpu->typeperf_str, 1024, tp);
    fgets(get_cpu->typeperf_str, 1024, tp); //the actual data we want

    fclose(tp);

    /* complete the packet*/
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}


static fbe_status_t fbe_transport_control_get_ica_status(fbe_packet_t *packet)
{
    fbe_payload_ex_t *payload = NULL;
    fbe_payload_control_operation_t *control_operation = NULL;
    fbe_api_sim_server_get_ica_util_t *get_ica = NULL;
    fbe_u32_t len = 0;
    
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    if(control_operation != NULL)
    {
        fbe_payload_control_get_buffer(control_operation, &get_ica); 
        fbe_payload_control_get_buffer_length(control_operation, &len); 
        if(len < sizeof(fbe_api_sim_server_get_ica_util_t))
        {
            fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        get_ica->is_ica_done = is_ica_stamp_generated;

        /* complete the packet*/
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_OK;
    }

    fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_GENERIC_FAILURE;
}

/**************************************
           Wrapper functions for sim mode
**************************************/
fbe_status_t FBE_API_CALL fbe_api_sim_transport_init_server(fbe_sim_transport_connection_target_t sp_mode)
{
    fbe_transport_connection_target_t transport_target = fbe_transport_translate_fbe_sim_connection_target_to_fbe_transport_connection_target(sp_mode);
    return fbe_api_transport_init_server(transport_target);
}

static fbe_status_t fbe_sim_transport_control_disable_package(fbe_packet_t * packet)
{
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_package_id_t    package_id = FBE_PACKAGE_ID_INVALID;

    /*package_id can be put into the payload*/
    status = fbe_transport_get_package_id(packet, &package_id);
    if (status != FBE_STATUS_OK) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s, fail to get package id\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_OK;
    }
    /* clear the control entry, so that we can tell if a package was loaded or not. */
    fbe_api_transport_set_control_entry(NULL, package_id);

    /* Only supported for neit for now.
     */
    switch(package_id) {
    case FBE_PACKAGE_ID_NEIT:
        status = fbe_test_disable_neit_package();
        if (status != FBE_STATUS_OK) {
            fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s, fail to unload_NEIT, status: 0x%x\n", __FUNCTION__, status);
            fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_OK;
        }
        break;
    default:
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s, package id not supported!\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_OK;
    }

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}
