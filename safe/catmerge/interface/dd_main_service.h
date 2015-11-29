/* dd_main_service.h */

#ifndef DD_MAIN_SERVICE_H_INCLUDED
#define DD_MAIN_SERVICE_H_INCLUDED 0x00000001

/***************************************************************************
 * Copyright (C) EMC Corporation 2000-20011
 * All rights reserved
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *
 * DESCRIPTION:
 *   This header file contains DD Main Service related definitions.
 *
 * NOTES:
 *   In this file _MS_ stands for MAIN SERVICE
 *
 * HISTORY:
 *   09-Aug-2000: Created.                               Mike Zelikov (MZ)
 *   01-May-2010: The last guy to touch ODBS.            Jim Cook (CJC)
 *   27-Apr-2011: Introduced external use db set         Greg Mogavero/
 *                for VM Simulator                       Srini M    
 *
 ***************************************************************************/

/* INCLUDE FILES */
#include "odb_data_directory.h"


/* LITERAL DEFINITIONS */
#define DD_MS_REVISION_ONE                            ((DBO_REVISION) 1)
#define DD_MS_REVISION_TWO                            ((DBO_REVISION) 2)
#define DD_MS_REVISION_HUNDRED                        ((DBO_REVISION) 100)
#define DD_MS_CURRENT_REVISION                        DD_MS_REVISION_ONE


#define DD_MS_REV_ONE_START_ADDRESS                   ((DBOLB_ADDR)0)    /* Notice that this is LOGICAL address */
#define DD_MS_REV_ONE_MAN_SPACE_BLK_SIZE              DBO_MB_2_BLOCKS(1)
#define DD_MS_REV_ONE_PREV_REV_MAN_SPACE_BLK_SIZE     DBO_MB_2_BLOCKS(1)

/* What is the maximum VAULT fru */
#define DD_MS_MAX_VAULT_FRU                           3

#define DD_MS_MIN_DB_FRU                              0
#define DD_MS_MAX_DB_FRU                              2

/* Default update count for newly created DD structures */
#define DD_MS_DEFAULT_UPDATE_COUNT                    ((UINT_32E) 1)


/*************************************************************
 * ENUM_DD_MS(USER/EXTERNAL_USE)_TYPE *
 *   This enum defines valid ODBS database types.
 * Note:
 *   Once a type has been used it should not be reused for any
 *   other set -- treat as WWN.
 *   THE USER TYPE is the special case: FLARE Database Code (CM)
 *   expects the number of USER DB types to be in 16 bit range!
 *************************************************************
 */

/* IMP NOTE:
 *   If new types are added, they should be added at the end of enumeration.
 *   Depending on the target platform, include or exclude the types
 *   accordingly. Also, excluded types should be #defined after the
 *   enumeration.
 */
typedef enum _ENUM_DD_MS_USER_TYPE
{

     DD_MS_FLARE_PAD_TYPE                   =    DBO_TYPE_MAKE_USER(0x001B),
     DD_MS_SYSCONFIG_TYPE                   =    DBO_TYPE_MAKE_USER(0x0000),
     DD_MS_SYSCONFIG_PAD_TYPE               =    DBO_TYPE_MAKE_USER(0x001C),
     DD_MS_GLUT_TYPE                        =    DBO_TYPE_MAKE_USER(0x0001),
     DD_MS_GLUT_PAD_TYPE                    =    DBO_TYPE_MAKE_USER(0x001D),
     DD_MS_FRU_TYPE                         =    DBO_TYPE_MAKE_USER(0x0002),
     DD_MS_FRU_PAD_TYPE                     =    DBO_TYPE_MAKE_USER(0x001E),
     DD_MS_RAID_GROUP_TYPE                  =    DBO_TYPE_MAKE_USER(0x0003),
     DD_MS_RAID_GROUP_PAD_TYPE              =    DBO_TYPE_MAKE_USER(0x001F),
     DD_MS_SPA_ULOG_TYPE                    =    DBO_TYPE_MAKE_USER(0x0004),
     DD_MS_SPA_ULOG_PAD_TYPE                =    DBO_TYPE_MAKE_USER(0x0020),
     DD_MS_SPB_ULOG_TYPE                    =    DBO_TYPE_MAKE_USER(0x0005),
     DD_MS_SPB_ULOG_PAD_TYPE                =    DBO_TYPE_MAKE_USER(0x0021),
     DD_MS_INIT_SPA_ULOG_TYPE               =    DBO_TYPE_MAKE_USER(0x0006),
     DD_MS_INIT_SPA_ULOG_PAD_TYPE           =    DBO_TYPE_MAKE_USER(0x0022),
     DD_MS_INIT_SPB_ULOG_TYPE               =    DBO_TYPE_MAKE_USER(0x0007),
     DD_MS_INIT_SPB_ULOG_PAD_TYPE           =    DBO_TYPE_MAKE_USER(0x0023),
     DD_MS_VERIFY_RESULTS_TYPE              =    DBO_TYPE_MAKE_USER(0x0008),
     DD_MS_VERIFY_RESULTS_PAD_TYPE          =    DBO_TYPE_MAKE_USER(0x0024),
     DD_MS_ENCL_TBL_TYPE                    =    DBO_TYPE_MAKE_USER(0x0025),
     DD_MS_ENCL_TBL_PAD_TYPE                =    DBO_TYPE_MAKE_USER(0x0026),

     DD_MS_FRU_SIGNATURE_REV1_TYPE          =    DBO_TYPE_MAKE_USER(0x0013),
     DD_MS_CLEAN_DIRTY_REV1_TYPE            =    DBO_TYPE_MAKE_USER(0x0016),
     DD_MS_L2_CLEAN_DIRTY_TYPE              =    DBO_TYPE_MAKE_USER(0x000B),
     DD_MS_EXPANSION_CHKPT_REV1_TYPE        =    DBO_TYPE_MAKE_USER(0x0019),
     DD_MS_HW_FRU_VERIFY_REV1_TYPE          =    DBO_TYPE_MAKE_USER(0x0014),
     //DD_MS_DISK_VALIDATION_TYPE           =    DBO_TYPE_MAKE_USER(0x000E), /* Piranha TYPE */
     DD_MS_LEGACY_FLARE_TYPE                =    DBO_TYPE_MAKE_USER(0x000F),
     DD_MS_RESERVED_EXP_TYPE                =    DBO_TYPE_MAKE_USER(0X0010),
     DD_MS_PADDER_TYPE                      =    DBO_TYPE_MAKE_USER(0x0011),

     /* combined dummy types: DH, BIOS, PROM, FUTURE_EXP, EAST, and BOOT_LOADER */
     DD_MS_EXTERNAL_DUMMY_TYPE              =    DBO_TYPE_MAKE_USER(0x0012),

     DD_MS_32K_ALIGNING_PAD_TYPE            =    DBO_TYPE_MAKE_USER(0x0015),
     DD_MS_FRU_SIGNATURE_TYPE               =    DBO_TYPE_MAKE_USER(0x0009),
     DD_MS_FRU_SIGNATURE_PAD_TYPE           =    DBO_TYPE_MAKE_USER(0x0017),
     DD_MS_HW_FRU_VERIFY_TYPE               =    DBO_TYPE_MAKE_USER(0x000D),
     DD_MS_HW_FRU_VERIFY_PAD_TYPE           =    DBO_TYPE_MAKE_USER(0x0018),
     DD_MS_EXPANSION_CHKPT_TYPE             =    DBO_TYPE_MAKE_USER(0x000C),
     DD_MS_EXPANSION_CHKPT_PAD_TYPE         =    DBO_TYPE_MAKE_USER(0x001A),
     DD_MS_CLEAN_DIRTY_TYPE                 =    DBO_TYPE_MAKE_USER(0x000A),
} ENUM_DD_MS_USER_TYPE;

// Number of ACTIVE types in the dd_ms_user_set_info table changes
#define DD_MS_NUM_USER_TYPES                0x17

// Maximum type number (highest value used)
#define DD_MS_MAX_USER_TYPE                 0x25

// defining USER type commented out in enumneration
#define DD_MS_DISK_VALIDATION_TYPE          DBO_TYPE_MAKE_USER(0x000E)

/* USER DB REVISION NUMBERS */
#define DD_MS_USER_DB_REV_NUM                         ((DBO_REVISION) 1)

#define DD_MS_FRU_SIGNATURE_REVISION                  ((DBO_REVISION) 1)
#define DD_MS_CLEAN_DIRTY_REVISION                    ((DBO_REVISION) 1)
#define DD_MS_EXPANSION_CHKPT_REVISION                ((DBO_REVISION) 1)
#define DD_MS_HW_FRU_VERIFY_REVISION                  ((DBO_REVISION) 1)
#define DD_MS_32K_ALIGNING_PAD_REVISION               ((DBO_REVISION) 1)
#define DD_MS_FRU_SIGNATURE_PAD_REVISION              ((DBO_REVISION) 1)
#define DD_MS_HW_FRU_VERIFY_PAD_REVISION              ((DBO_REVISION) 1)
#define DD_MS_EXPANSION_CHKPT_PAD_REVISION            ((DBO_REVISION) 1)
#define DD_MS_PADDER_REVISION                         ((DBO_REVISION) 1)
#define DD_MS_EXTERNAL_DUMMY_REVISION                 ((DBO_REVISION) 1)
#define DD_MS_FLARE_PAD_REVISION                      ((DBO_REVISION) 1)
#define DD_MS_SYSCONFIG_REVISION                      ((DBO_REVISION) 1)
#define DD_MS_SYSCONFIG_PAD_REVISION                  ((DBO_REVISION) 1)
#define DD_MS_GLUT_REVISION                           ((DBO_REVISION) 1)
#define DD_MS_GLUT_PAD_REVISION                       ((DBO_REVISION) 1)
#define DD_MS_FRU_REVISION                            ((DBO_REVISION) 1)
#define DD_MS_FRU_PAD_REVISION                        ((DBO_REVISION) 1)
#define DD_MS_RAID_GROUP_REVISION                     ((DBO_REVISION) 1)
#define DD_MS_RAID_GROUP_PAD_REVISION                 ((DBO_REVISION) 1)
#define DD_MS_VERIFY_RESULTS_REVISION                 ((DBO_REVISION) 1)
#define DD_MS_VERIFY_RESULTS_PAD_REVISION             ((DBO_REVISION) 1)
#define DD_MS_ENCL_TBL_REVISION                       ((DBO_REVISION) 1)
#define DD_MS_ENCL_TBL_PAD_REVISION                   ((DBO_REVISION) 1)
#define DD_MS_RESERVED_EXP_REVISION                   ((DBO_REVISION) 1)

/* EXTERNAL USE DB TYPES */
/* NOTE:
 *   Once a type has been used it should not be reused for any
 *   other set -- treat as WWN.
 *   If any new external type is added that exists on ALL FRUs,
 *   then sum it with DD_MS_EXTERNAL_USE_ALL_FRUS_BLK_SIZE
 */
/* IMP NOTE:
 *   If new types are added, they should be added at the end of enumeration.
 *   Depending on the target platform, include or exclude the new types
 *   accordingly. Also, excluded types should be #defined after the
 *   enumeration.
 */

