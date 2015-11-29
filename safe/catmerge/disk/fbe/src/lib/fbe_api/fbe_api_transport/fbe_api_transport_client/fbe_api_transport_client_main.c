/***************************************************************************
 * Copyright (C) EMC Corporation 2001-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *  fbe_api_transport_client_main.c
 ***************************************************************************
 *
 *  Description
 *      the client side of the fbe packet transport between the client and server
 ****************************************************************************/
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
//#define NTSTATUS fbe_u32_t
#include <stdlib.h>
#include <malloc.h>
#endif

#include "fbe/fbe_types.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_api_transport.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_api_common.h"
#include "fbe_api_transport_packet_interface_private.h"

//#include <winsock2.h>
#include <stdio.h>
#include <ws2tcpip.h>
#ifndef SD_BOTH
#define SD_BOTH SHUT_RDWR
#endif


/**************************************
				Local variables
**************************************/
#define FBE_API_CLIENT_REC_BUF_SIZE		4096


typedef enum fbe_api_transport_client_thread_flag_e{
    FBE_API_TRANSPORT_CLIENT_THREAD_RUN,
    FBE_API_TRANSPORT_CLIENT_THREAD_STOP,
    FBE_API_TRANSPORT_CLIENT_THREAD_DONE,
    FBE_API_TRANSPORT_CLIENT_THREAD_EXCEPTION
}fbe_api_transport_client_thread_flag_t;

static const fbe_u8_t *						fbe_transport_port;
static fbe_api_transport_client_thread_flag_t     fbe_api_transport_client_thread_flag[FBE_TRANSPORT_LAST_CONNECTION_TARGET];
static fbe_thread_t							receive_completion_thread[FBE_TRANSPORT_LAST_CONNECTION_TARGET];
static fbe_queue_head_t                     outstanding_packet_queue[FBE_TRANSPORT_LAST_CONNECTION_TARGET];
static SOCKET 								connect_socket[FBE_TRANSPORT_LAST_CONNECTION_TARGET];
static fbe_mutex_t  						connect_lock[FBE_TRANSPORT_LAST_CONNECTION_TARGET];
static fbe_client_mode_t 					client_mode = FBE_CLIENT_MODE_INVALID;
static fbe_transport_connection_target_t current_target = FBE_TRANSPORT_INVALID_SERVER;
static fbe_u32_t							fbe_transport_client_init_count = 0;
static fbe_bool_t       					notification_enabled[FBE_TRANSPORT_LAST_CONNECTION_TARGET];
static fbe_bool_t                           b_unregister_on_connnect = FBE_FALSE;

typedef struct package_to_server_target_s{
	fbe_package_id_t						package_id;
	fbe_transport_connection_target_t	server_target;
}package_to_server_target_t;

/*this structure tells us if we can explicitly assume the destination target server or not.
If the second argument is not FBE_TRANSPORT_INVALID_SERVER, we can choose this socket.
Otherwise, we have to rely on the switch the user have set*/
package_to_server_target_t	server_lookup_table[] = 
{
	{FBE_PACKAGE_ID_INVALID, FBE_TRANSPORT_INVALID_SERVER},
	{FBE_PACKAGE_ID_PHYSICAL, FBE_TRANSPORT_INVALID_SERVER},
	{FBE_PACKAGE_ID_NEIT, FBE_TRANSPORT_INVALID_SERVER},
	{FBE_PACKAGE_ID_SEP_0, FBE_TRANSPORT_INVALID_SERVER},
	{FBE_PACKAGE_ID_ESP, FBE_TRANSPORT_INVALID_SERVER},
};

/*******************************************
				Local functions
********************************************/

static void api_receive_completion_thread_function(void * context);


/*********************************************************************************************************************************/

