#ifndef FBE_BASE_CONFIG_DEBUG_H
#define FBE_BASE_CONFIG_DEBUG_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_base_config_debug.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the interfaces of the base config debug library.
 *
 * @author
 *  9/8/2010 - Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_types.h"
#include "fbe/fbe_dbgext.h"
#include "fbe_trace.h"

typedef enum fbe_metadata_nonpaged_state_e{
    FBE_METADATA_NONPAGED_STATE_INVALID,
    FBE_METADATA_NONPAGED_STATE_ALLOCATED,
    FBE_METADATA_NONPAGED_STATE_FREE,
    FBE_METADATA_NONPAGED_STATE_VAULT,
}fbe_metadata_nonpaged_state_t;

typedef struct fbe_metadata_nonpaged_entry_s{
    fbe_u32_t data_size;
    fbe_metadata_nonpaged_state_t state;
    fbe_u8_t data[FBE_METADATA_NONPAGED_MAX_SIZE];
}fbe_metadata_nonpaged_entry_t;

fbe_status_t fbe_base_config_debug_trace(const fbe_u8_t * module_name,
                                         fbe_dbgext_ptr base_p,
                                         fbe_trace_func_t trace_func,
                                         fbe_trace_context_t trace_context,
                                         fbe_debug_field_info_t *field_info_p,
                                         fbe_u32_t overall_spaces_to_indent);
fbe_status_t fbe_base_config_flag_debug_trace(const fbe_u8_t * module_name,
                                              fbe_dbgext_ptr base_ptr,
                                              fbe_trace_func_t trace_func,
                                              fbe_trace_context_t trace_context,
                                              fbe_debug_field_info_t *field_info_p,
                                              fbe_u32_t spaces_to_indent);

fbe_status_t fbe_base_config_flag_string_trace(fbe_base_config_flags_t flags, 
                                               fbe_trace_func_t trace_func, 
                                               fbe_trace_context_t trace_context);

fbe_status_t fbe_base_config_signature_debug_trace(const fbe_u8_t * module_name,
                                                   fbe_dbgext_ptr base_p,
                                                   fbe_trace_func_t trace_func,
                                                   fbe_trace_context_t trace_context,
                                                   fbe_debug_field_info_t *field_info_p,
                                                   fbe_u32_t overall_spaces_to_indent);
fbe_status_t fbe_base_config_nonpaged_metadata_debug_trace(const fbe_u8_t * module_name,
                                                           fbe_dbgext_ptr base_ptr,
                                                           fbe_trace_func_t trace_func,
                                                           fbe_trace_context_t trace_context,
                                                           fbe_debug_field_info_t *field_info_p,
                                                           fbe_u32_t spaces_to_indent);
fbe_status_t fbe_base_config_metadata_memory_debug_trace(const fbe_u8_t * module_name,
                                                         fbe_dbgext_ptr record_ptr,
                                                         fbe_trace_func_t trace_func,
                                                         fbe_trace_context_t trace_context,
                                                         fbe_debug_field_info_t *field_info_p,
                                                         fbe_u32_t spaces_to_indent);
fbe_status_t fbe_non_paged_record_debug_trace(const fbe_u8_t * module_name,
                                            fbe_dbgext_ptr object_ptr,
                                            fbe_trace_func_t trace_func,
                                            fbe_trace_context_t trace_context,
                                            fbe_u32_t spaces_to_indent);

fbe_status_t fbe_non_paged_metadata_set_class_id(fbe_class_id_t cid);

fbe_status_t fbe_non_paged_record_dump_debug_trace(const fbe_u8_t * module_name,
                                            fbe_dbgext_ptr object_ptr,
                                            fbe_trace_func_t trace_func,
                                            fbe_trace_context_t trace_context,
                                            fbe_u32_t spaces_to_indent);

fbe_status_t fbe_non_paged_record_metadata_dump_debug_trace(const fbe_u8_t * module_name,
                                             fbe_dbgext_ptr object_ptr,
                                             fbe_trace_func_t trace_func,
                                             fbe_trace_context_t trace_context,
                                             fbe_debug_field_info_t *field_info_p,
                                             fbe_u32_t spaces_to_indent);

fbe_status_t fbe_base_config_dump_debug_trace(const fbe_u8_t * module_name,
                                         fbe_dbgext_ptr base_p,
                                         fbe_trace_func_t trace_func,
                                         fbe_trace_context_t trace_context,
                                         fbe_debug_field_info_t *field_info_p,
                                         fbe_u32_t overall_spaces_to_indent);

fbe_status_t fbe_base_config_metadata_memory_dump_debug_trace(const fbe_u8_t * module_name,
                                                         fbe_dbgext_ptr record_ptr,
                                                         fbe_trace_func_t trace_func,
                                                         fbe_trace_context_t trace_context,
                                                         fbe_debug_field_info_t *field_info_p,
                                                         fbe_u32_t spaces_to_indent);

fbe_status_t fbe_metadata_memory_dump_debug_trace(const fbe_u8_t * module_name,
                                             fbe_dbgext_ptr object_ptr,
                                             fbe_trace_func_t trace_func,
                                             fbe_trace_context_t trace_context,
                                             fbe_debug_field_info_t *field_info_p,
                                             fbe_u32_t spaces_to_indent);

fbe_status_t fbe_base_config_nonpaged_metadata_dump_debug_trace(const fbe_u8_t * module_name,
                                                           fbe_dbgext_ptr base_ptr,
                                                           fbe_trace_func_t trace_func,
                                                           fbe_trace_context_t trace_context,
                                                           fbe_debug_field_info_t *field_info_p,
                                                           fbe_u32_t spaces_to_indent);

#endif /* FBE_BASE_CONFIG_DEBUG_H */

/*************************
 * end file fbe_base_config_debug.h
 *************************/
