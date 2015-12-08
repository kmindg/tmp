/***************************************************************************
 * Copyright (C) EMC Corporation 2012 
 * All rights reserved. Licensed material -- property of EMC 
 * Corporation 
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_base_physical_drive_error_handling.c
 ***************************************************************************
 *
 * @brief
 *  This file contains base physical drive error handling.
 * 
 *
 ***************************************************************************/

#include "fbe/fbe_winddk.h"
#include "base_object_private.h"
#include "base_physical_drive_private.h"
#include "fbe_scsi.h"
#include "fbe/fbe_atomic.h"
#include "fbe/fbe_event_log_api.h"
#include "PhysicalPackageMessages.h"

static const fbe_base_drive_error_translation_t     fbe_error_translation_table [] =
{
    { FBE_SCSI_CC_NOERR,                       DRV_EXT_NO_ERROR,                                     false,}, 
    { FBE_SCSI_BUSSHUTDOWN,                    DRV_EXT_SCSI_INVALIDSCB,                              true,},
    { FBE_SCSI_CC_DEV_RESET,                   DRV_EXT_DEV_RESET,                                    true,},
    { FBE_SCSI_SELECTIONTIMEOUT,               DRV_EXT_SELECTION_TIMEOUT,                            true,},
    { FBE_SCSI_DEVUNITIALIZED,                 DRV_EXT_SCSI_DEVUNITIALIZED,                          true,},
    { FBE_SCSI_AUTO_SENSE_FAILED,              DRV_EXT_REQ_SENSE_FAILED,                             true,},
    { FBE_SCSI_BADBUSPHASE,                    DRV_EXT_NO_ERROR_SENSE_KEY,                           false,},
    { FBE_SCSI_BADSTATUS,                      DRV_EXT_NO_ERROR_SENSE_KEY,                           false,},
    { FBE_SCSI_XFERCOUNTNOTZERO,               DRV_EXT_SCSI_XFERCOUNTNOTZERO,                        true,},
    { FBE_SCSI_INVALIDSCB,                     DRV_EXT_SCSI_INVALIDSCB,                              true,},
    { FBE_SCSI_TOOMUCHDATA,                    DRV_EXT_SCSI_TOOMUCHDATA,                             true,},
    { FBE_SCSI_CHANNEL_INACCESSIBLE,           DRV_EXT_BUS_SHUT_DOWN,                                true,},
    { FBE_SCSI_IO_LINKDOWN_ABORT,              DRV_EXT_IO_LINKDOWN_ABORT,                            true,},
    { FBE_SCSI_FCP_INVALIDRSP,                 DRV_EXT_SCSI_INVALIDSCB,                              true,},
    { FBE_SCSI_UNKNOWNRESPONSE,                DRV_EXT_UNKNOWN_RESPONSE,                             true,},
    { FBE_SCSI_CC_ILLEGAL_REQUEST,             DRV_EXT_DEV_ILLEGAL_REQ,                              true,},
    { FBE_SCSI_INVALIDREQUEST,                 DRV_EXT_INVALID_REQUEST,                              true,},
    { FBE_SCSI_CC_ABORTED_CMD_PARITY_ERROR,    DRV_EXT_DEV_DET_PARITY_ERROR,                         true,},
    { FBE_SCSI_CC_ABORTED_CMD,                 DRV_EXT_DEV_ABORTED_CMD,                              true,},
    { FBE_SCSI_CC_GENERAL_FIRMWARE_ERROR,      DRV_EXT_HARDWARE_ERROR,                               true,},
    { FBE_SCSI_CC_HARDWARE_ERROR_NO_SPARE,     DRV_EXT_HARDWARE_ERROR_NO_SPARE,                      true,},
    { FBE_SCSI_CC_HARDWARE_ERROR_PARITY,       DRV_EXT_DEV_DET_PARITY_ERROR,                         true,},
    { FBE_SCSI_CC_HARDWARE_ERROR_SELF_TEST,    DRV_EXT_HARDWARE_ERROR,                               true,},
    { FBE_SCSI_CC_HARDWARE_ERROR,              DRV_EXT_HARDWARE_ERROR,                               true,},
    { FBE_SCSI_CC_MEDIA_ERR_DONT_REMAP,        DRV_EXT_MEDIA_WRITE_ERR_CORRECTED,                    true,},
    { FBE_SCSI_CC_MEDIA_ERR_CANT_REMAP,        DRV_EXT_MEDIA_ERROR_CANT_REMAP,                       true,},
    { FBE_SCSI_CC_MODE_SELECT_OCCURRED,        DRV_EXT_NO_ERROR_SENSE_KEY,                           false,},
    { FBE_SCSI_CC_UNIT_ATTENTION,              DRV_EXT_UNIT_ATTENTION,                               true,},
    { FBE_SCSI_CC_UNEXPECTED_SENSE_KEY,        DRV_EXT_UNEXPECTED_SENSE_KEY,                         true,},
    { FBE_SCSI_CHECK_COND_OTHER_SLOT,          DRV_EXT_DEV_ABORTED_CMD,                              true,},
    { FBE_SCSI_CC_SEEK_POSITIONING_ERROR,      DRV_EXT_SEEK_POS_ERROR,                               true,},
    { FBE_SCSI_CC_SEL_ID_ERROR,                DRV_EXT_SEL_ID_ERROR,                                 true,},
    { FBE_SCSI_CC_HARDWARE_ERROR_WRITE_FAULT,  DRV_EXT_HARDWARE_ERROR,                               true,},
    { FBE_SCSI_CC_INTERNAL_TARGET_FAILURE,     DRV_EXT_HARDWARE_ERROR,                               true,},
    { FBE_SCSI_SLOT_BUSY,                      DRV_EXT_NO_ERROR_SENSE_KEY,                           false,},
    { FBE_SCSI_DEVICE_BUSY,                    DRV_EXT_NO_ERROR_SENSE_KEY,                           false,},
    { FBE_SCSI_CC_NOT_READY,                   DRV_EXT_NOT_READY,                                    true,},
    //{ FBE_SCSI_CC_FORMAT_IN_PROGRESS,          DRV_EXT_NOT_READY_FORMAT_IN_PROGRESS                  false,},   /*TODO: verify if this needed? */
    { FBE_SCSI_DEVICE_NOT_READY,               DRV_EXT_NOT_READY,                                    true,},
    { FBE_SCSI_CC_NOT_SPINNING,                DRV_EXT_NOT_READY,                                    true,},
    { FBE_SCSI_IO_TIMEOUT_ABORT,               DRV_EXT_TIMEOUT,                                      true,},
    { FBE_SCSI_SATA_IO_TIMEOUT_ABORT,          DRV_EXT_TIMEOUT,                                      true,},
    { FBE_SCSI_CC_DATA_OFFSET_ERROR,           DRV_EXT_SCSI_DATA_OFFSET_ERROR,                       true,},
    { FBE_SCSI_CC_MEDIA_ERR_WRITE_ERROR,       DRV_EXT_MEDIA_ERROR_WRITE_ERROR,                      true,},
    { FBE_SCSI_DRVABORT,                       DRV_EXT_BUS_RESET,                                    false,},
    { FBE_SCSI_INCIDENTAL_ABORT,               DRV_EXT_INCIDENTAL_ABORT,                             true,},
    { FBE_SCSI_BUSRESET,                       DRV_EXT_BUS_RESET,                                    true,},
    { FBE_SCSI_CC_DEFERRED_ERROR,              DRV_EXT_DEFERRED_ERROR,                               true,},
    { FBE_SCSI_CC_SENSE_DATA_MISSING,          DRV_EXT_SENSE_DATA_MISSING,                           false,},
    { FBE_SCSI_DEVICE_RESERVED,                DRV_EXT_NO_ERROR_SENSE_KEY,                           false,},
    { FBE_SCSI_CC_BECOMING_READY,              DRV_EXT_START_IN_PROGRESS,                            true,},
    { FBE_SCSI_CC_ABORTED_CMD_ATA_TO,          DRV_EXT_DEV_ABORTED_CMD,                              true,},
    { FBE_SCSI_CC_AUTO_REMAPPED,               DRV_EXT_RECOVERED_ERROR_AUTO_REMAPPED,                true,},
    { FBE_SCSI_CC_DEFECT_LIST_ERROR,           DRV_EXT_DEFECT_LIST_ERROR,                            true,},
   // { FBE_SCSI_CC_DRIVE_TABLE_REBUILD,         DRV_EXT_HARD_ERROR_MASK|DRV_EXT_DRIVE_TABLE_REBUILD,   true,},    
   // { FBE_SCSI_CC_FORMAT_CORRUPTED,            DRV_EXT_HARD_ERROR_MASK | DRV_EXT_BAD_FORMAT,          true,},
    { FBE_SCSI_CC_HARD_BAD_BLOCK,              DRV_EXT_BAD_BLOCK,                                    true,},
    { FBE_SCSI_CC_HARDWARE_ERROR_FIRMWARE,     DRV_EXT_HARDWARE_ERROR,                               true,},
    { FBE_SCSI_CC_MEDIA_ERR_DO_REMAP,          DRV_EXT_MEDIA_WRITE_ERR_CORRECTED,                    true,},
    { FBE_SCSI_CC_MEDIA_ERROR_WRITE_FAULT,     DRV_EXT_BAD_BLOCK,                                    true,},
    { FBE_SCSI_CC_PFA_THRESHOLD_REACHED,       DRV_EXT_PFA_THRESHOLD_REACHED,                        true,},
    { FBE_SCSI_CC_RECOVERED_BAD_BLOCK,         DRV_EXT_ECC_BAD_BLOCK,                                true,},
    { FBE_SCSI_CC_RECOVERED_DEFECT_LIST_ERROR, DRV_EXT_DEFECT_LIST_ERROR,                            true,},
    { FBE_SCSI_CC_RECOVERED_ERR,               DRV_EXT_RECOVERED_ERROR,                              true,},
    { FBE_SCSI_CC_RECOVERED_ERR_CANT_REMAP,    DRV_EXT_RECOVERED_ERROR_CANT_REMAP,                   true,},
    { FBE_SCSI_CC_RECOVERED_ERR_NOSYNCH,       DRV_EXT_RECOVERED_ERROR,                              true,},
    { FBE_SCSI_CC_RECOVERED_WRITE_FAULT,       DRV_EXT_BAD_BLOCK,                                    true,},
   // { FBE_SCSI_CC_SUPER_CAP_FAILURE,           DRV_EXT_HARD_ERROR_MASK | DRV_EXT_SUPER_CAP_FAILURE,   true,},
    { FBE_SCSI_CC_TEMP_THRESHOLD_REACHED,      DRV_EXT_TEMP_THRESHOLD_REACHED,                       true,},
    { FBE_SCSI_CC_WRITE_PROTECT,               DRV_EXT_WRITE_PROTECT,                                false,},
    { FBE_SCSI_USERABORT,                      DRV_EXT_BUS_RESET,                                    true,},
    { FBE_SCSI_CC_DIE_RETIREMENT_START,        DRV_EXT_DIE_RETIREMENT_START,                         true,},
    { FBE_SCSI_CC_DIE_RETIREMENT_END,          DRV_EXT_DIE_RETIREMENT_END,                           true,},
    { FBE_SCSI_ENCRYPTION_NOT_ENABLED,         DRV_EXT_ENCRYPTION_NOT_ENABLED,                       true,},
    { FBE_SCSI_ENCRYPTION_BAD_KEY_HANDLE,      DRV_EXT_ENCRYPTION_BAD_KEY_HANDLE,                    true,},
    { FBE_SCSI_ENCRYPTION_KEY_WRAP_ERROR,      DRV_EXT_ENCRYPTION_KEY_WRAP_ERROR,                    true,},

    /*add new stuff above*/
    {0, 0, false}
};

