
/***************************************************************************
 * Copyright (C) EMC Corporation 2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_virtual_drive_dump_debug.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the debug functions for the virtual drive object.
 *
 * @author
 *  8/15/2012 - Created. Jingcheng Zhang
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_dbgext.h"
#include "fbe/fbe_topology_interface.h"
#include "fbe_base_object.h"
#include "fbe_base_object_debug.h"
#include "fbe/fbe_virtual_drive.h"
#include "fbe_virtual_drive_private.h"
#include "fbe_base_object_trace.h"
#include "fbe_virtual_drive_debug.h"
#include "fbe_transport_debug.h"
#include "fbe_raid_library_debug.h"
#include "fbe_base_config_debug.h"
#include "fbe_topology_debug.h"
#include "fbe_raid_group_debug.h"


/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/*!*******************************************************************
 * @var fbe_virtual_drive_dump_field_info
 *********************************************************************
 * @brief Information about each of the fields of virtual drive.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_virtual_drive_dump_field_info[] =
{
    /* base config is first.
     */
    FBE_DEBUG_DECLARE_FIELD_INFO("flags", fbe_virtual_drive_flags_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("configuration_mode", fbe_virtual_drive_configuration_mode_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("new_configuration_mode", fbe_virtual_drive_configuration_mode_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("swap_in_edge_index", fbe_edge_index_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("swap_out_edge_index", fbe_edge_index_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("need_replacement_drive_start_time", fbe_u64_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("copy_request_type", fbe_spare_swap_command_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("proactive_copy_health", fbe_spare_proactive_health_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("orig_pvd_obj_id", fbe_object_id_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("unused_drive_as_spare", fbe_virtual_drive_unused_drive_as_spare_flag_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("debug_flags", fbe_virtual_drive_debug_flags_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("spare", fbe_mirror_t, FBE_TRUE, "0x%x",
                                    fbe_virtual_drive_spare_info_dump_debug_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_virtual_drive_dump_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        virtual drive.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_virtual_drive_t,
                              fbe_virtual_drive_dump_struct_info,
                              &fbe_virtual_drive_dump_field_info[0]);


/*!**************************************************************
 * @fn fbe_virtual_drive_dump_debug_trace(const fbe_u8_t * module_name,
 *                                   fbe_dbgext_ptr virtual_drive,
 *                                   fbe_trace_func_t trace_func,
 *                                   fbe_trace_context_t trace_context)
 ****************************************************************
 * @brief
 *  Display all the trace information about the virtual drive object.
 *
 * @param  module_name - This is the name of the module we are debugging.
 * @param virtual_drive_p - Ptr to virtual drive object.
 * @param trace_func - Function to use when tracing.
 * @param trace_context - The context to be passed to the trace function.
 *
 * @return - FBE_STATUS_OK  
 *
 * HISTORY:
 *  8/15/2012 - Created. Jingcheng Zhang
 *
 ****************************************************************/

fbe_status_t fbe_virtual_drive_dump_debug_trace(const fbe_u8_t * module_name,
                                           fbe_dbgext_ptr virtual_drive_p,
                                           fbe_trace_func_t trace_func,
                                           fbe_trace_context_t trace_context,
                                           fbe_u32_t overall_spaces_to_indent)
{
    fbe_status_t status;

    status = fbe_debug_display_structure(module_name, trace_func, trace_context, virtual_drive_p,
                                         &fbe_virtual_drive_dump_struct_info,
                                         1 /* fields per line */,
                                         0);
    trace_func(NULL, "\n");

    return status;
} 
/**************************************
 * end fbe_virtual_drive_dump_debug_trace()
 **************************************/
 
/*!*******************************************************************
 * @var fbe_virtual_drive_spare_dump_field_info
 *********************************************************************
 * @brief Information about each of the fields of provision drive
 *        non-paged drive info.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_virtual_drive_spare_dump_field_info[] =
{
    FBE_DEBUG_DECLARE_FIELD_INFO("flags", fbe_mirror_enum_t, FBE_FALSE, "0x%x"),
	FBE_DEBUG_DECLARE_FIELD_INFO_FN("mirror_opt_db", fbe_mirror_optimization_t, FBE_FALSE, "0x%x",
									fbe_virtual_drive_mirror_optimization_info_dump_debug_trace),
	FBE_DEBUG_DECLARE_FIELD_INFO_FN("raid_group", fbe_raid_group_t, FBE_TRUE, "0x%x",
									fbe_raid_group_dump_debug_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};
/*!*******************************************************************
 * @var fbe_virtual_drive_spare_dump_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *       fbe_mirror_t.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_mirror_t,
                                                  fbe_virtual_drive_spare_dump_struct_info,
                                                  &fbe_virtual_drive_spare_dump_field_info[0]);

/*!**************************************************************
 * fbe_virtual_drive_spare_info_dump_debug_trace()
 ****************************************************************
 * @brief
 *  Display the vitual drive spare info.
 *
 * @param module_name - Module name to use in symbol lookup.
 * @param drive_info_ptr - ptr to value to display.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param spaces_to_indent - Number of spaces to indent the next line(s).
 * 
 * @return fbe_status_t   
 *
 * @author
 *  8/24/2012 - Created Yang Zhang
 *
 ****************************************************************/
fbe_status_t fbe_virtual_drive_spare_info_dump_debug_trace(const fbe_u8_t * module_name,
                                                                 fbe_dbgext_ptr drive_info_ptr,
                                                                 fbe_trace_func_t trace_func,
                                                                 fbe_trace_context_t trace_context,
                                                                 fbe_debug_field_info_t *field_info_p,
                                                                 fbe_u32_t spaces_to_indent)
{
	fbe_status_t status = FBE_STATUS_OK;
    fbe_dump_debug_trace_prefix_t new_prefix;

    fbe_debug_dump_generate_prefix(&new_prefix, (fbe_dump_debug_trace_prefix_t *)trace_context,
                                   field_info_p->name);

    status = fbe_debug_display_structure(module_name, trace_func, &new_prefix,
		                                 drive_info_ptr + field_info_p->offset,
                                         &fbe_virtual_drive_spare_dump_struct_info,
                                         1 /* fields per line */,
                                         0);
	return status;
}
fbe_status_t fbe_virtual_drive_mirror_optimization_info_dump_debug_trace(const fbe_u8_t * module_name,
                                                                        fbe_dbgext_ptr base_ptr,
                                                                        fbe_trace_func_t trace_func,
                                                                        fbe_trace_context_t trace_context,
                                                                        fbe_debug_field_info_t *field_info_p,
                                                                        fbe_u32_t spaces_to_indent)
{
    fbe_mirror_optimization_t mirror_opt_db;
    fbe_u32_t mirror_opt_db_offset;
    fbe_u32_t i =0;

	
    FBE_GET_FIELD_OFFSET(module_name, fbe_mirror_t, "mirror_opt_db", &mirror_opt_db_offset);
    FBE_READ_MEMORY(base_ptr + mirror_opt_db_offset, &mirror_opt_db, sizeof(fbe_mirror_optimization_t));

	for(i = 0; i < FBE_MIRROR_MAX_WIDTH; i++)
    {
		trace_func(trace_context, "mirror_opt_db.num_reads_outstanding: 0x%x\n", mirror_opt_db.num_reads_outstanding[i]);	
	}
	for(i = 0; i < FBE_MIRROR_MAX_WIDTH; i++)
    {
		trace_func(trace_context, "mirror_opt_db.current_lba: 0x%x\n", (unsigned int)mirror_opt_db.current_lba[i]);	
	}

    return FBE_STATUS_OK;
}



/*************************
 * end file fbe_virtual_drive_dump_debug.c
 *************************/
