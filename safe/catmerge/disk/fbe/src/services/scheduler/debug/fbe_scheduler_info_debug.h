#ifndef FBE_SCHEDULER_INFO_DEBUG_H
#define FBE_SCHEDULER_INFO_DEBUG_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_scheduler_info_debug.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the interfaces of the scheduler debug interface.
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
#include "fbe_trace.h"

fbe_status_t fbe_scheduler_queue_debug_trace(const fbe_u8_t * module_name,
                                             fbe_dbgext_ptr scheduler_element_ptr,
											 fbe_u32_t display_format,
                                             fbe_trace_func_t trace_func,
                                             fbe_trace_context_t trace_context);
fbe_status_t fbe_scheduler_queue_type_debug_trace(const fbe_u8_t * module_name,
                                                  fbe_dbgext_ptr scheduler_element_ptr,
                                                  fbe_trace_func_t trace_func,
                                                  fbe_trace_context_t trace_context,
                                                  fbe_debug_field_info_t *field_info_p,
                                                  fbe_u32_t spaces_to_indent);
fbe_status_t fbe_scheduler_thread_info_debug_trace(const fbe_u8_t * module_name,
                                                   fbe_dbgext_ptr thread_info_ptr,
                                                   fbe_trace_func_t trace_func,
                                                   fbe_trace_context_t trace_context);
fbe_status_t fbe_scheduler_debug_hooks_debug_trace(const fbe_u8_t * module_name,
                                                   fbe_dbgext_ptr debug_hook_ptr,
                                                   fbe_trace_func_t trace_func,
                                                   fbe_trace_context_t trace_context);
fbe_status_t fbe_scheduler_core_credit_table_debug_trace(const fbe_u8_t * module_name,
                                                         fbe_dbgext_ptr core_credits_ptr,
                                                         fbe_trace_func_t trace_func,
                                                         fbe_trace_context_t trace_context);
fbe_status_t fbe_scheduler_credit_debug_trace(const fbe_u8_t * module_name,
                                              fbe_dbgext_ptr credit_base_ptr,
                                              fbe_trace_func_t trace_func,
                                              fbe_trace_context_t trace_context,
                                              fbe_debug_field_info_t *field_info_p,
                                              fbe_u32_t spaces_to_indent);

#endif /* FBE_SCHEDULER_INFO_DEBUG_H */

/*************************
 * end file fbe_scheduler_info_debug.h
 *************************/