/* As the meeting with Marc Cassano and Peter Puhov,  they state that the  PDO MUST set one of the FAULT flags if the drive 
   goes to Failed. As POD, sometimes,  drive fails  is not because of the drive is bad, it is possible the EOL, activate 
   timer expired , or other reasons. We call drive is in logical offline, the drive fault bit should not set. But for a hole 
   in PVD, we need set the drive fault bit when it is in the logical offline. This will be solved in Rockies MR1. AR#558420. */
static const fbe_base_physical_drive_fault_bit_translation_t     fbe_fault_bit_translation_table [] =
{
    /* death_reason                                                                drive_fault link_fault  Logical_offline */
    { FBE_BASE_DISCOVERED_DEATH_REASON_ACTIVATE_TIMER_EXPIRED,                        false,     true,         true,  },    
    { FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_SPECIALIZED_TIMER_EXPIRED,                 false,     true,         false, },    
    { FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_LINK_THRESHOLD_EXCEEDED,                   false,     true,         false, },   
    { FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_RECOVERED_THRESHOLD_EXCEEDED,              true,      false,        false, },   
    { FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_MEDIA_THRESHOLD_EXCEEDED,                  true,      false,        false, },   
    { FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_HARDWARE_THRESHOLD_EXCEEDED,               true,      false,        false, },   
    { FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_HEALTHCHECK_THRESHOLD_EXCEEDED,            true,      false,        false, },   
    { FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_DATA_THRESHOLD_EXCEEDED,                   true,      false,        false, },   
    { FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_FATAL_ERROR,                               true,      false,        false, },   
    { FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_ACTIVATE_TIMER_EXPIRED,                    false,     true,         true,  },   
    { FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_ENCLOSURE_OBJECT_FAILED,                   false,     true,         true,  },   
    { FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_INVALID_CAPACITY,                          true,      false,        false, },   
    { FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_FAILED_TO_SPIN_DOWN,                       true,      false,        false, },   
    { FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_CRC_MULTI_BIT,                             true,      false,        false, },   
    { FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_CRC_SINGLE_BIT,                            true,      false,        false, },   
    { FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_CRC_OTHER,                                 true,      false,        false, },   
    { FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_SIMULATED,                                 true,      false,        false, },    
    { FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_ENHANCED_QUEUING_NOT_SUPPORTED,            true,      false,        false, },   
    { FBE_BASE_PYHSICAL_DRIVE_DEATH_REASON_EXCEEDS_PLATFORM_LIMITS,                   false,     true,         true,  }, 
    { FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_INVALID_VAULT_CONFIGURATION,               false,     true,         false, },
    { FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_INVALID_ID,                                true,      false,        false, },
    { FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_DEFECT_LIST_ERRORS,                         true,      false,        false, },   
    { FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_MAX_REMAPS_EXCEEDED,                        true,      false,        false, },   
    { FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_FAILED_TO_SET_DEV_INFO,                     true,      false,        false, },   
    { FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_FAILED_TO_PROCESS_MODE_SENSE,               true,      false,        false, },   
    { FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_FAILED_TO_PROCESS_READ_CAPACITY,            true,      false,        false, },   
    { FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_FAILED_TO_REASSIGN,                         true,      false,        false, },   
    { FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_DRIVE_NOT_SPINNING,                         true,      false,        false, },   
    { FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_FAILED_TO_SPIN_DOWN,                        true,      false,        false, },   
    { FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_WRITE_PROTECT,                              true,      false,        false, },   
    { FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_FW_UPGRADE_REQUIRED,                        true,      false,        false, },   
    { FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_UNEXPECTED_FAILURE,                         true,      false,        false, },   
    { FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_DC_FLUSH_FAILED,                            true,      false,        false, },   
    { FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_HEALTHCHECK_FAILED,                         true,      false,        false, },   
    { FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_FW_UPGRADE_FAILED,                          true,      false,        true,  },   
    { FBE_SATA_PHYSICAL_DRIVE_DEATH_REASON_INVALID_INSCRIPTION,                       true,      false,        false, },   
    { FBE_SATA_PHYSICAL_DRIVE_DEATH_REASON_PIO_MODE_UNSUPPORTED,                      true,      false,        false, },   
    { FBE_SATA_PHYSICAL_DRIVE_DEATH_REASON_UDMA_MODE_UNSUPPORTED,                     true,      false,        false, },       
    { FBE_SATA_PHYSICAL_DRIVE_DEATH_REASON_FAILED_TO_SET_DEV_INFO,                    true,      false,        false, },       
    { FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_SED_DRIVE_NOT_SUPPORTED,					  true,      false,        false, },   
    /* Invalid death reasons from both enums*/
    { FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_INVALID,                                   false,     false,        false, },   
    { FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_INVALID,                                    false,     false,        false, },   

    /*add new stuff above*/
    {0, false, false, false}
};

