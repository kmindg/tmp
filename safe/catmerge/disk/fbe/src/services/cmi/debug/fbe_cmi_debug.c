/***************************************************************************
 * Copyright (C) EMC Corporation 2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_cmi_debug.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the debug functions for FBE CMI service.
 *
 * @author
 *  06/04/2012 - Created. Lili Chen
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_types.h"
#include "fbe/fbe_dbgext.h"
#include "fbe/fbe_transport.h"
#include "fbe_block_transport.h"
#include "fbe_transport_debug.h"
#include "fbe_cmi_debug.h"
#include "fbe_cmi_private.h"
#include "fbe_cmi_io.h"


/*************************
 *   FUNCTION DEFINITIONS
 *************************/
fbe_status_t fbe_cmi_io_print_context(const fbe_u8_t * module_name,
                                      fbe_u64_t io_context_p,
                                      fbe_trace_func_t trace_func,
                                      fbe_trace_context_t trace_context,
                                      fbe_u32_t spaces_to_indent);
fbe_status_t fbe_cmi_io_print_context_info(const fbe_u8_t * module_name,
                                           fbe_dbgext_ptr io_context_info_p,
                                           fbe_trace_func_t trace_func,
                                           fbe_trace_context_t trace_context,
										   fbe_debug_field_info_t *field_info_p,
                                           fbe_u32_t spaces_to_indent);
fbe_status_t fbe_cmi_io_print_float_data(const fbe_u8_t * module_name,
                                         fbe_dbgext_ptr base_ptr,
                                         fbe_trace_func_t trace_func,
                                         fbe_trace_context_t trace_context,
										 fbe_debug_field_info_t *field_info_p,
                                         fbe_u32_t spaces_to_indent);

/*!*******************************************************************
 * @var fbe_cmi_debug_service_field_info
 *********************************************************************
 * @brief Information about each of the fields of cmi service.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_cmi_debug_service_field_info[] =
{
    /* Object id is first.
     */
    FBE_DEBUG_DECLARE_FIELD_INFO("outstanding_message_counter", fbe_u32_t, FBE_FALSE, "%d"),
    FBE_DEBUG_DECLARE_FIELD_INFO("received_message_counter", fbe_u32_t, FBE_FALSE, "%d"),
    FBE_DEBUG_DECLARE_FIELD_INFO("traffic_disabled", fbe_u32_t, FBE_FALSE, "%d"),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_cmi_debug_service_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        cmi service structure.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_cmi_service_t,
                              fbe_cmi_debug_service_struct_info,
                              &fbe_cmi_debug_service_field_info[0]);

/*!*******************************************************************
 * @var fbe_cmi_io_float_data_debug_field_info
 *********************************************************************
 * @brief Information about each of the fields of fbe_cmi_io_float_data_t.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_cmi_io_float_data_debug_field_info[] =
{
    /* Object id is first.
     */
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO("msg_type", fbe_u32_t, FBE_TRUE, "%d"),
    FBE_DEBUG_DECLARE_FIELD_INFO("fixed_data_length", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("payload_opcode", fbe_payload_opcode_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("original_packet", fbe_u64_t, FBE_FALSE, "0x%x", fbe_debug_display_pointer),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_cmi_io_float_data_debug_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        fbe_cmi_io_float_data_t structure.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_cmi_io_float_data_t,
                              fbe_cmi_io_float_data_debug_struct_info,
                              &fbe_cmi_io_float_data_debug_field_info[0]);

/*!*******************************************************************
 * @var fbe_cmi_io_context_info_debug_field_info
 *********************************************************************
 * @brief Information about each of the fields of fbe_cmi_io_context_info_t.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_cmi_io_context_info_debug_field_info[] =
{
    /* Object id is first.
     */
    FBE_DEBUG_DECLARE_FIELD_INFO("pool", fbe_u32_t, FBE_FALSE, "%d"),
    FBE_DEBUG_DECLARE_FIELD_INFO("slot", fbe_u32_t, FBE_FALSE, "%d"),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("float_data", fbe_cmi_io_float_data_t, FBE_FALSE, "0x%x", 
                                    fbe_cmi_io_print_float_data),   
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_cmi_io_context_info_debug_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        fbe_cmi_io_context_info_t structure.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_cmi_io_context_info_t,
                              fbe_cmi_io_context_info_debug_struct_info,
                              &fbe_cmi_io_context_info_debug_field_info[0]);

/*!*******************************************************************
 * @var fbe_cmi_io_context_debug_field_info
 *********************************************************************
 * @brief Information about each of the fields of fbe_cmi_io_context_t.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_cmi_io_context_debug_field_info[] =
{
    /* Object id is first.
     */
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("context_info", fbe_cmi_io_context_info_t, FBE_FALSE, "0x%x", 
                                    fbe_cmi_io_print_context_info),   
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_cmi_io_context_debug_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        fbe_cmi_io_context_t structure.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_cmi_io_context_t,
                              fbe_cmi_io_context_debug_struct_info,
                              &fbe_cmi_io_context_debug_field_info[0]);


