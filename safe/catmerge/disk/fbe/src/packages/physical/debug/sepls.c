/***************************************************************************
 *  Copyright (C)  EMC Corporation 2010-11
 *  All rights reserved.
 *  Licensed material - property of EMC Corporation. * All rights reserved.
 ***************************************************************************/

/*!**************************************************************************
 * @file sepls.c
 ***************************************************************************
 *
 * @brief
 *  This file contains debugging command for sepls.
 *  sepls command is used to display the various sep objects and their states.
.*
 * @version
 *   10-May-2011:  Created. Omer Miranda
 *
 ***************************************************************************/

#include "pp_dbgext.h"
#include "fbe/fbe_topology_interface.h"
#include "fbe_base_object_debug.h"
#include "fbe/fbe_class.h"
#include "fbe/fbe_devices.h"
#include "fbe/fbe_transport.h"
#include "fbe_base_environment_debug.h"
#include "fbe_lun_debug.h"
#include "fbe_sepls_debug.h"
#include "fbe_raid_group_debug.h"
#include "fbe_provision_drive_debug.h"
#include "fbe_virtual_drive_debug.h"
#include "fbe_topology_debug.h"
#include "fbe_database_config_tables.h"
#include "fbe_raid_library.h"
#include "fbe/fbe_dbgext.h"

fbe_bool_t sys_vd_displayed = FBE_FALSE;		/* Flag to indicate whether system objects have been displayed */
fbe_u32_t current_sys_vd_index = 1;                  /* Which system VD's have been displayed */
fbe_u32_t current_vd_index = 0;                         /* Which VD's have been displayed */
fbe_object_id_t pvd_displayed[FBE_MAX_SPARE_OBJECTS];	/* List of displayed PVD objects */
/* List of system VDs displayed */
fbe_object_id_t sys_vd_ids[FBE_RAID_GROUP_MAX_DISK_ARRAY_WIDTH];
fbe_object_id_t vd_ids[FBE_MAX_SPARE_OBJECTS];
fbe_block_size_t exported_block_size;

/* saved offsets per sepls invocation */
fbe_sep_offsets_t fbe_sepls_offsets;

static void fbe_sepls_offsets_clear(void);
static void fbe_sepls_offsets_set(void);

/* ***************************************************************************
 *
 * @brief
 *  sepls - Display various SEP objects and their states.
 *
 *  Arguments: None
 *  Return Value: None
 * 
 * @version
 *   10-May-2011:  Created. Omer Miranda
 *
 ***************************************************************************/

#pragma data_seg ("EXT_HELP$4sepls")
static char CSX_MAYBE_UNUSED usageMsg_sepls[] =
"!sepls\n"
"  Displays SEP objects info.\n"
"  By default we show the LUN, RG SEP objects, and the configured hot spares.\n"
"  -allsep - This option displays all the sep objects\n"
"  -lun - This option displays all the lun objects\n"
"  -rg - This option displays all the RG objects\n"
"  -vd - This option displays all the VD objects\n"
"  -pvd - This option displays all the PVD objects\n"
"  -level <1,2,3,4> - This option display objects upto the depth level specified\n"
"       1 - Will display only RG objects\n"
"       2 - Will display LUN and RG objects\n"
"       3 - Will display LUN, RG and VD objects\n"
"       4 - Will display LUN, RG, VD and PVD objects\n";
#pragma data_seg ()

/* ***************************************************************************
 *
 * @brief
 *  sepls - Display various SEP objects and their states.
 *
 *  Arguments: None
 *  Return Value: None
 * 
 * @version
 *   10-May-2011:  Created. Omer Miranda
 *
 ***************************************************************************/
