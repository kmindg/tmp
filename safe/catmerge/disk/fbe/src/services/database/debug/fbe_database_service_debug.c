/***************************************************************************
 *  Copyright (C)  EMC Corporation 2011
 *  All rights reserved.
 *  Licensed material - property of EMC Corporation. * All rights reserved.
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_database_service_debug.c
 ***************************************************************************
 *
 * @brief
 *  This file contains debugging commands for the database service.
.*
 * @version
 *   03-Aug-2011:  Created. Omer Miranda
 *
 ***************************************************************************/

#include "fbe/fbe_types.h"
#include "fbe/fbe_dbgext.h"
#include "fbe/fbe_topology_interface.h"
#include "fbe_base_object_debug.h"
#include "fbe/fbe_class.h"
#include "fbe/fbe_devices.h"
#include "fbe_base_object_trace.h"
#include "fbe_transport_debug.h"
#include "fbe_topology_debug.h"
#include "fbe_database_private.h"
#include "fbe_database_config_tables.h"
#include "fbe_database.h"
#include "fbe_database_debug.h"
#include "fbe/fbe_scsi_interface.h"
#include "fbe/fbe_provision_drive.h"

/*!**************************************************************
 * fbe_database_service_debug_trace()
 ****************************************************************
 * @brief
 *  Display the specified database table 
 *  
 * @param module_name - Module name to use in symbol lookup
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param database_service_ptr -Pointer to the database service
 * @param display_format - Database table to be displayed.
 *.
 * 
 * @return fbe_status_t
 *
 * @author
 *  03-Aug-2011:  Created. Omer Miranda
 *
 ****************************************************************/
fbe_status_t fbe_database_service_debug_trace(const fbe_u8_t * module_name,
                                              fbe_trace_func_t trace_func,
                                              fbe_trace_context_t trace_context,
                                              fbe_dbgext_ptr database_service_ptr,
                                              fbe_u32_t display_format,
                                              fbe_object_id_t object_id)
{
    fbe_u32_t offset = 0;
    fbe_dbgext_ptr object_table_ptr = 0;
    fbe_dbgext_ptr user_table_ptr = 0;
    fbe_dbgext_ptr edge_table_ptr = 0;
    fbe_dbgext_ptr global_info_ptr = 0;
    

    switch(display_format)
    {
        case FBE_DATABASE_DISPLAY_OBJECT_TABLE:
            /* Get the object table pointer */
            FBE_GET_FIELD_OFFSET(module_name,fbe_database_service_t,"object_table",&offset);
            object_table_ptr = database_service_ptr + offset;
            fbe_database_display_object_table(module_name, trace_func, trace_context, object_table_ptr, object_id);
            break;
        case FBE_DATABASE_DISPLAY_EDGE_TABLE:
            /* Get the user table pointer */
            FBE_GET_FIELD_OFFSET(module_name,fbe_database_service_t,"edge_table",&offset);
            edge_table_ptr = database_service_ptr + offset;
            fbe_database_display_edge_table(module_name, trace_func, trace_context, edge_table_ptr, object_id);
            break;
        case FBE_DATABASE_DISPLAY_USER_TABLE:
            /* Get the user table pointer */
            FBE_GET_FIELD_OFFSET(module_name,fbe_database_service_t,"user_table",&offset);
            user_table_ptr = database_service_ptr + offset;
            fbe_database_display_user_table(module_name, trace_func, trace_context, user_table_ptr, object_id);
            break;
        case FBE_DATABASE_DISPLAY_GLOBAL_INFO_TABLE:
            /* Get the global info pointer */
            FBE_GET_FIELD_OFFSET(module_name,fbe_database_service_t,"global_info",&offset);
            global_info_ptr = database_service_ptr + offset;
            fbe_database_display_global_info_table(module_name, trace_func, trace_context, global_info_ptr, object_id);
            break;
        default:
            trace_func(NULL, "\t Unknown display format\n");
            return FBE_STATUS_INVALID;
    }
    return FBE_STATUS_OK;
}
/*****************************************
 * end fbe_database_service_debug_trace
 *****************************************/
/*!**************************************************************
 * fbe_database_display_object_table()
 ****************************************************************
 * @brief
 *  Display the database service object table 
 *  
 * @param module_name - Module name to use in symbol lookup
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param database_service_ptr -Pointer to the database service object table
 *.
 * 
 * @return fbe_status_t
 *
 * @author
 *  03-Aug-2011:  Created. Omer Miranda
 *
 ****************************************************************/
fbe_status_t fbe_database_display_object_table(const fbe_u8_t * module_name,
                                               fbe_trace_func_t trace_func,
                                               fbe_trace_context_t trace_context,
                                               fbe_dbgext_ptr object_table_ptr,
                                               fbe_object_id_t in_object_id)
{
    fbe_u32_t offset = 0;
    fbe_u32_t header_offset = 0;
    fbe_dbgext_ptr object_table_content_ptr = 0;
    fbe_dbgext_ptr object_entry_ptr = 0;
    fbe_u32_t database_object_entry_size;
    database_class_id_t class_id;
    fbe_dbgext_ptr fbe_pvd_config_ptr = 0;
    fbe_dbgext_ptr fbe_lun_config_ptr = 0;
    fbe_dbgext_ptr fbe_vd_config_ptr = 0;
    fbe_dbgext_ptr fbe_rg_config_ptr = 0;
    fbe_u32_t spaces_to_indent = 0;
    database_config_entry_state_t state;
    fbe_persist_entry_id_t entry_id;
    fbe_object_id_t object_id;
    database_table_size_t table_size;
    fbe_u32_t table_entries;
    fbe_dbgext_ptr debug_ptr = 0;

    FBE_GET_FIELD_OFFSET(module_name,database_table_t,"table_size",&offset);
    FBE_READ_MEMORY(object_table_ptr + offset, &table_size, sizeof(database_table_size_t));

    FBE_GET_FIELD_OFFSET(module_name,database_table_t,"table_content",&offset);
    object_table_content_ptr = object_table_ptr + offset;
	/* Get the Object entry pointer */
    FBE_READ_MEMORY(object_table_content_ptr, &object_entry_ptr, sizeof(fbe_dbgext_ptr));

    /* Get the size of the database object entry. */
    FBE_GET_TYPE_SIZE(module_name, database_object_entry_t, &database_object_entry_size);

    debug_ptr = object_entry_ptr;
    for(table_entries = 0;table_entries < table_size ; table_entries++)
    {
        FBE_GET_FIELD_OFFSET(module_name,database_object_entry_t,"header",&header_offset);
        FBE_GET_FIELD_OFFSET(module_name, database_table_header_t, "object_id", &offset);
        FBE_READ_MEMORY(debug_ptr + header_offset + offset, &object_id, sizeof(fbe_object_id_t));
        if(object_id == in_object_id || in_object_id == FBE_OBJECT_ID_INVALID)
        {

            FBE_GET_FIELD_OFFSET(module_name,database_table_header_t,"state",&offset);
            FBE_READ_MEMORY(debug_ptr + header_offset + offset , &state, sizeof(database_config_entry_state_t));

            if(state != DATABASE_CONFIG_ENTRY_INVALID)
            {
                spaces_to_indent = 4;
                fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
                trace_func(NULL,"OBJECT ENTRY:\n");
                fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
                trace_func(NULL,"state:");
                switch(state)
                {
                    case DATABASE_CONFIG_ENTRY_VALID:
                        trace_func(NULL,"VALID");
                        break;
                    case DATABASE_CONFIG_ENTRY_CREATE:
                        trace_func(NULL,"CREATE");
                        break;
                    case DATABASE_CONFIG_ENTRY_DESTROY:
                        trace_func(NULL,"DESTROY");
                        break;
                    case DATABASE_CONFIG_ENTRY_MODIFY:
                        trace_func(NULL,"MODIFY");
                        break;
                }

                spaces_to_indent += 4;
                fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
                FBE_GET_FIELD_OFFSET(module_name, database_table_header_t, "entry_id", &offset);
                FBE_READ_MEMORY(debug_ptr + header_offset + offset, &entry_id, sizeof(fbe_persist_entry_id_t));
                trace_func(NULL,"entry_id:0x%llx", (unsigned long long)entry_id);

                spaces_to_indent += 4;
                fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
                FBE_GET_FIELD_OFFSET(module_name, database_table_header_t, "object_id", &offset);
                FBE_READ_MEMORY(debug_ptr + header_offset + offset, &object_id, sizeof(fbe_object_id_t));
                trace_func(NULL,"object_id:0x%x",object_id);

                trace_func(NULL,"\n");
                spaces_to_indent = 4;
                fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
                FBE_GET_FIELD_OFFSET(module_name,database_object_entry_t,"db_class_id",&offset);
                FBE_READ_MEMORY(debug_ptr + offset , &class_id, sizeof(database_class_id_t));

                FBE_GET_FIELD_OFFSET(module_name,database_object_entry_t,"set_config_union",&offset);

                switch(class_id)
                {
                    case DATABASE_CLASS_ID_PROVISION_DRIVE:
                        trace_func(NULL,"DATABASE_CLASS_ID_PROVISION_DRIVE:\n");
                        fbe_pvd_config_ptr = debug_ptr + offset;
                        fbe_pvd_object_config_debug_trace(module_name, trace_func, trace_context,fbe_pvd_config_ptr);
                        break;
                    case DATABASE_CLASS_ID_LUN:
                        trace_func(NULL,"DATABASE_CLASS_ID_LUN:\n");
                        fbe_lun_config_ptr = debug_ptr + offset;
                        fbe_lun_object_config_debug_trace(module_name, trace_func, trace_context,fbe_lun_config_ptr);
                        break;
                    case DATABASE_CLASS_ID_VIRTUAL_DRIVE:
                        trace_func(NULL,"DATABASE_CLASS_ID_VIRTUAL_DRIVE:\n");
                        fbe_vd_config_ptr = debug_ptr + offset;
                        fbe_vd_object_config_debug_trace(module_name, trace_func, trace_context,fbe_vd_config_ptr);
                        break;
                    case DATABASE_CLASS_ID_RAID_START:
                    case DATABASE_CLASS_ID_MIRROR:
                    case DATABASE_CLASS_ID_STRIPER:
                    case DATABASE_CLASS_ID_PARITY:
                    case DATABASE_CLASS_ID_RAID_END:
                        trace_func(NULL,"DATABASE_CLASS_ID_RAID:\n");
                        fbe_rg_config_ptr = debug_ptr + offset;
                        fbe_rg_object_config_debug_trace(module_name, trace_func, trace_context,fbe_rg_config_ptr);
                        break;
                    case DATABASE_CLASS_ID_EXTENT_POOL:
                        trace_func(NULL,"DATABASE_CLASS_ID_EXTENT_POOL:\n");
                        fbe_ext_pool_object_config_debug_trace(module_name, trace_func, trace_context,debug_ptr + offset);
                        break;
                    case DATABASE_CLASS_ID_EXTENT_POOL_LUN:
                        trace_func(NULL,"DATABASE_CLASS_ID_EXTENT_POOL_LUN:\n");
                        fbe_ext_pool_lun_object_config_debug_trace(module_name, trace_func, trace_context,debug_ptr + offset);
                        break;
                    case DATABASE_CLASS_ID_EXTENT_POOL_METADATA_LUN:
                        trace_func(NULL,"DATABASE_CLASS_ID_EXTENT_POOL_METADATA_LUN:\n");
                        fbe_ext_pool_lun_object_config_debug_trace(module_name, trace_func, trace_context,debug_ptr + offset);
                        break;
                }
                trace_func(NULL,"\n");
                trace_func(NULL,"\n");
            }
			/* Advance to the next entry */
            debug_ptr += database_object_entry_size;
        }
        else
        {
			/* Advance to the next entry */
            debug_ptr += database_object_entry_size;
			/* Keep looping till we dont reach to the specified object id */
            continue;
        }
    }
    return FBE_STATUS_OK;
}
/*****************************************
 * end fbe_database_display_object_table
 *****************************************/
