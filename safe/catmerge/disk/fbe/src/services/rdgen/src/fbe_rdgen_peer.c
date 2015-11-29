/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_rdgen_peer.c
 ***************************************************************************
 *
 * @brief
 *  This file contains .
 *
 * @version
 *   9/23/2010:  Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_rdgen.h"
#include "fbe_rdgen_private_inlines.h"
#include "fbe_rdgen_cmi.h"
#include "fbe/fbe_memory.h"

/*!*******************************************************************
 * @var init_cmi_msg_p
 *********************************************************************
 * @brief global cmi init message we send to the peer.
 *********************************************************************/

static fbe_rdgen_cmi_message_t *init_cmi_msg_p = NULL;

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

fbe_status_t fbe_rdgen_peer_init()
{
    init_cmi_msg_p = fbe_memory_native_allocate(sizeof(fbe_rdgen_cmi_message_t));
    return FBE_STATUS_OK;
}

fbe_status_t fbe_rdgen_peer_destroy()
{
    fbe_memory_native_release(init_cmi_msg_p);
    return FBE_STATUS_OK;
}

/*!****************************************************************************
 * @fn fbe_rdgen_peer_request_response_cmi_completion()
 ******************************************************************************
 * @brief
 *  This is the completion function for the CMI transfer getting finished to
 *  send the response back to the peer.
 * 
 * @param packet_p - current packet
 * @param context        - Pointer to the provision drive object.
 *
 * @return fbe_status_t.
 *
 ******************************************************************************/
static fbe_status_t
fbe_rdgen_peer_request_response_cmi_completion(fbe_packet_t * packet_p,
                                               fbe_memory_completion_context_t context)
{
    fbe_rdgen_peer_request_t *peer_request_p = NULL;

    peer_request_p = (fbe_rdgen_peer_request_t *) context;

    if (!fbe_rdgen_peer_request_validate_magic(peer_request_p))
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s rdgen: peer request magic for %p is 0x%llx.\n",
                                __FUNCTION__, peer_request_p, (unsigned long long)peer_request_p->magic);
    }
    fbe_transport_destroy_packet(packet_p);
    
    /* The peer has the response.  Now free our resources.
     */
    fbe_rdgen_lock();
    fbe_rdgen_free_peer_request(peer_request_p);
    fbe_rdgen_unlock();
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/**************************************
 * end fbe_rdgen_peer_request_response_cmi_completion()
 **************************************/
/*!****************************************************************************
 * @fn fbe_rdgen_peer_request_packet_completion()
 ******************************************************************************
 * @brief
 *  This is the completion function for a packet sent to rdgen service from the
 *  peer thread.
 * 
 * @param packet_p - current packet
 * @param context - not used.
 *
 * @return fbe_status_t.
 *
 ******************************************************************************/
