/*******************************************************************************
 * Copyright (C) EMC Corporation, 1998 - 2015
 * All rights reserved.
 * Licensed material - Property of EMC Corporation
 ******************************************************************************/

/*******************************************************************************
 * flare_export_ioctls.h
 *
 * This header file defines constants and structures associated with IOCTLs
 * defined by CLARiiON and exported to user space. Changes to this file must be
 * reviewed by Navi, Admin and ASE.
 *
 ******************************************************************************/

#if !defined (FLARE_EXPORT_IOCTLS_H)
#define FLARE_EXPORT_IOCTLS_H

#ifdef ALAMOSA_WINDOWS_ENV
#include <ntdddisk.h> //Included for DISK_GEOMETRY structure.
#else
#include "safe_win_scsi.h" //Included for DISK_GEOMETRY structure.
#endif /* ALAMOSA_WINDOWS_ENV - STDPORT */

#if !defined (K10_DEFS_H)
#include "k10defs.h"
#endif

# if !defined (VolumeAttributes_H)
#include "VolumeAttributes.h"
# endif

# if !defined (PowerSavingPolicy_H)
#include "PowerSavingPolicy.h"
# endif

#include "EmcPAL_Ioctl.h"

/****************************/
/* CLARiiON defined IOCTLs. */
/****************************/

#define IOCTL_FLARE_CAPABILITIES_QUERY        \
    EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0x888, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

// Legacy name: deprecated
#define IOCTL_FLARE_DCA_QUERY IOCTL_FLARE_CAPABILITIES_QUERY

