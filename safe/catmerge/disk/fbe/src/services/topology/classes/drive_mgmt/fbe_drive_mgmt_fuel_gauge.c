/***************************************************************************
 * Copyright (C) EMC Corporation 2010 
 * All rights reserved. Licensed material -- property of EMC 
 * Corporation 
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_drive_mgmt_fuel_gauge.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the Drive Management Fuel Gauge feature methods.
 * 
 * @ingroup drive_mgmt_class_files
 * 
 * @version
 *   11/16/10:  Created. chenl6
 *   09/04/12:  Added support for SMART data. chianc1
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
#include "swap_exports.h"
#include "fbe_base_environment_debug.h"

#include "fbe_drive_mgmt_private.h"
 
/*************************************
 * LOCAL FUNCTIONS
 ************************************/
static void fbe_drive_mgmt_fuel_gauge_read_asynch_callback(fbe_packet_completion_context_t context);
static fbe_u64_t fbe_drive_mgmt_fuel_gauge_get_Emulex_P30_attribute(fbe_u8_t *byte_read_ptr, fbe_u16_t attribute_id);
static fbe_u64_t fbe_drive_mgmt_fuel_gauge_get_STEC_power_on_hours(fbe_u8_t *byte_read_ptr);
static void fbe_drive_mgmt_fuel_gauge_fill_write_drive_header(fbe_drive_mgmt_t *drive_mgmt, fbe_u32_t * write_count, fbe_u8_t *byte_write_ptr);
static void fbe_drive_mgmt_fuel_gauge_fill_write_session_header(fbe_drive_mgmt_t *drive_mgmt, fbe_u32_t * write_count);
static void fbe_drive_mgmt_get_fuel_gauge_file(fbe_char_t *location);
static void fbe_drive_mgmt_get_fuel_gauge_file_most_recent(fbe_char_t *location);
static void fbe_drive_mgmt_fuel_gauge_parse_bms_data(fbe_drive_mgmt_t *drive_mgmt, fbe_dmo_fuel_gauge_bms_t *bms_results);
static void fbe_drive_mgmt_fuel_gauge_parse_log_page31(fbe_drive_mgmt_t *drive_mgmt);

/*************************************
 * LOCAL DEFINES
 ************************************/
#define DMO_FUEL_GAUGE_MAGIC_WORD           "DCS2"   /* magic word for session. Added rev. 2 for handling DOS in BMS.*/
#define DMO_FUEL_GAUGE_DRIVE_MAGIC_WORD     "FRU1"   /* magic word for Drive */
#define DMO_FUEL_GAUGE_DATA_MAGIC_WORD      "DATA"   /* magic word for data */

#define DMO_DRIVE_STATS_MOST_RECENT_FILENAME "drive_stats_most_recent.bin"

/*Array for storing supported log pages for the drives*/
fbe_drive_mgmt_fuel_gauge_max_PE_cycles_t dmo_max_PE_cycles[] =
{
    {"HITACHI ",      100000  }, 
    {"SATA-SAM",      100000  }, 
    {"SATA-MIC",      20000   }, 
};

const fbe_u16_t DMO_FUEL_GAUGE_MAX_PE_CYCLES_TBL_COUNT = sizeof(dmo_max_PE_cycles)/sizeof(dmo_max_PE_cycles[0]);

/*Array for storing supported log pages for the drives*/
fbe_drive_mgmt_fuel_gauge_supported_pages_t dmo_fuel_gauge_supported_pages[] =
{
    {LOG_PAGE_2,      FBE_FALSE   }, 
    {LOG_PAGE_3,      FBE_FALSE   },
    {LOG_PAGE_5,      FBE_FALSE   },
    {LOG_PAGE_11,     FBE_FALSE   }, 
    {LOG_PAGE_15,     FBE_FALSE   }, 
    {LOG_PAGE_2F,     FBE_FALSE   }, 
    {LOG_PAGE_30,     FBE_FALSE   }, 
    {LOG_PAGE_31,     FBE_FALSE   }, 
    {LOG_PAGE_37,     FBE_FALSE   }, 
};

const fbe_u16_t DMO_FUEL_GAUGE_SUPPORTED_PAGES_COUNT = sizeof(dmo_fuel_gauge_supported_pages)/sizeof(dmo_fuel_gauge_supported_pages[0]);

/**************************************************************************
 *      fbe_drive_mgmt_fuel_gauge_init()
 **************************************************************************
 *
 *  @brief  
 *    This function initializes fuel gauge feature.
 *
 *  @param
 *    drive_mgmt - pointer to drive mgmt structure.
 * 
 *  @return
 *     fbe_status_t - fbe status.
 *
 *  @author
 *     11/16/2010 - Created.  chenl6
 *
 **************************************************************************/
