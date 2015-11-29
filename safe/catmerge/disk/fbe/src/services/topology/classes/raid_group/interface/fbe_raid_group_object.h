#ifndef FBE_RAID_GROUP_OBJECT_H
#define FBE_RAID_GROUP_OBJECT_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/
 
/*!*************************************************************************
 * @file fbe_raid_group_object.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the private defines for the raid group object.
 *
 * @version
 *   5/15/2009:  Created. rfoley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_raid_common.h"
#include "fbe/fbe_raid_group.h"
#include "fbe_raid_geometry.h"
#include "../../base_config/fbe_base_config_private.h"
#include "fbe/fbe_transport.h"
#include "fbe_block_transport.h"
#include "fbe_raid_library.h"



/************************************************************
 *  TYPEDEFS, ENUMS, DEFINES, CONSTANTS, MACROS, GLOBALS
 ************************************************************/
/* To disable additional checking in checked builds change the
 *  -> #if DBG to #if 0.
 */
#if DBG
#define RAID_GROUP_DBG_ENABLED (FBE_TRUE)
#else
#define RAID_GROUP_DBG_ENABLED (FBE_FALSE)
#endif
/*!*******************************************************************
 * @def FBE_RAID_GROUP_THROTTLE_ENABLE_DEFAULT
 *********************************************************************
 * @brief
 *  TRUE for throttling enabled
 *  FALSE for throttling disabled
 *********************************************************************/
#define FBE_RAID_GROUP_THROTTLE_ENABLE_DEFAULT FBE_TRUE

/*!*******************************************************************
 * @def FBE_RAID_GROUP_REG_KEY_USER_QUEUE_DEPTH
 *********************************************************************
 * @brief Name of registry key where we keep the default queue depth per drive.
 *
 *********************************************************************/
#define FBE_RAID_GROUP_REG_KEY_USER_QUEUE_DEPTH "raid_group_user_queue_depth"

/*!*******************************************************************
 * @def FBE_RAID_GROUP_REG_KEY_SYSTEM_QUEUE_DEPTH
 *********************************************************************
 * @brief Name of registry key where we keep the default queue depth per drive.
 *
 *********************************************************************/
#define FBE_RAID_GROUP_REG_KEY_SYSTEM_QUEUE_DEPTH "raid_group_system_queue_depth"


/*!*******************************************************************
 * @def FBE_RAID_GROUP_REG_KEY_THROTTLE_ENABLED
 *********************************************************************
 * @brief Name of registry key where we keep the enable/disable for throttling.
 *
 *********************************************************************/
#define FBE_RAID_GROUP_REG_KEY_THROTTLE_ENABLED "raid_group_throttle_enabled"

/*!*******************************************************************
 * @def FBE_RAID_GROUP_REG_KEY_USER_DEBUG_FLAG
 *********************************************************************
 * @brief Name of registry key where we keep the default debug flags
 *        for user raid groups.
 *
 *********************************************************************/
#define FBE_RAID_GROUP_REG_KEY_USER_DEBUG_FLAG "fbe_raid_group_user_debug_flag"

/*!*******************************************************************
 * @def FBE_RAID_GROUP_REG_KEY_SYSETEM_DEBUG_FLAG
 *********************************************************************
 * @brief Name of registry key where we keep the default debug flags
 *        for system raid groups.
 *
 *********************************************************************/
#define FBE_RAID_GROUP_REG_KEY_SYSTEM_DEBUG_FLAG "fbe_raid_group_system_debug_flag"

/*!*******************************************************************
 * @def FBE_RAID_GROUP_REG_KEY_QUEUE_RATIO_ADDEND
 *********************************************************************
 * @brief Name of registry key used to manipulate the queue processing ratio 
 *
 *********************************************************************/
#define FBE_RAID_GROUP_REG_KEY_REBUILD_QUEUE_RATIO_ADDEND "rebuild_queue_ratio_addend"

/*!*******************************************************************
 * @def FBE_RAID_GROUP_REG_KEY_REBUILD_PARITY_READ_MULTIPLIER
 *********************************************************************
 * @brief Name of registry key used to manipulate the queue processing ratio 
 *
 *********************************************************************/
#define FBE_RAID_GROUP_REG_KEY_REBUILD_PARITY_READ_MULTIPLIER "rebuild_parity_read_multiplier"

/*!*******************************************************************
 * @def FBE_RAID_GROUP_REG_KEY_REBUILD_PARITY_WRITE_MULTIPLIER
 *********************************************************************
 * @brief Name of registry key used to manipulate the queue processing ratio 
 *
 *********************************************************************/
#define FBE_RAID_GROUP_REG_KEY_REBUILD_PARITY_WRITE_MULTIPLIER "rebuild_parity_write_multiplier"

/*!*******************************************************************
 * @def FBE_RAID_GROUP_REG_KEY_REBUILD_MIRROR_READ_MULTIPLIER
 *********************************************************************
 * @brief Name of registry key used to manipulate the queue processing ratio 
 *
 *********************************************************************/
#define FBE_RAID_GROUP_REG_KEY_REBUILD_MIRROR_READ_MULTIPLIER "rebuild_mirror_read_multiplier" 

/*!*******************************************************************
 * @def FBE_RAID_GROUP_REG_KEY_REBUILD_MIRROR_WRITE_MULTIPLIER
 *********************************************************************
 * @brief Name of registry key used to manipulate the queue processing ratio 
 *
 *********************************************************************/
#define FBE_RAID_GROUP_REG_KEY_REBUILD_MIRROR_WRITE_MULTIPLIER "rebuild_mirror_write_multiplier"

/*!*******************************************************************
 * @def FBE_RAID_GROUP_REG_KEY_REBUILD_CEILING_DIVISOR
 *********************************************************************
 * @brief Name of registry key used to manipulate the queue processing ratio 
 *
 *********************************************************************/
#define FBE_RAID_GROUP_REG_KEY_REBUILD_CEILING_DIVISOR "rebuild_ceiling_divisor"

/*!*******************************************************************
 * @def FBE_RAID_GROUP_REG_KEY_EXTENDED_MEDIA_ERROR_HANDLING
 *********************************************************************
 * @brief Name of registry key used to change media error handling at PDO
 *
 *********************************************************************/
#define FBE_RAID_GROUP_REG_KEY_EXTENDED_MEDIA_ERROR_HANDLING "extended_media_error_handling"

/*!*******************************************************************
 * @def FBE_RAID_GROUP_REG_KEY_CHUNKS_PER_REBUILD
 *********************************************************************
 * @brief Name of registry key used to manipulate the queue processing ratio 
 *
 *********************************************************************/
#define FBE_RAID_GROUP_REG_KEY_CHUNKS_PER_REBUILD "fbe_raid_group_chunks_per_rebuild"


/* Macro to trace the entry of a function in the RAID group object or in one of its derived classes */
#define FBE_RAID_GROUP_TRACE_FUNC_ENTRY(m_object_p)             \
                                                                \
    fbe_base_object_trace((fbe_base_object_t*) m_object_p,      \
                          FBE_TRACE_LEVEL_DEBUG_HIGH,           \
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,  \
                          "%s entry\n", __FUNCTION__);          \


/* Values for getting permission for an I/O operation */
/*! @todo - these are based on the example for the PVD which says the values probably will change 
 */
#define FBE_RAID_GROUP_REBUILD_CPUS_PER_SECOND                  100
#define FBE_RAID_GROUP_VERIFY_CPUS_PER_SECOND                   100
#define FBE_RAID_GROUP_REBUILD_MBYTES_CONSUMPTION               8
#define FBE_RAID_GROUP_VERIFY_MBYTES_CONSUMPTION                1

/*! Number of seconds to wait before giving up on RG that should be broken */
#define FBE_RAID_GROUP_MAX_TIME_WAIT_FOR_BROKEN                 30

/*!*******************************************************************
 * @def FBE_RAID_GROUP_METADATA_EXPIRATION_TIME_MS
 *********************************************************************
 * @brief This is the number of milliseconds we want to wait for
 *        a metadata operation to complete when we are already
 *        expired or aborted.
 *
 *********************************************************************/
#define FBE_RAID_GROUP_METADATA_EXPIRATION_TIME_MS 0

/*!*******************************************************************
 * @def FBE_RAID_GROUP_USER_EXPIRATION_TIME_MS
 *********************************************************************
 * @brief This is the number of milliseconds we want to wait for
 *        a user operation to complete.
 *        We will set this expiration time whenever the arriving I/O
 *        does not already have an expiration time.
 * 
 *        45 seconds is an arbitrary number.
 *        However, the thought is that if we take longer than this
 *        on a given I/O then something is wrong.
 *
 *********************************************************************/
#define FBE_RAID_GROUP_USER_EXPIRATION_TIME_MS 30000

/*!*******************************************************************
 * @def FBE_RAID_GROUP_MAX_TRACE_CHARS
 *********************************************************************
 * @brief max number of chars we will put in a formatted trace string.
 *
 *********************************************************************/
#define FBE_RAID_GROUP_MAX_TRACE_CHARS 128

/*!*******************************************************************
 * @def FBE_RAID_GROUP_COALESCE_CHUNK_RANGE
 *********************************************************************
 * @brief If the difference between the current event in the
 *         base config queue and new event that will be added is in
 *        in this chunk range, then coalesce the requst
 *********************************************************************/
#define FBE_RAID_GROUP_COALESCE_CHUNK_RANGE 5

enum fbe_raid_group_constants_e
{
    FBE_RAID_GROUP_MAX_NONPAGED_RECORDS = 1,
    FBE_RAID_GROUP_NUMBER_OF_METADATA_COPIES = 1,
    /*! Spares reserve two copies for the metadata for redundancy  
     *  since both copies are on the same drive. 
     */
    FBE_RAID_GROUP_NUMBER_OF_METADATA_COPIES_SPARE  =     1, 
    FBE_RAID_GROUP_DEFAULT_POWER_SAVE_IDLE_TIME     =  1800,    /*RAID will sleep by default after 30 min. w/o activity*/
    FBE_RAID_GROUP_BACKGROUND_OP_CHUNKS             =     1,    /*!< Num of chunks for each background op. */
    FBE_RAID_GROUP_REBUILD_BACKGROUND_OP_CHUNKS     =     4,    /*!< Num of chunks for each rebuild background op. */

    /*! This time is the time we allow in between the start  
     *  of downloads.  This time includes the download time.
     *  
     *  We want to allow enough time in between so that it is unlikely that the
     *  same I/O will be impacted by more than one download request.
     *  We also want to allow enough time in between downloads so that some other
     *  user I/O can get processed before we kick off the next download.
     *  
     *  6/5/2013: At this point Downloads can take up to 15 seconds  
     *            on some Flash drives with paddle cards.
     *            Thus we choose 30 so that there is at least 15 seconds between downloads.
     */
    FBE_RAID_GROUP_SECONDS_BETWEEN_FW_DOWNLOAD      =    30,

    /*! Time after which we declare the monitor operations slow and back off to allow user I/O to proceed. 
     * 400 is twice what we expect a single drive op to take since these ops might have two stages. 
     */
    FBE_RAID_GROUP_MAX_IO_SERVICE_TIME = 400,
    /*! This is the time we will delay monitor ops that seem to be 
     *  encountering slow response times, which indicate heavy user I/O. 
     */
    FBE_RAID_GROUP_SLOW_IO_RESCHEDULE_TIME = 500,
    FBE_RAID_GROUP_VAULT_ENCRYPT_WAIT_SECONDS = 60,
    FBE_RAID_GROUP_VAULT_ENCRYPT_WAIT_MS = (FBE_RAID_GROUP_VAULT_ENCRYPT_WAIT_SECONDS * FBE_TIME_MILLISECONDS_PER_SECOND),

    /* Number of blocks we will scan at once.  This is the largest allocation we will perform.
     */
    FBE_RAID_GROUP_ENCRYPT_BLOB_BLOCKS = FBE_MEMORY_CHUNK_SIZE_BLOCK_COUNT_64,
    FBE_RAID_GROUP_PIN_TIME_LIMIT_MS = 10000, /*!< Time outstanding before we should trace it. */
};

/* Lifecycle definitions
 * Define the raid group lifecycle's instance.
 */
extern fbe_lifecycle_const_t FBE_LIFECYCLE_CONST_DATA(raid_group);

/*! @enum fbe_raid_group_lifecycle_cond_id_t  
 *  
 *  @brief These are the lifecycle conditions for the raid group object. 
 */
typedef enum fbe_raid_group_lifecycle_cond_id_e 
{
    FBE_RAID_GROUP_LIFECYCLE_COND_EVAL_JOURNAL_FLUSH = FBE_LIFECYCLE_COND_FIRST_ID(FBE_CLASS_ID_RAID_GROUP),
    FBE_RAID_GROUP_LIFECYCLE_COND_JOURNAL_FLUSH,            /*!< Journaled writes need to be flushed */
    FBE_RAID_GROUP_LIFECYCLE_COND_JOURNAL_REMAP,            /*!< Journal area needs remap */
    FBE_RAID_GROUP_LIFECYCLE_COND_EVAL_RB_LOGGING,          /*!< Evaluate if RB logging should start */
    FBE_RAID_GROUP_LIFECYCLE_COND_UPDATE_REBUILD_CHECKPOINT, /*!< Update rebuild checkpoint condition. */    
    FBE_RAID_GROUP_LIFECYCLE_COND_MARK_SPECIFIC_NR,         /*!< Mark NR on the specified LBA range */
    FBE_RAID_GROUP_LIFECYCLE_COND_CLEAR_ERRORS,             /*!< Clear any errors marked on the downstream edge. */    
    FBE_RAID_GROUP_LIFECYCLE_COND_CLEAR_RB_LOGGING,         /*!< Clear rebuild logging. */
    FBE_RAID_GROUP_LIFECYCLE_COND_EVAL_MARK_NR,             /*!< Evaluate NR marking. */
    FBE_RAID_GROUP_LIFECYCLE_COND_SET_REBUILD_CHECKPOINT_TO_END_MARKER, /*!< Set rebuild checkpoint to the logical end marker */
    FBE_RAID_GROUP_LIFECYCLE_COND_DEST_DRIVE_FAILED_SET_REBUILD_CHECKPOINT_TO_END_MARKER, /*!< Destination drive failed, set rebuild checkpoint to the logical end marker */
    FBE_RAID_GROUP_LIFECYCLE_COND_CONFIGURATION_CHANGE,     /*!< The object is changning it's configuration mode */
    FBE_RAID_GROUP_LIFECYCLE_COND_SETUP_FOR_PAGED_METADATA_RECONSTRUCT, /*!< Setup to run a paged metadata reconstuct. */
    FBE_RAID_GROUP_LIFECYCLE_COND_PAGED_METADATA_RECONSTRUCT, /*!< Run a paged metadata reconstuct. */
    FBE_RAID_GROUP_LIFECYCLE_COND_SETUP_FOR_VERIFY_PAGED_METADATA, /*!< Setup to run a verify on just the paged metadata. */
    FBE_RAID_GROUP_LIFECYCLE_COND_VERIFY_PAGED_METADATA,    /*!< Run a verify on just the paged metadata. */
    FBE_RAID_GROUP_LIFECYCLE_COND_EVAL_RB_LOGGING_ACTIVATE, /*!< Special activate eval condition. */
    FBE_RAID_GROUP_LIFECYCLE_COND_PASSIVE_WAIT_FOR_DS_EDGES, /*!< Passive side waits for downstream edges */ 
    FBE_RAID_GROUP_LIFECYCLE_COND_MARK_NR_FOR_MD,           /*!< Active side mark NR for MD, if RG is degraded */
    FBE_RAID_GROUP_LIFECYCLE_COND_ZERO_VERIFY_CHECKPOINT,   /*!< Active side sets the verify checkpoint to 0 if it was a valid */
    FBE_RAID_GROUP_LIFECYCLE_COND_EVAL_DOWNLOAD_REQUEST,    /*!< Decides which drive to send download permit */
    FBE_RAID_GROUP_LIFECYCLE_COND_MD_MARK_INCOMPLETE_WRITE,   /*!< Marks metadata region for incomplete write verify */
    FBE_RAID_GROUP_LIFECYCLE_COND_QUIESCED_BACKGROUND_OPERATION, /*!< Background ops that occur while quiesced */
    FBE_RAID_GROUP_LIFECYCLE_COND_REKEY_OPERATION, /*!< Start and end of rekey is under quesce. */
    FBE_RAID_GROUP_LIFECYCLE_COND_KEYS_REQUESTED, /* !< Key is being requested by the downstream object */
    FBE_RAID_GROUP_LIFECYCLE_COND_EMEH_REQUEST, /*!< Execute the Extended Media Error Handling (EMEH) request */
    FBE_RAID_GROUP_LIFECYCLE_COND_WEAR_LEVEL, /*!< get the max wear level of the drives in the raid group */
    FBE_RAID_GROUP_LIFECYCLE_COND_LAST /* must be last */
} 
fbe_raid_group_lifecycle_cond_id_t;

/*! @enum fbe_raid_group_clustered_flags_e
 *  
 *  @brief flags to indicate states of the raid group object related to dual-SP processing.
 */
enum fbe_raid_group_clustered_flags_e 
{
    /*! This requests that the peer set the eval mark NR condition. 
     */
    FBE_RAID_GROUP_CLUSTERED_FLAG_EVAL_MARK_NR_REQ                = 0x0000000000000010,

    /*! The active side sets this when it is starting the eval of 
     *  mark NR. The passive side sets this bit when it sees the active side.
     *  has set this bit.                                                       
     */
    FBE_RAID_GROUP_CLUSTERED_FLAG_EVAL_MARK_NR_STARTED            = 0x0000000000000020,
    FBE_RAID_GROUP_CLUSTERED_FLAG_EVAL_MARK_NR_MASK               = 0x0000000000000030,

    /*! This requests that the peer set the eval RL condition. 
     */
    FBE_RAID_GROUP_CLUSTERED_FLAG_EVAL_RL_REQ                 = 0x0000000000000100,

    /*! The active side sets this when it is starting the eval of rl.  
     *  The passive side sets this when it sees the active side has set   
     *  this bit. When this bit is set the clustered bitmasks ( failed_drives and failed_ios) are valid.             
     */
    FBE_RAID_GROUP_CLUSTERED_FLAG_EVAL_RL_STARTED             = 0x0000000000000200,

