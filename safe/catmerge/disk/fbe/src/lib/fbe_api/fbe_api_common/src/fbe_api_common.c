/***************************************************************************
 * Copyright (C) EMC Corporation 2001-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_api_common.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the common interfaces for FBE API.
 *      
 * @ingroup fbe_api_system_package_interface_class_files
 * @ingroup fbe_api_common_interface
 *
 *    
 ***************************************************************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_trace_interface.h"
#include "fbe/fbe_traffic_trace_interface.h"
#include "fbe/fbe_library_interface.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe_trace.h"
#include "fbe_base.h"
#include "fbe_api_packet_queue.h"
#include "fbe/fbe_api_discovery_interface.h"

#ifdef C4_INTEGRATED
#include "fbe/fbe_physical_package.h"

csx_p_bufman_t              fbe_io_bufman = NULL;
csx_bool_t                  call_from_fbe_cli_user = CSX_FALSE;
csx_p_rdevice_reference_t   ppDeviceRef = CSX_NULL;
#endif /* C4_INTEGRATED - C4ARCH - some sort of call/IOCTL wiring complexity */

/*********************************************************************/
/*                  loacal definitions                              */
/*********************************************************************/
#if 0
static fbe_u32_t    init_reference_count = 0;
#endif
#define FBE_API_POOL_TAG    'aebf'

/* Got an unknown enum value.
 * It's not necessarily an error.
 * This usually means that we're not up-to-date on the enum.
 */
static const fbe_u8_t *fbe_api_common_unknown_value(const fbe_u8_t *name, fbe_u32_t value)
{
    fbe_api_trace(FBE_TRACE_LEVEL_INFO, "%s:\tunknown %s %d(%#x)\n", __FUNCTION__, name, value, value);
    return "?";
}

/** Trace aid. */
#define CASENAME(E) case E: return #E
#define DFLTNAME(V, E)  default: return fbe_api_common_unknown_value(#V, E)


/*!********************************************************************* 
 * @struct state_enum_to_str_t 
 *  
 * @brief 
 *   This contains the data info for changing the life cycle state to string.
 *
 * @ingroup fbe_api_common_interface
 * @ingroup state_enum_to_str
 **********************************************************************/
typedef struct state_enum_to_str_s{
    fbe_lifecycle_state_t       state;      /*!< life cycle state */
    const fbe_u8_t *            state_str;  /*!< pointer to the output state string */
}state_enum_to_str_t;

static state_enum_to_str_t      state_table [] =
{
    FBE_API_ENUM_TO_STR(FBE_LIFECYCLE_STATE_SPECIALIZE),      
    FBE_API_ENUM_TO_STR(FBE_LIFECYCLE_STATE_ACTIVATE),             
    FBE_API_ENUM_TO_STR(FBE_LIFECYCLE_STATE_READY),                
    FBE_API_ENUM_TO_STR(FBE_LIFECYCLE_STATE_HIBERNATE),            
    FBE_API_ENUM_TO_STR(FBE_LIFECYCLE_STATE_OFFLINE),             
    FBE_API_ENUM_TO_STR(FBE_LIFECYCLE_STATE_FAIL),                 
    FBE_API_ENUM_TO_STR(FBE_LIFECYCLE_STATE_DESTROY),              
    FBE_API_ENUM_TO_STR(FBE_LIFECYCLE_STATE_PENDING_READY),
    FBE_API_ENUM_TO_STR(FBE_LIFECYCLE_STATE_PENDING_ACTIVATE),
    FBE_API_ENUM_TO_STR(FBE_LIFECYCLE_STATE_PENDING_HIBERNATE),
    FBE_API_ENUM_TO_STR(FBE_LIFECYCLE_STATE_PENDING_OFFLINE),
    FBE_API_ENUM_TO_STR(FBE_LIFECYCLE_STATE_PENDING_FAIL),
    FBE_API_ENUM_TO_STR(FBE_LIFECYCLE_STATE_PENDING_DESTROY),
    FBE_API_ENUM_TO_STR(FBE_LIFECYCLE_STATE_NOT_EXIST) ,
    FBE_API_ENUM_TO_STR(FBE_LIFECYCLE_STATE_INVALID)
};

/*!********************************************************************* 
 * @struct state_enum_to_str_t 
 *  
 * @brief 
 *   This contains the data info for changing the traffic priority  state to string.
 *
 * @ingroup fbe_api_common_interface
 * @ingroup state_enum_to_str
 **********************************************************************/
typedef struct traffic_priority_enum_to_str_s{
    fbe_traffic_priority_t       traffic_priority;      /*!< traffic priority value */
    const fbe_u8_t *            traffic_priority_str;  /*!< pointer to the traffic priority string */
}traffic_priority_enum_to_str_t;

static traffic_priority_enum_to_str_t      traffic_priority_table [] =
{
    FBE_API_ENUM_TO_STR(FBE_TRAFFIC_PRIORITY_INVALID),
    FBE_API_ENUM_TO_STR(FBE_TRAFFIC_PRIORITY_VERY_LOW),
    FBE_API_ENUM_TO_STR(FBE_TRAFFIC_PRIORITY_LOW),             
    FBE_API_ENUM_TO_STR(FBE_TRAFFIC_PRIORITY_NORMAL_LOW),                
    FBE_API_ENUM_TO_STR(FBE_TRAFFIC_PRIORITY_NORMAL),            
    FBE_API_ENUM_TO_STR(FBE_TRAFFIC_PRIORITY_NORMAL_HIGH),             
    FBE_API_ENUM_TO_STR(FBE_TRAFFIC_PRIORITY_HIGH),                 
    FBE_API_ENUM_TO_STR(FBE_TRAFFIC_PRIORITY_VERY_HIGH),              
    FBE_API_ENUM_TO_STR(FBE_TRAFFIC_PRIORITY_URGENT),
    FBE_API_ENUM_TO_STR(FBE_TRAFFIC_PRIORITY_LAST)
};

static fbe_api_libs_function_entries_t fbe_api_libs_function_entries[FBE_PACKAGE_ID_LAST];

/*********************************************************************/
/*                  loacal variables                                */
/*********************************************************************/
static fbe_queue_head_t contiguous_packet_q_head;
static fbe_spinlock_t   contiguous_packet_q_lock;
static fbe_queue_head_t outstanding_contiguous_packet_q_head;
/*********************************************************************/
/*                  loacal functions                                */
/*********************************************************************/

/*********************************************************************
 *            fbe_api_allocate_memory ()
 *********************************************************************
 *
 *  Description: memory allocation, ready to be cahnged for csx if needed
 *               memory is always zeroed
 *               !!!!Contiguous memory is not guaranteed here, but non pages is guaranteed!!!
 *
 *  Return Value: memory pointer
 *
 *  History:
 *    10/10/08: sharel  created
 *    
 *********************************************************************/
void * fbe_api_allocate_memory (fbe_u32_t  NumberOfBytes)
{
    void *      mem_ptr = NULL;

#ifdef C4_INTEGRATED
    /* fbecli is a program that links this code and does not need buffman or io memory */
    if (call_from_fbe_cli_user)
    {
        return (PVOID) csx_p_util_mem_alloc_maybe_zeroed (NumberOfBytes);
    }
#endif /* C4_INTEGRATED - C4ARCH - some sort of call/IOCTL wiring complexity */
    mem_ptr = fbe_allocate_io_nonpaged_pool_with_tag( NumberOfBytes, FBE_API_POOL_TAG);
    
    if (mem_ptr != NULL) {
        fbe_zero_memory(mem_ptr, NumberOfBytes);
    }

    return mem_ptr;

}

/*********************************************************************
 *            fbe_api_free_memory ()
 *********************************************************************
 *
 *  Description: memory freeing, ready to be cahnged for csx if needed
 *
 *  Return Value: memory pointer
 *
 *  History:
 *    10/10/08: sharel  created
 *    
 *********************************************************************/
void fbe_api_free_memory(void *  memory_ptr)
{
#ifdef C4_INTEGRATED
    /* fbecli is a program that links this code and does not need buffman or io memory */
    if (call_from_fbe_cli_user)
    {
         csx_p_util_mem_free (memory_ptr);
         return;
    }
#endif /* C4_INTEGRATED - C4ARCH - some sort of call/IOCTL wiring complexity */
    
    fbe_release_io_nonpaged_pool_with_tag(memory_ptr, FBE_API_POOL_TAG);
}

/*!***************************************************************
 * @fn fbe_api_trace(
 *       fbe_trace_level_t trace_level,
 *       const fbe_u8_t * fmt,
 *       ...)
 *****************************************************************
 * @brief
 *   tracing shortcut fro fbe API.
 *               This API is intened only for tracing with FBE_TRACE_MESSAGE_ID_INFO
 *               which is the most common one.
 *
 * @param trace_level - trace level
 * @param fmt - format
 * @param ... - continue
 *
 * @return 
 *
 * @version
 *    10/10/08: sharel  created
 *
 ****************************************************************/
void FBE_API_CALL fbe_api_trace(fbe_trace_level_t trace_level,
                   const fbe_char_t * fmt,
                   ...)
{
    va_list             argList;
    fbe_trace_level_t   default_trace_level = fbe_trace_get_default_trace_level();

    if (trace_level > default_trace_level) {
        return;
    }

    va_start(argList, fmt);
    fbe_trace_report (FBE_COMPONENT_TYPE_LIBRARY,
                      FBE_LIBRARY_ID_FBE_API,
                      trace_level,
                      FBE_TRACE_MESSAGE_ID_INFO,
                      fmt,
                      argList); 
    va_end(argList);

}

/*!***************************************************************
 * @fn fbe_api_common_send_control_packet(
 *        fbe_payload_control_operation_opcode_t control_code,
 *        fbe_payload_control_buffer_t buffer,
 *        fbe_payload_control_buffer_length_t buffer_length,
 *        fbe_object_id_t object_id,
 *        fbe_packet_attr_t attr,
 *        fbe_api_control_operation_status_info_t *control_status_info,
 *        fbe_package_id_t package_id)
 *****************************************************************
 * @brief
 *   send a control packet to the physical package
 *
 * @param control_code - control code
 * @param buffer - buffer
 * @param buffer_length - buffer length
 * @param object_id - object ID
 * @param attr - packet attribute
 * @param control_status_info - pointer to the control status info
 * @param package_id - packet ID
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    10/10/08: sharel  created
 *
 ****************************************************************/
