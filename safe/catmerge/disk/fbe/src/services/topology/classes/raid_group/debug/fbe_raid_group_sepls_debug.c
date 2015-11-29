/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_raid_group_sepls_debug.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the sepls related functions for the RG object.
 *  This  file contains functions which will aid in displaying RG related
 *  information for the sepls debug macro
 *
 * @author
 *  24/05/2011 - Created. Omer Miranda
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
#include "fbe_raid_group_object.h"
#include "fbe_base_object_trace.h"
#include "fbe_raid_library_debug.h"
#include "fbe_base_config_debug.h"
#include "fbe_sepls_debug.h"
#include "fbe_raid_group_debug.h"
#include "fbe_lun_debug.h"
#include "fbe_virtual_drive_debug.h"
#include "fbe_topology_debug.h"
#include "fbe_raid_library.h"
#include "fbe_raid_group_sepls_debug.h"



/* List of system VDs displayed */
extern fbe_object_id_t sys_vd_ids[FBE_RAID_GROUP_MAX_DISK_ARRAY_WIDTH];
extern fbe_object_id_t vd_ids[FBE_MAX_SPARE_OBJECTS];

extern fbe_u32_t current_sys_vd_index;
extern fbe_u32_t current_vd_index;
/* Flag to indicate whether system VDs have been displayed or not */
extern fbe_bool_t sys_vd_displayed;
extern fbe_block_size_t exported_block_size;
/*************************
 *   FUNCTION DEFINITIONS
 *************************/


/*!**************************************************************
 * fbe_sepls_rg_info_trace()
 ****************************************************************
 * @brief
 *  Populate the Raid Group information.
 *  
 * @param module_name - Module name to use in symbol lookup.
 * @param raid_group_p - ptr to RG object.
 * @param display_all_flag - Flag to indicate the type of display (default/allsep)
 * @param display_format - Bitmap to indicate the user options
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * 
 * @return fbe_status_t
 *
 * @author
 *  24/05/2011 - Created. Omer Miranda
 *
 ****************************************************************/
