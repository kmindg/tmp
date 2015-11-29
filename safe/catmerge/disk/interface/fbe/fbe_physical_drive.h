#ifndef FBE_PHYSICAL_DRIVE_H
#define FBE_PHYSICAL_DRIVE_H

#include "fbe/fbe_object.h"
#include "fbe/fbe_atomic.h"
#include "fbe/fbe_scsi_interface.h"
#include "fbe/fbe_payload_cdb_operation.h"
#include "fbe/fbe_sg_element.h"
#include "fbe/fbe_payload_block_operation.h"
#include "fbe/fbe_stat.h"
#include "fbe/fbe_types.h"
#include "fbe_imaging_structures.h"
#include "fbe/fbe_sector.h"

typedef enum fbe_drive_type_e {
    FBE_DRIVE_TYPE_INVALID = 0,       /* MBZ */
    FBE_DRIVE_TYPE_FIBRE,             /*Not really sure what this is doing here */
    FBE_DRIVE_TYPE_SAS,               /*For SAS HD drive */
    FBE_DRIVE_TYPE_SATA,              /*For Native SATA drive */
    FBE_DRIVE_TYPE_SAS_FLASH_HE,      /*For SAS EFD (HE) drive */
    FBE_DRIVE_TYPE_SATA_FLASH_HE,     /*For steeplechase SATA EFD drive */     
    FBE_DRIVE_TYPE_SAS_FLASH_ME,      /*For MCL EFD (ME) SSD drives (SAS or SATA paddlecard) */
    FBE_DRIVE_TYPE_SAS_NL,            /*For NL SAS HD drives */
    FBE_DRIVE_TYPE_SATA_PADDLECARD,   /*For steeplechase SATA HDD drive */
    FBE_DRIVE_TYPE_SAS_SED,			/* SED SED HDD drives*/
    FBE_DRIVE_TYPE_SAS_FLASH_LE,      /*For Low Endurance (LE) SSD drives (SAS or SATA paddlecard) */
    FBE_DRIVE_TYPE_SAS_FLASH_RI,      /*For Read Intensive Drive (SAS or SATA paddlecard)*/

    /*if you add a new FLASH drive type, you must update database_get_lun_info 
      where checks the drive type for rotational_rate. */
    FBE_DRIVE_TYPE_LAST
}fbe_drive_type_t;

typedef enum fbe_drive_vendor_id_e{
    FBE_DRIVE_VENDOR_INVALID,
    FBE_DRIVE_VENDOR_SIMULATION,
    FBE_DRIVE_VENDOR_HITACHI,
    FBE_DRIVE_VENDOR_SEAGATE,
    FBE_DRIVE_VENDOR_IBM,
    FBE_DRIVE_VENDOR_FUJITSU,
    FBE_DRIVE_VENDOR_MAXTOR,
    FBE_DRIVE_VENDOR_WESTERN_DIGITAL,
    FBE_DRIVE_VENDOR_STEC,
    FBE_DRIVE_VENDOR_SAMSUNG,
    FBE_DRIVE_VENDOR_SAMSUNG_2,
    FBE_DRIVE_VENDOR_EMULEX,
    FBE_DRIVE_VENDOR_TOSHIBA,
    FBE_DRIVE_VENDOR_GENERIC,
    FBE_DRIVE_VENDOR_ANOBIT,
    FBE_DRIVE_VENDOR_MICRON,
    FBE_DRIVE_VENDOR_SANDISK,
    FBE_DRIVE_VENDOR_ILLEGAL,
    FBE_DRIVE_VENDOR_LAST
}fbe_drive_vendor_id_t;

