/***************************************************************************
 * Copyright (C) EMC Corporation 2012
 * All rights reserved. Licensed material -- property of EMC
 * Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_cmi_service_io.c
 ***************************************************************************
 *
 * @brief
 *  This file contains functions that FBE CMI communicates with peer for 
 *  SEP IOs.
 * 
 * 
 * @version
 *   04/22/2012:  Created. Lili Chen
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_cmi_interface.h"
#include "fbe_cmi_private.h"
#include "fbe/fbe_memory.h"
#include "fbe_cmi.h"
#include "fbe/fbe_trace_interface.h"
#include "CmiUpperInterface.h"
#include "spid_types.h" 
#include "spid_kernel_interface.h" 
#include "fbe/fbe_queue.h"
#include "fbe_cmi_kernel_private.h"
#include "fbe_cmi_io.h"
#include "fbe_topology.h"
#include "fbe/fbe_service_manager_interface.h"


/*************************
 *   GLOBALS
 *************************/
static fbe_queue_head_t	sep_io_send_context_head[MAX_SEP_IO_DATA_POOLS];
static fbe_queue_head_t	sep_io_waiting_queue_head;
static fbe_queue_head_t	sep_io_outstanding_queue_head;
static fbe_queue_head_t	sep_io_receive_context_head[MAX_SEP_IO_DATA_POOLS];
static fbe_queue_head_t	sep_io_receive_processing_queue_head;
static fbe_spinlock_t	send_context_lock;
static fbe_spinlock_t	receive_context_lock;
static fbe_spinlock_t	waiting_q_lock;
static fbe_spinlock_t	outstand_q_lock;
static fbe_spinlock_t	receive_process_q_lock;
static fbe_cmi_io_translation_table_t local_table;
static fbe_cmi_io_translation_table_t peer_table;

static fbe_cmi_io_msg_t *table_context;
static fbe_cmi_io_msg_t *abort_context;
static fbe_cmi_io_statistics_t fbe_cmi_io_statistics[FBE_CMI_IO_MAX_CONDUITS];

static fbe_cmi_io_context_t *context_pool;
static fbe_packet_t *packet_pool;

static fbe_cmi_io_data_pool_t send_data_pool[MAX_SEP_IO_DATA_POOLS];
static fbe_cmi_io_data_pool_t receive_data_pool[MAX_SEP_IO_DATA_POOLS];

static fbe_u32_t pool_data_length[MAX_SEP_IO_DATA_POOLS] = { LARGE_SEP_IO_DATA_LENGTH, SMALL_SEP_IO_DATA_LENGTH };
static fbe_u32_t pool_memory_percent[MAX_SEP_IO_DATA_POOLS] = { 80, 20 };

extern fbe_cmi_client_info_t               client_list[FBE_CMI_CLIENT_ID_LAST];

/*************************
 *   FORWAD DECLARATIONS
 *************************/
static fbe_status_t fbe_cmi_sep_io_message_transmitted(fbe_cmi_message_t cmi_message, fbe_cmi_event_callback_context_t context);
static fbe_status_t fbe_cmi_sep_io_peer_lost(fbe_cmi_message_t cmi_message, fbe_cmi_event_callback_context_t context);
static fbe_status_t fbe_cmi_sep_io_message_received(fbe_cmi_message_t cmi_message);
static fbe_status_t fbe_cmi_sep_io_fatal_error_message(fbe_cmi_message_t cmi_message, fbe_cmi_event_callback_context_t context);

static fbe_status_t fbe_cmi_io_start_packet_with_context(fbe_packet_t * packet_p,
                                                                fbe_cmi_io_context_info_t * ctx_info);
static fbe_status_t fbe_cmi_io_start_packet_enqueue_context(fbe_packet_t *packet_p, fbe_cmi_io_context_info_t * ctx_info);
static fbe_status_t fbe_cmi_io_start_packet_failed(fbe_cmi_io_context_info_t *ctx_info);

static fbe_status_t fbe_cmi_io_fill_request_info(fbe_cmi_io_float_data_t * float_data, fbe_packet_t * packet_p);
static fbe_status_t fbe_cmi_io_fill_response_info(fbe_cmi_io_float_data_t * float_data, fbe_packet_t * packet_p);
static fbe_status_t fbe_cmi_io_process_packet_request(fbe_cmi_io_float_data_t * float_data);
static fbe_status_t fbe_cmi_io_process_packet_response(fbe_cmi_io_float_data_t * float_data);
static fbe_status_t fbe_cmi_io_start_io_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t fbe_cmi_io_start_control_packet_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);
static fbe_status_t fbe_cmi_io_send_packet_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t fbe_cmi_io_reconstruct_packet(fbe_packet_t * packet_p, fbe_cmi_io_float_data_t * float_data);
static fbe_status_t fbe_cmi_io_reconstruct_packet_status(fbe_packet_t * packet_p, fbe_cmi_io_float_data_t * float_data);

static fbe_status_t fbe_cmi_io_enqueue_to_waiting_q(fbe_packet_t *packet_p);
static fbe_packet_t * fbe_cmi_io_dequeue_from_waiting_q(void);
static fbe_status_t fbe_cmi_io_start_from_waiting_q(void);
static void fbe_cmi_io_enqueue_context_info_to_q(fbe_cmi_io_context_info_t * ctx_info,
                                            fbe_queue_head_t * queue_head, fbe_spinlock_t *lock);
static fbe_cmi_io_context_info_t * fbe_cmi_io_dequeue_context_info_from_q(fbe_u32_t pool, fbe_u32_t slot,
                                              fbe_queue_head_t * queue_head, fbe_spinlock_t *lock);
static fbe_cmi_io_context_info_t * fbe_cmi_io_get_send_context(fbe_packet_t * packet_p);
static fbe_status_t fbe_cmi_io_drain_outstanding_queue(void);
static fbe_status_t fbe_cmi_io_drain_waiting_queue(void);

static void fbe_cmi_io_send_translation_info(void);
static fbe_status_t fbe_cmi_io_receive_translation_info(fbe_cmi_io_float_data_t * float_data);
static fbe_cmi_io_context_t * fbe_cmi_io_init_context_element(fbe_u32_t pool_id, fbe_u32_t slot, fbe_bool_t is_send);
static fbe_status_t fbe_cmi_io_copy_sg_data(fbe_packet_t *packet_p, 
                                            fbe_cmi_io_context_info_t *ctx_info,
                                            fbe_bool_t to_packet);
static fbe_status_t fbe_cmi_io_send_cancel_to_peer(fbe_packet_t * packet_p, fbe_u32_t slot);
static fbe_status_t fbe_cmi_io_process_packet_abort(fbe_cmi_io_float_data_t * float_data);
static fbe_status_t fbe_cmi_io_enqueue_check_cancel(fbe_queue_head_t * queue_head, fbe_packet_t * packet_p);
static fbe_status_t fbe_cmi_io_dequeue_check_cancel(fbe_queue_head_t * queue_head, fbe_packet_t * packet_p);
static fbe_status_t fbe_cmi_io_log_statistics(fbe_bool_t is_send, fbe_u64_t bytes, fbe_cpu_id_t cpu_id);
static __forceinline fbe_cmi_io_context_t * fbe_cmi_io_context_info_to_context(fbe_cmi_io_context_info_t * ctx_info);
static void * fbe_cmi_io_allocate_from_data_pool(fbe_bool_t is_send, fbe_u32_t pool_id, fbe_u32_t slot);
static fbe_status_t fbe_cmi_io_get_packet_data_length(fbe_packet_t *packet_p, fbe_u32_t *data_length);
static fbe_u32_t fbe_cmi_io_get_total_slots_for_pools(fbe_bool_t is_send);
static fbe_cmi_io_context_t * fbe_cmi_io_allocate_context_from_pool(fbe_bool_t is_send, fbe_u32_t pool_id, fbe_u32_t slot);
static void * fbe_cmi_io_allocate_packet_from_pool(fbe_bool_t is_send, fbe_u32_t pool_id, fbe_u32_t slot);

static fbe_status_t fbe_cmi_service_sep_io_send_memory_callback(fbe_cmi_event_t event, 
                                            fbe_u32_t cmi_message_length, 
                                            fbe_cmi_message_t cmi_message, 
                                            fbe_cmi_event_callback_context_t context);


/*!**************************************************************
 * fbe_cmi_service_sep_io_callback()
 ****************************************************************
 * @brief
 *  This is the callback function for SEP IO client when a FBE
 *  CMI event is posted. 
 *
 * @param
 *   event - FBE CMI event.
 *   cmi_message - Pointer to the message.
 *   cmi_message_length - message length.
 *   context - context.
 *
 * @return fbe_status_t
 * 
 * @author
 *  04/22/2012 - Created. Lili Chen
 *
 ****************************************************************/
fbe_status_t 
fbe_cmi_service_sep_io_callback(fbe_cmi_event_t event, fbe_u32_t cmi_message_length, fbe_cmi_message_t cmi_message,  fbe_cmi_event_callback_context_t context)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_cmi_io_float_data_t * float_data = (fbe_cmi_io_float_data_t *)cmi_message;

    if (float_data && (float_data->msg_type == FBE_CMI_IO_MESSAGE_TYPE_SEND_MEMORY))
    {
        status = fbe_cmi_service_sep_io_send_memory_callback(event, cmi_message_length, cmi_message, context);
        return status;
    }

    switch (event) {
        case FBE_CMI_EVENT_MESSAGE_TRANSMITTED:
            status = fbe_cmi_sep_io_message_transmitted(cmi_message, context);
            break;
        case FBE_CMI_EVENT_SP_CONTACT_LOST:
            status = fbe_cmi_sep_io_peer_lost(cmi_message, context);
            break;
        case FBE_CMI_EVENT_MESSAGE_RECEIVED:
            status = fbe_cmi_sep_io_message_received(cmi_message);
            break;
        case FBE_CMI_EVENT_FATAL_ERROR:
            status = fbe_cmi_sep_io_fatal_error_message(cmi_message, context);
            break;
        case FBE_CMI_EVENT_PEER_NOT_PRESENT:
        case FBE_CMI_EVENT_PEER_BUSY:
            status = fbe_cmi_sep_io_fatal_error_message(cmi_message, context);
            break;
        default:
            fbe_cmi_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, "%s Illegal CMI opcode: 0x%X\n", __FUNCTION__, event);
            break;
    }
    
    return status;
}
/****************************************************************
 * end fbe_cmi_service_sep_io_callback()
 ****************************************************************/

/*!**************************************************************
 * fbe_cmi_sep_io_message_transmitted()
 ****************************************************************
 * @brief
 *  This function handles transmitted event for SEP IO client.
 *
 * @param
 *   cmi_message - pointer to the message.
 *   context - context.
 *
 * @return fbe_status_t
 * 
 * @author
 *  04/22/2012 - Created. Lili Chen
 *
 ****************************************************************/
fbe_status_t 
fbe_cmi_sep_io_message_transmitted(fbe_cmi_message_t cmi_message, fbe_cmi_event_callback_context_t context)
{  
    fbe_cmi_io_float_data_t * float_data = (fbe_cmi_io_float_data_t *)cmi_message;
    fbe_cmi_io_message_type_t  msg_type = float_data->msg_type;

    fbe_cmi_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, "%s packet %p transmitted slot %d_%d type %d\n",
                  __FUNCTION__, float_data->original_packet, float_data->pool, float_data->slot, float_data->msg_type);

    switch(msg_type)
    {
        case FBE_CMI_IO_MESSAGE_TYPE_PACKET_REQUEST:
            break;
        case FBE_CMI_IO_MESSAGE_TYPE_PACKET_RESPONSE:
            break;
        case FBE_CMI_IO_MESSAGE_TYPE_TRANSLATION_INFO:
            fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, "%s: translation table transmitted \n", __FUNCTION__);
            fbe_atomic_exchange(&table_context->in_use, FBE_FALSE);
            break;
        case FBE_CMI_IO_MESSAGE_TYPE_PACKET_ABORT:
            fbe_atomic_exchange(&abort_context->in_use, FBE_FALSE);
            fbe_cmi_io_cancel_thread_notify();
            break;
        default:
            fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR, "%s: invalid message type %d \n", __FUNCTION__, msg_type);
            break;
    }

    return FBE_STATUS_OK; 
}
/****************************************************************
 * end fbe_cmi_sep_io_message_transmitted()
 ****************************************************************/

/*!**************************************************************
 * fbe_cmi_sep_io_peer_lost()
 ****************************************************************
 * @brief
 *  This function handles peer_lost event for SEP IO client.
 *
 * @param
 *   cmi_message - Pointer to the message.
 *   context - context.
 *
 * @return fbe_status_t
 * 
 * @author
 *  04/22/2012 - Created. Lili Chen
 *
 ****************************************************************/
fbe_status_t 
fbe_cmi_sep_io_peer_lost(fbe_cmi_message_t cmi_message, fbe_cmi_event_callback_context_t context)
{    

    fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, "%s peer lost\n", __FUNCTION__);

    /* Set the table to not valid */
    peer_table.table_valid = FBE_FALSE;

    /* We should drain IO in waiting queue and outstanding queue */
    fbe_cmi_io_drain_outstanding_queue();
    fbe_cmi_io_drain_waiting_queue();

    return FBE_STATUS_OK; 
}
/****************************************************************
 * end fbe_cmi_sep_io_peer_lost()
 ****************************************************************/

/*!**************************************************************
 * fbe_cmi_sep_io_message_received()
 ****************************************************************
 * @brief
 *  This function handles received event for SEP IO client.
 *
 * @param
 *   cmi_message - Pointer to the message.
 *
 * @return fbe_status_t
 * 
 * @author
 *  04/22/2012 - Created. Lili Chen
 *
 ****************************************************************/
fbe_status_t 
fbe_cmi_sep_io_message_received(fbe_cmi_message_t cmi_message)
{
    fbe_cmi_io_float_data_t * float_data = (fbe_cmi_io_float_data_t *)cmi_message;
    fbe_cmi_io_message_type_t  msg_type = float_data->msg_type;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    fbe_cmi_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, "%s packet received: orig_pkt %p slot %d_%d type %d \n",
                  __FUNCTION__, float_data->original_packet, float_data->pool, float_data->slot, msg_type);

    if (!local_table.table_valid ||
        !peer_table.table_valid && (msg_type != FBE_CMI_IO_MESSAGE_TYPE_TRANSLATION_INFO))
    {
        fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, "%s msg dropped: slot %d_%d type %d orig_pkt %p \n",
                      __FUNCTION__, float_data->pool, float_data->slot, msg_type, float_data->original_packet);
        return FBE_STATUS_OK;
    }
                  
    switch(msg_type)
    {
        case FBE_CMI_IO_MESSAGE_TYPE_PACKET_REQUEST:
            status = fbe_cmi_io_process_packet_request(float_data);
            break;
        case FBE_CMI_IO_MESSAGE_TYPE_PACKET_RESPONSE:
            status = fbe_cmi_io_process_packet_response(float_data);
            break;
        case FBE_CMI_IO_MESSAGE_TYPE_TRANSLATION_INFO:
            status = fbe_cmi_io_receive_translation_info(float_data);
            break;
        case FBE_CMI_IO_MESSAGE_TYPE_PACKET_ABORT:
            status = fbe_cmi_io_process_packet_abort(float_data);
            break;
        default:
            fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR, "%s: invalid message type %d \n", __FUNCTION__, msg_type);
            break;
    }

    return status;
}
/****************************************************************
 * end fbe_cmi_sep_io_message_received()
 ****************************************************************/

/*!**************************************************************
 * fbe_cmi_sep_io_fatal_error_message()
 ****************************************************************
 * @brief
 *  This function handles fatal error event for SEP IO client.
 *
 * @param
 *   cmi_message - Pointer to the message.
 *   context - context.
 *
 * @return fbe_status_t
 * 
 * @author
 *  04/22/2012 - Created. Lili Chen
 *
 ****************************************************************/