fbe_status_t FBE_API_CALL fbe_api_transport_send_buffer(fbe_u8_t * packet,
															fbe_u32_t length,
															fbe_packet_completion_function_t completion_function,
															fbe_packet_completion_context_t  completion_context,
															fbe_package_id_t package_id)
{
    fbe_s32_t						bytes = 0;
	fbe_s32_t						rc = 0;
	fbe_api_client_send_context_t * connect_context = NULL;
	fbe_u32_t						total_length = 	0;
	fbe_u8_t *						send_buffer = NULL;
	fbe_transport_connection_target_t	server_target = FBE_TRANSPORT_INVALID_SERVER;
    fbe_packet_t *                  orig_packet = (fbe_packet_t*)completion_context;

    total_length =  length + sizeof(fbe_api_client_send_context_t);
    send_buffer = fbe_api_allocate_memory(total_length);/*we need it to send the user packet + the handle name*/

    connect_context = (fbe_api_client_send_context_t *)send_buffer;/*we stick it at the beginning*/
    connect_context->user_buffer =(void *) packet;/*we will need that later to copy the data into*/
    connect_context->completion_function = (void *)completion_function;
    connect_context->completion_context = (void *)completion_context;
    connect_context->packet_lengh = length;
    connect_context->total_msg_length = total_length;

    /* we add to the user buffer the context of the message*/
    //fbe_copy_memory(send_buffer, connect_context, sizeof(fbe_api_client_send_context_t));
    fbe_copy_memory ((fbe_u8_t*)(send_buffer + sizeof(fbe_api_client_send_context_t)), packet, length);

    /*we need to decide which server target we want to send it to.
    This depends on which mode we work. If we are in the developemnt PC, all packages are loaded into one executable
    so we have to rely on the user to set the target before he sends.
    However, if we are on the array, we will use the lookup table to know how to route the packet*/
    /* !@TODO: this code, and related code, needs to be revisited.  The server_lookup_table is not being used. */
    if (client_mode == FBE_CLIENT_MODE_DEV_PC) {
        server_target = current_target;
    }else{
        server_target = server_lookup_table[package_id].server_target;
    }

    if (fbe_api_transport_client_thread_flag[server_target] != FBE_API_TRANSPORT_CLIENT_THREAD_RUN)
    {
        /* the connection has not been initialized yet */
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s, the connection to SP: %d has not been setup yet\n", __FUNCTION__, server_target);
        fbe_api_free_memory(send_buffer);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_mutex_lock(&connect_lock[server_target]);
    /* if the SP panics, we have to drain the outstanding packet. So queue it to the dedicate outtanding queue */
    fbe_transport_set_cancel_function(orig_packet, NULL, &connect_lock[server_target]);
    fbe_transport_enqueue_packet(orig_packet, &outstanding_packet_queue[server_target]);
	bytes = 0;
	while(bytes < (fbe_s32_t)total_length){
		rc = send(connect_socket[server_target], send_buffer + bytes, total_length - bytes, 0);
		if (rc == SOCKET_ERROR) {
			fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s, send failed:%d\n", __FUNCTION__,EmcutilLastNetworkErrorGet());
			closesocket(connect_socket[server_target]);
            fbe_transport_set_cancel_function(orig_packet, NULL, NULL);
            fbe_transport_remove_packet_from_queue(orig_packet);
            fbe_mutex_unlock(&connect_lock[server_target]);
			fbe_api_free_memory(send_buffer);
			return FBE_STATUS_GENERIC_FAILURE;
		}
		bytes += rc;
	}
    fbe_mutex_unlock(&connect_lock[server_target]);
    fbe_api_free_memory(send_buffer);

    return FBE_STATUS_PENDING;
}

fbe_status_t FBE_API_CALL fbe_api_transport_unregister_all_apps(void)
{
    fbe_status_t	status = FBE_STATUS_GENERIC_FAILURE;
    fbe_api_control_operation_status_info_t	status_info;
    status = fbe_api_common_send_control_packet_to_service(FBE_SIM_SERVER_CONTROL_CODE_UNREGISTER_ALL_APPS,
                                                           NULL,
                                                           0,
                                                           FBE_SERVICE_ID_SIM_SERVER,
                                                           FBE_PACKET_FLAG_NO_ATTRIB,
                                                           &status_info,
                                                           /* package_id is not checked by the other end, just to pass thru the transport */
                                                           FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK || status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s:packet error:%d, packet qualifier:%d, payload error:%d, payload qualifier:%d\n", __FUNCTION__,
                       status, status_info.packet_qualifier, status_info.control_operation_status, status_info.control_operation_qualifier);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}
fbe_status_t fbe_api_transport_set_unregister_on_connect(fbe_bool_t b_value)
{
    b_unregister_on_connnect = b_value;
    return FBE_STATUS_OK;
}
fbe_status_t FBE_API_CALL fbe_api_transport_init_client(const char *server_name, fbe_transport_connection_target_t connect_to_sp,
                                                                            fbe_bool_t notification_enable, fbe_u32_t connect_retry_times)
{
    fbe_status_t							status = FBE_STATUS_GENERIC_FAILURE;
	fbe_transport_connection_target_t 	temp_target = FBE_TRANSPORT_INVALID_SERVER;

    fbe_queue_init(&outstanding_packet_queue[connect_to_sp]);

    fbe_transport_client_init_count++;

	temp_target = current_target;/*save it since we now may initialize a different channel and we want to get back to the defualt*/
	current_target = connect_to_sp;

    /*initialize the connection to the servers*/
    status = fbe_api_transport_init_client_connection(server_name, connect_to_sp, connect_retry_times);
    if (status != FBE_STATUS_OK) {
		fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s, fbe_api_transport_init_client_connection failed: %d\n", __FUNCTION__, status);
        current_target = temp_target;/*restore*/
        return FBE_STATUS_GENERIC_FAILURE;
    }

	if (notification_enable) {

        if (b_unregister_on_connnect){
            status = fbe_api_transport_unregister_all_apps();
            if (status != FBE_STATUS_OK) {
                fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s, fbe_api_transport_unregister_all_apps failed: %d\n", __FUNCTION__, status);
                current_target = temp_target;/*restore*/
                return FBE_STATUS_GENERIC_FAILURE;
            }
        }

		/*init notification related things*/
		status = fbe_api_transport_control_init_client_control(connect_to_sp);
		if (status != FBE_STATUS_OK) {
			fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s, fbe_api_transport_control_init_client_control failed: %d\n", __FUNCTION__, status);
			current_target = temp_target;/*restore*/
			return FBE_STATUS_GENERIC_FAILURE;
		}

		notification_enabled[connect_to_sp] = FBE_TRUE;
	}else{
		notification_enabled[connect_to_sp] = FBE_FALSE;
	}

    current_target = temp_target;/*restore*/

    return FBE_STATUS_OK;

}
fbe_status_t FBE_API_CALL fbe_api_transport_init_client_connection(const char *server_name, fbe_transport_connection_target_t connect_to_sp,
                                                                                                    fbe_u32_t connect_retry_times)
{
    fbe_u32_t			result = 0;
    fbe_status_t		status = FBE_STATUS_GENERIC_FAILURE;
    struct addrinfo *	addr_info_ptr = NULL;
    struct addrinfo 	hints;
    fbe_s32_t retry_times_left = connect_retry_times == 0 ? -1 : connect_retry_times;
    fbe_api_transport_client_thread_flag[connect_to_sp] = FBE_API_TRANSPORT_CLIENT_THREAD_DONE;
    
	/*what port should we use, based on the server we connect to*/
	status = fbe_api_transport_map_server_mode_to_port(connect_to_sp, &fbe_transport_port);
	if (status != FBE_STATUS_OK) {
		fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s, failed to map server target to port\n", __FUNCTION__);
		return FBE_STATUS_GENERIC_FAILURE;
	}
    
    // Initialize Winsock
    result = EmcutilNetworkWorldInitializeMaybe();
    if (result != 0) {
		fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s, WSAStartup failed: %d\n", __FUNCTION__, result);
		return FBE_STATUS_GENERIC_FAILURE;
    }
    
    fbe_zero_memory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    
    result = getaddrinfo(server_name, fbe_transport_port, &hints, &addr_info_ptr);
    if (result != 0) {
		fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s getaddrinfo failed: %d\n", __FUNCTION__, result);
		EmcutilNetworkWorldDeinitializeMaybe();
		return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_mutex_init(&connect_lock[connect_to_sp]);
    connect_socket[connect_to_sp] = INVALID_SOCKET;

    do
    {
        if(connect_socket[connect_to_sp] == INVALID_SOCKET)
        {
            /*Create a SOCKET for connecting to server*/
            connect_socket[connect_to_sp] = socket(addr_info_ptr->ai_family, addr_info_ptr->ai_socktype, addr_info_ptr->ai_protocol);
        }

        if (connect_socket[connect_to_sp] == INVALID_SOCKET) {
            fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "Error at socket(): %d\n", EmcutilLastNetworkErrorGet());
            freeaddrinfo(addr_info_ptr);
            EmcutilNetworkWorldDeinitializeMaybe();
            return FBE_STATUS_GENERIC_FAILURE;
        }
        
        /* Connect to server.*/
        result = connect(connect_socket[connect_to_sp], addr_info_ptr->ai_addr, (int)addr_info_ptr->ai_addrlen);
        if(result == SOCKET_ERROR)
        {
            if(retry_times_left > 0)
            {
                fbe_api_trace(FBE_TRACE_LEVEL_WARNING,"%s, Can't connect to server:%d, error:%d  Retry in 1 sec.[%d time(s) left]\n",
                                    __FUNCTION__, connect_to_sp, EmcutilLastNetworkErrorGet(), retry_times_left);
                fbe_thread_delay(1000);
                --retry_times_left;
                continue;
            }
            else if(retry_times_left == 0)
            {
                fbe_api_trace(FBE_TRACE_LEVEL_ERROR,"%s, Unable to connect to server:%d, error:%d.\n", __FUNCTION__, connect_to_sp, EmcutilLastNetworkErrorGet());
                freeaddrinfo(addr_info_ptr);
                closesocket(connect_socket[connect_to_sp]);
                connect_socket[connect_to_sp] = INVALID_SOCKET;
                EmcutilNetworkWorldDeinitializeMaybe();
                return FBE_STATUS_GENERIC_FAILURE;
            }
            else
            {
                fbe_api_trace(FBE_TRACE_LEVEL_WARNING,"%s, Can't connect to server:%d, error:%d  Retry in 1 sec.\n", __FUNCTION__, connect_to_sp, EmcutilLastNetworkErrorGet());
                fbe_thread_delay(1000);
                continue;
            }
        }


        /*now we start the thread that will accept calls and server them*/
        fbe_api_transport_client_thread_flag[connect_to_sp] = FBE_API_TRANSPORT_CLIENT_THREAD_RUN;
        fbe_thread_init(&receive_completion_thread[connect_to_sp], "fbe_recv_compl", api_receive_completion_thread_function, (void *)connect_to_sp);
        
        fbe_thread_delay(5000);
        
    }while(result == SOCKET_ERROR || fbe_api_transport_client_thread_flag[connect_to_sp] == FBE_API_TRANSPORT_CLIENT_THREAD_EXCEPTION);
    
    freeaddrinfo(addr_info_ptr);

    return FBE_STATUS_OK;
}

fbe_status_t FBE_API_CALL fbe_api_transport_destroy_client(fbe_transport_connection_target_t connect_to_sp)
{
	fbe_transport_connection_target_t	previous_target;
    LARGE_INTEGER       timeout;
    timeout.QuadPart = -10000 * 1;/*wait 1 msc for thread end*/
    
	if (connect_socket[connect_to_sp] == INVALID_SOCKET) { /* FBE30  */
		/*we never connected, do nothing*/
		fbe_api_trace(FBE_TRACE_LEVEL_INFO, "%s trying to disconnect a clinet we did not connect to: %d\n", __FUNCTION__, connect_to_sp);
		return FBE_STATUS_OK;
	}
	fbe_transport_client_init_count--;
    
	previous_target = current_target;
	current_target= connect_to_sp;

    /*first we destroy the client control part which is mostly in charge of notifications*/
	if (notification_enabled[connect_to_sp]) {
		fbe_api_transport_control_destroy_client_control(connect_to_sp);
	}

    fbe_api_transport_destroy_client_connection(connect_to_sp);

	current_target = previous_target;/*restore what we changed*/

	if (fbe_transport_client_init_count == 0) {
		EmcutilNetworkWorldDeinitializeMaybe();
	}

    return FBE_STATUS_OK;

}

fbe_status_t FBE_API_CALL fbe_api_transport_destroy_client_connection(fbe_transport_connection_target_t connect_to_sp)
{
    /*flag the thread we should stop*/
    fbe_api_transport_client_thread_flag[connect_to_sp] = FBE_API_TRANSPORT_CLIENT_THREAD_STOP;
    
	closesocket(connect_socket[connect_to_sp]);

    /*wait for it to end*/
    fbe_thread_delay(100);
    fbe_thread_wait(&receive_completion_thread[connect_to_sp]); /* SAFEBUG - this was not being done before */
    fbe_thread_destroy(&receive_completion_thread[connect_to_sp]);

    fbe_mutex_destroy(&connect_lock[connect_to_sp]); /* SAFEBUG - moved to proper place */

    /* Clean up client outstanding packet queue
     */
    fbe_api_sim_transport_cleanup_client_outstanding_packet(connect_to_sp);

	return FBE_STATUS_OK;
}

/* SAFEBUG - had to add this to allow recv() to come home occasionally so thread termination can happen */
static int recv_with_timeout(SOCKET sockfd, void *buf, size_t len, int timeout_secs, int *bytes_rv)
{
    FD_SET recv_set;
    FD_SET exc_set;
    struct timeval tmout;
    int rc;

    *bytes_rv = 0;

again:
    tmout.tv_sec = timeout_secs;
    tmout.tv_usec = 0;
    FD_ZERO(&recv_set);
    FD_ZERO(&exc_set);
    FD_SET(sockfd, &recv_set);
    FD_SET(sockfd, &exc_set);
#ifdef _MSC_VER
    rc = select(0, &recv_set, NULL, &exc_set, &tmout);
    if (rc < 0 && EmcutilLastNetworkErrorGet() == WSAEINTR) {
         goto again;
    }
#else
    rc = select(sockfd+1, &recv_set, NULL, &exc_set, &tmout);
    if (rc < 0 && errno == EINTR) {
         goto again;
    }
#endif
    if (rc == 0) {
        return 1;
    } else {
        *bytes_rv = (int) recv(sockfd, buf, (int)len, 0);
        return 0;
    }
}


static void api_receive_completion_thread_function(void * context)
{
    fbe_transport_connection_target_t target = (fbe_transport_connection_target_t)context;
    fbe_s32_t                               bytes = 0;
    fbe_u8_t *                              rec_buf = NULL;
    fbe_u8_t *                              temp_buf = NULL;
    fbe_api_client_send_context_t           connect_context;
    fbe_u32_t                               total_bytes = 0, bytes_left;
    fbe_u32_t                               bytes_to_receive = 0;
    fbe_u32_t                               original_packet_length;
    fbe_u32_t                               system_error = 0;
    fbe_u32_t                               send_context_size = sizeof(fbe_api_client_send_context_t);
    fbe_u32_t                               result = 0;

    /* this buffer is too big to live on a stack */
    rec_buf = fbe_api_allocate_memory(FBE_API_CLIENT_REC_BUF_SIZE);
    if (rec_buf==NULL) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s, failed to allocate receive buffer!\n", __FUNCTION__);
        closesocket(connect_socket[target]);
        connect_socket[target] = INVALID_SOCKET;
        fbe_api_transport_client_thread_flag[target] = FBE_API_TRANSPORT_CLIENT_THREAD_EXCEPTION;
        fbe_thread_exit(EMCPAL_STATUS_SUCCESS);
    }

    while(fbe_api_transport_client_thread_flag[target] == FBE_API_TRANSPORT_CLIENT_THREAD_RUN)
    {
        total_bytes = 0;
        bytes_to_receive = send_context_size;
        temp_buf = (fbe_u8_t *)&connect_context;

        /*first of all we read the context*/
        do{
            int timedout;
            do {
                timedout = recv_with_timeout(connect_socket[target], rec_buf, bytes_to_receive, 1, &bytes);
            } while (timedout && (fbe_api_transport_client_thread_flag[target] == FBE_API_TRANSPORT_CLIENT_THREAD_RUN));

            if (bytes <= 0){
                system_error = EmcutilLastNetworkErrorGet();
                if (system_error == WSAECONNABORTED){
                    fbe_api_trace(FBE_TRACE_LEVEL_INFO, "%s, closing thread WSAECONNABORTED \n", __FUNCTION__);
                }else{
                    fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s, recv header failed: %d, exiting !\n", __FUNCTION__, system_error);
                }
                
                result = shutdown(connect_socket[target], SD_BOTH);
                if (result == SOCKET_ERROR)
                {
                    fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "shutdown connection failed: %d\n", result);
                }
                 closesocket(connect_socket[target]);
                 connect_socket[target] = INVALID_SOCKET;
                 fbe_api_free_memory (rec_buf);
				 fbe_api_transport_client_thread_flag[target] = FBE_API_TRANSPORT_CLIENT_THREAD_EXCEPTION;
                 fbe_thread_exit(EMCPAL_STATUS_SUCCESS);
                 return;
            }

            total_bytes += bytes;
            fbe_copy_memory(temp_buf, rec_buf, bytes);
            temp_buf += bytes;

            bytes_to_receive -= bytes;

        }while (bytes_to_receive>0);

        /*now that we read the context, we are ready to read the rest of the message*/
        total_bytes = 0;
        original_packet_length = connect_context.total_msg_length;
        bytes_left = original_packet_length - send_context_size;
        temp_buf = (fbe_u8_t*)connect_context.user_buffer;

        if (bytes_left > FBE_API_CLIENT_REC_BUF_SIZE) {
            bytes_to_receive = FBE_API_CLIENT_REC_BUF_SIZE;
        }else{
            bytes_to_receive = bytes_left;
        }

        do {
            int timedout;
            do {
                timedout = recv_with_timeout(connect_socket[target], rec_buf, bytes_to_receive, 1, &bytes);
            } while (timedout && (fbe_api_transport_client_thread_flag[target] == FBE_API_TRANSPORT_CLIENT_THREAD_RUN));
            if (bytes > 0){
                fbe_copy_memory(temp_buf, rec_buf, bytes);
                temp_buf+= bytes;
                total_bytes += bytes;

                bytes_left -= bytes;
                if (bytes_left > FBE_API_CLIENT_REC_BUF_SIZE) {
                    bytes_to_receive = FBE_API_CLIENT_REC_BUF_SIZE;
                }else{
                    bytes_to_receive = bytes_left;
                }
            }else if (bytes == 0){
                system_error = EmcutilLastNetworkErrorGet();
                if (system_error == WSAECONNABORTED){
                    fbe_api_trace(FBE_TRACE_LEVEL_INFO, "%s, closing thread\n", __FUNCTION__);
                }else{
                    fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s, recv failed: %d, exiting !\n", __FUNCTION__, system_error);
                }
                result = shutdown(connect_socket[target], SD_BOTH);
                if (result == SOCKET_ERROR)
                {
                    fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "shutdown connection failed: %d\n", result);
                }
                closesocket(connect_socket[target]);
                connect_socket[target] = INVALID_SOCKET;
                fbe_api_free_memory (rec_buf);
                fbe_api_transport_client_thread_flag[target] = FBE_API_TRANSPORT_CLIENT_THREAD_EXCEPTION;
                fbe_thread_exit(EMCPAL_STATUS_SUCCESS);
                return;
            } else {
                fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s, recv failed: %d\n", __FUNCTION__, EmcutilLastNetworkErrorGet());
                result = shutdown(connect_socket[target], SD_BOTH);
                if (result == SOCKET_ERROR)
                {
                    fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "shutdown connection failed: %d\n", result);
                }
                closesocket(connect_socket[target]);
                connect_socket[target] = INVALID_SOCKET;
                fbe_api_free_memory (rec_buf);
                fbe_api_transport_client_thread_flag[target] = FBE_API_TRANSPORT_CLIENT_THREAD_EXCEPTION;
                fbe_thread_exit(EMCPAL_STATUS_SUCCESS);
                 return;
            }
        }while ((bytes > 0) && (bytes_left > 0));


        
        /*and call the completion function once the buffer is full*/
        ((fbe_packet_completion_function_t)connect_context.completion_function)((fbe_packet_t *)connect_context.user_buffer, (fbe_packet_completion_context_t)connect_context.completion_context);

    }

    result = shutdown(connect_socket[target], SD_BOTH);
    if (result == SOCKET_ERROR)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "shutdown connection failed: %d\n", result);
    }

    closesocket(connect_socket[target]);
    connect_socket[target] = INVALID_SOCKET;
    fbe_api_free_memory (rec_buf);
    fbe_api_transport_client_thread_flag[target] = FBE_API_TRANSPORT_CLIENT_THREAD_DONE;
    EmcutilNetworkWorldDeinitializeMaybe();
    fbe_thread_exit(EMCPAL_STATUS_SUCCESS);
}

