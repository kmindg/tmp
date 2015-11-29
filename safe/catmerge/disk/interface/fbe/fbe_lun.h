#ifndef FBE_LUN_H
#define FBE_LUN_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_lun.h
 ***************************************************************************
 *
 * @brief
 *  This file contains definitions of functions that are exported by the
 *  lun (logical unit) object.
 * 
 * @ingroup lun_class_files
 * 
 * @revision
 *   5/20/2009:  Created. RPF
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_object.h"
#include "fbe/fbe_xor_api.h"                // for fbe_raid_verify_error_counts_t
#include "fbe/fbe_raid_group.h"

/*Macros for LUN create and LUN destroy timeout*/
#define LU_READY_TIMEOUT           180000
#define LU_DESTROY_TIMEOUT         180000
 /*! @defgroup lun_class Lun Class
 *  @brief This is the set of definitions for the lun.
 *  @ingroup fbe_base_object
 */

/*! @enum fbe_lun_verify_type_t
 *  
 *  @brief These are the types of verify operations that can be performed.
 */
typedef enum fbe_lun_verify_type_e
{
    FBE_LUN_VERIFY_TYPE_NONE = 0,           // No verify
    FBE_LUN_VERIFY_TYPE_USER_RW,            // User read/write verify
    FBE_LUN_VERIFY_TYPE_USER_RO,            // User read-only verify
    FBE_LUN_VERIFY_TYPE_ERROR,              // Error verify  
    FBE_LUN_VERIFY_TYPE_INCOMPLETE_WRITE,   /*!< Verify when LUN shutdown with noncached writes in flight. */
    FBE_LUN_VERIFY_TYPE_SYSTEM,             /*!< Verify type for special system initiated verifies. */
}
fbe_lun_verify_type_t;

/*! @def    FBE_LUN_VERIFY_START_LBA_LUN_START
 *  
 *  @brief  This is a special value that designates the start lab to verify
 */
#define     FBE_LUN_VERIFY_START_LBA_LUN_START  ((fbe_lba_t)0)

/*! @def    FBE_LUN_VERIFY_BLOCKS_ENTIRE_LUN
 *  
 *  @brief  This is a special value that designates to verify the entire lu
 */
#define     FBE_LUN_VERIFY_BLOCKS_ENTIRE_LUN    ((fbe_block_count_t)-1)

/*************************
 *   FUNCTION DEFINITIONS 
 *************************/

/*! @defgroup lun_usurper_interface Lun Usurper Interface
 *  @brief This is the set of definitions that comprise the lun class
 *  usurper interface.
 *  @ingroup fbe_classes_usurper_interface
 */ 

/*!********************************************************************* 
 * @enum fbe_lun_control_code_t 
 *  
 * @brief 
 * This enumeration lists all the lun specific control codes. 
 * These are control codes specific to the lun, which only 
 * the lun object will accept. 
 *         
 **********************************************************************/
