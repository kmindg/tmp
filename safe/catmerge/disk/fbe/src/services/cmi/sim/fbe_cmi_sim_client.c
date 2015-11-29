/***************************************************************************
 * Copyright (C) EMC Corporation 2001-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *  fbe_cmi_sim_client.c
 ***************************************************************************
 *
 *  Description
 *      client implementation for CMI simulation service
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
#include "fbe/fbe_queue.h"
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
#define FBE_CMI_CLIENT_MAX_CONNECT_ATTEMPTS  3
#define CMI_SIM_RECEIVE_BUF_CHECK_IDLE_TIMER 1
#define MAX_SENT_MSG_TRACK 600

#define FBE_CMI_MAX_RECEIVE_COMPLETION_START_MSECS 10000

typedef volatile enum fbe_cmi_sim_client_thread_flag_e{
    FBE_CMI_SIM_CLIENT_THREAD_INVALID,
	FBE_CMI_SIM_CLIENT_THREAD_INITIALIZING,
    FBE_CMI_SIM_CLIENT_THREAD_RUN,
    FBE_CMI_SIM_CLIENT_THREAD_STOP,
    FBE_CMI_SIM_CLIENT_THREAD_DONE
}fbe_cmi_sim_client_thread_flag_t;

typedef struct client_connection_info_s{
    struct addrinfo *   addr_info_ptr;
    fbe_cmi_conduit_id_t conduit_id;
}client_connection_info_t;

/* Static members */
static SOCKET                               connect_socket[FBE_CMI_CONDUIT_ID_LAST];
static fbe_cmi_sim_client_thread_flag_t     fbe_cmi_sim_client_thread_flag[FBE_CMI_CONDUIT_ID_LAST];
static fbe_thread_t                         fbe_cmi_sim_client_receive_completion_thread[FBE_CMI_CONDUIT_ID_LAST];
//static fbe_thread_t                           fbe_cmi_sim_client_connection_thread[FBE_CMI_CONDUIT_ID_LAST];
static fbe_cmi_sim_tcp_message_t *          fbe_cmi_sim_client_sent_messages[FBE_CMI_CONDUIT_ID_LAST][MAX_SENT_MSG_TRACK];
static fbe_u32_t                            fbe_cmi_sim_client_sent_msg_idx[FBE_CMI_CONDUIT_ID_LAST];
static fbe_atomic_t                         fbe_cmi_sim_outstanding_message_counter[FBE_CMI_CONDUIT_ID_LAST];
static fbe_spinlock_t                       send_idx_lock[FBE_CMI_CONDUIT_ID_LAST];
/* Forward declerations */
static void cmi_sim_client_receive_completion_thread_function(void * context);
static void cmi_sim_client_receive_buffer_dispatch_queue(fbe_cmi_conduit_id_t conduit_id);
static void cmi_sim_client_process_message(fbe_cmi_sim_tcp_message_t *cmi_tcp_message);
static void cmi_sim_client_init_connection(client_connection_info_t connection_info);


