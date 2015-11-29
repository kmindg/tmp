#ifndef BASE_PHYSICAL_DRIVE_PRIVATE_H
#define BASE_PHYSICAL_DRIVE_PRIVATE_H

#include "spid_types.h" 

#include "csx_ext.h"
#include "fbe/fbe_physical_drive.h"

#include "base_discovering_private.h"
#include "fbe_block_transport.h"
#include "fbe_scsi.h"
#include "fbe/fbe_stat.h"
#include "fbe/fbe_drive_configuration_interface.h"
#include "fbe/fbe_payload_cdb_fis.h"
#include "fbe/fbe_notification_interface.h"
#include "fbe_notification.h"

/*set to 1 minutes by default. Drives should spin up < 1min, which is in EMC's SAS logical Spec*/
#define FBE_MAX_BECOMING_READY_LATENCY_DEFAULT 60

#define FBE_DRIVE_INQUIRY_EMC_STRING    " EMC"

extern fbe_lifecycle_const_t FBE_LIFECYCLE_CONST_DATA(base_physical_drive);

/* These are the lifecycle condition ids for a base_physical_drive object. */
typedef enum fbe_base_physical_drive_lifecycle_cond_id_e {
    FBE_BASE_PHYSICAL_DRIVE_LIFECYCLE_COND_INIT_SPECIALIZE_TIME = FBE_LIFECYCLE_COND_FIRST_ID(FBE_CLASS_ID_BASE_PHYSICAL_DRIVE),
    FBE_BASE_PHYSICAL_DRIVE_LIFECYCLE_COND_SPECIALIZE_TIMER, 
    FBE_BASE_PHYSICAL_DRIVE_LIFECYCLE_COND_GET_PROTOCOL_ADDRESS, 
    FBE_BASE_PHYSICAL_DRIVE_LIFECYCLE_COND_TYPE_UNKNOWN,
    FBE_BASE_PHYSICAL_DRIVE_LIFECYCLE_COND_INIT_ACTIVATE_TIME, 
    FBE_BASE_PHYSICAL_DRIVE_LIFECYCLE_COND_ACTIVATE_TIMER, 
    FBE_BASE_PHYSICAL_DRIVE_LIFECYCLE_COND_RESET,
    FBE_BASE_PHYSICAL_DRIVE_LIFECYCLE_COND_REGISTER_CONFIG,
    FBE_BASE_PHYSICAL_DRIVE_LIFECYCLE_COND_UNREGISTER_CONFIG,
    FBE_BASE_PHYSICAL_DRIVE_LIFECYCLE_COND_SET_CACHED_PATH_ATTR,
    FBE_BASE_PHYSICAL_DRIVE_LIFECYCLE_COND_CONFIG_CHANGED,
    FBE_BASE_PHYSICAL_DRIVE_LIFECYCLE_COND_GET_DRIVE_LOCATION_INFO,
    FBE_BASE_PHYSICAL_DRIVE_LIFECYCLE_COND_CHECK_TRAFFIC_LOAD,
    FBE_BASE_PHYSICAL_DRIVE_LIFECYCLE_COND_DEQUEUE_PENDING_IO,
    FBE_BASE_PHYSICAL_DRIVE_LIFECYCLE_COND_DISK_COLLECT_FLUSH,    
    FBE_BASE_PHYSICAL_DRIVE_LIFECYCLE_COND_SPIN_TIME_TIMER,
    FBE_BASE_PHYSICAL_DRIVE_LIFECYCLE_COND_ENTER_GLITCH,
    FBE_BASE_PHYSICAL_DRIVE_LIFECYCLE_COND_HEALTH_CHECK,
    FBE_BASE_PHYSICAL_DRIVE_LIFECYCLE_COND_INITIATE_HEALTH_CHECK,
    FBE_BASE_PHYSICAL_DRIVE_LIFECYCLE_COND_HEALTH_CHECK_COMPLETE,
    FBE_BASE_PHYSICAL_DRIVE_LIFECYCLE_COND_TRACE_LIFECYCLE_STATE,
    FBE_BASE_PHYSICAL_DRIVE_LIFECYCLE_COND_CLEAR_POWER_SAVE_ATTR,
    FBE_BASE_PHYSICAL_DRIVE_LIFECYCLE_COND_RESUME_IO,

    FBE_BASE_PHYSICAL_DRIVE_LIFECYCLE_COND_LAST /* must be last */
} fbe_base_physical_drive_lifecycle_cond_id_t;


/*!****************************************************************************
 *    
 * @struct base_pdo_transaction_state_t
 *  
 * @brief 
 *   Contains transaction state bitmap flags, which are set in the packet payload.
 *   This is used to remember what state the IO is in after it's sent.  
 ******************************************************************************/
typedef fbe_u32_t base_pdo_transaction_state_t;
#define BASE_PDO_TRANSACTION_STATE_IN_DC_REMAP  ((base_pdo_transaction_state_t)1)       /* 0x1 - disk collect rmap */
#define BASE_PDO_TRANSACTION_STATE_IN_REMAP     ((base_pdo_transaction_state_t)1<<1)    /* 0x2 - normal remap */


typedef enum fbe_base_physical_drive_powersave_attr_flags_e {
    FBE_BASE_PHYSICAL_DRIVE_POWERSAVE_SUPPORTED         = 0x00000001,   /* This flag is deprecated */
    FBE_BASE_PHYSICAL_DRIVE_POWERSAVE_ON                = 0x00000002,   /* Power save mode is on */
}fbe_base_physical_drive_powersave_attr_flags; /* This type may be used for debug purposes ONLY */

typedef struct fbe_base_physical_drive_info_s {
    fbe_u8_t revision[FBE_SCSI_INQUIRY_VPD_PROD_REV_SIZE + 1];  
    fbe_u8_t serial_number[FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE + 1];
    fbe_u8_t part_number[FBE_SCSI_INQUIRY_PART_NUMBER_SIZE + 1];
    fbe_u8_t vendor_id[FBE_SCSI_INQUIRY_VENDOR_ID_SIZE + 1];
    fbe_u8_t product_id[FBE_SCSI_INQUIRY_PRODUCT_ID_SIZE + 1];
    fbe_u8_t tla[FBE_SCSI_INQUIRY_TLA_SIZE + 1];
    fbe_u8_t product_rev_len;
    fbe_u8_t bridge_hw_rev[FBE_SCSI_INQUIRY_VPD_BRIDGE_HW_REV_SIZE + 1];
    fbe_drive_vendor_id_t       drive_vendor_id;
    fbe_physical_drive_speed_t speed_capability;
    fbe_u32_t drive_qdepth;
    fbe_u32_t drive_RPM;
    fbe_bool_t supports_enhanced_queuing; /*supports EMC proprietary enhanced queuing, see 999-999-097 rev A07+*/
    fbe_u32_t powercycle_duration;  /* in 500ms increments */
    fbe_u64_t max_becoming_ready_latency_in_seconds;
    fbe_u32_t powersave_attr;
    fbe_drive_price_class_t drive_price_class;
    fbe_pdo_drive_parameter_flags_t  drive_parameter_flags;        
    fbe_u64_t spin_time;/*we'll use for two 32 bit parts to save memory (enough for 8171 years w/o power cycle)*/
    fbe_u32_t spinup_count;/*how many time the drive did a spinup*/
} fbe_base_physical_drive_info_t;

typedef struct fbe_base_physcial_drive_serial_number_s{
    fbe_u8_t serial_number[FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE + 1];
}fbe_physcial_drive_serial_number_t;

