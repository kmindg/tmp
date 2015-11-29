/*********************************************************************
 * Copyright (C) EMC Corporation, 1989 - 2011
 * All Rights Reserved.
 * Licensed Material-Property of EMC Corporation.
 * This software is made available solely pursuant to the terms
 * of a EMC license agreement which governs its use.
 *********************************************************************/

/*********************************************************************
 *
 * Description:
 *      This is a global header file for the FBEAPIX class library.
 *      The variable declarations found in this file can be referred 
 *      from anywhere in the code.
 *
 *  History:
 *      12-May-2010 : initial version
 *
 *********************************************************************/

#ifndef FBEAPIX_H
#define FBEAPIX_H

/*********************************************************************
 * defines : 
 *********************************************************************/

#define Prog fbe_ioctl_object_interface_cli.exe

// The length of idname of an object
#define IDNAME_LENGTH 64

// The length of Params
#define MAX_PARAMS_SIZE 250

// The length of hold usage of a preprocessing call
#define HOLD_USAGE_LENGTH(uformat,key) (strlen(uformat) + (2*strlen(key)))

// The length of hold usage of a postprocessing call
#define HOLD_TEMP_LENGTH(temp,api_call) (strlen(temp) + strlen(api_call) + 200)

// Macro devices (Cache)
#define DRAMCACHE_DEVICE_NAME_CHAR   "\\\\.\\DRAMCache"
#define DRAMCACHE_UPPER_DEVICE_NAME  "\\Device\\CLARiiONdisk"
#define DRAMCACHE_DEVICE_LINK_NAME   "\\DosDevices\\CLARiiONdisk"
#define DRAMCACHE_LOWER_DEVICE_NAME  "\\Device\\FlareDisk"

// Volume number default.
#define INVALID_VOLUME (~0)

// Use with fbe_apix_util.cpp module
#define CASENAME(E) case E: return #E
#define DFLTNAME(E) default: return #E
#define LOCAL_SP 0
#define PEER_SP 1
#define INVALID_SP 2

/*********************************************************************
 * Constants : Object types
 *********************************************************************/

const unsigned FILEUTILCLASS         = 10;
const unsigned OBJECT                = 11;
const unsigned COLLECTIONOFOBJECTS   = 12;
const unsigned ARGUMENTS             = 13;
const unsigned FBEAPIINIT            = 14;
const unsigned NILOBJECT             = 98;
const unsigned DOBJECT               = 99;

/*********************************************************************
 * Constants : Other
 *********************************************************************/

const unsigned ARRAYSIZE             = 10;
const int      MAX_STRING_LENGTH     = 250;
const int      MAX_PHYDRIVE_FUNCS    = 3;
const unsigned HIGH_VALUE            = 0xffffffff;

/*********************************************************************
 * variable declarations
 *********************************************************************/