fbe_status_t 
fbe_base_physical_drive_update_error_counts(fbe_base_physical_drive_t * base_physical_drive, fbe_physical_drive_mgmt_get_error_counts_t * get_error_counts)
{
    /* get_error_counts->physical_drive_error_counts.recoverable_error_count = fbe_atomic_increment(&base_physical_drive->physical_drive_error_counts.recoverable_error_count); */
    get_error_counts->physical_drive_error_counts.remap_block_count = base_physical_drive->physical_drive_error_counts.remap_block_count; 
    get_error_counts->physical_drive_error_counts.reset_count = base_physical_drive->physical_drive_error_counts.reset_count; 
    get_error_counts->physical_drive_error_counts.error_count = base_physical_drive->physical_drive_error_counts.error_count; 
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_physical_drive_reset_error_counts(fbe_base_physical_drive_t * base_physical_drive)
{
    
    /* fbe_atomic_exchange(&base_physical_drive->physical_drive_error_counts.recoverable_error_count, 0); */
    base_physical_drive->physical_drive_error_counts.error_count = 0;   
    
    return FBE_STATUS_OK;
}

/*!**************************************************************
 * fbe_base_physical_drive_xlate_death_reason_to_fault_bit()
 ****************************************************************
 * @brief
 *  This function  Search the fault bit translation table to match
 *  the death reason and return the drive and link fault bool value.
 *
 * @param xlate_fault_bit - pointer to fault bit translation structure
 *
 * @return void
 *
 * @author
 *  08/21/2012 : Dipak Patel --Ported it from Upper DH code.
 *
 ****************************************************************/
void fbe_base_physical_drive_xlate_death_reason_to_fault_bit (fbe_base_physical_drive_fault_bit_translation_t *xlate_fault_bit)
{
    const fbe_base_physical_drive_fault_bit_translation_t *trans_ptr = fbe_fault_bit_translation_table;    
    
    while (trans_ptr->death_reason != 0 ) {
        if (trans_ptr->death_reason == xlate_fault_bit->death_reason) {
            /*found a match*/
            memcpy(xlate_fault_bit, trans_ptr, sizeof(fbe_base_physical_drive_fault_bit_translation_t));
            return;
        }

        trans_ptr++;/*go to next entry*/
    }
    /*we had no match.
     Update the defaults and do not log the event*/
                            
    xlate_fault_bit->drive_fault = false;
    xlate_fault_bit->link_fault = false;
    xlate_fault_bit->logical_offline = false;

    return;
}

/*!**************************************************************
 * fbe_base_physical_drive_get_error_initiator()
 ****************************************************************
 * @brief
 *  This function  will indentify if error is simulated one or actual
 *  drive error.
 *
 * @param last_error_code - Last error code.
 *
 * @return error_initiator
 *
 * @author
 *  07/18/2012 : Dipak Patel --Ported it from Upper DH code.
 *
 ****************************************************************/
fbe_u8_t * fbe_base_physical_drive_get_error_initiator(fbe_u32_t last_error_code)
{
    fbe_u8_t *error_initiator = NULL;

    if (last_error_code == 0xFFFFFFFF) {
        error_initiator = (fbe_u8_t *)("DEST");
    }
    else{
        error_initiator = (fbe_u8_t *)("");
    }
    
    return  error_initiator;

}

/*!**************************************************************
 * fbe_base_physical_drive_get_path_attribute_string()
 ****************************************************************
 * @brief
 *  This function  will return path attribute in string format.
 *
 * @param path_attribute - path attribute
 *
 * @return path attribute in string format
 *
 * @author
 *  10/01/2012 : Dipak Patel 
 *
 ****************************************************************/
fbe_u8_t * fbe_base_physical_drive_get_path_attribute_string(fbe_u32_t path_attribute)
{
    fbe_u8_t *path_attribute_string = NULL;

    switch (path_attribute) 
    {
        case FBE_BLOCK_PATH_ATTR_FLAGS_END_OF_LIFE:
             path_attribute_string = (fbe_u8_t *) ("END OF LIFE");
        break;
        case FBE_BLOCK_PATH_ATTR_FLAGS_CALL_HOME_END_OF_LIFE:
             path_attribute_string = (fbe_u8_t *) ("CALL HOME END OF LIFE");
        break;
        case FBE_BLOCK_PATH_ATTR_FLAGS_CALL_HOME_KILL:
             path_attribute_string = (fbe_u8_t *) ("CALL HOME KILL");
        break;
        case FBE_BLOCK_PATH_ATTR_FLAGS_LINK_FAULT:
             path_attribute_string = (fbe_u8_t *) ("LINK FAULT");
        break;
        case FBE_BLOCK_PATH_ATTR_FLAGS_DRIVE_FAULT:
             path_attribute_string = (fbe_u8_t *) ("DRIVE FAULT");
        break;
        case FBE_BLOCK_PATH_ATTR_FLAGS_CLEAR_DRIVE_FAULT_PENDING:
             path_attribute_string = (fbe_u8_t *) ("CLEAR DRIVE FAULT");
        break;
        case FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_IS_HIBERNATING:
             path_attribute_string = (fbe_u8_t *) ("CLIENT IS HIBERNATING");
        break;
        case FBE_BLOCK_PATH_ATTR_FLAGS_CHECK_QUEUED_IO_TIMER:
             path_attribute_string = (fbe_u8_t *) ("CHECK QUEUED IO TIMER");
        break;
        case FBE_BLOCK_PATH_ATTR_FLAGS_TIMEOUT_ERRORS:
             path_attribute_string = (fbe_u8_t *) ("TIMEOUT ERRORS");
        break;
        case FBE_BLOCK_PATH_ATTR_FLAGS_HAS_BEEN_WRITTEN:
             path_attribute_string = (fbe_u8_t *) ("HAS BEEN WRITTEN");
        break;
        case FBE_BLOCK_PATH_ATTR_FLAGS_DEGRADED:
             path_attribute_string = (fbe_u8_t *) ("DEGRADED");
        break;
        case FBE_BLOCK_PATH_ATTR_FLAGS_PATH_NOT_PREFERRED:
             path_attribute_string = (fbe_u8_t *) ("PATH NOT PREFERRED");
        break;
        case FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_IS_READY_TO_HIBERNATE:
             path_attribute_string = (fbe_u8_t *) ("CLIENT IS READY TO HIBERNATE");
        break;
        case FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_CAN_SAVE_POWER:
             path_attribute_string = (fbe_u8_t *) ("CLIENT CAN SAVE POWER");
        break;
        case FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_DOWNLOAD_REQ:
             path_attribute_string = (fbe_u8_t *) ("CLIENT DOWNLOAD REQ");
        break;
        case FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_DOWNLOAD_GRANT:
             path_attribute_string = (fbe_u8_t *) ("CLIENT DOWNLOAD GRANT");
        break;
        case FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_DOWNLOAD_REQ_FAST_DL:
             path_attribute_string = (fbe_u8_t *) ("CLIENT DOWNLOAD REQ FAST DL");
        break;
        case FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_DOWNLOAD_REQ_TRIAL_RUN:
             path_attribute_string = (fbe_u8_t *) ("CLIENT DOWNLOAD REQ TRIAL RUN");
        break;
        case FBE_BLOCK_PATH_ATTR_FLAGS_HEALTH_CHECK_REQUEST:
             path_attribute_string = (fbe_u8_t *) ("HEALTH CHECK REQUEST");
        break;
        case FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_IGNORE_OFFSET:
             path_attribute_string = (fbe_u8_t *) ("CLIENT IGNORE OFFSET");
        break;
        case FBE_BLOCK_PATH_ATTR_ALL:
        default:
             path_attribute_string = (fbe_u8_t *) ("INVALID PATH ATTRIBUTE");
        break;
    }
    return (path_attribute_string);
}
/*!**************************************************************
 * fbe_base_physical_drive_xlate_scsi_code_to_drive_ext_status()
 ****************************************************************
 * @brief
 *  This function  Search the error translation table to match the scsi code and
 *   return the DRV_EXT code.
 *
 * @param xlate_err_tuple - pointer to error translation structure
 *
 * @return void
 *
 * @author
 *  05/23/2012 : Dipak Patel --Ported it from Upper DH code.
 *
 ****************************************************************/
void fbe_base_physical_drive_xlate_scsi_code_to_drive_ext_status (fbe_base_drive_error_translation_t *xlate_err_tuple)
{
    const fbe_base_drive_error_translation_t *trans_ptr = fbe_error_translation_table;    
    
    while (trans_ptr->scsi_error_code != 0 ) {
        if (trans_ptr->scsi_error_code == xlate_err_tuple->scsi_error_code) {
            /*found a match*/
            memcpy(xlate_err_tuple, trans_ptr, sizeof(fbe_base_drive_error_translation_t));
            return;
        }

        trans_ptr++;/*go to next entry*/
    }
    /*we had no match.
     Update the defaults and do not log the event*/
                            
    xlate_err_tuple->drive_ext_code = DRV_EXT_NO_ERROR_SENSE_KEY;
    xlate_err_tuple->log_event = false;

    return;
}

/*!**************************************************************
 * fbe_base_physical_drive_create_drive_string()
 ****************************************************************
 * @brief
 *  This function create the string for drive considering bank width.
 *
 * @param base_physical_drive - pointer to PDO object
 * @param deviceStr - drive string.
 * @param bufferLen - lenght of the string.
 *
 * @return status
 *
 * @author
 *  06/07/2012 : Dipak Patel --Created.
 *
 ****************************************************************/
fbe_status_t fbe_base_physical_drive_create_drive_string(fbe_base_physical_drive_t * base_physical_drive, 
                                                         fbe_u8_t * deviceStr, 
                                                         fbe_u32_t bufferLen,
                                                         fbe_bool_t completeStr)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_port_number_t      port_number = 0;
    fbe_enclosure_number_t enclosure_number = 0;
    fbe_slot_number_t      slot_number = 0;
    fbe_u32_t bankWidth = 0;
    fbe_u8_t bankLtr = '\0';
    fbe_u16_t rowOffset = 0;

    /* Get Bus,Enclosure,Slot & bank width information. */
    status = fbe_base_physical_drive_get_port_number(base_physical_drive,&port_number);
    status = fbe_base_physical_drive_get_enclosure_number(base_physical_drive,&enclosure_number);
    status = fbe_base_physical_drive_get_slot_number(base_physical_drive,&slot_number);
    status = fbe_base_physical_drive_get_bank_width(base_physical_drive,&bankWidth);

    /* calculate bankLetter and offset according to bankWidth and add
       it in slot incase of Voyager enclosure */
    if(bankWidth != 0)
    {
        bankLtr = (slot_number/ bankWidth) + 'A';
        rowOffset =  (slot_number%bankWidth); 

        if (completeStr)
        {
            sprintf(deviceStr, "Bus %d Enclosure %d Disk %d (%c%d)",
                                                  port_number,
                                                  enclosure_number,
                                                  slot_number,
                                                  bankLtr,
                                                  rowOffset);
        }
        else
        {
            sprintf(deviceStr, "%d_%d_%d (%c%d)",
                                     port_number,
                                     enclosure_number,
                                     slot_number,
                                     bankLtr,
                                     rowOffset);
        }
    }
    else
    {
        if (completeStr)
        {
            sprintf(deviceStr, "Bus %d Enclosure %d Disk %d", port_number, enclosure_number, slot_number);
        }
        else
        {
            sprintf(deviceStr, "%d_%d_%d", port_number, enclosure_number, slot_number);
        }
    }

    return status;
}

