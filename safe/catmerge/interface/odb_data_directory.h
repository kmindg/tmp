/* odb_data_directory.h */

#ifndef ODB_DATA_DIRECTORY_H_INCLUDED
#define ODB_DATA_DIRECTORY_H_INCLUDED 0x00000001

/***************************************************************************
 * Copyright (C) EMC Corporation 2000-2009, 2012
 * All rights reserved
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *
 * DESCRIPTION:
 *   This header file contains DATA DIRECTORY related definitions.
 *
 * NOTES:
 *
 *
 * HISTORY:
 *   05-Apr-00: Created.                               Mike Zelikov (MZ)
 *
 ***************************************************************************/

/* INCLUDE FILES */
#include "drive_types.h"
#include "odb_dbo.h"


/* This defines Data Directory absolute starting point
 *  for all services.
 */
#define DD_START_ADDRESS                          ((DBOLB_ADDR)0)

/* The user space start address should start on a boundary 
 * that is a multiple of 0x40 (64) on a Klondike type disk 
 * (SATA disk)
 */
#define DD_KLONDIKE_USER_SPACE_BOUNDARY            0x40


/*************************************************************
 * ODB_SERVICE *
 *   This enum defines valid ODB services
 * Note: all enumerated types are treated as signed integers.
 *************************************************************
 */
typedef enum _DD_SERVICE
{
     DD_INVALID_SERVICE  = -1,
     DD_MAIN_SERVICE     = 0,

     /* Future possibilities:
        ODB_EXPORT_SERVICE = 1,
        ODB_IMPORT_SERVICE = 2,
     */

     DD_MAX_SERVICE
     
} DD_SERVICE;


typedef enum _DD_SPECIAL_ID_HIGH
{
     DD_MASTER_DDE_SPECIAL_ID_HIGH    = (UINT_32E)0,    /* Master Data Directory Entry -- Entry point to a Db */
     DD_MANUFACTURE_ZERO_MARK_ID_HIGH = (UINT_32E)1     /* Manufacture Zero Mark */

} DD_SPECIAL_ID_HIGH;

#define DD_ID_IS_MASTER_DDE(id_ptr) (DBO_ID_IS_SPECIAL((id_ptr)) && \
                                     (DD_MASTER_DDE_SPECIAL_ID_HIGH == (id_ptr)->high))
#define DD_ID_IS_MZM(id_ptr) (DBO_ID_IS_SPECIAL((id_ptr)) && \
                               (DD_MANUFACTURE_ZERO_MARK_ID_HIGH == (id_ptr)->high))


/*************************************************************
 * Data Directory extends DBO_TYPE by providing three classes:
 *
 *   MANAGEMENT    -- used to represent DATA DIRECTORY management
 *                    information;
 *   USER          -- used to represent information that is managed
 *                    by DATA DIRECTORY;
 *   EXTERNAL_USE  -- used to represent data that is managed outside
 *                    of DATA DIRECTORY;
 *   AUX           -- used to represent data that conatains service 
 *                    auxilurary information for managing Data Directory;
 * NOTE: up to 12 classes can be allocated.
 *************************************************************
 */
typedef enum _DD_TYPE_CLASS
{
     DBO_TYPE_MANAGEMENT_CLASS   =      DBO_TYPE_ALLOCATE_CLASS(0),
     DBO_TYPE_USER_CLASS         =      DBO_TYPE_ALLOCATE_CLASS(1),
     DBO_TYPE_EXTERNAL_USE_CLASS =      DBO_TYPE_ALLOCATE_CLASS(2),
     DBO_TYPE_AUX_CLASS          =      DBO_TYPE_ALLOCATE_CLASS(3)
} DD_TYPE_CLASS;

#define DBO_TYPE_MAKE_MANAGEMENT(index) ((DBO_TYPE)((index)|DBO_TYPE_MANAGEMENT_CLASS))
#define DBO_TYPE_MAKE_USER(index) ((DBO_TYPE)((index)|DBO_TYPE_USER_CLASS))
#define DBO_TYPE_MAKE_EXTERNAL_USE(index) ((DBO_TYPE)((index)|DBO_TYPE_EXTERNAL_USE_CLASS))
#define DBO_TYPE_MAKE_AUX(index) ((DBO_TYPE)((index)|DBO_TYPE_AUX_CLASS))

#define DBO_TYPE_IS_MANAGEMENT(dbo_type) (((dbo_type) & DBO_TYPE_MANAGEMENT_CLASS) != 0)
#define DBO_TYPE_IS_USER(dbo_type) (((dbo_type) & DBO_TYPE_USER_CLASS) != 0)
#define DBO_TYPE_IS_EXTERNAL_USE(dbo_type) (((dbo_type) & DBO_TYPE_EXTERNAL_USE_CLASS) != 0)
#define DBO_TYPE_IS_AUX(dbo_type) (((dbo_type) & DBO_TYPE_AUX_CLASS) != 0)


/*************************************************************
 * ENUM_DD_AUX_TYPE * 
 *   This enum defines valid DBO AUX types supported
 *   by this implementation of Data Directory.
 * NOTE: IT IS NOT REQUIRED FOR TYPES GO IN ORDER
 *       ONCE THE TYPE WAS USED WE SHOULD NEVER REUSE ITS NUMBER
 *       EVEN THAT IT IS NOT USED AGAIN.
 *************************************************************
 */