/* For Disk Collect */
#define FBE_DC_BLOCK_SIZE 520    /* 520 bytes */
#define FBE_DC_IN_MEMORY_MAX_COUNT  2
#define FBE_DC_MAX_BLOCK_COUNT 4096   /* blocks (1block = 520 bytes) */
#define FBE_DC_SPA_START_LBA 0x0
#define FBE_DC_SPB_START_LBA 0x800   /* 2048d */
#define FBE_DC_MAX_WRITE_ERROR_RETRIES 3

typedef struct fbe_dc_system_time_s {
    fbe_u8_t year;
    fbe_u8_t month;
    fbe_u8_t day;
    fbe_u8_t hour;
    fbe_u8_t minute;
    fbe_u8_t second;
    fbe_u16_t milliseconds;
} fbe_dc_system_time_t;

typedef enum fbe_dc_type_e {
    FBE_DC_NONE,
    FBE_DC_CDB,
    FBE_DC_FIS, 
    FBE_DC_HEADER, 
    FBE_DC_RECEIVE_DIAG,
    
    FBE_DC_TYPE_LAST /* must be last */
} fbe_dc_type_t;

typedef struct fbe_dc_cdb_s{
    fbe_u8_t                        cdb[FBE_PAYLOAD_CDB_CDB_SIZE];
    fbe_u8_t                        sense_info_buffer[FBE_PAYLOAD_CDB_SENSE_INFO_BUFFER_SIZE]; /* OUT scsi sense buffer */
    fbe_payload_cdb_scsi_status_t   payload_cdb_scsi_status; /* OUT scsi status returned by HBA or target device */ 
    fbe_port_request_status_t       port_request_status; /* OUT SRB status mapped to FBE */
}fbe_dc_cdb_t;

typedef struct fbe_dc_fis_s{
      fbe_u8_t                      fis[FBE_PAYLOAD_FIS_SIZE];
      fbe_u8_t                      response_buffer[FBE_PAYLOAD_FIS_RESPONSE_BUFFER_SIZE];
      fbe_port_request_status_t     port_request_status;
}fbe_dc_fis_t;

typedef struct fbe_base_physical_drive_dc_record_s{  
    fbe_dc_type_t                   dc_type;           /* message types are defined below */
    fbe_u16_t                       extended_A07;      /* Drive removal code, 0 if the drive is live           */
    fbe_dc_system_time_t            record_timestamp;
    
    union {
        
        struct {
            fbe_port_number_t         bus;
            fbe_enclosure_number_t    encl;
            fbe_slot_number_t         slot;
            fbe_u8_t                  product_rev[FBE_SCSI_INQUIRY_REVISION_SIZE+1];
            fbe_u8_t                  product_serial_num[FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE+1];
            fbe_u8_t                  tla_part_number[FBE_SCSI_INQUIRY_TLA_SIZE+1];
            fbe_drive_vendor_id_t     drive_vendor_id;
        }header;
            
        struct
        {
            fbe_physical_drive_mgmt_get_error_counts_t error_count; 
            union {
                fbe_dc_cdb_t cdb;
                fbe_dc_fis_t fis;
            }error; 
        }data;
        
        struct 
        {   
            fbe_u8_t scsi_rcv_diag[162];
        }rcv_diag; 
      
    }u;

}fbe_base_physical_drive_dc_record_t; 

/* This is the buffer that is written to the drive when drive has key failure data */
typedef struct fbe_base_physical_drive_dc_data_s{  
    fbe_u64_t       magic_number;      /* Use FBE_MAGIC_NUMBER_OBJECT. */
    fbe_u8_t        retry_count;
    fbe_u8_t        number_record;  /* Number of dc records.     */
    fbe_dc_system_time_t   dc_timestamp; 
    fbe_base_physical_drive_dc_record_t record[FBE_DC_IN_MEMORY_MAX_COUNT];
}fbe_base_physical_drive_dc_data_t; 

//Define this macro to be used in multiple places
#define FBE_BASE_DRIVE_CLOCK_MULTIPLIER         100ULL

/* Current clock value used in calculating busy/idle statistics */
typedef struct fbe_base_drive_clock_value_s{
    fbe_u32_t    value;           /* the number of 100 milliseconds   */
    fbe_u32_t    fraction;        /* the fraction of 100 milliseconds */
}fbe_base_drive_clock_value_t;

/* Info used to calculate disk utilization via busy/idel statistics */
typedef struct fbe_base_drive_util_stats_s{
    fbe_base_drive_clock_value_t    startTime;
    fbe_u64_t                       last_poll;
    fbe_u64_t                       cumulative;
    fbe_u32_t                       event_count;
}fbe_base_drive_util_stats_t;


/*! @enum fbe_base_physical_drive_local_state_e
 *  
 *  @brief Base Physical Drive local state which allows us to know where it is
 *         in its processing of certain conditions.
 *  
 *  @note This should be guarded by a lock
 */
enum fbe_base_physical_drive_local_state_e 
{
    FBE_BASE_DRIVE_LOCAL_STATE_HEALTH_CHECK               = 0x00000001,
    FBE_BASE_DRIVE_LOCAL_STATE_DRIVE_LOCKUP_RECOVERY      = 0x00000002,
    FBE_BASE_DRIVE_LOCAL_STATE_TIMEOUT_QUIESCE_HANDLING   = 0x00000004,
    FBE_BASE_DRIVE_LOCAL_STATE_DOWNLOAD                   = 0x00000008,
    FBE_BASE_DRIVE_LOCAL_STATE_DOWNLOAD_POWERCYCLE_DELAY  = 0x00000010,

    FBE_BASE_DRIVE_LOCAL_STATE_DOWNLOAD_MASK              = (FBE_BASE_DRIVE_LOCAL_STATE_DOWNLOAD |
                                                             FBE_BASE_DRIVE_LOCAL_STATE_DOWNLOAD_POWERCYCLE_DELAY),

    FBE_BASE_DRIVE_LOCAL_STATE_INVALID                    = 0xFFFFFFFF,
};
typedef fbe_u32_t fbe_base_physical_drive_local_state_t;

typedef enum fbe_base_physical_drive_health_check_state_e
{   
    FBE_BASE_PHYSICAL_DRIVE_HEALTH_CHECK_INITIATED,
    FBE_BASE_PHYSICAL_DRIVE_HEALTH_CHECK_STARTED,
    FBE_BASE_PHYSICAL_DRIVE_HEALTH_CHECK_COMPLETED,
    FBE_BASE_PHYSICAL_DRIVE_HEALTH_CHECK_FAILED,
    FBE_BASE_PHYSICAL_DRIVE_HEALTH_CHECK_INVALID

}fbe_base_physical_drive_health_check_state_t;

typedef enum {
    FBE_DRIVE_CMD_STATUS_INVALID            = 0x00000000,
    FBE_DRIVE_CMD_STATUS_SUCCESS            = 0x00000001,   /* success, retryable, and non_retryable are mutually exclusive*/
    FBE_DRIVE_CMD_STATUS_FAIL_RETRYABLE     = 0x00000002,
    FBE_DRIVE_CMD_STATUS_FAIL_NON_RETRYABLE = 0x00000004,
    FBE_DRIVE_CMD_STATUS_RESET              = 0x00000010,  
    FBE_DRIVE_CMD_STATUS_EOL                = 0x00000020,  
    FBE_DRIVE_CMD_STATUS_KILL               = 0x00000040,
} fbe_drive_error_handling_bitmap_t;


/*!****************************************************************************
 *    
 * @struct fbe_base_pdo_flags_t
 *  
 * @brief 
 *   This structure contains basic flags.
 ******************************************************************************/
