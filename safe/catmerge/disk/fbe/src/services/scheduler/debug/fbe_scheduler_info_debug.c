/***************************************************************************
 * Copyright (C) EMC Corporation 2010-2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_scheduler_info_debug.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the debug functions to display the scheduler information.
 *
 * @author
 *  12-Oct-2011:  Created. Omer Miranda
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_dbgext.h"
#include "fbe_topology_debug.h"
#include "fbe_base_object.h"
#include "fbe_scheduler_debug.h"
#include "fbe_base_object_debug.h"
#include "fbe_base_object_trace.h"
#include "fbe_scheduler_info_debug.h"
#include "fbe_transport_debug.h"
#include "fbe_scheduler_debug.h"
#include "fbe/fbe_scheduler_interface.h"



/*!*******************************************************************
 * @var fbe_scheduler_element_debug_struct_field_info
 *********************************************************************
 * @brief Information about each of the fields of fbe_scheduler_element_t.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_scheduler_element_debug_struct_field_info[] =
{
    FBE_DEBUG_DECLARE_FIELD_INFO("time_counter", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("current_queue", fbe_scheduler_queue_type_t, FBE_FALSE, "0x%x",
                                    fbe_scheduler_queue_type_debug_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO("scheduler_state", fbe_scheduler_state_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("hook", fbe_scheduler_debug_hook_function_t, FBE_FALSE, "0x%x",
                                    fbe_debug_display_function_ptr),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("packet", fbe_packet_t, FBE_FALSE, "0x%x",
                                    fbe_debug_display_pointer),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_scheduler_element_debug_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        scheduler element structure.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_scheduler_element_t,
                              fbe_scheduler_element_debug_struct_info,
                              &fbe_scheduler_element_debug_struct_field_info[0]);


/*!**************************************************************
 * fbe_scheduler_queue_debug_trace()
 ****************************************************************
 * @brief
 *  Display the queue elements
 *  
 * @param module_name - Module name to use in symbol lookup.
 * @param scheduler_element_ptr - The queue element's pointer.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * 
 * @return fbe_status_t
 *
 * @author
 *  12-Oct-2011:  Created. Omer Miranda
 *
 ****************************************************************/
fbe_status_t fbe_scheduler_queue_debug_trace(const fbe_u8_t * module_name,
                                             fbe_dbgext_ptr scheduler_element_ptr,
                                             fbe_u32_t display_format,
                                             fbe_trace_func_t trace_func,
                                             fbe_trace_context_t trace_context)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t spaces_to_indent = 4;
    fbe_u32_t offset;
    fbe_dbgext_ptr packet_ptr = 0;
    fbe_u32_t ptr_size;

    status = fbe_debug_display_structure(module_name, trace_func, trace_context,
                                         scheduler_element_ptr,
                                         &fbe_scheduler_element_debug_struct_info,
                                         4    /* fields per line */,
                                         spaces_to_indent);


    if(display_format == FBE_SCHEDULER_INFO_VERBOSE_DISPLAY)
    {
        status = fbe_debug_get_ptr_size(module_name, &ptr_size);
        if (status != FBE_STATUS_OK) 
        {
            trace_func(trace_context, "unable to get ptr size status: %d\n", status);
            return status; 
        }
        trace_func(NULL, "\n");
        FBE_GET_FIELD_OFFSET(module_name, fbe_scheduler_element_t, "packet", &offset);
        FBE_READ_MEMORY(scheduler_element_ptr + offset , &packet_ptr, ptr_size);
        if(packet_ptr == 0)
        {
            trace_func(trace_context, "Packet not available\n");
            return status;
        }
        else
        {
            fbe_transport_print_fbe_packet(module_name,
                                           packet_ptr,
                                           trace_func,
                                           trace_context,
                                           spaces_to_indent);
        }
    }
    return status;
}
/*****************************************
 * end fbe_scheduler_queue_debug_trace
 *****************************************/
/*!**************************************************************
 * fbe_scheduler_queue_debug_trace()
 ****************************************************************
 * @brief
 *  Processes the scheduler queue
 *  
 * @param module_name - Module name to use in symbol lookup.
 * @param head_ptr - The queue's HEAD pointer.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * 
 * @return fbe_status_t
 *
 * @author
 *  12-Oct-2011:  Created. Omer Miranda
 *
 ****************************************************************/
