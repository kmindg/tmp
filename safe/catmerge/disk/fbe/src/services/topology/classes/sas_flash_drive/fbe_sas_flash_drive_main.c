/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_sas_flash_drive_main.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the main entry points for the sas flash drive object.
 * 
 * HISTORY
 *   2/22/2010:  Created. chenl6
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_memory.h"
#include "fbe/fbe_transport.h"

#include "fbe_transport_memory.h"
#include "fbe_topology.h"
#include "fbe_scheduler.h"

#include "fbe_sas_port.h"
#include "sas_physical_drive_private.h"
#include "sas_flash_drive_private.h"
#include "fbe_scsi.h"


/* Class methods forward declaration */
fbe_status_t sas_flash_drive_load(void);
fbe_status_t sas_flash_drive_unload(void);


/* Export class methods  */
fbe_class_methods_t fbe_sas_flash_drive_class_methods = {FBE_CLASS_ID_SAS_FLASH_DRIVE,
													sas_flash_drive_load,
													sas_flash_drive_unload,
													fbe_sas_flash_drive_create_object,
													fbe_sas_flash_drive_destroy_object,
													fbe_sas_flash_drive_control_entry,
													fbe_sas_flash_drive_event_entry,
													fbe_sas_flash_drive_io_entry,
													fbe_sas_flash_drive_monitor_entry};


/*!***************************************************************
 * sas_flash_drive_load()
 ****************************************************************
 * @brief
 *  This function is called to construct any
 *  global data for the class.
 *
 * @param  - None.
 *
 * @return fbe_status_t
 *
 * @author
 *  2/22/2010 - Created. chenl6
 *
 ****************************************************************/
fbe_status_t 
sas_flash_drive_load(void)
{
	FBE_ASSERT_AT_COMPILE_TIME(sizeof(fbe_sas_flash_drive_t) < FBE_MEMORY_CHUNK_SIZE);
	/* KvTrace("%s object size %d\n", __FUNCTION__, sizeof(fbe_sas_flash_drive_t)); */

	return fbe_sas_flash_drive_monitor_load_verify();
}

/*!***************************************************************
 * sas_flash_drive_unload()
 ****************************************************************
 * @brief
 *  This function is called to destruct any
 *  global data for the class.
 *
 * @param  - None.
 *
 * @return fbe_status_t
 *
 * @author
 *  2/22/2010 - Created. chenl6
 *
 ****************************************************************/
fbe_status_t 
sas_flash_drive_unload(void)
{
	return FBE_STATUS_OK;
}

/*!***************************************************************
 * fbe_sas_flash_drive_create_object(fbe_packet_t * packet, 
 *                                 fbe_object_handle_t * object_handle)
 ****************************************************************
 * @brief
 *  This function is called to create an instance of this class.
 *
 * @param packet - The packet used to construct the object.  
 * @param object_handle - The object being created.
 *
 * @return fbe_status_t
 *  The status of the operation typically FBE_STATUS_OK.
 *
 * @author
 *  2/22/2010 - Created. chenl6
 *
 ****************************************************************/
fbe_status_t 
fbe_sas_flash_drive_create_object(fbe_packet_t * packet, fbe_object_handle_t * object_handle)
{
    fbe_sas_flash_drive_t * sas_flash_drive;
    fbe_status_t status;

    /* Call parent constructor */
    status = fbe_sas_physical_drive_create_object(packet, object_handle);
    if(status != FBE_STATUS_OK){
        return status;
    }

    /* Set class id */
    sas_flash_drive = (fbe_sas_flash_drive_t *)fbe_base_handle_to_pointer(*object_handle);
    fbe_base_object_set_class_id((fbe_base_object_t *) sas_flash_drive, FBE_CLASS_ID_SAS_FLASH_DRIVE);  

	fbe_sas_flash_drive_init(sas_flash_drive);

    return FBE_STATUS_OK;
}