fbe_status_t 
fbe_drive_mgmt_fuel_gauge_init(fbe_drive_mgmt_t *drive_mgmt)
{
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_drive_mgmt_fuel_gauge_info_t  * fuel_gauge_info = &drive_mgmt->fuel_gauge_info; 
    fbe_atomic_t        is_lock = FBE_FALSE;
    fbe_u8_t            file_path[100];

    if (fuel_gauge_info->is_initialized)
    {
        return FBE_STATUS_OK;
    }

    /* Set Fuel Gauge is in process flag. */
    is_lock = fbe_atomic_exchange(&drive_mgmt->fuel_gauge_info.is_in_progrss, FBE_TRUE);
    if (is_lock == FBE_TRUE) {
        fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                              FBE_TRACE_LEVEL_DEBUG_HIGH,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                              "%s Fuel Gauge is_in_progrss flag is already set. \n",
                              __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
     
    /* Allocate read buffer which requires the physcially contiguous memory. */
    if (fuel_gauge_info->read_buffer == NULL)
    {
        /* Initialize. */
        fuel_gauge_info->read_buffer = fbe_memory_native_allocate(DMO_FUEL_GAUGE_READ_BUFFER_LENGTH);

        if (fuel_gauge_info->read_buffer == NULL)
        {
            fbe_base_object_trace((fbe_base_object_t*)drive_mgmt, 
                                  FBE_TRACE_LEVEL_WARNING,  
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s Could not allocate read buffer.\n", __FUNCTION__);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    /* Allocate write buffer */
    fuel_gauge_info->write_buffer = fbe_base_env_memory_ex_allocate((fbe_base_environment_t *)drive_mgmt, DMO_FUEL_GAUGE_WRITE_BUFFER_LENGTH);
    if (fuel_gauge_info->write_buffer == NULL)
    {
        fbe_memory_native_release(fuel_gauge_info->read_buffer);

        fuel_gauge_info->read_buffer = NULL;

        fbe_base_object_trace((fbe_base_object_t*)drive_mgmt, 
                              FBE_TRACE_LEVEL_WARNING,  
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Could not allocate write buffer.\n", __FUNCTION__);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Allocate fuel_gauge_op */
    fuel_gauge_info->fuel_gauge_op = (fbe_physical_drive_fuel_gauge_asynch_t *)
                     fbe_base_env_memory_ex_allocate((fbe_base_environment_t *)drive_mgmt, sizeof(fbe_physical_drive_fuel_gauge_asynch_t));
    if (fuel_gauge_info->fuel_gauge_op == NULL)
    {
        fbe_memory_native_release(fuel_gauge_info->read_buffer);

        fuel_gauge_info->read_buffer = NULL;

        fbe_base_env_memory_ex_release((fbe_base_environment_t *)drive_mgmt, fuel_gauge_info->write_buffer);
        return FBE_STATUS_GENERIC_FAILURE;
    }
 
    fbe_drive_mgmt_get_fuel_gauge_file(file_path);

    /* First try to open the file in APPEND mode. This will check if the file is there */
    fuel_gauge_info->file = fbe_file_open(file_path, (FBE_FILE_APPEND), 0, NULL);
    if(fuel_gauge_info->file == FBE_FILE_INVALID_HANDLE)
    {
        fbe_base_object_trace((fbe_base_object_t *) drive_mgmt, 
                               FBE_TRACE_LEVEL_INFO,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s Could not open file, creating new file.\n",
                               __FUNCTION__);
        /* The open failed, the file must not be there, so lets create a new file. */
        fuel_gauge_info->file = fbe_file_open(file_path, (FBE_FILE_WRONLY|FBE_FILE_CREAT), 0, NULL);
        if(fuel_gauge_info->file == FBE_FILE_INVALID_HANDLE)
        {
            fbe_base_object_trace((fbe_base_object_t *) drive_mgmt, 
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s Could not create file.\n",
                                __FUNCTION__);
            status = FBE_STATUS_GENERIC_FAILURE;
        }
        if (status != FBE_STATUS_OK)
        {
            fbe_memory_native_release(fuel_gauge_info->read_buffer);

            fuel_gauge_info->read_buffer = NULL;

            fbe_base_env_memory_ex_release((fbe_base_environment_t *)drive_mgmt, fuel_gauge_info->write_buffer);
            fuel_gauge_info->write_buffer = NULL;
			
            fbe_base_env_memory_ex_release((fbe_base_environment_t *)drive_mgmt, fuel_gauge_info->fuel_gauge_op);

            return FBE_STATUS_GENERIC_FAILURE;
        }
    }
    
    fbe_drive_mgmt_get_fuel_gauge_file_most_recent(file_path);
    fuel_gauge_info->file_most_recent = fbe_file_open(file_path, (FBE_FILE_WRONLY|FBE_FILE_CREAT|FBE_FILE_TRUNC), 0, NULL);
    if(fuel_gauge_info->file_most_recent == FBE_FILE_INVALID_HANDLE)
    {
        fbe_base_object_trace((fbe_base_object_t *) drive_mgmt, 
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s Could not create new (or overwrite existing) file.\n",
                               __FUNCTION__);

        /* Even though we failed to create this file, continue to do collection in the accumulated file.*/       
    }
        
    fbe_spinlock_lock(&drive_mgmt->fuel_gauge_info_lock);
    fuel_gauge_info->is_initialized = FBE_TRUE;
    fuel_gauge_info->need_session_header = FBE_TRUE;
    fbe_spinlock_unlock(&drive_mgmt->fuel_gauge_info_lock);
    
    return status;
}
/******************************************************
 * end fbe_drive_mgmt_fuel_gauge_init() 
 ******************************************************/

/*!************************************************************************
 *      fbe_drive_mgmt_fuel_gauge_cleanup()
 **************************************************************************
 *
 *  @brief
 *    This function initializes fuel gauge feature.
 *
 *  @param
 *    drive_mgmt - pointer to drive mgmt structure.
 * 
 *  @return
 *     fbe_status_t - fbe status.
 *
 *  @author
 *     11/16/2010 - Created.  chenl6
 *
 **************************************************************************/
void 
fbe_drive_mgmt_fuel_gauge_cleanup(fbe_drive_mgmt_t *drive_mgmt)
{
    fbe_drive_mgmt_fuel_gauge_info_t  * fuel_gauge_info = &drive_mgmt->fuel_gauge_info;   

    /* Reset Fuel Gauge is in process flag. */
   fbe_atomic_exchange(&drive_mgmt->fuel_gauge_info.is_in_progrss, FBE_FALSE);
    
    fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s Fuel Gauge is_in_progrss flag is turned off. \n",
                          __FUNCTION__);
   
    if (fuel_gauge_info->is_initialized)
    {
        if (fuel_gauge_info->read_buffer)
        {
            fbe_memory_native_release(fuel_gauge_info->read_buffer);
            fuel_gauge_info->read_buffer = NULL;
        }
        
        fbe_base_env_memory_ex_release((fbe_base_environment_t *)drive_mgmt, fuel_gauge_info->write_buffer);
        fbe_base_env_memory_ex_release((fbe_base_environment_t *)drive_mgmt, fuel_gauge_info->fuel_gauge_op);
        fbe_file_close(fuel_gauge_info->file);
        if(fuel_gauge_info->file_most_recent != FBE_FILE_INVALID_HANDLE) 
        {
            fbe_file_close(fuel_gauge_info->file_most_recent);
        }
        
        fbe_spinlock_lock(&drive_mgmt->fuel_gauge_info_lock);
        fuel_gauge_info->is_initialized = FBE_FALSE;
        fuel_gauge_info->is_fbecli_cmd = FBE_FALSE;
        
        /* Initialize index */
        fuel_gauge_info->current_index = 0;
        fuel_gauge_info->current_supported_page_index = LOG_PAGE_NONE;
        
        fbe_spinlock_unlock(&drive_mgmt->fuel_gauge_info_lock);
    }

    return;
}
/******************************************************
 * end fbe_drive_mgmt_fuel_gauge_cleanup() 
 ******************************************************/

/*!************************************************************************
 *      fbe_drive_mgmt_fuel_gauge_get_eligible_drive()
 **************************************************************************
 *
 *  @brief
 *    This function returns next eligible drive for fuel gauge.
 *    error message.
 *
 *  @param
 *    drive_mgmt - pointer to drive mgmt structure.
 * 
 *  @return
 *     fbe_status_t - fbe status.
 *
 *  @author
 *     11/15/2010 - Created.  chenl6
 *
 **************************************************************************/
fbe_status_t 
fbe_drive_mgmt_fuel_gauge_get_eligible_drive(fbe_drive_mgmt_t *drive_mgmt)
{
    fbe_drive_info_t *  drive;
    fbe_drive_mgmt_fuel_gauge_info_t  * fuel_gauge_info = &drive_mgmt->fuel_gauge_info;   
    fbe_u32_t           i;
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;

   /* If log page table is not done yet, won't get the next drive. */
    if (drive_mgmt->fuel_gauge_info.current_supported_page_index != LOG_PAGE_NONE)
    {
        return status; 
    }

    dmo_drive_array_lock(drive_mgmt,__FUNCTION__); // higher level lock
   
    for (i = fuel_gauge_info->current_index; i < drive_mgmt->max_supported_drives; i++)
    {     
        drive = &(drive_mgmt->drive_info_array)[i];
        if ((drive->state == FBE_LIFECYCLE_STATE_READY) &&
               ((drive->last_log == 0) || (fuel_gauge_info->is_fbecli_cmd == FBE_TRUE) ||
               (fbe_get_elapsed_seconds(drive->last_log) > fuel_gauge_info->min_poll_interval) ))
        {
            /* Now we found the drive */
            fuel_gauge_info->current_object = drive->object_id;
            fuel_gauge_info->current_location = drive->location;
            fuel_gauge_info->currrent_drive_type = drive->drive_type;
            fbe_copy_memory(fuel_gauge_info->current_vendor, drive->vendor_id, FBE_SCSI_INQUIRY_VENDOR_ID_SIZE);
            (fuel_gauge_info->current_vendor)[FBE_SCSI_INQUIRY_VENDOR_ID_SIZE] = 0;
            fbe_copy_memory(fuel_gauge_info->current_sn, drive->sn, FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE);
            (fuel_gauge_info->current_sn)[FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE] = 0;
            fbe_copy_memory(fuel_gauge_info->current_tla, drive->tla, FBE_SCSI_INQUIRY_TLA_SIZE);
            (fuel_gauge_info->current_tla)[FBE_SCSI_INQUIRY_TLA_SIZE] = 0;
            fbe_copy_memory(fuel_gauge_info->current_fw_rev, drive->rev, FBE_SCSI_INQUIRY_VPD_PROD_REV_SIZE);
            (fuel_gauge_info->current_fw_rev)[FBE_SCSI_INQUIRY_VPD_PROD_REV_SIZE] = 0;
            drive->last_log = fbe_get_time();
            
            fuel_gauge_info->need_drive_header = FBE_TRUE;
            fuel_gauge_info->power_on_hours = 0;
            fuel_gauge_info->current_PE_cycles = 0;
            fuel_gauge_info->EOL_PE_cycles = 0;
            
            fuel_gauge_info->current_index = (i + 1);  /* next drive to start search */
            status = FBE_STATUS_OK;
            break;
        }
    }
    
    dmo_drive_array_unlock(drive_mgmt,__FUNCTION__);

    return status;
}
/******************************************************
 * end fbe_drive_mgmt_fuel_gauge_get_eligible_drive() 
 ******************************************************/

/*!************************************************************************
 *      fbe_drive_mgmt_fuel_gauge_fill_params()
 **************************************************************************
 *
 *  @brief
 *    This function fills in parameters to read fuel gauge information.
 *
 *  @param
 *    drive_mgmt - pointer to drive mgmt structure.
 * 
 *  @return
 *     fbe_status_t - fbe status.
 *
 *  @author
 *     11/16/2010 - Created.  chenl6
 *
 **************************************************************************/
void 
fbe_drive_mgmt_fuel_gauge_fill_params(fbe_drive_mgmt_t *drive_mgmt, fbe_physical_drive_fuel_gauge_asynch_t * fuel_gauge_op, fbe_u8_t page_code)
{
    fuel_gauge_op->get_log_page.transfer_count  = DMO_FUEL_GAUGE_READ_BUFFER_LENGTH;
    fuel_gauge_op->response_buf = drive_mgmt->fuel_gauge_info.read_buffer;
    fuel_gauge_op->get_log_page.page_code = page_code;
    fuel_gauge_op->completion_function = fbe_drive_mgmt_fuel_gauge_read_asynch_callback;
    fuel_gauge_op->drive_mgmt = drive_mgmt;
    
    return;
}
/******************************************************
 * end fbe_drive_mgmt_fuel_gauge_fill_params() 
 ******************************************************/

/*!************************************************************************
 *      fbe_drive_mgmt_fuel_gauge_find_valid_log_page_code()
 **************************************************************************
 *  @brief
 *    This routine finds the valid log page code from the supported log pages page for the drive.
 *
 *  @param
 *    byte_read_ptr - Pointer to read buffer.
 * 
 *  @return
 *     None
 *
 *  @author
 *     08/18/2012 - Created.  Christina Chiang
 *
 **************************************************************************/
void 
fbe_drive_mgmt_fuel_gauge_find_valid_log_page_code(UINT_8 *byte_read_ptr, fbe_physical_drive_fuel_gauge_asynch_t * fuel_gauge_op)
{
    fbe_drive_mgmt_fuel_gauge_page_header_t *page_header_ptr;
    fbe_drive_mgmt_t   *drive_mgmt = fuel_gauge_op->drive_mgmt;
    fbe_u16_t i;
    fbe_u8_t j;
    fbe_u32_t page_len;
    fbe_u8_t  page_code;
        
    page_header_ptr = (fbe_drive_mgmt_fuel_gauge_page_header_t *)byte_read_ptr;
    page_len =  ((page_header_ptr->page_length_high << 8) & 0xFF00) | page_header_ptr->page_length_low;
    
    /* page code is byte 0 bit 0-5 */
    if (fuel_gauge_op->get_log_page.page_code != LOG_PAGE_0)
    {
        fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s did NOT find a valid log page code = %d \n",
                                    __FUNCTION__, fuel_gauge_op->get_log_page.page_code);
        return;
    }
    
    /* Clear the supported flag. */
    for (j = 0; j < DMO_FUEL_GAUGE_SUPPORTED_PAGES_COUNT; j++)
    {
        dmo_fuel_gauge_supported_pages[j].supported = FBE_FALSE;    
    }

    drive_mgmt->fuel_gauge_info.current_supported_page_index = LOG_PAGE_NONE;
    
    byte_read_ptr += sizeof(fbe_drive_mgmt_fuel_gauge_page_header_t);

    /* Update the page supported table. */
    for (i = 0; i < page_len; i++)
    {
        page_code = *byte_read_ptr;
         
        for ( j = 0; j < DMO_FUEL_GAUGE_SUPPORTED_PAGES_COUNT; j++)
        {
            if (page_code == dmo_fuel_gauge_supported_pages[j].page_code)
            {
                fbe_spinlock_lock(&drive_mgmt->fuel_gauge_info_lock);
                dmo_fuel_gauge_supported_pages[j].supported = FBE_TRUE;
                
                /* Set the index for the first supported page. */
                if (drive_mgmt->fuel_gauge_info.current_supported_page_index == LOG_PAGE_NONE)
                {
                    drive_mgmt->fuel_gauge_info.current_supported_page_index = j; 
                }
                
                fbe_spinlock_unlock(&drive_mgmt->fuel_gauge_info_lock);
 
            }
        }
        byte_read_ptr++;
    }

    return;
}
/**************************************************************************
 *      end fbe_drive_mgmt_fuel_gauge_find_valid_log_page_code()
 **************************************************************************/

/*!************************************************************************
 *      fbe_drive_mgmt_fuel_gauge_read_asynch_callback()
 **************************************************************************
 *
 *  @brief
 *    This function fills in parameters to read fuel gauge information.
 *
 *  @param
 *    drive_mgmt - pointer to drive mgmt structure.
 * 
 *  @return
 *     fbe_status_t - fbe status.
 *
 *  @author
 *     11/16/2010 - Created.  chenl6
 *     08/31/2012 - Updated.  chianc1
 *
 **************************************************************************/
static void
fbe_drive_mgmt_fuel_gauge_read_asynch_callback(fbe_packet_completion_context_t context)
{
    fbe_physical_drive_fuel_gauge_asynch_t *fuel_gauge_op = (fbe_physical_drive_fuel_gauge_asynch_t *)context;
    fbe_drive_mgmt_t   *drive_mgmt = fuel_gauge_op->drive_mgmt;
    fbe_status_t        status = FBE_STATUS_OK;

    if ((fuel_gauge_op->status == FBE_STATUS_OK) &&  (fuel_gauge_op->get_log_page.page_code == LOG_PAGE_0))
    {
        /* Set FBE_DRIVE_MGMT_FUEL_GAUGE_UPDATE_PAGES_TBL_COND to search supported log pages page to 
         *find the correct log page 0x30, 0x31 or other log pages for the drivr. */
        fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                                  FBE_TRACE_LEVEL_DEBUG_HIGH,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                  "%s Find the supported log pages from the log page 0x%X. \n",
                                  __FUNCTION__, fuel_gauge_op->get_log_page.page_code);
                
        /* Continue to read */
        status = fbe_lifecycle_set_cond(&FBE_LIFECYCLE_CONST_DATA(drive_mgmt), 
                                  (fbe_base_object_t*)drive_mgmt, 
                                  FBE_DRIVE_MGMT_FUEL_GAUGE_UPDATE_PAGES_TBL_COND);
                
        if (status != FBE_STATUS_OK) {
            fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                                   FBE_TRACE_LEVEL_ERROR,
                                   FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                   "%s can't set FBE_DRIVE_MGMT_FUEL_GAUGE_UPDATE_PAGES_TBL_COND, status: 0x%X\n",
                                   __FUNCTION__, status);
        }
        
        return;
    }
    
   
    /* Write supported log page. */
    if (fuel_gauge_op->status == FBE_STATUS_OK) 
    {
        /* Continue write to file */
        status = fbe_lifecycle_set_cond(&FBE_LIFECYCLE_CONST_DATA(drive_mgmt), 
                                        (fbe_base_object_t*)drive_mgmt, 
                                        FBE_DRIVE_MGMT_FUEL_GAUGE_WRITE_COND);
        
        if (status != FBE_STATUS_OK) {
            fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s can't set READ condition, status: 0x%X\n",
                                  __FUNCTION__, status);
        }
    } 
    else
    {
        /* No data to write. Continue to read */
        status = fbe_lifecycle_set_cond(&FBE_LIFECYCLE_CONST_DATA(drive_mgmt), 
                                       (fbe_base_object_t*)drive_mgmt, 
                                       FBE_DRIVE_MGMT_FUEL_GAUGE_READ_COND);
        if (status != FBE_STATUS_OK) {
                    fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s can't set READ condition, status: 0x%X\n",
                                  __FUNCTION__, status);
        }
    }
    
    return;
}
/******************************************************
 * end fbe_drive_mgmt_fuel_gauge_read_asynch_callback() 
 ******************************************************/