/*************************************************************************************************************************/
fbe_status_t fbe_cmi_sim_init_spinlock(fbe_cmi_conduit_id_t conduit_id)
{
    if(conduit_id < FBE_CMI_CONDUIT_ID_LAST ) 
    {
        fbe_spinlock_init(&send_idx_lock[conduit_id]);
        return FBE_STATUS_OK;
    }
    return FBE_STATUS_GENERIC_FAILURE;
}
fbe_status_t fbe_cmi_sim_init_conduit_client(fbe_cmi_conduit_id_t conduit_id, fbe_cmi_sp_id_t sp_id)
{
    fbe_u32_t                   result = 0;
    fbe_u32_t                   entry = 0;
    fbe_status_t                status = FBE_STATUS_GENERIC_FAILURE;
    struct addrinfo *           addr_info_ptr = NULL;
    struct addrinfo             hints;
    const fbe_u8_t *            fbe_cmi_sim_port;
    client_connection_info_t    connection_info;
    fbe_package_id_t            package_id;

	/*not perfect but good enough for now*/
    fbe_cmi_sim_client_thread_flag[conduit_id] = FBE_CMI_SIM_CLIENT_THREAD_INITIALIZING;

    fbe_get_package_id(&package_id);
	fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, "%s, conduit:%d, flag=%d, package:%d\n", 
                  __FUNCTION__, conduit_id, fbe_cmi_sim_client_thread_flag[conduit_id],
                  package_id);

    fbe_cmi_sim_client_sent_msg_idx[conduit_id] = 0;
	 fbe_cmi_sim_outstanding_message_counter[conduit_id] = 0;
	 for (entry = 0; entry < MAX_SENT_MSG_TRACK; entry++) {
		 fbe_cmi_sim_client_sent_messages[conduit_id][entry] = NULL;
	
	 }
    
    /*what port should we use, based on the server we connect to*/
    status = fbe_cmi_sim_map_conduit_to_client_port(conduit_id, &fbe_cmi_sim_port, sp_id);
    if (status != FBE_STATUS_OK) {
        fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR, "%s, failed to map server target to port. Conduit:%d, package:%d\n", 
                      __FUNCTION__, conduit_id, package_id);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, 
                  "SP%s CMI client attempts to connect to port %s. Conduit:%d, package:%d\n", 
                  (sp_id == FBE_CMI_SP_ID_A ? "A" : "B"), fbe_cmi_sim_port, conduit_id, package_id);

    fbe_zero_memory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    
    result = getaddrinfo("localhost", fbe_cmi_sim_port, &hints, &addr_info_ptr);
    if (result != 0) {
        fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR, "%s getaddrinfo failed: %d\n", __FUNCTION__, result);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /*Create a SOCKET for connecting to server*/
    connect_socket[conduit_id] = socket(addr_info_ptr->ai_family, addr_info_ptr->ai_socktype, addr_info_ptr->ai_protocol);
    
    if (connect_socket[conduit_id] == INVALID_SOCKET) {
        fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR, "Error at socket(): %d\n",
		      EmcutilLastNetworkErrorGet());
        freeaddrinfo(addr_info_ptr);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    
    connection_info.addr_info_ptr = addr_info_ptr;
    connection_info.conduit_id = conduit_id;

    cmi_sim_client_init_connection(connection_info);

    return FBE_STATUS_OK;

}