typedef enum
{
    FBE_LUN_CONTROL_CODE_INVALID = FBE_OBJECT_CONTROL_CODE_INVALID_DEF(FBE_CLASS_ID_LUN),

    /*! Configure the basic attributes of the raid group such as width.
     */
    FBE_LUN_CONTROL_CODE_SET_CONFIGURATION,

    /*! Initiate a verify operation.
     */
    FBE_LUN_CONTROL_CODE_INITIATE_VERIFY,

    /*! Initiate LUN zero operation.
     */
    FBE_LUN_CONTROL_CODE_INITIATE_ZERO,
    
    /*! Set the nonpaged generation number.
     */
    FBE_LUN_CONTROL_CODE_SET_NONPAGED_GENERATION_NUM,
    
    /*! Clear the verify report.
     */
    FBE_LUN_CONTROL_CODE_CLEAR_VERIFY_REPORT,

    /*! Get the LUN verify status.
     */
    FBE_LUN_CONTROL_CODE_GET_VERIFY_STATUS,

    /*! Get the LUN verify report.
     */
    FBE_LUN_CONTROL_CODE_GET_VERIFY_REPORT,

    /*! Set the raid error counts into the verify report.
     * @todo: this is a temporary usurper command. It will 
     * be replaced by a data event when it is available.
     */
    FBE_LUN_CONTROL_CODE_TEMPORARY_SET_VERIFY_REPORT_COUNTS,

    /* ! Set the power information for the LUN
     */

    FBE_LUN_CONTROL_CODE_SET_POWER_SAVE_INFO,

    /* ! Trespass operation
     */
    FBE_LUN_CONTROL_CODE_TRESPASS_OP,

    /*! Mark blocks in Lun as Bad
     */
    FBE_LUN_CONTROL_CODE_MARK_BLOCK_BAD,

           
    /*! Get the imported capacity of lun
     */
    FBE_LUN_CONTROL_CODE_CLASS_CALCULATE_IMPORTED_CAPACITY,

    /*! Get the exported capacity of lun
     */
    FBE_LUN_CONTROL_CODE_CLASS_CALCULATE_EXPORTED_CAPACITY,

    /*! Prepare to destroy LUN
     */
    FBE_LUN_CONTROL_CODE_PREPARE_TO_DESTROY_LUN,

     /*! set the power saving policy of the LU
     */
    FBE_LUN_CONTROL_CODE_SET_POWER_SAVING_POLICY,

    /*! get the power saving policy of the LU
     */
    FBE_LUN_CONTROL_CODE_GET_POWER_SAVING_POLICY,

    /*! it is used to enable the performance statistics for the lun object.
     */
    FBE_LUN_CONTROL_CODE_ENABLE_PERFORMANCE_STATS,

    /*! it is used to disable the performance statistics for the lun object.
     */
    FBE_LUN_CONTROL_CODE_DISABLE_PERFORMANCE_STATS,

    /*! it is used to get the performance statistics for the lun object.
     */
    FBE_LUN_CONTROL_CODE_GET_PERFORMANCE_STATS,

    /*! it is used to get the zero status for the lun object.
     */
    FBE_LUN_CONTROL_CODE_GET_ZERO_STATUS,

	/*! get various attributes associated with the LUN
     */
    FBE_LUN_CONTROL_CODE_GET_ATTRIBUTES,

	/*! get various info associated with the LUN
     */
    FBE_LUN_CONTROL_CODE_GET_LUN_INFO,

	/*! Export the lun as a device to upper layers by connecting it to BVD objecd
     */
    FBE_LUN_CONTROL_CODE_EXPORT_DEVICE,


	FBE_LUN_CONTROL_CODE_ENABLE_WRITE_BYPASS, /* LUN will complete all incoming writes */
	FBE_LUN_CONTROL_CODE_DISABLE_WRITE_BYPASS,

    /*! it is used to get the rebuild status for the lun object.
     */
    FBE_LUN_CONTROL_CODE_GET_REBUILD_STATUS,

    /*! Used to set unexpected info params or clear them.
     */
    FBE_LUN_CONTROL_CODE_CLEAR_UNEXPECTED_ERROR_INFO,
    /*! Used to enable failing lun on unexpected errors.
     */
    FBE_LUN_CONTROL_CODE_ENABLE_FAIL_ON_UNEXPECTED_ERROR,
    /*! Used to disable failing lun on unexpected errors.
     */
    FBE_LUN_CONTROL_CODE_DISABLE_FAIL_ON_UNEXPECTED_ERROR,

    FBE_LUN_CONTROL_CODE_CLASS_PREPARE_FOR_POWER_SHUTDOWN,
    FBE_LUN_CONTROL_CODE_PREPARE_FOR_POWER_SHUTDOWN,
    FBE_LUN_CONTROL_CODE_CLASS_PREFSTATS_SET_ENABLED,
    FBE_LUN_CONTROL_CODE_CLASS_PREFSTATS_SET_DISABLED,
    FBE_LUN_CONTROL_CODE_CLASS_CALCULATE_CACHE_ZERO_BIT_MAP_SIZE_TO_REMOVE,
    FBE_LUN_CONTROL_CODE_SET_READ_AND_PIN_INDEX,

    FBE_LUN_CONTROL_CODE_ENABLE_IO_LOOPBACK,
    FBE_LUN_CONTROL_CODE_DISABLE_IO_LOOPBACK,
    FBE_LUN_CONTROL_CODE_GET_IO_LOOPBACK,

    /*! Initiate LUN hard zero operation.
    */
    FBE_LUN_CONTROL_CODE_INITIATE_HARD_ZERO,
    
    
    /* Insert new control codes here.
     */
    FBE_LUN_CONTROL_CODE_LAST
}
fbe_lun_control_code_t;

