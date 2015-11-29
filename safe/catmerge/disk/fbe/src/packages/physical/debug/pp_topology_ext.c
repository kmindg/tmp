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
#include "fbe_extent_pool_debug.h"
#include "fbe_encl_mgmt_debug.h"
#include "fbe_sps_mgmt_debug.h"
#include "fbe_drive_mgmt_debug.h"
#include "fbe_ps_mgmt_debug.h"
#include "fbe_modules_mgmt_debug.h"
#include "fbe_board_mgmt_debug.h"
#include "fbe_cooling_mgmt_debug.h"
#include "fbe_topology_debug.h"
#include "pp_ext.h"
#include "fbe/fbe_base_config.h"
#include "fbe/fbe_dbgext.h"

static fbe_status_t 
fbe_topology_print_object(const fbe_u8_t * module_name,
                          fbe_dbgext_ptr topology_object_tbl_ptr,
                          fbe_class_id_t filter_class,
                          fbe_object_id_t filter_object,
                          fbe_bool_t b_parity_write_log,
                          fbe_bool_t b_display_io,
                          fbe_bool_t b_summary,
                          fbe_u32_t object_status_offset,
                          fbe_u32_t control_handle_offset,
                          fbe_u32_t class_id_offset,
                          fbe_trace_func_t trace_func,
                          fbe_trace_context_t trace_context,
                          fbe_u32_t spaces_to_indent);

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
static char CSX_MAYBE_UNUSED usageMsg_ppver[] =
"!pptopver\n"
"  Display version of pp topology.\n";
#pragma data_seg ()

CSX_DBG_EXT_DEFINE_CMD(pptopver, "pptopver")
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
static char CSX_MAYBE_UNUSED usageMsg_pptopls[] =
"!pptopls\n"
"  Display pp topology objects.\n";
#pragma data_seg ()

CSX_DBG_EXT_DEFINE_CMD(pptopls, "pptopls")
{
    fbe_dbgext_ptr topology_object_table_ptr = 0;
    fbe_topology_object_status_t  object_status;
    fbe_dbgext_ptr control_handle_ptr = 0;
    ULONG topology_object_table_entry_size;
    ULONG64 max_toplogy_entries_p = 0;
    fbe_dbgext_ptr topology_object_tbl_ptr = 0;
    fbe_dbgext_ptr addr;
    int i = 0;
    fbe_u32_t max_entries;
    fbe_u32_t ptr_size;
    fbe_status_t status = FBE_STATUS_OK;

    status = fbe_debug_get_ptr_size(pp_ext_module_name, &ptr_size);
    if (status != FBE_STATUS_OK) 
    {
        csx_dbg_ext_print("%s unable to get ptr size status: %d\n", __FUNCTION__, status);
        return ; 
    }

    FBE_GET_EXPRESSION(pp_ext_module_name, topology_object_table, &topology_object_table_ptr);
    FBE_READ_MEMORY(topology_object_table_ptr, &topology_object_tbl_ptr, ptr_size);
    csx_dbg_ext_print("\t topology_object_table_ptr %llx\n", (unsigned long long)topology_object_tbl_ptr);

    FBE_GET_TYPE_SIZE(pp_ext_module_name, topology_object_table_entry_s, &topology_object_table_entry_size);
    csx_dbg_ext_print("\t topology_object_table_entry_size %d \n", topology_object_table_entry_size);

    FBE_GET_EXPRESSION(pp_ext_module_name, max_supported_objects, &max_toplogy_entries_p);
    FBE_READ_MEMORY(max_toplogy_entries_p, &max_entries, sizeof(fbe_u32_t));
    csx_dbg_ext_print("\t max_supported_objects %d\n", max_entries);

    if(topology_object_table_ptr == 0){
        return;
    }

    if(!fbe_sep_offsets.is_fbe_sep_offsets_set)
    {
        fbe_sep_offsets_set(pp_ext_module_name);
    }
    
    object_status = 0;

    csx_dbg_ext_print("ID \t\t PTR \t\t CLASS \t STATE\n");

    for(i = 0; i < max_entries ; i++){

        if (FBE_CHECK_CONTROL_C())
        {
            fbe_sep_offsets_clear();
            return;
        }

        /* The GetFieldData function returns the value of a member in a structure */
        addr = topology_object_tbl_ptr + fbe_sep_offsets.object_status_offset;
        FBE_READ_MEMORY(addr, &object_status,
                        sizeof(fbe_topology_object_status_t));

        if(object_status == FBE_TOPOLOGY_OBJECT_STATUS_READY){

            addr = topology_object_tbl_ptr + fbe_sep_offsets.control_handle_offset;
            FBE_READ_MEMORY(addr, &control_handle_ptr, sizeof(fbe_dbgext_ptr));

            csx_dbg_ext_print("%d (0x%x)\t %llx \t", i, i, (unsigned long long)control_handle_ptr);
            
            fbe_base_object_debug_trace_class_id(pp_ext_module_name, control_handle_ptr, fbe_debug_trace_func, NULL);
            csx_dbg_ext_print("\t");

            fbe_base_object_debug_trace_state(pp_ext_module_name, control_handle_ptr, fbe_debug_trace_func, NULL);
            csx_dbg_ext_print("\n");
        }
        
        topology_object_tbl_ptr += topology_object_table_entry_size;
        
    }
    fbe_sep_offsets_clear();
} // end of !pptopls extension

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
    csx_dbg_ext_vsnprintf(string_to_print, (sizeof(string_to_print) - 1), fmt, arguments);
    va_end(arguments);

    /* Finally, print out the string.
     */
    csx_dbg_ext_print("%s", string_to_print);
    return;
}

