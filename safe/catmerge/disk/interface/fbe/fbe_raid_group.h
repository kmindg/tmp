#ifndef FBE_RAID_GROUP_H
#define FBE_RAID_GROUP_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/
 
/*!**************************************************************************
 * @file fbe_raid_group.h
 ***************************************************************************
 *
 * @brief
 *  This file contains definitions of functions that are exported by the
 *  raid group object.
 * 
 * @ingroup raid_group_class_files
 * 
 * @revision
 *   5/20/2009:  Created. RPF
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#ifndef UEFI_ENV
#include "fbe/fbe_object.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_base_config.h"
#endif

//----------------------------------------------------------------
// Define the timeout period for RG to become enable
//----------------------------------------------------------------
/*! @def RG_READY_TIMEOUT
 *  @brief
 *    This is the definition for RG creation API. After RG is created, we will wait
 *    for defined time period for RG to become ready.

 *  @ingroup fbe_api_raid_group_interface
 */
 /* This macro is common for both fbecli and admin-shim*/
 //----------------------------------------------------------------

#define RG_READY_TIMEOUT 480000
#define RG_DESTROY_TIMEOUT 180000

/*!*******************************************************************
 * @def FBE_RAID_SECTORS_PER_ELEMENT
 *********************************************************************
 * @brief This is the standard element size in blocks.
 *
 *********************************************************************/
#define FBE_RAID_SECTORS_PER_ELEMENT 128

/*!*******************************************************************
 * @def FBE_RAID_ELEMENTS_PER_PARITY
 *********************************************************************
 * @brief This is the number of elements per parity stripe for the
 *        legacy/small element size.
 *
 *********************************************************************/
#define FBE_RAID_ELEMENTS_PER_PARITY 8

/*!*******************************************************************
 * @def FBE_RAID_DEFAULT_CHUNK_SIZE
 *********************************************************************
 * @brief This is the standard chunk size in blocks.
 *
 *********************************************************************/
#define FBE_RAID_DEFAULT_CHUNK_SIZE 2048

/*!*******************************************************************
 * @def FBE_RAID_MAX_BE_XFER_SIZE
 *********************************************************************
 * @brief Max blocks we will allow to be transferred on the backend.
 *        This determines how we break up I/Os.
 * 
 *        This is arbitrary, but is twice our current max I/O size of
 *        1 MB (2048 blocks).
 *
 *********************************************************************/
#define FBE_RAID_MAX_BE_XFER_SIZE (2048 * 2)

/*!*******************************************************************
 * @def FBE_RAID_MAX_XFER_SIZE
 *********************************************************************
 * @brief Max blocks we will allow to be transferred.
 *        This determines how we break up I/Os.
 * 
 *        This is arbitrary, but is twice our current max I/O size of
 *        1 MB (2048 blocks) across our max width of 16.
 *
 *********************************************************************/
#define FBE_RAID_MAX_XFER_SIZE (FBE_RAID_MAX_BE_XFER_SIZE * FBE_RAID_MAX_DISK_ARRAY_WIDTH)

/*!*******************************************************************
 * @def FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH
 *********************************************************************
 * @brief This is the bandwidth elements per parity size.
 *
 *********************************************************************/
#define FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH 1024

/*!*******************************************************************
 * @def FBE_RAID_ELEMENTS_PER_PARITY_BANDWIDTH
 *********************************************************************
 * @brief This is the bandwidth element size.
 *
 *********************************************************************/
#define FBE_RAID_ELEMENTS_PER_PARITY_BANDWIDTH 2

/*!*******************************************************************
 * @def FBE_RAID_GROUP_DEFAULT_QUEUE_DEPTH
 *********************************************************************
 * @brief We allow this many I/Os per drive by default before we throttle.
 *
 *********************************************************************/
#define FBE_RAID_GROUP_DEFAULT_QUEUE_DEPTH 30

/*!*******************************************************************
 * @def FBE_RAID_GROUP_MIN_QUEUE_DEPTH
 *********************************************************************
 * @brief We allow this many I/Os per drive by default before we throttle
 *        on raid groups bound on the system drives.
 *
 *********************************************************************/
#define FBE_RAID_GROUP_MIN_QUEUE_DEPTH 8

/*!*******************************************************************
 * @def     FBE_RAID_GROUP_PEER_CHECKPOINT_UPDATE_SECS
 *********************************************************************
 * @brief   This is period (in seconds) between updates to the peer
 *
 *********************************************************************/
#define FBE_RAID_GROUP_PEER_CHECKPOINT_UPDATE_SECS  30  /* Time to wait before sending checkpoint to peer. */


/*!*******************************************************************
 * @def     FBE_RAID_GROUP_PEER_CHECKPOINT_UPDATE_WINDOW
 *********************************************************************
 * @brief   This is the max granularity before we will force an update to the peer.
 *
 *********************************************************************/
#define FBE_RAID_GROUP_PEER_CHECKPOINT_UPDATE_WINDOW  (0x800 * 2 * 1024) //2GB 

/*! @enum fbe_raid_verify_type_t
 *  
 *  @brief These are the types of verify operations that can be performed.
 */
typedef enum fbe_raid_verify_type_e
{
    FBE_RAID_VERIFY_TYPE_NONE = 0,      // No verify
    FBE_RAID_VERIFY_TYPE_USER_RW,       // User read/write verify
    FBE_RAID_VERIFY_TYPE_USER_RO,       // User read-only verify
    FBE_RAID_VERIFY_TYPE_ERROR,         // "LU dirty" verify
    FBE_RAID_VERIFY_TYPE_INCOMPLETE_WRITE,    /*!< Lun shutdown with non-cached writes in flight. */
    FBE_RAID_VERIFY_TYPE_SYSTEM,        // System verify.
}fbe_raid_verify_type_t;

/*************************
 *   FUNCTION DEFINITIONS 
 *************************/

 /*! @defgroup raid_group_class Raid Group Class
 *  @brief This is the set of definitions for the base config.
 *  @ingroup fbe_base_object
 */ 
/*! @defgroup raid_group_usurper_interface Raid Group Usurper Interface
 *  @brief This is the set of definitions that comprise the base config class
 *  usurper interface.
 *  @ingroup fbe_classes_usurper_interface
 * @{
 */ 

/*!********************************************************************* 
 * @enum fbe_raid_group_control_code_t 
 *  
 * @brief 
 * This enumeration lists all the base config specific control codes. 
 * These are control codes specific to the base config, which only 
 * the base config object will accept. 
 *         
 **********************************************************************/
#ifndef UEFI_ENV
typedef enum
{
    FBE_RAID_GROUP_CONTROL_CODE_INVALID = FBE_OBJECT_CONTROL_CODE_INVALID_DEF(FBE_CLASS_ID_RAID_GROUP),

    /*! Configure the basic attributes of the raid group such as width.
     */
    FBE_RAID_GROUP_CONTROL_CODE_SET_CONFIGURATION,

    /*! Update a RAID object.
     */
    FBE_RAID_GROUP_CONTROL_CODE_UPDATE_CONFIGURATION,
    
    /*! Get the verify status for a LUN
     */
    FBE_RAID_GROUP_CONTROL_CODE_GET_LUN_VERIFY_STATUS,

    /*! Get the write log flush error counters for a raid group
     */
    FBE_RAID_GROUP_CONTROL_CODE_GET_FLUSH_ERROR_COUNTERS,

    /*! Clear the write log flush error counters for a raid group
     */
    FBE_RAID_GROUP_CONTROL_CODE_CLEAR_FLUSH_ERROR_COUNTERS,

    /*! Mark the raid group for quiescing.
     */
    FBE_RAID_GROUP_CONTROL_CODE_QUIESCE,

    /*! Mark the raid group for unquiescing.
     */
    FBE_RAID_GROUP_CONTROL_CODE_UNQUIESCE,

    /*! Get all information about the raid group.
     */
    FBE_RAID_GROUP_CONTROL_CODE_GET_INFO,

    /*! Get write log information from the raid group write log.
     */
    FBE_RAID_GROUP_CONTROL_CODE_GET_WRITE_LOG_INFO,

    /*! Map lba to pba.
     */
    FBE_RAID_GROUP_CONTROL_CODE_MAP_LBA,

    /*! Map pba to lba.
     */
    FBE_RAID_GROUP_CONTROL_CODE_MAP_PBA,

    /*! This is sent to the class to get the capacity.
     */
    FBE_RAID_GROUP_CONTROL_CODE_CLASS_GET_INFO,
    
    /*! This is sent to the object to GET the raid library debug flags.
     */
    FBE_RAID_GROUP_CONTROL_CODE_GET_RAID_LIBRARY_DEBUG_FLAGS,

    /*! This is sent to the object to change the raid library debug flags.
     */
    FBE_RAID_GROUP_CONTROL_CODE_SET_RAID_LIBRARY_DEBUG_FLAGS,

    /*! This is sent to the class to set the default raid library debug flags
     *  for all newly created raid groups.
     */
    FBE_RAID_GROUP_CONTROL_CODE_CLASS_GET_RAID_LIBRARY_DEBUG_FLAGS,
    
    /*! This is sent to the class to set the default raid library debug flags
     *  for all newly created raid groups.
     */
    FBE_RAID_GROUP_CONTROL_CODE_CLASS_SET_RAID_LIBRARY_DEBUG_FLAGS,

    /*! This is sent to the object to get the raid group debug flags.
     */
    FBE_RAID_GROUP_CONTROL_CODE_GET_RAID_GROUP_DEBUG_FLAGS,

    /*! This is sent to the object to change the raid group debug flags.
     */
    FBE_RAID_GROUP_CONTROL_CODE_SET_RAID_GROUP_DEBUG_FLAGS,

    FBE_RAID_GROUP_CONTROL_CODE_SET_DEFAULT_DEBUG_FLAGS,
    FBE_RAID_GROUP_CONTROL_CODE_SET_DEFAULT_LIBRARY_FLAGS,
    FBE_RAID_GROUP_CONTROL_CODE_GET_DEFAULT_DEBUG_FLAGS,
    FBE_RAID_GROUP_CONTROL_CODE_GET_DEFAULT_LIBRARY_FLAGS,

    /* Set default parameters used by block transport.
     */
    FBE_RAID_GROUP_CONTROL_CODE_SET_DEFAULT_BTS_PARAMS,
    FBE_RAID_GROUP_CONTROL_CODE_GET_DEFAULT_BTS_PARAMS,
     
    /*! This is sent to the class to get the default raid group debug flags
     *  for all newly created raid groups.
     */
    FBE_RAID_GROUP_CONTROL_CODE_CLASS_GET_RAID_GROUP_DEBUG_FLAGS,

    /*! This is sent to the class to set the default raid group debug flags
     *  for all newly created raid groups.
     */
    FBE_RAID_GROUP_CONTROL_CODE_CLASS_SET_RAID_GROUP_DEBUG_FLAGS,

    /*! sent frum LU to learn what the user configured in RAID. In the original design the condifuration for power saving was
    supposed to be at the LU level, but it is not implemented. To keep an option to this design, LU will still run the show,
    but it will ask RAID what the parameters the user asked for and work based on that.
     */
    FBE_RAID_GROUP_CONTROL_CODE_GET_POWER_SAVING_PARAMETERS,

    /*! This is sent to the class to enable the unexpected-error-testing path.
     */
    FBE_RAID_GROUP_CONTROL_CODE_CLASS_SET_RAID_LIBRARY_ERROR_TESTING,
    /*! This is sent to the class to enable the random unexpected-error-testing path.
     */
    FBE_RAID_GROUP_CONTROL_CODE_CLASS_SET_RAID_LIBRARY_RANDOM_ERRORS,

    /*! This is sent to the class to get the unexpected-error-testing stats.
     */
    FBE_RAID_GROUP_CONTROL_CODE_CLASS_GET_RAID_LIBRARY_ERROR_TESTING_STATS,

    /*! This is sent to the class to reset unexpected-error-testing stats
     */
    FBE_RAID_GROUP_CONTROL_CODE_CLASS_RESET_RAID_LIBRARY_ERROR_TESTING_STATS,

    /*! This is sent to the class to get raid memory statistics
     */
    FBE_RAID_GROUP_CONTROL_CODE_CLASS_GET_RAID_MEMORY_STATS,

    /*! Set various options to the raid group class.
     */
    FBE_RAID_GROUP_CONTROL_CODE_CLASS_SET_OPTIONS,

    /*! This is to get the current I/O information
    */
    FBE_RAID_GROUP_CONTROL_CODE_GET_IO_INFO,

    /*! This is to get the zero percentage for the the raid extent extent.
     */
    FBE_RAID_GROUP_CONTROL_CODE_GET_ZERO_CHECKPOINT_RAID_EXTENT,

    /*! This is to get the zero percentage for the lun extent.
     */
    FBE_RAID_GROUP_CONTROL_CODE_GET_ZERO_CHECKPOINT_FOR_DOWNSTREAM_OBJECT,

    /*! This is to get the zero percentage for the downstream mirror object in case of RAID10.
     */
    FBE_RAID_GROUP_CONTROL_CODE_GET_ZERO_CHECKPOINT_FOR_DOWNSTREAM_MIRROR_OBJECT,

    /*! This is to start the needs rebuild operation for particular chunk. 
     */
    FBE_RAID_GROUP_CONTROL_CODE_NEEDS_REBUILD_OPERATION,

    /*! This is to start the rebuild opreation for the particular chunk. 
     */
    FBE_RAID_GROUP_CONTROL_CODE_REBUILD_OPERATION,

    /*! This is to start the rebuild opreation for the particular chunk. 
     */
    FBE_RAID_GROUP_CONTROL_CODE_REKEY_OPERATION,

    /*! This control code is used to set the rebuild checkpoint for testing purposes.
     */
    FBE_RAID_GROUP_CONTROL_CODE_SET_RB_CHECKPOINT, 

    /*! This control code is used to set a verify checkpoint for testing purposes.
     */
    FBE_RAID_GROUP_CONTROL_CODE_SET_VERIFY_CHECKPOINT,

    /*! This control code is used to check whether a pvd can be reinitialized
     */
    FBE_RAID_GROUP_CONTROL_CODE_CAN_PVD_GET_REINTIALIZED,

    /*! This control code is used to mark NR 
    */
    FBE_RAID_GROUP_CONTROL_CODE_MARK_NR,

    /*! This control code gets the degraded information (is degraded and rebuilt percent)*/
    FBE_RAID_GROUP_CONTROL_CODE_GET_DEGRADED_INFO, 

    /*! This control code is used to get rebuild status for the raid group. */
    FBE_RAID_GROUP_CONTROL_CODE_GET_REBUILD_STATUS,

    /*! This control code gets the percent rebuilt for the lun information supplied. */
    FBE_RAID_GROUP_CONTROL_CODE_GET_LUN_PERCENT_REBUILT,

    /*! This control code is used to set preferred position in raid geometry. */
    FBE_RAID_GROUP_CONTROL_CODE_SET_PREFERED_POSITION,

    /* set background operation speed */
    FBE_RAID_GROUP_CONTROL_CODE_SET_BG_OP_SPEED,

    /* set background operation speed */
    FBE_RAID_GROUP_CONTROL_CODE_GET_BG_OP_SPEED,    

    /* get the downstream health of the raid group */
    FBE_RAID_GROUP_CONTROL_CODE_GET_HEALTH,

    /*! This usurper sets the period between sending checkpoint updates to the
     *  peer SP.
     */
    FBE_RAID_GROUP_CONTROL_CODE_CLASS_SET_UPDATE_PEER_CHECKPOINT_INTERVAL,
    /*! Get all information about the raid group.
     */
    FBE_RAID_GROUP_CONTROL_CODE_GET_STATS,

    /*! Set the state in order to start encryption. 
     */
    FBE_RAID_GROUP_CONTROL_CODE_SET_ENCRYPTION_STATE,

    /*! This is sent to the class to reset raid memory statistics
     */
    FBE_RAID_GROUP_CONTROL_CODE_CLASS_RESET_RAID_MEMORY_STATS,

    /*! Get the lowest drive tier of the raid group
     */
    FBE_RAID_GROUP_CONTROL_CODE_GET_LOWEST_DRIVE_TIER,

    /*! Query downstream to recalculate the lowest drive tier
     */
    FBE_RAID_GROUP_CONTROL_CODE_GET_LOWEST_DRIVE_TIER_DOWNSTREAM,

    /*! Set the raid group raid geometry attribute
     */
    FBE_RAID_GROUP_CONTROL_CODE_SET_RAID_ATTRIBUTE,

    /*! Gets the position of the drive in the RG based on Object/Server ID 
     */
    FBE_RAID_GROUP_CONTROL_CODE_GET_DRIVE_POS_BY_SERVER_ID,

    /*! Sends the specified usurper to the downstream positions specified.
     */
    FBE_RAID_GROUP_CONTROL_CODE_SEND_TO_DOWNSTREAM_POSITIONS,

    /*! Gets the current values of the extended media error handling
     */
    FBE_RAID_GROUP_CONTROL_CODE_CLASS_GET_EXTENDED_MEDIA_ERROR_HANDLING,

    /*! Sets the current values of the extended media error handling.
     *  If selected the values will also be persisted thru the registry.
     */
    FBE_RAID_GROUP_CONTROL_CODE_CLASS_SET_EXTENDED_MEDIA_ERROR_HANDLING,

    /*! Gets the current values of the extended media error handling
     *  for the raid group specified.
     */
    FBE_RAID_GROUP_CONTROL_CODE_GET_EXTENDED_MEDIA_ERROR_HANDLING,

    /*! Sets the extended media error handling values for the raid group
     *  specified.  This does not change the class values.
     */
    FBE_RAID_GROUP_CONTROL_CODE_SET_EXTENDED_MEDIA_ERROR_HANDLING,

    /*! Sets the following values for debug and testing:
     *      o Local state
     *      o Any condition
     *      o Clustered flags
     */
    FBE_RAID_GROUP_CONTROL_CODE_SET_DEBUG_STATE,

    /*! Set the number of chunks per rebuild
     */
    FBE_RAID_GROUP_CONTROL_CODE_CLASS_SET_CHUNKS_PER_REBUILD,

    /*! Gets the max wear level collected from the drives below
     */
    FBE_RAID_GROUP_CONTROL_CODE_GET_WEAR_LEVEL_DOWNSTREAM,

    /*! Gets the wear level recorded at the raid group
     */
    FBE_RAID_GROUP_CONTROL_CODE_GET_WEAR_LEVEL,

    /*! Sets the interval of the lifecycle timer condition
     */
    FBE_RAID_GROUP_CONTROL_CODE_SET_LIFECYCLE_TIMER,

    /* Insert new control codes here.
     */
    FBE_RAID_GROUP_CONTROL_CODE_LAST
}
fbe_raid_group_control_code_t;
#endif