typedef enum _ENUM_DD_AUX_TYPE
{
     DD_BOOTSERVICE_TYPE         = DBO_TYPE_MAKE_AUX(0x0000)           /* Entry point to DD Boot Loader area */
     
} ENUM_DD_AUX_TYPE;

#define DD_NUM_AUX_TYPES                              1

/* SIZE DEFINITIONS FOR AUX DD SPACE */
#if defined(UMODE_ENV) || defined(SIMMODE_ENV) /* ====== SIM ====== */
#define DD_BOOTSERVICE_BLK_SIZE                       0x10        
#else                  /* ====== HW ====== */
#define DD_BOOTSERVICE_BLK_SIZE                       (DBO_MB_2_BLOCKS(2))
#endif /* #if defined(UMODE_ENV) */


/* Number of copies for MANAGEMENT space -- required for ALL services */
#define DD_MANAGEMENT_SPACE_REDUNDANCY            2

/*************************************************************
 * ENUM_DD_MANAGEMENT_TYPE * 
 *   This enum defines valid DBO management types supported
 *   by this implementation of Data Directory.
 * NOTE: IT IS NOT REQUIRED FOR TYPES GO IN ORDER
 *       ONCE THE TYPE WAS USED WE SHOULD NEVER REUSE ITS NUMBER
 *       EVEN THAT IT IS NOT USED AGAIN.
 *************************************************************
 */
typedef enum _ENUM_DD_MANAGEMENT_TYPE
{
     DD_MASTER_DDE_TYPE          = DBO_TYPE_MAKE_MANAGEMENT(0x0000),   /* Entry point to DD Management Area */
     DD_INFO_TYPE                = DBO_TYPE_MAKE_MANAGEMENT(0x0001),   /* DD information */
     DD_PREV_REV_MASTER_DDE_TYPE = DBO_TYPE_MAKE_MANAGEMENT(0x0002),   /* Entry point to previous revision of the DD */
     DD_USER_DDE_TYPE            = DBO_TYPE_MAKE_MANAGEMENT(0x0003),   /* Entry point to USER DB  */
     DD_EXTERNAL_USE_DDE_TYPE    = DBO_TYPE_MAKE_MANAGEMENT(0x0004),   /* Entry point to EXTERNAL USE DB */
     DD_PUBLIC_BIND_TYPE         = DBO_TYPE_MAKE_MANAGEMENT(0x0005)    /* Entry point to PUBLIC BIND space */
     
} ENUM_DD_MANAGEMENT_TYPE;

#define DD_NUM_MANAGEMENT_TYPES              6                         /* Update this with the number of types changed */

/* Master DDE should always be present and MUST be the start type */
#define DD_FIRST_MANAGEMENT_TYPE             DD_MASTER_DDE_TYPE

/* Addition for R22 */
typedef enum _DD_REGION_TYPE
{
    DD_MS_MGMT_REGION = 0,
    DD_MS_INFO_REGION,
    DD_MS_PREV_REV_MGMT_REGION,
    DD_MS_USER_REGION_1,
    DD_MS_EXTERNAL_USE_REGION_1,
    DD_MS_USER_REGION_2,
    DD_MS_EXTERNAL_USE_REGION_2,
    DD_MS_PUBLIC_BIND_REGION
} DD_REGION_TYPE;

#define DD_INVALID_REGION       -1


/* ----- MASTER DATA DIRECTORY ENTRY RELATED DEFINITIONS ----- */
#define DD_MAN_DATA_CRC_OFFSET               (2*sizeof(UINT_32E))

/* MASTER DATA DIRECTORY entry is the starting point for
 * Data Directory database. We maintain two copies of MDDE on a
 * fru.
 */
/*************************************************************
 * DD Management Data Table Entry : 28 bytes | 40 bytes (64 bit LBA)
 *   This entry specifies information about the entry point
 *   to a single Management Set.
 * type             - DBO (management) Type
 * dde_bpe          - Blocks per single entry
 * num_entries      - Total number of Entries
 * prim_copy        - Block address of Primary copy
 * sec_copy         - Block address of Secondary copy
 * region_quota     - Space size reserved for the Region
 *                    identified by Management Dbo
 * region_addr      - Space address reserved for the Region
 *                    identified by Management Dbo
 *
 * Note: The order of elements in this structure is important.
 *       4 byte elements might get expanded to a 8 byte element 
 *       where the other 4 bytes are called 'holes'. 
 * 
 *       AV: I referred to the book "The C Programming Language" 
 *       and it says the following: "Don't assume that the size 
 *       of a structure is the sum of the sizes of its members. 
 *       Because of alignment requirements for different objects, 
 *       there may be unnamed holes in a structure. Thus, for 
 *       instance, if a char is one byte and an int four bytes, 
 *       the structure
 * 
 *           struct {
 *              char c;
 *              int i;
 *           };
 * 
 *       might well require eight bytes, not five. The sizeof 
 *       operator returns the proper value."
 *       Added: AV & HR - 05/24/05
 * 
 * Note: Instead of using the above mentioned approach, which
 *       could have unknown effects, the following method can
 *       be used to get the same effect (suggested by Mike Hamel)
 *
 *       #pragma pack(1)
 *       ...
 *       #pragma pack() <-- dont forget to end the directive
 *       The directive has been added to other structures as well.
 *
 *       Added: HR - 06/01/05
 *************************************************************
 */

