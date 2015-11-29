/**********************************************************************
 *  Copyright (C)  EMC Corporation 2008
 *  All rights reserved.
 *  Licensed material - property of EMC Corporation.
 **********************************************************************/

/*!*************************************************************************
 * @file fbe_sata_physical_drive_utils.c
 ***************************************************************************
 *
 *  The routines in this file contains all the utility functions used by the 
 *  SATA physical drive object.
 *
 * NOTE: 
 *
 * HISTORY
 *   22-May-2009 Eric Sondhi - Created. 
 *  
 *  *****************************************************************
 *  *** 2012_03_30 SATA DRIVES ARE FAULTED IF PLACED IN THE ARRAY ***
 *  *****************************************************************
 *      SATA drive support was added as black content to R30
 *      but SATA drives were never supported for performance reasons.
 *      The code has not been removed in the event that we wish to
 *      re-address the use of SATA drives at some point in the future.
 *
 ***************************************************************************/

#include "fbe/fbe_winddk.h"
#include "fbe/fbe_memory.h"
#include "fbe/fbe_transport.h"

#include "fbe_transport_memory.h"
#include "fbe_topology.h"
#include "fbe_scheduler.h"

#include "sata_physical_drive_private.h"
#include "base_physical_drive_init.h"

static __forceinline void
fbe_sata_physical_drive_byteswap_copy(fbe_u8_t * dest, fbe_u8_t * source, fbe_u32_t length)
{
    fbe_u32_t i;

    for(i = 0 ; i < length; i++)
    {
        if(!(i % 2))
        {
            dest[i] = source[i+1];
        } 
        else 
        {
            dest[i] = source[i-1];
        }
    }
}



fbe_sata_drive_status_t 
fbe_sata_physical_drive_set_dev_info_from_inscription(fbe_sata_physical_drive_t * sata_physical_drive, 
                                                      fbe_u8_t *inquiry_data,
                                                      fbe_physical_drive_information_t * info_ptr)
{
    
#ifdef BLOCKSIZE_512_SUPPORTED    
    fbe_bool_t clar_string_found_in_inquiry = FBE_FALSE;
    fbe_bool_t  drive_found_in_drive_table = FBE_FALSE;
    fbe_u32_t i;
    fbe_u32_t sn_len = 0;
    fbe_u32_t tla_len = 0;
#endif  

    /* get vendor id: byte 8-15 */
    fbe_zero_memory(info_ptr->vendor_id, FBE_SCSI_INQUIRY_VENDOR_ID_SIZE + 1);
    fbe_copy_memory(info_ptr->vendor_id, inquiry_data + FBE_SCSI_INQUIRY_VENDOR_ID_OFFSET, FBE_SCSI_INQUIRY_VENDOR_ID_SIZE); 

#ifndef BLOCKSIZE_512_SUPPORTED    
    /*In R30 support was added as black content for native SATA drives.  However it was
     *  subsequently decided not to use native SATA drives on the array.  The native SATA code was
     *  not maintained but was not removed.  The lines below will fault any native SATA
     *  drive plugged into the array.
     */
    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive, 
            FBE_TRACE_LEVEL_WARNING,
            "%s Native SATA drives not supported ID: %s\n",
            __FUNCTION__, info_ptr->vendor_id);  
    /* This return is intentional. Please see note below */
    return FBE_SATA_DRIVE_STATUS_INVALID;
}    
    /* All native SATA drives have been faulted at this point.
     *   The remaining SATA physical drive code below will not be executed
     *   but it has not been removed in case we add support at some point
     *   in the future.
     */ 
