/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_sata_flash_drive_main.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the main entry points for the sata flash drive object.
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
#include "sata_physical_drive_private.h"
#include "sata_flash_drive_private.h"
#include "fbe_scsi.h"

/* Class methods forward declaration */
fbe_status_t sata_flash_drive_load(void);
fbe_status_t sata_flash_drive_unload(void);


/* Export class methods  */
fbe_class_methods_t fbe_sata_flash_drive_class_methods = {FBE_CLASS_ID_SATA_FLASH_DRIVE,
													sata_flash_drive_load,
													sata_flash_drive_unload,
													fbe_sata_flash_drive_create_object,
													fbe_sata_flash_drive_destroy_object,
													fbe_sata_flash_drive_control_entry,
													fbe_sata_flash_drive_event_entry,
													fbe_sata_flash_drive_io_entry,
													fbe_sata_flash_drive_monitor_entry};


/*!***************************************************************
 * sata_flash_drive_load()
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
sata_flash_drive_load(void)
{
	FBE_ASSERT_AT_COMPILE_TIME(sizeof(fbe_sata_flash_drive_t) < FBE_MEMORY_CHUNK_SIZE);
	/* KvTrace("%s object size %d\n", __FUNCTION__, sizeof(fbe_sata_flash_drive_t)); */

	return fbe_sata_flash_drive_monitor_load_verify();
}

/*!***************************************************************
 * sata_flash_drive_unload()
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
sata_flash_drive_unload(void)
{
	return FBE_STATUS_OK;
}

/*!***************************************************************
 * fbe_sata_flash_drive_create_object(fbe_packet_t * packet, 
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
fbe_sata_flash_drive_create_object(fbe_packet_t * packet, fbe_object_handle_t * object_handle)
{
    fbe_sata_flash_drive_t * sata_flash_drive;
    fbe_status_t status;

    /* Call parent constructor */
    status = fbe_sata_physical_drive_create_object(packet, object_handle);
    if(status != FBE_STATUS_OK){
        return status;
    }

    /* Set class id */
    sata_flash_drive = (fbe_sata_flash_drive_t *)fbe_base_handle_to_pointer(*object_handle);
    fbe_base_object_set_class_id((fbe_base_object_t *) sata_flash_drive, FBE_CLASS_ID_SATA_FLASH_DRIVE);  

	fbe_sata_flash_drive_init(sata_flash_drive);

    return FBE_STATUS_OK;
}

/*!***************************************************************
 * fbe_sata_flash_drive_init(fbe_sata_flash_drive_t * sata_flash_drive)
 ****************************************************************
 * @brief
 *  This function initializes the sas flash drive object.
 *
 * @param sata_flash_drive - pointer to drive object.
 *
 * @return fbe_status_t
 *
 * @author
 *  2/22/2010 - Created. chenl6
 *
 ****************************************************************/
fbe_status_t
fbe_sata_flash_drive_init(fbe_sata_flash_drive_t * sata_flash_drive)
{
	return FBE_STATUS_OK;
}

/*!***************************************************************
 * fbe_sata_flash_drive_destroy_object()
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
fbe_sata_flash_drive_destroy_object( fbe_object_handle_t object_handle)
{
	fbe_status_t status;
	fbe_sata_flash_drive_t * sata_flash_drive;
	
	sata_flash_drive = (fbe_sata_flash_drive_t *)fbe_base_handle_to_pointer(object_handle);

	fbe_base_object_trace(	(fbe_base_object_t*)sata_flash_drive,
							FBE_TRACE_LEVEL_DEBUG_HIGH,
							FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
							"%s entry.\n", __FUNCTION__);

	/* Check parent edges */
	/* Cleanup */

	/* Call parent destructor */
	status = fbe_sata_physical_drive_destroy_object(object_handle);
	return status;
}
