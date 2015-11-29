/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved. Licensed material -- property of EMC
 * Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_event_log_esp_msgs.c
 ***************************************************************************
 *
 * @brief
 *  This file contain event log message string for SEP package.
 * 
 * @version
 *  28-June-2010: Vaibhav Gaonkar Created.
 *
 ***************************************************************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_event_log_utils.h"


/*********************************************************************/
/*                  loacal functions                                 */
/*********************************************************************/
static fbe_status_t fbe_event_log_sep_get_info_generic_msg(fbe_u32_t msg_id, 
                                                           fbe_u8_t* msg,
                                                           fbe_u32_t msg_len);
static fbe_status_t fbe_event_log_sep_get_warn_generic_msg(fbe_u32_t msg_id, 
                                                           fbe_u8_t *msg,
                                                           fbe_u32_t msg_len);
static fbe_status_t fbe_event_log_sep_get_error_generic_msg(fbe_u32_t msg_id, 
                                                            fbe_u8_t *msg,
                                                            fbe_u32_t msg_len);
static fbe_status_t fbe_event_log_sep_get_crit_generic_msg(fbe_u32_t msg_id, 
                                                           fbe_u8_t *msg,
                                                           fbe_u32_t msg_len);

/*!***************************************************************
 * @fn fbe_event_log_get_sep_msg(fbe_u32_t msg_id, 
 *                               fbe_u8_t *msg)
 ****************************************************************
 * @brief
 *  This function directs the appropriate function depend 
 *  upon SEP message severity type.
 *
 * @param msg Buffer to store message string
 * @param msg_id Message ID
 * @param msg_len Length of buffer to store message string
 * 
 * @return fbe_status_t
 *
 * @author
 *  28-June-2010: Vaibhav Gaonkar Created.
 *
 ****************************************************************/
fbe_status_t 
fbe_event_log_get_sep_msg(fbe_u32_t msg_id, fbe_u8_t *msg, fbe_u32_t msg_len)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t mask_bit = 0xFFFFF000;

    if((msg_id & mask_bit) == SEP_INFO_GENERIC)
    {
        status = fbe_event_log_sep_get_info_generic_msg(msg_id, msg, msg_len);
    }
    else if((msg_id & mask_bit) == SEP_WARN_GENERIC)
    {
        status = fbe_event_log_sep_get_warn_generic_msg(msg_id, msg, msg_len);
    }
    else if((msg_id & mask_bit) == SEP_ERROR_GENERIC)
    {
        status = fbe_event_log_sep_get_error_generic_msg(msg_id, msg, msg_len);
    }
    else if((msg_id & mask_bit) == SEP_CRIT_GENERIC)
    {
        status = fbe_event_log_sep_get_crit_generic_msg(msg_id, msg, msg_len);
    }

    return status; 
}
/***************************************
*   end of fbe_event_log_get_sep_msg()  
****************************************/
/*!***************************************************************
 * @fn fbe_event_log_sep_get_info_generic_msg(fbe_u32_t msg_id, 
                                              fbe_u8_t *msg,
                                              fbe_u32_t msg_len)
 ****************************************************************
 * @brief
 *  This function return event log message string for SEP of
 *  SEP_INFO_GENERIC type
 *
 * @param msg_id Message ID
 * @param msg Buffer to store message string
 * 
 * @return fbe_status_t
 *
 * @author
 *  28-June-2010: Vaibhav Gaonkar Created.
 *
 ****************************************************************/
