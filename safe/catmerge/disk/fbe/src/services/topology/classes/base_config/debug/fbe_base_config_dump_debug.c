
/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_base_config_dump_debug.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the debug functions for the base config object.
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
#include "fbe_base_object.h"
#include "fbe_base_object_debug.h"
#include "fbe_base_object_trace.h"
#include "fbe_transport_debug.h"
#include "fbe/fbe_base_config.h"
#include "fbe_base_config_debug.h"
#include "../fbe_base_config_private.h"
#include "fbe_metadata.h"
#include "fbe_provision_drive_private.h"
#include "fbe_raid_group_object.h"
#include "fbe_lun_private.h"
#include "fbe_metadata_cmi.h"
/*#include "fbe_metadata_stripe_lock.h"*/
#include "fbe_metadata_debug.h"
#include "fbe_raid_group_debug.h"
#include "fbe_provision_drive_debug.h"
#include "fbe_lun_debug.h"
#include "fbe_virtual_drive_debug.h"
#include "fbe_metadata_stripe_lock_debug.h"
#include "pp_ext.h"

fbe_status_t fbe_base_config_dump_flag_string_trace(fbe_base_config_flags_t flags, 
                                               fbe_trace_func_t trace_func, 
                                               fbe_trace_context_t trace_context);
fbe_status_t fbe_base_config_flag_dump_debug_trace(const fbe_u8_t * module_name,
                                              fbe_dbgext_ptr base_ptr,
                                              fbe_trace_func_t trace_func,
                                              fbe_trace_context_t trace_context,
                                              fbe_debug_field_info_t *field_info_p,
                                              fbe_u32_t spaces_to_indent);


static fbe_class_id_t fbe_dump_class_id = FBE_CLASS_ID_INVALID;

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/*!*******************************************************************
 * @var fbe_sep_version_header_field_info
 *********************************************************************
 * @brief Information about each of the fields of fbe sep version header.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_sep_version_header_dump_field_info[] =
{
    FBE_DEBUG_DECLARE_FIELD_INFO("size", fbe_u32_t, FBE_TRUE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};


/*!*******************************************************************
 * @var fbe_sep_version_header_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        version header.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_sep_version_header_t,
                              fbe_sep_version_header_dump_struct_info,
                              &fbe_sep_version_header_dump_field_info[0]);

/*!**************************************************************
 * fbe_non_paged_version_header_debug_trace()
 ****************************************************************
 * @brief
 *  Display the non-paged metadata version header information.
 *
 * @param module_name - Module name to use in symbol lookup.
 * @param object_ptr - ptr to value to display.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param field_info_p - Field to init. 
 * @param spaces_to_indent - Number of spaces to indent the next line(s).
 * 
 * @return fbe_status_t   
 *
 * @author
 *  8/15/2012 - Created. Jingcheng Zhang
 *
 ****************************************************************/ 
fbe_status_t fbe_non_paged_version_header_dump_debug_trace(const fbe_u8_t * module_name,
                                             fbe_dbgext_ptr object_ptr,
                                             fbe_trace_func_t trace_func,
                                             fbe_trace_context_t trace_context,
                                             fbe_debug_field_info_t *field_info_p,
                                             fbe_u32_t spaces_to_indent)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_dump_debug_trace_prefix_t new_prefix;

    fbe_debug_dump_generate_prefix(&new_prefix, (fbe_dump_debug_trace_prefix_t *)trace_context,
                                   field_info_p->name);

    status = fbe_debug_display_structure(module_name, trace_func, &new_prefix, object_ptr + field_info_p->offset,
                                         &fbe_sep_version_header_dump_struct_info,
                                         1 /* fields per line */,
                                         0 /*spaces_to_indent*/);

    return status;	
}




