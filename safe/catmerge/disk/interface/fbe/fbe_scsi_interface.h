#ifndef FBE_SCSI_INTERFACE_H
#define FBE_SCSI_INTERFACE_H

#define FBE_SCSI_SENSE_DATA_SENSE_KEY_MASK 0x00FF
#define FBE_SCSI_SENSE_KEY_MASK 0x0F

/* 
 * This string is written to the serial number field of the
 * eses_enclosure instead of a default value of 0.  If client ever
 * sees this string it suggests that Enclosure object failed to read
 * the SN field from the expander firmware.
 */
#define FBE_ESES_ENCL_SN_DEFAULT  "ENCLOBJ_NULLSN"

/* The FUA bit (Force Unit Access) is in byte 1 of some 10/16-byte CDBs.
 */
#define FBE_SCSI_FUA           0x08

/* The MSB in CDB byte[1]indicates that the command will go onto the Low Priority Queue
 */
#define FBE_SCSI_LOW_PRIORITY  0x80

/* The SPF bit in bit 6 of Log Sense(0x4D) CDB byte[2] is requried to be set for Log Sense command.
 */
#define FBE_LOG_SENSE_SPF  0x40

/* Increased the maximum transfer of inquiry data size 
 * from 120 to 128 to display properly the 8-character firmware 
 * revision number by navi.(#153573).
 */
enum fbe_scsi_data_e {
    FBE_SCSI_INQUIRY_DATA_SIZE = 136, /* Requested bytes (Max of 136 from ESES) */
    FBE_SCSI_INQUIRY_DATA_MINIMUM_SIZE = 36, /* Requested bytes (minimum of 36) */
    FBE_SCSI_INQUIRY_VENDOR_ID_SIZE = 8, /* Size of VENDOR IDENTIFICATION field in inquiry response */
    FBE_SCSI_INQUIRY_VENDOR_ID_OFFSET = 8, /* VENDOR IDENTIFICATION field offset in inquiry response */
    FBE_SCSI_INQUIRY_PRODUCT_ID_SIZE = 16, /* Size of PRODUCT IDENTIFICATION field in inquiry response */
    FBE_SCSI_INQUIRY_PRODUCT_ID_OFFSET = 16, /* PRODUCT IDENTIFICATION field offset in inquiry response */
    FBE_SCSI_INQUIRY_BOARD_TYPE_SIZE = 2, /* Size of PRODUCT IDENTIFICATION field in inquiry response */
    FBE_SCSI_INQUIRY_BOARD_TYPE_OFFSET = 114, /* PRODUCT IDENTIFICATION field offset in inquiry response */
    FBE_SCSI_INQUIRY_ENCLOSURE_PLATFORM_TYPE_SIZE = 2,
    FBE_SCSI_INQUIRY_ENCLOSURE_PLATFORM_TYPE_OFFSET = 112,
//  FBE_SCSI_INQUIRY_ENCLOSURE_UNIQUE_ID_SIZE = 4,
//  FBE_SCSI_INQUIRY_ENCLOSURE_UNIQUE_ID_OFFSET = 114,
    FBE_SCSI_INQUIRY_ESES_VERSION_DESCRIPTOR_SIZE = 2,
    FBE_SCSI_INQUIRY_ESES_VERSION_DESCRIPTOR_OFFSET = 118,
    FBE_SCSI_INQUIRY_ENCLOSURE_SERIAL_NUMBER_SIZE = 16,
    FBE_SCSI_INQUIRY_ENCLOSURE_SERIAL_NUMBER_OFFSET = 120,

    /* Read Capacity:
     * This is the number of bytes we expect back from a Read Capacity command.
     * The data indicates the last valid lba and the block length.
     */
    FBE_SCSI_READ_CAPACITY_DATA_SIZE = 8,  
    FBE_SCSI_READ_CAPACITY_DATA_SIZE_16 = 12,