typedef enum fbe_base_pdo_flags_e{
    FBE_PDO_FLAGS_NONE           = 0,           /*!< No flags are set. */
    FBE_PDO_FLAGS_SET_LINK_FAULT = 0x00000001,   /*!< Notifies monitor context to set link fault */
} fbe_base_pdo_flags_t;



typedef struct fbe_base_physical_drive_s{
    fbe_base_discovering_t base_discovering;

    fbe_block_transport_server_t    block_transport_server;

    fbe_address_t address;
    fbe_generation_code_t generation_code;
    fbe_payload_discovery_element_type_t element_type;

    fbe_base_physical_drive_info_t drive_info;

    fbe_lba_t block_count; /* must be 64 bits */
    fbe_block_size_t  block_size;
      
    fbe_physical_drive_error_counts_t physical_drive_error_counts;
    
    fbe_u8_t rd_wr_cache_value;  /* how host thinks it wants to set write caching param */

    fbe_time_t default_timeout; /*default IO timeout*/
    
    fbe_time_t glitch_time; /* glitch time */
    fbe_time_t glitch_end_time; /* glitch end time */

    /* DIEH */
    fbe_drive_configuration_handle_t    drive_configuration_handle;
    fbe_drive_error_t                   drive_error;
    fbe_drive_dieh_state_t              dieh_state;
    fbe_pdo_dieh_media_threshold_mode_t dieh_mode;    

    fbe_pdo_logical_error_action_t      logical_error_action; /* actions for logical errors from RAID such as mult-bit crcs */
    
    fbe_block_path_attr_flags_t         cached_path_attr;  /* we need to cache certains attributes since they can be set before edges are created (i.e. during drive initialzation).*/

    fbe_u8_t  sp_id;
    fbe_port_number_t      port_number;
    fbe_enclosure_number_t enclosure_number;
    fbe_slot_number_t      slot_number;
    fbe_u16_t slot_reset;
    fbe_u32_t bank_width;
    fbe_atomic_t    total_io_priority;  /*sum of all priorities, later will be devided by total_outstanding_ios to get an average*/
    fbe_atomic_t    total_outstanding_ios;/*total IOs since last monitor, we use it to get average*/
    fbe_atomic_t    highest_outstanding_ios;/*what was the highest number of IOs outstanding*/
    fbe_traffic_priority_t  previous_traffic_priority;/*keep it so we can see if we need to change the load on the edges*/
    /*! This is the pointer to the data of the Disk Collect is sending. 
     * For example, the Disk Collect writes data to the disk, it is the pointer to the sg 
     * list that contains write data. 
     */
    fbe_sg_element_t sg_list[2];  
    FBE_DRIVE_DEAD_REASON death_reason; 
    
    fbe_atomic_t                stats_num_io_in_progress; /* IOs actually on the run queue for stats purposes */
    
    fbe_time_t start_idle_timestamp;

    fbe_time_t start_busy_timestamp;

    fbe_lba_t                   prev_start_lba; /* used to help calculate sum_blocks_seeked PDO stat */    

    fbe_lba_t       dc_lba;        /* Number of blocks are written in disk. */
    fbe_atomic_t    dc_lock;
    fbe_base_physical_drive_dc_data_t dc;  /* Data for Disk Collect. */    

    fbe_block_transport_logical_state_t logical_state;  /* represents SEPs logical state of drive. */

    fbe_u32_t activate_count;    /* number of times we've gone through activate.  */

    /* tunables */
    fbe_time_t    service_time_limit_ms; /* Expected service time limit in milliseconds */  /*TODO: does not need to be a 64bit fbe_time_t*/

    fbe_time_t    quiesce_start_time_ms;  /* keep track of start time so we can resume if it takes too long */

    /* used to detect port lockup */
    fbe_time_t    first_port_timeout;  /* in msec*/
    fbe_u64_t     first_port_timeout_successful_io_count;

    fbe_base_physical_drive_local_state_t   local_state;               /* used for monitor state machines */
    fbe_base_pdo_flags_t                    flags;                     /* basic flags */
    fbe_pdo_maintenance_mode_flags_t        maintenance_mode_bitmap;   /* indicates if and why maintenance mode is requested */    

    FBE_LIFECYCLE_DEF_INST_DATA;
    FBE_LIFECYCLE_DEF_INST_COND(FBE_LIFECYCLE_COND_MAX(FBE_BASE_PHYSICAL_DRIVE_LIFECYCLE_COND_LAST));
    
}fbe_base_physical_drive_t;


typedef struct fbe_base_drive_error_translation_s{
    fbe_scsi_error_code_t           scsi_error_code;
    UINT_32                   drive_ext_code;
    BOOL_8E                   log_event;
}fbe_base_drive_error_translation_t;

typedef struct fbe_base_physical_drive_fault_bit_translation_s
{
    fbe_object_death_reason_t death_reason;
    fbe_bool_t                drive_fault;
    fbe_bool_t                link_fault;
    fbe_bool_t                logical_offline;  
}fbe_base_physical_drive_fault_bit_translation_t;

/* The following defines represent the number of 100ms intervals the pdo sleeps between
 * retries when it encounters various kinds of errors. 
 */
#define FBE_BASE_PHYSICAL_DRIVE_RETRY_MSECS_DEV_BUSY 500
#define FBE_BASE_PHYSICAL_DRIVE_RETRY_MSECS_DEV_RESERVED        600
#define FBE_BASE_PHYSICAL_DRIVE_RETRY_MSECS_NOT_READY           200
/* note, I checked with SBMT on reason for bus reset delay. They could not think of why and it's not called out in T10 spec.
   We can remove this some day if there is a need. -wayne garrett */
#define FBE_BASE_PHYSICAL_DRIVE_RETRY_MSECS_BUS_RESET           200  
#define FBE_BASE_PHYSICAL_DRIVE_RETRY_MSECS_CMD_TIMEOUT         200 
#define FBE_BASE_PHYSICAL_DRIVE_RETRY_MSECS_INCIDENTAL_ABORT    200
#define FBE_BASE_PHYSICAL_DRIVE_RETRY_MSECS_SELECT_TIMEOUT      500
#define FBE_BASE_PHYSICAL_DRIVE_RETRY_MSECS_SUSPENDED           500
#define FBE_BASE_PHYSICAL_DRIVE_RETRY_MSECS_BECOMING_READY     1000
/* It is expected that after 12 seconds, a drive that is spinning up
 * will reach its steady state current draw.  At this point, it safe to 
 * start spinning up the next drive.
 * Changed FBE_BASE_PHYSICAL_DRIVE_RETRY_MSECS_DRIVE_SPINUP to 110. It takes into account the slow clock.
 * This value creates a timeout of 12 seconds.
 * -- DIMS 233001. 
 */
#define FBE_BASE_PHYSICAL_DRIVE_RETRY_MSECS_DRIVE_SPINUP       11000

#define FBE_SPINDOWN_MAX_RETRY_COUNT 5
#define FBE_SPINDOWN_SUPPORTED_STRING "PWR"
#define FBE_FLASH_DRIVE_STRING "EFD"
#define FBE_MLC_FLASH_DRIVE_STRING "M  "
#define FBE_TLA_SUFFIX_STRING_EMPTY "   "
#define FBE_SPINDOWN_STRING_LENGTH 3
#define FBE_TLA_SUFFIX_STRING_LENGTH 3
#define FBE_DUMMY_TLA_ENTRY "000000000"
#define FBE_DUMMY_PN_ENTRY "00000000000"
#define FBE_HEALTH_CHECK_MAX_RETRY_COUNT 3
// TLA suffix defines
#define FBE_TLA_SUFFIX_LENGTH 3
#define FBE_TLA_SED_SUFFIX_KEY_CHAR 'S'

