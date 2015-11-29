#ifndef FBE_METADATA_DEBUG_H
#define FBE_METADATA_DEBUG_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2010-2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_metadata_debug.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the interfaces for the metadata operation debug library.
 *
 * @author
 *  02/15/2011 - Created. Omer Miranda
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_types.h"
#include "fbe/fbe_dbgext.h"
#include "fbe_trace.h"
#include "fbe_metadata.h"

fbe_status_t fbe_metadata_operation_metadata_debug_trace(const fbe_u8_t * module_name,
                                                         fbe_u64_t metadata_operation_p,
                                                         fbe_trace_func_t trace_func,
                                                         fbe_trace_context_t trace_context,
                                                         fbe_u32_t spaces_to_indent);
fbe_status_t fbe_metadata_operation_write_log_debug_trace(const fbe_u8_t * module_name,
                                                          fbe_u64_t metadata_write_log_p,
                                                          fbe_trace_func_t trace_func,
                                                          fbe_trace_context_t trace_context,
                                                          fbe_u32_t spaces_to_indent);
fbe_status_t fbe_base_config_nonpaged_metadata_debug_trace(const fbe_u8_t * module_name,
                                                           fbe_dbgext_ptr base_ptr,
                                                           fbe_trace_func_t trace_func,
                                                           fbe_trace_context_t trace_context,
                                                           fbe_debug_field_info_t *field_info_p,
                                                           fbe_u32_t spaces_to_indent);
fbe_status_t fbe_metadata_paged_blob_queue_debug_trace(const fbe_u8_t * module_name,
                                                       fbe_dbgext_ptr object_ptr,
                                                       fbe_trace_func_t trace_func,
                                                       fbe_trace_context_t trace_context,
                                                       fbe_debug_field_info_t *field_info_p,
                                                       fbe_u32_t spaces_to_indent);
fbe_status_t fbe_metadata_paged_blob_debug_trace(const fbe_u8_t * module_name,
                                                 fbe_dbgext_ptr object_ptr,
                                                 fbe_trace_func_t trace_func,
                                                 fbe_trace_context_t trace_context,
                                                 fbe_u32_t spaces_to_indent);
fbe_status_t fbe_metadata_paged_blob_sg_element_debug_trace(const fbe_u8_t * module_name,
                                                            fbe_dbgext_ptr object_ptr,
                                                            fbe_trace_func_t trace_func,
                                                            fbe_trace_context_t trace_context,
                                                            fbe_debug_field_info_t *field_info_p,
                                                            fbe_u32_t spaces_to_indent);

#endif /* FBE_METADATA_DEBUG_H */

/*************************
 * end file fbe_metadata_debug.h
 *************************/
