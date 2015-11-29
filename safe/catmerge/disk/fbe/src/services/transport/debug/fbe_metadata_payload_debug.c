/***************************************************************************
 * Copyright (C) EMC Corporation 2010-2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_metadata_debug.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the debug functions for the metadata payload.
 *
 * @author
 *  02/15/2011 - Created. Omer Miranda
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_dbgext.h"
#include "fbe_base_object.h"
#include "fbe_base_object_debug.h"
#include "fbe_base_object_trace.h"
#include "fbe_transport_debug.h"
#include "fbe_metadata.h"
#include "fbe_block_transport.h"
#include "fbe_transport_trace.h"
#include "fbe_block_transport_trace.h"
#include "fbe/fbe_payload_metadata_operation.h"
#include "fbe_metadata_debug.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/
static fbe_status_t fbe_transport_print_metadata_operation_opcode(const fbe_u8_t * module_name,
                                                                  fbe_dbgext_ptr base_ptr,
                                                                  fbe_trace_func_t trace_func,
                                                                  fbe_trace_context_t trace_context,
                                                                  fbe_debug_field_info_t *field_info_p,
                                                                  fbe_u32_t spaces_to_indent);

/*!*******************************************************************
 * @var fbe_metadata_operation_debug_field_info
 *********************************************************************
 * @brief Information about each of the fields of metadata payload.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_metadata_operation_debug_field_info[] =
{
    /* Object id is first.
     */
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("operation_header", fbe_payload_operation_header_t, FBE_TRUE, "0x%x",
                                    fbe_debug_display_field_pointer),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("opcode", fbe_payload_metadata_operation_opcode_t, FBE_FALSE, "0x%x",
                                    fbe_transport_print_metadata_operation_opcode),
    FBE_DEBUG_DECLARE_FIELD_INFO("metadata_status", fbe_payload_metadata_status_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("metadata_element", fbe_packet_t, FBE_FALSE, "0x%x", 
                                    fbe_debug_display_pointer),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("u", fbe_packet_t, FBE_FALSE, "0x%x", 
                                    fbe_payload_metadata_operation_union_debug),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_metadata_operation_debug_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        metadata operation structure.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_payload_metadata_operation_t,
                              fbe_metadata_operation_debug_struct_info,
                              &fbe_metadata_operation_debug_field_info[0]);


/*!**************************************************************
 * fbe_transport_print_metadata_payload()
 ****************************************************************
 * @brief
 *  Display all the trace information related to metadata operation
 *
 * @param  metadata_operation_p - Ptr to block operation payload.
 * @param trace_func - Function to use when tracing.
 * @param trace_context - The context to be passed to the trace function.
 *
 * @return - FBE_STATUS_OK  
 *
 * @author
 *  02/18/2011 - Created. Omer Miranda
 *
 ****************************************************************/
fbe_status_t fbe_transport_print_metadata_payload(const fbe_u8_t * module_name,
                                                  fbe_u64_t metadata_operation_p,
                                                  fbe_trace_func_t trace_func,
                                                  fbe_trace_context_t trace_context,
                                                  fbe_u32_t spaces_to_indent)
{
    fbe_status_t status;
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    status = fbe_debug_display_structure(module_name, trace_func, trace_context, metadata_operation_p,
                                         &fbe_metadata_operation_debug_struct_info,
                                         4    /* fields per line */,
                                         spaces_to_indent);
    trace_func(trace_context, "\n");
    return status;
}
/*************************************************
 * end fbe_transport_print_metadata_payload()
 *************************************************/

/*!**************************************************************
 * fbe_transport_print_metadata_operation_opcode()
 ****************************************************************
 * @brief
 *  Display the metadata operation opcode string.
 *
 * @param module_name - Module name to use in symbol lookup.
 * @param opcode - value to display.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param spaces_to_indent - Number of spaces to indent the next line(s).
 * 
 * @return fbe_status_t   
 *
 * @author
 *  02/15/2011 - Created. Omer Miranda
 *
 ****************************************************************/ 