static fbe_status_t
fbe_rdgen_peer_request_packet_completion(fbe_packet_t * packet_p, 
                                         fbe_memory_completion_context_t context)
{
    fbe_payload_ex_t *                 sep_payload_p = NULL;
    fbe_payload_control_operation_t *   control_operation_p = NULL;
    fbe_rdgen_control_start_io_t *      start_io_p = NULL;
    fbe_rdgen_cmi_message_t *cmi_msg_p = NULL;
    fbe_rdgen_peer_request_t *peer_request_p = NULL;
    fbe_rdgen_ts_t *peer_ts_p = NULL;

    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);
    fbe_payload_control_get_buffer(control_operation_p, &start_io_p);

    /* The packet is embedded in a larger structure.  Get the ptr to this structure.
     * And get the ptr to the CMI msg inside this structure. 
     */
    peer_request_p = (fbe_rdgen_peer_request_t *)context;
    if (!fbe_rdgen_peer_request_validate_magic(peer_request_p))
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s rdgen: peer request magic for %p is 0x%llx.\n",
                                __FUNCTION__, peer_request_p, (unsigned long long)peer_request_p->magic);
    }
    cmi_msg_p = &peer_request_p->cmi_msg;
    cmi_msg_p->metadata_cmi_message_type = FBE_RDGEN_CMI_MESSAGE_TYPE_REQUEST_RESPONSE;

    /* Set our completion function.  It gets called when we finish sending the data to the peer.
     */
    fbe_transport_set_completion_function(packet_p,
                                          fbe_rdgen_peer_request_response_cmi_completion,
                                          peer_request_p);

    /* If rdgen is shutting down, we do not need to complete this since the peer will soon find out we are gone. If we
     * complete this they will just send us more I/O, which we cannot process since we are going down. 
     * Simply complete this packet to free memory.
     */
    if (fbe_rdgen_service_is_flag_set(FBE_RDGEN_SERVICE_FLAGS_DESTROY))
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                "rdgen: send peer response not sent due to destroy ts_p: %p. object_id: 0x%x lba: 0x%llx blocks: 0x%llx\n",
                                peer_ts_p,  
                                peer_request_p->cmi_msg.msg.start_request.start_io.filter.object_id,
                                peer_request_p->cmi_msg.msg.start_request.start_io.specification.start_lba,
                                peer_request_p->cmi_msg.msg.start_request.start_io.specification.max_blocks);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }
    /* Save the packet status for sending back to the peer.
     */
    cmi_msg_p->status = fbe_transport_get_status_code(packet_p);

    peer_ts_p = peer_request_p->cmi_msg.peer_ts_ptr;
    fbe_rdgen_service_trace(FBE_TRACE_LEVEL_DEBUG_MEDIUM, FBE_TRACE_MESSAGE_ID_INFO,
                            "rdgen: send peer response ts_p: %p. object_id: 0x%x lba: 0x%llx blocks: 0x%llx\n",
                            peer_ts_p,  
                            peer_request_p->cmi_msg.msg.start_request.start_io.filter.object_id,
                            peer_request_p->cmi_msg.msg.start_request.start_io.specification.start_lba,
                            peer_request_p->cmi_msg.msg.start_request.start_io.specification.max_blocks);
    fbe_rdgen_cmi_send_message(cmi_msg_p, packet_p);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/**************************************
 * end fbe_rdgen_peer_request_packet_completion()
 **************************************/
/*!**************************************************************
 * fbe_rdgen_peer_send_rdgen_request()
 ****************************************************************
 * @brief
 *  Send the request from the peer to the rdgen service.
 *
 *  Assumption is that this request was already populated with the
 *  request information.
 *
 * @param request_p - This has the memory we need to start the request.
 *                    We just need to form the packet and send it to rdgen.
 * 
 * @return fbe_status_t
 *
 * @author
 *  7/13/2012 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_rdgen_peer_send_rdgen_request(fbe_rdgen_peer_request_t *request_p)
{
    fbe_packet_t *packet_p = NULL;
    fbe_payload_ex_t *sep_payload_p = NULL;
    fbe_payload_control_operation_t *control_operation_p = NULL;
    packet_p = &request_p->packet;

    /* Now init the packet and send it to rdgen.
     */
    fbe_transport_initialize_sep_packet(packet_p);
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_allocate_control_operation(sep_payload_p);

    fbe_payload_control_build_operation(control_operation_p,
                                        FBE_RDGEN_CONTROL_CODE_START_IO,
                                        &request_p->cmi_msg.msg.start_request.start_io,
                                        sizeof(fbe_rdgen_control_start_io_t));
    fbe_transport_set_completion_function(packet_p,
                                          fbe_rdgen_peer_request_packet_completion,
                                          request_p);
    fbe_transport_set_address(packet_p, 
                              FBE_PACKAGE_ID_NEIT, 
                              FBE_SERVICE_ID_RDGEN, 
                              FBE_CLASS_ID_INVALID, FBE_OBJECT_ID_INVALID);
    fbe_rdgen_service_trace(FBE_TRACE_LEVEL_DEBUG_MEDIUM, FBE_TRACE_MESSAGE_ID_INFO,
                            "rdgen: start ts from peer: ts_p: %p. object_id: 0x%x lba: 0x%llx blocks: 0x%llx\n",
                            request_p->cmi_msg.peer_ts_ptr,  
                            request_p->cmi_msg.msg.start_request.start_io.filter.object_id,
                            (unsigned long long)request_p->cmi_msg.msg.start_request.start_io.specification.start_lba,
                            (unsigned long long)request_p->cmi_msg.msg.start_request.start_io.specification.max_blocks);

    /* If we are destroying make sure we complete the packet without trying to send it to rdgen.
     */
    fbe_rdgen_lock();
    if (fbe_rdgen_service_is_flag_set(FBE_RDGEN_SERVICE_FLAGS_DESTROY))
    {
        fbe_rdgen_unlock();
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                "rdgen: drop peer request due to service destroy in progress.\n");
        fbe_transport_set_status(packet_p, FBE_STATUS_NO_DEVICE, 0);
        fbe_payload_ex_increment_control_operation_level(sep_payload_p);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_OK;
    }
    fbe_rdgen_unlock();

    fbe_service_manager_send_control_packet(packet_p);
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_rdgen_peer_send_rdgen_request()
 ******************************************/
