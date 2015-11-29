#ifndef CORE_CONFIG_H
#define CORE_CONFIG_H 0x00000001
#define FLARE__STAR__H

/***************************************************************************
 * Copyright (C) EMC Corporation 2001-2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/****************************************************************
 * $Id: core_config.h,v 1.17.10.3 2000/10/18 15:38:32 fladm Exp $
 ****************************************************************
 *
 * Description:
 *  This file contains global information regarding the configuration
 *  of the array controller.  It contains declarations used both within
 *  Flare and by other parts of the core software.  It was created by
 *  taking the global declarations out of corona_config.h, which should
 *  include this file.
 *  The code will be able to be reconfigured by changing this file and
 *  recompiling it.
 *
 * NOTES:
 *
 * HISTORY:
 *      06/21/01 CJH    Created
 *      09/11/01 CJH    Add MAX_RG_PARTITIONS
 *      03/12/02 KFK    Bump MAX_RG_PARTITIONS to 64
 *      04/01/02 KFK    Return MAX_RG_PARTITIONS to 32 - DIMS 71622
 *      10/22/02 HEW    Add L1 cache defines from corona_config, so
 *                      admin can access them.
 *      11/26/02 JK     Modified for 3 GB write cache support.
 *      09/11/03 CJH    Fix FRU & LUN constants
 ****************************************************************/

#include "csx_ext.h"
#include "core_config_runtime.h"
#include "environment.h"

/* MAX_SCSI_BUS defines the highest numbered bus attached to the
 * controller.  The lowest numbered bus is assumed to be zero.
 * UMODE_ENV now uses the same number of BE buses
 */
#define CM_BE_BUS_MAX 16
#define MAX_SCSI_BUS (CM_BE_BUS_MAX - 1)
 
/* SCSI_BUS_COUNT is the total number of SCSI buses attached to the controller.
 */
#define SCSI_BUS_COUNT      (MAX_SCSI_BUS + 1)
#define PHYSICAL_BUS_COUNT  (SCSI_BUS_COUNT)

/**********************************************************************
 *      START FIBRE / SAS DEFINES
 *********************************************************************/
#define ENCLOSURE_SLOTS_15      15
#define ENCLOSURE_SLOTS_25      25
#define ENCLOSURE_SLOTS_12      12
#define ENCLOSURE_SLOTS_60      60
#define ENCLOSURE_SLOTS_30      30

#define ENCLOSURE_SLOTS         ENCLOSURE_SLOTS_60  // Max disks per enclosure
#define ENCLOSURES_PER_BUS      10       // enclosures per bus
#define EXPANDERS_PER_ENCLOSURE 2   // expanders per enclosure
#define EXPANDERS_PER_BUS   (ENCLOSURES_PER_BUS * EXPANDERS_PER_ENCLOSURE)
#define EXPANDER_COUNT      (SCSI_BUS_COUNT * EXPANDERS_PER_BUS)

#ifdef FEQ_ENV
#define ENCLOSURE_COUNT     13
#else
// total encls.
#define ENCLOSURE_COUNT     (SCSI_BUS_COUNT * ENCLOSURES_PER_BUS)
#endif

/* SCSI_TARGET_COUNT defines the total number of targets which may
 * be attached to the SCSI bus at any time.
 */
#define SCSI_TARGET_COUNT       120

/* FRU_COUNT is no longer based on SCSI_BUS_COUNT, since that varies by
 * platform.  Since RG_USER_COUNT is based on FRU_COUNT and FRU signatures
 * are based on group IDs, the FRU_COUNT has to be fixed across different
 * platforms.  For those cases where (e.g. for performance reasons) the
 * actual FRU count for the platform is desired, PLATFORM_FRU_COUNT can be
 * used.  Generally, arrays should be sized at FRU_COUNT to prevent upgrade
 * problems.
 *
 * **NOTE** The number of nonvol records is directly based off of
 * FRU_COUNT.  Please make sure there is sufficient
 * space in the nonvol for the extra nonvol write records before
 * increasing FRU_COUNT.
 * ** UPDATE** 9/1/07:  The Dreadnought has increase the number of
 * FRUs beyond the available space in the nonvol ram to store the hash
 * table records.  Going forward the nonvol ram will not store the
 * hash table. 
 */
