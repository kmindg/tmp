/***************************************************************************
 * Copyright (C) EMC Corporation 2008-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_packet_debug.c
 ****************************************************************************
 *
 * @brief
 *  This file contains the debug functions for fbe packet.
 *
 * @author
 *   02/16/2009:  Created. Nishit Singh
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_types.h"
#include "fbe/fbe_dbgext.h"
#include "fbe_base_object_debug.h"
#include "fbe/fbe_transport.h"
#include "fbe_base_object_trace.h"
#include "fbe_logical_drive_debug.h"
#include "fbe_transport_debug.h"
#include "pp_ext.h"
#include "fbe_raid_library_debug.h"


static fbe_status_t fbe_packet_status_debug_trace(const fbe_u8_t * module_name,
                                                  fbe_dbgext_ptr base_ptr,
                                                  fbe_trace_func_t trace_func,
                                                  fbe_trace_context_t trace_context,
                                                  fbe_debug_field_info_t *field_info_p,
                                                  fbe_u32_t spaces_to_indent);
static fbe_status_t fbe_packet_state_debug_trace(const fbe_u8_t * module_name,
                                                 fbe_dbgext_ptr base_ptr,
                                                 fbe_trace_func_t trace_func,
                                                 fbe_trace_context_t trace_context,
                                                 fbe_debug_field_info_t *field_info_p,
                                                 fbe_u32_t spaces_to_indent);
fbe_status_t fbe_packet_print_state(fbe_packet_state_t packet_state,
                                    fbe_trace_func_t trace_func,
                                    fbe_trace_context_t trace_context);
/*!*******************************************************************
 * @var fbe_packet_debug_packet_queue_info_t
 *********************************************************************
 * @brief This structure holds info used to display a packet
 *        queue using the "queue_element" field of the packet.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_QUEUE_INFO(fbe_packet_t, "queue_element", 
                             FBE_FALSE, /* queue element is not the first field in the packet. */
                             fbe_packet_debug_packet_queue_info,
                             fbe_transport_print_fbe_packet);


/*!*******************************************************************
 * @var fbe_packet_debug_sub_packet_queue_info_t
 *********************************************************************
 * @brief This structure holds info used to display a packet
 *        queue using the "subpacket_queue_element" field of the packet.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_QUEUE_INFO(fbe_packet_t, "subpacket_queue_element", 
                             FBE_FALSE, /* queue element is not the first field in the packet. */
                             fbe_packet_debug_sub_packet_queue_info,
                             fbe_transport_print_fbe_packet);

