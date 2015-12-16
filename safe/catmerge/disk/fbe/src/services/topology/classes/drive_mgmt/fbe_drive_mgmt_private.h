/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2010 
 * All rights reserved. Licensed material -- property of EMC 
 * Corporation 
 ***************************************************************************/

#ifndef DRIVE_MGMT_PRIVATE_H
#define DRIVE_MGMT_PRIVATE_H

/*!**************************************************************************
 * @file fbe_drive_mgmt_private.h
 ***************************************************************************
 *
 * @brief
 *  This file contains private defines for the Drive
 *  Management object. 
 * 
 * @ingroup drive_mgmt_class_files
 * 
 * @revision
 *   02/25/2010:  Created. Ashok Tamilarasan
 *
 ***************************************************************************/

#include "fbe_base_object.h"
#include "base_object_private.h"
#include "fbe_base_environment_private.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_esp_drive_mgmt.h"
#include "fbe/fbe_file.h"
//local
#include "fbe_drive_mgmt_dieh_xml.h"

#define FBE_DRIVE_MGMT_OS_DRIVE_RIDE_THROUGH_TIMEOUT  30 /* 30 seconds */

/* todo: is maxint already defined somewhere?*/
#define DMO_MAX_UINT_32  0xFFFFFFFFU

/* The following will control the rebuild conditional timers.  Since conditional timers are static,
 * the timers will wake up every interval seconds and check if total timers time has elapsed. 
 * The conditional timers can now be set dynamically for testing purposes.
 */
#define DMO_COND_TIMER_POLL_INTERVAL_SEC 6  

/* Defines for Drive Mgmt Persistent Storage
*/
#define DMO_USE_PERSISTENT_STORAGE  0
#define FBE_DRIVE_MGMT_PERSISTENT_STORAGE_SIZE   1024 /* 1k of data for now.*/

/* Defines for Drive Capacity checking
*/
#define FBE_DRIVE_MGMT_CLAR_ID_OFFSET 8
#define FBE_DRIVE_MGMT_CLAR_ID_LENGTH 8

/*------------------------------------------------------------------
 * SMART Log Page Definition
 *------------------------------------------------------------------*/
#define LOG_PAGE_0    0x0       /* Supported Log pages */
#define LOG_PAGE_2    0x2       /* Write Error Counters */
#define LOG_PAGE_3    0x3       /* Read Error Counters */
#define LOG_PAGE_5    0x5       /* Verify Error Counters */
#define LOG_PAGE_11   0x11      /* Solid State Media */
#define LOG_PAGE_15   0x15      /* BMS */
#define LOG_PAGE_2F   0x2f      /* Informational Exceptions */   
#define LOG_PAGE_30   0x30         /* log page 0x30 */
#define LOG_PAGE_31   0x31         /* log page 0x31 */
#define LOG_PAGE_34   0x34         /* log page 0x34 */
#define LOG_PAGE_37   0x37      /* Vendor Specific */
#define LOG_PAGE_NONE 0xff         /* undefine log page */

#define DMO_FUEL_GAUGE_MIN_POLL_INTERVAL  (3600*24*7) /* min 7 days */

/* Lifecycle definitions
 * Define the Drive management lifecycle's instance.
 */
extern fbe_lifecycle_const_t FBE_LIFECYCLE_CONST_DATA(drive_mgmt);

/* These are the lifecycle condition ids for Drive
   Management object.*/

/*! @enum fbe_drive_mgmt_lifecycle_cond_id_t  
 *  
 *  @brief These are the lifecycle conditions for the drive
 *  management object.
                                                               */
typedef enum fbe_drive_mgmt_lifecycle_cond_id_e 
{

    /*! Condition for delayed registration of drive events.*/
    FBE_DRIVE_MGMT_DELAYED_STATE_CHANGE_CALLBACK_REGISTER_COND  = FBE_LIFECYCLE_COND_FIRST_ID(FBE_CLASS_ID_DRIVE_MGMT),

    /*! Processing of new drive
     */
    FBE_DRIVE_MGMT_DISCOVER_DRIVE,

    /*! Condition for DIEH (Drive Improved Error Handling) */
    FBE_DRIVE_MGMT_DIEH_INIT_COND,


    /*! Conditions to get fuel gauge information periodically 
     */
    FBE_DRIVE_MGMT_FUEL_GAUGE_TIMER_COND,
    FBE_DRIVE_MGMT_FUEL_GAUGE_READ_COND,
    FBE_DRIVE_MGMT_FUEL_GAUGE_WRITE_COND,


    /*! Conditions to get drive paddlecard logs. 
     */
    FBE_DRIVE_MGMT_GET_DRIVE_LOG_COND,
    FBE_DRIVE_MGMT_WRITE_DRIVE_LOG_COND,

  
    FBE_DRIVE_MGMT_FUEL_GAUGE_UPDATE_PAGES_TBL_COND,

    FBE_DRIVE_MGMT_CHECK_OS_DRIVE_AVAIL_COND,

    FBE_DRIVE_MGMT_LIFECYCLE_COND_LAST /* must be last */
} 
fbe_drive_mgmt_lifecycle_cond_id_t;




