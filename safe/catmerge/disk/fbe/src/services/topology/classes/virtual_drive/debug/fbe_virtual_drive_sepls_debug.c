/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_virtual_drive_sepls_debug.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the sepls related functions for the VD object.
 *  This  file contains functions which will aid in displaying VD related
 *  information for the sepls debug macro.
 *
 * @author
 *  01/06/2011 - Created. Omer Miranda
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
#include "fbe/fbe_virtual_drive.h"
#include "fbe_virtual_drive_private.h"
#include "fbe_base_object_trace.h"
#include "fbe_virtual_drive_debug.h"
#include "fbe_virtual_drive_sepls_debug.h"
#include "fbe_raid_library_debug.h"
#include "fbe_base_config_debug.h"
#include "fbe_sepls_debug.h"
#include "fbe_provision_drive_debug.h"
#include "fbe_raid_group_debug.h"
#include "fbe_topology_debug.h"


/*************************
 *   FUNCTION DEFINITIONS
 *************************/
/*!**************************************************************
 * fbe_sepls_get_vd_downstream_object()
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
 *  4/11/2012 - Created. Rob Foley
 *
 ****************************************************************/
fbe_object_id_t fbe_sepls_get_vd_downstream_object(const fbe_u8_t* module_name,
                                                   fbe_dbgext_ptr object_ptr,
                                                   fbe_trace_func_t trace_func,
                                                   fbe_trace_context_t trace_context,
                                                   fbe_sepls_vd_config_t *vd_config_p)
{
    fbe_u32_t width;
    fbe_u32_t base_edge_offset = 0;
    fbe_u32_t server_offset = 0;
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
    FBE_GET_FIELD_OFFSET(module_name, fbe_block_edge_t, "base_edge", &base_edge_offset);
    FBE_GET_FIELD_OFFSET(module_name, fbe_base_edge_t, "server_id", &server_offset);

    for ( edge_index = 0; edge_index < width; edge_index++)
    {
        if (FBE_CHECK_CONTROL_C())
        {
            return FBE_STATUS_CANCELED;
        }
        base_edge_ptr = block_edge_p + (block_edge_size * edge_index) + base_edge_offset;
        FBE_READ_MEMORY(base_edge_ptr + server_offset, &server_id, sizeof(fbe_object_id_t));
        vd_config_p->server_id[edge_index] = server_id;
    }

    return FBE_STATUS_OK;
}
/***************************************************
 * end fbe_sepls_get_vd_downstream_object
 ***************************************************/
/*!**************************************************************
 * fbe_sepls_vd_info_trace()
 ****************************************************************
 * @brief
 *  Populate the Virtual drive information.
 *  
 * @param module_name - Module name to use in symbol lookup.
 * @param fbe_object_id_t - VD object id.
 * @param fbe_lba_t - Capacity of the VD
 * @param display_format - Bitmap to indicate the user options
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * 
 * @return fbe_status_t
 *
 * @author
 *  01/06/2011 - Created. Omer Miranda
 *
 ****************************************************************/
