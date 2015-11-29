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

// The following must be included, first, to avoid conflicts with csx_ext.h 
// so as to be able to build in user and kernel modes.
#include "csx_ext_dbgext.h"
#include "string.h"

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
#include "fbe_sepls_debug.h"
#include "fbe_private_space_layout.h"

extern fbe_database_service_t fbe_database_service;
fbe_sep_offsets_t fbe_sep_offsets;

fbe_status_t fbe_database_service_debug_dump_all(const fbe_u8_t * module_name,
                                              fbe_trace_func_t trace_func,
                                              fbe_trace_context_t trace_context)
{

    fbe_dbgext_ptr database_service_ptr = 0;

	FBE_GET_EXPRESSION(module_name, fbe_database_service, &database_service_ptr);
	/* Get the database config service.ptr */
	if(database_service_ptr == 0)
	{
		trace_func(NULL, "database_service_ptr is not available");
		return FBE_STATUS_FAILED;
	}
	fbe_database_service_debug_dump(module_name, trace_func, trace_context, database_service_ptr, FBE_DATABASE_DISPLAY_OBJECT_TABLE, FBE_OBJECT_ID_INVALID);
	fbe_database_service_debug_dump(module_name, trace_func, trace_context, database_service_ptr, FBE_DATABASE_DISPLAY_EDGE_TABLE, FBE_OBJECT_ID_INVALID);
	fbe_database_service_debug_dump(module_name, trace_func, trace_context, database_service_ptr, FBE_DATABASE_DISPLAY_USER_TABLE, FBE_OBJECT_ID_INVALID);
	fbe_database_service_debug_dump(module_name, trace_func, trace_context, database_service_ptr, FBE_DATABASE_DISPLAY_GLOBAL_INFO_TABLE, FBE_OBJECT_ID_INVALID);
	fbe_system_db_header_debug_dump(module_name, trace_func, trace_context, database_service_ptr);
	
    return FBE_STATUS_OK;

}

/*!**************************************************************
 * fbe_database_service_debug_dump()
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
fbe_status_t fbe_database_service_debug_dump(const fbe_u8_t * module_name,
                                              fbe_trace_func_t trace_func,
                                              fbe_trace_context_t trace_context,
                                              fbe_dbgext_ptr database_service_ptr,
                                              fbe_u32_t display_format,
                                              fbe_object_id_t object_id)
{
    fbe_dbgext_ptr object_table_ptr = 0;
    fbe_dbgext_ptr user_table_ptr = 0;
    fbe_dbgext_ptr edge_table_ptr = 0;
    fbe_dbgext_ptr global_info_ptr = 0;
    
    switch(display_format)
    {
        case FBE_DATABASE_DISPLAY_OBJECT_TABLE:
            /* Get the object table pointer */
            object_table_ptr = database_service_ptr + fbe_sep_offsets.db_service_object_table_offset;
            fbe_database_dump_object_table(module_name, trace_func, trace_context, object_table_ptr, object_id);
            break;
        case FBE_DATABASE_DISPLAY_EDGE_TABLE:
            /* Get the user table pointer */
            edge_table_ptr = database_service_ptr + fbe_sep_offsets.db_service_edge_table_offset;
            fbe_database_dump_edge_table(module_name, trace_func, trace_context, edge_table_ptr, object_id);
            break;
        case FBE_DATABASE_DISPLAY_USER_TABLE:
            /* Get the user table pointer */
            user_table_ptr = database_service_ptr + fbe_sep_offsets.db_service_user_table_offset;
            fbe_database_dump_user_table(module_name, trace_func, trace_context, user_table_ptr, object_id);
            break;
        case FBE_DATABASE_DISPLAY_GLOBAL_INFO_TABLE:
            /* Get the global info pointer */
            global_info_ptr = database_service_ptr + fbe_sep_offsets.db_service_global_info_offset;
            fbe_database_dump_global_info_table(module_name, trace_func, trace_context, global_info_ptr, object_id);
            break;
        default:
            trace_func(NULL, "\t Unknown display format\n");
            return FBE_STATUS_INVALID;
    }
    return FBE_STATUS_OK;
}
/*****************************************
 * end fbe_database_service_debug_dump
 *****************************************/