typedef enum fbe_physical_drive_control_code_e {
        FBE_PHYSICAL_DRIVE_CONTROL_CODE_INVALID = FBE_OBJECT_CONTROL_CODE_INVALID_DEF(FBE_CLASS_ID_BASE_PHYSICAL_DRIVE),
        
        FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_ATTRIBUTES,
        FBE_PHYSICAL_DRIVE_CONTROL_CODE_RESET_DRIVE,
        FBE_PHYSICAL_DRIVE_CONTROL_CODE_MODE_SELECT,
        FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_DIEH_INFO,
        FBE_PHYSICAL_DRIVE_CONTROL_CODE_CLEAR_ERROR_COUNTS,
        FBE_PHYSICAL_DRIVE_CONTROL_CODE_SET_QDEPTH,
        FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_QDEPTH,
        FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_DRIVE_QDEPTH,
        FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_DRIVE_INFORMATION,
        FBE_PHYSICAL_DRIVE_CONTROL_CODE_IS_WR_CACHE_ENABLED,
        FBE_PHYSICAL_DRIVE_CONTROL_CODE_SEND_PASS_THRU_CDB,
        FBE_PHYSICAL_DRIVE_CONTROL_CODE_SEND_PASS_THRU_FIS,
        FBE_PHYSICAL_DRIVE_CONTROL_CODE_FW_DOWNLOAD,
        FBE_PHYSICAL_DRIVE_CONTROL_CODE_FW_DOWNLOAD_START,  /* for non-originator side */
        FBE_PHYSICAL_DRIVE_CONTROL_CODE_FW_DOWNLOAD_STOP,   /* for non-originator side */
        FBE_PHYSICAL_DRIVE_CONTROL_CODE_FW_DOWNLOAD_PERMIT,
        FBE_PHYSICAL_DRIVE_CONTROL_CODE_FW_DOWNLOAD_REJECT,
        FBE_PHYSICAL_DRIVE_CONTROL_CODE_FW_DOWNLOAD_ABORT,
        FBE_PHYSICAL_DRIVE_CONTROL_CODE_SET_DEFAULT_IO_TIMEOUT,
        FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_DEFAULT_IO_TIMEOUT,
        FBE_PHYSICAL_DRIVE_CONTROL_CODE_ENTER_POWER_SAVE,
        FBE_PHYSICAL_DRIVE_CONTROL_CODE_EXIT_POWER_SAVE,
        FBE_PHYSICAL_DRIVE_CONTROL_CODE_STAT_CONFIG_TABLE_CHANGED,
        FBE_PHYSICAL_DRIVE_CONTROL_CODE_ENTER_POWER_SAVE_PASSIVE,
        FBE_PHYSICAL_DRIVE_CONTROL_CODE_POWER_CYCLE,
        FBE_PHYSICAL_DRIVE_CONTROL_CODE_SET_SATA_WRITE_UNCORRECTABLE,
        FBE_PHYSICAL_DRIVE_CONTROL_CODE_RECEIVE_DIAG_PAGE,
        FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_CACHED_DRIVE_INFO,
        FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_LOG_PAGES,
        FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_VPD_PAGES,
        FBE_PHYSICAL_DRIVE_CONTROL_CODE_FAIL_DRIVE,
        FBE_PHYSICAL_DRIVE_CONTROL_CODE_SET_DRIVE_DEATH_REASON,
        FBE_PHYSICAL_DRIVE_CONTROL_CODE_RESET_SLOT,
        FBE_PHYSICAL_DRIVE_CONTROL_ENTER_GLITCH_MODE,
        FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_DISK_ERROR_LOG,
        FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_SMART_DUMP,
        FBE_PHYSICAL_DRIVE_CONTROL_CODE_SET_DC_RCV_DIAG_ENABLED,
        FBE_PHYSICAL_DRIVE_CONTROL_CODE_HEALTH_CHECK_INITIATE,   /* by user */
        FBE_PHYSICAL_DRIVE_CONTROL_CODE_HEALTH_CHECK_ACK,        /* PVD sends when ok to proceed */
        FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_LOCATION,
        FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_DOWNLOAD_INFO,
        FBE_PHYSICAL_DRIVE_CONTROL_CODE_SET_SERVICE_TIME,
        FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_SERVICE_TIME,
        FBE_PHYSICAL_DRIVE_CONTROL_CODE_CLEAR_EOL,
        FBE_PHYSICAL_DRIVE_CONTROL_CODE_SET_CRC_ACTION,
        FBE_PHYSICAL_DRIVE_CONTROL_CODE_ENABLE_DISABLE_PERF_STATS,
        FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_PERF_STATS,
        FBE_PHYSICAL_DRIVE_CONTROL_CODE_UPDATE_DRIVE_FAULT,
        FBE_PHYSICAL_DRIVE_CONTROL_CODE_UPDATE_LINK_FAULT,        
        FBE_PHYSICAL_DRIVE_CONTROL_CODE_HEALTH_CHECK_TEST,
        FBE_PHYSICAL_DRIVE_CONTROL_CODE_CLEAR_END_OF_LIFE,
        FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_POWER_SAVING_STATS,
        FBE_PHYSICAL_DRIVE_CONTROL_CODE_SET_POWER_SAVING_STATS,
        FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_LOGICAL_STATE,
        FBE_PHYSICAL_DRIVE_CONTROL_CODE_FW_DOWNLOAD_TRIAL_RUN,
        FBE_PHYSICAL_DRIVE_CONTROL_CODE_SET_IO_THROTTLE,
        FBE_PHYSICAL_DRIVE_CONTROL_CODE_CLASS_PREFSTATS_SET_ENABLED,
        FBE_PHYSICAL_DRIVE_CONTROL_CODE_CLASS_PREFSTATS_SET_DISABLED,
        FBE_PHYSICAL_DRIVE_CONTROL_CODE_READ_LONG,
        FBE_PHYSICAL_DRIVE_CONTROL_CODE_WRITE_LONG,
        FBE_PHYSICAL_DRIVE_CONTROL_CODE_SET_END_OF_LIFE,
        FBE_PHYSICAL_DRIVE_CONTROL_GENERATE_ICA_STAMP, /* stamp the ICA on the drive*/
        FBE_PHYSICAL_DRIVE_CONTROL_READ_ICA_STAMP, /* read the ICA stamp from the drive*/

        FBE_PHYSICAL_DRIVE_CONTROL_CODE_SET_ENHANCED_QUEUING_TIMER,
        FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_PORT_INFO,
        FBE_PHYSICAL_DRIVE_CONTROL_CODE_SANITIZE_DRIVE_ERASE_ONLY,
        FBE_PHYSICAL_DRIVE_CONTROL_CODE_SANITIZE_DRIVE_OVERWRITE_AND_ERASE,
        FBE_PHYSICAL_DRIVE_CONTROL_CODE_SANITIZE_DRIVE_AFSSI,
        FBE_PHYSICAL_DRIVE_CONTROL_CODE_SANITIZE_DRIVE_NSA,
        FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_SANITIZE_STATUS,
        FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_RLA_ABORT_REQUIRED,
        FBE_PHYSICAL_DRIVE_CONTROL_CODE_FORMAT_BLOCK_SIZE,
        FBE_PHYSICAL_DRIVE_CONTROL_CODE_SET_DIEH_MEDIA_THRESHOLD,
        FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_DIEH_MEDIA_THRESHOLD,
        FBE_PHYSICAL_DRIVE_CONTROL_CODE_REACTIVATE,

        FBE_PHYSICAL_DRIVE_CONTROL_CODE_LAST
}fbe_physical_drive_control_code_t;

#define FBE_DRIVE_MAX_SANITIZATION_PERCENT 0xFFFF

typedef enum fbe_physical_drive_sanitize_status_e {    
    FBE_DRIVE_SANITIZE_STATUS_OK = 0,
    FBE_DRIVE_SANITIZE_STATUS_IN_PROGRESS,
    FBE_DRIVE_SANITIZE_STATUS_RESTART,
} fbe_physical_drive_sanitize_status_t;

typedef struct fbe_physical_drive_sanitize_info_s {
   fbe_physical_drive_sanitize_status_t status;
   fbe_u16_t percent;   /* percent is from 0x0000 to 0xFFFF.  Only valid if Sanitize status is IN_PROGRESS */
} fbe_physical_drive_sanitize_info_t;

