#ifndef FBE_PAYLOAD_CDB_OPERATION_H
#define FBE_PAYLOAD_CDB_OPERATION_H

#include "fbe/fbe_types.h"
#include "fbe/fbe_time.h"
#include "fbe/fbe_payload_sg_descriptor.h"
#include "fbe/fbe_payload_operation_header.h"
#include "fbe/fbe_port.h"
#include "fbe/fbe_scsi_interface.h"

typedef enum fbe_payload_cdb_constants_e {
    FBE_PAYLOAD_CDB_CDB_SIZE = 16,
    FBE_PAYLOAD_CDB_SENSE_INFO_BUFFER_SIZE = 48
}fbe_payload_cdb_constants_t;

/* SCSI relates definitions from SPC-3 and SAM-3 */
typedef enum fbe_payload_cdb_task_attribute_e {
    FBE_PAYLOAD_CDB_TASK_ATTRIBUTE_INVALID,

    FBE_PAYLOAD_CDB_TASK_ATTRIBUTE_SIMPLE,          /* windows SRB_SIMPLE_TAG_REQUEST */
    FBE_PAYLOAD_CDB_TASK_ATTRIBUTE_ORDERED,     /* windows SRB_ORDERED_QUEUE_TAG_REQUEST */
    FBE_PAYLOAD_CDB_TASK_ATTRIBUTE_HEAD_OF_QUEUE,   /* windows SRB_HEAD_OF_QUEUE_TAG_REQUEST */

    FBE_PAYLOAD_CDB_TASK_ATTRIBUTE_LAST
}fbe_payload_cdb_task_attribute_t;


/* Command priority specifies the relative scheduling importance of a command having a SIMPLE task attribute in
 * relation to other commands having SIMPLE task attributes already in the task set. If the command has a task
 * attribute other than SIMPLE, then command priority is not used. Command priority is a value in the range of 0h
 * through Fh.
 * Value Description
 * 0h - A command with either no command priority or a command with a vendor-specific level of scheduling importance.
 * 1h - A command with the highest scheduling importance.
 * ...  A command with decreased scheduling importance.
 * Fh - A command with the lowest scheduling importance.
 *
 * NOTE, The priorities are mapped to fit the 2 levels of priority supported by EMC Enhanced Queuing, normal and low.
 */
typedef enum fbe_payload_cdb_priority_s {
    FBE_PAYLOAD_CDB_PRIORITY_NONE   = 0x0,
    FBE_PAYLOAD_CDB_PRIORITY_URGENT = 0x1,
    FBE_PAYLOAD_CDB_PRIORITY_NORMAL = 0x2,
    FBE_PAYLOAD_CDB_PRIORITY_LOW    = 0x3,
    FBE_PAYLOAD_CDB_PRIORITY_LAST   = 0xF
} fbe_payload_cdb_priority_t;


typedef enum fbe_payload_cdb_scsi_status_e {
    FBE_PAYLOAD_CDB_SCSI_STATUS_GOOD = 0x00,
    FBE_PAYLOAD_CDB_SCSI_STATUS_CHECK_CONDITION = 0x02,
    FBE_PAYLOAD_CDB_SCSI_STATUS_CONDITION_MET = 0x04,
    FBE_PAYLOAD_CDB_SCSI_STATUS_BUSY = 0x08,
    FBE_PAYLOAD_CDB_SCSI_STATUS_INTERMEDIATE = 0x10,
    FBE_PAYLOAD_CDB_SCSI_STATUS_INTERMEDIATE_CONDITION_MET = 0x14,
    FBE_PAYLOAD_CDB_SCSI_STATUS_RESERVATION_CONFLICT = 0x18,
    FBE_PAYLOAD_CDB_SCSI_STATUS_OBSOLETE = 0x22,
    FBE_PAYLOAD_CDB_SCSI_STATUS_TASK_SET_FULL = 0x28,
    FBE_PAYLOAD_CDB_SCSI_STATUS_ACA_ACTIVE = 0x30,
    FBE_PAYLOAD_CDB_SCSI_STATUS_TASK_ABORTED = 0x40
}fbe_payload_cdb_scsi_status_t;

