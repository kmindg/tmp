/***************************************************************************
 * Copyright (C) EMC Corporation 2001-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!***************************************************************************
 * @file    fbe_api_dps_memory_interface.c
 *****************************************************************************
 *
 * @brief   This file contains the APIs for the DPS memory service.  This
 *          includes APIs to display the memory statistics
 * 
 * @ingroup fbe_api_storage_extent_package_interface_class_files
 * @ingroup fbe_api_dps_memory_interface
 *
 *
 *****************************************************************************/

/**************************************
                Includes
**************************************/
#include "fbe_trace.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_library_interface.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_api_dps_memory_interface.h"
#include "fbe/fbe_service.h"

/**************************************
                Local variables
**************************************/
static fbe_bool_t last_memory_dps_statistics_valid = FBE_FALSE;
static fbe_memory_dps_statistics_t last_memory_dps_statistics;
/*******************************************
                Local functions
********************************************/

static void fbe_api_dps_memory_default_trace_func(fbe_trace_context_t trace_context, const fbe_char_t * fmt, ...)
{
    va_list ap;
    fbe_char_t string_to_print[256];

    va_start(ap, fmt);
    _vsnprintf(string_to_print, (sizeof(string_to_print) - 1), fmt, ap);
    va_end(ap);
    printf("%s", string_to_print);
    return;
}

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

#define fbe_trace_indent fbe_trace_indent_local
static void fbe_trace_indent(fbe_trace_func_t trace_func,
                      fbe_trace_context_t trace_context,
                      fbe_u32_t spaces)
{
    /* Simply display the input number of spaces.
     */
    while (spaces > 0)
    {
        trace_func(trace_context, " ");
        spaces--;
    }
    return;
}

fbe_status_t FBE_API_CALL fbe_api_dps_memory_add_more_memory(fbe_package_id_t package_id)
{
    fbe_status_t                              status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t   status_info;

    status = fbe_api_common_send_control_packet_to_service(FBE_MEMORY_SERVICE_CONTROL_CODE_DPS_ADD_MEMORY,
                                                           NULL,  // no input
                                                           0,
                                                           FBE_SERVICE_ID_MEMORY,
                                                           FBE_PACKET_FLAG_NO_ATTRIB,
                                                           &status_info,
                                                           package_id);
    /* Always return the execution status
     */
    return(status);
}


/*!***************************************************************************
 *          fbe_api_dps_memory_get_dps_statistics()
 ***************************************************************************** 
 * 
 * @brief   Get DPS statistics from the memory service
 *
 * @param   api_info_p - Pointer to sector api information block
 *
 * @return  fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 * 
 * @note    This request is only applicable for the FBE_PACKAGE_ID_SEP_0
 *          package.
 *
 * @author
 *  04/04/2011  Swati Fursule  - Created
 *
 *****************************************************************************/
