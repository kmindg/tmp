/***************************************************************************
 * Copyright (C) EMC Corporation 2012
 * All rights reserved. Licensed material -- property of EMC
 * Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_cmi_service_kernel_io.c
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
#ifndef ALAMOSA_WINDOWS_ENV
#include "safe_fix_null.h"
#endif /* ALAMOSA_WINDOWS_ENV - STDPORT */

extern fbe_cmi_driver_t                        fbe_cmi_driver_info;
extern SPID                                    fbe_cmi_kernel_other_sp_id;


/*!**************************************************************
 * fbe_cmi_service_sep_io_get_conduit()
 ****************************************************************
 * @brief
 *  This function gets fbe conduit, one for each core. 
 *
 * @param cpu_id
 *
 * @return fbe_status_t
 * 
 * @author
 *  05/15/2012 - Created. Lili Chen
 *
 ****************************************************************/
fbe_cmi_conduit_id_t fbe_cmi_service_sep_io_get_conduit(fbe_cpu_id_t cpu_id)
{
    return (FBE_CMI_CONDUIT_ID_SEP_IO_CORE0 + (cpu_id % FBE_CMI_IO_MAX_CONDUITS));
}
/****************************************************************
 * end fbe_cmi_service_sep_io_get_conduit()
 ****************************************************************/

/*!**************************************************************
 * fbe_cmi_service_need_to_open_sep_io_conduit()
 ****************************************************************
 * @brief
 *  This function checks whether we need to open sep io conduit. 
 *  We only need to open as many conduits as cores.
 *
 * @param conduit_id - conduit ID
 *
 * @return fbe_status_t
 * 
 * @author
 *  05/15/2012 - Created. Lili Chen
 *
 ****************************************************************/
fbe_bool_t fbe_cmi_service_need_to_open_sep_io_conduit(fbe_cmi_conduit_id_t conduit_id)
{
    fbe_cpu_id_t cpu_count = fbe_get_cpu_count();

    if ((conduit_id >= (FBE_CMI_CONDUIT_ID_SEP_IO_FIRST + cpu_count)) &&
        (conduit_id <= FBE_CMI_CONDUIT_ID_SEP_IO_LAST))
    {
        return FBE_FALSE;
    }

    return FBE_TRUE;
}
/****************************************************************
 * end fbe_cmi_service_need_to_open_sep_io_conduit()
 ****************************************************************/

/*!**************************************************************
 * fbe_cmi_io_fill_fixed_data_write_request()
 ****************************************************************
 * @brief
 *  This function fills fixed data for write request. 
 *
 * @param
 *   ctx_info - pointer to context.
 *   packet_p - pointer to the packet.
 *
 * @return fbe_status_t
 * 
 * @author
 *  06/11/2013 - Created. Lili Chen
 *
 ****************************************************************/
static fbe_status_t 
fbe_cmi_io_fill_fixed_data_write_request(fbe_cmi_io_context_info_t * ctx_info,
                                         fbe_packet_t * packet_p)
{
    fbe_cmi_io_float_data_t * float_data = &ctx_info->float_data;
    fbe_cmi_io_message_info_t * fbe_cmi_kernel_message = &ctx_info->kernel_message;
    fbe_sg_element_t *sg_list = NULL;

    fbe_cmi_io_write_request_preprocess(ctx_info, packet_p, &sg_list);
    if (sg_list != NULL)
    {
        fbe_u32_t sgl_count = 0;
        fbe_u32_t data_length = 0;
        fbe_sg_element_t * sg_ptr = sg_list;
        fbe_u32_t          bytes_to_copy;
        fbe_u32_t          bytes_remaining = float_data->fixed_data_length;

        while ((bytes_remaining != 0) && (sg_ptr->count != 0) &&
               (sgl_count < (MAX_SEP_IO_SG_COUNT - 1)))
        {
            bytes_to_copy = FBE_MIN(bytes_remaining, sg_ptr->count);
            fbe_cmi_kernel_message->fixed_data_sgl[sgl_count].PhysAddress = fbe_get_contigmem_physical_address(sg_ptr->address);
            fbe_cmi_kernel_message->fixed_data_sgl[sgl_count].length = bytes_to_copy;
            bytes_remaining -= bytes_to_copy;
            data_length += bytes_to_copy;
            sgl_count++;
            sg_ptr++;
        }
        if (data_length != float_data->fixed_data_length) {
            fbe_cmi_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, "%s data len 0x%x not as expected 0x%x sgl %d\n", __FUNCTION__,
                          data_length, float_data->fixed_data_length, sgl_count);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        fbe_cmi_kernel_message->fixed_data_sgl[sgl_count].PhysAddress = NULL;
        fbe_cmi_kernel_message->fixed_data_sgl[sgl_count].length = 0;
    }
    else
    {
        fbe_cmi_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, "%s: sg list is NULL\n", __FUNCTION__);
    }
    fbe_cmi_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, "%s slot %d_%d len 0x%x packet %p\n", __FUNCTION__, 
                  ctx_info->pool, ctx_info->slot, float_data->fixed_data_length, float_data->original_packet);

    fbe_cmi_kernel_message->fixed_data_blob.blob_sgl[0].PhysAddress = fbe_cmi_io_get_peer_slot_address(FBE_TRUE, float_data->pool, float_data->slot);
    fbe_cmi_kernel_message->fixed_data_blob.blob_sgl[0].length = float_data->fixed_data_length;
    fbe_cmi_kernel_message->fixed_data_blob.blob_sgl[1].PhysAddress = NULL;
    fbe_cmi_kernel_message->fixed_data_blob.blob_sgl[1].length = 0;

    /* set the fixed data sgl to point to the pre-allocated memory in our data structure*/
    fbe_cmi_kernel_message->cmid_ioctl.fixed_data_sgl = fbe_cmi_kernel_message->fixed_data_sgl;
    fbe_cmi_kernel_message->cmid_ioctl.fixedDataDescriptionBlob = (PVOID)&fbe_cmi_kernel_message->fixed_data_blob;
    fbe_cmi_kernel_message->cmid_ioctl.fixedDataDescriptionBlobLength = 2*sizeof(CMI_PHYSICAL_SG_ELEMENT);

    return FBE_STATUS_OK;
}
/****************************************************************
 * end fbe_cmi_io_fill_fixed_data_write_request()
 ****************************************************************/

