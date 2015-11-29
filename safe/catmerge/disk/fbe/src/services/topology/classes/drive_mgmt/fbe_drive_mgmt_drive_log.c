/***************************************************************************
 * Copyright (C) EMC Corporation 2010 
 * All rights reserved. Licensed material -- property of EMC 
 * Corporation 
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_drive_mgmt_drive_log.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the Drive Management Get Smart Dump (0xE6) feature methods.
 * 
 * @ingroup drive_mgmt_class_files
 * 
 * @version
 *   03/25/2011:  Created. chenl6
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_memory.h"
#include "fbe_topology.h"
#include "fbe_base_object.h"
#include "fbe_scheduler.h"
#include "fbe/fbe_api_common.h"
#include "fbe_transport_memory.h"
#include "fbe/fbe_esp_drive_mgmt.h"
#include "fbe/fbe_api_esp_drive_mgmt_interface.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_physical_drive.h"
#include "fbe/fbe_event_log_api.h"
#include "EspMessages.h"

#include "fbe_drive_mgmt_private.h"


/*************************************
 * LOCAL FUNCTIONS
 ************************************/
static fbe_status_t fbe_drive_mgmt_get_eligible_drive_for_log(fbe_drive_mgmt_t *drive_mgmt);
static void fbe_drive_mgmt_get_drive_log_asynch_callback(fbe_packet_completion_context_t context);

/*************************************
 * LOCAL DEFINES
 ************************************/
#define DMO_DRIVE_GETLOG_BUFFER_ALLOC_LENGTH  107520


/**************************************************************************
 *      fbe_drive_mgmt_handle_drivegetlog_request()
 **************************************************************************
 *
 *  @brief
 *    This function initializes get Smart Dump (0xE6) feature.
 *
 *  @param
 *    drive_mgmt - pointer to drive mgmt structure.
 * 
 *  @return
 *     fbe_status_t - fbe status.
 *
 *  @author
 *   03/25/2011:  Created. chenl6
 *
 **************************************************************************/
fbe_status_t 
fbe_drive_mgmt_handle_drivegetlog_request(fbe_drive_mgmt_t *drive_mgmt, 
                                          fbe_device_physical_location_t *location)
{
    fbe_status_t                      status = FBE_STATUS_OK;
    fbe_atomic_t                      old_value = FBE_FALSE;
    fbe_drive_mgmt_drive_log_info_t * log_info = &drive_mgmt->drive_log_info;

    /* Check whether another download is in progress */
    old_value = fbe_atomic_exchange(&log_info->in_progress, FBE_TRUE);
    if (old_value == FBE_TRUE) {
        fbe_base_object_trace((fbe_base_object_t*)drive_mgmt, 
                              FBE_TRACE_LEVEL_WARNING,  
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Drivegetlog already in progress.\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    log_info->drive_location.bus = location->bus;
    log_info->drive_location.enclosure = location->enclosure;
    log_info->drive_location.slot = location->slot;
    
    status = fbe_drive_mgmt_get_eligible_drive_for_log(drive_mgmt);
    if (status == FBE_STATUS_OK)
    {

        /* Set GET_DRIVE_LOG condition. */
        status = fbe_lifecycle_set_cond(&FBE_LIFECYCLE_CONST_DATA(drive_mgmt), 
                                        (fbe_base_object_t*)drive_mgmt, 
                                        FBE_DRIVE_MGMT_GET_DRIVE_LOG_COND);
        if (status != FBE_STATUS_OK) {
            fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s can't set GET_DRIVE_LOG condition, status: 0x%X\n",
                                  __FUNCTION__, status);
            fbe_atomic_exchange(&log_info->in_progress, FBE_FALSE);
        }
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s drive not eligible to getlog, status: 0x%X\n",
                              __FUNCTION__, status);
        fbe_atomic_exchange(&log_info->in_progress, FBE_FALSE);
    }

    return status;
}
/******************************************************
 * end fbe_drive_mgmt_handle_drivegetlog_request() 
 ******************************************************/


