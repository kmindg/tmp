#ifndef ADM_SP_API_H
#define ADM_SP_API_H 0x0000001
#define FLARE__STAR__H
/***************************************************************************
 * Copyright (C) EMC Corporation 2007 - 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *  adm_sp_api.h
 ***************************************************************************
 *
 *  Description
 *
 *      This file contains the external interface to the SP-related
 *      interfaces of the Admin Interface "adm" module in the Flare driver.
 *
 *      This file is included by adm_api.h, the top-level interface for
 *      the ADM module.
 *
 *  History:
 *
 *      06/15/01 CJH    Created
 *      10/04/01 CJH    Add read cache hit ratio, remove SYSCONFIG dups
 *      11/01/01 HEW    Added ifdefs for C++ compilation
 *      04/17/02 HEW    Added CALL_TYPE to decl for dyn_peer_sp_present()
 *      04/10/03 HEW    DIMS 85939: Added dyn_peer_write_cache_state()
 *      05/15/07 MK     Added dyn_write_cache_max_cache_limit()
 ***************************************************************************/

// #include "core_config.h"
// #include "k10defs.h"
// #include "sunburst.h"

#include "cm_environ_exports.h"
#include "specl_types.h"
#include "spid_types.h"

/************
 * Literals
 ************/
#define ADM_PORT_NOT_FOUND          0xffffffff
/*
 * Maximum number of SubFRU Replacements
 *
 * This number specifies the maximum number of Field Replaceable Units (FRUs),
 * Customer Replace Units (CRUs) in Bigfoot, that can be replacement at one
 * point in time because of a failure.
 * Some of the replaceable units: SP board, DIMM 0 & 1, I/O Module, 
 *  Power Supply, Fan Pack, Interposer, SFP 0 & 1
 */
#define ADM_MAX_SUBFRU_REPLACEMENTS 4

/************
 *  Types
 ************/

// These are status enums for communicating back with navi during online firmware download.
typedef enum 
{
    OFD_IDLE=0,                     // This should be initialized to 0 when the SP boots.
    OFD_RUNNING,
    OFD_FAILED,
    OFD_ABORTED,
    OFD_SUCCESSFUL
} 
OFD_SP_STAT;   

typedef enum
{
    OFD_NO_FAILURE=0,
    OFD_FAIL_NONQUALIFIED,          // One or more nonqualified drives exist in the array 
                                    //    and the FDF_CFLAGS_FAIL_NONQUALIFIED bit is set.
    OFD_FAIL_PROBATION,             // A drive failed to transition probational mode.                              
    OFD_FAIL_NO_DRIVES_TO_UPGRADE,  // There are no drives that can be upgraded.    
    OFD_FAIL_FW_REV_MISMATCH,       // After finishing upgrade, a drive did not have the expected FW rev.
    OFD_FAIL_WRITE_BUFFER,          // DH returned bad status from a write buffer command sent to a drive.
    OFD_FAIL_CREATE_LOG,            // An error ocurred that inhibits creating a rebuild log.        
    OFD_FAIL_BRICK_DRIVE,           // A drive failed to respond after sending the download.
    OFD_FAIL_DRIVE,                 // A drive failed sometime during the FW download.
    OFD_FAIL_ABORTED,               // The process was aborted by Navi.
    OFD_FAIL_MISSING_TLA            // The FDF file is missing the FDF SERVICEABILITY BLOCK, and TLA #. 
}
OFD_SP_FAIL;

typedef enum
{
    OFD_OP_NOP=0,                   // No operation at the moment, initialized at boot.
    OFD_OP_TRIAL_RUN,               // Trial run that reports back drives suitable for upgrade only.
    OFD_OP_UPGRADE                  // We're doing an upgrade to the disks now
}
OFD_OPERATION;

#ifndef ALAMOSA_WINDOWS_ENV
#define ADM_SP_A SP_A
#define ADM_SP_B SP_B
#define adm_sp_id SP_ID_TAG
#define ADM_SP_ID SP_ID
#else
typedef enum adm_sp_id
{
    ADM_SP_A,
    ADM_SP_B
}
ADM_SP_ID;
#endif /* ALAMOSA_WINDOWS_ENV - ODDCASE - enum abuse compiler issue */

/*******************
 * SP subFRU related
 *******************/
typedef enum adm_subfru_type
{
    ADM_CPU_TYPE    = 0,
    ADM_IO_TYPE,
    ADM_MANAGEMENT_TYPE,
    ADM_DIMM_TYPE,
    ADM_BEM_TYPE,
    ADM_SUBFRU_MEZZANINE_TYPE,
    ADM_MAX_TYPES
}
ADM_SUBFRU_TYPE;


typedef enum adm_subfru_type_slot
{
    ADM_TYPE_SLOT_0,
    ADM_TYPE_SLOT_1,
    ADM_TYPE_SLOT_2,
    ADM_TYPE_SLOT_3,
    ADM_TYPE_SLOT_4,
    ADM_TYPE_SLOT_5
}
ADM_SUBFRU_TYPE_SLOT;

/**********************************************************************
 * ADM_PEER_BOOT_STATE enum
 * Indicates the peer boot/fault status.
 **********************************************************************/
typedef enum adm_peer_boot_status_enum
{
    ADM_PEER_UNKNOWN,       // unknown peer state
    ADM_PEER_DONE,          // booting is complete
    ADM_PEER_DEGRADED,      // booted to degraded
    ADM_PEER_BOOTING,       // boot in process
    ADM_PEER_FAILED,        // post or bios test failed
    ADM_PEER_HUNG,          // hung during boot
    ADM_PEER_DUMPING,       // a dump is occurring
    ADM_PEER_SW_LOADING,    // drivers/services are loading
    ADM_PEER_OK,
    ADM_PEER_REMOVED,
    ADM_PEER_POWERED_OFF,
    ADM_PEER_INVALID    = -1
}
ADM_PEER_BOOT_STATE;


/*********************************************************************
 * ADM_PEER_BOOT_CODE enum
 * Indicates the most recent stage the peer entered.
 **********************************************************************/