fbe_status_t fbe_api_common_send_control_packet(fbe_payload_control_operation_opcode_t control_code,
                                                fbe_payload_control_buffer_t buffer,
                                                fbe_payload_control_buffer_length_t buffer_length,
                                                fbe_object_id_t object_id,
                                                fbe_packet_attr_t attr,
                                                fbe_api_control_operation_status_info_t *control_status_info,
                                                fbe_package_id_t package_id)
{
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_packet_t *                      packet = NULL;
    fbe_payload_ex_t *                     payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;

#ifdef C4_INTEGRATED
    fbe_api_fbe_cli_rdevice_inbuff_t      *fbe_cli_rdevice_in_data_ptr;
    fbe_api_fbe_cli_rdevice_outbuff_t     *fbe_cli_rdevice_out_data_ptr;
    unsigned char                         *rdevice_in_buff_ptr;
    unsigned char                         *rdevice_out_buff_ptr;
    csx_status_e     csx_status;
    csx_size_t bytesReturned = 0;

    if (call_from_fbe_cli_user)
    {
        /* buffer could be input, output or both */
        rdevice_in_buff_ptr = (unsigned char *) fbe_api_allocate_memory (sizeof(fbe_api_fbe_cli_rdevice_inbuff_t) + buffer_length);
        rdevice_out_buff_ptr = (unsigned char *) fbe_api_allocate_memory (sizeof(fbe_api_fbe_cli_rdevice_outbuff_t) + buffer_length);
        if ((rdevice_in_buff_ptr == NULL)||(rdevice_out_buff_ptr == NULL)) {
            csx_p_display_std_note("%s: failed to allocate rdevice buffers\n", __FUNCTION__); 
            return FBE_STATUS_GENERIC_FAILURE;
        }
        fbe_cli_rdevice_in_data_ptr  = (fbe_api_fbe_cli_rdevice_inbuff_t *) rdevice_in_buff_ptr;
        fbe_cli_rdevice_out_data_ptr = (fbe_api_fbe_cli_rdevice_outbuff_t *) rdevice_out_buff_ptr;

        /* use rdevice ioctl interface if this is a call from fbecli.x which runs in a differnet CSX IC */
        fbe_cli_rdevice_in_data_ptr->control_code = control_code;
        fbe_cli_rdevice_in_data_ptr->object_id = object_id;
        fbe_cli_rdevice_in_data_ptr->attr = attr;
        fbe_cli_rdevice_in_data_ptr->buffer_length = buffer_length;
        fbe_cli_rdevice_in_data_ptr->sgl0_length   = 0;
        fbe_cli_rdevice_in_data_ptr->package_id = package_id;

        fbe_copy_memory (rdevice_in_buff_ptr + sizeof(fbe_api_fbe_cli_rdevice_inbuff_t), buffer,
                         buffer_length);

        csx_status = csx_p_rdevice_ioctl(ppDeviceRef,
                                    FBE_CLI_SEND_PKT_IOCTL,
                                    rdevice_in_buff_ptr,
                                    sizeof(fbe_api_fbe_cli_rdevice_inbuff_t) + buffer_length,
                                    rdevice_out_buff_ptr,
                                    sizeof(fbe_api_fbe_cli_rdevice_outbuff_t) + buffer_length,
                                    &bytesReturned);

        fbe_copy_memory (control_status_info, &(fbe_cli_rdevice_out_data_ptr->control_status_info),
                         sizeof(fbe_api_control_operation_status_info_t));

        fbe_copy_memory (buffer, rdevice_out_buff_ptr + sizeof(fbe_api_fbe_cli_rdevice_outbuff_t),
                         buffer_length);
    
        fbe_api_free_memory(rdevice_in_buff_ptr);
        fbe_api_free_memory(rdevice_out_buff_ptr);

        if (CSX_FAILURE(csx_status)) {
            csx_p_display_std_note("%s: failed rdevice ioctl to PPFDPORT0 status = 0x%x (%s)\n",
                                   __FUNCTION__, csx_status,csx_p_cvt_csx_status_to_string(csx_status)); 
            return FBE_STATUS_GENERIC_FAILURE;
        }
        else
        {
            if(0) csx_p_display_std_note("%s: rdevice ioctl to PPFDPORT0 succeeded...\n",__FUNCTION__);
            return FBE_STATUS_OK;
        }

    }
#endif /* C4_INTEGRATED - C4ARCH - some sort of call/IOCTL wiring complexity - C4BUG - IOCTL model mixing */

    /* Allocate packet.*/
    packet = fbe_api_get_contiguous_packet();
    if (packet == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: unable to allocate memory for packet\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }
 
    payload = fbe_transport_get_payload_ex (packet);
    
        // If the payload is NULL, exit and return generic failure.
        if (payload == NULL) {
            fbe_api_return_contiguous_packet(packet);/*no need to destory !*/
            fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: Payload is NULL\n", __FUNCTION__); 
            return FBE_STATUS_GENERIC_FAILURE;
        }
        
    control_operation = fbe_payload_ex_allocate_control_operation(payload);
    
        // If the control operation cannot be allocated, exit and return generic failure.
        if (control_operation == NULL) {
            fbe_api_return_contiguous_packet(packet);/*no need to destory !*/
            fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: unable to allocate new control operation\n", __FUNCTION__); 
            return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_build_operation (control_operation,
                                         control_code,
                                         buffer,
                                         buffer_length);


    /* Set packet address.*/
    fbe_transport_set_address(packet,
                              package_id,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              object_id); 
    
    /* Mark packet with the attribute, either traverse or not */
    attr |= FBE_PACKET_FLAG_SYNC;
    fbe_transport_set_packet_attr(packet, attr);

    fbe_transport_set_sync_completion_type(packet, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);
    status = fbe_api_common_send_control_packet_to_driver(packet);
    
    if (status != FBE_STATUS_OK && status != FBE_STATUS_PENDING && status != FBE_STATUS_BUSY){
        /* Only trace when it is an error we do not expect.*/
        fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s error in sending packet, status:%d\n", __FUNCTION__, status);
    }

    fbe_transport_wait_completion(packet);
    
    /* Save the status for returning.*/
    status = fbe_transport_get_status_code(packet);
    if (control_status_info != NULL){
        control_status_info->packet_status = status;
        control_status_info->packet_qualifier = fbe_transport_get_status_qualifier(packet);
        fbe_payload_control_get_status(control_operation, &control_status_info->control_operation_status);
        fbe_payload_control_get_status_qualifier(control_operation, &control_status_info->control_operation_qualifier);
    }

    fbe_payload_ex_release_control_operation(payload, control_operation);
    fbe_api_return_contiguous_packet(packet);/*no need to destory !*/

    return status;
}

/*!***************************************************************
 * @fn fbe_api_common_send_io_packet(
 *        fbe_packet_t *packet,
 *        fbe_object_id_t object_id,
 *        fbe_packet_completion_function_t completion_function,
 *        fbe_packet_completion_context_t completion_context,
 *        fbe_packet_cancel_function_t cancel_function,
 *        fbe_packet_cancel_context_t cancel_context,
 *        fbe_package_id_t package_id)
 *****************************************************************
 * @brief
 *   send an io packet to the driver
 *
 * @param packet - pointer to packet info
 * @param object_id - object ID
 * @param completion_function - completion function
 * @param completion_context - completion context
 * @param cancel_function - cancel function
 * @param cancel_context - cancel context
 * @param package_id - packet ID
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    10/17/08: sharel  created
 *    
 ****************************************************************/
fbe_status_t fbe_api_common_send_io_packet(fbe_packet_t *packet,
                                           fbe_object_id_t object_id,
                                           fbe_packet_completion_function_t completion_function,
                                           fbe_packet_completion_context_t completion_context,
                                           fbe_packet_cancel_function_t cancel_function,
                                           fbe_packet_cancel_context_t cancel_context,
                                           fbe_package_id_t package_id)
{
    fbe_status_t            status = FBE_STATUS_GENERIC_FAILURE;
    
    /* Set packet address.*/
    fbe_transport_set_address(packet,
                              package_id,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              object_id); 

    /*and completion function*/
    fbe_transport_set_completion_function(packet, 
                                          completion_function, 
                                          completion_context);

    /*cancellation function*/
    fbe_transport_set_cancel_function(packet,
                                      cancel_function,
                                      cancel_context);


    status = fbe_api_common_send_io_packet_to_driver(packet);

    if (status != FBE_STATUS_OK && status != FBE_STATUS_PENDING && status != FBE_STATUS_BUSY){
        /* Only trace when it is an error we do not expect.*/
        if (status != FBE_STATUS_EDGE_NOT_ENABLED) {
            fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s error in sending packet, status:%d\n", __FUNCTION__, status);       
        }else {
            fbe_api_trace(FBE_TRACE_LEVEL_INFO, "%s:Object went away while IO was on it's way\n", __FUNCTION__);       
        }
    }

    return status;
}

/*!***************************************************************
 * @fn fbe_api_common_send_control_packet_with_sg_list(
 *        fbe_payload_control_operation_opcode_t control_code,
 *        fbe_payload_control_buffer_t buffer,
 *        fbe_payload_control_buffer_length_t buffer_length,
 *        fbe_object_id_t object_id,
 *        fbe_packet_attr_t attr,
 *        fbe_sg_element_t *sg_list,
 *        fbe_u32_t sg_list_count,
 *        fbe_api_control_operation_status_info_t *control_staus_info,
 *        fbe_package_id_t package_id)
 *****************************************************************
 * @brief
 *   send a control packet to the physical package
 *
 * @param control_code - control code
 * @param buffer - buffer
 * @param buffer_length - buffer length
 * @param object_id - object ID
 * @param attr - packet attribute
 * @param sg_list - pointer to the sg list
 * @param sg_list_count - sg list count
 * @param control_staus_info - control status info
 * @param package_id - packet ID
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    11/10/08: sharel  created
 *
 ****************************************************************/
fbe_status_t fbe_api_common_send_control_packet_with_sg_list(fbe_payload_control_operation_opcode_t control_code,
                                                             fbe_payload_control_buffer_t buffer,
                                                             fbe_payload_control_buffer_length_t buffer_length,
                                                             fbe_object_id_t object_id,
                                                             fbe_packet_attr_t attr,
                                                             fbe_sg_element_t *sg_list,
                                                             fbe_u32_t sg_list_count,
                                                             fbe_api_control_operation_status_info_t *control_staus_info,
                                                             fbe_package_id_t package_id)
{
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_packet_t *                      packet = NULL;
    fbe_payload_ex_t *                     payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;

    packet = fbe_api_get_contiguous_packet();
    if (packet == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: unable to allocate memory for packet\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }
 
    payload = fbe_transport_get_payload_ex (packet);
    control_operation = fbe_payload_ex_allocate_control_operation(payload);
    /*set sg list*/
    fbe_payload_ex_set_sg_list (payload, sg_list, sg_list_count);
    
    fbe_payload_control_build_operation (control_operation,
                                         control_code,
                                         buffer,
                                         buffer_length);

    /* Set packet address.*/
    fbe_transport_set_address(packet,
                              package_id,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              object_id); 
    
    /* Mark packet with the attribute, either traverse or not */
    attr |= FBE_PACKET_FLAG_SYNC;
    fbe_transport_set_packet_attr(packet, attr);

    fbe_transport_set_sync_completion_type(packet, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);
    status = fbe_api_common_send_control_packet_to_driver(packet);
    
    if (status != FBE_STATUS_OK && status != FBE_STATUS_PENDING && status != FBE_STATUS_BUSY){
        /* Only trace when it is an error we do not expect.*/
        if (status != FBE_STATUS_EDGE_NOT_ENABLED) {
            fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s error in sending packet, status:%d\n", __FUNCTION__, status);       
        }else {
            fbe_api_trace(FBE_TRACE_LEVEL_INFO, "%s:Object went away while IO was on it's way\n", __FUNCTION__);       
        }
    }

    fbe_transport_wait_completion(packet);

    /* Save the status for returning.*/
    status = fbe_transport_get_status_code(packet);
    if (control_staus_info != NULL){
        control_staus_info->packet_status = status;
        control_staus_info->packet_qualifier = fbe_transport_get_status_qualifier(packet);
        fbe_payload_control_get_status(control_operation, &control_staus_info->control_operation_status);
        fbe_payload_control_get_status_qualifier(control_operation, &control_staus_info->control_operation_qualifier);
    }

    fbe_payload_ex_release_control_operation(payload, control_operation);
    fbe_api_return_contiguous_packet(packet);/*no need to destory !*/

    return status;
}

/*!*******************************************************************
 *   @fn fbe_api_common_send_control_packet_with_sg_list_async ()
 *********************************************************************
 *
 *  @brief send a control packet async to the physical package
 * 
 *  @param  control_code - the opcode
 *          buffer - the buffer poitner
 *          out_buffer_length - return buffer length
 *          object_id - object id to send to
 *          attr - traversse or not
 *          qualifier - pointer to get full packet status
 *          sg_list - pointer to the sg list
 *          sg_list_count - number of elements in the list
 *                  completion_function - callback function to complete processing
 *                  completion_context -  context for completion function. 
 *
 *  @version
 *    02/11/10: Dipak Patel created
 *    
 *********************************************************************/
fbe_status_t fbe_api_common_send_control_packet_with_sg_list_async(fbe_packet_t *packet,
                                                                   fbe_payload_control_operation_opcode_t control_code,
                                                                   fbe_payload_control_buffer_t buffer,
                                                                   fbe_payload_control_buffer_length_t buffer_length,
                                                                   fbe_object_id_t object_id,
                                                                   fbe_packet_attr_t attr,
                                                                   fbe_sg_element_t *sg_list,
                                                                   fbe_u32_t sg_list_count,
                                                                   fbe_packet_completion_function_t completion_function,
                                                                   fbe_packet_completion_context_t completion_context,
                                                                   fbe_package_id_t package_id)
{

    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_ex_t *                     payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL; 

    payload = fbe_transport_get_payload_ex (packet);
    control_operation = fbe_payload_ex_allocate_control_operation(payload);
    
    fbe_payload_control_build_operation (control_operation,
                                         control_code,
                                         buffer,
                                         buffer_length);
 
    fbe_transport_set_completion_function (packet, completion_function,  completion_context);
  
    /* Set packet address.*/
    fbe_transport_set_address(packet,
                              package_id,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              object_id); 

    /* Mark packet with the attribute, either traverse or not */
    fbe_transport_set_packet_attr(packet, (attr | FBE_PACKET_FLAG_ASYNC));
 
    /*set sg list*/
    fbe_payload_ex_set_sg_list (payload, sg_list, sg_list_count);

    status = fbe_api_common_send_control_packet_to_driver(packet);

    if (status != FBE_STATUS_OK && status != FBE_STATUS_PENDING && status != FBE_STATUS_BUSY){
    /* Only trace when it is an error we do not expect.*/
    if ( (status != FBE_STATUS_EDGE_NOT_ENABLED) &&
         (status != FBE_STATUS_NO_OBJECT) ) {
            fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s error in sending packet, status:%d\n", __FUNCTION__, status);       
        }else {
            fbe_api_trace(FBE_TRACE_LEVEL_INFO, "%s:Object went away while IO was on it's way\n", __FUNCTION__);       
        }
    }
    
    return status;
}

/*!***************************************************************
 * @fn fbe_api_state_to_string(
 *        fbe_lifecycle_state_t state_in)
 *****************************************************************
 * @brief
 *   converts lifecycle state to a string
 *
 * @param state_in - lifecyle state
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    11/19/08: sharel  created
 *    
 ****************************************************************/
const fbe_u8_t * fbe_api_state_to_string (fbe_lifecycle_state_t state_in)
{
    state_enum_to_str_t *   state_lookup = state_table;

    while (state_lookup->state != FBE_LIFECYCLE_STATE_INVALID) {
        if (state_lookup->state == state_in) {
            return state_lookup->state_str;
        }
        state_lookup++;
    }

    return state_lookup->state_str;
}

/*!***************************************************************
 * @fn fbe_api_traffic_priority_to_string(
 *        fbe_traffic_priority_t traffic_priority_in)
 *****************************************************************
 * @brief
 *   converts traffic priority to a string
 *
 * @param traffic_priority_in - traffic priority in
 *
 * @return pointer to traffic priority string 
 *
 * @version
 *    02/07/11: Vishnu Sharma  created
 *
 ****************************************************************/
const fbe_u8_t * fbe_api_traffic_priority_to_string (fbe_traffic_priority_t traffic_priority_in)
{
    traffic_priority_enum_to_str_t *   traffic_priority_lookup = traffic_priority_table;

    while (traffic_priority_lookup->traffic_priority != FBE_TRAFFIC_PRIORITY_LAST) {
        if (traffic_priority_lookup->traffic_priority == traffic_priority_in) {
            return traffic_priority_lookup->traffic_priority_str;
        }
        traffic_priority_lookup++;
    }

    return traffic_priority_lookup->traffic_priority_str;
}

/*!***************************************************************
 * @fn fbe_api_common_send_control_packet_asynch(
 *        fbe_packet_t *packet,
 *        fbe_object_id_t object_id,
 *        fbe_packet_completion_function_t completion_function,
 *        fbe_packet_completion_context_t completion_context,
 *        fbe_packet_attr_t attr,
 *        fbe_package_id_t package_id)
 *****************************************************************
 * @brief
 *   send a control packet to the physical package
 *
 * @param packet - pointer to packet data
 * @param object_id - object ID
 * @param completion_function - completion function
 * @param completion_context - completion context
 * @param attr - packet attribute
 * @param package_id - packet ID
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    11/10/08: sharel  created
 *
 ****************************************************************/
fbe_status_t fbe_api_common_send_control_packet_asynch(fbe_packet_t *packet,
                                                       fbe_object_id_t object_id,
                                                       fbe_packet_completion_function_t completion_function,
                                                       fbe_packet_completion_context_t completion_context,
                                                       fbe_packet_attr_t attr,
                                                       fbe_package_id_t package_id)
{
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    
    fbe_transport_set_completion_function (packet, completion_function,  completion_context);

    /* Set packet address.*/
    fbe_transport_set_address(packet,
                              package_id,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              object_id); 
    
    /* Mark packet with the attribute, either traverse or not */
    fbe_transport_set_packet_attr(packet, (attr | FBE_PACKET_FLAG_ASYNC));

    status = fbe_api_common_send_control_packet_to_driver(packet);

    if (status != FBE_STATUS_OK && status != FBE_STATUS_PENDING && status != FBE_STATUS_BUSY){
        /* Only trace when it is an error we do not expect.*/
         if (status != FBE_STATUS_EDGE_NOT_ENABLED) {
            fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s error in sending packet, status:%d\n", __FUNCTION__, status);       
        }else {
            fbe_api_trace(FBE_TRACE_LEVEL_INFO, "%s:Object went away while IO was on it's way\n", __FUNCTION__);       
        }
    }

    return status;
}

/*!***************************************************************
 * @fn fbe_api_common_send_control_packet_to_service_asynch(
 *        fbe_packet_t *packet_p,
 *        fbe_payload_control_operation_opcode_t control_code,
 *        fbe_payload_control_buffer_t buffer,
 *        fbe_payload_control_buffer_length_t buffer_length,
 *        fbe_service_id_t service_id,
 *        fbe_packet_attr_t attr,
 *        fbe_packet_completion_function_t completion_function,
 *        fbe_packet_completion_context_t completion_context,
 *        fbe_package_id_t package_id)
 *****************************************************************
 * @brief
 *   send a control packet to one of the services in the services
 *
 * @param packet_p - pointer to the packet structure
 * @param control_code - control code
 * @param buffer - buffer
 * @param buffer_length - buffer length
 * @param service_id - service ID
 * @param attr - packet attribute
 * @param completion_function - completion function
 * @param completion_context - completion context
 * @param package_id - packet ID
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    7/17/09: sharel   created
 *
 ****************************************************************/
fbe_status_t fbe_api_common_send_control_packet_to_service_asynch(fbe_packet_t *packet_p,
                                                                  fbe_payload_control_operation_opcode_t control_code,
                                                                  fbe_payload_control_buffer_t buffer,
                                                                  fbe_payload_control_buffer_length_t buffer_length,
                                                                  fbe_service_id_t service_id,
                                                                  fbe_packet_attr_t attr,
                                                                  fbe_packet_completion_function_t completion_function,
                                                                  fbe_packet_completion_context_t completion_context,
                                                                  fbe_package_id_t package_id)
{
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_ex_t *                  payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
 
    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_allocate_control_operation(payload);
    
    fbe_transport_set_completion_function(packet_p, completion_function,  completion_context);

    fbe_payload_control_build_operation(control_operation,
                                        control_code,
                                        buffer,
                                        buffer_length);
    
    /* Set packet address.
     */
    fbe_transport_set_address(packet_p,
                              package_id,
                              service_id,
                              FBE_CLASS_ID_INVALID,
                              FBE_OBJECT_ID_INVALID); 
    
    /* Mark packet with the attribute, either traverse or not.
     */
    fbe_transport_set_packet_attr(packet_p, (attr | FBE_PACKET_FLAG_ASYNC));

    status = fbe_api_common_send_control_packet_to_driver(packet_p);

    /* Any non-pending status is considered a failure.
     */
    if ((status != FBE_STATUS_OK) && (status != FBE_STATUS_PENDING) && status != FBE_STATUS_BUSY)
    {
         fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s error in sending packet, control_code: 0x%x status:%d\n", 
                       __FUNCTION__, control_code, status);  
    }
    return status;
}