/*! @enum fbe_lun_power_save_mode_t
 *                                                                        
 *  @brief These are the types of power save mode

 */
typedef enum fbe_lun_power_save_mode_e
{
    FBE_LUN_POWER_SAVE_MODE_NONE = 0,
    FBE_LUN_POWER_SAVE_MODE_ON,
    FBE_LUN_POWER_SAVE_MODE_OFF
}
fbe_lun_power_save_mode_t;

/*! @enum fbe_lun_trespass_cmd_t
 *                                                                        
 *  @brief These are the types of tresspass operation

 */
typedef enum fbe_lun_trespass_cmd_e
{
    FBE_LUN_TRESPASS_CMD_NONE = 0,
    FBE_LUN_TRESPASS_OWNERSHIP_LOSS,
    FBE_LUN_TRESPASS_EXECUTE
}fbe_lun_trespass_cmd_t;

/*!*******************************************************************

     whether the LUN should run BV after bind
    fbe_bool_t                  noinitialverify;
 * @struct fbe_lun_initiate_verify_t
 *********************************************************************
 * @brief
 *  This structure is used with the
 *  FBE_LUN_CONTROL_CODE_INITIATE_VERIFY control code.
 *
 *********************************************************************/
typedef struct fbe_lun_initiate_verify_s
{
    /*! The type of lun verify to initiate.
     */
    fbe_lun_verify_type_t verify_type;
    fbe_bool_t            b_verify_entire_lun;
    fbe_lba_t             start_lba;
    fbe_block_count_t     block_count;
}
fbe_lun_initiate_verify_t;


/*!*******************************************************************
 * @struct fbe_lun_get_verify_status_t
 *********************************************************************
 * @brief
 *  This structure contains the background verify status of a LUN
 *  in the RAID group.  It is used by the
 *  FBE_LUN_CONTROL_CODE_GET_VERIFY_STATUS control code.
 *
 *********************************************************************/
typedef struct fbe_lun_get_verify_status_s
{
    fbe_chunk_count_t   total_chunks;       // total number of chunks in the LUN extent
    fbe_chunk_count_t   marked_chunks;      // number of LUN chunks marked for verify
}
fbe_lun_get_verify_status_t;


//*******************************************************************
// @struct fbe_lun_verify_counts_t
//*******************************************************************
// @brief
//  This is the LUN verify results data used in a verify report.
//
//*******************************************************************
typedef struct fbe_lun_verify_counts_s
{
    fbe_u32_t correctable_single_bit_crc;
    fbe_u32_t uncorrectable_single_bit_crc;
    fbe_u32_t correctable_multi_bit_crc;
    fbe_u32_t uncorrectable_multi_bit_crc;
    fbe_u32_t correctable_write_stamp;
    fbe_u32_t uncorrectable_write_stamp;
    fbe_u32_t correctable_time_stamp;
    fbe_u32_t uncorrectable_time_stamp;
    fbe_u32_t correctable_shed_stamp;
    fbe_u32_t uncorrectable_shed_stamp;
    fbe_u32_t correctable_lba_stamp;
    fbe_u32_t uncorrectable_lba_stamp;
    fbe_u32_t correctable_coherency;
    fbe_u32_t uncorrectable_coherency;
    fbe_u32_t uncorrectable_media;
    fbe_u32_t correctable_media;
    fbe_u32_t correctable_soft_media;
    fbe_u32_t reserved[8];                  // reserved for future growth
}
fbe_lun_verify_counts_t;


