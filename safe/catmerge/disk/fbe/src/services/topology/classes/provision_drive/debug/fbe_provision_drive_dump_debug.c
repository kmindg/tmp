
/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_provision_drive_dump_debug.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the debug functions for dump the provision drive object.
 *
 * @author
 *  08/15/2012 - Created. Jingcheng Zhang
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
#include "fbe_provision_drive_private.h"
#include "fbe_base_object_trace.h"
#include "fbe_provision_drive_debug.h"
#include "fbe_transport_debug.h"
#include "fbe/fbe_raid_group.h"
#include "fbe_raid_geometry.h"
#include "fbe_base_config_debug.h"
#include "fbe/fbe_payload_metadata_operation.h"
#include "fbe_metadata_debug.h"
#include "fbe_topology_debug.h"


/*************************
 *   FUNCTION DEFINITIONS
 *************************/


/*!*******************************************************************
 * @var fbe_provision_drive_metadata_memory_dump_field_info
 *********************************************************************
 * @brief Information about each of the fields of provision drive
 *        metadata memeory.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_provision_drive_metadata_memory_dump_field_info[] =
{
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("base_config_metadata_memory", fbe_base_config_metadata_memory_t, FBE_TRUE, "0x%x",
                                    fbe_base_config_metadata_memory_dump_debug_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO("flags", fbe_provision_drive_clustered_flags_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_provision_drive_metadata_memory_dump_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        provision drive metadata memory.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_provision_drive_metadata_memory_t,
                              fbe_provision_drive_metadata_memory_dump_struct_info,
                              &fbe_provision_drive_metadata_memory_dump_field_info[0]);

/*!**************************************************************
 * fbe_provision_drive_metadata_memory_dump_debug_trace()
 ****************************************************************
 * @brief
 *  Display the provision drive metadata memeory.
 *
 * @param module_name - Module name to use in symbol lookup.
 * @param object_ptr - ptr to value to display.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param spaces_to_indent - Number of spaces to indent the next line(s).
 * 
 * @return fbe_status_t   
 *
 * @author
 *  8/15/2012 - Created Jingcheng Zhang
 *
 ****************************************************************/
fbe_status_t fbe_provision_drive_metadata_memory_dump_debug_trace(const fbe_u8_t * module_name,
                                                             fbe_dbgext_ptr record_ptr,
                                                             fbe_trace_func_t trace_func,
                                                             fbe_trace_context_t trace_context,
                                                             fbe_u32_t spaces_to_indent)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_dump_debug_trace_prefix_t new_prefix;

    fbe_debug_dump_generate_prefix(&new_prefix, 
                                  (fbe_dump_debug_trace_prefix_t *)trace_context,
                                  "provision_drive_metadata_memory");
    status = fbe_debug_display_structure(module_name, trace_func, &new_prefix, record_ptr,
                                         &fbe_provision_drive_metadata_memory_dump_struct_info,
                                         1 /* fields per line */,
                                         0);
    return status;
}

/*!**************************************************************
 * fbe_provision_drive_remove_timestamp_dump_debug_trace()
 ****************************************************************
 * @brief
 *  Display the remove timestamp of PVD.
 *
 * @param module_name - Module name to use in symbol lookup.
 * @param struct_ptr - ptr to value to display.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param struct_ptr - This is the ptr to the structure.
 * @param field_info_p - Field to init. 
 * @param spaces_to_indent - Number of spaces to indent the next line(s).
 * 
 * @return fbe_status_t   
 *
 * @author
 *  8/21/2012 - Created. Jingcheng Zhang
 *
 ****************************************************************/
fbe_status_t fbe_provision_drive_remove_timestamp_dump_debug_trace(const fbe_u8_t * module_name,
                                              fbe_dbgext_ptr base_ptr,
                                              fbe_trace_func_t trace_func,
                                              fbe_trace_context_t trace_context,
                                              fbe_debug_field_info_t *field_info_p,
                                              fbe_u32_t spaces_to_indent)
{
    fbe_system_time_t remove_timestamp;
    fbe_u32_t timestamp_offset;

    FBE_GET_FIELD_OFFSET(module_name, fbe_provision_drive_nonpaged_metadata_t, "remove_timestamp", &timestamp_offset);
    FBE_READ_MEMORY(base_ptr + timestamp_offset, &remove_timestamp, sizeof(fbe_system_time_t));

    trace_func(trace_context, "remove_timestamp.year: 0x%x\n", remove_timestamp.year);	
    trace_func(trace_context, "remove_timestamp.month: 0x%x\n", remove_timestamp.month);	
    trace_func(trace_context, "remove_timestamp.day: 0x%x\n", remove_timestamp.day);
	trace_func(trace_context, "remove_timestamp.hour: 0x%x\n", remove_timestamp.hour);	
	trace_func(trace_context, "remove_timestamp.minute: 0x%x\n", remove_timestamp.minute);	
	trace_func(trace_context, "remove_timestamp.second: 0x%x\n", remove_timestamp.second);	
	trace_func(trace_context, "remove_timestamp.milliseconds: 0x%x\n", remove_timestamp.milliseconds);

    return FBE_STATUS_OK;
}