    FBE_SCSI_MAX_MODE_SENSE_SIZE = 0xFF,
    FBE_SCSI_INQUIRY_REVISION_OFFSET = 32,
    FBE_SCSI_INQUIRY_REVISION_SIZE = 4,
    FBE_SCSI_INQUIRY_SERIAL_NUMBER_OFFSET = 36,
    FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE = 20,
    FBE_SCSI_INQUIRY_PART_NUMBER_OFFSET = 56,
    FBE_SCSI_INQUIRY_PART_NUMBER_SIZE = 13,
    FBE_SCSI_INQUIRY_TLA_OFFSET = 69,
    FBE_SCSI_INQUIRY_TLA_SIZE = 12,
    FBE_SCSI_DESCRIPTION_BUFF_SIZE = 32,
    FBE_SCSI_RECEIVE_DIAGNOSTIC_RESULTS_CDB_SIZE = 6,
    FBE_SCSI_SEND_DIAGNOSTIC_CDB_SIZE = 6,
    FBE_SCSI_SEND_DIAGNOSTIC_QUICK_SELF_TEST_BITS = 0x06,
    FBE_SCSI_READ_BUFFER_CDB_SIZE = 10,
    FBE_SCSI_WRITE_BUFFER_CDB_SIZE = 10,
    FBE_SCSI_MODE_SENSE_10_CDB_SIZE = 10,
    FBE_SCSI_MODE_SELECT_10_CDB_SIZE = 10,
    /* Parameters for requesting and parsing the long form
     * of mode page 0x19 with SAS PHY information*/
    FBE_SCSI_MODE_SENSE_RETURN_NO_BLOCK_DESCRIPTOR = 0x08,
    FBE_SCSI_MODE_SENSE_REQUEST_PAGE_0x19 = 0x19,
    FBE_SCSI_MODE_SENSE_SUBPAGE_0x01 = 0x01,
    FBE_SCSI_MODE_SENSE_HEADER_LENGTH = 4,    
    FBE_SCSI_MODE_SENSE_PHY_DESCRIPTOR_OFFSET = 8,
    FBE_SCSI_MODE_SENSE_PHY_DESCRIPTOR_LENGTH = 48,    
    FBE_SCSI_MODE_SENSE_HARDWARE_MAXIMUM_LINK_RATE_OFFSET = 33,
    FBE_SCSI_MODE_SENSE_HARDWARE_MAXIMUM_LINK_RATE_MASK = 0x0F,    
    /* Mode Page 0x19 long format PHY Physical link rate field parameters */
    FBE_SCSI_MODE_PAGE_0x19_PHY_SPEED_FIELD_1_5_GBPS = 0x08,
    FBE_SCSI_MODE_PAGE_0x19_PHY_SPEED_FIELD_3_GBPS = 0x09,    
    FBE_SCSI_MODE_PAGE_0x19_PHY_SPEED_FIELD_6_GBPS = 0x0A,
    FBE_SCSI_MODE_PAGE_0x19_PHY_SPEED_FIELD_12_GBPS = 0x0B,
    FBE_SCSI_INQUIRY_VPDC0_MIN_SIZE = 4,   /* Min. expected for VPD pg. C0 inquiry */
    FBE_SCSI_INQUIRY_VPD_PROD_REV_SIZE = 9,
    FBE_SCSI_INQUIRY_VPD_BRIDGE_HW_REV_SIZE = 7,
    FBE_SCSI_FORMAT_UNIT_SANITIZE_HEADER_LENGTH = 8,

    
};

