/***************************************************************************
 *  Copyright (C)  EMC Corporation 2000-07
 *  All rights reserved.
 *  Licensed material - property of EMC Corporation. * All rights reserved.
 ***************************************************************************/

/***************************************************************************
 * pp_commands.c
 ***************************************************************************
 *
 * File Description:
 *
 *Debugging commands for PhysicalPackage driver.
 *
 * Author:
 *  uday Kumar
 *
 * Revision History:
 *
 * Peter Puhov 18/05/09  Initial version.2
 *
 ***************************************************************************/
#include "csx_ext.h"
#include "pp_dbgext.h"
#include "fbe/fbe_topology_interface.h"
#include "fbe/fbe_package.h"
#include "fbe_base_object_debug.h"
#include "fbe/fbe_class.h"
#include "fbe_logical_drive_debug.h"
#include "fbe_eses_enclosure_debug.h"
#include "fbe_sas_enclosure_debug.h"
#include "fbe_base_enclosure_debug.h"
#include "fbe_sas_pmc_port_debug.h"
#include "fbe_base_physical_drive_debug.h"
#include "fbe_topology_debug.h"

/***************** 
 * Globals
 ******************/
static fbe_bool_t       fbe_pp_commands_is_sim_mode_determined = FBE_FALSE;
static fbe_bool_t       vmsim_mode = FBE_FALSE;
static fbe_dbgext_ptr   vmsim_mode_global_ptr;
static fbe_dbgext_ptr   vmsim_mode_ptr;
static fbe_u32_t        vmsim_mode_offset;
static char             vmsim_type[128];

static void fbe_pp_topology_ext_trace_commands_func(fbe_trace_context_t trace_context, const char * fmt, ...) __attribute__((format(__printf_func__,2,3)));


static void fbe_pp_topology_ext_trace_commands_func(fbe_trace_context_t trace_context, const char * fmt, ...)
{
    va_list argList;
    va_start(argList, fmt);
    csx_dbg_ext_print(fmt, argList);
    va_end(argList);    
}




//
//  Description: Extension to display the version of pp.
//      
//
//  Arguments:
//  none
//
//  Return Value:
//  NONE
//

#pragma data_seg ("EXT_HELP$4ppeseselemgrp")
static char CSX_MAYBE_UNUSED usageMsg_ppeseselemgrp[] =
"!ppeseselemgrp\n"
" By default Display eses element grp status.\n"
"  -obj <object id> displays just for this object.\n"
"  -b <port index> -e <encl. position> displays just for this port and encl.\n"
"  -b <port index> displays just for this port.\n";
#pragma data_seg ()

CSX_DBG_EXT_DEFINE_CMD(ppeseselemgrp, "ppeseselemgrp")
{
	fbe_dbgext_ptr topology_object_table_ptr = 0;
	fbe_topology_object_status_t  object_status;
	fbe_dbgext_ptr control_handle_ptr = 0;
	ULONG topology_object_table_entry_size;
	int i = 0;
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
        return; 
    }

    /* Get the topology table. */
	FBE_GET_EXPRESSION(pp_ext_module_name, topology_object_table, &topology_object_table_ptr);
	FBE_READ_MEMORY(topology_object_table_ptr, &topology_object_tbl_ptr, ptr_size);
	csx_dbg_ext_print("\t topology_object_table_ptr %llx\n", (unsigned long long)topology_object_tbl_ptr);

	FBE_GET_EXPRESSION(pp_ext_module_name, max_supported_objects, &max_toplogy_entries_p);
	FBE_READ_MEMORY(max_toplogy_entries_p, &max_entries, sizeof(fbe_u32_t));
	csx_dbg_ext_print("\t max entries %d\n", max_entries);

    /* Get the size of the topology entry. */
	FBE_GET_TYPE_SIZE(pp_ext_module_name, topology_object_table_entry_s, &topology_object_table_entry_size);
	csx_dbg_ext_print("\t topology_object_table_entry_size %d \n", topology_object_table_entry_size);

    if(topology_object_tbl_ptr == 0)
    {
		return;
	}

	object_status = 0;
	for(i = 0; i < max_entries ; i++)
	{
        /* Fetch the object's topology status. */
          FBE_GET_FIELD_DATA(pp_ext_module_name,
                           topology_object_tbl_ptr,
                           topology_object_table_entry_s,
                           object_status,
                           sizeof(fbe_topology_object_status_t),
                           &object_status);

        /* Fetch the control handle, which is the pointer to the object. */
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
           // csx_dbg_ext_print("\n object status is ready \n");
            FBE_GET_FIELD_DATA(pp_ext_module_name, 
                               control_handle_ptr,
                               fbe_base_object_s,
                               class_id,
                               sizeof(fbe_class_id_t),
                               &class_id);

            if (IS_VALID_LEAF_ENCL_CLASS(class_id))
            {
                        fbe_eses_enclosure_element_group_debug_basic_info(pp_ext_module_name, control_handle_ptr, NULL, NULL);
                        csx_dbg_ext_print("\n");
            }
	} /* End if object's topology status is ready. */
	
         topology_object_tbl_ptr += topology_object_table_entry_size;	
    }
    return ;
	}



/* Temporary hack until dependency to base service will be cleaned */
fbe_status_t 
fbe_get_package_id(fbe_package_id_t * package_id)
{
	*package_id = FBE_PACKAGE_ID_PHYSICAL;
	return FBE_STATUS_OK;
}
/* End of temporary hack until dependency to base service will be cleaned */

static void fbe_pp_commands_determine_sim_mode(void)
{
#ifdef __SAFE__
  FBE_GET_EXPRESSION("specl", pSpeclDevExt,&vmsim_mode_global_ptr);
  FBE_READ_MEMORY(vmsim_mode_global_ptr,&vmsim_mode_ptr, sizeof(fbe_dbgext_ptr));
  strcpy(vmsim_type, "specl!pSpeclDevExt");     
  csx_dbg_ext_get_field_offset(vmsim_type,"vmSimulationMode", &vmsim_mode_offset);
  FBE_READ_MEMORY(vmsim_mode_ptr + vmsim_mode_offset, &vmsim_mode, sizeof(fbe_bool_t));
#else
  vmsim_mode = FBE_FALSE;
#endif
  fbe_pp_commands_is_sim_mode_determined = FBE_TRUE;
}

fbe_u8_t* fbe_get_module_name(void) {


  fbe_u8_t* module_name;

  if (fbe_pp_commands_is_sim_mode_determined == FBE_FALSE)
  {
      fbe_pp_commands_determine_sim_mode();
  }

  if(vmsim_mode) {
    module_name = "VmSimPhysicalPackage";
  }
  else {
    module_name = "PhysicalPackage";
  }
  return module_name;
}