/*!*******************************************************************
 * @var fbe_base_config_nonpaged_metadata_dump_field_info
 *********************************************************************
 * @brief Information about each of the fields of base config
 *        non-paged metadata.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_base_config_nonpaged_metadata_dump_field_info[] =
{
    FBE_DEBUG_DECLARE_FIELD_INFO("generation_number", fbe_generation_code_t, FBE_FALSE, "0x%x"),
	FBE_DEBUG_DECLARE_FIELD_INFO("operation_bitmask", fbe_base_config_background_operation_t, FBE_FALSE, "0x%x"),
	FBE_DEBUG_DECLARE_FIELD_INFO("nonpaged_metadata_state", fbe_base_config_nonpaged_metadata_state_t, FBE_FALSE, "0x%x"),
	FBE_DEBUG_DECLARE_FIELD_INFO("object_id", fbe_object_id_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("version_header", fbe_sep_version_header_t, FBE_TRUE, "0x%x", 
                                    fbe_non_paged_version_header_dump_debug_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};



/*!*******************************************************************
 * @var fbe_base_config_nonpaged_metadata_dump_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        generation number.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_base_config_nonpaged_metadata_t,
                              fbe_base_config_nonpaged_metadata_dump_struct_info,
                              &fbe_base_config_nonpaged_metadata_dump_field_info[0]);

/*!**************************************************************
 * fbe_base_config_nonpaged_metadata_dump_debug_trace()
 ****************************************************************
 * @brief
 *  Display the base config non paged metadata generation number.
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
 *  08/15/2012 - Created. Jingcheng Zhang
 ****************************************************************/
fbe_status_t fbe_base_config_nonpaged_metadata_dump_debug_trace(const fbe_u8_t * module_name,
                                                           fbe_dbgext_ptr base_ptr,
                                                           fbe_trace_func_t trace_func,
                                                           fbe_trace_context_t trace_context,
                                                           fbe_debug_field_info_t *field_info_p,
                                                           fbe_u32_t spaces_to_indent)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_dump_debug_trace_prefix_t new_prefix;

    fbe_debug_dump_generate_prefix(&new_prefix, (fbe_dump_debug_trace_prefix_t *)trace_context,
                                   field_info_p->name);

    status = fbe_debug_display_structure(module_name, trace_func, &new_prefix, base_ptr,
                                         &fbe_base_config_nonpaged_metadata_dump_struct_info,
                                         1 /* fields per line */,
                                         0);
    return status;
}



/*!**************************************************************
 * fbe_non_paged_record_dump_debug_trace()
 ****************************************************************
 * @brief
 *  Display the non-paged record for all objects like - raid , lun and provision drive.
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
 *  8/15/2012 - Created. Jingcheng Zhang
 *
 ****************************************************************/
fbe_status_t fbe_non_paged_record_dump_debug_trace(const fbe_u8_t * module_name,
                                            fbe_dbgext_ptr object_ptr,
                                            fbe_trace_func_t trace_func,
                                            fbe_trace_context_t trace_context,
                                            fbe_u32_t spaces_to_indent)
{
    fbe_u32_t data_ptr_offset;
    ULONG64 data_ptr;
       
    FBE_GET_FIELD_OFFSET(module_name, fbe_metadata_record_t, "data_ptr", &data_ptr_offset);
    FBE_READ_MEMORY(object_ptr + data_ptr_offset, &data_ptr, sizeof(ULONG64));	

    if (data_ptr == NULL)
    {
        return FBE_STATUS_OK;
    }

    trace_func(NULL, "\n");
    switch(fbe_dump_class_id)
    {
        case FBE_CLASS_ID_PARITY:
        case FBE_CLASS_ID_STRIPER:
        case FBE_CLASS_ID_MIRROR:
        case FBE_CLASS_ID_VIRTUAL_DRIVE: /* Virtual drive objects metadata is the same as its parent class (RAID) */
            fbe_raid_group_non_paged_record_dump_debug_trace(module_name, data_ptr,trace_func,trace_context,spaces_to_indent);
            break;
        case FBE_CLASS_ID_PROVISION_DRIVE:
            fbe_provision_drive_non_paged_record_dump_debug_trace(module_name, data_ptr,trace_func,trace_context,spaces_to_indent);
            break;
        case FBE_CLASS_ID_LUN:
            fbe_lun_non_paged_record_dump_debug_trace(module_name, data_ptr,trace_func,trace_context,spaces_to_indent);
            break;
        default:	
            /* There is no handling for this class yet.
               * Display only class id for it. 
            */
            trace_func(trace_context, "CLASS ID: %d", fbe_dump_class_id);
    }

   return FBE_STATUS_OK;
}

