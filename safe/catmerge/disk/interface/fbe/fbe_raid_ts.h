#ifndef FBE_RAID_TS_H
#define FBE_RAID_TS_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_raid_ts.h
 ***************************************************************************
 *
 * @brief
 *  This file contains raid tracking structure definitions.
 *
 * @version
 *   5/14/2009:  Created. rfoley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_sg_element.h"
#include "fbe/fbe_queue.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_memory.h"
#include "fbe/fbe_atomic.h"
#include "fbe_raid_common.h"
#include "fbe_raid_group_metadata.h"
#include "fbe_xor_api.h"


/*! @def FBE_RAID_MAX_DISK_ARRAY_WIDTH
 *  @brief This is the maximum # of positions we allow in a raid group.
  */
#define FBE_RAID_MAX_DISK_ARRAY_WIDTH FBE_XOR_MAX_FRUS

/*!*******************************************************************
 * @enum fbe_raid_sg_index_t
 *********************************************************************
 * @brief Defines all possible scatter gather type index values.
 *        Typically used to index into an array of counts of sg types.
 *
 *********************************************************************/
typedef enum fbe_raid_sg_index_e
{
    FBE_RAID_SG_INDEX_SG_1   = 0,
    FBE_RAID_SG_INDEX_SG_8   = 1,
    FBE_RAID_SG_INDEX_SG_32  = 2,
    FBE_RAID_SG_INDEX_SG_128 = 3,
    FBE_RAID_SG_INDEX_SG_MAX = 4,
    FBE_RAID_SG_INDEX_MAX,
    FBE_RAID_SG_INDEX_UNKNOWN = 0x1111,
    FBE_RAID_SG_INDEX_INVALID = 0xFFFF,
}
fbe_raid_sg_index_t;

/*! @def FBE_RAID_MAX_SG_TYPE
 *  @brief This is the maximum # of sg types we support
  */
#define FBE_RAID_MAX_SG_TYPE   FBE_RAID_SG_INDEX_MAX

/*! @def FBE_RAID_SIOTS_MAX_STACK
 *  @brief This is the size of the history
 *         in the siots.
  */
#define FBE_RAID_SIOTS_MAX_STACK        6



/* Fake these out for now until we port over the structures.
 */
typedef void *    fbe_memory_id_t;

/*!*******************************************************************
 * @struct fbe_raid_vrts_t
 *********************************************************************
 * @brief
 ******************************************************************
 *
 * RAID Driver Verify tracking Structure
 *
 ******************************************************************
 * We contain an XOR eboard in this structure because
 * we need to keep track of all the errors that have
 * occurred as a result of strip mining this SIOTS.
 *
 * The SIOTS may need to issue multiple requests to XOR
 * over the life of the SIOTS.
 *
 * Each XOR operation only reports back info
 * local to that operation, and the cxts
 * can only store information for
 * each individual XOR operation.
 *
 * Thus, a global overall structure is required to store
 * information on errors encountered over the life of
 * the SIOTS.
 *
 * Once the SIOTS is done it logs the accumulated errors
 * into the event log. RF 1/2005.
 *
 ******************************************************************/
typedef struct fbe_raid_vrts_s
{
    fbe_xor_error_t         eboard;

    /* This field keeps track of any position that
     * is undergoing a crc error retry.
     */
    fbe_u16_t               current_crc_retry_bitmask;

    /* This field keeps track of any position that
     * successfully retried a read that took a crc error.
     */
    fbe_u16_t               retried_crc_bitmask;

} 
fbe_raid_vrts_t;
/***************************************************
 * VR TS macro definitions.
 ***************************************************/

/*!*****************************************************************
 * @struct fbe_raid_vcts_t
 ****************************************************************** 
 * @brief 
 * RAID Driver Verify Counts Tracking Structure (RAID_VC_TS) 
 * curr_pass_eboard - This is the count structure for the
 *                    last xor driver verify.  This will
 *                    get rolled into the overall_eboard when
 *                    the state machine calls rg_record_errors()
 *
 * overall_eboard - This eboard contains the error board results
 *                  for this siots.  This eboard gets updated
 *                  when the state machine calls rg_record_errors()
 *                  These results get copied into a passed in
 *                  eboard (if available) in rg_report_errors.
 *
 * error_regions - The set of error regions that we pass to the XOR driver.
 *
 ******************************************************************/