/*! @enum fbe_raid_group_type_t 
 *  
 *  @brief The mutually exclusive different types of raid groups.
 *         This is the user specified (except where noted), RAID 
 *         type.
 *
 *  @note These types should map to the user visible raid types.
 */
typedef enum fbe_raid_group_type_e
{
    /*! The raid type hasn't been configured yet.
     */
    FBE_RAID_GROUP_TYPE_UNKNOWN                 = 0,

    /*! Indicates that this is a RAID-1 (mirror) raid group
     */
    FBE_RAID_GROUP_TYPE_RAID1                   = 1,

    /*! Indicates that this is a RAID-10 (striped mirror) raid group
     */
    FBE_RAID_GROUP_TYPE_RAID10                  = 2,

    /*! Indicates that this is a RAID-3 (MR3) raid group.
     */
    FBE_RAID_GROUP_TYPE_RAID3                   = 3,

    /*! Indicates that this is a RAID-0 (striped) raid group.
     */
    FBE_RAID_GROUP_TYPE_RAID0                   = 4,

    /*! Indicates that this is a RAID-5 (single parity) raid group
     */
    FBE_RAID_GROUP_TYPE_RAID5                   = 5,
   
    /*! Indicates that this is a RAID-6 (dual parity) raid group
     */
    FBE_RAID_GROUP_TYPE_RAID6                   = 6,

    /*! Indicates that this is a sparing (either hot-spare, proactive-spare, 
     *  or permanent-spare) raid group
     */
    FBE_RAID_GROUP_TYPE_SPARE                   = 7,

    /*! @note The following raid group types are for internal use only and thus
     *        cannot and should not be configured by the user.
     */

    /*! This type indicates that this extent is a mirror under a striper.
     */
    FBE_RAID_GROUP_TYPE_INTERNAL_MIRROR_UNDER_STRIPER = 8,
    
    /*! This type indicates the extent is a mirrored metadata extent.
     */
    FBE_RAID_GROUP_TYPE_INTERNAL_METADATA_MIRROR = 9,

    /*! This is a mirror that is managed by the metadata service
     *  and the configuration service. 
     *  This type of mirror does not have an object.
     */
    FBE_RAID_GROUP_TYPE_RAW_MIRROR = 10,
    
}
fbe_raid_group_type_t;

/*! @enum fbe_raid_group_debug_flags_t 
 *  
 *  @brief Debug flags that allow run-time debug functions.
 *
 *  @note Typically these flags should only be set for unit or
 *        intergration testing since they could have SEVERE 
 *        side effects.
 */
typedef enum fbe_raid_group_debug_flags_e
{
    FBE_RAID_GROUP_DEBUG_FLAG_NONE                            = 0x00000000,

    /*! Enable I/O tracing
     */
    FBE_RAID_GROUP_DEBUG_FLAG_IO_TRACING                      = 0x00000001,

    /*! Enable rebuild tracing.
     */
    FBE_RAID_GROUP_DEBUG_FLAG_REBUILD_TRACING                 = 0x00000002,

    /*! Trace changes to the paged metadata for NR.
     */
    FBE_RAID_GROUP_DEBUG_FLAGS_TRACE_PMD_NR                   = 0x00000004,
    FBE_RAID_GROUP_DEBUG_FLAGS_TRACE_NEEDS_REBUILD            = 0x00000008,

    /* Traces when iots arrives and completes.
     */
    FBE_RAID_GROUP_DEBUG_FLAGS_IOTS_TRACING                   = 0x00000010,

    /* Traces for the iots locking.
     */
    FBE_RAID_GROUP_DEBUG_FLAGS_STRIPE_LOCK_TRACING            = 0x00000040,

    FBE_RAID_GROUP_DEBUG_FLAGS_TRACE_PMD_VERIFY               = 0x00000100,
    /*! Trace when the raid group receives events.
     */
    FBE_RAID_GROUP_DEBUG_FLAGS_EVENT_TRACING                  = 0x00000400,
    /*! Trace when the raid group generates events in background ops.
     */
    FBE_RAID_GROUP_DEBUG_FLAGS_MONITOR_EVENT_TRACING          = 0x00000800,
    
    FBE_RAID_GROUP_DEBUG_FLAG_VERIFY_TRACING                  = 0x00001000,

    /*! Trace the medic action taken (multiple actions required).
     */
    FBE_RAID_GROUP_DEBUG_FLAG_MEDIC_ACTION_TRACING            = 0x00002000,

    /*! Trace the rebuild notification info if enabled.
     */
    FBE_RAID_GROUP_DEBUG_FLAG_REBUILD_NOTIFICATION_TRACING    = 0x00004000,

    /*! Trace detailed information about the quiesce.
     */
    FBE_RAID_GROUP_DEBUG_FLAG_QUIESCE_TRACING                 = 0x00008000,

    /*! Disable the mark NR validation in update paged.
     */
    FBE_RAID_GROUP_DEBUG_FLAG_DISABLE_UPDATE_NR_VALIDATION    = 0x00010000,

    /*! Trace the rebuild notification info if enabled.
     */
    FBE_RAID_GROUP_DEBUG_FLAG_LUN_PERCENT_REBUILT_NOTIFICATION_TRACING = 0x00020000,
    /*! Trace the rebuild notification info if enabled.
     */
    FBE_RAID_GROUP_DEBUG_FLAG_NP_LOCK_TRACING                 = 0x00040000,

    /*! Trace everything related to encryption.
     */
    FBE_RAID_GROUP_DEBUG_FLAG_ENCRYPTION_TRACING              = 0x00080000,

    /*! Trace drive tier related information.
     */
    FBE_RAID_GROUP_DEBUG_FLAG_DRIVE_TIER_TRACING              = 0x00100000,

    /*! Trace extended media error handling (EMEH)
     */
    FBE_RAID_GROUP_DEBUG_FLAG_EXTENDED_MEDIA_ERROR_HANDLING   = 0x00200000,

    /*! @note Special flag that indicates we should ignore set
     *        request.
     */
    FBE_RAID_GROUP_DEBUG_FLAG_RESERVED_IGNORE_SET             = 0x00400000,
    /*! Unused debug flags.*/
    /*! Disable sending the CRC error usurper to pdo.  This is typically
     *  used for tests that expect to see multi-bit CRC errors that
     *  were injected by an object below raid.
     */
    FBE_RAID_LIBRARY_DEBUG_FLAG_DISABLE_SEND_CRC_ERR_TO_PDO =   0x00800000,
    FBE_RAID_GROUP_DEBUG_FLAG_UNUSED2                         = 0x01000000,
    FBE_RAID_GROUP_DEBUG_FLAG_UNUSED3                         = 0x02000000,
    FBE_RAID_GROUP_DEBUG_FLAG_UNUSED4                         = 0x04000000,
    FBE_RAID_GROUP_DEBUG_FLAG_UNUSED5                         = 0x08000000,
    FBE_RAID_GROUP_DEBUG_FLAG_UNUSED_MASK                     = (FBE_RAID_GROUP_DEBUG_FLAG_RESERVED_IGNORE_SET |
                                                                 FBE_RAID_GROUP_DEBUG_FLAG_UNUSED2 | FBE_RAID_GROUP_DEBUG_FLAG_UNUSED3 |
                                                                 FBE_RAID_GROUP_DEBUG_FLAG_UNUSED4 | FBE_RAID_GROUP_DEBUG_FLAG_UNUSED5   ),

    /*! @note The following flags change behavior.  Don't set them unless
     *        you completely understand what you are doing!
     */
    /* Generate an `error' level trace when a `user' (i.e. not a metadata)
     * I/O encounters an error.
     */
    FBE_RAID_GROUP_DEBUG_FLAGS_ERROR_TRACE_USER_IOTS_ERROR    = 0x10000000,
    /* Generate an `error' level trace event.  Allows process to stop
     * if `stop on error' is set.
     */
    FBE_RAID_GROUP_DEBUG_FLAGS_GENERATE_ERROR_TRACE           = 0x20000000,
    /* Generate an `error' level trace when a `monitor' (i.e. not a user)
     * I/O encounters an error.
     */
    FBE_RAID_GROUP_DEBUG_FLAGS_ERROR_TRACE_MONITOR_IOTS_ERROR = 0x40000000,

    /*! Force all write to become WRITE-VERIFY.
     */
    FBE_RAID_GROUP_DEBUG_FLAG_FORCE_WRITE_VERIFY              = 0x80000000,

   
    FBE_RAID_GROUP_DEBUG_FLAG_INVALID_MASK                    = (FBE_RAID_GROUP_DEBUG_FLAG_UNUSED_MASK),
    FBE_RAID_GROUP_DEBUG_FLAG_VALID_MASK                      = (0xFFFFFFFF & ~FBE_RAID_GROUP_DEBUG_FLAG_INVALID_MASK),


} fbe_raid_group_debug_flags_t; 