/**************************************************************************
 *      fbe_drive_mgmt_get_drive_log()
 **************************************************************************
 *
 *  @brief
 *    This function initializes get Smart Dump (0xE6) feature.
 *
 *  @param
 *    drive_mgmt - pointer to drive mgmt structure.
 * 
 *  @return
 *     fbe_status_t - fbe status.
 *
 *  @author
 *   03/25/2011:  Created. chenl6
 *
 **************************************************************************/
fbe_status_t 
fbe_drive_mgmt_get_drive_log(fbe_drive_mgmt_t *drive_mgmt)
{
    fbe_status_t                                 status = FBE_STATUS_OK;
    fbe_drive_mgmt_drive_log_info_t            * log_info = &drive_mgmt->drive_log_info;
    fbe_physical_drive_get_smart_dump_asynch_t * drivegetlog_op = NULL;

    /* Check drive eligibility. */
    if (fbe_drive_mgmt_get_eligible_drive_for_log(drive_mgmt) == FBE_STATUS_OK)
    {
        if (log_info->log_buffer == NULL)
        {
            /* Initialize. */
            log_info->log_buffer = fbe_memory_native_allocate(DMO_DRIVE_GETLOG_BUFFER_ALLOC_LENGTH);
            if (log_info->log_buffer == NULL)
            {
                fbe_drive_mgmt_drive_log_cleanup(drive_mgmt);
                fbe_base_object_trace((fbe_base_object_t*)drive_mgmt, 
                                      FBE_TRACE_LEVEL_WARNING,  
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s Could not allocate header.\n", __FUNCTION__);
                return FBE_STATUS_GENERIC_FAILURE;
            }
        }

        if (log_info->drivegetlog_op == NULL)
        {
            /* Allocate drivegetlog_op */
            log_info->drivegetlog_op = (fbe_physical_drive_get_smart_dump_asynch_t *)
                                       fbe_base_env_memory_ex_allocate((fbe_base_environment_t *)drive_mgmt, sizeof(fbe_physical_drive_get_smart_dump_asynch_t));
            if (log_info->drivegetlog_op == NULL)
            {
                fbe_drive_mgmt_drive_log_cleanup(drive_mgmt);
                return FBE_STATUS_GENERIC_FAILURE;
            }
            drivegetlog_op = log_info->drivegetlog_op;
            drivegetlog_op->get_smart_dump.transfer_count  = DMO_DRIVE_GETLOG_BUFFER_ALLOC_LENGTH;
            drivegetlog_op->response_buf = log_info->log_buffer;
            drivegetlog_op->completion_function = fbe_drive_mgmt_get_drive_log_asynch_callback;
            drivegetlog_op->drive_mgmt = drive_mgmt;
        }

        /* Send asynch command to PDO. */
        status = fbe_api_physical_drive_drivegetlog_asynch(log_info->object_id, drivegetlog_op);
    }
    else
    {
        /* Clean up. */
        fbe_drive_mgmt_drive_log_cleanup(drive_mgmt);
    }

    return status;
}
/******************************************************
 * end fbe_drive_mgmt_get_drive_log() 
 ******************************************************/


/**************************************************************************
 *      fbe_drive_mgmt_get_drive_log_asynch_callback()
 **************************************************************************
 *
 *  @brief
 *    This function fills in parameters to read Smart Dump (0xE6) information.
 *
 *  @param
 *    drive_mgmt - pointer to drive mgmt structure.
 * 
 *  @return
 *     fbe_status_t - fbe status.
 *
 *  @author
 *   03/25/2011:  Created. chenl6
 *
 **************************************************************************/
static void
fbe_drive_mgmt_get_drive_log_asynch_callback(fbe_packet_completion_context_t context)
{
    fbe_physical_drive_get_smart_dump_asynch_t * drivegetlog_op = (fbe_physical_drive_get_smart_dump_asynch_t *)context;
    fbe_drive_mgmt_t                           * drive_mgmt = drivegetlog_op->drive_mgmt;
    fbe_status_t                                 status = FBE_STATUS_OK;

    if (drivegetlog_op->status == FBE_STATUS_OK &&
        drivegetlog_op->get_smart_dump.transfer_count)
    {
        /* Continue write to file */
        status = fbe_lifecycle_set_cond(&FBE_LIFECYCLE_CONST_DATA(drive_mgmt), 
                                        (fbe_base_object_t*)drive_mgmt, 
                                        FBE_DRIVE_MGMT_WRITE_DRIVE_LOG_COND);
        if (status != FBE_STATUS_OK) {
            fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s can't set WRITE_DRIVE_LOG condition, status: 0x%X\n",
                                  __FUNCTION__, status);
        }
    }
    else
    {
        /* Cleanup */
        fbe_drive_mgmt_drive_log_cleanup(drive_mgmt);
    }

    return;
}
/******************************************************
 * end fbe_drive_mgmt_get_drive_log_asynch_callback() 
 ******************************************************/