    /*! Eval RL is done and active side decided we are broken.  This is only ever set by active side  
     *  to tell passive side we are done and broken. 
     */
    FBE_RAID_GROUP_CLUSTERED_FLAG_EVAL_RL_DONE_BROKEN         = 0x0000000000000400,
    FBE_RAID_GROUP_CLUSTERED_FLAG_EVAL_RL_MASK                = 0x0000000000000700,

    /*! This requests that the peer set the clear RL condition. 
     */
    FBE_RAID_GROUP_CLUSTERED_FLAG_CLEAR_RL_REQ                 = 0x0000000000001000,

    /*! The active side sets this when it is starting the clear of rl.  
     *  The passive side sets this when it sees the active side has set   
     *  this bit. When this bit is set the clustered bitmasks ( failed_drives and failed_ios) are valid.             
     */
    FBE_RAID_GROUP_CLUSTERED_FLAG_CLEAR_RL_STARTED             = 0x0000000000002000,

    FBE_RAID_GROUP_CLUSTERED_FLAG_CLEAR_RL_MASK                = 0x0000000000003000,

    /*! This flag is set when any PVD is in SLF. 
     */
    FBE_RAID_GROUP_CLUSTERED_FLAG_SLF_REQ                      = 0x0000000000004000,
    /*! This flag is set to request peer to set NOT_PREFERRED attr. 
     */
    FBE_RAID_GROUP_CLUSTERED_FLAG_SLF_PEER_REQ                 = 0x0000000000008000,
    FBE_RAID_GROUP_CLUSTERED_FLAG_SLF_MASK                     = 0x000000000000C000,

    /*! Either side sets this when clear rl must wait for mark nr requests to
     * complete       
     */
    FBE_RAID_GROUP_CLUSTERED_FLAG_WAIT_FOR_MARK_NR_TO_COMPLETE = 0x0000000000010000,
    FBE_RAID_GROUP_CLUSTERED_FLAG_WAIT_FOR_MARK_NR_MASK        = 0x0000000000010000,

    /*! Either side sets this when the raid group goes degraded.  Then the peer
     *  will also set this.
     */
    FBE_RAID_GROUP_CLUSTERED_FLAG_EMEH_DEGRADED_STARTED        = 0x0000000000020000,

    /*! Either side sets to indicate that the emeh raid groip degraded handling is
     *  complete.
     */
    FBE_RAID_GROUP_CLUSTERED_FLAG_EMEH_DEGRADED_DONE           = 0x0000000000040000,
    FBE_RAID_GROUP_CLUSTERED_FLAG_EMEH_DEGRADED_MASK           = 0x0000000000060000,


    /*! Either side sets this when a proactive copy request is started.  Then the
     *  flag is clustered to the peer.
     */
    FBE_RAID_GROUP_CLUSTERED_FLAG_EMEH_PACO_STARTED            = 0x0000000000080000,

    /*! Either side sets to indicate that the emeh handling of the proactive copy
     *  has been completed      
     */
    FBE_RAID_GROUP_CLUSTERED_FLAG_EMEH_PACO_DONE               = 0x0000000000100000,
    FBE_RAID_GROUP_CLUSTERED_FLAG_EMEH_PACO_MASK               = 0x0000000000180000,

    /*! Either side sets this when a request to increase the media error threshold
     *  is seen by the raid group monitor (emeh increase thresholds condition).
     */
    FBE_RAID_GROUP_CLUSTERED_FLAG_EMEH_INCREASE_STARTED        = 0x0000000000200000,

    /*! Either side sets to indicate that the emeh increase thresholds request
     *  is complete.         
     */
    FBE_RAID_GROUP_CLUSTERED_FLAG_EMEH_INCREASE_DONE           = 0x0000000000400000,
    FBE_RAID_GROUP_CLUSTERED_FLAG_EMEH_INCREASE_MASK           = 0x0000000000600000,

    /*! Either side sets this when a request to restore the default media error 
     *  threshold is seen by the raid group monitor (emeh restore thresholds condition).
     */
    FBE_RAID_GROUP_CLUSTERED_FLAG_EMEH_RESTORE_STARTED         = 0x0000000000800000,

    /*! Either side sets to indicate that the emeh restore thresholds request
     *  is complete.         
     */
    FBE_RAID_GROUP_CLUSTERED_FLAG_EMEH_RESTORE_DONE            = 0x0000000001000000,
    FBE_RAID_GROUP_CLUSTERED_FLAG_EMEH_RESTORE_MASK            = 0x0000000001800000,


    FBE_RAID_GROUP_CLUSTERED_FLAG_ALL_MASKS = (FBE_RAID_GROUP_CLUSTERED_FLAG_EVAL_RL_MASK          |
                                               FBE_RAID_GROUP_CLUSTERED_FLAG_EVAL_MARK_NR_MASK     |
                                               FBE_RAID_GROUP_CLUSTERED_FLAG_CLEAR_RL_MASK         |
                                               FBE_RAID_GROUP_CLUSTERED_FLAG_WAIT_FOR_MARK_NR_MASK |
                                               FBE_RAID_GROUP_CLUSTERED_FLAG_EMEH_DEGRADED_MASK    |
                                               FBE_RAID_GROUP_CLUSTERED_FLAG_EMEH_PACO_MASK        |
                                               FBE_RAID_GROUP_CLUSTERED_FLAG_EMEH_INCREASE_MASK    |
                                               FBE_RAID_GROUP_CLUSTERED_FLAG_EMEH_RESTORE_MASK      ),

    /*! There are certain transitions (for instance join) that should be postponed
     *  if there is a swap job in progress (since the downstream path is changing).
     */
    FBE_RAID_GROUP_CLUSTERED_FLAG_SWAP_JOB_IN_PROGRESS         = 0x0000000002000000,
    FBE_RAID_GROUP_CLUSTERED_FLAG_SWAP_JOB_IN_PROGRESS_MASK    = 0x0000000002000000,

    FBE_RAID_GROUP_CLUSTERED_FLAG_CHANGE_CONFIG_REQ            = 0x0000000004000000,
    FBE_RAID_GROUP_CLUSTERED_FLAG_CHANGE_CONFIG_STARTED        = 0x0000000008000000,
    FBE_RAID_GROUP_CLUSTERED_FLAG_CHANGE_CONFIG_DONE_ERROR     = 0x0000000010000000,
    FBE_RAID_GROUP_CLUSTERED_FLAG_CHANGE_CONFIG_MASK           = 0x000000001C000000,

    FBE_RAID_GROUP_CLUSTERED_FLAG_INVALID                      = 0xFFFFFFFFFFFFFFFF,
};

enum fbe_raid_group_np_flag_status_e
{
    FBE_RAID_GROUP_NP_FLAGS_DEFAULT                 = 0x00, /*!< Initial value of flags. */
    FBE_RAID_GROUP_NP_FLAGS_SWAPPED_IN              = 0x01, /*!< A replacement drive has been swapped in. */
    FBE_RAID_GROUP_NP_FLAGS_NO_SPARE_REPORTED       = 0x02, /*!< We have reported that no spare was located for the associated virtual drive.*/   
    FBE_RAID_GROUP_NP_FLAGS_SOURCE_DRIVE_FAILED     = 0x04, /*!< Indicates that the source drive for a copy failed during the operation. */   
    FBE_RAID_GROUP_NP_FLAGS_MARK_NR_REQUIRED        = 0x08, /*!< Indicates that the eval mark NR condition must run prior to clearing rebuild logging. */   
    FBE_RAID_GROUP_NP_FLAGS_RECONSTRUCT_REQUIRED    = 0x10, /*!< Indicates that the paged metadata needs to be reconstructed. */   
    FBE_RAID_GROUP_NP_FLAGS_ERROR_VERIFY_REQUIRED   = 0x20, /*!< Indicates that another error_verify pass is needed after the current one completes. */
    FBE_RAID_GROUP_NP_FLAGS_DEGRADED_NEEDS_REBUILD  = 0x40, /*!< Indicates that this object is degraded and the parent needs to rebuild it. */
    FBE_RAID_GROUP_NP_FLAGS_IW_VERIFY_REQUIRED      = 0x80, /*!< A iw verify pass is needed after this one finishes.. */   
    FBE_RAID_GROUP_NP_FLAGS_LAST,
};

/* Since we ran out of space in the above flag structure, adding a new extended flags*/
enum fbe_raid_group_np_extended_flag_status_e
{
    FBE_RAID_GROUP_NP_EXTENDED_FLAGS_DEFAULT                 = 0x0000000000000000, /*!< Initial value of flags. */
    FBE_RAID_GROUP_NP_EXTENDED_FLAGS_4K_COMMITTED            = 0x0000000000000001,/*!< Indicates that 4K drive option is now committed */
    FBE_RAID_GROUP_NP_EXTENDED_FLAGS_LAST,
};

/*! @enum   fbe_raid_group_io_failure_type_t
 *  
 *  @brief  Enumeration of raid group failures:
 *              1) Transport
 *              2) Block operation
 *              3) Stripe lock 
 */
typedef enum fbe_raid_group_io_failure_type_e
{
    FBE_RAID_GROUP_IO_FAILURE_TYPE_INVALID  = 0,    /*!< Should never occur. */
    FBE_RAID_GROUP_IO_FAILURE_TYPE_TRANSPORT,       /*!< Packet never reached destination. */
    FBE_RAID_GROUP_IO_FAILURE_TYPE_BLOCK_OPERATION, /*!< Block operation failed. */
    FBE_RAID_GROUP_IO_FAILURE_TYPE_CHECKSUM_ERROR,  /*!< Data read or being written had checksum/lba stamp error.*/
    FBE_RAID_GROUP_IO_FAILURE_TYPE_STRIPE_LOCK,     /*!< Stripe lock operation failed. */
    FBE_RAID_GROUP_IO_FAILURE_TYPE_LAST             /* must be last */ 
} 
fbe_raid_group_io_failure_type_t;

/*!****************************************************************************
 * @struct fbe_raid_group_metadata_positions_s
 *
 * @brief
 *    This structure contains metadata position related information.
 *
 ******************************************************************************/
typedef struct fbe_raid_group_metadata_positions_s
{

    /*! raid paged metadata address. 
     */
    fbe_lba_t           paged_metadata_lba;

    /*! If metadata is mirrored this offset is used to calculate mirrored
     *  positions.
     */
    fbe_lba_t           paged_mirror_metadata_offset;

    /*! raid paged metadata block count.
     */
    fbe_block_count_t   paged_metadata_block_count;

    /*! raid paged metadata capacity. 
     */
    fbe_lba_t           paged_metadata_capacity;   

    /*! Blocks we consume on all disks in rg. Includes user, metadata, log. 
     */
    fbe_lba_t           imported_blocks_per_disk;

    fbe_lba_t           raid_capacity; /*!< Blocks protected by raid. */ 

    /*! exported capacity.
     */
    fbe_lba_t           exported_capacity;

    /*! imported capacity.
     */
    fbe_lba_t           imported_capacity;

    /*! Journal write log physical start lba.
     */
    fbe_lba_t           journal_write_log_pba;

    fbe_lba_t           journal_write_log_physical_capacity;
}
fbe_raid_group_metadata_positions_t;


/*!*******************************************************************
 * @struct fbe_raid_verify_error_tracking_t
 *********************************************************************
 * @brief
 *  This structure contains the pertinent information about a verify
 *  monitor I/O operation.
 *
 *********************************************************************/
typedef struct fbe_raid_verify_tracking_s
{    
    fbe_bool_t                      is_lun_start_b;         // !< true if this rebuild is the LUN's start
    fbe_bool_t                      is_lun_end_b;           // !< true if this rebuild is the LUN's end
    fbe_raid_verify_flags_t         verify_flags;           // !<verify flags
    fbe_raid_verify_error_counts_t  verify_error_counts;    // !< the verify error counters
    fbe_raid_flush_error_counts_t   flush_error_counts;     // !< the flush error counters
}
fbe_raid_verify_tracking_t;

/*!*******************************************************************
 * @enum    fbe_raid_group_get_checkpoint_request_type_t
 *********************************************************************
 * @brief
 *  Specifies the request type of the caller.
 *
 *********************************************************************/
typedef enum fbe_raid_group_get_checkpoint_request_type_e
{
    FBE_RAID_GROUP_GET_CHECKPOINT_REQUEST_TYPE_INVALID      = 0,    /*!< Invalid request type. */
    FBE_RAID_GROUP_GET_CHECKPOINT_REQUEST_TYPE_EVAL_MARK_NR = 1,    /*!< Request is to mark needs rebuild. */ 
    FBE_RAID_GROUP_GET_CHECKPOINT_REQUEST_TYPE_CLEAR_RL     = 2,    /*!< Request is to clear rebuild logging. */ 
} 
fbe_raid_group_get_checkpoint_request_type_t;

/*!*******************************************************************
 * @struct fbe_raid_group_get_checkpoint_info_t
 *********************************************************************
 * @brief
 *  This structure contains the checkpoint information for the raid
 *  group position specified.
 *
 *********************************************************************/
#pragma pack(1)
typedef struct fbe_raid_group_get_checkpoint_info_s
{
    fbe_lba_t       downstream_checkpoint;  /*!< Based on the downstream health, mode etc, this is the checkpoint */
    fbe_u32_t       edge_index;             /*!< Downstream edge index for checkpoint request. */
    fbe_class_id_t  requestor_class_id;     /*!< class id of the requesting object. */
    fbe_raid_group_get_checkpoint_request_type_t request_type;   /*!< Specifies the type/use of request. */

} fbe_raid_group_get_checkpoint_info_t;
#pragma pack()


/*!*******************************************************************
 * @struct fbe_raid_group_encryption_context_s
 *********************************************************************
 * @brief
 *  This structure contains the pertinent information about a encryption
 *  monitor I/O operation.
 *
 *********************************************************************/
typedef struct fbe_raid_group_encryption_context_s
{
    fbe_lba_t                       start_lba;                  // !< starting LBA
    fbe_block_count_t               block_count;                // !< number of blocks
    fbe_raid_position_bitmask_t     encryption_bitmask;            // !< bitmask of positions being rebuilt
    fbe_object_id_t                 lun_object_id;              // !< lun object-id
    fbe_bool_t                      is_lun_start_b;             // !< is lun start of the extent
    fbe_bool_t                      is_lun_end_b;               // !< is lun end of the extent
    fbe_raid_position_bitmask_t     update_checkpoint_bitmask;  // !< checkpoint update position information
    fbe_lba_t                       update_checkpoint_lba;      // !< checkpoint update start lba
    fbe_block_count_t               update_checkpoint_blocks;   // !< checkpoint update block count
    fbe_raid_position_t             event_log_position;         // !< position for event log
    fbe_base_config_physical_drive_location_t  event_log_location; // !< bus number of the rebuilt disk
    fbe_raid_position_t             event_log_source_position;  // !< position for event log
    fbe_base_config_physical_drive_location_t  event_log_source_location; // !< bus number of the rebuilt disk
    fbe_lun_index_t                object_index; /*!< Object index reported by LUN */
    fbe_status_t                   encryption_status; /*! Preserves status across unpin. */
    fbe_bool_t                      b_beyond_capacity; /*!< Extent is special with paged. */
    fbe_lba_t                       top_lba;  /*!< Top level object's lba. */
    void *opaque;                   /*!< Context from cache we need to keep. */
    fbe_bool_t                      b_was_dirty; /*!< More context from cache. */
    fbe_sg_element_t               *sg_p; /*!< Allocated sg for read and pin. */
    fbe_u8_t                       *chunk_p; /*! Memory allocated for use by persistent memory svc. */
}
fbe_raid_group_encryption_context_t;

/*!*******************************************************************
 * @def FBE_RAID_GROUP_INVALID_INDEX
 *********************************************************************
 *  @brief Invalid array index.  Used when initializing an array index
 *  to "none found".
 *
 *********************************************************************/
#define FBE_RAID_GROUP_INVALID_INDEX    ((fbe_u32_t) -1)

/*!*******************************************************************
 * @def FBE_RAID_GROUP_ACTIVATE_DEGRADED_WAIT_SEC
 *********************************************************************
 *  @brief Used when RAID group is coming up and drives are powering up.
 *
 *********************************************************************/
#define FBE_RAID_GROUP_ACTIVATE_DEGRADED_WAIT_SEC   5

/*!*******************************************************************
 * @def FBE_RAID_GROUP_DEFAULT_THROTTLE_PER_DRIVE
 *********************************************************************
 * @brief This is N Megabytes of blocks per drive that we will throttle at.
 *
 *********************************************************************/
#define FBE_RAID_GROUP_DEFAULT_THROTTLE_PER_DRIVE 5 * 0x800

struct fbe_raid_group_s;
typedef void(*fbe_raid_group_get_data_disks_fn_t)(struct fbe_raid_group_s *rg_p, fbe_u16_t *data_disks_p);

/*!*******************************************************************
 * @enum ffbe_raid_group_paged_reconst_state_t
 *********************************************************************
 * @brief State of the current paged reconstruct.
 *
 *********************************************************************/
enum fbe_raid_group_paged_reconst_state_e
{
    FBE_RAID_GROUP_RECONST_STATE_INVALID = 0,
    FBE_RAID_GROUP_RECONST_STATE_SCAN_PAGED,
    FBE_RAID_GROUP_RECONST_STATE_VERIFY,
    FBE_RAID_GROUP_RECONST_STATE_RECONSTRUCT_PAGED,
    FBE_RAID_GROUP_RECONST_STATE_DONE,

    FBE_RAID_GROUP_RECONST_STATE_LAST
};
typedef fbe_u64_t fbe_raid_group_paged_reconst_state_t;

/*!*******************************************************************
 * @struct fbe_raid_group_paged_reconstruct_t
 *********************************************************************
 * @brief Tracking info for a paged reconstruct.
 *
 *********************************************************************/
typedef struct fbe_raid_group_paged_reconstruct_s
{
    fbe_raid_group_paged_reconst_state_t state;
    fbe_lba_t orig_rekey_chkpt;
    fbe_chunk_index_t first_invalid_chunk_index;
    fbe_chunk_index_t last_invalid_chunk_index;
    fbe_chunk_index_t first_non_rekeyed_chunk_index;
    fbe_chunk_index_t last_rekeyed_chunk_index;
    fbe_lba_t verify_start;
    fbe_lba_t verify_blocks;
}
fbe_raid_group_paged_reconstruct_t;

