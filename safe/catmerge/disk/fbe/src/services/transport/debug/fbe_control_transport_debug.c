/***************************************************************************
 * Copyright (C) EMC Corporation 2008-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_control_transport_debug.c
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
#include "fbe_base_object_debug.h"
#include "fbe_base_transport.h"
#include "fbe/fbe_payload_control_operation.h"
#include "fbe_base_object_trace.h"
#include "fbe_transport_debug.h"
#include "fbe/fbe_payload_control_operation.h"
#include "fbe_block_transport.h"
#include "fbe_discovery_transport.h"
#include "fbe_smp_transport.h"
#include "fbe_ssp_transport.h"
#include "fbe_stp_transport.h"



fbe_status_t fbe_transport_print_control_payload_status(fbe_u32_t  control_payload_status,
                                              fbe_trace_func_t trace_func,
                                              fbe_trace_context_t trace_context);

fbe_status_t fbe_transport_print_control_opcode(fbe_u32_t  control_opcode,
                                              fbe_trace_func_t trace_func,
                                              fbe_trace_context_t trace_context);

/*!**************************************************************
 * fbe_transport_print_fbe_packet_control_payload()
 ****************************************************************
 * @brief
 *  Display all the trace information related to CDB operation
 *
 * @param  control_operation_p - Ptr to CDB operation payload.
 * @param trace_func - Function to use when tracing.
 * @param trace_context - The context to be passed to the trace function.
 *
 * @return - FBE_STATUS_OK  
 *
 * HISTORY:
 *
 ****************************************************************/
fbe_status_t fbe_tansport_print_fbe_packet_control_payload(const fbe_u8_t * module_name,
                                                         fbe_u64_t control_operation_p,
                                                         fbe_trace_func_t trace_func,
                                                         fbe_trace_context_t trace_context,
                                                         fbe_u32_t spaces_to_indent)
{    
	fbe_payload_control_operation_opcode_t	opcode;
	fbe_payload_control_buffer_length_t		buffer_length; 
	fbe_u64_t			buffer = 0;
	fbe_payload_control_status_t			status;
	fbe_payload_control_status_qualifier_t  status_qualifier;
	fbe_u32_t ptr_size;
    fbe_u32_t offset;
    fbe_debug_get_ptr_size(module_name, &ptr_size);
/*
	#define FBE_TRANSPORT_CONTROL_CODE_INVALID_DEF(_transport_id) \
	((FBE_PAYLOAD_CONTROL_CODE_TYPE_TRANSPORT << FBE_PAYLOAD_CONTROL_CODE_TYPE_SHIFT) | (_transport_id << FBE_PAYLOAD_CONTROL_CODE_ID_SHIFT))
*/

    /* Display the payload detail.
     */

    FBE_GET_FIELD_DATA(module_name, 
                       control_operation_p,
                       fbe_payload_control_operation_t,
                       opcode,
                       sizeof(fbe_payload_control_operation_opcode_t),
                       &opcode);
    FBE_GET_FIELD_DATA(module_name, 
                       control_operation_p,
                       fbe_payload_control_operation_t,
                       buffer_length,
                       sizeof(fbe_payload_control_buffer_length_t),
                       &buffer_length);

    FBE_GET_FIELD_OFFSET(module_name, fbe_payload_control_operation_t, "buffer", &offset);
    FBE_READ_MEMORY(control_operation_p + offset, &buffer, ptr_size);
    FBE_GET_FIELD_DATA(module_name, 
                       control_operation_p,
                       fbe_payload_control_operation_t,
                       status,
                       sizeof(fbe_payload_control_status_t),
                       &status);
    FBE_GET_FIELD_DATA(module_name, 
                       control_operation_p,
                       fbe_payload_control_operation_t,
                       status_qualifier,
                       sizeof(fbe_payload_control_status_qualifier_t),
                       &status_qualifier);

    
    trace_func(trace_context, " Opcode : ");
	fbe_transport_print_control_opcode(opcode,trace_func,trace_context);
	trace_func(trace_context, " \n");

	fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
	trace_func(trace_context, " Buffer : 0x%llx ",
		   (unsigned long long)buffer);
	trace_func(trace_context, " Buffer Length : 0x%x", buffer_length);
	trace_func(trace_context, " \n");

	fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    trace_func(trace_context, " Status :  ");
	fbe_transport_print_control_payload_status(status,trace_func,trace_context);	
    trace_func(trace_context, " Status Qualifier:  0x%x",status_qualifier);	
	trace_func(trace_context, "\n ");

    return FBE_STATUS_OK;
}
/*************************************************
 * end fbe_tansport_print_fbe_packet_control_payload()
 *************************************************/


