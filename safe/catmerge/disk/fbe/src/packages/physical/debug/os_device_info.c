/***************************************************************************
 * Copyright (C) EMC Corporation 2013
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file os_device_info.c
 ***************************************************************************
 *
 * @brief : To get the OS Device Information through Bvd Interface pointer.
 * 
 * @version
 *  July 1, 2013		created: Ankur Srivastava
 * 
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "pp_dbgext.h"
#include "fbe/fbe_topology_interface.h"
#include "fbe_base_object_debug.h"
#include "fbe/fbe_class.h"
#include "fbe_topology_debug.h"
#include "pp_ext.h"
#include "fbe/fbe_bvd_interface.h"
#include "fbe_transport_debug.h"
#include "fbe_transport_trace.h"
#include "fbe_block_transport_trace.h"

#define MAX_ENTRIES 1000
const fbe_u8_t * os_dev_module_name = "sep"; 


/*************************
 *   FUNCTION DECLERATIONs
 *************************/
void display_os_device_info_table_header(void);
fbe_u8_t * os_dev_get_ready_state_string(fbe_path_state_t path_state_info_arg);
fbe_u8_t * os_dev_get_tri_state_string(FBE_TRI_STATE ready_state_info_arg);
void os_dev_info_read_display(fbe_u64_t current_temp, fbe_bool_t table_format);
void os_dev_info_get_bvd_ptr_display_info(fbe_u32_t 	ptr_size, fbe_bool_t lun_info_display_flag, fbe_bool_t table_format, fbe_u64_t lun_id_info);


#pragma data_seg ("EXT_HELP$4dev_info")
static char CSX_MAYBE_UNUSED usageMsg_dev_info[] =
"!dev_info <-t> <-l> <bvd_interface ptr/lun_object_id>\n"
" Display all the os_dev_info that are queued in the bvd\n"
" -t - (optional) tabular format\n"
" -l - (optional) when lun obj id is to be entered\n";
#pragma data_seg ()


/*!**************************************************************
 * dev_info()
 ****************************************************************
 * @brief
 *  Debugger command to display os device information.
 *
 * @return 
 *  VOID
 *
 * @author
 *  Ankur Srivastava
 *
 ****************************************************************/
 
