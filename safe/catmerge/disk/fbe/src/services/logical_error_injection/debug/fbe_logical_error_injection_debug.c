/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_logical_error_injection_debug.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the debug functions for the logical error injection service.
 *
 * @author
 *  4/18/2011 - Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_types.h"
#include "fbe/fbe_dbgext.h"
#include "fbe/fbe_transport.h"
#include "fbe_block_transport.h"
#include "fbe_transport_debug.h"
#include "fbe_logical_error_injection_private.h"

/*!*******************************************************************
 * @var fbe_logical_error_injection_debug_err_type_strings
 *********************************************************************
 * @brief Gives us an array of strings for each error type.
 *
 *********************************************************************/
const fbe_char_t *fbe_logical_error_injection_debug_err_type_strings[FBE_XOR_ERR_TYPE_MAX + 1] = {FBE_XOR_ERROR_TYPE_STRINGS};

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

fbe_status_t fbe_logical_error_injection_debug_display_description(const fbe_u8_t * module_name,
                                                                   fbe_dbgext_ptr base_ptr,
                                                                   fbe_trace_func_t trace_func,
                                                                   fbe_trace_context_t trace_context,
                                                                   fbe_debug_field_info_t *field_info_p,
                                                                   fbe_u32_t spaces_to_indent);
fbe_status_t fbe_logical_error_injection_debug_display_error_info(const fbe_u8_t * module_name,
                                                                  fbe_dbgext_ptr base_ptr,
                                                                  fbe_trace_func_t trace_func,
                                                                  fbe_trace_context_t trace_context,
                                                                  fbe_debug_field_info_t *field_info_p,
                                                                  fbe_u32_t spaces_to_indent);

fbe_status_t fbe_logical_error_injection_debug_display_object(const fbe_u8_t * module_name,
                                                              fbe_dbgext_ptr object_ptr,
                                                              fbe_trace_func_t trace_func,
                                                              fbe_trace_context_t trace_context,
                                                              fbe_u32_t spaces_to_indent);
fbe_status_t fbe_logical_error_injection_display_first_media_error_lba(const fbe_u8_t * module_name,
                                                                       fbe_dbgext_ptr lba_ptr,
                                                                       fbe_trace_func_t trace_func,
                                                                       fbe_trace_context_t trace_context,
                                                                       fbe_debug_field_info_t *field_info_p,
                                                                       fbe_u32_t spaces_to_indent);
fbe_status_t fbe_logical_error_injection_display_last_media_error_lba(const fbe_u8_t * module_name,
                                                                      fbe_dbgext_ptr lba_ptr,
                                                                      fbe_trace_func_t trace_func,
                                                                      fbe_trace_context_t trace_context,
                                                                      fbe_debug_field_info_t *field_info_p,
                                                                      fbe_u32_t spaces_to_indent);

/*!*******************************************************************
 * @var fbe_logical_error_injection_debug_object_queue_info_t
 *********************************************************************
 * @brief This structure holds info used to display an lei
 *        queue using the "queue_element" field of the lei object.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_QUEUE_INFO(fbe_logical_error_injection_object_t, "queue_element", 
                             FBE_TRUE, /* queue element is the first field in the object. */
                             fbe_logical_error_injection_debug_active_object_queue_info,
                             fbe_logical_error_injection_debug_display_object);