#pragma pack(1)
typedef struct DataDirectoryManagementDataTableEntry
{
     DBO_TYPE     type;                           /* 4 bytes */

     UINT_32E     bpe;                            /* 4 bytes */
     UINT_32E     num_entries;                    /* 4 bytes */
     
     DBOLB_ADDR   prim_copy;                      /* 4 | 8 bytes */
     DBOLB_ADDR   sec_copy;                       /* 4 | 8 bytes */

	 UINT_32E     region_quota;                   /* 4 bytes */
     DBOLB_ADDR   region_addr;                    /* 4 | 8 bytes */

} DD_MAN_DATA_TBL_ENTRY;
#pragma pack()

/* The pre-Hammer Management data table structure was different. 
 * (it used 32-bit addresses). We'll define
 *  it here, so we will have something to compare against. 
 */
typedef struct DDOldManagementDataTableEntry
{
     DBO_TYPE     type;                           /* 4 bytes */

     UINT_32E     bpe;                            /* 4 bytes */
     UINT_32E     num_entries;                    /* 4 bytes */
     
     UINT_32E   prim_copy;                      /* 4 | 8 bytes */
     UINT_32E   sec_copy;                       /* 4 | 8 bytes */

	 UINT_32E     region_quota;                   /* 4 bytes */
     UINT_32E   region_addr;                    /* 4 | 8 bytes */

} DD_NT_FISH_MAN_DATA_TBL_ENTRY;

#define DD_MAN_DATA_TBL_QUOTA                10   /* Maximum Management sets allowed : Should be chosen so we have MDDE 1 sector */


/*************************************************************
 * Master Data Directory Entry : 320 | 440 bytes *
 * rev_num            - MDDE Revision number
 * crc                - Computed CRC;
 * dd_service         - DD Service that created DD;
 * dd_rev             - DD Service revision;
 * update_count       - Shows current update count of MASTER DD
 * logical_id         - Logical disk id. Used for HS
 * mtbl_quota         - Maximum Number of entries in MAN DATA table
 * mtbl_num_entries   - Number of entries in MAN DATA table
 * mtbl_tbl           - Management Data Table
 * layout_rev         - Revision number for disk layout.
 *                      (Added: 05/05 - HR)
 *************************************************************
 */
/* #define MASTER_DDE_BLOCKS_PER_ENTRY         -- Dynamically computed */
#define MASTER_DDE_NUM_ENTRIES               1

/* Disk layout revision number */
#define DD_MS_DEFAULT_LAYOUT_REV             1

#pragma pack(1)
typedef struct MasterDataDirectoryEntryData
{
     DBO_REVISION           rev_num;                        /* 4 bytes */
     UINT_32E               crc;                            /* 4 bytes */

     /* Data protected by CRC */
     UINT_32E               dd_service;                     /* 4 bytes */
     UINT_32E               dd_rev;                         /* 4 bytes */
     DBO_UPDATE_COUNT       update_count;                   /* 4 bytes */
     UINT_32E               flags;                          /* 4 bytes */
     UINT_32E               logical_id;                     /* 4 bytes */
     UINT_32E               mtbl_quota;                     /* 4 bytes */ 
     UINT_32E               mtbl_num_entries;               /* 4 bytes */ 
	 UINT_32E               layout_rev;                     /* 4 bytes */
     DD_MAN_DATA_TBL_ENTRY  mtbl[DD_MAN_DATA_TBL_QUOTA];    /* 280 | 400 bytes */

} MASTER_DDE_DATA;
#pragma pack()

/* The pre-Hammer MDDE structure was different. We'll define
 *  it here, so we will have something to compare against. 
 */
typedef struct MDDEOldStructure
{
     DBO_REVISION           rev_num;                        /* 4 bytes */
     UINT_32E               crc;                            /* 4 bytes */

     /* Data protected by CRC */
     UINT_32E               dd_service;                     /* 4 bytes */
     UINT_32E               dd_rev;                         /* 4 bytes */
     DBO_UPDATE_COUNT       update_count;                   /* 4 bytes */
     UINT_32E               flags;                          /* 4 bytes */
     UINT_32E               logical_id;                     /* 4 bytes */
     UINT_32E               mtbl_quota;                     /* 4 bytes */ 
     UINT_32E               mtbl_num_entries;               /* 4 bytes */ 
     DD_NT_FISH_MAN_DATA_TBL_ENTRY  mtbl[DD_MAN_DATA_TBL_QUOTA];    /* 280 bytes */
} NT_FISH_MASTER_DDE_DATA;

#define MASTER_DDE_DATA_SIZE                 sizeof(MASTER_DDE_DATA)
#define NT_FISH_MASTER_DDE_DATA_SIZE		 sizeof(NT_FISH_MASTER_DDE_DATA)
#define MASTER_DDE_DATA_CRC_SIZE             (MASTER_DDE_DATA_SIZE - DD_MAN_DATA_CRC_OFFSET)

