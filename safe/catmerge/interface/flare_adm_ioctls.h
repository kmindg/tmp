#ifndef FLARE_ADM_IOCTLS_H
#define FLARE_ADM_IOCTLS_H 0x0000001
#define FLARE__STAR__H
/***************************************************************************
 * Copyright (C) EMC Corporation 2001-2008,2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *  flare_adm_ioctls.h
 ***************************************************************************
 *
 *  Description
 *
 *      This file contains the external interface to the IOCTL commands
 *      supported by the Admin Interface "adm" module in the Flare driver.
 *
 *  History:
 *
 *      06/19/01 CJH    Created
 *      09/18/01 CJH    Add default values
 *      06/24/05 CJH    New power_on values for backdoor power (DIMS 126137)
 *      08/11/05 IM     Added new IOCTL for SP reboot
 ***************************************************************************/

#include "k10defs.h"
#include "adm_enclosure_api.h"
#include "cm_config_exports.h"
#include "cm_environ_exports.h"
#include "vp_exports.h"
#include "led_data.h"
#include "environment.h"
#include "flare_cpu_throttle.h"
#include "K10NDUAdminExport.h"
//#include <devioctl.h>

/************
 * Literals
 ************/

/*
 * NOTE: IOCTL codes 0x888 through 0x889 are defined in flare_ioctls.h
 * and IOCTL codes 0x88a through 0x891 are defined in
 * flare_export_ioctls.h.  Additions and changes must be coordinated
 * between these three header files.  A gap from 0x892 through 0x8A0 has
 * been left for future growth, but if more than 15 new IOCTLs are added
 * to the other headers, then they will have to skip the codes used
 * here.
 */

// Initialize the interface
#define IOCTL_FLARE_ADM_INITIALIZE \
    EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0x8A1, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

// Return data for Navi poll
#ifdef ALAMOSA_WINDOWS_ENV
#define IOCTL_FLARE_ADM_POLL \
    EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0x8A2, EMCPAL_IOCTL_METHOD_IN_DIRECT, EMCPAL_IOCTL_FILE_ANY_ACCESS)
#else
#define IOCTL_FLARE_ADM_POLL \
    EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0x8A2, EMCPAL_IOCTL_METHOD_OUT_DIRECT, EMCPAL_IOCTL_FILE_ANY_ACCESS)
#endif /* ALAMOSA_WINDOWS_ENV - ODDCASE - this should be OUT_DIRECT.   IN_DIRECT treats the output buffer as another input buffer.  here we're using the output buffer for output */

// End of poll (for future optimizations)
#define IOCTL_FLARE_ADM_END_POLL \
    EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0x8A3, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

// Bind command (former page 20)
#define IOCTL_FLARE_ADM_BIND \
    EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0x8A4, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

// Unbind command (former page 21)
#define IOCTL_FLARE_ADM_UNBIND \
    EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0x8A5, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

// Verify command (former page 23)
#define IOCTL_FLARE_ADM_VERIFY \
    EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0x8A6, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

// Get verify report (former page 23)
#define IOCTL_FLARE_ADM_GET_VERIFY_REPORT \
    EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0x8A7, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

// Backdoor power command (former page 24)
#define IOCTL_FLARE_ADM_BACKDOOR_POWER \
    EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0x8A8, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

// Unit configuration command (former page 2B)
#define IOCTL_FLARE_ADM_UNIT_CONFIG \
    EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0x8A9, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

// ULOG control command (former page 2E)
#define IOCTL_FLARE_ADM_ULOG_CONTROL \
    EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0x8AA, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

// Retrieve ULOG records (former page 31)
#ifdef ALAMOSA_WINDOWS_ENV
#define IOCTL_FLARE_ADM_RETRIEVE_ULOG \
    EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0x8AB, EMCPAL_IOCTL_METHOD_IN_DIRECT, EMCPAL_IOCTL_FILE_ANY_ACCESS)
#else
#define IOCTL_FLARE_ADM_RETRIEVE_ULOG \
    EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0x8AB, EMCPAL_IOCTL_METHOD_OUT_DIRECT, EMCPAL_IOCTL_FILE_ANY_ACCESS)
#endif /* ALAMOSA_WINDOWS_ENV - ODDCASE - this should be OUT_DIRECT.   IN_DIRECT treats the output buffer as another input buffer.  here we're using the output buffer for output */

// Enclosure configuration command (former page 33)
#define IOCTL_FLARE_ADM_ENCLOSURE_CONFIG \
    EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0x8AC, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

// System configuration command (former page 37)
#define IOCTL_FLARE_ADM_SYSTEM_CONFIG \
    EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0x8AD, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

// Memory configuration command (former page 38)
#define IOCTL_FLARE_ADM_MEMORY_CONFIG \
    EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0x8AE, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

// RAID group configuration command (former page 3B)
#define IOCTL_FLARE_ADM_RAID_GROUP_CONFIG \
    EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0x8AF, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

// Receive diagnostic command (SCSI Receive Diagnostic)
#ifdef ALAMOSA_WINDOWS_ENV
#define IOCTL_FLARE_ADM_RECEIVE_DIAG \
    EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0x8B0, EMCPAL_IOCTL_METHOD_IN_DIRECT, EMCPAL_IOCTL_FILE_ANY_ACCESS)
#else
#define IOCTL_FLARE_ADM_RECEIVE_DIAG \
    EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0x8B0, EMCPAL_IOCTL_METHOD_OUT_DIRECT, EMCPAL_IOCTL_FILE_ANY_ACCESS)
#endif /* ALAMOSA_WINDOWS_ENV - ODDCASE - this should be OUT_DIRECT.   IN_DIRECT treats the output buffer as another input buffer.  here we're using the output buffer for output */

//Control Code 0x8B1 (IOCTL_FLARE_ADM_GET_SERIAL_NO) has been removed
//and obsoleted.  It have been replaced by IOCTL_FLARE_ADM_GET_SYSTEM_INFO

// Get unit cache status (former page 8)
#define IOCTL_FLARE_ADM_GET_UNIT_CACHE \
    EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0x8B2, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

// Set unit cache command (former page 8)
#define IOCTL_FLARE_ADM_SET_UNIT_CACHE \
    EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0x8B3, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

// Get poll data for a single LUN
#ifdef ALAMOSA_WINDOWS_ENV
#define IOCTL_FLARE_ADM_POLL_LUN \
    EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0x8B4, EMCPAL_IOCTL_METHOD_IN_DIRECT, EMCPAL_IOCTL_FILE_ANY_ACCESS)
#else
#define IOCTL_FLARE_ADM_POLL_LUN \
    EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0x8B4, EMCPAL_IOCTL_METHOD_OUT_DIRECT, EMCPAL_IOCTL_FILE_ANY_ACCESS)