/**************************************************************************
 *      fbe_drive_mgmt_write_drive_log()
 **************************************************************************
 *
 *  @brief
 *    This function write Smart Dump (0xE6) to the file.
 *
 *  @param
 *    drive_mgmt - pointer to drive mgmt structure.
 * 
 *  @return
 *     fbe_status_t - fbe status.
 *
 *  @author
 *   03/25/2011:  Created. chenl6
 *
 **************************************************************************/
fbe_status_t 
fbe_drive_mgmt_write_drive_log(fbe_drive_mgmt_t *drive_mgmt)
{
    fbe_status_t                      status = FBE_STATUS_OK;
    fbe_drive_mgmt_drive_log_info_t * log_info = &drive_mgmt->drive_log_info;
    fbe_file_handle_t                 file;
    fbe_u32_t                         nbytes = 0;
    fbe_u8_t filename[128] = {'\0'};
    fbe_u8_t file_path[128] = {'\0'};
    fbe_u8_t drive_sn_trim[FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE+1] = {'\0'};
    int i = 0;
    int j = 0;

    
    /* Remove whitespaces from serial number, if any. */
    while(log_info->drive_sn[i])
    {
        if (!csx_isspace(log_info->drive_sn[i]))
        {
            drive_sn_trim[j] = log_info->drive_sn[i];
            j++;
        }
        i++;
    }
    
    fbe_sprintf(filename, sizeof(filename), "drive_log_%02d_%02d_%02d_%s.txt",
                log_info->drive_location.bus,
                log_info->drive_location.enclosure,
                log_info->drive_location.slot,
                drive_sn_trim);

    EmcutilFilePathMake(file_path, sizeof(file_path), EMCUTIL_BASE_DUMPS, filename, NULL);

    file = fbe_file_open(file_path, FBE_FILE_WRONLY|FBE_FILE_CREAT, 0, NULL);
 
    if (file == FBE_FILE_INVALID_HANDLE)
    {
        fbe_base_object_trace((fbe_base_object_t *) drive_mgmt, 
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s Could not create file %s.\n",
                                __FUNCTION__, file_path);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Write the buffer out to the persistent storage */
    nbytes = fbe_file_write(file, log_info->log_buffer, log_info->drivegetlog_op->get_smart_dump.transfer_count, NULL);

    /* We expect to read and write the number of bytes EXACTLY as it is in the fixed structure size */
    if((nbytes == 0) || (nbytes == FBE_FILE_ERROR) || (nbytes != log_info->drivegetlog_op->get_smart_dump.transfer_count))
    {
        fbe_base_object_trace((fbe_base_object_t *) drive_mgmt, 
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s Could not write to file, nbytes:%d \n",
                                __FUNCTION__, nbytes);
    }

    fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s Write Completed nbytes:%d. \n",
                          __FUNCTION__, nbytes);
    
    fbe_file_close(file);
    
    return status;
}
/******************************************************
 * end fbe_drive_mgmt_write_drive_log() 
 ******************************************************/


