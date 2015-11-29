// cm_config_exports.h
//
//
// Copyright (C) EMC Corporation 2000-2007
// All rights reserved.
// Licensed material -- property of EMC Corporation
//
//
// Header for Flare Configuration Manager declarations visible to all
// core software
//
//	Revision History
//	----------------
//	 7 Jan 00	D. Zeryck	Initial version.
//  21 Jun 01   CJH         Added LU_BIND_INFO and UNIT_CACHE_INFO
//                          from cm_config.h
//  17 Sep 01   CJH         Added BBU_TIME from cm_config.h
//
//  25 Jul 02   MSZ         Added LU_BIND_NDB for non-destructive bind
//	 1 May 03	HEW			Added LU_BIND_NO_INIT_VERIFY

#ifndef CM_CONFIG_EXPORTS_H
#define CM_CONFIG_EXPORTS_H

#include "generics.h"
#define FLARE__STAR__H

#include "core_config.h"

////++++
//
//  BEGIN Definitions
//
////----


/* System types */
/*
 * System type 1 WAS obsolete in rev 8 up to Jun 1, 1995.
 * As of Jun 1, 1995 system type 1 is again valid
 */
#define DEFAULT_SYSTEM_TYPE             1
#define HP_SYSTEM_TYPE                  2
#define FIBRE_OPEN_SYSTEM_TYPE          3
#define ITC_SYSTEM_TYPE                 4
#define CONVEX_SYSTEM_TYPE              5
#define BULL_SYSTEM_TYPE                6
#define NEC_SYSTEM_TYPE                 7
#define OPEN_CLARIION_SYSTEM_TYPE       8
#define SGI_SYSTEM_TYPE                 9
#define HP2_SYSTEM_TYPE                 0xA
#define AVIION_SYSTEM_TYPE              0xB
#define TEK_SYSTEM_TYPE	                0xC
#define SUTMYN_SYSTEM_TYPE		0xD
#define STK_SYSTEM_TYPE			0xE
#define NECVOD_SYSTEM_TYPE		0xF
/*
 * These two types are used in the fibrevod3 tree and 
 * should be treated as reserved

 #define SGIVOD_SYSTEM_TYPE               0x10
 #define GENVOD_SYSTEM_TYPE               0x11

 */
#define OPEN_CLARIION2_SYSTEM_TYPE	0x12
#define DELL_SYSTEM_TYPE			0x13
#define OPEN_SCSI2_SYSTEM_TYPE		0x14
#define UNISYS_SYSTEM_TYPE			0x15
#define FUJITSU_SIEMENS_SYSTEM_TYPE	0x16
#define SGI_NORESTRICT_SYSTEM_TYPE	0x17
#define CLARIION_ARRAY_TYPE			0x18
#define CLARIION_ARRAY_CMI_TYPE		0x19
#define CLARIION_ARRAY_NOT_CMI_TYPE	0x1A
#define HP_SAUNA_AT_SYSTEM_TYPE		0x1B
#define COMPAQ_TRU64_SYSTEM_TYPE	0x1C
#define HP_ALT_SYSTEM_TYPE			0x1D
#define HP2_ALT_SYSTEM_TYPE			0x1E
#define RECOVERPOINT_SYSTEM_TYPE	0x1F
#define CELERRA_SYSTEM_TYPE			0x20

#define MIN_SYSTEM_TYPE DEFAULT_SYSTEM_TYPE
#define MAX_SYSTEM_TYPE CELERRA_SYSTEM_TYPE


/* DEFAULT and AVIION have the same flag setting
 * one define will be used to determine system type
 */
#define CM_MISC_FLAGS_SYS_TYPE_DEFAULT_AVIION 0x00000000
#define CM_MISC_FLAGS_SYS_TYPE_DEFAULT       0x00000000
#define CM_MISC_FLAGS_SYS_TYPE_AVIION        0x00000000
#define CM_MISC_FLAGS_SYS_TYPE_HP            0x0000005D
#define CM_MISC_FLAGS_SYS_TYPE_SUN           0x00000002
#define CM_MISC_FLAGS_SYS_TYPE_ITC           0x00000280
#define CM_MISC_FLAGS_SYS_TYPE_CONVEX        0x00000001
/* since Bull, NEC and Open Clariion currently have the same flag setting
 * one define will be used to determine system type 
 */
