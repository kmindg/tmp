#ifndef FBE_PROTOCOL_ERROR_INJECTION_SERVICE_DEBUG_H
#define FBE_PROTOCOL_ERROR_INJECTION_SERVICE_DEBUG_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_protocol_error_injection_service_debug.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the interfaces of the protocol error injection service
 *  debug library.
 *
 * @author
 *  10/28/2011 - Created.Omer Miranda
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_types.h"
#include "fbe/fbe_dbgext.h"
#include "fbe_trace.h"

fbe_status_t fbe_protocol_base_service_debug_trace(const fbe_u8_t * module_name,
                                                   fbe_dbgext_ptr base_ptr,
                                                   fbe_trace_func_t trace_func,
                                                   fbe_trace_context_t trace_context,
                                                   fbe_debug_field_info_t *field_info_p,
                                                   fbe_u32_t spaces_to_indent);
fbe_status_t fbe_protocol_base_debug_trace(const fbe_u8_t * module_name,
                                           fbe_dbgext_ptr base_ptr,
                                           fbe_trace_func_t trace_func,
                                           fbe_trace_context_t trace_context,
                                           fbe_debug_field_info_t *field_info_p,
                                           fbe_u32_t spaces_to_indent);
fbe_status_t fbe_protocol_error_injection_debug_display_error_element(const fbe_u8_t * module_name,
                                                                      fbe_dbgext_ptr error_element_ptr,
                                                                      fbe_trace_func_t trace_func,
                                                                      fbe_trace_context_t trace_context,
                                                                      fbe_u32_t spaces_to_indent);
fbe_status_t fbe_protocol_error_injection_error_record_trace(const fbe_u8_t * module_name,
                                                             fbe_dbgext_ptr error_record_ptr,
                                                             fbe_trace_func_t trace_func,
                                                             fbe_trace_context_t trace_context,
                                                             fbe_debug_field_info_t *field_info_p,
                                                             fbe_u32_t spaces_to_indent);
fbe_status_t fbe_protocol_error_injection_error_info_debug(const fbe_u8_t * module_name,
                                                           fbe_dbgext_ptr error_type_ptr,
                                                           fbe_trace_func_t trace_func,
                                                           fbe_trace_context_t trace_context,
                                                           fbe_debug_field_info_t *field_info_p,
                                                           fbe_u32_t spaces_to_indent);
fbe_status_t fbe_protocol_error_injection_scsi_command_debug_trace(const fbe_u8_t * module_name,
                                                                   fbe_dbgext_ptr scsi_command_ptr,
                                                                   fbe_trace_func_t trace_func,
                                                                   fbe_trace_context_t trace_context,
                                                                   fbe_debug_field_info_t *field_info_p,
                                                                   fbe_u32_t spaces_to_indent);
fbe_status_t fbe_protocol_error_injection_errors_active_info_debug(const fbe_u8_t * module_name,
                                                                   fbe_dbgext_ptr errors_active_ptr,
                                                                   fbe_trace_func_t trace_func,
                                                                   fbe_trace_context_t trace_context,
                                                                   fbe_debug_field_info_t *field_info_p,
                                                                   fbe_u32_t spaces_to_indent);
fbe_status_t fbe_protocol_error_injection_response_buffer_trace(const fbe_u8_t * module_name,
                                                                fbe_dbgext_ptr buffer_ptr,
                                                                fbe_trace_func_t trace_func,
                                                                fbe_trace_context_t trace_context,
                                                                fbe_debug_field_info_t *field_info_p,
                                                                fbe_u32_t spaces_to_indent);
fbe_status_t fbe_protocol_error_injection_port_status_trace(const fbe_u8_t * module_name,
                                                            fbe_dbgext_ptr port_status_ptr,
                                                            fbe_trace_func_t trace_func,
                                                            fbe_trace_context_t trace_context,
                                                            fbe_debug_field_info_t *field_info_p,
                                                            fbe_u32_t spaces_to_indent);
#endif /* FBE_PROTOCOL_ERROR_INJECTION_SERVICE_DEBUG_H */

/*************************
 * end file fbe_protocol_error_injection_service_debug.h
 *************************/