/*!**************************************************************
 * fbe_rdgen_peer_thread_process_queue()
 ****************************************************************
 * @brief
 *  Handle entries off our queue arriving from the peer.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  9/23/2010 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_rdgen_peer_thread_process_queue(fbe_rdgen_thread_t *thread_p)
{
    fbe_queue_element_t *queue_element_p = NULL;
    fbe_rdgen_peer_request_t *request_p = NULL;
	fbe_queue_head_t tmp_queue;
    fbe_queue_init(&tmp_queue);

    /* Prevent things from changing while we are pulling
     * all the items off the queue.
     */
    fbe_rdgen_thread_lock(thread_p);

    /* Clear event under lock.  This allows us to coalesce the wait on events.
     */
    fbe_rendezvous_event_clear(&thread_p->event);

    if (fbe_queue_is_empty(&thread_p->ts_queue_head))
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                "rdgen: peer queue empty.\n");
        fbe_rdgen_thread_unlock(thread_p);
        return;
    }

    /* Populate temp queue from ts queue.
     */
    while(queue_element_p = fbe_queue_pop(&thread_p->ts_queue_head))
    {
        fbe_queue_push(&tmp_queue, queue_element_p);
    }
    /* Unlock to allow more items to arrive.
     */
    fbe_rdgen_thread_unlock(thread_p);

    /* Process the temp queue, kick each item off.
     */
    while(queue_element_p = fbe_queue_pop(&tmp_queue))
    {
        request_p = fbe_rdgen_peer_request_queue_element_to_ptr(queue_element_p);

        if (!fbe_rdgen_peer_request_validate_magic(request_p))
        {
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                    "rdgen: peer request magic for %p is 0x%llx.\n",
                                    request_p, (unsigned long long)request_p->magic);
        }

        /* Request was already populated, just kick it off.
         */
        fbe_rdgen_peer_send_rdgen_request(request_p);
    }
    fbe_queue_destroy(&tmp_queue);
    return;
}
/******************************************
 * end fbe_rdgen_peer_thread_process_queue()
 ******************************************/

/*!****************************************************************************
 * @fn fbe_rdgen_ts_peer_request_packet_completion()
 ******************************************************************************
 * @brief
 *  This is the completion function for a packet sent to rdgen service from the
 *  peer thread.
 * 
 * @param packet_p - current packet
 * @param context - not used.
 *
 * @return fbe_status_t.
 *
 ******************************************************************************/
static fbe_status_t
fbe_rdgen_ts_peer_request_packet_completion(fbe_packet_t * packet_p, 
                                            fbe_packet_completion_context_t context)
{
    fbe_rdgen_ts_t *ts_p = (fbe_rdgen_ts_t*) context;
    fbe_status_t status;

    if (RDGEN_COND(ts_p->object_p == NULL))
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: fbe_rdgen_ts object is null. \n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (RDGEN_COND(ts_p->b_outstanding == FBE_FALSE))
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: fbe_rdgen_ts outstanding flag 0x%x is not set to FBE_TRUE . \n", 
                                __FUNCTION__, ts_p->b_outstanding);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    ts_p->b_outstanding = FBE_FALSE;

    if (RDGEN_COND(ts_p->b_send_to_peer == FBE_FALSE))
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: fbe_rdgen_ts b_send_to_peer flag 0x%x is not set to FBE_TRUE . \n", 
                                __FUNCTION__, ts_p->b_outstanding);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    ts_p->b_send_to_peer = FBE_FALSE;

    if (fbe_queue_is_element_on_queue(&packet_p->queue_element))
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: packet %p is on a queue. \n", 
                                __FUNCTION__, packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* Increment our object stats.
     */
    fbe_rdgen_object_lock(ts_p->object_p);
    fbe_rdgen_object_dec_num_peer_requests(ts_p->object_p);
    fbe_rdgen_object_unlock(ts_p->object_p);

    fbe_rdgen_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO,
                            "%s: ts_p: %p. object_id: 0x%x lba: 0x%llx bl: 0x%llx\n", 
                            __FUNCTION__, ts_p, ts_p->object_p->object_id,
			    (unsigned long long)ts_p->current_lba,
			    (unsigned long long)ts_p->current_blocks);
    
    /* Simply put this on a thread.
     */
    status = fbe_rdgen_ts_enqueue_to_thread(ts_p);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/**************************************
 * end fbe_rdgen_ts_peer_request_packet_completion()
 **************************************/