/*!*******************************************************************
 * @var fbe_provision_drive_nonpaged_metadata_field_info
 *********************************************************************
 * @brief Information about each of the fields of provision drive
 *        non-paged metadata record.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_provision_drive_nonpaged_metadata_dump_field_info[] =
{
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("base_config_nonpaged_metadata", fbe_base_config_nonpaged_metadata_t, FBE_TRUE, "0x%x", 
                                    fbe_base_config_nonpaged_metadata_dump_debug_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO("sniff_verify_checkpoint", fbe_lba_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("zero_checkpoint", fbe_lba_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("zero_on_demand", fbe_bool_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("end_of_life_state", fbe_bool_t, FBE_FALSE, "0x%x"),
	FBE_DEBUG_DECLARE_FIELD_INFO("media_error_lba", fbe_lba_t, FBE_FALSE, "0x%x"),
	FBE_DEBUG_DECLARE_FIELD_INFO("drive_fault_state", fbe_bool_t, FBE_FALSE, "0x%x"),
	FBE_DEBUG_DECLARE_FIELD_INFO_FN("remove_timestamp", fbe_system_time_t, FBE_FALSE, "0x%x",
                                    fbe_provision_drive_remove_timestamp_dump_debug_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO("verify_invalidate_checkpoint", fbe_lba_t, FBE_FALSE, "0x%x"),
	FBE_DEBUG_DECLARE_FIELD_INFO_FN("nonpaged_drive_info", fbe_provision_drive_nonpaged_metadata_drive_info_t, FBE_FALSE, "0x%x", 
                                    fbe_provision_drive_nonpaged_drive_info_dump_debug_trace),	
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_provision_drive_nonpaged_metadata_dump_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        provision drive non paged metadata record.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_provision_drive_nonpaged_metadata_t,
                              fbe_provision_drive_nonpaged_metadata_dump_struct_info,
                              &fbe_provision_drive_nonpaged_metadata_dump_field_info[0]);

/*!*******************************************************************
 * @var fbe_provision_drive_nonpaged_drive_dump_field_info
 *********************************************************************
 * @brief Information about each of the fields of provision drive
 *        non-paged drive info.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_provision_drive_nonpaged_drive_dump_field_info[] =
{
    FBE_DEBUG_DECLARE_FIELD_INFO("port_number", fbe_u32_t, FBE_TRUE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("enclosure_number", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("slot_number", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("drive_type", fbe_drive_type_t, FBE_FALSE, "0x%x",
	                                fbe_provision_drive_type_dump_debug_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_provision_drive_nonpaged_drive_dump_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *       fbe_provision_drive_nonpaged_metadata_drive_info_t.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_provision_drive_nonpaged_metadata_drive_info_t,
                              fbe_provision_drive_nonpaged_drive_dump_struct_info,
                              &fbe_provision_drive_nonpaged_drive_dump_field_info[0]);

/*!**************************************************************
 * fbe_provision_drive_nonpaged_drive_info_dump_debug_trace()
 ****************************************************************
 * @brief
 *  Display the provision drive non-paged drive info.
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
 *  8/15/2012 - Created Jingcheng Zhang
 *
 ****************************************************************/
fbe_status_t fbe_provision_drive_nonpaged_drive_info_dump_debug_trace(const fbe_u8_t * module_name,
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
                                         &fbe_provision_drive_nonpaged_drive_dump_struct_info,
                                         1 /* fields per line */,
                                         0);
	return status;
}
/*******************************************************
 * end fbe_provision_drive_nonpaged_drive_info_dump_debug_trace()
 ******************************************************/
/*!**************************************************************
 * fbe_provision_drive_type_dump_debug_trace()
 ****************************************************************
 * @brief
 *  Display the provision drive's drive type info .
 *
 * @param module_name - Module name to use in symbol lookup.
 * @param drive_type_ptr - ptr to value to display.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param spaces_to_indent - Number of spaces to indent the next line(s).
 * 
 * @return fbe_status_t   
 *
 * @author
 *  11/02/2011 - Created Omer Miranda
 *
 ****************************************************************/
