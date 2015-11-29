/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_provision_drive_debug.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the debug functions for the provision drive object.
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

/*!**************************************************************
 * fbe_metadata_operation_pvd_paged_metadata_debug_trace()
 ****************************************************************
 * @brief
 *  Display the PVD paged metadata.
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
 *  03/14/2011 - Created Omer Miranda
 *
 ****************************************************************/
fbe_status_t fbe_metadata_operation_pvd_paged_metadata_debug_trace(const fbe_u8_t * module_name,
                                                                   fbe_dbgext_ptr paged_metadata_ptr,
                                                                   fbe_trace_func_t trace_func,
                                                                   fbe_trace_context_t trace_context,
                                                                   fbe_u32_t spaces_to_indent)
{
    fbe_provision_drive_paged_metadata_t provision_drive_paged_metadata = {0};

	fbe_trace_indent(trace_func, trace_context,spaces_to_indent);
    trace_func(trace_context, "provision_drive_paged_metadata: \n");
    
    FBE_READ_MEMORY(paged_metadata_ptr,&provision_drive_paged_metadata,sizeof(fbe_provision_drive_paged_metadata_t));
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent + 2);
    trace_func(trace_context, "need_zero_bit: %d\n", provision_drive_paged_metadata.need_zero_bit);
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent + 2);
    trace_func(trace_context, "user_zero_bit: %d\n", provision_drive_paged_metadata.user_zero_bit);
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent + 2);
    trace_func(trace_context, "consumed_user_bit: %d\n", provision_drive_paged_metadata.consumed_user_data_bit);

    return FBE_STATUS_OK;
}

/*!*******************************************************************
 * @var fbe_provision_drive_metadata_memory_field_info
 *********************************************************************
 * @brief Information about each of the fields of provision drive
 *        metadata memeory.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_provision_drive_metadata_memory_field_info[] =
{
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("base_config_metadata_memory", fbe_base_config_metadata_memory_t, FBE_TRUE, "0x%x",
                                    fbe_base_config_metadata_memory_debug_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO("flags", fbe_provision_drive_clustered_flags_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_provision_drive_metadata_memory_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        provision drive metadata memory.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_provision_drive_metadata_memory_t,
                              fbe_provision_drive_metadata_memory_struct_info,
                              &fbe_provision_drive_metadata_memory_field_info[0]);

/*!**************************************************************
 * fbe_provision_drive_metadata_memory_debug_trace()
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
 *  01/31/2011 - Created Omer Miranda
 *
 ****************************************************************/