enum fbe_raid_group_bg_op_flags_e
{
    FBE_RAID_GROUP_BG_OP_FLAG_INVALID = 0,
    FBE_RAID_GROUP_BG_OP_FLAG_OUTSTANDING       = 0x0001,
    FBE_RAID_GROUP_BG_OP_FLAG_PIN_OUTSTANDING   = 0x0002,
    FBE_RAID_GROUP_BG_OP_FLAG_UNPIN_OUTSTANDING = 0x0004,
    FBE_RAID_GROUP_BG_OP_FLAG_RECONSTRUCT_PAGED = 0x0008,
};
typedef fbe_u32_t fbe_raid_group_bg_op_flags_t;

/*!*******************************************************************
 * @struct fbe_raid_group_bg_op_info_t
 *********************************************************************
 * @brief Tracking info for background operations.
 *
 *********************************************************************/
typedef struct fbe_raid_group_bg_op_info_s
{
    fbe_time_t start_time; /*! Time we started an operation. */
    fbe_time_t pin_time; /*! Time we started an operation. */
    fbe_raid_group_bg_op_flags_t flags;
    union {
        fbe_memory_request_t memory_request;
        fbe_raid_group_paged_reconstruct_t reconstruct_context;
    } u;
}
fbe_raid_group_bg_op_info_t;

/*!*******************************************************************
 * @struct fbe_raid_group_t
 *********************************************************************
 * @brief
 *  This object represents a configured raid group.
 *
 *********************************************************************/
typedef struct fbe_raid_group_s
{
    /* The raid group object inherits from the base configuration object.
     */
    fbe_base_config_t base_config;

    /*! RAID gorup metadata memory.
     */
    fbe_raid_group_metadata_memory_t raid_group_metadata_memory;
    fbe_raid_group_metadata_memory_t raid_group_metadata_memory_peer;

    /*! This is the raid group geometry information required to execute
     *  raid I/O.
     */
    fbe_raid_geometry_t geo;

    fbe_raid_group_flags_t flags;

    /*! Tracks the raid group's state related to some conditions.
     */
    fbe_raid_group_local_state_t local_state; 

    /*! Debug flags to allow testing of different paths in the raid
     *  group object.
     */
    fbe_raid_group_debug_flags_t debug_flags;

    /*! The verify tracking structure is non-persistent data that contains the
     *  pertinent verify information for the current monitor I/O cycle.
     */
    fbe_raid_verify_tracking_t verify_ts;

    /*! The size of a chunk in the bitmap expressed as number of blocks.
     */
    fbe_chunk_size_t chunk_size;

    /*! how many sceonds the RAID user is wiling to wait for the object ot become
    ready after saving power*/
    fbe_u64_t max_raid_latency_time_is_sec;

    /*! Timestamp used to keep track of how long waiting for a RG to go broken */
    fbe_time_t  last_check_rg_broken_timestamp;

    /*! Priority for rebuild I/Os */
    fbe_traffic_priority_t  rebuild_priority;

    /*! Priority for background verify I/Os */
    fbe_traffic_priority_t  verify_priority;

    /*! Count of requests that were previously quiesced.  */
    fbe_u32_t quiesced_count;
    
    /*! The download request bitmask. One drive at a time. */
    fbe_u16_t download_request_bitmask;
    
    /*! The download granted bitmask. One drive at a time. */
    fbe_u16_t download_granted_bitmask;

    fbe_time_t last_download_timestamp; /*!< Time when we sent the last grant. */

    /*! Count for number of times paged metadata region has been verified */
    fbe_u32_t               paged_metadata_verify_pass_count;

    fbe_time_t last_checkpoint_time; /*!< Last time we updated the checkpoint. */
    fbe_lba_t last_checkpoint_to_peer; /*!< Last checkpoint we sent to the peer. */

    /*! keep the list of the PVD Ids we were rebulding. We need that because 
    by the time we finish the rebuild, we lost all of this information but we need it for the 
    final notification to user space to tell them we are done */
    fbe_object_id_t rebuilt_pvds[FBE_RAID_GROUP_MAX_REBUILD_POSITIONS];

    /*! keep track so we notify when it really changes, and also know when a new one starts*/
    fbe_u32_t       previous_rebuild_percent[FBE_RAID_GROUP_MAX_REBUILD_POSITIONS];

    fbe_metadata_stripe_lock_blob_t stripe_lock_blob;
    fbe_metadata_stripe_lock_hash_t stripe_lock_hash;

    fbe_time_t background_start_time; /*!< This is the time when we started the background op. */
    fbe_lba_t last_capacity; /*!< During expand this has the last capacity. */

    fbe_raid_group_bg_op_info_t *bg_info_p; /*!< Tracking information for background op. */

    fbe_raid_emeh_command_t         emeh_request;       /*! Outstanding EMEH request. */
    fbe_raid_emeh_mode_t            emeh_enabled_mode;  /*! Determines of EMEH is enabled for this raid group or not.*/
    fbe_raid_emeh_mode_t            emeh_current_mode;  /*! Current EMEH mode */
    fbe_raid_emeh_reliability_t     emeh_reliability;   /*! Current (from last get EMEH info) reliability */
    fbe_u32_t                       emeh_paco_position; /*! During the EMEH PACO request, this field contains the proactive copy position.*/

    fbe_u64_t max_pe_cycle;
    fbe_u64_t current_pe_cycle;
    fbe_u64_t power_on_hours;

    /*! Lifecycle defines.
     */
    FBE_LIFECYCLE_DEF_INST_DATA;
    FBE_LIFECYCLE_DEF_INST_COND(FBE_LIFECYCLE_COND_MAX(FBE_RAID_GROUP_LIFECYCLE_COND_LAST));
}
fbe_raid_group_t;

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

fbe_bool_t fbe_raid_group_is_r10_degraded(fbe_raid_group_t* in_raid_group_p);

/* Accessor for the raid geometry
 */
static __forceinline fbe_raid_geometry_t *fbe_raid_group_get_raid_geometry(fbe_raid_group_t *raid_group_p)
{
    return(&raid_group_p->geo);
}

static __forceinline fbe_class_id_t fbe_raid_group_get_class_id(fbe_raid_group_t *raid_group_p)
{
    return fbe_base_object_get_class_id(fbe_base_pointer_to_handle((fbe_base_t *)raid_group_p));
}

static __forceinline void fbe_raid_group_get_width(fbe_raid_group_t *raid_group_p,
                                            fbe_u32_t *width_p)
{
    fbe_base_config_get_width((fbe_base_config_t*)raid_group_p, width_p);
    return;
}

static __forceinline void fbe_raid_group_lock(fbe_raid_group_t *raid_group_p)
{
    fbe_base_object_lock((fbe_base_object_t *)raid_group_p);
    return;
}
static __forceinline void fbe_raid_group_unlock(fbe_raid_group_t *raid_group_p)
{
    fbe_base_object_unlock((fbe_base_object_t *)raid_group_p);
    return;
}

static __forceinline fbe_bool_t fbe_raid_group_is_debug_flag_set(fbe_raid_group_t *raid_p,
                                                          fbe_raid_group_debug_flags_t debug_flags)
{
    if (raid_p == NULL)
    {
        return FBE_FALSE;
    }
    return ((raid_p->debug_flags & debug_flags) == debug_flags);
}

/* Compiled out for non-checked builds
 */
#if RAID_GROUP_DBG_ENABLED
static __forceinline fbe_bool_t fbe_raid_group_io_is_debug_flag_set(fbe_raid_group_t *raid_p,
                                                                    fbe_raid_group_debug_flags_t debug_flags)
{
    if (raid_p == NULL)
    {
        return FBE_FALSE;
    }
    return ((raid_p->debug_flags & debug_flags) == debug_flags);
}

#else
#define fbe_raid_group_io_is_debug_flag_set(raid_p, debug_flags)    (FBE_FALSE)

#endif

static __forceinline void fbe_raid_group_init_debug_flags(fbe_raid_group_t *raid_p,
                                                   fbe_raid_group_debug_flags_t debug_flags)
{
    raid_p->debug_flags = debug_flags;
    return;
}
static __forceinline void fbe_raid_group_get_debug_flags(fbe_raid_group_t *raid_p,
                                                  fbe_raid_group_debug_flags_t *debug_flags_p)
{
    *debug_flags_p = raid_p->debug_flags;
}
static __forceinline void fbe_raid_group_set_debug_flags(fbe_raid_group_t *raid_p,
                                                  fbe_raid_group_debug_flags_t debug_flags)
{
    raid_p->debug_flags = debug_flags;
    return;
}
static __forceinline void fbe_raid_group_set_debug_flag(fbe_raid_group_t *raid_p,
                                                fbe_raid_group_debug_flags_t debug_flags)
{
    raid_p->debug_flags |= debug_flags;
}
static __forceinline void fbe_raid_group_clear_debug_flag(fbe_raid_group_t *raid_p,
                                                fbe_raid_group_debug_flags_t debug_flags)
{
    raid_p->debug_flags &= ~debug_flags;
}

/* Accessors for flags.
 */
static __forceinline fbe_bool_t fbe_raid_group_is_flag_set(fbe_raid_group_t *raid_p,
                                                          fbe_raid_group_flags_t flags)
{
    return ((raid_p->flags & flags) == flags);
}
static __forceinline void fbe_raid_group_init_flags(fbe_raid_group_t *raid_p)
{
    raid_p->flags = 0;
    return;
}
static __forceinline void fbe_raid_group_set_flag(fbe_raid_group_t *raid_p,
                                                  fbe_raid_group_flags_t flags)
{
    raid_p->flags |= flags;
}

static __forceinline void fbe_raid_group_clear_flag(fbe_raid_group_t *raid_p,
                                                    fbe_raid_group_flags_t flags)
{
    raid_p->flags &= ~flags;
}

/* Accessors related to local state.
 */
static __forceinline fbe_bool_t fbe_raid_group_is_local_state_set(fbe_raid_group_t *raid_p,
                                                                  fbe_raid_group_local_state_t local_state)
{
    return ((raid_p->local_state & local_state) == local_state);
}
static __forceinline void fbe_raid_group_init_local_state(fbe_raid_group_t *raid_p)
{
    raid_p->local_state = 0;
    return;
}
static __forceinline void fbe_raid_group_set_local_state(fbe_raid_group_t *raid_p,
                                                         fbe_raid_group_local_state_t local_state)
{
    raid_p->local_state |= local_state;
}

static __forceinline void fbe_raid_group_clear_local_state(fbe_raid_group_t *raid_p,
                                                           fbe_raid_group_local_state_t local_state)
{
    raid_p->local_state &= ~local_state;
}

static __forceinline void fbe_raid_group_update_quiesced_count(fbe_raid_group_t *raid_p,
                                                               fbe_u32_t count)
{
    raid_p->quiesced_count += count;
}

static __forceinline void fbe_raid_group_dec_quiesced_count(fbe_raid_group_t *raid_p)
{
    if ((fbe_s32_t)raid_p->quiesced_count < 1)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,  
                              "%s unexpected quiesced count: %d\n", 
                              __FUNCTION__, raid_p->quiesced_count);
    }
    else
    {
        raid_p->quiesced_count--;
    }
}
static __forceinline void fbe_raid_group_get_quiesced_count(fbe_raid_group_t *raid_p,
                                                            fbe_u32_t *count_p)
{
    *count_p = raid_p->quiesced_count;
}


static __forceinline void fbe_raid_group_get_paged_metadata_verify_pass_count(fbe_raid_group_t *raid_p,
                                                                              fbe_u32_t *md_verify_pass_count)
{
    *md_verify_pass_count = raid_p->paged_metadata_verify_pass_count;
}

static __forceinline void fbe_raid_group_set_paged_metadata_verify_pass_count(fbe_raid_group_t *raid_p,
                                                                              fbe_u32_t md_verify_pass_count)
{
    raid_p->paged_metadata_verify_pass_count = md_verify_pass_count;
    return;
}

static __forceinline void fbe_raid_group_inc_paged_metadata_verify_pass_count(fbe_raid_group_t *raid_p)
{
    raid_p->paged_metadata_verify_pass_count++;
    return;
}

/*!**************************************************************
 * fbe_raid_group_is_striper_class()
 ****************************************************************
 * @brief Determine if this is any type of striped raid group
 * @param raid_group_p - The raid group ptr.               
 *
 * @return -   FBE_TRUE if we are any type of striped raid group
 *            FBE_FALSE otherwise 
 *
 ****************************************************************/

static __forceinline fbe_bool_t fbe_raid_group_is_striper_class(fbe_raid_group_t *raid_group_p)
{
    return (raid_group_p->base_config.base_object.class_id == FBE_CLASS_ID_STRIPER);
}
/*!**************************************************************
 * fbe_raid_group_is_mirror_class()
 ****************************************************************
 * @brief Determine if this is any type of mirror raid group
 * @param raid_group_p - The raid group ptr.               
 *
 * @return -   FBE_TRUE if we are any type of mirror raid group
 *            FBE_FALSE otherwise 
 *
 ****************************************************************/

static __forceinline fbe_bool_t fbe_raid_group_is_mirror_class(fbe_raid_group_t *raid_group_p)
{
    return (raid_group_p->base_config.base_object.class_id == FBE_CLASS_ID_MIRROR);
}
/*!**************************************************************
 * fbe_raid_group_is_parity_class()
 ****************************************************************
 * @brief Determine if this is a parity raid group.
 * @param raid_group_p - The raid group ptr.
 *
 * @return -   FBE_TRUE if we are a parity raid group.
 *            FBE_FALSE otherwise.
 *
 ****************************************************************/

static __forceinline fbe_bool_t fbe_raid_group_is_parity_class(fbe_raid_group_t *raid_group_p)
{
    return (raid_group_p->base_config.base_object.class_id == FBE_CLASS_ID_PARITY);
}


/*!****************************************************************************
 * fbe_raid_group_is_nonpaged_metadata_initialized()
 ******************************************************************************
 * @brief 
 *   This function returns whether the non-paged metadata has been initialized 
 *   yet or not. 
 *
 * @param in_raid_group_p   - pointer to the raid group object 
 *
 * @return                  - FBE_TRUE if the non-paged metadata is valid
 *                            FBE_FALSE if not 
 *
 ******************************************************************************/

static __forceinline fbe_bool_t fbe_raid_group_is_nonpaged_metadata_initialized(
                                                fbe_raid_group_t*       in_raid_group_p)

{
    fbe_raid_group_nonpaged_metadata_t*         nonpaged_metadata_p;    // pointer to all of nonpaged metadata
    fbe_lifecycle_state_t                       lifecycle_state;        // raid object's current lifecycle state


    //  Get a pointer to the non-paged metadata 
    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t*) in_raid_group_p, (void **)&nonpaged_metadata_p);

    //  If the non-paged metadata pointer is null, it is not initialized yet - return false
    if (nonpaged_metadata_p == NULL)
    {
        return FBE_FALSE;
    }

    //  For now we will say it is not initialized when we are still in the specialize state. 
    //! @todo - check the WWN or other data to determine if the non-paged MD has been initialized yet or not.
    // 
    fbe_base_object_get_lifecycle_state((fbe_base_object_t*) in_raid_group_p, &lifecycle_state);
    if (lifecycle_state == FBE_LIFECYCLE_STATE_SPECIALIZE)
    {
        return FBE_FALSE;
    }

    //  Return true that the non-paged metadata is initialized  
    return FBE_TRUE;

} // End fbe_raid_group_is_nonpaged_metadata_initialized()


/*!****************************************************************************
 *  fbe_raid_group_get_rebuild_checkpoint()
 ******************************************************************************
 * @brief 
 *   This function is located in file fbe_raid_group_rebuild.c 
 * 
 ******************************************************************************/

fbe_status_t fbe_raid_group_get_rebuild_checkpoint(
                                            fbe_raid_group_t*           in_raid_group_p,
                                            fbe_u32_t                   in_position,
                                            fbe_lba_t*                  out_checkpoint_p);

fbe_status_t fbe_raid_group_get_min_rebuild_checkpoint(fbe_raid_group_t *raid_group_p,
                                                       fbe_lba_t *out_checkpoint_p);
fbe_status_t fbe_raid_group_get_max_rebuild_checkpoint(fbe_raid_group_t *raid_group_p,
                                                       fbe_lba_t *out_checkpoint_p);
fbe_status_t fbe_raid_group_get_id_of_pvd_being_worked_on(fbe_raid_group_t *raid_group_p,
                                                          fbe_packet_t *packet_p,
                                                          fbe_block_edge_t *downstream_edge_p,
                                                          fbe_object_id_t *pvd_id);

/*!****************************************************************************
 *  fbe_raid_group_get_raid10_rebuild_info()
 ******************************************************************************
 * @brief 
 *   This function is located in file fbe_raid_group_rebuild.c 
 * 
 ******************************************************************************/
fbe_status_t fbe_raid_group_get_raid10_rebuild_info(fbe_packet_t *packet_p,
                                                    fbe_raid_group_t*           raid_group_p,
                                                    fbe_u32_t                   position,
                                                    fbe_lba_t*                  checkpoint_p,
                                                    fbe_bool_t *b_rb_logging_p);


/*!****************************************************************************
 * fbe_raid_group_get_rb_logging_bitmask()
 ******************************************************************************
 * @brief 
 *   This function returns a bitmask that indicates which disk(s)
 *   we are rebuild logging on.
 *
 * @param in_raid_group_p           - pointer to the raid group object 
 * @param rl_bitmask_p    - pointer to a bitmask which indicates the value
 *                                    of rebuild logging bitmask.  This value is 
 *                                    only valid when the function returns success.
 *
 * @return fbe_status_t     
 *
 ******************************************************************************/

static __forceinline fbe_status_t fbe_raid_group_get_rb_logging_bitmask(
                                                    fbe_raid_group_t*            in_raid_group_p,
                                                    fbe_raid_position_bitmask_t* rl_bitmask_p)
{
    fbe_raid_group_nonpaged_metadata_t*             nonpaged_metadata_p;         // pointer to all of nonpaged metadata 


    //  If the non-paged metadata is not initialized yet, then return 0 for the rl bitmask
    if (fbe_raid_group_is_nonpaged_metadata_initialized(in_raid_group_p) == FBE_FALSE)
    {
        *rl_bitmask_p = 0; 
        return FBE_STATUS_NOT_INITIALIZED;
    }

    //  Get a pointer to the non-paged metadata 
    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t*) in_raid_group_p, (void **)&nonpaged_metadata_p);

    //  Set the rl bitmask in the output parameter
    *rl_bitmask_p = nonpaged_metadata_p->rebuild_info.rebuild_logging_bitmask;

    //  Return success 
    return FBE_STATUS_OK;

} // End fbe_raid_group_get_rb_logging_bitmask()


