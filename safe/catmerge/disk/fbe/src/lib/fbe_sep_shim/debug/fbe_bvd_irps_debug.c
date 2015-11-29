/***************************************************************************
 * Copyright (C) EMC Corporation 2010-2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_bvd_irps_debug.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the debug functions for the sep shim io structure.
 *
 * @author
 *  16- Sep- 2011 - Created. Omer Miranda
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_types.h"
#include "fbe/fbe_dbgext.h"
#include "fbe_sep_shim_debug.h"
#include "fbe_sep_shim_private_interface.h"
#include "fbe/fbe_multicore_queue.h"
#include "fbe_transport_debug.h"
#include "pp_ext.h"
#include "fbe_raid_library_debug.h"

/*******************************************
                Local functions
********************************************/

fbe_status_t fbe_sep_shim_io_struct_summary_debug_trace(const fbe_u8_t * module_name,
                                                        fbe_dbgext_ptr io_data_ptr,
                                                        fbe_trace_func_t trace_func,
                                                        fbe_trace_context_t trace_context,
                                                        fbe_u32_t check_time_limit,
                                                        fbe_bool_t b_list);
/*!**************************************************************
 * fbe_sep_shim_io_struct_queue_debug()
 ****************************************************************
 * @brief
 *  Processes the IO queue
 *
 * @param module_name - Module name to use in symbol lookup.
 * @param io_queue_head_ptr - The queue's HEAD pointer.
 * @param verbose_display - Flag to indicate the type of display.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 *
 * @return fbe_status_t
 *
 * @author
 *  16- Sep- 2011 - Created. Omer Miranda
 *
 ****************************************************************/

fbe_object_id_t fbe_sep_shim_io_struct_queue_debug(const fbe_u8_t * module_name,
                                                fbe_dbgext_ptr io_queue_head_ptr,
                                                fbe_bool_t verbose_display,
                                                fbe_bool_t b_summary,
                                                fbe_bool_t b_list,
												fbe_u32_t check_time_limit,
                                                fbe_trace_func_t trace_func,
                                                fbe_trace_context_t trace_context)
{
    fbe_u32_t next_offset;
    fbe_u32_t prev_offset;
    fbe_dbgext_ptr head = 0;
    fbe_multicore_queue_entry_t queue_entry = {0};
    fbe_dbgext_ptr next_io = 0;
    fbe_dbgext_ptr prev_io = 0;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t io_count = 1;
    fbe_u32_t ptr_size;
    fbe_dbgext_ptr next_io_ptr = 0;
    fbe_object_id_t print_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t packet_object_id = FBE_OBJECT_ID_INVALID;
    fbe_u32_t print_packet_count = 0;

    fbe_u32_t retry_count = 0;

   /* Do not display IO queue if active queue display is disabled using
     * '!disable_active_queue_display' debug macro
     */
     if(get_active_queue_display_flag() == FBE_FALSE)
    {
        return print_object_id;
    }
    if(io_queue_head_ptr == NULL)
    {
        trace_func(NULL, "\t IO Queue head ptr not available \n");
        return print_object_id;
    }

    status = fbe_debug_get_ptr_size(module_name, &ptr_size);
    if (status != FBE_STATUS_OK)
    {
        trace_func(NULL, "%s unable to get ptr size status: %d\n", __FUNCTION__, status);
        return print_object_id;
    }


    /* Get the HEAD's next and prev entries*/
    FBE_GET_FIELD_OFFSET(module_name, fbe_queue_head_t, "next", &next_offset);
    FBE_GET_FIELD_OFFSET(module_name, fbe_queue_head_t, "prev", &prev_offset);

    FBE_READ_MEMORY(io_queue_head_ptr, &queue_entry, sizeof(fbe_multicore_queue_entry_t));

    FBE_READ_MEMORY(io_queue_head_ptr + next_offset, &next_io, ptr_size);
    FBE_READ_MEMORY(io_queue_head_ptr + prev_offset, &prev_io, ptr_size);

    /* If Head's next and prev is pointing to itself, then its an empty queue */
    if(next_io == io_queue_head_ptr && prev_io == io_queue_head_ptr)
    {
        fbe_trace_indent(trace_func, NULL, 4);
        trace_func(NULL, "IO queue is EMPTY for this core\n");
        return print_object_id;
    }

    head = io_queue_head_ptr;		/* save for end of queue condition */
    next_io_ptr = next_io;

    while (next_io_ptr != head)
    {
        if (FBE_CHECK_CONTROL_C())
        {
            return print_object_id;
        }

        /* If retry count is greater than 10 I/Os, then just break out. */
        if(retry_count > 10)
        {
            break;
        }
        if (b_summary || b_list)
        {
            if (next_io_ptr != NULL)
            {
                fbe_sep_shim_io_struct_summary_debug_trace(module_name, next_io_ptr,
                                                           trace_func, trace_context,
                                                           check_time_limit, b_list);
                FBE_READ_MEMORY(next_io + next_offset , &next_io_ptr, ptr_size);
            }
            else
            {
                retry_count++;
                trace_func(NULL, "\t Next IO Ptr is NULL; Retry count: %d\n", retry_count);
                status = FBE_STATUS_GENERIC_FAILURE;
            }
        }
        else
        {
            trace_func(NULL, "\n");
            fbe_trace_indent(trace_func, NULL, 4);
            trace_func(NULL, "IO number:%d\n",io_count++);

            if (next_io_ptr != NULL)
            {
                packet_object_id = fbe_sep_shim_io_struct_debug_trace(module_name,next_io_ptr,verbose_display,check_time_limit,trace_func,trace_context);

                /* return valid object id meaning we've printed the packet */
                if (packet_object_id != FBE_OBJECT_ID_INVALID)
                {
                    print_packet_count ++;
                    /* we are limiting to displaying 3 packets with long delay, if not verbose mode */
                    if ((print_packet_count> 3) && !verbose_display)
                    {
                        check_time_limit = 0;
                    }
                    if (print_object_id == FBE_OBJECT_ID_INVALID)
                    {
                        print_object_id = packet_object_id;
                    }
                }

                FBE_READ_MEMORY(next_io + next_offset , &next_io_ptr, ptr_size);

                if(next_io_ptr == NULL)
                {
                    retry_count++;
                    trace_func(NULL, "\t Next IO Ptr is NULL; Retry count: %d\n", retry_count);
                    status = FBE_STATUS_GENERIC_FAILURE;
                }
            }
            else
            {
                retry_count++;
                trace_func(NULL, "\t Next IO Ptr is NULL; Retry count: %d\n", retry_count);
                status = FBE_STATUS_GENERIC_FAILURE;
            }
        }

        next_io = next_io_ptr;
    }


    trace_func(NULL, "\n");

    return print_object_id;
}
/*****************************************
 * end fbe_sep_shim_io_struct_queue_debug
 *****************************************/