/* End of SCSI relates definitions from SPC-3 and SAM-3 */


/* We have to define our own flags to be independent from SRB_FLAGS */
typedef enum fbe_payload_cdb_flags_e {
    FBE_PAYLOAD_CDB_FLAGS_QUEUE_ACTION_ENABLE       = 0x00000002,
    FBE_PAYLOAD_CDB_FLAGS_DISABLE_DISCONNECT        = 0x00000004,
    FBE_PAYLOAD_CDB_FLAGS_DISABLE_SYNCH_TRANSFER    = 0x00000008,

    FBE_PAYLOAD_CDB_FLAGS_DISABLE_AUTOSENSE         = 0x00000020,
    FBE_PAYLOAD_CDB_FLAGS_DATA_IN                   = 0x00000040,
    FBE_PAYLOAD_CDB_FLAGS_DATA_OUT                  = 0x00000080,
    FBE_PAYLOAD_CDB_FLAGS_NO_DATA_TRANSFER          = 0x00000000,
    FBE_PAYLOAD_CDB_FLAGS_UNSPECIFIED_DIRECTION     = 0x000000C0, /* (FBE_PAYLOAD_CDB_FLAGS_DATA_IN | FBE_PAYLOAD_CDB_FLAGS_DATA_OUT) */

    FBE_PAYLOAD_CDB_FLAGS_ADAPTER_CACHE_ENABLE      = 0x00000200,
    FBE_PAYLOAD_CDB_FLAGS_FREE_SENSE_BUFFER         = 0x00000400,
    FBE_PAYLOAD_CDB_FLAGS_DRIVE_FW_UPGRADE          = 0x00001000, /* Flag to inform the miniport bypass the write buffer size check, dims 239263 */
    FBE_PAYLOAD_CDB_FLAGS_ALLOW_UNMAP_READ          = 0x00002000,

    FBE_PAYLOAD_CDB_FLAGS_PORT_DRIVER_ALLOCSENSE    = 0x00200000,
    FBE_PAYLOAD_CDB_FLAGS_PORT_DRIVER_SENSEHASPORT  = 0x00400000,
    FBE_PAYLOAD_CDB_FLAGS_DONT_START_NEXT_PACKET    = 0x00800000,

	FBE_PAYLOAD_CDB_FLAGS_BLOCK_SIZE_4160           = 0x01000000,

    FBE_PAYLOAD_CDB_FLAGS_PORT_DRIVER_RESERVED      = 0x0F000000,
    FBE_PAYLOAD_CDB_FLAGS_CLASS_DRIVER_RESERVED     = 0xF0000000,
}fbe_payload_cdb_flags_t;

/* SCSI Positive error codes indicate the drive returned Check Condition status,
 * and the SCSI driver successfully got the sense data via auto request sense.
 */