typedef enum fbe_drive_price_class_e {
    FBE_DRIVE_PRICE_CLASS_INVALID,
    FBE_DRIVE_PRICE_CLASS_J,
    FBE_DRIVE_PRICE_CLASS_B,
    FBE_DRIVE_PRICE_CLASS_UNKNOWN,
    FBE_DRIVE_PRICE_CLASS_LAST = 0xFF 
}fbe_drive_price_class_t;

/* This structure is directly used by Disk Collect.  Any changes here
   will directly affect the data written to disk and any post processing tools.
   Talk to Disk Collect SME before changing.
*/
typedef struct fbe_physical_drive_error_counts_s{
        /* fbe_atomic_t recoverable_error_count; */
        fbe_u32_t remap_block_count; 
        fbe_u32_t reset_count; 
        fbe_u32_t error_count;
}fbe_physical_drive_error_counts_t;


/* FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_ERROR_COUNTS */
/* This structure is directly used by Disk Collect.  Any changes here
   will directly affect the data written to disk and any post processing tools.
   Talk to Disk Collect SME before changing.
*/
typedef struct fbe_physical_drive_mgmt_get_error_counts_s{
    fbe_physical_drive_error_counts_t physical_drive_error_counts;
    fbe_drive_error_t drive_error;

    fbe_u32_t    io_error_ratio; /* Overall drive error ratio */
    fbe_u32_t    recovered_error_ratio; /* Recovered errors error ratio */
    fbe_u32_t    media_error_ratio; /* Media errors error ratio */
    fbe_u32_t    hardware_error_ratio; /* Hardware errors error ratio */
    fbe_u32_t    healthCheck_error_ratio; /* HealthCheck errors error ratio */
    fbe_u32_t    link_error_ratio; /* Link errors error ratio */
    fbe_u32_t    reset_error_ratio; /* reset error ratio */
    fbe_u32_t    power_cycle_error_ratio; /* power cycle error ratio */
    fbe_u32_t    data_error_ratio; /* error reported by upper level (raid for example) */
}fbe_physical_drive_mgmt_get_error_counts_t;


/* This defines the action bits associated with various logical errors sent to use from RAID */
typedef enum fbe_pdo_logical_error_action_e{
    FBE_PDO_ACTION_KILL_ON_MULTI_BIT_CRC  = 0x1,  /* An unexpected CRC error with multi-bits were received. */
    FBE_PDO_ACTION_KILL_ON_SINGLE_BIT_CRC = 0x2,  /* An unexpected CRC error with a single bit was received. */
    FBE_PDO_ACTION_KILL_ON_OTHER_CRC      = 0x4,  /* A generic unexpected CRC error.  Non multi/non single CRC */
    FBE_PDO_ACTION_LAST                   = 0xFFFFFFFF,  /* marks end of enum */    
}fbe_pdo_logical_error_action_t;

#define FBE_PDO_ACTION_USE_REGISTRY    FBE_PDO_ACTION_LAST


typedef enum fbe_pdo_drive_parameter_flags_e{
    FBE_PDO_FLAG_FLASH_DRIVE_WRITES_PER_DAY = 0x00FF,      /* Indicates the number of drive writes / day to guarentee warentee */
    FBE_PDO_FLAG_USES_DESCRIPTOR_FORMAT     = 0x0100,      /* This drive uses descriptor bormat */
    FBE_PDO_FLAG_FLASH_DRIVE                = 0x0200,      /* This drive is a Flash Drive */
    FBE_PDO_FLAG_MLC_FLASH_DRIVE            = 0x0400,      /* This drive is an MLC Flash Drive */
    FBE_PDO_FLAG_SUPPORTS_UNMAP             = 0x0800,      /* This drive supports unmap */
    FBE_PDO_FLAG_SUPPORTS_WS_UNMAP          = 0x1000,      /* This drive supports write same with unmap */
    FBE_PDO_FLAG_SED_TLA_DETECTED        	= 0x2000,	   /* SED Drive - drive TLA has SED TLA encoding */
    FBE_PDO_FLAG_DRIVE_TYPE_UNKNOWN         = 0x4000,      /* For drives with no TLA suffix, this is set so we can discover the drive type.
                                                               This type is only to be used for the new EMC product ID string */
    FBE_PDO_FLAG_LAST                       = 0xFFFFFFFF,  /* marks end of enum */    
}fbe_pdo_drive_parameter_flags_t;


/*!****************************************************************************
 *    
 * @struct fbe_pdo_maintenance_mode_flags_t
 *  
 * @brief 
 *   This structure contains flags which tell clients why PDO should be put 
 *   into Maintenance Mode.  The drive will then be put into a Logically Offline 
 *   state until the correct maintenance is performed.
 ******************************************************************************/
enum fbe_pdo_maintenance_mode_flags_e{
    FBE_PDO_MAINTENANCE_FLAG_NONE             = 0,            /*!< No flags are set. */
    FBE_PDO_MAINTENANCE_FLAG_EQ_NOT_SUPPORTED = 0x00000001,   /*!< requires fw upgrade to fix */
    FBE_PDO_MAINTENANCE_FLAG_SANITIZE_STATE   = 0x00000002,   /*!< sanitize is currently inprogress or was interrupted. */
    FBE_PDO_MAINTENANCE_FLAG_INVALID_IDENTITY = 0x00000004,   /*!< drive identity is invalid and the pdo is in maintenance mode. */
};
typedef fbe_u32_t fbe_pdo_maintenance_mode_flags_t;

typedef enum fbe_pdo_dieh_media_threshold_mode_e{
    FBE_DRIVE_MEDIA_MODE_UNKNOWN = 0,
    FBE_DRIVE_MEDIA_MODE_THRESHOLDS_DEFAULT,
    FBE_DRIVE_MEDIA_MODE_THRESHOLDS_DISABLED,
    FBE_DRIVE_MEDIA_MODE_THRESHOLDS_INCREASED,
    FBE_DRIVE_MEDIA_MODE_THRESHOLDS_DECREASED,
}fbe_pdo_dieh_media_threshold_mode_t;