/*!************************************************************************
 *      fbe_drive_mgmt_fuel_gauge_get_data_variant_id()
 **************************************************************************
 *  @brief
 *    This routine finds data_variant_id for the drive type.
 *
 *  @param
 *    page code
 * 
 *  @return
 *     fbe_u8_t   data_variant_id.
 *        1. Unused/Unknown Data Variant Identifier
 *        2. SMART Data Log Page (0x30, 0x34, 0x37)
 *        3. Fuel Gauge Data (0x31)
 *
 *  @author
 *     08/20/2012 - Created.  Christina Chiang
 **************************************************************************/
fbe_u8_t 
fbe_drive_mgmt_fuel_gauge_get_data_variant_id(fbe_u8_t  page_code)
{
    fbe_u8_t data_variant_id = UNUSED_VARIANT_ID;  
   
    switch (page_code) 
    {
        case LOG_PAGE_2:
            data_variant_id = WRITE_ERROR_COUNTERS;
            break;
    
        case LOG_PAGE_3:
            data_variant_id = READ_ERROR_COUNTERS;
            break;
    
        case LOG_PAGE_5:
            data_variant_id = VERIFY_ERROR_COUNTERS;
            break;
    
        case LOG_PAGE_11:
            data_variant_id = SSD_MEDIA;
            break;
    
        case LOG_PAGE_15:   
            data_variant_id = BMS_DATA1;
            break;
    
        case LOG_PAGE_2F:
            data_variant_id = INFO_EXCEPTIONS;
            break;
    
        case LOG_PAGE_30:
            data_variant_id = SMART_DATA;
            break;
    
        case LOG_PAGE_31:   
            data_variant_id = FUEL_GAUGE_DATA;
            break;
    
        case LOG_PAGE_37:
            data_variant_id = VENDOR_SPECIFIC_SMART;
            break;
    
        default:
            data_variant_id = UNUSED_VARIANT_ID;
            break;
    } 
         
    return data_variant_id;
}
/**************************************************************************
 *    end fbe_drive_mgmt_fuel_gauge_get_data_variant_id()
 **************************************************************************/

/*!************************************************************************
 *      fbe_drive_mgmt_fuel_gauge_write_to_file()
 **************************************************************************
 *
 *  @brief
 *    This function writes fuel gauge information to file.
 *
 *  @param
 *    drive_mgmt - pointer to drive mgmt structure.
 * 
 *  @return
 *     fbe_status_t - fbe status.
 *
 *  @author
 *     11/16/2010 - Created.  chenl6
 *
 **************************************************************************/
fbe_status_t 
fbe_drive_mgmt_fuel_gauge_write_to_file(fbe_drive_mgmt_t *drive_mgmt)
{
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_u32_t           nbytes = 0, nbytes2 = 0, bytes_to_write = 0;
    fbe_drive_mgmt_fuel_gauge_info_t  * fuel_gauge_info = &drive_mgmt->fuel_gauge_info;   

    /* Copy the data to the write buffer */
    fbe_drive_mgmt_fuel_gauge_fill_write_buffer(drive_mgmt, &bytes_to_write);
 
    nbytes = fbe_file_write(fuel_gauge_info->file, drive_mgmt->fuel_gauge_info.write_buffer, bytes_to_write, NULL);
    if(fuel_gauge_info->file_most_recent != FBE_FILE_INVALID_HANDLE) 
    {
        nbytes2 = fbe_file_write(fuel_gauge_info->file_most_recent, drive_mgmt->fuel_gauge_info.write_buffer, bytes_to_write, NULL);
    }


    /* We expect to read and write the number of bytes EXACTLY as it is in the fixed structure size */
    if((nbytes != nbytes2) || (nbytes == 0) || (nbytes == FBE_FILE_ERROR) || (nbytes != bytes_to_write))
    {
        fbe_base_object_trace((fbe_base_object_t *) drive_mgmt, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s Could not write to file, expect:%d nbytes:%d nbytes2:%d\n",
                                __FUNCTION__, bytes_to_write, nbytes, nbytes2);
    }

    fbe_base_object_trace((fbe_base_object_t *) drive_mgmt, 
                         FBE_TRACE_LEVEL_INFO,
                         FBE_TRACE_MESSAGE_ID_INFO,
                         "%s Completed. \n",
                         __FUNCTION__);
    return status;
}
/******************************************************
 * end fbe_drive_mgmt_fuel_gauge_write_to_file() 
 ******************************************************/

/*!************************************************************************
 *      fbe_drive_mgmt_fuel_gauge_get_timestamp()
 **************************************************************************
 *  @brief
 *    This routine get the time stamp from TOD. 
 *    Init the time_data to prevent if the tod_get_time()fails, time_stamp get the bogus data. 
 *
 *  @param
 *    drive_mgmt - pointer to drive mgmt structure.
 *    time_stamp - pointer of time stamp.
 * 
 *  @return
 *     None
 *
 *  @author
 *     08/17/2012 - Created.  Christina Chiang
 *
 **************************************************************************/
