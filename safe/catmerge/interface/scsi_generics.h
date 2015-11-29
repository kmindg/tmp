
/***********************************************************************
 * Copyright (C) EMC Corp. 2011
 * All rights reserved.
 * Licensed material - Property of EMC Corporation.
 ***********************************************************************/
 
/***********************************************************************
 *  scsi_generics.h
 ***********************************************************************
 *
 *  DESCRIPTION:
 *      This file contains definitions for generic SCSI related structures, 
 *      macros, constants, etc.
 *
 *  NOTES:
 *
 *  HISTORY:
 *      17 Mar 11   PRB     Created
 ***********************************************************************/

/* Make sure we only include this file once */
#ifndef SCSI_GENERICS_H
#define SCSI_GENERICS_H


#pragma pack(1)
typedef struct _SCSI_SENSE_DATA {
    
    UINT_8E ResponseCode;
    
    union {
        
        struct {
            UINT_8E Obsolete;
            UINT_8E SenseKey;
            UINT_8E Information[4];
            UINT_8E AdditionalSenseLength;
            UINT_8E CommandSpecificInformation[4];
            UINT_8E AdditionalSenseCode;
            UINT_8E AdditionalSenseCodeQualifier;
            UINT_8E FieldReplaceableUnitCode;
            UINT_8E SenseKeySpecific[3];
        } fixed;

        struct {
            UINT_8E SenseKey;
            UINT_8E AdditionalSenseCode;
            UINT_8E AdditionalSenseCodeQualifier;
            UINT_8E Reserved[3];
            UINT_8E AdditionalSenseLength;
            // Sense Data Descriptors follow here...
        } descriptor;
        
    } format;

} SCSI_SENSE_DATA, *PSCSI_SENSE_DATA;
#pragma pack( )

// *************************
// Sense Data - MASK defines
// *************************

// -- Format Independent mask values --
#define SCSI_SENSE_KEY_MASK 0x0F

// -- Descriptor Format mask values --

// -- Fixed Format mask values --
#define SCSI_SENSE_VALID_MASK_FIXED_FORMAT 0x08


// *************************
// Sense Data - OFFSET defines
// *************************

// -- Format Independent defines --

// -- Descriptor Format defines --
#define SCSI_SENSE_DATA_ADDITIONAL_SENSE_LENGTH_OFFSET_DESCRIPTOR_FORMAT 8

// -- Fixed Format defines --
#define SCSI_SENSE_DATA_ADDITIONAL_SENSE_LENGTH_OFFSET_FIXED_FORMAT 8

typedef enum _SCSI_SENSE_FORMAT
{
    SCSI_FORMAT_FIXED           = 0x00,
    SCSI_FORMAT_DESCRIPTOR      = 0x01,
    SCSI_FORMAT_INVALID         = 0xFF,
} SCSI_SENSE_FORMAT;

// *************************
// MACROS
// *************************

#define CHECK_SENSE_DATA_FORMAT(sense_data_ptr) \
    (((((sense_data_ptr)->ResponseCode & 0x7F) == 0x70) ||                     \
      (((sense_data_ptr)->ResponseCode & 0x7F) == 0x71)) ? SCSI_FORMAT_FIXED : \
     ((((sense_data_ptr)->ResponseCode & 0x7F) == 0x72) ||                     \
      (((sense_data_ptr)->ResponseCode & 0x7F) == 0x73)) ? SCSI_FORMAT_DESCRIPTOR : \
                                                           SCSI_FORMAT_INVALID)

// -- Sense Key defines --

#define SCSI_SENSE_KEY_NO_SENSE           0x00
#define SCSI_SENSE_KEY_RECOVERED_ERROR    0x01
#define SCSI_SENSE_KEY_NOT_READY          0x02
#define SCSI_SENSE_KEY_MEDIUM_ERROR       0x03
#define SCSI_SENSE_KEY_HARDWARE_ERROR     0x04
#define SCSI_SENSE_KEY_ILLEGAL_REQUEST    0x05
#define SCSI_SENSE_KEY_UNIT_ATTENTION     0x06
#define SCSI_SENSE_KEY_WRITE_PROTECT      0x07
#define SCSI_SENSE_KEY_VENDOR_SPECIFIC_09 0x09
#define SCSI_SENSE_KEY_ABORTED_CMD        0x0B

// -- Additional Sense Code defines --