/*!*******************************************************************
 * @var fbe_sep_shim_io_struct_field_info
 *********************************************************************
 * @brief Information about each of the fields of sep_shim_io .
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_sep_shim_io_struct_field_info_main[] =
{
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("associated_io", fbe_u64_t,FBE_FALSE,"0x%x",
                                     fbe_debug_display_pointer),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("associated_device", fbe_u64_t, FBE_FALSE, "0x%x",
                                     fbe_debug_display_pointer),
    FBE_DEBUG_DECLARE_FIELD_INFO("core_take_from", fbe_cpu_id_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("memory_request", fbe_memory_request_t, FBE_FALSE, "0x%x",
                                    fbe_memory_request_debug_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("buffer", fbe_memory_request_t, FBE_FALSE, "0x%x",
                                     fbe_memory_request_debug_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("context", fbe_u64_t,FBE_FALSE,"0x%x",
                                     fbe_debug_display_pointer),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("xor_mem_move_ptr", fbe_u64_t,FBE_FALSE,"0x%x",
                                     fbe_debug_display_pointer),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("packet", fbe_packet_t, FBE_FALSE, "0x%x",
                                     fbe_debug_display_pointer),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_sep_shim_io_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the IRPs
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_sep_shim_io_struct_t,
                              fbe_sep_shim_io_struct_info_main,
                              &fbe_sep_shim_io_struct_field_info_main[0]);

/*!**************************************************************
 * fbe_sep_shim_io_struct_debug_trace()
 ****************************************************************
 * @brief
 *  Display the IO queue entries
 *
 * @param module_name - Module name to use in symbol lookup.
 * @param io_data_ptr - The IO queue element's pointer.
 * @param verbose_display - Flag to indicate the type of display.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 *
 * @return object id recorded by the packet
 *
 * @author
 *  16- Sep- 2011 - Created. Omer Miranda
 *
 ****************************************************************/