typedef struct fbe_tla_ids
{
    char tla[FBE_SCSI_INQUIRY_TLA_SIZE+1];
}  FBE_TLA_IDs;

typedef struct fbe_part_number_ids_s
{
    fbe_u8_t pn[FBE_SCSI_INQUIRY_PART_NUMBER_SIZE+1];
}  fbe_part_number_ids_t;

fbe_status_t fbe_base_physical_drive_create_object(fbe_packet_t * packet, fbe_object_handle_t * object_handle);
fbe_status_t fbe_base_physical_drive_destroy_object( fbe_object_handle_t object_handle);
fbe_status_t fbe_base_physical_drive_control_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet);

fbe_status_t fbe_base_physical_drive_monitor_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet);

fbe_status_t fbe_base_physical_drive_event_entry(fbe_object_handle_t object_handle, fbe_event_type_t event_type, fbe_event_context_t event_context);

fbe_status_t fbe_base_physical_drive_io_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet);

fbe_status_t fbe_base_physical_drive_monitor_load_verify(void);

fbe_status_t fbe_base_physical_drive_set_capacity(fbe_base_physical_drive_t * base_physical_drive, 
                                                  fbe_lba_t block_count, 
                                                  fbe_block_size_t  block_size);

fbe_status_t fbe_base_physical_drive_set_block_count(fbe_base_physical_drive_t * base_physical_drive, 
                                                    fbe_lba_t block_count);

fbe_status_t fbe_base_physical_drive_set_block_size(fbe_base_physical_drive_t * base_physical_drive, 
                                                      fbe_block_size_t  block_size);

fbe_status_t fbe_base_physical_drive_get_capacity(fbe_base_physical_drive_t * base_physical_drive, 
                                                  fbe_lba_t * block_count, 
                                                  fbe_block_size_t * block_size);
fbe_status_t fbe_base_physical_drive_get_exported_capacity(fbe_base_physical_drive_t * base_physical_drive, 
                                                           fbe_block_size_t client_block_size, 
                                                           fbe_lba_t * capacity_in_client_blocks_p);
fbe_status_t fbe_base_physical_drive_get_optimal_block_alignment(fbe_base_physical_drive_t * base_physical_drive, 
                                                                 fbe_block_size_t client_block_size, 
                                                                 fbe_block_size_t * optimal_block_alignment_p);
fbe_status_t fbe_base_physical_drive_set_sp_id(fbe_base_physical_drive_t * base_physical_drive, fbe_u8_t sp_id);
fbe_status_t fbe_base_physical_drive_get_sp_id(fbe_base_physical_drive_t * base_physical_drive, fbe_u8_t * sp_id);
fbe_status_t fbe_base_physical_drive_set_port_number(fbe_base_physical_drive_t * base_physical_drive, fbe_port_number_t port_number);
fbe_status_t fbe_base_physical_drive_get_port_number(fbe_base_physical_drive_t * base_physical_drive, fbe_port_number_t * port_number);
fbe_status_t fbe_base_physical_drive_set_enclosure_number(fbe_base_physical_drive_t * base_physical_drive, fbe_enclosure_number_t enclosure_number);
fbe_status_t fbe_base_physical_drive_get_enclosure_number(fbe_base_physical_drive_t * base_physical_drive, fbe_enclosure_number_t * enclosure_number);
fbe_status_t fbe_base_physical_drive_set_slot_number(fbe_base_physical_drive_t * base_physical_drive, fbe_slot_number_t slot_number);
fbe_status_t fbe_base_physical_drive_get_slot_number(fbe_base_physical_drive_t * base_physical_drive, fbe_slot_number_t * slot_number);
fbe_status_t fbe_base_physical_drive_set_bank_width(fbe_base_physical_drive_t * base_physical_drive, fbe_u32_t bankWidth);
fbe_status_t fbe_base_physical_drive_get_bank_width(fbe_base_physical_drive_t * base_physical_drive, fbe_u32_t * bankWidth);
fbe_status_t fbe_base_physical_drive_get_serial_number(fbe_base_physical_drive_t * base_physical_drive, fbe_physcial_drive_serial_number_t *ser_no);
fbe_status_t fbe_base_physical_drive_set_powersave_attr(fbe_base_physical_drive_t * base_physical_drive, fbe_u32_t attr);
fbe_status_t fbe_base_physical_drive_get_powersave_attr(fbe_base_physical_drive_t * base_physical_drive, fbe_u32_t * attr);
fbe_bool_t fbe_base_physical_drive_is_spindown_qualified(fbe_base_physical_drive_t * base_physical_drive, fbe_u8_t * tla);
fbe_bool_t base_physical_drive_is_vault_drive(fbe_base_physical_drive_t * base_physical_drive);
fbe_bool_t fbe_base_physical_drive_is_unmap_supported_override(fbe_base_physical_drive_t * base_physical_drive);
fbe_bool_t fbe_base_physical_drive_is_sed_drive_tla(fbe_base_physical_drive_t * base_physical_drive, fbe_u8_t * tla);
fbe_status_t fbe_base_pdo_send_discovery_edge_control_packet(fbe_base_physical_drive_t * base_physical_drive,
                                                             fbe_payload_control_operation_opcode_t control_code,
                                                             fbe_payload_control_buffer_t buffer,
                                                             fbe_payload_control_buffer_length_t buffer_length,                                                
                                                             fbe_packet_attr_t attr);
fbe_status_t fbe_base_physical_drive_get_platform_info(fbe_base_physical_drive_t * base_physical_drive, SPID_PLATFORM_INFO *platform_info);


/* Executer functions */
fbe_status_t fbe_base_physical_drive_get_protocol_address(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet);
fbe_status_t fbe_base_physical_drive_get_drive_location_info(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet);
fbe_status_t fbe_base_physical_drive_send_power_cycle(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet, fbe_bool_t completed);
fbe_status_t fbe_base_physical_drive_get_permission(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet, fbe_bool_t get);
fbe_status_t fbe_base_physical_drive_reset_slot(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet);

/* Usurper functions */
fbe_status_t fbe_base_physical_drive_get_dev_info(fbe_base_physical_drive_t * base_physical_drive, fbe_physical_drive_information_t * drive_information);
fbe_status_t fbe_base_physical_drive_reset_device(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet);


/* Utils */
void         fbe_base_physical_drive_clear_dieh_state(fbe_base_physical_drive_t * base_physical_drive);
fbe_time_t   fbe_base_physical_drive_reset_statistics(fbe_base_physical_drive_t * base_physical_drive);
fbe_bool_t   fbe_base_drive_test_and_set_health_check(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet);
fbe_bool_t   fbe_base_drive_is_service_time_exceeded(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet, fbe_time_t service_time_limit_ms);
void         fbe_base_drive_initiate_health_check(fbe_base_physical_drive_t * base_physical_drive, fbe_bool_t is_service_timeout);
fbe_bool_t   fbe_base_drive_is_remap_service_time_exceeded(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet);
fbe_bool_t   fbe_base_pdo_is_system_drive(fbe_base_physical_drive_t * base_physical_drive);


/* Common functions */
fbe_drive_type_t fbe_base_physical_drive_get_drive_type(fbe_base_physical_drive_t *base_physical_drive);
fbe_status_t fbe_base_physical_drive_get_adderess(fbe_base_physical_drive_t * base_physical_drive, fbe_address_t * address);
fbe_status_t fbe_base_physical_drive_update_error_counts(fbe_base_physical_drive_t * base_physical_drive, fbe_physical_drive_mgmt_get_error_counts_t * get_error_counts);
fbe_status_t fbe_base_physical_drive_reset_error_counts(fbe_base_physical_drive_t * base_physical_drive);