/*!****************************************************************************
 *    
 * @struct fbe_drive_info_s
 *  
 * @brief 
 *   This is the definition of the drive info. This structure
 *   stores information about the drive
 ******************************************************************************/
typedef struct fbe_drive_info_s {
    fbe_object_id_t                         object_id;
    fbe_object_id_t                         parent_object_id;
    fbe_lifecycle_state_t                   state;  
    fbe_u32_t                               death_reason;    
    fbe_drive_type_t                        drive_type;
    fbe_u8_t                                vendor_id[FBE_SCSI_INQUIRY_VENDOR_ID_SIZE+1];
    fbe_u8_t                                product_id[FBE_SCSI_INQUIRY_PRODUCT_ID_SIZE + 1];
    fbe_u8_t                                tla[FBE_SCSI_INQUIRY_TLA_SIZE+1];
    fbe_u8_t                                rev[FBE_SCSI_INQUIRY_VPD_PROD_REV_SIZE+1];
    fbe_u8_t                                sn[FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE+1];
    fbe_lba_t                               gross_capacity;
    fbe_device_physical_location_t          location;
    fbe_time_t                              last_log;
    fbe_bool_t                              inserted;
    fbe_u8_t                                local_side_id;
    fbe_bool_t                              bypassed;
    fbe_bool_t                              self_bypassed;
    fbe_u8_t                                bridge_hw_rev[FBE_SCSI_INQUIRY_VPD_BRIDGE_HW_REV_SIZE + 1];
    fbe_lba_t                               block_count;
    fbe_block_size_t                        block_size;
    fbe_u32_t                               drive_qdepth;
    fbe_u32_t                               drive_RPM;    
    fbe_u8_t                                drive_description_buff[FBE_SCSI_DESCRIPTION_BUFF_SIZE+1];
    fbe_u8_t                                dg_part_number_ascii[FBE_SCSI_INQUIRY_PART_NUMBER_SIZE+1];
    fbe_physical_drive_speed_t              speed_capability;
    fbe_bool_t                              spin_down_qualified;
    fbe_u32_t                               spin_up_time_in_minutes;/*how long the drive was spinning*/
    fbe_u32_t                               stand_by_time_in_minutes;/*how long the drive was not spinning*/
    fbe_u32_t                               spin_up_count;/*how many times the drive spun up*/
    fbe_bool_t                              dbg_remove;
    fbe_bool_t                              poweredOff;
    fbe_block_transport_logical_state_t     logical_state; 
    fbe_base_object_mgmt_drv_dbg_action_t   driveDebugAction;
}fbe_drive_info_t;


/*!****************************************************************************
 *    
 * @struct fbe_drive_mgmt_drive_capacity_lookup_s
 *  
 * @brief 
 *   This is the definition of the drive capacity lookup table entry
 *   for known capacities.
 ******************************************************************************/
typedef struct fbe_drive_mgmt_drive_capacity_lookup_s
{
    fbe_u8_t *clar_id;               /* string to compare to inquiry data */
    fbe_lba_t gross_capacity;    /* gross capacity of drive*/
} fbe_drive_mgmt_drive_capacity_lookup_t;

/*!****************************************************************************
 *    
 * @struct fbe_drive_mgmt_fuel_gauge_info_s
 *  
 * @brief 
 *   This is the definition of the drive fuel gauge info.
 ******************************************************************************/
typedef struct fbe_drive_mgmt_fuel_gauge_info_s {
    fbe_bool_t                               auto_log_flag;
    fbe_bool_t                               is_initialized;
    fbe_u8_t                               * read_buffer;
    fbe_u8_t                               * write_buffer;
    fbe_object_id_t                          current_object;
    fbe_device_physical_location_t           current_location;
    fbe_drive_type_t                         currrent_drive_type;
    fbe_u8_t                                 current_vendor[FBE_SCSI_INQUIRY_VENDOR_ID_SIZE+1];
    fbe_u8_t                                 current_sn[FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE+1];
    fbe_u8_t                                 current_tla[FBE_SCSI_INQUIRY_TLA_SIZE+1];
    fbe_u8_t                                 current_fw_rev[FBE_SCSI_INQUIRY_VPD_PROD_REV_SIZE+1];
    fbe_bool_t                               need_drive_header;
    fbe_bool_t                               need_session_header;
    fbe_bool_t                               is_fbecli_cmd;
    fbe_u32_t                                current_index;
    fbe_file_handle_t                        file;  /* handle to file that keeps historical "accumulated" stats */
    fbe_file_handle_t                        file_most_recent;  /* handle to file that keeps most recent stats collection*/
    fbe_physical_drive_fuel_gauge_asynch_t * fuel_gauge_op;
    fbe_u8_t                                 current_supported_page_index;
    fbe_u64_t                                current_PE_cycles;
    fbe_u64_t                                power_on_hours;
    fbe_u64_t                                EOL_PE_cycles;
    fbe_atomic_t                             is_in_progrss;
    fbe_u32_t                                min_poll_interval; 
    fbe_atomic_t                             is_1st_time;
}fbe_drive_mgmt_fuel_gauge_info_t;