#endif /* ALAMOSA_WINDOWS_ENV - ODDCASE - this should be OUT_DIRECT.   IN_DIRECT treats the output buffer as another input buffer.  here we're using the output buffer for output */

// Get poll data for all LUNs
#ifdef ALAMOSA_WINDOWS_ENV
#define IOCTL_FLARE_ADM_POLL_ALL_LUNS \
    EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0x8B5, EMCPAL_IOCTL_METHOD_IN_DIRECT, EMCPAL_IOCTL_FILE_ANY_ACCESS)
#else
#define IOCTL_FLARE_ADM_POLL_ALL_LUNS \
    EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0x8B5, EMCPAL_IOCTL_METHOD_OUT_DIRECT, EMCPAL_IOCTL_FILE_ANY_ACCESS)
#endif /* ALAMOSA_WINDOWS_ENV - ODDCASE - this should be OUT_DIRECT.   IN_DIRECT treats the output buffer as another input buffer.  here we're using the output buffer for output */

// Zero a disk
#define IOCTL_FLARE_ADM_ZERO_DISK \
    EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0x8B6, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

// Pass-through read
#define IOCTL_FLARE_ADM_PASS_THRU_READ \
    EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0x8B7, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

// Pass-through write
#define IOCTL_FLARE_ADM_PASS_THRU_WRITE \
    EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0x8B8, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

// Similar to unit config but only issues change bind portion
#define IOCTL_FLARE_ADM_UNIT_CHANGE_BIND \
    EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0x8B9, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

// NVK: adding Queue depth IOCTL. Some what of a misleading name, as we
// dont call the ADM once we reach NTFE. We straight away call the CM.
#define IOCTL_FLARE_ADM_SET_QUEUE_DEPTH \
    EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0x8BA, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

// Group wide verify command
#define IOCTL_FLARE_ADM_GROUP_VERIFY \
    EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0x8BB, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

// System wide verify command
#define IOCTL_FLARE_ADM_SYSTEM_VERIFY \
    EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0x8BC, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

// Get chassis serial number and cabinet ID (part of former page 37)
#define IOCTL_FLARE_ADM_GET_SYSTEM_INFO \
    EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0x8BD, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

// Reboot or Shutdown SP
#define IOCTL_FLARE_ADM_REBOOT_SP \
    EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0x8BE, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

// Proactive Spare Request
#define IOCTL_FLARE_ADM_PROACTIVE_SPARE \
    EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0x8BF, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

// LCC CTS Physical Layer Port Statistics
#define IOCTL_FLARE_ADM_LCC_STATS \
    EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK,0x8C0,EMCPAL_IOCTL_METHOD_BUFFERED,EMCPAL_IOCTL_FILE_ANY_ACCESS)


// LCC Fault Insertion
#define IOCTL_FLARE_ADM_LCC_CMD \
    EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0x8C1, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define IOCTL_FLARE_ADM_GET_YUKON_LOG   \
    EMCPAL_IOCTL_CTL_CODE (FILE_DEVICE_DISK, 0x8C2, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define IOCTL_FLARE_ADM_XPE_ERROR_STATS  \
    EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0x8C3, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

// Configure IO Ports
#define IOCTL_FLARE_ADM_SET_PORT_CONFIG \
    EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0x8C4, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

// Management Port Speed Configuration.
#define IOCTL_FLARE_ADM_CONFIG_MGMT_PORTS  \
    EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0x8C5, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)
// Mark IO Ports
#define IOCTL_FLARE_ADM_LED_CTRL \
    EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0x8C6, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

// Set an unresolved enclosure number
#define IOCTL_FLARE_ADM_ENCLOSURE_NUMBER \
    EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0x8C7, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

// Mamba/Bigfoot expander trace extraction from spcollect
#define IOCTL_FLARE_ADM_GET_EXPANDER_LOG \
    EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0x8C8, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)
// Power Saving configuration support.
#define IOCTL_FLARE_ADM_POWER_SAVING_CONFIG  \
    EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0x8C9, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

// LCC Frumon pause
#define IOCTL_FLARE_ADM_STOP_ENCL_UPGRADES \
    EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0x8CB, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

// LCC Frumon resume
#define IOCTL_FLARE_ADM_RESUME_ENCL_UPGRADES \
    EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0x8CC, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

// Upgrade SLICs
#define IOCTL_FLARE_ADM_SET_SLIC_UPGRADE \
    EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0x8D0, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

// Allocate CPU for latency sensitive command processing.
#define IOCTL_FLARE_ADM_USER_SPACE_PROC_NEEDS_CPU \
    EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0x8D1, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

// Deallocate CPU for latency sensitive command processing.
#define IOCTL_FLARE_ADM_USER_SPACE_PROC_CPU_NEED_COMPLETED \
    EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0x8D2, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

// Reset CPU throttling
#define IOCTL_FLARE_ADM_USER_SPACE_RESET_CPU_THROTTLE \
    EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0x8D3, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

// IOCTL to set or unset the CPU throttling feature
#define IOCTL_FLARE_SET_ENABLE_DISABLE_CPU_THROTTLE_FEATURE \
	EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0x8D4, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

// IOCTL to get the status of the CPU throttle whether it is turned on or turned off
#define IOCTL_FLARE_GET_ENABLE_DISABLE_CPU_THROTTLE_FEATURE \
	EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0x8D5, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

// IOCTL to set the NDU state
#define IOCTL_FLARE_ADM_SET_NDU_STATE \
	EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0x8D6, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

// IOCTL to get the bind configuration data
#define IOCTL_FLARE_ADM_BIND_GET_CONFIG \
	EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0x8D7, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

// MCR Config/Deconfig HS Request
#define IOCTL_ADM_CONFIGURE_SPARE \
    EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0x8D8, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

// MCR Copy a drive to a HS or unconfigured drive
#define IOCTL_ADM_COPY_DRIVE \
    EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0x8D9, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

// Get the port configuration status
#define IOCTL_ADM_GET_GENERAL_IO_PORT_STATUS \
    EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0x8DA, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)


// Sub-commands for Backdoor Power IOCTL
typedef enum adm_backdoor_power_sub_command_tag
{
    ADM_BACKDOOR_POWER_SUB_COMMAND_TAG_BBU,
    ADM_BACKDOOR_POWER_SUB_COMMAND_TAG_SP,
    ADM_BACKDOOR_POWER_SUB_COMMAND_TAG_DISK
}
ADM_BACKDOOR_POWER_SUB_COMMAND;

