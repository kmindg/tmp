#ifndef FBE_KTRACE_INTERFACE_H
#define FBE_KTRACE_INTERFACE_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2000-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 *
 * @brief
 *
 *  This file contains structures and definitions needed by the
 *  Fbe Ktrace facility
 * 
 * @author
 *
 *  01/13/2010  - Omer Miranda 
 *
 ***************************************************************************/

/*************************
 * INCLUDE FILES
 *************************/
#include "csx_ext.h"
#include <stdio.h>          /*!< This is required since we cannot include fbe_winddk.h since it include this file. */

#if defined(__cplusplus)
extern "C"
{
#endif

#include "fbe/fbe_trace_interface.h"    /*!< Required for fbe_trace_ing_t definition */

#include "ktrace.h"                         /*!< This module invokes ktrace.lib methods */
#include "ktrace_IOCTL.h"         
#include "ktrace_structs.h"
/*************************
 * LITERAL DEFINITIONS
 *************************/

/*                                                               
 *  Prototypes from fbe_ktrace_main.c
 */
fbe_bool_t  fbe_ktrace_is_initialized(void);
fbe_status_t __cdecl fbe_ktrace_initialize(void);
fbe_status_t __cdecl fbe_ktrace_destroy(void);
fbe_status_t fbe_ktrace_flush(void);
void fbe_KvTraceStart(const fbe_char_t *fmt, ...) __attribute__((format(__printf_func__,1,2)));
void fbe_KvTrace(const fbe_char_t *fmt, ...) __attribute__((format(__printf_func__,1,2)));
void fbe_KvTraceRing(fbe_trace_ring_t fbe_buf, const fbe_char_t *fmt, ...) __attribute__((format(__printf_func__,2,3)));
void fbe_KvTraceRingArgList(fbe_trace_ring_t fbe_buf, const fbe_char_t *fmt,  va_list argptr);
long fbe_KtraceEntriesRemaining(const fbe_trace_ring_t buf);
fbe_status_t fbe_KvTraceLocation(const fbe_trace_ring_t buf,
                                 const fbe_char_t *function, unsigned long line,
                                 fbe_u32_t function_string_size, fbe_u32_t line_string_size, 
                                 const fbe_char_t *fmt, va_list argptr);
typedef void (*fbe_ktrace_func_t)(const char *format, ...);
fbe_status_t fbe_ktrace_set_trace_function(fbe_ktrace_func_t func);

/* Determine if rba tracing is enabled. 
 * Inlined for performance. 
 */
static __forceinline fbe_bool_t fbe_ktrace_is_rba_logging_enabled(fbe_u64_t component_flag)
{
    extern PVOID RBA_logging_enabled_info;

    return (RBA_logging_enabled_info != NULL &&
            ((TRACE_INFO *) RBA_logging_enabled_info)->ti_tflag & component_flag);
}
#if defined(__cplusplus)
}
#endif

/****************************************
 * END fbe_ktrace_interface.h
 ****************************************/
#endif /*  FBE_KTRACE_INTERFACE_H */