/*!*******************************************************************
 * @def FBE_RAID_GROUP_DEBUG_FLAG_STRINGS
 *********************************************************************
 * @brief Set of strings that correspond to the above flags.
 *
 *********************************************************************/
#define FBE_RAID_GROUP_DEBUG_FLAG_STRINGS\
    "FBE_RAID_GROUP_DEBUG_FLAG_IO_TRACING",\
    "FBE_RAID_GROUP_DEBUG_FLAG_REBUILD_TRACING",\
    "FBE_RAID_GROUP_DEBUG_FLAGS_TRACE_PMD_NR",\
    "FBE_RAID_GROUP_DEBUG_FLAGS_TRACE_NEEDS_REBUILD",\
    "FBE_RAID_GROUP_DEBUG_FLAGS_IOTS_TRACING",\
    "FBE_RAID_GROUP_DEBUG_FLAGS_ERROR_TRACE_USER_IOTS_ERROR",\
    "FBE_RAID_GROUP_DEBUG_FLAGS_STRIPE_LOCK_TRACING",\
    "FBE_RAID_GROUP_DEBUG_FLAGS_GENERATE_ERROR_TRACE",\
    "FBE_RAID_GROUP_DEBUG_FLAGS_TRACE_PMD_VERIFY",\
    "FBE_RAID_GROUP_DEBUG_FLAGS_ERROR_TRACE_MONITOR_IOTS_ERROR",\
    "FBE_RAID_GROUP_DEBUG_FLAGS_EVENT_TRACING",\
    "FBE_RAID_GROUP_DEBUG_FLAGS_MONITOR_EVENT_TRACING",\
    "FBE_RAID_GROUP_DEBUG_FLAG_VERIFY_TRACING",\
    "FBE_RAID_GROUP_DEBUG_FLAG_MEDIC_ACTION_TRACING",\
    "FBE_RAID_GROUP_DEBUG_FLAG_REBUILD_NOTIFICATION_TRACING",\
    "FBE_RAID_GROUP_DEBUG_FLAG_QUIESCE_TRACING",\
    "FBE_RAID_GROUP_DEBUG_FLAG_DISABLE_UPDATE_NR_VALIDATION",\
    "FBE_RAID_GROUP_DEBUG_FLAG_LUN_PERCENT_REBUILT_NOTIFICATION_TRACING",\
    "FBE_RAID_GROUP_DEBUG_FLAG_NP_LOCK_TRACING",\
    "FBE_RAID_GROUP_DEBUG_FLAG_ENCRYPTION_TRACING",\
    "FBE_RAID_GROUP_DEBUG_FLAG_DRIVE_TIER_TRACING",\
    "FBE_RAID_GROUP_DEBUG_FLAG_EXTENDED_MEDIA_ERROR_HANDLING",\
    "FBE_RAID_GROUP_DEBUG_FLAG_RESERVED_IGNORE_SET",\
    "FBE_RAID_GROUP_DEBUG_FLAG_UNUSED1",\
    "FBE_RAID_GROUP_DEBUG_FLAG_UNUSED2",\
    "FBE_RAID_GROUP_DEBUG_FLAG_UNUSED3",\
    "FBE_RAID_GROUP_DEBUG_FLAG_UNUSED4",\
    "FBE_RAID_GROUP_DEBUG_FLAG_UNUSED5",\
    "FBE_RAID_GROUP_DEBUG_FLAGS_ERROR_TRACE_USER_IOTS_ERROR",\
    "FBE_RAID_GROUP_DEBUG_FLAGS_GENERATE_ERROR_TRACE",\
    "FBE_RAID_GROUP_DEBUG_FLAGS_ERROR_TRACE_MONITOR_IOTS_ERROR",\
    "FBE_RAID_GROUP_DEBUG_FLAG_FORCE_WRITE_VERIFY"

/*! @enum fbe_raid_group_flags_t
 *  
 *  @brief flags to indicate states of the raid group object
 *         related to I/O processing.
 */
typedef enum fbe_raid_group_flags_e
{
    /*! No flags set.
     */
    FBE_RAID_GROUP_FLAG_INVALID             = 0x00000000,

    /*! Raid group object has initialized enough to be able to respond to certain events. 
     */
    FBE_RAID_GROUP_FLAG_INITIALIZED         = 0x00000001, 

    /*! Raid Group object can process the event from the event queue
     */
    FBE_RAID_GROUP_FLAG_PROCESS_EVENT       = 0x00000002,

    FBE_RAID_GROUP_FLAG_LAST                = 0x00000004
}
fbe_raid_group_flags_t;

/*!*******************************************************************
 * @def FBE_RAID_GROUP_FLAG_STRINGS
 *********************************************************************
 * @brief Set of strings that correspond to the above flags.
 *
 *********************************************************************/
#define FBE_RAID_GROUP_FLAG_STRINGS\
    "FBE_RAID_GROUP_FLAG_INITIALIZED",\
    "FBE_RAID_GROUP_FLAG_PROCESS_EVENT",\
    "FBE_RAID_GROUP_FLAG_CHANGE_IN_PROGRESS",\
    "FBE_RAID_GROUP_FLAG_LAST"

/*! @enum fbe_raid_library_debug_flags_t 
 *  
 *  @brief Debug flags that allow run-time debug functions.
 *
 *  @note Typically these flags should only be set for unit or
 *        intergration testing since they could have SEVERE 
 *        side effects.
 */
typedef enum fbe_raid_library_debug_flags_e
{
    FBE_RAID_LIBRARY_DEBUG_FLAG_NONE                    = 0x00000000,

    /*! Log all siots that start and finish.
     */
    FBE_RAID_LIBRARY_DEBUG_FLAG_SIOTS_TRACING           = 0x00000001,

    /*! Log all fruts that start and finish.
     */
    FBE_RAID_LIBRARY_DEBUG_FLAG_FRUTS_TRACING           = 0x00000002,

    /*! Log data with each fruts we log.
     */
    FBE_RAID_LIBRARY_DEBUG_FLAG_FRUTS_DATA_TRACING      = 0x00000004,

    /*! Log information for rebuild requests.
     */
    FBE_RAID_LIBRARY_DEBUG_FLAG_REBUILD_TRACING         = 0x00000008,

    /*! Enable additional XOR error debug options
     */
    FBE_RAID_LIBRARY_DEBUG_FLAG_XOR_ERROR_TRACING       = 0x00000010,
    
    /*! Enable tracing of all siots that complete in error
     */
    FBE_RAID_LIBRARY_DEBUG_FLAG_SIOTS_ERROR_TRACING     = 0x00000020,    
    
    /*! Enable Debug checking of errors on sectors we are writing.
     */
    FBE_RAID_LIBRARY_DEBUG_FLAG_WRITE_SECTOR_CHECKING   = 0x00000040,    
    
    /*! Enable memory info tracing
     */
    FBE_RAID_LIBRARY_DEBUG_FLAG_MEMORY_INFO_TRACING     = 0x00000080,

    /*! Enable raid memory tracking
     * 
     *  @todo This is only temporary to help debug raid using the memory service.
     *        Longer term any memory debugging tools should be provided by the 
     *        memory service.
     */
    FBE_RAID_LIBRARY_DEBUG_FLAG_MEMORY_TRACKING         = 0x00000100,

    /*! Enable all memory tracing (very verbose!!)
     */
    FBE_RAID_LIBRARY_DEBUG_FLAG_MEMORY_TRACING          = 0x00000200,

    FBE_RAID_LIBRARY_DEBUG_FLAG_WRITE_JOURNAL_TRACING   = 0x00000400,
     
    /*! Enable XOR to validate data
     */
    FBE_RAID_LIBRARY_DEBUG_FLAG_XOR_VALIDATE_DATA       = 0x00000800,

    /*! Enable rdgen pattern data checking (read and write)
     */
    FBE_RAID_LIBRARY_DEBUG_FLAG_PATTERN_DATA_CHECKING   = 0x00001000,

    /*! Enable invalidated data checking (read and write)
     */
    FBE_RAID_LIBRARY_DEBUG_FLAG_INVALID_DATA_CHECKING   = 0x00002000,

    /*! Log information for rebuild logging requests.
     */
    FBE_RAID_LIBRARY_DEBUG_FLAG_REBUILD_LOG_TRACING     = 0x00004000,

    /*! This enable sending CRC error usurper to pdo in case of CRC errors
     * This is used in case we are injecting errors by logical error 
     * injection service but we still want to inform pdo that
     * CRC errors has occured.
     */
    FBE_RAID_LIBRARY_DEBUG_FLAG_SEND_CRC_ERR_TO_PDO     = 0x00008000,

    /*! Log information for verify requests.
     */
    FBE_RAID_LIBRARY_DEBUG_FLAG_VERIFY_TRACING          = 0x00010000,

    /*! Generate an `error' level trace when a I/O encounters an error.
     */
    FBE_RAID_LIBRARY_DEBUG_FLAG_ERROR_TRACE_SIOTS_ERROR = 0x00020000,

    /*! Dump the contents of the first and last metadata blocks when written
     */
    FBE_RAID_LIBRARY_DEBUG_FLAG_TRACE_PAGED_MD_WRITE_DATA = 0x00040000,

    /*! This flag allows us to force journaling of degraded writes without skipping
     * for single drive, all-parity-dead, and full-stripe writes.  This allows
     * tests for error handling such as agent_jay and agent_kay to run
     * without worrying about optimizing around journalling.
     */
    FBE_RAID_LIBRARY_DEBUG_FLAG_DISABLE_WRITE_LOG_SKIP  = 0x00080000,

    /*! This test flag allows us to turn off write logging completely.
     *  WARNING -- setting this exposes degraded arrays to incomplete write
     *  data corruption due to the RAID write hole!!!
     */
    FBE_RAID_LIBRARY_DEBUG_FLAG_DISABLE_WRITE_LOGGING   = 0x00100000,

    /*! Trace first and last 4 sectors of xfer as well as every 8 blocks.
     */
    FBE_RAID_LIBRARY_DEBUG_FLAG_TRACE_ALL_DATA          = 0x00200000,

    FBE_RAID_LIBRARY_DEBUG_FLAG_LAST                    = 0x00800000,

} fbe_raid_library_debug_flags_t;

/*!*******************************************************************
 * @def FBE_RAID_LIBRARY_DEBUG_FLAG_STRINGS
 *********************************************************************
 * @brief Set of strings that correspond to the above flags.
 *
 *********************************************************************/
#define FBE_RAID_LIBRARY_DEBUG_FLAG_STRINGS\
    "FBE_RAID_LIBRARY_DEBUG_FLAG_SIOTS_TRACING",\
    "FBE_RAID_LIBRARY_DEBUG_FLAG_FRUTS_TRACING",\
    "FBE_RAID_LIBRARY_DEBUG_FLAG_FRUTS_DATA_TRACING",\
    "FBE_RAID_LIBRARY_DEBUG_FLAG_REBUILD_TRACING",\
    "FBE_RAID_LIBRARY_DEBUG_FLAG_XOR_ERROR_TRACING",\
    "FBE_RAID_LIBRARY_DEBUG_FLAG_SIOTS_ERROR_TRACING",\
    "FBE_RAID_LIBRARY_DEBUG_FLAG_WRITE_SECTOR_CHECKING",\
    "FBE_RAID_LIBRARY_DEBUG_FLAG_MEMORY_INFO_TRACING",\
    "FBE_RAID_LIBRARY_DEBUG_FLAG_MEMORY_TRACKING",\
    "FBE_RAID_LIBRARY_DEBUG_FLAG_MEMORY_TRACING",\
    "FBE_RAID_LIBRARY_DEBUG_FLAG_WRITE_JOURNAL_TRACING",\
    "FBE_RAID_LIBRARY_DEBUG_FLAG_XOR_VALIDATE_DATA",\
    "FBE_RAID_LIBRARY_DEBUG_FLAG_PATTERN_DATA_CHECKING",\
    "FBE_RAID_LIBRARY_DEBUG_FLAG_INVALID_DATA_CHECKING",\
    "FBE_RAID_LIBRARY_DEBUG_FLAG_REBUILD_LOG_TRACING",\
    "FBE_RAID_LIBRARY_DEBUG_FLAG_SEND_CRC_ERR_TO_PDO",\
    "FBE_RAID_LIBRARY_DEBUG_FLAG_VERIFY_TRACING",\
    "FBE_RAID_LIBRARY_DEBUG_FLAG_ERROR_TRACE_SIOTS_ERROR",\
    "FBE_RAID_LIBRARY_DEBUG_FLAG_TRACE_PAGED_MD_WRITE_DATA",\
    "FBE_RAID_LIBRARY_DEBUG_FLAG_DISABLE_WRITE_LOG_SKIP",\
    "FBE_RAID_LIBRARY_DEBUG_FLAG_DISABLE_WRITE_LOGGING",\
    "FBE_RAID_LIBRARY_DEBUG_FLAG_TRACE_ALL_DATA",\
    "FBE_RAID_LIBRARY_DEBUG_FLAG_DISABLE_SEND_CRC_ERR_TO_PDO",\
    "FBE_RAID_LIBRARY_DEBUG_FLAG_LAST"
        