#define CM_MISC_FLAGS_SYS_TYPE_BULL_NEC_OPEN 0x00000200
/* the following three definitions are for future use */
#define CM_MISC_FLAGS_SYS_TYPE_BULL          0x00000200
#define CM_MISC_FLAGS_SYS_TYPE_NEC           0x00000200
#define CM_MISC_FLAGS_SYS_TYPE_OPEN_CLARIION 0x00000200
#define CM_MISC_FLAGS_SYS_TYPE_OPEN_CLARIION2 0x00000200
#define CM_MISC_FLAGS_SYS_TYPE_SGI           0x00002C00
#define CM_MISC_FLAGS_SYS_TYPE_HP2           0x0000004D
#define CM_MISC_FLAGS_SYS_TYPE_STK           0x00000800
#define CM_MISC_FLAGS_SYS_TYPE_MASK          0x00002EDF


/*
 * LU_BIND_INFO FLAGS.
 */

/* 0x00000001 is not in use. */

/* The following flags specify the units 
 * used for determining bind capacity.
 */
#define LU_BIND_MBYTES          0x00000002
#define LU_BIND_GBYTES          0x00000004
#define LU_BIND_BLOCKS          0x00000008
#define LU_BIND_UNITS           0x0000000E

/* The following flag is used to specify the first fit 
 * algorithm for placing a LUN within a raid group.
 */
#define LU_BIND_FF              0x00000010
#define LU_BIND_NDB             0x00000020
#define LU_BIND_NO_INIT_VERIFY  0x00000040

/* This bind flag determines if a change
 * bind command will be modifying the
 * external capacity of a unit.
 */
#define LU_BIND_CHANGE_EXTERNAL_CAPACITY  0x00000080
/* This bind flag tells the CM to
 * clear the notification needed flag.
 */
#define LU_BIND_CLEAR_NOTIFY_NEEDED       0x00000100
/* This flag tells CM that its a
 * FIX LUN command.
 */
#define LU_BIND_FIXLUN          0x00000200

/*
 * CM LU WIDTH DEFINES
 */
#define CM_INDIVIDUAL_DISK_COUNT    1
#define CM_RAID0_MIN_COUNT          3
#define CM_RAID1_COUNT              2
#define CM_RAID10_MIN_COUNT         2
#define CM_R5_MIN_DISK_COUNT        3
#define CM_SPARE_DISK_COUNT         1
#define RAID3_SHORT_ARRAY_SIZE      5
#define RAID3_TALL_ARRAY_SIZE       9
#define RAID6_FIX_ARRAY_SIZE       10

/* RAID 6 supports even drives from 4-16 after setting the FCLI flag.
 */
#define RAID6_SHORT_ARRAY_SIZE      4
#define RAID6_TALL_ARRAY_SIZE      16

/* 
 * Registry Related Defines
 */
#define FLARE_REGISTRY_PATH_WNAME      "SYSTEM\\CurrentControlSet\\Services\\Flaredrv\\Parameters"
#define FLARE_DH_DB_Q_SIZE_NAME		"FlareDhDbDriveQueueDepth"
#define FLARE_REGISTRY_PATH_NAME      "SYSTEM\\CurrentControlSet\\Services\\Flaredrv\\Parameters"
#define FLARE_DH_DB_Q_SIZE_NAME     "FlareDhDbDriveQueueDepth"

/************
 *  Types
 ************/


/*******************************************************************
 *
 *  LU_BIND_INFO
 *
 *      Used by process(es) to inform the CM how to bind (or change
 *      the bind parameters of an existing) unit.
 *******************************************************************/