typedef struct fbe_raid_vcts_s
{
    fbe_raid_verify_error_counts_t curr_pass_eboard;
    fbe_raid_verify_error_counts_t overall_eboard;
    fbe_xor_error_regions_t error_regions;
}
fbe_raid_vcts_t;


typedef struct fbe_raid_timestamp_s
{
    fbe_u64_t ticks;
} fbe_raid_timestamp_t;


struct fbe_packet_s;
typedef void * fbe_raid_iots_completion_context_t;
typedef fbe_status_t (* fbe_raid_iots_completion_function_t) (void * packet, fbe_raid_iots_completion_context_t context);

/*!*******************************************************************
 * @def FBE_RAID_IOTS_MAX_CHUNKS
 *********************************************************************
 * @brief Max number of chunks an iots will operate on.
 *        This number is fairly arbitrary.
 *        We picked this number to be much larger than most I/Os we
 *        will receive, so that performance path will not need to
 *        break up (serialize) I/os. This combined with the chunk size
 *        will limit the number of blocks any given iots can operate on.
 *
 *********************************************************************/
#define FBE_RAID_IOTS_MAX_CHUNKS 10


/*!*******************************************************************
 * @enum fbe_raid_iots_status_t
 *********************************************************************
 * @brief This determines the type of action that the caller of the
 *        library is expected to take when the iots calls back.
 *
 *********************************************************************/
typedef enum fbe_raid_iots_status_e
{
    FBE_RAID_IOTS_STATUS_INVALID = 0,
    FBE_RAID_IOTS_STATUS_COMPLETE,
    FBE_RAID_IOTS_STATUS_UPGRADE_LOCK,
    FBE_RAID_IOTS_STATUS_NOT_STARTED_TO_LIBRARY, /*!< Queued to termination queue, but not at library.  */
    FBE_RAID_IOTS_STATUS_NOT_USED,
    FBE_RAID_IOTS_STATUS_AT_LIBRARY, /*! outstanding to library. */
    FBE_RAID_IOTS_STATUS_LIBRARY_WAITING_FOR_CONTINUE, /*! outstanding to library. */
    FBE_RAID_IOTS_STATUS_WAITING_FOR_QUIESCE,
    FBE_RAID_IOTS_STATUS_LAST,
}
fbe_raid_iots_status_t;

/*!*************************************************
 * @typedef  fbe_raid_iots_flags_t
 ***************************************************
 * @brief These are the flags which are specific
 *        to the iots which describe state of the iots.
 *        IMPORTANT !!
 *        When you add or change algorithms,
 *        you need to update fbe_raid_debug_iots_flags_trace_info
 *
 ***************************************************/
