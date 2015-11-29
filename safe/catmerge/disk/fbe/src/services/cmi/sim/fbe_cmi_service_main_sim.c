/***************************************************************************
 * Copyright (C) EMC Corporation 2001-2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *  fbe_cmi_service_main_sim.c
 ***************************************************************************
 *
 *  Description
 *      This file contains the simulation implementation for the FBE CMI service code
 *      
 *    
 ***************************************************************************/

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "fbe/fbe_types.h"
#include "fbe/fbe_winddk.h"
#include "fbe_base_service.h"
#include "fbe/fbe_cmi_interface.h"
#include "fbe_cmi_private.h"
#include "fbe/fbe_terminator_api.h"
#include "fbe_cmi_sim_client_server.h"
#include <winsock2.h>

/* Local members */
static	fbe_terminator_api_get_sp_id_function_t terminator_get_sp_id_func = NULL;
static 	fbe_cmi_sp_id_t							cmi_sim_sp_id = FBE_CMI_SP_ID_INVALID;

/* Forward declerations */
static fbe_status_t fbe_cmi_sim_get_my_sp_id(fbe_cmi_sp_id_t *cmi_sp_id);

/*********************************************************************************************************/
fbe_status_t fbe_cmi_init_connections(void)
{
    fbe_cmi_conduit_id_t	conduit = FBE_CMI_CONDUIT_ID_INVALID;
    fbe_u32_t				result = 0;
    fbe_status_t			status  = FBE_STATUS_GENERIC_FAILURE;
    fbe_package_id_t		package_id = FBE_PACKAGE_ID_INVALID;

    /* update cmi ports if Terminator reset them */
    fbe_cmi_sim_update_cmi_port();

    /*we need our sp ID to make sure we initialize the right TCP sockets*/
    status = fbe_cmi_init_sp_id();
    if (status != FBE_STATUS_OK) {
        return status;
    }

     /*init winsock*/
    result = EmcutilNetworkWorldInitializeMaybe();
    if (result != 0) {
        fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR, "WSAStartup failed: %d\n", result);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*we need to open only the conduits that this package will use*/
    status = fbe_get_package_id(&package_id);
    if (status != FBE_STATUS_OK) {
        fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR,"%s Failed to get package ID\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*we start with the first consuit after FBE_CMI_CONDUIT_ID_INVALID*/
    for (conduit ++; conduit < FBE_CMI_CONDUIT_ID_LAST; conduit ++) {
        if (fbe_cmi_need_to_open_conduit(conduit, package_id)) {
            fbe_cmi_sim_init_spinlock(conduit);
			fbe_cmi_sim_init_conduit_server(conduit, cmi_sim_sp_id);
            verify_peer_connection_is_up(cmi_sim_sp_id, conduit);
           
        }
    }

    return FBE_STATUS_OK;

}

fbe_status_t fbe_cmi_send_mailbomb_to_other_sp(fbe_cmi_conduit_id_t conduit_id)
{
    // not implemented
    return FBE_STATUS_OK;
}

fbe_status_t fbe_cmi_send_message_to_other_sp(fbe_cmi_client_id_t client_id,
                                              fbe_cmi_conduit_id_t	conduit,
                                              fbe_u32_t message_length,
                                              fbe_cmi_message_t message,
                                              fbe_cmi_event_callback_context_t context)
{
    fbe_u32_t						total_message_lenth = sizeof(fbe_cmi_sim_tcp_message_t) + message_length;
    fbe_cmi_sim_tcp_message_t      *tcp_message_p = NULL;
    fbe_status_t					status = FBE_STATUS_GENERIC_FAILURE;

    /*any chance the other SP is not even there?*/
    if (!fbe_cmi_is_peer_alive()){
        return FBE_STATUS_NO_DEVICE;
    }

    /* Must allocate messages since we may use it later. */
    tcp_message_p = (fbe_cmi_sim_tcp_message_t *)malloc(sizeof(*tcp_message_p));
    if (tcp_message_p == NULL) {
         fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR, "%s, failed to allocate message via client\n", __FUNCTION__);
         return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Populate the message */
    fbe_zero_memory(tcp_message_p, sizeof(*tcp_message_p));
    tcp_message_p->total_message_lenth = total_message_lenth;
    tcp_message_p->user_message = message;
    tcp_message_p->conduit = conduit;
    tcp_message_p->client_id = client_id;
    tcp_message_p->context = context;

    status = fbe_cmi_sim_client_send_message (tcp_message_p);
    if (status == FBE_STATUS_GENERIC_FAILURE) {
        fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR, "%s, failed to send message via client\n", __FUNCTION__);
    }
    
    /* If the transmission failed return the status now.*/
    if (status != FBE_STATUS_OK) {
        free(tcp_message_p);
    }

    /* Return the status determined. */
    return status;

}

