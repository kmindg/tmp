#ifndef FBE_RAID_LIBRARY_DEBUG_H
#define FBE_RAID_LIBRARY_DEBUG_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_raid_library_debug.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the interfaces of the raid library debug library.
 *
 * @author
 *  11/24/2009 - Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_types.h"
#include "fbe/fbe_dbgext.h"
#include "fbe_trace.h"

fbe_status_t fbe_raid_library_debug_print_iots(const fbe_u8_t * module_name,
                                             fbe_dbgext_ptr iots_p,
                                             fbe_trace_func_t trace_func,
                                             fbe_trace_context_t trace_context,
                                             fbe_u32_t spaces_to_indent);
fbe_status_t fbe_raid_library_debug_print_iots_summary(const fbe_u8_t * module_name,
                                                       fbe_dbgext_ptr iots_p,
                                                       fbe_trace_func_t trace_func,
                                                       fbe_trace_context_t trace_context,
                                                       fbe_u32_t overall_spaces_to_indent);
fbe_status_t fbe_raid_geometry_debug_trace(const fbe_u8_t * module_name,
                                           fbe_dbgext_ptr base_p,
                                           fbe_trace_func_t trace_func,
                                           fbe_trace_context_t trace_context,
                                           fbe_debug_field_info_t *field_info_p,
                                           fbe_u32_t overall_spaces_to_indent);
fbe_status_t fbe_raid_library_debug_print_siots(const fbe_u8_t * module_name,
                                                fbe_dbgext_ptr siots_p,
                                                fbe_trace_func_t trace_func,
                                                fbe_trace_context_t trace_context,
                                                fbe_u32_t spaces_to_indent);
fbe_status_t fbe_raid_library_debug_print_sg_list(const fbe_u8_t * module_name,
                                                  fbe_dbgext_ptr sg_p,
                                                  fbe_trace_func_t trace_func,
                                                  fbe_trace_context_t trace_context,
                                                  fbe_u32_t spaces_to_indent,
                                                  fbe_u32_t num_sector_words_to_display,
                                                  fbe_lba_t lba_base);
fbe_status_t fbe_raid_library_raid_type_debug_trace(const fbe_u8_t * module_name,
                                           fbe_dbgext_ptr base_p,
                                           fbe_trace_func_t trace_func,
                                           fbe_trace_context_t trace_context,
                                           fbe_debug_field_info_t *field_info_p,
                                           fbe_u32_t overall_spaces_to_indent);

fbe_status_t fbe_raid_library_geometry_flags_debug_trace(const fbe_u8_t * module_name,
                                           fbe_dbgext_ptr base_p,
                                           fbe_trace_func_t trace_func,
                                           fbe_trace_context_t trace_context,
                                           fbe_debug_field_info_t *field_info_p,
                                           fbe_u32_t overall_spaces_to_indent);

fbe_status_t fbe_raid_library_attribute_flags_debug_trace(const fbe_u8_t * module_name,
                                           fbe_dbgext_ptr base_p,
                                           fbe_trace_func_t trace_func,
                                           fbe_trace_context_t trace_context,
                                           fbe_debug_field_info_t *field_info_p,
                                           fbe_u32_t overall_spaces_to_indent);

fbe_status_t fbe_raid_library_debug_flags_debug_trace(const fbe_u8_t * module_name,
                                           fbe_dbgext_ptr base_p,
                                           fbe_trace_func_t trace_func,
                                           fbe_trace_context_t trace_context,
                                           fbe_debug_field_info_t *field_info_p,
                                           fbe_u32_t overall_spaces_to_indent);

fbe_status_t fbe_raid_library_debug_print_full_raid_group_geometry(const fbe_u8_t * module_name,
                                                        fbe_dbgext_ptr raid_group_p,
                                                        fbe_trace_func_t trace_func,
                                                        fbe_trace_context_t trace_context,
                                                        fbe_u32_t spaces_to_indent);
#endif /* FBE_RAID_LIBRARY_DEBUG_H */

/*************************
 * end file fbe_raid_library_debug.h
 *************************/