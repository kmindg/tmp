/***************************************************************************
* Copyright (C) EMC Corporation 2010-2011
* All rights reserved.
* Licensed material -- property of EMC Corporation
***************************************************************************/

/*!**************************************************************************
* @file fbe_database_fru_descriptor_process.c
***************************************************************************
*
* @brief
*  This file contains the implementaion of processing fru descriptors in homewrecker logic
*
* @version
*    11/30/2011 - Created. zphu
*    12/5/2011 - Modified. zphu
*    4/17/2012 - Modified. hew2
***************************************************************************/

/*************************
*   INCLUDE FILES
*************************/
#include "fbe_database_private.h"
#include "fbe_fru_descriptor_structure.h"
#include "fbe_private_space_layout.h"
#include "fbe_database_homewrecker_fru_descriptor.h"
#include "fbe_database_config_tables.h"
#include "fbe_database_homewrecker_db_interface.h"

/* The global fru descriptor seq number would be initialized to 233391 when ICA
  * Increase it before every FRU Descriptor update.
  * On system booting path, we also need to initialize it. Just after we initialize the
  * In memory global varible for FRU_Descriptor.
  */
fbe_u32_t global_fru_descriptor_seq_number;

/*!**************************************************************************************************
@fn fbe_database_system_disk_fru_descriptor_inmemory_init()
*****************************************************************************************************

* @brief
*  Generate FRU descriptor according to current system. 
*  
* @param database_service_ptr- point to database service
* @param out_fru_descriptor_mem- output pointer for fru descriptor, when it's not passing NULL
* 
* @return
*  fbe_status_t
*
*****************************************************************************************************/

fbe_status_t fbe_database_system_disk_fru_descriptor_inmemory_init(fbe_homewrecker_fru_descriptor_t *out_fru_descriptor_mem)
{
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_status_t    pdo_info_status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u8_t        serial_num[FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE+1] = {0};
    fbe_u32_t       chassis_wwn_seed = 0;
    fbe_u32_t       slot = 0;
    fbe_object_id_t pdo_object_id = FBE_OBJECT_ID_INVALID;

    if(out_fru_descriptor_mem == NULL)
    {
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: illegal parameter(NULL pointer)\n",
                        __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_database_get_board_resume_prom_wwnseed_with_retry(&chassis_wwn_seed);
    if (FBE_STATUS_OK != status)
    {
        database_trace (FBE_TRACE_LEVEL_ERROR,
                    FBE_TRACE_MESSAGE_ID_INFO,
                    "%s: Failed to read PROM wwn_seed\n",
                    __FUNCTION__);

        return status;
    }

    /*Get wwn seed from midplane prom*/
    database_trace(FBE_TRACE_LEVEL_INFO, 
                   FBE_TRACE_MESSAGE_ID_INFO,
                   "Homewrecker: Get chassis wwn seed 0x%x\n", 
                   chassis_wwn_seed);
    
    fbe_copy_memory(out_fru_descriptor_mem->magic_string,
                    FBE_FRU_DESCRIPTOR_MAGIC_STRING, sizeof(FBE_FRU_DESCRIPTOR_MAGIC_STRING));
    out_fru_descriptor_mem->wwn_seed = chassis_wwn_seed;
    out_fru_descriptor_mem->structure_version = FBE_FRU_DESCRIPTOR_STRUCTURE_VERSION;   /*For extend*/
    out_fru_descriptor_mem->chassis_replacement_movement = FBE_CHASSIS_REPLACEMENT_FLAG_INVALID;
    
    /* added sequence number into FRU Descriptor @2013 Feb 26th */
    /* initialize in-memory fru_descriptor seq number as invalid one */
    out_fru_descriptor_mem->sequence_number = FBE_FRU_DESCRIPTOR_INVALID_SEQ_NUMBER;

    for (slot = 0; slot < FBE_FRU_DESCRIPTOR_SYSTEM_DRIVE_NUMBER; slot++) 
    {
        /*Get LDO object id*/
        pdo_info_status = fbe_database_get_pdo_object_id_by_location(0, 0, slot, &pdo_object_id);
        /*because system can tolerate disk lost case, doesn't care the status*/
        if(pdo_info_status != FBE_STATUS_OK)
        {
            database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                             "%s: Failed to get object id\n",
                             __FUNCTION__); 
            continue;                             
        }else
        {
            database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                             "Homewrecker: Drive %u_%u_%u corresponding object is 0x%x\n",
                             0, 0, slot, pdo_object_id); 
        }

        /*Get serial num from PDO*/
        pdo_info_status = fbe_database_get_serial_number_from_pdo(pdo_object_id, serial_num);
        if(pdo_info_status != FBE_STATUS_OK)
        {
            database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                             "%s: Failed to get serial number from PDO\n",
                             __FUNCTION__); 
            continue;
        }else
        {
            database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                             "Homewrecker: Got PDO 0x%x serial number: %s\n",
                             pdo_object_id, serial_num); 
        }

        fbe_copy_memory(&out_fru_descriptor_mem->system_drive_serial_number[slot],
                        serial_num, sizeof(serial_num));
    }

    /* output the in-memory FRU Descriptor into trace*/
    database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                    "Homewrecker: in-memory FRU_Descriptor:\n");
    fbe_database_display_fru_descriptor(out_fru_descriptor_mem);

    return FBE_STATUS_OK;
}

