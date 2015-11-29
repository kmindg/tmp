/***************************************************************************
 *  Copyright (C)  EMC Corporation 2000-11
 *  All rights reserved.
 *  Licensed material - property of EMC Corporation. * All rights reserved.
 ***************************************************************************/

/*!**************************************************************************
 * @file bvd_irps.c
 ***************************************************************************
 *
 * @brief
 *  This file contains debugging commands about incoming BVD IRPs
 *  and any associated packets.
.*
 * @version
 *   16-Sep-2010:  Created. Omer Miranda
 *
 ***************************************************************************/

#include "pp_dbgext.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_topology_interface.h"
#include "fbe/fbe_class.h"
#include "fbe_topology_debug.h"
#include "fbe_base_object_debug.h"
#include "fbe_base_object_trace.h"
#include "fbe/fbe_sep_shim.h"
#include "fbe_sep_shim_debug.h"
 
/* ***************************************************************************
 *
 * @brief
 *  Debug macro to display BVD IRPs and associated packets.
 *
 *  Arguments: None
 *  Return Value: None
 * 
 * @version
 *   16-Sep-2010:  Created. Omer Miranda
 *
 ***************************************************************************/

#pragma data_seg ("EXT_HELP$4bvd_irps")
static char CSX_MAYBE_UNUSED usageMsg_bvd_irps[] =
"!bvd_irps\n"
"  Displays BVD IRPs and associated packet's info.\n"
"  -v - This option displays all the packets details\n"
"  -t <sec> - This option displays all the packets not finished within number of seconds\n"
"  -l - Brief list of IRPs and packets\n"
"  -s - Summary of IRPS and packets\n";
#pragma data_seg ()