#define MASTER_DDE_REVISION_ONE              (DBO_REVISION)1
#define MASTER_DDE_REVISION_TWO              (DBO_REVISION)2
#define MASTER_DDE_REVISION_ONE_HUNDRED	     (DBO_REVISION)100
#define MASTER_DDE_REVISION_TWO_HUNDRED	     (DBO_REVISION)200
#define MASTER_DDE_REVISION_THREE_HUNDRED    (DBO_REVISION)300 // Tackhammer
#define MASTER_DDE_FLEET_REVISION			 (DBO_REVISION)400 // Fleet
#define MASTER_DDE_MAMBA_REVISION            (DBO_REVISION)500 // Mamba-Test.
#define MASTER_DDE_CX5_REVISION              (DBO_REVISION)600

#define MASTER_DDE_CURRENT_REVISION          MASTER_DDE_CX5_REVISION

#define DD_NUM_MASTER_DDE_COPIES             DD_MANAGEMENT_SPACE_REDUNDANCY

#define dd_WhatIsCurrentMasterDdeRev()       MASTER_DDE_CURRENT_REVISION


/* We use lower 12 bits for gerneric/common flags.
 * Upper 20 bits are used by each defined service.
 */
typedef enum DD_MASTER_DDE_FLAGS
{
     DD_INVALID_FLAGS               = (UINT_32E)0x00000000,  /* Initialization */
     DD_ZERO_MARK_FLAG              = (UINT_32E)0x00000001,  /* A FRU was zeroed */
     DD_NEW_LAYOUT_COMMITTED_FLAG   = (UINT_32E)0x00000002   /* Should be set when new Private Space Layout is commited */
}
DD_MASTER_DDE_FLAGS;

/* ----- DATA DIRECTORY INFORMATION RELATED DEFINITIONS ----- */
/* DD Info contains information about DD, its creator and
 * USER/EXTERNAL USE databases.
 */
/*************************************************************
 * DBO DD Info : 32 bytes *
 *
 *  rev_num              - Revision number
 *  update_count         - Shows current update count of DDE
 *
 *  dd_service           - Data Directory Service identifier
 *  dd_service_rev_num   - Data Directory Service revision number
 *  db_rev_num           - Data Base revision Number (used for USER and EXTERNAL types)
 *
 *  num_disks            - Total number of frus involved
 *  disk_set             - Frus that are involved in this data
 *************************************************************
 */
/* #define DD_INFO_BLOCKS_PER_ENTRY           -- Dynamically defined  */
#define DD_INFO_NUM_ENTRIES                  1

#define DBO_DD_INFO_MIN_FRU                  0
#define DBO_DD_INFO_MAX_FRU                  2
#define DBO_DD_INFO_NUM_DISKS                (DBO_DD_INFO_MAX_FRU - DBO_DD_INFO_MIN_FRU + 1)

#pragma pack(1)
typedef struct DDInfoData
{
     DBO_REVISION      rev_num;                         /* 4 bytes */
     UINT_32E          crc;                             /* 4 bytes */

     /* Data protected by CRC */
     DBO_UPDATE_COUNT  update_count;                    /* 4 bytes */
     UINT_32E          dd_service;                      /* 4 bytes */
     DBO_REVISION      dd_service_rev_num;              /* 4 bytes */
     DBO_REVISION      db_rev_num;                      /* 4 bytes */

     UINT_32E          num_disks;                       /* 4 bytes */
     UINT_32E          disk_set[DBO_DD_INFO_NUM_DISKS]; /* 3*4=12 bytes */

} DD_INFO_DATA;
#pragma pack()

#define DD_INFO_DATA_SIZE                    sizeof(DD_INFO_DATA)
#define DD_INFO_DATA_CRC_SIZE                (DD_INFO_DATA_SIZE - DD_MAN_DATA_CRC_OFFSET)

#define DD_INFO_REVISION_ONE                 ((DBO_REVISION) 1)
#define DD_INFO_CURRENT_REVISION             DD_INFO_REVISION_ONE
#define DD_INFO_NUM_COPIES                   DD_MANAGEMENT_SPACE_REDUNDANCY


/* ----- DATA DIRECTORY ENTRY RELATED DEFINITIONS ----- */
/* DDE is the starting point for any USER/EXTERNAL USE sets. 
 * They are organized as an array of DDEs. MDDE contains 
 * pointer to DDE ARRAY.
 */

/*************************************************************
 * Data Directory Entry : 104 | 108 bytes *
 *  DDE identifies the entry point to a DB set: USER or EXTERNAL USE.
 *
 *  rev_num              - Revision number
 *  crc                  - Computed CRC;
 *  update_count         - Shows current update count of DDE
 *  type                 - DBO (USER or EXTERNAL USE) Type
 *  algorithm_type       - Algorithm used to store data
 *  num_entries          - Total number of entries in the set
 *  info_bpe             - Blocks per set info entry
 *  start_addr           - Block address of first entry in the set
 *
 *  num_disks            - Total number of frus involved
 *  disk_set             - Frus that are involved in this data
 *************************************************************
 */
