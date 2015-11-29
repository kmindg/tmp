/***************************************************************************
 *  Copyright (C)  EMC Corporation 2000-09
 *  All rights reserved.
 *  Licensed material - property of EMC Corporation. * All rights reserved.
 ***************************************************************************/

/*!**************************************************************************
 * @file pp_topology_ext.c
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

#include "pp_dbgext.h"

#include "fbe/fbe_topology_interface.h"
#include "../src/services/topology/fbe_topology_private.h"

#include "fbe_base_object_debug.h"
#include "fbe/fbe_class.h"
#include "fbe_logical_drive_debug.h"
#include "fbe_eses_enclosure_debug.h"
#include "fbe_sas_enclosure_debug.h"
#include "fbe_sas_pmc_port_debug.h"
#include "fbe_base_physical_drive_debug.h"

//
//  Description: Extension to display the version of pp Topology.
//      
//
//  Arguments:
//  none
//
//  Return Value:
//  NONE
//
#pragma data_seg ("EXT_HELP$4pptopver")
static char usageMsg_ppver[] =
"!pptopver\n"
"  Display version of pp topology.\n";
#pragma data_seg ()

DECLARE_API(pptopver)
{
     csx_dbg_ext_print("\tPhysical Package Topology macros v1.0\n");
} // end of !ppver extension

//
//  Description: Extension to display the pp Topology objects.
//      
//
//  Arguments:
//  none
//
//  Return Value:
//  NONE
//
#pragma data_seg ("EXT_HELP$4pptopls")
static char usageMsg_pptopls[] =
"!pptopls\n"
"  Display pp topology objects.\n";
#pragma data_seg ()

DECLARE_API(pptopls)
{
	fbe_dbgext_ptr topology_object_table_ptr = 0;
	fbe_topology_object_status_t  object_status;
	fbe_dbgext_ptr control_handle_ptr = 0;
	ULONG topology_object_table_entry_size;
	int i = 0;
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

	/* topology_object_table_ptr = (fbe_dbgext_ptr)PPDRV_ADDR(topology_object_table);	 */
	FBE_GET_EXPRESSION(pp_ext_module_name, topology_object_table, &topology_object_table_ptr);
	FBE_READ_MEMORY(topology_object_table_ptr, &topology_object_tbl_ptr, ptr_size);
	csx_dbg_ext_print("\t topology_object_table_ptr %I64x\n", topology_object_tbl_ptr);

	/* topology_object_table_entry_size = GetTypeSize("topology_object_table_entry_s"); */
	FBE_GET_TYPE_SIZE(pp_ext_module_name, topology_object_table_entry_s, &topology_object_table_entry_size);
	csx_dbg_ext_print("\t topology_object_table_entry_size %d \n", topology_object_table_entry_size);

	if(topology_object_tbl_ptr == 0){
		return;
	}

	object_status = 0;

	FBE_GET_EXPRESSION(pp_ext_module_name, max_supported_objects, &max_toplogy_entries_p);
	FBE_READ_MEMORY(max_toplogy_entries_p, &max_entries, sizeof(fbe_u32_t));

	csx_dbg_ext_print("ID \t PTR \t\t CLASS \t STATE\n");

	for(i = 0; i < max_entries ; i++){

        if (FBE_CHECK_CONTROL_C())
        {
            return;
        }

		/* The GetFieldData function returns the value of a member in a structure */
		FBE_GET_FIELD_DATA(pp_ext_module_name,
								topology_object_tbl_ptr,
								topology_object_table_entry_s,
								object_status,
								sizeof(fbe_topology_object_status_t),
								&object_status);

		FBE_GET_FIELD_DATA(pp_ext_module_name,
							topology_object_tbl_ptr,
							topology_object_table_entry_s,
							control_handle,
							sizeof(fbe_dbgext_ptr),
							&control_handle_ptr);

		if(object_status == FBE_TOPOLOGY_OBJECT_STATUS_READY){
			csx_dbg_ext_print("%d\t %I64x \t", i, control_handle_ptr);
			
			fbe_base_object_debug_trace_class_id(pp_ext_module_name, control_handle_ptr, fbe_debug_trace_func, NULL);
			csx_dbg_ext_print("\t");

			fbe_base_object_debug_trace_state(pp_ext_module_name, control_handle_ptr, fbe_debug_trace_func, NULL);
			csx_dbg_ext_print("\n");
		}
		topology_object_tbl_ptr	+= topology_object_table_entry_size;
	}

} // end of !ppver extension

/* This is the max number of chars we will allow to be displayed.
 */
#define FBE_PP_TOPOLOGY_MAX_TRACE_CHARS 256 

void fbe_debug_trace_func(fbe_trace_context_t trace_context, const char * fmt, ...)
{
    va_list arguments;
    char string_to_print[FBE_PP_TOPOLOGY_MAX_TRACE_CHARS];

    /* Get the for this variable argument list.
     */
    va_start(arguments, fmt);

    /* This function allows us to do an sprintf to a string (string_to_print) using a 
     * char* and a va_list. 
     */
    _vsnprintf(string_to_print, (sizeof(string_to_print) - 1), fmt, arguments);
    va_end(arguments);

    /* Finally, print out the string.
     */
    csx_dbg_ext_print("%s", string_to_print);
    return;
}

#pragma data_seg ("EXT_HELP$4pptopdisplayobjects")
static char usageMsg_pptopdisplayobjects[] =
"!pptopdisplayobjects\n"
"  Display objects in the system.\n"
"  By default it lists verbose output for all objects in the system\n"
"  -c <class id> displays just this class.\n";
#pragma data_seg ()