void
fbe_drive_mgmt_fuel_gauge_get_timestamp(fbe_drive_mgmt_fuel_gauge_time_data_t * p_system_time)
{
    fbe_system_time_t system_time = {0};

    fbe_get_system_time(&system_time);
    p_system_time->year = (fbe_u8_t)(system_time.year % 100);
    p_system_time->month = (fbe_u8_t)system_time.month;
    p_system_time->day = (fbe_u8_t)system_time.day;
    p_system_time->hour = (fbe_u8_t)system_time.hour;
    p_system_time->minute = (fbe_u8_t)system_time.minute;
    p_system_time->second = (fbe_u8_t)system_time.second;

    return;
}

/******************************************************
 * end fbe_drive_mgmt_fuel_gauge_get_timestamp() 
 ******************************************************/

/*!************************************************************************
 *      fbe_drive_mgmt_fuel_gauge_fill_write_session_header()
 **************************************************************************
 *
 *  @brief
 *    This function fills session header in buffer to write fuel gauge information.
 *
 *  @param
 *    drive_mgmt - pointer to drive mgmt structure.
 *    write_count - bytes to be written.
 * 
 *  @return
 *     NULL.
 *
 *  @author
 *     08/18/2012 - Created.  chianc1
 *
 **************************************************************************/
static void 
fbe_drive_mgmt_fuel_gauge_fill_write_session_header(fbe_drive_mgmt_t *drive_mgmt, fbe_u32_t * write_count)
{
    fbe_drive_mgmt_fuel_gauge_info_t  * fuel_gauge_info = &drive_mgmt->fuel_gauge_info;   
    fbe_u8_t *byte_write_ptr = fuel_gauge_info->write_buffer;
    fbe_drive_mgmt_fuel_gauge_session_header_t session_header;
   
    if (fuel_gauge_info->need_session_header == FBE_FALSE)
    {   
        /* if we are still in the same session, not need for write the session header. */
        *write_count = 0;
        return;
    }

    /* Init Write buffer. */
    fbe_zero_memory(byte_write_ptr, DMO_FUEL_GAUGE_SESSION_HEADER_SIZE);
   
    /* Init session header. */
    fbe_zero_memory(&session_header, DMO_FUEL_GAUGE_SESSION_HEADER_SIZE);

    /* Store data to the session header */
    strncpy(session_header.magic_word, DMO_FUEL_GAUGE_MAGIC_WORD, strlen(DMO_FUEL_GAUGE_MAGIC_WORD));  
    session_header.session_header_len_high = (DMO_FUEL_GAUGE_SESSION_HEADER_SIZE >> 8) & 0xFF; 
    session_header.session_header_len_low = DMO_FUEL_GAUGE_SESSION_HEADER_SIZE & 0xFF;
               
    session_header.tool_id = FUEL_GAUGE_TOOL;
    session_header.file_fmt_rev_major = 0x01;
    session_header.file_fmt_rev_minor = 0x00;
     
    session_header.tool_rev_major = 0x01;
    session_header.tool_rev_minor = 0x00;

    fbe_drive_mgmt_fuel_gauge_get_timestamp(&(session_header.time_stamp));

    fbe_copy_memory (byte_write_ptr, &session_header, DMO_FUEL_GAUGE_SESSION_HEADER_SIZE );
    *write_count += DMO_FUEL_GAUGE_SESSION_HEADER_SIZE;
    fuel_gauge_info->need_session_header = FBE_FALSE;
    
    return;
}
/************************************************************
 * end fbe_drive_mgmt_fuel_gauge_fill_write_session_header() 
 ***********************************************************/
 
/*!************************************************************************
 *      fbe_drive_mgmt_fuel_gauge_fill_write_drive_header()
 **************************************************************************
 *
 *  @brief
 *    This function fills drive header in buffer to write fuel gauge information.
 *
 *  @param
 *    drive_mgmt - pointer to drive mgmt structure.
 *    write_count - bytes to be written.
 *    byte_write_ptr - current pointer of byte_write_ptr.
 * 
 *  @return
 *     NULL.
 *
 *  @author
 *     08/18/2012 - Created.  chianc1
 *
 **************************************************************************/
static void 
fbe_drive_mgmt_fuel_gauge_fill_write_drive_header(fbe_drive_mgmt_t *drive_mgmt, fbe_u32_t * write_count, fbe_u8_t *byte_write_ptr)
{
    fbe_drive_mgmt_fuel_gauge_info_t       * fuel_gauge_info = &drive_mgmt->fuel_gauge_info;   
    fbe_drive_mgmt_fuel_gauge_drive_header_t drive_header = {0};
    fbe_object_id_t                          object_id;
     
    if (fuel_gauge_info->need_drive_header == FBE_FALSE)
    {   
        /* if we are still in the same drive, not need for write the drive header. */
        *write_count = 0;
        return;
    }
 
    fbe_zero_memory(byte_write_ptr, DMO_FUEL_GAUGE_DRIVE_HEADER_SIZE);
    
    /* Store data for Drive header. */
    strncpy(drive_header.magic_word, DMO_FUEL_GAUGE_DRIVE_MAGIC_WORD, strlen(DMO_FUEL_GAUGE_DRIVE_MAGIC_WORD));  
    drive_header.drive_header_len_high = (DMO_FUEL_GAUGE_DRIVE_HEADER_SIZE >> 8) & 0xFF;
    drive_header.drive_header_len_low = DMO_FUEL_GAUGE_DRIVE_HEADER_SIZE & 0xFF;
    
    object_id = fuel_gauge_info->current_object;
    drive_header.object_id[0] = (object_id >> 24) & 0xFF;
    drive_header.object_id[1] = (object_id >> 16) & 0xFF;
    drive_header.object_id[2] = (object_id >> 8) &  0xFF;
    drive_header.object_id[3] = object_id & 0xFF;
    
    drive_header.bus_num = fuel_gauge_info->current_location.bus;
    drive_header.enclosure_num = fuel_gauge_info->current_location.enclosure;
    drive_header.slot_num = fuel_gauge_info->current_location.slot;
    
    strncpy (drive_header.serial_num, fuel_gauge_info->current_sn, FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE);
    drive_header.serial_num[FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE] = '\0';
    
    strncpy (drive_header.TLA_num, fuel_gauge_info->current_tla, FBE_SCSI_INQUIRY_TLA_SIZE);
    drive_header.TLA_num[FBE_SCSI_INQUIRY_TLA_SIZE] = '\0';
    
    strncpy (drive_header.FW_rev, fuel_gauge_info->current_fw_rev, FBE_SCSI_INQUIRY_VPD_PROD_REV_SIZE);
    drive_header.FW_rev[FBE_SCSI_INQUIRY_VPD_PROD_REV_SIZE] = '\0';
    
    fbe_drive_mgmt_fuel_gauge_get_timestamp(&(drive_header.time_stamp));
    
    fbe_copy_memory (byte_write_ptr, &drive_header, DMO_FUEL_GAUGE_DRIVE_HEADER_SIZE);
    
    *write_count = DMO_FUEL_GAUGE_DRIVE_HEADER_SIZE;
    
    fuel_gauge_info->need_drive_header = FBE_FALSE;
    return;
}
/************************************************************
 * end fbe_drive_mgmt_fuel_gauge_fill_write_drive_header() 
 ***********************************************************/

/*!************************************************************************
 *      fbe_drive_mgmt_fuel_gauge_fill_write_buffer()
 **************************************************************************
 *
 *  @brief
 *    This function fills in buffer to write fuel gauge information for each drive.
 *
 *  @param
 *    drive_mgmt - pointer to drive mgmt structure.
 *    write_count - bytes to be written.
 * 
 *  @return
 *     NULL.
 *
 *  @author
 *     11/16/2010 - Created.  chenl6
 *     08/18/2012 - Added SMART data.  Chianc1
 *
 **************************************************************************/