CSX_DBG_EXT_DEFINE_CMD(sepls, "sepls")
{
    fbe_char_t *str = NULL;
    fbe_dbgext_ptr topology_object_table_ptr = 0;
    fbe_topology_object_status_t  object_status = 0;
    fbe_dbgext_ptr control_handle_ptr = 0;
    fbe_u32_t topology_object_table_entry_size = 0;
    fbe_u32_t obj_index = 0;
    fbe_class_id_t class_id = FBE_CLASS_ID_INVALID;
    fbe_object_id_t object_id;
    fbe_bool_t b_display_allsep = FBE_FALSE;
    fbe_bool_t b_display_default = FBE_TRUE;
    ULONG64 max_toplogy_entries_p = 0;
    fbe_u32_t max_entries;
    fbe_dbgext_ptr topology_object_tbl_ptr = 0;
    fbe_dbgext_ptr head_topology_object_tbl_ptr = 0;
    fbe_u32_t display_format = FBE_SEPLS_DISPLAY_OBJECTS_NONE;
    fbe_u32_t level = 0;
    fbe_u32_t ptr_size;
    fbe_u32_t display_object;
    fbe_u32_t display_level;
    fbe_dbgext_ptr addr;
    fbe_status_t status = FBE_STATUS_OK;
    
    const fbe_u8_t * module_name = "sep"; 

    status = fbe_debug_get_ptr_size(module_name, &ptr_size);
    if (status != FBE_STATUS_OK) 
    {
        fbe_debug_trace_func(NULL, "%s unable to get ptr size status: %d\n", __FUNCTION__, status);
        return ; 
    }


    str = strtok((char*)args, " \t");
    /* Display all the objects */
    if (strlen(args) && strncmp(args, "-allsep", 7) == 0)
    {
        b_display_allsep = FBE_TRUE;
        b_display_default = FBE_FALSE;
        /* Increment past the allsep flag.
         */
        args += FBE_MIN(8, strlen(args));
    }
    /* Display only LUN objects */
    else if (strlen(args) && strncmp(args, "-lun", 4) == 0)
    {
        display_format = FBE_SEPLS_DISPLAY_ONLY_LUN_OBJECTS;
        b_display_allsep = FBE_FALSE;
        b_display_default = FBE_FALSE;
        args += FBE_MIN(5, strlen(args));
    }
    /* Display only RG objects */
    else if (strlen(args) && strncmp(args, "-rg", 3) == 0)
    {
        display_format = FBE_SEPLS_DISPLAY_ONLY_RG_OBJECTS;
        b_display_allsep = FBE_FALSE;
        b_display_default = FBE_FALSE;
        args += FBE_MIN(4, strlen(args));
    }
    /* Display only VD objects */
    else if (strlen(args) && strncmp(args, "-vd", 3) == 0)
    {
        display_format = FBE_SEPLS_DISPLAY_ONLY_VD_OBJECTS;
        b_display_allsep = FBE_FALSE;
        b_display_default = FBE_FALSE;
        args += FBE_MIN(4, strlen(args));
    }
    /* Display only PVD objects */
    else if (strlen(args) && strncmp(args, "-pvd", 4) == 0)
    {
        display_format = FBE_SEPLS_DISPLAY_ONLY_PVD_OBJECTS;
        b_display_allsep = FBE_FALSE;
        b_display_default = FBE_FALSE;
        args += FBE_MIN(5, strlen(args));
    }
    else if (strlen(args) && strncmp(args, "-level", 6) == 0)
    {
        str = strtok(NULL, " \t");
        if(str != NULL)
        {
            level = strtoul(str, 0, 0);
            if(level < 1 || level > 4)
            {
                fbe_debug_trace_func(NULL, "\t INVALID Level \n");
                return;
            }
            b_display_allsep = FBE_FALSE;
            b_display_default = FBE_FALSE;
            /* Display only RG objects */
            if(level == 1)
            {
                /* Level 1 will display only RG objects */
                display_format = FBE_SEPLS_DISPLAY_LEVEL_1;
            }
            /* Display only LUN and RG objects */
            else if(level == 2)
            {
                /* Level 2 will display only LUN and RG objects */
                display_format = FBE_SEPLS_DISPLAY_LEVEL_2;
            }
            /* Display LUN, RG and VD objects */
            else if(level == 3)
            {
                /* Level 3 will display  LUN, RG and VD objects */
                display_format = FBE_SEPLS_DISPLAY_LEVEL_3;
            }
            /* Display LUN, RG, VD and PVD objects */
            else if(level == 4)
            {
                /* Level 4 will display  LUN, RG, VD and PVD objects */
                display_format = FBE_SEPLS_DISPLAY_LEVEL_4;
            }
        }
    }

     /* Get the topology table. */
    FBE_GET_EXPRESSION(module_name, topology_object_table, &topology_object_table_ptr);
    FBE_READ_MEMORY(topology_object_table_ptr, &topology_object_tbl_ptr, ptr_size);
    if (topology_object_tbl_ptr == 0)
    {
        fbe_debug_trace_func(NULL, "\t topology_object_table_is not available \n");
        return;
    }
    head_topology_object_tbl_ptr = topology_object_tbl_ptr;

    if(!fbe_sep_offsets.is_fbe_sep_offsets_set)
    {
        fbe_sep_offsets_set(module_name);
    }

    FBE_GET_EXPRESSION(pp_ext_module_name, max_supported_objects, &max_toplogy_entries_p);
    FBE_READ_MEMORY(max_toplogy_entries_p, &max_entries, sizeof(fbe_u32_t));

    /* Get the size of the topology entry. */
    FBE_GET_TYPE_SIZE(module_name, topology_object_table_entry_s, &topology_object_table_entry_size);


    fbe_debug_trace_func(NULL, "Type    Obj ID       ID    Object Info    Lifecycle State  Downstream                        Drives\n");
    fbe_debug_trace_func(NULL, "                                                           Objects\n");
    fbe_debug_trace_func(NULL, "------------------------------------------------------------------------------------------------------------------------------------------------------\n");
    for(obj_index = 0; obj_index < max_entries ; obj_index++)
    {
        if (FBE_CHECK_CONTROL_C())
        {
            fbe_sep_offsets_clear();
            return;
        }

        /* Fetch the object's topology status. */
        addr = topology_object_tbl_ptr + fbe_sep_offsets.object_status_offset;
        FBE_READ_MEMORY(addr, &object_status,
                        sizeof(fbe_topology_object_status_t));

        /* Need to display the objects even if they are not in the ready state
         */
        if (object_status == FBE_TOPOLOGY_OBJECT_STATUS_READY ||
            object_status == FBE_TOPOLOGY_OBJECT_STATUS_EXIST ||
            object_status == FBE_TOPOLOGY_OBJECT_STATUS_RESERVED)
        {

            /* Fetch the control handle, which is the pointer to the object. */
            addr = topology_object_tbl_ptr + fbe_sep_offsets.control_handle_offset;

            FBE_READ_MEMORY(addr, &control_handle_ptr, sizeof(fbe_dbgext_ptr));

            addr = control_handle_ptr + fbe_sep_offsets.class_id_offset;
            FBE_READ_MEMORY(addr, &class_id, sizeof(fbe_class_id_t));

            /* Check to see if there are any RGs in the topology.
             * From the RG we frist display the LUN's data using the RGs client's list.
             * then we display the RG details.
             * Next we display the VD details using the RG's server list.
             */
            if(class_id == FBE_CLASS_ID_RAID_GROUP ||
               class_id == FBE_CLASS_ID_MIRROR ||
               class_id == FBE_CLASS_ID_STRIPER ||
               class_id == FBE_CLASS_ID_PARITY)
            {
                fbe_sepls_rg_info_trace(module_name,control_handle_ptr,b_display_allsep, display_format, fbe_debug_trace_func, NULL);
            }
        }
        topology_object_tbl_ptr += topology_object_table_entry_size;      
    }

    display_object = (display_format & FBE_SEPLS_DISPLAY_FORMAT_MASK); /* Extract the object info */
    display_level = (display_format & FBE_SEPLS_DISPLAY_FORMAT_MASK);  /* Extract the level info */

    if(sys_vd_displayed == FBE_FALSE)
    {
        fbe_sepls_display_sys_vds(b_display_allsep, display_format, ptr_size);
    }

    /* For the defualt display we show only the configured HS
     * For the "-allsep" option we show the pvd at two places.
     * First via the RG_VD_PVD link and then all the other PVDs
     * are shown at the end 
     */ 
    fbe_debug_trace_func(NULL,"\n\n");

    /* We need to display the PVD details the last. */
    topology_object_tbl_ptr = head_topology_object_tbl_ptr;

    for(obj_index = 0; obj_index < max_entries ; obj_index++)
    {
        if (FBE_CHECK_CONTROL_C())
        {
            fbe_sep_offsets_clear();
            return;
        }
        /* Fetch the object's topology status. */
        addr = topology_object_tbl_ptr + fbe_sep_offsets.object_status_offset;
        FBE_READ_MEMORY(addr, &object_status,
                        sizeof(fbe_topology_object_status_t));

        /* Need to display the objects even if they are not in the ready state
         */
        if (object_status == FBE_TOPOLOGY_OBJECT_STATUS_READY ||
            object_status == FBE_TOPOLOGY_OBJECT_STATUS_EXIST ||
            object_status == FBE_TOPOLOGY_OBJECT_STATUS_RESERVED)
        {
            /* Fetch the control handle, which is the pointer to the object. */
            addr = topology_object_tbl_ptr +
                   fbe_sep_offsets.control_handle_offset;

            FBE_READ_MEMORY(addr, &control_handle_ptr, sizeof(fbe_dbgext_ptr));

            addr = control_handle_ptr + fbe_sep_offsets.class_id_offset;
            FBE_READ_MEMORY(addr, &class_id, sizeof(fbe_class_id_t));

            if(class_id == FBE_CLASS_ID_PROVISION_DRIVE)
            {
                addr = control_handle_ptr + fbe_sep_offsets.object_id_offset;
                FBE_READ_MEMORY(addr, &object_id, sizeof(fbe_object_id_t));

                fbe_sepls_pvd_info_trace(module_name,object_id, 0, b_display_default, display_format, fbe_debug_trace_func, NULL);
            }
        }
        topology_object_tbl_ptr += topology_object_table_entry_size;
    }
    
    /* Processing all the objects have finished here
     * So reset all the necessary data 
     */
    sys_vd_displayed = FBE_FALSE;
    current_sys_vd_index = 1;
    current_vd_index = 0;
    for(obj_index = 0; obj_index < FBE_MAX_SPARE_OBJECTS;obj_index++)
    {
        pvd_displayed[obj_index] = 0;
        vd_ids[obj_index] = 0;

    }
    for(obj_index = 0; obj_index < FBE_RAID_GROUP_MAX_DISK_ARRAY_WIDTH;obj_index++)
    {
        sys_vd_ids[obj_index] = 0;
    }
    fbe_sep_offsets_clear();

    return;
}