/*!**************************************************************
 * fbe_pvd_object_config_debug_trace()
 ****************************************************************
 * @brief
 *  Display the PVD's objects object table 
 *  
 * @param module_name - Module name to use in symbol lookup
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param database_service_ptr -Pointer to the PVD config database 
 *.
 * 
 * @return fbe_status_t
 *
 * @author
 *  03-Aug-2011:  Created. Omer Miranda
 *
 ****************************************************************/
fbe_status_t fbe_pvd_object_config_debug_trace(const fbe_u8_t * module_name,
                                               fbe_trace_func_t trace_func,
                                               fbe_trace_context_t trace_context,
                                               fbe_dbgext_ptr pvd_object_ptr)
{
    fbe_u32_t offset = 0;
    fbe_provision_drive_config_type_t config_type;
    fbe_lba_t configured_capacity;
    fbe_provision_drive_configured_physical_block_size_t configured_physical_block_size;
    fbe_bool_t sniff_verify_state;
    fbe_config_generation_t generation_number;
    fbe_u8_t serial_num[FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE+1];
    fbe_update_pvd_type_t update_type;
    fbe_u32_t spaces_to_indent = 0;

    FBE_GET_FIELD_OFFSET(module_name, fbe_provision_drive_configuration_t, "config_type", &offset);
    FBE_READ_MEMORY(pvd_object_ptr + offset, &config_type, sizeof(fbe_provision_drive_config_type_t));

    spaces_to_indent += 8;
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    trace_func(NULL,"config_type:");
    switch(config_type)
    {
        case FBE_PROVISION_DRIVE_CONFIG_TYPE_UNCONSUMED:
            trace_func(NULL,"CONFIG_TYPE_UNKNOWN");
            break;
        case FBE_PROVISION_DRIVE_CONFIG_TYPE_RAID:
            trace_func(NULL,"CONFIG_TYPE_RAID");
            break;
        case FBE_PROVISION_DRIVE_CONFIG_TYPE_TEST_RESERVED_SPARE:
            trace_func(NULL,"CONFIG_TYPE_SPARE");
            break;
        case FBE_PROVISION_DRIVE_CONFIG_TYPE_EXT_POOL:
            trace_func(NULL,"CONFIG_TYPE_EXT_POOL");
            break;
        case FBE_PROVISION_DRIVE_CONFIG_TYPE_INVALID:
            trace_func(NULL,"CONFIG_TYPE_INVALID");
            break;
    }

    FBE_GET_FIELD_OFFSET(module_name, fbe_provision_drive_configuration_t, "configured_capacity", &offset);
    FBE_READ_MEMORY(pvd_object_ptr + offset, &configured_capacity, sizeof(fbe_lba_t));

    spaces_to_indent += 4;
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    trace_func(NULL,"configured_capacity:0x%llx", (unsigned long long)configured_capacity);

    FBE_GET_FIELD_OFFSET(module_name, fbe_provision_drive_configuration_t, "configured_physical_block_size", &offset);
    FBE_READ_MEMORY(pvd_object_ptr + offset, &configured_physical_block_size, sizeof(fbe_provision_drive_configured_physical_block_size_t));

    trace_func(NULL,"\n");
    spaces_to_indent = 8;
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    trace_func(NULL,"configured_physical_block_size:");

    switch(configured_physical_block_size)
    {
        case FBE_PROVISION_DRIVE_CONFIGURED_PHYSICAL_BLOCK_SIZE_520:
            trace_func(NULL,"BLOCK_SIZE_520");
            break;
        case FBE_PROVISION_DRIVE_CONFIGURED_PHYSICAL_BLOCK_SIZE_4160:
            trace_func(NULL,"BLOCK_SIZE_4160");
            break;
        case FBE_PROVISION_DRIVE_CONFIGURED_PHYSICAL_BLOCK_SIZE_INVALID:
            trace_func(NULL,"BLOCK_SIZE_INVALID");
            break;
    }

    spaces_to_indent += 4;
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    FBE_GET_FIELD_OFFSET(module_name, fbe_provision_drive_configuration_t, "sniff_verify_state", &offset);
    FBE_READ_MEMORY(pvd_object_ptr + offset, &sniff_verify_state, sizeof(fbe_bool_t));
    trace_func(NULL,"sniff_verify_state:0x%x",sniff_verify_state);

    trace_func(NULL,"\n");
    spaces_to_indent = 8;
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    FBE_GET_FIELD_OFFSET(module_name, fbe_provision_drive_configuration_t, "generation_number", &offset);
    FBE_READ_MEMORY(pvd_object_ptr + offset, &generation_number, sizeof(fbe_bool_t));
    trace_func(NULL,"generation_number:0x%llx", (unsigned long long)generation_number);
    
    spaces_to_indent += 4;
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    FBE_GET_FIELD_OFFSET(module_name, fbe_provision_drive_configuration_t, "serial_num", &offset);
    FBE_READ_MEMORY(pvd_object_ptr + offset, &serial_num, FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE+1);

    if(strlen(serial_num) > 0)
    {
        trace_func(NULL,"serial_num: %s",serial_num);
    }
    else
    {
        trace_func(NULL,"serial_num: %s","NULL");
    }

    spaces_to_indent += 4;
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    FBE_GET_FIELD_OFFSET(module_name, fbe_provision_drive_configuration_t, "update_type", &offset);
    FBE_READ_MEMORY(pvd_object_ptr + offset, &update_type, sizeof(fbe_update_pvd_type_t));
    trace_func(NULL,"update_type:");
    switch(update_type)
    {
        case FBE_UPDATE_PVD_INVALID:
            trace_func(NULL,"UPDATE_PVD_INVALID");
            break;
        case FBE_UPDATE_PVD_TYPE:
            trace_func(NULL,"UPDATE_PVD_TYPE");
            break;
        case FBE_UPDATE_PVD_SNIFF_VERIFY_STATE:
            trace_func(NULL,"UPDATE_PVD_SNIFF_VERIFY_STATE");
            break;
        case FBE_UPDATE_PVD_POOL_ID:
            trace_func(NULL,"UPDATE_PVD_POOL_ID");
            break;
    }

    trace_func(NULL,"\n");
    return FBE_STATUS_OK;
}
/*****************************************
 * end fbe_pvd_object_config_debug_trace
 *****************************************/

/*!**************************************************************
 * fbe_lun_object_config_debug_trace()
 ****************************************************************
 * @brief
 *  Display the LUN's objects object table 
 *  
 * @param module_name - Module name to use in symbol lookup
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param database_service_ptr -Pointer to the LUN config database 
 *.
 * 
 * @return fbe_status_t
 *
 * @author
 *  04-Aug-2011:  Created. Omer Miranda
 *
 ****************************************************************/