// Disk power request types
typedef enum adm_fru_power_tag
{   

    /* simulate drive removal (hard bypass) */
    ADM_FRU_POWER_SIM_PHY_REMOVAL,   
    ADM_FRU_POWER_SIM_PHY_INSERT,    

    /* simulate flare CM bypass (soft bypass) */
    ADM_FRU_POWER_SOFT_BYPASS,
    ADM_FRU_POWER_SOFT_UNBYPASS,

    /* simulate drive timeout failure (drive remap) */
    ADM_FRU_POWER_DH_DOWN,
    ADM_FRU_POWER_DH_UP,

    /* Power related operations */ 
    ADM_FRU_POWER_OFF,  
    ADM_FRU_POWER_ON,   
    ADM_FRU_POWER_CYCLE, 
    ADM_FRU_POWER_RESET,  

    /* values for range checking */
    ADM_FRU_POWER_MIN = ADM_FRU_POWER_SIM_PHY_REMOVAL,   
    ADM_FRU_POWER_MAX = ADM_FRU_POWER_RESET
} ADM_FRU_POWER;


#ifndef ALAMOSA_WINDOWS_ENV
#pragma pack(4)
#endif /* ALAMOSA_WINDOWS_ENV - PADMESS - packing mismatches */
// Opcode for RAID Group configuration IOCTL
typedef enum adm_rg_config_opcode_tag
{
    ADM_CREATE_RAID_GROUP,
    ADM_REMOVE_RAID_GROUP,
    ADM_EXPAND_RAID_GROUP,
    ADM_DEFRAG_RAID_GROUP,
    ADM_CONFIG_RAID_GROUP
}
ADM_RG_CONFIG_OPCODE;
#ifndef ALAMOSA_WINDOWS_ENV
#pragma pack()
#endif /* ALAMOSA_WINDOWS_ENV - PADMESS - packing mismatches */

// Expansion rates
typedef enum adm_rg_exp_rate_tag
{
    ADM_RG_EXP_RATE_LOW,
    ADM_RG_EXP_RATE_MEDIUM,
    ADM_RG_EXP_RATE_HIGH,
    ADM_RG_EXP_RATE_INVALID = UNSIGNED_MINUS_1
}
ADM_RG_EXP_RATE;

// Receive Diagnostics Page Codes
typedef enum adm_receive_diag_page_code_tag
{
    ADM_RECEIVE_DIAG_PAGE_CODE_FLARE = 0x80
}
ADM_RECEIVE_DIAG_PAGE_CODE;

/*
 * Constants for Receive Diagnostic IOCTL
 */
#define ADM_RCV_DIAG_PARAM_COUNT        2
#define ADM_RCV_DIAG_PARAM_SIZE         8
#define ADM_RCV_DIAG_PARAM_DATA_SIZE    4
#define ADM_RCV_DIAG_FEATURE_COUNT      1

#define ADM_RECEIVE_DIAG_VENDOR_ID_SIZE 8
#define ADM_RECEIVE_DIAG_PROD_REV_SIZE  4
#define ADM_RECEIVE_DIAG_PAGE_LENGTH    42


/*
 * Constants for pass-through I/O
 */
#define ADM_MAX_PASS_THRU_READ_LENGTH   1
#define ADM_MAX_PASS_THRU_WRITE_LENGTH  1
#define ADM_SCSI_10_BYTE_XFER_LENGTH    7

/*
 * Constants for pass-through I/O Error-injection
 */
#define ADM_MAX_PASS_THRU_READ_LENGTH_EI   512
#define ADM_MAX_PASS_THRU_WRITE_LENGTH_EI  512

/*
 * Defines for ULOG
 */

// Unsolicited Sense Block Event Codes
#define ADM_RS_ERR_CODE             0x70
#define ADM_UNSOL_ERR_CODE          0x71

// Number of extended bytes
#define ADM_SENSE_EXTENDED_CODES    2

// Maximum length of ULOG text
#define ADM_ULOG_MAX_TEXT_LENGTH    100

// ULOG pointer value for end-of-log
#define ADM_ULOG_POINTER_AT_END     0xffffffff

/*
 * Defines for pass-through
 */
#define ADM_MAX_SCSI_CDB_SIZE             12

/*
 * Defines for LCC Fault Insertion
 */
#define ADM_LCC_READ_SCSI_CDB_SIZE             64
#define ADM_LCC_EXT_READ_SCSI_CDB_SIZE         64
#define ADM_LCC_MASK_STATUS_SCSI_CDB_SIZE       4
#define ADM_LCC_CM_PROG_REV_SIZE                5
#define LCC_FI_MASK_STATUS_OVERRIDE             0
#define LCC_FI_MASK_REGISTER_SELECT             1
#define LCC_FI_MASK_BIT_AND_MASK                2
#define LCC_FI_MASK_BIT_OR_MASK                 3
#define MAX_SLEEP_VALUE                       255

/*
 * Default values for fields in IOCTLs
 */
#define ADM_INVALID_ADDRESS_OFFSET  UNSIGNED_64_MINUS_1
#define ADM_CACHE_CONFIG_NO_CHANGE  UNSIGNED_MINUS_1
#define ADM_CACHE_IDLE_THRESHOLD_NO_CHANGE  UNSIGNED_MINUS_1
#define ADM_CACHE_BACKEND_REQ_SIZE_NO_CHANGE  UNSIGNED_MINUS_1
#define ADM_CACHE_IDLE_DELAY_NO_CHANGE  UNSIGNED_MINUS_1
#define ADM_CACHE_WRITE_ASIDE_NO_CHANGE  UNSIGNED_MINUS_1
#define ADM_CACHE_READ_RETENTION_NO_CHANGE  UNSIGNED_MINUS_1
#define ADM_CACHE_PREFETCH_SEG_LEN_NO_CHANGE  UNSIGNED_MINUS_1
#define ADM_CACHE_PREFETCH_TOT_LEN_NO_CHANGE  UNSIGNED_MINUS_1
#define ADM_CACHE_PREFETCH_MAX_LEN_NO_CHANGE  UNSIGNED_MINUS_1
#define ADM_CACHE_PREFETCH_DIS_LEN_NO_CHANGE  UNSIGNED_MINUS_1
#define ADM_CACHE_PREFETCH_IDLE_CNT_NO_CHANGE  UNSIGNED_MINUS_1
#define ADM_CACHE_PREFETCH_DEFAULT_NO_CHANGE FALSE 
#define ADM_UNIT_REBUILD_PRIORITY_NO_CHANGE  UNSIGNED_MINUS_1
#define ADM_UNIT_ZERO_THROTTLE_RATE_NO_CHANGE  UNSIGNED_MINUS_1
#define ADM_UNIT_ATTRIBUTES_NO_CHANGE  UNSIGNED_MINUS_1
#define ADM_UNIT_VERIFY_PRIORITY_NO_CHANGE  UNSIGNED_MINUS_1
#define ADM_UNIT_SNIFF_RATE_NO_CHANGE UNSIGNED_MINUS_1
#define ADM_UNIT_WWN_NO_CHANGE UNSIGNED_MINUS_1
#define ADM_SYSCONFIG_CACHE_PG_SIZE_NO_CHANGE  UNSIGNED_MINUS_1
#define ADM_SYSCONFIG_SYSTEM_TYPE_NO_CHANGE  UNSIGNED_MINUS_1
#define ADM_SYSCONFIG_LOW_WMARK_NO_CHANGE  UNSIGNED_MINUS_1
#define ADM_SYSCONFIG_HIGH_WMARK_NO_CHANGE  UNSIGNED_MINUS_1
#define ADM_SYSCONFIG_SPS_DAY_NO_CHANGE  UNSIGNED_MINUS_1
#define ADM_SYSCONFIG_SPS_HOUR_NO_CHANGE  UNSIGNED_MINUS_1
#define ADM_SYSCONFIG_SPS_MINUTE_NO_CHANGE  UNSIGNED_MINUS_1
#define ADM_SYSCONFIG_SHARED_CONVERSION_MEMORY_INFO_NO_CHANGE UNSIGNED_64_MINUS_1
#define ADM_MEMCONFIG_CACHE_SIZE_NO_CHANGE  UNSIGNED_MINUS_1
#define ADM_UNIT_CONFIG_DEFAULT_LU_NAME_CHR (0xff)
#define ADM_POWER_SAVING_LATENCY_TIME_NO_CHANGE  UNSIGNED_64_MINUS_1
#define ADM_POWER_SAVING_IDLE_TIME_NO_CHANGE  UNSIGNED_64_MINUS_1



