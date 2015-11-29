/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_logical_drive_sepls_debug.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the sepls related functions for the LDO object.
 *  This  file contains functions which will aid in displaying LDO related
 *  information for the sepls debug macro.
 *
 * @author
 *  11/07/2011 - Created. Omer Miranda
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
#include "fbe_base_object_trace.h"
#include "fbe/fbe_base_config.h"
#include "fbe_base_config_debug.h"
#include "fbe_sepls_debug.h"
#include "fbe_logical_drive_debug.h"
#include "fbe_logical_drive_sepls_debug.h"
#include "fbe_topology_debug.h"
#include "fbe_base_physical_drive_debug.h"


/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/*!**************************************************************
 * fbe_sepls_ldo_info_trace()
 ****************************************************************
 * @brief
 *  Populate the logical drive information.
 *  
 * @param module_name - Module name to use in symbol lookup.
 * @param fbe_object_id_t - LDO object id.
 * @param fbe_lba_t - LDO capacity;
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * 
 * @return fbe_status_t
 *
 * @author
 *  01/06/2011 - Created. Omer Miranda
 *
 ****************************************************************/
fbe_status_t fbe_sepls_ldo_info_trace(const fbe_u8_t * module_name,
                                      fbe_object_id_t ldo_id,
                                      fbe_u32_t capacity,
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
    fbe_u32_t block_edge_ptr_offset = 0;
    fbe_u32_t server_id;
    fbe_sepls_ldo_config_t ldo_config ={0};
	fbe_dbgext_ptr topology_object_tbl_ptr = 0;
	fbe_u32_t ptr_size;

    if(ldo_id == FBE_OBJECT_ID_INVALID)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
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
    topology_object_tbl_ptr += (topology_object_table_entry_size * ldo_id);

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

        ldo_config.object_id = object_id;
        ldo_config.capacity_of_ldo = capacity;

        FBE_GET_FIELD_DATA(module_name,
                           control_handle_ptr,
                           fbe_base_object_s,
                           lifecycle.state,
                           sizeof(fbe_lifecycle_state_t),
                           &lifecycle_state);

        ldo_config.state = lifecycle_state;

        /* Get the down stream object and drives details */
        FBE_GET_FIELD_OFFSET(module_name,fbe_logical_drive_t,"block_edge",&block_edge_ptr_offset);
        FBE_GET_FIELD_OFFSET(module_name, fbe_base_edge_t, "server_id", &offset);
        FBE_READ_MEMORY(control_handle_ptr + block_edge_ptr_offset + offset, &server_id, sizeof(fbe_u32_t));
        ldo_config.server_id = server_id;
        fbe_sepls_display_ldo_info_debug_trace(trace_func, trace_context, &ldo_config);
    }

    /* From the LDO now go to PDO */
    fbe_sepls_pdo_info_trace(module_name,ldo_config.server_id, ldo_config.capacity_of_ldo, trace_func, NULL);
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_sepls_ldo_info_trace()
 ******************************************/

/*!**************************************************************
 * fbe_sepls_display_ldo_info_debug_trace()
 ****************************************************************
 * @brief
 *  Display the Logical Drive data.
 *  
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param fbe_sepls_ldo_config_t * - Pointer to the LDO info structure to be displayed.
 * 
 * @return fbe_status_t
 *
 * @author
 *  11/07/2011 - Created. Omer Miranda
 *
 ****************************************************************/
fbe_status_t fbe_sepls_display_ldo_info_debug_trace(fbe_trace_func_t trace_func ,
                                                    fbe_trace_context_t trace_context,
                                                    fbe_sepls_ldo_config_t *ldo_data_ptr)
{
    fbe_u8_t* module_name;

    trace_func(NULL,"%-8s","LDO");
    trace_func(NULL,"0x%-3x :%-6d",ldo_data_ptr->object_id,ldo_data_ptr->object_id);
    trace_func(NULL,"%s","-");
    trace_func(NULL,"%-5s"," ");
    trace_func(NULL,"%-7d",ldo_data_ptr->capacity_of_ldo);
    trace_func(NULL,"%-8s","MB");

    fbe_sepls_display_object_state_debug_trace(trace_func,trace_context,ldo_data_ptr->state);
    if(ldo_data_ptr->state == FBE_LIFECYCLE_STATE_READY)
    {
        trace_func(NULL,"%-12s"," ");
    }
    else
    {
        trace_func(NULL,"%-8s"," ");
    }
    
    trace_func(NULL,"%-33d",ldo_data_ptr->server_id);

    module_name = fbe_get_module_name();
    fbe_sepls_get_drives_info_debug_trace(module_name,trace_func,trace_context,ldo_data_ptr->server_id);
    trace_func(NULL,"\n");
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_sepls_display_ldo_info_debug_trace()
 ******************************************/

/*************************
 * end file fbe_logical_drive_debug.c
 *************************/
