#ifndef FBE_BASE_OBJECT_DEBUG_H
#define FBE_BASE_OBJECT_DEBUG_H

#include "fbe/fbe_types.h"
#include "fbe/fbe_dbgext.h"
#include "fbe_trace.h"

fbe_status_t
fbe_base_object_debug_trace_object_id(const fbe_u8_t * module_name,
                                      fbe_dbgext_ptr base_object,
                                      fbe_trace_func_t trace_func,
                                      fbe_trace_context_t trace_context);

fbe_status_t
fbe_base_object_debug_trace_class_id(const fbe_u8_t * module_name, 
                                     fbe_dbgext_ptr base_object, 
                                     fbe_trace_func_t trace_func, 
                                     fbe_trace_context_t trace_context);

fbe_status_t __cdecl
fbe_base_object_debug_trace_state(const fbe_u8_t * module_name,
                                  fbe_dbgext_ptr base_object,
                                  fbe_trace_func_t trace_func,
                                  fbe_trace_context_t trace_context);

fbe_status_t
fbe_base_object_debug_trace_level (const fbe_u8_t * module_name,
                                   fbe_dbgext_ptr base_object,
                                   fbe_trace_func_t trace_func,
                                   fbe_trace_context_t trace_context);

fbe_status_t fbe_base_object_debug_trace(const fbe_u8_t * module_name,
                                         fbe_dbgext_ptr object_ptr,
                                         fbe_trace_func_t trace_func,
                                         fbe_trace_context_t trace_context,
                                         fbe_debug_field_info_t *field_info_p,
                                         fbe_u32_t spaces_to_indent);
fbe_status_t fbe_class_id_debug_trace(const fbe_u8_t * module_name,
                                      fbe_dbgext_ptr base_ptr,
                                      fbe_trace_func_t trace_func,
                                      fbe_trace_context_t trace_context,
                                      fbe_debug_field_info_t *field_info_p,
                                      fbe_u32_t spaces_to_indent);
fbe_status_t fbe_lifecycle_state_debug_trace(const fbe_u8_t * module_name,
                                             fbe_dbgext_ptr base_ptr,
                                             fbe_trace_func_t trace_func,
                                             fbe_trace_context_t trace_context,
                                             fbe_debug_field_info_t *field_info_p,
                                             fbe_u32_t spaces_to_indent);
 fbe_status_t fbe_trace_level_debug_trace(const fbe_u8_t * module_name,
                                         fbe_dbgext_ptr base_ptr,
                                         fbe_trace_func_t trace_func,
                                         fbe_trace_context_t trace_context,
                                         fbe_debug_field_info_t *field_info_p,
                                         fbe_u32_t spaces_to_indent);

 fbe_status_t fbe_base_object_dump_debug_trace(const fbe_u8_t * module_name,
                                         fbe_dbgext_ptr object_ptr,
                                         fbe_trace_func_t trace_func,
                                         fbe_trace_context_t trace_context,
                                         fbe_debug_field_info_t *field_info_p,
                                         fbe_u32_t spaces_to_indent);

 fbe_status_t fbe_lifecycle_state_dump_debug_trace(const fbe_u8_t * module_name,
                                             fbe_dbgext_ptr base_ptr,
                                             fbe_trace_func_t trace_func,
                                             fbe_trace_context_t trace_context,
                                             fbe_debug_field_info_t *field_info_p,
                                             fbe_u32_t spaces_to_indent);
 fbe_status_t fbe_class_id_dump_debug_trace(const fbe_u8_t * module_name,
                                      fbe_dbgext_ptr base_ptr,
                                      fbe_trace_func_t trace_func,
                                      fbe_trace_context_t trace_context,
                                      fbe_debug_field_info_t *field_info_p,
                                      fbe_u32_t spaces_to_indent);


#endif /* FBE_BASE_OBJECT_DEBUG_H */
