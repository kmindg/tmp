/***************************************************************************
 * Copyright (C) EMC Corporation 2011 
 * All rights reserved. 
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_cli_lib_disk_info_cmd.c
 ***************************************************************************
 *
 * @brief
 *  This file contains fbecli "di" command which displays information
 *  of physical drive and also formats the drive.
 *
 * @ingroup fbe_cli
 *
 * @revision
 *  
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include <stdio.h>
#include "fbe_cli_private.h"
#include "fbe_trace.h"
#include "fbe/fbe_class.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe_transport_trace.h"
#include "fbe_base_object_trace.h"
#include "fbe_block_transport_trace.h"
#include "fbe/fbe_api_trace.h"
#include "fbe/fbe_class.h"
#include "fbe/fbe_api_common.h"
#include "fbe_cli_lib_disk_info_cmd.h"
#include "fbe_block_transport_trace.h"
#include "fbe/fbe_api_common_transport.h"
#include "fbe/fbe_api_virtual_drive_interface.h"
#include "fbe/fbe_api_power_saving_interface.h"
#include "fbe/fbe_api_lun_interface.h"
#include "fbe/fbe_api_base_config_interface.h"
#include "fbe/fbe_api_esp_drive_mgmt_interface.h"
#include "fbe/fbe_api_utils.h"
#include "fbe_cli_lurg.h"
#include "fbe/fbe_api_lurg_interface.h"
#include "fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_logical_error_injection_interface.h"
#include "fbe/fbe_protocol_error_injection_service.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe_system_limits.h"
#include "fbe/fbe_api_system_bg_service_interface.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_file.h"

typedef fbe_status_t (* fbe_cli_command_bms_fn)(fbe_u32_t context1, fbe_u32_t context2, fbe_package_id_t package_id, fbe_bool_t context3);

#define SAFE_STRNCPY( dest, source, destLen ) \
	{memset(dest, 0, destLen); \
	strncpy(dest, source, destLen -1);}

/*************************
 *   FUNCTION PROTOTYPES:
 *************************/
static fbe_status_t fbe_cli_disk_display_info_power_saving(fbe_u32_t port,fbe_u32_t enclosure,fbe_u32_t slot);
static char *fbe_cli_map_lifecycle_state_to_str(fbe_lifecycle_state_t state);
static fbe_status_t fbe_cli_disk_display_bms_list(fbe_u32_t object_id, fbe_u32_t logpage, fbe_package_id_t package_id, fbe_bool_t b_display_bms);
static void fbe_cli_disk_display_bms_completion(fbe_packet_completion_context_t context);
static void fbe_cli_disk_bms_file(fbe_char_t *location, fbe_char_t *drive_name);
static void fbe_cli_disk_execute_cmd(fbe_cli_command_fn command_fn, fbe_u32_t context, fbe_u32_t port, fbe_u32_t enclosure, fbe_u32_t slot);
static void fbe_cli_disk_execute_bms_cmd(fbe_cli_command_bms_fn command_bms_fn, fbe_u32_t context, fbe_u32_t port, fbe_u32_t enclosure, fbe_u32_t slot, fbe_bool_t display_b);
static fbe_status_t fbe_cli_execute_command_for_bms_objects(fbe_class_id_t filter_class_id,
                                                 fbe_package_id_t package_id,
                                                 fbe_u32_t filter_port,
                                                 fbe_u32_t filter_enclosure,
                                                 fbe_u32_t filter_drive_no,
                                                 fbe_cli_command_bms_fn command_function_p,
                                                 fbe_u32_t context,
                                                 fbe_bool_t display_b);
static void fbe_cli_disk_display_bms_write_info_completion(fbe_packet_completion_context_t context);


/*!**************************************************************
 *  fbe_cli_disk_display_info()
 ****************************************************************
 @brief
 *  This function displays drive info based on object id.
 *
 * @param object_id  - object id
 * @param context    - in this case we do not use the context.
 * @param package_id - package id of that object.
 *
 * @return fbe_status_t status of the operation.
 *
 * @author
 *  
 *
 ****************************************************************/
static fbe_status_t fbe_cli_disk_display_info(fbe_u32_t object_id, fbe_u32_t context,fbe_package_id_t package_id)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_const_class_info_t *class_info_p =NULL;
    fbe_job_service_bes_t phy_disk = {FBE_PORT_NUMBER_INVALID, FBE_PORT_NUMBER_INVALID, FBE_SLOT_NUMBER_INVALID};
    fbe_esp_drive_mgmt_drive_info_t drive_information = {0};
    fbe_u32_t drive_obj_id = FBE_OBJECT_ID_INVALID;
    fbe_api_provision_drive_info_t provision_drive_info = {0};
    fbe_physical_drive_information_t physical_drive_info = {0}; 
    fbe_fru_signature_t disk_signature = {0};
    // Make a copy of PVD object ID and status in case of get disk signature failure. They might be used elsewhere
    fbe_bool_t          disk_signature_read_success = TRUE;
    fbe_status_t        status_signature_read = FBE_STATUS_OK;
    fbe_u32_t           drive_obj_id_signature_read = FBE_OBJECT_ID_INVALID;
    fbe_bool_t          pvd_get_obj_valid = FALSE;
    fbe_bool_t          b_print_physical_info = FBE_FALSE;
    fbe_u32_t           drive_writes_per_day = 0;

    /* Make sure this is a valid object.
     */
    status = fbe_cli_get_class_info(object_id,package_id,&class_info_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_error("Can't get object class information for id [0x%X],package id:0x%X status: 0x%X\n",object_id,package_id 
        ,status);
        return status;
    }
    
    /* Fetch the port enclosure & slot value
     */
    status = fbe_cli_get_bus_enclosure_slot_info(object_id, 
                                                 class_info_p->class_id,
                                                 &phy_disk.bus,
                                                 &phy_disk.enclosure,
                                                 &phy_disk.slot,
                                                 package_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_error("Can't get bus, enclosure, slot information for object id [0x%X], class id [0x%X], package id:0x%x status:0x%X\n",object_id,class_info_p->class_id,package_id, status);
        return status;
    }

    status = fbe_api_physical_drive_get_drive_information(object_id, &physical_drive_info, FBE_PACKET_FLAG_NO_ATTRIB);

    if (status == FBE_STATUS_OK)
    {
        drive_writes_per_day = (fbe_u32_t)(physical_drive_info.drive_parameter_flags & FBE_PDO_FLAG_FLASH_DRIVE_WRITES_PER_DAY);
        if((physical_drive_info.drive_parameter_flags & FBE_PDO_FLAG_FLASH_DRIVE) || 
                                 (physical_drive_info.drive_parameter_flags & FBE_PDO_FLAG_MLC_FLASH_DRIVE) ||
                                 (physical_drive_info.drive_RPM == 0))
        {
            b_print_physical_info = FBE_TRUE;
        }
    }
    else
    {
        fbe_cli_error("\nERROR: failed status 0x%x to get physical drive information for object_id: 0x%x\n",
                      status, object_id);
    }

    // If this function fail, we should continue instead of bail out. Just skip printing info depending on provision_drive_info 
    status = fbe_api_provision_drive_get_obj_id_by_location_from_topology(phy_disk.bus, phy_disk.enclosure, phy_disk.slot, &drive_obj_id);
    if (status == FBE_STATUS_OK)
    {
        status = fbe_api_provision_drive_get_info(drive_obj_id, &provision_drive_info);
        if (status == FBE_STATUS_OK)
        {
            pvd_get_obj_valid = TRUE;
        }
        else
        {
            fbe_cli_error("\nFailed to get PVD information for object id:0x%x status: 0x%x\n",drive_obj_id, status);
        }
    }
    else
    {
        fbe_cli_error("\nFailed to get provision drive object id for %d_%d_%d status:0x%x\n",
                       phy_disk.bus,phy_disk.enclosure,phy_disk.slot,status);
    }

    drive_information.location.bus = phy_disk.bus;
    drive_information.location.enclosure= phy_disk.enclosure;
    drive_information.location.slot = phy_disk.slot;

    /* Get and display the fru signature of this disk*/
    disk_signature.bus = phy_disk.bus;
    disk_signature.enclosure= phy_disk.enclosure;
    disk_signature.slot= phy_disk.slot;
    status = fbe_api_database_get_disk_signature_info(&disk_signature);
    if(status != FBE_STATUS_OK)
    {
        // AR528533 fix: Even we are unable to get disk signature, we still want to move on.
        // Set the flag and process it later
        disk_signature_read_success = FALSE;
        status_signature_read = status;
        drive_obj_id_signature_read = drive_obj_id;
    }

    /* Get and display the physical drive attributes.
     */
    status = fbe_api_esp_drive_mgmt_get_drive_info(&drive_information);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_error("Can't get drive information for %d_%d_%d, status:0x%X\n",phy_disk.bus,phy_disk.enclosure,phy_disk.slot, status);
        return status;
    }
    
    fbe_cli_printf("\n--------------------------------------------------------------------------------\n");
    fbe_cli_printf("\n");
    fbe_cli_printf("Disk - %d_%d_%d ",phy_disk.bus,phy_disk.enclosure,phy_disk.slot);
    fbe_cli_printf("[Bus %d Encl %d Disk %d]\n",phy_disk.bus,phy_disk.enclosure,phy_disk.slot);
    fbe_cli_printf("\n");

    fbe_cli_printf("Object id:           0x%x\n", object_id);
    fbe_cli_printf("Drive Detail:        %s\n", drive_information.drive_description_buff);

    fbe_cli_printf("  -------------------------------------\n");
    fbe_cli_printf("  Physical Drive Attributes:\n");
    fbe_cli_printf("  -------------------------------------\n");
    fbe_cli_printf("   Vendor ID:                %s\n", drive_information.vendor_id);
    fbe_cli_printf("   Product ID:               %s\n", drive_information.product_id);
    fbe_cli_printf("   Product Rev:              %s\n", drive_information.product_rev);
    fbe_cli_printf("   Product Sr.No.:           %s\n", drive_information.product_serial_num);
    fbe_cli_printf("   Drive Capacity:           0x%llX\n", (unsigned long long)drive_information.gross_capacity);
    fbe_cli_printf("   Drive TLA info:           %s\n", drive_information.tla_part_number);
    if (b_print_physical_info == FBE_TRUE)
    {
        fbe_cli_printf("   Drive Writes Per Day:     %d\n", drive_writes_per_day); 
    }
    else
    {
        fbe_cli_printf("   Drive Writes Per Day:     N/A\n");
    }

    /* Drive Class is an enumerated type - print raw and interpreted value */
    switch(drive_information.drive_type)
    {
        case FBE_DRIVE_TYPE_FIBRE:
            fbe_cli_printf("   Drive Type:               0x%X  (%s)\n", drive_information.drive_type, "Fibre Channel");
            break;

         case FBE_DRIVE_TYPE_SAS:
            fbe_cli_printf("   Drive Type:               0x%X (%s)\n", drive_information.drive_type, "SAS");
            break;
            
         case FBE_DRIVE_TYPE_SATA:
            fbe_cli_printf("   Drive Type:               0x%X (%s)\n", drive_information.drive_type, "SATA");
            break;
            
         case FBE_DRIVE_TYPE_SAS_FLASH_HE:
            fbe_cli_printf("   Drive Type:               0x%X (%s)\n", drive_information.drive_type, "SAS FLASH HE");
            break;
            
         case FBE_DRIVE_TYPE_SATA_FLASH_HE:
            fbe_cli_printf("   Drive Type:               0x%X (%s)\n", drive_information.drive_type, "SATA FLASH HE");
            break;
            
        case FBE_DRIVE_TYPE_SAS_FLASH_ME:
            fbe_cli_printf("   Drive Type:               0x%X (%s)\n", drive_information.drive_type, "SAS FLASH ME");
            break;            
            
        case FBE_DRIVE_TYPE_SAS_FLASH_LE:
            fbe_cli_printf("   Drive Type:               0x%X (%s)\n", drive_information.drive_type, "SAS FLASH LE");
            break;            
            
        case FBE_DRIVE_TYPE_SAS_FLASH_RI:
            fbe_cli_printf("   Drive Type:               0x%X (%s)\n", drive_information.drive_type, "SAS FLASH RI");
            break;            
            
        case FBE_DRIVE_TYPE_SAS_NL:
            fbe_cli_printf("   Drive Type:               0x%X (%s)\n", drive_information.drive_type, "SAS NL");
            break;
            
        case FBE_DRIVE_TYPE_SAS_SED:
            fbe_cli_printf("   Drive Type:               0x%X (%s)\n", drive_information.drive_type, "SAS SED");
			break;
		
		default:
            fbe_cli_printf("   Drive Type:               0x%X (%s)\n", drive_information.drive_type, "INVALID");
            break;    
    }

    fbe_cli_printf("   Block Count:              0x%llX\n",(unsigned long long)drive_information.block_count);
    fbe_cli_printf("   Block Size:               0x%X\n", drive_information.block_size);
    fbe_cli_printf("   Drive Qdepth:             %d\n", drive_information.drive_qdepth);
    fbe_cli_printf("   Drive RPM:                %d\n", drive_information.drive_RPM);    
    fbe_cli_printf("   Spindown HW Qualified(DH):%s\n", drive_information.spin_down_qualified? "YES ": "NO");
    fbe_cli_printf("   Bridge H/W Rev:           0x%X\n", (unsigned int)drive_information.bridge_hw_rev);
    fbe_cli_printf("   PDO Lifecycle state:      %d\n", drive_information.state);
    fbe_cli_printf("   Death Reason:             %d\n", drive_information.death_reason);
    status =fbe_cli_disk_display_info_metadata(phy_disk.bus,phy_disk.enclosure,phy_disk.slot);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_error("\nFailed status 0x%x while getting metadata for %d_%d_%d\n",status, phy_disk.bus,phy_disk.enclosure,phy_disk.slot);
        return status;
    }

    fbe_cli_printf("   Spinning Ticks(min):      %d\n", drive_information.spin_up_time_in_minutes);
    fbe_cli_printf("   Standby Ticks(min):       %d\n", drive_information.stand_by_time_in_minutes);
    fbe_cli_printf("   Number of Spinups:        %d\n", drive_information.spin_up_count);

    /* Speed capability is an enumerated value - print raw and interpreted value */
    switch(drive_information.speed_capability)
    {
        case FBE_DRIVE_SPEED_1_5_GB:
            fbe_cli_printf("   Speed Capability:         0x%X (1.5-GB)\n", drive_information.speed_capability);
            break;

        case FBE_DRIVE_SPEED_2GB:
            fbe_cli_printf("   Speed Capability:         0x%X (2-GB)\n", drive_information.speed_capability);
            break;
            
        case FBE_DRIVE_SPEED_3GB:
            fbe_cli_printf("   Speed Capability:         0x%X (3-GB)\n", drive_information.speed_capability);
            break;
            
        case FBE_DRIVE_SPEED_4GB:
            fbe_cli_printf("   Speed Capability:         0x%X (4-GB)\n", drive_information.speed_capability);
            break;
            
        case FBE_DRIVE_SPEED_6GB:
            fbe_cli_printf("   Speed Capability:         0x%X (6-GB)\n", drive_information.speed_capability);
            break;
            
        case FBE_DRIVE_SPEED_8GB:
            fbe_cli_printf("   Speed Capability:         0x%X (8-GB)\n", drive_information.speed_capability);
            break;
            
        case FBE_DRIVE_SPEED_10GB:
            fbe_cli_printf("   Speed Capability:         0x%X (10-GB)\n", drive_information.speed_capability);
            break;

        case FBE_DRIVE_SPEED_12GB:
            fbe_cli_printf("   Speed Capability:         0x%X (12-GB)\n", drive_information.speed_capability);
            break;

        default:
            /* Display Unknown for any invalid speed capability*/
            fbe_cli_printf("   Speed Capability:          %d (Unknown)\n", drive_information.speed_capability);
            break;    
    }
    
    /* Drive logical state is an enumerated type - print raw and interpreted value */
    switch(drive_information.logical_state)
    {
        case FBE_BLOCK_TRANSPORT_LOGICAL_STATE_UNINITALIZED:
            fbe_cli_printf("   Logical state:            0x%X  (%s)\n", drive_information.logical_state, "Uninitalized");
            break;
            
        case FBE_BLOCK_TRANSPORT_LOGICAL_STATE_ONLINE:
            fbe_cli_printf("   Logical state:            0x%X (%s)\n", drive_information.logical_state, "Online");
            break;
            
        case FBE_BLOCK_TRANSPORT_LOGICAL_STATE_FAILED_EOL:
            fbe_cli_printf("   Logical state:            0x%X (%s)\n", drive_information.logical_state, "EOL");
            break;
            
        case FBE_BLOCK_TRANSPORT_LOGICAL_STATE_FAILED_LINK_FAULT:
            fbe_cli_printf("   Logical state:            0x%X (%s)\n", drive_information.logical_state, "Link Fault");
            break;
            
        case FBE_BLOCK_TRANSPORT_LOGICAL_STATE_FAILED_DRIVE_FAULT:
            fbe_cli_printf("   Logical state:            0x%X (%s)\n", drive_information.logical_state, "Drive Fault");
            break;

        case FBE_BLOCK_TRANSPORT_LOGICAL_STATE_FAILED_NON_EQ:
            fbe_cli_printf("   Logical state:            0x%X (%s)\n", drive_information.logical_state, "NON EQ");
            break;

        case FBE_BLOCK_TRANSPORT_LOGICAL_STATE_FAILED_INVALID_IDENTITY:
            fbe_cli_printf("   Logical state:            0x%X (%s)\n", drive_information.logical_state, "INV_ID");
            break;

        case FBE_BLOCK_TRANSPORT_LOGICAL_STATE_FAILED_SSD_LE:
            fbe_cli_printf("   Logical state:            0x%X (%s)\n", drive_information.logical_state, "SSD LE");
            break;

        case FBE_BLOCK_TRANSPORT_LOGICAL_STATE_FAILED_SSD_RI:
            fbe_cli_printf("   Logical state:            0x%X (%s)\n", drive_information.logical_state, "SSD RI");
            break;

        case FBE_BLOCK_TRANSPORT_LOGICAL_STATE_FAILED_HDD_520:
            fbe_cli_printf("   Logical state:            0x%X (%s)\n", drive_information.logical_state, "HDD 520");
            break;

        case FBE_BLOCK_TRANSPORT_LOGICAL_STATE_FAILED_LESS_12G_LINK:
            fbe_cli_printf("   Logical state:            0x%X (%s)\n", drive_information.logical_state, "Less 12G Link Rate");
            break;

        case FBE_BLOCK_TRANSPORT_LOGICAL_STATE_FAILED_OTHER:
            fbe_cli_printf("   Logical state:            0x%X (%s)\n", drive_information.logical_state, "Other");
            break;            
            
        default:
            fbe_cli_printf("   Logical state:            0x%X (%s)\n", drive_information.logical_state, "INVALID");
            break;    
    }

    fbe_cli_printf("   Additional info:          %s\n", drive_information.dg_part_number_ascii);
    fbe_cli_printf("   Time since last I/O:      %llu Seconds ago\n",
           (unsigned long long)(drive_information.last_log/100));
          
    if (pvd_get_obj_valid)
    {

        if(provision_drive_info.config_type == FBE_PROVISION_DRIVE_CONFIG_TYPE_TEST_RESERVED_SPARE ||provision_drive_info.config_type == FBE_PROVISION_DRIVE_CONFIG_TYPE_RAID)
        {
            fbe_cli_printf("   Status:                   %s\n","BOUND");
        }
        else
        {
           fbe_cli_printf("   Status:                   %s\n","UNBOUND");
        }

         /* Get and display the logical attributes.
         */
        fbe_cli_printf("\n  -------------------------------------\n");
        fbe_cli_printf("  Logical Attributes:\n");
        if(provision_drive_info.config_type == FBE_PROVISION_DRIVE_CONFIG_TYPE_TEST_RESERVED_SPARE)
        {
            fbe_cli_printf("  -------------------------------------\n");
            fbe_cli_printf("   Configured as Hotspare   \n");
            fbe_cli_printf("  -------------------------------------\n");
        }
        else if(provision_drive_info.config_type == FBE_PROVISION_DRIVE_CONFIG_TYPE_RAID)
        {
            status = fbe_cli_disk_display_info_logical_attributes(phy_disk.bus,phy_disk.enclosure,phy_disk.slot);
             
            if (status != FBE_STATUS_OK)
            {
                /*Above function displayed an error*/
                return status;
            }
        }
        else
        {
            fbe_cli_printf("  -------------------------------------\n");
            fbe_cli_printf("   No Luns present !!\n");
            fbe_cli_printf("  -------------------------------------\n");
        }
    }
    else
    {
        fbe_cli_printf("  -------------------------------------\n");
        fbe_cli_printf("   PVD Not Available. Lun info, Logical Attibutes and PVD status no valid!\n");
        fbe_cli_printf("  -------------------------------------\n");
    }

    fbe_cli_printf("\n  -------------------------------------\n");
    fbe_cli_printf("  FRU SIGNATURE:\n");
    {
        if(disk_signature_read_success == TRUE)
        {
            fbe_cli_printf("   *Magic String: %s\n",disk_signature.magic_string);
            fbe_cli_printf("   *Version: %d\n",disk_signature.version);
            fbe_cli_printf("   *WWN SEED: 0x%llX\n",(unsigned long long)disk_signature.system_wwn_seed);
            fbe_cli_printf("   *Location: %d_%d_%d\n",disk_signature.bus, disk_signature.enclosure, disk_signature.slot);
         
        }
        else
        {
            fbe_cli_printf("   *Failed to get fru signature for PVD object id:0x%x status: 0x%x.\n",drive_obj_id_signature_read, status_signature_read);
            // Assign with invalid values
            fbe_zero_memory(disk_signature.magic_string, FBE_FRU_SIGNATURE_MAGIC_STRING_SIZE); 
            fbe_copy_memory(disk_signature.magic_string, "Not available", (fbe_u32_t)(FBE_MIN(strlen("Not available"), FBE_FRU_SIGNATURE_MAGIC_STRING_SIZE))); 
            fbe_cli_printf("   *Magic String: %s\n",disk_signature.magic_string);
        }
    }    
    fbe_cli_printf("\n  -------------------------------------\n");
    
    return status;
}
/******************************************
 * end  fbe_cli_disk_display_info()
 ******************************************/

