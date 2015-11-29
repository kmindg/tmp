/***************************************************************************
 *  Copyright (C)  EMC Corporation 2000-07
 *  All rights reserved.
 *  Licensed material - property of EMC Corporation. * All rights reserved.
 ***************************************************************************/

/*!**************************************************************************
 * @file esp_commands.c
 ***************************************************************************
 *
 * @brief
 *  This file contains debugging commands for Environment Service Package driver.
.*
 * @version
 *   16-August-2010:  Created. Vishnu Sharma
 *
 ***************************************************************************/

#include "pp_dbgext.h"
#include "fbe/fbe_topology_interface.h"
#include "fbe_sps_mgmt_debug.h"
#include "fbe_base_object_debug.h"
#include "fbe/fbe_class.h"
#include "fbe/fbe_devices.h"
#include "fbe_ps_mgmt_debug.h"
#include "fbe_encl_mgmt_debug.h"
#include "fbe_base_environment_debug.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe_logical_drive_debug.h"
#include "fbe_base_physical_drive_debug.h"
#include "fbe_raid_group_debug.h"
#include "fbe_virtual_drive_debug.h"
#include "fbe_provision_drive_debug.h"
#include "fbe_base_object_trace.h"
#include "fbe_transport_debug.h"
#include "fbe_topology_debug.h"

#define PACKAGE "espkg"

fbe_status_t fbe_object_id_tree_debug_trace(const fbe_u8_t * module_name,fbe_object_id_t object_id,fbe_u32_t recursive_count);
fbe_status_t fbe_check_for_continues_raid_class_id(const fbe_u8_t * module_name , fbe_object_id_t object_id,fbe_object_id_t current_object_id);
fbe_status_t fbe_object_info_debug_trace(const fbe_u8_t * object_name,const fbe_u8_t * module_name, fbe_dbgext_ptr object_id, fbe_u32_t tabs);

/* ***************************************************************************
 *
 * @brief
 *  Extension to display sps status data.
 *
 *  Arguments: None
 *  Return Value: None
 * 
 * @version
 *   16-August-2010:  Created. Vishnu Sharma
 *
 ***************************************************************************/

#pragma data_seg ("EXT_HELP$4spsstat")
static char CSX_MAYBE_UNUSED usageMsg_spsstat[] =
"!spsstat\n"
"  Displays sps status info.\n";
#pragma data_seg ()