fbe_status_t fbe_base_physical_drive_set_block_transport_const(fbe_base_physical_drive_t * base_physical_drive,
                                                              fbe_block_transport_const_t * block_transport_const);

fbe_status_t fbe_base_physical_drive_set_outstanding_io_max(fbe_base_physical_drive_t * base_physical_drive,
                                                            fbe_u32_t outstanding_io_max);

fbe_status_t fbe_base_physical_drive_set_stack_limit(fbe_base_physical_drive_t * base_physical_drive);

fbe_status_t fbe_base_physical_drive_get_outstanding_io_max(fbe_base_physical_drive_t * base_physical_drive,
                                                  fbe_u32_t *outstanding_io_max);
fbe_status_t fbe_base_physical_drive_get_outstanding_os_io_max(fbe_base_physical_drive_t * base_physical_drive,
                                                  fbe_u32_t *outstanding_io_max);

void fbe_base_physical_drive_set_enhanced_queuing_supported(fbe_base_physical_drive_t * base_physical_drive, fbe_bool_t is_supported);
fbe_bool_t fbe_base_physical_drive_is_enhanced_queuing_supported(fbe_base_physical_drive_t * base_physical_drive);
void fbe_base_physical_drive_set_path_attr(fbe_base_physical_drive_t * base_physical_drive, fbe_block_path_attr_flags_t path_attr);

fbe_status_t fbe_base_physical_drive_bouncer_entry(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet);

fbe_status_t fbe_base_physical_drive_get_drive_info(fbe_base_physical_drive_t * base_physical_drive,
                                                               fbe_base_physical_drive_info_t ** drive_info);

fbe_status_t fbe_base_physical_drive_block_transport_enable_tags(fbe_base_physical_drive_t * base_physical_drive);

fbe_status_t fbe_base_physical_drive_set_default_timeout(fbe_base_physical_drive_t * base_physical_drive, fbe_time_t timeout);
fbe_status_t fbe_base_physical_drive_get_default_timeout(fbe_base_physical_drive_t * base_physical_drive, fbe_time_t *timeout);

fbe_status_t fbe_base_physical_drive_io_fail(fbe_base_physical_drive_t * base_physical_drive, 
                                             fbe_drive_configuration_handle_t drive_configuration_handle,
                                             fbe_drive_configuration_record_t * threshold_rec,
                                             fbe_payload_cdb_fis_error_flags_t error_flags,
                                             fbe_u8_t opcode,
                                             fbe_port_request_status_t port_status, 
                                             fbe_scsi_sense_key_t sk, fbe_scsi_additional_sense_code_t asc, fbe_scsi_additional_sense_code_qualifier_t ascq);

fbe_status_t fbe_base_physical_drive_io_success(fbe_base_physical_drive_t * base_physical_drive);


fbe_status_t 
fbe_base_physical_drive_get_configuration_handle(fbe_base_physical_drive_t * base_physical_drive, 
                                                 fbe_drive_configuration_handle_t * drive_configuration_handle);

fbe_status_t 
fbe_base_physical_drive_reset_completed(fbe_base_physical_drive_t * base_physical_drive);

fbe_status_t 
fbe_base_physical_drive_get_drive_vendor_id(fbe_base_physical_drive_t * base_physical_drive, fbe_drive_vendor_id_t * vendor_id);

fbe_status_t fbe_base_physical_drive_set_powercycle_duration(fbe_base_physical_drive_t * base_physical_drive, fbe_u32_t duration);
fbe_status_t fbe_base_physical_drive_get_powercycle_duration(fbe_base_physical_drive_t * base_physical_drive, fbe_u32_t *duration);
void fbe_base_physical_drive_customizable_trace(fbe_base_physical_drive_t * base_physical_drive, 
                      fbe_trace_level_t trace_level,
                      const fbe_char_t * fmt, ...)
                      __attribute__((format(__printf_func__,3,4)));

fbe_status_t fbe_base_physical_drive_set_retry_msecs(fbe_payload_ex_t * payload, fbe_scsi_error_code_t error_code);

fbe_status_t fbe_base_physical_drive_get_retry_msecs(fbe_scsi_error_code_t error_code,
                                                     fbe_time_t *msecs_to_retry);
fbe_status_t fbe_base_physical_drive_process_block_transport_event(fbe_block_transport_event_type_t event_type,
                                                                   fbe_block_trasnport_event_context_t context);
void         fbe_trace_RBA_traffic(fbe_base_physical_drive_t* base_physical_drive, fbe_lba_t lba, fbe_block_count_t block_count, fbe_u32_t io_traffic_info);
fbe_status_t base_physical_drive_get_dc_error_counts(fbe_base_physical_drive_t * base_physical_drive);
fbe_status_t base_physical_drive_fill_dc_header(fbe_base_physical_drive_t * base_physical_drive);
fbe_status_t base_physical_drive_fill_dc_death_reason(fbe_base_physical_drive_t * base_physical_drive, FBE_DRIVE_DEAD_REASON death_reason);
fbe_status_t fbe_get_dc_system_time(fbe_dc_system_time_t * p_system_time);
void         base_physical_drive_increment_down_time(fbe_base_physical_drive_t * base_physical_drive);
void         base_physical_drive_increment_up_time(fbe_base_physical_drive_t * base_physical_drive);
fbe_status_t base_physical_drive_get_down_time(fbe_base_physical_drive_t * base_physical_drive, fbe_u32_t *get_down_time);
fbe_status_t base_physical_drive_get_up_time(fbe_base_physical_drive_t * base_physical_drive, fbe_u32_t *get_up_time);
fbe_status_t fbe_base_physical_drive_set_glitch_time(fbe_base_physical_drive_t * base_physical_drive, fbe_u64_t glitch_time);
fbe_status_t fbe_base_physical_drive_get_glitch_time(fbe_base_physical_drive_t * base_physical_drive, fbe_u64_t * glitch_time);
fbe_status_t fbe_base_physical_drive_set_glitch_end_time(fbe_base_physical_drive_t * base_physical_drive, fbe_u64_t glitch_end_time);
fbe_status_t fbe_base_physical_drive_get_glitch_end_time(fbe_base_physical_drive_t * base_physical_drive, fbe_u64_t * glitch_end_time);
fbe_status_t fbe_base_physical_drive_handle_negotiate_block_size(fbe_base_physical_drive_t *physical_drive_p,
                                                                 fbe_packet_t *packet_p);
fbe_status_t fbe_base_physical_drive_executor_fail_drive(fbe_base_physical_drive_t * base_physical_drive_p, fbe_base_physical_drive_death_reason_t reason);
void         fbe_base_physical_drive_xlate_scsi_code_to_drive_ext_status (fbe_base_drive_error_translation_t *xlate_err_tuple);
void         fbe_base_physical_drive_xlate_death_reason_to_fault_bit (fbe_base_physical_drive_fault_bit_translation_t *xlate_fault_bit);
fbe_status_t fbe_base_physical_drive_log_event(fbe_base_physical_drive_t * base_physical_drive, fbe_scsi_error_code_t error, fbe_packet_t * packet, fbe_u32_t last_error_code, const fbe_payload_cdb_operation_t * payload_cdb_operation);
fbe_status_t fbe_base_physical_drive_create_drive_string(fbe_base_physical_drive_t * base_physical_drive, fbe_u8_t * deviceStr, fbe_u32_t bufferLen, fbe_bool_t completeStr);
fbe_u8_t *   fbe_base_physical_drive_get_error_initiator(fbe_u32_t last_error_code);
fbe_status_t fbe_base_physical_drive_get_additional_data(const fbe_payload_cdb_operation_t* payload_cdb_operation, fbe_u8_t *additionalDataStr);
fbe_status_t fbe_base_physical_drive_set_fail_condition_with_fault(fbe_base_physical_drive_t * base_physical_drive,
                                                            fbe_object_death_reason_t death_reason);