#else
    if (fbe_equal_memory(info_ptr->vendor_id, "ATA-ST", 6)) {
        info_ptr->drive_vendor_id = FBE_DRIVE_VENDOR_SEAGATE;
    } 
    else if (fbe_equal_memory(info_ptr->vendor_id, "ATA-IBM", 7)) {
        info_ptr->drive_vendor_id = FBE_DRIVE_VENDOR_IBM;
    }
    else if (fbe_equal_memory(info_ptr->vendor_id, "ATA-HTCH", 8)) {
        info_ptr->drive_vendor_id = FBE_DRIVE_VENDOR_HITACHI;
    }
    else if (fbe_equal_memory(info_ptr->vendor_id, "ATA-WDC", 7)) {
        info_ptr->drive_vendor_id = FBE_DRIVE_VENDOR_WESTERN_DIGITAL;
    }
    else if (fbe_equal_memory(info_ptr->vendor_id, "ATA-MXTR", 8)) {
        info_ptr->drive_vendor_id = FBE_DRIVE_VENDOR_MAXTOR;
    }
    else if (fbe_equal_memory(info_ptr->vendor_id, "ATA-SAMS", 8)) {
        info_ptr->drive_vendor_id = FBE_DRIVE_VENDOR_SAMSUNG;
    }
    else if (fbe_equal_memory(info_ptr->vendor_id, "SAMSUNG", 7)) {
        info_ptr->drive_vendor_id = FBE_DRIVE_VENDOR_SAMSUNG;
    } 
    else {
        info_ptr->drive_vendor_id = FBE_DRIVE_VENDOR_INVALID;
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive, 
                                FBE_TRACE_LEVEL_WARNING,
                                "%s Invalid vendor_id: %s\n",
                                __FUNCTION__, info_ptr->vendor_id);

        return FBE_SATA_DRIVE_STATUS_INVALID;
    }

    /* The logging is handled in usurper. We copy the inquiry data to 
     * the fbe_base_physical_drive_info_t and validate them when is needed. 
     */

    /* get product id: byte 16-31 */
    fbe_zero_memory(info_ptr->product_id, FBE_SCSI_INQUIRY_PRODUCT_ID_SIZE + 1);
    fbe_copy_memory(info_ptr->product_id, inquiry_data + FBE_SCSI_INQUIRY_PRODUCT_ID_OFFSET, FBE_SCSI_INQUIRY_PRODUCT_ID_SIZE);

    /* Validate product id here. Note that we are comparing the LOW HALF of the Inquiry
        * product-id (bytes 24-31).  We add 8 to drop bytes 16 thru 23.
        */
    clar_string_found_in_inquiry = FBE_FALSE;
    for (i = 8; i <= 12; i++) {
        if (fbe_equal_memory(info_ptr->product_id + i, "CLAR", 4)) {
            clar_string_found_in_inquiry = FBE_TRUE;      
            break;
        }
    }

    if (clar_string_found_in_inquiry == FBE_FALSE) {
        /* The "CLAR" string is not found. */
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%s Unsupported Drive: %s\n",
                                __FUNCTION__, info_ptr->product_id);
        return FBE_SATA_DRIVE_STATUS_INVALID;
    }

    info_ptr->drive_price_class = FBE_DRIVE_PRICE_CLASS_B;
    
    //last 4 bytes of FW rev should come from identify; not inscription so we copy it out of stored PDO data
    fbe_zero_memory(info_ptr->product_rev, FBE_SCSI_INQUIRY_REVISION_SIZE + 1);
    
    //If the Identify data had a 4 byte rev copy from the beginning of the buffer
    if(fbe_equal_memory(&sata_physical_drive->sata_drive_info.fw_rev[4], " ", 1))
    {
        fbe_copy_memory(info_ptr->product_rev, sata_physical_drive->sata_drive_info.fw_rev, FBE_SCSI_INQUIRY_REVISION_SIZE);
    }
    else
    {
        fbe_copy_memory(info_ptr->product_rev, &sata_physical_drive->sata_drive_info.fw_rev[4], FBE_SCSI_INQUIRY_REVISION_SIZE);
    }

    //Serial # should come from identify; not inscription so we copy it out of stored PDO data
    fbe_zero_memory(info_ptr->product_serial_num, FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE + 1);
    fbe_copy_memory(info_ptr->product_serial_num, sata_physical_drive->sata_drive_info.serial_num, FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE);

    /* get part number: byte 56-68 */
    fbe_zero_memory(info_ptr->dg_part_number_ascii, FBE_SCSI_INQUIRY_PART_NUMBER_SIZE + 1);
    fbe_copy_memory(info_ptr->dg_part_number_ascii, inquiry_data + FBE_SCSI_INQUIRY_PART_NUMBER_OFFSET, FBE_SCSI_INQUIRY_PART_NUMBER_SIZE);

    /* get TLA: byte 69-80 */
    fbe_zero_memory(info_ptr->tla_part_number, FBE_SCSI_INQUIRY_TLA_SIZE + 1);
    fbe_copy_memory(info_ptr->tla_part_number, inquiry_data + FBE_SCSI_INQUIRY_TLA_OFFSET, FBE_SCSI_INQUIRY_TLA_SIZE);

    /* Set spin_down_qualified */
    info_ptr->spin_down_qualified = FBE_FALSE;
    if (fbe_base_physical_drive_is_spindown_qualified((fbe_base_physical_drive_t*)sata_physical_drive, info_ptr->tla_part_number) == FBE_TRUE)
    {
        info_ptr->spin_down_qualified = FBE_TRUE;
    }

    return FBE_SATA_DRIVE_STATUS_OK;
}
#endif


