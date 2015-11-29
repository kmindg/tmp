#ifndef FBE_EXTENT_POOL_DEBUG_H
#define FBE_EXTENT_POOL_DEBUG_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_extent_pool_debug.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the interfaces of the provision drive debug library.
 *
 * @author
 *  06/27/2014 - Created. Lili Chen
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_types.h"
#include "fbe/fbe_dbgext.h"
#include "fbe_trace.h"


fbe_status_t fbe_extent_pool_debug_trace(const fbe_u8_t * module_name,
                                        fbe_dbgext_ptr extent_pool_p,
                                        fbe_trace_func_t trace_func,
                                        fbe_trace_context_t trace_context,
                                        fbe_debug_field_info_t *field_info_p,
                                        fbe_u32_t spaces_to_indent);

fbe_status_t fbe_extent_pool_debug_display_terminator_queue(const fbe_u8_t * module_name,
                                                           fbe_dbgext_ptr extent_pool_p,
                                                           fbe_trace_func_t trace_func,
                                                           fbe_trace_context_t trace_context,
                                                           fbe_u32_t spaces_to_indent,
                                                           fbe_bool_t b_summarize);

#endif /* FBE_EXTENT_POOL_DEBUG_H */

/*************************
 * end file fbe_extent_pool_debug.h
 *************************/