/*!**************************************************************************************************
@fn fbe_database_system_disk_fru_descriptor_init()
*****************************************************************************************************

* @brief
*  Homewreck logic when disk invalidate set. 
*  
* @param database_service_ptr- point to database service
* 
* @return
*  fbe_status_t
*
*****************************************************************************************************/

fbe_status_t fbe_database_system_disk_fru_descriptor_init(fbe_database_service_t *database_service_ptr)
{
    fbe_status_t    status = FBE_STATUS_INVALID;
    fbe_homewrecker_fru_descriptor_t    mem_fru_descriptor;

    database_initialize_fru_descriptor_sequence_number();
    
    database_trace(FBE_TRACE_LEVEL_INFO, 
                   FBE_TRACE_MESSAGE_ID_INFO, 
                   "%s: Homewrecker: Populate initial FRU descriptor for disks\n", __FUNCTION__);

    /*Generate fru descriptor according to current system*/
    status = fbe_database_system_disk_fru_descriptor_inmemory_init(&mem_fru_descriptor); 
    if (status != FBE_STATUS_OK) 
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s: Populate initial fru descriptor failed\n", __FUNCTION__);
        return status;
    }

    
    database_trace(FBE_TRACE_LEVEL_INFO, 
                   FBE_TRACE_MESSAGE_ID_INFO, 
                   "%s: Homewrecker: Persist FRU descriptor\n", __FUNCTION__);
                   
    status = fbe_database_set_homewrecker_fru_descriptor(database_service_ptr, &mem_fru_descriptor);
    if (status != FBE_STATUS_OK) 
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s: Persist fru descriptor failed\n", __FUNCTION__);
        return status;
    }

    return FBE_STATUS_OK;
}


/*!**************************************************************************************************
@fn database_homewrecker_raw_mirror_error()
*****************************************************************************************************

* @brief
*  Encapsulate the raw mirror return error, return simply error or ok. 
*  
* @param raw_mirror_error_report- input pointer to raw mirror error report
* 
* @return
*  fbe_homewrecker_get_raw_mirror_status_t- status for Homewrecker
*
*****************************************************************************************************/

fbe_homewrecker_get_raw_mirror_status_t
database_homewrecker_raw_mirror_error(fbe_raid_verify_raw_mirror_errors_t* raw_mirror_error_report)
{
    if (raw_mirror_error_report->raw_mirror_seq_bitmap | raw_mirror_error_report->raw_mirror_magic_bitmap)
        return FBE_GET_HW_STATUS_ERROR;
    else if (raw_mirror_error_report->verify_errors_counts.c_coh_count |
                  raw_mirror_error_report->verify_errors_counts.c_crc_count |
                  raw_mirror_error_report->verify_errors_counts.c_crc_multi_count |
                  raw_mirror_error_report->verify_errors_counts.c_crc_single_count |
                  raw_mirror_error_report->verify_errors_counts.c_media_count |
                  raw_mirror_error_report->verify_errors_counts.c_rm_magic_count |
                  raw_mirror_error_report->verify_errors_counts.c_rm_seq_count |
                  raw_mirror_error_report->verify_errors_counts.c_soft_media_count |
                  raw_mirror_error_report->verify_errors_counts.c_ss_count |
                  raw_mirror_error_report->verify_errors_counts.c_ts_count |
                  raw_mirror_error_report->verify_errors_counts.c_ws_count |
                  raw_mirror_error_report->verify_errors_counts.non_retryable_errors |
                  raw_mirror_error_report->verify_errors_counts.retryable_errors |
                  raw_mirror_error_report->verify_errors_counts.shutdown_errors |
                 raw_mirror_error_report->verify_errors_counts.u_coh_count |
                  raw_mirror_error_report->verify_errors_counts.u_crc_count |
                  raw_mirror_error_report->verify_errors_counts.u_crc_multi_count |
                  raw_mirror_error_report->verify_errors_counts.u_crc_single_count |
                  raw_mirror_error_report->verify_errors_counts.u_media_count |
                  raw_mirror_error_report->verify_errors_counts.u_ss_count |
                  raw_mirror_error_report->verify_errors_counts.u_ts_count |
                  raw_mirror_error_report->verify_errors_counts.u_ws_count)
        return FBE_GET_HW_STATUS_ERROR;
    else
        return FBE_GET_HW_STATUS_OK;

    
}