/*!**************************************************************
 *  fbe_cli_disk_phys_display_info()
 ****************************************************************
 @brief
 *  This function displays only Physical drive info based on object id.
 *
 * @param object_id  - object id
 * @param context    - in this case we do not use the context.
 * @param package_id - package id of that object.
 *
 * @return fbe_status_t status of the operation.
 *
 * @author
 *  
 *
 ****************************************************************/
static fbe_status_t fbe_cli_disk_phys_display_info(fbe_u32_t object_id, fbe_u32_t context,fbe_package_id_t package_id)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_const_class_info_t          *class_info_p =NULL;
    fbe_job_service_bes_t           phy_disk = {FBE_PORT_NUMBER_INVALID, FBE_PORT_NUMBER_INVALID, FBE_SLOT_NUMBER_INVALID};
    fbe_esp_drive_mgmt_drive_info_t drive_information = {0};
    char                            *lifecycle_state_str = NULL;
    char                            *death_reason_str = NULL;


    // Make sure this is a valid object.
    status = fbe_cli_get_class_info(object_id,package_id,&class_info_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_error("Can't get object class information for id [0x%X],package id:0x%X status: 0x%X\n",object_id,package_id 
        ,status);
        return status;
    }
    
    // Fetch the port enclosure & slot value
    status = fbe_cli_get_bus_enclosure_slot_info(object_id, 
                                                 class_info_p->class_id,
                                                 &phy_disk.bus,
                                                 &phy_disk.enclosure,
                                                 &phy_disk.slot,
                                                 package_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_error("Can't get bus, enclosure, slot information for object id [0x%X], class id [0x%X], package id:0x%x status:0x%X\n",object_id,class_info_p->class_id,package_id, status);
        return status;
    }

    drive_information.location.bus = phy_disk.bus;
    drive_information.location.enclosure= phy_disk.enclosure;
    drive_information.location.slot = phy_disk.slot;


    // Get and display the physical drive attributes.
    status = fbe_api_esp_drive_mgmt_get_drive_info(&drive_information);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_error("Can't get drive information for %d_%d_%d, status:0x%X\n",phy_disk.bus,phy_disk.enclosure,phy_disk.slot, status);
        return status;
    }
    
    fbe_cli_printf("\n");
    fbe_cli_printf("Disk:                        %d_%d_%d\n",phy_disk.bus,phy_disk.enclosure,phy_disk.slot);
    fbe_cli_printf("Object ID:                   0x%x\n", object_id);

    fbe_cli_printf("   Vendor ID:                %s\n", drive_information.vendor_id);
    fbe_cli_printf("   Product ID:               %s\n", drive_information.product_id);
    fbe_cli_printf("   Product Rev:              %s\n", drive_information.product_rev);
    fbe_cli_printf("   Product Sr.No.:           %s\n", drive_information.product_serial_num);
    fbe_cli_printf("   Part number:              %s\n", drive_information.dg_part_number_ascii);
    fbe_cli_printf("   TLA info:                 %s\n", drive_information.tla_part_number);
    fbe_cli_printf("   Disk Capacity:            0x%llX\n", (unsigned long long)drive_information.gross_capacity);

    // Drive Class is an enumerated type - print raw and interpreted value
    switch(drive_information.drive_type)
    {
        case FBE_DRIVE_TYPE_FIBRE:
            fbe_cli_printf("   Drive Type:               FBE_DRIVE_TYPE_FIBRE\n");
            break;

         case FBE_DRIVE_TYPE_SAS:
            fbe_cli_printf("   Drive Type:               FBE_DRIVE_TYPE_SAS\n");
            break;
            
         case FBE_DRIVE_TYPE_SATA:
            fbe_cli_printf("   Drive Type:               FBE_DRIVE_TYPE_SATA\n");
            break;
            
         case FBE_DRIVE_TYPE_SAS_FLASH_HE:
            fbe_cli_printf("   Drive Type:               FBE_DRIVE_TYPE_SAS_FLASH_HE\n");
            break;
            
         case FBE_DRIVE_TYPE_SATA_FLASH_HE:
            fbe_cli_printf("   Drive Type:               FBE_DRIVE_TYPE_SATA_FLASH_HE\n");
            break;
            
        case FBE_DRIVE_TYPE_SAS_FLASH_ME:
            fbe_cli_printf("   Drive Type:               FBE_DRIVE_TYPE_SAS_FLASH_ME\n");
            break;

        case FBE_DRIVE_TYPE_SAS_FLASH_RI:
            fbe_cli_printf("   Drive Type:               FBE_DRIVE_TYPE_SAS_FLASH_RI\n");
            break;            
            
        case FBE_DRIVE_TYPE_SAS_NL:
            fbe_cli_printf("   Drive Type:               FBE_DRIVE_TYPE_SAS_NL\n");
            break;
        
        case FBE_DRIVE_TYPE_SATA_PADDLECARD:
            fbe_cli_printf("   Drive Type:               FBE_DRIVE_TYPE_SATA_PADDLECARD\n");
            break;

        default:
            fbe_cli_printf("   Drive Type:               FBE_DRIVE_TYPE_INVALID\n");
            break;    
    }

    fbe_cli_printf("   Block Count:              0x%llX\n",(unsigned long long)drive_information.block_count);
    fbe_cli_printf("   Block Size:               0x%X\n", drive_information.block_size);
    fbe_cli_printf("   Drive Qdepth:             %d\n", drive_information.drive_qdepth);
    fbe_cli_printf("   Drive RPM:                %d\n", drive_information.drive_RPM);    
    fbe_cli_printf("   Spindown Qualified:       %s\n", drive_information.spin_down_qualified? "YES ": "NO");
    fbe_cli_printf("   Bridge H/W Rev:           0x%X\n", (unsigned int)drive_information.bridge_hw_rev);

    // Translate lifecycle state to string
    lifecycle_state_str = fbe_cli_map_lifecycle_state_to_str(drive_information.state);
    fbe_cli_printf("   PDO Lifecycle state:      %s\n", lifecycle_state_str);

    // Get death reason
    death_reason_str = fbe_api_esp_drive_mgmt_map_death_reason_to_str(drive_information.death_reason);
    fbe_cli_printf("   Death Reason:             %s\n", death_reason_str);

    // Display power saving info
    status = fbe_cli_disk_display_info_power_saving(phy_disk.bus,phy_disk.enclosure,phy_disk.slot);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_error("\nFailed status 0x%x while getting power saving for %d_%d_%d\n",status, phy_disk.bus,phy_disk.enclosure,phy_disk.slot);
        return status;
    }

    fbe_cli_printf("   Spinning Ticks:           %d\n", drive_information.spin_up_time_in_minutes);
    fbe_cli_printf("   Standby Ticks:            %d\n", drive_information.stand_by_time_in_minutes);
    fbe_cli_printf("   Spinup count:             %d\n", drive_information.spin_up_count);

    // Speed capability is an enumerated value - print raw and interpreted value 
    switch(drive_information.speed_capability)
    {
        case FBE_DRIVE_SPEED_1_5_GB:
            fbe_cli_printf("   Speed Capability:         FBE_DRIVE_SPEED_1_5_GB)\n");
            break;

        case FBE_DRIVE_SPEED_2GB:
            fbe_cli_printf("   Speed Capability:         FBE_DRIVE_SPEED_2GB\n");
            break;
            
        case FBE_DRIVE_SPEED_3GB:
            fbe_cli_printf("   Speed Capability:         FBE_DRIVE_SPEED_3GB\n");
            break;
            
        case FBE_DRIVE_SPEED_4GB:
            fbe_cli_printf("   Speed Capability:         FBE_DRIVE_SPEED_4GB\n");
            break;
            
        case FBE_DRIVE_SPEED_6GB:
            fbe_cli_printf("   Speed Capability:         FBE_DRIVE_SPEED_6GB\n");
            break;
            
        case FBE_DRIVE_SPEED_8GB:
            fbe_cli_printf("   Speed Capability:         FBE_DRIVE_SPEED_8GB\n");
            break;
            
        case FBE_DRIVE_SPEED_10GB:
            fbe_cli_printf("   Speed Capability:         FBE_DRIVE_SPEED_10GB\n");
            break;

        case FBE_DRIVE_SPEED_12GB:
            fbe_cli_printf("   Speed Capability:         FBE_DRIVE_SPEED_12GB\n");
            break;

        case FBE_DRIVE_SPEED_UNKNOWN:
            fbe_cli_printf("   Speed Capability:         FBE_DRIVE_SPEED_UNKNOWN\n");
            break;

        default:
            // Display Unknown for any invalid speed capability
            fbe_cli_printf("   Speed Capability:         %d\n", drive_information.speed_capability);
            break;    
    }
    
    // Drive logical state is an enumerated type - print raw and interpreted value
    switch(drive_information.logical_state)
    {
        case FBE_BLOCK_TRANSPORT_LOGICAL_STATE_UNINITALIZED:
            fbe_cli_printf("   Logical state:            FBE_BLOCK_TRANSPORT_LOGICAL_STATE_UNINITALIZED\n");
            break;
            
        case FBE_BLOCK_TRANSPORT_LOGICAL_STATE_ONLINE:
            fbe_cli_printf("   Logical state:            FBE_BLOCK_TRANSPORT_LOGICAL_STATE_ONLINE\n");
            break;
            
        case FBE_BLOCK_TRANSPORT_LOGICAL_STATE_FAILED_EOL:
            fbe_cli_printf("   Logical state:            FBE_BLOCK_TRANSPORT_LOGICAL_STATE_FAILED_EOL\n");
            break;
            
        case FBE_BLOCK_TRANSPORT_LOGICAL_STATE_FAILED_LINK_FAULT:
            fbe_cli_printf("   Logical state:            FBE_BLOCK_TRANSPORT_LOGICAL_STATE_FAILED_LINK_FAULT\n");
            break;
            
        case FBE_BLOCK_TRANSPORT_LOGICAL_STATE_FAILED_DRIVE_FAULT:
            fbe_cli_printf("   Logical state:            FBE_BLOCK_TRANSPORT_LOGICAL_STATE_FAILED_DRIVE_FAULT\n");
            break;

        case FBE_BLOCK_TRANSPORT_LOGICAL_STATE_FAILED_NON_EQ:
            fbe_cli_printf("   Logical state:            FBE_BLOCK_TRANSPORT_LOGICAL_STATE_FAILED_NON_EQ\n");
            break;

        case FBE_BLOCK_TRANSPORT_LOGICAL_STATE_FAILED_INVALID_IDENTITY:
            fbe_cli_printf("   Logical state:            FBE_BLOCK_TRANSPORT_LOGICAL_STATE_FAILED_INVALID_IDENTITY\n");
            break;

        case FBE_BLOCK_TRANSPORT_LOGICAL_STATE_FAILED_SSD_LE:
            fbe_cli_printf("   Logical state:            FBE_BLOCK_TRANSPORT_LOGICAL_STATE_FAILED_SSD_LE\n");
            break;

        case FBE_BLOCK_TRANSPORT_LOGICAL_STATE_FAILED_SSD_RI:
            fbe_cli_printf("   Logical state:            FBE_BLOCK_TRANSPORT_LOGICAL_STATE_FAILED_SSD_RI\n");
            break;

        case FBE_BLOCK_TRANSPORT_LOGICAL_STATE_FAILED_HDD_520:
            fbe_cli_printf("   Logical state:            FBE_BLOCK_TRANSPORT_LOGICAL_STATE_FAILED_HDD_520\n");
            break;

        case FBE_BLOCK_TRANSPORT_LOGICAL_STATE_FAILED_LESS_12G_LINK:
            fbe_cli_printf("   Logical state:            FBE_BLOCK_TRANSPORT_LOGICAL_STATE_FAILED_LESS_12G_LINK\n");
            break;

        case FBE_BLOCK_TRANSPORT_LOGICAL_STATE_FAILED_OTHER:
            fbe_cli_printf("   Logical state:            FBE_BLOCK_TRANSPORT_LOGICAL_STATE_FAILED_OTHER\n");
            break;            
            
        default:
            fbe_cli_printf("   Logical state:            %s (0x%X)\n", "UNKNOWN LOGICAL STATE", drive_information.logical_state);
            break;    
    }

    fbe_cli_printf("   Time since last I/O:      %llu\n",(unsigned long long)(drive_information.last_log/100));
          

    return status;
}
/******************************************
 * end  fbe_cli_disk_phys_display_info()
 ******************************************/

