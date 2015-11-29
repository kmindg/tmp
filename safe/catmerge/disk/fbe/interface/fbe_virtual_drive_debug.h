#ifndef FBE_VIRTUL_DRIVE_DEBUG_H
#define FBE_VIRTUL_DRIVE_DEBUG_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_virtual_drive_debug.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the interfaces of the virtual drive debug library.
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

/*! @struct fbe_vd_database_info_t
 *  
 * @brief VD object information structure. This structure holds information related to VD object.
 */
typedef struct fbe_vd_database_info_s
{
    fbe_lba_t capacity;
    fbe_lba_t imported_capacity;
}fbe_vd_database_info_t;

fbe_status_t fbe_virtual_drive_debug_trace(const fbe_u8_t * module_name,
                                           fbe_dbgext_ptr virtual_drive_p,
                                           fbe_trace_func_t trace_func,
                                           fbe_trace_context_t trace_context,
                                           fbe_u32_t spaces_to_indent);

fbe_status_t fbe_virtual_drive_debug_display_terminator_queue(const fbe_u8_t * module_name,
                                                              fbe_dbgext_ptr virtual_drive_p,
                                                              fbe_trace_func_t trace_func,
                                                              fbe_trace_context_t trace_context,
                                                              fbe_u32_t spaces_to_indent);
fbe_status_t fbe_sepls_vd_info_trace(const fbe_u8_t * module_name,
                                     fbe_object_id_t vd_id,
                                     fbe_block_size_t exported_block_size,
                                     fbe_u32_t display_format,
                                     fbe_trace_func_t trace_func,
                                     fbe_trace_context_t trace_context);
fbe_status_t fbe_sepls_get_vd_persist_data(const fbe_u8_t* module_name,
                                           fbe_dbgext_ptr vd_object_ptr,
                                           fbe_dbgext_ptr vd_user_ptr,
                                           fbe_vd_database_info_t *vd_data_ptr,
                                           fbe_trace_func_t trace_func,
                                           fbe_trace_context_t trace_context);
fbe_status_t fbe_sepls_virtual_drive_get_metadata_capacity(fbe_lba_t imported_capacity,
                                                   fbe_lba_t           *paged_metadata_capacity);

fbe_status_t fbe_virtual_drive_dump_debug_trace(const fbe_u8_t * module_name,
                                           fbe_dbgext_ptr virtual_drive_p,
                                           fbe_trace_func_t trace_func,
                                           fbe_trace_context_t trace_context,
                                           fbe_u32_t overall_spaces_to_indent);


fbe_status_t fbe_virtual_drive_spare_info_dump_debug_trace(const fbe_u8_t * module_name,
                                                                 fbe_dbgext_ptr drive_info_ptr,
                                                                 fbe_trace_func_t trace_func,
                                                                 fbe_trace_context_t trace_context,
                                                                 fbe_debug_field_info_t *field_info_p,
                                                                 fbe_u32_t spaces_to_indent);
fbe_status_t fbe_virtual_drive_mirror_optimization_info_dump_debug_trace(const fbe_u8_t * module_name,
                                                                        fbe_dbgext_ptr base_ptr,
                                                                        fbe_trace_func_t trace_func,
                                                                        fbe_trace_context_t trace_context,
                                                                        fbe_debug_field_info_t *field_info_p,
                                                                        fbe_u32_t spaces_to_indent);

#endif /* FBE_VIRTUL_DRIVE_DEBUG_H */

/*************************
 * end file fbe_virtual_drive_debug.h
 *************************/
