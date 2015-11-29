/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_stripe_lock_payload_debug.c.c
 ****************************************************************************
 *
 * @brief
 *  This file contains the debug functions for stripe lock payload.
 *
 * @author
 *  6/23/2010 - Created. Rob Foley
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
extern fbe_debug_queue_info_t fbe_stripe_lock_operation_queue_info; 
static fbe_status_t fbe_transport_print_stripe_lock_opcode(const fbe_u8_t * module_name,
                                                           fbe_dbgext_ptr base_ptr,
                                                           fbe_trace_func_t trace_func,
                                                           fbe_trace_context_t trace_context,
                                                           fbe_debug_field_info_t *field_info_p,
                                                           fbe_u32_t spaces_to_indent);

static fbe_status_t fbe_metadata_cmi_sl_display(const fbe_u8_t * module_name,
                                                fbe_dbgext_ptr base_ptr,
                                                fbe_trace_func_t trace_func,
                                                fbe_trace_context_t trace_context,
                                                fbe_debug_field_info_t *field_info_p,
                                                fbe_u32_t spaces_to_indent);


/*!*******************************************************************
 * @var fbe_metadata_cmi_message_header_debug_field_info
 *********************************************************************
 * @brief Information about each of the fields of sl cmi md request.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_metadata_cmi_message_header_debug_field_info[] =
{
    /* Object id is first.
     */
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("metadata_element_sender", fbe_u64_t, FBE_FALSE, "0x%x",
                                    fbe_debug_display_pointer),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_metadata_cmi_message_header_debug_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        sl cmi metadata struct.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_metadata_cmi_message_header_t,
                              fbe_metadata_cmi_message_header_debug_struct_info,
                              &fbe_metadata_cmi_message_header_debug_field_info[0]);