fbe_status_t fbe_provision_drive_metadata_memory_debug_trace(const fbe_u8_t * module_name,
                                                             fbe_dbgext_ptr record_ptr,
                                                             fbe_trace_func_t trace_func,
                                                             fbe_trace_context_t trace_context,
                                                             fbe_u32_t spaces_to_indent)
{
    fbe_status_t status = FBE_STATUS_OK;

	trace_func(trace_context, "\n");

	fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    trace_func(trace_context, "provision_drive_metadata_memory: \n");
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent + 4);
    status = fbe_debug_display_structure(module_name, trace_func, trace_context, record_ptr,
                                         &fbe_provision_drive_metadata_memory_struct_info,
                                         4 /* fields per line */,
                                         spaces_to_indent + 4);
    return FBE_STATUS_OK;
}
/*!*******************************************************************
 * @var fbe_provision_drive_nonpaged_metadata_field_info
 *********************************************************************
 * @brief Information about each of the fields of provision drive
 *        non-paged metadata record.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_provision_drive_nonpaged_metadata_field_info[] =
{
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("base_config_nonpaged_metadata", fbe_base_config_nonpaged_metadata_t, FBE_TRUE, "0x%x", 
                                    fbe_base_config_nonpaged_metadata_debug_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO("sniff_verify_checkpoint", fbe_lba_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("zero_checkpoint", fbe_lba_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("zero_on_demand", fbe_bool_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("end_of_life_state", fbe_bool_t, FBE_FALSE, "0x%x"),
	FBE_DEBUG_DECLARE_FIELD_INFO("media_error_lba", fbe_lba_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("verify_invalidate_checkpoint", fbe_lba_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("drive_fault_state", fbe_bool_t, FBE_FALSE, "0x%x"),
	FBE_DEBUG_DECLARE_FIELD_INFO_FN("nonpaged_drive_info", fbe_provision_drive_nonpaged_metadata_drive_info_t, FBE_FALSE, "0x%x", 
                                    fbe_provision_drive_nonpaged_drive_info_debug_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO("flags", fbe_provision_drive_np_flags_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("validate_area_bitmap", fbe_u32_t, FBE_FALSE, "0x%x"),	
    FBE_DEBUG_DECLARE_FIELD_INFO("pass_count", fbe_u32_t, FBE_FALSE, "0x%x"),	
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_provision_drive_nonpaged_metadata_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        provision drive non paged metadata record.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_provision_drive_nonpaged_metadata_t,
                              fbe_provision_drive_nonpaged_metadata_struct_info,
                              &fbe_provision_drive_nonpaged_metadata_field_info[0]);

/*!*******************************************************************
 * @var fbe_provision_drive_nonpaged_drive_field_info
 *********************************************************************
 * @brief Information about each of the fields of provision drive
 *        non-paged drive info.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_provision_drive_nonpaged_drive_field_info[] =
{
    FBE_DEBUG_DECLARE_FIELD_INFO("port_number", fbe_u32_t, FBE_TRUE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("enclosure_number", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("slot_number", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("drive_type", fbe_drive_type_t, FBE_FALSE, "0x%x",
	                                fbe_provision_drive_type_debug_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_provision_drive_nonpaged_drive_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *       fbe_provision_drive_nonpaged_metadata_drive_info_t.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_provision_drive_nonpaged_metadata_drive_info_t,
                              fbe_provision_drive_nonpaged_drive_struct_info,
                              &fbe_provision_drive_nonpaged_drive_field_info[0]);

/*!**************************************************************
 * fbe_provision_drive_nonpaged_drive_info_debug_trace()
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
 *  11/02/2011 - Created Omer Miranda
 *
 ****************************************************************/
fbe_status_t fbe_provision_drive_nonpaged_drive_info_debug_trace(const fbe_u8_t * module_name,
                                                                 fbe_dbgext_ptr drive_info_ptr,
                                                                 fbe_trace_func_t trace_func,
                                                                 fbe_trace_context_t trace_context,
                                                                 fbe_debug_field_info_t *field_info_p,
                                                                 fbe_u32_t spaces_to_indent)
{
	fbe_status_t status = FBE_STATUS_OK;

	trace_func(trace_context, "\n");
	fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    trace_func(trace_context, "%s: ", field_info_p->name);
    status = fbe_debug_display_structure(module_name, trace_func, trace_context,
		                                 drive_info_ptr + field_info_p->offset,
                                         &fbe_provision_drive_nonpaged_drive_struct_info,
                                         4 /* fields per line */,
                                         spaces_to_indent +4);
	return status;
}
/*******************************************************
 * end fbe_provision_drive_nonpaged_drive_info_debug_trace()
 ******************************************************/
/*!**************************************************************
 * fbe_provision_drive_type_debug_trace()
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
fbe_status_t fbe_provision_drive_type_debug_trace(const fbe_u8_t * module_name,
                                                  fbe_dbgext_ptr drive_type_ptr,
                                                  fbe_trace_func_t trace_func,
                                                  fbe_trace_context_t trace_context,
                                                  fbe_debug_field_info_t *field_info_p,
                                                  fbe_u32_t spaces_to_indent)
{
	fbe_status_t status = FBE_STATUS_OK;
	fbe_drive_type_t drive_type;

    trace_func(trace_context, "%s: ", field_info_p->name);
	FBE_READ_MEMORY(drive_type_ptr + field_info_p->offset, &drive_type, sizeof(fbe_drive_type_t));

	switch(drive_type)
	{
		case FBE_DRIVE_TYPE_INVALID:
			trace_func(trace_context, "%s", "INVALID");
			break;
		case FBE_DRIVE_TYPE_FIBRE:
			trace_func(trace_context, "%s", "FIBRE");
			break;
		case FBE_DRIVE_TYPE_SAS:
			trace_func(trace_context, "%s", "SAS");
			break;
		case FBE_DRIVE_TYPE_SATA:
			trace_func(trace_context, "%s", "SATA");
			break;
		case FBE_DRIVE_TYPE_SAS_FLASH_HE:
			trace_func(trace_context, "%s", "SAS_FLASH_HE");
			break;
		case FBE_DRIVE_TYPE_SATA_FLASH_HE:
			trace_func(trace_context, "%s", "SATA_FLASH HE");
			break;
		case FBE_DRIVE_TYPE_SAS_NL:
			trace_func(trace_context, "%s", "SAS_NL");
			break;
		case FBE_DRIVE_TYPE_SATA_PADDLECARD:
			trace_func(trace_context, "%s", "SATA_PADDLECARD");
			break;
		case FBE_DRIVE_TYPE_SAS_FLASH_ME:
			trace_func(trace_context, "%s", "SAS_FLASH_ME");
			break;            
        case FBE_DRIVE_TYPE_SAS_FLASH_LE:
            trace_func(trace_context, "%s", "SAS_FLASH_LE");
            break;            
        case FBE_DRIVE_TYPE_SAS_FLASH_RI:
            trace_func(trace_context, "%s", "SAS_FLASH_RI");
            break;            
		default:
			trace_func(trace_context, "%s", "UNKNOWN");
	}

	return status;
}
/*******************************************************
 * end fbe_provision_drive_type_debug_trace()
 ******************************************************/