/* #define DDE_BLOCKS_PER_ENTRY            -- Dynamically defined  */
/* #define DDE_NUM_ENTRIES                 -- Dynamically defined  */

#pragma pack(1)
typedef struct DataDirectoryEntryData
{
     DBO_REVISION       rev_num;                         /* 4 bytes */
     UINT_32E           crc;                             /* 4 bytes */

     /* Data protected by CRC */
     DBO_UPDATE_COUNT   update_count;                    /* 4 bytes */
     DBO_TYPE           type;                            /* 4 bytes */
     DBO_ALGORITHM      algorithm;                       /* 4 bytes */
     UINT_32E           num_entries;                     /* 4 bytes */
     DBO_REVISION       info_rev;                        /* 4 bytes */
     UINT_32E           info_bpe;                        /* 4 bytes */
     DBOLB_ADDR         start_addr;                      /* 8 bytes */
                        
     UINT_32E           num_disks;                       /* 4 bytes */
     UINT_32E           disk_set[MAX_DISK_ARRAY_WIDTH];  /* 16*4=64 bytes */

} DDE_DATA;
#pragma pack()

typedef struct DDEOldStructure
{
     DBO_REVISION       rev_num;                         /* 4 bytes */
     UINT_32E           crc;                             /* 4 bytes */

     /* Data protected by CRC */
     DBO_UPDATE_COUNT   update_count;                    /* 4 bytes */
     DBO_TYPE           type;                            /* 4 bytes */
     DBO_ALGORITHM      algorithm;                       /* 4 bytes */
     UINT_32E           num_entries;                     /* 4 bytes */
     DBO_REVISION       info_rev;                        /* 4 bytes */
     UINT_32E           info_bpe;                        /* 4 bytes */
     UINT_32E           start_addr;                      /* 8 bytes */
                        
     UINT_32E           num_disks;                       /* 4 bytes */
     UINT_32E           disk_set[MAX_DISK_ARRAY_WIDTH];  /* 16*4=64 bytes */

} NT_FISH_DDE_DATA;

#define DDE_DATA_SIZE                        sizeof(DDE_DATA)
#define NT_FISH_DDE_DATA_SIZE                sizeof(NT_FISH_DDE_DATA)
#define DDE_DATA_CRC_SIZE                    (DDE_DATA_SIZE - DD_MAN_DATA_CRC_OFFSET)

#define DDE_REVISION_ONE                     ((DBO_REVISION) 1)
#define DDE_CURRENT_REVISION                 DDE_REVISION_ONE
#define DD_NUM_DDE_COPIES                    DD_MANAGEMENT_SPACE_REDUNDANCY


/* ----- DATA DIRECTORY MANIFACTURE ZEROING SUPPORT ----- */
#define DD_MANUFACTURE_ZERO_MARK_DATA        "MANUFACTURE*ZERO*MARK*08202001*2f94f9622cd506564c5c5e7417e48724"
#define DD_MANUFACTURE_ZERO_MARK_DATA_SIZE   64 /* w '\0' */

/*************************************************************
 * DataDirectoryManufactureZeroMark *
 *   This defines data directory manufacture zero mark.
 * 
 *    rev_num              - Revision number
 *    crc                  - Computed CRC;
 *    raw_data             - Raw data that represents zero mark;
 *
 * Note: We never modify the mark therefore there is no need 
 * for the update_count to be present in this struccture.
 *************************************************************
 */
typedef struct DataDirectoryManufactureZeroMark
{
     DBO_REVISION  rev_num;
     UINT_32       crc;

     UINT_8E       raw_data[DD_MANUFACTURE_ZERO_MARK_DATA_SIZE];

} DD_MAN_ZERO_MARK;

#define DD_MAN_ZERO_MARK_SIZE                sizeof(DD_MAN_ZERO_MARK)

/* CRC size is always based on MASTER_DDE_DATA_SIZE */

#define DD_MAN_ZERO_MARK_REVISION_ONE        ((DBO_REVISION) 1)
#define DD_MAN_ZERO_MARK_CURRENT_REVISION    DD_MAN_ZERO_MARK_REVISION_ONE


/* ----- DATA DIRECTORY RESERVED SPACE RELATED DEFINITIONS ----- */
/*
 * Values are defined by each service.
 */
#define DD_RESERVED_SPACE_NUM_COPIES         DD_MANAGEMENT_SPACE_REDUNDANCY


/*************************************************************
 * DD_STATUS *
 *   This defines data directory status.
 * 
 *   code_1           - Main return status (VD status);
 *   code_2           - Additional status code;
 *
 *************************************************************
 */
typedef struct DD_STATUS
{
     INT_32  code_1;
     INT_32  code_2;
} DD_STATUS;




/*************************************************************
 * DD_INFO_API *
 *   This defines data directory information API..
 * 
 *   dbo_type         - Dbo Type for which to return the information;
 *   word2.WWN_seed   - The WWN seed read from the mid-plane resume
 *                      PROM is received from POST.
 *   word2.start_addr - The start address of DBO Type Set is returned
 *                      to POST.
 *   word3.print_ptr  - A pointer to the POST print function is received
 *                      from POST.
 *   word3.size       - Size is returned to POST.
 *   num_disks        - Number of disks in the set;
 *   disk_set         - Disks forming the set;
 *
 *************************************************************

*/