/* see scsi_common.h */
typedef enum fbe_scsi_command_e {
FBE_SCSI_COPY               = 0xD8,
FBE_SCSI_DATA_MOVE          = 0xEB,    /* Data mover */
FBE_SCSI_DEFINE_GROUP       = 0xE1,
FBE_SCSI_EXTENDED_SEEK      = 0x2b,
FBE_SCSI_SEEK               = 0x0B,
FBE_SCSI_FORCE_RESERVE      = 0xE4,
FBE_SCSI_FORMAT_UNIT        = 0x04,
FBE_SCSI_INQUIRY            = 0x12,
FBE_SCSI_LOG_SENSE          = 0x4D,
FBE_SCSI_MAINT_IN           = 0xA3,    /* SCSI-3/SCC-2 command */
FBE_SCSI_MAINT_OUT          = 0xA4,    /* SCSI-3/SCC-2 command */
FBE_SCSI_MODE_SELECT_6      = 0x15,
FBE_SCSI_MODE_SELECT_10     = 0x55,
FBE_SCSI_MODE_SENSE_6       = 0x1A,
FBE_SCSI_MODE_SENSE_10      = 0x5A,
FBE_SCSI_PER_RESERVE_IN     = 0x5E,    /* SCSI-3 command */
FBE_SCSI_PER_RESERVE_OUT    = 0x5F,    /* SCSI-3 command */
FBE_SCSI_PREFETCH           = 0x34,
FBE_SCSI_READ               = 0x08,
FBE_SCSI_READ_6             = 0x08,
FBE_SCSI_READ_10            = 0x28,
FBE_SCSI_READ_12            = 0xA8,
FBE_SCSI_READ_16            = 0x88,
FBE_SCSI_READ_LONG_10       = 0x3E,
FBE_SCSI_READ_LONG_16       = 0x9E,
FBE_SCSI_READ_BUFFER        = 0x3C,
FBE_SCSI_READ_CAPACITY      = 0x25,
FBE_SCSI_READ_CAPACITY_16   = 0x9E,
FBE_SCSI_REASSIGN_BLOCKS    = 0x07,
FBE_SCSI_RCV_DIAGNOSTIC     = 0x1C,
FBE_SCSI_RELEASE            = 0x17,
FBE_SCSI_RELEASE_10         = 0x57,    /* 10 byte SCSI-3 release */
FBE_SCSI_RELEASE_GROUP      = 0xE3,
FBE_SCSI_REQUEST_SENSE      = 0x03,
FBE_SCSI_REPORT_LUNS        = 0xA0,
FBE_SCSI_RESERVE            = 0x16,
FBE_SCSI_RESERVE_10         = 0x56,    /* 10 byte SCSI-3 reserve */
FBE_SCSI_RESERVE_GROUP      = 0xE2,
FBE_SCSI_REZERO             = 0x01,
FBE_SCSI_SEND_DIAGNOSTIC    = 0x1D,
FBE_SCSI_RECEIVE_DIAGNOSTIC = 0x1C,
FBE_SCSI_SKIPMASK_READ      = 0xE8,
FBE_SCSI_SKIPMASK_WRITE     = 0xEA,
FBE_SCSI_START_STOP_UNIT    = 0x1B,
FBE_SCSI_SYNCRONIZE         = 0x35,
FBE_SCSI_TEST_UNIT_READY    = 0x00,
FBE_SCSI_VERIFY             = 0x2F,
FBE_SCSI_VERIFY_10          = 0x2F,
FBE_SCSI_VERIFY_16          = 0x8F,
FBE_SCSI_VOLUME_SET_IN      = 0xBE,    /* SCSI-3/SCC-2 command */
FBE_SCSI_VOLUME_SET_OUT     = 0xBF,    /* SCSI-3/SCC-2 command */
FBE_SCSI_WRITE              = 0x0A,
FBE_SCSI_WRITE_6            = 0x0A,
FBE_SCSI_WRITE_LONG_10      = 0x3F,
FBE_SCSI_WRITE_LONG_16      = 0x9F,
FBE_SCSI_WRITE_10           = 0x2A,
FBE_SCSI_WRITE_12           = 0xAA,
FBE_SCSI_WRITE_16           = 0x8A,
FBE_SCSI_WRITE_BUFFER       = 0x3B,
FBE_SCSI_WRITE_SAME         = 0x41,
FBE_SCSI_WRITE_SAME_10      = 0x41,
FBE_SCSI_WRITE_SAME_16      = 0x93,
FBE_SCSI_WRITE_VERIFY       = 0x2E,
FBE_SCSI_WRITE_VERIFY_10    = 0x2E,
FBE_SCSI_WRITE_VERIFY_16    = 0x8E,
FBE_SCSI_UNMAP              = 0x42,
FBE_SCSI_SMART_DUMP         = 0xE6
}fbe_scsi_command_t;

typedef enum fbe_scsi_sense_data_offset_e {
    FBE_SCSI_SENSE_DATA_RESPONSE_CODE_OFFSET = 0,
    FBE_SCSI_SENSE_DATA_SENSE_KEY_OFFSET = 2,
    FBE_SCSI_SENSE_DATA_ADDL_SENSE_LENGTH_OFFSET = 7,
    FBE_SCSI_SENSE_DATA_ASC_OFFSET = 12,
    FBE_SCSI_SENSE_DATA_ASCQ_OFFSET = 13,
    FBE_SCSI_SENSE_DATA_SENSE_KEY_OFFSET_DESCRIPTOR_FORMAT = 1, 
    FBE_SCSI_SENSE_DATA_ASC_OFFSET_DESCRIPTOR_FORMAT = 2,   
    FBE_SCSI_SENSE_DATA_ASCQ_OFFSET_DESCRIPTOR_FORMAT = 3,
    FBE_SCSI_SENSE_DATA_ADDITIONAL_SENSE_LENGTH_OFFSET_DESCRIPTOR_FORMAT = 7,
    FBE_SCSI_SENSE_DATA_DESCRIPTOR_TYPE_OFFSET_DESCRIPTOR_FORMAT = 8,
    FBE_SCSI_SENSE_DATA_VALID_BIT_OFFSET_DESCRIPTOR_FORMAT = 10,
    FBE_SCSI_SENSE_DATA_BAD_LBA_MSB_OFFSET_DESCRIPTOR_FORMAT = 12,  
} fbe_scsi_sense_data_offset_t;