/*!**************************************************************
 * fbe_cmi_io_fill_fixed_data_read_request()
 ****************************************************************
 * @brief
 *  This function fills fixed data for read request. 
 *
 * @param
 *   ctx_info - pointer to context.
 *   packet_p - pointer to the packet.
 *
 * @return fbe_status_t
 * 
 * @author
 *  06/11/2013 - Created. Lili Chen
 *
 ****************************************************************/
static fbe_status_t 
fbe_cmi_io_fill_fixed_data_read_request(fbe_cmi_io_context_info_t * ctx_info,
                                        fbe_packet_t * packet_p)
{
    fbe_cmi_io_float_data_t * float_data = &ctx_info->float_data;
    fbe_cmi_io_message_info_t * fbe_cmi_kernel_message = &ctx_info->kernel_message;
    fbe_u32_t sgl_count = 0;
    fbe_bool_t read_copy_needed = FBE_FALSE;
    CMI_PHYSICAL_SG_ELEMENT * sg_ptr = (CMI_PHYSICAL_SG_ELEMENT *)(ctx_info->data);;

    fbe_cmi_io_read_request_preprocess(ctx_info, packet_p, MAX_SEP_IO_SG_COUNT, &sgl_count, &read_copy_needed);
    if (read_copy_needed)
    {
        ctx_info->context_attr |= FBE_CMI_IO_CONTEXT_ATTR_READ_COPY;
    }
    fbe_cmi_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, "%s slot %d_%d len 0x%x\n", __FUNCTION__, 
                  ctx_info->pool, ctx_info->slot, float_data->fixed_data_length);
    fbe_cmi_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, "packet %p sgl %d first len 0x%x addr 0x%llx\n", 
                  float_data->original_packet, sgl_count, sg_ptr[0].length, sg_ptr[0].PhysAddress);

    fbe_cmi_kernel_message->fixed_data_sgl[0].PhysAddress = fbe_get_contigmem_physical_address(ctx_info->data);
    fbe_cmi_kernel_message->fixed_data_sgl[0].length = sgl_count * sizeof(CMI_PHYSICAL_SG_ELEMENT);
    fbe_cmi_kernel_message->fixed_data_sgl[1].PhysAddress = NULL;
    fbe_cmi_kernel_message->fixed_data_sgl[1].length = 0;

    fbe_cmi_kernel_message->fixed_data_blob.blob_sgl[0].PhysAddress = fbe_cmi_io_get_peer_slot_address(FBE_TRUE, float_data->pool, float_data->slot);
    fbe_cmi_kernel_message->fixed_data_blob.blob_sgl[0].length = sgl_count * sizeof(CMI_PHYSICAL_SG_ELEMENT);
    fbe_cmi_kernel_message->fixed_data_blob.blob_sgl[1].PhysAddress = NULL;
    fbe_cmi_kernel_message->fixed_data_blob.blob_sgl[1].length = 0;

    /* set the fixed data sgl to point to the pre-allocated memory in our data structure*/
    fbe_cmi_kernel_message->cmid_ioctl.fixed_data_sgl = fbe_cmi_kernel_message->fixed_data_sgl;
    fbe_cmi_kernel_message->cmid_ioctl.fixedDataDescriptionBlob = (PVOID)&fbe_cmi_kernel_message->fixed_data_blob;
    fbe_cmi_kernel_message->cmid_ioctl.fixedDataDescriptionBlobLength = 2*sizeof(CMI_PHYSICAL_SG_ELEMENT);

    return FBE_STATUS_OK;
}
/****************************************************************
 * end fbe_cmi_io_fill_fixed_data_read_request()
 ****************************************************************/