/* Drive Statistics Pages variant Id */
typedef enum fbe_drive_mgmt_fuel_gauge_variant_id_e
{
    UNUSED_VARIANT_ID,
    SMART_DATA,             /* for SATA flash */
    FUEL_GAUGE_DATA,        /* for SAS flash */  
    BMS_DATA,               /* for HDD drives */  
    BMS_DATA1,              /* for HDD drives with two extra values (total_unique_03_errors and total_unique_01_errors)in output then it in BMA_DATA.*/  
    WRITE_ERROR_COUNTERS,   /* log page 0x2 */
    READ_ERROR_COUNTERS,    /* log page 0x3 */    
    VERIFY_ERROR_COUNTERS,  /* log page 0x5 */
    SSD_MEDIA,              /* log page 0x11 */
    INFO_EXCEPTIONS,        /* log page 0x2f */
    VENDOR_SPECIFIC_SMART,  /* log page 0x37*/

} fbe_drive_mgmt_fuel_gauge_variant_id_t;
          
/* Drive Statistics Tool Identifier */
typedef enum fbe_drive_mgmt_fuel_gauge_tool_id_e
{
    UNUSED_TOOL_ID,
    FUEL_GAUGE_TOOL,          /* Only use this one for now */
    BACKGROUND_MEDIA_SCAN_TOOL
} fbe_drive_mgmt_fuel_gauge_tool_id_t;

/* TOD_DATA without 4 digit year, day_of_week, and hundredth_second */
typedef struct fbe_drive_mgmt_fuel_gauge_time_data_s
{
    fbe_u8_t year;
    fbe_u8_t month;
    fbe_u8_t day;
    fbe_u8_t hour;
    fbe_u8_t minute;
    fbe_u8_t second;
} fbe_drive_mgmt_fuel_gauge_time_data_t;

/*
 * This structure is the header of each session (1 per 8 days or 1 hour after system is rebooted.
 */
typedef struct fbe_drive_mgmt_fuel_gauge_session_header_s
{
    fbe_u8_t                 magic_word[4];    /* DCS1 */
    fbe_u8_t                 session_header_len_high;
    fbe_u8_t                 session_header_len_low;
    fbe_u8_t                 tool_id;          /* Tool identifier 1 = Fuel Gauge Tool */
    fbe_u8_t                 file_fmt_rev_major;
    fbe_u8_t                 file_fmt_rev_minor;
    fbe_u8_t                 tool_rev_major;
    fbe_u8_t                 tool_rev_minor;
    fbe_drive_mgmt_fuel_gauge_time_data_t time_stamp;
} fbe_drive_mgmt_fuel_gauge_session_header_t;

/*
 * This structure is the header of each drive.
 */
typedef struct fbe_drive_mgmt_fuel_gauge_drive_header_s
{
    fbe_u8_t                 magic_word[4];    /* DRV1 */
    fbe_u8_t                 drive_header_len_high;   
    fbe_u8_t                 drive_header_len_low;
    fbe_u8_t                 object_id[4];
    fbe_u8_t                 bus_num;
    fbe_u8_t                 enclosure_num;
    fbe_u8_t                 slot_num;
    fbe_u8_t                 serial_num[FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE+1];
    fbe_u8_t                 TLA_num[FBE_SCSI_INQUIRY_TLA_SIZE+1];
    fbe_u8_t                 FW_rev[FBE_SCSI_INQUIRY_VPD_PROD_REV_SIZE+1];
    fbe_drive_mgmt_fuel_gauge_time_data_t time_stamp;
    fbe_u8_t                 data_magic_word[4];    /* DATA */
    fbe_u8_t                 page_size_high;        /* page size length of log page 0x30*/
    fbe_u8_t                 page_size_low; 
    fbe_u8_t                 data_variant_id;
} fbe_drive_mgmt_fuel_gauge_drive_header_t;

/*
 * This structure is the header of each log page.
 */
typedef struct fbe_drive_mgmt_fuel_gauge_data_header_s
{
    fbe_u8_t                 data_magic_word[4];    /* DATA */
    fbe_u8_t                 page_size_high;        /* page size length of log page 0x30*/
    fbe_u8_t                 page_size_low; 
    fbe_u8_t                 data_variant_id;
} fbe_drive_mgmt_fuel_gauge_data_header_t;

/* Log page header */
typedef struct fbe_drive_mgmt_fuel_gauge_page_header_s
{
    fbe_u8_t page_code;
    fbe_u8_t subpage_code;
    fbe_u8_t page_length_high;
    fbe_u8_t page_length_low;
}fbe_drive_mgmt_fuel_gauge_page_header_t;

/*Structure used for the list of supported pages */
typedef struct fbe_drive_mgmt_fuel_gauge_supported_pages_s
{
    fbe_u8_t page_code;
    fbe_bool_t supported;

}fbe_drive_mgmt_fuel_gauge_supported_pages_t;