fbe_u8_t *   fbe_base_physical_drive_get_path_attribute_string(fbe_u32_t path_attribute);
fbe_status_t base_physical_drive_set_down_time(fbe_base_physical_drive_t * base_physical_drive, fbe_u32_t *set_down_time);
fbe_status_t base_physical_drive_set_up_time(fbe_base_physical_drive_t * base_physical_drive, fbe_u32_t *set_up_time);

fbe_status_t fbe_base_physical_drive_get_logical_drive_state(fbe_base_physical_drive_t * base_physical_drive, fbe_block_transport_logical_state_t *logical_state);
void         fbe_base_physical_drive_send_data_change_notification(fbe_base_physical_drive_t * base_physical_drive, fbe_notification_data_changed_info_t * pDataChangeInfo);
fbe_status_t fbe_base_physical_drive_log_health_check_event(fbe_base_physical_drive_t *base_physical_drive, 
                                                            fbe_base_physical_drive_health_check_state_t state);
fbe_u8_t *   fbe_base_physical_drive_get_health_check_status_string(fbe_base_physical_drive_health_check_state_t state);

/* fbe_base_physical_drive_class.c */
fbe_status_t fbe_base_physical_drive_class_control_entry(fbe_packet_t * packet);



/*!****************************************************************************
 * fbe_base_physical_drive_lock
 ******************************************************************************
 *
 * @brief
 *    This function locks the specified PDO via the base object
 *    lock.
 *
 * @param   base_physical_drive_p   -  pointer to base PDO
 *
 * @return  none
 *
 * @version
 *    02/18/2013 Wayne Garrett - Created. 
 *
 ******************************************************************************/

static __forceinline void
fbe_base_physical_drive_lock(fbe_base_physical_drive_t* const base_physical_drive_p)
{
    fbe_base_object_lock( (fbe_base_object_t *)base_physical_drive_p );
}   

/*!****************************************************************************
 * fbe_base_physical_drive_unlock
 ******************************************************************************
 *
 * @brief
 *    This function unlocks the specified PDO via the base object
 *    lock.
 *
 * @param   base_physical_drive_p   -  pointer to base PDO
 *
 * @return  none
 *
 * @version
 *    02/18/2013 Wayne Garrett - Created. 
 *
 ******************************************************************************/

static __forceinline void
fbe_base_physical_drive_unlock(fbe_base_physical_drive_t* const base_physical_drive_p)
{
    fbe_base_object_unlock( (fbe_base_object_t *)base_physical_drive_p );
}   



/* Accessors related to local state.
 */
static __forceinline fbe_bool_t fbe_base_physical_drive_is_local_state_set(fbe_base_physical_drive_t *base_physical_drive_p,
                                                                           fbe_base_physical_drive_local_state_t local_state)
{
    return ((base_physical_drive_p->local_state & local_state) == local_state);
}
static __forceinline void fbe_base_physical_drive_init_local_state(fbe_base_physical_drive_t *base_physical_drive_p)
{
    base_physical_drive_p->local_state = 0;
    return;
}
static __forceinline void fbe_base_physical_drive_set_local_state(fbe_base_physical_drive_t *base_physical_drive_p,
                                                         fbe_base_physical_drive_local_state_t local_state)
{
    base_physical_drive_p->local_state |= local_state;
}

static __forceinline void fbe_base_physical_drive_clear_local_state(fbe_base_physical_drive_t *base_physical_drive_p,
                                                           fbe_base_physical_drive_local_state_t local_state)
{
    base_physical_drive_p->local_state &= ~local_state;
}
static __forceinline void fbe_base_physical_drive_get_local_state(fbe_base_physical_drive_t *base_physical_drive_p,
                                                         fbe_base_physical_drive_local_state_t *local_state)
{
     *local_state = base_physical_drive_p->local_state;
}

/* Accessors/Mutators for 32bit flags.
 */
static __forceinline fbe_bool_t fbe_base_pdo_are_flags_set(fbe_base_physical_drive_t *base_physical_drive_p,
                                                           fbe_base_pdo_flags_t flags)
{
    return ((base_physical_drive_p->flags & flags) == flags);
}

static __forceinline void fbe_base_pdo_init_flags(fbe_base_physical_drive_t *base_physical_drive_p)
{
    base_physical_drive_p->flags = FBE_PDO_FLAGS_NONE;
    return;
}

static __forceinline void fbe_base_pdo_set_flags(fbe_base_physical_drive_t *base_physical_drive_p,
                                                 fbe_base_pdo_flags_t flags)
{
    base_physical_drive_p->flags |= flags;
}

static __forceinline void fbe_base_pdo_set_flags_under_lock(fbe_base_physical_drive_t *base_physical_drive_p,
                                                            fbe_base_pdo_flags_t flags)
{
    fbe_base_physical_drive_lock(base_physical_drive_p);
    fbe_base_pdo_set_flags(base_physical_drive_p, flags);
    fbe_base_physical_drive_unlock(base_physical_drive_p);
}

static __forceinline void fbe_base_pdo_clear_flags(fbe_base_physical_drive_t *base_physical_drive_p,
                                                   fbe_base_pdo_flags_t flags)
{
    base_physical_drive_p->flags &= ~flags;
}

static __forceinline void fbe_base_pdo_clear_flags_under_lock(fbe_base_physical_drive_t *base_physical_drive_p,
                                                              fbe_base_pdo_flags_t flags)
{   
    fbe_base_physical_drive_lock(base_physical_drive_p);
    fbe_base_pdo_clear_flags(base_physical_drive_p, flags);
    fbe_base_physical_drive_unlock(base_physical_drive_p);
}

static __forceinline void fbe_base_pdo_get_flags(fbe_base_physical_drive_t *base_physical_drive_p,
                                                 fbe_base_pdo_flags_t *flags)
{
     *flags = base_physical_drive_p->flags;
}


/*!**************************************************************
 * @fn fbe_pdo_maintenance_mode_set_flag
 ****************************************************************
 * @brief
 *  This function sets a flag in the maintenance mode bitmap
 * 
 * @param bitmap - pointer to bitmap.
 * @param flag   - maintenance mode flag.
 * 
 * @return void
 *
 * @version
 *  12/10/2013  Wayne Garrett  - Created.
 *
 ****************************************************************/
static __forceinline void fbe_pdo_maintenance_mode_set_flag(fbe_pdo_maintenance_mode_flags_t * bitmap,
                                                            fbe_pdo_maintenance_mode_flags_t flag)
{
    (*bitmap) |= flag;
}


/*!**************************************************************
 * @fn fbe_pdo_maintenance_mode_clear_flag
 ****************************************************************
 * @brief
 *  This function clear a flag in the maintenance mode bitmap
 * 
 * @param bitmap - pointer to bitmap..
 * @param flag   - maintenance mode flag.
 * 
 * @return void
 *
 * @version
 *  12/10/2013  Wayne Garrett  - Created.
 *
 ****************************************************************/
static __forceinline void fbe_pdo_maintenance_mode_clear_flag(fbe_pdo_maintenance_mode_flags_t * bitmap,
                                                              fbe_pdo_maintenance_mode_flags_t flag)
{
    (*bitmap) &= ~flag;
}

