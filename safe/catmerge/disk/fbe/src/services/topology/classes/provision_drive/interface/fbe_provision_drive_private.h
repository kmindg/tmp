#ifndef PROVISION_DRIVE_PRIVATE_H
#define PROVISION_DRIVE_PRIVATE_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_provision_drive_private.h
 ***************************************************************************
 *
 * @brief
 *  This file contains private defines for the provision drive object.
 *  This includes the definitions of the
 *  structures and defines.
 * 
 * @ingroup provision_drive_class_files
 * 
 * @version
 *   07/23/2009:  Created. Peter Puhov
 *
 ***************************************************************************/

/*
 * INCLUDE FILES:
 */

#include "csx_ext.h"
#include "fbe/fbe_transport.h"
#include "fbe_block_transport.h"
#include "fbe_base_config_private.h"
#include "fbe/fbe_provision_drive.h"
#include "fbe/fbe_raid_ts.h"
#include "fbe/fbe_time.h"
#include "fbe/fbe_encryption.h"

/*
 * MACROS:
 */

//
// Description:
//
//    Macro to trace function entry in provision drive class.
//
// Synopsis:
//
//    FBE_PROVISION_DRIVE_TRACE_FUNC_ENTRY( object_p )
//
// Parameters:
//
//    object_p  -  pointer to provision drive object
//
#define FBE_PROVISION_DRIVE_TRACE_FUNC_ENTRY(m_object_p)                   \
                                                                           \
fbe_base_object_trace( (fbe_base_object_t *)m_object_p,                    \
                       FBE_TRACE_LEVEL_DEBUG_HIGH,                         \
                       FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,                \
                       "%s entry\n", __FUNCTION__                          \
                     )


/*
 * ENUMERATION CONSTANTS:
 */

/*!*******************************************************************
 * @def FBE_PROVISION_DRIVE_MAX_TRACE_CHARS
 *********************************************************************
 * @brief max number of chars we will put in a formatted trace string.
 *
 *********************************************************************/
#define FBE_PROVISION_DRIVE_MAX_TRACE_CHARS 190

/*!*******************************************************************
 * @def FBE_PROVISION_DRIVE_KEY_INFO_TABLE_MAX
 *********************************************************************
 * @brief max number of entries in the key info table.
 *
 *********************************************************************/
#define FBE_PROVISION_DRIVE_KEY_INFO_TABLE_MAX 8

enum fbe_provision_drive_constants_e
{
    FBE_PROVISION_DRIVE_MAX_NONPAGED_RECORDS      =        1,
    FBE_PROVISION_DRIVE_CHUNK_SIZE                =    0x800,   /* in blocks (2048 blocks)      */
    FBE_PROVISION_DRIVE_MAX_CHUNKS                =    FBE_RAID_IOTS_MAX_CHUNKS, /*! @note These two values MUST match. */
    FBE_PROVISION_DRIVE_MAX_REMAP_IO_RETRIES      =        2,   /* max. retries for remap i/o's */
    FBE_PROVISION_DRIVE_PEER_CHECKPOINT_UPDATE_MS =   120000,   /* Time to wait before sending checkpoint to peer. */
    FBE_PROVISION_DRIVE_MAX_CRC_ERR_FOR_KEY_CHECK =   20, /* Arbitrary # of errors we can get before we decide keys are bad. */
    FBE_PROVISION_DRIVE_KEY0_FLAG               = 0x01,  /* Bit to set if key0 is in use.*/
    FBE_PROVISION_DRIVE_KEY1_FLAG               = 0x02,  /* Bit to set if key1 is in use. */
    FBE_PROVISION_DRIVE_KEYS_MASK               = (FBE_PROVISION_DRIVE_KEY0_FLAG | FBE_PROVISION_DRIVE_KEY1_FLAG),
    FBE_PROVISION_DRIVE_VALIDATION_BITS         = 2,
};

typedef enum fbe_provision_drive_metadata_mirror_index_e
{
    FBE_PROVISION_DRIVE_INVALID_METADATA_INDEX    =       -1,
    FBE_PRIVISION_DRIVE_PRIMARY_METADATA_INDEX    =        0,
    FBE_PROVISION_DRIVE_SECONDARY_METADATA_INDEX   =       1,
    FBE_PROVISION_DRIVE_NUMBER_OF_METADATA_COPIES =        2,
} fbe_provision_drive_metadata_mirror_index_t;


/*!****************************************************************************
 * @enum fbe_provision_drive_io_status_t
 *
 * @brief
 *    This is a classification of status for provision drive verify/remap i/o.
 *
 ******************************************************************************/

typedef enum fbe_provision_drive_io_status_s
{
    FBE_PROVISION_DRIVE_IO_STATUS_INVALID = 0,
    FBE_PROVISION_DRIVE_IO_STATUS_SUCCESS,                /*!< no error on i/o         */
    FBE_PROVISION_DRIVE_IO_STATUS_HARD_MEDIA_ERROR,       /*!< hard media error on i/o */
    FBE_PROVISION_DRIVE_IO_STATUS_SOFT_MEDIA_ERROR,       /*!< soft media error on i/o */
    FBE_PROVISION_DRIVE_IO_STATUS_ERROR,                  /*!< error on i/o            */
    FBE_PROVISION_DRIVE_IO_STATUS_LAST

} fbe_provision_drive_io_status_t;

/* Lifecycle definitions
 * Define the provision_drive lifecycle's instance.
 */
extern fbe_lifecycle_const_t FBE_LIFECYCLE_CONST_DATA(provision_drive);

/*! @enum fbe_provision_drive_lifecycle_cond_id_t  
 *  
 *  @brief These are the lifecycle conditions for the provision_drive object. 
 */
typedef enum fbe_provision_drive_lifecycle_cond_id_e 
{

    /*!< remap operation needs to be performed
     */
    FBE_PROVISION_DRIVE_LIFECYCLE_COND_DO_REMAP = FBE_LIFECYCLE_COND_FIRST_ID(FBE_CLASS_ID_PROVISION_DRIVE),

	/*!< remap operation is done - meaning upstream objects marked the chunk for verify
    */
    FBE_PROVISION_DRIVE_LIFECYCLE_COND_REMAP_DONE,

    /*!< Send a remap event upstream
    */
    FBE_PROVISION_DRIVE_LIFECYCLE_COND_SEND_REMAP_EVENT,

	/*!< Clear the event outstanding flag */
	FBE_PROVISION_DRIVE_LIFECYCLE_COND_CLEAR_EVENT_FLAG,

    /*!< it is used to set the checkpoint at logical end marker.> 
     */
    FBE_PROVISION_DRIVE_LIFECYCLE_COND_SET_ZERO_CHECKPOINT_TO_END_MARKER,    

    /*!< set end of life condition. > 
     */
    FBE_PROVISION_DRIVE_LIFECYCLE_COND_SET_END_OF_LIFE,

    /*!< set the verify end of life state condition. > 
     */
    FBE_PROVISION_DRIVE_LIFECYCLE_COND_VERIFY_END_OF_LIFE_STATE,

    /*!< It is used to update the physical drive inforamtion for the provision drive object. 
     */
    FBE_PROVISION_DRIVE_LIFECYCLE_COND_UPDATE_PHYSICAL_DRIVE_INFO,

    /*!< It is used to send and wait for firmware download to complete. >
     */
    FBE_PROVISION_DRIVE_LIFECYCLE_COND_DOWNLOAD,

    /*!< Verify if metadata record is valid or not, if not invalidate the user region >
     */
    FBE_PROVISION_DRIVE_LIFECYCLE_COND_METADATA_VERIFY_INVALIDATE,

    /*!< clear end of life condition. > 
     */
    FBE_PROVISION_DRIVE_LIFECYCLE_COND_CLEAR_END_OF_LIFE,

	/*!< The active side started a zero operation, we need to let Admin on the passice side know about it >
	*/
	FBE_PROVISION_DRIVE_LIFECYCLE_COND_ZERO_STARTED,
	/*!< The active side ended a zero operation, we need to let Admin on the passice side know about it >
	*/
	FBE_PROVISION_DRIVE_LIFECYCLE_COND_ZERO_ENDED,

    /*!< It is used to evaluate single loop failure. >
     */
    FBE_PROVISION_DRIVE_LIFECYCLE_COND_EVAL_SLF,


    /*!< Need to send a remap request as part of verify invalidate algorithm >
    */
    FBE_PROVISION_DRIVE_LIFECYCLE_COND_VERIFY_INVALIDATE_REMAP_REQUEST,

    /*!< Notifies PDO that the drive has logically come online >
    */
    FBE_PROVISION_DRIVE_LIFECYCLE_COND_NOTIFY_SERVER_LOGICALLY_ONLINE,

    /*!< It is used to verify drive fault state. >
     */
    FBE_PROVISION_DRIVE_LIFECYCLE_COND_VERIFY_DRIVE_FAULT,

    /*!< It is used to wait for health check to complete. >
     */
    FBE_PROVISION_DRIVE_LIFECYCLE_COND_HEALTH_CHECK_PASSIVE_SIDE,

    /*!< It is used to zero the paged after an encrypted drive swaps out >
     */
    FBE_PROVISION_DRIVE_LIFECYCLE_COND_PAGED_METADATA_NEEDS_ZERO,

    /*!< It is used to reconstruct (set to default) the paged metadata. >
     */
    FBE_PROVISION_DRIVE_LIFECYCLE_COND_PAGED_METADATA_RECONSTRUCT,

    /*!< It is used to clear the DDMI region. >
    */
    FBE_PROVISION_DRIVE_LIFECYCLE_COND_CLEAR_DDMI_REGION,

    /*! Set the checkpoint back to 0 if zod is set.
     */
    FBE_PROVISION_DRIVE_LIFECYCLE_COND_RESET_CHECKPOINT,

    /*! Reconstruct the metadata in ready. 
     */
    FBE_PROVISION_DRIVE_LIFECYCLE_COND_PAGED_RECONSTRUCT_READY,

    FBE_PROVISION_DRIVE_LIFECYCLE_COND_LAST /* must be last */
} 
fbe_provision_drive_lifecycle_cond_id_t;

/*! @enum fbe_provision_drive_clustered_flags_e
 *  
 *  @brief flags to indicate states of the provision drive related to dual-SP processing.
 */
enum fbe_provision_drive_clustered_flags_e {
    /*! This flag requests the peer to set download condition. 
     */
    FBE_PROVISION_DRIVE_CLUSTERED_FLAG_DOWNLOAD_REQUEST		= 0x0000000000000010,
    /*! The master side sets this when it starts download.  
     *  The slave side sets this when it sees the master side sets the bit.   
     */
    FBE_PROVISION_DRIVE_CLUSTERED_FLAG_DOWNLOAD_STARTED		= 0x0000000000000020,

    FBE_PROVISION_DRIVE_CLUSTERED_FLAG_DOWNLOAD_MASK	    = 0x0000000000000030,
	FBE_PROVISION_DRIVE_CLUSTERED_FLAG_ZERO_STARTED		= 0x0000000000000040,/*used to flag the passive side the zero started so it can notify upper layers like admin */
	FBE_PROVISION_DRIVE_CLUSTERED_FLAG_ZERO_ENDED		= 0x0000000000000080,/*used to flag the passive side the zero started so it can notify upper layers like admin*/

    /*! This flag requests the peer to set download condition. 
     */
    FBE_PROVISION_DRIVE_CLUSTERED_FLAG_EVAL_SLF_REQUEST		= 0x0000000000000100,
    FBE_PROVISION_DRIVE_CLUSTERED_FLAG_EVAL_SLF_STARTED		= 0x0000000000000200,
    FBE_PROVISION_DRIVE_CLUSTERED_FLAG_EVAL_SLF_MASK		= 0x0000000000000300,
    FBE_PROVISION_DRIVE_CLUSTERED_FLAG_SLF          		= 0x0000000000000400,
    FBE_PROVISION_DRIVE_CLUSTERED_FLAG_DRIVE_FAULT			= 0x0000000000000800,
    FBE_PROVISION_DRIVE_CLUSTERED_FLAG_CLEAR_DRIVE_FAULT	= 0x0000000000001000,

    /*! This flag requests the peer to set health check condition. 
     */
    FBE_PROVISION_DRIVE_CLUSTERED_FLAG_HEALTH_CHECK_REQUEST	= 0x0000000000002000,
    /*! The master side sets this when it starts health check.  
     *  The slave side sets this when it sees the master side sets the bit.   
     */
    FBE_PROVISION_DRIVE_CLUSTERED_FLAG_HEALTH_CHECK_STARTED	= 0x0000000000004000,

    FBE_PROVISION_DRIVE_CLUSTERED_FLAG_HEALTH_CHECK_FAILED	= 0x0000000000008000,

    FBE_PROVISION_DRIVE_CLUSTERED_FLAG_HEALTH_CHECK_MASK    = (FBE_PROVISION_DRIVE_CLUSTERED_FLAG_HEALTH_CHECK_REQUEST |
                                                               FBE_PROVISION_DRIVE_CLUSTERED_FLAG_HEALTH_CHECK_STARTED |
                                                               FBE_PROVISION_DRIVE_CLUSTERED_FLAG_HEALTH_CHECK_FAILED) ,    

    /*! This flag requests the peer to clear end of life. 
     */
    FBE_PROVISION_DRIVE_CLUSTERED_FLAG_CLEAR_END_OF_LIFE	= 0x0000000000010000,

    FBE_PROVISION_DRIVE_CLUSTERED_FLAG_SCRUB_ENDED          = 0x0000000000020000,
    /*! Masks of all peer bits.
     */
    FBE_PROVISION_DRIVE_CLUSTERED_FLAG_ALL_MASKS = (FBE_PROVISION_DRIVE_CLUSTERED_FLAG_EVAL_SLF_MASK),

	
};

typedef struct fbe_provision_drive_metadata_positions_s
{

    /*! Provision drive paged metadata LBA. 
     */
    fbe_lba_t           paged_metadata_lba;

    /*! Paged metadata block count. 
     */
    fbe_block_count_t   paged_metadata_block_count;

    /*! Mirror metadata offset of provision drive object's metadata. 
     */
    fbe_lba_t           paged_mirror_metadata_offset;


    /*! It indicates capacity of the metadata for the provision drive object. */
    fbe_lba_t           paged_metadata_capacity;

    /*! It indicates number of stripes for the PVD object. */
    fbe_u64_t           number_of_stripes;

    /*! It indicates the number of private stripes for the PVD object. */
    fbe_u64_t           number_of_private_stripes;
}fbe_provision_drive_metadata_positions_t;