/*!**************************************************************
 * fbe_cmi_io_fill_fixed_data_read_response()
 ****************************************************************
 * @brief
 *  This function fills fixed data for read response. 
 *
 * @param
 *   ctx_info - pointer to context.
 *   packet_p - pointer to the packet.
 *
 * @return fbe_status_t
 * 
 * @author
 *  06/11/2013 - Created. Lili Chen
 *
 ****************************************************************/
static fbe_status_t 
fbe_cmi_io_fill_fixed_data_read_response(fbe_cmi_io_context_info_t * ctx_info,
                                         fbe_packet_t * packet_p)
{
    fbe_payload_ex_t *              payload_p = NULL;
    fbe_cmi_io_float_data_t * float_data = &ctx_info->float_data;
    fbe_cmi_io_message_info_t * fbe_cmi_kernel_message = &ctx_info->kernel_message;
    fbe_sg_element_t *sg_list = NULL;
    fbe_u32_t sgl_count = 0;
    CMI_PHYSICAL_SG_ELEMENT * sg_ptr = fbe_cmi_kernel_message->fixed_data_blob.blob_sgl;
    fbe_u32_t data_length = 0;

    payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_payload_ex_get_sg_list(payload_p, &sg_list, NULL);

    if (sg_list->address != ctx_info->data)
    {
        fbe_cmi_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, "%s: sg list %p address not preallocated\n", __FUNCTION__, sg_list->address);
    }
    fbe_cmi_kernel_message->fixed_data_sgl[0].PhysAddress = fbe_get_contigmem_physical_address(sg_list->address);
    fbe_cmi_kernel_message->fixed_data_sgl[0].length = float_data->fixed_data_length;
    fbe_cmi_kernel_message->fixed_data_sgl[1].PhysAddress = NULL;
    fbe_cmi_kernel_message->fixed_data_sgl[1].length = 0;

    /*fbe_cmi_kernel_message->fixed_data_blob.blob_sgl[0].PhysAddress = fbe_cmi_io_get_peer_slot_address(FBE_FALSE, float_data->pool, float_data->slot);
    fbe_cmi_kernel_message->fixed_data_blob.blob_sgl[0].length = float_data->fixed_data_length;
    fbe_cmi_kernel_message->fixed_data_blob.blob_sgl[1].PhysAddress = NULL;
    fbe_cmi_kernel_message->fixed_data_blob.blob_sgl[1].length = 0;*/

    while ((sg_ptr[sgl_count].length != 0) && (sgl_count < (MAX_SEP_IO_SG_COUNT - 1)))
    {
        data_length += sg_ptr[sgl_count].length;
        sgl_count++;
    }
    if (data_length != float_data->fixed_data_length) {
        fbe_cmi_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, "%s data len 0x%x not as expected 0x%x sgl %d\n", __FUNCTION__,
                      data_length, float_data->fixed_data_length, sgl_count);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_cmi_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, "%s slot %d_%d len 0x%x\n", __FUNCTION__, 
                  ctx_info->pool, ctx_info->slot, float_data->fixed_data_length);
    fbe_cmi_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, "packet %p sgl %d first len 0x%x addr 0x%llx\n", 
                  float_data->original_packet, (sgl_count+1), sg_ptr[0].length, sg_ptr[0].PhysAddress);

    /* set the fixed data sgl to point to the pre-allocated memory in our data structure*/
    fbe_cmi_kernel_message->cmid_ioctl.fixed_data_sgl = fbe_cmi_kernel_message->fixed_data_sgl;
    fbe_cmi_kernel_message->cmid_ioctl.fixedDataDescriptionBlob = (PVOID)&fbe_cmi_kernel_message->fixed_data_blob;
    fbe_cmi_kernel_message->cmid_ioctl.fixedDataDescriptionBlobLength = (sgl_count + 1) * sizeof(CMI_PHYSICAL_SG_ELEMENT);

    return FBE_STATUS_OK;
}
/****************************************************************
 * end fbe_cmi_io_fill_fixed_data_read_response()
 ****************************************************************/

/*!**************************************************************
 * fbe_cmi_io_fill_fixed_data_control_request()
 ****************************************************************
 * @brief
 *  This function fills fixed data for control request. 
 *
 * @param
 *   ctx_info - pointer to context.
 *   packet_p - pointer to the packet.
 *
 * @return fbe_status_t
 * 
 * @author
 *  06/11/2013 - Created. Lili Chen
 *
 ****************************************************************/