fbe_status_t 
fbe_cmi_sep_io_fatal_error_message(fbe_cmi_message_t cmi_message, fbe_cmi_event_callback_context_t context)
{
    fbe_cmi_io_float_data_t * float_data = (fbe_cmi_io_float_data_t *)cmi_message;
    fbe_cmi_io_context_info_t * ctx_info = (fbe_cmi_io_context_info_t *)context;
    fbe_cmi_io_message_type_t  msg_type = float_data->msg_type;

    /* If we are destroying, the context could be destroyed already. */
    if (!local_table.table_valid)
    {
        return FBE_STATUS_OK;
    }

    switch(msg_type)
    {
        case FBE_CMI_IO_MESSAGE_TYPE_PACKET_REQUEST:
            fbe_cmi_io_start_packet_failed(ctx_info);
            break;
        case FBE_CMI_IO_MESSAGE_TYPE_PACKET_RESPONSE:
            break;
        case FBE_CMI_IO_MESSAGE_TYPE_TRANSLATION_INFO:
            fbe_atomic_exchange(&table_context->in_use, FBE_FALSE);
            break;
        case FBE_CMI_IO_MESSAGE_TYPE_PACKET_ABORT:
            fbe_atomic_exchange(&abort_context->in_use, FBE_FALSE);
            fbe_cmi_io_cancel_thread_notify();
            break;
        default:
            fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR, "%s: invalid message type %d \n", __FUNCTION__, msg_type);
            break;
    }

    return FBE_STATUS_OK;
}
/****************************************************************
 * end fbe_cmi_sep_io_fatal_error_message()
 ****************************************************************/


/*!**************************************************************
 * fbe_cmi_io_init()
 ****************************************************************
 * @brief
 *  This function allocates memory for SEP IO during FBE CMI
 *  initialization. 
 *
 * @param None
 *
 * @return fbe_status_t
 * 
 * @author
 *  04/22/2012 - Created. Lili Chen
 *
 ****************************************************************/
fbe_status_t fbe_cmi_io_init(void)
{
    fbe_u32_t                   msg_count = 0, pool_id;
    fbe_cmi_io_context_t        *ctx_ptr;
    fbe_status_t                status;
    fbe_cmi_io_data_pool_t      *data_pool;
    fbe_u32_t                   total_slots;

    fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, "%s ENTRY\n", __FUNCTION__);

    total_slots = fbe_cmi_io_get_total_slots_for_pools(FBE_TRUE) + fbe_cmi_io_get_total_slots_for_pools(FBE_FALSE);
    if (total_slots == 0)
    {
        fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR,"%s We do not have data pool allocated for CMI IOs\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Initialize cancel thread */
    status = fbe_cmi_io_cancel_thread_init();
    if (status != FBE_STATUS_OK)
    {
        fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR,"%s failed to initialize cancel thread\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Initialize table_context and abort_context */
    table_context = (fbe_cmi_io_msg_t *)fbe_memory_native_allocate(sizeof(fbe_cmi_io_msg_t));
    if (table_context == NULL) {
        fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR,"%s failed to alloc table_context\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    abort_context = (fbe_cmi_io_msg_t *)fbe_memory_native_allocate(sizeof(fbe_cmi_io_msg_t));
    if (abort_context == NULL) {
        fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR,"%s failed to alloc abort_context\n", __FUNCTION__);
        fbe_memory_native_release(table_context);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_zero_memory(table_context, sizeof(fbe_cmi_io_msg_t));
    fbe_zero_memory(abort_context, sizeof(fbe_cmi_io_msg_t));

    /* Allocate contexts */
    context_pool = (fbe_cmi_io_context_t *)fbe_allocate_contiguous_memory(total_slots * sizeof(fbe_cmi_io_context_t));
    if (context_pool == NULL) {
        fbe_memory_native_release(table_context);
        fbe_memory_native_release(abort_context);
        fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR, "%s Unable to allocate cmi sep io contexts\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Allocate packets */
    total_slots = fbe_cmi_io_get_total_slots_for_pools(FBE_FALSE);
    packet_pool = (fbe_packet_t *)fbe_allocate_contiguous_memory(total_slots * FBE_MEMORY_CHUNK_SIZE_FOR_PACKET);
    if (packet_pool == NULL) {
        fbe_memory_native_release(table_context);
        fbe_memory_native_release(abort_context);
        fbe_release_contiguous_memory((void *)context_pool);
        fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR, "%s Unable to allocate packets for receive contexts\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Initialize the locks and queue */
    fbe_spinlock_init(&send_context_lock);
    fbe_spinlock_init(&receive_context_lock);
    fbe_spinlock_init(&waiting_q_lock);
    fbe_spinlock_init(&outstand_q_lock);
    fbe_spinlock_init(&receive_process_q_lock);
    for (pool_id = 0; pool_id < MAX_SEP_IO_DATA_POOLS; pool_id++) 
    {
        fbe_queue_init(&sep_io_send_context_head[pool_id]);
        fbe_queue_init(&sep_io_receive_context_head[pool_id]);
    }
    fbe_queue_init(&sep_io_waiting_queue_head);
    fbe_queue_init(&sep_io_outstanding_queue_head);
    fbe_queue_init(&sep_io_receive_processing_queue_head);
    
    /* Set the table to not valid */
    local_table.table_valid = FBE_FALSE;
    peer_table.table_valid = FBE_FALSE;
    
    /* Initialize send queue */
    for (pool_id = 0; pool_id < MAX_SEP_IO_DATA_POOLS; pool_id++) 
    {
        msg_count = 0;
        data_pool = &send_data_pool[pool_id];

        while (msg_count < data_pool->pool_slots) {
            ctx_ptr = fbe_cmi_io_init_context_element(pool_id, msg_count, FBE_TRUE);
            if (ctx_ptr == NULL) {
                fbe_cmi_io_destroy();
                return FBE_STATUS_GENERIC_FAILURE;
            }
        
            /* Add to queue */
            fbe_queue_push(&sep_io_send_context_head[pool_id], &ctx_ptr->queue_element);

            msg_count ++;
        }
    }
    
    /* Initialize receive queue */
    for (pool_id = 0; pool_id < MAX_SEP_IO_DATA_POOLS; pool_id++) {
        msg_count = 0;
        data_pool = &receive_data_pool[pool_id];

        while (msg_count < data_pool->pool_slots) {
            ctx_ptr = fbe_cmi_io_init_context_element(pool_id, msg_count, FBE_FALSE);
            if (ctx_ptr == NULL) {
                fbe_cmi_io_destroy();
                return FBE_STATUS_GENERIC_FAILURE;
            }

            /* Add to the queue */
            fbe_queue_push(&sep_io_receive_context_head[pool_id], &ctx_ptr->queue_element);

            msg_count ++;
        }
    }
    
    /* Set local table to valid */
    local_table.table_valid = FBE_TRUE;

    return FBE_STATUS_OK;
}
/****************************************************************
 * end fbe_cmi_io_init()
 ****************************************************************/

/*!**************************************************************
 * fbe_cmi_io_init_context_element()
 ****************************************************************
 * @brief
 *  This function allocates memory for SEP IO during FBE CMI
 *  initialization. 
 *
 * @param pool_id - pool id
 * @param slot - slot information
 * @param is_send - whether it is for send context
 *
 * @return ptr to context
 * 
 * @author
 *  04/22/2012 - Created. Lili Chen
 *
 ****************************************************************/
fbe_cmi_io_context_t *
fbe_cmi_io_init_context_element(fbe_u32_t pool_id, fbe_u32_t slot, fbe_bool_t is_send)
{
    fbe_cmi_io_context_t        *ctx_ptr;
    fbe_cmi_io_context_info_t   *ctx_info;
    fbe_packet_t                       *packet_p;
    fbe_payload_ex_t * payload_p;

    ctx_ptr = fbe_cmi_io_allocate_context_from_pool(is_send, pool_id, slot);
    if (ctx_ptr == NULL) {
        fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR, "%s Unable to allocate cmi sep io context\n", __FUNCTION__);
        return NULL;
    }
    fbe_zero_memory(ctx_ptr, sizeof(fbe_cmi_io_context_t));
    fbe_queue_element_init(&ctx_ptr->queue_element);
    fbe_spinlock_init(&ctx_ptr->context_lock);

    ctx_info = &ctx_ptr->context_info;
    ctx_info->alloc_length = pool_data_length[pool_id];
    ctx_info->data = fbe_cmi_io_allocate_from_data_pool(is_send, pool_id, slot);
    if (ctx_info->data == NULL) {
        fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR, "%s Unable to allocate cmi sep io data\n", __FUNCTION__);
        return NULL;
    }
 
    if (!is_send)
    {
        ctx_info->packet = fbe_cmi_io_allocate_packet_from_pool(is_send, pool_id, slot);
        if (ctx_info->packet == NULL) {
            fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR, "%s Unable to allocate cmi sep io packet\n", __FUNCTION__);
            return NULL;
        }
        packet_p = (fbe_packet_t *)ctx_info->packet;
        fbe_transport_initialize_packet(packet_p);
        payload_p = fbe_transport_get_payload_ex(packet_p);
        fbe_payload_ex_set_sg_list(payload_p, ctx_info->sg_element, 1);
    }

    /* Init ctx_ptr */
    ctx_info->pool = pool_id;
    ctx_info->slot = slot;
    ctx_info->float_data.pool = pool_id;
    ctx_info->float_data.slot = slot;
    if (is_send) {
        ctx_info->float_data.msg_type = FBE_CMI_IO_MESSAGE_TYPE_PACKET_REQUEST;
    }
    else {
        ctx_info->float_data.msg_type = FBE_CMI_IO_MESSAGE_TYPE_PACKET_RESPONSE;
    }

    ctx_info->kernel_message.pirp = EmcpalIrpAllocate(3);
    ctx_info->sg_element[0].address = ctx_info->data;
    ctx_info->sg_element[0].count = ctx_info->alloc_length;
    ctx_info->sg_element[1].address = NULL;
    ctx_info->sg_element[1].count = 0;

    return ctx_ptr;
}
/****************************************************************
 * end fbe_cmi_io_init_context_element()
 ****************************************************************/

/*!**************************************************************
 * fbe_cmi_io_destroy()
 ****************************************************************
 * @brief
 *  This function destroys memory for SEP IO during FBE CMI
 *  teardown. 
 *
 * @param None
 *
 * @return fbe_status_t
 * 
 * @author
 *  04/22/2012 - Created. Lili Chen
 *
 ****************************************************************/
void fbe_cmi_io_destroy(void)
{
    fbe_cmi_io_context_t    *ctx_ptr;
    fbe_u32_t 				delay_count = 0;
    fbe_u32_t 				i;

    fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, "%s ENTRY\n", __FUNCTION__);

    /* Set tables to invalid so that no IO could be sent */
    local_table.table_valid = FBE_FALSE;
    peer_table.table_valid = FBE_FALSE;

    /* We should drain IO in waiting queue and outstanding queue */
    fbe_cmi_io_drain_waiting_queue();

    /* Drain IO outstanding queue */
    while ((fbe_cmi_io_drain_outstanding_queue() != FBE_STATUS_OK) && (delay_count < 1000)) {
        fbe_thread_delay(10);/*wait 10ms*/
        delay_count++;
    }
    if (delay_count == 1000) {
        fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR, "%s can't drain outstanding queue in 10 seconds\n", __FUNCTION__);
    }

    /* Drain receive processing queue */
    delay_count = 0;
    while (!fbe_queue_is_empty(& sep_io_receive_processing_queue_head) && (delay_count < 1000)) {
        fbe_thread_delay(10);/*wait 10ms*/
        delay_count++;
    }
    if (delay_count == 1000) {
        fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR, "%s can't drain receive processing queue in 10 seconds\n", __FUNCTION__);
    }

    for (i = 0; i < MAX_SEP_IO_DATA_POOLS; i++)
    {
        /* Destroy send context */
        while ((ctx_ptr = (fbe_cmi_io_context_t *)fbe_queue_pop(&sep_io_send_context_head[i])) != NULL) {
            EmcpalIrpFree(ctx_ptr->context_info.kernel_message.pirp);
            fbe_spinlock_destroy(&ctx_ptr->context_lock);
        }
        fbe_queue_destroy(&sep_io_send_context_head[i]);

        /* Destroy receive context */
        while ((ctx_ptr = (fbe_cmi_io_context_t *)fbe_queue_pop(&sep_io_receive_context_head[i])) != NULL) {
            EmcpalIrpFree(ctx_ptr->context_info.kernel_message.pirp);
            fbe_spinlock_destroy(&ctx_ptr->context_lock);
        }
        fbe_queue_destroy(&sep_io_receive_context_head[i]);
    }

    fbe_spinlock_destroy(&send_context_lock);
    fbe_spinlock_destroy(&receive_context_lock);
    fbe_spinlock_destroy(&waiting_q_lock);
    fbe_spinlock_destroy(&outstand_q_lock);
    fbe_spinlock_destroy(&receive_process_q_lock);
    fbe_queue_destroy(&sep_io_waiting_queue_head);
    fbe_queue_destroy(&sep_io_outstanding_queue_head);
    fbe_queue_destroy(&sep_io_receive_processing_queue_head);

    /* Destroy context pool */
    if (context_pool) {
        fbe_release_contiguous_memory((void *)context_pool);
    }
    /* Destroy packet pool */
    if (packet_pool) {
        fbe_release_contiguous_memory((void *)packet_pool);
    }

    /* Destroy table_context and abort_context */
    if (table_context) {
        fbe_memory_native_release(table_context);
    }
    if (abort_context) {
        fbe_memory_native_release(abort_context);
    }

    /* Destroy cancel thread */
    fbe_cmi_io_cancel_thread_destroy();

    return;
}
/****************************************************************
 * end fbe_cmi_io_destroy()
 ****************************************************************/

/*!**************************************************************
 * fbe_cmi_send_packet_to_other_sp()
 ****************************************************************
 * @brief
 *  This function is the main function in FBE CMI kernel to send 
 *  a packet to peer SP. 
 *
 * @param
 *   packet_p  - Pointer to packet.
 *
 * @return fbe_status_t
 * 
 * @author
 *  04/22/2012 - Created. Lili Chen
 *
 ****************************************************************/
fbe_status_t fbe_cmi_send_packet_to_other_sp(fbe_packet_t * packet_p)
{
    fbe_status_t					        status  = FBE_STATUS_GENERIC_FAILURE;
    fbe_cmi_io_context_info_t *      ctx_info;
    fbe_u32_t pool, slot;
    
    /* any chance the other SP is not even there?*/
    if (!fbe_cmi_is_peer_alive()){
        return FBE_STATUS_NO_DEVICE;
    }
    
    /* This should not happen. The local table is initialized during fbe cmi initialization */
    if (!local_table.table_valid)
    {
        return FBE_STATUS_NO_DEVICE;
    }
    
    /* We should obtain peer table first */
    if (!peer_table.table_valid)
    {
        fbe_cmi_trace(FBE_TRACE_LEVEL_DEBUG_LOW, "Wait translation info: packet %p in queue \n", packet_p);
        fbe_cmi_io_enqueue_to_waiting_q(packet_p);
        fbe_cmi_io_send_translation_info();
        return FBE_STATUS_PENDING;
    }

    /* Enqueue to waiting if there are packets waiting */
    fbe_spinlock_lock(&waiting_q_lock);
    if (!fbe_queue_is_empty(&sep_io_waiting_queue_head))
    {
        fbe_spinlock_unlock(&waiting_q_lock);
        fbe_cmi_trace(FBE_TRACE_LEVEL_DEBUG_LOW, "Waiting queue not empty: packet %p in queue \n", packet_p);
        fbe_cmi_io_enqueue_to_waiting_q(packet_p);
        return FBE_STATUS_PENDING;
    }
    fbe_spinlock_unlock(&waiting_q_lock);

    /* Get a free structure from the pool*/
    fbe_spinlock_lock(&send_context_lock);
    ctx_info = fbe_cmi_io_get_send_context(packet_p);
    if (ctx_info == NULL)
    {
        fbe_spinlock_unlock(&send_context_lock);
        fbe_cmi_trace(FBE_TRACE_LEVEL_DEBUG_LOW, "No context: packet %p in queue \n", packet_p);
        fbe_cmi_io_enqueue_to_waiting_q(packet_p);
        return FBE_STATUS_PENDING;
    }
    fbe_spinlock_unlock(&send_context_lock);

    fbe_cmi_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, "%s slot %d_%d\n", __FUNCTION__, ctx_info->pool, ctx_info->slot);
    pool = ctx_info->pool;
    slot = ctx_info->slot;

    fbe_cmi_io_start_packet_enqueue_context(packet_p, ctx_info);
    status = fbe_cmi_io_start_packet_with_context(packet_p, ctx_info);
    if (status != FBE_STATUS_OK)
    {
        fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, "%s: start packet %p failed status 0x%x peer valid %d\n", __FUNCTION__, packet_p, status, peer_table.table_valid);
        fbe_cmi_io_start_packet_failed(ctx_info);
    }

    return FBE_STATUS_PENDING;
}
/****************************************************************
 * end fbe_cmi_send_packet_to_other_sp()
 ****************************************************************/