typedef enum fbe_payload_cdb_scsi_cc_code_e {
    FBE_PAYLOAD_CDB_SCSI_CC_NOERR                        = 0x101,
    FBE_PAYLOAD_CDB_SCSI_CC_AUTO_REMAPPED                = 0x102,
    FBE_PAYLOAD_CDB_SCSI_CC_RECOVERED_BAD_BLOCK          = 0x103,
    FBE_PAYLOAD_CDB_SCSI_CC_RECOVERED_ERR_CANT_REMAP     = 0x104,
    FBE_PAYLOAD_CDB_SCSI_CC_PFA_THRESHOLD_REACHED        = 0x105,
    FBE_PAYLOAD_CDB_SCSI_CC_RECOVERED_ERR_NOSYNCH        = 0x106,
    FBE_PAYLOAD_CDB_SCSI_CC_RECOVERED_ERR                = 0x107,
    FBE_PAYLOAD_CDB_SCSI_CC_BECOMING_READY               = 0x108,
    FBE_PAYLOAD_CDB_SCSI_CC_NOT_SPINNING                 = 0x109,
    FBE_PAYLOAD_CDB_SCSI_CC_NOT_READY                    = 0x10a,
    FBE_PAYLOAD_CDB_SCSI_CC_FORMAT_CORRUPTED             = 0x10b,
    FBE_PAYLOAD_CDB_SCSI_CC_MEDIA_ERROR_BAD_DEFECT_LIST  = 0x10c,
    FBE_PAYLOAD_CDB_SCSI_CC_HARD_BAD_BLOCK               = 0x10d,
    FBE_PAYLOAD_CDB_SCSI_CC_MEDIA_ERR_CANT_REMAP         = 0x10e,
    FBE_PAYLOAD_CDB_SCSI_CC_HARDWARE_ERROR_PARITY        = 0x10f,
    FBE_PAYLOAD_CDB_SCSI_CC_HARDWARE_ERROR               = 0x110,
    FBE_PAYLOAD_CDB_SCSI_CC_ILLEGAL_REQUEST              = 0x111,
    FBE_PAYLOAD_CDB_SCSI_CC_DEV_RESET                    = 0x112,
    FBE_PAYLOAD_CDB_SCSI_CC_MODE_SELECT_OCCURRED         = 0x113,
    FBE_PAYLOAD_CDB_SCSI_CC_SYNCH_SUCCESS                = 0x114,
    FBE_PAYLOAD_CDB_SCSI_CC_SYNCH_FAIL                   = 0x115,
    FBE_PAYLOAD_CDB_SCSI_CC_UNIT_ATTENTION               = 0x116,
    FBE_PAYLOAD_CDB_SCSI_CC_ABORTED_CMD_PARITY_ERROR     = 0x117,
    FBE_PAYLOAD_CDB_SCSI_CC_ABORTED_CMD                  = 0x118,
    FBE_PAYLOAD_CDB_SCSI_CC_UNEXPECTED_SENSE_KEY         = 0x119,
    FBE_PAYLOAD_CDB_SCSI_CC_MEDIA_ERR_DO_REMAP           = 0x11a,
    FBE_PAYLOAD_CDB_SCSI_CC_MEDIA_ERR_DONT_REMAP         = 0x11b,
    FBE_PAYLOAD_CDB_SCSI_CC_HARDWARE_ERROR_FIRMWARE      = 0x11c,
    FBE_PAYLOAD_CDB_SCSI_CC_HARDWARE_ERROR_NO_SPARE      = 0x11d,
    FBE_PAYLOAD_CDB_SCSI_CC_GENERAL_FIRMWARE_ERROR       = 0x11e,
    FBE_PAYLOAD_CDB_SCSI_CC_MEDIA_ERR_WRITE_ERROR        = 0x11f,
    FBE_PAYLOAD_CDB_SCSI_CC_DEFECT_LIST_ERROR            = 0x120,
    FBE_PAYLOAD_CDB_SCSI_CC_VENDOR_SPECIFIC_09_80_00     = 0x121,
    FBE_PAYLOAD_CDB_SCSI_CC_SEEK_POSITIONING_ERROR       = 0x122,
    FBE_PAYLOAD_CDB_SCSI_CC_SEL_ID_ERROR                 = 0x123,
    FBE_PAYLOAD_CDB_SCSI_CC_RECOVERED_WRITE_FAULT        = 0x124,
    FBE_PAYLOAD_CDB_SCSI_CC_MEDIA_ERROR_WRITE_FAULT      = 0x125,
    FBE_PAYLOAD_CDB_SCSI_CC_HARDWARE_ERROR_WRITE_FAULT   = 0x126,
    FBE_PAYLOAD_CDB_SCSI_CC_INTERNAL_TARGET_FAILURE      = 0x127,
    FBE_PAYLOAD_CDB_SCSI_CC_RECOVERED_DEFECT_LIST_ERROR  = 0x128,
    FBE_PAYLOAD_CDB_SCSI_CC_ABORTED_CMD_ATA_TO           = 0x129,
    FBE_PAYLOAD_CDB_SCSI_CC_INVALID_LUN                  = 0x12a,
    FBE_PAYLOAD_CDB_SCSI_LOGICAL_UNIT_FAILED_SELF_CONFIGURATION  = 0x12b,
    FBE_PAYLOAD_CDB_SCSI_CC_DATA_OFFSET_ERROR            = 0x12c,
    FBE_PAYLOAD_CDB_SCSI_CC_SUPER_CAP_FAILURE            = 0x12d,
    FBE_PAYLOAD_CDB_SCSI_CC_DRIVE_TABLE_REBUILD          = 0x12e,

    /* The following code is specific to enclosures. */
    FBE_PAYLOAD_CDB_SCSI_CC_TARGET_OPERATING_COND_CHANGED = 0x200,
    FBE_PAYLOAD_CDB_SCSI_CC_UNSUPPORTED_ENCL_FUNC         = 0x201,
    FBE_PAYLOAD_CDB_SCSI_CC_MODE_PARAM_CHANGED            = 0x202,
}fbe_payload_cdb_scsi_cc_code_t;