typedef enum fbe_scsi_sense_key_e {
    FBE_SCSI_SENSE_KEY_NO_SENSE       = 0x00,
    FBE_SCSI_SENSE_KEY_RECOVERED_ERROR = 0x01,
    FBE_SCSI_SENSE_KEY_NOT_READY       = 0x02,
    FBE_SCSI_SENSE_KEY_MEDIUM_ERROR    = 0x03,
    FBE_SCSI_SENSE_KEY_HARDWARE_ERROR  = 0x04,
    FBE_SCSI_SENSE_KEY_ILLEGAL_REQUEST = 0x05,
    FBE_SCSI_SENSE_KEY_UNIT_ATTENTION  = 0x06,
    FBE_SCSI_SENSE_KEY_WRITE_PROTECT   = 0x07,
    FBE_SCSI_SENSE_KEY_VENDOR_SPECIFIC_09 = 0x09,
    FBE_SCSI_SENSE_KEY_ABORTED_CMD     = 0x0B
}fbe_scsi_sense_key_t;

typedef enum fbe_scsi_additional_sense_code_e {
    /* See ESES spec. It is not a complete list yet, please add other codes here.  */
    FBE_SCSI_ASC_NO_ADDITIONAL_SENSE_INFO                 = 0x00,
    FBE_SCSI_ASC_DEV_WRITE_FAULT                          = 0x03,
    FBE_SCSI_ASC_NOT_READY                                = 0x04,
    FBE_SCSI_ASC_WARNING                                  = 0x0B,
    FBE_SCSI_ASC_WRITE_ERROR                              = 0x0C,
    FBE_SCSI_ASC_ID_CRC_ERROR                             = 0x10,
    FBE_SCSI_ASC_READ_DATA_ERROR                          = 0x11,
    FBE_SCSI_ASC_ID_ADDR_MARK_MISSING                     = 0x12,
    FBE_SCSI_ASC_DATA_ADDR_MARK_MISSING                   = 0x13,
    FBE_SCSI_ASC_RECORD_MISSING                           = 0x14,
    FBE_SCSI_ASC_SEEK_ERROR                               = 0x15,
    FBE_SCSI_ASC_DATA_SYNC_MARK_MISSING                   = 0x16,
    FBE_SCSI_ASC_RECOVERED_WITH_RETRIES                   = 0x17,
    FBE_SCSI_ASC_RECOVERED_WITH_ECC                       = 0x18,
    FBE_SCSI_ASC_DEFECT_LIST_ERROR                        = 0x19,
    FBE_SCSI_ASC_PRIMARY_LIST_MISSING                     = 0x1C,
    FBE_SCSI_ASC_ID_RECOVERED_VIA_ECC                     = 0x1E,
    FBE_SCSI_ASC_INVALID_COMMAND_OPERATION_CODE           = 0x20,
    FBE_SCSI_ASC_INVALID_FIELD_IN_CDB                     = 0x24,
    FBE_SCSI_ASC_INVALID_LUN                              = 0x25,
    FBE_SCSI_ASC_INVALID_FIELD_IN_PARAMETER_LIST          = 0x26,
    FBE_SCSI_ASC_PARAMETER_NOT_SUPPORTED                  = 0x26,
    FBE_SCSI_ASC_COMMAND_NOT_ALLOWED                      = 0x27,
    FBE_SCSI_ASC_POWER_ON_RESET_BUS_DEVICE_RESET_OCCURRED = 0x29,
    FBE_SCSI_ASC_MODE_PARAM_CHANGED                       = 0x2A,
    FBE_SCSI_ASC_FORMAT_CORRUPTED                         = 0x31,
    FBE_SCSI_ASC_NO_SPARES_LEFT                           = 0x32,
    FBE_SCSI_ASC_UNSUPPORTED_ENCLOSURE_FUNCTION           = 0x35,
    FBE_SCSI_ASC_ENCLOSURE_SERVICES_UNAVAILABLE           = 0x35,
    FBE_SCSI_ASC_ENCLOSURE_SERVICES_TRANSFER_REFUSED      = 0x35,
    FBE_SCSI_ASC_LOGICAL_UNIT_FAILURE                     = 0x3E,
    FBE_SCSI_ASC_TARGET_OPERATING_CONDITIONS_HAVE_CHANGED = 0x3F,
    FBE_SCSI_ASC_MESSAGE_REJECT                           = 0x43,
    FBE_SCSI_ASC_INTERNAL_TARGET_FAILURE                  = 0x44,
    FBE_SCSI_ASC_SCSI_INTFC_PARITY_ERR                    = 0x47,
    FBE_SCSI_ASC_INITIATOR_DETECT_ERR                     = 0x48,
    FBE_SCSI_ASC_DATA_PHASE_ERROR                         = 0x4B,
    FBE_SCSI_ASC_LOGICAL_UNIT_FAILED_SELF_CONFIGURATION   = 0x4C,
    FBE_SCSI_ASC_SPINDLE_SYNCH_CHANGE                     = 0x5C,
    FBE_SCSI_ASC_PFA_THRESHOLD_REACHED                    = 0x5D,
    FBE_SCSI_ASC_GENERAL_FIRMWARE_ERROR                   = 0x80,
    FBE_SCSI_ASC_BRIDGED_DRIVE_ABORT_CMD_ERROR            = 0x88,
    FBE_SCSI_ASC_VENDOR_SPECIFIC_FF                       = 0xFF
}fbe_scsi_additional_sense_code_t;