/*!*******************************************************************
 * @var fbe_metadata_record_dump_field_info
 *********************************************************************
 * @brief Information about each of the fields of metadata record.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_metadata_record_dump_field_info[] =
{
    FBE_DEBUG_DECLARE_FIELD_INFO("lba", fbe_lba_t, FBE_TRUE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("data_size", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("data_ptr", fbe_u64_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};


/*!*******************************************************************
 * @var fbe_metadata_record_dump_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        metadata record.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_metadata_record_t,
                              fbe_metadata_record_dump_struct_info,
                              &fbe_metadata_record_dump_field_info[0]);


/*!**************************************************************
 * fbe_non_paged_record_metadata_dump_debug_trace()
 ****************************************************************
 * @brief
 *  Display the non-paged record metadata information.
 *
 * @param module_name - Module name to use in symbol lookup.
 * @param object_ptr - ptr to value to display.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param field_info_p - Field to init. 
 * @param spaces_to_indent - Number of spaces to indent the next line(s).
 * 
 * @return fbe_status_t   
 *
 * @author
 *  8/15/2012 - Created. Jingcheng Zhang
 *
 ****************************************************************/ 
fbe_status_t fbe_non_paged_record_metadata_dump_debug_trace(const fbe_u8_t * module_name,
                                             fbe_dbgext_ptr object_ptr,
                                             fbe_trace_func_t trace_func,
                                             fbe_trace_context_t trace_context,
                                             fbe_debug_field_info_t *field_info_p,
                                             fbe_u32_t spaces_to_indent)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_dump_debug_trace_prefix_t new_prefix;

    fbe_debug_dump_generate_prefix(&new_prefix, (fbe_dump_debug_trace_prefix_t *)trace_context,
                                   field_info_p->name);

    status = fbe_debug_display_structure(module_name, trace_func, &new_prefix, object_ptr + field_info_p->offset,
                                         &fbe_metadata_record_dump_struct_info,
                                         1 /* fields per line */,
                                         0 /*spaces_to_indent*/);

    status = fbe_non_paged_record_dump_debug_trace(module_name,object_ptr + field_info_p->offset,
                                                                trace_func,trace_context, 0);

    return status;	
}


/*!**************************************************************
 * fbe_object_metadata_memory_dump_debug_trace()
 ****************************************************************
 * @brief
 *  Display the clustered memory(metadata memory and metadata_memory_peer) information.
 *  for all objects like raid , lun and provision drive
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
 *  8/15/2012 - Created. Jingcheng Zhang
 *
 ****************************************************************/
fbe_status_t fbe_object_metadata_memory_dump_debug_trace(const fbe_u8_t * module_name,
                                            fbe_dbgext_ptr object_ptr,
                                            fbe_trace_func_t trace_func,
                                            fbe_trace_context_t trace_context,
                                            fbe_u32_t spaces_to_indent)
{
    ULONG64 data_ptr;

    FBE_READ_MEMORY(object_ptr, &data_ptr, sizeof(ULONG64));

    if (data_ptr == NULL)
    {
        return FBE_STATUS_OK;
    }

    switch(fbe_dump_class_id)
    {
        /* All the below are derived from a raid group and do not have any 
         * class specific metadata memory of their own. 
         */
        case FBE_CLASS_ID_PARITY:
        case FBE_CLASS_ID_STRIPER:
        case FBE_CLASS_ID_MIRROR:
        case FBE_CLASS_ID_VIRTUAL_DRIVE:
            fbe_raid_group_metadata_memory_dump_debug_trace(module_name, data_ptr,trace_func,trace_context,spaces_to_indent);	
            break;
        case FBE_CLASS_ID_PROVISION_DRIVE:
            fbe_provision_drive_metadata_memory_dump_debug_trace(module_name, data_ptr,trace_func,trace_context,spaces_to_indent);	
            break;
        case FBE_CLASS_ID_LUN:
            fbe_lun_object_metadata_memory_dump_debug_trace(module_name, data_ptr,trace_func,trace_context,spaces_to_indent);	
            break;
        default:	
            /* There is no handling for this class yet.
              * Display only class id for it. 
              */
            trace_func(trace_context, "CLASS ID: %d", fbe_dump_class_id);			 
         
    }

    return FBE_STATUS_OK;	
}