/*!**************************************************************
 * fbe_base_physical_drive_get_additional_data()
 ****************************************************************
 * @brief
 *  This function create the string additional data from the CDB.
 *
 * @param payload_cdb_operation - CDB operation
 * @param additionalDataStr - Additional data string.
 *
 * @return status
 * 
 * @note  The strings that built up for the event log must be
 *        less than 96 chars, otherwise they will be truncated
 * 
 * @author
 *  07/18/2012 : Dipak Patel --Created.
 *
 ****************************************************************/
fbe_status_t fbe_base_physical_drive_get_additional_data(const fbe_payload_cdb_operation_t* payload_cdb_operation,
                                                         fbe_u8_t *additionalDataStr)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u8_t *sense_data = NULL;
    fbe_scsi_sense_key_t sense_key = FBE_SCSI_SENSE_KEY_NO_SENSE; 
    fbe_scsi_additional_sense_code_t asc = FBE_SCSI_ASC_NO_ADDITIONAL_SENSE_INFO; 
    fbe_scsi_additional_sense_code_qualifier_t ascq = FBE_SCSI_ASC_NO_ADDITIONAL_SENSE_INFO;
    fbe_bool_t lba_is_valid = FBE_FALSE;
    fbe_lba_t bad_lba = FBE_LBA_INVALID;
    fbe_lba_t lba = FBE_LBA_INVALID;
    fbe_block_count_t blocks = FBE_CDB_BLOCKS_INVALID;   

    fbe_payload_cdb_operation_get_sense_buffer((fbe_payload_cdb_operation_t*)payload_cdb_operation, &sense_data);

    /* Fetch LBA and Blocks from CDB */
    blocks = fbe_get_cdb_blocks(payload_cdb_operation);
    lba = fbe_get_cdb_lba(payload_cdb_operation);
 
    /* Function below parses the sense data */
    status = fbe_payload_cdb_parse_sense_data(sense_data, &sense_key, &asc, &ascq, &lba_is_valid, &bad_lba);

    if (status == FBE_STATUS_GENERIC_FAILURE) 
    {    
        sprintf(additionalDataStr, "INVALID SENSE BUFFER OP 0x%x, LBA 0x%x, SZ 0x%x", /* caution: eventlog strings must be < 96 chars*/
                                          payload_cdb_operation->cdb[0],
                                          (unsigned int)lba,
                                          (unsigned int)blocks);
    }
    else
    {

        if (lba_is_valid == FBE_TRUE)  /* This is the status on the new function */ 
        {
           
            sprintf(additionalDataStr, "%02x|%02x|%02x BLBA 0x%llx OP 0x%x, LBA 0x%llx, SZ 0x%llx", /* caution: eventlog strings must be < 96 chars*/
                                              sense_key,
                                              asc,
                                              ascq,
                                              bad_lba,
                                              payload_cdb_operation->cdb[0],
                                              lba,
                                              blocks);
        }
        else
        {
            
            sprintf(additionalDataStr, "%02x|%02x|%02x OP 0x%x, LBA 0x%llx, SZ 0x%llx", /* caution: eventlog strings must be < 96 chars*/
                                              sense_key,
                                              asc,
                                              ascq,
                                              payload_cdb_operation->cdb[0],
                                              lba,
                                              blocks);
        }
    }

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * fbe_base_physical_drive_log_event()
 ****************************************************************
 * @brief
 *  This function  logs the event according to SCSI error.
 *
 * @param base_physical_drive - pointer to PDO object
 * @param error - SCSI error code.
 *
 * @return status
 * 
 * @note  The strings that built up for the event log must be
 *        less than 96 chars, otherwise they will be truncated
 * 
 * @author
 *  05/23/2012 : Dipak Patel --Created.
 *
 ****************************************************************/
