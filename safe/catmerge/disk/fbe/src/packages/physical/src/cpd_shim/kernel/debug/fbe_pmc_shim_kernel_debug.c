/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_base_port_debug.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the debug functions for base port object.
 *
 * @author
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include <stdio.h>
//#include "cpd_generics.h"
//#include "cpd_interface.h"
//#include "flare_sgl.h"
#include "fbe/fbe_types.h"
#include "fbe_pmc_shim.h"
#include "fbe/fbe_queue.h"
#include "fbe_trace.h"
#include "fbe_pmc_shim_private.h"
//#include "pmc_shim_kernel_private.h"
//#include "dbgext.h"
#include "fbe/fbe_dbgext.h"
#include "fbe_pmc_shim_debug.h"
/*************************
 *   FUNCTION DEFINITIONS
 *************************/
fbe_status_t fbe_pmc_shim_debug_trace_port_details(const fbe_u8_t * module_name, 
													fbe_dbgext_ptr pmc_port_ptr, 
													fbe_trace_func_t trace_func, 
													fbe_trace_context_t trace_context
													);
fbe_status_t fbe_pmc_shim_debug_trace_port_device_table(const fbe_u8_t * module_name, 
														fbe_dbgext_ptr pmc_port_device_table_p,
														fbe_trace_func_t trace_func, 
														fbe_trace_context_t trace_context
														);
static fbe_status_t fbe_pmc_shim_debug_trace_device_type(fbe_pmc_shim_discovery_device_type_t device_type,
														fbe_trace_func_t trace_func, 
														fbe_trace_context_t trace_context
														);
static fbe_bool_t fbe_pmc_shim_debug_is_device_type_valid(fbe_pmc_shim_discovery_device_type_t device_type);

/*!**************************************************************
 * @fn fbe_pmc_shim_debug_trace_port_info(const fbe_u8_t * module_name, 
 *                                   fbe_dbgext_ptr logical_drive,
 *                                   fbe_trace_func_t trace_func,
 *                                   fbe_trace_context_t trace_context)
 ****************************************************************
 * @brief
 *  Display all the trace information about the port object.
 *
 * @param  module_name - This is the name of the module we are debugging.
 * @param port_object_p - Ptr to port object.
 * @param trace_func - Function to use when tracing.
 * @param trace_context - The context to be passed to the trace function.
 *
 * @return - FBE_STATUS_OK  
 *
 * HISTORY:
 *
 ****************************************************************/