/*!**************************************************************
 * fbe_cli_disk_display_info_logical_attributes()
 ****************************************************************
 * @brief
 *   This function displays logical attributes of a physical drive.
 *  
 *  @param    port - port number of drive
 *  @param    enclosure -enclosure number of drive
 *  @param    slot - slot number of drive
 *
 * @return - fbe_status_t status of the operation.
 *
 * @author
 *  
 *
 ****************************************************************/
static fbe_status_t fbe_cli_disk_display_info_logical_attributes(fbe_u32_t port,fbe_u32_t enclosure,fbe_u32_t slot)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t    rg_index = 0, lun_index = 0,lun_count = 0;
    fbe_object_id_t             provision_object_id = FBE_OBJECT_ID_INVALID;
    fbe_class_id_t class_id = FBE_CLASS_ID_INVALID;
    fbe_cli_lurg_rg_details_t   rg_details = {0};
    fbe_cli_lurg_lun_details_t  lun_details = {0};
    fbe_api_base_config_upstream_object_list_t  upstream_vd_object_list = {0};
    fbe_api_base_config_upstream_object_list_t  upstream_rd_object_list = {0};
    fbe_api_base_config_upstream_object_list_t  upstream_object_list = {0};
    fbe_api_base_config_upstream_object_list_t  upstream_lun_object_list = {0};
    fbe_provision_drive_get_verify_status_t     verify_status = {0};
    fbe_bool_t                                  system_sniff_verify_enabled;

    status = fbe_api_provision_drive_get_obj_id_by_location_from_topology(port, enclosure, slot,&provision_object_id);
    if(status != FBE_STATUS_OK)
    {
        fbe_cli_error("\nFail to get PVD object id for %d_%d_%d,status:0x%x,Function:%s",port, enclosure, slot,status,__FUNCTION__);
        return status;
    }

    status = fbe_api_provision_drive_get_verify_status(provision_object_id,FBE_PACKET_FLAG_NO_ATTRIB, &verify_status);

    if (status != FBE_STATUS_OK)
    {
        fbe_cli_error("Unable to get Sniff Verify Status for disk %d_%d_%d.  Error status %d\n\n",
                                port, enclosure, slot,status);
        return status;
    }

    system_sniff_verify_enabled = fbe_api_system_sniff_verify_is_enabled();
    fbe_cli_printf("     Sniff Verify is %s.", (verify_status.verify_enable)? "Enabled":"Disabled");
    fbe_cli_printf("     System sniff verify is %s.", (system_sniff_verify_enabled)? "Enabled":"Disabled");

    if(verify_status.verify_enable == FBE_TRUE)
    {
        fbe_cli_printf("\n      Sniff Verify Checkpoint: 0x%llX", (unsigned long long)verify_status.verify_checkpoint);
        fbe_cli_printf("\n      Sniff Verify Exported Capacity: 0x%llX", (unsigned long long)verify_status.exported_capacity);
    }
    fbe_cli_printf("\n");

    /* Get the upstream vd object list 
    */
    status = fbe_api_base_config_get_upstream_object_list(provision_object_id, &upstream_vd_object_list);
    if(status != FBE_STATUS_OK)
    {
        fbe_cli_error("\nFail to upstream VD Object list for PVD object id :0x%x ,status:0x%x,Function:%s",provision_object_id,status,__FUNCTION__);
        return status;
    }

    if(upstream_vd_object_list.number_of_upstream_objects == 1)
    {
        /* Get the upstream Raid object list 
        */
        status = fbe_api_base_config_get_upstream_object_list(upstream_vd_object_list.upstream_object_list[0], &upstream_rd_object_list);
        if(status != FBE_STATUS_OK)
        {
            fbe_cli_error("\nFail to upstream Raid Group Object list for VD object id :0x%x ,status:0x%x,Function:%s",upstream_vd_object_list.upstream_object_list[0],status,__FUNCTION__);
            return status;
        }
        for (rg_index = 0; rg_index< upstream_rd_object_list.number_of_upstream_objects; rg_index++)
        {

            if(upstream_rd_object_list.upstream_object_list[rg_index] == FBE_OBJECT_ID_INVALID )
            {
                continue;
            }
            /* Get the upstream Lun object list 
            */
            status = fbe_api_base_config_get_upstream_object_list(upstream_rd_object_list.upstream_object_list[rg_index], &upstream_object_list);
            if(status != FBE_STATUS_OK)
            {
                fbe_cli_error("\nFail to upstream Object list for Raid Group object id :0x%x ,status:0x%x,Function:%s",upstream_rd_object_list.upstream_object_list[rg_index],status,__FUNCTION__);
                return status;
            }

            /*if no lun present on raid group no need to go for lun checking */
            if(upstream_object_list.number_of_upstream_objects == 0)
            {
                break;
            }

            status = fbe_api_get_object_class_id(upstream_object_list.upstream_object_list[0], &class_id, FBE_PACKAGE_ID_SEP_0);

            /*checking class id for for special case of raid1_0*/
            if(class_id != FBE_CLASS_ID_LUN)
            {
                /* Get the upstream Lun object list for raid1_0
                */
                status = fbe_api_base_config_get_upstream_object_list(upstream_object_list.upstream_object_list[0], &upstream_lun_object_list);
                if(status != FBE_STATUS_OK)
                {
                    fbe_cli_error("\nFail to upstream LUN Object list for Raid Group object id :0x%x ,status:0x%x,Function:%s",upstream_object_list.upstream_object_list[0],status,__FUNCTION__);
                    return status;
                }
                /*discard other raid group as this is raid1_0 case*/
                upstream_rd_object_list.number_of_upstream_objects = 1;
            }
            else
            {
                upstream_lun_object_list = upstream_object_list;
            }
            for (lun_index= 0; lun_index < upstream_lun_object_list.number_of_upstream_objects; lun_index++)
            {
                /* get the LUN details based on object ID of LUN */
                status = fbe_cli_get_lun_details(upstream_lun_object_list.upstream_object_list[lun_index],&lun_details);
                if (status != FBE_STATUS_OK)
                {
                    fbe_cli_error("Failed to get the LUN Detail for an Object id:0x%x, Status:0x%x",upstream_object_list.upstream_object_list[lun_index],status);
                    return status;
                }

                /* We only want downstream objects for this RG indicate 
                * that by setting corresponding variable in structure*/
                rg_details.direction_to_fetch_object = DIRECTION_DOWNSTREAM;

                /* Get the details of RG associated with this LUN */
                status = fbe_cli_get_rg_details(upstream_lun_object_list.upstream_object_list[lun_index],&rg_details);
                if (status != FBE_STATUS_OK)
                {
                    fbe_cli_error("Failed to get RG detail associated with this LUN of an Object id:0x%x, Status:0x%x",upstream_lun_object_list.upstream_object_list[lun_index],status);
                    return status;
                }

                status = fbe_cli_display_disk_lun_stat(&lun_details,&rg_details);
                if (status != FBE_STATUS_OK)
                {
                    /*Above function displayed an error*/
                    return status;
                }
                ++lun_count;
            }
        }
    }

    if(lun_count == 0)
    {
        fbe_cli_printf("  -------------------------------------\n");
        fbe_cli_printf("   No Luns present !!\n");
    }

    fbe_cli_printf("  -------------------------------------\n");

    return status;
}
/******************************************
 * end  fbe_cli_disk_display_info_logical_attributes()
 ******************************************/
 
/*!**************************************************************
 * fbe_cli_display_disk_lun_stat()
 ****************************************************************
 * @brief
 *   This function displays lun detail for specific object id.
 *  
 *  @param    lun_details - lun detail of specific object id.
 *  @param    rg_detail - raid group detail of that lun which it belong 
 *
 * @return - fbe_status_t status of the operation.
 *
 * @author
 *  
 *
 ****************************************************************/