static __forceinline fbe_bool_t fbe_pdo_maintenance_mode_is_flag_set(const fbe_pdo_maintenance_mode_flags_t * bitmap,
                                                                     fbe_pdo_maintenance_mode_flags_t flag)
{
    return (((*bitmap) & flag) == flag);
}



/* Accessors related to payload transaction
 */
static __forceinline void base_pdo_payload_transaction_flag_init(fbe_payload_physical_drive_transaction_t *transaction)
{
    transaction->transaction_state = 0;
}
static __forceinline fbe_bool_t base_pdo_payload_transaction_flag_is_set(const fbe_payload_physical_drive_transaction_t *transaction, base_pdo_transaction_state_t flag)
{
    return ((transaction->transaction_state & flag) == flag);
}
static __forceinline void base_pdo_payload_transaction_flag_set(fbe_payload_physical_drive_transaction_t *transaction, base_pdo_transaction_state_t flag)
{
    transaction->transaction_state |= flag;
}
static __forceinline void base_pdo_payload_transaction_flag_clear(fbe_payload_physical_drive_transaction_t *transaction, base_pdo_transaction_state_t flag)
{
    transaction->transaction_state &= ~flag;
}

/*!****************************************************************************
 * fbe_base_physical_drive_is_io_aligned_to_block_size()
 ******************************************************************************
 *
 * @brief   This function simply checks if the I/O is aligned to the physical
 *          block size or not.
 *
 * @param   base_physical_drive_p   -  pointer to base PDO
 *
 * @return  bool
 *
 * @version
 *  11/21/2013  Ron Proulx  - Created.
 *
 ******************************************************************************/
static __forceinline fbe_bool_t fbe_base_physical_drive_is_io_aligned_to_block_size(fbe_base_physical_drive_t *const base_physical_drive_p,
                                                               const fbe_block_size_t client_block_size,
                                                               const fbe_lba_t lba, const fbe_block_count_t blocks)
{
    fbe_block_count_t   aligned_client_blocks;

    if (client_block_size == base_physical_drive_p->block_size)
    {
        return FBE_TRUE;
    }

    /*! @note Block Size Virtual is not longer supported.  Therefore the 
     *        following are require if the physical block size:
     *          o Must be equal or greater than client block size
     *          o Must be a multiple of block size
     */
    if ((base_physical_drive_p->block_size < client_block_size)        ||
        ((base_physical_drive_p->block_size % client_block_size) != 0)    )
    {
        return FBE_FALSE;
    }

    /* If the start and end of the command is aligned to the
     * optimal block size, then we are considered aligned.
     */
    aligned_client_blocks = (base_physical_drive_p->block_size / client_block_size);
    if (((lba % aligned_client_blocks) == 0)            &&
        (((lba + blocks) % aligned_client_blocks) == 0)    )
    {
        return FBE_TRUE;
    }

    /* Not aligned
     */
    return FBE_FALSE;
}
/***********************************************************
 * end fbe_base_physical_drive_is_io_aligned_to_block_size()
 ***********************************************************/

/*!****************************************************************************
 * fbe_base_physical_drive_dieh_is_emeh_mode()
 ******************************************************************************
 *
 * @brief
 *    This function looks at the DIEH reliability mode to determine if the drive
 *    is in EMEH (Extended Media Error Handling) mode or not.
 *
 * @param   base_physical_drive_p   -  pointer to base PDO
 *
 * @return  bool - True - Drive is in EMEH mode
 *
 * @version
 *  06/26/2015  Ron Proulx  - Created.
 *
 ******************************************************************************/

static __forceinline fbe_bool_t
fbe_base_physical_drive_dieh_is_emeh_mode(fbe_base_physical_drive_t* const base_physical_drive_p)
{
    if ((base_physical_drive_p->dieh_mode == FBE_DRIVE_MEDIA_MODE_UNKNOWN)            ||
        (base_physical_drive_p->dieh_mode == FBE_DRIVE_MEDIA_MODE_THRESHOLDS_DEFAULT)    )
    {
        return FBE_FALSE;
    }
    return FBE_TRUE;
}   


#define FBE_TRACE_PDO_DSK_POS_SIZE 15
#define FBE_TRACE_PDO_SER_NUM_SIZE (FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE+6)

#define FBE_MAX_TRANSFER_BLOCKS        2048  /* Maximum transfer blocks */
#define FBE_MLC_HIGH_ENDURANCE_LIMIT   25    /*A value >= for High Endurance (HE - Fast Cache - EFD) MLC Drive */
#define FBE_MLC_MEDIUM_ENDURANCE_LIMIT 10    /*A value >= or < FBE_MLC_HIGH_ENDURANCE_LIMIT for Medium Endurance (ME - Fast Cache/Fast VP) MLC Drive */
#define FBE_MLC_LOW_ENDURANCE_LIMIT    3     /*A value >= or < FBE_MLC_MEDIUM_ENDURANCE_LIMIT for Low Endurance (LE - Fast VP) MLC Drive */
#define FBE_MLC_READ_INTENSIVE_LIMIT   1     /*A value >= or < FBE_MLC_LOW_ENDURANCE_LIMIT for Read Intensive (RI - Fast VP) MLC Drive */
#define FBE_SAS_NL_RPM_LIMIT           7299  /* Max spindle speed for NL SAS drive */

#define FBE_VAULT_OUTSTANDING_OS_IO_MAX 7  /* max qdepth for OS IOs in vault drives */
#define FBE_PHYSICAL_DRIVE_STRING_LENGTH 210


/* Drive_Extended_Status */

