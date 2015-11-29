#ifndef NTMIRROR_INTERFACE_H
#define NTMIRROR_INTERFACE_H

/***************************************************************************
 *  Copyright (C)  EMC Corporation 2001-2005
 *  All rights reserved.
 *  Licensed material - property of EMC Corporation. * All rights reserved.
 ***************************************************************************/

/***************************************************************************
 * ntmirror_interface.h
 ***************************************************************************
 *
 * Description:
 *
 *  Contains defines that need to be shared between NtMirror Driver and
 *  other modules.  Most of these defines are related to the MIRROR_DATABASE
 *  sector that is stored on each of the fibre drives which participates in a
 *  mirrored NT partition, or to the ODBS/Data Directory interface we use to
 *  locate this sector on disk.
 *
 * History:
 *  
 *  15 Oct 01  MWH  Created
 *  08 Oct 02  MWH  Added Utility Partition device defines
 *  29 Jul 03  MWH  Moved device types to new ntmirror_user_interface.h file
 *  14 Aug 03  MWH  Added Mirror Version Sector for Flare Boot Partition
 *  27 Oct 03  MWH  Added NTMIRROR_MBR_OFFSET, etc.
 *
 ***************************************************************************/
#include "odb_defs.h"
#include "dd_main_service.h"    /* for DD_MS_NT_BOOT_... defines */

/****************************************************************************
 *
 * General defines used by NT Mirror Driver
 *
 ***************************************************************************/

#define SECTOR_SHIFT                           9

/* Define disk "physical" sector size */
#define META_DATA_SIZE                         SECTOR_OVERHEAD_BYTES

/* Borrow traditional bytes per block */
#define DATA_SECTOR_SIZE                       BYTES_PER_BLOCK

/* A BOOL is a 32-bit integer in the Windows world.  We want to define an
 * exact data type equivalent for this.
 */
typedef INT_32E BOOL_32E;

/****************************************************************************
 *
 * These are MIRROR_SIGNATURE-related defines
 *
 ***************************************************************************/
/************************************************************
 * Mirror Signature Format
 ************************************************************
 * A IEEE WWN Type 6 128 bit signature that allows us to
 * determine if the extents that make up a mirror are 
 * coherent at driver initialization time.
 ************************************************************/
/* Mirror Signature uses IEEE WWN Type 6 format:

    Bit | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    Byte|-------------------------------|
     0  |  NAA: 4 bits  |               |
        |----------------               |
     1  |                               |
     2  | Registered Company ID: 24 bits|
        |               |---------------|
     3  |               |   Unused (0)  |
        |-------------------------------|
     4  |                               |
     5  | Generated Sequence 1: 32-bits |
     6  |                               |
     7  |-------------------------------|
     8  |                               |
     9  | Generated Sequence 2: 32-bits |
    10  |                               |
    11  |-------------------------------|
    12  |                               |
    13  | Generated Sequence 3: 32-bits |
    14  |                               |
    15  |-------------------------------|

    Where NAA=0x6, Company ID=0x006016
*/

/* Pack the data structures we store on disk */
#pragma pack(1)

typedef struct _MIRROR_SIGNATURE
{
    UINT_32E   High;       /* IEEE ID and CLARiiON ID */
    UINT_32E   MidHigh;    /* 32-bit random number */
    UINT_32E   MidLow;     /* 32-bit random number */
    UINT_32E   Low;        /* 32-bit random number */
} MIRROR_SIGNATURE;

/* values used to construct and check validity of a MIRROR_SIGNATURE */
#define NTMIRROR_WWN_IEEE_COMPANY_ID (0x00006016)
#define NTMIRROR_WWN_IEEE_TYPE6      (6) /* 128 bits */
#define NTMIRROR_WWN_VALID           ((NTMIRROR_WWN_IEEE_COMPANY_ID | (NTMIRROR_WWN_IEEE_TYPE6 << 24)) << 4) /* aka 0x60060160 */
#define NTMIRROR_WWN_INVALID         (0xFFFFFFFF)

/****************************************************************************
 *
 * These are MIRROR_DATABASE-related defines
 *
 ***************************************************************************/
/* For now, we only support single-sector DDEs.  
 * This define will work to size a larger buffer for multi-sector DDEs if our
 * assumption that MASTER_DDE_DATA_SIZE will always be greater than the size
 * of the Boot DDEs.
 *
 * (FYI - DBOLB_PAGE_SIZE is defined as 520 in ODBS header files)
 */
//#define MIRROR_MAX_DBOLB_BUFFER_SIZE ( (((DBO_HEADER_SIZE + MASTER_DDE_DATA_SIZE) / DBOLB_PAGE_SIZE)+1) * DBOLB_PAGE_SIZE )

/* The disk signature data structure contains information that
 * identifies a disk that participates in a mirror.
 */
typedef struct _NTM_DISK_SIGNATURE
{
    UINT_8E PortNumber;
    UINT_8E PathId;
    UINT_8E TargetId;
    UINT_8E Lun;
} NTM_DISK_SIGNATURE;

/* This structure contains all of the information to describe a mirrored
 *  extent on a disk, including whether the extent information is valid.
 */