fbe_status_t fbe_cli_display_disk_lun_stat(fbe_cli_lurg_lun_details_t *lun_details, fbe_cli_lurg_rg_details_t *rg_details)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_lba_t         capacity;
    fbe_block_size_t  exported_block_size;
    fbe_u32_t         actual_capacity_of_lun;
    fbe_u32_t         rg_width = 0;
    fbe_u32_t         index;
    fbe_u16_t         reb_percentage;
    fbe_u32_t         wwn_index = 0;
    fbe_u32_t         wwn_size = 0;

    
    fbe_cli_printf("  -------------------------------------\n");
    /* Add LUN number to output */
    fbe_cli_printf( "   LUN number: %d\n", lun_details->lun_number);

    /* Add RG number to output */
    fbe_cli_printf( "     RG number:          %d\n", rg_details->rg_number);

    /* Add LUN lifecycle state to output */
    fbe_cli_printf( "     LUN lifecycle:      %s\n", lun_details->p_lifecycle_state_str);

    /* Add LUN world wide number to output */
    wwn_size = sizeof((lun_details->lun_info).world_wide_name.bytes);
    fbe_cli_printf( "     LUN WWN:            ");
    for(wwn_index = 0; wwn_index < wwn_size; ++wwn_index)
    {
        fbe_cli_printf("%02x",(fbe_u32_t)((lun_details->lun_info).world_wide_name.bytes[wwn_index]));
        fbe_cli_printf(":");    
    }
    fbe_cli_printf("\n");

    /* Add RAID type to output */
    fbe_cli_printf("     RG type:            %s\n", lun_details->p_rg_str);
    
    capacity =  lun_details->lun_info.capacity;
    exported_block_size = lun_details->lun_info.raid_info.exported_block_size;

    /* Convert the capacity to Mega Bytes */
    actual_capacity_of_lun = (fbe_u32_t)((capacity * exported_block_size)/(1024*1024));

    fbe_cli_printf( "     LUN capacity:       %d MB\n", actual_capacity_of_lun);

    fbe_cli_printf( "     Bind slots:        ");

    if(lun_details->lun_info.raid_info.raid_type == FBE_RAID_GROUP_TYPE_RAID10)
    {
        rg_width = lun_details->lun_info.raid_info.width * 2;
    }
    else 
    {
        rg_width = lun_details->lun_info.raid_info.width;
    }
    /* Print the details of the drives associated with this RG. */
    for (index = 0; index < rg_width; index++)
    {
        /* Add drive number to output */
        fbe_cli_printf( " %d_%d_%d", rg_details->drive_list[index].port_num, 
                                rg_details->drive_list[index].encl_num, rg_details->drive_list[index].slot_num);
    }
    fbe_cli_printf("\n\n");

    /*Print rebuild percentage*/
    for (index = 0; index < lun_details->lun_info.raid_info.width; index++)
    {
        if (lun_details->lun_info.raid_info.rebuild_checkpoint[index] != FBE_LBA_INVALID)
        {

            reb_percentage = fbe_cli_get_drive_rebuild_progress(&rg_details->drive_list[index],lun_details->lun_info.raid_info.rebuild_checkpoint[index]);

            if(reb_percentage == FBE_INVALID_PERCENTAGE)
            {
                return FBE_STATUS_OK;
            }

            fbe_cli_printf( "     Rebuild status: %d%% for disk %d_%d_%d\n",reb_percentage,rg_details->drive_list[index].port_num,
                rg_details->drive_list[index].encl_num, rg_details->drive_list[index].slot_num);

        }
    }
    
    /* Print the details zeroing process */
    if (lun_details->fbe_api_lun_get_zero_status.zero_checkpoint != FBE_LBA_INVALID)
    {
        fbe_cli_printf("     zeroing status: %d%% done\n", lun_details->fbe_api_lun_get_zero_status.zero_percentage);
    }


return status;

}
/******************************************
 * end  fbe_cli_display_disk_lun_stat()
 ******************************************/

/*!**************************************************************
 * fbe_cli_disk_display_info_metadata()
 ****************************************************************
 * @brief
 *   This function displays metadata & power saving informatin
 *   for the disk.
 *  
 *  @param    port - port number of drive
 *  @param    enclosure -enclosure number of drive
 *  @param    slot - slot number of drive
 *
 * @return - fbe_status_t status of the operation. 
 *
 * @author
 *  
 *
 ****************************************************************/
static fbe_status_t fbe_cli_disk_display_info_metadata(fbe_u32_t port,fbe_u32_t enclosure,fbe_u32_t slot)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t object_handle_p = FBE_OBJECT_ID_INVALID;
    fbe_u32_t drive_obj_id = FBE_OBJECT_ID_INVALID;
    fbe_physical_drive_attributes_t attributes = {0};
    fbe_api_provision_drive_info_t provision_drive_info = {0};
    fbe_api_virtual_drive_calculate_capacity_info_t virtual_drive_capacity_info = {0};
    fbe_base_config_object_power_save_info_t power_save_info = {0};

    status = fbe_api_get_physical_drive_object_id_by_location(port, enclosure, slot, &object_handle_p);

    if ((status != FBE_STATUS_OK) || (object_handle_p == FBE_OBJECT_ID_INVALID))
    {
        fbe_cli_error("\nFailed to get physical drive object id for %d_%d_%d  status: 0x%x\n",port,enclosure,slot, status);
        return status;
    }
    /* Get the PDO data.
     */
    status = fbe_api_physical_drive_get_attributes(object_handle_p, &attributes);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_error("\nFailed to get physical drive object attributes for object_id: 0x%x status: 0x%x\n",
                      object_handle_p, status);
        return status;
    }
    if (attributes.maintenance_mode_bitmap != 0)
    {
        fbe_cli_printf("   Maintenance Mode:         0x%X\n", (unsigned int)attributes.maintenance_mode_bitmap);
    }
    fbe_cli_printf("   Disk Capacity:            0x%llX\n", (unsigned long long)attributes.imported_capacity);

    status = fbe_api_provision_drive_get_obj_id_by_location_from_topology(port, enclosure, slot,&drive_obj_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_error("\nFailed to get provision drive status: 0x%x,object id:0x%x\n", status,drive_obj_id);
        return status;
    }
    status = fbe_api_provision_drive_get_info(drive_obj_id,&provision_drive_info);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_error("\nFailed to get provision drive information for object id:0x%x status: 0x%x\n", drive_obj_id, status);
        return status;
    }
    fbe_cli_printf("   PVD Capacity:             0x%llX\n",
           (unsigned long long)provision_drive_info.capacity);
    fbe_cli_printf("   PVD metadata Capacity:    0x%llX\n",
           (unsigned long long)provision_drive_info.paged_metadata_capacity);

    virtual_drive_capacity_info.imported_capacity = provision_drive_info.capacity;
    virtual_drive_capacity_info.block_edge_geometry = FBE_BLOCK_EDGE_GEOMETRY_520_520;
    status = fbe_api_virtual_drive_calculate_capacity(&virtual_drive_capacity_info);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_error("\nFailed to get virtual drive metadata:status: 0x%x\n", status);
        return status;
    }
    fbe_cli_printf("   VD Capacity:              0x%llX\n", 
           (unsigned long long)virtual_drive_capacity_info.exported_capacity);

    status = fbe_api_get_object_power_save_info(drive_obj_id,&power_save_info);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_error("\nFailed status 0x%x to power save info for object_id: 0x%x\n",
                  status, drive_obj_id);
        return status;
    }

    fbe_cli_printf("   Power saving:             %s \n", power_save_info.power_saving_enabled? "Enabled":"Disabled");
    fbe_cli_printf("   Configuration Time:       %llu sec\n",
           (unsigned long long) power_save_info.configured_idle_time_in_seconds);
    fbe_cli_printf("   Hibernation Time:         %llu sec\n",
           (unsigned long long)power_save_info.hibernation_time);

    switch(power_save_info.power_save_state)
    {
        case FBE_POWER_SAVE_STATE_NOT_SAVING_POWER:
            fbe_cli_printf("   Power save state:         0x%X (%s)\n", power_save_info.power_save_state, "Drive not saving power");
            break;

        case FBE_POWER_SAVE_STATE_SAVING_POWER:
            fbe_cli_printf("   Power save state:         0x%X (%s)\n", power_save_info.power_save_state, "Drive saving power ");
            break;
            
        case FBE_POWER_SAVE_STATE_ENTERING_POWER_SAVE:
            fbe_cli_printf("   Power save state:         0x%X (%s)\n", power_save_info.power_save_state, "Drive entering power saving state");
            break;
            
        case FBE_POWER_SAVE_STATE_EXITING_POWER_SAVE:
            fbe_cli_printf("   Power save state:         0x%X (%s)\n", power_save_info.power_save_state, "Drive exiting power saving state");
            break;

        default:
            fbe_cli_printf("   Power save state:         0x%X (%s)\n", power_save_info.power_save_state, "INVALID");
            break;    
    }

    return status;

}
/******************************************
 * end  fbe_cli_disk_display_info_metadata()
 ******************************************/

/*!**************************************************************
 * fbe_cli_disk_display_list()
 ****************************************************************
 * @brief
 *   This function displays list of physical attribute for all drive.
 *
 ****************************************************************/
void fbe_cli_disk_display_list(void)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_package_id_t package_id = FBE_PACKAGE_ID_PHYSICAL;
    fbe_u32_t class_id;

    /* Port, enclosure and slot are not used but necessary to call
     * fbe_cli_execute_command_for_objects(), defaults to all port,
     * enclosure and slot.
     */
    fbe_u32_t port = FBE_API_PHYSICAL_BUS_COUNT;
    fbe_u32_t enclosure = FBE_API_ENCLOSURES_PER_BUS;
    fbe_u32_t slot = FBE_API_ENCLOSURE_SLOTS;

    /* Context for the command.
     */
    fbe_u32_t context = 0;

    status = fbe_cli_execute_command_for_objects(FBE_CLASS_ID_BASE_PHYSICAL_DRIVE,
                                                 package_id,
                                                 port,
                                                 enclosure,
                                                 slot,
                                                 fbe_cli_disk_display_list_attr,
                                                 context/* no context */);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_printf("\nPhysical drive command to all drives failed status: 0x%x.\n", status);
            return;
    }

    fbe_cli_printf("+--------------------------------------------------------------------------------------------+ \n");
    fbe_cli_printf("| FRU    | Vendor       | Model            | Rev. |  Sr.No.      | Capacity   | TLA          |\n");
    fbe_cli_printf("|--------|--------------|------------------|------|--------------|------------|--------------|\n");

    /* Execute the command against all sas physical drives.
     */
    for (class_id = FBE_CLASS_ID_SAS_PHYSICAL_DRIVE_FIRST +1; class_id < FBE_CLASS_ID_SAS_PHYSICAL_DRIVE_LAST; class_id++)
    {
        status = fbe_cli_execute_command_for_objects(class_id,
                                                     package_id,
                                                     port,
                                                     enclosure,
                                                     slot,
                                                     fbe_cli_disk_display_list_attr,
                                                     context/* no context */);

        if (status != FBE_STATUS_OK)
        {
            fbe_cli_printf("\nPhysical drive command to all drives failed status: 0x%x.\n", status);
            return;
        }
    }

#ifdef  BLOCKSIZE_512_SUPPORTED
    /* Execute the command against all sata physical drives.
     */
    for (class_id = FBE_CLASS_ID_SATA_PHYSICAL_DRIVE_FIRST +1; class_id < FBE_CLASS_ID_SATA_PHYSICAL_DRIVE_LAST; class_id++)
    {
        status = fbe_cli_execute_command_for_objects(class_id,
                                                     package_id,
                                                     port,
                                                     enclosure,
                                                     slot,
                                                     fbe_cli_disk_display_list_attr,
                                                     context /* no context */);
        if (status != FBE_STATUS_OK)
        {
            fbe_cli_printf("\nPhysical drive command to all drives failed status: 0x%x.\n", status);
            return;
        }
    }
#endif
    
    fbe_cli_printf("+------------------------------------------------------------------------------------------+ \n");
    
    return;
}
/*!**************************************************************
 * fbe_cli_disk_display_list_attr()
 ****************************************************************
 * @brief
 *   This function displays list of physical attribute of a drive.
 *  
 * @param object_id - the object to display.
 * @param context - in this case we do not use the context.
 * @param package_id -package id of that object.
 *
 * @return fbe_status_t status of the operation.
 *
 * @author
 *  
 *
 ****************************************************************/

static fbe_status_t fbe_cli_disk_display_list_attr(fbe_u32_t object_id, fbe_u32_t context,fbe_package_id_t package_id)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_const_class_info_t *class_info_p =NULL;
    fbe_port_number_t port = FBE_PORT_NUMBER_INVALID;
    fbe_enclosure_number_t encl = FBE_ENCLOSURE_NUMBER_INVALID;
    fbe_enclosure_slot_number_t slot = FBE_ENCLOSURE_SLOT_NUMBER_INVALID;
    fbe_esp_drive_mgmt_drive_info_t drive_information = {0};

     /* Make sure this is a valid object.
     */
    status = fbe_cli_get_class_info(object_id,package_id,&class_info_p);
    if (status != FBE_STATUS_OK)
    {
        /* Just return the above function displayed an error.
         */
        fbe_cli_error("can't get object class information for id [0x%X],package id:0x%X status:0x%X\n",object_id,package_id,status);
        return status;
    }
    
    /* Fetch the port enclose &.slot value
     */
    status = fbe_cli_get_bus_enclosure_slot_info(object_id, 
                                                 class_info_p->class_id,
                                                 &port,
                                                 &encl,
                                                 &slot,
                                                 package_id);
    if (status != FBE_STATUS_OK)
    {
        /* Just return the above function displayed an error.
         */
        fbe_cli_error("can't get object bus ,enclosur,slot information for id [0x%X],class id [0x%X],package id:0x%x status:0x%X\n",object_id,class_info_p->class_id,package_id, status);
        return status;
    }

    drive_information.location.bus = port;
    drive_information.location.enclosure= encl;
    drive_information.location.slot = slot;
    /* Get and display the physical drive attributes.
     */
    status = fbe_api_esp_drive_mgmt_get_drive_info(&drive_information);
    if (status != FBE_STATUS_OK)
    {
        /* Just return the above function displayed an error.
         */
        return status;
    }
    /* set vendor id & sr.no.string to 12bit for column adjustment*/
    fbe_cli_printf("| %d_%d_%-2d | %12s | %16s | %s | %12s | 0x%llX | %11s |\n",port,
                            encl,slot,drive_information.vendor_id,drive_information.product_id,
                            drive_information.product_rev,drive_information.product_serial_num,
                            (unsigned long long)drive_information.gross_capacity,drive_information.tla_part_number);
    
    return status;
}

/******************************************
 * end fbe_cli_disk_display_list_attr()
 ******************************************/

/*!**************************************************************
 * fbe_cli_disk_format()
 ****************************************************************
 * @brief
 *   This function starts disk zeroing for specific disk based on its physical location.
 *  
 *  @param    phys_location - will supply exact physical location of drive
 *
 * @return - none.  
 *
 * @author
 *  
 *
 ****************************************************************/
 
void fbe_cli_disk_format( fbe_job_service_bes_t *phys_location)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t drive_obj_id = FBE_OBJECT_ID_INVALID;
    fbe_port_number_t port = FBE_PORT_NUMBER_INVALID;
    fbe_enclosure_number_t encl = FBE_ENCLOSURE_NUMBER_INVALID;
    fbe_enclosure_slot_number_t slot = FBE_ENCLOSURE_SLOT_NUMBER_INVALID;

    port = phys_location->bus;
    encl = phys_location->enclosure;
    slot = phys_location->slot;

    status = fbe_api_provision_drive_get_obj_id_by_location_from_topology(port,encl,slot,&drive_obj_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_error("\nFailed to get provision drive object ID:0x%x,status:0x%x\n",drive_obj_id, status);
        return;
    }

    status=fbe_api_provision_drive_initiate_disk_zeroing(drive_obj_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_error("Failed to initiate disk zeroing for disk %d_%d_%d, Status:0x%x, Obj_id:0x%x.", port, encl, slot, status, drive_obj_id);
        return;
    }

    fbe_cli_printf("\nFormating Started...........\n");
    return;
}
/******************************************
 * end fbe_cli_disk_format()
 ******************************************/

