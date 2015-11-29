/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_memory_dps_debug.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the debug functions for the dps memory statistics.
 *
 * @author
 *  03/24/2011 - Created. Swati Fursule
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_types.h"
#include "fbe/fbe_dbgext.h"
#include "fbe_base_object.h"
#include "fbe_base_object_debug.h"
#include "fbe_base_object_trace.h"
#include "fbe_transport_debug.h"
#include "fbe/fbe_memory.h"
#include "fbe_memory_private.h"
#include "pp_ext.h"


/*************************
 *   FUNCTION DEFINITIONS
 *************************/
 /*!*******************************************************************
 * @var fbe_memory_dps_stats_field_info
 *********************************************************************
 * @brief Information about each of the fields of memory dps stats
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_memory_dps_stats_field_info[] =
{
    /*  object is displayed first.
     */
    FBE_DEBUG_DECLARE_FIELD_INFO("Item_size", fbe_u64_t, FBE_TRUE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("Initial Count", fbe_u64_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("Current Count", fbe_u64_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("Low Count", fbe_u64_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("Allocation Count", fbe_u64_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("Free Count", fbe_u64_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("Pending Count", fbe_u64_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("Deferred Count", fbe_u64_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("Aborted Count", fbe_u64_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("Allocation Errors", fbe_u64_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("Free Errors", fbe_u64_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_memory_dps_stats_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        memory stats.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_memory_dps_statistics_t,
                              fbe_memory_dps_stats_struct_info,
                              &fbe_memory_dps_stats_field_info[0]);

/*!*******************************************************************
 * @var fbe_packet_debug_sub_packet_queue_info_t
 *********************************************************************
 * @brief This structure holds info used to display a memory request 
 *        queue using the "queue_element" field of the memory request.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_QUEUE_INFO(fbe_memory_request_t, "queue_element", 
                             FBE_TRUE, /* queue element is not the first field in the packet. */
                             fbe_memory_request_debug_queue_info,
                             fbe_memory_request_debug_trace_basic);

/*!*******************************************************************
 * @var fbe_memory_request_queues_field_info
 *********************************************************************
 * @brief Information about each memory request queue.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_memory_request_queues_field_info[] =
{
    FBE_DEBUG_DECLARE_FIELD_INFO_QUEUE("memory request queue head", fbe_queue_head_t, FBE_TRUE, 
                                       &fbe_memory_request_debug_queue_info),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_memory_request_queues_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        queues of memory requests.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_memory_request_t,
                              fbe_memory_request_queues_struct_info,
                              &fbe_memory_request_queues_field_info[0]);

/*!**************************************************************
 * fbe_memory_dps_debug_display_wait_queues()
 ****************************************************************
 * @brief
 *  Display the memory requests on the dps waiting queues.
 * 
 * @param module_name - Module name to use in symbol lookup.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param spaces_to_indent - Number of spaces to indent the next line(s).   
 *
 * @return -   
 *
 * @author
 *  4/29/2011 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_memory_dps_debug_display_wait_queues(const fbe_u8_t * module_name,
                                                      fbe_trace_func_t trace_func,
                                                      fbe_trace_context_t trace_context,
                                                      fbe_u32_t spaces_to_indent)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t ptr_size;
    fbe_u64_t queue_head_p = 0;
    fbe_u64_t memory_dps_request_queue_count[FBE_MEMORY_DPS_QUEUE_ID_LAST];
    fbe_u64_t count_p = 0;
    fbe_u32_t i;

    /* check if active queue display is disabled using !disable_active_queue_display debug macro
    */
    if(get_active_queue_display_flag() == FBE_FALSE)
    {
        return FBE_STATUS_OK;
    }
    trace_func(trace_context, "\n\n dps request queues\n");
    trace_func(trace_context, "--------------------------\n");
    FBE_GET_TYPE_SIZE(module_name, fbe_u32_t*, &ptr_size);

    FBE_GET_EXPRESSION(module_name, memory_dps_request_queue_count, &count_p);
    FBE_READ_MEMORY(count_p, &memory_dps_request_queue_count[0], sizeof(fbe_u64_t) * FBE_MEMORY_DPS_QUEUE_ID_LAST);

    FBE_GET_EXPRESSION(module_name, memory_dps_request_queue_head, &queue_head_p);
    
    for (i = 0; i < FBE_MEMORY_DPS_QUEUE_ID_LAST; i++) {
        trace_func(trace_context, "Q id %d waiters: %d", i, (int)memory_dps_request_queue_count[i]);
        status = fbe_debug_display_structure(module_name, trace_func, trace_context, queue_head_p,
                                             &fbe_memory_request_queues_struct_info,
                                             4 /* fields per line */,
                                             spaces_to_indent + 2);
        /* Advance to the next queue head.
         */
        queue_head_p += (ptr_size * 2);
    }
    return status;
}
/******************************************
 * end fbe_memory_dps_debug_display_wait_queues()
 ******************************************/

/*!**************************************************************
 * fbe_memory_dps_debug_display_request_count()
 ****************************************************************
 * @brief
 *  Display the counts provided by pool as well as priority basis..
 *  requested count
 * 
 * @param module_name - Module name to use in symbol lookup.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param spaces_to_indent - Number of spaces to indent the next line(s).   
 *
 * @return -   
 *
 * @author
 *  05/05/2011 - Created. Swati Fursule
 *
 ****************************************************************/

fbe_status_t fbe_memory_dps_debug_display_request_count(const fbe_u8_t * module_name,
                                                      fbe_trace_func_t trace_func,
                                                      fbe_trace_context_t trace_context,
                                                      fbe_u32_t spaces_to_indent)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t ptr1_size;
    fbe_u32_t pool_idx = 0;
    fbe_u64_t memory_dps_request_count[FBE_MEMORY_DPS_QUEUE_ID_LAST];
    fbe_u64_t count_p = 0;

    FBE_GET_TYPE_SIZE(module_name, fbe_u32_t*, &ptr1_size);
    FBE_GET_EXPRESSION(module_name, memory_dps_request_count, &count_p);
    FBE_READ_MEMORY(count_p, &memory_dps_request_count[0], sizeof(fbe_u64_t) * FBE_MEMORY_DPS_QUEUE_ID_LAST);

	trace_func(trace_context, "Requested counts :\n");

    for(pool_idx = 0; pool_idx < FBE_MEMORY_DPS_QUEUE_ID_LAST; pool_idx++) {
        fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
		trace_func(trace_context, " Pool id: %d request_count: %10lld\n",pool_idx, memory_dps_request_count[pool_idx]);		
    }
	trace_func(trace_context, "\n");        

    return status;
}
/******************************************
 * end fbe_memory_dps_debug_display_request_count()
 ******************************************/

/*!**************************************************************
 * fbe_memory_dps_debug_display_alloc_count()
 ****************************************************************
 * @brief
 *  Display the counts provided by pool as well as priority basis..
 *  allocation count
 * 
 * @param module_name - Module name to use in symbol lookup.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param spaces_to_indent - Number of spaces to indent the next line(s).   
 *
 * @return -   
 *
 * @author
 *  05/05/2011 - Created. Swati Fursule
 *
 ****************************************************************/

fbe_status_t fbe_memory_dps_debug_display_alloc_count(const fbe_u8_t * module_name,
                                                      fbe_trace_func_t trace_func,
                                                      fbe_trace_context_t trace_context,
                                                      fbe_u32_t spaces_to_indent)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t ptr1_size;
    fbe_u32_t pool_idx = 0;
    fbe_u64_t memory_pool_number_of_allocated_chunks[FBE_MEMORY_DPS_QUEUE_ID_LAST];
    fbe_u64_t count_p = 0;

    FBE_GET_TYPE_SIZE(module_name, fbe_u32_t*, &ptr1_size);
    FBE_GET_EXPRESSION(module_name, memory_pool_number_of_allocated_chunks, &count_p);
    FBE_READ_MEMORY(count_p, &memory_pool_number_of_allocated_chunks[0], sizeof(fbe_u64_t) * FBE_MEMORY_DPS_QUEUE_ID_LAST);

    for(pool_idx = 0; pool_idx < FBE_MEMORY_DPS_QUEUE_ID_LAST; pool_idx++)
    {
		trace_func(trace_context, "Pool id: %d Allocated chunks: %10lld\n",pool_idx, memory_pool_number_of_allocated_chunks[pool_idx]);
    }
    trace_func(trace_context, "\n");
    
    return status;
}
/******************************************
 * end fbe_memory_dps_debug_display_alloc_count()
 ******************************************/

/*!**************************************************************
 * fbe_memory_dps_debug_display_defer_count()
 ****************************************************************
 * @brief
 *  Display the counts provided by pool as well as priority basis..
 *  deferred count
 * 
 * @param module_name - Module name to use in symbol lookup.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param spaces_to_indent - Number of spaces to indent the next line(s).   
 *
 * @return -   
 *
 * @author
 *  05/05/2011 - Created. Swati Fursule
 *
 ****************************************************************/

fbe_status_t fbe_memory_dps_debug_display_defer_count(const fbe_u8_t * module_name,
                                                      fbe_trace_func_t trace_func,
                                                      fbe_trace_context_t trace_context,
                                                      fbe_u32_t spaces_to_indent)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t ptr1_size;
    fbe_u32_t pool_idx = 0;
    fbe_u64_t memory_dps_deferred_count[FBE_MEMORY_DPS_QUEUE_ID_LAST];
    fbe_u64_t count_p = 0;

    FBE_GET_TYPE_SIZE(module_name, fbe_u32_t*, &ptr1_size);
    FBE_GET_EXPRESSION(module_name, memory_dps_deferred_count, &count_p);
    FBE_READ_MEMORY(count_p, &memory_dps_deferred_count[0], sizeof(fbe_u64_t) * FBE_MEMORY_DPS_QUEUE_ID_LAST);

    for(pool_idx =0; pool_idx<FBE_MEMORY_DPS_QUEUE_ID_LAST; pool_idx++)
    {
		trace_func(trace_context, "Pool id: %d Deferred count: %10lld\n",pool_idx, memory_dps_deferred_count[pool_idx]);
    }
    trace_func(trace_context, "\n");
    
    return status;
}
/******************************************
 * end fbe_memory_dps_debug_display_defer_count()
 ******************************************/

/*!**************************************************************
 * fbe_memory_dps_debug_display_max_alloc_size()
 ****************************************************************
 * @brief
 *  Display the counts provided by pool as well as priority basis..
 * max allocation size
 * 
 * @param module_name - Module name to use in symbol lookup.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param spaces_to_indent - Number of spaces to indent the next line(s).   
 *
 * @return -   
 *
 * @author
 *  05/05/2011 - Created. Swati Fursule
 *
 ****************************************************************/

fbe_status_t fbe_memory_dps_debug_display_max_alloc_size(const fbe_u8_t * module_name,
                                                      fbe_trace_func_t trace_func,
                                                      fbe_trace_context_t trace_context,
                                                      fbe_u32_t spaces_to_indent)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t ptr1_size;
    fbe_u32_t pool_idx = 0;
    fbe_u64_t memory_pool_max_allocation_size[FBE_MEMORY_DPS_QUEUE_ID_LAST];
    fbe_u64_t count_p = 0;

    FBE_GET_TYPE_SIZE(module_name, fbe_u32_t*, &ptr1_size);
    FBE_GET_EXPRESSION(module_name, memory_pool_max_allocation_size, &count_p);
    FBE_READ_MEMORY(count_p, &memory_pool_max_allocation_size[0], sizeof(fbe_u64_t) * FBE_MEMORY_DPS_QUEUE_ID_LAST);


    for(pool_idx = 0; pool_idx < FBE_MEMORY_DPS_QUEUE_ID_LAST; pool_idx++)
    {
		trace_func(trace_context, "Pool id: %d max_allocation_size: %10lld",pool_idx, memory_pool_max_allocation_size[pool_idx]);
    }
    trace_func(trace_context, "\n");

	return status;
}
/******************************************
 * end fbe_memory_dps_debug_display_max_alloc_size()
 ******************************************/


/*!**************************************************************
 * fbe_memory_dps_debug_display_counts_by_pool()
 ****************************************************************
 * @brief
 *  Display the counts provided by pool basis..
 * 
 * 
 * @param module_name - Module name to use in symbol lookup.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param spaces_to_indent - Number of spaces to indent the next line(s).   
 *
 * @return -   
 *
 * @author
 *  05/05/2011 - Created. Swati Fursule
 *
 ****************************************************************/

fbe_status_t fbe_memory_dps_debug_display_counts_by_pool(const fbe_u8_t * module_name,
                                                      fbe_trace_func_t trace_func,
                                                      fbe_trace_context_t trace_context,
                                                      fbe_u32_t spaces_to_indent)
{
    fbe_u32_t ptr1_size;
    fbe_u32_t pool_idx = 0;
    fbe_u32_t memory_pool_number_of_chunks[FBE_MEMORY_DPS_QUEUE_ID_LAST];
    fbe_u32_t memory_pool_number_of_free_chunks[FBE_MEMORY_DPS_QUEUE_ID_LAST];
    fbe_u64_t count_p = 0;


	FBE_GET_TYPE_SIZE(module_name, fbe_u32_t*, &ptr1_size);

	FBE_GET_EXPRESSION(module_name, memory_pool_number_of_chunks, &count_p);
    FBE_READ_MEMORY(count_p, &memory_pool_number_of_chunks[0], sizeof(fbe_u64_t) * FBE_MEMORY_DPS_QUEUE_ID_LAST);

    FBE_GET_EXPRESSION(module_name, memory_pool_number_of_free_chunks, &count_p);
    FBE_READ_MEMORY(count_p, &memory_pool_number_of_free_chunks[0], sizeof(fbe_u64_t) * FBE_MEMORY_DPS_QUEUE_ID_LAST);

    for(pool_idx = 0; pool_idx < FBE_MEMORY_DPS_QUEUE_ID_LAST; pool_idx++)
    {
		trace_func(trace_context, "Pool id: %d Total chunks: %10d Free chunks: %10d\n",pool_idx,
																						memory_pool_number_of_chunks[pool_idx],
																						memory_pool_number_of_free_chunks[pool_idx]);
    }

    trace_func(trace_context, "\n");

	return FBE_STATUS_OK;
}
/******************************************
 * end fbe_memory_dps_debug_display_counts_by_pool()
 ******************************************/


/*!**************************************************************
 * fbe_memory_dps_statistics_debug_trace()
 ****************************************************************
 * @brief
 *  Display all the statistics information about fbe dps memory service.
 *
 * @param module_name - Module name to use in symbol lookup.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param base_p - This is the ptr to the structure.
 * @param spaces_to_indent - Number of spaces to indent the next line(s).
 * 
 * @return fbe_status_t   
 *
 * @author
 *  03/24/2011 - Created. Swati Fursule
 *
 ****************************************************************/

fbe_status_t fbe_memory_dps_statistics_debug_trace(const fbe_u8_t * module_name,
                                                  fbe_trace_func_t trace_func,
                                                  fbe_trace_context_t trace_context,
                                                  fbe_dbgext_ptr base_p,
                                                  fbe_u32_t spaces_to_indent)
{
    fbe_status_t    status = FBE_STATUS_OK;
    /* first display memory statistics by pool*/
    fbe_memory_dps_debug_display_counts_by_pool(module_name, trace_func, trace_context, spaces_to_indent);

    /* display memory statistics by pool as well as priority*/
    fbe_memory_dps_debug_display_request_count(module_name, trace_func, trace_context, spaces_to_indent);
    fbe_memory_dps_debug_display_alloc_count(module_name, trace_func, trace_context, spaces_to_indent);
    fbe_memory_dps_debug_display_defer_count(module_name, trace_func, trace_context, spaces_to_indent);
    fbe_memory_dps_debug_display_max_alloc_size(module_name, trace_func, trace_context, spaces_to_indent);

    fbe_memory_dps_debug_display_wait_queues(module_name, trace_func, trace_context, spaces_to_indent);

    /* Always return the executiuon status
     */
    return(status);

}