/*!****************************************************************************
 *    
 * @struct fbe_physical_drive_dieh_stats_s
 *  
 * @brief 
 *   This structure contains DIEH config and state info
 ******************************************************************************/
typedef struct fbe_physical_drive_dieh_stats_s{
    fbe_drive_configuration_handle_t            drive_configuration_handle;
    fbe_physical_drive_mgmt_get_error_counts_t  error_counts;
    fbe_drive_stat_t                            drive_stat;   
    fbe_drive_dieh_state_t                      dieh_state;
    fbe_pdo_dieh_media_threshold_mode_t         dieh_mode;  
    fbe_pdo_logical_error_action_t              crc_action;
}fbe_physical_drive_dieh_stats_t;

/* FBE_PHYSICAL_DRIVE_CONTROL_CODE_SET_QDEPTH 
   FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_QDEPTH
   FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_DRIVE_QDEPTH 
   FBE_PHYSICAL_DRIVE_CONTROL_CODE_SET_IO_THROTTLE
*/
typedef struct fbe_physical_drive_mgmt_qdepth_s{
    fbe_u32_t			qdepth; 
    fbe_u32_t			outstanding_io_count;
    fbe_block_count_t	io_throttle_count;
    fbe_block_count_t	io_throttle_max;
}fbe_physical_drive_mgmt_qdepth_t;

/* FBE_PHYSICAL_DRIVE_CONTROL_CODE_IS_WR_CACHE_ENABLED */
typedef struct fbe_physical_drive_mgmt_rd_wr_cache_value_s{
        fbe_bool_t fbe_physical_drive_mgmt_rd_wr_cache_value; 
}fbe_physical_drive_mgmt_rd_wr_cache_value_t;

typedef struct fbe_physical_drive_information_s{
    fbe_u8_t                    vendor_id[FBE_SCSI_INQUIRY_VENDOR_ID_SIZE+1];
    fbe_u8_t                    product_id[FBE_SCSI_INQUIRY_PRODUCT_ID_SIZE+1];
    fbe_u8_t                    product_rev[FBE_SCSI_INQUIRY_VPD_PROD_REV_SIZE+1];
    fbe_u32_t                   product_rev_len;
    fbe_u8_t                    product_serial_num[FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE+1];
    fbe_lba_t                   gross_capacity;
    fbe_u8_t                    tla_part_number[FBE_SCSI_INQUIRY_TLA_SIZE+1];
    fbe_u8_t                    dg_part_number_ascii[FBE_SCSI_INQUIRY_PART_NUMBER_SIZE+1];
    fbe_u8_t                    drive_description_buff[FBE_SCSI_DESCRIPTION_BUFF_SIZE+1];
    fbe_u8_t                    bridge_hw_rev[FBE_SCSI_INQUIRY_VPD_BRIDGE_HW_REV_SIZE + 1];
    fbe_u32_t                   speed_capability;
    fbe_drive_vendor_id_t       drive_vendor_id;
    fbe_drive_type_t            drive_type;
    fbe_lba_t                   block_count; /* must be 64 bits */
    fbe_block_size_t            block_size;
    fbe_u32_t                   drive_qdepth;
    fbe_u32_t                   drive_RPM;    
    fbe_bool_t                  spin_down_qualified;
    fbe_drive_price_class_t     drive_price_class; /*Added to support non clariion drives*/
    fbe_bool_t                  paddlecard; 
    fbe_pdo_drive_parameter_flags_t drive_parameter_flags;
    fbe_u32_t                   spin_up_time_in_minutes;/*how long the drive was spinning*/
    fbe_u32_t                   stand_by_time_in_minutes;/*how long the drive was not spinning*/
    fbe_u32_t                   spin_up_count;/*how many times the drive spun up*/
    fbe_port_number_t           port_number; 
    fbe_enclosure_number_t      enclosure_number; 
    fbe_slot_number_t           slot_number;
    fbe_bool_t                  enhanced_queuing_supported;
}fbe_physical_drive_information_t;

/* FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_DRIVE_INFORMATION */
typedef struct fbe_physical_drive_information_asynch_s{
        fbe_physical_drive_information_t  get_drive_info;
        void (* completion_function)(void *context);
}fbe_physical_drive_information_asynch_t;


/* FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_DRIVE_INFORMATION */
typedef struct fbe_physical_drive_mgmt_get_drive_information_s{
        fbe_physical_drive_information_t  get_drive_info; 
}fbe_physical_drive_mgmt_get_drive_information_t;


/* FBE_PHYSICAL_DRIVE_CONTROL_CODE_FW_DOWNLOAD_START */
typedef struct fbe_physical_drive_fwdl_start_request_s{
    fbe_bool_t force_download;
}fbe_physical_drive_fwdl_start_request_t;

typedef struct fbe_physical_drive_fwdl_start_response_s{
        fbe_bool_t          proceed;        // out data
        //fbe_fail_reason_t fail_reason;    // out data
}fbe_physical_drive_fwdl_start_response_t;

typedef struct fbe_physical_drive_fwdl_start_asynch_s{
        void                *caller_instance;    //TODO: Consider using object_id. Investigate what SEP does 
        fbe_object_id_t     drive_object_id;
        fbe_status_t        completion_status;  // indicates if call completed without errors
        fbe_physical_drive_fwdl_start_request_t  request;
        fbe_physical_drive_fwdl_start_response_t response;
        void (* completion_function)(void *context);
}fbe_physical_drive_fwdl_start_asynch_t;

/* FBE_PHYSICAL_DRIVE_CONTROL_CODE_FW_DOWNLOAD */
typedef struct fbe_physical_drive_fw_info_asynch_s{
        void                *caller_instance;     
        fbe_object_id_t     drive_object_id;
        fbe_status_t        completion_status;  // indicates if call completed without errors
        void (* completion_function)(void *context);
}fbe_physical_drive_fw_info_asynch_t; 