fbe_status_t FBE_API_CALL fbe_api_dps_memory_get_dps_statistics(fbe_memory_dps_statistics_t *api_info_p,
                                                                fbe_package_id_t package_id)
{
    fbe_status_t                              status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t   status_info;
    fbe_memory_dps_statistics_t          get_stats;

    /* Validate the request
     */
    if (api_info_p == NULL)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, 
                      "%s: null api_info_p pointer \n", 
                      __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Send out the request to populate the dps information
     * structure.  This request will only work when sent to the SEP_0 
     * package. 
     */
    fbe_zero_memory((void *)&get_stats, sizeof(fbe_memory_dps_statistics_t));
    status = fbe_api_common_send_control_packet_to_service(FBE_MEMORY_SERVICE_CONTROL_CODE_GET_DPS_STATS,
                                                           &get_stats,
                                                           sizeof(fbe_memory_dps_statistics_t),
                                                           FBE_SERVICE_ID_MEMORY,
                                                           FBE_PACKET_FLAG_NO_ATTRIB,
                                                           &status_info,
                                                           package_id);
    if (status != FBE_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: can't send control packet\n", __FUNCTION__); 
        return status;
    }
    if (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: get request failed\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Copy dps statistics to output parameter     */
    fbe_copy_memory (api_info_p, 
                     &get_stats,
                     sizeof(fbe_memory_dps_statistics_t));
    

    /* Always return the execution status
     */
    return(status);
}
/* end of fbe_api_dps_memory_get_dps_statistics() */

/*!***************************************************************************
 *          fbe_api_dps_memory_display_dps_count_by_pool_priority()
 ***************************************************************************** 
 * 
 * @brief   Describes (i.e. dumps to trace function) DPS information along with 
 *          exact values
 *
 * @param   counts_p - Pointer to DPS information to describe
 * @param   trace_func - The trace function to describe into
 * @param   trace_context - Context to use with trace function
 * @param   spaces_to_indent - spaces for indentation
 * 
 * @return  fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @author
 *  05/05/2011  Swati Fursule  - Created
 *
 *****************************************************************************/
static fbe_status_t fbe_api_dps_memory_display_dps_count_by_pool_priority(fbe_u64_t *counts_p,
                                                           fbe_trace_func_t trace_func,
                                                           fbe_trace_context_t trace_context,
                                                           fbe_u32_t spaces_to_indent,
                                                           fbe_u8_t* count_str)

{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_u32_t       pool_idx = 0;
    fbe_u32_t       priority_idx = 0;
    fbe_u64_t       the_count[FBE_MEMORY_DPS_QUEUE_ID_LAST][FBE_MEMORY_DPS_MAX_PRIORITY];

    /* Copy dps statistics to output parameter     */
    fbe_copy_memory (&the_count, 
                     counts_p,
                     sizeof(fbe_u64_t)*FBE_MEMORY_DPS_QUEUE_ID_LAST* FBE_MEMORY_DPS_MAX_PRIORITY);

    trace_func(trace_context, "                     DPS MEMORY STATISTICS by Pool and Priority                         \n");
    if (count_str != NULL)
    {
        trace_func(trace_context, "                    ------------(%s)-------------                       \n", count_str);
    }
    trace_func(trace_context, "                    ----------------------------------------                        \n");

    trace_func(trace_context, "Pool=>               ");
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    trace_func(trace_context, "Main     ");
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    trace_func(trace_context, "Packet    ");
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    trace_func(trace_context, "32-Blk    ");
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    trace_func(trace_context, "64-Blk    ");
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    trace_func(trace_context, "Total\n");

    trace_func(trace_context, "----------           ");
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    trace_func(trace_context, "----------");
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    trace_func(trace_context, "----------");
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    trace_func(trace_context, "----------");
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    trace_func(trace_context, "----------");
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    trace_func(trace_context, "----------\n");

    for(priority_idx =0; priority_idx<FBE_MEMORY_DPS_MAX_PRIORITY; priority_idx++)
    {
        trace_func(trace_context, "Priority %2d    ", priority_idx);
        for(pool_idx =0; pool_idx<FBE_MEMORY_DPS_QUEUE_ID_LAST; pool_idx++)
        {
            fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
            trace_func(trace_context, "%10llu",
		       (unsigned long long)the_count[pool_idx][priority_idx]);
        }
        trace_func(trace_context, "\n");
    }

    /* Always return the executiuon status
     */
    return(status);
}
/* end fbe_api_dps_memory_display_dps_count_by_pool_priority */
/*!***************************************************************************
 *          fbe_api_dps_memory_describe_info()
 ***************************************************************************** 
 * 
 * @brief   Describes (i.e. dumps to trace function) DPS information along with 
 *          exact values
 *
 * @param   dps_statistics_info_p - Pointer to DPS information to describe
 * @param   trace_func - The trace function to describe into
 * @param   trace_context - Context to use with trace function
 * @param   b_verbose - Determinse if verbose information is dumped or not
 * @param   spaces_to_indent - spaces for indentation
 * 
 * @return  fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @author
 *  04/04/2011  Swati Fursule  - Created
 *
 *****************************************************************************/
fbe_status_t FBE_API_CALL fbe_api_dps_memory_describe_info(fbe_memory_dps_statistics_t *dps_statistics_info_p,
                                                           fbe_trace_func_t trace_func,
                                                           fbe_trace_context_t trace_context,
                                                           fbe_bool_t b_verbose,
                                                           fbe_u32_t spaces_to_indent,
                                                           fbe_api_dps_display_category_t category)
{
    fbe_u32_t       pool_idx = 0;
	fbe_cpu_id_t	core_count;
	fbe_cpu_id_t	cpu_id;

    /* Validate arguments
     */
    if ((dps_statistics_info_p == NULL) ||
        (trace_func == NULL)       )
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR,
                      "%s - dps_statistics_info_p: 0x%p and trace_func: 0x%p must be valid \n",
                      __FUNCTION__, dps_statistics_info_p, trace_func);
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    /* If verbose was selected, display the meaning of each statistics
     */
    if (b_verbose == FBE_TRUE)
    {
    }

	if(category == FBE_API_DPS_DISPLAY_FAST_POOL_ONLY ||
		(category == FBE_API_DPS_DISPLAY_FAST_POOL_ONLY_DIFF && last_memory_dps_statistics_valid == FBE_FALSE)){

		core_count = fbe_get_cpu_count();
	    trace_func(trace_context, "DPS FAST CONTROL POOL STATISTICS\n");
		for(pool_idx = FBE_MEMORY_DPS_QUEUE_ID_FOR_PACKET; pool_idx < FBE_MEMORY_DPS_QUEUE_ID_LAST; pool_idx++){ /* Loop over queues */
			for(cpu_id = 0; cpu_id < core_count; cpu_id++){
				trace_func(trace_context, "Core %d, chunks %d free_chunks %d\n", cpu_id, 
                           dps_statistics_info_p->fast_pool_number_of_chunks[pool_idx][cpu_id],
                           dps_statistics_info_p->fast_pool_number_of_free_chunks[pool_idx][cpu_id]);
			}
		}
	    trace_func(trace_context, "DPS FAST DATA POOL STATISTICS\n");
		for(pool_idx = FBE_MEMORY_DPS_QUEUE_ID_FOR_PACKET; pool_idx < FBE_MEMORY_DPS_QUEUE_ID_LAST; pool_idx++){ /* Loop over queues */
			for(cpu_id = 0; cpu_id < core_count; cpu_id++){
				trace_func(trace_context, "Core %d, chunks %d free_chunks %d\n", cpu_id, 
                           dps_statistics_info_p->fast_pool_number_of_data_chunks[pool_idx][cpu_id],
                           dps_statistics_info_p->fast_pool_number_of_free_data_chunks[pool_idx][cpu_id]);
			}
		}

		for(pool_idx = FBE_MEMORY_DPS_QUEUE_ID_FOR_PACKET; pool_idx < FBE_MEMORY_DPS_QUEUE_ID_LAST; pool_idx++){ /* Loop over queues */
            trace_func(trace_context, "\nQ_id: %d C total/free: %lld/%lld D total/free: %lld/%lld Queued: %lld\n", 
                       pool_idx,
                       dps_statistics_info_p->number_of_chunks[pool_idx],
                       dps_statistics_info_p->number_of_free_chunks[pool_idx],
                       dps_statistics_info_p->number_of_data_chunks[pool_idx],
                       dps_statistics_info_p->number_of_free_data_chunks[pool_idx],
                       dps_statistics_info_p->request_queue_count[pool_idx]);


			for(cpu_id = 0; cpu_id < core_count; cpu_id++){
				trace_func(trace_context, "Fast pool: Core %d, control request count %lld data request count %lld \n", 
                           cpu_id, 
                           dps_statistics_info_p->fast_pool_request_count[pool_idx][cpu_id],
                           dps_statistics_info_p->fast_pool_data_request_count[pool_idx][cpu_id]);
			}
            for(cpu_id = 0; cpu_id < core_count; cpu_id++){
                trace_func(trace_context, "Core %d, control request/deferred count %lld/%lld data request/deferred count %lld/%lld \n", 
                           cpu_id, 
                           dps_statistics_info_p->request_count[pool_idx][cpu_id],
                           dps_statistics_info_p->deferred_count[pool_idx][cpu_id],
                           dps_statistics_info_p->request_data_count[pool_idx][cpu_id],
                           dps_statistics_info_p->deferred_data_count[pool_idx][cpu_id]);
            }
		}
        for(cpu_id = 0; cpu_id < core_count; cpu_id++){
            trace_func(trace_context, "Core %d, deadlock count %lld \n", 
                       cpu_id, dps_statistics_info_p->deadlock_count[cpu_id]);
        }
		return FBE_STATUS_OK;
	}

	if(category == FBE_API_DPS_DISPLAY_FAST_POOL_ONLY_DIFF){
		core_count = fbe_get_cpu_count();
	    trace_func(trace_context, "DPS FAST POOL STATISTICS DIFF\n");
		for(pool_idx = FBE_MEMORY_DPS_QUEUE_ID_FOR_PACKET; pool_idx < FBE_MEMORY_DPS_QUEUE_ID_LAST; pool_idx++){ /* Loop over queues */
			for(cpu_id = 0; cpu_id < core_count; cpu_id++){
				trace_func(trace_context, "Core %d, CONTROL chunks %d free_chunks %d DATA chunks %d free_chunks %d\n", cpu_id, 
                           dps_statistics_info_p->fast_pool_number_of_chunks[pool_idx][cpu_id],
                           dps_statistics_info_p->fast_pool_number_of_free_chunks[pool_idx][cpu_id],
                           dps_statistics_info_p->fast_pool_number_of_data_chunks[pool_idx][cpu_id],
                           dps_statistics_info_p->fast_pool_number_of_free_data_chunks[pool_idx][cpu_id]);
			}
		}

		for(pool_idx = FBE_MEMORY_DPS_QUEUE_ID_FOR_PACKET; pool_idx < FBE_MEMORY_DPS_QUEUE_ID_LAST; pool_idx++){ /* Loop over queues */
            for(cpu_id = 0; cpu_id < core_count; cpu_id++){
			trace_func(trace_context, "\nQ_id: %d Core %d, Requests: %lld Deferred: %lld \n", 
                       pool_idx, cpu_id,
                       dps_statistics_info_p->request_count[pool_idx][cpu_id] - last_memory_dps_statistics.request_count[pool_idx][cpu_id],
                       dps_statistics_info_p->deferred_count[pool_idx][cpu_id] - last_memory_dps_statistics.deferred_count[pool_idx][cpu_id]);

            trace_func(trace_context, "Fast pool: Core %d, request count %lld \n", cpu_id, 
                       dps_statistics_info_p->fast_pool_request_count[pool_idx][cpu_id] - last_memory_dps_statistics.fast_pool_request_count[pool_idx][cpu_id]);
			}
		}

		return FBE_STATUS_OK;
	}

	return FBE_STATUS_OK;
}
/* end of fbe_api_dps_memory_describe_info() */


