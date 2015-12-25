/**********************************************************************
 *  Copyright (C)  EMC Corporation 2008
 *  All rights reserved.
 *  Licensed material - property of EMC Corporation.
 **********************************************************************/

/*!*************************************************************************
 * @file fbe_sas_physical_drive_utils.c
 ***************************************************************************
 *
 *  The routines in this file contains all the utility functions used by the 
 *  SAS physical drive object.
 *
 * NOTE: 
 *
 * HISTORY
 *   11-Nov-2008 chenl6 - Created. 
 *   01-Jul-2014 Darren Insko - Added Read Look Ahead (RLA) workaround.  AR 653188.
 *
 ***************************************************************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_memory.h"
#include "fbe/fbe_transport.h"

#include "fbe_transport_memory.h"
#include "fbe_topology.h"
#include "fbe_scheduler.h"
#include "fbe_traffic_trace.h"

#include "sas_physical_drive_private.h"
#include "base_physical_drive_init.h"
#include "sas_physical_drive_config.h"
#include "swap_exports.h"

/* Standard includes */
#include <stdlib.h>
#include <stdio.h>
//#include <process.h>
#include <string.h>

/* Used to determine Seagate drives queue aging setting */
//fbe_u32_t fbe_seagate_queue_time = FBE_PG0A_QUEUE_DEFAULT;

/* Used to turn Segate drives smart handling off */
fbe_bool_t fbe_smart_delay_off = FBE_FALSE;

/* Used to enable and disable for beachcomber platform*/
static fbe_bool_t is_beachcomber_4k_enabled = FBE_TRUE;


/***********************
  *     PROTOTYPES     *
  **********************/
static fbe_sas_drive_status_t sas_pdo_validate_capacity(fbe_sas_physical_drive_t * sas_physical_drive, fbe_block_size_t  block_size);
static fbe_sas_drive_status_t fbe_sas_physical_drive_validate_tla_suffix(fbe_u8_t * tla_suffix);
static fbe_sas_drive_status_t fbe_sas_physical_drive_fix_ms_table(fbe_vendor_page_t * exp_ptr, fbe_u16_t max, fbe_u8_t page, fbe_u8_t offset, fbe_u8_t value);
static fbe_status_t fbe_sas_physical_drive_fix_table_for_enhanced_queuing(fbe_sas_physical_drive_t* sas_physical_drive, fbe_vendor_page_t *exp_ptr, fbe_u16_t max);



fbe_sas_drive_status_t
fbe_sas_physical_drive_process_read_capacity_10(fbe_sas_physical_drive_t * sas_physical_drive, fbe_sg_element_t * sg_list)
{
    fbe_u8_t * read_capacity_data = NULL;
    fbe_lba_t block_count;
    fbe_block_size_t  block_size;
    fbe_lba_t max_lba;
    fbe_sas_drive_status_t drive_status;

    read_capacity_data = (fbe_u8_t *)sg_list[0].address;

    block_count = 0;
    block_count |= (fbe_lba_t)(read_capacity_data[0] & 0xFF) << 24;
    block_count |= (fbe_lba_t)(read_capacity_data[1] & 0xFF) << 16;
    block_count |= (fbe_lba_t)(read_capacity_data[2] & 0xFF) << 8;
    block_count |= (fbe_lba_t)(read_capacity_data[3] & 0xFF);
    max_lba = block_count;
    block_count += 1;

    block_size = 0;
    block_size |= (fbe_block_size_t)(read_capacity_data[6] & 0xFF) << 8;
    block_size |= (fbe_block_size_t)(read_capacity_data[7] & 0xFF);
    
    /* Check for zero size lba count and block size */
    if ((max_lba == 0) || (block_size == 0))
    {
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                FBE_TRACE_LEVEL_WARNING,
                                "%s Invalid zero block_size: 0x%x, max_lba: 0x%x\n",
                                __FUNCTION__, block_size, (unsigned int)max_lba);
        
        return FBE_SAS_DRIVE_STATUS_NEED_RETRY;
    }

    
    block_count = max_lba + 1;
    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                        FBE_TRACE_LEVEL_DEBUG_LOW,
                        "%s block_count = %llu , block_size = %d\n",
			__FUNCTION__, (unsigned long long)block_count,
			block_size);

    drive_status = sas_pdo_validate_capacity(sas_physical_drive, block_size);
    if (drive_status != FBE_SAS_DRIVE_STATUS_OK)
    {
        return drive_status;
    }

    fbe_base_physical_drive_set_capacity((fbe_base_physical_drive_t *) sas_physical_drive, block_count, block_size);

    return FBE_SAS_DRIVE_STATUS_OK;
}
   
fbe_sas_drive_status_t
fbe_sas_physical_drive_process_read_capacity_16(fbe_sas_physical_drive_t * sas_physical_drive, fbe_sg_element_t * sg_list)
{
    fbe_u8_t * buff_ptr = NULL;
    fbe_u8_t * temp_ptr = NULL;
    fbe_lba_t block_count = 0;
    fbe_block_size_t  block_size = 0;
    fbe_lba_t max_lba = 0;
    fbe_sas_drive_status_t drive_status;
     
    buff_ptr = (fbe_u8_t *)sg_list[0].address;
    if (sg_list[0].count < FBE_SCSI_READ_CAPACITY_DATA_SIZE_16)
    {
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                "%s Invalid read capacity data size: 0x%x\n",
                                __FUNCTION__, sg_list[0].count);

        return FBE_SAS_DRIVE_STATUS_INVALID;
    }

    /* Find the Logical block length in bytes */
    temp_ptr = buff_ptr + 8;
    block_size = swap32(*((fbe_u32_t *)temp_ptr));

    /* Success -- the disk is formatted correctly.
     * Extract the read capacity out of the buffer.
     * What ever the number is, we assume this is the correct capacity.
     */
    max_lba = swap64(*((fbe_u64_t *)buff_ptr));

    block_count = max_lba +1;


    drive_status = sas_pdo_validate_capacity(sas_physical_drive, block_size);
    if (drive_status != FBE_SAS_DRIVE_STATUS_OK)
    {
        return drive_status;
    }

    fbe_base_physical_drive_set_capacity((fbe_base_physical_drive_t *)sas_physical_drive, block_count, block_size);

    return FBE_SAS_DRIVE_STATUS_OK;
}


/*!****************************************************************
 * sas_pdo_validate_capacity()
 ******************************************************************
 * @brief
 *  This function validates results from read capacity cmd.
 *
 * @param sas_physical_drive - The SAS physical drive object.
 * @param block_size - Drive's formated block size.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  01/20/2014  Wayne Garrett  - Created.
 *
 ****************************************************************/
static fbe_sas_drive_status_t sas_pdo_validate_capacity(fbe_sas_physical_drive_t * sas_physical_drive, fbe_block_size_t  block_size)
{
    /* Only supported formats are 520 and 4160 (if enabled) 
    */
    SPID_PLATFORM_INFO platform_info = {0};
    fbe_status_t status;
    fbe_bool_t enabled_4k_platform = FBE_TRUE;

    status = fbe_base_physical_drive_get_platform_info((fbe_base_physical_drive_t*)sas_physical_drive, &platform_info);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, FBE_TRACE_LEVEL_ERROR,
                                           "%s Unable to get SPID\n", __FUNCTION__);
    }

    if (platform_info.cpuModule == BEACHCOMBER_CPU_MODULE && !is_beachcomber_4k_enabled)
    {
        enabled_4k_platform = FBE_FALSE;
    }

    if (block_size == FBE_BLOCK_SIZES_520  ||
        (block_size == FBE_BLOCK_SIZES_4160 && fbe_dcs_param_is_enabled(FBE_DCS_4K_ENABLED) && enabled_4k_platform))
    {
        return FBE_SAS_DRIVE_STATUS_OK;
    }

    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                               FBE_TRACE_LEVEL_WARNING,
                                               "%s Invalid BPS format.  block size:%d\n",
                                               __FUNCTION__, block_size);
    
    return FBE_SAS_DRIVE_STATUS_INVALID;
}

/*!****************************************************************
 * fbe_sas_physical_drive_validate_tla_suffix()
 ******************************************************************
 * @brief
 *  This function validates TLA suffix in the inquiry.
 *  If any of the characters in suffix are in the a-z,A-Z range
 *  Then, we check if the suffix is known to us. 
 *
 * @param tla_suffix - Pointer to TLA suffix.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  06/10/2015  Saranyadevi Ganesan  - Created.
 *
 ****************************************************************/
static fbe_sas_drive_status_t fbe_sas_physical_drive_validate_tla_suffix(fbe_u8_t *tla_suffix)
{
    fbe_u32_t i = 0;

    if (!(fbe_equal_memory(tla_suffix, FBE_SPINDOWN_SUPPORTED_STRING, FBE_SPINDOWN_STRING_LENGTH)||
        fbe_equal_memory(tla_suffix, FBE_FLASH_DRIVE_STRING, FBE_SPINDOWN_STRING_LENGTH)||
        fbe_equal_memory(tla_suffix, FBE_MLC_FLASH_DRIVE_STRING, FBE_SPINDOWN_STRING_LENGTH)))
    {
        for (i=0; i<FBE_SPINDOWN_STRING_LENGTH; i++)
        {
            if((*tla_suffix >= 'a' && *tla_suffix <= 'z') || (*tla_suffix >= 'A' && *tla_suffix <= 'Z'))
            {
                return FBE_SAS_DRIVE_STATUS_INVALID;
            }
            tla_suffix++;
        }
    }
    return FBE_SAS_DRIVE_STATUS_OK; 
}

