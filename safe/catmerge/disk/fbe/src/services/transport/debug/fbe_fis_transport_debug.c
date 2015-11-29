/***************************************************************************
 * Copyright (C) EMC Corporation 2008-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_fis_transport_debug.c
 ****************************************************************************
 *
 * @brief
 *  This file contains the debug functions for Block Transport.
 *
 * @author
 * 
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_types.h"
#include "fbe/fbe_dbgext.h"
#include "fbe/fbe_payload_fis_operation.h"
#include "fbe/fbe_port.h"
#include "fbe_base_object_debug.h"
#include "fbe_base_object_trace.h"
#include "fbe_logical_drive_debug.h"
#include "fbe_transport_debug.h"


/*!**************************************************************
 * fbe_transport_print_fbe_packet_fis_payload()
 ****************************************************************
 * @brief
 *  Display all the trace information related to FIS operation
 *
 * @param  fis_operation_p - Ptr to FIS operation payload.
 * @param trace_func - Function to use when tracing.
 * @param trace_context - The context to be passed to the trace function.
 *
 * @return - FBE_STATUS_OK  
 *
 * HISTORY:
 *
 ****************************************************************/

#define FBE_FIS_TRANSPORT_DEBUG_LOCAL_BUFFER_SIZE 60
fbe_status_t fbe_tansport_print_fbe_packet_fis_payload(const fbe_u8_t * module_name,
                                                         fbe_u64_t fis_operation_p,
                                                         fbe_trace_func_t trace_func,
                                                         fbe_trace_context_t trace_context,
                                                         fbe_u32_t spaces_to_indent)
{    
	fbe_u8_t						    fis_length;
    fbe_payload_fis_task_attribute_t    payload_fis_task_attribute;    
    fbe_port_request_status_t           port_request_status; 
    fbe_payload_fis_flags_t             payload_fis_flags;      
    fbe_time_t                          timeout;    
    fbe_u32_t                           transfer_count,loop_index;    
    fbe_payload_sg_descriptor_t         payload_sg_descriptor;
	fbe_u8_t                            local_buffer[FBE_FIS_TRANSPORT_DEBUG_LOCAL_BUFFER_SIZE];


    /* Display the FIS payload detail.
     */

    FBE_GET_FIELD_DATA(module_name, 
                       fis_operation_p,
                       fbe_payload_fis_operation_t,
                       fis_length,
                       sizeof(fbe_u8_t),
                       &fis_length);
    FBE_GET_FIELD_DATA(module_name, 
                       fis_operation_p,
                       fbe_payload_fis_operation_t,
                       payload_fis_task_attribute,
                       sizeof(fbe_payload_fis_task_attribute_t),
                       &payload_fis_task_attribute);
    FBE_GET_FIELD_DATA(module_name, 
                       fis_operation_p,
                       fbe_payload_fis_operation_t,
                       port_request_status,
                       sizeof(fbe_port_request_status_t),
                       &port_request_status);
    FBE_GET_FIELD_DATA(module_name, 
                       fis_operation_p,
                       fbe_payload_fis_operation_t,
                       payload_fis_flags,
                       sizeof(fbe_payload_fis_flags_t),
                       &payload_fis_flags);
    FBE_GET_FIELD_DATA(module_name, 
                       fis_operation_p,
                       fbe_payload_fis_operation_t,
                       timeout,
                       sizeof(fbe_time_t),
                       &timeout);
    FBE_GET_FIELD_DATA(module_name,
                       fis_operation_p,
                       fbe_payload_fis_operation_t,
                       payload_sg_descriptor,
                       sizeof(fbe_payload_sg_descriptor_t),
                       &payload_sg_descriptor);   
    FBE_GET_FIELD_DATA(module_name, 
                       fis_operation_p,
                       fbe_payload_fis_operation_t,
                       transfer_count,
                       sizeof(fbe_u32_t),
                       &transfer_count);

    
    trace_func(trace_context, " fis flags: 0x%x ", payload_fis_flags);
	trace_func(trace_context, " task attributes: 0x%x ", payload_fis_task_attribute);
	trace_func(trace_context, " transfer count: 0x%x ", transfer_count);	
    trace_func(trace_context, " port status:  ");
	fbe_transport_print_port_request_status(port_request_status,trace_func,trace_context);
	trace_func(trace_context, "\n ");

	fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
	trace_func(trace_context, "fis: ");
    FBE_GET_FIELD_DATA(module_name,
                       fis_operation_p,
                       fbe_payload_fis_operation_t,
                       fis,
                       sizeof(fbe_u8_t) * FBE_PAYLOAD_FIS_SIZE,
                       local_buffer);

    for (loop_index = 0;
         loop_index < FBE_PAYLOAD_FIS_SIZE;
         loop_index++)
    {
        trace_func(trace_context, "%02d ", local_buffer[loop_index]);
    }
    trace_func(trace_context, "\n");

    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
	trace_func(trace_context, "response buffer: ");
    FBE_GET_FIELD_DATA(module_name,
                       fis_operation_p,
                       fbe_payload_fis_operation_t,
                       response_buffer,
                       sizeof(fbe_u8_t) * FBE_PAYLOAD_FIS_RESPONSE_BUFFER_SIZE,
                       local_buffer);

    for (loop_index = 0;
         loop_index < FBE_PAYLOAD_FIS_RESPONSE_BUFFER_SIZE;
         loop_index++)
    {
        trace_func(trace_context, "%02d ", local_buffer[loop_index]);
    }
    trace_func(trace_context, "\n ");

    return FBE_STATUS_OK;
}
/*************************************************
 * end fbe_tansport_print_fbe_packet_fis_payload()
 *************************************************/
