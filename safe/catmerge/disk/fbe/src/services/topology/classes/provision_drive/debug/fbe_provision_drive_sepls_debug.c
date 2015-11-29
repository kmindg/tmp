/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_provision_drive_sepls_debug.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the sepls related functions for the PVD object.
 *  This file contains functions which will aid in displaying PVD related
 *  information for the sepls debug macro
 *
 * @author
 *  27/05/2011 - Created. Omer Miranda
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
#include "fbe_provision_drive_private.h"
#include "fbe_base_object_trace.h"
#include "fbe_provision_drive_debug.h"
#include "fbe_provision_drive_sepls_debug.h"
#include "fbe/fbe_raid_group.h"
#include "fbe_base_config_debug.h"
#include "fbe_sepls_debug.h"
#include "fbe/fbe_provision_drive.h"
#include "fbe/fbe_topology_interface.h"
#include "fbe_topology_debug.h"
#include "fbe_logical_drive_debug.h"



/* List of already displayed PVDs */
extern fbe_object_id_t pvd_displayed[FBE_MAX_SPARE_OBJECTS];
fbe_u32_t current_pvd_index = 0;

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/*!**************************************************************
 * fbe_sepls_pvd_info_trace()
 ****************************************************************
 * @brief
 *  Populate the PVD information.
 *  
 * @param module_name - Module name to use in symbol lookup.
 * @param fbe_object_id_t - PVD object.
 * @param fbe_bool_t - default display flag
 * @param display_format - Bitmap to indicate the user options
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * 
 * @return fbe_status_t
 *
 * @author
 *  27/05/2011 - Created. Omer Miranda
 *
 ****************************************************************/
fbe_status_t fbe_sepls_pvd_info_trace(const fbe_u8_t* module_name,
                                      fbe_object_id_t pvd_id,
                                      fbe_block_size_t exported_block_size,
                                      fbe_bool_t display_default,
                                      fbe_u32_t display_format,
                                      fbe_trace_func_t trace_func,
                                      fbe_trace_context_t trace_context)
{
    fbe_dbgext_ptr topology_object_table_ptr = 0;
    fbe_topology_object_status_t  object_status = 0;
    fbe_dbgext_ptr control_handle_ptr = 0;
    fbe_u32_t topology_object_table_entry_size = 0;
    fbe_u32_t pvd_index;
    fbe_class_id_t class_id = FBE_CLASS_ID_INVALID;
    fbe_object_id_t object_id;
    fbe_lifecycle_state_t lifecycle_state;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t offset = 0;
    fbe_sepls_pvd_config_t pvd_config ={0};
	fbe_pvd_database_info_t pvd_database_info = {0};
    fbe_dbgext_ptr base_config_ptr = 0;
    fbe_traffic_priority_t  verify_priority;
    fbe_traffic_priority_t  zero_priority;
	fbe_dbgext_ptr topology_object_tbl_ptr = 0;
	fbe_u32_t ptr_size;
    
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
    topology_object_tbl_ptr += (topology_object_table_entry_size * pvd_id);

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
        /* Start populating the PVD info */
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

        pvd_config.object_id = object_id;

        /* Before we display the PVD object's details, check
         * if it has already been displayed. The PVD objects are displayed
         * from two sources, via the RG-VD-PVD link and then all other PVDs to be
         * displayed when the -allsep option is specified by the user
         */
        for(pvd_index = 0; pvd_index < FBE_MAX_SPARE_OBJECTS;pvd_index++)
        {
           if(pvd_displayed[pvd_index] == object_id)
           {
           /* If the current pvd id exist in the list of displayed pvds
            * then dont process any further since this pvd has already been displayed
            */
                return FBE_STATUS_OK;
            }
        }
        /* Get the state of the PVD object */
        FBE_GET_FIELD_DATA(module_name,
                           control_handle_ptr,
                           fbe_base_object_s,
                           lifecycle.state,
                           sizeof(fbe_lifecycle_state_t),
                           &lifecycle_state);
        pvd_config.state = lifecycle_state;

        /* Get the down stream object and drives details */
        FBE_GET_FIELD_OFFSET(module_name,fbe_provision_drive_t,"base_config",&offset);
        base_config_ptr = control_handle_ptr + offset;
       
        /*Get the downstream object */
        pvd_config.server_id = fbe_sepls_get_downstream_object_debug_trace(module_name,control_handle_ptr,trace_func,trace_context);
        /* Populate the sniff verify and zeroing checkpoints */
        fbe_sepls_get_pvd_checkpoints_info(module_name,base_config_ptr,&pvd_config);
        /* Get the sniff verify and zeroing priorities */
        FBE_GET_FIELD_OFFSET(module_name,fbe_provision_drive_t,"verify_priority",&offset);
        FBE_READ_MEMORY(control_handle_ptr + offset, &verify_priority, sizeof(fbe_traffic_priority_t));
        pvd_config.verify_priority = verify_priority;
        FBE_GET_FIELD_OFFSET(module_name,fbe_provision_drive_t,"zero_priority",&offset);
        FBE_READ_MEMORY(control_handle_ptr + offset, &zero_priority, sizeof(fbe_traffic_priority_t));
        pvd_config.zero_priority = zero_priority;

        /* Get configuration details */
        fbe_sepls_get_config_data_debug_trace(module_name, object_id, &pvd_database_info, trace_func, trace_context);
		pvd_config.pvd_database_info.config_type = pvd_database_info.config_type;
		pvd_config.pvd_database_info.configured_capacity = pvd_database_info.configured_capacity;
		pvd_config.pvd_database_info.configured_physical_block_size = pvd_database_info.configured_physical_block_size;
        fbe_sepls_check_pvd_display_info_debug_trace(trace_func, trace_context, &pvd_config, display_default, display_format);

        if(display_default == FBE_TRUE ||
           display_format != FBE_SEPLS_DISPLAY_OBJECTS_NONE)
        {
            /* Do nothing */
            return FBE_STATUS_OK;
        }
        else
        {
	     /* From the PVD now go to LDO */
	     module_name = fbe_get_module_name();
	     fbe_sepls_ldo_info_trace(module_name,pvd_config.server_id,pvd_config.capacity_of_pvd, trace_func, NULL);
        }
    }
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_sepls_pvd_info_trace()
 ******************************************/