static fbe_status_t 
fbe_cmi_io_fill_fixed_data_control_request(fbe_cmi_io_context_info_t * ctx_info,
                                           fbe_packet_t * packet_p)
{
    fbe_payload_ex_t *              payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_payload_control_operation_t *control_payload_p = fbe_payload_ex_get_control_operation(payload_p);
    fbe_cmi_io_float_data_t * float_data = &ctx_info->float_data;
    fbe_cmi_io_message_info_t * fbe_cmi_kernel_message = &ctx_info->kernel_message;

    fbe_cmi_kernel_message->fixed_data_sgl[0].PhysAddress = fbe_get_contigmem_physical_address(control_payload_p->buffer);
    fbe_cmi_kernel_message->fixed_data_sgl[0].length = float_data->fixed_data_length;
    fbe_cmi_kernel_message->fixed_data_sgl[1].PhysAddress = NULL;
    fbe_cmi_kernel_message->fixed_data_sgl[1].length = 0;

    fbe_cmi_kernel_message->fixed_data_blob.blob_sgl[0].PhysAddress = fbe_cmi_io_get_peer_slot_address(FBE_TRUE, float_data->pool, float_data->slot);
    fbe_cmi_kernel_message->fixed_data_blob.blob_sgl[0].length = float_data->fixed_data_length;
    fbe_cmi_kernel_message->fixed_data_blob.blob_sgl[1].PhysAddress = NULL;
    fbe_cmi_kernel_message->fixed_data_blob.blob_sgl[1].length = 0;

    /* set the fixed data sgl to point to the pre-allocated memory in our data structure*/
    fbe_cmi_kernel_message->cmid_ioctl.fixed_data_sgl = fbe_cmi_kernel_message->fixed_data_sgl;
    fbe_cmi_kernel_message->cmid_ioctl.fixedDataDescriptionBlob = (PVOID)&fbe_cmi_kernel_message->fixed_data_blob;
    fbe_cmi_kernel_message->cmid_ioctl.fixedDataDescriptionBlobLength = 2*sizeof(CMI_PHYSICAL_SG_ELEMENT);

    return FBE_STATUS_OK;
}
/****************************************************************
 * end fbe_cmi_io_fill_fixed_data_control_request()
 ****************************************************************/

/*!**************************************************************
 * fbe_cmi_kernel_send_packet_completion()
 ****************************************************************
 * @brief
 *  This is the callback function for SEP IO client when a packet
 *  transfer is complete. 
 *
 * @param
 *   DeviceObject - device object.
 *   Context      - context.
 *
 * @return fbe_status_t
 * 
 * @author
 *  04/22/2012 - Created. Lili Chen
 *
 ****************************************************************/
static EMCPAL_STATUS fbe_cmi_kernel_send_packet_completion(PEMCPAL_DEVICE_OBJECT DeviceObject,PEMCPAL_IRP Irp, PVOID  Context)
{
    fbe_cmi_io_context_info_t * ctx_info = (fbe_cmi_io_context_info_t *)Context;

    FBE_UNREFERENCED_PARAMETER(DeviceObject);
    fbe_cmi_io_return_held_context(ctx_info);

    return EMCPAL_STATUS_MORE_PROCESSING_REQUIRED;
}
/****************************************************************
 * end fbe_cmi_kernel_send_packet_completion()
 ****************************************************************/

/*!**************************************************************
 * fbe_cmi_kernel_io_fill_cmid_ioctl()
 ****************************************************************
 * @brief
 *  This function is used to fill kernel message structure
 *  (with both floating data and fixed data). 
 *
 * @param
 *   fbe_cmi_kernel_message - the data structure to fill.
 *   message_length - message length.
 *   message - message.
 *   packet_p - pointer to packet.
 *
 * @return fbe_status_t
 * 
 * @author
 *  04/22/2012 - Created. Lili Chen
 *
 ****************************************************************/