fbe_status_t fbe_lun_object_config_debug_trace(const fbe_u8_t * module_name,
                                               fbe_trace_func_t trace_func,
                                               fbe_trace_context_t trace_context,
                                               fbe_dbgext_ptr lun_object_ptr)
{
    fbe_u32_t offset = 0;
    fbe_u32_t spaces_to_indent = 0;
    fbe_u32_t char_offset = 0;
    fbe_u8_t world_wide_name[16];
    fbe_lba_t capacity;
    fbe_config_generation_t generation_number;
    fbe_u64_t power_save_io_drain_delay_in_sec;
    fbe_u64_t max_lun_latency_time_is_sec;
    fbe_u64_t   power_saving_idle_time_in_seconds;
    fbe_bool_t  power_saving_enabled;
    fbe_lun_config_flags_t  config_flags;
    fbe_assigned_wwid_t  wwn; 

    spaces_to_indent += 8;
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    FBE_GET_FIELD_OFFSET(module_name, fbe_database_lun_configuration_t, "world_wide_name", &offset);
    FBE_READ_MEMORY(lun_object_ptr + offset , &wwn, sizeof(fbe_assigned_wwid_t));
    FBE_GET_FIELD_OFFSET(module_name, fbe_assigned_wwid_t, "bytes", &char_offset);	
    FBE_READ_MEMORY(lun_object_ptr + offset + char_offset, &world_wide_name, 16);
    if(strlen(world_wide_name) > 0)
    {
        trace_func(NULL,"WWN: %s",world_wide_name);
    }
    else
    {
        trace_func(NULL,"WWN: %s","NULL");
    }

    spaces_to_indent += 4;
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    FBE_GET_FIELD_OFFSET(module_name, fbe_database_lun_configuration_t, "capacity", &offset);
    FBE_READ_MEMORY(lun_object_ptr + offset, &capacity, sizeof(fbe_lba_t));
    trace_func(NULL,"capacity:0x%llx", (unsigned long long)capacity);

    trace_func(NULL,"\n");
    spaces_to_indent = 8;
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    FBE_GET_FIELD_OFFSET(module_name, fbe_database_lun_configuration_t, "generation_number", &offset);
    FBE_READ_MEMORY(lun_object_ptr + offset, &generation_number, sizeof(fbe_bool_t));
    trace_func(NULL,"generation_number:0x%llx", (unsigned long long)generation_number);

    spaces_to_indent += 4;
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    FBE_GET_FIELD_OFFSET(module_name, fbe_database_lun_configuration_t, "power_save_io_drain_delay_in_sec", &offset);
    FBE_READ_MEMORY(lun_object_ptr + offset, &power_save_io_drain_delay_in_sec, sizeof(fbe_u64_t));
    trace_func(NULL,"power_save_io_drain_delay_in_sec:0x%llx", (unsigned long long)power_save_io_drain_delay_in_sec);

    spaces_to_indent += 4;
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    FBE_GET_FIELD_OFFSET(module_name, fbe_database_lun_configuration_t, "max_lun_latency_time_is_sec", &offset);
    FBE_READ_MEMORY(lun_object_ptr + offset, &max_lun_latency_time_is_sec, sizeof(fbe_u64_t));
    trace_func(NULL,"max_lun_latency_time_is_sec:0x%llx", (unsigned long long)max_lun_latency_time_is_sec);

    trace_func(NULL,"\n");
    spaces_to_indent = 8;
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    FBE_GET_FIELD_OFFSET(module_name, fbe_database_lun_configuration_t, "power_saving_idle_time_in_seconds", &offset);
    FBE_READ_MEMORY(lun_object_ptr + offset, &power_saving_idle_time_in_seconds, sizeof(fbe_u64_t));
    trace_func(NULL,"power_saving_idle_time_in_seconds:0x%llx", (unsigned long long)power_saving_idle_time_in_seconds);

    spaces_to_indent += 4;
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    FBE_GET_FIELD_OFFSET(module_name, fbe_database_lun_configuration_t, "power_saving_enabled", &offset);
    FBE_READ_MEMORY(lun_object_ptr + offset, &power_saving_enabled, sizeof(fbe_bool_t));
    trace_func(NULL,"power_saving_enabled:0x%x",power_saving_enabled);

    spaces_to_indent += 4;
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    FBE_GET_FIELD_OFFSET(module_name, fbe_database_lun_configuration_t, "config_flags", &offset);
    FBE_READ_MEMORY(lun_object_ptr + offset, &config_flags, sizeof(fbe_u32_t));
    trace_func(NULL,"config_flags:0x%x",config_flags);

    trace_func(NULL,"\n");
    return FBE_STATUS_OK;
}
/*****************************************
 * end fbe_lun_object_config_debug_trace
 *****************************************/

/*!**************************************************************
 * fbe_vd_object_config_debug_trace()
 ****************************************************************
 * @brief
 *  Display the VD's objects object table 
 *  
 * @param module_name - Module name to use in symbol lookup
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param database_service_ptr -Pointer to the VD config database 
 *.
 * 
 * @return fbe_status_t
 *
 * @author
 *  03-Aug-2011:  Created. Omer Miranda
 *
 ****************************************************************/
fbe_status_t fbe_vd_object_config_debug_trace(const fbe_u8_t * module_name,
                                               fbe_trace_func_t trace_func,
                                               fbe_trace_context_t trace_context,
                                               fbe_dbgext_ptr vd_object_ptr)
{
    fbe_u32_t offset = 0;
    fbe_u32_t spaces_to_indent = 0;
    fbe_u32_t width;
    fbe_lba_t capacity;
    fbe_chunk_size_t chunk_size;
    fbe_raid_group_type_t raid_type;
    fbe_element_size_t element_size;
    fbe_elements_per_parity_t elements_per_parity;
    fbe_raid_group_debug_flags_t debug_flags;
    fbe_u64_t power_saving_idle_time_in_seconds;
    fbe_bool_t	power_saving_enabled;
    fbe_u64_t max_raid_latency_time_in_sec;
    fbe_config_generation_t generation_number;
    fbe_update_raid_type_t update_rg_type;
    fbe_virtual_drive_configuration_mode_t configuration_mode;
    fbe_update_vd_type_t vd_update_type;


    spaces_to_indent += 8;
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    trace_func(NULL,"raid_configuration:");
    trace_func(NULL,"\n");

    spaces_to_indent += 4;
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    FBE_GET_FIELD_OFFSET(module_name, fbe_database_control_vd_set_configuration_t, "width", &offset);
    FBE_READ_MEMORY(vd_object_ptr + offset, &width, sizeof(fbe_u32_t));
    trace_func(NULL,"width:0x%x",width);

    spaces_to_indent += 4;
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    FBE_GET_FIELD_OFFSET(module_name, fbe_database_control_vd_set_configuration_t, "capacity", &offset);
    FBE_READ_MEMORY(vd_object_ptr + offset, &capacity, sizeof(fbe_lba_t));
    trace_func(NULL,"capacity:0x%llx",(unsigned long long)capacity);

    spaces_to_indent += 4;
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    FBE_GET_FIELD_OFFSET(module_name, fbe_database_control_vd_set_configuration_t, "chunk_size", &offset);
    FBE_READ_MEMORY(vd_object_ptr + offset, &chunk_size, sizeof(fbe_chunk_size_t));
    trace_func(NULL,"chunk_size:0x%x",chunk_size);

    trace_func(NULL,"\n");
    spaces_to_indent = 12;
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    FBE_GET_FIELD_OFFSET(module_name, fbe_database_control_vd_set_configuration_t, "raid_type", &offset);
    FBE_READ_MEMORY(vd_object_ptr + offset, &raid_type, sizeof(fbe_raid_group_type_t));
    trace_func(NULL,"raid_type:");
    
    switch(raid_type)
    {
        case FBE_RAID_GROUP_TYPE_UNKNOWN:
            trace_func(NULL,"UNKNOWN");
            break;
        case FBE_RAID_GROUP_TYPE_RAID1:
            trace_func(NULL,"RAID1");
            break;
        case FBE_RAID_GROUP_TYPE_RAID10:
            trace_func(NULL,"RAID10");
            break;
        case FBE_RAID_GROUP_TYPE_RAID3:
            trace_func(NULL,"RAID3");
            break;
        case FBE_RAID_GROUP_TYPE_RAID5:
            trace_func(NULL,"RAID5");
            break;
        case FBE_RAID_GROUP_TYPE_RAID0:
            trace_func(NULL,"RAID0");
            break;
        case FBE_RAID_GROUP_TYPE_RAID6:
            trace_func(NULL,"RAID6");
            break;
        case FBE_RAID_GROUP_TYPE_SPARE:
            trace_func(NULL,"SPARE");
            break;
        case FBE_RAID_GROUP_TYPE_INTERNAL_MIRROR_UNDER_STRIPER:
            trace_func(NULL,"INTERNAL_MIRROR_UNDER_STRIPER");
            break;
        case FBE_RAID_GROUP_TYPE_INTERNAL_METADATA_MIRROR:
            trace_func(NULL,"INTERNAL_METADATA_MIRROR");
            break;
    }

    spaces_to_indent += 4;
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    FBE_GET_FIELD_OFFSET(module_name, fbe_raid_group_configuration_t, "element_size", &offset);
    FBE_READ_MEMORY(vd_object_ptr + offset, &element_size, sizeof(fbe_element_size_t));
    trace_func(NULL,"element_size:0x%x",element_size);

    spaces_to_indent += 4;
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    FBE_GET_FIELD_OFFSET(module_name, fbe_database_control_vd_set_configuration_t, "elements_per_parity", &offset);
    FBE_READ_MEMORY(vd_object_ptr + offset, &elements_per_parity, sizeof(fbe_element_size_t));
    trace_func(NULL,"elements_per_parity:0x%x",elements_per_parity);

    trace_func(NULL,"\n");
    spaces_to_indent = 12;
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    FBE_GET_FIELD_OFFSET(module_name, fbe_database_control_vd_set_configuration_t, "debug_flags", &offset);
    FBE_READ_MEMORY(vd_object_ptr + offset, &debug_flags, sizeof(fbe_raid_group_debug_flags_t));
    trace_func(NULL,"debug_flags:");

    switch(debug_flags)
    {
        case FBE_RAID_GROUP_DEBUG_FLAG_NONE:
            trace_func(NULL,"NONE");
            break;
        case FBE_RAID_GROUP_DEBUG_FLAG_IO_TRACING:
            trace_func(NULL,"IO_TRACING");
            break;
        case FBE_RAID_GROUP_DEBUG_FLAG_REBUILD_TRACING:
            trace_func(NULL,"REBUILD_TRACING");
            break;
        case FBE_RAID_GROUP_DEBUG_FLAGS_TRACE_PMD_NR:
            trace_func(NULL,"TRACE_PMD_NR");
            break;
        case FBE_RAID_GROUP_DEBUG_FLAGS_TRACE_NEEDS_REBUILD:
            trace_func(NULL,"TRACE_NEEDS_REBUILD");
            break;
        case FBE_RAID_GROUP_DEBUG_FLAGS_IOTS_TRACING:
            trace_func(NULL,"IOTS_TRACING");
            break;
        case FBE_RAID_GROUP_DEBUG_FLAGS_ERROR_TRACE_USER_IOTS_ERROR:
            trace_func(NULL,"IOTS_USER_ERROR_TRACING");
            break;
        case FBE_RAID_GROUP_DEBUG_FLAGS_ERROR_TRACE_MONITOR_IOTS_ERROR:
            trace_func(NULL,"IOTS_MONITOR_ERROR_TRACING");
            break;
        case FBE_RAID_GROUP_DEBUG_FLAGS_STRIPE_LOCK_TRACING:
            trace_func(NULL,"STRIPE_LOCK_TRACING");
            break;
    }

    spaces_to_indent += 4;
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    FBE_GET_FIELD_OFFSET(module_name, fbe_database_control_vd_set_configuration_t, "power_saving_idle_time_in_seconds", &offset);
    FBE_READ_MEMORY(vd_object_ptr + offset, &power_saving_idle_time_in_seconds, sizeof(fbe_u64_t));
    trace_func(NULL,"power_saving_idle_time_in_seconds:0x%llx",(unsigned long long)power_saving_idle_time_in_seconds);

    spaces_to_indent += 4;
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    FBE_GET_FIELD_OFFSET(module_name, fbe_database_control_vd_set_configuration_t, "power_saving_enabled", &offset);
    FBE_READ_MEMORY(vd_object_ptr + offset, &power_saving_enabled, sizeof(fbe_bool_t));
    trace_func(NULL,"power_saving_enabled:0x%x",power_saving_enabled);

    trace_func(NULL,"\n");
    spaces_to_indent = 12;
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    FBE_GET_FIELD_OFFSET(module_name, fbe_database_control_vd_set_configuration_t, "max_raid_latency_time_in_sec", &offset);
    FBE_READ_MEMORY(vd_object_ptr + offset, &max_raid_latency_time_in_sec, sizeof(fbe_u64_t));
    trace_func(NULL,"max_raid_latency_time_in_sec:0x%llx",(unsigned long long)max_raid_latency_time_in_sec);

    spaces_to_indent += 4;
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    FBE_GET_FIELD_OFFSET(module_name, fbe_database_control_vd_set_configuration_t, "generation_number", &offset);
    FBE_READ_MEMORY(vd_object_ptr + offset, &generation_number, sizeof(fbe_config_generation_t));
    trace_func(NULL,"generation_number:0x%llx",(unsigned long long)generation_number);

    spaces_to_indent += 4;
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    FBE_GET_FIELD_OFFSET(module_name, fbe_database_control_vd_set_configuration_t, "update_rg_type", &offset);
    FBE_READ_MEMORY(vd_object_ptr + offset, &update_rg_type, sizeof(fbe_update_raid_type_t));
    trace_func(NULL,"update_rg_type:");

    switch(update_rg_type)
    {
        case FBE_UPDATE_RAID_TYPE_INVALID:
            trace_func(NULL,"INVALID");
            break;
        case FBE_UPDATE_RAID_TYPE_ENABLE_POWER_SAVE:
            trace_func(NULL,"ENABLE_POWER_SAVE");
            break;
        case FBE_UPDATE_RAID_TYPE_DISABLE_POWER_SAVE:
            trace_func(NULL,"DISABLE_POWER_SAVE");
            break;
        case FBE_UPDATE_RAID_TYPE_CHANGE_IDLE_TIME:
            trace_func(NULL,"CHANGE_IDLE_TIME");
            break;
    }

    trace_func(NULL,"\n");
    spaces_to_indent = 8;
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    trace_func(NULL,"vd_configuration:");
    trace_func(NULL,"\n");

    spaces_to_indent += 4;
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    FBE_GET_FIELD_OFFSET(module_name, fbe_database_control_vd_set_configuration_t, "configuration_mode", &offset);
    FBE_READ_MEMORY(vd_object_ptr + offset, &configuration_mode, sizeof(fbe_virtual_drive_configuration_mode_t));
    trace_func(NULL,"configuration_mode:");

    switch(configuration_mode)
    {
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_UNKNOWN:
            trace_func(NULL,"UNKNOWN");
            break;
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE:
            trace_func(NULL,"PASS_THRU_FIRST_EDGE");
            break;
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE:
            trace_func(NULL,"PASS_THRU_SECOND_EDGE");
            break;
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE:
            trace_func(NULL,"MIRROR_FIRST_EDGE");
            break;
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE:
            trace_func(NULL,"MIRROR_SECOND_EDGE");
            break;
    }

    spaces_to_indent += 4;
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    FBE_GET_FIELD_OFFSET(module_name, fbe_database_control_vd_set_configuration_t, "update_vd_type", &offset);
    FBE_READ_MEMORY(vd_object_ptr + offset, &vd_update_type, sizeof(fbe_update_vd_type_t));
    trace_func(NULL,"fbe_virtual_drive_configuration_t:");

    switch(vd_update_type)
    {
        case FBE_UPDATE_VD_INVALID:
            trace_func(NULL,"INVALID");
            break;
        case FBE_UPDATE_VD_MODE:
            trace_func(NULL,"MODE");
            break;
    }

    trace_func(NULL,"\n");
    return FBE_STATUS_OK;
}
/*****************************************
 * end fbe_vd_object_config_debug_trace
 *****************************************/

