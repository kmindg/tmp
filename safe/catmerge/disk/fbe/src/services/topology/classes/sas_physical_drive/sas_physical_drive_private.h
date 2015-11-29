#ifndef SAS_PHYSICAL_DRIVE_PRIVATE_H
#define SAS_PHYSICAL_DRIVE_PRIVATE_H
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_queue.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_perfstats_interface.h"
#include "fbe/fbe_scsi_interface.h"
#include "fbe_ssp_transport.h"
#include "base_physical_drive_private.h"
#include "base_physical_drive_init.h"
#include "swap_exports.h"
#include "sas_physical_drive_config.h"


#define FBE_PDO_DOWNLOAD_POWERCYCLE_DELAY_MS 60000  /* delay powercycle after download completes */
#define FBE_PDO_DOWNLOAD_POWERCYCLE_DURATION 6  /* = 3 secs. In 500ms increments */

#define FBE_SCSI_MAX_MODE_SENSE_10_SIZE 320 /* mode_sense_10 could be as large as 0xFFFF bytes */

/* For Receive Diagnostic .. */
#define RECEIVE_DIAG_PG82_CODE                  0x82
#define RECEIVE_DIAG_PG82_SIZE                  162

/* Solid State Drive are expected to have zero RPM */
#define FBE_EFD_DRIVE_EXPECTED_RPM              0

extern fbe_lifecycle_const_t FBE_LIFECYCLE_CONST_DATA(sas_physical_drive);

typedef struct fbe_sas_physical_drive_price_class_map_data_e {
    const char             *product_id_substring;
    fbe_drive_price_class_t drive_price_class;
} fbe_sas_physical_drive_price_class_map_data_t;

/* Table having the list of valid product ID substrings */
static fbe_sas_physical_drive_price_class_map_data_t
    CSX_MAYBE_UNUSED sas_drive_price_class_map_table[] = {
        {"CLAR", FBE_DRIVE_PRICE_CLASS_B},
        {"NEO", FBE_DRIVE_PRICE_CLASS_J},
        {FBE_DRIVE_INQUIRY_EMC_STRING, FBE_DRIVE_PRICE_CLASS_B},
        {"", FBE_DRIVE_PRICE_CLASS_LAST} /*This should always be LAST entry*/
    };

typedef struct fbe_sas_physical_drive_rla_fixed_map_data_s
{
    fbe_u8_t                 fw_rev[FBE_SCSI_INQUIRY_REVISION_SIZE+1];
}fbe_sas_physical_drive_rla_fixed_map_data_t;

/* Table of Seagate drive fiwmare revisions in which the Read Look Ahead (RLA) problem is fixed.
 * RLAs do not need to be aborted with Test Unit Ready (TUR) commands on Seagate drives with these
 * revisions (or later).
 * NOTE: There is still a special case with Compass bridge code (CSFC) & firmware (CSFE) revisions
 *       that come after CS21, but whose RLAs still need to be aborted with TURs.
 */
static fbe_sas_physical_drive_rla_fixed_map_data_t
    CSX_MAYBE_UNUSED sas_drive_rla_fixed_map_table[] = {
        {"LS19"}, /* Lightning SAS */
        {"GS16"}, /* Megalodon SAS */
        {"FF09"}, /* Firefly SAS */
        {"CS20"}, /* Compass SAS */
        {"ES13"}, /* Eagle SAS */
        {"JS09"}, /* Yellowjacket SAS */
        {"AS0B"}, /* Airwalker SAS */
        {"PS12"}, /* Muskie+ SAS */
        {"YS1C"}, /* Mantaray SAS */
        {"BS1E"}, /* Muskie SAS */
        {"HS0F"}, /* Hurricane SAS */
    };

