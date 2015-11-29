#ifndef FBE_RDGEN_H
#define FBE_RDGEN_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_rdgen.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the interface for the rdgen service.
 *  This service is named after the raid I/O generator.
 *  This service allows for generating I/O to any block edge.
 *
 * @version
 *   5/28/2009:  Created. rfoley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe_service_manager.h"
#include "fbe/fbe_topology_interface.h"
#include "fbe/fbe_data_pattern.h"
/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/*!*******************************************************************
 * @def FBE_RDGEN_DEFAULT_BLOCKS
 *********************************************************************
 * @brief
 *   The default number of blocks we will start rdgen with.
 *********************************************************************/
#define FBE_RDGEN_DEFAULT_BLOCKS 0x100

/*!*******************************************************************
 * @def FBE_RDGEN_MAX_THREADS_PER_IO_SPECIFICATION
 *********************************************************************
 * @brief
 *   The max number of concurrent requests that an io specification
 *   will be allowed to start.
 *********************************************************************/
#define FBE_RDGEN_MAX_THREADS_PER_IO_SPECIFICATION 128

/*!*******************************************************************
 * @def FBE_RDGEN_ABORT_TIME 
 *********************************************************************
 * @brief
 *   The time to cancel the IO requests in rdgen in millisecond
 *  FBE_RDGEN_ABORT_TIME_ZERO : zero sec 
 *  FBE_RDGEN_ABORT_TIME_ONE_TENTH_SEC : 1/10th of a second (100 msec)
 *  FBE_RDGEN_ABORT_TIME_TWO_SEC : two seconds (2000 msec)
 *********************************************************************/
#define FBE_RDGEN_ABORT_TIME_ZERO_MSEC 0
#define FBE_RDGEN_ABORT_TIME_ONE_TENTH_MSEC 100
#define FBE_RDGEN_ABORT_TIME_TWO_SEC 2000

#define FBE_RDGEN_INVALID_OBJECT_NUMBER 0x7FFFFFFD

typedef enum fbe_rdgen_control_code_e {
    FBE_RDGEN_SERVICE_CONTROL_CODE_INVALID = FBE_SERVICE_CONTROL_CODE_INVALID_DEF(FBE_SERVICE_ID_RDGEN),

    FBE_RDGEN_CONTROL_CODE_START_IO,
    FBE_RDGEN_CONTROL_CODE_STOP_IO,
    FBE_RDGEN_CONTROL_CODE_GET_STATS,
    FBE_RDGEN_CONTROL_CODE_RESET_STATS,
    FBE_RDGEN_CONTROL_CODE_GET_OBJECT_INFO,
    FBE_RDGEN_CONTROL_CODE_GET_REQUEST_INFO,
    FBE_RDGEN_CONTROL_CODE_GET_THREAD_INFO,
	FBE_RDGEN_CONTROL_CODE_TEST_CMI_PERFORMANCE,
	FBE_RDGEN_CONTROL_CODE_INIT_DPS_MEMORY,
	FBE_RDGEN_CONTROL_CODE_RELEASE_DPS_MEMORY,
	FBE_RDGEN_CONTROL_CODE_ALLOC_PEER_MEMORY,
	FBE_RDGEN_CONTROL_CODE_SET_TIMEOUT,
	FBE_RDGEN_CONTROL_CODE_ENABLE_SYSTEM_THREADS,
	FBE_RDGEN_CONTROL_CODE_DISABLE_SYSTEM_THREADS,
	FBE_RDGEN_CONTROL_CODE_UNLOCK,
	FBE_RDGEN_CONTROL_CODE_SET_SVC_OPTIONS,

    FBE_RDGEN_SERVICE_CONTROL_CODE_LAST
} fbe_rdgen_control_code_t;

/* FBE_RDGEN_CONTROL_CODE_START_IO */

/*!*******************************************************************
 * @enum fbe_rdgen_filter_type_t
 *********************************************************************
 * @brief
 *  Describes which objects to run I/O on.
 *********************************************************************/
typedef enum fbe_rdgen_filter_type_e
{
    FBE_RDGEN_FILTER_TYPE_INVALID = 0,

    /*! Run I/O to a specific object.
     */
    FBE_RDGEN_FILTER_TYPE_OBJECT,

    /*! Run I/O to all objects of a given class. 
     */
    FBE_RDGEN_FILTER_TYPE_CLASS,

    /*! Run I/O to a specific object using IRP/DCAL interface 
     */
    FBE_RDGEN_FILTER_TYPE_DISK_INTERFACE,
    /*! Run I/O to all objects of a given rg. 
     */
    FBE_RDGEN_FILTER_TYPE_RAID_GROUP,

    /*! Start a sequence of I/Os.
     */
    FBE_RDGEN_FILTER_TYPE_PLAYBACK_SEQUENCE,

    /*! Start a sequence of I/Os.
     */
    FBE_RDGEN_FILTER_TYPE_BLOCK_DEVICE,

    FBE_RDGEN_FILTER_TYPE_LAST
}
fbe_rdgen_filter_type_t;

/*!*******************************************************************
 * @enum fbe_rdgen_operation_t
 *********************************************************************
 * @brief
 *  Describes the operation rdgen will perform.
 *********************************************************************/
typedef enum fbe_rdgen_operation_e
{
    FBE_RDGEN_OPERATION_INVALID = 0,

    FBE_RDGEN_OPERATION_READ_ONLY, /*!< Only issue reads. */
    FBE_RDGEN_OPERATION_WRITE_ONLY, /*!< Only issue write commands. */
    FBE_RDGEN_OPERATION_WRITE_READ_CHECK, /*!< write a pattern, read, and compare to what we wrote. */
    FBE_RDGEN_OPERATION_VERIFY, /*!< Simply issue a verify. */
    FBE_RDGEN_OPERATION_READ_ONLY_VERIFY, /*!< Verify, but do not write out corrections. */
    FBE_RDGEN_OPERATION_WRITE_SAME, /*!< Only issue write same commands. */
    FBE_RDGEN_OPERATION_WRITE_SAME_READ_CHECK, /*!< write a pattern, read, and compare to what we wrote. */
    FBE_RDGEN_OPERATION_WRITE_VERIFY, /*!< Write-verify only */
    FBE_RDGEN_OPERATION_WRITE_VERIFY_READ_CHECK, /*!< Write-verify a pattern, read, and compare to what we wrote. */
    FBE_RDGEN_OPERATION_VERIFY_WRITE,            /*!< Verify and then Write */
    FBE_RDGEN_OPERATION_VERIFY_WRITE_READ_CHECK, /*!< Verify, Write a pattern, read, and compare to what we wrote. */
    FBE_RDGEN_OPERATION_READ_CHECK, /*!< read, and compare to a specific pattern. */
    FBE_RDGEN_OPERATION_ZERO_READ_CHECK, /*!< Zero, read, and compare to what we wrote. */
    FBE_RDGEN_OPERATION_ZERO_ONLY, /*!< Zero. */
    FBE_RDGEN_OPERATION_CORRUPT_CRC_READ_CHECK, /*!< Write corrupt crc, read, verify bad invalidated data writen. */
    FBE_RDGEN_OPERATION_CORRUPT_DATA_ONLY, /*!< Write corrupt data. */
    FBE_RDGEN_OPERATION_CORRUPT_DATA_READ_CHECK, /*!< Write corrupt data and read back. */
    FBE_RDGEN_OPERATION_READ_AND_BUFFER, /*!< Read in data, and buffer it locked in exclusive mode. */
    FBE_RDGEN_OPERATION_READ_AND_BUFFER_UNLOCK, /*!< Finishes the read and buffer. */
    FBE_RDGEN_OPERATION_READ_AND_BUFFER_UNLOCK_FLUSH, /*!< Ensures that a write can occur. */
    FBE_RDGEN_OPERATION_UNLOCK_RESTART, /*!< Ensures that a write can occur. */
    FBE_RDGEN_OPERATION_UNMAP, /*!< Only issue unmap commands. */


    FBE_RDGEN_OPERATION_LAST /*!<  Must be last. */
}
fbe_rdgen_operation_t;