fbe_status_t fbe_sepls_rg_info_trace(const fbe_u8_t * module_name,
                                     fbe_dbgext_ptr control_handle_ptr,
                                     fbe_bool_t display_all_flag,
                                     fbe_u32_t display_format,
                                     fbe_trace_func_t trace_func,
                                     fbe_trace_context_t trace_context)
{
    fbe_class_id_t class_id = FBE_CLASS_ID_INVALID;
    fbe_object_id_t object_id;
    fbe_lifecycle_state_t lifecycle_state;
    fbe_u32_t width;
    fbe_object_id_t server_ids[FBE_RAID_GROUP_MAX_DISK_ARRAY_WIDTH /* Max number of drives */];
    fbe_u32_t offset = 0;
    fbe_u32_t edge_index = 0;
    fbe_u32_t vd_index;
    fbe_u32_t ptr_size = 0;
    fbe_u32_t block_edge_ptr_offset;
    fbe_dbgext_ptr base_config_ptr = 0;
    fbe_u64_t block_edge_p = 0;
    fbe_u32_t block_edge_size = 0;
    fbe_object_id_t     server_id;
    fbe_dbgext_ptr base_edge_ptr = 0;
    fbe_dbgext_ptr geo_ptr = 0;
    fbe_raid_group_type_t   raid_type;
    fbe_u64_t queue_head = 0;
    fbe_u64_t queue_element = 0;
    fbe_u64_t block_transport_p = 0;
    fbe_object_id_t client_id;
    fbe_bool_t vd_displayed = FBE_FALSE;
    fbe_sepls_rg_config_t rg_config ={0};
    fbe_raid_group_database_info_t rg_database_info ={0};
    fbe_lba_t rg_capacity;
    fbe_lba_t lun_capacity;
    fbe_u32_t display_object = (display_format & FBE_SEPLS_DISPLAY_FORMAT_MASK); /* Extract the object info */
    fbe_u32_t display_level = (display_format & FBE_SEPLS_DISPLAY_FORMAT_MASK);  /* Extract the level info */
    fbe_u16_t rebuild_percent[FBE_RAID_GROUP_MAX_REBUILD_POSITIONS];
    fbe_lba_t rebuild_capacity;
    fbe_u32_t loop_counter;
    fbe_u32_t server_counter = 0; /* to keep track of the number of downstream objects */
    fbe_dbgext_ptr addr = 0;
    fbe_dbgext_ptr topology_object_tbl_ptr = 0;
    fbe_u32_t topology_object_table_entry_size = 0;
    fbe_dbgext_ptr server_control_handle_ptr = 0;
    fbe_class_id_t server_class_id = FBE_CLASS_ID_INVALID;
    fbe_status_t status = FBE_STATUS_OK;
    
    /* Get the size of pointer. */
    status = fbe_debug_get_ptr_size(module_name, &ptr_size);
    if (status != FBE_STATUS_OK) 
    {
        trace_func(NULL, "%s unable to get ptr size status: %d\n", __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE; 
    }

    /* Get the topology table. */
    FBE_GET_EXPRESSION(module_name, topology_object_table, &addr);
    FBE_READ_MEMORY(addr, &topology_object_tbl_ptr, ptr_size);

    /* Get the size of the topology entry. */
    FBE_GET_TYPE_SIZE(module_name, topology_object_table_entry_s,
                      &topology_object_table_entry_size);

    rg_config.total_lun_capacity = 0;

    /* Start populating the RG info */
    FBE_READ_MEMORY(control_handle_ptr+fbe_sep_offsets.class_id_offset, &class_id, sizeof(fbe_class_id_t));
    FBE_READ_MEMORY(control_handle_ptr+fbe_sep_offsets.object_id_offset, &object_id, sizeof(fbe_object_id_t));
    rg_config.object_id = object_id;

    FBE_GET_FIELD_DATA(module_name,
                       control_handle_ptr,
                       fbe_base_object_s,
                       lifecycle.state,
                       sizeof(fbe_lifecycle_state_t),
                       &lifecycle_state);

    rg_config.state = lifecycle_state;

    /* Get the Raid type info */
    FBE_GET_FIELD_OFFSET(module_name,fbe_raid_group_t,"geo",&offset);
    geo_ptr = control_handle_ptr + offset;
    FBE_GET_FIELD_OFFSET(module_name,fbe_raid_geometry_t,"exported_block_size",&offset);
    FBE_READ_MEMORY(geo_ptr + offset, &exported_block_size, sizeof(fbe_block_size_t));
    FBE_GET_FIELD_OFFSET(module_name,fbe_raid_geometry_t,"raid_type",&offset);
    FBE_READ_MEMORY(geo_ptr + offset, &raid_type, sizeof(fbe_raid_group_type_t));
    rg_config.raid_type = raid_type;

    /* Get the configured capacity for this RG */
    FBE_GET_FIELD_OFFSET(module_name,fbe_raid_geometry_t,"configured_capacity",&offset);
    FBE_READ_MEMORY(geo_ptr + offset, &rg_capacity, sizeof(fbe_lba_t));
    rg_config.rg_capacity = rg_capacity;

    /* Here we handle the RAID1_0 type rg
     * For RAID1_0 the topology is LUN-STRIPPER-MIRROR-VD-PVD
     * Hence we display the striper first and get the down stream objects from stripper
     * which is nothin but the MIRRORS
     */
    if(raid_type == FBE_RAID_GROUP_TYPE_INTERNAL_MIRROR_UNDER_STRIPER)
    {
        return FBE_STATUS_OK;
    }

    fbe_sepls_get_config_data_debug_trace(module_name,object_id,&rg_database_info, trace_func,trace_context);
    rg_config.rg_database_info.system = rg_database_info.system;
    rg_config.rg_database_info.rg_number = rg_database_info.rg_number;
    rg_config.rg_database_info.capacity = rg_database_info.capacity;

    /* Get the down stream object and drives details */
    FBE_GET_FIELD_OFFSET(module_name,fbe_raid_group_t,"base_config",&offset);
    base_config_ptr = control_handle_ptr + offset;

    /* Populate the rebuild information */
    fbe_sepls_get_rg_rebuild_info(module_name, base_config_ptr, &rg_config.rebuild_info);

    fbe_sepls_get_rg_encryption_info(module_name, base_config_ptr, &rg_config.encryption_info);

    FBE_GET_FIELD_DATA(module_name,
                       base_config_ptr,
                       fbe_base_config_t,
                       width, sizeof(fbe_u32_t),
                       &width);

    rg_config.width = width;

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
        server_ids[edge_index] = server_id;

        addr = topology_object_tbl_ptr + 
                   (topology_object_table_entry_size * server_id) +
                   fbe_sep_offsets.control_handle_offset;

        FBE_READ_MEMORY(addr, &server_control_handle_ptr, sizeof(fbe_dbgext_ptr));

        addr = server_control_handle_ptr + fbe_sep_offsets.class_id_offset;

        FBE_READ_MEMORY(addr, &server_class_id, sizeof(fbe_class_id_t));

        /* Since more than one type of RG can exist on system drives
         * we display all the system luns and RGs first and then
         * we display the associated VDs and PVDs
         */
        if((rg_config.rg_database_info.system == FBE_TRUE) && (FBE_CLASS_ID_VIRTUAL_DRIVE == server_class_id))
        {
            /* Store the VDs so that they can be displayed 
             * once we are done displaying the system luns and RGs
             */
            fbe_sepls_set_sys_vd_id(server_id);
        }
    }
    if(rg_config.rg_database_info.system == FBE_FALSE)
    {
        fbe_sepls_display_sys_vds(display_all_flag, display_format, ptr_size);
    }

    /* Get the client details of this RG to get to the LUNs
     */
    FBE_GET_FIELD_DATA(module_name, control_handle_ptr, fbe_base_config_t, width, sizeof(fbe_u32_t), &width); 
    FBE_GET_TYPE_SIZE(module_name, fbe_base_config_t*, &ptr_size);
    FBE_GET_FIELD_OFFSET(module_name,fbe_base_config_t,"block_transport_server",&offset);
    FBE_READ_MEMORY(control_handle_ptr + offset, &block_transport_p, ptr_size);
    if(block_transport_p != NULL)
    {
        FBE_READ_MEMORY(block_transport_p, &queue_head, sizeof(fbe_u64_t));
        queue_element = queue_head;
                                

        FBE_GET_FIELD_OFFSET(module_name, fbe_base_edge_t, "client_id", &offset);
        FBE_READ_MEMORY(block_transport_p + offset, &client_id, sizeof(fbe_u32_t));
        /* We first display the lun information from the RGs client list */
        lun_capacity = fbe_sepls_lun_info_trace(module_name, client_id, object_id, exported_block_size, display_format, trace_func, NULL);
        rg_config.total_lun_capacity += lun_capacity;
        while(queue_element != block_transport_p && queue_element != NULL)
        {
            if (FBE_CHECK_CONTROL_C())
            {
                return FBE_STATUS_CANCELED;
            }

            FBE_GET_FIELD_OFFSET(module_name, fbe_base_edge_t, "client_id", &offset);
            FBE_READ_MEMORY(queue_element + offset, &client_id, sizeof(fbe_u32_t));
            /* Process all the client elements in the queue */
            lun_capacity = fbe_sepls_lun_info_trace(module_name, client_id, object_id, exported_block_size, display_format, trace_func, NULL);
            rg_config.total_lun_capacity += lun_capacity;
            FBE_READ_MEMORY(queue_element, &queue_head, sizeof(fbe_u64_t));
            queue_element = queue_head;
        }
        /* We have finished displaying the lun objects,
         * so if the user has specified the "-lun" option
         * then we are done.
         */
        if(display_object == FBE_SEPLS_DISPLAY_ONLY_LUN_OBJECTS)
        {
            return FBE_STATUS_OK;
        }
    }
    /* Display RG objects based on the options specified by the user */
    if(display_object == FBE_SEPLS_DISPLAY_ONLY_RG_OBJECTS || 
       display_object == FBE_SEPLS_DISPLAY_OBJECTS_NONE ||
       display_level >= FBE_SEPLS_DISPLAY_LEVEL_1)
    {
        /* Once we are done with the LUN, we display the RG information next */
        fbe_sepls_display_rg_info_debug_trace(trace_func, trace_context, &rg_config);
        for ( edge_index = 0; edge_index < width; edge_index++)
        {
            trace_func(NULL,"%-3d ",server_ids[edge_index]);
            
            /* We will display only 8 downstream objects and drives in a row 
             * For Raid type R1_0 we will display 4 downstream objects and
             * 8 drives in a row.
             */
            if(raid_type == FBE_RAID_GROUP_TYPE_RAID10)
            {
                if(edge_index == 3)
                {
                    server_counter = edge_index + 1;
                    fbe_trace_indent(trace_func, trace_context,17);
                    break;
                }
            }
            else if(edge_index == 7)
            {
                server_counter = edge_index + 1; /* We have already displayed the 7th object */
                fbe_trace_indent(trace_func, trace_context,1);
                break;
            }
        }

        if(raid_type == FBE_RAID_GROUP_TYPE_RAID10)
        {
            if(width < 4)
            {
                fbe_trace_indent(trace_func, trace_context,(33- width *4));
            }
        }
        else if(width < 8)
        {
            fbe_trace_indent(trace_func, trace_context,(33- width *4));
        }
        rebuild_capacity = fbe_sepls_calculate_rebuild_capacity(&rg_config);
        for ( edge_index = 0; edge_index < width; edge_index++)
        {
            fbe_sepls_get_drives_info_debug_trace("sep",trace_func,trace_context,server_ids[edge_index]);
            for(loop_counter = 0; loop_counter < FBE_RAID_GROUP_MAX_REBUILD_POSITIONS; ++loop_counter)
            {
                if(rg_config.rebuild_info.rebuild_checkpoint_info[loop_counter].position == edge_index)
                {
                    if ((rg_config.rebuild_info.rebuild_checkpoint_info[loop_counter].checkpoint != FBE_LBA_INVALID) && 
                         rg_config.rebuild_info.rebuild_logging_bitmask == FBE_FALSE)
                    {
                        if(rg_config.state == FBE_LIFECYCLE_STATE_READY)
                        {
                            rebuild_percent[loop_counter] = (fbe_u16_t) ((rg_config.rebuild_info.rebuild_checkpoint_info[loop_counter].checkpoint * 100) / rebuild_capacity);
                            /* If rebuild percent > 100, means rebuild has
                             * completed,hence no need to display REB% now
                             */
                            if(rebuild_percent[loop_counter] > 100)
                            {
                                trace_func(NULL,"%-12s"," ");
                            }
                            else
                            {
                                trace_func(NULL,"(REB:%3d%%) ",rebuild_percent[loop_counter]);
                            }
                        }
                    }
                }
            }
            /* We will only display 8 drives in one Row */
            if(raid_type == FBE_RAID_GROUP_TYPE_RAID10)
            {
                if(edge_index == 3)
                {
                    break;
                }
            }
            else if(edge_index == 7)
            {
                break;
            }
        }
        trace_func(NULL,"\n");
        if(server_counter >= 4)
        {
            fbe_trace_indent(trace_func, trace_context,60);
            for ( edge_index = server_counter; edge_index < width; edge_index++)
            {
                trace_func(NULL,"%-3d ",server_ids[edge_index]);
            }
            fbe_trace_indent(trace_func, trace_context, (33- ((width-server_counter) *4)));
            for ( edge_index = server_counter; edge_index < width; edge_index++)
            {
                fbe_sepls_get_drives_info_debug_trace("sep",trace_func,trace_context,server_ids[edge_index]);
                for(loop_counter = 0; loop_counter < FBE_RAID_GROUP_MAX_REBUILD_POSITIONS; ++loop_counter)
                {
                    if(rg_config.rebuild_info.rebuild_checkpoint_info[loop_counter].position == edge_index)
                    {
                        if ((rg_config.rebuild_info.rebuild_checkpoint_info[loop_counter].checkpoint != FBE_LBA_INVALID) && 
                             rg_config.rebuild_info.rebuild_logging_bitmask == FBE_FALSE)
                        {
                            if(rg_config.state == FBE_LIFECYCLE_STATE_READY)
                            {
                                rebuild_percent[loop_counter] = (fbe_u16_t) ((rg_config.rebuild_info.rebuild_checkpoint_info[loop_counter].checkpoint * 100) / rebuild_capacity);
                                /* If rebuild percent > 100, means rebuild has
                                 * completed,hence no need to display REB% now
                                 */
                                if(rebuild_percent[loop_counter] > 100)
                                {
                                    trace_func(NULL,"%-12s"," ");
                                }
                                else
                                {
                                    trace_func(NULL,"(REB:%3d%%) ",rebuild_percent[loop_counter]);
                                }
                            }
                        }
                    }
                }
            }
            trace_func(NULL,"\n");
        }
        
    }

    /* If we have displayed all the system related VDs
     * then we can now display the user RGs related VDs
     */
    if(sys_vd_displayed == FBE_TRUE)
    {
        for (edge_index = 0; edge_index < width; edge_index++)
        {
            vd_displayed = FBE_FALSE;
            /* Here we check for user luns on system drives
             * Check whether they have already been displayed
             */
            for(vd_index = 0; vd_index < current_sys_vd_index; vd_index++)
            {
                if(sys_vd_ids[vd_index] == server_ids[edge_index])
                {
                    vd_displayed = FBE_TRUE;
                    break;
                }
            }
            /* Here we check for user VDs on other than system drives
             * Check whether they have already been displayed
             */
            if(vd_displayed == FBE_FALSE)
            {
                for(vd_index = 0; vd_index < current_vd_index; vd_index++)
                {
                    if(vd_ids[vd_index] == server_ids[edge_index])
                    {
                        vd_displayed = FBE_TRUE;
                        break;
                    }
                }
            }
            if(vd_displayed == FBE_FALSE)
            {
                /* If user has specified the -allsep option then only
                 * display the VDs and downstream objects
                 */
                if(display_all_flag == FBE_TRUE || display_format > FBE_SEPLS_DISPLAY_OBJECTS_NONE)
                {
                    if((class_id == FBE_CLASS_ID_STRIPER &&
                       raid_type == FBE_RAID_GROUP_TYPE_RAID10) &&
                       (display_object == FBE_SEPLS_DISPLAY_ONLY_RG_OBJECTS ||
                       display_object == FBE_SEPLS_DISPLAY_ONLY_VD_OBJECTS ||
                       display_object == FBE_SEPLS_DISPLAY_OBJECTS_NONE ||
                       display_level >= FBE_SEPLS_DISPLAY_LEVEL_1))
                    {
                        /* We take care of RAID1_0 differently
                         */
                        fbe_sepls_rg1_0_info_trace(module_name,server_ids[edge_index],rg_config.total_lun_capacity,rg_config.rg_database_info.rg_number, display_format, trace_func, NULL);
                    }
                    else if(display_all_flag == FBE_TRUE || 
                            display_object == FBE_SEPLS_DISPLAY_ONLY_VD_OBJECTS ||
                            display_level >= FBE_SEPLS_DISPLAY_LEVEL_3)
                    {
                        fbe_sepls_vd_info_trace(module_name, server_ids[edge_index],exported_block_size, display_format, trace_func, NULL);
                        vd_ids[current_vd_index] = server_ids[edge_index];
                        current_vd_index++;
                    }
                }
            }
        }
    }
 
    return FBE_STATUS_OK;
}
/********************************************
 * end fbe_sepls_rg_info_trace
 ********************************************/