fbe_status_t fbe_scheduler_queue_debug(const fbe_u8_t * module_name,
                                       fbe_dbgext_ptr head_ptr,
                                       fbe_u32_t display_format,
                                       fbe_trace_func_t trace_func,
                                       fbe_trace_context_t trace_context)
{
    fbe_u32_t next_offset;
    fbe_u32_t prev_offset;
    fbe_dbgext_ptr head = 0;
    fbe_dbgext_ptr next_element = 0;
    fbe_dbgext_ptr prev_element = 0;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t ptr_size;
    fbe_u32_t scheduler_offset;
    fbe_u32_t object_id_offset;
    fbe_dbgext_ptr base_object_ptr = 0;
    fbe_object_id_t object_id;
    fbe_u32_t item_count = 0;

    if(head_ptr == NULL)
    {
        trace_func(NULL, "\t Queue head ptr not available \n");
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    status = fbe_debug_get_ptr_size(module_name, &ptr_size);	
    if (status != FBE_STATUS_OK) 
    {
        trace_func(NULL, "%s unable to get ptr size status: %d\n", __FUNCTION__, status);
        return FBE_STATUS_OK; 
    }

    FBE_GET_FIELD_OFFSET(module_name, fbe_queue_head_t, "next", &next_offset);
    FBE_GET_FIELD_OFFSET(module_name, fbe_queue_head_t, "prev", &prev_offset);

    FBE_READ_MEMORY(head_ptr + next_offset, &next_element, ptr_size);
    FBE_READ_MEMORY(head_ptr + prev_offset, &prev_element, ptr_size);

    /* If Head's next and prev is pointing to itself, then its an empty queue */
    if(next_element == head_ptr && prev_element == head_ptr)
    {
        trace_func(NULL, "Scheduler queue is EMPTY \n");
        return FBE_STATUS_OK;
    }

    head = head_ptr;		/* save for end of queue condition */

    /* Get the queue length */
    fbe_transport_get_queue_size(module_name,head_ptr,trace_func,trace_context,&item_count);
    trace_func(NULL, "Queue Length = %d\n",item_count);
    FBE_GET_FIELD_OFFSET(module_name, fbe_base_object_t, "scheduler_element", &scheduler_offset);
    FBE_GET_FIELD_OFFSET(module_name, fbe_base_object_t, "object_id", &object_id_offset);

    while(next_element != head )
    {
        if (FBE_CHECK_CONTROL_C())
        {
            return FBE_STATUS_CANCELED;
        }
        trace_func(NULL, "\n");
        fbe_trace_indent(trace_func, NULL, 4);
        /* Get the object id associated with the scheduler element*/
		base_object_ptr =  next_element - scheduler_offset;
        FBE_READ_MEMORY(base_object_ptr + object_id_offset , &object_id, sizeof(fbe_object_id_t));
        trace_func(NULL, "Object ID:0x%x\n",object_id);
        fbe_trace_indent(trace_func, NULL, 4);
        fbe_scheduler_queue_debug_trace(module_name,next_element,display_format,trace_func,trace_context);

        FBE_READ_MEMORY(next_element + next_offset , &next_element, ptr_size);
    }
	trace_func(NULL, "\n");
    return status;
}
/*****************************************
 * end fbe_scheduler_queue_debug
 *****************************************/



/*!**************************************************************
 * fbe_scheduler_queue_type_debug_trace()
 ****************************************************************
 * @brief
 *  Display the scheduler queue type.
 *
 * @param module_name - Module name to use in symbol lookup.
 * @param scheduler_element_ptr - ptr to value to display.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param field_info_p - Field to init. 
 * @param spaces_to_indent - Number of spaces to indent the next line(s).
 * 
 * @return fbe_status_t   
 *
 * @author
 *  12-Oct-2011:  Created. Omer Miranda
 *
 ****************************************************************/ 
fbe_status_t fbe_scheduler_queue_type_debug_trace(const fbe_u8_t * module_name,
                                                  fbe_dbgext_ptr scheduler_element_ptr,
                                                  fbe_trace_func_t trace_func,
                                                  fbe_trace_context_t trace_context,
                                                  fbe_debug_field_info_t *field_info_p,
                                                  fbe_u32_t spaces_to_indent)
{
    fbe_scheduler_queue_type_t current_queue;

    FBE_READ_MEMORY(scheduler_element_ptr + field_info_p->offset, &current_queue, field_info_p->size);
    trace_func(trace_context, "Scheduler queue type:");
    switch(current_queue)
    {
        case FBE_SCHEDULER_QUEUE_TYPE_IDLE:
            trace_func(trace_context, "IDLE");
        break;
        case FBE_SCHEDULER_QUEUE_TYPE_RUN:
            trace_func(trace_context, "RUN");
        break;
        case FBE_SCHEDULER_QUEUE_TYPE_PENDING:
            trace_func(trace_context, "PENDING");
        break;
        case FBE_SCHEDULER_QUEUE_TYPE_DESTROYED:
            trace_func(trace_context, "DESTROYED");
        break;
        case FBE_SCHEDULER_QUEUE_TYPE_INVALID:
            trace_func(trace_context, "INVALID");
        break;
        default:
            trace_func(trace_context, "UNKNOWN %d", current_queue);
        break;
    }
    return FBE_STATUS_OK;
}
/*****************************************
 * end fbe_scheduler_queue_type_debug_trace
 *****************************************/

/*!*******************************************************************
 * @var fbe_scheduler_thread_info_struct_field_info
 *********************************************************************
 * @brief Information about each of the fields of fbe_scheduler_thread_info_t.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_scheduler_thread_info_struct_field_info[] =
{
    FBE_DEBUG_DECLARE_FIELD_INFO("object_id", fbe_object_id_t, FBE_TRUE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("dispatch_timestamp", fbe_time_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("exceed_counter", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_scheduler_thread_info_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        scheduler thread info structure.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_scheduler_thread_info_t,
                              fbe_scheduler_thread_info_struct_info,
                              &fbe_scheduler_thread_info_struct_field_info[0]);

/*!**************************************************************
 * fbe_scheduler_thread_info_debug_trace()
 ****************************************************************
 * @brief
 *  Display the Scheduler Thread info pool
 *  
 * @param module_name - Module name to use in symbol lookup.
 * @param thread_info_ptr - The Scheduler thread info ptr.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * 
 * @return fbe_status_t
 *
 * @author
 *  14-Oct-2011:  Created. Omer Miranda
 *
 ****************************************************************/
fbe_status_t fbe_scheduler_thread_info_debug_trace(const fbe_u8_t * module_name,
                                                   fbe_dbgext_ptr thread_info_ptr,
                                                   fbe_trace_func_t trace_func,
                                                   fbe_trace_context_t trace_context)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t spaces_to_indent = 8;

    status = fbe_debug_display_structure(module_name, trace_func, trace_context,
                                         thread_info_ptr,
                                         &fbe_scheduler_thread_info_struct_info,
                                         4    /* fields per line */,
                                         spaces_to_indent);

    return status;
}
/********************************************
 * end fbe_scheduler_thread_info_debug_trace
 ********************************************/