typedef enum fbe_raid_iots_flags_e
{
    FBE_RAID_IOTS_FLAG_NONE                         = 0, /*!< No flags are set. */
    FBE_RAID_IOTS_FLAG_ERROR                        = 0x00000001,    /*! Reads get single threaded on errors */
    FBE_RAID_IOTS_FLAG_GENERATING                   = 0x00000002,
    FBE_RAID_IOTS_FLAG_STATUS_SENT                  = 0x00000004,

    /*! This was previously quiesced.  This allows the  
     * raid group to wait until these quiesced requests have finished. 
     */
    FBE_RAID_IOTS_FLAG_WAS_QUIESCED                 = 0x00000008,
    FBE_RAID_IOTS_FLAG_LOCK_GRANTED                 = 0x00000010,
    FBE_RAID_IOTS_FLAG_WAIT_LOCK                    = 0x00000020,
    FBE_RAID_IOTS_FLAG_GENERATE_DONE                = 0x00000040,
    FBE_RAID_IOTS_FLAG_RAID_LIBRARY_TEST            = 0x00000080,
    /* We are in the process of allocating a new    
     * siots, which means we may be
     * enqueued to the state queue OR
     * we are waiting on a resource from the BM.
     */
    FBE_RAID_IOTS_FLAG_ALLOCATING_SIOTS             = 0x00000100,
    /* A flag to indicate that a specific IOTS
     * still is generating a count.
     */
    FBE_RAID_IOTS_FLAG_GENERATING_SIOTS             = 0x00000200,
    /* This flag indicates that we have one or more siots
     * that is waiting for an upgrade.
     */
    FBE_RAID_IOTS_FLAG_UPGRADE_WAITERS              = 0x00000400,
    /*! Mark that the embedded siots is in use.
     */
    FBE_RAID_IOTS_FLAG_EMBEDDED_SIOTS_IN_USE        = 0x00000800,
    /*! We are aborting this request since the packet is aborted. 
     */
    FBE_RAID_IOTS_FLAG_ABORT                        = 0x00001000,
    /*! We are aborting this request since the raid group is not ready.
     */
    FBE_RAID_IOTS_FLAG_ABORT_FOR_SHUTDOWN           = 0x00002000,
    /*! This IOTS should quiesce.
     */
    FBE_RAID_IOTS_FLAG_QUIESCE                      = 0x00004000,
    /*! This iots should generate a remap.
     */
    FBE_RAID_IOTS_FLAG_REMAP_NEEDED                 = 0x00008000,
    /*! This iots should be restarted instead of starting siots.
     */
    FBE_RAID_IOTS_FLAG_RESTART_IOTS                 = 0x00010000,
    /*! Tells us that we are not yet degraded, so certain 
     * optimizations can be employed to not look at degraded state. 
     */
    FBE_RAID_IOTS_FLAG_NON_DEGRADED                 = 0x00020000,
    /*! The allocated siots have been consumed.
     */
    FBE_RAID_IOTS_FLAG_SIOTS_CONSUMED               = 0x00040000,
    /*! This is the last iots of the request.
     */
    FBE_RAID_IOTS_FLAG_LAST_IOTS_OF_REQUEST         = 0x00080000,
    /*! We have already checked that this I/O is a small write 
     * that hits just one element. 
     */
    FBE_RAID_IOTS_FLAG_FAST_WRITE                   = 0x00100000,
    /*! This flag is set only for monitor packets when it detects FRU dead or
     * waiting. For monitor packets doing shutdown continue leads to deadlock
     * hence this flag is set to indicate operation can be retried.
     */
    FBE_RAID_IOTS_FLAG_INCOMPLETE_WRITE             = 0x00200000,
    /*! This flag is set when write_log header read operation determines that
     * there is valid data in write_log slot that needs to be flushed.
     */
    FBE_RAID_IOTS_FLAG_WRITE_LOG_FLUSH_REQUIRED     = 0x00400000,
    FBE_RAID_IOTS_FLAG_CHUNK_INFO_INITIALIZED       = 0x00800000,
    FBE_RAID_IOTS_FLAG_REKEY_MARK_DEGRADED          = 0x01000000,

     /* IMPORTANT !!
      *  When you add or change flags,
      *  you need to update fbe_raid_debug_iots_flags_trace_info
      */
    FBE_RAID_IOTS_FLAG_LAST                         = 0x02000000,

}
fbe_raid_iots_flags_t;

/*!**********************************************************
 * @struct fbe_raid_iots_t
 * 
 * @brief
 *  This is the main tracking structure used in raid.
 *  The iots contains one or more siots to actually perform the I/O.
 *  The iots acts as the coordinator for the siots.
 *
 ******************************************************************/