/*!**************************************************************
 * fbe_rdgen_ts_send_peer_request()
 ****************************************************************
 * @brief
 *  Put the relevant info from the ts into the peer message and
 *  send it to the peer.
 *
 * @param ts_p - Current TS that needs to be sent to the peer.
 *
 * @return fbe_status_t 
 *
 * @author
 *  9/24/2010 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_rdgen_ts_send_peer_request(fbe_rdgen_ts_t *ts_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_rdgen_control_start_io_t *start_req_p = &ts_p->cmi_msg.msg.start_request.start_io;
    fbe_packet_t *packet_p = &ts_p->read_transport.packet;
    fbe_lba_t lba = ts_p->current_lba;
    fbe_block_count_t blocks = ts_p->current_blocks;
    fbe_u32_t current_sequence_count =  fbe_rdgen_ts_get_sequence_count(ts_p);

    /* Mark this as outstanding since we're sending it to the peer.
     */
    ts_p->b_outstanding = FBE_TRUE;

    /* Increment our object stats.
     */
    fbe_rdgen_object_lock(ts_p->object_p);
    fbe_rdgen_object_inc_num_peer_requests(ts_p->object_p);
    fbe_rdgen_object_unlock(ts_p->object_p);

    fbe_rdgen_ts_update_last_send_time(ts_p);

    /* Setup the packet with a completion.
     */
    fbe_transport_reuse_packet(packet_p);
    fbe_transport_set_completion_function(packet_p, fbe_rdgen_ts_peer_request_packet_completion, ts_p);

    /* Drop the packet onto the peer request queue.
     */
    ts_p->cmi_msg.metadata_cmi_message_type = FBE_RDGEN_CMI_MESSAGE_TYPE_START_REQUEST;

    /* Copy the entire spec from the request.  Modified the starting seed to the
     * current value + 1 (since the `first' request won't increment).
     */
    start_req_p->specification = ts_p->request_p->specification;
    start_req_p->specification.options |= FBE_RDGEN_OPTIONS_USE_SEQUENCE_COUNT_SEED;
    current_sequence_count++;
    start_req_p->specification.sequence_count_seed = current_sequence_count;

    /* We want to run on the same core on the peer.
     */
    start_req_p->specification.affinity = FBE_RDGEN_AFFINITY_FIXED;
    start_req_p->specification.core = ts_p->core_num;

    /* If we are doing regions, copy over the region info we want the peer to execute for.
     */
    if (fbe_rdgen_specification_options_is_set(&ts_p->request_p->specification,
                                               FBE_RDGEN_OPTIONS_CREATE_REGIONS) ||
        fbe_rdgen_specification_options_is_set(&ts_p->request_p->specification,
                                               FBE_RDGEN_OPTIONS_USE_REGIONS))
    {
        start_req_p->specification.region_list_length = 1;
        start_req_p->specification.region_list[0] = ts_p->request_p->specification.region_list[ts_p->region_index];


        /* Mark this region index as being done by the peer if this is the point where we are
         * setting the pattern. 
         */
        if (fbe_rdgen_operation_is_modify(ts_p->operation))
        {
            if (fbe_rdgen_cmi_get_sp_id() == FBE_DATA_PATTERN_SP_ID_A)
            {
                ts_p->request_p->specification.region_list[ts_p->region_index].sp_id = FBE_DATA_PATTERN_SP_ID_B;
            }
            else
            {
                ts_p->request_p->specification.region_list[ts_p->region_index].sp_id = FBE_DATA_PATTERN_SP_ID_A;
            }
            fbe_rdgen_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, ts_p->object_p->object_id,
                                    "%s: writing on peer: SP %x lba: 0x%llx bl: 0x%llx\n", 
                                    __FUNCTION__,
				    ts_p->request_p->specification.region_list[ts_p->region_index].sp_id,
				    (unsigned long long)lba,
				    (unsigned long long)blocks);
        }
    }
    /* Setup the filter to just send to a single object.
     */
    start_req_p->filter.class_id = FBE_CLASS_ID_INVALID;
    start_req_p->filter.object_id = ts_p->object_p->object_id;
    start_req_p->filter.filter_type = FBE_RDGEN_FILTER_TYPE_OBJECT;
    start_req_p->filter.package_id = ts_p->object_p->package_id;
    ts_p->cmi_msg.peer_ts_ptr = ts_p;
    /* Setup just a single thread with one pass and no special peer options.
     */
    start_req_p->specification.threads = 1;

    /* We need to let the peer we are initiating it.  This allows the peer to disallow 
     * any requests that are coming from the user on the passive side. 
     */
    start_req_p->specification.b_from_peer = FBE_TRUE;
    /* We do not want the peer to be executing any of the options to load balance or send the request to the peer.
     */
    start_req_p->specification.peer_options = FBE_RDGEN_PEER_OPTIONS_INVALID;
    start_req_p->specification.max_passes = 1;

    /* Setup constant size and constant lba.
     */
    start_req_p->specification.block_spec = FBE_RDGEN_BLOCK_SPEC_CONSTANT;
    start_req_p->specification.min_blocks = blocks;
    start_req_p->specification.max_blocks = blocks;
    start_req_p->specification.inc_blocks = 0;
    start_req_p->specification.lba_spec = FBE_RDGEN_LBA_SPEC_FIXED;
    start_req_p->specification.start_lba = lba;
    start_req_p->specification.min_lba = lba;
    start_req_p->specification.max_lba = lba + blocks;
    start_req_p->specification.inc_lba = 0;
    start_req_p->specification.client_reject_allowed_flags = ts_p->request_p->specification.client_reject_allowed_flags;
    start_req_p->specification.arrival_reject_allowed_flags = ts_p->request_p->specification.arrival_reject_allowed_flags;

    fbe_rdgen_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO,
                            "%s: ts_p: %p. object_id: 0x%x lba: 0x%llx bl: 0x%llx\n", 
                            __FUNCTION__, ts_p, ts_p->object_p->object_id, (unsigned long long)lba, (unsigned long long)blocks);
    

    fbe_rdgen_lock();
    if ((fbe_rdgen_get_num_outstanding_peer_req() >= FBE_RDGEN_MAX_PEER_REQUESTS) ||
        (fbe_rdgen_get_peer_out_of_resource_count() > 0)                             )
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO, ts_p->object_p->object_id,
                                "enqueue peer queue: ts_p: 0x%x peer outstanding: %d out of resource: %d lba: 0x%llx bl: 0x%llx\n", 
                                (fbe_u32_t)ts_p,
                                fbe_rdgen_get_num_outstanding_peer_req(),
                                fbe_rdgen_get_peer_out_of_resource_count(),
                                (unsigned long long)lba, (unsigned long long)blocks);
        fbe_rdgen_wait_peer_ts_enqueue(ts_p);
        fbe_rdgen_unlock();
    }
    else
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_DEBUG_LOW, ts_p->object_p->object_id,
                                "send to peer: ts_p: 0x%x.  packet: 0x%x lba: 0x%llx bl: 0x%llx\n", 
                                (fbe_u32_t)ts_p, (fbe_u32_t)&ts_p->read_transport.packet, (unsigned long long)lba, (unsigned long long)blocks);
        fbe_rdgen_peer_packet_enqueue(packet_p);
        fbe_rdgen_inc_requests_to_peer();
        fbe_rdgen_inc_num_outstanding_peer_req();
        fbe_rdgen_unlock();
        status = fbe_rdgen_cmi_send_message(&ts_p->cmi_msg, NULL);
    }
    return status;
}
/******************************************
 * end fbe_rdgen_ts_send_peer_request()
 ******************************************/