/*!**************************************************************
 * fbe_cmi_io_start_packet_enqueue_context()
 ****************************************************************
 * @brief
 *  This function enqueues a context before starting sending a packet. 
 *
 * @param
 *   packet_p  - Pointer to packet.
 *   ctx_info  - Pointer to context.
 *
 * @return fbe_status_t
 * 
 * @author
 *  04/22/2012 - Created. Lili Chen
 *
 ****************************************************************/
fbe_status_t fbe_cmi_io_start_packet_enqueue_context(fbe_packet_t *packet_p, fbe_cmi_io_context_info_t * ctx_info)
{
    ctx_info->context_attr = FBE_CMI_IO_CONTEXT_ATTR_HOLD;

    if (ctx_info->float_data.msg_type == FBE_CMI_IO_MESSAGE_TYPE_PACKET_REQUEST)
    {
        /* Enqueue to outstanding queue */
        fbe_cmi_io_enqueue_context_info_to_q(ctx_info, &sep_io_outstanding_queue_head, &outstand_q_lock);
        fbe_cmi_io_enqueue_check_cancel(&sep_io_outstanding_queue_head, packet_p);
    }
    else
    {
        /* Enqueue to receive queue now */
        fbe_cmi_io_enqueue_context_info_to_q(ctx_info, &sep_io_receive_context_head[ctx_info->pool], &receive_context_lock);
    }

    return FBE_STATUS_OK;
}
/****************************************************************
 * end fbe_cmi_io_start_packet_enqueue_context()
 ****************************************************************/

/*!**************************************************************
 * fbe_cmi_io_start_packet_failed()
 ****************************************************************
 * @brief
 *  This function processes context and packet when sending a packet
 *  failed. 
 *
 * @param
 *   ctx_info  - Pointer to context.
 *
 * @return fbe_status_t
 * 
 * @author
 *  04/22/2012 - Created. Lili Chen
 *
 ****************************************************************/
fbe_status_t fbe_cmi_io_start_packet_failed(fbe_cmi_io_context_info_t *ctx_info)
{
    fbe_queue_element_t * queue_element;
    fbe_bool_t context_dequeued = FBE_FALSE;
    fbe_cmi_io_context_t * ctx_ptr = NULL;
    fbe_packet_t *packet_p = NULL;

    fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, "%s ENTRY\n", __FUNCTION__);
    fbe_spinlock_lock(&outstand_q_lock);
    queue_element = fbe_queue_front(&sep_io_outstanding_queue_head);
    while (queue_element)
    {
        ctx_ptr = (fbe_cmi_io_context_t *)queue_element;
        if (ctx_info == &ctx_ptr->context_info)
        {
            /* We found the context in queue. */
            fbe_queue_remove(queue_element);
            context_dequeued = FBE_TRUE;
            break;
        }
        queue_element = fbe_queue_next(&sep_io_outstanding_queue_head, queue_element);
    }
    fbe_spinlock_unlock(&outstand_q_lock);

    if (!context_dequeued)
    {
        fbe_cmi_trace(FBE_TRACE_LEVEL_WARNING, "%s context not in queue %p\n", __FUNCTION__, ctx_info);
        return FBE_STATUS_OK;
    }

    fbe_spinlock_lock(&ctx_ptr->context_lock);
    packet_p = (fbe_packet_t *)ctx_info->float_data.original_packet;
    ctx_info->float_data.original_packet = NULL;
    if (ctx_info->context_attr & FBE_CMI_IO_CONTEXT_ATTR_HOLD)
    {
        /* Still hold, the context need to be returned later. */
        ctx_info->context_attr |= FBE_CMI_IO_CONTEXT_ATTR_NEED_START;
        fbe_spinlock_unlock(&ctx_ptr->context_lock);
        fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, "%s: packet %p failed irp pending context %p\n", __FUNCTION__, packet_p, ctx_info);
    }
    else
    {
        fbe_spinlock_unlock(&ctx_ptr->context_lock);
        fbe_cmi_io_enqueue_context_info_to_q(ctx_info, &sep_io_send_context_head[ctx_info->pool], &send_context_lock);
        fbe_cmi_io_start_from_waiting_q();
    }

    if (packet_p) {
        fbe_cmi_io_dequeue_check_cancel(&sep_io_outstanding_queue_head, packet_p);
        /* Fail the packet back */
        fbe_transport_set_status(packet_p, FBE_STATUS_FAILED, 0);
        fbe_transport_complete_packet(packet_p);
    }

    return FBE_STATUS_OK;
}
/****************************************************************
 * end fbe_cmi_io_start_packet_failed()
 ****************************************************************/

/*!**************************************************************
 * fbe_cmi_io_start_packet_with_context()
 ****************************************************************
 * @brief
 *  This function sends a packet which already obtains context. 
 *
 * @param
 *   packet_p  - Pointer to packet.
 *   ctx_info  - Pointer to context.
 *
 * @return fbe_status_t
 * 
 * @author
 *  04/22/2012 - Created. Lili Chen
 *
 ****************************************************************/
fbe_status_t fbe_cmi_io_start_packet_with_context(fbe_packet_t * packet_p,
                                                  fbe_cmi_io_context_info_t * ctx_info)
{
    fbe_status_t					 status  = FBE_STATUS_GENERIC_FAILURE;
    fbe_cmi_io_message_info_t *      kernel_message;
    fbe_cmi_io_float_data_t *        float_data;

    fbe_cmi_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, "%s slot %d_%d packet %p\n", __FUNCTION__, ctx_info->pool, ctx_info->slot, packet_p);

    /* Fill packet information for floating data */
    float_data = &ctx_info->float_data;
    if (float_data->msg_type == FBE_CMI_IO_MESSAGE_TYPE_PACKET_REQUEST)
    {
        status = fbe_cmi_io_fill_request_info(float_data, packet_p);
    }
    else if (float_data->msg_type == FBE_CMI_IO_MESSAGE_TYPE_PACKET_RESPONSE)
    {
        status = fbe_cmi_io_fill_response_info(float_data, packet_p); 
    }
    else
    {
        fbe_cmi_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, "%s: invalid message type %d \n",
                      __FUNCTION__, float_data->msg_type);
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    if (status != FBE_STATUS_OK) {
        fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR, "%s, failed to fill float data info\n", __FUNCTION__);
        fbe_cmi_io_return_held_context(ctx_info);
        return status;
    }
    fbe_cmi_io_log_statistics((float_data->msg_type == FBE_CMI_IO_MESSAGE_TYPE_PACKET_REQUEST)?FBE_TRUE:FBE_FALSE, 
                              float_data->fixed_data_length,
                              packet_p->cpu_id);

    /* Fill kernel message structure */
    kernel_message = &ctx_info->kernel_message;
    kernel_message->context = ctx_info;

    /* Fill CMID related information */
    fbe_cmi_service_increase_message_counter();
    status = fbe_cmi_io_start_send(ctx_info, packet_p);
    if (status != FBE_STATUS_OK) {
        fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR, "%s, failed to send packet\n", __FUNCTION__);
        fbe_cmi_service_decrease_message_counter();
    }

    return status;
}
/****************************************************************
 * end fbe_cmi_io_start_packet_with_context()
 ****************************************************************/

/*!**************************************************************
 * fbe_cmi_io_process_packet_request()
 ****************************************************************
 * @brief
 *  This function process the IO request when the SEP IO client
 *  receives a message from peer. 
 *
 * @param float_data - packet information
 *
 * @return fbe_status_t
 * 
 * @author
 *  05/15/2012 - Created. Lili Chen
 *
 ****************************************************************/
fbe_status_t fbe_cmi_io_process_packet_request(fbe_cmi_io_float_data_t * float_data)
{
    fbe_cmi_io_context_info_t * ctx_info;
    fbe_packet_t * packet_p;
    fbe_queue_head_t tmp_queue;
    
    fbe_cmi_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, "%s slot %d_%d, original packet %p\n", __FUNCTION__,
                  float_data->pool, float_data->slot, float_data->original_packet);

    /* Get a free structure from the pool */
    ctx_info = fbe_cmi_io_dequeue_context_info_from_q(float_data->pool, float_data->slot, 
                                                      &sep_io_receive_context_head[float_data->pool],
                                                      &receive_context_lock);
    if (ctx_info == NULL)
    {
        /* There is a bug here. We should have a context. */
        fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR, "%s: No receive context! slot %d_%d \n",
                      __FUNCTION__, float_data->pool, float_data->slot);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Enqueue to processing queue */
    fbe_cmi_io_enqueue_context_info_to_q(ctx_info, &sep_io_receive_processing_queue_head, &receive_process_q_lock);

    ctx_info->float_data.original_packet = float_data->original_packet;
    ctx_info->float_data.fixed_data_length = float_data->fixed_data_length;
    ctx_info->float_data.payload_opcode = float_data->payload_opcode;

    /* For read, copy sgl_list first */
    if ((float_data->payload_opcode == FBE_PAYLOAD_OPCODE_BLOCK_OPERATION) && 
        fbe_payload_block_operation_opcode_is_media_read(float_data->msg.request_info.u.block_payload.block_opcode))
    {
        fbe_u32_t sg_list_count = 0;
        fbe_u64_t bytes_found;
        if (cmi_service_get_peer_version() == FBE_FALSE) {
            sg_list_count = fbe_sg_element_count_entries_and_bytes((fbe_sg_element_t *)ctx_info->data, ctx_info->float_data.fixed_data_length, &bytes_found);
            fbe_copy_memory (&ctx_info->kernel_message.fixed_data_blob, ctx_info->data, (sg_list_count + 1) * sizeof(CMI_PHYSICAL_SG_ELEMENT));
        } else {
            ctx_info->kernel_message.fixed_data_blob.blob_sgl[0].PhysAddress = fbe_cmi_io_get_peer_slot_address(FBE_FALSE, float_data->pool, float_data->slot);
            ctx_info->kernel_message.fixed_data_blob.blob_sgl[0].length = float_data->fixed_data_length;
            ctx_info->kernel_message.fixed_data_blob.blob_sgl[1].PhysAddress = 0;
            ctx_info->kernel_message.fixed_data_blob.blob_sgl[1].length = 0;
        }
    }

    /* Reconstruct packet */
    packet_p = (fbe_packet_t *)ctx_info->packet;
    fbe_cmi_io_reconstruct_packet(packet_p, float_data);
    
    /* Send to topology */
    fbe_transport_set_completion_function(packet_p, fbe_cmi_io_send_packet_completion, ctx_info);
    fbe_queue_init(&tmp_queue);
    if (fbe_queue_is_element_on_queue(&packet_p->queue_element))
    {
        /* There is a bug here. packet should not be on any queue. */
        fbe_cmi_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, "%s: packet %p already on the queue \n",
                      __FUNCTION__, packet_p);
    }
    fbe_queue_push(&tmp_queue, &packet_p->queue_element);
    if (ctx_info->float_data.payload_opcode == FBE_PAYLOAD_OPCODE_CONTROL_OPERATION)
    {
        fbe_transport_run_queue_push(&tmp_queue, fbe_cmi_io_start_control_packet_completion, packet_p);
    }
    else
    {
        fbe_transport_run_queue_push(&tmp_queue, fbe_cmi_io_start_io_completion, packet_p);
    }
    fbe_queue_destroy(&tmp_queue);
    
    return FBE_STATUS_OK;
}
/****************************************************************
 * end fbe_cmi_io_process_packet_request()
 ****************************************************************/

/*!**************************************************************
 * fbe_cmi_io_start_io_completion()
 ****************************************************************
 * @brief
 *  This is a wrapper function to send the IO (from peer) to topology. 
 *
 * @param packet_p - pointer to packet
 * @param context - completion context
 *
 * @return fbe_status_t
 * 
 * @author
 *  05/15/2012 - Created. Lili Chen
 *
 ****************************************************************/
fbe_status_t fbe_cmi_io_start_io_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{
    fbe_status_t status;

    fbe_cmi_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, "%s send packet %p to topology\n", __FUNCTION__, packet_p);

    /* send the packet to topology. */
    status = fbe_topology_send_io_packet(packet_p);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/****************************************************************
 * end fbe_cmi_io_start_io_completion()
 ****************************************************************/

/*!**************************************************************
 * fbe_cmi_io_start_control_packet_completion()
 ****************************************************************
 * @brief
 *  This is a wrapper function to send the control packet to topology. 
 *
 * @param packet_p - pointer to packet
 * @param context - completion context
 *
 * @return fbe_status_t
 * 
 * @author
 *  05/15/2012 - Created. Lili Chen
 *
 ****************************************************************/
fbe_status_t fbe_cmi_io_start_control_packet_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{
    fbe_status_t status;

    fbe_cmi_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, "%s send control packet %p to topology\n", __FUNCTION__, packet_p);

    /* send the packet to topology. */
    status = fbe_service_manager_send_control_packet(packet_p);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/****************************************************************
 * end fbe_cmi_io_start_control_packet_completion()
 ****************************************************************/

/*!**************************************************************
 * fbe_cmi_io_send_packet_completion()
 ****************************************************************
 * @brief
 *  This is the completion function that a packet (from peer) is 
 *  completed in this SP and ready to send a response to peer. 
 *
 * @param packet_p - pointer to packet
 * @param context - pointer to context
 *
 * @return fbe_status_t
 * 
 * @author
 *  05/15/2012 - Created. Lili Chen
 *
 ****************************************************************/
fbe_status_t fbe_cmi_io_send_packet_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{
    fbe_cmi_io_context_info_t * ctx_info = (fbe_cmi_io_context_info_t *)context;
    fbe_cmi_io_context_t * ctx_ptr = fbe_cmi_io_context_info_to_context(ctx_info);
    fbe_status_t status  = FBE_STATUS_GENERIC_FAILURE;

    fbe_cmi_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, "%s slot %d_%d\n", __FUNCTION__, ctx_info->pool, ctx_info->slot);

    /* Dequeue from receive processing queue */
    fbe_cmi_io_dequeue_context_info_from_q(ctx_info->pool, ctx_info->slot, 
                                           &sep_io_receive_processing_queue_head,
                                           &receive_process_q_lock);

    fbe_spinlock_lock(&ctx_ptr->context_lock);
    if (ctx_info->context_attr & FBE_CMI_IO_CONTEXT_ATTR_HOLD)
    {
        /* Still hold? */
        ctx_info->context_attr |= FBE_CMI_IO_CONTEXT_ATTR_NEED_START;
        fbe_spinlock_unlock(&ctx_ptr->context_lock);
        fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, "%s: packet %p complete irp pending context %p\n", __FUNCTION__, packet_p, ctx_info);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }
    fbe_spinlock_unlock(&ctx_ptr->context_lock);

    fbe_cmi_io_start_packet_enqueue_context(packet_p, ctx_info);
    status = fbe_cmi_io_start_packet_with_context(packet_p, ctx_info);
    if (status != FBE_STATUS_OK)
    {
        fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, "%s: start packet %p failed status 0x%x \n", __FUNCTION__, packet_p, status);
    }

    return status;
}
/****************************************************************
 * end fbe_cmi_io_send_packet_completion()
 ****************************************************************/

/*!**************************************************************
 * fbe_cmi_io_process_packet_response()
 ****************************************************************
 * @brief
 *  This function process the IO response (IO finishes in peer SP), 
 *  when the SEP IO client receives a message from peer. 
 *
 * @param float_data - packet information
 *
 * @return fbe_status_t
 * 
 * @author
 *  05/15/2012 - Created. Lili Chen
 *
 ****************************************************************/