/************
 *  Types
 ************/

// Output data from Initialize IOCTL
typedef struct adm_initialize_tag
{
    UINT_32 buffer_size;        // Buffer size needed for poll data
}
ADM_INITIALIZE;


// Perform a poll
typedef struct adm_poll_tag
{
    PVOID CSX_ALIGN_N(8) output_buffer;  // Used to calculate addresses within the output buffer
    BOOL  CSX_ALIGN_N(8) full_poll;      // buffer has not been filled previously

}
ADM_POLL;

// Input data for Bind IOCTL
typedef struct adm_bind_cmd_struct
{
    LU_BIND_INFO    bind_info;
    BOOL            non_destructive_bind;
    UINT_32         ndb_password;
    BOOL            fix_lun;
#ifndef ALAMOSA_WINDOWS_ENV
    csx_u32_t       pad;
#endif /* ALAMOSA_WINDOWS_ENV - PADMESS - packing mismatches */
}
ADM_BIND_CMD;

// Output data for Bind IOCTL
typedef struct adm_bind_data_struct
{
    UINT_32     lun;
    ADM_STATUS  status;
}
ADM_BIND_DATA;

// Input data for Unbind IOCTL
typedef struct adm_unbind_cmd_struct
{
    K10_WWID lu_wwn;
}
ADM_UNBIND_CMD;

// Status output for Unbind IOCTL
typedef struct adm_unbind_data_struct
{
    ADM_STATUS  status;
}
ADM_UNBIND_DATA;

// Input data for a Proactive Spare IOCTL
typedef struct adm_proactive_cmd_struct
{
    UINT_32 proactive_candidate;
}
ADM_PROACTIVE_SPARE_CMD;

// Status output for Proactive Spare IOCTL
typedef struct adm_proactive_spare_data_struct
{
    ADM_STATUS  status;
}
ADM_PROACTIVE_SPARE_DATA;

// Input data for Verify IOCTL
typedef struct adm_verify_cmd_struct
{
    K10_WWID    wwid;
    TRI_STATE   sniff_verify;
    BOOL        background_verify;
    BOOL        clear_reports;
    UINT_32     sniff_rate;         // 1-254 (0 or 0xff for no change)
    UINT_32     bg_verify_priority; // LU background verify priority
}
ADM_VERIFY_CMD;

// Input data for group verify and system verify IOCTL
typedef struct adm_group_verify_cmd_struct
{
    UINT_32     group;
    TRI_STATE   sniff_verify;
    BOOL        background_verify;
    BOOL        clear_reports;
    UINT_32     sniff_rate;
    UINT_32     bg_verify_priority;
    TRI_STATE   user_sniff_control_enabled; /* Only looked at for the 
                                               IOCTL_FLARE_ADM_SYSTEM_VERIFY IOCTL. 
                                               If set to anything except TRI_INVALID 
                                               all other fields will be ignored. */
}
ADM_GROUP_VERIFY_CMD;

// Status output for Verify IOCTL
typedef struct adm_verify_status_struct
{
    ADM_STATUS  status;
}
ADM_VERIFY_STATUS;

// Input data for Get Verify Report IOCTL
typedef struct adm_verify_rpt_cmd_struct
{
    K10_WWID    wwid;       // WWID of LUN
}
ADM_VERIFY_REPORT_CMD;

// Output data for Get Verify Report IOCTL
typedef struct adm_verify_rpt_struct
{
    VERIFY_RESULTS_REPORT   nonvol;
    VERIFY_RESULTS_REPORT   curr_background;
    VERIFY_RESULTS_REPORT   most_recent_background;
    VERIFY_RESULTS_REPORT   total_all_background;
    ADM_STATUS              status;
}
ADM_VERIFY_REPT;

// Input data for Backdoor Power IOCTL
typedef struct adm_backdoor_power_cmd_struct
{
    ADM_BACKDOOR_POWER_SUB_COMMAND  sub_cmd;

    union
    {
        BOOL        bbu_power_on;
        struct
        {
            BOOL    delay;
            UINT_32 lun;
        } sp_panic;
        struct
        {
            UINT_32         fru_number;
            ADM_FRU_POWER   power_on;
            BOOL            defer;
        } fru_power;
    } backdoor_options;
}
ADM_BACKDOOR_POWER_CMD;

// Status output for Backdoor Power IOCTL
typedef struct adm_backdoor_power_status_struct
{
    ADM_STATUS  status;
}
ADM_BACKDOOR_POWER_STATUS;

#ifndef ALAMOSA_WINDOWS_ENV
#pragma pack(4)
#endif /* ALAMOSA_WINDOWS_ENV - PADMESS - packing mismatches */
// Input data for Unit Configuration IOCTL
typedef struct adm_unit_config_cmd_struct
{
    K10_WWID                    wwid;
    UNIT_CACHE_INFO             unit_cache_info;
    BOOL                        default_prefetch;
    LU_BIND_INFO                unit_bind_info;
}
ADM_UNIT_CONFIG_CMD;
#ifndef ALAMOSA_WINDOWS_ENV
#pragma pack()
#endif /* ALAMOSA_WINDOWS_ENV - PADMESS - packing mismatches */

// Status output for Unit Configuration IOCTL
typedef struct adm_unit_config_status_struct
{
    ADM_STATUS  status;
}
ADM_UNIT_CONFIG_STATUS;

// Input data for ULOG Control IOCTL
typedef struct adm_ulog_control_cmd_struct
{
    BOOL            clear_log;
    UINT_32         seek_record;
}
ADM_ULOG_CONTROL_CMD;