/*!*******************************************************************
 * @var fbe_logical_error_injection_debug_service_field_info
 *********************************************************************
 * @brief Information about each of the fields of logical_error_injection service.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_logical_error_injection_debug_service_field_info[] =
{
    FBE_DEBUG_DECLARE_FIELD_INFO("b_enabled", fbe_bool_t, FBE_FALSE, "%d"),
    FBE_DEBUG_DECLARE_FIELD_INFO("num_objects", fbe_u32_t, FBE_FALSE, "%d"),
    FBE_DEBUG_DECLARE_FIELD_INFO("num_objects_enabled", fbe_u32_t, FBE_FALSE, "%d"),
    FBE_DEBUG_DECLARE_FIELD_INFO("num_records", fbe_u32_t, FBE_FALSE, "%d"),
    FBE_DEBUG_DECLARE_FIELD_INFO("num_errors_injected", fbe_u32_t, FBE_FALSE, "%d"),
    FBE_DEBUG_DECLARE_FIELD_INFO("correctable_errors_detected", fbe_u32_t, FBE_FALSE, "%d"),
    FBE_DEBUG_DECLARE_FIELD_INFO("uncorrectable_errors_detected", fbe_u32_t, FBE_FALSE, "%d"),
    FBE_DEBUG_DECLARE_FIELD_INFO("num_validations", fbe_u32_t, FBE_FALSE, "%d"),
    FBE_DEBUG_DECLARE_FIELD_INFO("num_failed_validations", fbe_u32_t, FBE_FALSE, "%d"),

    FBE_DEBUG_DECLARE_FIELD_INFO("table_flags", fbe_u32_t, FBE_FALSE, "0x%x"),

    FBE_DEBUG_DECLARE_FIELD_INFO("b_initialized", fbe_u32_t, FBE_FALSE, "%d"),
    FBE_DEBUG_DECLARE_FIELD_INFO("max_lba", fbe_lba_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("poc_injection", fbe_bool_t, FBE_FALSE, "%d"),
    FBE_DEBUG_DECLARE_FIELD_INFO("num_passes_per_trace", fbe_u32_t, FBE_FALSE, "%d"),
    FBE_DEBUG_DECLARE_FIELD_INFO("num_ios_per_trace", fbe_u32_t, FBE_FALSE, "%d"),
    FBE_DEBUG_DECLARE_FIELD_INFO("num_seconds_per_trace", fbe_u32_t, FBE_FALSE, "%d"),
	FBE_DEBUG_DECLARE_FIELD_INFO("last_trace_time", fbe_time_t, FBE_FALSE, "%d"),
	FBE_DEBUG_DECLARE_FIELD_INFO("b_ignore_corrupt_crc_data_errs", fbe_bool_t, FBE_FALSE, "%d"),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("unmatched_record_p", fbe_u64_t, FBE_FALSE, "0x%x",
                                    fbe_debug_display_pointer),
	FBE_DEBUG_DECLARE_FIELD_INFO_FN("log_p", fbe_u64_t, FBE_FALSE, "0x%x",
                                    fbe_debug_display_pointer),
    FBE_DEBUG_DECLARE_FIELD_INFO_QUEUE("active_object_head", fbe_queue_head_t, FBE_FALSE, 
                                       &fbe_logical_error_injection_debug_active_object_queue_info),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("table_desc", fbe_u32_t, FBE_FALSE, "%d",
                                    fbe_logical_error_injection_debug_display_description),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("error_info", fbe_u32_t, FBE_FALSE, "%d",
                                    fbe_logical_error_injection_debug_display_error_info),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_logical_error_injection_debug_service_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        logical_error_injection service structure.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_logical_error_injection_service_t,
                              fbe_logical_error_injection_debug_service_struct_info,
                              &fbe_logical_error_injection_debug_service_field_info[0]); 

/*!*******************************************************************
 * @var fbe_logical_error_injection_debug_record_info
 *********************************************************************
 * @brief Information about each of the fields of logical error record.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_logical_error_injection_debug_record_info[] =
{
    FBE_DEBUG_DECLARE_FIELD_INFO("pos_bitmap", fbe_raid_position_bitmask_t, FBE_TRUE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("width", fbe_u16_t, FBE_FALSE, "%d"),
    FBE_DEBUG_DECLARE_FIELD_INFO("opcode", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("lba", fbe_lba_t, FBE_FALSE, "%d"),
    FBE_DEBUG_DECLARE_FIELD_INFO("blocks", fbe_block_count_t, FBE_FALSE, "%d"),
    FBE_DEBUG_DECLARE_FIELD_INFO("err_type", fbe_xor_error_type_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("err_mode", fbe_logical_error_injection_mode_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("err_count", fbe_u16_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("err_limit", fbe_u16_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("skip_count", fbe_u16_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("skip_limit", fbe_u16_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("err_adj", fbe_raid_position_bitmask_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("start_bit", fbe_u16_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("num_bits", fbe_u16_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("bit_adj", fbe_u16_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("crc_det", fbe_u16_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_logical_error_injection_debug_service_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        fbe_logical_error_injection_record_t structure.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_logical_error_injection_record_t,
                              fbe_logical_error_injection_debug_record_struct_info,
                              &fbe_logical_error_injection_debug_record_info[0]); 

/*!*******************************************************************
 * @var fbe_logical_error_injection_debug_object_info
 *********************************************************************
 * @brief Information about each of the fields of logical error record.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_logical_error_injection_debug_object_info[] =
{

    FBE_DEBUG_DECLARE_FIELD_INFO("b_enabled", fbe_bool_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("object_id", fbe_object_id_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("package_id", fbe_package_id_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("capacity", fbe_lba_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("stripe_size", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("num_verify_read_media_errors_injected", fbe_u64_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("num_verify_write_verify_blocks_remapped", fbe_u64_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("errors_in_progress", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("num_errors_injected", fbe_u64_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("num_validations", fbe_u64_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("active_record_count", fbe_u64_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("num_ios", fbe_u32_t, FBE_FALSE, "0x%x"),
	FBE_DEBUG_DECLARE_FIELD_INFO_FN("first_media_error_lba", fbe_lba_t, FBE_FALSE, "0x%x",
		                            fbe_logical_error_injection_display_first_media_error_lba),
	FBE_DEBUG_DECLARE_FIELD_INFO_FN("last_media_error_lba", fbe_lba_t, FBE_FALSE, "0x%x",
		                            fbe_logical_error_injection_display_last_media_error_lba),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_logical_error_injection_debug_object_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        fbe_logical_error_injection_object_t structure.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_logical_error_injection_object_t,
                              fbe_logical_error_injection_debug_object_struct_info,
                              &fbe_logical_error_injection_debug_object_info[0]); 
/*!**************************************************************
 * fbe_logical_error_injection_debug_display()
 ****************************************************************
 * @brief
 *  Display info on logical error injection structures..
 *  
 * @param module_name - Module name to use in symbol lookup.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param spaces_to_indent - Spaces to indent this display.
 * 
 * @return fbe_status_t
 *
 * @author
 *  4/18/2011 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_logical_error_injection_debug_display(const fbe_u8_t * module_name,
                                                       fbe_trace_func_t trace_func,
                                                       fbe_trace_context_t trace_context,
                                                       fbe_u32_t spaces_to_indent)
{
    fbe_dbgext_ptr logical_error_injection_service_p = 0;

    /* Display basic logical_error_injection information.
     */
	FBE_GET_EXPRESSION(module_name, fbe_logical_error_injection_service, &logical_error_injection_service_p);
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    trace_func(trace_context, "logical error injection service info: \n");
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent + 2);
    fbe_debug_display_structure(module_name, trace_func, trace_context, logical_error_injection_service_p,
                                &fbe_logical_error_injection_debug_service_struct_info,
                                4 /* fields per line */,
                                spaces_to_indent + 2);
    trace_func(trace_context, "\n");
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_logical_error_injection_debug_display()
 ******************************************/