/*!*******************************************************************
 * @enum fbe_provision_drive_chunk_entry_size_t
 *********************************************************************
 * @brief
 *  This enum defines the size of each provision drive chunk entry in bytes
 *
 *********************************************************************/
typedef enum fbe_provision_drive_chunk_entry_size_s
{
    FBE_PROVISION_DRIVE_CHUNK_ENTRY_SIZE = sizeof(fbe_provision_drive_paged_metadata_t),
}
fbe_provision_drive_chunk_entry_size_t;

/*!****************************************************************************
 * @struct fbe_provision_drive_verify_tracking_t
 *
 * @brief
 *    This non-persistent data structure holds pertinent verify information
 *    for the current monitor i/o cycle.
 *
 ******************************************************************************/

typedef struct fbe_provision_drive_verify_tracking_s
{
    fbe_u32_t                                remap_retry_count;   // remap retry count
    fbe_object_id_t                          pdo_object_id; // physical drive object id to which request is sent

} fbe_provision_drive_verify_tracking_t;

/*! @enum fbe_provision_drive_local_state_e
 *  
 *  @brief Provision drive local state which allows us to know where this pvd is
 *         in its processing of certain conditions.
 */
enum fbe_provision_drive_local_state_e 
{

    FBE_PVD_LOCAL_STATE_HEALTH_CHECK_REQUEST    = 0x0000000000000001,
    FBE_PVD_LOCAL_STATE_HEALTH_CHECK_STARTED    = 0x0000000000000002,    /* state for originator only */
    FBE_PVD_LOCAL_STATE_HEALTH_CHECK_WAITING    = 0x0000000000000004,    /* state for originator only */
    FBE_PVD_LOCAL_STATE_HEALTH_CHECK_DONE       = 0x0000000000000008,
    FBE_PVD_LOCAL_STATE_HEALTH_CHECK_FAILED     = 0x0000000000000010,
    FBE_PVD_LOCAL_STATE_HEALTH_CHECK_PEER_START = 0x0000000000000020,    /* state for non-originator only */    
    FBE_PVD_LOCAL_STATE_HEALTH_CHECK_PEER_FAILED =0x0000000000000040,    /* state for non-originator only */ 
    FBE_PVD_LOCAL_STATE_HEALTH_CHECK_PEER_STOP  = 0x0000000000000080,    /* state for non-originator only */ 
    FBE_PVD_LOCAL_STATE_HEALTH_CHECK_MASK       = 0x00000000000000FF,

    FBE_PVD_LOCAL_STATE_EVAL_SLF_REQUEST        = 0x0000000000000100,
    FBE_PVD_LOCAL_STATE_EVAL_SLF_STARTED        = 0x0000000000000200,
    FBE_PVD_LOCAL_STATE_EVAL_SLF_DONE           = 0x0000000000000400,
    FBE_PVD_LOCAL_STATE_EVAL_SLF_MASK           = 0x0000000000000700,
    FBE_PVD_LOCAL_STATE_SLF                     = 0x0000000000000800,
    FBE_PVD_LOCAL_STATE_SLF_MASK                = 0x0000000000000F00,

    FBE_PVD_LOCAL_STATE_JOIN_REQUEST            = 0x0000000000001000,
    FBE_PVD_LOCAL_STATE_JOIN_STARTED            = 0x0000000000002000,
    FBE_PVD_LOCAL_STATE_JOIN_DONE               = 0x0000000000004000,
    FBE_PVD_LOCAL_STATE_JOIN_MASK               = 0x000000000000F000,

    FBE_PVD_LOCAL_STATE_DOWNLOAD_REQUEST        = 0x0000000000010000,
    FBE_PVD_LOCAL_STATE_DOWNLOAD_STARTED        = 0x0000000000020000,    /* state for originator only */
    FBE_PVD_LOCAL_STATE_DOWNLOAD_PUP            = 0x0000000000040000,
    FBE_PVD_LOCAL_STATE_DOWNLOAD_DONE           = 0x0000000000080000,
    FBE_PVD_LOCAL_STATE_DOWNLOAD_PEER_START     = 0x0000000000100000,    /* state for non-originator only */
    FBE_PVD_LOCAL_STATE_DOWNLOAD_PEER_STOP      = 0x0000000000200000,    /* state for non-originator only */
    FBE_PVD_LOCAL_STATE_DOWNLOAD_TRIAL_RUN      = 0x0000000000400000,
    FBE_PVD_LOCAL_STATE_DOWNLOAD_MASK           = (FBE_PVD_LOCAL_STATE_DOWNLOAD_REQUEST |
                                                   FBE_PVD_LOCAL_STATE_DOWNLOAD_STARTED |
                                                   FBE_PVD_LOCAL_STATE_DOWNLOAD_PUP |
                                                   FBE_PVD_LOCAL_STATE_DOWNLOAD_DONE |
                                                   FBE_PVD_LOCAL_STATE_DOWNLOAD_PEER_START |
                                                   FBE_PVD_LOCAL_STATE_DOWNLOAD_PEER_STOP |
                                                   FBE_PVD_LOCAL_STATE_DOWNLOAD_TRIAL_RUN),

    FBE_PVD_LOCAL_STATE_HEALTH_CHECK_ABORT_REQ  = 0x0000000000800000,    /* originator only.  intentionally not part of HC_MASK */

	FBE_PVD_LOCAL_STATE_ZERO_STARTED            = 0x0000000001000000,    /* state for non-originator only */
    FBE_PVD_LOCAL_STATE_ZERO_ENDED              = 0x0000000002000000,
    FBE_PVD_LOCAL_STATE_SCRUB_ENDED             = 0x0000000004000000,
    FBE_PVD_LOCAL_STATE_ZERO_MASK               = 0x000000000F000000,

    FBE_PVD_LOCAL_STATE_CLEAR_EOL_REQUEST       = 0x0000000010000000,
    FBE_PVD_LOCAL_STATE_CLEAR_EOL_STARTED       = 0x0000000020000000,    /* state for originator only */
    FBE_PVD_LOCAL_STATE_CLEAR_EOL_COMPLETE      = 0x0000000040000000,
    FBE_PVD_LOCAL_STATE_CLEAR_EOL_MASK          = 0x00000000F0000000,    

    /*! NOTE, any new values created above 32bit boundary may get compiler warning C4341 in Windows. Linux should
        be okay.   Also note that the below value FBE_PVD_LOCAL_STATE_INVALID is probably being
        truncated to 32bits in windows.  Be careful writing code against it. */


    /*! Mask of all our masks (don't include EOL masks)
     */
    FBE_PVD_LOCAL_STATE_ALL_MASKS = (FBE_PVD_LOCAL_STATE_JOIN_MASK | 
                                     FBE_PVD_LOCAL_STATE_DOWNLOAD_MASK |
                                     FBE_PVD_LOCAL_STATE_EVAL_SLF_MASK),


    FBE_PVD_LOCAL_STATE_INVALID = 0xFFFFFFFFFFFFFFFF,
};

typedef fbe_u64_t fbe_provision_drive_local_state_t;

/*!****************************************************************************
 * @struct fbe_provision_drive_verify_invalidate_tracking_t
 *
 * @brief
 *    This structure is used to track which LBA is going to be invalidated and 
 * whether the upper level objects are okay with it.
 *
 ******************************************************************************/

typedef struct fbe_provision_drive_verify_invalidate_tracking_s
{
    fbe_lba_t        lba;   // LBA to invalidate 
    fbe_bool_t       invalidate_proceed; // Upper level objects said okay to invalidate

} fbe_provision_drive_verify_invalidate_tracking_t;

typedef struct fbe_provision_drive_key_info_s
{
    fbe_encryption_key_info_t *key_handle;
    fbe_key_handle_t mp_key_handle_1;
    fbe_key_handle_t mp_key_handle_2;
    fbe_object_id_t port_object_id;
    fbe_base_config_encryption_state_t encryption_state;
}fbe_provision_drive_key_info_t;

/*!****************************************************************************
 * @enum fbe_provision_drive_metadata_cache_constants_e
 *
 * @brief
 *    Enum for paged metadata cache constants.
 ******************************************************************************/
typedef enum fbe_provision_drive_metadata_cache_constants_e
{
    FBE_PROVISION_DRIVE_METADATA_CACHE_SLOT_SIZE_IN_BYTES = 16,
    FBE_PROVISION_DRIVE_METADATA_CACHE_SLOT_SIZE          = 16 * 8, /* total of 128 chunks */
    FBE_PROVISION_DRIVE_METADATA_CACHE_MAX_SLOTS          = 4,      /* 6 slots in the cache */
}fbe_provision_drive_metadata_cache_constants_t;

/*!****************************************************************************
 * @struct fbe_provision_drive_metadata_cache_slot_s
 *
 * @brief
 *    This structure is used to define slot structure of paged metadata cache. 
 ******************************************************************************/
#pragma pack(1)
typedef struct fbe_provision_drive_metadata_cache_slot_s
{
    fbe_u64_t           last_io;
    fbe_u32_t           start_chunk;
    fbe_u8_t            cached_data[FBE_PROVISION_DRIVE_METADATA_CACHE_SLOT_SIZE_IN_BYTES];
}fbe_provision_drive_metadata_cache_slot_t;
#pragma pack()

/*!****************************************************************************
 * @struct fbe_provision_drive_metadata_cache_s
 *
 * @brief
 *    This structure is the definition of paged metadata cache. 
 ******************************************************************************/
typedef struct fbe_provision_drive_metadata_cache_s
{
    /*! io counter.  */
    fbe_atomic_t        io_count;

    /*! Miss/hit counter.  */
    fbe_u32_t           miss_count;
    fbe_u32_t           hit_count;

    void *              cache_lock;
    void *              bgz_cache_lock;
    /*! Slots. */
    fbe_provision_drive_metadata_cache_slot_t slots[FBE_PROVISION_DRIVE_METADATA_CACHE_MAX_SLOTS];

    /*! Slot for Background Zeroing. */
    fbe_provision_drive_metadata_cache_slot_t bgz_slot;
}fbe_provision_drive_metadata_cache_t;

typedef struct fbe_provision_drive_unmap_bitmap_s
{
    //void *              spin_lock;
    fbe_bool_t          is_initialized;
    fbe_bool_t          is_enabled;
    fbe_u8_t *          bitmap;
}fbe_provision_drive_unmap_bitmap_t;


/*!****************************************************************************
 *    
 * @struct fbe_provision_drive_t
 *  
 * @brief 
 *   This is the definition of the provision drive object.
 *   This object represents a provision drive, which is an object which can
 *   perform block size conversions.
 ******************************************************************************/
typedef struct fbe_provision_drive_s
{    
    /*! This must be first.  This is the object we inherit from. */
    fbe_base_config_t base_config;

    /*! block edge pointer to downtream edge. */
    fbe_block_edge_t  block_edge;

    fbe_provision_drive_config_type_t  config_type;

    /*! Tracks the state related to some conditions.
     */
    fbe_provision_drive_local_state_t local_state; 

    /*! Tracks general states of the provision drive.
     */
    fbe_provision_drive_flags_t flags;

    /*! This is the optimum block size that was negotiated for in the 
     *  negotiate block size condition of the monitor. 
     */
    fbe_provision_drive_configured_physical_block_size_t configured_physical_block_size;

    /*! maximum number of the block we can send for particular i/o operation. */
    fbe_block_count_t max_drive_xfer_limit;

    /*! user usable capacity. */
    fbe_lba_t user_capacity;

    /*! The number of ms since we last did checkpoint to peer.  
     * We can't  send checkpoint to peer too often to avoid flooding CMI.
     */
    fbe_time_t last_checkpoint_time;

    fbe_provision_drive_metadata_memory_t provision_drive_metadata_memory;
    fbe_provision_drive_metadata_memory_t provision_drive_metadata_memory_peer;

    /*! sniff verify report, one for each provision drive; holds verify
     *  report information collected during sniff verify operations
     */
    /*!@todo This needs to move to either nonpaged metadata or to the 
     * separate area on the disk to store the report.
     */
    fbe_provision_drive_verify_report_t sniff_verify_report;

    /*! verify tracking structure is non-persistent data that contains
     *  pertinent verify information for the current monitor i/o cycle
     */
    fbe_provision_drive_verify_tracking_t verify_ts;

    /*! what is the priority for verify */
    fbe_traffic_priority_t  verify_priority;

     /*! what is the priority for zeroing */
    fbe_traffic_priority_t  zero_priority;

    /*! what is the priority for verify_invalidate */
   fbe_traffic_priority_t  verify_invalidate_priority;

    /*! The start time of download */
    fbe_time_t download_start_time;

    /*! The start time of ds health disabled */
    fbe_time_t ds_disabled_start_time;

    /*! The start time of edge is not optimal */
    fbe_time_t  ds_health_not_optimal_start_time;

    /*! Debug flags to allow testing of different paths in the 
     *   provision drive object.
     */
    fbe_provision_drive_debug_flags_t debug_flags;   

	fbe_u64_t monitor_counter; /* This is a temporary conter of background zero operations that was resceduled immediatelly */

    fbe_provision_drive_verify_invalidate_tracking_t verify_invalidate_ts;

	fbe_event_t permision_event;

	/*don't flood user space with zero notifications*/
	fbe_u32_t last_zero_percent_notification;

    fbe_u32_t   percent_rebuilt;

	fbe_u8_t            serial_num[FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE+1];

	fbe_metadata_stripe_lock_blob_t stripe_lock_blob;
	//fbe_metadata_stripe_lock_hash_t stripe_lock_hash;
    fbe_provision_drive_key_info_t key_info_table[FBE_PROVISION_DRIVE_KEY_INFO_TABLE_MAX];

    fbe_provision_drive_metadata_cache_t paged_metadata_cache;
    fbe_provision_drive_unmap_bitmap_t unmap_bitmap;

    /*! The time we gathered the last wear leveling data from this pvd */
    fbe_time_t  last_wear_leveling_time;

        /*Background zeroing status*/
   fbe_bool_t bg_zeroing_log_status;


    /* Lifecycle defines. */
    FBE_LIFECYCLE_DEF_INST_DATA;
    FBE_LIFECYCLE_DEF_INST_COND(FBE_LIFECYCLE_COND_MAX(FBE_PROVISION_DRIVE_LIFECYCLE_COND_LAST));
} fbe_provision_drive_t;


