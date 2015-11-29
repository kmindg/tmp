/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_disk_block_correct.c
 *
 * @brief
 *  This file contains functions for correcting bad blocks
 *
 * @version
 *   2012-10-22 - Created. Socheavy Heng
 *
 ***************************************************************************/

/*
 * INCLUDE FILES
 */
#include "fbe/fbe_disk_block_correct.h"
#include <limits.h>
#include "flare_export_ioctls.h"

/* This global variable will contain all the records that
 * need to be corrected
 */
static fbe_disk_block_correct_record_t records[FBE_DISK_BLOCK_CORRECT_RECORDS_MAX];


/*!**************************************************************
 * fbe_disk_block_correct_execute()
 ****************************************************************
 * @brief
 *  Execute the bad blocks command as specified by the user
 *
 * @param 
 *  bb_command - the command parameters to execute
 *
 * @return 
 *  status - fbe status
 *
 * @author
 *  10/22/2012 - Created. Deanna Heng
 *
 ****************************************************************/
fbe_status_t fbe_disk_block_correct_execute(fbe_bad_blocks_cmd_t * bb_command) 
{
    fbe_status_t status = FBE_STATUS_OK;
    if (bb_command->command & FBE_BAD_BLOCKS_COMMAND_REPORT) {
        // Go through the event log,  populate the records to correct,
        // and create the report file
        status = fbe_disk_block_correct_report(bb_command);
        if (status != FBE_STATUS_OK) {
            return status;
        }
    }

    if (bb_command->command & FBE_BAD_BLOCKS_COMMAND_CLEAN) {
        if (!(bb_command->command & FBE_BAD_BLOCKS_COMMAND_REPORT)) {
            // if the report parameter was not specified we will need
            // the records from the specified report file 
            fbe_disk_block_correct_read_records_from_file(bb_command->file); 
        }

        fbe_disk_block_correct_clean();
    }

    return status;
}

/*!**************************************************************
 * fbe_disk_block_correct_report()
 ****************************************************************
 * @brief
 *  Execute the report command to process the event log error messages
 *
 * @param 
 *  bb_command - the command given by the user
 *
 * @return 
 *  status - fbe status
 *
 * @author
 *  10/22/2012 - Created. Deanna Heng
 *
 ****************************************************************/
