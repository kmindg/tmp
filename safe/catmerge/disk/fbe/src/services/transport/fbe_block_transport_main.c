/***************************************************************************
 * Copyright (C) EMC Corporation 2008-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/
/*!**************************************************************************
 * @file fbe_block_transport_main.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the functions for the FBE block transport.

 *  The block transport is an FBE functional transport that is used for 
 *  logical block commands such as read and write.
 *
 *  The definition of the block transport payload is the 
 *  @ref fbe_payload_block_operation_t.  And the valid opcodes for this functional
 *  transport are the @ref fbe_payload_block_operation_opcode_t for functional packets
 *  and the @ref fbe_block_transport_control_code_t for control packets.
 *
 * @version
 *   6/2008:  Created. Peter Puhov.
 *
 ***************************************************************************/
 
/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe_block_transport.h"
#include "fbe_service_manager.h"
#include "fbe_topology.h"

#define FBE_BLOCK_TRANSPORT_SERVER_MAX_CLIENT_COUNT 0x0000FFFF
static fbe_u8_t fbe_block_transport_server_queue_ratio_addend = FBE_BLOCK_TRANSPORT_NORMAL_QUEUE_RATIO_ADDEND;

/* Forward declarations */
static fbe_status_t block_transport_server_enqueue_packet(fbe_block_transport_server_t * block_transport_server,
                                                            fbe_packet_t * packet,
                                                            fbe_transport_entry_context_t context);

static fbe_status_t block_transport_server_bouncer_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_lifecycle_status_t block_transport_server_drain_io(fbe_block_transport_server_t * block_transport_server,
                                                              fbe_lifecycle_state_t lifecycle_state);

static fbe_status_t block_transport_server_complete_canceled_packets(fbe_queue_head_t * queue_head, fbe_spinlock_t *  queue_lock);

static fbe_status_t block_transport_server_allocate_tag(fbe_block_transport_server_t * block_transport_server, fbe_packet_t * packet);
static fbe_status_t block_transport_server_release_tag(fbe_block_transport_server_t * block_transport_server, fbe_packet_t * packet);
static fbe_status_t block_transport_server_bouncer_completion_with_offset(fbe_packet_t * packet, fbe_packet_completion_context_t context);


static fbe_status_t fbe_block_transport_drain_queue(fbe_block_transport_server_t* block_transport_server,
                                                     fbe_queue_head_t *queue_head,
                                                    fbe_status_t completion_status);

static fbe_status_t 
fbe_block_transport_block_operation_completion(fbe_packet_t * packet_p,
                                               fbe_packet_completion_context_t context);

static fbe_status_t block_transport_server_restart_io(fbe_block_transport_server_t * block_transport_server, fbe_queue_head_t * tmp_queue);

static fbe_status_t block_transport_server_run_queue_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_status_t fbe_block_transport_server_prepare_packet(fbe_block_transport_server_t * block_transport_server,
																fbe_packet_t * packet,
																fbe_transport_entry_context_t context);
static fbe_status_t 
block_transport_server_metadata_io_bouncer_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t block_transport_server_retry_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_lifecycle_status_t fbe_block_transport_server_pending_done(fbe_block_transport_server_t * block_transport_server,
                                                                      fbe_lifecycle_const_t * p_class_const,
                                                                      struct fbe_base_object_s * base_object,
                                                                      fbe_block_path_attr_flags_t attribute_exception);
static __forceinline fbe_u32_t block_transport_server_get_io_max(fbe_block_transport_server_t * block_transport_server, 
																 fbe_packet_priority_t packet_priority)
{
	if(packet_priority != FBE_PACKET_PRIORITY_LOW){
		return block_transport_server->outstanding_io_max;
	}

	return (block_transport_server->outstanding_io_max / 4) * 3;
}

/*!**************************************************************
 * @fn fbe_block_transport_send_control_packet(
 *             fbe_block_edge_t * block_edge,
 *             fbe_packet_t * packet)
 ****************************************************************
 * @brief
 *  This function sends a control packet to the block transport.
 *
 * @param block_edge - The edge to send the control packet to.
 * @param packet - The packet to send to the block tranasport.
 *
 * @return fbe_status_t
 *
 ****************************************************************/
fbe_status_t
fbe_block_transport_send_control_packet(fbe_block_edge_t * block_edge, fbe_packet_t * packet)
{
    fbe_object_id_t server_id;
    fbe_status_t status;

    /* Set packet address */
    fbe_base_transport_get_server_id((fbe_base_edge_t *)block_edge, &server_id);

    fbe_transport_set_address(packet,
                              block_edge->server_package_id,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              server_id);

    /* We also should attach edge to the packet */
    packet->base_edge = (fbe_base_edge_t *)block_edge;

    /* Control packets should be sent via service manager */
    status = fbe_service_manager_send_control_packet(packet);
    return status;
}

/*!**************************************************************
 * fbe_block_transport_send_funct_packet_params
 ****************************************************************
 * @brief
 *  This function sends a functional packet to the block transport.
 *  The assumption is that the functional packet is well formed.
 * 
 *  This function will increment the payload level in the packet payload.
 *
 * @param block_edge - The edge to send the packet to.
 * @param packet - The packet to send to the block tranasport.
 * @param b_check_capacity - FBE_FALSE to skip the capacity check.
 *
 * @return fbe_status_t
 *
 ****************************************************************/
fbe_status_t
fbe_block_transport_send_funct_packet_params(fbe_block_edge_t * block_edge, 
                                             fbe_packet_t * packet,
                                             fbe_bool_t b_check_capacity)
{
    fbe_status_t status;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_block_operation_t *block_operation_p = NULL;
    fbe_edge_hook_function_t hook;

    payload = fbe_transport_get_payload_ex(packet);
    status = fbe_payload_ex_increment_block_operation_level(payload);

    if(status != FBE_STATUS_OK){
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet_async(packet);
        return status;
    }

    /* AR 496054: The request memory should not be in used in the same packet as well as 
     * the subpacket queue head and queue element.  Just panic for now to see why.
     */
    if(fbe_memory_request_is_in_use(&packet->memory_request)) {
        fbe_base_transport_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
                                 FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                 "blk_trans_send_pkt: Requested Memory: %p in used. Packet: %p\n", 
                                 &packet->memory_request, packet);
    }

    if(!fbe_queue_is_empty(&packet->subpacket_queue_head)) {
        fbe_base_transport_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
                                 FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                 "blk_trans_send_pkt: sub_queue_head: %p not empty. Packet: %p\n", 
                                 &packet->subpacket_queue_head, packet);
    }

    if(fbe_queue_is_element_on_queue(&packet->queue_element)) {
        fbe_base_transport_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
                                 FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                 "blk_trans_send_pkt: queue_elem: %p on queue. Packet: %p\n", 
                                 &packet->queue_element, packet);
    }

    block_operation_p = fbe_payload_ex_get_block_operation(payload);

    if (block_operation_p->block_edge_p == NULL) {
        fbe_payload_block_set_block_edge(block_operation_p, block_edge);
    }

	/* Performance optimization */
	if((block_edge->io_entry == NULL) || (block_edge->object_handle == NULL)){
		status = fbe_base_transport_send_functional_packet((fbe_base_edge_t *) block_edge, packet, block_edge->server_package_id);
		return status;
	}
	
		/* It would be nice to check edge state here(we allow sending packets to a sleeping edge, it will wake him up) */
	if(( ((fbe_base_edge_t*)block_edge)->path_state != FBE_PATH_STATE_ENABLED) && (((fbe_base_edge_t*)block_edge)->path_state != FBE_PATH_STATE_SLUMBER)){
		fbe_transport_set_status(packet, FBE_STATUS_EDGE_NOT_ENABLED, 0);
		fbe_transport_complete_packet_async(packet);
		return FBE_STATUS_EDGE_NOT_ENABLED;
	}

    /* validate that the request does not exceed the edge capacity */
    if (b_check_capacity &&
        ((block_operation_p->lba + block_operation_p->block_count) > block_edge->capacity)) {
        /* Something wrong with the request, it is beyond the capacity of the 
         * extent. Mark the error in the block transport payload.
         */
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                 FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                 "block_transport: lba=0x%llx block_count=0x%llx beyond edge capacity=0x%llx\n", 
                                 (unsigned long long)block_operation_p->lba,
				 (unsigned long long)block_operation_p->block_count,
				 (unsigned long long)block_edge->capacity);
        fbe_payload_block_set_status(block_operation_p,
                                     FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                                     FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_CAPACITY_EXCEEDED);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet_async(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

	/* Set packet address */
	fbe_transport_set_address(	packet,
								block_edge->server_package_id,
								FBE_SERVICE_ID_TOPOLOGY,
								FBE_CLASS_ID_INVALID,
								((fbe_base_edge_t*)block_edge)->server_id);

	/* We also should attach edge to the packet */
	packet->base_edge = (fbe_base_edge_t *)block_edge;

	/* Check if the hook is set. */ 
    hook = ((fbe_base_edge_t*)block_edge)->hook; 
	if(hook != NULL) {
		/* The hook is set, redirect the packet to hook. */
		/* It is up to the hook to decide on how to complete the packet*/
		/* TODO: change the trace level to debug, once we are done debugging*/
		fbe_base_transport_trace(FBE_TRACE_LEVEL_DEBUG_MEDIUM,
								FBE_TRACE_MESSAGE_ID_INFO,
								"%s redirect packet with srv id %d to edge hook function.\n", __FUNCTION__, ((fbe_base_edge_t*)block_edge)->server_id);
        status = hook(packet);
		if(status != FBE_STATUS_CONTINUE){
			return status;
		}
	}

	/* No hook, continue with normal operation */
	status = block_edge->io_entry(block_edge->object_handle, packet);

    return status;
}
/*!**************************************************************
 * @fn fbe_block_transport_send_functional_packet(
 *             fbe_block_edge_t * block_edge,
 *             fbe_packet_t * packet)
 ****************************************************************
 * @brief
 *  This function sends a functional packet to the block transport.
 *  The assumption is that the functional packet is well formed.
 * 
 *  This function will increment the payload level in the packet payload.
 *
 * @param block_edge - The edge to send the packet to.
 * @param packet - The packet to send to the block tranasport.
 *
 * @return fbe_status_t
 *
 ****************************************************************/
fbe_status_t
fbe_block_transport_send_functional_packet(fbe_block_edge_t * block_edge, fbe_packet_t * packet)
{
    fbe_status_t status;
    status = fbe_block_transport_send_funct_packet_params(block_edge, 
                                                          packet, 
                                                          FBE_TRUE /* Check capacity)*/);
    return status;
}

/*!**************************************************************
 * fbe_block_transport_send_no_autocompletion()
 ****************************************************************
 * @brief
 *  This function sends a functional packet to the block transport.
 *  The packet does not get auto completed if the edge state is bad.
 * 
 *  This function will increment the payload level in the packet payload.
 *
 * @param block_edge - The edge to send the packet to.
 * @param packet - The packet to send to the block tranasport.
 *
 * @return fbe_status_t
 *
 ****************************************************************/
fbe_status_t
fbe_block_transport_send_no_autocompletion(fbe_block_edge_t * block_edge, fbe_packet_t * packet,
                                           fbe_bool_t b_check_capacity)
{
    fbe_status_t status;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_block_operation_t *block_operation_p = NULL;
    fbe_edge_hook_function_t hook;

    payload = fbe_transport_get_payload_ex(packet);
    status = fbe_payload_ex_increment_block_operation_level(payload);

    if(status != FBE_STATUS_OK){
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet_async(packet);
        return status;
    }

    /* AR 496054: The request memory should not be in used in the same packet as well as 
     * the subpacket queue head and queue element.  Just panic for now to see why.
     */
    if(fbe_memory_request_is_in_use(&packet->memory_request)) {
        fbe_base_transport_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
                                 FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                 "blk_trans_send_pkt: Requested Memory: %p in used. Packet: %p\n", 
                                 &packet->memory_request, packet);
    }

    if(!fbe_queue_is_empty(&packet->subpacket_queue_head)) {
        fbe_base_transport_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
                                 FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                 "blk_trans_send_pkt: sub_queue_head: %p not empty. Packet: %p\n", 
                                 &packet->subpacket_queue_head, packet);
    }

    if(fbe_queue_is_element_on_queue(&packet->queue_element)) {
        fbe_base_transport_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
                                 FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                 "blk_trans_send_pkt: queue_elem: %p on queue. Packet: %p\n", 
                                 &packet->queue_element, packet);
    }

    block_operation_p = fbe_payload_ex_get_block_operation(payload);

    if (block_operation_p->block_edge_p == NULL) {
        fbe_payload_block_set_block_edge(block_operation_p, block_edge);
    }

    /* validate that the request does not exceed the edge capacity */
    if (b_check_capacity && 
        ((block_operation_p->lba + block_operation_p->block_count) > block_edge->capacity)) {
        /* Something wrong with the request, it is beyond the capacity of the 
         * extent. Mark the error in the block transport payload.
         */
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                 FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                 "block_transport: lba=0x%llx block_count=0x%llx beyond edge capacity=0x%llx\n", 
                                 (unsigned long long)block_operation_p->lba, (unsigned long long)block_operation_p->block_count, (unsigned long long)block_edge->capacity);
        fbe_payload_block_set_status(block_operation_p,
                                     FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                                     FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_CAPACITY_EXCEEDED);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet_async(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

	/* Performance optimization */
	if((block_edge->io_entry == NULL) || (block_edge->object_handle == NULL)){
		status = fbe_base_transport_send_functional_packet((fbe_base_edge_t *) block_edge, packet, block_edge->server_package_id);
		return FBE_STATUS_PENDING;
	}
	
		/* It would be nice to check edge state here(we allow sending packets to a sleeping edge, it will wake him up) */
	if(( ((fbe_base_edge_t*)block_edge)->path_state != FBE_PATH_STATE_ENABLED) && (((fbe_base_edge_t*)block_edge)->path_state != FBE_PATH_STATE_SLUMBER)){
		fbe_transport_set_status(packet, FBE_STATUS_EDGE_NOT_ENABLED, __LINE__);

        //fbe_transport_complete_packet_async(packet);
		return FBE_STATUS_EDGE_NOT_ENABLED;
	}

	/* Set packet address */
	fbe_transport_set_address(	packet,
								block_edge->server_package_id,
								FBE_SERVICE_ID_TOPOLOGY,
								FBE_CLASS_ID_INVALID,
								((fbe_base_edge_t*)block_edge)->server_id);

	/* We also should attach edge to the packet */
	packet->base_edge = (fbe_base_edge_t *)block_edge;

	/* Check if the hook is set. */ 
    hook = ((fbe_base_edge_t*)block_edge)->hook; 
	if(hook != NULL) {
		/* The hook is set, redirect the packet to hook. */
		/* It is up to the hook to decide on how to complete the packet*/
		/* TODO: change the trace level to debug, once we are done debugging*/
		fbe_base_transport_trace(FBE_TRACE_LEVEL_DEBUG_MEDIUM,
								FBE_TRACE_MESSAGE_ID_INFO,
								"%s redirect packet with srv id %d to edge hook function.\n", __FUNCTION__, ((fbe_base_edge_t*)block_edge)->server_id);
        status = hook(packet);
		if(status != FBE_STATUS_CONTINUE){
			return status;
		}
	}

	/* No hook, continue with normal operation */
	status = block_edge->io_entry(block_edge->object_handle, packet);
    return status;
}
/*!**************************************************************
 * @fn fbe_block_transport_server_pending(
 *         fbe_block_transport_server_t * block_transport_server,
 *         fbe_lifecycle_const_t * p_class_const,
 *         struct fbe_base_object_s * base_object)
 ****************************************************************
 * @brief
 *  This is the pending function, which gets called when an object, which
 *  contains a block transport server, goes into a pending state.
 * 
 *  This function should be called by an object that contains a block transport
 * 
 *  This function will perform the necessary draining in order to get the
 *  object ready to transition to the next state.
 *
 * @param block_transport_server - The server to attach for.
 * @param p_class_const - This is the class lifecycle constant.
 * @param base_object - This is the ptr to the base object that we are
 *                      attaching this edge for.
 *                      This is used in conjunction with the p_class_const to
 *                      determine the state of the object.
 *
 * @return fbe_status_t
 *
 ****************************************************************/
fbe_lifecycle_status_t
fbe_block_transport_server_pending(fbe_block_transport_server_t * block_transport_server,
                                       fbe_lifecycle_const_t * p_class_const,
                                       struct fbe_base_object_s * base_object)
{
    fbe_lifecycle_status_t lifecycle_status;
    fbe_lifecycle_state_t lifecycle_state;
    fbe_status_t status;

    status = fbe_lifecycle_get_state(p_class_const, base_object, &lifecycle_state);
    if(status != FBE_STATUS_OK){
        /* KvTrace("%s Critical error fbe_lifecycle_get_state failed, status = %X \n", __FUNCTION__, status); */
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR, 
                                 FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                                 "%s Critical error fbe_lifecycle_get_state failed, status = %X", __FUNCTION__, status);
        /* We have no error code to return */
    }

    /* Update path state for cases where we should not have I/Os in flight.
     * This prevents new I/Os from arriving. 
     * If we are pending ready the base transport server pending below will update the path state. 
     */
    if (lifecycle_state != FBE_LIFECYCLE_STATE_PENDING_READY){
        fbe_block_transport_server_update_path_state(block_transport_server,
                                                     p_class_const,
                                                     base_object,
                                                     FBE_BLOCK_PATH_ATTR_FLAGS_NONE);
    }

    lifecycle_status = block_transport_server_drain_io(block_transport_server, lifecycle_state);

    if(lifecycle_status == FBE_LIFECYCLE_STATUS_DONE) {
        lifecycle_status = fbe_base_transport_server_pending((fbe_base_transport_server_t *) block_transport_server,
                                                                p_class_const,
                                                                base_object);
    }
    return lifecycle_status;
}

/*!**************************************************************
 * fbe_block_transport_server_pending_with_exception(
 *         fbe_block_transport_server_t * block_transport_server,
 *         fbe_lifecycle_const_t * p_class_const,
 *         struct fbe_base_object_s * base_object)
 ****************************************************************
 * @brief
 *  This is the pending function, which gets called when an object, which
 *  contains a block transport server, goes into a pending state.
 * 
 *  This function should be called by an object that contains a block transport
 * 
 *  This function will perform the necessary draining in order to get the
 *  object ready to transition to the next state.
 *
 * @param block_transport_server - The server to attach for.
 * @param p_class_const - This is the class lifecycle constant.
 * @param base_object - This is the ptr to the base object that we are
 *                      attaching this edge for.
 *                      This is used in conjunction with the p_class_const to
 *                      determine the state of the object.
 * @param attributes - The attributes that should be an exception where we
 *                     will not transition the state if these attributes are set.
 *
 * @return fbe_status_t
 *
 ****************************************************************/
fbe_lifecycle_status_t
fbe_block_transport_server_pending_with_exception(fbe_block_transport_server_t * block_transport_server,
                                                  fbe_lifecycle_const_t * p_class_const,
                                                  struct fbe_base_object_s * base_object,
                                                  fbe_block_path_attr_flags_t attributes)
{
    fbe_lifecycle_status_t lifecycle_status;
    fbe_lifecycle_state_t lifecycle_state;
    fbe_status_t status;

    status = fbe_lifecycle_get_state(p_class_const, base_object, &lifecycle_state);
    if(status != FBE_STATUS_OK){
        /* KvTrace("%s Critical error fbe_lifecycle_get_state failed, status = %X \n", __FUNCTION__, status); */
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR, 
                                 FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                                 "%s Critical error fbe_lifecycle_get_state failed, status = %X", __FUNCTION__, status);
        /* We have no error code to return */
    }

    /* Update path state for cases where we should not have I/Os in flight.
     * This prevents new I/Os from arriving. 
     * If we are pending ready the base transport server pending below will update the path state. 
     */
    if (lifecycle_state != FBE_LIFECYCLE_STATE_PENDING_READY){
        fbe_block_transport_server_update_path_state(block_transport_server,
                                                     p_class_const,
                                                     base_object,
                                                     attributes);
    }

    lifecycle_status = block_transport_server_drain_io(block_transport_server, lifecycle_state);

    if(lifecycle_status == FBE_LIFECYCLE_STATUS_DONE) {
        lifecycle_status = fbe_block_transport_server_pending_done(block_transport_server,
                                                                   p_class_const,
                                                                   base_object, 
                                                                   attributes);
    }
    return lifecycle_status;
}
/*!**************************************************************
 * fbe_block_transport_server_start_packet()
 ****************************************************************
 * @brief
 *  Apply any offset to the packet.
 *  Start the packet to the block_transport_entry.
 *
 * @param block_edge - The edge to send the packet to.
 * @param packet - The packet to send to the block tranasport.
 *
 * @return fbe_status_t
 *
 ****************************************************************/
fbe_status_t fbe_block_transport_server_start_packet(fbe_block_transport_server_t * block_transport_server,
                                        fbe_packet_t * packet,
                                        fbe_transport_entry_context_t context)
{
    fbe_status_t status;
    fbe_block_edge_t *block_edge = NULL;

    fbe_payload_ex_t * payload = NULL;
    fbe_payload_block_operation_t *block_operation_orig = NULL;
    fbe_payload_block_operation_t *block_operation_new = NULL;
    fbe_package_id_t package_id;
    fbe_path_attr_t     path_attr = 0;

    block_edge = (fbe_block_edge_t *)packet->base_edge;

    if(block_edge != NULL) 
    {
        fbe_base_transport_get_path_attributes(packet->base_edge, &path_attr);
    }
    

    fbe_transport_get_package_id(packet, &package_id);
    if (package_id == FBE_PACKAGE_ID_PHYSICAL){
        fbe_transport_set_completion_function(packet, block_transport_server_bouncer_completion, block_transport_server);
        status = block_transport_server->block_transport_const->block_transport_entry(context, packet);
        return status;
    }

    /* If we have an edge offset, then apply the offset to the block operation.
     *  
     * Check the edge type, since there are times when a packet gets sent over other edge 
     * types and the transport ID changes (e.g. ssp) and on completion we cannot do the 
     * offset handling if the edge is not a block transport edge. 
     *
     * Due to the fact that the edge between the raid group and virtual drive must
     * have the total offset, the offset between the virtual drive and provision drive
     * should not be added (it would result in the offset being added twice).  The flag
     * `FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_IGNORE_OFFSET' prevents added the offset twice.
     */
    /*! @todo checking offset is a temparory hack.  When block operation gets increase to 3, 
     *        checking for offset should be removed.  
     */
    if ((block_edge != NULL)                                            &&
        (block_edge->base_edge.transport_id == FBE_TRANSPORT_ID_BLOCK)  &&
        (block_edge->offset > 0)                                        && 
        (!(path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_IGNORE_OFFSET))    )
    {
        /* Allocate the new payload, and increment the level to make this new level 
         * active. 
         */
        payload = fbe_transport_get_payload_ex(packet);
        block_operation_orig = fbe_payload_ex_get_block_operation(payload);
        block_operation_new = fbe_payload_ex_allocate_block_operation(payload);
        fbe_payload_ex_increment_block_operation_level(payload);

        /* Copy the prior operation into the new operation.
         * And apply edge offset to the operation.
         */
        block_operation_new->lba = block_operation_orig->lba + block_edge->offset;
        block_operation_new->block_count = block_operation_orig->block_count;
        block_operation_new->block_opcode = block_operation_orig->block_opcode;
        block_operation_new->block_flags = block_operation_orig->block_flags;
        block_operation_new->block_size = block_operation_orig->block_size;
        block_operation_new->optimum_block_size = block_operation_orig->optimum_block_size;
        block_operation_new->payload_sg_descriptor = block_operation_orig->payload_sg_descriptor;
        block_operation_new->pre_read_descriptor = block_operation_orig->pre_read_descriptor;
        //block_operation_new->verify_error_counts_p = block_operation_orig->verify_error_counts_p;
        if (block_operation_orig->pre_read_descriptor != NULL)
        {
            block_operation_orig->pre_read_descriptor->lba += block_edge->offset;
        }
        fbe_payload_block_set_block_edge(block_operation_new, block_edge);

        /* Use a different completion function.
         */
        fbe_transport_set_completion_function(packet, 
                                              block_transport_server_bouncer_completion_with_offset, 
                                              block_transport_server);
    }
    else
    {
        fbe_transport_set_completion_function(packet, 
                                              block_transport_server_bouncer_completion, 
                                              block_transport_server);
    }
    status = block_transport_server->block_transport_const->block_transport_entry(context, packet);
    return status;
}

/*!**************************************************************
 * @fn fbe_block_transport_server_bouncer_entry(
 *         fbe_block_transport_server_t * block_transport_server,
 *         fbe_packet_t * packet,
 *         fbe_transport_entry_context_t context)
 ****************************************************************
 * @brief
 *  This is the entry point to the block transport server for handling
 *  new I/Os
 *  This function should be called by an object that contains a block transport
 *  when it receives a new block operation.  This allows the block transport
 *  to enforce policies about queueing I/Os.
 *
 * @param block_transport_server - The server this I/O is arriving for.
 * @param packet - The new packet that is arriving.
 * @param context - This is the context that should be sent to the
 *                  @ref fbe_block_transport_server_t::block_transport_const
 *                  transport entry function.
 *                  Typically this context is the ptr to the object.
 *
 * @return fbe_status_t
 *
 ****************************************************************/
