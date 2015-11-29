/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_rdgen_cmi.c
 ***************************************************************************
 *
 * @brief
 *  This file contains Rdgen code for peer<->peer communication.
 *
 * @version
 *   9/22/2010:  Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_transport.h"
#include "fbe_rdgen_cmi.h"
#include "fbe_rdgen_private_inlines.h"
#include "fbe_cmi.h"
#include "fbe_topology.h"

/*************************
 *   GLOBAL VARIABLES
 *************************/
static fbe_data_pattern_sp_id_t fbe_rdgen_cmi_sp_id = FBE_DATA_PATTERN_SP_ID_INVALID;

/*************************
 *   FUNCTION DEFINITIONS
 *************************/
static void fbe_rdgen_cmi_restart_waiters(void);

/*!****************************************************************************
 *          fbe_rdgen_cmi_init_sp_id()
 ******************************************************************************
 * @brief
 *  Simply initialize the sp id that rdgne resides.
 * 
 * @param None
 *
 * @return fbe_status_t.
 *
 ******************************************************************************/
static fbe_status_t fbe_rdgen_cmi_init_sp_id(void)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_cmi_sp_id_t my_sp_id = FBE_CMI_SP_ID_INVALID;

    /* Get our sp id
     */
    status = fbe_cmi_get_sp_id(&my_sp_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: Failed to get cmi sp id with status: 0x%x\n", 
                                __FUNCTION__, status);
        return status;
    }

    /* Validate that sp id is properly sert
     */
    switch(my_sp_id)
    {
        case FBE_CMI_SP_ID_A:
            /* Set our copy to SP A
             */
            fbe_rdgen_cmi_sp_id = FBE_DATA_PATTERN_SP_ID_A;
            break;

        case FBE_CMI_SP_ID_B:
            /* Set our copy to SP B
             */
            fbe_rdgen_cmi_sp_id = FBE_DATA_PATTERN_SP_ID_B;
            break;

        default:
            /* All other values are invalid
             */
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s: get sp id returned: %d which is invalid\n", 
                                    __FUNCTION__, my_sp_id);
            return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Otherwise return success
     */
    return FBE_STATUS_OK;
}



fbe_status_t
fbe_rdgen_cmi_send_message(fbe_rdgen_cmi_message_t * rdgen_cmi_message, fbe_packet_t * packet)
{
    fbe_cmi_send_message(FBE_CMI_CLIENT_ID_RDGEN, 
                         sizeof(fbe_rdgen_cmi_message_t),
                         (fbe_cmi_message_t)rdgen_cmi_message,
                         (fbe_cmi_event_callback_context_t)packet);

    return FBE_STATUS_OK;
}


/* Forward declarations */
static fbe_status_t fbe_rdgen_cmi_callback(fbe_cmi_event_t event, fbe_u32_t cmi_message_length, fbe_cmi_message_t cmi_message,  fbe_cmi_event_callback_context_t context);

static fbe_status_t fbe_rdgen_cmi_message_transmitted(fbe_rdgen_cmi_message_t *metadata_cmi_msg, 
                                                           fbe_cmi_event_callback_context_t context);

static fbe_status_t fbe_rdgen_cmi_peer_lost(fbe_rdgen_cmi_message_t *metadata_cmi_msg, 
                                                    fbe_cmi_event_callback_context_t context);

static fbe_status_t fbe_rdgen_cmi_message_received(fbe_rdgen_cmi_message_t * metadata_cmi_msg);

static fbe_status_t fbe_rdgen_cmi_fatal_error_message(fbe_rdgen_cmi_message_t *metadata_cmi_msg, 
                                                           fbe_cmi_event_callback_context_t context);

static fbe_status_t  fbe_rdgen_cmi_no_peer_message(fbe_rdgen_cmi_message_t *metadata_cmi_msg, 
                                                            fbe_cmi_event_callback_context_t context);


static fbe_status_t fbe_rdgen_cmi_process_peer_alive(fbe_rdgen_cmi_message_t *metadata_cmi_msg)
{
    fbe_status_t status = FBE_STATUS_OK;
    if (!fbe_rdgen_service_is_flag_set(FBE_RDGEN_SERVICE_FLAGS_PEER_ALIVE))
    {
        /* We cannot handle this here.  Tell the thread peer is initting.
         */
        fbe_rdgen_service_set_flag(FBE_RDGEN_SERVICE_FLAGS_PEER_INIT);
        fbe_rdgen_peer_thread_signal();
    }
    return status;
}