/*!**************************************************************
 * fbe_rg_object_config_debug_trace()
 ****************************************************************
 * @brief
 *  Display the RAID GROUP objects object table 
 *  
 * @param module_name - Module name to use in symbol lookup
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param database_service_ptr -Pointer to the RG config database 
 *.
 * 
 * @return fbe_status_t
 *
 * @author
 *  03-Aug-2011:  Created. Omer Miranda
 *
 ****************************************************************/
fbe_status_t fbe_rg_object_config_debug_trace(const fbe_u8_t * module_name,
                                               fbe_trace_func_t trace_func,
                                               fbe_trace_context_t trace_context,
                                               fbe_dbgext_ptr rg_object_ptr)
{
    fbe_u32_t offset = 0;
    fbe_u32_t spaces_to_indent = 0;
    fbe_u32_t width;
    fbe_lba_t capacity;
    fbe_chunk_size_t chunk_size;
    fbe_raid_group_type_t raid_type;
    fbe_element_size_t element_size;
    fbe_elements_per_parity_t elements_per_parity;
    fbe_raid_group_debug_flags_t debug_flags;
    fbe_u64_t power_saving_idle_time_in_seconds;
    fbe_bool_t	power_saving_enabled;
    fbe_u64_t max_raid_latency_time_in_sec;
    fbe_config_generation_t generation_number;
    fbe_update_raid_type_t update_type;

    spaces_to_indent += 4;
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    FBE_GET_FIELD_OFFSET(module_name, fbe_raid_group_configuration_t, "width", &offset);
    FBE_READ_MEMORY(rg_object_ptr + offset, &width, sizeof(fbe_u32_t));
    trace_func(NULL,"width:0x%x",width);

    spaces_to_indent += 4;
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    FBE_GET_FIELD_OFFSET(module_name, fbe_raid_group_configuration_t, "capacity", &offset);
    FBE_READ_MEMORY(rg_object_ptr + offset, &capacity, sizeof(fbe_lba_t));
    trace_func(NULL,"capacity:0x%llx", (unsigned long long)capacity);

    spaces_to_indent += 4;
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    FBE_GET_FIELD_OFFSET(module_name, fbe_raid_group_configuration_t, "chunk_size", &offset);
    FBE_READ_MEMORY(rg_object_ptr + offset, &chunk_size, sizeof(fbe_chunk_size_t));
    trace_func(NULL,"chunk_size:0x%x",chunk_size);

    trace_func(NULL,"\n");
    spaces_to_indent = 4;
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    FBE_GET_FIELD_OFFSET(module_name, fbe_raid_group_configuration_t, "raid_type", &offset);
    FBE_READ_MEMORY(rg_object_ptr + offset, &raid_type, sizeof(fbe_raid_group_type_t));
    trace_func(NULL,"raid_type:");
    
    switch(raid_type)
    {
        case FBE_RAID_GROUP_TYPE_UNKNOWN:
            trace_func(NULL,"UNKNOWN");
            break;
        case FBE_RAID_GROUP_TYPE_RAID1:
            trace_func(NULL,"RAID1");
            break;
        case FBE_RAID_GROUP_TYPE_RAID10:
            trace_func(NULL,"RAID10");
            break;
        case FBE_RAID_GROUP_TYPE_RAID3:
            trace_func(NULL,"RAID3");
            break;
        case FBE_RAID_GROUP_TYPE_RAID5:
            trace_func(NULL,"RAID5");
            break;
        case FBE_RAID_GROUP_TYPE_RAID0:
            trace_func(NULL,"RAID0");
            break;
        case FBE_RAID_GROUP_TYPE_RAID6:
            trace_func(NULL,"RAID6");
            break;
        case FBE_RAID_GROUP_TYPE_SPARE:
            trace_func(NULL,"SPARE");
            break;
        case FBE_RAID_GROUP_TYPE_INTERNAL_MIRROR_UNDER_STRIPER:
            trace_func(NULL,"INTERNAL_MIRROR_UNDER_STRIPER");
            break;
        case FBE_RAID_GROUP_TYPE_INTERNAL_METADATA_MIRROR:
            trace_func(NULL,"INTERNAL_METADATA_MIRROR");
            break;
    }

    spaces_to_indent += 4;
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    FBE_GET_FIELD_OFFSET(module_name, fbe_raid_group_configuration_t, "element_size", &offset);
    FBE_READ_MEMORY(rg_object_ptr + offset, &element_size, sizeof(fbe_element_size_t));
    trace_func(NULL,"element_size:0x%x",element_size);

    spaces_to_indent += 4;
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    FBE_GET_FIELD_OFFSET(module_name, fbe_raid_group_configuration_t, "elements_per_parity", &offset);
    FBE_READ_MEMORY(rg_object_ptr + offset, &elements_per_parity, sizeof(fbe_element_size_t));
    trace_func(NULL,"elements_per_parity:0x%x",elements_per_parity);

    trace_func(NULL,"\n");
    spaces_to_indent = 4;
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    FBE_GET_FIELD_OFFSET(module_name, fbe_raid_group_configuration_t, "debug_flags", &offset);
    FBE_READ_MEMORY(rg_object_ptr + offset, &debug_flags, sizeof(fbe_raid_group_debug_flags_t));
    trace_func(NULL,"debug_flags:");

    switch(debug_flags)
    {
        case FBE_RAID_GROUP_DEBUG_FLAG_NONE:
            trace_func(NULL,"NONE");
            break;
        case FBE_RAID_GROUP_DEBUG_FLAG_IO_TRACING:
            trace_func(NULL,"IO_TRACING");
            break;
        case FBE_RAID_GROUP_DEBUG_FLAG_REBUILD_TRACING:
            trace_func(NULL,"REBUILD_TRACING");
            break;
        case FBE_RAID_GROUP_DEBUG_FLAGS_TRACE_PMD_NR:
            trace_func(NULL,"TRACE_PMD_NR");
            break;
        case FBE_RAID_GROUP_DEBUG_FLAGS_TRACE_NEEDS_REBUILD:
            trace_func(NULL,"TRACE_NEEDS_REBUILD");
            break;
        case FBE_RAID_GROUP_DEBUG_FLAGS_IOTS_TRACING:
            trace_func(NULL,"IOTS_TRACING");
            break;
        case FBE_RAID_GROUP_DEBUG_FLAGS_ERROR_TRACE_USER_IOTS_ERROR:
            trace_func(NULL,"IOTS_USER_ERROR_TRACING");
            break;
        case FBE_RAID_GROUP_DEBUG_FLAGS_ERROR_TRACE_MONITOR_IOTS_ERROR:
            trace_func(NULL,"IOTS_MONITOR_ERROR_TRACING");
            break;
        case FBE_RAID_GROUP_DEBUG_FLAGS_STRIPE_LOCK_TRACING:
            trace_func(NULL,"STRIPE_LOCK_TRACING");
            break;
    }

    spaces_to_indent += 4;
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    FBE_GET_FIELD_OFFSET(module_name, fbe_raid_group_configuration_t, "power_saving_idle_time_in_seconds", &offset);
    FBE_READ_MEMORY(rg_object_ptr + offset, &power_saving_idle_time_in_seconds, sizeof(fbe_u64_t));
    trace_func(NULL,"power_saving_idle_time_in_seconds:0x%llx", (unsigned long long)power_saving_idle_time_in_seconds);

    spaces_to_indent += 4;
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    FBE_GET_FIELD_OFFSET(module_name, fbe_raid_group_configuration_t, "power_saving_enabled", &offset);
    FBE_READ_MEMORY(rg_object_ptr + offset, &power_saving_enabled, sizeof(fbe_bool_t));
    trace_func(NULL,"power_saving_enabled:0x%x",power_saving_enabled);

    trace_func(NULL,"\n");
    spaces_to_indent = 4;
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    FBE_GET_FIELD_OFFSET(module_name, fbe_raid_group_configuration_t, "max_raid_latency_time_in_sec", &offset);
    FBE_READ_MEMORY(rg_object_ptr + offset, &max_raid_latency_time_in_sec, sizeof(fbe_u64_t));
    trace_func(NULL,"max_raid_latency_time_in_sec:0x%llx", (unsigned long long)max_raid_latency_time_in_sec);

    spaces_to_indent += 4;
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    FBE_GET_FIELD_OFFSET(module_name, fbe_raid_group_configuration_t, "generation_number", &offset);
    FBE_READ_MEMORY(rg_object_ptr + offset, &generation_number, sizeof(fbe_config_generation_t));
    trace_func(NULL,"generation_number:0x%llx", (unsigned long long)generation_number);

    spaces_to_indent += 4;
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    FBE_GET_FIELD_OFFSET(module_name, fbe_raid_group_configuration_t, "update_type", &offset);
    FBE_READ_MEMORY(rg_object_ptr + offset, &update_type, sizeof(fbe_update_raid_type_t));
    trace_func(NULL,"update_type:");

    switch(update_type)
    {
        case FBE_UPDATE_RAID_TYPE_INVALID:
            trace_func(NULL,"INVALID");
            break;
        case FBE_UPDATE_RAID_TYPE_ENABLE_POWER_SAVE:
            trace_func(NULL,"ENABLE_POWER_SAVE");
            break;
        case FBE_UPDATE_RAID_TYPE_DISABLE_POWER_SAVE:
            trace_func(NULL,"DISABLE_POWER_SAVE");
            break;
        case FBE_UPDATE_RAID_TYPE_CHANGE_IDLE_TIME:
            trace_func(NULL,"CHANGE_IDLE_TIME");
            break;
    }

    trace_func(NULL,"\n");
    trace_func(NULL,"\n");
    return FBE_STATUS_OK;
}
/*****************************************
 * end fbe_rg_object_config_debug_trace
 *****************************************/