/*!**************************************************************************************************
@fn fbe_database_set_homewrecker_fru_descriptor()
*****************************************************************************************************

* @brief
*  Encapsulate raw mirror fru descriptor write for Homewrecker. 
*  
* @param in_fru_descriptor- input pointer to fru descriptor
* 
* @return
*  fbe_status_t
*
*****************************************************************************************************/

fbe_status_t 
fbe_database_set_homewrecker_fru_descriptor(fbe_database_service_t* db_service, fbe_homewrecker_fru_descriptor_t* in_fru_descriptor)
{
    /**************DO NOT FORGET TO INIT THE GLOBAL IN-MEMORY FRU DESCRIPTOR HERE**************/
    fbe_status_t    status = FBE_STATUS_INVALID;
    fbe_u64_t       done_count = 0;
    fbe_u64_t       done_count_expected = sizeof(fbe_homewrecker_fru_descriptor_t);
    database_homewrecker_db_raw_mirror_operation_status_t raw_mirror_err = {0};
    fbe_bool_t      is_clear = FBE_FALSE;
    fbe_homewrecker_get_raw_mirror_status_t err_report = FBE_GET_HW_STATUS_INVALID;
    fbe_u32_t fru_descriptor_seq_num = FBE_FRU_DESCRIPTOR_INVALID_SEQ_NUMBER;


    database_trace (FBE_TRACE_LEVEL_INFO,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s entry\n.", __FUNCTION__);
    
    if (in_fru_descriptor == NULL || db_service == NULL)
    {
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: Parameter error\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = database_get_next_fru_descriptor_sequence_number(&fru_descriptor_seq_num);
    if (status != FBE_STATUS_OK)
    {
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "Getting fru descriptor seq num failed\n");
        return FBE_STATUS_GENERIC_FAILURE;
    }
    in_fru_descriptor->sequence_number = fru_descriptor_seq_num;

    status = database_homewrecker_db_interface_general_write((fbe_u8_t*)in_fru_descriptor, 0, 
                                                             done_count_expected,
                                                             &done_count, is_clear, &raw_mirror_err);
    if (status != FBE_STATUS_OK || done_count != done_count_expected
        || raw_mirror_err.block_operation_successful != FBE_TRUE)   
    {
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "Writing fru descriptor to raw mirror failed\n");
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*Update global in memory descriptor*/
    fbe_spinlock_lock(&db_service->pvd_operation.system_fru_descriptor_lock);
    fbe_copy_memory(&db_service->pvd_operation.system_fru_descriptor, in_fru_descriptor, sizeof(*in_fru_descriptor));
    fbe_spinlock_unlock(&db_service->pvd_operation.system_fru_descriptor_lock);
    
    err_report = database_homewrecker_raw_mirror_error(&raw_mirror_err.error_verify_report);
    if (err_report != FBE_GET_HW_STATUS_OK)
    {
        database_trace (FBE_TRACE_LEVEL_WARNING,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "Raw mirror write for fru descriptor failed, warning existed\n");
        return FBE_STATUS_OK;
    }

    return FBE_STATUS_OK;
}


/*!**************************************************************************************************
@fn database_homewrecker_single_disk_mask_to_offset()
*****************************************************************************************************

* @brief
*  translate disk mask to offset. 
*  
* @param disk_mask- disk mask for raw mirror operation
* 
* @return
*  fbe_u16_t- offset
*
*****************************************************************************************************/

static fbe_u16_t database_homewrecker_single_disk_mask_to_offset(fbe_homewrecker_get_fru_disk_mask_t disk_mask)
{
    switch (disk_mask)
    {
        case FBE_FRU_DISK_2:
            return 0x2;
            
        case FBE_FRU_DISK_1:
            return 0x1;
            
        case FBE_FRU_DISK_0:
            return 0x0;
    }
    return 0xFF; /*unexpected*/
}

/*!**************************************************************************************************
@fn fbe_database_get_homewrecker_fru_descriptor()
*****************************************************************************************************

* @brief
*  Encapsulate raw mirror fru descriptor read for Homewrecker. 
*  
* @param out_fru_descriptor- output fru descriptor for read
* @param report- raw mirror access error report
* @param disk_mask- disk mask for raw mirror operation
* 
* @return
*  fbe_status_t
*
*****************************************************************************************************/

fbe_status_t fbe_database_get_homewrecker_fru_descriptor(fbe_homewrecker_fru_descriptor_t* out_fru_descriptor,
                                                                            fbe_homewrecker_get_raw_mirror_info_t* report,
                                                                            fbe_homewrecker_get_fru_disk_mask_t disk_mask)
{    
    fbe_status_t    status = FBE_STATUS_INVALID;
    fbe_u64_t       done_count = 0;
    fbe_u64_t       done_count_expected = DATABASE_HOMEWRECKER_DB_RAW_BLOCK_DATA_SIZE;
    database_homewrecker_db_raw_mirror_operation_status_t raw_mirror_err = {0};
    fbe_u8_t        block_buffer[DATABASE_HOMEWRECKER_DB_RAW_BLOCK_DATA_SIZE] = {0};
    fbe_bool_t      path_enable = FBE_TRUE;
    fbe_bool_t      path_status = FBE_TRUE;
    fbe_u32_t       disk_index = 0;
    fbe_u16_t       disk_offset = 0;
    
    if (out_fru_descriptor == NULL || report == NULL)
    {
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s Parameter illegal\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    report->status = FBE_GET_HW_STATUS_OK;

    /*edge may not be  valid, check it first*/
    if (disk_mask == FBE_FRU_DISK_ALL) 
    {
        for (disk_index = 0, path_enable = FBE_FALSE;                   \
            disk_index < FBE_FRU_DESCRIPTOR_DB_DRIVE_NUMBER; disk_index++)
        {
            status = database_homewrecker_get_system_pvd_edge_state(disk_index, &path_status);
            if (status != FBE_STATUS_OK)
            {
                database_trace (FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "Get edge state failed\n");
                path_status = FBE_FALSE;
            }

            if (path_status)
            {   
                /*has at least one edge valid*/
                path_enable = FBE_TRUE;
            }
            else/*has at least one edge invalid*/
                report->status = FBE_GET_HW_STATUS_ERROR;
        }
    }
    else if (disk_mask == FBE_FRU_DISK_0 ||
             disk_mask == FBE_FRU_DISK_1 ||
             disk_mask == FBE_FRU_DISK_2)
    {   /*read one disk*/
        disk_offset = database_homewrecker_single_disk_mask_to_offset(disk_mask);
        if (disk_offset == 0xFF)
        {
            database_trace (FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s: Wrong use of disk_mask\n", __FUNCTION__);
            path_status = FBE_FALSE;
        }
        status = database_homewrecker_get_system_pvd_edge_state((fbe_u32_t)disk_offset, &path_enable);
        if (status != FBE_STATUS_OK)
        {
            database_trace (FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "Get edge state failed\n");
            path_enable = FBE_FALSE;
        }
        
    }else
    {   /*read 2 disks at once*/
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "Doesn't support dual disk read now\n");
        path_enable = FBE_FALSE;
    }
    

    if (path_enable == FBE_FALSE)
    {
        database_trace (FBE_TRACE_LEVEL_INFO,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s No edge can be used for read\n", __FUNCTION__);
        report->status = FBE_GET_HW_STATUS_ERROR;
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    status = database_homewrecker_db_interface_general_read(block_buffer, 0, done_count_expected,
                                                            &done_count, disk_mask, &raw_mirror_err);
    
    if (status != FBE_STATUS_OK || done_count != done_count_expected)
    {
        database_trace (FBE_TRACE_LEVEL_WARNING,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "Read raw mirror fru descriptor failed, status = %d, done_count = %lld, drive mask: 0x%x\n",
                        status, (unsigned long long)done_count, disk_mask);
        report->status = FBE_GET_HW_STATUS_ERROR;
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_copy_memory(out_fru_descriptor, block_buffer, sizeof(*out_fru_descriptor));
    
    if (report->status == FBE_GET_HW_STATUS_OK) 
        report->status = database_homewrecker_raw_mirror_error(&raw_mirror_err.error_verify_report);
        
    /*compare homewrecker magic string*/
    if (fbe_compare_string(out_fru_descriptor->magic_string, FBE_FRU_DESCRIPTOR_MAGIC_STRING_LENGTH,
                            FBE_FRU_DESCRIPTOR_MAGIC_STRING, FBE_FRU_DESCRIPTOR_MAGIC_STRING_LENGTH, FBE_TRUE))
    {
        report->status = FBE_GET_HW_STATUS_ERROR;
    }
    status = fbe_homewrecker_db_util_get_raw_mirror_block_seq(block_buffer, &report->seq_number);
    if (status != FBE_STATUS_OK)
    {
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "Read raw mirror fru descriptor sequence number failed\n");
        report->status = FBE_GET_HW_STATUS_ERROR;
        return FBE_STATUS_GENERIC_FAILURE;
    }

    report->error_disk_bitmap = raw_mirror_err.error_disk_bitmap;
    
    return FBE_STATUS_OK;
}

void fbe_database_display_fru_descriptor(fbe_homewrecker_fru_descriptor_t* fru_descriptor)
{
    if(NULL == fru_descriptor)
    {
        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s Homewrecker: fru_descriptor is NULL.\n", __FUNCTION__);
        return;
    }

    /*output the FRU Descriptor into trace */
    database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                    "Homewrecker: FRU_Descriptor:\n");
    database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                    "Magic string: %s\n", fru_descriptor->magic_string);
    database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                    "Chassis wwn seed: 0x%x\n", fru_descriptor->wwn_seed);
    database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                    "Sys drive 0 SN: %s\n", fru_descriptor->system_drive_serial_number[0]);
    database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                    "Sys drive 1 SN: %s\n", fru_descriptor->system_drive_serial_number[1]);
    database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                    "Sys drive 2 SN: %s\n", fru_descriptor->system_drive_serial_number[2]);
    database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                    "Sys drive 3 SN: %s\n", fru_descriptor->system_drive_serial_number[3]);
    database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                    "Chassis replacement flag: 0x%x\n", fru_descriptor->chassis_replacement_movement);
    database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                    "Sequence number: %d\n", fru_descriptor->sequence_number);
    database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                    "Structure version: 0x%x\n", fru_descriptor->structure_version);

}