/*!*******************************************************************
 * @var fbe_cmi_debug_service_struct_info
 *********************************************************************
 * @brief This structure holds queue information for io context.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_QUEUE_INFO(fbe_cmi_io_context_t, "queue_element", 
                             FBE_TRUE, /* queue element is the first field in the object. */
                             fbe_cmi_io_context_queue_info,
                             fbe_cmi_io_print_context);


/*!*******************************************************************
 * @var fbe_cmi_service_io_context_queue_field_info
 *********************************************************************
 * @brief Information about each of the fields of context queue.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_cmi_service_io_context_queue_field_info[] =
{
    FBE_DEBUG_DECLARE_FIELD_INFO_QUEUE("context_queue_head", fbe_queue_head_t, FBE_FALSE, 
                                       &fbe_cmi_io_context_queue_info),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};


/*!**************************************************************
 * fbe_cmi_debug_display_info()
 ****************************************************************
 * @brief
 *  Display fbe CMI service information.
 *
 * @param module_name - Module name to use in symbol lookup.
 * @param struct_ptr - ptr to value to display.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param field_info_p - Info for getting and displaying the field. 
 * @param spaces_to_indent - Number of spaces to indent the next line(s).
 * 
 * @return fbe_status_t   
 *
 * @author
 *  06/06/2012 - Created. Lili Chen
 *
 ****************************************************************/
fbe_status_t fbe_cmi_debug_display_info(const fbe_u8_t * module_name,
                                        fbe_trace_func_t trace_func,
                                        fbe_trace_context_t trace_context,
                                        fbe_u32_t spaces_to_indent)
{
    fbe_dbgext_ptr cmi_service_p = 0;

    /* Display basic CMI information.
     */
	FBE_GET_EXPRESSION(module_name, fbe_cmi_service, &cmi_service_p);
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    trace_func(trace_context, "CMI info: \n");
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent + 4);
    fbe_debug_display_structure(module_name, trace_func, trace_context, cmi_service_p,
                                &fbe_cmi_debug_service_struct_info,
                                1 /* fields per line */,
                                spaces_to_indent + 4);
    trace_func(trace_context, "\n");
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_cmi_debug_display_info
 **************************************/

/*!**************************************************************
 * fbe_cmi_debug_display_io()
 ****************************************************************
 * @brief
 *  Display fbe CMI IO information.
 *
 * @param module_name - Module name to use in symbol lookup.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param spaces_to_indent - Number of spaces to indent the next line(s).
 * 
 * @return fbe_status_t   
 *
 * @author
 *  06/06/2012 - Created. Lili Chen
 *
 ****************************************************************/