/*!**************************************************************
 * fbe_database_display_user_table()
 ****************************************************************
 * @brief
 *  Display the database service user table 
 *  
 * @param module_name - Module name to use in symbol lookup
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param database_service_ptr -Pointer to the database service user table
 *.
 * 
 * @return fbe_status_t
 *
 * @author
 *  03-Aug-2011:  Created. Omer Miranda
 *
 ****************************************************************/
fbe_status_t fbe_database_display_user_table(const fbe_u8_t * module_name,
                                             fbe_trace_func_t trace_func,
                                             fbe_trace_context_t trace_context,
                                             fbe_dbgext_ptr user_table_ptr,
                                             fbe_object_id_t in_object_id)
{
    fbe_u32_t offset = 0;
    fbe_u32_t header_offset = 0;
    fbe_dbgext_ptr user_table_content_ptr = 0;
    fbe_dbgext_ptr user_entry_ptr = 0;
    fbe_u32_t database_user_entry_size;
    fbe_dbgext_ptr fbe_pvd_config_ptr = 0;
    fbe_dbgext_ptr fbe_lun_config_ptr = 0;
    fbe_dbgext_ptr fbe_rg_config_ptr = 0;
    database_class_id_t class_id;
    database_config_entry_state_t state;
    fbe_persist_entry_id_t entry_id;
    fbe_object_id_t object_id;
    fbe_u32_t spaces_to_indent = 0;

    database_table_size_t table_size;
    fbe_u32_t table_entries;
    fbe_dbgext_ptr debug_ptr = 0;

    FBE_GET_FIELD_OFFSET(module_name,database_table_t,"table_size",&offset);
    FBE_READ_MEMORY(user_table_ptr + offset, &table_size, sizeof(database_table_size_t));

    FBE_GET_FIELD_OFFSET(module_name,database_table_t,"table_content",&offset);
    user_table_content_ptr = user_table_ptr + offset;
    FBE_READ_MEMORY(user_table_content_ptr, &user_entry_ptr, sizeof(fbe_dbgext_ptr));

    /* Get the size of the database object entry. */
    FBE_GET_TYPE_SIZE(module_name, database_user_entry_t, &database_user_entry_size);

    debug_ptr = user_entry_ptr;
    for(table_entries = 0;table_entries < table_size ; table_entries++)
    {
        FBE_GET_FIELD_OFFSET(module_name,database_user_entry_t,"header",&header_offset);
        FBE_GET_FIELD_OFFSET(module_name, database_table_header_t, "object_id", &offset);
        FBE_READ_MEMORY(debug_ptr + header_offset + offset, &object_id, sizeof(fbe_object_id_t));
        if(object_id == in_object_id || in_object_id == FBE_OBJECT_ID_INVALID)
        {
            FBE_GET_FIELD_OFFSET(module_name,database_table_header_t,"state",&offset);
            FBE_READ_MEMORY(debug_ptr + header_offset + offset , &state, sizeof(database_config_entry_state_t));

            if(state != DATABASE_CONFIG_ENTRY_INVALID)
            {
                spaces_to_indent = 4;
                fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
                trace_func(NULL,"USER ENTRY:\n");
                fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
                trace_func(NULL,"state:");
                switch(state)
                {
                    case DATABASE_CONFIG_ENTRY_VALID:
                        trace_func(NULL,"VALID");
                        break;
                    case DATABASE_CONFIG_ENTRY_CREATE:
                        trace_func(NULL,"CREATE");
                        break;
                    case DATABASE_CONFIG_ENTRY_DESTROY:
                        trace_func(NULL,"DESTROY");
                        break;
                    case DATABASE_CONFIG_ENTRY_MODIFY:
                        trace_func(NULL,"MODIFY");
                        break;
                }

                spaces_to_indent += 4;
                fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
                FBE_GET_FIELD_OFFSET(module_name, database_table_header_t, "entry_id", &offset);
                FBE_READ_MEMORY(debug_ptr + header_offset + offset, &entry_id, sizeof(fbe_persist_entry_id_t));
                trace_func(NULL,"entry_id:0x%llx", (unsigned long long)entry_id);

                spaces_to_indent += 4;
                fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
                FBE_GET_FIELD_OFFSET(module_name, database_table_header_t, "object_id", &offset);
                FBE_READ_MEMORY(debug_ptr + header_offset + offset, &object_id, sizeof(fbe_object_id_t));
                trace_func(NULL,"object_id:0x%x",object_id);

                trace_func(NULL,"\n");
                spaces_to_indent = 4;
                fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
                FBE_GET_FIELD_OFFSET(module_name,database_user_entry_t,"db_class_id",&offset);
                FBE_READ_MEMORY(debug_ptr + offset , &class_id, sizeof(database_class_id_t));

                FBE_GET_FIELD_OFFSET(module_name,database_user_entry_t,"user_data_union",&offset);

                switch(class_id)
                {
                    case DATABASE_CLASS_ID_PROVISION_DRIVE:
                        trace_func(NULL,"DATABASE_CLASS_ID_PROVISION_DRIVE:\n");
                        fbe_pvd_config_ptr = debug_ptr + offset;
                        fbe_pvd_user_config_debug_trace(module_name, trace_func, trace_context,fbe_pvd_config_ptr);
                        break;
                    case DATABASE_CLASS_ID_LUN:
                        trace_func(NULL,"DATABASE_CLASS_ID_LUN:\n");
                        fbe_lun_config_ptr = debug_ptr + offset;
                        fbe_lun_user_config_debug_trace(module_name, trace_func, trace_context,fbe_lun_config_ptr);
                        break;
                    case DATABASE_CLASS_ID_RAID_START:
                    case DATABASE_CLASS_ID_MIRROR:
                    case DATABASE_CLASS_ID_STRIPER:
                    case DATABASE_CLASS_ID_PARITY:
                    case DATABASE_CLASS_ID_RAID_END:
                        trace_func(NULL,"DATABASE_CLASS_ID_RAID:\n");
                        fbe_rg_config_ptr = debug_ptr + offset;
                        fbe_rg_user_config_debug_trace(module_name, trace_func, trace_context,fbe_rg_config_ptr);
                        break;
                    case DATABASE_CLASS_ID_EXTENT_POOL:
                        trace_func(NULL,"DATABASE_CLASS_ID_EXTENT_POOL:\n");
                        fbe_ext_pool_user_config_debug_trace(module_name, trace_func, trace_context,debug_ptr + offset);
                        break;
                    case DATABASE_CLASS_ID_EXTENT_POOL_LUN:
                        trace_func(NULL,"DATABASE_CLASS_ID_EXTENT_POOL_LUN:\n");
                        fbe_ext_pool_lun_user_config_debug_trace(module_name, trace_func, trace_context,debug_ptr + offset);
                        break;
                    case DATABASE_CLASS_ID_EXTENT_POOL_METADATA_LUN:
                        trace_func(NULL,"DATABASE_CLASS_ID_EXTENT_POOL_METADATA_LUN:\n");
                        fbe_ext_pool_lun_user_config_debug_trace(module_name, trace_func, trace_context,debug_ptr + offset);
                        break;
                }
                trace_func(NULL,"\n");
                trace_func(NULL,"\n");
            }
            debug_ptr += database_user_entry_size;
        }
        else
        {
            debug_ptr += database_user_entry_size;
            continue;
        }
    }
    return FBE_STATUS_OK;
}
/*****************************************
 * end fbe_database_display_user_table
 ******************************************/