#define MAX_DD_API_DISKSET_WIDTH   8
#define DD_API_SPARES              7

enum DDBS_BOOT_DIRECTIVE
  {
    PXE_BOOT=0xFC,
    STOP_BOOT=0xFD,
    OS_BOOT=0xFE,
    DBO_BOOT=0xFF
  };

typedef struct _DDBS_SERVICE
{
  UINT_32E data         : 24;
  UINT_32E command      :  7;
  UINT_32E service_flag :  1;
} DDBS_SERVICE;

#ifndef UEFI_ENV
typedef struct _DD_INFO_API
{
  union
  {
    DDBS_SERVICE ddbs_service;
    DBO_TYPE    dbo_type;
  };

  union
  {
    struct
    {
      UINT_32 WWN_Seed;
      UINT_32 printf_ptr;

      UINT_32E nvram_base;  // 16-bit I/O bus port address (high 16 bits must be zero) 
                            // OR assumed to be base of mapped memory address (if high 16 bits not zero)
      UINT_32E ip_spare1[MAX_DD_API_DISKSET_WIDTH];
      UINT_32E ip_spare2[DD_API_SPARES];
      UINT_32E ip_spare3;
    } input;

    struct
    {
      UINT_32E start_addr; //was DBOLB_ADDR
      UINT_32 size;

      UINT_32E num_disks;
      UINT_32E disk_set[MAX_DD_API_DISKSET_WIDTH];
      UINT_32E op_spare1[DD_API_SPARES];

      UINT_32E directive: 8;
      UINT_32E          : 24;
    } output;
  } u_data;

}
DD_INFO_API;
#endif // #ifndef UEFI_ENV

typedef enum _DD_ERROR_CODE
{
    DD_OK =  0x0000,
    DD_E_INVALID_REVISION = 0x0002,
    DD_E_INVALID_SERVICE = 0x0003,
    DD_E_INVALID_REGION_TYPE = 0x0004,
    DD_E_INVALID_DBO_CLASS = 0x0005,
    DD_E_INVALID_DBO_TYPE = 0x0006,
    DD_E_INVALID_DDE_TYPE = 0x0007,
    DD_E_INVALID_MDDE = 0x0008,
    DD_E_INVALID_EXT_USE_DDE = 0x0009,
    DD_E_INVALID_MAN_SET_INFO = 0x000A,
    DD_E_SET_ABSENT_ON_FRU = 0x000B,
    DD_E_INVALID_FRU = 0x000C,
    DD_E_FAILED_MZM_CREATION = 0x000D,
    DD_E_INVALID_EXT_DBO_TYPE = 0x000E,
    DD_E_INVALID_USER_DBO_TYPE = 0x000F,
    DD_E_INVALID_MDDE_REVISION = 0x0010,
    DD_E_INVALID_MAN_TYPE = 0x0011,
    DD_E_INVALID_ARG = 0x0012,
    DD_E_INVALID_MEMSET = 0x0013,
    DD_E_GENERIC_FAILURE = 0x0014

} DD_ERROR_CODE;

/*************************************************************
 * DISK_LAYOUT_TYPE
 *  An argument to certian functions indicating whether the 
 *  default or VM Simulation layout should be considered.
 *  We override the default only for the ddbs_info utility
 *  when the user wants to know the VM Simulation layout.
 *
 *************************************************************/
typedef enum _DISK_LAYOUT_TYPE
{
    DefaultLayout=0,
    VmSimLayout=1
} DISK_LAYOUT_TYPE;

/************************
 * PROTOTYPE DEFINITIONS
 ************************/
BOOL dd_String2DboType(const DD_SERVICE service, 
                       const DBO_REVISION rev,
                       const char *str,
                       DBO_TYPE *dbo_type);
BOOL dd_DboType2String(const DD_SERVICE service, 
                       const DBO_REVISION rev,
                       const DBO_TYPE dbo_type, 
                       char str[30]);

DD_ERROR_CODE dd_DboType2Index(const DD_SERVICE service,
                      const DBO_REVISION rev,
                      const DBO_TYPE dbo_type,
                      UINT_32 *index_ptr);
DD_ERROR_CODE dd_Index2DboType(const DD_SERVICE service,
                      const DBO_REVISION rev,
                      const DD_TYPE_CLASS dd_class,
                      const UINT_32 index,
                      DBO_TYPE *dbo_type_ptr);
DD_ERROR_CODE dd_GetNextDboType(const DD_SERVICE service,
                       const DBO_REVISION rev,
                       DBO_TYPE *dbo_type_ptr,
                       MASTER_DDE_DATA *mdde_ptr,
                       const DBO_REVISION curr_mdde_rev);

BOOL dd_OdbRegion2String(const DD_SERVICE service, 
                         const DBO_REVISION rev,
                         const DBO_TYPE dbo_type, 
                         char str[15]);

DD_ERROR_CODE dd_CrcManagementData(UINT_8E *data, UINT_32 man_size, UINT_32 *crc_ptr);