typedef enum _ENUM_DD_MS_EXTERNAL_USE_TYPE
{
     /* EXTERNALLY MANAGED DB AREAS  */

     /* DH Diagnostic */
     DD_MS_DISK_DIAGNOSTIC_TYPE         =   DBO_TYPE_MAKE_EXTERNAL_USE(0x0000),
     DD_MS_DISK_DIAGNOSTIC_PAD_TYPE     =   DBO_TYPE_MAKE_EXTERNAL_USE(0x0032),

     /* External Databases */
     DD_MS_BIOS_UCODE_TYPE              =   DBO_TYPE_MAKE_EXTERNAL_USE(0x0001),
     DD_MS_PROM_UCODE_TYPE              =   DBO_TYPE_MAKE_EXTERNAL_USE(0x0002),

     /* CHAMELION data regions */
     //DD_MS_KERNEL_SOFTWARE_TYPE         =   DBO_TYPE_MAKE_EXTERNAL_USE(0x0003), /* CHAMELION */
     //DD_MS_VOLUME_MNGR_TYPE             =   DBO_TYPE_MAKE_EXTERNAL_USE(0x0004), /* CHAMELION */
     //DD_MS_FSDB_TYPE                    =   DBO_TYPE_MAKE_EXTERNAL_USE(0x0005), /* CHAMELION */
     //DD_MS_COREDUMP_PART_TYPE           =   DBO_TYPE_MAKE_EXTERNAL_USE(0x0006),

     /* NT Boot Partitions */
     DD_MS_NT_BOOT_SPA_PRIM_TYPE        =   DBO_TYPE_MAKE_EXTERNAL_USE(0x0007),
     DD_MS_NT_BOOT_SPA_SEC_TYPE         =   DBO_TYPE_MAKE_EXTERNAL_USE(0x0008),
     DD_MS_NT_BOOT_SPB_PRIM_TYPE        =   DBO_TYPE_MAKE_EXTERNAL_USE(0x0009),
     DD_MS_NT_BOOT_SPB_SEC_TYPE         =   DBO_TYPE_MAKE_EXTERNAL_USE(0x000A),

     /* Added to make start address of NT area on fru 3 the same as on fru 2
      * Allocated dummy region of size flare+prome code regions.
      * Once NT Mirror driver supports different start address this can
      * to be removed.
      * This region should be located before NT_BOOT_SPB_SEC
      */
     //DD_MS_NT_DUMMY1_TYPE               =   DBO_TYPE_MAKE_EXTERNAL_USE(0x000B),

     /* Added in order not to change the start of the PSM and Vault from their
      * current locations.
      */
     DD_MS_PSM_RESERVED_TYPE            =   DBO_TYPE_MAKE_EXTERNAL_USE(0x000C),
     DD_MS_VAULT_RESERVED_TYPE          =   DBO_TYPE_MAKE_EXTERNAL_USE(0X000D),

     /* Added to make the start address of utility partitions on all utility
      * partition disks. This is needed because the PSM is only on disks 0-2,
      * and the utility partition is on disks 0-3. This region should be
      * located before DD_MS_UTIL_VAULT_RESERVED_TYPE
      */
     //DD_MS_PSM_DUMMY_TYPE               =   DBO_TYPE_MAKE_EXTERNAL_USE(0X000E),

     /* Utility Partitions */
     DD_MS_UTIL_SPA_PRIM_TYPE           =   DBO_TYPE_MAKE_EXTERNAL_USE(0X000F),
     DD_MS_UTIL_SPA_SEC_TYPE            =   DBO_TYPE_MAKE_EXTERNAL_USE(0X0010),
     DD_MS_UTIL_SPB_PRIM_TYPE           =   DBO_TYPE_MAKE_EXTERNAL_USE(0X0011),
     DD_MS_UTIL_SPB_SEC_TYPE            =   DBO_TYPE_MAKE_EXTERNAL_USE(0X0012),

     /* ICA Image Repository */
     DD_MS_ICA_IMAGE_REP_TYPE           =   DBO_TYPE_MAKE_EXTERNAL_USE(0X0013),


     /********* IMP: Add new DBO_TYPES at the end *******/

     //DD_MS_EXTENDED_DIAGNOSTIC_TYPE   =   DBO_TYPE_MAKE_EXTERNAL_USE(0X0014), /* Piranha TYPE */
     DD_MS_BOOT_DIRECTIVES_TYPE         =   DBO_TYPE_MAKE_EXTERNAL_USE(0X0015),
     //DD_MS_EXTENDED_RESERVED_TYPE     =   DBO_TYPE_MAKE_EXTERNAL_USE(0X0016), /* Piranha TYPE */
     //DD_MS_NT_BOOT_RESERVED_TYPE      =   DBO_TYPE_MAKE_EXTERNAL_USE(0X0017), /* Piranha TYPE */
     //DD_MS_FUTURE_RESERVED_TYPE       =   DBO_TYPE_MAKE_EXTERNAL_USE(0X0018), /* Piranha TYPE */

     DD_MS_FUTURE_EXP_TYPE              =   DBO_TYPE_MAKE_EXTERNAL_USE(0x0019),
     DD_MS_GPS_FW_TYPE                  =   DBO_TYPE_MAKE_EXTERNAL_USE(0x001A),

     /* New Flare DB; already accounted for in User DB */
     DD_MS_EXP_USER_DUMMY_TYPE          =   DBO_TYPE_MAKE_EXTERNAL_USE(0X001B),

     /* Vault future growth */
     DD_MS_VAULT_FUTURE_EXP_TYPE        =   DBO_TYPE_MAKE_EXTERNAL_USE(0X001C),

     /* Utility Partitions for future expansion */
     DD_MS_UTIL_SPA_PRIM_EXP_TYPE       =   DBO_TYPE_MAKE_EXTERNAL_USE(0X001D),
     DD_MS_UTIL_SPA_SEC_EXP_TYPE        =   DBO_TYPE_MAKE_EXTERNAL_USE(0X001E),
     DD_MS_UTIL_SPB_PRIM_EXP_TYPE       =   DBO_TYPE_MAKE_EXTERNAL_USE(0X001F),
     DD_MS_UTIL_SPB_SEC_EXP_TYPE        =   DBO_TYPE_MAKE_EXTERNAL_USE(0X0020),

     /* Flare boot partition for future expansion */
     DD_MS_NT_BOOT_SPA_PRIM_EXP_TYPE    =   DBO_TYPE_MAKE_EXTERNAL_USE(0x0021),
     DD_MS_NT_BOOT_SPA_SEC_EXP_TYPE     =   DBO_TYPE_MAKE_EXTERNAL_USE(0x0022),
     DD_MS_NT_BOOT_SPB_PRIM_EXP_TYPE    =   DBO_TYPE_MAKE_EXTERNAL_USE(0x0023),
     DD_MS_NT_BOOT_SPB_SEC_EXP_TYPE     =   DBO_TYPE_MAKE_EXTERNAL_USE(0x0024),

     /* ICA Image Repository future expansiion */
     /* Note: Only one ICA_IMAGE_REP type is being used across all the disks */
     /* In future, three different types would be preferrable */
     DD_MS_ICA_IMAGE_REP_EXP_TYPE       =   DBO_TYPE_MAKE_EXTERNAL_USE(0X0025),

     /* Boot directives */
     DD_MS_FUTURE_GROWTH_TYPE           =   DBO_TYPE_MAKE_EXTERNAL_USE(0X0026),

     /* DUMMY TYPES */
     DD_MS_EXT_NEW_DB_DUMMY_TYPE        =   DBO_TYPE_MAKE_EXTERNAL_USE(0X0027),

     DD_MS_UTIL_FLARE_DUMMY_TYPE        =   DBO_TYPE_MAKE_EXTERNAL_USE(0X0028),

     /* A dummy region on disk 3 to occupy reserved space used on disks 0 - 2 */
     DD_MS_PSM_BOOT_DUMMY_TYPE          =   DBO_TYPE_MAKE_EXTERNAL_USE(0X0029),

     /* Boot Loader */
     DD_MS_BOOT_LOADER_TYPE             =   DBO_TYPE_MAKE_EXTERNAL_USE(0X0030),

     /* ICA Padder - We need this to make the ICA + Padder region to be an exact
      * multiple of 4 MB */
     DD_MS_ICA_IMAGE_REP_PAD_TYPE       =   DBO_TYPE_MAKE_EXTERNAL_USE(0X0031),

      //These are some of the new fields added for Bigfoot
     DD_MS_EXPANDER_UCODE_TYPE          =   DBO_TYPE_MAKE_EXTERNAL_USE(0x00032),
     DD_MS_BOOT_UCODE_TYPE              =   DBO_TYPE_MAKE_EXTERNAL_USE(0x00033),
     DD_MS_ISTR_UCODE_TYPE              =   DBO_TYPE_MAKE_EXTERNAL_USE(0x00034),
     DD_MS_CPLD_UCODE_TYPE              =   DBO_TYPE_MAKE_EXTERNAL_USE(0x00035),
     DD_MS_MUX_UCODE_TYPE               =   DBO_TYPE_MAKE_EXTERNAL_USE(0x00036),
     DD_MS_CC_UCODE_TYPE                =   DBO_TYPE_MAKE_EXTERNAL_USE(0x00037),
     DD_MS_DPE_MC_UCODE_TYPE            =   DBO_TYPE_MAKE_EXTERNAL_USE(0x00038),
     DD_MS_DAE_MC_UCODE_TYPE            =   DBO_TYPE_MAKE_EXTERNAL_USE(0x00039),
     DD_MS_LSI_UCODE_TYPE               =   DBO_TYPE_MAKE_EXTERNAL_USE(0x0003A),
     DD_MS_DDBS_TYPE                    =   DBO_TYPE_MAKE_EXTERNAL_USE(0x0003B),
     DD_MS_DDBS_LUN_OBJ_TYPE            =   DBO_TYPE_MAKE_EXTERNAL_USE(0x0003C),
     DD_MS_BIOS_LUN_OBJ_TYPE            =   DBO_TYPE_MAKE_EXTERNAL_USE(0x0003D),
     DD_MS_PROM_LUN_OBJ_TYPE            =   DBO_TYPE_MAKE_EXTERNAL_USE(0x0003E),
     DD_MS_GPS_FW_LUN_OBJ_TYPE          =   DBO_TYPE_MAKE_EXTERNAL_USE(0x0003F),
     DD_MS_FLARE_DB_TYPE                =   DBO_TYPE_MAKE_EXTERNAL_USE(0x00040),
     DD_MS_FLARE_DB_LUN_OBJ_TYPE        =   DBO_TYPE_MAKE_EXTERNAL_USE(0x00041),
     DD_MS_ESP_LUN_TYPE                 =   DBO_TYPE_MAKE_EXTERNAL_USE(0x00042),
     DD_MS_ESP_LUN_OBJ_TYPE             =   DBO_TYPE_MAKE_EXTERNAL_USE(0x00043),
     DD_MS_VCX_LUN_0_TYPE               =   DBO_TYPE_MAKE_EXTERNAL_USE(0x00044),
     DD_MS_VCX_LUN_0_OBJ_TYPE           =   DBO_TYPE_MAKE_EXTERNAL_USE(0x00045),
     DD_MS_VCX_LUN_1_TYPE               =   DBO_TYPE_MAKE_EXTERNAL_USE(0x00046),
     DD_MS_VCX_LUN_1_OBJ_TYPE           =   DBO_TYPE_MAKE_EXTERNAL_USE(0x00047),
     DD_MS_VCX_LUN_2_TYPE               =   DBO_TYPE_MAKE_EXTERNAL_USE(0x00048),
     DD_MS_VCX_LUN_2_OBJ_TYPE           =   DBO_TYPE_MAKE_EXTERNAL_USE(0x00049),
     DD_MS_VCX_LUN_3_TYPE               =   DBO_TYPE_MAKE_EXTERNAL_USE(0x0004A),
     DD_MS_VCX_LUN_3_OBJ_TYPE           =   DBO_TYPE_MAKE_EXTERNAL_USE(0x0004B),
     DD_MS_VCX_LUN_4_TYPE               =   DBO_TYPE_MAKE_EXTERNAL_USE(0x0004C),
     DD_MS_VCX_LUN_4_OBJ_TYPE           =   DBO_TYPE_MAKE_EXTERNAL_USE(0x0004D),
     DD_MS_VCX_LUN_5_TYPE               =   DBO_TYPE_MAKE_EXTERNAL_USE(0x0004E),
     DD_MS_VCX_LUN_5_OBJ_TYPE           =   DBO_TYPE_MAKE_EXTERNAL_USE(0x0004F),
     DD_MS_CENTERA_LUN_TYPE             =   DBO_TYPE_MAKE_EXTERNAL_USE(0x00050),
     DD_MS_CENTERA_LUN_OBJ_TYPE         =   DBO_TYPE_MAKE_EXTERNAL_USE(0x00051),
     DD_MS_PSM_LUN_OBJ_TYPE             =   DBO_TYPE_MAKE_EXTERNAL_USE(0x00052),
     DD_MS_DRAM_CACHE_LUN_TYPE          =   DBO_TYPE_MAKE_EXTERNAL_USE(0x00053),
     DD_MS_DRAM_CACHE_LUN_OBJ_TYPE      =   DBO_TYPE_MAKE_EXTERNAL_USE(0x00054),
     DD_MS_EFD_CACHE_LUN_TYPE           =   DBO_TYPE_MAKE_EXTERNAL_USE(0x00055),
     DD_MS_EFD_CACHE_LUN_OBJ_TYPE       =   DBO_TYPE_MAKE_EXTERNAL_USE(0x00056),
     DD_MS_PSM_RAID_GROUP_OBJ_TYPE      =   DBO_TYPE_MAKE_EXTERNAL_USE(0x00057),
     DD_MS_ALIGN_BUFFER1_TYPE           =   DBO_TYPE_MAKE_EXTERNAL_USE(0x00058),
     DD_MS_WIL_A_LUN_TYPE               =   DBO_TYPE_MAKE_EXTERNAL_USE(0x00059),
     DD_MS_WIL_A_LUN_OBJ_TYPE           =   DBO_TYPE_MAKE_EXTERNAL_USE(0x0005A),
     DD_MS_WIL_B_LUN_TYPE               =   DBO_TYPE_MAKE_EXTERNAL_USE(0x0005B),
     DD_MS_WIL_B_LUN_OBJ_TYPE           =   DBO_TYPE_MAKE_EXTERNAL_USE(0x0005C),
     DD_MS_CPL_A_LUN_TYPE               =   DBO_TYPE_MAKE_EXTERNAL_USE(0x0005D),
     DD_MS_CPL_A_LUN_OBJ_TYPE           =   DBO_TYPE_MAKE_EXTERNAL_USE(0x0005E),
     DD_MS_CPL_B_LUN_TYPE               =   DBO_TYPE_MAKE_EXTERNAL_USE(0x0005F),
     DD_MS_CPL_B_LUN_OBJ_TYPE           =   DBO_TYPE_MAKE_EXTERNAL_USE(0x00060),
     DD_MS_RAID_METADATA_LUN_TYPE       =   DBO_TYPE_MAKE_EXTERNAL_USE(0x00061),
     DD_MS_RAID_METADATA_LUN_OBJ_TYPE   =   DBO_TYPE_MAKE_EXTERNAL_USE(0x00062),
     DD_MS_RAID_METASTATS_LUN_TYPE      =   DBO_TYPE_MAKE_EXTERNAL_USE(0x00063),
     DD_MS_RAID_METASTATS_LUN_OBJ_TYPE  =   DBO_TYPE_MAKE_EXTERNAL_USE(0x00064),
     DD_MS_VAULT_LUN_OBJECT_TYPE        =   DBO_TYPE_MAKE_EXTERNAL_USE(0x00065),
     DD_MS_VAULT_RAID_GROUP_OBJ_TYPE    =   DBO_TYPE_MAKE_EXTERNAL_USE(0x00066),
     DD_MS_NON_LUN_RAID_GROUP_OBJ_TYPE  =   DBO_TYPE_MAKE_EXTERNAL_USE(0x00067)
} ENUM_DD_MS_EXTERNAL_USE_TYPE;

// Update as number of ACTIVE types in the dd_ms_ext_use_set_info table changes
#define DD_MS_NUM_EXTERNAL_USE_TYPES    76

#define DD_MS_KERNEL_SOFTWARE_TYPE          DBO_TYPE_MAKE_EXTERNAL_USE(0x0003) /* CHAMELION */
#define DD_MS_VOLUME_MNGR_TYPE              DBO_TYPE_MAKE_EXTERNAL_USE(0x0004) /* CHAMELION */
#define DD_MS_FSDB_TYPE                     DBO_TYPE_MAKE_EXTERNAL_USE(0x0005) /* CHAMELION */
#define DD_MS_COREDUMP_PART_TYPE            DBO_TYPE_MAKE_EXTERNAL_USE(0x0006) /* Pre-Hammer */
#define DD_MS_NT_DUMMY1_TYPE                DBO_TYPE_MAKE_EXTERNAL_USE(0x000B) /* Pre-Hammer */
#define DD_MS_PSM_DUMMY_TYPE                DBO_TYPE_MAKE_EXTERNAL_USE(0X000E) /* Pre-Hammer */
#define DD_MS_EXTENDED_DIAGNOSTIC_TYPE      DBO_TYPE_MAKE_EXTERNAL_USE(0X0014) /* Piranha TYPE */
#define DD_MS_EXTENDED_RESERVED_TYPE        DBO_TYPE_MAKE_EXTERNAL_USE(0X0016) /* Piranha TYPE */
#define DD_MS_NT_BOOT_RESERVED_TYPE         DBO_TYPE_MAKE_EXTERNAL_USE(0X0017) /* Piranha TYPE */
#define DD_MS_FUTURE_RESERVED_TYPE          DBO_TYPE_MAKE_EXTERNAL_USE(0X0018) /* Piranha TYPE */


/* This is to support both old and new layouts as we are in transition (before the new layout is commited) */
#define DD_MS_NUM_EXTERNAL_USE_TYPES_REV_ONE            0x0C
#define DD_MS_FIRST_NEW_EXT_USE_INDEX                   DD_MS_NUM_EXTERNAL_USE_TYPES_REV_ONE
#define DD_MS_LAST_NEW_EXT_USE_INDEX                    (DD_MS_NUM_EXTERNAL_USE_TYPES - 1)

/* EXTERNAL USE DB REVISION NUMBERS */
#define DD_MS_EXT_USE_DB_REV_NUM                         DBO_INVALID_REVISION

#define DD_MS_DISK_DIAGNOSTIC_REVISION                   ((DBO_REVISION) 1)
#define DD_MS_DISK_DIAGNOSTIC_PAD_REVISION               ((DBO_REVISION) 1)
#define DD_MS_BIOS_UCODE_REVISION                        ((DBO_REVISION) 1)
#define DD_MS_PROM_UCODE_REVISION                        ((DBO_REVISION) 1)
#define DD_MS_FUTURE_EXP_REVISION                        ((DBO_REVISION) 1)
#define DD_MS_GPS_FW_REVISION                            ((DBO_REVISION) 1)
#define DD_MS_BOOT_LOADER_REVISION                       ((DBO_REVISION) 1)
#define DD_MS_EXP_USER_DUMMY_REVISION                    ((DBO_REVISION) 1)
#define DD_MS_VAULT_RESERVED_REVISION                    ((DBO_REVISION) 1)
#define DD_MS_VAULT_FUTURE_EXP_REVISION                  ((DBO_REVISION) 1)
#define DD_MS_UTIL_SPA_PRIM_REVISION                     ((DBO_REVISION) 100)
#define DD_MS_UTIL_SPA_SEC_REVISION                      ((DBO_REVISION) 100)
#define DD_MS_UTIL_SPB_PRIM_REVISION                     ((DBO_REVISION) 100)
#define DD_MS_UTIL_SPB_SEC_REVISION                      ((DBO_REVISION) 100)
#define DD_MS_UTIL_SPA_PRIM_EXP_REVISION                 ((DBO_REVISION) 100)
#define DD_MS_UTIL_SPA_SEC_EXP_REVISION                  ((DBO_REVISION) 100)
#define DD_MS_UTIL_SPB_PRIM_EXP_REVISION                 ((DBO_REVISION) 100)
#define DD_MS_UTIL_SPB_SEC_EXP_REVISION                  ((DBO_REVISION) 100)
#define DD_MS_NT_BOOT_SPA_PRIM_REVISION                  ((DBO_REVISION) 1)
#define DD_MS_NT_BOOT_SPA_SEC_REVISION                   ((DBO_REVISION) 1)
#define DD_MS_NT_BOOT_SPB_PRIM_REVISION                  ((DBO_REVISION) 1)
#define DD_MS_NT_BOOT_SPB_SEC_REVISION                   ((DBO_REVISION) 1)
#define DD_MS_NT_BOOT_SPA_PRIM_EXP_REVISION              ((DBO_REVISION) 1)
#define DD_MS_NT_BOOT_SPA_SEC_EXP_REVISION               ((DBO_REVISION) 1)
#define DD_MS_NT_BOOT_SPB_PRIM_EXP_REVISION              ((DBO_REVISION) 1)
#define DD_MS_NT_BOOT_SPB_SEC_EXP_REVISION               ((DBO_REVISION) 1)
#define DD_MS_ICA_IMAGE_REP_EXP_REVISION                 ((DBO_REVISION) 1)
#define DD_MS_ICA_IMAGE_REP_REVISION                     ((DBO_REVISION) 1)
#define DD_MS_PSM_RESERVED_REVISION                      ((DBO_REVISION) 1)
#define DD_MS_BOOT_DIRECTIVES_REVISION                   ((DBO_REVISION) 1)
#define DD_MS_FUTURE_GROWTH_REVISION                     ((DBO_REVISION) 1)
#define DD_MS_EXT_NEW_DB_DUMMY_REVISION                  ((DBO_REVISION) 1)
#define DD_MS_UTIL_FLARE_DUMMY_REVISION                  ((DBO_REVISION) 1)
#define DD_MS_PSM_BOOT_DUMMY_REVISION                    ((DBO_REVISION) 1)
#define DD_MS_ICA_IMAGE_REP_PAD_REVISION                 ((DBO_REVISION) 1)
#define DD_MS_DDBS_REVISION                              ((DBO_REVISION) 1)
#define DD_MS_DDBS_LUN_OBJ_REVISION                      ((DBO_REVISION) 1)
#define DD_MS_BIOS_LUN_OBJ_REVISION                      ((DBO_REVISION) 1)
#define DD_MS_PROM_LUN_OBJ_REVISION                      ((DBO_REVISION) 1)
#define DD_MS_GPS_FW_LUN_OBJ_REVISION                    ((DBO_REVISION) 1)
#define DD_MS_FLARE_DB_REVISION                          ((DBO_REVISION) 1)
#define DD_MS_FLARE_DB_LUN_OBJ_REVISION                  ((DBO_REVISION) 1)
#define DD_MS_ESP_LUN_REVISION                           ((DBO_REVISION) 1)
#define DD_MS_ESP_LUN_OBJ_REVISION                       ((DBO_REVISION) 1)
#define DD_MS_VCX_LUN_0_REVISION                         ((DBO_REVISION) 1)
#define DD_MS_VCX_LUN_0_OBJ_REVISION                     ((DBO_REVISION) 1)
#define DD_MS_VCX_LUN_1_REVISION                         ((DBO_REVISION) 1)
#define DD_MS_VCX_LUN_1_OBJ_REVISION                     ((DBO_REVISION) 1)
#define DD_MS_VCX_LUN_2_REVISION                         ((DBO_REVISION) 1)
#define DD_MS_VCX_LUN_2_OBJ_REVISION                     ((DBO_REVISION) 1)
#define DD_MS_VCX_LUN_3_REVISION                         ((DBO_REVISION) 1)
#define DD_MS_VCX_LUN_3_OBJ_REVISION                     ((DBO_REVISION) 1)
#define DD_MS_VCX_LUN_4_REVISION                         ((DBO_REVISION) 1)
#define DD_MS_VCX_LUN_4_OBJ_REVISION                     ((DBO_REVISION) 1)
#define DD_MS_VCX_LUN_5_REVISION                         ((DBO_REVISION) 1)
#define DD_MS_VCX_LUN_5_OBJ_REVISION                     ((DBO_REVISION) 1)
#define DD_MS_CENTERA_LUN_REVISION                       ((DBO_REVISION) 1)
#define DD_MS_CENTERA_LUN_OBJ_REVISION                   ((DBO_REVISION) 1)
#define DD_MS_PSM_LUN_OBJ_REVISION                       ((DBO_REVISION) 1)
#define DD_MS_DRAM_CACHE_LUN_REVISION                    ((DBO_REVISION) 1)
#define DD_MS_DRAM_CACHE_LUN_OBJ_REVISION                ((DBO_REVISION) 1)
#define DD_MS_EFD_CACHE_LUN_REVISION                     ((DBO_REVISION) 1)
#define DD_MS_EFD_CACHE_LUN_OBJ_REVISION                 ((DBO_REVISION) 1)
#define DD_MS_PSM_RAID_GROUP_OBJ_REVISION                ((DBO_REVISION) 1)
#define DD_MS_ALIGN_BUFFER1_REVISION                     ((DBO_REVISION) 1)
#define DD_MS_WIL_A_LUN_REVISION                         ((DBO_REVISION) 1)
#define DD_MS_WIL_A_LUN_OBJ_REVISION                     ((DBO_REVISION) 1)
#define DD_MS_WIL_B_LUN_REVISION                         ((DBO_REVISION) 1)
#define DD_MS_WIL_B_LUN_OBJ_REVISION                     ((DBO_REVISION) 1)
#define DD_MS_CPL_A_LUN_REVISION                         ((DBO_REVISION) 1)
#define DD_MS_CPL_A_LUN_OBJ_REVISION                     ((DBO_REVISION) 1)
#define DD_MS_CPL_B_LUN_REVISION                         ((DBO_REVISION) 1)
#define DD_MS_CPL_B_LUN_OBJ_REVISION                     ((DBO_REVISION) 1)
#define DD_MS_RAID_METADATA_LUN_REVISION                 ((DBO_REVISION) 1)
#define DD_MS_RAID_METADATA_LUN_OBJ_REVISION             ((DBO_REVISION) 1)
#define DD_MS_RAID_METASTATS_LUN_REVISION                ((DBO_REVISION) 1)
#define DD_MS_RAID_METASTATS_LUN_OBJ_REVISION            ((DBO_REVISION) 1)
#define DD_MS_VAULT_LUN_OBJECT_REVISION                  ((DBO_REVISION) 1)
#define DD_MS_VAULT_RAID_GROUP_OBJ_REVISION              ((DBO_REVISION) 1)
#define DD_MS_NON_LUN_RAID_GROUP_OBJ_REVISION            ((DBO_REVISION) 1)