fbe_status_t fbe_disk_block_correct_report(fbe_bad_blocks_cmd_t * bb_command) {
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t event_log_index;
    fbe_event_log_statistics_t event_log_statistics = {0};
    fbe_event_log_get_event_log_info_t event_log = {0};
    fbe_object_id_t raid_group_id = FBE_OBJECT_ID_INVALID;
    fbe_lba_t disk_pba = FBE_LBA_INVALID;
    fbe_u32_t blocks = 0;
    fbe_u32_t position = 0;
    fbe_u32_t error_info = 0;
    fbe_u32_t extra_info = 0;
    fbe_u32_t num_fields_matched = 0;
    fbe_u32_t events_to_search = FBE_EVENT_LOG_MESSAGE_COUNT;

    // FBE_OBJECT_ID_INVALID is the anchor used to specify the end of the list of records
    records[0].lun_object_id = FBE_OBJECT_ID_INVALID;
    status = fbe_api_get_event_log_statistics(&event_log_statistics, FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // The max number of event log messages is FBE_EVENT_LOG_MESSAGE_COUNT
    printf("Processing %d event log messages\n", event_log_statistics.total_msgs_logged);
    if (event_log_statistics.total_msgs_logged < FBE_EVENT_LOG_MESSAGE_COUNT) {
        events_to_search = event_log_statistics.total_msgs_logged;
    }

    // Go through each event log message to search for SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED
    for (event_log_index = 0; event_log_index < events_to_search; event_log_index++)
    {
        event_log.event_log_index = event_log_index;
        status = fbe_api_get_event_log(&event_log, FBE_PACKAGE_ID_SEP_0);
        if (status != FBE_STATUS_OK) {
            printf("ERROR getting event log at index %d\n", event_log_index);
        }

        if (status == FBE_STATUS_OK &&
            event_log.event_log_info.msg_id == SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED) {
            printf("Proccessing %s\n", event_log.event_log_info.msg_string);
            // If the user has specified the start / end datetimes then filter out events
            // not within those dates
            if (fbe_disk_block_correct_is_within_dates(bb_command, &event_log.event_log_info.time_stamp)) {
                raid_group_id = FBE_OBJECT_ID_INVALID;
                blocks = 0;
                num_fields_matched = sscanf(event_log.event_log_info.msg_string,
                                            FBE_BAD_BLOCKS_EVENT_MSG,
                                            &raid_group_id,
                                            &position,
                                            &disk_pba,
                                            &blocks,
                                            &error_info,
                                            &extra_info);

                // if the number of fields matched is not exact, there was an error during parsing
                if (num_fields_matched != FBE_BAD_BLOCKS_EVENT_MSG_NUM_FIELDS ||
                    raid_group_id == FBE_OBJECT_ID_INVALID || blocks == 0) {
                    printf("ERROR Raidgroup 0x%x is invalid blocks 0x%x\n", raid_group_id, blocks);
                    return FBE_STATUS_GENERIC_FAILURE;
                }

                // Convert the event log record to a lun record we can use to zero sectors
                printf("Process record raidgroup: 0x%x position: 0x%x lba: 0x%llx blocks: 0x%x\n",
                       raid_group_id, position, (unsigned long long)disk_pba, blocks);
                fbe_disk_block_correct_add_record(raid_group_id, position, disk_pba, blocks);
            }
        }
    }

    // print the relevant records to the specified output file
    fbe_disk_block_correct_print_output(bb_command->file);

    return status;
}

/*!**************************************************************
 * fbe_disk_block_correct_clean()
 ****************************************************************
 * @brief
 *  Execute the clean command for all the records we have processed
 *
 * @param 
 *  None
 *
 * @return 
 *  None
 *
 * @author
 *  10/22/2012 - Created. Deanna Heng
 *
 ****************************************************************/
void fbe_disk_block_correct_clean(void) {
    fbe_u32_t i;

    // Attempt to clean every record
    for (i = 0; i < FBE_DISK_BLOCK_CORRECT_RECORDS_MAX; i++) {
        if (records[i].lun_object_id == FBE_OBJECT_ID_INVALID) {
            return;
        }
        fbe_disk_block_correct_clean_record(&records[i]);
    }
}

/*!**************************************************************
 * fbe_disk_block_correct_clean_record()
 ****************************************************************
 * @brief
 *  Clean the record
 *
 * @param 
 *  lunDeviceHandle - handle to the lun device
 *  lun_lba - the uncorrectable sector to zero
 *
 * @return 
 *  status - fbe status
 *
 * @author
 *  10/22/2012 - Created. Deanna Heng
 *
 ****************************************************************/
void fbe_disk_block_correct_clean_record(fbe_disk_block_correct_record_t * record) {
    EMCUTIL_RDEVICE_REFERENCE lunDeviceHandle = EMCUTIL_DEVICE_REFERENCE_INVALID;
    fbe_char_t lunDeviceName[64];
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t i = 0;

    sprintf(lunDeviceName, "%s%d", FBE_BAD_BLOCKS_DEVICE_PREFIX, record->lun_number);
    status = fbe_disk_block_correct_open_lun_device(lunDeviceName, &lunDeviceHandle);
    if (status != FBE_STATUS_OK) {
        printf("ERROR in opening lun device %s\n", lunDeviceName);
    }

    // Clean all the blocks for this record
    for (i = 0; i< record->blocks; i++) {
        printf("Reading lun device %s lun id 0x%x at lba 0x%llx\n", 
               lunDeviceName, record->lun_object_id, (unsigned long long)record->lun_lba+i);
        status = fbe_disk_block_correct_read_block(lunDeviceHandle, record->lun_lba+i);
        // This block is really corrupted. Make sure the block is still corrupted
        // prior to zeroing out the sector
        if (status == FBE_STATUS_GENERIC_FAILURE) {

            // Cache should be disabled on the lun in order to flush to disk
            //fbe_disk_block_correct_disable_lun_cache(record, lunDeviceHandle);
            printf("Writing lun device %s lun id 0x%x at lba 0x%llx\n", 
                   lunDeviceName, record->lun_object_id, (unsigned long long)record->lun_lba+i);
            // zero out the uncorrectable sector
            fbe_disk_block_correct_write_block(lunDeviceHandle, record->lun_lba+i);

            // Verify that there is no longer an error after zeroing the uncorrectable sector
            status = fbe_disk_block_correct_read_block(lunDeviceHandle, record->lun_lba+i);
            if (status == FBE_STATUS_GENERIC_FAILURE) {
                printf("ERROR Reading lun device %s lun id 0x%x at lba 0x%llx\n", 
                       lunDeviceName, record->lun_object_id, (unsigned long long)record->lun_lba+i);
            } else {
                printf("SUCCESS in zeroing sector!!!");
            }
            //fbe_disk_block_correct_enable_lun_cache(record, lunDeviceHandle);
        }
    }

    fbe_disk_block_correct_close_lun_device(lunDeviceHandle);
}

/*!**************************************************************
 * fbe_disk_block_correct_read_block()
 ****************************************************************
 * @brief
 *  Read the uncorrectable sector of the lun device
 *
 * @param 
 *  lunDeviceHandle - handle to the lun device
 *  lun_lba - the uncorrectable sector to zero
 *
 * @return 
 *  status - fbe status
 *
 * @author
 *  10/22/2012 - Created. Deanna Heng
 *
 ****************************************************************/
fbe_status_t fbe_disk_block_correct_read_block(EMCUTIL_RDEVICE_REFERENCE lunDeviceHandle, 
                                               fbe_lba_t lun_lba) {
    fbe_lba_t offset = 0;
    fbe_char_t buffer[FBE_DISK_BLOCK_CORRECT_BYTES_PER_BLOCK];
    size_t bytes_read = 0;
    EMCUTIL_STATUS status;

    offset = lun_lba * FBE_DISK_BLOCK_CORRECT_BYTES_PER_BLOCK;
    status = EmcutilRemoteDeviceRead (lunDeviceHandle,  // handle to device
                                      offset,
                                      &buffer,
                                      FBE_DISK_BLOCK_CORRECT_BYTES_PER_BLOCK,
                                      &bytes_read);

    if (!EMCUTIL_SUCCESS(status)) {
        printf("An error was found reading from block at 0x%llx 00x%x\n", lun_lba, (unsigned int)bytes_read);
        return FBE_STATUS_GENERIC_FAILURE;
    } else {
        printf("An error was NOT found reading from block at 0x%llx\n", lun_lba);
        return FBE_STATUS_OK;
    }
}

/*!**************************************************************
 * fbe_disk_block_correct_write_block()
 ****************************************************************
 * @brief
 *  Write zeros to the uncorrectable sector
 *
 * @param 
 *  lunDeviceHandle - handle to the lun device to write to
 *  lun_lba - the uncorrectable sector to zero
 *
 * @return 
 *  status - fbe status
 *
 * @author
 *  10/22/2012 - Created. Deanna Heng
 *
 ****************************************************************/
void fbe_disk_block_correct_write_block(EMCUTIL_RDEVICE_REFERENCE lunDeviceHandle, 
                                        fbe_lba_t lun_lba) {
    fbe_lba_t offset = 0;
    fbe_char_t * buffer;
    size_t bytes_returned = 0;

    buffer = (char*) malloc (FBE_DISK_BLOCK_CORRECT_BYTES_PER_BLOCK);
    fbe_set_memory(buffer, 0, FBE_DISK_BLOCK_CORRECT_BYTES_PER_BLOCK);
    offset = lun_lba * FBE_DISK_BLOCK_CORRECT_BYTES_PER_BLOCK;

    EmcutilRemoteDeviceWrite (lunDeviceHandle,  // handle to device
                              offset,
                              buffer,
                              FBE_DISK_BLOCK_CORRECT_BYTES_PER_BLOCK,
                              &bytes_returned);
    printf("Wrote zeros to lba 0x%llx\n", lun_lba);
    free(buffer);
}

/*!**************************************************************
 * fbe_disk_block_correct_read_records_from_file()
 ****************************************************************
 * @brief
 *  Given the report filename proces each record in the file
 *
 * @param 
 *  filename - the report filename
 *
 * @return 
 *  status - fbe status
 *
 * @author
 *  10/22/2012 - Created. Deanna Heng
 *
 ****************************************************************/
void fbe_disk_block_correct_read_records_from_file(fbe_char_t * filename) 
{
    FILE * fp = NULL;
    fbe_char_t sn[RESUME_PROM_EMC_SUB_ASSY_SERIAL_NUM_SIZE];
    fbe_object_id_t lun_id = FBE_OBJECT_ID_INVALID;
    fbe_u32_t lun_num = 0;
    fbe_lba_t lun_lba = FBE_LBA_INVALID;
    fbe_u32_t blocks = 0;
    fbe_u32_t position = 0;
    fbe_object_id_t rg_id = FBE_OBJECT_ID_INVALID;
    fbe_u32_t rg_num = 0;
    fbe_u32_t i = 0;
    fbe_char_t wwn[64];
    fbe_disk_block_correct_get_read_file_handle(filename, &fp);

    while(feof(fp) == 0 &&
          i < FBE_DISK_BLOCK_CORRECT_RECORDS_MAX) {
        
        fscanf(fp, "sn: %s lun id: 0x%x lun num: %d lun lba: 0x%llx blocks: 0x%x wwn: %s rg id: 0x%x rg num: %d position: %d\n",
               (char*)&sn,
               &lun_id,
               &lun_num,
               &lun_lba, 
               &blocks,
               (char*)&wwn,
               &rg_id,
               &rg_num,
               &position);
        
        printf("Record sn: %s lun id: 0x%x lun num: %d lun lba: 0x%llx blocks: 0x%x wwn: %s rg id: 0x%x rg num: %d position: %d\n",
               sn,
               lun_id,
               lun_num,
               lun_lba, 
               blocks,
               wwn,
               rg_id,
               rg_num,
               position);
        records[i].lun_object_id = lun_id;
        records[i].lun_number = lun_num;
        records[i].lun_lba = lun_lba;
        records[i].blocks = blocks;
        records[i].rg_object_id = rg_id;
        records[i].rg_number = rg_num;
        records[i].position = position;
        i++;
    }

    // There is no place to set this flag if i == FBE_DISK_BLOCK_CORRECT_RECORDS_MAX
    if (i < FBE_DISK_BLOCK_CORRECT_RECORDS_MAX) {
        records[i].lun_object_id = FBE_OBJECT_ID_INVALID;
    }
    fbe_disk_block_correct_close_file(fp);
}

/*!**************************************************************
 * fbe_disk_block_correct_add_record()
 ****************************************************************
 * @brief
 *  Given the parameters from the event log add the lun records to 
 *  process as necessary 
 *
 * @param 
 *  rg_object_id - pointer to the raid group object id in the event log
 *  position - position of the disk within the raid group
 *  pba - the uncorrectable logical pba of the disk 
 *  blocks - the number of uncorrectable blocks
 *
 * @return 
 *  status - fbe status
 *
 * @author
 *  10/22/2012 - Created. Deanna Heng
 *
 ****************************************************************/
fbe_status_t fbe_disk_block_correct_add_record(fbe_object_id_t raid_group_id,
                                               fbe_u16_t position,
                                               fbe_lba_t pba, 
                                               fbe_u32_t blocks) {

    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_raid_group_map_info_t map_info;
    fbe_api_base_config_upstream_object_list_t upstream_object_list;
    fbe_u32_t i = 0;
    fbe_database_lun_info_t lun_info;
    fbe_api_raid_group_get_info_t rg_info;

    status = fbe_api_raid_group_get_info(raid_group_id,
                                         &rg_info, 
                                         FBE_PACKET_FLAG_NO_ATTRIB);
    if (status != FBE_STATUS_OK) {
        return status;
    }

    status = fbe_disk_block_correct_get_upstream_objects(&raid_group_id,
                                                         &upstream_object_list,
                                                         &position);
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // If there are no luns in the raid group we cannot write any zeros
    if (upstream_object_list.number_of_upstream_objects == 0) {
        printf("Raid Group contains no luns::: raidgroup: 0x%x position: 0x%x lba: 0x%llx blocks: 0x%x\n",
               raid_group_id, position, (unsigned long long)pba, blocks);
        return FBE_STATUS_OK;
    }
    
    // Convert the disk pba to a lun lba
    map_info.pba = pba + rg_info.physical_offset;
    map_info.position = position;
    status = fbe_api_raid_group_map_pba(raid_group_id, &map_info);
    if (status != FBE_STATUS_OK) {
        return status;
    }

    // If the record is for a parity position, ignore the records since we cannot explicitly
    // write to this sector anyway
    if (map_info.parity_pos == position) {
        printf("Ignore parity sector::: raidgroup: 0x%x position: 0x%x lba: 0x%llx blocks: 0x%x\n",
               raid_group_id, position, (unsigned long long)pba, blocks);
        return FBE_STATUS_OK;
    }

    // Go through the luns in the raid group
    for (i=0; i<upstream_object_list.number_of_upstream_objects; i++) {
        lun_info.lun_object_id = upstream_object_list.upstream_object_list[i];
        status = fbe_api_database_get_lun_info(&lun_info);
        if (status != FBE_STATUS_OK) {
            return status;
        }
        // if the target lba is within this lun's consumed area, this is the target lun
        if (lun_info.offset <= map_info.lba && 
            lun_info.offset + lun_info.capacity >= map_info.lba) {
            // Now add the lun record with lun lba if it is unique
            fbe_disk_block_correct_add_lun_record(&lun_info,
                                                  map_info.lba - lun_info.offset,
                                                  position,
                                                  blocks);
            return FBE_STATUS_OK;
        }
    }
    printf("No matching lun was found\n");
    return FBE_STATUS_GENERIC_FAILURE;
}

/*!**************************************************************
 * fbe_disk_block_correct_add_lun_record()
 ****************************************************************
 * @brief
 *  Add the lun record to the list of records
 *
 * @param 
 *  lun_info - lun information
 *  lba - the lba of the lun
 *  position - position of the target disk in the raid group
 *  blocks - the number of blocks to process
 *
 * @return 
 *  status - fbe status
 *
 * @author
 *  10/22/2012 - Created. Deanna Heng
 *
 ****************************************************************/
void fbe_disk_block_correct_add_lun_record(fbe_database_lun_info_t * lun_info,
                                           fbe_lba_t lba,
                                           fbe_u16_t position,
                                           fbe_u32_t blocks) 
{
    fbe_u32_t i;
    for (i = 0; i < FBE_DISK_BLOCK_CORRECT_RECORDS_MAX; i++) {
        if (records[i].lun_object_id == FBE_OBJECT_ID_INVALID) {
            records[i].lun_object_id = lun_info->lun_object_id;
            records[i].lun_number = lun_info->lun_number;
            records[i].lun_lba = lba;
            records[i].world_wide_name = lun_info->world_wide_name;
            records[i].offset = lun_info->offset;
            records[i].rg_object_id = lun_info->raid_group_obj_id;
            records[i].rg_number = lun_info->rg_number;
            records[i].position = position;
            records[i].blocks = blocks;

            // set the next record to be invalid so we insert there
            if (i < FBE_DISK_BLOCK_CORRECT_RECORDS_MAX - 1) {
                // FBE_OBJECT_ID_INVALID is the anchor specifying the last of the records
                records[++i].lun_object_id = FBE_OBJECT_ID_INVALID;
            }
            printf("Adding record lun: 0x%x lba: 0x%llx raidgroup: 0x%x position: 0x%x\n",
                   lun_info->lun_object_id, (unsigned long long)lba, lun_info->raid_group_obj_id, position);
            return;
        } 

        if (records[i].lun_object_id == lun_info->lun_object_id &&
            records[i].rg_object_id == lun_info->raid_group_obj_id &&
            records[i].lun_lba == lba &&
            records[i].position == position &&
            records[i].blocks == blocks) {
            printf("Redundant record lun: 0x%x lba: 0x%llx raidgroup: 0x%x position: 0x%x\n",
                   lun_info->lun_object_id, (unsigned long long)lba, lun_info->raid_group_obj_id, position);
            return;
        }
    }
}

/*!**************************************************************
 * fbe_disk_block_correct_get_upstream_objects()
 ****************************************************************
 * @brief
 *  Make adjustments if the raidgroup is a mirror within a r10 raid group
 *  and return the luns as the upstream objects list 
 *
 * @param 
 *  rg_object_id - pointer to the raid group object id in the event log
 *  upstream_object_list - pointer to the raid group luns
 *  position - position of the disk within the raid group
 *
 * @return 
 *  status - fbe status
 *
 * @author
 *  10/22/2012 - Created. Deanna Heng
 *
 ****************************************************************/
fbe_status_t fbe_disk_block_correct_get_upstream_objects(fbe_object_id_t * rg_object_id,
                                                         fbe_api_base_config_upstream_object_list_t * upstream_object_list,
                                                         fbe_u16_t * position) {

    fbe_api_base_config_downstream_object_list_t downstream_object_list;
    fbe_class_id_t                              class_id;
    fbe_object_id_t   upstream_object_id = FBE_OBJECT_ID_INVALID;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t i;

    // Make api call:
    status = fbe_api_base_config_get_upstream_object_list(*rg_object_id, upstream_object_list);
    if (status != FBE_STATUS_OK) {
        return status;
    }
    /* First, check to see if the raid group is a mirror of a r10 
     * raid group and then convert from there.
     */
    if (upstream_object_list->number_of_upstream_objects == 1) {
        upstream_object_id = upstream_object_list->upstream_object_list[0];
        /* Get the class id of the upstream objects*/
        status = fbe_api_get_object_class_id(upstream_object_id, &class_id, FBE_PACKAGE_ID_SEP_0);
        if (status != FBE_STATUS_OK) {
            printf("ERROR getting class id on object 0x%x\n", upstream_object_id);
            return status;
        }
        // this is a raid10 raid group
        if (class_id == FBE_CLASS_ID_RAID_GROUP) {
            // convert the position to the correct position
            status = fbe_api_base_config_get_downstream_object_list(upstream_object_id, &downstream_object_list);
            if (status != FBE_STATUS_OK) {
                printf("ERROR getting downstream objects on 0x%x\n", upstream_object_id);
                return status;
            }
            for (i=0; i< downstream_object_list.number_of_downstream_objects; i++) {
                if (downstream_object_list.downstream_object_list[i] == *rg_object_id) {
                    *position = i * 2;
                    *rg_object_id = upstream_object_id;
                    break;
                }
            }
            // set up the upstream object list of luns for next calls
            status = fbe_api_base_config_get_upstream_object_list(*rg_object_id, upstream_object_list);
            if (status != FBE_STATUS_OK) {
                printf("ERROR getting upstream object list on rg 0x%x\n", *rg_object_id);
                return status;
            }
        }
    } 

    return status;
}

/*!**************************************************************
 * fbe_disk_block_correct_print_output()
 ****************************************************************
 * @brief
 *  Print the records to the report file
 *
 * @param 
 *  filename - the name of the report file
 *
 * @return 
 *  None
 *
 * @author
 *  10/22/2012 - Created. Deanna Heng
 *
 ****************************************************************/
void fbe_disk_block_correct_print_output(fbe_char_t * filename) {
    fbe_u32_t i = 0;
    FILE * fp = NULL;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_char_t wwn[64];
    fbe_char_t sn[RESUME_PROM_EMC_SUB_ASSY_SERIAL_NUM_SIZE];

    // open the file for writing
    status = fbe_disk_block_correct_get_write_file_handle(filename, &fp);
    if(status != FBE_STATUS_OK)
    {
        printf("ERROR opening file: %s\n", filename);
        return;
    }

    //read and show system id info
    status = fbe_disk_block_correct_get_arraysn(sn);
    if(status != FBE_STATUS_OK)
    { 
        fbe_disk_block_correct_close_file(fp);
        return;
    }

    // Go through all the unique lun records 
    for (i = 0; i < FBE_DISK_BLOCK_CORRECT_RECORDS_MAX; i++) {
        if (records[i].lun_object_id == FBE_OBJECT_ID_INVALID) {
            printf("%d records written to file %s\n", i, filename);
            break;
        }

        
        fbe_disk_block_correct_get_wwn_string(records[i].world_wide_name, &wwn[0]); 
        fprintf(fp, "sn: %.16s lun id: 0x%x lun num: %d lun lba: 0x%llx blocks: 0x%x wwn: %s rg id: 0x%x rg num: %d position: %d\n", 
                sn,
                records[i].lun_object_id,
                records[i].lun_number,
                records[i].lun_lba,
                records[i].blocks,
                wwn,
                records[i].rg_object_id,
                records[i].rg_number,
                records[i].position);
    }

    status = fbe_disk_block_correct_close_file(fp);
}

/*!**************************************************************
 * fbe_disk_block_correct_is_within_dates()
 ****************************************************************
 * @brief
 *  Determine if the timestamp is with the start and end datetimes 
 *  of the bb_command
 *
 * @param 
 *  bb_command - the command containing the start and end datetimes
 *  timestamp - the timestamp to compare to
 *
 * @return 
 *  fbe_bool_t - true if the timestamp is within the dates and false otherwise
 *
 * @author
 *  10/22/2012 - Created. Deanna Heng
 *
 ****************************************************************/
fbe_bool_t fbe_disk_block_correct_is_within_dates(fbe_bad_blocks_cmd_t * bb_command, fbe_system_time_t * timestamp) {
    time_t datetime;
    struct tm ltime;
    memset(&ltime, 0, sizeof(ltime));   /* get any nonstandard fields */
    ltime.tm_year = timestamp->year - 1900;
    ltime.tm_mon = timestamp->month;
    ltime.tm_mday = timestamp->day;
    ltime.tm_hour = timestamp->hour;
    ltime.tm_min = timestamp->minute;
    ltime.tm_sec = timestamp->second;
    ltime.tm_isdst = 0;
    datetime = mktime(&ltime);
    printf("%llu <= %llu <= %llu? ", (unsigned long long)bb_command->startdate, (unsigned long long)datetime, (unsigned long long)bb_command->enddate );
    if (bb_command->startdate != NULL &&
        bb_command->startdate > datetime) {
        printf("startdate %llu is greater than datetime %llu\n", (unsigned long long)bb_command->startdate, (unsigned long long)datetime);
        return FBE_FALSE;
    }

    if (bb_command->enddate !=NULL && 
        bb_command->enddate < datetime) {
        printf("enddate %llu is less than datetime %llu\n", (unsigned long long)bb_command->enddate, (unsigned long long)datetime);
        return FBE_FALSE;
    }

    printf("%llu <= %llu <= %llu? TRUE!!!!\n", (unsigned long long)bb_command->startdate, (unsigned long long)datetime, (unsigned long long)bb_command->enddate );
    return FBE_TRUE;
}

/*!**************************************************************
 * fbe_disk_block_correct_string_to_date()
 ****************************************************************
 * @brief
 *  Convert a date string to a time_t structure
 *
 * @param 
 *  date - string of the date from the event log
 *
 * @return 
 *  time_t - the time structure 
 *
 * @author
 *  10/22/2012 - Created. Deanna Heng
 *
 ****************************************************************/
time_t fbe_disk_block_correct_string_to_date(fbe_char_t * date) {
    time_t datetime;
    fbe_u32_t month = 0;
    fbe_u32_t day = 0;
    fbe_u32_t year = 0;
    struct tm ltime;
    memset(&ltime, 0, sizeof(ltime));   /* get any nonstandard fields */
    
    printf("Date entered is %s\n", date);
    sscanf(date, "%2d/%2d/%4d", &month, &day, &year);
    printf("sscanf month: %d day: %d year %d\n", month, day, year);
    ltime.tm_year = year - 1900;
    ltime.tm_mon = month;
    ltime.tm_mday = day;
    ltime.tm_hour = 0;
    ltime.tm_min = 0;
    ltime.tm_sec = 0;
    ltime.tm_isdst = 0;

    datetime = mktime(&ltime);
    printf("Datetime is %llu\n", (unsigned long long)datetime);
    return datetime;
}

/*!**************************************************************
 * fbe_disk_block_correct_get_arraysn()
 ****************************************************************
 * @brief
 *  Get the SP serial number as a string
 *
 * @param 
 *  sn - pointer to serial number string
 *
 * @return 
 *  status - fbe status
 *
 * @author
 *  10/22/2012 - Created. Deanna Heng
 *
 ****************************************************************/
fbe_status_t fbe_disk_block_correct_get_arraysn(fbe_char_t * sn) {
    fbe_status_t status = FBE_STATUS_OK;
    fbe_esp_encl_mgmt_system_id_info_t      system_info = {0};

    //read and show system id info
    status = fbe_api_esp_encl_mgmt_get_system_id_info(&system_info);
    if(status != FBE_STATUS_OK)
    {
        printf("Error when getting system id info: %d\n", status);
        return status;
    }

    if (strcmp(system_info.serialNumber, "") == 0) {
        sprintf(sn, "sim");
    } else {
        sprintf(sn, system_info.serialNumber);
    }
    return status;
}

/*!**************************************************************
 * fbe_disk_block_correct_get_wwn_string()
 ****************************************************************
 * @brief
 *  Convert the fbe wwn to a string
 *
 * @param 
 *  world_wide_name - fbe structure containing the bytes for of the lun wwn
 *  wwn - reference to the wwn string
 *
 * @return 
 *  None.
 *
 * @author
 *  10/22/2012 - Created. Deanna Heng
 *
 ****************************************************************/
void fbe_disk_block_correct_get_wwn_string(fbe_assigned_wwid_t world_wide_name, fbe_char_t * wwn) 
{
    //char buff[sizeof(world_wide_name.bytes) * 3];
    fbe_u32_t i = 0;
    fbe_u32_t wwn_size = sizeof(world_wide_name.bytes);

    sprintf(wwn, "%02x",world_wide_name.bytes[i]);
    for(i = 1; i < wwn_size; i++)
    {
        sprintf(wwn, "%s:%02x",wwn,world_wide_name.bytes[i]);
    }
}

/*!**************************************************************
 * fbe_disk_block_correct_initialize()
 ****************************************************************
 * @brief
 *  Destroy the fbe api
 *
 * @param 
 *  bb_command - command structure
 *
 * @return 
 *  status - status of the initialization
 *
 * @author
 *  10/22/2012 - Created. Deanna Heng
 *
 ****************************************************************/
fbe_status_t fbe_disk_block_correct_initialize(fbe_bad_blocks_cmd_t * bb_command)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    if (bb_command->is_sim) {
        status = fbe_disk_block_correct_initialize_sim(bb_command);
    } 
    else 
    {
        status = fbe_disk_block_correct_initialize_user();
    }
    
    return status;
}