/*******************************************************************
 * @def FBE_READ_WRITE_HISTOGRAM_SIZE
 *******************************************************************
 * @brief
 *  This represents the Number of elements in read/write histograms
 *
 *******************************************************************/
#define FBE_READ_WRITE_HISTOGRAM_SIZE   10

/*******************************************************************
 * @def FBE_READ_WRITE_HISTOGRAM_SIZE
 *******************************************************************
 * @brief
 *  This represents the size of transfers for overflow pool which is
 *  2**(FBE_READ_WRITE_HISTOGRAM_SIZE) 
 *******************************************************************/
#define FBE_HISTOGRAM_OVF_SIZE          1024

//*******************************************************************
// @def FBE_LUN_VERIFY_REPORT_CURRENT_REVISION
//*******************************************************************
// @brief
//  This represents the current revision of the verify report structure.
//
//*******************************************************************
#define FBE_LUN_VERIFY_REPORT_CURRENT_REVISION  1

/*******************************************************************
 * @def FBE_LUN_METADATA_BITMAP_SIZE
 *******************************************************************
 * @brief
 *  This represents the lun metadata bitmap size: 2 bytes per chunk
 *
 *******************************************************************/
#define FBE_LUN_METADATA_BITMAP_SIZE   2
#define FBE_LUN_DIRTY_FLAGS_SIZE_BLOCKS 256

/*!*******************************************************************
 * @struct fbe_lun_verify_report_t
 *********************************************************************
 * @brief
 *  This is the LUN verify report. This structure is used with the
 *  FBE_LUN_CONTROL_CODE_GET_VERIFY_REPORT control code.
 *
 *********************************************************************/
typedef struct fbe_lun_verify_report_s
{
    fbe_u32_t               revision;               // Revision number of this structure
    fbe_u32_t               pass_count;             // Number of passes completed
    fbe_u32_t               mirror_objects_count;   // Number of mirror objects in raid 10
    fbe_u32_t               reserved[4];            // Reserved space for future growth
    fbe_lun_verify_counts_t current;                // The counts from the current pass
    fbe_lun_verify_counts_t previous;               // The counts from the previous pass
    fbe_lun_verify_counts_t totals;                 // Accumulated total counts
}
fbe_lun_verify_report_t;

/*!*******************************************************************
 * @struct fbe_lun_trespass_op_t
 *********************************************************************
 * @brief
 *  This structure defines the trespass operation. This
 *  structure is used with the FBE_LUN_CONTROL_CODE_TRESPASS_OP
 *  control code.
 *
 *********************************************************************/
typedef struct fbe_lun_trespass_op_s
{
    fbe_lun_trespass_cmd_t trespass_cmd;
}
fbe_lun_trespass_op_t;

/*!*******************************************************************
 * @struct fbe_lun_verify_event_data_t
 *********************************************************************
 * @brief
 *  This is the LUN verify event data. This structure is used with the
 *  FBE_LUN_CONTROL_CODE_TEMPORARY_SET_VERIFY_REPORT_COUNTS control code.
 *
 *********************************************************************/
typedef struct fbe_lun_verify_event_data_s
{
    fbe_bool_t                      pass_completed_b;   // TRUE if pass completed
    fbe_raid_verify_error_counts_t  error_counts;       // The counts from the current pass
}
fbe_lun_verify_event_data_t;