/* FBE_PHYSICAL_DRIVE_CONTROL_CODE_FW_DOWNLOAD_STOP */
typedef struct fbe_physical_drive_fwdl_stop_asynch_s{
        void                *caller_instance;    
        fbe_object_id_t     drive_object_id;
        fbe_status_t        completion_status;  // indicates if call completed without errors
        void (* completion_function)(void *context);
}fbe_physical_drive_fwdl_stop_asynch_t; 


/*FBE_PHYSICAL_DRIVE_CONTROL_CODE_SEND_PASS_THRU_CDB*/
typedef struct fbe_physical_drive_mgmt_send_pass_thru_cdb_s{
    fbe_u8_t                            cdb[FBE_PAYLOAD_CDB_CDB_SIZE];
    fbe_u8_t                            cdb_length;
    fbe_payload_cdb_task_attribute_t    payload_cdb_task_attribute; /* IN scsi task attributes ( queueing type) */
    fbe_payload_cdb_scsi_status_t       payload_cdb_scsi_status; /* OUT scsi status returned by HBA or target device */ 
    fbe_port_request_status_t           port_request_status; /* OUT SRB status mapped to FBE */
    fbe_payload_cdb_flags_t             payload_cdb_flags;  /* IN SRB flags mapped to FBE */
    fbe_u8_t                            sense_info_buffer[FBE_PAYLOAD_CDB_SENSE_INFO_BUFFER_SIZE]; /*OUT scsi sense buffer */
    fbe_u8_t                            sense_buffer_lengh;
    fbe_u32_t                           transfer_count;
}fbe_physical_drive_mgmt_send_pass_thru_cdb_t;

typedef struct fbe_physical_drive_send_pass_thru_s{
    fbe_physical_drive_mgmt_send_pass_thru_cdb_t cmd;
    fbe_u8_t * response_buf;
    fbe_status_t status;    
}fbe_physical_drive_send_pass_thru_t;

typedef struct fbe_physical_drive_send_pass_thru_asynch_s{   
    fbe_physical_drive_send_pass_thru_t pass_thru; 
    void *context;
    void (* completion_function)(void *context);
}fbe_physical_drive_send_pass_thru_asynch_t;


/*FBE_PHYSICAL_DRIVE_CONTROL_CODE_DOWNLOAD_DEVICE*/
typedef struct fbe_physical_drive_mgmt_fw_download_s{
    fbe_object_id_t                             object_id;
    fbe_bool_t                                  trial_run;
    fbe_bool_t                                  fast_download;
    fbe_bool_t                                  force_download; /* force download for cases where PVD or RAID will reject */
    fbe_scsi_error_code_t                       download_error_code;
    fbe_payload_block_operation_status_t        status;
    fbe_payload_block_operation_qualifier_t     qualifier;
    fbe_u32_t                                   transfer_count;/*Image size in bytes*/
    /* these fields used internally by download state machine */
    fbe_u8_t                                    retry_count;
    fbe_time_t                                  powercycle_expiration_time;
}fbe_physical_drive_mgmt_fw_download_t;

typedef struct fbe_physical_drive_fw_download_asynch_s{
    fbe_physical_drive_mgmt_fw_download_t       download_info;
    fbe_sg_element_t *                          download_sg_list;
    void *                                      caller_instance;     
    fbe_u8_t                                    bus, encl, slot;
    fbe_status_t                                completion_status;  // indicates if call completed without errors
    void                                        (* completion_function)(void *context);
}fbe_physical_drive_fw_download_asynch_t;

/*FBE_PHYSICAL_DRIVE_CONTROL_CODE_SET_DEFAULT_IO_TIMEOUT*/
/*FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_DEFAULT_IO_TIMEOUT*/
typedef struct fbe_physical_drive_mgmt_default_timeout_s{
        fbe_time_t timeout; 
}fbe_physical_drive_mgmt_default_timeout_t;

/*FBE_PHYSICAL_DRIVE_CONTROL_CODE_RECEIVE_DIAG_PAGE*/
typedef struct fbe_physical_drive_receive_diag_pg82_s{
    fbe_u32_t initiate_port;
    struct port_info
    {
        fbe_u32_t invalid_dword_count;
        fbe_u32_t loss_sync_count;
        fbe_u32_t phy_reset_count;
        fbe_u32_t invalid_crc_count;
        fbe_u32_t disparity_error_count;
    } port[2];
}fbe_physical_drive_receive_diag_pg82_t;

typedef struct fbe_physical_drive_mgmt_receive_diag_page_s{
    fbe_u8_t page_code;
    union 
    {
        fbe_physical_drive_receive_diag_pg82_t pg82;
    } page_info;
}fbe_physical_drive_mgmt_receive_diag_page_t;

typedef struct fbe_physical_drive_mgmt_get_log_page_s{
    fbe_u8_t page_code;
    fbe_u16_t transfer_count;
}fbe_physical_drive_mgmt_get_log_page_t;

typedef struct fbe_physical_drive_fuel_gauge_asynch_s{
    fbe_physical_drive_mgmt_get_log_page_t get_log_page;
    CSX_ALIGN_N(8) fbe_u8_t * response_buf;
    fbe_sg_element_t sg_element[2];
    fbe_status_t status;
    void (* completion_function)(void *context);
    void *drive_mgmt;
}fbe_physical_drive_fuel_gauge_asynch_t;

typedef struct fbe_physical_drive_mgmt_get_smart_dump_s{
    fbe_u32_t transfer_count;
}fbe_physical_drive_mgmt_get_smart_dump_t;

typedef struct fbe_physical_drive_get_smart_dump_asynch_s{
    fbe_physical_drive_mgmt_get_smart_dump_t get_smart_dump;
    CSX_ALIGN_N(8) fbe_u8_t * response_buf;
    fbe_sg_element_t sg_element[2];
    fbe_status_t status;
    void (* completion_function)(void *context);
    void *drive_mgmt;
}fbe_physical_drive_get_smart_dump_asynch_t;