void FBE_API_CALL fbe_api_transport_reset_client(fbe_client_mode_t client_mode_in)
{
	fbe_transport_connection_target_t target = 	FBE_TRANSPORT_INVALID_SERVER;

	for (;target < FBE_TRANSPORT_LAST_CONNECTION_TARGET; target++) {
		connect_socket[target] = INVALID_SOCKET;
	}

	client_mode = client_mode_in;

	/*by defualt, to support all testing, we set the default target to SPA*/
	if (client_mode_in == FBE_CLIENT_MODE_DEV_PC) {
		current_target = FBE_TRANSPORT_SP_A;
	}
}

void FBE_API_CALL fbe_api_transport_reset_client_sp(fbe_sim_transport_connection_target_t target)
{
	connect_socket[target] = INVALID_SOCKET;
    // set the target to the peer sp
    current_target = (target == FBE_SIM_SP_A? FBE_TRANSPORT_SP_A : FBE_TRANSPORT_SP_B);
}

/*pay attention this function is not thread safe, if you use it before sending a command another thread might
switch it before you send the command so use it under a lock.*/
fbe_status_t FBE_API_CALL fbe_api_transport_set_target_server(fbe_transport_connection_target_t target)
{
	if (target <= FBE_TRANSPORT_INVALID_SERVER || target >= FBE_TRANSPORT_LAST_CONNECTION_TARGET) {
		return FBE_STATUS_GENERIC_FAILURE;
	}else{ 
        current_target = target;
		return FBE_STATUS_OK;
	}
}

