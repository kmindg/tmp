#ifndef FBE_TRACE_H
#define FBE_TRACE_H

#ifdef __cplusplus
/*
 * all of these functions need standard C linkage.  Otherwise, they can not be referenced from C.
 * Note, thee implementation of these functions is within a cpp file, which allows these functions to
 * access C++ instances, instance methods and class methods
 */
extern "C" {
#endif

#include "csx_ext.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_trace_interface.h"
#include "fbe/fbe_library_interface.h"
#include "fbe/fbe_class.h"
#include "fbe/fbe_payload_block_operation.h"
#include "ktrace_structs.h"

void fbe_trace_at_startup(fbe_u32_t component_type,
                          fbe_u32_t component_id,
                          fbe_trace_level_t trace_level,
                          fbe_u32_t message_id,
                          const fbe_u8_t * fmt, 
                          va_list argList);

void fbe_trace_report(fbe_u32_t component_type,
                      fbe_u32_t component_id,
                      fbe_trace_level_t trace_level,
                      fbe_u32_t message_id,
                      const fbe_u8_t * fmt, 
                      va_list argList);

void fbe_trace_report_to_traffic(fbe_u32_t component_type,
                      fbe_u32_t component_id,
                      fbe_trace_level_t trace_level,
                      fbe_u32_t message_id,
                      const fbe_u8_t * fmt, 
                      va_list argList);
void 
fbe_trace_report_w_header(fbe_u32_t component_type,
                 fbe_u32_t component_id,
                 fbe_trace_level_t trace_level,
                 fbe_u8_t * header_string,
                 const fbe_u8_t * fmt, 
                 va_list argList);

void fbe_trace_set_default_trace_level(fbe_trace_level_t default_trace_level);
//fbe_trace_level_t fbe_trace_get_default_trace_level(void);

extern fbe_trace_level_t fbe_trace_default_trace_level; /* defined in fbe_trace_main.c */

static __forceinline fbe_trace_level_t 
fbe_trace_get_default_trace_level(void)
{
    return fbe_trace_default_trace_level;
}

fbe_trace_level_t fbe_trace_get_lib_default_trace_level(fbe_library_id_t library_id);
fbe_status_t fbe_lifecycle_get_debug_trace_flag(fbe_u32_t * p_flag);

fbe_u32_t fbe_trace_get_critical_error_counter(void);
fbe_u32_t fbe_trace_get_error_counter(void);
fbe_status_t fbe_trace_init(void);
fbe_status_t fbe_trace_destroy(void);

typedef void * fbe_trace_context_t;
typedef void (* fbe_trace_func_t)(fbe_trace_context_t trace_context, const char * fmt, ...) __attribute__((format(__printf_func__,2,3)));

void fbe_trace_indent(fbe_trace_func_t trace_func,
                      fbe_trace_context_t trace_context,
                      fbe_u32_t spaces);
fbe_status_t fbe_trace_set_error_limit(fbe_trace_error_limit_t * error_limit_p);
void fbe_trace_backtrace(void);

#ifdef __cplusplus
};
#endif

#endif /* FBE_TRACE_H */