fbe_status_t fbe_base_physical_drive_log_event(fbe_base_physical_drive_t * base_physical_drive,
                                               fbe_scsi_error_code_t error,
                                               fbe_packet_t * packet,
                                               fbe_u32_t last_error_code,
                                               const fbe_payload_cdb_operation_t * payload_cdb_operation)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_base_drive_error_translation_t xlate_err_tuple = {0};
    fbe_u8_t deviceStr[FBE_PHYSICAL_DRIVE_STRING_LENGTH];
    fbe_u8_t additionalDataStr[FBE_PHYSICAL_DRIVE_STRING_LENGTH];
    fbe_u8_t latencyDataString[FBE_PHYSICAL_DRIVE_STRING_LENGTH];
    fbe_time_t io_start_time = 0;
    fbe_time_t io_end_time = 0;
    fbe_bool_t is_sense_data_available = FBE_FALSE;

    fbe_zero_memory(&deviceStr[0], FBE_PHYSICAL_DRIVE_STRING_LENGTH);
    fbe_zero_memory(&additionalDataStr[0], FBE_PHYSICAL_DRIVE_STRING_LENGTH);
    fbe_zero_memory(&latencyDataString[0], FBE_PHYSICAL_DRIVE_STRING_LENGTH);

    /* Get the CMD, LBA, Blocks from CDB and
       SCSI code, Field Replaceable unit code, Bad LBA from the sense data */
    if (FBE_PAYLOAD_CDB_SCSI_STATUS_CHECK_CONDITION == payload_cdb_operation->payload_cdb_scsi_status)
    {           
        fbe_base_physical_drive_get_additional_data(payload_cdb_operation, &additionalDataStr[0]);
        is_sense_data_available = FBE_TRUE;
    }

    /* Create string with Bus, Enclosure and Slot information*/
    status = fbe_base_physical_drive_create_drive_string(base_physical_drive, 
                                                         &deviceStr[0],
                                                         FBE_PHYSICAL_DRIVE_STRING_LENGTH,
                                                         TRUE);

    if (FBE_STATUS_OK != status)
    {
        fbe_base_physical_drive_customizable_trace( base_physical_drive, FBE_TRACE_LEVEL_ERROR,
                                                    "%s error creating string. status:%d.\n", __FUNCTION__, status); 
    }

    /* Update the SCSI error code in error translation structure */
    xlate_err_tuple.scsi_error_code = error;
    fbe_base_physical_drive_xlate_scsi_code_to_drive_ext_status(&xlate_err_tuple);

    /* Get IO start, end and servicibility time*/
    status = fbe_payload_cdb_get_physical_drive_io_start_time(payload_cdb_operation, &io_start_time);
    status = fbe_payload_cdb_get_physical_drive_io_end_time(payload_cdb_operation, &io_end_time);  


    _snprintf(&latencyDataString[0], FBE_PHYSICAL_DRIVE_STRING_LENGTH, "SRT %dms ST 0x%llx ET 0x%llx",
                               (int)packet->physical_drive_service_time_ms,
                               io_start_time,
                               io_end_time);

    /* Log IO Error*/
    if(xlate_err_tuple.log_event)
    {
        switch(xlate_err_tuple.drive_ext_code)
        {
            /* Hard Media Errors */
            case DRV_EXT_BAD_BLOCK:
            case DRV_EXT_ECC_BAD_BLOCK:
            case DRV_EXT_RECOVERED_ERROR_CANT_REMAP:
            case DRV_EXT_RECOVERED_ERROR_REMAP_FAILED:
            case DRV_EXT_MEDIA_ERROR_CANT_REMAP:
            case DRV_EXT_MEDIA_ERROR_WRITE_ERROR:
            case DRV_EXT_MEDIA_ERROR_REMAP_FAILED:
            case DRV_EXT_DEFECT_LIST_ERROR:

            /* Soft Media Errors */ 
            case DRV_EXT_MEDIA_WRITE_ERR_CORRECTED:
            case DRV_EXT_RECOVERED_ERROR_AUTO_REMAPPED:

                fbe_event_log_write(PHYSICAL_PACKAGE_WARN_SOFT_MEDIA_ERROR, NULL, 0, "%s %x %s %s",
                                            &deviceStr[0],
                                            xlate_err_tuple.drive_ext_code,
                                            &latencyDataString[0],
                                            fbe_base_physical_drive_get_error_initiator(last_error_code));

                break;

            case DRV_EXT_NO_ERROR_SENSE_KEY:
            case DRV_EXT_RECOVERED_ERROR:
            default:

                fbe_event_log_write(PHYSICAL_PACKAGE_WARN_SOFT_SCSI_BUS_ERROR, NULL, 0, "%s %x %s %s",
                                            &deviceStr[0],
                                            xlate_err_tuple.drive_ext_code,
                                            &latencyDataString[0],
                                            fbe_base_physical_drive_get_error_initiator(last_error_code));

                break;
       }

       /* Log a common event for IO Sense Info*/
        if (is_sense_data_available)
        {
            fbe_event_log_write(PHYSICAL_PACKAGE_INFO_PDO_SENSE_DATA, NULL, 0, "%s %s %s",
                                                    &deviceStr[0],
                                                    &additionalDataStr[0],
                                                    fbe_base_physical_drive_get_error_initiator(last_error_code));
        }

    }
    return status;
}