void 
fbe_drive_mgmt_fuel_gauge_fill_write_buffer(fbe_drive_mgmt_t *drive_mgmt, fbe_u32_t * write_count)
{
    fbe_drive_mgmt_fuel_gauge_info_t  * fuel_gauge_info = &drive_mgmt->fuel_gauge_info;   
    fbe_u8_t *byte_read_ptr = fuel_gauge_info->read_buffer;
    fbe_u8_t *byte_write_ptr = fuel_gauge_info->write_buffer;
    fbe_u32_t bytes_to_write = 0;
    fbe_drive_mgmt_fuel_gauge_data_header_t data_header = {0};
    fbe_drive_mgmt_fuel_gauge_page_header_t *page_header_ptr = NULL;
    fbe_u32_t xfer_count = 0;
    fbe_u32_t temp_count = 0;
    fbe_u32_t bms_output_len = 0;
    fbe_u32_t len;
    fbe_dmo_fuel_gauge_bms_t bms_results;

    fbe_spinlock_lock(&drive_mgmt->fuel_gauge_info_lock);
    
    /* Write session header. */
    fbe_drive_mgmt_fuel_gauge_fill_write_session_header(drive_mgmt, &bytes_to_write);
    byte_write_ptr += bytes_to_write;
    *write_count += bytes_to_write;

    /* Write drive header. */
    fbe_drive_mgmt_fuel_gauge_fill_write_drive_header(drive_mgmt, &bytes_to_write, byte_write_ptr);
    byte_write_ptr += bytes_to_write;
    *write_count += bytes_to_write;
     
    /* Write Data Header. */
    fbe_zero_memory(byte_write_ptr, DMO_FUEL_GAUGE_DATA_HEADER_SIZE);

    strncpy(data_header.data_magic_word, DMO_FUEL_GAUGE_DATA_MAGIC_WORD, strlen(DMO_FUEL_GAUGE_DATA_MAGIC_WORD));
    
    /* Read from buffer_read and write to buffer_write. */
    page_header_ptr = (fbe_drive_mgmt_fuel_gauge_page_header_t *)byte_read_ptr;

    /* Add 4 extra bytes for the log page header */
    xfer_count =  ( ((page_header_ptr->page_length_high << 8) & 0xFF00) | page_header_ptr->page_length_low) + 4;

    fuel_gauge_info->fuel_gauge_op->get_log_page.transfer_count =  xfer_count;
    
    data_header.data_variant_id = fbe_drive_mgmt_fuel_gauge_get_data_variant_id(fuel_gauge_info->fuel_gauge_op->get_log_page.page_code);
    
    /* Store data header and data to write buffer*/
    if (data_header.data_variant_id == BMS_DATA1)
    {
        /* the length for BMS is sizeof (fbe_dmo_fuel_gauge_bms_t). */
        bms_output_len = sizeof (fbe_dmo_fuel_gauge_bms_t);
        len = bms_output_len + DMO_FUEL_GAUGE_DATA_HEADER_SIZE;
        
        data_header.page_size_high = (len >> 8) & 0xFF;
        data_header.page_size_low = (len & 0xFF); 
        
        fbe_copy_memory (byte_write_ptr, &data_header, DMO_FUEL_GAUGE_DATA_HEADER_SIZE);
        byte_write_ptr += DMO_FUEL_GAUGE_DATA_HEADER_SIZE;
        *write_count += DMO_FUEL_GAUGE_DATA_HEADER_SIZE;
        
        fbe_drive_mgmt_fuel_gauge_parse_bms_data(drive_mgmt, &bms_results);
        
        /* copy raw log page from read buffer to write buffer */
        fbe_copy_memory(byte_write_ptr, &bms_results, bms_output_len);
        *write_count += bms_output_len;
    }
    else
    {
        /* Fuel Gauge and SMART data. */
        len = ((page_header_ptr->page_length_high << 8) & 0xFF00) +
              page_header_ptr->page_length_low+
              DMO_FUEL_GAUGE_DATA_HEADER_SIZE + 
              4;  /* extra bytes for log page header */
        data_header.page_size_high = (len >> 8) & 0xFF;; 
        data_header.page_size_low = (len & 0xFF); 
        
        fbe_copy_memory (byte_write_ptr, &data_header, DMO_FUEL_GAUGE_DATA_HEADER_SIZE);
        byte_write_ptr += DMO_FUEL_GAUGE_DATA_HEADER_SIZE;
        *write_count += DMO_FUEL_GAUGE_DATA_HEADER_SIZE;
        
        /* In case we don't get the page length from the drive, we will set the xfer_count to 
        DMO_FUEL_GAUGE_WRITE_BUFFER_LENGTH - *write_count. */
        temp_count = DMO_FUEL_GAUGE_WRITE_BUFFER_LENGTH - *write_count;
        if (xfer_count  > temp_count)
        {
            xfer_count = temp_count; 
        }
        
       /* copy raw log page from read buffer to write buffer */
        fbe_copy_memory(byte_write_ptr, byte_read_ptr, xfer_count);
        *write_count += xfer_count;
    }
  
    fbe_base_object_trace((fbe_base_object_t *) drive_mgmt, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s: xfer_count= %d. write count = %d \n",
                          __FUNCTION__, xfer_count, *write_count );
    
    fbe_drive_mgmt_fuel_gauge_get_PE_cycle_warrantee_info(drive_mgmt);
    
    fbe_spinlock_unlock(&drive_mgmt->fuel_gauge_info_lock);
  
    /* Handle eventlog for BMS after spinlock is unlock. */
    if (data_header.data_variant_id == BMS_DATA1)
    {
        fbe_drive_mgmt_fuel_gauge_check_BMS_alerts(drive_mgmt, &bms_results);
    }
    
    return;
}
/******************************************************
 * end fbe_drive_mgmt_fuel_gauge_fill_write_buffer() 
 ******************************************************/

/*!************************************************************************
 *      fbe_drive_mgmt_fuel_gauge_get_PE_cycle_warrantee_info()
 **************************************************************************
 *
 *  @brief
 *    This function get the worst PE cycle warrantee limit for SSD drives with vendor 
 *    (Samsung, Hitachi, Micron, or  STEC). 
 *
 *  @param
 *    drive_mgmt - pointer to drive mgmt structure.
 * 
 *  @return
 *     NULL.
 *
 *  @author
 *     09/14/2012 - Created.  Chianc1
 *
 **************************************************************************/
void 
fbe_drive_mgmt_fuel_gauge_get_PE_cycle_warrantee_info(fbe_drive_mgmt_t *drive_mgmt)
{
    fbe_drive_mgmt_fuel_gauge_info_t        * fuel_gauge_info = &drive_mgmt->fuel_gauge_info;   
    fbe_u8_t                                *byte_read_ptr = fuel_gauge_info->read_buffer;
    fbe_drive_mgmt_fuel_gauge_page_header_t *page_header_ptr;
    fbe_u8_t                                page_code;
   
    /* Read from buffer_read and write to buffer_write. */
    page_header_ptr = (fbe_drive_mgmt_fuel_gauge_page_header_t *)byte_read_ptr;
    page_code = (page_header_ptr->page_code) & 0x3f;
        
    if (page_code != fuel_gauge_info->fuel_gauge_op->get_log_page.page_code)
    {
        fbe_base_object_trace((fbe_base_object_t *) drive_mgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s page code %x doesn't match the one in the drive log page %x. \n",
                              __FUNCTION__, page_code, fuel_gauge_info->fuel_gauge_op->get_log_page.page_code );
        return;
    }
    
    /* Get Power On Hours and currecnt PE cycles from the drive. */
    if (fuel_gauge_info->currrent_drive_type == FBE_DRIVE_TYPE_SAS_FLASH_HE ||
        fuel_gauge_info->currrent_drive_type == FBE_DRIVE_TYPE_SATA_FLASH_HE ||
        fuel_gauge_info->currrent_drive_type == FBE_DRIVE_TYPE_SAS_FLASH_ME ||
        fuel_gauge_info->currrent_drive_type == FBE_DRIVE_TYPE_SAS_FLASH_LE ||
        fuel_gauge_info->currrent_drive_type == FBE_DRIVE_TYPE_SAS_FLASH_RI)
    {
        switch (page_code) {
            case LOG_PAGE_30:
                if ((fbe_equal_memory(fuel_gauge_info->current_vendor, "SATA-SAM", 8)) || (fbe_equal_memory(fuel_gauge_info->current_vendor, "SATA-MIC", 8)))
                {   
                  fuel_gauge_info->power_on_hours = fbe_drive_mgmt_fuel_gauge_get_Emulex_P30_attribute(byte_read_ptr, 0x09);
                  fuel_gauge_info->current_PE_cycles = fbe_drive_mgmt_fuel_gauge_get_Emulex_P30_attribute(byte_read_ptr, 0xAD);
                }
                break;
                
            case LOG_PAGE_31:
                fbe_drive_mgmt_fuel_gauge_parse_log_page31(drive_mgmt);
                break;
                
            default:
                break;
        } 
      
        fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s PoH: %d hrs  CurrecntPEcycles: %d.\n",
                              __FUNCTION__, (int)fuel_gauge_info->power_on_hours, (int)fuel_gauge_info->current_PE_cycles ); 
        return;
    }
    
    fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s Drive type doesn't support PE WARN. \n",
                          __FUNCTION__); 
 
    return; 
}
          
/*************************************************************
 * end fbe_drive_mgmt_fuel_gauge_get_PE_cycle_warrantee_info() 
 ************************************************************/

/*!************************************************************************
 *      fbe_drive_mgmt_fuel_gauge_check_EOL_warrantee()
 **************************************************************************
 *
 *  @brief
 *    This function check the max. PE cycle warrantee limit (End of life) for SSD drives with vendor 
 *    (Samsung, Hitachi, Micron, or  STEC). 
 *    A information event log will be triger if the PE cycle warrantee limit < 6 months and > 3 months.
 *    A warning event log will be triger if the PE cycle warrantee limit < 3 months and > 1 month.
 *    A error event log will be triger if the PE cycle warrantee limit < 1 month.
 *
 *  @param
 *    drive_mgmt - pointer to drive mgmt structure.
 * 
 *  @return
 *     NULL.
 *
 *  @author
 *     09/14/2012 - Created.  Chianc1
 *
 **************************************************************************/