DECLARE_API(pptopdisplayobjects)
{
    fbe_dbgext_ptr topology_object_table_ptr = 0;
	fbe_topology_object_status_t  object_status;
	fbe_dbgext_ptr control_handle_ptr = 0;
	ULONG topology_object_table_entry_size;
	int i = 0;
    fbe_class_id_t filter_class = FBE_CLASS_ID_INVALID;
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

    if (strlen(args) && strncmp(args, "-c", 2) == 0)
    {
        csx_dbg_ext_print("filter by class id: %d \n", GetArgument64(args, 2));
        filter_class = GetArgument64(args, 2);
    }

	/* Get the topology table.
     */
	FBE_GET_EXPRESSION(pp_ext_module_name, topology_object_table, &topology_object_table_ptr);
	FBE_READ_MEMORY(topology_object_table_ptr, &topology_object_tbl_ptr, ptr_size);
	csx_dbg_ext_print("\t topology_object_table_ptr %I64x\n", topology_object_tbl_ptr);

    /* Get the size of the topology entry.
     */
	FBE_GET_TYPE_SIZE(pp_ext_module_name, topology_object_table_entry_s, &topology_object_table_entry_size);
	csx_dbg_ext_print("\t topology_object_table_entry_size %d \n", topology_object_table_entry_size);

	if(topology_object_tbl_ptr == 0){
		return;
	}

	object_status = 0;

	FBE_GET_EXPRESSION(pp_ext_module_name, max_supported_objects, &max_toplogy_entries_p);
	FBE_READ_MEMORY(max_toplogy_entries_p, &max_entries, sizeof(fbe_u32_t));

	for(i = 0; i < max_entries ; i++)
    {
        if (FBE_CHECK_CONTROL_C())
        {
            return;
        }
        /* Fetch the object's topology status.
         */
        FBE_GET_FIELD_DATA(pp_ext_module_name,
                           topology_object_tbl_ptr,
                           topology_object_table_entry_s,
                           object_status,
                           sizeof(fbe_topology_object_status_t),
                           &object_status);

        /* Fetch the control handle, which is the pointer to the object.
         */
		FBE_GET_FIELD_DATA(pp_ext_module_name,
							topology_object_tbl_ptr,
							topology_object_table_entry_s,
							control_handle,
							sizeof(fbe_dbgext_ptr),
							&control_handle_ptr);

        /* If the object's topology status is ready, then get the
         * topology status. 
         */
		if (object_status == FBE_TOPOLOGY_OBJECT_STATUS_READY)
        {
            fbe_class_id_t class_id;

            FBE_GET_FIELD_DATA(pp_ext_module_name, 
                               control_handle_ptr,
                               fbe_base_object_s,
                               class_id,
                               sizeof(fbe_class_id_t),
                               &class_id);

            /* Display information on the object if 
             * 1) the class we are filtering by matches the object's class. 
             * or 
             * 2) the user did not select a class id, which means to display all objects.
             */
            if (filter_class == class_id ||
                filter_class == FBE_CLASS_ID_INVALID)
            {
                switch (class_id)
                {
                    case FBE_CLASS_ID_LOGICAL_DRIVE:
                        fbe_logical_drive_debug_trace(pp_ext_module_name, control_handle_ptr, fbe_debug_trace_func, NULL);
                        csx_dbg_ext_print("\n");
                        break;

                    case FBE_CLASS_ID_SAS_VIPER_ENCLOSURE:
                    case FBE_CLASS_ID_ESES_ENCLOSURE:
                        fbe_eses_enclosure_debug_basic_info(pp_ext_module_name, control_handle_ptr, fbe_debug_trace_func, NULL);
                        csx_dbg_ext_print("\n");
                        break;

                    case FBE_CLASS_ID_SAS_ENCLOSURE:
                        fbe_sas_enclosure_debug_basic_info(pp_ext_module_name, control_handle_ptr, fbe_debug_trace_func, NULL);
                        csx_dbg_ext_print("\n");
                        break;
					case FBE_CLASS_ID_SAS_PMC_PORT:
						fbe_sas_pmc_port_debug_trace(pp_ext_module_name, control_handle_ptr, fbe_debug_trace_func, NULL);
						csx_dbg_ext_print("\n");
						break;
                                                
                    case FBE_CLASS_ID_BASE_PHYSICAL_DRIVE:
                    case FBE_CLASS_ID_SAS_PHYSICAL_DRIVE:
                    case FBE_CLASS_ID_SATA_PHYSICAL_DRIVE:
                        fbe_base_physical_drive_debug_trace(pp_ext_module_name, control_handle_ptr, fbe_debug_trace_func, NULL);
                        csx_dbg_ext_print("\n");
                        break;
                                                
                        /* Add new classes here.
                         */
                        
                    default:
                        /* There is no handling for this class yet.
                         * Display some generic information about it. 
                         */
                        csx_dbg_ext_print("object id: %d object ptr: 0x%I64x ", i, control_handle_ptr);

                        csx_dbg_ext_print("class id: ");
                        fbe_base_object_debug_trace_class_id(pp_ext_module_name, control_handle_ptr, fbe_debug_trace_func, NULL);
                        csx_dbg_ext_print(" lifecycle state: ");
                        fbe_base_object_debug_trace_state(pp_ext_module_name, control_handle_ptr, fbe_debug_trace_func, NULL);
                        csx_dbg_ext_print("\n");
                        break;
                }
            }
		} /* End if object's topology status is ready. */
		topology_object_tbl_ptr += topology_object_table_entry_size;
	}
	return;
}



