#ifndef FBE_SEP_SHIM_DEBUG_H
#define FBE_SEP_SHIM_DEBUG_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2010-2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_sep_shim_debug.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the interfaces of the sep Shim / BVD Shim debug library.
 *
 * @author
 *  04-May-2011 - Created. Hari Singh Chauhan
 *
 ***************************************************************************/
/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_types.h"
#include "fbe/fbe_dbgext.h"
#include "fbe_trace.h"


fbe_status_t fbe_sep_shim_io_struct_queue_debug(const fbe_u8_t * module_name,
                                                fbe_dbgext_ptr io_queue_ptr,
												fbe_bool_t verbose_display,
                                                fbe_bool_t b_summary,
                                                fbe_bool_t b_list,
												fbe_u32_t check_time_limit,
                                                fbe_trace_func_t trace_func,
                                                fbe_trace_context_t trace_context);

fbe_status_t fbe_sep_shim_io_struct_debug_trace(const fbe_u8_t * module_name,
                                                fbe_dbgext_ptr io_data_ptr,
										        fbe_bool_t verbose_display,
												fbe_u32_t check_time_limit,
                                                fbe_trace_func_t trace_func,
                                                fbe_trace_context_t trace_context);


#endif  /* FBE_SEP_SHIM_DEBUG_H*/