/*!***************************************************************
 * @fn fbe_api_common_send_control_packet_to_service(
 *       fbe_payload_control_operation_opcode_t control_code,
 *       fbe_payload_control_buffer_t buffer,
 *       fbe_payload_control_buffer_length_t buffer_length,
 *       fbe_service_id_t service_id,
 *       fbe_packet_attr_t attr,
 *       fbe_api_control_operation_status_info_t *control_status_info,
 *       fbe_package_id_t package_id)
 *****************************************************************
 * @brief
 *   send a control packet to one of the services in the services
 *
 * @param control_code - control code
 * @param buffer - buffer
 * @param buffer_length - buffer length
 * @param service_id - service ID
 * @param attr - packet attribute
 * @param control_status_info - control status info
 * @param package_id - packet ID
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    7/17/09: sharel   created
 *
 ****************************************************************/
fbe_status_t fbe_api_common_send_control_packet_to_service(fbe_payload_control_operation_opcode_t control_code,
                                                           fbe_payload_control_buffer_t buffer,
                                                           fbe_payload_control_buffer_length_t buffer_length,
                                                           fbe_service_id_t service_id,
                                                           fbe_packet_attr_t attr,
                                                           fbe_api_control_operation_status_info_t *control_status_info,
                                                           fbe_package_id_t package_id)
{

    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_packet_t *                      packet = NULL;
    fbe_payload_ex_t *                  payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;

    /* Allocate packet.*/
    packet = fbe_api_get_contiguous_packet();
    if (packet == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: unable to allocate memory for packet\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }
 
    payload = fbe_transport_get_payload_ex (packet);
    control_operation = fbe_payload_ex_allocate_control_operation(payload);
    
    fbe_payload_control_build_operation (control_operation,
                                         control_code,
                                         buffer,
                                         buffer_length);

    
    /* Set packet address.*/
    fbe_transport_set_address(packet,
                              package_id,
                              service_id,
                              FBE_CLASS_ID_INVALID,
                              FBE_OBJECT_ID_INVALID); 
    
    /* Mark packet with the attribute, either traverse or not */
    attr |= FBE_PACKET_FLAG_SYNC;
    fbe_transport_set_packet_attr(packet, attr);

    fbe_transport_set_sync_completion_type(packet, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);
    status = fbe_api_common_send_control_packet_to_driver(packet);
    
    if ((status != FBE_STATUS_OK) && (status != FBE_STATUS_PENDING) && (status != FBE_STATUS_NOT_INITIALIZED) &&
        (status != FBE_STATUS_BUSY) && (status != FBE_STATUS_COMPONENT_NOT_FOUND))
    {
        /* Only trace when it is an error we do not expect.*/
         fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s error in sending packet, status:%d\n", __FUNCTION__, status);  
    }
    else
    {
        /* We must wait with ms, since in a test environment we need to be able to fail and cleanup 
         * when the entity we sent to never returns the packet. 
         */
        fbe_transport_wait_completion_ms(packet, 300000);
    }

    /* Save the status for returning.*/
    status = fbe_transport_get_status_code(packet);
    if (control_status_info != NULL){
        control_status_info->packet_status = status;
        control_status_info->packet_qualifier = fbe_transport_get_status_qualifier(packet);
        fbe_payload_control_get_status(control_operation, &control_status_info->control_operation_status);
        fbe_payload_control_get_status_qualifier(control_operation, &control_status_info->control_operation_qualifier);
    }

    fbe_payload_ex_release_control_operation(payload, control_operation);
    fbe_api_return_contiguous_packet(packet);

    return status;

}