fbe_status_t fbe_cmi_io_process_packet_response(fbe_cmi_io_float_data_t * float_data)
{
    fbe_packet_t * packet_p = (fbe_packet_t *)(float_data->original_packet);
    fbe_cmi_io_context_info_t * ctx_info;    
    fbe_cmi_io_context_t * ctx_ptr;
    fbe_queue_head_t tmp_queue;
 
    fbe_cmi_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, "%s slot %d_%d, original packet %p\n", __FUNCTION__,
                  float_data->pool, float_data->slot, float_data->original_packet);

    /* Find send context */
    ctx_info = fbe_cmi_io_dequeue_context_info_from_q(float_data->pool, float_data->slot,
                                                      &sep_io_outstanding_queue_head,
                                                      &outstand_q_lock);
    if (ctx_info == NULL)
    {
        fbe_cmi_trace(FBE_TRACE_LEVEL_WARNING, "%s packet not in queue, slot %d_%d, original packet %p\n", __FUNCTION__,
                      float_data->pool, float_data->slot, float_data->original_packet);
        return FBE_STATUS_OK;
    }
    ctx_ptr = fbe_cmi_io_context_info_to_context(ctx_info);

    if (packet_p != ctx_info->float_data.original_packet)
    {
        fbe_cmi_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, "%s context corrupted, slot %d_%d, original packet %p\n", __FUNCTION__,
                      float_data->pool, float_data->slot, float_data->original_packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_cmi_io_dequeue_check_cancel(&sep_io_outstanding_queue_head, packet_p);

    /* Fill status in packet */
    fbe_cmi_io_reconstruct_packet_status(packet_p, float_data);

    if (float_data->payload_opcode == FBE_PAYLOAD_OPCODE_BLOCK_OPERATION)
    {
        /* Copy read data when needed */
        if (/*fbe_payload_block_operation_opcode_is_media_read(opcode) && */(ctx_info->context_attr & FBE_CMI_IO_CONTEXT_ATTR_READ_COPY))
        {
            fbe_cmi_io_copy_sg_data(packet_p, ctx_info, FBE_TRUE);
        }
    }
    /* Complete packet */
    fbe_queue_init(&tmp_queue);
    if (fbe_queue_is_element_on_queue(&packet_p->queue_element))
    {
        /* There is a bug here. packet should not be on any queue. */
        fbe_cmi_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, "%s: packet %p already on the queue \n",
                      __FUNCTION__, packet_p);
    }
    fbe_queue_push(&tmp_queue, &packet_p->queue_element);
    fbe_transport_run_queue_push(&tmp_queue, NULL, NULL);
    fbe_queue_destroy(&tmp_queue);
    
    /* Return context to queue */
    fbe_spinlock_lock(&ctx_ptr->context_lock);
    if (ctx_info->context_attr & FBE_CMI_IO_CONTEXT_ATTR_HOLD)
    {
        /* Do not reuse the context when the irp is not completed yet */
        ctx_info->context_attr |= FBE_CMI_IO_CONTEXT_ATTR_NEED_START;
        fbe_spinlock_unlock(&ctx_ptr->context_lock);
        fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, "%s: packet %p complete irp pending context %p\n", __FUNCTION__, packet_p, ctx_info);
        return FBE_STATUS_OK;
    }
    fbe_spinlock_unlock(&ctx_ptr->context_lock);

    fbe_cmi_io_enqueue_context_info_to_q(ctx_info, &sep_io_send_context_head[ctx_info->pool], &send_context_lock);
    fbe_cmi_io_start_from_waiting_q();
    
    return FBE_STATUS_OK;
}
/****************************************************************
 * end fbe_cmi_io_process_packet_response()
 ****************************************************************/

/*!**************************************************************
 * fbe_cmi_io_fill_request_info()
 ****************************************************************
 * @brief
 *  This function fills information from a packet to floating data
 *  structure so that it could be send to peer. 
 *
 * @param float_data - pointer to floating data
 * @param packet_p - pointer to packet
 *
 * @return fbe_status_t
 * 
 * @author
 *  05/15/2012 - Created. Lili Chen
 *
 ****************************************************************/
fbe_status_t fbe_cmi_io_fill_request_info(fbe_cmi_io_float_data_t * float_data, fbe_packet_t * packet_p)
{
    fbe_payload_ex_t * payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_payload_block_operation_t *block_payload_p = NULL;
    fbe_payload_control_operation_t *control_payload_p = NULL;
    fbe_payload_opcode_t payload_opcode = FBE_PAYLOAD_OPCODE_INVALID;
    fbe_sg_element_t *sg_list = NULL;
    fbe_packet_t *  master_packet_p = NULL, *sub_packet_p = packet_p;
    fbe_cmi_io_packet_request_t * request_info = &float_data->msg.request_info;

    /* Check whether we support the operation */
    if (payload_p->current_operation != NULL)
    {
        payload_opcode = payload_p->current_operation->payload_opcode;
    }
    if ((payload_opcode != FBE_PAYLOAD_OPCODE_BLOCK_OPERATION) && 
        (payload_opcode != FBE_PAYLOAD_OPCODE_CONTROL_OPERATION))
    {
        fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR, "%s opcode 0x%x not supported\n", __FUNCTION__, payload_opcode);
        return FBE_STATUS_GENERIC_FAILURE;    
    }
    float_data->payload_opcode = payload_opcode;

    /* Get info from packet */
    float_data->original_packet = packet_p;
    fbe_transport_get_object_id(packet_p, &(request_info->object_id));
    while (request_info->object_id == FBE_OBJECT_ID_INVALID)
    {
        master_packet_p = (fbe_packet_t *)fbe_transport_get_master_packet(sub_packet_p);
        if (!master_packet_p)
        {
            return FBE_STATUS_GENERIC_FAILURE;    
        }
        fbe_transport_get_object_id(master_packet_p, &(request_info->object_id));
        sub_packet_p = master_packet_p;
    }
    fbe_transport_get_cpu_id(packet_p, &(request_info->cpu_id));
    fbe_transport_get_packet_attr(packet_p, &(request_info->packet_attr));
    fbe_transport_get_packet_priority(packet_p, &(request_info->packet_priority));
    fbe_transport_get_traffic_priority(packet_p, &(request_info->traffic_priority));
    fbe_transport_get_physical_drive_service_time(packet_p, &(request_info->physical_drive_service_time_ms));

    if (payload_opcode == FBE_PAYLOAD_OPCODE_BLOCK_OPERATION)
    {
        block_payload_p = fbe_payload_ex_get_block_operation(payload_p);
        if ((block_payload_p->block_count * block_payload_p->block_size) > FBE_U32_MAX)
        {
            fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR,
                          "%s block_count 0x%llx * block_size %d > FBE_U32_MAX\n",
                          __FUNCTION__, block_payload_p->block_count, block_payload_p->block_size);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        fbe_payload_ex_get_sg_list(payload_p, &sg_list, NULL);
        if (sg_list)
        {
            if ((block_payload_p->block_count * block_payload_p->block_size) > FBE_U32_MAX)
            {
                fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR,
                              "%s block_count 0x%llx * block_size %d > FBE_U32_MAX\n",
                              __FUNCTION__, (unsigned long long)block_payload_p->block_count, block_payload_p->block_size);
                return FBE_STATUS_GENERIC_FAILURE;
            }
            float_data->fixed_data_length = (fbe_u32_t)(block_payload_p->block_count * block_payload_p->block_size);
        }
        else
        {
            float_data->fixed_data_length = 0;
        }
        if (float_data->fixed_data_length > pool_data_length[float_data->pool])
        {
            fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR, "%s fixed data length 0x%x > pool data length 0x%x\n",
                          __FUNCTION__, float_data->fixed_data_length, pool_data_length[float_data->pool]);
            return FBE_STATUS_GENERIC_FAILURE;    
        }
        /* Get info from block payload */
        request_info->u.block_payload.block_opcode = block_payload_p->block_opcode;
        request_info->u.block_payload.block_flags = block_payload_p->block_flags;
        request_info->u.block_payload.lba = block_payload_p->lba;
        request_info->u.block_payload.block_count = block_payload_p->block_count;
        request_info->u.block_payload.block_size = block_payload_p->block_size;
        request_info->u.block_payload.optimum_block_size = block_payload_p->optimum_block_size;
        request_info->u.block_payload.payload_sg_descriptor.repeat_count = block_payload_p->payload_sg_descriptor.repeat_count;
        fbe_cmi_trace(FBE_TRACE_LEVEL_DEBUG_LOW, "%s slot %d_%d, opcode 0x%x lba 0x%llx count 0x%llx\n", __FUNCTION__,
                      float_data->pool, float_data->slot, block_payload_p->block_opcode, block_payload_p->lba, block_payload_p->block_count);
    }
    else if (payload_opcode == FBE_PAYLOAD_OPCODE_CONTROL_OPERATION)
    {
        control_payload_p = fbe_payload_ex_get_control_operation(payload_p);
        float_data->fixed_data_length = control_payload_p->buffer_length;
        if (float_data->fixed_data_length > pool_data_length[float_data->pool])
        {
            return FBE_STATUS_GENERIC_FAILURE;    
        }
        /* Get info from control payload */
        request_info->u.control_payload.control_opcode = control_payload_p->opcode;
        request_info->u.control_payload.buffer_length = control_payload_p->buffer_length;
    }

    return FBE_STATUS_OK;    
}
/****************************************************************
 * end fbe_cmi_io_fill_request_info()
 ****************************************************************/

/*!**************************************************************
 * fbe_cmi_io_fill_response_info()
 ****************************************************************
 * @brief
 *  This function fills status information from a packet to floating data
 *  structure so that it could be send to peer for IO response. 
 *
 * @param float_data - floating data structure
 * @param packet_p - pointer to packet
 *
 * @return fbe_status_t
 * 
 * @author
 *  05/15/2012 - Created. Lili Chen
 *
 ****************************************************************/
fbe_status_t fbe_cmi_io_fill_response_info(fbe_cmi_io_float_data_t * float_data, fbe_packet_t * packet_p)
{
    fbe_payload_ex_t * payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_payload_block_operation_t *block_payload_p = NULL;
    fbe_payload_control_operation_t *control_payload_p = NULL;
    fbe_cmi_io_packet_status_t *response_info = &float_data->msg.response_info;
    
    /* Get status from packet */
    response_info->packet_status.code = fbe_transport_get_status_code(packet_p);
    response_info->packet_status.qualifier = fbe_transport_get_status_qualifier(packet_p);
    if (float_data->payload_opcode == FBE_PAYLOAD_OPCODE_BLOCK_OPERATION)
    {
        block_payload_p = fbe_payload_ex_get_block_operation(payload_p);
        response_info->u.block_status.block_status = block_payload_p->status;
        response_info->u.block_status.block_status_qualifier = block_payload_p->status_qualifier;
        response_info->u.block_status.retry_msecs = payload_p->retry_wait_msecs;
        response_info->u.block_status.bad_lba = payload_p->media_error_lba;
        response_info->u.block_status.physical_drive_service_time_ms = packet_p->physical_drive_service_time_ms;
    }
    else if (float_data->payload_opcode == FBE_PAYLOAD_OPCODE_CONTROL_OPERATION)
    {
        control_payload_p = fbe_payload_ex_get_control_operation(payload_p);
        float_data->fixed_data_length = 0;
        response_info->u.control_status.control_status = control_payload_p->status;
        response_info->u.control_status.control_status_qualifier = control_payload_p->status_qualifier;
    }
    else
    {
        fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR, "%s opcode 0x%x not supported\n", __FUNCTION__, float_data->payload_opcode);
        return FBE_STATUS_GENERIC_FAILURE;    
    }
    fbe_cmi_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, "%s slot %d_%d\n", __FUNCTION__,
                  float_data->pool, float_data->slot);
   
    return FBE_STATUS_OK;    
}
/****************************************************************
 * end fbe_cmi_io_fill_response_info()
 ****************************************************************/

/*!**************************************************************
 * fbe_cmi_io_reconstruct_packet()
 ****************************************************************
 * @brief
 *  This function reconstructs a packet when an IO request is received
 *  from peer. 
 *
 * @param packet_p - pointer to packet
 * @param float_data - floating data structure
 *
 * @return fbe_status_t
 * 
 * @author
 *  05/15/2012 - Created. Lili Chen
 *
 ****************************************************************/
fbe_status_t fbe_cmi_io_reconstruct_packet(fbe_packet_t * packet_p, fbe_cmi_io_float_data_t * float_data)
{
    fbe_payload_ex_t * payload_p = NULL;
    fbe_payload_block_operation_t *block_operation_p = NULL;
    fbe_payload_control_operation_t *control_operation_p = NULL;
    fbe_sg_element_t * sg_list;
    fbe_cmi_io_packet_request_t *request_info = &float_data->msg.request_info;
    fbe_status_t status;

    /* Save sg_list before initialize packet */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_payload_ex_get_sg_list(payload_p, &sg_list, NULL);

    fbe_transport_reuse_packet(packet_p);
    fbe_payload_ex_allocate_memory_operation(payload_p);

    packet_p->package_id = FBE_PACKAGE_ID_SEP_0;
    packet_p->service_id = FBE_SERVICE_ID_TOPOLOGY;
    packet_p->class_id = FBE_CLASS_ID_INVALID;
    packet_p->object_id = request_info->object_id;
    packet_p->packet_attr = request_info->packet_attr | FBE_PACKET_FLAG_REDIRECTED;
    packet_p->cpu_id = request_info->cpu_id;
    packet_p->packet_status.code = FBE_STATUS_GENERIC_FAILURE;
    packet_p->packet_status.qualifier = 0;
    packet_p->packet_priority = request_info->packet_priority;
    packet_p->traffic_priority = request_info->traffic_priority;
    packet_p->physical_drive_service_time_ms = request_info->physical_drive_service_time_ms;
    /* Restore sg_list */
    fbe_payload_ex_set_sg_list(payload_p, sg_list, 1);

    if (float_data->payload_opcode == FBE_PAYLOAD_OPCODE_BLOCK_OPERATION)
    {
        block_operation_p = fbe_payload_ex_allocate_block_operation(payload_p);
        status = fbe_payload_block_build_operation(block_operation_p,
                                                   request_info->u.block_payload.block_opcode,
                                                   request_info->u.block_payload.lba,
                                                   request_info->u.block_payload.block_count,
                                                   request_info->u.block_payload.block_size,
                                                   request_info->u.block_payload.optimum_block_size,
                                                   NULL);
        block_operation_p->block_flags = request_info->u.block_payload.block_flags;
        block_operation_p->payload_sg_descriptor = request_info->u.block_payload.payload_sg_descriptor;
        fbe_payload_ex_increment_block_operation_level(payload_p);

        sg_list[0].count = float_data->fixed_data_length;
    }
    else if (float_data->payload_opcode == FBE_PAYLOAD_OPCODE_CONTROL_OPERATION)
    {
        control_operation_p = fbe_payload_ex_allocate_control_operation(payload_p);
        status = fbe_payload_control_build_operation(control_operation_p,
                                                     request_info->u.control_payload.control_opcode,
                                                     sg_list[0].address,
                                                     request_info->u.control_payload.buffer_length);
        /* Set packet address.*/
        fbe_transport_set_address(packet_p,
                                  FBE_PACKAGE_ID_SEP_0,
                                  FBE_SERVICE_ID_TOPOLOGY,
                                  FBE_CLASS_ID_INVALID,
                                  request_info->object_id); 
        fbe_transport_set_packet_attr(packet_p, FBE_PACKET_FLAG_NO_ATTRIB);
    }
    else
    {
        return FBE_STATUS_GENERIC_FAILURE;    
    }

    return FBE_STATUS_OK;
}
/****************************************************************
 * end fbe_cmi_io_reconstruct_packet()
 ****************************************************************/

/*!**************************************************************
 * fbe_cmi_io_reconstruct_packet_status()
 ****************************************************************
 * @brief
 *  This function reconstructs the status of a packet when an IO 
 *  response is received from peer. 
 *
 * @param packet_p - pointer to packet
 * @param float_data - floating data structure
 *
 * @return fbe_status_t
 * 
 * @author
 *  05/15/2012 - Created. Lili Chen
 *
 ****************************************************************/