fbe_transport_connection_target_t FBE_API_CALL fbe_api_transport_get_target_server(void)
{
    return current_target;
		
}
fbe_bool_t FBE_API_CALL fbe_api_transport_is_target_initted(fbe_transport_connection_target_t target)
{
    return (connect_socket[target] != INVALID_SOCKET);
}
fbe_status_t FBE_API_CALL fbe_api_transport_send_notification_buffer(fbe_u8_t * packet,
																		 fbe_u32_t length,
																		 fbe_packet_completion_function_t completion_function,
																		 fbe_packet_completion_context_t  completion_context,
																		 fbe_package_id_t package_id,
																		 fbe_transport_connection_target_t	server_target)
{
    fbe_s32_t						bytes = 0;
    fbe_s32_t						rc = 0;

	fbe_api_client_send_context_t * connect_context = NULL;
	fbe_u32_t						total_length = 	0;
	fbe_u8_t *						send_buffer = NULL;
    fbe_packet_t *                  orig_packet = (fbe_packet_t*)completion_context;
    

	total_length = 	length + sizeof(fbe_api_client_send_context_t);
	send_buffer = fbe_api_allocate_memory(total_length);/*we need it to send the user packet + the handle name*/

	connect_context = (fbe_api_client_send_context_t *)send_buffer;/*we stick it at the beginning*/
    connect_context->user_buffer = (void *)packet;/*we will need that later to copy the data into*/
	connect_context->completion_function = (void *)completion_function;
	connect_context->completion_context = (void *)completion_context;
    connect_context->packet_lengh = length;
    connect_context->total_msg_length = total_length;

    /* we add to the user buffer the packate lengh*/
    //fbe_copy_memory(send_buffer, connect_context, sizeof(fbe_api_client_send_context_t));
	fbe_copy_memory ((fbe_u8_t*)(send_buffer + sizeof(fbe_api_client_send_context_t)), packet, length);

	/*  Need to decide which server target we want to send it to.
	This depends on which mode we work. If we are in the developemnt PC, all packages are loaded into one executable
	for notification packet, we don't use current target, the server target is passed in.
	However, if we are on the array, we will use the lookup table to know how to route the packet*/
	if (client_mode != FBE_CLIENT_MODE_DEV_PC) {
		server_target = server_lookup_table[package_id].server_target;
	}

    fbe_mutex_lock(&connect_lock[server_target]);

    /* if the SP panics, we have to drain the outstanding packet. So queue it to the dedicate outtanding queue */
    fbe_transport_set_cancel_function(orig_packet, NULL, &connect_lock[server_target]);
    fbe_transport_enqueue_packet(orig_packet, &outstanding_packet_queue[server_target]);
	bytes = 0;
	while(bytes < (fbe_s32_t)total_length){
		rc = send(connect_socket[server_target], send_buffer + bytes, total_length - bytes, 0);
		if (rc == SOCKET_ERROR) {
			fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s, send failed:%d\n", __FUNCTION__,EmcutilLastNetworkErrorGet());
			closesocket(connect_socket[server_target]);
            fbe_transport_set_cancel_function(orig_packet, NULL, NULL);
            fbe_transport_remove_packet_from_queue(orig_packet);
            fbe_mutex_unlock(&connect_lock[server_target]);
			fbe_api_free_memory(send_buffer);
			return FBE_STATUS_GENERIC_FAILURE;
		}
		bytes += rc;
	}
    fbe_mutex_unlock(&connect_lock[server_target]);

	fbe_api_free_memory(send_buffer);

    return FBE_STATUS_PENDING;
}