#pragma data_seg ("EXT_HELP$4pptopdisplayobjects")
static char CSX_MAYBE_UNUSED usageMsg_pptopdisplayobjects[] =
"!pptopdisplayobjects\n"
"  Display objects in the system.\n"
"  By default it lists verbose output for all objects in the system\n"
"  -io - displays I/Os queued to the object. (Must be first option when used).\n"
"  -s When using -io, display a brief summary for I/Os instead of verbose display.\n"
"  -c <class id> displays just this class.\n"
"  -o <object id> display just this object id.\n"
"  -l (or -log) - displays a parity group's geometry and full parity write log info.\n"
"  -p <sep | phy> select objects from this package.\n";
#pragma data_seg ()

CSX_DBG_EXT_DEFINE_CMD(pptopdisplayobjects, "pptopdisplayobjects")
{
    fbe_class_id_t filter_class = FBE_CLASS_ID_INVALID;
    fbe_object_id_t filter_object = FBE_OBJECT_ID_INVALID;
    fbe_u32_t arg_number = 1; /* arg 0 is the cmd, arg 1 is the first arg, etc. */
    fbe_bool_t b_display_io = FBE_FALSE;
    fbe_char_t *str = NULL;
    fbe_bool_t b_parity_write_log = FBE_FALSE;
    fbe_bool_t b_summarize = FBE_FALSE;

    str = strtok((char*)args, " \t");
    while (str != NULL)
    {
        if(strncmp(str, "-io", 3) == 0)
        {
            b_display_io = FBE_TRUE;
        }
        if (strncmp(str, "-c", 2) == 0)
        {
            str = strtok(NULL, " \t");
            csx_dbg_ext_print("filter by class id: %d \n", (int)GetArgument64(str, arg_number));
            filter_class = GetArgument64(str, arg_number);
        }
        else if (strncmp(str, "-o", 2) == 0)
        {
            str = strtok(NULL, " \t");
            csx_dbg_ext_print("filter by object id: %d \n", (int)GetArgument64(str, arg_number));
            filter_object = (fbe_object_id_t) GetArgument64(str, arg_number);
        }
        else if (strncmp(str, "-p", 2) == 0)
        {
            str = strtok(NULL, " \t");
            if (strncmp(str, "sep", 3) == 0)
            {
                pp_ext_module_name = "sep";
            }
            else if (strncmp(str, "phy", 3) == 0)
            {
		pp_ext_module_name = fbe_get_module_name();
            }
            else
            {
                csx_dbg_ext_print("unknown package: %s\n", str);
            }
            csx_dbg_ext_print("pp_ext_module_name = \"%s\"  \n", pp_ext_module_name);
        }
        if(strncmp(str, "-l", 2) == 0)
        {
            b_parity_write_log = FBE_TRUE;
        }
        if(strncmp(str, "-s", 2) == 0)
        {
            b_summarize = FBE_TRUE;
        }
        str = strtok(NULL, " \t");
    }
       
    /* Do not display terminator queues if active queue display is disabled 
     * using !disable_active_queue_display' debug macro
     */
    if(get_active_queue_display_flag() == FBE_FALSE)
    {
        b_display_io = FBE_FALSE;
    }

    fbe_topology_print_object_by_id(pp_ext_module_name, 
                                    filter_class, 
                                    filter_object, 
                                    b_parity_write_log,
                                    b_display_io,
                                    b_summarize,
                                    fbe_debug_trace_func,
                                    NULL,
                                    0);

    return;
}

