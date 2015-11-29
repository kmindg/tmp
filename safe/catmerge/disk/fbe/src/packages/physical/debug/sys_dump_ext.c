
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
#include "fbe_base_object_debug.h"
#include "fbe/fbe_class.h"
#include "fbe_logical_drive_debug.h"
#include "fbe_eses_enclosure_debug.h"
#include "fbe_sas_enclosure_debug.h"
#include "fbe_sas_pmc_port_debug.h"
#include "fbe_fc_port_debug.h"
#include "fbe_iscsi_port_debug.h"
#include "fbe_base_physical_drive_debug.h"
#include "fbe_sepls_debug.h"
#include "fbe_raid_group_debug.h"
#include "fbe_raid_library_debug.h"
#include "fbe_virtual_drive_debug.h"
#include "fbe_provision_drive_debug.h"
#include "fbe_lun_debug.h"
#include "fbe_encl_mgmt_debug.h"
#include "fbe_sps_mgmt_debug.h"
#include "fbe_drive_mgmt_debug.h"
#include "fbe_ps_mgmt_debug.h"
#include "fbe_modules_mgmt_debug.h"
#include "fbe_board_mgmt_debug.h"
#include "fbe_cooling_mgmt_debug.h"
#include "fbe_topology_debug.h"
#include "pp_ext.h"
#include "fbe_database.h"
#include "fbe_database_debug.h"

/* This is the max number of chars we will allow to be displayed.
 */
#define FBE_PP_TOPOLOGY_MAX_TRACE_CHARS 512 

/* 
  trace function for dump sep config with the same format as flare DRT file. this function 
  will receive a prefix from the caller and print any line other than space or newline with
  the prefix
*/
void fbe_dump_debug_trace_func(fbe_trace_context_t trace_context, const char * fmt, ...)
{
    va_list arguments;
    int i = 0;
    fbe_dump_debug_trace_prefix_t *prefix = NULL;
    char string_to_print[FBE_PP_TOPOLOGY_MAX_TRACE_CHARS];
    char print_with_prefix[FBE_PP_TOPOLOGY_MAX_TRACE_CHARS];

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
    prefix = (fbe_dump_debug_trace_prefix_t *)trace_context;
    if (prefix != NULL) {
        /*will not add prefix for any lines only have ' ', '\t' and '\n'*/
        for (i = 0; i < sizeof (string_to_print); i ++) {
            if (string_to_print[i] == '\0') {
                break;
            }
            if (string_to_print[i] != ' ' && string_to_print[i] != '\n' && string_to_print[i] != '\t') {
                break;
            }
        }
        if (i == sizeof (string_to_print) || string_to_print[i] == '\0') {
            csx_dbg_ext_print("%s", string_to_print);
            return;
        }
        _snprintf(print_with_prefix, (sizeof(print_with_prefix) - 1), "%s.%s", prefix->prefix, string_to_print);
        csx_dbg_ext_print("%s", print_with_prefix);
    } else {
        csx_dbg_ext_print("%s", string_to_print);
    }

    return;
}

#pragma data_seg ("EXT_HELP$4sepdumpconfig")
static char CSX_MAYBE_UNUSED usageMsg_sepdumpconfig[] =
"!sepdumpconfig\n"
"  Dump objects (config and nonpaged metadata) and database configuration in the SEP.\n";
#pragma data_seg ()