#define IOCTL_FLARE_READ_BUFFER     \
    EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0x889, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define IOCTL_FLARE_WRITE_FIRMWARE  \
    EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0x88A, EMCPAL_IOCTL_METHOD_NEITHER, EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define IOCTL_FLARE_GET_RAID_INFO   \
    EMCPAL_IOCTL_CTL_CODE (FILE_DEVICE_DISK, 0x88B, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define IOCTL_FLARE_START_SHUTDOWN  \
    EMCPAL_IOCTL_CTL_CODE (FILE_DEVICE_DISK, 0x88E, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define IOCTL_FLARE_WAIT_FOR_SHUTDOWN \
    EMCPAL_IOCTL_CTL_CODE (FILE_DEVICE_DISK, 0x88F, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define IOCTL_FLARE_COMMIT \
    EMCPAL_IOCTL_CTL_CODE (FILE_DEVICE_DISK, 0x890, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

/* IOCTL_FLARE_GET_DISK_GEOMETRY is now replaced with IOCTL_FLARE_GET_VOLUME_STATE */
#define IOCTL_FLARE_GET_VOLUME_STATE \
    EMCPAL_IOCTL_CTL_CODE (FILE_DEVICE_DISK, 0x891, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define IOCTL_FLARE_TRESPASS_EXECUTE \
    EMCPAL_IOCTL_CTL_CODE (FILE_DEVICE_DISK, 0x892, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

/* IOCTL_FLARE_GET_LUN_STATUS is obsoleted */

#define IOCTL_FLARE_MARK_BLOCK_BAD \
    EMCPAL_IOCTL_CTL_CODE (FILE_DEVICE_DISK, 0x894, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

// deprecated; do not use. Only for layered driver source code compatibility with hammer
#define IOCTL_FLARE_TRESPASS_QUERY \
    EMCPAL_IOCTL_CTL_CODE (FILE_DEVICE_DISK, 0x895, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define IOCTL_FLARE_TRESPASS_OWNERSHIP_LOSS \
    EMCPAL_IOCTL_CTL_CODE (FILE_DEVICE_DISK, 0x896, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define IOCTL_FLARE_REGISTER_FOR_CHANGE_NOTIFICATION \
    EMCPAL_IOCTL_CTL_CODE (FILE_DEVICE_DISK, 0x897, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

// Legacy name
#define IOCTL_FLARE_REGISTER_FOR_CAPACITY_CHANGE_NOTIFICATION \
    IOCTL_FLARE_REGISTER_FOR_CHANGE_NOTIFICATION

#define IOCTL_FLARE_MODIFY_CAPACITY \
    EMCPAL_IOCTL_CTL_CODE (FILE_DEVICE_DISK, 0x898, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define IOCTL_FLARE_ZERO_FILL \
    EMCPAL_IOCTL_CTL_CODE (FILE_DEVICE_DISK, 0x89C, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define IOCTL_FLARE_VOLUME_MAY_HAVE_FAILED \
    EMCPAL_IOCTL_CTL_CODE (FILE_DEVICE_DISK, 0x89D, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define IOCTL_BVD_ENUM_VOLUMES \
    EMCPAL_IOCTL_CTL_CODE (FILE_DEVICE_DISK, 0x8D0, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define IOCTL_BVD_CREATE_VOLUME \
    EMCPAL_IOCTL_CTL_CODE (FILE_DEVICE_DISK, 0x8D1, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define IOCTL_BVD_DESTROY_VOLUME \
    EMCPAL_IOCTL_CTL_CODE (FILE_DEVICE_DISK, 0x8D2, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define IOCTL_BVD_SET_VOLUME_PARAMS \
    EMCPAL_IOCTL_CTL_CODE (FILE_DEVICE_DISK, 0x8D3, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define IOCTL_BVD_GET_VOLUME_PARAMS \
    EMCPAL_IOCTL_CTL_CODE (FILE_DEVICE_DISK, 0x8D4, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define IOCTL_BVD_QUERY_VOLUME_STATUS \
    EMCPAL_IOCTL_CTL_CODE (FILE_DEVICE_DISK, 0x8D5, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define IOCTL_BVD_GET_VOLUME_DRIVER_PARAMS \
    EMCPAL_IOCTL_CTL_CODE (FILE_DEVICE_DISK, 0x8D6, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define IOCTL_BVD_SET_VOLUME_DRIVER_PARAMS \
    EMCPAL_IOCTL_CTL_CODE (FILE_DEVICE_DISK, 0x8D7, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define IOCTL_BVD_QUERY_VOLUME_DRIVER_STATUS \
    EMCPAL_IOCTL_CTL_CODE (FILE_DEVICE_DISK, 0x8D8, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define IOCTL_BVD_COMMIT \
    EMCPAL_IOCTL_CTL_CODE (FILE_DEVICE_DISK, 0x8D9, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define IOCTL_BVD_GET_VOLUME_DRIVER_STATISTICS \
    EMCPAL_IOCTL_CTL_CODE (FILE_DEVICE_DISK, 0x8E0, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define IOCTL_BVD_GET_VOLUME_STATISTICS \
    EMCPAL_IOCTL_CTL_CODE (FILE_DEVICE_DISK, 0x8E1, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define IOCTL_BVD_LOAD_DATABASE \
    EMCPAL_IOCTL_CTL_CODE (FILE_DEVICE_DISK, 0x8E2, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define IOCTL_BVD_PERFORM_DRIVER_ACTION \
    EMCPAL_IOCTL_CTL_CODE (FILE_DEVICE_DISK, 0x8E3, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define IOCTL_BVD_PERFORM_VOLUME_ACTION \
    EMCPAL_IOCTL_CTL_CODE (FILE_DEVICE_DISK, 0x8E4, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define IOCTL_BVD_CHANGE_LUN_WWN \
    EMCPAL_IOCTL_CTL_CODE (FILE_DEVICE_DISK, 0x8E5, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define IOCTL_BVD_SHUTDOWN_WARNING \
    EMCPAL_IOCTL_CTL_CODE (FILE_DEVICE_DISK, 0x8E6, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define IOCTL_BVD_SHUTDOWN \
    EMCPAL_IOCTL_CTL_CODE (FILE_DEVICE_DISK, 0x8E7, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define IOCTL_AIDVD_INSERT_DISK_VOLUME  \
    EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0x8F0, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define IOCTL_AIDVD_DESTROY_DISK_VOLUME \
    EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0x8F1, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define IOCTL_AIDVD_GET_DRIVER_PARAMS   \
    EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0x8F2, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

#ifdef ALAMOSA_WINDOWS_ENV
#define IOCTL_FLARE_GET_ALL_RAID_INFO \
    EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0x8f3, EMCPAL_IOCTL_METHOD_IN_DIRECT, EMCPAL_IOCTL_FILE_ANY_ACCESS)
#else
#define IOCTL_FLARE_GET_ALL_RAID_INFO \
    EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0x8f3, EMCPAL_IOCTL_METHOD_OUT_DIRECT, EMCPAL_IOCTL_FILE_ANY_ACCESS)
#endif /* ALAMOSA_WINDOWS_ENV - ODDCASE - this should be OUT_DIRECT.   IN_DIRECT treats the output buffer as another input buffer.  here we're using the output buffer for output */

#define IOCTL_FLARE_SET_POWER_SAVING_POLICY \
    EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0x8f4, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define IOCTL_FLARE_OFD_CANCEL \
    EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0x8f5, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)


#define IOCTL_FLARE_DESCRIBE_EXTENTS \
    EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0x8f6, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define IOCTL_FLARE_ODFU_WRITE_BUFFER    \
    EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0x8f7, EMCPAL_IOCTL_METHOD_NEITHER, EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define IOCTL_FLARE_COMPARE_AND_SWAP \
    EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0x8f8, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define IOCTL_FLARE_REGISTER_FOR_SP_CHANGE_NOTIFICATION \
    EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0x8f9, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define IOCTL_FLARE_WRITE_BUFFER    \
    EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0x8fa, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define IOCTL_FLARE_GET_SP_ENVIRONMENT_STATE    \
    EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0x8fb, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define IOCTL_FLARE_QUIESCE_BACKGROUND_IO       \
    EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0x8fc, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define IOCTL_FLARE_OK_TO_POWER_DOWN      \
    EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0x8fd, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

// It is used to allocate control memory from unused persistent memory.
#define IOCTL_CACHE_ALLOCATE_PSEUDO_CONTROL_MEMORY      \
    EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0x8fe, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

// It is used to free the pseudo control memory which has been previously
// allocated.
#define IOCTL_CACHE_FREE_PSEUDO_CONTROL_MEMORY      \
    EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0x8ff, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

// It will be received by cache when booting is completed successfully.
#define IOCTL_CACHE_BOOT_COMPLETE       \
    EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0x900, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define IOCTL_BVD_CLEAR_VOLUME_STATISTICS      \
    EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0x901, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

// It will be used to create volumes using BVD volume factory. Input parameter would be
// FilterDriverVolumeParams & output parameter would be IOCTL_AIDVD_INSERT_DISK_VOLUME_PARAMS.
#define IOCTL_AIDVD_INSERT_DISK_VOLUME_WITH_PARAMS  \
    EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0x902, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

//  SP Cache queries the LU Cache config info. through this IOCTL from flare.
#define IOCTL_FLARE_GET_VOLUME_CACHE_CONFIG \
    EMCPAL_IOCTL_CTL_CODE (FILE_DEVICE_DISK, 0x903, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

// Used to cause Cache to read and hold data abd return SGL info for requested length
// This operation is below the MCC zero-fill bitmap level and MUST not be used above that level.
#define IOCTL_CACHE_READ_AND_PIN_DATA      \
    EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0x904, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

// Used to release previously pinned data.
#define IOCTL_CACHE_UNPIN_DATA       \
    EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0x905, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

// Used to send client(s) the index to send in a Read and Pin request.  Sent to the specific volume device
// rather than the factory
#define IOCTL_CACHE_SEND_READ_AND_PIN_INDEX       \
    EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0x906, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

//  Sent to MLU by hostside to resolve soft threshold unit attentions across SPs
#define IOCTL_FLARE_TEST_AND_CLEAR_VOLUME_ATTRIBUTES \
    EMCPAL_IOCTL_CTL_CODE (FILE_DEVICE_DISK, 0x907, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

//  Sent by auto-inserters (e.g. Hostside) to notify the auto-insert drivers
//  that initialization is complete.
#define IOCTL_AIDVD_INITIALIZATION_COMPLETE \
    EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0x908, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

// Sent by MCCCLI to get MCC VolumeGroups statistics
#define IOCTL_CACHE_VOLUME_GROUPS_STATISTICS \
    EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0x909, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

#define IOCTL_CACHE_DISPARATE_WRITE       \
    EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0x90A, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

//  Sent by MCR CBE before zero-encrypt vault LU: output status CacheVaultImageNotInUse or CacheVaultImageBusy
#define IOCTL_CACHE_VAULT_IMAGE \
    EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0x90B, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)

//Sent by MR and DVL(upper deck) to MLU to acknowledge it has received an SCN
#define IOCTL_FLARE_ACKNOWLEDGE_STATE_CHANGE_NOTIFICATION \
    EMCPAL_IOCTL_CTL_CODE(FILE_DEVICE_DISK, 0x90C, EMCPAL_IOCTL_METHOD_BUFFERED, EMCPAL_IOCTL_FILE_ANY_ACCESS)


// Returns the name of the FLARE IOCTL for a IRP_MJ_DEVICE_CONTROL.
// If not recognized, returns empty string (not NULL pointer).
static __inline char * FlareIoctlCodeToString(ULONG IoControlCode)
{
    char * name;

    switch (IoControlCode)
    {
    case IOCTL_DISK_GET_DRIVE_GEOMETRY: name = "IOCTL_DISK_GET_DRIVE_GEOMETRY"; break;
    case IOCTL_DISK_IS_WRITABLE: name = "IOCTL_DISK_IS_WRITABLE"; break;
    case IOCTL_DISK_VERIFY:  name = "IOCTL_DISK_VERIFY"; break;
    case IOCTL_STORAGE_EJECT_MEDIA:  name = "IOCTL_STORAGE_EJECT_MEDIA"; break;
    case IOCTL_STORAGE_LOAD_MEDIA:  name = "IOCTL_STORAGE_LOAD_MEDIA"; break;

    case IOCTL_FLARE_WRITE_BUFFER: name = "IOCTL_FLARE_WRITE_BUFFER"; break;
    case IOCTL_FLARE_ODFU_WRITE_BUFFER: name = "IOCTL_FLARE_ODFU_WRITE_BUFFER"; break;
    case IOCTL_FLARE_GET_RAID_INFO: name = "IOCTL_FLARE_GET_RAID_INFO"; break;
    case IOCTL_FLARE_START_SHUTDOWN: name = "IOCTL_FLARE_START_SHUTDOWN"; break;
    case IOCTL_FLARE_WAIT_FOR_SHUTDOWN: name = "IOCTL_FLARE_WAIT_FOR_SHUTDOWN"; break;
    case IOCTL_FLARE_COMMIT: name = "IOCTL_FLARE_COMMIT"; break;
    case IOCTL_FLARE_GET_VOLUME_STATE: name = "IOCTL_FLARE_GET_VOLUME_STATE"; break;
    case IOCTL_FLARE_MARK_BLOCK_BAD: name = "IOCTL_FLARE_MARK_BLOCK_BAD"; break;
    case IOCTL_FLARE_TRESPASS_OWNERSHIP_LOSS: name = "IOCTL_FLARE_TRESPASS_OWNERSHIP_LOSS"; break;
    case IOCTL_FLARE_REGISTER_FOR_CHANGE_NOTIFICATION: name = "IOCTL_FLARE_REGISTER_FOR_CHANGE_NOTIFICATION"; break;
    case IOCTL_FLARE_MODIFY_CAPACITY: name = "IOCTL_FLARE_MODIFY_CAPACITY"; break;
    case IOCTL_FLARE_ZERO_FILL: name = "IOCTL_FLARE_ZERO_FILL"; break;
    case IOCTL_FLARE_VOLUME_MAY_HAVE_FAILED: name = "IOCTL_FLARE_VOLUME_MAY_HAVE_FAILED"; break;
    case IOCTL_FLARE_GET_ALL_RAID_INFO: name = "IOCTL_FLARE_GET_ALL_RAID_INFO"; break;

    case IOCTL_BVD_ENUM_VOLUMES: name = "IOCTL_BVD_ENUM_VOLUMES"; break;
    case IOCTL_BVD_CREATE_VOLUME: name = "IOCTL_BVD_CREATE_VOLUME"; break;
    case IOCTL_BVD_DESTROY_VOLUME: name = "IOCTL_BVD_DESTROY_VOLUME"; break;
    case IOCTL_BVD_SET_VOLUME_PARAMS: name = "IOCTL_BVD_SET_VOLUME_PARAMS"; break;
    case IOCTL_BVD_GET_VOLUME_PARAMS: name = "IOCTL_BVD_GET_VOLUME_PARAMS"; break;
    case IOCTL_BVD_QUERY_VOLUME_STATUS: name = "IOCTL_BVD_QUERY_VOLUME_STATUS"; break;
    case IOCTL_BVD_GET_VOLUME_DRIVER_PARAMS: name = "IOCTL_BVD_GET_VOLUME_DRIVER_PARAMS"; break;
    case IOCTL_BVD_SET_VOLUME_DRIVER_PARAMS: name = "IOCTL_BVD_SET_VOLUME_DRIVER_PARAMS"; break;
    case IOCTL_BVD_QUERY_VOLUME_DRIVER_STATUS: name = "IOCTL_BVD_QUERY_VOLUME_DRIVER_STATUS"; break;
    case IOCTL_BVD_COMMIT: name = "IOCTL_BVD_COMMIT"; break;
    case IOCTL_BVD_GET_VOLUME_DRIVER_STATISTICS: name = "IOCTL_BVD_GET_VOLUME_DRIVER_STATISTICS"; break;
    case IOCTL_BVD_GET_VOLUME_STATISTICS: name = "IOCTL_BVD_GET_VOLUME_STATISTICS"; break;
    case IOCTL_BVD_LOAD_DATABASE: name = "IOCTL_BVD_LOAD_DATABASE"; break;
    case IOCTL_BVD_PERFORM_DRIVER_ACTION: name = "IOCTL_BVD_PERFORM_DRIVER_ACTION"; break;
    case IOCTL_BVD_PERFORM_VOLUME_ACTION: name = "IOCTL_BVD_PERFORM_VOLUME_ACTION"; break;
    case IOCTL_BVD_CHANGE_LUN_WWN: name = "IOCTL_BVD_CHANGE_LUN_WWN"; break;
    case IOCTL_BVD_SHUTDOWN_WARNING: name = "IOCTL_BVD_SHUTDOWN_WARNING"; break;
    case IOCTL_BVD_SHUTDOWN: name = "IOCTL_BVD_SHUTDOWN"; break;

    case IOCTL_FLARE_TRESPASS_EXECUTE: name = "IOCTL_FLARE_TRESPASS_EXECUTE"; break;
    case IOCTL_FLARE_CAPABILITIES_QUERY: name = "IOCTL_FLARE_CAPABILITIES_QUERY"; break;
    case IOCTL_FLARE_SET_POWER_SAVING_POLICY: name = "IOCTL_FLARE_SET_POWER_SAVING_POLICY"; break;
    case IOCTL_FLARE_OFD_CANCEL: name = "IOCTL_FLARE_OFD_CANCEL"; break;

    case IOCTL_FLARE_COMPARE_AND_SWAP: name = "IOCTL_FLARE_COMPARE_AND_SWAP"; break;

    case IOCTL_AIDVD_INSERT_DISK_VOLUME: name = "IOCTL_AIDVD_INSERT_DISK_VOLUME"; break;
    case IOCTL_AIDVD_INSERT_DISK_VOLUME_WITH_PARAMS: name = "IOCTL_AIDVD_INSERT_DISK_VOLUME_WITH_PARAMS"; break;
    case IOCTL_AIDVD_DESTROY_DISK_VOLUME: name = "IOCTL_AIDVD_DESTROY_DISK_VOLUME"; break;
    case IOCTL_AIDVD_GET_DRIVER_PARAMS: name = "IOCTL_AIDVD_GET_DRIVER_PARAMS"; break;
    case IOCTL_AIDVD_INITIALIZATION_COMPLETE: name = "IOCTL_AIDVD_INITIALIZATION_COMPLETE"; break;
    case IOCTL_FLARE_DESCRIBE_EXTENTS: name = "IOCTL_FLARE_DESCRIBE_EXTENTS"; break;
    case IOCTL_FLARE_REGISTER_FOR_SP_CHANGE_NOTIFICATION: name = "IOCTL_FLARE_REGISTER_FOR_SP_CHANGE_NOTIFICATION"; break;
    case IOCTL_FLARE_GET_SP_ENVIRONMENT_STATE: name = "IOCTL_FLARE_GET_SP_ENVIRONMENT_STATE"; break;
    case IOCTL_FLARE_QUIESCE_BACKGROUND_IO: name = "IOCTL_FLARE_QUIESCE_BACKGROUND_IO"; break;
    case IOCTL_FLARE_OK_TO_POWER_DOWN: name = "IOCTL_FLARE_OK_TO_POWER_DOWN"; break;
    case IOCTL_CACHE_ALLOCATE_PSEUDO_CONTROL_MEMORY: name = "IOCTL_CACHE_ALLOCATE_PSEUDO_CONTROL_MEMORY"; break;
    case IOCTL_CACHE_FREE_PSEUDO_CONTROL_MEMORY: name = "IOCTL_CACHE_FREE_PSEUDO_CONTROL_MEMORY"; break;
    case IOCTL_CACHE_BOOT_COMPLETE: name = "IOCTL_CACHE_BOOT_COMPLETE"; break;
    case IOCTL_FLARE_GET_VOLUME_CACHE_CONFIG: name = "IOCTL_FLARE_GET_VOLUME_CACHE_CONFIG"; break;
    case IOCTL_FLARE_TEST_AND_CLEAR_VOLUME_ATTRIBUTES: name = "IOCTL_FLARE_TEST_AND_CLEAR_VOLUME_ATTRIBUTES"; break;

    case IOCTL_CACHE_READ_AND_PIN_DATA: name = "IOCTL_CACHE_READ_AND_PIN_DATA"; break;
    case IOCTL_CACHE_UNPIN_DATA: name = "IOCTL_CACHE_UNPIN_DATA"; break;
    case IOCTL_CACHE_SEND_READ_AND_PIN_INDEX: name = "IOCTL_CACHE_SEND_READ_AND_PIN_INDEX"; break;
    case IOCTL_CACHE_VAULT_IMAGE: name = "IOCTL_CACHE_VAULT_IMAGE"; break;
    case IOCTL_CACHE_VOLUME_GROUPS_STATISTICS: name = "IOCTL_CACHE_VOLUME_GROUPS_STATISTICS"; break;
    case IOCTL_CACHE_DISPARATE_WRITE: name = "IOCTL_CACHE_DISPARATE_WRITE"; break;
    case IOCTL_FLARE_ACKNOWLEDGE_STATE_CHANGE_NOTIFICATION: name = "IOCTL_FLARE_ACKNOWLEDGE_STATE_CHANGE_NOTIFICATION"; break;
        // Return empty string, not NULL pointer, to indicate "not found".
    default: name = ""; break;
    }; /* end switch () */

    return name;
}

/*
 * The input structure for SP-directed IOCTLs.
 */

typedef struct _FLARE_SP_IOCTL_INPUT
{
    ULONG   Lun;
} FLARE_SP_IOCTL_INPUT, *PFLARE_SP_IOCTL_INPUT;


/********************************/
/* Flare RAID Info definitions. */
/********************************/

typedef struct _FLARE_RAID_INFO
{
    UCHAR           RaidType;           // See definitions below.
    UCHAR           FlareMajorRevision; // Major revision of Flare component.
    UCHAR           FlareMinorRevision; // Minor revision of Flare component.
    UCHAR           LunCharacteristics; // e.x. Thin, Replica etc
    UCHAR           NotificationThresholdExponent;  //VPD 0xB2
    UCHAR           Reserved[3];
    USHORT          RotationalRate;     // 0x01 for SSD bound lun
    USHORT          OptimalTransferLengthGranularity;   //VPD 0xB0 Page.
    ULONG           OptimalUnmapGranularity;    //VPD 0xB0 Page.
    ULONG           MaxUnmapLbaCount;             // VPD 0xB0 Page.
    ULONG           UnmapGranularityAlignment;   //VPD 0xB0 Page.
    ULONG           BytesPerSector;     // Bytes per logical sector.
    ULONG           DisksPerStripe;     // Number of disks in lun (includes parity).
    ULONG           SectorsPerStripe;   // Does not include parity element(s).
    ULONG           BytesPerElement;    // Element size for this lun.
    ULONG           StripeCount;        // Total stripe count for this lun.
    ULONG           ElementsPerParityStripe; // Parity Element Size
    ULONG           GroupId;            // RAID Group id.
    K10_WWID        WorldWideName;      // World-Wide Name
    K10_LOGICAL_ID  UserDefinedName;    // User-Defined Name
    ULONG           BindTime;           // Bind time of the LU
    ULONG           Lun;                // User Logical unit number
    ULONG           Flun;               // Flare Logical unit number
    USHORT          MaxBlockDescriptorsForTokenBasedCopy;   // For VPD 0x8F page
    ULONGLONG       MaxSectorCountForTokenBasedCopy;        // For VPD 0x8F page
    ULONGLONG       OptimalSectorCountForTokenBasedCopy;    // For VPD 0x8F page
    ULONG           MaxSecondsOfInactivityForCopyToken;     // For VPD 0x8F page
    ULONG           DefaultSecondsOfInactivityForCopyToken; // For VPD 0x8F page
    ULONG           MaxQueueDepth;      // Max number of commands allowed to be sent.
} FLARE_RAID_INFO, *PFLARE_RAID_INFO;

/*
 * Valid values for the RaidType field of the FLARE_RAID_INFO structure...
 */

#define FLARE_RAID_TYPE_NONE        0xFF  // hot spares, unbound disks, etc.
#define FLARE_RAID_TYPE_INDIVID     0x02  // striped disk
#define FLARE_RAID_TYPE_RAID0       0x04  // striped array
#define FLARE_RAID_TYPE_RAID1       0x03  // mirrored striped disk
#define FLARE_RAID_TYPE_RAID10      0x07  // mirrored striped array
#define FLARE_RAID_TYPE_RAID3       0x05  // dedicated-parity parallel-access
#define FLARE_RAID_TYPE_RAID5       0x01  // rotating-parity individual-access
#define FLARE_RAID_TYPE_RAID6       0x0A  // super-redundant with 2 parities
#define FLARE_RAID_TYPE_VRAID       0x0B  // virtualized raid-type
#define FLARE_RAID_TYPE_MIXED       0x0C  // virtualized mixed raid-type


/*
 * Bits for the bit map 'LunCharacteristics' of the FLARE_RAID_INFO structure...
 * Note that sequential Bits 01 -> XX are displayed to host via Inquiry byte-12
 * and hence are 'immovable'.  Host Displayed Characteristics must be added to the
 * below mask as well as the Clariion Interface spec.
 */

#define FLARE_LUN_CHARACTERISTIC_THIN       0x01
#define FLARE_LUN_CHARACTERISTIC_META       0x02
#define FLARE_LUN_CHARACTERISTIC_SNAPSHOT   0x04

#define FLARE_LUN_CHARACTERISTIC_CELERRA    0x40
#define FLARE_LUN_CHARACTERISTIC_POOL       0x80  // not exposed to host

// Mask of Characteristics reported in Byte-112 of Inquiry
// Any value added to this mask MUST be added to the CLARiiON Interface Spec for visibility outside the array
#define FLARE_LUN_CHARACTERISTIC_INQUIRY_MASK \
    (FLARE_LUN_CHARACTERISTIC_THIN | \
     FLARE_LUN_CHARACTERISTIC_META | \
     FLARE_LUN_CHARACTERISTIC_SNAPSHOT)

/*******************************************/
/* Read/Write Buffer specific definitions. */
/*******************************************/

/*
 * The RW_BUFFER_INFO structure describes a Read or Write Buffer operation.
 * This structure may only be written by the orginator.
 */

// CSX_ALIGN_N(8) added to prevent alignment failures when passing ICOTLs
// between 32 bit user apps and 64 bit kernel drivers.
typedef struct _RW_BUFFER_INFO
{

   ULONG Reserved1;         // Reserved, unused.
   ULONG BufferOffset;      // Bytes 3,4,5 of SCSI CDB.
   ULONG BufferID;          // Byte 2 of SCSI CDB.
   ULONG Length;            // Size of data buffer
   PVOID CSX_ALIGN_N(8) DataBuffer;      // Pointer to (TDD supplied) data buffer. Set to 0 if contiguous
   UCHAR CSX_ALIGN_N(8) Dummy; // Added for 64-bit "thunking" purposes
} RW_BUFFER_INFO, *PRW_BUFFER_INFO;


/*
 * VALID BufferID's for the RW_BUFFER_INFO structure...
 *
 *  Buffer IDs 0..255 are direct copies of SCSI Read/Write buffer requests.
 *  Buffer IDs >= 256 are reserved for driver to driver communication.
 *
 *  A driver servicing a BufferID < 256 should assume that the request came
 *  from an external device, and institute appropriate security.
 *
 * Allocating BufferIDs:
 *
 *  Use odd BufferIDs for requests which should pass through most layered
 *  drivers, and even BufferIDs for requests which should be either processed
 *  or rejected by each driver.   Requests containing LBAs or requests which
 *  change the data on an LU must have even BufferIDs.  SnapshotCopy uses even
 *  buffer IDs, since its requests contain LBAs.
 *
 * If a new buffer ID is required, and the value is > 255, then append a new
 * value to the list in this header file.
 *
 * If a new buffer ID is required with a value of less than 256, the BufferID
 * allocation must be approved prior to being added to this header file.
 *
 * BufferIDs should be feature centric, not driver centric, though many
 * features are implemented by one driver.    For example, a command to start a
 * Snapshot Copy is one which should be serviced by the first driver that
 * understands it.  If there was a RAID 0 driver above SnapshotCopy, the RAID0
 * driver might "split" the start snapshot command, and send a new command to
 * each element of the raid group.
 */

#define RW_BUFFER_ID_SNAPSHOT            8
#define RW_BUFFER_ID_INVALIDATE         10
#define RW_BUFFER_ID_FIRMWARE_UPDATE    12
#define RW_BUFFER_ID_BIOS_UPDATE        14
#define RW_BUFFER_ID_POST_UPDATE        16
#define RW_BUFFER_ID_DDBS_UPDATE        18
#define RW_BUFFER_ID_GPS_FW_UPDATE      20
#define RW_BUFFER_ID_ADVANCED_SNAP      22

#define RW_BUFFER_ID_RECOVER_POINT    0x80
#define RW_BUFFER_ID_RECOVER_POINT_CMD_CHANNEL      0x81

/* NOTE: All VolumeAttributes defines have been moved to VolumeAttributes.h file */
// 0 - trespass is not disabled, !=0 trespass is disabled.
// NOTE: this type is passed between SPs, so its defintion affects NDU.
typedef ULONG TRESPASS_DISABLE_REASONS;
# define TRESPASS_DISABLE_BY_MV_A               0x01  // MV/A wants ownership not to move from the owner.
# define TRESPASS_DISABLE_BY_MV_S               0x02  // MV/S wants ownership not to move from the owner.
# define TRESPASS_DISABLE_BY_CLONES             0x04  // Clones wants ownership not to move from the owner.
# define TRESPASS_DISABLE_RESERVATION           0x08  // Don't trespass if SCSI2 Reservation on owner.
# define TRESPASS_DISABLE_ALREADY_OWNED         0x10  // Don't trespass if lun is already in owned state.

// Trespass disable mask for all trespass disable reasons
#define TRESPASS_DISABLE_IGNORE_ALL_REASONS (TRESPASS_DISABLE_ALREADY_OWNED| \
                                             TRESPASS_DISABLE_BY_MV_A| \
                                             TRESPASS_DISABLE_BY_MV_S| \
                                             TRESPASS_DISABLE_BY_CLONES| \
                                             TRESPASS_DISABLE_RESERVATION)

// Informs IoRedirector the reason for issuing SET_OWNER. IoRedirector will use this value to set the lastTrespassType.
// Currently only SET_OWNERSHIP_DUE_TO_SHUTDOWN is used by IoRedirector, the rest of the #define are place holder.

# define SET_OWNERSHIP_DEFAULT						0x00
# define SET_OWNERSHIP_DUE_TO_SHUTDOWN				0x01
# define SET_OWNERSHIP_DUE_TO_TRESPASS				0x02
# define SET_OWNERSHIP_DUE_TO_ASSIGN				0x03

/*
 * Interpretation of the BufferOffset field within the RW_BUFFER_INFO structure
 * is a function of the BufferID.  If the BufferID is < 256, then the value of
 * BufferOffset is (cdb.byte3 <<16 | cdb.byte4 <<8 | cdb.byte5).  Otherwise
 * the meaning of the BufferOffset field should be defined in a header file
 * called "esd\disk\inc\BufferId<name>.h", where name is the name of a driver,
 * e.g., "BufferIdSnapshot.h".  That header file is readable by other components
 * which can utilize it to generate requests, or to determine which requests to
 * pass through.
 *
 * The DataBuffer field of the RW_BUFFER_INFO structure points to the data
 * buffer, which is allocated from the non-paged pool.  The physical memory
 * behind this virtual address need not be physically contiguous.
 *
 * The Length field of the RW_BUFFER_INFO structure specifies the number of
 * bytes in the data buffer.
 */

/* FLARE_VOLUME_STATE structure */
// NOTE: this type is passed between SPs, so its defintion affects NDU.
typedef struct _FLARE_VOLUME_STATE
{
    DISK_GEOMETRY   geometry;

    // The size of the volume in sectors.
    ULONGLONG       capacity;

    // 0 if the owner has no reason to restrict trepasses.
    // !0 indicates why the owner wants to restrict trespass.
    TRESPASS_DISABLE_REASONS         TrespassDisableReasons;

    // Information about state of this volume, with each
    // bit indicating a boolean state.  The parity of
    // these bits is such that volumes composed of multiple
    // sub-volumes  (aggregation) can blindly "or" these
    // bits together.
    VOLUME_ATTRIBUTE_BITS  VolumeAttributes;

    // FALSE if the device is Ready
    // TRUE if any layer considers the device not ready.
    BOOLEAN         NotReady;

    // Spare fields that are required to be initialized to zero by the creator
    // of this structure.  Since this structure is carried between SPs,
    // changing the contents of this structure is difficult due to NDU, but zeroed
    // space fields may make certain enhancements easier.
    UCHAR           SpareByte0;
    UCHAR           SpareByte1;
    UCHAR           SpareByte2;
    ULONG           SpareULong;

     //The three fields below represent the worst case Wear information of a drive in the FLU
    ULONGLONG       MaxPECycle;       
    ULONGLONG       CurrentPECycle;
    ULONGLONG       TotalPowerOnHours;
} FLARE_VOLUME_STATE, *PFLARE_VOLUME_STATE;

/*
 * DIMS 69506 - Added this structure to define the contents of the
 * FLARE_TRESPASS_EXECUTE: Input Buffer.  This DIMS incident required
 * a new field to be added to the trespass execute IOCTL to indicate
 * if 602-CRU_ENABLED and 606-HOST_UNIT_SHUTDOWN_FOR_TRESS messages
 * will be suppressed for this trespass.
 */
// The stateChangeNotify allows lower level drivers to signal that
// either ready state or volume size may have changed after the trespass
// execute was received.   The upper driver would typically issue a
// IOCTL_FLARE_GET_DRIVE_GEOMETRY to determine the current size/ready state.
//

// CSX_ALIGN_N(8) added to prevent alignment failures when passing ICOTLs
// between 32 bit user apps and 64 bit kernel drivers.
typedef struct _FLARE_TRESPASS_EXECUTE
{
    BOOLEAN   suppressTrespassLogging;          //Flag to suppressed 602/606 messages.
    VOID      CSX_ALIGN_N(8) (*stateChangeNotify)(PVOID context);  // NULL => No notification on change
    PVOID     CSX_ALIGN_N(8) context;        // Parameter to stateChangeNotify.
    BOOLEAN   CSX_ALIGN_N(8) autoTrespass;   // TRUE - caused by Auto trespass
                                                // FALSE - cause by assign, auto assign, or manual trespass
} FLARE_TRESPASS_EXECUTE, *PFLARE_TRESPASS_EXECUTE;


//
// Name:  enum FlareLunStatus
//
// Description:  Tracks status of a Flare Lun
//
typedef enum FlareLunStatus {
    //********************************************************
    //*** Be VERY CAREFUL with changing this enumeration.  ***
    //********************************************************
    // Callers use the ORDER OF THIS ENUMERATION
    // to determine severity of error.
    // The idea is that lower numbers translate into
    // more serious errors.
    // For example Broken is more severe than Degraded,
    // and Degraded is more severe than Enabled etc.
    //********************************************************

    FlareLuStatusUnknown = 0,

    LuStatusUnbound   = 1,
    LuStatusUnBinding = 2,
    LuStatusBinding   = 3,

    LuStatusBroken    = 4,
    LuStatusDegraded  = 5,
    LuStatusEnabled   = 6,

    // Max status

    FlareLuStatusMax = 7
}
FlareLunStatus;

//
// Name: FlareLunStatusStrings
//
// Description:  Defines the max length of strings
//               which descibe the FlareLunStatus enumeration
//
#define FlareLunStatusStringMaxLen 20

//
// Name: FlareLunStatusStrings
//
// Description:  Strings for the enumeration FlareLunStatus
//               The length of these strings is limited to the above
//               GldOwnershipStateStringMaxLen
//
//
#define FlareLunStatusStrings\
    "Unknown",\
    "Unbound",\
    "UnBinding",\
    "Binding",\
    "Broken",\
    "Degraded",\
    "Enabled",\
    "MaxState"


typedef struct ntfeLuCorrputCrcInfo
{
    ULONGLONG       startingLBA;            // In blocks
    ULONG           blockCount;             // In blocks
} NTFE_LU_CORRUPT_CRC_INFO, *PNTFE_LU_CORRUPT_CRC_INFO;

typedef struct invalidate_data_struct_rev1
{
    ULONG       revision;           // Revision number
    ULONGLONG   startingLBA;        // First logical block address to invalidate
    ULONG       numberOfBlocks;     // Number of blocks to invalidate
} INVALIDATE_DATA_STRUCT_REV1, *PINVALIDATE_DATA_STRUCT_REV1;

#define INVALIDATE_DATA_REV1            0x19990512

#define INVALIDATE_DATA_REVISION        INVALIDATE_DATA_REV1

/* Adding packing of 1 byte, as two fields of the structure needs
 * to be contiguous
 */
#pragma pack(1)
// The write buffer data comes in this form.
typedef struct rwbufferinvalidatedata
{
   RW_BUFFER_INFO    header;

   INVALIDATE_DATA_STRUCT_REV1  invalidate;
} RW_BUFFER_INVALIDATE_DATA, * PRW_BUFFER_INVALIDATE_DATA;
#pragma pack()


// deprecated; do not use
typedef struct FLARE_CAPACITY_CHANGE_NOTIFICATION_INFO
{
    ULONGLONG       capacity;
    ULONG           flags;
} FLARE_CAPACITY_CHANGE_NOTIFICATION_INFO, *PFLARE_CAPACITY_CHANGE_NOTIFICATION_INFO;

/* Determines if space needs to be zero filled.
 */
#define FLARE_CCN_ZERO_FILL_FLAG 0x01

//
// Name: FLARE_MODIFY_CAPACITY_INFO
//
// Description:  This is the structure, which is passed in for modify capacity ioctl.
//
//
typedef struct FLARE_MODIFY_CAPACITY_INFO
{
    ULONGLONG       capacity;
    ULONG           flags;
} FLARE_MODIFY_CAPACITY_INFO, *PFLARE_MODIFY_CAPACITY_INFO;

/* Determines if space needs to be zero filled.
 */
#define FLARE_MODIFY_CAPACITY_ZERO_FILL_FLAG 0x01

//
// Name: FLARE_ZERO_FILL_INFO
//
// Description:  This is the structure, which is passed in for the IOCTL_FLARE_ZERO_FILL.
//
//
typedef struct FLARE_ZERO_FILL_INFO
{
    // Input: Lba to start zeroing and
    //        Number of blocks to zero
    ULONGLONG StartLba;
    ULONGLONG Blocks;

    // Output: Number of blocks zeroed
    ULONGLONG BlocksZeroed;
} FLARE_ZERO_FILL_INFO, *PFLARE_ZERO_FILL_INFO;


//
//  If a request to zero more than "FLARE_ZERO_FILL
//  _MIN_BLOCKS is received by a driver, then it should
//  at least zero FLARE_ZERO_FILL_MIN_BLOCKS before it
//  can return STATUS_SUCCESS. If the servicing driver
//  cannot zero FLARE_ZERO_FILL_MIN_BLOCKS when the request
//  is larger than FLARE_ZERO_FILL_MIN_BLOCKS, then the
//  request will be failed by the servicing driver.
//
//  This is currently defined to be 512 blocks (256KB)
//
#define FLARE_ZERO_FILL_MIN_BLOCKS 512




// An operation on the AIDVD's factory to create and link in a disk device.
// The structure is input/output.
typedef struct _IOCTL_AIDVD_INSERT_DISK_VOLUME_PARAMS
{
    //
    // If non-NULL, this will be used as the lower device name,
    // otherwise one will be created and returned here.
    CHAR                LowerDeviceName[K10_DEVICE_ID_LEN];


    // These are a copy of the information in the DLU.
    K10_WWID            LunWwn;
    K10_WWID            GangId;

    // If non-NULL, this will be used as the name of the device created,
    // otherwise a name will be created and returned here.
    CHAR                UpperDeviceName[K10_DEVICE_ID_LEN];

    // If non-NULL, this will be used to find the name of the device created
    CHAR                DeviceLinkName[K10_DEVICE_ID_LEN];
} IOCTL_AIDVD_INSERT_DISK_VOLUME_PARAMS, *PIOCTL_AIDVD_INSERT_DISK_VOLUME_PARAMS;

// Tear down any device object that matches.
typedef struct _IOCTL_AIDVD_DESTORY_DISK_VOLUME_PARAMS
{
    K10_WWID            LunWwn;
} IOCTL_AIDVD_DESTORY_DISK_VOLUME_PARAMS, *PIOCTL_AIDVD_DESTROY_DISK_VOLUME_PARAMS;


// Defines the order that AIDVD's should have their device objects inserted.
// The higher the number, the closer to the DLU.
// The "holes" allows adding new drivers without moving the existing ones
// around. Be aware that MAX_AIDVDS is defined as 10.
typedef enum _AIDVD_PRIORITY
{
    AIDVD_PRIORITY_NONE = 0,

    AIDVD_PRIORITY_MIG = 1, //LM is not inserted in the device stack currently.
   
    AIDVD_PRIORITY_REQUEST_FORWARDING_MIDDLE = 2, // Middle Redirector

    AIDVD_PRIORITY_TDX = 3,

    AIDVD_PRIORITY_REQUEST_FORWARDING_TOP = 4,

    AIDVD_PRIORITY_APM = 5, // deprecated?    

    AIDVD_PRIORITY_REQUEST_FORWARDING = 5, // Upper and Lower Redirector
    
    AIDVD_PRIORITY_EFDC = 6,
    
    AIDVD_PRIORITY_SPLITTER = 7,
    
    AIDVD_PRIORITY_SNAPBACK = 8,
	
    AIDVD_PRIORITY_QOS = 9,

    // The HIGHEST value should be the maximum of the others.
    AIDVD_PRIORITY_HIGHEST = AIDVD_PRIORITY_QOS
} AIDVD_PRIORITY;

// A query operation on the AIDVD's factory get some global information.
typedef struct _IOCTL_AIDVD_DRIVER_PARAMS
{
    // What order should two AIDVDs have their device objects inserted?
    AIDVD_PRIORITY       Priority;

    // Should the auto-inserter (e.g. Hostside) send the
    // IOCTL_AIDVD_INITIALIZATION_COMPLETE after initialization is complete?
    BOOLEAN              NotifyWhenAvdidInitializationCompletes;

} IOCTL_AIDVD_DRIVER_PARAMS, *PIOCTL_AIDVD_DRIVER_PARAMS;
//
// Name: FLARE_USERSPACE_REQUEST_DATA
//
// Description: Flare returns this structure to a user space process
//              to carry out a userspace request for Flare. The userspace
//              process waits on a IOCTL_FLARE_TO_USERSPACE_REQUEST_WAIT ioctl.
//

typedef struct flare_userspace_request_data
{
    ULONG   revision;           // Revision number
    ULONG   requestType;        // Request type;
} FLARE_USERSPACE_REQUEST_DATA, *PFLARE_USERSPACE_REQUEST_DATA;

#define FLARE_USERSPACE_POWER_OFF_REQUEST           1


//
// Name: FLARE_DESCRIBE_EXTENTS_REQUEST
//
// Description:  This describes the input buffer for IOCTL_FLARE_DESCRIBE_EXTENTS.
//
//
typedef struct _FLARE_DESCRIBE_EXTENTS_REQUEST
{
    // The byte offset onto the Disk Side Logical Unit's media.  Must be a multiple of sector size.
    ULONGLONG   StartByteOffset;

    // The maximum length of the section of the LU starting at StartByteOffset for which extent information may be returned.
    // A value of EXTENT_END_OF_VOLUME_LENGTH indicates the remainder of the LU.
    // Otherwise, the value must be nonzero and a multiple of sector size.
    ULONGLONG   LengthInBytes;

    #define EXTENT_END_OF_VOLUME_LENGTH   (ULONGLONG) (-1)         // Describe extents for entire LU,
                                                                   // so that requestors need not know LU size.

    // Indicates the version of the volume that should be compared with the version in focus on this device stack
    // when determining the state of the EXTENT_IS_KNOWN_SAME bit in the Flags field.
    // A ComparisonVersion of 0 indicates the initial creation of the file, prior to any writes, and is always valid.
    // Other valid values are obtained from the responder prior to this IOCTL by means unspecified here.
    ULONGLONG   ComparisonVersion;

} FLARE_DESCRIBE_EXTENTS_REQUEST, *PFLARE_DESCRIBE_EXTENTS_REQUEST;


//
// Name: FLARE_EXTENT_INFO
//
// Description:  This describes one element of the array of extent descriptions
//               returned in the output buffer for IOCTL_FLARE_DESCRIBE_EXTENTS.
//
//
typedef struct _FLARE_EXTENT_INFO
{
    // The starting byte offset onto the Disk Side Logical Unit's media.  Must be a multiple of sector size.
    ULONGLONG   FirstByteOffset;

    // The number of bytes in this extent.  Must be nonzero and a multiple of sector size.
    ULONGLONG   ExtentSizeInBytes;

    // Attributes of this extent.  Bits other than those defined below must be zero.
    ULONGLONG   Flags;

    #define EXTENT_STORAGE_ALLOCATED 0x01   // If set, writes to this extent will not fail due to lack of space.
    #define EXTENT_IS_KNOWN_SAME     0x02   // If set, the entire contents of the extent are known to match those at the time indicated by ComparisonVersion.
    #define EXTENT_IS_KNOWN_ZERO     0x04   // If set, the entire contents of the extent are known to be zero.
    #define EXTENT_IS_METADATA       0x08   // If set, the entire contents of the extent are metadata targeted to higher tier storage.

    // Reserved for future use.  Must be zero.
    ULONGLONG   Spare;
} FLARE_EXTENT_INFO, *PFLARE_EXTENT_INFO;

//
// Name: CACHE_HDW_STATUS
//
// Description:  This is the state of the Flare ATM hardware
//
//
typedef enum cache_hdw_state
{
    CACHE_HDW_OK = 1,
    CACHE_HDW_FAULT,
    CACHE_HDW_SHUTDOWN_IMMINENT,
    CACHE_HDW_APPROACHING_OVER_TEMP
} CACHE_HDW_STATUS;

//
// Name: SP_ENVIRONNEMT_STATE
//
// Description:  This is the SP's environmental state from SPS, fans, etc.
//
//
typedef struct sp_environment_state
{
    CACHE_HDW_STATUS    HardwareStatus;
    ULONG               SpsPercentCharged;
    BOOL                SsdFault;
} SP_ENVIRONMENT_STATE, *PSP_ENVIRONMENT_STATE;

//
// Name: SP_QUIESCE_IO
//
// Description:  The quiesce I/O flag for IOCTL_FLARE_QUIESCE_BACKGROUND_IO.
//               The flag is set to true to quiesce; false to unquiesce.
//
typedef struct sp_quiesce_io
{
    BOOL   flag;
} SP_QUIESCE_IO, *PSP_QUIESCE_IO;

// Input parameter for IOCTL_CACHE_ALLOCATE_PSEUDO_CONTROL_MEMORY
struct IoctlCacheAllocatePseudoControlMemory {
    // Number of bytes to be allocated from persistent memory
    // to use as control memory. There is restriction on minimum
    // size i.e. 8K which one can request to the SPCache. Less than
    // 8K request will be rejected by SPCache.
    ULONGLONG NumBytes;

    // Whether the allocated memory should include in crash
    // dump or not.
    // FIXME: How to use it?
    BOOLEAN IncludeInCrashDump;

    // 4 character name of the client. Debug purpose.
    ULONG Tag;

    // Spare bytes for future use.
    UINT_32 spare[4];
};

// Output parameter for IOCTL_CACHE_ALLOCATE_PSEUDO_CONTROL_MEMORY
struct IoctlCacheAllocatePseudoControlMemoryOutput {
    // Holds address of the allocated pseudo control memory.
    ULONG64 pseudoCtrlMemAddr;

    // Spare bytes for future use.
    UINT_32 spare[2];
};

// Input parameter for IOCTL_CACHE_FREE_PSEUDO_CONTROL_MEMORY
struct IoctlCacheFreePseudoControlMemory {
    // Memory address of pseudo control memory which needs to be freed. After that it
    // can be used either to allocate pseudo control memory for the next request or
    // to expand the cache memory if required.
    ULONG64 MemAddress;

    // Spare bytes for future use.
    UINT_32 spare[2];
};

// FIX: Need to identify the parameters.
struct IoctlCacheBootComplete {
    UINT_32 spare[2];
};

// Structure used for the IOCTL_FLARE_GET_VOLUME_CACHE_CONFIG IOCTL.
// The SP Cache uses this structure to get the LU caching configuration
// and size from Flare during the NDU from a bundle not running the SP
// Cache to a bundle running the SP Cache.
typedef struct _FLARE_GET_VOLUME_CACHE_CONFIG {
    // Unit cache configuration parameters copied from dba_export_types.h
    // Inclusion of dba_export_types.h in this file caused a compilation
    // assert in generic.h.
    struct _FLARE_CACHE_PARAMS
    {
        UINT_32 priority_lru_length;
        // % of cache dedicated to the read cache lru

        UINT_16E prefetch_segment_length;
        // Segment size used for prefetching

        UINT_16E prefetch_total_length;
        // Total size that should be prefetched

        UINT_16E prefetch_maximum_length;
        // Maximum size that a prefetch can be

        UINT_16E prefetch_disable_length;
        // Do not prefetch if requests are larger than this

        BITS_8E cache_config_flags;
        // Various cache related flags

        UINT_8E read_retention_priority;
        // see mode page 8

        UINT_8E cache_idle_threshold;
        // Number of requests that make a unit busy (non-idle)

        UINT_8E cache_idle_delay;   /* */ /* 1 byte */
        // Time (sec) that a unit must be idle
        // before idle time activity starts

        UINT_32 cache_write_aside;
        // I/O block size above which requests write around the cache

        UINT_32 cache_dirty_word;
        // indicates if the cache currently has dirty pages.

        UINT_32 prefetch_idle_count;
        // ??

        UINT_32 L2_cache_dirty_word;
        // CACHE_DIRTY indicates that there is L2 cache info in memory
        // for this LUN

        UINT_32 read_be_req_size;
        // read_be_req_size forces all read cache requests below a certain size
        // to a standard size

    } unit_cache_params;

    // User capacity
    LBA_T user_capacity;

    // Internal Flare capacity including space for zero map and MCR meta data.
    LBA_T internal_capacity;

    UINT_32 spares[4];
} FLARE_GET_VOLUME_CACHE_CONFIG, *PFLARE_GET_VOLUME_CACHE_CONFIG;

// Reply data for Query on current vault-LUN usage (for encryption purposes) 
enum CacheVaultImageStatus {
    CacheVaultImageNotInUse = 0,
    CacheVaultImageBusy
};

struct IoctlCacheVaultImageOutput {
    enum CacheVaultImageStatus status;
};


#endif /* !defined (FLARE_EXPORT_IOCTLS_H) */

