/***************************************************************************
* Copyright (C) EMC Corporation 2010-2011
* All rights reserved.
* Licensed material -- property of EMC Corporation
***************************************************************************/

/*!**************************************************************************
* @file fbe_database_ica_flags_process.c
***************************************************************************
*
* @brief
*  This file contains database service boot path routines to handle the ICA flags
*  
*
***************************************************************************/

/*************************
*   INCLUDE FILES
*************************/
#include "fbe_database_private.h"
#include "fbe_database_config_tables.h"
#include "fbe_imaging_structures.h"
#include "fbe_private_space_layout.h"

/*********************/
/* local definitions */
/*********************/



/*************************************************************************************************************************************************/


/*after processing the ICA flag, we want to clear it so next boot will not zero PVDs again*/
fbe_status_t fbe_database_set_invalidate_configuration_flag(fbe_u32_t flag)
{	
	fbe_imaging_flags_t 	imaging_flags;
    database_physical_drive_info_t drive_location_info[FBE_DB_NUMBER_OF_RESOURCES_FOR_ICA_OPERATION];
    fbe_u32_t       count;
    fbe_u32_t       i;

	/*let's copy the current flags we read a while ago into these blocks*/
	fbe_database_get_imaging_flags(&imaging_flags);
	/*now, invalidate the field we want*/
	imaging_flags.invalidate_configuration = flag;
	fbe_zero_memory(&imaging_flags.images_installed, sizeof(imaging_flags.images_installed));

    /* Always preserve DDBS and other binaries */
    for(i = FBE_IMAGING_IMAGE_TYPE_INVALID+1; i < FBE_IMAGING_IMAGE_TYPE_MAX; i++) {
        imaging_flags.images_installed[i] = FBE_IMAGING_FLAGS_TRUE;
    }
    

        /*Construct which drives do we want to clear the flags on*/
        for(count = 0 ; count < FBE_DB_NUMBER_OF_RESOURCES_FOR_ICA_OPERATION; count++)
        {
            drive_location_info[count].enclosure_number = 0;
            drive_location_info[count].port_number = 0;
            drive_location_info[count].slot_number = count;
            /* Force to write 8 blocks for 4K drive */
            drive_location_info[count].block_geometry = FBE_BLOCK_EDGE_GEOMETRY_4160_4160;
            drive_location_info[count].logical_drive_object_id = FBE_OBJECT_ID_INVALID;
        }

	return fbe_database_write_flags_on_raw_drive(&drive_location_info[0], FBE_DB_NUMBER_OF_RESOURCES_FOR_ICA_OPERATION,
		                                          FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_ICA_FLAGS,
                                                          (fbe_u8_t*)(&imaging_flags), sizeof(imaging_flags));
}