/*!***************************************************************
 * @fn fbe_api_trace_get_level(
 *       fbe_api_trace_level_control_t * p_level_control, 
 *       fbe_package_id_t package_id)
 *****************************************************************
 * @brief
 *   Get trace level 
 *
 * @param p_level_control - pointer to the level control
 * @param package_id - packet ID
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    7/17/09: sharel   created
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_trace_get_level(fbe_api_trace_level_control_t * p_level_control, fbe_package_id_t package_id)
{
    fbe_trace_level_control_t level_control;
    fbe_api_control_operation_status_info_t status_info;
    fbe_status_t status;

    if (p_level_control == NULL) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: null control pointer\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }
    if ((p_level_control->trace_type == FBE_TRACE_TYPE_INVALID) ||
        (p_level_control->trace_type >= FBE_TRACE_TYPE_LAST)) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: invalid trace type: 0x%X\n",
                      __FUNCTION__, p_level_control->trace_type); 
        return FBE_STATUS_GENERIC_FAILURE;
    }
    level_control.trace_type = p_level_control->trace_type;
    level_control.fbe_id = p_level_control->fbe_id;
    level_control.trace_level = FBE_TRACE_LEVEL_INVALID;
    level_control.trace_flag = FBE_TRACE_FLAG_DEFAULT;
    status = fbe_api_common_send_control_packet_to_service(FBE_TRACE_CONTROL_CODE_GET_LEVEL,
                                                           &level_control,
                                                           sizeof(fbe_trace_level_control_t),
                                                           FBE_SERVICE_ID_TRACE,
                                                           FBE_PACKET_FLAG_NO_ATTRIB,
                                                           &status_info,
                                                           package_id);
    if (status != FBE_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: can't send control packet\n", __FUNCTION__); 
        return status;
    }
    if (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: get request failed\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }
    p_level_control->trace_level = level_control.trace_level;
    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn fbe_api_trace_set_level(
 *      fbe_api_trace_level_control_t * p_level_control, 
 *      fbe_package_id_t package_id)
 *****************************************************************
 * @brief
 *   Set trace level 
 *
 * @param p_level_control - pointer to the level control
 * @param package_id - packet ID
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    7/17/09: sharel   created
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_trace_set_level(fbe_api_trace_level_control_t * p_level_control, fbe_package_id_t package_id)
{
    fbe_trace_level_control_t level_control;
    fbe_api_control_operation_status_info_t status_info;
    fbe_status_t status;

    if (p_level_control == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: null control pointer\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }
    if ((p_level_control->trace_type == FBE_TRACE_TYPE_INVALID) ||
        (p_level_control->trace_type >= FBE_TRACE_TYPE_LAST)) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: invalid trace type: 0x%X\n",
                      __FUNCTION__, p_level_control->trace_type); 
        return FBE_STATUS_GENERIC_FAILURE;
    }
    if ((p_level_control->trace_level == FBE_TRACE_LEVEL_INVALID) ||
        (p_level_control->trace_level >= FBE_TRACE_LEVEL_LAST)) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: invalid trace level: 0x%X\n",
                      __FUNCTION__, p_level_control->trace_level); 
        return FBE_STATUS_GENERIC_FAILURE;
    }
    level_control.trace_type = p_level_control->trace_type;
    level_control.fbe_id = p_level_control->fbe_id;
    level_control.trace_level = p_level_control->trace_level;
    status = fbe_api_common_send_control_packet_to_service(FBE_TRACE_CONTROL_CODE_SET_LEVEL,
                                                           &level_control,
                                                           sizeof(fbe_trace_level_control_t),
                                                           FBE_SERVICE_ID_TRACE,
                                                           FBE_PACKET_FLAG_NO_ATTRIB,
                                                           &status_info,
                                                           package_id);
    if (status != FBE_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: can't send control packet\n", __FUNCTION__); 
        return status;
    }
    if (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: set request failed\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn fbe_api_trace_get_error_counter(
 *        fbe_api_trace_error_counters_t * p_error_counters, 
 *        fbe_package_id_t package_id)
 *****************************************************************
 * @brief
 *   Get error counter
 *
 * @param p_error_counters - pointer to the error counters
 * @param package_id - packet ID
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    7/17/09: sharel   created
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_trace_get_error_counter(fbe_api_trace_error_counters_t * p_error_counters, fbe_package_id_t package_id)
{
    fbe_trace_error_counters_t error_counters;
    fbe_api_control_operation_status_info_t status_info;
    fbe_status_t status;

    if (p_error_counters == NULL) {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: null control pointer\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }
    error_counters.trace_critical_error_counter = 0;
    error_counters.trace_error_counter = 0;
    status = fbe_api_common_send_control_packet_to_service(FBE_TRACE_CONTROL_CODE_GET_ERROR_COUNTERS,
                                                           &error_counters,
                                                           sizeof(fbe_trace_error_counters_t),
                                                           FBE_SERVICE_ID_TRACE,
                                                           FBE_PACKET_FLAG_NO_ATTRIB,
                                                           &status_info,
                                                           package_id);
    if (status != FBE_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: can't send control packet\n", __FUNCTION__); 
        return status;
    }
    if (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: get request failed\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }
    p_error_counters->trace_critical_error_counter = error_counters.trace_critical_error_counter;
    p_error_counters->trace_error_counter = error_counters.trace_error_counter;
    return FBE_STATUS_OK;
}

fbe_status_t FBE_API_CALL fbe_api_trace_clear_error_counter(fbe_package_id_t package_id)
{
    fbe_api_control_operation_status_info_t status_info;
    fbe_status_t status;

    status = fbe_api_common_send_control_packet_to_service(FBE_TRACE_CONTROL_CODE_CLEAR_ERROR_COUNTERS,
                                                           NULL,
                                                           0,
                                                           FBE_SERVICE_ID_TRACE,
                                                           FBE_PACKET_FLAG_NO_ATTRIB,
                                                           &status_info,
                                                           package_id);
    if (status != FBE_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: can't send control packet\n", __FUNCTION__); 
        return status;
    }
    if (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: get request failed\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

	return FBE_STATUS_OK;
}
/*!***************************************************************
 * @fn fbe_api_common_send_control_packet_to_driver (fbe_packet_t *packet)
 *****************************************************************
 * @brief
 *   This is where we route the Control packet to the correct location
 *   We need this support in case the application links with more than one driver
 *
 * @param packet - pointer to the packet
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    7/17/09: sharel   created
 *
 ****************************************************************/