/*!*******************************************************************
 * @struct fbe_lun_control_class_calculate_capacity_t
 *********************************************************************
 * @brief
 *  This is the LUN capacity data structure. This structure is used with the
 *  FBE_LUN_CONTROL_CODE_CLASS_CALCULATE_IMPORTED_CAPACITY and
 *  FBE_LUN_CONTROL_CODE_CLASS_CALCULATE_EXPORTED_CAPACITY code.
 *
 *********************************************************************/
               
typedef struct fbe_lun_control_class_calculate_capacity_s {

    fbe_lba_t       imported_capacity;  /* lun import capacity */
    fbe_lba_t       exported_capacity;  /* lun export capacity */
    fbe_lba_t       lun_align_size;  /* imported capacity is aligned with RG chunk boundary */
}fbe_lun_control_class_calculate_capacity_t;


/*!*******************************************************************
 * @struct fbe_lun_control_class_calculate_cache_zero_bitmap_blocks_t
 *********************************************************************
 * @brief
 *  This is to get the total number of blocks of cache zero bitmap for the given lun capacity.
 *  This structure is used with the FBE_LUN_CONTROL_CODE_CLASS_CALCULATE_CACHE_ZERO_BIT_MAP_SIZE_TO_REMOVE.
 *
 *********************************************************************/
               
typedef struct fbe_lun_control_class_calculate_cache_zero_bitmap_blocks_s {

    fbe_block_count_t lun_capacity_in_blocks;                   /* lun capacity in blocks*/
    fbe_block_count_t cache_zero_bitmap_block_count;            /* cache zero bitmap block count */
}fbe_lun_control_class_calculate_cache_zero_bitmap_blocks_t;


/*!*******************************************************************
 * @struct fbe_api_set_lun_power_saving_policy_t
 *********************************************************************
 * @brief
 *  This is the structure we use to set the power saving policy for the LUN. 
 *  It goes with FBE_LUN_CONTROL_CODE_SET_POWER_SAVING_POLICY
 *********************************************************************/
typedef struct fbe_lun_set_power_saving_policy_s{
    fbe_u64_t lun_delay_before_hibernate_in_sec; /*!< similar to StandbyConditionTimerin100ns in the FALRE_POWER_SAVING_POLICY strucutre
                                                it means how long shall we wait from the timer we notified the user we are going to hibernate, until 
                                                the time we really go to hibernate(this applies to LU only)*/
    fbe_time_t max_latency_allowed_in_100ns; /*!< How long is the user willing to wait for the LU to become active after getting out of power saving */
}fbe_lun_set_power_saving_policy_t;

/*!*******************************************************************
 * @struct fbe_lun_get_zero_status_s
 *********************************************************************
 * @brief
 *  This is the structure we use to set the power saving policy for the LUN. 
 *  It goes with FBE_LUN_CONTROL_CODE_GET_ZERO_STATUS
 *********************************************************************/
typedef struct fbe_lun_get_zero_status_s {
    fbe_lba_t   zero_checkpoint;    /*!< zero checkpoint for LUN object. */
    fbe_u16_t   zero_percentage;    /*!< zero percentage for the LUN object. */
} fbe_lun_get_zero_status_t;

enum fbe_lun_public_constants_s{
    FBE_LUN_POWER_SAVE_DELAY_DEFAULT  = 120,
    FBE_LUN_MAX_LATENCY_TIME_DEFAULT = 120,
    FBE_LUN_DEFAULT_POWER_SAVE_IDLE_TIME = 1800,/*LUN will sleep by default after 30 min. w/o activity*/
};


/*!****************************************************************************
 * @struct fbe_lun_performance_statistics_t
 *
 * @brief
 *    This structure contains LUN specific Performance statistics counters.
 *
 ******************************************************************************/