/*!*******************************************************************
 * @enum fbe_rdgen_interface_type_t
 *********************************************************************
 * @brief Type of interface to use to deliver I/O.
 *
 *********************************************************************/
typedef enum fbe_rdgen_interface_type_e
{
    FBE_RDGEN_INTERFACE_TYPE_INVALID = 0,
    FBE_RDGEN_INTERFACE_TYPE_PACKET,
    FBE_RDGEN_INTERFACE_TYPE_IRP_DCA,
    FBE_RDGEN_INTERFACE_TYPE_IRP_MJ,
    FBE_RDGEN_INTERFACE_TYPE_IRP_SGL,
    FBE_RDGEN_INTERFACE_TYPE_SYNC_IO,
    FBE_RDGEN_INTERFACE_TYPE_LAST,
}
fbe_rdgen_interface_type_t;
/*!************************************************************
 * @def FBE_RDGEN_OPERATION_STRINGS
 *
 * @brief  Each string represents a different operation
 *
 * @notes This structure must be kept in sync with the enum above.
 *
 *************************************************************/
#define FBE_RDGEN_OPERATION_STRINGS\
 "FBE_RDGEN_OPERATION_INVALID",\
 "FBE_RDGEN_OPERATION_READ_ONLY",\
 "FBE_RDGEN_OPERATION_WRITE_ONLY",\
 "FBE_RDGEN_OPERATION_WRITE_READ_CHECK",\
 "FBE_RDGEN_OPERATION_VERIFY",\
 "FBE_RDGEN_OPERATION_READ_ONLY_VERIFY",\
 "FBE_RDGEN_OPERATION_WRITE_SAME",\
 "FBE_RDGEN_OPERATION_WRITE_SAME_READ_CHECK",\
 "FBE_RDGEN_OPERATION_WRITE_VERIFY",\
 "FBE_RDGEN_OPERATION_WRITE_VERIFY_READ_CHECK",\
 "FBE_RDGEN_OPERATION_VERIFY_WRITE",\
 "FBE_RDGEN_OPERATION_VERIFY_WRITE_READ_CHECK",\
 "FBE_RDGEN_OPERATION_READ_CHECK",\
 "FBE_RDGEN_OPERATION_ZERO_READ_CHECK",\
 "FBE_RDGEN_OPERATION_ZERO_ONLY",\
 "FBE_RDGEN_OPERATION_CORRUPT_CRC_READ_CHECK",\
 "FBE_RDGEN_OPERATION_CORRUPT_DATA_ONLY",\
 "FBE_RDGEN_OPERATION_CORRUPT_DATA_READ_CHECK",\
 "FBE_RDGEN_OPERATION_LAST"

/* Returns true if this operation is checking data.
 */
static __inline fbe_bool_t fbe_rdgen_operation_is_checking(fbe_rdgen_operation_t op)
{
    fbe_bool_t b_is_checking = FBE_FALSE;
    switch (op)
    {
        case FBE_RDGEN_OPERATION_WRITE_READ_CHECK:
        case FBE_RDGEN_OPERATION_WRITE_SAME_READ_CHECK:
        case FBE_RDGEN_OPERATION_WRITE_VERIFY_READ_CHECK:
        case FBE_RDGEN_OPERATION_VERIFY_WRITE_READ_CHECK:
        case FBE_RDGEN_OPERATION_READ_CHECK:
        case FBE_RDGEN_OPERATION_READ_AND_BUFFER:
        case FBE_RDGEN_OPERATION_ZERO_READ_CHECK:
        case FBE_RDGEN_OPERATION_CORRUPT_CRC_READ_CHECK:
        case FBE_RDGEN_OPERATION_CORRUPT_DATA_READ_CHECK:
            b_is_checking = FBE_TRUE;
            break;

        default:
            b_is_checking = FBE_FALSE;
            break;
    };
    return  b_is_checking;
}
/* end fbe_rdgen_operation_is_checking() */

/* Returns true if this operation is modifying data.
 */
static __inline fbe_bool_t fbe_rdgen_operation_is_modify(fbe_rdgen_operation_t op)
{
    fbe_bool_t b_is_modify = FBE_FALSE;
    switch (op)
    {
        case FBE_RDGEN_OPERATION_WRITE_ONLY:
        case FBE_RDGEN_OPERATION_CORRUPT_DATA_ONLY:
        case FBE_RDGEN_OPERATION_VERIFY_WRITE:
        case FBE_RDGEN_OPERATION_ZERO_ONLY:
        case FBE_RDGEN_OPERATION_WRITE_READ_CHECK:
        case FBE_RDGEN_OPERATION_WRITE_SAME_READ_CHECK:
        case FBE_RDGEN_OPERATION_WRITE_VERIFY_READ_CHECK:
        case FBE_RDGEN_OPERATION_VERIFY_WRITE_READ_CHECK:
        case FBE_RDGEN_OPERATION_ZERO_READ_CHECK:
        case FBE_RDGEN_OPERATION_CORRUPT_CRC_READ_CHECK:
        case FBE_RDGEN_OPERATION_CORRUPT_DATA_READ_CHECK:
            b_is_modify = FBE_TRUE;
            break;

        default:
            b_is_modify = FBE_FALSE;
            break;
    };
    return b_is_modify;
}
/* end fbe_rdgen_operation_is_modify() */

/*!*******************************************************************
 * @enum fbe_rdgen_affinity_t
 *********************************************************************
 * @brief
 *  Describes how to affine this operation
 *********************************************************************/
typedef enum fbe_rdgen_affinity_e
{
    FBE_RDGEN_AFFINITY_INVALID = 0,
    FBE_RDGEN_AFFINITY_NONE, /*!< Randomly pick the core. */
    FBE_RDGEN_AFFINITY_SPREAD, /*!< Spread all threads amongst different cores. */
    FBE_RDGEN_AFFINITY_FIXED, /*!< The threads should be fixed to a single core. */
    FBE_RDGEN_AFFINITY_LAST
}
fbe_rdgen_affinity_t;

/*!*******************************************************************
 * @enum fbe_rdgen_sp_id_t
 *********************************************************************
 * @brief
 *  Specifies SP identifier
 *********************************************************************/
typedef fbe_data_pattern_sp_id_t fbe_rdgen_sp_id_t;

/*!*******************************************************************
 * @enum fbe_rdgen_lba_specification_t
 *********************************************************************
 * @brief
 *  Describes how rdgen will choose the lba.
 *********************************************************************/