typedef enum fbe_scsi_additional_sense_code_qualifier_e {
    /* See ESES spec. It is not a complete list yet, please add other code qualifiers here.  */
    FBE_SCSI_ASCQ_NO_ADDITIONAL_SENSE_INFO                 = 0x00,
    FBE_SCSI_ASCQ_POWER_ON_RESET_BUS_DEVICE_RESET_OCCURRED = 0x00,
    FBE_SCSI_ASCQ_TARGET_OPERATING_CONDITIONS_HAVE_CHANGED = 0x00,
    FBE_SCSI_ASCQ_INVALID_FIELD_IN_CDB                     = 0x00,
    FBE_SCSI_ASCQ_INVALID_FIELD_IN_PARAMETER_LIST          = 0x00,
    FBE_SCSI_ASCQ_INVALID_COMMAND_OPERATION_CODE           = 0x00,
    FBE_SCSI_ASCQ_INTERNAL_TARGET_FAILURE                  = 0x00, 
    FBE_SCSI_ASCQ_UNSUPPORTED_ENCLOSURE_FUNCTION           = 0x01,
    FBE_SCSI_ASCQ_MODE_PARAM_CHANGED                       = 0x01,                
    FBE_SCSI_ASCQ_ENCLOSURE_SERVICES_UNAVAILABLE           = 0x02,
    FBE_SCSI_ASCQ_PARAMETER_NOT_SUPPORTED                  = 0x02,
    FBE_SCSI_ASCQ_ENCLOSURE_SERVICES_TRANSFER_REFUSED      = 0x04,

    FBE_SCSI_ASCQ_GENERAL_QUALIFIER                        = 0x00,
    FBE_SCSI_ASCQ_BECOMING_READY                           = 0x01,
    FBE_SCSI_ASCQ_NOT_SPINNING                             = 0x02,
    FBE_SCSI_ASCQ_SYNCH_SUCCESS                            = 0x01,
    FBE_SCSI_ASCQ_SYNCH_FAIL                               = 0x02,
    FBE_SCSI_ASCQ_QUALIFIER_3                              = 0x03,
    FBE_SCSI_ASCQ_QUALIFIER_7                              = 0x07,
    FBE_SCSI_ASCQ_NOTIFY_ENABLE_SPINUP                     = 0x11,
    FBE_SCSI_ASCQ_FIFO_READ_ERROR                          = 0x80,
    FBE_SCSI_ASCQ_FIFO_WRITE_ERROR                         = 0x81,
    FBE_SCSI_ASCQ_IOEDC_READ_ERROR                         = 0x86,
    FBE_SCSI_ASCQ_IOEDC_WRITE_ERROR                        = 0x87,
    FBE_SCSI_ASCQ_HOST_PARITY_ERROR                        = 0x88,
    FBE_SCSI_ASCQ_DIE_RETIREMENT_START                     = 0xFD,
    FBE_SCSI_ASCQ_DIE_RETIREMENT_END                       = 0xFE
    
}fbe_scsi_additional_sense_code_qualifier_t;


/*********************************
 *    SCSI Driver Error Codes    *
 *********************************/