typedef fbe_status_t (*fbe_provision_drive_edge_handling_check_func_t)(fbe_provision_drive_paged_metadata_t * paged_data_bits_p,
                                                                     fbe_bool_t * is_edge_condition_true);

/*!*******************************************************************
 * @struct fbe_provision_drive_register_keys_contextt
 *********************************************************************
 * @brief
 *  Context to remember the keys we expect to process.
 *********************************************************************/
typedef struct fbe_provision_drive_register_keys_context_s
{
    fbe_u32_t first_handle; /*! The first handle to set. */
    fbe_u32_t client_index; /*! Client index to place the handle. */
    fbe_encryption_key_info_t *key_handle;
}
fbe_provision_drive_register_keys_context_t;

/*!*******************************************************************
 * @def FBE_PROVISION_DRIVE_REG_KEY_USER_DEBUG_FLAG
 *********************************************************************
 * @brief Name of registry key for the wear level timer
 *
 *********************************************************************/
#define FBE_PROVISION_DRIVE_REG_KEY_WEAR_LEVEL_TIMER_SEC "fbe_pvd_wear_level_timer_seconds"

/*!*******************************************************************
 * @def FBE_PROVISION_DRIVE_DEFAULT_WEAR_LEVEL_TIMER_SEC
 *********************************************************************
 * @brief default wear level timer in seconds (8 days) 8 * 24 * 60 * 60
 *          Note: Disabled for now and set to 0
 *
 *********************************************************************/
#define FBE_PROVISION_DRIVE_DEFAULT_WEAR_LEVEL_TIMER_SEC 0

/*!*******************************************************************
 * @def FBE_PROVISION_DRIVE_WEAR_LEVEL_5YR_WARRANTY_IN_HOURS
 *********************************************************************
 * @brief 5 year warranty in hours. 
 *
 *********************************************************************/
#define FBE_PROVISION_DRIVE_WEAR_LEVEL_5YR_WARRANTY_IN_HOURS 5 * 365 * 24 

/*!*******************************************************************
 * @def FBE_PROVISION_DRIVE_DEFAULT_EOL_PE_CYCLE
 *********************************************************************
 * @brief Some sata drives may not report an eol pe cycle so default to 20000 
 *
 *********************************************************************/
#define FBE_PROVISION_DRIVE_DEFAULT_EOL_PE_CYCLES 20000


/*
 * INLINE FUNCTIONS:
 */

/*!****************************************************************************
 * fbe_provision_drive_init_flags()
 ******************************************************************************
 * @brief 
 *  Initialize flags to NULL.
 *
 * @param provision_drive_p     - pointer to the provision drive object 
 *
 * @return None        
 *
 ******************************************************************************/
static __forceinline void 
fbe_provision_drive_init_flags(fbe_provision_drive_t * provision_drive_p)
{
    provision_drive_p->flags = 0;
}

/*!****************************************************************************
 * fbe_provision_drive_is_flag_set()
 ******************************************************************************
 * @brief 
 *  Check whether the specified flag is set.
 *
 * @param provision_drive_p     - pointer to the provision drive object 
 * @param flags                 - flags to check 
 *
 * @return fbe_bool_t        
 *
 ******************************************************************************/
static __forceinline fbe_bool_t 
fbe_provision_drive_is_flag_set(fbe_provision_drive_t * provision_drive_p, fbe_provision_drive_flags_t flags)
{
    fbe_bool_t is_flag_set = FBE_FALSE;
    if(((provision_drive_p->flags & flags) == flags)) {
        is_flag_set = FBE_TRUE;
    }
    return is_flag_set;
}

/*!****************************************************************************
 * fbe_provision_drive_set_flag()
 ******************************************************************************
 * @brief 
 *  Set a flag in the provision drive.
 *
 * @param provision_drive_p     - pointer to the provision drive object 
 * @param flags                 - flags to set 
 *
 * @return None        
 *
 ******************************************************************************/
static __forceinline void 
fbe_provision_drive_set_flag(fbe_provision_drive_t * provision_drive_p, fbe_provision_drive_flags_t flags)
{
    provision_drive_p->flags |= flags;
}

/*!****************************************************************************
 * fbe_provision_drive_clear_flag()
 ******************************************************************************
 * @brief 
 *  Clear a flag in provision drive.
 *
 * @param provision_drive_p     - pointer to the provision drive object 
 * @param flags                 - flags to clear 
 *
 * @return None        
 *
 ******************************************************************************/
static __forceinline void 
fbe_provision_drive_clear_flag(fbe_provision_drive_t * provision_drive_p, fbe_provision_drive_flags_t flags)
{
    provision_drive_p->flags &= ~flags;
}

/*!****************************************************************************
 * fbe_provision_drive_get_zero_priority()
 ******************************************************************************
 * @brief 
 *  Gets the priority for zero I/O.
 *
 * @param provision_drive_p     - pointer to the provision drive object 
 *
 * @return fbe_traffic_priority_t   - rebuild priority        
 *
 ******************************************************************************/

static __forceinline fbe_traffic_priority_t
fbe_provision_drive_get_zero_priority(fbe_provision_drive_t * provision_drive_p)
{
    return provision_drive_p->zero_priority;
}

/*!****************************************************************************
 * fbe_provision_drive_set_zero_priority()
 ******************************************************************************
 * @brief 
 *  Sets the priority for zero I/O. 
 *
 * @param provision_drive_p     - pointer to the provision drive object 
 * @param traffic_priority      - zero priority.
 *
 * @return void  
 *
 ******************************************************************************/

static __forceinline void
fbe_provision_drive_set_zero_priority(fbe_provision_drive_t * provision_drive_p,
                                      fbe_traffic_priority_t  zero_priority)
{
    provision_drive_p->zero_priority = zero_priority;

}

/*!****************************************************************************
 * fbe_provision_drive_get_verify_invalidate_priority()
 ******************************************************************************
 * @brief 
 *  Gets the priority for verify invalidate I/O.
 *
 * @param provision_drive_p     - pointer to the provision drive object 
 *
 * @return fbe_traffic_priority_t   - rebuild priority        
 *
 ******************************************************************************/

static __forceinline fbe_traffic_priority_t
fbe_provision_drive_get_verify_invalidate_priority(fbe_provision_drive_t * provision_drive_p)
{
    return provision_drive_p->verify_invalidate_priority;
}

/*!****************************************************************************
 * fbe_provision_drive_set_verify_invalidate_priority()
 ******************************************************************************
 * @brief 
 *  Sets the priority for verify invalidate I/O. 
 *
 * @param provision_drive_p     - pointer to the provision drive object 
 * @param traffic_priority      - priority.
 *
 * @return void  
 *
 ******************************************************************************/

static __forceinline void
fbe_provision_drive_set_verify_invalidate_priority(fbe_provision_drive_t * provision_drive_p,
                                                   fbe_traffic_priority_t  verify_invalidate_priority)
{
    provision_drive_p->verify_invalidate_priority = verify_invalidate_priority;

}
/*!****************************************************************************
 * fbe_provision_drive_set_verify_invalidate_ts_lba()
 ******************************************************************************
 * @brief 
 *  Sets the current LBA that is being processed by the verif invalidate code 
 *
 * @param provision_drive_p     - pointer to the provision drive object 
 * @param lba      - lba being processed.
 *
 * @return void  
 *
 ******************************************************************************/

static __forceinline void
fbe_provision_drive_set_verify_invalidate_ts_lba(fbe_provision_drive_t * provision_drive_p,
                                                 fbe_lba_t  lba)
{
    provision_drive_p->verify_invalidate_ts.lba = lba;

}

/*!****************************************************************************
 * fbe_provision_drive_get_verify_invalidate_ts_lba()
 ******************************************************************************
 * @brief 
 *  Returns the current LBA that is being processed by the verif invalidate code 
 *
 * @param provision_drive_p     - pointer to the provision drive object 
 * 
 * @return void  
 *
 ******************************************************************************/

static __forceinline fbe_lba_t
fbe_provision_drive_get_verify_invalidate_ts_lba(fbe_provision_drive_t * provision_drive_p)
{
    return(provision_drive_p->verify_invalidate_ts.lba);

}

/*!****************************************************************************
 * fbe_provision_drive_set_verify_invalidate_proceed()
 ******************************************************************************
 * @brief 
 *  Sets the boolean to indicate if we can proceed with the verify invalidate
 *
 * @param provision_drive_p     - pointer to the provision drive object 
 * @param proceed      - FBE_TRUE - Proceed, FBE_FALSE - Dont Proceed
 *
 * @return void  
 *
 ******************************************************************************/

static __forceinline void
fbe_provision_drive_set_verify_invalidate_proceed(fbe_provision_drive_t * provision_drive_p,
                                                  fbe_bool_t  proceed)
{
    provision_drive_p->verify_invalidate_ts.invalidate_proceed = proceed;

}

/*!****************************************************************************
 * fbe_provision_drive_get_verify_invalidate_proceed()
 ******************************************************************************
 * @brief 
 *  Sets the boolean to indicate if we can proceed with the verify invalidate
 *
 * @param provision_drive_p     - pointer to the provision drive object 
 *
 * @return void  
 *
 ******************************************************************************/

static __forceinline fbe_bool_t
fbe_provision_drive_get_verify_invalidate_proceed(fbe_provision_drive_t * provision_drive_p)
{
    return(provision_drive_p->verify_invalidate_ts.invalidate_proceed);

}
/*!****************************************************************************
 * fbe_provision_drive_clear_verify_enable
 ******************************************************************************
 *
 * @brief
 *    This function clears sniff verify enable flag in the non-paged metadata
 *    for the specified provision drive.
 *
 * @param   in_provision_drive_p  -  pointer to provision drive
 *
 * @return  void
 *
 * @version
 *    02/12/2010 - Created. Randy Black
 *
 ******************************************************************************/

static __forceinline void
fbe_provision_drive_clear_verify_enable(fbe_provision_drive_t* in_provision_drive_p)
{
    // clear verify enable flag in provision drive's non-paged metadata
    fbe_provision_drive_clear_flag(in_provision_drive_p, FBE_PROVISION_DRIVE_FLAG_SNIFF_VERIFY_ENABLED);

    return;
}   // end fbe_provision_drive_clear_verify_enable()


/*!****************************************************************************
 * fbe_provision_drive_get_pdo_object_id
 ******************************************************************************
 *
 * @brief
 *    This function gets the pdo object id for the current monitor i/o
 *    cycle.
 *
 * @param   in_provision_drive_p   -  pointer to provision drive
 * @param   pdo_object_id  -  physical drive object id output parameter
 *
 * @return  void
 *
 * @version
 *    09/13/2010 - Created. Pradnya Patankar
 *
 ******************************************************************************/

static __forceinline void
fbe_provision_drive_get_pdo_object_id(
                                         fbe_provision_drive_t* in_provision_drive_p,
                                         fbe_object_id_t*  pdo_object_id
                                       )
{
    // get media error lba for current monitor i/o cycle
    *pdo_object_id = in_provision_drive_p->verify_ts.pdo_object_id;

    return;
}   // end fbe_provision_drive_get_pdo_object_id()


/*!****************************************************************************
 * fbe_provision_drive_get_verify_enable
 ******************************************************************************
 *
 * @brief
 *    This function gets the sniff verify enable flag in the non-paged metadata
 *    for the specified provision drive.
 *
 * @param   in_provision_drive_p  -  pointer to provision drive
 * @param   out_verify_enable_p   -  verify enable flag output parameter
 *
 * @return  void
 *
 * @version
 *    02/12/2010 - Created. Randy Black
 *
 ******************************************************************************/

static __forceinline void
fbe_provision_drive_get_verify_enable(
                                       fbe_provision_drive_t* in_provision_drive_p,
                                       fbe_bool_t*            out_verify_enable_p
                                     )
{
    // get verify enable flag in provision drive's non-paged metadata
    *out_verify_enable_p = fbe_provision_drive_is_flag_set(in_provision_drive_p, FBE_PROVISION_DRIVE_FLAG_SNIFF_VERIFY_ENABLED);

    return;
}   // end fbe_provision_drive_get_verify_enable()



/*!****************************************************************************
 * fbe_provision_drive_get_verify_report_ptr
 ******************************************************************************
 *
 * @brief
 *    This function gets the address of the sniff verify report in non-paged
 *    metadata for the specified provision drive.
 *
 * @param   in_provision_drive_p  -  pointer to provision drive
 * @param   out_verify_enable_pp  -  pointer to verify report pointer output
 *                                   parameter
 *
 * @return  void
 *
 * @version
 *    02/12/2010 - Created. Randy Black
 *
 ******************************************************************************/

static __forceinline void
fbe_provision_drive_get_verify_report_ptr(
                                           fbe_provision_drive_t*                in_provision_drive_p,
                                           fbe_provision_drive_verify_report_t** out_verify_report_pp
                                         )
{
    // get address of verify report in provision drive's non-paged metadata
    *out_verify_report_pp = &in_provision_drive_p->sniff_verify_report;

    return;
}   // end fbe_provision_drive_get_verify_report_ptr()


/*!****************************************************************************
 * fbe_provision_drive_lock
 ******************************************************************************
 *
 * @brief
 *    This function locks the specified provision drive for exclusive access
 *    via the base object lock.
 *
 * @param   in_provision_drive_p   -  pointer to provision drive
 *
 * @return  void
 *
 * @version
 *    02/12/2010 - Created. Randy Black
 *
 ******************************************************************************/

static __forceinline void
fbe_provision_drive_lock(
                          fbe_provision_drive_t* const in_provision_drive_p
                        )
{
    // lock this provision drive
    fbe_base_object_lock( (fbe_base_object_t *)in_provision_drive_p );

    return;
}   // end fbe_provision_drive_lock()