/* All FBE timeouts in miliseconds */
enum fbe_sas_physical_drive_timeouts_e {
    FBE_SAS_PHYSICAL_DRIVE_INQUIRY_TIMEOUT = 10000, /* 10 sec */
    FBE_SAS_PHYSICAL_DRIVE_READ_CAPACITY_TIMEOUT = 10000, /* 10 sec */
    FBE_SAS_PHYSICAL_DRIVE_START_UNIT_TIMEOUT = 45000, /* 45 sec */
    FBE_SAS_PHYSICAL_DRIVE_WRITE_BUFFER_TIMEOUT = 50000, /* 50 sec */
    FBE_SAS_PHYSICAL_DRIVE_DEFAULT_TIMEOUT = 5000, /* 5 sec */
    FBE_SAS_PHYSICAL_DRIVE_FORMAT_TIMEOUT = 30000, /* 30 sec */
};

enum fbe_sas_physical_drive_constants_e {
    FBE_SAS_PHYSICAL_DRIVE_OUTSTANDING_IO_MAX = 22,    /* Qdepth for all IOs */
    FBE_SAS_PHYSICAL_DRIVE_MAX_FW_DOWNLOAD_RETRIES = 3,
};

/* These are the lifecycle condition ids for a sas physical drive object. */
typedef enum fbe_sas_physical_drive_lifecycle_cond_id_e {
    FBE_SAS_PHYSICAL_DRIVE_LIFECYCLE_COND_PROTOCOL_EDGE_DETACHED = FBE_LIFECYCLE_COND_FIRST_ID(FBE_CLASS_ID_SAS_PHYSICAL_DRIVE),
    FBE_SAS_PHYSICAL_DRIVE_LIFECYCLE_COND_PROTOCOL_EDGE_CLOSED,
    FBE_SAS_PHYSICAL_DRIVE_LIFECYCLE_COND_INQUIRY,
    FBE_SAS_PHYSICAL_DRIVE_LIFECYCLE_COND_VPD_INQUIRY,
    FBE_SAS_PHYSICAL_DRIVE_LIFECYCLE_COND_IDENTITY_INVALID,
    FBE_SAS_PHYSICAL_DRIVE_LIFECYCLE_COND_TEST_UNIT_READY,
    FBE_SAS_PHYSICAL_DRIVE_LIFECYCLE_COND_NOT_SPINNING,
    FBE_SAS_PHYSICAL_DRIVE_LIFECYCLE_COND_GET_SPINUP_CREDIT,
    FBE_SAS_PHYSICAL_DRIVE_LIFECYCLE_COND_RELEASE_SPINUP_CREDIT,
    FBE_SAS_PHYSICAL_DRIVE_LIFECYCLE_COND_RELEASE_SPINUP_CREDIT_TIMER,
    FBE_SAS_PHYSICAL_DRIVE_LIFECYCLE_COND_GET_MODE_PAGES,
    FBE_SAS_PHYSICAL_DRIVE_LIFECYCLE_COND_SET_MODE_PAGES,
    FBE_SAS_PHYSICAL_DRIVE_LIFECYCLE_COND_GET_MODE_PAGE_ATTRIBUTES,
    FBE_SAS_PHYSICAL_DRIVE_LIFECYCLE_COND_READ_CAPACITY,

    FBE_SAS_PHYSICAL_DRIVE_LIFECYCLE_COND_DETACH_SSP_EDGE, /* Preset in destroy rotary */
    FBE_SAS_PHYSICAL_DRIVE_LIFECYCLE_COND_DUMP_DISK_LOG,
    FBE_SAS_PHYSICAL_DRIVE_LIFECYCLE_COND_MONITOR_DISCOVERY_EDGE_IN_FAIL_STATE,
    FBE_SAS_PHYSICAL_DRIVE_LIFECYCLE_COND_CHECK_DRIVE_BUSY,
    FBE_SAS_PHYSICAL_DRIVE_LIFECYCLE_COND_LAST /* must be last */
} fbe_sas_physical_drive_lifecycle_cond_id_t;