fbe_sas_drive_status_t 
fbe_sas_physical_drive_set_dev_info(fbe_sas_physical_drive_t * sas_physical_drive, 
                                    fbe_u8_t *inquiry,
                                    fbe_physical_drive_information_t * info_ptr)
{
    fbe_u32_t i = 0;
    fbe_u64_t disk_size = 0;
    fbe_sas_physical_drive_price_class_map_data_t *sas_drive_price_class_map_data;
    fbe_u32_t num_rla_fixed_fw_revs = sizeof(sas_drive_rla_fixed_map_table)/sizeof(fbe_sas_physical_drive_rla_fixed_map_data_t);
#ifdef FBE_HANDLE_UNKNOWN_TLA_SUFFIX
    fbe_sas_drive_status_t drive_status;
    fbe_u8_t * tla_suffix_ptr = NULL;
#endif
    fbe_bool_t is_emc_string = FBE_FALSE;

    /* check first 3 bytes to insure we have a disk that support ANSI SCSI. */
    if ((inquiry[0] != 0) || ((inquiry[1] & 0x80) != 0) || ((inquiry[2]& 0x07) == 0)) {
        return FBE_SAS_DRIVE_STATUS_INVALID;
    }

    /* clear the drive parameter flags in the drive information structure */
    info_ptr->drive_parameter_flags = 0;
        
    /* get vendor id: byte 8-15 */
    fbe_zero_memory(info_ptr->vendor_id, FBE_SCSI_INQUIRY_VENDOR_ID_SIZE + 1);
    fbe_copy_memory(info_ptr->vendor_id, inquiry + FBE_SCSI_INQUIRY_VENDOR_ID_OFFSET, FBE_SCSI_INQUIRY_VENDOR_ID_SIZE); 

    if (fbe_equal_memory(info_ptr->vendor_id, "SEAGATE ", 8)) {
        info_ptr->drive_vendor_id = FBE_DRIVE_VENDOR_SEAGATE;
    } 
    else if (fbe_equal_memory(info_ptr->vendor_id, "IBM", 3)) {
        info_ptr->drive_vendor_id = FBE_DRIVE_VENDOR_IBM;
    }
    else if (fbe_equal_memory(info_ptr->vendor_id, "HITACHI", 7)) {
        info_ptr->drive_vendor_id = FBE_DRIVE_VENDOR_HITACHI;
    }
    else if (fbe_equal_memory(info_ptr->vendor_id, "SATAHGST", 8)) {
        info_ptr->drive_vendor_id = FBE_DRIVE_VENDOR_HITACHI;
    }
    else if (fbe_equal_memory(info_ptr->vendor_id, "FUJITSU", 7)) {
        info_ptr->drive_vendor_id = FBE_DRIVE_VENDOR_FUJITSU;
    }
    else if (fbe_equal_memory(info_ptr->vendor_id, "MAXTOR", 6)) {
        info_ptr->drive_vendor_id = FBE_DRIVE_VENDOR_MAXTOR;
    }
    else if (fbe_equal_memory(info_ptr->vendor_id, "EMULEX", 6)) {
        info_ptr->drive_vendor_id = FBE_DRIVE_VENDOR_EMULEX;
    } 
    else if (fbe_equal_memory(info_ptr->vendor_id, "SAMSUNG", 7)) {
        /* The Samsung RDX drive introduces both a native SAS SLC and an MLC
         *  version.  This required a new drive vendor since the existing
         *  FBE_DRIVE_VENDOR_SAMSUNG is hard coded to be a 
         *  FBE_PDO_FLAG_FLASH_DRIVE further below in this function.
         */
        info_ptr->drive_vendor_id = FBE_DRIVE_VENDOR_SAMSUNG_2;
    }
    else if (fbe_equal_memory(info_ptr->vendor_id, "SATA-SAM", 8)) {
        info_ptr->drive_vendor_id = FBE_DRIVE_VENDOR_SAMSUNG;
    } 
    else if (fbe_equal_memory(info_ptr->vendor_id, "TOSHIBA", 7)) {
        info_ptr->drive_vendor_id = FBE_DRIVE_VENDOR_TOSHIBA;
    } 
    else if (fbe_equal_memory(info_ptr->vendor_id, "GENERIC", 7)) {
        info_ptr->drive_vendor_id = FBE_DRIVE_VENDOR_GENERIC;
    } 
    else if (fbe_equal_memory(info_ptr->vendor_id, "SATA-GEN", 8)) {
        info_ptr->drive_vendor_id = FBE_DRIVE_VENDOR_GENERIC;
    }     
    else if (fbe_equal_memory(info_ptr->vendor_id, "SATA-ANB", 8)) {
        info_ptr->drive_vendor_id = FBE_DRIVE_VENDOR_ANOBIT;
    }
    else if (fbe_equal_memory(info_ptr->vendor_id, "STEC", 4)) {
        info_ptr->drive_vendor_id = FBE_DRIVE_VENDOR_STEC;
    }    
    else if (fbe_equal_memory(info_ptr->vendor_id, "SATA-MIC", 8)) {
        info_ptr->drive_vendor_id = FBE_DRIVE_VENDOR_MICRON;
    }
    else if (fbe_equal_memory(info_ptr->vendor_id, "WD", 2)) {
        info_ptr->drive_vendor_id = FBE_DRIVE_VENDOR_WESTERN_DIGITAL;
    }  
    else if (fbe_equal_memory(info_ptr->vendor_id, "SANDISK", 7)) {
        info_ptr->drive_vendor_id = FBE_DRIVE_VENDOR_SANDISK;
    }   
    else {
        info_ptr->drive_vendor_id = FBE_DRIVE_VENDOR_INVALID;
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                FBE_TRACE_LEVEL_WARNING,
                                "%s Invalid vendor_id: %s\n",
                                __FUNCTION__, info_ptr->vendor_id);

        return FBE_SAS_DRIVE_STATUS_INVALID;
    }

    /* The logging is handled in usurper. We copy the inquiry data to 
     * the fbe_base_physical_drive_info_t and validate them when is needed. 
     */

    /* get product id: byte 16-31 */
    fbe_zero_memory(info_ptr->product_id, FBE_SCSI_INQUIRY_PRODUCT_ID_SIZE + 1);
    fbe_copy_memory(info_ptr->product_id, inquiry + FBE_SCSI_INQUIRY_PRODUCT_ID_OFFSET, FBE_SCSI_INQUIRY_PRODUCT_ID_SIZE);


    /* Validate product id here. Note that we are comparing the LOW HALF of the Inquiry
     * product-id (bytes 24-31).  We add 8 to drop bytes 16 thru 23.
     */

    info_ptr->drive_price_class = FBE_DRIVE_PRICE_CLASS_UNKNOWN;
    sas_drive_price_class_map_data = sas_drive_price_class_map_table;

    /* See if we can find a match for the product id substring in the table */
    while(sas_drive_price_class_map_data->drive_price_class != FBE_DRIVE_PRICE_CLASS_LAST)
    {
        fbe_u8_t end_position;

        end_position = (fbe_u8_t) (FBE_SCSI_INQUIRY_PRODUCT_ID_SIZE - 
            strlen(sas_drive_price_class_map_data->product_id_substring));
        for(i = 8; i <= end_position; i++)
        {
            if(fbe_equal_memory(info_ptr->product_id + i, 
                                sas_drive_price_class_map_data->product_id_substring,
                                (fbe_u32_t)strlen(sas_drive_price_class_map_data->product_id_substring)))
            {
                /* We found a product id substring match in the supported list.
                 * So get the corresponding product id substring code.
                 */
                info_ptr->drive_price_class =
                    sas_drive_price_class_map_data->drive_price_class;
                break;
            }
        }
        if(info_ptr->drive_price_class != FBE_DRIVE_PRICE_CLASS_UNKNOWN) {
            break;
        }
        sas_drive_price_class_map_data ++;
    }

    if (info_ptr->drive_price_class == FBE_DRIVE_PRICE_CLASS_UNKNOWN) {
        /* A supported product id sub string is not found. */
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                FBE_TRACE_LEVEL_WARNING,
                                "%s Unsupported Drive: %s\n",

                                __FUNCTION__, info_ptr->product_id);
        return FBE_SAS_DRIVE_STATUS_INVALID;
    }
    
    /* Is this the new EMC string format?  Determining drive attributes will be done differently. */
    if(fbe_equal_memory(info_ptr->product_id + i, FBE_DRIVE_INQUIRY_EMC_STRING, (fbe_u32_t)strlen(FBE_DRIVE_INQUIRY_EMC_STRING)))
    {
        is_emc_string = FBE_TRUE;
    }


    /* Get the number after the product ID sub-string */
    disk_size = atol((TEXT *) info_ptr->product_id + i + strlen(sas_drive_price_class_map_data->product_id_substring));

    /* Note that the fbe_scsi_build_block_cdb and fbe_scsi_build_cdb arrays have been initialized
     *  to NULL in fbe_sas_physical_drive_main.c.  Here we fill the jump table with the ones that
     *  we will use. */
    if (disk_size <= 2000) { /* Less than 2TB */
		/* We should learn it from sense_buffer[FBE_SCSI_SENSE_DATA_RESPONSE_CODE_OFFSET] = 0x70 */
        info_ptr->drive_parameter_flags &= (~FBE_PDO_FLAG_USES_DESCRIPTOR_FORMAT); 
    } else {
		/* We should learn it from sense_buffer[FBE_SCSI_SENSE_DATA_RESPONSE_CODE_OFFSET] = 0x72 */
        info_ptr->drive_parameter_flags |= FBE_PDO_FLAG_USES_DESCRIPTOR_FORMAT;
    }


    /* get revision: byte 32-35 */
    /* Zero the entire string even though we are only using the first part of it here */
    fbe_zero_memory(info_ptr->product_rev, FBE_SCSI_INQUIRY_VPD_PROD_REV_SIZE + 1);
    /* Copy in the drive FW revision string */
    fbe_copy_memory(info_ptr->product_rev, inquiry + FBE_SCSI_INQUIRY_REVISION_OFFSET, FBE_SCSI_INQUIRY_REVISION_SIZE);
    /* Set the length of the drive FW revision string */
    info_ptr->product_rev_len = FBE_SCSI_INQUIRY_REVISION_SIZE;
    /* It is a string so NULL terminate it */
    info_ptr->product_rev[info_ptr->product_rev_len] = '\0';

    /* Determine whether or not a Seagate drive's firmware revision requires that its Read Look Aheads (RLAs) be
     * aborted, which involves issuing a Test Unit Ready (TUR) in specific situations.  AR 653188
     */
    sas_physical_drive->sas_drive_info.rla_abort_required = FBE_FALSE;

    if (info_ptr->drive_vendor_id == FBE_DRIVE_VENDOR_SEAGATE)
    {
        /* Handle a special case for the Seagate Compass bridge code (CSFC) and firmware (CSFE) used to fix those
         * drives that were misconfigured at the factory and could fail in about 6 months, but whose revision strings
         * are greater than the revision string of the Compass firmware that fixes this RLA problem (CS21).
         */
        if (strncmp(info_ptr->product_rev, "CS", 2) == 0 &&
            (strncmp(info_ptr->product_rev+2, "FC", 2) == 0 ||
             strncmp(info_ptr->product_rev+2, "FE", 2) == 0))
        {
            fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                                                       FBE_TRACE_LEVEL_INFO,
                                                       "%s RLA abort required, since FW rev %s matches CSFC or CSFE\n",
                                                       __FUNCTION__, info_ptr->product_rev);
            sas_physical_drive->sas_drive_info.rla_abort_required = FBE_TRUE;
        }
        else
        {
            /* Otherwise, determine if this drive's firmware revision string's:
             * 1) first 2 letters match the first 2 letters of any strings in the list of those firmware revisions
             *    that fix this RLA problem AND its
             * 2) last 2 letters are less than the last 2 letters of the matching revision string discovered in
             *    step #1.
             */
            for (i=0; i < num_rla_fixed_fw_revs; i++)
            {
                if (strncmp(info_ptr->product_rev, sas_drive_rla_fixed_map_table[i].fw_rev, 2) == 0)
                {
                    if (strncmp(info_ptr->product_rev+2, sas_drive_rla_fixed_map_table[i].fw_rev+2, 2) < 0)
                    {
                        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                                                                   FBE_TRACE_LEVEL_INFO,
                                                                   "%s RLA abort required, since FW rev %s comes before %s\n",
                                                                   __FUNCTION__, info_ptr->product_rev,
                                                                   sas_drive_rla_fixed_map_table[i].fw_rev);
                        sas_physical_drive->sas_drive_info.rla_abort_required = FBE_TRUE;
                        break;
                    }
                }
            }
        }
    }

    /* get serial number: byte 36-55 */
    fbe_zero_memory(info_ptr->product_serial_num, FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE + 1);
    fbe_copy_memory(info_ptr->product_serial_num, inquiry + FBE_SCSI_INQUIRY_SERIAL_NUMBER_OFFSET, FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE);

    /* Check Serial number here and retry if spaces are found.
     * This is because spaces are returned if not all inquiry data has been retrieved from the media.
     */
    if (fbe_equal_memory(info_ptr->product_serial_num, "                    ", FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE))
    {
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                                FBE_TRACE_LEVEL_WARNING,
                                "%s Invalid SN: %s\n",
                                __FUNCTION__, info_ptr->product_serial_num);
        
        return FBE_SAS_DRIVE_STATUS_NEED_RETRY;
    }

    /* get part number: byte 56-68 */
    fbe_zero_memory(info_ptr->dg_part_number_ascii, FBE_SCSI_INQUIRY_PART_NUMBER_SIZE + 1);
    fbe_copy_memory(info_ptr->dg_part_number_ascii, inquiry + FBE_SCSI_INQUIRY_PART_NUMBER_OFFSET, FBE_SCSI_INQUIRY_PART_NUMBER_SIZE);

    /* get TLA: byte 69-80 */
    fbe_zero_memory(info_ptr->tla_part_number, FBE_SCSI_INQUIRY_TLA_SIZE + 1);
    fbe_copy_memory(info_ptr->tla_part_number, inquiry + FBE_SCSI_INQUIRY_TLA_OFFSET, FBE_SCSI_INQUIRY_TLA_SIZE); 