CSX_DBG_EXT_DEFINE_CMD(dev_info, "dev_info")
{
    fbe_status_t                            status = FBE_STATUS_INVALID;
    fbe_u64_t                               packet_p = 0;
    fbe_u64_t                               lun_id_info = 0;
    fbe_u32_t                               ptr_size = 0;
    fbe_u32_t 					server_queue_head_offset = 0;
    fbe_u64_t		 			packet_element = 0;
    fbe_u64_t	 				next_element = 0;
    fbe_u64_t 					current_element = 0;
    fbe_u32_t 					element_count = 0;
    fbe_bool_t 					table_format = FBE_FALSE;
    fbe_bool_t 					lun_info_display_flag = FBE_FALSE;
    fbe_dbgext_ptr 				topology_object_table_ptr = 0;
    fbe_dbgext_ptr 				topology_object_tbl_ptr = 0;
    fbe_dbgext_ptr				control_handle_ptr = 0; 


	status = fbe_debug_get_ptr_size(os_dev_module_name, &ptr_size);
	if (FBE_STATUS_OK != status)
	{
		fbe_debug_trace_func(NULL, "%s unable to get ptr size status: %d\n", __FUNCTION__, status);
		return; 
	}

	FBE_GET_EXPRESSION(os_dev_module_name, topology_object_table, &topology_object_table_ptr);
	FBE_READ_MEMORY(topology_object_table_ptr, &topology_object_tbl_ptr, ptr_size);

	if(0 == topology_object_table_ptr)
	{
		return;
	}


	FBE_GET_FIELD_DATA(os_dev_module_name,
							topology_object_tbl_ptr/*topology_object_table_ptr*/,
							topology_object_table_entry_s,
							control_handle,
							sizeof(fbe_dbgext_ptr),
							&control_handle_ptr);

	if (strlen(args) && strncmp(args, "-t", 2) == 0)
	{
		args += FBE_MIN(3, strlen(args));
		table_format = FBE_TRUE;	
	}

	if (strlen(args) && strncmp(args, "-l", 2) == 0)
	{
		args += FBE_MIN(3, strlen(args));
		lun_info_display_flag = FBE_TRUE;	
	}
	

	if (strlen(args) && strncmp(args, "-help", 5) == 0)
	{
		fbe_debug_trace_func(NULL,  "%s\n", usageMsg_dev_info);
		return;
	}

	if(FBE_TRUE == table_format)
	{
		display_os_device_info_table_header();
	}

	if (strlen(args) && (FBE_TRUE!= lun_info_display_flag))
	{
		packet_p = GetArgument64(args, 1);
		
		if(0 == packet_p)
		{
			fbe_debug_trace_func(NULL, "Missing Argument: bvd_interface_ptr\n");
			fbe_debug_trace_func(NULL, "%s\n", usageMsg_dev_info);
			return;
		}

		else if(control_handle_ptr!=packet_p)
		{
			fbe_debug_trace_func(NULL, "Invalid ptr entered: bvd_interface_ptr\n");
			return;
		}

		FBE_GET_FIELD_OFFSET(os_dev_module_name, fbe_bvd_interface_t, "server_queue_head", &server_queue_head_offset);
		packet_element = packet_p + server_queue_head_offset;
		FBE_READ_MEMORY(packet_element, &next_element, ptr_size);

		while ((packet_element != next_element) && (next_element != 0) && (element_count < MAX_ENTRIES))
		{
			if (FBE_CHECK_CONTROL_C())
			{
				return;
			}
			current_element = next_element; 
			os_dev_info_read_display(current_element, table_format);
			FBE_READ_MEMORY(next_element, &next_element, ptr_size);
			element_count++;
		}
		return;
	}
	
	else if (!strlen(args))
	{
		os_dev_info_get_bvd_ptr_display_info(ptr_size,  lun_info_display_flag, table_format, lun_id_info);
		return;
	}

	else
	{
		lun_id_info = GetArgument64(args, 1);	
		os_dev_info_get_bvd_ptr_display_info(ptr_size,  lun_info_display_flag, table_format, lun_id_info);
		return;
	}
}


/*************************
 *   FUNCTION DEFINITIONS
 *************************/

void display_os_device_info_table_header(void)
{
	fbe_debug_trace_func(NULL,"\nOS Device Information\n\n");
	fbe_debug_trace_func(NULL,"%-22s%-24s%-14s%-14s%-13s%-17s%-15s%\n",
							    "  LUN",  "Attrib", "Prev", "Cache",  "Path-St","Canceld", "Ready-St");
	fbe_debug_trace_func(NULL,"%-14s%-17s%-12s%-14s%-14s%\n","   ","Currnt", "Prev", "Path-Attrib", "Evnt-Signld");
	fbe_debug_trace_func(NULL,"--------------------------------------------------------------");
	fbe_debug_trace_func(NULL,"------------------------------------------------------------\n");
	return;
}

fbe_u8_t * os_dev_get_ready_state_string(fbe_path_state_t path_state_info_arg)
{
	fbe_u8_t *ready_state_string = NULL;

	if(FBE_PATH_STATE_INVALID == path_state_info_arg)
		ready_state_string = "INVALID";
	
	else if(FBE_PATH_STATE_ENABLED == path_state_info_arg)
		ready_state_string = "ENABLED";
	
	else if(FBE_PATH_STATE_DISABLED == path_state_info_arg)
		ready_state_string = "DISABLED";
	
	else if(FBE_PATH_STATE_BROKEN == path_state_info_arg)
		ready_state_string = "BROKEN";
	
	else if(FBE_PATH_STATE_SLUMBER == path_state_info_arg)
		ready_state_string = "SLUMBER";
	
	else if(FBE_PATH_STATE_GONE == path_state_info_arg)
		ready_state_string = "GONE";

	return ready_state_string;
}