/*************************************************************
 * ENUM_DD_MS_ALGORITHM *
 *   This enum defines valid Data Directory Main Service
 *   database algorithms.
 *************************************************************
 */
typedef enum _ENUM_DD_MS_ALGORITHM
{
     DD_MS_EXTERNAL_ALG            =   (DBO_ALGORITHM)0,
     DD_MS_SINGLE_DISK_ALG         =   (DBO_ALGORITHM)1,
     DD_MS_THREE_DISK_MIRROR_ALG   =   (DBO_ALGORITHM)2

} ENUM_DD_MS_ALGORITHM;

#define DD_MS_THREE_DISK_MIRROR_ALG_MIN_FRU             0
#define DD_MS_THREE_DISK_MIRROR_ALG_MAX_FRU             2
#define DD_MS_THREE_DISK_MIRROR_ALG_NUM_DISKS           (DD_MS_THREE_DISK_MIRROR_ALG_MAX_FRU - DD_MS_THREE_DISK_MIRROR_ALG_MIN_FRU + 1)


/*************************************************************
 * BLOCKS PER ENTRY PER DD MS USER DB TYPE
 *************************************************************
 */

/* ODB DATA DIRECTORY MAIN SERVICE MANAGED DB AREAS */
#define DD_MS_FRU_SIGNATURE_BLOCKS_PER_ENTRY            64
#define DD_MS_CLEAN_DIRTY_BLOCKS_PER_ENTRY              64
#define DD_MS_EXPANSION_CHKPT_BLOCKS_PER_ENTRY          1
#define DD_MS_HW_FRU_VERIFY_BLOCKS_PER_ENTRY            64
#define DD_MS_FRU_SIGNATURE_PAD_BLOCKS_PER_ENTRY        64
#define DD_MS_HW_FRU_VERIFY_PAD_BLOCKS_PER_ENTRY        64
#define DD_MS_EXPANSION_CHKPT_PAD_BLOCKS_PER_ENTRY      64
#define DD_MS_PADDER_BLOCKS_PER_ENTRY                   1

//number of blocks to aligning to second closest 32k align point
#define DD_MS_FLARE_PAD_BLOCKS_PER_ENTRY                120

#define DD_MS_SYSCONFIG_BLOCKS_PER_ENTRY                4
#define DD_MS_SYSCONFIG_PAD_BLOCKS_PER_ENTRY            64
#define DD_MS_GLUT_BLOCKS_PER_ENTRY                     2
#define DD_MS_GLUT_PAD_BLOCKS_PER_ENTRY                 64
#define DD_MS_FRU_BLOCKS_PER_ENTRY                      1
#define DD_MS_FRU_PAD_BLOCKS_PER_ENTRY                  64
#define DD_MS_RAID_GROUP_BLOCKS_PER_ENTRY               4
#define DD_MS_RAID_GROUP_PAD_BLOCKS_PER_ENTRY           64
#define DD_MS_SPA_ULOG_BLOCKS_PER_ENTRY                 1
#define DD_MS_SPA_ULOG_PAD_BLOCKS_PER_ENTRY             64
#define DD_MS_SPB_ULOG_BLOCKS_PER_ENTRY                 1
#define DD_MS_SPB_ULOG_PAD_BLOCKS_PER_ENTRY             64
#define DD_MS_INIT_SPA_ULOG_BLOCKS_PER_ENTRY            1
#define DD_MS_INIT_SPA_ULOG_PAD_BLOCKS_PER_ENTRY        64
#define DD_MS_INIT_SPB_ULOG_BLOCKS_PER_ENTRY            1
#define DD_MS_INIT_SPB_ULOG_PAD_BLOCKS_PER_ENTRY        64
#define DD_MS_VERIFY_RESULTS_BLOCKS_PER_ENTRY           1
#define DD_MS_VERIFY_RESULTS_PAD_BLOCKS_PER_ENTRY       64
#define DD_MS_ENCL_TBL_BLOCKS_PER_ENTRY                 64
#define DD_MS_ENCL_TBL_PAD_BLOCKS_PER_ENTRY             64
#define DD_MS_RESERVED_EXP_BLOCKS_PER_ENTRY             1


/*************************************************************
 * USER DB SET sizes: each size defines number of blocks allocated
 * by DB data set (set is indicated by DDE).
 *************************************************************
 */
#if defined(UMODE_ENV) || defined(SIMMODE_ENV) /* ====== SIM ====== */

#define DD_MS_FRU_SIGNATURE_NUM_ENTRIES                 2
#define DD_MS_CLEAN_DIRTY_NUM_ENTRIES                   272
#define DD_MS_EXPANSION_CHKPT_NUM_ENTRIES               64
#define DD_MS_HW_FRU_VERIFY_NUM_ENTRIES                 2
#define DD_MS_32K_ALIGNING_PAD_NUM_ENTRIES              0
#define DD_MS_FRU_SIGNATURE_PAD_NUM_ENTRIES             1
#define DD_MS_HW_FRU_VERIFY_PAD_NUM_ENTRIES             1
#define DD_MS_EXPANSION_CHKPT_PAD_NUM_ENTRIES           1
#define DD_MS_PADDER_NUM_ENTRIES                        0

/* add num entries for dummy type for clarity of code */
#define DD_MS_EXTERNAL_DUMMY_NUM_ENTRIES                1

#define DD_MS_FLARE_PAD_NUM_ENTRIES                     0
#define DD_MS_SYSCONFIG_NUM_ENTRIES                     1
#define DD_MS_SYSCONFIG_PAD_NUM_ENTRIES                 1
#define DD_MS_GLUT_NUM_ENTRIES                          64
#define DD_MS_GLUT_PAD_NUM_ENTRIES                      1
#define DD_MS_FRU_NUM_ENTRIES                           1000
#define DD_MS_FRU_PAD_NUM_ENTRIES                       1
#define DD_MS_RAID_GROUP_NUM_ENTRIES                    120
#define DD_MS_RAID_GROUP_PAD_NUM_ENTRIES                1
#define DD_MS_VERIFY_RESULTS_NUM_ENTRIES                64
#define DD_MS_VERIFY_RESULTS_PAD_NUM_ENTRIES            1
#define DD_MS_ENCL_TBL_NUM_ENTRIES                      128
#define DD_MS_ENCL_TBL_PAD_NUM_ENTRIES                  1
#define DD_MS_RESERVED_EXP_NUM_ENTRIES                  1

#else                  /* ====== HW ====== */

#define DD_MS_FRU_SIGNATURE_NUM_ENTRIES                 2
#define DD_MS_CLEAN_DIRTY_NUM_ENTRIES                   272
#define DD_MS_EXPANSION_CHKPT_NUM_ENTRIES               64
#define DD_MS_HW_FRU_VERIFY_NUM_ENTRIES                 2
#define DD_MS_32K_ALIGNING_PAD_NUM_ENTRIES              0
#define DD_MS_FRU_SIGNATURE_PAD_NUM_ENTRIES             1
#define DD_MS_HW_FRU_VERIFY_PAD_NUM_ENTRIES             1
#define DD_MS_EXPANSION_CHKPT_PAD_NUM_ENTRIES           1
#define DD_MS_PADDER_NUM_ENTRIES                        0x1A00

/* add num entries to each dummy type - for clarity of code */
#define DD_MS_EXTERNAL_DUMMY_NUM_ENTRIES                1

#define DD_MS_FLARE_PAD_NUM_ENTRIES                     0
#define DD_MS_SYSCONFIG_NUM_ENTRIES                     48
#define DD_MS_SYSCONFIG_PAD_NUM_ENTRIES                 1
// The following was 34016
#define DD_MS_GLUT_NUM_ENTRIES                          136064
#define DD_MS_GLUT_PAD_NUM_ENTRIES                      1
#define DD_MS_FRU_NUM_ENTRIES                           2048
#define DD_MS_FRU_PAD_NUM_ENTRIES                       1
#define DD_MS_RAID_GROUP_NUM_ENTRIES                    2048
#define DD_MS_RAID_GROUP_PAD_NUM_ENTRIES                1
#define DD_MS_VERIFY_RESULTS_NUM_ENTRIES                33024
#define DD_MS_VERIFY_RESULTS_PAD_NUM_ENTRIES            1
#define DD_MS_ENCL_TBL_NUM_ENTRIES                      128
#define DD_MS_ENCL_TBL_PAD_NUM_ENTRIES                  1

// The following size is the expansion area following the Flare "3 disk DB"
// It should round the total space up to occupying 160Mb.
#define DD_MS_RESERVED_EXP_NUM_ENTRIES                0xDC0  /*0x1B0DC0*/

#endif /* #if defined (UMODE_ENV) */



/*************************************************************
 * EXT USE DB SET sizes for UMODE, SIMMODE and Hardware: each size defines 
 * number of blocks allocated by DB data set (set is indicated by DDE).
 *************************************************************
 */
#if defined(UMODE_ENV) || defined(SIMMODE_ENV) /* ====== SIM ====== */

#define DD_MS_DISK_DIAGNOSTIC_BLK_SIZE                  72
#define DD_MS_DISK_DIAGNOSTIC_PAD_BLK_SIZE              8

#define DD_MS_DDBS_BLK_SIZE                             1
#define DD_MS_DDBS_LUN_OBJ_BLK_SIZE                     0
#define DD_MS_BIOS_UCODE_BLK_SIZE                       1
#define DD_MS_BIOS_LUN_OBJ_BLK_SIZE                     0
#define DD_MS_PROM_UCODE_BLK_SIZE                       1
#define DD_MS_PROM_LUN_OBJ_BLK_SIZE                     0
#define DD_MS_GPS_FW_BLK_SIZE                           1
#define DD_MS_GPS_FW_LUN_OBJ_BLK_SIZE                   0
#define DD_MS_FLARE_DB_BLK_SIZE                         DBO_MB_2_BLOCKS(1)
#define DD_MS_FLARE_DB_LUN_OBJ_BLK_SIZE                 0
#define DD_MS_PSM_RESERVED_BLK_SIZE                     DBO_MB_2_BLOCKS(2)
#define DD_MS_PSM_LUN_OBJ_BLK_SIZE                      0

#define DD_MS_VAULT_RESERVED_BLK_SIZE                   DBO_MB_2_BLOCKS(6)
#define DD_MS_VAULT_LUN_OBJECT_BLK_SIZE                 0

#define DD_MS_EFD_CACHE_LUN_BLK_SIZE                    DBO_MB_2_BLOCKS(1)
#define DD_MS_EFD_CACHE_LUN_OBJ_BLK_SIZE                0

#define DD_MS_CPL_A_LUN_BLK_SIZE                        DBO_MB_2_BLOCKS(1)
#define DD_MS_CPL_A_LUN_OBJ_BLK_SIZE                    0
#define DD_MS_CPL_B_LUN_BLK_SIZE                        DBO_MB_2_BLOCKS(1)
#define DD_MS_CPL_B_LUN_OBJ_BLK_SIZE                    0

#define DD_MS_WIL_A_LUN_BLK_SIZE                        DBO_MB_2_BLOCKS(1)
#define DD_MS_WIL_A_LUN_OBJ_BLK_SIZE                    0
#define DD_MS_WIL_B_LUN_BLK_SIZE                        DBO_MB_2_BLOCKS(1)
#define DD_MS_WIL_B_LUN_OBJ_BLK_SIZE                    0

#define DD_MS_VCX_LUN_0_BLK_SIZE                        DBO_MB_2_BLOCKS(1)
#define DD_MS_VCX_LUN_0_OBJ_BLK_SIZE                    0
#define DD_MS_VCX_LUN_1_BLK_SIZE                        DBO_MB_2_BLOCKS(1)
#define DD_MS_VCX_LUN_1_OBJ_BLK_SIZE                    0
#define DD_MS_VCX_LUN_2_BLK_SIZE                        DBO_MB_2_BLOCKS(1)
#define DD_MS_VCX_LUN_2_OBJ_BLK_SIZE                    0
#define DD_MS_VCX_LUN_3_BLK_SIZE                        DBO_MB_2_BLOCKS(1)
#define DD_MS_VCX_LUN_3_OBJ_BLK_SIZE                    0
#define DD_MS_VCX_LUN_4_BLK_SIZE                        DBO_MB_2_BLOCKS(1)
#define DD_MS_VCX_LUN_4_OBJ_BLK_SIZE                    0
#define DD_MS_VCX_LUN_5_BLK_SIZE                        DBO_MB_2_BLOCKS(1)
#define DD_MS_VCX_LUN_5_OBJ_BLK_SIZE                    0

#define DD_MS_CENTERA_LUN_BLK_SIZE                      DBO_MB_2_BLOCKS(1)
#define DD_MS_CENTERA_LUN_OBJ_BLK_SIZE                  0

#define DD_MS_ESP_LUN_BLK_SIZE                          DBO_MB_2_BLOCKS(1)
#define DD_MS_ESP_LUN_OBJ_BLK_SIZE                      0

#define DD_MS_FUTURE_EXP_BLK_SIZE                       1
#define DD_MS_PSM_RAID_GROUP_OBJ_BLK_SIZE               0
#define DD_MS_ALIGN_BUFFER1_BLK_SIZE                    0x20

#define DD_MS_RAID_METADATA_LUN_BLK_SIZE                DBO_MB_2_BLOCKS(1)
#define DD_MS_RAID_METADATA_LUN_OBJ_BLK_SIZE            0

#define DD_MS_RAID_METASTATS_LUN_BLK_SIZE               DBO_MB_2_BLOCKS(1)
#define DD_MS_RAID_METASTATS_LUN_OBJ_BLK_SIZE           0

#define DD_MS_DRAM_CACHE_LUN_BLK_SIZE                   DBO_MB_2_BLOCKS(1)
#define DD_MS_DRAM_CACHE_LUN_OBJ_BLK_SIZE               0

#define DD_MS_VAULT_FUTURE_EXP_BLK_SIZE                 1
#define DD_MS_VAULT_RAID_GROUP_OBJ_BLK_SIZE             0

#define DD_MS_NT_BOOT_SPA_PRIM_BLK_SIZE                 1
#define DD_MS_NT_BOOT_SPA_SEC_BLK_SIZE                  1
#define DD_MS_NT_BOOT_SPB_PRIM_BLK_SIZE                 1
#define DD_MS_NT_BOOT_SPB_SEC_BLK_SIZE                  1