static fbe_status_t  
fbe_event_log_sep_get_info_generic_msg(fbe_u32_t msg_id, 
                                       fbe_u8_t *msg,
                                       fbe_u32_t msg_len)
{
    fbe_u8_t *full_msg;
    
    /* Add new switch case statement for every new SEP_INFO_GENERIC message 
     * that added in SepMessages.mc file, along with the appropriate format 
     * specifier.
     */
     switch(msg_id)
    {
/*
        case SEP_INFO_LOAD_VERSION:
            full_msg ="Driver compiled for %s.\n";
        break;
 */    
        case SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED:
            full_msg = "Sector Reconstructed RG: 0x%x Pos: 0x%x LBA: 0x%I64x Blocks: 0x%x Err info: 0x%x Extra info: 0x%x";
            break;
       
        case SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED:
            full_msg = "Parity Sector Reconstructed RG: 0x%x Pos: 0x%x LBA: 0x%I64x Blocks: 0x%x Err info: 0x%x Extra info: 0x%x";
            break;

        case SEP_INFO_RAID_HOST_CORRECTING_WITH_NEW_DATA:
            full_msg = "Sector Reconstructed with new data RG: 0x%x Pos: 0x%x LBA: 0x%I64x Blocks: 0x%x Err info: 0x%x Extra info: 0x%x";
            break;

        case SEP_INFO_RAID_PARITY_WRITE_LOG_FLUSH_ABANDONED:
            full_msg = "Write log flush abandoned for RG: 0x%x Pos bitmap: 0x%x LBA: 0x%I64x Blocks: 0x%x Err info: 0x%x Extra info: 0x%x Slot: 0x%x Time: 0x%I64x %x";
            break;

         case SEP_INFO_CODE_RAID_EXPECTED_COHERENCY_ERROR:
             full_msg = "Expected Coherency Error RG: 0x%x Pos: 0x%x LBA: 0x%I64x Blocks: 0x%x Err info: 0x%x Extra info: 0x%x";
             break;

        //  Start of rebuild message codes 
        case SEP_INFO_RAID_OBJECT_RAID_GROUP_REBUILD_STARTED:
            full_msg = "Rebuild for RAID group %d (obj 0x%x) position %d to disk %d_%d_%d started";
            break;

        case SEP_INFO_RAID_OBJECT_RAID_GROUP_REBUILD_COMPLETED:
            full_msg = "Rebuild of all LUNs in RAID group %d (obj 0x%x) position %d to disk %d_%d_%d finished";
            break;

        case SEP_INFO_RAID_OBJECT_RAID_GROUP_COPY_STARTED:
            full_msg = "Copy from disk %d_%d_%d to disk %d_%d_%d started"; 
            break;

        case SEP_INFO_RAID_OBJECT_RAID_GROUP_COPY_COMPLETED:
            full_msg = "Copy of all LUNs/all RAID groups from disk %d_%d_%d to disk %d_%d_%d finished"; 
            break;
        //  End of rebuild message codes 

        //  Start of BV message codes
        case SEP_INFO_RAID_OBJECT_USER_RW_BCKGRND_VERIFY_STARTED:
            full_msg = "User initiated verify for Flare LUN %d (obj 0x%x) started";
            break;

        case SEP_INFO_RAID_OBJECT_USER_RO_BCKGRND_VERIFY_STARTED:
            full_msg = "User initiated read only verify for Flare LUN %d (obj 0x%x) started";
            break;

        case SEP_INFO_RAID_OBJECT_LU_AUTOMATIC_VERIFY_STARTED:
            full_msg = "LU automatic verify for Flare LUN %d (obj 0x%x) started";
            break;

         case SEP_INFO_RAID_OBJECT_LU_AUTOMATIC_VERIFY_COMPLETE:
            full_msg = "LU automatic verify for Flare LUN %d (obj 0x%x) complete";
            break;

        case SEP_INFO_USER_BCKGRND_VERIFY_COMPLETED:
            full_msg = "User initiated verify for Flare LUN %d (obj 0x%x) finished";
            break;

        case SEP_INFO_USER_RO_BCKGRND_VERIFY_COMPLETED:
            full_msg = "User initiated read only verify for Flare LUN %d (obj 0x%x) finished";
            break;
        // End of BV message codes

        /*  Start of LUN Zeroing message codes */
        case SEP_INFO_LUN_OBJECT_LUN_ZEROING_STARTED:
            full_msg = "Zeroing of Flare LUN %d (obj 0x%x) started";
            break;

        case SEP_INFO_LUN_OBJECT_LUN_ZEROING_COMPLETED:
            full_msg = "Zeroing of Flare LUN %d (obj 0x%x) finished";
            break;
        /*  End of LUN Zeroing message codes */

        /*  Start of Disk Zeroing message codes */
        case SEP_INFO_PROVISION_DRIVE_OBJECT_DISK_ZEROING_STARTED:
            full_msg = "Zeroing of disk %d_%d_%d(obj 0x%x) started";
            break;

        case SEP_INFO_PROVISION_DRIVE_OBJECT_DISK_ZEROING_COMPLETED:
            full_msg = "Zeroing of disk %d_%d_%d(obj 0x%x) finished";
            break;
        /*  End of Disk Zeroing message codes */

        /*  Start of rebuild logging message codes */
        case SEP_INFO_RAID_OBJECT_DISK_RB_LOGGING_STARTED:
             full_msg = "Rebuild Logging started on disk position %d in RG %d (obj 0x%x)";
             break;

        case SEP_INFO_RAID_OBJECT_DISK_RB_LOGGING_STOPPED:
             full_msg = "Rebuild Logging stopped on position %d in RG %d (obj 0x%x)";
             break;
        /*   End of rebuild logging message codes */

        //  Start of Permanent spare and Proactive spare message codes 

        case SEP_INFO_SPARE_PERMANENT_SPARE_SWAP_IN_OPERATION_INITIATED:
            full_msg = "Permanent Spare Swap-in operation is initiated.Permanent disk:%d_%d_%d,Failed disk:%d_%d_%d";
            break;

        case SEP_INFO_SPARE_PERMANENT_SPARE_SWAPPED_IN:
            full_msg = "Permanent spare swapped-in.Disk %d_%d_%d,serial num:%s is permanently replacing disk %d_%d_%d,serial num:%s";
            break;

        case SEP_INFO_SPARE_PROACTIVE_SPARE_SWAP_IN_OPERATION_INITIATED:
            full_msg ="Proactive Spare Swap-in operation is initiated.Permanent disk:%d_%d_%d,Failed disk:%d_%d_%d";
            break;

        case SEP_INFO_SPARE_PROACTIVE_SPARE_DRIVE_SWAPPED_IN:
            full_msg = "Proactive spare swapped-in.Disk %d_%d_%d,serial num:%s is permanently replacing disk %d_%d_%d,serial num:%s";
            break;

        case SEP_INFO_SPARE_DRIVE_SWAP_OUT:
            full_msg = "Copy operation swapped out disk:%d_%d_%d";
            break;

        case SEP_INFO_SPARE_COPY_OPERATION_COMPLETED:
            full_msg = "Copy operation completed.Permanent disk:%d_%d_%d,Original disk:%d_%d_%d";
            break;

        case SEP_INFO_SPARE_USER_COPY_INITIATED:
            full_msg = "User copy is initiated.Destination disk:%d_%d_%d,Original disk:%d_%d_%d";
            break;

        case SEP_INFO_SPARE_USER_COPY_DESTINATION_DRIVE_SWAPPED_IN:
            full_msg = "User copy swapped-in.Disk %d_%d_%d,serial num:%s is permanently replacing disk %d_%d_%d,serial num:%s";
            break;
 
         case SEP_INFO_SPARE_PERMANENT_SPARE_REQUEST_DENIED:
            full_msg = "Permanent spare request for %d_%d_%d,serial num:%s denied. RAID Group unconsumed, broken or non-redundant";
            break;
          
         case SEP_INFO_SPARE_PROACTIVE_SPARE_REQUEST_DENIED:
             full_msg = "Proactive copy request for %d_%d_%d,serial num:%s denied. RAID Group unconsumed, degraded or non-redundant";
             break;
                     
         case SEP_INFO_SPARE_USER_COPY_REQUEST_DENIED:
             full_msg = "User copy request for %d_%d_%d,serial num:%s denied. RAID Group unconsumed, degraded or non-redundant";
             break; 

         case SEP_INFO_SPARE_PROACTIVE_COPY_NO_LONGER_REQUIRED:
             full_msg = "Disk %d_%d_%d,serial num:%s. No longer requires proactive copy";
             break;

         case SEP_INFO_SPARE_PERMANENT_SPARE_NO_LONGER_REQUIRED:
             full_msg = "Disk %d_%d_%d,serial num:%s has been re-inserted.  Permanent spare not required";
             break;

         case SEP_INFO_SPARE_USER_COPY_SPARE_DRIVE_SWAPPED_IN:
             full_msg = "User copy - spare swapped-in.Disk %d_%d_%d,serial num:%s is permanently replacing disk %d_%d_%d,serial num:%s";
             break;

    //  End of Permanent spare and Proactive spare message codes 
    
    // Start of sniff-verify messages codes
        case SEP_INFO_SNIFF_VERIFY_ENABLED:
            full_msg = "Sniff verify enabled on obj 0x%0x by user. Disk Info:%d_%d_%d";
            break;
        case SEP_INFO_SNIFF_VERIFY_DISABLED:
            full_msg = "Sniff verify disabled on obj 0x%0x by user. Disk Info:%d_%d_%d";
            break;
            
    // End of sniff-verify messages codes

         /* Message codes for LUN creation & destroy */
        case SEP_INFO_LUN_CREATED:
             full_msg = "Lun Created with obj id 0x%0x and Flare LUN %d ";
             break;

        case SEP_INFO_LUN_DESTROYED:
             full_msg = "Lun Destroyed with Flare LUN %d";
             break;

         /* Message codes for raid group creation & destroy */
        case SEP_INFO_RAID_GROUP_CREATED:
             full_msg = "Raid group created with obj ID 0x%0x and RG Number %d";
             break;

        case SEP_INFO_RAID_GROUP_DESTROYED:
             full_msg = "Raid group destroyed with obj ID 0x%0x and RG number %d";
             break;            

         case SEP_INFO_LUN_CREATED_FBE_CLI:
            full_msg = "LUN (object ID 0x%0x) is create with Fbecli.exe with status %d";
            break;

         case SEP_INFO_LUN_DESTROYED_FBE_CLI:
            full_msg = "LUN (object ID 0x%0x) is destroyed with Fbecli.exe with status %d";
            break;

         /* Start of Extended Media Error Handling (EMEH) Messages */
         case SEP_INFO_RAID_OBJECT_RAID_GROUP_ERROR_THRESHOLDS_DISABLED:
             full_msg = "Error Thresholds for RAID group %d (obj 0x%x) position mask 0x%x have been disabled";
             break;

         case SEP_INFO_RAID_OBJECT_RAID_GROUP_ERROR_THRESHOLDS_RESTORED:
             full_msg = "Error Thresholds for RAID group %d (obj 0x%x) position mask 0x%x have been restored";
             break;

         case SEP_INFO_RAID_OBJECT_PROACTIVE_SPARE_ERROR_THRESHOLDS_DISABLED:
             full_msg = "Proactive spare RAID group %d (obj 0x%x) position %d error thresholds for position mask 0x%x have been disabled";
             break;

         case SEP_INFO_RAID_OBJECT_PROACTIVE_SPARE_ERROR_THRESHOLDS_RESTORED:
             full_msg = "Proactive spare RAID group %d (obj 0x%x) position %d error thresholds for position mask 0x%x have been restored";
             break;

         case SEP_INFO_RAID_OBJECT_PROACTIVE_SPARE_RESILIENCY_DISABLED:
             full_msg = "Proactive spare Resiliency has been disabled persist %d";
             break;

         case SEP_INFO_RAID_OBJECT_PROACTIVE_SPARE_RESILIENCY_ENABLED:
             full_msg = "Proactive spare Resiliency has been enabled persist %d";
             break;
         /* End of Extended Media Error Handling (EMEH) Messages */
		 case SEP_INFO_SYSTEM_DISK_END_OF_LIFE_CLEAR:
			full_msg = "Disk in slot 0_0_%d EOL state has been cleared, serial num:%s ";
			break;
		 case SEP_INFO_USER_DISK_END_OF_LIFE_CLEAR:
		 	full_msg = "Disk in slot %d_%d_%d EOL state has been cleared, serial num:%s ";
			break;
		 case SEP_INFO_SYSTEM_DISK_DRIVE_FAULT_CLEAR:
		 	full_msg = "Disk in slot 0_0_%d Drive Fault has been cleared, serial num:%s ";
			break;
		 case SEP_INFO_USER_DISK_DRIVE_FAULT_CLEAR:
		 	full_msg = "Disk in slot %d_%d_%d Drive Fault state has been cleared, serial num:%s";
			break;
        default:
            return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_copy_memory(msg, full_msg,  msg_len);
    return FBE_STATUS_OK;
}
/****************************************************
*   end of fbe_event_log_sep_get_info_generic_msg()  
****************************************************/
/*!***************************************************************
 * @fn fbe_event_log_sep_get_warn_generic_msg(fbe_u32_t msg_id, 
                                              fbe_u8_t *msg,
                                              fbe_u32_t msg_len)
 ****************************************************************
 * @brief
 *  This function return event log message string for SEP of
 *  SEP_WARN_GENERIC type
 *
 * @param msg_id Message ID
 * @param msg Buffer to store message string
 * @param msg_len Length of buffer to store message string
 * 
 * @return fbe_status_t
 *
 * @author
 *  28-June-2010: Vaibhav Gaonkar Created.
 *
 ****************************************************************/
static fbe_status_t 
fbe_event_log_sep_get_warn_generic_msg(fbe_u32_t msg_id, 
                                       fbe_u8_t *msg,
                                       fbe_u32_t msg_len)
{
    fbe_u8_t *full_msg;
    
    /* Add new switch case statement for every new SEP_INFO_GENERIC message 
     * that added in SepMessages.mc file, along with the appropriate format 
     * specifier.
     */
    switch(msg_id)
    {
        case SEP_WARN_SPARE_NO_SPARES_AVAILABLE:
            full_msg = "There are no spares in the system to replace failed disk:%d_%d_%d,serial num:%s";
            break;
       
        case SEP_WARN_SPARE_NO_SUITABLE_SPARE_AVAILABLE:
            full_msg = "There is no suitable spare to replace failed disk:%d_%d_%d,serial num:%s";
            break;

        case SEP_WARN_SYSTEM_DISK_NEED_OPERATION:
            full_msg = "Disk in slot 0_0_%d is %s may not be consumed. Use fbecli can force it online if it could not be online.";
            break;

        case SEP_WARN_SYSTEM_DISK_NEED_REPLACEMENT:
            full_msg = "Disk in slot 0_0_%d needs to be replaced, serial num:%s";
            break;

        case SEP_WARN_USER_DISK_END_OF_LIFE:
            full_msg = "Disk in slot %d_%d_%d is in EOL state, serial num:%s";
            break;

        case SEP_WARN_SYSTEM_DISK_DRIVE_FAULT:
            full_msg = "Disk in slot 0_0_%d is drive fault and needs to be replaced, serial num:%s";
            break;

        case SEP_WARN_USER_DISK_DRIVE_FAULT:
            full_msg = "Disk in slot %d_%d_%d is in Drive Fault state, serial num:%s ";
            break;
        case SEP_WARN_TWO_OF_THE_FIRST_THREE_SYSTEM_DRIVES_FAILED:
            full_msg = "Two of the first three system drives are failed at booting time";
            break;
            
        default:
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_copy_memory(msg, full_msg,  msg_len);
    return FBE_STATUS_OK;
}
/****************************************************
*   end of fbe_event_log_sep_get_warn_generic_msg()  
****************************************************/
/*!***************************************************************
 * @fn fbe_event_log_sep_get_error_generic_msg(fbe_u32_t msg_id, 
 *                                             fbe_u8_t *msg,
 *                                             fbe_u32_t msg_len)
 ****************************************************************
 * @brief
 *  This function return event log message string for SEP of
 *  SEP_ERROR_GENERIC type
 *
 * @param msg_id Message ID
 * @param msg Buffer to store message string
 * @param msg_len Length of buffer to store message string
 * 
 * @return fbe_status_t
 *
 * @author
 *  28-June-2010: Vaibhav Gaonkar Created.
 *
 ****************************************************************/
static fbe_status_t 
fbe_event_log_sep_get_error_generic_msg(fbe_u32_t msg_id, 
                                        fbe_u8_t *msg,
                                        fbe_u32_t msg_len)
{
    fbe_u8_t *full_msg;

    /* Add new switch case statement for every new SEP_INFO_GENERIC message 
     * that added in SepMessages.mc file, along with the appropriate format 
     * specifier.
     */
    switch(msg_id)
    {
        case SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED:
            full_msg = "Data Sector Invalidated RAID Group: 0x%x Position: 0x%x LBA: 0x%I64x Blocks: 0x%x Error info: 0x%x Extra info: 0x%x";
            break;

        case SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_DATA_SECTOR: 
            full_msg = "Uncorrectable Data Sector RAID Group: 0x%x Position: 0x%x LBA: 0x%I64x Blocks: 0x%x Error info: 0x%x Extra info: 0x%x";
            break;

        case SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_PARITY_SECTOR:
            full_msg = "Uncorrectable Parity Sector RAID Group: 0x%x Position: 0x%x LBA: 0x%I64x Blocks: 0x%x Error info: 0x%x Extra info: 0x%x";
            break;

        case SEP_ERROR_CODE_RAID_HOST_PARITY_INVALIDATED:
            full_msg = "Parity Invalidated RAID Group: 0x%x Position: 0x%x LBA: 0x%I64x Blocks: 0x%x Error info: 0x%x Extra info: 0x%x";
            break;

        case SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR:
            full_msg = "Uncorrectable Sector RAID Group: 0x%x Position: 0x%x LBA: 0x%I64x Blocks: 0x%x Error info: 0x%x Extra info: 0x%x";
            break;

        case SEP_ERROR_RAID_GROUP_METADATA_RECONSTRUCT_REQUIRED:
            full_msg = "RAID group %d obj %d Raid type RAID-%d metadata reconstruction required";
            break;

        case SEP_ERROR_CODE_DB_DRIVES_DISORDERED_ERROR:
            full_msg = "The System Drives are out of order. Disk0 should be in slot%d, disk1 in slot%d, disk2 in slot%d, disk3 in slot%d";
            break;

        case SEP_ERROR_CODE_INTEGRITY_BROKEN_ERROR:
            full_msg = "System integrity broken:0_0_0: %s, 0_0_1: %s, 0_0_2: %s, 0_0_3: %s.";
            break;
        
        case SEP_ERROR_CODE_CHASSIS_MISMATCHED_ERROR:
            full_msg = "Chassis is mismatched with system drives. If you want to replace chassis, please set the Chassis Replacement Flag.";
            break;

        case SEP_ERROR_CODE_OPERATION_ON_WWNSEED_CHAOS:
            full_msg = "usermodifiedWwnSeedFlag is set through unisphere, meantime, chassis_replacement_movement flag is set through fbecli.";
            break;
            
        case SEP_ERROR_CODE_SPARE_UNEXPECTED_ERROR:
            full_msg = "An unexpected error(%d) occurred when attempting to replace failed disk:%d_%d_%d";
            break;

         case SEP_ERROR_CODE_LUN_UNEXPECTED_ERRORS:
             full_msg = "Flare LUN %d (Obj %x) encountered unexpected errors";
             break;

		case SEP_ERROR_CODE_NEWER_CONFIG_ON_DISK:
            full_msg = "Database configuration entry size(size: 0x%x) loaded from disk is larger than the entry size(size: 0x%x) in software.";
			break;
		case SEP_ERROR_CODE_NEWER_CONFIG_FROM_PEER:
            full_msg = "Database config(size: 0x%x) received from Peer is larger than the config(size: 0x%x) in software";
			break;
		case SEP_ERROR_CODE_EMV_MISMATCH_WITH_PEER:
            full_msg = "Local expected memory value (0x%x) mismatches with Peer Shared expected memory value(0x%x).";
			break;
		case SEP_ERROR_CODE_MEMORY_UPGRADE_FAIL:
            full_msg = "After upgrade, the memory(0x%x) is neither the source memory size(0x%x) nor the target memory size(0x%x).";
			break;
		case SEP_ERROR_CODE_SYSTEM_DB_HEADER_CORRUPTED:
			full_msg = "The system DB header is corrupted.";
			break;
		case SEP_ERROR_CODE_SYSTEM_DB_CONTENT_CORRUPTED:
			full_msg = "The system db content is corrupted.";
			break;
		case SEP_ERROR_CODE_SYSTEM_DB_HEADER_RW_ERROR:
			full_msg = "Read/Write system DB header fail.";
			break;
		case SEP_ERROR_CODE_SYSTEM_DB_CONTENT_RW_ERROR:
			full_msg = "Read/Write system db content fail.";
			break;
		case SEP_ERROR_CODE_TOO_SMALL_DB_DRIVE:
			full_msg = "%d Database drive(s) is too small to create the RAID defined in PSL. Use a drive equal to other system drives.";
			break;

        case SEP_ERROR_CODE_COPY_SOURCE_DRIVE_REMOVED:
            full_msg = "Copy operation failed because source disk:%d_%d_%d was removed";
            break;
        case SEP_ERROR_CODE_COPY_DESTINATION_DRIVE_REMOVED:
            full_msg = "Copy operation failed because destination disk:%d_%d_%d was removed.";
            break;
        case SEP_ERROR_CODE_SPARE_UNEXPECTED_COMMAND:
            full_msg = "Unexpected command(%d) for spare operation to disk:%d_%d_%d";
            break;
        case SEP_ERROR_CODE_COPY_SOURCE_DRIVE_INVALID:
            full_msg = "The source disk:%d_%d_%d is not part of a raid group. Make sure you chose the correct source disk.";
            break;
        case SEP_ERROR_CODE_SPARE_SOURCE_IN_UNEXPECTED_STATE:
            full_msg = "Source disk:%d_%d_%d is in an unexpected state. Remove and re-insert the source disk.";
            break;
        case SEP_ERROR_CODE_SPARE_DRIVE_VALIDATION_FAILED:
            full_msg = "Validation of replacement disk for original disk:%d_%d_%d failed. Choose a different destination disk.";
            break;
        case SEP_ERROR_CODE_COPY_DESTINATION_INCOMPATIBLE_TIER:
            full_msg = "Destination disk:%d_%d_%d is not compatible with original disk:%d_%d_%d. Choose a different destination disk.";
            break;
        case SEP_ERROR_CODE_SPARE_OPERATION_IN_PROGRESS:
            full_msg = "A sparing or copy operation is already in progress on disk:%d_%d_%d. Wait until copy is completed.";
            break;
        case SEP_ERROR_CODE_SPARE_RAID_GROUP_IS_UNCONSUMED:
            full_msg = "Policy does not allow sparing or copy operation to an empty RAID group disk:%d_%d_%d";
            break;
        case SEP_ERROR_CODE_SPARE_RAID_GROUP_IS_NOT_REDUNDANT:
            full_msg = "Policy does not allow sparing or copy operation to a non-redundant RAID group disk:%d_%d_%d";
            break;
        case SEP_ERROR_CODE_SPARE_RAID_GROUP_IS_BROKEN:
            full_msg = "Policy does not allow a sparing or copy operation to a broken RAID group disk:%d_%d_%d";
            break;
        case SEP_ERROR_CODE_COPY_RAID_GROUP_DEGRADED:
            full_msg = "Policy does not allow copy operations to a degraded raid group disk:%d_%d_%d";
            break;
        case SEP_ERROR_CODE_SPARE_BLOCK_SIZE_MISMATCH:
            full_msg = "Destination disk:%d_%d_%d has a different block size from original:%d_%d_%d. Choose a compatible destination disk.";
            break;
        case SEP_ERROR_CODE_SPARE_INSUFFICIENT_CAPACITY:
            full_msg = "Destination disk:%d_%d_%d has less capacity than original:%d_%d_%d";
            break;
        case SEP_ERROR_CODE_SWAP_FAILED_RAID_GROUP_DESTROYED:
            full_msg = "The spare or copy operation to disk:%d_%d_%d failed because the RAID group no longer exists.";
            break;
        case SEP_ERROR_CODE_SWAP_FAILED_RAID_GROUP_DENIED:
            full_msg = "The spare or copy operation to disk:%d_%d_%d was denied by RAID group. Check if RAID group is degraded.";
            break;
        case SEP_ERROR_CODE_COPY_DESTINATION_OFFSET_MISMATCH:
            full_msg = "Destination disk:%d_%d_%d bind offset is greater than original:%d_%d_%d. Choose another destination disk.";
            break;
        case SEP_ERROR_CODE_BIND_USER_DRIVE_IN_SYSTEM_SLOT:
            full_msg = "The bind user disk with PVD id:%x in system slot:%d_%d_%d. Please pull out it and return to user drive slot.";
            break;
        case SEP_ERROR_CODE_SYSTEM_DRIVE_IN_WRONG_SLOT:
            full_msg = "The system disk (with PVD id:%x) is in the wrong slot:%d_%d_%d. It should be in:%d_%d_%d.";
            break;
        case SEP_ERROR_CODE_TWO_INVALID_SYSTEM_DRIVE_AND_ONE_MORE_OF_THEM_IN_USER_SLOT:
            full_msg = "Two system drives are invalid and at least one of them are in user slot.";
            break;
        case SEP_ERROR_CODE_RAID_GROUP_HAS_COPY_IN_PROGRESS:
            full_msg = "Policy does not allow a new copy to a RAID group with a copy in progress disk:%d_%d_%d";
            break;
        case SEP_ERROR_CODE_COPY_SOURCE_DRIVE_DEGRADED:
            full_msg = "Currently disk %d_%d_%d is degraded. Retry operation later";
            break;
        case SEP_ERROR_CODE_SPARE_COPY_OBJECT_NOT_READY:
            full_msg = "The copy object is not in the Ready state for disk:%d_%d_%d";
            break;
        case SEP_ERROR_CODE_SWAP_ABORT_COPY_REQUEST_DENIED:
            full_msg = "Attempt to abort copy operation to disk %d_%d_%d from disk %d_%d_%d denied";
            break;
        case SEP_ERROR_CODE_COPY_INVALID_DESTINATION_DRIVE:
            full_msg = "Copy operation failed because destination disk:%d_%d_%d cannot be found.";
            break;
        case SEP_ERROR_CODE_COPY_DESTINATION_DRIVE_NOT_HEALTHY:
            full_msg = "Copy operation failed because destination disk:%d_%d_%d is faulted";
            break;
        case SEP_ERROR_CODE_COPY_DESTINATION_DRIVE_IN_USE:
            full_msg = "Copy operation failed because destination disk:%d_%d_%d is part of a RAID group";
            break;
        case SEP_ERROR_CODE_DESTINATION_INCOMPATIBLE_RAID_GROUP_OFFSET:
            full_msg = "Copy operation failed because destination disk:%d_%d_%d has an incompatible bind offset";
            break;

        case SEP_ERROR_CREATE_RAID_GROUP_INVALID_DRIVE_COUNT:
            full_msg = "RAID type RAID-%d does not support a drive count of %d";
            break;
        case SEP_ERROR_CREATE_RAID_GROUP_INSUFFICIENT_DRIVE_CAPACITY:
            full_msg = "Drives selected do not have sufficient capacity for request";
            break;
        case SEP_ERROR_CREATE_RAID_GROUP_INVALID_RAID_TYPE:
            full_msg = "The RAID type: %d is not a supported raid type";
            break;
        case SEP_ERROR_CREATE_RAID_GROUP_INVALID_CONFIGURATION:
            full_msg = "Requested RAID type RAID-%d with drive count %d is not a valid configuration";
            break;
        case SEP_ERROR_CREATE_RAID_GROUP_INCOMPATIBLE_DRIVE_TYPES:
            full_msg = "One or more drives for RAID type RAID-%d with drive count %d are not compatible";
            break;
			
        case SEP_ERROR_CODE_NOT_ALL_DRIVES_SET_ICA_FLAGS:
            full_msg = "Not all drives are set ICA flags. (drive0,drive1,drive2)=>(%d, %d, %d) (1=flags is not valid;2=flags is set; 3=flags is not set).";
            break;
        case SEP_ERROR_CODE_INCOMPATIBLE_REPLACED_SYSTEM_DRIVE:
            full_msg = "New system drive %d_%d_%d is incompatible with original one. Please check its capacity or drive type";
            break;
			
        default:
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_copy_memory(msg, full_msg, msg_len);
    return FBE_STATUS_OK;
}
/****************************************************
*   end of fbe_event_log_sep_get_error_generic_msg()  
****************************************************/
/*!***************************************************************
 * @fn fbe_event_log_sep_get_crit_generic_msg(fbe_u32_t msg_id, 
                                              fbe_u8_t *msg,
                                              fbe_u32_t msg_len)
 ****************************************************************
 * @brief
 *  This function return event log message string for SEP of
 *  SEP_CRIT_GENERIC type
 *
 * @param msg_id Message ID
 * @param msg Buffer to store message string
 * 
 * @return fbe_status_t
 *
 * @author
 *  28-June-2010: Vaibhav Gaonkar Created.
 *
 ****************************************************************/
static fbe_status_t 
fbe_event_log_sep_get_crit_generic_msg(fbe_u32_t msg_id, 
                                       fbe_u8_t *msg,
                                       fbe_u32_t msg_len)
{
    fbe_u8_t *full_msg;

    /* Add new switch case statement for every new SEP_INFO_GENERIC message 
     * that added in SepMessages.mc file, along with the appropriate format 
     * specifier.
     */
    switch(msg_id)
    {
        case SEP_ERROR_CODE_RAID_HOST_DATA_CHECKSUM_ERROR:
            full_msg = "Checksum Error RAID Group: 0x%x Position: 0x%x LBA: 0x%I64x Blocks: 0x%x Error info: 0x%x Extra info: 0x%x";
            break;

        case SEP_ERROR_CODE_RAID_HOST_PARITY_CHECKSUM_ERROR:
            full_msg = "Parity Checksum Error RAID Group: 0x%x Position: 0x%x LBA: 0x%I64x Blocks: 0x%x Error info: 0x%x Extra info: 0x%x";
            break;

        case SEP_ERROR_CODE_RAID_HOST_COHERENCY_ERROR:
            full_msg = "Coherency Error RAID Group: 0x%x Position: 0x%x LBA: 0x%I64x Blocks: 0x%x Error info: 0x%x Extra info: 0x%x";
            break;

        case SEP_ERROR_CODE_RAID_HOST_LBA_STAMP_ERROR:
            full_msg = "LBA Stamp Error RAID Group: 0x%x Position: 0x%x LBA: 0x%I64x Blocks: 0x%x Error info: 0x%x Extra info: 0x%x";
            break;

        case SEP_ERROR_CODE_PROVISION_DRIVE_INCORRECT_KEY:
            full_msg = "Encryption Key Incorrect for disk: %d_%d_%d (object id: 0x%x) SN: %s";
            break;

        case SEP_ERROR_CODE_PROVISION_DRIVE_WEAR_LEVELING_THRESHOLD:
            full_msg = "Writes exceed writes per day limit for disk: %d_%d_%d (object id: 0x%x) SN: %s";
            break;

        default:
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_copy_memory(msg, full_msg,  msg_len);
    return FBE_STATUS_OK;
}
/****************************************************
*   end of fbe_event_log_sep_get_crit_generic_msg()  
****************************************************/