DD_ERROR_CODE dd_WhatIsDdAuxStartAddr(const DD_SERVICE service,
                             const DBO_REVISION rev,
                             DBOLB_ADDR *addr);
DD_ERROR_CODE dd_WhatIsDdAuxSpace(const DD_SERVICE service,
                         const DBO_REVISION rev,
                         UINT_32 *blk_size);
DD_ERROR_CODE dd_WhatIsDdAuxSetSpace(const DD_SERVICE service,
                            const DBO_REVISION rev,
                            const DBO_TYPE aux_type,                            
                            UINT_32 *blk_size);
DD_ERROR_CODE dd_WhatIsDdAuxSetStartAddr(const DD_SERVICE service,
                                const DBO_REVISION rev,
                                const DBO_TYPE aux_type,
                                DBOLB_ADDR *addr);

DD_ERROR_CODE dd_WhatIsDataDirectoryStartAddr(const DD_SERVICE service,
                                     const DBO_REVISION rev,
                                     DBOLB_ADDR *addr);

DD_ERROR_CODE dd_WhatIsDataDirectorySpace(const DD_SERVICE service,
                                          const DBO_REVISION rev,
                                          const UINT_32 fru,
                                          UINT_32 *blk_size,
                                          FLARE_DRIVE_TYPE drive_type);

DD_ERROR_CODE dd_WhatIsDbSpace(const DD_SERVICE service,
                      const DBO_REVISION rev,
                      const UINT_32 fru,
                      const DBO_TYPE man_type,
                      const DISK_LAYOUT_TYPE DiskLayout,
                      UINT_32 *blk_size);

DD_ERROR_CODE dd_WhatIsDbStartAddr(const DD_SERVICE service,
                                   const DBO_REVISION rev,
                                   const UINT_32 fru,
                                   const DBO_TYPE man_type,
                                   DBOLB_ADDR *addr,
                                   FLARE_DRIVE_TYPE drive_type,
                                   const DISK_LAYOUT_TYPE DiskLayout);

DD_ERROR_CODE dd_WhatIsManagementDboCurrentRev(const DD_SERVICE service, 
                                      const DBO_REVISION rev,
                                      const DBO_TYPE man_dbo_type, 
                                      DBO_REVISION *man_dbo_rev);
DD_ERROR_CODE dd_WhatIsManagementDboBpe(const DD_SERVICE service, 
                               const DBO_REVISION rev,
                               const DBO_TYPE man_dbo_type, 
                               const DBO_REVISION man_dbo_rev,
                               UINT_32 *man_dbo_bpe);
DD_ERROR_CODE dd_WhatIsManagementDboSpace(const DD_SERVICE service, 
                                 const DBO_REVISION rev,
                                 const DBO_TYPE man_dbo_type, 
                                 const DBO_REVISION man_dbo_rev,
                                 UINT_32 *man_dbo_space);
DD_ERROR_CODE dd_WhatIsManagementDboAddr(const DD_SERVICE service, 
                                const DBO_REVISION rev,
                                const DBO_TYPE dbo_type,
                                const DBO_INDEX flag,
                                DBOLB_ADDR *addr);

DD_ERROR_CODE dd_Is_User_ExtUse_SetOnFru(const DD_SERVICE service, 
                                const DBO_REVISION rev,
                                const UINT_32 fru,
                                const DBO_TYPE dbo_type,
                                BOOL *set_present);
DD_ERROR_CODE dd_Is_User_ExtUse_SetOnEachFru(const DD_SERVICE service, 
                                    const DBO_REVISION rev,
                                    const DBO_TYPE dbo_type,
                                    BOOL *set_present);

DD_ERROR_CODE dd_WhatIs_User_ExtUse_DiskSet(const DD_SERVICE service, 
                                   const DBO_REVISION rev,
                                   const DBO_TYPE dbo_type,
                                   UINT_32 *min_fru,
                                   UINT_32 *max_fru);

DD_ERROR_CODE dd_GetDefaultMasterDde(const DD_SERVICE service,
                                     const DBO_REVISION rev,
                                     const DBO_REVISION mdde_rev,
                                     UINT_32 fru,
                                     MASTER_DDE_DATA *mdde,
                                     FLARE_DRIVE_TYPE drive_type);

DD_ERROR_CODE dd_GetDefault_User_ExtUse_SetInfo(const DD_SERVICE service,
                                                const DBO_REVISION rev,
                                                const DBO_REVISION dde_rev, 
                                                const UINT_32 fru,
                                                const DBO_TYPE dbo_type,
                                                DDE_DATA *dde,
                                                FLARE_DRIVE_TYPE drive_type,
                                                const DBO_REVISION current_dde_rev,
                                                const DISK_LAYOUT_TYPE DiskLayout);

DD_ERROR_CODE dd_WhatIsDefault_User_ExtUse_SetAddr(const DD_SERVICE service,
                                                   const DBO_REVISION rev,
                                                   const UINT_32 fru,
                                                   const DBO_TYPE dbo_type,
                                                   DBOLB_ADDR *set_addr,
                                                   FLARE_DRIVE_TYPE drive_type,
                                                   const DISK_LAYOUT_TYPE DiskLayout);