/*!**************************************************************
 * fbe_pvd_user_config_debug_trace()
 ****************************************************************
 * @brief
 *  Display the PVD objects user table 
 *  
 * @param module_name - Module name to use in symbol lookup
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param database_service_ptr -Pointer to the PVD config database 
 *.
 * 
 * @return fbe_status_t
 *
 * @author
 *  03-Aug-2011:  Created. Omer Miranda
 *
 ****************************************************************/
 fbe_status_t fbe_pvd_user_config_debug_trace(const fbe_u8_t * module_name,
                                               fbe_trace_func_t trace_func,
                                               fbe_trace_context_t trace_context,
                                               fbe_dbgext_ptr pvd_user_ptr)
{
    fbe_u32_t offset = 0;
    fbe_u8_t opaque_data[FBE_DATABASE_PROVISION_DRIVE_OPAQUE_DATA_MAX];
    fbe_u32_t spaces_to_indent = 0;
    fbe_u32_t pool_id;
    fbe_u32_t index;

    spaces_to_indent += 8;
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    FBE_GET_FIELD_OFFSET(module_name, database_pvd_user_data_t, "opaque_data", &offset);
    FBE_READ_MEMORY(pvd_user_ptr + offset, &opaque_data, FBE_DATABASE_PROVISION_DRIVE_OPAQUE_DATA_MAX);

    if(strlen(opaque_data) > 0)
    {
        trace_func(NULL,"opaque_data:");
        for(index = 0;index < FBE_DATABASE_PROVISION_DRIVE_OPAQUE_DATA_MAX; index++)
        {
            trace_func(NULL,"%x",opaque_data[index]);
        }
    }
    else
    {
        trace_func(NULL,"opaque_data: %s","NULL");
    }

    spaces_to_indent += 4;
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    FBE_GET_FIELD_OFFSET(module_name, database_pvd_user_data_t, "pool_id", &offset);
    FBE_READ_MEMORY(pvd_user_ptr + offset, &pool_id, sizeof(fbe_u32_t));
    trace_func(NULL,"pool_id:0x%x",pool_id);

    trace_func(NULL,"\n");
    return FBE_STATUS_OK;
}
/*****************************************
 * end fbe_pvd_user_config_debug_trace
 ******************************************/

/*!**************************************************************
 * fbe_lun_user_config_debug_trace()
 ****************************************************************
 * @brief
 *  Display the LUN objects user table 
 *  
 * @param module_name - Module name to use in symbol lookup
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param database_service_ptr -Pointer to the LUN config database 
 *.
 * 
 * @return fbe_status_t
 *
 * @author
 *  03-Aug-2011:  Created. Omer Miranda
 *
 ****************************************************************/
fbe_status_t fbe_lun_user_config_debug_trace(const fbe_u8_t * module_name,
                                             fbe_trace_func_t trace_func,
                                             fbe_trace_context_t trace_context,
                                             fbe_dbgext_ptr lun_user_ptr)
{
    fbe_u32_t offset = 0;
    fbe_u32_t spaces_to_indent = 0;
    fbe_bool_t export_device_b;
    fbe_lun_number_t lun_number;
    fbe_assigned_wwid_t wwn;
    fbe_u32_t char_offset = 0;
    fbe_u8_t world_wide_name[16];
    fbe_user_defined_lun_name_t user_defined_name;
    fbe_u8_t name[FBE_USER_DEFINED_LUN_NAME_LEN];
    fbe_u32_t index;
    
    spaces_to_indent += 8;
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    FBE_GET_FIELD_OFFSET(module_name, database_lu_user_data_t, "export_device_b", &offset);
    FBE_READ_MEMORY(lun_user_ptr + offset, &export_device_b, sizeof(fbe_bool_t));
    trace_func(NULL,"export_device_b:0x%x",export_device_b);

    spaces_to_indent += 4;
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    FBE_GET_FIELD_OFFSET(module_name, database_lu_user_data_t, "lun_number", &offset);
    FBE_READ_MEMORY(lun_user_ptr + offset, &lun_number, sizeof(fbe_lun_number_t));
    trace_func(NULL,"lun_number:0x%x",lun_number);

    spaces_to_indent += 4;
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    FBE_GET_FIELD_OFFSET(module_name, database_lu_user_data_t, "world_wide_name", &offset);
    FBE_READ_MEMORY(lun_user_ptr + offset , &wwn, sizeof(fbe_assigned_wwid_t));
    FBE_GET_FIELD_OFFSET(module_name, fbe_assigned_wwid_t, "bytes", &char_offset);	
    FBE_READ_MEMORY(lun_user_ptr + offset + char_offset, &world_wide_name, 16);
    if(strlen(world_wide_name) > 0)
    {
        trace_func(NULL,"WWN:");
        for(index = 0; index < 16; index++)
        {
            if(index == 15)
            {
                trace_func(NULL,"%x",world_wide_name[index]);
            }
            else
            {
                trace_func(NULL,"%x:",world_wide_name[index]);
            }
        }
    }
    else
    {
        trace_func(NULL,"WWN: %s","NULL");
    }

    trace_func(NULL,"\n");
    spaces_to_indent = 8;
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    FBE_GET_FIELD_OFFSET(module_name, database_lu_user_data_t, "user_defined_name", &offset);
    FBE_READ_MEMORY(lun_user_ptr + offset, &user_defined_name, sizeof(fbe_user_defined_lun_name_t));
    FBE_GET_FIELD_OFFSET(module_name, fbe_user_defined_lun_name_t, "name", &char_offset);	
    FBE_READ_MEMORY(lun_user_ptr + offset + char_offset, &name, FBE_USER_DEFINED_LUN_NAME_LEN);
    if(strlen(name) > 0)
    {
        trace_func(NULL,"user_defined_name: %s",name);
    }
    else
    {
        trace_func(NULL,"user_defined_name: %s","NULL");
    }

    trace_func(NULL,"\n");
    return FBE_STATUS_OK;
}
/*****************************************
 * end fbe_lun_user_config_debug_trace
 ******************************************/

/*!**************************************************************
 * fbe_rg_user_config_debug_trace()
 ****************************************************************
 * @brief
 *  Display the RG objects user table 
 *  
 * @param module_name - Module name to use in symbol lookup
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param database_service_ptr -Pointer to the RG config database 
 *.
 * 
 * @return fbe_status_t
 *
 * @author
 *  03-Aug-2011:  Created. Omer Miranda
 *
 ****************************************************************/
fbe_status_t fbe_rg_user_config_debug_trace(const fbe_u8_t * module_name,
                                            fbe_trace_func_t trace_func,
                                            fbe_trace_context_t trace_context,
                                            fbe_dbgext_ptr rg_user_ptr)
{
    fbe_u32_t offset = 0;
    fbe_u32_t spaces_to_indent = 0;
    fbe_bool_t is_system;
    fbe_raid_group_number_t raid_group_number;

    spaces_to_indent += 8;
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    FBE_GET_FIELD_OFFSET(module_name, database_rg_user_data_t, "is_system", &offset);
    FBE_READ_MEMORY(rg_user_ptr + offset, &is_system, sizeof(fbe_bool_t));
    trace_func(NULL,"is_system:0x%x",is_system);

    spaces_to_indent += 4;
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    FBE_GET_FIELD_OFFSET(module_name, database_rg_user_data_t, "raid_group_number", &offset);
    FBE_READ_MEMORY(rg_user_ptr + offset, &raid_group_number, sizeof(fbe_raid_group_number_t));
    trace_func(NULL,"raid_group_number:0x%x",raid_group_number);

    trace_func(NULL,"\n");
    return FBE_STATUS_OK;
}
/*****************************************
 * end fbe_rg_user_config_debug_trace
 ******************************************/

/*!**************************************************************
 * fbe_database_display_edge_table()
 ****************************************************************
 * @brief
 *  Display the database service Edge table 
 *  
 * @param module_name - Module name to use in symbol lookup
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param database_service_ptr -Pointer to the database service edge table
 *.
 * 
 * @return fbe_status_t
 *
 * @author
 *  05-Aug-2011:  Created. Omer Miranda
 *
 ****************************************************************/