CSX_DBG_EXT_DEFINE_CMD(sepdumpconfig, "sepdumpconfig")
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
    fbe_class_id_t class_id;
    fbe_dump_debug_trace_prefix_t dump_prefix;
    fbe_dbgext_ptr addr;
    
    /*it only works for SEP package*/
    pp_ext_module_name = "sep";


    status = fbe_debug_get_ptr_size(pp_ext_module_name, &ptr_size);
    if (status != FBE_STATUS_OK) 
    {
        csx_dbg_ext_print("%s unable to get ptr size status: %d\n", __FUNCTION__, status);
        return ; 
    }

    FBE_GET_EXPRESSION(pp_ext_module_name, max_supported_objects, &max_toplogy_entries_p);
    FBE_READ_MEMORY(max_toplogy_entries_p, &max_entries, sizeof(fbe_u32_t));

    FBE_GET_EXPRESSION(pp_ext_module_name, topology_object_table, &topology_object_table_ptr);
    FBE_READ_MEMORY(topology_object_table_ptr, &topology_object_tbl_ptr, ptr_size);
    csx_dbg_ext_print("\ntopology_object_table_ptr %llx\n", (unsigned long long)topology_object_tbl_ptr);

    /* Get the size of the topology entry.
     */
    FBE_GET_TYPE_SIZE(pp_ext_module_name, topology_object_table_entry_t, &topology_object_table_entry_size);
    csx_dbg_ext_print("topology_object_table_entry_size %d \n", topology_object_table_entry_size);

    if(topology_object_tbl_ptr == 0){
        return;
    }

    if(!fbe_sep_offsets.is_fbe_sep_offsets_set)
    {
        fbe_sep_offsets_set(pp_ext_module_name);
    }
    
    object_status = 0;

    for(i = 0; i < max_entries ; i++)
    {
        if (FBE_CHECK_CONTROL_C())
        {
            fbe_sep_offsets_clear();
            return;
        }
        /* Fetch the object's topology status.
         */
        addr = topology_object_tbl_ptr + fbe_sep_offsets.object_status_offset;
        FBE_READ_MEMORY(addr, &object_status,
                        sizeof(fbe_topology_object_status_t));

        /* If the object's topology status is ready, then get the
         * topology status. 
         */
        if (object_status == FBE_TOPOLOGY_OBJECT_STATUS_READY)
        {
            /* Fetch the control handle, which is the pointer to the object.
             */
            addr = topology_object_tbl_ptr +
                   fbe_sep_offsets.control_handle_offset;

            FBE_READ_MEMORY(addr, &control_handle_ptr, sizeof(fbe_dbgext_ptr));
            
            fbe_debug_trace_func(NULL, "topology_object_table_ptr[0x%x].control_handle: 0x%llx \n", i, (unsigned long long)control_handle_ptr);
            fbe_debug_trace_func(NULL, "topology_object_table_ptr[0x%x].object:   ", i);

            addr = control_handle_ptr + fbe_sep_offsets.class_id_offset;
            FBE_READ_MEMORY(addr, &class_id, sizeof(fbe_class_id_t));

            switch (class_id)
            {
                case FBE_CLASS_ID_PARITY:                                                    
                case FBE_CLASS_ID_STRIPER:
                case FBE_CLASS_ID_MIRROR:
                    fbe_debug_trace_func(NULL, "Raid Group(0x%x): \n", i);
                    fbe_debug_dump_generate_prefix(&dump_prefix, NULL, "RaidGroup");
                    fbe_raid_group_dump_debug_trace(pp_ext_module_name, control_handle_ptr, fbe_dump_debug_trace_func, 
                                                       &dump_prefix, NULL /* No field info. */, 0    /* spaces to indent */);
                    break;
                                                    

                case FBE_CLASS_ID_VIRTUAL_DRIVE:
                    fbe_debug_trace_func(NULL, "virtual Drive(0x%x): \n", i);
                    fbe_debug_dump_generate_prefix(&dump_prefix, NULL, "VirtualDrive");
                    fbe_virtual_drive_dump_debug_trace(pp_ext_module_name, control_handle_ptr, fbe_dump_debug_trace_func, &dump_prefix, 0);
                    csx_dbg_ext_print("\n");

                    break;
                      
                case FBE_CLASS_ID_PROVISION_DRIVE:    
                    fbe_debug_trace_func(NULL, "provision Drive(0x%x): \n", i);
                    fbe_debug_dump_generate_prefix(&dump_prefix, NULL, "ProvisionDrive");
                    fbe_provision_drive_dump_debug_trace(pp_ext_module_name, control_handle_ptr, fbe_dump_debug_trace_func, &dump_prefix, 0);
                    csx_dbg_ext_print("\n");

                    break;
                case FBE_CLASS_ID_LUN:
                    fbe_debug_trace_func(NULL, "Lun object(0x%x): \n", i);
                    fbe_debug_dump_generate_prefix(&dump_prefix, NULL, "Lun");
                    fbe_lun_dump_debug_trace(pp_ext_module_name, control_handle_ptr, fbe_dump_debug_trace_func, &dump_prefix, 0);
                    csx_dbg_ext_print("\n");

                    break;
                default:
                    /* There is no handling for this class yet.
                     * Display some generic information about it. 
                    */
                    csx_dbg_ext_print("object id: %d object ptr: 0x%llx \n", i, (unsigned long long)control_handle_ptr);

                    csx_dbg_ext_print("class id: ");
                    fbe_base_object_debug_trace_class_id(pp_ext_module_name, control_handle_ptr, fbe_debug_trace_func, NULL);
                    csx_dbg_ext_print("class id: \n");
                    csx_dbg_ext_print(" lifecycle state: ");
                    fbe_base_object_debug_trace_state(pp_ext_module_name, control_handle_ptr, fbe_debug_trace_func, NULL);
                    csx_dbg_ext_print("\n");
                    break;
            }
               
        } /* End if object's topology status is ready. */
        topology_object_tbl_ptr += topology_object_table_entry_size;
    }

    /*dump the database table*/
    fbe_database_service_debug_dump_all(pp_ext_module_name, fbe_debug_trace_func, NULL);

    fbe_debug_trace_func(NULL, "\n");
    fbe_sep_offsets_clear();
    
    return;
}