typedef enum adm_peer_boot_code_enum
{
    ADM_BOOT_FAULT_CPU          = 0,
    ADM_BOOT_FAULT_IO_0,        // 0x01
    ADM_BOOT_FAULT_IO_1,        // 0x02
    ADM_BOOT_FAULT_ANNEX,       // 0x03
    ADM_BOOT_FAULT_CPU_IO_0,    // 0x04
    ADM_BOOT_FAULT_CPU_IO_1,    // 0x05
    ADM_BOOT_FAULT_CPU_ANNEX,   // 0x06
    ADM_BOOT_FAULT_CPU_DIMMS,   // 0x07
    ADM_BOOT_FAULT_DIMMS,       // 0x08
    ADM_BOOT_FAULT_DIMM_0,      // 0x09
    ADM_BOOT_FAULT_DIMM_1,      // 0x0A
    ADM_BOOT_FAULT_DIMM_2,      // 0x0B
    ADM_BOOT_FAULT_DIMM_3,      // 0x0C
    ADM_BOOT_FAULT_DIMM_4,      // 0x0D
    ADM_BOOT_FAULT_DIMM_5,      // 0x0E
    ADM_BOOT_FAULT_DIMM_6,      // 0x0F
    ADM_BOOT_FAULT_DIMM_7,      // 0x10
    ADM_BOOT_FAULT_DIMM_0_1,    // 0x11
    ADM_BOOT_FAULT_DIMM_2_3,    // 0x12
    ADM_BOOT_FAULT_DIMM_4_5,    // 0x13
    ADM_BOOT_FAULT_DIMM_6_7,    // 0x14
    ADM_BOOT_FAULT_VRM_0,       // 0x15
    ADM_BOOT_FAULT_VRM_1,       // 0x16
    ADM_BOOT_MANAGEMENT,        // 0x17
    ADM_BOOT_FAULT_ANNEX_MIDFUSE,   // 0x18
    ADM_BOOT_FAULT_CANT_ISOLATE,    // 0x19
    ADM_BOOT_FAULT_ENCLOSURE,       // 0x1A
    ADM_BOOT_FAULT_SFP,             // 0x1B
    ADM_BOOT_DEGRADED           = 0x25,
    ADM_BOOT_FAULT_CMI_PATH,        // 0x26,
    ADM_BOOT_FAULT_ILL_CONFIG,      // 0x27
    ADM_BOOT_FAULT_SW_IMAGE,        // 0x28
    ADM_BOOT_FAULT_DISK_ACCESS,     // 0x29
    ADM_BOOT_STAGE_BLADE_FUP,       // 0x2A
    ADM_BOOT_SW_PANIC_DUMP,         // 0x2B
    ADM_BOOT_NORMAL,                // 0x2C
    ADM_BOOT_STAGE_OS_RUNNING,      // 0x2D
    ADM_BOOT_STAGE_REMOTE_FRU_FUP,  // 0x2E
    ADM_BOOT_BLADE_SERVICED,        // 0x2F
    ADM_BOOT_STAGE_POST_BLADE   = 0x34,
    ADM_BOOT_STAGE_POST_CPU_IO_0,   // 0x35
    ADM_BOOT_STAGE_POST_CPU_IO_1,   // 0x36
    ADM_BOOT_STAGE_POST_CPU_ANNEX,  // 0x37
    ADM_BOOT_STAGE_POST_CPU_DIMMS,  // 0x38
    ADM_BOOT_STAGE_POST_CPU,        // 0x39
    ADM_BOOT_STAGE_POST_COMPLETE,   // 0x3A
    ADM_BOOT_STAGE_BIOS_CPU_IO_0,   // 0x3B
    ADM_BOOT_STAGE_BIOS_CPU_IO_1,   // 0x3C
    ADM_BOOT_STAGE_BIOS_CPU_ANNEX,  // 0x3D
    ADM_BOOT_STAGE_BIOS_CPU_DIMMS,  // 0x3E
    ADM_BOOT_STAGE_BIOS_CPU,        // 0x3F
    ADM_BOOT_STAGE_FAULT_IOM_2,     // 0x40
    ADM_BOOT_FAULT_IOM_3,           // 0x41
    ADM_BOOT_FAULT_IOM_4,           // 0x42
    ADM_BOOT_FAULT_IOM_5,           // 0x43
    ADM_BOOT_FAULT_DOUBLE_IOM_0,    // 0x44
    ADM_BOOT_FAULT_DOUBLE_IOM_1,    // 0x45
    ADM_BOOT_STAGE_POST_CPU_IOM_2,  // 0x46
    ADM_BOOT_STAGE_POST_CPU_IOM_3,  // 0x47
    ADM_BOOT_STAGE_POST_CPU_IOM_4,  // 0x48
    ADM_BOOT_STAGE_POST_CPU_IOM_5,  // 0x49
    ADM_BOOT_STAGE_POST_CPU_DOUBLE_IOM_0,   // 0x4A
    ADM_BOOT_STAGE_POST_CPU_DOUBLE_IOM_1,   // 0x4B
    ADM_BOOT_STAGE_BIOS_CPU_IOM_2,  // 0x4C
    ADM_BOOT_STAGE_BIOS_CPU_IOM_3,  // 0x4D
    ADM_BOOT_STAGE_BIOS_CPU_IOM_4,  // 0x4E
    ADM_BOOT_STAGE_BIOS_CPU_IOM_5,  // 0x4F
    ADM_BOOT_STAGE_BIOS_CPU_DOUBLE_IOM_0,   // 0x50
    ADM_BOOT_STAGE_BIOS_CPU_DOUBLE_IOM_1,   // 0x51
    ADM_BOOT_STAGE_POST_ANNEX_IOM_4,        // 0x52
    ADM_BOOT_STAGE_POST_ANNEX_IOM_5,        // 0x53
    ADM_BOOT_STAGE_POST_ANNEX_IOM_4_5,      // 0x54
    ADM_BOOT_STAGE_POST_CPU_ANNEX_IOM_4,    // 0x55
    ADM_BOOT_STAGE_POST_CPU_ANNEX_IOM_5,    // 0x56
    ADM_BOOT_STAGE_POST_CPU_ANNEX_IOM_4_5,  // 0x57
    ADM_BOOT_STAGE_POST_CPU_MGMT_A,         // 0x58
    ADM_BOOT_STAGE_POST_CPU_MGMT_B,         // 0x59
    ADM_BOOT_STAGE_POST_CPU_MGMT_A_B,       // 0x5A
    ADM_BOOT_FAULT_COMPACT_FLASH,           // 0x5B
    ADM_BOOT_STAGE_POST_CF,                 // 0x5C
    ADM_BOOT_STAGE_BIOS_CF,                 // 0x5D
    ADM_BOOT_STAGE_POST_CF_CPU,             // 0x5E
    ADM_BOOT_STAGE_POST_ALL_CPU,            // 0x5F
    ADM_BOOT_FAULT_UDOC,                    // 0x60
    ADM_BOOT_STAGE_POST_UDOC,               // 0x61
    ADM_BOOT_STAGE_BIOS_UDOC,               // 0x62
    ADM_BOOT_STAGE_POST_CPU_VRM_0,          // 0x63
    ADM_BOOT_STAGE_POST_CPU_VRM_1,          // 0x64
    ADM_BOOT_STAGE_POST_CPU_VRM_0_1,        // 0x65
    ADM_BOOT_FAULT_BOOT_TO_FLASH,           // 0x66
    ADM_BOOT_FAULT_IMAGE,                   // 0x67
    ADM_BOOT_FAULT_CPU_IOM_2,               // 0x68
    ADM_BOOT_FAULT_CPU_IOM_3,               // 0x69
    ADM_BOOT_FAULT_CPU_IOM_4,               // 0x6A
    ADM_BOOT_FAULT_CPU_IOM_5,               // 0x6B
    ADM_BOOT_FAULT_CPU_ANNEX_IOM_4,         // 0x6C
    ADM_BOOT_FAULT_CPU_ANNEX_IOM_5,         // 0x6D
    ADM_BOOT_FAULT_DIMM_0_2,                // 0x6E
    ADM_BOOT_FAULT_DIMM_1_3,                // 0x6F
    ADM_BOOT_FAULT_DIMM_4_6,                // 0x70
    ADM_BOOT_FAULT_DIMM_5_7,                // 0x71
    ADM_BOOT_FAULT_DIMM_0_1_2_3,            // 0x72
    ADM_BOOT_FAULT_DIMM_4_5_6_7,            // 0x73
    ADM_BOOT_FAULT_MRC,                     // 0x74
    ADM_BOOT_FAULT_CPU_ANNEX_IOM_4_5,       // 0x75
    ADM_BOOT_USB_START,                     // 0x76
    ADM_BOOT_USB_POST,                      // 0x77
    ADM_BOOT_USB_ENGINUITY,                 // 0x78
    ADM_BOOT_USB_END,                       // 0x79
    ADM_BOOT_USB_FAIL_POST,                 // 0x7A
    ADM_BOOT_USB_FAIL_ENGINUITY,            // 0x7B
    ADM_BOOT_STAGE_BOOT_PATH,               // 0x7C
    ADM_BOOT_FAULT_MEM_CONFIG,              // 0x7D
    ADM_BOOT_STAGE_BIOS_TEST_CPU = 0xFF
}ADM_PEER_BOOT_CODE;

/*********************************************************************
 * ADM_PEER_SUBFRU_REPLACEMENT_ID enum
 * This enumeration contains the list of FRU Replacement IDs valid for the 
 * "subfru" field of the ADM_PEER_BOOT_INFO.
 **********************************************************************/
typedef enum adm_peer_subfru_replacement_id_enum {
    ADM_FRU_NONE,
    ADM_FRU_SP,
    ADM_FRU_IO_0,
    ADM_FRU_IO_1,
    ADM_FRU_ANNEX,
    ADM_FRU_DIMMS,
    ADM_FRU_DIMM_0,
    ADM_FRU_DIMM_1,
    ADM_FRU_DIMM_2,
    ADM_FRU_DIMM_3,
    ADM_FRU_DIMM_4,
    ADM_FRU_DIMM_5,
    ADM_FRU_DIMM_6,
    ADM_FRU_DIMM_7,
    ADM_FRU_VRM_0,
    ADM_FRU_VRM_1,
    ADM_FRU_MANAGEMENT,
    ADM_FRU_ENCL,
    ADM_FRU_SFP,
    ADM_FRU_CACHE_CARD,
    ADM_FRU_POWER_SUPPLY,
    ADM_FRU_FAN_PACK
} ADM_PEER_SUBFRU_REPLACEMENT_ID;

typedef enum adm_temp_sensor_tag
{
    tempSensor_near_MCH,
    tempSensor_near_IOModule,
    tempSensor_near_PS
} ADM_TEMP_SENSOR;

/***************************
 *  Macro Definitions
 ***************************/


/********************************
 *  Function Prototypes
 ********************************/