typedef fbe_u32_t fbe_payload_cdb_queue_tag_t;

typedef struct fbe_payload_cdb_operation_s{
    fbe_payload_operation_header_t      operation_header; /* Must be first */

    fbe_u8_t                            cdb[FBE_PAYLOAD_CDB_CDB_SIZE];
    fbe_u8_t                            cdb_length;

    /* fbe_block_size_t                 block_size; *//* Block Size in bytes. */
    fbe_payload_cdb_task_attribute_t    payload_cdb_task_attribute; /* IN scsi task attributes ( queueing type) */
    fbe_payload_cdb_priority_t          payload_cdb_priority; /* IN scsi command priority (if supported) */
/*  fbe_payload_cdb_queue_tag_t         payload_cdb_queue_tag;  *//* OUT queue-tag value assigned by port driver */

    fbe_payload_cdb_scsi_status_t       payload_cdb_scsi_status; /* OUT scsi status returned by HBA or target device */ 
    fbe_port_request_status_t           port_request_status; /* OUT SRB status mapped to FBE */
    fbe_port_recovery_status_t          port_recovery_status; /* OUT SRB status mapped to FBE */
    fbe_payload_cdb_flags_t             payload_cdb_flags;  /* IN SRB flags mapped to FBE */
    fbe_u8_t                            sense_info_buffer[FBE_PAYLOAD_CDB_SENSE_INFO_BUFFER_SIZE]; /* OUT scsi sense buffer */
    fbe_time_t                          timeout;
    fbe_payload_sg_descriptor_t         payload_sg_descriptor; /* describes sg list passed in. */
    fbe_u32_t                           transfer_count;
    fbe_u32_t                           transferred_count; /* Actual number of bytes transfered.*/
    /* fbe_u64_t                           time_stamp; */
    fbe_time_t                          service_start_time_us; /* Start time of IO at PDO */
    fbe_time_t                          service_end_time_us; /* End time of IO at PDO*/
}fbe_payload_cdb_operation_t;

/*!**************************************************************
 * @fn fbe_payload_cdb_get_sg_descriptor(
 *          fbe_payload_cdb_operation_t *cdb_operation,
 *          fbe_payload_sg_descriptor_t ** sg_descriptor)
 ****************************************************************
 * @brief Returns the pointer to the sg descriptor of the cdb operation
 * payload.
 *
 * @param cdb_operation - Pointer to the cdb payload struct.
 * @param sg_descriptor - Pointer to the sg descriptor pointer to return.
 *
 * @return Always FBE_STATUS_OK
 * 
 * @see fbe_payload_cdb_operation_t 
 * @see fbe_payload_cdb_operation_t::sg_descriptor
 *
 ****************************************************************/