struct fbe_sas_physical_drive_s;
typedef struct fbe_sas_physical_drive_info_block_s {
    fbe_u8_t  ms_pages[FBE_SCSI_MAX_MODE_SENSE_10_SIZE];
    fbe_bool_t do_mode_select;
    fbe_u8_t ms_state;
    fbe_u16_t ms_size;
    fbe_u16_t ms_retry_count;
    fbe_u16_t test_unit_ready_retry_count;
    fbe_u8_t  dc_rcv_diag_enabled;

    fbe_u16_t spindown_retry_count;
    /* Read Look Ahead (RLA) abort is required based upon this disk's current firmware revision */
    fbe_bool_t rla_abort_required;
    fbe_u16_t lpq_timer;
    fbe_u16_t hpq_timer;
    fbe_bool_t use_additional_override_tbl;   /* always set before setting GET_MODE_PAGES cond */

    /* NOTE, this is only supported for flash drives.   It should eventually be moved to flash subclass. */
    fbe_physical_drive_sanitize_status_t  sanitize_status; 
    fbe_u16_t                             sanitize_percent;  /* percentage of the sanitize operation - from 0x0000 to 0xffff*/

} fbe_sas_physical_drive_info_block_t;

typedef enum fbe_sas_drive_status_e{
    FBE_SAS_DRIVE_STATUS_OK                               = 0x0000,
    FBE_SAS_DRIVE_STATUS_NOT_SPINNING,
    FBE_SAS_DRIVE_STATUS_DEVICE_NOT_PRESENT,
    FBE_SAS_DRIVE_STATUS_HARD_ERROR,
    FBE_SAS_DRIVE_STATUS_NOT_SUPPORTED_IN_SYSTEM_SLOT,
    FBE_SAS_DRIVE_STATUS_INVALID,
    FBE_SAS_DRIVE_STATUS_NEED_RETRY,
    FBE_SAS_DRIVE_STATUS_NEED_MORE_PROCESSING,
    FBE_SAS_DRIVE_STATUS_SET_PROACTIVE_SPARE,
    FBE_SAS_DRIVE_STATUS_NEED_REMAP,
    FBE_SAS_DRIVE_STATUS_NEED_RESCHEDULE,
    FBE_SAS_DRIVE_STATUS_NEED_RESET,
    FBE_SAS_DRIVE_STATUS_BECOMING_READY,
    FBE_SAS_DRIVE_STATUS_SANITIZE_IN_PROGRESS,
    FBE_SAS_DRIVE_STATUS_SANITIZE_NEED_RESTART,
    
    FBE_SAS_DRIVE_STATUS_LAST
}fbe_sas_drive_status_t;


typedef struct fbe_sas_physical_drive_s{
    fbe_base_physical_drive_t   base_physical_drive;

    /* Indicates that PDO performance statistics are enabled */
    fbe_bool_t  b_perf_stats_enabled;

    /* Stores performance statistics PDO counters manipulated by the SAS physical drive object */
    fbe_performance_statistics_t performance_stats;

    fbe_ssp_edge_t ssp_edge;

    fbe_sas_physical_drive_info_block_t  sas_drive_info;
    
    // virtual functions
    fbe_sas_drive_status_t (*get_vendor_table)(struct fbe_sas_physical_drive_s * sas_physical_drive, fbe_drive_vendor_id_t drive_vendor_id, fbe_vendor_page_t ** table_ptr, fbe_u16_t * num_table_entries); 
    fbe_status_t (*get_port_error)(struct fbe_sas_physical_drive_s * sas_physical_drive, fbe_packet_t * packet, fbe_payload_cdb_operation_t * payload_cdb_operation, fbe_scsi_error_code_t * scsi_error);

    FBE_LIFECYCLE_DEF_INST_DATA;
    FBE_LIFECYCLE_DEF_INST_COND(FBE_LIFECYCLE_COND_MAX(FBE_SAS_PHYSICAL_DRIVE_LIFECYCLE_COND_LAST));

}fbe_sas_physical_drive_t;


/* Max retry counts */
#define FBE_PAYLOAD_CDB_MAX_REMAPS_PER_REQ       8

#define FBE_PAYLOAD_CDB_MAX_REMAPS_PER_LBA       3