#define DD_MS_NT_BOOT_SPA_PRIM_EXP_BLK_SIZE             0
#define DD_MS_NT_BOOT_SPA_SEC_EXP_BLK_SIZE              0
#define DD_MS_NT_BOOT_SPB_PRIM_EXP_BLK_SIZE             0
#define DD_MS_NT_BOOT_SPB_SEC_EXP_BLK_SIZE              0

#define DD_MS_UTIL_SPA_PRIM_BLK_SIZE                    1
#define DD_MS_UTIL_SPA_SEC_BLK_SIZE                     1
#define DD_MS_UTIL_SPB_PRIM_BLK_SIZE                    1
#define DD_MS_UTIL_SPB_SEC_BLK_SIZE                     1

#define DD_MS_UTIL_SPA_PRIM_EXP_BLK_SIZE                0
#define DD_MS_UTIL_SPA_SEC_EXP_BLK_SIZE                 0
#define DD_MS_UTIL_SPB_PRIM_EXP_BLK_SIZE                0
#define DD_MS_UTIL_SPB_SEC_EXP_BLK_SIZE                 0

#define DD_MS_ICA_IMAGE_REP_BLK_SIZE                    1
#define DD_MS_ICA_IMAGE_REP_EXP_BLK_SIZE                0
#define DD_MS_ICA_IMAGE_REP_PAD_BLK_SIZE                0

#define DD_MS_FUTURE_GROWTH_BLK_SIZE                    0
#define DD_MS_NON_LUN_RAID_GROUP_OBJ_BLK_SIZE           0

#else                  /* ====== HW ====== */

// DH Diagnostic area
#define DD_MS_DISK_DIAGNOSTIC_BLK_SIZE                  128

// The following takes us from the end of the DH diagnostic area
// to the beginning of the user bind area on non-vault frus.
#define DD_MS_DISK_DIAGNOSTIC_PAD_BLK_SIZE              0x7F80

#define DD_MS_DDBS_BLK_SIZE                             DBO_MB_2_BLOCKS(2)
#define DD_MS_DDBS_LUN_OBJ_BLK_SIZE                     0x1A00
#define DD_MS_BIOS_UCODE_BLK_SIZE                       DBO_MB_2_BLOCKS(16)
#define DD_MS_BIOS_LUN_OBJ_BLK_SIZE                     0x1A00
#define DD_MS_PROM_UCODE_BLK_SIZE                       DBO_MB_2_BLOCKS(32)
#define DD_MS_PROM_LUN_OBJ_BLK_SIZE                     0x1A00
#define DD_MS_GPS_FW_BLK_SIZE                           DBO_MB_2_BLOCKS(48)
#define DD_MS_GPS_FW_LUN_OBJ_BLK_SIZE                   0x1A00
#define DD_MS_FLARE_DB_BLK_SIZE                         0x1B0000
#define DD_MS_FLARE_DB_LUN_OBJ_BLK_SIZE                 0x1A00

#define DD_MS_ESP_LUN_BLK_SIZE                          DBO_MB_2_BLOCKS(100)
#define DD_MS_ESP_LUN_OBJ_BLK_SIZE                      0x1A00

#define DD_MS_VCX_LUN_0_BLK_SIZE                        DBO_MB_2_BLOCKS(11264)
#define DD_MS_VCX_LUN_0_OBJ_BLK_SIZE                    0x1A00
#define DD_MS_VCX_LUN_1_BLK_SIZE                        DBO_MB_2_BLOCKS(11264)
#define DD_MS_VCX_LUN_1_OBJ_BLK_SIZE                    0x1A00
#define DD_MS_VCX_LUN_2_BLK_SIZE                        0x155600
#define DD_MS_VCX_LUN_2_OBJ_BLK_SIZE                    0xA00
#define DD_MS_VCX_LUN_3_BLK_SIZE                        0x155600
#define DD_MS_VCX_LUN_3_OBJ_BLK_SIZE                    0xA00
#define DD_MS_VCX_LUN_4_BLK_SIZE                        DBO_MB_2_BLOCKS(2048)
#define DD_MS_VCX_LUN_4_OBJ_BLK_SIZE                    0x1A00
#define DD_MS_VCX_LUN_5_BLK_SIZE                        0x2AAAC00
#define DD_MS_VCX_LUN_5_OBJ_BLK_SIZE                    DBO_MB_2_BLOCKS(2)

#define DD_MS_CENTERA_LUN_BLK_SIZE                      DBO_MB_2_BLOCKS(10240)
#define DD_MS_CENTERA_LUN_OBJ_BLK_SIZE                  0x1A00

#define DD_MS_PSM_RESERVED_BLK_SIZE                     DBO_MB_2_BLOCKS(8192)
#define DD_MS_PSM_LUN_OBJ_BLK_SIZE                      0x1A00

#define DD_MS_DRAM_CACHE_LUN_BLK_SIZE                   DBO_MB_2_BLOCKS(100)
#define DD_MS_DRAM_CACHE_LUN_OBJ_BLK_SIZE               0x1A00
#define DD_MS_EFD_CACHE_LUN_BLK_SIZE                    DBO_MB_2_BLOCKS(100)
#define DD_MS_EFD_CACHE_LUN_OBJ_BLK_SIZE                0x1A00

#define DD_MS_FUTURE_EXP_BLK_SIZE                       0xFF600 /* 510.75 Mb */
#define DD_MS_PSM_RAID_GROUP_OBJ_BLK_SIZE               0x53800
#define DD_MS_ALIGN_BUFFER1_BLK_SIZE                    0x600

#define DD_MS_WIL_A_LUN_BLK_SIZE                        0x15600
#define DD_MS_WIL_A_LUN_OBJ_BLK_SIZE                    0xA00
#define DD_MS_WIL_B_LUN_BLK_SIZE                        0x15600
#define DD_MS_WIL_B_LUN_OBJ_BLK_SIZE                    0xA00
#define DD_MS_CPL_A_LUN_BLK_SIZE                        0xAAC00
#define DD_MS_CPL_A_LUN_OBJ_BLK_SIZE                    DBO_MB_2_BLOCKS(2)
#define DD_MS_CPL_B_LUN_BLK_SIZE                        0xAAC00
#define DD_MS_CPL_B_LUN_OBJ_BLK_SIZE                    DBO_MB_2_BLOCKS(2)

#define DD_MS_RAID_METADATA_LUN_BLK_SIZE                0x8600
#define DD_MS_RAID_METADATA_LUN_OBJ_BLK_SIZE            0xA00
#define DD_MS_RAID_METASTATS_LUN_BLK_SIZE               0x8600
#define DD_MS_RAID_METASTATS_LUN_OBJ_BLK_SIZE           0xA00

#define DD_MS_VAULT_RESERVED_BLK_SIZE                   DBO_MB_2_BLOCKS(10240)
#define DD_MS_VAULT_LUN_OBJECT_BLK_SIZE                 0xA00

                                                        /* 1000.5 Mb */
#define DD_MS_VAULT_FUTURE_EXP_BLK_SIZE                 0x1F4400

                                                        /* 0x1C000 */
#define DD_MS_VAULT_RAID_GROUP_OBJ_BLK_SIZE             DBO_MB_2_BLOCKS(56)

#define DD_MS_NT_BOOT_SPA_PRIM_BLK_SIZE                 DBO_MB_2_BLOCKS((84*1024))
#define DD_MS_NT_BOOT_SPA_SEC_BLK_SIZE                  DBO_MB_2_BLOCKS((84*1024))
#define DD_MS_NT_BOOT_SPB_PRIM_BLK_SIZE                 DBO_MB_2_BLOCKS((84*1024))
#define DD_MS_NT_BOOT_SPB_SEC_BLK_SIZE                  DBO_MB_2_BLOCKS((84*1024))

#define DD_MS_NT_BOOT_SPA_PRIM_EXP_BLK_SIZE             0
#define DD_MS_NT_BOOT_SPA_SEC_EXP_BLK_SIZE              0
#define DD_MS_NT_BOOT_SPB_PRIM_EXP_BLK_SIZE             0
#define DD_MS_NT_BOOT_SPB_SEC_EXP_BLK_SIZE              0

#define DD_MS_UTIL_SPA_PRIM_BLK_SIZE                    DBO_MB_2_BLOCKS(10240)
#define DD_MS_UTIL_SPA_SEC_BLK_SIZE                     DBO_MB_2_BLOCKS(10240)
#define DD_MS_UTIL_SPB_PRIM_BLK_SIZE                    DBO_MB_2_BLOCKS(10240)
#define DD_MS_UTIL_SPB_SEC_BLK_SIZE                     DBO_MB_2_BLOCKS(10240)

#define DD_MS_UTIL_SPA_PRIM_EXP_BLK_SIZE                0
#define DD_MS_UTIL_SPA_SEC_EXP_BLK_SIZE                 0
#define DD_MS_UTIL_SPB_PRIM_EXP_BLK_SIZE                0
#define DD_MS_UTIL_SPB_SEC_EXP_BLK_SIZE                 0

#define DD_MS_ICA_IMAGE_REP_BLK_SIZE                    DBO_MB_2_BLOCKS(10240)
#define DD_MS_ICA_IMAGE_REP_EXP_BLK_SIZE                0
#define DD_MS_ICA_IMAGE_REP_PAD_BLK_SIZE                0

#define DD_MS_FUTURE_GROWTH_BLK_SIZE                    DBO_MB_2_BLOCKS(512)
#define DD_MS_NON_LUN_RAID_GROUP_OBJ_BLK_SIZE           0x57000

#endif /* #if !defined(UMODE_ENV) */

/* The following defines were added to support Mamba code in the convergence
 * branch. It needs to remove when we clear the Mamba specific code from AEP.
 */
#define DD_MS_EXPANDER_UCODE_BLK_SIZE                   0
#define DD_MS_BOOT_UCODE_BLK_SIZE                       0
#define DD_MS_ISTR_UCODE_BLK_SIZE                       0
#define DD_MS_CPLD_UCODE_BLK_SIZE                       0
#define DD_MS_MUX_UCODE_BLK_SIZE                        0
#define DD_MS_CC_UCODE_BLK_SIZE                         0
#define DD_MS_DPE_MC_UCODE_BLK_SIZE                     0
#define DD_MS_DAE_MC_UCODE_BLK_SIZE                     0
#define DD_MS_LSI_UCODE_BLK_SIZE                        0

/* Size of Legacy Flare Database (present on all FRUs) */
#define DD_MS_USER_DB1_BLK_SIZE     ((DD_MS_FRU_SIGNATURE_BLOCKS_PER_ENTRY * DD_MS_FRU_SIGNATURE_NUM_ENTRIES) + \
                                     (DD_MS_FRU_SIGNATURE_PAD_BLOCKS_PER_ENTRY * DD_MS_FRU_SIGNATURE_PAD_NUM_ENTRIES) + \
                                     (DD_MS_HW_FRU_VERIFY_BLOCKS_PER_ENTRY * DD_MS_HW_FRU_VERIFY_NUM_ENTRIES) + \
                                     (DD_MS_HW_FRU_VERIFY_PAD_BLOCKS_PER_ENTRY * DD_MS_HW_FRU_VERIFY_PAD_NUM_ENTRIES) + \
                                     (DD_MS_EXPANSION_CHKPT_BLOCKS_PER_ENTRY * DD_MS_EXPANSION_CHKPT_NUM_ENTRIES) + \
                                     (DD_MS_EXPANSION_CHKPT_PAD_BLOCKS_PER_ENTRY * DD_MS_EXPANSION_CHKPT_PAD_NUM_ENTRIES) + \
                                     (DD_MS_CLEAN_DIRTY_BLOCKS_PER_ENTRY * DD_MS_CLEAN_DIRTY_NUM_ENTRIES) + \
                                     (DD_MS_PADDER_BLOCKS_PER_ENTRY * DD_MS_PADDER_NUM_ENTRIES))

#define DD_MS_USER_DB_BLK_SIZE                          DD_MS_USER_DB1_BLK_SIZE


/* Area taken up by EXTERNAL class storage preceding the Flare "3 disk" database area. */
#define DD_MS_EXTERNAL_DUMMY_BLK_SIZE                   (DD_MS_DISK_DIAGNOSTIC_BLK_SIZE + \
                                                         DD_MS_DISK_DIAGNOSTIC_PAD_BLK_SIZE + \
                                                         DD_MS_DDBS_BLK_SIZE + DD_MS_DDBS_LUN_OBJ_BLK_SIZE +\
                                                         DD_MS_BIOS_UCODE_BLK_SIZE + DD_MS_BIOS_LUN_OBJ_BLK_SIZE + \
                                                         DD_MS_PROM_UCODE_BLK_SIZE + DD_MS_PROM_LUN_OBJ_BLK_SIZE + \
                                                         DD_MS_GPS_FW_BLK_SIZE + DD_MS_GPS_FW_LUN_OBJ_BLK_SIZE)

// Area used by the Flare "3 disk" database area.
#define DD_MS_FLARE_LEGACY_DB_BLK_SIZE                  (DD_MS_SYSCONFIG_NUM_ENTRIES * DD_MS_SYSCONFIG_BLOCKS_PER_ENTRY) + \
                                                        (DD_MS_SYSCONFIG_PAD_NUM_ENTRIES * DD_MS_SYSCONFIG_PAD_BLOCKS_PER_ENTRY) + \
                                                        (DD_MS_GLUT_NUM_ENTRIES * DD_MS_GLUT_BLOCKS_PER_ENTRY) + \
                                                        (DD_MS_GLUT_PAD_NUM_ENTRIES * DD_MS_GLUT_PAD_BLOCKS_PER_ENTRY) + \
                                                        (DD_MS_FRU_NUM_ENTRIES * DD_MS_FRU_BLOCKS_PER_ENTRY) + \
                                                        (DD_MS_FRU_PAD_NUM_ENTRIES * DD_MS_FRU_PAD_BLOCKS_PER_ENTRY) + \
                                                        (DD_MS_RAID_GROUP_NUM_ENTRIES * DD_MS_RAID_GROUP_BLOCKS_PER_ENTRY) + \
                                                        (DD_MS_RAID_GROUP_PAD_NUM_ENTRIES * DD_MS_RAID_GROUP_PAD_BLOCKS_PER_ENTRY) + \
                                                        (DD_MS_VERIFY_RESULTS_NUM_ENTRIES * DD_MS_VERIFY_RESULTS_BLOCKS_PER_ENTRY) + \
                                                        (DD_MS_VERIFY_RESULTS_PAD_NUM_ENTRIES * DD_MS_VERIFY_RESULTS_PAD_BLOCKS_PER_ENTRY) + \
                                                        (DD_MS_ENCL_TBL_NUM_ENTRIES * DD_MS_ENCL_TBL_BLOCKS_PER_ENTRY) + \
                                                        (DD_MS_ENCL_TBL_PAD_NUM_ENTRIES * DD_MS_ENCL_TBL_PAD_BLOCKS_PER_ENTRY) + \
                                                        (DD_MS_RESERVED_EXP_NUM_ENTRIES * DD_MS_RESERVED_EXP_BLOCKS_PER_ENTRY)

// Area used by the Flare "3 disk" database area.
#define DD_MS_USER_DB2_BLK_SIZE                         (DD_MS_FLARE_LEGACY_DB_BLK_SIZE + DD_MS_FLARE_DB_BLK_SIZE)


// Size of a dummy region on disk 3 to occupy the same space used by the PSM raid group
// on disks 0 - 2 so the Vault raid group and subsequent boot & utility partitions start
// at the same LBA for all disks 0 - 3.
#define DD_MS_EXT_NEW_DB_DUMMY_BLK_SIZE                 (DD_MS_DDBS_BLK_SIZE + DD_MS_DDBS_LUN_OBJ_BLK_SIZE + \
                                                         DD_MS_BIOS_UCODE_BLK_SIZE + DD_MS_BIOS_LUN_OBJ_BLK_SIZE + \
                                                         DD_MS_PROM_UCODE_BLK_SIZE + DD_MS_PROM_LUN_OBJ_BLK_SIZE + \
                                                         DD_MS_GPS_FW_BLK_SIZE + DD_MS_GPS_FW_LUN_OBJ_BLK_SIZE + \
                                                         DD_MS_USER_DB2_BLK_SIZE + DD_MS_FLARE_DB_LUN_OBJ_BLK_SIZE + \
                                                         DD_MS_ESP_LUN_BLK_SIZE + DD_MS_ESP_LUN_OBJ_BLK_SIZE + \
                                                         DD_MS_VCX_LUN_0_BLK_SIZE + DD_MS_VCX_LUN_0_OBJ_BLK_SIZE + \
                                                         DD_MS_VCX_LUN_1_BLK_SIZE + DD_MS_VCX_LUN_1_OBJ_BLK_SIZE + \
                                                         DD_MS_VCX_LUN_4_BLK_SIZE + DD_MS_VCX_LUN_4_OBJ_BLK_SIZE + \
                                                         DD_MS_CENTERA_LUN_BLK_SIZE + DD_MS_CENTERA_LUN_OBJ_BLK_SIZE + \
                                                         DD_MS_PSM_RESERVED_BLK_SIZE + DD_MS_PSM_LUN_OBJ_BLK_SIZE + \
                                                         DD_MS_DRAM_CACHE_LUN_BLK_SIZE + DD_MS_DRAM_CACHE_LUN_OBJ_BLK_SIZE + \
                                                         DD_MS_EFD_CACHE_LUN_BLK_SIZE + DD_MS_EFD_CACHE_LUN_OBJ_BLK_SIZE + \
                                                         DD_MS_FUTURE_EXP_BLK_SIZE + DD_MS_PSM_RAID_GROUP_OBJ_BLK_SIZE + DD_MS_ALIGN_BUFFER1_BLK_SIZE)