/*!**************************************************************
 * fbe_database_dump_object_table()
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
char db_entry_prefix[200];
fbe_status_t fbe_database_dump_object_table(const fbe_u8_t * module_name,
                                               fbe_trace_func_t trace_func,
                                               fbe_trace_context_t trace_context,
                                               fbe_dbgext_ptr object_table_ptr,
                                               fbe_object_id_t in_object_id)
{
    fbe_dbgext_ptr object_table_content_ptr = 0;
    fbe_dbgext_ptr object_entry_ptr = 0;
    fbe_u32_t database_object_entry_size;
    database_class_id_t class_id;

    fbe_dbgext_ptr fbe_pvd_config_ptr = 0;
    fbe_dbgext_ptr fbe_lun_config_ptr = 0;
    fbe_dbgext_ptr fbe_vd_config_ptr = 0;
    fbe_dbgext_ptr fbe_rg_config_ptr = 0;
    database_config_entry_state_t state;
    fbe_persist_entry_id_t entry_id;
    fbe_object_id_t object_id;
    database_table_size_t table_size;
    fbe_u32_t table_entries;
    fbe_dbgext_ptr debug_ptr = 0;

    FBE_READ_MEMORY(object_table_ptr + fbe_sep_offsets.db_table_size_offset, &table_size, sizeof(database_table_size_t));

    object_table_content_ptr = object_table_ptr + fbe_sep_offsets.db_table_content_offset;
    /* Get the Object entry pointer */
    FBE_READ_MEMORY(object_table_content_ptr, &object_entry_ptr, sizeof(fbe_dbgext_ptr));

    /* Get the size of the database object entry. */
    FBE_GET_TYPE_SIZE(module_name, database_object_entry_t, &database_object_entry_size);

    debug_ptr = object_entry_ptr;
    for(table_entries = 0;table_entries < table_size ; table_entries++)
    {
        sprintf(db_entry_prefix,"object_table[%d]",table_entries);
        FBE_READ_MEMORY(debug_ptr + fbe_sep_offsets.db_obj_entry_object_header_offset+ fbe_sep_offsets.db_table_header_object_id_offset, &object_id, sizeof(fbe_object_id_t));
        if(object_id == in_object_id || in_object_id == FBE_OBJECT_ID_INVALID)
        {

            FBE_READ_MEMORY(debug_ptr + fbe_sep_offsets.db_obj_entry_object_header_offset + fbe_sep_offsets.db_table_header_state_offset, &state, sizeof(database_config_entry_state_t));

            if(state != DATABASE_CONFIG_ENTRY_INVALID)
            {
                trace_func(trace_context,"object_table[%d].header.state:0x%x ;",table_entries,state);
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

                trace_func(NULL,"\n");

                
                FBE_READ_MEMORY(debug_ptr + fbe_sep_offsets.db_obj_entry_object_header_offset + fbe_sep_offsets.db_table_header_entry_id_offset, &entry_id, sizeof(fbe_persist_entry_id_t));
                trace_func(trace_context,"object_table[%d].header.entry_id:0x%llx\n",table_entries,entry_id);

                FBE_READ_MEMORY(debug_ptr + fbe_sep_offsets.db_obj_entry_object_header_offset + fbe_sep_offsets.db_table_header_object_id_offset, &object_id, sizeof(fbe_object_id_t));
                trace_func(trace_context,"object_table[%d].header.object_id:0x%x\n",table_entries,object_id);

                FBE_READ_MEMORY(debug_ptr + fbe_sep_offsets.db_obj_entry_class_id_offset , &class_id, sizeof(database_class_id_t));
                trace_func(trace_context,"object_table[%d].db_class_id:0x%x ;",table_entries,class_id);

                switch(class_id)
                {
                    case DATABASE_CLASS_ID_PROVISION_DRIVE:
                        trace_func(NULL,"DATABASE_CLASS_ID_PROVISION_DRIVE\n");
                        fbe_pvd_config_ptr = debug_ptr + fbe_sep_offsets.db_obj_entry_set_config_union_offset;
                        fbe_pvd_object_config_debug_dump(module_name, trace_func, trace_context,fbe_pvd_config_ptr);
                        break;
                    case DATABASE_CLASS_ID_LUN:
                        trace_func(NULL,"DATABASE_CLASS_ID_LUN\n");
                        fbe_lun_config_ptr = debug_ptr + fbe_sep_offsets.db_obj_entry_set_config_union_offset;
                        fbe_lun_object_config_debug_dump(module_name, trace_func, trace_context,fbe_lun_config_ptr);
                        break;
                    case DATABASE_CLASS_ID_VIRTUAL_DRIVE:
                        trace_func(NULL,"DATABASE_CLASS_ID_VIRTUAL_DRIVE\n");
                        fbe_vd_config_ptr = debug_ptr + fbe_sep_offsets.db_obj_entry_set_config_union_offset;
                        fbe_vd_object_config_debug_dump(module_name, trace_func, trace_context,fbe_vd_config_ptr);
                        break;
                    case DATABASE_CLASS_ID_RAID_START:
                    case DATABASE_CLASS_ID_MIRROR:
                    case DATABASE_CLASS_ID_STRIPER:
                    case DATABASE_CLASS_ID_PARITY:
                    case DATABASE_CLASS_ID_RAID_END:
                        trace_func(NULL,"DATABASE_CLASS_ID_RAID\n");
                        fbe_rg_config_ptr = debug_ptr + fbe_sep_offsets.db_obj_entry_set_config_union_offset;
                        fbe_rg_object_config_debug_dump(module_name, trace_func, trace_context,fbe_rg_config_ptr);
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

    sprintf(db_entry_prefix,"%s","");
    return FBE_STATUS_OK;
}
/*****************************************
 * end fbe_database_dump_object_table
 *****************************************/
/*!**************************************************************
 * fbe_pvd_object_config_debug_dump()
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
fbe_status_t fbe_pvd_object_config_debug_dump(const fbe_u8_t * module_name,
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
    trace_func(NULL,"%s.pvd_config.config_type:0x%x ;",db_entry_prefix,config_type);
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
    trace_func(NULL,"\n");

    FBE_GET_FIELD_OFFSET(module_name, fbe_provision_drive_configuration_t, "configured_capacity", &offset);
    FBE_READ_MEMORY(pvd_object_ptr + offset, &configured_capacity, sizeof(fbe_lba_t));

    trace_func(NULL,"%s.pvd_config.configured_capacity:0x%llx\n",db_entry_prefix,configured_capacity);

    FBE_GET_FIELD_OFFSET(module_name, fbe_provision_drive_configuration_t, "configured_physical_block_size", &offset);
    FBE_READ_MEMORY(pvd_object_ptr + offset, &configured_physical_block_size, sizeof(fbe_provision_drive_configured_physical_block_size_t));

    trace_func(NULL,"%s.pvd_config.configured_physical_block_size:0x%x ;",db_entry_prefix,configured_physical_block_size);

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

    trace_func(NULL,"\n");

    FBE_GET_FIELD_OFFSET(module_name, fbe_provision_drive_configuration_t, "sniff_verify_state", &offset);
    FBE_READ_MEMORY(pvd_object_ptr + offset, &sniff_verify_state, sizeof(fbe_bool_t));
    trace_func(NULL,"%s.pvd_config.sniff_verify_state:0x%x\n",db_entry_prefix,sniff_verify_state);


    FBE_GET_FIELD_OFFSET(module_name, fbe_provision_drive_configuration_t, "generation_number", &offset);
    FBE_READ_MEMORY(pvd_object_ptr + offset, &generation_number, sizeof(fbe_config_generation_t));
    trace_func(NULL,"%s.pvd_config.generation_number:0x%llx\n",db_entry_prefix,generation_number);
    
    FBE_GET_FIELD_OFFSET(module_name, fbe_provision_drive_configuration_t, "serial_num", &offset);
    FBE_READ_MEMORY(pvd_object_ptr + offset, &serial_num, FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE+1);

    if(strlen(serial_num) > 0)
    {
		trace_func(NULL,"%s.pvd_config.serial_num:%s\n",db_entry_prefix,serial_num);
    }
    else
    {
		trace_func(NULL,"%s.pvd_config.serial_num:%s\n",db_entry_prefix,"NULL");
    }

    FBE_GET_FIELD_OFFSET(module_name, fbe_provision_drive_configuration_t, "update_type", &offset);
    FBE_READ_MEMORY(pvd_object_ptr + offset, &update_type, sizeof(fbe_update_pvd_type_t));
	trace_func(NULL,"%s.pvd_config.update_type:0x%x ;",db_entry_prefix,update_type);
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
 * end fbe_pvd_object_config_debug_dump
 *****************************************/

/*!**************************************************************
 * fbe_lun_object_config_debug_dump()
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
fbe_status_t fbe_lun_object_config_debug_dump(const fbe_u8_t * module_name,
                                               fbe_trace_func_t trace_func,
                                               fbe_trace_context_t trace_context,
                                               fbe_dbgext_ptr lun_object_ptr)
{
    fbe_u32_t offset = 0;
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

    FBE_GET_FIELD_OFFSET(module_name, fbe_database_lun_configuration_t, "world_wide_name", &offset);
    FBE_READ_MEMORY(lun_object_ptr + offset , &wwn, sizeof(fbe_assigned_wwid_t));
    FBE_GET_FIELD_OFFSET(module_name, fbe_assigned_wwid_t, "bytes", &char_offset);	
    FBE_READ_MEMORY(lun_object_ptr + offset + char_offset, &world_wide_name, 16);
    if(strlen(world_wide_name) > 0)
    {
		trace_func(NULL,"%s.lu_config.world_wide_name:%s\n",db_entry_prefix,world_wide_name);
    }
    else
    {
		trace_func(NULL,"%s.lu_config.world_wide_name:%s\n",db_entry_prefix,"NULL");
    }

    FBE_GET_FIELD_OFFSET(module_name, fbe_database_lun_configuration_t, "capacity", &offset);
    FBE_READ_MEMORY(lun_object_ptr + offset, &capacity, sizeof(fbe_lba_t));
	trace_func(NULL,"%s.lu_config.capacity:0x%llx\n",db_entry_prefix,capacity);

    FBE_GET_FIELD_OFFSET(module_name, fbe_database_lun_configuration_t, "generation_number", &offset);
    FBE_READ_MEMORY(lun_object_ptr + offset, &generation_number, sizeof(fbe_config_generation_t));
	trace_func(NULL,"%s.lu_config.generation_number:0x%llx\n",db_entry_prefix,generation_number);

    FBE_GET_FIELD_OFFSET(module_name, fbe_database_lun_configuration_t, "power_save_io_drain_delay_in_sec", &offset);
    FBE_READ_MEMORY(lun_object_ptr + offset, &power_save_io_drain_delay_in_sec, sizeof(fbe_u64_t));
	trace_func(NULL,"%s.lu_config.power_save_io_drain_delay_in_sec:0x%llx\n",db_entry_prefix,power_save_io_drain_delay_in_sec);

    FBE_GET_FIELD_OFFSET(module_name, fbe_database_lun_configuration_t, "max_lun_latency_time_is_sec", &offset);
    FBE_READ_MEMORY(lun_object_ptr + offset, &max_lun_latency_time_is_sec, sizeof(fbe_u64_t));
	trace_func(NULL,"%s.lu_config.max_lun_latency_time_is_sec:0x%llx\n",db_entry_prefix,max_lun_latency_time_is_sec);

    FBE_GET_FIELD_OFFSET(module_name, fbe_database_lun_configuration_t, "power_saving_idle_time_in_seconds", &offset);
    FBE_READ_MEMORY(lun_object_ptr + offset, &power_saving_idle_time_in_seconds, sizeof(fbe_u64_t));
	trace_func(NULL,"%s.lu_config.power_saving_idle_time_in_seconds:0x%llx\n",db_entry_prefix,power_saving_idle_time_in_seconds);

    FBE_GET_FIELD_OFFSET(module_name, fbe_database_lun_configuration_t, "power_saving_enabled", &offset);
    FBE_READ_MEMORY(lun_object_ptr + offset, &power_saving_enabled, sizeof(fbe_bool_t));
	trace_func(NULL,"%s.lu_config.power_saving_enabled:0x%x\n",db_entry_prefix,power_saving_enabled);

    FBE_GET_FIELD_OFFSET(module_name, fbe_database_lun_configuration_t, "config_flags", &offset);
    FBE_READ_MEMORY(lun_object_ptr + offset, &config_flags, sizeof(fbe_u32_t));
	trace_func(NULL,"%s.lu_config.config_flags:0x%x\n",db_entry_prefix,config_flags);

    trace_func(NULL,"\n");
    return FBE_STATUS_OK;
}
/*****************************************
 * end fbe_lun_object_config_debug_dump
 *****************************************/

/*!**************************************************************
 * fbe_vd_object_config_debug_dump()
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
fbe_status_t fbe_vd_object_config_debug_dump(const fbe_u8_t * module_name,
                                               fbe_trace_func_t trace_func,
                                               fbe_trace_context_t trace_context,
                                               fbe_dbgext_ptr vd_object_ptr)
{
    fbe_u32_t offset = 0;
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


    FBE_GET_FIELD_OFFSET(module_name, fbe_database_control_vd_set_configuration_t, "width", &offset);
    FBE_READ_MEMORY(vd_object_ptr + offset, &width, sizeof(fbe_u32_t));
	trace_func(NULL,"%s.vd_config.width:0x%x\n",db_entry_prefix,width);

    FBE_GET_FIELD_OFFSET(module_name, fbe_database_control_vd_set_configuration_t, "capacity", &offset);
    FBE_READ_MEMORY(vd_object_ptr + offset, &capacity, sizeof(fbe_lba_t));
	trace_func(NULL,"%s.vd_config.capacity:0x%x\n",db_entry_prefix,(unsigned int)capacity);

    FBE_GET_FIELD_OFFSET(module_name, fbe_database_control_vd_set_configuration_t, "chunk_size", &offset);
    FBE_READ_MEMORY(vd_object_ptr + offset, &chunk_size, sizeof(fbe_chunk_size_t));
	trace_func(NULL,"%s.vd_config.chunk_size:0x%x\n",db_entry_prefix,chunk_size);

    FBE_GET_FIELD_OFFSET(module_name, fbe_database_control_vd_set_configuration_t, "raid_type", &offset);
    FBE_READ_MEMORY(vd_object_ptr + offset, &raid_type, sizeof(fbe_raid_group_type_t));
	trace_func(NULL,"%s.vd_config.raid_type:0x%x ;",db_entry_prefix,raid_type);
	
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
    trace_func(NULL,"\n");

    FBE_GET_FIELD_OFFSET(module_name, fbe_raid_group_configuration_t, "element_size", &offset);
    FBE_READ_MEMORY(vd_object_ptr + offset, &element_size, sizeof(fbe_element_size_t));
	trace_func(NULL,"%s.vd_config.element_size:0x%x\n",db_entry_prefix,element_size);

    FBE_GET_FIELD_OFFSET(module_name, fbe_database_control_vd_set_configuration_t, "elements_per_parity", &offset);
    FBE_READ_MEMORY(vd_object_ptr + offset, &elements_per_parity, sizeof(fbe_element_size_t));
	trace_func(NULL,"%s.vd_config.elements_per_parity:0x%x\n",db_entry_prefix,elements_per_parity);

    FBE_GET_FIELD_OFFSET(module_name, fbe_database_control_vd_set_configuration_t, "debug_flags", &offset);
    FBE_READ_MEMORY(vd_object_ptr + offset, &debug_flags, sizeof(fbe_raid_group_debug_flags_t));
	trace_func(NULL,"%s.vd_config.debug_flags:0x%x ;",db_entry_prefix,debug_flags);

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
    trace_func(NULL,"\n");

    FBE_GET_FIELD_OFFSET(module_name, fbe_database_control_vd_set_configuration_t, "power_saving_idle_time_in_seconds", &offset);
    FBE_READ_MEMORY(vd_object_ptr + offset, &power_saving_idle_time_in_seconds, sizeof(fbe_u64_t));
	trace_func(NULL,"%s.vd_config.power_saving_idle_time_in_seconds:0x%llx\n",db_entry_prefix,power_saving_idle_time_in_seconds);

    FBE_GET_FIELD_OFFSET(module_name, fbe_database_control_vd_set_configuration_t, "power_saving_enabled", &offset);
    FBE_READ_MEMORY(vd_object_ptr + offset, &power_saving_enabled, sizeof(fbe_bool_t));
	trace_func(NULL,"%s.vd_config.power_saving_enabled:0x%x\n",db_entry_prefix,power_saving_enabled);

    FBE_GET_FIELD_OFFSET(module_name, fbe_database_control_vd_set_configuration_t, "max_raid_latency_time_in_sec", &offset);
    FBE_READ_MEMORY(vd_object_ptr + offset, &max_raid_latency_time_in_sec, sizeof(fbe_u64_t));
	trace_func(NULL,"%s.vd_config.max_raid_latency_time_in_sec:0x%llx\n",db_entry_prefix,max_raid_latency_time_in_sec);

    FBE_GET_FIELD_OFFSET(module_name, fbe_database_control_vd_set_configuration_t, "generation_number", &offset);
    FBE_READ_MEMORY(vd_object_ptr + offset, &generation_number, sizeof(fbe_config_generation_t));
	trace_func(NULL,"%s.vd_config.generation_number:0x%llx\n",db_entry_prefix,generation_number);

    FBE_GET_FIELD_OFFSET(module_name, fbe_database_control_vd_set_configuration_t, "update_rg_type", &offset);
    FBE_READ_MEMORY(vd_object_ptr + offset, &update_rg_type, sizeof(fbe_update_raid_type_t));
	trace_func(NULL,"%s.vd_config.update_rg_type:0x%x ;",db_entry_prefix,update_rg_type);

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
    FBE_GET_FIELD_OFFSET(module_name, fbe_database_control_vd_set_configuration_t, "configuration_mode", &offset);
    FBE_READ_MEMORY(vd_object_ptr + offset, &configuration_mode, sizeof(fbe_virtual_drive_configuration_mode_t));
	trace_func(NULL,"%s.vd_config.configuration_mode:0x%x ;",db_entry_prefix,configuration_mode);

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
    trace_func(NULL,"\n");

    FBE_GET_FIELD_OFFSET(module_name, fbe_database_control_vd_set_configuration_t, "update_vd_type", &offset);
    FBE_READ_MEMORY(vd_object_ptr + offset, &vd_update_type, sizeof(fbe_update_vd_type_t));
	trace_func(NULL,"%s.vd_config.update_vd_type:0x%x ;",db_entry_prefix,vd_update_type);

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
 * end fbe_vd_object_config_debug_dump
 *****************************************/

/*!**************************************************************
 * fbe_rg_object_config_debug_dump()
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
fbe_status_t fbe_rg_object_config_debug_dump(const fbe_u8_t * module_name,
                                               fbe_trace_func_t trace_func,
                                               fbe_trace_context_t trace_context,
                                               fbe_dbgext_ptr rg_object_ptr)
{
    fbe_u32_t offset = 0;
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

    FBE_GET_FIELD_OFFSET(module_name, fbe_raid_group_configuration_t, "width", &offset);
    FBE_READ_MEMORY(rg_object_ptr + offset, &width, sizeof(fbe_u32_t));
	trace_func(NULL,"%s.rg_config.width:0x%x\n",db_entry_prefix,width);

    FBE_GET_FIELD_OFFSET(module_name, fbe_raid_group_configuration_t, "capacity", &offset);
    FBE_READ_MEMORY(rg_object_ptr + offset, &capacity, sizeof(fbe_lba_t));
	trace_func(NULL,"%s.rg_config.capacity:0x%llx\n",db_entry_prefix,capacity);

    FBE_GET_FIELD_OFFSET(module_name, fbe_raid_group_configuration_t, "chunk_size", &offset);
    FBE_READ_MEMORY(rg_object_ptr + offset, &chunk_size, sizeof(fbe_chunk_size_t));
	trace_func(NULL,"%s.rg_config.chunk_size:0x%x\n",db_entry_prefix,chunk_size);

    FBE_GET_FIELD_OFFSET(module_name, fbe_raid_group_configuration_t, "raid_type", &offset);
    FBE_READ_MEMORY(rg_object_ptr + offset, &raid_type, sizeof(fbe_raid_group_type_t));
	trace_func(NULL,"%s.rg_config.raid_type:0x%x ;",db_entry_prefix,raid_type);
    
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

    trace_func(NULL,"\n");

    FBE_GET_FIELD_OFFSET(module_name, fbe_raid_group_configuration_t, "element_size", &offset);
    FBE_READ_MEMORY(rg_object_ptr + offset, &element_size, sizeof(fbe_element_size_t));
	trace_func(NULL,"%s.rg_config.element_size:0x%x\n",db_entry_prefix,element_size);

    FBE_GET_FIELD_OFFSET(module_name, fbe_raid_group_configuration_t, "elements_per_parity", &offset);
    FBE_READ_MEMORY(rg_object_ptr + offset, &elements_per_parity, sizeof(fbe_element_size_t));
	trace_func(NULL,"%s.rg_config.elements_per_parity:0x%x\n",db_entry_prefix,elements_per_parity);

    FBE_GET_FIELD_OFFSET(module_name, fbe_raid_group_configuration_t, "debug_flags", &offset);
    FBE_READ_MEMORY(rg_object_ptr + offset, &debug_flags, sizeof(fbe_raid_group_debug_flags_t));
	trace_func(NULL,"%s.rg_config.debug_flags:0x%x ;",db_entry_prefix,debug_flags);

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
    trace_func(NULL,"\n");

    FBE_GET_FIELD_OFFSET(module_name, fbe_raid_group_configuration_t, "power_saving_idle_time_in_seconds", &offset);
    FBE_READ_MEMORY(rg_object_ptr + offset, &power_saving_idle_time_in_seconds, sizeof(fbe_u64_t));
	trace_func(NULL,"%s.rg_config.power_saving_idle_time_in_seconds:0x%llx\n",db_entry_prefix,power_saving_idle_time_in_seconds);

    FBE_GET_FIELD_OFFSET(module_name, fbe_raid_group_configuration_t, "power_saving_enabled", &offset);
    FBE_READ_MEMORY(rg_object_ptr + offset, &power_saving_enabled, sizeof(fbe_bool_t));
	trace_func(NULL,"%s.rg_config.power_saving_enabled:0x%x\n",db_entry_prefix,power_saving_enabled);

    FBE_GET_FIELD_OFFSET(module_name, fbe_raid_group_configuration_t, "max_raid_latency_time_in_sec", &offset);
    FBE_READ_MEMORY(rg_object_ptr + offset, &max_raid_latency_time_in_sec, sizeof(fbe_u64_t));
	trace_func(NULL,"%s.rg_config.max_raid_latency_time_in_sec:0x%llx\n",db_entry_prefix,max_raid_latency_time_in_sec);

    FBE_GET_FIELD_OFFSET(module_name, fbe_raid_group_configuration_t, "generation_number", &offset);
    FBE_READ_MEMORY(rg_object_ptr + offset, &generation_number, sizeof(fbe_config_generation_t));
	trace_func(NULL,"%s.rg_config.generation_number:0x%llx\n",db_entry_prefix,generation_number);

    FBE_GET_FIELD_OFFSET(module_name, fbe_raid_group_configuration_t, "update_type", &offset);
    FBE_READ_MEMORY(rg_object_ptr + offset, &update_type, sizeof(fbe_update_raid_type_t));
	trace_func(NULL,"%s.rg_config.update_type:0x%x ;",db_entry_prefix,update_type);

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
 * end fbe_rg_object_config_debug_dump
 *****************************************/
/*!**************************************************************
 * fbe_database_dump_user_table()
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
fbe_status_t fbe_database_dump_user_table(const fbe_u8_t * module_name,
                                             fbe_trace_func_t trace_func,
                                             fbe_trace_context_t trace_context,
                                             fbe_dbgext_ptr user_table_ptr,
                                             fbe_object_id_t in_object_id)
{
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

    database_table_size_t table_size;
    fbe_u32_t table_entries;
    fbe_dbgext_ptr debug_ptr = 0;

    FBE_READ_MEMORY(user_table_ptr + fbe_sep_offsets.db_table_size_offset, &table_size, sizeof(database_table_size_t));

    user_table_content_ptr = user_table_ptr + fbe_sep_offsets.db_table_content_offset;
    FBE_READ_MEMORY(user_table_content_ptr, &user_entry_ptr, sizeof(fbe_dbgext_ptr));

    /* Get the size of the database object entry. */
    FBE_GET_TYPE_SIZE(module_name, database_user_entry_t, &database_user_entry_size);

    debug_ptr = user_entry_ptr;
    for(table_entries = 0;table_entries < table_size ; table_entries++)
    {
        sprintf(db_entry_prefix,"user_table[%d]",table_entries);

        FBE_READ_MEMORY(debug_ptr + fbe_sep_offsets.db_user_entry_header_offset + fbe_sep_offsets.db_table_header_object_id_offset, &object_id, sizeof(fbe_object_id_t));
        if(object_id == in_object_id || in_object_id == FBE_OBJECT_ID_INVALID)
        {
            FBE_READ_MEMORY(debug_ptr + fbe_sep_offsets.db_user_entry_header_offset + fbe_sep_offsets.db_table_header_state_offset , &state, sizeof(database_config_entry_state_t));

            if(state != DATABASE_CONFIG_ENTRY_INVALID)
            {
		trace_func(NULL,"%s.header.state:0x%x ;",db_entry_prefix,state);
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
                trace_func(NULL,"\n");

                FBE_READ_MEMORY(debug_ptr + fbe_sep_offsets.db_user_entry_header_offset + fbe_sep_offsets.db_table_header_entry_id_offset , &entry_id, sizeof(fbe_persist_entry_id_t));
                trace_func(NULL,"%s.header.entry_id:0x%llx\n",db_entry_prefix,entry_id);

                FBE_READ_MEMORY(debug_ptr + fbe_sep_offsets.db_user_entry_header_offset + fbe_sep_offsets.db_table_header_object_id_offset , &object_id, sizeof(fbe_object_id_t));
                trace_func(NULL,"%s.header.object_id:0x%x\n",db_entry_prefix,object_id);

                FBE_READ_MEMORY(debug_ptr + fbe_sep_offsets.db_user_entry_db_class_id_offset , &class_id, sizeof(database_class_id_t));
                trace_func(NULL,"%s.db_class_id:0x%x ;",db_entry_prefix,class_id);

                switch(class_id)
                {
                    case DATABASE_CLASS_ID_PROVISION_DRIVE:
                        trace_func(NULL,"DATABASE_CLASS_ID_PROVISION_DRIVE\n");
                        fbe_pvd_config_ptr = debug_ptr + fbe_sep_offsets.db_user_entry_user_data_union_offset;
                        fbe_pvd_user_config_debug_dump(module_name, trace_func, trace_context,fbe_pvd_config_ptr);
                        break;
                    case DATABASE_CLASS_ID_LUN:
                        trace_func(NULL,"DATABASE_CLASS_ID_LUN\n");
                        fbe_lun_config_ptr = debug_ptr + fbe_sep_offsets.db_user_entry_user_data_union_offset;
                        fbe_lun_user_config_debug_dump(module_name, trace_func, trace_context,fbe_lun_config_ptr);
                        break;
                    case DATABASE_CLASS_ID_RAID_START:
                    case DATABASE_CLASS_ID_MIRROR:
                    case DATABASE_CLASS_ID_STRIPER:
                    case DATABASE_CLASS_ID_PARITY:
                    case DATABASE_CLASS_ID_RAID_END:
                        trace_func(NULL,"DATABASE_CLASS_ID_RAID\n");
                        fbe_rg_config_ptr = debug_ptr + fbe_sep_offsets.db_user_entry_user_data_union_offset;
                        fbe_rg_user_config_debug_dump(module_name, trace_func, trace_context,fbe_rg_config_ptr);
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
 * end fbe_database_dump_user_table
 ******************************************/

/*!**************************************************************
 * fbe_pvd_user_config_debug_dump()
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
 fbe_status_t fbe_pvd_user_config_debug_dump(const fbe_u8_t * module_name,
                                               fbe_trace_func_t trace_func,
                                               fbe_trace_context_t trace_context,
                                               fbe_dbgext_ptr pvd_user_ptr)
{
    fbe_u32_t offset = 0;
    fbe_u8_t opaque_data[FBE_DATABASE_PROVISION_DRIVE_OPAQUE_DATA_MAX];
    fbe_u32_t pool_id;
    fbe_u32_t index;

    FBE_GET_FIELD_OFFSET(module_name, database_pvd_user_data_t, "opaque_data", &offset);
    FBE_READ_MEMORY(pvd_user_ptr + offset, &opaque_data, FBE_DATABASE_PROVISION_DRIVE_OPAQUE_DATA_MAX);

    if(strlen(opaque_data) > 0)
    {
 		trace_func(NULL,"%s.pvd_user_data.opaque_data:",db_entry_prefix);
        for(index = 0;index < FBE_DATABASE_PROVISION_DRIVE_OPAQUE_DATA_MAX; index++)
        {
            trace_func(NULL,"%x",opaque_data[index]);
        }
    }
    else
    {
 		trace_func(NULL,"%s.pvd_user_data.opaque_data:NULL",db_entry_prefix);
    }
    trace_func(NULL,"\n");

    FBE_GET_FIELD_OFFSET(module_name, database_pvd_user_data_t, "pool_id", &offset);
    FBE_READ_MEMORY(pvd_user_ptr + offset, &pool_id, sizeof(fbe_u32_t));
	trace_func(NULL,"%s.pvd_user_data.pool_id:0x%x\n",db_entry_prefix,pool_id);

    trace_func(NULL,"\n");
    return FBE_STATUS_OK;
}
/*****************************************
 * end fbe_pvd_user_config_debug_dump
 ******************************************/

/*!**************************************************************
 * fbe_lun_user_config_debug_dump()
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
fbe_status_t fbe_lun_user_config_debug_dump(const fbe_u8_t * module_name,
                                             fbe_trace_func_t trace_func,
                                             fbe_trace_context_t trace_context,
                                             fbe_dbgext_ptr lun_user_ptr)
{
    fbe_u32_t offset = 0;
    fbe_bool_t export_device_b;
    fbe_lun_number_t lun_number;
    fbe_assigned_wwid_t wwn;
    fbe_u32_t char_offset = 0;
    fbe_u8_t world_wide_name[16];
    fbe_user_defined_lun_name_t user_defined_name;
    fbe_u8_t name[FBE_USER_DEFINED_LUN_NAME_LEN];
    fbe_u32_t index;
    
    FBE_GET_FIELD_OFFSET(module_name, database_lu_user_data_t, "export_device_b", &offset);
    FBE_READ_MEMORY(lun_user_ptr + offset, &export_device_b, sizeof(fbe_bool_t));
	trace_func(NULL,"%s.lu_user_data.export_device_b:0x%x\n",db_entry_prefix,export_device_b);

    FBE_GET_FIELD_OFFSET(module_name, database_lu_user_data_t, "lun_number", &offset);
    FBE_READ_MEMORY(lun_user_ptr + offset, &lun_number, sizeof(fbe_lun_number_t));
	trace_func(NULL,"%s.lu_user_data.lun_number:0x%x\n",db_entry_prefix,lun_number);

    FBE_GET_FIELD_OFFSET(module_name, database_lu_user_data_t, "world_wide_name", &offset);
    FBE_READ_MEMORY(lun_user_ptr + offset , &wwn, sizeof(fbe_assigned_wwid_t));
    FBE_GET_FIELD_OFFSET(module_name, fbe_assigned_wwid_t, "bytes", &char_offset);	
    FBE_READ_MEMORY(lun_user_ptr + offset + char_offset, &world_wide_name, 16);
	
	trace_func(NULL,"%s.lu_user_data.WWN:",db_entry_prefix);
    if(strlen(world_wide_name) > 0)
    {
        for(index = 0; index < 16; index++)
        {
            if(index == 15)
            {
                trace_func(NULL,"%02x",world_wide_name[index]);
            }
            else
            {
                trace_func(NULL,"%02x:",world_wide_name[index]);
            }
        }
    }
    else
    {
        trace_func(NULL,"NULL");
    }

    trace_func(NULL,"\n");
    FBE_GET_FIELD_OFFSET(module_name, database_lu_user_data_t, "user_defined_name", &offset);
    FBE_READ_MEMORY(lun_user_ptr + offset, &user_defined_name, sizeof(fbe_user_defined_lun_name_t));
    FBE_GET_FIELD_OFFSET(module_name, fbe_user_defined_lun_name_t, "name", &char_offset);	
    FBE_READ_MEMORY(lun_user_ptr + offset + char_offset, &name, FBE_USER_DEFINED_LUN_NAME_LEN);
	
	trace_func(NULL,"%s.lu_user_data.user_defined_name.name:",db_entry_prefix);
    if(strlen(name) > 0)
    {
        trace_func(NULL,"%s",name);
    }
    else
    {
        trace_func(NULL," %s","NULL");
    }

    trace_func(NULL,"\n");
    return FBE_STATUS_OK;
}
/*****************************************
 * end fbe_lun_user_config_debug_dump
 ******************************************/

/*!**************************************************************
 * fbe_rg_user_config_debug_dump()
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
fbe_status_t fbe_rg_user_config_debug_dump(const fbe_u8_t * module_name,
                                            fbe_trace_func_t trace_func,
                                            fbe_trace_context_t trace_context,
                                            fbe_dbgext_ptr rg_user_ptr)
{
    fbe_u32_t offset = 0;
    fbe_bool_t is_system;
    fbe_raid_group_number_t raid_group_number;

    FBE_GET_FIELD_OFFSET(module_name, database_rg_user_data_t, "is_system", &offset);
    FBE_READ_MEMORY(rg_user_ptr + offset, &is_system, sizeof(fbe_bool_t));
	trace_func(NULL,"%s.rg_user_data.is_system:0x%x\n",db_entry_prefix,is_system);

    FBE_GET_FIELD_OFFSET(module_name, database_rg_user_data_t, "raid_group_number", &offset);
    FBE_READ_MEMORY(rg_user_ptr + offset, &raid_group_number, sizeof(fbe_raid_group_number_t));
	trace_func(NULL,"%s.rg_user_data.raid_group_number:0x%x\n",db_entry_prefix,raid_group_number);

    trace_func(NULL,"\n");
    return FBE_STATUS_OK;
}
/*****************************************
 * end fbe_rg_user_config_debug_dump
 ******************************************/

/*!**************************************************************
 * fbe_database_dump_edge_table()
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
fbe_status_t fbe_database_dump_edge_table(const fbe_u8_t * module_name,
                                             fbe_trace_func_t trace_func,
                                             fbe_trace_context_t trace_context,
                                             fbe_dbgext_ptr edge_table_ptr,
                                             fbe_object_id_t in_object_id)
{
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

    FBE_READ_MEMORY(edge_table_ptr + fbe_sep_offsets.db_table_size_offset, &table_size, sizeof(database_table_size_t));

    edge_table_content_ptr = edge_table_ptr + fbe_sep_offsets.db_table_content_offset;
    FBE_READ_MEMORY(edge_table_content_ptr, &edge_entry_ptr, sizeof(fbe_dbgext_ptr));

    /* Get the size of the database object entry. */
    FBE_GET_TYPE_SIZE(module_name, database_edge_entry_t, &database_edge_entry_size);

    debug_ptr = edge_entry_ptr;
    for(table_entries = 0;table_entries < table_size ; table_entries++)
    {
        sprintf(db_entry_prefix,"edge_table[%d]",table_entries);
        
        FBE_READ_MEMORY(debug_ptr + fbe_sep_offsets.db_edge_entry_header_offset + fbe_sep_offsets.db_table_header_object_id_offset, &object_id, sizeof(fbe_object_id_t));
        if(object_id == in_object_id || in_object_id == FBE_OBJECT_ID_INVALID)
        {
            FBE_READ_MEMORY(debug_ptr + fbe_sep_offsets.db_edge_entry_header_offset + fbe_sep_offsets.db_table_header_state_offset , &state, sizeof(database_config_entry_state_t));
            
            if(state != DATABASE_CONFIG_ENTRY_INVALID)
            {
                trace_func(NULL,"%s.header.state:0x%x ;",db_entry_prefix,state);
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
                trace_func(NULL,"\n");
                FBE_READ_MEMORY(debug_ptr + fbe_sep_offsets.db_edge_entry_header_offset + fbe_sep_offsets.db_table_header_entry_id_offset, &entry_id, sizeof(fbe_persist_entry_id_t));
                trace_func(NULL,"%s.header.entry_id:0x%llx\n",db_entry_prefix,entry_id);

                FBE_READ_MEMORY(debug_ptr + fbe_sep_offsets.db_edge_entry_header_offset + fbe_sep_offsets.db_table_header_object_id_offset, &object_id, sizeof(fbe_object_id_t));
                trace_func(NULL,"%s.header.object_id:0x%x\n",db_entry_prefix,object_id);

                FBE_READ_MEMORY(debug_ptr + fbe_sep_offsets.db_edge_entry_server_id_offset, &server_id, sizeof(fbe_object_id_t));
                trace_func(NULL,"%s.server_id:0x%x\n",db_entry_prefix,server_id);

                FBE_READ_MEMORY(debug_ptr + fbe_sep_offsets.db_edge_entry_client_index_offset, &client_index, sizeof(fbe_edge_index_t));
                trace_func(NULL,"%s.client_index:0x%x\n",db_entry_prefix,client_index);

                FBE_READ_MEMORY(debug_ptr + fbe_sep_offsets.db_edge_entry_capacity_offset, &capacity, sizeof(fbe_lba_t));
                trace_func(NULL,"%s.capacity:0x%x\n",db_entry_prefix,(unsigned int)capacity);

                FBE_READ_MEMORY(debug_ptr + fbe_sep_offsets.db_edge_entry_offset_offset, &edge_offset, sizeof(fbe_lba_t));
                trace_func(NULL,"%s.offset:0x%x\n",db_entry_prefix,fbe_sep_offsets.db_edge_entry_offset_offset);

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
 * end fbe_database_dump_edge_table
 ******************************************/

/*!**************************************************************
 * fbe_database_dump_global_info_table()
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
fbe_status_t fbe_database_dump_global_info_table(const fbe_u8_t * module_name,
                                                    fbe_trace_func_t trace_func,
                                                    fbe_trace_context_t trace_context,
                                                    fbe_dbgext_ptr global_info_ptr,
                                                    fbe_object_id_t in_object_id)
{
    fbe_u32_t header_offset = 0;
    fbe_u32_t db_global_info_entry_type_offset = 0;
    fbe_u32_t db_global_info_entry_info_union_offset = 0;
    database_table_size_t table_size;
    fbe_u32_t table_entries;
    fbe_dbgext_ptr debug_ptr = 0;
    fbe_dbgext_ptr global_entry_ptr = 0;
    fbe_dbgext_ptr global_info_union_ptr = 0;
    fbe_dbgext_ptr power_saving_info_ptr = 0;
    fbe_dbgext_ptr spare_config_info_ptr = 0;
    fbe_dbgext_ptr system_generation_info_ptr = 0;
    fbe_dbgext_ptr system_time_threshold_info_ptr = 0;
    database_global_info_type_t type;
    fbe_u32_t database_global_entry_size;
    database_config_entry_state_t state;
    fbe_persist_entry_id_t entry_id;
    fbe_object_id_t object_id;

    database_table_t global_table = {0};
    database_global_info_entry_t debug_global = {0};
    
    FBE_READ_MEMORY(global_info_ptr , &global_table, sizeof(database_table_t));

    FBE_READ_MEMORY(global_info_ptr + fbe_sep_offsets.db_table_size_offset, &table_size, sizeof(database_table_size_t));

    global_info_union_ptr = global_info_ptr + fbe_sep_offsets.db_table_content_offset;
    FBE_READ_MEMORY(global_info_union_ptr, &global_entry_ptr, sizeof(fbe_dbgext_ptr));

    /* Get the size of the database object entry. */
    FBE_GET_TYPE_SIZE(module_name, database_global_info_entry_t, &database_global_entry_size);
    FBE_GET_FIELD_OFFSET(module_name,database_global_info_entry_t,"header",&header_offset); 
    FBE_GET_FIELD_OFFSET(module_name,database_global_info_entry_t,"type",&db_global_info_entry_type_offset); 
    FBE_GET_FIELD_OFFSET(module_name,database_global_info_entry_t,"info_union",&db_global_info_entry_info_union_offset);
    
    debug_ptr = global_entry_ptr;

    for(table_entries = 0;table_entries < table_size ; table_entries++)
    {
    
        sprintf(db_entry_prefix,"global_info[%d]",table_entries);
        FBE_READ_MEMORY(debug_ptr , &debug_global, sizeof(database_global_info_entry_t));
        FBE_READ_MEMORY(debug_ptr + header_offset + fbe_sep_offsets.db_table_header_object_id_offset, &object_id, sizeof(fbe_object_id_t));

        if(object_id == in_object_id || in_object_id == FBE_OBJECT_ID_INVALID)
        {
            FBE_READ_MEMORY(debug_ptr + header_offset + fbe_sep_offsets.db_table_header_state_offset , &state, sizeof(database_config_entry_state_t));

            if(state != DATABASE_CONFIG_ENTRY_INVALID)
            {
                trace_func(NULL,"%s.header.state:0x%x ;",db_entry_prefix,state);

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
                trace_func(NULL,"\n");

                FBE_READ_MEMORY(debug_ptr + header_offset + fbe_sep_offsets.db_table_header_entry_id_offset, &entry_id, sizeof(fbe_persist_entry_id_t));
                trace_func(NULL,"%s.header.entry_id:0x%llx\n",db_entry_prefix,entry_id);

                FBE_READ_MEMORY(debug_ptr + header_offset + fbe_sep_offsets.db_table_header_object_id_offset, &object_id, sizeof(fbe_object_id_t));
                trace_func(NULL,"%s.header.object_id:0x%x\n",db_entry_prefix,object_id);

                
                FBE_READ_MEMORY(debug_ptr + db_global_info_entry_type_offset , &type, sizeof(database_global_info_type_t));
                trace_func(NULL,"%s.type:0x%x ;",db_entry_prefix,type);

                

                switch(type)
                {
                    case DATABASE_GLOBAL_INFO_TYPE_SYSTEM_POWER_SAVE:
                        trace_func(NULL,"DATABASE_GLOBAL_INFO_TYPE_SYSTEM_POWER_SAVE\n");
                        power_saving_info_ptr = debug_ptr + db_global_info_entry_info_union_offset;
                        fbe_global_info_power_saving_debug_dump(module_name, trace_func, trace_context,power_saving_info_ptr);
                        break;
                    case DATABASE_GLOBAL_INFO_TYPE_SYSTEM_SPARE:
                        trace_func(NULL,"DATABASE_GLOBAL_INFO_TYPE_SYSTEM_SPARE\n");
                        spare_config_info_ptr = debug_ptr + db_global_info_entry_info_union_offset;
                        fbe_global_info_spare_config_debug_dump(module_name, trace_func, trace_context,spare_config_info_ptr);
                        break;
                    case DATABASE_GLOBAL_INFO_TYPE_SYSTEM_GENERATION:
                        trace_func(NULL,"DATABASE_GLOBAL_INFO_TYPE_SYSTEM_GENERATION\n");
                        system_generation_info_ptr = debug_ptr + db_global_info_entry_info_union_offset;
                        fbe_global_info_system_generation_debug_dump(module_name, trace_func, trace_context,system_generation_info_ptr);
                        break;
                    case DATABASE_GLOBAL_INFO_TYPE_SYSTEM_TIME_THRESHOLD:
                        trace_func(NULL,"DATABASE_GLOBAL_INFO_TYPE_SYSTEM_TIME_THRESHOLD\n");
                        system_time_threshold_info_ptr = debug_ptr + db_global_info_entry_info_union_offset;
                        fbe_global_info_system_time_threshold_debug_dump(module_name, trace_func, trace_context,system_time_threshold_info_ptr);
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
 * fbe_status_t fbe_global_info_power_saving_debug_dump()
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
fbe_status_t fbe_global_info_power_saving_debug_dump(const fbe_u8_t * module_name,
                                                     fbe_trace_func_t trace_func,
                                                     fbe_trace_context_t trace_context,
                                                     fbe_dbgext_ptr power_saving_info_ptr)
{
    fbe_u32_t offset = 0;	
    fbe_bool_t	enabled;
    fbe_u64_t	hibernation_wake_up_time_in_minutes;

    FBE_GET_FIELD_OFFSET(module_name,fbe_system_power_saving_info_t,"enabled",&offset);
    FBE_READ_MEMORY(power_saving_info_ptr + offset , &enabled, sizeof(fbe_bool_t));
	trace_func(NULL,"%s.power_saving_info.enabled:0x%x\n",db_entry_prefix,enabled);

    FBE_GET_FIELD_OFFSET(module_name,fbe_system_power_saving_info_t,"hibernation_wake_up_time_in_minutes",&offset);
    FBE_READ_MEMORY(power_saving_info_ptr + offset , &hibernation_wake_up_time_in_minutes, sizeof(fbe_u64_t));
	trace_func(NULL,"%s.power_saving_info.hibernation_wake_up_time_in_minutes:0x%llx\n",db_entry_prefix,hibernation_wake_up_time_in_minutes);

    return FBE_STATUS_OK;
}
/*****************************************
 * end fbe_global_info_power_saving_debug_dump
 ******************************************/

/*!**************************************************************
 * fbe_status_t fbe_global_info_spare_config_debug_dump()
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
fbe_status_t fbe_global_info_spare_config_debug_dump(const fbe_u8_t * module_name,
                                                     fbe_trace_func_t trace_func,
                                                     fbe_trace_context_t trace_context,
                                                     fbe_dbgext_ptr spare_config_info_ptr)
{
    fbe_u32_t offset = 0;	
    fbe_u64_t permanent_spare_trigger_time;

    FBE_GET_FIELD_OFFSET(module_name,fbe_system_spare_config_info_t,"permanent_spare_trigger_time",&offset);
    FBE_READ_MEMORY(spare_config_info_ptr + offset , &permanent_spare_trigger_time, sizeof(fbe_u64_t));
	trace_func(NULL,"%s.system_spare_config_info.permanent_spare_trigger_time:0x%llx\n",db_entry_prefix,permanent_spare_trigger_time);

    return FBE_STATUS_OK;
}
/*****************************************
 * end fbe_global_info_spare_config_debug_dump
 ******************************************/
    
/*!**************************************************************
 * fbe_status_t fbe_global_info_system_generation_debug_dump()
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
fbe_status_t fbe_global_info_system_generation_debug_dump(const fbe_u8_t * module_name,
                                                           fbe_trace_func_t trace_func,
                                                           fbe_trace_context_t trace_context,
                                                           fbe_dbgext_ptr system_generation_info_ptr)
{
    fbe_u32_t offset = 0;	
    fbe_config_generation_t current_generation_number;

    FBE_GET_FIELD_OFFSET(module_name,fbe_system_generation_info_t,"current_generation_number",&offset);
    FBE_READ_MEMORY(system_generation_info_ptr + offset , &current_generation_number, sizeof(fbe_config_generation_t));
	trace_func(NULL,"%s.system_generation_info.current_generation_number:0x%llx\n",db_entry_prefix,current_generation_number);

    trace_func(NULL,"\n");
    return FBE_STATUS_OK;
}
/*****************************************
 * end fbe_global_info_system_generation_debug_dump
 ******************************************/

/*!**************************************************************
 * fbe_status_t fbe_global_info_system_time_threshold_debug_dump()
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
fbe_status_t fbe_global_info_system_time_threshold_debug_dump(const fbe_u8_t * module_name,
                                                           fbe_trace_func_t trace_func,
                                                           fbe_trace_context_t trace_context,
                                                           fbe_dbgext_ptr system_time_threshold_info_ptr)
{
    fbe_u32_t offset = 0;	
    fbe_u64_t time_threshold_in_minutes;

    FBE_GET_FIELD_OFFSET(module_name,fbe_system_time_threshold_info_t,"time_threshold_in_minutes",&offset);
    FBE_READ_MEMORY(system_time_threshold_info_ptr + offset , &time_threshold_in_minutes, sizeof(fbe_u64_t));
	trace_func(NULL,"%s.system_time_threshold_info.time_threshold_in_minutes:0x%llx\n",db_entry_prefix,time_threshold_in_minutes);

    trace_func(NULL,"\n");
    return FBE_STATUS_OK;
}
/*****************************************
 * end fbe_global_info_system_time_threshold_debug_dump
 ******************************************/

/*****************************************
 * end fbe_global_info_system_generation_debug_dump
 ******************************************/

/*!**************************************************************
 * fbe_status_t fbe_system_db_header_debug_dump()
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
fbe_status_t fbe_system_db_header_debug_dump(const fbe_u8_t * module_name,
                                                           fbe_trace_func_t trace_func,
                                                           fbe_trace_context_t trace_context,
                                                           fbe_dbgext_ptr database_service_ptr)
{

	fbe_dbgext_ptr system_db_header_ptr = 0;

    fbe_u32_t offset = 0;	
    fbe_u64_t magic_number = 0;            /* default value: DATABASE_SYSTEM_DB_HEADER_MAGIC_NUMBER */
    fbe_sep_version_header_t    version_header;     /*used to record the size of system db header*/
    fbe_u64_t persisted_sep_version =0;                /* revision number: 1 */
    
    /* persist the size for object entry */
    fbe_u32_t bvd_interface_object_entry_size =0;
    fbe_u32_t pvd_object_entry_size =0;
    fbe_u32_t vd_object_entry_size =0;
    fbe_u32_t rg_object_entry_size =0;

    /* persist the size for user entry */
    fbe_u32_t lun_user_entry_size =0;

    /* persist the size for global info entry */
    fbe_u32_t power_save_info_global_info_entry_size =0;
    fbe_u32_t spare_info_global_info_entry_size =0;
    fbe_u32_t generation_info_global_info_entry_size =0;
    fbe_u32_t time_threshold_info_global_info_entry_size =0;

    /* persist the nonpaged metadata size */
    fbe_u32_t pvd_nonpaged_metadata_size =0;
    fbe_u32_t lun_nonpaged_metadata_size =0;
    fbe_u32_t raid_nonpaged_metadata_size =0;

	const char*prefix="db";

    FBE_GET_FIELD_OFFSET(module_name,fbe_database_service_t,"system_db_header",&offset);
	system_db_header_ptr = database_service_ptr + offset;
    FBE_GET_FIELD_OFFSET(module_name,database_system_db_header_t,"magic_number",&offset);
    FBE_READ_MEMORY(system_db_header_ptr + offset , &magic_number, sizeof(fbe_u64_t));
	trace_func(NULL,"%s.system_db_header.magic_number:0x%llx\n",prefix,magic_number);

    FBE_GET_FIELD_OFFSET(module_name,database_system_db_header_t,"version_header",&offset);
    FBE_READ_MEMORY(system_db_header_ptr + offset , &version_header, sizeof(fbe_sep_version_header_t));
	trace_func(NULL,"%s.system_db_header.magic_number.version_header.size:0x%x\n",prefix,version_header.size);
	trace_func(NULL,"%s.system_db_header.magic_number.version_header.padded_reserve_space[0]:0x%x\n",prefix,version_header.padded_reserve_space[0]);
	trace_func(NULL,"%s.system_db_header.magic_number.version_header.padded_reserve_space[1]:0x%x\n",prefix,version_header.padded_reserve_space[0]);
	trace_func(NULL,"%s.system_db_header.magic_number.version_header.padded_reserve_space[2]:0x%x\n",prefix,version_header.padded_reserve_space[0]);


    FBE_GET_FIELD_OFFSET(module_name,database_system_db_header_t,"persisted_sep_version",&offset);
    FBE_READ_MEMORY(system_db_header_ptr + offset , &persisted_sep_version, sizeof(persisted_sep_version));
	trace_func(NULL,"%s.system_db_header.persisted_sep_version:0x%llx\n",prefix,persisted_sep_version);

    FBE_GET_FIELD_OFFSET(module_name,database_system_db_header_t,"bvd_interface_object_entry_size",&offset);
    FBE_READ_MEMORY(system_db_header_ptr + offset , &bvd_interface_object_entry_size, sizeof(bvd_interface_object_entry_size));
	trace_func(NULL,"%s.system_db_header.bvd_interface_object_entry_size:0x%x\n",prefix,bvd_interface_object_entry_size);

    FBE_GET_FIELD_OFFSET(module_name,database_system_db_header_t,"pvd_object_entry_size",&offset);
    FBE_READ_MEMORY(system_db_header_ptr + offset , &pvd_object_entry_size, sizeof(pvd_object_entry_size));
	trace_func(NULL,"%s.system_db_header.pvd_object_entry_size:0x%x\n",prefix,pvd_object_entry_size);

    FBE_GET_FIELD_OFFSET(module_name,database_system_db_header_t,"vd_object_entry_size",&offset);
    FBE_READ_MEMORY(system_db_header_ptr + offset , &vd_object_entry_size, sizeof(vd_object_entry_size));
	trace_func(NULL,"%s.system_db_header.vd_object_entry_size:0x%x\n",prefix,vd_object_entry_size);

    FBE_GET_FIELD_OFFSET(module_name,database_system_db_header_t,"rg_object_entry_size",&offset);
    FBE_READ_MEMORY(system_db_header_ptr + offset , &rg_object_entry_size, sizeof(rg_object_entry_size));
	trace_func(NULL,"%s.system_db_header.rg_object_entry_size:0x%x\n",prefix,rg_object_entry_size);

    FBE_GET_FIELD_OFFSET(module_name,database_system_db_header_t,"lun_user_entry_size",&offset);
    FBE_READ_MEMORY(system_db_header_ptr + offset , &lun_user_entry_size, sizeof(lun_user_entry_size));
	trace_func(NULL,"%s.system_db_header.lun_user_entry_size:0x%x\n",prefix,lun_user_entry_size);

    FBE_GET_FIELD_OFFSET(module_name,database_system_db_header_t,"power_save_info_global_info_entry_size",&offset);
    FBE_READ_MEMORY(system_db_header_ptr + offset , &power_save_info_global_info_entry_size, sizeof(power_save_info_global_info_entry_size));
	trace_func(NULL,"%s.system_db_header.power_save_info_global_info_entry_size:0x%x\n",prefix,power_save_info_global_info_entry_size);

    FBE_GET_FIELD_OFFSET(module_name,database_system_db_header_t,"spare_info_global_info_entry_size",&offset);
    FBE_READ_MEMORY(system_db_header_ptr + offset , &spare_info_global_info_entry_size, sizeof(spare_info_global_info_entry_size));
	trace_func(NULL,"%s.system_db_header.spare_info_global_info_entry_size:0x%x\n",prefix,spare_info_global_info_entry_size);

    FBE_GET_FIELD_OFFSET(module_name,database_system_db_header_t,"generation_info_global_info_entry_size",&offset);
    FBE_READ_MEMORY(system_db_header_ptr + offset , &generation_info_global_info_entry_size, sizeof(generation_info_global_info_entry_size));
	trace_func(NULL,"%s.system_db_header.generation_info_global_info_entry_size:0x%x\n",prefix,generation_info_global_info_entry_size);

    FBE_GET_FIELD_OFFSET(module_name,database_system_db_header_t,"time_threshold_info_global_info_entry_size",&offset);
    FBE_READ_MEMORY(system_db_header_ptr + offset , &time_threshold_info_global_info_entry_size, sizeof(time_threshold_info_global_info_entry_size));
	trace_func(NULL,"%s.system_db_header.time_threshold_info_global_info_entry_size:0x%x\n",prefix,time_threshold_info_global_info_entry_size);

    FBE_GET_FIELD_OFFSET(module_name,database_system_db_header_t,"pvd_nonpaged_metadata_size",&offset);
    FBE_READ_MEMORY(system_db_header_ptr + offset , &pvd_nonpaged_metadata_size, sizeof(pvd_nonpaged_metadata_size));
	trace_func(NULL,"%s.system_db_header.pvd_nonpaged_metadata_size:0x%x\n",prefix,pvd_nonpaged_metadata_size);

    FBE_GET_FIELD_OFFSET(module_name,database_system_db_header_t,"lun_nonpaged_metadata_size",&offset);
    FBE_READ_MEMORY(system_db_header_ptr + offset , &lun_nonpaged_metadata_size, sizeof(lun_nonpaged_metadata_size));
	trace_func(NULL,"%s.system_db_header.lun_nonpaged_metadata_size:0x%x\n",prefix,lun_nonpaged_metadata_size);

    FBE_GET_FIELD_OFFSET(module_name,database_system_db_header_t,"raid_nonpaged_metadata_size",&offset);
    FBE_READ_MEMORY(system_db_header_ptr + offset , &raid_nonpaged_metadata_size, sizeof(raid_nonpaged_metadata_size));
	trace_func(NULL,"%s.system_db_header.raid_nonpaged_metadata_size:0x%x\n",prefix,raid_nonpaged_metadata_size);

    FBE_GET_FIELD_OFFSET(module_name,database_system_db_header_t,"time_threshold_info_global_info_entry_size",&offset);
    FBE_READ_MEMORY(system_db_header_ptr + offset , &time_threshold_info_global_info_entry_size, sizeof(time_threshold_info_global_info_entry_size));
	trace_func(NULL,"%s.system_db_header.time_threshold_info_global_info_entry_size:0x%x\n",prefix,time_threshold_info_global_info_entry_size);

    trace_func(NULL,"\n");
    return FBE_STATUS_OK;
}

fbe_char_t *
fbe_database_get_lun_type_string(fbe_u32_t type)
{
    switch (type) 
    {
    case FBE_RAID_GROUP_TYPE_UNKNOWN:
        return "unknown raid group type";
    case FBE_RAID_GROUP_TYPE_RAID1:
        return "raid1";
    case FBE_RAID_GROUP_TYPE_RAID10:
        return "raid10";
    case FBE_RAID_GROUP_TYPE_RAID3:
        return "raid3";
    case FBE_RAID_GROUP_TYPE_RAID0:
        return "raid0";
    case FBE_RAID_GROUP_TYPE_RAID5:
        return "raid5";
    case FBE_RAID_GROUP_TYPE_RAID6:
        return "raid6";
    case FBE_RAID_GROUP_TYPE_SPARE:
        return "spare";
    case FBE_RAID_GROUP_TYPE_INTERNAL_MIRROR_UNDER_STRIPER:
        return "internal mirror under striper";
    case FBE_RAID_GROUP_TYPE_INTERNAL_METADATA_MIRROR:
        return "internal metadata mirror";
    case FBE_RAID_GROUP_TYPE_RAW_MIRROR:
        return "raw mirror";
    default:
        return "unknow raid group type";
    }
}



/*!**************************************************************
 * void fbe_database_get_lun_config_debug()
 ****************************************************************
 * @brief
 *  Get the wwn, user defined name 
 *
 * @param lun_obj_id - user lun object id  
 * @param lun_number(Input) - the lun number
 * @param *wwn(output) - the WWN of this LUN.
 * @param *type (output) - In MCR, we can't get the raid type of the lun. 
 *                                     The return value is invalid.
 * @param *name(output) - the user defined name of the LUN.
 * @param *is_exist(output) - whether this lun exists.
 * @param *status (output) - the status
 * 
 * 
 * @author
 *  10/11/2012:  Created. gaoh1
 *  11/19/2013:  Modified. Yingying Song
 *
 ****************************************************************/
void fbe_database_get_lun_config_debug(fbe_object_id_t lun_obj_id,
					fbe_lun_number_t *lun_number,
                                        fbe_assigned_wwid_t *wwn,
                                        fbe_u32_t *type,
                                        fbe_user_defined_lun_name_t *name,
                                        fbe_bool_t *status,
                                        fbe_bool_t *is_exists)
{
    fbe_dbgext_ptr database_service_ptr = 0;
    fbe_dbgext_ptr table_ptr = 0;
    fbe_dbgext_ptr table_content_ptr = 0;
    fbe_dbgext_ptr entry_ptr = 0;
    fbe_dbgext_ptr debug_ptr = 0;
    fbe_u32_t       database_user_entry_size;
    fbe_u32_t       database_edge_entry_size;
    fbe_u32_t       database_object_entry_size;
    fbe_u32_t       table_entries;
    fbe_u32_t       offset;
    fbe_u32_t       db_class_id_offset;
    fbe_u32_t       user_data_union_offset;
    fbe_u32_t       table_size;
    fbe_u32_t       lun_number_offset;
    fbe_u32_t       wwn_offset;
    fbe_u32_t       user_defined_name_offset;
    fbe_u32_t       raid_type_offset;
    database_class_id_t db_class_id;
    fbe_lun_number_t    lun_number_in_table;
    fbe_u32_t   lun_obj_id_offset;
    fbe_object_id_t raid_obj_id;
   
    /* Set the initial value */
    *status = FBE_FALSE;
    *is_exists = FBE_FALSE;
    *type = 0;  /* We don't have the lun type in MCR (the raid type of the lun)*/

   
    FBE_GET_EXPRESSION("sep", fbe_database_service, &database_service_ptr);
    /* Get the database config service.ptr */
    if(database_service_ptr == 0)
    {
        return;
    }


    /* Get the user table address */
    FBE_GET_FIELD_OFFSET("sep", fbe_database_service_t, "user_table", &offset);
    table_ptr = database_service_ptr + offset;
 
    FBE_GET_FIELD_OFFSET("sep",database_table_t,"table_size",&offset);
    FBE_READ_MEMORY(table_ptr + offset, &table_size, sizeof(database_table_size_t));
 
    FBE_GET_FIELD_OFFSET("sep",database_table_t,"table_content",&offset);
    table_content_ptr = table_ptr + offset;

    FBE_READ_MEMORY(table_content_ptr, &entry_ptr, sizeof(fbe_dbgext_ptr));

    /* Get the size of the database object entry. */
    FBE_GET_TYPE_SIZE("sep", database_user_entry_t, &database_user_entry_size);

     /* Get the following offsets *once* */
    FBE_GET_FIELD_OFFSET("sep", database_user_entry_t, "db_class_id", &db_class_id_offset);
    FBE_GET_FIELD_OFFSET("sep",database_user_entry_t,"user_data_union", &user_data_union_offset);
    FBE_GET_FIELD_OFFSET("sep", database_lu_user_data_t, "lun_number", &lun_number_offset);


    debug_ptr = entry_ptr ;

    if(lun_obj_id != FBE_OBJECT_ID_INVALID) {
		  debug_ptr +=  database_user_entry_size*lun_obj_id;
    }
    for(table_entries = 0;table_entries < table_size ; table_entries++) {
    
      if(lun_obj_id == FBE_OBJECT_ID_INVALID) {
        FBE_READ_MEMORY(debug_ptr + db_class_id_offset, &db_class_id, sizeof(database_class_id_t));
        /* Only process the LUN */
        if (db_class_id != DATABASE_CLASS_ID_LUN) {
            debug_ptr += database_user_entry_size;
            continue;
        }
	FBE_READ_MEMORY(debug_ptr + user_data_union_offset + lun_number_offset, &lun_number_in_table, sizeof(fbe_lun_number_t));
    
      }
    else {
        FBE_READ_MEMORY(debug_ptr + user_data_union_offset + lun_number_offset, lun_number, sizeof(fbe_lun_number_t));
    }
    
    if(lun_obj_id == FBE_OBJECT_ID_INVALID) {
      /* It is not the lun we want */
        if (lun_number_in_table != *lun_number) {
            debug_ptr += database_user_entry_size;
            continue;
        }
    }
    
    /*If we find out the information of the LUN, return the value.*/
    FBE_GET_FIELD_OFFSET("sep", database_lu_user_data_t, "world_wide_name", &wwn_offset);
    FBE_GET_FIELD_OFFSET("sep", database_lu_user_data_t, "user_defined_name", &user_defined_name_offset);
    FBE_READ_MEMORY(debug_ptr + user_data_union_offset + wwn_offset, wwn, sizeof(fbe_assigned_wwid_t));
        FBE_READ_MEMORY(debug_ptr + user_data_union_offset + user_defined_name_offset, name, sizeof(fbe_user_defined_lun_name_t));
    *status = FBE_TRUE;
    *is_exists = FBE_TRUE;
    
    /* Get the Raid type of the LUN */
    /*first, get the lun object id from the user entry header*/
    FBE_GET_FIELD_OFFSET("sep", database_user_entry_t, "header", &offset);
    FBE_GET_FIELD_OFFSET("sep", database_table_header_t, "object_id", &lun_obj_id_offset);
    FBE_READ_MEMORY(debug_ptr + offset + lun_obj_id_offset, &lun_obj_id, sizeof(fbe_object_id_t));

    /* then, get the edge entry by lun object id, then get the raid object id from the edge entry */
    FBE_GET_FIELD_OFFSET("sep", fbe_database_service_t, "edge_table", &offset);
    /* edge table ptr */
    table_ptr = database_service_ptr + offset;
    
    FBE_GET_FIELD_OFFSET("sep",database_table_t,"table_content",&offset);
    table_content_ptr = table_ptr + offset;
    /* edge entry array */
    FBE_READ_MEMORY(table_content_ptr, &entry_ptr, sizeof(fbe_dbgext_ptr));
    
    FBE_GET_FIELD_OFFSET("sep", database_edge_entry_t, "server_id", &offset);
    FBE_GET_TYPE_SIZE("sep", database_edge_entry_t, &database_edge_entry_size);
    FBE_READ_MEMORY(entry_ptr + lun_obj_id * DATABASE_MAX_EDGE_PER_OBJECT * database_edge_entry_size + offset,
		    &raid_obj_id, sizeof(fbe_object_id_t));
    
    /* next, we get the object entry by raid object id */
    FBE_GET_FIELD_OFFSET("sep", fbe_database_service_t, "object_table", &offset);
    table_ptr = database_service_ptr + offset;
    
    FBE_GET_FIELD_OFFSET("sep",database_table_t,"table_content",&offset);
    table_content_ptr = table_ptr + offset;
    /* get the address of object entry array */
    FBE_READ_MEMORY(table_content_ptr, &entry_ptr, sizeof(fbe_dbgext_ptr));
    
    /* entry_ptr + sizeof(database_object_entry_t) * raid_obj_id  --> database_object_entry */
    FBE_GET_FIELD_OFFSET("sep", database_object_entry_t, "set_config_union", &offset);
    FBE_GET_FIELD_OFFSET("sep", fbe_raid_group_configuration_t, "raid_type", &raid_type_offset);
    FBE_GET_TYPE_SIZE("sep", database_object_entry_t, &database_object_entry_size);
    FBE_READ_MEMORY(entry_ptr + raid_obj_id * database_object_entry_size + offset + raid_type_offset, type, sizeof(fbe_raid_group_type_t));
    
    break;
    }

    return;
}


/*!**************************************************************
 * fbe_u32_t fbe_database_get_total_object_count()
 ****************************************************************
 * @brief
 *  return the max_object_count in system. 
 *  The object_id in system should be 0 to returned max_object_count-1. 
 *
 * @return  fbe_u32_t: max_object_count in system.
 *
 * @author
 *  11/20/2013:  Created. Yingying Song
 ****************************************************************/
fbe_u32_t fbe_database_get_total_object_count(void)
{
    fbe_u32_t max_object_count = 0;
    ULONG64 max_topology_entries_p = 0;

    FBE_GET_EXPRESSION("sep", max_supported_objects, &max_topology_entries_p);
    FBE_READ_MEMORY(max_topology_entries_p, &max_object_count, sizeof(fbe_u32_t));

    return max_object_count;
}/* End fbe_database_get_total_object_count() */



/*!**************************************************************
 * fbe_bool_t fbe_database_is_object_id_lun()
 ****************************************************************
 * @brief
 *  Determine whether the given the object_id is a LUN or not. 
 *
 * @param  object_id - the given object_id.
 *
 * @return fbe_bool_t - If it is a LUN or not.
 *
 * @author
 *  11/20/2013:  Created. Yingying Song
 ****************************************************************/
fbe_bool_t fbe_database_is_object_id_lun(fbe_u32_t object_id)
{
    fbe_u32_t ptr_size;
    fbe_dbgext_ptr topology_object_table_ptr = 0;
    ULONG topology_object_table_entry_size;
    fbe_dbgext_ptr topology_object_table_ptr_two = 0;
    fbe_topology_object_status_t  object_status = 0;
    fbe_dbgext_ptr addr;
    fbe_u32_t object_status_offset;
    fbe_u32_t control_handle_offset;
    fbe_dbgext_ptr control_handle_ptr = 0;
    fbe_class_id_t class_id;

    FBE_GET_TYPE_SIZE("sep", fbe_u32_t*, &ptr_size);
    FBE_GET_EXPRESSION("sep", topology_object_table, &topology_object_table_ptr);
    FBE_READ_MEMORY(topology_object_table_ptr, &topology_object_table_ptr_two, ptr_size);
    
    if(topology_object_table_ptr == 0) 
    {
    	return FBE_FALSE;
    }

    FBE_GET_TYPE_SIZE("sep", topology_object_table_entry_s, &topology_object_table_entry_size);
    FBE_GET_FIELD_OFFSET("sep", topology_object_table_entry_s,"object_status", &object_status_offset);
    FBE_GET_FIELD_OFFSET("sep", topology_object_table_entry_s,"control_handle", &control_handle_offset);

    /* Find the object_id corresponding entry ptr */
    topology_object_table_ptr_two = topology_object_table_ptr_two + object_id * topology_object_table_entry_size;

    addr = topology_object_table_ptr_two + object_status_offset;
    FBE_READ_MEMORY(addr, &object_status,sizeof(fbe_topology_object_status_t));

    if(object_status == FBE_TOPOLOGY_OBJECT_STATUS_READY)
    {
        addr = topology_object_table_ptr_two + control_handle_offset;
        FBE_READ_MEMORY(addr, &control_handle_ptr, sizeof(fbe_dbgext_ptr));
        FBE_GET_FIELD_DATA("sep", control_handle_ptr, fbe_base_object_s, class_id, sizeof(fbe_class_id_t), &class_id);

        if(class_id == FBE_CLASS_ID_LUN) 
        {
            return FBE_TRUE;
        }

    }

    return FBE_FALSE;
}/* End fbe_database_is_object_id_lun() */



/*!**************************************************************
 * void fbe_database_get_all_lun_object_ids()
 ****************************************************************
 * @brief
 *  Put all lun object ids into the given buffer. 
 *
 * @param  fbe_u32_t *buffer- output param, used to put all lun object ids.
 * @param  fbe_u32_t size - input param, how many object_id(fbe_u32_t) the buffer can hold.
 * @param  fbe_u32_t *object_count - output param, how many lun the system has totally.
 *
 * @author
 *  11/20/2013:  Created. Yingying Song
 ****************************************************************/
void fbe_database_get_all_lun_object_ids(fbe_u32_t *buffer, fbe_u32_t size, fbe_u32_t *object_count)
{
    fbe_u32_t ptr_size;
    fbe_dbgext_ptr topology_object_table_ptr = 0;
    ULONG topology_object_table_entry_size;
    fbe_dbgext_ptr topology_object_table_ptr_two = 0;
    fbe_topology_object_status_t  object_status = 0;
    fbe_dbgext_ptr addr;
    fbe_u32_t object_status_offset;
    fbe_u32_t control_handle_offset;
    fbe_dbgext_ptr control_handle_ptr = 0;
    fbe_class_id_t class_id;
    fbe_u32_t max_object_count = 0;
    ULONG64 max_topology_entries_p = 0;
    fbe_u32_t cur_object_id = 0;
    fbe_u32_t object_num = 0;

    FBE_GET_EXPRESSION("sep", max_supported_objects, &max_topology_entries_p);
    FBE_READ_MEMORY(max_topology_entries_p, &max_object_count, sizeof(fbe_u32_t));

    FBE_GET_TYPE_SIZE("sep", fbe_u32_t*, &ptr_size);
    FBE_GET_EXPRESSION("sep", topology_object_table, &topology_object_table_ptr);
    FBE_READ_MEMORY(topology_object_table_ptr, &topology_object_table_ptr_two, ptr_size);
    
    if(topology_object_table_ptr == 0) 
    {
        *object_count = 0;
    	return;
    }

    FBE_GET_TYPE_SIZE("sep", topology_object_table_entry_s, &topology_object_table_entry_size);
    FBE_GET_FIELD_OFFSET("sep", topology_object_table_entry_s,"object_status", &object_status_offset);
    FBE_GET_FIELD_OFFSET("sep", topology_object_table_entry_s,"control_handle", &control_handle_offset);

    
    for (cur_object_id = 0; cur_object_id < max_object_count; cur_object_id++)
    {
        addr = topology_object_table_ptr_two + object_status_offset;
        FBE_READ_MEMORY(addr, &object_status,sizeof(fbe_topology_object_status_t));

        if(object_status == FBE_TOPOLOGY_OBJECT_STATUS_READY)
        {
            addr = topology_object_table_ptr_two + control_handle_offset;
            FBE_READ_MEMORY(addr, &control_handle_ptr, sizeof(fbe_dbgext_ptr));
            FBE_GET_FIELD_DATA("sep", control_handle_ptr, fbe_base_object_s, class_id, sizeof(fbe_class_id_t), &class_id);

            if(class_id == FBE_CLASS_ID_LUN) 
            {
                if(object_num >= size)
                {
                    *object_count = size;
                    return;
                }
                buffer[object_num] = cur_object_id;
                object_num++;
            }

        }

        //find next object entry ptr
        topology_object_table_ptr_two += topology_object_table_entry_size;
    }

    *object_count = object_num;
    return ;
}/* End fbe_database_get_all_lun_object_ids() */



/*!**************************************************************
 * fbe_bool_t fbe_database_is_object_id_user_lun()
 ****************************************************************
 * @brief
 *  Determine whether the given the object_id is a "user" LUN or not. 
 *  If given a system LUN object_id, this will return FALSE.
 *
 * @param  object_id - the given object_id.
 *
 * @return fbe_bool_t - If it is a "user" LUN or not.
 *
 * @author
 *  11/20/2013:  Created. Yingying Song
 ****************************************************************/
fbe_bool_t fbe_database_is_object_id_user_lun(fbe_u32_t object_id)
{
    fbe_u32_t ptr_size;
    fbe_dbgext_ptr topology_object_table_ptr = 0;
    ULONG topology_object_table_entry_size;
    fbe_dbgext_ptr topology_object_table_ptr_two = 0;
    fbe_topology_object_status_t  object_status = 0;
    fbe_dbgext_ptr addr;
    fbe_u32_t object_status_offset;
    fbe_u32_t control_handle_offset;
    fbe_dbgext_ptr control_handle_ptr = 0;
    fbe_class_id_t class_id;

    /* If given a system object_id, this will return FALSE. */
    if(fbe_private_space_layout_object_id_is_system_lun(object_id))
    {
        return FBE_FALSE;
    }

    FBE_GET_TYPE_SIZE("sep", fbe_u32_t*, &ptr_size);
    FBE_GET_EXPRESSION("sep", topology_object_table, &topology_object_table_ptr);
    FBE_READ_MEMORY(topology_object_table_ptr, &topology_object_table_ptr_two, ptr_size);
    
    if(topology_object_table_ptr == 0) 
    {
    	return FBE_FALSE;
    }

    FBE_GET_TYPE_SIZE("sep", topology_object_table_entry_s, &topology_object_table_entry_size);
    FBE_GET_FIELD_OFFSET("sep", topology_object_table_entry_s,"object_status", &object_status_offset);
    FBE_GET_FIELD_OFFSET("sep", topology_object_table_entry_s,"control_handle", &control_handle_offset);

    /* Find the object_id corresponding entry ptr */
    topology_object_table_ptr_two = topology_object_table_ptr_two + object_id * topology_object_table_entry_size;

    addr = topology_object_table_ptr_two + object_status_offset;
    FBE_READ_MEMORY(addr, &object_status,sizeof(fbe_topology_object_status_t));

    if(object_status == FBE_TOPOLOGY_OBJECT_STATUS_READY)
    {
        addr = topology_object_table_ptr_two + control_handle_offset;
        FBE_READ_MEMORY(addr, &control_handle_ptr, sizeof(fbe_dbgext_ptr));
        FBE_GET_FIELD_DATA("sep", control_handle_ptr, fbe_base_object_s, class_id, sizeof(fbe_class_id_t), &class_id);

        if(class_id == FBE_CLASS_ID_LUN) 
        {
            return FBE_TRUE;
        }

    }

    return FBE_FALSE;
}/* End fbe_database_is_object_id_user_lun() */



/*!**************************************************************
 * void fbe_database_get_all_user_lun_object_ids()
 ****************************************************************
 * @brief
 *  Put all "user" lun object ids into the given buffer. 
 *  System luns will not be put into the given buffer.
 *
 * @param  fbe_u32_t *buffer- output param, used to put all "user" lun object ids.
 * @param  fbe_u32_t size - input param, how many object_id(fbe_u32_t) the buffer can hold.
 * @param  fbe_u32_t *object_count - output param, how many "user" lun the system has totally.
 *
 * @author
 *  11/20/2013:  Created. Yingying Song
 ****************************************************************/
void fbe_database_get_all_user_lun_object_ids(fbe_u32_t *buffer, fbe_u32_t size, fbe_u32_t *object_count)
{
    fbe_u32_t ptr_size;
    fbe_dbgext_ptr topology_object_table_ptr = 0;
    ULONG topology_object_table_entry_size;
    fbe_dbgext_ptr topology_object_table_ptr_two = 0;
    fbe_topology_object_status_t  object_status = 0;
    fbe_dbgext_ptr addr;
    fbe_u32_t object_status_offset;
    fbe_u32_t control_handle_offset;
    fbe_dbgext_ptr control_handle_ptr = 0;
    fbe_class_id_t class_id;
    fbe_u32_t max_object_count = 0;
    ULONG64 max_topology_entries_p = 0;
    fbe_u32_t cur_object_id = 0;
    fbe_u32_t object_num = 0;
    fbe_u32_t start_object_id = FBE_RESERVED_OBJECT_IDS;

    FBE_GET_EXPRESSION("sep", max_supported_objects, &max_topology_entries_p);
    FBE_READ_MEMORY(max_topology_entries_p, &max_object_count, sizeof(fbe_u32_t));

    FBE_GET_TYPE_SIZE("sep", fbe_u32_t*, &ptr_size);
    FBE_GET_EXPRESSION("sep", topology_object_table, &topology_object_table_ptr);
    FBE_READ_MEMORY(topology_object_table_ptr, &topology_object_table_ptr_two, ptr_size);

    if(topology_object_table_ptr == 0) 
    {
        *object_count = 0;
    	return;
    }

    FBE_GET_TYPE_SIZE("sep", topology_object_table_entry_s, &topology_object_table_entry_size);
    FBE_GET_FIELD_OFFSET("sep", topology_object_table_entry_s,"object_status", &object_status_offset);
    FBE_GET_FIELD_OFFSET("sep", topology_object_table_entry_s,"control_handle", &control_handle_offset);

    /* Find the first non system object corresponding entry */
    topology_object_table_ptr_two = topology_object_table_ptr_two 
                                    + (topology_object_table_entry_size * start_object_id);

    for (cur_object_id = start_object_id; cur_object_id < max_object_count; cur_object_id++)
    {
        addr = topology_object_table_ptr_two + object_status_offset;
        FBE_READ_MEMORY(addr, &object_status,sizeof(fbe_topology_object_status_t));

        if(object_status == FBE_TOPOLOGY_OBJECT_STATUS_READY)
        {
            addr = topology_object_table_ptr_two + control_handle_offset;
            FBE_READ_MEMORY(addr, &control_handle_ptr, sizeof(fbe_dbgext_ptr));
            FBE_GET_FIELD_DATA("sep", control_handle_ptr, fbe_base_object_s, class_id, sizeof(fbe_class_id_t), &class_id);

            if(class_id == FBE_CLASS_ID_LUN) 
            {
                if(object_num >= size)
                {
                    *object_count = size;
                    return;
                }
                buffer[object_num] = cur_object_id;
                object_num++;
            }

        }

        //find next object entry ptr
        topology_object_table_ptr_two += topology_object_table_entry_size;
    }

    *object_count = object_num;
    return;
}/* End fbe_database_get_all_user_lun_object_ids() */




fbe_u32_t fbe_database_get_max_number_of_lun(void)
{
    fbe_dbgext_ptr database_service_ptr = 0;
    fbe_u32_t   offset;
    fbe_u32_t   max_user_lun_offset;
    fbe_environment_limits_t logical_objects_limits;
    fbe_u32_t   platform_max_user_lun;

    FBE_GET_EXPRESSION("sep", fbe_database_service, &database_service_ptr);

    FBE_GET_FIELD_OFFSET("sep", fbe_database_service_t, "logical_objects_limits", &offset);
    FBE_READ_MEMORY(database_service_ptr + offset, &logical_objects_limits, sizeof(fbe_environment_limits_t));

    FBE_GET_FIELD_OFFSET("sep", fbe_environment_limits_t, "platform_max_user_lun", &max_user_lun_offset);
    FBE_READ_MEMORY(database_service_ptr + offset + max_user_lun_offset, &platform_max_user_lun, sizeof(fbe_u32_t));

    return platform_max_user_lun + FBE_RESERVED_OBJECT_IDS;
}



//end of fbe_database_service_debug.c