#ifdef FBE_HANDLE_UNKNOWN_TLA_SUFFIX
    tla_suffix_ptr = (fbe_u8_t *)(info_ptr->tla_part_number + FBE_SCSI_INQUIRY_TLA_SIZE - FBE_SPINDOWN_STRING_LENGTH);
    drive_status = fbe_sas_physical_drive_validate_tla_suffix(tla_suffix_ptr);
    if (drive_status != FBE_SAS_DRIVE_STATUS_OK)
    {
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                FBE_TRACE_LEVEL_WARNING,
                                "%s Unsupported TLA: %s\n", __FUNCTION__, info_ptr->tla_part_number);
        return drive_status;
    }
#endif

    /* Initialize attributes that could be set by TLA suffix */
    info_ptr->spin_down_qualified = FBE_FALSE;
    /* Make sure the Flash Drive Flag and the MLC Flash Drive Flags are clear */
    info_ptr->drive_parameter_flags &= (~FBE_PDO_FLAG_FLASH_DRIVE);
    info_ptr->drive_parameter_flags &= (~FBE_PDO_FLAG_MLC_FLASH_DRIVE);

    if (is_emc_string)
    {
        /* If we are using new EMC string then the TLA suffix is no longer used. By default all HDDs
           will have spindown enabled and SSDs will have it disabled.  Mode page 4 will be used to 
           determine if drive is an SSD vs an HDD.  See how FBE_PDO_FLAG_DRIVE_TYPE_UNKNOWN is used. 
        */
        if (!fbe_equal_memory(info_ptr->tla_part_number + FBE_SCSI_INQUIRY_TLA_SIZE - FBE_TLA_SUFFIX_STRING_LENGTH, 
                              FBE_TLA_SUFFIX_STRING_EMPTY, FBE_TLA_SUFFIX_STRING_LENGTH))
        {
            fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                    FBE_TRACE_LEVEL_WARNING,
                                    "%s Unsupported TLA: %s when using EMC string\n", __FUNCTION__, info_ptr->tla_part_number);
            return FBE_SAS_DRIVE_STATUS_INVALID;                       
        }
        /* This flag tells the completion function that it needs to do more work to discover the drive type */
        info_ptr->drive_parameter_flags |= FBE_PDO_FLAG_DRIVE_TYPE_UNKNOWN;
    }
    else  /* legacy handling of TLA suffix */
    {
        /* Set spin_down_qualified */
        
        if (fbe_base_physical_drive_is_spindown_qualified((fbe_base_physical_drive_t*)sas_physical_drive, info_ptr->tla_part_number) == FBE_TRUE)
        {
            info_ptr->spin_down_qualified = FBE_TRUE;
        }
    
        /* Set flash_drive */

        if (fbe_equal_memory(info_ptr->tla_part_number + FBE_SCSI_INQUIRY_TLA_SIZE - FBE_SPINDOWN_STRING_LENGTH, 
            FBE_FLASH_DRIVE_STRING, FBE_SPINDOWN_STRING_LENGTH))
        {
            info_ptr->drive_parameter_flags |= FBE_PDO_FLAG_FLASH_DRIVE;
        }
        else if ( (info_ptr->drive_vendor_id == FBE_DRIVE_VENDOR_SAMSUNG) ||
                  ((info_ptr->drive_vendor_id == FBE_DRIVE_VENDOR_HITACHI) && fbe_equal_memory(info_ptr->product_id, "HUSSL", 5)) )
        {
            /* This is for legacy fw without "EFD" set. */
            info_ptr->drive_parameter_flags |= FBE_PDO_FLAG_FLASH_DRIVE;        
        }
        else if (fbe_equal_memory(info_ptr->tla_part_number + FBE_SCSI_INQUIRY_TLA_SIZE - FBE_SPINDOWN_STRING_LENGTH, 
                             FBE_MLC_FLASH_DRIVE_STRING, FBE_SPINDOWN_STRING_LENGTH))
        {
            info_ptr->drive_parameter_flags |= FBE_PDO_FLAG_MLC_FLASH_DRIVE;          
        }
    }

    /* Check for paddlecard.  All paddlecards have vendor ID prefixed with SATA. */
    info_ptr->paddlecard = FBE_FALSE;
    if (fbe_equal_memory(info_ptr->vendor_id, "SATA", 4))
    {
        info_ptr->paddlecard = FBE_TRUE;
    }
	
	info_ptr->drive_parameter_flags &= (~FBE_PDO_FLAG_SED_TLA_DETECTED);
	if(fbe_base_physical_drive_is_sed_drive_tla((fbe_base_physical_drive_t *)sas_physical_drive, info_ptr->tla_part_number) == FBE_TRUE) {
		// Detected drive with SED TLA, update parameter flag
		info_ptr->drive_parameter_flags |= FBE_PDO_FLAG_SED_TLA_DETECTED;		
		fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t *)sas_physical_drive,
				FBE_TRACE_LEVEL_DEBUG_HIGH,
				"%s SED Drive TLA detected\n", 
				__FUNCTION__);
	}

    /* TODO verify inquiry has QST enabled and set appropriate flag.   HC should not issue QST if not enabled, otherwise
       there is a good chance we'll shoot a healthy drive */

    return FBE_SAS_DRIVE_STATUS_OK;
}