fbe_u8_t * os_dev_get_tri_state_string(FBE_TRI_STATE ready_state_info_arg)
{
	fbe_u8_t *tri_state_string = NULL;

	if(FBE_TRI_STATE_FALSE == ready_state_info_arg)
		tri_state_string = "FBE_TRI_STATE_FALSE";
	
	else if(FBE_TRI_STATE_TRUE == ready_state_info_arg)
		tri_state_string = "FBE_TRI_STATE_TRUE";
	
	else
		tri_state_string = "FBE_TRI_STATE_INVALID";

	return tri_state_string;
}

void os_dev_info_read_display(fbe_u64_t current_temp, fbe_bool_t table_format)
{
	fbe_u64_t			current_temp_operational = current_temp;
	fbe_u32_t			offset_info = 0;
	fbe_u32_t 			lun_number_info = 0;
	fbe_u32_t 			current_attrib_info = 0;
	fbe_u32_t 			prev_attrib_info = 0;
	fbe_u32_t 			prev_path_attrib_info = 0;
	fbe_u32_t 			cache_event_signld_info = 0;
	fbe_path_state_t		path_state_info = FBE_PATH_STATE_INVALID;
	FBE_TRI_STATE		ready_state_info = FBE_TRI_STATE_INVALID;
	long long 			cancel = 0;
	fbe_u8_t *			tri_state_string = NULL;
	fbe_u8_t *			path_state_string = NULL;

	FBE_GET_FIELD_DATA(os_dev_module_name, 
							current_temp_operational,
							os_device_info_t,
							lun_number,
							sizeof(fbe_u32_t),
							&lun_number_info); 

	FBE_GET_FIELD_DATA(os_dev_module_name, 
							current_temp_operational,
							os_device_info_t,
							current_attributes,
							sizeof(fbe_u32_t),
							&current_attrib_info);

	FBE_GET_FIELD_DATA(os_dev_module_name, 
							current_temp_operational,
							os_device_info_t,
							previous_attributes,
							sizeof(fbe_u32_t),
							&prev_attrib_info); 

	FBE_GET_FIELD_DATA(os_dev_module_name, 
							current_temp_operational,
							os_device_info_t,
							previous_path_attributes,
							sizeof(fbe_u32_t),
							&prev_path_attrib_info);

	FBE_GET_FIELD_DATA(os_dev_module_name, 
							current_temp_operational,
							os_device_info_t,
							cache_event_signaled,
							sizeof(fbe_u32_t),
							&cache_event_signld_info);

	FBE_GET_FIELD_DATA(os_dev_module_name, 
							current_temp_operational,
							os_device_info_t,
							path_state,
							sizeof(fbe_u32_t),
							&path_state_info); 

	FBE_GET_FIELD_DATA(os_dev_module_name, 
							current_temp_operational,
							os_device_info_t,
							cancelled,
							sizeof(long long),
							&cancel);

	FBE_GET_FIELD_DATA(os_dev_module_name, 
							current_temp_operational,
							os_device_info_t,
							ready_state,
							sizeof(fbe_u32_t),
							&ready_state_info); 
	
	path_state_string = os_dev_get_ready_state_string(path_state_info);
	tri_state_string = os_dev_get_tri_state_string(ready_state_info);

	if(FBE_TRUE == table_format)
	{
		fbe_debug_trace_func(NULL, " 0x%-12x", lun_number_info);
		fbe_debug_trace_func(NULL, "0x%-15x", current_attrib_info);
		fbe_debug_trace_func(NULL, "0x%-12x", prev_attrib_info);
		fbe_debug_trace_func(NULL, "0x%-13x", prev_path_attrib_info);
		fbe_debug_trace_func(NULL, "0x%-11x", cache_event_signld_info);
		fbe_debug_trace_func(NULL, "%-16s", path_state_string);
		fbe_debug_trace_func(NULL, "%-8lld", cancel);
		fbe_debug_trace_func(NULL, "%-26s\n", tri_state_string);
	}
	
	else
	{
		FBE_GET_FIELD_OFFSET(os_dev_module_name, os_device_info_t, "block_edge", &offset_info);
		fbe_block_transport_display_edge(os_dev_module_name,
											current_temp_operational+offset_info,
											fbe_debug_trace_func,
											NULL,
											2);
		fbe_debug_trace_func(NULL, "\tlun_number				:0x%-20x\n", lun_number_info);
		fbe_debug_trace_func(NULL, "\tcurrent_attributes			:0x%-20x\n", current_attrib_info);
		fbe_debug_trace_func(NULL, "\tprevious_attributes		:0x%-20x\n", prev_attrib_info);
		fbe_debug_trace_func(NULL, "\tprevious_path_attributes	:0x%-20x\n", prev_path_attrib_info);
		fbe_debug_trace_func(NULL, "\tcache_event_signaled		:0x%-20x\n", cache_event_signld_info);
		fbe_debug_trace_func(NULL, "\tpath_state				:%-20s\n", path_state_string);
		fbe_debug_trace_func(NULL, "\tcancelled				:%-20lld\n", cancel);
		fbe_debug_trace_func(NULL, "\tready_state				:%-20s\n\n\n", tri_state_string);
	}
	return;
}