typedef enum fbe_scsi_error_code_e {
    /* Assorted SCSI protocol errors. 
    */
    FBE_SCSI_UNEXPECTEDDISCONNECT       = 0xfffffffe,   /* Target dropped the bus */
    FBE_SCSI_BADINTERRUPT               = 0xfffffffd,   /* strange/unknown interrupt */
    FBE_SCSI_BADBUSPHASE                = 0xfffffffc,   /* incorrect bus phase */
    FBE_SCSI_BADMESSAGE                 = 0xfffffffb,   /* incorrect message received */
    FBE_SCSI_BADSTATUS                  = 0xfffffffa,   /* incorrect status byte */
    FBE_SCSI_BADRESELECTION             = 0xfffffff9,   /* inactive device reselected */
    FBE_SCSI_PARITYERROR                = 0xfffffff8,   /* par err during data phase */
    FBE_SCSI_MSGPARERR                  = 0xfffffff7,   /* par err during msg. phase */
    FBE_SCSI_STATUSPARITYERROR          = 0xfffffff6,   /* par err during stat. phase */
    FBE_SCSI_INT_SCB_NOT_FOUND          = 0xfffffff5,   /* rec'd interrupt from device with no outstanding IO */
    /* Fairly common driver errors
     */
    FBE_SCSI_DEVICE_NOT_PRESENT         = 0xfffffff4,   /* disk not responding */
    FBE_SCSI_SELECTIONTIMEOUT           = 0xfffffff4,   
    FBE_SCSI_BUSRESET                   = 0xfffffff3,   /* IO killed due to bus reset */
    FBE_SCSI_DRVABORT                   = 0xfffffff3,   
    FBE_SCSI_USERRESET                  = 0xfffffff2,   /* caller requested bus reset */
    FBE_SCSI_USERABORT                  = 0xfffffff1,   /* caller aborted IO request */
    /* These errors could be due to a crazy drive,
     * OR they could be Flare's fault
     */
    FBE_SCSI_XFERCOUNTNOTZERO           = 0xfffffff0,   /* didn't xfer enough bytes */
    FBE_SCSI_TOOMUCHDATA                = 0xffffffef,   /* tried to xfer too much data */
    /* Other misc. errors 
     */
    FBE_SCSI_BUSSHUTDOWN                = 0xffffffee,   /* bus for IO request is down */
    FBE_SCSI_INVALIDSCB                 = 0xffffffed,   /* bad input data in the SCB */
    FBE_SCSI_SLOT_BUSY                  = 0xffffffec,   /* No slot available to accept request */
    FBE_SCSI_CHANNEL_INACCESSIBLE       = 0xffffffeb,   /* Fibre loop is down */
    FBE_SCSI_FCP_INVALIDRSP             = 0xffffffea,   /* the fibre channel FCP_CMD has invalid response code */
    FBE_SCSI_DEVICE_NOT_READY           = 0xffffffe9,   /* the device is not in an appropriate state */
    FBE_SCSI_IO_TIMEOUT_ABORT           = 0xffffffe8,   /* the IO timed out and is aborted. */
    FBE_SCSI_IO_LINKDOWN_ABORT          = 0xffffffe7,   /* IO aborted due to fibre link down. */
    FBE_SCSI_SATA_IO_TIMEOUT_ABORT      = 0xffffffe6,   /* the IO timed out and is aborted. */
    /* Hemi related SCSI driver errors, so we can pretend
     * this is a "real" driver
     */
    FBE_SCSI_NO_DEVICE_DRIVER           = 0xffffffdf,
    FBE_SCSI_UNSUPPORTEDFUNCTION        = 0xffffffde,
    FBE_SCSI_UNSUPPORTED_IOCTL_OPTION   = 0xffffffdd,
    /* Some additional codes to handle some unusual errors from the
     * miniport driver
     */
    FBE_SCSI_INVALIDREQUEST             = 0xffffffdc,
    FBE_SCSI_UNKNOWNRESPONSE            = 0xffffffdb,
    /* Error from SFD indicates no device stack.
     */
    FBE_SCSI_DEVUNITIALIZED             = 0xffffffda,

    FBE_SCSI_INCIDENTAL_ABORT           = 0xffffffd9,    /* miniport aborted as Incidental Abort */

    FBE_SCSI_ENCRYPTION_NOT_ENABLED     = 0xffffffd8,
    FBE_SCSI_ENCRYPTION_BAD_KEY_HANDLE  = 0xffffffd7,
    FBE_SCSI_ENCRYPTION_KEY_WRAP_ERROR  = 0xffffffd6,
    /* These are for device reserved and busy status from the drive.
     */
    FBE_SCSI_DEVICE_RESERVED            = 0x80000000,
    FBE_SCSI_DEVICE_BUSY                = 0x80000001,
    /* More driver errors - these are all for Check Condition/auto sense results.
     */
    FBE_SCSI_AUTO_SENSE_FAILED          = 0x80000002,
    FBE_SCSI_CHECK_CONDITION_PENDING    = 0x80000003,
    FBE_SCSI_CHECK_COND_OTHER_SLOT      = 0x80000004,
    /* Positive error codes indicate the drive returned Check Condition status,
     * and the SCSI driver successfully got the sense data via auto request sense.
     */
    FBE_SCSI_CC_NOERR                        = 0x101,
    FBE_SCSI_CC_AUTO_REMAPPED                = 0x102,
    FBE_SCSI_CC_RECOVERED_BAD_BLOCK          = 0x103,
    FBE_SCSI_CC_RECOVERED_ERR_CANT_REMAP     = 0x104,
    FBE_SCSI_CC_PFA_THRESHOLD_REACHED        = 0x105,
    FBE_SCSI_CC_RECOVERED_ERR_NOSYNCH        = 0x106,
    FBE_SCSI_CC_RECOVERED_ERR                = 0x107,
    FBE_SCSI_CC_BECOMING_READY               = 0x108,
    FBE_SCSI_CC_NOT_SPINNING                 = 0x109,
    FBE_SCSI_CC_NOT_READY                    = 0x10a,
    FBE_SCSI_CC_FORMAT_CORRUPTED             = 0x10b,
    FBE_SCSI_CC_MEDIA_ERROR_BAD_DEFECT_LIST  = 0x10c,
    FBE_SCSI_CC_HARD_BAD_BLOCK               = 0x10d,
    FBE_SCSI_CC_MEDIA_ERR_CANT_REMAP         = 0x10e,
    FBE_SCSI_CC_HARDWARE_ERROR_PARITY        = 0x10f,
    FBE_SCSI_CC_HARDWARE_ERROR               = 0x110,
    FBE_SCSI_CC_ILLEGAL_REQUEST              = 0x111,
    FBE_SCSI_CC_DEV_RESET                    = 0x112,
    FBE_SCSI_CC_MODE_SELECT_OCCURRED         = 0x113,
    FBE_SCSI_CC_SYNCH_SUCCESS                = 0x114,
    FBE_SCSI_CC_SYNCH_FAIL                   = 0x115,
    FBE_SCSI_CC_UNIT_ATTENTION               = 0x116,
    FBE_SCSI_CC_ABORTED_CMD_PARITY_ERROR     = 0x117,
    FBE_SCSI_CC_ABORTED_CMD                  = 0x118,
    FBE_SCSI_CC_UNEXPECTED_SENSE_KEY         = 0x119,
    FBE_SCSI_CC_MEDIA_ERR_DO_REMAP           = 0x11a,
    FBE_SCSI_CC_MEDIA_ERR_DONT_REMAP         = 0x11b,
    FBE_SCSI_CC_HARDWARE_ERROR_FIRMWARE      = 0x11c,
    FBE_SCSI_CC_HARDWARE_ERROR_NO_SPARE      = 0x11d,
    FBE_SCSI_CC_GENERAL_FIRMWARE_ERROR       = 0x11e,
    FBE_SCSI_CC_MEDIA_ERR_WRITE_ERROR        = 0x11f,
    FBE_SCSI_CC_DEFECT_LIST_ERROR            = 0x120,
    FBE_SCSI_CC_VENDOR_SPECIFIC_09_80_00     = 0x121,
    FBE_SCSI_CC_SEEK_POSITIONING_ERROR       = 0x122,
    FBE_SCSI_CC_SEL_ID_ERROR                 = 0x123,
    FBE_SCSI_CC_RECOVERED_WRITE_FAULT        = 0x124,
    FBE_SCSI_CC_MEDIA_ERROR_WRITE_FAULT      = 0x125,
    FBE_SCSI_CC_HARDWARE_ERROR_WRITE_FAULT   = 0x126,
    FBE_SCSI_CC_INTERNAL_TARGET_FAILURE      = 0x127,
    FBE_SCSI_CC_RECOVERED_DEFECT_LIST_ERROR  = 0x128,
    FBE_SCSI_CC_ABORTED_CMD_ATA_TO           = 0x129,
    FBE_SCSI_CC_INVALID_LUN                  = 0x12a,
    FBE_SCSI_LOGICAL_UNIT_FAILED_SELF_CONFIGURATION  = 0x12b,
    FBE_SCSI_CC_DATA_OFFSET_ERROR            = 0x12c,
    FBE_SCSI_CC_SUPER_CAP_FAILURE            = 0x12d,
    FBE_SCSI_CC_DRIVE_TABLE_REBUILD          = 0x12e,
    FBE_SCSI_CC_DEFERRED_ERROR               = 0x12f,
    FBE_SCSI_CC_WRITE_PROTECT                = 0x130,
    FBE_SCSI_CC_SENSE_DATA_MISSING           = 0x131,    
    FBE_SCSI_CC_TEMP_THRESHOLD_REACHED       = 0x132,
    FBE_SCSI_CC_DIE_RETIREMENT_START         = 0x133,
    FBE_SCSI_CC_DIE_RETIREMENT_END           = 0x134,
    FBE_SCSI_CC_HARDWARE_ERROR_SELF_TEST     = 0x135,
    FBE_SCSI_CC_FORMAT_IN_PROGRESS           = 0x136,
    FBE_SCSI_CC_SANITIZE_INTERRUPTED         = 0x137,

    /* 
     * The Enclosure shares some of the error codes above. 
     * The following code is specific to enclosures. 
     */
    FBE_SCSI_CC_TARGET_OPERATING_COND_CHANGED = 0x200,
    FBE_SCSI_CC_UNSUPPORTED_ENCL_FUNC         = 0x201,
    FBE_SCSI_CC_MODE_PARAM_CHANGED            = 0x202,
    FBE_SCSI_CC_ENCLOSURE_SERVICES_TRANSFER_REFUSED = 0x203,
}fbe_scsi_error_code_t;