typedef struct fbe_raid_iots_s
{
    fbe_raid_common_t common;

    struct fbe_packet_s * packet_p;        /* Packet for this operation. */
    void *raid_geometry_p;             /* Our raid geometry structure pointer */
    fbe_raid_iots_flags_t flags;
    fbe_raid_iots_completion_function_t callback; /*! Callback for library to use when it is done. */
    fbe_raid_iots_completion_context_t callback_context;   /*  Context for library to use when it is done. */

    /* Although the associated packet also has a status, this limit the packet access.
     */
    fbe_payload_block_operation_status_t error;

    /* This is the qualifier we should return for this iots.
     */
    fbe_payload_block_operation_qualifier_t qualifier; 

    fbe_raid_iots_status_t status; /*!< Status of this IOTS, determines action caller should take. */

    fbe_queue_head_t siots_queue;           /* Queue of all active siots for this iots. */

    fbe_queue_head_t available_siots_queue; /* Queue of all available siots for this iots. */

    fbe_memory_request_t    memory_request; /* The memory request for any allocations the iots must perform */

    /* This is required since the block operation might not be active. 
     * There can be other operations (like stripe lock or metadata) active 
     * so we need to keep a reference to the block operation. 
     */
    fbe_payload_block_operation_t * block_operation_p;
    fbe_lba_t packet_lba;            /*!< Lba from the packet. */
    fbe_block_count_t packet_blocks; /*!< blocks from the packet. */

    /*! Lba for the current operation.  Usually the same as lba/blocks  
     * unless the object is doing more than one operation to the library 
     * for each lba/block range. (Verify on demand or rebuild on demand) 
     */
    fbe_lba_t current_operation_lba;
    /*! Block count to go with the above lba.
     */
    fbe_block_count_t current_operation_blocks;
    /*! Opcode this iots is executing.  This will be different from the block operation's opcode 
     *  in cases where we need to do multiple operations such as a verify on-demand or rebuild on-demand. 
     */
    fbe_payload_block_operation_opcode_t current_opcode; 
    fbe_lba_t lba;                  /*! Start lba of this iots. */
    fbe_block_count_t blocks;       /*! Total blocks in this iots. */
    fbe_lba_t current_lba;       /* Next lba to generate a siots for.
                                  */
    fbe_u32_t outstanding_requests; /* Number of sub requests outstanding. */
    fbe_block_count_t blocks_remaining;   /* Number of blocks worth of sub requests generated. */
    fbe_block_count_t blocks_transferred; /* Number of blocks transferred. */
    fbe_time_t time_stamp;        /* Timing info for this IOTS.  It gets marked when the iots is started. */

    /*! This is the information on the chunks this iots is currently operating on. 
     */
    fbe_raid_group_paged_metadata_t chunk_info[FBE_RAID_IOTS_MAX_CHUNKS];
    fbe_lba_t start_chunk_lba; /*!< Start lba for the chunk info. */
    fbe_chunk_size_t chunk_size; /*!< size of each chunk. */

    /*! Bitmask of positions that are still rebuild logging.
     */
    fbe_raid_position_bitmask_t rebuild_logging_bitmask;

    fbe_raid_position_bitmask_t rekey_bitmask; /*! Positions in this chunk being rekeyed. Bit set means use new key. */

    /*! This is where the host buffer offset that is generated when we need
     * to split a host request into multiple iots (due to size).
     */
    fbe_block_count_t host_start_offset;

    fbe_u32_t journal_slot_id;

    /*! This flag will indicate whether the NP lock has been acquired or not.
        Currently this flag will only be set in case of background verify
     */
    fbe_bool_t    np_lock;

    //fbe_spinlock_t lock; /*! This protects access to the iots. */

    /*! Drives that were detected dead while processing this IOTS.
     */
    fbe_raid_position_bitmask_t dead_err_bitmap;

    /*! Ptr to the extent description.
     */
    void *extent_p;
    /*! Opaque context used in conjunction with extent description.
     */
    void *extent_context_p;
}
fbe_raid_iots_t;

/*!**************************************************
 * @struct fbe_raid_siots_geometry_t
 * @brief
 *   This is the structure the defines a snapshot of
 *   the geometry being used for a given request.
 * Assume each line is equivalent to 8 blocks.
 * Where the below unit is a raid 5 4+1 with
 * an element size of 64.  Thus there are 2
 * vertical elements per parity stripe.
 * The below I/O is 96 blocks total.
 *   -------- -------- -------- -------- -------- <---unit starts
 *  |        |        |        |        |        |     /|\
 *  .                                                   |
 *  .                                                   C
 *  .                                                   |
 *  .                                                  \|/
 *   -------- -------- -------- -------- -------- <-- Parity stripe starts 
 *  |        |        |  /|\   |ioioioio|ioioioio|     /|\
 *  |        |        |   |    |ioioioio|ioioioio|      |
 *  |   P    |        |   A    |ioioioio|        |      |
 *  |        |        |   |    |ioioioio|        |      |
 *  |   A    |        |   |    |ioioioio|        |      D
 *  |        |        |  \|/   |ioioioio|        |      |
 *  |   R    |        |ioioioio|ioioioio|        |      |
 *  |        |        |ioioioio|ioioioio|        | <----|- end of data element
 *  |   I    |        |        |        |        |      |
 *  |        |        |        |        |        |      |
 *  |   T    |        |        |        |        |      |
 *  |        |        |        |        |        |      |
 *  |   Y    |        |        |        |        |      |
 *  |        |        |        |        |        |      |
 *  |        |        |        |        |        |     \|/
 *  |________|________|________|________|________|<-- Parity stripe ends
 *                                              (128 blocks in parity stripe)
 *
 *    - start index                    - 2
 *    - width                          - 5
 *    - sectors per data element       - 64
 *    - max blocks                     - 400
 *    - blocks remaining in parity is  - 400
 *  A - start_offset_rel_parity_stripe - 48
 *  B - blocks_remaining_in_data       - 16
 *  C - logical parity start 
 *  D - logical parity count
 ****************************************************/