fbe_object_id_t fbe_sep_shim_io_struct_debug_trace(const fbe_u8_t * module_name,
                                                fbe_dbgext_ptr io_data_ptr,
                                                fbe_bool_t verbose_display,
												fbe_u32_t check_time_limit,
                                                fbe_trace_func_t trace_func,
                                                fbe_trace_context_t trace_context)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_object_id_t obj_id = FBE_OBJECT_ID_INVALID;
    fbe_u32_t spaces_to_indent = 4;
    fbe_u32_t offset;
    fbe_dbgext_ptr packet_ptr = 0;
    fbe_u32_t ptr_size;
    fbe_u32_t coarse_time_offset;
    fbe_u32_t irp_coarse_time;
    fbe_u32_t irp_coarse_wait_time;
    fbe_u32_t current_coarse_time;
    fbe_time_t current_time;

    fbe_trace_indent(trace_func, NULL, spaces_to_indent);
    status = fbe_debug_display_structure(module_name,
                                         trace_func,
                                         trace_context,
                                         io_data_ptr,
                                         &fbe_sep_shim_io_struct_info_main,
                                         4 /* fields per line */,
                                         spaces_to_indent);
    /* Get the current time.
     */
    fbe_debug_get_idle_thread_time(module_name, &current_time);
    current_coarse_time = (fbe_u32_t)current_time;
    FBE_GET_FIELD_OFFSET(module_name, fbe_sep_shim_io_struct_t, "coarse_time", &coarse_time_offset);
    FBE_READ_MEMORY(io_data_ptr + coarse_time_offset , &irp_coarse_time, sizeof(fbe_u32_t));
    FBE_GET_FIELD_OFFSET(module_name, fbe_sep_shim_io_struct_t, "coarse_wait_time", &coarse_time_offset);
    FBE_READ_MEMORY(io_data_ptr + coarse_time_offset , &irp_coarse_wait_time, sizeof(fbe_u32_t));
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    trace_func(trace_context, "irp outstanding %dms, waiting %dms for io_struct\n",
               (current_coarse_time-irp_coarse_time),
               (irp_coarse_time-irp_coarse_wait_time));

    if((verbose_display == FBE_TRUE)||
       ((check_time_limit>0) &&
        ((current_coarse_time-irp_coarse_time)>(check_time_limit*1000))))
    {
        status = fbe_debug_get_ptr_size(module_name, &ptr_size);
        if (status != FBE_STATUS_OK)
        {
            trace_func(trace_context, "unable to get ptr size status: %d\n", status);
            return obj_id;
        }
        trace_func(NULL, "\n");
        FBE_GET_FIELD_OFFSET(module_name, fbe_sep_shim_io_struct_t, "packet", &offset);
        FBE_READ_MEMORY(io_data_ptr + offset , &packet_ptr, ptr_size);
        if(packet_ptr == 0)
        {
            trace_func(trace_context, "Packet not available\n");
            return obj_id;
        }
        else
        {
            fbe_transport_print_fbe_packet(module_name,
                                           packet_ptr,
                                           trace_func,
                                           trace_context,
                                           spaces_to_indent);
            FBE_GET_FIELD_OFFSET(module_name, fbe_packet_t, "object_id", &offset);
            FBE_READ_MEMORY(packet_ptr + offset, &obj_id, sizeof(fbe_object_id_t));
        }
    }
    return obj_id;
}
/*****************************************
 * end fbe_sep_shim_io_struct_debug_trace
 *****************************************/