/*!**************************************************************
 * fbe_cli_disk_address_range()
 ****************************************************************
 * @brief
 *   This function displays address range of different attributes for disk based on its physical location 
 *  
 *  @param    phys_location - will supply exact physical location of drive
 *
 * @return - none.  
 *
 * @author
 *  
 *
 ****************************************************************/
 void fbe_cli_disk_address_range( fbe_job_service_bes_t *phys_location)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_lba_t       capacity;
    fbe_block_size_t    exported_block_size;
    fbe_u32_t              actual_capacity_of_lun;
    fbe_u32_t              actual_capacity_of_drive;
    fbe_cli_lurg_rg_details_t  rg_details = {0};
    fbe_u32_t                 lun_count = 0;
    fbe_cli_lurg_lun_details_t  lun_details = {0};
    fbe_u32_t    rg_index = 0, lun_index = 0;
    fbe_class_id_t class_id = FBE_CLASS_ID_INVALID;
    fbe_api_base_config_upstream_object_list_t  upstream_rd_object_list = {0};
    fbe_api_base_config_upstream_object_list_t  upstream_object_list = {0};
    fbe_api_base_config_upstream_object_list_t  upstream_lun_object_list = {0};
    fbe_lba_t                                   end_lba = 0;
    fbe_u32_t drive_obj_id = FBE_OBJECT_ID_INVALID;
    fbe_api_provision_drive_info_t provision_drive_info = {0};
    fbe_raid_group_map_info_t map_info = {0};
    fbe_lba_t                 start_pba = 0;
    fbe_lba_t                 end_pba = 0;
    fbe_bool_t                is_system = FBE_FALSE;

    status = fbe_api_provision_drive_get_obj_id_by_location_from_topology(phys_location->bus, phys_location->enclosure,phys_location->slot,&drive_obj_id);
    if(status != FBE_STATUS_OK)
    {
        fbe_cli_error("\nFail to get PVD object id for %d_%d_%d,status:0x%x,Function:%s",phys_location->bus, phys_location->enclosure,phys_location->slot,status,__FUNCTION__);
        return ;
    }

    fbe_cli_printf("\n");
    fbe_cli_printf(" Disk %d_%d_%d Layout\n",phys_location->bus, phys_location->enclosure,phys_location->slot);
    fbe_cli_printf("\n LUN layout:\n");
    fbe_cli_printf("+----------------------------------------------+\n");
    fbe_cli_printf("|   LUN   | Size(mb) |        PBA Range        |\n");
    fbe_cli_printf("|---------|----------|-------------------------|\n");

        fbe_api_database_is_system_object(drive_obj_id, &is_system);
        if(is_system == FBE_TRUE)
        {
            status = fbe_cli_disk_get_system_rg_list(drive_obj_id, 
            &upstream_rd_object_list);
            if(status != FBE_STATUS_OK)
            {
                return ;
            }
        }
        else
        {
            status = fbe_cli_disk_get_user_rg_list(drive_obj_id, 
            &upstream_rd_object_list);
            if(status != FBE_STATUS_OK)
            {
                return ;
            }
        }
    for (rg_index = 0; rg_index< upstream_rd_object_list.number_of_upstream_objects; rg_index++)
    {
         if(upstream_rd_object_list.upstream_object_list[rg_index] == FBE_OBJECT_ID_INVALID )
        {
            continue;
        }
        /* Get the upstream Lun object list 
        */
        status = fbe_api_base_config_get_upstream_object_list(upstream_rd_object_list.upstream_object_list[rg_index], &upstream_object_list);
        if(status != FBE_STATUS_OK)
        {
            fbe_cli_error("\nFail to upstream Object list for Raid Group object id :0x%x ,status:0x%x,Function:%s",upstream_rd_object_list.upstream_object_list[rg_index],status,__FUNCTION__);
            return ;
        }

        /*if no lun present on raid group no need to go for lun checking */
        if(upstream_object_list.number_of_upstream_objects == 0)
        {
            break;
        }

        status = fbe_api_get_object_class_id(upstream_object_list.upstream_object_list[0], &class_id, FBE_PACKAGE_ID_SEP_0);

        /*checking class id for for special case of raid1_0*/
        if(class_id != FBE_CLASS_ID_LUN)
        {
            /* Get the upstream Lun object list for raid1_0
            */
            status = fbe_api_base_config_get_upstream_object_list(upstream_object_list.upstream_object_list[0], &upstream_lun_object_list);
            if(status != FBE_STATUS_OK)
            {
                fbe_cli_error("\nFail to upstream LUN Object list for Raid Group object id :0x%x ,status:0x%x,Function:%s",upstream_object_list.upstream_object_list[0],status,__FUNCTION__);
                return ;
            }
            /*discard other raid group as this is raid1_0 case*/
            upstream_rd_object_list.number_of_upstream_objects = 1;
        }
        else
        {
            upstream_lun_object_list = upstream_object_list;
        }
        for (lun_index= 0; lun_index < upstream_lun_object_list.number_of_upstream_objects; lun_index++)
        {
            status = fbe_cli_get_lun_details(upstream_lun_object_list.upstream_object_list[lun_index],&lun_details);
            if (status != FBE_STATUS_OK)
            {
                fbe_cli_error("Failed to get the LUN Detail for an Object id:0x%x, Status:0x%x",upstream_object_list.upstream_object_list[lun_index],status);
                return ;
            }
            
            rg_details.direction_to_fetch_object = DIRECTION_DOWNSTREAM;
                
            /* Get the details of RG associated with this LUN */
            status = fbe_cli_get_rg_details(upstream_lun_object_list.upstream_object_list[lun_index],&rg_details);
            if (status != FBE_STATUS_OK)
            {
                fbe_cli_error("Failed to get RG detail associated with this LUN of an Object id:0x%x, Status:0x%x",upstream_lun_object_list.upstream_object_list[lun_index],status);
                return ;
            }

            /* there should not be any error in getting RG details for LUN */
            capacity =  lun_details.lun_info.capacity;                
                exported_block_size = lun_details.lun_info.raid_info.exported_block_size;
                /* Convert the capacity to Mega Bytes */
                actual_capacity_of_lun = (fbe_u32_t)((capacity * exported_block_size)/(1024*1024));                
                                
                map_info.lba = 0;
                status = fbe_api_lun_map_lba(lun_details.lun_info.lun_object_id, &map_info);
                if (status != FBE_STATUS_OK)
                {
                    fbe_cli_printf("Failed doing map for lun obj id %d start lba: 0x%llx\n", lun_details.lun_info.lun_number, map_info.lba);
                    return;
                }
                start_pba = map_info.pba;
                
                map_info.lba = capacity - 1;
                status = fbe_api_lun_map_lba(lun_details.lun_info.lun_object_id, &map_info);
                if (status != FBE_STATUS_OK)
                {
                    fbe_cli_printf("Failed doing map for lun obj id %d end lba: 0x%llx\n", lun_details.lun_info.lun_number, map_info.lba);
                    return;
                }                                
                end_pba = map_info.pba;
                                              
                fbe_cli_printf("| %5d   |   %5d  | 0x%08llx - 0x%08llx |\n",lun_details.lun_number,actual_capacity_of_lun,start_pba,end_pba);                
                
                ++lun_count;
        }
    }

    if(lun_count == 0)
    {
        fbe_cli_printf("|                                              |\n");
        fbe_cli_printf("|        No luns present on this drive         |\n");
        fbe_cli_printf("|                                              |\n");
    }

    status = fbe_api_provision_drive_get_info(drive_obj_id,&provision_drive_info);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_error("\nfailed to get provision drive status: 0x%x,Function:%s\n", status,__FUNCTION__);
        return ;
    }

    fbe_cli_printf("+----------------------------------------------+\n");
    fbe_cli_printf("\n Physical Space layout:\n");
    fbe_cli_printf("+----------------------------------------------+\n");

    /* Convert the capacity to Mega Bytes */
    actual_capacity_of_drive = (fbe_u32_t)((provision_drive_info.capacity * 520)/(1024*1024));
    
    fbe_cli_printf("| User    | %7d  | 0x%08llx - 0x%08llx |\n",actual_capacity_of_drive,provision_drive_info.default_offset,provision_drive_info.capacity);

    
    /* Convert the capacity to Mega Bytes */
    actual_capacity_of_drive = (fbe_u32_t)((provision_drive_info.paged_metadata_capacity * 520)/(1024*1024));
    
    end_lba = provision_drive_info.paged_metadata_lba + provision_drive_info.paged_metadata_capacity;
    
    fbe_cli_printf("| Private | %7d  | 0x%llx - 0x%llx |\n",
           actual_capacity_of_drive,
           (unsigned long long)provision_drive_info.paged_metadata_lba,
           (unsigned long long)end_lba);

    
    fbe_cli_printf("+----------------------------------------------+\n");
    fbe_cli_printf("\n");
    return;

}
/******************************************
 * end fbe_cli_disk_address_range()
 ******************************************/
 
 /*!**************************************************************
 * fbe_cli_cmd_disk_info()
 ****************************************************************
 * @brief
 *   This function will allow viewing and changing attributes for a physical drive.
 *  
 *  @param    argc - argument count
 *  @param    argv - argument string
 *
 * @return - none. 
 * 
 * @version
 * 29-Oct-2013: 
 *  - Remove the -f option (fbe_cli_disk_format) since the functionality is to display the disk info.
 *
 ****************************************************************/

void fbe_cli_cmd_disk_info(int argc , char ** argv)
{
    fbe_u32_t object_id = FBE_OBJECT_ID_INVALID;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_package_id_t package_id = FBE_PACKAGE_ID_PHYSICAL;
    fbe_job_service_bes_t phys_location = {0};
    fbe_u32_t max_objects = 0;
    fbe_u32_t logpage = 0xFF;

    /* Port, enclosure and slot are not used but necessary to call
     * fbe_cli_execute_command_for_objects(), defaults to all port,
     * enclosure and slot.
     */
    fbe_u32_t port = FBE_API_PHYSICAL_BUS_COUNT;
    fbe_u32_t enclosure = FBE_API_ENCLOSURES_PER_BUS;
    fbe_u32_t slot = FBE_API_ENCLOSURE_SLOTS;

    /* Context for the command.
    */
    fbe_u32_t context = 0;

    /* Determines if we should target this command at all physical drives. 
    * If the port, enclosure, slot are given, then we will use that single 
    * physical drive as the target. 
    */
    fbe_bool_t b_target_all = FBE_TRUE;

    /* display Physical Disk Info only */
    fbe_bool_t b_phys_info_only = FBE_FALSE;

    /* Command Function */
    fbe_cli_command_fn command_fn = NULL;
    fbe_cli_command_bms_fn command_bms_fn = NULL;

    fbe_bool_t b_collect_bms = FBE_FALSE;
    fbe_bool_t b_display_bms = FBE_TRUE;

    /* By default if we are not given any arguments we do a 
    * terse display of all object. 
    * By default we also have -all enabled. 
    */

    /*
    * Parse the command line.
    */
    phys_location.bus=FBE_API_PHYSICAL_BUS_COUNT;
    phys_location.enclosure=FBE_API_ENCLOSURES_PER_BUS;
    phys_location.slot=FBE_API_ENCLOSURE_SLOTS;

    fbe_system_limits_get_max_objects (FBE_PACKAGE_ID_PHYSICAL, &max_objects); 

    while (argc > 0)
    {
        if ((strcmp(*argv, "-help") == 0) ||
            (strcmp(*argv, "-h") == 0))
        {
            /* If they are asking for help, just display help and exit.
            */
            fbe_cli_printf("%s", DISK_INFO_USAGE);
            return;
        }
        else if (strcmp(*argv, "-l") == 0)
        {
            argc--;
            argv++;

            fbe_cli_disk_display_list();

            return;
        }
        else if (strcmp(*argv, "-s") == 0)
        {
            argc--;
            argv++;

            fbe_cli_disk_address_range(&phys_location);

            return;
        }
        else if (strcmp(*argv, "-p") == 0) {
            argc--;
            argv++;

            b_phys_info_only = FBE_TRUE;

            continue;
        }
        else if (strcmp(*argv, "-bms") == 0)
        {
            argc--;
            argv++;

            b_collect_bms = FBE_TRUE;
            logpage = 0x15;

            continue;
        }
        else if (strcmp(*argv, "-logpage") == 0)
        {
            argc--;
            argv++;

            b_collect_bms = FBE_TRUE;
            
            if(argc == 0)
            {
                fbe_cli_error("-logpage: expected HEX #. Too few arguments \n");
                return;
            }

            if ((strcmp(*argv, "0x15") == 0) || (strcmp(*argv, "0x30") == 0) || (strcmp(*argv, "0x31") == 0))
            {
                logpage = (fbe_u32_t)strtoul(*argv, 0, 0);
            }
            else
            {
                fbe_cli_error("Unsupported logpage.  Only logpages: 0x15, 0x30, or 0x31 supported.\n");
                return;
            }
        }
        else if (strcmp(*argv, "-file") == 0)
        {
            argc--;
            argv++;

            b_display_bms = FBE_FALSE;
        }
        else
        {
            status = fbe_cli_convert_diskname_to_bed(*argv, &phys_location);
            
            if (status != FBE_STATUS_OK)
            {
                /* Just return the above function displayed an error.
                */
                return;
            }

            if (phys_location.bus >= FBE_API_PHYSICAL_BUS_COUNT)
            {
                fbe_cli_error("Wrong Bus value\n");
                return;
            }

            if (phys_location.enclosure >= FBE_API_ENCLOSURES_PER_BUS)
            {
                fbe_cli_error("Wrong Enclosure value\n");
                return;
            }

            if (phys_location.slot >= FBE_API_ENCLOSURE_SLOTS)
            {
                fbe_cli_error("Wrong Slot valuse\n");
                return;
            }

            b_target_all = FBE_FALSE;

        }
        argc--;
        argv++;
    }/* end of while */

    if (b_target_all == FBE_FALSE)
    {
        fbe_const_class_info_t *class_info_p;

        /* Get the object handle for this target.
        */
        status = fbe_api_get_physical_drive_object_id_by_location(phys_location.bus , phys_location.enclosure, phys_location.slot, &object_id);
        if ((status != FBE_STATUS_OK) || (object_id == FBE_OBJECT_ID_INVALID))
        {
            /* Just return the above function displayed an error.
            */
            fbe_cli_error("\nFailed to get physical drive object handle for %d_%d_%d\n",phys_location.bus , phys_location.enclosure, phys_location.slot);
            return;
        }

        else if (object_id > max_objects)
        {
            fbe_cli_error("\nFailed to get correct physical drive object id for %d_%d_%d\n",phys_location.bus , phys_location.enclosure, phys_location.slot);
            return;
        }
        /* Make sure this is a valid physical drive.
        */
        status = fbe_cli_get_class_info(object_id,package_id,&class_info_p);
            
        if (status != FBE_STATUS_OK)
        {
            /* Just return the above function displayed an error.
            */
            fbe_cli_error("Can't get object class information for id [0x%X],package id:0x%x status:0x%X\n",object_id,package_id, status);
            return;
        }

        /* Now that we have all the info, make sure this is a physical drive.
        */
        if ((class_info_p->class_id <= FBE_CLASS_ID_PHYSICAL_DRIVE_FIRST) || (class_info_p->class_id >= FBE_CLASS_ID_PHYSICAL_DRIVE_LAST))
        {
            fbe_cli_error("This object id 0x%x is not a physical drive but a %s (0x%x).\n",
                                            object_id, class_info_p->class_name, class_info_p->class_id);
            return;
        }

        // Execute the command against this single object based on '-p' option
        if ((b_phys_info_only == FBE_TRUE) && (b_collect_bms == FBE_FALSE))
        {
            status = fbe_cli_disk_phys_display_info(object_id, context, package_id);

            if (status != FBE_STATUS_OK)
            {
                fbe_cli_printf("\nCommand failed to port: %d enclosure: %d slot: %d status: 0x%x.\n",
                   phys_location.bus, phys_location.enclosure,
                   phys_location.slot, status);
                return;
            }
            return;
        }

        // Execute the command against this single object based on non '-p' option
        if ((b_phys_info_only == FBE_FALSE) && (b_collect_bms == FBE_FALSE))
        {
            status = fbe_cli_disk_display_info(object_id, context, package_id);
            if (status != FBE_STATUS_OK)
            {
                fbe_cli_printf("\nCommand failed to port: %d enclosure: %d slot: %d status: 0x%x.\n",
                   phys_location.bus, phys_location.enclosure,
                   phys_location.slot, status);
                return;
            }
            return;
        }

        // Execute command function for BMS data collection
        if (b_collect_bms == FBE_TRUE)
        {
            status = fbe_cli_disk_display_bms_list(object_id, logpage, package_id, b_display_bms);
            
            if (status != FBE_STATUS_OK && status != FBE_STATUS_PENDING)
            {
                fbe_cli_printf("\nCommand failed to port: %d enclosure: %d slot: %d status: 0x%x.\n",
                   phys_location.bus, phys_location.enclosure,
                   phys_location.slot, status);
                return;
            }
        }
        return;
    }

    // Execute the command function based on the -p option
    if ((b_phys_info_only == FBE_TRUE) && (b_collect_bms == FBE_FALSE))
    {
        command_fn = fbe_cli_disk_phys_display_info;
        fbe_cli_disk_execute_cmd(command_fn, context, port, enclosure, slot);
        return;
    }

    // Execute the command function based on the non '-p' option
    if ((b_phys_info_only == FBE_FALSE) && (b_collect_bms == FBE_FALSE))
    {
        command_fn = fbe_cli_disk_display_info;
        fbe_cli_disk_execute_cmd(command_fn, context, port, enclosure, slot);
        return;
    }

    // Execute command function for BMS data collection
    if (b_collect_bms == FBE_TRUE)
    {
        command_bms_fn = fbe_cli_disk_display_bms_list;
        fbe_cli_disk_execute_bms_cmd(command_bms_fn, logpage, port, enclosure, slot, b_display_bms);
        return;
    }

    return;
}      