/*!**************************************************************
 * fbe_scheduler_thread_info_debug()
 ****************************************************************
 * @brief
 *  Processes the Scheduler Thread info pool
 *  
 * @param module_name - Module name to use in symbol lookup.
 * @param thread_info_pool_ptr - The Scheduler thread info pool ptr.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * 
 * @return fbe_status_t
 *
 * @author
 *  14-Oct-2011:  Created. Omer Miranda
 *
 ****************************************************************/
fbe_status_t fbe_scheduler_thread_info_debug(const fbe_u8_t * module_name,
                                             fbe_dbgext_ptr thread_info_pool_ptr,
                                             fbe_trace_func_t trace_func,
                                             fbe_trace_context_t trace_context)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t fbe_scheduler_thread_info_size;
    fbe_u32_t thread_pool;
    fbe_cpu_id_t scheduler_cpu_count = 0;
    fbe_dbgext_ptr cpu_count_ptr = 0;

    /* Get the scheduler cpu count. */
    FBE_GET_EXPRESSION(module_name, fbe_scheduler_cpu_count, &cpu_count_ptr);
    FBE_READ_MEMORY(cpu_count_ptr , &scheduler_cpu_count, sizeof(fbe_cpu_id_t));

    /* Get the size of the fbe_scheduler_thread_info_t. */
    FBE_GET_TYPE_SIZE(module_name, fbe_scheduler_thread_info_t, &fbe_scheduler_thread_info_size);

    for(thread_pool = 0; thread_pool < scheduler_cpu_count; thread_pool++)
    {
        trace_func(NULL, "\n");
        fbe_trace_indent(trace_func, NULL, 4);
        trace_func(NULL, "thread_info[%d]:",thread_pool);
        thread_info_pool_ptr += (fbe_scheduler_thread_info_size * thread_pool);
        fbe_scheduler_thread_info_debug_trace(module_name,thread_info_pool_ptr,trace_func,trace_context);
    }
	trace_func(NULL, "\n");
    return status;
}
/*****************************************
 * end fbe_scheduler_thread_info_debug
 *****************************************/

