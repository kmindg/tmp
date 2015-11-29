/***************************************************************************
 *  Copyright (C)  EMC Corporation 2000-11
 *  All rights reserved.
 *  Licensed material - property of EMC Corporation. * All rights reserved.
 ***************************************************************************/

/*!**************************************************************************
 * @file scheduler.c
 ***************************************************************************
 *
 * @brief
 *  This file contains debugging commands about scheduler related information
.*
 * @version
 *   12-Oct-2011:  Created. Omer Miranda
 *
 ***************************************************************************/

#include "pp_dbgext.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_topology_interface.h"
#include "fbe/fbe_class.h"
#include "fbe_base_object_debug.h"
#include "fbe_base_object_trace.h"
#include "fbe/fbe_scheduler_interface.h"
#include "fbe_scheduler_debug.h"
#include "pp_ext.h"


/* ***************************************************************************
 *
 * @brief
 *  Debug macro to display all the scheduler information.
 *
 *  Arguments: None
 *  Return Value: None
 * 
 * @version
 *   12-Oct-2011:  Created. Omer Miranda
 *
 ***************************************************************************/

#pragma data_seg ("EXT_HELP$4scheduler")
static char CSX_MAYBE_UNUSED usageMsg_scheduler[] =
"!scheduler\n"
"  Displays all the scheduler related information.\n"
"  By default we show all the information \n"
"  -v - This option will also display the associated packet information \n"
"  -q - This option will display only the IDLE scheduler queue \n"
"  -no_q - This option will not display the scheduler queues information \n"
"  -c - This option will display only the core_credit_table \n"
"  -t - This option will display only the scheduler thread information \n"
"  -dbg_hook - This option will display only the scheduler debug hooks \n";
#pragma data_seg ()