static fbe_status_t fbe_cmi_sim_get_my_sp_id(fbe_cmi_sp_id_t *cmi_sp_id)
{
    terminator_sp_id_t	terminator_sp_id = TERMINATOR_SP_A;
    fbe_status_t		status = FBE_STATUS_GENERIC_FAILURE;

    if (terminator_get_sp_id_func == NULL) {
        fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR, "%s, user did not set terminator entry to get sp id\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = terminator_get_sp_id_func(&terminator_sp_id);
    if (status != FBE_STATUS_OK) {
        return status;
    }

    *cmi_sp_id = ((terminator_sp_id == TERMINATOR_SP_A) ? FBE_CMI_SP_ID_A : FBE_CMI_SP_ID_B);

    return FBE_STATUS_OK;
}

fbe_status_t fbe_cmi_close_connections(void)
{
    fbe_cmi_conduit_id_t	conduit = FBE_CMI_CONDUIT_ID_INVALID;
    fbe_status_t			status  = FBE_STATUS_GENERIC_FAILURE;
    fbe_package_id_t		package_id = FBE_PACKAGE_ID_INVALID;

    /*we need to close only the conduits that this package will use*/
    status = fbe_get_package_id(&package_id);
    if (status != FBE_STATUS_OK) {
        fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR,"%s Failed to get package ID\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    for (conduit ++; conduit < FBE_CMI_CONDUIT_ID_LAST; conduit ++) {
        if (fbe_cmi_need_to_open_conduit(conduit, package_id)) {
            fbe_cmi_sim_destroy_conduit_server(conduit);
            fbe_cmi_sim_destroy_conduit_client(conduit);
        }
    }

    return FBE_STATUS_OK;
}

fbe_status_t fbe_cmi_sim_set_get_sp_id_func(fbe_terminator_api_get_sp_id_function_t function)
{
    terminator_get_sp_id_func = function;
    return FBE_STATUS_OK;
}


fbe_status_t fbe_cmi_get_sp_id(fbe_cmi_sp_id_t *cmi_sp_id)
{
    if (cmi_sp_id != NULL) {
        *cmi_sp_id = cmi_sim_sp_id;
        return FBE_STATUS_OK;
    }else{
        return FBE_STATUS_GENERIC_FAILURE;
    }
}

fbe_cmi_conduit_id_t fbe_cmi_service_sep_io_get_conduit(fbe_cpu_id_t cpu_id)
{
    return (FBE_CMI_CONDUIT_ID_SEP_IO_CORE0 + (cpu_id % FBE_CMI_IO_MAX_CONDUITS));
}

#define MAX_SEP_IO_SLOTS 3 /* For simulation we use 3 conduits only */
fbe_bool_t fbe_cmi_service_need_to_open_sep_io_conduit(fbe_cmi_conduit_id_t conduit_id)
{
    if ((conduit_id >= (FBE_CMI_CONDUIT_ID_SEP_IO_FIRST + MAX_SEP_IO_SLOTS)) &&
        (conduit_id <= FBE_CMI_CONDUIT_ID_SEP_IO_LAST))
    {
        return FBE_FALSE;
    }

    return FBE_TRUE;
}

/*!**************************************************************
 * fbe_cmi_io_get_sgl_for_transfer()
 ****************************************************************
 * @brief
 *  This function gets the source fixed data sg list and length 
 *  to fill into cmid ioctl structure. 
 *
 * @param
 *   float_data  - pointer to packet information.
 *   packet_p  - pointer to packet.
 *   sg_list - source sg list.
 *   dest_addr - pointer to destination address.
 *   fixed_data_length  - fixed data length.
 *
 * @return fbe_status_t
 * 
 * @author
 *  04/22/2012 - Created. Lili Chen
 *
 ****************************************************************/
fbe_status_t fbe_cmi_io_get_sgl_for_transfer(fbe_cmi_io_context_info_t * ctx_info, 
                                             fbe_packet_t * packet_p, 
                                             fbe_sg_element_t **sg_list, 
                                             fbe_u64_t *dest_addr, 
                                             fbe_u32_t * fixed_data_length)
{
    fbe_payload_ex_t *              payload_p = NULL;
    fbe_payload_block_operation_t * block_operation_p = NULL;
    fbe_payload_block_operation_opcode_t    opcode;
    fbe_cmi_io_float_data_t * float_data = &ctx_info->float_data;

    payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_get_present_block_operation(payload_p);
    fbe_payload_block_get_opcode(block_operation_p, &opcode);

    *sg_list = NULL;
    *fixed_data_length = 0;

    if ((float_data->msg_type == FBE_CMI_IO_MESSAGE_TYPE_PACKET_REQUEST) &&
        (fbe_payload_block_operation_opcode_is_media_modify(opcode)) &&
        (fbe_payload_block_operation_opcode_requires_sg(opcode)))
    {
        fbe_cmi_io_write_request_preprocess(ctx_info, packet_p, sg_list);
        *fixed_data_length = float_data->fixed_data_length;
        *dest_addr = fbe_cmi_io_get_peer_slot_address(FBE_TRUE, ctx_info->pool, ctx_info->slot);
    }
    else if ((float_data->msg_type == FBE_CMI_IO_MESSAGE_TYPE_PACKET_RESPONSE) &&
        (fbe_payload_block_operation_opcode_is_media_read(opcode)) &&
        (fbe_payload_block_operation_opcode_requires_sg(opcode)))
    {
        fbe_payload_ex_get_sg_list(payload_p, sg_list, NULL);
        *fixed_data_length = float_data->fixed_data_length;
        //*dest_addr = fbe_cmi_io_get_peer_slot_address(FBE_FALSE, ctx_info->pool, ctx_info->slot);
        *dest_addr = ctx_info->kernel_message.fixed_data_blob.blob_sgl[0].PhysAddress;
    }
    else if ((float_data->msg_type == FBE_CMI_IO_MESSAGE_TYPE_PACKET_REQUEST) &&
        (fbe_payload_block_operation_opcode_is_media_read(opcode)) &&
        (fbe_payload_block_operation_opcode_requires_sg(opcode)))
    {
        fbe_u32_t sgl_count = 0;
        fbe_bool_t read_copy_needed = FBE_FALSE;

        fbe_cmi_io_read_request_preprocess(ctx_info, packet_p, 2, &sgl_count, &read_copy_needed);
        if (read_copy_needed)
        {
            ctx_info->context_attr |= FBE_CMI_IO_CONTEXT_ATTR_READ_COPY;
        }
        *sg_list = ctx_info->sg_element;
        *fixed_data_length = sgl_count * sizeof(CMI_PHYSICAL_SG_ELEMENT);
        *dest_addr = fbe_cmi_io_get_peer_slot_address(FBE_TRUE, ctx_info->pool, ctx_info->slot);
    }

    return FBE_STATUS_OK;
}
/****************************************************************
 * end fbe_cmi_io_get_sgl_for_transfer()
 ****************************************************************/

fbe_status_t fbe_cmi_io_start_send(fbe_cmi_io_context_info_t * ctx_info,
                                   fbe_packet_t * packet_p)
{
    fbe_cmi_sim_tcp_message_t     *tcp_message_p = NULL;
    fbe_status_t                  status = FBE_STATUS_GENERIC_FAILURE;
    fbe_sg_element_t * sg_list;
    fbe_u32_t fixed_data_length = 0;
    fbe_cmi_io_float_data_t *       float_data = &ctx_info->float_data;
    fbe_u64_t                       dest_addr;

    /* Must allocate messages since we may use it later. */
    tcp_message_p = (fbe_cmi_sim_tcp_message_t *)malloc(sizeof(*tcp_message_p));
    if (tcp_message_p == NULL) {
         fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR, "%s, failed to allocate message via client\n", __FUNCTION__);
         return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }
    /* Populate the message */
    fbe_zero_memory(tcp_message_p, sizeof(*tcp_message_p));
    tcp_message_p->total_message_lenth = sizeof(fbe_cmi_sim_tcp_message_t) + sizeof(fbe_cmi_io_float_data_t);
    tcp_message_p->user_message = &ctx_info->float_data;
    tcp_message_p->conduit = fbe_cmi_service_sep_io_get_conduit((ctx_info->float_data.pool) * 2 + ctx_info->float_data.slot);
    tcp_message_p->client_id = FBE_CMI_CLIENT_ID_SEP_IO;
    tcp_message_p->context = ctx_info;
    tcp_message_p->sg_list = NULL;
    tcp_message_p->control_buffer = NULL;

    if (float_data->payload_opcode == FBE_PAYLOAD_OPCODE_BLOCK_OPERATION)
    {
        /* Serialize fixed data to transfer */
        fbe_cmi_io_get_sgl_for_transfer(ctx_info, packet_p, &sg_list, &dest_addr, &fixed_data_length);
        if (sg_list != NULL && fixed_data_length != 0)
        {
            tcp_message_p->total_message_lenth += fixed_data_length;
            tcp_message_p->msg_type = FBE_CMI_SIM_TCP_MESSAGE_TYPE_WITH_FIXED_DATA;
            tcp_message_p->fixed_data_length = fixed_data_length;
            tcp_message_p->sg_list = sg_list;
            tcp_message_p->dest_addr = (void *)(fbe_ptrhld_t)dest_addr;
        }
    }
    else if ((float_data->payload_opcode == FBE_PAYLOAD_OPCODE_CONTROL_OPERATION) && 
		(float_data->fixed_data_length != 0) &&
		(float_data->msg_type == FBE_CMI_IO_MESSAGE_TYPE_PACKET_REQUEST))
    {
        fbe_payload_ex_t * payload_p = fbe_transport_get_payload_ex(packet_p);
        fbe_payload_control_operation_t *control_payload_p = fbe_payload_ex_get_control_operation(payload_p);

        fixed_data_length = float_data->fixed_data_length;
        tcp_message_p->total_message_lenth += fixed_data_length;
        tcp_message_p->msg_type = FBE_CMI_SIM_TCP_MESSAGE_TYPE_WITH_FIXED_DATA;
        tcp_message_p->fixed_data_length = fixed_data_length;
        tcp_message_p->control_buffer = control_payload_p->buffer;
        tcp_message_p->dest_addr = (void *)(fbe_ptrhld_t)fbe_cmi_io_get_peer_slot_address(FBE_TRUE, ctx_info->pool, ctx_info->slot);
    }

    fbe_cmi_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, "%s slot %d_%d packet %p\n", __FUNCTION__, ctx_info->pool, ctx_info->slot, packet_p);
    fbe_cmi_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, "dest %p total 0x%x fixed 0x%x conduit %d\n", tcp_message_p->dest_addr, tcp_message_p->total_message_lenth,
		float_data->fixed_data_length, tcp_message_p->conduit);

    status = fbe_cmi_sim_client_send_message (tcp_message_p);
    if (status == FBE_STATUS_GENERIC_FAILURE) {
        fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR, "%s, failed to send message via client\n", __FUNCTION__);
    }
    fbe_cmi_io_return_held_context(ctx_info);

    /* If the transmission failed return the status now.*/
    if (status != FBE_STATUS_OK) {
        free(tcp_message_p);
    }

    /* Return the status determined. */
    return status;
}

