/***************************************************************************
 * Copyright (C) EMC Corporation 2014
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_memory_payload_debug.c
 ****************************************************************************
 *
 * @brief
 *  This file contains the debug functions for memory payload.
 *
 * @author
 *  4/1/2014 - Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_types.h"
#include "fbe/fbe_dbgext.h"
#include "fbe/fbe_transport.h"
#include "fbe_transport_debug.h"
#include "fbe_transport_trace.h"
#include "fbe/fbe_payload_stripe_lock_operation.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

static fbe_status_t fbe_memory_io_master_display(const fbe_u8_t * module_name,
                                                fbe_dbgext_ptr base_ptr,
                                                fbe_trace_func_t trace_func,
                                                fbe_trace_context_t trace_context,
                                                fbe_debug_field_info_t *field_info_p,
                                                fbe_u32_t spaces_to_indent);
/*!*******************************************************************
 * @var fbe_memory_io_master_debug_field_info
 *********************************************************************
 * @brief Information about each of the fields of sl payload.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_memory_io_master_debug_field_info[] =
{
    /* Object id is first.
     */
    FBE_DEBUG_DECLARE_FIELD_INFO("priority", fbe_memory_request_priority_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("flags", fbe_memory_request_priority_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("client_reject_allowed_flags", fbe_u8_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("arrival_reject_allowed_flags", fbe_u8_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_memory_io_master_debug_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        memory_io_master struct.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_memory_io_master_t,
                              fbe_memory_io_master_debug_struct_info,
                              &fbe_memory_io_master_debug_field_info[0]);

/*!*******************************************************************
 * @var fbe_memory_operation_debug_field_info
 *********************************************************************
 * @brief Information about each of the fields of sl payload.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_memory_operation_debug_field_info[] =
{
    /* Object id is first.
     */
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("operation_header", fbe_u32_t, FBE_TRUE, "0x%x",
                                    fbe_debug_display_field_pointer),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("memory_io_master", fbe_bool_t, FBE_FALSE, "0x%x",
                                    fbe_debug_display_field_pointer),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("memory_io_master", fbe_bool_t, FBE_FALSE, "0x%x",
                                    fbe_memory_io_master_display),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_memory_operation_debug_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        memory operation struct.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_payload_memory_operation_t,
                              fbe_memory_operation_debug_struct_info,
                              &fbe_memory_operation_debug_field_info[0]);

/*!**************************************************************
 * fbe_memory_io_master_display()
 ****************************************************************
 * @brief
 *  Display all the trace information related md cmi stripe lock.
 *
 * @param module_name - Module name to use in symbol lookup.
 * @param base_ptr - This is the ptr to the structure this field.
 *                   is located inside of.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function.
 * @param field_info_p - Info for getting and displaying the field. 
 * @param spaces_to_indent - Number of spaces to indent the next line(s).
 * @param  stripe_lock_opcode - block operation code.
 * @param trace_func - Function to use when tracing.
 * @param trace_context - The context to be passed to the trace function.
 *
 * @return - FBE_STATUS_OK  
 *
 * @author
 *  7/10/2012 - Created. Rob Foley
 *
 ****************************************************************/
static fbe_status_t fbe_memory_io_master_display(const fbe_u8_t * module_name,
                                                fbe_dbgext_ptr base_ptr,
                                                fbe_trace_func_t trace_func,
                                                fbe_trace_context_t trace_context,
                                                fbe_debug_field_info_t *field_info_p,
                                                fbe_u32_t spaces_to_indent)
{
    fbe_status_t status;
	fbe_trace_indent(trace_func, trace_context, 8 /*spaces_to_indent*/);
    status = fbe_debug_display_structure(module_name, trace_func, trace_context, base_ptr + field_info_p->offset,
                                         &fbe_memory_io_master_debug_struct_info,
                                         4    /* fields per line */,
                                         10);
    return status;
}
/********************************************
 * end fbe_memory_io_master_display()
 ********************************************/

/*!**************************************************************
 * fbe_transport_print_memory_payload()
 ****************************************************************
 * @brief
 *  Display all the trace information related to stripe lock operation
 *
 * @param  memory_operation_p - Ptr to block operation payload.
 * @param trace_func - Function to use when tracing.
 * @param trace_context - The context to be passed to the trace function.
 *
 * @return - FBE_STATUS_OK  
 *
 * @author
 *  6/24/2010 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_transport_print_memory_payload(const fbe_u8_t * module_name,
                                                     fbe_u64_t memory_operation_p,
                                                     fbe_trace_func_t trace_func,
                                                     fbe_trace_context_t trace_context,
                                                     fbe_u32_t spaces_to_indent)
{
    fbe_status_t status;
	fbe_trace_indent(trace_func, trace_context, 8 /*spaces_to_indent*/);
    status = fbe_debug_display_structure(module_name, trace_func, trace_context, memory_operation_p,
                                         &fbe_memory_operation_debug_struct_info,
                                         4    /* fields per line */,
                                         10);
    trace_func(trace_context, "\n");
    return status;
}
/*************************************************
 * end fbe_transport_print_memory_payload()
 *************************************************/

/*!*******************************************************************
 * @var fbe_buffer_operation_debug_field_info
 *********************************************************************
 * @brief Information about each of the fields of buffer payload.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_buffer_operation_debug_field_info[] =
{
    /* Object id is first.
     */
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("operation_header", fbe_u32_t, FBE_TRUE, "0x%x",
                                    fbe_debug_display_field_pointer),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("buffer", fbe_bool_t, FBE_FALSE, "0x%x",
                                    fbe_debug_display_field_pointer),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_buffer_operation_debug_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        memory operation struct.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_payload_buffer_operation_t,
                              fbe_buffer_operation_debug_struct_info,
                              &fbe_buffer_operation_debug_field_info[0]);

/*!**************************************************************
 * fbe_transport_print_buffer_payload()
 ****************************************************************
 * @brief
 *  Display all the trace information related to buffer operation
 *
 * @param buffer_operation_p - Ptr to block operation payload.
 * @param trace_func - Function to use when tracing.
 * @param trace_context - The context to be passed to the trace function.
 *
 * @return - FBE_STATUS_OK  
 *
 * @author
 *  03/04/2015  Ron Proulx  - Created.
 *
 ****************************************************************/
fbe_status_t fbe_transport_print_buffer_payload(const fbe_u8_t * module_name,
                                                fbe_u64_t buffer_operation_p,
                                                fbe_trace_func_t trace_func,
                                                fbe_trace_context_t trace_context,
                                                fbe_u32_t spaces_to_indent)
{
    fbe_status_t status;
	fbe_trace_indent(trace_func, trace_context, 8 /*spaces_to_indent*/);
    status = fbe_debug_display_structure(module_name, trace_func, trace_context, buffer_operation_p,
                                         &fbe_buffer_operation_debug_struct_info,
                                         4    /* fields per line */,
                                         10);
    trace_func(trace_context, "\n");
    return status;
}
/*************************************************
 * end fbe_transport_print_buffer_payload()
 *************************************************/

/*************************
 * end file fbe_memory_payload_debug.c.c
 *************************/