fbe_sas_drive_status_t
fbe_sas_physical_drive_process_mode_sense_10(fbe_sas_physical_drive_t * sas_physical_drive, fbe_sg_element_t *sg_list)
{
    fbe_vendor_page_t *exp_ptr = NULL, *saved_exp_ptr;
    fbe_vendor_page_t local_copy[FBE_MS_MAX_SAS_TBL_ENTRIES_COUNT];
    fbe_response_table_t act_table;
    fbe_u8_t *mp_ptr = (fbe_u8_t *)sg_list[0].address;
    fbe_u8_t *pg_ptr = mp_ptr;
    fbe_u8_t *buff_ptr;
    fbe_u8_t *last_addr;
    fbe_u16_t act_entry, max;
    fbe_u8_t current_page;
    fbe_bool_t do_mode_select = FALSE;    
    //fbe_lba_t block_count;
    //fbe_block_size_t  block_size;
    fbe_drive_vendor_id_t drive_vendor_id;
    fbe_u16_t data_length, block_length;
    fbe_u8_t *block_ptr;
    fbe_sas_drive_status_t status;

    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, FBE_TRACE_LEVEL_DEBUG_HIGH, 
                                               "%s entry. sg addr:0x%x, sg len:0x%x\n", __FUNCTION__, (unsigned int)sg_list->address, sg_list->count );

    /* get expected mode pages */
    fbe_base_physical_drive_get_drive_vendor_id((fbe_base_physical_drive_t *) sas_physical_drive, &drive_vendor_id);


    /* get vendor table*/

    // Call virtual function, which can be overriden by subclasses
    if (sas_physical_drive->get_vendor_table == NULL)
    {
        fbe_base_physical_drive_customizable_trace( (fbe_base_physical_drive_t *)sas_physical_drive, 
                            FBE_TRACE_LEVEL_ERROR,
                            "%s get_vendor_table virtual function never initialized. \n",
                            __FUNCTION__);

        return FBE_SAS_DRIVE_STATUS_INVALID;
    }

    status = (sas_physical_drive->get_vendor_table)(sas_physical_drive, drive_vendor_id, &exp_ptr, &max);

    if (status != FBE_SAS_DRIVE_STATUS_OK)
    {
        return status;
    }

    if (max == 0)
    {
        /* nothing to do if override table is empty.
        */
        fbe_base_physical_drive_customizable_trace( (fbe_base_physical_drive_t *)sas_physical_drive, 
                            FBE_TRACE_LEVEL_INFO,
                            "%s mode page table is empty. Skipping mode select.\n",
                            __FUNCTION__);
        sas_physical_drive->sas_drive_info.do_mode_select = FALSE;
        sas_physical_drive->sas_drive_info.ms_retry_count = 0;
        return FBE_SAS_DRIVE_STATUS_OK;
    }

    /* Save a local copy. */
    max = FBE_MIN(max, FBE_MS_MAX_SAS_TBL_ENTRIES_COUNT);
    fbe_copy_memory(local_copy, exp_ptr, max * sizeof(fbe_vendor_page_t));
    saved_exp_ptr = local_copy;

    /* Fix table for enhanced queuing */
    fbe_sas_physical_drive_fix_table_for_enhanced_queuing(sas_physical_drive, saved_exp_ptr, max);

    /* Set cache write data. This value is 0x10. */
    ((fbe_base_physical_drive_t *)sas_physical_drive)->rd_wr_cache_value = FBE_PG08_RCD_DISC_BYTE;


    /* Set up tables */
    pg_ptr = buff_ptr = (fbe_u8_t *) sg_list->address;
    data_length = ((fbe_u16_t)(buff_ptr[0] << 8)) | ((fbe_u16_t)buff_ptr[1]);
    last_addr = (buff_ptr + data_length + 2);

    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, FBE_TRACE_LEVEL_DEBUG_HIGH, 
                                               "%s pg: 0x%x, data len:0x%x, last addr:0x%x\n", __FUNCTION__, (unsigned int)pg_ptr, data_length, (unsigned int)last_addr );

    if ((fbe_u8_t *) (sg_list->count + sg_list->address) < last_addr)
    {
         fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%s Bad SG: 0x%p\n",
                                __FUNCTION__, sg_list->count + sg_list->address);
         return FBE_SAS_DRIVE_STATUS_INVALID;
    }

    /* get actual mode pages */
    block_ptr = pg_ptr + FBE_MS10_BLOCK_DESCR_LEN_OFFSET;
    block_length = ((fbe_u16_t)(block_ptr[0] << 8)) | ((fbe_u16_t)block_ptr[1]);
    pg_ptr += block_length + FBE_MS10_HEADER_LEN;
    *pg_ptr &= FBE_MS_PAGE_NUM_MASK;
    current_page = *pg_ptr;
    act_entry = 0;
    while (pg_ptr < last_addr)
    {
        if (current_page == exp_ptr->page)
        {
            fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, FBE_TRACE_LEVEL_DEBUG_HIGH, 
                                                       "%s page match. current:0x%x exp:0x%x\n", __FUNCTION__, current_page, exp_ptr->page);                                                      
            act_table.mp[act_entry].pg_ptr = pg_ptr;
            act_table.mp[act_entry].page = exp_ptr->page;
            act_table.mp[act_entry].offset = exp_ptr->offset;
            act_table.mp[act_entry].value = *(pg_ptr + exp_ptr->offset);
            act_table.mp[act_entry].offset_ptr = (pg_ptr + exp_ptr->offset);

            fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, FBE_TRACE_LEVEL_DEBUG_HIGH, 
                                                       "prcs_ms_10 i:%d exp_ptr:0x%x pg_ptr:0x%x pg:0x%x off:%d val:0x%x off_ptr:0x%x\n", 
                                                       act_entry, (unsigned int)exp_ptr,
                                                       (unsigned int)act_table.mp[act_entry].pg_ptr,
                                                       act_table.mp[act_entry].page,
                                                       act_table.mp[act_entry].offset,
                                                       act_table.mp[act_entry].value,
                                                       (unsigned int)act_table.mp[act_entry].offset_ptr);

            act_entry++;
            exp_ptr++;

            if (act_entry > max)
            {
                break;
            }
        }
        else
        {
            fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, FBE_TRACE_LEVEL_DEBUG_HIGH, 
                                                       "%s page mis-match. current:0x%x exp:0x%x\n", __FUNCTION__, current_page, exp_ptr->page);

            pg_ptr += *(pg_ptr + 1) + 2;
            *pg_ptr &= FBE_MS_PAGE_NUM_MASK;
            current_page = *pg_ptr;
        }
    }
    while (pg_ptr < last_addr) {
        pg_ptr += *(pg_ptr + 1) + 2;
    }
    if (pg_ptr != last_addr) {
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%s Invalid mode page format. pg:0x%x != last:0x%x\n",
                                __FUNCTION__, (unsigned int)pg_ptr, (unsigned int)last_addr);

        return FBE_SAS_DRIVE_STATUS_INVALID;
    }

    /* Check the drive format */
    /*todo: why are we clearing length fields in the sg element?  answer: being reused to for mode select I think */
    buff_ptr = (fbe_u8_t *) sg_list->address;
    buff_ptr[0] = (fbe_u8_t) 0;
    buff_ptr[1] = (fbe_u8_t) 0;
    (*(buff_ptr + FBE_MS10_MEDIUM_TYPE_OFFSET)) = (fbe_u8_t) 0;
    (*(buff_ptr + FBE_MS10_DEVICE_SPECIFIC_PARAM_OFFSET)) = (fbe_u8_t) 0;
    buff_ptr += FBE_MS10_HEADER_LEN;

    /* This will not work! */
    /*if (drive_vendor_id == FBE_DRIVE_VENDOR_SEAGATE)
    {
        fbe_sas_physical_drive_fix_ms_table(exp_ptr, max, (fbe_u8_t) FBE_MS_CONTROL_PG0A,
                        (fbe_u8_t) FBE_PG0A_SEAGATE_QUEUE_TIME_OFFSET1,
                        (fbe_u8_t) fbe_seagate_queue_time);
        if (fbe_smart_delay_off == FBE_TRUE)
        {
            fbe_sas_physical_drive_fix_ms_table(exp_ptr, max, (fbe_u8_t) FBE_MS_REPORT_PG1C,
                    (fbe_u8_t) FBE_PG1C_PERF_OFFSET,
                    (fbe_u8_t) FBE_PG1C_PERF_DELAY_OFF);
        }
    }*/

    /* check response data against expected response data */
    exp_ptr = saved_exp_ptr;
    act_entry = 0;
    while (act_entry < max)
    {
        if (act_table.mp[act_entry].page != exp_ptr->page)
        {
            /* Page not supported by drive.  Serious problem, fail the drive. */
            fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                                    FBE_TRACE_LEVEL_ERROR,
                                    "%s page not supported by drive. page:0x%x != exp:0x%x\n",
                                    __FUNCTION__, act_table.mp[act_entry].page, exp_ptr->page);
            return FBE_SAS_DRIVE_STATUS_INVALID;
        }

        if ((act_table.mp[act_entry].value & exp_ptr->mask) != exp_ptr->value)
        {
            /* Expect valid input data, must be something wrong with the
             * response data...like not all pages are there. */
            if (act_table.mp[act_entry].offset_ptr == 0)
            {
                fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                        FBE_TRACE_LEVEL_ERROR,
                                        "%s page not correct. i:%d\n",
                                        __FUNCTION__, act_entry);
                return FBE_SAS_DRIVE_STATUS_INVALID;
            }

            /* Value not as expected, then fix it.  */
            /* Clear unwanted bits                                      */
            *(act_table.mp[act_entry].offset_ptr) &= ~exp_ptr->mask;
            /* Set desired bits                                         */
            *(act_table.mp[act_entry].offset_ptr) |= exp_ptr->value;
            do_mode_select = TRUE;  /* force a mode select */
        }
        act_entry++;
        exp_ptr++;
    }

    if ((do_mode_select == TRUE) || (sas_physical_drive->sas_drive_info.ms_retry_count == 0)) 
    {
        sas_physical_drive->sas_drive_info.do_mode_select = TRUE;
        sas_physical_drive->sas_drive_info.ms_size = data_length + 2;
        memcpy(sas_physical_drive->sas_drive_info.ms_pages, mp_ptr, sas_physical_drive->sas_drive_info.ms_size);
    } 
    else
    {
        sas_physical_drive->sas_drive_info.do_mode_select = FALSE;
        sas_physical_drive->sas_drive_info.ms_retry_count = 0;
    }

    return FBE_SAS_DRIVE_STATUS_OK;
}

/************************************************************************
 *       fbe_sas_physical_drive_fix_ms_table()
 ************************************************************************
 *
 * Description:
 *      This function modifies the expected mode page table in case
 *      there are parameters needing to be modified.
 *
 * Input:
 * Anscillary:
 *        Does not check for page-not-found or other error conditions.
 *
 * Returns:
 *        modified expected mode page table.
 *
 * History:
 *    12/2/2008 - CLC created.
 *
 ************************************************************************/