void fbe_database_display_fru_signature(fbe_fru_signature_t* fru_signature)
{
    if(NULL == fru_signature)
    {
        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s Homewrecker: fru_signature is NULL.\n", __FUNCTION__);
        return;
    }

    database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                    "Magic string: %s\n",
                                    fru_signature->magic_string);
    database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                    "Version: %d\n",
                                    fru_signature->version);
    database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                    "System_wwn_seed: 0x%x\n",
                                    (unsigned int)fru_signature->system_wwn_seed);
    database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                    "Bus: %d\n",
                                    fru_signature->bus);
    database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                    "Enclosure: %d\n",
                                    fru_signature->enclosure);
    database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                    "Slot: %d\n",
                                    fru_signature->slot);

    return;
}

/*!**************************************************************************************************
@fn fbe_database_initialize_fru_descriptor_sequence_number()
*****************************************************************************************************

* @brief
*  Initialize fru descriptor sequence number, this function should be only called in invalidate disk process
*  
* 
* @return NA
*  
*
*****************************************************************************************************/

void database_initialize_fru_descriptor_sequence_number(void)
{
    /* We are in ICA procressing, so initialize FRU_Descriptor sequnce number as default */
     global_fru_descriptor_seq_number = FBE_FRU_DESCRIPTOR_DEFAULT_SEQ_NUMBER;    
    database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                    "Initialize global fru descriptor seq number: %d\n",
                                    global_fru_descriptor_seq_number);  
}