static fbe_status_t fbe_transport_print_metadata_operation_opcode(const fbe_u8_t * module_name,
                                                                  fbe_dbgext_ptr base_ptr,
                                                                  fbe_trace_func_t trace_func,
                                                                  fbe_trace_context_t trace_context,
                                                                  fbe_debug_field_info_t *field_info_p,
                                                                  fbe_u32_t spaces_to_indent)
{
    fbe_payload_metadata_operation_opcode_t opcode;
    trace_func(trace_context, "%s: ", field_info_p->name);

    FBE_READ_MEMORY(base_ptr + field_info_p->offset, &opcode, field_info_p->size);

    switch(opcode)
    {
        case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_INVALID:
            trace_func(trace_context, "(invalid)");
            break;
        case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_INIT_MEMORY:
            trace_func(trace_context, "(init memory)");
            break;
        case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_NONPAGED_INIT:
            trace_func(trace_context, "(nonpaged init)");
            break;
        case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_NONPAGED_WRITE:
            trace_func(trace_context, "(nonpaged write)");
            break;
        case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_NONPAGED_SET_BITS:
            trace_func(trace_context, "(nonpaged set bits)");
            break;
        case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_NONPAGED_CLEAR_BITS:
            trace_func(trace_context, "(nonpaged clear bits)");
            break;
        case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_NONPAGED_POST_PERSIST:
            trace_func(trace_context, "(nonpaged post persist)");
            break;
        case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_WRITE:
            trace_func(trace_context, "(paged write)");
            break;
        case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_WRITE_VERIFY:
            trace_func(trace_context, "(paged write verify)");
            break;
        case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_VERIFY_WRITE:
            trace_func(trace_context, "(paged verify write)");
            break;
        case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_SET_BITS:
            trace_func(trace_context, "(paged set bits)");
            break;
        case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_CLEAR_BITS:
            trace_func(trace_context, "(paged clear bits)");
            break;
        case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_XOR_BITS:
            trace_func(trace_context, "(paged xor bits)");
            break;
        case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_GET_BITS:
            trace_func(trace_context, "(paged get bits)");
            break;
        case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_GET_NEXT_MARKED_BITS:
            trace_func(trace_context, "(paged get next marked bits)");
            break;
        case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_READ:
            trace_func(trace_context, "(paged read)");
            break;
        case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_UPDATE:
            trace_func(trace_context, "(paged update)");
            break;
        case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_WRITE_VERIFY_UPDATE:
            trace_func(trace_context, "(paged wr-verify update)");
            break;
        case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_UNREGISTER_ELEMENT:
            trace_func(trace_context, "(unregister element)");
            break;
        case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_MEMORY_UPDATE:
            trace_func(trace_context, "(memeory update)");
            break;
        default:
            trace_func(trace_context, "UNKNOWN opcode 0x%x",opcode);
            break;
    }
    return FBE_STATUS_OK;
}
/*************************************************
 * end fbe_transport_print_metadata_operation_opcode()
 *************************************************/

/*!**************************************************************
 * fbe_payload_metadata_operation_union_debug()
 ****************************************************************
 * @brief
 *  Display the metadata operation union 
 *
 * @param module_name - Module name to use in symbol lookup.
 * @param opcode - value to display.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param spaces_to_indent - Number of spaces to indent the next line(s).
 * 
 * @return fbe_status_t   
 *
 * @author
 *  02/25/2011 - Created. Omer Miranda
 *
 ****************************************************************/ 
fbe_status_t fbe_payload_metadata_operation_union_debug(const fbe_u8_t * module_name,
                                                               fbe_dbgext_ptr base_ptr,
                                                               fbe_trace_func_t trace_func,
                                                               fbe_trace_context_t trace_context,
                                                               fbe_debug_field_info_t *field_info_p,
                                                               fbe_u32_t spaces_to_indent)
{
    fbe_payload_metadata_operation_opcode_t opcode;
    fbe_u32_t opcode_offset;
    fbe_status_t status = FBE_STATUS_OK;

    FBE_GET_FIELD_OFFSET(module_name, fbe_payload_metadata_operation_t, "opcode", &opcode_offset);
    FBE_READ_MEMORY(base_ptr + opcode_offset, &opcode, sizeof(fbe_payload_metadata_operation_opcode_t));

    switch(opcode)
    {
        case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_NONPAGED_INIT: 
        case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_NONPAGED_WRITE:
        case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_NONPAGED_SET_BITS:
        case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_NONPAGED_CLEAR_BITS:
        case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_NONPAGED_PERSIST:
        case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_NONPAGED_POST_PERSIST:
        case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_WRITE:
        case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_WRITE_VERIFY:
        case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_VERIFY_WRITE:
        case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_SET_BITS:
        case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_CLEAR_BITS:
        case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_XOR_BITS:
        case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_GET_BITS:
        case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_GET_NEXT_MARKED_BITS:
        case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_READ:
        case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_UPDATE:
        case FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_WRITE_VERIFY_UPDATE:
            /* Get the metadata operation metadata */
            trace_func(trace_context, "metadata operation metadata:\n");
            status = fbe_metadata_operation_metadata_debug_trace(module_name,
                                                                base_ptr + field_info_p->offset,
                                                                trace_func,
                                                                trace_context,
                                                                spaces_to_indent);
            break;
    }

    return status;
}


/*************************
 * end file fbe_metadata_debug.c
 *************************/