fbe_sata_drive_status_t 
fbe_sata_physical_drive_parse_identify(fbe_sata_physical_drive_t * sata_physical_drive, 
                                       fbe_u8_t *identify_data,
                                       fbe_physical_drive_information_t * drive_info)
{
    fbe_sata_drive_status_t drive_status;
    fbe_status_t status;
    fbe_lba_t block_count;
    fbe_u16_t word_106 = ((fbe_u16_t)identify_data[212] | ((fbe_u16_t)identify_data[213] << 8));

    //drive_status = FBE_SATA_DRIVE_STATUS_PROG_IDENTIFY_NOT_SUPPORTED;
    drive_status = FBE_SATA_DRIVE_STATUS_OK;

    //TODO: Programmable Identify Support Here
    //for now set read inscription condition
    status = fbe_lifecycle_set_cond(&fbe_sata_physical_drive_lifecycle_const, 
                                    (fbe_base_object_t*)sata_physical_drive, 
                                    FBE_SATA_PHYSICAL_DRIVE_LIFECYCLE_COND_READ_INSCRIPTION);
    
    

    if (status != FBE_STATUS_OK) 
    {
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%s can't set read_inscription condition, status: 0x%X\n",
                                __FUNCTION__, status);
    }
    
    /* If word 106 bit 15,14=0,1 and bit 12=1, we should use word 117 and 118 for block size. */
    if ((word_106 & 0xd000) == 0x5000) {
        drive_info->block_size = ( ((fbe_block_size_t)identify_data[237] << 24) | 
                                   ((fbe_block_size_t)identify_data[236] << 16) | 
                                   ((fbe_block_size_t)identify_data[235] << 8) | 
                                   ((fbe_block_size_t)identify_data[234]) );
        drive_info->block_size *= 2;
    } else {
        drive_info->block_size = 512;
    }

    //Set Queue Depth
    drive_info->drive_qdepth = (fbe_u32_t)(identify_data[FBE_SATA_Q_DEPTH_OFFSET] & FBE_SATA_Q_DEPTH_MASK);
    
    /* Read in link speed capability - The drive will have a bit set in byte 152 for each of the
     *   link speeds that it supports.
     *   Bit 1 is set if the drive supports 1.5 Gb/s
      *  Bit 2 is set if the drive supports 3.0 Gb/s
     * Report the hightst speed.
     */

    if ((identify_data[FBE_SATA_SPEED_CAPABILITY] & FBE_SATA_SPEED_CAPABILITY_3_0_GBPS_BIT_MASK))
    {
        drive_info->speed_capability = FBE_DRIVE_SPEED_3GB;
    }
    else if((identify_data[FBE_SATA_SPEED_CAPABILITY] & FBE_SATA_SPEED_CAPABILITY_1_5_GBPS_BIT_MASK))
    {
        drive_info->speed_capability = FBE_DRIVE_SPEED_1_5_GB;
    }    
    else
    {
        drive_info->speed_capability = FBE_DRIVE_SPEED_UNKNOWN;
    }    
   
    //Copy block count
    block_count = ((fbe_u64_t)identify_data[205] << 40) | 
                  ((fbe_u64_t)identify_data[204] << 32) |
                  ((fbe_u64_t)identify_data[203] << 24) | 
                  ((fbe_u64_t)identify_data[202] << 16) | 
                  ((fbe_u64_t)identify_data[201] << 8) | 
                  ((fbe_u64_t)identify_data[200]);

    drive_info->block_count = block_count;
    

    //Copy Serial Number
    fbe_sata_physical_drive_byteswap_copy(drive_info->product_serial_num, identify_data + FBE_SATA_SERIAL_NUMBER_OFFSET, FBE_SATA_SERIAL_NUMBER_SIZE);
    
    /* Check Serial number here */
    if (fbe_equal_memory(drive_info->product_serial_num, "                    ", FBE_SATA_SERIAL_NUMBER_SIZE))
    {
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive,
                                FBE_TRACE_LEVEL_WARNING,
                                "%s Invalid SN: %s\n",
                                __FUNCTION__, drive_info->product_serial_num);
        
        return FBE_SATA_DRIVE_STATUS_NEED_RETRY;
    }
    
    // Copy Block count
    sata_physical_drive->sata_drive_info.native_capacity = block_count - 1;

    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive,
                        FBE_TRACE_LEVEL_INFO,
                        "%s Block Count %llX, Native Capacity %llX\n", 
                        __FUNCTION__, 
                        (unsigned long long)drive_info->block_count,
                        (unsigned long long)sata_physical_drive->sata_drive_info.native_capacity);
                        
    //Store model number in PDO:
    fbe_zero_memory(sata_physical_drive->sata_drive_info.model_num, FBE_SATA_MODEL_NUMBER_SIZE + 1);
    fbe_sata_physical_drive_byteswap_copy(sata_physical_drive->sata_drive_info.model_num, identify_data + FBE_SATA_MODEL_NUMBER_OFFSET, FBE_SATA_MODEL_NUMBER_SIZE);

    //Store Serial Number in PDO
    fbe_zero_memory(sata_physical_drive->sata_drive_info.serial_num, FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE + 1);
    fbe_copy_memory(sata_physical_drive->sata_drive_info.serial_num, drive_info->product_serial_num, FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE);

    //Store FW revision in PDO
    fbe_zero_memory(sata_physical_drive->sata_drive_info.fw_rev, FBE_SATA_FIRMWARE_REVISION_SIZE + 1);
    fbe_sata_physical_drive_byteswap_copy(sata_physical_drive->sata_drive_info.fw_rev, identify_data + FBE_SATA_FIRMWARE_REVISION_OFFSET, FBE_SATA_FIRMWARE_REVISION_SIZE);

    //Store last 4 bytes of FW revision in drive data to go to FLARE.
    //If the Identify data had a 4 byte rev copy from the beginning of the buffer
    if(fbe_equal_memory(identify_data + FBE_SATA_FIRMWARE_REVISION_OFFSET + 4, " ", 1))
    {
        fbe_sata_physical_drive_byteswap_copy(drive_info->product_rev, identify_data + FBE_SATA_FIRMWARE_REVISION_OFFSET, FBE_SCSI_INQUIRY_REVISION_SIZE);
    }
    else
    {
        fbe_sata_physical_drive_byteswap_copy(drive_info->product_rev, identify_data + FBE_SATA_FIRMWARE_REVISION_OFFSET + 4, FBE_SCSI_INQUIRY_REVISION_SIZE);
    }

    //Read in PIO & UDMA modes
    if((identify_data[FBE_SATA_PIO_MODE_OFFSET] & FBE_SATA_PIO_MODE_4_BIT_MASK))
    {
        sata_physical_drive->sata_drive_info.PIOMode = FBE_SATA_TRANSFER_PIO_4;
    }
    else if ((identify_data[FBE_SATA_PIO_MODE_OFFSET] & FBE_SATA_PIO_MODE_3_BIT_MASK))
    {
        sata_physical_drive->sata_drive_info.PIOMode = FBE_SATA_TRANSFER_PIO_3;
    }
    else
    {
        drive_status = FBE_SATA_DRIVE_STATUS_PIO_MODE_UNSUPPORTED;
    }

    //Only support modes 6-2 at this time.
    if((identify_data[FBE_SATA_UDMA_MODE_OFFSET] & FBE_SATA_UDMA_MODE_6_BIT_MASK))
    {
        sata_physical_drive->sata_drive_info.UDMAMode = FBE_SATA_TRANSFER_UDMA_6;
    }
    else if((identify_data[FBE_SATA_UDMA_MODE_OFFSET] & FBE_SATA_UDMA_MODE_5_BIT_MASK))
    {
        sata_physical_drive->sata_drive_info.UDMAMode = FBE_SATA_TRANSFER_UDMA_5;
    }
    else if((identify_data[FBE_SATA_UDMA_MODE_OFFSET] & FBE_SATA_UDMA_MODE_4_BIT_MASK))
    {
        sata_physical_drive->sata_drive_info.UDMAMode = FBE_SATA_TRANSFER_UDMA_4;
    }
    else if((identify_data[FBE_SATA_UDMA_MODE_OFFSET] & FBE_SATA_UDMA_MODE_3_BIT_MASK))
    {
        sata_physical_drive->sata_drive_info.UDMAMode = FBE_SATA_TRANSFER_UDMA_3;
    }    
    else if((identify_data[FBE_SATA_UDMA_MODE_OFFSET] & FBE_SATA_UDMA_MODE_2_BIT_MASK))
    {
        sata_physical_drive->sata_drive_info.UDMAMode = FBE_SATA_TRANSFER_UDMA_2;
    } 
    else
    {
        drive_status = FBE_SATA_DRIVE_STATUS_UDMA_MODE_UNSUPPORTED;
    }

    //If Write Cache is enabled set flush and disable conditions
    if ((identify_data[FBE_SATA_ENABLED_COMMANDS1] & FBE_SATA_WCE_BIT_MASK))
    {
        status = fbe_lifecycle_set_cond(&fbe_sata_physical_drive_lifecycle_const, 
                                        (fbe_base_object_t*)sata_physical_drive, 
                                        FBE_SATA_PHYSICAL_DRIVE_LIFECYCLE_COND_FLUSH_WRITE_CACHE);

        if (status != FBE_STATUS_OK) 
        {
            fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive, 
                                    FBE_TRACE_LEVEL_ERROR,
                                    "%s can't set Flush Write Cache condition, status: 0x%X\n",
                                    __FUNCTION__, status);
        }
        
        status = fbe_lifecycle_set_cond(&fbe_sata_physical_drive_lifecycle_const, 
                                        (fbe_base_object_t*)sata_physical_drive, 
                                        FBE_SATA_PHYSICAL_DRIVE_LIFECYCLE_COND_DISABLE_WRITE_CACHE);

        if (status != FBE_STATUS_OK) 
        {
            fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive, 
                                    FBE_TRACE_LEVEL_ERROR,
                                    "%s can't set Disable Write Cache condition, status: 0x%X\n",
                                    __FUNCTION__, status);
        }
    }

    //SCT commands cause problems this needs to be debugged then the code can be turned on.

    ////Read in SCT support Byte
    //sata_physical_drive->sata_drive_info.sct_support = identify_data[FBE_SATA_SCT_SUPPORT_OFFSET];

    ////Make Sure both the SCT feature set and Error Recovery Control are supported.  
    //if ((sata_physical_drive->sata_drive_info.sct_support & FBE_SATA_SCT_BIT0) && 
    //    (sata_physical_drive->sata_drive_info.sct_support & FBE_SATA_SCT_BIT3))
    //{
    //    status = fbe_lifecycle_set_cond(&fbe_sata_physical_drive_lifecycle_const, 
    //                                    (fbe_base_object_t*)sata_physical_drive, 
    //                                    FBE_SATA_PHYSICAL_DRIVE_LIFECYCLE_COND_SCT_SET_READ_TIMER);

    //    if (status != FBE_STATUS_OK) 
    //    {
    //        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive, 
    //                                FBE_TRACE_LEVEL_ERROR,
    //                                "%s can't set SCT Read Timer condition, status: 0x%X\n",
    //                                __FUNCTION__, status);
    //    }

    //    status = fbe_lifecycle_set_cond(&fbe_sata_physical_drive_lifecycle_const, 
    //                                    (fbe_base_object_t*)sata_physical_drive, 
    //                                    FBE_SATA_PHYSICAL_DRIVE_LIFECYCLE_COND_SCT_SET_WRITE_TIMER);

    //    if (status != FBE_STATUS_OK) 
    //    {
    //        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive, 
    //                                FBE_TRACE_LEVEL_ERROR,
    //                                "%s can't set SCT Write Timer condition, status: 0x%X\n",
    //                                __FUNCTION__, status);
    //    }
    //}

    return drive_status;    
}