fbe_status_t fbe_rdgen_cmi_init(void)
{
    fbe_status_t status;

    status = fbe_rdgen_cmi_init_sp_id();
    if (status != FBE_STATUS_OK)
    {
        return status;
    }

    status = fbe_cmi_register(FBE_CMI_CLIENT_ID_RDGEN, fbe_rdgen_cmi_callback, NULL);
    if (status == FBE_STATUS_OK)
    {
        /* Send an init message to the peer.
         */
        status = fbe_rdgen_peer_send_peer_alive_msg();
    }

    return status;
}

fbe_status_t fbe_rdgen_cmi_destroy(void)
{
    fbe_status_t status;

    fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                            "%s: entry \n", __FUNCTION__);
    status = fbe_cmi_unregister(FBE_CMI_CLIENT_ID_RDGEN);
    return status;
}

/* 
   Note: cmi_message_length is only valid for received msg notifications, all other cases will have zero passed in
*/
static fbe_status_t 
fbe_rdgen_cmi_callback(fbe_cmi_event_t event, fbe_u32_t cmi_message_length, fbe_cmi_message_t cmi_message,  fbe_cmi_event_callback_context_t context)
{
    fbe_rdgen_cmi_message_t * metadata_cmi_msg = NULL;
    fbe_status_t status = FBE_STATUS_OK;

    metadata_cmi_msg = (fbe_rdgen_cmi_message_t*)cmi_message;

    switch (event) {
        case FBE_CMI_EVENT_MESSAGE_TRANSMITTED:
            status = fbe_rdgen_cmi_message_transmitted(metadata_cmi_msg, context);
            break;
        case FBE_CMI_EVENT_SP_CONTACT_LOST:
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                    "rdgen: Peer is Gone.\n");
            fbe_rdgen_service_clear_flag(FBE_RDGEN_SERVICE_FLAGS_PEER_ALIVE);
            status = fbe_rdgen_cmi_peer_lost(metadata_cmi_msg, context);
            break;
        case FBE_CMI_EVENT_MESSAGE_RECEIVED:
            status = fbe_rdgen_cmi_message_received(metadata_cmi_msg);
            break;
        case FBE_CMI_EVENT_FATAL_ERROR:
            status = fbe_rdgen_cmi_fatal_error_message(metadata_cmi_msg, context);
            break;
		case FBE_CMI_EVENT_PEER_NOT_PRESENT:
		case FBE_CMI_EVENT_PEER_BUSY:
            status = fbe_rdgen_cmi_no_peer_message(metadata_cmi_msg, context);
            break;
        default:
            break;
    }
    
    return status;
}


static fbe_status_t 
fbe_rdgen_cmi_message_transmitted(fbe_rdgen_cmi_message_t *metadata_cmi_msg, fbe_cmi_event_callback_context_t context)
{
    fbe_packet_t * packet_p = NULL;

    if (metadata_cmi_msg->metadata_cmi_message_type == FBE_RDGEN_CMI_MESSAGE_TYPE_REQUEST_RESP_NO_RESOURCES)
    {
        /* Free the memory we used to send this message.
         */
        fbe_memory_native_release(metadata_cmi_msg);
        return FBE_STATUS_OK;
    }
    if (metadata_cmi_msg->metadata_cmi_message_type == FBE_RDGEN_CMI_MESSAGE_TYPE_START_REQUEST)
    {
        /* We are waiting for the peer to send us a response.
         */
        return FBE_STATUS_OK;
    }
    if (metadata_cmi_msg->metadata_cmi_message_type == FBE_RDGEN_CMI_MESSAGE_TYPE_PEER_ALIVE)
    {
        /* If the peer actually gets our peer alive message, then we can mark it alive.
         */
        fbe_rdgen_service_set_flag(FBE_RDGEN_SERVICE_FLAGS_PEER_ALIVE);
        return FBE_STATUS_OK;
    }

	if (metadata_cmi_msg->metadata_cmi_message_type == FBE_RDGEN_CMI_MESSAGE_TYPE_CMI_PERFORMANCE_TEST)
    {
        /* If this is an ack for a message that was just transmitted, the callback is a semaphore we need to bang and that's it.
         */
        fbe_semaphore_release((fbe_semaphore_t *)context, 0, 1, FBE_FALSE);
        return FBE_STATUS_OK;
    }

    packet_p = (fbe_packet_t *)context;

    /* If the active side is sending a message to the peer, we do not supply a context since we're waiting 
     * for the message to come back and do not need a response here. 
     *  
     * Otherwise the passive side is sending a message to the originator and needs to free resources. 
     * We complete the packet now so the passive side can free resources. 
     */
    if (packet_p != NULL)
    {
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
    }

    return FBE_STATUS_OK; 
}