/*!**************************************************************
 * fbe_sep_offsets_clear
 ****************************************************************
 * @brief
 *  Clear the offsets for frequently used fields stored globally.
 *  
 * @return void
 *
 ****************************************************************/
void
fbe_sep_offsets_clear()
{
    fbe_sep_offsets.is_fbe_sep_offsets_set = FBE_FALSE;
    
    fbe_sep_offsets.control_handle_offset = FBE_SEPLS_MAX_OFFSET;
    fbe_sep_offsets.object_status_offset = FBE_SEPLS_MAX_OFFSET;
    
    fbe_sep_offsets.class_id_offset = FBE_SEPLS_MAX_OFFSET;
    fbe_sep_offsets.object_id_offset = FBE_SEPLS_MAX_OFFSET;

    fbe_sep_offsets.db_service_object_table_offset = FBE_SEPLS_MAX_OFFSET;
    fbe_sep_offsets.db_service_user_table_offset = FBE_SEPLS_MAX_OFFSET;
    fbe_sep_offsets.db_service_edge_table_offset = FBE_SEPLS_MAX_OFFSET;
    fbe_sep_offsets.db_service_global_info_offset = FBE_SEPLS_MAX_OFFSET;

    fbe_sep_offsets.db_obj_entry_class_id_offset = FBE_SEPLS_MAX_OFFSET;
    fbe_sep_offsets.db_obj_entry_set_config_union_offset = FBE_SEPLS_MAX_OFFSET;
    fbe_sep_offsets.db_obj_entry_object_header_offset = FBE_SEPLS_MAX_OFFSET;

    fbe_sep_offsets.db_table_content_offset = FBE_SEPLS_MAX_OFFSET;
    fbe_sep_offsets.db_table_size_offset = FBE_SEPLS_MAX_OFFSET;
    fbe_sep_offsets.db_table_header_object_id_offset = FBE_SEPLS_MAX_OFFSET;
    fbe_sep_offsets.db_table_header_state_offset = FBE_SEPLS_MAX_OFFSET;
    fbe_sep_offsets.db_table_header_entry_id_offset = FBE_SEPLS_MAX_OFFSET;

    fbe_sep_offsets.db_user_entry_user_data_union_offset = FBE_SEPLS_MAX_OFFSET;
    fbe_sep_offsets.db_user_entry_header_offset = FBE_SEPLS_MAX_OFFSET;
    fbe_sep_offsets.db_user_entry_db_class_id_offset = FBE_SEPLS_MAX_OFFSET;

    fbe_sep_offsets.db_edge_entry_header_offset = FBE_SEPLS_MAX_OFFSET;
    fbe_sep_offsets.db_edge_entry_server_id_offset = FBE_SEPLS_MAX_OFFSET;
    fbe_sep_offsets.db_edge_entry_client_index_offset = FBE_SEPLS_MAX_OFFSET;
    fbe_sep_offsets.db_edge_entry_capacity_offset = FBE_SEPLS_MAX_OFFSET;
    fbe_sep_offsets.db_edge_entry_offset_offset = FBE_SEPLS_MAX_OFFSET;

    return ;
}
/*****************************************
 * end fbe_sep_offsets_clear
 *****************************************/

/*!**************************************************************
 * fbe_sep_offsets_set
 ****************************************************************
 * @brief
 *  Set the offsets for frequently used fields in a global for later use.
 *  
 * @return void
 *
 ****************************************************************/
void
fbe_sep_offsets_set(const fbe_u8_t * module_name)
{
    const fbe_u8_t *module_name_pp = "PhysicalPackage"; 
    
    /* topology_object_table_entry_t */
    FBE_GET_FIELD_OFFSET(module_name,
                         topology_object_table_entry_t, "control_handle",
                         &fbe_sep_offsets.control_handle_offset);
    /* Validate sep is loaded */
    if (fbe_sep_offsets.control_handle_offset == FBE_SEPLS_MAX_OFFSET)
    {
        module_name = (fbe_u8_t *)module_name_pp;
        FBE_GET_FIELD_OFFSET(module_name,
                             topology_object_table_entry_t, "control_handle",
                             &fbe_sep_offsets.control_handle_offset);
        if (fbe_sep_offsets.control_handle_offset == FBE_SEPLS_MAX_OFFSET)
        {
            fbe_debug_trace_func(NULL, "%s unable to get control_handle for module: %s\n", 
                                 __FUNCTION__, module_name);
            return;
        }
    }

    /* topology_object_table_entry_t */
    FBE_GET_FIELD_OFFSET(module_name,
                         topology_object_table_entry_t, "object_status",
                         &fbe_sep_offsets.object_status_offset);

    /* fbe_base_object_s */
    FBE_GET_FIELD_OFFSET(module_name,
                         fbe_base_object_s, "class_id",
                         &fbe_sep_offsets.class_id_offset);

    FBE_GET_FIELD_OFFSET(module_name,
                         fbe_base_object_s, "object_id",
                         &fbe_sep_offsets.object_id_offset);

    /* fbe_database_service_t */
    FBE_GET_FIELD_OFFSET(module_name,
                         fbe_database_service_t, "object_table",
                         &fbe_sep_offsets.db_service_object_table_offset);

    FBE_GET_FIELD_OFFSET(module_name,
                         fbe_database_service_t, "user_table",
                         &fbe_sep_offsets.db_service_user_table_offset);

    FBE_GET_FIELD_OFFSET(module_name,
                         fbe_database_service_t,"edge_table",
                         &fbe_sep_offsets.db_service_edge_table_offset);

    FBE_GET_FIELD_OFFSET(module_name,
                         fbe_database_service_t,"global_info",
                         &fbe_sep_offsets.db_service_global_info_offset);

    /* database_object_entry_t */
    FBE_GET_FIELD_OFFSET(module_name,
                         database_object_entry_t, "db_class_id",
                         &fbe_sep_offsets.db_obj_entry_class_id_offset);

    FBE_GET_FIELD_OFFSET(module_name,
                         database_object_entry_t, "set_config_union",
                         &fbe_sep_offsets.db_obj_entry_set_config_union_offset);

    FBE_GET_FIELD_OFFSET(module_name,
                         database_object_entry_t, "header",
                         &fbe_sep_offsets.db_obj_entry_object_header_offset);

    /* database_table_t */
    FBE_GET_FIELD_OFFSET(module_name,
                         database_table_t, "table_content",
                         &fbe_sep_offsets.db_table_content_offset);

    FBE_GET_FIELD_OFFSET(module_name,
                         database_table_t, "table_size",
                         &fbe_sep_offsets.db_table_size_offset);

    /* database_table_header_t */
    FBE_GET_FIELD_OFFSET(module_name,
                         database_table_header_t, "object_id",
                         &fbe_sep_offsets.db_table_header_object_id_offset);

    FBE_GET_FIELD_OFFSET(module_name,
                         database_table_header_t,"state",
                         &fbe_sep_offsets.db_table_header_state_offset);
    
    FBE_GET_FIELD_OFFSET(module_name,
                         database_table_header_t, "entry_id",
                         &fbe_sep_offsets.db_table_header_entry_id_offset);

    /* database_user_entry_t */
    FBE_GET_FIELD_OFFSET(module_name,
                         database_user_entry_t, "user_data_union",
                         &fbe_sep_offsets.db_user_entry_user_data_union_offset);

    FBE_GET_FIELD_OFFSET(module_name,
                         database_user_entry_t, "header",
                         &fbe_sep_offsets.db_user_entry_header_offset);

    FBE_GET_FIELD_OFFSET(module_name,
                         database_user_entry_t,"db_class_id",
                         &fbe_sep_offsets.db_user_entry_db_class_id_offset);

    /* database_edge_entry_t */
    FBE_GET_FIELD_OFFSET(module_name,
                         database_edge_entry_t,"header",
                         &fbe_sep_offsets.db_edge_entry_header_offset);
    
    FBE_GET_FIELD_OFFSET(module_name,
                         database_edge_entry_t,"server_id",
                         &fbe_sep_offsets.db_edge_entry_server_id_offset);
    
    FBE_GET_FIELD_OFFSET(module_name,
                         database_edge_entry_t,"client_index",
                         &fbe_sep_offsets.db_edge_entry_client_index_offset);
    
    FBE_GET_FIELD_OFFSET(module_name,
                         database_edge_entry_t,"capacity",
                         &fbe_sep_offsets.db_edge_entry_capacity_offset);
    
    FBE_GET_FIELD_OFFSET(module_name,
                         database_edge_entry_t,"offset",
                         &fbe_sep_offsets.db_edge_entry_offset_offset);

    fbe_sep_offsets.is_fbe_sep_offsets_set = FBE_TRUE;
    return ;
}
/*****************************************
 * end fbe_sep_offsets_set
 *****************************************/