/*!***************************************************************
 * fbe_sas_flash_drive_init(fbe_sas_flash_drive_t * sas_flash_drive)
 ****************************************************************
 * @brief
 *  This function initializes the sas flash drive object.
 *
 * @param sas_flash_drive - pointer to drive object.
 *
 * @return fbe_status_t
 *
 * @author
 *  2/22/2010 - Created. chenl6
 *
 ****************************************************************/
fbe_status_t
fbe_sas_flash_drive_init(fbe_sas_flash_drive_t * sas_flash_drive)
{
    fbe_base_physical_drive_t *base_physical_drive = (fbe_base_physical_drive_t *)sas_flash_drive;
    fbe_base_physical_drive_info_t * info_ptr = & base_physical_drive->drive_info;

	/* Initialize */
	fbe_zero_memory(&sas_flash_drive->vpd_pages_info, sizeof(fbe_vpd_pages_info_t));

	/* We want to have larger Q depth for flash drives */
	fbe_base_physical_drive_set_outstanding_io_max((fbe_base_physical_drive_t *) sas_flash_drive, FBE_SAS_FLASH_DRIVE_OUTSTANDING_IO_MAX);
	info_ptr->drive_qdepth = FBE_SAS_FLASH_DRIVE_OUTSTANDING_IO_MAX;

    /* Override get_vendor_table virtual function, which is used when setting default mode pages. */
    sas_flash_drive->sas_physical_drive.get_vendor_table = fbe_flash_drive_get_vendor_table;

	return FBE_STATUS_OK;
}


fbe_sas_drive_status_t
fbe_flash_drive_get_vendor_table(fbe_sas_physical_drive_t * sas_physical_drive, fbe_drive_vendor_id_t drive_vendor_id, fbe_vendor_page_t ** table_ptr, fbe_u16_t * num_table_entries)
{
    fbe_sas_drive_status_t status = FBE_SAS_DRIVE_STATUS_OK;

    if (sas_physical_drive->sas_drive_info.use_additional_override_tbl)
    {
        *table_ptr = fbe_cli_mode_page_override_tbl;
        *num_table_entries = fbe_cli_mode_page_override_entries;
        return FBE_SAS_DRIVE_STATUS_OK;
    }

    switch (drive_vendor_id) {
        case FBE_DRIVE_VENDOR_SAMSUNG:
        case FBE_DRIVE_VENDOR_SAMSUNG_2:            
        case FBE_DRIVE_VENDOR_ANOBIT:        
        case FBE_DRIVE_VENDOR_TOSHIBA:  
             *table_ptr = fbe_sam_sas_vendor_tbl;     /*this should really be called the default flash drive table*/
             *num_table_entries = FBE_MS_MAX_SAM_SAS_TBL_ENTRIES_COUNT;
            break;
        default:
            /* Let parent class set the vendor table. */
            status = fbe_sas_physical_drive_get_vendor_table(sas_physical_drive,drive_vendor_id,table_ptr,num_table_entries);
    }

    return status;
}


/*!***************************************************************
 * fbe_sas_flash_drive_destroy_object()
 ****************************************************************
 * @brief
 *  This function is the interface function, which is called
 *  by the topology to destroy an instance of this class.
 *
 * @param object_handle - The object being destroyed.
 *
 * @return fbe_status_t
 *  STATUS of the destroy operation.
 *
 * @author
 *  2/22/2010 - Created. chenl6
 *
 ****************************************************************/
fbe_status_t 
fbe_sas_flash_drive_destroy_object( fbe_object_handle_t object_handle)
{
	fbe_status_t status;
	fbe_sas_flash_drive_t * sas_flash_drive;
	
	sas_flash_drive = (fbe_sas_flash_drive_t *)fbe_base_handle_to_pointer(object_handle);

	fbe_base_object_trace(	(fbe_base_object_t*)sas_flash_drive,
							FBE_TRACE_LEVEL_DEBUG_HIGH,
							FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
							"%s entry.\n", __FUNCTION__);

	/* Check parent edges */
	/* Cleanup */

	/* Call parent destructor */
	status = fbe_sas_physical_drive_destroy_object(object_handle);
	return status;
}
