/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_lun_sepls.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the sepls related functions for the lun object.
 *  This  file contains functions which will aid in displaying LUN related
 *  information for the sepls debug macro
 *
 * @author
 *  20/05/2011 - Created. Omer Miranda
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
#include "fbe_lun_private.h"
#include "fbe_base_object_trace.h"
#include "fbe_lun_debug.h"
#include "fbe_lun_sepls_debug.h"
#include "fbe/fbe_raid_group.h"
#include "fbe_raid_geometry.h"
#include "fbe_base_config_debug.h"
#include "fbe_raid_library_debug.h"
#include "fbe_sepls_debug.h"
#include "fbe_raid_group_debug.h"
#include "fbe_topology_debug.h"


/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/*!**************************************************************
 * fbe_sepls_lun_info_trace()
 ****************************************************************
 * @brief
 *  Populate the lun information.
 *  
 * @param module_name - Module name to use in symbol lookup.
 * @param lun_id - The lun for which we need to populate the info.
 * @param rg_id - This lun belongs to this RG
 * @param exported_block_size - Will be needed to calculate the lun capacity
 * @param display_format - Bitmap to indicate the user options
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * 
 * @return fbe_lba_t - This will help us to find he total lun capacity in the RG
 *
 * @author
 *  20/05/2011 - Created. Omer Miranda
 *
 ****************************************************************/