DD_ERROR_CODE dd_WhatIsDefault_User_ExtUse_SetRev(const DD_SERVICE service, 
                                         const DBO_REVISION rev,
                                         const DBO_TYPE dbo_type,
                                         DBO_REVISION *dbo_rev);

DD_ERROR_CODE dd_WhatIsNumUserExtUseTypes(const DD_SERVICE service,
                                 const DBO_REVISION rev,
                                 const DBO_TYPE dbo_type,
                                 const DBO_REVISION curr_mdde_rev,
                                 UINT_32 *num_types);

DD_ERROR_CODE dd_ChooseMasterDde(const DD_SERVICE service, 
                                 const DBO_REVISION rev,
                                 MASTER_DDE_DATA *mdde_buf,
                                 const UINT_32 fru,
                                 const UINT_32 blk_count,
                                 UINT_8E *mdde_raw_arr[DD_NUM_MASTER_DDE_COPIES],
                                 DBOLB_ADDR mdde_addr_arr[DD_NUM_MASTER_DDE_COPIES],
                                 UINT_32 *validity_mask_ptr,
                                 FLARE_DRIVE_TYPE drive_type,
                                 const DBO_REVISION current_mdde_rev);

DD_ERROR_CODE dd_ChooseValidMasterDde(const DD_SERVICE service, 
                                      const DBO_REVISION rev,
                                      MASTER_DDE_DATA *mdde_buf,
                                      const UINT_32 fru,
                                      const UINT_32 blk_count,
                                      UINT_8E *mdde_raw_arr[DD_NUM_MASTER_DDE_COPIES],
                                      DBOLB_ADDR mdde_addr_arr[DD_NUM_MASTER_DDE_COPIES],
                                      UINT_32 *validity_mask_ptr,
                                      BOOL check_crc_only,
                                      FLARE_DRIVE_TYPE drive_type);

DD_ERROR_CODE dd_FormatMDDE(MASTER_DDE_DATA *mdde_ptr);
DD_ERROR_CODE dd_FormatDDE(DDE_DATA *dde_ptr);

DD_ERROR_CODE dd_Choose_User_ExtUse_Dde(const DD_SERVICE service, 
                                        const DBO_REVISION rev,
                                        const DBO_TYPE dbo_type,
                                        DDE_DATA *dde_buf,
                                        const UINT_32 fru,
                                        const UINT_32 blk_count,
                                        UINT_8E *dde_raw_arr[DD_NUM_DDE_COPIES],
                                        DBOLB_ADDR dde_addr_arr[DD_NUM_DDE_COPIES],
                                        UINT_32 *validity_mask_ptr,
                                        UINT_32 *dd_swap_ptr,
                                        FLARE_DRIVE_TYPE drive_type,
                                        const DBO_REVISION current_dde_rev);

DD_ERROR_CODE dd_ChooseValidDde(const DD_SERVICE service,
                                const DBO_REVISION rev,
                                DBO_TYPE dde_type,
                                DDE_DATA *dde_buf,
                                const UINT_32 fru,
                                const UINT_32 blk_count,
                                UINT_8E *dde_raw_arr[DD_NUM_DDE_COPIES],
                                DBOLB_ADDR dde_addr_arr[DD_NUM_DDE_COPIES],
                                UINT_32 *validity_mask_ptr,
                                BOOL check_crc_only,
                                DBO_REVISION mdde_rev);

DD_ERROR_CODE dd_CreateManufactureZeroMark(const DD_SERVICE service, 
                                  const DBO_REVISION rev,
                                  const DBO_REVISION mzm_rev,
                                  UINT_8E *buff,
                                  UINT_32 *size_ptr);

DD_ERROR_CODE dd_ChooseDde(const DD_SERVICE service,
                           const DBO_REVISION rev,
                           const DBO_TYPE dbo_type,
                           DDE_DATA *dde_buf,
                           const UINT_32 fru,
                           const UINT_32 blk_count,
                           UINT_8E *dde_raw_arr[DD_NUM_DDE_COPIES],
                           DBOLB_ADDR dde_addr_arr[DD_NUM_DDE_COPIES],
                           UINT_32 *validity_mask_ptr,
                           UINT_32 *dd_swap_ptr,
                           FLARE_DRIVE_TYPE drive_type,
                           const DBO_REVISION current_mdde_rev);

DD_ERROR_CODE dd_GetDefaultDde(const DD_SERVICE service,
                               const DBO_REVISION rev,
                               const UINT_32 fru,
                               const DBO_TYPE dbo_type,
                               DDE_DATA *dde,
                               FLARE_DRIVE_TYPE drive_type,
                               const DBO_REVISION current_mdde_rev);

DD_ERROR_CODE dd_ManType2Region(const DD_SERVICE service,
                       const DBO_REVISION rev,
                       const DBO_TYPE man_type,
                       DD_REGION_TYPE *region_ptr);

DD_ERROR_CODE dd_WhatIsRegionSpace(const DD_SERVICE service,
                          const DBO_REVISION rev,
                          const UINT_32 fru,
                          const DD_REGION_TYPE region,
                          const DISK_LAYOUT_TYPE DiskLayout,
                          UINT_32 *blk_size);

#endif /* ODB_DATA_DIRECTORY_H_INCLUDED */