typedef struct fbe_physical_drive_mgmt_set_enhanced_queuing_timer_s{
    fbe_u32_t lpq_timer_ms;
    fbe_u32_t hpq_timer_ms;
}fbe_physical_drive_mgmt_set_enhanced_queuing_timer_t;

typedef struct fbe_physical_drive_get_download_info_s{
    fbe_u8_t                    vendor_id[FBE_SCSI_INQUIRY_VENDOR_ID_SIZE+1];
    fbe_u8_t                    product_id[FBE_SCSI_INQUIRY_PRODUCT_ID_SIZE+1];
    fbe_u8_t                    product_rev[FBE_SCSI_INQUIRY_VPD_PROD_REV_SIZE+1];
    fbe_u8_t                    tla_part_number[FBE_SCSI_INQUIRY_TLA_SIZE+1];
    fbe_port_number_t           port;
    fbe_u32_t                   enclosure;
    fbe_u16_t                   slot;
    fbe_lifecycle_state_t       state;
}fbe_physical_drive_get_download_info_t;

typedef struct fbe_physical_drive_get_port_info_s{
    fbe_object_id_t             port_object_id;
}fbe_physical_drive_get_port_info_t;



typedef enum fbe_pdo_dieh_media_threshold_cmd_e{
    FBE_DRIVE_THRESHOLD_CMD_INVALID = 0,
    FBE_DRIVE_THRESHOLD_CMD_RESTORE_DEFAULTS,
    FBE_DRIVE_THRESHOLD_CMD_INCREASE,
    FBE_DRIVE_THRESHOLD_CMD_DISABLE,
}fbe_pdo_dieh_media_threshold_cmd_t;

/*!****************************************************************************
 *    
 * @struct fbe_physical_drive_set_dieh_media_threshold_t
 *  
 * @brief 
 *   This structure is sent to a PDO's usurper to changed the DIEH threshold for the
 *   media error categories. It contains a DIEH media threshold cmd with an option 
 *   parameter.  When the cmd is INCREASE the option parameter represents a 
 *   percentage multiplier. Predefined increase values are listed above.
 *
 ******************************************************************************/
typedef struct fbe_physical_drive_set_dieh_media_threshold_s{
    fbe_pdo_dieh_media_threshold_cmd_t cmd;
    fbe_u32_t option;
}fbe_physical_drive_set_dieh_media_threshold_t;


typedef enum fbe_pdo_dieh_media_reliability_e{
    FBE_DRIVE_MEDIA_RELIABILITY_UNKNOWN     = 0,
    FBE_DRIVE_MEDIA_RELIABILITY_VERY_HIGH   = 1,
    FBE_DRIVE_MEDIA_RELIABILITY_HIGH        = 2,
    FBE_DRIVE_MEDIA_RELIABILITY_NORMAL      = 3,
    FBE_DRIVE_MEDIA_RELIABILITY_LOW         = 4,
    FBE_DRIVE_MEDIA_RELIABILITY_VERY_LOW    = 5,
}fbe_pdo_dieh_media_reliability_t;

/*!****************************************************************************
 *    
 * @struct fbe_physical_drive_get_dieh_media_threshold_t
 *  
 * @brief 
 *   This structure is sent to a PDO's usurper to get the DIEH threshold for the
 *   media error categories.
 *
 ******************************************************************************/
typedef struct fbe_physical_drive_get_dieh_media_threshold_s {
    fbe_pdo_dieh_media_threshold_mode_t dieh_mode;           /* Determines if thresolds have been changed or not*/
    fbe_pdo_dieh_media_reliability_t    dieh_reliability;    /* The reliability of the media for this particular drive */ 
    fbe_u32_t                           media_weight_adjust; /* adjust percentage multiplier.  100 means stay the same. */
    fbe_u32_t                           vd_paco_position;      /*!< IF there is a proactive copy in progress this is the position in the raid group */
}fbe_physical_drive_get_dieh_media_threshold_t;

typedef enum fbe_base_physical_drive_death_reason_e{
    FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_INVALID = FBE_OBJECT_DEATH_CODE_INVALID_DEF(FBE_CLASS_ID_BASE_PHYSICAL_DRIVE),
    FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_SPECIALIZED_TIMER_EXPIRED,
    FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_LINK_THRESHOLD_EXCEEDED,
    FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_RECOVERED_THRESHOLD_EXCEEDED,
    FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_MEDIA_THRESHOLD_EXCEEDED,
    FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_HARDWARE_THRESHOLD_EXCEEDED,
    FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_HEALTHCHECK_THRESHOLD_EXCEEDED,
    FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_DATA_THRESHOLD_EXCEEDED,
    FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_FATAL_ERROR,
    FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_ACTIVATE_TIMER_EXPIRED,
    FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_ENCLOSURE_OBJECT_FAILED,
    FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_INVALID_CAPACITY,
    FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_FAILED_TO_SPIN_DOWN,
    FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_CRC_MULTI_BIT,
    FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_CRC_SINGLE_BIT,
    FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_CRC_OTHER,
    FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_SIMULATED,   /*simulation only*/
    FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_PRICE_CLASS_MISMATCH,
    FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_ENHANCED_QUEUING_NOT_SUPPORTED,
    FBE_BASE_PYHSICAL_DRIVE_DEATH_REASON_EXCEEDS_PLATFORM_LIMITS,
    FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_INVALID_VAULT_CONFIGURATION,
    FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_INVALID_ID,
	FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_SED_DRIVE_NOT_SUPPORTED,
    FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_LAST
}fbe_base_physical_drive_death_reason_t;