/*Structure used for the list of Max. PE cycles */
typedef struct fbe_drive_mgmt_fuel_gauge_max_PE_cycles_s
{
    fbe_u8_t        vendor_id[FBE_SCSI_INQUIRY_VENDOR_ID_SIZE+1];
    fbe_u64_t       max_PE_cycles;

}fbe_drive_mgmt_fuel_gauge_max_PE_cycles_t;


/* START OF SCSI LOG PAGE PARAMETER DEFINITIONS */

/* Log Page 0x15 Background Scan Results: Parameter 0000: BG Scan Status: */
#pragma pack(1)
typedef struct fbe_dmo_fuel_gauge_log_param_bg_scan_status_s
{
    fbe_u16_t   param_code;
    fbe_u8_t    flags;
    fbe_u8_t    length;
    fbe_u32_t   power_on_minutes;
    fbe_u8_t    reserved;
    fbe_u8_t    bg_scan_status;
    fbe_u16_t   num_bg_scans_performed;
    fbe_u16_t   bg_scan_progress;
    fbe_u16_t   num_bg_medium_scans_performed;
}fbe_dmo_fuel_gauge_log_param_bg_scan_status_t;
#pragma pack()

/* Seagate vendor specific area of BG Scan Result Log Paramter 0001-0800 */
#pragma pack(1)
typedef struct fbe_dmo_fuel_gauge_log_param_bg_scan_vs_SEAGATE_s
{
    fbe_u8_t  head_cyl_high;    /* bits 7-4 are head. bits 3-0 are high order bits of cylinder */
    fbe_u16_t head_cyl_low;         /* low order bits of cylinder */
    fbe_u8_t  asi_sector_high;  /* bits 7-6 are Add-Scan-Info.  Bits 5-0 are high order bits of sector. */
    fbe_u8_t  asi_sector_low;       /* low order bits of sector */
}fbe_dmo_fuel_gauge_log_param_bg_scan_vs_SEAGATE_t;
#pragma pack()

/* Log Page 0x15 Background Scan Results: Parameter 0001 throguh 0800: BG Scan: */
#pragma pack(1)
typedef struct fbe_dmo_fuel_gauge_log_param_bg_scan_s
{
    fbe_u16_t    param_code;
    fbe_u8_t     flags;
    fbe_u8_t     length;
    fbe_u32_t    power_on_minutes;
    fbe_u8_t     reassign_status_sense_key;
    fbe_u8_t     asc;
    fbe_u8_t     ascq;
    union
    {
        fbe_u8_t bytes[5];
        fbe_dmo_fuel_gauge_log_param_bg_scan_vs_SEAGATE_t seagate;
    } vs; /* vendor specific area */
    fbe_u64_t   lba;
}fbe_dmo_fuel_gauge_log_param_bg_scan_t;
#pragma pack()

/* Log Page 0x15 Parameter 8100 throguh 8132: Seagate IRAW: Idle Read after Write Test Results: */
#pragma pack(1)
typedef struct fbe_dmo_fuel_gauge_log_param_SEAGATE_IRAW_s
{
    fbe_u16_t   param_code;
    fbe_u8_t    flags;
    fbe_u8_t    length;
    fbe_u32_t   power_on_minutes;
    fbe_u32_t   start_lba;
    fbe_u32_t   failed_lba;
    fbe_u16_t   xfer_count;
    fbe_u8_t    activity_action;  /* bits 7-6: Activity; bits 5-0: Action */
    fbe_u8_t    temperature;      /* temp in degrees C */
    fbe_u8_t    errcode_sense_key;
    fbe_u8_t    asc;
    fbe_u8_t    ascq;
    fbe_u8_t    fru;
}fbe_dmo_fuel_gauge_log_param_SEAGATE_IRAW_t;
#pragma pack()

/* physical drive BMS Scan State   */
#pragma pack(1)
typedef struct fbe_dmo_fuel_gauge_bms_state_s
{
    fbe_u32_t   last_scan_timestamp;    /* timestamp of last BMS scan */
    fbe_u32_t   error_timestamp;        /* timestamp of last trace error */
    fbe_u32_t   pad1;
    fbe_u8_t    state;                  /* Current BMS state : OK, FAILING, or CRITICAL */
    fbe_u8_t    pad2[3];
} fbe_dmo_fuel_gauge_bms_state_t;
#pragma pack()

/* physical drive BMS Scan Summary */
#pragma pack(1)
typedef struct fbe_dmo_fuel_gauge_bms_s
{
    fbe_drive_mgmt_fuel_gauge_time_data_t time_stamp;
    fbe_u16_t   num_bg_scans_performed;
    fbe_u16_t   total_entries;
    fbe_u8_t    total_iraw;
    fbe_u8_t    worst_head;
    fbe_u16_t   worst_head_entries;
    fbe_u16_t   total_03_errors;
    fbe_u16_t   total_01_errors;
    fbe_u32_t   time_range;
    fbe_u32_t   drive_power_on_minutes; 
    fbe_u16_t   total_unique_03_errors; 
    fbe_u16_t   total_unique_01_errors;
} fbe_dmo_fuel_gauge_bms_t;
#pragma pack()