fbe_status_t fbe_cmi_debug_display_io(const fbe_u8_t * module_name,
                                      fbe_trace_func_t trace_func,
                                      fbe_trace_context_t trace_context,
                                      fbe_u32_t spaces_to_indent)
{
    fbe_u64_t packet_queue_p = 0;
    fbe_status_t status;
    fbe_u32_t i;

    trace_func(trace_context, "\n");
    /* Waiting queue */
    FBE_GET_EXPRESSION(module_name, sep_io_waiting_queue_head, &packet_queue_p);
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    trace_func(trace_context, "IO waiting queue: 0x%llx\n", (unsigned long long)packet_queue_p);
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent + 4);
    fbe_transport_print_packet_queue(module_name,
                                     packet_queue_p,
                                     trace_func,
                                     trace_context,
                                     spaces_to_indent);
    trace_func(trace_context, "\n");

    /* Outstanding queue */
    FBE_GET_EXPRESSION(module_name, sep_io_outstanding_queue_head, &packet_queue_p);
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    trace_func(trace_context, "IO outstanding queue: 0x%llx\n", (unsigned long long)packet_queue_p);
    status = fbe_debug_display_queue(module_name, packet_queue_p, 
                                     fbe_cmi_service_io_context_queue_field_info,
                                     trace_func, trace_context, spaces_to_indent);
    if (status != FBE_STATUS_OK) 
    {
        trace_func(trace_context, "%s failure displaying queue\n", 
               __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    trace_func(trace_context, "\n");

    /* Receive processing queue */
    FBE_GET_EXPRESSION(module_name, sep_io_receive_processing_queue_head, &packet_queue_p);
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    trace_func(trace_context, "IO receive processing queue: 0x%llx\n", (unsigned long long)packet_queue_p);
    status = fbe_debug_display_queue(module_name, packet_queue_p, 
                                     fbe_cmi_service_io_context_queue_field_info,
                                     trace_func, trace_context, spaces_to_indent);
    if (status != FBE_STATUS_OK) 
    {
        trace_func(trace_context, "%s failure displaying queue\n", 
               __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    trace_func(trace_context, "\n");

    /* send context queue */
    FBE_GET_EXPRESSION(module_name, sep_io_send_context_head, &packet_queue_p);
    for (i = 0; i < MAX_SEP_IO_DATA_POOLS; i++) {
        fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
        trace_func(trace_context, "Send context queue %d: 0x%llx\n", i, (unsigned long long)packet_queue_p);
        status = fbe_debug_display_queue(module_name, packet_queue_p, 
                                     fbe_cmi_service_io_context_queue_field_info,
                                     trace_func, trace_context, spaces_to_indent);
        if (status != FBE_STATUS_OK) 
        {
            trace_func(trace_context, "%s failure displaying queue\n", __FUNCTION__);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        trace_func(trace_context, "\n");
        packet_queue_p += sizeof(fbe_queue_head_t);
    }

    /* receive context queue */
    FBE_GET_EXPRESSION(module_name, sep_io_receive_context_head, &packet_queue_p);
    for (i = 0; i < MAX_SEP_IO_DATA_POOLS; i++) {
        fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
        trace_func(trace_context, "Receive context queue %d: 0x%llx\n", i, (unsigned long long)packet_queue_p);
        status = fbe_debug_display_queue(module_name, packet_queue_p, 
                                     fbe_cmi_service_io_context_queue_field_info,
                                     trace_func, trace_context, spaces_to_indent);
        if (status != FBE_STATUS_OK) 
        {
            trace_func(trace_context, "%s failure displaying queue\n", __FUNCTION__);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        trace_func(trace_context, "\n");
        packet_queue_p += sizeof(fbe_queue_head_t);
    }

    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_cmi_debug_display_io
 **************************************/

/*!**************************************************************
 * fbe_cmi_io_print_context()
 ****************************************************************
 * @brief
 *  Display CMI IO context information.
 *
 * @param module_name - Module name to use in symbol lookup.
 * @param io_context_p - ptr to value to display.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param spaces_to_indent - Number of spaces to indent the next line(s).
 * 
 * @return fbe_status_t   
 *
 * @author
 *  06/06/2012 - Created. Lili Chen
 *
 ****************************************************************/
fbe_status_t fbe_cmi_io_print_context(const fbe_u8_t * module_name,
                                      fbe_u64_t io_context_p,
                                      fbe_trace_func_t trace_func,
                                      fbe_trace_context_t trace_context,
                                      fbe_u32_t spaces_to_indent)
{
    fbe_status_t status;
	fbe_trace_indent(trace_func, trace_context, 8 /*spaces_to_indent*/);
    status = fbe_debug_display_structure(module_name, trace_func, trace_context, 
                                         io_context_p,
                                         &fbe_cmi_io_context_debug_struct_info,
                                         4    /* fields per line */,
                                         10);
    trace_func(trace_context, "\n");
    return status;
}
/**************************************
 * end fbe_cmi_io_print_context
 **************************************/

/*!**************************************************************
 * fbe_cmi_io_print_context_info()
 ****************************************************************
 * @brief
 *  Display CMI IO context_info information.
 *
 * @param module_name - Module name to use in symbol lookup.
 * @param struct_ptr - ptr to value to display.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param field_info_p - Info for getting and displaying the field. 
 * @param spaces_to_indent - Number of spaces to indent the next line(s).
 * 
 * @return fbe_status_t   
 *
 * @author
 *  06/06/2012 - Created. Lili Chen
 *
 ****************************************************************/
fbe_status_t fbe_cmi_io_print_context_info(const fbe_u8_t * module_name,
                                           fbe_dbgext_ptr io_context_info_p,
                                           fbe_trace_func_t trace_func,
                                           fbe_trace_context_t trace_context,
										   fbe_debug_field_info_t *field_info_p,
                                           fbe_u32_t spaces_to_indent)
{
    fbe_status_t status;
    status = fbe_debug_display_structure(module_name, trace_func, trace_context, 
                                         io_context_info_p + field_info_p->offset,
                                         &fbe_cmi_io_context_info_debug_struct_info,
                                         4    /* fields per line */,
                                         10);
    return status;
}
/**************************************
 * end fbe_cmi_io_print_context_info
 **************************************/

/*!**************************************************************
 * fbe_cmi_io_print_float_data()
 ****************************************************************
 * @brief
 *  Display fbe CMI IO float data information.
 *
 * @param module_name - Module name to use in symbol lookup.
 * @param base_ptr - ptr to value to display.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param field_info_p - Info for getting and displaying the field. 
 * @param spaces_to_indent - Number of spaces to indent the next line(s).
 * 
 * @return fbe_status_t   
 *
 * @author
 *  06/06/2012 - Created. Lili Chen
 *
 ****************************************************************/
fbe_status_t fbe_cmi_io_print_float_data(const fbe_u8_t * module_name,
                                         fbe_dbgext_ptr base_ptr,
                                         fbe_trace_func_t trace_func,
                                         fbe_trace_context_t trace_context,
										 fbe_debug_field_info_t *field_info_p,
                                         fbe_u32_t spaces_to_indent)
{
    fbe_status_t status;
    status = fbe_debug_display_structure(module_name, trace_func, trace_context, 
                                         base_ptr + field_info_p->offset,
                                         &fbe_cmi_io_float_data_debug_struct_info,
                                         4    /* fields per line */,
                                         10);
    return status;
}
/**************************************
 * end fbe_cmi_io_print_float_data
 **************************************/

/*************************
 * end file fbe_cmi_debug.c
 *************************/