// Output data for ULOG Control IOCTL
typedef struct adm_ulog_control_data_struct
{
    UINT_32         current_pointer;
    UINT_32         total_count;
    ADM_STATUS      status;
}
ADM_ULOG_CONTROL_DATA;

/* These values are the CRU type values for the cru_type field of
 * unsolicited request sense blocks.
 */
#define ADM_SMU_DISK             0
//      ADM_SMU_CHASSIS      was 1, now obsolete
#define ADM_SMU_FAN              2
#define ADM_SMU_VSC              3              // Power supply
#define ADM_SMU_PERIPHERAL_CONTROLLER 4         // SP
//      ADM_SMU_BBU          was 5, now obsolete
//      ADM_SMU_TAPE         was 6, now obsolete
#define ADM_SMU_UNDEFINED        7
//      ADM_SMU_PCMCIA       was 8, now obsolete
#define ADM_SMU_LCC              9
#define ADM_SMU_SPS              10
#define ADM_SMU_ENCL             11


// Unsolicited Sense Block
typedef struct adm_sense_block_struct
{
    UINT_8E         event_code;
    UINT_8E         sense_key; 
    UINT_16E        sunburst_code;
    UINT_16E        raid_group_id;
    UINT_8E         cru_type;
    UINT_8E         enclosure_id;
    UINT_8E         slot_number;
    UINT_8E         additional_sense_code;
    UINT_8E         additional_sense_code_qualifier;
    UINT_16E        cru_number;
    UINT_8E         bus;
    UINT_8E         ulog_status_code;
    UINT_32         extended_data[ADM_SENSE_EXTENDED_CODES];
}
ADM_SENSE_BLOCK;

// ULOG record
typedef struct adm_ulog_record_struct
{
    ADM_SENSE_BLOCK unsol_sense_block;
    UINT_32         year;
    UINT_32         month;
    UINT_32         day;
    UINT_32         weekday;
    UINT_32         hour;
    UINT_32         minute;
    UINT_32         second;
    TEXT            text_description[ADM_ULOG_MAX_TEXT_LENGTH];
}
ADM_ULOG_RECORD;

// Input data for Retrieve ULOG IOCTL
typedef struct adm_retrieve_ulog_cmd_struct
{
    UINT_32         record_count;   // count of ULOG records to retrieve
}
ADM_RETRIEVE_ULOG_CMD;

// Output data for Retrieve ULOG IOCTL
typedef struct adm_retrieve_ulog_data_struct
{
    UINT_32         records_read;       // count actually returned
    UINT_32         current_pointer;    // after read
    ADM_STATUS      status;
    UINT_32         total_count;        // number of records available
    ADM_ULOG_RECORD ulog_buffer[1];     // returned records (placeholder)
}
ADM_RETRIEVE_ULOG_DATA;

// Input data for Enclosure Configuration IOCTL
typedef struct adm_enclosure_config_cmd_struct
{
    UINT_32         enclosure;
    BOOL            flash_enable;
    ADM_FLASH_BITS  flash_bits;
}
ADM_ENCLOSURE_CONFIG_CMD;

// Status output for Enclosure Configuration IOCTL
typedef struct adm_enclosure_config_status_struct
{
    ADM_STATUS  status;
}
ADM_ENCLOSURE_CONFIG_STATUS;

// Input data for set enclosure number IOCTL
typedef struct adm_encl_number_set_cmd_struct
{
    UINT_64 sas_addr;
    UINT_32 encl_num;
}
ADM_ENCL_NUMBER_SET_CMD;

// Status output for Enclosure Configuration IOCTL
typedef struct adm_enclosure_number_status_struct
{
    ADM_STATUS  status;
}
ADM_ENCLOSURE_NUMBER_STATUS;

typedef union adm_led_ctrl_info_struct
{
    struct
    {
        UINT_32         ioModuleNum;
        UINT_32         ioPortNum;
        BOOL            markPortOn;
        IO_MODULE_CLASS iom_class;
    } portLedInfo;

    struct
    {
        UINT_32         ioModuleNum;
        BOOL            markModuleOn;
        BOOL            markModuleFaultOn;
        IO_MODULE_CLASS iom_class;
    } moduleLedInfo;

} 
ADM_LED_CTRL_INFO;

typedef struct adm_led_ctrl_cmd_struct
{
    LED_ID_TYPE         ledIdType;
    ADM_LED_CTRL_INFO   ledControlInfo;
}
ADM_LED_CTRL_CMD;

// Status output for LED Control IOCTL
typedef struct adm_led_ctrl_status_struct
{
    ADM_STATUS  status;
}
ADM_LED_CTRL_STATUS;

// Input data for LCC Stats IOCTL
typedef struct adm_lcc_stats_struct
{
    UINT_32 encl_addr;
    UINT_32 lcc_number;
}
ADM_LCC_STATS_CMD;

// Status output for LCC Stats IOCTL


typedef struct adm_lcc_stats_data_struct
{
    UINT_16 port_type;
    UINT_16 retimer_and_monitor_configuration;
    UINT_16 retimer_and_monitor_status;
    UINT_32 retimer_and_monitor_LCV_error_count;
    UINT_32 retimer_and_monitor_CRC_error_count;
    UINT_16 lip_count;
    UINT_8 expansionA;
    UINT_16 expansionB;
    UINT_16 expansionC;
    UINT_8 expansionD;
}
ADM_LCC_STATS_DATA;

typedef struct adm_lcc_stats_returned_data_struct
{
    UINT_16 number_of_ports;
    ADM_LCC_STATS_DATA data[NUMBER_OF_LCC_PORTS];
    ADM_STATUS status;    
}
ADM_LCC_STATS_RETURNED_DATA;

// XPE error stats command structures
typedef struct adm_xpe_error_stats_struct
{
  UINT_32 port_number;
}
ADM_XPE_ERROR_STATS_CMD;

typedef struct adm_xpe_error_stats_data_struct
{
  UINT_32 link_failure_count;
  UINT_32 loss_of_sync_count;
  UINT_32 loss_of_signal_count;
  UINT_32 prim_seq_error_count;
  UINT_32 invalid_xmit_word_count;
  UINT_32 invalid_crc_count;
}
ADM_XPE_ERROR_STATS_DATA;

typedef struct adm_xpe_error_stats_returned_data_struct
{
  ADM_XPE_ERROR_STATS_DATA data;
  ADM_STATUS status;
}
ADM_XPE_ERROR_STATS_RETURNED_DATA;

// Input data for Yukon Log Retrieval IOCTL
typedef struct adm_get_yukon_log_cmd_struct
{
    UINT_32         enclosure;
}
ADM_GET_YUKON_LOG_CMD;