/*!**************************************************************
 * fbe_logical_error_injection_debug_display_description()
 ****************************************************************
 * @brief
 *  Display string.  This reads in the pointer located
 *  at this field's offset.
 *
 * @param module_name - Module name to use in symbol lookup.
 * @param struct_ptr - ptr to value to display.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param field_info_p - Info for getting and displaying the field. 
 * @param spaces_to_indent - Number of spaces to indent the next line(s).
 * 
 * @return fbe_status_t   
 *
 * @author
 *  4/18/2011 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_logical_error_injection_debug_display_description(const fbe_u8_t * module_name,
                                                                   fbe_dbgext_ptr base_ptr,
                                                                   fbe_trace_func_t trace_func,
                                                                   fbe_trace_context_t trace_context,
                                                                   fbe_debug_field_info_t *field_info_p,
                                                                   fbe_u32_t spaces_to_indent)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t index;
    fbe_logical_error_injection_table_description_t desc;

    FBE_READ_MEMORY(base_ptr + field_info_p->offset, &desc, sizeof(fbe_logical_error_injection_table_description_t));

    /* See if there is a terminator.
     */
    for (index = 0; index < FBE_LOGICAL_ERROR_INJECTION_DESCRIPTION_MAX_CHARS; index++)
    {
        if (desc[index] == 0)
        {
            break;
        }
    }

    if (index == FBE_LOGICAL_ERROR_INJECTION_DESCRIPTION_MAX_CHARS)
    {
        trace_func(trace_context, "%s: description is too long\n",
		   field_info_p->name);
    }

    trace_func(trace_context, "%s: (%s)\n", field_info_p->name, desc);
    return status;
}
/**************************************
 * end fbe_logical_error_injection_debug_display_description
 **************************************/