fbe_status_t fbe_cmi_sim_destroy_conduit_client(fbe_cmi_conduit_id_t conduit_id)
{
    fbe_bool_t is_thread_wait_needed = FBE_FALSE;
    fbe_package_id_t            package_id;

    fbe_get_package_id(&package_id);
    /* Make sure the thread was created(!= *_INVALID) and not destroyed(!= *_DONE) then
     * we need to destroy it
     */
    if((fbe_cmi_sim_client_thread_flag[conduit_id] != FBE_CMI_SIM_CLIENT_THREAD_INVALID) &&
       (fbe_cmi_sim_client_thread_flag[conduit_id] != FBE_CMI_SIM_CLIENT_THREAD_DONE)) 
    {
        is_thread_wait_needed = FBE_TRUE;
    }
    
    /*flag the theread we should stop*/
    fbe_cmi_sim_client_thread_flag[conduit_id] = FBE_CMI_SIM_CLIENT_THREAD_STOP;

    fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, 
                  "%s Destroy Client Conduit:%d, package:%d\n", 
                  __FUNCTION__, conduit_id, package_id);

    shutdown(connect_socket[conduit_id], SD_BOTH);
    closesocket (connect_socket[conduit_id]);
    
     
    fbe_cmi_sim_outstanding_message_counter[conduit_id] = 0;
    
    /*we have to be carefull with that, if we started and closed quick it may have not even started*/
    if (is_thread_wait_needed) {
        fbe_thread_wait(&fbe_cmi_sim_client_receive_completion_thread[conduit_id]);
        fbe_thread_destroy(&fbe_cmi_sim_client_receive_completion_thread[conduit_id]);
    }

    fbe_spinlock_destroy(&send_idx_lock[conduit_id]);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_cmi_sim_client_send_message_buffer_overrun(fbe_cmi_sim_tcp_message_t *tcp_message)
{
    fbe_cmi_sim_tcp_message_t  *current_message = NULL;
    fbe_u32_t                   index = 0;
    fbe_cmi_conduit_id_t        conduit;
    fbe_cmi_client_id_t 	    client_id;
    fbe_u32_t                   start_index;
    fbe_u32_t                   end_index; 

    /*! @note Assumption is that ring buffer lock has been taken*/
    index = tcp_message->msg_idx;
    conduit = tcp_message->conduit;
    client_id = tcp_message->client_id;
    fbe_cmi_trace(FBE_TRACE_LEVEL_WARNING, 
                  "cmi: sim - send conduit: %d msg count: %d client: %d slot(index): %d out of %d in use.\n",
                  conduit, fbe_cmi_sim_client_sent_msg_idx[conduit],
                  client_id, index, MAX_SENT_MSG_TRACK);
    current_message = fbe_cmi_sim_client_sent_messages[conduit][index];
    fbe_cmi_trace(FBE_TRACE_LEVEL_WARNING,
                  "cmi: sim - send slot: %d in use by: 0x%p client: %d new request: 0x%p \n",
                  (fbe_cmi_sim_client_sent_msg_idx[conduit] % MAX_SENT_MSG_TRACK), 
                  current_message, current_message->client_id, tcp_message);

    /* Dump the previous and next (4) entries */
    start_index = ((fbe_s32_t)index < 4) ? 0 : (index - 4);
    end_index =  (index > (MAX_SENT_MSG_TRACK - 5)) ? (MAX_SENT_MSG_TRACK - 4) : (index + 4);
    for (index = start_index; index < end_index; index++)
    {
        current_message = fbe_cmi_sim_client_sent_messages[conduit][index];
        fbe_cmi_trace(FBE_TRACE_LEVEL_INFO,
                      "cmi: sim - [%3d] tcp_message: 0x%p conduit: %d client_id: %d \n",
                      index, current_message,
                      (current_message == NULL) ? -1 : current_message->conduit,
                      (current_message == NULL) ? -1 : current_message->client_id);
    }

    /* If the first and last slots are in use return `no resources'*/
    if ((fbe_cmi_sim_client_sent_messages[conduit][0] != NULL)                        &&
        (fbe_cmi_sim_client_sent_messages[conduit][(MAX_SENT_MSG_TRACK - 1)] != NULL)    )
    {
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Else return generic failure*/
    return FBE_STATUS_GENERIC_FAILURE;
}

fbe_status_t fbe_cmi_sim_client_send_message (fbe_cmi_sim_tcp_message_t *tcp_message)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_s32_t                   bytes = 0, rc = 0;
    fbe_u8_t                   *message_buffer = NULL;
    fbe_u8_t                   *tmp_ptr = NULL;
    fbe_cmi_sim_tcp_message_t  *orig_tcp_message = NULL;
    fbe_cmi_sim_tcp_message_t  *tmp_tcp_message = NULL;
    fbe_package_id_t            package_id;

    fbe_get_package_id(&package_id);

    if (!fbe_cmi_is_peer_alive()) {
        return FBE_STATUS_NO_DEVICE;/*this status means the other SP is dead*/
    }

    /* Allocate a temporary buffer to transmit the message.*/
    message_buffer = malloc (tcp_message->total_message_lenth);
    tmp_ptr = message_buffer;
    
    /*before we send the message we put it's pointer in a ring buffer so we can sent
    abort messages for all the ones that are sent but then the other side went down before returning
    the messages back. we have to lock because two threads might try to send at the same time and 
    incrementing fbe_cmi_sim_client_sent_msg_idx should be atomic with sending*/
    fbe_spinlock_lock(&send_idx_lock[tcp_message->conduit]);
    
    /*this will tell the receiver it got a message*/
    tcp_message->event_id = FBE_CMI_EVENT_MESSAGE_RECEIVED;
    tcp_message->msg_idx = fbe_cmi_sim_client_sent_msg_idx[tcp_message->conduit] % MAX_SENT_MSG_TRACK;
    if (fbe_cmi_sim_client_sent_messages[tcp_message->conduit][tcp_message->msg_idx] != NULL) {
        status = fbe_cmi_sim_client_send_message_buffer_overrun(tcp_message);              
        fbe_spinlock_unlock(&send_idx_lock[tcp_message->conduit]);
        fbe_cmi_trace(FBE_TRACE_LEVEL_WARNING, "%s, ring buffer overflow, idx: %d status: %d\n", 
                      __FUNCTION__, tcp_message->msg_idx, status);
        fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, 
                      "%s Conduit:%d, package:%d\n", 
                      __FUNCTION__, tcp_message->conduit, package_id);
        free (message_buffer);
        return status;
    }

    /* Increment the sent count to the next. */
    fbe_cmi_sim_client_sent_msg_idx[tcp_message->conduit]++;

    /*do some serialization*/
    orig_tcp_message = tcp_message;
    tmp_tcp_message = (fbe_cmi_sim_tcp_message_t  *)message_buffer;
    fbe_copy_memory(tmp_tcp_message, tcp_message, sizeof(fbe_cmi_sim_tcp_message_t));
    tmp_ptr += sizeof(fbe_cmi_sim_tcp_message_t);
    if (tcp_message->msg_type != FBE_CMI_SIM_TCP_MESSAGE_TYPE_WITH_FIXED_DATA)
    {
        fbe_copy_memory(tmp_ptr, tcp_message->user_message, (tcp_message->total_message_lenth - sizeof(fbe_cmi_sim_tcp_message_t)));
    }
    else
    {
        /* For message with fixed data transfer */
        fbe_u32_t message_length;
        fbe_u32_t bytes_remaining, bytes_to_transfer;
        fbe_sg_element_t * sg_list = tcp_message->sg_list;

        message_length = tcp_message->total_message_lenth - sizeof(fbe_cmi_sim_tcp_message_t) - tcp_message->fixed_data_length;
        fbe_copy_memory(tmp_ptr, tcp_message->user_message, message_length);
        tmp_ptr += message_length;
        if (sg_list != NULL)
        {
            /* We need to limit the transfer to what was asked.  It is possible for the sg list to either not be null 
             * terminated or have more data in it, so we should not transfer beyond the byte count requested. 
             */
            bytes_remaining = tcp_message->fixed_data_length;
            while ((sg_list->count != 0) && (bytes_remaining != 0)) {
                bytes_to_transfer = FBE_MIN(sg_list->count, bytes_remaining);
                fbe_copy_memory(tmp_ptr, sg_list->address, bytes_to_transfer);
                tmp_ptr += sg_list->count;
                sg_list++;
                bytes_remaining -= bytes_to_transfer;
            }
        }
        else if (tcp_message->control_buffer != NULL)
        {
            fbe_copy_memory(tmp_ptr, tcp_message->control_buffer, tcp_message->fixed_data_length);
        }
    }

    /* Flag the fact that this slot is in use. */
    fbe_cmi_sim_client_sent_messages[tcp_message->conduit][tcp_message->msg_idx] = tcp_message;
    fbe_atomic_increment(&fbe_cmi_sim_outstanding_message_counter[tcp_message->conduit]);


    bytes = 0;
    while(bytes < tmp_tcp_message->total_message_lenth){
        rc = send(connect_socket[tmp_tcp_message->conduit], message_buffer + bytes, tmp_tcp_message->total_message_lenth - bytes, 0);
        if (rc == SOCKET_ERROR) {
            /* We can get transmission failures log the message but
             * take recovery action under lock below.
             */
            //closesocket(conne
            fbe_cmi_trace(FBE_TRACE_LEVEL_WARNING, "%s, send conduit: %d index: %d failed:%d, package:%d\n", 
                          __FUNCTION__, tmp_tcp_message->conduit, tmp_tcp_message->msg_idx, EmcutilLastNetworkErrorGet(),package_id);
            status = FBE_STATUS_IO_FAILED_RETRYABLE;
            break;
        }
        bytes += rc;
    }

    /* on linux, we could not guarentee concurrent send to a socket is atomic, we has to use lock to guarentee that */
    fbe_spinlock_unlock(&send_idx_lock[tcp_message->conduit]);

    /* If the transmission failed we will execute the cleanup here.
     */
    if (status != FBE_STATUS_OK) {
        /* First take the send lock */
        fbe_spinlock_lock(&send_idx_lock[tmp_tcp_message->conduit]);

        /* Now determine if we have already processed the completion.*/
        if (fbe_cmi_sim_client_sent_messages[tmp_tcp_message->conduit][tmp_tcp_message->msg_idx] != orig_tcp_message) {
            /* Log a message and simply free the local message and return success.*/
            fbe_cmi_trace(FBE_TRACE_LEVEL_WARNING, "%s, Current conduit: %d msg_idx: %d msg: 0x%p not orig: 0x%p, pkg:%d\n", 
                          __FUNCTION__, tmp_tcp_message->conduit, tmp_tcp_message->msg_idx,
                          fbe_cmi_sim_client_sent_messages[tmp_tcp_message->conduit][tmp_tcp_message->msg_idx], orig_tcp_message,
                          package_id);
            fbe_spinlock_unlock(&send_idx_lock[tmp_tcp_message->conduit]);
            free (message_buffer);/*since we are done with the message we can now delete it*/
            return FBE_STATUS_OK;
        }

        /* Else we need to back-out any actions taken.  We must return
         * a failure status so that the caller frees any allocated message.
         */
        fbe_cmi_trace(FBE_TRACE_LEVEL_WARNING, "%s, Current conduit: %d msg_idx: %d msg: 0x%p xmit failed.package:%d\n", 
                      __FUNCTION__, orig_tcp_message->conduit, orig_tcp_message->msg_idx,
                      fbe_cmi_sim_client_sent_messages[orig_tcp_message->conduit][orig_tcp_message->msg_idx],
                      package_id);
        fbe_cmi_sim_client_sent_messages[orig_tcp_message->conduit][orig_tcp_message->msg_idx] = NULL;
        fbe_atomic_decrement(&fbe_cmi_sim_outstanding_message_counter[orig_tcp_message->conduit]);
        fbe_spinlock_unlock(&send_idx_lock[orig_tcp_message->conduit]);
        free (message_buffer);/*since we are done with the message we can now delete it*/

        /* If the peer went down report no device.*/
        if (!fbe_cmi_is_peer_alive()){
            return FBE_STATUS_NO_DEVICE;
        }else {
            return FBE_STATUS_IO_FAILED_NOT_RETRYABLE;
        }
    }

    /* Else there were no errors.  Free the local buffer.*/
    free (message_buffer);/*since we are done with the message we can now delete it*/
    return FBE_STATUS_OK;
     
}