#define FRU_COUNT               1000

/* The definition for PLATFORM_FRU_COUNT is being removed from here
  * as it is getting defined in the CCR library as part of common bundle effort.
  */

/* Definitions for BYTES_PER_BLOCK and BE_BYTES_PER_BLOCK.
 */
#define BYTES_PER_BLOCK     (512)
#define WORDS_PER_BLOCK     (BYTES_PER_BLOCK / sizeof(UINT_32))

/* SECTOR_OVERHEAD_BYTES defines the number of bytes appended
 * to each sector.  These bytes are used for checksums, sequence bits, etc.
 */
#define SECTOR_OVERHEAD_BYTES 8

/* SECTOR_OVERHEAD_WORDS defines the number of words appended to each sector. */
#define SECTOR_OVERHEAD_WORDS (SECTOR_OVERHEAD_BYTES / sizeof(UINT_32))

#define BE_BYTES_PER_BLOCK      (BYTES_PER_BLOCK + SECTOR_OVERHEAD_BYTES)

/* 
 * Maximum number of disks in a logical unit
 */
#define MAX_DISK_ARRAY_WIDTH 16


/*
 * The bus/enclosure that contains the vault/db is special. Use defines to
 * document them in the code.
 */
#define VAULT_BUS_NUM                   0
#define VAULT_ENCL_NUM                  0
#define VAULT_ENCL_ADDR                 0   // encl_addr has both bus and enclnum

#define FIRST_NON_VAULT_BUS_NUM         1



/***************************************************************
 *
 *  ARRAY CONFIGURATION DEFINES
 *
 *   This section has constants which are used by Hostside (TCD/TDD) and Admin.
 *
 *   Several of these values are now defined by core_config_runtime.h
 *   and the CCR library in core_config_runtime.c
 *
 *   The PLATFORM_HOST_ACCESSIBLE_FLARE_LUNS constant determines the number
 *   of Flare "General" (or user) LUNs that can be bound for that particular
 *   platform (2/300,4/500,6/700).
 *
 **************************************************************/

#define SP_COUNT_PER_ARRAY              2   // Storage Processors per Array.
#define MV_PORT_COUNT_PER_SP            1   // Mirror View ports per SP.

// Celerra needs 16 initiators per SP on VNX - this is increasing for Rockies -
// but none are needed on KH
#ifndef C4_INTEGRATED
#define MAX_CELERRA_INITIATORS_PER_SP   16   
#else
#define MAX_CELERRA_INITIATORS_PER_SP    0   
#endif 

#define MAX_LOGIN_SESSION_ENTRIES_PER_SP     (MAX_USER_LOGIN_SESSION_ENTRIES_PER_SP + CMI_PORT_COUNT_PER_SP)

// Number of celerra boot devices, these Luns are seen by the Celerra initiators 
// hence are part of the MAX_HOST_ACCESSIBLE_LUNS (but not part of MaxPoolLus or PlatformHostAccessibleFlareLuns).

#define MAX_CELERRA_BOOT_LUNS                 6

// Max number of "general" or "user" LUNs supported by the GLUT.  Put here to export to mgmt.
// Hardware (simulation version is defined in corona_config.h).

#define FLARE_MAX_USER_LUNS  8192

// Max number of LUNs visible to array users. 
#ifdef C4_INTEGRATED
#define MAX_USER_VISIBLE_LUNS               (CSX_MAX(MAX_POOL_LUS, MAX_BOUND_VVOLS_FOR_BLOCK) + MAX_ADVANCED_SNAPSHOT_LUNS + (MAX_PROTOCOL_ENDPOINTS_PER_SP * SP_COUNT_PER_ARRAY))
#else
#define MAX_USER_VISIBLE_LUNS               (MAX_FAR_SHADOW_LUS + MAX_SNAP_LUS + CSX_MAX(MAX_POOL_LUS, PLATFORM_HOST_ACCESSIBLE_FLARE_LUNS) + MAX_ADVANCED_SNAPSHOT_LUNS)
#endif