/***************************************************      
 *  Raid Group Object Interfaces                                  
 ***************************************************/             

/*!*******************************************************************
 * @struct fbe_raid_group_get_lun_verify_status_t
 *********************************************************************
 * @brief
 *  This structure contains the background verify status of a LUN
 *  in the RAID group.  It is used by the
 *  FBE_RAID_GROUP_CONTROL_CODE_GET_LUN_VERIFY_STATUS control code.
 *
 *********************************************************************/
typedef struct fbe_raid_group_get_lun_verify_status_s
{
    fbe_chunk_count_t   total_chunks;       // total number of chunks in the LUN extent
    fbe_chunk_count_t   marked_chunks;      // number of LUN extent chunks marked for verify
}
fbe_raid_group_get_lun_verify_status_t;

/*! @enum fbe_raid_group_local_state_e
 *  
 *  @brief Raid group local state which allows us to know where this raid group is
 *         in its processing of certain conditions.
 */
enum fbe_raid_group_local_state_e {
    /* Eval mark NR has 3 states: 
     *  Request: where a request has been issued to go 
     */
    FBE_RAID_GROUP_LOCAL_STATE_EVAL_MARK_NR_REQUEST = 0x0000000000000001,
    FBE_RAID_GROUP_LOCAL_STATE_EVAL_MARK_NR_STARTED = 0x0000000000000002,
    FBE_RAID_GROUP_LOCAL_STATE_EVAL_MARK_NR_DONE    = 0x0000000000000004,
    FBE_RAID_GROUP_LOCAL_STATE_EVAL_MARK_NR_MASK    = 0x0000000000000007,

    FBE_RAID_GROUP_LOCAL_STATE_EVAL_RL_REQUEST  = 0x0000000000000010,
    FBE_RAID_GROUP_LOCAL_STATE_EVAL_RL_STARTED  = 0x0000000000000020,
    FBE_RAID_GROUP_LOCAL_STATE_EVAL_RL_DONE     = 0x0000000000000040,
    FBE_RAID_GROUP_LOCAL_STATE_EVAL_RL_BROKEN   = 0x0000000000000080,
    FBE_RAID_GROUP_LOCAL_STATE_EVAL_RL_MASK     = 0x00000000000000F0,

    FBE_RAID_GROUP_LOCAL_STATE_CLEAR_RL_REQUEST = 0x0000000000000100,
    FBE_RAID_GROUP_LOCAL_STATE_CLEAR_RL_STARTED = 0x0000000000000200,
    FBE_RAID_GROUP_LOCAL_STATE_CLEAR_RL_DONE    = 0x0000000000000400,
    FBE_RAID_GROUP_LOCAL_STATE_CLEAR_RL_MASK    = 0x0000000000000700,

    FBE_RAID_GROUP_LOCAL_STATE_JOIN_REQUEST = 0x0000000000001000,
    FBE_RAID_GROUP_LOCAL_STATE_JOIN_STARTED = 0x0000000000002000,
    FBE_RAID_GROUP_LOCAL_STATE_JOIN_DONE    = 0x0000000000004000,
    FBE_RAID_GROUP_LOCAL_STATE_JOIN_MASK    = 0x0000000000007000,

    /*! This bit tells us we ran the activate condition as active.
     */
    FBE_RAID_GROUP_LOCAL_STATE_ACTIVATE_ACTIVE  = 0x0000000000010000,
    /*! This bit tells us we are in activate state.
     */
    FBE_RAID_GROUP_LOCAL_STATE_ACTIVATE  = 0x0000000000020000,

    /*! Thes bits are used to let us know we need to send notifications to the upper layers
     */
    FBE_RAID_GROUP_LOCAL_STATE_DATA_RECONSTRUCTION_STARTED  = 0x0000000000100000,
    FBE_RAID_GROUP_LOCAL_STATE_DATA_RECONSTRUCTION_ENDED    = 0x0000000000200000,
    FBE_RAID_GROUP_LOCAL_STATE_DATA_RECONSTRUCTION_MASK     = 0x0000000000300000,

    /*! This bit says we came from specialize, hibernate or fail state and must wait for drives to spin up before marking rl
     */
    FBE_RAID_GROUP_LOCAL_STATE_NEED_RL_DRIVE_WAIT     = 0x0000000000400000,


    FBE_RAID_GROUP_LOCAL_STATE_CHANGE_CONFIG_REQUEST  = 0x0000000001000000,
    FBE_RAID_GROUP_LOCAL_STATE_CHANGE_CONFIG_STARTED  = 0x0000000002000000,
    FBE_RAID_GROUP_LOCAL_STATE_CHANGE_CONFIG_DONE     = 0x0000000004000000,
    FBE_RAID_GROUP_LOCAL_STATE_CHANGE_CONFIG_ERROR    = 0x0000000008000000,
    FBE_RAID_GROUP_LOCAL_STATE_CHANGE_CONFIG_MASK     = 0x000000000F000000,

    /*! These bits indicate if we are in one of the EMEH processing modes
     */
    FBE_RAID_GROUP_LOCAL_STATE_EMEH_DEGRADED_STARTED  = 0x0000000010000000,
    FBE_RAID_GROUP_LOCAL_STATE_EMEH_PACO_STARTED      = 0x0000000020000000,
    FBE_RAID_GROUP_LOCAL_STATE_EMEH_INCREASE_STARTED  = 0x0000000040000000,
    FBE_RAID_GROUP_LOCAL_STATE_EMEH_RESTORE_STARTED   = 0x0000000080000000ULL,
    FBE_RAID_GROUP_LOCAL_STATE_EMEH_MASK              = 0x00000000F0000000ULL,

    /*! Mask of all our masks.  Does not include ...NEED_RL_DRIVE_WAIT since this must persist through state transition to activate
     */
    FBE_RAID_GROUP_LOCAL_STATE_ALL_MASKS = (FBE_RAID_GROUP_LOCAL_STATE_EVAL_MARK_NR_MASK |
                                            FBE_RAID_GROUP_LOCAL_STATE_EVAL_RL_MASK | 
                                            FBE_RAID_GROUP_LOCAL_STATE_CLEAR_RL_MASK | 
                                            FBE_RAID_GROUP_LOCAL_STATE_JOIN_MASK |
                                            FBE_RAID_GROUP_LOCAL_STATE_ACTIVATE_ACTIVE |
                                            FBE_RAID_GROUP_LOCAL_STATE_ACTIVATE |
                                            FBE_RAID_GROUP_LOCAL_STATE_DATA_RECONSTRUCTION_MASK |
                                            FBE_RAID_GROUP_LOCAL_STATE_CHANGE_CONFIG_MASK |
                                            FBE_RAID_GROUP_LOCAL_STATE_EMEH_MASK),

    FBE_RAID_GROUP_LOCAL_STATE_INVALID = 0xFFFFFFFFFFFFFFFFULL,
};
typedef fbe_u64_t fbe_raid_group_local_state_t;

/*!*******************************************************************
 * @struct fbe_raid_group_get_info_t
 *********************************************************************
 * @brief
 *  This structure is used with the
 *  FBE_RAID_GROUP_CONTROL_CODE_GET_INFO control code.
 *
 *********************************************************************/
#ifndef UEFI_ENV
typedef struct fbe_raid_group_get_info_s
{
    fbe_u32_t width; /*!< Number of drives in the raid group. */ 
    fbe_u32_t physical_width; /*! For R10 this is the actual # of physical drives. For other Raid types it is equal to width */
    fbe_lba_t capacity;  /*!< The user capacity of the raid group. */ 
    fbe_lba_t raid_capacity; /*!< Raid protected capacity. Includes user + metadata. */
    fbe_lba_t imported_blocks_per_disk; /*!< Total blocks imported by the raid group on every disk. */

    fbe_lba_t paged_metadata_start_lba; /*!< The lba where the paged metadata starts. */
    fbe_lba_t paged_metadata_capacity; /*!< Total capacity of paged metadata. */
    fbe_chunk_count_t total_chunks; /*!< Total chunks of paged */

    fbe_lba_t write_log_start_pba; /*!< The physical lba where the write_log starts. */
    fbe_lba_t write_log_physical_capacity; /*!< Total per disk capacity of write_log. */
    fbe_lba_t physical_offset; /*!< The offset on a disk  basis to get to physical address */

    /*! raid group debug flags to force different options.
     */
    fbe_raid_group_debug_flags_t debug_flags;

    /*! These are debug flags passed to the raid library.
     */
    fbe_raid_library_debug_flags_t library_debug_flags;

    /*! flags related to I/O processing.
     */
    fbe_raid_group_flags_t flags;

    /*! Flags related to I/O processing. 
     */
    fbe_base_config_flags_t base_config_flags;

    /*! Flags related to I/O processing. 
     */
    fbe_base_config_clustered_flags_t base_config_clustered_flags;

    /*! Flags related to I/O processing. 
     */
    fbe_base_config_clustered_flags_t base_config_peer_clustered_flags;

    /*! This is the type of raid group (user configurable).
     */
    fbe_raid_group_type_t raid_type;

    fbe_element_size_t          element_size; /*!< Blocks in each stripe element.*/
    fbe_block_size_t    exported_block_size; /*!< The logical block size that RAID exports to clients. */  
    fbe_block_size_t    imported_block_size; /*!< The imported physical block size. */
    fbe_block_size_t    optimal_block_size;  /*!< Number of blocks needed to avoid extra pre-reads. */

    /*! Total Stripe count.
     */
    fbe_block_count_t stripe_count;

    /*! Total sectors per stripe and does not include partity drive
     */
    fbe_block_count_t sectors_per_stripe;
    
    fbe_raid_position_bitmask_t bitmask_4k; /*!< Bitmask of positions that are 4k. */

    /*! array of flags, one for each disk in the RG.  When set to TRUE, signifies that the RG is 
     *  rebuild logging for the given disk.  Every I/O will be tracked and its chunk marked as NR when in
     *  this mode. 
     */
    fbe_bool_t b_rb_logging[FBE_RAID_MAX_DISK_ARRAY_WIDTH]; 

    /*!  Rebuild checkpoint - one for each disk in the RG */
    fbe_lba_t rebuild_checkpoint[FBE_RAID_MAX_DISK_ARRAY_WIDTH];

    /*! Verify checkpoint */
    fbe_lba_t ro_verify_checkpoint;    
    fbe_lba_t rw_verify_checkpoint;
    fbe_lba_t error_verify_checkpoint;
    fbe_lba_t journal_verify_checkpoint;
    fbe_lba_t incomplete_write_verify_checkpoint;
    fbe_lba_t system_verify_checkpoint;
    
    /*! NP metadata flags, values defined in fbe_raid_group_np_flag_status_e */    
    fbe_u8_t  raid_group_np_md_flags;
    /*! NP metadata flags, values defined in fbe_raid_group_np_extended_flag_status_e */    
    fbe_u64_t raid_group_np_md_extended_flags;

    /*! The size that lun should be aligned to  
     * currently lun_align_size = chunk_size*num_data_disks.
     */
    fbe_lba_t       lun_align_size;

    fbe_u16_t        num_data_disk; /*!< Number of data drives does not include redundancy. */

    fbe_raid_group_local_state_t       local_state; /*state of the raid group - locally */
    fbe_raid_group_local_state_t       peer_state;  /*state of the raid group - on the peer */

    /* indicates whether the SP is active or passive for the object */
    fbe_metadata_element_state_t metadata_element_state; 

    /*! Count for number of times paged metadata region has been verified */
    fbe_u32_t               paged_metadata_verify_pass_count;

    /*! elements per parity stripe */
    fbe_elements_per_parity_t elements_per_parity_stripe;
    
    /*! user private*/
    fbe_bool_t              user_private; 
    fbe_bool_t b_is_event_q_empty; /* Indciates if the rg objects event q is empty or not. */
    fbe_u32_t background_op_seconds; /*! Total seconds Background op is running for. */

    fbe_lba_t rekey_checkpoint; /*! Current rekey checkpoint. */

    fbe_base_config_encryption_mode_t encryption_mode;
    fbe_base_config_encryption_state_t encryption_state;
}
fbe_raid_group_get_info_t;
#endif /* #ifndef UEFI_ENV */

/*!*******************************************************************
 * @struct fbe_raid_group_get_io_info_t
 *********************************************************************
 * @brief   Get the I/O information about this raid group:
 *              o Outstanding I/O count
 *              o Quiesced I/O count
 *              o Is quiesced
 *
 *********************************************************************/
#ifndef UEFI_ENV
typedef struct fbe_raid_group_get_io_info_s
{
    fbe_u32_t   outstanding_io_count;   /*!< Number of I/Os outstanding */
    fbe_bool_t  b_is_quiesced;          /*!< Is the raid group currently quiesced */
    fbe_u32_t   quiesced_io_count;      /*!< Number of I/Os that have been quiesced */
}
fbe_raid_group_get_io_info_t;
#endif /* #ifndef UEFI_ENV */

/*!*******************************************************************
 * @struct fbe_raid_group_class_get_info_t
 *********************************************************************
 * @brief
 *  This structure is used with the
 *  FBE_RAID_GROUP_CONTROL_CODE_CLASS_GET_INFO control code.
 * 
 *  In the below either exported capacity or single_drive_capacity
 *  must be an input or we will get an error.
 *
 *********************************************************************/