static fbe_status_t 
fbe_rdgen_cmi_peer_lost(fbe_rdgen_cmi_message_t *metadata_cmi_msg, fbe_cmi_event_callback_context_t context)
{
    fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry\n", __FUNCTION__);

    /*this is an asynchronous event about the sp dying and not a reply about CMI unable to send the message
    because the SP is dead*/
    fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO, "%s, Peer died, restarting all packets\n", __FUNCTION__);

    /* Some or all of our messages may never get a reply.
     * Complete these packets with an error. 
     */
    fbe_rdgen_peer_died_restart_packets();
    return FBE_STATUS_OK; 
}
static void fbe_rdgen_cmi_process_peer_start_request(fbe_rdgen_cmi_message_t *msg)
{
    fbe_rdgen_peer_request_t *peer_request_p = NULL;

    /* Allocate memory for the request from the peer.
     * The memory passed in is not ours so we need to save the contents. 
     */
    fbe_rdgen_lock();

    /* If the service is being destroyed then drop any new requests from the peer.
     */
    if (fbe_rdgen_service_is_flag_set(FBE_RDGEN_SERVICE_FLAGS_DESTROY))
    {
        fbe_rdgen_unlock();
        return;
    }
    fbe_rdgen_allocate_peer_request(&peer_request_p);

    if (peer_request_p == NULL)
    {
        fbe_rdgen_cmi_message_t *failure_msg_p = NULL;

        /* We are out of resources.  Send failure.
         */
        fbe_rdgen_unlock();
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO, 
                                "%s, no peer request resources.  Send failure to peer.\n", __FUNCTION__);

        failure_msg_p = fbe_memory_native_allocate(sizeof(fbe_rdgen_cmi_message_t));
        if (failure_msg_p == NULL)
        {
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, 
                                    "%s, native allocate failed\n", __FUNCTION__);
            return;
        }
        failure_msg_p->metadata_cmi_message_type = FBE_RDGEN_CMI_MESSAGE_TYPE_REQUEST_RESP_NO_RESOURCES;
        failure_msg_p->peer_ts_ptr = msg->peer_ts_ptr;
        fbe_cmi_send_message(FBE_CMI_CLIENT_ID_RDGEN, 
                             sizeof(fbe_rdgen_cmi_message_t),
                             (fbe_cmi_message_t)failure_msg_p,
                             (fbe_cmi_event_callback_context_t)NULL);
        return;
    }

    /* Copy data from incoming cmi msg to our cmi msg. 
     */
    peer_request_p->cmi_msg.msg.start_request.start_io = msg->msg.start_request.start_io;
    peer_request_p->cmi_msg.peer_ts_ptr = msg->peer_ts_ptr;
    
    fbe_rdgen_peer_request_set_magic(peer_request_p);
    /* The peer should know before sending the request that we have enough 
     * resources to process this. 
     */
    fbe_rdgen_peer_thread_enqueue(&peer_request_p->queue_element);
    fbe_rdgen_unlock();
    return;
}