// Max number of LUNs visible to Hosts.
#define MAX_HOST_ACCESSIBLE_LUNS            (MAX_USER_VISIBLE_LUNS + MAX_CELERRA_BOOT_LUNS)

// The system private luns: EFD Cache, DRAM Cache, and PSM, are exposed to the Lower Redirector
// as are the MAX_CELERRA_BOOT_LUNS
// At least (FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_LUN_LAST - FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_LUN_FIRST + 1)
#define MAX_LR_CAPABLE_FLARE_PRIVATE_LUNS 37

//
//  Use this for gross Device map check.  In the extreme limit each Flare LUN
//  could also be a MetaLun with a single constituent, giving us twice
//  the number of LUNs from a single driver.
//
//  DEPRECATED: "gross" check is outaded. Each driver has own limits largely unrelated to any drivers above or below
// #define MAX_DRIVER_MAPPABLE_LUNS                ( PLATFORM_HOST_ACCESSIBLE_FLARE_LUNS * 2 )

// Storage Group Limits
#define MAX_LUN_MAPS_IN_USER_SG_ON_ANY_PLATFORM 8190

#define MAX_LUN_MAPS_IN_PHYSICAL_SG     (CSX_MAX(MAX_POOL_LUS, PLATFORM_HOST_ACCESSIBLE_FLARE_LUNS) + MAX_ADVANCED_SNAPSHOT_LUNS)
#define MAX_LUN_MAPS_IN_USER_SG        ((MAX_LUNS_PER_USER_DEFINED_SG>MAX_LUN_MAPS_IN_USER_SG_ON_ANY_PLATFORM) ? MAX_LUN_MAPS_IN_USER_SG_ON_ANY_PLATFORM : MAX_LUNS_PER_USER_DEFINED_SG)

#define MAX_LUN_MAPS_IN_ANY_STORAGE_GROUP                                                   \
    (MAX_LUN_MAPS_IN_USER_SG>MAX_LUN_MAPS_IN_PHYSICAL_SG?               \
        MAX_LUN_MAPS_IN_USER_SG:MAX_LUN_MAPS_IN_PHYSICAL_SG)

#define DEFAULT_MAX_LUN_MAPS                256
#define DEFAULT_MAX_HOST_LUN_NUMBER     (DEFAULT_MAX_LUN_MAPS-1)


#define MAX_HOST_VISIBLE_LUN_NUMBER     (CMI_LUN-1)

#ifdef C4_INTEGRATED
#define ARRAY_DEFINED_SG_COUNT          8   // Storage Groups defined by array.
#else
#define ARRAY_DEFINED_SG_COUNT          3   // Storage Groups defined by array.
#endif /* C4_INTEGRATED - C4ARCH - blockshim_fakeport */

#define MAX_USER_DEFINED_SGS_PER_ARRAY  (SP_COUNT_PER_ARRAY * MAX_USER_DEFINED_SGS_PER_SP)

#define MAX_PROTOCOL_ENDPOINTS_PER_ARRAY (SP_COUNT_PER_ARRAY * MAX_PROTOCOL_ENDPOINTS_PER_SP) 

#if !defined(MIN_USER_DEFINED_SGS_PER_ARRAY) || (MIN_USER_DEFINED_SGS_PER_ARRAY == 0)
#define MIN_USER_DEFINED_SGS_PER_ARRAY   MAX_USER_DEFINED_SGS_PER_ARRAY
#endif

#define MAX_STORAGE_GROUPS_PER_ARRAY    (MAX_USER_DEFINED_SGS_PER_ARRAY + MAX_PROTOCOL_ENDPOINTS_PER_ARRAY +  \
                                        ARRAY_DEFINED_SG_COUNT)

#if !defined(MAX_STORAGE_CONNECTED_HOSTS) || (MAX_STORAGE_CONNECTED_HOSTS == 0)
#define MAX_STORAGE_CONNECTED_HOSTS     MAX_USER_DEFINED_SGS_PER_ARRAY
#endif