/**************************************
           Wrapper functions for sim mode
**************************************/
fbe_sim_transport_connection_target_t FBE_API_CALL fbe_api_sim_transport_get_target_server(void)
{
    return fbe_api_transport_get_target_server();
}

fbe_status_t FBE_API_CALL fbe_api_sim_transport_set_target_server(fbe_sim_transport_connection_target_t target)
{
    fbe_transport_connection_target_t transport_target = fbe_transport_translate_fbe_sim_connection_target_to_fbe_transport_connection_target(target);
    return fbe_api_transport_set_target_server(transport_target);
}

fbe_status_t FBE_API_CALL fbe_api_sim_transport_init_client(fbe_sim_transport_connection_target_t connect_to_sp, fbe_bool_t notification_enable)
{
    fbe_status_t status;
    status = fbe_api_transport_init_client("127.0.0.1", connect_to_sp, notification_enable, -1); // currently we sent the connection retry time out to infinite

    if (status == FBE_STATUS_OK) 
    { 
        status = fbe_api_common_init_job_notification();/*Make sure jobs will notify us*/
    }
    return status; 
}

fbe_status_t FBE_API_CALL fbe_api_sim_transport_destroy_client(fbe_sim_transport_connection_target_t connect_to_sp)
{
    return fbe_api_transport_destroy_client(connect_to_sp);
}