typedef struct fbe_raid_group_class_get_info_s
{
    /*! Input: Number of drives in the raid group.
     */
    fbe_u32_t width;

    /*! Output: number of data disks (so on R5 for example this is width-1)
     */
    fbe_u16_t data_disks;

    /*! The overall capacity the raid group is exporting.
     * If this is not FBE_LBA_INVALID, then it is an input. 
     * Otherwise it is an output. 
     * This is imported capacity - the metadata capacity. 
     */
    fbe_lba_t exported_capacity;

    /*! Output: The overall capacity the raid group is importing. 
     *          This is exported capacity + the metadata capacity. 
     */
    fbe_lba_t imported_capacity;

    /* This is the size imported of a single drive. 
     * If this is not FBE_LBA_INVALID, then it is an input. 
     * Otherwise it is an output. 
     */
    fbe_lba_t single_drive_capacity;

    /*! Input: This is the type of raid group (user configurable).
     */
    fbe_raid_group_type_t raid_type;

    /*! Input: Determines if the user selected bandwidth or iops  
     */
    fbe_bool_t b_bandwidth;

    /*! Output: Size of each data element.
     */
    fbe_element_size_t        element_size;

    /*! Output: Number of elements (vertically) before parity rotates.
     */
    fbe_elements_per_parity_t     elements_per_parity;
}
fbe_raid_group_class_get_info_t;


/*!*******************************************************************
 * @struct fbe_raid_group_map_info_t
 *********************************************************************
 * @brief
 *  This structure is used with the
 *  FBE_RAID_GROUP_CONTROL_CODE_MAP_LBA control code.
 *  FBE_RAID_GROUP_CONTROL_CODE_MAP_PBA control code.
 *
 *********************************************************************/
#ifndef UEFI_ENV
typedef struct fbe_raid_group_map_info_s
{
    /*! This is the only input that needs to get filled in by the user.  
     */
    fbe_lba_t lba; 

    /* The rest of the fields are outputs.
     */
    fbe_lba_t pba; /*!< Output pba or input lba to map. */

    fbe_chunk_index_t chunk_index; /*!< Chunk in paged. */
    fbe_lba_t metadata_lba; /*! Logical Address of this metadata chunk in the paged metadata. */
    fbe_raid_position_t data_pos;  /*!< Index of data position in array.  */
    fbe_raid_position_t parity_pos; /*!< Index of parity position in array. */
    fbe_lba_t offset; /*!< Offset on raid group's downstream edge. */

    fbe_lba_t original_lba; /*!< For R10, this is the top level RG's lba. Otherwise not used. */
    fbe_raid_position_t position;
}
fbe_raid_group_map_info_t;
#endif /* #ifndef UEFI_ENV */

typedef struct fbe_raid_group_class_set_library_errors_s
{
    /*! Percentage of time to inject errors.
     */
    fbe_u32_t inject_percentage;
    /*! FBE_TRUE to inject only on user rgs.  
     *  FBE_FALSE to inject on system and user RGs. 
     */
    fbe_bool_t b_only_inject_user_rgs;
}
fbe_raid_group_class_set_library_errors_t;
/*!*******************************************************************
 * @struct fbe_raid_group_raid_library_debug_payload_t
 *********************************************************************
 *
 * @brief   This structure is used with the following control codes:
 *          o   FBE_RAID_GROUP_CONTROL_CODE_GET_RAID_LIBRARY_DEBUG_FLAGS
 *          o   FBE_RAID_GROUP_CONTROL_CODE_SET_RAID_LIBRARY_DEBUG_FLAGS
 *          o   FBE_RAID_GROUP_CONTROL_CODE_CLASS_GET_RAID_LIBRARY_DEBUG_FLAGS
 *          o   FBE_RAID_GROUP_CONTROL_CODE_CLASS_SET_RAID_LIBRARY_DEBUG_FLAGS 
 *          Currently there is only (1) field in this payload but it could
 *          be expanded in the future. 
 *
 *********************************************************************/
typedef struct fbe_raid_group_raid_library_debug_payload_s
{
    fbe_raid_library_debug_flags_t  raid_library_debug_flags; /*! raid library debug flags */
}
fbe_raid_group_raid_library_debug_payload_t;

/*!*******************************************************************
 * @struct fbe_raid_group_raid_group_debug_payload_t
 *********************************************************************
 *
 * @brief   This structure is used with the following control codes:
 *          o   FBE_RAID_GROUP_CONTROL_CODE_GET_RAID_GROUP_DEBUG_FLAGS
 *          o   FBE_RAID_GROUP_CONTROL_CODE_SET_RAID_GROUP_DEBUG_FLAGS
 *          o   FBE_RAID_GROUP_CONTROL_CODE_CLASS_GET_RAID_GROUP_DEBUG_FLAGS
 *          o   FBE_RAID_GROUP_CONTROL_CODE_CLASS_SET_RAID_GROUP_DEBUG_FLAGS
 *          Currently there is only (1) field in this payload but it could
 *          be expanded in the future.  
 *
 *********************************************************************/
typedef struct fbe_raid_group_raid_group_debug_payload_s
{
    fbe_raid_group_debug_flags_t    raid_group_debug_flags; /*! raid group debug flags */
}
fbe_raid_group_raid_group_debug_payload_t;

/*!*******************************************************************
 * @struct fbe_raid_group_default_debug_flag_payload_t
 *********************************************************************
 *
 * @brief   This structure is used with the following control codes:
 *          o   FBE_RAID_GROUP_CONTROL_CODE_SET_DEFAULT_DEBUG_FLAGS
 *          o   FBE_RAID_GROUP_CONTROL_CODE_GET_DEFAULT_DEBUG_FLAGS
 *
 *********************************************************************/
typedef struct fbe_raid_group_default_debug_flag_payload_s
{
    fbe_raid_group_debug_flags_t    user_debug_flags; /*! debug flags for user rgs */
    fbe_raid_group_debug_flags_t    system_debug_flags; /*! debug flags for system rgs */
}
fbe_raid_group_default_debug_flag_payload_t;

/*!*******************************************************************
 * @struct fbe_raid_group_default_library_flag_payload_t
 *********************************************************************
 *
 * @brief   This structure is used with the following control codes:
 *          o   FBE_RAID_GROUP_CONTROL_CODE_GET_DEFAULT_LIBRARY_FLAGS
 *          o   FBE_RAID_GROUP_CONTROL_CODE_SET_DEFAULT_LIBRARY_FLAGS
 *
 *********************************************************************/
typedef struct fbe_raid_group_default_library_flag_payload_s
{
    fbe_raid_library_debug_flags_t    user_debug_flags; /*! library flags for user rgs */
    fbe_raid_library_debug_flags_t    system_debug_flags; /*! library flags for system rgs */
}
fbe_raid_group_default_library_flag_payload_t;


/*!*******************************************************************
 * @struct fbe_raid_group_get_power_saving_info_t
 *********************************************************************
 *
 * @brief   This structure is used with the following control codes:
 *          o   FBE_RAID_GROUP_CONTROL_CODE_GET_POWER_SAVING_PARAMETERS
 *
 *********************************************************************/
typedef struct fbe_raid_group_get_power_saving_info_s{
    fbe_u64_t   max_raid_latency_time_is_sec;/*! how many sceonds the RAID user is wiling to wait for the object ot become ready after saving power*/
    fbe_bool_t  power_saving_enabled;/*! is power saving enabled or not*/
    fbe_u64_t   idle_time_in_sec;/*! how long to wait before starting to save power*/
}fbe_raid_group_get_power_saving_info_t;



/*!*******************************************************************
 * @struct fbe_raid_group_raid_lib_error_testing_stats_t
 *********************************************************************
 * @brief Defines elements which will represent the stastitcs of 
 *        unexpected error path encountered while processing I/O
 *        The values of fields are having significance only if unexpected
 *        error testing is enabled for the raid library.
 *
 *********************************************************************/
typedef struct fbe_raid_group_raid_lib_error_testing_stats_s
{
    fbe_u32_t error_count;
} 
fbe_raid_group_raid_lib_error_testing_stats_t;


/*!*******************************************************************
 * @struct fbe_raid_group_raid_library_error_testing_stats_payload_t
 *********************************************************************
 *
 * @brief   This structure is used with the following control codes:
 *          o   FBE_RAID_GROUP_CONTROL_CODE_CLASS_GET_RAID_GROUP_ERROR_TESTING_STATS
 *
 *********************************************************************/
typedef struct fbe_raid_group_raid_library_error_testing_stats_payload_s
{
    fbe_raid_group_raid_lib_error_testing_stats_t error_stats;
}
fbe_raid_group_raid_library_error_testing_stats_payload_t;

/*!*******************************************************************
 * @struct fbe_raid_group_class_set_options_t
 *********************************************************************
 * @brief   This structure is used with the following control codes: 
 *          o   FBE_RAID_GROUP_CONTROL_CODE_CLASS_GET_RAID_GROUP_ERROR_TESTING_STATS
 *
 *********************************************************************/
#ifndef UEFI_ENV
typedef struct fbe_raid_group_class_set_options_s
{
    fbe_time_t user_io_expiration_time; /*!< Time for user I/Os to expire. */
    fbe_time_t paged_metadata_io_expiration_time; /*!< Time for metadata I/Os to expire. */
    fbe_u32_t encrypt_vault_wait_time; /*! Time to wait for no io on vault before encrypting. */
    fbe_u32_t encryption_blob_blocks; /*! Size of blob used to scan paged metadata.  */
}
fbe_raid_group_class_set_options_t;
#endif

/*!*******************************************************************
 * @struct fbe_raid_group_get_zero_checkpoint_raid_extent_s
 *********************************************************************
 * @brief   This structure is used with the following control codes: 
 *          o   FBE_RAID_GROUP_CONTROL_CODE_GET_ZERO_CHECKPOINT_RAID_EXTENT
 *
 *********************************************************************/
typedef struct fbe_raid_group_get_zero_checkpoint_raid_extent_s
{
    fbe_lba_t           zero_checkpoint; /*!< zero checkpoint for the lu extent. */
    fbe_lba_t           lun_capacity;   /*< lun capacity */
    fbe_lba_t           lun_offset;     /*< lun offset within RG */    
}
fbe_raid_group_get_zero_checkpoint_raid_extent_t;

/*!*******************************************************************
 * @struct fbe_raid_group_get_zero_checkpoint_downstream_object_s
 *********************************************************************
 * @brief   This structure is used with the following control codes: 
 *          o   FBE_RAID_GROUP_CONTROL_CODE_GET_ZERO_CHECKPOINT_FOR_DOWNSTREAM_OBJECT
 *
 *********************************************************************/
#ifndef UEFI_ENV
typedef struct fbe_raid_group_get_zero_checkpoint_downstream_object_s
{
    fbe_lba_t    zero_checkpoint[FBE_RAID_MAX_DISK_ARRAY_WIDTH]; /*!< zero checkpoint for individual downstream object. */
}
fbe_raid_group_get_zero_checkpoint_downstream_object_t;
#endif

/*!*******************************************************************
 * @struct fbe_raid_group_set_rb_checkpoint_s
 *********************************************************************
 * @brief   This structure is used with the following control codes (for testing purposes): 
 *          o   FBE_RAID_GROUP_CONTROL_CODE_SET_RB_CHECKPOINT
 *
 *********************************************************************/
typedef struct fbe_raid_group_set_rb_checkpoint_s
{
    /*! Disk position to set rebuild logging checkpoint. 
     */
    fbe_u32_t position;

    /*! Rebuild logging checkpoint to set for disk position. 
     */
    fbe_lba_t rebuild_checkpoint;
}
fbe_raid_group_set_rb_checkpoint_t;


/*!*******************************************************************
 * @struct fbe_raid_group_set_verify_checkpoint_s
 *********************************************************************
 * @brief   This structure is used with the following control codes (for testing purposes): 
 *          o   FBE_RAID_GROUP_CONTROL_CODE_SET_VERIFY_CHECKPOINT
 *
 *********************************************************************/
#ifndef UEFI_ENV
typedef struct fbe_raid_group_set_verify_checkpoint_s
{
    /*! Type of verify request. 
     */
    fbe_raid_verify_flags_t verify_flags;

    /*! Verify checkpoint. 
     */
    fbe_lba_t verify_checkpoint;
}
fbe_raid_group_set_verify_checkpoint_t;

/*!*******************************************************************
 * @struct fbe_raid_group_get_statistics_t
 *********************************************************************
 * @brief Fetch stats on rg.
 *
 *********************************************************************/
typedef struct fbe_raid_group_get_statistics_s
{
    fbe_raid_library_statistics_t raid_library_stats;
    fbe_u32_t total_packets;
    fbe_u64_t outstanding_io_count;               /*!< Number of I/Os currently in flight. */
    fbe_u32_t outstanding_io_credits;              /*!< Current outstanding operations.*/
    fbe_u32_t queue_length[FBE_PACKET_PRIORITY_LAST - 1];
}
fbe_raid_group_get_statistics_t;

/*!*******************************************************************
 * @struct fbe_raid_group_set_encryption_mode_t
 *********************************************************************
 * @brief Usurper to allow RG to proceed with encryption.
 *
 *********************************************************************/
typedef struct fbe_raid_group_set_encryption_mode_s
{
    fbe_base_config_encryption_state_t encryption_state;
}
fbe_raid_group_set_encryption_mode_t;

#endif