typedef struct fbe_lun_performance_statistics_s
{
    /*! Number of times an I/O crossed a stripe
     */
    fbe_atomic_32_t stripe_crossings;
    /*! mr3 writes on unit
     */
    fbe_atomic_32_t mr3_writes;
    /*! Sum of lengths of current request queues 
     *  seen by each user request to this LUN
     */
    fbe_atomic_t sum_q_length_arrival;
    /*! Number of requests which encountered at least one other
     *  request in progress on arrivaldirty flag for the lun object. 
     */
    fbe_atomic_32_t arrivals_to_nonzero_q;
    /*! Number of blocks read on unit
     */
    fbe_atomic_32_t blocks_read;
    /*! Number of blocks written on unit
     */
    fbe_atomic_32_t blocks_written;
    /*! Number of disk reads on unit. 
     */
    fbe_atomic_32_t disk_reads[FBE_RAID_MAX_DISK_ARRAY_WIDTH];
    /*! Number of disk writes on unit. 
     */
    fbe_atomic_32_t disk_writes[FBE_RAID_MAX_DISK_ARRAY_WIDTH];
    /*! Number of disk block read on unit. 
     */
    fbe_atomic_t disk_blocks_read[FBE_RAID_MAX_DISK_ARRAY_WIDTH];
    /*! Number of disk block written on unit. 
     */
    fbe_atomic_t disk_blocks_written[FBE_RAID_MAX_DISK_ARRAY_WIDTH];
    /*! read histogram. 
     */
    fbe_atomic_32_t read_histogram[FBE_READ_WRITE_HISTOGRAM_SIZE];
    /*! read histogram.overflow 
     */
    fbe_atomic_32_t read_histogram_overflow;
    /*! write histogram. 
     */
    fbe_atomic_32_t write_histogram[FBE_READ_WRITE_HISTOGRAM_SIZE];
    /*! write histogram overflow
     */
    fbe_atomic_32_t write_histogram_overflow;
    /*! Calculated average read time for this unit 
     */
    fbe_atomic_t cum_read_time;  
    /*! Calculated average write time for this unit 
     */
    fbe_atomic_t cum_write_time;     
    /*! timestamp
     */
    fbe_atomic_t timestamp;
}
fbe_lun_performance_statistics_t;

typedef fbe_u32_t fbe_lun_attributes_t;

/*FBE_LUN_CONTROL_CODE_GET_ATTRIBUTES*/
typedef struct fbe_lun_get_attributes_s{
	fbe_lun_attributes_t lun_attr;
}fbe_lun_get_attributes_t;

/*!*******************************************************************
 * @struct fbe_lun_get_lun_info_s
 *********************************************************************
 * @brief
 *  This is the structure we use to get information for the LUN. 
 *  It goes with FBE_LUN_CONTROL_CODE_GET_LUN_INFO
 *********************************************************************/
typedef struct fbe_lun_get_lun_info_s {
    fbe_lifecycle_state_t   peer_lifecycle_state;    /*!< peer lifecycle_state. */
    fbe_lba_t               capacity;          /*!< LU Capacity */    
    fbe_lba_t               offset; /*!< Lun offset */
    fbe_bool_t              b_initial_verify_requested; /*!< When LUN was created, was initial verify requested? */
    fbe_bool_t              b_initial_verify_already_run; /*!< Did the initial verify run on this lun? */
} fbe_lun_get_lun_info_t;

/*for user luns, the sp cahce needs some extra space at the end of the LUN it can access, but we don't want to expose it to the end users*/
#define MCC_ZERO_BITMAP_HEADER_SIZE 128
#define MCC_ZERO_BITMAP_CHUNKS_SIZE 128
#define MCC_ZERO_BITMAP_GB_PER_CHUNK 512

fbe_status_t fbe_lun_calculate_cache_zero_bit_map_size(IN fbe_block_count_t lun_capacity, OUT fbe_block_count_t *blocks);
fbe_status_t fbe_lun_calculate_cache_zero_bit_map_size_to_remove(fbe_block_count_t lun_capacity, fbe_block_count_t *blocks);


typedef fbe_u64_t fbe_lun_clustered_flags_t;

