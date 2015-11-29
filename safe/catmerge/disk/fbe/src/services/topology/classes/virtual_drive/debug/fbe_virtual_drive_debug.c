/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_virtual_drive_debug.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the debug functions for the virtual drive object.
 *
 * @author
 *  04/09/2010 - Created. Prafull Parab
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
#include "fbe/fbe_virtual_drive.h"
#include "fbe_virtual_drive_private.h"
#include "fbe_base_object_trace.h"
#include "fbe_virtual_drive_debug.h"
#include "fbe_transport_debug.h"
#include "fbe_raid_library_debug.h"
#include "fbe_base_config_debug.h"
#include "fbe_topology_debug.h"
#include "fbe_raid_group_debug.h"


/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/*!*******************************************************************
 * @var fbe_virtual_drive_field_info
 *********************************************************************
 * @brief Information about each of the fields of base config.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_virtual_drive_field_info[] =
{
    /* base config is first.
     */
    FBE_DEBUG_DECLARE_FIELD_INFO("flags", fbe_virtual_drive_flags_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("configuration_mode", fbe_virtual_drive_configuration_mode_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("new_configuration_mode", fbe_virtual_drive_configuration_mode_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("swap_in_edge_index", fbe_edge_index_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("swap_out_edge_index", fbe_edge_index_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("need_replacement_drive_start_time", fbe_u64_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("copy_request_type", fbe_spare_swap_command_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("orig_pvd_obj_id", fbe_object_id_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("unused_drive_as_spare", fbe_virtual_drive_unused_drive_as_spare_flag_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("debug_flags", fbe_virtual_drive_debug_flags_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("raid_group", fbe_raid_group_t, FBE_TRUE, "0x%x",
                                    fbe_raid_group_debug_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_virtual_drive_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        base config.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_virtual_drive_t,
                              fbe_virtual_drive_struct_info,
                              &fbe_virtual_drive_field_info[0]);

/*************************
 *   FUNCTION DEFINITIONS
 *************************/
char * fbe_convert_config_mode_to_string(fbe_virtual_drive_configuration_mode_t config_mode);


/*!**************************************************************
 * @fn fbe_virtual_drive_debug_trace(const fbe_u8_t * module_name,
 *                                   fbe_dbgext_ptr virtual_drive,
 *                                   fbe_trace_func_t trace_func,
 *                                   fbe_trace_context_t trace_context)
 ****************************************************************
 * @brief
 *  Display all the trace information about the virtual drive object.
 *
 * @param  module_name - This is the name of the module we are debugging.
 * @param virtual_drive_p - Ptr to virtual drive object.
 * @param trace_func - Function to use when tracing.
 * @param trace_context - The context to be passed to the trace function.
 *
 * @return - FBE_STATUS_OK  
 *
 * HISTORY:
 *  04/26/2010 - Created. RPF
 *  04/26/2010 - Modified to display virtual drive detail. Prafull Parab
 *
 ****************************************************************/

fbe_status_t fbe_virtual_drive_debug_trace(const fbe_u8_t * module_name,
                                           fbe_dbgext_ptr virtual_drive_p,
                                           fbe_trace_func_t trace_func,
                                           fbe_trace_context_t trace_context,
                                           fbe_u32_t overall_spaces_to_indent)
{
    fbe_status_t status;
    fbe_u32_t spaces_to_indent = overall_spaces_to_indent + 2;

    status = fbe_debug_display_structure(module_name, trace_func, trace_context, virtual_drive_p,
                                         &fbe_virtual_drive_struct_info,
                                         4 /* fields per line */,
                                         spaces_to_indent + 2);
    trace_func(trace_context, "\n");
    return status;
} 
/**************************************
 * end fbe_virtual_drive_debug_trace()
 **************************************/

char * fbe_convert_config_mode_to_string(fbe_virtual_drive_configuration_mode_t configuration_mode)
{
    char *configuration_mode_String;

    switch(configuration_mode)
    {
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE:
            configuration_mode_String = "FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE";
            break;

        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE:
            configuration_mode_String = "FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE";
            break;

        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE:
            configuration_mode_String = "FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE";
            break;
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE:
            configuration_mode_String = "FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE";
            break;
        default:
            configuration_mode_String = "Unknown config_mode";
            break;
    }

    return(configuration_mode_String);
}

/*!**************************************************************
 * fbe_virtual_drive_debug_display_terminator_queue()
 ****************************************************************
 * @brief
 *  Display the virtual drive  terminator queue.
 *  
 * @param module_name - Module name to use in symbol lookup.
 * @param raid_group_p - ptr to virtual drive to display.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param spaces_to_indent - Spaces to indent this display.
 * 
 * @return fbe_status_t
 *
 * @author
 *  10/23/2009 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_virtual_drive_debug_display_terminator_queue(const fbe_u8_t * module_name,
                                                                  fbe_dbgext_ptr virtual_drive_p,
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
    fbe_virtual_drive_configuration_mode_t configuration_mode;

    FBE_GET_TYPE_SIZE(module_name, fbe_u32_t*, &ptr_size);

    FBE_GET_FIELD_OFFSET(module_name, fbe_packet_t, "payload_union", &payload_offset);
    FBE_GET_FIELD_OFFSET(module_name, fbe_payload_ex_t, "iots", &iots_offset);

    FBE_GET_FIELD_OFFSET(module_name, fbe_packet_t, "queue_element", &packet_queue_element_offset);
    FBE_GET_FIELD_OFFSET(module_name, fbe_base_object_t, "terminator_queue_head", &terminator_queue_offset);
    terminator_queue_head_ptr = virtual_drive_p + terminator_queue_offset;

    FBE_READ_MEMORY(virtual_drive_p + terminator_queue_offset, &queue_head_p, ptr_size);
    queue_element_p = queue_head_p;

    {
        fbe_u32_t item_count = 0;
        fbe_transport_get_queue_size(module_name,
                                     virtual_drive_p + terminator_queue_offset,
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

        FBE_GET_FIELD_DATA(module_name, 
                   virtual_drive_p,
                   fbe_virtual_drive_t,
                   configuration_mode,
                   sizeof(fbe_virtual_drive_configuration_mode_t),
                   &configuration_mode); 
        
        /* Display packet. */
        fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
        trace_func(trace_context, "packet: 0x%llx\n",
	           (unsigned long long)packet_p);
        fbe_transport_print_fbe_packet(module_name, packet_p, trace_func, trace_context, spaces_to_indent + 2);
        if((configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE) ||
           (configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE))
        {
            /* Display the iots. */
            fbe_raid_library_debug_print_iots(module_name, iots_p, trace_func, trace_context, spaces_to_indent + 2);
       
        }

        FBE_READ_MEMORY(queue_element_p, &next_queue_element_p, ptr_size);
        queue_element_p = next_queue_element_p;

   }
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_virtual_drive_debug_display_terminator_queue()
 ******************************************/



/*************************
 * end file fbe_virtual_drive_debug.c
 *************************/