fbe_status_t fbe_sepls_vd_info_trace(const fbe_u8_t * module_name,
                                     fbe_object_id_t vd_id,
                                     fbe_block_size_t exported_block_size,
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
    fbe_u32_t offset = 0;
    fbe_sepls_vd_config_t vd_config ={0};
	fbe_vd_database_info_t vd_database_info = {0};
    fbe_u32_t display_object = (display_format & FBE_SEPLS_DISPLAY_FORMAT_MASK);
    fbe_u32_t display_level = (display_format & FBE_SEPLS_DISPLAY_FORMAT_MASK);
	fbe_dbgext_ptr topology_object_tbl_ptr = 0;
	fbe_u32_t ptr_size;
	fbe_pvd_database_info_t pvd_database_info = {0};

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
    topology_object_tbl_ptr += (topology_object_table_entry_size * vd_id);

    /* Fetch the object's topology status.
     */
    FBE_GET_FIELD_OFFSET(module_name,topology_object_table_entry_t,"object_status",&offset);
    FBE_READ_MEMORY(topology_object_tbl_ptr + offset, &object_status, sizeof(fbe_topology_object_status_t));
    /* Fetch the control handle, which is the pointer to the object.
     */
    FBE_GET_FIELD_OFFSET(module_name,topology_object_table_entry_t,"control_handle",&offset);
    FBE_READ_MEMORY(topology_object_tbl_ptr + offset, &control_handle_ptr, sizeof(fbe_dbgext_ptr));

    if (object_status == FBE_TOPOLOGY_OBJECT_STATUS_READY ||
        object_status == FBE_TOPOLOGY_OBJECT_STATUS_EXIST ||
        object_status == FBE_TOPOLOGY_OBJECT_STATUS_RESERVED)
    {
        fbe_u32_t offset = 0;

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

        vd_config.object_id = object_id;

        FBE_GET_FIELD_DATA(module_name,
                           control_handle_ptr,
                           fbe_base_object_s,
                           lifecycle.state,
                           sizeof(fbe_lifecycle_state_t),
                           &lifecycle_state);

        vd_config.state = lifecycle_state;
        
        FBE_GET_FIELD_DATA(module_name,
                           control_handle_ptr,
                           fbe_virtual_drive_s,
                           configuration_mode,
                           sizeof(fbe_virtual_drive_configuration_mode_t),
                           &vd_config.config_mode);

        /* Get the down stream object and drives details */
        /* Get the capacity of PVD */
        status = fbe_sepls_get_vd_downstream_object(module_name,control_handle_ptr,trace_func,trace_context, &vd_config);
        if (status != FBE_STATUS_OK)
        {
            return status;
        }
        if ((vd_config.config_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE) ||
            (vd_config.config_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE))
        {
            fbe_sepls_get_config_data_debug_trace(module_name, vd_config.server_id[0], &pvd_database_info, trace_func, trace_context);
        }
        else
        {
            fbe_sepls_get_config_data_debug_trace(module_name, vd_config.server_id[1], &pvd_database_info, trace_func, trace_context);
        }
        vd_database_info.imported_capacity = pvd_database_info.configured_capacity;
       /* Get the VD capacity */
        fbe_sepls_get_config_data_debug_trace(module_name, object_id, &vd_database_info, trace_func,trace_context);
		vd_config.vd_database_info.capacity = vd_database_info.capacity;

        FBE_GET_FIELD_OFFSET(module_name, fbe_raid_group_t, "base_config", &offset);
        fbe_sepls_get_rg_rebuild_info(module_name, control_handle_ptr + offset, &vd_config.rebuild_info);
    }

    /* If the user has specified the "-vd" option
     * then display only VD objects and dont traverse to the next level
     */
    if(display_object == FBE_SEPLS_DISPLAY_ONLY_VD_OBJECTS ||
       display_level == FBE_SEPLS_DISPLAY_LEVEL_3)
    {
        fbe_sepls_display_vd_info_debug_trace(trace_func, trace_context, exported_block_size, &vd_config);
        return FBE_STATUS_OK;
    }
    else if(display_object == FBE_SEPLS_DISPLAY_OBJECTS_NONE ||
            display_level == FBE_SEPLS_DISPLAY_LEVEL_4) /* From the VD now go to PVD */
    {
        fbe_sepls_display_vd_info_debug_trace(trace_func, trace_context, exported_block_size, &vd_config);

        if ((vd_config.config_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE) ||
            (vd_config.config_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE))
        {
            fbe_sepls_pvd_info_trace(module_name,vd_config.server_id[0], exported_block_size, FBE_FALSE, display_format, trace_func, NULL);
            fbe_sepls_pvd_info_trace(module_name,vd_config.server_id[1], exported_block_size, FBE_FALSE, display_format, trace_func, NULL);
        }
        else if (vd_config.config_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE)
        {
            fbe_sepls_pvd_info_trace(module_name,vd_config.server_id[0], exported_block_size, FBE_FALSE, display_format, trace_func, NULL);
        }
        else if (vd_config.config_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE)
        {
            fbe_sepls_pvd_info_trace(module_name,vd_config.server_id[1], exported_block_size, FBE_FALSE, display_format, trace_func, NULL);
        }
    }
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_sepls_vd_info_trace()
 ******************************************/

/*!**************************************************************
 * fbe_sepls_display_vd_info_debug_trace()
 ****************************************************************
 * @brief
 *  Display the Virtual Drive data.
 *  
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param exported_block_size - Will be needed to calculate the VD capacity
 * @param fbe_sepls_vd_config_t * - Pointer to the VD info structure to be displayed.
 * 
 * @return fbe_status_t
 *
 * @author
 *  01/06/2011 - Created. Omer Miranda
 *
 ****************************************************************/
fbe_status_t fbe_sepls_display_vd_info_debug_trace(fbe_trace_func_t trace_func ,
                                                   fbe_trace_context_t trace_context,
                                                   fbe_block_size_t exported_block_size,
                                                   fbe_sepls_vd_config_t *vd_data_ptr)
{
    fbe_u32_t   capacity_of_vd;
    fbe_u32_t   primary_index = FBE_EDGE_INDEX_INVALID;
    fbe_u32_t   secondary_index = FBE_EDGE_INDEX_INVALID;

    trace_func(NULL,"%-8s","VD");
    trace_func(NULL,"0x%-3x :%-6d",vd_data_ptr->object_id,vd_data_ptr->object_id);
    trace_func(NULL,"%-6s","-");
    capacity_of_vd = (fbe_u32_t)((vd_data_ptr->vd_database_info.capacity * exported_block_size)/(1024*1024));
    trace_func(NULL,"%-7d",capacity_of_vd);
    trace_func(NULL,"%-8s","MB");
    fbe_sepls_display_object_state_debug_trace(trace_func,trace_context,vd_data_ptr->state);
    switch (vd_data_ptr->config_mode)
    {
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE:
            primary_index = 0;
            break;
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE:
            primary_index = 1;
            break;
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE:
            primary_index = 0;
            secondary_index = 1;
            break;
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE:
            primary_index = 1;
            secondary_index = 0;
            break;
    };
    if ((vd_data_ptr->config_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE) ||
        (vd_data_ptr->config_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE))
    {
        if(vd_data_ptr->state == FBE_LIFECYCLE_STATE_READY)
        {
            trace_func(NULL,"%-12s"," ");
        }
        else
        {
            trace_func(NULL,"%-8s"," ");
        }

        trace_func(NULL,"%-33d", vd_data_ptr->server_id[primary_index]);
        fbe_sepls_get_drives_info_debug_trace("sep",trace_func,trace_context,vd_data_ptr->server_id[primary_index]);
    }
    else
    {
        /* Else we are in mirror mode.  There should be only (1) rebuilding 
         * position.
         */
        fbe_u64_t percent = 0;
        fbe_u32_t percentage;
        fbe_lba_t rb_checkpoint = vd_data_ptr->rebuild_info.rebuild_checkpoint_info[0].checkpoint;
        fbe_u32_t rb_position = vd_data_ptr->rebuild_info.rebuild_checkpoint_info[0].position;

        if (rb_position == primary_index)
        {
            percentage = 100;
        }
        else
        {
            if (rb_checkpoint == FBE_LBA_INVALID)
            {
                percentage = 100;
            }
            else if (rb_checkpoint >= vd_data_ptr->vd_database_info.capacity)
            {
                percentage = 0;
            }
            else
            {
                percent = (rb_checkpoint * 100) / vd_data_ptr->vd_database_info.capacity;
                percentage = (fbe_u32_t)percent;
            }
        }
        if(vd_data_ptr->state == FBE_LIFECYCLE_STATE_READY)
        {
            trace_func(NULL,"COPY:%3d%%", percentage);
            trace_func(NULL,"%-3s"," ");
        }
        else
        {
            trace_func(NULL,"COPY:%3d%%", percentage);
            trace_func(NULL,"%-3s"," ");
        }
        if (vd_data_ptr->config_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE)
        {
            trace_func(NULL,"%-3d -> %-26d", vd_data_ptr->server_id[0], vd_data_ptr->server_id[1]);
            fbe_sepls_get_drives_info_debug_trace("sep",trace_func,trace_context,vd_data_ptr->server_id[0]);
            trace_func(NULL," -> ");
            fbe_sepls_get_drives_info_debug_trace("sep",trace_func,trace_context,vd_data_ptr->server_id[1]);
        }
        else
        {
            trace_func(NULL,"%-3d <- %-26d", vd_data_ptr->server_id[0], vd_data_ptr->server_id[1]);
            fbe_sepls_get_drives_info_debug_trace("sep",trace_func,trace_context,vd_data_ptr->server_id[0]);
            trace_func(NULL," <- ");
            fbe_sepls_get_drives_info_debug_trace("sep",trace_func,trace_context,vd_data_ptr->server_id[1]);
        }
    }
    trace_func(NULL,"\n");
    return FBE_STATUS_OK;
}
/**********************************************
 * end fbe_sepls_display_vd_info_debug_trace()
 **********************************************/

/*!**************************************************************
 * fbe_sepls_get_vd_persist_data()
 ****************************************************************
 * @brief
 *  Populate the vd configuration data.
 *  
 * @param module_name - Module name to use in symbol lookup.
 * @param raid_group_p - ptr to vd object.
 * @param fbe_sepls_vd_config_t - Pointer to the VD info structure.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * 
 * @return fbe_status_t
 *
 * @author
 *  06/02/2011 - Created. Omer Miranda
 *
 ****************************************************************/
fbe_status_t fbe_sepls_get_vd_persist_data(const fbe_u8_t* module_name,
                                           fbe_dbgext_ptr vd_object_ptr,
                                           fbe_dbgext_ptr vd_user_ptr,
                                           fbe_vd_database_info_t *vd_data_ptr,
                                           fbe_trace_func_t trace_func,
                                           fbe_trace_context_t trace_context)
{
    fbe_database_control_vd_set_configuration_t vd_config;
    fbe_lba_t           paged_metadata_capacity; 
    fbe_status_t status;
    FBE_READ_MEMORY(vd_object_ptr, &vd_config, sizeof(fbe_database_control_vd_set_configuration_t));

    /* Round the imported capacity up to a chunk before calculating metadata capacity. */
    if (vd_data_ptr->imported_capacity % FBE_VIRTUAL_DRIVE_CHUNK_SIZE)
    {
        vd_data_ptr->imported_capacity += FBE_VIRTUAL_DRIVE_CHUNK_SIZE - (vd_data_ptr->imported_capacity % FBE_VIRTUAL_DRIVE_CHUNK_SIZE);
    }
    status = fbe_sepls_virtual_drive_get_metadata_capacity(vd_data_ptr->imported_capacity,
                                                      &paged_metadata_capacity);
    vd_data_ptr->capacity = vd_data_ptr->imported_capacity - paged_metadata_capacity;

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_sepls_get_vd_persist_data()
 ******************************************/
/*!****************************************************************************
 * fbe_sepls_virtual_drive_get_metadata_capacity()
 ******************************************************************************
 * @brief
 *  This function is used to calculate the metadata capacity
 *  based on the imported capacity provided by the caller.
 *
 * @param exported_capacity                 - Exported capacity for the LUN.
 *        virtual_drive_metadata_positions  - Pointer to the virtual drive
 *                                            metadata positions.
 *
 * @return fbe_status_t
 *
 * @author
 *  2/08/2012 - Created. Trupti Ghate
 *
 ******************************************************************************/
fbe_status_t  
fbe_sepls_virtual_drive_get_metadata_capacity(fbe_lba_t imported_capacity,
                                                   fbe_lba_t           *paged_metadata_capacity)

{
    fbe_lba_t   overall_chunks = 0;
    fbe_lba_t   bitmap_entries_per_block = 0;
    fbe_lba_t   paged_bitmap_blocks = 0;
    fbe_lba_t   paged_bitmap_capacity = 0;

    if(imported_capacity == FBE_LBA_INVALID)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* We need to round the edge capacity down to full virtual drive chunk size. */
    if (imported_capacity % FBE_VIRTUAL_DRIVE_CHUNK_SIZE)
    {
        /* Caller needs to provide rounded capacity always. */
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Compute overall number of chunks */
    overall_chunks = imported_capacity / FBE_VIRTUAL_DRIVE_CHUNK_SIZE;

    /* Compute how many chunk entries can be saved in (1) logical block.
     * Currently we assume that a block can hold a whole multiple of entries.
     * Round up to the optimal block size.
     */
    bitmap_entries_per_block = FBE_METADATA_BLOCK_DATA_SIZE / FBE_RAID_GROUP_CHUNK_ENTRY_SIZE;
    paged_bitmap_blocks = (overall_chunks + (bitmap_entries_per_block - 1)) / bitmap_entries_per_block;

    /* Round the number of metadata blocks to virtual drive chunk size. */
    if(paged_bitmap_blocks % FBE_VIRTUAL_DRIVE_CHUNK_SIZE)
    {
        paged_bitmap_blocks += FBE_VIRTUAL_DRIVE_CHUNK_SIZE - (paged_bitmap_blocks % FBE_VIRTUAL_DRIVE_CHUNK_SIZE);
    }

    /* Now multiply by the number of metadata copies for the total paged bitmat capacity. */
    paged_bitmap_capacity = paged_bitmap_blocks * FBE_VIRTUAL_DRIVE_NUMBER_OF_METADATA_COPIES;


    /* Paged metadata starting position. */
    *paged_metadata_capacity = paged_bitmap_capacity;
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_sepls_virtual_drive_get_metadata_capacity()
 ******************************************************************************/
/*************************
 * end file fbe_virtual_drive_debug.c
 *************************/