static void cmi_sim_client_receive_completion_thread_function(void * context)
{
    fbe_cmi_conduit_id_t                        conduit_id = (fbe_cmi_conduit_id_t)context;
    fbe_u8_t                                    rec_buf[RECEIVE_BUFFER_SIZE]={""};
    fbe_s32_t                                   bytes = 0;
    fbe_u32_t                                   total_bytes = 0;
    fbe_u32_t                                   incomming_transfer_count = 0;
    fbe_u32_t                                   current_receive_count = 0;
    fbe_u32_t                                   bytes_left_to_transfer = 0;
    fbe_u32_t                                   initial_read_size = sizeof(fbe_cmi_sim_tcp_message_t);
    fbe_u8_t                                    target_buf[MAX_PACKET_BUFFER_SIZE];
    fbe_u8_t *                                  target_buf_ptr = target_buf;
    fbe_cmi_sim_tcp_message_t *                 cmi_tcp_message = NULL;
    fbe_u32_t                                   os_error = 0;
    fbe_package_id_t            package_id;

    fbe_get_package_id(&package_id);

    fbe_cmi_sim_client_thread_flag[conduit_id] = FBE_CMI_SIM_CLIENT_THREAD_RUN;

	fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, "%s, started, cond: %d!\n", __FUNCTION__, conduit_id);

    while(fbe_cmi_sim_client_thread_flag[conduit_id] == FBE_CMI_SIM_CLIENT_THREAD_RUN){        
        total_bytes = 0;
        current_receive_count = initial_read_size;/*initial read size assumes no buffer was sent, just an empty structure*/
        fbe_zero_memory (target_buf, MAX_PACKET_BUFFER_SIZE);
        target_buf_ptr = target_buf;
        
        do {
            bytes = recv(connect_socket[conduit_id], rec_buf, current_receive_count, 0);
            if (bytes > 0) {

                /*for the first transfer, we also take the size of the overall packaet we are aobut to receive.
                we assume the first argument in the fbe_cmi_sim_tcp_message_t will contain it*/
                if (total_bytes == 0) {
                    fbe_copy_memory(&incomming_transfer_count, rec_buf, sizeof(fbe_u32_t));
                    bytes_left_to_transfer = incomming_transfer_count;
                }

                /*sanity check*/
                if ((incomming_transfer_count > MAX_PACKET_BUFFER_SIZE) ||
                    ((total_bytes + bytes)    > MAX_PACKET_BUFFER_SIZE)    ) {
                    /*too much information, we can't process it*/
                    fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR, 
                                  "%s: Request size: %d greater then buffer size: %d Conduit:%d, package:%d\n", 
                                  __FUNCTION__,  incomming_transfer_count, MAX_PACKET_BUFFER_SIZE, conduit_id, package_id);
                    fbe_cmi_sim_client_thread_flag[conduit_id] = FBE_CMI_SIM_CLIENT_THREAD_DONE;
                    //closesocket(connect_socket[conduit_id]);
					fbe_thread_exit(EMCPAL_STATUS_SUCCESS);
					return;
				}

                bytes_left_to_transfer -= bytes;
                if (bytes_left_to_transfer > RECEIVE_BUFFER_SIZE) {
                    current_receive_count = RECEIVE_BUFFER_SIZE;
                }else{
                    current_receive_count = bytes_left_to_transfer;
                }

                /*copy to destination_buffer*/
                fbe_copy_memory(target_buf_ptr, rec_buf, bytes);
                target_buf_ptr += bytes;/*advance for next time*/
                total_bytes += bytes;

            }else if (bytes == 0 ){
                fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, "%s: client disconnected!Conduit:%d, package:%d\n", 
                              __FUNCTION__, conduit_id, package_id);
                fbe_cmi_mark_other_sp_dead(conduit_id);
                //closesocket(connect_socket[conduit_id]);
                fbe_cmi_sim_client_thread_flag[conduit_id] = FBE_CMI_SIM_CLIENT_THREAD_DONE;
                //fbe_cmi_sim_client_receive_completion_thread[conduit_id].thread = NULL;
                /*also if we have messages that were not yet received, send an abort message to the sender*/
                fbe_cmi_client_clear_pending_messages(conduit_id);
                fbe_thread_exit(EMCPAL_STATUS_SUCCESS);
                return;
            }else {
                os_error = EmcutilLastNetworkErrorGet();
                if (os_error == WSAEINTR || os_error ==WSAECONNRESET) {
                    fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, "%s: closing connection. Conduit:%d, package:%d\n", 
                                  __FUNCTION__,conduit_id, package_id);
                }else{
                    fbe_cmi_trace(FBE_TRACE_LEVEL_WARNING, 
                                  "%s: recv failed with code %d, packet lost. Conduit:%d, package:%d\n", 
                                  __FUNCTION__,os_error, conduit_id, package_id);
                }
                //fbe_cmi_sim_client_receive_completion_thread[conduit_id].thread = NULL;
                fbe_cmi_mark_other_sp_dead(conduit_id);
                fbe_cmi_sim_client_thread_flag[conduit_id] = FBE_CMI_SIM_CLIENT_THREAD_DONE;
                fbe_cmi_client_clear_pending_messages(conduit_id);  
                fbe_thread_exit(EMCPAL_STATUS_SUCCESS);
                return;
            }
        } while ((bytes > 0) && (total_bytes < incomming_transfer_count));
        
        /*now that we got the message, we need to process it*/
        cmi_tcp_message = (fbe_cmi_sim_tcp_message_t *)target_buf;
        cmi_sim_client_process_message(cmi_tcp_message);
        
    }

    fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, "%s: Destroying client conduit. Conduit:%d, package:%d\n", 
                                  __FUNCTION__,conduit_id, package_id);
    fbe_cmi_mark_other_sp_dead(conduit_id);
    fbe_cmi_sim_client_thread_flag[conduit_id] = FBE_CMI_SIM_CLIENT_THREAD_DONE;
    fbe_cmi_client_clear_pending_messages(conduit_id);
    //fbe_cmi_sim_client_receive_completion_thread[conduit_id].thread = NULL;
    fbe_thread_exit(EMCPAL_STATUS_SUCCESS);
}