/*!****************************************************************************
 * fbe_provision_drive_set_pdo_object_id
 ******************************************************************************
 *
 * @brief
 *    This function sets the physical drive object id for the current monitor i/o
 *    cycle.
 *
 * @param   in_provision_drive_p  -  pointer to provision drive
 * @param   pdo_object_id    -  physical drive object id
 *
 * @return  void
 *
 * @version
 *    09/13/2010 - Created. Pradnya Patankar
 *
 ******************************************************************************/

static __forceinline void
fbe_provision_drive_set_pdo_object_id(
                                         fbe_provision_drive_t* in_provision_drive_p,
                                         fbe_object_id_t pdo_object_id
                                       )
{
    // set pdo_object_id for current monitor i/o cycle
    in_provision_drive_p->verify_ts.pdo_object_id = pdo_object_id;

    return;
}   // end fbe_provision_drive_set_pdo_object_id()



/*!****************************************************************************
 * fbe_provision_drive_set_verify_enable
 ******************************************************************************
 *
 * @brief
 *    This function sets sniff verify enable flag in the non-paged metadata for
 *    the specified provision drive.
 *
 * @param   in_provision_drive_p  -  pointer to provision drive
 *
 * @return  void
 *
 * @version
 *    02/12/2010 - Created. Randy Black
 *
 ******************************************************************************/

static __forceinline void
fbe_provision_drive_set_verify_enable(
                                       fbe_provision_drive_t* in_provision_drive_p
                                     )
{
    // set verify enable flag in provision drive's non-paged metadata
    fbe_provision_drive_set_flag(in_provision_drive_p, FBE_PROVISION_DRIVE_FLAG_SNIFF_VERIFY_ENABLED);

    return;
}   // end fbe_provision_drive_set_verify_enable()


/*!****************************************************************************
 * fbe_provision_drive_get_remap_retry_count
 ******************************************************************************
 *
 * @brief
 *    This function gets sniff retry count for the current monitor
 *    i/o cycle.
 *
 * @param   in_provision_drive_p  -  pointer to provision drive
 * @param   out_retry_count_p     -  remap retry count output parameter
 *
 * @return  void
 *
 *
 ******************************************************************************/

static __forceinline void
fbe_provision_drive_get_remap_retry_count(
                                            fbe_provision_drive_t* in_provision_drive_p,
                                            fbe_block_count_t*     out_retry_count_p
                                          )
{
    // get remap retry count for current monitor i/o cycle
    *out_retry_count_p = in_provision_drive_p->verify_ts.remap_retry_count;

    return;
}   // end fbe_provision_drive_get_remap_retry_count()


/*!****************************************************************************
 * fbe_provision_drive_increment_remap_retry_count
 ******************************************************************************
 *
 * @brief
 *    This function increments the remap retry count for the current
 *    monitor i/o cycle.
 *
 * @param   in_provision_drive_p  -  pointer to provision drive
 *
 * @return  void
 *
 *
 ******************************************************************************/

static __forceinline void
fbe_provision_drive_increment_remap_retry_count(
                                                  fbe_provision_drive_t* in_provision_drive_p
                                                )
{
    // increment remap retry count for current monitor i/o cycle
    in_provision_drive_p->verify_ts.remap_retry_count++;

    return;
}   // end fbe_provision_drive_increment_remap_retry_count()

/*!****************************************************************************
 * fbe_provision_drive_reset_remap_retry_count
 ******************************************************************************
 *
 * @brief
 *    This function resets the remap retry count 
 *
 * @param   in_provision_drive_p  -  pointer to provision drive
 *
 * @return  void
 *
 *
 ******************************************************************************/

static __forceinline void
fbe_provision_drive_reset_remap_retry_count(fbe_provision_drive_t* in_provision_drive_p)
{
    // reset remap retry count for current monitor i/o cycle
    in_provision_drive_p->verify_ts.remap_retry_count = 0;

    return;
}   // end fbe_provision_drive_reset_remap_retry_count()


/*!****************************************************************************
 * fbe_provision_drive_unlock
 ******************************************************************************
 *
 * @brief
 *    This function unlocks the specified provision drive via the base object
 *    lock.
 *
 * @param   in_provision_drive_p   -  pointer to provision drive
 *
 * @return  none
 *
 * @version
 *    02/12/2010 - Created. Randy Black
 *
 ******************************************************************************/

static __forceinline void
fbe_provision_drive_unlock(fbe_provision_drive_t* const in_provision_drive_p)
{
    // unlock this provision drive
    fbe_base_object_unlock( (fbe_base_object_t *)in_provision_drive_p );

    return;
}   // end fbe_provision_drive_unlock()

/*!****************************************************************************
 *   fbe_provision_drive_set_debug_flag
 ******************************************************************************
 *
 * @brief
 *    This function sets the pvd debug flag value.
 *    lock.
 *
 * @param   provision_drive_p   -  pointer to provision drive
 * @param   debug_flags         -   pvd debug flag value to set 
 *
 * @return  none
 *
 * @version
 *    09/21/2010 - Created. Amit Dhaduk
 *
 ******************************************************************************/
static __forceinline void
fbe_provision_drive_set_debug_flag(fbe_provision_drive_t *provision_drive,
                                   fbe_provision_drive_debug_flags_t debug_flags)
{
    provision_drive->debug_flags = debug_flags;
    
} 
/******************************************************************************
 * end fbe_provision_drive_set_debug_flag()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_get_debug_flag
 ******************************************************************************
 *
 * @brief
 *    This function returns pvd debug flag value.
 *    
 *
 * @param   provision_drive_p   -  pointer to provision drive
 * @param   debug_flags_p      -   pointer to return pvd debug value
 *
 * @return  none
 *
 * @version
 *    09/21/2010 - Created. Amit Dhaduk
 *
 ******************************************************************************/
static __forceinline void
fbe_provision_drive_get_debug_flag(fbe_provision_drive_t *provision_drive,
                                   fbe_provision_drive_debug_flags_t *debug_flags_p)
{
    *debug_flags_p = provision_drive->debug_flags;
    
} 
/******************************************************************************
 * end fbe_provision_drive_get_debug_flag()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_is_pvd_debug_flag_set
 ******************************************************************************
 *
 * @brief
 *    This function checks if the given pvd debug flag value is set or not.
 *    
 *
 * @param   provision_drive_p   -  pointer to provision drive
 * @param   debug_flags      -   pvd debug flag to check if set
 *
 * @return  FBE_TRUE - if given pvd flag is set
 *              FBE_FALSE - if given pvd flag is not set 
 *
 * @version
 *    09/21/2010 - Created. Amit Dhaduk
 *
 ******************************************************************************/
static __forceinline fbe_bool_t
fbe_provision_drive_is_pvd_debug_flag_set(fbe_provision_drive_t *provision_drive_p,
                                          fbe_provision_drive_debug_flags_t debug_flags)
{
    if (provision_drive_p == NULL)
    {
        return FBE_FALSE;
    }
    return ((provision_drive_p->debug_flags & debug_flags) == debug_flags);
    
} 
/******************************************************************************
 * end fbe_provision_drive_is_pvd_debug_flag_set()
 ******************************************************************************/

/*!****************************************************************************
 *   fbe_provision_drive_set_percent_rebuilt()
 ******************************************************************************
 *
 * @brief
 *    This function sets the pvd percent rebuilt.
 *
 * @param   provision_drive_p   -  pointer to provision drive
 * @param   percent_rebuilt - opaque percent rebuilt
 *
 * @return  none
 *
 ******************************************************************************/
static __forceinline void
fbe_provision_drive_set_percent_rebuilt(fbe_provision_drive_t *provision_drive,
                                        fbe_u32_t percent_rebuilt)
{
    provision_drive->percent_rebuilt = percent_rebuilt;
    
} 
/******************************************************************************
 * end fbe_provision_drive_set_percent_rebuilt()
 ******************************************************************************/

/*!****************************************************************************
 *   fbe_provision_drive_get_percent_rebuilt()
 ******************************************************************************
 *
 * @brief
 *    This function gets the pvd percent rebuilt.
 *
 * @param   provision_drive_p   -  pointer to provision drive
 * @param   percent_rebuilt_p - opaque percent rebuilt
 *
 * @return  none
 *
 ******************************************************************************/
static __forceinline void
fbe_provision_drive_get_percent_rebuilt(fbe_provision_drive_t *provision_drive,
                                        fbe_u32_t *percent_rebuilt_p)
{
    *percent_rebuilt_p = provision_drive->percent_rebuilt;
    
} 
/******************************************************************************
 * end fbe_provision_drive_get_percent_rebuilt()
 ******************************************************************************/

/* Accessors related to local state.
 */
static __forceinline fbe_bool_t fbe_provision_drive_is_local_state_set(fbe_provision_drive_t *provision_drive_p,
                                                                  fbe_provision_drive_local_state_t local_state)
{
    return ((provision_drive_p->local_state & local_state) == local_state);
}
static __forceinline void fbe_provision_drive_init_local_state(fbe_provision_drive_t *provision_drive_p)
{
    provision_drive_p->local_state = 0;
    return;
}
static __forceinline void fbe_provision_drive_set_local_state(fbe_provision_drive_t *provision_drive_p,
                                                         fbe_provision_drive_local_state_t local_state)
{
    provision_drive_p->local_state |= local_state;
}

static __forceinline void fbe_provision_drive_clear_local_state(fbe_provision_drive_t *provision_drive_p,
                                                           fbe_provision_drive_local_state_t local_state)
{
    provision_drive_p->local_state &= ~local_state;
}
static __forceinline void fbe_provision_drive_get_local_state(fbe_provision_drive_t *provision_drive_p,
                                                         fbe_provision_drive_local_state_t *local_state)
{
     *local_state = provision_drive_p->local_state;
}


/*!****************************************************************************
 * fbe_provision_drive_get_download_start_time()
 ******************************************************************************
 * @brief 
 *  Get the download start time.
 *
 * @param provision_drive_p     - pointer to the provision drive object 
 *
 * @return fbe_time_t   - download start time       
 *
 ******************************************************************************/
static __forceinline fbe_time_t
fbe_provision_drive_get_download_start_time(fbe_provision_drive_t * provision_drive_p)
{
    return (provision_drive_p->download_start_time);
}

/*!****************************************************************************
 * fbe_provision_drive_set_download_start_time()
 ******************************************************************************
 * @brief 
 *  Set the download start time.
 *
 * @param provision_drive_p     - pointer to the provision drive object 
 * @param start_time            - start time 
 *
 * @return none      
 *
 ******************************************************************************/
static __forceinline void
fbe_provision_drive_set_download_start_time(fbe_provision_drive_t * provision_drive_p, fbe_time_t start_time)
{
    provision_drive_p->download_start_time = start_time;
}

/*!****************************************************************************
 * fbe_provision_drive_get_ds_disabled_start_time()
 ******************************************************************************
 * @brief 
 *  Get the ds_disabled start time.
 *
 * @param provision_drive_p     - pointer to the provision drive object 
 *
 * @return fbe_time_t   - start time       
 *
 ******************************************************************************/
static __forceinline fbe_time_t
fbe_provision_drive_get_ds_disabled_start_time(fbe_provision_drive_t * provision_drive_p)
{
    return (provision_drive_p->ds_disabled_start_time);
}

/*!****************************************************************************
 * fbe_provision_drive_set_ds_disabled_start_time()
 ******************************************************************************
 * @brief 
 *  Set the ds_disabled start time.
 *
 * @param provision_drive_p     - pointer to the provision drive object 
 * @param start_time            - start time 
 *
 * @return none      
 *
 ******************************************************************************/
static __forceinline void
fbe_provision_drive_set_ds_disabled_start_time(fbe_provision_drive_t * provision_drive_p, fbe_time_t start_time)
{
    provision_drive_p->ds_disabled_start_time = start_time;
}

/*!****************************************************************************
 * fbe_provision_drive_set_ds_health_not_optimal_start_time()
 ******************************************************************************
 * @brief 
 *  Set the downstream health not ready start time.
 *
 * @param provision_drive_p     - pointer to the provision drive object 
 * @param start_time            - start time 
 *
 * @return none      
 *
 ******************************************************************************/
static __forceinline void
fbe_provision_drive_set_ds_health_not_optimal_start_time(fbe_provision_drive_t * provision_drive_p, fbe_time_t start_time)
{
    provision_drive_p->ds_health_not_optimal_start_time = start_time;
}

/*!****************************************************************************
 * fbe_provision_drive_get_ds_health_not_optimal_start_time()
 ******************************************************************************
 * @brief 
 *  Get the downstream not ready start time.
 *
 * @param provision_drive_p     - pointer to the provision drive object 
 * @param start_time            - start time 
 *
 * @return none      
 *
 ******************************************************************************/
static __forceinline fbe_time_t
fbe_provision_drive_get_ds_health_not_optimal_start_time(fbe_provision_drive_t * provision_drive_p)
{
    return(provision_drive_p->ds_health_not_optimal_start_time);
}

/*!****************************************************************************
 * fbe_provision_drive_set_wear_leveling_query_time()
 ******************************************************************************
 * @brief 
 *  Set the wear leveling query time.
 *
 * @param provision_drive_p     - pointer to the provision drive object 
 * @param time            - new wearl leveling time 
 *
 * @return none      
 *
 ******************************************************************************/
static __forceinline void
fbe_provision_drive_set_wear_leveling_query_time(fbe_provision_drive_t * provision_drive_p, fbe_time_t time)
{
    provision_drive_p->last_wear_leveling_time = time;
}

/*!****************************************************************************
 * fbe_provision_drive_get_wear_leveling_query_time()
 ******************************************************************************
 * @brief 
 *  Get the last wear leveling query time.
 *
 * @param provision_drive_p     - pointer to the provision drive object 
 *
 * @return fbe_time_t     
 *
 ******************************************************************************/
static __forceinline fbe_time_t
fbe_provision_drive_get_wear_leveling_query_time(fbe_provision_drive_t * provision_drive_p)
{
    return(provision_drive_p->last_wear_leveling_time);
}