/*!**************************************************************
 * fbe_sepls_get_rg_persist_data()
 ****************************************************************
 * @brief
 *  Populate the raid group data.
 *  
 * @param module_name - Module name to use in symbol lookup.
 * @param raid_group_p - ptr to rg object.
 * @param fbe_raid_group_database_info_t* - Pointer to the rg database to be filled
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * 
 * @return fbe_status_t
 *
 * @author
 *  25/05/2011 - Created. Omer Miranda
 *
 ****************************************************************/
fbe_status_t fbe_sepls_get_rg_persist_data(const fbe_u8_t* module_name,
                                           fbe_dbgext_ptr rg_object_ptr,
                                           fbe_dbgext_ptr rg_user_ptr,
                                           fbe_raid_group_database_info_t *rg_data_ptr,
                                           fbe_trace_func_t trace_func,
                                           fbe_trace_context_t trace_context)
{
    fbe_u32_t offset = 0;
    fbe_bool_t system;
    fbe_raid_group_number_t rg_number;
    fbe_lba_t capacity;
    fbe_raid_group_configuration_t rg_config;
    database_rg_user_data_t rg_user_data;

    FBE_READ_MEMORY(rg_object_ptr, &rg_config, sizeof(fbe_raid_group_configuration_t));
    FBE_READ_MEMORY(rg_user_ptr, &rg_user_data, sizeof(database_rg_user_data_t));

    FBE_GET_FIELD_OFFSET(module_name, database_rg_user_data_t, "is_system", &offset);
    FBE_READ_MEMORY(rg_user_ptr +offset, &system, sizeof(fbe_bool_t));
    rg_data_ptr->system = system;
            
    FBE_GET_FIELD_OFFSET(module_name, database_rg_user_data_t, "raid_group_number", &offset);
    FBE_READ_MEMORY(rg_user_ptr +offset, &rg_number, sizeof(fbe_raid_group_number_t));
    rg_data_ptr->rg_number = rg_number;
    
    FBE_GET_FIELD_OFFSET(module_name, fbe_raid_group_configuration_t, "capacity", &offset);
    FBE_READ_MEMORY(rg_object_ptr + offset, &capacity, sizeof(fbe_lba_t));
    rg_data_ptr->capacity = capacity;

    return FBE_STATUS_OK;
}
/********************************************
 * end fbe_sepls_get_rg_persist_data
 ********************************************/