void 
fbe_drive_mgmt_fuel_gauge_check_EOL_warrantee(fbe_drive_mgmt_t *drive_mgmt)
{
    fbe_drive_mgmt_fuel_gauge_info_t  * fuel_gauge_info = &drive_mgmt->fuel_gauge_info;   
    fbe_s64_t                         days_to_EOL_warrantee;
    fbe_s64_t                         hours_to_EOL_warrantee;
    fbe_u16_t                         i;
    fbe_bool_t                        got_eol = FBE_FALSE;
    fbe_status_t                      status = FBE_STATUS_GENERIC_FAILURE;
    char deviceStr[FBE_DEVICE_STRING_LENGTH] = {0};
   
    /* Check if denominators is 0 before calculate the ELO warrantee. */
    if ((fuel_gauge_info->current_PE_cycles == 0) || (fuel_gauge_info->power_on_hours == 0))
    {
        fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Current PE cycle or PoH is 0. Can't calculate EOL WARN.\n",
                              __FUNCTION__); 
        return;
    }
    
    /* The EOL PE cycle count has not set yet, get the default value from the table. */
    if (fuel_gauge_info->EOL_PE_cycles == 0)
    {
        for (i = 0; i < DMO_FUEL_GAUGE_MAX_PE_CYCLES_TBL_COUNT; i++) { 
        
            /* Find the max. PE cycles and calculate the EOL warrantee limits. */ 
            if (fbe_equal_memory(fuel_gauge_info->current_vendor, dmo_max_PE_cycles[i].vendor_id, sizeof (dmo_max_PE_cycles[i].vendor_id)))
            {   
                fuel_gauge_info->EOL_PE_cycles = dmo_max_PE_cycles[i].max_PE_cycles;
                got_eol = FBE_TRUE;
                fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s Get the default EOL PE cycle count %lld. \n",
                                      __FUNCTION__, fuel_gauge_info->EOL_PE_cycles );
                break;
              
            }
        }
    }   
    else
    {
        got_eol = FBE_TRUE;
    }
    
    /* check if we get Max. PE cycle. */
    if (!got_eol)
    {
        fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s Can't find the Max. PE Cycles. \n",
                              __FUNCTION__); 
        return;
    }
    
    /* Due to the Production ESS process, we will start calculating the EOL WARN after the current PE cycles count reaches the half of the max PE cycle count. */
    if (fuel_gauge_info->current_PE_cycles > (fuel_gauge_info->EOL_PE_cycles /2))
    {
        hours_to_EOL_warrantee = ((((fbe_s64_t)fuel_gauge_info->EOL_PE_cycles - (fbe_s64_t)fuel_gauge_info->current_PE_cycles) * (fbe_s64_t)fuel_gauge_info->power_on_hours)/ (fbe_s64_t)fuel_gauge_info->current_PE_cycles);
        days_to_EOL_warrantee = hours_to_EOL_warrantee/24;

        fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s EOL_WARN:%lld hrs EOL_WARN: %d days.\n",
                          __FUNCTION__, hours_to_EOL_warrantee, (int)days_to_EOL_warrantee ); 
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s Current PE cycyle %lld not reach its half of drive life yet.\n",
                          __FUNCTION__, fuel_gauge_info->current_PE_cycles );
        return;
    }

    /* generate default the device prefix string (used for tracing & events) */
    status = fbe_base_env_create_device_string(FBE_DEVICE_TYPE_DRIVE,        
             &(fuel_gauge_info->current_location), 
             &deviceStr[0],
             FBE_DEVICE_STRING_LENGTH);
    
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "DMO: %s fbe_base_env_create_device_string failed, state=FAIL, status: 0x%X\n", __FUNCTION__, status);
    }
    
    /* Send the Event Log for different PE Cycle Warrantee Limit. */
    if ((days_to_EOL_warrantee >= 90) && (days_to_EOL_warrantee < 180))  
    {
          fbe_event_log_write(ESP_INFO_DRIVE_PREDICTED_PE_CYCLE_WARRANTEE_EXPIRATION, NULL, 0, "%s %lld",
                              &deviceStr[0], days_to_EOL_warrantee );
          return;
    }
    
    if ((days_to_EOL_warrantee >= 30) && (days_to_EOL_warrantee < 90))  
    {
        fbe_event_log_write(ESP_WARN_DRIVE_PREDICTED_PE_CYCLE_WARRANTEE_EXPIRATION, NULL, 0, "%s %lld",
                            &deviceStr[0], days_to_EOL_warrantee );
        return;
    }
    
    if (days_to_EOL_warrantee < 30)  
    {
        fbe_event_log_write(ESP_ERROR_DRIVE_PREDICTED_PE_CYCLE_WARRANTEE_EXPIRATION, NULL, 0, "%s %lld",
                            &deviceStr[0], days_to_EOL_warrantee );
    }
    
    return; 
}
/*************************************************************
 * end fbe_drive_mgmt_fuel_gauge_check_EOL_warrantee() 
 ************************************************************/

/*!************************************************************************
 *      fbe_drive_mgmt_fuel_gauge_get_Emulex_P30_attribute()
 **************************************************************************
 *
 *  @brief
 *    This function get the value of the required attribute from Emulex SMART data (log page 0x30).
 *
 *  @param
 *    byte_read_ptr - pointer to read buffer.
 *    attribute_id  - a required attribute.
 * 
 *  @return
 *     The value of attribute is required.
 *
 *  @author
 *     09/14/2012 - Created.  Chianc1
 *
 **************************************************************************/
static fbe_u64_t 
fbe_drive_mgmt_fuel_gauge_get_Emulex_P30_attribute(fbe_u8_t *byte_read_ptr, fbe_u16_t attribute_id)
{
    fbe_u8_t *tmp_byte_read_ptr = 0;
    fbe_u64_t ret_value = 0; 
    fbe_u16_t i;
    
    tmp_byte_read_ptr = byte_read_ptr;
    tmp_byte_read_ptr += 6; /* Move to the first SMART attribute data. */
    
    for (i = 0; i < 361; i += 12)
    {    
        if ( *tmp_byte_read_ptr == attribute_id) 
        {   
            ret_value = fbe_drive_mgmt_cvt_value_to_ull( &tmp_byte_read_ptr[5], FBE_TRUE, 6);
            break;
        }
        tmp_byte_read_ptr += 12;
    }
    
    return ret_value;
}
/******************************************************
 * end fbe_drive_mgmt_fuel_gauge_get_Emulex_P30_attribute() 
 ******************************************************/
/*!************************************************************************
 *      fbe_drive_mgmt_fuel_gauge_parse_log_page31()
 **************************************************************************
 *
 *  @brief
 *    Get the following parameter codes from log page 0x31:
 *        0x8000 - the Worst PE Cycle Count/the worse case wearleveling count
 *        0x8ffe - the Power on hour at parameter code 
 *        0x8fff - EOL PE Cycle count/NAND Endurance  
 *  @param
 *    drive_mgmt - pointer to drive mgmt structure.
 * 
 *  @return
 *     NULL.
 *
 *  @author
 *     11/01/2013 - Created.  Chianc1
 *
 **************************************************************************/
static void
fbe_drive_mgmt_fuel_gauge_parse_log_page31(fbe_drive_mgmt_t *drive_mgmt)
{
    fbe_drive_mgmt_fuel_gauge_info_t        * fuel_gauge_info = &drive_mgmt->fuel_gauge_info;   
    fbe_u8_t                                *byte_read_ptr = fuel_gauge_info->read_buffer;
    fbe_u32_t   i;
    fbe_u32_t   page_length;
 
    fbe_drive_mgmt_fuel_gauge_log_parm_header_t *log_param_header;
    
    /* Read from buffer_read and write to buffer_write. */
    page_length = fuel_gauge_info->fuel_gauge_op->get_log_page.transfer_count;
    
    /* offset of first log parameter */
    i = 4;
  
    /* parse each log parameter out of the buffer */
    while( (i < page_length) && (i < DMO_FUEL_GAUGE_READ_BUFFER_LENGTH) )
    {
        log_param_header = (fbe_drive_mgmt_fuel_gauge_log_parm_header_t *) &byte_read_ptr[i];
        log_param_header->param_code = swap16(log_param_header->param_code);
        
        if( log_param_header->param_code == 0x8000)                 
        {
            /* parameter 8000 : Worst PE Cycle Count/the worse case wearleveling count */
            fuel_gauge_info->current_PE_cycles = fbe_drive_mgmt_cvt_value_to_ull( &byte_read_ptr[i+4], FBE_FALSE, log_param_header->length);
                                                              
            fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                                  FBE_TRACE_LEVEL_DEBUG_HIGH,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  " Parameter code:%04xh %02x Value: 0x%012llxh \n",
                                  log_param_header->param_code, log_param_header->length, fuel_gauge_info->current_PE_cycles);  
       }
        else if( log_param_header->param_code == 0x8ffe) 
        {
            /* parameter 0x8ffe : Power on Hours */
            fuel_gauge_info->power_on_hours = (fbe_u32_t) ((byte_read_ptr[i+4] << 24) | 
                                                           (byte_read_ptr[i+5] << 16) |
                                                           (byte_read_ptr[i+6] << 8) |
                                                           (byte_read_ptr[i+7]));
            fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                                  FBE_TRACE_LEVEL_DEBUG_HIGH,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  " Parameter code:%04xh %02x Value: 0x%012llxh \n",
                                  log_param_header->param_code, log_param_header->length, fuel_gauge_info->power_on_hours);  
        }
        else if( log_param_header->param_code == 0x8fff)
        {
            /* parameter 0x8fff : NAND Endurance/ EOL PE Cycle count */
            fuel_gauge_info->EOL_PE_cycles = (fbe_u32_t) ((byte_read_ptr[i+4] << 24) | 
                                                              (byte_read_ptr[i+5] << 16) |
                                                              (byte_read_ptr[i+6] << 8) |
                                                              (byte_read_ptr[i+7]));
            
            fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                                  FBE_TRACE_LEVEL_DEBUG_HIGH,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  " Parameter code:%04xh %02x Value: 0x%012llxh \n",
                                  log_param_header->param_code, log_param_header->length, fuel_gauge_info->EOL_PE_cycles);  
        }
        else
        {
            /* Unknow parameter code. */
            fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                                  FBE_TRACE_LEVEL_DEBUG_HIGH,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  " Parameter code:%04xh %02x Value: 0x%012llxh \n",
                                  log_param_header->param_code, log_param_header->length, fbe_drive_mgmt_cvt_value_to_ull( &byte_read_ptr[i+4], FBE_FALSE, log_param_header->length));  
        }
        
        i += log_param_header->length + 4;
        
    }
    
    fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s Current PE: %lld EOL PE: %lld POH: %lld PageLen %d\n",
                          __FUNCTION__,fuel_gauge_info->current_PE_cycles, fuel_gauge_info->EOL_PE_cycles, fuel_gauge_info->power_on_hours, page_length);  
           
    return;    
}
          