typedef union
{
    fbe_dmo_fuel_gauge_log_param_bg_scan_status_t scan_status;
    fbe_dmo_fuel_gauge_log_param_bg_scan_t        scan;
    fbe_dmo_fuel_gauge_log_param_SEAGATE_IRAW_t   iraw;
} fbe_dmo_fuel_gauge_log_param_bg_scan_param_t;

/* Log page parameters header */
typedef struct fbe_drive_mgmt_fuel_gauge_log_parm_header_s
{
    fbe_u16_t   param_code;
    fbe_u8_t    flags;
    fbe_u8_t    length;
}fbe_drive_mgmt_fuel_gauge_log_parm_header_t;

/* these thresholds are based on the Seagate HURRICANE disk drive */
#define DMO_FUEL_GAUGE_BMS_MAX_NUMBER_OF_ENTRIES               (2048)
#define DMO_FUEL_GAUGE_BMS_FAILING_HEAD_THRESHOLD              (1000)
#define DMO_SCSI_SENSE_KEY_MASK                                0x0f
#define DMO_SCSI_SENSE_KEY_MEDIUM_ERROR                        0x03
#define DMO_SCSI_SENSE_KEY_RECOVERED_ERROR                     0x01

/* END OF SCSI LOG PAGE PARAMETER DEFINITIONS */


/*!****************************************************************************
 *    
 * @struct fbe_drive_mgmt_drive_log_info_s
 *  
 * @brief 
 *   This is the definition of the drive log info. 
 ******************************************************************************/
typedef struct fbe_drive_mgmt_drive_log_info_s {
    fbe_atomic_t                             in_progress;
    fbe_u8_t                               * log_buffer;
    fbe_object_id_t                          object_id;
    fbe_device_physical_location_t           drive_location;
    fbe_u8_t                                 drive_sn[FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE+1];
    fbe_physical_drive_get_smart_dump_asynch_t * drivegetlog_op;

}fbe_drive_mgmt_drive_log_info_t;


/*!****************************************************************************
 *    
 * @struct fbe_drive_mgmt_s
 *  
 * @brief 
 *   This is the definition of the drive mgmt object. This object
 *   deals with handling drive related functions
 ******************************************************************************/
typedef struct fbe_drive_mgmt_s {
    /*! This must be first.  This is the base object we inherit from.
     */
    fbe_base_environment_t              base_environment;
    fbe_drive_info_t *                  drive_info_array;      // head of array
    fbe_u32_t                           max_supported_drives;  // num elements in array
    fbe_bool_t                          osDriveAvail;
    fbe_time_t                          startTimeForOsDriveUnavail;
    fbe_u32_t                           osDriveUnavailRideThroughInSec;
    fbe_spinlock_t                      drive_info_lock;
    fbe_u32_t                           total_drive_count;    //TODO: not used anywhere and not updated on new drives
    fbe_drive_mgmt_fuel_gauge_info_t    fuel_gauge_info;
    fbe_spinlock_t                      fuel_gauge_info_lock;
    fbe_drive_mgmt_drive_log_info_t     drive_log_info;

    fbe_bool_t                          primary_os_drive_policy_check_failed;
    fbe_bool_t                          secondary_os_drive_policy_check_failed;

    fbe_pdo_logical_error_action_t      logical_error_actions;

    /*dieh */
    fbe_dmo_thrsh_parser_t              * xml_parser;

    fbe_u32_t                           system_drive_normal_queue_depth;
    fbe_u32_t                           system_drive_reduced_queue_depth;
    fbe_u32_t                           system_drive_queue_depth_reduce_count;

    fbe_esp_dmo_driveconfig_xml_info_t                  drive_config_xml_info;

    /* Lifecycle defines. */
    FBE_LIFECYCLE_DEF_INST_DATA;
    FBE_LIFECYCLE_DEF_INST_COND(FBE_LIFECYCLE_COND_MAX(FBE_DRIVE_MGMT_LIFECYCLE_COND_LAST));
} fbe_drive_mgmt_t;


/*--- Private Inlines -------------------------------------------------------------*/

//TODO: make this macro so caller doesn't have to pass function in.
__forceinline static void
dmo_drive_array_lock(fbe_drive_mgmt_t *drive_mgmt, const char* dbgstr)
{
    fbe_spinlock_lock(&drive_mgmt->drive_info_lock);

    /*
    fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                      FBE_TRACE_LEVEL_DEBUG_HIGH,
                      FBE_TRACE_MESSAGE_ID_INFO,
                      "%s drive array locked\n",
                      dbgstr);
    */
}

__forceinline static void
dmo_drive_array_unlock(fbe_drive_mgmt_t *drive_mgmt, const char* dbgstr)
{
    fbe_spinlock_unlock(&drive_mgmt->drive_info_lock);

    /*
    fbe_base_object_trace((fbe_base_object_t *)drive_mgmt, 
                      FBE_TRACE_LEVEL_DEBUG_HIGH,
                      FBE_TRACE_MESSAGE_ID_INFO,
                      "%s drive array unlocked\n",
                      dbgstr);
                      */
}