/*!**************************************************************
 * fbe_rdgen_ts_send_to_peer()
 ****************************************************************
 * @brief
 *  Send this ts to be executed on the peer.
 *
 * @param ts_p - Current TS that needs to be sent to the peer.
 *
 * @return fbe_status_t 
 *
 * @author
 *  9/24/2010 - Created. Rob Foley
 *
 ****************************************************************/
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_send_to_peer(fbe_rdgen_ts_t *ts_p)
{
    fbe_status_t status;
    ts_p->b_send_to_peer = FBE_TRUE;
    /* Enter through a common handler on the way back.
     */
    fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_handle_peer_error);
    status = fbe_rdgen_ts_send_peer_request(ts_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: failed sending to peer status: 0x%x lba: 0x%llx blocks: 0x%llx object_id: 0x%x\n",
                                __FUNCTION__, status,
				(unsigned long long)ts_p->lba,
				(unsigned long long)ts_p->blocks,
				ts_p->object_p->object_id);
        fbe_rdgen_ts_inc_error_count(ts_p);
        fbe_rdgen_ts_set_state(ts_p, fbe_rdgen_ts_finished);
        return FBE_RDGEN_TS_STATE_STATUS_EXECUTING; 
    }
    return FBE_RAID_STATE_STATUS_WAITING;
}
/**************************************
 * end fbe_rdgen_ts_send_to_peer()
 **************************************/