/*!*******************************************************************
 * @var fbe_scheduler_debug_hook_struct_field_info
 *********************************************************************
 * @brief Information about each of the fields of fbe_scheduler_debug_hook_t.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_scheduler_debug_hook_struct_field_info[] =
{
    FBE_DEBUG_DECLARE_FIELD_INFO("object_id", fbe_object_id_t, FBE_TRUE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("monitor_state", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("monitor_substate", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("val1", fbe_u64_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("val2", fbe_u64_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("check_type", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("action", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("counter", fbe_u64_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_scheduler_debug_hooks_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        scheduler debug hook structure.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_scheduler_debug_hook_t,
                              fbe_scheduler_debug_hook_struct_info,
                              &fbe_scheduler_debug_hook_struct_field_info[0]);

/*!**************************************************************
 * fbe_scheduler_debug_hooks_debug_trace()
 ****************************************************************
 * @brief
 *  Display the Scheduler debug hooks
 *  
 * @param module_name - Module name to use in symbol lookup.
 * @param debug_hook_ptr - The Scheduler debug hook info ptr.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * 
 * @return fbe_status_t
 *
 * @author
 *  14-Oct-2011:  Created. Omer Miranda
 *
 ****************************************************************/
fbe_status_t fbe_scheduler_debug_hooks_debug_trace(const fbe_u8_t * module_name,
                                                   fbe_dbgext_ptr debug_hook_ptr,
                                                   fbe_trace_func_t trace_func,
                                                   fbe_trace_context_t trace_context)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t spaces_to_indent = 8;

    status = fbe_debug_display_structure(module_name, trace_func, trace_context,
                                         debug_hook_ptr,
                                         &fbe_scheduler_debug_hook_struct_info,
                                         4    /* fields per line */,
                                         spaces_to_indent);

    return status;
}
/********************************************
 * end fbe_scheduler_debug_hooks_debug_trace
 ********************************************/

/*!**************************************************************
 * fbe_scheduler_debug_hooks_info_debug()
 ****************************************************************
 * @brief
 *  Processes the Scheduler Debug hooks info
 *  
 * @param module_name - Module name to use in symbol lookup.
 * @param debug_hooks_info_ptr - The Scheduler debug hooks info ptr.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * 
 * @return fbe_status_t
 *
 * @author
 *  14-Oct-2011:  Created. Omer Miranda
 *
 ****************************************************************/
fbe_status_t fbe_scheduler_debug_hooks_info_debug(const fbe_u8_t * module_name,
                                                  fbe_dbgext_ptr debug_hooks_info_ptr,
                                                  fbe_trace_func_t trace_func,
                                                  fbe_trace_context_t trace_context)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t fbe_scheduler_debug_hook_size;
    fbe_u32_t hook_count;

    /* Get the size of the fbe_scheduler_debug_hook_t. */
    FBE_GET_TYPE_SIZE(module_name, fbe_scheduler_debug_hook_t, &fbe_scheduler_debug_hook_size);

    for(hook_count = 0; hook_count < MAX_SCHEDULER_DEBUG_HOOKS; hook_count++)
    {
        trace_func(NULL, "\n");
        fbe_trace_indent(trace_func, NULL, 4);
        trace_func(NULL, "hook_info[%d]:",hook_count);
        debug_hooks_info_ptr += (fbe_scheduler_debug_hook_size * hook_count);
        fbe_scheduler_debug_hooks_debug_trace(module_name,debug_hooks_info_ptr,trace_func,trace_context);
    }
	trace_func(NULL, "\n");
    return status;
}
/*****************************************
 * end fbe_scheduler_debug_hooks_info_debug
 *****************************************/