/*!**************************************************************************************************
@fn fbe_database_set_fru_descriptor_sequence_number()
*****************************************************************************************************

* @brief
*  Set fru descriptor sequence number to a designate value, 
*  this function should be called on DB booting path, when Homewrecker determine the standard FRU_Descriptor
*  then set global_fru_descriptor_seq_number to standard FRU_Descriptor.seq_num.
*  
* 
* @return fbe_status_t
*  
*
*****************************************************************************************************/
fbe_status_t database_set_fru_descriptor_sequence_number(fbe_u32_t seq_num)
{
    if(seq_num == FBE_FRU_DESCRIPTOR_INVALID_SEQ_NUMBER)
    {
        database_trace(FBE_TRACE_LEVEL_WARNING, 
                   FBE_TRACE_MESSAGE_ID_INFO, 
                   "set_fru_des_seq_num: set global fru descriptor seq num to INVALID.\n");
        global_fru_descriptor_seq_number = seq_num;
        return FBE_STATUS_GENERIC_FAILURE;
    }
    else
    {
        database_trace(FBE_TRACE_LEVEL_WARNING, 
                   FBE_TRACE_MESSAGE_ID_INFO, 
                   "set_fru_des_seq_num: set global fru descriptor seq num to %d.\n", seq_num);
        global_fru_descriptor_seq_number = seq_num;
        return FBE_STATUS_OK;
    }
}