/*!**************************************************************
 * fbe_logical_error_injection_debug_display_error_record()
 ****************************************************************
 * @brief
 *  Display error record information.
 *
 * @param module_name - Module name to use in symbol lookup.
 * @param struct_ptr - ptr to value to display.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param field_info_p - Info for getting and displaying the field. 
 * @param spaces_to_indent - Number of spaces to indent the next line(s).
 * 
 * @return fbe_status_t   
 *
 * @author
 *  4/18/2011 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t
fbe_logical_error_injection_debug_display_error_record(const fbe_u8_t * module_name,
                                                       fbe_dbgext_ptr rec_p,
                                                       fbe_trace_func_t trace_func,
                                                       fbe_trace_context_t trace_context,
                                                       fbe_u32_t spaces_to_indent)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_logical_error_injection_record_t record;

    FBE_READ_MEMORY(rec_p, &record, sizeof(fbe_logical_error_injection_record_t));

    trace_func(trace_context, "%5x ", record.pos_bitmap);
    trace_func(trace_context, "%08llX %3llx ", (unsigned long long)record.lba,
	       (unsigned long long)record.blocks);
    if (record.err_type < FBE_XOR_ERR_TYPE_MAX)
    {
        trace_func(trace_context, "%38s ", fbe_logical_error_injection_debug_err_type_strings[record.err_type]);
    }
    else
    {
        trace_func(trace_context, "%32s (%3d) ", "unknown err type", record.err_type);
    }
    trace_func(trace_context, "%d ", record.err_mode);
    trace_func(trace_context, "%3x %3x ", record.err_count, record.err_limit);
    trace_func(trace_context, "%5s ", record.err_adj ? "Yes" : "No"); 
    trace_func(trace_context, "%4s ", record.start_bit ? "Yes" : "No"); 
    trace_func(trace_context, "%4s ", record.num_bits ? "Yes" : "No"); 
    trace_func(trace_context, "%4x ", record.bit_adj); 
    trace_func(trace_context, "%5x ", record.crc_det); 
    trace_func(trace_context, "\n");
    return status;
}
/**************************************
 * end fbe_logical_error_injection_debug_display_error_record
 **************************************/

/*!**************************************************************
 * fbe_logical_error_injection_debug_display_error_info()
 ****************************************************************
 * @brief
 *  Display error information.
 *
 * @param module_name - Module name to use in symbol lookup.
 * @param struct_ptr - ptr to value to display.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param field_info_p - Info for getting and displaying the field. 
 * @param spaces_to_indent - Number of spaces to indent the next line(s).
 * 
 * @return fbe_status_t   
 *
 * @author
 *  4/18/2011 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_logical_error_injection_debug_display_error_info(const fbe_u8_t * module_name,
                                                                  fbe_dbgext_ptr base_ptr,
                                                                  fbe_trace_func_t trace_func,
                                                                  fbe_trace_context_t trace_context,
                                                                  fbe_debug_field_info_t *field_info_p,
                                                                  fbe_u32_t spaces_to_indent)
{
    fbe_u32_t index;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_dbgext_ptr error_records_p = base_ptr + field_info_p->offset;
    fbe_u32_t size;

    FBE_GET_TYPE_SIZE(module_name, fbe_logical_error_injection_record_t, &size);

    trace_func(trace_context, "\n\n");

    trace_func(trace_context, "                               Err                           Err Err Err  Err  Str   Num  Bit  CRC\n");
    trace_func(trace_context, " Rec  Pos  LBA       Blk       Type                          Mod Cnt Lmt  Adj  Bit   Bit  Adj  Det\n");
    trace_func(trace_context, "----------------------------------------------------------------------------------------------------\n"); 

    /* Display the entire table
     */
    for (index = 0; index < FBE_LOGICAL_ERROR_INJECTION_MAX_RECORDS; index++)
    {
        trace_func(trace_context, "%03d) ", index);
        fbe_logical_error_injection_debug_display_error_record(module_name, error_records_p, trace_func,
                                                               trace_context, spaces_to_indent);
        error_records_p += size;
    }
    return status;
}
/**************************************
 * end fbe_logical_error_injection_debug_display_error_info
 **************************************/

/*!**************************************************************
 * fbe_logical_error_injection_debug_display_object()
 ****************************************************************
 * @brief
 *  Display info on logical_error_injection objects.
 *  
 * @param module_name - Module name to use in symbol lookup.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param spaces_to_indent - Spaces to indent this display.
 * 
 * @return fbe_status_t
 *
 * @author
 *  4/18/2011 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_logical_error_injection_debug_display_object(const fbe_u8_t * module_name,
                                                              fbe_dbgext_ptr object_ptr,
                                                              fbe_trace_func_t trace_func,
                                                              fbe_trace_context_t trace_context,
                                                              fbe_u32_t spaces_to_indent)
{
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    trace_func(trace_context, "object_p: 0x%llx", (unsigned long long)object_ptr);

    fbe_debug_display_structure(module_name, trace_func, trace_context, object_ptr,
                                &fbe_logical_error_injection_debug_object_struct_info,
                                4 /* fields per line */,
                                spaces_to_indent + 10);
    trace_func(trace_context, "\n");
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_logical_error_injection_debug_display_object()
 ******************************************/