/*!***************************************************************************
 *          fbe_api_dps_memory_display_statistics()
 ***************************************************************************** 
 * 
 * @brief   Dump the DPS information (item count, allocation count etc)
 *           to the standard output.
 *  
 * @param   b_verbose - Display detailed information or not
 * @param   b_stats_by_pool - Display stats by pool
 *
 * @return  status - Typically FBE_STATUS_OK
 *
 * @author
 *  04/04/2011  Swati Fursule  - Created
 *
 *****************************************************************************/
fbe_status_t FBE_API_CALL fbe_api_dps_memory_display_statistics(fbe_bool_t b_verbose, fbe_api_dps_display_category_t category, fbe_package_id_t package_id)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_memory_dps_statistics_t dps_stats;
    fbe_u32_t                                    spaces_to_indent = 4;

    /* First get the dps memory statistics information */
    status = fbe_api_dps_memory_get_dps_statistics(&dps_stats, package_id);
    if (status != FBE_STATUS_OK) {
        return(status);
    }
	
    /* Now `describe' (i.e. print) the statistics information using our
     * print function.
     */
    status = fbe_api_dps_memory_describe_info(&dps_stats,
                                                   fbe_api_dps_memory_default_trace_func,
                                                   (fbe_trace_context_t)NULL,
                                                   b_verbose,
                                                   spaces_to_indent,
                                                   category);
    /* Always return the execution status */
	fbe_copy_memory(&last_memory_dps_statistics, &dps_stats, sizeof(fbe_memory_dps_statistics_t));
	last_memory_dps_statistics_valid = FBE_TRUE;

    return(status);
}
/* end of fbe_api_dps_memory_display_statistics()*/