// Size of a dummy region on disk 3 to occupy the same space used in boot/utility/image repository areas
// on disks 0 - 2 so the areas after them start at the same LBA for all disks 0 - 3.
#define DD_MS_UTIL_FLARE_DUMMY_BLK_SIZE                  DD_MS_ICA_IMAGE_REP_BLK_SIZE


// Size of External Use type space on all FRUs (vault and non-vault)
#define DD_MS_EXTERNAL_USE_ALL_FRUS_BLK_SIZE            (DD_MS_DISK_DIAGNOSTIC_BLK_SIZE + \
                                                         DD_MS_DISK_DIAGNOSTIC_PAD_BLK_SIZE)

// Size of External Use type space on all vault FRUs (0 - 3)
// Do not include DD_MS_USER_DB2_BLK_SIZE in the following as that is User type space.
#define DD_MS_EXTERNAL_USE_VAULT_FRUS_TOTAL_BLK_SIZE    (DD_MS_EXTERNAL_USE_ALL_FRUS_BLK_SIZE + \
                                                         DD_MS_DDBS_BLK_SIZE + DD_MS_DDBS_LUN_OBJ_BLK_SIZE + \
                                                         DD_MS_BIOS_UCODE_BLK_SIZE + DD_MS_BIOS_LUN_OBJ_BLK_SIZE + \
                                                         DD_MS_PROM_UCODE_BLK_SIZE + DD_MS_PROM_LUN_OBJ_BLK_SIZE + \
                                                         DD_MS_GPS_FW_BLK_SIZE + DD_MS_GPS_FW_LUN_OBJ_BLK_SIZE + \
                                                         DD_MS_FLARE_DB_LUN_OBJ_BLK_SIZE + \
                                                         DD_MS_ESP_LUN_BLK_SIZE + DD_MS_ESP_LUN_OBJ_BLK_SIZE + \
                                                         DD_MS_VCX_LUN_0_BLK_SIZE + DD_MS_VCX_LUN_0_OBJ_BLK_SIZE + \
                                                         DD_MS_VCX_LUN_1_BLK_SIZE + DD_MS_VCX_LUN_1_OBJ_BLK_SIZE + \
                                                         DD_MS_VCX_LUN_4_BLK_SIZE + DD_MS_VCX_LUN_4_OBJ_BLK_SIZE + \
                                                         DD_MS_CENTERA_LUN_BLK_SIZE + DD_MS_CENTERA_LUN_OBJ_BLK_SIZE + \
                                                         DD_MS_PSM_RESERVED_BLK_SIZE + DD_MS_PSM_LUN_OBJ_BLK_SIZE + \
                                                         DD_MS_DRAM_CACHE_LUN_BLK_SIZE + DD_MS_DRAM_CACHE_LUN_OBJ_BLK_SIZE + \
                                                         DD_MS_EFD_CACHE_LUN_BLK_SIZE + DD_MS_EFD_CACHE_LUN_OBJ_BLK_SIZE + \
                                                         DD_MS_FUTURE_EXP_BLK_SIZE + DD_MS_PSM_RAID_GROUP_OBJ_BLK_SIZE + DD_MS_ALIGN_BUFFER1_BLK_SIZE + \
                                                         DD_MS_WIL_A_LUN_BLK_SIZE + DD_MS_WIL_A_LUN_OBJ_BLK_SIZE + \
                                                         DD_MS_WIL_B_LUN_BLK_SIZE + DD_MS_WIL_B_LUN_OBJ_BLK_SIZE + \
                                                         DD_MS_CPL_A_LUN_BLK_SIZE + DD_MS_CPL_A_LUN_OBJ_BLK_SIZE + \
                                                         DD_MS_CPL_B_LUN_BLK_SIZE + DD_MS_CPL_B_LUN_OBJ_BLK_SIZE + \
                                                         DD_MS_VCX_LUN_2_BLK_SIZE + DD_MS_VCX_LUN_2_OBJ_BLK_SIZE + \
                                                         DD_MS_VCX_LUN_3_BLK_SIZE + DD_MS_VCX_LUN_3_OBJ_BLK_SIZE + \
                                                         DD_MS_VCX_LUN_5_BLK_SIZE + DD_MS_VCX_LUN_5_OBJ_BLK_SIZE + \
                                                         DD_MS_RAID_METADATA_LUN_BLK_SIZE + DD_MS_RAID_METADATA_LUN_OBJ_BLK_SIZE + \
                                                         DD_MS_RAID_METASTATS_LUN_BLK_SIZE + DD_MS_RAID_METASTATS_LUN_OBJ_BLK_SIZE + \
                                                         DD_MS_VAULT_RESERVED_BLK_SIZE + DD_MS_VAULT_LUN_OBJECT_BLK_SIZE + \
                                                         DD_MS_VAULT_FUTURE_EXP_BLK_SIZE + DD_MS_VAULT_RAID_GROUP_OBJ_BLK_SIZE + \
                                                         DD_MS_NT_BOOT_SPA_PRIM_BLK_SIZE + DD_MS_NT_BOOT_SPA_PRIM_EXP_BLK_SIZE + \
                                                         DD_MS_UTIL_SPA_PRIM_BLK_SIZE + DD_MS_UTIL_SPA_PRIM_EXP_BLK_SIZE + \
                                                         DD_MS_ICA_IMAGE_REP_BLK_SIZE + DD_MS_ICA_IMAGE_REP_EXP_BLK_SIZE + \
                                                         DD_MS_FUTURE_GROWTH_BLK_SIZE + DD_MS_NON_LUN_RAID_GROUP_OBJ_BLK_SIZE)


/*************************************************************
 * EXT USE DB SET sizes for VM Simulation: each size defines number 
 * of blocks allocated by DB data set (set is indicated by DDE).
 *************************************************************
 */

/* VM Simulator specific LUNs sizes were compared and made consistent in the 
 * cm_aplu.h file.
 */

// DH Diagnostic area
#define DD_MS_VSIM_DISK_DIAGNOSTIC_BLK_SIZE                  128

// The following takes us from the end of the DH diagnostic area
// to the beginning of the user bind area on non-vault frus.
#define DD_MS_VSIM_DISK_DIAGNOSTIC_PAD_BLK_SIZE              0x7F80

#define DD_MS_VSIM_DDBS_BLK_SIZE                             DBO_MB_2_BLOCKS(2)
#define DD_MS_VSIM_DDBS_LUN_OBJ_BLK_SIZE                     0x1A00
#define DD_MS_VSIM_BIOS_UCODE_BLK_SIZE                       DBO_MB_2_BLOCKS(2) //DBO_MB_2_BLOCKS(16)
#define DD_MS_VSIM_BIOS_LUN_OBJ_BLK_SIZE                     0x1A00
#define DD_MS_VSIM_PROM_UCODE_BLK_SIZE                       DBO_MB_2_BLOCKS(2) //DBO_MB_2_BLOCKS(32)
#define DD_MS_VSIM_PROM_LUN_OBJ_BLK_SIZE                     0x1A00
#define DD_MS_VSIM_GPS_FW_BLK_SIZE                           DBO_MB_2_BLOCKS(2) //DBO_MB_2_BLOCKS(48)
#define DD_MS_VSIM_GPS_FW_LUN_OBJ_BLK_SIZE                   0x1A00
#define DD_MS_VSIM_FLARE_DB_BLK_SIZE                         0x1B0000
#define DD_MS_VSIM_FLARE_DB_LUN_OBJ_BLK_SIZE                 0x1A00

#define DD_MS_VSIM_ESP_LUN_BLK_SIZE                          DBO_MB_2_BLOCKS(100)
#define DD_MS_VSIM_ESP_LUN_OBJ_BLK_SIZE                      0x1A00

#define DD_MS_VSIM_VCX_LUN_0_BLK_SIZE                        DBO_MB_2_BLOCKS(512) //DBO_MB_2_BLOCKS(11264)
#define DD_MS_VSIM_VCX_LUN_0_OBJ_BLK_SIZE                    0x1A00
#define DD_MS_VSIM_VCX_LUN_1_BLK_SIZE                        DBO_MB_2_BLOCKS(512) //DBO_MB_2_BLOCKS(1024) 
#define DD_MS_VSIM_VCX_LUN_1_OBJ_BLK_SIZE                    0x1A00

#ifndef C4_INTEGRATED
#define DD_MS_VSIM_VCX_LUN_2_BLK_SIZE                        0x155600
#define DD_MS_VSIM_VCX_LUN_2_OBJ_BLK_SIZE                    0xA00
#else
#define DD_MS_VSIM_VCX_LUN_2_BLK_SIZE                        DBO_MB_2_BLOCKS(512) //0x155600
#define DD_MS_VSIM_VCX_LUN_2_OBJ_BLK_SIZE                    0xA00
#endif /* C4_INTEGRATED - C4HW - disk layout */

#ifndef C4_INTEGRATED
#define DD_MS_VSIM_VCX_LUN_3_BLK_SIZE                        0x155600
#define DD_MS_VSIM_VCX_LUN_3_OBJ_BLK_SIZE                    0xA00
#else
#define DD_MS_VSIM_VCX_LUN_3_BLK_SIZE                        DBO_MB_2_BLOCKS(512) //0x155600
#define DD_MS_VSIM_VCX_LUN_3_OBJ_BLK_SIZE                    0xA00
#endif /* C4_INTEGRATED - C4HW - disk layout */

#define DD_MS_VSIM_VCX_LUN_4_BLK_SIZE                        DBO_MB_2_BLOCKS(512) //DBO_MB_2_BLOCKS(2048)
#define DD_MS_VSIM_VCX_LUN_4_OBJ_BLK_SIZE                    0x1A00
#define DD_MS_VSIM_VCX_LUN_5_BLK_SIZE                        DBO_MB_2_BLOCKS(512)//0x2AAAC00
#define DD_MS_VSIM_VCX_LUN_5_OBJ_BLK_SIZE                    DBO_MB_2_BLOCKS(2)

#ifndef C4_INTEGRATED
#define DD_MS_VSIM_CENTERA_LUN_BLK_SIZE                      DBO_MB_2_BLOCKS(500) //DBO_MB_2_BLOCKS(1024) //DBO_MB_2_BLOCKS(10240)
#else
#define DD_MS_VSIM_CENTERA_LUN_BLK_SIZE                      DBO_MB_2_BLOCKS(10)
#endif /* C4_INTEGRATED - C4HW - disk layout */

#define DD_MS_VSIM_CENTERA_LUN_OBJ_BLK_SIZE                  0x1A00

#ifndef C4_INTEGRATED
#define DD_MS_VSIM_PSM_RESERVED_BLK_SIZE                     DBO_MB_2_BLOCKS(8192)
#else
#define DD_MS_VSIM_PSM_RESERVED_BLK_SIZE                     DBO_MB_2_BLOCKS(1920) /* The KH simulator need a larger PSM */
#endif /* C4_INTEGRATED - C4HW - disk layout */

#define DD_MS_VSIM_PSM_LUN_OBJ_BLK_SIZE                      0x1A00

#ifndef C4_INTEGRATED
#define DD_MS_VSIM_DRAM_CACHE_LUN_BLK_SIZE                   DBO_MB_2_BLOCKS(100)
#else
#define DD_MS_VSIM_DRAM_CACHE_LUN_BLK_SIZE                   DBO_MB_2_BLOCKS(1)
#endif /* C4_INTEGRATED - C4HW - disk layout */

#define DD_MS_VSIM_DRAM_CACHE_LUN_OBJ_BLK_SIZE               0x1A00

#ifndef C4_INTEGRATED
#define DD_MS_VSIM_EFD_CACHE_LUN_BLK_SIZE                    DBO_MB_2_BLOCKS(100)
#else
#define DD_MS_VSIM_EFD_CACHE_LUN_BLK_SIZE                    DBO_MB_2_BLOCKS(1)
#endif /* C4_INTEGRATED - C4HW - disk layout */

#define DD_MS_VSIM_EFD_CACHE_LUN_OBJ_BLK_SIZE                0x1A00

#ifndef C4_INTEGRATED
#define DD_MS_VSIM_FUTURE_EXP_BLK_SIZE                       0xFF600 /* 510.75 Mb */
#else
#define DD_MS_VSIM_FUTURE_EXP_BLK_SIZE                       DBO_MB_2_BLOCKS(1)
#endif /* C4_INTEGRATED - C4HW - disk layout */

#define DD_MS_VSIM_PSM_RAID_GROUP_OBJ_BLK_SIZE               0x53800
#define DD_MS_VSIM_ALIGN_BUFFER1_BLK_SIZE                    0x600

#ifndef C4_INTEGRATED
#define DD_MS_VSIM_WIL_A_LUN_BLK_SIZE                        0x15600
#define DD_MS_VSIM_WIL_A_LUN_OBJ_BLK_SIZE                    0xA00
#define DD_MS_VSIM_WIL_B_LUN_BLK_SIZE                        0x15600
#define DD_MS_VSIM_WIL_B_LUN_OBJ_BLK_SIZE                    0xA00
#define DD_MS_VSIM_CPL_A_LUN_BLK_SIZE                        0xAAC00
#define DD_MS_VSIM_CPL_A_LUN_OBJ_BLK_SIZE                    DBO_MB_2_BLOCKS(2)
#define DD_MS_VSIM_CPL_B_LUN_BLK_SIZE                        0xAAC00
#define DD_MS_VSIM_CPL_B_LUN_OBJ_BLK_SIZE                    DBO_MB_2_BLOCKS(2)
#else
#define DD_MS_VSIM_WIL_A_LUN_BLK_SIZE                        DBO_MB_2_BLOCKS(1)
#define DD_MS_VSIM_WIL_A_LUN_OBJ_BLK_SIZE                    0xA00
#define DD_MS_VSIM_WIL_B_LUN_BLK_SIZE                        DBO_MB_2_BLOCKS(1)
#define DD_MS_VSIM_WIL_B_LUN_OBJ_BLK_SIZE                    0xA00
#define DD_MS_VSIM_CPL_A_LUN_BLK_SIZE                        DBO_MB_2_BLOCKS(1)
#define DD_MS_VSIM_CPL_A_LUN_OBJ_BLK_SIZE                    DBO_MB_2_BLOCKS(1)
#define DD_MS_VSIM_CPL_B_LUN_BLK_SIZE                        DBO_MB_2_BLOCKS(1)
#define DD_MS_VSIM_CPL_B_LUN_OBJ_BLK_SIZE                    DBO_MB_2_BLOCKS(1)
#endif /* C4_INTEGRATED - C4HW - disk layout */

#define DD_MS_VSIM_RAID_METADATA_LUN_BLK_SIZE                0x8600
#define DD_MS_VSIM_RAID_METADATA_LUN_OBJ_BLK_SIZE            0xA00
#define DD_MS_VSIM_RAID_METASTATS_LUN_BLK_SIZE               0x8600
#define DD_MS_VSIM_RAID_METASTATS_LUN_OBJ_BLK_SIZE           0xA00

#ifndef C4_INTEGRATED
#define DD_MS_VSIM_VAULT_RESERVED_BLK_SIZE                   DBO_MB_2_BLOCKS(10240)
#else
#define DD_MS_VSIM_VAULT_RESERVED_BLK_SIZE                   DBO_MB_2_BLOCKS(256)
#endif /* C4_INTEGRATED - C4HW - disk layout */

#define DD_MS_VSIM_VAULT_LUN_OBJECT_BLK_SIZE                 0xA00

#ifndef C4_INTEGRATED
                                                        /* 1000.5 Mb */
#define DD_MS_VSIM_VAULT_FUTURE_EXP_BLK_SIZE                 0x1F4400
#else
#define DD_MS_VSIM_VAULT_FUTURE_EXP_BLK_SIZE                 DBO_MB_2_BLOCKS(2)
#endif /* C4_INTEGRATED - C4HW - disk layout */
                                                        /* 0x1C000 */
#define DD_MS_VSIM_VAULT_RAID_GROUP_OBJ_BLK_SIZE             DBO_MB_2_BLOCKS(56)

#define DD_MS_VSIM_NT_BOOT_SPA_PRIM_BLK_SIZE                 1 //DBO_MB_2_BLOCKS((84*1024))
#define DD_MS_VSIM_NT_BOOT_SPA_SEC_BLK_SIZE                  1 //DBO_MB_2_BLOCKS((84*1024))
#define DD_MS_VSIM_NT_BOOT_SPB_PRIM_BLK_SIZE                 1 //DBO_MB_2_BLOCKS((84*1024))
#define DD_MS_VSIM_NT_BOOT_SPB_SEC_BLK_SIZE                  1 //DBO_MB_2_BLOCKS((84*1024))

#define DD_MS_VSIM_NT_BOOT_SPA_PRIM_EXP_BLK_SIZE             0
#define DD_MS_VSIM_NT_BOOT_SPA_SEC_EXP_BLK_SIZE              0
#define DD_MS_VSIM_NT_BOOT_SPB_PRIM_EXP_BLK_SIZE             0
#define DD_MS_VSIM_NT_BOOT_SPB_SEC_EXP_BLK_SIZE              0