fbe_status_t fbe_cmi_io_reconstruct_packet_status(fbe_packet_t * packet_p, fbe_cmi_io_float_data_t * float_data)
{
    fbe_payload_ex_t * payload_p = NULL;
    fbe_payload_block_operation_t *block_operation_p = NULL;
    fbe_payload_control_operation_t *control_operation_p = NULL;
    fbe_cmi_io_packet_status_t *response_info = &float_data->msg.response_info;

    payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_transport_set_status(packet_p, 
                             response_info->packet_status.code, 
                             response_info->packet_status.qualifier);

    if (float_data->payload_opcode == FBE_PAYLOAD_OPCODE_BLOCK_OPERATION)
    {
        block_operation_p = fbe_payload_ex_get_block_operation(payload_p);
        fbe_payload_block_set_status(block_operation_p, 
                                     response_info->u.block_status.block_status,
                                     response_info->u.block_status.block_status_qualifier);    
        fbe_payload_ex_set_retry_wait_msecs(payload_p, response_info->u.block_status.retry_msecs);
        fbe_payload_ex_set_media_error_lba(payload_p, response_info->u.block_status.bad_lba);
        packet_p->physical_drive_service_time_ms = response_info->u.block_status.physical_drive_service_time_ms;
        fbe_cmi_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, "%s slot %d_%d, status 0x%x, block_status 0x%x\n", __FUNCTION__,
                      float_data->pool, float_data->slot, response_info->packet_status.code, response_info->u.block_status.block_status);
    }
    else if (float_data->payload_opcode == FBE_PAYLOAD_OPCODE_CONTROL_OPERATION)
    {
        control_operation_p = fbe_payload_ex_get_control_operation(payload_p);
        fbe_payload_control_set_status(control_operation_p, response_info->u.control_status.control_status);
        fbe_payload_control_set_status_qualifier(control_operation_p, response_info->u.control_status.control_status_qualifier);
    }
    else
    {
        return FBE_STATUS_GENERIC_FAILURE;    
    }
    
    return FBE_STATUS_OK;
}
/****************************************************************
 * end fbe_cmi_io_reconstruct_packet_status()
 ****************************************************************/

/*!**************************************************************
 * fbe_cmi_io_read_request_preprocess()
 ****************************************************************
 * @brief
 *  This function processes read request: it copies sg lists to  
 *  context data area to be transferred to peer. 
 *
 * @param
 *   float_data  - pointer to packet information.
 *   packet_p  - pointer to packet.
 *   max_sgl_count - max sg list count to be used.
 *   read_sgl_count - sg list count, including ending sg.
 *   read_copy_needed  - whether copy is needed.
 *
 * @return fbe_status_t
 * 
 * @author
 *  06/11/2013 - Created. Lili Chen
 *
 ****************************************************************/
fbe_status_t fbe_cmi_io_read_request_preprocess(fbe_cmi_io_context_info_t * ctx_info, 
                                                fbe_packet_t * packet_p, 
                                                fbe_u32_t max_sgl_count,
                                                fbe_u32_t * read_sgl_count,
                                                fbe_bool_t * read_copy_needed)
{
    fbe_payload_ex_t *              payload_p = NULL;
    fbe_cmi_io_float_data_t * float_data = &ctx_info->float_data;
    fbe_sg_element_t *sg_list = NULL;
    fbe_u32_t sg_list_count = 0;
    fbe_u64_t bytes_found;
    fbe_u32_t sgl_count = 0;

    *read_sgl_count = 0;
    *read_copy_needed = FBE_FALSE;

    payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_payload_ex_get_sg_list(payload_p, &sg_list, &sg_list_count);
    sg_list_count = fbe_sg_element_count_entries_and_bytes(sg_list, float_data->fixed_data_length, &bytes_found);
    if ((sg_list_count >= max_sgl_count) || cmi_service_get_peer_version() == FBE_TRUE)
    {
        fbe_cmi_trace(FBE_TRACE_LEVEL_DEBUG_LOW, "%s: sg list count %d, peer %d use ctx sg_list instead\n", 
                      __FUNCTION__, sg_list_count, cmi_service_get_peer_version());
        ctx_info->sg_element[0].count = float_data->fixed_data_length;
        sg_list = ctx_info->sg_element;
        *read_copy_needed = FBE_TRUE;
    }

    if (sg_list != NULL)
    {
        fbe_u32_t data_length = 0;
        fbe_sg_element_t * sg_ptr = sg_list;
        CMI_PHYSICAL_SG_ELEMENT * sg_dest_ptr = (CMI_PHYSICAL_SG_ELEMENT *)(ctx_info->data);
        fbe_u32_t          bytes_to_copy;
        fbe_u32_t          bytes_remaining = float_data->fixed_data_length;

        while ((bytes_remaining != 0) && (sg_ptr->count != 0) &&
               (sgl_count < (max_sgl_count - 1)))
        {
            bytes_to_copy = FBE_MIN(bytes_remaining, sg_ptr->count);
            sg_dest_ptr->PhysAddress = fbe_cmi_io_get_physical_address(sg_ptr->address);
            sg_dest_ptr->length = bytes_to_copy;
            bytes_remaining -= bytes_to_copy;
            data_length += bytes_to_copy;
            sgl_count++;
            sg_ptr++;
            sg_dest_ptr++;
        }
        if (data_length != float_data->fixed_data_length) {
            fbe_cmi_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, "%s data len 0x%x not as expected 0x%x sgl %d\n", __FUNCTION__,
                          data_length, float_data->fixed_data_length, sgl_count);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        sg_dest_ptr->PhysAddress = 0;
        sg_dest_ptr->length = 0;
        sgl_count++;
    }
    else
    {
        fbe_cmi_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, "%s: sg list is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    *read_sgl_count = sgl_count;
    return FBE_STATUS_OK;
}
/****************************************************************
 * end fbe_cmi_io_read_request_preprocess()
 ****************************************************************/

/*!**************************************************************
 * fbe_cmi_io_write_request_preprocess()
 ****************************************************************
 *  This function processes write request: if sg list exceeds we 
 *  need to copy the write data to context data area. 
 *
 * @param
 *   float_data  - pointer to packet information.
 *   packet_p  - pointer to packet.
 *   sg_list - source sg list.
 *
 * @return fbe_status_t
 * 
 * @author
 *  06/11/2013 - Created. Lili Chen
 *
 ****************************************************************/
fbe_status_t fbe_cmi_io_write_request_preprocess(fbe_cmi_io_context_info_t * ctx_info, 
                                                 fbe_packet_t * packet_p, 
                                                 fbe_sg_element_t **sg_list)
{
    fbe_payload_ex_t *              payload_p = NULL;
    fbe_cmi_io_float_data_t * float_data = &ctx_info->float_data;
    fbe_u32_t sg_list_count = 0;
    fbe_u64_t bytes_found;

    *sg_list = NULL;
    payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_payload_ex_get_sg_list(payload_p, sg_list, &sg_list_count);
    sg_list_count = fbe_sg_element_count_entries_and_bytes(*sg_list, float_data->fixed_data_length, &bytes_found);
    if (sg_list_count >= MAX_SEP_IO_SG_COUNT)
    {
        fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, "%s: sg list count %d, use ctx sg_list instead\n", 
                      __FUNCTION__, sg_list_count);
        fbe_cmi_io_copy_sg_data(packet_p, ctx_info, FBE_FALSE);
        ctx_info->sg_element[0].count = float_data->fixed_data_length;
        *sg_list = ctx_info->sg_element;
    }

    return FBE_STATUS_OK;
}
/****************************************************************
 * end fbe_cmi_io_write_request_preprocess()
 ****************************************************************/

/*!**************************************************************
 * fbe_cmi_io_context_info_to_context()
 ****************************************************************
 * @brief
 *  This function gets context from ctx_info structure. 
 *
 * @param ctx_info - pointer to ctx_info
 *
 * @return context
 * 
 * @author
 *  05/15/2012 - Created. Lili Chen
 *
 ****************************************************************/
static __forceinline fbe_cmi_io_context_t * 
fbe_cmi_io_context_info_to_context(fbe_cmi_io_context_info_t * ctx_info)
{
    return (fbe_cmi_io_context_t *)((fbe_u8_t *)ctx_info - (fbe_u8_t *)(&((fbe_cmi_io_context_t  *)0)->context_info));
}
/****************************************************************
 * end fbe_cmi_io_context_info_to_context()
 ****************************************************************/

/*!**************************************************************
 * fbe_cmi_io_enqueue_to_waiting_q()
 ****************************************************************
 * @brief
 *  This function enqueues a packet to waiting queue. 
 *
 * @param packet_p - pointer to context
 *
 * @return fbe_status_t
 * 
 * @author
 *  05/15/2012 - Created. Lili Chen
 *
 ****************************************************************/
fbe_status_t fbe_cmi_io_enqueue_to_waiting_q(fbe_packet_t * packet_p)
{
    fbe_status_t status;

    fbe_transport_set_cancel_function(packet_p, fbe_cmi_io_cancel_function, NULL);
    fbe_spinlock_lock(&waiting_q_lock);
    status = fbe_transport_enqueue_packet(packet_p, &sep_io_waiting_queue_head);
    fbe_spinlock_unlock(&waiting_q_lock);
    if (status != FBE_STATUS_OK)
    {
        fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR, "%s: error to enqueue status 0x%x \n", __FUNCTION__, status);
    }

    fbe_transport_record_callback_with_action(packet_p, 
                                              (fbe_packet_completion_function_t)fbe_cmi_io_enqueue_to_waiting_q, 
                                              PACKET_STACK_COMPLETE);

    fbe_cmi_io_start_from_waiting_q();

    return status;
}
/****************************************************************
 * end fbe_cmi_io_enqueue_to_waiting_q()
 ****************************************************************/

/*!**************************************************************
 * fbe_cmi_io_dequeue_from_waiting_q()
 ****************************************************************
 * @brief
 *  This function dequeues a packet from waiting queue. 
 *
 * @param None
 *
 * @return fbe_packet_t
 * 
 * @author
 *  05/15/2012 - Created. Lili Chen
 *
 ****************************************************************/
fbe_packet_t * fbe_cmi_io_dequeue_from_waiting_q(void)
{
    fbe_packet_t * packet_p = NULL;

    fbe_spinlock_lock(&waiting_q_lock);
    packet_p = fbe_transport_dequeue_packet(&sep_io_waiting_queue_head);
    fbe_spinlock_unlock(&waiting_q_lock);

    return packet_p;
}
/****************************************************************
 * end fbe_cmi_io_dequeue_from_waiting_q()
 ****************************************************************/

/*!**************************************************************
 * fbe_cmi_io_start_from_waiting_q_callback()
 ****************************************************************
 * @brief
 *  This is the callback function from transport run queue to start
 *  processing from waiting queue. 
 *
 * @param packet_p - Pointer to the packet
 * @param context - Pointer to the context
 *
 * @return fbe_status_t
 * 
 * @author
 *  05/15/2012 - Created. Lili Chen
 *
 ****************************************************************/
static fbe_status_t 
fbe_cmi_io_start_from_waiting_q_callback(fbe_packet_t * in_packet_p, fbe_packet_completion_context_t context)
{
    fbe_cmi_io_context_info_t * ctx_info = (fbe_cmi_io_context_info_t *)context;
    fbe_packet_t * packet_p = in_packet_p;
    fbe_status_t status;

    if (packet_p && ctx_info)
    {
        fbe_cmi_trace(FBE_TRACE_LEVEL_DEBUG_LOW, "%s packet %p slot %d_%d\n", __FUNCTION__, packet_p, ctx_info->pool, ctx_info->slot);
        fbe_cmi_io_start_packet_enqueue_context(packet_p, ctx_info);
        status = fbe_cmi_io_start_packet_with_context(packet_p, ctx_info);
        if (status != FBE_STATUS_OK)
        {
            fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, "%s: start packet %p failed status 0x%x peer valid %d\n", __FUNCTION__, packet_p, status, peer_table.table_valid);
            fbe_cmi_io_start_packet_failed(ctx_info);
        }
    }

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/****************************************************************
 * end fbe_cmi_io_start_from_waiting_q_callback()
 ****************************************************************/

/*!**************************************************************
 * fbe_cmi_io_start_from_waiting_q()
 ****************************************************************
 * @brief
 *  This function starts packets from waiting queue. 
 *
 * @param None
 *
 * @return fbe_status_t
 * 
 * @author
 *  05/15/2012 - Created. Lili Chen
 *
 ****************************************************************/
fbe_status_t fbe_cmi_io_start_from_waiting_q(void)
{
    fbe_packet_t *packet_p = NULL;
    fbe_cmi_io_context_info_t * ctx_info_p;
    fbe_queue_element_t * queue_element;
    fbe_queue_head_t tmp_queue;

    if (!peer_table.table_valid || !local_table.table_valid)
    {
        return FBE_STATUS_OK;
    }

    fbe_spinlock_lock(&waiting_q_lock);
    if (fbe_queue_is_empty(&sep_io_waiting_queue_head))
    {
        fbe_spinlock_unlock(&waiting_q_lock);
        return FBE_STATUS_OK;
    }

    fbe_queue_init(&tmp_queue);
    /* This is the only place we try to get 2 spinlocks */
    fbe_spinlock_lock(&send_context_lock);

    queue_element = fbe_queue_front(&sep_io_waiting_queue_head);
    while (queue_element)
    {
        packet_p = fbe_transport_queue_element_to_packet(queue_element);
        ctx_info_p = fbe_cmi_io_get_send_context(packet_p);
        if (ctx_info_p == NULL)
        {
            break;
        }
        fbe_cmi_trace(FBE_TRACE_LEVEL_DEBUG_LOW, "%s packet %p slot %d_%d\n", __FUNCTION__, packet_p, ctx_info_p->pool, ctx_info_p->slot);
        fbe_transport_remove_packet_from_queue(packet_p);
        fbe_transport_set_completion_function(packet_p, fbe_cmi_io_start_from_waiting_q_callback, ctx_info_p);
        fbe_queue_push(&tmp_queue, queue_element);
        queue_element = fbe_queue_front(&sep_io_waiting_queue_head);
    }

    fbe_spinlock_unlock(&send_context_lock);
    fbe_spinlock_unlock(&waiting_q_lock);

    /* Enqueue the request to run queue. */
    if(!fbe_queue_is_empty(&tmp_queue)){
        fbe_transport_run_queue_push(&tmp_queue, NULL, NULL);
    }

    fbe_queue_destroy(&tmp_queue);
    return FBE_STATUS_OK;
}
/****************************************************************
 * end fbe_cmi_io_start_from_waiting_q()
 ****************************************************************/

/*!**************************************************************
 * fbe_cmi_io_enqueue_context_info_to_q()
 ****************************************************************
 * @brief
 *  This function enqueues a context to send/receive/outstanding
 *  queue. 
 *
 * @param ctx_info - pointer to context info.
 * @param queue_head - pointer to queue head.
 * @param lock - pointer to spin lock.
 *
 * @return none
 * 
 * @author
 *  05/15/2012 - Created. Lili Chen
 *
 ****************************************************************/
void 
fbe_cmi_io_enqueue_context_info_to_q(fbe_cmi_io_context_info_t * ctx_info,
                                            fbe_queue_head_t * queue_head,
                                            fbe_spinlock_t *lock)
{
    fbe_cmi_io_context_t * ctx_ptr = fbe_cmi_io_context_info_to_context(ctx_info);

    if (fbe_queue_is_element_on_queue(&ctx_ptr->queue_element))
    {
        fbe_cmi_trace(FBE_TRACE_LEVEL_WARNING, "%s: context %p already in queue\n", __FUNCTION__, ctx_ptr);
        return;
    }
    /* Queue to queue */
    fbe_spinlock_lock(lock);
    fbe_queue_push(queue_head, &ctx_ptr->queue_element);
    fbe_spinlock_unlock(lock);

    return;
}
/****************************************************************
 * end fbe_cmi_io_enqueue_context_info_to_q()
 ****************************************************************/

/*!**************************************************************
 * fbe_cmi_io_dequeue_context_info_from_q()
 ****************************************************************
 * @brief
 *  This function dequeues a context to send/receive/outstanding
 *  queue. 
 *
 * @param pool - pool to dequeue.
 * @param slot - slot to dequeue.
 * @param queue_head - pointer to queue head.
 * @param lock - pointer to spin lock.
 *
 * @return ctx_info
 * 
 * @author
 *  05/15/2012 - Created. Lili Chen
 *
 ****************************************************************/