/********************* CMI MESSAGE TYPES **********************/

/* NOTE: All cmi msgs should have compile time asserts to ensure size
 *       does not exceed maximum buffer allocation.  These asserts
 *       are done in fbe_drive_mgmt_load
 */ 


/*!****************************************************************************
 *    
 * @struct dmo_cmi_msg_type_e
 *  
 * @brief 
 *   This is the definition of the enumeration of drive mgmt object's
 *   CMI (Peer-to-Peer) messages.
 ******************************************************************************/
typedef enum dmo_cmi_msg_type_e{
    DMO_CMI_MSG_INVALID,
    /* If CMI is needed, add msgs here */
}dmo_cmi_msg_type_t;

#define FBE_CMI_MSG_CLASS_SHIFT 16
#define FBE_CMI_MSG_FIRST_ID(class_id) ((class_id) << (FBE_CMI_MSG_CLASS_SHIFT))

typedef enum fbe_drive_mgmt_cmi_msg_type_e{
    FBE_DRIVE_MGMT_CMI_MSG_INVALID = FBE_CMI_MSG_FIRST_ID(FBE_CLASS_ID_DRIVE_MGMT),
    /* If CMI is needed, add here*/
    FBE_DRIVE_MGMT_CMI_MSG_DEBUG_REMOVE,    /* cru off */
    FBE_DRIVE_MGMT_CMI_MSG_DEBUG_INSERT,    /* cru on */
    FBE_DRIVE_MGMT_CMI_MSG_LAST
}fbe_drive_mgmt_cmi_msg_type_t;

/*!****************************************************************************
 *    
 * @struct dmo_cmi_std_info_s
 *  
 * @brief 
 *   This is the definition of the drive mgmt object info structure used for
 *   CMI (Peer-to-Peer) message communication.
 ******************************************************************************/
typedef struct dmo_cmi_std_info_s
{
    fbe_u8_t bus, enclosure, slot;

} dmo_cmi_std_info_t;

/*!****************************************************************************
 *    
 * @struct dmo_cmi_msg_base_s
 *  
 * @brief 
 *   This is the definition of the drive mgmt object base structure used for
 *   CMI (Peer-to-Peer) message communication.  All other messages are
 *   derived from this.
 ******************************************************************************/
typedef struct dmo_cmi_msg_base_s
{
    dmo_cmi_msg_type_t      type;

} dmo_cmi_msg_base_t;

/*!****************************************************************************
 *    
 * @struct dmo_cmi_msg_standard_s
 *  
 * @brief 
 *   This is the definition of the drive mgmt object standard structure used for
 *   most CMI (Peer-to-Peer) message communication.
 ******************************************************************************/
typedef struct dmo_cmi_msg_standard_s 
{
    dmo_cmi_msg_base_t  base;       // base must be first element
    dmo_cmi_std_info_t  info;

} dmo_cmi_msg_standard_t;

/*!****************************************************************************
 *    
 * @struct dmo_cmi_msg_start_s
 *  
 * @brief 
 *   This is the definition of the drive mgmt object start structure used for
 *   CMI (Peer-to-Peer) message communication.
 ******************************************************************************/
typedef struct dmo_cmi_msg_start_s 
{
    dmo_cmi_msg_standard_t  std;       // base must be first element
    fbe_physical_drive_fwdl_start_request_t  info;

} dmo_cmi_msg_start_t;


typedef struct fbe_drive_mgmt_cmi_info_s 
{
    fbe_u8_t                              bus;
    fbe_u8_t                              encl;
    fbe_u8_t                              slot;
    fbe_bool_t                            peer_rebuilding;
} fbe_drive_mgmt_cmi_info_t;

typedef struct fbe_drive_mgmt_cmi_msg_s 
{
    fbe_drive_mgmt_cmi_msg_type_t         type;
    fbe_drive_mgmt_cmi_info_t             info;
} fbe_drive_mgmt_cmi_msg_t;

typedef enum dmo_drive_death_action_e
{
    DMO_DRIVE_DEATH_ACTION_REPLACE_DRIVE,
    DMO_DRIVE_DEATH_ACTION_REINSERT_DRIVE,
    DMO_DRIVE_DEATH_ACTION_UPGRADE_FW,
    DMO_DRIVE_DEATH_ACTION_CONTACT_SERVICE_PROVIDER,
    DMO_DRIVE_DEATH_ACTION_RELOCATE,
    DMO_DRIVE_DEATH_ACTION_PLATFORM_LIMIT_EXCEEDED,
} dmo_drive_death_action_t;

/*--- Function Prototypes -------------------------------------------------------------*/

/* main*/
fbe_status_t fbe_drive_mgmt_create_object(fbe_packet_t * packet, fbe_object_handle_t * object_handle);
fbe_status_t fbe_drive_mgmt_destroy_object( fbe_object_handle_t object_handle);
fbe_status_t fbe_drive_mgmt_control_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet);
fbe_status_t fbe_drive_mgmt_monitor_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet);
fbe_status_t fbe_drive_mgmt_event_entry(fbe_object_handle_t object_handle, fbe_event_type_t event_type, fbe_event_context_t event_context);