// Object counters 
static unsigned CSX_MAYBE_UNUSED gARRAYCount = ARRAYSIZE;
static unsigned CSX_MAYBE_UNUSED gObjCount  = 0;
static unsigned CSX_MAYBE_UNUSED gArgCount  = 0;
static unsigned CSX_MAYBE_UNUSED gUtlCount  = 0;
static unsigned CSX_MAYBE_UNUSED gPhysCount  = 0;
static unsigned CSX_MAYBE_UNUSED gPhysDriveCount = 0;
static unsigned CSX_MAYBE_UNUSED gPhysLogDriveCount = 0;
static unsigned CSX_MAYBE_UNUSED gApiCount  = 0;
static unsigned CSX_MAYBE_UNUSED gLstCount  = 0;
static unsigned CSX_MAYBE_UNUSED gNulCount  = 0;
static unsigned CSX_MAYBE_UNUSED gSepCount = 0;
static unsigned CSX_MAYBE_UNUSED gSepBindCount = 0;
static unsigned CSX_MAYBE_UNUSED gSepBvdCount = 0;
static unsigned CSX_MAYBE_UNUSED gSepCmiCount = 0;
static unsigned CSX_MAYBE_UNUSED gSepLunCount = 0;
static unsigned CSX_MAYBE_UNUSED gSepPanicSPCount = 0;
static unsigned CSX_MAYBE_UNUSED gSepPowerSavingCount = 0;
static unsigned CSX_MAYBE_UNUSED gSepProvisionDriveCount = 0;
static unsigned CSX_MAYBE_UNUSED gSepRaidGroupCount = 0;
static unsigned CSX_MAYBE_UNUSED gSepVirtualDriveCount = 0;
static unsigned CSX_MAYBE_UNUSED gSysCount = 0;
static unsigned CSX_MAYBE_UNUSED gSysEventLogCount = 0;
static unsigned CSX_MAYBE_UNUSED gEspCount = 0;
static unsigned CSX_MAYBE_UNUSED gEspDriveMgmtCount = 0;
static unsigned CSX_MAYBE_UNUSED gPhysDiscoveryCount = 0;
static unsigned CSX_MAYBE_UNUSED gSysDiscoveryCount = 0;
static unsigned CSX_MAYBE_UNUSED gPhysEnclCount = 0;
static unsigned CSX_MAYBE_UNUSED gSepDatabaseCount = 0;
static unsigned CSX_MAYBE_UNUSED gEspSpsMgmtCount = 0;
static unsigned CSX_MAYBE_UNUSED gEspEirCount =0;
static unsigned CSX_MAYBE_UNUSED gEspEnclMgmtCount = 0;
static unsigned CSX_MAYBE_UNUSED gSepMetadataCount = 0;
static unsigned CSX_MAYBE_UNUSED gEspPsMgmtCount = 0;
static unsigned CSX_MAYBE_UNUSED gPhyBoardCount = 0;
static unsigned CSX_MAYBE_UNUSED gEspBoardMgmtCount = 0;
static unsigned CSX_MAYBE_UNUSED gEspCoolingMgmtCount = 0;
static unsigned CSX_MAYBE_UNUSED gSepJobServiceCount = 0;
static unsigned CSX_MAYBE_UNUSED gEspModuleMgmtCount = 0;
static unsigned CSX_MAYBE_UNUSED gSepSchedulerCount = 0;
static unsigned CSX_MAYBE_UNUSED gSepBaseConfigCount = 0;
static unsigned CSX_MAYBE_UNUSED gIoctlCount = 0;
static unsigned CSX_MAYBE_UNUSED gIoctlDramCacheCount = 0;
static unsigned CSX_MAYBE_UNUSED gCount[100];
static unsigned CSX_MAYBE_UNUSED gSepLogicalErrorInjectionCount = 0;
static unsigned CSX_MAYBE_UNUSED gSepSystemBgServiceCount = 0;

// Object identification
static unsigned CSX_MAYBE_UNUSED id;

// strings
static char CSX_MAYBE_UNUSED line[MAX_STRING_LENGTH];
static char CSX_MAYBE_UNUSED iCmd[200]; // program arguments

// File pointers
static CSX_MAYBE_UNUSED FILE * gLfp;
static CSX_MAYBE_UNUSED FILE * gTfp;

// Flags: 
static int CSX_MAYBE_UNUSED gDebug;
static int CSX_MAYBE_UNUSED gHelp;
static unsigned CSX_MAYBE_UNUSED gIoctl;

/*********************************************************************
 * enum definitions
 *********************************************************************/

enum FLAG  {FIRSTTIME, NOTFIRSTTIME};
enum BOOLe {OFF, ON};
enum STAT  {PASS, FAIL};

enum DAY_NUM {
    SUNDAY = 0,
    MONDAY,
    TUESDAY,
    WEDNESDAY,
    THURSDAY,
    FRIDAY,
    SATURDAY,
    NOT_DAY
};

typedef enum driver_e {
    INVALID_DRIVER = 0,
    KERNEL,
    SIMULATOR,
} driver_t;

typedef enum package_e {
    INVALID_PACKAGE  = 0x0,
    PHYSICAL_PACKAGE = 2,
    ESP_PACKAGE      = 3,
    NEIT_PACKAGE     = 4,
    SYS_PACKAGE      = 5,
    SEP_PACKAGE      = 6,
    IOCTL_PACKAGE    = 7,
} package_t;