fbe_status_t fbe_provision_drive_type_dump_debug_trace(const fbe_u8_t * module_name,
                                                  fbe_dbgext_ptr drive_type_ptr,
                                                  fbe_trace_func_t trace_func,
                                                  fbe_trace_context_t trace_context,
                                                  fbe_debug_field_info_t *field_info_p,
                                                  fbe_u32_t spaces_to_indent)
{
	fbe_status_t status = FBE_STATUS_OK;
	fbe_drive_type_t drive_type;

	FBE_READ_MEMORY(drive_type_ptr + field_info_p->offset, &drive_type, sizeof(fbe_drive_type_t));

	switch(drive_type)
	{
		case FBE_DRIVE_TYPE_INVALID:
			trace_func(trace_context, "%s: %d (%s) \n", field_info_p->name, drive_type, "INVALID");
			break;
		case FBE_DRIVE_TYPE_FIBRE:
			trace_func(trace_context, "%s: %d (%s) \n", field_info_p->name, drive_type, "FIBRE");
			break;
		case FBE_DRIVE_TYPE_SAS:
			trace_func(trace_context, "%s: %d (%s) \n", field_info_p->name, drive_type, "SAS");
			break;
		case FBE_DRIVE_TYPE_SATA:
			trace_func(trace_context, "%s: %d (%s) \n", field_info_p->name, drive_type, "SATA");
			break;
		case FBE_DRIVE_TYPE_SAS_FLASH_HE:
			trace_func(trace_context, "%s: %d (%s) \n", field_info_p->name, drive_type, "SAS_FLASH_HE");
			break;
        case FBE_DRIVE_TYPE_SAS_FLASH_ME:
            trace_func(trace_context, "%s: %d (%s) \n", field_info_p->name, drive_type, "SAS_FLASH_ME");
            break;
        case FBE_DRIVE_TYPE_SAS_FLASH_LE:
            trace_func(trace_context, "%s: %d (%s) \n", field_info_p->name, drive_type, "SAS_FLASH_LE");
            break;
        case FBE_DRIVE_TYPE_SAS_FLASH_RI:
            trace_func(trace_context, "%s: %d (%s) \n", field_info_p->name, drive_type, "SAS_FLASH_RI");
            break;
		case FBE_DRIVE_TYPE_SATA_FLASH_HE:
			trace_func(trace_context, "%s: %d (%s) \n", field_info_p->name, drive_type, "SATA_FLASH_HE");
			break;
		case FBE_DRIVE_TYPE_SAS_NL:
			trace_func(trace_context, "%s: %d (%s) \n", field_info_p->name, drive_type, "SAS_NL");
			break;
		case FBE_DRIVE_TYPE_SATA_PADDLECARD:
			trace_func(trace_context, "%s: %d (%s) \n", field_info_p->name, drive_type, "SATA_PADDLECARD");
			break;
		default:
			trace_func(trace_context, "%s: %d (%s) \n", field_info_p->name, drive_type, "UNKNOWN");
            break;
	}

	return status;
}
/*******************************************************
 * end fbe_provision_drive_type_dump_debug_trace()
 ******************************************************/

/*!**************************************************************
 * fbe_provision_drive_non_paged_record_dump_debug_trace()
 ****************************************************************
 * @brief
 *  Display the provision drive non-paged record.
 *
 * @param module_name - Module name to use in symbol lookup.
 * @param object_ptr - ptr to value to display.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param spaces_to_indent - Number of spaces to indent the next line(s).
 * 
 * @return fbe_status_t   
 *
 * @author
 *  08/15/2012 - Created Jingcheng Zhang
 *
 ****************************************************************/
fbe_status_t fbe_provision_drive_non_paged_record_dump_debug_trace(const fbe_u8_t * module_name,
                                                              fbe_dbgext_ptr record_ptr,
                                                              fbe_trace_func_t trace_func,
                                                              fbe_trace_context_t trace_context,
                                                              fbe_u32_t spaces_to_indent)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_dump_debug_trace_prefix_t new_prefix;

    fbe_debug_dump_generate_prefix(&new_prefix, (fbe_dump_debug_trace_prefix_t *)trace_context,
                                   "provision_drive_non_paged_metadata");
    status = fbe_debug_display_structure(module_name, trace_func, &new_prefix, record_ptr,
                                         &fbe_provision_drive_nonpaged_metadata_dump_struct_info,
                                         1 /* fields per line */,
                                         0);
    return FBE_STATUS_OK;
}
/*******************************************************
 * end fbe_provision_drive_non_paged_record_dump_debug_trace()
 ******************************************************/

/*!**************************************************************
 * fbe_pvd_sniff_verify_report_dump_debug_trace()
 ****************************************************************
 * @brief
 *  Display the PVD sniff verify report information.
 *
 * @param module_name - Module name to use in symbol lookup.
 * @param struct_ptr - ptr to value to display.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param struct_ptr - This is the ptr to the structure.
 * @param field_info_p - Field to init. 
 * @param spaces_to_indent - Number of spaces to indent the next line(s).
 * 
 * @return fbe_status_t   
 *
 * @author
 *  8/21/2012 - Created. Jingcheng Zhang
 *
 ****************************************************************/
