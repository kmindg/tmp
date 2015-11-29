#ifndef BGSL_TRACE_H
#define BGSL_TRACE_H

#include "csx_ext.h"
#include "fbe/bgsl_trace_interface.h"
/* #include "fbe/bgsl_library_interface.h" */

void bgsl_trace_at_startup(bgsl_u32_t component_type,
                          bgsl_u32_t component_id,
                          bgsl_trace_level_t trace_level,
                          bgsl_u32_t message_id,
                          const bgsl_u8_t * fmt, 
                          va_list argList);

void bgsl_trace_report(bgsl_u32_t component_type,
                      bgsl_u32_t component_id,
                      bgsl_trace_level_t trace_level,
                      bgsl_u32_t message_id,
                      const bgsl_u8_t * fmt, 
                      va_list argList);

void 
bgsl_trace_report_w_header(bgsl_u32_t component_type,
                 bgsl_u32_t component_id,
                 bgsl_trace_level_t trace_level,
                 bgsl_u8_t * header_string,
                 const bgsl_u8_t * fmt, 
                 va_list argList);

void bgsl_trace_set_default_trace_level(bgsl_trace_level_t default_trace_level);
bgsl_trace_level_t bgsl_trace_get_default_trace_level(void);
/* bgsl_trace_level_t bgsl_trace_get_lib_default_trace_level(bgsl_library_id_t library_id); */

bgsl_u32_t bgsl_trace_get_critical_error_counter(void);
bgsl_u32_t bgsl_trace_get_error_counter(void);
bgsl_status_t bgsl_trace_init(void);
bgsl_status_t bgsl_trace_destroy(void);

typedef void * bgsl_trace_context_t;
typedef void (* bgsl_trace_func_t)(bgsl_trace_context_t trace_context, const char * fmt, ...) __attribute__((format(__printf_func__,2,3)));

void bgsl_trace_indent(bgsl_trace_func_t trace_func,
                      bgsl_trace_context_t trace_context,
                      bgsl_u32_t spaces);

#endif /* BGSL_TRACE_H */