/*!**************************************************************
 * fbe_sepls_display_rg_info_debug_trace()
 ****************************************************************
 * @brief
 *  Display the Raid group data.
 *  
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param fbe_sepls_rg_config_t - Pointer to RG info structure.
 * 
 * @return fbe_status_t 
 *
 * @author
 *  25/05/2011 - Created. Omer Miranda
 *
 ****************************************************************/
fbe_status_t fbe_sepls_display_rg_info_debug_trace(fbe_trace_func_t trace_func ,
                                                   fbe_trace_context_t trace_context,
                                                   fbe_sepls_rg_config_t *rg_data_ptr)
{

    trace_func(NULL,"%-8s","RG");
    trace_func(NULL,"0x%-3x :%-6d",rg_data_ptr->object_id,rg_data_ptr->object_id);

    trace_func(NULL,"%-6d",rg_data_ptr->rg_database_info.rg_number);

    fbe_sepls_display_rg_type_debug_trace(trace_func,trace_context,rg_data_ptr);
    fbe_sepls_display_object_state_debug_trace(trace_func,trace_context,rg_data_ptr->state);

    if (rg_data_ptr->encryption_info.flags != 0) {
        fbe_lba_t encryption_capacity;
        fbe_u16_t encryption_percent;
        encryption_capacity = fbe_sepls_calculate_encryption_capacity(rg_data_ptr);
        encryption_percent = (fbe_u16_t) ((rg_data_ptr->encryption_info.rekey_checkpoint * 100) / encryption_capacity);
        if (encryption_percent > 100) {
            encryption_percent = 100;
        }
        trace_func(NULL,"(ENC:%3d%%) ", encryption_percent);
    }
    else {
        if (rg_data_ptr->state == FBE_LIFECYCLE_STATE_READY) {
            trace_func(NULL,"%-12s"," ");
        } else {
            trace_func(NULL,"%-8s"," ");
        }
    }

    return FBE_STATUS_OK;
}
/********************************************
 * end fbe_sepls_display_rg_info_debug_trace
 ********************************************/

/*!**************************************************************
 * fbe_sepls_r1_0_info_trace()
 ****************************************************************
 * @brief
 *  populate the mirror information for raid1_0.
 *  
 * @param module_name - Module name to use in symbol lookup.
 * @param fbe_object_id_t - mirror object id.
 * @param fbe_lba_t - Total lun capacity for the RG.
 * @param rg_number - Base Raid groups ID
 * @param display_format - Bitmap to indicate user optins
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * 
 * @return fbe_status_t
 *
 * @author
 *  01/06/2011 - Created. Omer Miranda
 *
 ****************************************************************/