fbe_cmi_io_context_info_t * 
fbe_cmi_io_dequeue_context_info_from_q(fbe_u32_t pool, fbe_u32_t slot,
                                       fbe_queue_head_t * queue_head,
                                       fbe_spinlock_t *lock)
{
    fbe_cmi_io_context_t * ctx_ptr;
    fbe_cmi_io_context_info_t * ctx_info;
    fbe_queue_element_t * queue_element;

    fbe_spinlock_lock(lock);
    queue_element = fbe_queue_front(queue_head);
    while(queue_element)
    {
        ctx_ptr = (fbe_cmi_io_context_t *)queue_element;
        ctx_info = &ctx_ptr->context_info;
        if ((ctx_info->float_data.pool == pool) && (ctx_info->float_data.slot == slot))
        {
            /* Dequeue context */
            fbe_queue_remove(queue_element);
            fbe_spinlock_unlock(lock);
            return ctx_info;
        }

        queue_element = fbe_queue_next(queue_head, queue_element);
    }
    fbe_spinlock_unlock(lock);

    return NULL;
}
/****************************************************************
 * end fbe_cmi_io_dequeue_context_info_from_q()
 ****************************************************************/

/*!**************************************************************
 * fbe_cmi_io_get_send_context()
 ****************************************************************
 * @brief
 *  This function gets send context from queue. 
 *  The caller should obtain the lock already.
 *
 * @param packet_p - pointer to the packet.
 *
 * @return send_context
 * 
 * @author
 *  05/15/2012 - Created. Lili Chen
 *
 ****************************************************************/
fbe_cmi_io_context_info_t * fbe_cmi_io_get_send_context(fbe_packet_t * packet_p)
{
    fbe_cmi_io_context_t *		ctx_ptr = NULL;
    fbe_u32_t                   data_length = 0;
    fbe_u32_t                   i, pool_id;

    fbe_cmi_io_get_packet_data_length(packet_p, &data_length);
    if (data_length > LARGE_SEP_IO_DATA_LENGTH)
    {
        fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR,
                      "%s data length exceeds limit 0x%x\n",
                      __FUNCTION__, data_length);
        return NULL;
    }

    /* Look for a send context in queue */
    for (i = MAX_SEP_IO_DATA_POOLS; i > 0; i--) 
    {
        pool_id = i - 1;
        if (!fbe_queue_is_empty(&sep_io_send_context_head[pool_id]) && (data_length <= pool_data_length[pool_id])) {
            ctx_ptr = (fbe_cmi_io_context_t *)fbe_queue_pop(&sep_io_send_context_head[pool_id]);
            return (&ctx_ptr->context_info);
        }
    }

    return NULL;
}
/****************************************************************
 * end fbe_cmi_io_get_send_context()
 ****************************************************************/

/*!**************************************************************
 * fbe_cmi_io_drain_outstanding_queue()
 ****************************************************************
 * @brief
 *  This function drains outstanding queue when peer is dead. 
 *
 * @param None
 *
 * @return fbe_status_t
 * 
 * @author
 *  05/15/2012 - Created. Lili Chen
 *
 ****************************************************************/
fbe_status_t fbe_cmi_io_drain_outstanding_queue(void)
{
    fbe_cmi_io_context_t * ctx_ptr = NULL;
    fbe_cmi_io_context_info_t * ctx_info = NULL;
    fbe_packet_t * packet_p;
    fbe_queue_head_t * queue_head = &sep_io_outstanding_queue_head;
    fbe_queue_element_t * queue_element;
    fbe_spinlock_t	* lock = &outstand_q_lock;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t i = 0;

    fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, "%s ENTRY\n", __FUNCTION__);
    fbe_spinlock_lock(lock);
    queue_element = fbe_queue_front(queue_head);
    while (queue_element)
    {
        ctx_ptr = (fbe_cmi_io_context_t *)queue_element;
        ctx_info = &ctx_ptr->context_info;

        if ((ctx_info->context_attr & FBE_CMI_IO_CONTEXT_ATTR_HOLD) == 0)
        {
            fbe_queue_remove(queue_element);
            fbe_spinlock_unlock(lock);
            packet_p = (fbe_packet_t *)ctx_ptr->context_info.float_data.original_packet;
            ctx_ptr->context_info.float_data.original_packet = NULL;
            if (packet_p)
            {
                fbe_cmi_io_dequeue_check_cancel(queue_head, packet_p);
                fbe_transport_set_status(packet_p, FBE_STATUS_FAILED, 0);
                fbe_transport_complete_packet(packet_p);
            }

            fbe_cmi_io_enqueue_context_info_to_q(ctx_info, &sep_io_send_context_head[ctx_info->pool], &send_context_lock);
            fbe_spinlock_lock(lock);
            queue_element = fbe_queue_front(queue_head);
        }
        else
        {
            status = FBE_STATUS_BUSY;
            queue_element = fbe_queue_next(queue_head, queue_element);
        }
        i++;
        if(i > 0x0000ffff){
            fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR, "%s Critical error looped queue\n", __FUNCTION__);
            fbe_spinlock_unlock(lock);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }
    fbe_spinlock_unlock(lock);

    return status;
}
/****************************************************************
 * end fbe_cmi_io_drain_outstanding_queue()
 ****************************************************************/

/*!**************************************************************
 * fbe_cmi_io_drain_waiting_queue()
 ****************************************************************
 * @brief
 *  This function drains waiting queue when peer is dead. 
 *
 * @param None
 *
 * @return fbe_status_t
 * 
 * @author
 *  05/15/2012 - Created. Lili Chen
 *
 ****************************************************************/
fbe_status_t fbe_cmi_io_drain_waiting_queue(void)
{
    fbe_packet_t *packet_p = NULL;

    fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, "%s ENTRY\n", __FUNCTION__);
    while ((packet_p = fbe_cmi_io_dequeue_from_waiting_q()) != NULL)
    {
        fbe_transport_set_status(packet_p, FBE_STATUS_FAILED, 0);
        fbe_transport_complete_packet(packet_p);
    }

    return FBE_STATUS_OK;
}
/****************************************************************
 * end fbe_cmi_io_drain_waiting_queue()
 ****************************************************************/


/*!**************************************************************
 * fbe_cmi_io_send_translation_info()
 ****************************************************************
 * @brief
 *  This function sends local translation table to peer. 
 *
 * @param None
 *
 * @return None
 * 
 * @author
 *  05/22/2012 - Created. Lili Chen
 *
 ****************************************************************/
void fbe_cmi_io_send_translation_info(void)
{
    fbe_atomic_t old_val;

    if (!local_table.table_valid)
    {
        return;
    }

    old_val = fbe_atomic_exchange(&table_context->in_use, FBE_TRUE);
    if (old_val == FBE_TRUE)
    {
        /* We are transmitting translation table */
        return;
    }

    fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, "%s ENTRY\n", __FUNCTION__);
    /*set up the message*/
    table_context->float_data.original_packet = NULL;
    table_context->float_data.msg_type = FBE_CMI_IO_MESSAGE_TYPE_TRANSLATION_INFO;
    table_context->float_data.msg.translation_info.table_valid = peer_table.table_valid;
    fbe_copy_memory(table_context->float_data.msg.translation_info.send_slot_address, local_table.send_slot_address, 
                    2*MAX_SEP_IO_DATA_POOLS*sizeof(fbe_u64_t));

    /*and send it to the peer, we will continue processing with the ack*/
    fbe_cmi_send_message(FBE_CMI_CLIENT_ID_SEP_IO,
                         sizeof(fbe_cmi_io_float_data_t),
                         (fbe_cmi_message_t)&(table_context->float_data),
                         NULL);

    return;
}
/****************************************************************
 * end fbe_cmi_io_send_translation_info()
 ****************************************************************/

/*!**************************************************************
 * fbe_cmi_io_receive_translation_info()
 ****************************************************************
 * @brief
 *  This function process messages about peer translation table. 
 *
 * @param float_data - float data
 *
 * @return fbe_status_t
 * 
 * @author
 *  05/22/2012 - Created. Lili Chen
 *
 ****************************************************************/
fbe_status_t fbe_cmi_io_receive_translation_info(fbe_cmi_io_float_data_t * float_data)
{
    fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, "%s ENTRY\n", __FUNCTION__);

    /* Fill in information */
    fbe_copy_memory(peer_table.send_slot_address, float_data->msg.translation_info.send_slot_address,
                    2*MAX_SEP_IO_DATA_POOLS*sizeof(fbe_u64_t));
    peer_table.table_valid = FBE_TRUE;

    /* Send a msg to peer if needed */
    if (float_data->msg.translation_info.table_valid == FBE_FALSE)
    {
        fbe_cmi_io_send_translation_info();
    }

    fbe_cmi_io_start_from_waiting_q();

    return FBE_STATUS_OK;
}
/****************************************************************
 * end fbe_cmi_io_receive_translation_info()
 ****************************************************************/

/*!**************************************************************
 * fbe_cmi_io_copy_sg_data()
 ****************************************************************
 * @brief
 *  This function copies data between packet and context data area. 
 *
 * @param packet_p - pointer to packet
 * @param ctx_info - pointer to context
 * @param to_packet - TRUE if copy from context to packet
 *
 * @return fbe_status_t
 * 
 * @author
 *  05/22/2012 - Created. Lili Chen
 *
 ****************************************************************/
fbe_status_t fbe_cmi_io_copy_sg_data(fbe_packet_t *packet_p, 
                                            fbe_cmi_io_context_info_t *ctx_info,
                                            fbe_bool_t to_packet)
{
    fbe_payload_ex_t *	payload_p;
    fbe_sg_element_t *	sg_element;
    fbe_u8_t *          data_p = (fbe_u8_t *)ctx_info->data;
    fbe_u32_t           total_bytes = 0;
    fbe_u32_t           bytes_to_copy;
    fbe_u32_t           bytes_remaining = ctx_info->float_data.fixed_data_length;

    fbe_cmi_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, "%s packet %p\n", __FUNCTION__, packet_p);
    payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_payload_ex_get_sg_list(payload_p, &sg_element, NULL);

    /* We will not copy back more bytes than were requested in ctx_info->float_data.fixed_data_length. 
     * This is since the sg list might not be null terminated or it might have more data in it than requested. 
     */
    while ((sg_element->count != 0) && (bytes_remaining != 0)) {
        bytes_to_copy = FBE_MIN(bytes_remaining, sg_element->count);
        if (to_packet)
        {
            fbe_copy_memory (sg_element->address, data_p, bytes_to_copy);
        }
        else
        {
            fbe_copy_memory (data_p, sg_element->address, bytes_to_copy);
        }
        total_bytes += bytes_to_copy;
        bytes_remaining -= bytes_to_copy;
        data_p += bytes_to_copy;
        sg_element ++;
    }

    if (total_bytes != ctx_info->float_data.fixed_data_length)
    {
        fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR, "%s data len 0x%x not as expected 0x%x packet %p\n", __FUNCTION__,
                      total_bytes, ctx_info->float_data.fixed_data_length, packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;
}
/****************************************************************
 * end fbe_cmi_io_copy_sg_data()
 ****************************************************************/

/*!**************************************************************
 * fbe_cmi_io_return_held_context()
 ****************************************************************
 * @brief
 *  This function is invoked by irp completion function to see 
 *  whether there is anything to do with held context. 
 *
 * @param ctx_info - context info.
 *
 * @return fbe_status_t
 * 
 * @author
 *  06/08/2012 - Created. Lili Chen
 *
 ****************************************************************/
fbe_status_t fbe_cmi_io_return_held_context(fbe_cmi_io_context_info_t *ctx_info)
{
    fbe_status_t status  = FBE_STATUS_GENERIC_FAILURE;
    fbe_cmi_io_context_t * ctx_ptr = fbe_cmi_io_context_info_to_context(ctx_info);

    fbe_spinlock_lock(&ctx_ptr->context_lock);
    if (!(ctx_info->context_attr & FBE_CMI_IO_CONTEXT_ATTR_HOLD))
    {
        fbe_spinlock_unlock(&ctx_ptr->context_lock);
        fbe_cmi_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, "%s slot %d_%d NOT HELD!\n", __FUNCTION__, ctx_info->pool, ctx_info->slot);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    ctx_info->context_attr &= ~FBE_CMI_IO_CONTEXT_ATTR_HOLD;

    if (!(ctx_info->context_attr & FBE_CMI_IO_CONTEXT_ATTR_NEED_START))
    {
        /* Packet not completed */
        fbe_spinlock_unlock(&ctx_ptr->context_lock);
        /* Drain outstanding queue again */
        if (!peer_table.table_valid && 
            (ctx_info->float_data.msg_type == FBE_CMI_IO_MESSAGE_TYPE_PACKET_REQUEST))
        {
            fbe_cmi_io_drain_outstanding_queue();
        }
        return FBE_STATUS_OK;
    }
    ctx_info->context_attr &= ~FBE_CMI_IO_CONTEXT_ATTR_NEED_START;
    fbe_spinlock_unlock(&ctx_ptr->context_lock);

    fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, "%s slot %d_%d\n", __FUNCTION__, ctx_info->pool, ctx_info->slot);
    if (ctx_info->float_data.msg_type == FBE_CMI_IO_MESSAGE_TYPE_PACKET_RESPONSE)
    {
        /* Receive context */
        fbe_cmi_io_start_packet_enqueue_context(ctx_info->packet, ctx_info);
        status = fbe_cmi_io_start_packet_with_context(ctx_info->packet, ctx_info);
        if (status != FBE_STATUS_OK)
        {
            fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, "%s: start packet %p failed status 0x%x \n", __FUNCTION__, ctx_info->packet, status);
        }
    }
    else
    {
        /* Send context */
        fbe_cmi_io_enqueue_context_info_to_q(ctx_info, &sep_io_send_context_head[ctx_info->pool], &send_context_lock);
        fbe_cmi_io_start_from_waiting_q();
    }

    return FBE_STATUS_OK;
}
/****************************************************************
 * end fbe_cmi_io_return_held_context()
 ****************************************************************/


/*!**************************************************************
 * fbe_cmi_io_scan_for_cancelled_packets()
 ****************************************************************
 * @brief
 *  This function scans queues for cancelled packets. 
 *
 * @param None
 *
 * @return fbe_status_t
 * 
 * @author
 *  06/04/2012 - Created. Lili Chen
 *
 ****************************************************************/
fbe_status_t fbe_cmi_io_scan_for_cancelled_packets(void)
{
    fbe_queue_element_t *       queue_element = NULL;
    fbe_queue_head_t *          queue_head;
    fbe_packet_t *              packet_p;
    fbe_status_t                status;
    fbe_cmi_io_context_t *      ctx_ptr = NULL;

    /* Scan the waiting queue first.  
     */
    fbe_spinlock_lock(&waiting_q_lock);
    queue_head = &sep_io_waiting_queue_head;
    queue_element = fbe_queue_front(queue_head);
    while(queue_element){
        packet_p = fbe_transport_queue_element_to_packet(queue_element);
        status  = fbe_transport_get_status_code(packet_p);
        if(status == FBE_STATUS_CANCEL_PENDING){
            fbe_transport_remove_packet_from_queue(packet_p);
            fbe_transport_set_status(packet_p, FBE_STATUS_CANCELED, 0);
            fbe_spinlock_unlock(&waiting_q_lock);
            fbe_transport_complete_packet(packet_p);
            fbe_spinlock_lock(&waiting_q_lock);
            queue_element = fbe_queue_front(queue_head);
        } else {
            queue_element = fbe_queue_next(queue_head, queue_element);
        }
    }
    fbe_spinlock_unlock(&waiting_q_lock);

    /* Scan the outstanding queue.  
     */
    fbe_spinlock_lock(&outstand_q_lock);
    queue_head = &sep_io_outstanding_queue_head;
    queue_element = fbe_queue_front(queue_head);
    while(queue_element){
        ctx_ptr = (fbe_cmi_io_context_t *)queue_element;
        packet_p = (fbe_packet_t *)ctx_ptr->context_info.float_data.original_packet;
        status  = fbe_transport_get_status_code(packet_p);
        if (status == FBE_STATUS_CANCEL_PENDING &&
            !(ctx_ptr->context_info.context_attr & FBE_CMI_IO_CONTEXT_ATTR_ABORT_SENT))
        {
            fbe_atomic_t old_val;
            old_val = fbe_atomic_exchange(&abort_context->in_use, FBE_TRUE);
            if (old_val == FBE_FALSE)
            {
                ctx_ptr->context_info.context_attr |= FBE_CMI_IO_CONTEXT_ATTR_ABORT_SENT;
                fbe_spinlock_unlock(&outstand_q_lock);
                fbe_cmi_io_send_cancel_to_peer(packet_p, ctx_ptr->context_info.slot);
                return FBE_STATUS_OK;
            }
            break;
        } else {
            queue_element = fbe_queue_next(queue_head, queue_element);
        }
    }
    fbe_spinlock_unlock(&outstand_q_lock);

    return FBE_STATUS_OK;
}
/****************************************************************
 * end fbe_cmi_io_scan_for_cancelled_packets()
 ****************************************************************/