static fbe_status_t 
fbe_rdgen_cmi_process_peer_response(fbe_rdgen_cmi_message_t *metadata_cmi_msg)
{
    fbe_packet_t   *packet_p = NULL;
    fbe_rdgen_ts_t *ts_p = NULL;

    /* Just complete the packet that is waiting for this response.
     * The ts ptr is in the message. 
     */
    ts_p = (fbe_rdgen_ts_t *)metadata_cmi_msg->peer_ts_ptr;
    packet_p = (fbe_packet_t*)&ts_p->read_transport.packet;

    if (!fbe_rdgen_ts_validate_magic(ts_p))
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, 
                                "%s ts %p magic number is invalid 0x%llx",
				__FUNCTION__, ts_p,
				(unsigned long long)ts_p->magic);
        return FBE_STATUS_OK;
    }

    /* Copy over the response msg.
     * Start by incrementing our counters. 
     * We intentionally do not increment ts_p->pass_count since this is already getting incremented 
     * by this side (the active side). 
     */
    ts_p->abort_err_count += metadata_cmi_msg->msg.start_request.start_io.statistics.aborted_error_count;
    ts_p->bad_crc_blocks_count += metadata_cmi_msg->msg.start_request.start_io.statistics.bad_crc_blocks_count;
    ts_p->err_count += metadata_cmi_msg->msg.start_request.start_io.statistics.error_count;
    ts_p->inv_blocks_count += metadata_cmi_msg->msg.start_request.start_io.statistics.inv_blocks_count;
    ts_p->io_count += metadata_cmi_msg->msg.start_request.start_io.statistics.io_count;
    ts_p->invalid_request_err_count += metadata_cmi_msg->msg.start_request.start_io.statistics.invalid_request_err_count;
    ts_p->io_failure_err_count += metadata_cmi_msg->msg.start_request.start_io.statistics.io_failure_error_count;
    ts_p->still_congested_err_count += metadata_cmi_msg->msg.start_request.start_io.statistics.still_congested_err_count;
    ts_p->congested_err_count += metadata_cmi_msg->msg.start_request.start_io.statistics.congested_err_count;
    ts_p->media_err_count += metadata_cmi_msg->msg.start_request.start_io.statistics.media_error_count;

    /* This is the only place we inc the peer counts.
     */
    fbe_rdgen_ts_inc_peer_pass_count(ts_p, metadata_cmi_msg->msg.start_request.start_io.statistics.pass_count);
    fbe_rdgen_ts_inc_peer_io_count(ts_p, metadata_cmi_msg->msg.start_request.start_io.statistics.io_count);

    /*! @todo we should increment the errors.
     */
    ts_p->verify_errors = metadata_cmi_msg->msg.start_request.start_io.statistics.verify_errors;

    /* Copy over the last status.
     */
    ts_p->last_packet_status = metadata_cmi_msg->msg.start_request.start_io.status;
    ts_p->status = metadata_cmi_msg->status;

    /* We must dequeue the packet before it gets completed. 
     */ 
    fbe_rdgen_lock();
    fbe_rdgen_peer_packet_dequeue(packet_p);
    fbe_rdgen_dec_num_outstanding_peer_req();
    if (metadata_cmi_msg->msg.start_request.start_io.status.rdgen_status != FBE_RDGEN_OPERATION_STATUS_PEER_OUT_OF_RESOURCES)
    {
        if (fbe_rdgen_dec_peer_out_of_resource_count() != FBE_STATUS_OK)
        {
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, ts_p->object_p->object_id,
                                    "unexpected peer out of resource count: ts_p: 0x%x request count: %d peer requests: %d\n", 
                                    (fbe_u32_t)ts_p,
                                    fbe_rdgen_get_num_requests(),
                                    fbe_rdgen_get_num_outstanding_peer_req());
        }
    }
    fbe_rdgen_unlock();
    
    fbe_rdgen_service_trace(FBE_TRACE_LEVEL_DEBUG_LOW, ts_p->object_p->object_id,
                            "complete from peer: ts_p: 0x%x.  packet: 0x%x lba: 0x%llx bl: 0x%llx\n", 
                            (fbe_u32_t)ts_p, (fbe_u32_t)&ts_p->read_transport.packet, (unsigned long long)ts_p->lba, (unsigned long long)ts_p->blocks);

    if (metadata_cmi_msg->msg.start_request.start_io.statistics.error_count)
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO, ts_p->object_p->object_id,
                                "err from peer: ts_p: 0x%x err: 0x%x st:%d bst:%d bq:%d ios: 0x%x\n", 
                                (fbe_u32_t)ts_p,
                                metadata_cmi_msg->msg.start_request.start_io.statistics.error_count,
                                metadata_cmi_msg->msg.start_request.start_io.status.status, 
                                metadata_cmi_msg->msg.start_request.start_io.status.block_status,
                                metadata_cmi_msg->msg.start_request.start_io.status.block_qualifier,
                                metadata_cmi_msg->msg.start_request.start_io.statistics.io_count);
    }
    /* Complete the packet that is waiting for this response.
     */
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);

    /* Now that this request is done, kick start any waiters
     */
    fbe_rdgen_cmi_restart_waiters();

    return FBE_STATUS_OK; 
}