/*!**************************************************************
 * fbe_debug_display_extension_edge()
 ****************************************************************
 * @brief
 *  Display the extention's edge information.
 *
 * @param module_name
 * @param base_ptr
 * @param trace_func
 * @param trace_context
 * @param field_info_p
 * @param spaces_to_indent
 *
 * @return fbe_status_t
 *
 * @author
 *  4/1/2013 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_debug_display_extension_edge(const fbe_u8_t * module_name,
                                              fbe_dbgext_ptr base_ptr,
                                              fbe_trace_func_t trace_func,
                                              fbe_trace_context_t trace_context,
                                              fbe_debug_field_info_t *field_info_p,
                                              fbe_u32_t spaces_to_indent)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u64_t data = 0;
    fbe_u64_t deviceobject = 0;
    fbe_u32_t ptr_size;
    fbe_u32_t block_edge_offset;
    fbe_u32_t server_offset;
    fbe_object_id_t object_id = 0;
    status = fbe_debug_get_ptr_size(module_name, &ptr_size);
    if (status != FBE_STATUS_OK)
    {
        trace_func(trace_context, "unable to get ptr size status: %d\n", status);
        return status;
    }

    FBE_READ_MEMORY(base_ptr + field_info_p->offset, &deviceobject, ptr_size);

    /* Take the compatibility into account */
    #ifdef EMCPAL_USE_CSX_DCALL
        #ifdef CSX_BO_FD_DBG_ENABLE_PRIVATE_DATA_CHECKS
        {
            fbe_u32_t offset;
            csx_dbg_ext_get_field_offset("csx_p_int_device_handle_info_t", "device_private_data", &offset);
            FBE_READ_MEMORY(deviceobject + offset, &data, ptr_size);
        }
        #else
            FBE_READ_MEMORY(deviceobject, &data, ptr_size);
        #endif
    #else
    {
        fbe_u32_t offset;
        FBE_GET_FIELD_OFFSET(module_name, _DEVICE_OBJECT, "DeviceExtension", &offset);
        FBE_READ_MEMORY(deviceobject + offset, &data, ptr_size);
    }
    #endif

    FBE_GET_FIELD_OFFSET(module_name, os_device_info_t, "block_edge", &block_edge_offset);
    FBE_GET_FIELD_OFFSET(module_name, fbe_base_edge_t, "server_id", &server_offset);
    FBE_READ_MEMORY(data + block_edge_offset + server_offset, &object_id, sizeof(fbe_object_id_t));
    trace_func(trace_context, "lun object: 0x%x", object_id);
    return status;
}
/******************************************
 * end fbe_debug_display_extension_edge()
 ******************************************/

