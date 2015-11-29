/***************************************************************************
 *  Copyright (C)  EMC Corporation 2000-07
 *  All rights reserved.
 *  Licensed material - property of EMC Corporation. * All rights reserved.
 ***************************************************************************/

/***************************************************************************
 * pp_topology_ext.c
 ***************************************************************************
 *
 * File Description:
 *
 *   Debugging extensions for PhysicalPackage driver.
 *
 * Author:
 *   Peter Puhov
 *
 * Revision History:
 *
 * Peter Puhov 11/05/00  Initial version.
 *
 ***************************************************************************/

#include "csx_ext.h"
#include "pp_dbgext.h"

#include "fbe/fbe_topology_interface.h"
#include "../src/services/topology/fbe_topology_private.h"

#include "fbe_base_object_debug.h"
#include "fbe/fbe_class.h"
#include "fbe_sas_pmc_port_debug.h"
#include "fbe_pmc_shim_debug.h"

static void fbe_pp_pmc_shim_ext_trace_func(fbe_trace_context_t trace_context, const char * fmt, ...) __attribute__((format(__printf_func__,2,3)));


/* TODO .h file*/
fbe_status_t fbe_pmc_shim_debug_trace_port_info(const fbe_u8_t * module_name, 
												fbe_dbgext_ptr pmc_port_array_ptr, 
												fbe_trace_func_t trace_func, 
												fbe_trace_context_t trace_context,
												fbe_u32_t io_port_number,
												fbe_u32_t io_portal_number
												);


//
//  Description: 
//      
//
//  Arguments:
//  none
//
//  Return Value:
//  NONE
//
#pragma data_seg ("EXT_HELP$4pp_display_mp_shim_table")
static char usageMsg_pp_display_mp_shim_table[] =
"!pp_display_mp_shim_table\n"
"  Display miniport shim table contents.\n"
"  -h <port_handle>\n"
"  -p <port_number> <portal_number>\n";
#pragma data_seg ()

DECLARE_API(pp_display_mp_shim_table)
{
	fbe_u32_t port_number,portal_number;
	fbe_u32_t port_handle;
	fbe_dbgext_ptr pmc_port_array_ptr = 0;

	/* Get the port table.
     */
	FBE_GET_EXPRESSION(pp_ext_module_name, pmc_port_array, &pmc_port_array_ptr);
	csx_dbg_ext_print("\t Port array ptr %I64x\n", pmc_port_array_ptr);

    if (strlen(args)){
		if (strncmp(args, "-p", 2) == 0){			
			port_number = (fbe_u32_t)GetArgument64(args, 2);
			portal_number = (fbe_u32_t)GetArgument64(args, 3);
			csx_dbg_ext_print("Displaying for Port# : %d Portal # : %d\n", port_number,portal_number);
		}else 
			if (strncmp(args, "-h", 2) == 0){
				port_handle = (fbe_u32_t)GetArgument64(args, 2);
				csx_dbg_ext_print("Displaying for port handle # : %d \n", port_handle);				
			}
	}
#if 0
	else{
		csx_dbg_ext_print("Displaying for all valid ports \n");
		for (index = 0; index < FBE_PMC_SHIM_MAX_PORTS; index++){
			fbe_pmc_shim_debug_display_device_table(pp_ext_module_name, fbe_pp_pmc_shim_ext_trace_func, NULL);
		}
	}
#endif
	return;
} // end of !pp_display_mp_shim_table extension

//
//  Description: 
//      
//
//  Arguments:
//  none
//
//  Return Value:
//  NONE
//
#pragma data_seg ("EXT_HELP$4pp_display_mp_shim_port_info")
static char usageMsg_pp_display_mp_shim_port_info[] =
"!pp_display_mp_shim_port_info\n"
"  Display miniport shim port info.\n"
"  -h <port_handle>\n"
"  -p <port_number> <portal_number>\n"
"  if no parameter is specified, details about all valid ports are displayed.";
#pragma data_seg ()