#define FBE_PAYLOAD_CDB_MAX_RESERVE_COUNT        10
#define FBE_PAYLOAD_CDB_MAX_REASSIGN_COUNT       5
#define FBE_PAYLOAD_CDB_MAX_RELEASE_COUNT        3

#define FBE_PAYLOAD_CDB_REMAP_DEFECT_LIST_BYTE_SIZE 12
#define FBE_PAYLOAD_CDB_REMAP_DEFECT_LIST_BYTE_SIZE_SHORT 8

/*Max number of Test Unit Ready retry before set drive faulted 
 The worse case of spining up time is 70 seconds for CDES to send NOTIFY primitive,
 plus 60 seconds for actual drive spinnup (per EMC SAS HDD logical spec). Retries are ever 3 seconds
 so retry count is 130/3 = 44
*/ 
#define FBE_MAX_TUR_RETRY_COUNT                  44

/* List of VPD pages of interest. Add more as needed. */
typedef enum fbe_sas_flash_scsi_inquiry_vpd_page_e {
    FBE_SCSI_INQUIRY_VPD_PAGE_00 = 0x00,
    FBE_SCSI_INQUIRY_VPD_PAGE_C0 = 0xC0,
    FBE_SCSI_INQUIRY_VPD_PAGE_D2 = 0xD2,
    FBE_SCSI_INQUIRY_VPD_PAGE_F3 = 0xF3,
    FBE_SCSI_INQUIRY_VPD_PAGE_B2 = 0xB2,
}fbe_sas_flash_scsi_inquiry_vpd_page_t;

enum fbe_sas_flash_scsi_inquiry_vpd_offset_e {
    FBE_SCSI_INQUIRY_VPD_PAGE_CODE_OFFSET = 1, /* Page Code is found at same offset in all VPD pages */
    FBE_SCSI_INQUIRY_VPD_00_MAX_SUPPORTED_PAGES_OFFSET = 3,
    FBE_SCSI_INQUIRY_VPD_00_SUPPORTED_PAGES_START_OFFSET = 4, 
    FBE_SCSI_INQUIRY_VPD_C0_PROD_REV_OFFSET = 6,     /* The Product Revision Major/minor (MMmm)is in VPD 0xC0, bytes 6-9 */
    FBE_SCSI_INQUIRY_VPD_C0_DRIVE_WRITES_PER_DAY_OFFSET = 20, /*The number of drive writes / day is in VPC 0xC0 byte 20  */
    FBE_SCSI_INQUIRY_VPD_D2_BRIDGE_HW_REV_OFFSET =9, /* Bridge H/W revision is at byte 9 in VPD Page 0xD2 */
    FBE_SCSI_INQUIRY_VPD_F3_SUPPORTS_ENHANCED_QUEUING_OFFSET = 7, /* Supports Enhanced Queuing bit is in Byte 7, Bit 3 in VPD Page F3h */
    FBE_SCSI_INQUIRY_VPD_B2_SUPPORTS_UNMAP_OFFSET = 5, /* Supports unmap */
};

/* 10-22-2012: Revert back the changes from update ID: 487001 from upc-performance-cs stream */
#define FBE_SCSI_INQUIRY_VPD_PAGE_F3_ENHANCED_QUEUING_BIT_MASK 0x08

/* Methods */
fbe_status_t fbe_sas_physical_drive_create_object(fbe_packet_t * packet, fbe_object_handle_t * object_handle);
fbe_status_t fbe_sas_physical_drive_destroy_object( fbe_object_handle_t object_handle);
fbe_status_t fbe_sas_physical_drive_control_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet);
fbe_status_t fbe_sas_physical_drive_io_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet);

fbe_status_t fbe_sas_physical_drive_event_entry(fbe_object_handle_t object_handle, fbe_event_type_t event_type, fbe_event_context_t event_context);

fbe_status_t fbe_sas_physical_drive_monitor_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet);

fbe_status_t fbe_sas_physical_drive_monitor_load_verify(void);

