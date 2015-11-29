#ifndef FBE_FC_PORT_DEBUG_H
#define FBE_FC_PORT_DEBUG_H

#include "fbe/fbe_types.h"
#include "fbe/fbe_dbgext.h"
#include "fbe_trace.h"

fbe_status_t fbe_fc_port_debug_trace(const fbe_u8_t * module_name, 
                                     fbe_dbgext_ptr port_object_p, 
                                     fbe_trace_func_t trace_func, 
                                     fbe_trace_context_t trace_context);

#endif /* FBE_FC_PORT_DEBUG_H */