typedef enum fbe_rdgen_lba_specification_e
{
    FBE_RDGEN_LBA_SPEC_INVALID = 0,

    FBE_RDGEN_LBA_SPEC_RANDOM, /*!< Randomly pick the lba. */
    FBE_RDGEN_LBA_SPEC_FIXED, /*!< Use the fixed lba provided. */
    FBE_RDGEN_LBA_SPEC_SEQUENTIAL_INCREASING, /*!< Increase lba sequentially.*/
    FBE_RDGEN_LBA_SPEC_SEQUENTIAL_DECREASING, /*!< Decrease lba sequentially. */
    FBE_RDGEN_LBA_SPEC_CATERPILLAR_INCREASING, /*!< Sequential increasing with multiple threads. */
    FBE_RDGEN_LBA_SPEC_CATERPILLAR_DECREASING, /*!< Sequential decreasing with multiple threads. */

    FBE_RDGEN_LBA_SPEC_LAST
}
fbe_rdgen_lba_specification_t;

/*!*******************************************************************
 * @enum fbe_rdgen_block_specification_t
 *********************************************************************
 * @brief
 *  Describes how rdgen will choose the blocks.
 *********************************************************************/
typedef enum fbe_rdgen_block_specification_e
{
    FBE_RDGEN_BLOCK_SPEC_INVALID = 0,

    FBE_RDGEN_BLOCK_SPEC_RANDOM, /*!< Randomly pick the blocks. */
    FBE_RDGEN_BLOCK_SPEC_CONSTANT, /*!< Use the fixed blocks provided. */
    FBE_RDGEN_BLOCK_SPEC_STRIPE_SIZE, /*< I/O size will match the stripe size.*/
    FBE_RDGEN_BLOCK_SPEC_INCREASING, /*!< Increase the block count up to a maximum. */
    FBE_RDGEN_BLOCK_SPEC_DECREASING, /*!< Decrease the block count down to a minimum. */

    FBE_RDGEN_BLOCK_SPEC_LAST
}
fbe_rdgen_block_specification_t;


/*!*******************************************************************
 * @enum fbe_rdgen_extra_options_t
 *********************************************************************
 * @brief
 *  Options flags that can be used to tailor the I/O.
 *********************************************************************/
enum fbe_rdgen_extra_options_e
{
    FBE_RDGEN_EXTRA_OPTIONS_INVALID = 0,

        /* These are flags for now, we may later decide to make these into
         *  their own enum.  
         */
    FBE_RDGEN_EXTRA_OPTIONS_RANDOM_DELAY   = 0x00000001,   /*!< Use Random delays */
    FBE_RDGEN_EXTRA_OPTIONS_NEXT           = 0x80000000,   /*!< Next available flag value */

};
typedef fbe_u64_t fbe_rdgen_extra_options_t;
/*!*******************************************************************
 * @enum fbe_rdgen_options_t
 *********************************************************************
 * @brief
 *  Options flags that can be used to tailor the I/O.
 *********************************************************************/
enum fbe_rdgen_options_e
{
    FBE_RDGEN_OPTIONS_INVALID = 0,

        /* These are flags for now, we may later decide to make these into
         *  their own enum.  
         */
    FBE_RDGEN_OPTIONS_UNUSED_0                  = 0x00000001,   /*!< Not used yet. */
    FBE_RDGEN_OPTIONS_DO_NOT_ALIGN_TO_OPTIMAL   = 0x00000002,   /*!< Do NOT align write I/Os to the optimal (physical) block size. */
    FBE_RDGEN_OPTIONS_RANDOM_ABORT              = 0x00000004,   /*!< Randomly abort I/Os in 0..io_spec->msecs_to_abort seconds. */
    FBE_RDGEN_OPTIONS_ALLOW_BREAKUP             = 0x00000008,   /*!< Allow I/Os to breakup into randomly sized smaller pieces.  */   
    FBE_RDGEN_OPTIONS_CONTINUE_ON_ERROR         = 0x00000010,   /*!< Continue even if we get a media error or aborted error. */ 
    FBE_RDGEN_OPTIONS_NON_CACHED                = 0x00000020,   /*!< This operation not from cache.*/
    FBE_RDGEN_OPTIONS_STRIPE_ALIGN              = 0x00000040,   /*!< Align to the stripe size. */
    FBE_RDGEN_OPTIONS_DO_NOT_CHECK_CRC          = 0x00000080,   /*!< Don't check the crc. */
    FBE_RDGEN_OPTIONS_USE_SEQUENCE_COUNT_SEED   = 0x00000100,   /*!< Start will use sequence_count_seed supplied */
    FBE_RDGEN_OPTIONS_DO_NOT_RANDOMIZE_CORRUPT_OPERATION  = 0x00000200, /*!< Don't randomize corrupt operation */
    FBE_RDGEN_OPTIONS_PERFORMANCE               = 0x00000400,   /*!< Performance optimized mode. */
    FBE_RDGEN_OPTIONS_DO_NOT_PANIC_ON_DATA_MISCOMPARE = 0x00000800, /*!< Do not panic when we get a data compare error. */
    FBE_RDGEN_OPTIONS_USE_PRIORITY              = 0x00001000,   /*!< Use the specified traffic priority. */
    FBE_RDGEN_OPTIONS_CREATE_REGIONS            = 0x00002000,   /*!< Create the list of regions. */
    FBE_RDGEN_OPTIONS_USE_REGIONS               = 0x00004000,   /*!< Create the list of regions. */
    FBE_RDGEN_OPTIONS_DO_NOT_CHECK_INVALIDATION_REASON = 0x00008000, /*!< Do not validate that the invalidation reason is valid. */
    FBE_RDGEN_OPTIONS_FAIL_ON_MEDIA_ERROR       = 0x00010000,   /*!< Stop processing a request when a media error is detected. */
    FBE_RDGEN_OPTIONS_DISABLE_OVERLAPPED_IOS    = 0x00020000,   /*!< For non-check requests disables overlapping I/Os. */
    FBE_RDGEN_OPTIONS_PANIC_ON_ALL_ERRORS       = 0x00040000,   /*!< Panic on all errors. */
    FBE_RDGEN_OPTIONS_DISABLE_RANDOM_SG_ELEMENT = 0x00080000,   /*!< Prevent RDGEN from randomly adding a non-NULL terminated SG element. */
    FBE_RDGEN_OPTIONS_PERFORMANCE_BYPASS_TERMINATOR = 0x00100000,   /*!< Terminate I/O at port level */  
    FBE_RDGEN_OPTIONS_INVALID_AND_VALID_PATTERNS = 0x00200000,   /*!< We might find a mix of invalid pattern and valid pattern. */
    FBE_RDGEN_OPTIONS_CONTINUE_ON_ALL_ERRORS    = 0x00400000,   /*!< Continue even if we get a non-retryable error. */
    FBE_RDGEN_OPTIONS_RANDOM_START_LBA          = 0x00800000,   /*!< Don't use a start lba, typically used to choose random start lba */
    FBE_RDGEN_OPTIONS_CHECK_ORIGINATING_SP      = 0x01000000,   /*!< Validate which SP wrote the data (used when checking the data) */
    FBE_RDGEN_OPTIONS_FIXED_RANDOM_NUMBERS      = 0x02000000,   /*!< Generate random numbers using repeatable algorithm.  */
    FBE_RDGEN_OPTIONS_ALLOW_FAIL_CONGESTION     = 0x04000000,   /*!< Allow the request to fail for congestion. */
    FBE_RDGEN_OPTIONS_FILE                      = 0x08000000,   /*!< Do file playback. */
    FBE_RDGEN_OPTIONS_OPTIMAL_RANDOM            = 0x10000000,   /*!< Generate random numbers more optimally. */
    FBE_RDGEN_OPTIONS_DISABLE_BREAKUP           = 0x20000000,   /*!< Don't break up I/O once they are generated. */
    FBE_RDGEN_OPTIONS_DISABLE_CAPACITY_CHECK    = 0x40000000,   /*!< Do not check capacity when generating I/O. */
    FBE_RDGEN_OPTIONS_NEXT                      = 0x80000000,   /*!< Next available flag value */
};
typedef fbe_u64_t fbe_rdgen_options_t;
/*!*******************************************************************
 * @enum fbe_rdgen_data_pattern_flags_trdgen_data_pattern_flags
 *********************************************************************
 * @brief
 *  Options flags for generating data patterns.
 *********************************************************************/