/*!*******************************************************************
 * @var fbe_metadata_memory_dump_field_info
 *********************************************************************
 * @brief Information about each of the fields of metadata memory.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_metadata_memory_dump_field_info[] =
{
    FBE_DEBUG_DECLARE_FIELD_INFO("memory_ptr", fbe_u64_t, FBE_TRUE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("memory_size", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};


/*!*******************************************************************
 * @var fbe_metadata_memory_dump_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        metadata memory.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_metadata_memory_t,
                              fbe_metadata_memory_dump_struct_info,
                              &fbe_metadata_memory_dump_field_info[0]);

/*!**************************************************************
 * fbe_metadata_memory_dump_debug_trace()
 ****************************************************************
 * @brief
 *  Display the clustered memory(metadata memory and metadata_memory_peer) information.
 *
 * @param module_name - Module name to use in symbol lookup.
 * @param object_ptr - ptr to value to display.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param field_info_p - Field to init. 
 * @param spaces_to_indent - Number of spaces to indent the next line(s).
 * 
 * @return fbe_status_t   
 *
 * @author
 *  8/15/2012 - Created. Jingcheng Zhang
 *
 ****************************************************************/ 
fbe_status_t fbe_metadata_memory_dump_debug_trace(const fbe_u8_t * module_name,
                                             fbe_dbgext_ptr object_ptr,
                                             fbe_trace_func_t trace_func,
                                             fbe_trace_context_t trace_context,
                                             fbe_debug_field_info_t *field_info_p,
                                             fbe_u32_t spaces_to_indent)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t metadata_memory_size;
    fbe_dump_debug_trace_prefix_t new_prefix;
        
    FBE_GET_TYPE_SIZE(module_name, fbe_metadata_memory_t, &metadata_memory_size);
    if (field_info_p->size != metadata_memory_size)
    {
        trace_func(NULL, "%s size is %d not %d\n",
                   field_info_p->name, field_info_p->size, metadata_memory_size);
        return FBE_STATUS_GENERIC_FAILURE; 
    }
    fbe_debug_dump_generate_prefix(&new_prefix, (fbe_dump_debug_trace_prefix_t *)trace_context,
                                   field_info_p->name);


    fbe_debug_display_structure(module_name, trace_func, &new_prefix, object_ptr + field_info_p->offset,
                                &fbe_metadata_memory_dump_struct_info,
                                1 /* fields per line */,
                                0 /*spaces_to_indent */);

    trace_func(NULL, "\n");
    status = fbe_object_metadata_memory_dump_debug_trace(module_name,object_ptr + field_info_p->offset,
                                                    trace_func, &new_prefix, 0);
        
    return status;	
}