/*!**************************************************************
 * fbe_rdgen_ts_should_send_to_peer()
 ****************************************************************
 * @brief
 *  Determine if this ts should be executed on the peer.
 *
 * @param ts_p - Current TS that needs to be sent to the peer.
 *
 * @return fbe_bool_t
 *
 * @author
 *  9/24/2010 - Created. Rob Foley
 *
 ****************************************************************/
fbe_bool_t fbe_rdgen_ts_should_send_to_peer(fbe_rdgen_ts_t *ts_p)
{
    fbe_u32_t thread_num;

    if (!fbe_rdgen_service_is_flag_set(FBE_RDGEN_SERVICE_FLAGS_PEER_ALIVE))
    {
        /* If the peer is not there then we can't send any request through the peer.
         */
        ts_p->b_send_to_peer = FBE_FALSE;
    }
    else if (ts_p->request_p->specification.b_from_peer)
    {
        /* If we already came from the peer, we cannot be directed back since the 
         * peer is telling us to execute a single request. 
         */
        ts_p->b_send_to_peer = FBE_FALSE;
    }
    else if (ts_p->request_p->specification.peer_options & FBE_RDGEN_PEER_OPTIONS_SEND_THRU_PEER)
    {
        ts_p->b_send_to_peer = FBE_TRUE;
    }
    else if (ts_p->request_p->specification.peer_options & FBE_RDGEN_PEER_OPTIONS_RANDOM)
    {
        /* Get a random number. 
         * If it is a multiple of 2 then send the request to the peer. 
         * Otherwise send it locally. 
         */
        if ((fbe_random() % 2) == 0)
        {
            ts_p->b_send_to_peer = FBE_TRUE;
        }
        else
        {
            ts_p->b_send_to_peer = FBE_FALSE;
        }
    }
    else if (ts_p->request_p->specification.peer_options & FBE_RDGEN_PEER_OPTIONS_LOAD_BALANCE)
    {
        if (ts_p->request_p->specification.threads == 1)
        {
            /* Can't load balance with just one thread.
             */
            return FBE_FALSE;
        }
        thread_num = fbe_rdgen_ts_get_thread_num(ts_p);

        if ((thread_num % 2) == 0)
        {
            /* Even requests stay local.
             */
            ts_p->b_send_to_peer = FBE_TRUE;
        }
        else
        {
            /* Odd requests go to the peer.
             */
            ts_p->b_send_to_peer = FBE_FALSE;
        }
    }
    return ts_p->b_send_to_peer;
}
/**************************************
 * end fbe_rdgen_ts_should_send_to_peer()
 **************************************/

/*!**************************************************************
 * fbe_rdgen_peer_mark_ts_successful()
 ****************************************************************
 * @brief
 *  Mark ts as if it completed successfully.
 *
 * @param ts_p - Current TS.
 *
 * @return fbe_bool_t
 *
 * @author
 *  10/8/2010 - Created. Rob Foley
 *
 ****************************************************************/