void 
fbe_topology_print_object_by_id(const fbe_u8_t * pp_ext_module_name,
                          fbe_class_id_t filter_class,
                          fbe_object_id_t filter_object,
                          fbe_bool_t b_parity_write_log,
                          fbe_bool_t b_display_io,
                          fbe_bool_t b_summary,
                          fbe_trace_func_t trace_func,
                          fbe_trace_context_t trace_context,
                          fbe_u32_t spaces_to_indent)
{
    fbe_dbgext_ptr topology_object_table_ptr = 0;
    ULONG topology_object_table_entry_size;
    int i = 0;
    fbe_status_t status = FBE_STATUS_OK;
    ULONG64 max_toplogy_entries_p = 0;
    fbe_u32_t max_entries;
    fbe_dbgext_ptr topology_object_tbl_ptr = 0;
    fbe_u32_t ptr_size;
   

    status = fbe_debug_get_ptr_size(pp_ext_module_name, &ptr_size);
    if (status != FBE_STATUS_OK) 
    {
        trace_func(trace_context,"%s unable to get ptr size status: %d\n", __FUNCTION__, status);
        return ; 
    }

    FBE_GET_EXPRESSION(pp_ext_module_name, max_supported_objects, &max_toplogy_entries_p);
    FBE_READ_MEMORY(max_toplogy_entries_p, &max_entries, sizeof(fbe_u32_t));

    FBE_GET_EXPRESSION(pp_ext_module_name, topology_object_table, &topology_object_table_ptr);
    FBE_READ_MEMORY(topology_object_table_ptr, &topology_object_tbl_ptr, ptr_size);
    fbe_trace_indent(trace_func, trace_context, 4 /*spaces_to_indent*/);
    trace_func(trace_context,"\t topology_object_table_ptr %llx\n", (unsigned long long)topology_object_tbl_ptr);

    /* Get the size of the topology entry.
     */
    FBE_GET_TYPE_SIZE(pp_ext_module_name, topology_object_table_entry_s, &topology_object_table_entry_size);
    fbe_trace_indent(trace_func, trace_context, 4 /*spaces_to_indent*/);
    trace_func(trace_context,"\t topology_object_table_entry_size %d \n", topology_object_table_entry_size);

    if(topology_object_tbl_ptr == 0){
        return;
    }

    if(!fbe_sep_offsets.is_fbe_sep_offsets_set)
    {
        fbe_sep_offsets_set(pp_ext_module_name);
    }

    /* we don't have to loop through all objects if we know which object we wanted */
    if (filter_object != FBE_OBJECT_ID_INVALID)
    {
        if (filter_object >= max_entries)
        {
            trace_func(trace_context,"\t object id %d is greater than table size %d\n", filter_object, max_entries);
        }
        else
        {
            topology_object_tbl_ptr += filter_object*topology_object_table_entry_size;
            fbe_topology_print_object(pp_ext_module_name, 
                                  topology_object_tbl_ptr,
                                  filter_class,
                                  filter_object,
                                  b_parity_write_log,
                                  b_display_io,
                                  b_summary,
                                  fbe_sep_offsets.object_status_offset,
                                  fbe_sep_offsets.control_handle_offset,
                                  fbe_sep_offsets.class_id_offset,
                                  trace_func,
                                  trace_context,
                                  spaces_to_indent);

        }
        fbe_sep_offsets_clear();
        return;
    }
    for(i = 0; i < max_entries ; i++)
    {
        if (FBE_CHECK_CONTROL_C())
        {
            fbe_sep_offsets_clear();
            return;
        }

        fbe_topology_print_object(pp_ext_module_name, 
                                  topology_object_tbl_ptr,
                                  filter_class,
                                  i,
                                  b_parity_write_log,
                                  b_display_io,
                                  b_summary,
                                  fbe_sep_offsets.object_status_offset,
                                  fbe_sep_offsets.control_handle_offset,
                                  fbe_sep_offsets.class_id_offset,
                                  trace_func,
                                  trace_context,
                                  spaces_to_indent);


        topology_object_tbl_ptr += topology_object_table_entry_size;
    }
    fbe_sep_offsets_clear();
    return;
}

