/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_lun_debug.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the debug functions for the lun object.
 *
 * @author
 *  30/11/2010 - Created. Omer Miranda
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_dbgext.h"
#include "fbe_base_object.h"
#include "fbe_base_object_debug.h"
#include "fbe_lun_private.h"
#include "fbe_base_object_trace.h"
#include "fbe_lun_debug.h"
#include "fbe_transport_debug.h"
#include "fbe/fbe_raid_group.h"
#include "fbe_raid_geometry.h"
#include "fbe_base_config_debug.h"
#include "fbe_raid_library_debug.h"

#include "fbe_topology_debug.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/*!*******************************************************************
 * @var fbe_lun_object_metadata_memory_field_info
 *********************************************************************
 * @brief Information about each of the fields of lun object
 *        metadata memeory.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_lun_object_metadata_memory_field_info[] =
{
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("base_config_metadata_memory", fbe_base_config_metadata_memory_t, FBE_TRUE, "0x%x",
                                    fbe_base_config_metadata_memory_debug_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO("flags", fbe_lun_clustered_flags_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_lun_object_metadata_memory_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        lun object metadata memory.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_lun_metadata_memory_t,
                              fbe_lun_object_metadata_memory_struct_info,
                              &fbe_lun_object_metadata_memory_field_info[0]);

/*!**************************************************************
 * fbe_lun_object_metadata_memory_debug_trace()
 ****************************************************************
 * @brief
 *  Display the lun object metadata memeory.
 *
 * @param module_name - Module name to use in symbol lookup.
 * @param object_ptr - ptr to value to display.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param spaces_to_indent - Number of spaces to indent the next line(s).
 * 
 * @return fbe_status_t   
 *
 * @author
 *  01/31/2011 - Created Omer Miranda
 *
 ****************************************************************/
fbe_status_t fbe_lun_object_metadata_memory_debug_trace(const fbe_u8_t * module_name,
                                                        fbe_dbgext_ptr record_ptr,
                                                        fbe_trace_func_t trace_func,
                                                        fbe_trace_context_t trace_context,
                                                        fbe_u32_t spaces_to_indent)
{
    fbe_status_t status = FBE_STATUS_OK;
    trace_func(trace_context, "\n");
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);	
    trace_func(trace_context, "lun_object_metadata_memory: \n");
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent + 4);
    status = fbe_debug_display_structure(module_name, trace_func, trace_context, record_ptr,
                                         &fbe_lun_object_metadata_memory_struct_info,
                                         4 /* fields per line */,
                                         spaces_to_indent + 4);
    return FBE_STATUS_OK;
}

/*!*******************************************************************
 * @var fbe_lun_nonpaged_metadata_field_info
 *********************************************************************
 * @brief Information about each of the fields of lun object
 *        non-paged metadata record.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_lun_nonpaged_metadata_field_info[] =
{
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("base_config_nonpaged_metadata", fbe_base_config_nonpaged_metadata_t, FBE_TRUE, "0x%x", 
                                    fbe_base_config_nonpaged_metadata_debug_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO("flags", fbe_lun_nonpaged_flags_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_lun_nonpaged_metadata_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        lun object non paged metadata record.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_lun_nonpaged_metadata_t,
                              fbe_lun_nonpaged_metadata_struct_info,
                              &fbe_lun_nonpaged_metadata_field_info[0]);
/*!**************************************************************
 * fbe_lun_non_paged_record_debug_trace()
 ****************************************************************
 * @brief
 *  Display the lun object non-paged record.
 *
 * @param module_name - Module name to use in symbol lookup.
 * @param object_ptr - ptr to value to display.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param spaces_to_indent - Number of spaces to indent the next line(s).
 * 
 * @return fbe_status_t   
 *
 * @author
 *  01/28/2011 - Created Omer Miranda
 *
 ****************************************************************/
fbe_status_t fbe_lun_non_paged_record_debug_trace(const fbe_u8_t * module_name,
                                                  fbe_dbgext_ptr record_ptr,
                                                  fbe_trace_func_t trace_func,
                                                  fbe_trace_context_t trace_context,
                                                  fbe_u32_t spaces_to_indent)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    trace_func(trace_context, "lun_non_paged_metadata: \n");
    status = fbe_debug_display_structure(module_name, trace_func, trace_context, record_ptr,
                                         &fbe_lun_nonpaged_metadata_struct_info,
                                         4 /* fields per line */,
                                         spaces_to_indent +4);
    return FBE_STATUS_OK;
}