fbe_status_t
fbe_block_transport_server_bouncer_entry(fbe_block_transport_server_t * block_transport_server,
                                         fbe_packet_t * packet,
                                         fbe_transport_entry_context_t context)
{
    fbe_status_t status;
    fbe_packet_priority_t packet_priority;
    fbe_payload_block_operation_t *block_operation_p = NULL;
    fbe_block_edge_t * edge_p = (fbe_block_edge_t *)packet->base_edge;    
    fbe_path_state_t    path_state = FBE_PATH_STATE_INVALID;
    fbe_packet_attr_t   packet_attributes = 0;
    fbe_bool_t              queues_not_empty = FBE_FALSE;
    fbe_packet_priority_t   priority;
    fbe_u32_t               index;
	fbe_atomic_t			io_gate = 0;
    fbe_package_id_t package_id;    
	fbe_bool_t is_throttle = FBE_FALSE;
    fbe_block_count_t io_cost;
    fbe_u32_t io_credits;

    /*let's mark the time of the last IO we had, this is used for power saving.
    We don't count background operations such as sniff as an IO, otherwise we will never go to sleep*/
    fbe_transport_get_packet_attr(packet, &packet_attributes);
    if ((!(packet_attributes & FBE_PACKET_FLAG_BACKGROUND_SNIFF)) &&
        (!(block_transport_server->attributes & FBE_BLOCK_TRANSPORT_FLAGS_IO_IN_PROGRESS))){

        /*we will take the actual timestamp in the monitor context*/
        block_transport_server_set_attributes(block_transport_server,
                                              FBE_BLOCK_TRANSPORT_FLAGS_IO_IN_PROGRESS);
    }

    fbe_transport_get_package_id(packet, &package_id);
    if ((packet_attributes & FBE_PACKET_FLAG_DO_NOT_QUIESCE) &&
        !(packet_attributes & FBE_PACKET_FLAG_DO_NOT_HOLD) &&
        (FBE_PACKAGE_ID_SEP_0 == package_id))
    {
        /* In some cases we need to bypass the quiesce code.
         */
        block_transport_server_increment_outstanding_io_count(block_transport_server, packet);        
        status = fbe_block_transport_server_start_packet(block_transport_server, packet, context);
        return FBE_STATUS_PENDING;
    }
    //fbe_transport_get_packet_priority(packet, &packet_priority);

    /* Validate the lba of the block request.
     */
    edge_p = (fbe_block_edge_t *)packet->base_edge;

    if (edge_p != NULL) { 
        /*If we were hibrenating, let's generate an event to the object after queuing the packet.
        The object will use this event from us to wake up a monitor and call fbe_block_transport_server_process_io_from_q
        on the monitor context once it's ready*/
        fbe_block_transport_get_path_state(edge_p, &path_state);
        if (path_state == FBE_PATH_STATE_SLUMBER) {
            fbe_spinlock_lock(&block_transport_server->queue_lock);
            block_transport_server_enqueue_packet(block_transport_server, packet, context);
            fbe_spinlock_unlock(&block_transport_server->queue_lock);
            fbe_transport_set_status(packet, FBE_STATUS_HIBERNATION_EXIT_PENDING, 0);/*mark it in a way it would not be drained from the queue during ACTIVATE*/
            block_transport_server->block_transport_const->block_transport_event_entry(FBE_BLOCK_TRASPORT_EVENT_TYPE_IO_WAITING_ON_QUEUE,
                                                                                       block_transport_server->event_context);
            return FBE_STATUS_PENDING;

        }
    }/* if (edge_p != NULL) */
    else
    {   
        block_operation_p = fbe_transport_get_block_operation(packet);
        if((fbe_payload_block_is_flag_set(block_operation_p, FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_METADATA_OP)) &&
           (FBE_PACKAGE_ID_SEP_0 == package_id))
        {
            block_transport_server_increment_outstanding_io_count(block_transport_server, packet);        
            status = fbe_block_transport_server_start_packet(block_transport_server, packet, context);
            return FBE_STATUS_PENDING; /* We need to confirm that start packet will never return FBE_STATUS_OK */
        }
    }
    

	if(block_transport_server->outstanding_io_max == 0){ /* There are no priority queue's to check */
		/* Check the gate */
		io_gate = fbe_atomic_increment(&block_transport_server->outstanding_io_count);
		if(!(io_gate & FBE_BLOCK_TRANSPORT_SERVER_GATE_BIT)){
			/* Just send the I/O */
			if(block_transport_server->attributes & FBE_BLOCK_TRANSPORT_ENABLE_STACK_LIMIT){
				status = fbe_block_transport_server_prepare_packet(block_transport_server, packet, context);
				return status;
			} else {
				status = fbe_block_transport_server_start_packet(block_transport_server, packet, context);
				return FBE_STATUS_PENDING; /* We need to confirm that start packet will never return FBE_STATUS_OK */
			}
		} else {
			/* We do have a gate, so decrement the I/O count and go to the old code */
			fbe_atomic_decrement(&block_transport_server->outstanding_io_count);
		}
	}

    /* If the LUN is marked for flushing I/O, complete new I/O with error; the
     * LUN is being destroyed.
	 * This is a bad code and we should remove it!!!!!
     */
    if (block_transport_server->attributes & FBE_BLOCK_TRANSPORT_FLAGS_FLUSH_AND_BLOCK) {
        fbe_base_transport_trace(FBE_TRACE_LEVEL_DEBUG_LOW,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_EXIT,
                                "%s packet 0x%p completed with FBE_STATUS_DEAD because flushing I/O for LUN destroy\n", 
                                __FUNCTION__, packet);

        fbe_transport_set_status(packet, FBE_STATUS_DEAD, 0);
        fbe_transport_complete_packet_async(packet);
        return FBE_STATUS_DEAD;
    }

    /* Get the queue lock */
    fbe_spinlock_lock(&block_transport_server->queue_lock);

    /* Check if we need to forcefully complete the packet */
    /* For monitor packets the edge is NULL */
    if((block_transport_server->attributes & FBE_BLOCK_TRANSPORT_ENABLE_FORCE_COMPLETION) && (edge_p != NULL)){
        fbe_spinlock_unlock(&block_transport_server->queue_lock);
        fbe_transport_set_status(packet, block_transport_server->force_completion_status, 0);
        fbe_transport_complete_packet_async(packet);
        return block_transport_server->force_completion_status;
    }

    if ((block_transport_server->outstanding_io_max == 0) &&
        ((block_transport_server->attributes & FBE_BLOCK_TRANSPORT_FLAGS_HOLD) == 0 )) { 
        /* The queueing is disabled. Logical drive case */
        block_transport_server_increment_outstanding_io_count(block_transport_server, packet);
        

        /* Release the queue lock */
        fbe_spinlock_unlock(&block_transport_server->queue_lock);

		if(block_transport_server->attributes & FBE_BLOCK_TRANSPORT_ENABLE_STACK_LIMIT){
			status = fbe_block_transport_server_prepare_packet(block_transport_server, packet, context);
			return status;
		} else {
			status = fbe_block_transport_server_start_packet(block_transport_server, packet, context);
			return FBE_STATUS_PENDING; /* We need to confirm that start packet will never return FBE_STATUS_OK */
		}
        
    }

	fbe_transport_get_packet_priority(packet, &packet_priority);

	/* This code needs to be optimized */
    for(priority = packet_priority; priority < FBE_PACKET_PRIORITY_LAST; priority++) {
        index = priority - 1;
        if(! fbe_queue_is_empty(&block_transport_server->queue_head[index])) {
            queues_not_empty = FBE_TRUE;
            break;
        }
    }

    /* If the HOLD mode is set we have to enqueue the I/O */
    if(block_transport_server->attributes & FBE_BLOCK_TRANSPORT_FLAGS_HOLD){

        /* We should never enqueue redirected IOs.
         */
        if (packet_attributes & FBE_PACKET_FLAG_REDIRECTED) {
            fbe_spinlock_unlock(&block_transport_server->queue_lock);
            fbe_transport_set_status(packet, FBE_STATUS_QUIESCED, 0);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_PENDING;
        }

        /* We should never enqueue background monitor ops under hold.
         * We only clear hold on the monitor context, and if a monitor op is  
         * Queued, we cannot get to monitor context.  
         */
        block_operation_p = fbe_transport_get_block_operation(packet);
        if ((fbe_payload_block_is_monitor_opcode(block_operation_p->block_opcode)) ||
            (packet_attributes & FBE_PACKET_FLAG_DO_NOT_HOLD)) {
            fbe_spinlock_unlock(&block_transport_server->queue_lock);
            fbe_transport_set_status(packet, FBE_STATUS_QUIESCED, 0);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_PENDING;
        }
        block_transport_server_enqueue_packet(block_transport_server, packet, context);
        /* Release the queue lock */
        fbe_spinlock_unlock(&block_transport_server->queue_lock);
        return FBE_STATUS_PENDING;
    }

    block_operation_p = fbe_transport_get_block_operation(packet);
    if ((package_id == FBE_PACKAGE_ID_PHYSICAL) &&
        (block_transport_server->outstanding_io_max != 0) &&
        fbe_payload_block_is_flag_set(block_operation_p, FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_DO_NOT_QUEUE)){
        /* For vault dump I/Os, we will simply set the outstanding io max out for the PDO so we will not queue.
         */
        block_transport_server->outstanding_io_max = 22;
    }
    /* We do not throttle monitor operations. 
     * Monitor operations check before they are issued if they can run. 
     * So once they run we are expected to let them through. 
     */
    if (block_transport_server->io_credits_max != 0){ 
        fbe_bool_t b_do_not_queue = FBE_FALSE;
        block_operation_p = fbe_transport_get_block_operation(packet);

        if (block_transport_server->block_transport_const->calc_io_credits_fn){
            io_credits = (block_transport_server->block_transport_const->calc_io_credits_fn)((struct fbe_base_object_s *)
                                                                                           block_transport_server->event_context,
                                                                                           block_operation_p,
                                                                                           packet,
                                                                                           &b_do_not_queue);
        }
        else {
            io_credits = 1;
        }

        /* Make sure the cost is no more than the max credits. 
         * Otherwise this I/O will never be processed.
         */
        io_credits = FBE_MIN(io_credits, block_transport_server->io_credits_max - 1);
        fbe_payload_block_set_io_credits(block_operation_p, io_credits);

        fbe_base_transport_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO,
                                 "context: %p packet: %p inc 0x%x lba: %llx bl: %llx iw: 0x%x ic: %lld\n", 
                                 block_transport_server->event_context,
                                 packet, block_operation_p->io_credits, block_operation_p->lba, block_operation_p->block_count,
                                 block_transport_server->outstanding_io_credits, block_transport_server->outstanding_io_count);
        if (b_do_not_queue) {
            block_transport_server_increment_outstanding_io_count(block_transport_server, packet);

            if(block_transport_server->io_credits_max != 0){ /* Throttling enabled */
                block_transport_server->outstanding_io_credits += io_credits;
            }

            /* Release the queue lock */
            fbe_spinlock_unlock(&block_transport_server->queue_lock);

            if(block_transport_server->attributes & FBE_BLOCK_TRANSPORT_ENABLE_STACK_LIMIT){
                status = fbe_block_transport_server_prepare_packet(block_transport_server, packet, context);
                return status;
            } else {
                status = fbe_block_transport_server_start_packet(block_transport_server, packet, context);
                return FBE_STATUS_PENDING; /* We need to confirm that start packet will never return FBE_STATUS_OK */
            }
        }
        if ( (io_credits != 0) &&
             (block_transport_server->outstanding_io_credits >= block_transport_server->io_credits_max)) {
            if (block_operation_p->block_flags & FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_ALLOW_FAIL_CONGESTION) {
                fbe_spinlock_unlock(&block_transport_server->queue_lock);
                fbe_payload_block_set_status(block_operation_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED,
                                             FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_CONGESTED);
                fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
                fbe_transport_complete_packet(packet);
                return FBE_STATUS_PENDING;
            }
            is_throttle = FBE_TRUE;
        }
	}
	if(block_transport_server->io_throttle_max != 0){ /* Throttling enabled */
		block_operation_p = fbe_transport_get_block_operation(packet);

		if (block_transport_server->block_transport_const->throttle_calc_fn) {
            io_cost = (block_transport_server->block_transport_const->throttle_calc_fn)((struct fbe_base_object_s *)
                                                                                        block_transport_server->event_context,
                                                                                        block_operation_p);
        }
        else {
            io_cost = block_operation_p->block_count;
        }

        /* Make sure the cost is no more than the max throttle. 
         * Otherwise this I/O will never be processed.  This can happen with zeros.
         */
        io_cost = FBE_MIN(io_cost, block_transport_server->io_throttle_max - 1);
        fbe_payload_block_set_throttle_count(block_operation_p, (fbe_u32_t)io_cost);

        fbe_base_transport_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO,
                                 "context: %p packet: %p inc 0x%x lba: %llx bl: %llx tc: %llx ic: %lld\n", 
                                 block_transport_server->event_context,
                                 packet, block_operation_p->throttle_count, block_operation_p->lba, block_operation_p->block_count,
                                 block_transport_server->io_throttle_count, block_transport_server->outstanding_io_count);
		if(block_transport_server->io_throttle_count + io_cost >= block_transport_server->io_throttle_max){
			is_throttle = FBE_TRUE;
		}
	}

    /* If number of outstanding I/O's greater than outstanding_io_max or one of the queues not empty
        we have to enqueue the I/O
    */
    if(queues_not_empty) {
        block_transport_server_enqueue_packet(block_transport_server, packet, context);
    } else if((fbe_u32_t)(block_transport_server->outstanding_io_count & FBE_BLOCK_TRANSPORT_SERVER_GATE_MASK) >= block_transport_server_get_io_max(block_transport_server, packet_priority)) {
        block_transport_server_enqueue_packet(block_transport_server, packet, context);
	} else if(is_throttle){
		block_transport_server_enqueue_packet(block_transport_server, packet, context);
	} else {
        if(block_transport_server->attributes & FBE_BLOCK_TRANSPORT_FLAGS_TAGS_ENABLED){
            block_transport_server_allocate_tag(block_transport_server, packet);
        }
        block_transport_server_increment_outstanding_io_count(block_transport_server, packet);

        if(block_transport_server->io_throttle_max != 0){ /* Throttling enabled */
			block_transport_server->io_throttle_count += io_cost;
        }
        if(block_transport_server->io_credits_max != 0){ /* Throttling enabled */
			block_transport_server->outstanding_io_credits += io_credits;
        }

        /* Release the queue lock */
        fbe_spinlock_unlock(&block_transport_server->queue_lock);

		if(block_transport_server->attributes & FBE_BLOCK_TRANSPORT_ENABLE_STACK_LIMIT){
			status = fbe_block_transport_server_prepare_packet(block_transport_server, packet, context);
			return status;
		} else {
			status = fbe_block_transport_server_start_packet(block_transport_server, packet, context);
			return FBE_STATUS_PENDING; /* We need to confirm that start packet will never return FBE_STATUS_OK */
		}
    }

    /* Release the queue lock */
    fbe_spinlock_unlock(&block_transport_server->queue_lock);

    return FBE_STATUS_PENDING;
}

/*!**************************************************************
 * @fn fbe_block_transport_server_flush_and_block_io_for_destroy(
 *         fbe_block_transport_server_t * block_transport_server)
 ****************************************************************
 * @brief
 *  This function is called when the LUN is being destroyed.
 *  It is responsible for completing any I/O waiting on the block 
 *  transport queues and ensuring the appropriate flag is set
 *  to block new I/O.
 *
 * @param block_transport_server - the server for the I/O
 *
 * @return fbe_status_t
 *
 ****************************************************************/
fbe_status_t
fbe_block_transport_server_flush_and_block_io_for_destroy(fbe_block_transport_server_t* block_transport_server)
{
    fbe_packet_t*   packet      = NULL;
    fbe_bool_t      done        = FALSE;
    fbe_packet_priority_t   priority;
    fbe_u32_t               index;


    /* Get the queue lock */
    fbe_spinlock_lock(&block_transport_server->queue_lock);

    /* Mark the block transport server as flushing for LUN destroy; this is a flush and block operation
     * in that new I/O to the LUN is blocked and I/O waiting on the queues is flushed.  I/O in progress
     * completes normally.
     */
    fbe_block_transport_server_flush_and_block(block_transport_server);

    /* Complete any outstanding I/O waiting on the block transport server queues */
    while (!done)
    {
        packet = NULL;
        
        if ((block_transport_server->outstanding_io_count & FBE_BLOCK_TRANSPORT_SERVER_GATE_MASK) > 0) 
        {
            for(priority = FBE_PACKET_PRIORITY_LAST - 1; priority > FBE_PACKET_PRIORITY_INVALID; priority--)
            {
                index = priority - 1;
                if(! fbe_queue_is_empty(&block_transport_server->queue_head[index])) {
                    packet = fbe_transport_dequeue_packet(&block_transport_server->queue_head[index]);
                    break;
                }
            }

            if(packet == NULL) { /* All queues are empty */
                done = TRUE;
            }
        } 
        else
        {
            done = TRUE;
        }

        if (packet != NULL)
        {
            block_transport_server_decrement_outstanding_io_count(block_transport_server, packet);
            /* Complete the packet with the appropriate error */
            fbe_transport_set_status(packet, FBE_STATUS_DEAD, 0);
            fbe_transport_complete_packet(packet);
        }

    } /* while (!done) */

    /* Release the queue lock */
    fbe_spinlock_unlock(&block_transport_server->queue_lock);

    return FBE_STATUS_PENDING;
}

static fbe_status_t
block_transport_server_enqueue_packet(fbe_block_transport_server_t * block_transport_server,
                                         fbe_packet_t * packet,
                                         fbe_transport_entry_context_t context)
{
    fbe_packet_priority_t packet_priority;
    fbe_u32_t index;

    fbe_transport_get_packet_priority(packet, &packet_priority);

    packet->queue_context = context;

    if((packet_priority <= FBE_PACKET_PRIORITY_INVALID) || (packet_priority >= FBE_PACKET_PRIORITY_LAST))
    {
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR, 
                                 FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                 "%s Error invalid packet_priority 0x%X", __FUNCTION__, packet_priority);
        return(FBE_STATUS_GENERIC_FAILURE);
    }
    else
    {
        index = packet_priority - 1;
        fbe_transport_enqueue_packet(packet, &block_transport_server->queue_head[index]);
    }

    return FBE_STATUS_OK;
}
static fbe_status_t 
block_transport_server_retry_completion_with_offset(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_block_operation_t *block_operation_current = NULL;
    fbe_payload_block_operation_t *block_operation_orig = NULL;
    fbe_block_edge_t *block_edge = NULL;

    payload = fbe_transport_get_payload_ex(packet);
    if ((payload->current_operation != NULL) &&
        (payload->current_operation->payload_opcode == FBE_PAYLOAD_OPCODE_BLOCK_OPERATION))
    {
        block_operation_current = fbe_payload_ex_get_block_operation(payload);
        if (block_operation_current != NULL)
        {
            fbe_payload_block_get_block_edge(block_operation_current, &block_edge);
        }
        fbe_payload_ex_release_block_operation(payload, block_operation_current);
        block_operation_orig = fbe_payload_ex_get_block_operation(payload);
    }

    /* If we have an edge offset, then unwind the block op
     * and if needed apply the offset to any status values in the block op.
     *  
     * Check the edge type, since there are times when a packet gets sent over other edge 
     * types and the transport ID changes (e.g. ssp) and on completion we cannot do the
     * offset handling if the edge is not a block transport edge. 
     */
    if ((block_edge != NULL) && 
        (block_edge->base_edge.transport_id == FBE_TRANSPORT_ID_BLOCK) &&
        (block_edge->offset > 0))
    {
        /* Copy status values to the original block payload.
         * Also apply the offset to the media error lba. 
         */
        if (block_operation_orig != NULL)
        {
            block_operation_orig->status = block_operation_current->status;
            block_operation_orig->status_qualifier = block_operation_current->status_qualifier;
            if (block_operation_orig->pre_read_descriptor != NULL)
            {
                /* Restore this since we incremented it on the way down.
                 */
                block_operation_orig->pre_read_descriptor->lba -= block_edge->offset;
            }
        }
        payload->media_error_lba -= block_edge->offset;
    }

    /* Call the standard retry completion function to finish up.
     */
    return block_transport_server_retry_completion(packet, context);
    
}
static fbe_status_t 
block_transport_server_bouncer_completion_with_offset(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_block_transport_server_t * block_transport_server = (fbe_block_transport_server_t *)context;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_block_operation_t *block_operation_current = NULL;
    fbe_payload_block_operation_t *block_operation_orig = NULL;
    fbe_block_edge_t *block_edge = NULL;

    payload = fbe_transport_get_payload_ex(packet);
    if ((payload->current_operation != NULL) &&
        (payload->current_operation->payload_opcode == FBE_PAYLOAD_OPCODE_BLOCK_OPERATION)){
        block_operation_current = fbe_payload_ex_get_block_operation(payload);
        if (block_operation_current != NULL)
        {
            fbe_payload_block_get_block_edge(block_operation_current, &block_edge);
        }
        fbe_payload_ex_release_block_operation(payload, block_operation_current);
        block_operation_orig = fbe_payload_ex_get_block_operation(payload);

        if (block_transport_server->attributes & FBE_BLOCK_TRANSPORT_FLAGS_EVENT_ON_SW_ERROR)
        {
            fbe_payload_block_operation_status_t block_status;
            fbe_payload_ex_t *payload_p = fbe_transport_get_payload_ex(packet);
            fbe_payload_block_operation_t *block_operation_p = fbe_payload_ex_get_block_operation(payload_p);
            fbe_payload_block_get_status(block_operation_p, &block_status);

            if (block_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST)
            {
                /* We are resending this block op and need to reuse the resource priority.
                 */
                packet->resource_priority_boost = 0;
                fbe_transport_set_completion_function(packet, 
                                                      block_transport_server_retry_completion_with_offset, 
                                                      block_transport_server);
                block_transport_server->block_transport_const->block_transport_entry(block_transport_server->event_context, packet);
                return FBE_STATUS_MORE_PROCESSING_REQUIRED;
            }
        }
    }
    
    /* If we have an edge offset, then unwind the block op
     * and if needed apply the offset to any status values in the block op.
     *  
     * Check the edge type, since there are times when a packet gets sent over other edge 
     * types and the transport ID changes (e.g. ssp) and on completion we cannot do the
     * offset handling if the edge is not a block transport edge. 
     */
    if ((block_edge != NULL) && 
        (block_edge->base_edge.transport_id == FBE_TRANSPORT_ID_BLOCK) &&
        (block_edge->offset > 0))
    {
        /* Copy status values to the original block payload.
         * Also apply the offset to the media error lba. 
         */
        if(block_operation_orig != NULL)
        {
            block_operation_orig->status = block_operation_current->status;
            block_operation_orig->status_qualifier = block_operation_current->status_qualifier;
            //block_operation_orig->media_error_lba = block_operation_current->media_error_lba;
            //block_operation_orig->retry_wait_msecs = block_operation_current->retry_wait_msecs;
            //block_operation_orig->pdo_object_id = block_operation_current->pdo_object_id;
            if (block_operation_orig->pre_read_descriptor != NULL)
            {
                /* Restore this since we incremented it on the way down.
                 */
                block_operation_orig->pre_read_descriptor->lba -= block_edge->offset;
            }
        }
        payload->media_error_lba -= block_edge->offset;
    }

    /* Call the standard completion function to finish up.
     */
    return block_transport_server_bouncer_completion(packet, context);
    
}