CSX_DBG_EXT_DEFINE_CMD(spsstat, "spsstat")
{
    fbe_dbgext_ptr topology_object_table_ptr = 0;
    fbe_topology_object_status_t  object_status = 0;
    fbe_dbgext_ptr control_handle_ptr = 0;
    ULONG topology_object_table_entry_size = 0;
    fbe_u32_t loop_counter = 0;
    fbe_class_id_t class_id = FBE_CLASS_ID_INVALID;
	ULONG64 max_toplogy_entries_p = 0;
	fbe_u32_t max_entries;
	fbe_dbgext_ptr topology_object_tbl_ptr = 0;
    fbe_u32_t ptr_size;
    fbe_status_t status = FBE_STATUS_OK;

    /* For now we assume that only esp package has SPS  object.
     * If it is not set to esp, then some things might not get resolved properly.
     */
    const fbe_u8_t * module_name = PACKAGE; 

    status = fbe_debug_get_ptr_size(pp_ext_module_name, &ptr_size);
    if (status != FBE_STATUS_OK) 
    {
        fbe_debug_trace_func(NULL, "%s unable to get ptr size status: %d\n", __FUNCTION__, status);
        return; 
    }
    
     /* Get the topology table. */
    FBE_GET_EXPRESSION(module_name, topology_object_table, &topology_object_table_ptr);
	FBE_READ_MEMORY(topology_object_table_ptr, &topology_object_tbl_ptr, ptr_size);
    fbe_debug_trace_func(NULL, "\t topology_object_tbl_ptr %llx\n",
			 (unsigned long long)topology_object_tbl_ptr);

    /* Get the size of the topology entry. */
    FBE_GET_TYPE_SIZE(module_name, topology_object_table_entry_s, &topology_object_table_entry_size);
    fbe_debug_trace_func(NULL, "\t topology_object_table_entry_size %d \n", topology_object_table_entry_size);

	FBE_GET_EXPRESSION(pp_ext_module_name, max_supported_objects, &max_toplogy_entries_p);
	FBE_READ_MEMORY(max_toplogy_entries_p, &max_entries, sizeof(fbe_u32_t));

	fbe_debug_trace_func(NULL, "\t max_entries:%d \n", max_entries);

    if(topology_object_table_ptr == 0){
        fbe_debug_trace_func(NULL, "\t topology_object_table_is not available \n");
        return;
    }

    for(loop_counter = 0; loop_counter < max_entries ; loop_counter++)
    {
        /* Fetch the object's topology status. */
        FBE_GET_FIELD_DATA(module_name,
                        topology_object_tbl_ptr,
                        topology_object_table_entry_s,
                        object_status,
                        sizeof(fbe_topology_object_status_t),
                        &object_status);

        /* Fetch the control handle, which is the pointer to the object. */
        FBE_GET_FIELD_DATA(module_name,
                        topology_object_tbl_ptr,
                        topology_object_table_entry_s,
                        control_handle,
                        sizeof(fbe_dbgext_ptr),
                        &control_handle_ptr);
        
        FBE_GET_FIELD_DATA(module_name, 
                        control_handle_ptr,
                        fbe_base_object_s,
                        class_id,
                        sizeof(fbe_class_id_t),
                        &class_id);

        if(class_id == FBE_CLASS_ID_SPS_MGMT)
        {
            if (object_status == FBE_TOPOLOGY_OBJECT_STATUS_READY)
            {
                 /* Display all the information about SPS MGMT object */
                  fbe_sps_mgmt_debug_stat(module_name, control_handle_ptr, fbe_debug_trace_func, NULL);
                  fbe_debug_trace_func(NULL, "\n");
                  
            }
            else
            {
                fbe_debug_trace_func(NULL,"\n\t Information cannot be displayed because the object is in  %s state\n",
                                      fbe_topology_object_getStatusString(object_status));
            }
            return;
        }
        topology_object_tbl_ptr += topology_object_table_entry_size;      
    }
    return;
}

/* ***************************************************************************
 *
 * @brief
 *  Extension to display fup status data.
 *
 *  Arguments: None
 *  Return Value: None
 * 
 * @version
 *   8-October-2010:  Created. Vishnu Sharma
 *
 ***************************************************************************/

#pragma data_seg ("EXT_HELP$4fupstat")
static char usageMsg_fupstat[] =
"!fupstat\n"
"    <s> [-d <device type>] [-b <bus num>] [-e <encl num>] \n"
"        - Device Type: lcc, ps or fan.\n"           
"        - Display the completion status of the firmware upgrade.\n"
"          If the firmware upgrade is in progress, display the work state.\n"
"    <r> [-d <device type>] [-b <bus num>] [-e <encl num>] \n"
"        - Display the firmware revision.\n"
"    If there is no location specified,\n"
"    do all the device with the specified device type.\n ";
#pragma data_seg ()