CSX_DBG_EXT_DEFINE_CMD(scheduler, "scheduler")
{
    fbe_dbgext_ptr scheduler_idle_queue_head_ptr = 0;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t ptr_size;
    fbe_u32_t display_format = FBE_SCHEDULER_INFO_DEFAULT_DISPLAY;
    fbe_char_t *str = NULL;
    fbe_dbgext_ptr scheduler_thread_info_pool_ptr = 0;
    fbe_dbgext_ptr scheduler_debug_hooks_ptr = 0;
    fbe_dbgext_ptr core_credit_table_ptr = 0;

    const fbe_u8_t * module_name = "sep"; 

    status = fbe_debug_get_ptr_size(module_name, &ptr_size);
    if (status != FBE_STATUS_OK) 
    {
        fbe_debug_trace_func(NULL, "%s unable to get ptr size status: %d\n", __FUNCTION__, status);
        return ; 
    }

    str = strtok((char*)args, " \t");
    /* Display associated scheduler packets */
    if ((strlen(args) && strncmp(args, "-v", 2) == 0) ||
        (strlen(args) && strncmp(args, "-V", 2) == 0))
    {
        display_format = FBE_SCHEDULER_INFO_VERBOSE_DISPLAY;
        /* Increment past the allsep flag.
         */
        args += FBE_MIN(3, strlen(args));
    }
    /* Display only the scheduler queues */
    else if (strlen(args) && strncmp(args, "-q", 2) == 0)
    {
        display_format = FBE_SCHEDULER_INFO_QUEUE_ONLY_DISPLAY;
        args += FBE_MIN(3, strlen(args));
    }
    /* Do not display the scheduler queues */
    else if (strlen(args) && strncmp(args, "-no_q", 5) == 0)
    {
        display_format = FBE_SCHEDULER_INFO_NO_QUEUE_DISPLAY;
        args += FBE_MIN(6, strlen(args));
    }
    /* Display only the scheduler thread info */
    else if (strlen(args) && strncmp(args, "-t", 2) == 0)
    {
        display_format = FBE_SCHEDULER_INFO_THREAD_INFO_DISPLAY;
        args += FBE_MIN(3, strlen(args));
    }
    /* Display only the scheduler core credit table */
    else if (strlen(args) && strncmp(args, "-c", 2) == 0)
    {
        display_format = FBE_SCHEDULER_INFO_CORE_CREDIT_TABLE_DISPLAY;
        args += FBE_MIN(3, strlen(args));
    }
    /* Display only the scheduler debug hooks */
    else if (strlen(args) && strncmp(args, "-dbg_hook", 9) == 0)
    {
        display_format = FBE_SCHEDULER_INFO_DEBUG_HOOKS_INFO_DISPLAY;
        args += FBE_MIN(10, strlen(args));
    }
	else if (strlen(args) && strncmp(args, "-help", 5) == 0)
	{
		fbe_debug_trace_func(NULL, "!scheduler\n"
									"Displays all the scheduler related information.\n"
									"By default we show all the information \n"
									"-v - This option will also display the associated packet information \n");
		fbe_debug_trace_func(NULL, "-q - This option will display only the IDLE scheduler queue \n"
								   "-no_q - This option will not display the scheduler queues information \n");
		fbe_debug_trace_func(NULL, "-c - This option will display only the core_credit_table \n"
									"-t - This option will display only the scheduler thread information \n"
									"-dbg_hook - This option will display only the scheduler debug hooks \n");
		return;
	}

    if(get_active_queue_display_flag() == FBE_FALSE)
    {
        display_format = FBE_SCHEDULER_INFO_NO_QUEUE_DISPLAY;
    }

    /* Get the scheduler_thread_info_pool. */
    FBE_GET_EXPRESSION(pp_ext_module_name, scheduler_thread_info_pool, &scheduler_thread_info_pool_ptr);
    if(scheduler_thread_info_pool_ptr == 0)
    {
        fbe_trace_indent(fbe_debug_trace_func, NULL, 4);
        fbe_debug_trace_func(NULL, "scheduler_thread_info_pool_ptr is not available \n");
    }

    /* Get the scheduler_debug_hook_info. */
    FBE_GET_EXPRESSION(pp_ext_module_name, scheduler_debug_hooks, &scheduler_debug_hooks_ptr);
    if(scheduler_debug_hooks_ptr == 0)
    {
        fbe_trace_indent(fbe_debug_trace_func, NULL, 4);
        fbe_debug_trace_func(NULL, "scheduler_debug_hooks_ptr is not available \n");
    }

    /* Get the scheduler_core_credit_table. */
    FBE_GET_EXPRESSION(pp_ext_module_name, core_credit_table, &core_credit_table_ptr);
    if(core_credit_table_ptr == 0)
    {
        fbe_trace_indent(fbe_debug_trace_func, NULL, 4);
        fbe_debug_trace_func(NULL, "core_credit_table_ptr is not available \n");
    }

    /* Get the scheduler_idle_queue_head. */
    FBE_GET_EXPRESSION(pp_ext_module_name, scheduler_idle_queue_head, &scheduler_idle_queue_head_ptr);
    if(scheduler_idle_queue_head_ptr == 0)
    {
        fbe_trace_indent(fbe_debug_trace_func, NULL, 4);
        fbe_debug_trace_func(NULL, "scheduler_idle_queue_head is not available \n");
    }
    

    /* Display the scheduler thread info only if defualt display or if user sepcified so */
    if(display_format == FBE_SCHEDULER_INFO_DEFAULT_DISPLAY ||
       display_format == FBE_SCHEDULER_INFO_THREAD_INFO_DISPLAY ||
       display_format == FBE_SCHEDULER_INFO_NO_QUEUE_DISPLAY)
    {
        fbe_debug_trace_func(NULL,"\nScheduler Thread info:");
        fbe_scheduler_thread_info_debug(module_name,scheduler_thread_info_pool_ptr,fbe_debug_trace_func,NULL);
        /* if user needs to see only the thred info then return */
        if(display_format == FBE_SCHEDULER_INFO_THREAD_INFO_DISPLAY)
        {
            return;
        }
    }

    /* Display the core credit table only if defualt display or if user sepcified so */
    if(display_format == FBE_SCHEDULER_INFO_DEFAULT_DISPLAY ||
       display_format == FBE_SCHEDULER_INFO_CORE_CREDIT_TABLE_DISPLAY ||
       display_format == FBE_SCHEDULER_INFO_NO_QUEUE_DISPLAY)
    {
        fbe_debug_trace_func(NULL,"\nCore Credit table:");
        fbe_scheduler_core_credit_table_debug(module_name,core_credit_table_ptr,fbe_debug_trace_func,NULL);
        /* if user needs to see only the core credits info then return */
        if(display_format == FBE_SCHEDULER_INFO_CORE_CREDIT_TABLE_DISPLAY)
        {
            return;
        }
    }

    /* Display the scheduler queues only if defualt display or if user sepcified so */
    if(display_format == FBE_SCHEDULER_INFO_DEFAULT_DISPLAY ||
       display_format == FBE_SCHEDULER_INFO_QUEUE_ONLY_DISPLAY ||
       display_format == FBE_SCHEDULER_INFO_VERBOSE_DISPLAY)
    {
        fbe_debug_trace_func(NULL, "\n");
        fbe_debug_trace_func(NULL,"\nScheduler IDLE queue:");
        fbe_scheduler_queue_debug(module_name,scheduler_idle_queue_head_ptr,display_format,fbe_debug_trace_func,NULL);
        /* if user needs to see only the thred info then return */
        if(display_format == FBE_SCHEDULER_INFO_QUEUE_ONLY_DISPLAY)
        {
            return;
        }
    }
    /* Display the debug hooks only if defualt display or if user sepcified so */
    if(display_format == FBE_SCHEDULER_INFO_DEFAULT_DISPLAY ||
       display_format == FBE_SCHEDULER_INFO_DEBUG_HOOKS_INFO_DISPLAY ||
       display_format == FBE_SCHEDULER_INFO_NO_QUEUE_DISPLAY)
    {
        /* Display the scheduler debug hooks info */
        fbe_debug_trace_func(NULL,"\nScheduler debug hooks info:");
        fbe_scheduler_debug_hooks_info_debug(module_name,scheduler_debug_hooks_ptr,fbe_debug_trace_func,NULL);
        /* if user needs to see only the debug hooks then return */
        if(display_format == FBE_SCHEDULER_INFO_DEBUG_HOOKS_INFO_DISPLAY)
        {
            return;
        }
    }

    return;
}

//end of scheduler.c