fbe_status_t fbe_database_display_edge_table(const fbe_u8_t * module_name,
                                             fbe_trace_func_t trace_func,
                                             fbe_trace_context_t trace_context,
                                             fbe_dbgext_ptr edge_table_ptr,
                                             fbe_object_id_t in_object_id)
{
    fbe_u32_t offset = 0;
    fbe_u32_t header_offset = 0;
    fbe_u32_t spaces_to_indent = 0;
    fbe_dbgext_ptr edge_table_content_ptr = 0;
    fbe_dbgext_ptr edge_entry_ptr = 0;
    fbe_u32_t database_edge_entry_size;
    database_config_entry_state_t state;
    fbe_persist_entry_id_t entry_id;
    fbe_object_id_t object_id;
    fbe_object_id_t server_id;
    fbe_edge_index_t client_index;
    fbe_lba_t capacity;
    fbe_lba_t edge_offset;
    database_table_size_t table_size;
    fbe_u32_t table_entries;
    fbe_dbgext_ptr debug_ptr = 0;

    FBE_GET_FIELD_OFFSET(module_name,database_table_t,"table_size",&offset);
    FBE_READ_MEMORY(edge_table_ptr + offset, &table_size, sizeof(database_table_size_t));

    FBE_GET_FIELD_OFFSET(module_name,database_table_t,"table_content",&offset);
    edge_table_content_ptr = edge_table_ptr + offset;
    FBE_READ_MEMORY(edge_table_content_ptr, &edge_entry_ptr, sizeof(fbe_dbgext_ptr));

    /* Get the size of the database object entry. */
    FBE_GET_TYPE_SIZE(module_name, database_edge_entry_t, &database_edge_entry_size);


    debug_ptr = edge_entry_ptr;
    for(table_entries = 0;table_entries < table_size ; table_entries++)
    {
        FBE_GET_FIELD_OFFSET(module_name,database_edge_entry_t,"header",&header_offset);
        FBE_GET_FIELD_OFFSET(module_name, database_table_header_t, "object_id", &offset);
        FBE_READ_MEMORY(debug_ptr + header_offset + offset, &object_id, sizeof(fbe_object_id_t));
        if(object_id == in_object_id || in_object_id == FBE_OBJECT_ID_INVALID)
        {
            FBE_GET_FIELD_OFFSET(module_name,database_table_header_t,"state",&offset);
            FBE_READ_MEMORY(debug_ptr + header_offset + offset , &state, sizeof(database_config_entry_state_t));
            if(state != DATABASE_CONFIG_ENTRY_INVALID)
            {
                spaces_to_indent = 4;
                fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
                trace_func(NULL,"EDGE ENTRY:\n");
                fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
                trace_func(NULL,"state:");
                switch(state)
                {
                    case DATABASE_CONFIG_ENTRY_VALID:
                        trace_func(NULL,"VALID");
                        break;
                    case DATABASE_CONFIG_ENTRY_CREATE:
                        trace_func(NULL,"CREATE");
                        break;
                    case DATABASE_CONFIG_ENTRY_DESTROY:
                        trace_func(NULL,"DESTROY");
                        break;
                    case DATABASE_CONFIG_ENTRY_MODIFY:
                        trace_func(NULL,"MODIFY");
                        break;
                }
                spaces_to_indent += 4;
                fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
                FBE_GET_FIELD_OFFSET(module_name, database_table_header_t, "entry_id", &offset);
                FBE_READ_MEMORY(debug_ptr + header_offset + offset, &entry_id, sizeof(fbe_persist_entry_id_t));
                trace_func(NULL,"entry_id:0x%llx", (unsigned long long)entry_id);

                spaces_to_indent += 4;
                fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
                FBE_GET_FIELD_OFFSET(module_name, database_table_header_t, "object_id", &offset);
                FBE_READ_MEMORY(debug_ptr + header_offset + offset, &object_id, sizeof(fbe_object_id_t));
                trace_func(NULL,"object_id:0x%x",object_id);

                trace_func(NULL,"\n");
                spaces_to_indent = 4;
                fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
                FBE_GET_FIELD_OFFSET(module_name, database_edge_entry_t, "server_id", &offset);
                FBE_READ_MEMORY(debug_ptr + offset, &server_id, sizeof(fbe_object_id_t));
                trace_func(NULL,"server_id:0x%x",server_id);

                spaces_to_indent += 4;
                fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
                FBE_GET_FIELD_OFFSET(module_name, database_edge_entry_t, "client_index", &offset);
                FBE_READ_MEMORY(debug_ptr + offset, &client_index, sizeof(fbe_edge_index_t));
                trace_func(NULL,"client_index:0x%x",client_index);

                spaces_to_indent += 4;
                fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
                FBE_GET_FIELD_OFFSET(module_name, database_edge_entry_t, "capacity", &offset);
                FBE_READ_MEMORY(debug_ptr + offset, &capacity, sizeof(fbe_lba_t));
                trace_func(NULL,"capacity:0x%llx", (unsigned long long)capacity);

                spaces_to_indent += 4;
                fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
                FBE_GET_FIELD_OFFSET(module_name, database_edge_entry_t, "offset", &offset);
                FBE_READ_MEMORY(debug_ptr + offset, &edge_offset, sizeof(fbe_lba_t));
                trace_func(NULL,"offset:0x%llx", (unsigned long long)edge_offset);

                trace_func(NULL,"\n");
                trace_func(NULL,"\n");
            }
            debug_ptr += database_edge_entry_size;
        }
        else
        {
            debug_ptr += database_edge_entry_size;
            continue;
        }
    }
    return FBE_STATUS_OK;
}
/*****************************************
 * end fbe_database_display_edge_table
 ******************************************/

/*!**************************************************************
 * fbe_database_display_global_info_table()
 ****************************************************************
 * @brief
 *  Display the database service Global Info table 
 *  
 * @param module_name - Module name to use in symbol lookup
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param database_service_ptr -Pointer to the database service global info table
 *.
 * 
 * @return fbe_status_t
 *
 * @author
 *  11-Aug-2011:  Created. Omer Miranda
 *
 ****************************************************************/
fbe_status_t fbe_database_display_global_info_table(const fbe_u8_t * module_name,
                                                    fbe_trace_func_t trace_func,
                                                    fbe_trace_context_t trace_context,
                                                    fbe_dbgext_ptr global_info_ptr,
                                                    fbe_object_id_t in_object_id)
{
    fbe_u32_t offset = 0;
    fbe_u32_t header_offset = 0;
    fbe_u32_t spaces_to_indent = 0;
    database_table_size_t table_size;
    fbe_u32_t table_entries;
    fbe_dbgext_ptr debug_ptr = 0;
    fbe_dbgext_ptr global_entry_ptr = 0;
    fbe_dbgext_ptr global_info_union_ptr = 0;
    fbe_dbgext_ptr power_saving_info_ptr = 0;
    fbe_dbgext_ptr spare_config_info_ptr = 0;
    fbe_dbgext_ptr system_generation_info_ptr = 0;
    database_global_info_type_t type;
    fbe_u32_t database_global_entry_size;
    database_config_entry_state_t state;
    fbe_persist_entry_id_t entry_id;
    fbe_object_id_t object_id;


    database_table_t global_table = {0};
    database_global_info_entry_t debug_global = {0};
    

    FBE_READ_MEMORY(global_info_ptr , &global_table, sizeof(database_table_t));


    FBE_GET_FIELD_OFFSET(module_name,database_table_t,"table_size",&offset);
    FBE_READ_MEMORY(global_info_ptr + offset, &table_size, sizeof(database_table_size_t));

    FBE_GET_FIELD_OFFSET(module_name,database_table_t,"table_content",&offset);
    global_info_union_ptr = global_info_ptr + offset;
    FBE_READ_MEMORY(global_info_union_ptr, &global_entry_ptr, sizeof(fbe_dbgext_ptr));

    /* Get the size of the database object entry. */
    FBE_GET_TYPE_SIZE(module_name, database_global_info_entry_t, &database_global_entry_size);

    debug_ptr = global_entry_ptr;

    for(table_entries = 0;table_entries < table_size ; table_entries++)
    {
        FBE_READ_MEMORY(debug_ptr , &debug_global, sizeof(database_global_info_entry_t));

        FBE_GET_FIELD_OFFSET(module_name,database_global_info_entry_t,"header",&header_offset);
        FBE_GET_FIELD_OFFSET(module_name, database_table_header_t, "object_id", &offset);
        FBE_READ_MEMORY(debug_ptr + header_offset + offset, &object_id, sizeof(fbe_object_id_t));
        if(object_id == in_object_id || in_object_id == FBE_OBJECT_ID_INVALID)
        {
            FBE_GET_FIELD_OFFSET(module_name,database_table_header_t,"state",&offset);
            FBE_READ_MEMORY(debug_ptr + header_offset + offset , &state, sizeof(database_config_entry_state_t));

            if(state != DATABASE_CONFIG_ENTRY_INVALID)
            {
                spaces_to_indent = 4;
                fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
                trace_func(NULL,"GLOBAL INFO ENTRY:\n");
                fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
                trace_func(NULL,"state:");
                switch(state)
                {
                    case DATABASE_CONFIG_ENTRY_VALID:
                        trace_func(NULL,"VALID");
                        break;
                    case DATABASE_CONFIG_ENTRY_CREATE:
                        trace_func(NULL,"CREATE");
                        break;
                    case DATABASE_CONFIG_ENTRY_DESTROY:
                        trace_func(NULL,"DESTROY");
                        break;
                    case DATABASE_CONFIG_ENTRY_MODIFY:
                        trace_func(NULL,"MODIFY");
                        break;
                    case DATABASE_CONFIG_ENTRY_UNCOMMITTED:
                        trace_func(NULL,"UNCOMMITTED");
                        break;
                }

                spaces_to_indent += 4;
                fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
                FBE_GET_FIELD_OFFSET(module_name, database_table_header_t, "entry_id", &offset);
                FBE_READ_MEMORY(debug_ptr + header_offset + offset, &entry_id, sizeof(fbe_persist_entry_id_t));
                trace_func(NULL,"entry_id:0x%llx", (unsigned long long)entry_id);

                spaces_to_indent += 4;
                fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
                FBE_GET_FIELD_OFFSET(module_name, database_table_header_t, "object_id", &offset);
                FBE_READ_MEMORY(debug_ptr + header_offset + offset, &object_id, sizeof(fbe_object_id_t));
                trace_func(NULL,"object_id:0x%x",object_id);

                trace_func(NULL,"\n");
                spaces_to_indent = 4;
                fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
                FBE_GET_FIELD_OFFSET(module_name,database_global_info_entry_t,"type",&offset);
                FBE_READ_MEMORY(debug_ptr + offset , &type, sizeof(database_global_info_type_t));

                FBE_GET_FIELD_OFFSET(module_name,database_global_info_entry_t,"info_union",&offset);

                switch(type)
                {
                    case DATABASE_GLOBAL_INFO_TYPE_SYSTEM_POWER_SAVE:
                        trace_func(NULL,"DATABASE_GLOBAL_INFO_TYPE_SYSTEM_POWER_SAVE:\n");
                        power_saving_info_ptr = debug_ptr + offset;
                        fbe_global_info_power_saving_debug_trace(module_name, trace_func, trace_context,power_saving_info_ptr);
                        break;
                    case DATABASE_GLOBAL_INFO_TYPE_SYSTEM_SPARE:
                        trace_func(NULL,"DATABASE_GLOBAL_INFO_TYPE_SYSTEM_SPARE:\n");
                        spare_config_info_ptr = debug_ptr + offset;
                        fbe_global_info_spare_config_debug_trace(module_name, trace_func, trace_context,spare_config_info_ptr);
                        break;
                    case DATABASE_GLOBAL_INFO_TYPE_SYSTEM_GENERATION:
                        trace_func(NULL,"DATABASE_GLOBAL_INFO_TYPE_SYSTEM_GENERATION:\n");
                        system_generation_info_ptr = debug_ptr + offset;
                        fbe_global_info_system_generation_debug_trace(module_name, trace_func, trace_context,system_generation_info_ptr);
                        break;
                }
                trace_func(NULL,"\n");
                trace_func(NULL,"\n");
            }
            debug_ptr += database_global_entry_size;
        }
        else
        {
            debug_ptr += database_global_entry_size;
            continue;
        }
    }
    return FBE_STATUS_OK;
}


/*!**************************************************************
 * fbe_status_t fbe_global_info_power_saving_debug_trace()
 ****************************************************************
 * @brief
 *  Display the database service Global power saving info 
 *  
 * @param module_name - Module name to use in symbol lookup
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param database_service_ptr -Pointer to the database service power saving info
 *.
 * 
 * @return fbe_status_t
 *
 * @author
 *  11-Aug-2011:  Created. Omer Miranda
 *
 ****************************************************************/
