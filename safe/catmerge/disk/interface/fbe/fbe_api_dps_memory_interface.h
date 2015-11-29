#ifndef FBE_API_DPS_MEMORY_INTERFACE_H
#define FBE_API_DPS_MEMORY_INTERFACE_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_api_trace_interface.h
 ***************************************************************************
 *
 * @brief
 *  This file contains definitions of functions use for interacting with the DPS memory service
 * @ingroup fbe_api_system_package_interface_class_files
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_api_common.h"
#include "fbe/fbe_memory.h"

//----------------------------------------------------------------
// Define the FBE API TRACE Interface for the Usurper Interface. 
// This is where all the data structures defined. 
//----------------------------------------------------------------
/*! @defgroup fbe_api_trace_interface_usurper_interface FBE API TRACE Interface Usurper Interface
 *  @brief 
 *    This is the set of definitions that comprise the FBE API TRACE Interface class
 *    usurper interface.
 *
 *  @ingroup fbe_api_classes_usurper_interface
 *  @{
 */ 
//----------------------------------------------------------------

FBE_API_CPP_EXPORT_START

typedef enum fbe_api_dps_display_category_e{
    /*! Display DPS stats by pool and priority both
     */
    FBE_API_DPS_DISPLAY_DEFAULT,
    /*! Display DPS stats by pool.
     */
    FBE_API_DPS_DISPLAY_BY_POOL,
    /*! Display DPS stats by priority.
     */
    FBE_API_DPS_DISPLAY_BY_PRIORITY,
    /*! Display DPS stats by pool and priority both
     */
    FBE_API_DPS_DISPLAY_BY_POOL_PRIORITY,

	/*! Display DPS fast pools stats */
	FBE_API_DPS_DISPLAY_FAST_POOL_ONLY,

	/*! Display DPS fast pools stats difference*/
	FBE_API_DPS_DISPLAY_FAST_POOL_ONLY_DIFF,

    FBE_API_DPS_DISPLAY_LAST
}fbe_api_dps_display_category_t;

fbe_status_t FBE_API_CALL fbe_api_dps_memory_get_dps_statistics(fbe_memory_dps_statistics_t *api_info_p,
                                                                fbe_package_id_t package_id);

fbe_status_t FBE_API_CALL fbe_api_dps_memory_add_more_memory(fbe_package_id_t package_id);

fbe_status_t FBE_API_CALL fbe_api_dps_memory_display_statistics(fbe_bool_t b_verbose, fbe_u32_t priority_idx,
                                                                fbe_package_id_t package_id);
fbe_status_t FBE_API_CALL fbe_api_dps_memory_describe_info(fbe_memory_dps_statistics_t *dps_statistics_info_p,
                                                           fbe_trace_func_t trace_func,
                                                           fbe_trace_context_t trace_context,
                                                           fbe_bool_t b_verbose,
                                                           fbe_u32_t spaces_to_indent,
                                                           fbe_u32_t category);
fbe_status_t FBE_API_CALL fbe_api_memory_get_env_limits(fbe_environment_limits_t *env_limits);
fbe_status_t FBE_API_CALL fbe_api_persistent_memory_set_params(fbe_persistent_memory_set_params_t *set_params_p);

fbe_status_t FBE_API_CALL fbe_api_dps_memory_reduce_size(fbe_memory_dps_init_parameters_t * control_params, 
														 fbe_memory_dps_init_parameters_t * data_params);




FBE_API_CPP_EXPORT_END
#endif /*FBE_API_DPS_MEMORY_INTERFACE_H*/