fbe_status_t fbe_drive_mgmt_monitor_load_verify(void);
fbe_status_t fbe_drive_mgmt_init(fbe_drive_mgmt_t * drive_mgmt);
void         fbe_drive_mgmt_init_special(fbe_drive_mgmt_t * drive_mgmt); 

fbe_bool_t   fbe_drive_mgmt_policy_enabled(fbe_drive_mgmt_policy_id_t policy_id);
void         fbe_drive_mgmt_init_drive(fbe_drive_info_t *drive);
void         fbe_drive_mgmt_set_all_pdo_crc_actions(fbe_drive_mgmt_t *drive_mgmt, fbe_pdo_logical_error_action_t action);

fbe_status_t fbe_drive_mgmt_get_drive_phy_location_by_objectId(fbe_drive_mgmt_t  *drive_mgmt,
                                                               fbe_object_id_t  object_id,
                                                               fbe_device_physical_location_t *phys_location);
fbe_status_t fbe_drive_mgmt_get_drive_info_by_location(fbe_drive_mgmt_t *drive_mgmt,
                                          fbe_device_physical_location_t * pLocation,
                                          fbe_drive_info_t **pDriveInfoPtr);
fbe_status_t fbe_drive_mgmt_get_available_drive_info_entry(fbe_drive_mgmt_t *pDriveMgmt, 
                                                           fbe_drive_info_t **ppDriveInfo);
fbe_status_t fbe_drive_mgmt_get_drive_info_pointer(fbe_drive_mgmt_t *pDriveMgmt,
                                                   fbe_device_physical_location_t *pLocation,
                                                   fbe_drive_info_t **ppDriveInfoPtr);


/* utils */
fbe_u32_t           fbe_drive_mgmt_bes_to_fru_num(fbe_u32_t bus, fbe_u32_t enclosure, fbe_u32_t slot); 
fbe_status_t        fbe_drive_mgmt_send_cmi_message(const fbe_drive_mgmt_t * drive_mgmt, fbe_u8_t bus, fbe_u8_t encl, fbe_u8_t slot, fbe_drive_mgmt_cmi_msg_type_t msg_type);
void                fbe_drive_mgmt_process_cmi_message(fbe_drive_mgmt_t * drive_mgmt, fbe_drive_mgmt_cmi_msg_t * base_msg);
fbe_status_t        fbe_drive_mgmt_process_cmi_peer_not_present(fbe_drive_mgmt_t * drive_mgmt, fbe_drive_mgmt_cmi_msg_t * base_msg);
char*               fbe_drive_mgmt_death_reason_desc(fbe_u32_t death_reason);
fbe_u32_t           fbe_drive_mgmt_get_logical_offline_reason(fbe_drive_mgmt_t *drive_mgmt, fbe_block_transport_logical_state_t logical_state);
dmo_drive_death_action_t  fbe_drive_mgmt_drive_failed_action(const fbe_drive_mgmt_t *drive_mgmt, fbe_u32_t death_reason);
void fbe_drive_mgmt_log_drive_offline_event(const fbe_drive_mgmt_t *drive_mgmt, 
                                            dmo_drive_death_action_t action,
                                            fbe_physical_drive_information_t * drive_info,
                                            fbe_u32_t reason,
                                            fbe_device_physical_location_t * phys_location);
fbe_u32_t           dmo_str2int(const char *src);
void                fbe_drive_mgmt_get_logical_error_action(fbe_drive_mgmt_t * drive_mgmt, fbe_pdo_logical_error_action_t * actions);
void                dmo_physical_drive_information_init(fbe_physical_drive_information_t *p_drive_info);
void                fbe_drive_mgmt_get_fuel_gauge_poll_interval_from_registry(fbe_drive_mgmt_t * drive_mgmt, fbe_u32_t * fuel_gauge_min_poll_interval);
void                fbe_drive_mgmt_fuel_gauge_get_PE_cycle_warrantee_info(fbe_drive_mgmt_t *drive_mgmt);
fbe_u64_t           fbe_drive_mgmt_cvt_value_to_ull(fbe_u8_t *num, fbe_bool_t swap, fbe_s32_t numbytes);
fbe_u16_t           fbe_drive_mgmt_find_duplicate_bms_lba(fbe_lba_t *failed_lba, fbe_lba_t current_failed_lba, fbe_u16_t total_errors);


/* For TESTING ONLY */
void fbe_drive_mgmt_inject_clar_id_for_simulated_drive_only(fbe_drive_mgmt_t *drive_mgmt, fbe_physical_drive_information_t *physical_drive_info);