static __forceinline fbe_status_t
fbe_payload_cdb_get_sg_descriptor(fbe_payload_cdb_operation_t * cdb_operation, fbe_payload_sg_descriptor_t ** sg_descriptor)
{
    * sg_descriptor = &cdb_operation->payload_sg_descriptor;
    return FBE_STATUS_OK;
}

static __forceinline fbe_status_t 
fbe_payload_cdb_set_physical_drive_io_start_time(fbe_payload_cdb_operation_t * cdb_payload, fbe_time_t io_start_time)
{
    cdb_payload->service_start_time_us = io_start_time;
    return FBE_STATUS_OK;
}
static __forceinline fbe_status_t 
fbe_payload_cdb_get_physical_drive_io_start_time(const fbe_payload_cdb_operation_t * cdb_payload, fbe_time_t *io_start_time)
{
    *io_start_time = cdb_payload->service_start_time_us;
    return FBE_STATUS_OK;
}
static __forceinline fbe_status_t 
fbe_payload_cdb_set_physical_drive_io_end_time(fbe_payload_cdb_operation_t * cdb_payload, fbe_time_t io_end_time)
{
    cdb_payload->service_end_time_us = io_end_time;
    return FBE_STATUS_OK;
}
static __forceinline fbe_status_t 
fbe_payload_cdb_get_physical_drive_io_end_time(const fbe_payload_cdb_operation_t * cdb_payload, fbe_time_t *io_end_time)
{
    *io_end_time = cdb_payload->service_end_time_us;
    return FBE_STATUS_OK;
}


fbe_status_t fbe_payload_cdb_set_parameters(fbe_payload_cdb_operation_t * cdb_operation, 
                                             fbe_time_t timeout,
                                             fbe_payload_cdb_task_attribute_t attribute,
                                             fbe_payload_cdb_flags_t flags,
                                             fbe_u32_t cdb_length);

fbe_status_t fbe_payload_cdb_build_inquiry(fbe_payload_cdb_operation_t * cdb_operation, fbe_time_t timeout);
fbe_status_t fbe_payload_cdb_build_vpd_inquiry(fbe_payload_cdb_operation_t * cdb_operation, fbe_time_t timeout, fbe_u8_t page_code);
fbe_status_t fbe_payload_cdb_build_tur(fbe_payload_cdb_operation_t * cdb_operation, fbe_time_t timeout);
fbe_status_t fbe_payload_cdb_build_start_stop_unit(fbe_payload_cdb_operation_t * cdb_operation, fbe_time_t timeout, fbe_bool_t start);
fbe_status_t fbe_payload_cdb_build_mode_sense(fbe_payload_cdb_operation_t * cdb_operation, fbe_time_t timeout, fbe_bool_t get_default);
fbe_status_t fbe_payload_cdb_build_mode_select(fbe_payload_cdb_operation_t * cdb_operation, fbe_time_t timeout, fbe_u32_t data_length);
fbe_status_t fbe_payload_cdb_build_mode_sense_page(fbe_payload_cdb_operation_t * cdb_operation, fbe_time_t timeout, fbe_u8_t page);
fbe_status_t fbe_payload_cdb_build_page_0x19_mode_sense(fbe_payload_cdb_operation_t * cdb_operation, fbe_time_t timeout);
fbe_status_t fbe_payload_cdb_build_mode_sense_10(fbe_payload_cdb_operation_t * cdb_operation, fbe_time_t timeout, 
                                                 fbe_bool_t get_default, fbe_u16_t buffer_size);
fbe_status_t fbe_payload_cdb_build_mode_select_10(fbe_payload_cdb_operation_t * cdb_operation, fbe_time_t timeout, fbe_u32_t data_length);
fbe_status_t fbe_payload_cdb_build_read_capacity_10(fbe_payload_cdb_operation_t * cdb_operation, fbe_time_t timeout);
fbe_status_t fbe_payload_cdb_build_read_capacity_16(fbe_payload_cdb_operation_t * cdb_operation, fbe_time_t timeout);
fbe_status_t fbe_payload_cdb_parse_sense_data(fbe_u8_t* sense_data, fbe_scsi_sense_key_t* sense_key, fbe_scsi_additional_sense_code_t* asc,
                                              fbe_scsi_additional_sense_code_qualifier_t* ascq, fbe_bool_t*  lba_is_valid, fbe_lba_t*  bad_lba);