fbe_status_t fbe_sas_physical_drive_init(fbe_sas_physical_drive_t * sas_physical_drive);

fbe_sas_drive_status_t fbe_sas_physical_drive_process_mode_sense(fbe_sas_physical_drive_t * sas_physical_drive, 
                                                  fbe_sg_element_t *sg_list);
fbe_sas_drive_status_t fbe_sas_physical_drive_process_mode_sense_10(fbe_sas_physical_drive_t * sas_physical_drive, 
                                                  fbe_sg_element_t *sg_list);
fbe_sas_drive_status_t fbe_sas_physical_drive_process_mode_page_attributes(fbe_sas_physical_drive_t * sas_physical_drive, fbe_sg_element_t *sg_list);
fbe_status_t fbe_sas_physical_drive_get_current_page(fbe_sas_physical_drive_t * sas_physical_drive, fbe_u8_t *page, fbe_u8_t *max_pages);
fbe_status_t fbe_sas_physical_drive_set_dc_rcv_diag_enabled(fbe_sas_physical_drive_t * sas_physical_drive, fbe_u8_t flag);

/* Usurper functions */
fbe_status_t fbe_sas_physical_drive_attach_ssp_edge(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet);
fbe_status_t fbe_sas_physical_drive_detach_edge(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet);
fbe_status_t fbe_sas_physical_drive_open_ssp_edge(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet);
fbe_status_t fbe_sas_physical_drive_receive_diag_page(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet);
fbe_status_t fbe_sas_physical_drive_get_smart_dump(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet);
fbe_status_t fbe_sas_physical_drive_get_disk_error_log(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet);
fbe_status_t fbe_sas_physical_drive_dc_rcv_diag_enabled(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet_p);
fbe_status_t fbe_sas_physical_drive_usurper_health_check_initiate(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet);     
fbe_status_t fbe_sas_physical_drive_health_check_ack(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet);
fbe_status_t fbe_sas_physical_drive_read_long(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet);
fbe_status_t fbe_sas_physical_drive_read_long_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
fbe_status_t fbe_sas_physical_drive_write_long(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet);
fbe_status_t fbe_sas_physical_drive_write_long_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

/* Executer functions */
fbe_status_t fbe_sas_physical_drive_send_inquiry(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet);
fbe_status_t fbe_sas_physical_drive_send_vpd_inquiry(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet);
fbe_status_t fbe_sas_physical_drive_send_qst(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet);
fbe_status_t fbe_sas_physical_drive_send_tur(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet);
fbe_status_t fbe_sas_physical_drive_send_tur_to_abort_rla(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet);
fbe_status_t fbe_sas_physical_drive_start_unit(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet);
fbe_status_t fbe_sas_physical_drive_mode_sense_10(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet);
fbe_status_t fbe_sas_physical_drive_mode_select_10(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet);
fbe_status_t fbe_sas_physical_drive_mode_sense_page(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet);
fbe_status_t fbe_sas_physical_drive_read_capacity_10(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet);
fbe_status_t fbe_sas_physical_drive_read_capacity_16(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet);
fbe_status_t fbe_sas_physical_drive_send_reserve(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet);
fbe_status_t fbe_sas_physical_drive_send_reassign(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet);
fbe_status_t fbe_sas_physical_drive_send_release(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet);
fbe_status_t fbe_sas_physical_drive_reset_device(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet);
fbe_status_t fbe_sas_physical_drive_download_device(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet);
fbe_status_t fbe_sas_physical_drive_set_power_save(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet, 
                                                   fbe_bool_t on, fbe_bool_t passive);
fbe_status_t fbe_sas_physical_drive_spin_down(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet, fbe_bool_t spinup);
fbe_status_t fbe_sas_physical_drive_get_permission(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet, fbe_bool_t get);
fbe_status_t fbe_sas_physical_drive_send_RDR(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet, fbe_u8_t page_code);
fbe_status_t fbe_sas_physical_drive_send_smart_dump(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet);
fbe_status_t sas_physical_drive_process_send_block_cdb(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet);
fbe_status_t sas_physical_drive_process_resend_block_cdb(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet);
fbe_status_t fbe_sas_physical_drive_send_log_sense(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet);

