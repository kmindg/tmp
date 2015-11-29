/***************************************************************************
 * Copyright (C) EMC Corporation 2008-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_CDB_transport_debug.c
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
#include "fbe/fbe_payload_cdb_operation.h"
#include "fbe/fbe_port.h"
#include "fbe_base_object_trace.h"
#include "fbe_logical_drive_debug.h"
#include "fbe_transport_debug.h"



fbe_status_t fbe_transport_print_payload_cdb_scsi_status(fbe_payload_cdb_scsi_status_t cdb_scsi_status,
                                              fbe_trace_func_t trace_func,
                                              fbe_trace_context_t trace_context);


/*!**************************************************************
 * fbe_transport_print_fbe_packet_cdb_payload()
 ****************************************************************
 * @brief
 *  Display all the trace information related to CDB operation
 *
 * @param  cdb_operation_p - Ptr to CDB operation payload.
 * @param trace_func - Function to use when tracing.
 * @param trace_context - The context to be passed to the trace function.
 *
 * @return - FBE_STATUS_OK  
 *
 * HISTORY:
 *
 ****************************************************************/

#define FBE_CDB_TRANSPORT_DEBUG_LOCAL_BUFFER_SIZE 60
fbe_status_t fbe_tansport_print_fbe_packet_cdb_payload(const fbe_u8_t * module_name,
                                                         fbe_u64_t cdb_operation_p,
                                                         fbe_trace_func_t trace_func,
                                                         fbe_trace_context_t trace_context,
                                                         fbe_u32_t spaces_to_indent)
{    
	fbe_u8_t						    cdb_length;
    fbe_payload_cdb_task_attribute_t    payload_cdb_task_attribute;
    fbe_payload_cdb_scsi_status_t       payload_cdb_scsi_status; 
    fbe_port_request_status_t           port_request_status; 
    fbe_payload_cdb_flags_t             payload_cdb_flags;      
    fbe_time_t                          timeout;
    fbe_payload_sg_descriptor_t         payload_sg_descriptor;
    fbe_u32_t                           transfer_count,loop_index;
    fbe_u64_t                           time_stamp;
	fbe_u8_t                            local_buffer[FBE_CDB_TRANSPORT_DEBUG_LOCAL_BUFFER_SIZE];


    /* Display the CDB payload detail.
     */

    FBE_GET_FIELD_DATA(module_name, 
                       cdb_operation_p,
                       fbe_payload_cdb_operation_t,
                       cdb_length,
                       sizeof(fbe_u8_t),
                       &cdb_length);
    FBE_GET_FIELD_DATA(module_name, 
                       cdb_operation_p,
                       fbe_payload_cdb_operation_t,
                       payload_cdb_task_attribute,
                       sizeof(fbe_payload_cdb_task_attribute_t),
                       &payload_cdb_task_attribute);
	FBE_GET_FIELD_DATA(module_name, 
                    cdb_operation_p,
                    fbe_payload_cdb_operation_t,
                    payload_cdb_scsi_status,
                    sizeof(fbe_payload_cdb_scsi_status_t),
                    &payload_cdb_scsi_status);
    FBE_GET_FIELD_DATA(module_name, 
                       cdb_operation_p,
                       fbe_payload_cdb_operation_t,
                       port_request_status,
                       sizeof(fbe_port_request_status_t),
                       &port_request_status);
    FBE_GET_FIELD_DATA(module_name, 
                       cdb_operation_p,
                       fbe_payload_cdb_operation_t,
                       payload_cdb_flags,
                       sizeof(fbe_payload_cdb_flags_t),
                       &payload_cdb_flags);
    FBE_GET_FIELD_DATA(module_name, 
                       cdb_operation_p,
                       fbe_payload_cdb_operation_t,
                       timeout,
                       sizeof(fbe_time_t),
                       &timeout);
    FBE_GET_FIELD_DATA(module_name,
                       cdb_operation_p,
                       fbe_payload_cdb_operation_t,
                       payload_sg_descriptor,
                       sizeof(fbe_payload_sg_descriptor_t),
                       &payload_sg_descriptor);   
    FBE_GET_FIELD_DATA(module_name, 
                       cdb_operation_p,
                       fbe_payload_cdb_operation_t,
                       transfer_count,
                       sizeof(fbe_u32_t),
                       &transfer_count);
    FBE_GET_FIELD_DATA(module_name, 
                       cdb_operation_p,
                       fbe_payload_cdb_operation_t,
                       time_stamp,
                       sizeof(fbe_u64_t),
                       &time_stamp);


    
    trace_func(trace_context, " cdb flags: 0x%x ", payload_cdb_flags);
	trace_func(trace_context, " task attributes: 0x%x ", payload_cdb_task_attribute);
	trace_func(trace_context, " transfer count: 0x%x \n", transfer_count);

	fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    trace_func(trace_context, " port status:  ");
	fbe_transport_print_port_request_status(port_request_status,trace_func,trace_context);
    trace_func(trace_context, " scsi status:  ");
	fbe_transport_print_payload_cdb_scsi_status(payload_cdb_scsi_status,trace_func,trace_context);
	trace_func(trace_context, "\n ");

	fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
	trace_func(trace_context, "cdb: ");
    FBE_GET_FIELD_DATA(module_name,
                       cdb_operation_p,
                       fbe_payload_cdb_operation_t,
                       cdb,
                       sizeof(fbe_u8_t) * FBE_PAYLOAD_CDB_CDB_SIZE,
                       local_buffer);

    for (loop_index = 0;
         loop_index < FBE_PAYLOAD_CDB_CDB_SIZE;
         loop_index++)
    {
        trace_func(trace_context, "%02d ", local_buffer[loop_index]);
    }
    trace_func(trace_context, "\n");

    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
	trace_func(trace_context, "sense info: ");
    FBE_GET_FIELD_DATA(module_name,
                       cdb_operation_p,
                       fbe_payload_cdb_operation_t,
                       sense_info_buffer,
                       sizeof(fbe_u8_t) * FBE_PAYLOAD_CDB_SENSE_INFO_BUFFER_SIZE,
                       local_buffer);

    for (loop_index = 0;
         loop_index < FBE_PAYLOAD_CDB_SENSE_INFO_BUFFER_SIZE;
         loop_index++)
    {
        trace_func(trace_context, "%02d ", local_buffer[loop_index]);
    }
    trace_func(trace_context, "\n ");

    return FBE_STATUS_OK;
}
/*************************************************
 * end fbe_tansport_print_fbe_packet_cdb_payload()
 *************************************************/