#if !defined(MIN_STORAGE_CONNECTED_HOSTS) || (MIN_STORAGE_CONNECTED_HOSTS == 0)
#define MIN_STORAGE_CONNECTED_HOSTS     MAX_STORAGE_CONNECTED_HOSTS
#endif

/************************************************************************
 * The following constants are used for memory allocatin in
 * hostside and the layered drivers. The MAX_IOS_PER_PORT and 
 * MAX_IOS_PER_DISK are actually defined in .inf files and available
 * through the registry, but this is not convenient for memory allocation.
 * Hostside verifies that that the registry numbers are <= the constants
 * defined below when it reads the constants from the registry.
 ************************************************************************/

#define MAX_OPS_PER_FE_PORT 2048
#define MAX_DATA_PHASES_PER_FE_PORT (MAX_OPS_PER_FE_PORT + 64)
#define MAX_OPS_PER_SPINDLE 14      // Defined in base.inf file
#define MAX_OPS_PER_LUN     (MAX_OPS_PER_SPINDLE * MAX_DISK_ARRAY_WIDTH)
#define MAX_OPS_PER_SP      (MAX_OPS_PER_FE_PORT * FE_PORT_COUNT_PER_SP)

/***************************************************************
 * L1 Cache Defines
 * Set sizes for L1 Cache requirements 
 ***************************************************************/
#if defined(UMODE_ENV) || defined(SIMMODE_ENV)
#define MIN_RC_MEM_SIZE       2   /* MB needed for minimum sized cache */
#define MIN_WC_MEM_SIZE       2   /* MB needed for minimum sized cache */
#else
#define MIN_RC_MEM_SIZE       4   /* MB needed for minimum sized cache */
#define MIN_WC_MEM_SIZE       3   /* MB needed for minimum sized cache */
#endif

// See core_config_runtime.h for definitions of:
// MAX_WC_MEM_SIZE and EFFECTIVE_MAX_WC_MEM_SIZE
#if (!defined(UMODE_ENV) && !defined(SIMMODE_ENV))  ||  defined(ADMIN_ENV)
// If  bus 0 speed is 2 G, the maximum write cache size will be 6 Gbytes
#define MAX_WCA_FC_2GB_WC_SIZE      6144
// If  bus 0 speed is 4 G, the maximum write cache size will be 16 Gbytes
#define MAX_WCA_FC_4GB_WC_SIZE      16600
// If Vault drives are SAS drives, the maximum write cache size is 16 GB
// For now Use the same define for both 3G and 6G SAS vault drives
#define MAX_WCA_SAS_WC_SIZE         16600
#define MAX_WCA_FSSD_WC_SIZE        16600
// If Vault drives are SATA drives, the maximum write cache size is 6GB
// For now Use the same define for both 3G and 6G SATA vault drives
#define MAX_WCA_SATA_WC_SIZE        6144
//  This is only used to bind the vault
#define MAX_WRITE_CACHE_VAULT_SIZE  (30*1024)

#ifdef C4_INTEGRATED
//  Vault size for VMSIM
#define MAX_WRITE_CACHE_VAULT_SIZE_VMSIM  (256)
#endif /* C4_INTEGRATED - C4ARCH - memory_config */

// Limit to WC size without WCA enabled
#define WC_SZ_LIMIT_NO_WCA          3072

// Max Flare read cache for all backend protocols in MB.
#define MAX_RC_SIZE_ALL_PROTOCOLS   26800

#else
/* 
 * Note!! Write cache size must at LEAST be 10 MB to support the number 
 * of RGs that we support in the simulator!!
 */