/*!****************************************************************************
 * fbe_raid_group_get_glitched_drive_bitmask()
 ******************************************************************************
 * @brief 
 *   This function returns a bitmask that indicates which disk(s) experienced
 *   glitch.
 *
 * @param in_raid_group_p           - pointer to the raid group object 
 * @param glitch_bitmask_p          - pointer to a bitmask which indicates the value
 *                                    of glitched drive bitmask. This value is 
 *                                    only valid when the function returns success.
 *
 * @return fbe_status_t     
 *
 ******************************************************************************/

static __forceinline fbe_status_t fbe_raid_group_get_glitched_drive_bitmask(
                                                    fbe_raid_group_t*            in_raid_group_p,
                                                    fbe_raid_position_bitmask_t* glitch_bitmask_p)
{
    fbe_raid_group_nonpaged_metadata_t*             nonpaged_metadata_p;         // pointer to all of nonpaged metadata 


    //  If the non-paged metadata is not initialized yet, then return 0 for the rl bitmask
    if (fbe_raid_group_is_nonpaged_metadata_initialized(in_raid_group_p) == FBE_FALSE)
    {
        *glitch_bitmask_p = 0; 
        return FBE_STATUS_NOT_INITIALIZED;
    }

    //  Get a pointer to the non-paged metadata 
    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t*) in_raid_group_p, (void **)&nonpaged_metadata_p);

    //  Set the rl bitmask in the output parameter
    *glitch_bitmask_p = nonpaged_metadata_p->glitching_disks_bitmask;

    //  Return success 
    return FBE_STATUS_OK;

} // End fbe_raid_group_get_rb_logging_bitmask()


/*!****************************************************************************
 * fbe_raid_group_get_rb_logging()
 ******************************************************************************
 * @brief 
 *   Get the value of rl bitmask for the given disk position.
 *
 * @param in_raid_group_p       - pointer to the raid group object 
 * @param in_position           - disk position in the RG 
 * @param b_is_rl_set_p  - TRUE if RL is set FALSE otherwise.
 *
 * @return fbe_status_t         
 *
 ******************************************************************************/

static __forceinline fbe_status_t fbe_raid_group_get_rb_logging(fbe_raid_group_t*    in_raid_group_p,
                                                                  fbe_u32_t            in_position,
                                                                  fbe_bool_t*          b_is_rl_set_p)

{
    fbe_raid_position_bitmask_t     rl_bitmask;
    fbe_raid_position_bitmask_t     cur_position_mask;          //  bitmask of the current disk's position


    //  Get the rl bitmask 
    fbe_raid_group_get_rb_logging_bitmask(in_raid_group_p, &rl_bitmask);

    //  Create a bitmask for this position
    cur_position_mask = 1 << in_position; 

    //  See if the rebuild logging bit is set for this position 
    if ((rl_bitmask & cur_position_mask) == 0)
    {
        //  Bit is not set
        *b_is_rl_set_p = FBE_FALSE;
    }
    else
    {
        //  Bit is set
        *b_is_rl_set_p = FBE_TRUE; 
    }

    //  Return success 
    return FBE_STATUS_OK;

} // End fbe_raid_group_get_rb_logging()


/*!****************************************************************************
 * fbe_raid_group_get_rebuild_priority()
 ******************************************************************************
 * @brief 
 *   Gets the priority for rebuild I/O.
 *
 * @param in_raid_group_p           - pointer to the raid group object 
 *
 * @return fbe_traffic_priority_t   - rebuild priority        
 *
 ******************************************************************************/

static __forceinline fbe_traffic_priority_t fbe_raid_group_get_rebuild_priority(
                                        fbe_raid_group_t*       in_raid_group_p)
{
    //  Return the rebuild priority   
    return in_raid_group_p->rebuild_priority;

} // End fbe_raid_group_get_rebuild_priority()


/*!****************************************************************************
 * fbe_raid_group_set_rebuild_priority()
 ******************************************************************************
 * @brief 
 *   Sets the priority for rebuild I/O. 
 *
 * @param in_raid_group_p       - pointer to the raid group object 
 * @param in_priority           - rebuild priority 
 *
 * @return void       
 *
 ******************************************************************************/

static __forceinline void fbe_raid_group_set_rebuild_priority(
                                    fbe_raid_group_t*       in_raid_group_p,
                                    fbe_traffic_priority_t  in_priority)
{

    //  Set the rebuild priority to the input value    
    in_raid_group_p->rebuild_priority = in_priority;

} // End fbe_raid_group_set_rebuild_priority()


/*!****************************************************************************
 * fbe_raid_group_get_verify_ts_ptr()
 ******************************************************************************
 * @brief 
 *   This function gets a pointer to the verify tracking structure in the 
 *   raid group object. 
 *
 * @param  in_raid_group_p                      - pointer to the raid group object 
 *
 * @return fbe_raid_group_verify_tracking_t    - pointer to the RG object's rebuild 
 *                                                tracking structure 
 *
 ******************************************************************************/

static __forceinline fbe_raid_verify_tracking_t* fbe_raid_get_verify_ts_ptr(
                                        fbe_raid_group_t*           in_raid_group_p)
{

    //  Return the address of the rebuild tracking structure 
    return &in_raid_group_p->verify_ts;

} // End fbe_raid_group_get_verify_ts_ptr()


/*!****************************************************************************
 * fbe_raid_group_get_verify_priority()
 ******************************************************************************
 * @brief 
 *   Gets the priority for verify I/O.
 *
 * @param in_raid_group_p           - pointer to the raid group object 
 *
 * @return fbe_traffic_priority_t   - verify priority        
 *
 ******************************************************************************/

static __forceinline fbe_traffic_priority_t fbe_raid_group_get_verify_priority(
                                        fbe_raid_group_t*       in_raid_group_p)
{
    //  Return the verify priority   
    return in_raid_group_p->verify_priority;

} // End fbe_raid_group_get_verify_priority()


/*!****************************************************************************
 * fbe_raid_group_set_verify_priority()
 ******************************************************************************
 * @brief 
 *   Sets the priority for verify I/O. 
 *
 * @param in_raid_group_p       - pointer to the raid group object 
 * @param in_priority           - verify priority 
 *
 * @return void       
 *
 ******************************************************************************/

static __forceinline void fbe_raid_group_set_verify_priority(
                                    fbe_raid_group_t*       in_raid_group_p,
                                    fbe_traffic_priority_t  in_priority)
{

    //  Set the verify priority to the input value    
    in_raid_group_p->verify_priority = in_priority;

} // End fbe_raid_group_set_verify_priority()


/*!****************************************************************************
 *  fbe_raid_group_is_rebuilding
 ******************************************************************************
 * @brief 
 *  Check if there is a drive rebuilding by going over the non-paged metadata
 *  rebuilding positions
 *
 * @param raid_group_p       - pointer to the raid group object  
 *
 * @return fbe_bool_t        
 *
 ******************************************************************************/

static __forceinline fbe_bool_t fbe_raid_group_is_rebuilding (fbe_raid_group_t*           raid_group_p)
{
    fbe_raid_group_nonpaged_metadata_t*     nonpaged_metadata_p = NULL;        // pointer to all of nonpaged metadata 
    fbe_u32_t  i;
    fbe_bool_t is_rebuilding = FBE_FALSE;

    //  If the non-paged metadata is not initialized yet, return false
    if (fbe_raid_group_is_nonpaged_metadata_initialized(raid_group_p) == FBE_FALSE)
    {
        return FBE_FALSE;
    }

    //  Get a pointer to the non-paged metadata 
    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t*) raid_group_p, (void **)&nonpaged_metadata_p);
    
    for(i=0;i<FBE_RAID_GROUP_MAX_REBUILD_POSITIONS ;i++)
    {
        if (nonpaged_metadata_p->rebuild_info.rebuild_checkpoint_info[i].position != FBE_RAID_INVALID_DISK_POSITION )
        {
            is_rebuilding = FBE_TRUE;
            break;   
        }
    }
    
    return is_rebuilding;

} // fbe_raid_group_is_rebuilding()

/*!****************************************************************************
 * fbe_raid_group_is_degraded()
 ******************************************************************************
 * @brief 
 *   Determine if this raid group is degraded.  The RG is degraded if:
 * 
 *   - Any of the disk positions in the RG are rebuild logging, or 
 *   - Any of the disk positions in the RG have any chunks in the map marked NR 
 * 
 * @param in_raid_group_p   - pointer to the raid group object 
 *
 * @return                  - FBE_TRUE if we are a degraded raid group;
 *                            FBE_FALSE otherwise.
 *
 * Note: This function has to be positioned after
 * fbe_raid_group_get_nr_on_demand_bitmask() because it calls that function. 
 *
 ******************************************************************************/

static __forceinline fbe_bool_t fbe_raid_group_is_degraded(fbe_raid_group_t* in_raid_group_p)
{
    fbe_raid_position_bitmask_t         rl_bitmask;             //  bitmask of the current disk's position
    fbe_class_id_t                      class_id;

    // Get Class ID
    class_id = fbe_raid_group_get_class_id(in_raid_group_p);

    // If it is a striper, check to see if it is degraded and return whether RG is degraded or not.
    if (class_id == FBE_CLASS_ID_STRIPER) 
    {
        // If R10 is degraded, return TRUE; otherwise, return FALSE.
        if (fbe_raid_group_is_r10_degraded(in_raid_group_p) == FBE_TRUE) 
        {
            return FBE_TRUE;
        }

        return FBE_FALSE;
    }

    //  If any bit is set, then the RG is degraded - return TRUE.
    fbe_raid_group_get_rb_logging_bitmask(in_raid_group_p, &rl_bitmask);
    if (rl_bitmask != 0)
    {
        return FBE_TRUE;
    }

    // If RG is rebuilding, return TRUE; otherwise, return FALSE.
    if(fbe_raid_group_is_rebuilding(in_raid_group_p))
    {
        return FBE_TRUE;
    }

    return FBE_FALSE;

} // End fbe_raid_group_is_degraded()


/*!****************************************************************************
 * fbe_raid_group_io_is_degraded()
 ******************************************************************************
 * @brief 
 *   Determine if this raid group is degraded.  The RG is degraded if:
 * 
 *   - Any of the disk positions in the RG are rebuild logging, or 
 *   - Any of the disk positions in the RG have any chunks in the map marked NR
 * 
 *   ********** IMPORTANT *************  This function is used in the I/O path
 *   We should not use this function in other paths where it is possible that
 *   the NP has not been initialized yet.
 * 
 * @param raid_group_p   - pointer to the raid group object 
 *
 * @return                  - FBE_TRUE if we are a degraded raid group;
 *                            FBE_FALSE otherwise.
 *
 ******************************************************************************/
static __forceinline fbe_bool_t fbe_raid_group_io_is_degraded(fbe_raid_group_t *raid_group_p)
{
    fbe_raid_group_nonpaged_metadata_t *nonpaged_metadata_p;
    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t*) raid_group_p, (void **)&nonpaged_metadata_p);

    /* Note that we optimize for the case where we are not degraded.
     */
    if ((nonpaged_metadata_p->rebuild_info.rebuild_logging_bitmask == 0) &&
        (nonpaged_metadata_p->rebuild_info.rebuild_checkpoint_info[0].position == FBE_RAID_INVALID_DISK_POSITION) &&
        (nonpaged_metadata_p->rebuild_info.rebuild_checkpoint_info[1].position == FBE_RAID_INVALID_DISK_POSITION))
    {
        return FBE_FALSE;
    }
    return FBE_TRUE;
}
/*!****************************************************************************
 * fbe_raid_group_is_verify_needed()
 ******************************************************************************
 * @brief 
 *   Determine if this raid group is in need of a verify. 
 * 
 * @param raid_group_p - pointer to the raid group object 
 *
 * @return                  - FBE_TRUE if we are in need of a verify on this rg.
 *                            FBE_FALSE otherwise.
 *
 ******************************************************************************/
static __forceinline fbe_bool_t fbe_raid_group_is_verify_needed(fbe_raid_group_t *raid_group_p)
{
    fbe_raid_group_nonpaged_metadata_t *nonpaged_metadata_p;
    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t*) raid_group_p, (void **)&nonpaged_metadata_p);

    /* Note that we optimize for the case where we are not needing verify.
     */
    if ((nonpaged_metadata_p->error_verify_checkpoint == FBE_LBA_INVALID) &&
        (nonpaged_metadata_p->ro_verify_checkpoint == FBE_LBA_INVALID) &&
        (nonpaged_metadata_p->rw_verify_checkpoint == FBE_LBA_INVALID) &&
        (nonpaged_metadata_p->system_verify_checkpoint == FBE_LBA_INVALID) &&
        (nonpaged_metadata_p->journal_verify_checkpoint == FBE_LBA_INVALID) &&
        (nonpaged_metadata_p->incomplete_write_verify_checkpoint == FBE_LBA_INVALID))
    {
        return FBE_FALSE;
    }
    return FBE_TRUE;
}

/*!****************************************************************************
 * fbe_raid_group_is_iw_verify_needed()
 ******************************************************************************
 * @brief 
 *   Determine if this raid group is in need of a verify. 
 * 
 * @param raid_group_p - pointer to the raid group object 
 *
 * @return                  - FBE_TRUE if we are in need of a verify on this rg.
 *                            FBE_FALSE otherwise.
 *
 ******************************************************************************/
static __forceinline fbe_bool_t fbe_raid_group_is_iw_verify_needed(fbe_raid_group_t *raid_group_p)
{
    fbe_raid_group_nonpaged_metadata_t *nonpaged_metadata_p;
    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t*) raid_group_p, (void **)&nonpaged_metadata_p);

    /* Note that we optimize for the case where we are not needing verify.
     */
    if (nonpaged_metadata_p->incomplete_write_verify_checkpoint == FBE_LBA_INVALID)
    {
        return FBE_FALSE;
    }
    return FBE_TRUE;
}


/* @fn fbe_raid_group_get_verify_error_counters_ptr(fbe_raid_group_t *in_raid_p)
 *  
 * @brief 
 *   This function gets a pointer to the RAID verify error counters for the current verify I/O cycle.
 *
 * @return fbe_raid_verify_error_counts_t* - a pointer to the counter structure.
 */
static __forceinline fbe_raid_verify_error_counts_t* fbe_raid_group_get_verify_error_counters_ptr(fbe_raid_group_t *in_raid_p)
{
    return &in_raid_p->verify_ts.verify_error_counts;
}

/* @fn fbe_raid_group_get_flush_error_counters_ptr(fbe_raid_group_t *in_raid_p)
 *  
 * @brief 
 *   This function gets a pointer to the RAID write_log_flush error counters for the current flush I/O cycle.
 *
 * @return fbe_raid_flush_error_counts_t* - a pointer to the fbe_raid_flush_error_counts_t structure, which contains
 *         fbe_raid_verify_error_counts_t as its first member, so we can cast it to fbe_raid_verify_error_counts_t*, so
 *         when flush does a verify these counters will be used.
 */
static __forceinline fbe_raid_flush_error_counts_t* fbe_raid_group_get_flush_error_counters_ptr(fbe_raid_group_t *in_raid_p)
{
    return &in_raid_p->verify_ts.flush_error_counts;
}

/* @fn fbe_raid_group_set_verify_flags(fbe_raid_group_t *in_raid_p, fbe_raid_verify_flags_t in_verify_flags)
 *  
 * @brief 
 *   Function that sets verify flags in the RAID verify tracking structure.
 *
 * @return void
 */
static __forceinline void fbe_raid_group_set_verify_flags(fbe_raid_group_t *in_raid_p, fbe_raid_verify_flags_t in_verify_flags)
{
    in_raid_p->verify_ts.verify_flags = in_verify_flags;
}

/* @fn fbe_raid_group_get_verify_flags(fbe_raid_group_t *in_raid_p)
 *  
 * @brief 
 *   Function that gets the verify flags from the RAID verify tracking structure.
 *
 * @return fbe_raid_verify_flags_t - verify flags
 */
static __forceinline fbe_raid_verify_flags_t fbe_raid_group_get_verify_flags(fbe_raid_group_t *in_raid_p)
{
    return in_raid_p->verify_ts.verify_flags;
}


/*!****************************************************************************
 * fbe_raid_group_get_paged_metadata_metadata_ptr()
 ******************************************************************************
 * @brief 
 *   This function gets a pointer to the paged metadata metadata in the non-paged
 *   metadata of the raid group object. 
 *
 * @param  in_raid_group_p                      - pointer to the raid group object 
 *
 * @return fbe_raid_group_paged_metadata_t*     - pointer to the start of the paged
 *                                                metadata metadata 
 *
 ******************************************************************************/

static __forceinline fbe_raid_group_paged_metadata_t* fbe_raid_group_get_paged_metadata_metadata_ptr(
                                            fbe_raid_group_t*           in_raid_group_p)
{
    fbe_raid_group_nonpaged_metadata_t*     nonpaged_metadata_p;        // pointer to all of nonpaged metadata 


    //  Get a pointer to the non-paged metadata 
    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t*) in_raid_group_p, (void **)&nonpaged_metadata_p);

    //  Return the address of the paged metadata metadata in the non-paged metadata 
    return &nonpaged_metadata_p->paged_metadata_metadata[0];

} // End fbe_raid_group_get_paged_metadata_metadata_ptr()

/*!****************************************************************************
 * fbe_raid_group_get_rekey_checkpoint()
 ******************************************************************************
 * @brief 
 *   This function gets a pointer to the paged metadata metadata in the non-paged
 *   metadata of the raid group object. 
 *
 * @param  raid_group_p                      - pointer to the raid group object 
 *
 ******************************************************************************/

static __forceinline fbe_lba_t 
fbe_raid_group_get_rekey_checkpoint(fbe_raid_group_t *raid_group_p)
{
    fbe_raid_group_nonpaged_metadata_t*     nonpaged_metadata_p;


    //  Get a pointer to the non-paged metadata 
    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t*) raid_group_p, (void **)&nonpaged_metadata_p);

    //  Return the address of the paged metadata metadata in the non-paged metadata 
    return nonpaged_metadata_p->encryption.rekey_checkpoint;

} // End fbe_raid_group_get_rekey_checkpoint()

/*!****************************************************************************
 * fbe_raid_group_io_rekeying()
 ******************************************************************************
 * @brief 
 *   Determine if this raid group is rekeying.
 * 
 *   ********** IMPORTANT *************  This function is used in the I/O path
 *   We should not use this function in other paths where it is possible that
 *   the NP has not been initialized yet.
 * 
 * @param raid_group_p   - pointer to the raid group object 
 *
 * @return                  - FBE_TRUE if we are rekeying the raid group;
 *                            FBE_FALSE otherwise.
 *
 ******************************************************************************/