static __forceinline fbe_bool_t 
fbe_provision_drive_system_time_is_zero(fbe_system_time_t *time_stamp)
{
    if(time_stamp->year == 0 &&
       time_stamp->weekday == 0 && 
       time_stamp->day == 0 && 
       time_stamp->hour == 0 &&
       time_stamp->minute == 0 && 
       time_stamp->second == 0 &&
       time_stamp->milliseconds == 0) {
        return FBE_TRUE;
    } else {
        return FBE_FALSE;
    }
}

static  __forceinline void
fbe_provision_drive_set_bg_zeroing_log_status(fbe_provision_drive_t *provision_drive, fbe_bool_t bg_zeroing_log_status)
{
    provision_drive->bg_zeroing_log_status = bg_zeroing_log_status;
}

/*
 * FUNCTION PROTOTYPES:
 */

fbe_status_t fbe_provision_drive_get_imported_offset(fbe_provision_drive_t * provision_drive, fbe_lba_t *offset);

fbe_status_t fbe_provision_drive_set_config_type(fbe_provision_drive_t * provision_drive_, fbe_provision_drive_config_type_t config_type);
fbe_status_t fbe_provision_drive_get_config_type(fbe_provision_drive_t * provision_drive, fbe_provision_drive_config_type_t *config_type);
fbe_status_t fbe_provision_drive_get_configured_physical_block_size(fbe_provision_drive_t * provision_drive, 
        fbe_provision_drive_configured_physical_block_size_t *configured_physical_block_size);
fbe_status_t fbe_provision_drive_set_configured_physical_block_size(fbe_provision_drive_t * provision_drive, 
        fbe_provision_drive_configured_physical_block_size_t configured_physical_block_size);
fbe_status_t fbe_provision_drive_set_max_drive_transfer_limit(fbe_provision_drive_t * provision_drive_p, fbe_block_count_t max_drive_xfer_limit);
fbe_status_t fbe_provision_drive_get_max_drive_transfer_limit(fbe_provision_drive_t * provision_drive_p, fbe_block_count_t * max_drive_xfer_limit_p);
fbe_status_t fbe_provision_drive_set_download_originator(fbe_provision_drive_t *provision_drive_p, fbe_bool_t is_originator);
fbe_status_t  fbe_provision_drive_set_health_check_originator(fbe_provision_drive_t *provision_drive_p, fbe_bool_t is_originator);
fbe_status_t fbe_provision_drive_update_last_checkpoint_time(fbe_provision_drive_t * provision_drive_p);
fbe_status_t fbe_provision_drive_get_last_checkpoint_time(fbe_provision_drive_t * provision_drive_p, fbe_time_t *last_checkpoint_time);
fbe_status_t fbe_provision_drive_set_spin_down_qualified(fbe_provision_drive_t *provision_drive_p, fbe_bool_t is_qualified);
fbe_status_t fbe_provision_drive_set_unmap_supported(fbe_provision_drive_t *provision_drive_p, fbe_bool_t is_supported);
fbe_status_t fbe_provision_drive_set_user_capacity(fbe_provision_drive_t * provision_drive_p, fbe_lba_t user_capacity);
fbe_status_t fbe_provision_drive_get_user_capacity(fbe_provision_drive_t * provision_drive_p, fbe_lba_t *user_capacity);

/* fbe_provision_drive_main.c */
fbe_status_t fbe_provision_drive_destroy( fbe_object_handle_t object_handle);

fbe_status_t fbe_provision_drive_load(void);
fbe_status_t fbe_provision_drive_unload(void);
fbe_status_t fbe_provision_drive_create_object(fbe_packet_t * packet, fbe_object_handle_t * object_handle);
fbe_status_t fbe_provision_drive_destroy_object( fbe_object_handle_t object_handle);

fbe_status_t fbe_provision_drive_io_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet_p);
fbe_status_t fbe_provision_drive_monitor_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet_p);

fbe_status_t fbe_provision_drive_get_metadata_positions(fbe_provision_drive_t *  provision_drive,
                                                        fbe_provision_drive_metadata_positions_t * provision_drive_metadata_positions);

fbe_u32_t fbe_provision_drive_get_encryption_qual_substate(fbe_payload_block_operation_qualifier_t qualifier);
fbe_status_t fbe_provision_drive_get_physical_drive_location(fbe_provision_drive_t * provision_drive_p,
                                                             fbe_packet_t * packet_p);
fbe_status_t fbe_provision_drive_handle_shutdown(fbe_provision_drive_t * const provision_drive_p);
fbe_status_t fbe_provision_drive_set_clustered_flag(fbe_provision_drive_t * provision_drive_p, fbe_provision_drive_clustered_flags_t flags);
fbe_status_t fbe_provision_drive_clear_clustered_flag(fbe_provision_drive_t * provision_drive_p, fbe_provision_drive_clustered_flags_t flags);
fbe_status_t fbe_provision_drive_get_clustered_flags(fbe_provision_drive_t * provision_drive_p, fbe_provision_drive_clustered_flags_t * flags);
fbe_status_t fbe_provision_drive_get_peer_clustered_flags(fbe_provision_drive_t * provision_drive_p, fbe_provision_drive_clustered_flags_t * flags);
fbe_bool_t fbe_provision_drive_is_clustered_flag_set(fbe_provision_drive_t * provision_drive_p, fbe_provision_drive_clustered_flags_t flags);
fbe_bool_t fbe_provision_drive_is_peer_clustered_flag_set(fbe_provision_drive_t * provision_drive_p, fbe_provision_drive_clustered_flags_t flags);
fbe_status_t  fbe_provision_drive_init_event_outstanding_flag(fbe_provision_drive_t  *provision_drive_p, fbe_bool_t  is_event_outstanding);
fbe_status_t  fbe_provision_drive_set_event_outstanding_flag(fbe_provision_drive_t  *provision_drive_p);
fbe_status_t  fbe_provision_drive_get_event_outstanding_flag(fbe_provision_drive_t  *provision_drive_p, fbe_bool_t  *is_event_outstanding);
fbe_status_t  fbe_provision_drive_clear_event_outstanding_flag(fbe_provision_drive_t  *provision_drive_p);
fbe_bool_t fbe_provision_drive_is_peer_flag_set(fbe_provision_drive_t * provision_drive_p, 
                                                fbe_provision_drive_clustered_flags_t peer_flag);
fbe_bool_t fbe_provision_drive_check_drive_location(fbe_provision_drive_t * provision_drive_p);
fbe_status_t fbe_provision_drive_set_drive_location(fbe_provision_drive_t * provision_drive_p, fbe_u32_t port_num, fbe_u32_t encl_num, fbe_u32_t slot_num);
fbe_status_t fbe_provision_drive_get_drive_location(fbe_provision_drive_t * provision_drive_p, fbe_u32_t *port_num, fbe_u32_t *encl_num, fbe_u32_t *slot_num);
fbe_status_t fbe_provision_drive_get_peer_drive_location(fbe_provision_drive_t * provision_drive_p, fbe_u32_t *port_num, fbe_u32_t *encl_num, fbe_u32_t *slot_num);
fbe_bool_t fbe_provision_drive_process_state_change_for_faulted_drives (fbe_provision_drive_t *provision_drive_p, 
                                                                        fbe_path_state_t path_state,
                                                                        fbe_path_attr_t path_attr,
                                                                        fbe_base_config_downstream_health_state_t downstream_health_state);

/*PVD garbage collection */
fbe_status_t fbe_provision_drive_should_permanently_removed(fbe_provision_drive_t * provision_drive_p, fbe_bool_t * should_removed);
fbe_bool_t fbe_provision_drive_wait_for_downstream_health_optimal_timeout(fbe_provision_drive_t *provision_drive_p);

/* fbe_provision_drive_monitor.c */
fbe_status_t fbe_provision_drive_monitor(fbe_provision_drive_t * const provision_drive_p, fbe_packet_t * packet);
fbe_status_t fbe_provision_drive_monitor_load_verify(void);

/* fbe_provision_drive_usurper.c */
fbe_status_t fbe_provision_drive_control_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet);
fbe_status_t fbe_provision_drive_set_configuration(fbe_provision_drive_t *provision_drive_p, fbe_packet_t * packet_p);
fbe_status_t fbe_provision_drive_update_configuration(fbe_provision_drive_t *provision_drive_p, fbe_packet_t * packet_p);
fbe_status_t fbe_provision_drive_update_block_size(fbe_provision_drive_t *provision_drive_p, fbe_packet_t * packet_p);

fbe_status_t fbe_provision_drive_get_control_buffer(fbe_provision_drive_t * in_provision_drive_p,
                                                    fbe_packet_t * in_packet_p,
                                                    fbe_payload_control_buffer_length_t in_buffer_length,
                                                    fbe_payload_control_buffer_t * out_buffer_pp);
fbe_status_t fbe_provision_drive_unregister_keys(fbe_provision_drive_t * provision_drive, fbe_packet_t * packet);
fbe_status_t fbe_provision_drive_port_unregister_keys(fbe_provision_drive_t * provision_drive_p, 
                                                      fbe_packet_t * packet_p,
                                                      fbe_edge_index_t server_index);
fbe_status_t fbe_provision_drive_initiate_consumed_area_zeroing(fbe_provision_drive_t*   provision_drive, fbe_packet_t* packet_p);
fbe_status_t fbe_provision_drive_set_scrub_complete_bits(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);
fbe_status_t fbe_provision_drive_usurper_get_ssd_statitics(fbe_provision_drive_t *provision_drive_p, 
                                                                  fbe_packet_t *packet_p);

/* fbe_provision_drive_executor.c */
fbe_status_t fbe_provision_drive_block_transport_entry(fbe_transport_entry_context_t context, fbe_packet_t * packet);

fbe_raid_state_status_t fbe_provision_drive_block_transport_restart_io(fbe_raid_iots_t * iots_p);

fbe_status_t fbe_provision_drive_send_packet_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
fbe_status_t fbe_provision_drive_send_monitor_packet
(
    fbe_provision_drive_t*               in_provision_drive_p,
    fbe_packet_t*                        in_packet_p,
    fbe_payload_block_operation_opcode_t in_opcode,
    fbe_lba_t                            in_start_lba,
    fbe_block_count_t                    in_block_count
);
fbe_status_t fbe_provision_drive_send_to_cmi_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);

/* fbe_provision_drive_event.c */
fbe_status_t fbe_provision_drive_event_entry(fbe_object_handle_t object_handle, 
                                             fbe_event_type_t event_type,
                                             fbe_event_context_t event_context);

fbe_base_config_downstream_health_state_t fbe_provision_drive_verify_downstream_health(fbe_provision_drive_t * provision_drive_p);

fbe_status_t fbe_provision_drive_set_condition_based_on_downstream_health (fbe_provision_drive_t * provision_drive_p, 
                                                                           fbe_path_attr_t path_attr,
                                                                           fbe_base_config_downstream_health_state_t downstream_health_state);
fbe_status_t fbe_provision_drive_send_checkpoint_notification(fbe_provision_drive_t * provision_drive_p);

/* fbe_provision_drive_metadata.c */
fbe_status_t fbe_provision_drive_metadata_get_background_zero_checkpoint(fbe_provision_drive_t * provision_drive,
                                                                fbe_lba_t * background_zero_checkpoint);
fbe_status_t fbe_provision_drive_metadata_set_background_zero_checkpoint(fbe_provision_drive_t * provision_drive, 
                                                                fbe_packet_t * packet,
                                                                fbe_lba_t background_zero_checkpoint);
fbe_status_t fbe_provision_drive_metadata_incr_background_zero_checkpoint(fbe_provision_drive_t * provision_drive_p, 
                                                            fbe_packet_t * packet_p,
                                                            fbe_lba_t zero_checkpoint,
                                                            fbe_block_count_t  block_count);
fbe_status_t fbe_provision_drive_metadata_update_background_zero_checkpoint(fbe_provision_drive_t * provision_drive_p, 
                                                            fbe_packet_t * packet_p,
                                                            fbe_lba_t zero_checkpoint);
fbe_status_t fbe_provision_drive_metadata_set_bz_chkpt_without_persist(fbe_provision_drive_t * provision_drive_p, 
                                                            fbe_packet_t * packet_p,
                                                            fbe_lba_t zero_checkpoint);

fbe_status_t fbe_provision_drive_metadata_get_zero_on_demand_flag(fbe_provision_drive_t * provision_drive_p,
                                                                  fbe_bool_t * zero_on_demand_p);
fbe_status_t fbe_provision_drive_metadata_set_zero_on_demand_flag(fbe_provision_drive_t * provision_drive_p, 
                                                                  fbe_packet_t * packet_p,
                                                                  fbe_bool_t zero_on_demand_b);
fbe_status_t fbe_provision_drive_metadata_get_verify_invalidate_checkpoint(fbe_provision_drive_t * provision_drive,
                                                                         fbe_lba_t * metadata_verify_checkpoint);
fbe_status_t fbe_provision_drive_metadata_set_verify_invalidate_checkpoint(fbe_provision_drive_t * provision_drive, 
                                                                         fbe_packet_t * packet,
                                                                         fbe_lba_t metadata_verify_checkpoint);
fbe_status_t 
fbe_provision_drive_metadata_incr_verify_invalidate_checkpoint(fbe_provision_drive_t * provision_drive_p, 
                                                                 fbe_packet_t * packet_p,
                                                                 fbe_lba_t invalidate_checkpoint,
                                                                 fbe_block_count_t  block_count);
fbe_status_t 
fbe_provision_drive_metadata_set_verify_invalidate_chkpt_without_persist(fbe_provision_drive_t * provision_drive_p, 
                                                                         fbe_packet_t * packet_p,
                                                                         fbe_lba_t verify_invalidate_checkpoint);

fbe_status_t fbe_provision_drive_metadata_get_end_of_life_state(fbe_provision_drive_t * provision_drive,
                                                                fbe_bool_t * end_of_life_state);

fbe_status_t fbe_provision_drive_metadata_set_end_of_life_state(fbe_provision_drive_t * provision_drive_p,
                                                                fbe_packet_t * packet_p,
                                                                fbe_bool_t end_of_life_state);

fbe_status_t fbe_provision_drive_nonpaged_metadata_init(fbe_provision_drive_t * provision_drive_p,
                                                        fbe_packet_t *packet_p);