// If  bus 0 speed is 2 G, the maximum write cache size will be 10 MB for user sim
#define MAX_WCA_FC_2GB_WC_SIZE      10
// If  bus 0 speed is 4 G, the maximum write cache size will be 10 MB for user sim
#define MAX_WCA_FC_4GB_WC_SIZE      10
// For now assume write cache size of 10 MB is sufficient for simulation with SAS BE0
#define MAX_WCA_SAS_WC_SIZE         10
#define MAX_WCA_SATA_WC_SIZE        10
#define MAX_WCA_FSSD_WC_SIZE        10
//  write cache size must at LEAST be 10 MB to support the number of RGs that we support!!
#define MAX_WRITE_CACHE_VAULT_SIZE  12   /* MB maximum for vault */

#ifdef C4_INTEGRATED
//  Vault size for VMSIM
#define MAX_WRITE_CACHE_VAULT_SIZE_VMSIM  12 /* Same as above, just for successful compilation of never executed code path */
#endif /* C4_INTEGRATED - C4ARCH - memory_config */

// Limit to WC size without WCA enabled
#define WC_SZ_LIMIT_NO_WCA          10

// Max Flare read cache for all backend protocols in MB.
#define MAX_RC_SIZE_ALL_PROTOCOLS   10

#endif

#define DEFAULT_RC_MEM_SIZE   0   /* Default value for Read-cache size  */
#define DEFAULT_WC_MEM_SIZE   0   /* Default value for Write-cache size */ 

// ARS 432547: New, for SP Cache.  Used by aplu, commit, and raid.
#define VAULT_STRIPE_SIZE_FOR_SP_CACHE   384

/***************************************************************
 *
 *   RAID GROUP DEFINES 
 *
 *   The Raid Group name space is divided into 2 sections.
 *
 *                                   (SIM)
 *       RG_USER       0 - 227       0 - 24
 *       RG_SYSTEM   228 - 234      25 - 31
 *
 **************************************************************/
/* Max number of "user" raid group.  Put here
 * to export to mgmt.
 */
#if (defined(UMODE_ENV) || defined(SIMMODE_ENV))  &&  !defined(NON_FLARE_ENV)

#define MAX_RG_PARTITIONS           10  /* Partitions per RG */
#define FLARE_MAX_USER_RG           25

#else

#define MAX_RG_PARTITIONS           256  /* Partitions per RG */
#define FLARE_MAX_USER_RG           FRU_COUNT

#endif //UMODE_ENV & !NON_FLARE_ENV.

#define MAX_GROUP_IDS               8   /* Max RG's per FRU */



/**********************************************************
 *
 *   LU PARTITION DEFINES 
 *
 *   The LU Partition space on a FRU is divided
 *   into 2 sections.
 *
 *                               (hammer)     (fleet)   (sim)
 *      FRU_USER_PARTITIONS      0 - 127      0 - 255   0 - 3
 *      FRU_SYSTEM_PARTITIONS  128 - 143    256 - 271   4 - 10
 *
 *********************************************************/

#if (defined(UMODE_ENV) || defined(SIMMODE_ENV))  &&  !defined(NON_FLARE_ENV)

#define MAX_FRU_PARTITIONS          22  /* Partitions per FRU */

#else

#define MAX_FRU_PARTITIONS          288  /* Partitions per FRU */

#if (MAX_FRU_PARTITIONS != (MAX_RG_PARTITIONS + 32))
#error "Need to change MAX_FRU_PARTITIONS and MAX_RG_PARTITIONS together"
#endif
#endif //UMODE_ENV && !NON_FLARE_ENV


/*
 * This define is used with dba_fru_get_slot_mask.  It identifies the slot
 * mask bit for drive 0 and may be shifted (left) for other drives.
 */
#define SLOT_MASK_DRV0              0x0001LL



/***********************************************************************
 *
 * Miscellaneous constants
 *
 ***********************************************************************/

#define BITS_PER_BYTE       8

#define INVALID_BUS_NUM             UNSIGNED_MINUS_1
#define INVALID_ENCLOSURE           UNSIGNED_MINUS_1
#define INVALID_DRIVE_SLOT          UNSIGNED_MINUS_1
#define INVALID_FC_TARGET_ID        UNSIGNED_MINUS_1
#define INVALID_DEVICE              UNSIGNED_MINUS_1
            
#endif /* end file core_config.h */