static __forceinline fbe_bool_t fbe_raid_group_io_rekeying(fbe_raid_group_t *raid_group_p)
{
    fbe_base_config_encryption_mode_t encryption_mode;
    fbe_base_config_get_encryption_mode((fbe_base_config_t*)raid_group_p, &encryption_mode);

    if (encryption_mode == FBE_BASE_CONFIG_ENCRYPTION_MODE_UNENCRYPTED) {
        return FBE_FALSE;
    }
    if ((encryption_mode == FBE_BASE_CONFIG_ENCRYPTION_MODE_ENCRYPTION_IN_PROGRESS) ||
         (encryption_mode == FBE_BASE_CONFIG_ENCRYPTION_MODE_REKEY_IN_PROGRESS)) {
        return FBE_TRUE;
    }
    return FBE_FALSE;
}
/*!**************************************************************
 * fbe_raid_group_set_chunk_size()
 ****************************************************************
 * @brief 
 *  Set the size of a chunk in the raid group bitmap.
 *
 * @param in_raid_group_p - The raid group pointer. 
 * @param in_chunk_size   - The chunk size.
 *
 * @return - void
 *
 ****************************************************************/
static __forceinline void fbe_raid_group_set_chunk_size(fbe_raid_group_t* in_raid_group_p,
                                                 fbe_chunk_size_t in_chunk_size)
{
    in_raid_group_p->chunk_size = in_chunk_size;
    return;

}   // End fbe_raid_group_set_chunk_size()


/*!**************************************************************
 * fbe_raid_group_get_chunk_size()
 ****************************************************************
 * @brief 
 *  Get the size of a chunk in the raid group bitmap.
 *
 * @param in_raid_group_p - The raid group pointer. 
 *
 * @return - The chunk size
 *
 ****************************************************************/
static __forceinline fbe_chunk_size_t fbe_raid_group_get_chunk_size(fbe_raid_group_t *in_raid_group_p)
{
    return in_raid_group_p->chunk_size;

}   // End fbe_raid_group_get_chunk_size()


/*!****************************************************************************
 * fbe_raid_group_convert_disk_lba_to_lun_lba()
 ******************************************************************************
 * @brief 
 *   Convert the disk based lba to lun lba
 *
 * @param in_raid_group_p       - pointer to the raid group object
 * @param disk_lba              - disk lba to convert
 * @param lun_lba               - lun lba
 *
 * @return  
 *
 ******************************************************************************/

static __forceinline void fbe_raid_group_convert_disk_lba_to_lun_lba(fbe_raid_group_t* in_raid_group_p,
                                                                     fbe_lba_t disk_lba,
                                                                     fbe_lba_t* lun_lba)
{
    fbe_raid_geometry_t*       raid_geometry_p;
    fbe_u16_t                  data_disks;

    // Get the number of data disks in this raid object
    raid_geometry_p = fbe_raid_group_get_raid_geometry(in_raid_group_p);
    fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);
    
    *lun_lba = disk_lba * data_disks;

    return;

} // End fbe_raid_group_convert_disk_lba_to_lun_lba()

/*!****************************************************************************
 * fbe_raid_group_convert_lun_lba_to_disk_lba()
 ******************************************************************************
 * @brief 
 *   Convert the lun lba to disk based lba
 *
 * @param in_raid_group_p       - pointer to the raid group object
 * @param lun_lba               - lun lba to convert
 * @param disk_lba              - disk lba after conversion
 *
 * @return  
 *
 ******************************************************************************/

static __forceinline void fbe_raid_group_convert_lun_lba_to_disk_lba(fbe_raid_group_t* in_raid_group_p,
                                                                     fbe_lba_t lun_lba,
                                                                     fbe_lba_t* disk_lba)
{
    fbe_raid_geometry_t*       raid_geometry_p;
    fbe_u16_t                  data_disks;

    // Get the number of data disks in this raid object
    raid_geometry_p = fbe_raid_group_get_raid_geometry(in_raid_group_p);
    fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);
    
    *disk_lba = lun_lba/data_disks;

    return;

} // End fbe_raid_group_convert_lun_lba_to_disk_lba()


/*!****************************************************************************
 * fbe_raid_group_get_max_latency_time()
 ******************************************************************************
 * @brief 
 *   Get the monitor context data. 
 *
 * @param in_raid_group_p       - pointer to the raid group object 
 *
 * @return fbe_u32_t            - monitor context data         
 *
 ******************************************************************************/

static __forceinline fbe_u64_t fbe_raid_group_get_max_latency_time(
                                    fbe_raid_group_t*      in_raid_group_p)
{
    //  Return the latency time
    return in_raid_group_p->max_raid_latency_time_is_sec;
 
} // End fbe_raid_group_get_max_latency_time()


/*!****************************************************************************
 * fbe_raid_group_set_max_latency_time()
 ******************************************************************************
 * @brief 
 *   Set the monitor context data to the given value. 
 *
 * @param in_raid_group_p       - pointer to the raid group object 
 * @param in_latency_in_sec       - max latency time we allow for getting back to ready
 *
 * @return void       
 *
 ******************************************************************************/

static __forceinline void fbe_raid_group_set_max_latency_time(
                                    fbe_raid_group_t*       in_raid_group_p,
                                    fbe_u64_t               in_latency_in_sec)
{
    //  Set the latency time   
    in_raid_group_p->max_raid_latency_time_is_sec = in_latency_in_sec;

} // End fbe_raid_group_set_max_latency_time()


/*!****************************************************************************
 * fbe_raid_group_get_check_rg_broken_timestamp()
 ******************************************************************************
 * @brief 
 *   Gets the timestamp keeping track of how long waiting for a RG that should
 *   be broken to go broken.
 *
 * @param in_raid_group_p           - pointer to the raid group object 
 *
 * @return fbe_time_t               - the timestamp      
 *
 ******************************************************************************/

static __forceinline fbe_time_t fbe_raid_group_get_check_rg_broken_timestamp(
                                        fbe_raid_group_t*       in_raid_group_p)
{
    //  Return the timestamp   
    return in_raid_group_p->last_check_rg_broken_timestamp;

} // End fbe_raid_group_get_check_rg_broken_timestamp()


/*!****************************************************************************
 * fbe_raid_group_set_check_rg_broken_timestamp()
 ******************************************************************************
 * @brief 
 *   Sets the timestamp keeping track of how long waiting for a RG that should
 *   be broken to go broken. 
 *
 * @param in_raid_group_p       - pointer to the raid group object 
 * @param in_time               - time to set timestamp to 
 *
 * @return void       
 *
 ******************************************************************************/

static __forceinline void fbe_raid_group_set_check_rg_broken_timestamp(
                                    fbe_raid_group_t*       in_raid_group_p,
                                    fbe_time_t              in_time)
{

    //  Set the timestamp to the input value    
    in_raid_group_p->last_check_rg_broken_timestamp = in_time;

} // End fbe_raid_group_set_check_rg_broken_timestamp()

static __forceinline void fbe_raid_group_set_last_download_time(fbe_raid_group_t *raid_group_p,
                                                                fbe_time_t time)
{
    raid_group_p->last_download_timestamp = time;
}
static __forceinline void fbe_raid_group_get_last_download_time(fbe_raid_group_t *raid_group_p,
                                                                fbe_time_t *time_p)
{
    *time_p = raid_group_p->last_download_timestamp;
}

void fbe_raid_group_add_to_terminator_queue(fbe_raid_group_t *raid_group_p,
                                            fbe_packet_t *packet_p);

/*!****************************************************************************
 * fbe_raid_group_remove_from_terminator_queue()
 ******************************************************************************
 * @brief 
 *   Removes the packet from the raid groups terminator queue. 
 *
 * @param raid_group_p       - pointer to the raid group object 
 * @param in_packet             - packet to remove from the queue.
 *
 * @return void       
 *
 ******************************************************************************/

static __forceinline fbe_status_t fbe_raid_group_remove_from_terminator_queue(
                                    fbe_raid_group_t*       raid_group_p,
                                    fbe_packet_t*           in_packet_p)
{
    // Add the packet to the terminator q.
    return fbe_base_object_remove_from_terminator_queue((fbe_base_object_t*) raid_group_p, in_packet_p);

} // End fbe_raid_group_remove_from_terminator_queue()

/* Accessor for the block edge.
 */
static __forceinline void fbe_raid_group_get_block_edge(fbe_raid_group_t *raid_group_p,
                                          fbe_block_edge_t **block_edge_p_p, fbe_u32_t edge_index)
{
    fbe_base_config_get_block_edge(&raid_group_p->base_config, 
                                   block_edge_p_p, 
                                   edge_index);
    return;
}

/*!****************************************************************************
 * fbe_raid_group_get_offset
 ******************************************************************************
 *
 * @brief
 *    This function is used to get the offset of the RG.
 *
 * @param   raid_group_p          -  pointer to raid object.
 * @param   offset_p             -  Pointer to the offset of the Raid object.
 *
 * @return  void
 *
 * @version
 *    03/27/2011 - Created. Ashwin Tamilarasan
 *
 ******************************************************************************/
static __forceinline void fbe_raid_group_get_offset(fbe_raid_group_t *raid_group_p,
                                      fbe_lba_t * offset_p, fbe_u32_t edge_index)
{
    fbe_block_edge_t * block_edge_p = NULL;

    fbe_raid_group_get_block_edge(raid_group_p, &block_edge_p, edge_index);    
    *offset_p = fbe_block_transport_edge_get_offset(block_edge_p);

    return;
} // End fbe_raid_group_get_offset()

/*!****************************************************************************
 *          fbe_raid_group_get_object_id()
 ******************************************************************************
 *
 * @brief   This function is used to get the object id of the raid group
 *
 * @param   raid_group_p          -  pointer to raid object.
 *
 * @return  void
 *
 * @author
 *
 ******************************************************************************/
static __forceinline fbe_object_id_t fbe_raid_group_get_object_id(fbe_raid_group_t *raid_group_p)
{
    return (((fbe_base_config_t *)raid_group_p)->base_object.object_id);

} // End fbe_raid_group_get_object_id()

/*!****************************************************************************
 * fbe_raid_group_is_in_download
 ******************************************************************************
 *
 * @brief
 *    This function is used to check whether the RG is in download.
 *
 * @param   raid_group_p    - pointer to raid object.
 *
 * @return  fbe_bool_t      - FBE_TRUE if we are eval download process;
 *                            FBE_FALSE otherwise.
 *
 * @version
 *    12/01/2011 - Created. chenl6
 *
 ******************************************************************************/
static __forceinline fbe_bool_t fbe_raid_group_is_in_download(fbe_raid_group_t *raid_group_p)
{
    if (raid_group_p->download_request_bitmask || raid_group_p->download_granted_bitmask)
        return FBE_TRUE;

    return FBE_FALSE;
} // End fbe_raid_group_is_in_download()

/*!****************************************************************************
 * fbe_raid_group_start_download_after_rebuild
 ******************************************************************************
 *
 * @brief
 *    This function is used to check whether download should be started after rebuild.
 *
 * @param   raid_group_p    - pointer to raid object.
 *
 * @return  fbe_bool_t      - FBE_TRUE if we need start download;
 *                            FBE_FALSE otherwise.
 *
 * @version
 *    12/01/2011 - Created. chenl6
 *
 ******************************************************************************/
static __forceinline fbe_bool_t fbe_raid_group_start_download_after_rebuild(fbe_raid_group_t *raid_group_p)
{
    if (raid_group_p->download_request_bitmask && !raid_group_p->download_granted_bitmask)
        return FBE_TRUE;

    return FBE_FALSE;
} // End fbe_raid_group_start_download_after_rebuild()

/*!****************************************************************************
 * fbe_raid_group_is_small_io()
 ******************************************************************************
 * @brief 
 *   Convert the disk based lba to lun lba
 *
 * @param raid_group_p       - pointer to the raid group object
 * @param lba                   - start lba
 * @param blocks                - block count
 *
 * @return  fbe_bool_t      - FBE_TRUE if it is a small IO;
 *                            FBE_FALSE otherwise.
 *
 ******************************************************************************/

static __forceinline fbe_bool_t fbe_raid_group_is_small_io(fbe_raid_group_t* raid_group_p,
                                                           fbe_lba_t lba,
                                                           fbe_block_count_t blocks)
{
    fbe_raid_geometry_t*       raid_geometry_p;
    raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);

    if (fbe_raid_group_is_verify_needed(raid_group_p))
    {
        return FBE_FALSE;    
    }

    if ((raid_geometry_p->raid_type == FBE_RAID_GROUP_TYPE_RAID5) ||
        (raid_geometry_p->raid_type == FBE_RAID_GROUP_TYPE_RAID6) ||
        (raid_geometry_p->raid_type == FBE_RAID_GROUP_TYPE_RAID3) ||
        fbe_raid_geometry_is_striper_type(raid_geometry_p)) 
    {
        if (blocks <= (raid_geometry_p->element_size - (lba % raid_geometry_p->element_size)))
        {
            return FBE_TRUE;
        }
    }
    else if (fbe_raid_geometry_is_mirror_type(raid_geometry_p))
    {
        fbe_block_count_t max_blocks = raid_geometry_p->max_blocks_per_drive;

        max_blocks = (max_blocks > 0) ? max_blocks : 0x800;
        if (blocks <= max_blocks)
        {
            return FBE_TRUE;
        }
    }

    return FBE_FALSE;

} // End fbe_raid_group_is_small_io()

static __inline fbe_status_t
fbe_raid_group_update_last_checkpoint_time(fbe_raid_group_t * raid_group_p)
{
    raid_group_p->last_checkpoint_time = fbe_get_time();
    return FBE_STATUS_OK; 
}

static __inline fbe_status_t
fbe_raid_group_get_last_checkpoint_time(fbe_raid_group_t * raid_group_p, fbe_time_t *last_checkpoint_time)
{
    *last_checkpoint_time = raid_group_p->last_checkpoint_time;
    return FBE_STATUS_OK;
}

static __inline fbe_status_t
fbe_raid_group_update_last_checkpoint_to_peer(fbe_raid_group_t * raid_group_p, fbe_lba_t checkpoint)
{
    raid_group_p->last_checkpoint_to_peer = checkpoint;
    return FBE_STATUS_OK; 
}

static __inline fbe_status_t
fbe_raid_group_get_last_checkpoint_to_peer(fbe_raid_group_t * raid_group_p, fbe_lba_t *last_checkpoint_to_peer)
{
    *last_checkpoint_to_peer = raid_group_p->last_checkpoint_to_peer;
    return FBE_STATUS_OK;
}

static __forceinline void fbe_raid_group_set_last_capacity(fbe_raid_group_t *raid_p,
                                                           fbe_lba_t last_cap)
{
    raid_p->last_capacity = last_cap;
}

static __forceinline void fbe_raid_group_get_last_capacity(fbe_raid_group_t *raid_p,
                                                           fbe_lba_t *last_cap_p)
{
    *last_cap_p = raid_p->last_capacity;
}

/* Accessors for flags.
 */
static __forceinline fbe_bool_t fbe_raid_group_bg_op_is_flag_set(fbe_raid_group_t *raid_group_p,
                                                          fbe_raid_group_bg_op_flags_t flags)
{
    if (raid_group_p->bg_info_p != NULL) {
        return ((raid_group_p->bg_info_p->flags & flags) == flags);
    }
    return FBE_FALSE;
}
static __forceinline void fbe_raid_group_bg_op_init_flags(fbe_raid_group_t *raid_group_p)
{
    if (raid_group_p->bg_info_p != NULL) {
        raid_group_p->bg_info_p->flags = 0;
    }
    return;
}
static __forceinline void fbe_raid_group_bg_op_set_flag(fbe_raid_group_t *raid_group_p,
                                                  fbe_raid_group_bg_op_flags_t flags)
{
    if (raid_group_p->bg_info_p != NULL) {
        raid_group_p->bg_info_p->flags |= flags;
    }
}

static __forceinline void fbe_raid_group_bg_op_clear_flag(fbe_raid_group_t *raid_group_p,
                                                    fbe_raid_group_bg_op_flags_t flags)
{
    if (raid_group_p->bg_info_p != NULL) {
        raid_group_p->bg_info_p->flags &= ~flags;
    }
}
static __forceinline fbe_memory_request_t *fbe_raid_group_get_bg_op_memory_request(fbe_raid_group_t *raid_group_p)
{
    if (raid_group_p->bg_info_p != NULL) {
        return &raid_group_p->bg_info_p->u.memory_request;
    }
    return NULL;
}

static __forceinline fbe_raid_group_paged_reconstruct_t *fbe_raid_group_get_bg_op_reconstruct_context(fbe_raid_group_t *raid_group_p)
{
    if (raid_group_p->bg_info_p != NULL) {
        return &raid_group_p->bg_info_p->u.reconstruct_context;
    }
    return NULL;
}

static __forceinline fbe_raid_group_bg_op_info_t *fbe_raid_group_get_bg_op_ptr(fbe_raid_group_t *raid_group_p)
{
    return raid_group_p->bg_info_p;
}
static __forceinline void fbe_raid_group_set_bg_op_ptr(fbe_raid_group_t *raid_group_p,
                                                       fbe_raid_group_bg_op_info_t *bg_info_p)
{
    raid_group_p->bg_info_p = bg_info_p;
}
static __forceinline void fbe_raid_group_bg_op_set_time(fbe_raid_group_t *raid_group_p,
                                                        fbe_time_t time)
{
    if (raid_group_p->bg_info_p != NULL) {
        raid_group_p->bg_info_p->start_time = time;
    }
}
static __forceinline fbe_time_t fbe_raid_group_bg_op_get_time(fbe_raid_group_t *raid_group_p)
{
    if (raid_group_p->bg_info_p != NULL) {
        return raid_group_p->bg_info_p->start_time;
    }
    return 0;
}

static __forceinline void fbe_raid_group_bg_op_set_pin_time(fbe_raid_group_t *raid_group_p,
                                                            fbe_time_t pin_time)
{
    if (raid_group_p->bg_info_p != NULL) {
        raid_group_p->bg_info_p->pin_time = pin_time;
    }
}
static __forceinline fbe_time_t fbe_raid_group_bg_op_get_pin_time(fbe_raid_group_t *raid_group_p)
{
    if (raid_group_p->bg_info_p != NULL) {
        return raid_group_p->bg_info_p->pin_time;
    }
    return 0;
}