/*!*******************************************************************
 * @var fbe_scheduler_credit_struct_field_info
 *********************************************************************
 * @brief Information about each of the fields of fbe_scheduler_credit_t.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_scheduler_credit_struct_field_info[] =
{
    FBE_DEBUG_DECLARE_FIELD_INFO("cpu_operations_per_second", fbe_atomic_t, FBE_TRUE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("mega_bytes_consumption", fbe_atomic_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_scheduler_credits_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        scheduler credits structure.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_scheduler_credit_t,
                              fbe_scheduler_credit_struct_info,
                              &fbe_scheduler_credit_struct_field_info[0]);

/*!**************************************************************
 * fbe_scheduler_credit_debug_trace()
 ****************************************************************
 * @brief
 *  Display the Scheduler credits info
 *  
 * @param module_name - Module name to use in symbol lookup.
 * @param credit_base_ptr - The Scheduler credits ptr.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * 
 * @return fbe_status_t
 *
 * @author
 *  14-Oct-2011:  Created. Omer Miranda
 *
 ****************************************************************/
fbe_status_t fbe_scheduler_credit_debug_trace(const fbe_u8_t * module_name,
                                              fbe_dbgext_ptr credit_base_ptr,
                                              fbe_trace_func_t trace_func,
                                              fbe_trace_context_t trace_context,
                                              fbe_debug_field_info_t *field_info_p,
                                              fbe_u32_t spaces_to_indent)
{
    fbe_status_t status = FBE_STATUS_OK;

    trace_func(NULL, "\n");
    fbe_trace_indent(trace_func, NULL, 4);

    trace_func(trace_context, "%s:\n", field_info_p->name);
    fbe_trace_indent(trace_func, NULL, 8);
    status = fbe_debug_display_structure(module_name, trace_func, trace_context,
                                         credit_base_ptr,
                                         &fbe_scheduler_credit_struct_info,
                                         4    /* fields per line */,
                                         spaces_to_indent);

    return status;
}
/*****************************************
 * end fbe_scheduler_credit_debug_trace
 *****************************************/

/*!*******************************************************************
 * @var fbe_scheduler_credit_table_struct_field_info
 *********************************************************************
 * @brief Information about each of the fields of schduler_credit_table_entry_t.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_scheduler_credit_table_struct_field_info[] =
{
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("current_credits", fbe_scheduler_credit_t, FBE_TRUE, "0x%x",
                                    fbe_scheduler_credit_debug_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("master_credits", fbe_scheduler_credit_t, FBE_FALSE, "0x%x",
                                    fbe_scheduler_credit_debug_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_scheduler_core_credits_table_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        scheduler core credits structure.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(schduler_credit_table_entry_t,
                              fbe_scheduler_credit_table_struct_info,
                              &fbe_scheduler_credit_table_struct_field_info[0]);

/*!**************************************************************
 * fbe_scheduler_core_credit_table_debug_trace()
 ****************************************************************
 * @brief
 *  Display the Scheduler core credits table info
 *  
 * @param module_name - Module name to use in symbol lookup.
 * @param core_credits_ptr - The Scheduler core credits ptr.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * 
 * @return fbe_status_t
 *
 * @author
 *  14-Oct-2011:  Created. Omer Miranda
 *
 ****************************************************************/
fbe_status_t fbe_scheduler_core_credit_table_debug_trace(const fbe_u8_t * module_name,
                                                         fbe_dbgext_ptr core_credits_ptr,
                                                         fbe_trace_func_t trace_func,
                                                         fbe_trace_context_t trace_context)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t spaces_to_indent = 8;

    status = fbe_debug_display_structure(module_name, trace_func, trace_context,
                                         core_credits_ptr,
                                         &fbe_scheduler_credit_table_struct_info,
                                         4    /* fields per line */,
                                         spaces_to_indent);

    return status;
}
/*************************************************
 * end fbe_scheduler_core_credit_table_debug_trace
 *************************************************/