fbe_status_t sas_physical_drive_cdb_convert_to_logical_block_size(fbe_sas_physical_drive_t *const sas_physical_drive_p,
                                                                  const fbe_block_size_t client_block_size,
                                                                  const fbe_block_size_t physical_block_size,
                                                                  fbe_lba_t *const lba_p, 
                                                                  fbe_block_count_t *const blocks_p);
fbe_status_t sas_physical_drive_cdb_build_reserve(fbe_payload_cdb_operation_t * cdb_operation, fbe_time_t timeout);
fbe_status_t sas_physical_drive_cdb_build_reassign(fbe_payload_cdb_operation_t * cdb_operation, fbe_time_t timeout, fbe_bool_t longlba);
fbe_status_t sas_physical_drive_cdb_build_release(fbe_payload_cdb_operation_t * cdb_operation, fbe_time_t timeout);
fbe_status_t sas_physical_drive_cdb_build_download(fbe_payload_cdb_operation_t * cdb_operation, fbe_time_t timeout, fbe_u32_t image_size);
fbe_status_t sas_physical_drive_cdb_build_send_diagnostics(fbe_payload_cdb_operation_t * cdb_operation, fbe_time_t timeout, fbe_u8_t test_bits);
fbe_status_t sas_physical_drive_cdb_build_receive_diag(fbe_payload_cdb_operation_t * cdb_operation, fbe_time_t timeout,
                                                       fbe_u8_t page_code, fbe_u16_t resp_size);
fbe_status_t sas_physical_drive_cdb_build_log_sense(fbe_payload_cdb_operation_t * cdb_operation, fbe_time_t timeout,
                                                       fbe_u8_t page_code, fbe_u16_t resp_size);
fbe_status_t fbe_payload_cdb_build_disk_collect_write(fbe_payload_cdb_operation_t * cdb_operation, fbe_time_t timeout, 
                                                      fbe_block_size_t block_size, fbe_lba_t lba, fbe_block_count_t block_count);
fbe_status_t sas_physical_drive_cdb_build_dump_disk_log(fbe_payload_cdb_operation_t * cdb_operation, fbe_time_t timeout);                       
fbe_status_t sas_physical_drive_cdb_build_smart_dump(fbe_payload_cdb_operation_t * cdb_operation, fbe_time_t timeout, fbe_u32_t size);

fbe_status_t sas_physical_drive_get_scsi_error_code(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet, fbe_payload_cdb_operation_t * payload_cdb_operation,
                                                    fbe_scsi_error_code_t * error, fbe_lba_t * bad_lba, fbe_u32_t * sense_code);
fbe_u32_t    sas_physical_drive_set_control_status(fbe_scsi_error_code_t error, fbe_payload_control_operation_t * payload_control_operation);
fbe_status_t sas_physical_drive_process_scsi_error(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet, fbe_scsi_error_code_t error, const fbe_payload_cdb_operation_t * payload_cdb_operation);
fbe_status_t sas_physical_drive_setup_remap(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet);
fbe_status_t sas_physical_drive_end_remap(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet);
fbe_u32_t    sas_physical_drive_process_sense_buffer(fbe_payload_ex_t *payload_p);
fbe_status_t sas_physical_drive_disk_collect_write(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet);
fbe_status_t sas_physical_drive_setup_dc_remap(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet);
fbe_u32_t    sas_physical_drive_set_dc_control_status(fbe_sas_physical_drive_t * sas_physical_drive, fbe_scsi_error_code_t error, fbe_payload_control_operation_t * payload_control_operation);
fbe_status_t sas_physical_drive_dump_disk_log(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet);
fbe_status_t sas_physical_drive_end_dc_remap(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet);
fbe_status_t fbe_sas_physical_drive_send_read_disk_blocks(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet);
fbe_status_t sas_physical_drive_cdb_build_read_disk_blocks(fbe_payload_cdb_operation_t * cdb_operation, fbe_time_t timeout, fbe_lba_t lba);
fbe_status_t sas_physical_drive_get_port_error(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet, fbe_payload_cdb_operation_t * payload_cdb_operation, fbe_scsi_error_code_t * scsi_error);
fbe_u32_t    sas_physical_drive_set_qst_control_status(fbe_sas_physical_drive_t * sas_physical_drive, fbe_scsi_error_code_t error, fbe_payload_control_operation_t * payload_control_operation);
void         sas_physical_drive_get_default_error_handling(fbe_scsi_error_code_t error, fbe_drive_error_handling_bitmap_t *cmd_status, fbe_physical_drive_death_reason_t *death_reason);
fbe_status_t fbe_sas_physical_drive_format_block_size(fbe_sas_physical_drive_t* sas_physical_drive, fbe_packet_t* packet, fbe_u32_t block_size);