/*!**************************************************************
 * fbe_transport_print_control_payload_status()
 ****************************************************************
 * @brief
 *  Display control payload status as a string.
 *
 * @param  control_payload_status - control payload status.
 * @param trace_func - Function to use when tracing.
 * @param trace_context - The context to be passed to the trace function.
 *
 * @return - FBE_STATUS_OK  
 *
 * HISTORY: 
 *
 ****************************************************************/
fbe_status_t fbe_transport_print_control_payload_status(fbe_u32_t  control_payload_status,
                                              fbe_trace_func_t trace_func,
                                              fbe_trace_context_t trace_context)
{
    switch(control_payload_status)
    {
        case FBE_PAYLOAD_CONTROL_STATUS_OK:
          trace_func(trace_context, "FBE_PAYLOAD_CONTROL_STATUS_OK ");
          break;
        case FBE_PAYLOAD_CONTROL_STATUS_FAILURE:
          trace_func(trace_context, "FBE_PAYLOAD_CONTROL_STATUS_FAILURE ");
          break;
        default:
          trace_func(trace_context, "UNKNOWN Error 0x%x", control_payload_status);
          break;
    }

    return FBE_STATUS_OK;
}
/********************************************
 * end fbe_transport_print_control_payload_status()
 ********************************************/


/*!**************************************************************
 * fbe_transport_print_control_opcode()
 ****************************************************************
 * @brief
 *  Display Control operation opcode as a string.
 *
 * @param  control_opcode - Control operation opcode.
 * @param trace_func - Function to use when tracing.
 * @param trace_context - The context to be passed to the trace function.
 *
 * @return - FBE_STATUS_OK  
 *
 * HISTORY: 
 *
 ****************************************************************/