/******************************************************************************//**
 * SCSI Inquiry VPD page codes.
 *
 * 00h - Supported VPD page                     <br>
 * 80h - Unit Serial Number VPD Page            <br>
 * 83h - Device Identification VPD Page         <br>
 * 86h - Extended Inquiry VPD Page              <br>
 * C0h - DFh - SubEnclosure Specific VPD Pages  <br>
 *******************************************************************************/
typedef enum {
    // SCSI Inquiry VPD pages 0x00 - 0xDF
    FBE_SCSI_INQUIRY_VPD_PG_SUPPORTED       = 0x00,
    FBE_SCSI_INQUIRY_VPD_PG_UNIT_SERIALNO   = 0x80,
    FBE_SCSI_INQUIRY_VPD_PG_DEVICE_ID       = 0x83,
    FBE_SCSI_INQUIRY_VPD_PG_EXTENDED        = 0x86,
    FBE_SCSI_INQUIRY_VPD_PG_SUB_ENCL        = 0xC0, /* C0h - DFh */
    /// The maximum page code for VPD pages we store persistently
    FBE_SCSI_INQUIRY_VPD_PG_INVALID         = 0xFF
} fbe_scsi_inquiry_vpd_pg_enum;

/********************************************************************************
 * Mode Sense (10) page codes.
 *
 * 00h - Enclosure Services Management Mode Page    <br>
 * 01h - Protocol-Specific Port Mode Page           <br>
 * 20h - EMC ESES Persistent Mode Page              <br>
 * 21h - EMC ESES Non-Persistent Mode Page          <br>
 *******************************************************************************/