typedef enum fbe_physical_drive_death_reason_e{
    FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_INVALID = FBE_OBJECT_DEATH_CODE_INVALID_DEF(FBE_CLASS_ID_SAS_PHYSICAL_DRIVE),
    FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_DEFECT_LIST_ERRORS,
    FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_MAX_REMAPS_EXCEEDED,
    FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_FAILED_TO_SET_DEV_INFO,
    FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_FAILED_TO_PROCESS_MODE_SENSE,
    FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_FAILED_TO_PROCESS_READ_CAPACITY,
    FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_FAILED_TO_REASSIGN,
    FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_DRIVE_NOT_SPINNING,
    FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_FAILED_TO_SPIN_DOWN,
    FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_WRITE_PROTECT,
    FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_FW_UPGRADE_REQUIRED,
    FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_UNEXPECTED_FAILURE,
    FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_DC_FLUSH_FAILED,
    FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_HEALTHCHECK_FAILED,
    FBE_SAS_PHYSICAL_DRIVE_DEATH_REASON_FW_UPGRADE_FAILED,
    FBE_SATA_PHYSICAL_DRIVE_DEATH_REASON_INVALID_INSCRIPTION,
    FBE_SATA_PHYSICAL_DRIVE_DEATH_REASON_PIO_MODE_UNSUPPORTED,
    FBE_SATA_PHYSICAL_DRIVE_DEATH_REASON_UDMA_MODE_UNSUPPORTED,
    FBE_SATA_PHYSICAL_DRIVE_DEATH_REASON_FAILED_TO_SET_DEV_INFO,
    FBE_PHYSICAL_DRIVE_DEATH_REASON_LAST
}fbe_physical_drive_death_reason_t;

typedef enum fbe_physical_drive_speed_e{
    FBE_DRIVE_SPEED_UNKNOWN,
    FBE_DRIVE_SPEED_1_5_GB, 
    FBE_DRIVE_SPEED_2GB,
    FBE_DRIVE_SPEED_3GB,
    FBE_DRIVE_SPEED_4GB,
    FBE_DRIVE_SPEED_6GB,
    FBE_DRIVE_SPEED_8GB,
    FBE_DRIVE_SPEED_10GB,
    FBE_DRIVE_SPEED_12GB,
    FBE_DRIVE_SPEED_LAST
}fbe_physical_drive_speed_t;

typedef struct fbe_physical_drive_mgmt_set_write_uncorrectable_s{
    fbe_lba_t lba; 
}fbe_physical_drive_mgmt_set_write_uncorrectable_t;

typedef struct fbe_physical_drive_mgmt_create_read_uncorrectable_s{
    fbe_lba_t lba; 
    /* This is signed since it can be negative */
    fbe_s32_t long_block_size;
}fbe_physical_drive_mgmt_create_read_uncorrectable_t;


/***************************************************************************
 *
 * DESCRIPTION
 *    FBE Drive death reason is set by CM. (NOTE: Enum value should not be more than 0xFFFF)
 *
 ***************************************************************************/
typedef enum _FBE_DRIVE_DEAD_REASON
{
    FBE_DRIVE_DEATH_UNKNOWN = -1,               /* Drive death reason unknown */
    FBE_DRIVE_ALIVE,                            /* Drive is alive */
    FBE_DRIVE_PHYSICALLY_REMOVED,               /* Drive physically removed */
    FBE_DRIVE_PEER_REQUESTED,                   /* Peer requested to power down */
    FBE_DRIVE_ASSERTED_PBC,                     /* Drive PBC asserted */
    FBE_DRIVE_USER_INITIATED_CMD,               /* User initiated command (simulate) for drive removal */
    FBE_DRIVE_ENCL_MISSING,                     /* Enclosure missing */
    FBE_DRIVE_ENCL_FAILED,                      /* Enclosure failed */
    FBE_DRIVE_PACO_COMPLETED,                   /* Proactive copy completed */
    FBE_DRIVE_UNSUPPORTED,                      /* Unsupported drive */
    FBE_DRIVE_UNSUPPORTED_PARTITION,            /* Drive unsupported partition */
    FBE_DRIVE_FRU_ID_VERIFY_FAILED,             /* FRU ID verify failed */
    FBE_DRIVE_FRU_SIG_CACHE_DIRTY_FLG_INVALID,  /* CACHE dirty flag is invalid */
    FBE_DRIVE_ODBS_REQUESTED,                   /* ODBS has requested for drive removal */
    FBE_DRIVE_DH_REQUESTED,                     /* DH has requested for drive removal */
    FBE_DRIVE_LOGIN_FAILED,                     /* Drive failed to login */
    FBE_DRIVE_BE_FLT_REC_REQUESTED,             /* BE fault recovery has requested for drive removal */
    FBE_DRIVE_ENCL_TYPE_MISMATCH,               /* Drive mismatch detected */
    FBE_DRIVE_FRU_SIG_FAILURE,                  /* FRU signature was incorrect - in some instances, we'll
                                             * fault the drive - ie) proactive copy in progress */
    FBE_DRIVE_USER_INITIATED_POWER_CYCLE_CMD,   /* User initiated drive power cycle command */
    FBE_DRIVE_USER_INITIATED_POWER_OFF_CMD,     /* Uset initiated drive power off command */
    FBE_DRIVE_USER_INITIATED_RESET_CMD,         /* Uset initiated drive reset off command */
    FBE_DRIVE_ENCL_COOLING_FAULT,               /* When there is a multi-blower fault, power supply physically removed
                                             * or ambient overtemp */
    FBE_DRIVE_TYPE_RG_SAS_IN_SATA2,             /* A SAS drive is inserted into a SATA2 raid group */
    FBE_DRIVE_TYPE_RG_SATA2_IN_SAS,             /* A SATA2 drive is inserted into a SAS raid group */
    FBE_DRIVE_TYPE_SYS_SAS_IN_SATA2,            /* A SAS drive is inserted into a SATA2 system raid group */
    FBE_DRIVE_TYPE_SYS_SATA2_IN_SAS,            /* A SATA2 drive is inserted into a SAS system raid group */

    FBE_DRIVE_TYPE_SYS_MISMATCH ,
    FBE_DRIVE_TOO_SMALL,                        /* Drive is too small for the raid group */
    FBE_DRIVE_TYPE_UNSUPPORTED,                 /* The drive is of the wrong technology (e.g. SATA1 in a SAS/SATA2 systems) */
    FBE_DRIVE_TYPE_RG_MISMATCH,                 /* Generic mismatch for catch-all */
    FBE_DRIVE_FW_DOWNLOAD,                      /* Drive is dead to download firmware */
    FBE_DRIVE_SERIAL_NUM_INVALID                /* The drive had an invalid serial number */

} FBE_DRIVE_DEAD_REASON;

