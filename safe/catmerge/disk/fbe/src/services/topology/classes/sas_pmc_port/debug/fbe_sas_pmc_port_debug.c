/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_sas_pmc_port_debug.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the debug functions for PMC SAS Port object.
 *
 * @author
 *   01/16/2009:  Adapted for PMC SAS Port object. 
 *   12/3/2008:  Created. RPF
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_types.h"
#include "fbe/fbe_dbgext.h"
#include "fbe_base_object_debug.h"
#include "fbe_sas_port_debug.h"
#include "sas_pmc_port_private.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/*!**************************************************************
 * @fn fbe_sas_pmc_port_debug_trace(const fbe_u8_t * module_name, 
 *                                   fbe_dbgext_ptr port_object_p,
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
 *  01/16/2009:  Adapted for PMC SAS Port object. 
 *  12/3/2008 - Created. RPF
 *
 ****************************************************************/

fbe_status_t fbe_sas_pmc_port_debug_trace(const fbe_u8_t * module_name, 
                                           fbe_dbgext_ptr port_object_p, 
                                           fbe_trace_func_t trace_func, 
                                           fbe_trace_context_t trace_context)
{
	fbe_sas_port_debug_trace(module_name,port_object_p,trace_func,trace_context);	
	csx_dbg_ext_print("\n");
	return FBE_STATUS_OK;
}
/*************************
 * end file fbe_sas_pmc_port_debug.c
 *************************/


static fbe_bool_t sas_pmc_debug_is_device_type_valid(fbe_cpd_shim_discovery_device_type_t device_type)
{
	fbe_bool_t device_type_valid = FALSE;
	switch(device_type){
		case FBE_CPD_SHIM_DEVICE_TYPE_SSP: 
		case FBE_CPD_SHIM_DEVICE_TYPE_STP: 
		case FBE_CPD_SHIM_DEVICE_TYPE_ENCLOSURE: 
		case FBE_CPD_SHIM_DEVICE_TYPE_VIRTUAL: 
			device_type_valid = TRUE;
			break;
		default:
			device_type_valid = FALSE;
			break;
	};

	return device_type_valid;
}

	
static fbe_status_t sas_pmc_debug_trace_device_type(fbe_cpd_shim_discovery_device_type_t device_type,
														fbe_trace_func_t trace_func, 
														fbe_trace_context_t trace_context
														)
{
	switch(device_type){
		case FBE_CPD_SHIM_DEVICE_TYPE_INVALID: 
			csx_dbg_ext_print(" INVALID DEVICE TYPE ");
			break;
		case FBE_CPD_SHIM_DEVICE_TYPE_SSP: 
			csx_dbg_ext_print(" SSP DEVICE ");
			break;
		case FBE_CPD_SHIM_DEVICE_TYPE_STP: 
			csx_dbg_ext_print(" STP DEVICE ");
			break;
		case FBE_CPD_SHIM_DEVICE_TYPE_ENCLOSURE: 
			csx_dbg_ext_print(" ENCLOSURE ");
			break;
		case FBE_CPD_SHIM_DEVICE_TYPE_VIRTUAL: 
			csx_dbg_ext_print(" VIRTUAL DEVICE ");
			break;
		default:
			csx_dbg_ext_print(" UNKNOWN DEVICE ");
			break;
	};
	
	return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_sas_pmc_port_display_device_table(const fbe_u8_t * module_name, 
 *                                   fbe_dbgext_ptr port_object_p,
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
 *  03/13/2009:  AJ  
 *
 ****************************************************************/

fbe_status_t fbe_sas_pmc_port_display_device_table(const fbe_u8_t * module_name, 
                                           fbe_dbgext_ptr port_object_p, 
                                           fbe_trace_func_t trace_func, 
                                           fbe_trace_context_t trace_context)
{
	fbe_dbgext_ptr  device_entry_p;
	fbe_u32_t index,table_size,device_entry_size;
	fbe_miniport_device_id_t             device_id,parent_device_id;
    fbe_sas_address_t                    device_address;
    fbe_cpd_shim_device_locator_t        device_locator;
    fbe_cpd_shim_discovery_device_type_t device_type;
	fbe_bool_t							 device_logged_in;
	fbe_u32_t							 current_gen_number;

	FBE_GET_FIELD_DATA(module_name, 
						port_object_p,
						fbe_sas_pmc_port_s,
						device_table_ptr,
						sizeof(fbe_dbgext_ptr),
						&device_entry_p);
	csx_dbg_ext_print("\t\t Device Table Ptr : 0x%llx \n", (unsigned long long)device_entry_p);


	FBE_GET_FIELD_DATA(module_name, 
						port_object_p,
						fbe_sas_pmc_port_s,
						device_table_max_index,
						sizeof(fbe_u32_t),
						&table_size);
	csx_dbg_ext_print("\t Device Table Size: 0x%x \n", table_size);
	

	for(index = 0; index < table_size ;index++){
		FBE_GET_FIELD_DATA(module_name, 
							device_entry_p,
							fbe_sas_pmc_port_device_table_entry_s,
							device_id,
							sizeof(fbe_miniport_device_id_t),
							&device_id);
		FBE_GET_FIELD_DATA(module_name, 
							device_entry_p,
							fbe_sas_pmc_port_device_table_entry_s,
							device_address,
							sizeof(fbe_sas_address_t),
							&device_address);
		FBE_GET_FIELD_DATA(module_name, 
							device_entry_p,
							fbe_sas_pmc_port_device_table_entry_s,
							device_locator,
							sizeof(fbe_cpd_shim_device_locator_t),
							&device_locator);
		FBE_GET_FIELD_DATA(module_name, 
							device_entry_p,
							fbe_sas_pmc_port_device_table_entry_s,
							device_type,
							sizeof(fbe_cpd_shim_discovery_device_type_t),
							&device_type);
		FBE_GET_FIELD_DATA(module_name, 
							device_entry_p,
							fbe_sas_pmc_port_device_table_entry_s,
							parent_device_id,
							sizeof(fbe_miniport_device_id_t),
							&parent_device_id);
		FBE_GET_FIELD_DATA(module_name, 
							device_entry_p,
							fbe_sas_pmc_port_device_table_entry_s,
							device_logged_in,
							sizeof(fbe_bool_t),
							&device_logged_in);
		FBE_GET_FIELD_DATA(module_name, 
							device_entry_p,
							fbe_sas_pmc_port_device_table_entry_s,
							current_gen_number,
							sizeof(fbe_u32_t),
							&current_gen_number);


		if (sas_pmc_debug_is_device_type_valid(device_type)){
			csx_dbg_ext_print("\t  Entry : 0x%llx \n", (unsigned long long)device_entry_p);		
			csx_dbg_ext_print("\t\t Device ID : 0x%llx \n", (unsigned long long)device_id);
			csx_dbg_ext_print("\t\t Entry Gen # : 0x%x  Device Logged %s.\n", current_gen_number,(device_logged_in ? "IN" : "OUT"));
			csx_dbg_ext_print("\t\t Device Address : 0x%llx \n", (unsigned long long)device_address);
			csx_dbg_ext_print("\t\t Device Locator : Depth: 0x%x Phy #: 0x%x\n", device_locator.enclosure_chain_depth,
						device_locator.phy_number);
			csx_dbg_ext_print("\t\t Device Type : ");
			sas_pmc_debug_trace_device_type(device_type,trace_func,trace_context);
			csx_dbg_ext_print("\n");
			csx_dbg_ext_print("\t\t Parent Device ID : 0x%llx \n", (unsigned long long)parent_device_id);
		}

		FBE_GET_TYPE_SIZE(module_name, fbe_sas_pmc_port_device_table_entry_s, &device_entry_size)
		device_entry_p += device_entry_size;
	}

	return FBE_STATUS_OK;
}


/**********************************************
 * end fbe_sas_pmc_port_display_device_table()
 *********************************************/