void FBE_API_CALL fbe_api_sim_transport_reset_client(fbe_client_mode_t client_mode_in)
{
    fbe_api_transport_reset_client(client_mode_in);
}

void FBE_API_CALL fbe_api_sim_transport_reset_client_sp(fbe_sim_transport_connection_target_t target)
{
	fbe_api_transport_reset_client_sp(target);
}

void FBE_API_CALL fbe_api_sim_transport_cleanup_client_outstanding_packet(fbe_sim_transport_connection_target_t target)
{
    fbe_queue_element_t *queue_element_p;
    fbe_packet_t *current_packet_p;
    /* Loop over the termination queue and break out when we find something that is not quiesced.  */
    queue_element_p = fbe_queue_front(&outstanding_packet_queue[target]);
    while (queue_element_p != NULL)
    {
        current_packet_p = fbe_transport_queue_element_to_packet(queue_element_p);
        queue_element_p = fbe_queue_next(&outstanding_packet_queue[target], &current_packet_p->queue_element);
        fbe_transport_remove_packet_from_queue(current_packet_p);
        fbe_transport_set_cancel_function(current_packet_p, NULL, NULL);
        fbe_transport_set_status(current_packet_p, FBE_STATUS_CANCELED, 0);
        fbe_transport_complete_packet(current_packet_p);
        fbe_api_trace(FBE_TRACE_LEVEL_INFO, "%s, cancel outstanding packet:0x%p\n", __FUNCTION__, current_packet_p);
    }
}