DECLARE_API(pp_display_mp_shim_port_info)
{
	fbe_u32_t port_number = 0,portal_number = 0;
	fbe_u32_t port_handle = 0,index = 0;
	fbe_dbgext_ptr pmc_port_array_ptr = 0;

	/* Get the port table.
     */
	FBE_GET_EXPRESSION(pp_ext_module_name, pmc_port_array, &pmc_port_array_ptr);
	csx_dbg_ext_print("\t Port array ptr %I64x\n", pmc_port_array_ptr);

    if (strlen(args)){
		if (strncmp(args, "-p", 2) == 0){			
			port_number = (fbe_u32_t)GetArgument64(args, 2);
			portal_number = (fbe_u32_t)GetArgument64(args, 3);
			csx_dbg_ext_print("Displaying for Port# : %d Portal # : %d\n", port_number,portal_number);
			fbe_pmc_shim_debug_trace_port_info(pp_ext_module_name,pmc_port_array_ptr, fbe_pp_pmc_shim_ext_trace_func, 
												NULL, port_number,portal_number);

		}else 
			if (strncmp(args, "-h", 2) == 0){
				port_handle = (fbe_u32_t)GetArgument64(args, 2);
				csx_dbg_ext_print("Displaying for port handle # : %d \n", port_handle);
				fbe_pmc_shim_debug_trace_port_handle_info(pp_ext_module_name,pmc_port_array_ptr, fbe_pp_pmc_shim_ext_trace_func, 
															NULL, port_handle);

			}
	}
#if 0
	else{
		csx_dbg_ext_print("Displaying for all valid ports \n");
		for (index = 0; index < FBE_PMC_SHIM_MAX_PORTS; index++){
			fbe_pmc_shim_debug_display_device_table(pp_ext_module_name, fbe_pp_pmc_shim_ext_trace_func, NULL);
		}
	}
#endif
	return;
} // end of !pp_display_mp_shim_table extension

//
//  Description: 
//      
//
//  Arguments:
//  none
//
//  Return Value:
//  NONE
//
#pragma data_seg ("EXT_HELP$4pp_display_port_obj_dev_table")
static char usageMsg_pp_display_port_obj_dev_table[] =
"!pp_display_port_obj_dev_table <port object ID>\n"
"  Display port object's device table contents.\n";
#pragma data_seg ()

DECLARE_API(pp_display_port_obj_dev_table)
{
	fbe_dbgext_ptr topology_object_table_ptr = 0;	
	fbe_dbgext_ptr control_handle_ptr = 0;
	fbe_u32_t	port_object_id = 1;
	fbe_u32_t	topology_object_table_entry_size;
	fbe_class_id_t class_id;
	ULONG64 max_toplogy_entries_p = 0;
	fbe_u32_t max_entries;
	fbe_dbgext_ptr topology_object_tbl_ptr = 0;
    fbe_u32_t ptr_size;
    fbe_status_t status = FBE_STATUS_OK;

    status = fbe_debug_get_ptr_size(pp_ext_module_name, &ptr_size);
    if (status != FBE_STATUS_OK) 
    {
        csx_dbg_ext_print("%s unable to get ptr size status: %d\n", __FUNCTION__, status);
        return ; 
    }

	FBE_GET_EXPRESSION(pp_ext_module_name, max_supported_objects, &max_toplogy_entries_p);
	FBE_READ_MEMORY(max_toplogy_entries_p, &max_entries, sizeof(fbe_u32_t));

    if (strlen(args)){
		port_object_id = (fbe_u32_t)GetArgument64(args, 1);
	}

	if (port_object_id >= max_entries){
		return;
	}

	FBE_GET_EXPRESSION(pp_ext_module_name, topology_object_table, &topology_object_table_ptr);
	FBE_READ_MEMORY(p_object_table, &topology_object_tbl_ptr, ptr_size);
	FBE_GET_TYPE_SIZE(pp_ext_module_name, topology_object_table_entry_s, &topology_object_table_entry_size);
	
	topology_object_tbl_ptr	+= port_object_id*topology_object_table_entry_size;

	FBE_GET_FIELD_DATA(pp_ext_module_name,
						topology_object_tbl_ptr,
						topology_object_table_entry_s,
						control_handle,
						sizeof(fbe_dbgext_ptr),
						&control_handle_ptr);

    FBE_GET_FIELD_DATA(pp_ext_module_name, 
                        control_handle_ptr,
                        fbe_base_object_s,
                        class_id,
                        sizeof(fbe_class_id_t),
                        &class_id);

	if (class_id != FBE_CLASS_ID_SAS_PMC_PORT){
		return;
	}

	fbe_sas_pmc_port_display_device_table(pp_ext_module_name, 
                                           control_handle_ptr, 
                                           fbe_debug_trace_func, 
                                           NULL);


	return;
} // end of !pp_display_port_obj_dev_table  extension

static void fbe_pp_pmc_shim_ext_trace_func(fbe_trace_context_t trace_context, const char * fmt, ...)
{
    va_list argList;
    va_start(argList, fmt);
    csx_dbg_ext_print(fmt, argList);
    va_end(argList);    
}

/*  TODO:
 *   1) Display Port Table details.
 *   2) Lookup port table entry with port handle.
 *   3) Lookup port table entry with back end number.
 *   4) Display port table entries.
 *   5) Display Port Object Info using port table info.
 *   6) Display Port Object Device Table info.
 *   7) Diff of ShimDev Table and Port Object dev Table.
 *
 *   8) Display DMRB details for a given packet.
 *   9) Display Packet information for a given DMRB.
 */