fbe_status_t fbe_provision_drive_metadata_initialize_metadata_element(fbe_provision_drive_t * provision_drive_p,
                                                                      fbe_packet_t * packet_p);

fbe_status_t fbe_provision_drive_metadata_write_default_nonpaged_metadata(fbe_provision_drive_t * provision_drive_p,
                                                                          fbe_packet_t *packet_p);

fbe_status_t fbe_provision_drive_metadata_write_default_paged_metadata(fbe_provision_drive_t * provision_drive_p,
                                                                       fbe_packet_t *packet_p);
fbe_status_t fbe_provision_drive_write_default_system_paged(fbe_provision_drive_t * provision_drive_p,
                                                            fbe_packet_t *packet_p);
fbe_status_t fbe_provision_drive_metadata_write_default_paged_for_ext_pool(fbe_provision_drive_t * provision_drive_p,
                                                                           fbe_packet_t *packet_p);
fbe_status_t fbe_provision_drive_metadata_write_default_paged_for_ext_pool_clear_np_flag(fbe_packet_t * packet_p,
                                                                                         fbe_packet_completion_context_t context);
fbe_status_t fbe_provision_drive_metadata_mark_consumed(fbe_provision_drive_t * provision_drive_p, fbe_packet_t * packet_p);

fbe_status_t fbe_provision_drive_metadata_get_paged_metadata_capacity(fbe_provision_drive_t * provision_drive_p,
                                                                      fbe_lba_t * paged_metadata_capacity_p);

fbe_status_t fbe_provision_drive_metadata_set_sniff_verify_checkpoint(fbe_provision_drive_t * provision_drive_p,
                                                                      fbe_packet_t * packet_p,
                                                                      fbe_lba_t sniff_verify_checkpoint);

fbe_status_t fbe_provision_drive_metadata_get_sniff_verify_checkpoint(fbe_provision_drive_t * provision_drive,
                                                                      fbe_lba_t * sniff_verify_checkpoint);

fbe_status_t fbe_provision_drive_metadata_get_nonpaged_metadata_drive_info(fbe_provision_drive_t * provision_drive_p,
                                                                           fbe_provision_drive_nonpaged_metadata_drive_info_t * drive_location_p);

fbe_status_t fbe_provision_drive_sniff_verify_checkpoint_update_complete(fbe_packet_t * packet_p, 
                                                                         fbe_packet_completion_context_t context);

fbe_status_t fbe_provision_drive_metadata_update_physical_drive_info(fbe_provision_drive_t * provision_drive_p, 
                                                                     fbe_packet_t * packet_p,
                                                                     fbe_physical_drive_mgmt_get_drive_information_t * physical_drive_info);

fbe_status_t fbe_provision_drive_metadata_get_physical_drive_info(fbe_provision_drive_t * provision_drive_p,
                                                                  fbe_packet_t * packet_p);

/* for set/get pvd nonpaged metadata--time_stamp */
fbe_status_t fbe_provision_drive_metadata_set_time_stamp(fbe_provision_drive_t * provision_drive_p,
                                                                      fbe_packet_t * packet_p,
                                                                      fbe_bool_t is_clear);
fbe_status_t fbe_provision_drive_metadata_ex_set_time_stamp(fbe_provision_drive_t * provision_drive_p,
																		  fbe_packet_t * packet_p,
																		  fbe_system_time_t time_stamp,
																		  fbe_bool_t is_persist);

fbe_status_t 
fbe_provision_drive_metadata_write_paged_metadata_chunk(fbe_provision_drive_t * provision_drive_p,
                                                        fbe_packet_t *packet_p,
                                                        fbe_provision_drive_paged_metadata_t *paged_set_bits,
                                                        fbe_chunk_index_t chunk_index,
                                                        fbe_bool_t verify_required);

fbe_status_t fbe_provision_drive_metadata_get_md_chunk_index_for_lba(fbe_provision_drive_t * in_provision_drive_p,
                                                                     fbe_lba_t in_media_error_lba,
                                                                     fbe_chunk_index_t *chunk_index);

fbe_status_t fbe_provision_drive_metadata_get_time_stamp(fbe_provision_drive_t * provision_drive,
                                                                      fbe_system_time_t *time_stamp);
fbe_status_t fbe_provision_drive_metadata_is_paged_metadata_valid(fbe_lba_t host_start_lba,
                                                                  fbe_block_count_t host_block_count,
                                                                  fbe_provision_drive_paged_metadata_t * paged_data_bits_p, 
                                                                  fbe_bool_t * is_paged_data_valid);
fbe_bool_t
fbe_provision_drive_metadata_verify_invalidate_is_need_to_run(fbe_provision_drive_t * provision_drive_p);


fbe_status_t fbe_provision_drive_set_swap_pending(fbe_packet_t *packet_p,
                                                        fbe_packet_completion_context_t context);
fbe_status_t fbe_provision_drive_clear_swap_pending(fbe_packet_t *packet_p,
                                                          fbe_packet_completion_context_t context);

fbe_status_t fbe_provision_drive_set_eas_start(fbe_packet_t *packet_p, fbe_packet_completion_context_t context);
fbe_status_t fbe_provision_drive_set_eas_complete(fbe_packet_t *packet_p, fbe_packet_completion_context_t context);

fbe_status_t fbe_provision_drive_metadata_set_sniff_media_error_lba(fbe_provision_drive_t * provision_drive_p, fbe_packet_t * packet_p, 
                                                                    fbe_lba_t sniff_verify_checkpoint);
fbe_status_t fbe_provision_drive_metadata_get_sniff_media_error_lba(fbe_provision_drive_t * provision_drive, 
                                                                    fbe_lba_t * sniff_verify_checkpoint);

fbe_status_t fbe_provision_drive_metadata_get_sniff_pass_count(fbe_provision_drive_t * provision_drive, fbe_u32_t * pass_count);
fbe_status_t fbe_provision_drive_metadata_set_sniff_pass_count(fbe_provision_drive_t * provision_drive, fbe_packet_t * packet,
                                                               fbe_u32_t pass_count);
fbe_status_t fbe_provision_drive_metadata_clear_sniff_pass_count(fbe_packet_t * packet, fbe_packet_completion_context_t context);

fbe_status_t fbe_provision_drive_metadata_zero_nonpaged_metadata(fbe_provision_drive_t * provision_drive_p,
                                                                          fbe_packet_t *packet_p);

fbe_status_t fbe_provision_drive_metadata_set_default_nonpaged_metadata(fbe_provision_drive_t * provision_drive_p,
                                                                                fbe_packet_t *packet_p);
fbe_status_t fbe_provision_drive_metadata_get_drive_fault_state(fbe_provision_drive_t * provision_drive,
                                                                fbe_bool_t * drive_fault_state);
fbe_status_t fbe_provision_drive_metadata_set_drive_fault_state(fbe_provision_drive_t * provision_drive_p,
                                                                fbe_packet_t * packet_p,
                                                                fbe_bool_t drive_fault_state);
fbe_status_t fbe_provision_drive_mark_paged_write_start(fbe_packet_t * packet_p,
                                           fbe_packet_completion_context_t context);
fbe_status_t fbe_provision_drive_metadata_set_np_flag_need_zero_paged(fbe_packet_t * packet_p,
                                                                      fbe_packet_completion_context_t context);
fbe_status_t fbe_provision_drive_metadata_clear_np_flag_need_zero_paged(fbe_packet_t * packet_p,
                                                                        fbe_packet_completion_context_t context);
fbe_status_t fbe_provision_drive_metadata_get_np_flag(fbe_provision_drive_t * provision_drive_p,
                                                      fbe_provision_drive_np_flags_t * flag) ;
fbe_status_t fbe_provision_drive_verify_paged(fbe_provision_drive_t * provision_drive_p,
                                              fbe_packet_t *packet_p);
fbe_status_t fbe_provision_drive_metadata_write_verify_paged_metadata_callback(fbe_packet_t * packet, 
                                                                               fbe_metadata_callback_context_t context);
fbe_status_t fbe_provision_drive_metadata_paged_init_client_blob(fbe_packet_t * packet, 
                                                                 fbe_payload_metadata_operation_t * metadata_operation_p);
fbe_status_t fbe_provision_drive_metadata_paged_callback_read(fbe_packet_t * packet, fbe_metadata_callback_context_t context);
fbe_status_t fbe_provision_drive_metadata_paged_callback_set_bits(fbe_packet_t * packet, fbe_provision_drive_paged_metadata_t * set_bits);
fbe_status_t fbe_provision_drive_metadata_paged_callback_clear_bits(fbe_packet_t * packet, fbe_provision_drive_paged_metadata_t * clear_bits);
fbe_status_t fbe_provision_drive_metadata_paged_callback_write(fbe_packet_t * packet, fbe_provision_drive_paged_metadata_t * set_bits);


/* fbe_provision_drive_bitmap.c */
fbe_status_t fbe_provision_drive_bitmap_get_lba_for_the_chunk_index(fbe_provision_drive_t * provision_drive_p,
                                                                    fbe_chunk_index_t chunk_index,
                                                                    fbe_u32_t chunk_size,
                                                                    fbe_lba_t * checkpoint_lba_p);

fbe_status_t fbe_provision_drive_bitmap_get_chunk_index_for_lba(fbe_provision_drive_t * provision_drive_p,
                                                                fbe_lba_t lba, 
                                                                fbe_u32_t chunk_size,
                                                                fbe_chunk_index_t * chunk_index_p);

fbe_status_t fbe_provision_drive_bitmap_get_total_number_of_chunks(fbe_provision_drive_t * provision_drive_p,
                                                                   fbe_u32_t chunk_size,
                                                                   fbe_u32_t * total_number_of_chunks_p);

fbe_status_t fbe_provision_drive_bitmap_get_number_of_chunks_marked_for_zero(fbe_provision_drive_paged_metadata_t * paged_data_bits_p,
                                                                             fbe_chunk_count_t chunk_count,
                                                                             fbe_chunk_count_t * number_of_chunks_marked_zero_p);

fbe_status_t fbe_provision_drive_bitmap_is_chunk_marked_for_zero(fbe_provision_drive_paged_metadata_t * paged_data_bits_p,
                                                                 fbe_bool_t * is_chunk_marked_for_zero_p);

fbe_status_t fbe_provision_drive_bitmap_get_next_need_zero_chunk(fbe_provision_drive_paged_metadata_t * paged_data_bits_p,
                                                                 fbe_chunk_index_t current_chunk_index,
                                                                 fbe_chunk_count_t chunk_count,
                                                                 fbe_chunk_index_t * next_need_zero_index_in_paged_bits_p);

fbe_status_t fbe_provision_drive_bitmap_get_next_zeroed_chunk(fbe_provision_drive_paged_metadata_t * paged_data_bits_p,
                                                              fbe_chunk_index_t current_chunk_index,
                                                              fbe_chunk_count_t chunk_count,
                                                              fbe_chunk_index_t * next_zeroed_chunk_index_in_paged_bits_p);
fbe_status_t
fbe_provision_drive_bitmap_get_number_of_invalid_chunks(fbe_provision_drive_paged_metadata_t * paged_data_bits_p,
                                                        fbe_chunk_count_t chunk_count,
                                                        fbe_chunk_count_t * total_number_of_chunks);
fbe_status_t
fbe_provision_drive_bitmap_is_paged_data_invalid(fbe_provision_drive_paged_metadata_t * paged_data_bits_p,
                                                 fbe_bool_t * is_paged_data_invalid);
fbe_status_t
fbe_provision_drive_bitmap_get_next_invalid_chunk(fbe_provision_drive_paged_metadata_t * paged_data_bits_p,
                                                 fbe_chunk_index_t current_chunk_index,
                                                 fbe_chunk_count_t chunk_count,
                                                 fbe_chunk_index_t * next_invalid_chunk_index_in_paged_bits_p);
fbe_status_t fbe_provision_drive_set_key_handle(fbe_provision_drive_t * provision_drive, fbe_packet_t * packet);

/* fbe_provision_drive_class.c */
fbe_status_t fbe_provision_drive_class_control_entry(fbe_packet_t * packet);
fbe_status_t  fbe_class_provision_drive_get_metadata_positions(fbe_lba_t imported_capacity, 
                                                               fbe_provision_drive_metadata_positions_t * provision_drive_metadata_positions);
fbe_status_t  fbe_class_provision_drive_get_metadata_positions_from_exported_capacity(fbe_lba_t exported_capacity, 
                                                               fbe_provision_drive_metadata_positions_t * provision_drive_metadata_positions);
fbe_status_t fbe_provision_drive_class_get_drive_transfer_limits(fbe_provision_drive_t *provision_drive_p,
                                                                 fbe_block_count_t *max_blocks_per_request_p);
void fbe_provision_drive_class_get_debug_flag(fbe_provision_drive_debug_flags_t *debug_flags_p);
void fbe_provision_drive_class_set_class_debug_flag(fbe_provision_drive_debug_flags_t new_pvd_debug_flags);
fbe_bool_t fbe_provision_drive_class_is_debug_flag_set(fbe_provision_drive_debug_flags_t debug_flags);
fbe_status_t fbe_provision_drive_class_set_resource_priority(fbe_packet_t * packet_p);
void fbe_provision_drive_class_load_registry_keys(void);
void fbe_provision_drive_class_set_wear_leveling_timer(fbe_u64_t wear_level_timer);
fbe_u64_t fbe_provision_drive_class_get_wear_leveling_timer(void);
fbe_u64_t fbe_provision_drive_class_get_warranty_period(void);


/* fbe_provision_drive_remap.c */

fbe_status_t
fbe_provision_drive_ask_remap_action( fbe_provision_drive_t* in_provision_drive_p,
                                      fbe_packet_t*          in_packet_p,
                                      fbe_lba_t lba,
                                      fbe_block_count_t block_count,
                                      fbe_event_completion_function_t event_completion
                                    );

fbe_status_t fbe_provision_drive_start_next_remap_io(fbe_provision_drive_t* in_provision_drive_p, fbe_packet_t*  in_packet_p);
// ask upstream object for remap action completion function
fbe_status_t
fbe_provision_drive_ask_remap_action_completion
(
    fbe_event_t*                                in_event_p,
    fbe_event_completion_context_t              in_context
);