static fbe_status_t fbe_cmi_io_fill_cmid_ioctl(fbe_cmi_io_context_info_t * ctx_info,
                                               fbe_packet_t * packet_p)
{
    fbe_cmi_conduit_id_t	conduit;
    CMI_CONDUIT_ID			cmid_conduit_id = Cmi_Invalid_Conduit;
    fbe_cmi_client_id_t     client_id = FBE_CMI_CLIENT_ID_SEP_IO;
    fbe_status_t			status = FBE_STATUS_GENERIC_FAILURE;
    PEMCPAL_IO_STACK_LOCATION		newIrpStack = NULL;
    fbe_cmi_io_float_data_t *       float_data = &ctx_info->float_data;
    fbe_cmi_io_message_info_t *     fbe_cmi_kernel_message = &ctx_info->kernel_message;
    fbe_status_t fbe_cmi_kernel_get_cmid_conduit_id(fbe_cmi_conduit_id_t conduit_id, CMI_CONDUIT_ID * cmid_conduit_id);

    /* Fill kernel message structure */
    fbe_cmi_kernel_message->client_id = client_id;
    fbe_cmi_kernel_message->user_msg = float_data;

    fbe_cmi_kernel_message->cmid_ioctl.transmission_handle = fbe_cmi_kernel_message;/*this is how we will find ourselvs back on completion*/

    conduit = fbe_cmi_service_sep_io_get_conduit(packet_p->cpu_id);
    status = fbe_cmi_kernel_get_cmid_conduit_id(conduit, &cmid_conduit_id);
    if (status != FBE_STATUS_OK) {
        fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR, "%s cannot get cmid conduit id\n", __FUNCTION__);
        return status;
    }else{
        fbe_cmi_kernel_message->cmid_ioctl.conduit_id = cmid_conduit_id;
    }
    
    /* copy our sp id to the structure */
    fbe_copy_memory(&fbe_cmi_kernel_message->cmid_ioctl.destination_sp, &fbe_cmi_kernel_other_sp_id, sizeof(SPID));
    /* Fill client ID */
    fbe_copy_memory(fbe_cmi_kernel_message->cmid_ioctl.extra_data, &client_id, sizeof(fbe_cmi_client_id_t));

    /* set up floating data sgl correctly */
    fbe_cmi_kernel_message->cmi_sgl[0].PhysAddress = fbe_get_contigmem_physical_address(float_data);
    fbe_cmi_kernel_message->cmi_sgl[0].length = sizeof(fbe_cmi_io_float_data_t);
    fbe_cmi_kernel_message->cmi_sgl[1].PhysAddress = NULL;/*termination*/
    fbe_cmi_kernel_message->cmi_sgl[1].length = 0;
    /* set the floating data sgl to the pre-allocated memory in our data structure */
    fbe_cmi_kernel_message->cmid_ioctl.physical_floating_data_sgl = fbe_cmi_kernel_message->cmi_sgl;
    fbe_cmi_kernel_message->cmid_ioctl.physical_data_offset = 0;
    fbe_cmi_kernel_message->cmid_ioctl.virtual_floating_data_sgl = NULL;
    fbe_cmi_kernel_message->cmid_ioctl.fixedDataDescriptionBlob = NULL;
    fbe_cmi_kernel_message->cmid_ioctl.fixedDataDescriptionBlobLength = 0;
    fbe_cmi_kernel_message->cmid_ioctl.fixed_data_sgl = NULL;
    fbe_cmi_kernel_message->cmid_ioctl.cancelled = FALSE;

    /* Setup fixed_data_sgl and fixed_data_blob */
    if ((float_data->payload_opcode == FBE_PAYLOAD_OPCODE_BLOCK_OPERATION) &&
        (float_data->fixed_data_length != 0))
    {
        fbe_payload_ex_t * payload_p = fbe_transport_get_payload_ex(packet_p);
        fbe_payload_block_operation_t * block_operation_p = fbe_payload_ex_get_present_block_operation(payload_p);
        fbe_payload_block_operation_opcode_t    opcode;
        fbe_payload_block_get_opcode(block_operation_p, &opcode);

        if (float_data->msg_type == FBE_CMI_IO_MESSAGE_TYPE_PACKET_REQUEST)
        {
            if ((fbe_payload_block_operation_opcode_is_media_modify(opcode)) &&
                (fbe_payload_block_operation_opcode_requires_sg(opcode)))
            {
                status = fbe_cmi_io_fill_fixed_data_write_request(ctx_info, packet_p);
            }
            else if ((fbe_payload_block_operation_opcode_is_media_read(opcode)) &&
                (fbe_payload_block_operation_opcode_requires_sg(opcode)))
            {
                status = fbe_cmi_io_fill_fixed_data_read_request(ctx_info, packet_p);
            }
        }
        else if (float_data->msg_type == FBE_CMI_IO_MESSAGE_TYPE_PACKET_RESPONSE)
        {
            if ((fbe_payload_block_operation_opcode_is_media_read(opcode)) &&
                (fbe_payload_block_operation_opcode_requires_sg(opcode)))
            {
                status = fbe_cmi_io_fill_fixed_data_read_response(ctx_info, packet_p);
            }
        }

    }
    else if ((float_data->payload_opcode == FBE_PAYLOAD_OPCODE_CONTROL_OPERATION) && 
             (float_data->fixed_data_length != 0) &&
             (float_data->msg_type == FBE_CMI_IO_MESSAGE_TYPE_PACKET_REQUEST))
    {
        status = fbe_cmi_io_fill_fixed_data_control_request(ctx_info, packet_p);
    }

    EmcpalIrpReuse(fbe_cmi_kernel_message->pirp, EMCPAL_STATUS_PENDING);
    /* fbe_cmi_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, "kernel IO slot %d, current 0x%x StackCount0x%x.\n", 
			float_data->slot, fbe_cmi_kernel_message->pirp->CurrentLocation, fbe_cmi_kernel_message->pirp->StackCount);*/
    newIrpStack = EmcpalIrpGetNextStackLocation(fbe_cmi_kernel_message->pirp);
    EmcpalExtIrpStackMajorFunctionSet(newIrpStack, EMCPAL_IRP_TYPE_CODE_DEVICE_CONTROL);
    EmcpalExtIrpStackMinorFunctionSet(newIrpStack, 0);
    EmcpalExtIrpStackParamIoctlCodeSet(newIrpStack, IOCTL_CMI_TRANSMIT_MESSAGE);
    EmcpalExtIrpStackParamIoctlInputSizeSet(newIrpStack, sizeof(fbe_cmi_kernel_message->cmid_ioctl));
    EmcpalExtIrpStackParamIoctlOutputSizeSet(newIrpStack, 0);
    EmcpalIrpStackSetFlags(newIrpStack, 0);
    
    /*we use that since IOCTL_CMI_TRANSMIT_MESSAGE is define as NEITHER*/
    EmcpalExtIrpStackParamType3InputBufferSet(newIrpStack, &fbe_cmi_kernel_message->cmid_ioctl);
    EmcpalExtIrpCompletionRoutineSet(fbe_cmi_kernel_message->pirp, fbe_cmi_kernel_send_packet_completion, ctx_info, TRUE, TRUE, TRUE);

    return FBE_STATUS_OK;

}
/****************************************************************
 * end fbe_cmi_kernel_io_fill_cmid_ioctl()
 ****************************************************************/