/*!**************************************************************
 * fbe_sepls_set_sys_vd_id
 ****************************************************************
 * @brief
 * Store system VD so it can be displayed when desired
 *  
 * @param vd_id - the object ID of the system VD
 *
 * @return void
 *
 ****************************************************************/
void
fbe_sepls_set_sys_vd_id(fbe_object_id_t vd_id)
{
    fbe_u32_t  vd_index;

    for (vd_index = 0; vd_index < current_sys_vd_index; vd_index++)
    {
        if (sys_vd_ids[vd_index] == vd_id)
        {
            /* already noted */
            return ;
        }
    }
    sys_vd_ids[current_sys_vd_index - 1] = vd_id;
    current_sys_vd_index++;

    return ;
}
/*****************************************
 * end fbe_sepls_set_sys_vd_id
 *****************************************/

/*!**************************************************************
 * fbe_sepls_display_sys_vds
 ****************************************************************
 * @brief
 * Display system VDs if appropriate
 *  
 * @param display_all_flag - indicates if sepls is displaying all objects
 * @param display_format - indicates what sepls is supposed to display
 *
 * @return void
 *
 ****************************************************************/
void
fbe_sepls_display_sys_vds(fbe_bool_t display_all_flag,
                          fbe_u32_t display_format,
                          fbe_u32_t ptr_size)
{
    fbe_u32_t vd_index;
    fbe_u32_t display_object;
    fbe_u32_t display_level;

    if (FBE_TRUE == sys_vd_displayed)
    {
        return ; /* already displayed, so nothing to do */
    }
    sys_vd_displayed = FBE_TRUE;

    /* Check if displaying of VDs is appropriate.
     * Process VD information when the user has specified the -allsep option
     * or only VD objects needs to be displayed,
     * or the level specified is 0x0002 or above.
     */
    display_object = (display_format & FBE_SEPLS_DISPLAY_FORMAT_MASK);
    display_level = (display_format & FBE_SEPLS_DISPLAY_FORMAT_MASK);

    if (display_all_flag == FBE_TRUE ||
        display_object == FBE_SEPLS_DISPLAY_ONLY_VD_OBJECTS ||
        display_level >= FBE_SEPLS_DISPLAY_LEVEL_3)
    {
        const fbe_u8_t * module_name = "sep";
        fbe_dbgext_ptr addr = 0;
        fbe_dbgext_ptr topology_object_tbl_ptr = 0;
        fbe_u32_t topology_object_table_entry_size = 0;

        /* Get the topology table. */
        FBE_GET_EXPRESSION(module_name, topology_object_table, &addr);
        FBE_READ_MEMORY(addr, &topology_object_tbl_ptr, ptr_size);

        /* Get the size of the topology entry. */
        FBE_GET_TYPE_SIZE(module_name, topology_object_table_entry_s,
                          &topology_object_table_entry_size);

        for (vd_index = 0; vd_index < current_sys_vd_index - 1; vd_index++)
        {
            fbe_dbgext_ptr control_handle_ptr = 0;
            fbe_class_id_t class_id = FBE_CLASS_ID_INVALID;

            if (sys_vd_ids[vd_index] == 0)
            {
                continue;
            }
            addr = topology_object_tbl_ptr + 
                   (topology_object_table_entry_size * sys_vd_ids[vd_index]) +
                   fbe_sep_offsets.control_handle_offset;

            FBE_READ_MEMORY(addr, &control_handle_ptr, sizeof(fbe_dbgext_ptr));

            addr = control_handle_ptr + fbe_sep_offsets.class_id_offset;

            FBE_READ_MEMORY(addr, &class_id, sizeof(fbe_class_id_t));

            if (class_id == FBE_CLASS_ID_VIRTUAL_DRIVE)
            {
                (void) fbe_sepls_vd_info_trace(module_name,
                                               sys_vd_ids[vd_index],
                                               exported_block_size,
                                               display_format,
                                               fbe_debug_trace_func, NULL);
            }
            else if (class_id == FBE_CLASS_ID_PROVISION_DRIVE)
            {
                (void)fbe_sepls_pvd_info_trace(module_name,
                                               sys_vd_ids[vd_index],
                                               0, FALSE,
                                               display_format,
                                               fbe_debug_trace_func, NULL);
            }
            else
            {
                fbe_debug_trace_func(NULL, "%s unknown class 0x%x sys vd position: %d obj_id: 0x%x\n",
                                     __FUNCTION__, class_id, vd_index,
                                     sys_vd_ids[vd_index]);
            }
        }
    }
    return ;
}
/*****************************************
 * end fbe_sepls_display_sys_vds
 *****************************************/

/*!**************************************************************
 * fbe_sepls_get_lifecycle_state_name()
 ****************************************************************
 * @brief
 *  Get the object state string.
 *  
 * @param fbe_lifecycle_state_t - Object state.
 * @param const char ** - Buffer to pass the value in
 * 
 * @return fbe_status_t
 *
 * @author
 *  20/05/2011 - Created. Omer Miranda
 *
 ****************************************************************/