typedef struct fbe_raid_siots_geometry_s
{ 
    fbe_u32_t start_index;
    fbe_block_count_t start_offset_rel_parity_stripe;
    fbe_lba_t logical_parity_start;
    fbe_u8_t position[FBE_RAID_MAX_DISK_ARRAY_WIDTH];
    fbe_block_count_t blocks_remaining_in_parity;
    fbe_block_count_t blocks_remaining_in_data;
    fbe_block_count_t logical_parity_count;
    fbe_block_count_t max_blocks;
}
fbe_raid_siots_geometry_t;

/*!*******************************************************************
 * @struct fbe_raid_cxts_t
 *********************************************************************
 * @brief
 *  RAID Driver Checksum/XOR tracking Structure (RAID_CX_TS)
 *
 *********************************************************************/
typedef struct fbe_raid_cxts_s
{
    fbe_xor_error_t     eboard;
}
fbe_raid_cxts_t;

struct fbe_raid_vrts_s;
struct fbe_raid_siots_s;

/*!*************************************************
 * @typedef fbe_raid_siots_error_validation_callback_t
 ***************************************************
 * @brief Defines a callback that can be used
 *        to validate errors encountered are expected.
 *        Used by logical error injection to
 *        set a callback that the raid library will invoke. *
 *
 ***************************************************/
typedef fbe_status_t (*fbe_raid_siots_error_validation_callback_t)(struct fbe_raid_siots_s * const,
                                                                   fbe_xor_error_region_t **);

/* Raid SUB IO Tracking Structure.
 */