typedef struct _EXTENT_DATA
{
    BOOL_32E Valid;                /* This information has been initialized */
    NTM_DISK_SIGNATURE DiskSignature; /* Signature for the slot of a disk */
    UINT_32E   Start;                /* The starting block address of this extent. */
    UINT_32E   Size;                 /* The size (in blocks) of this extent. */                       
    UINT_32E   RB_Checkpoint;        /* The rebuild checkpoint associated with this extent. */
} EXTENT_DATA;


/**********************************************************************
 * MIRROR_DATA
 **********************************************************************
 * MirrorSignature  - Allows us to determine if both halves of a mirror
 *                    are coherent.
 * MirrorTimeStamp  - Allows us to determine which half of a mirror is
 *                    the most recent if they are incoherent at
 *                    initialization time.
 * PrimaryData      - Geometry and rebuild checkpoint of the extent that
 *                    makes up the primary half of the mirror.
 * SecondaryData    - Geometry and rebuild checkpoint fo the extent that
 *                    makes up the secondary half of the mirror.
 * WWN_Seed         - The WWN seed (chassis signature). This identifies
 *                    the chassis that the mirror is associated with.
 **********************************************************************/
typedef struct _MIRROR_DATABASE
{
    MIRROR_SIGNATURE    MirrorSignature;
    LARGE_INTEGER       MirrorTimeStamp;
    EXTENT_DATA         PrimaryData;
    EXTENT_DATA         SecondaryData;
    UINT_32E            ChassisWWNSeed;
} MIRROR_DATABASE;

#pragma pack()

/* current size allocated for Mirror Database on disk */
#define MIRROR_MDB_SIZE 1 /* Reserve 1 sector for the Mirror Database */
#define MIRROR_MDB_SIZE_IN_BYTES (MIRROR_MDB_SIZE * DATA_SECTOR_SIZE)

// Defines for WWN seeds
#define WWN_SEED_ON_MASK   0x00200000   // Bit 21 must be set in a WWN seed
#define WWN_SEED_OFF_MASK  0x80100000   // Bits 31 and 20 must be cleared in a WWN seed
#define VALID_WWN_SEED     0x5EED5EED   // A recognizable valid WWN seed
#define INVALID_WWN_SEED   0x00000000   // A recognizable invalid WWN seed 

#define WWN_SEED_VALID(wwn_seed) (((wwn_seed & WWN_SEED_ON_MASK) == WWN_SEED_ON_MASK) && \
								  ((wwn_seed & WWN_SEED_OFF_MASK) == 0x00000000))
#define WWN_SEEDS_MATCH(wwn_seed1,wwn_seed2) (WWN_SEED_VALID(wwn_seed1) && \
                                              WWN_SEED_VALID(wwn_seed2) && \
                                              (wwn_seed1 == wwn_seed2))

/* Size of Mirror Version Sector, found at the start of all Partitions.
 */
#define NTMIRROR_MVS_SIZE   1
#define NTMIRROR_MVS_SIZE_IN_BYTES (NTMIRROR_MVS_SIZE * DATA_SECTOR_SIZE)

/****************************************************************************
 *
 * Mirrored Partition Layout
 *
 * LBA  Description
 * 0    MVS - Mirror Version Sector
 * 1    MDB - Mirror Database Sector
 * x-n  Partition exposed to OS, beginning with Master Boot Record (MBR)
 *
 * CX Series: x = 2
 *
 * Where n = length of region as defined by ODBS.  Therefore, the size of the
 * partition exposed to OS is n - NTMIRROR_MBR_OFFSET.
 *
 * Notes: 
 * 1. MVS added for Release 13 (XP only) for all partitions.
 * 2. Utility Partitions (added for Release 11) have always contained a
 *    version sector, known as a UVS.  The MVS replaces the UVS to be
 *    consistant for all mirrored partitions.
 * 3. On Piranha systems, both the MVS and MBR are aligned for SATA
 *    purposes, so in that case, NTMIRROR_MBR_OFFSET will NOT immediately
 *    follow NTMIRROR_MDB_OFFSET - use the offsets instead of doing the math
 *    in multiple places throughout the code!
 *
 ****************************************************************************/
/* 
 * Offsets in sectors from start of region defined by ODBS.
 */
#define NTMIRROR_MVS_OFFSET (0)
#define NTMIRROR_MDB_OFFSET (NTMIRROR_MVS_OFFSET + NTMIRROR_MVS_SIZE)
#define NTMIRROR_MBR_OFFSET (NTMIRROR_MDB_OFFSET + MIRROR_MDB_SIZE)


/* define used to check for an invalid MDB address */
#define INVALID_MIRROR_DB_ADDRESS 0xFFFFFFFF

/* Special write_stamp and shed_stamp values for crash dump IO.
 * Used by NtMirror Driver to ignore checksum errors on these sectors.
 *
 * These errors are caused by the fact that a small part of the dump is
 * dumping the memory area that we use to process the dump!  Therefore,
 * our processing overwrites these areas after we checksum them, but 
 * before (and technically, after) we write the data to disk.
 */
#define NTMIRROR_DUMP_WRITE_STAMP   (0x5544)    /* 'UD' */
#define NTMIRROR_DUMP_SHED_STAMP    (0x504D)    /* 'PM' */

/* "Normal" write and shed stamps */
#define NTMIRROR_WRITE_STAMP        (0x0000)
#define NTMIRROR_SHED_STAMP         (0x0000)


#endif /* NTMIRROR_INTERFACE_H */