/*!*******************************************************************
 * @var fbe_sep_shim_summary_io_struct_field_info
 *********************************************************************
 * @brief Information about each of the fields of sep_shim_io .
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_sep_shim_summary_io_struct_field_info_main[] =
{
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("associated_io", fbe_u64_t,FBE_FALSE,"0x%x",
                                     fbe_debug_display_pointer),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("packet", fbe_packet_t, FBE_FALSE, "0x%x",
                                     fbe_debug_display_pointer),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("associated_device", fbe_u64_t, FBE_FALSE, "0x%x",
                                     fbe_debug_display_extension_edge),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_sep_shim_summary_io_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the IRPs
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_sep_shim_io_struct_t,
                              fbe_sep_shim_summary_io_struct_info_main,
                              &fbe_sep_shim_summary_io_struct_field_info_main[0]);

/*!**************************************************************
 * fbe_sep_shim_io_struct_summary_debug_trace()
 ****************************************************************
 * @brief
 *  Display just a summary view of the io structure.
 *  We will display all packets that are within a given
 *  time limit (if provided).
 *
 * @param module_name
 * @param io_data_ptr
 * @param trace_func
 * @param trace_context
 * @param check_time_limit - milliseconds.  Display packets that are
 *                           older than this time.  Set to 0
 *                           to display all packets.
 *
 * @return fbe_status_t
 *
 * @author
 *  4/1/2013 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_sep_shim_io_struct_summary_debug_trace(const fbe_u8_t * module_name,
                                                        fbe_dbgext_ptr io_data_ptr,
                                                        fbe_trace_func_t trace_func,
                                                        fbe_trace_context_t trace_context,
                                                        fbe_u32_t check_time_limit,
                                                        fbe_bool_t b_list)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t spaces_to_indent = 4;
    fbe_dbgext_ptr packet_ptr = 0;
    fbe_u32_t coarse_time_offset;
    fbe_u32_t packet_offset;
    fbe_u32_t payload_offset;
    fbe_u32_t irp_coarse_time;
    fbe_u32_t irp_coarse_wait_time;
    fbe_u32_t current_coarse_time;
    fbe_time_t current_time;
    fbe_u32_t ptr_size;
    fbe_payload_opcode_t payload_opcode;
    fbe_u32_t payload_operation;
    fbe_u64_t next_payload = 0;

    status = fbe_debug_get_ptr_size(module_name, &ptr_size);
    if (status != FBE_STATUS_OK)
    {
        trace_func(trace_context, "unable to get ptr size status: %d\n", status);
        return status;
    }

    /* Get the current time.
     */
    fbe_debug_get_idle_thread_time(module_name, &current_time);
    current_coarse_time = (fbe_u32_t)current_time;
    FBE_GET_FIELD_OFFSET(module_name, fbe_sep_shim_io_struct_t, "coarse_time", &coarse_time_offset);
    FBE_READ_MEMORY(io_data_ptr + coarse_time_offset , &irp_coarse_time, sizeof(fbe_u32_t));
    FBE_GET_FIELD_OFFSET(module_name, fbe_sep_shim_io_struct_t, "coarse_wait_time", &coarse_time_offset);
    FBE_READ_MEMORY(io_data_ptr + coarse_time_offset , &irp_coarse_wait_time, sizeof(fbe_u32_t));

    /* Don't display packets that are younger than the check time.
     */
    if ((check_time_limit > 0) &&
        ((current_coarse_time - irp_coarse_time) < (check_time_limit * 1000))){
        return FBE_STATUS_OK;
    }

    FBE_GET_FIELD_OFFSET(module_name, fbe_sep_shim_io_struct_t, "packet", &packet_offset);
    FBE_GET_FIELD_OFFSET(module_name, fbe_packet_t, "payload_union", &payload_offset);
    FBE_GET_FIELD_OFFSET(module_name, fbe_payload_ex_t, "operation_memory", &payload_operation);
    FBE_READ_MEMORY(io_data_ptr + packet_offset, &packet_ptr, ptr_size);

    fbe_debug_set_display_queue_header(FBE_FALSE);
    fbe_trace_indent(trace_func, NULL, spaces_to_indent);
    status = fbe_debug_display_structure(module_name,
                                         trace_func,
                                         trace_context,
                                         io_data_ptr,
                                         &fbe_sep_shim_summary_io_struct_info_main,
                                         4 /* fields per line */,
                                         spaces_to_indent);
    FBE_GET_FIELD_DATA(module_name,
                       packet_ptr + payload_offset + payload_operation,
                       fbe_payload_operation_header_t,
                       payload_opcode,
                       sizeof(fbe_payload_opcode_t),
                       &payload_opcode);
    //trace_func(trace_context, " packet: %llx payload_off: %d payload_operation: %d\n pay_opcode: %d\n",
    //           packet_ptr, payload_offset, payload_operation, payload_opcode);
    /* If the first two operations are memory and block operation, print block op data.
     */
    if (payload_opcode == FBE_PAYLOAD_OPCODE_MEMORY_OPERATION)
    {
        fbe_u32_t memory_op_size;
        FBE_GET_TYPE_SIZE(module_name, fbe_payload_memory_operation_t, &memory_op_size);
        next_payload = packet_ptr + payload_offset + payload_operation + memory_op_size;
        if (next_payload != 0)
        {
            FBE_GET_FIELD_DATA(module_name, next_payload,
                               fbe_payload_operation_header_t,
                               payload_opcode,
                               sizeof(fbe_payload_opcode_t),
                               &payload_opcode);
            if (payload_opcode == FBE_PAYLOAD_OPCODE_BLOCK_OPERATION)
            {
                fbe_payload_block_debug_print_summary(module_name, next_payload, trace_func, trace_context, NULL, 0);
            }
        }
    }
    else
    {
        trace_func(trace_context, " payload_opcode: %d ", payload_opcode);
    }

    //fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    trace_func(trace_context, " (%d seconds)\n",
               ((current_coarse_time-irp_coarse_time)/1000));

    if ((irp_coarse_time-irp_coarse_wait_time)/1000 > 0){
        trace_func(trace_context, "%d seconds for io_struct\n",
                   ((irp_coarse_time-irp_coarse_wait_time)/1000));
    }
    if (!b_list && packet_ptr != NULL) {
        fbe_transport_print_packet_summary(module_name, packet_ptr, trace_func, trace_context, spaces_to_indent);
    }

    fbe_debug_set_display_queue_header(FBE_TRUE);
    return status;
}
/******************************************
 * end fbe_sep_shim_io_struct_summary_debug_trace()
 ******************************************/

/*************************
 * end file fbe_sep_shim_debug.c
 *************************/