fbe_sata_drive_status_t 
fbe_sata_physical_drive_parse_inscription(fbe_sata_physical_drive_t * sata_physical_drive, 
                                          fbe_u8_t *inscription_data,
                                          fbe_physical_drive_information_t * drive_info)
{
    fbe_sata_drive_status_t drive_status = FBE_SATA_DRIVE_STATUS_OK;
    fbe_u8_t signature[FBE_SATA_DD_BLOCK_SIGNATURE_SIZE+1];
    
    fbe_zero_memory(&signature, FBE_SATA_DD_BLOCK_SIGNATURE_SIZE + 1);
    fbe_copy_memory (signature, inscription_data, FBE_SATA_DD_BLOCK_SIGNATURE_SIZE);

    if (strcmp(signature, FBE_SATA_EMC_SIGNATURE))
    {                
        //This is not EMC block
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                "%s Inscription Signature not Valid! \n",
                                __FUNCTION__);
        
        sata_physical_drive->sata_drive_info.is_inscribed = FBE_FALSE;

        drive_status = FBE_SATA_DRIVE_STATUS_INVALID_INSCRIPTION;
    }
    else
    {  
        sata_physical_drive->sata_drive_info.is_inscribed = FBE_TRUE; 
        
        //drive_info->block_count--;
        drive_info->block_count = sata_physical_drive->sata_drive_info.native_capacity;
        drive_status = fbe_sata_physical_drive_set_dev_info_from_inscription(sata_physical_drive, 
                                                              &inscription_data[FBE_SATA_DD_BLOCK_INQUIRY_DATA_OFFSET],
                                                              drive_info);
    }

    return drive_status;
}