/*!*******************************************************************
 * @var fbe_metadata_stripe_lock_region_debug_field_info
 *********************************************************************
 * @brief Information about each of the fields of sl cmi md request.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_metadata_stripe_lock_region_debug_field_info[] =
{
    /* Object id is first.
     */
    FBE_DEBUG_DECLARE_FIELD_INFO("first", fbe_lba_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("last", fbe_lba_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_metadata_stripe_lock_region_debug_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        sl cmi metadata struct.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_metadata_stripe_lock_region_t,
                              fbe_metadata_stripe_lock_region_debug_struct_info,
                              &fbe_metadata_stripe_lock_region_debug_field_info[0]);

/*!*******************************************************************
 * @var fbe_metadata_cmi_stripe_lock_debug_field_info
 *********************************************************************
 * @brief Information about each of the fields of sl cmi md request.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_metadata_cmi_stripe_lock_debug_field_info[] =
{
    /* Object id is first.
     */
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("cmi", fbe_metadata_cmi_message_header_t, FBE_FALSE, "0x%x", 
                                    fbe_metadata_cmi_message_header_display),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("write_region", fbe_metadata_stripe_lock_region_t, FBE_FALSE, "0x%x",
                                    fbe_metadata_sl_region_display),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("read_region", fbe_metadata_stripe_lock_region_t, FBE_FALSE, "0x%x",
                                    fbe_metadata_sl_region_display),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("packet", fbe_packet_t, FBE_FALSE, "0x%x", 
                                    fbe_debug_display_pointer),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("request_sl_ptr", fbe_u64_t, FBE_FALSE, "0x%x", 
                                    fbe_debug_display_pointer),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("grant_sl_ptr", fbe_u64_t, FBE_FALSE, "0x%x", 
                                    fbe_debug_display_pointer),
    FBE_DEBUG_DECLARE_FIELD_INFO("flags", fbe_metadata_cmi_stripe_lock_flags_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_metadata_cmi_stripe_lock_debug_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        sl cmi metadata struct.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_metadata_cmi_stripe_lock_t,
                              fbe_metadata_cmi_stripe_lock_debug_struct_info,
                              &fbe_metadata_cmi_stripe_lock_debug_field_info[0]);

/*!*******************************************************************
 * @var fbe_stripe_lock_operation_debug_field_info
 *********************************************************************
 * @brief Information about each of the fields of sl payload.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_stripe_lock_operation_debug_field_info[] =
{
    /* Object id is first.
     */
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("operation_header", fbe_u32_t, FBE_TRUE, "0x%x",
                                    fbe_debug_display_field_pointer),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("opcode", fbe_bool_t, FBE_FALSE, "0x%x",
                                    fbe_transport_print_stripe_lock_opcode),
    FBE_DEBUG_DECLARE_FIELD_INFO("status", fbe_payload_stripe_lock_status_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("flags", fbe_payload_stripe_lock_flags_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("stripe", fbe_metadata_stripe_lock_region_t, FBE_FALSE, "0x%x",
                                    fbe_metadata_sl_region_display),
    FBE_DEBUG_DECLARE_FIELD_INFO("priv_flags", fbe_payload_stripe_lock_priv_flags_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("cmi_stripe_lock", fbe_metadata_cmi_stripe_lock_t, FBE_FALSE, "0x%x", 
                                    fbe_metadata_cmi_sl_display),
    FBE_DEBUG_DECLARE_FIELD_INFO_QUEUE("wait_queue", fbe_queue_head_t, FBE_FALSE, 
                                       &fbe_stripe_lock_operation_queue_info),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_stripe_lock_operation_debug_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        stripe lock operation struct.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_payload_stripe_lock_operation_t,
                              fbe_stripe_lock_operation_debug_struct_info,
                              &fbe_stripe_lock_operation_debug_field_info[0]);

/*!**************************************************************
 * fbe_transport_print_block_stripe_lock_opcode()
 ****************************************************************
 * @brief
 *  Display all the trace information related to stripe lock opcode.
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
 *  6/24/2010 - Created. Rob Foley
 *
 ****************************************************************/
static fbe_status_t fbe_transport_print_stripe_lock_opcode(const fbe_u8_t * module_name,
                                                           fbe_dbgext_ptr base_ptr,
                                                           fbe_trace_func_t trace_func,
                                                           fbe_trace_context_t trace_context,
                                                           fbe_debug_field_info_t *field_info_p,
                                                           fbe_u32_t spaces_to_indent)
{
    fbe_payload_stripe_lock_operation_opcode_t stripe_lock_opcode;

    trace_func(trace_context, "%s: ", field_info_p->name);

    FBE_READ_MEMORY(base_ptr + field_info_p->offset, &stripe_lock_opcode, field_info_p->size);

    switch(stripe_lock_opcode)
    {
        case FBE_PAYLOAD_STRIPE_LOCK_OPERATION_OPCODE_INVALID:
          trace_func(trace_context, "(invalid)");
          break;
        case FBE_PAYLOAD_STRIPE_LOCK_OPERATION_OPCODE_READ_LOCK:
          trace_func(trace_context, "(read lock)");
          break;
        case FBE_PAYLOAD_STRIPE_LOCK_OPERATION_OPCODE_READ_UNLOCK:
          trace_func(trace_context, "(read unlock)");
          break;
        case FBE_PAYLOAD_STRIPE_LOCK_OPERATION_OPCODE_WRITE_LOCK:
          trace_func(trace_context, "(write lock)");
          break;
        case FBE_PAYLOAD_STRIPE_LOCK_OPERATION_OPCODE_WRITE_UNLOCK:
          trace_func(trace_context, "(write unlock)");
          break;
        default:
          trace_func(trace_context, "UNKNOWN opcode 0x%x", stripe_lock_opcode);
          break;
    }

    return FBE_STATUS_OK;
}
/********************************************
 * end fbe_transport_print_stripe_lock_opcode()
 ********************************************/

/*!**************************************************************
 * fbe_transport_print_stripe_lock_payload()
 ****************************************************************
 * @brief
 *  Display all the trace information related to stripe lock operation
 *
 * @param  stripe_lock_operation_p - Ptr to block operation payload.
 * @param trace_func - Function to use when tracing.
 * @param trace_context - The context to be passed to the trace function.
 *
 * @return - FBE_STATUS_OK  
 *
 * @author
 *  6/24/2010 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_transport_print_stripe_lock_payload(const fbe_u8_t * module_name,
                                                     fbe_u64_t stripe_lock_operation_p,
                                                     fbe_trace_func_t trace_func,
                                                     fbe_trace_context_t trace_context,
                                                     fbe_u32_t spaces_to_indent)
{
    fbe_status_t status;
	fbe_trace_indent(trace_func, trace_context, 8 /*spaces_to_indent*/);
    status = fbe_debug_display_structure(module_name, trace_func, trace_context, stripe_lock_operation_p,
                                         &fbe_stripe_lock_operation_debug_struct_info,
                                         4    /* fields per line */,
                                         10);
    trace_func(trace_context, "\n");
    return status;
}
/*************************************************
 * end fbe_transport_print_stripe_lock_payload()
 *************************************************/
/*!**************************************************************
 * fbe_metadata_cmi_sl_display()
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
static fbe_status_t fbe_metadata_cmi_sl_display(const fbe_u8_t * module_name,
                                                fbe_dbgext_ptr base_ptr,
                                                fbe_trace_func_t trace_func,
                                                fbe_trace_context_t trace_context,
                                                fbe_debug_field_info_t *field_info_p,
                                                fbe_u32_t spaces_to_indent)
{
    fbe_status_t status;
	fbe_trace_indent(trace_func, trace_context, 8 /*spaces_to_indent*/);
    status = fbe_debug_display_structure(module_name, trace_func, trace_context, base_ptr + field_info_p->offset,
                                         &fbe_metadata_cmi_stripe_lock_debug_struct_info,
                                         4    /* fields per line */,
                                         10);
    return status;
}
/********************************************
 * end fbe_metadata_cmi_sl_display()
 ********************************************/