/* fbe_provision_drive_verify.c */
fbe_provision_drive_io_status_t fbe_provision_drive_classify_io_status(fbe_status_t in_transport_status,
    fbe_payload_block_operation_status_t        in_block_staus,
                                                                       fbe_payload_block_operation_qualifier_t in_block_qualifier);

void fbe_provision_drive_update_verify_report(fbe_provision_drive_t * in_provision_drive_p);

fbe_status_t
fbe_provision_drive_ask_verify_permission( fbe_base_object_t* in_object_p,
                                           fbe_packet_t*      in_packet_p  );   
//sends a single sniff verify i/o request to the provision drive's executor
fbe_status_t
fbe_provision_drive_start_next_verify_io(fbe_provision_drive_t* in_provision_drive_p,
                                         fbe_packet_t*          in_packet_p);
fbe_status_t
fbe_provision_drive_verify_invalidate_update_paged_metadata(fbe_provision_drive_t * provision_drive_p,
                                                            fbe_packet_t * packet_p);

fbe_status_t fbe_provision_drive_metadata_verify_checkpoint_lock_write_error_completion(fbe_packet_t * packet_p,
                                                                                       fbe_packet_completion_context_t context);

fbe_status_t fbe_provision_drive_verify_update_sniff_checkpoint(fbe_provision_drive_t*  provision_drive_p,
                                                                fbe_packet_t*  packet_p,
                                                                fbe_lba_t     sniff_checkpoint,
                                                                fbe_lba_t     next_consumed_lba,
																fbe_block_count_t   blocks);

fbe_status_t
fbe_provision_drive_verify_update_invalidate_checkpoint(fbe_provision_drive_t*  provision_drive_p,
                                                   fbe_packet_t*  packet_p,
                                                 fbe_lba_t     invalidate_checkpoint,
                                                 fbe_lba_t     next_consumed_lba,
                                                 fbe_block_count_t   blocks);
fbe_status_t
fbe_provision_drive_verify_invalidate_remap_completion(
                                                 fbe_event_t*                   in_event_p,
                                                 fbe_event_completion_context_t in_context
                                               );


/* fbe_provision_drive_zero_utils.c */
fbe_status_t fbe_provision_drive_zero_utils_can_zeroing_start(fbe_provision_drive_t * provision_drive_p,
                                                              fbe_bool_t * can_zeroing_start_p);

fbe_status_t fbe_provision_drive_zero_utils_check_permission_based_on_current_load(fbe_provision_drive_t * provision_drive_p,
                                                                                   fbe_bool_t * is_ok_to_perform_zero_b_p);

fbe_status_t fbe_provision_drive_zero_utils_ask_zero_permision(fbe_provision_drive_t * provision_drive_p,
                                                               fbe_lba_t lba,
                                                               fbe_block_count_t block_count,
                                                               fbe_packet_t * packet_p);

fbe_status_t fbe_provision_drive_zero_utils_determine_if_zeroing_complete(fbe_provision_drive_t * provision_drive_p,
                                                                          fbe_lba_t zero_checkpoint,
                                                                          fbe_bool_t * is_zeroing_complete_p);

fbe_status_t fbe_provision_drive_zero_utils_issue_write_same_request(fbe_provision_drive_t * provision_drive_p, 
                                                                     fbe_packet_t * packet_p,
                                                                     fbe_lba_t start_lba);

fbe_status_t fbe_provision_drive_zero_utils_compute_crc(void * block_p, fbe_u32_t number_of_blocks);


/* fbe_provision_drive_stripe_lock.c */
fbe_status_t 
fbe_provision_drive_is_stripe_lock_needed(fbe_provision_drive_t * provision_drive,
                                          fbe_payload_block_operation_t * block_operation, 
                                          fbe_bool_t * is_stripe_lock_needed);

fbe_status_t fbe_provision_drive_determine_stripe_lock_range(fbe_provision_drive_t * provision_drive_p,
                                                             fbe_lba_t start_lba,
                                                             fbe_block_count_t block_count,
                                                             fbe_chunk_size_t  chunk_size,
                                                             fbe_u64_t * stripe_number_p, 
															 fbe_u64_t * stripe_count_p);

fbe_status_t fbe_provision_drive_acquire_stripe_lock(fbe_packet_t * packet, fbe_packet_completion_context_t context);

fbe_status_t fbe_provision_drive_release_stripe_lock(fbe_provision_drive_t * provision_drive_p,
                                                     fbe_packet_t * packet_p);

fbe_status_t fbe_provision_drive_get_NP_lock(fbe_provision_drive_t*  provision_drive_p,
                                             fbe_packet_t*    packet_p,
                                             fbe_packet_completion_function_t  completion_function);

fbe_status_t fbe_provision_drive_release_NP_lock(fbe_packet_t*    packet_p,
                                                fbe_packet_completion_context_t context);

fbe_status_t fbe_provision_drive_acquire_stripe_lock_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);

/* fbe_provision_drive_utils.c */
fbe_status_t fbe_provision_drive_utils_initialize_iots(fbe_provision_drive_t * provision_drive_p, fbe_packet_t *packet_p);

fbe_status_t fbe_provision_drive_utils_quiesce_io_if_needed(fbe_provision_drive_t * provision_drive_p,
                                                            fbe_bool_t * is_already_quiesce_b_p);

fbe_status_t fbe_provision_drive_utils_complete_packet(fbe_packet_t * packet_p, fbe_status_t status, fbe_u32_t status_qualifier);

fbe_status_t fbe_provision_drive_utils_block_transport_subpacket_completion(fbe_packet_t * packet_p,
                                                                            fbe_packet_completion_context_t context);

fbe_status_t fbe_provision_drive_release_block_op_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

fbe_status_t
fbe_provision_drive_utils_process_block_status(fbe_provision_drive_t * provision_drive_p,
			                                    fbe_packet_t * master_packet_p,
												fbe_packet_t * packet);

fbe_status_t fbe_provision_drive_utils_get_packet_and_sg_list(fbe_provision_drive_t * provision_drive_p,
                                                              fbe_memory_request_t * memory_request_p,
                                                              fbe_packet_t ** new_packet_p,
                                                              fbe_u32_t number_of_subpackets,
                                                              fbe_sg_element_t ** sg_list_p,
                                                              fbe_u32_t sg_element_count);

fbe_status_t fbe_provision_drive_utils_calculate_chunk_range(fbe_lba_t start_lba,
                                                             fbe_block_count_t block_count,
                                                             fbe_u32_t chunk_size,
                                                             fbe_chunk_index_t * start_chunk_index_p,
                                                             fbe_chunk_count_t * chunk_count_p);

fbe_status_t fbe_provision_drive_utils_calculate_chunk_range_without_edges(fbe_lba_t start_lba,
                                                                           fbe_block_count_t block_count,
                                                                           fbe_u32_t chunk_size,
                                                                           fbe_chunk_index_t * start_chunk_index_p,
                                                                           fbe_chunk_count_t * chunk_count_p);

fbe_status_t fbe_provision_drive_utils_calculate_number_of_unaligned_edges(fbe_lba_t start_lba,
                                                                           fbe_block_count_t block_count,
                                                                           fbe_u32_t chunk_size,
                                                                           fbe_u32_t * number_of_edges_p);

fbe_status_t fbe_provision_drive_utils_calculate_unaligned_edges_lba_range(fbe_lba_t start_lba,
                                                                           fbe_block_count_t block_count,
                                                                           fbe_chunk_size_t chunk_size,
                                                                           fbe_lba_t * pre_edge_start_lba_p,
                                                                           fbe_block_count_t * pre_edge_blk_count_p,
                                                                           fbe_lba_t * post_edge_start_lba_p,
                                                                           fbe_block_count_t * post_edge_blk_count_p,
                                                                           fbe_u32_t * number_of_edges_p);

fbe_status_t fbe_provision_drive_utils_calc_number_of_edge_chunks_needs_process(fbe_lba_t start_lba,
                                                                                  fbe_block_count_t block_count,
                                                                                  fbe_u32_t chunk_size,
                                                                                  fbe_provision_drive_paged_metadata_t * paged_data_bits_p,
                                                                                  fbe_provision_drive_edge_handling_check_func_t edge_handling_check_func,
                                                                                  fbe_chunk_count_t * number_of_edge_chunks_need_process_p);

fbe_status_t fbe_provision_drive_utils_calculate_edge_chunk_lba_range_which_need_process(fbe_lba_t start_lba,
                                                                                      fbe_block_count_t block_count,
                                                                                      fbe_chunk_size_t chunk_size,
                                                                                      fbe_provision_drive_paged_metadata_t * paged_data_bits_p,
                                                                                      fbe_lba_t * pre_edge_chunk_start_lba_p,
                                                                                      fbe_block_count_t * pre_edge_chunk_block_count_p,
                                                                                      fbe_lba_t * post_edge_chunk_start_lba_p,
                                                                                      fbe_block_count_t * post_edge_chunk_block_count_p,
                                                                                      fbe_chunk_count_t * number_of_edge_chunks_need_zero_p);

fbe_status_t fbe_provision_drive_utils_is_io_request_aligned(fbe_lba_t start_lba,
                                                             fbe_block_count_t block_count,
                                                             fbe_optimum_block_size_t optimum_block_size,
                                                             fbe_bool_t * is_request_aligned_p);

void fbe_provision_drive_utils_trace(fbe_provision_drive_t * provision_drive_p, 
                                                              fbe_trace_level_t trace_level,
                                                              fbe_u32_t message_id,
                                                              fbe_provision_drive_debug_flags_t debug_flag,
                                                              const fbe_char_t * fmt,  ...) __attribute__ ((format(__printf_func__, 5, 6)));
fbe_status_t fbe_provision_drive_utils_get_num_of_exported_chunks(fbe_provision_drive_t*   provision_drive_p,
                                                                  fbe_chunk_count_t      * out_num_of_chunks);          
fbe_status_t
fbe_provision_drive_utils_can_verify_invalidate_start(fbe_provision_drive_t * provision_drive_p,
                                                      fbe_bool_t * can_verify_invalidate_start_p);
fbe_status_t 
fbe_provision_drive_utils_ask_verify_invalidate_permission(fbe_provision_drive_t * provision_drive_p,
                                                          fbe_lba_t lba,
                                                          fbe_block_count_t block_count,
                                                          fbe_packet_t * packet_p);

fbe_status_t fbe_provision_drive_check_hook(fbe_provision_drive_t *provision_drive_p,
                                            fbe_u32_t state,
                                            fbe_u32_t substate,
                                            fbe_u64_t val2,
                                            fbe_scheduler_hook_status_t *status);
fbe_status_t
fbe_provision_drive_utils_verify_invalidate_check_permission_based_on_current_load(fbe_provision_drive_t * provision_drive_p,
                                                                                   fbe_bool_t * is_ok_to_perform);
fbe_status_t fbe_provision_drive_utils_is_sys_pvd(fbe_provision_drive_t*   provision_drive_p,
						                    fbe_bool_t*         sys_pvd);
/* fbe_provision_drive_user_zero.c */

/* it handles incoming user zero request to block transport server. */
fbe_status_t fbe_provision_drive_user_zero_handle_request(fbe_provision_drive_t * provision_drive_p, fbe_packet_t * packet_p);
fbe_status_t fbe_provision_drive_consumed_area_zero_handle_request(fbe_provision_drive_t * provision_drive_p, fbe_packet_t * packet_p);

/* fbe_provision_drive_background_zero.c */
fbe_status_t
fbe_provision_drive_background_zero_issue_write_same(fbe_provision_drive_t * provision_drive_p,
                                                     fbe_packet_t * packet_p);
fbe_status_t
fbe_provision_drive_background_zero_issue_write_same_completion(fbe_packet_t * packet_p,
                                                                fbe_packet_completion_context_t packet_completion_context);


/* it handles incoming background zero request to the block transport server. */
fbe_status_t fbe_provision_drive_background_zero_process_background_zero_request(fbe_provision_drive_t * provision_drive_p, 
                                                                                 fbe_packet_t * packet_p);

/* fbe_provision_drive_zero_on_demand.c */

/* it handles the zero on demand for the incoming i/o request if required. */
fbe_status_t fbe_provision_drive_zero_on_demand_process_io_request(fbe_provision_drive_t * provision_drive_p, 
                                                                   fbe_packet_t * packet_p);

/* fbe_provision_drive_event_log.c */

/* it logs event messages for disk zero start/complete */
fbe_status_t fbe_provision_drive_event_log_disk_zero_start_or_complete(fbe_provision_drive_t * provision_drive_p,
                                                                         fbe_bool_t is_disk_zero_start,
                                                                         fbe_packet_t* packet_p);
fbe_status_t fbe_provision_drive_write_event_log(fbe_provision_drive_t *   provision_drive_p,
                                                 fbe_u32_t event_code);


/* fbe_provision_drive_sniff_utils.c */

/* Advance verify checkpoint to next chunk, handle wraparounds, and update verify report*/
fbe_status_t fbe_provision_drive_advance_verify_checkpoint(fbe_provision_drive_t* in_provision_drive_p,
                                                           fbe_packet_t*   in_packet_p);

/* Update verify report error counters after a  media error was discovered */
void fbe_provision_drive_update_verify_report_error_counts(fbe_provision_drive_t*          in_provision_drive_p,
                                                           fbe_provision_drive_io_status_t in_io_status);

/*It calls fbe_base_config_get_operation_permission to get permsission to issue verify/remap i/o  */
fbe_status_t fbe_provision_drive_get_verify_permission_by_priority(
              fbe_provision_drive_t*      provision_drive_p,			
              fbe_bool_t*                         verify_permission);

fbe_bool_t
fbe_provision_drive_background_op_is_zeroing_need_to_run(fbe_provision_drive_t * provision_drive_p);

fbe_bool_t
fbe_provision_drive_background_op_is_sniff_need_to_run(fbe_provision_drive_t * provision_drive_p);

/* fbe_provision_drive_join.c
 */