/*!**************************************************************
 * fbe_cmi_io_send_cancel_to_peer()
 ****************************************************************
 * @brief
 *  This function sends abort command to peer. 
 *
 * @param packet_p - pointer to a packet.
 * @param slot - slot number.
 *
 * @return fbe_status_t
 * 
 * @author
 *  06/08/2012 - Created. Lili Chen
 *
 ****************************************************************/
fbe_status_t fbe_cmi_io_send_cancel_to_peer(fbe_packet_t * packet_p, fbe_u32_t slot)
{
    fbe_cmi_io_float_data_t * float_data;

    float_data = &abort_context->float_data;
    float_data->msg_type = FBE_CMI_IO_MESSAGE_TYPE_PACKET_ABORT;
    float_data->slot = slot;
    float_data->original_packet = packet_p;

    fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, "%s send abort slot %d packet %p\n", __FUNCTION__, 
                  slot, packet_p);
    /* And send it to the peer*/
    fbe_cmi_send_message(FBE_CMI_CLIENT_ID_SEP_IO,
                         sizeof(fbe_cmi_io_float_data_t),
                         (fbe_cmi_message_t)float_data,
                         NULL);

    return FBE_STATUS_OK;
}
/****************************************************************
 * end fbe_cmi_io_send_cancel_to_peer()
 ****************************************************************/

/*!**************************************************************
 * fbe_cmi_io_process_packet_abort()
 ****************************************************************
 * @brief
 *  This function processes abort IO command from peer. 
 *
 * @param float_data - pointer to float data.
 *
 * @return fbe_status_t
 * 
 * @author
 *  06/08/2012 - Created. Lili Chen
 *
 ****************************************************************/
fbe_status_t fbe_cmi_io_process_packet_abort(fbe_cmi_io_float_data_t * float_data)
{
    fbe_cmi_io_context_t * ctx_ptr = NULL;
    fbe_packet_t * packet_p;
    fbe_queue_element_t *       queue_element = NULL;
    fbe_queue_head_t *          queue_head;

    fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, "%s received slot %d_%d orig packet %p\n", __FUNCTION__, 
                  float_data->pool, float_data->slot, float_data->original_packet);
    fbe_spinlock_lock(&receive_process_q_lock);
    queue_head = &sep_io_receive_processing_queue_head;
    queue_element = fbe_queue_front(queue_head);
    while(queue_element){
        ctx_ptr = (fbe_cmi_io_context_t *)queue_element;
        packet_p = (fbe_packet_t *)ctx_ptr->context_info.float_data.original_packet;
        if ((float_data->slot == ctx_ptr->context_info.float_data.slot) &&
            (float_data->original_packet == packet_p))
        {
            fbe_spinlock_unlock(&receive_process_q_lock);
            fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, "%s orig packet %p aborted\n", __FUNCTION__, packet_p);
            fbe_transport_cancel_packet(ctx_ptr->context_info.packet);
            return FBE_STATUS_OK;
        } else {
            queue_element = fbe_queue_next(queue_head, queue_element);
        }
    }
    fbe_spinlock_unlock(&receive_process_q_lock);

    return FBE_STATUS_OK;
}
/****************************************************************
 * end fbe_cmi_io_process_packet_abort()
 ****************************************************************/

/*!**************************************************************
 * fbe_cmi_io_enqueue_check_cancel()
 ****************************************************************
 * @brief
 *  This function checks cancel status when a packet is enqueued
 *  to outstanding queue. 
 *
 * @param queue_head - queue head.
 * @param packet_p - pointer to a packet.
 *
 * @return fbe_status_t
 * 
 * @author
 *  06/08/2012 - Created. Lili Chen
 *
 ****************************************************************/
fbe_status_t fbe_cmi_io_enqueue_check_cancel(fbe_queue_head_t * queue_head, fbe_packet_t * packet_p)
{
    fbe_packet_state_t old_state;

    if (queue_head != &sep_io_outstanding_queue_head)
    {
        return FBE_STATUS_OK;
    }

    /* Set cancel function */
    fbe_transport_set_cancel_function(packet_p, fbe_cmi_io_cancel_function, NULL);

    old_state = fbe_transport_exchange_state(packet_p, FBE_PACKET_STATE_QUEUED);
    switch(old_state) {
        case FBE_PACKET_STATE_IN_PROGRESS:
            break;
        case FBE_PACKET_STATE_CANCELED:
            if ((packet_p->packet_attr & FBE_PACKET_FLAG_DO_NOT_CANCEL) == 0)
            {
                fbe_transport_set_status(packet_p, FBE_STATUS_CANCEL_PENDING, 0);
                fbe_cmi_io_cancel_thread_notify();
            }
            break;
        default:
            fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR, "%s: Invalid packet state %X \n", __FUNCTION__, old_state);
            break;
    }

    return FBE_STATUS_OK;
}
/****************************************************************
 * end fbe_cmi_io_enqueue_check_cancel()
 ****************************************************************/

/*!**************************************************************
 * fbe_cmi_io_dequeue_check_cancel()
 ****************************************************************
 * @brief
 *  This function checks cancel status when a packet is dequeued
 *  from outstanding queue. 
 *
 * @param queue_head - queue head.
 * @param packet_p - pointer to a packet.
 *
 * @return fbe_status_t
 * 
 * @author
 *  06/08/2012 - Created. Lili Chen
 *
 ****************************************************************/
fbe_status_t fbe_cmi_io_dequeue_check_cancel(fbe_queue_head_t * queue_head, fbe_packet_t * packet_p)
{
    fbe_packet_state_t old_state;

    if (queue_head != &sep_io_outstanding_queue_head)
    {
        return FBE_STATUS_OK;
    }

    /* Set cancel function to NULL */
    fbe_transport_set_cancel_function(packet_p, NULL, NULL);

    old_state = fbe_transport_exchange_state(packet_p, FBE_PACKET_STATE_IN_PROGRESS);
    switch(old_state) {
        case FBE_PACKET_STATE_QUEUED:
            break;
        case FBE_PACKET_STATE_CANCELED:
            old_state = fbe_transport_exchange_state(packet_p, FBE_PACKET_STATE_CANCELED);
            break;
        default:
            fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR, "%s: Invalid packet state %X \n", __FUNCTION__, old_state);
            break;
    }

    return FBE_STATUS_OK;
}
/****************************************************************
 * end fbe_cmi_io_dequeue_check_cancel()
 ****************************************************************/

/*!**************************************************************
 * fbe_cmi_io_get_statistics()
 ****************************************************************
 * @brief
 *  This function gets IO statistics for the conduit. 
 *
 * @param get_stat - pointer to the structure.
 *
 * @return fbe_status_t
 * 
 * @author
 *  06/12/2012 - Created. Lili Chen
 *
 ****************************************************************/
fbe_status_t fbe_cmi_io_get_statistics(fbe_cmi_service_get_io_statistics_t * get_stat)
{
    fbe_cmi_io_statistics_t * io_stat;
    fbe_cmi_conduit_id_t conduit_id = get_stat->conduit_id;

    if (conduit_id < FBE_CMI_CONDUIT_ID_SEP_IO_FIRST || conduit_id > FBE_CMI_CONDUIT_ID_SEP_IO_LAST)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    io_stat = &fbe_cmi_io_statistics[conduit_id - FBE_CMI_CONDUIT_ID_SEP_IO_FIRST];
    get_stat->sent_ios = io_stat->sent_ios;
    get_stat->sent_bytes = io_stat->sent_bytes;
    get_stat->received_ios = io_stat->received_ios;
    get_stat->received_bytes = io_stat->received_bytes;

    return FBE_STATUS_OK;
}
/****************************************************************
 * end fbe_cmi_io_get_statistics()
 ****************************************************************/

/*!**************************************************************
 * fbe_cmi_io_clear_statistics()
 ****************************************************************
 * @brief
 *  This function clears IO statistics for the conduit. 
 *
 * @param conduit_id - conduit id.
 *
 * @return fbe_status_t
 * 
 * @author
 *  06/12/2012 - Created. Lili Chen
 *
 ****************************************************************/
fbe_status_t fbe_cmi_io_clear_statistics(fbe_cmi_conduit_id_t conduit_id)
{
    fbe_cmi_io_statistics_t * io_stat;

    if (conduit_id < FBE_CMI_CONDUIT_ID_SEP_IO_FIRST || conduit_id > FBE_CMI_CONDUIT_ID_SEP_IO_LAST)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    io_stat = &fbe_cmi_io_statistics[conduit_id - FBE_CMI_CONDUIT_ID_SEP_IO_FIRST];
    io_stat->sent_ios = 0;
    io_stat->sent_bytes = 0;
    io_stat->received_ios = 0;
    io_stat->received_bytes = 0;
    io_stat->sent_errors = 0;
    io_stat->received_errors = 0;

    return FBE_STATUS_OK;
}
/****************************************************************
 * end fbe_cmi_io_clear_statistics()
 ****************************************************************/

/*!**************************************************************
 * fbe_cmi_io_log_statistics()
 ****************************************************************
 * @brief
 *  This function logs IO statistics for each conduit. 
 *
 * @param is_send - whether it is send request or response.
 * @param bytes - bytes for transfer.
 * @param cpu_id - cpu id from packet.
 *
 * @return fbe_status_t
 * 
 * @author
 *  06/12/2012 - Created. Lili Chen
 *
 ****************************************************************/
fbe_status_t fbe_cmi_io_log_statistics(fbe_bool_t is_send,
                                       fbe_u64_t bytes,
                                       fbe_cpu_id_t cpu_id)
{
    fbe_cmi_io_statistics_t * io_stat;
    fbe_cmi_conduit_id_t conduit_id;

    conduit_id = fbe_cmi_service_sep_io_get_conduit(cpu_id);    
    if (conduit_id < FBE_CMI_CONDUIT_ID_SEP_IO_FIRST || conduit_id > FBE_CMI_CONDUIT_ID_SEP_IO_LAST)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_cmi_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, "%s conduit %d is_send %d bytes 0x%x\n", __FUNCTION__, 
                  conduit_id, is_send, (unsigned int)bytes);

    io_stat = &fbe_cmi_io_statistics[conduit_id - FBE_CMI_CONDUIT_ID_SEP_IO_FIRST];
    if (is_send)
    {
        fbe_atomic_increment(&io_stat->sent_ios);
        fbe_atomic_add(&io_stat->sent_bytes, bytes);
    }
    else
    {
        fbe_atomic_increment(&io_stat->received_ios);
        fbe_atomic_add(&io_stat->received_bytes, bytes);
    }

    return FBE_STATUS_OK;
}
/****************************************************************
 * end fbe_cmi_io_log_statistics()
 ****************************************************************/

/*!**************************************************************
 * fbe_cmi_io_set_data_pool()
 ****************************************************************
 * @brief
 *  This function sets up data pool information. 
 *
 * @param mem_mb - memory in MB for send/receive pools.
 * @param virtual_addr - virtual address.
 * @param pool_head - pointer to pool head.
 *
 * @return fbe_status_t
 * 
 * @author
 *  02/13/2013 - Created. Lili Chen
 *
 ****************************************************************/
fbe_status_t fbe_cmi_io_set_data_pool(fbe_u64_t mem_mb, fbe_u64_t virtual_addr, fbe_cmi_io_data_pool_t *pool_head)
{
    fbe_u32_t i;
    fbe_u64_t starting_address = virtual_addr, ending_address;
    fbe_u64_t memory_assigned = mem_mb * 1024 * 1024;
    fbe_u64_t percent_to_use = 0;

    for (i = 0; i < MAX_SEP_IO_DATA_POOLS; i++)
    {
        percent_to_use += pool_memory_percent[i];
        ending_address = virtual_addr + percent_to_use * memory_assigned / 100;
        pool_head[i].pool_start_address = starting_address;
        pool_head[i].pool_slots = (fbe_u32_t)((ending_address - starting_address) / pool_data_length[i]);
        pool_head[i].data_length = pool_data_length[i];
        starting_address += (pool_data_length[i] * pool_head[i].pool_slots);
    }

    /* For simulation */
#if defined(UMODE_ENV) || defined(SIMMODE_ENV)
    pool_head[0].pool_slots = 2;
    pool_head[1].pool_slots = 1;
#endif

    return FBE_STATUS_OK;
}
/****************************************************************
 * end fbe_cmi_io_set_data_pool()
 ****************************************************************/

/*!**************************************************************
 * fbe_cmi_io_init_data_memory()
 ****************************************************************
 * @brief
 *  This function initialize data memory from CMM. 
 *
 * @param total_mem_mb - total MB assigned from CMM.
 * @param virtual_addr - virtual address.
 *
 * @return fbe_status_t
 * 
 * @author
 *  02/13/2013 - Created. Lili Chen
 *
 ****************************************************************/
fbe_status_t fbe_cmi_io_init_data_memory(fbe_u64_t total_mem_mb, void * virtual_addr)
{
    fbe_u32_t i;
    fbe_u64_t receive_pool_address;
    fbe_u64_t mem_mb;

    if (virtual_addr == NULL)
    {
        fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR, "%s allocation from CMM failed\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Calculate the memory for send/receive data pool */
    mem_mb = total_mem_mb / 2;
    fbe_cmi_io_set_data_pool(mem_mb, (fbe_u64_t)virtual_addr, send_data_pool);
    receive_pool_address = (fbe_u64_t)virtual_addr + mem_mb * 1024 * 1024; 
    fbe_cmi_io_set_data_pool(mem_mb, receive_pool_address, receive_data_pool);

    for (i = 0; i < MAX_SEP_IO_DATA_POOLS; i++)
    {
        local_table.send_slot_address[i] = fbe_cmi_io_get_physical_address((void *)(fbe_ptrhld_t)(send_data_pool[i].pool_start_address));
        local_table.receive_slot_address[i] = fbe_cmi_io_get_physical_address((void *)(fbe_ptrhld_t)(receive_data_pool[i].pool_start_address));
        fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, "%s send pool %d phy addr %llx\n", __FUNCTION__, 
                      i, (unsigned long long)local_table.send_slot_address[i]);
        fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, "%s receive pool %d phy addr %llx\n", __FUNCTION__, 
                      i, (unsigned long long)local_table.receive_slot_address[i]);
    }

    return FBE_STATUS_OK;
}
/****************************************************************
 * end fbe_cmi_io_init_data_memory()
 ****************************************************************/

/*!**************************************************************
 * fbe_cmi_io_allocate_from_data_pool()
 ****************************************************************
 * @brief
 *  This function gets virtual address for a slot in a pool. 
 *
 * @param is_send - from send data pool or not.
 * @param pool_id - pool number.
 * @param slot - slot number.
 *
 * @return virtual_address
 * 
 * @author
 *  02/13/2013 - Created. Lili Chen
 *
 ****************************************************************/
void * fbe_cmi_io_allocate_from_data_pool(fbe_bool_t is_send, fbe_u32_t pool_id, fbe_u32_t slot)
{
    fbe_cmi_io_data_pool_t *pool;
    fbe_u64_t virtual_address;

    if (pool_id >= MAX_SEP_IO_DATA_POOLS) {
        return NULL;
    }

    pool = is_send ? (&send_data_pool[pool_id]) : (&receive_data_pool[pool_id]);

    if (slot >= pool->pool_slots) {
        return NULL;
    }

    virtual_address = pool->pool_start_address + slot * pool_data_length[pool_id];

    return (void *)(fbe_ptrhld_t)virtual_address;
}
/****************************************************************
 * end fbe_cmi_io_allocate_from_data_pool()
 ****************************************************************/