/*!*******************************************************************
 * @var fbe_lun_object_field_info
 *********************************************************************
 * @brief Information about each of the fields of base config.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_lun_object_field_info[] =
{
    /* base config is first.
     */
    FBE_DEBUG_DECLARE_FIELD_INFO("dirty_pending", fbe_bool_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("clean_pending", fbe_bool_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO("power_save_io_drain_delay_in_sec", fbe_u64_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("max_lun_latency_time_is_sec", fbe_u64_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("wait_for_power_save", fbe_u64_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO("io_counter", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("ndb_b", fbe_bool_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("dirty_pending", fbe_bool_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("clean_pending", fbe_bool_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("marked_dirty", fbe_bool_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("clean_time", fbe_time_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("dirty_flags_start_lba", fbe_lba_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("b_perf_stats_enabled", fbe_bool_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("lun_attributes", fbe_lun_attributes_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("noinitialverify_b", fbe_bool_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("clear_need_zero_b", fbe_bool_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("write_bypass_mode", fbe_bool_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("last_rebuild_percent", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("num_unexpected_errors", fbe_atomic_32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("zero_update_interval_count", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("zero_update_count_to_update", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("base_config", fbe_object_id_t, FBE_TRUE, "0x%x",
                                    fbe_base_config_debug_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_lun_object_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        base config.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_lun_t,
                              fbe_lun_object_struct_info,
                              &fbe_lun_object_field_info[0]);

/*!**************************************************************
 * @fn fbe_lun_debug_trace(const fbe_u8_t * module_name,
 *                                   fbe_dbgext_ptr lun_object_ptr,
 *                                   fbe_trace_func_t trace_func,
 *                                   fbe_trace_context_t trace_context)
 ****************************************************************
 * @brief
 *  Display all the trace information about the lun object.
 *
 * @param  module_name - This is the name of the module we are debugging.
 * @param logical_drive_p - Ptr to lun object.
 * @param trace_func - Function to use when tracing.
 * @param trace_context - The context to be passed to the trace function.
 *
 * @return - FBE_STATUS_OK  
 *
 * HISTORY:
 *  12/2/2010 - Created. Omer Miranda
 *
 ****************************************************************/
fbe_status_t fbe_lun_debug_trace(const fbe_u8_t * module_name,
                                 fbe_dbgext_ptr lun_object_ptr,
                                 fbe_trace_func_t trace_func,
                                 fbe_trace_context_t trace_context,
                                 fbe_u32_t overall_spaces_to_indent)
{
    fbe_status_t status;
    fbe_u32_t spaces_to_indent = overall_spaces_to_indent + 2;

    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    status = fbe_debug_display_structure(module_name, trace_func, trace_context, lun_object_ptr,
                                         &fbe_lun_object_struct_info,
                                         4 /* fields per line */,
                                         spaces_to_indent + 2);
    trace_func(trace_context, "\n");
    return status;
}
/**************************************
 * end fbe_lun_debug_trace()
 **************************************/

/*!**************************************************************
 * fbe_lun_debug_display_terminator_queue()
 ****************************************************************
 * @brief
 *  Display the lun terminator queue.
 *  
 * @param module_name - Module name to use in symbol lookup.
 * @param raid_group_p - ptr to lun object.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param spaces_to_indent - Spaces to indent this display.
 * 
 * @return fbe_status_t
 *
 * @author
 *  12/2/2010 - Created. Omer Miranda
 *
 ****************************************************************/
fbe_status_t fbe_lun_debug_display_terminator_queue(const fbe_u8_t * module_name,
                                                    fbe_dbgext_ptr lun_object_p,
                                                    fbe_trace_func_t trace_func,
                                                    fbe_trace_context_t trace_context,
                                                    fbe_u32_t spaces_to_indent)
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

    FBE_GET_TYPE_SIZE(module_name, fbe_u32_t*, &ptr_size);

    FBE_GET_FIELD_OFFSET(module_name, fbe_packet_t, "payload_union", &payload_offset);
    FBE_GET_FIELD_OFFSET(module_name, fbe_payload_ex_t, "iots", &iots_offset);

    FBE_GET_FIELD_OFFSET(module_name, fbe_packet_t, "queue_element", &packet_queue_element_offset);
    FBE_GET_FIELD_OFFSET(module_name, fbe_base_object_t, "terminator_queue_head", &terminator_queue_offset);
    terminator_queue_head_ptr = lun_object_p + terminator_queue_offset;

    FBE_READ_MEMORY(lun_object_p + terminator_queue_offset, &queue_head_p, ptr_size);
    queue_element_p = queue_head_p;

    {
        fbe_u32_t item_count = 0;
        fbe_transport_get_queue_size(module_name,
                                     lun_object_p + terminator_queue_offset,
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
        fbe_u64_t iots_p = packet_p + payload_offset + iots_offset;

        if (FBE_CHECK_CONTROL_C())
        {
            return FBE_STATUS_CANCELED;
        }

        /* Display packet.
         */
        fbe_transport_print_fbe_packet(module_name, packet_p, trace_func, trace_context, spaces_to_indent + 2);

        /* Display the iots.
         */
        fbe_raid_library_debug_print_iots(module_name, iots_p, trace_func, trace_context, spaces_to_indent + 2);
        FBE_READ_MEMORY(queue_element_p, &next_queue_element_p, ptr_size);
        queue_element_p = next_queue_element_p;
    }
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_lun_debug_display_terminator_queue()
 ******************************************/

/*************************
 * end file fbe_lun_debug.c
 *************************/