/* Accessors for Extended Media Error Handling (EMEH) */
static __forceinline void fbe_raid_group_set_emeh_request(fbe_raid_group_t *raid_p,
                                                          fbe_raid_emeh_command_t emeh_request)
{
    raid_p->emeh_request = emeh_request;
}
static __forceinline void fbe_raid_group_get_emeh_request(fbe_raid_group_t *raid_p,
                                                          fbe_raid_emeh_command_t *emeh_request_p)
{
    *emeh_request_p = raid_p->emeh_request;
}
static __forceinline void fbe_raid_group_set_emeh_enabled_mode(fbe_raid_group_t *raid_p,
                                                               fbe_raid_emeh_mode_t emeh_mode)
{
    raid_p->emeh_enabled_mode = emeh_mode;
}
static __forceinline void fbe_raid_group_get_emeh_enabled_mode(fbe_raid_group_t *raid_p,
                                                               fbe_raid_emeh_mode_t *emeh_mode_p)
{
    *emeh_mode_p = raid_p->emeh_enabled_mode;
}
static __forceinline void fbe_raid_group_set_emeh_current_mode(fbe_raid_group_t *raid_p,
                                                               fbe_raid_emeh_mode_t emeh_mode)
{
    raid_p->emeh_current_mode = emeh_mode;
}
static __forceinline void fbe_raid_group_get_emeh_current_mode(fbe_raid_group_t *raid_p,
                                                               fbe_raid_emeh_mode_t *emeh_mode_p)
{
    *emeh_mode_p = raid_p->emeh_current_mode;
}
static __forceinline void fbe_raid_group_set_emeh_paco_position(fbe_raid_group_t *raid_p,
                                                                 fbe_u32_t paco_position)
{
    raid_p->emeh_paco_position = paco_position;
}
static __forceinline void fbe_raid_group_get_emeh_paco_position(fbe_raid_group_t *raid_p,
                                                                fbe_u32_t *paco_position_p)
{
    *paco_position_p = raid_p->emeh_paco_position;
}
static __forceinline void fbe_raid_group_set_emeh_reliability(fbe_raid_group_t *raid_p,
                                                              fbe_raid_emeh_reliability_t emeh_reliability)
{
    raid_p->emeh_reliability = emeh_reliability;
}
static __forceinline void fbe_raid_group_get_emeh_reliability(fbe_raid_group_t *raid_p,
                                                              fbe_raid_emeh_reliability_t *emeh_reliability_p)
{
    *emeh_reliability_p = raid_p->emeh_reliability;
}

/*!*******************************************************************
 * @struct fbe_raid_group_monitor_packet_params_t
 *********************************************************************
 * @brief Set of parameters to specify a monitor operation.
 *
 *********************************************************************/
typedef struct fbe_raid_group_monitor_packet_params_s
{
    fbe_payload_block_operation_opcode_t opcode;
    fbe_lba_t          lba;
    fbe_block_count_t  block_count;
    fbe_payload_block_operation_flags_t flags;
}
fbe_raid_group_monitor_packet_params_t;

static __inline fbe_status_t fbe_raid_group_setup_monitor_params(fbe_raid_group_monitor_packet_params_t *params_p,
                                                                 fbe_payload_block_operation_opcode_t opcode,
                                                                 fbe_lba_t          start_lba,
                                                                 fbe_block_count_t  block_count,
                                                                 fbe_payload_block_operation_flags_t flags)
{
    params_p->opcode = opcode;
    params_p->lba = start_lba;
    params_p->block_count = block_count;
    params_p->flags = flags;
    return FBE_STATUS_OK;
}
/* 
 *  Imports for visibility.
 */

/************************** 
 *fbe_raid_group_main.c 
 **************************/
void fbe_raid_group_get_default_raid_group_debug_flags(fbe_raid_group_debug_flags_t *debug_flags_p);
fbe_status_t fbe_raid_group_set_default_raid_group_debug_flags(fbe_raid_group_debug_flags_t new_default_raid_group_debug_flags);
void fbe_raid_group_get_default_object_debug_flags(fbe_object_id_t object_id,
                                                   fbe_raid_group_debug_flags_t *debug_flags_p);
void fbe_raid_group_set_default_system_debug_flags(fbe_raid_group_debug_flags_t debug_flags);
void fbe_raid_group_set_default_user_debug_flags(fbe_raid_group_debug_flags_t debug_flags);
void fbe_raid_group_persist_default_system_debug_flags(fbe_raid_group_debug_flags_t debug_flags);
void fbe_raid_group_persist_default_user_debug_flags(fbe_raid_group_debug_flags_t debug_flags);
fbe_status_t fbe_raid_group_destroy( fbe_object_handle_t object_handle);
fbe_status_t fbe_raid_group_init(fbe_raid_group_t * const raid_group_p,
                                 fbe_block_edge_t * block_edge_p,
                                 fbe_raid_common_state_t generate_state);
fbe_status_t fbe_raid_group_destroy_interface( fbe_object_handle_t object_handle);
fbe_lba_t fbe_raid_group_get_disk_capacity(fbe_raid_group_t* raid_group_p);
fbe_lba_t fbe_raid_group_get_exported_disk_capacity(fbe_raid_group_t* raid_group_p);
fbe_status_t fbe_raid_group_get_configured_capacity(fbe_raid_group_t* in_raid_group_p, fbe_lba_t* capacity_p);
fbe_status_t fbe_raid_group_get_imported_capacity(fbe_raid_group_t* in_raid_group_p, fbe_lba_t* capacity_p);
fbe_chunk_count_t fbe_raid_group_get_total_chunks(fbe_raid_group_t* raid_group_p);
fbe_chunk_count_t fbe_raid_group_get_exported_chunks(fbe_raid_group_t* raid_group_p);
fbe_time_t fbe_raid_group_get_paged_metadata_timeout_ms(fbe_raid_group_t *raid_group_p);
fbe_time_t fbe_raid_group_get_default_user_timeout_ms(fbe_raid_group_t *raid_group_p);
fbe_status_t fbe_raid_group_get_max_allowed_metadata_chunk_count(fbe_chunk_count_t *max_allowed_metadata_chunk_count);
fbe_status_t fbe_raid_group_set_default_user_timeout_ms(fbe_time_t time);
fbe_status_t fbe_raid_group_set_paged_metadata_timeout_ms(fbe_time_t time);
fbe_status_t fbe_raid_group_set_vault_wait_ms(fbe_time_t time);
fbe_time_t fbe_raid_group_get_vault_wait_ms(fbe_raid_group_t *raid_group_p);
fbe_status_t fbe_raid_group_set_encrypt_blob_blocks(fbe_u32_t blocks);
fbe_u32_t fbe_raid_group_get_encrypt_blob_blocks(fbe_raid_group_t *raid_group_p);
void fbe_raid_group_trace(fbe_raid_group_t *raid_group_p,
                          fbe_trace_level_t trace_level,
                          fbe_raid_group_debug_flags_t trace_debug_flags,
                          fbe_char_t * string_p, ...)
                          __attribute__((format(__printf_func__,4,5)));
fbe_status_t fbe_raid_group_load(void);
fbe_status_t fbe_raid_group_unload(void);
fbe_status_t
fbe_raid_group_allocate_event_stack_with_raid_lba(fbe_raid_group_t * raid_group_p,
                                                  fbe_event_context_t event_context);
fbe_status_t fbe_raid_group_get_and_clear_next_set_position(fbe_raid_group_t *raid_group_p,
                                                            fbe_raid_position_bitmask_t *clear_bitmask_p,
                                                            fbe_u32_t *edge_index_p);


/************************** 
 *fbe_raid_group_monitor.c
***************************/
fbe_lifecycle_status_t fbe_raid_group_quiesce_cond_function(fbe_raid_group_t *raid_group_p, 
                                                            fbe_packet_t * packet_p,
                                                            fbe_bool_t b_is_pass_thru_object);
fbe_lifecycle_status_t fbe_raid_group_unquiesce_cond_function(fbe_raid_group_t *raid_group_p, fbe_packet_t *packet_p);
fbe_status_t fbe_raid_group_check_hook(fbe_raid_group_t *raid_group_p,
                                       fbe_u32_t monitor_state,
                                       fbe_u32_t monitor_substate,
                                       fbe_u64_t val2,
                                       fbe_scheduler_hook_status_t *status);

fbe_status_t fbe_raid_group_monitor_run_rebuild_completion(fbe_packet_t*                   in_packet_p,
                                                           fbe_packet_completion_context_t in_context);

/* write the default nopaged metadata for the raid group object during object initialization*/
fbe_lifecycle_status_t fbe_raid_group_write_default_paged_metadata_cond_function(fbe_base_object_t * object_p,
                                                                                 fbe_packet_t * packet_p);
fbe_status_t raid_group_download_send_ack(fbe_raid_group_t * raid_group_p, 
                                          fbe_packet_t * packet_p, 
                                          fbe_raid_position_t selected_position,
                                          fbe_bool_t is_ack);
fbe_bool_t fbe_raid_group_is_write_log_flush_required(fbe_raid_group_t * raid_group_p);
/*************************** 
 *fbe_raid_group_metadata.c 
 ***************************/
fbe_status_t fbe_raid_group_initialize_metadata_positions_with_default_values(fbe_raid_group_metadata_positions_t * raid_group_metadata_positions_p);
fbe_status_t fbe_raid_group_get_metadata_positions(fbe_raid_group_t * raid_group_p, fbe_raid_group_metadata_positions_t * raid_group_metadata_positions_p);
fbe_status_t fbe_raid_group_metadata_initialize_metadata_element(fbe_raid_group_t * raid_group_p, fbe_packet_t * packet_p);
fbe_status_t fbe_raid_group_metadata_zero_metadata(fbe_raid_group_t * raid_group_p, fbe_packet_t * packet_p);
fbe_status_t fbe_raid_group_metadata_write_default_paged_metadata(fbe_raid_group_t * raid_group_p, fbe_packet_t *packet_p);
fbe_status_t fbe_raid_group_metadata_write_default_nonpaged_metadata(fbe_raid_group_t * raid_group_p, fbe_packet_t *packet_p);
fbe_status_t fbe_raid_group_nonpaged_metadata_init(fbe_raid_group_t * raid_group_p, fbe_packet_t *packet_p);
fbe_status_t fbe_raid_group_metadata_set_default_nonpaged_metadata(fbe_raid_group_t * raid_group_p,fbe_packet_t * packet_p);

fbe_status_t fbe_raid_group_metadata_get_paged_metadata_capacity(fbe_raid_group_t * raid_group_p, fbe_lba_t * paged_metadata_capacity_p);
fbe_status_t fbe_raid_group_get_NP_lock(fbe_raid_group_t*  raid_group_p,
                                        fbe_packet_t*    packet_p,
                                        fbe_packet_completion_function_t  completion_function);

fbe_status_t fbe_raid_group_release_NP_lock(fbe_packet_t*    packet_p,
                                            fbe_packet_completion_context_t context);

fbe_status_t fbe_raid_group_metadata_init_paged_metadata(fbe_raid_group_paged_metadata_t* paged_set_bits_p);
fbe_status_t fbe_raid_group_get_metadata_lba(fbe_raid_group_t *raid_group_p,
                                             fbe_chunk_index_t chunk_index,
                                             fbe_lba_t *metadata_lba_p);

fbe_status_t fbe_raid_group_metadata_clear_error_verify_required(fbe_packet_t * packet_p, 
                                                                 fbe_packet_completion_context_t context);
fbe_status_t fbe_raid_group_metadata_set_error_verify_required(fbe_raid_group_t *raid_group_p,
                                                               fbe_packet_t *packet_p);
fbe_bool_t fbe_raid_group_metadata_is_error_verify_required(fbe_raid_group_t *raid_group_p);
fbe_status_t fbe_raid_group_metadata_is_degraded_needs_rebuild(fbe_raid_group_t *raid_group_p,
                                                               fbe_bool_t *b_is_degraded_needs_rebuild);

fbe_status_t fbe_raid_group_metadata_clear_iw_verify_required(fbe_packet_t * packet_p, 
                                                                 fbe_packet_completion_context_t context);
fbe_status_t fbe_raid_group_metadata_set_iw_verify_required(fbe_raid_group_t *raid_group_p,
                                                               fbe_packet_t *packet_p);
fbe_bool_t fbe_raid_group_metadata_is_iw_verify_required(fbe_raid_group_t *raid_group_p);

fbe_status_t fbe_raid_group_metadata_write_drive_performance_tier_type(fbe_raid_group_t *raid_group_p,
                                                                       fbe_packet_t *packet_p,
                                                                       fbe_drive_performance_tier_type_np_t lowest_drive_tier);
fbe_status_t fbe_raid_group_metadata_is_journal_committed(fbe_raid_group_t *raid_group_p,
                                                          fbe_bool_t *b_committed_p);
fbe_status_t fbe_raid_group_metadata_write_4k_committed(fbe_raid_group_t *raid_group_p,
                                                        fbe_packet_t *packet_p);
fbe_status_t fbe_raid_group_change_config_quiesce(fbe_raid_group_t *raid_group_p);


/**************************** 
 * fbe_raid_group_monitor.c 
 ****************************/
fbe_status_t fbe_raid_group_monitor(fbe_raid_group_t * const raid_group_p, 
                                     fbe_packet_t * mgmt_packet_p);
fbe_lifecycle_status_t fbe_raid_group_pending_func(fbe_base_object_t *base_object_p,
                                                   fbe_packet_t * packet);
fbe_status_t fbe_raid_group_monitor_mark_specific_nr_cond_completion(
                                fbe_packet_t * packet_p,
                                fbe_packet_completion_context_t context);

/* Check if the RG is active or passive and perform any needed proceessing if passive */
fbe_status_t fbe_raid_group_monitor_check_active_state(
                                fbe_raid_group_t*               raid_group_p,
                                fbe_bool_t*                     out_is_active_b_p);
fbe_bool_t fbe_raid_group_is_opcode_monitor_operation(fbe_payload_block_operation_opcode_t opcode);

fbe_lifecycle_status_t 
raid_group_downstream_health_disabled_cond_function(fbe_base_object_t * object_p, fbe_packet_t * packet_p);

/* Check and run hooks set in raid group monitor */
fbe_status_t  fbe_check_rg_monitor_hooks(fbe_raid_group_t *raid_group_p,
                                         fbe_u32_t state,
                                         fbe_u32_t substate,
                                         fbe_scheduler_hook_status_t * status);
/* raid group zero consumed area condition completion function. */
fbe_status_t fbe_raid_group_zero_metadata_cond_completion(fbe_packet_t * packet_p,
                                                          fbe_packet_completion_context_t context);

fbe_status_t fbe_raid_group_monitor_run_verify_completion(fbe_packet_t*                   in_packet_p,
                                                          fbe_packet_completion_context_t in_context);

fbe_status_t fbe_raid_group_mark_iw_verify_update_checkpoint(fbe_packet_t * packet_p,
                                                                   fbe_packet_completion_context_t context);

fbe_status_t fbe_raid_group_mark_mdd_for_incomplete_write_verify(fbe_packet_t * packet_p, 
                                                                 fbe_packet_completion_context_t context);

fbe_status_t fbe_raid_group_zero_journal_completion(fbe_packet_t * packet_p,
                                                    fbe_packet_completion_context_t context);

fbe_status_t fbe_raid_group_monitor_update_nonpaged_drive_tier(fbe_base_object_t * object_p, fbe_packet_t * packet_p);

fbe_bool_t fbe_raid_group_is_drive_tier_query_needed(fbe_raid_group_t *raid_group_p); 
/*************************** 
 * fbe_raid_group_usurper.c 
 ***************************/
fbe_status_t fbe_raid_group_control_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet);
fbe_status_t fbe_raid_group_detach_block_edge(fbe_raid_group_t * raid_group_p, fbe_packet_t * packet_p);

fbe_status_t fbe_raid_group_usurper_get_control_buffer(
                                        fbe_raid_group_t*                   raid_group_p,
                                        fbe_packet_t*                       in_packet_p,
                                        fbe_payload_control_buffer_length_t in_buffer_length,
                                        fbe_payload_control_buffer_t*       out_buffer_pp);
fbe_status_t fbe_raid_group_create_sub_packet_and_initiate_verify(fbe_packet_t * packet_p,
                                                     fbe_base_object_t*  base_object_p);

fbe_status_t fbe_raid_group_usurper_memory_allocation_completion(fbe_memory_request_t * memory_request_p, 
                                                          fbe_memory_completion_context_t in_context);

fbe_status_t fbe_raid_group_initiate_verify_completion(fbe_packet_t* packet_p,
                                                       fbe_packet_completion_context_t context);

fbe_status_t fbe_raid_group_usurper_subpacket_completion(fbe_packet_t* packet_p,
                                                       fbe_packet_completion_context_t context);

fbe_status_t fbe_raid_group_allocate_and_build_control_operation(
                                fbe_raid_group_t*                       raid_group_p,
                                fbe_packet_t*                           in_packet_p,
                                fbe_payload_control_operation_opcode_t  in_op_code,
                                fbe_payload_control_buffer_t            in_buffer_p,
                                fbe_payload_control_buffer_length_t     in_buffer_length);

fbe_status_t fbe_raid_group_quiesce_implementation(fbe_raid_group_t * raid_group_p);

fbe_status_t fbe_raid_group_query_for_lowest_drive_tier(fbe_raid_group_t *raid_group_p, fbe_packet_t * packet_p);
fbe_status_t fbe_raid_group_usurper_set_raid_attribute(fbe_raid_group_t *raid_group_p, fbe_packet_t * packet_p); 
fbe_status_t fbe_raid_group_usurper_get_wear_level_downstream(fbe_raid_group_t *raid_group_p, fbe_packet_t *packet_p);
fbe_status_t fbe_striper_usurper_get_wear_level_downstream(fbe_raid_group_t *raid_group_p, fbe_packet_t *packet_p);
/**************************
 * fbe_raid_group_utils.c 
 **************************/
fbe_status_t fbe_raid_group_handle_transport_error(fbe_raid_group_t *raid_group_p,
                                                   fbe_status_t transport_status,
                                                   fbe_u32_t transport_qualifier,
                                                   fbe_payload_block_operation_status_t *block_status_p,
                                                   fbe_payload_block_operation_qualifier_t *block_qualifier_p,
                                                   fbe_raid_verify_error_counts_t *error_counters_p);