/*!**************************************************************************************************
@fn fbe_database_get_next_fru_descriptor_sequence_number()
*****************************************************************************************************

* @brief
*  Get next fru descriptor sequence number, this function should be only called in set fru descriptor function
* if the global_fru_descriptor_seq_number is invalid, log warning, return invalid sequence number to caller.
*  
* 
* @return fbe_status_t
*  
*
*****************************************************************************************************/
fbe_status_t database_get_next_fru_descriptor_sequence_number(fbe_u32_t* seq_num)
{
    /* judge whether global fru descriptor is initialized or not */
    if(global_fru_descriptor_seq_number == FBE_FRU_DESCRIPTOR_INVALID_SEQ_NUMBER)
    {
        database_trace(FBE_TRACE_LEVEL_WARNING, 
                   FBE_TRACE_MESSAGE_ID_INFO, 
                   "get_next_fru_des_seq_num: seq num of FRU_Descriptor is not initialized.\n");
        *seq_num = FBE_FRU_DESCRIPTOR_INVALID_SEQ_NUMBER;
        return FBE_STATUS_GENERIC_FAILURE;
    }
    else {
        global_fru_descriptor_seq_number++;
        /*seq number reach the maxim value, so reset it to default value */
        if(global_fru_descriptor_seq_number == FBE_FRU_DESCRIPTOR_INVALID_SEQ_NUMBER)
        {
            global_fru_descriptor_seq_number = FBE_FRU_DESCRIPTOR_DEFAULT_SEQ_NUMBER;
            *seq_num = global_fru_descriptor_seq_number;
            database_trace(FBE_TRACE_LEVEL_INFO, 
                   FBE_TRACE_MESSAGE_ID_INFO, 
                   "get_next_fru_des_seq_num: get next seq num successfully, seq_num:%d. seq_num gets reset.\n", 
                   *seq_num);
            return FBE_STATUS_OK;
        }
        else {
            *seq_num = global_fru_descriptor_seq_number;
            database_trace(FBE_TRACE_LEVEL_INFO, 
                   FBE_TRACE_MESSAGE_ID_INFO, 
                   "get_next_fru_des_seq_num: get next seq num successfully, seq_num:%d.\n", 
                   *seq_num);            
            return FBE_STATUS_OK;
        }
    }
}