/*************************************************************
 * end fbe_drive_mgmt_fuel_gauge_parse_log_page31() 
 ************************************************************/

/*!************************************************************************
 *      fbe_drive_mgmt_get_fuel_gauge_file()
 **************************************************************************
 *
 *  @brief
 *    Use EmcutilFilePathMake to set up the file and its path for 
 *                 drive_flash_stats.txt.
 *
 *  @param
 *    location - pointer to location of file.
 * 
 *  @return
 *     None.
 *
 *  @author
 *     11/05/2012 - Created.  Chianc1
 *
 **************************************************************************/
static void fbe_drive_mgmt_get_fuel_gauge_file(fbe_char_t *location)
{
    fbe_char_t filename[64];

    EmcutilFilePathMake(filename, sizeof(filename), EMCUTIL_BASE_DUMPS,
                       "drive_flash_stats.txt", NULL);
    strncpy(location, filename, sizeof(filename));
    
    location[sizeof(filename)] = '\0';
    
    return;
}
/*************************************************
 * end fbe_drive_mgmt_get_fuel_gauge_file() 
 *************************************************/


/*!************************************************************************
 *      fbe_drive_mgmt_get_fuel_gauge_file_most_recent()
 **************************************************************************
 *
 *  @brief
 *    Returns filename which captures just the most recent collect for
 *    drive stats (Fuel Gauge, BMS, etc).
 *
 *  @param
 *    location - pointer to location of file.
 * 
 *  @return
 *     None.
 *
 *  @author
 *     09/14/2015 - Created.  Wayne Garrett
 *
 **************************************************************************/
static void fbe_drive_mgmt_get_fuel_gauge_file_most_recent(fbe_char_t *location)
{
    csx_char_t filename[64];

#if defined(UMODE_ENV) || defined(SIMMODE_ENV)
    strncpy(filename, DMO_DRIVE_STATS_MOST_RECENT_FILENAME ,sizeof(filename));
#else
    EmcutilFilePathMake(filename, sizeof(filename), EMCUTIL_BASE_DUMPS,
                       DMO_DRIVE_STATS_MOST_RECENT_FILENAME, NULL);
#endif 
    strncpy(location, filename, sizeof(filename));

    location[sizeof(filename)] = '\0';

    return;
}


/*!************************************************************************
 *      fbe_drive_mgmt_fuel_gauge_parse_bms_data()
 **************************************************************************
 *
 *  @brief
 *    Parse Log Page 15 data and save results in bms_results for all drive types.
 *
 *  @param
 *    drive_mgmt - pointer to drive mgmt structure.
 * 
 *  @return
 *     NULL.
 *
 *  @author
 *     09/14/2012 - Created.  Chianc1
 *
 **************************************************************************/
static void
fbe_drive_mgmt_fuel_gauge_parse_bms_data(fbe_drive_mgmt_t *drive_mgmt, fbe_dmo_fuel_gauge_bms_t *bms_results)
{
    fbe_drive_mgmt_fuel_gauge_info_t        * fuel_gauge_info = &drive_mgmt->fuel_gauge_info;   
    fbe_u8_t                                *byte_read_ptr = fuel_gauge_info->read_buffer;
    fbe_u32_t   page_length;
    fbe_u32_t   i;
    fbe_u32_t   total_iraw = 0;
    fbe_u32_t   total_bms_per_head[16]; /* head is 4-bit field. */
    fbe_u32_t   min_timestamp = 0xFFFFFFFF;
    fbe_u32_t   max_timestamp = 0;
    fbe_dmo_fuel_gauge_log_param_bg_scan_vs_SEAGATE_t vs_test_zero;
    fbe_dmo_fuel_gauge_log_param_bg_scan_vs_SEAGATE_t vs_test_ff;
    fbe_dmo_fuel_gauge_log_param_bg_scan_param_t *log_param;
    fbe_lba_t * failed_lba_01_buff;  /* Failed lba for BMS 01/xx/xx. */
    fbe_lba_t * failed_lba_03_buff;  /* Failed lba for BMS 03/xx/xx. */
    fbe_u16_t   add_vaule = 0;
    fbe_u8_t    DOS_flag = 0;
    fbe_u16_t   DOS_count = 0;
 
    
    /* Allocate failed lba 01/xx buffer and zero memory. */
    failed_lba_01_buff = (fbe_lba_t *)fbe_allocate_nonpaged_pool_with_tag(DMO_FUEL_GAUGE_FAILED_LBA_BUFFER_LENGTH, 'pfbe');
    if (failed_lba_01_buff == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)drive_mgmt, 
                              FBE_TRACE_LEVEL_WARNING,  
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Could not allocate failed lba 01/xx buffer.\n", __FUNCTION__);
    }
    
    /* Allocate failed lba 03/xx buffer and zero memory. */
    failed_lba_03_buff = (fbe_lba_t *)fbe_allocate_nonpaged_pool_with_tag(DMO_FUEL_GAUGE_FAILED_LBA_BUFFER_LENGTH, 'pfbe');
    if (failed_lba_03_buff == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)drive_mgmt, 
                              FBE_TRACE_LEVEL_WARNING,  
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Could not allocate failed lba 03/xx buffer.\n", __FUNCTION__);
    }
    
    fbe_zero_memory(bms_results, sizeof (fbe_dmo_fuel_gauge_bms_t));
    fbe_zero_memory(&total_bms_per_head[0], sizeof (total_bms_per_head));
    fbe_zero_memory(&vs_test_zero, sizeof (vs_test_zero));
    fbe_set_memory(&vs_test_ff, 0xFF, sizeof (vs_test_ff));

    /* Read from buffer_read and write to buffer_write. */
    page_length = fuel_gauge_info->fuel_gauge_op->get_log_page.transfer_count;
    
    /* offset of first log parameter */
    i = 4;
  
    /* parse each log parameter out of the buffer */
    while( (i < page_length) && (i < DMO_FUEL_GAUGE_READ_BUFFER_LENGTH) )
    {
        log_param = (fbe_dmo_fuel_gauge_log_param_bg_scan_param_t *) &byte_read_ptr[i];
        log_param->scan.param_code = swap16(log_param->scan.param_code);
        if( log_param->scan_status.param_code == 0 &&
                log_param->scan_status.length == sizeof(log_param->scan_status) - 4 )
        {
            /* parameter 0000 : Scan Status */
    
            /* current power on minutes */
             bms_results->drive_power_on_minutes = swap32(log_param->scan_status.power_on_minutes);
                  
            /* number of BMS scans drive has performed */
            bms_results->num_bg_scans_performed = swap16(log_param->scan_status.num_bg_scans_performed);
    
            i += log_param->scan_status.length + 4;
            
        }
        else if( (log_param->scan.param_code >= 0x001) &&
                 (log_param->scan.param_code <= 0x800) &&
                 (log_param->scan.length == (sizeof(log_param->scan) - 4)) )
        {
            /* parameter 0001 to 08000 : Scan Results */
            
            /* For the Seagate drive and DOS_flag is not zero, then this entry is NOT created by BMS */
            DOS_flag = (fbe_u8_t)((log_param->scan.vs.seagate.asi_sector_high >> 6) & 0x03);
            if ((DOS_flag) && (fbe_equal_memory(fuel_gauge_info->current_vendor, "SEAGATE", 7) ))
            { 
                DOS_count++;
                fbe_base_object_trace((fbe_base_object_t*)drive_mgmt, 
                                      FBE_TRACE_LEVEL_DEBUG_HIGH,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                      "%s DOS entry DOS=%x Parameter code:%04xh sn#=%s\n", __FUNCTION__, DOS_flag, log_param->scan.param_code, fuel_gauge_info->current_sn); 
                i += log_param->scan_status.length + 4;
                continue;
            }
            
            bms_results->total_entries++;
            if( ( log_param->scan.reassign_status_sense_key & DMO_SCSI_SENSE_KEY_MASK ) == DMO_SCSI_SENSE_KEY_MEDIUM_ERROR )
            {
                /* medium error */
                if (failed_lba_03_buff != NULL)
                {
                    failed_lba_03_buff[bms_results->total_03_errors] = log_param->scan.lba;
                    add_vaule = fbe_drive_mgmt_find_duplicate_bms_lba(&failed_lba_03_buff[0], log_param->scan.lba, bms_results->total_03_errors);
                    bms_results->total_unique_03_errors += add_vaule; /* Only increment if there is no duplicate lba. */
                }
                bms_results->total_03_errors++;
                                
                /* TODO in future some time if it is safe. */
                /* If there are new 03 unrecovered blocks, mark them in our suspect
                 * block list so that they are rebuilt and written back to disk. */
                /* add LBA to suspect block list */
                /* lba = BETOH64(log_param->scan.lba); */
            }
    
            if( ( log_param->scan.reassign_status_sense_key & DMO_SCSI_SENSE_KEY_MASK ) == DMO_SCSI_SENSE_KEY_RECOVERED_ERROR )
            {
                /* recovered error */
                if (failed_lba_01_buff != NULL)
                {
                    failed_lba_01_buff[bms_results->total_01_errors] = log_param->scan.lba;
                    add_vaule = fbe_drive_mgmt_find_duplicate_bms_lba(&failed_lba_01_buff[0], log_param->scan.lba, bms_results->total_01_errors);
                    bms_results->total_unique_01_errors += add_vaule; /* Only increment if there is no duplicate lba. */
                }
                bms_results->total_01_errors++;
            }
    
            /* find min and max timestamps of all entries */
            log_param->scan.power_on_minutes = swap32(log_param->scan.power_on_minutes);
            if( log_param->scan.power_on_minutes < min_timestamp ) { min_timestamp = log_param->scan.power_on_minutes; }
            if( log_param->scan.power_on_minutes > max_timestamp ) { max_timestamp = log_param->scan.power_on_minutes; }
    
            /* check if vendor specific is valid */
            if( !fbe_equal_memory( &log_param->scan.vs.seagate, &vs_test_zero, sizeof(vs_test_zero) ) &&
                    !fbe_equal_memory( &log_param->scan.vs.seagate, &vs_test_ff,   sizeof(vs_test_ff) ))
            {
                /* incrememnt counter for head with the error */
                total_bms_per_head[ (log_param->scan.vs.seagate.head_cyl_high >> 4) & 0x0F ]++;
            }
    
            i += log_param->scan_status.length + 4;
        }
        else if( log_param->iraw.param_code >= 0x8100 &&
                 log_param->iraw.param_code <= 0x8132 &&
                 log_param->iraw.length == sizeof(log_param->iraw) - 4 )
        {
            /* vendor specific IRAW parameters  */
            total_iraw++;
            i += log_param->scan_status.length + 4;
        }
        else
        {
            /* unknown parameter */
            i += byte_read_ptr[i + 3] + 4;
        }
    }
    
    fbe_base_object_trace((fbe_base_object_t*)drive_mgmt, 
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s DOS entries:%d \n", __FUNCTION__, DOS_count); 
    /* release failed lba buffers. */
    if (failed_lba_01_buff != NULL)
    {
        fbe_release_nonpaged_pool_with_tag(failed_lba_01_buff,'pfbe');
        failed_lba_01_buff = NULL;
    }
    
    if (failed_lba_03_buff != NULL)
    {
        fbe_release_nonpaged_pool_with_tag(failed_lba_03_buff,'pfbe');
        failed_lba_03_buff = NULL;
    }
    
    /* save total iraw */
    bms_results->total_iraw = ( total_iraw > 0xFF ? 0xFF : total_iraw );
    
    /* calculate time range of all entries */
    if( bms_results->total_entries > 0 )
    {
        bms_results->time_range = max_timestamp - min_timestamp;  /*TODO: AR 637098 - range should be days, not minutes. */
    }
    
    /* Figure out which head is the worst */
    bms_results->worst_head = 0;
    for( i = 0; i < 16; i++ )
    {
        if( total_bms_per_head[i] > total_bms_per_head[ bms_results->worst_head ] )
        {
            bms_results->worst_head = i;
        }
    }
    
    bms_results->worst_head_entries = total_bms_per_head[ bms_results->worst_head ];
    
    /* Set timestamp of when we parsed this data */
    fbe_drive_mgmt_fuel_gauge_get_timestamp(&(bms_results->time_stamp));
 
    fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s Completed pasre BMS data. \n",
                          __FUNCTION__); 
       
    return;    
}
          