/*!*******************************************************************
 * @var fbe_metadata_element_dump_field_info
 *********************************************************************
 * @brief Information about each of the fields of base config.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_metadata_element_dump_field_info[] =
{
    FBE_DEBUG_DECLARE_FIELD_INFO("paged_record_start_lba", fbe_lba_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("paged_mirror_metadata_offset", fbe_lba_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("metadata_element_state", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("paged_record_request_count", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("attributes", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("peer_metadata_element_ptr", fbe_u64_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("stripe_lock_number_of_stripes", fbe_u64_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("stripe_lock_number_of_private_stripes", fbe_u64_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("stripe_lock_count", fbe_u64_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("local_collision_count", fbe_u64_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("peer_collision_count", fbe_u64_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("cmi_message_count", fbe_u64_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("attributes", fbe_u32_t, FBE_FALSE, "0x%x"),

    FBE_DEBUG_DECLARE_FIELD_INFO_FN("nonpaged_record", fbe_metadata_record_t, FBE_FALSE, "0x%x", 
                                    fbe_non_paged_record_metadata_dump_debug_trace), 
      
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("metadata_memory", fbe_metadata_memory_t, FBE_FALSE, "0x%x", 
                                    fbe_metadata_memory_dump_debug_trace), 
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("metadata_memory_peer", fbe_metadata_memory_t, FBE_FALSE, "0x%x", 
                                    fbe_metadata_memory_dump_debug_trace), 
                                    
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_metadata_element_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        base config.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_metadata_element_t,
                              fbe_metadata_element_dump_struct_info,
                              &fbe_metadata_element_dump_field_info[0]);

/*!**************************************************************
 * fbe_metadata_element_dump_debug_trace()
 ****************************************************************
 * @brief
 *  Display info on metadata element.
 *  
 * @param module_name - Module name to use in symbol lookup.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param spaces_to_indent - Spaces to indent this display.
 * @param field_info_p - Info for getting and displaying the field. 
 * @param spaces_to_indent - Number of spaces to indent the next line(s).
 * 
 * @return fbe_status_t
 *
 * @author
 *  8/15/2012 - Created. Jingcheng Zhang
 *
 ****************************************************************/
fbe_status_t fbe_metadata_element_dump_debug_trace(const fbe_u8_t * module_name,
                                              fbe_dbgext_ptr object_ptr,
                                              fbe_trace_func_t trace_func,
                                              fbe_trace_context_t trace_context,
                                              fbe_debug_field_info_t *field_info_p,
                                              fbe_u32_t spaces_to_indent)
{
    fbe_status_t status;
    fbe_dump_debug_trace_prefix_t new_prefix;

    fbe_debug_dump_generate_prefix(&new_prefix, (fbe_dump_debug_trace_prefix_t *)trace_context,
                                   field_info_p->name);

    status = fbe_debug_display_structure(module_name, trace_func, &new_prefix, 
                                         object_ptr + field_info_p->offset,
                                         &fbe_metadata_element_dump_struct_info,
                                         1 /* fields per line */,
                                         0 /*spaces_to_indent*/);

    return status;
}



/*!*******************************************************************
 * @var fbe_base_config_signature_dump_field_info
 *********************************************************************
 * @brief Information about each of the fields of base config signature.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_base_config_signature_dump_field_info[] =
{
    FBE_DEBUG_DECLARE_FIELD_INFO("signature_crc", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("object_id", fbe_object_id_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("class_id", fbe_class_id_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_base_config_signature_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        base signature data.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_base_config_signature_data_t,
                              fbe_base_config_signature_dump_info,
                              &fbe_base_config_signature_dump_field_info[0]);

/*!**************************************************************
 * fbe_base_config_signature_dump_debug_trace()
 ****************************************************************
 * @brief
 *  Display all the trace information about the object signature.
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
 *  8/15/2012 - Created. Jingcheng Zhang
 *
 ****************************************************************/
fbe_status_t fbe_base_config_signature_dump_debug_trace(const fbe_u8_t * module_name,
                                                   fbe_dbgext_ptr base_p,
                                                   fbe_trace_func_t trace_func,
                                                   fbe_trace_context_t trace_context,
                                                   fbe_debug_field_info_t *field_info_p,
                                                   fbe_u32_t spaces_to_indent)
{
    fbe_status_t status;
    fbe_dump_debug_trace_prefix_t new_prefix;

    fbe_debug_dump_generate_prefix(&new_prefix, (fbe_dump_debug_trace_prefix_t *)trace_context,
                                   field_info_p->name);

    status = fbe_debug_display_structure(module_name, trace_func, &new_prefix, 
                                         base_p + field_info_p->offset,
                                         &fbe_base_config_signature_dump_info,
                                         1 /* fields per line */,
                                         0);
    return status;
}