static void cmi_sim_client_process_message(fbe_cmi_sim_tcp_message_t *cmi_tcp_message)
{
    fbe_cmi_sim_tcp_message_t *cmi_originating_message = NULL;

    switch(cmi_tcp_message->event_id){
    case FBE_CMI_EVENT_PEER_BUSY:
    case FBE_CMI_EVENT_MESSAGE_TRANSMITTED:
        if ((cmi_tcp_message->message_status == FBE_STATUS_OK) || (cmi_tcp_message->message_status == FBE_STATUS_BUSY)) {
            fbe_cmi_call_registered_client(cmi_tcp_message->event_id, cmi_tcp_message->client_id, 0, cmi_tcp_message->user_message, cmi_tcp_message->context);
        }else{
            fbe_cmi_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, "%s: TRANSMITTED, client %d, bad msg_status %d\n", 
                __FUNCTION__, cmi_tcp_message->client_id, cmi_tcp_message->message_status);
            fbe_cmi_call_registered_client(FBE_CMI_EVENT_FATAL_ERROR, cmi_tcp_message->client_id, 0, cmi_tcp_message->user_message, cmi_tcp_message->context);
        }

        /* The `transmitted' (acknowledgement) message is local buffer.  But
         * the original message must be freed.
         */
        fbe_spinlock_lock(&send_idx_lock[cmi_tcp_message->conduit]);
        cmi_originating_message = fbe_cmi_sim_client_sent_messages[cmi_tcp_message->conduit][cmi_tcp_message->msg_idx];

        if (cmi_originating_message == NULL)
        {
            fbe_spinlock_unlock(&send_idx_lock[cmi_tcp_message->conduit]);
            fbe_cmi_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, 
                          "%s client_id: %d event_id: %d No originating message.\n", 
                          __FUNCTION__, cmi_tcp_message->client_id, cmi_tcp_message->event_id);
            return;
        }
        else
        {
            /* Not NULL but has an invalid event_id.. */
            if ((cmi_originating_message->event_id != FBE_CMI_EVENT_MESSAGE_RECEIVED) ||
                (cmi_originating_message->conduit != cmi_tcp_message->conduit)        ||
                (cmi_originating_message->msg_idx != cmi_tcp_message->msg_idx)           )
            {
                fbe_spinlock_unlock(&send_idx_lock[cmi_tcp_message->conduit]);
                fbe_cmi_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, 
                              "%s client_id: %d event_id: %d Incorrect originating message index: %d\n", 
                              __FUNCTION__, cmi_tcp_message->client_id, cmi_tcp_message->event_id, cmi_originating_message->msg_idx);
                return;
            }
        }

        /*! @note Using the `transmitted' message information (since we cannot 
         *        access the originating message after freeing it), clear the
         *        originating pointer, then free the message.
         */
        free(cmi_originating_message);
        fbe_cmi_sim_client_sent_messages[cmi_tcp_message->conduit][cmi_tcp_message->msg_idx] = NULL;
        fbe_atomic_decrement(&fbe_cmi_sim_outstanding_message_counter[cmi_tcp_message->conduit]);
        fbe_spinlock_unlock(&send_idx_lock[cmi_tcp_message->conduit]);
        break;

    case FBE_CMI_EVENT_CLOSE_COMPLETED:
    case FBE_CMI_EVENT_SP_CONTACT_LOST:
        fbe_cmi_client_clear_pending_messages(cmi_tcp_message->conduit);
        break;
    default:
        fbe_cmi_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, "%s: unhandled CMI message %d\n", __FUNCTION__, cmi_tcp_message->event_id);

    }
}