/*!**************************************************************
 * fbe_base_physical_drive_log_health_check_event()
 ****************************************************************
 * @brief
 *  This function  will log health check status event
 *
 * @param base_physical_drive - pointer to base physical drive
 * @param state - Health check status
 *
 * @return status - 
 *
 * @author
 *  29/12/2012 : Dipak Patel 
 *
 ****************************************************************/
fbe_status_t fbe_base_physical_drive_log_health_check_event(fbe_base_physical_drive_t *base_physical_drive, 
                                                            fbe_base_physical_drive_health_check_state_t state)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u8_t deviceStr[FBE_PHYSICAL_DRIVE_STRING_LENGTH];

    fbe_zero_memory(&deviceStr[0], FBE_PHYSICAL_DRIVE_STRING_LENGTH);

    /* Create string with Bus, Enclosure and Slot information*/
    status = fbe_base_physical_drive_create_drive_string(base_physical_drive, 
                                                         &deviceStr[0],
                                                         FBE_PHYSICAL_DRIVE_STRING_LENGTH,
                                                         TRUE);

    fbe_event_log_write(PHYSICAL_PACKAGE_INFO_HEALTH_CHECK_STATUS, NULL, 0, "%s %s",
                                    &deviceStr[0],
                                    fbe_base_physical_drive_get_health_check_status_string(state));

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * fbe_base_physical_drive_get_health_check_status_string()
 ****************************************************************
 * @brief
 *  This function  will return health check status in string format.
 *
 * @param state - Health check status
 *
 * @return Health check status in string format
 *
 * @author
 *  20/12/2012 : Dipak Patel 
 *
 ****************************************************************/
fbe_u8_t * fbe_base_physical_drive_get_health_check_status_string(fbe_base_physical_drive_health_check_state_t state)
{
    fbe_u8_t *health_check_string = NULL;

    switch (state) 
    {
        case FBE_BASE_PHYSICAL_DRIVE_HEALTH_CHECK_INITIATED:
             health_check_string = (fbe_u8_t *) ("INITIATED");
        break;
        case FBE_BASE_PHYSICAL_DRIVE_HEALTH_CHECK_STARTED:
             health_check_string = (fbe_u8_t *) ("STARTED");
        break;
        case FBE_BASE_PHYSICAL_DRIVE_HEALTH_CHECK_COMPLETED:
             health_check_string = (fbe_u8_t *) ("COMPLETED");
        break;
        case FBE_BASE_PHYSICAL_DRIVE_HEALTH_CHECK_FAILED:
             health_check_string = (fbe_u8_t *) ("FAILED");
        break;
        case FBE_BASE_PHYSICAL_DRIVE_HEALTH_CHECK_INVALID:
        default:
             health_check_string = (fbe_u8_t *) ("INVALID");
        break;
    }
    return (health_check_string);
}