static void fbe_rdgen_cmi_restart_waiters(void)
{
    fbe_queue_head_t *queue_p = NULL;
    fbe_queue_element_t *queue_element_p = NULL;
    fbe_rdgen_ts_t * current_ts_p = NULL;

    /* Restart any waiters.
     */
    fbe_rdgen_lock();
    fbe_rdgen_get_wait_peer_ts_queue(&queue_p);

    while (!fbe_queue_is_empty(queue_p)                                              &&
            (fbe_rdgen_get_num_outstanding_peer_req() < FBE_RDGEN_MAX_PEER_REQUESTS) &&
            (fbe_rdgen_get_peer_out_of_resource_count() < 1)                            )
    {
        queue_element_p = fbe_queue_pop(queue_p);
        current_ts_p = fbe_rdgen_ts_thread_queue_element_to_ts_ptr(queue_element_p);
        fbe_rdgen_peer_packet_enqueue(&current_ts_p->read_transport.packet);
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO, current_ts_p->object_p->object_id,
                                "restart from peer queue: ts_p: 0x%x.  packet: 0x%x lba: 0x%llx bl: 0x%llx\n", 
                                (fbe_u32_t)current_ts_p, (fbe_u32_t)&current_ts_p->read_transport.packet, 
                                (unsigned long long)current_ts_p->lba, (unsigned long long)current_ts_p->blocks);
        fbe_rdgen_inc_requests_to_peer();
        fbe_rdgen_inc_num_outstanding_peer_req();
        fbe_rdgen_unlock();
        fbe_rdgen_cmi_send_message(&current_ts_p->cmi_msg, NULL);
        fbe_rdgen_lock();
    }
    fbe_rdgen_unlock();
}

/*!**************************************************************
 * fbe_rdgen_cmi_mark_peer_request_successful()
 ****************************************************************
 * @brief
 *  Mark ts as if it completed successfully.
 *
 * @param   metadata_cmi_msg - Pointer to cmi message to mark as successful
 *
 * @return fbe_bool_t
 *
 * @author
 *  10/8/2010 - Created. Rob Foley
 *
 ****************************************************************/
static void fbe_rdgen_cmi_mark_peer_request_successful(fbe_rdgen_cmi_message_t *metadata_cmi_msg, fbe_rdgen_operation_status_t rdgen_status)
{
    metadata_cmi_msg->status = FBE_STATUS_OK;
    metadata_cmi_msg->msg.start_request.start_io.status.block_status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS;
    metadata_cmi_msg->msg.start_request.start_io.status.block_qualifier = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE;
    metadata_cmi_msg->msg.start_request.start_io.status.status = FBE_STATUS_OK;
    metadata_cmi_msg->msg.start_request.start_io.status.rdgen_status = rdgen_status;

    return;
}
/**************************************************
 * end fbe_rdgen_cmi_mark_peer_request_successful()
 **************************************************/