// Status output for Yukon Log Retrieval IOCTL
typedef struct adm_get_yukon_log_status_struct
{
    ADM_STATUS  status;
}
ADM_GET_YUKON_LOG_STATUS;

#ifndef ALAMOSA_WINDOWS_ENV
#pragma pack(4)
#endif /* ALAMOSA_WINDOWS_ENV - PADMESS - packing mismatches */
// Input data for System Configuration IOCTL
typedef struct adm_system_config_cmd_struct
{
    BOOL        change_serial_number;
    TEXT        serial_number[ADM_EMC_SERIAL_NO_SIZE];
    UINT_32     cache_page_size;
    TRI_STATE   statistics_logging;
    UINT_32     system_type;
    TRI_STATE   drive_type_checking;        // not currently used
    UINT_32     clear_cache_dirty_lun;
    UINT_32     clear_cache_dirty_password;
    UINT_32     low_watermark;
    UINT_32     high_watermark;
    UINT_32     sps_test_day;
    UINT_32     sps_test_hour;
    UINT_32     sps_test_minute;
    TRI_STATE   write_cache_enable;
    TRI_STATE   read_cache_enable_spA;
    TRI_STATE   read_cache_enable_spB;
    TRI_STATE   vault_fault_override;
    BOOL        change_wwn_seed;
    UINT_32     wwn_seed;
    BOOL        change_part_number;
    TEXT        part_number[ADM_EMC_PART_NUMBER_SIZE];
    BOOL        change_sys_orientation;
    TEXT        system_orientation[ADM_SYSTEM_ORIENTATION_SIZE];
    BOOL        time_sync;
    BOOL        system_bus_reset;
    TRI_STATE   wca_state;
    BOOL        erase_bbu_card;
    UINT_32     erase_bbu_card_password;        //0xdeadbeef
    BOOL        set_shared_expected_memory_info;
    OPAQUE_64   shared_expected_memory_info;

}
ADM_SYSTEM_CONFIG_CMD;
#ifndef ALAMOSA_WINDOWS_ENV
#pragma pack()
#endif /* ALAMOSA_WINDOWS_ENV - PADMESS - packing mismatches */

// Status output for System Configuration IOCTL
typedef struct adm_system_config_status_struct
{
    ADM_STATUS  status;
}
ADM_SYSTEM_CONFIG_STATUS;

// Input data for Memory Configuration IOCTL
typedef struct adm_memory_config_cmd_struct
{
    UINT_32 write_cache_size;
    UINT_32 read_cache_spa_size;
    UINT_32 read_cache_spb_size;
    UINT_32 hi5_cache_size;
}
ADM_MEMORY_CONFIG_CMD;

// Status output for Memory Configuration IOCTL
typedef struct adm_memory_config_status_struct
{
    ADM_STATUS  status;
}
ADM_MEMORY_CONFIG_STATUS;

#ifndef ALAMOSA_WINDOWS_ENV
#pragma pack(4)
#endif /* ALAMOSA_WINDOWS_ENV - PADMESS - packing mismatches */
// Input data for RAID Group Configuration IOCTL
typedef struct adm_raid_group_config_cmd_struct
{
    ADM_RG_CONFIG_OPCODE    opcode;
    BOOL                    extend_length;
    TRI_STATE               explicit_remove;
    UINT_32                 raid_group_id;  // INVALID_RAID_GROUP to let
                                            // Flare pick
    UINT_32                 disk_count;
    UINT_32                 fru_list[MAX_DISK_ARRAY_WIDTH];
    ADM_RG_EXP_RATE         expansion_rate;
    UINT_32                 unit_type;      // RG unit type
    TRI_STATE               is_private;     // flagged by Admin/Navi as private
    UINT_64                 idle_time_for_standby;  // Time RG must be idle 
                                                    // before it is put in standby

    // if 0, RG can't be put in standby even if it is power_saving_capable.
    UINT_64                 latency_time_for_becoming_active;
    
    UINT_32                 element_size;
}
ADM_RAID_GROUP_CONFIG_CMD;
#ifndef ALAMOSA_WINDOWS_ENV
#pragma pack()
#endif /* ALAMOSA_WINDOWS_ENV - PADMESS - packing mismatches */

// Status output for RAID Group Configuration IOCTL
typedef struct adm_raid_group_config_status_struct
{
    ADM_STATUS  status;
}
ADM_RAID_GROUP_CONFIG_STATUS;


// Input data for Receive Diagnostic IOCTL
typedef struct adm_receive_diag_cmd_struct
{
    UINT_32                     param_list_length;
    ADM_RECEIVE_DIAG_PAGE_CODE  page_code;
    UINT_32                     fru;
}
ADM_RECEIVE_DIAG_CMD;


#pragma pack(1)

/*
 * Output structures for Receive Diagnostic IOCTL
 */
typedef struct adm_rcv_diag_flare_param_struct
{
    UINT_16E param_id;
    UINT_16E param_sub_id;
    UINT_8E  param_data[ADM_RCV_DIAG_PARAM_DATA_SIZE];
}
ADM_RCV_DIAG_FLARE_PARAM;

typedef struct adm_rcv_diag_flare_feat_struct
{
    UINT_16E feature_id;
    UINT_16E feature_sub_id;
    UINT_8E  reserved;
    UINT_16E param_count;
    UINT_8E  param_size;
    ADM_RCV_DIAG_FLARE_PARAM   params[ADM_RCV_DIAG_PARAM_COUNT];
}
ADM_RCV_DIAG_FLARE_FEAT;

typedef struct adm_receive_diag_flare_page_struct
{
    UINT_8E  page_code;            /* 0x80 */
    UINT_8E  reserved;
    UINT_16E page_length;          /* in bytes */
    UINT_8E  vendor_id[ADM_RECEIVE_DIAG_VENDOR_ID_SIZE];
    UINT_8E  product_rev[ADM_RECEIVE_DIAG_PROD_REV_SIZE];
    UINT_16E feature_count;        /* # of feature desc (curr. == 1)*/
    ADM_RCV_DIAG_FLARE_FEAT  features[ADM_RCV_DIAG_FEATURE_COUNT];
}
ADM_RECEIVE_DIAG_FLARE_PAGE;
#pragma pack()

typedef struct adm_receive_diag_data_struct
{
    ADM_STATUS                      status;
    union
    {
        ADM_RECEIVE_DIAG_FLARE_PAGE flare_page;

        // no other page codes supported yet
    } page_data;
}
ADM_RECEIVE_DIAG_DATA;

// Output data for Get Serial Number IOCTL
typedef struct adm_get_system_info_data_struct
{
    UINT_8E     system_serial_number[ADM_EMC_SERIAL_NO_SIZE];
    UINT_8E     cabinet_id;
    ADM_STATUS  status;
}
ADM_GET_SYSTEM_INFO_DATA;

