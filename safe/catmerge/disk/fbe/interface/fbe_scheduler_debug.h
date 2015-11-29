#ifndef FBE_SCHEDULER_DEBUG_H
#define FBE_SCHEDULER_DEBUG_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_scheduler_debug.h
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

/*!********************************************************************* 
 * @enum fbe_scheduler_info_display_e
 *  
 * @brief 
 * This enumeration lists the scheduler info display.
 *
 **********************************************************************/
typedef enum fbe_scheduler_info_display_e
{
    FBE_SCHEDULER_INFO_DEFAULT_DISPLAY = 0x0000,
    FBE_SCHEDULER_INFO_VERBOSE_DISPLAY = 0x0001,
    FBE_SCHEDULER_INFO_QUEUE_ONLY_DISPLAY = 0x0002,
    FBE_SCHEDULER_INFO_NO_QUEUE_DISPLAY = 0x0004,
    FBE_SCHEDULER_INFO_THREAD_INFO_DISPLAY = 0x0008,
    FBE_SCHEDULER_INFO_CORE_CREDIT_TABLE_DISPLAY = 0x0010,
    FBE_SCHEDULER_INFO_DEBUG_HOOKS_INFO_DISPLAY = 0x0020,
} fbe_scheduler_info_display_t;

fbe_status_t fbe_scheduler_queue_debug(const fbe_u8_t * module_name,
                                       fbe_dbgext_ptr head_ptr,
                                       fbe_u32_t display_format,
                                       fbe_trace_func_t trace_func,
                                       fbe_trace_context_t trace_context);
fbe_status_t fbe_scheduler_thread_info_debug(const fbe_u8_t * module_name,
                                             fbe_dbgext_ptr thread_info_pool_ptr,
                                             fbe_trace_func_t trace_func,
                                             fbe_trace_context_t trace_context);
fbe_status_t fbe_scheduler_debug_hooks_info_debug(const fbe_u8_t * module_name,
                                                  fbe_dbgext_ptr debug_hooks_info_ptr,
                                                  fbe_trace_func_t trace_func,
                                                  fbe_trace_context_t trace_context);
fbe_status_t fbe_scheduler_core_credit_table_debug(const fbe_u8_t * module_name,
                                                   fbe_dbgext_ptr core_credit_table_ptr,
                                                   fbe_trace_func_t trace_func,
                                                   fbe_trace_context_t trace_context);


#endif /* FBE_SCHEDULER_DEBUG_H */

/*************************
 * end file fbe_scheduler_debug.h
 *************************/