fbe_status_t fbe_pmc_shim_debug_trace_port_info(const fbe_u8_t * module_name, 
												fbe_dbgext_ptr pmc_port_array_ptr, 
												fbe_trace_func_t trace_func, 
												fbe_trace_context_t trace_context,
												fbe_u32_t io_port_number,
												fbe_u32_t io_portal_number
												)
{
	fbe_u32_t index = 0,pmc_port_size;
	fbe_dbgext_ptr pmc_port_ptr;
	fbe_u32_t local_io_port_number, local_io_portal_number;
    /* Display the pointer to the object.
     */	
	csx_dbg_ext_print("PMC Port Array : 0x%llx \n", (unsigned long long)pmc_port_array_ptr);

	pmc_port_ptr = pmc_port_array_ptr;
	for (index = 0; index < FBE_PMC_SHIM_MAX_PORTS;index++){
		FBE_GET_FIELD_DATA(module_name, 
							pmc_port_ptr,
							pmc_port_s,
							io_port_number,
							sizeof(fbe_u32_t),
							&local_io_port_number);

		FBE_GET_FIELD_DATA(module_name, 
							pmc_port_ptr,
							pmc_port_s,
							io_portal_number,
							sizeof(fbe_u32_t),
							&local_io_portal_number);

		if ((local_io_portal_number == io_portal_number) &&
			(local_io_port_number == io_port_number)){
				/* Display information.*/
				fbe_pmc_shim_debug_trace_port_details(module_name, pmc_port_ptr, 
													trace_func, trace_context);
				break;
			}

		FBE_GET_TYPE_SIZE(module_name, pmc_port_s, &pmc_port_size);
		pmc_port_ptr += pmc_port_size;
	}

	if (FBE_PMC_SHIM_MAX_PORTS == index){
		csx_dbg_ext_print("\t\tIO Port: 0x%x ", io_port_number);
		csx_dbg_ext_print("IO Portal: 0x%x ", io_portal_number);
		csx_dbg_ext_print("Could not locate active port information.\n");
	}
	return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_pmc_shim_debug_trace_port_info(const fbe_u8_t * module_name, 
 *                                   fbe_dbgext_ptr logical_drive,
 *                                   fbe_trace_func_t trace_func,
 *                                   fbe_trace_context_t trace_context)
 ****************************************************************
 * @brief
 *  Display all the trace information about the port object.
 *
 * @param  module_name - This is the name of the module we are debugging.
 * @param port_object_p - Ptr to port object.
 * @param trace_func - Function to use when tracing.
 * @param trace_context - The context to be passed to the trace function.
 *
 * @return - FBE_STATUS_OK  
 *
 * HISTORY:
 *
 ****************************************************************/

fbe_status_t fbe_pmc_shim_debug_trace_port_handle_info(const fbe_u8_t * module_name, 
														fbe_dbgext_ptr pmc_port_array_ptr, 
														fbe_trace_func_t trace_func, 
														fbe_trace_context_t trace_context,
														fbe_u32_t io_port_handle
														)
{
	fbe_u32_t pmc_port_size;
	fbe_dbgext_ptr pmc_port_ptr;	
    /* Display the pointer to the object.
     */	
	csx_dbg_ext_print("PMC Port Array : 0x%llx \n", (unsigned long long)pmc_port_array_ptr);

	FBE_GET_TYPE_SIZE(module_name, pmc_port_s, &pmc_port_size);
	pmc_port_ptr = pmc_port_array_ptr + io_port_handle*pmc_port_size;


	if (io_port_handle >= FBE_PMC_SHIM_MAX_PORTS){
		csx_dbg_ext_print("Could not locate active port information.\n");
	}else {		
		fbe_pmc_shim_debug_trace_port_details(module_name, 
											  pmc_port_ptr, 
											  trace_func, 
											  trace_context);
	}
	return FBE_STATUS_OK;
}


fbe_status_t fbe_pmc_shim_debug_trace_port_details(const fbe_u8_t * module_name, 
													fbe_dbgext_ptr pmc_port_ptr, 
													fbe_trace_func_t trace_func, 
													fbe_trace_context_t trace_context
													)
{
	fbe_dbgext_ptr local_ptr;
	fbe_u32_t local_io_port_number, local_io_portal_number;	
	pmc_shim_port_state_t	port_state;

	csx_dbg_ext_print("PMC Port : 0x%llx \n", (unsigned long long)pmc_port_ptr);
	FBE_GET_FIELD_DATA(module_name, 
						pmc_port_ptr,
						pmc_port_s,
						io_port_number,
						sizeof(fbe_u32_t),
						&local_io_port_number);

	FBE_GET_FIELD_DATA(module_name, 
						pmc_port_ptr,
						pmc_port_s,
						io_portal_number,
						sizeof(fbe_u32_t),
						&local_io_portal_number);
	csx_dbg_ext_print("\t\t Port : %d  Portal : %d\n", local_io_port_number,local_io_portal_number);

	FBE_GET_FIELD_DATA(module_name, 
						pmc_port_ptr,
						pmc_port_s,
						state,
						sizeof(pmc_shim_port_state_t),
						&port_state);
	csx_dbg_ext_print("\t\t Port State : %d\n", port_state);


	FBE_GET_FIELD_DATA(module_name, 
						pmc_port_ptr,
						pmc_port_s,
						miniport_file_object,
						sizeof(fbe_dbgext_ptr),
						&local_ptr);
	csx_dbg_ext_print("\t\t File Object : 0x%llx \n", (unsigned long long)local_ptr);

	FBE_GET_FIELD_DATA(module_name, 
						pmc_port_ptr,
						pmc_port_s,
						miniport_device_object,
						sizeof(fbe_dbgext_ptr),
						&local_ptr);
	csx_dbg_ext_print("\t\t Device Object : 0x%llx \n", (unsigned long long)local_ptr);

	FBE_GET_FIELD_DATA(module_name, 
						pmc_port_ptr,
						pmc_port_s,
						miniport_callback_handle,
						sizeof(fbe_dbgext_ptr),
						&local_ptr);
	csx_dbg_ext_print("\t\t Miniport Callback Handle : 0x%llx \n", (unsigned long long)local_ptr);


	FBE_GET_FIELD_DATA(module_name, 
						pmc_port_ptr,
						pmc_port_s,
						enqueue_pending_dmrb,
						sizeof(fbe_dbgext_ptr),
						&local_ptr);
	csx_dbg_ext_print("\t\t Miniport Pending Function Ptr : 0x%llx \n", (unsigned long long)local_ptr);

	FBE_GET_FIELD_DATA(module_name, 
						pmc_port_ptr,
						pmc_port_s,
						payload_completion_function,
						sizeof(fbe_dbgext_ptr),
						&local_ptr);
	csx_dbg_ext_print("\t\t Payload Completion Fn. : 0x%llx \n", (unsigned long long)local_ptr);

	FBE_GET_FIELD_DATA(module_name, 
						pmc_port_ptr,
						pmc_port_s,
						payload_completion_context,
						sizeof(fbe_dbgext_ptr),
						&local_ptr);
	csx_dbg_ext_print("\t\t Payload Completion Context : 0x%llx \n", (unsigned long long)local_ptr);

	FBE_GET_FIELD_DATA(module_name, 
						pmc_port_ptr,
						pmc_port_s,
						callback_function,
						sizeof(fbe_dbgext_ptr),
						&local_ptr);
	csx_dbg_ext_print("\t\t Callback Function : 0x%llx \n", (unsigned long long)local_ptr);

	FBE_GET_FIELD_DATA(module_name, 
						pmc_port_ptr,
						pmc_port_s,
						callback_context,
						sizeof(fbe_dbgext_ptr),
						&local_ptr);
	csx_dbg_ext_print("\t\t Callback Context : 0x%llx \n", (unsigned long long)local_ptr);

	FBE_GET_FIELD_DATA(module_name, 
						pmc_port_ptr,
						pmc_port_s,
						topology_device_information_table,
						sizeof(fbe_dbgext_ptr),
						&local_ptr);
	csx_dbg_ext_print("\t\t Device Table Ptr : 0x%llx \n", (unsigned long long)local_ptr);


	if (port_state == PMC_SHIM_PORT_STATE_INITIALIZED){
		fbe_pmc_shim_debug_trace_port_device_table(module_name, 
												local_ptr,
												trace_func, 
												trace_context
												);
	}

	return FBE_STATUS_OK;
}

fbe_status_t fbe_pmc_shim_debug_trace_port_device_table(const fbe_u8_t * module_name, 
														fbe_dbgext_ptr pmc_port_device_table_p,
														fbe_trace_func_t trace_func, 
														fbe_trace_context_t trace_context
														)
{
	fbe_dbgext_ptr  device_entry_p;
	fbe_u32_t device_entry_offset = 0;	
	fbe_u32_t index,table_size,device_entry_size;
	fbe_miniport_device_id_t             device_id,parent_device_id;
    fbe_sas_address_t                    device_address;
    fbe_pmc_shim_device_locator_t        device_locator;
    fbe_pmc_shim_discovery_device_type_t device_type;
	fbe_bool_t							 log_out_received;
	fbe_u32_t							 current_gen_number;

	FBE_GET_FIELD_DATA(module_name, 
						pmc_port_device_table_p,
						fbe_pmc_shim_device_table_s,
						device_table_size,
						sizeof(fbe_u32_t),
						&table_size);
	csx_dbg_ext_print("\t Device Table Size: 0x%x \n", table_size);

	FBE_GET_FIELD_OFFSET(module_name,
						fbe_pmc_shim_device_table_t,
						"device_entry",
						&device_entry_offset);

	device_entry_p = pmc_port_device_table_p + device_entry_offset;
	for(index = 0; index < table_size ;index++){
		FBE_GET_FIELD_DATA(module_name, 
							device_entry_p,
							fbe_pmc_shim_device_table_entry_s,
							device_id,
							sizeof(fbe_miniport_device_id_t),
							&device_id);
		FBE_GET_FIELD_DATA(module_name, 
							device_entry_p,
							fbe_pmc_shim_device_table_entry_s,
							device_address,
							sizeof(fbe_sas_address_t),
							&device_address);
		FBE_GET_FIELD_DATA(module_name, 
							device_entry_p,
							fbe_pmc_shim_device_table_entry_s,
							device_locator,
							sizeof(fbe_pmc_shim_device_locator_t),
							&device_locator);
		FBE_GET_FIELD_DATA(module_name, 
							device_entry_p,
							fbe_pmc_shim_device_table_entry_s,
							device_type,
							sizeof(fbe_pmc_shim_discovery_device_type_t),
							&device_type);
		FBE_GET_FIELD_DATA(module_name, 
							device_entry_p,
							fbe_pmc_shim_device_table_entry_s,
							parent_device_id,
							sizeof(fbe_miniport_device_id_t),
							&parent_device_id);
		FBE_GET_FIELD_DATA(module_name, 
							device_entry_p,
							fbe_pmc_shim_device_table_entry_s,
							log_out_received,
							sizeof(fbe_bool_t),
							&log_out_received);
		FBE_GET_FIELD_DATA(module_name, 
							device_entry_p,
							fbe_pmc_shim_device_table_entry_s,
							current_gen_number,
							sizeof(fbe_u32_t),
							&current_gen_number);


		if (fbe_pmc_shim_debug_is_device_type_valid(device_type)){
			csx_dbg_ext_print("\t  Entry : 0x%llx \n", (unsigned long long)device_entry_p);		
			csx_dbg_ext_print("\t\t Device ID : 0x%llx \n", (unsigned long long)device_id);
			csx_dbg_ext_print("\t\t Entry Gen # : 0x%x  Device Logged %s.\n", current_gen_number,(log_out_received ? "OUT" : "IN"));
			csx_dbg_ext_print("\t\t Device Address : 0x%llx \n", (unsigned long long)device_address);
			csx_dbg_ext_print("\t\t Device Locator : Depth: 0x%x Phy #: 0x%x\n", device_locator.enclosure_chain_depth,
						device_locator.phy_number);
			csx_dbg_ext_print("\t\t Device Type : ");
			fbe_pmc_shim_debug_trace_device_type(device_type,trace_func,trace_context);
			csx_dbg_ext_print("\n");
			csx_dbg_ext_print("\t\t Parent Device ID : 0x%llx \n", (unsigned long long)parent_device_id);
		}

		FBE_GET_TYPE_SIZE(module_name, fbe_pmc_shim_device_table_entry_s, &device_entry_size)
		device_entry_p += device_entry_size;
	}

	return FBE_STATUS_OK;
}

static fbe_bool_t fbe_pmc_shim_debug_is_device_type_valid(fbe_pmc_shim_discovery_device_type_t device_type)
{
	fbe_bool_t device_type_valid = FALSE;
	switch(device_type){
		case FBE_PMC_SHIM_DEVICE_TYPE_SSP: 
		case FBE_PMC_SHIM_DEVICE_TYPE_STP: 
		case FBE_PMC_SHIM_DEVICE_TYPE_ENCLOSURE: 
		case FBE_PMC_SHIM_DEVICE_TYPE_VIRTUAL: 
			device_type_valid = TRUE;
			break;
		default:
			device_type_valid = FALSE;
			break;
	};

	return device_type_valid;
}

	
static fbe_status_t fbe_pmc_shim_debug_trace_device_type(fbe_pmc_shim_discovery_device_type_t device_type,
														fbe_trace_func_t trace_func, 
														fbe_trace_context_t trace_context
														)
{
	switch(device_type){
		case FBE_PMC_SHIM_DEVICE_TYPE_INVALID: 
			csx_dbg_ext_print(" INVALID DEVICE TYPE ");
			break;
		case FBE_PMC_SHIM_DEVICE_TYPE_SSP: 
			csx_dbg_ext_print(" SSP DEVICE ");
			break;
		case FBE_PMC_SHIM_DEVICE_TYPE_STP: 
			csx_dbg_ext_print(" STP DEVICE ");
			break;
		case FBE_PMC_SHIM_DEVICE_TYPE_ENCLOSURE: 
			csx_dbg_ext_print(" ENCLOSURE ");
			break;
		case FBE_PMC_SHIM_DEVICE_TYPE_VIRTUAL: 
			csx_dbg_ext_print(" VIRTUAL DEVICE ");
			break;
		default:
			csx_dbg_ext_print(" UNKNOWN DEVICE ");
			break;
	};
	
	return FBE_STATUS_OK;
}