#define DD_MS_VSIM_UTIL_SPA_PRIM_BLK_SIZE                    1 //DBO_MB_2_BLOCKS(10240)
#define DD_MS_VSIM_UTIL_SPA_SEC_BLK_SIZE                     1 //DBO_MB_2_BLOCKS(10240)
#define DD_MS_VSIM_UTIL_SPB_PRIM_BLK_SIZE                    1 //DBO_MB_2_BLOCKS(10240)
#define DD_MS_VSIM_UTIL_SPB_SEC_BLK_SIZE                     1 //DBO_MB_2_BLOCKS(10240)

#define DD_MS_VSIM_UTIL_SPA_PRIM_EXP_BLK_SIZE                0
#define DD_MS_VSIM_UTIL_SPA_SEC_EXP_BLK_SIZE                 0
#define DD_MS_VSIM_UTIL_SPB_PRIM_EXP_BLK_SIZE                0
#define DD_MS_VSIM_UTIL_SPB_SEC_EXP_BLK_SIZE                 0


#define DD_MS_VSIM_ICA_IMAGE_REP_BLK_SIZE                    1 //DBO_MB_2_BLOCKS(10240)
#define DD_MS_VSIM_ICA_IMAGE_REP_EXP_BLK_SIZE                0
#define DD_MS_VSIM_ICA_IMAGE_REP_PAD_BLK_SIZE                0

#define DD_MS_VSIM_FUTURE_GROWTH_BLK_SIZE                    1 //DBO_MB_2_BLOCKS(512)
#define DD_MS_VSIM_NON_LUN_RAID_GROUP_OBJ_BLK_SIZE           1 //0x57000


/* The following defines were added to support Mamba code in the convergence
 * branch. It needs to remove when we clear the Mamba specific code from AEP.
 */
#define DD_MS_VSIM_EXPANDER_UCODE_BLK_SIZE                   0
#define DD_MS_VSIM_BOOT_UCODE_BLK_SIZE                       0
#define DD_MS_VSIM_ISTR_UCODE_BLK_SIZE                       0
#define DD_MS_VSIM_CPLD_UCODE_BLK_SIZE                       0
#define DD_MS_VSIM_MUX_UCODE_BLK_SIZE                        0
#define DD_MS_VSIM_CC_UCODE_BLK_SIZE                         0
#define DD_MS_VSIM_DPE_MC_UCODE_BLK_SIZE                     0
#define DD_MS_VSIM_DAE_MC_UCODE_BLK_SIZE                     0
#define DD_MS_VSIM_LSI_UCODE_BLK_SIZE                        0

/* Size of Legacy Flare Database (present on all FRUs) */
#define DD_MS_VSIM_USER_DB1_BLK_SIZE     ((DD_MS_FRU_SIGNATURE_BLOCKS_PER_ENTRY * DD_MS_FRU_SIGNATURE_NUM_ENTRIES) + \
                                     (DD_MS_FRU_SIGNATURE_PAD_BLOCKS_PER_ENTRY * DD_MS_FRU_SIGNATURE_PAD_NUM_ENTRIES) + \
                                     (DD_MS_HW_FRU_VERIFY_BLOCKS_PER_ENTRY * DD_MS_HW_FRU_VERIFY_NUM_ENTRIES) + \
                                     (DD_MS_HW_FRU_VERIFY_PAD_BLOCKS_PER_ENTRY * DD_MS_HW_FRU_VERIFY_PAD_NUM_ENTRIES) + \
                                     (DD_MS_EXPANSION_CHKPT_BLOCKS_PER_ENTRY * DD_MS_EXPANSION_CHKPT_NUM_ENTRIES) + \
                                     (DD_MS_EXPANSION_CHKPT_PAD_BLOCKS_PER_ENTRY * DD_MS_EXPANSION_CHKPT_PAD_NUM_ENTRIES) + \
                                     (DD_MS_CLEAN_DIRTY_BLOCKS_PER_ENTRY * DD_MS_CLEAN_DIRTY_NUM_ENTRIES) + \
                                     (DD_MS_PADDER_BLOCKS_PER_ENTRY * DD_MS_PADDER_NUM_ENTRIES))

#define DD_MS_VSIM_USER_DB_BLK_SIZE                          DD_MS_VSIM_USER_DB1_BLK_SIZE


/* Area taken up by EXTERNAL class storage preceding the Flare "3 disk" database area. */
#define DD_MS_VSIM_EXTERNAL_DUMMY_BLK_SIZE              (DD_MS_VSIM_DISK_DIAGNOSTIC_BLK_SIZE + \
                                                         DD_MS_VSIM_DISK_DIAGNOSTIC_PAD_BLK_SIZE + \
                                                         DD_MS_VSIM_DDBS_BLK_SIZE + DD_MS_VSIM_DDBS_LUN_OBJ_BLK_SIZE +\
                                                         DD_MS_VSIM_BIOS_UCODE_BLK_SIZE + DD_MS_VSIM_BIOS_LUN_OBJ_BLK_SIZE + \
                                                         DD_MS_VSIM_PROM_UCODE_BLK_SIZE + DD_MS_VSIM_PROM_LUN_OBJ_BLK_SIZE + \
                                                         DD_MS_VSIM_GPS_FW_BLK_SIZE + DD_MS_VSIM_GPS_FW_LUN_OBJ_BLK_SIZE)

// Area used by the Flare "3 disk" database area.
#define DD_MS_VSIM_FLARE_LEGACY_DB_BLK_SIZE             (DD_MS_SYSCONFIG_NUM_ENTRIES * DD_MS_SYSCONFIG_BLOCKS_PER_ENTRY) + \
                                                        (DD_MS_SYSCONFIG_PAD_NUM_ENTRIES * DD_MS_SYSCONFIG_PAD_BLOCKS_PER_ENTRY) + \
                                                        (DD_MS_GLUT_NUM_ENTRIES * DD_MS_GLUT_BLOCKS_PER_ENTRY) + \
                                                        (DD_MS_GLUT_PAD_NUM_ENTRIES * DD_MS_GLUT_PAD_BLOCKS_PER_ENTRY) + \
                                                        (DD_MS_FRU_NUM_ENTRIES * DD_MS_FRU_BLOCKS_PER_ENTRY) + \
                                                        (DD_MS_FRU_PAD_NUM_ENTRIES * DD_MS_FRU_PAD_BLOCKS_PER_ENTRY) + \
                                                        (DD_MS_RAID_GROUP_NUM_ENTRIES * DD_MS_RAID_GROUP_BLOCKS_PER_ENTRY) + \
                                                        (DD_MS_RAID_GROUP_PAD_NUM_ENTRIES * DD_MS_RAID_GROUP_PAD_BLOCKS_PER_ENTRY) + \
                                                        (DD_MS_VERIFY_RESULTS_NUM_ENTRIES * DD_MS_VERIFY_RESULTS_BLOCKS_PER_ENTRY) + \
                                                        (DD_MS_VERIFY_RESULTS_PAD_NUM_ENTRIES * DD_MS_VERIFY_RESULTS_PAD_BLOCKS_PER_ENTRY) + \
                                                        (DD_MS_ENCL_TBL_NUM_ENTRIES * DD_MS_ENCL_TBL_BLOCKS_PER_ENTRY) + \
                                                        (DD_MS_ENCL_TBL_PAD_NUM_ENTRIES * DD_MS_ENCL_TBL_PAD_BLOCKS_PER_ENTRY) + \
                                                        (DD_MS_RESERVED_EXP_NUM_ENTRIES * DD_MS_RESERVED_EXP_BLOCKS_PER_ENTRY)

// Area used by the Flare "3 disk" database area.
#define DD_MS_VSIM_USER_DB2_BLK_SIZE                    (DD_MS_VSIM_FLARE_LEGACY_DB_BLK_SIZE + DD_MS_VSIM_FLARE_DB_BLK_SIZE)


// Size of a dummy region on disk 3 to occupy the same space used by the PSM raid group
// on disks 0 - 2 so the Vault raid group and subsequent boot & utility partitions start
// at the same LBA for all disks 0 - 3.
#define DD_MS_VSIM_EXT_NEW_DB_DUMMY_BLK_SIZE            (DD_MS_VSIM_DDBS_BLK_SIZE + DD_MS_VSIM_DDBS_LUN_OBJ_BLK_SIZE + \
                                                         DD_MS_VSIM_BIOS_UCODE_BLK_SIZE + DD_MS_VSIM_BIOS_LUN_OBJ_BLK_SIZE + \
                                                         DD_MS_VSIM_PROM_UCODE_BLK_SIZE + DD_MS_VSIM_PROM_LUN_OBJ_BLK_SIZE + \
                                                         DD_MS_VSIM_GPS_FW_BLK_SIZE + DD_MS_VSIM_GPS_FW_LUN_OBJ_BLK_SIZE + \
                                                         DD_MS_VSIM_USER_DB2_BLK_SIZE + DD_MS_VSIM_FLARE_DB_LUN_OBJ_BLK_SIZE + \
                                                         DD_MS_VSIM_ESP_LUN_BLK_SIZE + DD_MS_VSIM_ESP_LUN_OBJ_BLK_SIZE + \
                                                         DD_MS_VSIM_VCX_LUN_0_BLK_SIZE + DD_MS_VSIM_VCX_LUN_0_OBJ_BLK_SIZE + \
                                                         DD_MS_VSIM_VCX_LUN_1_BLK_SIZE + DD_MS_VSIM_VCX_LUN_1_OBJ_BLK_SIZE + \
                                                         DD_MS_VSIM_VCX_LUN_4_BLK_SIZE + DD_MS_VSIM_VCX_LUN_4_OBJ_BLK_SIZE + \
                                                         DD_MS_VSIM_CENTERA_LUN_BLK_SIZE + DD_MS_VSIM_CENTERA_LUN_OBJ_BLK_SIZE + \
                                                         DD_MS_VSIM_PSM_RESERVED_BLK_SIZE + DD_MS_VSIM_PSM_LUN_OBJ_BLK_SIZE + \
                                                         DD_MS_VSIM_DRAM_CACHE_LUN_BLK_SIZE + DD_MS_VSIM_DRAM_CACHE_LUN_OBJ_BLK_SIZE + \
                                                         DD_MS_VSIM_EFD_CACHE_LUN_BLK_SIZE + DD_MS_VSIM_EFD_CACHE_LUN_OBJ_BLK_SIZE + \
                                                         DD_MS_VSIM_FUTURE_EXP_BLK_SIZE + DD_MS_VSIM_PSM_RAID_GROUP_OBJ_BLK_SIZE + DD_MS_VSIM_ALIGN_BUFFER1_BLK_SIZE)

// Size of a dummy region on disk 3 to occupy the same space used in boot/utility/image repository areas
// on disks 0 - 2 so the areas after them start at the same LBA for all disks 0 - 3.
#define DD_MS_VSIM_UTIL_FLARE_DUMMY_BLK_SIZE             DD_MS_VSIM_ICA_IMAGE_REP_BLK_SIZE


// Size of External Use type space on all FRUs (vault and non-vault)
#define DD_MS_VSIM_EXTERNAL_USE_ALL_FRUS_BLK_SIZE       (DD_MS_VSIM_DISK_DIAGNOSTIC_BLK_SIZE + \
                                                         DD_MS_VSIM_DISK_DIAGNOSTIC_PAD_BLK_SIZE)

// Size of External Use type space on all vault FRUs (0 - 3)
// Do not include DD_MS_USER_DB2_BLK_SIZE in the following as that is User type space.
#define DD_MS_VSIM_EXTERNAL_USE_VAULT_FRUS_TOTAL_BLK_SIZE    (DD_MS_VSIM_EXTERNAL_USE_ALL_FRUS_BLK_SIZE + \
                                                         DD_MS_VSIM_DDBS_BLK_SIZE + DD_MS_VSIM_DDBS_LUN_OBJ_BLK_SIZE + \
                                                         DD_MS_VSIM_BIOS_UCODE_BLK_SIZE + DD_MS_VSIM_BIOS_LUN_OBJ_BLK_SIZE + \
                                                         DD_MS_VSIM_PROM_UCODE_BLK_SIZE + DD_MS_VSIM_PROM_LUN_OBJ_BLK_SIZE + \
                                                         DD_MS_VSIM_GPS_FW_BLK_SIZE + DD_MS_VSIM_GPS_FW_LUN_OBJ_BLK_SIZE + \
                                                         DD_MS_VSIM_FLARE_DB_LUN_OBJ_BLK_SIZE + \
                                                         DD_MS_VSIM_ESP_LUN_BLK_SIZE + DD_MS_VSIM_ESP_LUN_OBJ_BLK_SIZE + \
                                                         DD_MS_VSIM_VCX_LUN_0_BLK_SIZE + DD_MS_VSIM_VCX_LUN_0_OBJ_BLK_SIZE + \
                                                         DD_MS_VSIM_VCX_LUN_1_BLK_SIZE + DD_MS_VSIM_VCX_LUN_1_OBJ_BLK_SIZE + \
                                                         DD_MS_VSIM_VCX_LUN_4_BLK_SIZE + DD_MS_VSIM_VCX_LUN_4_OBJ_BLK_SIZE + \
                                                         DD_MS_VSIM_CENTERA_LUN_BLK_SIZE + DD_MS_VSIM_CENTERA_LUN_OBJ_BLK_SIZE + \
                                                         DD_MS_VSIM_PSM_RESERVED_BLK_SIZE + DD_MS_VSIM_PSM_LUN_OBJ_BLK_SIZE + \
                                                         DD_MS_VSIM_DRAM_CACHE_LUN_BLK_SIZE + DD_MS_VSIM_DRAM_CACHE_LUN_OBJ_BLK_SIZE + \
                                                         DD_MS_VSIM_EFD_CACHE_LUN_BLK_SIZE + DD_MS_VSIM_EFD_CACHE_LUN_OBJ_BLK_SIZE + \
                                                         DD_MS_VSIM_FUTURE_EXP_BLK_SIZE + DD_MS_VSIM_PSM_RAID_GROUP_OBJ_BLK_SIZE + DD_MS_VSIM_ALIGN_BUFFER1_BLK_SIZE + \
                                                         DD_MS_VSIM_WIL_A_LUN_BLK_SIZE + DD_MS_VSIM_WIL_A_LUN_OBJ_BLK_SIZE + \
                                                         DD_MS_VSIM_WIL_B_LUN_BLK_SIZE + DD_MS_VSIM_WIL_B_LUN_OBJ_BLK_SIZE + \
                                                         DD_MS_VSIM_CPL_A_LUN_BLK_SIZE + DD_MS_VSIM_CPL_A_LUN_OBJ_BLK_SIZE + \
                                                         DD_MS_VSIM_CPL_B_LUN_BLK_SIZE + DD_MS_VSIM_CPL_B_LUN_OBJ_BLK_SIZE + \
                                                         DD_MS_VSIM_VCX_LUN_2_BLK_SIZE + DD_MS_VSIM_VCX_LUN_2_OBJ_BLK_SIZE + \
                                                         DD_MS_VSIM_VCX_LUN_3_BLK_SIZE + DD_MS_VSIM_VCX_LUN_3_OBJ_BLK_SIZE + \
                                                         DD_MS_VSIM_VCX_LUN_5_BLK_SIZE + DD_MS_VSIM_VCX_LUN_5_OBJ_BLK_SIZE + \
                                                         DD_MS_VSIM_RAID_METADATA_LUN_BLK_SIZE + DD_MS_VSIM_RAID_METADATA_LUN_OBJ_BLK_SIZE + \
                                                         DD_MS_VSIM_RAID_METASTATS_LUN_BLK_SIZE + DD_MS_VSIM_RAID_METASTATS_LUN_OBJ_BLK_SIZE + \
                                                         DD_MS_VSIM_VAULT_RESERVED_BLK_SIZE + DD_MS_VSIM_VAULT_LUN_OBJECT_BLK_SIZE + \
                                                         DD_MS_VSIM_VAULT_FUTURE_EXP_BLK_SIZE + DD_MS_VSIM_VAULT_RAID_GROUP_OBJ_BLK_SIZE + \
                                                         DD_MS_VSIM_NT_BOOT_SPA_PRIM_BLK_SIZE + DD_MS_VSIM_NT_BOOT_SPA_PRIM_EXP_BLK_SIZE + \
                                                         DD_MS_VSIM_UTIL_SPA_PRIM_BLK_SIZE + DD_MS_VSIM_UTIL_SPA_PRIM_EXP_BLK_SIZE + \
                                                         DD_MS_VSIM_ICA_IMAGE_REP_BLK_SIZE + DD_MS_VSIM_ICA_IMAGE_REP_EXP_BLK_SIZE + \
                                                         DD_MS_VSIM_FUTURE_GROWTH_BLK_SIZE + DD_MS_VSIM_NON_LUN_RAID_GROUP_OBJ_BLK_SIZE)

/*************************************************************
 * DISKs definitions for EXTERNAL USE sets
 *************************************************************/

#define DD_MS_DDBS_MIN_FRU                    0
#define DD_MS_DDBS_MAX_FRU                    2
#define DD_MS_DDBS_NUM_DISKS                  (DD_MS_DDBS_MAX_FRU - DD_MS_DDBS_MIN_FRU + 1)

#define DD_MS_BIOS_UCODE_MIN_FRU              DD_MS_THREE_DISK_MIRROR_ALG_MIN_FRU
#define DD_MS_BIOS_UCODE_MAX_FRU              DD_MS_THREE_DISK_MIRROR_ALG_MAX_FRU
#define DD_MS_BIOS_UCODE_NUM_DISKS            (DD_MS_BIOS_UCODE_MAX_FRU - DD_MS_BIOS_UCODE_MIN_FRU + 1)