typedef enum fbe_rdgen_data_pattern_flags_e
{
    FBE_RDGEN_DATA_PATTERN_FLAGS_INVALID = 0,

    FBE_RDGEN_DATA_PATTERN_FLAGS_DO_NOT_CHECK_CRC          = 0x00000001,   /*!< Don't check the crc. */
    FBE_RDGEN_DATA_PATTERN_FLAGS_USE_SEQUENCE_COUNT_SEED   = 0x00000002,   /*!< Start will use sequence_count_seed supplied */
    FBE_RDGEN_DATA_PATTERN_FLAGS_DO_NOT_RANDOMIZE_CORRUPT_OPERATION  = 0x00000004, /*!< Don't randomize corrupt operation */
    FBE_RDGEN_DATA_PATTERN_FLAGS_CREATE_REGIONS            = 0x00000008,   /*!< Create the list of regions. */
    FBE_RDGEN_DATA_PATTERN_FLAGS_USE_REGIONS               = 0x00000010,   /*!< Create the list of regions. */
    FBE_RDGEN_DATA_PATTERN_FLAGS_DO_NOT_CHECK_INVALIDATION_REASON = 0x00000020, /*!< Do not validate that the invalidation reason is valid. */
    FBE_RDGEN_DATA_PATTERN_FLAGS_INVALID_AND_VALID_PATTERNS = 0x00000040,   /*!< We might find a mix of invalid pattern and valid pattern. */
    FBE_RDGEN_DATA_PATTERN_FLAGS_FIXED_SEQUENCE_NUMBER      = 0x00000080,   /*!< Do not increment sequence Number. */
    FBE_RDGEN_DATA_PATTERN_FLAGS_CHECK_CRC                  = 0x00000100,   /*!< Check the crc. */
    FBE_RDGEN_DATA_PATTERN_FLAGS_CHECK_LBA_STAMP            = 0x00000200,   /*!< Check the lba stamp. */
    FBE_RDGEN_DATA_PATTERN_FLAGS_NEXT                       = 0x00000400,   /*!< Next available flag value */

    FBE_RDGEN_DATA_PATTERN_FLAGS_MAX_VALUE                 = (FBE_RDGEN_DATA_PATTERN_FLAGS_NEXT - 1)
}
fbe_rdgen_data_pattern_flags_t;

/*!*******************************************************************
 * @enum fbe_rdgen_peer_options_t
 *********************************************************************
 * @brief
 *  Options flags that can be used to tailor the way we direct I/O to the peer.
 *********************************************************************/
typedef enum fbe_rdgen_peer_options_e
{
    /*! When the option is set to invalid we only issue locally.
     */
    FBE_RDGEN_PEER_OPTIONS_INVALID = 0,

        /* These are flags for now, we may later decide to make these into
         *  their own enum.  
         */
    FBE_RDGEN_PEER_OPTIONS_SEND_THRU_PEER    = 0x001, /*!< Send using peer. */

    /*! We will always have the same number of threads 
     *   being issued locally and being issued by the peer.
     */
    FBE_RDGEN_PEER_OPTIONS_LOAD_BALANCE      = 0x002, 
    FBE_RDGEN_PEER_OPTIONS_RANDOM            = 0x004, /*!< Randomly send locally or to peer. */

    /*! Determines if we do local write/zero,etc. 
     *  Must be used in conjunction with remote compare and/or local compare. 
     */
    FBE_RDGEN_PEER_OPTIONS_LOCAL_MODIFY      = 0x008,
    /*! Says to do a remote write/zero,etc. 
     * Must be used in conjunction with local compare and/or remote compare. 
     */
    FBE_RDGEN_PEER_OPTIONS_REMOTE_MODIFY     = 0x010,

    /*! Says to do a local compare
     * Must be used in conjunction with remote modify and/or remote compare. 
     */
    FBE_RDGEN_PEER_OPTIONS_LOCAL_COMPARE     = 0x040, 

    /*! Says to do a remote compare
     * Must be used in conjunction with local compare and/or local compare. 
     */
    FBE_RDGEN_PEER_OPTIONS_REMOTE_COMPARE    = 0x080, 
    FBE_RDGEN_PEER_OPTIONS_LAST
}
fbe_rdgen_peer_options_t;

/*!*******************************************************************
 * @enum fbe_rdgen_pattern_t
 *********************************************************************
 * @brief
 *  Pattern to be used when writing.
 *********************************************************************/
typedef enum fbe_rdgen_pattern_e
{
    FBE_RDGEN_PATTERN_INVALID = 0, /*!< Always first. */
    FBE_RDGEN_PATTERN_ZEROS,       /*!< Zeros but with checksum set. */
    FBE_RDGEN_PATTERN_LBA_PASS,    /*!< Lba seed plus pass count. */
    FBE_RDGEN_PATTERN_CLEAR,       /*!< Zeros including meta-data. */
    FBE_RDGEN_PATTERN_INVALIDATED, /*!< Expect invalidated blocks. */
    FBE_RDGEN_PATTERN_LAST         /*!< Always last. */
}
fbe_rdgen_pattern_t;
/*!*******************************************************************
 * @def FBE_RDGEN_DEVICE_NAME_CHARS
 *********************************************************************
 * @brief Max number of chars in a windows device name.
 *
 *********************************************************************/
#define FBE_RDGEN_DEVICE_NAME_CHARS 80

/*!*******************************************************************
 * @struct fbe_rdgen_filter_t
 *********************************************************************
 * @brief
 *  Describes which object to kick I/O off on.
 *********************************************************************/
typedef struct fbe_rdgen_filter_s
{
    /*! Determines if we are running to:  
     * all objects, to all objects of a given class or to a specific object. 
     */
    fbe_rdgen_filter_type_t filter_type;

    /*! The object ID of the object to run I/O for. 
     *  This can be FBE_OBJECT_ID_INVALID, in cases where
     *  we are not runnint I/O to a specific edge.
     */
    fbe_object_id_t object_id; 

    /*! The edge of this object to run I/O on.
     */
    fbe_u32_t edge_index;

    /*! This can be FBE_CLASS_ID_INVALID if class is not used.
     */
    fbe_class_id_t class_id;

    /*! This must always be set and is the package we will run I/O against.
     */
    fbe_package_id_t package_id;

    /*! FBE_TRUE to use driver (IRP/DCALL) interface rather than packet interface.
     */
    fbe_char_t device_name[FBE_RDGEN_DEVICE_NAME_CHARS];
}
fbe_rdgen_filter_t;


/*!*******************************************************************
 * @enum fbe_rdgen_operation_status_t
 *********************************************************************
 * @brief
 *  Generic raid operation status.
 *********************************************************************/