/* Perf Stats functions */
fbe_status_t fbe_sas_physical_drive_perf_stats_update_counters_io_completion(fbe_sas_physical_drive_t * sas_physical_drive_p, fbe_payload_block_operation_t *block_operation_p, fbe_cpu_id_t cpu_id, const fbe_payload_cdb_operation_t * payload_cdb_operation);
void fbe_sas_physical_drive_perf_stats_inc_num_io_in_progress(fbe_sas_physical_drive_t * sas_physical_drive_p, fbe_cpu_id_t cpu_id);
void fbe_sas_physical_drive_perf_stats_dec_num_io_in_progress(fbe_sas_physical_drive_t * sas_physical_drive_p);
fbe_status_t  fbe_sas_physical_drive_send_read_long(fbe_sas_physical_drive_t* sas_physical_drive, fbe_packet_t* packet);
fbe_status_t  fbe_sas_physical_drive_send_read_long_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
fbe_status_t  fbe_sas_physical_drive_send_write_long(fbe_sas_physical_drive_t* sas_physical_drive, fbe_packet_t* packet);
fbe_status_t  fbe_sas_physical_drive_send_write_long_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);


/* Utils functions */
fbe_sas_drive_status_t fbe_sas_physical_drive_set_dev_info(fbe_sas_physical_drive_t * sas_physical_drive, fbe_u8_t *inquiry, 
                                                           fbe_physical_drive_information_t * info_ptr);
fbe_sas_drive_status_t fbe_sas_physical_drive_process_read_capacity_16(fbe_sas_physical_drive_t * sas_physical_drive, fbe_sg_element_t * sg_list);
fbe_sas_drive_status_t fbe_sas_physical_drive_process_read_capacity_10(fbe_sas_physical_drive_t * sas_physical_drive, fbe_sg_element_t * sg_list);



fbe_status_t sas_physical_drive_convert_block_to_cdb_return_error(fbe_sas_physical_drive_t* sas_physical_drive, fbe_payload_ex_t* payload, fbe_payload_cdb_operation_t* cdb_operation,
                                                                  fbe_time_t timeout);

fbe_status_t sas_physical_drive_block_to_cdb(fbe_sas_physical_drive_t* sas_physical_drive, 
											 fbe_payload_ex_t* payload, 
											 fbe_payload_cdb_operation_t* cdb_operation);


fbe_status_t fbe_sas_physical_drive_fill_dc_info(fbe_sas_physical_drive_t * sas_physical_drive, fbe_payload_cdb_operation_t * payload_cdb_operation,
                                                           fbe_u8_t * sense_data, fbe_u8_t * cdb, fbe_bool_t no_bus_device_reset);