fbe_status_t fbe_api_common_send_control_packet_to_driver (fbe_packet_t *packet)
{
    fbe_package_id_t    package_id = FBE_PACKAGE_ID_LAST;
    fbe_cpu_id_t cpu_id;

    fbe_transport_get_package_id(packet, &package_id);

    fbe_transport_get_cpu_id(packet, &cpu_id);
    if(cpu_id == FBE_CPU_ID_INVALID){
        cpu_id = fbe_get_cpu_id();
        fbe_transport_set_cpu_id(packet, cpu_id);
    }

    if (fbe_api_libs_function_entries[package_id].magic_number != IO_AND_CONTROL_ENTRY_MAGIC_NUMBER) {
        fbe_api_trace (FBE_TRACE_LEVEL_WARNING, "%s: entry not initialized, package: %d\n", __FUNCTION__, package_id); 
        fbe_transport_set_status(packet, FBE_STATUS_NO_OBJECT, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* If someone sets the internal flag we will make it appear that we are inside.
     */
    if ((packet->packet_attr & FBE_PACKET_FLAG_INTERNAL) == 0)
    {
        fbe_transport_set_packet_attr(packet, FBE_PACKET_FLAG_EXTERNAL);
    }
    return fbe_api_libs_function_entries[package_id].lib_control_entry(packet);
}

/*!***************************************************************
 * @fn fbe_api_common_send_io_packet_to_driver(fbe_packet_t *packet)
 *****************************************************************
 * @brief
 *   This is where we route the IO packet to the correct location
 *   We need this support in case the application links with more than one driver
 *
 * @param packet - pointer to the packet
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    7/17/09: sharel   created
 *
 ****************************************************************/
fbe_status_t fbe_api_common_send_io_packet_to_driver (fbe_packet_t *packet)
{
    fbe_package_id_t    package_id = FBE_PACKAGE_ID_LAST;

    fbe_transport_get_package_id(packet, &package_id);

    if (fbe_api_libs_function_entries[package_id].magic_number != IO_AND_CONTROL_ENTRY_MAGIC_NUMBER) {
        /* We need to complete the packet since the caller must always assume the packet 
         * completion is always called regardless of what the status of sending the packet is.
         */
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: entry not initialized\n", __FUNCTION__); 
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return fbe_api_libs_function_entries[package_id].lib_io_entry(packet);
}


/*!***************************************************************
 * @fn fbe_api_sleep(fbe_u32_t msec)
 *****************************************************************
 * @brief
 *   msec to sleep
 *
 * @param msec - msec to sleep
 *
 * @return 
 *
 * @version
 *    11/20/09: Created. Peter Puhov
 *
 ****************************************************************/
void  FBE_API_CALL fbe_api_sleep (fbe_u32_t msec)
{
    fbe_thread_delay(msec);
}


/*!***************************************************************
 * @fn fbe_api_common_set_function_entries(
 *       fbe_api_libs_function_entries_t * entries, 
 *       fbe_package_id_t package_id)
 *****************************************************************
 * @brief
 *   set function entries
 *
 * @param entries - pointer to the function entries
 * @param package_id - package ID
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    11/20/09: Created. Peter Puhov
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_common_set_function_entries (fbe_api_libs_function_entries_t * entries, fbe_package_id_t package_id)
{
    if (entries == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: entries is NULL\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_copy_memory(&fbe_api_libs_function_entries[package_id], entries, sizeof (fbe_api_libs_function_entries_t));
    fbe_api_libs_function_entries[package_id].magic_number = IO_AND_CONTROL_ENTRY_MAGIC_NUMBER;

    return FBE_STATUS_OK;
}


/*!***************************************************************
 * @fn fbe_api_common_send_control_packet_to_class(
 *        fbe_payload_control_operation_opcode_t control_code,
 *        fbe_payload_control_buffer_t buffer,
 *        fbe_payload_control_buffer_length_t buffer_length,
 *        fbe_class_id_t class_id,
 *        fbe_packet_attr_t attr,
 *        fbe_api_control_operation_status_info_t *control_status_info,
 *        fbe_package_id_t package_id)
 *****************************************************************
 * @brief
 *   send a control packet to one of the services in the services 
 *
 * @param control_code - the opcode
 * @param buffer - the buffer poitner
 * @param buffer_length - length of buffer
 * @param class_id - class id to send to
 * @param attr - traversse or not
 * @param control_status_info - pointer to the control status info
 * @param package_id - package ID
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    11/20/09: Created. Peter Puhov
 *
 ****************************************************************/
fbe_status_t fbe_api_common_send_control_packet_to_class(fbe_payload_control_operation_opcode_t control_code,
                                                           fbe_payload_control_buffer_t buffer,
                                                           fbe_payload_control_buffer_length_t buffer_length,
                                                           fbe_class_id_t class_id,
                                                           fbe_packet_attr_t attr,
                                                           fbe_api_control_operation_status_info_t *control_status_info,
                                                           fbe_package_id_t package_id)
{

    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_packet_t *                      packet = NULL;
    fbe_payload_ex_t *                  payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;

    /* Allocate packet.*/
    packet = fbe_api_get_contiguous_packet();
    if (packet == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: unable to allocate memory for packet\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }
 
    payload = fbe_transport_get_payload_ex (packet);
    control_operation = fbe_payload_ex_allocate_control_operation(payload);
    
    fbe_payload_control_build_operation (control_operation,
                                         control_code,
                                         buffer,
                                         buffer_length);

    
    /* Set packet address.*/
    fbe_transport_set_address(packet,
                              package_id,
                              FBE_SERVICE_ID_TOPOLOGY,
                              class_id,
                              FBE_OBJECT_ID_INVALID); 
    
    /* Mark packet with the attribute, either traverse or not */
    attr |= FBE_PACKET_FLAG_SYNC;
    fbe_transport_set_packet_attr(packet, attr);

    fbe_transport_set_sync_completion_type(packet, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);
    status = fbe_api_common_send_control_packet_to_driver(packet);
    
    if (status != FBE_STATUS_OK && status != FBE_STATUS_PENDING && status != FBE_STATUS_BUSY){
        /* Only trace when it is an error we do not expect.*/
         fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s error in sending packet, status:%d\n", __FUNCTION__, status);
    }

    fbe_transport_wait_completion(packet);

    /* Save the status for returning.*/
    status = fbe_transport_get_status_code(packet);
    if (control_status_info != NULL){
        control_status_info->packet_status = status;
        control_status_info->packet_qualifier = fbe_transport_get_status_qualifier(packet);
        fbe_payload_control_get_status(control_operation, &control_status_info->control_operation_status);
        fbe_payload_control_get_status_qualifier(control_operation, &control_status_info->control_operation_qualifier);
    }

    fbe_payload_ex_release_control_operation(payload, control_operation);
    fbe_api_return_contiguous_packet(packet);

    return status;

}

/*!***************************************************************
 * @fn fbe_api_trace_get_flags(
 *       fbe_api_trace_level_control_t * p_flag_control)
 *****************************************************************
 * @brief
 *   This function gets trace flag control level. 
 *
 * @param p_flag_control - pointer to the flag control
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_trace_get_flags(fbe_api_trace_level_control_t * p_flag_control,
                                                  fbe_package_id_t package_id)
{
    fbe_trace_level_control_t flag_control;
    fbe_api_control_operation_status_info_t status_info;
    fbe_status_t status;

    if (p_flag_control == NULL) 
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: NULL control pointer\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if ((p_flag_control->trace_type == FBE_TRACE_TYPE_FLAG_INVALID) ||
        (p_flag_control->trace_type >= FBE_TRACE_TYPE_FLAG_LAST)) 
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: invalid trace flag: 0x%X\n",
                      __FUNCTION__, p_flag_control->trace_flag); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    flag_control.trace_type = p_flag_control->trace_type;
    flag_control.fbe_id = p_flag_control->fbe_id;
    flag_control.trace_level = FBE_TRACE_LEVEL_INVALID;
    flag_control.trace_flag = FBE_TRACE_FLAG_DEFAULT;

    status = fbe_api_common_send_control_packet_to_service(FBE_TRACE_CONTROL_CODE_GET_TRACE_FLAG,
                                                           &flag_control,
                                                           sizeof(fbe_trace_level_control_t),
                                                           FBE_SERVICE_ID_TRACE,
                                                           FBE_PACKET_FLAG_NO_ATTRIB,
                                                           &status_info,
                                                           package_id);
    if (status != FBE_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: can't send control packet\n", __FUNCTION__); 
        return status;
    }

    if (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: get request failed\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }
    p_flag_control->trace_flag = flag_control.trace_flag;
    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn fbe_api_trace_set_flags(
 *       fbe_api_trace_level_control_t * p_flag_control)
 *****************************************************************
 * @brief
 *   This function sets trace flags
 *
 * @param p_flag_control - pointer to the flag control
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_trace_set_flags(fbe_api_trace_level_control_t * p_flag_control,
                                                  fbe_package_id_t package_id)
{
    fbe_trace_level_control_t flag_control;
    fbe_api_control_operation_status_info_t status_info;
    fbe_status_t status;

    if (p_flag_control == NULL) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: NULL control pointer\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if ((p_flag_control->trace_type == FBE_TRACE_TYPE_FLAG_INVALID) ||
        (p_flag_control->trace_type >= FBE_TRACE_TYPE_FLAG_LAST)) 
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: invalid trace flag: 0x%X\n",
                      __FUNCTION__, p_flag_control->trace_flag); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    flag_control.trace_type = p_flag_control->trace_type;
    flag_control.fbe_id = p_flag_control->fbe_id;
    flag_control.trace_level = FBE_TRACE_LEVEL_INVALID;
    flag_control.trace_flag = p_flag_control->trace_flag;

    status = fbe_api_common_send_control_packet_to_service(FBE_TRACE_CONTROL_CODE_SET_TRACE_FLAG,
                                                           &flag_control,
                                                           sizeof(fbe_trace_level_control_t),
                                                           FBE_SERVICE_ID_TRACE,
                                                           FBE_PACKET_FLAG_NO_ATTRIB,
                                                           &status_info,
                                                           package_id);
    if (status != FBE_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: can't send control packet\n", __FUNCTION__); 
        return status;
    }

    if (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: set request failed\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}

/*!***************************************************************
 *  fbe_api_trace_set_error_limit()
 *****************************************************************
 * @brief
 *   This function sets a limit on given errors and specifies
 *   an action to take when the limit is hit.
 *
 * @param trace_level - Level to set a limit on.
 * @param action - Action to take when number of errors encountered
 *                 is greater than or equal to the num errors passed in.
 *                 Set this to FBE_API_TRACE_ERROR_LIMIT_ACTION_INVALID
 *                 if you want to disable this limit.
 * @param num_errors - Number of errors before we should take action.
 * @param package_id - Package to send this to.
 * 
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_trace_set_error_limit(fbe_trace_level_t trace_level,
                                                        fbe_api_trace_error_limit_action_t action,
                                                        fbe_u32_t num_errors,
                                                        fbe_package_id_t package_id)
{
    fbe_trace_error_limit_t trace_error_limit;
    fbe_api_control_operation_status_info_t status_info;
    fbe_status_t status;

    if ((trace_level == FBE_TRACE_LEVEL_INVALID) ||
        (trace_level >= FBE_TRACE_LEVEL_LAST)) 
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: invalid trace level: 0x%X\n",
                      __FUNCTION__, trace_level); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    trace_error_limit.trace_level = trace_level;
    trace_error_limit.num_errors = num_errors;

    switch (action)
    {
        case FBE_API_TRACE_ERROR_LIMIT_ACTION_INVALID:
            trace_error_limit.action = FBE_TRACE_ERROR_LIMIT_ACTION_INVALID;
            break;
        case FBE_API_TRACE_ERROR_LIMIT_ACTION_TRACE:
            trace_error_limit.action = FBE_TRACE_ERROR_LIMIT_ACTION_TRACE;
            break;
        case FBE_API_TRACE_ERROR_LIMIT_ACTION_STOP_SYSTEM:
            trace_error_limit.action = FBE_TRACE_ERROR_LIMIT_ACTION_STOP_SYSTEM;
            break;
        default:
            fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: invalid action: 0x%X\n",
                          __FUNCTION__, action); 
        return FBE_STATUS_GENERIC_FAILURE;
            break;
    }

    status = fbe_api_common_send_control_packet_to_service(FBE_TRACE_CONTROL_CODE_SET_ERROR_LIMIT,
                                                           &trace_error_limit,
                                                           sizeof(fbe_trace_error_limit_t),
                                                           FBE_SERVICE_ID_TRACE,
                                                           FBE_PACKET_FLAG_NO_ATTRIB,
                                                           &status_info,
                                                           package_id);
    if (status != FBE_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: can't send control packet\n", __FUNCTION__); 
        return status;
    }

    if (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: set request failed\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_api_trace_set_error_limit()
 **************************************/

/*!***************************************************************
 *  fbe_api_trace_get_error_limit()
 *****************************************************************
 * @brief
 *   This function gets the trace error limits which are
 *   currently configured for a given package.
 *
 * @param package_id - Package to get error limits for.
 * @param error_limit_p - Structure to put fetched limits in.
 * 
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_trace_get_error_limit(fbe_package_id_t package_id,
                                                        fbe_api_trace_get_error_limit_t *error_limit_p)
{
    fbe_u32_t level;
    fbe_trace_get_error_limit_t trace_error_limit;
    fbe_api_control_operation_status_info_t status_info;
    fbe_status_t status;

    status = fbe_api_common_send_control_packet_to_service(FBE_TRACE_CONTROL_CODE_GET_ERROR_LIMIT,
                                                           &trace_error_limit,
                                                           sizeof(fbe_trace_get_error_limit_t),
                                                           FBE_SERVICE_ID_TRACE,
                                                           FBE_PACKET_FLAG_NO_ATTRIB,
                                                           &status_info,
                                                           package_id);
    if (status != FBE_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: can't send control packet\n", __FUNCTION__); 
        return status;
    }

    if (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: set request failed\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* Copy data from our fbe_trace struct to our fbe_api_trace struct.
     */
    for ( level = 0; level < FBE_TRACE_LEVEL_LAST; level++)
    {
        error_limit_p->records[level].error_limit = trace_error_limit.records[level].error_limit;
        error_limit_p->records[level].num_errors = trace_error_limit.records[level].num_errors;

        /* action needs to be translated from one enum to another.
         */
        switch (trace_error_limit.records[level].action)
        {
            case FBE_TRACE_ERROR_LIMIT_ACTION_TRACE:
                error_limit_p->records[level].action = FBE_API_TRACE_ERROR_LIMIT_ACTION_TRACE;
                break;
            case FBE_TRACE_ERROR_LIMIT_ACTION_STOP_SYSTEM:
                error_limit_p->records[level].action = FBE_API_TRACE_ERROR_LIMIT_ACTION_STOP_SYSTEM;
                break;

            case FBE_TRACE_ERROR_LIMIT_ACTION_INVALID:
            default:
                error_limit_p->records[level].action = FBE_API_TRACE_ERROR_LIMIT_ACTION_INVALID;
                break;
        };
    }
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_api_trace_get_error_limit()
 **************************************/

/*!***************************************************************
 *  fbe_api_traffic_trace_enable()
 *****************************************************************
 * @brief
 *   This function enables RBA tracing for user simulator.
 *   Since on kernel, we use RBA tool to enable tracing
 *
 * @param object_tag - set RBA tracing for object tag.
 * 
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_traffic_trace_enable(fbe_u32_t object_tag,
                                                        fbe_package_id_t package_id)
{
    fbe_status_t status;
    fbe_api_control_operation_status_info_t status_info;

    status = fbe_api_common_send_control_packet_to_service(FBE_TRAFFIC_TRACE_CONTROL_CODE_ENABLE_RBA_TRACE,
                                                           &object_tag,
                                                           sizeof(object_tag),
                                                           FBE_SERVICE_ID_TRAFFIC_TRACE,
                                                           FBE_PACKET_FLAG_NO_ATTRIB,
                                                           &status_info,
                                                           package_id);
    if (status != FBE_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: can't send control packet\n", __FUNCTION__); 
        return status;
    }

    if (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: set request failed\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_api_traffic_trace_enable()
 **************************************/

/*!***************************************************************
 *  fbe_api_traffic_trace_disable()
 *****************************************************************
 * @brief
 *   This function disables RBA tracing for user simulator.
 *   Since on kernel, we use RBA tool to disable tracing
 *
 * @param object_tag - set RBA tracing for object tag.
 * 
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_traffic_trace_disable(fbe_u32_t object_tag,
                                                        fbe_package_id_t package_id)
{
    fbe_status_t status;
    fbe_api_control_operation_status_info_t status_info;

    status = fbe_api_common_send_control_packet_to_service(FBE_TRAFFIC_TRACE_CONTROL_CODE_DISABLE_RBA_TRACE,
                                                           &object_tag,
                                                           sizeof(object_tag),
                                                           FBE_SERVICE_ID_TRAFFIC_TRACE,
                                                           FBE_PACKET_FLAG_NO_ATTRIB,
                                                           &status_info,
                                                           package_id);
    if (status != FBE_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: can't send control packet\n", __FUNCTION__); 
        return status;
    }

    if (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: set request failed\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_api_traffic_trace_disable()
 **************************************/

/*!***************************************************************
 * @fn fbe_api_common_send_control_packet_to_service_with_sg_list(
 *       fbe_payload_control_operation_opcode_t control_code,
 *       fbe_payload_control_buffer_t buffer,
 *       fbe_payload_control_buffer_length_t buffer_length,
 *       fbe_service_id_t service_id,
 *       fbe_packet_attr_t attr,
 *       fbe_sg_element_t *sg_list,
 *       fbe_u32_t sg_list_count,
 *       fbe_api_control_operation_status_info_t *control_status_info,
 *       fbe_package_id_t package_id)
 *****************************************************************
 * @brief
 *   This function sends the control packet to the service with
 *   sg list
 *
 * @param control_code - control code
 * @param buffer - buffer
 * @param buffer_length - buffer length
 * @param service_id - service ID
 * @param attr - packet attribute
 * @param sg_list - sg list
 * @param sg_list_count - sg list count
 * @param control_status_info - control status info
 * @param package_id - packet ID
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 ****************************************************************/
fbe_status_t fbe_api_common_send_control_packet_to_service_with_sg_list(fbe_payload_control_operation_opcode_t control_code,
                                                                        fbe_payload_control_buffer_t buffer,
                                                                        fbe_payload_control_buffer_length_t buffer_length,
                                                                        fbe_service_id_t service_id,
                                                                        fbe_packet_attr_t attr,
                                                                        fbe_sg_element_t *sg_list,
                                                                        fbe_u32_t sg_list_count,
                                                                        fbe_api_control_operation_status_info_t *control_status_info,
                                                                        fbe_package_id_t package_id)
{
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_packet_t *                      packet = NULL;
    fbe_payload_ex_t *                  payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;

    /* Allocate packet.*/
    packet = fbe_api_get_contiguous_packet();
    if (packet == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: unable to allocate memory for packet\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }
 
    payload = fbe_transport_get_payload_ex (packet);
    control_operation = fbe_payload_ex_allocate_control_operation(payload);
    /*set sg list*/
    fbe_payload_ex_set_sg_list (payload, sg_list, sg_list_count);
    
    fbe_payload_control_build_operation (control_operation,
                                         control_code,
                                         buffer,
                                         buffer_length);

    
    /* Set packet address.*/
    fbe_transport_set_address(packet,
                              package_id,
                              service_id,
                              FBE_CLASS_ID_INVALID,
                              FBE_OBJECT_ID_INVALID); 
    
    /* Mark packet with the attribute, either traverse or not */
    attr |= FBE_PACKET_FLAG_SYNC;
    fbe_transport_set_packet_attr(packet, attr);

    fbe_transport_set_sync_completion_type(packet, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);
    status = fbe_api_common_send_control_packet_to_driver(packet);
    
    if ((status != FBE_STATUS_OK) && (status != FBE_STATUS_PENDING) && (status != FBE_STATUS_NOT_INITIALIZED) &&
        (status != FBE_STATUS_BUSY) && (status != FBE_STATUS_NO_OBJECT) && (status != FBE_STATUS_COMPONENT_NOT_FOUND)){
        /* Only trace when it is an error we do not expect.*/
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s error in sending packet, status:%d\n", __FUNCTION__, status);  
    }

    fbe_transport_wait_completion(packet);

    /* Save the status for returning.*/
    status = fbe_transport_get_status_code(packet);
    if (control_status_info != NULL){
        control_status_info->packet_status = status;
        control_status_info->packet_qualifier = fbe_transport_get_status_qualifier(packet);
        fbe_payload_control_get_status(control_operation, &control_status_info->control_operation_status);
        fbe_payload_control_get_status_qualifier(control_operation, &control_status_info->control_operation_qualifier);
    }

    fbe_payload_ex_release_control_operation(payload, control_operation);
    fbe_api_return_contiguous_packet(packet);

    return status;


}

/*!***************************************************************
 * @fn fbe_api_esp_getObjectId()
 ****************************************************************
 * @brief
 *  This function will retrieve the object ID for the specified
 *  class from ESP.
 *
 * @param 
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  03/16/10 - Created. Joe Perry
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL 
fbe_api_esp_getObjectId(fbe_class_id_t espClassId,
                        fbe_object_id_t *object_id)
{
    fbe_status_t                                status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t actual_objects;
    fbe_object_id_t *obj_list_p = NULL;

    if (object_id == NULL) 
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_enumerate_class(espClassId,  
                                     FBE_PACKAGE_ID_ESP,
                                     &obj_list_p, 
                                     &actual_objects);

    if (status != FBE_STATUS_OK) 
    {
        if(obj_list_p != NULL) 
        {
            fbe_api_free_memory(obj_list_p);
        }
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
                       "%s: packet error:%d\n", 
                       __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if(actual_objects != 1) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, 
                       "%s: ESP objects are singleton. Class:%d, No. of Objects:%d\n", 
                       __FUNCTION__, espClassId, actual_objects);
        fbe_api_free_memory(obj_list_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    *object_id = *obj_list_p;

    fbe_api_free_memory(obj_list_p);
    return status;

}   // end of fbe_api_esp_getObjectId

/*********************************************************************
 *            fbe_api_allocate_contiguous_memory ()
 *********************************************************************
 *
 *  Description: memory allocation, ready to be cahnged for csx if needed
 *               memory is always zeroed
 *               Contiguous memory IS guaranteed here, and non pages is guaranteed
 *
 *  Return Value: memory pointer
 *
 *  History:
 *    10/10/08: sharel  created
 *    
 *********************************************************************/
PVOID fbe_api_allocate_contiguous_memory (ULONG  NumberOfBytes)
{
    void *                  mem_ptr = NULL;

#ifdef C4_INTEGRATED
    /* fbecli is a program that links this code and does not need buffman or io memory */
    if (call_from_fbe_cli_user)
    {
        return (PVOID) csx_p_util_mem_alloc_maybe (NumberOfBytes);
    }
#endif /* C4_INTEGRATED - C4ARCH - some sort of call/IOCTL wiring complexity */

    mem_ptr = fbe_allocate_contiguous_memory(NumberOfBytes);
    if (mem_ptr != NULL) {
        fbe_zero_memory(mem_ptr, NumberOfBytes);
    }

    return mem_ptr;

}

/*********************************************************************
 *            fbe_api_free_contiguous_memory ()
 *********************************************************************
 *
 *  Description: memory freeing, ready to be cahnged for csx if needed
 *
 *  Return Value: memory pointer
 *
 *  History:
 *    10/10/08: sharel  created
 *    
 *********************************************************************/
void fbe_api_free_contiguous_memory (PVOID  P)
{
#ifdef C4_INTEGRATED
    /* fbecli is a program that links this code and does not need buffman or io memory */
    if (call_from_fbe_cli_user)
    {
        csx_p_util_mem_free (P);
        return;
    }
#endif /* C4_INTEGRATED - C4ARCH - some sort of call/IOCTL wiring complexity */
    fbe_release_contiguous_memory(P); /* SAFEBUG - MORPHEUS */
}




fbe_packet_t * fbe_api_get_contiguous_packet()
{
    fbe_packet_t *packet = NULL;
    fbe_api_packet_q_element_t *packet_q_element = NULL;
    fbe_status_t status;

    fbe_spinlock_lock(&contiguous_packet_q_lock);

    if (!fbe_queue_is_empty(&contiguous_packet_q_head)) {
        packet_q_element = (fbe_api_packet_q_element_t *)fbe_queue_pop(&contiguous_packet_q_head);
        packet = &packet_q_element->packet;
        packet->magic_number = FBE_MAGIC_NUMBER_BASE_PACKET;
    }else{
        fbe_spinlock_unlock(&contiguous_packet_q_lock);
        /*we ran out of pre-allocated packates so let's allocate it dynamically and mark it as such so we can return it when we are done*/
        packet_q_element = fbe_api_allocate_contiguous_memory(sizeof(fbe_api_packet_q_element_t));
        if (packet_q_element == NULL) {
            fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: can't allocated contig. packets\n", __FUNCTION__); 
            return NULL;
        }
        
        fbe_zero_memory(packet_q_element, sizeof(fbe_api_packet_q_element_t));
        packet_q_element->dynamically_allocated = FBE_TRUE;/*mark it as an extra packet which we want to free when it's done*/

        fbe_queue_element_init(&packet_q_element->q_element);
        status = fbe_transport_initialize_packet(&packet_q_element->packet);
        if (status != FBE_STATUS_OK) {
            fbe_api_free_contiguous_memory(packet_q_element);
            fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: can't init packet\n", __FUNCTION__); 
            return NULL;
        }

        packet = &packet_q_element->packet;

        fbe_spinlock_lock(&contiguous_packet_q_lock);
    }

    fbe_queue_push(&outstanding_contiguous_packet_q_head, &packet_q_element->q_element);/*move to outstanding queue*/

    fbe_spinlock_unlock(&contiguous_packet_q_lock);

    return packet;

}

void fbe_api_return_contiguous_packet (fbe_packet_t *packet)
{
    /*for now it's easy, the queue elment starts before the packet*/
    fbe_queue_element_t *q_element  = (fbe_queue_element_t *)((fbe_u8_t *)((fbe_u8_t *)packet - sizeof(fbe_queue_element_t)));
    fbe_api_packet_q_element_t * queued_packet = (fbe_api_packet_q_element_t*)q_element;

    fbe_spinlock_lock(&contiguous_packet_q_lock);

    fbe_queue_remove(q_element);/*remove from outstanding queue*/

    if (queued_packet->dynamically_allocated) {
        fbe_transport_destroy_packet(&queued_packet->packet);
        fbe_api_free_contiguous_memory(queued_packet);
    }else{
        fbe_transport_reuse_packet(packet);
        packet->magic_number = FBE_MAGIC_NUMBER_DESTROYED_PACKET;
        fbe_queue_push(&contiguous_packet_q_head, q_element);
    }

    fbe_spinlock_unlock(&contiguous_packet_q_lock);

}




/*********************************************************************
 *            fbe_api_job_service_notification_error_code_to_string ()
 *********************************************************************
 *
 *  Description: return the string for the job service error type
 *
 *  Return Value: @param error_type string 
 *
 *  History:
 *    10/26/10: DM created
 *    
 *********************************************************************/
const fbe_u8_t *
fbe_api_job_service_notification_error_code_to_string(fbe_job_service_error_type_t job_error_code)
{
    switch(job_error_code)
    {
        CASENAME(FBE_JOB_SERVICE_ERROR_NO_ERROR);

        /* common job service errors. */
        CASENAME(FBE_JOB_SERVICE_ERROR_INTERNAL_ERROR);
        CASENAME(FBE_JOB_SERVICE_ERROR_INVALID_VALUE);
        CASENAME(FBE_JOB_SERVICE_ERROR_INVALID_ID);                           /* RG/LUN create invalid id*/
        CASENAME(FBE_JOB_SERVICE_ERROR_NULL_COMMAND);
        CASENAME(FBE_JOB_SERVICE_ERROR_UNKNOWN_ID);                           /* RG destroy and LUN update unknown id*/
        CASENAME(FBE_JOB_SERVICE_ERROR_INVALID_JOB_ELEMENT);
        CASENAME(FBE_JOB_SERVICE_ERROR_TIMEOUT);                              /* The job didn't complete in the time specified*/

        /* create/destroy library error codes. */
        CASENAME(FBE_JOB_SERVICE_ERROR_INSUFFICIENT_DRIVE_CAPACITY);          /* One or more drives had insufficient capacity */
        CASENAME(FBE_JOB_SERVICE_ERROR_RAID_GROUP_ID_IN_USE);                 /* RG create rg id is in use*/
        CASENAME(FBE_JOB_SERVICE_ERROR_LUN_ID_IN_USE);                        /* LUN create lun id is in use*/
        CASENAME(FBE_JOB_SERVICE_ERROR_INVALID_CONFIGURATION);                /* RG create invalid configuration*/
        CASENAME(FBE_JOB_SERVICE_ERROR_BAD_PVD_CONFIGURATION);                /* RG create on a drive that doesn't exist*/
        CASENAME(FBE_JOB_SERVICE_ERROR_DUPLICATE_PVD);                        /* RG create on a duplicate drive*/
        CASENAME(FBE_JOB_SERVICE_ERROR_INCOMPATIBLE_PVDS);                    /* RG create on a incompatible drive*/
        CASENAME(FBE_JOB_SERVICE_ERROR_INCOMPATIBLE_DRIVE_TYPES);             /* RG create with incompatiable drive types*/
        CASENAME(FBE_JOB_SERVICE_ERROR_INVALID_RAID_GROUP_TYPE);              /* RG create with invalid rg type*/
        CASENAME(FBE_JOB_SERVICE_ERROR_INVALID_RAID_GROUP_NUMBER);            /* RG create with invalid rg number*/
        CASENAME(FBE_JOB_SERVICE_ERROR_REQUEST_BEYOND_CURRENT_RG_CAPACITY);   /* RG/LUN create request beyond current available capacity*/
        CASENAME(FBE_JOB_SERVICE_ERROR_INVALID_DRIVE_COUNT);                  /* RG create with invalid drive count*/
        CASENAME(FBE_JOB_SERVICE_ERROR_REQUEST_OBJECT_HAS_UPSTREAM_EDGES);    /* RG destroy object which has lun connect to it*/
        CASENAME(FBE_JOB_SERVICE_ERROR_INVALID_CAPACITY);                     /* LUN create got invalid capacity from RG*/
        CASENAME(FBE_JOB_SERVICE_ERROR_INVALID_ADDRESS_OFFSET);               /* LUN create with invalid NDB address offset*/
        CASENAME(FBE_JOB_SERVICE_ERROR_CONFIG_UPDATE_FAILED);
        CASENAME(FBE_JOB_SERVICE_ERROR_RAID_GROUP_NOT_READY);                 /* LUN create on the rg which is not ready*/
        CASENAME(FBE_JOB_SERVICE_ERROR_SYSTEM_LIMITS_EXCEEDED);               /* RG/LUN create exceed system configuration limits*/
        CASENAME(FBE_JOB_SERVICE_ERROR_DB_DRIVE_DOUBLE_DEGRADED);         /* The DB drives are double degraded. so the triple mirror is fragile */
        CASENAME(FBE_JOB_SERVICE_ERROR_INVALID);

        /* Non-terminal errors*/
        CASENAME(FBE_JOB_SERVICE_ERROR_PRESENTLY_NO_SPARES_AVAILABLE);
        CASENAME(FBE_JOB_SERVICE_ERROR_PRESENTLY_NO_SUITABLE_SPARE);
        CASENAME(FBE_JOB_SERVICE_ERROR_PRESENTLY_RAID_GROUP_DENIED);
        CASENAME(FBE_JOB_SERVICE_ERROR_PRESENTLY_RAID_GROUP_DEGRADED);
        CASENAME(FBE_JOB_SERVICE_ERROR_PRESENTLY_RAID_GROUP_BROKEN);
        CASENAME(FBE_JOB_SERVICE_ERROR_PRESENTLY_RAID_GROUP_HAS_COPY_IN_PROGRESS);
        CASENAME(FBE_JOB_SERVICE_ERROR_PRESENTLY_SOURCE_DRIVE_DEGRADED);

        /* spare library job service error codes. */
        CASENAME(FBE_JOB_SERVICE_ERROR_INVALID_SWAP_COMMAND);
        CASENAME(FBE_JOB_SERVICE_ERROR_INVALID_VIRTUAL_DRIVE_OBJECT_ID);
        CASENAME(FBE_JOB_SERVICE_ERROR_INVALID_EDGE_INDEX);
        CASENAME(FBE_JOB_SERVICE_ERROR_INVALID_PERM_SPARE_EDGE_INDEX);
        CASENAME(FBE_JOB_SERVICE_ERROR_INVALID_PROACTIVE_SPARE_EDGE_INDEX);
        CASENAME(FBE_JOB_SERVICE_ERROR_INVALID_ORIGINAL_OBJECT_ID);
        CASENAME(FBE_JOB_SERVICE_ERROR_INVALID_SPARE_OBJECT_ID);
        CASENAME(FBE_JOB_SERVICE_ERROR_SWAP_UNEXPECTED_ERROR);
        CASENAME(FBE_JOB_SERVICE_ERROR_SWAP_PERMANENT_SPARE_NOT_REQUIRED);
        CASENAME(FBE_JOB_SERVICE_ERROR_SWAP_PROACTIVE_SPARE_NOT_REQUIRED);
        CASENAME(FBE_JOB_SERVICE_ERROR_SWAP_VIRTUAL_DRIVE_BROKEN);
        CASENAME(FBE_JOB_SERVICE_ERROR_SWAP_CURRENT_VIRTUAL_DRIVE_CONFIG_MODE_DOESNT_SUPPORT);
        CASENAME(FBE_JOB_SERVICE_ERROR_SWAP_COPY_SOURCE_DRIVE_REMOVED);
        CASENAME(FBE_JOB_SERVICE_ERROR_SWAP_COPY_DESTINATION_DRIVE_REMOVED);
        CASENAME(FBE_JOB_SERVICE_ERROR_SWAP_COPY_INVALID_DESTINATION_DRIVE);
        CASENAME(FBE_JOB_SERVICE_ERROR_SWAP_COPY_DESTINATION_DRIVE_NOT_HEALTHY);
        CASENAME(FBE_JOB_SERVICE_ERROR_SWAP_COPY_DESTINATION_HAS_UPSTREAM_RAID_GROUP);
        CASENAME(FBE_JOB_SERVICE_ERROR_SWAP_VALIDATION_FAIL);
        CASENAME(FBE_JOB_SERVICE_ERROR_SWAP_STATUS_NOT_POPULATED);
        CASENAME(FBE_JOB_SERVICE_ERROR_SWAP_CREATE_EDGE_FAILED);
        CASENAME(FBE_JOB_SERVICE_ERROR_SWAP_DESTROY_EDGE_FAILED);

        /* spare library job service error code used for mapping spare validation failure.*/
        CASENAME(FBE_JOB_SERVICE_ERROR_SPARE_NOT_READY);
        CASENAME(FBE_JOB_SERVICE_ERROR_SPARE_REMOVED);
        CASENAME(FBE_JOB_SERVICE_ERROR_SPARE_BUSY);
        CASENAME(FBE_JOB_SERVICE_ERROR_SPARE_RAID_GROUP_UNCONSUMED);
        CASENAME(FBE_JOB_SERVICE_ERROR_SPARE_RAID_GROUP_NOT_REDUNDANT);
        CASENAME(FBE_JOB_SERVICE_ERROR_SPARE_RAID_GROUP_DESTROYED);
        CASENAME(FBE_JOB_SERVICE_ERROR_INVALID_DESIRED_SPARE_DRIVE_TYPE);
        CASENAME(FBE_JOB_SERVICE_ERROR_OFFSET_MISMATCH);
        CASENAME(FBE_JOB_SERVICE_ERROR_BLOCK_SIZE_MISMATCH);
        CASENAME(FBE_JOB_SERVICE_ERROR_CAPACITY_MISMATCH);
        CASENAME(FBE_JOB_SERVICE_ERROR_SYS_DRIVE_MISMATCH);
        CASENAME(FBE_JOB_SERVICE_ERROR_UNSUPPORTED_EVENT_CODE);

        /* configure pvd object with different config type - job service error codes. */
        CASENAME(FBE_JOB_SERVICE_ERROR_PVD_INVALID_UPDATE_TYPE);
        CASENAME(FBE_JOB_SERVICE_ERROR_PVD_INVALID_CONFIG_TYPE);
        CASENAME(FBE_JOB_SERVICE_ERROR_PVD_IS_CONFIGURED_AS_UNCONSUMED);
        CASENAME(FBE_JOB_SERVICE_ERROR_PVD_IS_CONFIGURED_AS_SPARE);
        CASENAME(FBE_JOB_SERVICE_ERROR_PVD_IS_CONFIGURED_AS_RAID);
        CASENAME(FBE_JOB_SERVICE_ERROR_PVD_IS_NOT_CONFIGURED);
        CASENAME(FBE_JOB_SERVICE_ERROR_PVD_IS_IN_USE_FOR_RAID_GROUP);
        CASENAME(FBE_JOB_SERVICE_ERROR_PVD_IS_IN_END_OF_LIFE_STATE);

        /* system drive copy back error codes*/
        CASENAME(FBE_JOB_SERVICE_ERROR_SDCB_NOT_SYSTEM_PVD);
        CASENAME(FBE_JOB_SERVICE_ERROR_SDCB_FAIL_GET_ACTION);
        CASENAME(FBE_JOB_SERVICE_ERROR_SDCB_FAIL_CHANGE_PVD_CONFIG);
        CASENAME(FBE_JOB_SERVICE_ERROR_SDCB_FAIL_POST_PROCESS);
        CASENAME(FBE_JOB_SERVICE_ERROR_SDCB_START_DB_TRASACTION_FAIL);
        CASENAME(FBE_JOB_SERVICE_ERROR_SDCB_COMMIT_DB_TRASACTION_FAIL);


        DFLTNAME(fbe_job_service_error_type_t, job_error_code);
    }
}

/*!***************************************************************
 * @fn fbe_api_common_send_control_packet_to_service_with_sg_list_asynch(fbe_packet_t *packet_p,
                                                                               fbe_payload_control_operation_opcode_t control_code,
                                                                               fbe_payload_control_buffer_t buffer,
                                                                               fbe_payload_control_buffer_length_t buffer_length,
                                                                               fbe_service_id_t service_id,
                                                                               fbe_sg_element_t *sg_list,
                                                                               fbe_u32_t sg_list_count,
                                                                               fbe_packet_completion_function_t completion_function,
                                                                               fbe_packet_completion_context_t completion_context,
                                                                               fbe_package_id_t package_id)
 *****************************************************************
 * @brief
 *   This function sends the control packet to the service with
 *   sg list
 *
 * @param control_code - control code
 * @param buffer - buffer
 * @param buffer_length - buffer length
 * @param service_id - service ID
 * @param sg_list - sg list
 * @param sg_list_count - sg list count
 * @param completion_function - function to call on completion
 * @param completion_context - completion context
 * @param package_id - packet ID
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 ****************************************************************/
fbe_status_t fbe_api_common_send_control_packet_to_service_with_sg_list_asynch(fbe_packet_t *packet,
                                                                               fbe_payload_control_operation_opcode_t control_code,
                                                                               fbe_payload_control_buffer_t buffer,
                                                                               fbe_payload_control_buffer_length_t buffer_length,
                                                                               fbe_service_id_t service_id,
                                                                               fbe_sg_element_t *sg_list,
                                                                               fbe_u32_t sg_list_count,
                                                                               fbe_packet_completion_function_t completion_function,
                                                                               fbe_packet_completion_context_t completion_context,
                                                                               fbe_package_id_t package_id)
{
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_ex_t *                  payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;

    payload = fbe_transport_get_payload_ex (packet);
    control_operation = fbe_payload_ex_allocate_control_operation(payload);
    /*set sg list*/
    fbe_payload_ex_set_sg_list (payload, sg_list, sg_list_count);

    fbe_payload_control_build_operation (control_operation,
                                         control_code,
                                         buffer,
                                         buffer_length);


    /* Set packet address.*/
    fbe_transport_set_address(packet,
                              package_id,
                              service_id,
                              FBE_CLASS_ID_INVALID,
                              FBE_OBJECT_ID_INVALID); 

    /* Mark packet with the attribute, either traverse or not */
    fbe_transport_set_packet_attr(packet, FBE_PACKET_FLAG_ASYNC);

    fbe_transport_set_completion_function(packet, completion_function, completion_context);

    status = fbe_api_common_send_control_packet_to_driver(packet);

    if (status != FBE_STATUS_OK && status != FBE_STATUS_PENDING && status != FBE_STATUS_BUSY){
        /* ESP tries to read from persist service and SEP unload will cause a failure.*/
         fbe_api_trace(FBE_TRACE_LEVEL_WARNING, "%s error in sending packet, status:%d\n", __FUNCTION__, status);  
    }
    
    return status;

}


fbe_queue_head_t * fbe_api_common_get_contiguous_packet_q_head()
{
    return &contiguous_packet_q_head;

}

fbe_spinlock_t * fbe_api_common_get_contiguous_packet_q_lock()
{
    return &contiguous_packet_q_lock;

}

fbe_queue_head_t * fbe_api_common_get_outstanding_contiguous_packet_q_head()
{
    return &outstanding_contiguous_packet_q_head;

}

fbe_status_t FBE_API_CALL fbe_api_common_package_entry_initialized (fbe_package_id_t package_id, fbe_bool_t *initialized)
{
    if (initialized == NULL) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: NULL pointer\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (package_id >= FBE_PACKAGE_ID_LAST ) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: invalid index for package id\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (fbe_api_libs_function_entries[package_id].magic_number == IO_AND_CONTROL_ENTRY_MAGIC_NUMBER) {
        *initialized = FBE_TRUE;
    }else{
        *initialized = FBE_FALSE;
    }

    return FBE_STATUS_OK;
}

/*!****************************************************************************
 * fbe_api_lifecycle_set_debug_trace_flag()
 ******************************************************************************
 * @brief
 *   This function is used to set the lifecycle debug trace flag.
 *
 * @param fbe_lifecycle_state_debug_tracing_flags_t 
 *
 * @return  fbe_status_t  
 *
 * @author
 *   02/06/2012 - Created. Omer Miranda
 *
 ******************************************************************************/
fbe_status_t FBE_API_CALL fbe_api_lifecycle_set_debug_trace_flag(fbe_api_lifecycle_debug_trace_control_t* p_trace_control, fbe_package_id_t package_id)
{
    fbe_lifecycle_debug_trace_control_t trace_control;
    fbe_api_control_operation_status_info_t status_info;
    fbe_status_t status;

    if (p_trace_control == NULL) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: NULL control pointer\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if ((p_trace_control->trace_flag == FBE_LIFECYCLE_DEBUG_FLAG_NONE) ||
        (p_trace_control->trace_flag >= FBE_LIFECYCLE_DEBUG_FLAG_LAST)) 
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, "%s: invalid trace flag: 0x%X\n",
                      __FUNCTION__, p_trace_control->trace_flag); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    trace_control.trace_flag = p_trace_control->trace_flag;

    status = fbe_api_common_send_control_packet_to_service(FBE_TRACE_CONTROL_CODE_SET_LIFECYCLE_DEBUG_TRACE_FLAG,
                                                           &trace_control,
                                                           sizeof(fbe_lifecycle_debug_trace_control_t),
                                                           FBE_SERVICE_ID_TRACE,
                                                           FBE_PACKET_FLAG_NO_ATTRIB,
                                                           &status_info,
                                                           package_id);

    if (status != FBE_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: can't send control packet\n", __FUNCTION__); 
        return status;
    }

    if (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) 
    {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: set request failed\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;
}
/***************************************************************
 * end fbe_api_lifecycle_set_debug_trace_flag()
 ***************************************************************/


