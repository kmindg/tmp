#ifndef FBE_TERMINATOR_DEVICE_DEBUG_H
#define FBE_TERMINATOR_DEVICE_DEBUG_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2010-2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_terminator_device_debug.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the interfaces of the terminator device registry debug library.
 *
 * @author
 *  16- May- 2011 - Created. Hari Singh Chauhan
 *
 ***************************************************************************/
/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_types.h"
#include "fbe/fbe_dbgext.h"
#include "fbe_trace.h"

#define TERMINATOR_REGISTRY_INDENT			4
#define TERMINATOR_PORT_INDENT				8
#define TERMINATOR_ENCLOSURE_INDENT			16
#define TERMINATOR_DRIVE_INDENT				28


fbe_status_t fbe_terminator_registry_debug_trace(fbe_trace_func_t trace_func,
                                                 fbe_trace_context_t trace_context,
                                                 fbe_dbgext_ptr object_ptr,
                                                 fbe_s32_t port_number,
                                                 fbe_s32_t enclosure_number,
                                                 fbe_s32_t drive_number,
                                                 fbe_s32_t recursive_number);


fbe_status_t fbe_terminator_device_registry_entry_debug_trace(fbe_trace_func_t trace_func,
                                                              fbe_trace_context_t trace_context,
                                                              fbe_dbgext_ptr registry_ptr,
                                                              fbe_bool_t *registry_info_display);


fbe_status_t fbe_terminator_device_registry_device_ptr_debug_trace(fbe_trace_func_t trace_func,
                                                                   fbe_trace_context_t trace_context,
                                                                   void *device_ptr);


fbe_bool_t fbe_terminator_device_registry_component_type_port_debug_trace(fbe_trace_func_t trace_func,
                                                                          fbe_trace_context_t trace_context,
                                                                          void *device_ptr);


fbe_bool_t fbe_terminator_device_registry_component_type_enclosure_debug_trace(fbe_trace_func_t trace_func,
                                                                               fbe_trace_context_t trace_context,
                                                                               void *device_ptr);


fbe_bool_t fbe_terminator_device_registry_component_type_drive_debug_trace(fbe_trace_func_t trace_func,
                                                                           fbe_trace_context_t trace_context,
                                                                           void *device_ptr);


fbe_status_t fbe_port_type_string_debug_trace(fbe_trace_func_t trace_func,
                                              fbe_trace_context_t trace_context,
                                              fbe_u32_t port_type);


fbe_status_t fbe_port_speed_string_debug_trace(fbe_trace_func_t trace_func,
                                               fbe_trace_context_t trace_context,
                                               fbe_u32_t port_speed);


fbe_status_t fbe_terminator_sas_port_attributes_debug_trace(fbe_trace_func_t trace_func,
                                                            fbe_trace_context_t trace_context,
                                                            fbe_u64_t attributes_ptr);


fbe_status_t fbe_terminator_fc_port_attributes_debug_trace(fbe_trace_func_t trace_func,
                                                           fbe_trace_context_t trace_context,
                                                           fbe_u64_t attributes_ptr);


fbe_status_t fbe_base_component_port_attributes_debug_trace(fbe_trace_func_t trace_func,
                                                            fbe_trace_context_t trace_context,
                                                            fbe_u32_t port_type,
                                                            void *port_ptr);


fbe_status_t fbe_terminator_device_registry_port_type_debug_trace(fbe_trace_func_t trace_func,
                                                                  fbe_trace_context_t trace_context,
                                                                  void *port_ptr);


fbe_status_t fbe_enclosure_type_string_debug_trace(fbe_trace_func_t trace_func,
                                                   fbe_trace_context_t trace_context,
                                                   fbe_u32_t enclosure_type);


fbe_status_t fbe_sas_enclosure_type_string_debug_trace(fbe_trace_func_t trace_func,
                                                       fbe_trace_context_t trace_context,
                                                       fbe_u32_t enclosure_type);


fbe_status_t fbe_fc_enclosure_type_string_debug_trace(fbe_trace_func_t trace_func,
                                                      fbe_trace_context_t trace_context,
                                                      fbe_u32_t enclosure_type);


fbe_status_t fbe_base_component_enclosure_attributes_debug_trace(fbe_trace_func_t trace_func,
                                                                 fbe_trace_context_t trace_context,
                                                                 fbe_u32_t enclosure_type,
                                                                 void *enclosure_ptr);


fbe_status_t fbe_terminator_sas_enclosure_attributes_debug_trace(fbe_trace_func_t trace_func,
                                                                 fbe_trace_context_t trace_context,
                                                                 fbe_u64_t attributes_ptr);


fbe_status_t fbe_terminator_fc_enclosure_attributes_debug_trace(fbe_trace_func_t trace_func,
                                                                fbe_trace_context_t trace_context,
                                                                fbe_u64_t attributes_ptr);


fbe_status_t fbe_terminator_device_registry_enclosure_type_debug_trace(fbe_trace_func_t trace_func,
                                                                       fbe_trace_context_t trace_context,
                                                                       void *enclosure_ptr);


fbe_status_t fbe_drive_type_string_debug_trace(fbe_trace_func_t trace_func,
                                               fbe_trace_context_t trace_context,
                                               fbe_u32_t drive_type);


fbe_status_t fbe_fc_drive_type_string_debug_trace(fbe_trace_func_t trace_func,
                                                  fbe_trace_context_t trace_context,
                                                  fbe_u32_t drive_type);


fbe_status_t fbe_sas_drive_type_string_debug_trace(fbe_trace_func_t trace_func,
                                                   fbe_trace_context_t trace_context,
                                                   fbe_u32_t drive_type);


fbe_status_t fbe_sata_drive_type_string_debug_trace(fbe_trace_func_t trace_func,
                                                    fbe_trace_context_t trace_context,
                                                    fbe_u32_t drive_type);


fbe_status_t fbe_base_component_drive_attributes_debug_trace(fbe_trace_func_t trace_func,
                                                             fbe_trace_context_t trace_context,
                                                             fbe_u32_t enclosure_type,
                                                             void *enclosure_ptr);


fbe_status_t fbe_terminator_sas_drive_attributes_debug_trace(fbe_trace_func_t trace_func,
                                                             fbe_trace_context_t trace_context,
                                                             fbe_u64_t attributes_ptr);


fbe_status_t fbe_terminator_fc_drive_attributes_debug_trace(fbe_trace_func_t trace_func,
                                                            fbe_trace_context_t trace_context,
                                                            fbe_u64_t attributes_ptr);


fbe_status_t fbe_terminator_sata_drive_attributes_debug_trace(fbe_trace_func_t trace_func,
                                                              fbe_trace_context_t trace_context,
                                                              fbe_u64_t attributes_ptr);


fbe_status_t fbe_terminator_device_registry_drive_type_debug_trace(fbe_trace_func_t trace_func,
                                                                   fbe_trace_context_t trace_context,
                                                                   void *drive_ptr);


fbe_bool_t fbe_terminator_registry_recursive_debug_trace(void *device_ptr,
                                                         fbe_u32_t *spaces_to_indent);

fbe_status_t fbe_terminator_drive_debug_trace(const fbe_u8_t * module_name,
                                              fbe_dbgext_ptr terminator_drive_p,
                                              fbe_trace_func_t trace_func,
                                              fbe_trace_context_t trace_context,
                                              fbe_debug_field_info_t *field_info_p,
                                              fbe_u32_t overall_spaces_to_indent,
                                              fbe_bool_t b_display_journal);

#endif  /* FBE_TERMINATOR_DEVICE_DEBUG_H */