#define DRV_EXT_NO_ERROR                      0x00
#define DRV_EXT_REQ_SENSE_FAILED              0x01
#define DRV_EXT_DEV_DET_PARITY_ERROR          0x02
#define DRV_EXT_DEV_BUSY_RESERVED             0x03
#define DRV_EXT_TIMEOUT_ON_REMAP              0x04
#define DRV_EXT_BAD_BLOCK                     0x05
#define DRV_EXT_TIMEOUT                       0x06
#define DRV_EXT_SELECTION_TIMEOUT             0x07
#define DRV_EXT_NOT_READY                     0x08
#define DRV_EXT_HARDWARE_ERROR                0x09
#define DRV_EXT_BAD_FORMAT                    0x0A
#define DRV_EXT_WRONG_DEVICE                  0x0B
#define DRV_EXT_USER_ABORT                    0x0C
#define DRV_EXT_DEV_ILLEGAL_REQ               0x0D
#define DRV_EXT_DEV_RESET                     0x0E
#define DRV_EXT_MODE_SEL_OCCURRED             0x0F
#define DRV_EXT_UNIT_ATTENTION                0x10
#define DRV_EXT_DEV_ABORTED_CMD               0x11
#define DRV_EXT_BUS_RESET                     0x12
#define DRV_EXT_MODE_SENSE_ERROR              0x13
#define DRV_EXT_READ_REMAP_ERROR              0x14
#define DRV_EXT_MODE_PAGE_DATA_BAD            0x15
#define DRV_EXT_INVALID_REQUEST               0x16
#define DRV_EXT_UNKNOWN_RESPONSE              0x17
#define DRV_EXT_SCSI_DEVUNITIALIZED           0x18
#define DRV_EXT_PFA_THRESHOLD_REACHED         0x19
#define DRV_EXT_DEV_INSUFFICIENT_CAPACITY     0x1A
#define DRV_EXT_BUS_SHUT_DOWN                 0x1B
#define DRV_EXT_UNEXPECTED_SENSE_KEY          0x1C
#define DRV_EXT_UNKNOWN_SCSI_ERROR            0x1D
#define DRV_EXT_DIAG_MISCOMPARE               0x1E
#define DRV_EXT_SCSI_DATA_OFFSET_ERROR        0x1F
#define DRV_EXT_DEVICE_DEAD                   0x20
#define DRV_EXT_RECOVERED_ERROR               0x21
#define DRV_EXT_ECC_BAD_BLOCK                 0x22
#define DRV_EXT_NO_ERROR_SENSE_KEY            0x23
#define DRV_EXT_RECOVERED_REMAP_REQUIRED      0x24
#define DRV_EXT_SCSI_INT_SCB_NOT_FOUND        0x26
#define DRV_EXT_SCSI_SLOT_BUSY                0x27
#define DRV_EXT_SYNCH_CHANGE_OCCURRED         0x28
#define DRV_EXT_START_IN_PROGRESS             0x29
#define DRV_EXT_SCSI_XFERCOUNTNOTZERO         0x2A
#define DRV_EXT_SCSI_BADINTERRUPT             0x2B
#define DRV_EXT_SCSI_TOOMUCHDATA              0x2C
#define DRV_EXT_SCSI_BADBUSPHASE              0x2D
#define DRV_EXT_SCSI_BADMESSAGE               0x2E
#define DRV_EXT_SCSI_BADRESELECTION           0x2F
#define DRV_EXT_SCSI_PARITYERROR              0x30
#define DRV_EXT_SCSI_MSGPARERR                0x31
#define DRV_EXT_SCSI_STATUSPARITYERROR        0x32
#define DRV_EXT_SCSI_BADSTATUS                0x33
#define DRV_EXT_SCSI_INVALIDSCB               0x34
#define DRV_EXT_CHECK_CHKSUM_FAILED           0x35
#define DRV_EXT_MEDIA_WRITE_ERR_CORRECTED     0x36
#define DRV_EXT_IO_LINKDOWN_ABORT             0x37
#define DRV_EXT_SCSI_UNEXPECTEDDISCONNECT     0x38
#define DRV_EXT_SCSI_USERRESET                0x39
#define DRV_EXT_RECOVERED_ERROR_AUTO_REMAPPED 0x3A
#define DRV_EXT_RECOVERED_ERROR_CANT_REMAP    0x3B
#define DRV_EXT_RECOVERED_ERROR_REMAP_FAILED  0x3C
#define DRV_EXT_MEDIA_ERROR_CANT_REMAP        0x3D
#define DRV_EXT_MEDIA_ERROR_REMAP_FAILED      0x3E
#define DRV_EXT_DEFECT_LIST_ERROR             0x3F
#define DRV_EXT_HARDWARE_ERROR_NO_SPARE       0xA0
#define DRV_EXT_MEDIA_ERROR_WRITE_ERROR       0xA1
#define DRV_EXT_WR_CACHE_ENA                  0xA2
#define DRV_EXT_WR_CACHE_ENA_FAILED           0xA3
#define DRV_EXT_WR_CACHE_DIS                  0xA4
#define DRV_EXT_WR_CACHE_DIS_FAILED           0xA5
#define DRV_EXT_DRIVE_DOWNLOAD_ACK_RCVD       0xA6
#define DRV_EXT_SEEK_POS_ERROR                0xA7
#define DRV_EXT_SEL_ID_ERROR                  0xA8
#define DRV_EXT_UNSUPPORTED_DRIVE             0xA9
#define DRV_EXT_SUPER_CAP_FAILURE             0xAA
#define DRV_EXT_DRIVE_TABLE_REBUILD           0xAB
#define DRV_EXT_DEFERRED_ERROR                0xAC
#define DRV_EXT_RAID_MULTI_BITS_CRC_ERROR     0XAD 
#define DRV_EXT_RAID_SINGLE_BIT_CRC_ERROR     0XAE 
#define DRV_EXT_AEL_REPLACE_DRIVE             0xAF
#define DRV_EXT_AEL_PROACTIVE_COPY            0xB0
#define DRV_EXT_WONT_SPIN_UP                  0XB1    
#define DRV_EXT_CLAR_STRING_MISSING           0XB2  
#define DRV_EXT_DEVICE_GONE                   0xB3
#define DRV_EXT_KILLED_BY_CM                  0xB4
#define DRV_EXT_DIE_RETIREMENT_START          0xB7
#define DRV_EXT_DIE_RETIREMENT_END            0xB8
#define DRV_EXT_TEMP_THRESHOLD_REACHED        0xC0
#define DRV_EXT_WRITE_PROTECT                 0xC1
#define DRV_EXT_SENSE_DATA_MISSING            0xC2
#define DRV_EXT_QUEUED_IO_TIMER_EXP           0xC3
#define DRV_EXT_BASE_PDO_SPECIALIZED_TIMER_EXPIRED       0xC4
#define DRV_EXT_BASE_DISCOVERED_ACTIVATE_TIMER_EXPIRED   0xC5
#define DRV_EXT_BASE_DISCOVERED_HARDWARE_NOT_READY   0xC6
#define DRV_EXT_BASE_PDO_ENCLOSURE_OBJECT_FAILED     0xC7
#define DRV_EXT_BASE_PDO_FAILED_TO_SPIN_DOWN         0xC8
#define DRV_EXT_SAS_PDO_MAX_REMAPS_EXCEEDED          0xC9
#define DRV_EXT_SAS_PDO_FAILED_TO_REASSIGN           0xCA
/* Extended status for threshold exceeded */
#define DRV_EXT_DIEH_LINK_THRESHOLD_EXCEEDED         0xCB
#define DRV_EXT_DIEH_RECOVERED_THRESHOLD_EXCEEDED    0xCC
#define DRV_EXT_DIEH_MEDIA_THRESHOLD_EXCEEDED        0xCD
#define DRV_EXT_DIEH_HARDWARE_THRESHOLD_EXCEEDED     0xCE
#define DRV_EXT_DIEH_DATA_THRESHOLD_EXCEEDED         0xCF
#define DRV_EXT_DIEH_DEFINED_FATAL_ERROR             0xD0
#define DRV_EXT_SAS_PDO_WRITE_PROTECT                0xD1
#define DRV_EXT_SAS_PDO_DRIVE_NOT_SPINNING           0xD2
#define DRV_EXT_SAS_PDO_FAILED_TO_PROCESS_READ_CAPACITY  0xD3
#define DRV_EXT_SAS_PDO_FAILED_TO_PROCESS_MODE_SENSE 0xD4
#define DRV_EXT_SAS_PDO_FAILED_TO_SET_DEV_INFO       0xD5
#define DRV_EXT_SAS_PDO_DEFECT_LIST_ERRORS           0xD6
#define DRV_EXT_SAS_PDO_FAILED_VIA_SIMULATION        0xD7
#define DRV_EXT_INCIDENTAL_ABORT                     0xDD
#define DRV_EXT_ENCRYPTION_NOT_ENABLED               0xDE
#define DRV_EXT_ENCRYPTION_BAD_KEY_HANDLE            0xDF
#define DRV_EXT_ENCRYPTION_KEY_WRAP_ERROR            0xE0



#define DRV_MAX_EXT_STATUS   DRV_EXT_ENCRYPTION_KEY_WRAP_ERROR

fbe_status_t fbe_base_physical_drive_set_io_throttle_max(fbe_base_physical_drive_t * base_physical_drive,fbe_block_count_t io_throttle_max);


#endif /* BASE_PHYSICAL_DRIVE_PRIVATE_H */