/*************************************************************
 * end fbe_drive_mgmt_fuel_gauge_parse_bms_data() 
 ************************************************************/

/*!************************************************************************
 *      fbe_drive_mgmt_fuel_gauge_check_BMS_alerts()
 **************************************************************************
 *
 *  @brief
 *    This function check the BMS entries, unique 03/xx entries, and Lambda_BB 
 *    A information event log will be triger for every BMS log.
 *    A information event log will be triger if the unique 03/xx BMS entries >=10.
 *    A warning event log will be triger if the BMS entries >= 1000.
 *    A error event log will be triger if the BMS entries >= 2048.
 *    A error event log will be triger if the Lambda_BB >= 30.
 *
 *  @param
 *    drive_mgmt - pointer to drive mgmt structure.
 *    bms_results - pointer to the bms results.
 *
 *  @return
 *     NULL.
 *
 *  @author
 *     04/25/2013 - Created.  Chianc1
 *
 **************************************************************************/
void 
fbe_drive_mgmt_fuel_gauge_check_BMS_alerts(fbe_drive_mgmt_t *drive_mgmt, fbe_dmo_fuel_gauge_bms_t *bms_results)
{
    fbe_drive_mgmt_fuel_gauge_info_t  * fuel_gauge_info = &drive_mgmt->fuel_gauge_info;   
    fbe_u16_t                         lambda_BB = 0;
    char deviceStr[FBE_DEVICE_STRING_LENGTH] = {0};
    fbe_status_t                      status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t                         time_range_in_days;
    
    /* generate default the device prefix string (used for tracing & events) */
    status = fbe_base_env_create_device_string(FBE_DEVICE_TYPE_DRIVE,        
             &(fuel_gauge_info->current_location), 
             &deviceStr[0],
             FBE_DEVICE_STRING_LENGTH);
    
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "DMO: %s fbe_base_env_create_device_string failed, state=FAIL, status: 0x%X\n", __FUNCTION__, status);
    }

    fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "DMO: %d_%d_%d BMS_alerts. SN:%s found %d entries for head: %d. Total: %d\n",
                          fuel_gauge_info->current_location.bus, fuel_gauge_info->current_location.enclosure, fuel_gauge_info->current_location.slot, 
                          fuel_gauge_info->current_sn, bms_results->worst_head_entries, bms_results->worst_head, bms_results->total_entries); 
    
    /* Log an eventlog for every BMS scan */
    fbe_event_log_write(ESP_INFO_DRIVE_BMS_FOR_WORST_HEAD, NULL, 0, "%s %s %d %d %d",
                        &deviceStr[0], fuel_gauge_info->current_sn, bms_results->worst_head_entries, bms_results->worst_head, bms_results->total_entries);
    
    /* Handle the worst head. */
    /* In r33 SP3 this was changed from 1000 to 1500 to match the value that Symm uses */
    if ( bms_results->worst_head_entries >= 1500)
    {
        fbe_event_log_write(ESP_ERROR_DRIVE_BMS_FOR_WORST_HEAD_OVER_WARN_THRESHOLD, NULL, 0, "%s %s %d %d %d",
                            &deviceStr[0], fuel_gauge_info->current_sn, bms_results->worst_head_entries, bms_results->worst_head, bms_results->total_entries);
    }
    
    /* Handle total BMS entries. */
    if ( bms_results->total_entries >= 2048)
    {
        fbe_event_log_write(ESP_ERROR_DRIVE_BMS_OVER_MAX_ENTRIES, NULL, 0, "%s %s %d",
                            &deviceStr[0], fuel_gauge_info->current_sn, bms_results->total_entries);
    }
    else if ( bms_results->total_entries >= 1500)
    {
        fbe_event_log_write(ESP_WARN_DRIVE_BMS_OVER_WARN_THRESHOLD, NULL, 0, "%s %s %d %d %d",
                            &deviceStr[0], fuel_gauge_info->current_sn, bms_results->worst_head_entries, bms_results->worst_head, bms_results->total_entries);
        fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "DMO: %d_%d_%d BMS entries >= 1500. SN:%s Worst Head %d & Entries:%d  Total:%d\n",
                              fuel_gauge_info->current_location.bus, fuel_gauge_info->current_location.enclosure, fuel_gauge_info->current_location.slot,
                              fuel_gauge_info->current_sn,bms_results->worst_head, bms_results->worst_head_entries, bms_results->total_entries); 
    }
    
    /* Handle Lambda_BB.*/
    if (bms_results->total_unique_03_errors >= 10)   /*TODO: make this a define and add ref to spec*/
    {
		
        /* Get the BMS time_range in days. */
        time_range_in_days = bms_results->time_range/(60*24);
		
        lambda_BB = ((bms_results->total_unique_03_errors)*1000)/(time_range_in_days + 1);  /*time_range+1 to avoid div by zero*/
		
        fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                              "DMO: %d_%d_%d check_BMS_alerts. SN:%s LBB:%d 03s:%d Time range in %d minutes = %d days\n",
                              fuel_gauge_info->current_location.bus, fuel_gauge_info->current_location.enclosure, fuel_gauge_info->current_location.slot,
                              fuel_gauge_info->current_sn, lambda_BB, bms_results->total_unique_03_errors, bms_results->time_range, time_range_in_days + 1); 
        
        if (lambda_BB >=30)   /*TODO: make this a define and add ref to spec*/
        {
            /* AR:637098 - removing call home since Lambda_BB formula is using minutes instead of days for range
               AR#650186 - fixed the Lambda_BB formula to using the time range in days. Added a ktrace but still keep this call home removed.
              
            fbe_event_log_write(ESP_ERROR_DRIVE_BMS_LBB_OVER_THRESHOLD, NULL, 0, FBE_WIDE_CHAR("%S %S %d"),
                                &deviceStr[0], fuel_gauge_info->current_sn, lambda_BB ); 
            */ 

            fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "DMO: %d_%d_%d Lamda_BB >= 30. SN:%s LBB:%d 03s:%d Time range in %d minutes = %d days\n",
                                  fuel_gauge_info->current_location.bus, fuel_gauge_info->current_location.enclosure, fuel_gauge_info->current_location.slot,
                                  fuel_gauge_info->current_sn, lambda_BB, bms_results->total_unique_03_errors, bms_results->time_range, time_range_in_days + 1); 
        }
    }
        
   return; 
}
/*************************************************************
 * end fbe_drive_mgmt_fuel_gauge_check_BMS_alerts() 
 ************************************************************/