typedef struct fbe_raid_group_get_disk_pos_by_server_id_s
{
    fbe_object_id_t server_id;
    fbe_u32_t position;
}fbe_raid_group_get_disk_pos_by_server_id_t;

  /*!**********************************************************************
 * @enum fbe_raid_group_background_op_e
 *  
 *  @brief Background operations supported by raid group object.
 *         Each operation represent with specific bit value.
 *
 *
*************************************************************************/
typedef enum fbe_raid_group_background_op_e
{
    FBE_RAID_GROUP_BACKGROUND_OP_NONE                       = 0x0000,
    FBE_RAID_GROUP_BACKGROUND_OP_METADATA_REBUILD           = 0x0001,
    FBE_RAID_GROUP_BACKGROUND_OP_REBUILD                    = 0x0002,
    FBE_RAID_GROUP_BACKGROUND_OP_ERROR_VERIFY               = 0x0004,
    FBE_RAID_GROUP_BACKGROUND_OP_RW_VERIFY                  = 0x0008,
    FBE_RAID_GROUP_BACKGROUND_OP_RO_VERIFY                  = 0x0010,
    FBE_RAID_GROUP_BACKGROUND_OP_INCOMPLETE_WRITE_VERIFY    = 0x0020,
    FBE_RAID_GROUP_BACKGROUND_OP_SYSTEM_VERIFY              = 0x0040, 
    FBE_RAID_GROUP_BACKGROUND_OP_ENCRYPTION_REKEY           = 0x0080,    
    FBE_RAID_GROUP_BACKGROUND_OP_VERIFY_ALL           = (FBE_RAID_GROUP_BACKGROUND_OP_ERROR_VERIFY |
                                                         FBE_RAID_GROUP_BACKGROUND_OP_RW_VERIFY |
                                                         FBE_RAID_GROUP_BACKGROUND_OP_RO_VERIFY |
                                                         FBE_RAID_GROUP_BACKGROUND_OP_INCOMPLETE_WRITE_VERIFY |
                                                         FBE_RAID_GROUP_BACKGROUND_OP_SYSTEM_VERIFY),
    FBE_RAID_GROUP_BACKGROUND_OP_ALL                  = 0x00FF, /* bitmask of all operations */
}fbe_raid_group_background_op_t;


  /*!**********************************************************************
 * @enum fbe_raid_group_background_op_e
 *  
 *  @brief Background operations supported by raid group object.
 *         Each operation represent with specific bit value.
 *
 *
*************************************************************************/
typedef enum fbe_raid_group_data_reconstrcution_notification_op_e
{
    DATA_RECONSTRUCTION_START,
    DATA_RECONSTRUCTION_END,
    DATA_RECONSTRUCTION_IN_PROGRESS,

}fbe_raid_group_data_reconstrcution_notification_op_t;

/*!**********************************************************************
 * @struct fbe_raid_group_reconstruction_notification_data_s
 *  
 *  @brief we use this structure to pass a notification together with 
 *  the FBE_NOTIFICATION_TYPE_DATA_RECONSTRUCTION type.
 It can be used for copy_to/ rebuild/ verify and so on
 *
 *
*************************************************************************/
typedef struct fbe_raid_group_reconstruction_notification_data_s
{
    fbe_raid_group_data_reconstrcution_notification_op_t state;
    fbe_u32_t   percent_complete;/*how much did we progress*/
}fbe_raid_group_reconstruction_notification_data_t;
/*************************************************** 
 *  End Raid Group Object Interfaces
 ***************************************************/
typedef fbe_u8_t fbe_raid_group_np_metadata_flags_t;
typedef fbe_u64_t fbe_raid_group_np_metadata_extended_flags_t;
/*!*******************************************************************
 * @def FBE_RAID_GROUP_MAX_PAGED_METADATA_METADATA_SLOTS
 *********************************************************************
 * @brief Maximum number of paged metadata metadata records
 *
 *********************************************************************/
#define FBE_RAID_GROUP_MAX_PAGED_METADATA_METADATA_SLOTS    14  

/*!*******************************************************************
 * @def FBE_RAID_GROUP_MAX_REBUILD_POSITIONS
 *********************************************************************
 * @brief Maximum number of disks that can be degraded/rebuilding at 
 *        at time.  
 *
 *********************************************************************/
#define FBE_RAID_GROUP_MAX_REBUILD_POSITIONS    2 


typedef fbe_u64_t fbe_raid_group_clustered_flags_t;

#ifndef UEFI_ENV
/* Don't break UEFI build */
/*!****************************************************************************
 * @struct fbe_raid_group_metadata_memory_s
 *
 * @brief
 *    This structure contains metadata memory information.
 *
 ******************************************************************************/
typedef struct fbe_raid_group_metadata_memory_s
{
    fbe_base_config_metadata_memory_t   base_config_metadata_memory; /* MUST be first */
    fbe_raid_group_clustered_flags_t    flags;
    fbe_raid_position_bitmask_t         failed_drives_bitmap;
    fbe_raid_position_bitmask_t         failed_ios_pos_bitmap;
    fbe_u16_t                           num_slf_drives;
    fbe_raid_position_bitmask_t         disabled_pos_bitmap;
    fbe_lba_t                           blocks_rebuilt[FBE_RAID_GROUP_MAX_REBUILD_POSITIONS];
    fbe_lba_t                           capacity_expansion_blocks; /*!< Amount to expand capacity by. */
}
fbe_raid_group_metadata_memory_t;
#endif

/*!****************************************************************************
 * @struct fbe_raid_group_rebuild_checkpoint_info_t
 ******************************************************************************
 * @brief
 *  This structure contains rebuild checkpoint information.  This consists of 
 *  the checkpoint itself and the disk position that the checkpoint corresponds
 *  to.  
 *
 ******************************************************************************/
#pragma pack(1)
typedef struct fbe_raid_group_rebuild_checkpoint_info_s
{
    /*! Rebuild checkpoint */
    fbe_lba_t checkpoint;

    /*! Disk position that the rebuild checkpoint corresponds to */
    fbe_u32_t position;
}
fbe_raid_group_rebuild_checkpoint_info_t;
#pragma pack()


/*!****************************************************************************
 * @struct fbe_raid_group_rebuild_nonpaged_info_t
 ******************************************************************************
 * @brief
 *  This structure contains the rebuild log bitmask and checkpoint
 *  info in the non-paged metadata for the RAID object.
 *
 ******************************************************************************/
#ifndef UEFI_ENV
#pragma pack(1)
typedef struct fbe_raid_group_rebuild_nonpaged_info_s
{
    /*! bitmask of flags, one for each disk in the RG. 
     *  When set to TRUE, signifies that the RG is rebuild logging for the given disk.
     *  Every I/O will be tracked and its chunk marked as NR when in this mode.
     */
    fbe_raid_position_bitmask_t rebuild_logging_bitmask;

    /*! Rebuild checkpoint info (checkpoint and position) */
    fbe_raid_group_rebuild_checkpoint_info_t rebuild_checkpoint_info[FBE_RAID_GROUP_MAX_REBUILD_POSITIONS];

}
fbe_raid_group_rebuild_nonpaged_info_t;
#pragma pack()
#endif

typedef fbe_u32_t fbe_raid_group_np_encryption_flags_t;
/*!*******************************************************************
 * @enum fbe_raid_group_encryption_flags_t
 *********************************************************************
 * @brief Flags to indicate state of encryption.
 *
 *********************************************************************/
typedef enum fbe_raid_group_encryption_flags_e
{
    /*! No flags set.
     */
    FBE_RAID_GROUP_ENCRYPTION_FLAG_INVALID                 = 0x00000000,

    /*! The paged needs to be re-written. 
     */
    FBE_RAID_GROUP_ENCRYPTION_FLAG_PAGED_NEEDS_RECONSTRUCT = 0x00000001, 

    /*! We told the PVD to encrypt its paged.
     */
    FBE_RAID_GROUP_ENCRYPTION_FLAG_PVD_NOTIFIED     = 0x00000002,

    FBE_RAID_GROUP_ENCRYPTION_FLAG_PAGED_ENCRYPTED         = 0x00000004,

    FBE_RAID_GROUP_ENCRYPTION_FLAG_LAST                    = 0x00000008
}
fbe_raid_group_encryption_flags_t;

/*!****************************************************************************
 * @struct fbe_raid_group_encryption_nonpaged_info_t
 ******************************************************************************
 * @brief
 *  Information to save in nonpaged about encryption..
 *
 ******************************************************************************/
#ifndef UEFI_ENV
#pragma pack(1)
typedef struct fbe_raid_group_encryption_nonpaged_info_s
{
    /*! Encryption rekey related information.  */
    fbe_lba_t rekey_checkpoint;
    fbe_u16_t unused; /*!< Previously used for rekey position. */
    fbe_raid_group_np_encryption_flags_t flags;
}
fbe_raid_group_encryption_nonpaged_info_t;
#pragma pack()
#endif

/*!*******************************************************************
* @enum fbe_drive_performance_tier_type_t
*********************************************************************
* @brief Enum to indicate the raid group's minimum drive tier
*
*********************************************************************/
typedef enum fbe_drive_performance_tier_type_e
{
    FBE_DRIVE_PERFORMANCE_TIER_TYPE_UNINITIALIZED    = 0,
    FBE_DRIVE_PERFORMANCE_TIER_TYPE_INVALID          = 1,
    FBE_DRIVE_PERFORMANCE_TIER_TYPE_7K               = 32, 
    FBE_DRIVE_PERFORMANCE_TIER_TYPE_10K              = 64,
    FBE_DRIVE_PERFORMANCE_TIER_TYPE_15K              = 96,
    FBE_DRIVE_PERFORMANCE_TIER_TYPE_LAST             = 97,
}
fbe_drive_performance_tier_type_t;

typedef fbe_u16_t fbe_drive_performance_tier_type_np_t;

/*!*******************************************************************
 * @struct fbe_raid_group_get_drive_info_downstream_s
 *********************************************************************
 * @brief   This structure is used to gather the minimum drive tier
 *          of the raid group
 *
 *********************************************************************/
#ifndef UEFI_ENV
typedef struct fbe_raid_group_get_drive_tier_downstream_s
{
    fbe_drive_performance_tier_type_np_t    drive_tier[FBE_RAID_MAX_DISK_ARRAY_WIDTH]; /* drive info for each pdo in rg */
}
fbe_raid_group_get_drive_tier_downstream_t;
#endif

#ifndef UEFI_ENV
typedef struct fbe_get_lowest_drive_tier 
{
    fbe_drive_performance_tier_type_np_t    drive_tier;
} fbe_get_lowest_drive_tier_t;
#endif

/*!*******************************************************************
 * @struct fbe_raid_group_get_wear_level_s
 *********************************************************************
 * @brief   This structure is used to get the wear level information
 *          of the raid group
 *
 *********************************************************************/
#ifndef UEFI_ENV
typedef struct fbe_raid_group_get_wear_level_s
{
    fbe_u64_t max_pe_cycle;
    fbe_u64_t current_pe_cycle;
    fbe_u64_t power_on_hours;

} fbe_raid_group_get_wear_level_t;
#endif

/*!*******************************************************************
 * @struct fbe_raid_group_get_drive_wear_level_downstream_s
 *********************************************************************
 * @brief   This structure is used to gather the maximum wear level
 *          of the drives in the raid group
 *
 *********************************************************************/
#ifndef UEFI_ENV
typedef struct fbe_raid_group_get_wear_level_downstream_s
{
    fbe_u32_t max_wear_level_percent;
    fbe_u8_t edge_index;
    fbe_u64_t max_erase_count;
    fbe_u64_t eol_PE_cycles;
    fbe_u64_t power_on_hours;
    fbe_raid_group_get_wear_level_t drive_wear_level[FBE_RAID_MAX_DISK_ARRAY_WIDTH]; /* drive info for each drive in rg */
}
fbe_raid_group_get_wear_level_downstream_t;
#endif


/*!****************************************************************************
 * @struct fbe_raid_group_nonpaged_metadata_t
 ******************************************************************************
 * @brief
 *  This structure contains the persistent, non-paged metadata for the RAID 
 *  object, it uses vault memory to store these data.
 *
 * @note
 * Make sure you use correct interface for accessing nonpaged metadata for
 * using this data structure.
 *
 * fbe_raid_group_peer_lost_trigger_pmd_reconstruct_if_needed() depends on the
 * checkpoints defined here. If there are any new checkpoints added here then
 * this function needs to be updated accordingly.
 *
 ******************************************************************************/