/***************************
 * end fbe_cli_cmd_disk_info()
 ***************************/

 /*!**************************************************************
 * fbe_cli_disk_get_system_rg_list()
 ****************************************************************
 * @brief
 *   This function will return the list of raidgroup on system drive.
 *  
 *  @param    object_id - object id.
 *  @param    upstream_object_list_p - upstream object list pointer.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK  - if no error.
 *
 * @version
 *  08/30/2012 - Created. Harshal Wanjari
 *
 ****************************************************************/
static fbe_status_t fbe_cli_disk_get_system_rg_list(fbe_object_id_t object_id,
                                                                         fbe_api_base_config_upstream_object_list_t * upstream_object_list_p)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_u32_t    rg_index = 0,loop_index = 0,index = 0;
    fbe_class_id_t class_id = FBE_CLASS_ID_INVALID;
    fbe_api_base_config_upstream_object_list_t  upstream_vd_object_list = {0};
    fbe_api_base_config_upstream_object_list_t  upstream_object_list = {0};

    /* Get the upstream vd object list */
    status = fbe_api_base_config_get_upstream_object_list(object_id, &upstream_vd_object_list);
    if(status != FBE_STATUS_OK)
    {
        fbe_cli_error("\nFail to upstream VD Object list for PVD object id :0x%x ,status:0x%x,Function:%s",object_id,status,__FUNCTION__);
        return status;
    }
    
    for (rg_index = 0; rg_index< upstream_vd_object_list.number_of_upstream_objects; rg_index++)
    {
        fbe_api_get_object_class_id(upstream_vd_object_list.upstream_object_list[rg_index], &class_id, FBE_PACKAGE_ID_SEP_0);
            /*check for system raid group*/
        if(class_id < FBE_CLASS_ID_RAID_LAST && class_id > FBE_CLASS_ID_RAID_FIRST)
        {
            upstream_object_list_p->upstream_object_list[index] = 
            upstream_vd_object_list.upstream_object_list[index];
            index++;
        }
        else
        {
            /* Get the upstream Raid object list 
            */
            status = fbe_api_base_config_get_upstream_object_list(upstream_vd_object_list.upstream_object_list[rg_index], &upstream_object_list);
            if(status != FBE_STATUS_OK)
            {
                fbe_cli_error("\nFail to upstream Raid Group Object list for VD object id :0x%x ,status:0x%x,Function:%s",upstream_vd_object_list.upstream_object_list[rg_index],status,__FUNCTION__);
                return status;
            }
            for(loop_index = 0; loop_index < upstream_object_list.number_of_upstream_objects; loop_index++)
            {
                upstream_object_list_p->upstream_object_list[index] = upstream_object_list.upstream_object_list[loop_index];
                index++;
            }
        }
    }
    
    upstream_object_list_p->number_of_upstream_objects = index;
    return status;
}

/***************************
 * end fbe_cli_disk_get_system_rg_list()
 ***************************/

 /*!**************************************************************
 * fbe_cli_disk_get_user_rg_list()
 ****************************************************************
 * @brief
 *   This function will return the list of raidgroup on user drive.
 *  
 *  @param    object_id - object id.
 *  @param    upstream_object_list_p - upstream object list pointer.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK  - if no error.
 *
 * @version
 *  08/30/2012 - Created. Harshal Wanjari
 *
 ****************************************************************/
static fbe_status_t fbe_cli_disk_get_user_rg_list(fbe_object_id_t object_id,
                                                                         fbe_api_base_config_upstream_object_list_t * upstream_object_list_p)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_u32_t    rg_index = 0,loop_index = 0,index = 0;
    fbe_api_base_config_upstream_object_list_t  upstream_vd_object_list = {0};
    fbe_api_base_config_upstream_object_list_t  upstream_object_list = {0};

    status = fbe_api_base_config_get_upstream_object_list(object_id, &upstream_vd_object_list);
    if(status != FBE_STATUS_OK)
    {
        fbe_cli_error("\nFail to upstream VD Object list for PVD object id :0x%x ,status:0x%x,Function:%s",object_id,status,__FUNCTION__);
        return status;
    }
    
    for (rg_index = 0; rg_index< upstream_vd_object_list.number_of_upstream_objects; rg_index++)
    {
            /* Get the upstream Raid object list 
            */
            status = fbe_api_base_config_get_upstream_object_list(upstream_vd_object_list.upstream_object_list[rg_index], &upstream_object_list);
            if(status != FBE_STATUS_OK)
            {
                fbe_cli_error("\nFail to upstream Raid Group Object list for VD object id :0x%x ,status:0x%x,Function:%s",upstream_vd_object_list.upstream_object_list[rg_index],status,__FUNCTION__);
                return status;
            }
            for(loop_index = 0; loop_index < upstream_object_list.number_of_upstream_objects; loop_index++)
            {
                upstream_object_list_p->upstream_object_list[index] = upstream_object_list.upstream_object_list[loop_index];
                index++;
            }
    }
    upstream_object_list_p->number_of_upstream_objects = index;
    return status;
}

/***************************
 * end fbe_cli_disk_get_user_rg_list()
 ***************************/

/*!**************************************************************
 * fbe_cli_disk_display_info_power_saving()
 ****************************************************************
 * @brief
 *   This function displays power saving information
 *   for the disk.
 *  
 *  @param    port - port number of drive
 *  @param    enclosure -enclosure number of drive
 *  @param    slot - slot number of drive
 *
 * @return - fbe_status_t status of the operation. 
 *
 * @author
 *  
 *
 ****************************************************************/
static fbe_status_t fbe_cli_disk_display_info_power_saving(fbe_u32_t port,fbe_u32_t enclosure,fbe_u32_t slot)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t drive_obj_id = FBE_OBJECT_ID_INVALID;
    fbe_base_config_object_power_save_info_t power_save_info = {0};

    status = fbe_api_provision_drive_get_obj_id_by_location_from_topology(port, enclosure, slot,&drive_obj_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_error("\nFailed to get provision drive status: 0x%x,object id:0x%x\n", status,drive_obj_id);
        return status;
    }

    status = fbe_api_get_object_power_save_info(drive_obj_id, &power_save_info);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_error("\nFailed status 0x%x to power save info for object_id: 0x%x\n",
                  status, drive_obj_id);
        return status;
    }

    fbe_cli_printf("   Power saving:             %s \n", power_save_info.power_saving_enabled? "Enabled":"Disabled");
    fbe_cli_printf("   Idle Time:                %llu\n", (unsigned long long) power_save_info.configured_idle_time_in_seconds);
    fbe_cli_printf("   Hibernation Time:         %llu\n", (unsigned long long)power_save_info.hibernation_time);
    fbe_cli_printf("   Seconds since last IO:    %llu\n", (unsigned long long)power_save_info.seconds_since_last_io);

    switch(power_save_info.power_save_state)
    {
        case FBE_POWER_SAVE_STATE_NOT_SAVING_POWER:
            fbe_cli_printf("   Power save state:         FBE_POWER_SAVE_STATE_NOT_SAVING_POWER\n");
            break;

        case FBE_POWER_SAVE_STATE_SAVING_POWER:
            fbe_cli_printf("   Power save state:         FBE_POWER_SAVE_STATE_SAVING_POWER\n");
            break;
            
        case FBE_POWER_SAVE_STATE_ENTERING_POWER_SAVE:
            fbe_cli_printf("   Power save state:         FBE_POWER_SAVE_STATE_ENTERING_POWER_SAVE\n");
            break;
            
        case FBE_POWER_SAVE_STATE_EXITING_POWER_SAVE:
            fbe_cli_printf("   Power save state:         FBE_POWER_SAVE_STATE_EXITING_POWER_SAVE\n");
            break;

        default:
            fbe_cli_printf("   Power save state:         FBE_POWER_SAVE_STATE_INVALID\n");
            break;    
    }

    return status;

}
/******************************************
 * end  fbe_cli_disk_display_info_power_saving()
 ******************************************/

/*!*************************************************************************
 * @fn fbe_cli_map_lifecycle_state_to_str(fbe_lifecycle_state_t state) 
 **************************************************************************
 *
 *  @brief
 *      This function converts the lifecycle state into text string.
 *
 *  @param    fbe_lifecycle_state_t - state
 *
 *  @return     char * - lifecycle_state_str
 *
 *  NOTES:
 *
 *  HISTORY:
 *   29-Oct-2013 - Created
 *
 **************************************************************************/
static char *fbe_cli_map_lifecycle_state_to_str(fbe_lifecycle_state_t state)
{
    char *lifecycle_state_str = NULL;

    switch (state)
    {
        case FBE_LIFECYCLE_STATE_SPECIALIZE:
            lifecycle_state_str = (char *)("FBE_LIFECYCLE_STATE_SPECIALIZE");
            break;
        case FBE_LIFECYCLE_STATE_ACTIVATE:
            lifecycle_state_str = (char *)("FBE_LIFECYCLE_STATE_ACTIVATE");
            break;
        case FBE_LIFECYCLE_STATE_READY:
            lifecycle_state_str = (char *)("FBE_LIFECYCLE_STATE_READY");
            break;
        case FBE_LIFECYCLE_STATE_HIBERNATE:
            lifecycle_state_str = (char *)("FBE_LIFECYCLE_STATE_HIBERNATE");
            break;
        case FBE_LIFECYCLE_STATE_OFFLINE:
            lifecycle_state_str = (char *)("FBE_LIFECYCLE_STATE_OFFLINE");
            break;
        case FBE_LIFECYCLE_STATE_FAIL:
            lifecycle_state_str = (char *)("FBE_LIFECYCLE_STATE_FAIL");
            break;
        case FBE_LIFECYCLE_STATE_DESTROY:
            lifecycle_state_str = (char *)("FBE_LIFECYCLE_STATE_DESTROY");
            break;
        case FBE_LIFECYCLE_STATE_NON_PENDING_MAX:
            lifecycle_state_str = (char *)("FBE_LIFECYCLE_STATE_NON_PENDING_MAX");
            break;
        case FBE_LIFECYCLE_STATE_PENDING_ACTIVATE:
            lifecycle_state_str = (char *)("FBE_LIFECYCLE_STATE_PENDING_ACTIVATE");
            break;
        case FBE_LIFECYCLE_STATE_PENDING_HIBERNATE:
            lifecycle_state_str = (char *)("FBE_LIFECYCLE_STATE_PENDING_HIBERNATE");
            break;
        case FBE_LIFECYCLE_STATE_PENDING_OFFLINE:
            lifecycle_state_str = (char *)("FBE_LIFECYCLE_STATE_PENDING_OFFLINE");
            break;
        case FBE_LIFECYCLE_STATE_PENDING_FAIL:
            lifecycle_state_str = (char *)("FBE_LIFECYCLE_STATE_PENDING_FAIL");
            break;
        case FBE_LIFECYCLE_STATE_PENDING_DESTROY:
            lifecycle_state_str = (char *)("FBE_LIFECYCLE_STATE_PENDING_DESTROY");
            break;
        case FBE_LIFECYCLE_STATE_NOT_EXIST:
            lifecycle_state_str = (char *)( "FBE_LIFECYCLE_STATE_NOT_EXIST");
            break;
        case FBE_LIFECYCLE_STATE_INVALID:
        default:
            lifecycle_state_str = (char *)("FBE_LIFECYCLE_STATE_INVALID");
            break;
    }

    return (lifecycle_state_str);

}
/******************************************
 * end  fbe_cli_map_lifecycle_state_to_str()
 ******************************************/