/*!**************************************************************
 * fbe_disk_block_correct_destroy_fbe_api()
 ****************************************************************
 * @brief
 *  Destroy the fbe api
 *
 * @param 
 *  bb_command - command structure
 *
 * @return 
 *  None.
 *
 * @author
 *  10/22/2012 - Created. Deanna Heng
 *
 ****************************************************************/
void __cdecl fbe_disk_block_correct_destroy_fbe_api(fbe_bad_blocks_cmd_t * bb_command)
{
    if (bb_command->is_sim) {
        fbe_disk_block_correct_destroy_fbe_api_sim(0);
    } 
    else 
    {
        fbe_disk_block_correct_destroy_fbe_api_user();
    }
  
    return;
}

/*!**************************************************************
 * fbe_get_package_id()
 ****************************************************************
 * @brief
 *  This is a required dummy functions for linking
 *
 * @param 
 *  package_id - reference to a package id
 *
 * @return 
 *  status - fbe status 
 *
 * @author
 *  10/22/2012 - Created. Deanna Heng
 *
 ****************************************************************/
fbe_status_t fbe_get_package_id(fbe_package_id_t * package_id)
{
    *package_id = FBE_PACKAGE_ID_SEP_0;
    return FBE_STATUS_OK;
}

/*!**************************************************************
 * fbe_disk_block_correct_open_lun_device()
 ****************************************************************
 * @brief
 *  close the lun device
 *
 * @param 
 *  lunDeviceHandle - handle to the lun device
 *
 * @return None.   
 *
 * @author
 *  10/22/2012 - Created. Deanna Heng
 *
 ****************************************************************/