/*!**************************************************************
 * fbe_metadata_sl_region_display()
 ****************************************************************
 * @brief
 *  Display all the trace information related to md sl region.
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
fbe_status_t fbe_metadata_sl_region_display(const fbe_u8_t * module_name,
                                            fbe_dbgext_ptr base_ptr,
                                            fbe_trace_func_t trace_func,
                                            fbe_trace_context_t trace_context,
                                            fbe_debug_field_info_t *field_info_p,
                                            fbe_u32_t spaces_to_indent)
{
    fbe_status_t status;
    trace_func(trace_context, "%s: ", field_info_p->name);
    status = fbe_debug_display_structure(module_name, trace_func, trace_context, base_ptr + field_info_p->offset,
                                         &fbe_metadata_stripe_lock_region_debug_struct_info,
                                         4    /* fields per line */,
                                         10);
    return status;
}
/********************************************
 * end fbe_metadata_sl_region_display()
 ********************************************/

/*!**************************************************************
 * fbe_metadata_cmi_message_header_display()
 ****************************************************************
 * @brief
 *  Display all the trace information related to md sl region.
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
 *
 ****************************************************************/
fbe_status_t fbe_metadata_cmi_message_header_display(const fbe_u8_t * module_name,
                                                     fbe_dbgext_ptr base_ptr,
                                                     fbe_trace_func_t trace_func,
                                                     fbe_trace_context_t trace_context,
                                                     fbe_debug_field_info_t *field_info_p,
                                                     fbe_u32_t spaces_to_indent)
{
    fbe_status_t status;
    trace_func(trace_context, "%s: ", field_info_p->name);
    status = fbe_debug_display_structure(module_name, trace_func, trace_context, base_ptr + field_info_p->offset,
                                         &fbe_metadata_cmi_message_header_debug_struct_info,
                                         4    /* fields per line */,
                                         10);
    return status;
}
/********************************************
 * end fbe_metadata_cmi_message_header_display()
 ********************************************/

/*************************
 * end file fbe_stripe_lock_payload_debug.c.c
 *************************/