/* This function should be called under the lock */
static fbe_status_t
block_transport_server_restart_io(fbe_block_transport_server_t * block_transport_server, fbe_queue_head_t * tmp_queue)
{
    fbe_packet_t * new_packet = NULL;
    fbe_bool_t done = FALSE;
    fbe_packet_priority_t priority;
    fbe_u32_t index;
	fbe_queue_element_t * queue_element;
	fbe_payload_block_operation_t *block_operation = NULL;

	if(block_transport_server->outstanding_io_max == 0){
        for(priority = FBE_PACKET_PRIORITY_LAST - 1; priority > FBE_PACKET_PRIORITY_INVALID; priority--)
        {
            index = priority - 1;
            while(!fbe_queue_is_empty(&block_transport_server->queue_head[index])) {
               new_packet = fbe_transport_dequeue_packet(&block_transport_server->queue_head[index]);
	           block_transport_server_increment_outstanding_io_count(block_transport_server, new_packet);

		        if(block_transport_server->attributes & FBE_BLOCK_TRANSPORT_FLAGS_TAGS_ENABLED){
			        block_transport_server_allocate_tag(block_transport_server, new_packet);
				}
				fbe_queue_push(tmp_queue, &new_packet->queue_element);
            }
        }
		return FBE_STATUS_OK;  
	}

    while(!done){
        new_packet = NULL;        

        if(((fbe_u32_t)(block_transport_server->outstanding_io_count & FBE_BLOCK_TRANSPORT_SERVER_GATE_MASK) < block_transport_server->outstanding_io_max)
				|| (block_transport_server->outstanding_io_max == 0))
		{ /* We can send more I/O's! A joyous occasion! */

            /* Find a guaranteed I/O in priority order */
            for(priority = FBE_PACKET_PRIORITY_LAST - 1; priority > FBE_PACKET_PRIORITY_INVALID; priority--) {
                index = priority - 1;
                if( (block_transport_server->io_credits_per_queue[index] > 0) &&
                   (!fbe_queue_is_empty(&block_transport_server->queue_head[index]))) {
					   queue_element = fbe_queue_front(&block_transport_server->queue_head[index]);
					   new_packet = fbe_transport_queue_element_to_packet(queue_element);
                    //new_packet = fbe_transport_dequeue_packet(&block_transport_server->queue_head[index]);
                    break;
                }
            }

            /* If we haven't found a guaranteed I/O to send, take the highest priority packet */
            if(new_packet == NULL) {
                for(priority = FBE_PACKET_PRIORITY_LAST - 1; priority > FBE_PACKET_PRIORITY_INVALID; priority--) {
                    index = priority - 1;
                    if(!fbe_queue_is_empty(&block_transport_server->queue_head[index])) {
					   queue_element = fbe_queue_front(&block_transport_server->queue_head[index]);
					   new_packet = fbe_transport_queue_element_to_packet(queue_element);

                        //new_packet = fbe_transport_dequeue_packet(&block_transport_server->queue_head[index]);
                        break;
                    }
                }
            }
		}

        if(new_packet != NULL) {

			if(block_transport_server->io_throttle_max != 0){
                fbe_block_count_t io_cost;
				block_operation = fbe_transport_get_block_operation(new_packet);
                if (block_transport_server->block_transport_const->throttle_calc_fn) {
                    io_cost = (block_transport_server->block_transport_const->throttle_calc_fn)((struct fbe_base_object_s *)
                                                                                                block_transport_server->event_context,
                                                                                                block_operation);
                }
                else {
                    io_cost = block_operation->block_count;
                }
                /* Make sure the cost is no more than the max throttle. 
                 * Otherwise this I/O will never be processed.  This can happen with zeros.
                 */
                io_cost = FBE_MIN(io_cost, block_transport_server->io_throttle_max - 1);

				if(block_transport_server->io_throttle_count + io_cost > block_transport_server->io_throttle_max){
					/* We exceeded the throttle threshold */
					return FBE_STATUS_OK;
				}
                fbe_payload_block_set_throttle_count(block_operation, (fbe_u32_t)io_cost);
				block_transport_server->io_throttle_count += io_cost;
                fbe_base_transport_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO,
                                         "context: %p packet: %p inc1 0x%x lba: %llx bl: %llx tc: %llx ic: %lld\n", 
                                         block_transport_server->event_context,
                                         new_packet, block_operation->throttle_count, block_operation->lba, block_operation->block_count,
                                         block_transport_server->io_throttle_count, block_transport_server->outstanding_io_count);
			}

            if (block_transport_server->io_credits_max != 0) {    /* Throttling enabled */
                fbe_u32_t io_credits;
                block_operation = fbe_transport_get_block_operation(new_packet);

                if (block_transport_server->block_transport_const->calc_io_credits_fn){
                    io_credits = (block_transport_server->block_transport_const->calc_io_credits_fn)((struct fbe_base_object_s *)
                                                                                                   block_transport_server->event_context,
                                                                                                   block_operation,
                                                                                                   new_packet, NULL);
                }
                else {
                    io_credits = 1;
                }

                /* Make sure the cost is no more than the max credits. 
                 * Otherwise this I/O will never be processed.
                 */
                io_credits = FBE_MIN(io_credits, block_transport_server->io_credits_max - 1);
                fbe_payload_block_set_io_credits(block_operation, io_credits);

                fbe_base_transport_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO,
                                         "context: %p packet: %p inc1 %d lba: %llx bl: %llx iw: %d ic: %lld\n", 
                                         block_transport_server->event_context,
                                         new_packet, block_operation->io_credits, block_operation->lba, block_operation->block_count,
                                         block_transport_server->outstanding_io_credits, block_transport_server->outstanding_io_count);

                if ((io_credits != 0) &&
                    (block_transport_server->outstanding_io_credits >= block_transport_server->io_credits_max)){
                    /* Can't start this I/O we are congested. */
                    return FBE_STATUS_OK;
                }
                block_transport_server->outstanding_io_credits += io_credits;
            }

			if(block_transport_server->outstanding_io_count_per_queue[new_packet->packet_priority - 1] >= 
				block_transport_server_get_io_max(block_transport_server, new_packet->packet_priority)) {

				/* Can't start this I/O  */
				return FBE_STATUS_OK;
			}

			/* We are under the spin lock, so it should be the same packet */
			new_packet = fbe_transport_dequeue_packet(&block_transport_server->queue_head[index]);

            block_transport_server_increment_outstanding_io_count(block_transport_server, new_packet);

            if(block_transport_server->attributes & FBE_BLOCK_TRANSPORT_FLAGS_TAGS_ENABLED){
                block_transport_server_allocate_tag(block_transport_server, new_packet);
            }

            fbe_queue_push(tmp_queue, &new_packet->queue_element);
            //fbe_block_transport_server_start_packet(block_transport_server, new_packet, new_packet->queue_context);
        } else {
			done = TRUE;
		}
    }/* while(!done) */


    return FBE_STATUS_OK;  
}

static fbe_status_t 
block_transport_server_retry_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_block_transport_server_t * block_transport_server = NULL;
    fbe_queue_head_t tmp_queue;
	fbe_atomic_t io_gate = 0;

    block_transport_server = (fbe_block_transport_server_t *)context;

    /* Our retry is done.  If we still have an error, tell the object.
     */
    if(block_transport_server->attributes & FBE_BLOCK_TRANSPORT_FLAGS_EVENT_ON_SW_ERROR){
        fbe_payload_block_operation_status_t block_status;
        fbe_payload_ex_t *payload_p = fbe_transport_get_payload_ex(packet);
        fbe_payload_block_operation_t *block_operation_p = fbe_payload_ex_get_block_operation(payload_p);

        fbe_payload_block_get_status(block_operation_p, &block_status);

        if (block_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST) {
            block_transport_server->block_transport_const->block_transport_event_entry(FBE_BLOCK_TRASPORT_EVENT_TYPE_UNEXPECTED_ERROR,
                                                                                       block_transport_server->event_context);
        }
    }

	if(block_transport_server->outstanding_io_max == 0){ /* There are no priority queue's to check */
		io_gate = fbe_atomic_decrement(&block_transport_server->outstanding_io_count);
		return FBE_STATUS_OK;
	}

    fbe_queue_init(&tmp_queue);

    fbe_spinlock_lock(&block_transport_server->queue_lock);

    /* release the i/o tag if required. */
    if(block_transport_server->attributes & FBE_BLOCK_TRANSPORT_FLAGS_TAGS_ENABLED){
        block_transport_server_release_tag(block_transport_server, packet);
    }
    
    /* Decrement the outstanding i/o count if hold attribute is set before returning
     * more processing required.
     */

    block_transport_server_decrement_outstanding_io_count(block_transport_server, packet);

    /* We have to check it under the lock */
    if(!(block_transport_server->attributes & FBE_BLOCK_TRANSPORT_ENABLE_FORCE_COMPLETION) && 
        !(block_transport_server->attributes & FBE_BLOCK_TRANSPORT_FLAGS_HOLD)){

        block_transport_server_restart_io(block_transport_server, &tmp_queue);
    }

    fbe_spinlock_unlock(&block_transport_server->queue_lock);

    /* Put all the packets from tmp_queue to transport run queue */
    if(!fbe_queue_is_empty(&tmp_queue)) {
        fbe_transport_run_queue_push(&tmp_queue, block_transport_server_run_queue_completion, block_transport_server);
    }

    fbe_queue_destroy(&tmp_queue);

    return FBE_STATUS_OK;
}

static fbe_status_t 
block_transport_server_bouncer_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_block_transport_server_t * block_transport_server = NULL;
    fbe_queue_head_t tmp_queue;
	fbe_atomic_t io_gate = 0;
    fbe_payload_ex_t * payload_p;
   fbe_payload_block_operation_t * block_operation_p;


    block_transport_server = (fbe_block_transport_server_t *)context;

    if(block_transport_server->attributes & FBE_BLOCK_TRANSPORT_FLAGS_EVENT_ON_SW_ERROR){
        fbe_payload_block_operation_status_t block_status;
        payload_p = fbe_transport_get_payload_ex(packet);
        block_operation_p = fbe_payload_ex_get_block_operation(payload_p);
        fbe_payload_block_get_status(block_operation_p, &block_status);

        if (block_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST){

            packet->resource_priority_boost = 0;
            fbe_transport_set_completion_function(packet, 
                                                  block_transport_server_retry_completion, 
                                                  block_transport_server);
            block_transport_server->block_transport_const->block_transport_entry(block_transport_server->event_context, packet);
            return FBE_STATUS_MORE_PROCESSING_REQUIRED;
        }
    }

	if(block_transport_server->outstanding_io_max == 0){ /* There are no priority queue's to check */
		io_gate = fbe_atomic_decrement(&block_transport_server->outstanding_io_count);
		return FBE_STATUS_OK;
	}

    fbe_queue_init(&tmp_queue);

    fbe_spinlock_lock(&block_transport_server->queue_lock);

    /* release the i/o tag if required. */
    if(block_transport_server->attributes & FBE_BLOCK_TRANSPORT_FLAGS_TAGS_ENABLED){
        block_transport_server_release_tag(block_transport_server, packet);
    }
    
	if(block_transport_server->io_throttle_max != 0){
        payload_p = fbe_transport_get_payload_ex(packet);
        block_operation_p = fbe_payload_ex_get_block_operation(payload_p);

        if (block_operation_p->throttle_count > block_transport_server->io_throttle_count){
            fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR, 
                                     FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                     "%s block op throttle count %d > bts %lld\n", 
                                     __FUNCTION__, block_operation_p->throttle_count, block_transport_server->io_throttle_count);
        }
        
		block_transport_server->io_throttle_count -= block_operation_p->throttle_count;

        fbe_base_transport_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO,
                                 "context: %p packet: %p dec 0x%x lba: %llx bl: %llx tc: %llx ic: %lld\n", 
                                 block_transport_server->event_context,
                                 packet, block_operation_p->throttle_count, block_operation_p->lba, block_operation_p->block_count,
                                 block_transport_server->io_throttle_count, block_transport_server->outstanding_io_count);
	}
    if(block_transport_server->io_credits_max != 0){
        payload_p = fbe_transport_get_payload_ex(packet);
        block_operation_p = fbe_payload_ex_get_block_operation(payload_p);

        if (block_operation_p->io_credits > block_transport_server->outstanding_io_credits){
            fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR, 
                                     FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                     "%s block op io_credits_max %d > bts %d\n", 
                                     __FUNCTION__, block_operation_p->io_credits, block_transport_server->outstanding_io_credits);
        }
        
		block_transport_server->outstanding_io_credits -= block_operation_p->io_credits;
        /* If we are still congested, and the caller wants us to fail this request 
         * on congested, return back a qualifier to let them know we are still congested. 
         */
        if ((block_transport_server->outstanding_io_credits >= block_transport_server->io_credits_max) &&
            (block_operation_p->block_flags & FBE_PAYLOAD_BLOCK_OPERATION_FLAGS_ALLOW_FAIL_CONGESTION)) {

            if ((block_operation_p->status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS) &&
                (block_operation_p->status_qualifier == FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE)){

                fbe_payload_block_set_status(block_operation_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS,
                                             FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_STILL_CONGESTED);
            }
        }
        fbe_base_transport_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO,
                                 "context: %p packet: %p dec %d lba: %llx bl: %llx iw: %d ic: %lld\n", 
                                 block_transport_server->event_context,
                                 packet, block_operation_p->io_credits, block_operation_p->lba, block_operation_p->block_count,
                                 block_transport_server->outstanding_io_credits, block_transport_server->outstanding_io_count);
	}

    /* Decrement the outstanding i/o count if hold attribute is set before returning
     * more processing required.
     */

    block_transport_server_decrement_outstanding_io_count(block_transport_server, packet);

    /* We have to check it under the lock */
    if(!(block_transport_server->attributes & FBE_BLOCK_TRANSPORT_ENABLE_FORCE_COMPLETION) && 
        !(block_transport_server->attributes & FBE_BLOCK_TRANSPORT_FLAGS_HOLD)){

        block_transport_server_restart_io(block_transport_server, &tmp_queue);
    }

    fbe_spinlock_unlock(&block_transport_server->queue_lock);

    /* Put all the packets from tmp_queue to transport run queue */
    if(!fbe_queue_is_empty(&tmp_queue)) {
        fbe_transport_run_queue_push(&tmp_queue, block_transport_server_run_queue_completion, block_transport_server);
    }

    fbe_queue_destroy(&tmp_queue);

    return FBE_STATUS_OK;
}

static fbe_status_t 
block_transport_server_metadata_io_bouncer_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_block_transport_server_t * block_transport_server = NULL;
    //fbe_packet_priority_t priority;
    fbe_queue_head_t tmp_queue;
	fbe_atomic_t io_gate = 0;


    block_transport_server = (fbe_block_transport_server_t *)context;
    //fbe_transport_get_packet_priority(packet, &priority);
//#if 0
	if(block_transport_server->outstanding_io_max == 0){ /* There are no priority queue's to check */
		io_gate = fbe_atomic_decrement(&block_transport_server->outstanding_io_count);
		return FBE_STATUS_OK;
	}
//#endif

    fbe_queue_init(&tmp_queue);

    fbe_spinlock_lock(&block_transport_server->queue_lock);

    /* release the i/o tag if required. */
    if(block_transport_server->attributes & FBE_BLOCK_TRANSPORT_FLAGS_TAGS_ENABLED){
        block_transport_server_release_tag(block_transport_server, packet);
    }

    /* We have to check it under the lock */
    if(!(block_transport_server->attributes & FBE_BLOCK_TRANSPORT_ENABLE_FORCE_COMPLETION) && 
        !(block_transport_server->attributes & FBE_BLOCK_TRANSPORT_FLAGS_HOLD)){

        block_transport_server_restart_io(block_transport_server, &tmp_queue);
    }

    fbe_spinlock_unlock(&block_transport_server->queue_lock);

    /* Put all the packets from tmp_queue to transport run queue */
    if(!fbe_queue_is_empty(&tmp_queue)) {
        fbe_transport_run_queue_push(&tmp_queue, block_transport_server_run_queue_completion, block_transport_server);
    }

    fbe_queue_destroy(&tmp_queue);

    return FBE_STATUS_OK;
}

fbe_lifecycle_status_t
fbe_block_transport_server_drain_all_queues(fbe_block_transport_server_t * block_transport_server,
                                            fbe_lifecycle_const_t * p_class_const,
                                            struct fbe_base_object_s * base_object)
{
    fbe_lifecycle_status_t lifecycle_status;
    fbe_status_t completion_status;
    fbe_lifecycle_state_t lifecycle_state;
    fbe_status_t status;
    fbe_packet_priority_t   priority;
    fbe_u32_t               index;

    fbe_block_transport_server_update_path_state(block_transport_server,
                                                 p_class_const,
                                                 base_object,
                                                 FBE_BLOCK_PATH_ATTR_FLAGS_NONE);

    status = fbe_lifecycle_get_state(p_class_const, base_object, &lifecycle_state);
    if(status != FBE_STATUS_OK){
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR, 
                                 FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                 "%s Critical error fbe_lifecycle_get_state failed, status = %X", __FUNCTION__, status);
        /* We have no error code to return */
    }

    completion_status = FBE_STATUS_BUSY;
    fbe_spinlock_lock(&block_transport_server->queue_lock);
    switch(lifecycle_state){
        case FBE_LIFECYCLE_STATE_PENDING_READY:
            fbe_block_transport_server_disable_force_completion(block_transport_server);
            break;
        case FBE_LIFECYCLE_STATE_PENDING_ACTIVATE:
            fbe_block_transport_server_enable_force_completion(block_transport_server);
            completion_status = FBE_STATUS_BUSY;
            break;
        case FBE_LIFECYCLE_STATE_PENDING_HIBERNATE:
            fbe_block_transport_server_disable_force_completion(block_transport_server);
            completion_status = FBE_STATUS_SLUMBER;
            break;
        case FBE_LIFECYCLE_STATE_PENDING_OFFLINE:
            fbe_block_transport_server_enable_force_completion(block_transport_server);
            break;
        case FBE_LIFECYCLE_STATE_PENDING_FAIL:
            fbe_block_transport_server_enable_force_completion(block_transport_server);
            completion_status = FBE_STATUS_FAILED;
            break;
        case FBE_LIFECYCLE_STATE_PENDING_DESTROY:
            fbe_block_transport_server_enable_force_completion(block_transport_server);
            completion_status = FBE_STATUS_DEAD;
            break;
    }

    block_transport_server->force_completion_status = completion_status;

    if((fbe_u32_t)(block_transport_server->outstanding_io_count & FBE_BLOCK_TRANSPORT_SERVER_GATE_MASK) > 0){
        lifecycle_status = FBE_LIFECYCLE_STATUS_PENDING;
    } else {
        lifecycle_status = FBE_LIFECYCLE_STATUS_DONE;
    }

    fbe_spinlock_unlock(&block_transport_server->queue_lock);

    /* Unconditionally drain our queues. 
     * If we had a metadata I/O queued, then it might not drain by using 
     * block_transport_server_drain_io() since our outstanding I/O count will not be zero. 
     * The packet that initiated the metadata I/O will not complete (and our outstanding I/O count will not be zero) 
     * until the metadata I/O completes. 
     */
    /* we may want to only drain os_queue */       

    for(priority = FBE_PACKET_PRIORITY_LAST - 1; priority > FBE_PACKET_PRIORITY_INVALID; priority--)
    {
        index = priority - 1;
        fbe_block_transport_drain_queue(block_transport_server, &block_transport_server->queue_head[index], completion_status);
    }

    return lifecycle_status;
}

static fbe_lifecycle_status_t
block_transport_server_drain_io(fbe_block_transport_server_t * block_transport_server, fbe_lifecycle_state_t lifecycle_state)
{
    fbe_lifecycle_status_t lifecycle_status;
    fbe_status_t completion_status;
    fbe_packet_priority_t   priority;
    fbe_u32_t               index;

    completion_status = FBE_STATUS_BUSY;
    fbe_spinlock_lock(&block_transport_server->queue_lock);
    switch(lifecycle_state){
        case FBE_LIFECYCLE_STATE_PENDING_READY:
            fbe_block_transport_server_disable_force_completion(block_transport_server);
            break;
        case FBE_LIFECYCLE_STATE_PENDING_ACTIVATE:
            fbe_block_transport_server_enable_force_completion(block_transport_server);
            completion_status = FBE_STATUS_BUSY;
            break;
        case FBE_LIFECYCLE_STATE_PENDING_HIBERNATE:
            fbe_block_transport_server_disable_force_completion(block_transport_server);
            completion_status = FBE_STATUS_SLUMBER;
            break;
        case FBE_LIFECYCLE_STATE_PENDING_OFFLINE:
            fbe_block_transport_server_enable_force_completion(block_transport_server);
            break;
        case FBE_LIFECYCLE_STATE_PENDING_FAIL:
            fbe_block_transport_server_enable_force_completion(block_transport_server);
            completion_status = FBE_STATUS_FAILED;
            break;
        case FBE_LIFECYCLE_STATE_PENDING_DESTROY:
            fbe_block_transport_server_enable_force_completion(block_transport_server);
            completion_status = FBE_STATUS_DEAD;
            break;
    }

    block_transport_server->force_completion_status = completion_status;

    if((fbe_u32_t)(block_transport_server->outstanding_io_count & FBE_BLOCK_TRANSPORT_SERVER_GATE_MASK) > 0){
        lifecycle_status = FBE_LIFECYCLE_STATUS_PENDING;
    } else {
        lifecycle_status = FBE_LIFECYCLE_STATUS_DONE;
    }

    fbe_spinlock_unlock(&block_transport_server->queue_lock);

    for(priority = FBE_PACKET_PRIORITY_LAST - 1; priority > FBE_PACKET_PRIORITY_INVALID; priority--)
    {
        index = priority - 1;
        fbe_block_transport_drain_queue(block_transport_server, &block_transport_server->queue_head[index], completion_status);
    }

    return lifecycle_status;
}

/*!**************************************************************
 * @fn fbe_block_transport_server_process_canceled_packets(
 *         fbe_block_transport_server_t * block_transport_server)
 ****************************************************************
 * @brief
 *  This function completes all canceled packets from the block transport queues.
 *  
 *  This function should be called in monitor context only. (FBE_BASE_OBJECT_LIFECYCLE_COND_PACKET_CANCELED)
 *
 * @param block_transport_server - The block transport server.
 *
 * @return fbe_status_t   
 *
 ****************************************************************/