/*!**************************************************************
 * fbe_sepls_get_pvd_persist_data()
 ****************************************************************
 * @brief
 *  Populate the PVD configuration data.
 *  
 * @param module_name - Module name to use in symbol lookup.
 * @param raid_group_p - ptr to PVD object.
 * @param fbe_sepls_pvd_config_t - Pointer to the PVD info structure.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * 
 * @return fbe_status_t
 *
 * @author
 *  27/05/2011 - Created. Omer Miranda
 *
 ****************************************************************/
fbe_status_t fbe_sepls_get_pvd_persist_data(const fbe_u8_t* module_name,
                                            fbe_dbgext_ptr pvd_object_ptr,
                                            fbe_dbgext_ptr pvd_user_ptr,
                                            fbe_pvd_database_info_t *pvd_data_ptr,
                                            fbe_trace_func_t trace_func,
                                            fbe_trace_context_t trace_context)
{
    fbe_u32_t offset = 0;
    fbe_provision_drive_configuration_t pvd_config;
    fbe_provision_drive_config_type_t config_type;
    fbe_lba_t configured_capacity;
    fbe_provision_drive_configured_physical_block_size_t configured_physical_block_size;

    FBE_READ_MEMORY(pvd_object_ptr, &pvd_config, sizeof(fbe_provision_drive_configuration_t));

    FBE_GET_FIELD_OFFSET(module_name, fbe_provision_drive_configuration_t, "config_type", &offset);
    FBE_READ_MEMORY(pvd_object_ptr + offset, &config_type, sizeof(fbe_provision_drive_config_type_t));
    pvd_data_ptr->config_type = config_type;

    FBE_GET_FIELD_OFFSET(module_name, fbe_provision_drive_configuration_t, "configured_capacity", &offset);
    FBE_READ_MEMORY(pvd_object_ptr + offset, &configured_capacity, sizeof(fbe_lba_t));
    pvd_data_ptr->configured_capacity = configured_capacity;

    FBE_GET_FIELD_OFFSET(module_name, fbe_provision_drive_configuration_t, "configured_physical_block_size", &offset);
    FBE_READ_MEMORY(pvd_object_ptr + offset, &configured_physical_block_size, sizeof(fbe_provision_drive_configured_physical_block_size_t));
     pvd_data_ptr->configured_physical_block_size = configured_physical_block_size;

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_sepls_get_pvd_persist_data()
 ******************************************/
/*!**************************************************************
 * fbe_sepls_check_pvd_display_info_debug_trace()
 ****************************************************************
 * @brief
 *  Check the display type and accrodingly display the PVD.
 *  
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param fbe_sepls_pvd_config_t* - Pointer to the PVD info structure to be displayed.
 *.@param fbe_bool_t - default display flag
 * @param display_format - Bitmap to indicate the user options
 * 
 * @return fbe_status_t
 *
 * @author
 *  27/05/2011 - Created. Omer Miranda
 *
 ****************************************************************/
fbe_status_t fbe_sepls_check_pvd_display_info_debug_trace(fbe_trace_func_t trace_func ,
                                                          fbe_trace_context_t trace_context,
                                                          fbe_sepls_pvd_config_t *pvd_data_ptr,
                                                          fbe_bool_t display_default,
                                                          fbe_u32_t display_format)
{
    fbe_u32_t display_object = (display_format & FBE_SEPLS_DISPLAY_FORMAT_MASK);
    fbe_u32_t display_level = (display_format & FBE_SEPLS_DISPLAY_FORMAT_MASK);

    if(display_default == TRUE )
    {
        /* For default display we only display the configured HS
         */
        if(pvd_data_ptr->pvd_database_info.config_type == FBE_PROVISION_DRIVE_CONFIG_TYPE_TEST_RESERVED_SPARE)
        {
            fbe_sepls_display_pvd_info_debug_trace(trace_func, trace_context, pvd_data_ptr);
        }
    }
    else if (display_object == FBE_SEPLS_DISPLAY_OBJECTS_NONE ||
             display_object == FBE_SEPLS_DISPLAY_ONLY_PVD_OBJECTS ||
             display_level == FBE_SEPLS_DISPLAY_LEVEL_4)
    {
        /* We have come here via the RG-VD-PVD link so display the PVD
         */
        fbe_sepls_display_pvd_info_debug_trace(trace_func, trace_context, pvd_data_ptr);
    }
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_sepls_check_pvd_display_info_debug_trace()
 ******************************************/
/*!**************************************************************
 * fbe_sepls_display_pvd_info_debug_trace()
 ****************************************************************
 * @brief
 *  Display the PVD details.
 *  
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param fbe_sepls_pvd_config_t* - Pointer to the PVD info structure to be displayed
 * 
 * @return fbe_status_t
 *
 * @author
 *  27/05/2011 - Created. Omer Miranda
 *
 ****************************************************************/
fbe_status_t fbe_sepls_display_pvd_info_debug_trace(fbe_trace_func_t trace_func ,
                                                    fbe_trace_context_t trace_context,
                                                    fbe_sepls_pvd_config_t *pvd_data_ptr)
{
    fbe_u32_t  capacity_of_pvd;
    fbe_u16_t sniff_percent;
    fbe_u16_t zero_percent;

    fbe_u8_t * module_name;
    trace_func(NULL,"%-8s","PVD");
    trace_func(NULL,"0x%-3x :%-6d",pvd_data_ptr->object_id,pvd_data_ptr->object_id);
    trace_func(NULL,"%-6s","-");

    if ((pvd_data_ptr->pvd_database_info.configured_physical_block_size == FBE_PROVISION_DRIVE_CONFIGURED_PHYSICAL_BLOCK_SIZE_520)  ||
        (pvd_data_ptr->pvd_database_info.configured_physical_block_size == FBE_PROVISION_DRIVE_CONFIGURED_PHYSICAL_BLOCK_SIZE_4160)    )
    {
        /* Capacity is alwasy in terms of FBE_BE_BYTES_PER_BLOCK */
        capacity_of_pvd = (fbe_u32_t)((pvd_data_ptr->pvd_database_info.configured_capacity * 520)/(1024*1024));
        pvd_data_ptr->capacity_of_pvd = capacity_of_pvd;
        trace_func(NULL,"%-7d",capacity_of_pvd);
    }
    else
    {
        trace_func(NULL,"%-7s","INVALID");
    }
    if(pvd_data_ptr->pvd_database_info.config_type == FBE_PROVISION_DRIVE_CONFIG_TYPE_TEST_RESERVED_SPARE)
    {
        trace_func(NULL,"%-2s","MB");
        trace_func(NULL,"%s","(TEST)");
    }
    else
    {
        trace_func(NULL,"%-8s","MB");
    }
    fbe_sepls_display_object_state_debug_trace(trace_func,trace_context,pvd_data_ptr->state);


    if ((pvd_data_ptr->zero_checkpoint != FBE_LBA_INVALID) && pvd_data_ptr->zero_checkpoint > 0 )
    {
        if(pvd_data_ptr->state == FBE_LIFECYCLE_STATE_READY)
        {
            zero_percent = (fbe_u16_t) ((pvd_data_ptr->zero_checkpoint * 100) / pvd_data_ptr->pvd_database_info.configured_capacity);
            if(zero_percent >= 101)
            {
                trace_func(NULL,"%-12s"," ");
                return FBE_STATUS_OK;
            }
            else
            {
                /* During disk zeroing if a user initiates a sniff verify operation
                 * with higher priority then don't display the zeroing checkpoint
                 */
                if(((pvd_data_ptr->sniff_checkpoint != FBE_LBA_INVALID) && pvd_data_ptr->sniff_checkpoint > 0) &&
                   (pvd_data_ptr->verify_priority > pvd_data_ptr->zero_priority))
                {
                    sniff_percent = (fbe_u16_t) ((pvd_data_ptr->sniff_checkpoint * 100) / pvd_data_ptr->pvd_database_info.configured_capacity);
                    if(sniff_percent >= 101)
                    {
                        trace_func(NULL,"%-12s"," ");
                        return FBE_STATUS_OK;
                    }
                    else
                    {
                        trace_func(NULL,"SV:%3d%%",sniff_percent);
                        trace_func(NULL,"%-5s"," ");
                    }
                }
                else
                {
                    trace_func(NULL,"ZER:%3d%%",zero_percent);
                    trace_func(NULL,"%-4s"," ");
                }
            }
        }
    }
    else if ((pvd_data_ptr->sniff_checkpoint != FBE_LBA_INVALID) && pvd_data_ptr->sniff_checkpoint > 0 )
    {
        if(pvd_data_ptr->state == FBE_LIFECYCLE_STATE_READY)
        {
            sniff_percent = (fbe_u16_t) ((pvd_data_ptr->sniff_checkpoint * 100) / pvd_data_ptr->pvd_database_info.configured_capacity);
            if(sniff_percent >= 101)
            {
                trace_func(NULL,"%-12s"," ");
                return FBE_STATUS_OK;
            }
            else
            {
                /* During sniff verify if a user initiates a disk zeroing operation
                 * with higher priority then don't display the sniff verify checkpoint
                 */
                if(((pvd_data_ptr->zero_checkpoint != FBE_LBA_INVALID) && pvd_data_ptr->zero_checkpoint > 0) &&
                   (pvd_data_ptr->zero_priority > pvd_data_ptr->verify_priority))
                {
                    zero_percent = (fbe_u16_t) ((pvd_data_ptr->zero_checkpoint * 100) / pvd_data_ptr->pvd_database_info.configured_capacity);
                    if(zero_percent >= 101)
                    {
                        trace_func(NULL,"%-12s"," ");
                        return FBE_STATUS_OK;
                    }
                    else
                    {
                        trace_func(NULL,"ZER:%3d%%",zero_percent);
                        trace_func(NULL,"%-4s"," ");
                    }
                }
                else
                {
                    trace_func(NULL,"SV:%3d%%",sniff_percent);
                    trace_func(NULL,"%-5s"," ");
                }
            }
        }
    }
    else if(pvd_data_ptr->state == FBE_LIFECYCLE_STATE_READY)
    {
        trace_func(NULL,"%-12s"," ");
    }
    else
    {
        trace_func(NULL,"%-8s"," ");
    }
    if(pvd_data_ptr->server_id != FBE_OBJECT_ID_INVALID)
    {
        trace_func(NULL,"%-33d",pvd_data_ptr->server_id);
    }
    else
    {
        trace_func(NULL,"%-8s"," ");
        trace_func(NULL,"%s","-");
        trace_func(NULL,"%-32s"," ");
    }

    module_name = fbe_get_module_name();

    fbe_sepls_get_drives_info_debug_trace(module_name,trace_func,trace_context,pvd_data_ptr->server_id);
    trace_func(NULL,"\n");
    /* Add the current pvd to the list of displayed PVDs
     */
    pvd_displayed[current_pvd_index] = pvd_data_ptr->object_id;
    current_pvd_index++;
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_sepls_display_pvd_info_debug_trace()
 ******************************************/

/*!**************************************************************
 * fbe_sepls_get_pvd_checkpoints_info()
 ****************************************************************
 * @brief
 *  Populate the sniff verify checkpoint and disk zeroing checkpoint.
 *  
 * @param module_name - Module name to use in symbol lookup.
 * @param fbe_dbgext_ptr - PVD base config ptr.
 * @param fbe_sepls_rg_config_t * - Pointer to the PVD data structure where we 
 *                                 populate the information.
 * 
 * @return fbe_status_t
 *
 * @author
 *  7/07/2011 - Created. Omer Miranda
 *
 ****************************************************************/
fbe_status_t fbe_sepls_get_pvd_checkpoints_info(const fbe_u8_t * module_name,
                                                fbe_dbgext_ptr pvd_base_config_ptr,
                                                fbe_sepls_pvd_config_t *pvd_data_ptr)
{
    fbe_u32_t offset;
    fbe_dbgext_ptr fbe_metadata_ptr;
    fbe_dbgext_ptr nonpaged_record_ptr;
    fbe_dbgext_ptr data_ptr;
    fbe_lba_t sniff_checkpoint;
    fbe_lba_t zero_checkpoint;

    FBE_GET_FIELD_OFFSET(module_name, fbe_base_config_t, "metadata_element", &offset);
    fbe_metadata_ptr = pvd_base_config_ptr + offset;

    FBE_GET_FIELD_OFFSET(module_name, fbe_metadata_element_t, "nonpaged_record", &offset);
    nonpaged_record_ptr = fbe_metadata_ptr + offset;

    FBE_GET_FIELD_OFFSET(module_name, fbe_metadata_record_t, "data_ptr", &offset);
    FBE_READ_MEMORY(nonpaged_record_ptr + offset, &data_ptr, sizeof(fbe_dbgext_ptr));	

    FBE_GET_FIELD_OFFSET(module_name, fbe_provision_drive_nonpaged_metadata_t, "sniff_verify_checkpoint", &offset);
    FBE_READ_MEMORY(data_ptr + offset, &sniff_checkpoint, sizeof(fbe_lba_t));
    

    FBE_GET_FIELD_OFFSET(module_name, fbe_provision_drive_nonpaged_metadata_t, "zero_checkpoint", &offset);
    FBE_READ_MEMORY(data_ptr + offset, &zero_checkpoint, sizeof(fbe_lba_t));

    pvd_data_ptr->sniff_checkpoint = sniff_checkpoint;
    pvd_data_ptr->zero_checkpoint = zero_checkpoint;

    return FBE_STATUS_OK;
}
/********************************************
 * end fbe_sepls_get_rg_rebuild_info
 ********************************************/

/*************************
 * end file fbe_provision_drive_debug.c
 *************************/