fbe_status_t fbe_raid_group_process_io_failure(fbe_raid_group_t *raid_group_p,
                                               fbe_packet_t *packet_p,
                                               fbe_raid_group_io_failure_type_t failure_type,
                                               fbe_payload_block_operation_t  **block_operation_pp);

fbe_status_t fbe_raid_group_utils_is_sys_rg(fbe_raid_group_t*   raid_group_p,
                                            fbe_bool_t*         sys_rg);

fbe_status_t fbe_raid_group_util_is_raid_group_in_use(fbe_raid_group_t *raid_group_p,
                                                      fbe_bool_t *b_is_raid_group_in_use_p);

fbe_raid_group_error_precedence_t fbe_raid_group_utils_get_block_operation_error_precedece(fbe_payload_block_operation_status_t status);

fbe_status_t fbe_raid_group_monitor_io_get_consumed_info(fbe_raid_group_t* raid_group_p, 
                                                         fbe_block_count_t     unconsumed_block_count,
                                                         fbe_lba_t io_start_lba, 
                                                         fbe_block_count_t* io_block_count,                                 
                                fbe_bool_t* is_consumed);
fbe_status_t fbe_raid_group_monitor_io_get_unconsumed_block_counts(fbe_raid_group_t* raid_group_p, 
                                                                   fbe_block_count_t     unconsumed_block_count,
                                                                   fbe_lba_t io_start_lba, 
                                                                   fbe_block_count_t* updated_block_count);
fbe_status_t fbe_raid_group_monitor_update_client_blk_cnt_if_needed(fbe_raid_group_t* raid_group_p, 
                                fbe_lba_t io_start_lba, fbe_block_count_t* io_block_count);
fbe_u32_t fbe_raid_group_util_count_bits_set_in_bitmask(fbe_raid_position_bitmask_t bitmask);
fbe_status_t fbe_raid_group_util_handle_too_many_dead_positions(fbe_raid_group_t *raid_group_p,
                                                                fbe_raid_iots_t *iots_p);
fbe_status_t fbe_raid_group_process_and_release_block_operation(fbe_packet_t *packet_p, 
                                                                fbe_packet_completion_context_t in_context);
fbe_bool_t fbe_raid_group_is_verify_after_write_allowed(fbe_raid_group_t *raid_group_p,
                                                        fbe_raid_iots_t *iots_p);
fbe_status_t fbe_raid_group_generate_verify_after_write(fbe_raid_group_t *raid_group_p, 
                                                        fbe_raid_iots_t *iots_p);
fbe_status_t fbe_raid_group_cleanup_verify_after_write(fbe_raid_group_t *raid_group_p,
                                                       fbe_raid_iots_t *iots_p);
fbe_status_t fbe_raid_group_should_refresh_block_sizes(fbe_raid_group_t *raid_group_p,
                                                          fbe_bool_t *b_refresh_p);

fbe_status_t fbe_raid_group_get_valid_bitmask(fbe_raid_group_t *raid_group_p, 
                                              fbe_raid_position_bitmask_t *valid_bitmask_p);
fbe_status_t fbe_raid_group_build_send_to_downstream_positions(fbe_raid_group_t *raid_group_p, 
                                                               fbe_packet_t *packet_p, 
                                                               fbe_raid_position_bitmask_t positions_to_send_to,
                                                               fbe_u32_t size_of_request);
fbe_status_t fbe_raid_group_get_send_to_downsteam_request(fbe_raid_group_t *raid_group_p, 
                                                         fbe_packet_t *packet_p, 
                                                         fbe_raid_group_send_to_downstream_positions_t **send_to_downstream_pp);
fbe_status_t fbe_raid_group_send_to_downstream_positions_completion(fbe_packet_t *packet_p,
                                                                  fbe_packet_completion_context_t context_p);
fbe_status_t fbe_raid_group_send_to_downstream_positions(fbe_raid_group_t *raid_group_p, fbe_packet_t *packet_p,
                                                         fbe_u32_t request_opcode,
                                                         fbe_u8_t *request_p,
                                                         fbe_u32_t request_size,
                                                         fbe_packet_completion_function_t completion_function,
                                                         fbe_packet_completion_context_t completion_context);

/**************************
 * fbe_raid_group_class.c 
 **************************/
fbe_status_t fbe_raid_group_class_control_entry(fbe_packet_t * packet_p);
fbe_status_t fbe_raid_group_class_get_info(fbe_packet_t * packet);
fbe_status_t fbe_raid_group_class_get_metadata_positions(fbe_lba_t exported_capacity,
                                                         fbe_lba_t imported_capacity,
                                                         fbe_u32_t data_disks,
                                                         fbe_raid_group_metadata_positions_t *raid_group_metadata_position_p,
                                                         fbe_raid_group_type_t raid_group_type);
fbe_status_t fbe_raid_group_class_validate_width(fbe_raid_group_t *raid_group_p,
                                                 fbe_u32_t requested_width);
fbe_status_t fbe_raid_group_class_validate_downstream_capacity(fbe_raid_group_t *raid_group_p,
                                                               fbe_lba_t required_blocks_per_disk);
fbe_status_t fbe_raid_group_class_get_drive_transfer_limits(fbe_raid_group_t *raid_group_p,
                                                            fbe_block_count_t *max_blocks_per_request_p);
fbe_status_t fbe_raid_group_class_get_peer_update_checkpoint_interval_ms(fbe_u32_t *peer_update_period_ms_p);
void fbe_raid_group_class_load_block_transport_parameters(void);
fbe_bool_t fbe_raid_group_class_get_throttle_enabled(void);
void fbe_raid_group_class_load_debug_flags(void);

/**************************** 
 * fbe_raid_group_emeh.c
 ****************************/
fbe_u32_t *fbe_raid_group_class_get_extended_media_error_handling_params(void);
fbe_status_t fbe_raid_group_emeh_get_class_default_mode(fbe_raid_emeh_mode_t *emeh_mode_p);
fbe_status_t fbe_raid_group_emeh_get_class_current_mode(fbe_raid_emeh_mode_t *emeh_mode_p);
fbe_status_t fbe_raid_group_emeh_set_class_current_mode(fbe_raid_emeh_mode_t set_emeh_mode);
fbe_status_t fbe_raid_group_emeh_get_class_default_increase(fbe_u32_t *threshold_increase_p);
fbe_status_t fbe_raid_group_emeh_get_class_current_increase(fbe_u32_t *threshold_increase_p);
fbe_status_t fbe_raid_group_emeh_set_class_current_increase(fbe_u32_t set_threshold_increase,
                                                            fbe_raid_emeh_command_t set_emeh_control);
fbe_bool_t fbe_raid_group_is_extended_media_error_handling_capable(fbe_raid_group_t *raid_group_p);
fbe_bool_t fbe_raid_group_is_extended_media_error_handling_enabled(fbe_raid_group_t *raid_group_p);
fbe_status_t fbe_raid_group_emeh_get_reliability_from_dieh_info(fbe_raid_group_t *raid_group_p,
                                                                fbe_pdo_dieh_media_reliability_t drive_reliability,
                                                                fbe_raid_emeh_reliability_t *emeh_reliability_p);
fbe_status_t fbe_raid_group_emeh_peer_memory_update(fbe_raid_group_t *raid_group_p);
fbe_status_t fbe_raid_group_emeh_get_dieh_completion(fbe_packet_t *packet_p, fbe_packet_completion_context_t context);
fbe_lifecycle_status_t fbe_raid_group_emeh_degraded_cond_function(fbe_raid_group_t *raid_group_p, fbe_packet_t *packet_p);
fbe_lifecycle_status_t fbe_raid_group_emeh_paco_cond_function(fbe_raid_group_t *raid_group_p, fbe_packet_t *packet_p);
fbe_lifecycle_status_t fbe_raid_group_emeh_increase_cond_function(fbe_raid_group_t *raid_group_p, fbe_packet_t *packet_p);
fbe_lifecycle_status_t fbe_raid_group_emeh_restore_default_cond_function(fbe_raid_group_t *raid_group_p, fbe_packet_t *packet_p);

/**************************** 
 * fbe_raid_group_eval_rl.c
 ****************************/

/**************************** 
 * fbe_raid_group_event.c
 ****************************/
fbe_status_t fbe_raid_group_state_change_event_entry(fbe_raid_group_t * const raid_group_p, 
                                                     fbe_event_context_t event_context);
fbe_status_t fbe_raid_group_handle_consumed_permit_request(fbe_raid_group_t *raid_group_p, fbe_event_t* in_event_p);
fbe_status_t fbe_raid_group_edge_state_change_event_entry(fbe_raid_group_t* const    raid_group_p,
                                                     fbe_event_context_t    in_event_context);
fbe_base_config_downstream_health_state_t fbe_raid_group_verify_downstream_health(fbe_raid_group_t*  raid_group_p);
fbe_status_t fbe_raid_group_set_condition_based_on_downstream_health(fbe_raid_group_t*  raid_group_p, 
                                                         fbe_base_config_downstream_health_state_t in_downstream_health_state,                                                         
                                                         fbe_path_state_t       path_state,
                                                         fbe_base_edge_t * edge_p);
fbe_status_t fbe_raid_group_determine_edge_health(
                                    fbe_raid_group_t*                       raid_group_p,
                                    fbe_base_config_path_state_counters_t*  out_edge_state_counters_p);
fbe_status_t fbe_raid_group_handle_sparing_request_event(
                                                fbe_raid_group_t*       raid_group_p,
                                                fbe_event_context_t     in_event_context);
fbe_status_t fbe_raid_group_handle_copy_request_event(fbe_raid_group_t *raid_group_p,
                                                      fbe_event_context_t event_context);
fbe_status_t fbe_raid_group_handle_abort_copy_request_event(fbe_raid_group_t *raid_group_p,
                                                       fbe_event_context_t event_context);
fbe_lifecycle_status_t fbe_raid_group_process_remap_request(
                                    fbe_raid_group_t*   raid_group_p,
                                    fbe_event_t*        data_event_p,
                                    fbe_packet_t*       packet_p);

fbe_status_t fbe_raid_group_empty_event_queue(fbe_raid_group_t*   raid_group_p);

fbe_status_t fbe_raid_group_attribute_changed(fbe_raid_group_t* const raid_group_p,
                                              fbe_event_context_t     event_context);

fbe_status_t fbe_raid_group_get_medic_action_priority(fbe_raid_group_t *in_raid_group_p, 
                                                             fbe_medic_action_priority_t *medic_action_priority);
fbe_status_t fbe_raid_group_set_upstream_medic_action_priority(fbe_raid_group_t *raid_group_p);
fbe_status_t fbe_raid_group_event_check_slf_attr(fbe_raid_group_t * raid_group_p, 
                                                 fbe_block_edge_t * edge_p,
                                                 fbe_bool_t *b_set, fbe_bool_t *b_clear);

fbe_status_t fbe_raid_group_background_op_peer_died_cleanup(fbe_raid_group_t *raid_group_p);

/****************************
 * fbe_raid_group_executor.c
 ****************************/
fbe_status_t fbe_raid_group_block_transport_entry(fbe_transport_entry_context_t context, fbe_packet_t * packet);
fbe_status_t fbe_raid_group_io_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet_p);
fbe_status_t fbe_raid_group_negotiate_block_size(fbe_raid_group_t * const raid_group_p, 
                                                 fbe_packet_t * const packet_p);
fbe_status_t fbe_raid_group_executor_send_monitor_packet(fbe_raid_group_t* raid_group_p, 
                                                         fbe_packet_t* in_packet_p,
                                                         fbe_payload_block_operation_opcode_t in_opcode,
                                                         fbe_lba_t in_start_lba,
                                                         fbe_block_count_t in_block_count);
fbe_status_t fbe_raid_group_setup_monitor_packet(fbe_raid_group_t*  raid_group_p, 
                                                 fbe_packet_t*      packet_p,
                                                 fbe_raid_group_monitor_packet_params_t *params_p);
fbe_status_t fbe_raid_group_executor_break_context_send_monitor_packet(fbe_raid_group_t*  in_raid_group_p, 
                                                         fbe_packet_t* in_packet_p,
                                                         fbe_payload_block_operation_opcode_t in_opcode,
                                                         fbe_lba_t in_start_lba,
                                                         fbe_block_count_t in_block_count);

fbe_status_t fbe_raid_group_update_block_sizes(fbe_raid_group_t * const raid_group_p, 
                                               fbe_packet_t * const packet_p);
fbe_status_t fbe_raid_group_sem_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

fbe_status_t fbe_raid_group_event_entry(fbe_object_handle_t object_handle, 
                                        fbe_event_type_t event,
                                        fbe_event_context_t event_context);
fbe_status_t fbe_raid_group_mark_nr_for_iots(fbe_raid_group_t *raid_group_p,
                                             fbe_raid_iots_t *iots_p,
                                             fbe_packet_completion_function_t completion_fn);
fbe_status_t fbe_raid_group_iots_completion(void * packet_p, fbe_packet_completion_context_t context);
fbe_status_t fbe_raid_group_get_chunk_info(fbe_raid_group_t *raid_group_p, 
                                           fbe_packet_t* packet_p,
                                           fbe_raid_iots_t *iots_p);
fbe_status_t fbe_raid_group_start_iots(fbe_raid_group_t *raid_group_p,
                                       fbe_raid_iots_t *iots_p);
fbe_status_t fbe_raid_group_start_iots_with_lock(fbe_raid_group_t *raid_group_p, 
                                                 fbe_raid_iots_t *iots_p);

fbe_status_t fbe_raid_group_iots_cleanup(void * packet_p, fbe_packet_completion_context_t context);
fbe_status_t fbe_raid_iots_proceed_cleanup(void * packet_p, fbe_packet_completion_context_t context);
fbe_status_t fbe_raid_group_iots_start_next_piece(void * packet_p, fbe_packet_completion_context_t context);
fbe_status_t fbe_raid_group_executor_handle_metadata_error(fbe_raid_group_t* raid_group_p, fbe_packet_t* in_packet_p,
                                                           fbe_raid_iots_state_t in_retry_function_p, fbe_bool_t* out_packet_failed_b_p,
                                                           fbe_status_t packet_status);
fbe_status_t fbe_raid_group_limit_and_start_iots(fbe_packet_t*  in_packet_p,
                                                      fbe_raid_group_t*  raid_group_p);
fbe_status_t fbe_raid_group_paged_metadata_iots_completion(fbe_packet_t *in_packet_p,
                                                           fbe_packet_completion_context_t     in_context);
fbe_status_t fbe_raid_group_rebuild_not_required_iots_completion(fbe_packet_t *in_packet_p,
                                                                 fbe_packet_completion_context_t     in_context);
fbe_status_t fbe_raid_group_iots_finished(fbe_raid_group_t *raid_group_p, fbe_raid_iots_t *iots_p);
fbe_status_t fbe_raid_group_mark_nr_control_operation(fbe_raid_group_t*  raid_group_p,
                                                      fbe_packet_t*      packet_p,
                                                      fbe_packet_completion_function_t completion_fn);
fbe_status_t fbe_raid_group_initiate_verify(fbe_raid_group_t * const raid_group_p, 
                                            fbe_packet_t * const packet_p);
fbe_status_t fbe_raid_group_get_stripe_lock(fbe_raid_group_t *raid_group_p, fbe_packet_t *packet_p);
fbe_status_t fbe_raid_group_handle_was_quiesced_for_next(fbe_raid_group_t *raid_group_p,
                                                         fbe_raid_iots_t *iots_p);

/****************************** 
 * fbe_raid_group_io_quiesce.c
 ******************************/
fbe_status_t fbe_raid_group_handle_aborted_packets(fbe_raid_group_t * const raid_group_p);

void fbe_raid_group_handle_continue(fbe_raid_group_t * const raid_group_p);
void fbe_raid_group_handle_stuck_io(fbe_raid_group_t * const raid_group_p);
fbe_status_t fbe_raid_group_handle_shutdown(fbe_raid_group_t * const raid_group_p);
fbe_status_t fbe_raid_group_abort_all_metadata_operations(fbe_raid_group_t * const raid_group_p);
fbe_raid_state_status_t fbe_raid_group_restart_mark_nr(fbe_raid_iots_t *iots_p);
fbe_raid_state_status_t fbe_raid_group_restart_iots(fbe_raid_iots_t *iots_p);
fbe_raid_state_status_t fbe_raid_group_iots_restart_completion(fbe_raid_iots_t *iots_p);
fbe_status_t fbe_raid_group_find_failed_io_positions(fbe_raid_group_t  *raid_group_p,
                                                     fbe_u16_t         *out_failed_io_position_bitmap_p);

fbe_status_t fbe_raid_group_calc_quiesced_count(fbe_raid_group_t * const raid_group_p);
fbe_status_t fbe_raid_group_quiesce_md_ops(fbe_raid_group_t * const raid_group_p);

fbe_status_t fbe_raid_group_count_ios(fbe_raid_group_t * const raid_group_p, 
                                    fbe_raid_group_get_statistics_t *stats_p);
void fbe_raid_group_get_quiesced_io(fbe_raid_group_t *raid_group_p, fbe_packet_t **packet_pp);
fbe_status_t fbe_raid_group_drain_usurpers(fbe_raid_group_t * const raid_group_p);
fbe_bool_t fbe_raid_group_is_request_quiesced(fbe_raid_group_t * const raid_group_p,
                                              fbe_payload_block_operation_opcode_t request_opcode);

/****************************** 
 * fbe_raid_verify.c
 ******************************/
fbe_status_t fbe_raid_verify_update_verify_bitmap_completion(
                                            fbe_packet_t*                   in_packet_p,
                                            fbe_packet_completion_context_t in_context);

fbe_status_t fbe_raid_verify_update_metadata(fbe_packet_t* in_packet_p, 
                                             fbe_raid_group_t* raid_group_p);
fbe_status_t fbe_raid_group_verify_convert_verify_type_to_verify_flags(fbe_raid_group_t*    in_raid_group_p,
                                                                fbe_raid_verify_type_t in_verify_type,
                                                                fbe_raid_verify_flags_t*   out_verify_flags_p);

fbe_raid_group_background_op_t fbe_raid_group_verify_get_opcode_for_verify(
                                        fbe_raid_group_t*           in_raid_group_p,
                                        fbe_raid_verify_flags_t     in_verify_flags);
fbe_lifecycle_status_t fbe_raid_group_verify_process_mark_verify_event(fbe_raid_group_t*   in_raid_group_p,
                                                             fbe_event_t*        in_data_event_p,
                                                             fbe_packet_t*       in_packet_p);