fbe_status_t 
fbe_topology_print_object(const fbe_u8_t * pp_ext_module_name,
                          fbe_dbgext_ptr topology_object_tbl_ptr,
                          fbe_class_id_t filter_class,
                          fbe_object_id_t filter_object,
                          fbe_bool_t b_parity_write_log,
                          fbe_bool_t b_display_io,
                          fbe_bool_t b_summary,
                          fbe_u32_t object_status_offset,
                          fbe_u32_t control_handle_offset,
                          fbe_u32_t class_id_offset,
                          fbe_trace_func_t trace_func,
                          fbe_trace_context_t trace_context,
                          fbe_u32_t spaces_to_indent)
{
    fbe_topology_object_status_t  object_status;
    fbe_dbgext_ptr control_handle_ptr = 0;
    fbe_dbgext_ptr addr;

    object_status = 0;

    /* Fetch the object's topology status.
     */
    addr = topology_object_tbl_ptr + object_status_offset;
    FBE_READ_MEMORY(addr, &object_status,
                    sizeof(fbe_topology_object_status_t));

    /* If the object's topology status is ready, then get the
     * topology status. 
     */
    if (object_status == FBE_TOPOLOGY_OBJECT_STATUS_READY)
    {
        fbe_class_id_t class_id;

        /* Fetch the control handle, which is the pointer to the object.
         */
        addr = topology_object_tbl_ptr + control_handle_offset;
        FBE_READ_MEMORY(addr, &control_handle_ptr, sizeof(fbe_dbgext_ptr));

        addr = control_handle_ptr + class_id_offset;
        FBE_READ_MEMORY(addr, &class_id, sizeof(fbe_class_id_t));


         /* Display information on the object if 
         * 1) the class we are filtering by matches the object's class. 
         * or 
         * 2) the user did not select a class id, which means to display all objects.
         */
        if ((filter_class == class_id) || 
            (filter_class == FBE_CLASS_ID_INVALID))
        {
            switch (class_id)
            {
                case FBE_CLASS_ID_LOGICAL_DRIVE:
					trace_func(trace_context, "Logical Drive(0x%x): 0x%llx \n", filter_object, (unsigned long long)control_handle_ptr);
                    fbe_logical_drive_debug_trace(pp_ext_module_name, control_handle_ptr, trace_func, trace_context);
                    trace_func(trace_context,"\n");
					trace_func(trace_context,"------------------------------------------------------------------------------------");
					trace_func(trace_context,"------------------------------------------------------------------------------------\n\n");
                    break;

                //case FBE_CLASS_ID_BASE_ENCLOSURE:
                case FBE_CLASS_ID_SAS_BULLET_ENCLOSURE:
                case FBE_CLASS_ID_SAS_VIPER_ENCLOSURE:
                case FBE_CLASS_ID_SAS_MAGNUM_ENCLOSURE:
                case FBE_CLASS_ID_SAS_CITADEL_ENCLOSURE:
                case FBE_CLASS_ID_SAS_BUNKER_ENCLOSURE:
                case FBE_CLASS_ID_SAS_DERRINGER_ENCLOSURE:
                case FBE_CLASS_ID_SAS_VOYAGER_ICM_ENCLOSURE:
                case FBE_CLASS_ID_SAS_VIKING_IOSXP_ENCLOSURE:
                case FBE_CLASS_ID_SAS_CAYENNE_IOSXP_ENCLOSURE:
                case FBE_CLASS_ID_SAS_NAGA_IOSXP_ENCLOSURE:
                case FBE_CLASS_ID_SAS_FALLBACK_ENCLOSURE:
                case FBE_CLASS_ID_SAS_BOXWOOD_ENCLOSURE:
                case FBE_CLASS_ID_SAS_KNOT_ENCLOSURE:
                case FBE_CLASS_ID_SAS_PINECONE_ENCLOSURE:
                case FBE_CLASS_ID_SAS_STEELJAW_ENCLOSURE:
                case FBE_CLASS_ID_SAS_RAMHORN_ENCLOSURE:
                case FBE_CLASS_ID_SAS_ANCHO_ENCLOSURE:
                case FBE_CLASS_ID_SAS_CALYPSO_ENCLOSURE:
                case FBE_CLASS_ID_SAS_RHEA_ENCLOSURE:
                case FBE_CLASS_ID_SAS_MIRANDA_ENCLOSURE:
                case FBE_CLASS_ID_SAS_TABASCO_ENCLOSURE:
                case FBE_CLASS_ID_ESES_ENCLOSURE:
                    /* Display the pointer to the object. */
                    trace_func(trace_context,"ESES Enclosure(0x%x): 0x%llx \n", filter_object, (unsigned long long)control_handle_ptr);
                    fbe_eses_enclosure_debug_basic_info(pp_ext_module_name, control_handle_ptr, trace_func, trace_context);
                    trace_func(trace_context,"\n");
                    trace_func(trace_context,"------------------------------------------------------------------------------------");
                    trace_func(trace_context,"------------------------------------------------------------------------------------\n\n");
                    break;

                case FBE_CLASS_ID_SAS_ENCLOSURE:
                    /* Display the pointer to the object.*/
                    trace_func(trace_context,"SAS Enclosure(0x%x): 0x%llx \n",filter_object, (unsigned long long)control_handle_ptr);
                    fbe_sas_enclosure_debug_basic_info(pp_ext_module_name, control_handle_ptr, trace_func, trace_context);
                    trace_func(trace_context,"\n");
                    trace_func(trace_context,"------------------------------------------------------------------------------------");
                    trace_func(trace_context,"------------------------------------------------------------------------------------\n\n");
                    break;
                case FBE_CLASS_ID_SAS_PMC_PORT:
                    /* Display the pointer to the object.*/
                    trace_func(trace_context,"Port Object(0x%x): 0x%llx \n",filter_object, (unsigned long long)control_handle_ptr);
                    fbe_sas_pmc_port_debug_trace(pp_ext_module_name, control_handle_ptr, trace_func, trace_context);
                    trace_func(trace_context,"\n");
                    trace_func(trace_context,"------------------------------------------------------------------------------------");
                    trace_func(trace_context,"------------------------------------------------------------------------------------\n\n");
                    break;
		case FBE_CLASS_ID_FC_PORT:
                    /* Display the pointer to the object.*/
                    trace_func(trace_context,"Port Object(0x%x): 0x%llx \n",filter_object, (unsigned long long)control_handle_ptr);
                    fbe_fc_port_debug_trace(pp_ext_module_name, control_handle_ptr, trace_func, trace_context);
                    trace_func(trace_context,"\n");
                    trace_func(trace_context,"------------------------------------------------------------------------------------");
                    trace_func(trace_context,"------------------------------------------------------------------------------------\n\n");
                    break;
		case FBE_CLASS_ID_ISCSI_PORT:
                    /* Display the pointer to the object.*/
                    trace_func(trace_context,"Port Object(0x%x): 0x%llx \n",filter_object, (unsigned long long)control_handle_ptr);
                    fbe_iscsi_port_debug_trace(pp_ext_module_name, control_handle_ptr, trace_func, trace_context);
                    trace_func(trace_context,"\n");
                    trace_func(trace_context,"------------------------------------------------------------------------------------");
                    trace_func(trace_context,"------------------------------------------------------------------------------------\n\n");
                    break;
                                            
                case FBE_CLASS_ID_BASE_PHYSICAL_DRIVE:
                case FBE_CLASS_ID_SAS_PHYSICAL_DRIVE:
                case FBE_CLASS_ID_SAS_FLASH_DRIVE:
                case FBE_CLASS_ID_SATA_PADDLECARD_DRIVE:
                case FBE_CLASS_ID_SATA_PHYSICAL_DRIVE:
                case FBE_CLASS_ID_SATA_FLASH_DRIVE:
                    trace_func(trace_context, "Physical Drive(0x%x): 0x%llx \n",filter_object, (unsigned long long)control_handle_ptr);
                    fbe_base_physical_drive_debug_trace(pp_ext_module_name, control_handle_ptr, trace_func, trace_context);
                    trace_func(trace_context,"\n");
                    trace_func(trace_context,"------------------------------------------------------------------------------------");
                    trace_func(trace_context,"------------------------------------------------------------------------------------\n\n");
                    break;

                    /*! @todo This is temporary here until we get a sep debug library.
                     */
                case FBE_CLASS_ID_PARITY:
                    fbe_trace_indent(trace_func, trace_context, 4);
                    trace_func(trace_context, "Raid Group(0x%x): 0x%llx ", filter_object, (unsigned long long)control_handle_ptr);
                    if (!b_parity_write_log)
                    {
                        /* Display the raid group.
                         */
                        fbe_raid_group_debug_trace(pp_ext_module_name, control_handle_ptr, trace_func, trace_context, NULL /* No field info. */, 2    /* spaces to indent */);
                        if (b_display_io)
                        {
                            /* Display all the packets, iots and siots on the terminator queue.
                             */
                            fbe_raid_group_debug_display_terminator_queue(pp_ext_module_name, control_handle_ptr, 
                                                                          trace_func, trace_context, 2    /* spaces to indent */,
                                                                          b_summary);
                        }
                    }
                    else
                    {
                        
                        /* Display the raid group geometry with all write log headers even if invalid.
                         */
                        trace_func(trace_context, " geometry with verbose write log:\n");
                        fbe_trace_indent(trace_func, trace_context, 2);
                        fbe_raid_library_debug_print_full_raid_group_geometry(pp_ext_module_name, control_handle_ptr,
                                                                              trace_func, trace_context, 2);
                        trace_func(trace_context, "\n");
                    }
                    trace_func(trace_context,"------------------------------------------------------------------------------------");
                    trace_func(trace_context,"------------------------------------------------------------------------------------\n\n");
                    break;
                                            
                case FBE_CLASS_ID_STRIPER:
                case FBE_CLASS_ID_MIRROR:
                    fbe_trace_indent(trace_func, trace_context, 4);
                    trace_func(trace_context, "Raid Group(0x%x): 0x%llx ", filter_object, (unsigned long long)control_handle_ptr);
                    fbe_raid_group_debug_trace(pp_ext_module_name, control_handle_ptr, trace_func, trace_context, NULL /* No field info. */, 2    /* spaces to indent */);

                    if (b_display_io)
                    {
                        /* Display all the packets, iots and siots on the terminator queue.
                         */
                        fbe_raid_group_debug_display_terminator_queue(pp_ext_module_name, control_handle_ptr, 
                                                                      trace_func, trace_context, 2    /* spaces to indent */,
                                                                      b_summary);
                    }
                    trace_func(trace_context,"------------------------------------------------------------------------------------");
                    trace_func(trace_context,"------------------------------------------------------------------------------------\n\n");
                    break;
                                            
                    /* Add new classes here.
                     */
                case FBE_CLASS_ID_VIRTUAL_DRIVE:
                    fbe_trace_indent(trace_func, trace_context, 4);
                    trace_func(trace_context, "virtual Drive(0x%x): 0x%llx ", filter_object,(unsigned long long)control_handle_ptr);
                    fbe_virtual_drive_debug_trace(pp_ext_module_name, control_handle_ptr, trace_func, trace_context, 2);
                    trace_func(trace_context,"\n");
                    if (b_display_io)
                    {
                        /* Display all the packets on the terminator queue.
                         */
                        fbe_virtual_drive_debug_display_terminator_queue(pp_ext_module_name, control_handle_ptr, 
                                                                         trace_func, trace_context, 2    /* spaces to indent */);
                    }
                    trace_func(trace_context,"------------------------------------------------------------------------------------");
                    trace_func(trace_context,"------------------------------------------------------------------------------------\n\n");
                    break;
              
                case FBE_CLASS_ID_PROVISION_DRIVE:
                    fbe_trace_indent(trace_func, trace_context, 4 /*spaces_to_indent*/);
                    trace_func(trace_context, "provision Drive(0x%x): 0x%llx ", filter_object,(unsigned long long)control_handle_ptr);
                    fbe_provision_drive_debug_trace(pp_ext_module_name, control_handle_ptr, trace_func, trace_context, 2);
                    trace_func(trace_context,"\n");
                    if (b_display_io)
                    {
                        /* Display all the packets on the terminator queue.
                         */
                        fbe_provision_drive_debug_display_terminator_queue(pp_ext_module_name, control_handle_ptr, 
                                                                           trace_func, trace_context, 2    /* spaces to indent */);
                    }
                    trace_func(trace_context,"------------------------------------------------------------------------------------");
                    trace_func(trace_context,"------------------------------------------------------------------------------------\n\n");
                    break;
                case FBE_CLASS_ID_LUN:
                case FBE_CLASS_ID_EXTENT_POOL_LUN:
                case FBE_CLASS_ID_EXTENT_POOL_METADATA_LUN:
					fbe_trace_indent(trace_func, trace_context, 4);
					trace_func(trace_context, "Lun object(0x%x): 0x%llx \n", filter_object,(unsigned long long)control_handle_ptr);
                    fbe_lun_debug_trace(pp_ext_module_name, control_handle_ptr, trace_func, trace_context, 2);
                    trace_func(trace_context,"\n");
                    if (b_display_io)
                    {
                        fbe_lun_debug_display_terminator_queue(pp_ext_module_name, control_handle_ptr, 
                                                               trace_func, trace_context, 2    /* spaces to indent */);
                    }
                    trace_func(trace_context,"------------------------------------------------------------------------------------");
                    trace_func(trace_context,"------------------------------------------------------------------------------------\n\n");
                    break;

                case FBE_CLASS_ID_EXTENT_POOL:
					fbe_trace_indent(trace_func, trace_context, 4);
					trace_func(trace_context, "Extent Pool(0x%x): 0x%llx ", filter_object, (unsigned long long)control_handle_ptr);
                    fbe_extent_pool_debug_trace(pp_ext_module_name, control_handle_ptr, trace_func, trace_context, NULL /* No field info. */, 2    /* spaces to indent */);

                    if (b_display_io)
                    {
                        /* Display all the packets, iots and siots on the terminator queue.
                         */
                        fbe_raid_group_debug_display_terminator_queue(pp_ext_module_name, control_handle_ptr, 
                                                                      trace_func, trace_context, 2    /* spaces to indent */,
                                                                      b_summary);
                    }
					trace_func(trace_context,"------------------------------------------------------------------------------------");
					trace_func(trace_context,"------------------------------------------------------------------------------------\n\n");
                    break;

                case FBE_CLASS_ID_ENCL_MGMT:
                    trace_func(trace_context, "Enclosure Mgmt(0x%x): 0x%llx \n", filter_object, (unsigned long long)control_handle_ptr);
                    fbe_encl_mgmt_debug_trace(pp_ext_module_name,
                                              control_handle_ptr,
                                              trace_func, trace_context);
                    trace_func(trace_context,"------------------------------------------------------------------------------------");
                    trace_func(trace_context,"------------------------------------------------------------------------------------\n\n");
                    break;
                case FBE_CLASS_ID_SPS_MGMT:
                    trace_func(trace_context, "SPS Mgmt(0x%x): 0x%llx \n",filter_object, (unsigned long long)control_handle_ptr);
                    fbe_sps_mgmt_debug_trace(pp_ext_module_name,
                                             control_handle_ptr,
                                             trace_func, trace_context);
                    trace_func(trace_context,"------------------------------------------------------------------------------------");
                    trace_func(trace_context,"------------------------------------------------------------------------------------\n\n");
                    break;
                case FBE_CLASS_ID_DRIVE_MGMT:
                    trace_func(trace_context, "Drive Mgmt(0x%x): 0x%llx \n", filter_object, (unsigned long long)control_handle_ptr);
                    fbe_drive_mgmt_debug_trace(pp_ext_module_name,
                                               control_handle_ptr,
                                               trace_func, trace_context);
                    trace_func(trace_context,"------------------------------------------------------------------------------------");
                    trace_func(trace_context,"------------------------------------------------------------------------------------\n\n");
                    break;
                case FBE_CLASS_ID_MODULE_MGMT:
                    trace_func(trace_context, "Modules Mgmt(0x%x): 0x%llx \n",filter_object, (unsigned long long)control_handle_ptr);
                    fbe_modules_mgmt_debug_trace(pp_ext_module_name,
                                                 control_handle_ptr,
                                                 trace_func, trace_context);
                    trace_func(trace_context,"------------------------------------------------------------------------------------");
                    trace_func(trace_context,"------------------------------------------------------------------------------------\n\n");
                    break;
                case FBE_CLASS_ID_PS_MGMT:
                    trace_func(trace_context, "PS Mgmt(0x%x): 0x%llx \n", filter_object, (unsigned long long)control_handle_ptr);
                    fbe_ps_mgmt_debug_trace(pp_ext_module_name,
                                            control_handle_ptr,
                                            trace_func, trace_context);
                    trace_func(trace_context,"------------------------------------------------------------------------------------");
                    trace_func(trace_context,"------------------------------------------------------------------------------------\n\n");
                    break;          
                case FBE_CLASS_ID_BOARD_MGMT:
                    trace_func(trace_context, "BOARD Mgmt(0x%x): 0x%llx \n",filter_object, (unsigned long long)control_handle_ptr);
                    fbe_board_mgmt_debug_trace(pp_ext_module_name,
                                               control_handle_ptr,
                                               trace_func, trace_context);
                    trace_func(trace_context,"------------------------------------------------------------------------------------");
                    trace_func(trace_context,"------------------------------------------------------------------------------------\n\n");
                    break;
                case FBE_CLASS_ID_COOLING_MGMT:
                    trace_func(trace_context, "COOLING Mgmt(0x%x): 0x%llx \n",filter_object, (unsigned long long)control_handle_ptr);
                    fbe_cooling_mgmt_debug_trace(pp_ext_module_name,
                                               control_handle_ptr,
                                               trace_func, trace_context);
                    trace_func(trace_context,"------------------------------------------------------------------------------------");
                    trace_func(trace_context,"------------------------------------------------------------------------------------\n\n");
                    break;
                default:
                    /* There is no handling for this class yet.
                     * Display some generic information about it. 
                     */
                    trace_func(trace_context,"object id: %d object ptr: 0x%llx ", filter_object, (unsigned long long)control_handle_ptr);

                    trace_func(trace_context,"class id: ");
                    fbe_base_object_debug_trace_class_id(pp_ext_module_name, control_handle_ptr, trace_func, trace_context);
                    trace_func(trace_context," lifecycle state: ");
                    fbe_base_object_debug_trace_state(pp_ext_module_name, control_handle_ptr, trace_func, trace_context);
                    trace_func(trace_context,"\n");
                    break;
            }
        }
    } /* End if object's topology status is ready. */

    return FBE_STATUS_OK;
}