/*!**************************************************************
 * fbe_cmi_io_start_send()
 ****************************************************************
 * @brief
 *  This function is used to start sending packet over FBE CMI 
 *  in kernel. 
 *
 * @param
 *   ctx_info - context info.
 *   packet_p - pointer to packet.
 *
 * @return fbe_status_t
 * 
 * @author
 *  06/01/2012 - Created. Lili Chen
 *
 ****************************************************************/
fbe_status_t fbe_cmi_io_start_send(fbe_cmi_io_context_info_t * ctx_info,
                                   fbe_packet_t * packet_p)
{
    fbe_status_t                     status;
    EMCPAL_STATUS                    nt_status = EMCPAL_STATUS_UNSUCCESSFUL;
    fbe_cmi_io_message_info_t *      kernel_message = &ctx_info->kernel_message;

    /* Fill CMID related information */
    status = fbe_cmi_io_fill_cmid_ioctl(ctx_info, packet_p);
    if (status != FBE_STATUS_OK) {
        fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR, "%s, failed to fill CMI_TRANSMIT_MESSAGE_IOCTL_INFO\n", __FUNCTION__);
        fbe_cmi_io_return_held_context(ctx_info);
        return status;
    }

    fbe_cmi_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, "%s, sending packet %p, slot %d\n", __FUNCTION__, packet_p, ctx_info->slot);
    /* Send IRP */
    nt_status = EmcpalExtIrpSendAsync(EmcpalDeviceGetRelatedDeviceObject(fbe_cmi_driver_info.cmid_file_object), kernel_message->pirp);
    if (nt_status == EMCPAL_STATUS_PORT_DISCONNECTED) {
        cmi_service_handshake_set_peer_alive(FBE_FALSE);
        return FBE_STATUS_NO_DEVICE;
    }else if (nt_status != EMCPAL_STATUS_SUCCESS && nt_status != EMCPAL_STATUS_PENDING) {
        return FBE_STATUS_GENERIC_FAILURE;
    }else{
        return FBE_STATUS_OK;
    }
}
/****************************************************************
 * end fbe_cmi_io_start_send()
 ****************************************************************/

fbe_u64_t fbe_cmi_io_get_physical_address(void *virt_address)
{
    return (fbe_get_contigmem_physical_address(virt_address));
}


/*!**************************************************************
 * fbe_cmi_kernel_send_memory_completion()
 ****************************************************************
 * @brief
 *  This is the callback function for SEP IO client when a memory
 *  transfer is complete. 
 *
 * @param
 *   DeviceObject - device object.
 *   Context      - context.
 *
 * @return fbe_status_t
 * 
 * @author
 *  10/22/2013 - Created. Lili Chen
 *
 ****************************************************************/
static EMCPAL_STATUS fbe_cmi_kernel_send_memory_completion(PEMCPAL_DEVICE_OBJECT DeviceObject,PEMCPAL_IRP Irp, PVOID  Context)
{
    fbe_cmi_io_context_info_t * ctx_info = (fbe_cmi_io_context_info_t *)Context;

    FBE_UNREFERENCED_PARAMETER(DeviceObject);
    fbe_cmi_io_return_send_context_for_memory(ctx_info);

    return EMCPAL_STATUS_MORE_PROCESSING_REQUIRED;
}
/****************************************************************
 * end fbe_cmi_kernel_send_memory_completion()
 ****************************************************************/