fbe_status_t fbe_raid_group_metadata_get_verify_chkpt_offset(fbe_raid_group_t        *raid_group_p,
                                                             fbe_raid_verify_flags_t verify_flags,
                                                             fbe_u64_t *offset_p);
fbe_lba_t fbe_raid_group_verify_get_checkpoint(fbe_raid_group_t*         raid_group_p,
                                               fbe_raid_verify_flags_t   verify_flags);
fbe_status_t fbe_raid_group_get_verify_bits_for_md_chkpt(fbe_raid_group_t *raid_group_p,
                                                         fbe_raid_verify_flags_t *verify_flags_p);
fbe_status_t fbe_raid_group_set_verify_checkpoint(fbe_raid_group_t       *raid_group_p,
                                                  fbe_packet_t            *packet_p,
                                                  fbe_raid_verify_flags_t verify_flags,
                                                  fbe_lba_t               value);
fbe_bool_t fbe_raid_group_block_opcode_is_user_initiated_verify(fbe_payload_block_operation_opcode_t block_opcode);

fbe_status_t fbe_raid_group_verify_iots_completion(fbe_packet_t *packet_p,
                                                   fbe_packet_completion_context_t context);
fbe_status_t fbe_raid_group_verify_handle_chunks(fbe_raid_group_t *raid_group_p,
                                                 fbe_packet_t *packet_p);
fbe_status_t fbe_raid_group_verify_get_unmarked_blocks(fbe_raid_group_t *raid_group_p,
                                                       fbe_raid_iots_t *iots_p,
                                                       fbe_raid_verify_flags_t verify_flags,
                                                       fbe_block_count_t *block_p);
fbe_status_t fbe_raid_group_verify_get_marked_blocks(fbe_raid_group_t *raid_group_p,
                                                     fbe_raid_iots_t *iots_p,
                                                     fbe_raid_verify_flags_t verify_flags,
                                                     fbe_block_count_t *block_p);
fbe_status_t fbe_raid_group_process_lun_extent_init_metadata(fbe_raid_group_t * raid_group_p, 
                                                             fbe_packet_t * packet_p);
fbe_status_t fbe_raid_group_verify_process_verify_request(fbe_packet_t * packet_p,
                                                          fbe_raid_group_t * raid_group_p,
                                                          fbe_chunk_index_t chunk_index,
                                                          fbe_raid_verify_type_t verify_type,
                                                          fbe_chunk_count_t chunk_count);         
fbe_status_t fbe_raid_group_verify_mark_iw_verify(fbe_packet_t * packet_p,
                                                  fbe_raid_group_t * raid_group_p);
fbe_bool_t fbe_raid_group_background_op_is_metadata_verify_need_to_run(fbe_raid_group_t*       raid_group_p,
                                                                       fbe_raid_verify_flags_t verify_flags);
fbe_status_t fbe_raid_group_verify_get_next_verify_type(fbe_raid_group_t*       in_raid_group_p,
                                                        fbe_bool_t              metadata_region,
                                                        fbe_raid_verify_flags_t *out_verify_flags_p);
fbe_status_t fbe_raid_group_verify_get_next_metadata_verify_lba(fbe_raid_group_t*       in_raid_group_p,
                                                                fbe_raid_verify_flags_t in_verify_flags,
                                                                fbe_lba_t*              out_metadata_verify_lba_p);
fbe_status_t fbe_raid_group_mark_for_verify(fbe_raid_group_t*   raid_group_p,
                                                        fbe_packet_t*       packet_p);
fbe_bool_t fbe_raid_group_is_paged_reconstruct_needed(fbe_raid_group_t *raid_group_p);
/****************************** 
 * fbe_raid_group_metadata_memory.c
 ******************************/
fbe_status_t fbe_raid_group_is_peer_flag_set(fbe_raid_group_t* raid_group_p, 
                                             fbe_bool_t* b_peer_set_p,
                                             fbe_raid_group_clustered_flags_t peer_flag);
void fbe_raid_group_get_disabled_peer_bitmap(fbe_raid_group_t * raid_group_p,
                                             fbe_raid_position_bitmask_t *disabled_pos_bitmap_p);
void fbe_raid_group_get_disabled_local_bitmap(fbe_raid_group_t * raid_group_p,
                                              fbe_raid_position_bitmask_t *disabled_pos_bitmap_p);
fbe_bool_t fbe_raid_group_is_peer_clustered_flag_set(fbe_raid_group_t * raid_group_p, 
                                                     fbe_raid_group_clustered_flags_t flags);
fbe_bool_t fbe_raid_group_is_peer_clust_flag_set_and_get_peer_bitmap(fbe_raid_group_t * raid_group_p, fbe_raid_group_clustered_flags_t flags,
                                                                     fbe_raid_position_bitmask_t *failed_drives_bitmap_p,
                                                                     fbe_raid_position_bitmask_t *failed_ios_bitmap_p);

fbe_status_t fbe_raid_group_clear_clustered_flag_and_bitmap(fbe_raid_group_t * raid_group_p, 
                                                 fbe_raid_group_clustered_flags_t flags);
fbe_status_t fbe_raid_group_clear_clustered_flag(fbe_raid_group_t * raid_group_p, 
                                                 fbe_raid_group_clustered_flags_t flags);

fbe_status_t fbe_raid_group_set_clustered_flag(fbe_raid_group_t * raid_group_p, 
                                               fbe_raid_group_clustered_flags_t flags);
fbe_status_t fbe_raid_group_set_clustered_flag_and_bitmap(fbe_raid_group_t * raid_group_p, 
                                                          fbe_raid_group_clustered_flags_t flags,
                                                          fbe_raid_position_bitmask_t failed_drives_bitmap, 
                                                          fbe_raid_position_bitmask_t failed_ios_pos_bitmap, 
                                                          fbe_raid_position_bitmask_t disabled_pos_bitmap);

fbe_bool_t fbe_raid_group_is_clustered_flag_set(fbe_raid_group_t * raid_group_p, 
                                                fbe_raid_group_clustered_flags_t flags);
fbe_bool_t fbe_raid_group_is_any_clustered_flag_set(fbe_raid_group_t * raid_group_p, fbe_raid_group_clustered_flags_t flags);
fbe_status_t fbe_raid_group_is_peer_flag_set_and_get_peer_bitmap(fbe_raid_group_t* raid_group_p, 
                                                                 fbe_bool_t* b_peer_set_p,
                                                                 fbe_lifecycle_state_t expected_state,
                                                                 fbe_raid_group_clustered_flags_t peer_flag,
                                                                 fbe_raid_position_bitmask_t   *failed_drives_bitmap_p,
                                                                 fbe_raid_position_bitmask_t   *failed_ios_bitmap_p);

void fbe_raid_group_get_local_bitmap(fbe_raid_group_t * raid_group_p, fbe_raid_position_bitmask_t *failed_drives_bitmap_p,
                                     fbe_raid_position_bitmask_t *failed_ios_pos_bitmap_p);
void fbe_raid_group_get_peer_bitmap(fbe_raid_group_t * raid_group_p, 
                                    fbe_lifecycle_state_t expected_state,
                                    fbe_raid_position_bitmask_t *failed_drives_bitmap_p,
                                    fbe_raid_position_bitmask_t *failed_ios_pos_bitmap_p);

void fbe_raid_group_get_clustered_peer_bitmap(fbe_raid_group_t * raid_group_p,
                                              fbe_raid_position_bitmask_t *failed_drives_bitmap_p,
                                              fbe_raid_position_bitmask_t *failed_ios_pos_bitmap_p);
fbe_bool_t fbe_raid_group_is_peer_present(fbe_raid_group_t *raid_group_p);  
fbe_status_t fbe_raid_group_set_clustered_bitmap(fbe_raid_group_t * raid_group_p,
                                                 fbe_raid_position_bitmask_t failed_drives_bitmap);
fbe_status_t fbe_raid_group_set_clustered_disabled_bitmap(fbe_raid_group_t * raid_group_p,
                                                          fbe_raid_position_bitmask_t disabled_pos_bitmap);
fbe_u16_t fbe_raid_group_get_local_slf_drives(fbe_raid_group_t * raid_group_p);
fbe_u16_t fbe_raid_group_get_peer_slf_drives(fbe_raid_group_t * raid_group_p);
void fbe_raid_group_set_local_slf_drives(fbe_raid_group_t * raid_group_p, fbe_u16_t num_slf_drives);
fbe_status_t fbe_raid_group_metadata_memory_set_capacity(fbe_raid_group_t * raid_group_p,    
                                                         fbe_lba_t new_cap);
fbe_u32_t fbe_raid_group_get_rebuild_speed(void);
void fbe_raid_group_set_rebuild_speed(fbe_u32_t rebuild_speed);

fbe_u32_t fbe_raid_group_get_verify_speed(void);
void fbe_raid_group_set_verify_speed(fbe_u32_t verify_speed);

fbe_status_t fbe_raid_group_encryption_notify_key_need(fbe_raid_group_t *raid_group_p, fbe_bool_t metadata_initialized);
fbe_bool_t fbe_raid_group_is_encryption_key_needed(fbe_raid_group_t *raid_group_p);
fbe_status_t fbe_raid_group_push_keys_downstream(fbe_raid_group_t *raid_group_p, fbe_packet_t * packet_p);

fbe_lifecycle_status_t fbe_raid_group_monitor_push_keys_if_needed(fbe_base_object_t * object_p, fbe_packet_t * packet_p);

fbe_status_t fbe_raid_group_request_database_encryption_key(fbe_raid_group_t *raid_group_p, fbe_packet_t * packet_p);

fbe_bool_t fbe_raid_group_is_any_peer_clustered_flag_set(fbe_raid_group_t * raid_group_p, fbe_raid_group_clustered_flags_t flags);


/**************************************
 * fbe_raid_group_journal_rebuild.c
 **************************************/
fbe_status_t fbe_raid_group_journal_rebuild(fbe_raid_group_t *raid_group_p,
                                            fbe_packet_t *packet_p,
                                            fbe_raid_position_bitmask_t positions_to_rebuild);

/**************************************
 * fbe_raid_group_journal_join.c
 **************************************/
fbe_status_t fbe_raid_group_set_join_condition_if_required(fbe_raid_group_t *raid_group_p);

/**************************************
 * fbe_raid_group_bitmap.c
 **************************************/
fbe_status_t fbe_raid_group_bitmap_get_md_of_md_count(fbe_raid_group_t *raid_group_p,
                                                      fbe_u32_t *md_of_md_count_p);

fbe_status_t fbe_raid_group_bitmap_reconstruct_paged_metadata(
                                        fbe_raid_group_t*                    in_raid_group_p,
                                        fbe_packet_t*                        in_packet_p,
                                        fbe_lba_t                            in_paged_md_lba,
                                        fbe_block_count_t                    in_paged_md_block_count,
                                        fbe_payload_block_operation_opcode_t in_op_code);

fbe_status_t fbe_raid_group_start_drain(fbe_raid_group_t *raid_group_p);

/**************************************
 * fbe_raid_group_encryption.c
 **************************************/
fbe_bool_t fbe_raid_group_encryption_is_rekey_needed(fbe_raid_group_t*    raid_group_p);
fbe_lifecycle_status_t fbe_raid_group_monitor_run_encryption(fbe_base_object_t *object_p,
                                                             fbe_packet_t *packet_p);
fbe_status_t fbe_raid_group_encryption_start(fbe_raid_group_t *raid_group_p, fbe_packet_t * packet_p);
fbe_status_t fbe_raid_group_encrypt_paged(fbe_raid_group_t *raid_group_p,
                                          fbe_packet_t *packet_p);
fbe_bool_t fbe_raid_group_is_encryption_state_set(fbe_raid_group_t *raid_group_p,
                                                 fbe_raid_group_encryption_flags_t flags);
fbe_status_t fbe_raid_group_set_paged_encrypted(fbe_packet_t * packet_p, 
                                                fbe_packet_completion_context_t context);
void fbe_raid_group_encryption_get_state(fbe_raid_group_t *raid_group_p,
                                            fbe_raid_group_encryption_flags_t *flags_p);
fbe_status_t fbe_raid_rekey_update_metadata(fbe_packet_t* packet_p, 
                                            fbe_raid_group_t* raid_group_p);

fbe_bool_t fbe_raid_group_should_read_paged_for_rekey(fbe_raid_group_t* raid_group_p,
                                                      fbe_packet_t* packet_p);
fbe_status_t fbe_raid_group_rekey_get_bitmap(fbe_raid_group_t* raid_group_p,
                                             fbe_packet_t* packet_p,
                                             fbe_raid_position_bitmask_t *bitmask_p);
fbe_lifecycle_status_t fbe_raid_group_continue_encryption(fbe_raid_group_t * raid_group_p, fbe_packet_t* packet_p);
fbe_status_t fbe_raid_group_encryption_change_passive_state(fbe_raid_group_t *raid_group_p);
fbe_status_t fbe_raid_group_check_vault(fbe_raid_group_t *raid_group_p,
                                        fbe_packet_t *packet_p);
fbe_status_t fbe_raid_group_reinitialize_vault(fbe_raid_group_t *raid_group_p,
                                              fbe_packet_t *packet_p);
fbe_status_t fbe_raid_group_notify_pvd_encryption_start(fbe_raid_group_t *raid_group_p,
                                                               fbe_packet_t *packet_p); 
fbe_status_t fbe_raid_rekey_mark_degraded(fbe_packet_t* packet_p, 
                                          fbe_raid_group_t* raid_group_p);
fbe_status_t fbe_raid_group_rekey_mark_deg_completion(fbe_packet_t *packet_p,
                                                      fbe_packet_completion_context_t context);
fbe_status_t fbe_raid_group_zero_paged_metadata(fbe_packet_t *packet_p,
                                                fbe_packet_completion_context_t context);
fbe_status_t fbe_raid_group_write_paged_metadata(fbe_packet_t *packet_p,
                                                 fbe_packet_completion_context_t context);
fbe_status_t fbe_raid_group_encrypt_paged_completion(fbe_packet_t *packet_p,
                                                     fbe_packet_completion_context_t context);
fbe_status_t fbe_raid_group_rekey_read_paged(fbe_packet_t * packet_p,                                    
                                             fbe_packet_completion_context_t context);
fbe_status_t fbe_striper_send_read_rekey(fbe_packet_t * packet_p,
                                         fbe_base_object_t * base_object_p,
                                         fbe_raid_geometry_t * raid_geometry_p);
fbe_status_t fbe_raid_group_notify_lower_encryption_start(fbe_raid_group_t *raid_group_p,
                                                          fbe_packet_t *packet_p); 
fbe_status_t fbe_raid_group_start_set_encryption_chkpt(fbe_raid_group_t *raid_group_p, 
                                                       fbe_packet_t *packet_p);

fbe_status_t fbe_striper_check_ds_encryption_state(fbe_raid_group_t* raid_group_p, fbe_base_config_encryption_state_t expected_encryption_state);
fbe_status_t fbe_striper_check_ds_encryption_chkpt(fbe_raid_group_t* raid_group_p, fbe_lba_t chkpt);

fbe_bool_t fbe_raid_group_encryption_is_active(fbe_raid_group_t *raid_group_p);
fbe_status_t fbe_raid_group_encryption_iw_check(fbe_raid_group_t *raid_group_p, 
                                                fbe_packet_t *packet_p);
fbe_status_t fbe_raid_group_rekey_write_update_paged(fbe_raid_group_t *raid_group_p,
                                                     fbe_packet_t *packet_p);
fbe_status_t fbe_raid_group_rekey_iots_completion(fbe_packet_t *packet_p,
                                                   fbe_packet_completion_context_t context);

fbe_lifecycle_status_t fbe_raid_group_monitor_run_vault_encryption(fbe_base_object_t *object_p,
                                                                   fbe_packet_t *packet_p);
fbe_status_t fbe_raid_group_get_ds_encryption_state(fbe_raid_group_t *raid_group_p,
                                                    fbe_packet_t *packet_p);

fbe_status_t fbe_raid_group_get_ds_encryption_state_alloc(fbe_raid_group_t *raid_group_p,
                                                          fbe_packet_t *packet_p);
fbe_status_t fbe_raid_group_encryption_start_mark_np(fbe_packet_t * packet_p, 
                                                     fbe_packet_completion_context_t context);
fbe_bool_t fbe_raid_group_has_identical_keys(fbe_raid_group_t *raid_group_p);

fbe_status_t fbe_raid_group_update_rekey_checkpoint(fbe_packet_t*     packet_p,
                                                    fbe_raid_group_t* raid_group_p,
                                                    fbe_lba_t         start_lba,
                                                    fbe_block_count_t block_count,
                                                    const char *caller);

fbe_status_t fbe_raid_group_encrypt_reconstruct_sm_start(fbe_raid_group_t *raid_group_p,
                                                         fbe_packet_t *packet_p,
                                                         fbe_lba_t verify_start,
                                                         fbe_lba_t verify_blocks);
fbe_status_t fbe_raid_group_encrypt_reconstruct_sm(fbe_raid_group_t *raid_group_p,
                                                   fbe_packet_t *packet_p);
fbe_status_t fbe_raid_group_reconstruct_paged_update(fbe_raid_group_t *raid_group_p,
                                                     fbe_packet_t *packet_p);
fbe_status_t fbe_raid_group_update_write_log_for_encryption(fbe_raid_group_t *raid_group_p);

fbe_status_t fbe_raid_group_c4_mirror_load_config(fbe_raid_group_t *raid_group_p, fbe_packet_t *packet_p);
fbe_bool_t fbe_raid_group_is_c4_mirror_raid_group(fbe_raid_group_t*in_raid_group_p);
fbe_status_t fbe_raid_group_c4_mirror_get_new_pvd(fbe_raid_group_t *raid_group_p, fbe_u32_t *bitmask);
fbe_status_t fbe_raid_group_c4_mirror_set_new_pvd(fbe_raid_group_t *raid_group_p,
                                                 fbe_packet_t *packet_p,
                                                 fbe_u32_t edge);
fbe_status_t fbe_raid_group_c4_mirror_is_config_loadded(fbe_raid_group_t *raid_group_p, fbe_bool_t *is_loadded);
fbe_status_t fbe_raid_group_c4_mirror_write_default(fbe_raid_group_t *raid_group_p, fbe_packet_t *packet_p);

/***********************************
 * end file fbe_raid_group_object.h
 ***********************************/
#endif