fbe_status_t fbe_pvd_sniff_verify_report_dump_debug_trace(const fbe_u8_t * module_name,
                                              fbe_dbgext_ptr base_ptr,
                                              fbe_trace_func_t trace_func,
                                              fbe_trace_context_t trace_context,
                                              fbe_debug_field_info_t *field_info_p,
                                              fbe_u32_t spaces_to_indent)
{
    fbe_provision_drive_verify_report_t report_info;
    fbe_u32_t report_offset;

    FBE_GET_FIELD_OFFSET(module_name, fbe_provision_drive_t, "sniff_verify_report", &report_offset);
    FBE_READ_MEMORY(base_ptr + report_offset, &report_info, sizeof(fbe_provision_drive_verify_report_t));

    trace_func(trace_context, "sniff_verify_report.revision: 0x%x\n", report_info.revision);		
    trace_func(trace_context, "sniff_verify_report.pass_count: 0x%x\n", report_info.pass_count);
    trace_func(trace_context, "sniff_verify_report.current.recov_read_count: 0x%x\n", report_info.current.recov_read_count);
    trace_func(trace_context, "sniff_verify_report.current.unrecov_read_count: 0x%x\n", report_info.current.unrecov_read_count);
    trace_func(trace_context, "sniff_verify_report.previous.recov_read_count: 0x%x\n", report_info.previous.recov_read_count);
    trace_func(trace_context, "sniff_verify_report.previous.unrecov_read_count: 0x%x\n", report_info.previous.unrecov_read_count);
    trace_func(trace_context, "sniff_verify_report.totals.recov_read_count: 0x%x\n", report_info.totals.recov_read_count);
    trace_func(trace_context, "sniff_verify_report.totals.unrecov_read_count: 0x%x\n", report_info.totals.unrecov_read_count);


    return FBE_STATUS_OK;
}

/*!*******************************************************************
 * @var fbe_provision_drive_dump_field_info
 *********************************************************************
 * @brief Information about each of the fields of base config.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_provision_drive_dump_field_info[] =
{
    /* base config is first.
     */
    FBE_DEBUG_DECLARE_FIELD_INFO("debug_flags", fbe_provision_drive_debug_flags_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("config_type", fbe_provision_drive_config_type_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("local_state", fbe_provision_drive_local_state_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("flags", fbe_provision_drive_flags_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("configured_physical_block_size", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("max_drive_xfer_limit", fbe_block_count_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("last_checkpoint_time", fbe_time_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("zero_priority", fbe_traffic_priority_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("verify_priority", fbe_traffic_priority_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("verify_invalidate_priority", fbe_traffic_priority_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("download_start_time", fbe_time_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("ds_disabled_start_time", fbe_time_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("last_zero_percent_notification", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("percent_rebuilt", fbe_u32_t, FBE_FALSE, "%d"),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("sniff_verify_report", fbe_provision_drive_verify_report_t, FBE_FALSE, "0x%x",
                                    fbe_pvd_sniff_verify_report_dump_debug_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("base_config", fbe_object_id_t, FBE_TRUE, "0x%x",
                                    fbe_base_config_dump_debug_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_provision_drive_dump_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        base config.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_provision_drive_t,
                              fbe_provision_drive_dump_struct_info,
                              &fbe_provision_drive_dump_field_info[0]);


/*!**************************************************************
 * @fn fbe_provision_drive_dump_debug_trace(const fbe_u8_t * module_name,
 *                                     fbe_dbgext_ptr provision_drive,
 *                                     fbe_trace_func_t trace_func,
 *                                     fbe_trace_context_t trace_context)
 ****************************************************************
 * @brief
 *  Display all the trace information about the provision drive object.
 *
 * @param  module_name - This is the name of the module we are debugging.
 * @param provision_drive_p - Ptr to provision drive object.
 * @param trace_func - Function to use when tracing.
 * @param trace_context - The context to be passed to the trace function.
 *
 * @return - FBE_STATUS_OK  
 *
 * HISTORY:
 *  08/15/2012 - Created. Jingcheng Zhang
 *
 ****************************************************************/

fbe_status_t fbe_provision_drive_dump_debug_trace(const fbe_u8_t * module_name,
                                             fbe_dbgext_ptr provision_drive_p,
                                             fbe_trace_func_t trace_func,
                                             fbe_trace_context_t trace_context,
                                             fbe_u32_t overall_spaces_to_indent)
{
    fbe_status_t status;
 
    status = fbe_debug_display_structure(module_name, trace_func, trace_context, provision_drive_p,
                                         &fbe_provision_drive_dump_struct_info,
                                         1 /* fields per line */,
                                         0 /*spaces_to_indent*/);

    return status;
} 
/**************************************
 * end fbe_provision_drive_dump_debug_trace()
 **************************************/


/*************************
 * end file fbe_provision_drive_dump_debug.c
 *************************/