/*!**************************************************************
 * fbe_scheduler_core_credit_table_debug()
 ****************************************************************
 * @brief
 *  Processes the Scheduler core credit's info
 *  
 * @param module_name - Module name to use in symbol lookup.
 * @param core_credit_table_ptr - The Scheduler core credit table ptr.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * 
 * @return fbe_status_t
 *
 * @author
 *  14-Oct-2011:  Created. Omer Miranda
 *
 ****************************************************************/
fbe_status_t fbe_scheduler_core_credit_table_debug(const fbe_u8_t * module_name,
                                                   fbe_dbgext_ptr core_credit_table_ptr,
                                                   fbe_trace_func_t trace_func,
                                                   fbe_trace_context_t trace_context)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t schduler_credit_table_entry_size;
    fbe_u32_t table_entry;
    fbe_dbgext_ptr scheduler_master_memory_ptr = 0;
    fbe_u64_t scheduler_master_memory;
    fbe_dbgext_ptr current_scale_ptr = 0;
    fbe_u64_t current_scale;
    fbe_dbgext_ptr credit_load_timer_count_ptr = 0;
    fbe_u32_t credit_load_timer_count;
    fbe_u32_t scheduler_cpu_count = 0;
    fbe_dbgext_ptr cpu_count_ptr = 0;

    /* Get the scheduler cpu count. */
    FBE_GET_EXPRESSION(module_name, fbe_scheduler_number_of_cores, &cpu_count_ptr);
    FBE_READ_MEMORY(cpu_count_ptr , &scheduler_cpu_count, sizeof(fbe_u32_t));

    /* Get the size of the fbe_scheduler_debug_hook_t. */
    FBE_GET_TYPE_SIZE(module_name, schduler_credit_table_entry_t, &schduler_credit_table_entry_size);

    for(table_entry = 0; table_entry < scheduler_cpu_count; table_entry++)
    {
        trace_func(NULL, "\n");
        fbe_trace_indent(trace_func, NULL, 4);
        trace_func(NULL, "credit_table_entry[%d]:",table_entry);
        core_credit_table_ptr += (schduler_credit_table_entry_size * table_entry);
        fbe_scheduler_core_credit_table_debug_trace(module_name,core_credit_table_ptr,trace_func,trace_context);
    }

    trace_func(NULL, "\n\n");
    /* Get the scheduler_master_memory. */
    FBE_GET_EXPRESSION(module_name, scheduler_master_memory, &scheduler_master_memory_ptr);
    FBE_READ_MEMORY(scheduler_master_memory_ptr, &scheduler_master_memory, sizeof(fbe_u64_t));
    fbe_trace_indent(trace_func, NULL, 4);
    trace_func(NULL, "scheduler_master_memory:0x%llx", (unsigned long long)scheduler_master_memory);

    /* Get the current_scale. */
    FBE_GET_EXPRESSION(module_name, current_scale, &current_scale_ptr);
    FBE_READ_MEMORY(current_scale_ptr, &current_scale, sizeof(fbe_u64_t));
    fbe_trace_indent(trace_func, NULL, 4);
    trace_func(NULL, "current_scale:%llu", (unsigned long long)current_scale);

    /* Get the credit_load_timer_count. */
    FBE_GET_EXPRESSION(module_name, credit_load_timer_count, &credit_load_timer_count_ptr);
    FBE_READ_MEMORY(credit_load_timer_count_ptr, &credit_load_timer_count, sizeof(fbe_u32_t));
    fbe_trace_indent(trace_func, NULL, 4);
    trace_func(NULL, "credit_load_timer_count:0x%x",credit_load_timer_count);
	trace_func(NULL, "\n");

    return status;
}
/*****************************************
 * end fbe_scheduler_core_credit_table_debug
 *****************************************/


/*************************
 * end file fbe_scheduler_queue_debug.c
 *************************/