/*!**************************************************************
 * fbe_cmi_io_allocate_context_from_pool()
 ****************************************************************
 * @brief
 *  This function allocate context from context pool. 
 *
 * @param is_send - for send pools or receive pools.
 * @param pool_id - pool id.
 * @param slot    - slot id.
 *
 * @return pointer to a context
 * 
 * @author
 *  03/05/2013 - Created. Lili Chen
 *
 ****************************************************************/
fbe_cmi_io_context_t * fbe_cmi_io_allocate_context_from_pool(fbe_bool_t is_send, fbe_u32_t pool_id, fbe_u32_t slot)
{
    fbe_cmi_io_data_pool_t *pool;
    fbe_u32_t slot_offset = 0, i;

    if (pool_id >= MAX_SEP_IO_DATA_POOLS) {
        return NULL;
    }

    pool = is_send ? send_data_pool : receive_data_pool;
    slot_offset = is_send ? 0 : fbe_cmi_io_get_total_slots_for_pools(FBE_TRUE);

    if (slot >= pool[pool_id].pool_slots) {
        return NULL;
    }

    for (i = 0; i < pool_id; i++)
    {
        slot_offset += pool[i].pool_slots;
    }
    slot_offset += slot;

    return (context_pool + slot_offset);
}
/****************************************************************
 * end fbe_cmi_io_allocate_context_from_pool()
 ****************************************************************/

/*!**************************************************************
 * fbe_cmi_io_allocate_packet_from_pool()
 ****************************************************************
 * @brief
 *  This function allocate packet from packet pool. 
 *
 * @param is_send - for send pools or receive pools.
 * @param pool_id - pool id.
 * @param slot    - slot id.
 *
 * @return pointer to a packet
 * 
 * @author
 *  03/05/2013 - Created. Lili Chen
 *
 ****************************************************************/
void * fbe_cmi_io_allocate_packet_from_pool(fbe_bool_t is_send, fbe_u32_t pool_id, fbe_u32_t slot)
{
    fbe_cmi_io_data_pool_t *pool;
    fbe_u32_t slot_offset = 0, i;

    if (is_send) {
        return NULL;
    }
   
    if (pool_id >= MAX_SEP_IO_DATA_POOLS) {
        return NULL;
    }

    pool = receive_data_pool;
    if (slot >= pool[pool_id].pool_slots) {
        return NULL;
    }

    for (i = 0; i < pool_id; i++)
    {
        slot_offset += pool[i].pool_slots;
    }
    slot_offset += slot;

    return (void *)(fbe_ptrhld_t)((fbe_u64_t)packet_pool + (slot_offset * FBE_MEMORY_CHUNK_SIZE_FOR_PACKET));
}
/****************************************************************
 * end fbe_cmi_io_allocate_packet_from_pool()
 ****************************************************************/

/*!**************************************************************
 * fbe_cmi_io_get_packet_data_length()
 ****************************************************************
 * @brief
 *  This function calcutes fixed data length from a packet. 
 *
 * @param packet_p - pointer to a packet.
 * @param data_length - pointer to data length.
 *
 * @return fbe_status_t
 * 
 * @author
 *  02/13/2013 - Created. Lili Chen
 *
 ****************************************************************/
fbe_status_t fbe_cmi_io_get_packet_data_length(fbe_packet_t *packet_p, fbe_u32_t *data_length)
{
    fbe_payload_ex_t * payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_payload_block_operation_t *block_payload_p = NULL;
    fbe_payload_control_operation_t *control_payload_p = NULL;
    fbe_payload_opcode_t payload_opcode = FBE_PAYLOAD_OPCODE_INVALID;
    fbe_sg_element_t *sg_list = NULL;

    if (payload_p->current_operation != NULL)
    {
        payload_opcode = payload_p->current_operation->payload_opcode;
    }

    if (payload_opcode == FBE_PAYLOAD_OPCODE_BLOCK_OPERATION)
    {
        block_payload_p = fbe_payload_ex_get_block_operation(payload_p);
        fbe_payload_ex_get_sg_list(payload_p, &sg_list, NULL);
        if (sg_list)
        {
            *data_length = (fbe_u32_t)(block_payload_p->block_count * block_payload_p->block_size);
        }
        else
        {
            *data_length = 0;
        }
    }
    else if (payload_opcode == FBE_PAYLOAD_OPCODE_CONTROL_OPERATION)
    {
        control_payload_p = fbe_payload_ex_get_control_operation(payload_p);
        *data_length = control_payload_p->buffer_length;
    }

    return FBE_STATUS_OK;
}
/****************************************************************
 * end fbe_cmi_io_get_packet_data_length()
 ****************************************************************/

/*!**************************************************************
 * fbe_cmi_io_get_peer_slot_address()
 ****************************************************************
 * @brief
 *  This function gets peer slot address from peer table. 
 *
 * @param is_request - is it a request or a response.
 * @param pool_id - pool number.
 * @param slot - slot number.
 *
 * @return fbe_status_t
 * 
 * @author
 *  02/13/2013 - Created. Lili Chen
 *
 ****************************************************************/
fbe_u64_t 
fbe_cmi_io_get_peer_slot_address(fbe_bool_t is_request, fbe_u32_t pool_id, fbe_u32_t slot)
{
    fbe_u64_t starting_address = is_request ? (peer_table.receive_slot_address[pool_id]) : (peer_table.send_slot_address[pool_id]);

    return (starting_address + slot * pool_data_length[pool_id]);
}
/****************************************************************
 * end fbe_cmi_io_get_peer_slot_address()
 ****************************************************************/

/*!**************************************************************
 * fbe_cmi_io_get_total_slots_for_pools()
 ****************************************************************
 * @brief
 *  This function gets total slots for all send or receive pools. 
 *
 * @param is_send - for send pools or receive pools.
 *
 * @return fbe_u32_t
 * 
 * @author
 *  03/05/2013 - Created. Lili Chen
 *
 ****************************************************************/
fbe_u32_t 
fbe_cmi_io_get_total_slots_for_pools(fbe_bool_t is_send)
{
    fbe_cmi_io_data_pool_t * pool = is_send ? send_data_pool : receive_data_pool;
    fbe_u32_t total_slots = 0, i;

    for (i = 0; i < MAX_SEP_IO_DATA_POOLS; i++)
    {
        total_slots += pool[i].pool_slots;
    }

    return total_slots;
}
/****************************************************************
 * end fbe_cmi_io_get_total_slots_for_pools()
 ****************************************************************/


/*!**************************************************************
 * fbe_cmi_io_get_send_context_for_memory()
 ****************************************************************
 * @brief
 *  This function gets send context for sending memory. 
 *
 * @param
 *
 * @return context
 * 
 * @author
 *  10/22/2013 - Created. Lili Chen
 *
 ****************************************************************/
fbe_cmi_io_context_info_t * fbe_cmi_io_get_send_context_for_memory(void)
{
    fbe_cmi_io_context_t *		ctx_ptr = NULL;
    fbe_cmi_io_context_info_t * ctx_info;
    fbe_u32_t                   i, pool_id;

    /* Look for a send context in queue */
    /* For now we use the context for packets. We could allocate other context for sending memory
     * if necessary.
     */
    fbe_spinlock_lock(&send_context_lock);
    for (i = MAX_SEP_IO_DATA_POOLS; i > 0; i--) 
    {
        pool_id = i - 1;
        if (!fbe_queue_is_empty(&sep_io_send_context_head[pool_id])) {
            ctx_ptr = (fbe_cmi_io_context_t *)fbe_queue_pop(&sep_io_send_context_head[pool_id]);
            fbe_spinlock_unlock(&send_context_lock);
            ctx_ptr->context_info.context_attr = 0;
            return (&ctx_ptr->context_info);
        }
    }
    fbe_spinlock_unlock(&send_context_lock);

    /* Allocate from memory */
    fbe_cmi_trace(FBE_TRACE_LEVEL_WARNING, "%s Need to allocate from OS !\n", __FUNCTION__);
    ctx_ptr = (fbe_cmi_io_context_t *)fbe_allocate_contiguous_memory(sizeof(fbe_cmi_io_context_t));
    if (ctx_ptr == NULL) {
        fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR, "%s Unable to allocate cmi context\n", __FUNCTION__);
        return NULL;
    }

    fbe_zero_memory(ctx_ptr, sizeof(fbe_cmi_io_context_t));
    fbe_queue_element_init(&ctx_ptr->queue_element);
    fbe_spinlock_init(&ctx_ptr->context_lock);
    ctx_info = &ctx_ptr->context_info;
    ctx_info->pool = INVALID_SEP_IO_POOL_SLOT;
    ctx_info->slot = INVALID_SEP_IO_POOL_SLOT;
    ctx_info->float_data.pool = INVALID_SEP_IO_POOL_SLOT;
    ctx_info->float_data.slot = INVALID_SEP_IO_POOL_SLOT;
    ctx_info->context_attr = 0;
    ctx_info->kernel_message.pirp = EmcpalIrpAllocate(3);

    return ctx_info;
}
/****************************************************************
 * end fbe_cmi_io_get_send_context_for_memory()
 ****************************************************************/

/*!**************************************************************
 * fbe_cmi_io_return_send_context_for_memory()
 ****************************************************************
 * @brief
 *  This function returns send context after sending memory. 
 *
 * @param
 *   ctx_info  - Pointer to send memory structure.
 *
 * @return fbe_status_t
 * 
 * @author
 *  10/22/2013 - Created. Lili Chen
 *
 ****************************************************************/
fbe_status_t fbe_cmi_io_return_send_context_for_memory(fbe_cmi_io_context_info_t * ctx_info)
{
    fbe_cmi_io_context_t * ctx_ptr = NULL;

    if (ctx_info == NULL)
    {
        fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR, "%s context lost.\n", __FUNCTION__);
        return FBE_STATUS_OK;
    }

    ctx_ptr = fbe_cmi_io_context_info_to_context(ctx_info);
    fbe_spinlock_lock(&ctx_ptr->context_lock);
    if (!(ctx_info->context_attr & FBE_CMI_IO_CONTEXT_ATTR_MEMORY_SENT)) {
        ctx_info->context_attr |= FBE_CMI_IO_CONTEXT_ATTR_MEMORY_SENT;
        fbe_spinlock_unlock(&ctx_ptr->context_lock);
        return FBE_STATUS_OK;
    }

    ctx_info->context_attr &= ~FBE_CMI_IO_CONTEXT_ATTR_MEMORY_SENT;
    fbe_spinlock_unlock(&ctx_ptr->context_lock);

    fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, "%s: returning slot %d_%d \n", __FUNCTION__, ctx_info->pool, ctx_info->slot);

    if ((ctx_info->pool == INVALID_SEP_IO_POOL_SLOT) && (ctx_info->slot == INVALID_SEP_IO_POOL_SLOT))
    {
        EmcpalIrpFree(ctx_ptr->context_info.kernel_message.pirp);
        fbe_spinlock_destroy(&ctx_ptr->context_lock);
        fbe_release_contiguous_memory((void *)ctx_ptr);
    }
    else
    {
        ctx_info->float_data.msg_type = FBE_CMI_IO_MESSAGE_TYPE_PACKET_REQUEST;
        ctx_info->packet = NULL;
        fbe_cmi_io_enqueue_context_info_to_q(ctx_info, &sep_io_send_context_head[ctx_info->pool], &send_context_lock);
    }

    return FBE_STATUS_OK;
}
/****************************************************************
 * end fbe_cmi_io_return_send_context_for_memory()
 ****************************************************************/

/*!**************************************************************
 * fbe_cmi_send_memory_to_other_sp()
 ****************************************************************
 * @brief
 *  This function is the main function in FBE CMI to send 
 *  memory to peer SP. 
 *
 * @param
 *   send_memory_p  - Pointer to send memory structure.
 *
 * @return fbe_status_t
 * 
 * @author
 *  10/22/2013 - Created. Lili Chen
 *
 ****************************************************************/
fbe_status_t fbe_cmi_send_memory_to_other_sp(fbe_cmi_memory_t * send_memory_p)
{
    fbe_status_t					 status  = FBE_STATUS_GENERIC_FAILURE;
    fbe_cmi_io_context_info_t *      ctx_info;
    fbe_cmi_io_send_memory_t *       memory_info;

    fbe_cmi_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, "%s src 0x%llx dest 0x%llx len 0x%x\n", __FUNCTION__, 
                  send_memory_p->source_addr, send_memory_p->dest_addr, send_memory_p->data_length);

    if (send_memory_p->message_length > MAX_SEP_IO_MEM_MSG_LEN)
    {
        fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR, "%s message length %d is too large.\n", __FUNCTION__, send_memory_p->message_length);
        return status;
    }

    ctx_info = fbe_cmi_io_get_send_context_for_memory();
    if (ctx_info == NULL)
    {
        fbe_cmi_trace(FBE_TRACE_LEVEL_DEBUG_LOW, "%s No context\n", __FUNCTION__);
        return FBE_STATUS_BUSY;
    }
    fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, "%s: getting slot %d_%d \n", __FUNCTION__, ctx_info->pool, ctx_info->slot);

    /* Fill in float data here. */
    ctx_info->float_data.msg_type = FBE_CMI_IO_MESSAGE_TYPE_SEND_MEMORY;
    ctx_info->float_data.fixed_data_length = send_memory_p->data_length;
    ctx_info->packet = send_memory_p->message; /* cache message address here */
    memory_info = &ctx_info->float_data.msg.memory_info;
    memory_info->client_id = send_memory_p->client_id;
    memory_info->context = send_memory_p->context;
    memory_info->message_length = send_memory_p->message_length;
    fbe_copy_memory(memory_info->message, send_memory_p->message, send_memory_p->message_length);

    status = fbe_cmi_memory_start_send(ctx_info, send_memory_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, "%s: failed status 0x%x \n", __FUNCTION__, status);
    }

    return status;
}
/****************************************************************
 * end fbe_cmi_send_memory_to_other_sp()
 ****************************************************************/

/*!**************************************************************
 * fbe_cmi_service_sep_io_send_memory_callback()
 ****************************************************************
 * @brief
 *  This is the callback function for sending memory over SEP IO client 
 *  when a FBE CMI event is posted. 
 *
 * @param
 *   event - FBE CMI event.
 *   cmi_message - Pointer to the message.
 *   cmi_message_length - message length.
 *   context - context.
 *
 * @return fbe_status_t
 * 
 * @author
 *  10/22/2013 - Created. Lili Chen
 *
 ****************************************************************/
fbe_status_t 
fbe_cmi_service_sep_io_send_memory_callback(fbe_cmi_event_t event_id, 
                                            fbe_u32_t cmi_message_length, 
                                            fbe_cmi_message_t cmi_message, 
                                            fbe_cmi_event_callback_context_t context)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_cmi_io_context_info_t * ctx_info = (fbe_cmi_io_context_info_t *)context;
    fbe_cmi_io_float_data_t * float_data = (fbe_cmi_io_float_data_t *)cmi_message;
    fbe_cmi_io_send_memory_t * memory_info = &float_data->msg.memory_info;

    /* Return context to send queue. */
    switch (event_id) {
        case FBE_CMI_EVENT_MESSAGE_TRANSMITTED:
        case FBE_CMI_EVENT_FATAL_ERROR:
        case FBE_CMI_EVENT_PEER_NOT_PRESENT:
        case FBE_CMI_EVENT_PEER_BUSY:
            if (client_list[memory_info->client_id].client_callback) {
                status = client_list[memory_info->client_id].client_callback(event_id, memory_info->message_length, ctx_info->packet, memory_info->context); 
                fbe_cmi_io_return_send_context_for_memory(ctx_info);
            }
            break;
    case FBE_CMI_EVENT_MESSAGE_RECEIVED:
            if (client_list[memory_info->client_id].client_callback) {
                status = client_list[memory_info->client_id].client_callback(event_id, memory_info->message_length, memory_info->message, context);
            }
            break;
    case FBE_CMI_EVENT_SP_CONTACT_LOST:
            if (client_list[memory_info->client_id].client_callback) {
                status = client_list[memory_info->client_id].client_callback(event_id, cmi_message_length, cmi_message, context);
            }
            break;
        default:
            fbe_cmi_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, "%s Illegal CMI opcode: 0x%X\n", __FUNCTION__, event_id);
            break;
    }
    
    return status;
}
/****************************************************************
 * end fbe_cmi_service_sep_io_send_memory_callback()
 ****************************************************************/