/*!**************************************************************
 * fbe_provision_drive_non_paged_record_debug_trace()
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
 *  01/28/2011 - Created Omer Miranda
 *
 ****************************************************************/
fbe_status_t fbe_provision_drive_non_paged_record_debug_trace(const fbe_u8_t * module_name,
                                                              fbe_dbgext_ptr record_ptr,
                                                              fbe_trace_func_t trace_func,
                                                              fbe_trace_context_t trace_context,
                                                              fbe_u32_t spaces_to_indent)
{
    fbe_status_t status = FBE_STATUS_OK;
	fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    trace_func(trace_context, "provision_drive_non_paged_metadata: \n");
    status = fbe_debug_display_structure(module_name, trace_func, trace_context, record_ptr,
                                         &fbe_provision_drive_nonpaged_metadata_struct_info,
                                         4 /* fields per line */,
                                         spaces_to_indent +4);
    trace_func(trace_context, "\n");
    return FBE_STATUS_OK;
}
/*******************************************************
 * end fbe_provision_drive_non_paged_record_debug_trace()
 ******************************************************/

/*!*******************************************************************
 * @var fbe_provision_drive_field_info
 *********************************************************************
 * @brief Information about each of the fields of base config.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_provision_drive_field_info[] =
{
    /* base config is first.
     */
    FBE_DEBUG_DECLARE_FIELD_INFO("debug_flags", fbe_provision_drive_debug_flags_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("config_type", fbe_provision_drive_config_type_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("local_state", fbe_provision_drive_local_state_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("flags", fbe_provision_drive_flags_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("base_config", fbe_object_id_t, FBE_TRUE, "0x%x",
                                    fbe_base_config_debug_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_provision_drive_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        base config.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_provision_drive_t,
                              fbe_provision_drive_struct_info,
                              &fbe_provision_drive_field_info[0]);

char * fbe_convert_config_type_to_string(fbe_provision_drive_config_type_t config_type);

/*!**************************************************************
 * @fn fbe_provision_drive_debug_trace(const fbe_u8_t * module_name,
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
 *  04/26/2010 - Created. RPF
 *  04/26/2010 - Modified to display provision drive detail. Prafull Parab
 *
 ****************************************************************/

fbe_status_t fbe_provision_drive_debug_trace(const fbe_u8_t * module_name,
                                             fbe_dbgext_ptr provision_drive_p,
                                             fbe_trace_func_t trace_func,
                                             fbe_trace_context_t trace_context,
                                             fbe_u32_t overall_spaces_to_indent)
{
    fbe_status_t status;
    
    status = fbe_debug_display_structure(module_name, trace_func, trace_context, provision_drive_p,
                                         &fbe_provision_drive_struct_info,
                                         4 /* fields per line */,
                                         6 /*spaces_to_indent*/);
    trace_func(trace_context, "\n");
    return status;
} 
/**************************************
 * end fbe_provision_drive_debug_trace()
 **************************************/

char * fbe_convert_config_type_to_string(fbe_provision_drive_config_type_t config_type)
{
    char *config_type_String;

    switch(config_type)
    {
        case FBE_PROVISION_DRIVE_CONFIG_TYPE_INVALID:
            config_type_String = "FBE_PROVISION_DRIVE_CONFIG_TYPE_INVALID";
        break;

        case FBE_PROVISION_DRIVE_CONFIG_TYPE_UNCONSUMED:
            config_type_String = "FBE_PROVISION_DRIVE_CONFIG_TYPE_UNCONSUMED";
            break;

        case FBE_PROVISION_DRIVE_CONFIG_TYPE_TEST_RESERVED_SPARE:
            config_type_String = "FBE_PROVISION_DRIVE_CONFIG_TYPE_TEST_RESERVED_SPARE";
            break;

        case FBE_PROVISION_DRIVE_CONFIG_TYPE_RAID:
            config_type_String = "FBE_PROVISION_DRIVE_CONFIG_TYPE_RAID";
        break;

    default :
        config_type_String = "Unknown config_type";
        break;
    }

    return(config_type_String);
}

/*!**************************************************************
 * fbe_provision_drive_debug_display_terminator_queue()
 ****************************************************************
 * @brief
 *  Display the provision drive  terminator queue.
 *  
 * @param module_name - Module name to use in symbol lookup.
 * @param raid_group_p - ptr to provision drive to display.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param spaces_to_indent - Spaces to indent this display.
 * 
 * @return fbe_status_t
 *
 * @author
 *  10/23/2009 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_provision_drive_debug_display_terminator_queue(const fbe_u8_t * module_name,
                                                                  fbe_dbgext_ptr provision_drive_p,
                                                                  fbe_trace_func_t trace_func,
                                                                  fbe_trace_context_t trace_context,
                                                                  fbe_u32_t spaces_to_indent)
{
    fbe_u32_t terminator_queue_offset;
    fbe_u64_t terminator_queue_head_ptr;
    fbe_u64_t queue_head_p = 0;
    fbe_u64_t queue_element_p = 0;
    fbe_u64_t next_queue_element_p = 0;
    fbe_u32_t packet_queue_element_offset = 0;
    fbe_u32_t payload_offset;
    fbe_u32_t iots_offset;
    fbe_u32_t ptr_size;

    FBE_GET_TYPE_SIZE(module_name, fbe_u32_t*, &ptr_size);

    FBE_GET_FIELD_OFFSET(module_name, fbe_packet_t, "payload_union", &payload_offset);
    FBE_GET_FIELD_OFFSET(module_name, fbe_payload_ex_t, "iots", &iots_offset);

    FBE_GET_FIELD_OFFSET(module_name, fbe_packet_t, "queue_element", &packet_queue_element_offset);
    FBE_GET_FIELD_OFFSET(module_name, fbe_base_object_t, "terminator_queue_head", &terminator_queue_offset);
    terminator_queue_head_ptr = provision_drive_p + terminator_queue_offset;

    FBE_READ_MEMORY(provision_drive_p + terminator_queue_offset, &queue_head_p, ptr_size);
    queue_element_p = queue_head_p;

    {
        fbe_u32_t item_count = 0;
        fbe_transport_get_queue_size(module_name,
                                     provision_drive_p + terminator_queue_offset,
                                     trace_func,
                                     trace_context,
                                     &item_count);
        fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
        if (item_count == 0)
        {
            trace_func(trace_context, "terminator queue: EMPTY\n");
        }
        else
        {
            trace_func(trace_context, "terminator queue: (%d items)\n",
                       item_count);
        }
    }

    /* Loop over all entries on the terminator queue, displaying each packet and iots. 
     * We stop when we reach the end (head) or NULL. 
     */
    while ((queue_element_p != terminator_queue_head_ptr) && (queue_element_p != NULL))
    {
        fbe_u64_t packet_p = queue_element_p - packet_queue_element_offset;

        if (FBE_CHECK_CONTROL_C())
        {
            return FBE_STATUS_CANCELED;
        }

        /* Display packet.
         */
        fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
        trace_func(trace_context, "packet: 0x%llx\n",
		   (unsigned long long)packet_p);
        fbe_transport_print_fbe_packet(module_name, packet_p, trace_func, trace_context, spaces_to_indent + 2);

        FBE_READ_MEMORY(queue_element_p, &next_queue_element_p, ptr_size);
        queue_element_p = next_queue_element_p;

   }
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_provision_drive_debug_display_terminator_queue()
 ******************************************/



/*************************
 * end file fbe_provision_drive_debug.c
 *************************/