/**************************************************************************
 *      fbe_drive_mgmt_get_eligible_drive_for_log()
 **************************************************************************
 *
 *  @brief
 *    This function find eligible drive (drive with paddle card) for 
 *    getting Smart Dump (0xE6) feature.
 *
 *  @param
 *    drive_mgmt - pointer to drive mgmt structure.
 * 
 *  @return
 *     fbe_status_t - fbe status.
 *
 *  @author
 *   03/25/2011:  Created. chenl6
 *
 **************************************************************************/
fbe_status_t 
fbe_drive_mgmt_get_eligible_drive_for_log(fbe_drive_mgmt_t *drive_mgmt)
{
    fbe_drive_mgmt_drive_log_info_t * log_info = &drive_mgmt->drive_log_info;
    fbe_drive_info_t *                drive;
    fbe_u32_t                         i;

    /* Check drive eligibility */
    dmo_drive_array_lock(drive_mgmt,__FUNCTION__);
    
    for (i = 0; i < drive_mgmt->max_supported_drives; i++)
    {        
        drive = &(drive_mgmt->drive_info_array)[i];
        if ((drive->location.bus == log_info->drive_location.bus) &&
            (drive->location.enclosure == log_info->drive_location.enclosure) &&
            (drive->location.slot == log_info->drive_location.slot))
            
        {
            /* Only support drive with paddle card*/
            if(fbe_equal_memory(drive->vendor_id, "SATA-", 5) == FBE_FALSE )
            {
                dmo_drive_array_unlock(drive_mgmt,__FUNCTION__);
                return FBE_STATUS_GENERIC_FAILURE;
            }
                
            /* Found the drive */
            if (drive->state == FBE_LIFECYCLE_STATE_READY || drive->state == FBE_LIFECYCLE_STATE_FAIL)
            {
                log_info->object_id = drive->object_id;
                fbe_copy_memory(log_info->drive_sn, drive->sn, FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE);
                log_info->drive_sn[FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE] = 0;
                dmo_drive_array_unlock(drive_mgmt,__FUNCTION__);
                return FBE_STATUS_OK;
            }
            else
            {
                dmo_drive_array_unlock(drive_mgmt,__FUNCTION__);
                return FBE_STATUS_NO_OBJECT;
            }
        }
    }
    dmo_drive_array_unlock(drive_mgmt,__FUNCTION__);

    return FBE_STATUS_NO_OBJECT;
}
/******************************************************
 * end fbe_drive_mgmt_get_eligible_drive_for_log() 
 ******************************************************/



/**************************************************************************
 *      fbe_drive_mgmt_drive_log_cleanup()
 **************************************************************************
 *
 *  @brief
 *    This function cleans up drivegetlog feature.
 *
 *  @param
 *    drive_mgmt - pointer to drive mgmt structure.
 * 
 *  @return
 *     fbe_status_t - fbe status.
 *
 *  @author
 *   03/25/2011:  Created. chenl6
 *
 **************************************************************************/
void 
fbe_drive_mgmt_drive_log_cleanup(fbe_drive_mgmt_t *drive_mgmt)
{
    fbe_drive_mgmt_drive_log_info_t    * log_info = &drive_mgmt->drive_log_info;

    /* Clean up. */
    if (log_info->log_buffer)
    {
        fbe_memory_native_release(log_info->log_buffer);
        log_info->log_buffer = NULL;
    }
    if (log_info->drivegetlog_op)
    {
        fbe_base_env_memory_ex_release((fbe_base_environment_t *)drive_mgmt, log_info->drivegetlog_op);
        log_info->drivegetlog_op = NULL;
    }
    fbe_atomic_exchange(&log_info->in_progress, FBE_FALSE);

    return;
}
/******************************************************
 * end fbe_drive_mgmt_drive_log_cleanup() 
 ******************************************************/