CSX_DBG_EXT_DEFINE_CMD(fupstat, "fupstat")
{
    fbe_dbgext_ptr topology_object_table_ptr = 0;
    fbe_topology_object_status_t  object_status = 0;
    fbe_dbgext_ptr control_handle_ptr = 0;
    fbe_u32_t topology_object_table_entry_size = 0;
    fbe_u32_t loop_counter = 0;
    fbe_class_id_t class_id = FBE_CLASS_ID_INVALID;
    fbe_bool_t  fup_status = FALSE,fup_revision = FALSE;
    fbe_u64_t deviceType = FBE_DEVICE_TYPE_INVALID;
    fbe_char_t *   str = NULL;
    fbe_bool_t  location_flag = FBE_FALSE;
    fbe_status_t  status = FBE_STATUS_OK;
    fbe_class_id_t      classId;
    fbe_device_physical_location_t location = {0};
	ULONG64 max_toplogy_entries_p = 0;
	fbe_u32_t max_entries;
	fbe_dbgext_ptr topology_object_tbl_ptr = 0;
    fbe_u32_t ptr_size;
    
    const fbe_u8_t * module_name = PACKAGE; 

    status = fbe_debug_get_ptr_size(module_name, &ptr_size);
    if (status != FBE_STATUS_OK) 
    {
        csx_dbg_ext_print("%s unable to get ptr size status: %d\n", __FUNCTION__, status);
        return ; 
    }
    
    if (strlen(args) && strncmp(args,"s",1) == 0)
    {
        fup_status = FBE_TRUE;
    }
    else if (strlen(args) && strncmp(args,"r",1) == 0)
    {
        fup_revision =FBE_TRUE;
    }
    args += FBE_MIN(2, strlen(args));

    if (strlen(args) && strncmp(args,"-d",2) == 0)
    {
        args += FBE_MIN(3, strlen(args));
        if (strlen(args) && strncmp(args,"lcc",3) == 0)
        {
            deviceType = FBE_DEVICE_TYPE_LCC;
            args += FBE_MIN(4, strlen(args));
        }
        else if(strlen(args) && strncmp(args,"ps",2) == 0)
        {
            deviceType = FBE_DEVICE_TYPE_PS;
            args += FBE_MIN(3, strlen(args));
        }
        else if(strlen(args) && strncmp(args,"fan",3) == 0)
        {
            deviceType = FBE_DEVICE_TYPE_FAN;
            args += FBE_MIN(4, strlen(args));
        }
        else
        {
            csx_dbg_ext_print("Invalid device type\n");
            csx_dbg_ext_print("Please provide valid device type:lcc, ps or fan\n");
            return;
        }
    }
    
    status = fbe_base_env_map_devicetype_to_classid(deviceType,&classId);
    
    if ((status != FBE_STATUS_OK)|| 
        (fup_status != FBE_TRUE && fup_revision != FBE_TRUE))
    {
        csx_dbg_ext_print("\n Invalid Switch.Please check the command \n");
        csx_dbg_ext_print(usageMsg_fupstat);
        return;
    }
    if (strlen(args))
    {
        str = strtok((char *) args, " \t" );
        while (str != NULL)
        {
            if(strncmp(str, "-b", 2) == 0)
            {
                str = strtok (NULL, " \t");
                location.bus = (fbe_u32_t)strtoul(str, 0, 0);
                location_flag =  FBE_TRUE;
            }
            else if(strncmp(str, "-e", 2) == 0)
            {
                str = strtok (NULL, " \t");
                location.enclosure = (fbe_u32_t)strtoul(str, 0, 0);
                location_flag =  FBE_TRUE;
            }
            str = strtok (NULL, " \t");
        }
    }
    /* Get the topology table. */
    FBE_GET_EXPRESSION(module_name, topology_object_table, &topology_object_table_ptr);
	FBE_READ_MEMORY(topology_object_table_ptr, &topology_object_tbl_ptr, ptr_size);

	FBE_GET_EXPRESSION(pp_ext_module_name, max_supported_objects, &max_toplogy_entries_p);
	FBE_READ_MEMORY(max_toplogy_entries_p, &max_entries, sizeof(fbe_u32_t));

    /* Get the size of the topology entry. */
    FBE_GET_TYPE_SIZE(module_name, topology_object_table_entry_s, &topology_object_table_entry_size);
    
    if(topology_object_table_ptr == 0){
        fbe_debug_trace_func(NULL, "\t topology_object_table_is not available \n");
        return;
    }
    fbe_debug_trace_func(NULL,"   \nFirmware Upgrade Information:\n");
    
    for(loop_counter = 0; loop_counter < max_entries ; loop_counter++)
    {
        /* Fetch the object's topology status. */
        FBE_GET_FIELD_DATA(module_name,
                        topology_object_tbl_ptr,
                        topology_object_table_entry_s,
                        object_status,
                        sizeof(fbe_topology_object_status_t),
                        &object_status);

        /* Fetch the control handle, which is the pointer to the object. */
        FBE_GET_FIELD_DATA(module_name,
                        topology_object_tbl_ptr,
                        topology_object_table_entry_s,
                        control_handle,
                        sizeof(fbe_dbgext_ptr),
                        &control_handle_ptr);
        
        FBE_GET_FIELD_DATA(module_name, 
                        control_handle_ptr,
                        fbe_base_object_s,
                        class_id,
                        sizeof(fbe_class_id_t),
                        &class_id);

        if(class_id == classId)
        {
            if (object_status == FBE_TOPOLOGY_OBJECT_STATUS_READY)
            {
                  /* Display all the information about PS MGMT object */
                  if(class_id == FBE_CLASS_ID_PS_MGMT)
                  {
                      fbe_ps_mgmt_debug_stat(module_name, control_handle_ptr, fbe_debug_trace_func, NULL,
                          &location,fup_status,location_flag);
                  }
                  /* Display all the information about ENCL MGMT object */
                  else if(class_id == FBE_CLASS_ID_ENCL_MGMT)
                  {
                      fbe_encl_mgmt_debug_stat(module_name, control_handle_ptr, fbe_debug_trace_func, NULL,
                          &location,fup_status,location_flag);
                  }
                  fbe_debug_trace_func(NULL, "\n");
                  
            }
            else
            {
                fbe_debug_trace_func(NULL,"\n\t Information cannot be displayed because the object is in  %s state\n",
                                      fbe_topology_object_getStatusString(object_status));
            }
            return;
        }
        topology_object_tbl_ptr += topology_object_table_entry_size;      
    }
    return;
}

