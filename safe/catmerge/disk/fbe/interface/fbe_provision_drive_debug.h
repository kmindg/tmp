#ifndef FBE_PROVISION_DRIVE_DEBUG_H
#define FBE_PROVISION_DRIVE_DEBUG_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_provision_drive_debug.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the interfaces of the provision drive debug library.
 *
 * @author
 *  04/09/2010 - Created. Prafull Parab
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_types.h"
#include "fbe/fbe_dbgext.h"
#include "fbe_trace.h"

/*! @struct fbe_pvd_database_info_t
 *  
 * @brief PVD object information structure. This structure holds information related to PVD object.
 */
typedef struct fbe_pvd_database_info_s
{
    fbe_provision_drive_config_type_t config_type;
    fbe_lba_t configured_capacity;
    fbe_provision_drive_configured_physical_block_size_t configured_physical_block_size;
}fbe_pvd_database_info_t;

fbe_status_t fbe_provision_drive_debug_trace(const fbe_u8_t * module_name,
                                             fbe_dbgext_ptr provision_drive_p,
                                             fbe_trace_func_t trace_func,
                                             fbe_trace_context_t trace_context,
                                             fbe_u32_t spaces_to_indent);


fbe_status_t fbe_provision_drive_debug_display_terminator_queue(const fbe_u8_t * module_name,
                                                                fbe_dbgext_ptr provision_drive_p,
                                                                fbe_trace_func_t trace_func,
                                                                fbe_trace_context_t trace_context,
                                                                fbe_u32_t spaces_to_indent);
fbe_status_t fbe_provision_drive_non_paged_record_debug_trace(const fbe_u8_t * module_name,
                                                              fbe_dbgext_ptr record_ptr,
                                                              fbe_trace_func_t trace_func,
                                                              fbe_trace_context_t trace_context,
                                                              fbe_u32_t spaces_to_indent);
fbe_status_t fbe_provision_drive_metadata_memory_debug_trace(const fbe_u8_t * module_name,
                                                             fbe_dbgext_ptr record_ptr,
                                                             fbe_trace_func_t trace_func,
                                                             fbe_trace_context_t trace_context,
                                                             fbe_u32_t spaces_to_indent);
fbe_status_t fbe_metadata_operation_pvd_paged_metadata_debug_trace(const fbe_u8_t * module_name,
                                                                   fbe_dbgext_ptr paged_metadata_ptr,
                                                                   fbe_trace_func_t trace_func,
                                                                   fbe_trace_context_t trace_context,
                                                                   fbe_u32_t spaces_to_indent);
fbe_status_t fbe_provision_drive_nonpaged_drive_info_debug_trace(const fbe_u8_t * module_name,
                                                                 fbe_dbgext_ptr drive_info_ptr,
                                                                 fbe_trace_func_t trace_func,
                                                                 fbe_trace_context_t trace_context,
                                                                 fbe_debug_field_info_t *field_info_p,
                                                                 fbe_u32_t spaces_to_indent);
fbe_status_t fbe_provision_drive_type_debug_trace(const fbe_u8_t * module_name,
                                                  fbe_dbgext_ptr drive_type_ptr,
                                                  fbe_trace_func_t trace_func,
                                                  fbe_trace_context_t trace_context,
                                                  fbe_debug_field_info_t *field_info_p,
                                                  fbe_u32_t spaces_to_indent);

fbe_status_t fbe_sepls_pvd_info_trace(const fbe_u8_t * module_name,
                                      fbe_object_id_t pvd_id,
                                      fbe_block_size_t exported_block_size,
                                      fbe_bool_t display_default,
                                      fbe_u32_t display_format,
                                      fbe_trace_func_t trace_func,
                                      fbe_trace_context_t trace_context);

fbe_status_t fbe_sepls_get_pvd_persist_data(const fbe_u8_t* module_name,
                                            fbe_dbgext_ptr pvd_object_ptr,
                                            fbe_dbgext_ptr pvd_user_ptr,
                                            fbe_pvd_database_info_t *pvd_data_ptr,
                                            fbe_trace_func_t trace_func,
                                            fbe_trace_context_t trace_context);

fbe_status_t fbe_provision_drive_dump_debug_trace(const fbe_u8_t * module_name,
                                             fbe_dbgext_ptr provision_drive_p,
                                             fbe_trace_func_t trace_func,
                                             fbe_trace_context_t trace_context,
                                             fbe_u32_t overall_spaces_to_indent);

fbe_status_t fbe_provision_drive_non_paged_record_dump_debug_trace(const fbe_u8_t * module_name,
                                                              fbe_dbgext_ptr record_ptr,
                                                              fbe_trace_func_t trace_func,
                                                              fbe_trace_context_t trace_context,
                                                              fbe_u32_t spaces_to_indent);

fbe_status_t fbe_provision_drive_metadata_memory_dump_debug_trace(const fbe_u8_t * module_name,
                                                             fbe_dbgext_ptr record_ptr,
                                                             fbe_trace_func_t trace_func,
                                                             fbe_trace_context_t trace_context,
                                                             fbe_u32_t spaces_to_indent);

fbe_status_t fbe_provision_drive_metadata_memory_dump_debug_trace(const fbe_u8_t * module_name,
                                                             fbe_dbgext_ptr record_ptr,
                                                             fbe_trace_func_t trace_func,
                                                             fbe_trace_context_t trace_context,
                                                             fbe_u32_t spaces_to_indent);

fbe_status_t fbe_provision_drive_type_dump_debug_trace(const fbe_u8_t * module_name,
                                                  fbe_dbgext_ptr drive_type_ptr,
                                                  fbe_trace_func_t trace_func,
                                                  fbe_trace_context_t trace_context,
                                                  fbe_debug_field_info_t *field_info_p,
                                                  fbe_u32_t spaces_to_indent);

fbe_status_t fbe_provision_drive_nonpaged_drive_info_dump_debug_trace(const fbe_u8_t * module_name,
                                                                 fbe_dbgext_ptr drive_info_ptr,
                                                                 fbe_trace_func_t trace_func,
                                                                 fbe_trace_context_t trace_context,
                                                                 fbe_debug_field_info_t *field_info_p,
                                                                 fbe_u32_t spaces_to_indent);

#endif /* FBE_PROVISION_DRIVE_DEBUG_H */

/*************************
 * end file fbe_provision_drive_debug.h
 *************************/