/*!**************************************************************
 * fbe_transport_print_port_request_status()
 ****************************************************************
 * @brief
 *  Display port status as a string.
 *
 * @param  port_status - Port status.
 * @param trace_func - Function to use when tracing.
 * @param trace_context - The context to be passed to the trace function.
 *
 * @return - FBE_STATUS_OK  
 *
 * HISTORY: 
 *
 ****************************************************************/
fbe_status_t fbe_transport_print_port_request_status(fbe_u32_t  port_request_status,
                                              fbe_trace_func_t trace_func,
                                              fbe_trace_context_t trace_context)
{
    switch(port_request_status)
    {
        case FBE_PORT_REQUEST_STATUS_SUCCESS:
          trace_func(trace_context, "FBE_PORT_REQUEST_STATUS_SUCCESS ");
          break;
        case FBE_PORT_REQUEST_STATUS_INVALID_REQUEST:
          trace_func(trace_context, "FBE_PORT_REQUEST_STATUS_INVALID_REQUEST ");
          break;
        case FBE_PORT_REQUEST_STATUS_INSUFFICIENT_RESOURCES:
          trace_func(trace_context, "FBE_PORT_REQUEST_STATUS_INSUFFICIENT_RESOURCES ");
          break;
        case FBE_PORT_REQUEST_STATUS_DEVICE_NOT_LOGGED_IN:
          trace_func(trace_context, "FBE_PORT_REQUEST_STATUS_DEVICE_NOT_LOGGED_IN ");
          break;
        case FBE_PORT_REQUEST_STATUS_BUSY:
          trace_func(trace_context, "FBE_PORT_REQUEST_STATUS_BUSY ");
          break;
        case FBE_PORT_REQUEST_STATUS_PROTOCOL_ERROR:
          trace_func(trace_context, "FBE_PORT_REQUEST_STATUS_PROTOCOL_ERROR ");
          break;
        case FBE_PORT_REQUEST_STATUS_ABORT_TIMEOUT:
          trace_func(trace_context, "FBE_PORT_REQUEST_STATUS_ABORT_TIMEOUT ");
          break;
        case FBE_PORT_REQUEST_STATUS_SELECTION_TIMEOUT:
          trace_func(trace_context, "FBE_PORT_REQUEST_STATUS_SELECTION_TIMEOUT ");
          break;
        case FBE_PORT_REQUEST_STATUS_TRANSIENT_ERROR:
          trace_func(trace_context, "FBE_PORT_REQUEST_STATUS_TRANSIENT_ERROR ");
          break;
        case FBE_PORT_REQUEST_STATUS_DATA_OVERRUN:
          trace_func(trace_context, "FBE_PORT_REQUEST_STATUS_DATA_OVERRUN ");
          break;
        case FBE_PORT_REQUEST_STATUS_DATA_UNDERRUN:
          trace_func(trace_context, "FBE_PORT_REQUEST_STATUS_DATA_UNDERRUN ");
          break;
        case FBE_PORT_REQUEST_STATUS_ABORTED_BY_SOFTWARE:
          trace_func(trace_context, "FBE_PORT_REQUEST_STATUS_ABORTED_BY_SOFTWARE ");
          break;
        case FBE_PORT_REQUEST_STATUS_ABORTED_BY_DEVICE:
          trace_func(trace_context, "FBE_PORT_REQUEST_STATUS_ABORTED_BY_DEVICE ");
          break;
        case FBE_PORT_REQUEST_STATUS_SATA_NCQ_ERROR:
          trace_func(trace_context, "FBE_PORT_REQUEST_STATUS_SATA_NCQ_ERROR ");
          break;
        case FBE_PORT_REQUEST_STATUS_REJECTED_NCQ_MODE:
          trace_func(trace_context, "FBE_PORT_REQUEST_STATUS_REJECTED_NCQ_MODE ");
          break;
        case FBE_PORT_REQUEST_STATUS_INCIDENTAL_ABORT:
          trace_func(trace_context, "FBE_PORT_REQUEST_STATUS_INCIDENTAL_ABORT ");
          break;
        case FBE_PORT_REQUEST_STATUS_ERROR:
          trace_func(trace_context, "FBE_PORT_REQUEST_STATUS_ERROR - Unexpected ");
          break;
        case FBE_PORT_REQUEST_STATUS_ENCRYPTION_NOT_ENABLED:
          trace_func(trace_context, "FBE_PORT_REQUEST_STATUS_ENCRYPTION_NOT_ENABLED ");
          break;
        case FBE_PORT_REQUEST_STATUS_ENCRYPTION_BAD_HANDLE:
          trace_func(trace_context, "FBE_PORT_REQUEST_STATUS_ENCRYPTION_BAD_HANDLE ");
          break;
        case FBE_PORT_REQUEST_STATUS_ENCRYPTION_KEY_WRAP_ERROR:
          trace_func(trace_context, "FBE_PORT_REQUEST_STATUS_ENCRYPTION_KEY_WRAP_ERROR ");
          break;
        default:
          trace_func(trace_context, "UNKNOWN Error 0x%x", port_request_status);
          break;
    }

    return FBE_STATUS_OK;
}
/********************************************
 * end fbe_transport_print_port_request_status()
 ********************************************/