typedef enum fbe_rdgen_operation_status_e
{
    FBE_RDGEN_OPERATION_STATUS_INVALID = 0, /*!< Always first. */
    FBE_RDGEN_OPERATION_STATUS_SUCCESS,     /*!< We were successful. */
    FBE_RDGEN_OPERATION_STATUS_IO_ERROR,    /*!< We got some I/O error. */
    FBE_RDGEN_OPERATION_STATUS_NO_OBJECTS_OF_CLASS,   /*!< Enumeration found 0 objects of this class id. */
    FBE_RDGEN_OPERATION_STATUS_OBJECT_DOES_NOT_EXIST, /*!< This object was not found. */
    FBE_RDGEN_OPERATION_STATUS_PEER_ERROR,  /*!< We got an error from the peer. */
    FBE_RDGEN_OPERATION_STATUS_PEER_OUT_OF_RESOURCES, /*!< Peer is out of resources */
    FBE_RDGEN_OPERATION_STATUS_PEER_DIED,  /*!< Peer went away. */
    FBE_RDGEN_OPERATION_STATUS_MISCOMPARE,  /*!< A miscompare has occurred. */
    FBE_RDGEN_OPERATION_STATUS_INVALID_BLOCK_MISMATCH,  /*!< Unexpected # of invalid blocks.  */
    FBE_RDGEN_OPERATION_STATUS_BAD_CRC_MISMATCH,  /*!< Unexpected # of bad crc blocks.  */
    FBE_RDGEN_OPERATION_STATUS_MEM_UNINITIALIZED,  /*!< The rdgen operation is unexpected.  */
    FBE_RDGEN_OPERATION_STATUS_INVALID_OPERATION,  /*!< DPS not initted yet.  */
    FBE_RDGEN_OPERATION_STATUS_UNEXPECTED_ERROR, /*!< We hit an unhandled condition. */
    FBE_RDGEN_OPERATION_STATUS_UNRESOLVED_CONFLICT, /*!< Unable to issue I/O due to a lock conflict. */
    FBE_RDGEN_OPERATION_STATUS_STOPPED, /*!< I/O was stopped by user. */
    FBE_RDGEN_OPERATION_STATUS_LAST         /*!< Always last. */
}
fbe_rdgen_operation_status_t;

/*!*************************************************
 * @enum fbe_rdgen_memory_type_t
 ***************************************************
 * @brief Tells rdgen to get memory from which location.
 *
 ***************************************************/
typedef enum fbe_rdgen_memory_type_e
{
    FBE_RDGEN_MEMORY_TYPE_INVALID,
    FBE_RDGEN_MEMORY_TYPE_CMM,
    FBE_RDGEN_MEMORY_TYPE_CACHE,
    FBE_RDGEN_MEMORY_TYPE_LAST
}
fbe_rdgen_memory_type_t;


/*!*************************************************
 * @enum fbe_rdgen_io_complete_flags_t
 ***************************************************
 * @brief Tells rdgen what to wait for.
 *
 ***************************************************/
typedef enum fbe_rdgen_io_complete_flags_e
{
    FBE_RDGEN_IO_COMPLETE_FLAGS_NONE,
    FBE_RDGEN_IO_COMPLETE_FLAGS_UNEXPECTED_ERRORS,
    FBE_RDGEN_IO_COMPLETE_FLAGS_LAST,
}
fbe_rdgen_io_complete_flags_t;

/*!*******************************************************************
 * @enum FBE_RDGEN_IO_SPEC_REGION_LIST_LENGTH
 *********************************************************************
 * @brief Number of entries we allow in a region list.
 *
 *********************************************************************/
enum {FBE_RDGEN_IO_SPEC_REGION_LIST_LENGTH = 128};

/*!*******************************************************************
 * @struct fbe_rdgen_io_specification_t
 *********************************************************************
 * @brief
 *  Structure that describes an I/O specification to a given object.
 *********************************************************************/
typedef struct fbe_rdgen_io_specification_s
{
    /*! Type of operation we need.  
     * e.g. write/read/check, write same/read/check, read only, write only, etc. 
     */
    fbe_rdgen_operation_t operation;

    /*! Number of threads to kick off.
     */
    fbe_u32_t threads;

    fbe_rdgen_affinity_t affinity; /*!< How we should affine this request. */
    fbe_u32_t core; /*!< Core to affine this to. */  

    fbe_rdgen_interface_type_t io_interface; /*!< I/O interface to use, packet, irp dca, irp mj, irp sgl, etc. */

    fbe_lba_t start_lba; /*!< Lba to use for the first request. */
    fbe_lba_t min_lba;   /*!< Smallest lba to use. */
    fbe_block_count_t inc_lba; /*!< Amount to incrment lba by. */

    /*!< Largest lba to use. 
     * If this is set to FBE_LBA_INVALID then rdgen 
     * will automatically get the capacity of the object and use this capacity 
     * as the maximum lba.
     */
    fbe_lba_t max_lba;  
    
    fbe_block_count_t min_blocks; /*!< Min number of blocks to use. */
    fbe_block_count_t max_blocks; /*!< Max number of blocks to use. */
    fbe_block_count_t inc_blocks; /*!< Amount to incrment blocks by. */
    /*! If we are aligning to a specific number of blocks, this determines the  
     * alignment factor. 
     */
    fbe_u32_t alignment_blocks;

    /*! If we are performing a limited number of I/Os, this counter is set to 
     *  something other than 0. 
     */
    fbe_u32_t max_passes;

    fbe_rdgen_io_complete_flags_t io_complete_flags; /*!< Tell us when we can consider the I/O finished */

    /*! If we are performing a limited number of passes this counter is set to 
     *  something other than 0. 
     */
    fbe_u32_t max_number_of_ios;

    /*! If number of I/Os is set to 0, then we use time instead of io count  
     * to determine how long we run for. 
     * If this is set to 0, then we run forever. 
     */
    fbe_time_t msec_to_run;

    fbe_rdgen_options_t options;  /*! First 64 options. */
    fbe_rdgen_extra_options_t extra_options; /* Next 64 options*/
    fbe_rdgen_data_pattern_flags_t data_pattern_flags;
    fbe_rdgen_lba_specification_t lba_spec;
    fbe_rdgen_block_specification_t block_spec;

    /*! Specifies the pattern to be used in cases where we are writing.
     */
    fbe_rdgen_pattern_t pattern;

    /*! Specifies if I/Os should be retried or not. 
     *  Default is FBE_FALSE, which causes us to fail on first I/O failure. 
     */
    fbe_bool_t b_retry_ios;

    /*! Milliseconds this packet will be allowed to be outstanding  
     *  before it expires. 
     */
    fbe_time_t msecs_to_expiration;

    /*! Milliseconds this packet will be allowed to be outstanding 
     * before rdgen will cancel the packet.
     */
    fbe_time_t msecs_to_abort;   
    /*! Number of milli to delay the I/O before issuing the next one. 
     *  If the option FBE_RDGEN_OPTIONS_RANDOM_DELAY is set then
     *  we will use this as the max random delay to use.
     */ 
    fbe_time_t msecs_to_delay;

    /*! Optional seed for sequence_count 
     *  (only valid if FBE_RDGEN_OPTIONS_USE_SEQUENCE_COUNT_SEED is set)
     */
    fbe_u32_t sequence_count_seed;

    /*! Optional seed for using fixed random numbers. 
     * This is the starting random seed we will base our seed on. 
     */
    fbe_u32_t random_seed_base;

    fbe_packet_priority_t priority;

    /*! The active side sets this when it sends a request from the peer.  
     * No-one else should be setting this. 
     * This will allow the passive side to block any new requests. 
     */
    fbe_bool_t b_from_peer;

    fbe_rdgen_peer_options_t peer_options; /*! Options to specify how we send using the peer. */

    /*! List of regions that determines the I/Os to perform. 
     */
    fbe_data_pattern_region_list_t region_list[FBE_RDGEN_IO_SPEC_REGION_LIST_LENGTH];
    fbe_u32_t region_list_length; /*! number of regions in the list. */

    /* The below two fields get copied into the memory io master.
     */
    fbe_u8_t client_reject_allowed_flags;   /*!< Flags client gave us. */
    fbe_u8_t arrival_reject_allowed_flags;  /*!< Flags client knew about on arrival. */

    /*! If is FBE_RDGEN_OPTIONS_CHECK_ORIGINATING_SP set this is the expected 
     *  SP id of the data. 
     */
    fbe_rdgen_sp_id_t originating_sp_id;
    fbe_sg_element_t *sg_p; /*!< Caller supplied sg for read buffer.*/
}
fbe_rdgen_io_specification_t;