/* fuel gauge */
fbe_status_t        fbe_drive_mgmt_fuel_gauge_get_eligible_drive(fbe_drive_mgmt_t *drive_mgmt);
fbe_status_t        fbe_drive_mgmt_fuel_gauge_init(fbe_drive_mgmt_t *drive_mgmt);
void                fbe_drive_mgmt_fuel_gauge_cleanup(fbe_drive_mgmt_t *drive_mgmt);
fbe_status_t        fbe_drive_mgmt_fuel_gauge_write_to_file(fbe_drive_mgmt_t *drive_mgmt);
void                fbe_drive_mgmt_fuel_gauge_fill_params(fbe_drive_mgmt_t *drive_mgmt, fbe_physical_drive_fuel_gauge_asynch_t * fuel_gauge_op, fbe_u8_t page_code);
fbe_u8_t            fbe_drive_mgmt_fuel_gauge_get_data_variant_id(fbe_u8_t  page_code);
void                fbe_drive_mgmt_fuel_gauge_check_EOL_warrantee(fbe_drive_mgmt_t *drive_mgmt);
void fbe_drive_mgmt_fuel_gauge_find_valid_log_page_code(UINT_8 *byte_read_ptr, fbe_physical_drive_fuel_gauge_asynch_t * fuel_gauge_op);
void fbe_drive_mgmt_fuel_gauge_check_BMS_alerts(fbe_drive_mgmt_t *drive_mgmt, fbe_dmo_fuel_gauge_bms_t *bms_results);
void fbe_drive_mgmt_fuel_gauge_fill_write_buffer(fbe_drive_mgmt_t *drive_mgmt, fbe_u32_t * write_count);

/* drivegetlog */
fbe_status_t fbe_drive_mgmt_handle_drivegetlog_request(fbe_drive_mgmt_t *drive_mgmt, fbe_device_physical_location_t *location);
fbe_status_t fbe_drive_mgmt_get_drive_log(fbe_drive_mgmt_t *drive_mgmt);
fbe_status_t fbe_drive_mgmt_write_drive_log(fbe_drive_mgmt_t *drive_mgmt);
void fbe_drive_mgmt_drive_log_cleanup(fbe_drive_mgmt_t *drive_mgmt);

/* DIEH */ 
fbe_dieh_load_status_t fbe_drive_mgmt_dieh_init(fbe_drive_mgmt_t *drive_mgmt, fbe_u8_t * file);
void fbe_drive_mgmt_dieh_get_path(fbe_u8_t * dir, fbe_u32_t buff_size);

/* DIEH XML Parser */
dieh_xml_error_info_t  fbe_drive_mgmt_dieh_xml_parse( fbe_drive_mgmt_t * drive_mgmt,
                                                      char * filename,
                                                      XML_StartElementHandler start_handler,
                                                      XML_EndElementHandler end_handler,
                                                      XML_CharacterDataHandler data_handler );
void fbe_dmo_thrsh_xml_start_element( void *user_data, const char *element, const char **attributes);
void fbe_dmo_thrsh_xml_data_element(void* data, const char* el, int len);
void fbe_dmo_thrsh_xml_end_element(void *user_data, const char* element);

fbe_status_t fbe_drive_mgmt_is_system_drive(fbe_drive_mgmt_t * pDriveMgmt, 
                                        fbe_device_physical_location_t * pDrivePhysicalLocation,
                                        fbe_bool_t * is_system_drive);


extern fbe_drive_mgmt_policy_t  fbe_drive_mgmt_policy_table[];
extern fbe_spinlock_t           fbe_drive_mgmt_policy_table_lock;



/* Not Thread Safe.  Caller's responsibility to guard the drive array.
*/
static __forceinline fbe_drive_info_t * 
fbe_drive_mgmt_get_drive_p(fbe_drive_mgmt_t *drive_mgmt, fbe_object_id_t object_id)
{
    fbe_u32_t i;
    fbe_drive_info_t *drive = drive_mgmt->drive_info_array;

    for (i=0; i < drive_mgmt->max_supported_drives; i++)
    {
        if (drive->object_id == object_id)
        {
            return drive;
        }
        drive++;
    }

    return NULL;   // not found
}

static __forceinline fbe_drive_info_t* 
fbe_drive_mgmt_bes_to_drive_p(const fbe_drive_mgmt_t *drive_mgmt, const fbe_drive_info_t * drive_array, fbe_u8_t bus, fbe_u8_t encl, fbe_u8_t slot)
{
    /* Thread Safety Notes:  This is not thread safe.  It's the caller's responsibility to guard drive_info
     */
    fbe_u32_t i;
    fbe_drive_info_t *drive = drive_mgmt->drive_info_array;

    if (drive->object_id != FBE_OBJECT_ID_INVALID)
    {    
        for (i=0; i < drive_mgmt->max_supported_drives; i++)
        {
            if (drive->location.bus == bus &&
                drive->location.enclosure == encl &&
                drive->location.slot == slot )
            {
                return drive;
            }
            drive++;
        }
    }

    return NULL;   // not found
}


/* Temporary work around for persistence. */
fbe_status_t fbe_drive_mgmt_get_esp_lun_location(fbe_char_t *location);

fbe_status_t fbe_drive_mgmt_panic_sp(fbe_bool_t force_degraded_mode);

#endif /* DRIVE_MGMT_PRIVATE_H */

/*******************************
 * end fbe_drive_mgmt_private.h
 *******************************/