fbe_status_t fbe_sas_physical_drive_fill_receive_diag_info(fbe_sas_physical_drive_t * sas_physical_drive, fbe_u8_t * rcv_diag, fbe_u8_t buffer_size);
fbe_sas_drive_status_t fbe_sas_physical_drive_get_vendor_table(fbe_sas_physical_drive_t * sas_physical_drive, fbe_drive_vendor_id_t drive_vendor_id, fbe_vendor_page_t ** table_ptr, fbe_u16_t * num_table_entries);
fbe_bool_t fbe_sas_physical_drive_compare_mode_pages(fbe_u8_t *default_p, fbe_u8_t *saved_p);
fbe_bool_t sas_pdo_is_in_sanitize_state(const fbe_sas_physical_drive_t *sas_physical_drive);
void sas_pdo_process_sanitize_error_for_conditions(fbe_sas_physical_drive_t *sas_physical_drive, fbe_payload_control_operation_t *payload_control_operation, fbe_payload_cdb_operation_t * payload_cdb_operation);
fbe_bool_t fbe_sas_physical_drive_is_enhanced_queuing_supported(fbe_sas_physical_drive_t* sas_physical_drive);
fbe_status_t sas_physical_drive_cdb_build_read_long_10(fbe_payload_cdb_operation_t * cdb_operation, fbe_time_t timeout, fbe_lba_t lba, fbe_u32_t long_block_size);
fbe_status_t sas_physical_drive_cdb_build_read_long_16(fbe_payload_cdb_operation_t * cdb_operation, fbe_time_t timeout, fbe_lba_t lba, fbe_u32_t long_block_size);
fbe_status_t sas_physical_drive_cdb_build_write_long_10(fbe_payload_cdb_operation_t * cdb_operation, fbe_time_t timeout, fbe_lba_t lba, fbe_u32_t long_block_size);
fbe_status_t sas_physical_drive_cdb_build_write_long_16(fbe_payload_cdb_operation_t * cdb_operation, fbe_time_t timeout, fbe_lba_t lba, fbe_u32_t long_block_size);
fbe_status_t sas_physical_drive_cdb_build_sanitize(fbe_payload_cdb_operation_t * cdb_operation, fbe_time_t timeout, fbe_u8_t * sanitize_header, fbe_scsi_sanitize_pattern_t pattern);
fbe_status_t sas_physical_drive_cdb_build_format(fbe_payload_cdb_operation_t * cdb_operation, fbe_time_t timeout, fbe_u8_t * paramter_list, fbe_u32_t paramter_list_size);
fbe_bool_t fbe_sas_physical_drive_is_write_media_error(fbe_payload_cdb_operation_t * cdb_operation, fbe_scsi_error_code_t error);
fbe_status_t fbe_sas_physical_drive_init_enhanced_queuing_timer(fbe_sas_physical_drive_t* sas_physical_drive);
fbe_status_t fbe_sas_physical_drive_set_enhanced_queuing_timer(fbe_sas_physical_drive_t* sas_physical_drive, fbe_u32_t lpq_timer, fbe_u32_t hpq_timer, fbe_bool_t *timer_changed);
fbe_status_t fbe_sas_physical_drive_get_enhanced_queuing_timer(fbe_sas_physical_drive_t* sas_physical_drive, fbe_u16_t *lpq_timer, fbe_u16_t *hpq_timer);
fbe_status_t fbe_sas_physical_drive_get_queuing_info_from_DCS(fbe_sas_physical_drive_t* sas_physical_drive, fbe_u32_t *lpq_timer, fbe_u32_t *hpq_timer);
void fbe_sas_physical_drive_trace_rba(fbe_sas_physical_drive_t * sas_physical_drive, fbe_payload_cdb_operation_t *cdb_operation, fbe_u32_t io_traffic_info);
void fbe_sas_physical_drive_trace_completion(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet, fbe_payload_block_operation_t *block_operation, fbe_payload_cdb_operation_t * payload_cdb_operation, fbe_scsi_error_code_t error);


/* Diag functions */
fbe_status_t fbe_sas_physical_drive_process_receive_diag_page(fbe_sas_physical_drive_t * sas_physical_drive, fbe_packet_t * packet);

/* Block transport entry */
fbe_status_t fbe_sas_physical_drive_block_transport_entry(fbe_transport_entry_context_t context, fbe_packet_t * packet);


#endif /* SAS_PHYSICAL_DRIVE_PRIVATE_H */