/*!*******************************************************************
 * @struct fbe_rdgen_status_t
 *********************************************************************
 * @brief
 *  This contains the last packet status for the rdgen ts structure.
 *  We save this here since it needs to get returned to the client
 *  and the packet status values may not be accessable.
 *
 *********************************************************************/
typedef struct fbe_rdgen_status_s
{
    fbe_status_t status; /*!< Packet status */

    /*! Block payload status.
     */
    fbe_payload_block_operation_status_t block_status;

    /*! Block payload qualifier.
     */
    fbe_payload_block_operation_qualifier_t block_qualifier;

    /*! Contains the bad LBA if block_status returned an error
    */
    fbe_lba_t media_error_lba;

    /*! This is the rdgen service's status. 
     *   Rdgen uses this to return its specific status. 
     */
    fbe_rdgen_operation_status_t rdgen_status;

    void* context; /*! For use on multi-part requests to indicate pending context. */
}
fbe_rdgen_status_t;

/* Accessor to set the status.
 */
static __inline void fbe_rdgen_status_set(fbe_rdgen_status_t *rdgen_status_p,
                                   fbe_status_t transport_status,
                                   fbe_payload_block_operation_status_t block_status,
                                   fbe_payload_block_operation_qualifier_t block_qualifier)
{
    rdgen_status_p->status = transport_status;
    rdgen_status_p->block_status = block_status;
    rdgen_status_p->block_qualifier = block_qualifier;
    return;
}

/*!*******************************************************************
 * @struct fbe_rdgen_statistics_t
 *********************************************************************
 * @brief
 *  Contains the set of statistics about the request.
 *
 *********************************************************************/
typedef struct fbe_rdgen_statistics_s
{
    /*! Total number of I/Os completed for the operation. 
     *  This includes individual I/Os needed to complete a pass.
     *  So for example a write/read/check would have two I/os minimum,
     *  but will only account for one pass.
     */
    fbe_u32_t io_count;

    /*! Total number of milliseconds for this request.
     */
    fbe_u64_t elapsed_msec;

    /*! Total number of passes completed for the operation.
     */
    fbe_u32_t pass_count;

    /*! As threads finish we keep track of the min I/O rate
     * of all the threads started for this request. 
     */
    fbe_u32_t min_rate_ios;

    /*! As threads finish we keep track of the min I/O rate
     * of all the threads started for this request. 
     */
    fbe_time_t min_rate_msec;

    /*! This is the object id that got the min ios per sec.
     */
    fbe_object_id_t min_rate_object_id;

    /*! As threads finish we keep track of the max I/O rate
     * of all the threads started for this request. 
     */
    fbe_u32_t max_rate_ios;

    /*! As threads finish we keep track of the max I/O rate
     * of all the threads started for this request. 
     */
    fbe_time_t max_rate_msec;

    /*! This is the object id that got the max ios per sec.
     */
    fbe_object_id_t max_rate_object_id;

    /*! Total number of errors encountered for the operation.
     */
    fbe_u32_t error_count;

    /*! Total number of media errors encountered for the operation.
     */
    fbe_u32_t media_error_count;

    /*! Total number of aborted errors encountered for the operation.
     */
    fbe_u32_t aborted_error_count;

    /*! Total number of io failure errors encountered for the operation.
     */
    fbe_u32_t io_failure_error_count;

    fbe_u32_t congested_err_count;
    fbe_u32_t still_congested_err_count;

    /*! Total number of invalid request errors encountered for the operation.
     */
    fbe_u32_t invalid_request_err_count;

    /*! Total number of invalidated blocks found doing errored reads.
     */
    fbe_u32_t inv_blocks_count;

    /*! Total number of blocks with bad crc's found doing reads.
     */
    fbe_u32_t bad_crc_blocks_count;
    /*! Total number of times we hit lock conflicts.
     */
    fbe_u32_t lock_conflicts;

    /*! Total number of verify encountered for the operation.
     */
    fbe_raid_verify_error_counts_t verify_errors;
}
fbe_rdgen_statistics_t;

/*!*******************************************************************
 * @struct fbe_rdgen_control_start_io_t
 *********************************************************************
 * @brief
 *  Structure used to kick off I/O.
 *********************************************************************/
typedef struct fbe_rdgen_control_start_io_s
{
    /*! Has all information for which objects to kick I/O off on.
     */
    fbe_rdgen_filter_t filter;

    /*! This is the specification of the I/O being run to this object.
     */
    fbe_rdgen_io_specification_t specification;

    /*! This is the set of status about the operation.
     */
    fbe_rdgen_status_t status;

    /*! This is the set of statistics for the operation.
     */
    fbe_rdgen_statistics_t statistics;
}
fbe_rdgen_control_start_io_t;

/*!*******************************************************************
 * @struct fbe_rdgen_control_stop_io_t
 *********************************************************************
 * @brief
 *  Structure used to stop I/O.
 *********************************************************************/
typedef struct fbe_rdgen_control_stop_io_s
{
    /*! Has all information for which objects to stop io.
     */
    fbe_rdgen_filter_t filter;
}
fbe_rdgen_control_stop_io_t;

/*!*******************************************************************
 * @struct fbe_rdgen_io_statistics_t
 *********************************************************************
 * @brief Generic set of io statistics.
 *
 *********************************************************************/
typedef struct fbe_rdgen_io_statistics_s
{
    /*! Total number of I/Os completed for the operation. 
     *  This includes individual I/Os needed to complete a pass.
     *  So for example a write/read/check would have two I/os minimum,
     *  but will only account for one pass.
     */
    fbe_u32_t io_count;

    /*! Total number of passes completed for the operation.
     */
    fbe_u32_t pass_count;

    /*! Total number of errors encountered for the operation.
     */
    fbe_u32_t error_count;

    /*! Total number of media errors encountered for the operation.
     */
    fbe_u32_t media_error_count;

    /*! Total number of aborted errors encountered for the operation.
     */
    fbe_u32_t aborted_error_count;

    /*! Total number of io failure errors encountered for the operation.
     */
    fbe_u32_t io_failure_error_count;

    /*! Total number of congested io failures errors encountered for the operation.
     */
    fbe_u32_t congested_error_count;
    /*! Total number of still congested qualifiers encountered for the operation.
     */
    fbe_u32_t still_congested_error_count;

    /*! Total number of invalid request errors encountered for the operation.
     */
    fbe_u32_t invalid_request_error_count;

    /*! Total number of invalidated blocks during failed reads for the operation.
     */
    fbe_u32_t inv_blocks_count;

    /*! Total number of blocks with bad crc's during reads for the operation.
     */
    fbe_u32_t bad_crc_blocks_count;
    /*! Total I/O requests received from the peer.
     */
    fbe_u32_t requests_from_peer_count;

    /*! Total I/O requests sent to the peer.
     */
    fbe_u32_t requests_to_peer_count;

    /*!< Peer out of resources count 
     */
    fbe_u32_t peer_out_of_resource_count;

    fbe_u32_t pin_count; /*!<  Total pins in progress.  */ 
} 
fbe_rdgen_io_statistics_t;