typedef enum {
    // Mode Sense (10) pages 0x00 - 0xDF
    FBE_SCSI_MODE_SENSE_10_PG_ESM         = 0x00,    /* Enclosure Services Management Mode */            
    FBE_SCSI_MODE_SENSE_10_PG_PSP_SAS_SSP = 0x01,    /* Protocol Specific Port Mode for SAS SSP */
    FBE_SCSI_MODE_SENSE_10_PG_EEP         = 0x20,    /* 20h - EMC ESES Persistent Mode */
    FBE_SCSI_MODE_SENSE_10_PG_EENP        = 0x21,    /* 21h - EMC ESES Non-Persistent Mode */
    FBE_SCSI_MODE_SENSE_10_PG_INVALID     = 0xFF,
} fbe_scsi_mode_sense_10_pg_enum;


/********************************************************************************
 * Format Unit (opcode 0x04) Sanitize Header Parameter Types
 *
 *******************************************************************************/
typedef fbe_u8_t fbe_scsi_sanitize_pattern_t;
#define FBE_SCSI_FORMAT_UNIT_SANITIZE_PATTERN_ERASE_ONLY          ((fbe_scsi_sanitize_pattern_t)0x01)
#define FBE_SCSI_FORMAT_UNIT_SANITIZE_PATTERN_OVERWRITE_AND_ERASE ((fbe_scsi_sanitize_pattern_t)0x02)    /* DoD */
#define FBE_SCSI_FORMAT_UNIT_SANITIZE_PATTERN_AFSSI               ((fbe_scsi_sanitize_pattern_t)0x03)    /* AFSSI 5020 */
#define FBE_SCSI_FORMAT_UNIT_SANITIZE_PATTERN_NSA                 ((fbe_scsi_sanitize_pattern_t)0x04)    /* NSA 130-2 */
#define FBE_SCSI_FORMAT_UNIT_SANITIZE_PATTERN_INVALID             ((fbe_scsi_sanitize_pattern_t)0xFF)    /* NSA 130-2 */

/******************************************************************************** 
  *  SCSI Structures from SPC and SBC t10 specs.
  *  Note:  Any integer values should be converted to the correct endian.
  *         DO NOT use directly by casting. 
  *   
  *******************************************************************************/
#pragma pack(1)
typedef struct fbe_scsi_mode_parameter_header_10_s
{
    fbe_u8_t data_len[2];
    fbe_u8_t medium_type;
    fbe_u8_t device_specific_param;
    fbe_u8_t reserved[2];
    fbe_u8_t block_descriptor_len[2];
} fbe_scsi_mode_parameter_header_10_t;
#pragma pack()

#pragma pack(1)
typedef struct fbe_scsi_mode_parameter_block_descriptor_short_lba_s
{
    fbe_u8_t num_blocks[4];
    fbe_u8_t reserved;
    fbe_u8_t block_len[3];           
} fbe_scsi_mode_parameter_block_descriptor_short_lba_t;
#pragma pack()

#pragma pack(1)
typedef struct fbe_scsi_mode_parameter_list_10_s
{
    fbe_scsi_mode_parameter_header_10_t                      header;
    fbe_scsi_mode_parameter_block_descriptor_short_lba_t     block_descriptor;                
} fbe_scsi_mode_parameter_list_10_t;
#pragma pack()


#endif /* FBE_SCSI_INTERFACE_H */