/******************************************
 * end fbe_metadata_element_debug_trace()
 ******************************************/
/*!*******************************************************************
 * @def FBE_BASE_CONFIG_WIDTH_FIELD_INFO_INDEX
 *********************************************************************
 * @brief Index of the width in the field info array.
 *
 *********************************************************************/
#define FBE_BASE_CONFIG_WIDTH_FIELD_INFO_INDEX 1
/*!*******************************************************************
 * @var fbe_base_config_dump_field_info
 *********************************************************************
 * @brief Information about each of the fields of base config.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_base_config_dump_field_info[] =
{
    /* base object is displayed first.
     */
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("base_object", fbe_u32_t, FBE_TRUE, "0x%x",
                                    fbe_base_object_dump_debug_trace),
    /*FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),*/
    /* IMPORTANT: If the index of the width changes please update the above. 
     *            FBE_BASE_CONFIG_WIDTH_FIELD_INFO_INDEX 
     */
    FBE_DEBUG_DECLARE_FIELD_INFO("width", fbe_object_id_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("power_saving_idle_time_in_seconds", fbe_u64_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("background_operation_deny_count", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("hibernation_time", fbe_time_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("generation_number", fbe_config_generation_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("flags", fbe_base_config_flags_t, FBE_FALSE, "0x%x",
                                    fbe_base_config_flag_dump_debug_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("block_edge_p", fbe_block_edge_t, FBE_FALSE, "0x%x", 
                                    fbe_debug_display_pointer),
    
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("metadata_element", fbe_metadata_element_t, FBE_FALSE, "0x%x", 
                                    fbe_metadata_element_dump_debug_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_base_config_dump_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        base config.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_base_config_t,
                              fbe_base_config_dump_struct_info,
                              &fbe_base_config_dump_field_info[0]);


/*!**************************************************************
 * fbe_base_config_dump_debug_trace()
 ****************************************************************
 * @brief
 *  Display all the trace information about the raid group object.
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
 *  8/15/2012 - Created. Jingcheng Zhang
 *
 ****************************************************************/
fbe_status_t fbe_base_config_dump_debug_trace(const fbe_u8_t * module_name,
                                         fbe_dbgext_ptr base_p,
                                         fbe_trace_func_t trace_func,
                                         fbe_trace_context_t trace_context,
                                         fbe_debug_field_info_t *field_info_p,
                                         fbe_u32_t overall_spaces_to_indent)
{
    fbe_status_t status;
    fbe_u32_t transport_server_offset;
    fbe_u64_t block_edge_p = 0;
    fbe_u32_t ptr_size = 0;
    fbe_u32_t block_edge_size = 0;
    fbe_u32_t edge_index = 0;
    fbe_u32_t width;
    fbe_u32_t block_edge_ptr_offset;
    fbe_u64_t base_config_p = base_p + field_info_p->offset;
    fbe_base_config_t base_config ={0};
    fbe_dump_debug_trace_prefix_t new_prefix;
    fbe_dump_debug_trace_prefix_t edge_prefix;
    char str_edge_prefix[128];

    /* Display the structure information.
     */
    FBE_READ_MEMORY(base_config_p , &base_config, sizeof(fbe_base_config_t));
    fbe_dump_class_id = base_config.base_object.class_id;
    fbe_debug_dump_generate_prefix(&new_prefix, (fbe_dump_debug_trace_prefix_t *)trace_context,
                                   field_info_p->name);

    status = fbe_debug_display_structure(module_name, trace_func, &new_prefix, base_config_p,
                                         &fbe_base_config_dump_struct_info,
                                         1 /* fields per line */,
                                         0 /*spaces_to_indent*/);


    if (status != FBE_STATUS_OK) 
    { 
        return status; 
    }

    /*dump the signature details according to object type*/
    trace_func(NULL, "\n");
/*
    if(base_config.base_object.class_id == FBE_CLASS_ID_PROVISION_DRIVE)
    {
        trace_func(&new_prefix, "u.serial_num: %s\n",base_config.u.serial_num);
    }
    else
*/
    {
        trace_func(&new_prefix, "u.wwn: 0x%016llx\n",base_config.u.wwn);
    }
	
    /* Display the Block Edge information.
     */
    fbe_debug_get_ptr_size(module_name, &ptr_size);
    FBE_GET_FIELD_OFFSET(module_name, fbe_base_config_t, "block_edge_p", &block_edge_ptr_offset);
    FBE_READ_MEMORY(base_config_p + block_edge_ptr_offset, &block_edge_p, ptr_size);
    FBE_GET_TYPE_SIZE(module_name, fbe_block_edge_t, &block_edge_size);

    FBE_READ_MEMORY(base_config_p + fbe_base_config_dump_field_info[FBE_BASE_CONFIG_WIDTH_FIELD_INFO_INDEX].offset,
                    &width, sizeof(fbe_u32_t));
    for ( edge_index = 0; edge_index < width; edge_index++)
    {
        if (FBE_CHECK_CONTROL_C())
        {
            return FBE_STATUS_CANCELED;
        }
        _snprintf(str_edge_prefix, 127, "%s.block_edge[%d]", field_info_p->name, edge_index);
        fbe_debug_dump_generate_prefix(&edge_prefix, (fbe_dump_debug_trace_prefix_t *)trace_context,
                                   str_edge_prefix);
        fbe_block_transport_dump_edge(module_name,
                                      block_edge_p + (block_edge_size * edge_index),
                                      trace_func,
                                      &edge_prefix,
                                      0 /* spaces to indent */);

    }

    FBE_GET_FIELD_OFFSET(module_name,
                         fbe_base_config_t,
                         "block_transport_server",
                         &transport_server_offset);


    fbe_block_transport_dump_server_detail(module_name,
                                           base_config_p + transport_server_offset,
                                           trace_func,
                                           &new_prefix,
                                           0);


    fbe_dump_class_id = FBE_CLASS_ID_INVALID;
    	
    return FBE_STATUS_OK;
} 
/**************************************
 * end fbe_base_config_dump_debug_trace()
 **************************************/

/*!**************************************************************
 * fbe_base_config_flag_dump_debug_trace()
 ****************************************************************
 * @brief
 *  Display the base config flag.
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
 *  8/15/2012 - Created. Jingcheng Zhang
 *
 ****************************************************************/
fbe_status_t fbe_base_config_flag_dump_debug_trace(const fbe_u8_t * module_name,
                                              fbe_dbgext_ptr base_ptr,
                                              fbe_trace_func_t trace_func,
                                              fbe_trace_context_t trace_context,
                                              fbe_debug_field_info_t *field_info_p,
                                              fbe_u32_t spaces_to_indent)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_base_config_flags_t flags;
    if (field_info_p->size != sizeof(fbe_base_config_flags_t))
    {
        trace_func(NULL, "%s size is %d not %d\n",
                   field_info_p->name, field_info_p->size, (int)sizeof(fbe_base_config_flags_t));
        return FBE_STATUS_GENERIC_FAILURE; 
    }
    FBE_READ_MEMORY(base_ptr + field_info_p->offset, &flags, field_info_p->size);

    trace_func(trace_context, "%s: 0x%x ", field_info_p->name, (unsigned int)flags);
    fbe_base_config_dump_flag_string_trace(flags, trace_func, NULL);

    return status;
}
/**************************************
 * end fbe_base_config_flag_dump_debug_trace
 **************************************/

/*!***************************************************************************
 * fbe_base_config_dump_flag_string_trace()
 ******************************************************************************
 * @brief
 *  Display's the base config flag string
 *
 * @param  flags - config flag to be converted to string format
  *@param trace_func - Function to use when tracing.
 * @param trace_context - The context to be passed to the trace function.
 *
 * @return - FBE_STATUS_OK 
 *
 * @author
 *  8/15/2012 - Created. Jingcheng Zhang
 *
 ****************************************************************/
fbe_status_t fbe_base_config_dump_flag_string_trace(fbe_base_config_flags_t flags, 
                                               fbe_trace_func_t trace_func, 
                                               fbe_trace_context_t trace_context)
{
    switch(flags)
    {
        case FBE_BASE_CONFIG_FLAG_POWER_SAVING_ENABLED:
            trace_func(NULL, "POWER_SAVING_ENABLED");
            break;
        case FBE_BASE_CONFIG_FLAG_CONFIGURATION_COMMITED:
            trace_func(NULL, "CONFIGURATION_COMMITED");
            break;
        case FBE_BASE_CONFIG_FLAG_USER_INIT_QUIESCE:
            trace_func(trace_context, "USER_QUIESCE");
            break;
        case FBE_BASE_CONFIG_FLAG_DENY_OPERATION_PERMISSION:
            trace_func(trace_context, "DENY_PERMISSION");
            break;
        case FBE_BASE_CONFIG_FLAG_PERM_DESTROY:
            trace_func(trace_context, "PERM_DESTROY");
            break;
        case FBE_BASE_CONFIG_FLAG_RESPECIALIZE:
            trace_func(trace_context, "RESPECIALIZE");
            break;
        case FBE_BASE_CONFIG_FLAG_INITIAL_CONFIGURATION:
            trace_func(trace_context, "INITIAL_CONFIG");
            break;
        default:
            trace_func(NULL, "UNKNOWN BASE CONFIG FLAG %d", (int)flags);
        break;
    }

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_base_config_dump_flag_string_trace()
 ******************************************/

/*!*******************************************************************
 * @var fbe_base_config_metadata_memory_dump_field_info
 *********************************************************************
 * @brief Information about each of the fields of lun object
 *        metadata memeory.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_base_config_metadata_memory_dump_field_info[] =
{
    FBE_DEBUG_DECLARE_FIELD_INFO("lifecycle_state", fbe_lifecycle_state_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("power_save_state", fbe_power_save_state_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("last_io_time", fbe_time_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("flags", fbe_base_config_clustered_flags_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_base_config_metadata_memory_dump_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        lun object metadata memory.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_base_config_metadata_memory_t,
                              fbe_base_config_metadata_memory_dump_struct_info,
                              &fbe_base_config_metadata_memory_dump_field_info[0]);

/*!**************************************************************
 * fbe_base_config_metadata_memory_dump_debug_trace()
 ****************************************************************
 * @brief
 *  Display the base config object metadata memeory.
 *
 * @param module_name - Module name to use in symbol lookup.
 * @param record_ptr - ptr to value to display.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param spaces_to_indent - Number of spaces to indent the next line(s).
 * 
 * @return fbe_status_t   
 *
 * @author
 *  8/15/2012 - Created. Jingcheng Zhang
 *
 ****************************************************************/
fbe_status_t fbe_base_config_metadata_memory_dump_debug_trace(const fbe_u8_t * module_name,
                                                         fbe_dbgext_ptr record_ptr,
                                                         fbe_trace_func_t trace_func,
                                                         fbe_trace_context_t trace_context,
                                                         fbe_debug_field_info_t *field_info_p,
                                                         fbe_u32_t spaces_to_indent)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_dump_debug_trace_prefix_t new_prefix;

    fbe_debug_dump_generate_prefix(&new_prefix, (fbe_dump_debug_trace_prefix_t *)trace_context,
                                   "base_config_metadata_memory"); 
    status = fbe_debug_display_structure(module_name, trace_func, &new_prefix, record_ptr + field_info_p->offset,
                                         &fbe_base_config_metadata_memory_dump_struct_info,
                                         1    /* fields per line */,
                                         0);
    return status;
}


/**************************************
 * end fbe_base_config_metadata_memory_dump_debug_trace
 **************************************/
/*************************
 * end file fbe_base_config_dump_debug.c
 *************************/