fbe_sas_drive_status_t
fbe_sas_physical_drive_fix_ms_table(fbe_vendor_page_t * exp_ptr, fbe_u16_t max, fbe_u8_t page, fbe_u8_t offset, fbe_u8_t value)
{
    fbe_u64_t lclMax = (fbe_u64_t)(csx_ptrhld_t) exp_ptr + (4 * max);

    for (exp_ptr = exp_ptr; ((fbe_u64_t)(csx_ptrhld_t) exp_ptr < lclMax); exp_ptr++)
    {
        if (exp_ptr->page == page)
        {
            if (exp_ptr->offset == offset)
            {
                exp_ptr->value = value;
                break;
            }
        }
    }

    return FBE_SAS_DRIVE_STATUS_OK;
} /* end of fbe_sas_physical_drive_fix_ms_table() */

fbe_sas_drive_status_t
fbe_sas_physical_drive_process_mode_page_attributes(fbe_sas_physical_drive_t * sas_physical_drive, fbe_sg_element_t *sg_list)
{
    fbe_u8_t* buff_ptr = (fbe_u8_t *) sg_list[0].address;
    fbe_u32_t hardware_max_link_rate = 0;
    fbe_physical_drive_speed_t hardware_max_speed_capability = FBE_DRIVE_SPEED_UNKNOWN;
    fbe_pdo_drive_parameter_flags_t  drive_parameter_flag;
    fbe_u16_t drive_RPM = 0;
    fbe_u8_t page = 0;

    fbe_sas_physical_drive_get_current_page(sas_physical_drive, &page, NULL);
    switch (page)
    {
        case FBE_MS_FIBRE_PG19:
           /* Parse the data in long format mode page 0x19 and extract link speed information
            * SAS PHY Physicsl Link Rate Field Information 
            *   0x0 to 0x7  Reserved
            *   0x8  1.5 Gbps    FBE_SCSI_MODE_PAGE_0x19_PHY_SPEED_FIELD_1_5_GBPS
            *   0x9  3  Gbps     FBE_SCSI_MODE_PAGE_0x19_PHY_SPEED_FIELD_3_GBPS
            *   0xA  6  Gbps     FBE_SCSI_MODE_PAGE_0x19_PHY_SPEED_FIELD_6_GBPS
            *   0xB  12 Gbps     FBE_SCSI_MODE_PAGE_0x19_PHY_SPEED_FIELD_12_GBPS
            *   0x C to 0xF  Reserved for future link rates            
            */  
            /* Get the link rate for the primary PHY */
            hardware_max_link_rate =
                                    *(FBE_SCSI_MODE_SENSE_HEADER_LENGTH +
                                        buff_ptr + FBE_SCSI_MODE_SENSE_PHY_DESCRIPTOR_OFFSET +
                                           FBE_SCSI_MODE_SENSE_HARDWARE_MAXIMUM_LINK_RATE_OFFSET) &
                                              FBE_SCSI_MODE_SENSE_HARDWARE_MAXIMUM_LINK_RATE_MASK;

            switch (hardware_max_link_rate)
            {
                case FBE_SCSI_MODE_PAGE_0x19_PHY_SPEED_FIELD_1_5_GBPS:
                    hardware_max_speed_capability = FBE_DRIVE_SPEED_1_5_GB;
                    break;
                case FBE_SCSI_MODE_PAGE_0x19_PHY_SPEED_FIELD_3_GBPS:
                    hardware_max_speed_capability = FBE_DRIVE_SPEED_3GB;
                    break;
                case FBE_SCSI_MODE_PAGE_0x19_PHY_SPEED_FIELD_6_GBPS:
                    hardware_max_speed_capability = FBE_DRIVE_SPEED_6GB;
                    break;
                case FBE_SCSI_MODE_PAGE_0x19_PHY_SPEED_FIELD_12_GBPS:
                    hardware_max_speed_capability = FBE_DRIVE_SPEED_12GB;
                    break;
                default:
                    hardware_max_speed_capability = FBE_DRIVE_SPEED_UNKNOWN;
                    break;
           }
           ((fbe_base_physical_drive_t*)sas_physical_drive)->drive_info.speed_capability = hardware_max_speed_capability;
            break;

        case FBE_MS_GEOMETRY_PG04:
            buff_ptr += FBE_SCSI_MODE_SENSE_HEADER_LENGTH + 20;
            drive_RPM = (fbe_u16_t)(buff_ptr[0]) << 8 | (fbe_u16_t)(buff_ptr[1]);

            /* Micron Buckhorn drives running B315/2496 FW are reporting an RPM value of 7200 RPM.  This is
             * displayed in Rockies but is not specifically used.  However it caused a failure in KittyHawk
             * because it is used when building storage pools.  Samsung RCX and Samsung RCX+ drives were also
             * found to be reporting 7200 RPM.  As these drives are in the field it was decided to hard code
             * the RPM for solid state drives if they are reporting an incorred RPM.
             * A message is logged if the RPM is not zero.
             */
            drive_parameter_flag = ((fbe_base_physical_drive_t*)sas_physical_drive)->drive_info.drive_parameter_flags;
            if ( ((drive_parameter_flag & FBE_PDO_FLAG_FLASH_DRIVE) || (drive_parameter_flag & FBE_PDO_FLAG_MLC_FLASH_DRIVE)) && 
                 (drive_RPM != FBE_EFD_DRIVE_EXPECTED_RPM) ) 
            {
                fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                        FBE_TRACE_LEVEL_WARNING,
                        "%s SSD RPM not zero. Reported: %d Rev: %s TLA: %-12s\n",
                        __FUNCTION__, drive_RPM,
                       ((fbe_base_physical_drive_t*)sas_physical_drive)->drive_info.revision,
                       ((fbe_base_physical_drive_t*)sas_physical_drive)->drive_info.tla );
                
                /* EFD Drive is reporting the wrong RPM so fix it here */
                drive_RPM = FBE_EFD_DRIVE_EXPECTED_RPM;
            }
            ((fbe_base_physical_drive_t*)sas_physical_drive)->drive_info.drive_RPM = (fbe_u32_t)drive_RPM;
            break;

        default:
            break;
    }

    return FBE_SAS_DRIVE_STATUS_OK;
}

fbe_status_t
fbe_sas_physical_drive_get_current_page(fbe_sas_physical_drive_t * sas_physical_drive, 
                                        fbe_u8_t *page, fbe_u8_t *max_pages)
{
    fbe_u8_t state = sas_physical_drive->sas_drive_info.ms_state;

    if (page != NULL)
    {
        *page = fbe_sas_attr_pages[state];
    }
    if (max_pages != NULL)
    {
        *max_pages = FBE_MS_MAX_SAS_ATTR_PAGES_ENTRIES_COUNT;
    }

    return FBE_STATUS_OK;
}

/**************************************************************************
* fbe_sas_physical_drive_fill_dc_info()                  
***************************************************************************
*
* DESCRIPTION
*       This function fill the disk collect structure.
*       
* PARAMETERS
*       sas_physical_drive - Pointer to SAS physical drive object.
*       payload_cdb_operation - Pointer to the cdb payload.
*       sense_data - Pointer to sense data.
*       cdb - Pointer to cdb.
*       no_bus_device_reset - No bus reset sense code.
*
* RETURN VALUES
*       status. 
*
* NOTES
*
* HISTORY
*   03-Mar-2010 Christina Chiang - Created.
***************************************************************************/

fbe_status_t
fbe_sas_physical_drive_fill_dc_info(fbe_sas_physical_drive_t * sas_physical_drive, 
                                       fbe_payload_cdb_operation_t * payload_cdb_operation,
                                       fbe_u8_t * sense_data,
                                       fbe_u8_t * cdb,
                                       fbe_bool_t no_bus_device_reset)
{
    fbe_base_physical_drive_t *base_physical_drive = (fbe_base_physical_drive_t*)sas_physical_drive;
    fbe_port_request_status_t cdb_request_status = FBE_PORT_REQUEST_STATUS_SUCCESS;
    fbe_payload_cdb_scsi_status_t cdb_scsi_status = FBE_PAYLOAD_CDB_SCSI_STATUS_GOOD;
    fbe_atomic_t dc_lock = FBE_FALSE;
    fbe_u8_t id;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_dc_system_time_t system_time;
    
    
    fbe_payload_cdb_get_request_status(payload_cdb_operation, &cdb_request_status);
    fbe_payload_cdb_get_scsi_status(payload_cdb_operation, &cdb_scsi_status);
    
    if ((cdb_scsi_status == FBE_PAYLOAD_CDB_SCSI_STATUS_CHECK_CONDITION) && (no_bus_device_reset == FBE_FALSE)) {
        /* It is 06/29/00, don't log it. */
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, 
                                 FBE_TRACE_LEVEL_DEBUG_HIGH,
                                 "%s DC doesn't log 06/29/00. \n", __FUNCTION__);
        return FBE_STATUS_OK;
    }
    
    /* Grap the lock. */
    dc_lock = fbe_atomic_exchange(&base_physical_drive->dc_lock, FBE_TRUE);
    if (dc_lock == FBE_TRUE) {
        /* Don't get the lock. */
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t *)sas_physical_drive, 
                                 FBE_TRACE_LEVEL_DEBUG_HIGH,
                                 "%s DC is locked. lba:0x%llx Record cnt:%d.\n",
                                  __FUNCTION__, (unsigned long long)base_physical_drive->dc_lba, base_physical_drive->dc.number_record);
        return FBE_STATUS_OK;
    }
    
    id = base_physical_drive->dc.number_record;
    if (id < FBE_DC_IN_MEMORY_MAX_COUNT)
    {
        /* Fill in the data for disk collect. */
        fbe_get_dc_system_time(&system_time);
        base_physical_drive->dc.magic_number = FBE_MAGIC_NUMBER_OBJECT+1;
        base_physical_drive->dc.dc_timestamp = system_time;
        base_physical_drive->dc.record[id].extended_A07 = 0;
        base_physical_drive->dc.record[id].dc_type = FBE_DC_CDB;
        base_physical_drive->dc.record[id].record_timestamp = system_time;
        base_physical_drive_get_dc_error_counts(base_physical_drive);
        fbe_copy_memory(base_physical_drive->dc.record[id].u.data.error.cdb.cdb, cdb, FBE_PAYLOAD_CDB_CDB_SIZE);
        fbe_copy_memory(base_physical_drive->dc.record[id].u.data.error.cdb.sense_info_buffer, sense_data, FBE_PAYLOAD_CDB_SENSE_INFO_BUFFER_SIZE);
        base_physical_drive->dc.record[id].u.data.error.cdb.port_request_status = cdb_request_status;
        base_physical_drive->dc.record[id].u.data.error.cdb.payload_cdb_scsi_status = cdb_scsi_status;
        
    }
    
    /* Increase the number_record even the record number is over 2. */
    base_physical_drive->dc.number_record++; 
    
    /* Release the lock. */
    fbe_atomic_exchange(&base_physical_drive->dc_lock, FBE_FALSE);
    
    if  (base_physical_drive->dc.number_record == FBE_DC_IN_MEMORY_MAX_COUNT)
    {
       fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                            FBE_TRACE_LEVEL_INFO,
                            "%s: set condition to flush data.\n", __FUNCTION__);

      status = fbe_lifecycle_set_cond(&fbe_sas_physical_drive_lifecycle_const, 
                             (fbe_base_object_t*)sas_physical_drive, 
                             FBE_BASE_PHYSICAL_DRIVE_LIFECYCLE_COND_DISK_COLLECT_FLUSH);
    }
    
    return status;
}