CSX_DBG_EXT_DEFINE_CMD(bvd_irps, "bvd_irps")
{
    fbe_dbgext_ptr io_in_progress_ptr = 0;
    fbe_dbgext_ptr io_queue_entry_ptr = 0;
    fbe_dbgext_ptr cpu_count_ptr = 0;
    fbe_cpu_id_t cpu_count;
    fbe_cpu_id_t cpu_id = 0;
    fbe_dbgext_ptr shim_stats_data_ptr = 0;
    fbe_sep_shim_perf_stat_per_core_t fbe_sep_shim_stats_data_per_core;
    fbe_u32_t fbe_sep_shim_perf_stat_per_core_size;
    fbe_u32_t fbe_multicore_queue_entry_size;
    fbe_dbgext_ptr io_queue_entry_per_core_ptr;
    fbe_dbgext_ptr shim_stats_data_per_core_ptr;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t ptr_size;
    fbe_u32_t check_time_limit = 0;
    fbe_bool_t b_verbose = FBE_FALSE;
    fbe_bool_t b_summary = FBE_FALSE;
    fbe_bool_t b_list = FBE_FALSE;
    fbe_object_id_t print_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t packet_object_id = FBE_OBJECT_ID_INVALID;

    const fbe_u8_t * module_name = "sep"; 

    status = fbe_debug_get_ptr_size(module_name, &ptr_size);
    if (status != FBE_STATUS_OK) 
    {
        fbe_debug_trace_func(NULL, "%s unable to get ptr size status: %d\n", __FUNCTION__, status);
        return ; 
    }

    /* Display packets also */
    if (strlen(args) && strncmp(args, "-v", 2) == 0)
    {
        b_verbose = FBE_TRUE;
        args += FBE_MIN(3, strlen(args));
    }
    else if (strlen(args) && strncmp(args, "-s", 2) == 0)
    {
        b_summary = FBE_TRUE;
        args += FBE_MIN(3, strlen(args));
        fbe_debug_set_display_queue_header(FBE_FALSE);
    }
    else if (strlen(args) && strncmp(args, "-l", 2) == 0)
    {
        b_list = FBE_TRUE;
        args += FBE_MIN(3, strlen(args));
    }
	if (strlen(args) && strncmp(args, "-help", 5) == 0)
    {
       fbe_debug_trace_func(NULL, "!bvd_irps\n"
								  "  Displays BVD IRPs and associated packet's info.\n"
								  "  -v - This option displays all the packets details\n"
                                  "  -t <sec> - This option displays all the packets not finished within number of seconds\n");
        return;
    }
    if (strlen(args) && strncmp(args, "-t", 2) == 0)
    {
        args += FBE_MIN(3, strlen(args));

        /* Get and display our packet ptr.
         */
        if (strlen(args))
        {
            fbe_debug_trace_func(NULL, "display : %d \n", (int)GetArgument64(args, 1));
            check_time_limit = (fbe_u32_t) GetArgument64(args, 1);
        }
    }
    
    /* Get the fbe_sep_shim_io_in_progress_queue. */
    FBE_GET_EXPRESSION(pp_ext_module_name, fbe_sep_shim_io_in_progress_queue, &io_in_progress_ptr);
    FBE_READ_MEMORY(io_in_progress_ptr, &io_queue_entry_ptr, ptr_size);
    fbe_trace_indent(fbe_debug_trace_func, NULL, 4);
    fbe_debug_trace_func(NULL, "io_queue_ptr %llx\n",
		         (unsigned long long)io_in_progress_ptr);
    fbe_trace_indent(fbe_debug_trace_func, NULL, 4);
    fbe_debug_trace_func(NULL, "io_queue_entry_ptr(head) %llx\n",
		         (unsigned long long)io_queue_entry_ptr);
    if(io_in_progress_ptr == 0)
    {
        fbe_trace_indent(fbe_debug_trace_func, NULL, 4);
        fbe_debug_trace_func(NULL, "fbe_sep_shim_io_in_progress_queue is not available \n");
        return;
    }

    /* Get the CPU count */
    FBE_GET_EXPRESSION(module_name, fbe_sep_shim_cpu_count, &cpu_count_ptr);
    FBE_READ_MEMORY(cpu_count_ptr, &cpu_count, sizeof(fbe_cpu_id_t));
    fbe_trace_indent(fbe_debug_trace_func, NULL, 4);
    cpu_count = cpu_count & 0x3f;
    fbe_debug_trace_func(NULL, "cpu_count 0x%x\n", cpu_count);

    /* Get the IO statistics */
    FBE_GET_EXPRESSION(pp_ext_module_name, fbe_sep_shim_stats_data, &shim_stats_data_ptr);
    fbe_trace_indent(fbe_debug_trace_func, NULL, 4);
    fbe_debug_trace_func(NULL, "fbe_sep_shim_stats_data_ptr %llx\n",
			 (unsigned long long)shim_stats_data_ptr);
    if(shim_stats_data_ptr == 0)
    {
        fbe_trace_indent(fbe_debug_trace_func, NULL, 4);
        fbe_debug_trace_func(NULL, "fbe_sep_shim_stats_data is not available \n");
        return;
    }
    FBE_GET_TYPE_SIZE(module_name, fbe_sep_shim_perf_stat_per_core_t, &fbe_sep_shim_perf_stat_per_core_size);
    FBE_GET_TYPE_SIZE(module_name, fbe_multicore_queue_entry_t, &fbe_multicore_queue_entry_size);
    fbe_trace_indent(fbe_debug_trace_func, NULL, 4);
    fbe_debug_trace_func(NULL, "fbe_sep_shim_stats_data size:0x%x \n",fbe_sep_shim_perf_stat_per_core_size);

    /* Loop through each queue for each CPU*/
    for(cpu_id = 0; cpu_id < cpu_count; cpu_id++)
    {
        shim_stats_data_per_core_ptr = shim_stats_data_ptr + (fbe_sep_shim_perf_stat_per_core_size * cpu_id);
        FBE_READ_MEMORY(shim_stats_data_per_core_ptr, &fbe_sep_shim_stats_data_per_core, sizeof(fbe_sep_shim_perf_stat_per_core_t));
        /* Display IO stats data for each CPU*/
        fbe_debug_trace_func(NULL,"\n");
        fbe_debug_trace_func(NULL, "cpu_id: %d\n", cpu_id);
        fbe_trace_indent(fbe_debug_trace_func, NULL, 4);
        fbe_debug_trace_func(NULL, "total_ios: %llx\n", fbe_sep_shim_stats_data_per_core.total_ios);
        fbe_trace_indent(fbe_debug_trace_func, NULL, 4);
        fbe_debug_trace_func(NULL, "current_ios_in_progress: %llx\n", fbe_sep_shim_stats_data_per_core.current_ios_in_progress);
        fbe_trace_indent(fbe_debug_trace_func, NULL, 4);
        fbe_debug_trace_func(NULL, "max_io_q_depth: %llx\n", fbe_sep_shim_stats_data_per_core.max_io_q_depth);
        io_queue_entry_per_core_ptr = io_queue_entry_ptr + (fbe_multicore_queue_entry_size * cpu_id);
        
        packet_object_id = fbe_sep_shim_io_struct_queue_debug(module_name,io_queue_entry_per_core_ptr,
                                                              b_verbose, b_summary, b_list, 
                                                              check_time_limit,fbe_debug_trace_func,NULL);
        if (print_object_id == FBE_OBJECT_ID_INVALID)
        {
            print_object_id = packet_object_id;
        }
    }

    fbe_debug_trace_func(NULL,"\n");
    if (print_object_id != FBE_OBJECT_ID_INVALID)
    {
        fbe_topology_print_object_by_id(module_name, 
                                    FBE_CLASS_ID_INVALID, 
                                    print_object_id, 
                                    FBE_FALSE,
                                    FBE_FALSE,
                                        b_summary,
                                    fbe_debug_trace_func,
                                    NULL,
                                    0);
    }
    fbe_debug_set_display_queue_header(FBE_TRUE);

    return;
}

//end of bvd_irps.c