fbe_lba_t fbe_sepls_lun_info_trace(const fbe_u8_t * module_name,
                                   fbe_object_id_t lun_id,
                                   fbe_object_id_t rg_id,
                                   fbe_block_size_t exported_block_size,
                                   fbe_u32_t display_format,
                                   fbe_trace_func_t fbe_debug_trace_func,
                                   fbe_trace_context_t trace_context)
{
    fbe_class_id_t class_id = FBE_CLASS_ID_INVALID;
    fbe_object_id_t object_id;
    fbe_lifecycle_state_t lifecycle_state;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_dbgext_ptr topology_object_table_ptr = 0;
    fbe_topology_object_status_t  object_status = 0;
    fbe_dbgext_ptr control_handle_ptr = 0;
    fbe_u32_t topology_object_table_entry_size = 0;
    fbe_u32_t offset = 0;
    fbe_sepls_lun_config_t lun_config ={0};
	fbe_lun_database_info_t lun_database_info = {0};
    fbe_u32_t display_object = (display_format & FBE_SEPLS_DISPLAY_FORMAT_MASK);
    fbe_u32_t display_level = (display_format & FBE_SEPLS_DISPLAY_FORMAT_MASK);
	fbe_u32_t ptr_size;
	fbe_dbgext_ptr topology_object_tbl_ptr = 0;

	status = fbe_debug_get_ptr_size(module_name, &ptr_size);
    if (status != FBE_STATUS_OK) 
    {
        fbe_debug_trace_func(NULL, "%s unable to get ptr size status: %d\n", __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE; 
    }

    /* Get the topology table. */
    FBE_GET_EXPRESSION(module_name, topology_object_table, &topology_object_table_ptr);
	FBE_READ_MEMORY(topology_object_table_ptr, &topology_object_tbl_ptr, ptr_size);

    /* Get the size of the topology entry. */
    FBE_GET_TYPE_SIZE(module_name, topology_object_table_entry_s, &topology_object_table_entry_size);

    if(topology_object_tbl_ptr == 0)
    {
        fbe_debug_trace_func(NULL, "\t topology_object_table_is not available \n");
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /* Point to the object we are looking for 
     */
    topology_object_tbl_ptr += (topology_object_table_entry_size * lun_id);

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
        /* Start populating the LUN info */
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

        lun_config.object_id = object_id;
        lun_config.exported_block_size = exported_block_size;
        lun_config.server_id = rg_id;

        FBE_GET_FIELD_DATA(module_name,
                           control_handle_ptr,
                           fbe_base_object_s,
                           lifecycle.state,
                           sizeof(fbe_lifecycle_state_t),
                           &lifecycle_state);
        lun_config.state = lifecycle_state;

        /* Get configuration details */
        fbe_sepls_get_config_data_debug_trace(module_name,object_id,&lun_database_info,fbe_debug_trace_func,trace_context);
		lun_config.lun_database_info.lun_number = lun_database_info.lun_number;
		lun_config.lun_database_info.capacity = lun_database_info.capacity;
        /* Display LUN object only if the user has specified so,
         * else return with capacity data
         */
        if(display_object == FBE_SEPLS_DISPLAY_OBJECTS_NONE ||
           display_object == FBE_SEPLS_DISPLAY_ONLY_LUN_OBJECTS ||
           display_level >= FBE_SEPLS_DISPLAY_LEVEL_2)
        {
            fbe_sepls_display_lun_info_debug_trace(fbe_debug_trace_func,trace_context,&lun_config);
            return lun_config.lun_database_info.capacity;
        }
        else
        {
            return lun_config.lun_database_info.capacity;
        }
    }
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_sepls_lun_info_trace()
 ******************************************/
/*!**************************************************************
 * fbe_sepls_get_lun_persist_data()
 ****************************************************************
 * @brief
 *  Populate the lun configuration data.
 *  
 * @param module_name - Module name to use in symbol lookup.
 * @param lun_object_ptr - ptr to lun object.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * 
 * @return fbe_status_t
 *
 * @author
 *  20/05/2011 - Created. Omer Miranda
 *
 ****************************************************************/
fbe_status_t fbe_sepls_get_lun_persist_data(const fbe_u8_t* module_name,
                                            fbe_dbgext_ptr lun_object_ptr,
                                            fbe_dbgext_ptr lun_user_ptr,
                                            fbe_lun_database_info_t *lun_data_ptr,
                                            fbe_trace_func_t trace_func,
                                            fbe_trace_context_t trace_context)
{
    fbe_u32_t offset = 0;
    fbe_lun_number_t lun_number;
    fbe_lba_t capacity;
    fbe_database_lun_configuration_t lun_config;
    database_lu_user_data_t lun_user_data;

    FBE_READ_MEMORY(lun_object_ptr, &lun_config, sizeof(fbe_database_lun_configuration_t));
    FBE_READ_MEMORY(lun_user_ptr, &lun_user_data, sizeof(database_lu_user_data_t));

    FBE_GET_FIELD_OFFSET(module_name, database_lu_user_data_t, "lun_number", &offset);
    FBE_READ_MEMORY(lun_user_ptr +offset, &lun_number, sizeof(fbe_lun_number_t));
    lun_data_ptr->lun_number = lun_number;
    
    FBE_GET_FIELD_OFFSET(module_name, fbe_database_lun_configuration_t, "capacity", &offset);
    FBE_READ_MEMORY(lun_object_ptr + offset, &capacity, sizeof(fbe_lba_t));
    lun_data_ptr->capacity = capacity;

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_sepls_get_lun_persist_data()
 ******************************************/
/*!**************************************************************
 * fbe_sepls_display_lun_info_debug_trace()
 ****************************************************************
 * @brief
 *  Display the lun data.
 *  
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param fbe_sepls_lun_config_t - Lun object database structure
 * 
 * @return fbe_status_t
 *
 * @author
 *  24/05/2011 - Created. Omer Miranda
 *
 ****************************************************************/
fbe_status_t fbe_sepls_display_lun_info_debug_trace(fbe_trace_func_t trace_func ,
                                                    fbe_trace_context_t trace_context,
                                                    fbe_sepls_lun_config_t *lun_data_ptr)
{
    fbe_u32_t  capacity_of_lun;

    trace_func(NULL,"%-8s","LUN");
    trace_func(NULL,"0x%-3x :%-6d",lun_data_ptr->object_id,lun_data_ptr->object_id);
    trace_func(NULL,"%-6d",lun_data_ptr->lun_database_info.lun_number);

    capacity_of_lun = (fbe_u32_t)((lun_data_ptr->lun_database_info.capacity * lun_data_ptr->exported_block_size)/(1024*1024));
    trace_func(NULL,"%-7d",capacity_of_lun);
    trace_func(NULL,"%-8s","MB");

    fbe_sepls_display_object_state_debug_trace(trace_func,trace_context,lun_data_ptr->state);
    if(lun_data_ptr->state == FBE_LIFECYCLE_STATE_READY)
    {
        trace_func(NULL,"%-12s"," ");
    }
    else
    {
        trace_func(NULL,"%-8s"," ");
    }
    trace_func(NULL,"%-33d",lun_data_ptr->server_id);
    /* Display drives associated with this lun */
    fbe_sepls_get_drives_info_debug_trace("sep",trace_func,trace_context,lun_data_ptr->server_id);
    trace_func(NULL,"\n");
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_sepls_display_lun_info_debug_trace()
 ******************************************/

/*************************
 * end file fbe_lun_debug.c
 *************************/