#define DD_MS_PROM_UCODE_MIN_FRU              DD_MS_THREE_DISK_MIRROR_ALG_MIN_FRU
#define DD_MS_PROM_UCODE_MAX_FRU              DD_MS_THREE_DISK_MIRROR_ALG_MAX_FRU
#define DD_MS_PROM_UCODE_NUM_DISKS            (DD_MS_PROM_UCODE_MAX_FRU - DD_MS_PROM_UCODE_MIN_FRU + 1)

#define DD_MS_FUTURE_EXP_MIN_FRU              DD_MS_THREE_DISK_MIRROR_ALG_MIN_FRU
#define DD_MS_FUTURE_EXP_MAX_FRU              DD_MS_THREE_DISK_MIRROR_ALG_MAX_FRU
#define DD_MS_FUTURE_EXP_NUM_DISKS            (DD_MS_FUTURE_EXP_MAX_FRU - DD_MS_FUTURE_EXP_MIN_FRU + 1)

#define DD_MS_GPS_FW_MIN_FRU                  DD_MS_THREE_DISK_MIRROR_ALG_MIN_FRU
#define DD_MS_GPS_FW_MAX_FRU                  DD_MS_THREE_DISK_MIRROR_ALG_MAX_FRU
#define DD_MS_GPS_FW_NUM_DISKS                (DD_MS_GPS_FW_MAX_FRU - DD_MS_GPS_FW_MIN_FRU + 1)

#define DD_MS_BOOT_LOADER_MIN_FRU             DD_MS_THREE_DISK_MIRROR_ALG_MIN_FRU
#define DD_MS_BOOT_LOADER_MAX_FRU             DD_MS_THREE_DISK_MIRROR_ALG_MAX_FRU
#define DD_MS_BOOT_LOADER_NUM_DISKS           (DD_MS_BOOT_LOADER_MAX_FRU - DD_MS_BOOT_LOADER_MIN_FRU + 1)

#define DD_MS_EXP_USER_DUMMY_MIN_FRU          DD_MS_THREE_DISK_MIRROR_ALG_MIN_FRU
#define DD_MS_EXP_USER_DUMMY_MAX_FRU          DD_MS_THREE_DISK_MIRROR_ALG_MAX_FRU
#define DD_MS_EXP_USER_DUMMY_NUM_DISKS        (DD_MS_EXP_USER_DUMMY_MAX_FRU - DD_MS_EXP_USER_DUMMY_MIN_FRU + 1)

#define DD_MS_VAULT_RESERVED_MIN_FRU          0
#define DD_MS_VAULT_RESERVED_MAX_FRU          3
#define DD_MS_VAULT_RESERVED_NUM_DISKS        (DD_MS_VAULT_RESERVED_MAX_FRU - DD_MS_VAULT_RESERVED_MIN_FRU + 1)

#define DD_MS_VAULT_FUTURE_EXP_MIN_FRU        0
#define DD_MS_VAULT_FUTURE_EXP_MAX_FRU        3
#define DD_MS_VAULT_FUTURE_EXP_NUM_DISKS      (DD_MS_VAULT_FUTURE_EXP_MAX_FRU - DD_MS_VAULT_FUTURE_EXP_MIN_FRU + 1)

#define DD_MS_UTIL_SPA_PRIM_MIN_FRU           1
#define DD_MS_UTIL_SPA_PRIM_MAX_FRU           1
#define DD_MS_UTIL_SPA_PRIM_NUM_DISKS         (DD_MS_UTIL_SPA_PRIM_MAX_FRU - DD_MS_UTIL_SPA_PRIM_MIN_FRU + 1)

#define DD_MS_UTIL_SPA_SEC_MIN_FRU            3
#define DD_MS_UTIL_SPA_SEC_MAX_FRU            3
#define DD_MS_UTIL_SPA_SEC_NUM_DISKS          (DD_MS_UTIL_SPA_SEC_MAX_FRU - DD_MS_UTIL_SPA_SEC_MIN_FRU + 1)

#define DD_MS_UTIL_SPB_PRIM_MIN_FRU           0
#define DD_MS_UTIL_SPB_PRIM_MAX_FRU           0
#define DD_MS_UTIL_SPB_PRIM_NUM_DISKS         (DD_MS_UTIL_SPB_PRIM_MAX_FRU - DD_MS_UTIL_SPB_PRIM_MIN_FRU + 1)

#define DD_MS_UTIL_SPB_SEC_MIN_FRU            2
#define DD_MS_UTIL_SPB_SEC_MAX_FRU            2
#define DD_MS_UTIL_SPB_SEC_NUM_DISKS          (DD_MS_UTIL_SPB_SEC_MAX_FRU - DD_MS_UTIL_SPB_SEC_MIN_FRU + 1)

#define DD_MS_UTIL_SPA_PRIM_EXP_MIN_FRU       1
#define DD_MS_UTIL_SPA_PRIM_EXP_MAX_FRU       1
#define DD_MS_UTIL_SPA_PRIM_EXP_NUM_DISKS     (DD_MS_UTIL_SPA_PRIM_EXP_MAX_FRU - DD_MS_UTIL_SPA_PRIM_EXP_MIN_FRU + 1)

#define DD_MS_UTIL_SPA_SEC_EXP_MIN_FRU        3
#define DD_MS_UTIL_SPA_SEC_EXP_MAX_FRU        3
#define DD_MS_UTIL_SPA_SEC_EXP_NUM_DISKS      (DD_MS_UTIL_SPA_SEC_EXP_MAX_FRU - DD_MS_UTIL_SPA_SEC_EXP_MIN_FRU + 1)

#define DD_MS_UTIL_SPB_PRIM_EXP_MIN_FRU       0
#define DD_MS_UTIL_SPB_PRIM_EXP_MAX_FRU       0
#define DD_MS_UTIL_SPB_PRIM_EXP_NUM_DISKS     (DD_MS_UTIL_SPB_PRIM_EXP_MAX_FRU - DD_MS_UTIL_SPB_PRIM_EXP_MIN_FRU + 1)

#define DD_MS_UTIL_SPB_SEC_EXP_MIN_FRU        2
#define DD_MS_UTIL_SPB_SEC_EXP_MAX_FRU        2
#define DD_MS_UTIL_SPB_SEC_EXP_NUM_DISKS      (DD_MS_UTIL_SPB_SEC_EXP_MAX_FRU - DD_MS_UTIL_SPB_SEC_EXP_MIN_FRU + 1)

#define DD_MS_NT_BOOT_SPA_PRIM_MIN_FRU        0
#define DD_MS_NT_BOOT_SPA_PRIM_MAX_FRU        0
#define DD_MS_NT_BOOT_SPA_PRIM_NUM_DISKS      (DD_MS_NT_BOOT_SPA_PRIM_MAX_FRU - DD_MS_NT_BOOT_SPA_PRIM_MIN_FRU + 1)

#define DD_MS_NT_BOOT_SPA_SEC_MIN_FRU         2
#define DD_MS_NT_BOOT_SPA_SEC_MAX_FRU         2
#define DD_MS_NT_BOOT_SPA_SEC_NUM_DISKS       (DD_MS_NT_BOOT_SPA_SEC_MAX_FRU - DD_MS_NT_BOOT_SPA_SEC_MIN_FRU + 1)

#define DD_MS_NT_BOOT_SPB_PRIM_MIN_FRU        1
#define DD_MS_NT_BOOT_SPB_PRIM_MAX_FRU        1
#define DD_MS_NT_BOOT_SPB_PRIM_NUM_DISKS      (DD_MS_NT_BOOT_SPB_PRIM_MAX_FRU - DD_MS_NT_BOOT_SPB_PRIM_MIN_FRU + 1)

#define DD_MS_NT_BOOT_SPB_SEC_MIN_FRU         3
#define DD_MS_NT_BOOT_SPB_SEC_MAX_FRU         3
#define DD_MS_NT_BOOT_SPB_SEC_NUM_DISKS       (DD_MS_NT_BOOT_SPB_SEC_MAX_FRU - DD_MS_NT_BOOT_SPB_SEC_MIN_FRU + 1)

#define DD_MS_NT_BOOT_SPB_PRIM_EXP_MIN_FRU    1
#define DD_MS_NT_BOOT_SPB_PRIM_EXP_MAX_FRU    1
#define DD_MS_NT_BOOT_SPB_PRIM_EXP_NUM_DISKS  (DD_MS_NT_BOOT_SPB_PRIM_EXP_MAX_FRU - DD_MS_NT_BOOT_SPB_PRIM_EXP_MIN_FRU + 1)

#define DD_MS_NT_BOOT_SPA_PRIM_EXP_MIN_FRU    0
#define DD_MS_NT_BOOT_SPA_PRIM_EXP_MAX_FRU    0
#define DD_MS_NT_BOOT_SPA_PRIM_EXP_NUM_DISKS  (DD_MS_NT_BOOT_SPA_PRIM_EXP_MAX_FRU - DD_MS_NT_BOOT_SPA_PRIM_EXP_MIN_FRU + 1)

#define DD_MS_NT_BOOT_SPA_SEC_EXP_MIN_FRU     2
#define DD_MS_NT_BOOT_SPA_SEC_EXP_MAX_FRU     2
#define DD_MS_NT_BOOT_SPA_SEC_EXP_NUM_DISKS   (DD_MS_NT_BOOT_SPA_SEC_EXP_EXP_MAX_FRU - DD_MS_NT_BOOT_SPA_SEC_EXP_MIN_FRU + 1)

#define DD_MS_NT_BOOT_SPB_SEC_EXP_MIN_FRU     3
#define DD_MS_NT_BOOT_SPB_SEC_EXP_MAX_FRU     3
#define DD_MS_NT_BOOT_SPB_SEC_EXP_NUM_DISKS   (DD_MS_NT_BOOT_SPB_SEC_EXP_MAX_FRU - DD_MS_NT_BOOT_SPB_SEC_EXP_MIN_FRU + 1)

#define DD_MS_ICA_IMAGE_REP_MIN_FRU           0
#define DD_MS_ICA_IMAGE_REP_MAX_FRU           2
#define DD_MS_ICA_IMAGE_REP_NUM_DISKS         (DD_MS_ICA_IMAGE_REP_MAX_FRU - DD_MS_ICA_IMAGE_REP_MIN_FRU + 1)

#define DD_MS_ICA_IMAGE_REP_EXP_MIN_FRU       0
#define DD_MS_ICA_IMAGE_REP_EXP_MAX_FRU       2
#define DD_MS_ICA_IMAGE_REP_EXP_NUM_DISKS     (DD_MS_ICA_IMAGE_REP_EXP_MAX_FRU - DD_MS_ICA_IMAGE_REP_EXP_MIN_FRU + 1)

#define DD_MS_ICA_IMAGE_REP_PAD_MIN_FRU       3
#define DD_MS_ICA_IMAGE_REP_PAD_MAX_FRU       3
#define DD_MS_ICA_IMAGE_REP_PAD_NUM_DISKS     (DD_MS_ICA_IMAGE_REP_PAD_MAX_FRU - DD_MS_ICA_IMAGE_REP_PAD_MIN_FRU + 1)

#define DD_MS_PSM_RESERVED_MIN_FRU            0
#define DD_MS_PSM_RESERVED_MAX_FRU            2
#define DD_MS_PSM_RESERVED_NUM_DISKS          (DD_MS_PSM_RESERVED_MAX_FRU - DD_MS_PSM_RESERVED_MIN_FRU + 1)

#define DD_MS_BOOT_DIRECTIVES_MIN_FRU         0
#define DD_MS_BOOT_DIRECTIVES_MAX_FRU         3
#define DD_MS_BOOT_DIRECTIVES_NUM_DISKS       (DD_MS_BOOT_DIRECTIVES_MAX_FRU - DD_MS_BOOT_DIRECTIVES_MIN_FRU + 1)

#define DD_MS_FUTURE_GROWTH_MIN_FRU           0
#define DD_MS_FUTURE_GROWTH_MAX_FRU           3
#define DD_MS_FUTURE_GROWTH_NUM_DISKS         (DD_MS_FUTURE_GROWTH_MAX_FRU - DD_MS_FUTURE_GROWTH_MIN_FRU + 1)

/* dummy MIN/MAX defs */
#define DD_MS_EXT_NEW_DB_DUMMY_MIN_FRU        3
#define DD_MS_EXT_NEW_DB_DUMMY_MAX_FRU        3
#define DD_MS_EXT_NEW_DB_DUMMY_NUM_DISKS      (DD_MS_EXT_NEW_DB_DUMMY_MAX_FRU - DD_MS_EXT_NEW_DB_DUMMY_MIN_FRU + 1)

#define DD_MS_UTIL_FLARE_DUMMY_MIN_FRU        3
#define DD_MS_UTIL_FLARE_DUMMY_MAX_FRU        3
#define DD_MS_UTIL_FLARE_DUMMY_NUM_DISKS      (DD_MS_UTIL_FLARE_DUMMY_MAX_FRU - DD_MS_UTIL_FLARE_DUMMY_MIN_FRU + 1)

#define DD_MS_PSM_BOOT_DUMMY_MIN_FRU          3
#define DD_MS_PSM_BOOT_DUMMY_MAX_FRU          3
#define DD_MS_PSM_BOOT_DUMMY_NUM_DISKS        (DD_MS_PSM_BOOT_DUMMY_MAX_FRU - DD_MS_PSM_BOOT_DUMMY_MIN_FRU + 1)

#define DD_MS_DDBS_LUN_OBJ_MIN_FRU            0
#define DD_MS_DDBS_LUN_OBJ_MAX_FRU            2
#define DD_MS_DDBS_LUN_OBJ_NUM_DISKS          (DD_MS_DDBS_LUN_OBJ_MAX_FRU - DD_MS_DDBS_LUN_OBJ_MIN_FRU + 1)

#define DD_MS_BIOS_LUN_OBJ_MIN_FRU            0
#define DD_MS_BIOS_LUN_OBJ_MAX_FRU            2
#define DD_MS_BIOS_LUN_OBJ_NUM_DISKS          (DD_MS_BIOS_LUN_OBJ_MAX_FRU - DD_MS_BIOS_LUN_OBJ_MIN_FRU + 1)

#define DD_MS_PROM_LUN_OBJ_MIN_FRU            0
#define DD_MS_PROM_LUN_OBJ_MAX_FRU            2
#define DD_MS_PROM_LUN_OBJ_NUM_DISKS          (DD_MS_PROM_LUN_OBJ_MAX_FRU - DD_MS_PROM_LUN_OBJ_MIN_FRU + 1)

#define DD_MS_GPS_FW_LUN_OBJ_MIN_FRU          0
#define DD_MS_GPS_FW_LUN_OBJ_MAX_FRU          2
#define DD_MS_GPS_FW_LUN_OBJ_NUM_DISKS        (DD_MS_GPS_FW_LUN_OBJ_MAX_FRU - DD_MS_GPS_FW_LUN_OBJ_MIN_FRU + 1)

#define DD_MS_FLARE_DB_MIN_FRU                0
#define DD_MS_FLARE_DB_MAX_FRU                2
#define DD_MS_FLARE_DB_NUM_DISKS              (DD_MS_FLARE_DB_MAX_FRU - DD_MS_FLARE_DB_MIN_FRU + 1)

#define DD_MS_FLARE_DB_LUN_OBJ_MIN_FRU        0
#define DD_MS_FLARE_DB_LUN_OBJ_MAX_FRU        2
#define DD_MS_FLARE_DB_LUN_OBJ_NUM_DISKS      (DD_MS_FLARE_DB_LUN_OBJ_MAX_FRU - DD_MS_FLARE_DB_LUN_OBJ_MIN_FRU + 1)

#define DD_MS_ESP_LUN_MIN_FRU                 0
#define DD_MS_ESP_LUN_MAX_FRU                 2
#define DD_MS_ESP_LUN_NUM_DISKS               (DD_MS_ESP_LUN_MAX_FRU - DD_MS_ESP_LUN_MIN_FRU + 1)

#define DD_MS_ESP_LUN_OBJ_MIN_FRU             0
#define DD_MS_ESP_LUN_OBJ_MAX_FRU             2
#define DD_MS_ESP_LUN_OBJ_NUM_DISKS           (DD_MS_ESP_LUN_OBJ_MAX_FRU - DD_MS_ESP_LUN_OBJ_MIN_FRU + 1)

#define DD_MS_VCX_LUN_0_MIN_FRU               0
#define DD_MS_VCX_LUN_0_MAX_FRU               2
#define DD_MS_VCX_LUN_0_NUM_DISKS             (DD_MS_VCX_LUN_0_MAX_FRU - DD_MS_VCX_LUN_0_MIN_FRU + 1)

#define DD_MS_VCX_LUN_0_OBJ_MIN_FRU           0
#define DD_MS_VCX_LUN_0_OBJ_MAX_FRU           2
#define DD_MS_VCX_LUN_0_OBJ_NUM_DISKS         (DD_MS_VCX_LUN_0_OBJ_MAX_FRU - DD_MS_VCX_LUN_0_OBJ_MIN_FRU + 1)

#define DD_MS_VCX_LUN_1_MIN_FRU               0
#define DD_MS_VCX_LUN_1_MAX_FRU               2
#define DD_MS_VCX_LUN_1_NUM_DISKS             (DD_MS_VCX_LUN_1_MAX_FRU - DD_MS_VCX_LUN_1_MIN_FRU + 1)