typedef struct fbe_physical_drive_mgmt_drive_death_reason_s{
        FBE_DRIVE_DEAD_REASON death_reason; 
}fbe_physical_drive_mgmt_drive_death_reason_t;

typedef struct fbe_physical_drive_mgmt_get_disk_error_log_s{
    fbe_lba_t lba;
}fbe_physical_drive_mgmt_get_disk_error_log_t;

/* FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_LOCATION*/
typedef struct fbe_physical_drive_get_location_s{
    fbe_port_number_t port_number;
    fbe_enclosure_number_t enclosure_number;
    fbe_slot_number_t      slot_number;
}fbe_physical_drive_get_location_t;

typedef struct fbe_physical_drive_control_set_service_time_s{
    fbe_time_t  service_time_ms; 
}fbe_physical_drive_control_set_service_time_t;

typedef struct fbe_physical_drive_control_get_service_time_s{
    fbe_time_t current_service_time_ms; 
    fbe_time_t default_service_time_ms; 
    fbe_time_t emeh_service_time_ms; 
    fbe_time_t remap_service_time_ms; 
}fbe_physical_drive_control_get_service_time_t;

typedef struct fbe_physical_drive_control_crc_action_s{
    fbe_pdo_logical_error_action_t action;
}fbe_physical_drive_control_crc_action_t;

/* FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_POWER_SAVING_STATS */
typedef struct fbe_physical_drive_power_saving_stats_s{
    fbe_u32_t                   spin_up_time_in_minutes;/*how long the drive was spinning*/
    fbe_u32_t                   stand_by_time_in_minutes;/*how long the drive was not spinning*/
    fbe_u32_t                   spin_up_count;/*how many times the drive spun up*/
}fbe_physical_drive_power_saving_stats_t;

/*!****************************************************************************
 *    
 * @enum fbe_physical_drive_health_check_req_status_e
 *  
 * @brief 
 *   Contains the status for the Health Check Request sent to PVD.
 ******************************************************************************/
typedef enum fbe_physical_drive_health_check_req_status_e {
    FBE_DRIVE_HC_STATUS_PROCEED, /* informs PDO to proceed with HC */
    FBE_DRIVE_HC_STATUS_ABORT,   /* aborts a HC request */
}fbe_physical_drive_health_check_req_status_t;

/*!********************************************************************* 
 * @struct fbe_physical_drive_attributes_t
 *  
 * @brief 
 *  These are logical drive attributes we can get/set with control code of
 *  @ref FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_ATTRIBUTES .
 **********************************************************************/
typedef struct
{
    /*! This is the physical block size of the drive in bytes.
     */
    fbe_block_size_t    physical_block_size;

    /*! This is the number of blocks exported to the client.
     *  The blocks size of the imported blocks is the client block size.
     */
    fbe_lba_t           imported_capacity;

    /*! This is the identify information from the logical drive. 
     * This gets fetched at init time. 
     */
    fbe_block_transport_identify_t initial_identify;

    /*! This is the last copy of the identify information that was received. 
     */
    fbe_block_transport_identify_t last_identify;
 
    /*! This indicates if PDO is in maitenance mode, which would require putting 
     *  it into a Logically Offline state until issue is resolved.
     */
    fbe_pdo_maintenance_mode_flags_t    maintenance_mode_bitmap;

    struct  
    {
        fbe_bool_t      b_optional_queued;           /*!< True if objects queued to optional queue. */
        fbe_bool_t      b_low_queued;                /*!< True if objects queued to low queue. */
        fbe_bool_t      b_normal_queued;             /*!< True if objects queued to normal queue. */
        fbe_bool_t      b_urgent_queued;             /*!< True if objects queued to urgent queue. */
        fbe_u32_t       number_of_clients;           /*!< Number of client edges attached.        */
        fbe_object_id_t server_object_id;            /*!< Object id of the server object. */
        fbe_u32_t		outstanding_io_count;        /*!< Number of I/Os currently in flight. */
        fbe_u32_t		outstanding_io_max;			 /*!< The maximum number of outstanding IO's */
        fbe_u32_t		tag_bitfield;				 /*!< Used for tag allocations */
        fbe_u32_t       attributes; /*!< Collection of block transport flags. See @ref fbe_block_transport_flags_e. */
    }
    server_info;
} 
fbe_physical_drive_attributes_t;

/*FBE_PHYSICAL_DRIVE_CONTROL_READ_ICA_STAMP*/
typedef struct fbe_physical_drive_read_ica_stamp_s{
    /*technically we need to read only fbe_imaging_flags_t but for simlicty and not having to allocate another block
    in the PDO, we'll use the user memory but it has to be contigus*/
    fbe_u8_t 	read_ica[FBE_BE_BYTES_PER_BLOCK + sizeof(fbe_sg_element_t) * 2];
}fbe_physical_drive_read_ica_stamp_t;

// get BMS log page
typedef struct fbe_physical_drive_bms_op_asynch_s{
    fbe_physical_drive_information_t        get_drive_info;
    fbe_physical_drive_mgmt_get_log_page_t  get_log_page;
    fbe_bool_t                              display_b;
    fbe_object_id_t                         object_id;
    fbe_object_id_t                         esp_object_id;
    fbe_u32_t                               rw_buffer_length;
    CSX_ALIGN_N(8) fbe_u8_t              *response_buf;
    fbe_sg_element_t                        sg_element[2];
    fbe_status_t                            status;
    void (* completion_function)           (void *context);
    void                                   *context;
}fbe_physical_drive_bms_op_asynch_t;

typedef fbe_u32_t fbe_physical_drive_set_format_block_size_t;

#endif /* FBE_PHYSICAL_DRIVE_H */
