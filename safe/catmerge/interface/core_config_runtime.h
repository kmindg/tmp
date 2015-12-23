#ifndef __CORE_CONFIG_RUNTIME__
#define __CORE_CONFIG_RUNTIME__

/***************************************************************************
 * Copyright (C) EMC Corporation 2007 - 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *
 * File Name:
 *      core_config_runtime.h
 *
 * Contents:
 *      The Core Config Runtime library main header file, which will be
 *      included by core_config.h and environment.h
 *
 * Revision History:
 *  01/05/07   Mike Hamel       Created
 *
 ***************************************************************************/

#define CCR_API __stdcall

#include "generic_types.h"
#include "spid_enum_types.h"

#if defined(__cplusplus)
extern "C"
{
#endif

#define MAX_DART_INSTANCES_PER_SP                     5

// Max system LUNs, e.g. SYSTEM_FASTVP_LUN, SYSTEM_METRICS_LUN
#define MAX_SYSTEM_LUN_MAPS_IN_PHYSICAL_SG              2

/*********************  EXPORTED INTERFACE structures  ***********************/

/* The following structure is used to initialize the core config members from the Registry */
typedef struct
{
    UINT_32    *CoreConfigParam;
    const char *RegistryValue;
} CORE_CONFIG_RUNTIME_REG_HELPER;

/* This structure is used to initialize the core config members from the Registry.
   Use this structure if you need to read different sections of the Registry
 */
typedef struct
{
    UINT_32       *CoreConfigParam;
    csx_cstring_t  RegistryPath;
    const char    *RegistryValue;
} CORE_CONFIG_RUNTIME_ADV_REG_HELPER;

typedef struct _CORE_CONFIG_HWATTRIBUTES
{
    UINT_32 PlatformMaxLunsPerRG;
    UINT_32 PlatformMaxEnclPerBus;
    UINT_32 PlatformMaxSlics;
    UINT_32 PlatformMaxMezzanines;
    UINT_32 PlatformMaxFbeMemoryBudgetMB;
}CORE_CONFIG_HWATTRIBUTES;

typedef struct _CORE_CONFIG_PORT_LIMITS
{   
    UINT_32 PlatformMaxSasBeCount;
    UINT_32 PlatformMaxSasFeCount;
    UINT_32 PlatformMaxFcFeCount;
    UINT_32 PlatformMaxiSCSI1GFeCount;
    UINT_32 PlatformMaxiSCSI10GFeCount;
    UINT_32 PlatformMaxFcoeFeCount;
    UINT_32 PlatformMaxCombinediSCSIFeCount;
}CORE_CONFIG_PORT_LIMITS;

typedef struct _CORE_CONFIG_RUNTIME
{
    UINT_32 PlatformMaxBeCount;
    UINT_32 CmiPortCountPerSP; 
    UINT_32 MaxFarShadowLus;
    UINT_32 MaxSnapLus;
    UINT_32 MaxPoolLus;
    UINT_32 MaxAdvancedSnapshotLuns;
    UINT_32 MaxPoolsPerSystem;
    UINT_32 MaxDisksPerPool;
    UINT_32 MaxDisksUsableForAllPools;
    UINT_32 MaxFeatureStorageLunsPerPool;
    UINT_32 MaxFeatureStorageLuns;
    UINT_32 FEPortCountPerSP;
    UINT_32 LegacyFEPortCountPerSP;
    UINT_32 MaxPortalGroupEntries;
    UINT_32 PlatformHostAccessibleFlareLuns;
    UINT_32 MaxHostEntries;
    UINT_32 MaxInitiatorEntries;
    UINT_32 MaxITNexusEntriesPerSP;
    UINT_32 MaxUserDefinedSgsPerSP;
    UINT_32 MaxUserLoginSessionEntriesPerSP;
    UINT_32 MaxLoginSessionEntriesPerPort;
    UINT_32 AuthMaxAuthenticationRecs;
    UINT_32 MaxConnectLogRecordsPerSP;
    UINT_32 MaxConnectLogFilterEntries;
    UINT_32 MaxHostLunNumber;
    UINT_32 MaxVLUsPerSP;
    UINT_32 MaxLunsPerUserDefinedSG;
    UINT_32 PlatformMaxWcSize;
    UINT_32 PlatformFruCount;
    UINT_32 RgUserCount;
    UINT_32 NaviMemoryBudgetMB;
    UINT_32 MinRecReadCacheSize;			//Minimum Recommended Flare Read Cache Size	
    UINT_32 MaxPlatformPerCoreMCCBeRequests;
    UINT_32 MaxPlatformFCTInternalBeRequests;
    UINT_32 MaxPlatformPerCoreMCRFeRequests;
    UINT_32 MaxPoolLUsforMR;
    UINT_32 MaxPlatformFastCacheSizeGB;
    UINT_32 MaxBoundVVolsForBlock;
    UINT_32 MaxProtocolEndpointsPerSP;   
    UINT_32 MinVVolALU;
    UINT_32 MaxVVolALU;
    //FBE HW Attributes
    struct _CORE_CONFIG_HWATTRIBUTES HwAttributes;
    //Port Limits
    struct _CORE_CONFIG_PORT_LIMITS PortLimits;
} CORE_CONFIG_RUNTIME;

extern CORE_CONFIG_RUNTIME   CoreConfigRuntime_Global;
extern CORE_CONFIG_RUNTIME *pCoreConfigRuntime_Global;

/*********************  EXPORTED INTERFACE FUNCTIONS  ***********************/
extern  UINT_32 CCR_API ccr_getPlatformMaxBeCount(void);
extern  UINT_32 CCR_API ccr_getCmiPortCountPerSP(void);
extern  UINT_32 CCR_API ccr_getMaxFarShadowLus(void);
extern  UINT_32 CCR_API ccr_getMaxSnapLus(void);
extern  UINT_32 CCR_API ccr_getMaxPoolLus(void);
extern  UINT_32 CCR_API ccr_getMaxAdvancedSnapshotLuns(void);
extern  UINT_32 CCR_API ccr_getMaxFeatureStorageLuns(void);
extern  UINT_32 CCR_API ccr_getFEPortCountPerSP(void);
extern  UINT_32 CCR_API ccr_getLegacyFEPortCountPerSP(void);
extern  UINT_32 CCR_API ccr_getMaxPortalGroupEntries(void);
extern  UINT_32 CCR_API ccr_getPlatformHostAccessibleFlareLuns(void);
extern  UINT_32 CCR_API ccr_getMaxHostEntries(void);
extern  UINT_32 CCR_API ccr_getMaxInitiatorEntries(void);
extern  UINT_32 CCR_API ccr_getMaxITNexusEntriesPerSP(void);
extern  UINT_32 CCR_API ccr_getMaxUserDefinedSgsPerSP(void);
extern  UINT_32 CCR_API ccr_getMaxUserLoginSessionEntriesPerSP(void);
extern  UINT_32 CCR_API ccr_getMaxLoginSessionEntriesPerPort(void);
extern  UINT_32 CCR_API ccr_getAuthMaxAuthenticationRecs(void);
extern  UINT_32 CCR_API ccr_getMaxConnectLogRecordsPerSP(void);
extern  UINT_32 CCR_API ccr_getMaxConnectLogFilterEntries(void);
extern  UINT_32 CCR_API ccr_getMaxHostLunNumber(void);
extern  UINT_32 CCR_API ccr_getMinVVolALU(void);
extern  UINT_32 CCR_API ccr_getMaxVVolALU(void);
extern  UINT_32 CCR_API ccr_getMaxVLUsPerSP(void);
extern  UINT_32 CCR_API ccr_getMaxLunsPerUserDefinedSG(void);
extern  UINT_32 CCR_API ccr_getPlatformMaxWcSize(void);
extern  UINT_32 CCR_API ccr_getPlatformFruCount(void);
extern  UINT_32 CCR_API ccr_getRgUserCount(void);
extern  UINT_32 CCR_API ccr_getNaviMemoryBudgetMB(void);
extern  UINT_32 CCR_API ccr_getMaxDisksPerPool(void);
extern  UINT_32 CCR_API ccr_getMaxDisksUsableForAllPools(void);
extern  UINT_32 CCR_API ccr_getPlatformMinRecReadCacheSize(void);
extern  UINT_32 CCR_API ccr_getMaxPlatformPerCoreMCCBeRequests(void);
extern  UINT_32 CCR_API ccr_getMaxPlatformFCTInternalBeRequests(void);
extern  UINT_32 CCR_API ccr_getMaxPlatformFastCacheSizeGB(void);
extern  UINT_32 CCR_API ccr_getMaxPlatformPerCoreMCRFeRequests(void);
extern  UINT_32 CCR_API ccr_getMaxPoolLUsforMR(void);
extern  UINT_32 CCR_API ccr_getMaxBoundVVolsForBlock(void);
extern  UINT_32 CCR_API ccr_getMaxProtocolEndpointsPerSP(void);
extern  BOOL    CCR_API ccrGetPlatformType(SPID_HW_TYPE *pPlatformType);
#if defined(CCR_MODE_SIMULATION) || defined(SIMMODE_ENV)
extern SPID_HW_TYPE CCR_API ccrGetPlatformConfig(csx_pchar_t platform);
#endif //defined(CCR_MODE_SIMULATION) || defined(SIMMODE_ENV)
extern  UINT_32 CCR_API ccr_getPlatformMaxLunsPerRG(void);
extern  UINT_32 CCR_API ccr_getPlatformMaxEnclPerBus(void);
extern  UINT_32 CCR_API ccr_getPlatformMaxSlics(void);
extern  UINT_32 CCR_API ccr_getPlatformMaxMezzanines(void);
extern  UINT_32 CCR_API ccr_getPlatformMaxFbeMemoryBudgetMB(void);
//Port Limits
extern  UINT_32 CCR_API ccr_getPlatformMaxSasBeCount(void);
extern  UINT_32 CCR_API ccr_getPlatformMaxSasFeCount(void);
extern  UINT_32 CCR_API ccr_getPlatformMaxFcFeCount(void);
extern  UINT_32 CCR_API ccr_getPlatformMaxiSCSI1GFeCount(void);
extern  UINT_32 CCR_API ccr_getPlatformMaxiSCSI10GFeCount(void);
extern  UINT_32 CCR_API ccr_getMPlatformMaxFcoeFeCount(void);
extern  UINT_32 CCR_API ccr_getPlatformMaxCombinediSCSIFeCount(void);


/*********************** EXPORTED INTERFACE MACROS ***************************/
/*                   Formerly defined in core_config.h                       */

#define PLATFORM_MAX_BE_COUNT                 (ccr_getPlatformMaxBeCount())
#define CMI_PORT_COUNT_PER_SP                 (ccr_getCmiPortCountPerSP())        // Peer-to-peer FC ports.
#define MAX_FAR_SHADOW_LUS                    (ccr_getMaxFarShadowLus())
#define MAX_SNAP_LUS                          (ccr_getMaxSnapLus())
#define MAX_POOL_LUS                          (ccr_getMaxPoolLus())
#define MAX_ADVANCED_SNAPSHOT_LUNS            (ccr_getMaxAdvancedSnapshotLuns())  // Host visible
#define MAX_FEATURE_STORAGE_LUNS              (ccr_getMaxFeatureStorageLuns())    // FSL: Feature Storage LUN
#define FE_PORT_COUNT_PER_SP                  (ccr_getFEPortCountPerSP())         // Front-end ports per SP.
#define LEGACY_FE_PORT_COUNT_PER_SP           (ccr_getLegacyFEPortCountPerSP())   // Headhunter to combo conversion.
#define MAX_PORTAL_GROUP_ENTRIES              (ccr_getMaxPortalGroupEntries())

#define PLATFORM_HOST_ACCESSIBLE_FLARE_LUNS   (ccr_getPlatformHostAccessibleFlareLuns() + MAX_SYSTEM_LUN_MAPS_IN_PHYSICAL_SG)
#define MAX_POOL_LUS_FOR_MR                   (ccr_getMaxPoolLUsforMR())

#ifndef C4_INTEGRATED
#define MAX_HOST_ENTRIES                      (ccr_getMaxHostEntries())
#define MAX_INITIATOR_ENTRIES                 (ccr_getMaxInitiatorEntries())
#define MAX_I_T_NEXUS_ENTRIES_PER_SP          (ccr_getMaxITNexusEntriesPerSP())
#define MAX_USER_LOGIN_SESSION_ENTRIES_PER_SP (ccr_getMaxUserLoginSessionEntriesPerSP())

#else // C4_INTEGRATED
//
//  We need to accomodate the Dart instances without taking away from the user's actual platform limits.
//
#define MAX_HOST_ENTRIES                    (ccr_getMaxHostEntries() + 5)
#define MAX_INITIATOR_ENTRIES               (ccr_getMaxInitiatorEntries() + 5)
#define MAX_I_T_NEXUS_ENTRIES_PER_SP        (ccr_getMaxITNexusEntriesPerSP() + 5)
#define MAX_USER_LOGIN_SESSION_ENTRIES_PER_SP (ccr_getMaxUserLoginSessionEntriesPerSP() + MAX_DART_INSTANCES_PER_SP)
#endif /* C4_INTEGRATED - C4ARCH - blockshim_fakeport */

/* Removed 5 Fakeports added in here, as they are already accounted for in ARRAY_DEFINED_SG_COUNT  */
#define MAX_USER_DEFINED_SGS_PER_SP           (ccr_getMaxUserDefinedSgsPerSP())

#define MAX_LUNS_PER_USER_DEFINED_SG          (ccr_getMaxLunsPerUserDefinedSG())

#define MAX_PROTOCOL_ENDPOINTS_PER_SP          (ccr_getMaxProtocolEndpointsPerSP())
#define MAX_BOUND_VVOLS_FOR_BLOCK              (ccr_getMaxBoundVVolsForBlock()) 

#ifndef C4_INTEGRATED
#define MAX_LOGIN_SESSION_ENTRIES_PER_PORT    (ccr_getMaxLoginSessionEntriesPerPort())
#else // C4_INTEGRATED
#define MAX_LOGIN_SESSION_ENTRIES_PER_PORT    (ccr_getMaxLoginSessionEntriesPerPort() + MAX_DART_INSTANCES_PER_SP)
#endif /* C4_INTEGRATED - C4ARCH - blockshim_fakeport */

#define AUTH_MAX_AUTHENTICATION_RECS          (ccr_getAuthMaxAuthenticationRecs())
#define MAX_CONNECT_LOG_RECORDS_PER_SP        (ccr_getMaxConnectLogRecordsPerSP())
#define MAX_CONNECT_LOG_FILTER_ENTRIES        (ccr_getMaxConnectLogFilterEntries())  // Total unique log entries tracked including one for general bucket

#define NAVI_MEMORY_BUDGET_MB                 (ccr_getNaviMemoryBudgetMB())
#define MAX_LUN_PRESENTATIONS                 (ccr_getMaxVLUsPerSP())

#define MAX_DISKS_PER_POOL                    (ccr_getMaxDisksPerPool())
#define MAX_DISKS_USABLE_FOR_ALL_POOLS        (ccr_getMaxDisksUsableForAllPools())
#define MIN_REC_READ_CACHE_SIZE               (ccr_getPlatformMinRecReadCacheSize()) 

//
//  MAX_HOST_LUN_NUMBER has always been twice the number of Flare luns which is enough space to account
//  for Snapshots, FAR shadows, and meta luns. On the HAMMERHEAD_HW because of the large increas in Snapshots
//  this is no longer true so we need to make sure the value is increased to allow all of the devices to be
//  created.
//
//  For HH if we multiply by 3 there is a little room as we set the value to 6143.
//
#define MAX_HOST_LUN_NUMBER                   (ccr_getMaxHostLunNumber())

#define MIN_VVOL_ALU                          (ccr_getMinVVolALU())
#define MAX_VVOL_ALU                          (ccr_getMaxVVolALU())

// This is the most memory we think we might use for write cache on this platform
// This value will change bitmap size, some table sizes, overhead block allocation, etc 
// and may warrant a change to the cache revision.  This # can be slightly beyond
// the real maximum write cache size because available physical memory at runtime 
// will determine what can actually be allocated.
#define MAX_PLATFORM_WC_SIZE                  (ccr_getPlatformMaxWcSize())
#define PLATFORM_FRU_COUNT                    (ccr_getPlatformFruCount())
#define RG_USER_COUNT                         (ccr_getRgUserCount())

#define PLATFORM_MAX_LUNS_PER_RG              (ccr_getPlatformMaxLunsPerRG())
#define PLATFORM_MAX_ENCL_PER_BUS             (ccr_getPlatformMaxEnclPerBus())
#define PLATFORM_MAX_SLICS                    (ccr_getPlatformMaxSlics())
#define PLATFORM_MAX_MEZZANINES               (ccr_getPlatformMaxMezzanines())
#define PLATFORM_MAX_FBE_MEMORY_BUDGET        (ccr_getPlatformMaxFbeMemoryBudgetMB(void))
//Port Limits
#define PLATFORM_MAX_SAS_BE_COUNT             (ccr_getPlatformMaxSasBeCount())
#define PLATFORM_MAX_SAS_FE_COUNT             (ccr_getPlatformMaxSasFeCount())
#define PLATFORM_FC_FE_COUNT                  (ccr_getPlatformMaxFcFeCount())
#define PLATFORM_MAX_ISCSI_1G_FE_COUNT        (ccr_getPlatformMaxiSCSI1GFeCount())
#define PLATFORM_MAX_ISCSI_10G_FE_COUNT       (ccr_getPlatformMaxiSCSI10GFeCount())
#define PLATFORM_MAX_FCOE_FE_COUNT            (ccr_getMPlatformMaxFcoeFeCount())
#define PLATFORM_MAX_COMBINED_ISCSI_FE_COUNT  (ccr_getPlatformMaxCombinediSCSIFeCount())


/*********************  EXPORTED CONFIGURATION INTERFACE FUNCTIONS  ***********************/

/*
 *Interface functions for kernel-space and user-space clients
 *
 * Most clients will use this function to initialize ccr
 *
 * Note This function queries SPID for the the platform type
 *      for use in configuring ccr
 */

 BOOL CCR_API ccrInit(void);

 BOOL CCR_API ccrUnInit(void);


/* 
 * Clients that run OFF-ARRAY need to specify the platform type
 * should call this function instead of ccrInit().
 *
 */
BOOL CCR_API ccrInitCoreConfig(SPID_HW_TYPE PlatformType);

BOOL CCR_API ccrReadRegPlatform(csx_cstring_t pRegPath);
BOOL CCR_API ccrReadRegPlatformSim(csx_cstring_t pRegPath);
BOOL CCR_API ccrReadRegHWAttribiutes(csx_cstring_t pRegPath);
BOOL CCR_API ccrReadRegPortLimits(csx_cstring_t pRegPath);
BOOL CCR_API ccrReadRegPlatformVirt(void);

/***************** OTHER EXPORTED INTERFACE MACROS ***************************/
/* These are not part of CCR library, but formerly defined in core_config.h  */

/* TBD - move rest of core_config.h into this file later. */

#if defined(__cplusplus)
}
#endif

#endif // __CORE_CONFIG_RUNTIME__