void os_dev_info_get_bvd_ptr_display_info(fbe_u32_t 	ptr_size, fbe_bool_t lun_info_display_flag, fbe_bool_t table_format, fbe_u64_t lun_id_info)
{
	fbe_dbgext_ptr 				topology_object_table_ptr = 0;
	fbe_dbgext_ptr 				topology_object_tbl_ptr = 0;
	fbe_topology_object_status_t 	object_status = 0;
	fbe_dbgext_ptr				control_handle_ptr = 0;	
	fbe_u32_t 					server_queue_head_offset = 0;
	fbe_u64_t		 			packet_element = 0;
	fbe_u64_t	 				next_element = 0;
	fbe_u32_t 					element_count = 0;
	fbe_u64_t 					current_element = 0;
	fbe_u32_t			 		lun_number_info = 0;
	
	FBE_GET_EXPRESSION(os_dev_module_name, topology_object_table, &topology_object_table_ptr);
	FBE_READ_MEMORY(topology_object_table_ptr, &topology_object_tbl_ptr, ptr_size);

	if(0 == topology_object_table_ptr)
	{
		return;
	}


	FBE_GET_FIELD_DATA(os_dev_module_name,
							topology_object_tbl_ptr/*topology_object_table_ptr*/,
							topology_object_table_entry_s,
							object_status,
							sizeof(fbe_topology_object_status_t),
							&object_status);

	FBE_GET_FIELD_DATA(os_dev_module_name,
							topology_object_tbl_ptr/*topology_object_table_ptr*/,
							topology_object_table_entry_s,
							control_handle,
							sizeof(fbe_dbgext_ptr),
							&control_handle_ptr);

	if(FBE_TOPOLOGY_OBJECT_STATUS_READY == object_status)
	{
		FBE_GET_FIELD_OFFSET(os_dev_module_name, fbe_bvd_interface_t, "server_queue_head", &server_queue_head_offset);
		packet_element = control_handle_ptr + server_queue_head_offset;
		FBE_READ_MEMORY(packet_element, &next_element, ptr_size);

		while ((packet_element != next_element) && (next_element != 0) && (element_count < MAX_ENTRIES))
		{
			if (FBE_CHECK_CONTROL_C())
			{
				return;
			}

			current_element = next_element; 

			if(FBE_TRUE != lun_info_display_flag)
			{

				os_dev_info_read_display(current_element, table_format);
				FBE_READ_MEMORY(next_element, &next_element, ptr_size);
				element_count++;			
			}

			else
			{
				FBE_GET_FIELD_DATA(os_dev_module_name, 
										current_element,
										os_device_info_t,
										lun_number,
										sizeof(fbe_u32_t),
										&lun_number_info); 
				
				if(lun_id_info == lun_number_info)
				{
			              os_dev_info_read_display(current_element, table_format);
					break;
				}
				
				else
				{
					FBE_READ_MEMORY(next_element, &next_element, ptr_size);
					element_count++;
				}
			}
		}
	}	
}

/*End of os_device_info.c*/