static void fbe_rdgen_peer_mark_ts_successful(fbe_rdgen_ts_t *ts_p)
{
    ts_p->cmi_msg.status = FBE_STATUS_OK;
    ts_p->cmi_msg.msg.start_request.start_io.status.block_status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS;
    ts_p->cmi_msg.msg.start_request.start_io.status.block_qualifier = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE;
    ts_p->cmi_msg.msg.start_request.start_io.status.status = FBE_STATUS_OK;
    ts_p->cmi_msg.msg.start_request.start_io.status.rdgen_status = FBE_RDGEN_OPERATION_STATUS_PEER_DIED;

    ts_p->last_packet_status.block_status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS;
    ts_p->last_packet_status.block_qualifier = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE;
    ts_p->last_packet_status.status = FBE_STATUS_OK;
    return;
}
/******************************************** 
 * end fbe_rdgen_peer_mark_ts_successful()
 ********************************************/


/*!**************************************************************
 * fbe_rdgen_peer_died_restart_packets()
 ****************************************************************
 * @brief
 *  Kick off any packets that were waiting for a response.
 *
 * @param None.
 *
 * @return fbe_status_t
 *
 * @author
 *  10/4/2010 - Created. Rob Foley
 * 
 ****************************************************************/
fbe_status_t fbe_rdgen_peer_died_restart_packets(void)
{
    fbe_packet_t *current_packet_p = NULL;
    fbe_queue_element_t *queue_element_p = NULL;
    fbe_queue_head_t *queue_p = NULL;
    fbe_queue_head_t tmp_queue;
    fbe_u32_t packet_count = 0;
    fbe_rdgen_ts_t *current_ts_p = NULL;

    fbe_queue_init(&tmp_queue);

    fbe_rdgen_lock();
    fbe_rdgen_get_peer_packet_queue(&queue_p);

    /* Just move all packets to tmp queue.
     */
    while (!fbe_queue_is_empty(queue_p))
    {
        queue_element_p = fbe_queue_pop(queue_p);
        fbe_queue_push(&tmp_queue, queue_element_p);
        
        packet_count++;
    }

    fbe_rdgen_get_wait_peer_ts_queue(&queue_p);

    /* Just move all packets to tmp queue.
     */
    while (!fbe_queue_is_empty(queue_p))
    {
        queue_element_p = fbe_queue_pop(queue_p);
        current_ts_p = fbe_rdgen_ts_thread_queue_element_to_ts_ptr(queue_element_p);
        fbe_queue_push(&tmp_queue, &current_ts_p->read_transport.packet.queue_element);
        
        packet_count++;
    }
    
    fbe_rdgen_unlock();

    fbe_rdgen_service_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                            "%s: %d packets restarted.\n", __FUNCTION__, packet_count);
    
    while (!fbe_queue_is_empty(&tmp_queue))
    {
        fbe_rdgen_ts_t *ts_p = NULL;
        queue_element_p = fbe_queue_pop(&tmp_queue);
        current_packet_p = fbe_transport_queue_element_to_packet(queue_element_p);
        
        /* Get the packet so we can fill in the cmi msg.
         */
        ts_p = fbe_rdgen_ts_packet_to_ts_ptr(current_packet_p);

        /* We will make it look as if the request to the peer was successful.
         */
        fbe_rdgen_peer_mark_ts_successful(ts_p);

        /* Complete the packet that is waiting for this response.
         */
        fbe_transport_complete_packet(current_packet_p);
    }

    fbe_queue_destroy(&tmp_queue);
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_rdgen_peer_died_restart_packets()
 **************************************/

/*!*******************************************************************
 * @var init_cmi_msg_p
 *********************************************************************
 * @brief global cmi init message we send to the peer.
 *********************************************************************/

fbe_status_t fbe_rdgen_peer_send_peer_alive_msg(void)
{
    fbe_status_t status;

    init_cmi_msg_p->metadata_cmi_message_type = FBE_RDGEN_CMI_MESSAGE_TYPE_PEER_ALIVE;
    status = fbe_rdgen_cmi_send_message(init_cmi_msg_p, NULL);
    return status;
}
fbe_status_t fbe_rdgen_peer_thread_process_peer_init(void)
{
    fbe_status_t status;
    /* We set the flag to mark the peer alive once the message is received by the peer.
     */
    fbe_rdgen_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                            "rdgen: Peer is alive\n");
    /* If we do not know the peer is there, tell the peer we are alive.
     */
    status = fbe_rdgen_peer_send_peer_alive_msg();
    if (status != FBE_STATUS_OK)
    {
        fbe_rdgen_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: status %d sending peer alive msg. \n", __FUNCTION__, status);
        return status; 
    }
    return FBE_STATUS_OK;
}

/*************************
 * end file fbe_rdgen_peer.c
 *************************/