static fbe_status_t fbe_rdgen_cmi_process_peer_out_of_resources(fbe_rdgen_cmi_message_t *metadata_cmi_msg)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_packet_t   *packet_p = NULL;
    fbe_rdgen_ts_t *ts_p = NULL;

    /* Just complete the packet that is waiting for this response.
     * The ts ptr is in the message. 
     */
    ts_p = (fbe_rdgen_ts_t *)metadata_cmi_msg->peer_ts_ptr;
    packet_p = (fbe_packet_t*)&ts_p->read_transport.packet;

    /* We just consider this a response and kick off the ts again.
     * None of our pass counts will get incremented since the peer did not do anything. 
     * We want this side to go through the effort to re-submit this request. 
     */
    metadata_cmi_msg->status = FBE_STATUS_OK;
    fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, ts_p->object_p->object_id,
                            "peer out of resources: ts_p: 0x%x packet: 0x%x lba: 0x%llx bl: 0x%llx\n", 
                            (fbe_u32_t)ts_p, (fbe_u32_t)packet_p,
                            (unsigned long long)ts_p->lba, (unsigned long long)ts_p->blocks);

    /* Increment the `peer out of resources' count
     */ 
    fbe_rdgen_lock();
    fbe_rdgen_inc_peer_out_of_resource_count();
    fbe_rdgen_inc_peer_out_of_resource();
    fbe_rdgen_unlock();
    fbe_rdgen_cmi_mark_peer_request_successful(metadata_cmi_msg, FBE_RDGEN_OPERATION_STATUS_PEER_OUT_OF_RESOURCES);
    fbe_rdgen_cmi_process_peer_response(metadata_cmi_msg);
    return status;
}
static fbe_status_t 
fbe_rdgen_cmi_message_received(fbe_rdgen_cmi_message_t *metadata_cmi_msg)
{
    fbe_rdgen_service_trace(FBE_TRACE_LEVEL_DEBUG_LOW,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry\n", __FUNCTION__);

    /* If the service is being destroyed then drop any new requests from the peer.
     */
    if (fbe_rdgen_service_is_flag_set(FBE_RDGEN_SERVICE_FLAGS_DESTROY))
    {
        return FBE_STATUS_OK;
    }

    switch(metadata_cmi_msg->metadata_cmi_message_type)
    {
        case FBE_RDGEN_CMI_MESSAGE_TYPE_START_REQUEST:
            fbe_rdgen_cmi_process_peer_start_request(metadata_cmi_msg);
            break;
        case FBE_RDGEN_CMI_MESSAGE_TYPE_REQUEST_RESPONSE:
            metadata_cmi_msg->msg.start_request.start_io.status.rdgen_status = FBE_RDGEN_OPERATION_STATUS_SUCCESS;
            fbe_rdgen_cmi_process_peer_response(metadata_cmi_msg);
            break;
        case FBE_RDGEN_CMI_MESSAGE_TYPE_PEER_ALIVE:
            fbe_rdgen_cmi_process_peer_alive(metadata_cmi_msg);
            break;
        case FBE_RDGEN_CMI_MESSAGE_TYPE_REQUEST_RESP_NO_RESOURCES:
            /* This is an indication we are overrunning the peer's ability to 
             * allocate memory to handle this request. 
             */
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s rdgen: peer is out of resources %d \n", 
                                    __FUNCTION__, metadata_cmi_msg->metadata_cmi_message_type);
            fbe_rdgen_cmi_process_peer_out_of_resources(metadata_cmi_msg);
            break;
		case FBE_RDGEN_CMI_MESSAGE_TYPE_CMI_PERFORMANCE_TEST:
			/*we don'e really care about this message*/
			break;
        default:
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s Uknown message type %d \n", __FUNCTION__ , metadata_cmi_msg->metadata_cmi_message_type);
            break;

    };

    return FBE_STATUS_OK;
}

static fbe_status_t 
fbe_rdgen_cmi_fatal_error_message(fbe_rdgen_cmi_message_t *metadata_cmi_msg, fbe_cmi_event_callback_context_t context)
{
    /*process a message that say not only we could not send the message but the whole CMI bus is bad*/
    fbe_packet_t * packet = NULL;

    fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                            "%s: packet: %p\n", __FUNCTION__, context);

    packet = (fbe_packet_t *)context;
    if (packet != NULL)
    {
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
    }

    return FBE_STATUS_OK;
}


static fbe_status_t 
fbe_rdgen_cmi_no_peer_message(fbe_rdgen_cmi_message_t *metadata_cmi_msg, fbe_cmi_event_callback_context_t context)
{
    fbe_packet_t * packet = NULL;

    fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                            "%s: packet: %p\n", __FUNCTION__, context);

	if (metadata_cmi_msg->metadata_cmi_message_type == FBE_RDGEN_CMI_MESSAGE_TYPE_CMI_PERFORMANCE_TEST) {
		fbe_semaphore_release((fbe_semaphore_t *)context, 0, 1, FBE_FALSE);
		return FBE_STATUS_OK; 
	}

    packet = (fbe_packet_t *)context;

    if (packet != NULL)
    {
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
    }

    return FBE_STATUS_OK; 
}

/*!***************************************************************************
 *          fbe_rdgen_cmi_get_sp_id()
 ***************************************************************************** 
 * 
 * @brief   Since our local sp cannot change from the time that rdgen is
 *          initialized, simply retrive the local copy of the configured sp
 *          value.
 *
 * @param None.
 *
 * @return fbe_rdgen_sp_id_t
 *
 *****************************************************************************/
fbe_data_pattern_sp_id_t fbe_rdgen_cmi_get_sp_id(void)
{
    /* Simply return out local copy since the sp id cannot change
     */
    return(fbe_rdgen_cmi_sp_id);
}


/*************************
 * end file fbe_rdgen_cmi.c
 *************************/