fbe_status_t fbe_sepls_get_lifecycle_state_name(fbe_lifecycle_state_t state, const char ** state_name)
{
    *state_name = NULL;
    switch (state) {
        case FBE_LIFECYCLE_STATE_SPECIALIZE:
            *state_name = "SPE";
            break;
        case FBE_LIFECYCLE_STATE_ACTIVATE:
            *state_name = "ACT";
            break;
        case FBE_LIFECYCLE_STATE_READY:
            *state_name = "READY";
            break;
        case FBE_LIFECYCLE_STATE_HIBERNATE:
            *state_name = "HIBE";
            break;
        case FBE_LIFECYCLE_STATE_OFFLINE:
            *state_name = "OFFLN";
            break;
        case FBE_LIFECYCLE_STATE_FAIL:
            *state_name = "FAIL";
            break;
        case FBE_LIFECYCLE_STATE_DESTROY:
            *state_name = "DEST";
            break;
        case FBE_LIFECYCLE_STATE_PENDING_READY:
            *state_name = "PEND_RDY";
            break;
        case FBE_LIFECYCLE_STATE_PENDING_ACTIVATE:
            *state_name = "PEND_ACT";
            break;
        case FBE_LIFECYCLE_STATE_PENDING_HIBERNATE:
            *state_name = "PEND_HIB";
            break;
        case FBE_LIFECYCLE_STATE_PENDING_OFFLINE:
            *state_name = "PEND_OFFLN";
            break;
        case FBE_LIFECYCLE_STATE_PENDING_FAIL:
            *state_name = "PEND_FAIL";
            break;
        case FBE_LIFECYCLE_STATE_PENDING_DESTROY:
            *state_name = "PEND_DES";
            break;
        case FBE_LIFECYCLE_STATE_NOT_EXIST:
            *state_name = "NOT_EXIST";
            break;
        case FBE_LIFECYCLE_STATE_INVALID:
            *state_name = "INVALID";
            break;
        default:
            return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}
/*****************************************
 * end fbe_sepls_get_lifecycle_state_name
 *****************************************/

/*!**************************************************************
 * fbe_sepls_display_object_state_debug_trace()
 ****************************************************************
 * @brief
 *  Display the object state string.
 *  
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param fbe_lifecycle_state_t - Object state.
 * 
 * @return fbe_status_t
 *
 * @author
 *  30/06/2011 - Created. Omer Miranda
 *
 ****************************************************************/
fbe_status_t fbe_sepls_display_object_state_debug_trace(fbe_trace_func_t trace_func,
                                                        fbe_trace_context_t trace_context,
                                                        fbe_lifecycle_state_t state)
{
    switch (state) {
        case FBE_LIFECYCLE_STATE_SPECIALIZE:
            trace_func(NULL,"%-10s","SPE");
            break;
        case FBE_LIFECYCLE_STATE_ACTIVATE:
            trace_func(NULL,"%-10s","ACT");
            break;
        case FBE_LIFECYCLE_STATE_READY:
            trace_func(NULL,"%-6s","READY");
            break;
        case FBE_LIFECYCLE_STATE_HIBERNATE:
            trace_func(NULL,"%-10s","HIBE");
            break;
        case FBE_LIFECYCLE_STATE_OFFLINE:
            trace_func(NULL,"%-10s","OFFLN");
            break;
        case FBE_LIFECYCLE_STATE_FAIL:
            trace_func(NULL,"%-10s","FAIL");
            break;
        case FBE_LIFECYCLE_STATE_DESTROY:
            trace_func(NULL,"%-10s","DEST");
            break;
        case FBE_LIFECYCLE_STATE_PENDING_READY:
            trace_func(NULL,"%-10s","PEND_RDY");
            break;
        case FBE_LIFECYCLE_STATE_PENDING_ACTIVATE:
            trace_func(NULL,"%-10s","PEND_ACT");
            break;
        case FBE_LIFECYCLE_STATE_PENDING_HIBERNATE:
            trace_func(NULL,"%-10s","PEND_HIB");
            break;
        case FBE_LIFECYCLE_STATE_PENDING_OFFLINE:
            trace_func(NULL,"%-10s","PEND_OFFLN");
            break;
        case FBE_LIFECYCLE_STATE_PENDING_FAIL:
            trace_func(NULL,"%-10s","PEND_FAIL");
            break;
        case FBE_LIFECYCLE_STATE_PENDING_DESTROY:
            trace_func(NULL,"%-10s","PEND_DES");
            break;
        case FBE_LIFECYCLE_STATE_NOT_EXIST:
            trace_func(NULL,"%-10s","NOT_EXIST");
            break;
        case FBE_LIFECYCLE_STATE_INVALID:
            trace_func(NULL,"%-10s","INVALID");
            break;
        default:
            return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}
/*********************************************
 * end fbe_sepls_display_object_state_debug_trace
 *********************************************/

/*!**************************************************************
 * fbe_sepls_get_persist_data_debug_trace()
 ****************************************************************
 * @brief
 *  Get the object's persist data according to the object type.
 *  
 * @param module_name - Module name to use in symbol lookup
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param fbe_dbgext_ptr - ptr to object
 *.@param fbe_object_id_t - Object id
 * @param void * - Pointer to the object data structure to be filled with details
 * 
 * @return fbe_status_t
 *
 * @author
 *  24/05/2011 - Created. Omer Miranda
 *
 ****************************************************************/
fbe_status_t fbe_sepls_get_persist_data_debug_trace(const fbe_u8_t* module_name,
                                                    fbe_trace_func_t trace_func,
                                                    fbe_trace_context_t trace_context,
                                                    fbe_dbgext_ptr object_ptr,
                                                    fbe_dbgext_ptr user_ptr,
                                                    void *object_data_ptr)
{
    fbe_u32_t object_offset = 0;
    fbe_u32_t user_offset = 0;
    fbe_dbgext_ptr addr;
    database_class_id_t class_id;
     
    addr = object_ptr + fbe_sep_offsets.db_obj_entry_class_id_offset;
    FBE_READ_MEMORY(addr, &class_id, sizeof(database_class_id_t));

    object_offset = fbe_sep_offsets.db_obj_entry_set_config_union_offset;
    user_offset = fbe_sep_offsets.db_user_entry_user_data_union_offset;

    switch(class_id)
    {
        case DATABASE_CLASS_ID_PROVISION_DRIVE:
            fbe_sepls_get_pvd_persist_data(module_name,
                                           object_ptr + object_offset,
                                           user_ptr + user_offset,
                                           (fbe_pvd_database_info_t *)object_data_ptr,
                                           trace_func,
                                           trace_context);
            break;
        case DATABASE_CLASS_ID_VIRTUAL_DRIVE:
            fbe_sepls_get_vd_persist_data(module_name,
                                          object_ptr + object_offset,
                                          user_ptr + user_offset,
                                          (fbe_vd_database_info_t *)object_data_ptr,
                                          trace_func,
                                          trace_context);
            break;   
        case DATABASE_CLASS_ID_RAID_START:
        case DATABASE_CLASS_ID_MIRROR:
        case DATABASE_CLASS_ID_STRIPER:
        case DATABASE_CLASS_ID_PARITY:
        case DATABASE_CLASS_ID_RAID_END:
            fbe_sepls_get_rg_persist_data(module_name,
                                          object_ptr + object_offset,
                                          user_ptr + user_offset,
                                          (fbe_raid_group_database_info_t *)object_data_ptr,
                                          trace_func,
                                          trace_context);
            break;
        case DATABASE_CLASS_ID_LUN:
            fbe_sepls_get_lun_persist_data(module_name,
                                           object_ptr + object_offset,
                                           user_ptr + user_offset,
                                           (fbe_lun_database_info_t *)object_data_ptr, 
                                           trace_func,
                                           trace_context);
            break;
        default:
            fbe_debug_trace_func(NULL, "Invalid Class ID:0x%x.  ", class_id);
            return FBE_STATUS_GENERIC_FAILURE;

    }
    return FBE_STATUS_OK;
}
/*****************************************
 * end fbe_sepls_get_persist_data_debug_trace
 *****************************************/

/*!**************************************************************
 * fbe_sepls_get_config_data_debug_trace()
 ****************************************************************
 * @brief
 *  Get the object's persist data 
 *  
 * @param module_name - Module name to use in symbol lookup
 * @param fbe_object_id_t - Object id
 * @param void* - pointer to the object data structure to be filled with details
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 *.
 * 
 * @return fbe_status_t
 *
 * @author
 *  24/05/2011 - Created. Omer Miranda
 *
 ****************************************************************/
fbe_status_t fbe_sepls_get_config_data_debug_trace(const fbe_u8_t * module_name,
                                                   fbe_object_id_t in_object_id,
                                                   void *object_data_ptr,
                                                   fbe_trace_func_t trace_func,
                                                   fbe_trace_context_t trace_context)
{
    fbe_dbgext_ptr database_service_ptr = 0;
    fbe_u32_t header_offset = 0;
    fbe_u32_t object_id_offset = 0;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_dbgext_ptr object_table_ptr = 0;
    fbe_u32_t database_object_entry_size;
    fbe_dbgext_ptr object_entry_ptr = 0;
    fbe_dbgext_ptr user_table_ptr = 0;
    fbe_dbgext_ptr user_entry_ptr = 0;
    fbe_u32_t database_user_entry_size;
    database_table_size_t table_size;
    fbe_u32_t table_entries;
    fbe_dbgext_ptr db_object_ptr = 0;
    fbe_dbgext_ptr db_user_ptr = 0;
    fbe_object_id_t object_id;
    fbe_dbgext_ptr addr;

    FBE_GET_EXPRESSION(module_name, fbe_database_service, &database_service_ptr);
    if(database_service_ptr == 0)
    {
        fbe_debug_trace_func(NULL, "database_service_ptr is not available");
        return FBE_STATUS_OK;
    }
    
    /* Get the object table pointer */
    object_table_ptr = database_service_ptr +
                       fbe_sep_offsets.db_service_object_table_offset;

    /* Get the number of object entries */
    addr = object_table_ptr + fbe_sep_offsets.db_table_size_offset;
    FBE_READ_MEMORY(addr, &table_size, sizeof(database_table_size_t));

    addr = object_table_ptr + fbe_sep_offsets.db_table_content_offset;
    FBE_READ_MEMORY(addr, &object_entry_ptr, sizeof(fbe_dbgext_ptr));

    /* Get the size of the database object entry. */
    FBE_GET_TYPE_SIZE(module_name, database_object_entry_t, &database_object_entry_size);

    db_object_ptr = object_entry_ptr;
    header_offset = fbe_sep_offsets.db_obj_entry_object_header_offset;
    object_id_offset = fbe_sep_offsets.db_table_header_object_id_offset;

    for(table_entries = 0;table_entries < table_size ; table_entries++)
    {
        FBE_READ_MEMORY(db_object_ptr + header_offset + object_id_offset,
                        &object_id, sizeof(fbe_object_id_t));

        if(object_id == in_object_id )
        {
            break;
        }
        /* Advance to the next entry */
        db_object_ptr += database_object_entry_size;
        /* Keep looping till we dont reach to the specified object id */
    }

    /* Get the user table pointer */
    user_table_ptr = database_service_ptr +
                     fbe_sep_offsets.db_service_user_table_offset;

    addr = user_table_ptr + fbe_sep_offsets.db_table_content_offset;
    FBE_READ_MEMORY(addr, &user_entry_ptr, sizeof(fbe_dbgext_ptr));

    /* Get the size of the database user entry. */
    FBE_GET_TYPE_SIZE(module_name, database_user_entry_t, &database_user_entry_size);

    db_user_ptr = user_entry_ptr;
    header_offset = fbe_sep_offsets.db_user_entry_header_offset;
    object_id_offset = fbe_sep_offsets.db_table_header_object_id_offset;

    for(table_entries = 0;table_entries < table_size ; table_entries++)
    {
        FBE_READ_MEMORY(db_user_ptr + header_offset + object_id_offset,
                        &object_id, sizeof(fbe_object_id_t));

        if(object_id == in_object_id )
        {
            break;
        }
        /* Advance to the next entry */
        db_user_ptr += database_user_entry_size;
        /* Keep looping till we dont reach to the specified object id */
    }

    status = fbe_sepls_get_persist_data_debug_trace(module_name,
                                           trace_func, 
                                           trace_context,
                                           db_object_ptr,
                                           db_user_ptr,
                                           object_data_ptr);

    if (status != FBE_STATUS_OK)
    {
        fbe_debug_trace_func(NULL, "Get Persist Data failed for obj id: 0x%x\n", object_id);

        return status;
    }

    return FBE_STATUS_OK;
}
/*****************************************
 * end fbe_sepls_get_config_data_debug_trace
 *****************************************/

/*!**************************************************************
 * fbe_sepls_get_drives_info_debug_trace()
 ****************************************************************
 * @brief
 *  Get the drives associated with the object 
 *  
 * @param module_name - Module name to use in symbol lookup
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param fbe_object_id_t - Object id
 *.
 * 
 * @return fbe_status_t
 *
 * @author
 *  24/05/2011 - Created. Omer Miranda
 *
 ****************************************************************/
fbe_status_t fbe_sepls_get_drives_info_debug_trace(const fbe_u8_t* module_name,
                                                   fbe_trace_func_t trace_func,
                                                   fbe_trace_context_t trace_context,
                                                   fbe_object_id_t in_object_id)
{
    fbe_dbgext_ptr topology_object_table_ptr = 0;
    fbe_topology_object_status_t  object_status = 0;
    fbe_dbgext_ptr control_handle_ptr = 0;
    fbe_u32_t topology_object_table_entry_size = 0;
    fbe_u32_t offset = 0;
    fbe_u32_t block_edge_ptr_offset;
    fbe_u32_t path_state_offset;
    fbe_u32_t server_id_offset;
    fbe_object_id_t server_id = 0;
    fbe_path_state_t path_state = FBE_PATH_STATE_INVALID; 
    fbe_u32_t width;
    fbe_u32_t ptr_size = 0;
    fbe_u32_t edge_index = 0;
    fbe_u32_t block_edge_size = 0;
    fbe_u64_t block_edge_p = 0;
    fbe_slot_number_t slot_number; 	
    fbe_port_number_t port_number;	    
    fbe_enclosure_number_t enclosure_number;
    ULONG64 max_toplogy_entries_p = 0;
    fbe_u32_t max_entries;
    fbe_dbgext_ptr topology_object_tbl_ptr = 0;
    fbe_dbgext_ptr addr = 0;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_block_edge_t block_edge_debug = {0};
    fbe_u32_t drives_counter = 0;
    fbe_dbgext_ptr geo_ptr = 0;
    fbe_raid_group_type_t raid_type;

    status = fbe_debug_get_ptr_size("sep", &ptr_size);
    if (status != FBE_STATUS_OK) 
    {
        csx_dbg_ext_print("%s unable to get ptr size status: %d\n", __FUNCTION__, status);
        return status; 
    }

    FBE_GET_EXPRESSION(pp_ext_module_name, max_supported_objects, &max_toplogy_entries_p);
    FBE_READ_MEMORY(max_toplogy_entries_p, &max_entries, sizeof(fbe_u32_t));

    if(in_object_id >= max_entries)
    {
        trace_func(NULL, "(FAIL) ");
        return FBE_STATUS_NO_OBJECT;
    }
    FBE_GET_EXPRESSION(module_name, topology_object_table, &topology_object_table_ptr);
    FBE_READ_MEMORY(topology_object_table_ptr, &topology_object_tbl_ptr, ptr_size);

    FBE_GET_TYPE_SIZE(module_name, topology_object_table_entry_s, &topology_object_table_entry_size);

    if(topology_object_tbl_ptr == 0)
    {
        trace_func(NULL, "\t topology_object_table_is not available \n");
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Point to the object we are looking for 
     */
    topology_object_tbl_ptr += (topology_object_table_entry_size * in_object_id);

    /* Fetch the object's topology status.
     */
    addr = topology_object_tbl_ptr + fbe_sep_offsets.object_status_offset;
    FBE_READ_MEMORY(addr, &object_status, sizeof(fbe_topology_object_status_t));
    /* Fetch the control handle, which is the pointer to the object.
     */
    addr = topology_object_tbl_ptr + fbe_sep_offsets.control_handle_offset;
    FBE_READ_MEMORY(addr, &control_handle_ptr, sizeof(fbe_dbgext_ptr));

    if (object_status == FBE_TOPOLOGY_OBJECT_STATUS_READY ||
        object_status == FBE_TOPOLOGY_OBJECT_STATUS_EXIST ||
        object_status == FBE_TOPOLOGY_OBJECT_STATUS_RESERVED)
    {
        fbe_class_id_t class_id;

        addr = control_handle_ptr + fbe_sep_offsets.class_id_offset;
        FBE_READ_MEMORY(addr, &class_id, sizeof(fbe_class_id_t));

	
         switch (class_id)
         {
            case FBE_CLASS_ID_LOGICAL_DRIVE:
                FBE_GET_FIELD_OFFSET(module_name,fbe_logical_drive_t,"block_edge",&block_edge_ptr_offset);
                FBE_GET_FIELD_OFFSET(module_name, fbe_base_edge_t, "server_id", &offset);
                FBE_READ_MEMORY(control_handle_ptr + block_edge_ptr_offset + offset, &server_id, sizeof(fbe_object_id_t));
                fbe_sepls_get_drives_info_debug_trace(module_name,
                                                      trace_func,
                                                      trace_context,
                                                      server_id);
                break;
            case FBE_CLASS_ID_BASE_PHYSICAL_DRIVE:
            case FBE_CLASS_ID_SAS_PHYSICAL_DRIVE:
            case FBE_CLASS_ID_SAS_FLASH_DRIVE:
            case FBE_CLASS_ID_SATA_PHYSICAL_DRIVE:
            case FBE_CLASS_ID_SATA_FLASH_DRIVE:
                        
                /* Extract bus-enclosure-slot information
                 */
                FBE_GET_FIELD_OFFSET(module_name, fbe_base_physical_drive_t, "port_number", &offset);
                FBE_READ_MEMORY(control_handle_ptr + offset, &port_number, sizeof(fbe_port_number_t));
                trace_func(NULL, "%d_",port_number);
                        
                FBE_GET_FIELD_OFFSET(module_name, fbe_base_physical_drive_t, "enclosure_number", &offset);
                FBE_READ_MEMORY(control_handle_ptr + offset, &enclosure_number, sizeof(fbe_enclosure_number_t));
                trace_func(NULL, "%d_",enclosure_number);
                        
                FBE_GET_FIELD_OFFSET(module_name, fbe_base_physical_drive_t, "slot_number", &offset);
                FBE_READ_MEMORY(control_handle_ptr + offset, &slot_number, sizeof(fbe_slot_number_t));
                trace_func(NULL, "%d",slot_number);		
                trace_func(NULL, " ");		
                        
                break;
            case FBE_CLASS_ID_PARITY:
            case FBE_CLASS_ID_STRIPER:
            case FBE_CLASS_ID_MIRROR:
                FBE_GET_FIELD_OFFSET(module_name, fbe_base_config_t, "width", &offset);
                FBE_READ_MEMORY(control_handle_ptr + offset, &width, sizeof(fbe_u32_t));
                /* Get the raid type */
                FBE_GET_FIELD_OFFSET(module_name, fbe_raid_group_t, "geo", &offset);
                geo_ptr = control_handle_ptr + offset;
                FBE_GET_FIELD_OFFSET(module_name, fbe_raid_geometry_t, "raid_type", &offset);
                FBE_READ_MEMORY(geo_ptr + offset, &raid_type, sizeof(fbe_raid_group_type_t));

                FBE_GET_FIELD_OFFSET(module_name, fbe_base_config_t, "block_edge_p", &block_edge_ptr_offset);
                FBE_READ_MEMORY(control_handle_ptr + block_edge_ptr_offset, &block_edge_p, sizeof(fbe_dbgext_ptr));
                FBE_READ_MEMORY(block_edge_p, &block_edge_debug, sizeof(fbe_block_edge_t));
                FBE_GET_TYPE_SIZE(module_name, fbe_block_edge_t, &block_edge_size);
                /* Get the drive info for each downstream object in the RG
                 */

                FBE_GET_FIELD_OFFSET(module_name, fbe_base_edge_t, "server_id", &offset);

                for ( edge_index = 0; edge_index < width; edge_index++)
                {
                    if (FBE_CHECK_CONTROL_C())
                    {
                        return FBE_STATUS_CANCELED;
                    }
                    FBE_READ_MEMORY(block_edge_p + (block_edge_size * edge_index) + offset, &server_id, sizeof(fbe_object_id_t));

                    fbe_sepls_get_drives_info_debug_trace(module_name,
                                                          trace_func,
                                                          trace_context,
                                                          server_id);
                    /* We only display eight drives in one row */
                    if(raid_type == FBE_RAID_GROUP_TYPE_RAID10)
                    {
                        if(edge_index == 3)
                        {
                            drives_counter = edge_index + 1;
                            break;
                        }
                    }
                    else if(edge_index == 7)
                    {
                        drives_counter = edge_index + 1;
                        break;
                    }
                }

                if(drives_counter >= 3)
                {
                    trace_func(NULL,"\n");
                    fbe_trace_indent(trace_func, trace_context,93);
                    for ( edge_index = drives_counter; edge_index < width; edge_index++)
                    {
                        if (FBE_CHECK_CONTROL_C())
                        {
                            return FBE_STATUS_CANCELED;
                        }
                        FBE_READ_MEMORY(block_edge_p + (block_edge_size * edge_index) + offset, &server_id, sizeof(fbe_object_id_t));

                        fbe_sepls_get_drives_info_debug_trace(module_name,
                                                              trace_func,
                                                              trace_context,
                                                              server_id);
                    }
                }
                break;
            case FBE_CLASS_ID_VIRTUAL_DRIVE:
                FBE_GET_FIELD_OFFSET(module_name, fbe_base_config_t, "width", &offset);
                FBE_READ_MEMORY(control_handle_ptr + offset, &width, sizeof(fbe_u32_t));
                        
                /* Extract downstream object
                 */
                FBE_GET_FIELD_OFFSET(module_name, fbe_base_config_t, "block_edge_p", &block_edge_ptr_offset);
                FBE_READ_MEMORY(control_handle_ptr + block_edge_ptr_offset, &block_edge_p, sizeof(fbe_dbgext_ptr));
                FBE_GET_TYPE_SIZE(module_name, fbe_block_edge_t, &block_edge_size);

                FBE_GET_FIELD_OFFSET(module_name, fbe_base_edge_t, "path_state", &path_state_offset);
		FBE_GET_FIELD_OFFSET(module_name, fbe_base_edge_t, "server_id", &server_id_offset);

                for ( edge_index = 0; edge_index < width; edge_index++)
                {
                    if (FBE_CHECK_CONTROL_C())
                    {
                        return FBE_STATUS_CANCELED;
                    }
                    /* For VD, we should check the edge state first in case there is an invalid downstream PVD involved */
                    FBE_READ_MEMORY(block_edge_p + (block_edge_size * edge_index) + path_state_offset, &path_state, sizeof(fbe_path_state_t));
                    if (path_state == FBE_PATH_STATE_INVALID)
                    {
                        /* continue the loop for next edge */
                        continue;
                    }
                    FBE_READ_MEMORY(block_edge_p + (block_edge_size * edge_index) + server_id_offset, &server_id, sizeof(fbe_object_id_t));
                    fbe_sepls_get_drives_info_debug_trace(module_name, trace_func, trace_context, server_id);
                }
                break;
            case FBE_CLASS_ID_PROVISION_DRIVE:
                FBE_GET_FIELD_OFFSET(module_name, fbe_base_config_t, "width", &offset);
                FBE_READ_MEMORY(control_handle_ptr + offset, &width, sizeof(fbe_u32_t));
                       
                /* Extract downstream object
                 */
                FBE_GET_FIELD_OFFSET(module_name, fbe_base_config_t, "block_edge_p", &block_edge_ptr_offset);
                FBE_READ_MEMORY(control_handle_ptr + block_edge_ptr_offset, &block_edge_p, sizeof(fbe_dbgext_ptr));
                FBE_GET_TYPE_SIZE(module_name, fbe_block_edge_t, &block_edge_size);

                FBE_GET_FIELD_OFFSET(module_name, fbe_base_edge_t, "server_id", &offset);
    
		module_name = fbe_get_module_name();

		for ( edge_index = 0; edge_index < width; edge_index++)
                {
                    if (FBE_CHECK_CONTROL_C())
                    {
                        return FBE_STATUS_CANCELED;
                    }
                    FBE_READ_MEMORY(block_edge_p + (block_edge_size * edge_index) + offset, &server_id, sizeof(fbe_object_id_t));
                    
		    fbe_sepls_get_drives_info_debug_trace(module_name,
                                                          trace_func,
                                                          trace_context,
                                                          server_id);
                }
                break;
        }
    }
    return FBE_STATUS_OK;
}
/*****************************************
 * end fbe_sepls_get_drives_info_debug_trace
 *****************************************/

/*!**************************************************************
 * fbe_sepls_get_downstream_object_debug_trace()
 ****************************************************************
 * @brief
 *  Get the object's associated downstream object 
 *  
 * @param module_name - Module name to use in symbol lookup
 * @param fbe_dbgext_ptr - object_ptr
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 *.
 * 
 * @return fbe_status_t
 *
 * @author
 *  24/05/2011 - Created. Omer Miranda
 *
 ****************************************************************/
fbe_object_id_t fbe_sepls_get_downstream_object_debug_trace(const fbe_u8_t* module_name,
                                                            fbe_dbgext_ptr object_ptr,
                                                            fbe_trace_func_t trace_func,
                                                            fbe_trace_context_t trace_context)
{
    fbe_u32_t width;
    fbe_u32_t offset = 0;
    fbe_dbgext_ptr base_config_ptr = 0;
    fbe_u32_t block_edge_ptr_offset;
    fbe_u32_t ptr_size = 0;
    fbe_u32_t edge_index = 0;
    fbe_u64_t block_edge_p = 0;
    fbe_u32_t block_edge_size = 0;
    fbe_object_id_t     server_id;
    fbe_dbgext_ptr base_edge_ptr = 0;

    /* Get the down stream object */
    FBE_GET_FIELD_OFFSET(module_name,fbe_lun_t,"base_config",&offset);
    base_config_ptr = object_ptr + offset;
    FBE_GET_FIELD_DATA(module_name,
                       base_config_ptr,
                       fbe_base_config_t,
                       width, sizeof(fbe_u32_t),
                       &width);

    FBE_GET_TYPE_SIZE(module_name, fbe_base_config_t*, &ptr_size);
    FBE_GET_FIELD_OFFSET(module_name, fbe_base_config_t, "block_edge_p", &block_edge_ptr_offset);
    FBE_READ_MEMORY(base_config_ptr + block_edge_ptr_offset, &block_edge_p, ptr_size);
    FBE_GET_TYPE_SIZE(module_name, fbe_block_edge_t, &block_edge_size);

    for ( edge_index = 0; edge_index < width; edge_index++)
    {
        if (FBE_CHECK_CONTROL_C())
        {
            return FBE_STATUS_CANCELED;
        }

        FBE_GET_FIELD_OFFSET(module_name, fbe_block_edge_t, "base_edge", &offset);
        base_edge_ptr = block_edge_p + (block_edge_size * edge_index) + offset;
        
        FBE_GET_FIELD_OFFSET(module_name, fbe_base_edge_t, "server_id", &offset);
        FBE_READ_MEMORY(base_edge_ptr + offset, &server_id, sizeof(fbe_object_id_t));
        return server_id;  /* return with the first server id
                            * valid only for lun, VD and PVD objects
                            */
    }

    return FBE_STATUS_CANCELED;
}
/***************************************************
 * end fbe_sepls_get_downstream_object_debug_trace
 ***************************************************/

//end of sepls.c
