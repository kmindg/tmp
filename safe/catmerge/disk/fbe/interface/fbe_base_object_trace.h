#ifndef FBE_BASE_OBJECT_TRACE_H
#define FBE_BASE_OBJECT_TRACE_H

#include "fbe/fbe_types.h"
#include "fbe_trace.h"

fbe_status_t
fbe_base_object_trace_object_id(fbe_object_id_t object_id,
                                fbe_trace_func_t trace_func,
                                fbe_trace_context_t trace_context);

fbe_status_t
fbe_base_object_trace_class_id(fbe_class_id_t class_id, 
                               fbe_trace_func_t trace_func, 
                               fbe_trace_context_t trace_context);

fbe_status_t
fbe_base_object_trace_state(fbe_lifecycle_state_t lifecycle_state,
                            fbe_trace_func_t trace_func,
                            fbe_trace_context_t trace_context);

fbe_status_t
fbe_base_object_trace_level(fbe_trace_level_t trace_level,
                            fbe_trace_func_t trace_func,
                            fbe_trace_context_t trace_context);

char * fbe_base_object_trace_get_state_string(fbe_lifecycle_state_t lifecycle_state);
char * fbe_base_object_trace_get_level_string(fbe_trace_level_t trace_level);

#endif /* FBE_BASE_OBJECT_TRACE_H */