/*!*******************************************************************
 * @var fbe_packet_status_field_info
 *********************************************************************
 * @brief Information about each of the fields of rdgen service.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_packet_status_field_info[] =
{
    /* Object id is first.
     */
    FBE_DEBUG_DECLARE_FIELD_INFO("code", fbe_status_t, FBE_TRUE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("qualifier", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_packet_status_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        fbe_packet_status_t.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_packet_status_t,
                              fbe_packet_status_struct_info,
                              &fbe_packet_status_field_info[0]);
/*!*******************************************************************
 * @var fbe_packet_field_info
 *********************************************************************
 * @brief Information about each of the fields of rdgen service.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_packet_field_info[] =
{
    /* Object id is first.
     */
    FBE_DEBUG_DECLARE_FIELD_INFO("magic_number", fbe_u64_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("master_packet", fbe_packet_t, FBE_FALSE, "0x%x", 
                                    fbe_debug_display_pointer),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("packet_cancel_function", fbe_packet_cancel_function_t, FBE_FALSE, "0x%x", 
                                    fbe_debug_display_function_ptr),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("packet_cancel_context", fbe_packet_cancel_context_t, FBE_FALSE, "0x%x", 
                                    fbe_debug_display_pointer),

    FBE_DEBUG_DECLARE_FIELD_INFO("package_id", fbe_package_id_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("service_id", fbe_service_id_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("class_id", fbe_class_id_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("object_id", fbe_object_id_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("cpu_id", fbe_cpu_id_t, FBE_FALSE, "0x%x"),

    FBE_DEBUG_DECLARE_FIELD_INFO("expiration_time", fbe_time_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("packet_attr", fbe_packet_attr_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("packet_state", fbe_atomic_t, FBE_FALSE, "0x%x", fbe_packet_state_debug_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("packet_status", fbe_packet_status_t, FBE_FALSE, "0x%x", fbe_packet_status_debug_trace),

    FBE_DEBUG_DECLARE_FIELD_INFO("packet_priority", fbe_packet_priority_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("traffic_priority", fbe_traffic_priority_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("resource_priority", fbe_packet_resource_priority_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("io_stamp", fbe_packet_io_stamp_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("tag", fbe_u8_t, FBE_FALSE, "0x%x"),

    FBE_DEBUG_DECLARE_FIELD_INFO_FN("memory_request", fbe_memory_request_t, FBE_FALSE, "0x%x", 
                                    fbe_memory_request_debug_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),

    FBE_DEBUG_DECLARE_FIELD_INFO_QUEUE("subpacket_queue_head", fbe_queue_head_t, FBE_FALSE, 
                                       &fbe_packet_debug_sub_packet_queue_info),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_packet_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        fbe_packet_t.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_packet_t,
                              fbe_packet_struct_info,
                              &fbe_packet_field_info[0]);

/*************************
 *   FUNCTION DEFINITIONS
 *************************/
fbe_status_t fbe_payload_print_opcode(fbe_payload_opcode_t payload_opcode,
                                      fbe_trace_func_t trace_func,
                                      fbe_trace_context_t trace_context);

/* These globals save the offsets that we will set once.
 */
static fbe_u32_t tracker_ring_offset = FBE_U32_MAX;
static fbe_u32_t tracker_current_index_offset;
static fbe_u32_t tracker_index_wrapped_offset;
static fbe_u32_t tracker_callback_offset;
static fbe_u32_t tracker_coarse_time_offset;
static fbe_u32_t tracker_action_offset;
static fbe_u32_t tracker_entry_size;

/*!*******************************************************************
 * @def FBE_TRACKER_MAX_SYMBOL_NAME_LENGTH
 *********************************************************************
 * @brief This is the max length we use to size arrays that will hold
 *        a symbol name.
 *
 *********************************************************************/
#define FBE_TRACKER_MAX_SYMBOL_NAME_LENGTH 256
fbe_status_t fbe_packet_display_tracker(const fbe_u8_t * module_name,
                                            fbe_u64_t fbe_packet_p,
                                            fbe_trace_func_t trace_func,
                                            fbe_trace_context_t trace_context,
                                            fbe_u32_t spaces_to_indent)
{
    csx_string_t functionName = NULL;
    fbe_u32_t tracker_count;          /* Counter of number of states output. */
    fbe_u64_t tracker_address = 0;
    fbe_u32_t ptr_size;
    fbe_u32_t indent;
    fbe_u8_t tracker_current_index;  /* Entry to start printing at. */
    fbe_u8_t tracker_index_wrapped;  /* Entry to start printing at. */
    fbe_u64_t log_p = 0;
    fbe_u32_t max_log_entries = FBE_PACKET_TRACKER_DEPTH;
    fbe_u32_t current_coarse_time;
    fbe_u64_t time_ptr = 0;
    fbe_time_t current_time;

    if (tracker_ring_offset == FBE_U32_MAX)
    {
        FBE_GET_FIELD_OFFSET(module_name, fbe_packet_t, "tracker_ring", &tracker_ring_offset);
        FBE_GET_FIELD_OFFSET(module_name, fbe_packet_t, "tracker_current_index", &tracker_current_index_offset);
        FBE_GET_FIELD_OFFSET(module_name, fbe_packet_t, "tracker_index_wrapped", &tracker_index_wrapped_offset);
        FBE_GET_FIELD_OFFSET(module_name, fbe_packet_tracker_entry_t, "callback_fn", &tracker_callback_offset);
        FBE_GET_FIELD_OFFSET(module_name, fbe_packet_tracker_entry_t, "coarse_time", &tracker_coarse_time_offset);
        FBE_GET_FIELD_OFFSET(module_name, fbe_packet_tracker_entry_t, "action", &tracker_action_offset);
        FBE_GET_TYPE_SIZE(module_name, fbe_packet_tracker_entry_t, &tracker_entry_size);
    }
    log_p = fbe_packet_p + tracker_ring_offset;

    FBE_READ_MEMORY(fbe_packet_p + tracker_current_index_offset, &tracker_current_index, sizeof(fbe_char_t));
    FBE_READ_MEMORY(fbe_packet_p + tracker_index_wrapped_offset, &tracker_index_wrapped, sizeof(fbe_char_t));

    /* Get the current time.
     */
    FBE_GET_EXPRESSION(module_name, idle_thread_time, &time_ptr);
    FBE_READ_MEMORY(time_ptr, &current_time, sizeof(fbe_time_t));
    current_coarse_time = (fbe_u32_t)current_time;
    
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    trace_func(trace_context, "tracker_current_index: %d tracker_index_wrapped: %d current_time: 0x%llx coarse_current_time: 0x%x\n",
               tracker_current_index, tracker_index_wrapped, (unsigned long long)current_time, current_coarse_time);

    /* If we did not wrap, then we will only display as many entries as we logged.
     */
    if (tracker_index_wrapped == FBE_FALSE)
    {
        max_log_entries = tracker_current_index;
    }

    if (tracker_current_index >= FBE_PACKET_TRACKER_DEPTH)
    {
        trace_func(trace_context, "tracker_current_index is %d >= FBE_PACKET_TRACKER_DEPTH\n",
                   tracker_current_index);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    
    trace_func(trace_context, "tracker: ");

    fbe_debug_get_ptr_size(module_name, &ptr_size);

    /* Loop over all the states.
     */
    for (tracker_count = 0;
         tracker_count < max_log_entries; 
         tracker_count++)
    {
        fbe_u64_t callback;
        fbe_u32_t coarse_time;
        stack_tracker_action action;

        if (FBE_CHECK_CONTROL_C())
        {
            return FBE_STATUS_CANCELED;
        }

        /* Update the index pointer.  Wrap around if we hit the beginning of
         * the circular buffer.
         */
        if (tracker_current_index == 0)
        {
            tracker_current_index = max_log_entries;
        }
        tracker_current_index--;

        /* Calculate and read in the state address and the value of the state.
         */
        tracker_address = (log_p + ( tracker_current_index * tracker_entry_size) );


        FBE_READ_MEMORY(tracker_address + tracker_callback_offset, &callback, ptr_size);
        FBE_READ_MEMORY(tracker_address + tracker_coarse_time_offset, &coarse_time, sizeof(coarse_time));
        FBE_READ_MEMORY(tracker_address + tracker_action_offset, &action, sizeof(action));
        functionName = csx_dbg_ext_lookup_symbol_name(callback);
        
        /* If we have no more data in the buffer, break out of the loop.
         */
        if ((functionName == NULL) || (*functionName == '\0'))
        {
            /* If functionName isn't NULL, csx dbg allocates the memory.
             * We should free it here
             */
            if (functionName != NULL)
                csx_dbg_ext_free_symbol_name(functionName);

            break;
        }
        else
        {
            /* Insert an extra newline every 2 states to keep output reasonable.
             */
            if (tracker_count != 0) //&& (tracker_count % 3 == 0))
            {
                trace_func(trace_context, "\n");
    
                for (indent = 0; indent < spaces_to_indent; indent ++)
                {
                    trace_func(trace_context, " ");
                }
                trace_func(trace_context, "         ");
            }
            if ((action & PACKET_STACK_TIME_DELTA) == 0)
            {
                fbe_u32_t delta = current_coarse_time - coarse_time;
                if (current_coarse_time > coarse_time)
                {
                    trace_func(trace_context, "[%2d]: 0x%8x (delta: %08dms) 0x%08x %s", tracker_count, coarse_time, delta, action, functionName);
                }
                else
                {
                    trace_func(trace_context, "[%2d]: 0x%8x 0x%08x %s", tracker_count, coarse_time, action, functionName);
                }
            }
            else
            {
                trace_func(trace_context, "[%2d]:            (delta: %08dms) 0x%08x %s", tracker_count ,  coarse_time, action, functionName);
            }

        }/* end else function name not null. */

        csx_dbg_ext_free_symbol_name(functionName);
    } /* end for tracker_count < max_log_entries */

    trace_func(trace_context, "\n");


    return FBE_STATUS_OK;
}
/*!**************************************************************
 * fbe_transport_print_fbe_packet()
 ****************************************************************
 * @brief
 *  Display's all the trace information about the fbe packet.
 *
 * @param  module_name - This is the name of the module we are debugging.
 * @param fbe_packet_p - Ptr to fbe packet.
 * @param trace_func - Function to use when tracing.
 * @param trace_context - The context to be passed to the trace function.
 *
 * @return - FBE_STATUS_OK  
 *
 * HISTORY:
 *  02/13/2009 - Created. Nishit Singh
 *
 ****************************************************************/
fbe_status_t fbe_transport_print_fbe_packet(const fbe_u8_t * module_name,
                                            fbe_u64_t fbe_packet_p,
                                            fbe_trace_func_t trace_func,
                                            fbe_trace_context_t trace_context,
                                            fbe_u32_t spaces_to_indent)
{
    fbe_status_t status;
    fbe_u32_t payload_offset;
    fbe_u32_t current_level;
    fbe_u32_t stack_offset;
    fbe_u32_t stack_size;
    fbe_u64_t stack_fn = 0;
    fbe_u64_t stack_context = 0;
    fbe_u32_t ptr_size;
    fbe_u64_t magic_number;

    fbe_u32_t iots_offset;
    fbe_u32_t siots_offset;

    status = fbe_debug_get_ptr_size(module_name, &ptr_size);
    if (status != FBE_STATUS_OK) 
    {
        trace_func(trace_context, "%s unable to get ptr size status: %d\n", __FUNCTION__, status);
        return status; 
    }

    FBE_GET_FIELD_OFFSET(module_name, fbe_packet_t, "payload_union", &payload_offset);
    FBE_GET_FIELD_OFFSET(module_name, fbe_packet_t, "stack", &stack_offset);

    FBE_GET_TYPE_SIZE(module_name, fbe_packet_stack_t, &stack_size);

    FBE_GET_FIELD_DATA(module_name, 
                       fbe_packet_p,
                       fbe_packet_t,
                       magic_number,
                       sizeof(fbe_u64_t),
                       &magic_number);

    FBE_GET_FIELD_DATA(module_name, 
                       fbe_packet_p,
                       fbe_packet_t,
                       current_level,
                       sizeof(fbe_u32_t),
                       &current_level);


    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    trace_func(trace_context, "packet: 0x%llx ", (unsigned long long)fbe_packet_p);
    /* Display individual fields.
     */
    status = fbe_debug_display_structure(module_name, trace_func, trace_context, fbe_packet_p,
                                         &fbe_packet_struct_info,
                                         4 /* fields per line */,
                                         spaces_to_indent + 2);

    /* Display the packet stack.
     */
    if (current_level > FBE_PACKET_STACK_SIZE)
    {
        fbe_trace_indent(trace_func, trace_context, spaces_to_indent + 2);
        trace_func(trace_context, "level: %d > FBE_PACKET_STACK_SIZE %d \n", current_level, FBE_PACKET_STACK_SIZE);
    }
    else
    {
        while (current_level >= 0)
        {
            FBE_READ_MEMORY(fbe_packet_p + stack_offset + (stack_size * current_level), &stack_fn, ptr_size);
            FBE_READ_MEMORY(fbe_packet_p + stack_offset + (stack_size * current_level) + ptr_size, &stack_context, ptr_size);
            
            fbe_trace_indent(trace_func, trace_context, spaces_to_indent + 2);
            trace_func(trace_context, "level: %d callback: ", current_level);
            fbe_debug_display_function_ptr_symbol(module_name, stack_fn, trace_func, trace_context, spaces_to_indent);
            trace_func(trace_context, " context: 0x%llx\n",
		       (unsigned long long)stack_context);
    
            if (current_level == 0)
            {
                break;
            }
            current_level--;
            if (FBE_CHECK_CONTROL_C())
            {
                return FBE_STATUS_GENERIC_FAILURE;
            }
        }
    }

    fbe_packet_display_tracker(module_name, fbe_packet_p, trace_func, trace_context, spaces_to_indent + 2);
    
    /* Do not display payload if active queue display is disabled 
     */
    if(get_active_queue_display_flag() != FBE_FALSE)
    {
        /* Display the payloads.       
        */
        fbe_trace_indent(trace_func, trace_context, spaces_to_indent + 2);

        FBE_GET_FIELD_OFFSET(module_name, fbe_payload_ex_t, "iots", &iots_offset);
        FBE_GET_FIELD_OFFSET(module_name, fbe_payload_ex_t, "siots", &siots_offset);
        trace_func(trace_context, "payload IOTS: 0x%llx SIOTS: 0x%llx\n",
                   (unsigned long long)fbe_packet_p + payload_offset + iots_offset,
                   (unsigned long long)fbe_packet_p + payload_offset + siots_offset);

        fbe_transport_print_fbe_packet_payload(module_name,
                                               fbe_packet_p + payload_offset,
                                               trace_func,
                                               trace_context,
                                               spaces_to_indent + 2);

        status = fbe_transport_should_display_payload_iots(module_name, fbe_packet_p,
                                                           fbe_packet_p + payload_offset);
        if (status == FBE_STATUS_OK)
        {
            fbe_u32_t iots_offset;
            FBE_GET_FIELD_OFFSET(module_name, fbe_payload_ex_t, "iots", &iots_offset);
            fbe_raid_library_debug_print_iots(module_name, fbe_packet_p + payload_offset + iots_offset,
                                              trace_func, trace_context, spaces_to_indent + 2);
        }
    }
    else
    {
        trace_func(trace_context, "Active queue display is disabled, don't display payload.\n");
    }

    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    trace_func(trace_context, "end packet 0x%llx\n", (unsigned long long)fbe_packet_p);
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_transport_print_fbe_packet()
 ******************************************/

/*!**************************************************************
 * fbe_transport_print_fbe_packet_payload()
 ****************************************************************
 * @brief
 *  Display's all the trace information about the fbe packet.
 *
 * @param  module_name - This is the name of the module we are debugging.
 * @param fbe_payload_p - Ptr to fbe packet payload.
 * @param trace_func - Function to use when tracing.
 * @param trace_context - The context to be passed to the trace function.
 *
 * @return - FBE_STATUS_OK  
 *
 * HISTORY:
 *  02/26/2009 - Created. Nishit Singh
 *
 ****************************************************************/
fbe_status_t fbe_transport_print_fbe_packet_payload(const fbe_u8_t * module_name,
                                                    fbe_u64_t fbe_payload_p,
                                                    fbe_trace_func_t trace_func,
                                                    fbe_trace_context_t trace_context,
                                                    fbe_u32_t spaces_to_indent)
{
    fbe_u64_t sg_list, current_operation, next_operation, key_handle;
    //fbe_u32_t block_operation_offset;
    //fbe_u32_t control_operation_offset;
    //fbe_u32_t block_payload_instance;
    //fbe_u32_t control_payload_instance;
    fbe_u32_t header_index;
    fbe_u32_t ptr_size = 0;
    fbe_debug_get_ptr_size(module_name, &ptr_size);
/*
    FBE_GET_FIELD_OFFSET(module_name,
                         fbe_payload_ex_t,
                         "block_operation",
                         &block_operation_offset);
    FBE_GET_FIELD_OFFSET(module_name,
                         fbe_payload_ex_t,
                         "control_operation",
                         &control_operation_offset);
*/

    /* Get the value of sg_list stored in fbe_payload_ex_t. This information
     * will be displayed alongwith block payload information.
     */
    FBE_GET_FIELD_DATA(module_name,
                       fbe_payload_p,
                       fbe_payload_ex_t,
                       sg_list,
                       sizeof(fbe_u64_t),
                       &sg_list);

    FBE_GET_FIELD_DATA(module_name,
                       fbe_payload_p,
                       fbe_payload_ex_t,
                       current_operation,
                       sizeof(fbe_u64_t),
                       &current_operation);

    FBE_GET_FIELD_DATA(module_name, 
                       fbe_payload_p,
                       fbe_payload_ex_t,
                       next_operation,
                       sizeof(fbe_u64_t),
                       &next_operation);

    FBE_GET_FIELD_DATA(module_name, 
                       fbe_payload_p,
                       fbe_payload_ex_t,
                       key_handle,
                       sizeof(fbe_u64_t),
                       &key_handle);


    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);

    trace_func(trace_context, " fbe payload: sg_list: 0x%llx",
	       (unsigned long long)sg_list);
    trace_func(trace_context, " current operation: 0x%llx",
	       (unsigned long long)current_operation);
    trace_func(trace_context, " next operation:  0x%llx",
	       (unsigned long long)next_operation);
    trace_func(trace_context, " key_handle:  0x%llx\n",
	       (unsigned long long)key_handle);

    /* Loop over all the operations in the payload, starting with the 
     * current active one.  We'll keep going until we hit the end (top) 
     * of the payloads. 
     */
    header_index = 0;
    while (current_operation != NULL)
    {
        fbe_payload_opcode_t payload_opcode;
        fbe_u64_t prev_header;

        if (FBE_CHECK_CONTROL_C())
        {
            return FBE_STATUS_GENERIC_FAILURE;
        }
        
        FBE_GET_FIELD_DATA(module_name, 
                           current_operation,
                           fbe_payload_operation_header_t,
                           payload_opcode,
                           sizeof(fbe_u32_t),
                           &payload_opcode);

        FBE_GET_FIELD_DATA(module_name, 
                           current_operation,
                           fbe_payload_operation_header_t,
                           prev_header,
                           sizeof(fbe_u64_t),
                           &prev_header);

        /* Display information about the current header.
         */
        fbe_trace_indent(trace_func, trace_context, spaces_to_indent + 2);
        trace_func(trace_context, "%d) header: 0x%llx opcode: 0x%x (",
	           header_index, (unsigned long long)current_operation,
		   payload_opcode);
        fbe_payload_print_opcode(payload_opcode, trace_func, trace_context);
        trace_func(trace_context, ") prev_header: 0x%llx",
		   (unsigned long long)prev_header);
        header_index++;
        trace_func(trace_context, "\n");

        /* Next display more in depth details about this payload.
         */
        {
            switch (payload_opcode)
            {
                case FBE_PAYLOAD_OPCODE_BLOCK_OPERATION:
                    fbe_trace_indent(trace_func, trace_context, spaces_to_indent + 5);                    
                    fbe_tansport_print_fbe_packet_block_payload(module_name,
                                                                current_operation,
                                                                trace_func,
                                                                trace_context,
                                                                spaces_to_indent + 8);
                    break;

                /* @todo add support for displaying other payloads.
                 */
                case FBE_PAYLOAD_OPCODE_CONTROL_OPERATION:
                    fbe_trace_indent(trace_func, trace_context, spaces_to_indent + 5);
					fbe_tansport_print_fbe_packet_control_payload(module_name,
																current_operation,
																trace_func,
																trace_context,
																spaces_to_indent + 8);

					break;
                case FBE_PAYLOAD_OPCODE_DISCOVERY_OPERATION:
					break;
                case FBE_PAYLOAD_OPCODE_DIPLEX_OPERATION:
                    fbe_trace_indent(trace_func, trace_context, spaces_to_indent + 5);
                    trace_func(trace_context, "diplex payload ");
					break;
                case FBE_PAYLOAD_OPCODE_CDB_OPERATION:
                    fbe_trace_indent(trace_func, trace_context, spaces_to_indent + 5);
                    trace_func(trace_context, "cdb payload ");
                    
                    fbe_tansport_print_fbe_packet_cdb_payload(module_name,
                                                                current_operation,
                                                                trace_func,
                                                                trace_context,
                                                                spaces_to_indent + 8);
                    break;

                case FBE_PAYLOAD_OPCODE_FIS_OPERATION:
                    fbe_trace_indent(trace_func, trace_context, spaces_to_indent + 5);
                    trace_func(trace_context, "fis payload  ");
                    
                    fbe_tansport_print_fbe_packet_fis_payload(module_name,
                                                                current_operation,
                                                                trace_func,
                                                                trace_context,
                                                                spaces_to_indent + 8);
                    break;

                case FBE_PAYLOAD_OPCODE_DMRB_OPERATION:
                    fbe_trace_indent(trace_func, trace_context, spaces_to_indent + 5);
                    trace_func(trace_context, "dmrb payload  ");
                    
                    fbe_tansport_print_fbe_packet_dmrb_payload(module_name,
                                                                current_operation,
                                                                trace_func,
                                                                trace_context,
                                                                spaces_to_indent + 8);
                    break;

                case FBE_PAYLOAD_OPCODE_STRIPE_LOCK_OPERATION:
                    fbe_trace_indent(trace_func, trace_context, spaces_to_indent + 5);
                    trace_func(trace_context, "stripe lock payload  ");
                    fbe_transport_print_stripe_lock_payload(module_name,
                                                            current_operation,
                                                            trace_func,
                                                            trace_context,
                                                            spaces_to_indent + 8);
                    break;

                case FBE_PAYLOAD_OPCODE_MEMORY_OPERATION:
                    fbe_trace_indent(trace_func, trace_context, spaces_to_indent + 5);
                    trace_func(trace_context, "memory payload  ");
                    fbe_transport_print_memory_payload(module_name,
                                                       current_operation,
                                                       trace_func,
                                                       trace_context,
                                                       spaces_to_indent + 8);
                    break;
                case FBE_PAYLOAD_OPCODE_BUFFER_OPERATION:
                    fbe_trace_indent(trace_func, trace_context, spaces_to_indent + 5);
                    trace_func(trace_context, "buffer payload  ");
                    fbe_transport_print_buffer_payload(module_name,
                                                       current_operation,
                                                       trace_func,
                                                       trace_context,
                                                       spaces_to_indent + 8);
                    break;
                case FBE_PAYLOAD_OPCODE_SMP_OPERATION:
                default:
                    break;
            };
        }

        current_operation = prev_header;
    }

    return FBE_STATUS_OK;
}
/**********************************************
 * end fbe_transport_print_fbe_packet_payload()
 *********************************************/
/*!**************************************************************
 * fbe_transport_should_display_payload_iots()
 ****************************************************************
 * @brief
 *  Determine if this is an iots we should display.
 *
 * @param module_name
 * @param packet_p
 * @param payload_p
 * 
 * @return fbe_status_t
 *
 * @author
 *  9/4/2012 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_transport_should_display_payload_iots(const fbe_u8_t * module_name, fbe_u64_t packet_p,
                                                       fbe_u64_t payload_p)
{
    fbe_raid_iots_flags_t iots_flags;
    fbe_raid_iots_status_t iots_status;
    fbe_u64_t iots_packet_p = 0;
    fbe_u64_t iots_geo_p = 0;
    fbe_u32_t iots_payload_offset = 0;
    fbe_u64_t iots_p = 0;
    fbe_u32_t ptr_size = 0;
    fbe_debug_get_ptr_size(module_name, &ptr_size);

    FBE_GET_FIELD_OFFSET(module_name, 
                         fbe_payload_ex_t,
                         "iots",
                         &iots_payload_offset);

    iots_p = payload_p + iots_payload_offset;
    FBE_GET_FIELD_DATA(module_name,
                       iots_p,
                       fbe_raid_common_t,
                       flags,
                       sizeof(fbe_raid_common_flags_t),
                       &iots_flags);
    FBE_GET_FIELD_DATA(module_name,
                       iots_p,
                       fbe_raid_iots_t,
                       status,
                       sizeof(fbe_raid_iots_status_t),
                       &iots_status);
    FBE_GET_FIELD_DATA(module_name,
                       iots_p,
                       fbe_raid_iots_t,
                       packet_p,
                       ptr_size,
                       &iots_packet_p);
    FBE_GET_FIELD_DATA(module_name,
                       iots_p,
                       fbe_raid_iots_t,
                       raid_geometry_p,
                       ptr_size,
                       &iots_geo_p);

    /* Display the Iots only if it looks like an iots that is in use.
     */
    if (((iots_flags & FBE_RAID_COMMON_FLAG_STRUCT_TYPE_MASK) == FBE_RAID_COMMON_FLAG_TYPE_IOTS) &&
        (iots_status != FBE_RAID_IOTS_STATUS_INVALID) && 
        (iots_status < FBE_RAID_IOTS_STATUS_LAST) &&
        (iots_packet_p == packet_p))
    {
        return FBE_STATUS_OK;
    }
    return FBE_STATUS_GENERIC_FAILURE;
}
/******************************************
 * end fbe_transport_should_display_payload_iots()
 ******************************************/
/*!**************************************************************
 * fbe_payload_print_opcode()
 ****************************************************************
 * @brief
 *  Display all the trace information related to payload opcode.
 *
 * @param payload_opcode - payload opcode.
 * @param trace_func - Function to use when tracing.
 * @param trace_context - The context to be passed to the trace function.
 *
 * @return - FBE_STATUS_OK  
 *
 * HISTORY:
 *  03/02/2009 - Created. RPF
 *
 ****************************************************************/
fbe_status_t fbe_payload_print_opcode(fbe_payload_opcode_t payload_opcode,
                                      fbe_trace_func_t trace_func,
                                      fbe_trace_context_t trace_context)
{
    switch(payload_opcode)
    {
        case FBE_PAYLOAD_OPCODE_INVALID:
          trace_func(trace_context, "(invalid)");
          break;
        case FBE_PAYLOAD_OPCODE_BLOCK_OPERATION:
          trace_func(trace_context, "block");
          break;
        case FBE_PAYLOAD_OPCODE_CONTROL_OPERATION:
          trace_func(trace_context, "control");
          break;
        case FBE_PAYLOAD_OPCODE_DISCOVERY_OPERATION:
          trace_func(trace_context, "discovery");
          break;
        case FBE_PAYLOAD_OPCODE_CDB_OPERATION:
          trace_func(trace_context, "cdb");
          break;
        case FBE_PAYLOAD_OPCODE_FIS_OPERATION:
          trace_func(trace_context, "fis");
          break;
        case FBE_PAYLOAD_OPCODE_DMRB_OPERATION:
          trace_func(trace_context, "dmrb");
          break;
        case FBE_PAYLOAD_OPCODE_SMP_OPERATION:
          trace_func(trace_context, "smp");
          break;
        case FBE_PAYLOAD_OPCODE_DIPLEX_OPERATION:
          trace_func(trace_context, "diplex");
          break;
        case FBE_PAYLOAD_OPCODE_STRIPE_LOCK_OPERATION:
          trace_func(trace_context, "stripe lock");
          break;
        case FBE_PAYLOAD_OPCODE_MEMORY_OPERATION:
          trace_func(trace_context, "memory");
          break;
        case FBE_PAYLOAD_OPCODE_METADATA_OPERATION:
          trace_func(trace_context, "metadata");
          break;
        case FBE_PAYLOAD_OPCODE_BUFFER_OPERATION:
          trace_func(trace_context, "buffer");
          break;
        default:
          trace_func(trace_context, "UNKNOWN payload opcode 0x%x", payload_opcode);
          break;
    }
    return FBE_STATUS_OK;
}
/********************************************
 * end fbe_payload_print_opcode()
 ********************************************/

/*!**************************************************************
 * fbe_transport_print_packet_queue()
 ****************************************************************
 * @brief
 *  Display's all the element and its detail for a given queue.
 *
 * @param  module_name - This is the name of the module we are debugging.
 * @param queue_p - Pointer to queue.
 * @param trace_func - Function to use when tracing.
 * @param trace_context - The context to be passed to the trace function.
 *
 * @return - FBE_STATUS_OK  
 *
 * HISTORY:
 *  02/13/2009 - Created. Nishit Singh
 *
 ****************************************************************/
fbe_status_t fbe_transport_print_packet_queue(const fbe_u8_t * module_name,
                                                    fbe_u64_t queue_p,
                                                    fbe_trace_func_t trace_func,
                                                    fbe_trace_context_t trace_context,
                                                    fbe_u32_t spaces_to_indent)
{
    fbe_u64_t curr_queue_element_p, next_queue_element_p;
    fbe_u32_t item_count, offset;

    item_count = 0;
    FBE_GET_FIELD_DATA(module_name,
                       queue_p,
                       fbe_queue_head_t,
                       next,
                       sizeof(fbe_u64_t),
                       &next_queue_element_p);

    FBE_GET_FIELD_OFFSET(module_name, fbe_packet_t, "queue_element", &offset);

    /* Iterate through the queue to display the packet detail.
     */
    while (next_queue_element_p != queue_p)
    {
        if (FBE_CHECK_CONTROL_C())
        {
            return FBE_STATUS_GENERIC_FAILURE;
        }

       /* Display the fbe packet detail.
        */
       curr_queue_element_p = next_queue_element_p;
       fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
       trace_func(trace_context, "fbe_packet_ptr: 0x%llx\n",
                  (unsigned long long)(curr_queue_element_p - offset));
       fbe_transport_print_fbe_packet(module_name,
                                      curr_queue_element_p - offset,
                                      trace_func,
                                      trace_context,
                                      spaces_to_indent + 3);

       FBE_GET_FIELD_DATA(module_name,
                          curr_queue_element_p,
                          fbe_queue_element_t,
                          next,
                          sizeof(fbe_u64_t),
                          &next_queue_element_p);

       item_count += 1;
       if (item_count > 0xff)
       {
           trace_func(trace_context, "%s queue: %llx is too long\n",
                      __FUNCTION__, (unsigned long long)queue_p);
           break;
       }

    } /* end while (next_queue_element_p != queue_p) */

     return FBE_STATUS_OK;
}
/**********************************************
 * end fbe_transport_print_packet_queue()
 *********************************************/

/*!**************************************************************
 * fbe_transport_get_queue_size()
 ****************************************************************
 * @brief
 *  Compute the size of a queue.
 *
 * @param  module_name - This is the name of the module we are debugging.
 * @param queue_p - pointer to queue.
 * @param trace_func - Function to use when tracing.
 * @param trace_context - The context to be passed to the trace function.
 * @param queue_size - The size of the element in the queue to be returned.
 *
 * @return - FBE_STATUS_OK  
 *
 * HISTORY:
 *  02/13/2009 - Created. Nishit Singh
 *
 ****************************************************************/
fbe_status_t fbe_transport_get_queue_size(const fbe_u8_t * module_name,
                                          fbe_u64_t queue_p,
                                          fbe_trace_func_t trace_func,
                                          fbe_trace_context_t trace_context,
                                          fbe_u32_t *queue_size)
{
    fbe_u32_t item_count = 0;
    fbe_u32_t offset;
    fbe_u64_t curr_queue_element_p, next_queue_element_p;

    FBE_GET_FIELD_DATA(module_name,
                       queue_p,
                       fbe_queue_head_t,
                       next,
                       sizeof(fbe_u64_t),
                       &next_queue_element_p);

    FBE_GET_FIELD_OFFSET(module_name, fbe_packet_t, "queue_element", &offset);

    /* Count the no of queue element. 
     */
    while ((next_queue_element_p != queue_p) && (next_queue_element_p != NULL))
    {
        if (FBE_CHECK_CONTROL_C())
        {
            return FBE_STATUS_GENERIC_FAILURE;
        }

       curr_queue_element_p = next_queue_element_p;
       FBE_GET_FIELD_DATA(module_name,
                          curr_queue_element_p,
                          fbe_queue_element_t,
                          next,
                          sizeof(fbe_u64_t),
                          &next_queue_element_p);
       item_count += 1;
       if (item_count > 0xffff)
       {
           trace_func(trace_context, "%s queue: %llx is too long\n",
                      __FUNCTION__, (unsigned long long)queue_p);
           *queue_size = 0;
           return FBE_STATUS_GENERIC_FAILURE;
           break;
       }
    } /* end while (next_queue_element_p != queue_p) */

    *queue_size = item_count;
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_transport_get_queue_size()
 *****************************************/
/*!**************************************************************
 * fbe_trace_indent
 ****************************************************************
 * @brief
 *   Indent by the given number of spaces.
 *   Note that we use the input trace function so
 *   that can be used in either user or kernel.
 *  
 * @param trace_func - The function to use for displaying/tracing.
 * @param trace_context - Context for the trace fn.
 * @param spaces - # of spaces to indent by.
 *
 * @return - None.
 *
 * HISTORY:
 *  3/2/2009 - Created. RPF
 *
 ****************************************************************/

void fbe_trace_indent(fbe_trace_func_t trace_func,
                      fbe_trace_context_t trace_context,
                      fbe_u32_t spaces)
{
    /* Simply display the input number of spaces.
     */
    while (spaces > 0)
    {
        if (spaces > 100)
        {
            trace_func(trace_context, "%s error spaces to indent is %d\n", __FUNCTION__, spaces);
            return;
        }
        trace_func(trace_context, " ");
        spaces--;
    }
    return;
}
/**************************************
 * end fbe_trace_indent()
 **************************************/

/*!**************************************************************
 * fbe_packet_status_debug_trace()
 ****************************************************************
 * @brief
 *  Display packet status.
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
 *  1/12/2011 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_packet_status_debug_trace(const fbe_u8_t * module_name,
                                                  fbe_dbgext_ptr base_ptr,
                                                  fbe_trace_func_t trace_func,
                                                  fbe_trace_context_t trace_context,
                                                  fbe_debug_field_info_t *field_info_p,
                                                  fbe_u32_t spaces_to_indent)
{
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);

    trace_func(trace_context, "%s: ", field_info_p->name);
    fbe_debug_display_structure(module_name, trace_func, trace_context, base_ptr + field_info_p->offset,
                                &fbe_packet_status_struct_info,
                                4 /* fields per line */,
                                spaces_to_indent);
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_packet_status_debug_trace
 **************************************/

/*!**************************************************************
 * fbe_packet_status_debug_trace_no_field_ptr()
 ****************************************************************
 * @brief
 *  Display packet status.
 *
 * @param module_name - Module name to use in symbol lookup.
 * @param struct_ptr - ptr to value to display.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param spaces_to_indent - Number of spaces to indent the next line(s).
 * 
 * @return fbe_status_t   
 *
 * @author
 *  1/20/2011 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_packet_status_debug_trace_no_field_ptr(const fbe_u8_t * module_name,
                                                        fbe_dbgext_ptr base_ptr,
                                                        fbe_trace_func_t trace_func,
                                                        fbe_trace_context_t trace_context,
                                                        fbe_u32_t spaces_to_indent)
{
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);

    trace_func(trace_context, "packet_status: ");
    fbe_debug_display_structure(module_name, trace_func, trace_context, base_ptr,
                                &fbe_packet_status_struct_info,
                                4 /* fields per line */,
                                spaces_to_indent);
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_packet_status_debug_trace_no_field_ptr
 **************************************/
fbe_status_t fbe_memory_request_state_debug_trace(const fbe_u8_t * module_name,
                                                  fbe_dbgext_ptr base_ptr,
                                                  fbe_trace_func_t trace_func,
                                                  fbe_trace_context_t trace_context,
                                                  fbe_debug_field_info_t *field_info_p,
                                                  fbe_u32_t spaces_to_indent);
/*!*******************************************************************
 * @var fbe_memory_request_field_info
 *********************************************************************
 * @brief Information about each of the fields of raid common.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_memory_request_field_info[] =
{
    /* base config is first.
     */
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("request_state", fbe_memory_request_state_t, FBE_FALSE, "0x%x", fbe_memory_request_state_debug_trace),

    FBE_DEBUG_DECLARE_FIELD_INFO("chunk_size", fbe_memory_number_of_objects_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("number_of_objects", fbe_memory_number_of_objects_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("priority", fbe_memory_request_priority_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("ptr", fbe_memory_ptr_t, FBE_FALSE, "0x%x", fbe_debug_display_pointer),

    FBE_DEBUG_DECLARE_FIELD_INFO_FN("completion_function", fbe_memory_completion_function_t, FBE_FALSE, "0x%x", fbe_debug_display_function_ptr),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("completion_context", fbe_memory_ptr_t, FBE_FALSE, "0x%x", fbe_debug_display_pointer),

    FBE_DEBUG_DECLARE_FIELD_INFO_FN("packet", fbe_memory_ptr_t, FBE_FALSE, "0x%x", fbe_debug_display_pointer),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("memory_io_master", fbe_memory_ptr_t, FBE_FALSE, "0x%x", fbe_debug_display_pointer),

    FBE_DEBUG_DECLARE_FIELD_INFO("flags", fbe_u32_t, FBE_FALSE, "0x%x"),

    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_memory_request_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        raid common.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_memory_request_t,
                              fbe_memory_request_struct_info,
                              &fbe_memory_request_field_info[0]);


/*!**************************************************************
 * fbe_memory_request_state_display()
 ****************************************************************
 * @brief
 *  Display memory request state.
 *
 * @param state - State to display.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function.
 *
 * @return - FBE_STATUS_OK  
 *
 * @date
 *  4/1/2011 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_memory_request_state_display(fbe_memory_request_state_t state,
                                              fbe_trace_func_t trace_func,
                                              fbe_trace_context_t trace_context)
{
    switch(state)
    {
        case FBE_MEMORY_REQUEST_STATE_INVALID:
          trace_func(trace_context, "(invalid)");
          break;
        case FBE_MEMORY_REQUEST_STATE_ABORTED:
          trace_func(trace_context, "(aborted)");
          break;
        case FBE_MEMORY_REQUEST_STATE_ALLOCATE_BUFFER:
          trace_func(trace_context, "(alloc buffer)");
          break;
        case FBE_MEMORY_REQUEST_STATE_ALLOCATE_CHUNK:
          trace_func(trace_context, "(alloc chunk)");
          break;
        case FBE_MEMORY_REQUEST_STATE_ALLOCATE_CHUNK_COMPLETED:
          trace_func(trace_context, "(alloc chunk complete)");
          break;
        case FBE_MEMORY_REQUEST_STATE_ALLOCATE_CHUNK_COMPLETED_IMMEDIATELY:
          trace_func(trace_context, "(alloc chunk complete immediate)");
          break;
        case FBE_MEMORY_REQUEST_STATE_ALLOCATE_OBJECT:
          trace_func(trace_context, "(allocate object)");
          break;
        case FBE_MEMORY_REQUEST_STATE_ALLOCATE_PACKET:
          trace_func(trace_context, "(allocate packet)");
          break;
        case FBE_MEMORY_REQUEST_STATE_DESTROYED:
          trace_func(trace_context, "(destroyed)");
          break;
        case FBE_MEMORY_REQUEST_STATE_INITIALIZED:
          trace_func(trace_context, "(initialized)");
          break;
        case FBE_MEMORY_REQUEST_STATE_ERROR:
          trace_func(trace_context, "(error)");
          break;
        case FBE_MEMORY_REQUEST_STATE_LAST:
          trace_func(trace_context, "(last)");
          break;
        default:
          trace_func(trace_context, "UNKNOWN opcode 0x%x", state);
          break;
    }

    return FBE_STATUS_OK;
}
/********************************************
 * end fbe_memory_request_state_display()
 ********************************************/
/*!**************************************************************
 * fbe_memory_request_state_debug_trace()
 ****************************************************************
 * @brief
 *  Display the memory request's information.
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
 *  4/1/2011 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_memory_request_state_debug_trace(const fbe_u8_t * module_name,
                                                  fbe_dbgext_ptr base_ptr,
                                                  fbe_trace_func_t trace_func,
                                                  fbe_trace_context_t trace_context,
                                                  fbe_debug_field_info_t *field_info_p,
                                                  fbe_u32_t spaces_to_indent)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t state_size;
    fbe_memory_request_state_t state = 0;

	fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
	trace_func(trace_context, "%s:",field_info_p->name);
    FBE_GET_TYPE_SIZE(module_name, fbe_memory_request_state_t, &state_size);
    FBE_READ_MEMORY(base_ptr + field_info_p->offset, &state, state_size);
    fbe_memory_request_state_display(state, trace_func, trace_context);
    return status;
}
/**************************************
 * end fbe_memory_request_state_debug_trace
 **************************************/
/*!**************************************************************
 * fbe_memory_request_debug_trace()
 ****************************************************************
 * @brief
 *  Display the memory request's information.
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
 *  12/15/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_memory_request_debug_trace(const fbe_u8_t * module_name,
                                            fbe_dbgext_ptr base_ptr,
                                            fbe_trace_func_t trace_func,
                                            fbe_trace_context_t trace_context,
                                            fbe_debug_field_info_t *field_info_p,
                                            fbe_u32_t spaces_to_indent)
{
    fbe_status_t status = FBE_STATUS_OK;
	trace_func(trace_context, "%s:\n",field_info_p->name);
    status = fbe_debug_display_structure(module_name, trace_func, trace_context, 
                                         base_ptr + field_info_p->offset,
                                         &fbe_memory_request_struct_info,
                                         4 /* fields per line */,
                                         spaces_to_indent + 2);
    return status;
}
/**************************************
 * end fbe_memory_request_debug_trace
 **************************************/

/*!**************************************************************
 * fbe_memory_request_debug_trace_basic()
 ****************************************************************
 * @brief
 *  Display the memory request's information.
 *
 * @param module_name - Module name to use in symbol lookup.
 * @param base_ptr - ptr to value to display.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param spaces_to_indent - Number of spaces to indent the next line(s).
 * 
 * @return fbe_status_t   
 *
 * @author
 *  4/29/2011 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_memory_request_debug_trace_basic(const fbe_u8_t * module_name,
                                                  fbe_dbgext_ptr base_ptr,
                                                  fbe_trace_func_t trace_func,
                                                  fbe_trace_context_t trace_context,
                                                  fbe_u32_t spaces_to_indent)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    status = fbe_debug_display_structure(module_name, trace_func, trace_context, 
                                         base_ptr,
                                         &fbe_memory_request_struct_info,
                                         4 /* fields per line */,
                                         spaces_to_indent + 2);
    trace_func(trace_context, "\n");
    return status;
}
/**************************************
 * end fbe_memory_request_debug_trace_basic
 **************************************/

/*!**************************************************************
 * fbe_packet_print_state()
 ****************************************************************
 * @brief
 *  Display all the trace information related to packet_state.
 *
 * @param packet_state - payload opcode.
 * @param trace_func - Function to use when tracing.
 * @param trace_context - The context to be passed to the trace function.
 *
 * @return - FBE_STATUS_OK  
 *
 * @author
 *  5/17/2011 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_packet_print_state(fbe_packet_state_t packet_state,
                                    fbe_trace_func_t trace_func,
                                    fbe_trace_context_t trace_context)
{
    switch(packet_state)
    {
        case FBE_PACKET_STATE_INVALID:
          trace_func(trace_context, "(invalid)");
          break;
        case FBE_PACKET_STATE_CANCELED:
          trace_func(trace_context, "(cancelled)");
          break;
        case FBE_PACKET_STATE_IN_PROGRESS:
          trace_func(trace_context, "(in progress)");
          break;
        case FBE_PACKET_STATE_QUEUED:
          trace_func(trace_context, "(queued)");
          break;
        default:
          trace_func(trace_context, "UNKNOWN packet_state 0x%x", packet_state);
          break;
    }
    return FBE_STATUS_OK;
}
/********************************************
 * end fbe_packet_print_state()
 ********************************************/

/*!**************************************************************
 * fbe_packet_state_debug_trace()
 ****************************************************************
 * @brief
 *  Display packet status.
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
 *  5/17/2011 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_packet_state_debug_trace(const fbe_u8_t * module_name,
                                                 fbe_dbgext_ptr base_ptr,
                                                 fbe_trace_func_t trace_func,
                                                 fbe_trace_context_t trace_context,
                                                 fbe_debug_field_info_t *field_info_p,
                                                 fbe_u32_t spaces_to_indent)
{
    fbe_packet_state_t state = 0;

    FBE_READ_MEMORY(base_ptr + field_info_p->offset, &state, sizeof(fbe_packet_state_t));

    trace_func(trace_context, "%s: %d", field_info_p->name, state);
    fbe_packet_print_state(state, trace_func, trace_context);

    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_packet_state_debug_trace
 **************************************/


/*!*******************************************************************
 * @var fbe_packet_debug_sub_packet_summary_queue_info_t
 *********************************************************************
 * @brief This structure holds info used to display a packet
 *        queue using the "subpacket_queue_element" field of the packet.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_QUEUE_INFO(fbe_packet_t, "subpacket_queue_element", 
                             FBE_FALSE, /* queue element is not the first field in the packet. */
                             fbe_packet_debug_sub_packet_summary_queue_info,
                             fbe_transport_print_packet_summary);

/*!*******************************************************************
 * @var fbe_packet_summary_field_info
 *********************************************************************
 * @brief Information about each of the fields of rdgen service.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_packet_summary_field_info[] =
{
    /* Object id is first.
     */
    FBE_DEBUG_DECLARE_FIELD_INFO("object_id", fbe_object_id_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("packet_state", fbe_atomic_t, FBE_FALSE, "0x%x", fbe_packet_state_debug_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO_QUEUE("subpacket_queue_head", fbe_queue_head_t, FBE_FALSE, 
                                       &fbe_packet_debug_sub_packet_summary_queue_info),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_packet_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        fbe_packet_t.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_packet_t,
                              fbe_packet_summary_struct_info,
                              &fbe_packet_summary_field_info[0]);

/*!**************************************************************
 * fbe_debug_display_packet_summary()
 ****************************************************************
 * @brief
 *  Read in the packet ptr and display the packet.
 *
 * @param module_name
 * @param base_ptr
 * @param trace_func
 * @param trace_context
 * @param field_info_p
 * @param spaces_to_indent
 *
 * @author
 *  4/1/2013 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_debug_display_packet_summary(const fbe_u8_t * module_name,
                                              fbe_dbgext_ptr base_ptr,
                                              fbe_trace_func_t trace_func,
                                              fbe_trace_context_t trace_context,
                                              fbe_debug_field_info_t *field_info_p,
                                              fbe_u32_t spaces_to_indent)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u64_t data = 0;
    fbe_u32_t ptr_size;

    status = fbe_debug_get_ptr_size(module_name, &ptr_size);
    if (status != FBE_STATUS_OK) 
    {
        trace_func(trace_context, "unable to get ptr size status: %d\n", status);
        return status; 
    }
    FBE_READ_MEMORY(base_ptr + field_info_p->offset, &data, ptr_size);
    return fbe_transport_print_packet_summary(module_name, data,
                                              trace_func, trace_context, spaces_to_indent);
}
/******************************************
 * end fbe_debug_display_packet_summary()
 ******************************************/
/*!**************************************************************
 * fbe_transport_print_packet_field_summary()
 ****************************************************************
 * @brief
 *  Display a brief summary of the packet information.
 *
 * @param module_name
 * @param fbe_packet_p
 * @param trace_func
 * @param trace_context
 * @param spaces_to_indent
 * 
 * @return fbe_status_t
 *
 * @author
 *  4/4/2013 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_transport_print_packet_field_summary(const fbe_u8_t * module_name,
                                                      fbe_u64_t fbe_packet_p,
                                                      fbe_trace_func_t trace_func,
                                                      fbe_trace_context_t trace_context,
                                                      fbe_u32_t spaces_to_indent)
{
    fbe_status_t status;
    fbe_u32_t payload_offset;
    fbe_u32_t current_level;
    fbe_u32_t stack_offset;
    fbe_u32_t tracker_offset;
    fbe_u32_t tracker_size;
    fbe_u32_t tracker_time_offset;
    fbe_u32_t tracker_time;
    fbe_u32_t tracker_index_offset;
    fbe_u32_t stack_size;
    fbe_u64_t stack_fn = 0;
    fbe_u64_t stack_context = 0;
    fbe_u32_t ptr_size;
    fbe_u64_t magic_number;
    fbe_time_t current_time = 0;
    fbe_u8_t tracker_index;
    fbe_u32_t current_coarse_time;

    status = fbe_debug_get_ptr_size(module_name, &ptr_size);
    if (status != FBE_STATUS_OK) 
    {
        trace_func(trace_context, "%s unable to get ptr size status: %d\n", __FUNCTION__, status);
        return status; 
    }
    
    fbe_debug_get_idle_thread_time(module_name, &current_time);
    current_coarse_time = (fbe_u32_t)current_time;

    FBE_GET_FIELD_OFFSET(module_name, fbe_packet_t, "payload_union", &payload_offset);
    FBE_GET_FIELD_OFFSET(module_name, fbe_packet_t, "stack", &stack_offset);
    FBE_GET_FIELD_OFFSET(module_name, fbe_packet_t, "tracker_index", &tracker_index_offset);
    FBE_GET_FIELD_OFFSET(module_name, fbe_packet_t, "tracker_ring", &tracker_offset);
    FBE_GET_FIELD_OFFSET(module_name, fbe_packet_tracker_entry_t, "coarse_time", &tracker_time_offset);

    FBE_GET_TYPE_SIZE(module_name, fbe_packet_stack_t, &stack_size);
    FBE_GET_TYPE_SIZE(module_name, fbe_packet_tracker_entry_t, &tracker_size);

    FBE_GET_FIELD_DATA(module_name, 
                       fbe_packet_p,
                       fbe_packet_t,
                       magic_number,
                       sizeof(fbe_u64_t),
                       &magic_number);

    FBE_GET_FIELD_DATA(module_name, 
                       fbe_packet_p,
                       fbe_packet_t,
                       current_level,
                       sizeof(fbe_u32_t),
                       &current_level);

    FBE_READ_MEMORY(fbe_packet_p + stack_offset + (stack_size * current_level), &stack_fn, ptr_size);
    FBE_READ_MEMORY(fbe_packet_p + stack_offset + (stack_size * current_level) + ptr_size, &stack_context, ptr_size);

    FBE_READ_MEMORY(fbe_packet_p + tracker_index_offset + current_level, &tracker_index, sizeof(fbe_u8_t));
    FBE_READ_MEMORY(fbe_packet_p + tracker_offset + (tracker_index * tracker_size) + tracker_time_offset, &tracker_time, sizeof(fbe_u32_t));

    trace_func(trace_context, "callback: ");
    fbe_debug_display_function_ptr_symbol(module_name, stack_fn, trace_func, trace_context, spaces_to_indent);
    trace_func(trace_context, " ");
    trace_func(trace_context, "(%d seconds) ", (current_coarse_time - tracker_time)/1000);
    status = fbe_debug_display_structure(module_name, trace_func, trace_context, fbe_packet_p,
                                         &fbe_packet_summary_struct_info,
                                         4 /* fields per line */,
                                         spaces_to_indent + 2);
    return status;
}
/******************************************
 * end fbe_transport_print_packet_field_summary()
 ******************************************/
/*!**************************************************************
 * fbe_transport_print_packet_summary()
 ****************************************************************
 * @brief
 *  Display a brief summary of the packet information.
 *
 * @param module_name
 * @param fbe_packet_p
 * @param trace_func
 * @param trace_context
 * @param spaces_to_indent
 * 
 * @return fbe_status_t
 *
 * @author
 *  4/1/2013 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_transport_print_packet_summary(const fbe_u8_t * module_name,
                                                fbe_u64_t packet_p,
                                                fbe_trace_func_t trace_func,
                                                fbe_trace_context_t trace_context,
                                                fbe_u32_t spaces_to_indent)
{
    fbe_status_t status;
    fbe_u32_t iots_payload_offset;
    fbe_u32_t payload_offset;

    FBE_GET_FIELD_OFFSET(module_name, fbe_packet_t, "payload_union", &payload_offset);
    FBE_GET_FIELD_OFFSET(module_name, fbe_payload_ex_t, "iots", &iots_payload_offset);

    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    trace_func(trace_context, "packet: 0x%llx ", packet_p);
    status = fbe_transport_print_packet_field_summary(module_name, packet_p,
                                                      trace_func, trace_context, 0);

    if (fbe_transport_should_display_payload_iots(module_name, packet_p, packet_p + payload_offset) ==
        FBE_STATUS_OK)
    {
        fbe_raid_library_debug_print_iots_summary(module_name, packet_p + iots_payload_offset + payload_offset,
                                                  trace_func, trace_context, spaces_to_indent + 2);
    }
    return status;
}
/******************************************
 * end fbe_transport_print_packet_summary()
 ******************************************/

/*************************
 * end file fbe_packet_debug.c
 *************************/