/*!*******************************************************************
 * @struct fbe_rdgen_control_get_stats_t
 *********************************************************************
 * @brief
 *  Returns information about i/o stats.
 *********************************************************************/
typedef struct fbe_rdgen_control_get_stats_s
{
    /*! Has all information for which objects to get status for.
     */
    fbe_rdgen_filter_t filter;

    fbe_u32_t objects;
    fbe_u32_t threads;
    fbe_u32_t requests;

    /*! Statistics for I/Os specified by the get stats filter.
     */
    fbe_rdgen_io_statistics_t in_process_io_stats;

    /*! These are cumulative statistics for the rdgen service.  
     * The above counters are for ongoing requests. 
     * The below counters are historical. 
     */
    fbe_rdgen_io_statistics_t historical_stats;

    fbe_rdgen_memory_type_t memory_type; /*!< Kind of memory we allocated. */
    fbe_u32_t memory_size_mb;      /*!< Memory size in MB. */
    fbe_time_t io_timeout_msecs; /*!< milliseconds before we timeout I/Os. */
}
fbe_rdgen_control_get_stats_t;

typedef fbe_u64_t fbe_rdgen_handle_t;

/*!*******************************************************************
 * @struct fbe_rdgen_control_thread_info_t
 *********************************************************************
 * @brief
 *  Returns information about threads.
 *********************************************************************/
typedef struct fbe_rdgen_control_thread_info_s
{
    /*! This is the handle to this thread.
     */
    fbe_rdgen_handle_t thread_handle;

    /*! This is the handle to our object.
     */
    fbe_rdgen_handle_t object_handle;

    /*! This is the handle to the request that originated this thread.
     */
    fbe_rdgen_handle_t request_handle;

    /*! set to TRUE if we are aborted.
     */
    fbe_bool_t b_aborted;

    fbe_u32_t err_count;
    fbe_u32_t abort_err_count;
    fbe_u32_t media_err_count;
    fbe_u32_t io_failure_err_count;
    fbe_u32_t congested_err_count;
    fbe_u32_t still_congested_err_count;
    fbe_u32_t invalid_request_err_count;

    /*! This is used when seeding the pattern in a block.  
     * This seed typically does not go beyond 0xFF so we can fit it in 
     * the upper bits of an LBA. 
     */
    fbe_u32_t pass_count;

    /*! This is the number of total I/O passes we have performed.  
     * This does not account for the I/Os that occur due to a breakup. 
     * And if multiple I/Os are needed for a pass (write/read for example), 
     * then this only gets incremented once. 
     */
    fbe_u32_t io_count;

    fbe_u32_t partial_io_count;
    fbe_u32_t memory_size; /*! Number of bytes allocated.*/
    fbe_u32_t elapsed_milliseconds; /*! Number of milliseconds since thread started. */

    fbe_time_t last_response_time;/*!< Last response took this long. */
    fbe_time_t max_response_time; /*!< Max of all responses. */

    fbe_time_t cumulative_response_time; /*!< In milliseconds */
    fbe_time_t cumulative_response_time_us; /*!< In microseconds. */
    fbe_u64_t total_responses; /*!< Number of responses that have gone into cumulative. */

    fbe_raid_verify_error_counts_t verify_errors;
}
fbe_rdgen_control_thread_info_t;

/*!*******************************************************************
 * @struct fbe_rdgen_control_get_thread_info_t
 *********************************************************************
 * @brief Returns information about threads.
 *********************************************************************/
typedef struct fbe_rdgen_control_get_thread_info_s
{
    fbe_u32_t num_threads; /*!< Input number of threads that follow. */

    fbe_rdgen_control_thread_info_t thread_info_array[1]; /*!< Must be last. */
}
fbe_rdgen_control_get_thread_info_t;

/*!*******************************************************************
 * @struct fbe_rdgen_control_request_info_t
 *********************************************************************
 * @brief
 *  Returns information about requests.
 *********************************************************************/
typedef struct fbe_rdgen_control_request_info_s
{
    /*! This is the handle to this request.
     */
    fbe_rdgen_handle_t request_handle;

    /*! This is the count of active ts on this object.
     */
    fbe_u32_t active_ts_count;

    /*! This is the specification of the I/O.
     */
    fbe_rdgen_io_specification_t specification;

    /*! Original filter for this request.
     */
    fbe_rdgen_filter_t filter;

    /*! Set to TRUE when this request has been stopped by the user.
     */
    fbe_bool_t b_stop;

    /*! This is the number of passes completed for this request. 
     */
    fbe_u32_t pass_count;

    /*! This is the number of total I/Os completed for this request. 
     */
    fbe_u32_t io_count;

    /*! Number of errors encountered for this request.
     */
    fbe_u32_t err_count;

    /*! Number of aborted errors encountered for this request.
     */
    fbe_u32_t aborted_err_count;

    /*! Number of media errors encountered for this request.
     */
    fbe_u32_t media_err_count;

    /*! Number of io_failure errors encountered for this request.
     */
    fbe_u32_t io_failure_err_count;

    /*! Number of io_failure errors encountered for this request.
     */
    fbe_u32_t invalid_request_err_count;

    /*! Time since the request was started.
     */
    fbe_u32_t elapsed_seconds;

    fbe_raid_verify_error_counts_t verify_errors;
}
fbe_rdgen_control_request_info_t;

/*!*******************************************************************
 * @struct fbe_rdgen_control_get_request_info_t
 *********************************************************************
 * @brief Returns information about requests.
 *********************************************************************/
typedef struct fbe_rdgen_control_get_request_info_s
{
    fbe_u32_t num_requests; /*!< Input number of requests that follow. */

    fbe_rdgen_control_request_info_t request_info_array[1]; /*!< Must be last. */
}
fbe_rdgen_control_get_request_info_t;

/*!*******************************************************************
 * @struct fbe_rdgen_control_object_info_t
 *********************************************************************
 * @brief
 *  Returns information about objects.
 *********************************************************************/
typedef struct fbe_rdgen_control_object_info_s
{
    /*! This is the handle to this object.
     */
    fbe_rdgen_handle_t object_handle;

    /*! This is the object we are testing.
     */
    fbe_object_id_t object_id;

    /*! This is package this object is in.
     */
    fbe_object_id_t package_id;

    /*! This is the count of active ts on this object.
     */
    fbe_u32_t active_ts_count;

    /*! This is the count of active requests on this object.
     */
    fbe_u32_t active_request_count;

    /*! This is the number of passes completed on this object 
     */
    fbe_u32_t num_passes;

    /*! This is the number of total I/Os completed on this object 
     */
    fbe_u32_t num_ios;

    /*! This is the object capacity.
     */
    fbe_lba_t capacity;

    /*! This is the object's stripe size.
     */
    fbe_block_count_t stripe_size;

    /*! Block size of the object.
     */
    fbe_block_size_t block_size;

    /*! The name of the object we are running to.
     */
    fbe_char_t driver_object_name[FBE_RDGEN_DEVICE_NAME_CHARS];
}
fbe_rdgen_control_object_info_t;

