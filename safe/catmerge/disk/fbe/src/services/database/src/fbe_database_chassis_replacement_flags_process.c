/***************************************************************************
* Copyright (C) EMC Corporation 2010-2011
* All rights reserved.
* Licensed material -- property of EMC Corporation
***************************************************************************/

/*!**************************************************************************
* @file fbe_database_chassis_replacement_flags_process.c
***************************************************************************
*
* @brief
*  This file contains database service boot path routines to handle the chassis replacement flags
*
* @version
*    11/23/2011 - Created. zphu
*
***************************************************************************/

/*************************
*   INCLUDE FILES
*************************/
#include "fbe_database_private.h"
#include "fbe_chassis_replacement_structure.h"      /*This file should be remove @@@@hewei*/
#include "fbe_private_space_layout.h"

static fbe_status_t fbe_database_change_chassis_replacement_flags(fbe_u32_t replacement_value, fbe_bool_t auto_assign_magic_string)
{
    fbe_chassis_replacement_flags_t    replacement_flags;
    database_physical_drive_info_t drive_location_info[FBE_DB_NUMBER_OF_RESOURCES_FOR_CHASSIS_REPLACEMENT_OPERATION];
    fbe_u32_t       count, retry_count;
    fbe_u32_t      magic_match;
    fbe_status_t   status;

    if(FBE_CHASSIS_REPLACEMENT_FLAG_TRUE != replacement_value
       && FBE_CHASSIS_REPLACEMENT_FLAG_INVALID != replacement_value)
    {
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "db_change_chassis_flag: flag value %d is not a valid one!\n", replacement_value);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*let's copy the current flags we read a while ago into these blocks*/
    fbe_database_get_chassis_replacement_flags(&replacement_flags);

    /*if the chassis replacement region has not been recognized by us, then can not clear*/
    if(!auto_assign_magic_string)
    {
        magic_match = fbe_compare_string(replacement_flags.magic_string,
                    FBE_CHASSIS_REPLACEMENT_FLAG_MAGIC_STRING_LENGTH,
                    FBE_CHASSIS_REPLACEMENT_FLAG_MAGIC_STRING,
                    FBE_CHASSIS_REPLACEMENT_FLAG_MAGIC_STRING_LENGTH, FBE_TRUE);
        if(magic_match)
        {
            database_trace (FBE_TRACE_LEVEL_WARNING,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "db_change_chassis_flag: chassis replacement region has not been initialized, can not change the flag\n");
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }
    else
    {
        fbe_copy_memory(replacement_flags.magic_string, FBE_CHASSIS_REPLACEMENT_FLAG_MAGIC_STRING,
                                            FBE_CHASSIS_REPLACEMENT_FLAG_MAGIC_STRING_LENGTH);
        replacement_flags.magic_string[FBE_CHASSIS_REPLACEMENT_FLAG_MAGIC_STRING_LENGTH] = '\0';
    }
    
    /*now, reset the field we want*/
    replacement_flags.chassis_replacement_movement = replacement_value;
    /* any need to increase the replacement_flag's seq_no ? if yes, 
     * make the consistence of the memory and disk*/

        /*Construct which drives do we want to clear the flags on*/
        for(count = 0 ; count < FBE_DB_NUMBER_OF_RESOURCES_FOR_CHASSIS_REPLACEMENT_OPERATION; count++)
        {
            drive_location_info[count].enclosure_number = 0;
            drive_location_info[count].port_number = 0;
            drive_location_info[count].slot_number = count;
            /* Force to write 8 blocks for 4K drive */
            drive_location_info[count].block_geometry = FBE_BLOCK_EDGE_GEOMETRY_4160_4160;
            drive_location_info[count].logical_drive_object_id = FBE_OBJECT_ID_INVALID;
        }

        retry_count = 0;
        /*before change the in-memory flag, change the flags on the drives first*/
        status =  fbe_database_write_flags_on_raw_drive(&drive_location_info[0], 
                                                  FBE_DB_NUMBER_OF_RESOURCES_FOR_CHASSIS_REPLACEMENT_OPERATION,
                                                  FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_CHASSIS_REPLACEMENT_FLAGS,
                                                          (fbe_u8_t*)(&replacement_flags), sizeof(replacement_flags));
        while((status != FBE_STATUS_OK) && (retry_count < FBE_DB_FAILED_RAW_IO_RETRIES)){
            /* Wait 1 second */
            fbe_thread_delay(FBE_DB_FAILED_RAW_IO_RETRY_INTERVAL);
            retry_count++;
            status =  fbe_database_write_flags_on_raw_drive(&drive_location_info[0], 
                                                  FBE_DB_NUMBER_OF_RESOURCES_FOR_CHASSIS_REPLACEMENT_OPERATION,
                                                  FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_CHASSIS_REPLACEMENT_FLAGS,
                                                          (fbe_u8_t*)(&replacement_flags), sizeof(replacement_flags));
        }
        if(FBE_STATUS_OK != status)
        {
            database_trace (FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "db_change_chassis_flag: writing the chassis replacement flags onto the drives failed.\n");
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /*Now change the in-memory flags*/
        return fbe_database_modify_chassis_replacment_flags(replacement_value); 
}

fbe_status_t fbe_database_initialize_chassis_replacement_flags(void)
{    
    return fbe_database_change_chassis_replacement_flags(FBE_CHASSIS_REPLACEMENT_FLAG_INVALID, FBE_TRUE);
}

/*after saving the chassis replacement flag, we want to clear it so next boot will not have this flag set again*/
fbe_status_t fbe_database_clear_chassis_replacement_flags(void)
{    
    return fbe_database_change_chassis_replacement_flags(FBE_CHASSIS_REPLACEMENT_FLAG_INVALID, FBE_FALSE);
}

fbe_status_t fbe_database_set_chassis_replacement_flags(void)
{
    return fbe_database_change_chassis_replacement_flags(FBE_CHASSIS_REPLACEMENT_FLAG_TRUE, FBE_FALSE);
}