/*!***************************************************************************
 *          fbe_api_memory_get_env_limits()
 ***************************************************************************** 
 * 
 * @brief   Get environmnet limits from the memory service
 *
 * @param   env_limits - Pointer to fbe_environment_limits_t
 *
 * @return  fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 * 
 *
 * @author
 *  06/28/2012  Vera Wang  - Created
 *
 *****************************************************************************/
fbe_status_t FBE_API_CALL fbe_api_memory_get_env_limits(fbe_environment_limits_t *env_limits)
{
    fbe_status_t                              status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t   status_info;
    fbe_environment_limits_t                  get_env_limits;

    /* Validate the request
     */
    if (env_limits == NULL)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, 
                      "%s: null env_limits pointer \n", 
                      __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Send out the request to memory service to populate the environmnet limits information
     * structure.
     */
    fbe_zero_memory((void *)&get_env_limits, sizeof(fbe_environment_limits_t));
    status = fbe_api_common_send_control_packet_to_service(FBE_MEMORY_SERVICE_CONTROL_CODE_GET_ENV_LIMITS,
                                                           &get_env_limits,
                                                           sizeof(fbe_environment_limits_t),
                                                           FBE_SERVICE_ID_MEMORY,
                                                           FBE_PACKET_FLAG_NO_ATTRIB,
                                                           &status_info,
                                                           FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: can't send control packet\n", __FUNCTION__); 
        return status;
    }
    if (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: get request failed\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Copy env_limits to output parameter     */
    fbe_copy_memory (env_limits, 
                     &get_env_limits,
                     sizeof(fbe_environment_limits_t));  

    /* Always return the execution status
     */
    return status;
}
/* end of fbe_api_memory_get_env_limits() */

/*!**************************************************************
 * fbe_api_persistent_memory_set_params()
 ****************************************************************
 * @brief
 *  Set the parameters into the memory persistence service.
 *
 * @param set_params_p - pointer to parameters to set.               
 *
 * @return fbe_status_t
 *
 * @author
 *  1/15/2014 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t FBE_API_CALL fbe_api_persistent_memory_set_params(fbe_persistent_memory_set_params_t *set_params_p)
{
    fbe_status_t                              status = FBE_STATUS_OK;
    fbe_api_control_operation_status_info_t   status_info;

    /* Validate the request
     */
    if (set_params_p == NULL)
    {
        fbe_api_trace(FBE_TRACE_LEVEL_ERROR, 
                      "%s: null params pointer \n", 
                      __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_api_common_send_control_packet_to_service(FBE_MEMORY_SERVICE_CONTROL_CODE_PERSIST_SET_PARAMS,
                                                           set_params_p,
                                                           sizeof(fbe_persistent_memory_set_params_t),
                                                           FBE_SERVICE_ID_MEMORY,
                                                           FBE_PACKET_FLAG_NO_ATTRIB,
                                                           &status_info,
                                                           FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: can't send control packet\n", __FUNCTION__); 
        return status;
    }
    if (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: get request failed\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}
/******************************************
 * end fbe_api_persistent_memory_set_params()
 ******************************************/

/*!**************************************************************
 * fbe_api_persistent_memory_init_params()
 ****************************************************************
 * @brief
 *  Set the parameters into the memory persistence service.
 *
 * @param set_params_p - pointer to parameters to set.               
 *
 * @return fbe_status_t
 *
 * @author
 *  1/15/2014 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_api_persistent_memory_init_params(fbe_persistent_memory_set_params_t *params_p)
{
    fbe_zero_memory(params_p, sizeof(fbe_persistent_memory_set_params_t));
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_api_persistent_memory_init_params()
 ******************************************/

fbe_status_t FBE_API_CALL fbe_api_dps_memory_reduce_size(fbe_memory_dps_init_parameters_t * control_params, 
														 fbe_memory_dps_init_parameters_t * data_params)
{
    fbe_status_t                              status = FBE_STATUS_OK;
	fbe_api_control_operation_status_info_t   status_info;


    status = fbe_api_common_send_control_packet_to_service(FBE_MEMORY_SERVICE_CONTROL_CODE_DPS_REDUCE_SIZE,
                                                           NULL,
                                                           0,
                                                           FBE_SERVICE_ID_MEMORY,
                                                           FBE_PACKET_FLAG_NO_ATTRIB,
                                                           &status_info,
                                                           FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: can't send control packet\n", __FUNCTION__); 
        return status;
    }

    if (status_info.control_operation_status != FBE_PAYLOAD_CONTROL_STATUS_OK) {
        fbe_api_trace (FBE_TRACE_LEVEL_ERROR, "%s: get request failed\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return(status);
}



/********************************************
 * end of fbe_api_dps_memory_interface.c
 *******************************************/