typedef enum package_subset_e {
    INVALID_PACKAGE_SUBSET = 0x0,

    //  ESP Package Interface

    ESP_SPS_MGMT_INTERFACE     = 311,
    ESP_ENCL_MGMT_INTERFACE    = 312,
    ESP_DRIVE_MGMT_INTERFACE   = 313,
    ESP_EIR_INTERFACE          = 314,
    ESP_PS_MGMT_INTERFACE      = 315,
    ESP_BOARD_MGMT_INTERFACE   = 316,
    ESP_COOLING_MGMT_INTERFACE = 317,
	ESP_MODULE_MGMT_INTERFACE  = 318,

    // NEIT Package Interface

    NEIT_RDGEN_INTERFACE = 411,
    NEIT_PROTOCOL_ERROR_INJECTION_INTERFACE = 412,
    NEIT_LOGICAL_ERROR_INJECTION_INTERFACE  = 413,

    // Physical Drive Package Interface

    PHY_DRIVE_INTERFACE = 211,
    PHY_DRIVE_CONFIG_SERVICE_INTERFACE = 212,
    PHY_DISCOVERY_INTERFACE = 213,
    PHY_LOG_DRIVE_INTERFACE = 214,
    PHY_BOARD_INTERFACE     = 215,
    PHY_PORT_INTERFACE      = 216,
    PHY_ENCL_INTERFACE      = 217,

    // System Package Interface

    SYS_COMMON_NOTIFICATION_INTERFACE = 511,
    SYS_COMMON_USER_INTERFACE         = 512,
    SYS_EVENT_LOG_SERVICE_INTERFACE   = 513,
    SYS_COMMON_INTERFACE              = 514,
    SYS_COMMON_SIM_INTERFACE          = 515,
    SYS_COMMON_KERNEL_INTERFACE       = 516,
    SYS_DISCOVERY_INTERFACE           = 517,

    // Storage Extent Package Interface

    SEP_METADATA_INTERFACE         = 611,
    SEP_DATABASE_INTERFACE         = 612,
    SEP_PROVISION_DRIVE_INTERFACE  = 613,
    SEP_IO_INTERFACE               = 614,
    SEP_POWER_SAVING_INTERFACE     = 615,
    SEP_CMI_INTERFACE              = 616,
    SEP_LUN_INTERFACE              = 617,
    SEP_BVD_INTERFACE              = 618,
    SEP_JOB_SERVICE_INTERFACE      = 619,
    SEP_RAID_GROUP_INTERFACE       = 621,
    SEP_VIRTUAL_DRIVE_INTERFACE    = 622,
    SEP_BLOCK_TRANSPORT_INTERFACE  = 623,
    SEP_SCHEDULER_INTERFACE        = 624,
    SEP_PANIC_SP_INTERFACE         = 625,
    SEP_BIND_INTERFACE             = 626,
    SEP_BASE_CONFIG_INTERFACE      = 627,
    SEP_LOGICAL_ERROR_INJECTION_INTERFACE = 628,
    SEP_SYSTEM_BG_SERVICE_INTERFACE = 629,
    SEP_SYS_TIME_THRESHOLD_INTERFACE= 630,

	// Ioctl Package Interface
	
	IOCTL_DRAMCACHE_INTERFACE      = 711,
	
} package_subset_t;

// IOCTL's sent to the device (Cache). 

enum Enum_DRAMCacheIOCTLS {
    ENUM_IOCTL_BVD_CREATE_VOLUME = 1,
    ENUM_IOCTL_BVD_DESTROY_VOLUME,
    ENUM_IOCTL_BVD_GET_VOLUME_DRIVER_PARAMS,
    ENUM_IOCTL_BVD_SET_VOLUME_DRIVER_PARAMS,
    ENUM_IOCTL_BVD_QUERY_VOLUME_DRIVER_STATUS,
    ENUM_IOCTL_BVD_QUERY_VOLUME_STATUS,
    ENUM_IOCTL_BVD_GET_VOLUME_PARAMS,
    ENUM_IOCTL_BVD_SET_VOLUME_PARAMS,
    ENUM_IOCTL_BVD_ENUM_VOLUMES,
    ENUM_IOCTL_BVD_COMMIT,
    ENUM_IOCTL_BVD_GET_VOLUME_STATISTICS,
    ENUM_IOCTL_BVD_GET_VOLUME_DRIVER_STATISTICS,
    ENUM_IOCTL_BVD_CLEAR_VOL_STATS,
    ENUM_IOCTL_BVD_PERFORM_VOLUME_ACTION,
    ENUM_IOCTL_BVD_PERFORM_DRIVER_ACTION,
    ENUM_IOCTL_BVD_CHANGE_LUN_WWN,
    ENUM_IOCTL_CACHE_ALLOCATE_PSEUDO_CONTROL_MEMORY,
    ENUM_IOCTL_CACHE_FREE_PSEUDO_CONTROL_MEMORY,
    ENUM_IOCTL_CACHE_BOOT_COMPLETE,
    ENUM_IOCTL_BVD_SHUTDOWN
};

/*********************************************************************
 * local structure declarations
 *********************************************************************/
/*
 * Table of key/value pairs.
 *  - Where name (Key) is the abbreviated function name (user input).
 *  - Where func (Value) is the actual function name (calls associated
 *    FBE API).
 */
typedef void (*cmd_func)(int argc , char ** argv);
typedef struct cmd_s {
    const char * name;
    cmd_func func;
} cmd_t;

#endif /* FBEAPIX_H */