/*!**************************************************************
 * fbe_logical_error_injection_display_first_media_error_lba()
 ****************************************************************
 * @brief
 *  Display info on logical_error_injection first media error lbas.
 *  
 * @param module_name - Module name to use in symbol lookup.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param spaces_to_indent - Spaces to indent this display.
 * 
 * @return fbe_status_t
 *
 * @author
 *  11/04/2011 - Created. Omer Miranda
 *
 ****************************************************************/
fbe_status_t fbe_logical_error_injection_display_first_media_error_lba(const fbe_u8_t * module_name,
                                                                       fbe_dbgext_ptr lba_ptr,
                                                                       fbe_trace_func_t trace_func,
                                                                       fbe_trace_context_t trace_context,
                                                                       fbe_debug_field_info_t *field_info_p,
                                                                       fbe_u32_t spaces_to_indent)
{
	fbe_lba_t first_media_error_lba[FBE_RAID_MAX_DISK_ARRAY_WIDTH];
	fbe_u32_t index;
	fbe_u32_t next_level = 3;
    fbe_status_t status = FBE_STATUS_OK;

	FBE_READ_MEMORY(lba_ptr + field_info_p->offset, &first_media_error_lba, (sizeof(fbe_lba_t) * FBE_RAID_MAX_DISK_ARRAY_WIDTH));
	trace_func(trace_context, "%s: ", field_info_p->name);
	trace_func(trace_context, "\n");
	fbe_trace_indent(trace_func, trace_context, spaces_to_indent);

	for(index = 0; index < FBE_RAID_MAX_DISK_ARRAY_WIDTH; index++)
	{
		trace_func(trace_context, "[%2d]: 0x%-20llx",index,
			   (unsigned long long)first_media_error_lba[index]);
		if(index == next_level)
		{
			trace_func(trace_context, "\n");
			fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
			next_level += 4;
		}
	}

	return status;
}
/***********************************************************
 * end fbe_logical_error_injection_display_first_media_error_lba()
 ***********************************************************/

/*!**************************************************************
 * fbe_logical_error_injection_display_last_media_error_lba()
 ****************************************************************
 * @brief
 *  Display info on logical_error_injection last media error lbas.
 *  
 * @param module_name - Module name to use in symbol lookup.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param spaces_to_indent - Spaces to indent this display.
 * 
 * @return fbe_status_t
 *
 * @author
 *  11/04/2011 - Created. Omer Miranda
 *
 ****************************************************************/
fbe_status_t fbe_logical_error_injection_display_last_media_error_lba(const fbe_u8_t * module_name,
                                                                      fbe_dbgext_ptr lba_ptr,
                                                                      fbe_trace_func_t trace_func,
                                                                      fbe_trace_context_t trace_context,
                                                                      fbe_debug_field_info_t *field_info_p,
                                                                      fbe_u32_t spaces_to_indent)
{
	fbe_lba_t last_media_error_lba[FBE_RAID_MAX_DISK_ARRAY_WIDTH];
	fbe_u32_t index;
	fbe_u32_t next_level = 3;
    fbe_status_t status = FBE_STATUS_OK;

	FBE_READ_MEMORY(lba_ptr + field_info_p->offset, &last_media_error_lba, (sizeof(fbe_lba_t) * FBE_RAID_MAX_DISK_ARRAY_WIDTH));
	trace_func(trace_context, "%s: ", field_info_p->name);
	trace_func(trace_context, "\n");
	fbe_trace_indent(trace_func, trace_context, spaces_to_indent);

	for(index = 0; index < FBE_RAID_MAX_DISK_ARRAY_WIDTH; index++)
	{
		trace_func(trace_context, "[%2d]: 0x%-20llx",index,
			   (unsigned long long)last_media_error_lba[index]);
		if(index == next_level)
		{
			trace_func(trace_context, "\n");
			fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
			next_level += 4;
		}
	}

	return status;
}
/***********************************************************
 * end fbe_logical_error_injection_display_last_media_error_lba()
 ***********************************************************/

/*******************************************
 * end file fbe_logical_error_injection_debug.c
 ******************************************/