void         fbe_payload_cdb_parse_sense_data_for_sanitization_percent(const fbe_u8_t* const sense_data, fbe_bool_t *is_sksv_set, fbe_u16_t *percent);

fbe_status_t fbe_payload_cdb_set_request_status(fbe_payload_cdb_operation_t * cdb_operation, fbe_port_request_status_t request_status);
fbe_status_t fbe_payload_cdb_get_request_status(const fbe_payload_cdb_operation_t * cdb_operation, fbe_port_request_status_t * request_status);

fbe_status_t fbe_payload_cdb_set_recovery_status(fbe_payload_cdb_operation_t * cdb_operation, fbe_port_recovery_status_t recovery_status);
fbe_status_t fbe_payload_cdb_get_recovery_status(fbe_payload_cdb_operation_t * cdb_operation, fbe_port_recovery_status_t * recovery_status);

fbe_status_t fbe_payload_cdb_set_scsi_status(fbe_payload_cdb_operation_t * cdb_operation, fbe_payload_cdb_scsi_status_t scsi_status);
fbe_status_t fbe_payload_cdb_get_scsi_status(fbe_payload_cdb_operation_t * cdb_operation, fbe_payload_cdb_scsi_status_t * scsi_status);

fbe_status_t fbe_payload_cdb_operation_get_cdb(fbe_payload_cdb_operation_t * const cdb_operation, fbe_u8_t **cdb);
fbe_status_t fbe_payload_cdb_operation_get_sense_buffer(fbe_payload_cdb_operation_t * const cdb_operation, fbe_u8_t **sense_buffer);

fbe_status_t fbe_payload_cdb_set_transfer_count(fbe_payload_cdb_operation_t * cdb_operation, fbe_u32_t transfer_count);
fbe_status_t fbe_payload_cdb_get_transfer_count(fbe_payload_cdb_operation_t * cdb_operation, fbe_u32_t * transfer_count);

fbe_status_t fbe_payload_cdb_set_transferred_count(fbe_payload_cdb_operation_t * cdb_operation, fbe_u32_t transferred_count);
fbe_status_t fbe_payload_cdb_get_transferred_count(fbe_payload_cdb_operation_t * cdb_operation, fbe_u32_t * transferred_count);

fbe_lba_t           fbe_get_cdb_lba(const fbe_payload_cdb_operation_t * payload_cdb_operation);
fbe_block_count_t   fbe_get_cdb_blocks(const fbe_payload_cdb_operation_t * payload_cdb_operation);

/* Deprecated functions */
fbe_lba_t fbe_get_six_byte_cdb_lba(const fbe_u8_t *const cdb_p);
fbe_block_count_t fbe_get_six_byte_cdb_blocks(const fbe_u8_t *const cdb_p);

fbe_lba_t fbe_get_ten_byte_cdb_lba(const fbe_u8_t *const cdb_p);
fbe_block_count_t fbe_get_ten_byte_cdb_blocks(const fbe_u8_t *const cdb_p);
void fbe_set_ten_byte_cdb_blocks(fbe_u8_t *cdb_p, fbe_block_count_t blk_cnt);
fbe_lba_t fbe_get_sixteen_byte_cdb_lba(const fbe_u8_t *const cdb_p);
fbe_block_count_t fbe_get_sixteen_byte_cdb_blocks(const fbe_u8_t *const cdb_p);
void fbe_set_sixteen_byte_cdb_blocks(fbe_u8_t *cdb_p, fbe_block_count_t blk_cnt);

#endif /* FBE_PAYLOAD_CDB_OPERATION_H */