// Output data for Get Unit Cache IOCTL
typedef struct adm_get_unit_cache_data_struct
{
    BOOL        write_cache_enabled; // TRUE if unit write cache enabled
    BOOL        read_cache_enabled;  // TRUE if unit read cache enabled
}
ADM_GET_UNIT_CACHE_DATA;

// Input data for Set Unit Cache IOCTL
typedef struct adm_set_unit_cache_cmd_struct
{
    TRI_STATE   write_cache_state;  // TRUE enables unit write cache
                                    // FALSE disables unit write cache
                                    // INVALID leaves state unchanged
    TRI_STATE   read_cache_state;   // TRUE enables unit read cache
                                    // FALSE disables unit read cache
                                    // INVALID leaves state unchanged
}
ADM_SET_UNIT_CACHE_CMD;

// Input data for Poll LUN IOCTL
typedef struct adm_poll_lun_cmd_struct
{
    PVOID       CSX_ALIGN_N(8) output_buffer; // Used to calculate addresses within the output buffer
    K10_WWID    CSX_ALIGN_N(8) wwid;
}
ADM_POLL_LUN_CMD;

// Input data for Zero Disk IOCTL
typedef struct adm_poll_zero_disk_struct
{
    UINT_32 fru;
    BOOL    stop_zero;
}
ADM_ZERO_DISK_CMD;

// Status output for Zero Disk IOCTL
typedef struct adm_zero_disk_status_struct
{
    ADM_STATUS  status;
}
ADM_ZERO_DISK_STATUS;


// Pass-through read command
typedef struct adm_pass_thru_read_cmd_struct
{
    UINT_32 fru;
    UINT_8E cdb[ADM_MAX_SCSI_CDB_SIZE];
}
ADM_PASS_THRU_READ_CMD;

// Pass-through read data
typedef struct adm_pass_thru_read_data_struct
{
    ADM_STATUS  status;
    UINT_8E     condition_code;
    UINT_8E     sense_key;
    UINT_8E     additional_sense_code;
    UINT_8E     additional_sense_code_qualifier;
    UINT_8E     data[BE_BYTES_PER_BLOCK];
}
ADM_PASS_THRU_READ_DATA;

// Pass-through write command
typedef struct adm_pass_thru_write_cmd_struct
{
    UINT_32 fru;
    UINT_8E cdb[ADM_MAX_SCSI_CDB_SIZE];
    UINT_8E data[BE_BYTES_PER_BLOCK];
}
ADM_PASS_THRU_WRITE_CMD;

// Pass-through write data
typedef struct adm_pass_thru_write_data_struct
{
    ADM_STATUS  status;
    UINT_8E     condition_code;
    UINT_8E     sense_key;
    UINT_8E     additional_sense_code;
    UINT_8E     additional_sense_code_qualifier;
}
ADM_PASS_THRU_WRITE_DATA;

// reboot SP command
typedef struct adm_reboot_sp_cmd_struct
{
    ADM_SP_ID   sp_id;
    UINT_32     options;
}
ADM_REBOOT_SP_CMD;

// ADM_REBOOT_SP_CMD options definitions
#define ADM_REBOOT_SP_OPTIONS_NONE          0x0
#define ADM_REBOOT_SP_OPTIONS_HOLD_IN_POST  0x1
#define ADM_REBOOT_SP_OPTIONS_HOLD_IN_RESET 0x2
#define ADM_REBOOT_SP_OPTIONS_POWEROFF      0x3
#define ADM_REBOOT_SP_OPTIONS_HOLD_IN_POWER_OFF 0x4
#define ADM_REBOOT_SP_OPTIONS_HOLD_IN_POST_POFF_SLICS 0x8

// reboot SP data
typedef struct adm_reboot_sp_status_struct
{
    ADM_STATUS  status;
}
ADM_REBOOT_SP_STATUS;

// commands for LCC FRUMON Fault Insertion IOCTL
typedef enum adm_lcc_cmd_tag
{
    ADM_LCC_CMD_MIN = 0, 
    ADM_LCC_CMD_FRU_RESET = ADM_LCC_CMD_MIN,
    ADM_LCC_CMD_FRU_POWERDOWN,
    ADM_LCC_CMD_FRU_POWER_OFF,
    ADM_LCC_CMD_FRU_POWERUP,
    ADM_LCC_CMD_READ,
    ADM_LCC_CMD_EXT_READ,
    ADM_LCC_CMD_SET_MASK,
    ADM_LCC_CMD_GET_MASK,
    ADM_LCC_CMD_RESET_LCC,
    ADM_LCC_CMD_FRUMON_REVISION,
    ADM_LCC_CMD_SET_SPEED_CHANGE,
    ADM_LCC_CMD_GET_SPEED,
    ADM_LCC_CMD_DISABLE_DIPLEX,
    ADM_LCC_CMD_ENABLE_DIPLEX,
    ADM_LCC_CMD_ENCL_SLEEP,
    ADM_LCC_CMD_MAX
} ADM_LCC_CMD;

// Requests for the LCC FRUMON Fault insertion IOCTL
typedef enum lcc_apply_control_page_tag
{
    LCC_APPLY_CTRL_PAGE_RESET_LCC = 0,
    LCC_APPLY_CTRL_PAGE_CHANGE_SPEED_AND_RESET,
    LCC_APPLY_CTRL_PAGE_MISCELLANEOUS,
    LCC_APPLY_CTRL_PAGE_ENCLOSURE_MAP,
    LCC_APPLY_CTRL_PAGE_ENCLOSURE_SLEEP = 11
} LCC_APPLY_CONTROL_PAGE;

typedef struct lcc_mask_request
{
    UINT_8E status_override_subcode;
    UINT_8E reg_select;
    UINT_8E and_mask;
    UINT_8E or_mask;
} LFI_MASK_REQUEST;

typedef struct lcc_mask_reply
{
    UINT_8E and_mask;
    UINT_8E or_mask;
} LFI_MASK_REPLY;

// input/output for LCC Fault Insertion IOCTL
typedef struct adm_lcc_data
{
    /* user level request type */
    ADM_LCC_CMD  u_request;

    /* defines if request is async or synch type */
    UINT_8       cmd_type;

    /* bus id where enclosure resides */
    UINT_8       bus_num;

    /* user defined enclosure id */
    UINT_8       u_encl_addr;

    /* CM defined enclosure id */
    UINT_8       cm_encl_addr;

    /* request type for CM */
    UINT_8       cm_lcc_request;

    /* request type for CM */
    UINT_8       req_initiated;

    /* defines specific LCC error type */
    UINT_32      error;

    /* status back for layers above Flare */  
    ADM_STATUS  status;

    union
    {
        UINT_8   frumon_rev;

        struct
        {
            UINT_8E fru_num;
        } fru_resetdown;

        struct
        {
            BITS_64E drive_bit_mask;
        } fru_onoff;

        LFI_MASK_REQUEST request_mask;

        struct
        {   
            LCC_APPLY_CONTROL_PAGE  ctl_page;
            UINT_8E    misc_page;
            UINT_8E    speed;
            UINT_8E    sleep_second;
        } apply_control;

    } request;

    union
    {
        LFI_MASK_REPLY reply_mask;

        struct
        {
            UINT_8E   cdb[ADM_LCC_READ_SCSI_CDB_SIZE];
        } read;

        struct
        {
            UINT_8E   cdb[ADM_LCC_EXT_READ_SCSI_CDB_SIZE];
        } ext_read;

        struct
        {
            UINT_8  rev[ADM_LCC_CM_PROG_REV_SIZE];
        } frumon_revision;

        UINT_8   ret_speed_change;

    } reply;

} ADM_LCC_DATA;