fbe_u64_t fbe_cmi_io_get_physical_address(void *virt_address)
{
    return ((fbe_u64_t)virt_address);
}

/*!**************************************************************
 * fbe_cmi_memory_start_send()
 ****************************************************************
 * @brief
 *  This function is used to start sending memory over FBE CMI 
 *  in simulation. 
 *
 * @param
 *   ctx_info - context info.
 *
 * @return fbe_status_t
 * 
 * @author
 *  10/18/2013 - Created. Lili Chen
 *
 ****************************************************************/
fbe_status_t fbe_cmi_memory_start_send(fbe_cmi_io_context_info_t * ctx_info, fbe_cmi_memory_t * send_memory_p)
{
    fbe_cmi_sim_tcp_message_t     *tcp_message_p = NULL;
    fbe_status_t                  status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t fixed_data_length = 0;
    fbe_cmi_io_float_data_t *       float_data = &ctx_info->float_data;

    /* Must allocate messages since we may use it later. */
    tcp_message_p = (fbe_cmi_sim_tcp_message_t *)malloc(sizeof(*tcp_message_p));
    if (tcp_message_p == NULL) {
         fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR, "%s, failed to allocate message via client\n", __FUNCTION__);
         return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }
    /* Populate the message */
    fbe_zero_memory(tcp_message_p, sizeof(*tcp_message_p));
    tcp_message_p->total_message_lenth = sizeof(fbe_cmi_sim_tcp_message_t) + sizeof(fbe_cmi_io_float_data_t);
    tcp_message_p->user_message = &ctx_info->float_data;
    tcp_message_p->conduit = FBE_CMI_CONDUIT_ID_SEP_IO_CORE0;
    tcp_message_p->client_id = FBE_CMI_CLIENT_ID_SEP_IO;
    tcp_message_p->context = ctx_info;
    tcp_message_p->sg_list = NULL;
    tcp_message_p->control_buffer = NULL;

    fixed_data_length = send_memory_p->data_length;
    tcp_message_p->total_message_lenth += send_memory_p->data_length;
    tcp_message_p->msg_type = FBE_CMI_SIM_TCP_MESSAGE_TYPE_WITH_FIXED_DATA;
    tcp_message_p->fixed_data_length = send_memory_p->data_length;
    tcp_message_p->control_buffer = (void *)(fbe_ptrhld_t)send_memory_p->source_addr;
    tcp_message_p->dest_addr = (void *)(fbe_ptrhld_t)send_memory_p->dest_addr;

    fbe_cmi_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, "%s slot %d_%d \n", __FUNCTION__, ctx_info->pool, ctx_info->slot);
    fbe_cmi_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, "dest %p total 0x%x fixed 0x%x conduit %d\n", tcp_message_p->dest_addr, tcp_message_p->total_message_lenth,
		float_data->fixed_data_length, tcp_message_p->conduit);

    status = fbe_cmi_sim_client_send_message (tcp_message_p);
    if (status == FBE_STATUS_GENERIC_FAILURE) {
        fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR, "%s, failed to send message via client\n", __FUNCTION__);
    }
    fbe_cmi_io_return_send_context_for_memory(ctx_info);

    /* If the transmission failed return the status now.*/
    if (status != FBE_STATUS_OK) {
        free(tcp_message_p);
    }

    /* Return the status determined. */
    return status;
}

fbe_status_t fbe_cmi_init_sp_id(void)
{
    fbe_status_t status  = FBE_STATUS_GENERIC_FAILURE;

    status = fbe_cmi_sim_get_my_sp_id(&cmi_sim_sp_id);

    return status;
}

/****************************************************************
 * end fbe_cmi_memory_start_send()
 ****************************************************************/