/**************************************************************************
* fbe_sas_physical_drive_fill_receive_diag_info()                  
***************************************************************************
*
* DESCRIPTION
*       This function fill the receive diagnostic page in the disk collect structure.
*       
* PARAMETERS
*       sas_physical_drive - Pointer to SAS physical drive object.
*       pass_thru_info - Pointer to the pass thru info.
*       buffer_size - size of pass thru info.
*
* RETURN VALUES
*       status. 
*
* NOTES
*
* HISTORY
*   28-June-2010 Christina Chiang - Created.
***************************************************************************/

fbe_status_t
fbe_sas_physical_drive_fill_receive_diag_info(fbe_sas_physical_drive_t * sas_physical_drive, 
                                       fbe_u8_t * rcv_diag,
                                       fbe_u8_t buffer_size)                                       
{
    fbe_base_physical_drive_t *base_physical_drive = (fbe_base_physical_drive_t*)sas_physical_drive;
    fbe_atomic_t dc_lock = FBE_FALSE;
    fbe_u8_t id;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_dc_system_time_t system_time;
   
    /* Grap the lock. */
    dc_lock = fbe_atomic_exchange(&base_physical_drive->dc_lock, FBE_TRUE);
    if (dc_lock == FBE_TRUE) {
        /* Don't get the lock. */
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t *)sas_physical_drive, 
                                 FBE_TRACE_LEVEL_INFO,
                                 "%s DC is locked. lba:0x%llx Record cnt:%d.\n",
                                  __FUNCTION__, (unsigned long long)base_physical_drive->dc_lba, base_physical_drive->dc.number_record);
                                   
        return FBE_STATUS_OK;
    }
    
    id = base_physical_drive->dc.number_record;
    if (id < FBE_DC_IN_MEMORY_MAX_COUNT)
    {
        /* Fill in the data for disk collect. */
        fbe_get_dc_system_time(&system_time);
        base_physical_drive->dc.magic_number = FBE_MAGIC_NUMBER_OBJECT+1;
        base_physical_drive->dc.dc_timestamp = system_time;
        base_physical_drive->dc.record[id].extended_A07 = 0;
        base_physical_drive->dc.record[id].dc_type = FBE_DC_RECEIVE_DIAG;
        base_physical_drive->dc.record[id].record_timestamp = system_time;
        fbe_copy_memory(&base_physical_drive->dc.record[id].u.rcv_diag.scsi_rcv_diag, rcv_diag, buffer_size);
    }
    base_physical_drive->dc.number_record++;
    
    /* Release the lock. */
    fbe_atomic_exchange(&base_physical_drive->dc_lock, FBE_FALSE);
    
    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive,
                        FBE_TRACE_LEVEL_INFO,
                        "%s: set condition to flush data.\n", __FUNCTION__);

    status = fbe_lifecycle_set_cond(&fbe_sas_physical_drive_lifecycle_const, 
                             (fbe_base_object_t*)sas_physical_drive, 
                             FBE_BASE_PHYSICAL_DRIVE_LIFECYCLE_COND_DISK_COLLECT_FLUSH);
    return status;
}

/**************************************************************************
* fbe_sas_physical_drive_get_vendor_table()                  
***************************************************************************
*
* DESCRIPTION
*       This is a base virtual function which will return the vendor table
*       for the default mode page settings.
*       
* PARAMETERS
*       sas_physical_drive - Pointer to SAS physical drive object.
*       drive_vendor_id - Vendor ID
*       table_ptr - Ptr to vendor table
*       num_table_entries - Number of entries in vendor table
*
* RETURN VALUES
*       status. 
*
* NOTES
*       The GENERIC drive vendor (FBE_DRIVE_VENDOR_GENERIC) is not included
*       in the vendor table selection since the GENERIC drive does not issue
*       mode sense or mode select commands at the specific request of SBMT.
*
* HISTORY
*   12/30/2010 Wayne Garrett - Created.
***************************************************************************/
fbe_sas_drive_status_t    
fbe_sas_physical_drive_get_vendor_table(fbe_sas_physical_drive_t * sas_physical_drive, fbe_drive_vendor_id_t drive_vendor_id, fbe_vendor_page_t ** table_ptr, fbe_u16_t * num_table_entries)
{
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
             *table_ptr = fbe_sam_sas_vendor_tbl;
             *num_table_entries = FBE_MS_MAX_SAM_SAS_TBL_ENTRIES_COUNT;
            break;
        case FBE_DRIVE_VENDOR_IBM:
        case FBE_DRIVE_VENDOR_SEAGATE:
        case FBE_DRIVE_VENDOR_FUJITSU:
        case FBE_DRIVE_VENDOR_HITACHI:
        case FBE_DRIVE_VENDOR_WESTERN_DIGITAL:            
        case FBE_DRIVE_VENDOR_MAXTOR:    
        case FBE_DRIVE_VENDOR_EMULEX:
        case FBE_DRIVE_VENDOR_TOSHIBA:  
            *table_ptr = fbe_def_sas_vendor_tbl;
            *num_table_entries = FBE_MS_MAX_DEF_SAS_TBL_ENTRIES_COUNT;
            break;
        case FBE_DRIVE_VENDOR_STEC:            
        case FBE_DRIVE_VENDOR_MICRON:
        case FBE_DRIVE_VENDOR_SANDISK:            
            *table_ptr = fbe_minimal_sas_vendor_tbl;
            *num_table_entries = FBE_MS_MAX_MINIMAL_SAS_TBL_ENTRIES_COUNT;
            break;
        default:
            fbe_base_physical_drive_customizable_trace( (fbe_base_physical_drive_t *)sas_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%s Unsupported vendor: 0x%X\n",
                                __FUNCTION__, drive_vendor_id);
            return FBE_SAS_DRIVE_STATUS_INVALID;
    }

    return FBE_SAS_DRIVE_STATUS_OK;
}

fbe_bool_t
fbe_sas_physical_drive_is_enhanced_queuing_supported(fbe_sas_physical_drive_t* sas_physical_drive)
{
    return fbe_base_physical_drive_is_enhanced_queuing_supported((fbe_base_physical_drive_t*)sas_physical_drive);
}

/**************************************************************************
* fbe_sas_physical_drive_is_write_media_error()                  
***************************************************************************
*
* DESCRIPTION
*       This function check if the error code is the media error with a valid lba and cdb is the write, write verify, or write same.
*       
* PARAMETERS
*       payload_cdb_operation - Pointer to the cdb payload.
*
* RETURN VALUES
*       FBE_TRUE  - The error code is the media error with a valid lba and cdb is the write, write verify, or write same.
*       FBE_FALSE - It is not above.
* NOTES
*
* HISTORY
*   10-Dec-2013 Christina Chiang - Created.
***************************************************************************/
fbe_bool_t
fbe_sas_physical_drive_is_write_media_error(fbe_payload_cdb_operation_t * cdb_operation, fbe_scsi_error_code_t error)
{

    /* These error codes need keep sync with the list of error codes in the caller of sas_physical_drive_setup_remap().*/
    if ((error == FBE_SCSI_CC_RECOVERED_BAD_BLOCK) ||
        (error == FBE_SCSI_CC_HARD_BAD_BLOCK) ||
        (error == FBE_SCSI_CC_MEDIA_ERR_DO_REMAP) ||
        (error == FBE_SCSI_CC_MEDIA_ERROR_WRITE_FAULT) ||
        (error == FBE_SCSI_CC_MEDIA_ERR_WRITE_ERROR))
    {
        switch(cdb_operation->cdb[0])
        {   
            case FBE_SCSI_WRITE_6:
            case FBE_SCSI_WRITE_10:
            case FBE_SCSI_WRITE_12:
            case FBE_SCSI_WRITE_16:
            case FBE_SCSI_WRITE_SAME_10:
            case FBE_SCSI_WRITE_SAME_16:
            case FBE_SCSI_WRITE_VERIFY_10:
            case FBE_SCSI_WRITE_VERIFY_16:
                return FBE_TRUE;
        
            default:
                return FBE_FALSE;
        }
    }
    
    return FBE_FALSE;
}
/*!**************************************************************
 * @fn fbe_sas_physical_drive_init_enhanced_queuing_timer
 ****************************************************************
 * @brief
 *  This function initializes the timer value in sas drive.
 * 
 * @param sas_physical_drive - pointer to the drive.
 * 
 * @return status - The status of the operation.
 *
 * @version
 *  08/22/2012  Lili Chen  - Created.
 *
 ****************************************************************/