static void cmi_sim_client_init_connection(client_connection_info_t connection_info)
{
    fbe_u32_t                   result = 0;
    fbe_u32_t                   total_retries = 0;
    fbe_u32_t                   total_msecs;
    fbe_package_id_t            package_id;

    fbe_get_package_id(&package_id);
     /* Connect to server.*/
    do {

        result = connect(connect_socket[connection_info.conduit_id],
                         connection_info.addr_info_ptr->ai_addr,
                         (int)connection_info.addr_info_ptr->ai_addrlen);

        if (result == SOCKET_ERROR){
            /*wait to connect to the other side, but not too much*/
            fbe_cmi_trace(FBE_TRACE_LEVEL_WARNING,"%s, Can't connect to conduit:%d.package:%d\n",
                           __FUNCTION__, connection_info.conduit_id, package_id);

            total_retries ++;
            fbe_thread_delay(100);
        }else{
            break;
        }

    }while (total_retries < FBE_CMI_CLIENT_MAX_CONNECT_ATTEMPTS);

    if (result == SOCKET_ERROR) {
		fbe_cmi_sim_client_thread_flag[connection_info.conduit_id] = FBE_CMI_SIM_CLIENT_THREAD_INVALID;
        fbe_cmi_trace(FBE_TRACE_LEVEL_WARNING,
                      "%s, Failed to connect to conduit:%d, other SP must be dead.package:%d\n",
                       __FUNCTION__, connection_info.conduit_id, package_id);

        freeaddrinfo(connection_info.addr_info_ptr);
        fbe_cmi_mark_other_sp_dead(connection_info.conduit_id);
        //fbe_cmi_sim_client_connection_thread[connection_info.conduit_id].thread  = NULL;
        return;
    }
    
    freeaddrinfo(connection_info.addr_info_ptr);
    
    fbe_cmi_trace(FBE_TRACE_LEVEL_INFO,
                  "%s, Peer is alive on conduit %d, package:%d\n", 
                  __FUNCTION__, connection_info.conduit_id, package_id);
    
    /*now we start the thread that will accept calls and server them*/
    fbe_thread_init(&fbe_cmi_sim_client_receive_completion_thread[connection_info.conduit_id],
                    "fbe_cmisim_rcv", cmi_sim_client_receive_completion_thread_function,
                    (void *)connection_info.conduit_id);

    /* We need to make sure we wait for the thread to start before returning. 
     * Otherwise we would be subject to a race where we return and then kick off the thread again 
     * since we don't see that it started yet.   The fbe_cmi_sim_client_thread_flag tells us if 
     * the thread has actually started. 
     */
    total_msecs = 0;
    while (fbe_cmi_sim_client_thread_flag[connection_info.conduit_id] != FBE_CMI_SIM_CLIENT_THREAD_RUN)
    {
        fbe_thread_delay(100);
        total_msecs += 100;
        fbe_cmi_trace(FBE_TRACE_LEVEL_WARNING,
                      "%s, Conduit %d receive thread not yet started in %d msecs. package:%d\n",
					   __FUNCTION__, connection_info.conduit_id, total_msecs, package_id);
        if (total_msecs >= FBE_CMI_MAX_RECEIVE_COMPLETION_START_MSECS)
        {
            fbe_cmi_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
                          "%s, Conduit %d Failed to start receive thread in %d msecs. package:%d\n",
					   __FUNCTION__, connection_info.conduit_id, total_msecs, package_id);
            break;
        }
    }

	fbe_cmi_trace(FBE_TRACE_LEVEL_INFO,
                  "%s, EXIT: conduit:%d flag=%d. package:%d\n", 
                  __FUNCTION__, connection_info.conduit_id, fbe_cmi_sim_client_thread_flag[connection_info.conduit_id], package_id);
}
fbe_bool_t fbe_cmi_sim_client_is_up(fbe_cmi_conduit_id_t conduit_id)
{
    if(fbe_cmi_sim_client_thread_flag[conduit_id] == FBE_CMI_SIM_CLIENT_THREAD_RUN ||
	   fbe_cmi_sim_client_thread_flag[conduit_id] == FBE_CMI_SIM_CLIENT_THREAD_INITIALIZING){
		fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, "%s, flag=%d\n", __FUNCTION__, fbe_cmi_sim_client_thread_flag[conduit_id]);
        return FBE_TRUE;
    }else{
        return FBE_FALSE;
    }
}