/*!**************************************************************
 * system_bg_service_flag()
 ****************************************************************
 * @brief
 *  Display base_config_control_system_bg_service_flag
 *
 ****************************************************************/

#pragma data_seg ("EXT_HELP$4system_bg_service_flag")
static char CSX_MAYBE_UNUSED usageMsg_system_bg_service_flag[] =
"!system_bg_service_flag\n"
"  Shows the shows system background services enabled flag\n";
#pragma data_seg ()

CSX_DBG_EXT_DEFINE_CMD(system_bg_service_flag, "system_bg_service_flag")
{
    ULONG64 system_bgo_flag_p = 0;
    fbe_base_config_control_system_bg_service_flags_t system_bgo_flag = 0;

    /*it only works for SEP package*/
    pp_ext_module_name = "sep";
    FBE_GET_EXPRESSION(pp_ext_module_name, base_config_control_system_bg_service_flag, &system_bgo_flag_p);
    FBE_READ_MEMORY(system_bgo_flag_p, &system_bgo_flag, sizeof(fbe_base_config_control_system_bg_service_flags_t));
    csx_dbg_ext_print("base_config_control_system_bg_service_flag = 0x%llx \n", system_bgo_flag);
} 
/******************************************
 * end system_bg_service_flag
 ******************************************/