#define SCSI_ASC_NO_ADDITIONAL_SENSE_INFO                 0x00
#define SCSI_ASC_DEV_WRITE_FAULT                          0x03
#define SCSI_ASC_NOT_READY                                0x04
#define SCSI_ASC_WARNING                                  0x0B
#define SCSI_ASC_WRITE_ERROR                              0x0C
#define SCSI_ASC_ID_CRC_ERROR                             0x10
#define SCSI_ASC_READ_DATA_ERROR                          0x11
#define SCSI_ASC_ID_ADDR_MARK_MISSING                     0x12
#define SCSI_ASC_DATA_ADDR_MARK_MISSING                   0x13
#define SCSI_ASC_RECORD_MISSING                           0x14
#define SCSI_ASC_SEEK_ERROR                               0x15
#define SCSI_ASC_DATA_SYNC_MARK_MISSING                   0x16
#define SCSI_ASC_RECOVERED_WITH_RETRIES                   0x17
#define SCSI_ASC_RECOVERED_WITH_ECC                       0x18
#define SCSI_ASC_DEFECT_LIST_ERROR                        0x19
#define SCSI_ASC_PRIMARY_LIST_MISSING                     0x1C
#define SCSI_ASC_ID_RECOVERED_VIA_ECC                     0x1E
#define SCSI_ASC_INVALID_COMMAND_OPERATION_CODE           0x20
#define SCSI_ASC_INVALID_FIELD_IN_CDB                     0x24
#define SCSI_ASC_INVALID_LUN                              0x25
#define SCSI_ASC_INVALID_FIELD_IN_PARAMETER_LIST          0x26
#define SCSI_ASC_PARAMETER_NOT_SUPPORTED                  0x26
#define SCSI_ASC_COMMAND_NOT_ALLOWED                      0x27
#define SCSI_ASC_POWER_ON_RESET_BUS_DEVICE_RESET_OCCURRED 0x29
#define SCSI_ASC_MODE_PARAM_CHANGED                       0x2A
#define SCSI_ASC_FORMAT_CORRUPTED                         0x31
#define SCSI_ASC_NO_SPARES_LEFT                           0x32
#define SCSI_ASC_UNSUPPORTED_ENCLOSURE_FUNCTION           0x35
#define SCSI_ASC_ENCLOSURE_SERVICES_UNAVAILABLE           0x35
#define SCSI_ASC_ENCLOSURE_SERVICES_TRANSFER_REFUSED      0x35
#define SCSI_ASC_TARGET_OPERATING_CONDITIONS_HAVE_CHANGED 0x3F
#define SCSI_ASC_MESSAGE_REJECT                           0x43
#define SCSI_ASC_INTERNAL_TARGET_FAILURE                  0x44
#define SCSI_ASC_SCSI_INTFC_PARITY_ERR                    0x47
#define SCSI_ASC_INITIATOR_DETECT_ERR                     0x48
#define SCSI_ASC_DATA_PHASE_ERROR                         0x4B
#define SCSI_ASC_LOGICAL_UNIT_FAILED_SELF_CONFIGURATION   0x4C
#define SCSI_ASC_SPINDLE_SYNCH_CHANGE                     0x5C
#define SCSI_ASC_PFA_THRESHOLD_REACHED                    0x5D
#define SCSI_ASC_GENERAL_FIRMWARE_ERROR                   0x80
#define SCSI_ASC_BRIDGED_DRIVE_ABORT_CMD_ERROR            0x88
#define SCSI_ASC_VENDOR_SPECIFIC_FF                       0xFF

// -- Additional Sense Code Qualifier defines --

#define SCSI_ASCQ_NO_ADDITIONAL_SENSE_INFO                 0x00
#define SCSI_ASCQ_POWER_ON_RESET_BUS_DEVICE_RESET_OCCURRED 0x00
#define SCSI_ASCQ_TARGET_OPERATING_CONDITIONS_HAVE_CHANGED 0x00
#define SCSI_ASCQ_INVALID_FIELD_IN_CDB                     0x00
#define SCSI_ASCQ_INVALID_FIELD_IN_PARAMETER_LIST          0x00
#define SCSI_ASCQ_INVALID_COMMAND_OPERATION_CODE           0x00
#define SCSI_ASCQ_INTERNAL_TARGET_FAILURE                  0x00
#define SCSI_ASCQ_UNSUPPORTED_ENCLOSURE_FUNCTION           0x01
#define SCSI_ASCQ_MODE_PARAM_CHANGED                       0x01
#define SCSI_ASCQ_ENCLOSURE_SERVICES_UNAVAILABLE           0x02
#define SCSI_ASCQ_PARAMETER_NOT_SUPPORTED                  0x02
#define SCSI_ASCQ_ENCLOSURE_SERVICES_TRANSFER_REFUSED      0x04
#define SCSI_ASCQ_GENERAL_QUALIFIER                        0x00
#define SCSI_ASCQ_BECOMING_READY                           0x01
#define SCSI_ASCQ_NOT_SPINNING                             0x02
#define SCSI_ASCQ_SYNCH_SUCCESS                            0x01
#define SCSI_ASCQ_SYNCH_FAIL                               0x02
#define SCSI_ASCQ_QUALIFIER_3                              0x03
#define SCSI_ASCQ_QUALIFIER_7                              0x07
#define SCSI_ASCQ_NOTIFY_ENABLE_SPINUP                     0x11
#define SCSI_ASCQ_FIFO_READ_ERROR                          0x80
#define SCSI_ASCQ_FIFO_WRITE_ERROR                         0x81
#define SCSI_ASCQ_IOEDC_READ_ERROR                         0x86
#define SCSI_ASCQ_IOEDC_WRITE_ERROR                        0x87
#define SCSI_ASCQ_HOST_PARITY_ERROR                        0x88

#endif  /* SCSI_GENERICS_H */