fbe_status_t
fbe_sas_physical_drive_init_enhanced_queuing_timer(fbe_sas_physical_drive_t* sas_physical_drive)
{
    sas_physical_drive->sas_drive_info.lpq_timer = FBE_MS_PRIORITY_QUEUE_TIMER_INVALID;
    sas_physical_drive->sas_drive_info.hpq_timer = FBE_MS_PRIORITY_QUEUE_TIMER_INVALID;
    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_sas_physical_drive_set_enhanced_queuing_timer
 ****************************************************************
 * @brief
 *  This function sets enhanced queuing timers.
 * 
 * @param sas_physical_drive - pointer to the drive.
 * @param lpq_timer - lpq timer in ms.
 * @param hpq_timer - hpq timer in ms.
 * @param timer_changed - whether the timer is changed in drive.
 * 
 * @return status - The status of the operation.
 *
 * @version
 *  08/22/2012  Lili Chen  - Created.
 *
 ****************************************************************/
fbe_status_t
fbe_sas_physical_drive_set_enhanced_queuing_timer(fbe_sas_physical_drive_t* sas_physical_drive, fbe_u32_t lpq_timer_ms, fbe_u32_t hpq_timer_ms, fbe_bool_t *timer_changed)
{
    fbe_u32_t max = FBE_MS_PRIORITY_QUEUE_TIMER_INVALID * 50;
    fbe_u16_t lpq_timer, hpq_timer;

    *timer_changed = FBE_FALSE;

    if ((lpq_timer_ms == ENHANCED_QUEUING_TIMER_MS_INVALID) || (hpq_timer_ms == ENHANCED_QUEUING_TIMER_MS_INVALID))
    {
        /* Set default value. */
        lpq_timer = FBE_MS_PRIORITY_QUEUE_TIMER_INVALID;
        hpq_timer = FBE_MS_PRIORITY_QUEUE_TIMER_INVALID;
    }
    else if ((lpq_timer_ms >= max) || (hpq_timer_ms >= max)) 
    {
        fbe_base_physical_drive_customizable_trace( (fbe_base_physical_drive_t*)sas_physical_drive, FBE_TRACE_LEVEL_INFO,
                                                    "%s timer exceeds max lpq_timer %d hpq_timer %d.\n", 
                                                    __FUNCTION__, lpq_timer_ms, hpq_timer_ms);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    else
    {
        lpq_timer = lpq_timer_ms/50;
        hpq_timer = (hpq_timer_ms/100) * 2;
    }

    if ((sas_physical_drive->sas_drive_info.lpq_timer != lpq_timer) ||
        (sas_physical_drive->sas_drive_info.hpq_timer != hpq_timer))
    {
        fbe_base_physical_drive_customizable_trace( (fbe_base_physical_drive_t *)sas_physical_drive, 
                            FBE_TRACE_LEVEL_INFO,
                            "Enhanced Queuing timer changed lpq: 0x%x -> 0x%x hpq: 0x%x -> 0x%x\n",
                            sas_physical_drive->sas_drive_info.lpq_timer, lpq_timer, sas_physical_drive->sas_drive_info.hpq_timer, hpq_timer);
        sas_physical_drive->sas_drive_info.lpq_timer = lpq_timer;
        sas_physical_drive->sas_drive_info.hpq_timer = hpq_timer;
        *timer_changed = FBE_TRUE;
    }
    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_sas_physical_drive_get_enhanced_queuing_timer
 ****************************************************************
 * @brief
 *  This function gets enhanced queuing timer value from the drive.
 * 
 * @param sas_physical_drive - pointer to the drive.
 * @param lpq_timer - pointer to lpq timer in 50ms increment.
 * @param hpq_timer - pointer to hpq timer in 50ms increment.
 * 
 * @return status - The status of the operation.
 *
 * @version
 *  08/22/2012  Lili Chen  - Created.
 *
 ****************************************************************/
fbe_status_t
fbe_sas_physical_drive_get_enhanced_queuing_timer(fbe_sas_physical_drive_t* sas_physical_drive, fbe_u16_t *lpq_timer, fbe_u16_t *hpq_timer)
{
    *lpq_timer = sas_physical_drive->sas_drive_info.lpq_timer;
    *hpq_timer = sas_physical_drive->sas_drive_info.hpq_timer;
    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_sas_physical_drive_fix_table_for_enhanced_queuing
 ****************************************************************
 * @brief
 *  This function fixes mode page tables for enhanced queuing timer.
 * 
 * @param sas_physical_drive - pointer to the drive.
 * 
 * @return status - The status of the operation.
 *
 * @version
 *  08/22/2012  Lili Chen  - Created.
 *
 ****************************************************************/
static fbe_status_t
fbe_sas_physical_drive_fix_table_for_enhanced_queuing(fbe_sas_physical_drive_t* sas_physical_drive, fbe_vendor_page_t *exp_ptr, fbe_u16_t max)
{
    fbe_u16_t lpq_timer = 0, hpq_timer = 0;

    fbe_sas_physical_drive_get_enhanced_queuing_timer(sas_physical_drive, &lpq_timer, &hpq_timer);

    if ((lpq_timer == FBE_MS_PRIORITY_QUEUE_TIMER_INVALID) || (hpq_timer == FBE_MS_PRIORITY_QUEUE_TIMER_INVALID))
    {
        /* No need to change default value. */
        return FBE_STATUS_OK;
    }

    /* Set LPQ timer. */
    fbe_sas_physical_drive_fix_ms_table(exp_ptr, max, (fbe_u8_t) FBE_MS_VENDOR_PG00, (fbe_u8_t) 18, (fbe_u8_t) ((lpq_timer >> 8) & 0xFF));
    fbe_sas_physical_drive_fix_ms_table(exp_ptr, max, (fbe_u8_t) FBE_MS_VENDOR_PG00, (fbe_u8_t) 19, (fbe_u8_t) (lpq_timer & 0xFF));

    /* Set HPQ timer. */
    fbe_sas_physical_drive_fix_ms_table(exp_ptr, max, (fbe_u8_t) FBE_MS_VENDOR_PG00, (fbe_u8_t) 16, (fbe_u8_t) ((hpq_timer >> 8) & 0xFF));
    fbe_sas_physical_drive_fix_ms_table(exp_ptr, max, (fbe_u8_t) FBE_MS_VENDOR_PG00, (fbe_u8_t) 17, (fbe_u8_t) (hpq_timer & 0xFF));
    hpq_timer /= 2;
    fbe_sas_physical_drive_fix_ms_table(exp_ptr, max, (fbe_u8_t) FBE_MS_CONTROL_PG0A, (fbe_u8_t) 10, (fbe_u8_t) ((hpq_timer >> 8) & 0xFF));
    fbe_sas_physical_drive_fix_ms_table(exp_ptr, max, (fbe_u8_t) FBE_MS_CONTROL_PG0A, (fbe_u8_t) 11, (fbe_u8_t) (hpq_timer & 0xFF));

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * @fn fbe_sas_physical_drive_get_queuing_info_from_DCS
 ****************************************************************
 * @brief
 *  This function gets enhanced queuing information from DCS.
 * 
 * @param sas_physical_drive - pointer to the drive.
 * @param lpq_timer - pointer to lpq timer in ms.
 * @param hpq_timer - pointer to hpq timer in ms.
 * 
 * @return status - The status of the operation.
 *
 * @version
 *  08/22/2012  Lili Chen  - Created.
 *
 ****************************************************************/
fbe_status_t
fbe_sas_physical_drive_get_queuing_info_from_DCS(fbe_sas_physical_drive_t* sas_physical_drive, fbe_u32_t *lpq_timer, fbe_u32_t *hpq_timer)
{
    fbe_drive_configuration_registration_info_t drive_info;
    fbe_base_physical_drive_t * base_physical_drive = (fbe_base_physical_drive_t*)sas_physical_drive;
    fbe_status_t status = FBE_STATUS_OK;

    /* Initialize drive info */
    drive_info.drive_type = FBE_DRIVE_TYPE_SAS;
    fbe_base_physical_drive_get_drive_vendor_id(base_physical_drive, &drive_info.drive_vendor);
    fbe_zero_memory(drive_info.fw_revision, FBE_SCSI_INQUIRY_REVISION_SIZE + 1);
    fbe_copy_memory(drive_info.fw_revision, base_physical_drive->drive_info.revision,FBE_SCSI_INQUIRY_REVISION_SIZE);
    fbe_zero_memory(drive_info.serial_number, FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE + 1);
    fbe_copy_memory(drive_info.serial_number, base_physical_drive->drive_info.serial_number, FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE);
    fbe_zero_memory(drive_info.part_number, FBE_SCSI_INQUIRY_PART_NUMBER_SIZE + 1);
    fbe_copy_memory(drive_info.part_number, base_physical_drive->drive_info.tla, FBE_SCSI_INQUIRY_TLA_SIZE); //we used tla part number instead.

    status = fbe_drive_configuration_get_queuing_info(&drive_info, lpq_timer, hpq_timer);
    return status;
}

/*!**************************************************************
 * @fn fbe_sas_physical_drive_compare_mode_pages
 ****************************************************************
 * @brief
 *  This function compares the default pages and saved pages.
 * 
 * @param default_p - pointer to default mode pages.
 * @param saved_p   - pointer to saved mode pages.
 * 
 * @return FBE_TRUE if two sets are equal.
 *
 * @version
 *  10/25/2013  Lili Chen  - Created.
 *
 ****************************************************************/
fbe_bool_t
fbe_sas_physical_drive_compare_mode_pages(fbe_u8_t *default_p, fbe_u8_t *saved_p)
{
    fbe_u8_t *default_pg_ptr, *saved_pg_ptr, *last_addr, *block_ptr;
    fbe_u16_t data_length, total_length, block_length;
    fbe_u8_t saved_page, page_len;

    default_pg_ptr = default_p;
    saved_pg_ptr = saved_p;
    data_length = ((fbe_u16_t)(saved_pg_ptr[0] << 8)) | ((fbe_u16_t)saved_pg_ptr[1]);
    total_length = data_length + 2;
    last_addr = (saved_pg_ptr + total_length);

    /* We need to change page_num bytes with the mask, as it is done in 
     * fbe_sas_physical_drive_process_mode_sense_10()
     */
    block_ptr = saved_pg_ptr + FBE_MS10_BLOCK_DESCR_LEN_OFFSET;
    block_length = ((fbe_u16_t)(block_ptr[0] << 8)) | ((fbe_u16_t)block_ptr[1]);
    saved_pg_ptr += (block_length + FBE_MS10_HEADER_LEN);
    while (saved_pg_ptr < last_addr)
    {
        *saved_pg_ptr &= FBE_MS_PAGE_NUM_MASK;
        saved_page = saved_pg_ptr[0];
        page_len = saved_pg_ptr[1];

        saved_pg_ptr += (page_len + 2);
    }

    return (fbe_equal_memory(default_p+FBE_MS10_HEADER_LEN, saved_p+FBE_MS10_HEADER_LEN, total_length-FBE_MS10_HEADER_LEN));
}

/* Trace in logical 520 block sizes.*/
void fbe_sas_physical_drive_trace_rba(fbe_sas_physical_drive_t * sas_physical_drive,
                                      fbe_payload_cdb_operation_t *payload_cdb_operation,
                                      fbe_u32_t io_traffic_info)
{
    fbe_lba_t lba = 0;        
    fbe_block_count_t block_count= 0;

    lba = fbe_get_cdb_lba(payload_cdb_operation);
    block_count = fbe_get_cdb_blocks(payload_cdb_operation);
    
    if (sas_physical_drive->base_physical_drive.block_size == FBE_4K_BYTES_PER_BLOCK)
    {
        /* if this is a 4k drive, then convert lba and blocks to 520. */
        lba *= FBE_520_BLOCKS_PER_4K;
        block_count *= FBE_520_BLOCKS_PER_4K;
    }

    fbe_trace_RBA_traffic((fbe_base_physical_drive_t*)sas_physical_drive, lba, block_count, io_traffic_info);   
}


void fbe_sas_physical_drive_trace_completion(fbe_sas_physical_drive_t * sas_physical_drive, 
                                             fbe_packet_t * packet,
                                             fbe_payload_block_operation_t *block_operation,
                                             fbe_payload_cdb_operation_t * payload_cdb_operation,
                                             fbe_scsi_error_code_t error)
{
    fbe_u16_t extra = fbe_traffic_trace_get_priority(packet);
    fbe_lba_t lba = 0;        
    fbe_block_count_t block_count= 0;

    if (error != 0)
    {
        extra |= KT_FBE_ERROR;
    }

    lba = fbe_get_cdb_lba(payload_cdb_operation);
    block_count = fbe_get_cdb_blocks(payload_cdb_operation);

    if (sas_physical_drive->base_physical_drive.block_size == FBE_4K_BYTES_PER_BLOCK)
    {
        /* if this is a 4k drive, then convert lba and blocks to 520. */
        lba *= FBE_520_BLOCKS_PER_4K;
        block_count *= FBE_520_BLOCKS_PER_4K;
    }

    /* Determine if the command that is completing is a read or
       a write so we can log it properly */
    if ( payload_cdb_operation->cdb[0] == FBE_SCSI_READ_10 ||
         payload_cdb_operation->cdb[0] == FBE_SCSI_READ_16 )
    {
        fbe_trace_RBA_traffic((fbe_base_physical_drive_t*)sas_physical_drive, lba, block_count, KT_TRAFFIC_READ_DONE | extra);
    }
    else if ( payload_cdb_operation->cdb[0] == FBE_SCSI_WRITE_10 || 
              payload_cdb_operation->cdb[0] == FBE_SCSI_WRITE_16 || 
              payload_cdb_operation->cdb[0] == FBE_SCSI_WRITE_VERIFY_10 || 
              payload_cdb_operation->cdb[0] == FBE_SCSI_WRITE_VERIFY_16 )
    {
        fbe_trace_RBA_traffic((fbe_base_physical_drive_t*)sas_physical_drive, lba, block_count, KT_TRAFFIC_WRITE_DONE | extra);
    }
    else if ( payload_cdb_operation->cdb[0] == FBE_SCSI_WRITE_SAME_10 || 
              payload_cdb_operation->cdb[0] == FBE_SCSI_WRITE_SAME_16 )
    {
        // We map our write same to the zero bit.
        fbe_trace_RBA_traffic((fbe_base_physical_drive_t*)sas_physical_drive, lba, block_count, KT_TRAFFIC_ZERO_DONE | extra);
    }
    else if ( payload_cdb_operation->cdb[0] == FBE_SCSI_VERIFY_10|| 
              payload_cdb_operation->cdb[0] == FBE_SCSI_VERIFY_16 )
    {
#if 0 // Too verbose for now.
        // We map our verifies to the corrupt crc bit.
        fbe_trace_RBA_traffic((fbe_base_physical_drive_t*)sas_physical_drive, lba, block_count, KT_TRAFFIC_CRPT_CRC_DONE | extra);
#endif
    }
    else
    {
        /* Unknown commands get mapped to yet a different command.
         */
        fbe_trace_RBA_traffic((fbe_base_physical_drive_t*)sas_physical_drive, lba, block_count, 
                              (KT_TRAFFIC_NO_OP | KT_DONE | extra));
    }
}



/*!**************************************************************
 * @fn sas_pdo_is_in_sanitize_state
 ****************************************************************
 * @brief
 *  This function checks if we are in a Sanitize maintanence mode.
 * 
 * @param sas_physical_drive - pointer to pdo object.
 * 
 * @return FBE_TRUE if in maintanence mode.
 *
 * @version
 *  07/25/2014  Wayne Garrett  - Created.
 *
 ****************************************************************/
fbe_bool_t
sas_pdo_is_in_sanitize_state(const fbe_sas_physical_drive_t *sas_physical_drive)
{
    if (fbe_pdo_maintenance_mode_is_flag_set(&sas_physical_drive->base_physical_drive.maintenance_mode_bitmap, 
                                             FBE_PDO_MAINTENANCE_FLAG_SANITIZE_STATE))
    {
        return FBE_TRUE;
    }
    else
    {
        return FALSE;
    }
}


/*!**************************************************************
 * @fn sas_pdo_process_sanitize_error_for_conditions
 ****************************************************************
 * @brief
 *  This function processes Sanitize errors returned in
 *  condition completion functions.   It is intended to be used
 *  to allow the activate rotary to continue to the Ready state,
 *  otherwise we would eventually fail the drive if we continued
 *  to retry.  It does this by changing the return status. It will
 *  also set the appropriate maintenance mode and sanitization states.
 *  These state tell the clients that PDO will need to be
 *  re-initalized.
 *  
 * @param sas_physical_drive - pointer to pdo object.
 * @param payload_control_operation - Packet payload control info
 * 
 * @return void
 *
 * @version
 *  07/25/2014  Wayne Garrett  - Created.
 *
 ****************************************************************/
void sas_pdo_process_sanitize_error_for_conditions(fbe_sas_physical_drive_t *sas_physical_drive, fbe_payload_control_operation_t *payload_control_operation, fbe_payload_cdb_operation_t * payload_cdb_operation)
{
    fbe_payload_control_status_qualifier_t control_status_qualifier;

    fbe_payload_control_get_status_qualifier(payload_control_operation, &control_status_qualifier);

    if (control_status_qualifier == (fbe_payload_control_status_qualifier_t)FBE_SAS_DRIVE_STATUS_SANITIZE_IN_PROGRESS)
    {
        fbe_u8_t * sense_data = NULL;
        fbe_bool_t is_sksv_set = FBE_FALSE;

        sas_physical_drive->sas_drive_info.sanitize_status = FBE_DRIVE_SANITIZE_STATUS_IN_PROGRESS;

        /* update sanitize percent */
        fbe_payload_cdb_operation_get_sense_buffer(payload_cdb_operation, &sense_data);
        fbe_payload_cdb_parse_sense_data_for_sanitization_percent(sense_data, &is_sksv_set, &sas_physical_drive->sas_drive_info.sanitize_percent);
        if(!is_sksv_set) /* percent not valid */
        {
            fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, FBE_TRACE_LEVEL_WARNING,
                                                       "%s SKSV bit not set.  Must be bug in drive fw. set prcnt=0\n", __FUNCTION__);
            sas_physical_drive->sas_drive_info.sanitize_percent = 0;  /* indicates inprogress, but SKSV bit not set.  No way to now how far along we are. */
        }

        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, FBE_TRACE_LEVEL_INFO,
                                                   "%s sanitize in progress. percent:0x%04x/0x%04x \n", 
                                                   __FUNCTION__, sas_physical_drive->sas_drive_info.sanitize_percent, FBE_DRIVE_MAX_SANITIZATION_PERCENT);            
    }
    else if (control_status_qualifier == (fbe_payload_control_status_qualifier_t)FBE_SAS_DRIVE_STATUS_SANITIZE_NEED_RESTART)
    {
        sas_physical_drive->sas_drive_info.sanitize_status = FBE_DRIVE_SANITIZE_STATUS_RESTART;

        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, FBE_TRACE_LEVEL_WARNING,
                                                   "%s sanitize needs restart\n", __FUNCTION__);  
    }
    else
    {
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_physical_drive, FBE_TRACE_LEVEL_ERROR,
                                                   "%s qualifier %d not expected\n", __FUNCTION__, control_status_qualifier); 
    }

    /* Set maintenence mode so PVD can take corrective action */
    fbe_pdo_maintenance_mode_set_flag(&sas_physical_drive->base_physical_drive.maintenance_mode_bitmap, FBE_PDO_MAINTENANCE_FLAG_SANITIZE_STATE);

    /* Change control status to report OK*/
    fbe_payload_control_set_status(payload_control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_payload_control_set_status_qualifier(payload_control_operation, FBE_SAS_DRIVE_STATUS_OK);
}