void verify_peer_connection_is_up(fbe_cmi_sp_id_t this_sp_id, fbe_cmi_conduit_id_t conduit_id)
{
    if (!fbe_cmi_sim_client_is_up(conduit_id)) {
            fbe_cmi_sim_init_conduit_client(conduit_id, this_sp_id);
    }
}

void fbe_cmi_client_clear_pending_messages(fbe_cmi_conduit_id_t conduit_id)
{
	fbe_u32_t                   idx = 0;
    fbe_cmi_sim_tcp_message_t * cmi_tcp_message;
    fbe_bool_t                  msg_printed = FBE_FALSE;

    fbe_spinlock_lock(&send_idx_lock[conduit_id]);

    for (idx = 0;idx < MAX_SENT_MSG_TRACK;idx ++) {
        if (fbe_cmi_sim_client_sent_messages[conduit_id][idx] != NULL) {
            if (FBE_IS_FALSE(msg_printed)) {
                fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, "Sending FBE_CMI_EVENT_PEER_NOT_PRESENT messages to peer during cmi shutdonw \n");
                msg_printed = FBE_TRUE;
            }

            cmi_tcp_message = fbe_cmi_sim_client_sent_messages[conduit_id][idx];
            /*let the sender know the peer died w/o answering it's message*/
            fbe_cmi_call_registered_client(FBE_CMI_EVENT_PEER_NOT_PRESENT, cmi_tcp_message->client_id, 0, cmi_tcp_message->user_message, cmi_tcp_message->context);
            /* The client doesn't need the wrapper so free the wrapper now. */
            free(cmi_tcp_message);
            fbe_cmi_sim_client_sent_messages[conduit_id][idx] = NULL;

        }
    }
    fbe_spinlock_unlock(&send_idx_lock[conduit_id]);
}

fbe_status_t fbe_cmi_sim_wait_for_client_to_init(fbe_cmi_conduit_id_t conduit_id)
{
    fbe_u32_t retry_ms;
    retry_ms = 0;
    while (fbe_cmi_sim_client_thread_flag[conduit_id] == FBE_CMI_SIM_CLIENT_THREAD_INITIALIZING) {
        fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, "%s waiting for client thread to init for conduit %u\n", __FUNCTION__, conduit_id);
        fbe_thread_delay(500);
        if (retry_ms >= 120000) {
            fbe_cmi_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, "%s CMI Did not initialize in %u msec for conduit %u\n",
                           __FUNCTION__, retry_ms, conduit_id);
            return FBE_STATUS_GENERIC_FAILURE; 
        }
        retry_ms += 500;
    }
    return FBE_STATUS_OK; 
}