fbe_status_t fbe_sepls_rg1_0_info_trace(const fbe_u8_t * module_name,
                                       fbe_object_id_t mirror_id,
                                       fbe_lba_t lun_capacity,
                                       fbe_raid_group_number_t rg_number,
                                       fbe_u32_t display_format,
                                       fbe_trace_func_t trace_func,
                                       fbe_trace_context_t trace_context)
{
    fbe_dbgext_ptr topology_object_table_ptr = 0;
    fbe_topology_object_status_t  object_status = 0;
    fbe_dbgext_ptr control_handle_ptr = 0;
    fbe_u32_t topology_object_table_entry_size = 0;
    fbe_class_id_t class_id = FBE_CLASS_ID_INVALID;
    fbe_object_id_t object_id;
    fbe_lifecycle_state_t lifecycle_state;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_dbgext_ptr geo_ptr = 0;
    fbe_raid_group_type_t   raid_type;
    fbe_block_size_t exported_block_size;
    fbe_u32_t width;
    fbe_object_id_t server_ids[FBE_RAID_GROUP_MAX_DISK_ARRAY_WIDTH /* Max number of drives */];
    fbe_u32_t offset = 0;
    fbe_u32_t ptr_size = 0;
    fbe_u32_t block_edge_ptr_offset;
    fbe_dbgext_ptr base_config_ptr = 0;
    fbe_u64_t block_edge_p = 0;
    fbe_u32_t block_edge_size = 0;
    fbe_object_id_t     server_id;
    fbe_dbgext_ptr base_edge_ptr = 0;
    fbe_u32_t edge_index = 0;
    fbe_sepls_rg_config_t rg_config ={0};
    fbe_raid_group_database_info_t rg_database_info ={0};
    fbe_u32_t display_object = (display_format & FBE_SEPLS_DISPLAY_FORMAT_MASK);
    fbe_u32_t display_level = (display_format & FBE_SEPLS_DISPLAY_FORMAT_MASK);
    fbe_u16_t rebuild_percent;
    fbe_lba_t rebuild_capacity;
    fbe_dbgext_ptr topology_object_tbl_ptr = 0;
    fbe_u32_t vd_index=0;
    fbe_bool_t vd_displayed;
    status = fbe_debug_get_ptr_size(module_name, &ptr_size);
    if (status != FBE_STATUS_OK) 
    {
        trace_func(NULL, "%s unable to get ptr size status: %d\n", __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE; 
    }

    /* Get the topology table. */
    FBE_GET_EXPRESSION(module_name, topology_object_table, &topology_object_table_ptr);
    FBE_READ_MEMORY(topology_object_table_ptr, &topology_object_tbl_ptr, ptr_size);


    /* Get the size of the topology entry. */
    FBE_GET_TYPE_SIZE(module_name, topology_object_table_entry_s, &topology_object_table_entry_size);

    if(topology_object_tbl_ptr == 0)
    {
        trace_func(NULL, "\t topology_object_table_is not available \n");
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Point to the object we are looking for 
     */
    topology_object_tbl_ptr += (topology_object_table_entry_size * mirror_id);

    /* Fetch the object's topology status.
     */
    FBE_GET_FIELD_OFFSET(module_name,topology_object_table_entry_t,"object_status",&offset);
    FBE_READ_MEMORY(topology_object_tbl_ptr + offset, &object_status, sizeof(fbe_topology_object_status_t));
    /* Fetch the control handle, which is the pointer to the object.
     */
    FBE_GET_FIELD_OFFSET(module_name,topology_object_table_entry_t,"control_handle",&offset);
    FBE_READ_MEMORY(topology_object_tbl_ptr + offset, &control_handle_ptr, sizeof(fbe_dbgext_ptr));

    /* Need to display the objects even if they are not in the ready state
     */
    if (object_status == FBE_TOPOLOGY_OBJECT_STATUS_READY ||
        object_status == FBE_TOPOLOGY_OBJECT_STATUS_EXIST ||
        object_status == FBE_TOPOLOGY_OBJECT_STATUS_RESERVED)
    {
        /* Start populating the VD info */
        FBE_GET_FIELD_DATA(module_name, 
                           control_handle_ptr,
                           fbe_base_object_s,
                           class_id,
                           sizeof(fbe_class_id_t),
                           &class_id);

        FBE_GET_FIELD_DATA(module_name, 
                           control_handle_ptr,
                           fbe_base_object_s,
                           object_id,
                           sizeof(fbe_object_id_t),
                           &object_id);

        rg_config.object_id = object_id;
        rg_config.total_lun_capacity = lun_capacity;

        FBE_GET_FIELD_DATA(module_name,
                           control_handle_ptr,
                           fbe_base_object_s,
                           lifecycle.state,
                           sizeof(fbe_lifecycle_state_t),
                           &lifecycle_state);

        rg_config.state = lifecycle_state;

        /* Get the Raid type info */
        FBE_GET_FIELD_OFFSET(module_name,fbe_raid_group_t,"geo",&offset);
        geo_ptr = control_handle_ptr + offset;
        FBE_GET_FIELD_OFFSET(module_name,fbe_raid_geometry_t,"exported_block_size",&offset);
        FBE_READ_MEMORY(geo_ptr + offset, &exported_block_size, sizeof(fbe_block_size_t));
        FBE_GET_FIELD_OFFSET(module_name,fbe_raid_geometry_t,"raid_type",&offset);
        FBE_READ_MEMORY(geo_ptr + offset, &raid_type, sizeof(fbe_raid_group_type_t));
        rg_config.raid_type = raid_type;

        fbe_sepls_get_config_data_debug_trace(module_name, object_id, &rg_database_info, trace_func, trace_context);
        rg_config.rg_database_info.system = rg_database_info.system;
        rg_config.rg_database_info.capacity = rg_database_info.capacity;
        /* We need to take the base RG's Rg-number */
        rg_config.rg_database_info.rg_number = rg_number;

        /* Get the down stream object and drives details */
        FBE_GET_FIELD_OFFSET(module_name,fbe_raid_group_t,"base_config",&offset);
        base_config_ptr = control_handle_ptr + offset;

        /* Populate the rebuild information */
        fbe_sepls_get_rg_rebuild_info(module_name,base_config_ptr,&rg_config.rebuild_info);

        FBE_GET_FIELD_DATA(module_name,
                           base_config_ptr,
                           fbe_base_config_t,
                           width, sizeof(fbe_u32_t),
                           &width);
        rg_config.width = width;

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
            server_ids[edge_index] = server_id;
        }

        /* Display RG objects based on user option */
        if(display_object == FBE_SEPLS_DISPLAY_ONLY_RG_OBJECTS || 
           display_object == FBE_SEPLS_DISPLAY_OBJECTS_NONE ||
           display_level >= FBE_SEPLS_DISPLAY_LEVEL_1)
        {
            fbe_sepls_display_rg1_0_info_debug_trace(trace_func,trace_context, &rg_config);
            for ( edge_index = 0; edge_index < width; edge_index++)
            {
                trace_func(NULL,"%-3d ",server_ids[edge_index]);
            }

            fbe_trace_indent(trace_func, trace_context,(33- width *4));
            rebuild_capacity = fbe_sepls_calculate_rebuild_capacity(&rg_config);
            for ( edge_index = 0; edge_index < width; edge_index++)
            {
                fbe_sepls_get_drives_info_debug_trace("sep",trace_func,trace_context,server_ids[edge_index]);
                if(rg_config.rebuild_info.rebuild_checkpoint_info[0].position == edge_index)
                {
                    if ((rg_config.rebuild_info.rebuild_checkpoint_info[0].checkpoint != FBE_LBA_INVALID) && 
                         rg_config.rebuild_info.rebuild_logging_bitmask == FBE_FALSE)
                    {
                        if(rg_config.state == FBE_LIFECYCLE_STATE_READY)
                        {
                            rebuild_percent = (fbe_u16_t) ((rg_config.rebuild_info.rebuild_checkpoint_info[0].checkpoint * 100) / rebuild_capacity);
                            /* If rebuild percent > 100, means rebuild has
                             * completed,hence no need to display REB% now
                             */
                            if(rebuild_percent > 100)
                            {
                                trace_func(NULL,"%-12s"," ");
                            }
                            else
                            {
                                trace_func(NULL,"(REB:%3d%%) ",rebuild_percent);
                            }
                        }
                    }
                }
            }
            trace_func(NULL,"\n");
        }

        /* Traverse to VD only if the user has specified so 
         */
        if(display_object == FBE_SEPLS_DISPLAY_OBJECTS_NONE ||
           display_object == FBE_SEPLS_DISPLAY_ONLY_VD_OBJECTS ||
           display_level >= FBE_SEPLS_DISPLAY_LEVEL_3)
        {
            for ( edge_index = 0; edge_index < width; edge_index++)
            {
                /* Here we check for user luns on other than system drives
                 * Check whether they have already been displayed
                 */
                vd_displayed = FBE_FALSE;
                for(vd_index = 0; vd_index < current_vd_index; vd_index++)
                {
                    if(vd_ids[vd_index] == server_ids[edge_index])
                    {
                        vd_displayed = FBE_TRUE;
                        break;
                    }
                }
                if(vd_displayed == FBE_FALSE)
                {
                    fbe_sepls_vd_info_trace(module_name, server_ids[edge_index],exported_block_size, display_format, trace_func, NULL);
                    vd_ids[current_vd_index] = server_ids[edge_index];
                    current_vd_index++;
                }
            }

        }
    }
    return FBE_STATUS_OK;
}
/********************************************
 * end fbe_sepls_rg1_0_info_trace
 ********************************************/

/*!**************************************************************
 * fbe_sepls_display_rg1_0_info_debug_trace()
 ****************************************************************
 * @brief
 *  Display the Raid1_0 data.
 *  
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * 
 * @return fbe_status_t
 *
 * @author
 *  25/05/2011 - Created. Omer Miranda
 *
 ****************************************************************/
fbe_status_t fbe_sepls_display_rg1_0_info_debug_trace(fbe_trace_func_t trace_func ,
                                                      fbe_trace_context_t trace_context,
                                                      fbe_sepls_rg_config_t *rg_data_ptr)
{
    trace_func(NULL,"%-8s","RG");
    trace_func(NULL,"0x%-3x :%-6d",rg_data_ptr->object_id,rg_data_ptr->object_id);
    
    trace_func(NULL,"%-6d",rg_data_ptr->rg_database_info.rg_number);

    fbe_sepls_display_rg_type_debug_trace(trace_func,trace_context,rg_data_ptr);
    
    fbe_sepls_display_object_state_debug_trace(trace_func,trace_context,rg_data_ptr->state);
    if(rg_data_ptr->state == FBE_LIFECYCLE_STATE_READY)
    {
        trace_func(NULL,"%-12s"," ");
    }
    else
    {
        trace_func(NULL,"%-8s"," ");
    }

    return FBE_STATUS_OK;
}
/********************************************
 * end fbe_sepls_display_rg1_0_info_debug_trace
 ********************************************/

/*!**************************************************************
 * fbe_sepls_display_rg_type_debug_trace()
 ****************************************************************
 * @brief
 *  Display the Raid type.
 *  
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * 
 * @return fbe_status_t
 *
 * @author
 *  22/06/2011 - Created. Omer Miranda
 *
 ****************************************************************/
fbe_status_t fbe_sepls_display_rg_type_debug_trace(fbe_trace_func_t trace_func ,
                                                   fbe_trace_context_t trace_context,
                                                   fbe_sepls_rg_config_t *rg_data_ptr)
{
    switch(rg_data_ptr->raid_type)
    {
        case FBE_RAID_GROUP_TYPE_RAID1:
            trace_func(NULL,"%-15s","RAID1");
            break;
        case FBE_RAID_GROUP_TYPE_RAID10:
            trace_func(NULL,"%-15s","RAID10");
            break;
        case FBE_RAID_GROUP_TYPE_RAID3:
            trace_func(NULL,"%-15s","RAID3");
            break;
        case FBE_RAID_GROUP_TYPE_RAID0:
            trace_func(NULL,"%-15s","RAID0");
            break;
        case FBE_RAID_GROUP_TYPE_RAID5:
            trace_func(NULL,"%-15s","RAID5");
            break;
        case FBE_RAID_GROUP_TYPE_RAID6:
            trace_func(NULL,"%-15s","RAID6");
            break;
        case FBE_RAID_GROUP_TYPE_INTERNAL_MIRROR_UNDER_STRIPER:
            trace_func(NULL,"%-15s","MIRROR");
            break;
        case FBE_RAID_GROUP_TYPE_RAW_MIRROR:
            trace_func(NULL,"%-15s","RAW-MIRROR");
            break;
        default:
            trace_func(NULL,"%-15s","UNKNOWN");
            break;
    }
    return FBE_STATUS_OK;
}
/********************************************
 * end fbe_sepls_display_rg_type_debug_trace
 ********************************************/

/*!**************************************************************
 * fbe_sepls_get_rg_rebuild_info()
 ****************************************************************
 * @brief
 *  Populate the rebuild information.
 *  
 * @param module_name - Module name to use in symbol lookup.
 * @param fbe_dbgext_ptr - Raid group base config ptr.
 * @param fbe_sepls_rg_config_t * - Pointer to the RG data structure where we 
 *                                 populate the information.
 * 
 * @return fbe_status_t
 *
 * @author
 *  29/06/2011 - Created. Omer Miranda
 *
 ****************************************************************/
fbe_status_t fbe_sepls_get_rg_rebuild_info(const fbe_u8_t * module_name,
                                           fbe_dbgext_ptr rg_base_config_ptr,
                                           void *rb_info_p)
{
    fbe_u32_t offset;
    fbe_dbgext_ptr fbe_metadata_ptr;
    fbe_dbgext_ptr nonpaged_record_ptr;
    fbe_dbgext_ptr data_ptr;
    fbe_dbgext_ptr rg_rebuild_info_ptr;
    fbe_dbgext_ptr rebuild_checkpoint_ptr;
    fbe_raid_position_bitmask_t rebuild_logging_bitmask;
    fbe_u32_t loop_counter = 0;
    fbe_u32_t checkpoint_offset;
    fbe_u32_t position_offset;
    fbe_u32_t checkpoint_info_size;
    fbe_lba_t checkpoint;
    fbe_u32_t position;
    fbe_raid_group_rebuild_nonpaged_info_t *rebuild_info_p = (fbe_raid_group_rebuild_nonpaged_info_t*)rb_info_p;

    FBE_GET_FIELD_OFFSET(module_name, fbe_base_config_t, "metadata_element", &offset);
    fbe_metadata_ptr = rg_base_config_ptr + offset;

    FBE_GET_FIELD_OFFSET(module_name, fbe_metadata_element_t, "nonpaged_record", &offset);
    nonpaged_record_ptr = fbe_metadata_ptr + offset;

    FBE_GET_FIELD_OFFSET(module_name, fbe_metadata_record_t, "data_ptr", &offset);
    FBE_READ_MEMORY(nonpaged_record_ptr + offset, &data_ptr, sizeof(fbe_dbgext_ptr));	

    FBE_GET_FIELD_OFFSET(module_name, fbe_raid_group_nonpaged_metadata_t, "rebuild_info", &offset);
    rg_rebuild_info_ptr = data_ptr + offset;

    FBE_GET_FIELD_OFFSET(module_name, fbe_raid_group_rebuild_nonpaged_info_t, "rebuild_logging_bitmask", &offset);
    FBE_READ_MEMORY(rg_rebuild_info_ptr + offset, &rebuild_logging_bitmask, sizeof(fbe_raid_position_bitmask_t));

    rebuild_info_p->rebuild_logging_bitmask = rebuild_logging_bitmask;

    FBE_GET_FIELD_OFFSET(module_name, fbe_raid_group_rebuild_nonpaged_info_t, "rebuild_checkpoint_info", &offset);
    rebuild_checkpoint_ptr = rg_rebuild_info_ptr + offset;

    FBE_GET_FIELD_OFFSET(module_name, fbe_raid_group_rebuild_checkpoint_info_t, "checkpoint", &checkpoint_offset);
    FBE_GET_FIELD_OFFSET(module_name, fbe_raid_group_rebuild_checkpoint_info_t, "position", &position_offset);
    FBE_GET_TYPE_SIZE(module_name, fbe_raid_group_rebuild_checkpoint_info_t, &checkpoint_info_size);

    for(loop_counter = 0; loop_counter < FBE_RAID_GROUP_MAX_REBUILD_POSITIONS; ++loop_counter)
    {
        FBE_READ_MEMORY(rebuild_checkpoint_ptr + checkpoint_offset, &checkpoint, sizeof(fbe_lba_t));
        rebuild_info_p->rebuild_checkpoint_info[loop_counter].checkpoint = checkpoint;
        FBE_READ_MEMORY(rebuild_checkpoint_ptr + position_offset, &position, sizeof(fbe_u32_t));
        rebuild_info_p->rebuild_checkpoint_info[loop_counter].position = position;

        rebuild_checkpoint_ptr += checkpoint_info_size;
    }

    return FBE_STATUS_OK;
}
/********************************************
 * end fbe_sepls_get_rg_rebuild_info
 ********************************************/

/*!**************************************************************
 * fbe_sepls_calculate_rebuild_capacity()
 ****************************************************************
 * @brief
 *  Calculate the rebuild capacity which will help in calculating the
 *  rebuild percentage.
 *  
 * @param fbe_sepls_rg_config_t - Pointer to the RG data structure where we 
 *                                 populate the information.
 * 
 * @return fbe_lba_t - Rebuild capacity
 *
 * @author
 *  04/07/2011 - Created. Omer Miranda
 *
 ****************************************************************/
fbe_lba_t fbe_sepls_calculate_rebuild_capacity(fbe_sepls_rg_config_t *rg_data_ptr)
{
    fbe_lba_t rebuild_capacity;

    if(rg_data_ptr->raid_type == FBE_RAID_GROUP_TYPE_RAID3 ||
       rg_data_ptr->raid_type == FBE_RAID_GROUP_TYPE_RAID5)
    {
        rebuild_capacity = rg_data_ptr->total_lun_capacity/(rg_data_ptr->width - 1);
    }
    else if ((rg_data_ptr->raid_type == FBE_RAID_GROUP_TYPE_RAID1) ||
             (rg_data_ptr->raid_type == FBE_RAID_GROUP_TYPE_RAW_MIRROR))
    {
        rebuild_capacity = rg_data_ptr->total_lun_capacity;
    }
    else if(rg_data_ptr->raid_type == FBE_RAID_GROUP_TYPE_RAID0)
    {
        rebuild_capacity = rg_data_ptr->total_lun_capacity;
    }
    else if(rg_data_ptr->raid_type == FBE_RAID_GROUP_TYPE_RAID6)
    {
        rebuild_capacity = rg_data_ptr->total_lun_capacity/(rg_data_ptr->width - 2);
    }
    else if(rg_data_ptr->raid_type == FBE_RAID_GROUP_TYPE_RAID10 ||
            rg_data_ptr->raid_type ==FBE_RAID_GROUP_TYPE_INTERNAL_MIRROR_UNDER_STRIPER)
    {
        rebuild_capacity = rg_data_ptr->total_lun_capacity/(rg_data_ptr->width);
    }
    else
        rebuild_capacity = FBE_LBA_INVALID;

    return rebuild_capacity;
}
/********************************************
 * end fbe_sepls_calculate_rebuild_capacity
 ********************************************/

/*!**************************************************************
 * fbe_sepls_calculate_encryption_capacity()
 ****************************************************************
 * @brief
 *  Calculate the rebuild capacity which will help in calculating the
 *  rebuild percentage.
 *  
 * @param fbe_sepls_rg_config_t - Pointer to the RG data structure where we 
 *                                 populate the information.
 * 
 * @return fbe_lba_t - Rebuild capacity
 *
 * @author
 *  04/07/2011 - Created. Omer Miranda
 *
 ****************************************************************/
fbe_lba_t fbe_sepls_calculate_encryption_capacity(fbe_sepls_rg_config_t *rg_data_ptr)
{
    fbe_lba_t encryption_capacity;

    if(rg_data_ptr->raid_type == FBE_RAID_GROUP_TYPE_RAID3 ||
       rg_data_ptr->raid_type == FBE_RAID_GROUP_TYPE_RAID5)
    {
        encryption_capacity = rg_data_ptr->rg_capacity/(rg_data_ptr->width - 1);
    }
    else if ((rg_data_ptr->raid_type == FBE_RAID_GROUP_TYPE_RAID1) ||
             (rg_data_ptr->raid_type == FBE_RAID_GROUP_TYPE_RAW_MIRROR))
    {
        encryption_capacity = rg_data_ptr->rg_capacity;
    }
    else if(rg_data_ptr->raid_type == FBE_RAID_GROUP_TYPE_RAID0)
    {
        encryption_capacity = rg_data_ptr->rg_capacity;
    }
    else if(rg_data_ptr->raid_type == FBE_RAID_GROUP_TYPE_RAID6)
    {
        encryption_capacity = rg_data_ptr->rg_capacity/(rg_data_ptr->width - 2);
    }
    else if(rg_data_ptr->raid_type == FBE_RAID_GROUP_TYPE_RAID10 ||
            rg_data_ptr->raid_type ==FBE_RAID_GROUP_TYPE_INTERNAL_MIRROR_UNDER_STRIPER)
    {
        encryption_capacity = rg_data_ptr->rg_capacity/(rg_data_ptr->width);
    }
    else
        encryption_capacity = FBE_LBA_INVALID;

    return encryption_capacity;
}
/********************************************
 * end fbe_sepls_calculate_encryption_capacity
 ********************************************/

/*!**************************************************************
 * fbe_sepls_get_rg_encryption_info()
 ****************************************************************
 * @brief
 *  Populate the encryption information.
 *  
 * @param module_name - Module name to use in symbol lookup.
 * @param fbe_dbgext_ptr - Raid group base config ptr.
 * @param fbe_sepls_rg_config_t * - Pointer to the RG data structure where we 
 *                                 populate the information.
 * 
 * @return fbe_status_t
 *
 * @author
 *  1/8/2014 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_sepls_get_rg_encryption_info(const fbe_u8_t * module_name,
                                              fbe_dbgext_ptr rg_base_config_ptr,
                                              void *encr_info_p)
{
    fbe_u32_t offset;
    fbe_dbgext_ptr fbe_metadata_ptr;
    fbe_dbgext_ptr nonpaged_record_ptr;
    fbe_dbgext_ptr data_ptr;
    fbe_dbgext_ptr rg_encryption_info_ptr;
    fbe_raid_group_encryption_flags_t flags;
    fbe_lba_t encryption_chkpt;
    fbe_raid_group_encryption_nonpaged_info_t *encryption_info_p = (fbe_raid_group_encryption_nonpaged_info_t*)encr_info_p;

    FBE_GET_FIELD_OFFSET(module_name, fbe_base_config_t, "metadata_element", &offset);
    fbe_metadata_ptr = rg_base_config_ptr + offset;

    FBE_GET_FIELD_OFFSET(module_name, fbe_metadata_element_t, "nonpaged_record", &offset);
    nonpaged_record_ptr = fbe_metadata_ptr + offset;

    FBE_GET_FIELD_OFFSET(module_name, fbe_metadata_record_t, "data_ptr", &offset);
    FBE_READ_MEMORY(nonpaged_record_ptr + offset, &data_ptr, sizeof(fbe_dbgext_ptr));	

    FBE_GET_FIELD_OFFSET(module_name, fbe_raid_group_nonpaged_metadata_t, "encryption", &offset);
    rg_encryption_info_ptr = data_ptr + offset;

    FBE_GET_FIELD_OFFSET(module_name, fbe_raid_group_encryption_nonpaged_info_t, "rekey_checkpoint", &offset);
    FBE_READ_MEMORY(rg_encryption_info_ptr + offset, &encryption_chkpt, sizeof(fbe_lba_t));
    encryption_info_p->rekey_checkpoint = encryption_chkpt;

    FBE_GET_FIELD_OFFSET(module_name, fbe_raid_group_encryption_nonpaged_info_t, "flags", &offset);
    FBE_READ_MEMORY(rg_encryption_info_ptr + offset, &flags, sizeof(fbe_raid_group_encryption_flags_t));
    encryption_info_p->flags = flags;
    return FBE_STATUS_OK;
}
/********************************************
 * end fbe_sepls_get_rg_encryption_info
 ********************************************/

/*************************
 * end file fbe_raid_group_debug.c
 *************************/