fbe_lifecycle_status_t fbe_provision_drive_join_request(fbe_provision_drive_t *  provision_drive_p, fbe_packet_t * packet_p);
fbe_lifecycle_status_t fbe_provision_drive_join_done(fbe_provision_drive_t *  provision_drive_p, fbe_packet_t * packet_p);
fbe_lifecycle_status_t fbe_provision_drive_join_passive(fbe_provision_drive_t *  provision_drive_p, fbe_packet_t * packet_p);
fbe_lifecycle_status_t fbe_provision_drive_join_active(fbe_provision_drive_t *  provision_drive_p, fbe_packet_t * packet_p);
fbe_lifecycle_status_t fbe_provision_drive_join_sync_complete(fbe_provision_drive_t *  provision_drive_p, fbe_packet_t * packet_p);

/* fbe_provision_drive_download.c
 */
fbe_lifecycle_status_t fbe_provision_drive_download_request(fbe_provision_drive_t *  provision_drive_p, fbe_packet_t * packet_p);
fbe_lifecycle_status_t fbe_provision_drive_download_started_originator(fbe_provision_drive_t *  provision_drive_p, fbe_packet_t * packet_p);
fbe_lifecycle_status_t fbe_provision_drive_download_start_non_originator(fbe_provision_drive_t *  provision_drive_p, fbe_packet_t * packet_p);
fbe_lifecycle_status_t fbe_provision_drive_download_stop_non_originator(fbe_provision_drive_t *  provision_drive_p, fbe_packet_t * packet_p);
fbe_lifecycle_status_t fbe_provision_drive_download_pup(fbe_provision_drive_t *  provision_drive_p, fbe_packet_t * packet_p);
fbe_lifecycle_status_t fbe_provision_drive_download_done(fbe_provision_drive_t *  provision_drive_p, fbe_packet_t * packet_p);
fbe_bool_t             fbe_provision_drive_process_download_path_attributes(fbe_provision_drive_t * provision_drive_p, fbe_path_attr_t path_attr);
fbe_lifecycle_status_t fbe_provision_drive_ask_download_trial_run_permission(fbe_provision_drive_t * provision_drive_p, fbe_packet_t * packet_p);
fbe_lifecycle_status_t fbe_provision_drive_send_download_command_to_pdo(fbe_provision_drive_t * provision_drive_p, fbe_packet_t * packet_p, fbe_physical_drive_control_code_t command, fbe_packet_completion_function_t completion_function);
fbe_status_t           fbe_provision_drive_send_download_command_to_pdo_async(fbe_provision_drive_t * provision_drive_p, fbe_packet_t * packet_p, fbe_physical_drive_control_code_t command);

/* fbe_provision_drive_health_check.c
 */
fbe_lifecycle_status_t fbe_provision_drive_health_check_request(fbe_provision_drive_t *  provision_drive_p, fbe_packet_t * packet_p);
fbe_lifecycle_status_t fbe_provision_drive_health_check_abort_req_originator(fbe_provision_drive_t *  provision_drive_p, fbe_packet_t * packet_p);
fbe_lifecycle_status_t fbe_provision_drive_health_check_started_originator(fbe_provision_drive_t *  provision_drive_p, fbe_packet_t * packet_p);
fbe_lifecycle_status_t fbe_provision_drive_health_check_waiting_originator(fbe_provision_drive_t *  provision_drive_p, fbe_packet_t * packet_p);
fbe_lifecycle_status_t fbe_provision_drive_health_check_start_non_originator(fbe_provision_drive_t *  provision_drive_p, fbe_packet_t * packet_p);
fbe_lifecycle_status_t fbe_provision_drive_health_check_failed(fbe_provision_drive_t *  provision_drive_p, fbe_packet_t * packet_p);
fbe_lifecycle_status_t fbe_provision_drive_health_check_peer_failed(fbe_provision_drive_t *  provision_drive_p, fbe_packet_t * packet_p);
fbe_lifecycle_status_t fbe_provision_drive_health_check_done(fbe_provision_drive_t *  provision_drive_p, fbe_packet_t * packet_p);
fbe_bool_t             fbe_provision_drive_process_health_check_path_attributes(fbe_provision_drive_t * provision_drive_p, fbe_path_attr_t path_attr);
fbe_bool_t             fbe_provision_drive_is_health_check_needed(fbe_path_state_t path_state, fbe_path_attr_t path_attr);

/* fbe_provision_drive_slf.c
 */
fbe_status_t fbe_provision_drive_slf_send_packet_to_peer(fbe_provision_drive_t * provision_drive_p, fbe_packet_t * packet_p);
fbe_lifecycle_status_t fbe_provision_drive_eval_slf_request(fbe_provision_drive_t * provision_drive_p, fbe_packet_t * packet_p);
fbe_lifecycle_status_t fbe_provision_drive_eval_slf_started_active(fbe_provision_drive_t *  provision_drive_p, fbe_packet_t * packet_p);
fbe_lifecycle_status_t fbe_provision_drive_eval_slf_started_passive(fbe_provision_drive_t *  provision_drive_p, fbe_packet_t * packet_p);
fbe_lifecycle_status_t fbe_provision_drive_eval_slf_done(fbe_provision_drive_t *  provision_drive_p, fbe_packet_t * packet_p);
fbe_bool_t fbe_provision_drive_initiate_eval_slf(fbe_provision_drive_t *  provision_drive_p,
                                                 fbe_path_attr_t path_state,
                                                 fbe_path_attr_t path_attr);
fbe_status_t fbe_provision_drive_set_slf_flag_based_on_downstream_edge(fbe_provision_drive_t *  provision_drive_p);
fbe_bool_t fbe_provision_drive_slf_need_redirect(fbe_provision_drive_t *provision_drive_p);
fbe_bool_t fbe_provision_drive_slf_need_to_send_passive_request(fbe_provision_drive_t *provision_drive_p);
fbe_bool_t fbe_provision_drive_slf_passive_need_to_join(fbe_provision_drive_t *provision_drive_p);
fbe_bool_t fbe_provision_drive_slf_check_passive_request_after_eval_slf(fbe_provision_drive_t *provision_drive_p);


fbe_bool_t fbe_provision_drive_is_background_request(fbe_payload_block_operation_opcode_t opcode);
fbe_status_t fbe_provision_drive_abort_monitor_ops(fbe_provision_drive_t * const provision_drive_p);

fbe_status_t fbe_provision_drive_utils_get_lba_for_chunk_index(
                                    fbe_provision_drive_t*   provision_drive_p,
                                    fbe_chunk_index_t        chunk_index,
                                    fbe_lba_t*               lba_p);
/* it release the stripe lock when i/o operation completes - set this completion function if we acquire stripe lock for i/o. */
fbe_status_t fbe_provision_drive_block_transport_release_stripe_lock(fbe_packet_t * packet, fbe_packet_completion_context_t context);

fbe_u64_t fbe_provision_drive_class_get_zeroing_cpu_rate(void);
void fbe_provision_drive_class_set_zeroing_cpu_rate(fbe_u64_t zeroing_cpu_rate);
fbe_u64_t fbe_provision_drive_class_get_verify_invalidate_cpu_rate(void);
void fbe_provision_drive_class_set_verify_invalidate_cpu_rate(fbe_u64_t verify_invalidate_cpu_rate);
fbe_u64_t fbe_provision_drive_class_get_sniff_cpu_rate(void);
void fbe_provision_drive_class_set_sniff_cpu_rate(fbe_u64_t sniff_cpu_rate);

fbe_bool_t fbe_provision_drive_is_slf_enabled(void);
void fbe_provision_drive_set_slf_enabled(fbe_bool_t enabled);

fbe_u32_t fbe_provision_drive_get_disk_zeroing_speed(void);
void fbe_provision_drive_set_disk_zeroing_speed(fbe_u32_t disk_zeroing_speed);
fbe_u32_t fbe_provision_drive_get_sniff_speed(void);
void fbe_provision_drive_set_sniff_speed(fbe_u32_t sniff_speed);

fbe_bool_t fbe_provision_drive_metadata_need_reinitialize(fbe_provision_drive_t * provision_drive_p);


fbe_status_t fbe_provision_drive_handle_io_with_lock(fbe_packet_t * packet, fbe_packet_completion_context_t context);

fbe_status_t fbe_provision_drive_metadata_np_flag_set(fbe_provision_drive_t * provision_drive_p,
                                                      fbe_packet_t * packet_p,
                                                      fbe_provision_drive_np_flags_t flags);

fbe_bool_t fbe_provision_drive_metadata_is_np_flag_set(fbe_provision_drive_t * provision_drive_p,
                                                       fbe_provision_drive_np_flags_t flag);
fbe_bool_t fbe_provision_drive_metadata_is_any_np_flag_set(fbe_provision_drive_t * provision_drive_p,
                                                           fbe_provision_drive_np_flags_t flags);

fbe_status_t fbe_provision_drive_reinit_paged_metadata(fbe_provision_drive_t * provision_drive_p,
                                                       fbe_packet_t *packet_p);

/* Accessors for the serial number */
fbe_status_t fbe_provision_drive_set_serial_num(fbe_provision_drive_t *provision_drive, fbe_u8_t *serial_num);

fbe_status_t fbe_provision_drive_get_serial_num(fbe_provision_drive_t *provision_drive, fbe_u8_t **serial_num);

fbe_bool_t fbe_provision_drive_is_write_same_enabled(fbe_provision_drive_t *provision_drive, fbe_lba_t lba);
fbe_bool_t fbe_provision_drive_is_unmap_supported(fbe_provision_drive_t *provision_drive);

fbe_status_t fbe_provision_drive_get_key_info(fbe_provision_drive_t *provision_drive, 
											 fbe_edge_index_t client_index, 
											 fbe_provision_drive_key_info_t ** key_info);

fbe_status_t fbe_provision_drive_invalidate_keys(fbe_provision_drive_t *provision_drive);


fbe_status_t fbe_provision_drive_validate_data(fbe_provision_drive_t * provision_drive,
                                                      fbe_sg_element_t *sg_p,
                                                      fbe_lba_t lba,
                                                      fbe_u32_t blocks,
                                                      fbe_u32_t offset);
fbe_status_t fbe_provision_drive_validate_checksum(fbe_provision_drive_t * provision_drive,
                                               fbe_sg_element_t *sg_p,
                                               fbe_lba_t lba,
                                               fbe_u32_t blocks,
                                               fbe_u32_t offset);
fbe_status_t fbe_provision_drive_register_keys(fbe_provision_drive_t * provision_drive, fbe_packet_t * packet,
                                               fbe_encryption_key_info_t *key_handle,
                                               fbe_edge_index_t server_index);

fbe_status_t fbe_provision_drive_write_default_paged_metadata_set_np_flag(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);


fbe_status_t fbe_provision_drive_init_validation_area(fbe_packet_t * packet_p,
                                                      fbe_packet_completion_context_t context);

fbe_status_t fbe_provision_drive_mark_edge_state(fbe_provision_drive_t *provision_drive_p,
                                                 fbe_edge_index_t client_index);
fbe_status_t fbe_provision_drive_encryption_validation_set(fbe_packet_t * packet_p,
                                                           fbe_packet_completion_context_t context);
fbe_bool_t fbe_provision_drive_validation_area_needs_init(fbe_provision_drive_t * provision_drive_p,
                                                       fbe_u32_t client_index);
fbe_status_t fbe_provision_drive_get_validation_area_bitmap(fbe_provision_drive_t * provision_drive_p,
                                                            fbe_u32_t *bitmap_p);
fbe_status_t fbe_provision_drive_set_encryption_state(fbe_provision_drive_t *provision_drive_p,
                                                      fbe_edge_index_t client_index,
                                                      fbe_base_config_encryption_state_t encryption_state);
fbe_status_t fbe_provision_drive_get_encryption_state(fbe_provision_drive_t *provision_drive_p,
                                                      fbe_edge_index_t client_index,
                                                      fbe_base_config_encryption_state_t *encryption_state_p);
fbe_status_t fbe_provision_drive_encrypt_validate_update_all(fbe_packet_t * packet_p,
                                                             fbe_packet_completion_context_t context);
fbe_lifecycle_status_t fbe_provision_drive_check_key_info(fbe_provision_drive_t *provision_drive_p,
                                                                 fbe_packet_t *packet_p);


/* fbe_provision_drive_metadata_cache.c
 */
fbe_status_t fbe_provision_drive_metadata_cache_init(fbe_provision_drive_t *  provision_drive_p);
fbe_status_t fbe_provision_drive_metadata_cache_destroy(fbe_provision_drive_t *  provision_drive_p);
fbe_bool_t fbe_provision_drive_metadata_cache_is_enabled(fbe_provision_drive_t *  provision_drive_p);
fbe_status_t fbe_provision_drive_metadata_cache_callback_entry(fbe_metadata_paged_cache_action_t action,
                                                               fbe_metadata_element_t * mde,
                                                               fbe_payload_metadata_operation_t * mdo,
                                                               fbe_lba_t start_lba, fbe_block_count_t block_count);
fbe_bool_t fbe_provision_drive_metadata_cache_is_flush_valid(fbe_provision_drive_t * provision_drive_p);
fbe_status_t fbe_provision_drive_metadata_cache_invalidate_bgz_slot(fbe_provision_drive_t * provision_drive_p, 
                                                                    fbe_bool_t force_flush);

/* fbe_provision_drive_unmap_bitmap.c
 */
fbe_status_t fbe_provision_drive_unmap_bitmap_init(fbe_provision_drive_t *  provision_drive_p);
fbe_status_t fbe_provision_drive_unmap_bitmap_destroy(fbe_provision_drive_t *  provision_drive_p);
fbe_status_t fbe_provision_drive_unmap_bitmap_callback_entry(fbe_metadata_paged_cache_action_t action,
                                                             fbe_metadata_element_t * mde,
                                                             fbe_payload_metadata_operation_t * mdo,
                                                             fbe_lba_t start_lba, fbe_block_count_t block_count);
fbe_bool_t fbe_provision_drive_unmap_bitmap_is_lba_range_mapped(fbe_provision_drive_t * provision_drive_p, 
                                                                fbe_lba_t lba, fbe_block_count_t block_count);


fbe_bool_t fbe_provision_drive_is_location_valid(fbe_provision_drive_t *provision_drive_p);

#endif /* PROVISION_DRIVE_PRIVATE_H */

/*******************************
 * end fbe_provision_drive_private.h
 *******************************/