fbe_sata_drive_status_t 
fbe_sata_physical_drive_parse_read_log_extended(fbe_sata_physical_drive_t * sata_physical_drive,
                                                fbe_payload_fis_operation_t * fis_operation,
                                                fbe_payload_ex_t * payload)
{
    fbe_lba_t bad_lba;
    fbe_sata_drive_status_t drive_status = FBE_SATA_DRIVE_STATUS_OK;
	fbe_payload_block_operation_t * block_operation = NULL;

    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sata_physical_drive,
                                FBE_TRACE_LEVEL_WARNING,
                                "%s error 0x%x\n",
                                __FUNCTION__, fis_operation->response_buffer[FBE_SATA_RESPONSE_FIS_ERROR_OFFSET]);

	block_operation = fbe_payload_ex_get_prev_block_operation(payload);

    switch(fis_operation->response_buffer[FBE_SATA_RESPONSE_FIS_ERROR_OFFSET])
    {
        case FBE_SATA_RESPONSE_FIS_ERROR_UNC:

            if(fis_operation->fis[FBE_SATA_COMMAND_FIS_COMMAND_OFFSET] == FBE_SATA_READ_FPDMA_QUEUED)
            {
                bad_lba = (fbe_lba_t) fis_operation->response_buffer[FBE_SATA_RESPONSE_FIS_LBA_LOW_OFFSET];
                bad_lba += (fbe_lba_t)fis_operation->response_buffer[FBE_SATA_RESPONSE_FIS_LBA_MID_OFFSET] << 8;
                bad_lba += (fbe_lba_t)fis_operation->response_buffer[FBE_SATA_RESPONSE_FIS_LBA_HIGH_OFFSET] << 16;
                bad_lba += (fbe_lba_t)fis_operation->response_buffer[FBE_SATA_RESPONSE_FIS_LBA_LOW_EXT_OFFSET] << 24;
                bad_lba += (fbe_lba_t)fis_operation->response_buffer[FBE_SATA_RESPONSE_FIS_LBA_MID_EXT_OFFSET] << 32;
                bad_lba += (fbe_lba_t)fis_operation->response_buffer[FBE_SATA_RESPONSE_FIS_LBA_HIGH_EXT_OFFSET] << 40;
                
                fbe_payload_ex_set_scsi_error_code (payload, FBE_SCSI_CC_HARD_BAD_BLOCK);
                fbe_payload_ex_set_media_error_lba(payload, bad_lba);
                fbe_payload_block_set_status(block_operation,   FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR,
                                                                FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_DATA_LOST);
            }
            else
            {
                fbe_base_physical_drive_set_retry_msecs (payload, FBE_SCSI_INVALIDREQUEST);
                fbe_payload_block_set_status(block_operation,   FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED,
                                                                FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_POSSIBLE);
            }
            break;

        case FBE_SATA_RESPONSE_FIS_ERROR_ICRC:
        case FBE_SATA_RESPONSE_FIS_ERROR_ICRC_ABRT:
            fbe_base_physical_drive_set_retry_msecs (payload, FBE_SCSI_CC_HARDWARE_ERROR);
            fbe_payload_block_set_status(block_operation,   FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED,
                                                            FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_POSSIBLE);
            break;
        case FBE_SATA_RESPONSE_FIS_ERROR_IDNF:
        case FBE_SATA_RESPONSE_FIS_ERROR_UNC_IDNF_ABRT:
            fbe_base_physical_drive_set_retry_msecs (payload, FBE_SCSI_INVALIDREQUEST);
            fbe_payload_block_set_status(block_operation,   FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED,
                                                            FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_POSSIBLE);
            break;
        case FBE_SATA_RESPONSE_FIS_ERROR_ABRT:
            fbe_base_physical_drive_set_retry_msecs (payload, FBE_SCSI_DRVABORT);
            fbe_payload_block_set_status(block_operation,   FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED,
                                                            FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_POSSIBLE);
            break;

        default:
            
            //May want to panic here eventually?
            //USe SCSI_UNSPPORTED_CONDITION error code
            fbe_base_physical_drive_set_retry_msecs (payload, FBE_SCSI_UNKNOWNRESPONSE);
            fbe_payload_block_set_status(block_operation,   FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED,
                                                            FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_POSSIBLE);
            break;

    }

    return drive_status;
}