#if defined(__cplusplus)
#define CALL_TYPE __stdcall
extern "C"
{
#else
#define CALL_TYPE
#endif

// returns SP signature
UINT_32 CALL_TYPE  dyn_sp_signature(OPAQUE_PTR handle);

// returns peer SP signature
UINT_32 CALL_TYPE  dyn_peer_sp_signature(OPAQUE_PTR handle);

// returns cabinet ID
UINT_32 CALL_TYPE  dyn_cabinet_id(OPAQUE_PTR handle);

// returns SP ID (0=A 1=B);
ADM_SP_ID CALL_TYPE  dyn_sp_id(OPAQUE_PTR handle);

// returns SP nport ID
UINT_32 CALL_TYPE  dyn_nport_id(OPAQUE_PTR handle);

// returns peer SP nport ID
UINT_32 CALL_TYPE  dyn_peer_nport_id(OPAQUE_PTR handle);

// returns SP model number
UINT_32 CALL_TYPE  dyn_sp_model_number(OPAQUE_PTR handle);

// returns SP revision number
void CALL_TYPE  dyn_revision_number(OPAQUE_PTR handle,
                         UINT_32 *major,
                         UINT_32 *minor,
                         UINT_32 *pass_number);

// returns write cache hit ratio (0-100)
UINT_32 CALL_TYPE  dyn_write_cache_hit_ratio(OPAQUE_PTR handle);

// returns write cache dirty page percentage (0-100)
UINT_32 CALL_TYPE  dyn_cache_dirty_page_pct(OPAQUE_PTR handle);

// returns percentage of write cache pages owned by this SP (15-85)
UINT_32 CALL_TYPE  dyn_cache_owned_page_pct(OPAQUE_PTR handle);

// returns the current number of enclosures in the system
UINT_32 CALL_TYPE  dyn_enclosure_count(OPAQUE_PTR handle);

// returns latest user request for write cache state
BOOL CALL_TYPE  dyn_write_cache_enable(OPAQUE_PTR handle);

// returns current write cache state, see ca_states.h
UINT_32 CALL_TYPE  dyn_write_cache_state(OPAQUE_PTR handle);

// returns current write cache state, see ca_states.h
UINT_32 CALL_TYPE  dyn_peer_write_cache_state(OPAQUE_PTR handle);

// returns number of write cache pages
// owned by the indicated SP (0=A 1=B)
UINT_32 CALL_TYPE  dyn_write_cache_pages(OPAQUE_PTR handle, ADM_SP_ID sp);

// returns number of unassigned write cache pages
// owned by the indicated SP (0=A 1=B)
UINT_32 CALL_TYPE  dyn_unassigned_pages(OPAQUE_PTR handle, ADM_SP_ID sp);

// returns TRUE if the read cache state for
// the indicated SP (0=A 1=B) is currently enabled
BOOL CALL_TYPE  dyn_read_cache_enable(OPAQUE_PTR handle, ADM_SP_ID sp);

// returns current read cache state (see ca_states.h for values)
// for the indicated SP (0=A 1=B)
UINT_32 CALL_TYPE  dyn_read_cache_state(OPAQUE_PTR handle, ADM_SP_ID sp);

// returns the physical memory size in megabytes for indicated SP
// returns ADM_PHYSICAL_MEMORY_UNKNOWN if the SP is not present
UINT_32 CALL_TYPE  dyn_physical_memory_size(OPAQUE_PTR handle, ADM_SP_ID sp);

// returns the physical cache size in megabytes for the indicated SP
// This is the total amount of memory available for caching
UINT_32 CALL_TYPE dyn_physical_cache_size(OPAQUE_PTR handle, ADM_SP_ID sp);

// returns the data pool memory in megabytes used outside of the flare driver.
// This is subtracted from the total amount of memory available for caching
UINT_32 CALL_TYPE dyn_external_cache_mem_users_size(OPAQUE_PTR handle);

// returns the top of CMM-managed memory in megabytes
UINT_32 CALL_TYPE  dyn_cmm_control_pool_top(OPAQUE_PTR handle, ADM_SP_ID sp);

// returns the bottom of CMM-managed memory (aka MAXMEM) in megabytes
UINT_32 CALL_TYPE  dyn_cmm_control_pool_bottom(OPAQUE_PTR handle,
                                               ADM_SP_ID sp);

// returns the amount of CMM-managed memory used, not including cache
// control structures (in megabytes)
UINT_32 CALL_TYPE  dyn_cmm_noncache_used(OPAQUE_PTR handle, ADM_SP_ID sp);

// returns the amount of CMM-managed memory used by the cache that does
// not vary based on either page size or cache size
UINT_32 CALL_TYPE  dyn_cache_fixed_overhead(OPAQUE_PTR handle);

// returns the number of bytes of cache data per byte of cache metadata
// this varies depending on page size & cache size
UINT_32 CALL_TYPE  dyn_cache_data_metadata_ratio(OPAQUE_PTR handle);


// returns the world-wide name seed from the SP's non-volatile memory
UINT_32 dyn_wwn_seed(OPAQUE_PTR handle);

// returns TRUE if the peer SP is present, FALSE if it is not
BOOL CALL_TYPE dyn_peer_sp_present(OPAQUE_PTR handle);

// TRUE if the Power Saving feature has been enabled
BOOL CALL_TYPE dyn_global_power_savings_on(OPAQUE_PTR handle);

// Default time a drive can be spun down before being awakened for 
// a health check.
UINT_32 CALL_TYPE dyn_drive_health_check_time(OPAQUE_PTR handle);

// Default time a RAID Group can be idle before it is set to the 
// standby state.
UINT_64 CALL_TYPE dyn_global_idle_time_for_standby(OPAQUE_PTR handle);

// Default time a RAID Group can delay an I/O while waiting for its drives
// to spin up.
UINT_64 CALL_TYPE dyn_latency_time_for_becoming_active(OPAQUE_PTR handle);
/*
 * Performance-related data
 *
 * These functions will always return zero if statistics logging is disabled.
 */

// returns the hard error count
UINT_32 CALL_TYPE  dyn_hard_error_count(OPAQUE_PTR handle);

// returns the number of times the write cache turned on flushing
// due to reaching the high watermark
UINT_32 CALL_TYPE  dyn_high_watermark_flush_on(OPAQUE_PTR handle);

// returns the number of times the write cache turned on flushing
// due to an idle unit
UINT_32 CALL_TYPE  dyn_idle_flush_on(OPAQUE_PTR handle);

// returns the number of times the write cache turned off flushing
// due to reaching the low watermark
UINT_32 CALL_TYPE  dyn_low_watermark_flush_off(OPAQUE_PTR handle);

// returns the number of write cache flushes performed
UINT_32 CALL_TYPE  dyn_write_cache_flushes(OPAQUE_PTR handle);

// returns the number of write cache blocks flushed
UINT_32 CALL_TYPE  dyn_write_blocks_flushed(OPAQUE_PTR handle);

// returns read cache hit ratio (0-100)
UINT_32 CALL_TYPE  dyn_read_cache_hit_ratio(OPAQUE_PTR handle);

// returns the time that the performance data was generated, in the form
// 0xhhmmssxx, where hh is hours (0-23), mm is minutes (0-59), ss is
// seconds (0-59), and xx is hundredths of a second
UINT_32 CALL_TYPE dyn_timestamp(OPAQUE_PTR handle);

// returns the time that Spin-down statistics were first requested, in the form
// 0xhhmmssxx, where hh is hours (0-23), mm is minutes (0-59), ss is
// seconds (0-59), and xx is hundredths of a second
UINT_32 CALL_TYPE dyn_power_saving_stats_start_timestamp(OPAQUE_PTR handle);

// returns the date that Spin-down statistics was first requested, in the form
// 0xMMDDYYYY, where MM is the month (1-12), DD is the day (1-31), 
// and YYYY is the year
UINT_32 CALL_TYPE dyn_power_saving_stats_start_date(OPAQUE_PTR handle);

// returns the sum of the lengths of current request queues seen by each
// user request to this SP (from Flare's point of view). (The length of
// the queue is added to this number each time a request is entered into
// the queue. You can divide this number by the number of arrivals to
// non-zero queue (see below) to determine the average queue length seen
// by a queue request.
UINT_32 CALL_TYPE dyn_sum_of_queue_lengths(OPAQUE_PTR handle);

// returns the number of times a user request arrived while at least one
// other request was being processed.
UINT_32 CALL_TYPE dyn_arrivals_to_nonzero_q(OPAQUE_PTR handle);


/*
 * SP Resumé PROM
 */
// fills in the buffer with the EMC part number string
// the buffer_size parameter indicates the maximum size accepted by the
// caller
// the return value indicates the actual number of bytes entered into
// the buffer (ADM_EMC_PART_NUMBER_SIZE)
UINT_32 CALL_TYPE  dyn_sp_emc_part_no(OPAQUE_PTR handle,
                           UINT_32 buffer_size,
                           TEXT *buffer,
                           ADM_SP_ID sp);

// fills in the buffer with the EMC artwork revision level string
// the buffer_size parameter indicates the maximum size accepted by the
// caller
// the return value indicates the actual number of bytes entered into
// the buffer (ADM_ARTWORK_REVISION_SIZE)
UINT_32 CALL_TYPE  dyn_sp_emc_artwork_revision_level(OPAQUE_PTR handle,
                                          UINT_32 buffer_size,
                                          TEXT *buffer,
                                          ADM_SP_ID sp);

// fills in the buffer with the EMC assembly revision level string
// the buffer_size parameter indicates the maximum size accepted by the
// caller
// the return value indicates the actual number of bytes entered into
// the buffer (ADM_ASSEMBLY_REVISION_SIZE)
UINT_32 CALL_TYPE  dyn_sp_emc_assembly_revision_level(OPAQUE_PTR handle,
                                           UINT_32 buffer_size,
                                           TEXT *buffer,
                                           ADM_SP_ID sp);

// fills in the buffer with the EMC serial number string
// the buffer_size parameter indicates the maximum size accepted by the
// caller
// the return value indicates the actual number of bytes entered into
// the buffer (ADM_EMC_SERIAL_NO_SIZE)
UINT_32 CALL_TYPE  dyn_sp_emc_serial_number(OPAQUE_PTR handle,
                                 UINT_32 buffer_size,
                                 TEXT *buffer,
                                 ADM_SP_ID sp);

// fills in the buffer with the Vendor part number string
// the buffer_size parameter indicates the maximum size accepted by the
// caller
// the return value indicates the actual number of bytes entered into
// the buffer (ADM_VENDOR_PART_NUMBER_SIZE)
UINT_32 CALL_TYPE  dyn_sp_vendor_part_no(OPAQUE_PTR handle,
                           UINT_32 buffer_size,
                           TEXT *buffer,
                           ADM_SP_ID sp);

// fills in the buffer with the Vendor artwork revision level string
// the buffer_size parameter indicates the maximum size accepted by the
// caller
// the return value indicates the actual number of bytes entered into
// the buffer (ADM_ARTWORK_REVISION_SIZE)
UINT_32 CALL_TYPE  dyn_sp_vendor_artwork_revision_level(OPAQUE_PTR handle,
                                          UINT_32 buffer_size,
                                          TEXT *buffer,
                                          ADM_SP_ID sp);

// fills in the buffer with the Vendor assembly revision level string
// the buffer_size parameter indicates the maximum size accepted by the
// caller
// the return value indicates the actual number of bytes entered into
// the buffer (ADM_ASSEMBLY_REVISION_SIZE)
UINT_32 CALL_TYPE  dyn_sp_vendor_assembly_revision_level(OPAQUE_PTR handle,
                                           UINT_32 buffer_size,
                                           TEXT *buffer,
                                           ADM_SP_ID sp);

// fills in the buffer with the Vendor serial number string
// the buffer_size parameter indicates the maximum size accepted by the
// caller
// the return value indicates the actual number of bytes entered into
// the buffer (ADM_VENDOR_SERIAL_NO_SIZE)
UINT_32 CALL_TYPE  dyn_sp_vendor_serial_number(OPAQUE_PTR handle,
                                 UINT_32 buffer_size,
                                 TEXT *buffer,
                                 ADM_SP_ID sp);

// fills in the buffer with the vendor name string
// the buffer_size parameter indicates the maximum size accepted by the
// caller
// the return value indicates the actual number of bytes entered into
// the buffer (ADM_VENDOR_NAME_SIZE)
UINT_32 CALL_TYPE  dyn_sp_vendor_name(OPAQUE_PTR handle,
                           UINT_32 buffer_size,
                           TEXT *buffer,
                           ADM_SP_ID sp);

// fills in the buffer with the location of manufacture string
// the buffer_size parameter indicates the maximum size accepted by the
// caller
// the return value indicates the actual number of bytes entered into
// the buffer (ADM_LOCATION_MANUFACTURE_SIZE)
UINT_32 CALL_TYPE  dyn_sp_location_of_manufacture(OPAQUE_PTR handle,
                                       UINT_32 buffer_size,
                                       TEXT *buffer,
                                       ADM_SP_ID sp);

// fills in the buffer with the date of manufacture string (yyyymmdd)
// the buffer_size parameter indicates the maximum size accepted by the
// caller
// the return value indicates the actual number of bytes entered into
// the buffer (ADM_DATE_MANUFACTURE_SIZE)
UINT_32 CALL_TYPE  dyn_sp_date_of_manufacture(OPAQUE_PTR handle,
                                   UINT_32 buffer_size,
                                   TEXT *buffer,
                                   ADM_SP_ID sp);

// fills in the buffer with the assembly name string
// the buffer_size parameter indicates the maximum size accepted by the
// caller
// the return value indicates the actual number of bytes entered into
// the buffer (ADM_ASSEMBLY_NAME_SIZE)
UINT_32 CALL_TYPE  dyn_sp_assembly_name(OPAQUE_PTR handle,
                             UINT_32 buffer_size,
                             TEXT *buffer,
                             ADM_SP_ID sp);

// the return value indicates the number of programmables listed in the
// resumé PROM
UINT_32 CALL_TYPE  dyn_sp_num_programmables(OPAQUE_PTR handle,
                                            ADM_SP_ID sp);

// Fills in the buffer with the name of the requested programmable
// The programmable parameter indicates which programmable is requested
// The buffer_size parameter indicates the maximum size accepted by the
// caller
// The return value indicates the actual number of bytes entered into
// the buffer (ADM_PROGRAMMABLE_NAME_SIZE)
UINT_32 CALL_TYPE  dyn_sp_programmable_name(OPAQUE_PTR handle,
                                 UINT_32 programmable,
                                 UINT_32 buffer_size,
                                 TEXT *buffer,
                                 ADM_SP_ID sp);

// Fills in the buffer with the revision of the requested programmable
// The programmable parameter indicates which programmable is requested
// The buffer_size parameter indicates the maximum size accepted by the
// caller
// The return value indicates the actual number of bytes entered into
// the buffer (ADM_PROGRAMMABLE_NAME_SIZE)
UINT_32 CALL_TYPE  dyn_sp_programmable_revision(OPAQUE_PTR handle,
                                     UINT_32 programmable,
                                     UINT_32 buffer_size,
                                     TEXT *buffer,
                                     ADM_SP_ID sp);

// the return value indicates the number of MAC addresses listed in the
// resumé PROM
UINT_32 CALL_TYPE  dyn_sp_num_mac_addresses(OPAQUE_PTR handle,
                                            ADM_SP_ID sp);

// Fills in the buffer with the requested MAC address, in ASCII
// The mac_address parameter indicates which MAC address is requested
// The buffer_size parameter indicates the maximum size accepted by the
// caller
// The return value indicates the actual number of bytes entered into
// the buffer (ADM_MAC_ADDRESS_SIZE)
UINT_32 CALL_TYPE  dyn_sp_mac_address(OPAQUE_PTR handle,
                           UINT_32 mac_address,
                           UINT_32 buffer_size,
                           TEXT *buffer,
                           ADM_SP_ID sp);

//  This function returns TRUE it the last resume PROM read was not successful and FALSE
//  if the last read was successful. If TRUE is returned than CM is lighting a
//  FLT light!
BOOL CALL_TYPE dyn_sp_resume_faulted(OPAQUE_PTR handle,
                                     ADM_SP_ID sp);

////////
// subfru resume information
////////
// fills in the buffer with the SubFRU slot number
// the buffer_size parameter indicates the maximum size accepted by the
// caller
// the return value indicates the actual number of bytes entered into
// the buffer
UINT_32 CALL_TYPE dyn_get_subfru_index(OPAQUE_PTR handle,
                                       UINT_32 subfru_type, 
                                       UINT_32 subfru_type_slot);

// fills in the buffer with the EMC part number string
// the buffer_size parameter indicates the maximum size accepted by the
// caller
// the return value indicates the actual number of bytes entered into
// the buffer (ADM_EMC_PART_NUMBER_SIZE)
UINT_32 CALL_TYPE dyn_subfru_emc_part_no(OPAQUE_PTR handle, 
                               UINT_32 buffer_size,
                               TEXT *buffer,
                               ADM_SP_ID sp,
                               UINT_32 subfru_type, 
                               UINT_32 subfru_type_slot);

// fills in the buffer with the EMC artwork revision level string
// the buffer_size parameter indicates the maximum size accepted by the
// caller
// the return value indicates the actual number of bytes entered into
// the buffer (ADM_ARTWORK_REVISION_SIZE)
UINT_32 CALL_TYPE dyn_subfru_emc_artwork_revision_level(OPAQUE_PTR handle, 
                                              UINT_32 buffer_size,
                                              TEXT *buffer,
                                              ADM_SP_ID sp,
                                              UINT_32 subfru_type, 
                                              UINT_32 subfru_type_slot);

// fills in the buffer with the EMC assembly revision level string
// the buffer_size parameter indicates the maximum size accepted by the
// caller
// the return value indicates the actual number of bytes entered into
// the buffer (ADM_ASSEMBLY_REVISION_SIZE)
UINT_32 CALL_TYPE dyn_subfru_emc_assembly_revision_level(OPAQUE_PTR handle, 
                                               UINT_32 buffer_size,
                                               TEXT *buffer,
                                               ADM_SP_ID sp,
                                               UINT_32 subfru_type, 
                                               UINT_32 subfru_type_slot);

// fills in the buffer with the EMC serial number string
// the buffer_size parameter indicates the maximum size accepted by the
// caller
// the return value indicates the actual number of bytes entered into
// the buffer (ADM_EMC_SERIAL_NO_SIZE)
UINT_32 CALL_TYPE dyn_subfru_emc_serial_number(OPAQUE_PTR handle, 
                                     UINT_32 buffer_size,
                                     TEXT *buffer,
                                     ADM_SP_ID sp,
                                     UINT_32 subfru_type, 
                                     UINT_32 subfru_type_slot);

// fills in the buffer with the Vendor part number string
// the buffer_size parameter indicates the maximum size accepted by the
// caller
// the return value indicates the actual number of bytes entered into
// the buffer (ADM_VENDOR_PART_NUMBER_SIZE)
UINT_32 CALL_TYPE dyn_subfru_vendor_part_no(OPAQUE_PTR handle, 
                                            UINT_32 buffer_size,
                                            TEXT *buffer,
                                            ADM_SP_ID sp,
                                            UINT_32 subfru_type, 
                                            UINT_32 subfru_type_slot);

// fills in the buffer with the Vendor artwork revision level string
// the buffer_size parameter indicates the maximum size accepted by the
// caller
// the return value indicates the actual number of bytes entered into
// the buffer (ADM_ARTWORK_REVISION_SIZE)
UINT_32 CALL_TYPE dyn_subfru_vendor_artwork_revision_level(OPAQUE_PTR handle, 
                                                           UINT_32 buffer_size,
                                                           TEXT *buffer,
                                                           ADM_SP_ID sp,
                                                           UINT_32 subfru_type, 
                                                           UINT_32 subfru_type_slot);

// fills in the buffer with the Vendor assembly revision level string
// the buffer_size parameter indicates the maximum size accepted by the
// caller
// the return value indicates the actual number of bytes entered into
// the buffer (ADM_ASSEMBLY_REVISION_SIZE)
UINT_32 CALL_TYPE dyn_subfru_vendor_assembly_revision_level(OPAQUE_PTR handle, 
                                                            UINT_32 buffer_size,
                                                            TEXT *buffer,
                                                            ADM_SP_ID sp,
                                                            UINT_32 subfru_type, 
                                                            UINT_32 subfru_type_slot);

// fills in the buffer with the EMC serial number string
// the buffer_size parameter indicates the maximum size accepted by the
// caller
// the return value indicates the actual number of bytes entered into
// the buffer (ADM_VENDOR_SERIAL_NO_SIZE)
UINT_32 CALL_TYPE dyn_subfru_vendor_serial_number(OPAQUE_PTR handle, 
                                                  UINT_32 buffer_size,
                                                  TEXT *buffer,
                                                  ADM_SP_ID sp,
                                                  UINT_32 subfru_type, 
                                                  UINT_32 subfru_type_slot);

// fills in the buffer with the vendor name string
// the buffer_size parameter indicates the maximum size accepted by the
// caller
// the return value indicates the actual number of bytes entered into
// the buffer (ADM_VENDOR_NAME_SIZE)
UINT_32 CALL_TYPE dyn_subfru_vendor_name(OPAQUE_PTR handle, 
                               UINT_32 buffer_size,
                               TEXT *buffer,
                               ADM_SP_ID sp,
                               UINT_32 subfru_type, 
                               UINT_32 subfru_type_slot);

// fills in the buffer with the location of manufacture string
// the buffer_size parameter indicates the maximum size accepted by the
// caller
// the return value indicates the actual number of bytes entered into
// the buffer (ADM_LOCATION_MANUFACTURE_SIZE)
UINT_32 CALL_TYPE dyn_subfru_location_of_manufacture(OPAQUE_PTR handle, 
                                           UINT_32 buffer_size,
                                           TEXT *buffer,
                                           ADM_SP_ID sp,
                                           UINT_32 subfru_type, 
                                           UINT_32 subfru_type_slot);

// fills in the buffer with the date of manufacture string (yyyymmdd)
// the buffer_size parameter indicates the maximum size accepted by the
// caller
// the return value indicates the actual number of bytes entered into
// the buffer (ADM_DATE_MANUFACTURE_SIZE)
UINT_32 CALL_TYPE dyn_subfru_date_of_manufacture(OPAQUE_PTR handle, 
                                       UINT_32 buffer_size,
                                       TEXT *buffer,
                                       ADM_SP_ID sp,
                                       UINT_32 subfru_type, 
                                       UINT_32 subfru_type_slot);

// fills in the buffer with the assembly name string
// the buffer_size parameter indicates the maximum size accepted by the
// caller
// the return value indicates the actual number of bytes entered into
// the buffer (ADM_ASSEMBLY_NAME_SIZE)
UINT_32 CALL_TYPE dyn_subfru_assembly_name(OPAQUE_PTR handle, 
                                 UINT_32 buffer_size,
                                 TEXT *buffer,
                                 ADM_SP_ID sp,
                                 UINT_32 subfru_type, 
                                 UINT_32 subfru_type_slot);

// the return value indicates the number of programmables listed in the
// resumé PROM
UINT_32 CALL_TYPE dyn_subfru_num_programmables(OPAQUE_PTR handle,
                                     ADM_SP_ID sp,
                                     UINT_32 subfru_type, 
                                     UINT_32 subfru_type_slot);

// Fills in the buffer with the name of the requested programmable
// The programmable parameter indicates which programmable is requested
// The buffer_size parameter indicates the maximum size accepted by the
// caller
// The return value indicates the actual number of bytes entered into
// the buffer (ADM_PROGRAMMABLE_NAME_SIZE)
UINT_32 CALL_TYPE dyn_subfru_programmable_name(OPAQUE_PTR handle, 
                                     UINT_32 programmable,
                                     UINT_32 buffer_size,
                                     TEXT *buffer,
                                     ADM_SP_ID sp,
                                     UINT_32 subfru_type, 
                                     UINT_32 subfru_type_slot);

// Fills in the buffer with the revision of the requested programmable
// The programmable parameter indicates which programmable is requested
// The buffer_size parameter indicates the maximum size accepted by the
// caller
// The return value indicates the actual number of bytes entered into
// the buffer (ADM_PROGRAMMABLE_NAME_SIZE)
UINT_32 CALL_TYPE dyn_subfru_programmable_revision(OPAQUE_PTR handle, 
                                         UINT_32 programmable,
                                         UINT_32 buffer_size,
                                         TEXT *buffer,
                                         ADM_SP_ID sp,
                                         UINT_32 subfru_type, 
                                         UINT_32 subfru_type_slot);

// the return value indicates the number of MAC addresses listed in the
// resumé PROM
UINT_32 CALL_TYPE dyn_subfru_num_mac_addresses(OPAQUE_PTR handle,
                                     ADM_SP_ID sp,
                                     UINT_32 subfru_type, 
                                     UINT_32 subfru_type_slot);

// Fills in the buffer with the requested MAC address, in ASCII
// The mac_address parameter indicates which MAC address is requested
// The buffer_size parameter indicates the maximum size accepted by the
// caller
// The return value indicates the actual number of bytes entered into
// the buffer (ADM_MAC_ADDRESS_SIZE)
UINT_32 CALL_TYPE dyn_subfru_mac_address(OPAQUE_PTR handle, 
                               UINT_32 mac_address,
                               UINT_32 buffer_size,
                               TEXT *buffer,
                               ADM_SP_ID sp,
                               UINT_32 subfru_type, 
                               UINT_32 subfru_type_slot);

//  This function returns TRUE it the last resume PROM read was not successful and FALSE
//  if the last read was successful. If TRUE is returned than CM is lighting a
//  FLT light!
BOOL CALL_TYPE dyn_subfru_resume_faulted(OPAQUE_PTR handle,
                               ADM_SP_ID sp,
                               UINT_32 subfru_type, 
                               UINT_32 subfru_type_slot);

//This function returns TRUE if the subfru is physically inserted (present)
BOOL CALL_TYPE dyn_subfru_inserted(OPAQUE_PTR handle,
                         ADM_SP_ID sp,
                         UINT_32 subfru_type, 
                         UINT_32 subfru_type_slot);

//This function returns a quantity associated with the subfru type based on SP.
UINT_32 CALL_TYPE dyn_subfru_count(OPAQUE_PTR handle,
                                    ADM_SP_ID sp,
                                    UINT_32 subfru_type);

//This function returns a quantity associated with the subfru type in xPE enclosure.
UINT_32 CALL_TYPE dyn_subfru_total_count(OPAQUE_PTR handle,
                                    UINT_32 subfru_type);

// This function returns the subfru state code. Used to determine
// if the subfru is faulted.
UINT_32 CALL_TYPE dyn_subfru_state(OPAQUE_PTR handle, 
                                      ADM_SP_ID sp,
                                      UINT_32 subfru_type,
                                      UINT_32 subfru_type_slot);

//This function returns a quantity associated with the subfru type
BOOL CALL_TYPE dyn_subfru_is_resume_supported(OPAQUE_PTR handle,
                                                ADM_SP_ID sp,
                                                UINT_32 subfru_type);

/*
 * Personality Card Resumé PROM
 */

// fills in the buffer with the personality card EMC part number string
// the buffer_size parameter indicates the maximum size accepted by the
// caller
// the return value indicates the actual number of bytes entered into
// the buffer (ADM_EMC_PART_NUMBER_SIZE)
UINT_32 CALL_TYPE dyn_sp_pers_card_emc_part_no(OPAQUE_PTR handle,
                                               UINT_32 buffer_size,
                                               TEXT *buffer);

// fills in the buffer with the personality card EMC artwork revision level
// string
// the buffer_size parameter indicates the maximum size accepted by the
// caller
// the return value indicates the actual number of bytes entered into
// the buffer (ADM_ARTWORK_REVISION_SIZE)
UINT_32 CALL_TYPE
    dyn_sp_pers_card_emc_artwork_revision_level(OPAQUE_PTR handle,
                                                UINT_32 buffer_size,
                                                TEXT *buffer);

// fills in the buffer with the personality card EMC assembly revision level
// string
// the buffer_size parameter indicates the maximum size accepted by the
// caller
// the return value indicates the actual number of bytes entered into
// the buffer (ADM_ASSEMBLY_REVISION_SIZE)
UINT_32 CALL_TYPE
    dyn_sp_pers_card_emc_assembly_revision_level(OPAQUE_PTR handle,
                                                 UINT_32 buffer_size,
                                                 TEXT *buffer);

// fills in the buffer with the personality card EMC serial number string
// the buffer_size parameter indicates the maximum size accepted by the
// caller
// the return value indicates the actual number of bytes entered into
// the buffer (ADM_EMC_SERIAL_NO_SIZE)
UINT_32 CALL_TYPE dyn_sp_pers_card_emc_serial_number(OPAQUE_PTR handle,
                                                     UINT_32 buffer_size,
                                                     TEXT *buffer);

// fills in the buffer with the personality card Vendor part number string
// the buffer_size parameter indicates the maximum size accepted by the
// caller
// the return value indicates the actual number of bytes entered into
// the buffer (ADM_VENDOR_PART_NUMBER_SIZE)
UINT_32 CALL_TYPE dyn_sp_pers_card_vendor_part_no(OPAQUE_PTR handle,
                                                  UINT_32 buffer_size,
                                                  TEXT *buffer);

// fills in the buffer with the personality card Vendor artwork revision level
// string
// the buffer_size parameter indicates the maximum size accepted by the
// caller
// the return value indicates the actual number of bytes entered into
// the buffer (ADM_ARTWORK_REVISION_SIZE)
UINT_32 CALL_TYPE
    dyn_sp_pers_card_vendor_artwork_revision_level(OPAQUE_PTR handle,
                                                   UINT_32 buffer_size,
                                                   TEXT *buffer);

// fills in the buffer with the personality card Vendor assembly revision level
// string
// the buffer_size parameter indicates the maximum size accepted by the
// caller
// the return value indicates the actual number of bytes entered into
// the buffer (ADM_ASSEMBLY_REVISION_SIZE)
UINT_32 CALL_TYPE
    dyn_sp_pers_card_vendor_assembly_revision_level(OPAQUE_PTR handle,
                                                    UINT_32 buffer_size,
                                                    TEXT *buffer);

// fills in the buffer with the personality card Vendor serial number string
// the buffer_size parameter indicates the maximum size accepted by the
// caller
// the return value indicates the actual number of bytes entered into
// the buffer (ADM_VENDOR_SERIAL_NO_SIZE)
UINT_32 CALL_TYPE dyn_sp_pers_card_vendor_serial_number(OPAQUE_PTR handle,
                                                        UINT_32 buffer_size,
                                                        TEXT *buffer);

// fills in the buffer with the personality card Vendor part number string
// the buffer_size parameter indicates the maximum size accepted by the
// caller
// the return value indicates the actual number of bytes entered into
// the buffer (ADM_VENDOR_PART_NUMBER_SIZE)
UINT_32 CALL_TYPE dyn_sp_pers_card_vendor_part_no(OPAQUE_PTR handle,
                                                  UINT_32 buffer_size,
                                                  TEXT *buffer);

// fills in the buffer with the personality card Vendor artwork revision level
// string
// the buffer_size parameter indicates the maximum size accepted by the
// caller
// the return value indicates the actual number of bytes entered into
// the buffer (ADM_ARTWORK_REVISION_SIZE)
UINT_32 CALL_TYPE
    dyn_sp_pers_card_vendor_artwork_revision_level(OPAQUE_PTR handle,
                                                   UINT_32 buffer_size,
                                                   TEXT *buffer);

// fills in the buffer with the personality card Vendor assembly revision level
// string
// the buffer_size parameter indicates the maximum size accepted by the
// caller
// the return value indicates the actual number of bytes entered into
// the buffer (ADM_ASSEMBLY_REVISION_SIZE)
UINT_32 CALL_TYPE
    dyn_sp_pers_card_vendor_assembly_revision_level(OPAQUE_PTR handle,
                                                    UINT_32 buffer_size,
                                                    TEXT *buffer);

// fills in the buffer with the personality card Vendor serial number string
// the buffer_size parameter indicates the maximum size accepted by the
// caller
// the return value indicates the actual number of bytes entered into
// the buffer (ADM_VENDOR_SERIAL_NO_SIZE)
UINT_32 CALL_TYPE dyn_sp_pers_card_vendor_serial_number(OPAQUE_PTR handle,
                                                        UINT_32 buffer_size,
                                                        TEXT *buffer);
// fills in the buffer with the personality card vendor name string
// the buffer_size parameter indicates the maximum size accepted by the
// caller
// the return value indicates the actual number of bytes entered into
// the buffer (ADM_VENDOR_NAME_SIZE)
UINT_32 CALL_TYPE dyn_sp_pers_card_vendor_name(OPAQUE_PTR handle,
                                               UINT_32 buffer_size,
                                               TEXT *buffer);

// fills in the buffer with the personality card location of manufacture
// string
// the buffer_size parameter indicates the maximum size accepted by the
// caller
// the return value indicates the actual number of bytes entered into
// the buffer (ADM_LOCATION_MANUFACTURE_SIZE)
UINT_32 CALL_TYPE
    dyn_sp_pers_card_location_of_manufacture(OPAQUE_PTR handle,
                                             UINT_32 buffer_size,
                                             TEXT *buffer);

// fills in the buffer with the personality card date of manufacture string
// (yyyymmdd)
// the buffer_size parameter indicates the maximum size accepted by the
// caller
// the return value indicates the actual number of bytes entered into
// the buffer (ADM_DATE_MANUFACTURE_SIZE)
UINT_32 CALL_TYPE dyn_sp_pers_card_date_of_manufacture(OPAQUE_PTR handle,
                                                       UINT_32 buffer_size,
                                                       TEXT *buffer);

// fills in the buffer with the personality card assembly name string
// the buffer_size parameter indicates the maximum size accepted by the
// caller
// the return value indicates the actual number of bytes entered into
// the buffer (ADM_ASSEMBLY_NAME_SIZE)
UINT_32 CALL_TYPE dyn_sp_pers_card_assembly_name(OPAQUE_PTR handle,
                                                 UINT_32 buffer_size,
                                                 TEXT *buffer);


//  This function returns TRUE it the last sp pers card resume PROM read was not successful and FALSE
//  if the last read was successful. If TRUE is returned than CM is lighting a
//  FLT light! 
BOOL CALL_TYPE dyn_sp_pers_card_resume_faulted(OPAQUE_PTR handle);

//  This function returns the SFP condition for back end ports
UINT_32 CALL_TYPE dyn_sfp_be_condition(OPAQUE_PTR handle, UINT_32 sfp_slot);

// return the peer boot state code
UINT_32 CALL_TYPE dyn_peer_boot_state(OPAQUE_PTR handle);

// return the peer boot status/failure code
UINT_32 CALL_TYPE dyn_peer_boot_fault_code(OPAQUE_PTR handle);

// WCA
UINT_32 CALL_TYPE dyn_write_cache_max_cache_limit(OPAQUE_PTR handle);
UINT_32 CALL_TYPE dyn_read_cache_max_cache_limit(OPAQUE_PTR handle);

// Returns overall process information for online drive firmware download

UINT_32 CALL_TYPE dyn_ofd_filename(OPAQUE_PTR handle, UINT_32 buffer_size, TEXT *buffer);

UINT_32 CALL_TYPE dyn_ofd_max_rebuild_logs(OPAQUE_PTR handle);

OFD_OPERATION CALL_TYPE dyn_ofd_operation_type(OPAQUE_PTR handle);

OFD_SP_STAT CALL_TYPE dyn_ofd_process_status(OPAQUE_PTR handle);

OFD_SP_FAIL CALL_TYPE dyn_ofd_process_fail_code(OPAQUE_PTR handle);

// Returns the start time for online drive firmware download
UINT_32 CALL_TYPE dyn_ofd_start_time(OPAQUE_PTR handle);

// Returns the stop time for online drive firmware download
UINT_32 CALL_TYPE dyn_ofd_stop_time(OPAQUE_PTR handle);

//
// Functions to return IO module, port, and sfp related information
//
// return the max number of IO modules supported in xPE enclosure 
UINT_32 CALL_TYPE dyn_get_iom_total_max_count(OPAQUE_PTR handle);

// return the max number of IO modules supported on each SP
UINT_32 CALL_TYPE dyn_get_iom_max_count(OPAQUE_PTR handle, IO_MODULE_CLASS iom_class);

// indicates if a module is inserted inthe specified slot
BOOL CALL_TYPE dyn_get_iom_is_inserted(OPAQUE_PTR handle, 
                                ADM_SP_ID sp,
                                IO_MODULE_CLASS iom_class,
                                UINT_32 slot);

// return the IO module state
IOM_STATE CALL_TYPE dyn_get_iom_state(OPAQUE_PTR handle, ADM_SP_ID sp, 
                                      IO_MODULE_CLASS iom_class, UINT_32 slot);

// return the IO module substate
IOM_SUBSTATE CALL_TYPE dyn_get_iom_substate(OPAQUE_PTR handle, 
                                ADM_SP_ID sp,
                                IO_MODULE_CLASS iom_class,
                                UINT_32 slot);

// return the IO module type
IO_CONTROLLER_PROTOCOL CALL_TYPE dyn_get_iom_type(OPAQUE_PTR handle, ADM_SP_ID sp, 
                                                  IO_MODULE_CLASS iom_class, UINT_32 slot);

// return the power status of the IO module
IOM_POWER_STATUS CALL_TYPE dyn_get_iom_power_status(OPAQUE_PTR handle, 
                                    ADM_SP_ID sp, 
                                    IO_MODULE_CLASS iom_class,
                                    UINT_32 slot);

// indicates if the IO module is inserted in the IO Annex
BOOL CALL_TYPE dyn_get_iom_is_in_io_annex(OPAQUE_PTR handle, 
                                ADM_SP_ID sp, 
                                IO_MODULE_CLASS iom_class,
                                UINT_32 slot);

// return the number of ports on the IO module
UINT_32 CALL_TYPE dyn_get_iom_num_ports(OPAQUE_PTR handle, ADM_SP_ID sp, IO_MODULE_CLASS iom_class, UINT_32 slot);

// return the IO module slot number
UINT_32 CALL_TYPE dyn_get_iom_slot_num(OPAQUE_PTR handle, 
                             UINT_32 logical_num, 
                             PORT_ROLE port_role);
// return the io module class
IO_MODULE_CLASS CALL_TYPE dyn_get_io_module_class(OPAQUE_PTR handle,
                                        UINT_32 logical_num, 
                                        UINT_32 port_role);

// return the port address
UINT_32 dyn_get_port_addr(OPAQUE_PTR handle,
                          IO_MODULE_CLASS iom_class,
                          UINT_32 slot, 
                          UINT_32 port);

//takes a slot and port number and converts it to a slot, controller, port location.
VOID dyn_adjust_controller_port_number(OPAQUE_PTR handle, 
                                       UINT_32 iom_num, 
                                       UINT_32 *controller_num, 
                                       UINT_32 *port_num);

// return the port state
PORT_STATE CALL_TYPE dyn_get_port_state(OPAQUE_PTR handle, 
                                        IO_MODULE_CLASS iom_class,
                                        UINT_32 slot, 
                                        UINT_32 port);

// return the port substate
PORT_SUBSTATE CALL_TYPE dyn_get_port_substate(OPAQUE_PTR handle, 
                                              IO_MODULE_CLASS iom_class,
                                              UINT_32 slot, 
                                              UINT_32 port);

// return the port role
PORT_ROLE CALL_TYPE dyn_get_port_role(OPAQUE_PTR handle, 
                                      IO_MODULE_CLASS iom_class,
                                      UINT_32 slot, 
                                      UINT_32 port);

// return the port subrole
PORT_SUBROLE CALL_TYPE dyn_get_port_subrole(OPAQUE_PTR handle, 
                                    IO_MODULE_CLASS iom_class,
                                    UINT_32 slot, 
                                    UINT_32 port);

// return the port protocol
PORT_PROTOCOL CALL_TYPE dyn_get_port_protocol(OPAQUE_PTR handle, 
                                    IO_MODULE_CLASS iom_class,
                                    UINT_32 slot,
                                    UINT_32 port);

// return iom group
UINT_32 CALL_TYPE dyn_get_port_iom_group(OPAQUE_PTR handle,
                                         IO_MODULE_CLASS iom_class,
                                    UINT_32 slot, 
                                    UINT_32 port);

// return the slic type
UINT_32 CALL_TYPE dyn_get_slic_type(OPAQUE_PTR handle, ADM_SP_ID sp,
                                    IO_MODULE_CLASS iom_class,
                                    UINT_32 slot);

//return the expected slic types
UINT_32 CALL_TYPE dyn_get_slic_expected_types(OPAQUE_PTR handle, 
                                              IO_MODULE_CLASS iom_class,
                                    UINT_32 slot);
// return the logical number
UINT_32 CALL_TYPE dyn_get_port_logical_num(OPAQUE_PTR handle, 
                                           IO_MODULE_CLASS iom_class,
                                    UINT_32 slot, 
                                    UINT_32 port);

// return the physical port number
UINT_32 CALL_TYPE dyn_get_port_physical_num(OPAQUE_PTR handle, 
                                    UINT_32 logical_num, 
                                    UINT_32 port_role);

// return the max allowed ports for the pecified protocol
UINT_32 CALL_TYPE dyn_get_max_ports_port_protocol(OPAQUE_PTR handle,
                                        PORT_PROTOCOL port_protocol);

// return the SFP condition
UINT_32 CALL_TYPE dyn_get_sfp_condition(OPAQUE_PTR handle, IO_MODULE_CLASS iom_class, 
                                        UINT_32 slot, UINT_32 port);

// return the SFP cable length
UINT_32 CALL_TYPE dyn_get_sfp_cable_length(OPAQUE_PTR handle, IO_MODULE_CLASS iom_class, 
                                           UINT_32 slot, UINT_32 port);

// return the max SFP speed
UINT_32 CALL_TYPE dyn_get_sfp_speed(OPAQUE_PTR handle, IO_MODULE_CLASS iom_class, 
                                    UINT_32 slot, UINT_32 port);

// return the SFP type
UINT_32 CALL_TYPE dyn_get_sfp_type(OPAQUE_PTR handle, IO_MODULE_CLASS iom_class, 
                                   UINT_32 slot, UINT_32 port);

/* 
 * These functions return the EMC and vendor part and serial number from the SFP
 * resume information.
 */
VOID CALL_TYPE dyn_get_sfp_emc_part_num(OPAQUE_PTR handle, IO_MODULE_CLASS iom_class, 
                              UINT_32 slot, UINT_32 port, TEXT *buffer);
VOID CALL_TYPE dyn_get_sfp_emc_serial_num(OPAQUE_PTR handle, IO_MODULE_CLASS iom_class, 
                                UINT_32 slot, UINT_32 port, TEXT *buffer);
VOID CALL_TYPE dyn_get_sfp_vendor_part_num(OPAQUE_PTR handle, IO_MODULE_CLASS iom_class, 
                                 UINT_32 slot, UINT_32 port, TEXT *buffer);
VOID CALL_TYPE dyn_get_sfp_vendor_serial_num(OPAQUE_PTR handle, IO_MODULE_CLASS iom_class, 
                                   UINT_32 slot, UINT_32 port, TEXT *buffer);

// return if I/O module Fault LED on
BOOL CALL_TYPE dyn_get_iom_is_flt_led_on(OPAQUE_PTR handle, ADM_SP_ID sp, 
                                         IO_MODULE_CLASS iom_class, UINT_32 slot);

// return if I/O module is being Marked
BOOL CALL_TYPE dyn_get_iom_is_marked(OPAQUE_PTR handle, ADM_SP_ID sp, 
                                     IO_MODULE_CLASS iom_class, UINT_32 slot);

// return I/O module's port LED states (bitmask)
UINT_32 CALL_TYPE dyn_get_iom_port_led_states(OPAQUE_PTR handle, ADM_SP_ID sp, 
                                              IO_MODULE_CLASS iom_class, UINT_32 slot);

// return the bitmask of supported slic types
UINT_32 CALL_TYPE dyn_get_supported_slic_types(OPAQUE_PTR handle);

// return TRUE if port uses SFP
BOOL CALL_TYPE dyn_get_port_uses_sfps (OPAQUE_PTR handle, IO_MODULE_CLASS iom_class, 
                                       UINT_32 slot, UINT_32 port);

// return the supported speeds for IO module
UINT_32 CALL_TYPE dyn_get_port_supported_speeds (OPAQUE_PTR handle,
                                                IO_MODULE_CLASS iom_class, 
                                                UINT_32 slot,
                                                UINT_32 port);

// retunr the unique_id (fru id) for IO module
HW_MODULE_TYPE CALL_TYPE dyn_get_iom_id(OPAQUE_PTR handle, ADM_SP_ID sp, 
                                        IO_MODULE_CLASS iom_class, UINT_32 slot);

// return maximun number of FC BE ports
UINT_32 CALL_TYPE dyn_get_max_fc_be_ports(OPAQUE_PTR handle);

// return the maximum number of FC (BE + FE) ports
UINT_32 CALL_TYPE dyn_get_max_fc_ports(OPAQUE_PTR handle);

// return the maximum number of iSCSI (1G + 10G) ports
UINT_32 CALL_TYPE dyn_get_max_iscsi_ports(OPAQUE_PTR handle);

// return the maximum number of FCoE ports
UINT_32 CALL_TYPE dyn_get_max_fcoe_fe_ports(OPAQUE_PTR handle);

// return the maximum number of 10G iSCSI ports
UINT_32 CALL_TYPE dyn_get_max_iscsi_10g_ports(OPAQUE_PTR handle);

UINT_32 CALL_TYPE dyn_get_max_sas_be_ports(OPAQUE_PTR handle);
UINT_32 CALL_TYPE dyn_get_max_sas_fe_ports(OPAQUE_PTR handle);
UINT_32 CALL_TYPE dyn_get_max_fcoe_fe_ports(OPAQUE_PTR handle);

UINT_32 CALL_TYPE dyn_self_sp_isolated_fault_code(OPAQUE_PTR handle);
BOOL CALL_TYPE dyn_self_sp_has_definitive_isolated_fault(OPAQUE_PTR handle);
BOOL CALL_TYPE dyn_self_sp_has_ambiguous_isolated_fault(OPAQUE_PTR handle);
UINT_32 CALL_TYPE dyn_peer_sp_isolated_fault_code(OPAQUE_PTR handle);
BOOL CALL_TYPE dyn_peer_sp_has_definitive_isolated_fault(OPAQUE_PTR handle);
BOOL CALL_TYPE dyn_peer_sp_has_ambiguous_isolated_fault(OPAQUE_PTR handle);

/*Suitcase Info*/
BOOL CALL_TYPE dyn_self_sp_temperature_sensor_faulted(OPAQUE_PTR handle);
BOOL CALL_TYPE dyn_peer_sp_temperature_sensor_faulted(OPAQUE_PTR handle);
BOOL CALL_TYPE dyn_self_sp_ambient_temperature_faulted(OPAQUE_PTR handle);
BOOL CALL_TYPE dyn_peer_sp_ambient_temperature_faulted(OPAQUE_PTR handle);
BOOL CALL_TYPE dyn_self_sp_shutting_down(OPAQUE_PTR handle);
BOOL CALL_TYPE dyn_peer_sp_shutting_down(OPAQUE_PTR handle);
BOOL CALL_TYPE dyn_is_suitcase_DataStale(OPAQUE_PTR handle);
BOOL CALL_TYPE dyn_is_suitcase_fruSpecificSmbusReadFail(OPAQUE_PTR handle);
BOOL CALL_TYPE dyn_is_local_suitcase(OPAQUE_PTR handle);
BOOL CALL_TYPE dyn_is_suitcase_coolingModule(OPAQUE_PTR handle);

/*Mezzanine Info*/
BOOL CALL_TYPE dyn_mezzanine_inserted(OPAQUE_PTR handle);
BOOL CALL_TYPE dyn_peer_mezzanine_inserted(OPAQUE_PTR handle);
BOOL CALL_TYPE dyn_is_mezzanine_DataStale(OPAQUE_PTR handle);
BOOL CALL_TYPE dyn_is_local_mezzanine(OPAQUE_PTR handle);
ENCLOSURE_SP_ID CALL_TYPE dyn_mezzanine_associatedSP(OPAQUE_PTR handle);
ENCLOSURE_FRU_LOCATION CALL_TYPE dyn_mezzanine_location(OPAQUE_PTR handle);
ENCLOSURE_FRU_SUB_LOCATION  CALL_TYPE dyn_mezzanine_subLocation(OPAQUE_PTR handle);
BOOL CALL_TYPE dyn_is_mezzanine_logicalFault(OPAQUE_PTR handle);
BOOL CALL_TYPE dyn_is_mezzanine_stateReadFault(OPAQUE_PTR handle);
BOOL CALL_TYPE dyn_is_mezzanine_fruSpecificSmbusReadFail (OPAQUE_PTR handle);
HW_MODULE_TYPE CALL_TYPE dyn_mezzanine_uniqueID(OPAQUE_PTR handle);
ULONG CALL_TYPE dyn_mezzanine_ioControllerCount(OPAQUE_PTR handle);

/*
 * Energy Info Reporting statistics
 */
// General stats values
ULONG CALL_TYPE dyn_sp_stats_polling_interval(OPAQUE_PTR handle);
ULONG CALL_TYPE dyn_sp_stats_window_duration(OPAQUE_PTR handle);
// Input Power stats
ULONG CALL_TYPE dyn_sp_current_input_power(OPAQUE_PTR handle);
ULONG CALL_TYPE dyn_sp_max_input_power(OPAQUE_PTR handle);
ULONG CALL_TYPE dyn_sp_average_input_power(OPAQUE_PTR handle);
ULONG CALL_TYPE dyn_sp_input_power_status(OPAQUE_PTR handle);
// Utilization stats
ULONG CALL_TYPE dyn_sp_current_array_utilization(OPAQUE_PTR handle);
ULONG CALL_TYPE dyn_sp_max_array_utilization(OPAQUE_PTR handle);
ULONG CALL_TYPE dyn_sp_average_array_utilization(OPAQUE_PTR handle);
ULONG CALL_TYPE dyn_sp_array_utilization_status(OPAQUE_PTR handle);

#if defined(__cplusplus)
}
#endif
#endif /* ADM_SP_API_H */