#ifndef UEFI_ENV
#pragma pack(1)
typedef struct fbe_raid_group_nonpaged_metadata_t
{
    /*! nonpaged metadata which is common to all the objects derived from
     *  the base config.
     */
    fbe_base_config_nonpaged_metadata_t base_config_nonpaged_metadata;  /* MUST be first */

    /*!  rebuild logging and degraded information */ 
    fbe_raid_group_rebuild_nonpaged_info_t  rebuild_info; 

    /*! The verify checkpoint. This checkpoint represents the lba of any given
     * disk in the RAID extent from which the verify operation still needs to 
     * be done. 
     */
    fbe_lba_t ro_verify_checkpoint;
    fbe_lba_t rw_verify_checkpoint;
    fbe_lba_t error_verify_checkpoint;
    fbe_lba_t journal_verify_checkpoint; 
    fbe_lba_t incomplete_write_verify_checkpoint;    
    fbe_lba_t system_verify_checkpoint;  

    /*! Chunk information for the paged metadata - the paged metadata metadata */
    fbe_raid_group_paged_metadata_t paged_metadata_metadata[FBE_RAID_GROUP_MAX_PAGED_METADATA_METADATA_SLOTS];

    /*! Flags to control various ongoing processes, values defined in fbe_raid_group_np_flag_status_e */
    fbe_raid_group_np_metadata_flags_t  raid_group_np_md_flags;

    /*! Bitmask identifying the set of disk(s) that experienced glitch. */
    fbe_raid_position_bitmask_t         glitching_disks_bitmask;

    /*! Encryption rekey related information.  */
    fbe_raid_group_encryption_nonpaged_info_t encryption;

    /*! Lowest drive performance tier to report */
    fbe_drive_performance_tier_type_np_t drive_tier;

    /*! Flags to control various ongoing processes, values defined in fbe_raid_group_np_extended_flag_status_e */
    /*!Since we ran out of space in the fbe_raid_group_np_flag_status_e these extended flag are being added */
    fbe_raid_group_np_metadata_extended_flags_t  raid_group_np_md_extended_flags;

    /*NOTES!!!!!!*/
    /*non-paged metadata versioning requires that any modification for this data structure should 
      pay attention to version difference between disk and memory. Please note that:
      1) the data structure should be appended only (after MCR released first version)
      2) the metadata verify conditon in its monitor will detect version difference, The
         new version developer is requried to set its defualt value when the new version
         software detects old version data in the metadata verify condition of specialize state
      3) database service maintain the committed metadata size, so any modification please
         invalidate the disk
    */
}
fbe_raid_group_nonpaged_metadata_t;
#pragma pack()


/*FBE_RAID_GROUP_CONTROL_CODE_GET_DEGRADED_INFO*/
typedef struct fbe_raid_group_get_degraded_info_s{
    fbe_u32_t        rebuild_percent;
    fbe_bool_t      b_is_raid_group_degraded;
}fbe_raid_group_get_degraded_info_t;
#endif /* #ifndef UEFI_ENV */


/*!*******************************************************************
 * @enum fbe_raid_group_error_precedence_t
 *********************************************************************
 * @brief
 *   Enumeration of the different error precedence.  This determines
 *   which error is reported for a multi-subrequests I/O. They are
 *   enumerated from lowest to highest weight.
 *
 *********************************************************************/
typedef enum fbe_raid_group_error_precedence_e
{
    FBE_RAID_GROUP_ERROR_PRECEDENCE_NO_ERROR           = 0,
    FBE_RAID_GROUP_ERROR_PRECEDENCE_INVALID_REQUEST    = 10,
    FBE_RAID_GROUP_ERROR_PRECEDENCE_MEDIA_ERROR        = 20,
    FBE_RAID_GROUP_ERROR_PRECEDENCE_NOT_READY          = 30,
    FBE_RAID_GROUP_ERROR_PRECEDENCE_ABORTED            = 40,
    FBE_RAID_GROUP_ERROR_PRECEDENCE_TIMEOUT            = 50,
    FBE_RAID_GROUP_ERROR_PRECEDENCE_DEVICE_DEAD        = 60,
    FBE_RAID_GROUP_ERROR_PRECEDENCE_UNKNOWN            = 100,
} fbe_raid_group_error_precedence_t;

/*!*******************************************************************
 * @struct fbe_raid_group_get_rebuild_status_s
 *********************************************************************
 * @brief   This structure is used with the following control codes: 
 *          o   FBE_RAID_GROUP_CONTROL_CODE_GET_REBUILD_STATUS
 *
 *********************************************************************/
typedef struct fbe_raid_group_rebuild_status_s
{
    fbe_lba_t           rebuild_checkpoint; /*!< rebuild checkpoint. */
    fbe_u32_t           rebuild_percentage; /*!< rebuild percentage complete*/
}
fbe_raid_group_rebuild_status_t;

/*!*******************************************************************
 * @struct fbe_raid_group_get_lun_percent_rebuilt_t
 *********************************************************************
 * @brief   This structure is used with the following control codes: 
 *          o   FBE_RAID_GROUP_CONTROL_CODE_GET_LUN_PERCENT_REBUILT
 *
 *********************************************************************/
typedef struct fbe_raid_group_get_lun_percent_rebuilt_s
{
    fbe_lba_t   lun_offset;             /*!< Supplied by lun - lun start offset */
    fbe_lba_t   lun_capacity;           /*!< Supplied by lun - lun capacity */

    fbe_lba_t   lun_rebuild_checkpoint; /*!< logical rebuild checkpoint for lun */
    fbe_u32_t   lun_percent_rebuilt;    /*!< rebuild percentage complete*/
    fbe_u32_t   debug_flags;            /*!< debug flags */
    fbe_bool_t  is_rebuild_in_progress; /*!< 1 : rebuild is in progress 0: not rebuilding*/
}
fbe_raid_group_get_lun_percent_rebuilt_t;

/* FBE_RAID_GROUP_CONTROL_CODE_SET_BG_OP_SPEED */
typedef struct fbe_raid_group_control_set_bg_op_speed_s {
    fbe_u32_t           background_op_speed;
#ifndef UEFI_ENV
    fbe_base_config_background_operation_t background_op;    
#endif
}fbe_raid_group_control_set_bg_op_speed_t;

/* FBE_RAID_GROUP_CONTROL_CODE_GET_BG_OP_SPEED */
typedef struct fbe_raid_group_control_get_bg_op_speed_s {
    fbe_u32_t           rebuild_speed; 
    fbe_u32_t           verify_speed; 
}fbe_raid_group_control_get_bg_op_speed_t;

/*!@todo DJA - fix this build issue workaround 
 * Can't find original def in UEFI mode, including fbe_raid_group_metadata.h fails for other reasons
 */
/*! Write log area (common to reg and bandwidth raid groups), slot sizes and counts. 
 *  Slot size is the raid group element size + 1 for the header block.
 */
//#define FBE_RAID_GROUP_WRITE_LOG_SLOT_COUNT_NORM ((32* 2048) / (128 + 1)) /* write log size / slot size norm*/


#if 0
#define FBE_RAID_GROUP_WRITE_LOG_SIZE (32 * FBE_RAID_DEFAULT_CHUNK_SIZE)  /* historical, came from 32 slots of 1MB */
#define FBE_RAID_GROUP_WRITE_LOG_SLOT_SIZE_NORM (FBE_RAID_SECTORS_PER_ELEMENT + 1)  /* element size + 1 */
#if 1 /*!@todo DJA fix this UEFI build issue, need fbe_raid_group.h version to be same as here exactly */
#define FBE_RAID_GROUP_WRITE_LOG_SLOT_COUNT_NORM (FBE_RAID_GROUP_WRITE_LOG_SIZE / FBE_RAID_GROUP_WRITE_LOG_SLOT_SIZE_NORM)
#else
#define FBE_RAID_GROUP_WRITE_LOG_SLOT_COUNT_NORM ((32* 2048) / (128 + 1)) /* write log size / slot size norm*/
#endif
#define FBE_RAID_GROUP_WRITE_LOG_SLOT_SIZE_BANDWIDTH (FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH + 1)  
#define FBE_RAID_GROUP_WRITE_LOG_SLOT_COUNT_BANDWIDTH (FBE_RAID_GROUP_WRITE_LOG_SIZE / FBE_RAID_GROUP_WRITE_LOG_SLOT_SIZE_BANDWIDTH)
#endif

/*! Write log area (common to reg and bandwidth raid groups), slot sizes and counts. 
 *  Slot size is the raid group element size + 1 for the header block.
 */
enum fbe_raid_group_write_log_defines_e {
    FBE_RAID_GROUP_WRITE_LOG_SIZE = (32 * FBE_RAID_DEFAULT_CHUNK_SIZE),  /* historical, came from 32 slots of 1MB */
    FBE_RAID_GROUP_WRITE_LOG_SLOT_SIZE_NORM = (FBE_RAID_SECTORS_PER_ELEMENT + 1), /* element size + 1 (hdr size) */
    FBE_RAID_GROUP_WRITE_LOG_SLOT_COUNT_NORM = (FBE_RAID_GROUP_WRITE_LOG_SIZE / FBE_RAID_GROUP_WRITE_LOG_SLOT_SIZE_NORM),
    FBE_RAID_GROUP_WRITE_LOG_SLOT_SIZE_BANDWIDTH = (FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH + 1), /* element size + 1 (hdr size) */
    FBE_RAID_GROUP_WRITE_LOG_SLOT_COUNT_BANDWIDTH = (FBE_RAID_GROUP_WRITE_LOG_SIZE / FBE_RAID_GROUP_WRITE_LOG_SLOT_SIZE_BANDWIDTH),


    FBE_RAID_GROUP_WRITE_LOG_SLOT_SIZE_NORM_4K = (FBE_RAID_SECTORS_PER_ELEMENT + FBE_520_BLOCKS_PER_4K), /* element size + 1 (hdr size) */
    FBE_RAID_GROUP_WRITE_LOG_SLOT_COUNT_NORM_4K = (FBE_RAID_GROUP_WRITE_LOG_SIZE / FBE_RAID_GROUP_WRITE_LOG_SLOT_SIZE_NORM_4K),
    /*! element size + 1 (hdr size) */ 
    FBE_RAID_GROUP_WRITE_LOG_SLOT_SIZE_BANDWIDTH_4K = (FBE_RAID_SECTORS_PER_ELEMENT_BANDWIDTH + FBE_520_BLOCKS_PER_4K), 
    FBE_RAID_GROUP_WRITE_LOG_SLOT_COUNT_BANDWIDTH_4K = (FBE_RAID_GROUP_WRITE_LOG_SIZE / FBE_RAID_GROUP_WRITE_LOG_SLOT_SIZE_BANDWIDTH_4K),
};
/* Write log slot state and invalidate state, set as bytes to compress the slot array
   Now have 508 slots and this makes the containing fbe_parity_t struct too big for memory chunk */
typedef fbe_u8_t fbe_parity_write_log_slot_state_t;
#define FBE_PARITY_WRITE_LOG_SLOT_STATE_INVALID             ((fbe_parity_write_log_slot_state_t) 0)
#define FBE_PARITY_WRITE_LOG_SLOT_STATE_FREE                ((fbe_parity_write_log_slot_state_t) 1)
#define FBE_PARITY_WRITE_LOG_SLOT_STATE_ALLOCATED           ((fbe_parity_write_log_slot_state_t) 2)
#define FBE_PARITY_WRITE_LOG_SLOT_STATE_ALLOCATED_FOR_FLUSH ((fbe_parity_write_log_slot_state_t) 3)
#define FBE_PARITY_WRITE_LOG_SLOT_STATE_ALLOCATED_FOR_REMAP ((fbe_parity_write_log_slot_state_t) 4)
#define FBE_PARITY_WRITE_LOG_SLOT_STATE_FLUSHING            ((fbe_parity_write_log_slot_state_t) 5)
#define FBE_PARITY_WRITE_LOG_SLOT_STATE_REMAPPING           ((fbe_parity_write_log_slot_state_t) 6)

typedef fbe_u8_t fbe_parity_write_log_slot_invalidate_state_t;
#define FBE_PARITY_WRITE_LOG_SLOT_INVALIDATE_STATE_SUCCESS  ((fbe_parity_write_log_slot_invalidate_state_t) 0)
#define FBE_PARITY_WRITE_LOG_SLOT_INVALIDATE_STATE_DEAD     ((fbe_parity_write_log_slot_invalidate_state_t) 2)

/*!*******************************************************************
 * @struct fbe_parity_get_write_log_slot_t
 *********************************************************************
 * @brief Returns write log slot to user.
 *
 *********************************************************************/
typedef struct fbe_parity_get_write_log_slot_s {
    fbe_parity_write_log_slot_state_t state;
    fbe_parity_write_log_slot_invalidate_state_t invalidate_state;
}fbe_parity_get_write_log_slot_t;

/*!*******************************************************************
 * @struct fbe_parity_get_write_log_info_t
 *********************************************************************
 * @brief Returns write log info to user.
 *
 *********************************************************************/
typedef struct fbe_parity_get_write_log_info_t{
    fbe_lba_t start_lba;
    fbe_u32_t slot_size;
    fbe_u32_t slot_count; /* Counter for allocated slots */
    fbe_u32_t header_size;
    fbe_bool_t is_request_queue_empty;     /* is siots waiting for slots queue empty */
    fbe_bool_t quiesced;
    fbe_bool_t needs_remap;
    fbe_u32_t flags;
    fbe_parity_get_write_log_slot_t slot_array[FBE_RAID_GROUP_WRITE_LOG_SLOT_COUNT_NORM];
}fbe_parity_get_write_log_info_t;

/*!*******************************************************************
 * @struct fbe_raid_group_class_set_update_peer_checkpoint_interval_t
 *********************************************************************
 * @brief
 *  This structure is used with the
 *  FBE_RAID_GROUP_CONTROL_CODE_CLASS_SET_UPDATE_PEER_CHECKPOINT_INTERVAL
 *
 *********************************************************************/
typedef struct fbe_raid_group_class_set_update_peer_checkpoint_interval_s
{
    /*! Value in seconds for the peer update period.
     */
    fbe_u32_t   peer_update_seconds;

} fbe_raid_group_class_set_update_peer_checkpoint_interval_t;

/*!*******************************************************************
 * @struct fbe_raid_group_set_default_bts_params_t
 *********************************************************************
 * @brief   Parameters to keep for new raid groups that are created. 
 *
 *********************************************************************/