/*!****************************************************************************
 * @struct fbe_lun_metadata_memory_t
 *
 * @brief
 *    This structure contains LU metadata memory information.
 *
 ******************************************************************************/
typedef struct fbe_lun_metadata_memory_s
{
    fbe_base_config_metadata_memory_t base_config_metadata_memory; /* MUST be first */
    fbe_lun_clustered_flags_t flags;
}
fbe_lun_metadata_memory_t;


/*!*******************************************************************
 * @enum fbe_lun_nonpaged_flags_t
 *********************************************************************
 * @brief Set of non-paged flags.
 *
 *********************************************************************/
enum fbe_lun_nonpaged_flags_e
{
    FBE_LUN_NONPAGED_FLAGS_NONE = 0,
    FBE_LUN_NONPAGED_FLAGS_INITIAL_VERIFY_RAN = 0x01,/*!< Started initial verify when zeroing was done. */
};
typedef fbe_u8_t fbe_lun_nonpaged_flags_t;

#pragma pack(1)
/*!****************************************************************************
 *    
 * @struct fbe_lun_nonpaged_metadata_s
 *  
 * @brief 
 *   This is the metadata for the lun object
 *   Currently it has io counters and dirty bit
 *   
 ******************************************************************************/
typedef struct fbe_lun_nonpaged_metadata_s
{
    /*! nonpaged metadata which is common to all the objects derived from
     *  the base config.
     */
    fbe_base_config_nonpaged_metadata_t base_config_nonpaged_metadata;  /* MUST be first */

    fbe_lba_t   zero_checkpoint;
	fbe_bool_t	has_been_written;/*indicated user data exists on the drive(required from cache)*/
    fbe_lun_nonpaged_flags_t flags; 

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
fbe_lun_nonpaged_metadata_t;
#pragma pack()

/*!*******************************************************************
 * @struct fbe_lun_rebuild_status_s
 *********************************************************************
 * @brief
 *  This is the structure we use to represent lun rebuild status for the LUN. 
 *  It goes with FBE_LUN_CONTROL_CODE_GET_REBUILD_STATUS
 *********************************************************************/
typedef struct fbe_lun_rebuild_status_s {
    fbe_lba_t   rebuild_checkpoint;    /*!< rebuild checkpoint for LUN object. */
    fbe_u16_t   rebuild_percentage;    /*!< rebuild percentage for the LUN object. */
} fbe_lun_rebuild_status_t;

/*!*******************************************************************
 * @struct fbe_lun_init_zero_s
 *********************************************************************
 * @brief
 *  This is the structure we use to init zero lun and it indicate whether
 *  ignore the ndb_b flag of LUN to force zero
 *********************************************************************/
typedef struct fbe_lun_init_zero_s {
    fbe_bool_t   force_init_zero;    /*!< force init zero and ignore ndb_b flag of LUN. */
} fbe_lun_init_zero_t;

/*!*******************************************************************
 * @struct fbe_lun_hard_zero_s
 *********************************************************************
 * @brief
 *  This is the structure we use to init hard zero lun and it indicate the lba range to zero
 *  and how many threads used to hard zero
 *********************************************************************/
typedef struct fbe_lun_hard_zero_s {
    fbe_lba_t         lba;
    fbe_block_count_t blocks;
    fbe_u64_t         threads;
    fbe_bool_t        clear_paged_metadata;
} fbe_lun_hard_zero_t;


/*!*******************************************************************
 * @struct fbe_lun_set_read_and_pin_index_s
 *********************************************************************
 * @brief
 *  This is the structure we use to set the read and pin index of the lun.
 *********************************************************************/
typedef struct fbe_lun_set_read_and_pin_index_s {
    fbe_lun_index_t index;
} fbe_lun_set_read_and_pin_index_t;

#endif /* FBE_LUN_H */

/*************************
 * end file fbe_lun.h
 *************************/