/*!**************************************************************
 *  fbe_cli_disk_execute_cmd()
 ****************************************************************
 @brief
 *  This function displays the BMS data.
 *
 * @param object_id  - object id
 * @param context    - in this case we do not use the context.
 * @param package_id - package id of that object.
 *
 * @return fbe_status_t status of the operation.
 *
 * @author
 *  
 *
 ****************************************************************/
static void fbe_cli_disk_execute_cmd(fbe_cli_command_fn command_fn, fbe_u32_t context, fbe_u32_t port, fbe_u32_t enclosure, fbe_u32_t slot)
{
    fbe_package_id_t package_id = FBE_PACKAGE_ID_PHYSICAL;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_class_id_t class_id = FBE_CLASS_ID_INVALID;

    /* The operation will be performed to all physical drives.
    */
    status = fbe_cli_execute_command_for_objects(FBE_CLASS_ID_BASE_PHYSICAL_DRIVE,
                                                 package_id,
                                                 port,
                                                 enclosure,
                                                 slot,
                                                 command_fn,
                                                 context/* no context */);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_printf("\nPhysical drive command to all drives failed status: 0x%x.\n", status);
        return;
    }

    /* Execute the command against all sas physical drives.
    */
    for (class_id = FBE_CLASS_ID_SAS_PHYSICAL_DRIVE_FIRST +1; class_id < FBE_CLASS_ID_SAS_PHYSICAL_DRIVE_LAST; class_id++)
    {
        status = fbe_cli_execute_command_for_objects(class_id,
                                                     package_id,
                                                     port,
                                                     enclosure,
                                                     slot,
                                                     command_fn,
                                                     context/* no context */);

        if (status != FBE_STATUS_OK)
         {
             fbe_cli_printf("\nPhysical drive command to all drives failed status: 0x%x.\n", status);
             return;
         }
    }

#ifdef  BLOCKSIZE_512_SUPPORTED
    /* Execute the command against all sata physical drives.
    */
    for (class_id = FBE_CLASS_ID_SATA_PHYSICAL_DRIVE_FIRST +1; class_id < FBE_CLASS_ID_SATA_PHYSICAL_DRIVE_LAST; class_id++)
    {
        status = fbe_cli_execute_command_for_objects(class_id,
                                                 package_id,
                                                 port,
                                                 enclosure,
                                                 slot,
                                                 command_fn,
                                                 context /* no context */);

        if (status != FBE_STATUS_OK)
        {
            fbe_cli_printf("\nPhysical drive command to all drives failed status: 0x%x.\n", status);
            return;
        }
    }
#endif

    return;
}

/*!**************************************************************
 *  fbe_cli_disk_display_bms_list()
 ****************************************************************
 @brief
 *  This function sends the log page to PDO to get the BMS data.
 *
 * @param object_id  - object id
 * @param context    - use logpage as context.
 * @param package_id - package id of that object.
 *
 * @return fbe_status_t status of the operation.
 *
 * @author
 *  
 *
 ****************************************************************/
