#ifndef FBE_PMC_SHIM_DEBUG_H
#define FBE_PMC_SHIM_DEBUG_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_pmc_shim_debug.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the interfaces of the pmc shim debug library.
 *
 * @author 
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

/*************************
 *   FUNCTION DEFINITIONS
 *************************/


#include "fbe/fbe_types.h"
#include "fbe/fbe_dbgext.h"
#include "fbe_trace.h"

fbe_status_t fbe_pmc_shim_debug_trace_port_info(const fbe_u8_t * module_name, 
												fbe_dbgext_ptr pmc_port_array_ptr, 
												fbe_trace_func_t trace_func, 
												fbe_trace_context_t trace_context,
												fbe_u32_t io_port_number,
												fbe_u32_t io_portal_number
												);
fbe_status_t fbe_pmc_shim_debug_trace_port_handle_info(const fbe_u8_t * module_name, 
														fbe_dbgext_ptr pmc_port_array_ptr, 
														fbe_trace_func_t trace_func, 
														fbe_trace_context_t trace_context,
														fbe_u32_t io_port_handle
														);
#endif /* FBE_PMC_SHIM_DEBUG_H */

/*************************
 * end file fbe_pmc_shim_debug.h
 *************************/