/*!**************************************************************
 * fbe_cmi_memory_fill_cmid_ioctl()
 ****************************************************************
 * @brief
 *  This function is used to fill kernel message structure
 *  (with both floating data and fixed data). 
 *
 * @param
 *   fbe_cmi_kernel_message - the data structure to fill.
 *   message_length - message length.
 *   message - message.
 *   packet_p - pointer to packet.
 *
 * @return fbe_status_t
 * 
 * @author
 *  10/22/2013 - Created. Lili Chen
 *
 ****************************************************************/
static fbe_status_t fbe_cmi_memory_fill_cmid_ioctl(fbe_cmi_io_context_info_t * ctx_info, fbe_cmi_memory_t * send_memory_p)
{
    fbe_cmi_conduit_id_t	conduit = FBE_CMI_CONDUIT_ID_SEP_IO_CORE0;
    CMI_CONDUIT_ID			cmid_conduit_id = Cmi_Invalid_Conduit;
    fbe_cmi_client_id_t     client_id = FBE_CMI_CLIENT_ID_SEP_IO;
    fbe_status_t			status = FBE_STATUS_GENERIC_FAILURE;
    PEMCPAL_IO_STACK_LOCATION		newIrpStack = NULL;
    fbe_cmi_io_float_data_t *       float_data = &ctx_info->float_data;
    fbe_cmi_io_message_info_t *     fbe_cmi_kernel_message = &ctx_info->kernel_message;
    fbe_status_t fbe_cmi_kernel_get_cmid_conduit_id(fbe_cmi_conduit_id_t conduit_id, CMI_CONDUIT_ID * cmid_conduit_id);

    /* Fill kernel message structure */
    fbe_cmi_kernel_message->client_id = client_id;
    fbe_cmi_kernel_message->user_msg = float_data;
    fbe_cmi_kernel_message->context = ctx_info;

    fbe_cmi_kernel_message->cmid_ioctl.transmission_handle = fbe_cmi_kernel_message;/*this is how we will find ourselvs back on completion*/

    //conduit = fbe_cmi_service_sep_io_get_conduit(fbe_get_cpu_id());
    status = fbe_cmi_kernel_get_cmid_conduit_id(conduit, &cmid_conduit_id);
    if (status != FBE_STATUS_OK) {
        fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR, "%s cannot get cmid conduit id\n", __FUNCTION__);
        return status;
    }else{
        fbe_cmi_kernel_message->cmid_ioctl.conduit_id = cmid_conduit_id;
    }
    
    /* copy our sp id to the structure */
    fbe_copy_memory(&fbe_cmi_kernel_message->cmid_ioctl.destination_sp, &fbe_cmi_kernel_other_sp_id, sizeof(SPID));
    /* Fill client ID */
    fbe_copy_memory(fbe_cmi_kernel_message->cmid_ioctl.extra_data, &client_id, sizeof(fbe_cmi_client_id_t));

    /* set up floating data sgl correctly */
    fbe_cmi_kernel_message->cmi_sgl[0].PhysAddress = fbe_get_contigmem_physical_address(float_data);
    fbe_cmi_kernel_message->cmi_sgl[0].length = sizeof(fbe_cmi_io_float_data_t);
    fbe_cmi_kernel_message->cmi_sgl[1].PhysAddress = NULL;/*termination*/
    fbe_cmi_kernel_message->cmi_sgl[1].length = 0;
    /* set the floating data sgl to the pre-allocated memory in our data structure */
    fbe_cmi_kernel_message->cmid_ioctl.physical_floating_data_sgl = fbe_cmi_kernel_message->cmi_sgl;
    fbe_cmi_kernel_message->cmid_ioctl.physical_data_offset = 0;
    fbe_cmi_kernel_message->cmid_ioctl.virtual_floating_data_sgl = NULL;
    //fbe_cmi_kernel_message->cmid_ioctl.fixedDataDescriptionBlob = NULL;
    //fbe_cmi_kernel_message->cmid_ioctl.fixedDataDescriptionBlobLength = 0;
    //fbe_cmi_kernel_message->cmid_ioctl.fixed_data_sgl = NULL;
    fbe_cmi_kernel_message->cmid_ioctl.cancelled = FALSE;

    /* Setup fixed_data_sgl and fixed_data_blob */
    fbe_cmi_kernel_message->fixed_data_sgl[0].PhysAddress = send_memory_p->source_addr;
    fbe_cmi_kernel_message->fixed_data_sgl[0].length = send_memory_p->data_length;
    fbe_cmi_kernel_message->fixed_data_sgl[1].PhysAddress = NULL;
    fbe_cmi_kernel_message->fixed_data_sgl[1].length = 0;

    fbe_cmi_kernel_message->fixed_data_blob.blob_sgl[0].PhysAddress = send_memory_p->dest_addr;
    fbe_cmi_kernel_message->fixed_data_blob.blob_sgl[0].length = send_memory_p->data_length;
    fbe_cmi_kernel_message->fixed_data_blob.blob_sgl[1].PhysAddress = NULL;
    fbe_cmi_kernel_message->fixed_data_blob.blob_sgl[1].length = 0;

    /* set the fixed data sgl to point to the pre-allocated memory in our data structure*/
    fbe_cmi_kernel_message->cmid_ioctl.fixed_data_sgl = fbe_cmi_kernel_message->fixed_data_sgl;
    fbe_cmi_kernel_message->cmid_ioctl.fixedDataDescriptionBlob = (PVOID)&fbe_cmi_kernel_message->fixed_data_blob;
    fbe_cmi_kernel_message->cmid_ioctl.fixedDataDescriptionBlobLength = 2*sizeof(CMI_PHYSICAL_SG_ELEMENT);

    EmcpalIrpReuse(fbe_cmi_kernel_message->pirp, EMCPAL_STATUS_PENDING);
    newIrpStack = EmcpalIrpGetNextStackLocation(fbe_cmi_kernel_message->pirp);
    EmcpalExtIrpStackMajorFunctionSet(newIrpStack, EMCPAL_IRP_TYPE_CODE_DEVICE_CONTROL);
    EmcpalExtIrpStackMinorFunctionSet(newIrpStack, 0);
    EmcpalExtIrpStackParamIoctlCodeSet(newIrpStack, IOCTL_CMI_TRANSMIT_MESSAGE);
    EmcpalExtIrpStackParamIoctlInputSizeSet(newIrpStack, sizeof(fbe_cmi_kernel_message->cmid_ioctl));
    EmcpalExtIrpStackParamIoctlOutputSizeSet(newIrpStack, 0);
    EmcpalIrpStackSetFlags(newIrpStack, 0);
    
    /*we use that since IOCTL_CMI_TRANSMIT_MESSAGE is define as NEITHER*/
    EmcpalExtIrpStackParamType3InputBufferSet(newIrpStack, &fbe_cmi_kernel_message->cmid_ioctl);
    EmcpalExtIrpCompletionRoutineSet(fbe_cmi_kernel_message->pirp, fbe_cmi_kernel_send_memory_completion, ctx_info, TRUE, TRUE, TRUE);

    return FBE_STATUS_OK;

}
/****************************************************************
 * end fbe_cmi_memory_fill_cmid_ioctl()
 ****************************************************************/