/**************************************************************************
 *      di_display()
 **************************************************************************
 *
 *  @brief
 *    Helper method that displays the physical drive information for all
 *    drives within a dmo object.
 *
 *  @param
 *    control_handle_ptr - pointer to drive mgmt structure.
 * 
 *  @author
 *   07/26/2011:  Created. Armando Vega
 *
 **************************************************************************/
void di_display(fbe_dbgext_ptr control_handle_ptr)
{
    int i;
    fbe_u32_t max_drives;    
    fbe_u32_t drive_info_size;
    fbe_dbgext_ptr drive_info_ptr;
    fbe_device_physical_location_t drive_location;
    fbe_u8_t drive_description_buff[FBE_SCSI_DESCRIPTION_BUFF_SIZE+1];
    fbe_u8_t vendor_id[FBE_SCSI_INQUIRY_VENDOR_ID_SIZE+1];
    fbe_u8_t product_id[FBE_SCSI_INQUIRY_PRODUCT_ID_SIZE + 1];
    fbe_u8_t product_rev[FBE_SCSI_INQUIRY_VPD_PROD_REV_SIZE+1];
    fbe_u8_t product_sn[FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE+1];
    fbe_lba_t gross_capacity;
    fbe_u8_t tla_part_number[FBE_SCSI_INQUIRY_TLA_SIZE+1];
    fbe_drive_type_t drive_type;    
    fbe_block_size_t block_size;
    fbe_u32_t drive_qdepth;
    fbe_u32_t drive_RPM;    
    fbe_bool_t spin_down_qualified;    
    fbe_u32_t spin_up_time_in_minutes;
    fbe_u32_t stand_by_time_in_minutes;
    fbe_u32_t spin_up_count;
    fbe_physical_drive_speed_t speed_capability;
    fbe_u8_t dg_part_number_ascii[FBE_SCSI_INQUIRY_PART_NUMBER_SIZE+1];
    fbe_time_t last_log;
    fbe_object_id_t object_id;    
    
    FBE_GET_TYPE_SIZE(PACKAGE, fbe_drive_info_t, &drive_info_size);

    FBE_GET_FIELD_DATA(PACKAGE,
                       control_handle_ptr,
                       fbe_drive_mgmt_t,
                       max_supported_drives,
                       sizeof(fbe_u32_t),
                       &max_drives); 

    FBE_GET_FIELD_DATA(PACKAGE,
                       control_handle_ptr,
                       fbe_drive_mgmt_t,
                       drive_info_array,
                       sizeof(fbe_dbgext_ptr),
                       &drive_info_ptr); 
       
    for (i = 0; i < max_drives; i++)
    {
        FBE_GET_FIELD_DATA(PACKAGE,
                           drive_info_ptr,
                           fbe_drive_info_t,
                           object_id,
                           sizeof(fbe_object_id_t),
                           &object_id); 

        if (object_id == FBE_OBJECT_ID_INVALID)
        {
            /* This drive is invalid, skip */
            continue;
        }

        FBE_GET_FIELD_DATA(PACKAGE,
                           drive_info_ptr,
                           fbe_drive_info_t,
                           location,
                           sizeof(fbe_device_physical_location_t),
                           &drive_location); 

        FBE_GET_FIELD_DATA(PACKAGE,
                           drive_info_ptr,
                           fbe_drive_info_t,
                           drive_description_buff,
                           (FBE_SCSI_DESCRIPTION_BUFF_SIZE+1),
                           &drive_description_buff);         

        FBE_GET_FIELD_DATA(PACKAGE,
                           drive_info_ptr,
                           fbe_drive_info_t,
                           vendor_id,
                           (FBE_SCSI_INQUIRY_VENDOR_ID_SIZE+1),
                           &vendor_id);  

        FBE_GET_FIELD_DATA(PACKAGE,
                           drive_info_ptr,
                           fbe_drive_info_t,
                           product_id,
                           (FBE_SCSI_INQUIRY_PRODUCT_ID_SIZE + 1),
                           &product_id);  

        FBE_GET_FIELD_DATA(PACKAGE,
                           drive_info_ptr,
                           fbe_drive_info_t,
                           rev,
                           (FBE_SCSI_INQUIRY_VPD_PROD_REV_SIZE+1),
                           &product_rev);  

        FBE_GET_FIELD_DATA(PACKAGE,
                           drive_info_ptr,
                           fbe_drive_info_t,
                           sn,
                           (FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE+1),
                           &product_sn); 

        FBE_GET_FIELD_DATA(PACKAGE,
                           drive_info_ptr,
                           fbe_drive_info_t,
                           gross_capacity,
                           sizeof(fbe_lba_t),
                           &gross_capacity); 

        FBE_GET_FIELD_DATA(PACKAGE,
                           drive_info_ptr,
                           fbe_drive_info_t,
                           tla,
                           (FBE_SCSI_INQUIRY_TLA_SIZE+1),
                           &tla_part_number); 

        FBE_GET_FIELD_DATA(PACKAGE,
                           drive_info_ptr,
                           fbe_drive_info_t,
                           drive_type,
                           sizeof(fbe_drive_type_t),
                           &drive_type); 

        FBE_GET_FIELD_DATA(PACKAGE,
                           drive_info_ptr,
                           fbe_drive_info_t,
                           block_size,
                           sizeof(fbe_block_size_t),
                           &block_size); 

        FBE_GET_FIELD_DATA(PACKAGE,
                           drive_info_ptr,
                           fbe_drive_info_t,
                           drive_qdepth,
                           sizeof(fbe_u32_t),
                           &drive_qdepth); 

        FBE_GET_FIELD_DATA(PACKAGE,
                           drive_info_ptr,
                           fbe_drive_info_t,
                           drive_RPM,
                           sizeof(fbe_u32_t),
                           &drive_RPM);         
        
        FBE_GET_FIELD_DATA(PACKAGE,
                           drive_info_ptr,
                           fbe_drive_info_t,
                           spin_down_qualified,
                           sizeof(fbe_bool_t),
                           &spin_down_qualified); 

        FBE_GET_FIELD_DATA(PACKAGE,
                           drive_info_ptr,
                           fbe_drive_info_t,
                           spin_up_time_in_minutes,
                           sizeof(fbe_u32_t),
                           &spin_up_time_in_minutes); 

        FBE_GET_FIELD_DATA(PACKAGE,
                           drive_info_ptr,
                           fbe_drive_info_t,
                           stand_by_time_in_minutes,
                           sizeof(fbe_u32_t),
                           &stand_by_time_in_minutes); 

        FBE_GET_FIELD_DATA(PACKAGE,
                           drive_info_ptr,
                           fbe_drive_info_t,
                           spin_up_count,
                           sizeof(fbe_u32_t),
                           &spin_up_count);          

        FBE_GET_FIELD_DATA(PACKAGE,
                           drive_info_ptr,
                           fbe_drive_info_t,
                           speed_capability,
                           sizeof(fbe_physical_drive_speed_t),
                           &speed_capability);          

        FBE_GET_FIELD_DATA(PACKAGE,
                           drive_info_ptr,
                           fbe_drive_info_t,
                           dg_part_number_ascii,
                           (FBE_SCSI_INQUIRY_PART_NUMBER_SIZE+1),
                           &dg_part_number_ascii);          

        FBE_GET_FIELD_DATA(PACKAGE,
                           drive_info_ptr,
                           fbe_drive_info_t,
                           last_log,
                           sizeof(fbe_time_t),
                           &last_log);             

        csx_dbg_ext_print("\n--------------------------------------------------------------------------------\n");        
        csx_dbg_ext_print("\n");
        csx_dbg_ext_print("Disk - %d_%d_%d ",
            drive_location.bus,
            drive_location.enclosure,
            drive_location.slot);
        csx_dbg_ext_print("[Bus %d Encl %d Disk %d]\n",
            drive_location.bus,
            drive_location.enclosure,
            drive_location.slot);
        csx_dbg_ext_print("\n");    

        csx_dbg_ext_print("Object id:           0x%x\n", object_id);
        csx_dbg_ext_print("Drive Detail:        %s\n", drive_description_buff);

        csx_dbg_ext_print("  -------------------------------------\n");
        csx_dbg_ext_print("  Physical Drive Attributes:\n");
        csx_dbg_ext_print("  -------------------------------------\n");
        csx_dbg_ext_print("   Vendor ID:                %s\n", vendor_id);
        csx_dbg_ext_print("   Product ID:               %s\n", product_id);
        csx_dbg_ext_print("   Product Rev:              %s\n", product_rev);     
        csx_dbg_ext_print("   Product Sr.No.:           %s\n", product_sn);
        csx_dbg_ext_print("   Drive Capacity:           0x%X sectors\n", (unsigned int)gross_capacity);
        csx_dbg_ext_print("   Drive TLA info:           %s\n", tla_part_number);

        /* Drive Class is an enumerated type - print raw and interpreted value */
        switch(drive_type)
        {
            case FBE_DRIVE_TYPE_FIBRE:
                csx_dbg_ext_print("   Drive Type:               0x%X  (%s)\n", drive_type, "Fibre Channel");
                break;
    
            case FBE_DRIVE_TYPE_SAS:
                csx_dbg_ext_print("   Drive Type:               0x%X (%s)\n", drive_type, "SAS");
                break;
                
             case FBE_DRIVE_TYPE_SATA:
                csx_dbg_ext_print("   Drive Type:               0x%X (%s)\n", drive_type, "SATA");
                break;
            
            case FBE_DRIVE_TYPE_SAS_FLASH_HE:
                csx_dbg_ext_print("   Drive Type:               0x%X (%s)\n", drive_type, "SAS FLASH HE");
                break;
            
            case FBE_DRIVE_TYPE_SATA_FLASH_HE:
                csx_dbg_ext_print("   Drive Type:               0x%X (%s)\n", drive_type, "SATA FLASH HE");
                break;
            
            case FBE_DRIVE_TYPE_SAS_NL:
                csx_dbg_ext_print("   Drive Type:               0x%X (%s)\n", drive_type, "SAS NL");
                break;
            
            case FBE_DRIVE_TYPE_SAS_FLASH_ME:
                csx_dbg_ext_print("   Drive Type:               0x%X (%s)\n", drive_type, "SAS FLASH ME");
                break;

            case FBE_DRIVE_TYPE_SAS_FLASH_LE:
                csx_dbg_ext_print("   Drive Type:               0x%X (%s)\n", drive_type, "SAS FLASH LE");
                break;

            case FBE_DRIVE_TYPE_SAS_FLASH_RI:
                csx_dbg_ext_print("   Drive Type:               0x%X (%s)\n", drive_type, "SAS FLASH RI");
                break;

            default:
                csx_dbg_ext_print("   Drive Type:               0x%X (%s)\n", drive_type, "INVALID");
                break;    
        } // end switch

        csx_dbg_ext_print("   Block Count:              0x%llX\n", (unsigned long long)gross_capacity);
        csx_dbg_ext_print("   Block Size:               0x%X\n", block_size);
        csx_dbg_ext_print("   Drive Qdepth:             %d\n", drive_qdepth);
        csx_dbg_ext_print("   Drive RPM:                %d\n", drive_RPM);
        csx_dbg_ext_print("   Spindown HW Qualified(DH):%s\n", spin_down_qualified? "YES ": "NO");            
        csx_dbg_ext_print("   Spinning Ticks(min):      %d\n", spin_up_time_in_minutes);
        csx_dbg_ext_print("   Standby Ticks(min):       %d\n", stand_by_time_in_minutes);
        csx_dbg_ext_print("   Number of Spinups:        %d\n", spin_up_count);

        /* Speed capability is an enumerated value - print raw and interpreted value */
        switch(speed_capability)
        {
            case FBE_DRIVE_SPEED_1_5_GB:
                csx_dbg_ext_print("   Speed Capability:         0x%X (1.5-GB)\n", speed_capability);
                break;

            case FBE_DRIVE_SPEED_2GB:
                csx_dbg_ext_print("   Speed Capability:         0x%X (2-GB)\n", speed_capability);
                break;
            
            case FBE_DRIVE_SPEED_3GB:
                csx_dbg_ext_print("   Speed Capability:         0x%X (3-GB)\n", speed_capability);
                break;
            
            case FBE_DRIVE_SPEED_4GB:
                csx_dbg_ext_print("   Speed Capability:         0x%X (4-GB)\n", speed_capability);
                break;
            
            case FBE_DRIVE_SPEED_6GB:
                csx_dbg_ext_print("   Speed Capability:         0x%X (6-GB)\n", speed_capability);
                break;
            
            case FBE_DRIVE_SPEED_8GB:
                csx_dbg_ext_print("   Speed Capability:         0x%X (8-GB)\n", speed_capability);
                break;
            
            case FBE_DRIVE_SPEED_10GB:
                csx_dbg_ext_print("   Speed Capability:         0x%X (10-GB)\n", speed_capability);
                break;

            case FBE_DRIVE_SPEED_12GB:
                csx_dbg_ext_print("   Speed Capability:         0x%X (12-GB)\n", speed_capability);
                break;

            default:
                /* Display Unknown for any invalid speed capability*/
                csx_dbg_ext_print("   Speed Capability:          %d (Unknown)\n", speed_capability);
                break;    
        } // end switch

        csx_dbg_ext_print("   Additional info:          %s\n", dg_part_number_ascii);
        csx_dbg_ext_print("   Time since last I/O:      %d Seconds ago\n", (int)last_log/100);
      
        drive_info_ptr += drive_info_size;

    } // end for loop
}