/*!**************************************************************************************************
@fn database_homewrecker_determine_standard_fru_descriptor()
*****************************************************************************************************

* @brief
* Homewrecker determine the standard fru descriptor by itself, not trust in raw_mirror.
* 3 copies of fru descriptors(on-disks) are in system_disk_table, 
* firstly, check the consitence of system_disk_talbe[n].dp.wwn_seed with PROM wwn_seed
* bypass the inconsitent on disk fru descriptor
* secondly, if there more than one copies fru_dp.wwn_seed are consistent with PROM wwn_seed
* we choose the copy with biggest fru descriptor sequence number as standard fru descriptor
* then if there is no consitent copy, we check whether on disk copies are consistent with each other
* if yes, it may be chassis replacement case, leave it to be handled by following Homewrecker logic
* if not, enter service mode with SYSTEM_INTEGRITY_BROKEN
* 
* @return fbe_status_t
* Created by Jian Gao @ 2013 Feb 27th
*****************************************************************************************************/
fbe_status_t fbe_database_homewrecker_determine_standard_fru_descriptor(
                          homewrecker_system_disk_table_t* system_disk_table,
                          homewrecker_system_disk_table_t* standard_disk)
{
    fbe_status_t status = FBE_STATUS_INVALID;
    fbe_u32_t wwn_seed_in_PROM = 0;
    fbe_bool_t all_on_disk_fru_descriptors_consistent = FBE_FALSE;
    fbe_bool_t is_on_disk_fru_descriptor_valid[3];
    /* set first valid copy to a invalid value  */ 
    fbe_u32_t first_valid_copy_index = 23;
    fbe_u32_t index;
    
    if(NULL == system_disk_table || NULL == standard_disk)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
               FBE_TRACE_MESSAGE_ID_INFO, 
               "%s: Invalid parameter, NULL pointer.\n", __FUNCTION__);                
        status = FBE_STATUS_GENERIC_FAILURE;
        return status;
    }
    
    /* Check 3 copies of on disk fru descriptor is valid or not, via magic string */
    for (index = 0; index < FBE_FRU_DESCRIPTOR_DB_DRIVE_NUMBER; index++)
    {
        if (!fbe_compare_string(system_disk_table[index].dp.magic_string, 
                                                         FBE_FRU_DESCRIPTOR_MAGIC_STRING_LENGTH,
                                                         FBE_FRU_DESCRIPTOR_MAGIC_STRING,
                                                         FBE_FRU_DESCRIPTOR_MAGIC_STRING_LENGTH,
                                                         FBE_TRUE))
        {
            is_on_disk_fru_descriptor_valid[index] = FBE_TRUE;
            database_trace (FBE_TRACE_LEVEL_INFO,
                                             FBE_TRACE_MESSAGE_ID_INFO,
                                             "%s: fru descriptor on disk 0_0_%d has VALID magic string.\n",
                                             __FUNCTION__, index);
        }
        else
        {
            is_on_disk_fru_descriptor_valid[index] = FBE_FALSE;
            database_trace (FBE_TRACE_LEVEL_INFO,
                                             FBE_TRACE_MESSAGE_ID_INFO,
                                             "%s: fru descriptor on disk 0_0_%d has INVALID magic string.\n",
                                             __FUNCTION__, index);
        }
    }

    /* get PROM resume wwn_seed */
    status = fbe_database_get_board_resume_prom_wwnseed_with_retry(&wwn_seed_in_PROM);
    if (FBE_STATUS_OK != status)
    {
        database_trace (FBE_TRACE_LEVEL_ERROR,
                    FBE_TRACE_MESSAGE_ID_INFO,
                    "%s: Failed to read PROM wwn_seed\n",
                    __FUNCTION__);

        return status;
    }

    for (index = 0; index < FBE_FRU_DESCRIPTOR_DB_DRIVE_NUMBER; index++)
    {
        if (is_on_disk_fru_descriptor_valid[index] &&
             wwn_seed_in_PROM == system_disk_table[index].dp.wwn_seed)
        {
            if (system_disk_table[index].dp.sequence_number > standard_disk->dp.sequence_number)
            {
                fbe_copy_memory(&(standard_disk->dp), 
                                                       &(system_disk_table[index].dp), 
                                                       sizeof(standard_disk->dp));
                database_trace (FBE_TRACE_LEVEL_INFO,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: temp standard fru descriptor is from disk:%d\n",
                        __FUNCTION__, index);
            }
            else
                continue;
        }
        else
            continue;
    }

    /* after above loop done, we should check whether standard_disk->dp is set
      * we do this via dp.magic_string to validate dp is set or not.
      */
    if (!fbe_compare_string(standard_disk->dp.magic_string, 
                                                     FBE_FRU_DESCRIPTOR_MAGIC_STRING_LENGTH,
                                                     FBE_FRU_DESCRIPTOR_MAGIC_STRING,
                                                     FBE_FRU_DESCRIPTOR_MAGIC_STRING_LENGTH,
                                                     FBE_TRUE))
    {
        database_trace (FBE_TRACE_LEVEL_INFO,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: we get the standard fru descriptor:\n",
                        __FUNCTION__);
        fbe_database_display_fru_descriptor(&(standard_disk->dp));
        return FBE_STATUS_OK;
    }
    else
    {
        database_trace (FBE_TRACE_LEVEL_WARNING,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: all copies of on disk fru_descriptor.wwn_seed inconsitent with PROM.\n",
                        __FUNCTION__);
        database_trace (FBE_TRACE_LEVEL_WARNING,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: More procressing needed, maybe it is chassis replacement case\n",
                        __FUNCTION__);

        /* All copies of on-disk fru descriptor's wwn_seed are inconsitent with PROM wwn_seed.
          * Then we check whether all copies of on-disk fru descriptors are consitent with each other.
          * Do this check based on wwn_seed check.
          * If all copies of on-disk fru descriptor are consitent with each other, we choose the copy with
          * biggest sequence number. Array may be in chassis replacement cases, we do not handle these
          * case here, only determine the standard fru descriptor.
          *
          * If not all copies are consistent with each other, we should let array enter service mode.
          * Because, meantime, there is not any copy fru descriptor consistent with chassis PROM resume
          */

        /* check all_on_disk_fru_descriptors_consistent or not */
        /* set it to FALSE as initial value */
        /* one notes, we only check valid fru descriptor */
        all_on_disk_fru_descriptors_consistent = FBE_FALSE;
        
        for (index = 0; index < FBE_FRU_DESCRIPTOR_DB_DRIVE_NUMBER; index ++)
        {
            if (first_valid_copy_index >= FBE_FRU_DESCRIPTOR_DB_DRIVE_NUMBER)
            {
                /* Means we did not find any valid copy of fru_descriptor 
                  * we need to find one, then use this one as baseline to do the check
                  */
                if (is_on_disk_fru_descriptor_valid[index])
                {
                    /* if we found one valid copy, we needs to 
                      * set all_on_disk_fru_descriptors_consistent to true temporarily
                      * then, in following logic, if we found any copy inconsistent with 
                      * this first valid copy we would set it to false, and break the loop.
                      */
                    all_on_disk_fru_descriptors_consistent = FBE_TRUE;
                    first_valid_copy_index = index;
                    continue;
                }
                else
                    continue;
            }
            else
            {
                /* we already have the first valid copy, use that copy to check others */
                if (system_disk_table[first_valid_copy_index].dp.wwn_seed !=
                     system_disk_table[index].dp.wwn_seed)
                {
                    /* we found inconsistent one */
                    all_on_disk_fru_descriptors_consistent = FBE_FALSE;
                    break;
                }
                else
                {
                    /* current copy is consistent with first valid copy, continue loop */
                    continue;
                }
            }
        }

        /* if all valid copies' wwn_seed are consistent with each other, 
          * we choose the copy with biggest sequence number as standard
          */
        if (all_on_disk_fru_descriptors_consistent == FBE_TRUE)
        {
            for (index = 0; index <  FBE_FRU_DESCRIPTOR_DB_DRIVE_NUMBER; index++)
            {
                if (is_on_disk_fru_descriptor_valid[index])
                {
                    if (system_disk_table[index].dp.sequence_number > standard_disk->dp.sequence_number)
                    {
                        fbe_copy_memory(&(standard_disk->dp), 
                                                               &(system_disk_table[index].dp), 
                                                               sizeof(standard_disk->dp));
                        database_trace (FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: temp standard fru descriptor is from disk:%d\n",
                                __FUNCTION__, index);
                    }
                    else
                        /* current copy's sequence number is less or equal to temp standard one */
                        continue;
                }
                else
                    /* current copy is invalid fru descriptor */
                    continue;                    
            }
            
            /* after above loop done, we should check whether standard_disk->dp is set
              * we do this via dp.magic_string to validate dp is set or not.
              */
            if (!fbe_compare_string(standard_disk->dp.magic_string, 
                                                             FBE_FRU_DESCRIPTOR_MAGIC_STRING_LENGTH,
                                                             FBE_FRU_DESCRIPTOR_MAGIC_STRING,
                                                             FBE_FRU_DESCRIPTOR_MAGIC_STRING_LENGTH,
                                                             FBE_TRUE))
            {
                database_trace (FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: we get the standard fru descriptor:\n",
                                __FUNCTION__);
                fbe_database_display_fru_descriptor(&(standard_disk->dp));
                return FBE_STATUS_OK;
            }
            else
            {
                /* this should not be reached... */
                database_trace (FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: Can not get standard fru descriptor when we should be able to...\n",
                                __FUNCTION__);
            }
        }
        else {
            /* not all copies of fru_descriptor are consistent with each other,
              * meantime, no one copy is consistent with PROM wwn_seed.
              * Let DB enter service mode, then return service mode status.
              */
            database_trace (FBE_TRACE_LEVEL_WARNING, 
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "Homewrecker: DB would enter service mode with SYS_DRIVES_INTEGRALITY_BROKEN.\n");

            fbe_database_enter_service_mode(FBE_DATABASE_SERVICE_MODE_REASON_HOMEWRECKER_INTEGRITY_BROKEN);
            return FBE_STATUS_SERVICE_MODE;         
        }
    }
    
    return FBE_STATUS_OK;
}