/*!*******************************************************************
 * @struct fbe_rdgen_control_get_object_info_t
 *********************************************************************
 * @brief Returns information about objects.
 *********************************************************************/
typedef struct fbe_rdgen_control_get_object_info_s
{
    fbe_u32_t num_objects; /*!< Input number of objects that follow. */

    fbe_rdgen_control_object_info_t object_info_array[1]; /*!< Must be last. */
}
fbe_rdgen_control_get_object_info_t;

/*!*******************************************************************
 * @struct fbe_rdgen_control_alloc_mem_info_t
 *********************************************************************
 * @brief Allocates memory.
 *********************************************************************/
typedef struct fbe_rdgen_control_alloc_mem_info_s
{
    fbe_u32_t mem_size_in_mb;
    fbe_rdgen_memory_type_t memory_type;
}
fbe_rdgen_control_alloc_mem_info_t;

/*!*******************************************************************
 * @struct fbe_rdgen_control_set_timeout_info_t
 *********************************************************************
 * @brief Allows us to set the timeouts for I/O.
 * 
 *********************************************************************/
typedef struct fbe_rdgen_control_set_timeout_info_s
{
    fbe_time_t default_io_timeout;
}
fbe_rdgen_control_set_timeout_info_t;

/*!*******************************************************************
 * @enum fbe_rdgen_svc_options_t
 *********************************************************************
 * @brief
 *  Options flags that can be used to configure the service.
 *********************************************************************/
enum fbe_rdgen_svc_options_e
{
    FBE_RDGEN_SVC_OPTIONS_INVALID = 0,
    /*! On an unpin and flush, hold it for manual restart via an unlock usurper. */
    FBE_RDGEN_SVC_OPTIONS_HOLD_UNPIN_FLUSH  = 0x00000001,   

    FBE_RDGEN_SVC_OPTIONS_NEXT              = 0x00000002,

};
typedef fbe_u64_t fbe_rdgen_svc_options_t;

/*!*******************************************************************
 * @struct fbe_rdgen_control_set_svc_options_t
 *********************************************************************
 * @brief Allows us to set the options.
 * 
 *********************************************************************/
typedef struct fbe_rdgen_control_set_svc_s
{
    fbe_rdgen_svc_options_t options;
}
fbe_rdgen_control_set_svc_options_t;

static __forceinline void fbe_rdgen_specification_options_set(fbe_rdgen_io_specification_t *spec_p,
                                                              fbe_rdgen_options_t flag)
{
    spec_p->options |= flag;
    return;
}

static __forceinline void fbe_rdgen_specification_options_clear(fbe_rdgen_io_specification_t *spec_p,
                                                                fbe_rdgen_options_t flag)
{
    spec_p->options &= ~flag;
    return;
}

static __forceinline fbe_rdgen_options_t fbe_rdgen_specification_options_get(fbe_rdgen_io_specification_t *spec_p)
{
    return spec_p->options;
}

static __forceinline fbe_bool_t fbe_rdgen_specification_options_is_set(fbe_rdgen_io_specification_t *spec_p,
                                                                       fbe_rdgen_options_t flags)
{
    return ((spec_p->options & flags) == flags);
}

static __forceinline void fbe_rdgen_specification_extra_options_set(fbe_rdgen_io_specification_t *spec_p,
                                                              fbe_rdgen_extra_options_t flag)
{
    spec_p->extra_options |= flag;
    return;
}

static __forceinline void fbe_rdgen_specification_extra_options_clear(fbe_rdgen_io_specification_t *spec_p,
                                                                fbe_rdgen_extra_options_t flag)
{
    spec_p->extra_options &= ~flag;
    return;
}

static __forceinline fbe_rdgen_extra_options_t fbe_rdgen_specification_extra_options_get(fbe_rdgen_io_specification_t *spec_p)
{
    return spec_p->extra_options;
}

static __forceinline fbe_bool_t fbe_rdgen_specification_extra_options_is_set(fbe_rdgen_io_specification_t *spec_p,
                                                                       fbe_rdgen_extra_options_t flags)
{
    return ((spec_p->extra_options & flags) == flags);
}

static __forceinline void fbe_rdgen_specification_data_pattern_flags_set(fbe_rdgen_io_specification_t *spec_p,
                                                           fbe_rdgen_data_pattern_flags_t flag)
{
    spec_p->data_pattern_flags |= flag;
    return;
}

static __forceinline void fbe_rdgen_specification_data_pattern_flags_clear(fbe_rdgen_io_specification_t *spec_p,
                                                                fbe_rdgen_data_pattern_flags_t flag)
{
    spec_p->data_pattern_flags &= ~flag;
    return;
}

static __forceinline fbe_rdgen_data_pattern_flags_t fbe_rdgen_data_pattern_flags_get(fbe_rdgen_io_specification_t *spec_p)
{
    return spec_p->data_pattern_flags;
}

static __forceinline fbe_bool_t fbe_rdgen_specification_data_pattern_flags_is_set(fbe_rdgen_io_specification_t *spec_p,
                                                                                  fbe_rdgen_data_pattern_flags_t flags)
{
    return ((spec_p->data_pattern_flags & flags) == flags);
}
static __forceinline void fbe_rdgen_specification_get_affinity(fbe_rdgen_io_specification_t *spec_p,
                                                               fbe_rdgen_affinity_t *affinity_p)
{
    *affinity_p = spec_p->affinity;
}
static __forceinline void fbe_rdgen_specification_get_core(fbe_rdgen_io_specification_t *spec_p,
                                                           fbe_u32_t *core_p)
{
    *core_p = spec_p->core;
}

fbe_status_t fbe_rdgen_peer_init(void);
fbe_status_t fbe_rdgen_peer_destroy(void);


/*!*******************************************************************
 * @struct fbe_rdgen_control_cmi_perf_t
 *********************************************************************
 * @brief runs cmi performance tests and returns the results. Goese with FBE_RDGEN_CONTROL_CODE_TEST_CMI_PERFORMANCE
 *********************************************************************/
typedef struct fbe_rdgen_control_cmi_perf_s
{
    fbe_u32_t number_of_messages; /*!< Input how many messages per thread.*/
	fbe_u32_t bytes_per_message; /*!< Input how many byte per message.*/
	fbe_u32_t thread_count;/*!< Input how many threads to run*/
	fbe_bool_t with_mask;/*!< 3rd argument is the cpu mask and not the number of cpus to run*/
	fbe_u32_t run_time_in_msec[FBE_CPU_ID_MAX];/*OUTPUT how long in msec did it take to send all meeasges(this is per core)*/
}
fbe_rdgen_control_cmi_perf_t;


/* function to set rdgen external queue location for use by rdgen to access system threads
 */
void fbe_rdgen_queue_set_external_handler(void (*handler) (fbe_queue_element_t *, fbe_cpu_id_t));


#endif /* FBE_RDGEN_H */

/*************************
 * end file fbe_rdgen.h
 *************************/