fbe_status_t fbe_global_info_power_saving_debug_trace(const fbe_u8_t * module_name,
                                                     fbe_trace_func_t trace_func,
                                                     fbe_trace_context_t trace_context,
                                                     fbe_dbgext_ptr power_saving_info_ptr)
{
    fbe_u32_t offset = 0;	
    fbe_u32_t spaces_to_indent = 0;
    fbe_bool_t	enabled;
    fbe_u64_t	hibernation_wake_up_time_in_minutes;

    spaces_to_indent += 8;
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    FBE_GET_FIELD_OFFSET(module_name,fbe_system_power_saving_info_t,"enabled",&offset);
    FBE_READ_MEMORY(power_saving_info_ptr + offset , &enabled, sizeof(fbe_bool_t));
    trace_func(NULL,"enabled:0x%x",enabled);

    spaces_to_indent += 8;
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    FBE_GET_FIELD_OFFSET(module_name,fbe_system_power_saving_info_t,"hibernation_wake_up_time_in_minutes",&offset);
    FBE_READ_MEMORY(power_saving_info_ptr + offset , &hibernation_wake_up_time_in_minutes, sizeof(fbe_u64_t));
    trace_func(NULL,"hibernation_wake_up_time_in_minutes:0x%llx", (unsigned long long)hibernation_wake_up_time_in_minutes);

    trace_func(NULL,"\n");
    return FBE_STATUS_OK;
}
/*****************************************
 * end fbe_global_info_power_saving_debug_trace
 ******************************************/

/*!**************************************************************
 * fbe_status_t fbe_global_info_spare_config_debug_trace()
 ****************************************************************
 * @brief
 *  Display the database service Global spare config info 
 *  
 * @param module_name - Module name to use in symbol lookup
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param database_service_ptr -Pointer to the database service spare config info
 *.
 * 
 * @return fbe_status_t
 *
 * @author
 *  11-Aug-2011:  Created. Omer Miranda
 *
 ****************************************************************/
fbe_status_t fbe_global_info_spare_config_debug_trace(const fbe_u8_t * module_name,
                                                     fbe_trace_func_t trace_func,
                                                     fbe_trace_context_t trace_context,
                                                     fbe_dbgext_ptr spare_config_info_ptr)
{
    fbe_u32_t offset = 0;	
    fbe_u32_t spaces_to_indent = 0;
    fbe_u64_t permanent_spare_trigger_time;

    spaces_to_indent += 8;
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    FBE_GET_FIELD_OFFSET(module_name,fbe_system_spare_config_info_t,"permanent_spare_trigger_time",&offset);
    FBE_READ_MEMORY(spare_config_info_ptr + offset , &permanent_spare_trigger_time, sizeof(fbe_u64_t));
    trace_func(NULL,"permanent_spare_trigger_time:0x%llx", (unsigned long long)permanent_spare_trigger_time);

    trace_func(NULL,"\n");
    return FBE_STATUS_OK;
}
/*****************************************
 * end fbe_global_info_spare_config_debug_trace
 ******************************************/
    
/*!**************************************************************
 * fbe_status_t fbe_global_info_system_generation_debug_trace()
 ****************************************************************
 * @brief
 *  Display the database service Global system generation info 
 *  
 * @param module_name - Module name to use in symbol lookup
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param database_service_ptr -Pointer to the database service system generation info
 *.
 * 
 * @return fbe_status_t
 *
 * @author
 *  11-Aug-2011:  Created. Omer Miranda
 *
 ****************************************************************/
fbe_status_t fbe_global_info_system_generation_debug_trace(const fbe_u8_t * module_name,
                                                           fbe_trace_func_t trace_func,
                                                           fbe_trace_context_t trace_context,
                                                           fbe_dbgext_ptr system_generation_info_ptr)
{
    fbe_u32_t offset = 0;	
    fbe_u32_t spaces_to_indent = 0;
    fbe_config_generation_t current_generation_number;

    spaces_to_indent += 8;
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    FBE_GET_FIELD_OFFSET(module_name,fbe_system_generation_info_t,"current_generation_number",&offset);
    FBE_READ_MEMORY(system_generation_info_ptr + offset , &current_generation_number, sizeof(fbe_config_generation_t));
    trace_func(NULL,"current_generation_number:0x%llx", (unsigned long long)current_generation_number);

    trace_func(NULL,"\n");
    return FBE_STATUS_OK;
}
/*****************************************
 * end fbe_global_info_system_generation_debug_trace
 ******************************************/

fbe_status_t fbe_ext_pool_object_config_debug_trace(const fbe_u8_t * module_name,
                                               fbe_trace_func_t trace_func,
                                               fbe_trace_context_t trace_context,
                                               fbe_dbgext_ptr ext_pool_object_ptr)
{
    fbe_u32_t offset = 0;
    fbe_u32_t width;
    fbe_lba_t capacity;
    fbe_config_generation_t generation_number;
    fbe_drive_type_t drives_type;
    fbe_u32_t spaces_to_indent = 0;

    FBE_GET_FIELD_OFFSET(module_name, fbe_extent_pool_configuration_t, "width", &offset);
    FBE_READ_MEMORY(ext_pool_object_ptr + offset, &width, sizeof(fbe_u32_t));

    spaces_to_indent += 8;
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    trace_func(NULL,"width:%d", width);

    FBE_GET_FIELD_OFFSET(module_name, fbe_extent_pool_configuration_t, "capacity", &offset);
    FBE_READ_MEMORY(ext_pool_object_ptr + offset, &capacity, sizeof(fbe_lba_t));

    spaces_to_indent += 4;
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    trace_func(NULL,"capacity:0x%llx", (unsigned long long)capacity);

    spaces_to_indent += 4;
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    FBE_GET_FIELD_OFFSET(module_name, fbe_extent_pool_configuration_t, "drives_type", &offset);
    FBE_READ_MEMORY(ext_pool_object_ptr + offset, &drives_type, sizeof(fbe_drive_type_t));
    trace_func(NULL,"drives_type:0x%x",drives_type);

    trace_func(NULL,"\n");
    spaces_to_indent = 8;
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    FBE_GET_FIELD_OFFSET(module_name, fbe_extent_pool_configuration_t, "generation_number", &offset);
    FBE_READ_MEMORY(ext_pool_object_ptr + offset, &generation_number, sizeof(fbe_config_generation_t));
    trace_func(NULL,"generation_number:0x%llx", (unsigned long long)generation_number);
    
    trace_func(NULL,"\n");
    return FBE_STATUS_OK;
}

fbe_status_t fbe_ext_pool_lun_object_config_debug_trace(const fbe_u8_t * module_name,
                                               fbe_trace_func_t trace_func,
                                               fbe_trace_context_t trace_context,
                                               fbe_dbgext_ptr ext_pool_lun_object_ptr)
{
    fbe_u32_t offset = 0;
    fbe_edge_index_t server_index;
    fbe_lba_t capacity;
    fbe_config_generation_t generation_number;
    fbe_lba_t edge_offset;
    fbe_u32_t spaces_to_indent = 0;

    FBE_GET_FIELD_OFFSET(module_name, fbe_ext_pool_lun_configuration_t, "server_index", &offset);
    FBE_READ_MEMORY(ext_pool_lun_object_ptr + offset, &server_index, sizeof(fbe_edge_index_t));

    spaces_to_indent += 8;
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    trace_func(NULL,"server_index:%d", server_index);

    FBE_GET_FIELD_OFFSET(module_name, fbe_ext_pool_lun_configuration_t, "capacity", &offset);
    FBE_READ_MEMORY(ext_pool_lun_object_ptr + offset, &capacity, sizeof(fbe_lba_t));

    spaces_to_indent += 4;
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    trace_func(NULL,"capacity:0x%llx", (unsigned long long)capacity);

    spaces_to_indent += 4;
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    FBE_GET_FIELD_OFFSET(module_name, fbe_ext_pool_lun_configuration_t, "offset", &offset);
    FBE_READ_MEMORY(ext_pool_lun_object_ptr + offset, &edge_offset, sizeof(fbe_lba_t));
    trace_func(NULL,"offset:0x%llx",(unsigned long long)edge_offset);

    trace_func(NULL,"\n");
    spaces_to_indent = 8;
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    FBE_GET_FIELD_OFFSET(module_name, fbe_ext_pool_lun_configuration_t, "generation_number", &offset);
    FBE_READ_MEMORY(ext_pool_lun_object_ptr + offset, &generation_number, sizeof(fbe_config_generation_t));
    trace_func(NULL,"generation_number:0x%llx", (unsigned long long)generation_number);
    
    trace_func(NULL,"\n");
    return FBE_STATUS_OK;
}

fbe_status_t fbe_ext_pool_user_config_debug_trace(const fbe_u8_t * module_name,
                                               fbe_trace_func_t trace_func,
                                               fbe_trace_context_t trace_context,
                                               fbe_dbgext_ptr ext_pool_user_ptr)
{
    fbe_u32_t offset = 0;
    fbe_u32_t spaces_to_indent = 0;
    fbe_u32_t pool_id;

    spaces_to_indent += 8;
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    FBE_GET_FIELD_OFFSET(module_name, database_ext_pool_user_data_t, "pool_id", &offset);
    FBE_READ_MEMORY(ext_pool_user_ptr + offset, &pool_id, sizeof(fbe_u32_t));
    trace_func(NULL,"pool_id:0x%x",pool_id);

    trace_func(NULL,"\n");
    return FBE_STATUS_OK;
}

fbe_status_t fbe_ext_pool_lun_user_config_debug_trace(const fbe_u8_t * module_name,
                                               fbe_trace_func_t trace_func,
                                               fbe_trace_context_t trace_context,
                                               fbe_dbgext_ptr ext_pool_lun_user_ptr)
{
    fbe_u32_t offset = 0;
    fbe_u32_t spaces_to_indent = 0;
    fbe_u32_t pool_id, lun_id;

    spaces_to_indent += 8;
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    FBE_GET_FIELD_OFFSET(module_name, database_ext_pool_lun_user_data_t, "pool_id", &offset);
    FBE_READ_MEMORY(ext_pool_lun_user_ptr + offset, &pool_id, sizeof(fbe_u32_t));
    trace_func(NULL,"pool_id:0x%x",pool_id);

    spaces_to_indent += 4;
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    FBE_GET_FIELD_OFFSET(module_name, database_ext_pool_lun_user_data_t, "lun_id", &offset);
    FBE_READ_MEMORY(ext_pool_lun_user_ptr + offset, &lun_id, sizeof(fbe_u32_t));
    trace_func(NULL,"lun_id:0x%x",lun_id);

    trace_func(NULL,"\n");
    return FBE_STATUS_OK;
}

//end of fbe_database_service_debug.c