/*!**************************************************************
 * fbe_transport_print_payload_cdb_scsi_status()
 ****************************************************************
 * @brief
 *  Display port status as a string.
 *
 * @param  port_status - Port status.
 * @param trace_func - Function to use when tracing.
 * @param trace_context - The context to be passed to the trace function.
 *
 * @return - FBE_STATUS_OK  
 *
 * HISTORY: 
 *
 ****************************************************************/
fbe_status_t fbe_transport_print_payload_cdb_scsi_status(fbe_payload_cdb_scsi_status_t cdb_scsi_status,
                                              fbe_trace_func_t trace_func,
                                              fbe_trace_context_t trace_context)
{
    switch(cdb_scsi_status)
    {
        case FBE_PAYLOAD_CDB_SCSI_STATUS_GOOD:
          trace_func(trace_context, "FBE_PAYLOAD_CDB_SCSI_STATUS_GOOD ");
          break;
        case FBE_PAYLOAD_CDB_SCSI_STATUS_CHECK_CONDITION:
          trace_func(trace_context, "FBE_PAYLOAD_CDB_SCSI_STATUS_CHECK_CONDITION ");
          break;
        case FBE_PAYLOAD_CDB_SCSI_STATUS_CONDITION_MET:
          trace_func(trace_context, "FBE_PAYLOAD_CDB_SCSI_STATUS_CONDITION_MET ");
          break;
        case FBE_PAYLOAD_CDB_SCSI_STATUS_BUSY:
          trace_func(trace_context, "FBE_PAYLOAD_CDB_SCSI_STATUS_BUSY ");
          break;
        case FBE_PAYLOAD_CDB_SCSI_STATUS_INTERMEDIATE:
          trace_func(trace_context, "FBE_PAYLOAD_CDB_SCSI_STATUS_INTERMEDIATE ");
          break;
        case FBE_PAYLOAD_CDB_SCSI_STATUS_INTERMEDIATE_CONDITION_MET:
          trace_func(trace_context, "FBE_PAYLOAD_CDB_SCSI_STATUS_INTERMEDIATE_CONDITION_MET ");
          break;
        case FBE_PAYLOAD_CDB_SCSI_STATUS_RESERVATION_CONFLICT:
          trace_func(trace_context, "FBE_PAYLOAD_CDB_SCSI_STATUS_RESERVATION_CONFLICT ");
          break;
        case FBE_PAYLOAD_CDB_SCSI_STATUS_OBSOLETE:
          trace_func(trace_context, "FBE_PAYLOAD_CDB_SCSI_STATUS_OBSOLETE ");
          break;
        case FBE_PAYLOAD_CDB_SCSI_STATUS_TASK_SET_FULL:
          trace_func(trace_context, "FBE_PAYLOAD_CDB_SCSI_STATUS_TASK_SET_FULL ");
          break;
        case FBE_PAYLOAD_CDB_SCSI_STATUS_ACA_ACTIVE:
          trace_func(trace_context, "FBE_PAYLOAD_CDB_SCSI_STATUS_ACA_ACTIVE ");
          break;
        case FBE_PAYLOAD_CDB_SCSI_STATUS_TASK_ABORTED:
          trace_func(trace_context, "FBE_PAYLOAD_CDB_SCSI_STATUS_TASK_ABORTED ");
          break;
        default:
          trace_func(trace_context, "UNKNOWN Error 0x%x", cdb_scsi_status);
          break;
    }

    return FBE_STATUS_OK;

}
/***************************************************
 * end fbe_transport_print_payload_cdb_scsi_status()
 **************************************************/