// This is a special (broadcast) value used in the ADM_LCC_DATA struct
// for fru_resetdown.fru_num.  It is an indication to the FRUMON that
// the command coming down is for ALL drives in the enclosure. So 
// if you want to send a command to all drives you use this fru_num 
// instead of sending 15 individual commands (i.e. one for each fru)
#define POWER_CMD_FOR_ALL_DRIVES    0xA5

// Input data for management ports configuration IOCTL
typedef struct adm_config_mgmt_ports_cmd_struct
{
    MGMT_PORT_AUTO_NEG     mgmt_port_auto_neg;
    MGMT_PORT_SPEED        mgmt_port_speed;
    MGMT_PORT_DUPLEX_MODE  mgmt_port_duplex_mode;
} ADM_CONFIG_MGMT_PORTS_CMD;

typedef struct adm_config_mgmt_ports_status_struct
{
    ADM_STATUS  status;
}
ADM_CONFIG_MGMT_PORTS_STATUS;

// adm set port config data
typedef struct adm_port_data_struct
{
    UINT_32         slot;
    UINT_32         port;
    UINT_32         logical_num;
    IOM_GROUP       iom_group;
    PORT_ROLE       port_role;
    PORT_SUBROLE    port_subrole;
    IO_MODULE_CLASS iom_class;
} ADM_PORT_DATA;

// adm set port config command
typedef struct adm_port_config_data_struct
{
    UINT_32         num_ports;
    BOOL            reset_all_ports;
    BOOL            reboot_peer;
    BOOL            disable_port;
    ADM_PORT_DATA   adm_port_data[MAX_IO_PORTS_PER_SP];
} ADM_PORT_CONFIG_DATA;

// adm config io port status
typedef struct adm_config_io_port_status_struct
{
    ADM_STATUS  status;
}
ADM_CONFIG_IO_PORT_STATUS;

// @struct ADM_POWER_SAVING_CONFIG_CMD
// @brief Input data for Power Saving Configuration IOCTL
//
typedef struct adm_power_saving_config_data_struct
{
    // Default time RG must be idle before it is put in standby
    UINT_64 idle_time_for_standby;

    // Default time RG can delay I/O due to being in standby
    UINT_64 latency_time_for_becoming_active;

    // Time a disk can remain in standby before it is woken up
    // so that diagnostics can run for a period equal to 
    // idle_time_for_standby.
    UINT_32 drive_health_check_time;

    // Used to toggle the Power Saving feature, IF it is 
    // available.
    TRI_STATE system_power_saving_state;

    // Used to toggle Power Saving statistics logging.  
    // The 3rd state is "no change".    
    TRI_STATE  power_saving_stats_enabled;
}
ADM_POWER_SAVING_CONFIG_CMD;

// @struct ADM_POWER_SAVING_CONFIG_STATUS
// @brief Status output for Power Saving Configuration IOCTL
typedef struct adm_power_saving_config_status_struct
{
    ADM_STATUS  status;
}
ADM_POWER_SAVING_CONFIG_STATUS;

typedef enum adm_slic_upgrade_cmd_type_tag
{
    ADM_SLIC_UPGRADE_CMD_TYPE_UPGRADE,
    ADM_SLIC_UPGRADE_CMD_TYPE_REPLACE
}
ADM_SLIC_UPGRADE_CMD_TYPE;

// adm set slic upgrade data
typedef struct adm_set_slic_upgrade_data
{
    ADM_SLIC_UPGRADE_CMD_TYPE type;
}
ADM_SET_SLIC_UPGRADE_DATA;

// adm set slic upgrade status
typedef struct adm_set_slic_upgrade_status_struct
{
    ADM_STATUS  status;
}
ADM_SET_SLIC_UPGRADE_STATUS;

// Set NDU state parameters
typedef struct adm_set_ndu_state_cmd
{
    UINT_16E                        primarySPCompatLevel;
    UINT_16E                        targetBundleCompatLevel;
    enum K10_NDU_ADMIN_OPC_MANAGE   nduOperation;
    enum K10_NDU_PROGRESS           nduProgress;
    BOOL                            isSPAPrimarySP;
    BOOL                            isOperationComplete;
}
ADM_SET_NDU_STATE_CMD;

// Set NDU state status
typedef struct adm_set_ndu_state_status
{
    ADM_STATUS  status;
}
ADM_SET_NDU_STATE_STATUS;

// Input data for Bind_get_config IOCTL
typedef struct adm_bind_get_config_cmd_struct
{
    UINT_32         raid_group_id;  // raid group that the bind will be issued to
    UINT_32         raid_group_type;      // raid group type
    UINT_32         num_of_luns_to_bind;
    LBA_T           capacity_to_use;  // 0, if Flare is to find and use the largest contiguous space
}
ADM_BIND_GET_CONFIG_CMD;

// Output data for Bind_get_config IOCTL
typedef struct adm_bind_get_config_data_struct
{
    LBA_T       lun_capacity;
    ADM_STATUS  status;
}
ADM_BIND_GET_CONFIG_DATA;

// Input data for FBE Configure/Deconfigure Spare IOCTL
typedef struct adm_configure_spare_cmd_struct
{
    UINT_32  fru_number;
    BOOL     spare;

} ADM_CONFIGURE_SPARE_CMD;

// Status output for FBE Configure/Deconfigure Spare IOCTL
typedef struct adm_configure_spare_data_struct
{
    ADM_STATUS  status;

} ADM_CONFIGURE_SPARE_DATA;

// Input data for FBE Copy a drive to a HS or unconfigured drive IOCTL
typedef struct adm_copy_drive_cmd_struct
{
    UINT_32  from_fru_number; // drive to copy from
    BOOL     to_fru_number;   // drive to copy to

} ADM_COPY_DRIVE_CMD;

// Status output for FBE Copy a drive to the HS or unconfigured drive IOCTL
typedef struct adm_copy_drive_data_struct
{
    ADM_STATUS  status;

} ADM_COPY_DRIVE_DATA;

#endif /* FLARE_ADM_IOCTLS_H */