fbe_status_t fbe_transport_print_control_opcode(fbe_u32_t  control_opcode,
                                              fbe_trace_func_t trace_func,
                                              fbe_trace_context_t trace_context)
{
    switch(control_opcode)
    {
        case FBE_BLOCK_TRANSPORT_CONTROL_CODE_ATTACH_EDGE:
          trace_func(trace_context, "FBE_BLOCK_TRANSPORT_CONTROL_CODE_ATTACH_EDGE ");
          break;
        case FBE_BLOCK_TRANSPORT_CONTROL_CODE_DETACH_EDGE:
          trace_func(trace_context, "FBE_BLOCK_TRANSPORT_CONTROL_CODE_DETACH_EDGE ");
          break;
        case FBE_BLOCK_TRANSPORT_CONTROL_CODE_OPEN_EDGE:
          trace_func(trace_context, "FBE_BLOCK_TRANSPORT_CONTROL_CODE_OPEN_EDGE ");
          break;
        case FBE_BLOCK_TRANSPORT_CONTROL_CODE_GET_PATH_STATE:
          trace_func(trace_context, "FBE_BLOCK_TRANSPORT_CONTROL_CODE_GET_PATH_STATE ");
          break;
        case FBE_BLOCK_TRANSPORT_CONTROL_CODE_GET_EDGE_INFO:
          trace_func(trace_context, "FBE_BLOCK_TRANSPORT_CONTROL_CODE_GET_EDGE_INFO ");
          break;
        case FBE_DISCOVERY_TRANSPORT_CONTROL_CODE_CREATE_EDGE:
          trace_func(trace_context, "FBE_DISCOVERY_TRANSPORT_CONTROL_CODE_CREATE_EDGE ");
          break;
        case FBE_DISCOVERY_TRANSPORT_CONTROL_CODE_ATTACH_EDGE:
          trace_func(trace_context, "FBE_DISCOVERY_TRANSPORT_CONTROL_CODE_ATTACH_EDGE ");
          break;
        case FBE_DISCOVERY_TRANSPORT_CONTROL_CODE_DETACH_EDGE:
          trace_func(trace_context, "FBE_DISCOVERY_TRANSPORT_CONTROL_CODE_DETACH_EDGE ");
          break;
        case FBE_DISCOVERY_TRANSPORT_CONTROL_CODE_GET_EDGE_INFO:
          trace_func(trace_context, "FBE_DISCOVERY_TRANSPORT_CONTROL_CODE_GET_EDGE_INFO ");
          break;
        case FBE_SMP_TRANSPORT_CONTROL_CODE_ATTACH_EDGE:
          trace_func(trace_context, "FBE_SMP_TRANSPORT_CONTROL_CODE_ATTACH_EDGE ");
          break;
        case FBE_SMP_TRANSPORT_CONTROL_CODE_DETACH_EDGE:
          trace_func(trace_context, "FBE_SMP_TRANSPORT_CONTROL_CODE_DETACH_EDGE ");
          break;
        case FBE_SMP_TRANSPORT_CONTROL_CODE_OPEN_EDGE:
          trace_func(trace_context, "FBE_SMP_TRANSPORT_CONTROL_CODE_OPEN_EDGE ");
          break;
        case FBE_SMP_TRANSPORT_CONTROL_CODE_GET_PATH_STATE:
          trace_func(trace_context, "FBE_SMP_TRANSPORT_CONTROL_CODE_GET_PATH_STATE ");
          break;
        case FBE_SMP_TRANSPORT_CONTROL_CODE_GET_EDGE_INFO:
          trace_func(trace_context, "FBE_SMP_TRANSPORT_CONTROL_CODE_GET_EDGE_INFO ");
          break;
        case FBE_SSP_TRANSPORT_CONTROL_CODE_ATTACH_EDGE:
          trace_func(trace_context, "FBE_SSP_TRANSPORT_CONTROL_CODE_ATTACH_EDGE ");
          break;
        case FBE_SSP_TRANSPORT_CONTROL_CODE_DETACH_EDGE:
          trace_func(trace_context, "FBE_SSP_TRANSPORT_CONTROL_CODE_DETACH_EDGE ");
          break;
        case FBE_SSP_TRANSPORT_CONTROL_CODE_OPEN_EDGE:
          trace_func(trace_context, "FBE_SSP_TRANSPORT_CONTROL_CODE_OPEN_EDGE ");
          break;
        case FBE_SSP_TRANSPORT_CONTROL_CODE_GET_PATH_STATE:
          trace_func(trace_context, "FBE_SSP_TRANSPORT_CONTROL_CODE_GET_PATH_STATE ");
          break;
        case FBE_SSP_TRANSPORT_CONTROL_CODE_GET_EDGE_INFO:
          trace_func(trace_context, "FBE_SSP_TRANSPORT_CONTROL_CODE_GET_EDGE_INFO ");
          break;
        case FBE_SSP_TRANSPORT_CONTROL_CODE_RESET_DEVICE:
          trace_func(trace_context, "FBE_SSP_TRANSPORT_CONTROL_CODE_RESET_DEVICE ");
          break;
        case FBE_SSP_TRANSPORT_CONTROL_CODE_SET_EDGE_TAP_HOOK:
          trace_func(trace_context, "FBE_SSP_TRANSPORT_CONTROL_CODE_SET_EDGE_TAP_HOOK ");
          break;
        case FBE_STP_TRANSPORT_CONTROL_CODE_ATTACH_EDGE:
          trace_func(trace_context, "FBE_STP_TRANSPORT_CONTROL_CODE_ATTACH_EDGE ");
          break;
        case FBE_STP_TRANSPORT_CONTROL_CODE_DETACH_EDGE:
          trace_func(trace_context, "FBE_STP_TRANSPORT_CONTROL_CODE_DETACH_EDGE ");
          break;
        case FBE_STP_TRANSPORT_CONTROL_CODE_OPEN_EDGE:
          trace_func(trace_context, "FBE_STP_TRANSPORT_CONTROL_CODE_OPEN_EDGE ");
          break;
        case FBE_STP_TRANSPORT_CONTROL_CODE_GET_PATH_STATE:
          trace_func(trace_context, "FBE_STP_TRANSPORT_CONTROL_CODE_GET_PATH_STATE ");
          break;
        case FBE_STP_TRANSPORT_CONTROL_CODE_GET_EDGE_INFO:
          trace_func(trace_context, "FBE_STP_TRANSPORT_CONTROL_CODE_GET_EDGE_INFO ");
          break;
        case FBE_STP_TRANSPORT_CONTROL_CODE_SEND_DIAGNOSTIC:
          trace_func(trace_context, "FBE_STP_TRANSPORT_CONTROL_CODE_SEND_DIAGNOSTIC ");
          break;
        case FBE_STP_TRANSPORT_CONTROL_CODE_RESET_DEVICE:
          trace_func(trace_context, "FBE_STP_TRANSPORT_CONTROL_CODE_RESET_DEVICE ");
          break;
        case FBE_STP_TRANSPORT_CONTROL_CODE_SET_EDGE_TAP_HOOK:
          trace_func(trace_context, "FBE_STP_TRANSPORT_CONTROL_CODE_SET_EDGE_TAP_HOOK ");
          break;

        default:
          trace_func(trace_context, "UNKNOWN Control Code 0x%x", control_opcode);
          break;
    }

    return FBE_STATUS_OK;
}
/********************************************
 * end fbe_transport_print_control_opcode()
 ********************************************/

