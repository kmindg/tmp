#ifndef PP_DBGEXT_H
#define PP_DBGEXT_H

#include "fbe/fbe_dbgext.h"
#include "csx_ext.h"
#include "fbe_trace.h"

extern char * pp_ext_module_name;
csx_s64_t GetArgument64(const char * String, ULONG Count);
void fbe_debug_trace_func(fbe_trace_context_t trace_context, const char * fmt, ...) __attribute__((format(__printf_func__,2,3)));

#endif /* PP_DBGEXT_H */