static fbe_status_t fbe_cli_disk_display_bms_list(fbe_u32_t object_id, fbe_u32_t logpage, fbe_package_id_t package_id, fbe_bool_t b_display_bms)
{
    fbe_status_t                         status = FBE_STATUS_OK;
    fbe_const_class_info_t               *class_info_p =NULL;
    fbe_physical_drive_bms_op_asynch_t   *bms_op_p = NULL;
    fbe_semaphore_t                      resume_semaphore;
    fbe_physical_drive_information_t     drive_info = {0}; 
    fbe_object_id_t                      esp_object_id;


    /* get DRIVE Mgmt object ID */
    status = fbe_api_esp_getObjectId(FBE_CLASS_ID_DRIVE_MGMT, &esp_object_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_error("%s: failed to get ESP object.\n",__FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    bms_op_p = (fbe_physical_drive_bms_op_asynch_t*)fbe_api_allocate_contiguous_memory(sizeof(fbe_physical_drive_bms_op_asynch_t));
    if (NULL == bms_op_p)
    {
        fbe_cli_error("%s failed pass thru alloc.\n",__FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_zero_memory(bms_op_p, sizeof(fbe_physical_drive_bms_op_asynch_t));

    bms_op_p->response_buf = fbe_api_allocate_contiguous_memory(DMO_FUEL_GAUGE_READ_BUFFER_LENGTH);
    if (NULL == bms_op_p->response_buf)
    {
        fbe_cli_error("%s failed recv buff alloc.\n",__FUNCTION__);
        fbe_api_free_contiguous_memory(bms_op_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_zero_memory(bms_op_p->response_buf, DMO_FUEL_GAUGE_READ_BUFFER_LENGTH);

    bms_op_p->display_b = b_display_bms;
    bms_op_p->object_id = object_id;
    bms_op_p->esp_object_id = esp_object_id;

    // Make sure this is a valid object.
    status = fbe_cli_get_class_info(object_id,package_id,&class_info_p);
    if (status != FBE_STATUS_OK)
    {
        // Just return the above function displayed an error.
        fbe_cli_error("can't get object class information for id [0x%X],package id:0x%X status:0x%X\n",object_id,package_id,status);
        return status;
    }

    // Get and display the physical drive attributes.
    status = fbe_api_physical_drive_get_drive_information(object_id, &drive_info, FBE_PACKET_FLAG_NO_ATTRIB);
    if (status != FBE_STATUS_OK)
    {
        // Just return the above function displayed an error.
        fbe_cli_error("can't get drive info for obj[0x%X],package id:0x%X status:0x%X\n",object_id,package_id,status);
        return status;
    }

    fbe_copy_memory(&bms_op_p->get_drive_info, &drive_info, sizeof(fbe_physical_drive_information_t));

    fbe_cli_printf("Get BMS data:  \n");
    fbe_cli_printf("   Drive:               %d_%d_%d\n", drive_info.port_number, 
                   drive_info.enclosure_number, drive_info.slot_number);

    // Drive Class is an enumerated type - print raw and interpreted value
    switch(drive_info.drive_type)
    {
        case FBE_DRIVE_TYPE_FIBRE:

            fbe_cli_printf("   Drive Type:  FBE_DRIVE_TYPE_FIBRE\n");
            break;

         case FBE_DRIVE_TYPE_SAS:
            fbe_cli_printf("   Drive Type:  FBE_DRIVE_TYPE_SAS\n");
            break;
            
         case FBE_DRIVE_TYPE_SATA:
            fbe_cli_printf("   Drive Type:  FBE_DRIVE_TYPE_SATA\n");
            break;
            
         case FBE_DRIVE_TYPE_SAS_FLASH_HE:
            fbe_cli_printf("   Drive Type:  FBE_DRIVE_TYPE_SAS_FLASH_HE\n");
            break;
            
         case FBE_DRIVE_TYPE_SATA_FLASH_HE:
            fbe_cli_printf("   Drive Type:  FBE_DRIVE_TYPE_SATA_FLASH_HE\n");
            break;
            
        case FBE_DRIVE_TYPE_SAS_FLASH_ME:
            fbe_cli_printf("   Drive Type:  FBE_DRIVE_TYPE_SAS_FLASH_ME\n");
            break;            
            
        case FBE_DRIVE_TYPE_SAS_FLASH_LE:
            fbe_cli_printf("   Drive Type:  FBE_DRIVE_TYPE_SAS_FLASH_LE\n");
            break;            

        case FBE_DRIVE_TYPE_SAS_FLASH_RI:
            fbe_cli_printf("   Drive Type:  FBE_DRIVE_TYPE_SAS_FLASH_RI\n");
            break;            
            
        case FBE_DRIVE_TYPE_SAS_NL:
            fbe_cli_printf("   Drive Type:  FBE_DRIVE_TYPE_SAS_NL\n");
            break;
        
        case FBE_DRIVE_TYPE_SATA_PADDLECARD:
            fbe_cli_printf("   Drive Type:  FBE_DRIVE_TYPE_SATA_PADDLECARD\n");
            break;

        default:
            fbe_cli_printf("   Drive Type:  FBE_DRIVE_TYPE_INVALID\n");
            break;    
    }

    fbe_semaphore_init(&resume_semaphore, 0, 1);

    bms_op_p->get_log_page.page_code = logpage;
    bms_op_p->get_log_page.transfer_count = DMO_FUEL_GAUGE_READ_BUFFER_LENGTH;
    bms_op_p->completion_function = fbe_cli_disk_display_bms_completion;
    bms_op_p->context = &resume_semaphore;

    // get BMS log page
    status = fbe_api_physical_drive_get_bms_op_asynch(object_id, bms_op_p);
    if (status != FBE_STATUS_OK && status != FBE_STATUS_PENDING)
    {
        fbe_cli_error( "fbe_api_physical_drive_get_bms_asynch. failed. status: %d\n", status);

        fbe_api_free_contiguous_memory(bms_op_p->response_buf);
        fbe_api_free_contiguous_memory(bms_op_p);

        fbe_semaphore_destroy(&resume_semaphore); 
        return status;
    }  

    fbe_semaphore_wait(&resume_semaphore, NULL);

    fbe_semaphore_destroy(&resume_semaphore); 

    return status;
}

/*!**************************************************************
 *  fbe_cli_disk_display_bms_completion()
 ****************************************************************
 @brief
 *  This function displays the BMS data.
 *
 * @param object_id  - object id
 * @param context    - use logpage as context.
 * @param package_id - package id of that object.
 *
 * @return fbe_status_t status of the operation.
 *
 * @author
 *  
 *
 ****************************************************************/
static void fbe_cli_disk_display_bms_completion(fbe_packet_completion_context_t context)
{
    fbe_physical_drive_bms_op_asynch_t   *bms_op_p = (fbe_physical_drive_bms_op_asynch_t*)context;
    fbe_u32_t                            i = 0;
    fbe_file_handle_t                    file_handle = FBE_FILE_INVALID_HANDLE;
    fbe_u8_t                             drive_name[30];
    int                                  access_mode = 0;
    fbe_u32_t                            bytes_sent = 0;
    fbe_u8_t                             file_path[100];
    fbe_u32_t                            bytes_to_xfer = bms_op_p->get_log_page.transfer_count;
    fbe_semaphore_t                      *get_mp_completion_semaphore_p = (fbe_semaphore_t*)bms_op_p->context;
    fbe_status_t                         status = FBE_STATUS_OK;

    fbe_cli_printf("   object id:           0x%x\n", bms_op_p->object_id);
    fbe_cli_printf("   log page:            0x%x\n", bms_op_p->get_log_page.page_code);
    fbe_cli_printf("   transfer count:      0x%x\n", bms_op_p->get_log_page.transfer_count);
    fbe_cli_printf("   bms_op_p->status:    0x%x\n", bms_op_p->status);


    // Do not display on-screen
    if (bms_op_p->display_b == FBE_FALSE)
    {

        fbe_zero_memory(drive_name, sizeof(drive_name));
        _snprintf(drive_name, sizeof(drive_name), "log_%d_%d_%d", 
                  bms_op_p->get_drive_info.port_number, 
                  bms_op_p->get_drive_info.enclosure_number, 
                  bms_op_p->get_drive_info.slot_number);

        // set up file name and path
        fbe_cli_disk_bms_file(file_path, drive_name);

        // Create a file. 
        access_mode |= FBE_FILE_RDWR|(FBE_FILE_CREAT | FBE_FILE_TRUNC);
        if ((file_handle = fbe_file_open(file_path, access_mode, 0, NULL)) == FBE_FILE_INVALID_HANDLE)
        {
            // The target file doesn't exist.  
            fbe_cli_printf("The log_disk_%d_%d_%d file is not Existed . \n",
                           bms_op_p->get_drive_info.port_number, 
                           bms_op_p->get_drive_info.enclosure_number, 
                           bms_op_p->get_drive_info.slot_number);
            // Close the file since we are done with it.
            fbe_file_close(file_handle);

            fbe_semaphore_release(get_mp_completion_semaphore_p, 0, 1 ,FALSE);

            return; 
        }

        // Try to seek to the correct location.
        if (fbe_file_lseek(file_handle, 0, 0) == -1)
        {
            fbe_cli_printf("%s drive %s attempt to seek past end of disk.\n",  __FUNCTION__, file_path);

            fbe_api_free_contiguous_memory(bms_op_p->response_buf);
            fbe_api_free_contiguous_memory(bms_op_p);

            // Close the file since we are done with it.
            fbe_file_close(file_handle);
            fbe_semaphore_release(get_mp_completion_semaphore_p, 0, 1 ,FALSE);

            return; 
        }
        
        // Write data to disk. 
        bytes_sent = fbe_file_write(file_handle, bms_op_p->response_buf, bytes_to_xfer, NULL);

        // Check if we transferred enough
        if (bytes_sent != bytes_to_xfer)
        {
            // Error, we didn't transfer what we expected.
            fbe_cli_printf("%s drive %s unexpected transfer. bytes_received = %d, bytes_to_xfer = %d. \n",
                           __FUNCTION__, drive_name, bytes_sent, bytes_to_xfer);

            fbe_file_close(file_handle);
            return;
        }

        fbe_cli_printf("   Write raw data to file:    %s\n", file_path);

        fbe_cli_printf("\n");

        bms_op_p->rw_buffer_length = DMO_FUEL_GAUGE_WRITE_BUFFER_LENGTH;
        bms_op_p->completion_function = fbe_cli_disk_display_bms_write_info_completion;
        bms_op_p->esp_object_id = bms_op_p->esp_object_id;

        // get BMS log page
        status = fbe_api_esp_write_bms_logpage_data_asynch(bms_op_p);

        if (status != FBE_STATUS_OK && status != FBE_STATUS_PENDING)
        {
            fbe_cli_error( "fbe_api_esp_write_bms_info. failed. status: %d\n", status);

            fbe_api_free_contiguous_memory(bms_op_p->response_buf);
            fbe_api_free_contiguous_memory(bms_op_p);

            // Close the file since we are done with it.
            fbe_file_close(file_handle); 

            return;
        }  

        // Close the file since we are done with it.
        fbe_file_close(file_handle); 

        return;
    } // end of if (bms_op_p->display_b == FBE_FALSE)


    // Display on-screen 
    while (i < bms_op_p->get_log_page.transfer_count)
    {
        fbe_cli_printf("%02x ", bms_op_p->response_buf[i]);
   
        i++;
   
        if (i%16 == 0)
        {
            fbe_cli_printf("\n");
        }
    }
        
    fbe_cli_printf("\n");

    fbe_semaphore_release(get_mp_completion_semaphore_p, 0, 1 ,FALSE);

    return;
} // end of fbe_cli_disk_display_bms_completion

/*!**************************************************************
 *  fbe_cli_disk_display_bms_write_info_completion()
 ****************************************************************
 @brief
 *  This function displays the BMS data.
 *
 * @param object_id  - object id
 * @param context    - use logpage as context.
 * @param package_id - package id of that object.
 *
 * @return fbe_status_t status of the operation.
 *
 * @author
 *  
 *
 ****************************************************************/
static void fbe_cli_disk_display_bms_write_info_completion(fbe_packet_completion_context_t context)
{
    fbe_physical_drive_bms_op_asynch_t* bms_op_p = (fbe_physical_drive_bms_op_asynch_t*)context;
    fbe_semaphore_t * get_mp_completion_semaphore_p = (fbe_semaphore_t*)bms_op_p->context;
    fbe_u8_t                             drive_name[30];
    int                                  access_mode = 0;
    fbe_u32_t                            bytes_sent = 0;
    fbe_u8_t                             file_path[100];
    fbe_u32_t                            bytes_to_xfer = bms_op_p->rw_buffer_length;
    fbe_file_handle_t                    file_handle = FBE_FILE_INVALID_HANDLE;

    fbe_cli_printf("Write BMS collected data:  \n");
    fbe_cli_printf("   rw buffer length:          0x%x\n", bms_op_p->rw_buffer_length);
    fbe_cli_printf("   bms_op_p->status:        0x%x\n", bms_op_p->status);

    fbe_zero_memory(drive_name, sizeof(drive_name));
    _snprintf(drive_name, sizeof(drive_name), "log_%d_%d_%d_bms_stats.txt", 
              bms_op_p->get_drive_info.port_number, 
              bms_op_p->get_drive_info.enclosure_number, 
              bms_op_p->get_drive_info.slot_number);

    // set up file name and path
    fbe_cli_disk_bms_file(file_path, drive_name);

    // Create a file. 
    access_mode |= FBE_FILE_RDWR|(FBE_FILE_CREAT | FBE_FILE_TRUNC );
    if ((file_handle = fbe_file_open(file_path, access_mode, 0, NULL)) == FBE_FILE_INVALID_HANDLE)
    {
        // The target file doesn't exist.  
        fbe_cli_printf("Could not open file: %s.  Create new file\n", file_path);

        fbe_api_free_contiguous_memory(bms_op_p->response_buf);
        fbe_api_free_contiguous_memory(bms_op_p);

        // Close the file since we are done with it.
        fbe_file_close(file_handle);

        fbe_semaphore_release(get_mp_completion_semaphore_p, 0, 1 ,FALSE);

        return;
    }

    // Try to seek to the correct location.
    if (fbe_file_lseek(file_handle, 0, 0) == -1)
    {
        fbe_cli_printf("%s drive %s attempt to seek past end of disk.\n",  __FUNCTION__, file_path);

        fbe_api_free_contiguous_memory(bms_op_p->response_buf);
        fbe_api_free_contiguous_memory(bms_op_p);

        // Close the file since we are done with it.
        fbe_file_close(file_handle);
        fbe_semaphore_release(get_mp_completion_semaphore_p, 0, 1 ,FALSE);

        return; 
    }

    // Write data to disk. 
    bytes_sent = fbe_file_write(file_handle, bms_op_p->response_buf, bytes_to_xfer, NULL);

    // Check if we transferred enough
    if (bytes_sent != bytes_to_xfer)
    {
        // Error, we didn't transfer what we expected.
        fbe_cli_printf("%s drive %s unexpected transfer. bytes_received = 0x%x, bytes_to_xfer = 0x%x. \n",
                       __FUNCTION__, drive_name, bytes_sent, bytes_to_xfer);

        fbe_api_free_contiguous_memory(bms_op_p->response_buf);
        fbe_api_free_contiguous_memory(bms_op_p);

        fbe_file_close(file_handle);

        fbe_semaphore_release(get_mp_completion_semaphore_p, 0, 1 ,FALSE);

        return;
    }

    fbe_cli_printf("   Write collected data to file:    %s\n", file_path);
    fbe_cli_printf("\n");

    fbe_file_close(file_handle);

    fbe_semaphore_release(get_mp_completion_semaphore_p, 0, 1 ,FALSE);

    fbe_api_free_contiguous_memory(bms_op_p->response_buf);
    fbe_api_free_contiguous_memory(bms_op_p);

    return;
} // end of fbe_cli_disk_display_bms_write_info_completion

/*!**************************************************************
 *  fbe_cli_disk_bms_file()
 ****************************************************************
 @brief
 *  This function setup file name and path.
 *
 * @param location    - pointer to the location
 * @param drive_name  - pointer to the drive name
 *
 * @return fbe_status_t status of the operation.
 *
 * @author
 *  
 *
 ****************************************************************/
static void fbe_cli_disk_bms_file(fbe_char_t *location, fbe_char_t *drive_name)
{
    csx_char_t filename[64];
    
#if !defined(SIMMODE_ENV)
    EmcutilFilePathMake(filename, sizeof(filename), EMCUTIL_BASE_DUMPS, drive_name, NULL);
#else
    SAFE_STRNCPY(filename, drive_name,sizeof(filename));
#endif 

    strncpy(location, filename, sizeof(filename));
    
    location[sizeof(filename)] = '\0';
    
    return;
} // end of fbe_cli_disk_bms_file

/*!**************************************************************
 *  fbe_cli_execute_command_for_bms_objects()
 ****************************************************************
 @brief
 *  This function setup file name and path.
 *
 * @param location    - pointer to the location
 * @param drive_name  - pointer to the drive name
 *
 * @return fbe_status_t status of the operation.
 *
 * @author
 *  
 *
 ****************************************************************/
static fbe_status_t fbe_cli_execute_command_for_bms_objects(fbe_class_id_t filter_class_id,
                                                 fbe_package_id_t package_id,
                                                 fbe_u32_t filter_port,
                                                 fbe_u32_t filter_enclosure,
                                                 fbe_u32_t filter_drive_no,
                                                 fbe_cli_command_bms_fn command_function_p,
                                                 fbe_u32_t context,
                                                 fbe_bool_t display_b)
{
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_u32_t           object_index;
    fbe_u32_t           total_objects = 0;
    fbe_object_id_t    *object_list_p = NULL;
    fbe_class_id_t      class_id;
    fbe_port_number_t port = FBE_API_PHYSICAL_BUS_COUNT;
    fbe_enclosure_number_t enclosure = FBE_API_ENCLOSURES_PER_BUS;
    fbe_enclosure_slot_number_t drive_no = FBE_API_ENCLOSURE_SLOTS;
    fbe_status_t        error_status = FBE_STATUS_OK; /* Save last error we see.*/
    fbe_u32_t           found_objects_count = 0;      /* Number matching class filter. */
    fbe_u32_t			object_count = 0;
    fbe_topology_object_type_t  obj_type = FBE_TOPOLOGY_OBJECT_TYPE_INVALID;

    status = fbe_api_get_total_objects(&object_count, package_id);
    if (status != FBE_STATUS_OK) {
        return status;
    }

    /* Allocate memory for the objects.
     */
    object_list_p = fbe_api_allocate_memory(sizeof(fbe_object_id_t) * object_count);

    if (object_list_p == NULL)
    {
        fbe_cli_error("Unable to allocate memory for object list %s \n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Find the count of total objects.
     */
    status = fbe_api_enumerate_objects(object_list_p, object_count, &total_objects, package_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_api_free_memory(object_list_p);
        return status;
    }

    /* Loop over all objects found in the table.
     */
    for (object_index = 0; object_index < total_objects; object_index++)
    {
        fbe_object_id_t object_id = object_list_p[object_index];

        status = fbe_api_get_object_class_id(object_id, &class_id, package_id);
        if(status == FBE_STATUS_OK)
        {
            status = fbe_api_get_object_type (object_id, &obj_type, package_id);
        }

        if (FBE_STATUS_OK == status)
        {
            switch(obj_type)
            {


                /* If the object is a drive or logical drive get the drive number and fall through*/
                case FBE_TOPOLOGY_OBJECT_TYPE_PHYSICAL_DRIVE:
                case FBE_TOPOLOGY_OBJECT_TYPE_LOGICAL_DRIVE:
                    if ((status = fbe_api_get_object_drive_number(object_id, &drive_no)) != FBE_STATUS_OK)
                    {
                        break;
                    }

                /* If the object is an enclosure get the enclosure number and fall through*/
                case FBE_TOPOLOGY_OBJECT_TYPE_ENCLOSURE:
                    if ((status = fbe_api_get_object_enclosure_number(object_id, &enclosure)) != FBE_STATUS_OK)
                    {
                        break;
                    }

                /* If the object is a port get the port number and fall through*/
                case FBE_TOPOLOGY_OBJECT_TYPE_PORT:
                    if ((status = fbe_api_get_object_port_number(object_id, &port)) != FBE_STATUS_OK)
                    {
                        break;
                    }

                /* If the object is any other type exit the switch */
                default:
                    break;
            }
        }

        /* Match the class id to the filter class id.
         * OR 
         * Assume a match if the filter id is set to invalid, which means to apply to 
         * every object. 
         */
        if ((status == FBE_STATUS_OK) && 
            ((class_id == filter_class_id) || (filter_class_id == FBE_CLASS_ID_INVALID)) &&
            ((port == filter_port) || (filter_port == FBE_API_PHYSICAL_BUS_COUNT)) &&
            ((enclosure == filter_enclosure) || (filter_enclosure == FBE_API_ENCLOSURES_PER_BUS)) &&
            ((drive_no == filter_drive_no) || (filter_drive_no == FBE_API_ENCLOSURE_SLOTS)))
        {
            /* Now that we found it, call our input function to apply some operation 
             * to this object.
             */
            status = (command_function_p)(object_id, context, package_id, display_b);

            if (status != FBE_STATUS_OK)
            {
                fbe_cli_error("%s error on  object 0x%x \n", __FUNCTION__, object_id);
                error_status = status;
            }

            found_objects_count++;

        } /* End if class id fetched OK and it is a match.*/
        else if (status != FBE_STATUS_OK)
        {
            /* Error fetching class id.
             */
            fbe_cli_error("can't get object class_id for id [%x], status: %x\n",
                          object_id, status);
        }
    }/* Loop over all objects found. */

    /* Display our totals.
     */
    fbe_cli_printf("Discovered %.4d objects in total.\n", total_objects);

    /* Only display this total if we have a filter.
     */
    if (filter_class_id != FBE_CLASS_ID_INVALID)
    {
        fbe_cli_printf("Discovered %.4d objects matching filter of class %d.\n", 
                       found_objects_count, filter_class_id);
    }
    /* Free up the memory we allocated for the object list.
     */
    fbe_api_free_memory(object_list_p);

    return error_status;
} // end of fbe_cli_execute_command_for_bms_objects

/*!**************************************************************
 *  fbe_cli_disk_execute_bms_cmd()
 ****************************************************************
 @brief
 *  This function displays the BMS data.
 *
 * @param object_id  - object id
 * @param context    - in this case we do not use the context.
 * @param package_id - package id of that object.
 *
 * @return fbe_status_t status of the operation.
 *
 * @author
 *  
 *
 ****************************************************************/
static void fbe_cli_disk_execute_bms_cmd(fbe_cli_command_bms_fn command_bms_fn, fbe_u32_t context, fbe_u32_t port, fbe_u32_t enclosure, fbe_u32_t slot, fbe_bool_t display_b)
{
    fbe_package_id_t package_id = FBE_PACKAGE_ID_PHYSICAL;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_class_id_t class_id = FBE_CLASS_ID_INVALID;

    /* The operation will be performed to all physical drives.
     */
    status = fbe_cli_execute_command_for_bms_objects(FBE_CLASS_ID_BASE_PHYSICAL_DRIVE,
                                                     package_id,
                                                     port,
                                                     enclosure,
                                                     slot,
                                                     command_bms_fn,
                                                     context, /* logpage */
                                                     display_b);
    if (status != FBE_STATUS_OK)
    {
        fbe_cli_printf("\nPhysical drive command to all drives failed status: 0x%x.\n", status);
        return;
    }
    
    /* Execute the command against all sas physical drives.
     */
    for (class_id = FBE_CLASS_ID_SAS_PHYSICAL_DRIVE_FIRST +1; class_id < FBE_CLASS_ID_SAS_PHYSICAL_DRIVE_LAST; class_id++)
    {
        status = fbe_cli_execute_command_for_bms_objects(class_id,
                                                     package_id,
                                                     port,
                                                     enclosure,
                                                     slot,
                                                     command_bms_fn,
                                                     context, /* logpage */
                                                     display_b);
    
        if (status != FBE_STATUS_OK)
        {
            fbe_cli_printf("\nPhysical drive command to all drives failed status: 0x%x.\n", status);
            return;
        }
    }
    
#ifdef  BLOCKSIZE_512_SUPPORTED
    /* Execute the command against all sata physical drives.
     */
    for (class_id = FBE_CLASS_ID_SATA_PHYSICAL_DRIVE_FIRST +1; class_id < FBE_CLASS_ID_SATA_PHYSICAL_DRIVE_LAST; class_id++)
    {
        status = fbe_cli_execute_command_for_bms_objects(class_id,
                                                     package_id,
                                                     port,
                                                     enclosure,
                                                     slot,
                                                     command_bms_fn,
                                                     context, /* logpage */
                                                     display_b);
    
        if (status != FBE_STATUS_OK)
        {
            fbe_cli_printf("\nPhysical drive command to all drives failed status: 0x%x.\n", status);
            return;
        }
    }
#endif

    return;
} // end of fbe_cli_disk_execute_bms_cmd

/****************************
 * end fbe_cli_lib_disk_info_cmd.c
 ****************************/