fbe_status_t 
fbe_block_transport_server_process_canceled_packets(fbe_block_transport_server_t * block_transport_server)
{
    fbe_packet_priority_t   priority;
    fbe_u32_t               index;

    for(priority = FBE_PACKET_PRIORITY_LAST - 1; priority > FBE_PACKET_PRIORITY_INVALID; priority--)
    {
        index = priority - 1;
        block_transport_server_complete_canceled_packets(&block_transport_server->queue_head[index], &block_transport_server->queue_lock);
    }

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn block_transport_server_complete_canceled_packets(
 *                                                      fbe_queue_head_t * queue_head,
 *                                                      fbe_spinlock_t *  queue_lock)
 ****************************************************************
 * @brief
 *  This function completes all canceled packets from the specific queue.
 *  
 *
 * @param queue_head - The head of the packet queue.
 * @param queue_lock - The spinlock to protect the queue
 *
 * @return fbe_status_t   
 *
 ****************************************************************/
static fbe_status_t
block_transport_server_complete_canceled_packets(fbe_queue_head_t * queue_head, fbe_spinlock_t *  queue_lock)
{
    fbe_queue_element_t * queue_element = NULL;
    fbe_packet_t * packet = NULL;
    fbe_u32_t i = 0;
    fbe_status_t status;

    fbe_spinlock_lock(queue_lock);
    queue_element = fbe_queue_front(queue_head);
    while(queue_element){
        packet = fbe_transport_queue_element_to_packet(queue_element);
        status  = fbe_transport_get_status_code(packet);
        if(status == FBE_STATUS_CANCEL_PENDING){
            fbe_transport_remove_packet_from_queue(packet);
            fbe_transport_set_status(packet, FBE_STATUS_CANCELED, 0);
            fbe_spinlock_unlock(queue_lock);
            fbe_transport_complete_packet(packet);
            fbe_spinlock_lock(queue_lock);
            queue_element = fbe_queue_front(queue_head);
        } else {
            queue_element = fbe_queue_next(queue_head, queue_element);
        }
        i++;
        if(i > 0x00ffffff){
            fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR, 
                                     FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                     "%s Critical error looped queue\n", __FUNCTION__);
            fbe_spinlock_unlock(queue_lock);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    fbe_spinlock_unlock(queue_lock);

    return FBE_STATUS_OK;
}

static fbe_status_t
block_transport_server_allocate_tag(fbe_block_transport_server_t * block_transport_server, fbe_packet_t * packet)
{
    fbe_u32_t i;

    fbe_transport_set_tag(packet,FBE_TRANSPORT_TAG_INVALID);
    /* Check if the outstanding_io_max is set */
    if(block_transport_server->outstanding_io_max == 0){
            fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s outstanding_io_max is zero\n", __FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Look for the free tag */
    for(i = 0; ((i < block_transport_server->outstanding_io_max) && (i < 8*sizeof(block_transport_server->tag_bitfield))); i++){
        if(!(block_transport_server->tag_bitfield & (0x1 << i))){ /* The bit is not set */
            block_transport_server->tag_bitfield |= (0x1 << i);     /* Set the bit */
            fbe_transport_set_tag(packet,i);
            return FBE_STATUS_OK;;
        }
    }

    fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s Out of tags\n", __FUNCTION__);

    return FBE_STATUS_GENERIC_FAILURE;
}

static fbe_status_t
block_transport_server_release_tag(fbe_block_transport_server_t * block_transport_server, fbe_packet_t * packet)
{
    fbe_u8_t tag;

    fbe_transport_get_tag(packet, &tag);

    if(tag == FBE_TRANSPORT_TAG_INVALID){
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Invalid tag\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Check if this tag was allocated */
    if(!(block_transport_server->tag_bitfield & (0x1 << tag))){ /* The bit is not set */
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s tag %X was not allocated\n", __FUNCTION__, tag);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Clear the bit */
    block_transport_server->tag_bitfield &= ~(0x1 << tag);
    fbe_transport_set_tag(packet, FBE_TRANSPORT_TAG_INVALID);

    return FBE_STATUS_OK;
}


fbe_status_t
fbe_block_transport_server_update_path_state(fbe_block_transport_server_t * block_transport_server,
                                             fbe_lifecycle_const_t * p_class_const,
                                             struct fbe_base_object_s * base_object,
                                             fbe_block_path_attr_flags_t attribute_exception)
{
    fbe_status_t status;
    fbe_lifecycle_state_t lifecycle_state;

    status = fbe_lifecycle_get_state(p_class_const,
                                     base_object,
                                     &lifecycle_state);

    if(status != FBE_STATUS_OK)
    {
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR, 
                                 FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                 "%s Critical error fbe_lifecycle_get_state failed, status = %X", __FUNCTION__, 
                                 status);
        /* We have no error code to return */
    }

    /* Based on current lifecycle state it sends updated path status on different
     * edges connected with upstream objects.
     */
    switch(lifecycle_state){
        case FBE_LIFECYCLE_STATE_PENDING_READY:
        case FBE_LIFECYCLE_STATE_READY:
            fbe_block_transport_server_set_path_state(block_transport_server, FBE_PATH_STATE_ENABLED, attribute_exception);
            break;
        case FBE_LIFECYCLE_STATE_SPECIALIZE:
        case FBE_LIFECYCLE_STATE_PENDING_ACTIVATE:
        case FBE_LIFECYCLE_STATE_ACTIVATE:
            fbe_block_transport_server_set_path_state(block_transport_server, FBE_PATH_STATE_DISABLED, attribute_exception);
            break;
        case FBE_LIFECYCLE_STATE_PENDING_HIBERNATE:
        case FBE_LIFECYCLE_STATE_HIBERNATE:
            fbe_block_transport_server_set_path_state(block_transport_server, FBE_PATH_STATE_SLUMBER, attribute_exception);
            break;
        case FBE_LIFECYCLE_STATE_PENDING_OFFLINE:
        case FBE_LIFECYCLE_STATE_OFFLINE:
            fbe_block_transport_server_set_path_state(block_transport_server, FBE_PATH_STATE_DISABLED, attribute_exception);
            break;
        case FBE_LIFECYCLE_STATE_PENDING_FAIL:
        case FBE_LIFECYCLE_STATE_FAIL:
            fbe_block_transport_server_set_path_state(block_transport_server, FBE_PATH_STATE_BROKEN, attribute_exception);
            break;
        case FBE_LIFECYCLE_STATE_PENDING_DESTROY:
        case FBE_LIFECYCLE_STATE_DESTROY:
            fbe_block_transport_server_set_path_state(block_transport_server, FBE_PATH_STATE_GONE, attribute_exception);
            break;
        default :
            /* KvTrace("%s Critical error Uknown lifecycle_state = %X \n", __FUNCTION__, lifecycle_state); */
            fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR, 
                                     FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                                     "%s Critical error Uknown lifecycle_state = %X \n", 
                                     __FUNCTION__, 
                                     lifecycle_state);

            break;
    }
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_block_transport_edge_get_capacity(fbe_block_edge_t * block_edge, fbe_lba_t * capacity)
{
    fbe_path_state_t path_state;

    *capacity = 0;
    fbe_block_transport_get_path_state(block_edge, &path_state);
    if(path_state == FBE_PATH_STATE_INVALID){
        return FBE_STATUS_GENERIC_FAILURE;
    }
     *capacity = block_edge->capacity;
     return FBE_STATUS_OK;
}


/*!**************************************************************
 * @fn fbe_block_transport_find_first_upstream_edge_index_and_obj_id
 ****************************************************************
 * @brief
 *  This function finds the first edge known to the transport server
 *
 * @param in_block_transport_server_p - Pointer to block transport server
 * @param out_block_edge_index_p     - Pointer to found edge client index
 * @param out_object_id_p            - Pointer to found edge client object id
 *
 * @return fbe_status_t
 *
 ****************************************************************/
fbe_status_t
fbe_block_transport_find_first_upstream_edge_index_and_obj_id
(
    fbe_block_transport_server_t*   in_block_transport_server_p,
    fbe_edge_index_t*               out_block_edge_index_p,
    fbe_object_id_t*                out_object_id_p
)
{
    fbe_block_edge_t*   block_edge_p;

    // Initialize the default output value
    *out_object_id_p = FBE_OBJECT_ID_INVALID;
    *out_block_edge_index_p = 0;

    if (in_block_transport_server_p==NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_block_transport_server_lock(in_block_transport_server_p);

    // the first one on the list.
    block_edge_p = (fbe_block_edge_t*)in_block_transport_server_p->base_transport_server.client_list;
    if (block_edge_p != NULL)
    {
        fbe_block_transport_get_client_id(block_edge_p, out_object_id_p);
        fbe_block_transport_get_client_index(block_edge_p, out_block_edge_index_p);
    }
    fbe_block_transport_server_unlock(in_block_transport_server_p);

    return FBE_STATUS_OK;
}
// End fbe_block_transport_find_first_upstream_edge_index_and_obj_id()

fbe_status_t fbe_block_transport_find_edge_and_obj_id_by_lba(fbe_block_transport_server_t*   in_block_transport_server_p,
                                                              fbe_lba_t                       in_lba,
                                                              fbe_u32_t*                      out_object_id_p,
                                                              fbe_block_edge_t**              out_block_edge_pp)
{
    fbe_block_edge_t*   block_edge_p = NULL;

    *out_object_id_p = FBE_OBJECT_ID_INVALID;

    fbe_block_transport_server_lock(in_block_transport_server_p);

    // Initialize the next edge to be the first one on the list.
    block_edge_p = (fbe_block_edge_t*)in_block_transport_server_p->base_transport_server.client_list;

    while (block_edge_p != NULL)
    {       
        // If the LBA is within this extent then this is the edge to use.
        if (fbe_block_transport_is_lba_in_edge_extent(in_lba, block_edge_p))
        {
            fbe_block_transport_get_client_id(block_edge_p, out_object_id_p);
            break;
        }

        block_edge_p = (fbe_block_edge_t*)block_edge_p->base_edge.next_edge;
    }
    fbe_block_transport_server_unlock(in_block_transport_server_p);

    *out_block_edge_pp = block_edge_p;
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_block_transport_server_set_capacity(fbe_block_transport_server_t *block_transport_server_p, fbe_lba_t capacity)
{
    block_transport_server_p->capacity = capacity;
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_block_transport_server_get_capacity(fbe_block_transport_server_t *block_transport_server_p, fbe_lba_t *capacity_p)
{
    *capacity_p = block_transport_server_p->capacity;
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_block_transport_server_get_path_attr(fbe_block_transport_server_t * block_transport_server,
                                         fbe_edge_index_t  server_index, 
                                         fbe_path_attr_t *path_attr_p)
{
    fbe_base_transport_server_t * base_transport_server = (fbe_base_transport_server_t *)block_transport_server;
    fbe_u32_t i = 0;
    fbe_base_edge_t * base_edge = NULL;

    fbe_base_transport_server_lock(base_transport_server);

    base_edge = base_transport_server->client_list;
    while(base_edge != NULL) {
        if ((base_edge->client_id != FBE_OBJECT_ID_INVALID) &&
            (base_edge->server_index == server_index)){
            *path_attr_p = base_edge->path_attr;

            /* Just return attributes*/
            break;
        }
        i++;
        if(i > FBE_BLOCK_TRANSPORT_SERVER_MAX_CLIENT_COUNT){
            fbe_base_transport_server_unlock(base_transport_server);
            /* KvTrace("%s Critical error looped client list\n", __FUNCTION__); */
            fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR, 
                                     FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                                     "%s Critical error looped client list\n", __FUNCTION__);

            return FBE_STATUS_GENERIC_FAILURE;
        }
        base_edge = base_edge->next_edge;
    }

    fbe_base_transport_server_unlock(base_transport_server);

    if(base_edge == NULL){ /*We did not found the edge */
        /* KvTrace("%s Critical error edge not found index = %d\n", __FUNCTION__, server_index); */
        fbe_base_transport_trace(FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Error edge not found, server_index = %d\n", __FUNCTION__, server_index);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;
}

fbe_status_t
fbe_block_transport_server_set_path_attr(fbe_block_transport_server_t * block_transport_server,
                                             fbe_edge_index_t  server_index, 
                                             fbe_path_attr_t path_attr)
{
    fbe_base_transport_server_t * base_transport_server = (fbe_base_transport_server_t *)block_transport_server;
    fbe_u32_t i = 0;
    fbe_base_edge_t * base_edge = NULL;
    fbe_block_edge_t * block_edge = NULL;

    fbe_base_transport_server_lock(base_transport_server);

    base_edge = base_transport_server->client_list;
    while(base_edge != NULL) {
        if ((base_edge->client_id != FBE_OBJECT_ID_INVALID) &&
            (base_edge->server_index == server_index)){
            base_edge->path_attr |= path_attr;
            /* We should send event here */
            block_edge = (fbe_block_edge_t *)base_edge;
            
            if(block_edge->server_package_id != block_edge->client_package_id){
                fbe_topology_send_event_to_another_package(base_edge->client_id, 
                                                            block_edge->client_package_id,
                                                            FBE_EVENT_TYPE_ATTRIBUTE_CHANGED, 
                                                            base_edge);
            } else {
                fbe_topology_send_event(base_edge->client_id, FBE_EVENT_TYPE_ATTRIBUTE_CHANGED, base_edge);
            }
            break;
        }
        i++;
        if(i > FBE_BLOCK_TRANSPORT_SERVER_MAX_CLIENT_COUNT){
            fbe_base_transport_server_unlock(base_transport_server);
            /* KvTrace("%s Critical error looped client list\n", __FUNCTION__); */
            fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR, 
                                     FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                                     "%s Critical error looped client list\n", __FUNCTION__);

            return FBE_STATUS_GENERIC_FAILURE;
        }
        base_edge = base_edge->next_edge;
    }

    fbe_base_transport_server_unlock(base_transport_server);

    if(base_edge == NULL){ /*We did not found the edge */
        /* KvTrace("%s Critical error edge not found index = %d\n", __FUNCTION__, server_index); */
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Error edge not found, server_index = %d\n", __FUNCTION__, server_index);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * fbe_block_transport_server_set_path_attr_all_servers()
 ****************************************************************
 * @brief
 *  This function sets the path attribute for all edges in this block transport
 *  server.
 *
 * @param block_transport_server - The block transport server.
 * @param path_attr - The attributes to be set.
 *
 * @return fbe_status_t   
 *
 ****************************************************************/
fbe_status_t
fbe_block_transport_server_set_path_attr_all_servers(fbe_block_transport_server_t * block_transport_server,
                                                     fbe_path_attr_t path_attr)
{
    fbe_base_transport_server_t * base_transport_server = (fbe_base_transport_server_t *)block_transport_server;
    fbe_u32_t i = 0;
    fbe_base_edge_t * base_edge = NULL;
    fbe_block_edge_t * block_edge = NULL;

    fbe_base_transport_server_lock(base_transport_server);

    /* Get a RG edge, i.e. an edge to a LUN */
    base_edge = base_transport_server->client_list;
    while(base_edge != NULL) {
        if ((base_edge->client_id != FBE_OBJECT_ID_INVALID) && 
            (path_attr & base_edge->path_attr) != path_attr){

            /* Get the block transport server of the LUN */
            base_edge->path_attr |= path_attr;
            /* We should send event here */
            block_edge = (fbe_block_edge_t *)base_edge;
            
            fbe_base_transport_trace(FBE_TRACE_LEVEL_INFO,
                                     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                     "block_trans_srv_set_path_attr_all_srvs set attributes 0x%x, server_index = %d\n", 
                                     path_attr, 
                                     base_edge->server_index);

            if(block_edge->server_package_id != block_edge->client_package_id){
                fbe_topology_send_event_to_another_package(base_edge->client_id, 
                                                           block_edge->client_package_id,
                                                           FBE_EVENT_TYPE_ATTRIBUTE_CHANGED, 
                                                           base_edge);
            } else {
                fbe_topology_send_event(base_edge->client_id, FBE_EVENT_TYPE_ATTRIBUTE_CHANGED, base_edge);
            }
            
        }
        i++;
        if(i > FBE_BLOCK_TRANSPORT_SERVER_MAX_CLIENT_COUNT){
            fbe_base_transport_server_unlock(base_transport_server);
            fbe_base_transport_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "block_trans_srv_set_path_attr_all_srvs Critical error looped client list\n");

            return FBE_STATUS_GENERIC_FAILURE;
        }
        base_edge = base_edge->next_edge;
    }

    fbe_base_transport_server_unlock(base_transport_server);

	if(base_edge == NULL){  /*We did not found an edge */
		fbe_base_transport_trace(FBE_TRACE_LEVEL_INFO,
								 FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
								 "block_trans_srv_set_path_attr_all_srvs no edge found\n");
    }

    return FBE_STATUS_OK;
}

fbe_status_t
fbe_block_transport_server_set_path_attr_without_event_notification(fbe_block_transport_server_t * block_transport_server,
                                             fbe_object_id_t client_id, 
                                             fbe_path_attr_t path_attr)
{
    fbe_base_transport_server_t * base_transport_server = (fbe_base_transport_server_t *)block_transport_server;
    fbe_u32_t i = 0;
    fbe_base_edge_t * base_edge = NULL;

    fbe_base_transport_server_lock(base_transport_server);

    base_edge = base_transport_server->client_list;
    while(base_edge != NULL) {
        if (base_edge->client_id == client_id){
            base_edge->path_attr |= path_attr;
            break;
        }
        i++;
        if(i > FBE_BLOCK_TRANSPORT_SERVER_MAX_CLIENT_COUNT){
            fbe_base_transport_server_unlock(base_transport_server);
            fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR, 
                                     FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                                     "%s Critical error looped client list\n", __FUNCTION__);

            return FBE_STATUS_GENERIC_FAILURE;
        }
        base_edge = base_edge->next_edge;
    }

    fbe_base_transport_server_unlock(base_transport_server);

    if(base_edge == NULL){ /*We did not found the edge */
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Error edge not found, client id = 0x%x\n", __FUNCTION__, client_id);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;
}

fbe_status_t
fbe_block_transport_server_set_edge_path_state(fbe_block_transport_server_t * block_transport_server,
                                             fbe_edge_index_t  server_index, 
                                             fbe_path_state_t path_state)
{
    fbe_base_transport_server_t * base_transport_server = (fbe_base_transport_server_t *)block_transport_server;
    fbe_u32_t i = 0;
    fbe_base_edge_t * base_edge = NULL;
    fbe_block_edge_t * block_edge = NULL;

    fbe_base_transport_server_lock(base_transport_server);

    base_edge = base_transport_server->client_list;
    while(base_edge != NULL) {
        if ((base_edge->client_id != FBE_OBJECT_ID_INVALID) &&
            (base_edge->server_index == server_index)){
            base_edge->path_state = path_state;

            /* We should send event here */
            block_edge = (fbe_block_edge_t *)base_edge;
            
            if(block_edge->server_package_id != block_edge->client_package_id){
                fbe_topology_send_event_to_another_package(base_edge->client_id, 
                                                            block_edge->client_package_id,
                                                            FBE_EVENT_TYPE_EDGE_STATE_CHANGE, 
                                                            base_edge);
            } else {
                fbe_topology_send_event(base_edge->client_id, FBE_EVENT_TYPE_EDGE_STATE_CHANGE, base_edge);
            }
            break;
        }
        i++;
        if(i > FBE_BLOCK_TRANSPORT_SERVER_MAX_CLIENT_COUNT){
            fbe_base_transport_server_unlock(base_transport_server);
            /* KvTrace("%s Critical error looped client list\n", __FUNCTION__); */
            fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR, 
                                     FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                                     "%s Critical error looped client list\n", __FUNCTION__);

            return FBE_STATUS_GENERIC_FAILURE;
        }
        base_edge = base_edge->next_edge;
    }

    fbe_base_transport_server_unlock(base_transport_server);

    if(base_edge == NULL){ /*We did not found the edge */
        /* KvTrace("%s Critical error edge not found index = %d\n", __FUNCTION__, server_index); */
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Error edge not found, server_index = %d\n", __FUNCTION__, server_index);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;
}

fbe_status_t
fbe_block_transport_server_set_path_state(fbe_block_transport_server_t * block_transport_server, 
                                          fbe_path_state_t path_state,
                                          fbe_block_path_attr_flags_t attribute_exception)
{
    fbe_base_transport_server_t * base_transport_server = (fbe_base_transport_server_t *) block_transport_server;
    fbe_u32_t i = 0;
    fbe_base_edge_t * base_edge = NULL;
    fbe_block_edge_t * block_edge = NULL;

    fbe_base_transport_server_lock(base_transport_server);

    base_edge = base_transport_server->client_list;
    while(base_edge != NULL) {
        /* Only change state for non exception cases.
         */
        if ((base_edge->path_state != path_state) &&
            ((base_edge->path_attr & attribute_exception) == 0) ){
            base_edge->path_state = path_state;
            /* We should send event here */
            block_edge = (fbe_block_edge_t *)base_edge;
            
            if(block_edge->server_package_id != block_edge->client_package_id){
                fbe_topology_send_event_to_another_package(base_edge->client_id, 
                                                            block_edge->client_package_id,
                                                            FBE_EVENT_TYPE_EDGE_STATE_CHANGE, 
                                                            base_edge);
            } else {
                fbe_topology_send_event(base_edge->client_id, FBE_EVENT_TYPE_EDGE_STATE_CHANGE, base_edge);
            }
        }
        i++;
        if(i > FBE_BLOCK_TRANSPORT_SERVER_MAX_CLIENT_COUNT){
            fbe_base_transport_server_unlock(base_transport_server);
            /* KvTrace("%s Critical error edge not found\n", __FUNCTION__); */
            fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR, 
                                     FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                                     "%s Critical error looped client list\n", __FUNCTION__);

            return FBE_STATUS_GENERIC_FAILURE;
        }
        base_edge = base_edge->next_edge;
    }

    fbe_base_transport_server_unlock(base_transport_server);

    return FBE_STATUS_OK;
}

fbe_status_t
fbe_block_transport_server_send_event(fbe_block_transport_server_t * block_transport_server, 
                                      fbe_event_t * event)
{
    fbe_base_transport_server_t* base_transport_server = NULL;
    fbe_u32_t                    index = 0;
    fbe_base_edge_t*             base_edge = NULL;
    fbe_block_edge_t*            block_edge = NULL;
    fbe_event_stack_t*           event_stack = NULL;
    fbe_event_type_t             event_type;
    fbe_lba_t                    lba;
    fbe_lba_t                    offset_lba;
    fbe_block_count_t            block_count;
    fbe_lba_t                    current_offset;
    fbe_lba_t                    previous_offset;
    fbe_lba_t                    block_offset;
    fbe_package_id_t             client_package_id = FBE_PACKAGE_ID_INVALID;
    fbe_package_id_t             server_package_id = FBE_PACKAGE_ID_INVALID;
    fbe_object_id_t              client_id = FBE_OBJECT_ID_INVALID;
    fbe_status_t                 status;

    base_transport_server = (fbe_base_transport_server_t *) block_transport_server;
    fbe_event_get_type(event, &event_type);

    /* not all event types are supported by this function */
    switch(event_type)
    {
        case FBE_EVENT_TYPE_DATA_REQUEST:
        case FBE_EVENT_TYPE_PERMIT_REQUEST:
        case FBE_EVENT_TYPE_SPARING_REQUEST:
        case FBE_EVENT_TYPE_COPY_REQUEST:
        case FBE_EVENT_TYPE_ABORT_COPY_REQUEST:
        case FBE_EVENT_TYPE_VERIFY_REPORT: 
        case FBE_EVENT_TYPE_EVENT_LOG:     
        case FBE_EVENT_TYPE_DOWNLOAD_REQUEST:
            break;
        default:
            fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                     FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                     "%s Invalid event type %d\n", __FUNCTION__, event_type);
            fbe_event_set_status(event, FBE_EVENT_STATUS_GENERIC_FAILURE);
            fbe_event_complete(event);
            return FBE_STATUS_GENERIC_FAILURE;
    };

    /* get the current event stack details. */
    event_stack = fbe_event_get_current_stack(event);
    fbe_event_stack_get_extent(event_stack, &lba, &block_count);
    fbe_event_stack_get_current_offset(event_stack, &current_offset);
    fbe_event_stack_get_previous_offset(event_stack, &previous_offset);

    fbe_base_transport_server_lock(base_transport_server);

    base_edge = base_transport_server->client_list;

    while (base_edge != NULL)
    {
        /* we should send event here */
        block_edge = (fbe_block_edge_t *)base_edge;

        block_offset = block_edge->offset;
        if (block_edge->base_edge.path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_IGNORE_OFFSET)
        {
            block_offset = 0;
        }

        /* check the lba range and determine whether it belongs to any of the edge. */
        if ((block_edge->base_edge.client_id != FBE_OBJECT_ID_INVALID) &&
            (fbe_block_transport_is_lba_range_overlaps_edge_extent(lba, block_count, block_edge)))
        {
            /* get everything we'll need before we releasing the lock */
            client_package_id = block_edge->client_package_id;
            server_package_id = block_edge->server_package_id;
            client_id = base_edge->client_id;


            /* save the client id for in fly event */
            fbe_event_service_in_fly_event_save_object_id(client_id);
            
            /* get the next lba for the event. */
            if (lba > block_offset)
            {
                offset_lba = lba - block_offset;
            }
            else
            {
                offset_lba = 0;
            }

            /* recalculate previous and current offset before sending event. */
            previous_offset = block_offset;
            current_offset  = block_offset + block_edge->capacity;

            /* block count needs to be adjusted based on which edge we are sending an event */
            if (block_count > block_edge->capacity)
            {
                block_count = (fbe_block_count_t) (block_edge->capacity - offset_lba);
            }

            /* get the current edge checkpoint */
            fbe_event_set_edge(event, base_edge);

            /* One edge support for now. We release the lock here so the receiver
             * can do block transport server locks if he needs on this context    */
            fbe_base_transport_server_unlock(base_transport_server);

            /* update the current offset and previous offsets while going through each edge */
            fbe_event_stack_set_previous_offset(event_stack, previous_offset);
            fbe_event_stack_set_current_offset(event_stack, current_offset);

            /* set new offset lba and block count before sending event to upstream edge */
            fbe_event_stack_set_extent(event_stack, offset_lba, block_count);

            if (server_package_id != client_package_id)
            {
                fbe_topology_send_event_to_another_package(client_id, client_package_id, event_type, event);
            }
            else
            {
                status = fbe_topology_send_event(client_id, event_type, event);

                if (status != FBE_STATUS_OK)
                {
                    fbe_base_transport_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                             "topology send event failed with status %d. Complete event.\n", status);
                    fbe_event_set_flags(event, FBE_EVENT_FLAG_DENY);
                    fbe_event_set_status(event,FBE_EVENT_STATUS_BUSY);
                    fbe_event_complete(event);
                }
            }
            /* clear the client id for in fly event */
            fbe_event_service_in_fly_event_clear_object_id();
            
            return FBE_STATUS_OK;
        }

        index++;

        if (index > FBE_BLOCK_TRANSPORT_SERVER_MAX_CLIENT_COUNT)
        {
            fbe_base_transport_server_unlock(base_transport_server);

            /* KvTrace("%s Critical error edge not found\n", __FUNCTION__); */
            fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                     FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                     "%s Critical error looped client list\n", __FUNCTION__);

            return FBE_STATUS_GENERIC_FAILURE;
        }

        base_edge = base_edge->next_edge;
    }

    fbe_base_transport_server_unlock(base_transport_server);
    
    if (current_offset == previous_offset)
    {
        /* current offset is lba - no upstream edge has our range of lba so return no user data status. */
        fbe_event_set_status(event, FBE_EVENT_STATUS_NO_USER_DATA);
    }
    else
    {
        /* if current offset is not zero then return good status. */
        fbe_event_set_status(event, FBE_EVENT_STATUS_OK);
    }

    /* we did not find any further edge to send event, so update the current offset with last lba of the range. */
    fbe_event_complete(event);

    return FBE_STATUS_OK;
}

fbe_status_t
fbe_block_transport_server_is_lba_range_consumed(fbe_block_transport_server_t * block_transport_server,
                                                 fbe_lba_t lba,
                                                 fbe_block_count_t block_count,
                                                 fbe_bool_t * is_consumed_range_p)
{
    fbe_base_transport_server_t *   base_transport_server = NULL;
    fbe_base_edge_t *               base_edge = NULL;
    fbe_block_edge_t *              block_edge = NULL;
    fbe_u32_t                       index = 0;

    base_transport_server = (fbe_base_transport_server_t *) block_transport_server;

    /* initialize the is consumed flag as false. */
    *is_consumed_range_p = FBE_FALSE;

    /* acquire the base transport server lock before we look at the client edge. */
    fbe_base_transport_server_lock(base_transport_server);

    base_edge = base_transport_server->client_list;

    while (base_edge != NULL)
    {
        /* we should send event here */
        block_edge = (fbe_block_edge_t *) base_edge;

        /* check the lba range and determine whether it belongs to the current edge. */
        if(fbe_block_transport_is_lba_range_overlaps_edge_extent(lba, block_count, block_edge))
        {
            /* we found the lba range as consumed and so return true to the caller. */
            *is_consumed_range_p = FBE_TRUE;
            break;
        }

        index++;

        if (index > FBE_BLOCK_TRANSPORT_SERVER_MAX_CLIENT_COUNT)
        {
            fbe_base_transport_server_unlock(base_transport_server);
            fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                     FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                     "%s Critical error looped client list\n", __FUNCTION__);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* get the next edge pointer. */
        base_edge = base_edge->next_edge;
    }

    fbe_base_transport_server_unlock(base_transport_server);
    return FBE_STATUS_OK;
}
fbe_status_t
fbe_block_transport_correct_server_index(fbe_block_transport_server_t * block_transport_server)
{
    fbe_u32_t edge_index = 0;
    fbe_block_edge_t * current_block_edge = NULL;

    current_block_edge = (fbe_block_edge_t*)block_transport_server->base_transport_server.client_list;

    while (current_block_edge != NULL) {

        if (current_block_edge->base_edge.server_index != edge_index) {
            fbe_base_transport_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                     "%s correct server idx %d to %d for server: 0x%x\n", 
                                     __FUNCTION__, 
                                     current_block_edge->base_edge.server_index, edge_index,
                                     current_block_edge->base_edge.server_id);
            current_block_edge->base_edge.server_index = edge_index;
        }
        edge_index++;
        if (edge_index > FBE_BLOCK_TRANSPORT_SERVER_MAX_CLIENT_COUNT) {
            fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                     "%s Critical error looped client list\n", __FUNCTION__);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        current_block_edge = (fbe_block_edge_t*)current_block_edge->base_edge.next_edge;
    } /* end while block edge != NULL */
    return FBE_STATUS_OK;
}
fbe_status_t
fbe_block_transport_server_attach_edge(fbe_block_transport_server_t * block_transport_server, 
                                       fbe_block_edge_t * block_edge,
                                       fbe_lifecycle_const_t * p_class_const,
                                       struct fbe_base_object_s * base_object)
{
    fbe_status_t status;
    fbe_u32_t edge_index = 0;
    fbe_block_edge_t * current_block_edge = NULL;
    fbe_block_edge_t * prev_block_edge = NULL;
    fbe_lba_t current_edge_end;
    fbe_lifecycle_state_t lifecycle_state;
    fbe_path_state_t path_state;
    fbe_package_id_t my_package_id;
    fbe_lba_t       block_edge_offset;

    fbe_get_package_id(&my_package_id);
    if (block_edge->capacity == 0 && my_package_id == FBE_PACKAGE_ID_SEP_0) 
    {
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                 FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                                 "%s Invalid capacity == 0\n",
                                 __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (block_edge->base_edge.server_index == FBE_EDGE_INDEX_INVALID)
    {
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                 FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                                 "%s Invalid server_index: %d\n",
                                 __FUNCTION__, block_edge->base_edge.server_index);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /* Make sure that the combination of offset and capacity does not go beyond the 
     * size of the object. 
     */
    block_edge_offset = (block_edge->base_edge.path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_IGNORE_OFFSET) ? 0 : block_edge->offset;
    if ((block_edge->capacity + block_edge_offset) > block_transport_server->capacity)
    {
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                 FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                                 "%s capacity 0x%llx + offset 0x%llx > server capacity: 0x%llx\n",
                                 __FUNCTION__,
                                 (unsigned long long)block_edge->capacity,
			         (unsigned long long)block_edge->offset,
			         (unsigned long long)block_transport_server->capacity);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_lifecycle_get_state(p_class_const, base_object, &lifecycle_state);
    /* We don't want to zero out the next edge, which could break the existing link.  
     * And it will be caught by overlapping check later.
     * ((fbe_base_edge_t *)block_edge)->next_edge = NULL;
    */

    current_block_edge = (fbe_block_edge_t*)block_transport_server->base_transport_server.client_list;
    prev_block_edge = NULL;
    while (current_block_edge != NULL) {
        current_edge_end = current_block_edge->capacity + current_block_edge->offset;
        /* The block edges should be sorted */
        if(current_block_edge->offset >=  block_edge->offset + block_edge->capacity){
            if ( prev_block_edge == NULL ) {
                /* We are inserting a new list head */
                block_transport_server->base_transport_server.client_list = (fbe_base_edge_t *)block_edge;
            } else {
                ((fbe_base_edge_t *)prev_block_edge)->next_edge = (fbe_base_edge_t *)block_edge;
            }
            ((fbe_base_edge_t *)block_edge)->next_edge = (fbe_base_edge_t *)current_block_edge;
            break;
        } else {
            /* check for overlap with current edge */
            if(current_edge_end > block_edge->offset){
                fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                         "%s error edge overlap, current(0x%llx/0x%llx), new (0x%llx/0x%llx)\n", 
                                         __FUNCTION__,
					 (unsigned long long)current_block_edge->offset,
					 (unsigned long long)current_block_edge->capacity,
                                         (unsigned long long)block_edge->offset,
					 (unsigned long long)block_edge->capacity);

                return FBE_STATUS_GENERIC_FAILURE;
            }
        }

        edge_index++;
        if(edge_index > FBE_BLOCK_TRANSPORT_SERVER_MAX_CLIENT_COUNT) {
            fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                     FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                     "%s Critical error looped client list\n", __FUNCTION__);
            
            return FBE_STATUS_GENERIC_FAILURE;
        }
        prev_block_edge = current_block_edge;
        current_block_edge = (fbe_block_edge_t*)current_block_edge->base_edge.next_edge;
    } /* end while block edge != NULL */

    /* Set the server index if it is not set correctly.
     */
    if (block_edge->base_edge.server_index != edge_index) {
        block_edge->base_edge.server_index = edge_index;
    }
    if(current_block_edge == NULL){
        ((fbe_base_edge_t *)block_edge)->next_edge = NULL;
        if(prev_block_edge == NULL){ /* The client list was empty */
            block_transport_server->base_transport_server.client_list = (fbe_base_edge_t *)block_edge;
        } else {
            ((fbe_base_edge_t *)prev_block_edge)->next_edge = (fbe_base_edge_t *)block_edge;
        }
    }

    status = fbe_base_transport_server_lifecycle_to_path_state(lifecycle_state, &path_state);
    ((fbe_base_edge_t *)block_edge)->path_state = path_state;

    if(status == FBE_STATUS_OK){
        fbe_get_package_id(&block_edge->server_package_id);
    }

    /*set some default priority on the esdge*/
    fbe_block_transport_edge_set_priority(block_edge, FBE_MEDIC_ACTION_IDLE);

	/* Set up io_entry and object_handle */
	block_edge->io_entry = block_transport_server->block_transport_const->io_entry;
	block_edge->object_handle = (fbe_object_handle_t)base_object;
    return status;
}

fbe_status_t
fbe_block_transport_edge_set_server_package_id(fbe_block_edge_t * block_edge_p,
                                                fbe_package_id_t server_package_id)
{
    block_edge_p->server_package_id = server_package_id;
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_block_transport_edge_get_server_package_id( fbe_block_edge_t * block_edge_p,
                                               fbe_package_id_t * server_package_id)
{
     * server_package_id = block_edge_p->server_package_id;
     return FBE_STATUS_OK;
}

fbe_status_t
fbe_block_transport_edge_set_client_package_id(fbe_block_edge_t * block_edge_p,
                                                fbe_package_id_t client_package_id)
{
    block_edge_p->client_package_id = client_package_id;
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_block_transport_edge_get_client_package_id( fbe_block_edge_t * block_edge_p,
                                               fbe_package_id_t * client_package_id)
{
     * client_package_id = block_edge_p->client_package_id;
     return FBE_STATUS_OK;
}

fbe_status_t 
fbe_block_transport_edge_set_geometry(fbe_block_edge_t * block_edge, fbe_block_edge_geometry_t block_edge_geometry)
{
    block_edge->block_edge_geometry = block_edge_geometry;
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_block_transport_edge_get_geometry(fbe_block_edge_t * block_edge, fbe_block_edge_geometry_t * block_edge_geometry)
{
     *block_edge_geometry = block_edge->block_edge_geometry;
     return FBE_STATUS_OK;
}

fbe_status_t
fbe_block_transport_edge_set_path_attr(fbe_block_edge_t * block_edge, fbe_path_attr_t path_attr)
{
    block_edge->base_edge.path_attr = path_attr;
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_block_transport_get_optimum_block_size(fbe_block_edge_geometry_t block_edge_geometry, fbe_optimum_block_size_t * optimum_block_size)
{
    switch(block_edge_geometry){
        case FBE_BLOCK_EDGE_GEOMETRY_512_512:
            *optimum_block_size = 1;
            break;
        case FBE_BLOCK_EDGE_GEOMETRY_512_520:
            *optimum_block_size = 64;
            break;
        case FBE_BLOCK_EDGE_GEOMETRY_520_520:
            *optimum_block_size = 1;
            break;
        case FBE_BLOCK_EDGE_GEOMETRY_4096_4096:
            *optimum_block_size = 1;
            break;
        case FBE_BLOCK_EDGE_GEOMETRY_4096_520:
            *optimum_block_size = 512;
            break;
        case FBE_BLOCK_EDGE_GEOMETRY_4160_4160:
            *optimum_block_size = 8;
            break;
        case FBE_BLOCK_EDGE_GEOMETRY_4160_520:
            *optimum_block_size = 8;
            break;
        default:
            *optimum_block_size = 0;
            return FBE_STATUS_GENERIC_FAILURE;

    }
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_block_transport_edge_get_optimum_block_size(fbe_block_edge_t * block_edge, fbe_optimum_block_size_t * optimum_block_size)
{
    fbe_status_t status;
    status =  fbe_block_transport_get_optimum_block_size(block_edge->block_edge_geometry, optimum_block_size);
    return status;
}

fbe_status_t
fbe_block_transport_get_exported_block_size(fbe_block_edge_geometry_t block_edge_geometry, fbe_block_size_t  * block_size)
{
    switch(block_edge_geometry){
        case FBE_BLOCK_EDGE_GEOMETRY_512_512:
            *block_size = 512;
            break;
        case FBE_BLOCK_EDGE_GEOMETRY_512_520:
            *block_size = 520;
            break;
        case FBE_BLOCK_EDGE_GEOMETRY_520_520:
            *block_size = 520;
            break;
        case FBE_BLOCK_EDGE_GEOMETRY_4096_4096:
            *block_size = 4096;
            break;
        case FBE_BLOCK_EDGE_GEOMETRY_4096_520:
            *block_size = 520;
            break;
        case FBE_BLOCK_EDGE_GEOMETRY_4160_4160:
            *block_size = 4160;
            break;
        case FBE_BLOCK_EDGE_GEOMETRY_4160_520:
            *block_size = 520;
            break;
        default:
            *block_size = 0;
            return FBE_STATUS_GENERIC_FAILURE;

    }
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_block_transport_get_physical_block_size(fbe_block_edge_geometry_t block_edge_geometry, fbe_block_size_t  * block_size)
{
    switch(block_edge_geometry){
        case FBE_BLOCK_EDGE_GEOMETRY_512_512:
            *block_size = 512;
            break;
        case FBE_BLOCK_EDGE_GEOMETRY_512_520:
            *block_size = 512;
            break;
        case FBE_BLOCK_EDGE_GEOMETRY_520_520:
            *block_size = 520;
            break;
        case FBE_BLOCK_EDGE_GEOMETRY_4096_4096:
        case FBE_BLOCK_EDGE_GEOMETRY_4096_520:
            *block_size = 4096;
            break;
        case FBE_BLOCK_EDGE_GEOMETRY_4160_4160:
        case FBE_BLOCK_EDGE_GEOMETRY_4160_520:
            *block_size = 4160;
            break;
        default:
            *block_size = 0;
            return FBE_STATUS_GENERIC_FAILURE;

    }
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_block_transport_server_attach_preserve_server(fbe_block_transport_server_t * block_transport_server, 
                                                  fbe_block_edge_t * block_edge,
                                                  fbe_lifecycle_const_t * p_class_const,
                                                  struct fbe_base_object_s * base_object)
{
    fbe_status_t status;
    fbe_u32_t edge_index = 0;
    fbe_block_edge_t * current_block_edge = NULL;
    fbe_block_edge_t * prev_block_edge = NULL;
    fbe_lba_t current_edge_end;
    fbe_lifecycle_state_t lifecycle_state;
    fbe_path_state_t path_state;
    fbe_package_id_t my_package_id;
    fbe_lba_t       block_edge_offset;

    fbe_get_package_id(&my_package_id);
    if (block_edge->capacity == 0 && my_package_id == FBE_PACKAGE_ID_SEP_0) 
    {
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                 FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                                 "%s Invalid capacity == 0\n",
                                 __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (block_edge->base_edge.server_index == FBE_EDGE_INDEX_INVALID)
    {
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                 FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                                 "%s Invalid server_index: %d\n",
                                 __FUNCTION__, block_edge->base_edge.server_index);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /* Make sure that the combination of offset and capacity does not go beyond the 
     * size of the object. 
     */
    block_edge_offset = (block_edge->base_edge.path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_IGNORE_OFFSET) ? 0 : block_edge->offset;
    if ((block_edge->capacity + block_edge_offset) > block_transport_server->capacity)
    {
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                 FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                                 "%s capacity 0x%llx + offset 0x%llx > server capacity: 0x%llx\n",
                                 __FUNCTION__,
                                 (unsigned long long)block_edge->capacity,
			         (unsigned long long)block_edge->offset,
			         (unsigned long long)block_transport_server->capacity);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_lifecycle_get_state(p_class_const, base_object, &lifecycle_state);
    /* We don't want to zero out the next edge, which could break the existing link.  
     * And it will be caught by overlapping check later.
     * ((fbe_base_edge_t *)block_edge)->next_edge = NULL;
    */

    current_block_edge = (fbe_block_edge_t*)block_transport_server->base_transport_server.client_list;
    prev_block_edge = NULL;
    while (current_block_edge != NULL) {
        current_edge_end = current_block_edge->capacity + current_block_edge->offset;
        /* The block edges should be sorted */
        if(current_block_edge->offset >=  block_edge->offset + block_edge->capacity){
            if ( prev_block_edge == NULL ) {
                /* We are inserting a new list head */
                block_transport_server->base_transport_server.client_list = (fbe_base_edge_t *)block_edge;
            } else {
                ((fbe_base_edge_t *)prev_block_edge)->next_edge = (fbe_base_edge_t *)block_edge;
            }
            ((fbe_base_edge_t *)block_edge)->next_edge = (fbe_base_edge_t *)current_block_edge;
            break;
        } else {
            /* check for overlap with current edge */
            if(current_edge_end > block_edge->offset){
                fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                         "%s error edge overlap, current(0x%llx/0x%llx), new (0x%llx/0x%llx)\n", 
                                         __FUNCTION__,
					 (unsigned long long)current_block_edge->offset,
					 (unsigned long long)current_block_edge->capacity,
                                         (unsigned long long)block_edge->offset,
					 (unsigned long long)block_edge->capacity);

                return FBE_STATUS_GENERIC_FAILURE;
            }
        }

        edge_index++;
        if(edge_index > FBE_BLOCK_TRANSPORT_SERVER_MAX_CLIENT_COUNT) {
            fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                     FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                     "%s Critical error looped client list\n", __FUNCTION__);
            
            return FBE_STATUS_GENERIC_FAILURE;
        }
        prev_block_edge = current_block_edge;
        current_block_edge = (fbe_block_edge_t*)current_block_edge->base_edge.next_edge;
    } /* end while block edge != NULL */

    if(current_block_edge == NULL){
        ((fbe_base_edge_t *)block_edge)->next_edge = NULL;
        if(prev_block_edge == NULL){ /* The client list was empty */
            block_transport_server->base_transport_server.client_list = (fbe_base_edge_t *)block_edge;
        } else {
            ((fbe_base_edge_t *)prev_block_edge)->next_edge = (fbe_base_edge_t *)block_edge;
        }
    }

    status = fbe_base_transport_server_lifecycle_to_path_state(lifecycle_state, &path_state);
    ((fbe_base_edge_t *)block_edge)->path_state = path_state;

    if(status == FBE_STATUS_OK){
        fbe_get_package_id(&block_edge->server_package_id);
    }

    /*set some default priority on the esdge*/
    fbe_block_transport_edge_set_priority(block_edge, FBE_MEDIC_ACTION_IDLE);

	/* Set up io_entry and object_handle */
	block_edge->io_entry = block_transport_server->block_transport_const->io_entry;
	block_edge->object_handle = (fbe_object_handle_t)base_object;
    return status;
}

fbe_status_t
fbe_block_transport_edge_get_exported_block_size(fbe_block_edge_t * block_edge, fbe_block_size_t  * block_size)
{
    fbe_status_t status;
    status = fbe_block_transport_get_exported_block_size(block_edge->block_edge_geometry, block_size);
    return status;
}

/*!**************************************************************
 * @fn fbe_block_transport_server_validate_capacity(
 *         fbe_block_transport_server_t * block_transport_server,
 *         fbe_lba_t  capacity,
 *         fbe_u32_t  placement,
 *         fbe_edge_index_t  *client_index,
 *         fbe_lba_t  *block_offset)
 ****************************************************************
 * @brief
 *  This function validates the given capacity can be fitted in 
 *  block transport server. If it can be fitted, a valid offset 
 *  and client index is returned.
 * 
 * @param block_transport_server - The server to attach in the future. (IN)
 * @param capacity - The capacity that client requested. (IN/OUT)
 *                   In case of SPECIFIC_LOCATION placment, it will
 *                   return the available capacity from the requested offset.
 * @param placement - Type of placement (best fit etc)
 * @param b_ignore_offset - The offset will not be used so ignore it.
 * @param client_index - This is next available client index. (OUT)
 * @param block_offset - This is block offset, if a client connects 
 *                       to the server with given capacity. (IN/OUT)
 *
 * @return fbe_status_t   
 *
 ****************************************************************/
fbe_status_t
fbe_block_transport_server_validate_capacity(fbe_block_transport_server_t * block_transport_server, 
                                             fbe_lba_t          *capacity,
                                             fbe_u32_t          placement,
                                             fbe_bool_t         b_ignore_offset,
                                             fbe_edge_index_t   *client_index,
                                             fbe_lba_t          *block_offset)
{
    fbe_u32_t edge_index = 0;
    fbe_block_edge_t * current_block_edge = NULL;
    fbe_block_edge_t * prev_block_edge = NULL;
    fbe_lba_t prev_edge_end = 0;
    fbe_lba_t min_unuse_space = block_transport_server->capacity;  /* set it to the capacity, which is the maxmium possible unused space.*/
    fbe_lba_t current_edge_start = 0;
    fbe_lba_t current_edge_end = 0;
    fbe_lba_t requested_start = 0;
    fbe_lba_t requested_end = 0;

    *client_index = edge_index;


    /* Make sure that the capacity does not go beyond the 
     * size of the object. 
     */
    if (*capacity > block_transport_server->capacity)
    {
        fbe_base_transport_trace(FBE_TRACE_LEVEL_WARNING,
                                 FBE_TRACE_MESSAGE_ID_INVALID_PARAMETER,
                                 "block_trans_srv_valid_capaciy 0x%llx > srv cap: 0x%llx\n",
                                 (unsigned long long)(*capacity),
				 (unsigned long long)block_transport_server->capacity);
        if(placement == FBE_BLOCK_TRANSPORT_SPECIFIC_LOCATION)
        {
            *capacity = 0;  // nothing available from the specified offset.
        }
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* get the client list and go thru each of the client to see if there is enough room for this capacity*/
    current_block_edge = (fbe_block_edge_t*)block_transport_server->base_transport_server.client_list;
    prev_block_edge = NULL;
    prev_edge_end = 0;

    /* Support for NDB, and other cases where we may wish to specify exact offsets for a LUN */
    if (placement == FBE_BLOCK_TRANSPORT_SPECIFIC_LOCATION)
    {
        /* Make sure that the block offset does not go beyond the 
         * size of the object.
         */
        if (*block_offset > block_transport_server->capacity)
        {
            fbe_base_transport_trace(FBE_TRACE_LEVEL_WARNING,
                                     FBE_TRACE_MESSAGE_ID_INVALID_PARAMETER,
                                     "block_trans_srv_valid_cap: Invalid block_offset : 0x%llx\n",
                                     (unsigned long long)(*block_offset));

            *capacity = 0;  // nothing available from the specified offset.

            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* Validate that the available capacity is at least greater than 0.
         */
        if (*capacity < 1)
        {
            fbe_base_transport_trace(FBE_TRACE_LEVEL_WARNING,
                     FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                     "block_trans_srv_valid_cap: cap 0x%llx offset 0x%llx exceed srv cap 0x%llx\n", 
                     (unsigned long long)*capacity, (unsigned long long)*block_offset, (unsigned long long)block_transport_server->capacity);

            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* Check to make sure the requested offset & capacity wouldn't exceed the capacity
         * exposed by the server for our use
         */
        requested_start = *block_offset;
        requested_end   = *block_offset + *capacity - 1;
        if (requested_end > block_transport_server->capacity)
        {
            fbe_base_transport_trace(FBE_TRACE_LEVEL_WARNING,
                     FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                     "block_trans_srv_valid_cap: cap %p offset 0x%llx exceed srv cap 0x%llx\n", 
                     capacity, (unsigned long long)*block_offset, (unsigned long long)block_transport_server->capacity);

            requested_end = block_transport_server->capacity - 1;
            *capacity = block_transport_server->capacity - *block_offset ;  
            if (*capacity == 0)
            {
                return FBE_STATUS_GENERIC_FAILURE;
            }
        }

        /* Check to make sure that the requested offset & capacity wouldn't overlap any
         * existing edges
         */
        while (current_block_edge != NULL) {
            current_edge_start  = current_block_edge->offset;
            current_edge_end    = current_block_edge->offset + current_block_edge->capacity - 1;

            /* Check to see if this edge overlaps the requested offset and capacity */
            /* The current edge starts inside the requested edge */
            if ((current_edge_start >= requested_start) && (current_edge_start <= requested_end))
            {
                fbe_base_transport_trace(FBE_TRACE_LEVEL_WARNING,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "block_trans_srv_valid_cap: cap %p offset 0x%llx overlap existing edge start 0x%llx\n", 
                         capacity, *block_offset, current_edge_start);
            
                *capacity = current_edge_start - requested_start;  // some space available.

                return FBE_STATUS_GENERIC_FAILURE;
            }
            /* The current edge ends inside the requested edge*/
            if ((current_edge_end >= requested_start) && (current_edge_end <= requested_end))
            {
                fbe_base_transport_trace(FBE_TRACE_LEVEL_WARNING,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "block_trans_srv_valid_cap: cap 0x%llx offset 0x%llx overlap existing edge end 0x%llx\n", 
                         (unsigned long long)(*capacity),
			 (unsigned long long)(*block_offset),
			 (unsigned long long)current_edge_end);
            
                *capacity = 0;  // can't start.

                return FBE_STATUS_GENERIC_FAILURE;
            }
            /* The requested edge starts inside the current edge */
            if ((requested_start >= current_edge_start) && (requested_start <= current_edge_end))
            {
                fbe_base_transport_trace(FBE_TRACE_LEVEL_WARNING,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "block_trans_srv_valid_cap: request start 0x%llx overlap existing edge,0x%llx/0x%llx\n", 
                         (unsigned long long)(*block_offset),
			 (unsigned long long)current_edge_start,
			 (unsigned long long)current_edge_end);
            
                *capacity = 0;  // can't start.

                return FBE_STATUS_GENERIC_FAILURE;
            }
            /* The requested edge ends inside the current edge */
            if ((requested_end >= current_edge_start) && (requested_end <= current_edge_end))
            {
                fbe_base_transport_trace(FBE_TRACE_LEVEL_WARNING,
                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                         "block_trans_srv_valid_cap: request end 0x%llx overlap existing edge,0x%llx/0x%llx\n", 
                         (unsigned long long)requested_end,
			 (unsigned long long)current_edge_start,
			 (unsigned long long)current_edge_end);

                *capacity = current_edge_start - requested_start;  // some space available.

                return FBE_STATUS_GENERIC_FAILURE;
            }
            /* Determine whether or not to use this edge index */
            if(requested_start > current_edge_start)
            {
                *client_index = edge_index;
            }
            edge_index++;
            /* Sanity check */
            if(edge_index > FBE_BLOCK_TRANSPORT_SERVER_MAX_CLIENT_COUNT) {
                fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                         FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                         "block_trans_srv_valid_cap: Critical error looped client list\n");
                *capacity = 0;
                return FBE_STATUS_GENERIC_FAILURE;
            }
            current_block_edge = (fbe_block_edge_t*)current_block_edge->base_edge.next_edge;
        }

        /* The request fits into available, unallocated space */
        return FBE_STATUS_OK;
    }


	/*let's roll to the next available space after the default start and skip private luns for example*/
	while (current_block_edge != NULL) {
		if ((current_block_edge->capacity + current_block_edge->offset) <= block_transport_server->default_offset) {
			current_block_edge = (fbe_block_edge_t*)current_block_edge->base_edge.next_edge;
		}else{
			/*sanity check*/
			if (current_block_edge->offset < block_transport_server->default_offset) {
				fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                     FBE_TRACE_MESSAGE_ID_INFO,
                                     "block_trans_srv_valid_cap: offset 0x%llX small than default:0x%llX\n", 
                                     (unsigned long long)current_block_edge->offset,
				     (unsigned long long)block_transport_server->default_offset);

				return FBE_STATUS_GENERIC_FAILURE;
			}

			break;
		}
	}

	/*after scrolling let's see where did we end up, can we fit the request before this edge ?*/
	if ((current_block_edge != NULL) && (current_block_edge->offset - block_transport_server->default_offset) >= *capacity) {
		fbe_base_transport_trace(FBE_TRACE_LEVEL_DEBUG_LOW,
                                     FBE_TRACE_MESSAGE_ID_INFO,
                                     "block_trans_srv_valid_cap: 0x%llx fit free space(offset 0x%llx,block_cnt 0x%llx)\n", 
                                     (unsigned long long)(*capacity),
				     (unsigned long long)prev_edge_end, 
                                     (unsigned long long)(current_block_edge->offset - prev_edge_end));

            /*the free space between default offset and current client is big enough for this capacity */ 
            if (placement == FBE_BLOCK_TRANSPORT_FIRST_FIT) {
                /* found the first fit, return with success */
                fbe_base_transport_trace(FBE_TRACE_LEVEL_DEBUG_LOW,
                                         FBE_TRACE_MESSAGE_ID_INFO,
                                         "block_trans_srv_valid_cap: found first fit(offset 0x%llx, block_cnt 0x%llx).\n", 
                                         (unsigned long long)prev_edge_end, 
                                         (unsigned long long)(current_block_edge->offset - prev_edge_end));

                *block_offset = prev_edge_end;
                *client_index = edge_index;
				return FBE_STATUS_OK;
                
            }
            /*! @todo: to be verified: Best fit case, where the unused space is minium */
            if(min_unuse_space > (current_block_edge->offset - (prev_edge_end + *capacity))) {
                min_unuse_space = (current_block_edge->offset - (prev_edge_end + *capacity));
                *block_offset = prev_edge_end;
                *client_index = edge_index;
                fbe_base_transport_trace(FBE_TRACE_LEVEL_DEBUG_LOW,
                                         FBE_TRACE_MESSAGE_ID_INFO,
                                         "block_trans_srv_valid_cap: current best fit(offset 0x%llx,block_cnt 0x%llx)\n", 
                                         (unsigned long long)prev_edge_end, 
                                         (unsigned long long)(current_block_edge->offset - prev_edge_end));
				return FBE_STATUS_OK;
            }

			
	}

	/*if we could not do that, we need to move the prev_edge_end*/
	if (current_block_edge != NULL){
		prev_edge_end = block_transport_server->default_offset;
	}


    while (current_block_edge != NULL) {
        /* find out where the previous client ends*/

        /* The block edges should be sorted */
        if((current_block_edge->offset - prev_edge_end) >= *capacity){
            fbe_base_transport_trace(FBE_TRACE_LEVEL_DEBUG_LOW,
                                     FBE_TRACE_MESSAGE_ID_INFO,
                                     "block_trans_srv_valid_cap: 0x%llx fit free space(offset 0x%llx, block_cnt 0x%llx).\n", 
                                     (unsigned long long)(*capacity),
				     (unsigned long long)prev_edge_end, 
                                     (unsigned long long)(current_block_edge->offset - prev_edge_end));

            /*the free space between previous client and current client is big enough for this capacity */ 
            if (placement == FBE_BLOCK_TRANSPORT_FIRST_FIT) {
                /* found the first fit, return with success */
                fbe_base_transport_trace(FBE_TRACE_LEVEL_DEBUG_LOW,
                                         FBE_TRACE_MESSAGE_ID_INFO,
                                         "block_trans_srv_valid_cap: found first fit(offset 0x%llx,block_cnt 0x%llx)\n", 
                                         (unsigned long long)prev_edge_end, 
                                         (unsigned long long)(current_block_edge->offset - prev_edge_end));

                *block_offset = prev_edge_end;
                *client_index = edge_index;
                return FBE_STATUS_OK;
            }
            /*! @todo: to be verified: Best fit case, where the unused space is minium */
            if(min_unuse_space > (current_block_edge->offset - (prev_edge_end + *capacity))) {
                min_unuse_space = (current_block_edge->offset - (prev_edge_end + *capacity));
                *block_offset = prev_edge_end;
                *client_index = edge_index;
                fbe_base_transport_trace(FBE_TRACE_LEVEL_DEBUG_LOW,
                                         FBE_TRACE_MESSAGE_ID_INFO,
                                         "block_trans_srv_valid_cap: current best fit(offset 0x%llx,block_cnt 0x%llx)\n", 
                                        (unsigned long long)prev_edge_end, 
                                         (unsigned long long)(current_block_edge->offset - prev_edge_end));
				return FBE_STATUS_OK;
            }
        }
        edge_index++;
        if(edge_index > FBE_BLOCK_TRANSPORT_SERVER_MAX_CLIENT_COUNT) {
            fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                     FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                     "block_trans_srv_valid_cap: Critical error looped client list\n");
            return FBE_STATUS_GENERIC_FAILURE;
        }
        prev_block_edge = current_block_edge;
        prev_edge_end = prev_block_edge->capacity + prev_block_edge->offset;
        
        current_block_edge = (fbe_block_edge_t*)current_block_edge->base_edge.next_edge;
    } /* end while block edge != NULL */
    if(current_block_edge == NULL){ /*reach the end of the client list*/
        if(prev_block_edge == NULL){ /* The client list was empty */
            /* Always populate the block offset for the client.*/
            *block_offset = block_transport_server->default_offset; 

            /* If the `b_ignore_offset' flag is set it means that the offset
             * will not be used/added.
             */
            if (b_ignore_offset == FBE_TRUE)
            {
                /* If there insufficient capacity fail the request.
                 */
                if (*capacity > block_transport_server->capacity)
                {
                    fbe_base_transport_trace(FBE_TRACE_LEVEL_WARNING,
                                 FBE_TRACE_MESSAGE_ID_INVALID_PARAMETER,
                                 "block_trans_srv_valid_cap: request cap: 0x%llx is > server cap: 0x%llx\n",
                                 (unsigned long long)*capacity, (unsigned long long)block_transport_server->capacity);
                    *capacity = 0;  /* Insufficient capacity. */
                    return FBE_STATUS_GENERIC_FAILURE;
                }
            }
            else if ((*capacity + *block_offset) > block_transport_server->capacity) 
            {
                /* This will be the first client, use the default offset to determine
                 * if there is sufficient capacity or not.
                 */
                fbe_base_transport_trace(FBE_TRACE_LEVEL_WARNING,
                                 FBE_TRACE_MESSAGE_ID_INVALID_PARAMETER,
                                 "block_trans_srv_valid_cap: request cap: 0x%llx plus offset: 0x%llx is > server cap: 0x%llx\n",
                                 (unsigned long long)(*capacity),
				 (unsigned long long)(*block_offset),
				 (unsigned long long)block_transport_server->capacity);
                *capacity = 0;  /* Insufficient capacity. */
                return FBE_STATUS_GENERIC_FAILURE;
            }

            /* If success populate the client index and return */
            *client_index = edge_index;
            fbe_base_transport_trace(FBE_TRACE_LEVEL_DEBUG_LOW,
                                     FBE_TRACE_MESSAGE_ID_INFO,
                                     "block_trans_srv_valid_cap: first client, offset starts at 0x%llx\n",
                                     (unsigned long long)(*block_offset));
            return FBE_STATUS_OK;
        }
        else{ /*reach the end of the client list, check if the remaining space is a better fit*/
            prev_edge_end = prev_block_edge->capacity + prev_block_edge->offset;
            if ((prev_edge_end + *capacity)<= block_transport_server->capacity){
                if (placement == FBE_BLOCK_TRANSPORT_FIRST_FIT) {
                    fbe_base_transport_trace(FBE_TRACE_LEVEL_DEBUG_LOW,
                                             FBE_TRACE_MESSAGE_ID_INFO,
                                             "block_trans_srv_valid_cap: found first fit(offset 0x%llx,block_cnt 0x%llx)\n", 
                                             (unsigned long long)prev_edge_end, 
                                             (unsigned long long)(block_transport_server->capacity - prev_edge_end));
                    *block_offset = prev_edge_end;
                    *client_index = edge_index;
                    return FBE_STATUS_OK;
                }
                if(min_unuse_space > (block_transport_server->capacity - (prev_edge_end + *capacity))) {
                    /*remaining space is a better fit */
                    fbe_base_transport_trace(FBE_TRACE_LEVEL_DEBUG_LOW,
                                             FBE_TRACE_MESSAGE_ID_INFO,
                                             "block_trans_srv_valid_cap: current best fit(offset 0x%llx,block_cnt 0x%llx)\n", 
                                            (unsigned long long)prev_edge_end, 
                                             (unsigned long long)(block_transport_server->capacity - prev_edge_end));
                    *block_offset = prev_edge_end;
                    *client_index = edge_index;
                    return FBE_STATUS_OK;
                }
                if(min_unuse_space != block_transport_server->capacity) {
                    /* a fit was found */
                    return FBE_STATUS_OK;
                }
            }
        }
    }
    /* Did not find a fit, return failure */
    fbe_base_transport_trace(FBE_TRACE_LEVEL_WARNING,
                             FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                             "block_trans_srv_valid_cap: cap 0x%llx doesn't fit into the remaining space.\n", 
                             (unsigned long long)(*capacity));
    return FBE_STATUS_GENERIC_FAILURE;
}

/*!**************************************************************
 * @fn fbe_block_transport_server_get_max_unused_extent_size
 ****************************************************************
 * @brief
 *  This function search thru all the unused extents and 
 *  returns the size of the max extent.
 * 
 * @param block_transport_server - The server to attach in the future. (IN)
 * @param extent_size - The size to be returned. (OUT)
 *
 * @return fbe_status_t   
 *
 ****************************************************************/
fbe_status_t
fbe_block_transport_server_get_max_unused_extent_size(fbe_block_transport_server_t * block_transport_server, 
                                                      fbe_lba_t * extent_size)
{
    fbe_u32_t edge_index = 0;
    fbe_block_edge_t * current_block_edge = NULL;
    fbe_block_edge_t * prev_block_edge = NULL;
    fbe_lba_t prev_edge_end = block_transport_server->default_offset;  // skip the reserved area
    fbe_lba_t max_unused_space = 0;

    /* get the client list and go thru each of the client to see if there is enough room for this capacity*/
    current_block_edge = (fbe_block_edge_t*)block_transport_server->base_transport_server.client_list;
    while (current_block_edge != NULL) {
        /* The block edges are sorted */
        /* Check to see if there's a gap between the previous edge and the current edge */
        if(current_block_edge->offset > prev_edge_end){
            /* Handle the case where there's a gap between the previous edge and the
               current edge, and that gap happens to span the default_offset.
            */
            if((prev_edge_end < block_transport_server->default_offset) &&
               (current_block_edge->offset > block_transport_server->default_offset))
            {
                if((current_block_edge->offset - block_transport_server->default_offset) > max_unused_space)
                {
                    max_unused_space = current_block_edge->offset - block_transport_server->default_offset;
                }
            }
            /* The gap doesn't span the default_offset */
            else
            {
                if(((current_block_edge->offset - prev_edge_end) > max_unused_space) &&
                (current_block_edge->offset >= block_transport_server->default_offset))
                {
                    max_unused_space = current_block_edge->offset - prev_edge_end;
                }
            }
        }
        edge_index++;
        if(edge_index > FBE_BLOCK_TRANSPORT_SERVER_MAX_CLIENT_COUNT) {
            fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR, 
                                     FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                                     "%s Critical error looped client list\n", __FUNCTION__);
            *extent_size = 0;
            return FBE_STATUS_GENERIC_FAILURE;
        }
        prev_block_edge = current_block_edge;
        /* find out where the previous client ends*/
        prev_edge_end = prev_block_edge->capacity + prev_block_edge->offset;
        current_block_edge = (fbe_block_edge_t*)current_block_edge->base_edge.next_edge;
    } /* end while block edge != NULL */
    if(current_block_edge == NULL)
    { /* end of client list */
        if((prev_block_edge == NULL)                                ||
           (prev_edge_end <= block_transport_server->default_offset)    )
        { 
            /* Either the client list was empty or there are only private raid
             * groups using this extent.
             */
            max_unused_space = block_transport_server->capacity - block_transport_server->default_offset;
        } 
        else 
        { 
            /* reach the end of the client list, check if the remaining space is bigger */
            if ((block_transport_server->capacity - prev_edge_end) > max_unused_space){
                max_unused_space = block_transport_server->capacity - prev_edge_end;
            }
        }
    }
    *extent_size = max_unused_space;
    return FBE_STATUS_OK;
}

/*!**************************************************************
 * fbe_block_transport_server_can_object_be_removed_when_edge_is_removed()
 ****************************************************************
 * @brief
 * This function is used to determine if given object can be removed
 * if upstream edge is removed. 
 * 
 * @param base_config_p - The object pointer.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  03/22/2010 - Created. Jesus Medina
 *
 ****************************************************************/
fbe_status_t 
fbe_block_transport_server_can_object_be_removed_when_edge_is_removed(fbe_block_transport_server_t * block_transport_server,
        fbe_object_id_t object_id, fbe_bool_t *can_object_be_removed)
{
    fbe_u32_t        number_of_leftover_objects = 0;
    fbe_block_edge_t *current_block_edge = NULL;

    *can_object_be_removed = FBE_TRUE;

    current_block_edge = (fbe_block_edge_t*)block_transport_server->base_transport_server.client_list;
    while (current_block_edge != NULL)
    {
        if ((object_id != current_block_edge->base_edge.client_id)
            &&(current_block_edge->base_edge.client_id != FBE_OBJECT_ID_INVALID))
        {
            number_of_leftover_objects++;
        }

        current_block_edge = (fbe_block_edge_t*)current_block_edge->base_edge.next_edge;
    }

    if (number_of_leftover_objects >= 1)
    {
        *can_object_be_removed = FBE_FALSE;
    }
    return FBE_STATUS_OK;
}
/****************************************************************************
 * end fbe_block_transport_server_can_object_be_removed_when_edge_is_removed
 ****************************************************************************/

fbe_status_t fbe_block_transport_server_set_medic_priority(fbe_block_transport_server_t * block_transport_server,
                                                           fbe_medic_action_priority_t set_priority)
{
    fbe_base_transport_server_t *   base_transport_server = (fbe_base_transport_server_t *)block_transport_server;
    fbe_u32_t                       i = 0;
    fbe_base_edge_t *               base_edge = NULL;
    fbe_block_edge_t *              block_edge = NULL;

    fbe_base_transport_server_lock(base_transport_server);

    base_edge = base_transport_server->client_list;
    block_edge = (fbe_block_edge_t *)base_edge;
    while(base_edge != NULL) {
        if(block_edge->medic_action_priority != set_priority){
            fbe_base_transport_trace(FBE_TRACE_LEVEL_DEBUG_LOW, FBE_TRACE_MESSAGE_ID_INFO, 
                                     "bts new medic priority: %d old: %d server: 0x%x client: 0x%x\n", 
                                     set_priority, block_edge->medic_action_priority, 
                                     base_edge->server_id, base_edge->client_id);
            block_edge->medic_action_priority = set_priority;
            /* We should send event here */
            if(block_edge->server_package_id != block_edge->client_package_id){
                fbe_topology_send_event_to_another_package(base_edge->client_id, 
                                                           block_edge->client_package_id,
                                                           FBE_EVENT_TYPE_ATTRIBUTE_CHANGED, 
                                                           base_edge);
            } else {
                fbe_topology_send_event(base_edge->client_id, FBE_EVENT_TYPE_ATTRIBUTE_CHANGED, base_edge);
            }
        }
        i++;
        if(i > FBE_BLOCK_TRANSPORT_SERVER_MAX_CLIENT_COUNT){
            fbe_base_transport_server_unlock(base_transport_server);
            fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR, 
                                     FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                                     "%s Critical error looped client list\n", __FUNCTION__);

            return FBE_STATUS_GENERIC_FAILURE;
        }

        base_edge = base_edge->next_edge;
    }

    fbe_base_transport_server_unlock(base_transport_server);

    return FBE_STATUS_OK;
}

fbe_status_t fbe_block_transport_server_set_traffic_priority(fbe_block_transport_server_t *block_transport_server_p, fbe_traffic_priority_t set_priority)
{
    block_transport_server_p->current_priority = set_priority;

    /*and mark all the edges of the server as busy and let the clients process that in their event context*/
    fbe_block_transport_server_set_path_traffic_priority(block_transport_server_p, set_priority);
    return FBE_STATUS_OK;

}

fbe_status_t fbe_block_transport_server_get_traffic_priority(fbe_block_transport_server_t *block_transport_server_p, fbe_traffic_priority_t *get_priority)
{
    *get_priority = block_transport_server_p->current_priority;
    return FBE_STATUS_OK;
}


fbe_status_t fbe_block_transport_server_set_path_traffic_priority(fbe_block_transport_server_t * block_transport_server, fbe_traffic_priority_t traffic_priority)
{
    fbe_base_transport_server_t * base_transport_server = (fbe_base_transport_server_t *) block_transport_server;
    fbe_u32_t i = 0;
    fbe_base_edge_t * base_edge = NULL;
    fbe_block_edge_t * block_edge = NULL;

    fbe_base_transport_server_lock(base_transport_server);

    base_edge = base_transport_server->client_list;
    while(base_edge != NULL) {
        block_edge = (fbe_block_edge_t *)base_edge;
        if(block_edge->traffic_priority != traffic_priority){
            block_edge->traffic_priority = traffic_priority;

            /* We should send event here */
            if(block_edge->server_package_id != block_edge->client_package_id){
                fbe_topology_send_event_to_another_package(base_edge->client_id, 
                                                            block_edge->client_package_id,
                                                            FBE_EVENT_TYPE_EDGE_TRAFFIC_LOAD_CHANGE, 
                                                            base_edge);
            } else {
                fbe_topology_send_event(base_edge->client_id, FBE_EVENT_TYPE_EDGE_TRAFFIC_LOAD_CHANGE, base_edge);
            }
        }
        i++;
        if(i > FBE_BLOCK_TRANSPORT_SERVER_MAX_CLIENT_COUNT){
            fbe_base_transport_server_unlock(base_transport_server);
            
            fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR, 
                                     FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                                     "%s Critical error looped client list\n", __FUNCTION__);

            return FBE_STATUS_GENERIC_FAILURE;
        }
        base_edge = base_edge->next_edge;
    }

    fbe_base_transport_server_unlock(base_transport_server);

    return FBE_STATUS_OK;
}

/*check if all the clients are hibernating or not*/
fbe_bool_t fbe_block_transport_server_all_clients_hibernating(fbe_block_transport_server_t *block_transport_server)
{
    fbe_base_transport_server_t * base_transport_server = (fbe_base_transport_server_t *) block_transport_server;
    fbe_base_edge_t * base_edge = NULL;
    fbe_block_edge_t * block_edge = NULL;
    fbe_path_attr_t     path_attr = 0;

    fbe_base_transport_server_lock(base_transport_server);

    base_edge = base_transport_server->client_list;
    while(base_edge != NULL) {
        block_edge = (fbe_block_edge_t *)base_edge;
        fbe_block_transport_get_path_attributes(block_edge, &path_attr);

        /* If the object ID is invalid, then this is a dummy edge, 
         * do not look for the attribute on this edge since there is no client.
         */
        if ((block_edge->base_edge.client_id != FBE_OBJECT_ID_INVALID) &&
            !(path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_IS_HIBERNATING)){
            fbe_base_transport_server_unlock(base_transport_server);
            return FBE_FALSE;
        }
       
        base_edge = base_edge->next_edge;
    }

    fbe_base_transport_server_unlock(base_transport_server);

    /*if we got here, everyone is hibernating*/
    return FBE_TRUE;

}


/*what is the lowest latency time to becoming ready after hibernating*/
fbe_status_t fbe_block_transport_server_get_lowest_ready_latency_time(fbe_block_transport_server_t *block_transport_server_p,
                                                                       fbe_u64_t *lowest_latency_in_sec)
{
    fbe_base_transport_server_t *   base_transport_server = (fbe_base_transport_server_t *) block_transport_server_p;
    fbe_base_edge_t *               base_edge = NULL;
    fbe_block_edge_t *              block_edge = NULL;
    fbe_u64_t                       lowest_time = 0x00000000FFFFFFFF;
    
    fbe_base_transport_server_lock(base_transport_server);

    base_edge = base_transport_server->client_list;
    while(base_edge != NULL) {
        block_edge = (fbe_block_edge_t *)base_edge;
        if ((base_edge->client_id != FBE_OBJECT_ID_INVALID) &&
            (block_edge->time_to_become_readay_in_sec < lowest_time)) {
            lowest_time = block_edge->time_to_become_readay_in_sec;
        }
       
        base_edge = base_edge->next_edge;
    }

    fbe_base_transport_server_unlock(base_transport_server);

    /*we don't really care if we found nothing, we just give a huge latency time of 0x00000000FFFFFFFF so every object
    can satisy that*/

    /*give the lowest one we found*/
    *lowest_latency_in_sec = lowest_time;
    
    return FBE_STATUS_OK;


}


fbe_status_t fbe_block_transport_server_set_time_to_become_ready(fbe_block_transport_server_t *block_transport_server_p,
                                                                 fbe_object_id_t client_id,
                                                                 fbe_u64_t time_to_ready_in_sec)
{
    fbe_base_transport_server_t * base_transport_server = (fbe_base_transport_server_t *)block_transport_server_p;
    fbe_u32_t i = 0;
    fbe_base_edge_t * base_edge = NULL;
    fbe_block_edge_t * block_edge = NULL;

    fbe_base_transport_server_lock(base_transport_server);

    base_edge = base_transport_server->client_list;
    while(base_edge != NULL) {
        if (base_edge->client_id == client_id){
            block_edge = (fbe_block_edge_t *)base_edge;
            block_edge->time_to_become_readay_in_sec = time_to_ready_in_sec;
            break;
        }
        i++; 
        if(i > FBE_BLOCK_TRANSPORT_SERVER_MAX_CLIENT_COUNT){
            fbe_base_transport_server_unlock(base_transport_server);
            
            fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR, 
                                     FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                                     "%s Critical error looped client list\n", __FUNCTION__);

            return FBE_STATUS_GENERIC_FAILURE;
        }
        base_edge = base_edge->next_edge;
    }

    fbe_base_transport_server_unlock(base_transport_server);

    if(base_edge == NULL){ /*We did not found the edge */
        
        fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Error edge not found, client_id = 0x%x\n", __FUNCTION__, client_id);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;

}

/*this funtion is sending IO from the queues. We use it usually as a trigger to send IOs that were queued on
the object for various resons, other than the drive physical queue was full*/
fbe_status_t 
fbe_block_transport_server_process_io_from_queue(fbe_block_transport_server_t* block_transport_server)
{
    fbe_status_t status;
    fbe_queue_head_t tmp_queue;

    fbe_queue_init(&tmp_queue);

    fbe_spinlock_lock(&block_transport_server->queue_lock);
    status = block_transport_server_restart_io(block_transport_server, &tmp_queue);
    fbe_spinlock_unlock(&block_transport_server->queue_lock);

    if(!fbe_queue_is_empty(&tmp_queue)) {
        fbe_transport_run_queue_push(&tmp_queue, block_transport_server_run_queue_completion, block_transport_server);
    }

    fbe_queue_destroy(&tmp_queue);
    return status;
}

/*this funtion is sending IO from the queues synchronously. We use it usually as a trigger to send IOs that were queued on
the object for various resons, other than the drive physical queue was full*/
fbe_status_t 
fbe_block_transport_server_process_io_from_queue_sync(fbe_block_transport_server_t* block_transport_server)
{
    fbe_status_t status;
    fbe_queue_head_t tmp_queue;
	fbe_packet_t * packet;
	fbe_queue_element_t * queue_element;

    fbe_queue_init(&tmp_queue);

    fbe_spinlock_lock(&block_transport_server->queue_lock);
    status = block_transport_server_restart_io(block_transport_server, &tmp_queue);
    fbe_spinlock_unlock(&block_transport_server->queue_lock);

	while(queue_element = fbe_queue_pop(&tmp_queue)){
		packet = fbe_transport_queue_element_to_packet(queue_element);
		fbe_transport_set_completion_function(packet, block_transport_server_run_queue_completion, block_transport_server);
		fbe_transport_complete_packet(packet);
	}

    fbe_block_transport_server_resume(block_transport_server);
	
	/* Due to race some I/O may got on the queue's and we need to deal with it */
    fbe_spinlock_lock(&block_transport_server->queue_lock);
    status = block_transport_server_restart_io(block_transport_server, &tmp_queue);
    fbe_spinlock_unlock(&block_transport_server->queue_lock);

	while(queue_element = fbe_queue_pop(&tmp_queue)){
		packet = fbe_transport_queue_element_to_packet(queue_element);
		fbe_transport_set_completion_function(packet, block_transport_server_run_queue_completion, block_transport_server);
		fbe_transport_complete_packet(packet);
	}


    fbe_queue_destroy(&tmp_queue);
    return status;
}

static fbe_status_t fbe_block_transport_drain_queue(fbe_block_transport_server_t* block_transport_server,
                                                    fbe_queue_head_t *queue_head,
                                                    fbe_status_t completion_status)
{
    fbe_packet_t *          new_packet = NULL;
    fbe_status_t            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_queue_element_t *   queue_element = NULL;
    fbe_queue_element_t *   next_queue_element = NULL;
    fbe_queue_head_t        temp_queue;

    fbe_queue_init(&temp_queue);
    
    fbe_spinlock_lock(&block_transport_server->queue_lock);
    queue_element = fbe_queue_front(queue_head);
    while (queue_element != NULL) {
            /* Get the next queue element since we might free the packet below and cannot touch
             * the queue element after that point.  
             */
            next_queue_element = fbe_queue_next(queue_head, queue_element);

            new_packet = fbe_transport_queue_element_to_packet(queue_element);
            status = fbe_transport_get_status_code(new_packet);

            /*the packet we just look at might be a packet we left on the queue while the object was hibernating and we got an IO.
            We don't want to drain these packets, but just leave them here. The condition FBE_BASE_CONFIG_LIFECYCLE_COND_DEQUEUE_PENDING_IO
            will take care of them*/

            if ((status == FBE_STATUS_HIBERNATION_EXIT_PENDING) && (FBE_STATUS_BUSY == completion_status)){
                queue_element = next_queue_element;
                continue;
            }else{
                status = fbe_transport_remove_packet_from_queue(new_packet);
                if (status != FBE_STATUS_OK) {
                    continue;
                }

                fbe_transport_set_status(new_packet, completion_status, 0);

                /*now we don't complete the packet, but just stick it in the temporary queue which we will complete later*/
                fbe_queue_push(&temp_queue, &new_packet->queue_element);
            }

            queue_element = next_queue_element;/*go to next one*/
    }


    fbe_spinlock_unlock(&block_transport_server->queue_lock);

    /*now we are ready to complete all the packets that were not hibrenating*/
    while (!fbe_queue_is_empty(&temp_queue)) {
		fbe_transport_run_queue_push(&temp_queue, NULL, NULL);
		/*
        queue_element = fbe_queue_pop(&temp_queue_head);
        new_packet = fbe_transport_queue_element_to_packet(queue_element);
        fbe_transport_complete_packet(new_packet);
		*/
    }

	fbe_queue_destroy(&temp_queue);

    return FBE_STATUS_OK;
}

/*used by RAID (or any other server) to inform LU (or any other clinet) that a user changed the power saving info
and not it needs to ask for the new information*/
fbe_status_t fbe_block_transport_server_power_saving_config_changed(fbe_block_transport_server_t * block_transport_server)
{
    fbe_base_transport_server_t * base_transport_server = (fbe_base_transport_server_t *) block_transport_server;
    fbe_u32_t i = 0;
    fbe_base_edge_t * base_edge = NULL;
    fbe_block_edge_t * block_edge = NULL;

    fbe_base_transport_server_lock(base_transport_server);

    base_edge = base_transport_server->client_list;
    while(base_edge != NULL) {
        block_edge = (fbe_block_edge_t *)base_edge;
       
        /* We should send event here */
        if(block_edge->server_package_id != block_edge->client_package_id){
            fbe_topology_send_event_to_another_package(base_edge->client_id, 
                                                        block_edge->client_package_id,
                                                        FBE_EVENT_TYPE_POWER_SAVING_CONFIG_CHANGED, 
                                                        base_edge);
        } else {
            fbe_topology_send_event(base_edge->client_id, FBE_EVENT_TYPE_POWER_SAVING_CONFIG_CHANGED, base_edge);
        }
       
        i++;
        if(i > FBE_BLOCK_TRANSPORT_SERVER_MAX_CLIENT_COUNT){
            fbe_base_transport_server_unlock(base_transport_server);
            
            fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR, 
                                     FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                                     "%s Critical error looped client list\n", __FUNCTION__);

            return FBE_STATUS_GENERIC_FAILURE;
        }
        base_edge = base_edge->next_edge;
    }

    fbe_base_transport_server_unlock(base_transport_server);

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * fbe_block_transport_send_block_operation()
 ****************************************************************
 * @brief
 *  This function sends block operations for this raid group object.
 *
 * @param base_object_p - The Base object.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @revision
 *  06/01/2010  Swati Fursule  - Created.
 *
 ****************************************************************/
fbe_status_t 
fbe_block_transport_send_block_operation(fbe_block_transport_server_t *block_transport_server_p,
                                         fbe_packet_t * packet_p,
                                         struct fbe_base_object_s * base_object_p)
{
    fbe_status_t                            status;
    fbe_payload_ex_t*                          payload_p = NULL;
    fbe_payload_block_operation_t*          block_operation_p;
    fbe_payload_block_operation_t*          new_block_operation_p;
    fbe_block_transport_block_operation_info_t*           block_operation_info_p;
    fbe_payload_control_operation_t *       control_operation_p = NULL;
    fbe_payload_control_buffer_length_t     length = 0;
    fbe_sg_element_t *                      sg_list_p = NULL;
    fbe_u32_t                               sg_list_count;
    
    fbe_base_transport_trace(
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry\n", __FUNCTION__);

	payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    if (control_operation_p == NULL) {
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Verify the input buffer. */
    fbe_payload_control_get_buffer(control_operation_p, &block_operation_info_p);
    if (block_operation_info_p == NULL) {
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Verify the length of the input buffer. */
    fbe_payload_control_get_buffer_length (control_operation_p, &length);
    if(length != sizeof(fbe_block_transport_block_operation_info_t)) {
        fbe_payload_control_set_status(control_operation_p, 
                                       FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    block_operation_p = &(block_operation_info_p->block_operation);

    /*  Setup the block operation in the packet. */
    new_block_operation_p = fbe_payload_ex_allocate_block_operation(payload_p);
	
     /* Since fbe_payload_block_build_operation will set the 'check checksum' flag
     * hence manually clear the flag in block operation
     */
    fbe_payload_block_build_operation(new_block_operation_p,
        block_operation_p->block_opcode,
        block_operation_p->lba,
        block_operation_p->block_count,
        block_operation_p->block_size,
        block_operation_p->optimum_block_size,
        block_operation_p->pre_read_descriptor);

    /* The 'Check Checksum' flag is set by default in build operation above.
     * Clear it out and set the flags specified by the caller.
     */
    fbe_payload_block_clear_flag(new_block_operation_p, 0xFFFF);
    fbe_payload_block_set_flag(new_block_operation_p, block_operation_p->block_flags);

    fbe_payload_ex_set_verify_error_count_ptr(payload_p, &(block_operation_info_p->verify_error_counts));
    status = fbe_payload_ex_get_sg_list(payload_p,
                                        &sg_list_p, &sg_list_count);
    status = fbe_payload_ex_set_sg_list(payload_p, 
                                        sg_list_p, sg_list_count);

    /* We use data buffer memory provided in control buffer so caller has to
     * take into account of these two sg elements.
     */
    /* 
     * Set the completion function before issuing I/O request. 
     */
    status = fbe_transport_set_completion_function(packet_p, 
                      fbe_block_transport_block_operation_completion, 
                      base_object_p);

    /* 
     * Increment the block operation before calling bouncer entry. 
     */
    status = fbe_payload_ex_increment_block_operation_level(payload_p);

    if(status != FBE_STATUS_OK)
    {
        fbe_base_transport_trace(
                        FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                        "%s monitor I/O operation failed, status: 0x%X\n",
                        __FUNCTION__, status);

        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Send the block operation. */
    status = fbe_block_transport_server_bouncer_entry(block_transport_server_p, packet_p, base_object_p);
	if(block_transport_server_p->attributes & FBE_BLOCK_TRANSPORT_ENABLE_STACK_LIMIT){
		if(status == FBE_STATUS_OK){
			block_transport_server_p->block_transport_const->block_transport_entry(base_object_p, packet_p);
		}
	}

    if(status == FBE_STATUS_GENERIC_FAILURE){
        fbe_base_transport_trace(
                          FBE_TRACE_LEVEL_ERROR, 
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                          "%s fbe_block_transport_server_bouncer_entry failed!\n", __FUNCTION__);

    }
    return status;
}
/**************************************
 * end fbe_block_transport_send_block_operation()
 **************************************/
/*!**************************************************************
 * fbe_block_transport_block_operation_completion()
 ****************************************************************
 * @brief
 *  This function is used as completion function for the I/O
 *  control operation.
 * 
 * @param packet_p - Pointer to the packet.
 * @param packet_p - Packet completion context.
 *
 * @return status - The status of the operation.
 *
 * @revision
 *  06/01/2010  Swati Fursule  - Created.
 *
 ****************************************************************/
static fbe_status_t 
fbe_block_transport_block_operation_completion(fbe_packet_t * packet_p,
                           fbe_packet_completion_context_t context)
{
    fbe_payload_ex_t *                          payload_p = NULL;
    fbe_payload_control_operation_t *           control_operation_p = NULL;
    fbe_payload_block_operation_t *             block_operation_p = NULL;
    fbe_block_transport_block_operation_info_t * ret_block_operation_info_p = NULL;
    fbe_status_t                                status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_block_operation_status_t        block_operation_status;
    fbe_payload_control_status_t                control_status;
    fbe_payload_control_operation_opcode_t      opcode;
    fbe_raid_verify_error_counts_t*             verify_err_counts_p = NULL;
    fbe_sg_element_t *                          sg_list_p = NULL;
    fbe_u32_t                                   sg_list_count;

    /* Get the payload and control operation of the packet. */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = 
        fbe_payload_ex_get_prev_control_operation(payload_p);
 
    /* Get the input buffer. */
    fbe_payload_control_get_buffer(control_operation_p, &ret_block_operation_info_p);
    fbe_payload_control_get_opcode(control_operation_p, &opcode);

    /* Get the block operation of the packet. */
    block_operation_p = fbe_payload_ex_get_block_operation(payload_p);

    /* Get the status of the update signature. */
    status = fbe_transport_get_status_code(packet_p);
    if (status == FBE_STATUS_OK)
    {
        fbe_payload_block_get_status(block_operation_p, &block_operation_status);
    }

    if ((status != FBE_STATUS_OK) || 
        (block_operation_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS))
    {
        /* On failure update the control status as failed. */
        control_status = FBE_PAYLOAD_CONTROL_STATUS_FAILURE;
    }
    else
    {
        /* On success update the control status as ok. */
        control_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
    }
    /* Complete the packet with status. */
    fbe_transport_set_status(packet_p, status, 0);
    fbe_payload_control_set_status (control_operation_p, control_status);

	fbe_payload_ex_get_verify_error_count_ptr(payload_p, (void **)&verify_err_counts_p);

    /*
     * Setup sglist.
     * Get the sglist from payload and set it
     */
    status = fbe_payload_ex_get_sg_list(payload_p,
                                        &sg_list_p, &sg_list_count);
    status = fbe_payload_ex_set_sg_list(payload_p, 
                                        sg_list_p++, sg_list_count--);

    ret_block_operation_info_p->block_operation.status = block_operation_p->status;
    ret_block_operation_info_p->block_operation.status_qualifier = block_operation_p->status_qualifier;
    ret_block_operation_info_p->verify_error_counts = *verify_err_counts_p;
    /* Save the control operation with the result of block operation.info */
    fbe_payload_control_build_operation(control_operation_p, opcode,
                     (fbe_payload_block_operation_t*)ret_block_operation_info_p,
                     sizeof(fbe_payload_block_operation_t));

	/* Release this block payload since we are done with it. */
    fbe_payload_ex_release_block_operation(payload_p, block_operation_p);
 
    return status;
}
/**************************************
 * end fbe_block_transport_block_operation_completion()
 **************************************/
static fbe_status_t 
block_transport_server_run_queue_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_block_transport_server_t * block_transport_server = NULL;
    block_transport_server = (fbe_block_transport_server_t *)context;

    fbe_block_transport_server_start_packet(block_transport_server, packet, packet->queue_context);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}

fbe_status_t
fbe_block_transport_server_find_next_consumed_lba(fbe_block_transport_server_t * block_transport_server,
                                                  fbe_lba_t lba,
                                                  fbe_lba_t * next_consumed_lba_p)
{
    fbe_base_transport_server_t *   base_transport_server = NULL;
    fbe_base_edge_t *               base_edge = NULL;
    fbe_block_edge_t *              block_edge = NULL;
    fbe_u32_t                       index = 0;

    base_transport_server = (fbe_base_transport_server_t *) block_transport_server;

    /* initialize next consumed lba as invalid. */
    *next_consumed_lba_p = FBE_LBA_INVALID;

    /* acquire the base transport server lock before we look at the client edge. */
    fbe_base_transport_server_lock(base_transport_server);

    base_edge = base_transport_server->client_list;

    while ((base_edge != NULL) && (*next_consumed_lba_p == FBE_LBA_INVALID))
    {
        /* we should send event here */
        block_edge = (fbe_block_edge_t *) base_edge;

        /* Skip dummy edges.
         */
        if (block_edge->base_edge.client_id != FBE_OBJECT_ID_INVALID)
        {
            /* check the lba range and determine whether it belongs to the current edge. */
            if(fbe_block_transport_is_lba_range_overlaps_edge_extent(lba, 0, block_edge)) {
                /* we found the lba range as consumed and so return the same lba as consumed. */
                *next_consumed_lba_p = lba;
                continue;
            } 
            
            if(lba <= block_edge->offset) {
                /* set the next lba as start of next edge extent, */
                *next_consumed_lba_p = block_edge->offset;
                continue;
            } 
        }
                
        index++;

        if (index > FBE_BLOCK_TRANSPORT_SERVER_MAX_CLIENT_COUNT)
        {
            fbe_base_transport_server_unlock(base_transport_server);
            fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                     FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                     "%s Critical error looped client list\n", __FUNCTION__);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* get the next edge pointer. */
        base_edge = base_edge->next_edge;
    }

    fbe_base_transport_server_unlock(base_transport_server);
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_block_transport_server_get_end_of_extent(fbe_block_transport_server_t * block_transport_server,
                                             fbe_lba_t lba,
                                             fbe_block_count_t blocks,
                                             fbe_lba_t * end_lba_p)
{
    fbe_base_transport_server_t *   base_transport_server = NULL;
    fbe_base_edge_t *               base_edge = NULL;
    fbe_block_edge_t *              block_edge = NULL;
    fbe_u32_t                       index = 0;

    base_transport_server = (fbe_base_transport_server_t *) block_transport_server;

    /* initialize next consumed lba as invalid. */
    *end_lba_p = FBE_LBA_INVALID;

    /* acquire the base transport server lock before we look at the client edge. */
    fbe_base_transport_server_lock(base_transport_server);

    base_edge = base_transport_server->client_list;

    while ((base_edge != NULL) && (*end_lba_p == FBE_LBA_INVALID))
    {
        /* we should send event here */
        block_edge = (fbe_block_edge_t *) base_edge;

        /* Skip dummy edges.
         */
        if (block_edge->base_edge.client_id != FBE_OBJECT_ID_INVALID)
        {
            /* check the lba range and determine whether it belongs to the current edge. */
            if (fbe_block_transport_is_lba_range_overlaps_edge_extent(lba, 0, block_edge)) {
                /* we found the lba range as consumed and so return either the end of the
                 * request (blocks) or the end of the edge extent, whichever is smaller. 
                 */
                if ((lba + blocks) > (block_edge->offset + block_edge->capacity))
                {
                    *end_lba_p = block_edge->offset + block_edge->capacity - 1;
                }
                else
                {
                    *end_lba_p = lba + blocks - 1;
                }
                fbe_base_transport_server_unlock(base_transport_server);
               return FBE_STATUS_OK;
            }
        }
                
        index++;

        if (index > FBE_BLOCK_TRANSPORT_SERVER_MAX_CLIENT_COUNT)
        {
            fbe_base_transport_server_unlock(base_transport_server);
            fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                     FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                     "%s Critical error looped client list\n", __FUNCTION__);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* get the next edge pointer. */
        base_edge = base_edge->next_edge;
    }

    fbe_base_transport_server_unlock(base_transport_server);
    return FBE_STATUS_OK;
}

fbe_status_t fbe_block_transport_server_find_last_consumed_lba(fbe_block_transport_server_t * block_transport_server,
                                                               fbe_lba_t *last_consumed_lba_p)
{
    fbe_lba_t                       lba;
    fbe_base_transport_server_t *   base_transport_server = NULL;
    fbe_base_edge_t *               base_edge = NULL;
    fbe_block_edge_t *              block_edge = NULL;
    fbe_u32_t                       index = 0;

    base_transport_server = (fbe_base_transport_server_t *) block_transport_server;

    /* initialize last consumed lba as invalid. */
    *last_consumed_lba_p = FBE_LBA_INVALID;

    /* acquire the base transport server lock before we look at the client edge. */
    fbe_base_transport_server_lock(base_transport_server);

    base_edge = base_transport_server->client_list;

    while (base_edge != NULL)
    {
        /* we should send event here */
        block_edge = (fbe_block_edge_t *) base_edge;

        /* Skip dummy edges.
         */
        if (block_edge->base_edge.client_id != FBE_OBJECT_ID_INVALID)
        {
            /* force the lab to be beyond extent */
            lba = block_edge->offset + block_edge->capacity;
            /* check the lba range and determine whether it belongs to the current edge. */
            if (fbe_block_transport_is_lba_above_edge_extent(lba, block_edge)) {
                /* we found the lba above the extent.  set the last consumed to the end */
                *last_consumed_lba_p = block_edge->offset + block_edge->capacity - 1;
            } 
        }
                
        index++;

        if (index > FBE_BLOCK_TRANSPORT_SERVER_MAX_CLIENT_COUNT)
        {
            fbe_base_transport_server_unlock(base_transport_server);
            fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                     FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                     "%s Critical error looped client list\n", __FUNCTION__);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* get the next edge pointer. */
        base_edge = base_edge->next_edge;
    }

    fbe_base_transport_server_unlock(base_transport_server);
    return FBE_STATUS_OK;
}
fbe_status_t fbe_block_transport_server_find_max_consumed_lba(fbe_block_transport_server_t * block_transport_server,
                                                              fbe_lba_t *max_consumed_lba_p)
{
    fbe_lba_t                       max_lba = 0;
    fbe_base_transport_server_t *   base_transport_server = NULL;
    fbe_base_edge_t *               base_edge = NULL;
    fbe_block_edge_t *              block_edge = NULL;
    fbe_u32_t                       index = 0;
    fbe_lba_t                       block_edge_offset;

    base_transport_server = (fbe_base_transport_server_t *) block_transport_server;

    /* initialize last consumed lba as invalid. */
    *max_consumed_lba_p = FBE_LBA_INVALID;

    /* acquire the base transport server lock before we look at the client edge. */
    fbe_base_transport_server_lock(base_transport_server);

    base_edge = base_transport_server->client_list;
    while (base_edge != NULL) {
        block_edge = (fbe_block_edge_t *) base_edge;
        /* Skip dummy edges.
         */
        if (block_edge->base_edge.client_id != FBE_OBJECT_ID_INVALID){
            block_edge_offset = (block_edge->base_edge.path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_IGNORE_OFFSET) ? 0 : block_edge->offset;
            if ((block_edge_offset + block_edge->capacity) > max_lba) {
                max_lba = block_edge_offset + block_edge->capacity - 1;
            }
        }
        index++;

        if (index > FBE_BLOCK_TRANSPORT_SERVER_MAX_CLIENT_COUNT) {
            fbe_base_transport_server_unlock(base_transport_server);
            fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                     FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                     "%s Critical error looped client list\n", __FUNCTION__);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        base_edge = base_edge->next_edge;
    }
    if (max_lba > 0) {
        *max_consumed_lba_p = max_lba;
    }
    fbe_base_transport_server_unlock(base_transport_server);
    return FBE_STATUS_OK;
}
/*!**************************************************************
 * fbe_block_transport_server_clear_path_attr()
 ****************************************************************
 * @brief
 *  This function clears the path attribute for the edge in this block transprot
 *  server with the given input server index.
 *
 * @param block_transport_server - The block transport server.
 * @param server_index - The server index is in the edge.
 *                       We will search for this server index in
 *                       the list of client edges and then clear the path
 *                       attribute.
 * @param path_attr - The attributes to clear in this server index's edge.
 *
 * @return fbe_status_t   
 *
 ****************************************************************/
fbe_status_t
fbe_block_transport_server_clear_path_attr(fbe_block_transport_server_t * block_transport_server,
                                             fbe_edge_index_t  server_index, 
                                             fbe_path_attr_t path_attr)
{   fbe_u32_t i = 0;
 fbe_base_edge_t * base_edge = NULL;
 fbe_block_edge_t *block_edge = NULL;

 fbe_block_transport_server_lock(block_transport_server);

 base_edge = block_transport_server->base_transport_server.client_list;
 while(base_edge != NULL) {
     if((base_edge->client_id != FBE_OBJECT_ID_INVALID) &&
        (base_edge->server_index == server_index)){
         if (base_edge->path_attr & path_attr) {
             base_edge->path_attr &= ~path_attr;
             block_edge = (fbe_block_edge_t*)base_edge;
             if(block_edge->server_package_id != block_edge->client_package_id){
                 fbe_topology_send_event_to_another_package(base_edge->client_id, 
                                                            block_edge->client_package_id,
                                                            FBE_EVENT_TYPE_ATTRIBUTE_CHANGED, 
                                                            base_edge);
             } else {
                 fbe_topology_send_event(base_edge->client_id, FBE_EVENT_TYPE_ATTRIBUTE_CHANGED, base_edge);
             }
         }
         break;
     }
     i++;
     if(i > 0x0000FFFF){
         fbe_block_transport_server_unlock(block_transport_server);
         /* KvTrace("%s Critical error looped client list\n", __FUNCTION__); */
         fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR, 
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                                  "%s Critical error looped client list\n", __FUNCTION__);

         return FBE_STATUS_GENERIC_FAILURE;
     }
     base_edge = base_edge->next_edge;
 }

 fbe_block_transport_server_unlock(block_transport_server);

 if(base_edge == NULL){ /*We did not found the edge */
     /* KvTrace("%s Critical error edge not found index %d\n", __FUNCTION__, server_index); */
     fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Error edge not found, server_index = %d\n", __FUNCTION__, server_index);

     return FBE_STATUS_GENERIC_FAILURE;
 }

 return FBE_STATUS_OK;
}

/*!**************************************************************
 * fbe_block_transport_server_clear_path_attr_all_servers()
 ****************************************************************
 * @brief
 *  This function clears the path attribute for all edges in this block transport
 *  server.
 *
 * @param block_transport_server - The block transport server.
 * @param path_attr - The attributes to be cleared.
 *
 * @return fbe_status_t   
 *
 ****************************************************************/
fbe_status_t
fbe_block_transport_server_clear_path_attr_all_servers(fbe_block_transport_server_t* block_transport_server,
                                                       fbe_path_attr_t path_attr)
{	fbe_u32_t i = 0;
	fbe_base_edge_t * base_edge = NULL;
    fbe_block_edge_t *block_edge = NULL;

	fbe_block_transport_server_lock(block_transport_server);

	base_edge = block_transport_server->base_transport_server.client_list;

    while(base_edge != NULL) {
		if((base_edge->client_id != FBE_OBJECT_ID_INVALID) && (base_edge->path_attr & path_attr)){
			base_edge->path_attr &= ~path_attr;
            block_edge = (fbe_block_edge_t*)base_edge;

            fbe_base_transport_trace(FBE_TRACE_LEVEL_INFO,
                                     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                     "%s clear attributes 0x%x, server_index = %d\n", 
                                     __FUNCTION__, 
                                     path_attr,
                                     base_edge->server_index);

            if(block_edge->server_package_id != block_edge->client_package_id){
                fbe_topology_send_event_to_another_package(base_edge->client_id, 
                                                            block_edge->client_package_id,
                                                            FBE_EVENT_TYPE_ATTRIBUTE_CHANGED, 
                                                            base_edge);
            } else {
                fbe_topology_send_event(base_edge->client_id, FBE_EVENT_TYPE_ATTRIBUTE_CHANGED, base_edge);
            }
			
		}
		i++;
		if(i > 0x0000FFFF){
			fbe_block_transport_server_unlock(block_transport_server);
			fbe_base_transport_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
									FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
									"%s Critical error looped client list\n", __FUNCTION__);

			return FBE_STATUS_GENERIC_FAILURE;
		}
		base_edge = base_edge->next_edge;
	}

	if(base_edge == NULL){  /*We did not found an edge */
		fbe_base_transport_trace(FBE_TRACE_LEVEL_INFO,
								 FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
								 "%s no edges found\n", __FUNCTION__);
	}

	fbe_block_transport_server_unlock(block_transport_server);

	return FBE_STATUS_OK;
}

/*!**************************************************************
 * fbe_block_transport_server_propogate_path_attr_all_servers()
 ****************************************************************
 * @brief
 *  This function propogates the path attribute for all edges in this block transport
 *  server.
 *
 * @param block_transport_server - The block transport server.
 * @param path_attr - The attributes to be propogated.
 *
 * @return fbe_status_t   
 *
 ****************************************************************/
fbe_status_t
fbe_block_transport_server_propogate_path_attr_all_servers(fbe_block_transport_server_t* block_transport_server,
                                                           fbe_path_attr_t path_attr)
{	fbe_u32_t i = 0;
	fbe_base_edge_t * base_edge = NULL;
    fbe_block_edge_t *block_edge = NULL;

	fbe_block_transport_server_lock(block_transport_server);

	base_edge = block_transport_server->base_transport_server.client_list;

    while(base_edge != NULL) {
		if(base_edge->client_id != FBE_OBJECT_ID_INVALID){
            base_edge->path_attr = path_attr;
            block_edge = (fbe_block_edge_t*)base_edge;

            fbe_base_transport_trace(FBE_TRACE_LEVEL_INFO,
                                     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                     "blk_transport_srv_propogate_pathattr  attr 0x%x, srv_idx = %d\n", 
                                     path_attr,
                                     base_edge->server_index);

            if(block_edge->server_package_id != block_edge->client_package_id){
                fbe_topology_send_event_to_another_package(base_edge->client_id, 
                                                            block_edge->client_package_id,
                                                            FBE_EVENT_TYPE_ATTRIBUTE_CHANGED, 
                                                            base_edge);
            } else {
                fbe_topology_send_event(base_edge->client_id, FBE_EVENT_TYPE_ATTRIBUTE_CHANGED, base_edge);
            }
			
		}
		i++;
		if(i > 0x0000FFFF){
			fbe_block_transport_server_unlock(block_transport_server);
			fbe_base_transport_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
									FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
									"blk_transport_srv_propogate_pathattr Critical err looped client list\n");

			return FBE_STATUS_GENERIC_FAILURE;
		}
		base_edge = base_edge->next_edge;
	}

	if(i == 0){  /*We did not found an edge */
		fbe_base_transport_trace(FBE_TRACE_LEVEL_INFO,
								 FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
								 "blk_transport_srv_propogate_pathattr no edges found\n");
	}

	fbe_block_transport_server_unlock(block_transport_server);

	return FBE_STATUS_OK;
}

/*!**************************************************************
 * fbe_block_transport_server_propogate_path_attr_stop_with_error()
 ****************************************************************
 * @brief
 *  This function propogates the path attribute for all edges in this block transport
 *  server and it will exit if encountered anything other than FBE_STATUS_OK.
 *
 * @param block_transport_server - The block transport server.
 * @param path_attr - The attributes to be propogated.
 *
 * @return fbe_status_t   
 *
 ****************************************************************/
fbe_status_t
fbe_block_transport_server_propogate_path_attr_stop_with_error(fbe_block_transport_server_t* block_transport_server,
                                                               fbe_path_attr_t path_attr)
{	
    fbe_u32_t i = 0;
	fbe_base_edge_t * base_edge = NULL;
    fbe_block_edge_t *block_edge = NULL;
    fbe_status_t    status = FBE_STATUS_OK;

	fbe_block_transport_server_lock(block_transport_server);

	base_edge = block_transport_server->base_transport_server.client_list;

    while(base_edge != NULL) {
		if(base_edge->client_id != FBE_OBJECT_ID_INVALID){
            base_edge->path_attr |= path_attr;
            block_edge = (fbe_block_edge_t*)base_edge;

            if(block_edge->server_package_id != block_edge->client_package_id){
                status = fbe_topology_send_event_to_another_package(base_edge->client_id, 
                                                                    block_edge->client_package_id,
                                                                    FBE_EVENT_TYPE_ATTRIBUTE_CHANGED, 
                                                                    base_edge);
            } else {
                status = fbe_topology_send_event(base_edge->client_id, FBE_EVENT_TYPE_ATTRIBUTE_CHANGED, base_edge);
            }
            if (status != FBE_STATUS_OK)
            {
                break;
            }
		}
		i++;
		if(i > 0x0000FFFF){
			fbe_block_transport_server_unlock(block_transport_server);
			fbe_base_transport_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
									FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
									"%s Critical error looped client list\n", __FUNCTION__);

			return FBE_STATUS_GENERIC_FAILURE;
		}
		base_edge = base_edge->next_edge;
	}

	fbe_block_transport_server_unlock(block_transport_server);

	return status;
}

/*!**************************************************************
 * fbe_block_transport_server_propagate_path_attr_with_mask_all_servers()
 ****************************************************************
 * @brief
 *  This function sets all path attributes within the mask for all 
 *  edges in this block transport server.
 *
 * @param block_transport_server - The block transport server.
 * @param path_attr - The attributes to be set.
 * @param mask - The mask.
 *
 * @return fbe_status_t   
 *
 ****************************************************************/
fbe_status_t
fbe_block_transport_server_propagate_path_attr_with_mask_all_servers(fbe_block_transport_server_t * block_transport_server,
                                                                     fbe_path_attr_t path_attr,
                                                                     fbe_path_attr_t mask)
{
    fbe_base_transport_server_t * base_transport_server = (fbe_base_transport_server_t *)block_transport_server;
    fbe_u32_t i = 0;
    fbe_base_edge_t * base_edge = NULL;
    fbe_block_edge_t * block_edge = NULL;

    fbe_base_transport_server_lock(base_transport_server);

    /* Get a RG edge, i.e. an edge to a LUN */
    base_edge = base_transport_server->client_list;
    while(base_edge != NULL) {
        if (base_edge->client_id != FBE_OBJECT_ID_INVALID){
            if ((base_edge->path_attr & mask) != path_attr)
            {
                /* Set attributes */
                path_attr &= mask;
                base_edge->path_attr &= ~mask;
                base_edge->path_attr |= path_attr;
                /* We should send event here */
                block_edge = (fbe_block_edge_t *)base_edge;
            
                fbe_base_transport_trace(FBE_TRACE_LEVEL_INFO,
                                         FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                         "block_trans_srv_prop_attr_mask_all_srvs set attributes 0x%x, server_index:%d, client:0x%x, server:0x%x\n", 
                                         path_attr, base_edge->server_index, base_edge->client_id, base_edge->server_id);

                if(block_edge->server_package_id != block_edge->client_package_id){
                    fbe_topology_send_event_to_another_package(base_edge->client_id, 
                                                               block_edge->client_package_id,
                                                               FBE_EVENT_TYPE_ATTRIBUTE_CHANGED, 
                                                               base_edge);
                } else {
                    fbe_topology_send_event(base_edge->client_id, FBE_EVENT_TYPE_ATTRIBUTE_CHANGED, base_edge);
                }
            }
        }
        i++;
        if(i > FBE_BLOCK_TRANSPORT_SERVER_MAX_CLIENT_COUNT){
            fbe_base_transport_server_unlock(base_transport_server);
            fbe_base_transport_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "block_trans_srv_set_path_attr_all_srvs Critical error looped client list\n");

            return FBE_STATUS_GENERIC_FAILURE;
        }
        base_edge = base_edge->next_edge;
    }

    fbe_base_transport_server_unlock(base_transport_server);

    return FBE_STATUS_OK;
}


static fbe_status_t 
fbe_block_transport_server_prepare_packet(fbe_block_transport_server_t * block_transport_server,
                                        fbe_packet_t * packet,
                                        fbe_transport_entry_context_t context)
{
//    fbe_status_t status = FBE_STATUS_OK;
    fbe_block_edge_t *block_edge = NULL;

    fbe_payload_ex_t * payload = NULL;
    fbe_payload_block_operation_t *block_operation_orig = NULL;
    fbe_payload_block_operation_t *block_operation_new = NULL;
    fbe_package_id_t package_id;
    fbe_path_attr_t     path_attr = 0;

    block_edge = (fbe_block_edge_t *)packet->base_edge;
    if(block_edge != NULL) 
    {
        fbe_base_transport_get_path_attributes(packet->base_edge, &path_attr);
    }

    fbe_transport_get_package_id(packet, &package_id);
    if (package_id == FBE_PACKAGE_ID_PHYSICAL){
        fbe_transport_set_completion_function(packet, block_transport_server_bouncer_completion, block_transport_server);
        //status = block_transport_server->block_transport_const->block_transport_entry(context, packet);
        return FBE_STATUS_OK;
    }

    /* If we have an edge offset, then apply the offset to the block operation.
     *  
     * Check the edge type, since there are times when a packet gets sent over other edge 
     * types and the transport ID changes (e.g. ssp) and on completion we cannot do the 
     * offset handling if the edge is not a block transport edge. 
     *
     * Due to the fact that the edge between the raid group and virtual drive must
     * have the total offset, the offset between the virtual drive and provision drive
     * should not be added (it would result in the offset being added twice).  The flag
     * `FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_IGNORE_OFFSET' prevents added the offset twice.
     */
    /*! @todo checking offset is a temparory hack.  When block operation gets increase to 3, 
     *        checking for offset should be removed.  
     */
    if ((block_edge != NULL)                                            &&
        (block_edge->base_edge.transport_id == FBE_TRANSPORT_ID_BLOCK)  &&
        (block_edge->offset > 0)                                        && 
        (!(path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_IGNORE_OFFSET))    )
    {
        /* Allocate the new payload, and increment the level to make this new level 
         * active. 
         */
        payload = fbe_transport_get_payload_ex(packet);
        block_operation_orig = fbe_payload_ex_get_block_operation(payload);
        block_operation_new = fbe_payload_ex_allocate_block_operation(payload);
        fbe_payload_ex_increment_block_operation_level(payload);

        /* Copy the prior operation into the new operation.
         * And apply edge offset to the operation.
         */
        block_operation_new->lba = block_operation_orig->lba + block_edge->offset;
        block_operation_new->block_count = block_operation_orig->block_count;
        block_operation_new->block_opcode = block_operation_orig->block_opcode;
        block_operation_new->block_flags = block_operation_orig->block_flags;
        block_operation_new->block_size = block_operation_orig->block_size;
        block_operation_new->optimum_block_size = block_operation_orig->optimum_block_size;
        block_operation_new->payload_sg_descriptor = block_operation_orig->payload_sg_descriptor;
        block_operation_new->pre_read_descriptor = block_operation_orig->pre_read_descriptor;
        //block_operation_new->verify_error_counts_p = block_operation_orig->verify_error_counts_p;
        if (block_operation_orig->pre_read_descriptor != NULL)
        {
            block_operation_orig->pre_read_descriptor->lba += block_edge->offset;
        }
        fbe_payload_block_set_block_edge(block_operation_new, block_edge);

        /* Use a different completion function.
         */
        fbe_transport_set_completion_function(packet, 
                                              block_transport_server_bouncer_completion_with_offset, 
                                              block_transport_server);
    }
    else
    {
        fbe_transport_set_completion_function(packet, 
                                              block_transport_server_bouncer_completion, 
                                              block_transport_server);
    }
    //status = block_transport_server->block_transport_const->block_transport_entry(context, packet);
    return FBE_STATUS_OK;
}


fbe_status_t
fbe_block_transport_server_set_stack_limit(fbe_block_transport_server_t * block_transport_server)
{
    block_transport_server_set_attributes(block_transport_server,
                                          FBE_BLOCK_TRANSPORT_ENABLE_STACK_LIMIT);
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_block_transport_is_empty(fbe_block_transport_server_t * block_transport_server, fbe_bool_t * is_empty)
{
    fbe_spinlock_lock(&block_transport_server->queue_lock);
	if((fbe_u32_t)(block_transport_server->outstanding_io_count & FBE_BLOCK_TRANSPORT_SERVER_GATE_MASK) == 0){
		* is_empty = FBE_TRUE;
	} else {
        * is_empty = FBE_FALSE;
    }
    fbe_spinlock_unlock(&block_transport_server->queue_lock);
	return FBE_STATUS_OK;
}

fbe_status_t fbe_block_transport_server_set_default_offset(fbe_block_transport_server_t * block_transport_server, fbe_lba_t default_offset)
{
	block_transport_server->default_offset = default_offset;
	return FBE_STATUS_OK;
}

fbe_lba_t fbe_block_transport_server_get_default_offset(fbe_block_transport_server_t * block_transport_server)
{
	return block_transport_server->default_offset;
}

/*!**************************************************************
 * @fn fbe_block_transport_drain_event_queue
 ****************************************************************
 * @brief
 *  This function determines if there is any event which belongs to given block
 *  transport server and pending to process then return pending status to the caller.
 *
 * @param in_block_transport_server_p - Pointer to block transport server
 *
 * @return fbe_lifecycle_status_t - lifecycle state based on queue status
 *
 ****************************************************************/
fbe_lifecycle_status_t
fbe_block_transport_drain_event_queue(fbe_block_transport_server_t * block_transport_server)
{

    fbe_u32_t  pend_event_count = 0;

    /* close the gate so that no any more event gets queued while destoy */
    block_transport_server_set_attributes(block_transport_server,
                                          FBE_BLOCK_TRANSPORT_FLAGS_COMPLETE_EVENTS_ON_DESTROY);

    /* get number of events associate with this block transport from event service queue */
    fbe_event_service_get_block_transport_event_count(&pend_event_count , block_transport_server);

    /* if there is any event in queue then we need to wait until it gets processed
     */
    if( pend_event_count >0 )
    {
        fbe_base_transport_trace(FBE_TRACE_LEVEL_INFO,
                                                FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                                "%s  pend block transport server 0x%p, events in queue %d \n", __FUNCTION__, block_transport_server, pend_event_count);
        return FBE_LIFECYCLE_STATUS_PENDING;
    }

    return FBE_LIFECYCLE_STATUS_DONE;
}


/*this function returns the total capacity of all the edges connected to the server.
We use it for example on a VD to see how much the RG is consuming on a PVD*/
fbe_status_t fbe_block_transport_server_get_minimum_capacity_required(fbe_block_transport_server_t * block_transport_server,
                                                                      fbe_lba_t *exported_offset,
                                                                      fbe_lba_t *minimum_capacity_required)
{
    fbe_base_transport_server_t *base_transport_server = (fbe_base_transport_server_t *)block_transport_server;
    fbe_u32_t                   i = 0;
    fbe_base_edge_t            *base_edge = NULL;
    fbe_block_edge_t           *block_edge = NULL;
    fbe_lba_t                   maximum_offset = 0;
    fbe_lba_t                   cur_offset = 0;
    fbe_lba_t                   cur_edge_end = 0;
    fbe_lba_t                   already_consumed_capacity = 0;

	*minimum_capacity_required = FBE_LBA_INVALID;

    fbe_base_transport_server_lock(base_transport_server);

    /* Get a RG edge, i.e. an edge to a LUN */
    base_edge = base_transport_server->client_list;
    while(base_edge != NULL) 
    {
        if (base_edge->client_id != FBE_OBJECT_ID_INVALID)
        {
            block_edge = (fbe_block_edge_t *)base_edge;

            cur_offset = block_edge->offset;
            cur_edge_end = block_edge->offset + block_edge->capacity;
            if(block_edge->base_edge.path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_IGNORE_OFFSET)
            {
                /*ignore the offset when calc edge end if the attribute is set*/
                cur_edge_end = block_edge->capacity;
            }
            
            if (cur_offset > maximum_offset)
            {
                maximum_offset = cur_offset;
            }

            if(cur_edge_end > already_consumed_capacity)
            {
                already_consumed_capacity = cur_edge_end;
            }
            
        }
        i++;
        if(i > FBE_BLOCK_TRANSPORT_SERVER_MAX_CLIENT_COUNT)
        {
            fbe_base_transport_server_unlock(base_transport_server);
            fbe_base_transport_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "block_trans_srv_set_path_attr_all_srvs Critical error looped client list\n");

            return FBE_STATUS_GENERIC_FAILURE;
        }
        base_edge = base_edge->next_edge;
    }

    fbe_base_transport_server_unlock(base_transport_server);

    /* If we didn't locate a valid client it is not a failure since it is
     * perfectly valid to call this method with no upstream objects.
     */
    if (already_consumed_capacity == 0)
    {
        fbe_base_transport_trace(FBE_TRACE_LEVEL_DEBUG_HIGH,
                                 FBE_TRACE_MESSAGE_ID_INFO,
                                 "%s No client for %p \n",
                                 __FUNCTION__, base_transport_server);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Populate the output and return success.
     */
    *exported_offset = maximum_offset;
    *minimum_capacity_required = already_consumed_capacity;
    return FBE_STATUS_OK;
}
void fbe_block_transport_server_get_throttle_info(fbe_block_transport_server_t * block_transport_server,
                                                  fbe_block_transport_get_throttle_info_t *get_throttle_info)
{
    fbe_packet_priority_t priority;
    fbe_u32_t index;
    get_throttle_info->io_throttle_count = block_transport_server->io_throttle_count;
    get_throttle_info->io_throttle_max = block_transport_server->io_throttle_max;
    get_throttle_info->outstanding_io_count = block_transport_server->outstanding_io_count;
    get_throttle_info->outstanding_io_max = block_transport_server->outstanding_io_max;
    get_throttle_info->io_credits_max = block_transport_server->io_credits_max;
    get_throttle_info->outstanding_io_credits = block_transport_server->outstanding_io_credits;
    
    fbe_spinlock_lock(&block_transport_server->queue_lock);
    for(priority = FBE_PACKET_PRIORITY_LAST - 1; priority > FBE_PACKET_PRIORITY_INVALID; priority--)
    {
        index = priority - 1;
        get_throttle_info->queue_length[index] = fbe_queue_length(&block_transport_server->queue_head[index]);
    }
    fbe_spinlock_unlock(&block_transport_server->queue_lock);
}

fbe_status_t fbe_block_transport_server_set_throttle_info(fbe_block_transport_server_t * block_transport_server,
                                                          fbe_block_transport_set_throttle_info_t *set_throttle_info)
{
    fbe_spinlock_lock(&block_transport_server->queue_lock);
    block_transport_server->io_throttle_max = set_throttle_info->io_throttle_max;
    block_transport_server->outstanding_io_max = set_throttle_info->outstanding_io_max;
    block_transport_server->io_credits_max = set_throttle_info->io_credits_max;
    fbe_spinlock_unlock(&block_transport_server->queue_lock);
    return FBE_STATUS_OK;
}
void fbe_block_transport_server_abort_monitor_operations(fbe_block_transport_server_t * block_transport_server,
                                                         fbe_object_id_t object_id)
{
    fbe_status_t status;
    fbe_packet_t * new_packet_p = NULL;
    fbe_packet_priority_t priority;
    fbe_u32_t index;
	fbe_queue_element_t * queue_element_p = NULL;
	fbe_queue_element_t * next_queue_element_p = NULL;
    fbe_queue_head_t tmp_queue;

    fbe_queue_init(&tmp_queue);

    fbe_spinlock_lock(&block_transport_server->queue_lock);
    for(priority = FBE_PACKET_PRIORITY_LAST - 1; priority > FBE_PACKET_PRIORITY_INVALID; priority--)
    {
        index = priority - 1;
        queue_element_p = fbe_queue_front(&block_transport_server->queue_head[index]);
        while(queue_element_p != NULL) {
            next_queue_element_p = fbe_queue_next(&block_transport_server->queue_head[index],
                                                  queue_element_p);
            new_packet_p = fbe_transport_queue_element_to_packet(queue_element_p);

            /* Remove the monitor packet from the queue, give it a bad status. 
             */
            if (fbe_transport_is_monitor_packet(new_packet_p, object_id)){
                status = fbe_transport_remove_packet_from_queue(new_packet_p);
                fbe_transport_set_status(new_packet_p, FBE_STATUS_FAILED, __LINE__);
                fbe_base_transport_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                         "BTS: monitor op aborted packet: %p object_id: 0x%x\n",
                                         new_packet_p, object_id);
                fbe_queue_push(&tmp_queue, &new_packet_p->queue_element);
            }
            queue_element_p = next_queue_element_p;
        }
    }
    fbe_spinlock_unlock(&block_transport_server->queue_lock);

    /* Put all the packets from tmp_queue to transport run queue */
    if(!fbe_queue_is_empty(&tmp_queue)) {
        fbe_transport_run_queue_push(&tmp_queue, NULL, NULL);
    }

    fbe_queue_destroy(&tmp_queue);
}

/*!**************************************************************
 * fbe_block_transport_server_get_available_io_credits()
 ****************************************************************
 * @brief
 *  Determine how many io credits are remaining to be used
 *  on the block transport server.
 *  This checks if a given request is likely to be able to
 *  get started.
 *
 * @param base_config
 * @param available_io_credits_p - The number of io credits
 *                                 unused on the block transport server.
 *
 * @return None.
 *
 * @author
 *  4/18/2013 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_block_transport_server_get_available_io_credits(fbe_block_transport_server_t * bts, fbe_u32_t *available_io_credits_p)
{
    /* We want to make an intelligent decision and can't afford to have these values changing.
     */
    fbe_spinlock_lock(&bts->queue_lock);
    if (bts->io_credits_max == 0){
        /* Not throttling, infinite unused.
         */
        *available_io_credits_p = FBE_U32_MAX;
    }
    else if (bts->outstanding_io_credits >= bts->io_credits_max){
        /* Already fully subscribed.
         */
        *available_io_credits_p = 0;
    }
    else {
        /* Determine how much is left between max credits and outstanding credits.
         */
        *available_io_credits_p = bts->io_credits_max - bts->outstanding_io_credits;
    }
    fbe_spinlock_unlock(&bts->queue_lock);
}
/******************************************
 * end fbe_block_transport_server_get_available_io_credits()
 ******************************************/    
        	   
/*!**************************************************************
 * @fn fbe_block_transport_server_set_queue_ratio_addend
 ****************************************************************
 * @brief
 *  Set the ratio on the normal priority queue processing
 *
 * @param normal_queue_ratio_addend - the additional number of elements to process in the normal queue
 *
 * @return   
 *
 ****************************************************************/
void fbe_block_transport_server_set_queue_ratio_addend(fbe_u8_t normal_queue_ratio_addend)
{
    fbe_block_transport_server_queue_ratio_addend = normal_queue_ratio_addend;
}

fbe_u8_t fbe_block_transport_server_get_queue_ratio_addend()
{
    return fbe_block_transport_server_queue_ratio_addend;
}

fbe_status_t
fbe_block_transport_server_get_server_index_for_lba(fbe_block_transport_server_t * block_transport_server,
                                                    fbe_lba_t lba,
                                                    fbe_edge_index_t * server_index_p)
{
    fbe_base_transport_server_t *   base_transport_server = NULL;
    fbe_base_edge_t *               base_edge = NULL;
    fbe_block_edge_t *              block_edge = NULL;
    fbe_u32_t                       index = 0;

    base_transport_server = (fbe_base_transport_server_t *) block_transport_server;

    /* acquire the base transport server lock before we look at the client edge. */
    fbe_base_transport_server_lock(base_transport_server);

    /* Default to returning INVALID if no edge is found.  */
    *server_index_p = FBE_EDGE_INDEX_INVALID;
    base_edge = base_transport_server->client_list;

    while (base_edge != NULL) {
        block_edge = (fbe_block_edge_t *) base_edge;

        /* Skip dummy edges. */
        if (block_edge->base_edge.client_id != FBE_OBJECT_ID_INVALID) {
            /* check the lba range and determine whether it belongs to the current edge. */
            if (fbe_block_transport_is_lba_range_overlaps_edge_extent(lba, 0, block_edge)) {
                *server_index_p = block_edge->base_edge.server_index;
                fbe_base_transport_server_unlock(base_transport_server);
                return FBE_STATUS_OK;
            }
        }

        index++;

        if (index > FBE_BLOCK_TRANSPORT_SERVER_MAX_CLIENT_COUNT) {
            fbe_base_transport_server_unlock(base_transport_server);
            fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                     FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                     "%s Critical error looped client list\n", __FUNCTION__);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* get the next edge pointer. */
        base_edge = base_edge->next_edge;
    }

    fbe_base_transport_server_unlock(base_transport_server);
    return FBE_STATUS_OK;
}

static fbe_lifecycle_status_t
fbe_block_transport_server_pending_done(fbe_block_transport_server_t * block_transport_server,
                                        fbe_lifecycle_const_t * p_class_const,
                                        struct fbe_base_object_s * base_object,
                                        fbe_block_path_attr_flags_t attribute_exception)
{
	fbe_status_t status;
	fbe_lifecycle_state_t lifecycle_state;
	fbe_lifecycle_status_t lifecycle_status;
	fbe_u32_t number_of_clients;

	status = fbe_lifecycle_get_state(p_class_const, base_object, &lifecycle_state);
	if(status != FBE_STATUS_OK){
		/* KvTrace("%s Critical error fbe_lifecycle_get_state failed, status = %X \n", __FUNCTION__, status); */
		fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR, 
                                 FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                                 "%s Critical error fbe_lifecycle_get_state failed, status = %X", __FUNCTION__, status);
		/* We have no error code to return */
	}

	lifecycle_status = FBE_LIFECYCLE_STATUS_DONE;

    fbe_block_transport_server_update_path_state(block_transport_server,
                                                 p_class_const,
                                                 base_object,
                                                 attribute_exception);

	if(lifecycle_state == FBE_LIFECYCLE_STATE_PENDING_DESTROY){
		/* we need to make shure that all edges are detached */
		fbe_base_transport_server_get_number_of_clients((fbe_base_transport_server_t*)block_transport_server, &number_of_clients);
		if(number_of_clients != 0){
			lifecycle_status = FBE_LIFECYCLE_STATUS_PENDING;
		}
	}

	return lifecycle_status;
}
/*!**************************************************************
 * fbe_block_transport_align_io()
 ****************************************************************
 * @brief
 *  Align an I/O's lba and block count to a passed in number of blocks.
 *
 * @param alignment_blocks
 * @param lba_p
 * @param blocks_p
 * 
 * @return fbe_status_t
 *
 * @author
 *  3/27/2014 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_block_transport_align_io(fbe_block_size_t alignment_blocks,
                                          fbe_lba_t *lba_p,
                                          fbe_block_count_t *blocks_p)
{   
    fbe_lba_t lba = *lba_p;
    fbe_lba_t end_lba = *lba_p + *blocks_p - 1;
    fbe_block_count_t blocks;

    lba = (lba / alignment_blocks) * alignment_blocks;
    blocks = end_lba - lba + 1;
    if ((lba + blocks) % alignment_blocks) {
        end_lba += (alignment_blocks - ((lba + blocks) % alignment_blocks));
        blocks = end_lba - lba + 1;
    }

    if ( (*lba_p != lba) || (*blocks_p != blocks)) {
        *lba_p = lba;
        *blocks_p = blocks;
    }
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_block_transport_align_io()
 ******************************************/
/*!**************************************************************
 * fbe_block_transport_server_set_block_size()
 ****************************************************************
 * @brief
 *  This function propogates the geometry for all edges in this block transport
 *  server.
 *
 * @param block_transport_server - The block transport server.
 * @param block_edge_geometry - The geometry to propogate upstream
 *
 * @return fbe_status_t   
 *
 ****************************************************************/
fbe_status_t
fbe_block_transport_server_set_block_size(fbe_block_transport_server_t* block_transport_server,
                                          fbe_block_edge_geometry_t block_edge_geometry)
{	fbe_u32_t i = 0;
	fbe_base_edge_t * base_edge = NULL;
    fbe_block_edge_t *block_edge = NULL;

	fbe_block_transport_server_lock(block_transport_server);

	base_edge = block_transport_server->base_transport_server.client_list;

    while(base_edge != NULL) {
        block_edge = (fbe_block_edge_t*)base_edge;
		if ((base_edge->client_id != FBE_OBJECT_ID_INVALID) &&
            (block_edge->block_edge_geometry != block_edge_geometry)){
            fbe_base_transport_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                     "Block size changed for object 0x%x old_size: 0x%x new_size: 0x%x\n",
                                     block_edge->base_edge.server_id, block_edge->block_edge_geometry, block_edge_geometry);
            block_edge->block_edge_geometry = block_edge_geometry;
        }
		i++;
		if(i > 0x0000FFFF){
			fbe_block_transport_server_unlock(block_transport_server);
			fbe_base_transport_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                     "blk_transport_srv_propogate_block_size Critical err looped client list\n");
			return FBE_STATUS_GENERIC_FAILURE;
		}
		base_edge = base_edge->next_edge;
	}

	if(i == 0){  /*We did not find an edge */
		fbe_base_transport_trace(FBE_TRACE_LEVEL_INFO,
								 FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
								 "blk_transport_srv_propogate_block_size no edges found\n");
	}

	fbe_block_transport_server_unlock(block_transport_server);

	return FBE_STATUS_OK;
}

/******************************
 * end fbe_block_transport_main.c
 ******************************/