typedef struct fbe_raid_siots_s
{
    fbe_raid_common_t   common;

    fbe_memory_request_t memory_request; /* Will be used to allocate DPS memory */

    fbe_u32_t flags;
    fbe_atomic_t wait_count;         /* # of requests outstanding. */

    fbe_lba_t start_lba;          /* LBA of first block in request */
    fbe_block_count_t xfer_count;         /* Total size of current request */

    fbe_raid_position_t parity_pos;         /*! Relative position of parity. */
    fbe_raid_position_t start_pos;          /*! Relative position where request starts. */
    fbe_u32_t data_disks;         /*! # of frus involved in operation [data + parity] */

    fbe_raid_position_t dead_pos;           /* Dead fru Relative position(s). Used for degraded opeations. */

    fbe_raid_position_t dead_pos_2;         /* This is used by R1, R6 for tracking the second dead position. */
    

    /* Start of the entire parity range
     * hs, mirrors - Includes address offset.
     * R0, R5/R3, R6 - Does not include address offset.
     */
    fbe_lba_t parity_start;   
    fbe_block_count_t parity_count; /* Count of entire parity range. */

    /* This is the error value this siots is returning.
     */
    fbe_payload_block_operation_status_t error;

    /* This is the block_qualifier value this siots is returning.
     */
    fbe_payload_block_operation_qualifier_t qualifier;

    fbe_u32_t algorithm;

    /* Geometry Information - This is fixed for a request at generate/lock time.
     *                        Thus, we reference glut for geometry once.
     */
    fbe_raid_siots_geometry_t geo;

    fbe_u16_t               ctrl_page_size; /*!< Size in blocks of ctrl memory pages. */
    fbe_u16_t               data_page_size; /*!< Size in blocks of data memory pages. */

    /*! Number of blocks of buffers allocated.  This only includes memory allocated 
     *  for buffers and does not include memory for sgs, or tracking structures. 
     */
    fbe_block_count_t       total_blocks_to_allocate; 

    fbe_u16_t               num_data_pages; /*!< Number of data buffer pages we allocate. */
    fbe_u16_t               num_ctrl_pages; /*!< Number of control pages we allocate. */
    
    struct misc_ts_s
    {
        fbe_raid_cxts_t cxts;
        fbe_xor_status_t xor_status;
    }
    misc_ts;

    fbe_queue_head_t read_fruts_head;	/* Head of list of frus doing a read */
    fbe_queue_head_t read2_fruts_head;	/* Head of list of frus doing a read */
    fbe_queue_head_t write_fruts_head;	/* Head of list of frus doing a write */

    fbe_queue_head_t freed_fruts_head;	/*! Head of list of frus we are no longer using. */

    fbe_raid_vrts_t        *vrts_ptr;
    fbe_raid_vcts_t        *vcts_ptr;
    struct fbe_raid_siots_s *nested_siots_ptr;

    fbe_u32_t drive_operations;   /* Total drive operations in this request */

    /* Debug fields */
    fbe_raid_common_state_t prevState[FBE_RAID_SIOTS_MAX_STACK];
    fbe_u32_t prevStateStamp[FBE_RAID_SIOTS_MAX_STACK];
    fbe_u16_t state_index;

    /* Linked list of nest siots.
     */
    fbe_queue_head_t nsiots_q;

    /* Bitmask of positions that have received continues.
     */
    fbe_u16_t continue_bitmask;

    /* Bitmask of positions that need continues for this SIOTS.
     */
    fbe_u16_t needs_continue_bitmask;
    
    /* Degraded region info */
    fbe_lba_t degraded_start;
    fbe_block_count_t degraded_count;

    /*! @todo This is temporary to check for leaked handle due to fruts 
     *        not being destroyed properly.
     */
    fbe_u32_t               total_fruts_initialized;

    fbe_u32_t retry_count;        /* Total retries remaining. */
    fbe_u32_t media_error_count;  /* Total media errors hit so far. */
    fbe_lba_t media_error_lba; /*! Lba that is no good. */

    /* Timestamp for diagnosis
     */
    fbe_time_t time_stamp;
    
    /*! Spinlock to protect things like the wait_count, etc. */
    //fbe_spinlock_t lock;     

    /* Journal slot assigned to this request if a degraded write.
     */
    fbe_u32_t journal_slot_id;

	/* Queue element to pend Siots waiting for journal slot to free-up*/
	fbe_queue_element_t journal_q_elem;
    
    fbe_raid_siots_error_validation_callback_t error_validation_callback;
} 
fbe_raid_siots_t;

/*!*******************************************************************
 * @struct fbe_raid_fru_statistics_t
 *********************************************************************
 * @brief Fetch stats on rg.
 *
 *********************************************************************/
typedef struct fbe_raid_fru_statistics_s
{
    fbe_u32_t read_count;
    fbe_u32_t write_count;
    fbe_u32_t write_verify_count;
    fbe_u32_t zero_count;
    fbe_u32_t unknown_count;
    fbe_payload_block_operation_opcode_t unknown_opcode;
    fbe_u32_t parity_count;
}
fbe_raid_fru_statistics_t;

/*!*******************************************************************
 * @struct fbe_raid_library_statistics_t
 *********************************************************************
 * @brief Fetch stats on rg.
 *
 *********************************************************************/
typedef struct fbe_raid_statistics_s
{
    fbe_u32_t total_iots;
    fbe_u32_t total_siots;
    fbe_u32_t total_nest_siots;

    fbe_u32_t read_count;
    fbe_u32_t write_count;
    fbe_u32_t write_verify_count;
    fbe_u32_t verify_write_count;
    fbe_u32_t zero_count;
    fbe_u32_t unknown_count;
    fbe_payload_block_operation_opcode_t unknown_opcode;

    fbe_raid_fru_statistics_t total_fru_stats[FBE_RAID_MAX_DISK_ARRAY_WIDTH];
    fbe_raid_fru_statistics_t user_fru_stats[FBE_RAID_MAX_DISK_ARRAY_WIDTH];
    fbe_raid_fru_statistics_t metadata_fru_stats[FBE_RAID_MAX_DISK_ARRAY_WIDTH];
    fbe_raid_fru_statistics_t journal_fru_stats[FBE_RAID_MAX_DISK_ARRAY_WIDTH];
}
fbe_raid_library_statistics_t;

/*************************
 * end file fbe_raid_ts.h
 *************************/

#endif