/* ***************************************************************************
 *
 * @brief
 *  Debug macro to display physical drive attributes from dmo.
 *
 *  Arguments: None
 *  Return Value: None
 * 
 * @version
 *   25-July-2012:  Created. Armando Vega
 *
 ***************************************************************************/
#pragma data_seg ("EXT_HELP$4di")
static char CSX_MAYBE_UNUSED usageMsg_di[] =
"!di\n"
"  Displays physical drive attributes.\n";
#pragma data_seg ()

CSX_DBG_EXT_DEFINE_CMD(di, "di")
{
    fbe_dbgext_ptr topology_object_table_ptr = 0;
	fbe_topology_object_status_t  object_status;
	fbe_dbgext_ptr control_handle_ptr = 0;
	ULONG topology_object_table_entry_size;
	int i = 0;
	ULONG64 max_toplogy_entries_p = 0;
	fbe_u32_t max_entries;
    fbe_u32_t ptr_size;
    fbe_status_t status = FBE_STATUS_OK;    
    fbe_dbgext_ptr topology_object_tbl_ptr = 0;    

    status = fbe_debug_get_ptr_size(PACKAGE, &ptr_size);

    if (status != FBE_STATUS_OK) 
    {
        csx_dbg_ext_print("%s unable to get ptr size status: %d\n", __FUNCTION__, status);
        status = FBE_STATUS_INVALID;
        return ; 
    }

	/* Get the topology table.
     */
	FBE_GET_EXPRESSION(PACKAGE, topology_object_table, &topology_object_table_ptr);
	FBE_READ_MEMORY(topology_object_table_ptr, &topology_object_tbl_ptr, ptr_size);
	csx_dbg_ext_print("\t topology_object_table_ptr %llx\n", (unsigned long long)topology_object_tbl_ptr);

    /* Get the size of the topology entry.
     */
	FBE_GET_TYPE_SIZE(PACKAGE, topology_object_table_entry_s, &topology_object_table_entry_size);
	csx_dbg_ext_print("\t topology_object_table_entry_size %d \n", topology_object_table_entry_size);

	FBE_GET_EXPRESSION(PACKAGE, max_supported_objects, &max_toplogy_entries_p);
	FBE_READ_MEMORY(max_toplogy_entries_p, &max_entries, sizeof(fbe_u32_t));

    if(topology_object_tbl_ptr == 0){
        csx_dbg_ext_print("%s Could not get topology_object_pointer\n", __FUNCTION__);
		return;
	}

	object_status = 0;

	for(i = 0; i < max_entries ; i++)
    {
        if (FBE_CHECK_CONTROL_C())
        {
            return;
        }
        /* Fetch the object's topology status.
         */
        FBE_GET_FIELD_DATA(PACKAGE,
                           topology_object_tbl_ptr,
                           topology_object_table_entry_s,
                           object_status,
                           sizeof(fbe_topology_object_status_t),
                           &object_status);

        /* Fetch the control handle, which is the pointer to the object.
         */
		FBE_GET_FIELD_DATA(PACKAGE,
						   topology_object_tbl_ptr,
						   topology_object_table_entry_s,
						   control_handle,
						   sizeof(fbe_dbgext_ptr),
						   &control_handle_ptr);

        /* If the object's topology status is ready, then get the
         * topology status. 
         */
		if (object_status == FBE_TOPOLOGY_OBJECT_STATUS_READY)
        {
            fbe_class_id_t class_id;

            FBE_GET_FIELD_DATA(PACKAGE, 
                               control_handle_ptr,
                               fbe_base_object_s,
                               class_id,
                               sizeof(fbe_class_id_t),
                               &class_id);
             

            if (class_id == FBE_CLASS_ID_DRIVE_MGMT)
            {                               
                di_display(control_handle_ptr);
            }
		} /* End if object's topology status is ready. */
		topology_object_tbl_ptr += topology_object_table_entry_size;
	}

	return;
}

//end of esp_commands.c