fbe_status_t fbe_disk_block_correct_open_lun_device(fbe_char_t * lunDeviceName, 
                                                    EMCUTIL_RDEVICE_REFERENCE * lunDeviceHandle)
{
    EmcutilRemoteDeviceOpen(lunDeviceName,
                            NULL,
                            lunDeviceHandle);

    if (EMCUTIL_DREF_IS_INVALID(*lunDeviceHandle)) {
        printf("\n\tCan't connect to %s\n", lunDeviceName);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * fbe_disk_block_correct_close_lun_device()
 ****************************************************************
 * @brief
 *  close the lun device
 *
 * @param 
 *  lunDeviceHandle - handle to the lun device
 *
 * @return None.   
 *
 * @author
 *  10/22/2012 - Created. Deanna Heng
 *
 ****************************************************************/
void fbe_disk_block_correct_close_lun_device(EMCUTIL_RDEVICE_REFERENCE lunDeviceHandle)
{
    fbe_u32_t result;
	if (!EMCUTIL_DREF_IS_INVALID(lunDeviceHandle)) {
        result = EmcutilRemoteDeviceClose(lunDeviceHandle);
        if(result) {
            printf("\n\t  ** Close device failed. ");
        }
    }
}

/*!**************************************************************
 * fbe_disk_block_correct_close_file()
 ****************************************************************
 * @brief
 *  close the report file
 *
 * @param 
 *  fp - filehandle of the report file 
 *
 * @return 
 *  status - fbe status
 *
 * @author
 *  10/22/2012 - Created. Deanna Heng
 *
 ****************************************************************/
fbe_status_t fbe_disk_block_correct_close_file(FILE * fp)                                                                           
{                                                                                                                          
    fclose(fp);                                                                                                            
    return FBE_STATUS_OK;                                                                                                  
} 

/*!**************************************************************
 * fbe_disk_block_correct_get_write_file_handle()
 ****************************************************************
 * @brief
 *  Get the write file handle of the report file 
 *
 * @param 
 *  filename - filename to write to
 *  filehanlde - the handle to the file
 *
 * @return 
 *  status - fbe status
 *
 * @author
 *  10/22/2012 - Created. Deanna Heng
 *
 ****************************************************************/
fbe_status_t fbe_disk_block_correct_get_write_file_handle(fbe_char_t * filename, FILE ** filehandle)
{
    if((*filehandle = fopen(filename, "w")) == NULL)                                                                                 
    {                                                                                                                      
        return FBE_STATUS_GENERIC_FAILURE;                                                                                                       
    }
    return FBE_STATUS_OK;
}

/*!**************************************************************
 * fbe_disk_block_correct_get_read_file_handle()
 ****************************************************************
 * @brief
 *  Get the read file handle of the report file 
 *
 * @param 
 *  filename - filename to read from
 *  filehanlde - the handle to the file
 *
 * @return 
 *  status - fbe status
 *
 * @author
 *  10/22/2012 - Created. Deanna Heng
 *
 ****************************************************************/
fbe_status_t fbe_disk_block_correct_get_read_file_handle(fbe_char_t * filename, FILE ** filehandle)
{
    if((*filehandle = fopen(filename, "r")) == NULL)                                                                                 
    {                                                                                                                      
        return FBE_STATUS_GENERIC_FAILURE;                                                                                                       
    }
    return FBE_STATUS_OK;
}