/*!**************************************************************
 * fbe_cmi_memory_start_send()
 ****************************************************************
 * @brief
 *  This function is used to start sending memory over FBE CMI 
 *  in kernel. 
 *
 * @param
 *   ctx_info - context info.
 *   packet_p - pointer to packet.
 *
 * @return fbe_status_t
 * 
 * @author
 *  10/22/2013 - Created. Lili Chen
 *
 ****************************************************************/
fbe_status_t fbe_cmi_memory_start_send(fbe_cmi_io_context_info_t * ctx_info, fbe_cmi_memory_t * send_memory_p)
{
    fbe_status_t                     status;
    EMCPAL_STATUS                    nt_status = EMCPAL_STATUS_UNSUCCESSFUL;
    fbe_cmi_io_message_info_t *      kernel_message = &ctx_info->kernel_message;

    /* Fill CMID related information */
    status = fbe_cmi_memory_fill_cmid_ioctl(ctx_info, send_memory_p);
    if (status != FBE_STATUS_OK) {
        fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR, "%s, failed to fill CMI_TRANSMIT_MESSAGE_IOCTL_INFO\n", __FUNCTION__);
        return status;
    }

    fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, "%s, sending memory slot %d_%d\n", __FUNCTION__, ctx_info->pool, ctx_info->slot);
    /* Send IRP */
    nt_status = EmcpalExtIrpSendAsync(EmcpalDeviceGetRelatedDeviceObject(fbe_cmi_driver_info.cmid_file_object), kernel_message->pirp);
    if (nt_status == EMCPAL_STATUS_PORT_DISCONNECTED) {
        cmi_service_handshake_set_peer_alive(FBE_FALSE);
        return FBE_STATUS_NO_DEVICE;
    }else if (nt_status != EMCPAL_STATUS_SUCCESS && nt_status != EMCPAL_STATUS_PENDING) {
        return FBE_STATUS_GENERIC_FAILURE;
    }else{
        return FBE_STATUS_OK;
    }
}
/****************************************************************
 * end fbe_cmi_memory_start_send()
 ****************************************************************/