typedef struct fbe_raid_group_set_default_bts_params_s
{
    fbe_u32_t user_queue_depth; /*!< Used to calculate max be credits for user rgs. */
    fbe_u32_t system_queue_depth; /*!< Used to calculate max be credits for system rgs. */ 
    fbe_u32_t b_throttle_enabled; /*! 0 is disabled, 1 is enabled.*/

    fbe_bool_t b_reload; /*! FBE_TRUE to just scan the registry and pull in new values. */
    fbe_bool_t b_persist; /*! FBE_TRUE to save values. */
}
fbe_raid_group_set_default_bts_params_t;

/*!*******************************************************************
 * @struct fbe_raid_group_get_default_bts_params_t
 *********************************************************************
 * @brief   Parameters to keep for new raid groups that are created. 
 *
 *********************************************************************/
typedef struct fbe_raid_group_get_default_bts_params_s 
{
    fbe_u32_t user_queue_depth; /*!< Used to calculate max be credits for user rgs. */
    fbe_u32_t system_queue_depth; /*!< Used to calculate max be credits for system rgs. */ 
    fbe_u32_t b_throttle_enabled; /*! 0 is disabled, 1 is enabled.*/
}
fbe_raid_group_get_default_bts_params_t;

/*!*******************************************************************
 * @struct fbe_raid_group_set_raid_attributes_t
 *********************************************************************
 * @brief   Attribute flags to set on the raid geometry
 *
 *********************************************************************/
typedef struct fbe_raid_group_set_raid_attributes_s 
{
    fbe_u32_t raid_attribute_flags; /*!< Flags to set on the raid geometry. */
}
fbe_raid_group_set_raid_attributes_t;

/*!*******************************************************************
 * @struct fbe_raid_group_send_to_downstream_positions_t
 *********************************************************************
 * @brief   Send the requested usurper to the downstream positions
 *          specified.
 *
 *********************************************************************/
#ifndef UEFI_ENV
typedef struct fbe_raid_group_send_to_downstream_positions_s
{
    fbe_raid_position_bitmask_t positions_to_send_to;   /*!< Mask of positions to send usurper to (debug).*/
    fbe_raid_position_bitmask_t positions_sent_to;      /*!< Mask of positions that we have sent the usurper to. */
    fbe_edge_index_t            position_in_progress;   /*!< Current position in progress. */
    fbe_packet_completion_function_t request_completion; /*!< Need to populate specific request completion. */
    fbe_packet_completion_context_t request_context;    /*!< Need to populate specific request completion context. */
}
fbe_raid_group_send_to_downstream_positions_t;
#endif /* #ifndef UEFI_ENV */


/*!*******************************************************************
 * @enum fbe_raid_emeh_mode_t
 *********************************************************************
 * @brief   Possible values for extended media error handling (EMEH)
 *          mode.
 *
 *********************************************************************/
typedef enum fbe_raid_emeh_mode_e
{
    FBE_RAID_EMEH_MODE_INVALID              = 0,
    FBE_RAID_EMEH_MODE_ENABLED_NORMAL       = 1,
    FBE_RAID_EMEH_MODE_DEGRADED_HA          = 2,    /*! @note Internal use only */
    FBE_RAID_EMEH_MODE_PACO_HA              = 3,    /*! @note Internal use only */
    FBE_RAID_EMEH_MODE_THRESHOLDS_INCREASED = 4,
    FBE_RAID_EMEH_MODE_DISABLED             = 5,

    /*! EMEH is enabled by default. */
    FBE_RAID_EMEH_MODE_DEFAULT              = FBE_RAID_EMEH_MODE_ENABLED_NORMAL,
}
fbe_raid_emeh_mode_t;

/*!*******************************************************************
 * @enum fbe_raid_emeh_params_t
 *********************************************************************
 * @brief   Possible values for extended media error handling (EMEH)
 *          parameters.
 *
 *********************************************************************/
typedef enum fbe_raid_emeh_params_e
{
    /*! @note See fbe_raid_emeh_mode_e above for the values. */
    FBE_RAID_EXTENDED_MEDIA_ERROR_HANDLING_PARAMS_MODE_MASK         = FBE_U16_MAX,
    FBE_RAID_EXTENDED_MEDIA_ERROR_HANDLING_PARAMS_OPTION_SHIFT      = 16,
    FBE_RAID_EXTENDED_MEDIA_ERROR_HANDLING_PARAMS_OPTION_MASK       = (FBE_U16_MAX << FBE_RAID_EXTENDED_MEDIA_ERROR_HANDLING_PARAMS_OPTION_SHIFT),
    FBE_RAID_EXTENDED_MEDIA_ERROR_HANDLING_PARAMS_DEFAULT_INCREASE  = 100,  /*! Default percent to increase media error threshold*/
    FBE_RAID_EXTENDED_MEDIA_ERROR_HANDLING_PARAMS_USE_DEFAULT_INCREASE  = 0,
    FBE_RAID_EXTENDED_MEDIA_ERROR_HANDLING_PARAMS_DEFAULT           = ((FBE_RAID_EXTENDED_MEDIA_ERROR_HANDLING_PARAMS_USE_DEFAULT_INCREASE << 
                                                                        FBE_RAID_EXTENDED_MEDIA_ERROR_HANDLING_PARAMS_OPTION_SHIFT)         |
                                                                       FBE_RAID_EMEH_MODE_DEFAULT),
}
fbe_raid_emeh_params_t;
 
/*!*******************************************************************
 * @enum fbe_raid_emeh_command_t
 *********************************************************************
 * @brief   Possible values for extended media error handling (EMEH)
 *          command.
 *
 *********************************************************************/
typedef enum fbe_raid_emeh_command_e
{
    FBE_RAID_EMEH_COMMAND_INVALID               = 0,
    FBE_RAID_EMEH_COMMAND_RAID_GROUP_DEGRADED   = 1,    /*! @note This is an internally generated command. */
    FBE_RAID_EMEH_COMMAND_PACO_STARTED          = 2,    /*! @note This is an internally generated command. */
    FBE_RAID_EMEH_COMMAND_RESTORE_NORMAL_MODE   = 3,
    FBE_RAID_EMEH_COMMAND_INCREASE_THRESHOLDS   = 4,   
    FBE_RAID_EMEH_COMMAND_DISABLE_EMEH          = 5,
    FBE_RAID_EMEH_COMMAND_ENABLE_EMEH           = 6,
}
fbe_raid_emeh_command_t;

/*!*******************************************************************
 * @enum fbe_raid_emeh_reliability_t
 *********************************************************************
 * @brief   Possible values for extended media error handling (EMEH)
 *          raid group reliability (determine from drives for this raid
 *          group).
 *
 *********************************************************************/
typedef enum fbe_raid_emeh_reliability_s
{
    FBE_RAID_EMEH_RELIABILITY_UNKNOWN           = 0,
    FBE_RAID_EMEH_RELIABILITY_VERY_HIGH         = 1,
    FBE_RAID_EMEH_RELIABILITY_LOW               = 2,
}
fbe_raid_emeh_reliability_t;

/*!*******************************************************************
 * @struct fbe_raid_group_class_get_extended_media_error_handling_t
 *********************************************************************
 *
 * @brief   This structure is used with the following control codes:
 *          o   FBE_RAID_GROUP_CONTROL_CODE_CLASS_SET_EXTENDED_MEDIA_ERROR_HANDLING
 *
 *********************************************************************/
#ifndef UEFI_ENV
typedef struct fbe_raid_group_class_get_extended_media_error_handling_s
{
    fbe_raid_emeh_mode_t    default_mode;    /*! Default setting for EMEH */
    fbe_raid_emeh_mode_t    current_mode;    /*! Current setting for EMEH */
    fbe_u32_t               default_threshold_increase; /*! Percentage to increase media error threshold by (if enabled)*/
    fbe_u32_t               current_threshold_increase; /*! Percentage to increase media error threshold by (if enabled)*/
}
fbe_raid_group_class_get_extended_media_error_handling_t;

/*!*******************************************************************
 * @struct fbe_raid_group_class_set_extended_media_error_handling_t
 *********************************************************************
 *
 * @brief   This structure is used with the following control codes:
 *          o   FBE_RAID_GROUP_CONTROL_CODE_CLASS_SET_EXTENDED_MEDIA_ERROR_HANDLING
 *
 *********************************************************************/
typedef struct fbe_raid_group_class_set_extended_media_error_handling_s
{
    fbe_raid_emeh_mode_t    set_mode;           /*! The mode value to set */
    fbe_bool_t              b_change_threshold; /*! If true change the increase media error threshold to the percent passed */
    fbe_u32_t               percent_threshold_increase; /*! If b_change_threshold is true use this value to increase .*/
    fbe_bool_t              b_persist;          /*! If True persist the modified values using the registry. */
}
fbe_raid_group_class_set_extended_media_error_handling_t;

/*!*******************************************************************
 * @struct fbe_raid_group_get_extended_media_error_handling_t
 *********************************************************************
 *
 * @brief   This structure is used with the following control codes:
 *          o   FBE_RAID_GROUP_CONTROL_CODE_GET_EXTENDED_MEDIA_ERROR_HANDLING
 *
 *********************************************************************/
typedef struct fbe_raid_group_get_extended_media_error_handling_s
{
    fbe_u32_t                   width;          /*! Raid group width */
    fbe_raid_emeh_mode_t        enabled_mode;   /*! Class current mode (enabled or not) */
    fbe_raid_emeh_mode_t        current_mode;   /*! Current setting for EMEH for this raid group*/
    fbe_raid_emeh_reliability_t reliability;    /*! Current reliability */
    fbe_u32_t                   paco_position;  /*! If proactive copy in progress, this is the position*/
    fbe_raid_emeh_command_t     active_command; /*! If valid, the current EMEH request in progress */
    fbe_u32_t                   default_threshold_increase; /*! Percentage to increase media error threshold by (if enabled)*/
    fbe_u32_t                   dieh_reliability[FBE_RAID_MAX_DISK_ARRAY_WIDTH];
    fbe_u32_t                   dieh_mode[FBE_RAID_MAX_DISK_ARRAY_WIDTH];
    fbe_u32_t                   media_weight_adjust[FBE_RAID_MAX_DISK_ARRAY_WIDTH];
    fbe_u32_t                   vd_paco_position[FBE_RAID_MAX_DISK_ARRAY_WIDTH];
}
fbe_raid_group_get_extended_media_error_handling_t;

/*!*******************************************************************
 * @struct fbe_raid_group_set_extended_media_error_handling_t
 *********************************************************************
 *
 * @brief   This structure is used with the following control codes:
 *          o   FBE_RAID_GROUP_CONTROL_CODE_SET_EXTENDED_MEDIA_ERROR_HANDLING
 *
 *********************************************************************/
typedef struct fbe_raid_group_set_extended_media_error_handling_s
{
    fbe_raid_emeh_command_t     set_control;        /*! The control value to set for this raid group */
    fbe_bool_t                  b_is_paco;          /*! Is this request generated by proactive copy */
    fbe_object_id_t             paco_vd_object_id;  /*! The VD object id for the proactive copy*/
    fbe_bool_t                  b_change_threshold; /*! If true change the increase media error threshold to the percent passed */
    fbe_u32_t                   percent_threshold_increase; /*! If b_change_threshold is true use this value to increase .*/
}
fbe_raid_group_set_extended_media_error_handling_t;

/*!*******************************************************************
 * @struct fbe_raid_group_set_debug_state_t
 *********************************************************************
 *
 * @brief   This structure is used with the following control codes:
 *          o   FBE_RAID_GROUP_CONTROL_CODE_SET_DEBUG_STATE
 * 
 * @note    This is to be used for testing purposes only !!!!
 *********************************************************************/
typedef struct fbe_raid_group_set_debug_state_s
{
    fbe_u64_t   local_state_mask;       /*! If not 0, this value is ORed into the current local state mask */
    fbe_u64_t   clustered_flags;        /*! If not 0, the bitmask of flags to OR into current local clustered flags value */
    fbe_u32_t   raid_group_condition;   /*! If not FBE_U32_MAX, this is hte raid group condition to set */
}
fbe_raid_group_set_debug_state_t;
#endif

/*!*******************************************************************
 * @struct fbe_raid_group_set_chunks_per_rebuild_t
 *********************************************************************
 * @brief   Number of chunks to rebuild per cycle
 *
 *********************************************************************/
typedef struct fbe_raid_group_class_set_chunks_per_rebuild_s 
{
    fbe_u32_t chunks_per_rebuild; /*!< Chunks per rebuild. */
}
fbe_raid_group_class_set_chunks_per_rebuild_t;

void fbe_raid_group_class_get_queue_depth(fbe_object_id_t object_id,
                                          fbe_u32_t width,
                                          fbe_u32_t *queue_depth_p);

/*!*******************************************************************
 * @enum fbe_raid_group_lifecycle_timer_cond_t
 *********************************************************************
 * @brief   Possible values for setting the timer interval for a
 *          raid group's lifecycle timer condition
 *
 *********************************************************************/
typedef enum fbe_raid_group_set_lifecycle_timer_cond_s
{
    FBE_RAID_GROUP_SET_LIFECYCLE_TIMER_COND_INVALID             = 0,
    FBE_RAID_GROUP_SET_LIFECYCLE_TIMER_COND_WEAR_LEVEL          = 1, /*!< get the max wear level of the drives in the raid group */
    FBE_RAID_GROUP_SET_LIFECYCLE_TIMER_COND_LAST                = 2,
}
fbe_raid_group_set_lifecycle_timer_cond_t;

#endif /* FBE_RAID_GROUP_H */

/*****************************
 * end file fbe_raid_group.h
 *****************************/