typedef struct lu_bind_info
{
    UINT_32 checksum;
    UINT_32 unit_type;          // Mimics same field in LOGICAL_UNIT 
    UINT_32 phys_unit;          // host selects unit number 
    UINT_32 rebuild_priority;   // determines rebuild rate (ASAP, High, Medium, Low etc)

    /* If the LU_BIND_CHANGE_EXTERNAL_CAPACITY flag is set in
     * the flags field, then num_stripes will be used as an external_capacity.
     * This capacity allows FLARE to internally generate
     * capacity changes, due to IOCTL_FLARE_MODIFY_CAPACITY ioctls.
     */
    LBA_T num_stripes;          // use "0" for controller to determine size
    
    UINT_32 element_size;       // use "0" for controller to use default 
	UINT_32 elements_per_parity_stripe; // use "0" for controller to use default 
    UINT_32 lu_attributes;      // mimics same field in LOGICAL_UNIT 
    UINT_32 cache_config_flags; // mimics same field in LOGICAL_UNIT
    UINT_32 fru_count;          // numbers of FRUs in this unit 
	UINT_32 flags;              // Miscellaneous Flags 
    UINT_32 verify_priority;    // determines background verify rate (ASAP, High, Medium, Low etc)
	UINT_32 group_id;           // Raid group ID.
    LBA_T address_offset;
	UINT_16 align_lba;          // Sector to align with stripe boundary.
	
	/* List of FRU's on the unit and the index to the LU
	 * partition for each FRU.  Matches the LOGICAL_UNIT.
	 */
	UINT_32 fru[MAX_DISK_ARRAY_WIDTH];  
	UINT_32 partition_index[MAX_DISK_ARRAY_WIDTH];
	
#ifndef ALAMOSA_WINDOWS_ENV
    csx_u32_t pad;
#endif /* ALAMOSA_WINDOWS_ENV - PADMESS - packing mismatches */
    /* L2 cache parameters: Log-Based RAID.
	 */
    LBA_T L2_cache_size;
    UINT_32 L2_cache_map_mem_size;
	
    /* The LUN's World Wide Name
     */
    K10_WWID        world_wide_name;
	
    /* The LUN's user defined name.
     */
    K10_LOGICAL_ID  user_defined_name;
	
    UINT_32 zero_throttle_rate; // 0(ASAP) 12GB/hr(HI) 6GB/HR(MED) 4GB/HR(LOW)

    TRI_STATE       is_private; // flagged by Admin/Navi as private
	
#ifndef ALAMOSA_WINDOWS_ENV
    csx_u32_t pad2;
#endif /* ALAMOSA_WINDOWS_ENV - PADMESS - packing mismatches */
}LU_BIND_INFO;

/*  This structure is used to pass unit cache information
 *  around from ADM to the CM, from the Admin lib to ADM,
 *  and from FCLI to the CM.
 */
typedef struct unit_cache_info
{
    void    CSX_ALIGN_N(8) *cm_element;
    UINT_32 CSX_ALIGN_N(8) phys_unit;
    UINT_32 cache_config_flags;
    UINT_32 cache_idle_threshold;
    UINT_32 cache_idle_delay;   /* in sec */
    UINT_32 cache_write_aside;  /* in blocks */
    UINT_32 read_retention_priority;
    UINT_32 prefetch_segment_length;
    UINT_32 prefetch_total_length;
    UINT_32 prefetch_maximum_length;
    UINT_32 prefetch_disable_length;
    UINT_32 prefetch_idle_count;
    UINT_32 read_be_req_size;
    UINT_32 extra_info;
}
UNIT_CACHE_INFO;


/* NOTE: DO NOT ALTER THIS STRUCTURE.
 * A copy of it is kept on the SysConfig page.  Altering it will
 * mangle any existing SysConfig pages.
 */

typedef struct bbu_time
{
    UINT_8E min;
    UINT_8E hour;
    UINT_8E day;
    UINT_8E correct_day;        /* Boolean value */
}
BBU_TIME;

typedef struct lcc_time
{
    UINT_8E min;
    UINT_8E hour;
    UINT_8E day;
    UINT_8E CheckLccRevOnly;
 }
LCC_REV_TIME;

#define SUNDAY                  1
#define MONDAY                  2
#define TUESDAY                 3
#define WEDNESDAY               4
#define THURSDAY                5
#define FRIDAY                  6
#define SATURDAY                7

/* enum type for the type of operation (PSM, REBOOT, etc)
 * that can be sent to cm_set_queue_depth_per_fru */
/* make sure to make changes in all layers for changes made in this enum */
typedef enum cm_queue_depth_op
{
    CM_QUEUE_DEPTH_OP_BOOT_DRIVES_REDUCE,        /* type of operation is for the PSM drives */
    CM_QUEUE_DEPTH_OP_BOOT_DRIVES_RESTORE,       /* type of operation came from a clock 
                                                  * timeout or NDU */
    CM_QUEUE_DEPTH_OP_VAULT_DRIVES_REDUCE,       /* reduce queue depth of Vault drives during
                                                  * a cache dump*/
    CM_QUEUE_DEPTH_OP_VAULT_DRIVES_RESTORE,       /* restore q depth on vault drives after cache
                                                  * dump */
    CM_QUEUE_DEPTH_OP_DB_DRIVES_SET               /* change DB drive default Q depth */
} CM_QUEUE_DEPTH_OP;

/* this is used in passing the IOCTL down from FlareCommand to the NTFE */
typedef struct cm_queue_depth_struct
{
    CM_QUEUE_DEPTH_OP   type_of_op;
    UINT_32             depth;
} CM_QUEUE_DEPTH_STRUCT, *PTR_CM_QUEUE_DEPTH_STRUCT;

#endif   // CM_CONFIG_EXPORTS_H
