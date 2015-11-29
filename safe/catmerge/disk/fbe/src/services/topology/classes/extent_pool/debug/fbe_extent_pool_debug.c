/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rightsche reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_extent_pool_debug.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the debug functions for the extent pool object.
 *
 * @author
 *  06/27/2014 - Created.
 *
 ***************************************************************************/
 
/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_types.h"
#include "fbe/fbe_dbgext.h"
#include "fbe/fbe_topology_interface.h"
#include "fbe_base_object.h"
#include "fbe_base_object_debug.h"
#include "fbe_raid_group_object.h"
#include "fbe_base_object_trace.h"
#include "fbe_transport_debug.h"
#include "fbe_raid_library_debug.h"
#include "fbe_base_config_debug.h"
#include "fbe_raid_group_debug.h"
#include "fbe/fbe_payload_metadata_operation.h"
#include "fbe_metadata_debug.h"
#include "fbe_topology_debug.h"
#include "fbe_extent_pool_debug.h"
#include "fbe_extent_pool_private.h"


/*************************
 *   FUNCTION DEFINITIONS
 *************************/


/*!*******************************************************************
 * @var fbe_extent_pool_field_info
 *********************************************************************
 * @brief Information about each of the fields of base config.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_extent_pool_field_info[] =
{
    /* base config is first.
     */
    FBE_DEBUG_DECLARE_FIELD_INFO("pool_id", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("pool_width", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("total_slices", fbe_slice_count_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("base_config", fbe_object_id_t, FBE_TRUE, "0x%x",
                                    fbe_base_config_debug_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_extent_pool_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        base config.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_extent_pool_t,
                              fbe_extent_pool_struct_info,
                              &fbe_extent_pool_field_info[0]);

/*!**************************************************************
 * fbe_extent_pool_debug_trace()
 ****************************************************************
 * @brief
 *  Display all the trace information about the extent pool object.
 *
 * @param  module_name - This is the name of the module we are debugging.
 * @param extent_pool_p - Ptr to extent pool object.
 * @param trace_func - Function to use when tracing.
 * @param trace_context - The context to be passed to the trace function.
 * @param spaces_to_indent - Spaces to indent this display.
 *
 * @return - FBE_STATUS_OK  
 *
 ****************************************************************/

fbe_status_t fbe_extent_pool_debug_trace(const fbe_u8_t * module_name,
                                        fbe_dbgext_ptr extent_pool_p,
                                        fbe_trace_func_t trace_func,
                                        fbe_trace_context_t trace_context,
                                        fbe_debug_field_info_t *field_info_p,
                                        fbe_u32_t overall_spaces_to_indent)
{
    fbe_status_t status;
    fbe_u32_t spaces_to_indent = overall_spaces_to_indent + 2;

    if (field_info_p != NULL)
    {
        /* If field info is provided, the ptr passed in is a ptr to the base structure. 
         * We need to add the offset to get to the actual extent_pool ptr.
         */
        extent_pool_p += field_info_p->offset;
    }
    
    status = fbe_debug_display_structure(module_name, trace_func, trace_context, extent_pool_p,
                                         &fbe_extent_pool_struct_info,
                                         4 /* fields per line */,
                                         spaces_to_indent + 2);
    trace_func(trace_context, "\n");
    return status;
} 
/**************************************
 * end fbe_extent_pool_debug_trace()
 **************************************/

/*!**************************************************************
 * fbe_extent_pool_debug_display_terminator_queue()
 ****************************************************************
 * @brief
 *  Display the extent pool siots structure.
 *  
 * @param module_name - Module name to use in symbol lookup.
 * @param extent_pool_p - ptr to extent_pool to display.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param spaces_to_indent - Spaces to indent this display.
 * @param b_summarize - TRUE to only print a summary of I/Os.
 * 
 * @return fbe_status_t
 *
 * @author
 *
 ****************************************************************/
fbe_status_t fbe_extent_pool_debug_display_terminator_queue(const fbe_u8_t * module_name,
                                                           fbe_dbgext_ptr extent_pool_p,
                                                           fbe_trace_func_t trace_func,
                                                           fbe_trace_context_t trace_context,
                                                           fbe_u32_t spaces_to_indent,
                                                           fbe_bool_t b_summarize)
{
    fbe_u32_t terminator_queue_offset;
    fbe_u64_t terminator_queue_head_ptr;
    fbe_u64_t queue_head_p = 0;
    fbe_u64_t queue_element_p = 0;
    fbe_u64_t next_queue_element_p = 0;
    fbe_u32_t packet_queue_element_offset = 0;
    fbe_u32_t payload_offset;
    fbe_u32_t iots_offset;
    fbe_u32_t ptr_size;

    if (b_summarize)
    {
        fbe_debug_set_display_queue_header(FBE_FALSE);
    }
    FBE_GET_TYPE_SIZE(module_name, fbe_u32_t*, &ptr_size);

    FBE_GET_FIELD_OFFSET(module_name, fbe_packet_t, "payload_union", &payload_offset);
    FBE_GET_FIELD_OFFSET(module_name, fbe_payload_ex_t, "iots", &iots_offset);

    FBE_GET_FIELD_OFFSET(module_name, fbe_packet_t, "queue_element", &packet_queue_element_offset);
    FBE_GET_FIELD_OFFSET(module_name, fbe_base_object_t, "terminator_queue_head", &terminator_queue_offset);
    terminator_queue_head_ptr = extent_pool_p + terminator_queue_offset;

    FBE_READ_MEMORY(extent_pool_p + terminator_queue_offset, &queue_head_p, ptr_size);
    queue_element_p = queue_head_p;

    {
        fbe_u32_t item_count = 0;
        fbe_transport_get_queue_size(module_name,
                                     extent_pool_p + terminator_queue_offset,
                                     trace_func,
                                     trace_context,
                                     &item_count);
        fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
        if (item_count == 0)
        {
            trace_func(trace_context, "terminator queue: EMPTY\n");
        }
        else
        {
            trace_func(trace_context, "terminator queue: (%d items)\n",
                       item_count);
        }
    }

    /* Loop over all entries on the terminator queue, displaying each packet and iots. 
     * We stop when we reach the end (head) or NULL. 
     */
    while ((queue_element_p != terminator_queue_head_ptr) && (queue_element_p != NULL))
    {
        fbe_u64_t packet_p = queue_element_p - packet_queue_element_offset;

        if (FBE_CHECK_CONTROL_C())
        {
            return FBE_STATUS_CANCELED;
        }

        /* Display packet.
         */
        if (b_summarize)
        {
            fbe_transport_print_packet_summary(module_name, packet_p, trace_func, trace_context, spaces_to_indent + 2);
        }
        else
        {
            fbe_transport_print_fbe_packet(module_name, packet_p, trace_func, trace_context, spaces_to_indent + 2);
        }

        FBE_READ_MEMORY(queue_element_p, &next_queue_element_p, ptr_size);
        queue_element_p = next_queue_element_p;
    }
    
    if (b_summarize)
    {
        fbe_debug_set_display_queue_header(FBE_TRUE);
    }
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_extent_pool_debug_display_terminator_queue()
 ******************************************/

/*************************
 * end file fbe_extent_pool_debug.c
 *************************/