#define DD_MS_VCX_LUN_1_OBJ_MIN_FRU           0
#define DD_MS_VCX_LUN_1_OBJ_MAX_FRU           2
#define DD_MS_VCX_LUN_1_OBJ_NUM_DISKS         (DD_MS_VCX_LUN_1_OBJ_MAX_FRU - DD_MS_VCX_LUN_1_OBJ_MIN_FRU + 1)

#define DD_MS_VCX_LUN_2_MIN_FRU               0
#define DD_MS_VCX_LUN_2_MAX_FRU               3
#define DD_MS_VCX_LUN_2_NUM_DISKS             (DD_MS_VCX_LUN_2_MAX_FRU - DD_MS_VCX_LUN_2_MIN_FRU + 1)

#define DD_MS_VCX_LUN_2_OBJ_MIN_FRU           0
#define DD_MS_VCX_LUN_2_OBJ_MAX_FRU           3
#define DD_MS_VCX_LUN_2_OBJ_NUM_DISKS         (DD_MS_VCX_LUN_2_OBJ_MAX_FRU - DD_MS_VCX_LUN_2_OBJ_MIN_FRU + 1)

#define DD_MS_VCX_LUN_3_MIN_FRU               0
#define DD_MS_VCX_LUN_3_MAX_FRU               3
#define DD_MS_VCX_LUN_3_NUM_DISKS             (DD_MS_VCX_LUN_3_MAX_FRU - DD_MS_VCX_LUN_3_MIN_FRU + 1)

#define DD_MS_VCX_LUN_3_OBJ_MIN_FRU           0
#define DD_MS_VCX_LUN_3_OBJ_MAX_FRU           3
#define DD_MS_VCX_LUN_3_OBJ_NUM_DISKS         (DD_MS_VCX_LUN_3_OBJ_MAX_FRU - DD_MS_VCX_LUN_3_OBJ_MIN_FRU + 1)

#define DD_MS_VCX_LUN_4_MIN_FRU               0
#define DD_MS_VCX_LUN_4_MAX_FRU               2
#define DD_MS_VCX_LUN_4_NUM_DISKS             (DD_MS_VCX_LUN_4_MAX_FRU - DD_MS_VCX_LUN_4_MIN_FRU + 1)

#define DD_MS_VCX_LUN_4_OBJ_MIN_FRU           0
#define DD_MS_VCX_LUN_4_OBJ_MAX_FRU           2
#define DD_MS_VCX_LUN_4_OBJ_NUM_DISKS         (DD_MS_VCX_LUN_4_OBJ_MAX_FRU - DD_MS_VCX_LUN_4_OBJ_MIN_FRU + 1)

#define DD_MS_VCX_LUN_5_MIN_FRU               0
#define DD_MS_VCX_LUN_5_MAX_FRU               3
#define DD_MS_VCX_LUN_5_NUM_DISKS             (DD_MS_VCX_LUN_5_MAX_FRU - DD_MS_VCX_LUN_5_MIN_FRU + 1)

#define DD_MS_VCX_LUN_5_OBJ_MIN_FRU           0
#define DD_MS_VCX_LUN_5_OBJ_MAX_FRU           3
#define DD_MS_VCX_LUN_5_OBJ_NUM_DISKS         (DD_MS_VCX_LUN_5_OBJ_MAX_FRU - DD_MS_VCX_LUN_5_OBJ_MIN_FRU + 1)

#define DD_MS_CENTERA_LUN_MIN_FRU             0
#define DD_MS_CENTERA_LUN_MAX_FRU             2
#define DD_MS_CENTERA_LUN_NUM_DISKS           (DD_MS_CENTERA_LUN_MAX_FRU - DD_MS_CENTERA_LUN_MIN_FRU + 1)

#define DD_MS_CENTERA_LUN_OBJ_MIN_FRU         0
#define DD_MS_CENTERA_LUN_OBJ_MAX_FRU         2
#define DD_MS_CENTERA_LUN_OBJ_NUM_DISKS       (DD_MS_CENTERA_LUN_OBJ_MAX_FRU - DD_MS_CENTERA_LUN_OBJ_MIN_FRU + 1)

#define DD_MS_PSM_LUN_OBJ_MIN_FRU             0
#define DD_MS_PSM_LUN_OBJ_MAX_FRU             2
#define DD_MS_PSM_LUN_OBJ_NUM_DISKS           (DD_MS_PSM_LUN_OBJ_MAX_FRU - DD_MS_PSM_LUN_OBJ_MIN_FRU + 1)

#define DD_MS_DRAM_CACHE_LUN_MIN_FRU          0
#define DD_MS_DRAM_CACHE_LUN_MAX_FRU          2
#define DD_MS_DRAM_CACHE_LUN_NUM_DISKS        (DD_MS_DRAM_CACHE_LUN_MAX_FRU - DD_MS_DRAM_CACHE_LUN_MIN_FRU + 1)

#define DD_MS_DRAM_CACHE_LUN_OBJ_MIN_FRU      0
#define DD_MS_DRAM_CACHE_LUN_OBJ_MAX_FRU      2
#define DD_MS_DRAM_CACHE_LUN_OBJ_NUM_DISKS    (DD_MS_DRAM_CACHE_LUN_OBJ_MAX_FRU - DD_MS_DRAM_CACHE_LUN_OBJ_MIN_FRU + 1)

#define DD_MS_EFD_CACHE_LUN_MIN_FRU           0
#define DD_MS_EFD_CACHE_LUN_MAX_FRU           2
#define DD_MS_EFD_CACHE_LUN_NUM_DISKS         (DD_MS_EFD_CACHE_LUN_MAX_FRU - DD_MS_EFD_CACHE_LUN_MIN_FRU + 1)

#define DD_MS_EFD_CACHE_LUN_OBJ_MIN_FRU       0
#define DD_MS_EFD_CACHE_LUN_OBJ_MAX_FRU       2
#define DD_MS_EFD_CACHE_LUN_OBJ_NUM_DISKS     (DD_MS_EFD_CACHE_LUN_OBJ_MAX_FRU - DD_MS_EFD_CACHE_LUN_OBJ_MIN_FRU + 1)

#define DD_MS_PSM_RAID_GROUP_OBJ_MIN_FRU      0
#define DD_MS_PSM_RAID_GROUP_OBJ_MAX_FRU      2
#define DD_MS_PSM_RAID_GROUP_OBJ_NUM_DISKS    (DD_MS_PSM_RAID_GROUP_OBJ_MAX_FRU - DD_MS_PSM_RAID_GROUP_OBJ_MIN_FRU + 1)

#define DD_MS_ALIGN_BUFFER1_MIN_FRU           0
#define DD_MS_ALIGN_BUFFER1_MAX_FRU           2
#define DD_MS_ALIGN_BUFFER1_NUM_DISKS         (DD_MS_ALIGN_BUFFER1_MAX_FRU - DD_MS_ALIGN_BUFFER1_MIN_FRU + 1)

#define DD_MS_WIL_A_LUN_MIN_FRU               0
#define DD_MS_WIL_A_LUN_MAX_FRU               3
#define DD_MS_WIL_A_LUN_NUM_DISKS             (DD_MS_WIL_A_LUN_MAX_FRU - DD_MS_WIL_A_LUN_MIN_FRU + 1)

#define DD_MS_WIL_A_LUN_OBJ_MIN_FRU           0
#define DD_MS_WIL_A_LUN_OBJ_MAX_FRU           3
#define DD_MS_WIL_A_LUN_OBJ_NUM_DISKS         (DD_MS_WIL_A_LUN_OBJ_MAX_FRU - DD_MS_WIL_A_LUN_OBJ_MIN_FRU + 1)

#define DD_MS_WIL_B_LUN_MIN_FRU               0
#define DD_MS_WIL_B_LUN_MAX_FRU               3
#define DD_MS_WIL_B_LUN_NUM_DISKS             (DD_MS_WIL_B_LUN_MAX_FRU - DD_MS_WIL_B_LUN_MIN_FRU + 1)

#define DD_MS_WIL_B_LUN_OBJ_MIN_FRU           0
#define DD_MS_WIL_B_LUN_OBJ_MAX_FRU           3
#define DD_MS_WIL_B_LUN_OBJ_NUM_DISKS         (DD_MS_WIL_B_LUN_OBJ_MAX_FRU - DD_MS_WIL_B_LUN_OBJ_MIN_FRU + 1)

#define DD_MS_CPL_A_LUN_MIN_FRU               0
#define DD_MS_CPL_A_LUN_MAX_FRU               3
#define DD_MS_CPL_A_LUN_NUM_DISKS             (DD_MS_CPL_A_LUN_MAX_FRU - DD_MS_CPL_A_LUN_MIN_FRU + 1)

#define DD_MS_CPL_A_LUN_OBJ_MIN_FRU           0
#define DD_MS_CPL_A_LUN_OBJ_MAX_FRU           3
#define DD_MS_CPL_A_LUN_OBJ_NUM_DISKS         (DD_MS_CPL_A_LUN_OBJ_MAX_FRU - DD_MS_CPL_A_LUN_OBJ_MIN_FRU + 1)

#define DD_MS_CPL_B_LUN_MIN_FRU               0
#define DD_MS_CPL_B_LUN_MAX_FRU               3
#define DD_MS_CPL_B_LUN_NUM_DISKS             (DD_MS_CPL_B_LUN_MAX_FRU - DD_MS_CPL_B_LUN_MIN_FRU + 1)

#define DD_MS_CPL_B_LUN_OBJ_MIN_FRU           0
#define DD_MS_CPL_B_LUN_OBJ_MAX_FRU           3
#define DD_MS_CPL_B_LUN_OBJ_NUM_DISKS         (DD_MS_CPL_B_LUN_OBJ_MAX_FRU - DD_MS_CPL_B_LUN_OBJ_MIN_FRU + 1)

#define DD_MS_RAID_METADATA_LUN_MIN_FRU       0
#define DD_MS_RAID_METADATA_LUN_MAX_FRU       3
#define DD_MS_RAID_METADATA_LUN_NUM_DISKS     (DD_MS_RAID_METADATA_LUN_MAX_FRU - DD_MS_RAID_METADATA_LUN_MIN_FRU + 1)

#define DD_MS_RAID_METADATA_LUN_OBJ_MIN_FRU   0
#define DD_MS_RAID_METADATA_LUN_OBJ_MAX_FRU   3
#define DD_MS_RAID_METADATA_LUN_OBJ_NUM_DISKS (DD_MS_RAID_METADATA_LUN_OBJ_MAX_FRU - DD_MS_RAID_METADATA_LUN_OBJ_MIN_FRU + 1)

#define DD_MS_RAID_METASTATS_LUN_MIN_FRU      0
#define DD_MS_RAID_METASTATS_LUN_MAX_FRU      3
#define DD_MS_RAID_METASTATS_LUN_NUM_DISKS    (DD_MS_RAID_METASTATS_LUN_MAX_FRU - DD_MS_RAID_METASTATS_LUN_MIN_FRU + 1)

#define DD_MS_RAID_METASTATS_LUN_OBJ_MIN_FRU  0
#define DD_MS_RAID_METASTATS_LUN_OBJ_MAX_FRU  3
#define DD_MS_RAID_METASTATS_LUN_OBJ_NUM_DISKS (DD_MS_RAID_METASTATS_LUN_OBJ_MAX_FRU - DD_MS_RAID_METASTATS_LUN_OBJ_MIN_FRU + 1)

#define DD_MS_VAULT_LUN_OBJECT_MIN_FRU        0
#define DD_MS_VAULT_LUN_OBJECT_MAX_FRU        3
#define DD_MS_VAULT_LUN_OBJECT_NUM_DISKS      (DD_MS_VAULT_LUN_OBJECT_MAX_FRU - DD_MS_VAULT_LUN_OBJECT_MIN_FRU + 1)

#define DD_MS_VAULT_RAID_GROUP_OBJ_MIN_FRU    0
#define DD_MS_VAULT_RAID_GROUP_OBJ_MAX_FRU    3
#define DD_MS_VAULT_RAID_GROUP_OBJ_NUM_DISKS  (DD_MS_VAULT_RAID_GROUP_OBJ_MAX_FRU - DD_MS_VAULT_RAID_GROUP_OBJ_MIN_FRU + 1)

#define DD_MS_NON_LUN_RAID_GROUP_OBJ_MIN_FRU  0
#define DD_MS_NON_LUN_RAID_GROUP_OBJ_MAX_FRU  3
#define DD_MS_NON_LUN_RAID_GROUP_OBJ_NUM_DISKS (DD_MS_NON_LUN_RAID_GROUP_OBJ_MAX_FRU - DD_MS_NON_LUN_RAID_GROUP_OBJ_MIN_FRU + 1)


/*************************************************************
 * DD_MS_AUX_SET_INFO *
 *   This structure is used to keep information about each valid
 *   Data Directory Main Service AUX type data.
 *
 *   dbo_type         - DB type;
 *   blk_size         - Specifies block size for allocated space;
 *************************************************************
 */
typedef struct DD_MS_AUX_SET_INFO
{
     DBO_TYPE     dbo_type;
     UINT_32E     blk_size;

} DD_MS_AUX_SET_INFO;

/* The following array is used to provide ordering of AUX SETS */
IMPORT  DD_MS_AUX_SET_INFO dd_ms_aux_set_info[DD_NUM_AUX_TYPES];


/*************************************************************
 * DD_MS_MANAGEMENT_SET_INFO *
 *   This structure is used to keep information about each valid
 *   Data Directory Main Service MANAGEMENT type data.
 *
 *   dbo_type         - DB type;
 *   is_dbo           - Specifies whether this type has DBO asssociated;
 *   is_set           - Specifies whether this type is a DB set entry point;
 *   rev              - Currently supported revision;
 *************************************************************
 */
typedef struct DD_MS_MANAGEMENT_SET_INFO
{
     DBO_TYPE     dbo_type;
     BOOL         is_dbo;
     BOOL         is_set;
     DBO_REVISION rev;
     UINT_32E     dbo_data_size;
     UINT_32E     num_entries;

} DD_MS_MANAGEMENT_SET_INFO;

/* The following array is used to provide ordering of MANAGEMENT SETS */
IMPORT  DD_MS_MANAGEMENT_SET_INFO dd_ms_man_set_info[DD_NUM_MANAGEMENT_TYPES];


/*************************************************************
 * DD_MS_USER_SET_INFO *
 *   This structure is used to keep information about each valid
 *   Data Directory Main Service USER DB type data.
 *
 *   dbo_type         - DB type;
 *   algorithm        - DB algortihm to be used with data;
 *   num_entries      - Number of entries of this type;
 *   info_bpe         - Blocks per entry;
 *   min_fru          - Minimum fru index in the set;
 *   max_fru          - Maximum fru index in the set;
 *************************************************************
 */
typedef struct DD_MS_USER_SET_INFO
{
     DBO_TYPE         dbo_type;
     DBO_ALGORITHM    algorithm;
     UINT_32E         num_entries;
     UINT_32E         info_bpe;
     UINT_32E         min_fru;
     UINT_32E         max_fru;
     DD_REGION_TYPE   region_type;

} DD_MS_USER_SET_INFO;

IMPORT DD_MS_USER_SET_INFO dd_ms_user_set_info[DD_MS_NUM_USER_TYPES];


/*************************************************************
 * DD_MS_EXTERNAL_USE_SET_INFO *
 *   This structure is used to keep information about each valid
 *   Data Directory Main Service EXTERNAL USE DB type data.
 *
 *   dbo_type         - DB type;
 *   blk_size         - allocated size of set of this type in blocks;
 *   min_fru          - Minimum fru index in the set;
 *   max_fru          - Maximum fru index in the set;
 *************************************************************
 */
typedef struct DD_MS_EXTERNAL_USE_SET_INFO
{
     DBO_TYPE   dbo_type;
     UINT_32E   blk_size;
     UINT_32E   min_fru;
     UINT_32E   max_fru;
     DD_REGION_TYPE   region_type;

} DD_MS_EXTERNAL_USE_SET_INFO;


/* Specifies which Ext Use DB set is valid 
*/
typedef enum _SELECTED_DD_MS_EXTERNAL_USE_SET 
{
    DEFAULT_EXT_USE_SET = 0, /* The default external use set covers Hardware and UMODE simulation. This decision as two which is made at compile time. */
    VMSIM_EXT_USE_SET   = 1  /* We may switch to the VM Simulation External use set, but this is a run time decision. */
} SELECTED_DD_MS_EXTERNAL_USE_SET;

/* A collection of external use DB sets for three different disk layouts: physical hardware, User Mode Simulation 
 * and VM Mode Simulation.
 */
typedef struct _DD_MS_EXTERNAL_USE_SET_INFO_STRUCT
{
    SELECTED_DD_MS_EXTERNAL_USE_SET Selected_Ext_Use_Set;
    DD_MS_EXTERNAL_USE_SET_INFO Default_Ext_Use_Set[DD_MS_NUM_EXTERNAL_USE_TYPES];
    DD_MS_EXTERNAL_USE_SET_INFO VmSim_Ext_Use_Set[DD_MS_NUM_EXTERNAL_USE_TYPES];
} DD_MS_EXTERNAL_USE_SET_INFO_STRUCT;

DD_MS_EXTERNAL_USE_SET_INFO* dd_GetExtUseSetInfoPtr(int i);

#endif /* DD_MAIN_SERVICE_H_INCLUDED */
