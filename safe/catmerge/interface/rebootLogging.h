//***************************************************************************
// Copyright (C) EMC Corporation 1989-2013
// All rights reserved.
// Licensed material -- property of EMC Corporation
//**************************************************************************/

//++
// File Name:
//      rebootLogging.h
//
// Contents:
//      Data structures for accessing Reboot Logging Section of NvRam.
//
// Revision History:
//      02/13/13    ZHF         Created reboot log related structures and types
//
//--
#ifndef _REBOOT_LOGGING_H_
#define _REBOOT_LOGGING_H_

#include "EmcPAL.h"

/* Defines the current version of reboot log structure
 * Initial version is set to 0xF0F0F001
 */
#define REBOOT_LOG_VERSION     0xF0F0F003

/* Number of entries in reboot log */
#define REBOOT_LOG_TOTAL_LOG_ENTRIES 14

/* Enum of the drivers that calls reboot (logging) */
typedef enum _REBOOT_DRIVER_LIST
{
    DRIVER_UNKNOWN       = 0, 
    DRIVER_POST,
    DRIVER_AGGREGATE,
    DRIVER_APM,
    DRIVER_ASEHWWATCHDOG,
    DRIVER_ASIDC,
    DRIVER_CLUTCHES,
    DRIVER_CMIPCI,
    DRIVER_CMID,
    DRIVER_CMISCD,
    DRIVER_CMM,
    DRIVER_CRASHCOORDINATOR,
    DRIVER_DISKTARG,
    DRIVER_DLS,
    DRIVER_DLSDRV,
    DRIVER_DLU,
    DRIVER_DUMPMANAGER,
    DRIVER_ESP,
    DRIVER_EXPATDLL,
    DRIVER_FCT,
    DRIVER_FEDISK,
    DRIVER_FLAREDRV,
    DRIVER_HBD,
    DRIVER_IDM,
    DRIVER_INTERSPLOCK,
    DRIVER_IZDAEMON,
    DRIVER_K10DGSSP,
    DRIVER_K10GOVERNOR,
    DRIVER_LOCKWATCH,
    DRIVER_MSGDISPATCHER,
    DRIVER_MIGRATION,
    DRIVER_MINIPORT,
    DRIVER_MINISETUP,
    DRIVER_MIR,
    DRIVER_MLU,
    DRIVER_MPS,
    DRIVER_MPSDLL,
    DRIVER_NDUAPP,
    DRIVER_NDUMON,
    DRIVER_NEWSP,
    DRIVER_PCIHALEXP,
    DRIVER_PEERWATCH,
    DRIVER_PSM,
    DRIVER_REBOOT,
    DRIVER_REDIRECTOR,
    DRIVER_REMOTEAGENT,
    DRIVER_RPSPLITTER,
    DRIVER_SAFETYNET,
    DRIVER_SCSITARG,
    DRIVER_SMBUS,
    DRIVER_SMD,
    DRIVER_SPCACHE,
    DRIVER_SPECL,
    DRIVER_SPID,
    DRIVER_SPM,
    DRIVER_WDT,
    DRIVER_ZMD,
    DRIVER_CMISOCK,
    DRIVER_SEP,
    DRIVER_DPCMON,
    DRIVER_CACHEPAGEREFERENCE,
    DRIVER_TOOLS_PEERSTATE,
    DRIVER_TOOLS_SMBTOOL,
    DRIVER_TOOLS_SPECLCLI,
    DRIVER_TOOLS_SPTOOL,
    DRIVER_NVCM,
    /* Do not make list size exceed DRIVER_LIMIT */

    DRIVER_LIST_SIZE,
    DRIVER_LIMIT          = 0x80  // also used as invalidation mask

}REBOOT_DRIVER_LIST, *PREBOOT_DRIVER_LIST;



/* Enum of the reasons for the reboot */
typedef enum _REBOOT_REASON_LIST
{
    REASON_UNKNOWN_REASON    = 0,   // for unknown reason
    REASON_USER_INITIATED,
    REASON_IO_MODULE_POWER_DOWN,
    REASON_MINISETUP_COMPLETE,
    REASON_FBE_BASE_REQUESTED,
    REASON_CMID_REQUESTED,
    REASON_SET_ARRAY_WWN,
    REASON_ADDITIONAL_REBOOT,
    REASON_CHECKSPTYPE_ERROR,
    REASON_NETCONFIG_CHANGE,
    REASON_RESTORE_NETREG,
    REASON_AUTOUPGRADED_SP,
    REASON_WATCHDOG_REQUEST,
    REASON_CACHE_FROM_PEER_INCOMPLETE,
    REASON_DUMP_TIMEOUT,
    REASON_VAULT_DUMP,
    REASON_HOST_INITIATED_BUGCHECK,
    REASON_NVCM_RECOVERY_FAILED,
    /* Do not make list size exceed REASON_LIMIT */

    REASON_LIST_SIZE,
    REASON_LIMIT =      0x80  // also used as invalidation mask
}REBOOT_REASON_LIST, *PREBOOT_REASON_LIST;

/* Store the enum in a short type in order to save NVRAM space */
typedef unsigned char REBOOT_DRIVER_LIST_SHORT;
typedef unsigned char REBOOT_REASON_LIST_SHORT;

//++
// Type:
//      REBOOT_LOG_ENTRY
//
// Description:
//      Reboot information is logged into a new entry everytime a reboot
//      is called (ideally).
//
// Members:
//      timestamp
//      reboot_driver      
//      reboot_reason
//
// Revision History:
//      13-Feb-13   ZHF  Created.
//
//--
#pragma pack (push, 2)
typedef struct _REBOOT_LOG_ENTRY
{
    EMCPAL_LARGE_INTEGER        timestamp;
    REBOOT_DRIVER_LIST_SHORT    reboot_driver;
    REBOOT_REASON_LIST_SHORT    reboot_reason;
    ULONG                       extended_status;
} REBOOT_LOG_ENTRY, *PREBOOT_LOG_ENTRY;
#pragma pack(pop)
//++
// Type:
//      REBOOT_LOG
//
// Description:
//      This structure contains general information of logged reboots,
//      and an array of REBOOT_LOG_ENTRY.
//
// Members:
//      version
//      logged_reboot_count
//      current_index
//      reserved
//      log_entry[]
//
// Revision History:
//      13-Feb-13   ZHF  Created.
//
//--
#pragma pack (push, 2)
typedef struct _REBOOT_LOG
{
    ULONG version;
    unsigned short logged_reboot_count;
    unsigned short current_index;

    REBOOT_LOG_ENTRY log_entry[REBOOT_LOG_TOTAL_LOG_ENTRIES];
} REBOOT_LOG, *PREBOOT_LOG;
#pragma pack(pop)
#endif // #ifndef _REBOOT_GLOBAL_H_

// End rebootLogging.h
