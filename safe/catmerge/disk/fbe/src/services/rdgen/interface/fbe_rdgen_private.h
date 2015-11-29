#ifndef FBE_RDGEN_PRIVATE_H
#define FBE_RDGEN_PRIVATE_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_rdgen_private.h
 ***************************************************************************
 *
 * @brief
 *  This file contains local definitions for the rdgen service.
 *
 * @version
 *   5/28/2009:  Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "csx_ext.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_queue.h"
#include "fbe/fbe_memory.h"
#include "fbe/fbe_winddk.h"
#include "fbe_base_object.h"
#include "fbe_base_service.h"
#include "fbe_service_manager.h"
#include "fbe/fbe_rdgen.h"
#include "fbe_block_transport.h"
#include "fbe_rdgen_thread.h"
#include "fbe/fbe_sector.h"
#include "fbe/fbe_notification_interface.h"
#include "fbe/fbe_xor_api.h"
#include "fbe/fbe_random.h"
#include "fbe_rdgen_cmi.h"
#include "flare_ioctls.h"
#include "fbe/fbe_neit.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/
/*!*******************************************************************
 * @enum fbe_rdgen_constants_e
 *********************************************************************
 * @brief
 *
 *********************************************************************/
enum fbe_rdgen_constants_e
{
    /*! Max number of requests in the pool to keep track of incoming peer requests.
     */
    FBE_RDGEN_PEER_REQUEST_POOL_COUNT = 200,
    FBE_RDGEN_MAX_ABORT_RETRIES = 4,
    /*! Limit to some amount less than the total number of packets in neit.
     */
    FBE_RDGEN_MAX_REQUESTS = 200,
    FBE_RDGEN_MAX_PEER_REQUESTS = 200,
    FBE_RDGEN_SERVICE_TIME_MAX_MICROSEC = 5000000,
};
/*!*******************************************************************
 * @def FBE_RDGEN_MAX_SG_DATA_ELEMENTS
 *********************************************************************
 * @brief
 *   This is the largest sg we will allocate.
 *   We use 128, since historically this is what flare uses.
 *********************************************************************/
#define FBE_RDGEN_MAX_SG_DATA_ELEMENTS 2048

/*!*******************************************************************
 * @def FBE_RDGEN_MAX_SG
 *********************************************************************
 * @brief
 *   This is the largest sg we will allocate.
 *   This takes the terminator into account.
 *********************************************************************/
#define FBE_RDGEN_MAX_SG (FBE_RDGEN_MAX_SG_DATA_ELEMENTS+1)

/*!*******************************************************************
 * @def FBE_RDGEN_BUFFER_BLOCKS
 *********************************************************************
 * @brief
 *   This is the size of each buffer that we will allocate.
 *
 *********************************************************************/
#define FBE_RDGEN_MAX_POOL_BLOCKS 32

/***************************************************
 * FBE_RDGEN_MAX_BLOCKS
 ***************************************************
 * This is the upper limit to
 * an I/O we may generate.
 * This is limited by the max buffers we could
 * reference by an I/O request.
 ***************************************************/
#define FBE_RDGEN_MAX_BLOCKS (FBE_RDGEN_MAX_POOL_BLOCKS * FBE_RDGEN_MAX_SG_DATA_ELEMENTS)

/*!**************************************************
 * @def FBE_RDGEN_ZERO_VERIFY_BLOCKS
 ***************************************************
 * @brief This is the number of blocks we read to verify
 * we zeroed correctly.
 ***************************************************/

#define FBE_RDGEN_ZERO_VERIFY_BLOCKS 512

/*!**************************************************
 * @def FBE_RDGEN_MAX_BLOCKS_PER_SG
 ***************************************************
 * @brief
 *   This is the number of blocks we will put per sg
 *   that we generate.
 ***************************************************/
#define FBE_RDGEN_MAX_BLOCKS_PER_SG 32

/*!**************************************************
 * @def FBE_RDGEN_MAP_MEMORY
 ***************************************************
 * @brief
 *   This is a placeholder in case we ever need to map/unmap.
 ***************************************************/
#define FBE_RDGEN_MAP_MEMORY(memory_p, byte_count)((void*)memory_p)

/*!**************************************************
 * @def FBE_RDGEN_UNMAP_MEMORY
 ***************************************************
 * @brief
 *   This is a placeholder in case we ever need to map/unmap.
 ***************************************************/
#define FBE_RDGEN_UNMAP_MEMORY(memory_p, byte_count)((void)0)

/*!**************************************************
 * @def FBE_RDGEN_VIRTUAL_ADDRESS_AVAIL
 ***************************************************
 * @brief
 *   This is a placeholder in case we ever need to map/unmap.
 *   For now everything is virtual.
 ***************************************************/
#define FBE_RDGEN_VIRTUAL_ADDRESS_AVAIL(sg_ptr)(FBE_TRUE)

/*!**************************************************
 * @def FBE_RDGEN_HEADER_PATTERN
 ***************************************************
 * @brief This pattern appears at the head of all
 * sectors generated.
 ***************************************************/
#define FBE_RDGEN_HEADER_PATTERN 0x3CC3


/*!**************************************************
 * @def FBE_RDGEN_BUILD_PATTERN_WORD
 ***************************************************
 * @brief Merge the supplied pattern header with the
 *        sp id
 ***************************************************/
#define FBE_RDGEN_BUILD_PATTERN_WORD(pattern, sp_id)  ((fbe_u64_t)(((fbe_u64_t)(FBE_U32_MAX & pattern) << 32) | (FBE_U32_MAX & sp_id))) 

/*!**************************************************
 * @def FBE_RDGEN_DEFAULT_STRIPE_SIZE
 ***************************************************
 * @brief The number of blocks to use as a
 *        stripe size when the object does not have a stripe size.
 ***************************************************/
#define FBE_RDGEN_DEFAULT_STRIPE_SIZE 1

/*!**************************************************
 * @def FBE_RDGEN_MAX_IO_MSECS
 ***************************************************
 * @brief Max time we will allow an I/O to pend before
 *        starting to complain with a critical error.
 ***************************************************/
#define FBE_RDGEN_MAX_IO_MSECS 60 * 1000

/*!*******************************************************************
 * @def FBE_RDGEN_MAX_TRACE_CHARS
 *********************************************************************
 * @brief Max number of chars we can buffer to trace.
 *
 *********************************************************************/
#define FBE_RDGEN_MAX_TRACE_CHARS 256

/*!*******************************************************************
 * @def RDGEN_COND
 *********************************************************************
 * @brief We define this as the default condition handler.
 *        We overload this so that in test code we can
 *        test when these conditions are false.
 * 
 *********************************************************************/
#define RDGEN_COND(m_cond) (m_cond)

/*!*******************************************************************
 * @def FBE_RDGEN_TS_TRACE_ERROR_PARAMS
 *********************************************************************
 * @brief Parameters we send when tracing an rdgen ts for error.
 *
 *********************************************************************/
#define FBE_RDGEN_TS_TRACE_ERROR_PARAMS FBE_TRACE_LEVEL_ERROR, __LINE__, __FUNCTION__

typedef struct fbe_rdgen_memory_stats_s
{
    fbe_atomic_t allocations;
    fbe_atomic_t frees;
    fbe_atomic_t allocated_bytes;
    fbe_atomic_t freed_bytes;
    fbe_atomic_32_t current_bytes;
    fbe_atomic_32_t max_bytes;
    fbe_atomic_t memory_exausted_count;
    fbe_atomic_t ts_allocated_count;
    fbe_atomic_t ts_freed_count;
    fbe_atomic_t req_allocated_count;
    fbe_atomic_t req_freed_count;
    fbe_atomic_t obj_allocated_count;
    fbe_atomic_t obj_freed_count;
    fbe_atomic_t sg_allocated_count;
    fbe_atomic_t sg_freed_count;
    fbe_atomic_t mem_allocated_count;
    fbe_atomic_t mem_freed_count;
    fbe_spinlock_t spin;
}
fbe_rdgen_memory_stats_t;


/*!*******************************************************************
 * @struct fbe_rdgen_peer_request_t
 *********************************************************************
 * @brief Defines the request size needed to process a peer request.
 *********************************************************************/
typedef struct fbe_rdgen_peer_request_s 
{
    fbe_packet_t packet; /*! Packet to send usurper request.*/
    fbe_u64_t magic;
    fbe_rdgen_cmi_message_t cmi_msg; /*!< CMI msg for ack. */
    fbe_queue_element_t queue_element;
    fbe_queue_element_t pool_queue_element;
}
fbe_rdgen_peer_request_t;


/*!*******************************************************************
 * @enum fbe_rdgen_service_flags_t
 *********************************************************************
 * @brief
 *  Set of flags for use by rdgen service.
 *
 *********************************************************************/
typedef enum fbe_rdgen_service_flags_e
{
    FBE_RDGEN_SERVICE_FLAGS_NONE = 0,
    FBE_RDGEN_SERVICE_FLAGS_DESTROY      = 0x0001, /*!< Service is destroying.*/
    FBE_RDGEN_SERVICE_FLAGS_DRAIN_IO     = 0x0002, /*!< Service is draining.*/
    FBE_RDGEN_SERVICE_FLAGS_PEER_ALIVE   = 0x0004, /*!< Peer alive and well. */
    FBE_RDGEN_SERVICE_FLAGS_PEER_INIT    = 0x0008, /*!< Peer response to init needed. */
    FBE_RDGEN_SERVICE_FLAGS_DPS_MEM_INITTED  = 0x0010, /*!< DPS Memory already initted */
    FBE_RDGEN_SERVICE_FLAGS_LAST         = 0x0010,
}
fbe_rdgen_service_flags_t;


/*!*******************************************************************
 * @struct fbe_rdgen_service_t
 *********************************************************************
 * @brief
 *  This is the definition of the rdgen service, which is used to
 *  generate I/O.
 *
 *********************************************************************/
typedef struct fbe_rdgen_service_s
{
    fbe_base_service_t base_service;

    /*! Lock to protect our queues.
     */
    fbe_spinlock_t lock;

    /*! This is the list of fbe_rdgen_object_t structures, 
     * which track all rdgen activity to a given object. 
     */
    fbe_queue_head_t active_object_head;

    /*! This is the list of active request structures, 
     * Which tracks all user generated activity. 
     */
    fbe_queue_head_t active_request_head;

    /*! list of fbe_rdgen_peer_request_t structures 
     *  used to kick off peer requests on this SP. 
     */
    fbe_queue_head_t peer_request_pool_head;
    fbe_queue_head_t peer_request_active_pool_head;

    fbe_rdgen_peer_request_t peer_request_pool[FBE_RDGEN_PEER_REQUEST_POOL_COUNT];

   /* This is the list of packets outstanding to the peer.
    * If the peer dies we process this and complete the packets. 
    */ 
    fbe_queue_head_t peer_outstanding_packets;

   /* This is the list of ts waiting to get sent to the peer.
    * If the peer dies we process this and complete the packets.
    */ 
    fbe_queue_head_t peer_wait_ts;
    /*! Number of active objects.
     */
    fbe_u32_t num_objects;

    /*! Number of active requests. 
     */
    fbe_u32_t num_requests;

    /*! Number of active rdgen ts. 
     */
    fbe_u32_t num_ts;

    /*! Number of active threads. 
     */
    fbe_u32_t num_threads;

    /*! Number of passes after which to trace. 
     * If set to 0, we do not trace per pass. 
     */
    fbe_u32_t num_passes_per_trace;

    /*! Number of ios after which to trace. 
     *  If set to 0, we do not trace per io. 
     */
    fbe_u32_t num_ios_per_trace;

    /*! Num seconds to allow pass before tracing again.
     */
    fbe_u32_t num_seconds_per_trace;

    /*! Last time we traced.
     */
    fbe_time_t last_trace_time;

    /*! Threads initializing. 
     * Prevents multiple new requests from initializing the threads. 
     * Whoever wins in setting this to TRUE will kick off all the threads. 
     * Everyone else waits until threads initializing is cleared. 
     */
    fbe_bool_t b_threads_initializing;

    /*! Generic flags for rdgen.
     */
    fbe_rdgen_service_flags_t flags;

    /*! If I/O takes more than this number of milliseconds we will complain 
     *  with a critical error and abort the I/O.
     */
    fbe_time_t max_io_msec;

    /*! Last time we scanned our queues for hung I/Os.
     */
    fbe_time_t last_scan_time;

    /*! Array of rdgen threads.  We allocate this dynamically depending on how many cores there are.
     */
    fbe_rdgen_thread_t *thread_pool;

    /*! Thread to abort I/Os.
     */
    fbe_rdgen_thread_t abort_thread;

    /*! Thread for object I/O.
     */
    fbe_rdgen_thread_t object_thread;

    /*! Thread to scan for I/Os that are taking too long.
     */
    fbe_rdgen_thread_t scan_thread;
    /*! Thread to handle peer requests.
     */
    fbe_rdgen_thread_t peer_thread;

    fbe_rdgen_memory_stats_t memory_statistics;
    fbe_u32_t mem_size_in_mb; /*!< Total size of memory allocated in MB. */
    fbe_rdgen_memory_type_t memory_type; /*< Kind of memory we allocated. */

    /*! These are statistics that get incremented every time a request is completed.  
     * This keeps track of the historical statistics over time and can be reset. 
     */
    fbe_rdgen_io_statistics_t historical_stats;

    fbe_u32_t num_outstanding_peer_req; /*!< Number of requests outstanding to peer.  */

    fbe_u32_t random_context; /*! Used to seed all our random numbers. */

    fbe_u32_t peer_out_of_resource_count; /*!< Peer out of resources count */
    fbe_rdgen_svc_options_t service_options; /*! Global options.*/
}
fbe_rdgen_service_t;

/*!*******************************************************************
 * @struct fbe_rdgen_root_file_header_t
 *********************************************************************
 * @brief Header for the main file used for playback.
 *
 *********************************************************************/
#pragma pack(1)
typedef struct fbe_rdgen_root_file_header_s
{
    fbe_u32_t num_targets; /*!< Number of objects/devices to run to. */
    fbe_u32_t num_threads; /*!< Total number of threads per object/device to start. */
}
fbe_rdgen_root_file_header_t;
#pragma pack()

/*!*******************************************************************
 * @struct fbe_rdgen_root_file_filter_t
 *********************************************************************
 * @brief The filter stored inside the root file.
 *        This tells us the identity of the targets to run to.
 *
 *********************************************************************/
#pragma pack(1)
typedef struct fbe_rdgen_root_file_filter_s
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

    /*! This must always be set and is the package we will run I/O against.
     */
    fbe_package_id_t package_id;

    /*! FBE_TRUE to use driver (IRP/DCALL) interface rather than packet interface.
     */
    fbe_char_t device_name[FBE_RDGEN_DEVICE_NAME_CHARS];

    /*! Number of threads we will start max per this target.
     */
    fbe_u32_t threads;
}
fbe_rdgen_root_file_filter_t;
#pragma pack()

/*!*******************************************************************
 * @struct fbe_rdgen_object_file_header_t
 *********************************************************************
 * @brief During playback there is one file per object.
 *        This is the top of that file.
 *
 *********************************************************************/
#pragma pack(1)
typedef struct fbe_rdgen_object_file_header_s
{
    fbe_rdgen_root_file_filter_t filter; /*!< Confirmation of the object identity. */
    fbe_u32_t records; /*!< Number of records listed below. */
    fbe_u32_t threads; /*!< Number of threads to start. */
}
fbe_rdgen_object_file_header_t;
#pragma pack()
/*!*******************************************************************
 * @struct fbe_rdgen_object_file_record_t
 *********************************************************************
 * @brief This is the record that identifies a single I/O to issue.
 *
 *********************************************************************/
#pragma pack(1)
typedef struct fbe_rdgen_object_file_record_s
{
    fbe_rdgen_operation_t opcode;
    fbe_cpu_id_t cpu;
    fbe_lba_t lba;
    fbe_block_count_t blocks;
    fbe_u32_t flags;
    fbe_u32_t threads; /*!< Number of threads running when this starts. */
    fbe_u32_t msec_to_ramp; /*!< number of milliseconds to ramp up. */
    fbe_u32_t priority;
    fbe_u32_t io_interface; /*!< Interface to be used for this I/O. */
    fbe_u32_t reserved_1;
    fbe_u32_t reserved_2;
}
fbe_rdgen_object_file_record_t;
#pragma pack()

enum {
    /*! Number of records that will fit in one 64 block buffer */ 
    FBE_RDGEN_PLAYBACK_RECORDS_PER_BUFFER = ( (512*64) / sizeof(fbe_rdgen_object_file_record_t)),
    /*! Number of buffers to allocate at once. */
    FBE_RDGEN_PLAYBACK_CHUNK_COUNT = 2, 
};

/*!*******************************************************************
 * @struct fbe_rdgen_object_record_t
 *********************************************************************
 * @brief This is the structure we use for queuing
 *        records that we read off the disk.
 *
 *********************************************************************/
typedef struct fbe_rdgen_object_record_s
{
    fbe_queue_element_t queue_element;
    fbe_u32_t num_valid_records; /*!< Number of valid records. */
    fbe_rdgen_object_file_record_t record_data[FBE_RDGEN_PLAYBACK_RECORDS_PER_BUFFER];
}
fbe_rdgen_object_record_t;

/*!*******************************************************************
 * @enum fbe_rdgen_object_flags_t
 *********************************************************************
 * @brief
 *  This is the set of flags used in the ts.
 *
 *********************************************************************/
typedef enum fbe_rdgen_object_flags_e
{
    FBE_RDGEN_OBJECT_FLAGS_INVALID          = 0x00000000,   /*!< No special flags */
    FBE_RDGEN_OBJECT_FLAGS_QUEUED           = 0x00000001,   /*!< The object is at the thread. */
    FBE_RDGEN_OBJECT_FLAGS_LAST
}
fbe_rdgen_object_flags_t;
/*!*******************************************************************
 * @struct fbe_rdgen_object_t
 *********************************************************************
 * @brief
 *  This is the definition of the rdgen object, which is used to track
 *  all I/O to a given object.
 *
 *********************************************************************/
typedef struct fbe_rdgen_object_s
{
    /*! We make this first so it is easy to enqueue, dequeue since 
     * we can simply cast the queue element to an object. 
     */
    fbe_queue_element_t queue_element;
    fbe_u64_t magic;

    /*! Used for enqueuing to the thread.
     */
    fbe_queue_element_t thread_queue_element;

    /*! Used by the thread when it is processing the items.
     */
    fbe_queue_element_t process_queue_element;

    /*! This is the list of fbe_rdgen_request_t structures, 
     * which tracks each active request to this object. 
     */
    fbe_queue_head_t active_request_head;

    /*! This is the list of fbe_rdgen_ts_t structures, 
     * which tracks each active thread to this object. 
     */
    fbe_queue_head_t active_ts_head;

    /*! This is the spinlock to protect the rdgen object queues.
     */
    fbe_spinlock_t lock;

    /*! This is the object we are testing.
     */
    fbe_object_id_t object_id;

    /*! This is package this object is in.
     */
    fbe_object_id_t package_id;

    /* Irp requests can be sent to this device name.
     */
    fbe_char_t device_name[FBE_RDGEN_DEVICE_NAME_CHARS];

    PEMCPAL_DEVICE_OBJECT device_p; /*!< Driver object for irp requests. */
    PEMCPAL_FILE_OBJECT file_p;     /*!< File object for irp requests. */
    fbe_u8_t stack_size;            /*!< Number of stack entries for irp requests. */

    /*! This is the count of active ts on this object.
     */
    fbe_u32_t active_ts_count;

    /*! Edge to use for sending I/O. 
     * Needed since we do not have an edge to send I/O on in all cases. 
     */
    fbe_block_edge_t * edge_ptr;

     /*! Mostly TRUE, unless we connect to an existing edge, in which case, we don't want to destroy it
     */
    fbe_bool_t destroy_edge_ptr;

    /*! This is the count of active requests on this object.
     */
    fbe_u32_t active_request_count;

    /*! This is the number of passes completed on this object 
     */
    fbe_u32_t num_passes;

    /*! This is the number of requests at the peer. 
     */
    fbe_u32_t num_peer_requests;

    /*! This is the number of total I/Os completed on this object 
     */
    fbe_u32_t num_ios;

    /*! This is the object capacity.
     */
    fbe_lba_t capacity;

    /*! This is the object's stripe size.
     */
    fbe_u32_t stripe_size;

    /*! Block size of the object.
     */
    fbe_block_size_t block_size;
    /*! Block size of the physical object.
     */
    fbe_block_size_t physical_block_size;
    /*! Size requred for aligning writes.
     */
    fbe_block_size_t optimum_block_size;

    /*! Object type 
     */
    fbe_topology_object_type_t topology_object_type;

    /*! Set if the SEP object bene destroyed
     */
    fbe_bool_t  b_is_object_under_test_destroyed;

    fbe_rdgen_object_record_t *playback_records_p;
    fbe_u32_t playback_record_index;
    fbe_rdgen_object_record_t *playback_records_memory_p;
    fbe_u32_t playback_chunk_index;
    fbe_u32_t playback_total_chunks;
    fbe_u32_t playback_total_records;
    fbe_u32_t playback_num_chunks_processed;
    fbe_char_t file_name[FBE_RDGEN_DEVICE_NAME_CHARS];
    fbe_queue_head_t valid_record_queue;
    fbe_queue_head_t free_record_queue;
    fbe_rdgen_object_flags_t flags;
    fbe_u32_t current_threads;
    fbe_u32_t max_threads;
    fbe_u32_t restart_msec;
    fbe_time_t time_to_restart;

    fbe_u64_t random_context; /*! Seed for generating object random numbers. */
}
fbe_rdgen_object_t;

/*!*******************************************************************
 * @struct fbe_rdgen_request_t
 *********************************************************************
 * @brief
 *  This is the definition of the rdgen request,
 *  which is used to track all threads that are associated with a given user request.
 *
 *********************************************************************/
typedef struct fbe_rdgen_request_s
{
    /*! We make this first so it is easy to enqueue, dequeue since 
     * we can simply cast the queue element to an object. 
     */
    fbe_queue_element_t queue_element;
    fbe_u64_t magic;

    /*! This is the list of fbe_rdgen_ts_t structures, 
     * which tracks each active thread to this object. 
     */
    fbe_queue_head_t active_ts_head;

    /*! This is the spinlock to protect the rdgen object queues.
     */
    fbe_spinlock_t lock;

    /*! This is the count of active ts on this object.
     */
    fbe_u32_t active_ts_count;

    /*! This is the packet that originated this request.
     */
    fbe_packet_t *packet_p;

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
    fbe_u32_t congested_err_count;
    fbe_u32_t still_congested_err_count;

    /*! Number of io_failure errors encountered for this request.
     */
    fbe_u32_t invalid_request_err_count;

    /*! Number of blocks that contained invalidated blocks on an errored read.
     */
    fbe_u32_t inv_blocks_count;

    /*! Number of blocks that contained bad crc's during a read.
     */
    fbe_u32_t bad_crc_blocks_count;
    /*! This is the number of lock conflicts this request had.. 
     */
    fbe_u32_t lock_conflict_count;

    /*! This is the last rdgen ts status that we saved.
     */
    fbe_rdgen_status_t status;

    /*! When we have a caterpillar request, the lba in  
     *  the request keeps track of where each caterpillar is. 
     */
    fbe_lba_t caterpillar_lba;

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

    /*! Time the request was started.
     */
    fbe_time_t start_time;

    /*! Verify error info
     */
    fbe_raid_verify_error_counts_t verify_errors;
}
fbe_rdgen_request_t;

/*!*******************************************************************
 * @enum fbe_rdgen_ts_state_status_t
 *********************************************************************
 * @brief
 *  This is the common definition of return codes for all rdgen
 *  states.
 *
 *********************************************************************/
typedef enum fbe_rdgen_ts_state_status_e
{
    FBE_RDGEN_TS_STATE_STATUS_EXECUTING = 0,
    FBE_RDGEN_TS_STATE_STATUS_WAITING = 1,
    FBE_RDGEN_TS_STATE_STATUS_DONE = 2,
    FBE_RDGEN_TS_STATE_STATUS_SLEEPING = 3
}
fbe_rdgen_ts_state_status_t;

/* Common state for all rdgen ts functions.
 */
struct fbe_rdgen_ts_s;
typedef fbe_rdgen_ts_state_status_t(*fbe_rdgen_ts_state_t_local) (struct fbe_rdgen_ts_s *);

/*!*******************************************************************
 * @enum fbe_rdgen_ts_flags_t
 *********************************************************************
 * @brief
 *  This is the set of flags used in the ts.
 *
 *********************************************************************/
typedef enum fbe_rdgen_ts_flags_e
{
    FBE_RDGEN_TS_FLAGS_INVALID          = 0x00000000,   /*!< No special flags */
    FBE_RDGEN_TS_FLAGS_FIRST_REQUEST    = 0x00000001,   /*!< This is the sfirst time this request has been processed. */
    FBE_RDGEN_TS_FLAGS_SKIP_COMPARE     = 0x00000002,   /*!< When uncorrectables are injected we don't want to execute the data compare. */
    FBE_RDGEN_TS_FLAGS_WAITING_LOCK     = 0x00000004, /*!< The TS is waiting for the lock. */
    FBE_RDGEN_TS_FLAGS_LOCK_OBTAINED    = 0x00000008, /*!< The lock has been obtained by the ts. */
    FBE_RDGEN_TS_FLAGS_RESET_STATS      = 0x00000010, /*!< This TS should self reset its stats. */
    FBE_RDGEN_TS_FLAGS_WAIT_OBJECT      = 0x00000020, /*!< Wait for object to kick us off. */
    FBE_RDGEN_TS_FLAGS_WAIT_UNPIN       = 0x00000040, /*!< Wait for user to kick us off. */

    FBE_RDGEN_TS_FLAGS_LAST
}
fbe_rdgen_ts_flags_t;

/*!*******************************************************************
 * @def fbe_rdgen_io_stamp_t
 *********************************************************************
 * @brief
 *  This is a globally unique per I/O (aka packet) identifier.
 *
 *********************************************************************/
typedef fbe_u64_t fbe_rdgen_io_stamp_t;

typedef struct fbe_rdgen_disk_transport_s
{
    FLARE_SGL_INFO sgl_info;
    FLARE_ZERO_FILL_INFO zero_fill_info;
    EMCPAL_IRP irp;
}
fbe_rdgen_disk_transport_t;
/*!*******************************************************************
 * @struct fbe_rdgen_ts_t
 *********************************************************************
 * @brief
 *  This is the definition of the rdgen tracking structure,
 *  which is used to track a single thread of I/O to a given object.
 *
 *********************************************************************/
typedef struct fbe_rdgen_ts_s
{
    /*! This is for enqueuing to the fbe_rdgen_object. 
     * We make this first so it is easy to enqueue, dequeue since we can simply cast the 
     * queue element to a ts. 
     */  
    fbe_queue_element_t queue_element;
    fbe_u64_t magic;

    fbe_memory_request_t memory_request; /*!< Needed to allocate memory asynchronously. */
    void *memory_p; /*! Some requests like irp mj use contiguous memory. */
    fbe_rdgen_memory_type_t memory_type; /*!< Type of memory for the above memory_p. */

    /*! This is for enqueuing our ts to an fbe_rdgen_thread for execution.
     */
    fbe_queue_element_t thread_queue_element;

    /*! This is for enqueuing our ts to a fbe_rdgen_request.
     */
    fbe_queue_element_t request_queue_element;

    /*! This is the pointer to our object.
     */
    fbe_rdgen_object_t *object_p;
    fbe_spinlock_t spin;

    /*! This is the pointer to the request that originated this thread.
     */
    fbe_rdgen_request_t *request_p;

    /*! This is the next state to execute.
     */
    fbe_rdgen_ts_state_t_local state;

    /*! This is the state to return to for cases where one state machine  
     *  needs to invoke another.  
     */
    fbe_rdgen_ts_state_t_local calling_state;

    /*! Flags field.
     */
    fbe_rdgen_ts_flags_t flags;

    /*! set to TRUE if we are aborted.
     */
    fbe_bool_t b_aborted;

    /*! Last time we received a response. 
     * We use this to determine when this has taken too long. 
     */
    fbe_time_t last_send_time;

    fbe_time_t last_response_time;/*!< Last response took this long. */
    fbe_time_t max_response_time; /*!< Max of all responses. */
    /*! Every time we increment io count we also increment this. 
     * cumulative_response_time / total_responses gives you avg response time. 
     */
    fbe_time_t cumulative_response_time;
    fbe_u64_t total_responses; /*!< Number of responses that have gone into cumulative. */

    /*! Time when this thread started.
     */
    fbe_time_t start_time;

    /*! This is the number of this ts on this object.
     */
    fbe_u32_t thread_num;

    /*! This is the packet used to issue read requests.
     */
    union {
        fbe_packet_t packet;
        fbe_rdgen_disk_transport_t disk_transport;
    } read_transport;

    /*! This is the packet used to issue the write.
     */
    union {
        fbe_packet_t packet;
        fbe_rdgen_disk_transport_t disk_transport;
    } write_transport;
    fbe_packet_t *packet_p; /*!< Currently being used packet. */
    fbe_lba_t lba;
    fbe_block_count_t blocks;
    fbe_lba_t current_lba; /*!< Current lba in the overall lba range. */
    fbe_block_count_t current_blocks; /*!< Current block count. */
    fbe_block_count_t blocks_remaining;
    fbe_u32_t random_context; /*! context that is used to generate random numbers.  */
    fbe_u64_t random_mask; /*! Used to mask off bits when generating a random number.  */
    fbe_block_size_t block_size;

    fbe_lba_t pre_read_lba; /*!< Pre-read for write lba. */
    fbe_block_count_t pre_read_blocks; /*!< Pre-read for write blocks. */
    fbe_payload_pre_read_descriptor_t pre_read_descriptor; /*!< Descriptor for write pre-read. */

    fbe_u32_t breakup_count;  /*!< Number of I/Os to divide this into. */

    fbe_u32_t err_count;
    fbe_u32_t abort_err_count;
    fbe_u32_t media_err_count;
    fbe_u32_t congested_err_count;
    fbe_u32_t still_congested_err_count;
    fbe_u32_t io_failure_err_count;
    fbe_u32_t invalid_request_err_count;
    fbe_u32_t inv_blocks_count;
    fbe_u32_t bad_crc_blocks_count;
    fbe_u32_t lock_conflict_count; /*!< Total number of times we failed to get a lock. */

    fbe_u32_t abort_retry_count; /*! Number of times we have retried an aborted I/O. */

    fbe_status_t status; /*!< Status of the rdgen ts. */

    /*! For debug.  Set to FALSE when we send a packet and TRUE when it completes. 
     * Should be useful for catching multiple completions. 
     */
    fbe_bool_t b_outstanding; 

    /*! This is used when seeding the pattern in a block.  
     * This seed typically does not go beyond 0xFF so we can fit it in 
     * the upper bits of an LBA. 
     */
    fbe_u32_t sequence_count;
    fbe_u32_t core_num; /*!< The core this ts should run on.*/

    /*! This is the number of total I/O passes we have performed.  
     * This does not account for the I/Os that occur due to a breakup. 
     * And if multiple I/Os are needed for a pass (write/read for example), 
     * then this only gets incremented once. 
     */
    fbe_u32_t io_count;

    /*! This is used to determine when we are done with a thread or not. 
     */
    fbe_u32_t pass_count;

    /*! Passes completed by peer for this thread.
     */
    fbe_u32_t peer_pass_count;

    /*! Num io completed by peer for this thread.
     */
    fbe_u32_t peer_io_count;

    fbe_u32_t partial_io_count;
    fbe_sg_element_t *sg_ptr;
    fbe_sg_element_t *pre_read_sg_ptr;
    fbe_u32_t memory_size; /*! Number of bytes allocated.*/

    /*! This is the last packet status that we saved.
     */
    fbe_rdgen_status_t last_packet_status;

    /*! Verify error info.
    */
    fbe_raid_verify_error_counts_t verify_errors;

    fbe_u64_t corrupted_blocks_bitmap; /*!< Bitmap for corrupted blocks pattern
                                        * when doing corrrupt crc or corrupt
                                        * data..
                                        */
    /*! this is used to generate time to abort the I/Os.
     */
    fbe_time_t time_to_abort;

    /*! We need memory for the message in order to send a msg and 
     * receive a response. 
     */
    fbe_rdgen_cmi_message_t cmi_msg;
    fbe_bool_t b_send_to_peer; /*!< TRUE if we are going to peer only. */

    /*! This is a per I/O stamp (for debug) 
     */
    fbe_rdgen_io_stamp_t    io_stamp;

    /*! Determines if we do local write and peer compare or some combination.
     */
    fbe_rdgen_peer_options_t current_peer_options;

    fbe_u32_t region_index; /* Current index in region structure.*/

    /* The reason we need the below values is to decouple the ts from the request. 
     * This helps us in the case of a playback, which needs these values on a per thread 
     * not a per-request basis. 
     * Typically we just set these from the request except in the case of the 
     * playback which generates these per request. 
     */
    fbe_rdgen_operation_t operation; /*!< Type of operation. */
    fbe_rdgen_interface_type_t io_interface; /*!< I/O interface to use, packet, irp dca, irp mj, irp sgl, etc. */
    fbe_packet_priority_t priority; /*!< Tells us which priority to issue this I/O at. */
    fbe_payload_block_operation_opcode_t block_opcode; /*!< Tells us the current operation code. */

    /* This is needed for IOs to block device */
    fbe_s32_t fp;
    fbe_rdgen_thread_t bdev_thread;
}
fbe_rdgen_ts_t;
typedef fbe_rdgen_ts_state_status_t(*fbe_rdgen_ts_state_t) (fbe_rdgen_ts_t *);

/*!*******************************************************************
 * @struct fbe_rdgen_memory_info_t
 *********************************************************************
 * @brief This struct contains all the info rdgen needs to keep
 *        track of memory allocations.
 * 
 * @note  See accessors below
 *
 *********************************************************************/
typedef struct fbe_rdgen_memory_info_s
{
    /*! This is our current memory page we are allocating from.
     */
    fbe_memory_header_t *memory_header_p;

    /*! Bytes left in the current memory page.
     */
    fbe_u32_t bytes_remaining;

    /*! Bytes in each memory chunk we are allocating.
     */
    fbe_u32_t page_size_bytes;
}
fbe_rdgen_memory_info_t;

fbe_rdgen_ts_t * 
fbe_rdgen_ts_thread_queue_element_to_ts_ptr(fbe_queue_element_t * thread_queue_element_p);

/*!*******************************************************************
 * @struct fbe_rdgen_notification_context_t
 *********************************************************************
 * @brief
 *  This is the definition of the rdgen notification context
 *
 *********************************************************************/
typedef struct fbe_rdgen_notification_context_s
{
    fbe_bool_t                  b_is_initialized;
    fbe_notification_element_t  notification_element;

} fbe_rdgen_notification_context_t;

/* fbe_rdgen_main.c
 */
void fbe_rdgen_service_trace(fbe_trace_level_t trace_level,
                         fbe_trace_message_id_t message_id,
                         const char * fmt, ...)
                         __attribute__((format(__printf_func__,3,4)));
fbe_bool_t fbe_rdgen_service_allow_trace(fbe_trace_level_t trace_level);
fbe_status_t rdgen_service_send_rdgen_ts_to_thread(fbe_queue_element_t *queue_element_p,
                                                   fbe_u32_t thread_number);
fbe_status_t rdgen_service_send_rdgen_object_to_thread(fbe_rdgen_object_t *object_p);
void fbe_rdgen_complete_packet(fbe_packet_t *packet_p,
                               fbe_payload_control_status_t control_payload_status,
                               fbe_status_t packet_status);
void fbe_rdgen_scan_queues(void);
void fbe_rdgen_scan_for_abort(fbe_time_t *max_wait_time_p);

/* fbe_rdgen_memory.c
 */
fbe_status_t fbe_rdgen_memory_initialize(void);
fbe_status_t fbe_rdgen_cmm_initialize(void);
fbe_status_t fbe_rdgen_cmm_destroy(void);
fbe_status_t fbe_rdgen_memory_destroy(void);
void * fbe_rdgen_allocate_memory(fbe_u32_t allocation_size_in_bytes);
void fbe_rdgen_free_memory(void * ptr, fbe_u32_t size);
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_allocate_memory(fbe_rdgen_ts_t *ts_p);
void fbe_rdgen_ts_free_memory(fbe_rdgen_ts_t *ts_p);
void * fbe_rdgen_memory_allocate_next_buffer(fbe_rdgen_memory_info_t *info_p,
                                             fbe_u32_t bytes_to_allocate,
                                             fbe_u32_t *bytes_allocated_p,
                                             fbe_bool_t b_allocate_contiguous);
fbe_status_t fbe_rdgen_ts_get_chunk_size(fbe_rdgen_ts_t *ts_p, fbe_u32_t *chunk_size_p);
fbe_status_t fbe_rdgen_memory_info_init(fbe_rdgen_memory_info_t *info_p,
                                        fbe_memory_request_t *memory_request_p,
                                        fbe_u32_t bytes_per_page);
fbe_status_t fbe_rdgen_allocate_request_memory(fbe_packet_t *packet_p, fbe_u32_t num_ts);

/* fbe_rdgen_object.c
 */
void fbe_rdgen_object_init(fbe_rdgen_object_t *object_p,
                           fbe_object_id_t object_id,
                           fbe_package_id_t package_id,
                           fbe_char_t *name_p,
                           fbe_rdgen_control_start_io_t *start_p);
fbe_status_t fbe_rdgen_object_initialize_capacity(fbe_rdgen_object_t *object_p);
fbe_status_t fbe_rdgen_object_init_bdev(fbe_rdgen_object_t *object_p);
fbe_status_t fbe_rdgen_object_open_bdev(int *fp, csx_cstring_t bdev);
fbe_status_t fbe_rdgen_object_close_bdev(int *fp);
void fbe_rdgen_object_stop_io(fbe_rdgen_object_t *object_p);
void fbe_rdgen_object_destroy(fbe_rdgen_object_t *object_p);
fbe_bool_t fbe_rdgen_object_is_lun_destroy_registered(void);
fbe_status_t fbe_rdgen_object_register_lun_destroy_notifications(void);
fbe_status_t fbe_rdgen_object_unregister_lun_destroy_notifications(void);
void fbe_rdgen_object_set_file(fbe_rdgen_object_t *object_p, fbe_char_t *file_p);
fbe_rdgen_object_t * fbe_rdgen_object_thread_queue_element_to_ts_ptr(fbe_queue_element_t * thread_queue_element_p);
fbe_rdgen_object_t * fbe_rdgen_object_process_queue_element_to_ts_ptr(fbe_queue_element_t * thread_queue_element_p);
fbe_status_t fbe_rdgen_object_enqueue_to_thread(fbe_rdgen_object_t *object_p);

/* fbe_rdgen_user_main.c and fbe_rdgen_kernel_main.c
 */
fbe_status_t fbe_rdgen_object_size_disk_object(fbe_rdgen_object_t *object_p);
fbe_status_t fbe_rdgen_ts_send_disk_device(fbe_rdgen_ts_t *ts_p,
                                           fbe_payload_block_operation_opcode_t opcode,
                                           fbe_lba_t lba,
                                           fbe_block_count_t blocks,
                                           fbe_sg_element_t *sg_ptr,
                                           fbe_payload_block_operation_flags_t payload_flags);
fbe_status_t fbe_rdgen_object_close(fbe_rdgen_object_t *object_p);
void * fbe_rdgen_alloc_cache_mem(fbe_u32_t bytes);
void fbe_rdgen_release_cache_mem(void * ptr);
fbe_rdgen_object_t *fbe_rdgen_find_object(fbe_object_id_t object_id,
                                          fbe_char_t *name_p);

/* fbe_rdgen_request.c
 */
void fbe_rdgen_request_init(fbe_rdgen_request_t *request_p,
                            fbe_packet_t *packet_p,
                            fbe_rdgen_control_start_io_t *start_p);
void fbe_rdgen_request_stop_io(fbe_rdgen_request_t *request_p);
void fbe_rdgen_request_stop_for_bdev(fbe_rdgen_request_t *request_p);
void fbe_rdgen_request_destroy(fbe_rdgen_request_t *request_p);
void fbe_rdgen_request_finished(fbe_rdgen_request_t *request_p);
void fbe_rdgen_request_check_finished(fbe_rdgen_request_t *request_p);
void fbe_rdgen_request_save_status(fbe_rdgen_request_t *request_p,
                                   fbe_rdgen_status_t *status_p);

void fbe_rdgen_request_update_verify_errors(fbe_rdgen_request_t *request_p,
                                            fbe_raid_verify_error_counts_t *in_verify_errors_p);

void fbe_rdgen_request_save_ts_io_statistics(fbe_rdgen_request_t *request_p,
                                             fbe_u32_t io_count,
                                             fbe_time_t elapsed_msec,
                                             fbe_object_id_t object_id);
/* fbe_rdgen_ts.c
 */
void fbe_rdgen_ts_init(fbe_rdgen_ts_t *ts_p,    
                       fbe_rdgen_object_t *object_p,
                       fbe_rdgen_request_t *request_p);
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_start(fbe_rdgen_ts_t *ts_p);
void fbe_rdgen_ts_state(fbe_rdgen_ts_t *ts_p);
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_finished(fbe_rdgen_ts_t *ts_p);
fbe_rdgen_ts_state_t fbe_rdgen_ts_get_state_for_opcode(fbe_rdgen_operation_t opcode,
                                                       fbe_rdgen_interface_type_t io_interface);
void fbe_rdgen_ts_set_complete_status(fbe_rdgen_ts_t *ts_p);
void fbe_rdgen_build_packet(fbe_packet_t * const packet_p,
                            fbe_package_id_t destination_package_id,
                            fbe_payload_block_operation_opcode_t opcode,
                            fbe_lba_t lba,
                            fbe_block_count_t blocks,
                            fbe_block_size_t block_size,
                            fbe_block_size_t optimal_block_size,
                            fbe_sg_element_t * const sg_p,
                            fbe_raid_verify_error_counts_t *verify_errors_p,
                            fbe_time_t msec_to_expiration,
                            fbe_packet_io_stamp_t io_stamp,
                            fbe_packet_completion_context_t context,
                            fbe_payload_pre_read_descriptor_t *pre_read_desc_p,
                            fbe_payload_block_operation_flags_t payload_flags);
fbe_status_t fbe_rdgen_ts_send_packet(fbe_rdgen_ts_t *ts_p,
                              fbe_payload_block_operation_opcode_t opcode,
                              fbe_lba_t lba,
                              fbe_block_count_t blocks,
                              fbe_sg_element_t *sg_ptr,
                              fbe_payload_block_operation_flags_t payload_flags);
fbe_status_t fbe_rdgen_ts_set_time_to_abort(fbe_rdgen_ts_t *ts_p);
void fbe_rdgen_ts_save_packet_status(fbe_rdgen_ts_t *ts_p);
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_handle_error(fbe_rdgen_ts_t *ts_p,
                                                      fbe_rdgen_ts_state_t_local retry_state);
fbe_bool_t fbe_rdgen_ts_is_complete(fbe_rdgen_ts_t *ts_p);
fbe_status_t fbe_rdgen_ts_enqueue_to_thread(fbe_rdgen_ts_t *ts_p);

void fbe_rdgen_ts_adjust_aligned_size(fbe_rdgen_ts_t *ts_p, fbe_u32_t aligned_size);
fbe_bool_t fbe_rdgen_ts_generate(fbe_rdgen_ts_t *ts_p);
void fbe_rdgen_ts_gen_sequential_increasing(fbe_rdgen_ts_t * ts_p);
void fbe_rdgen_ts_gen_sequential_decreasing(fbe_rdgen_ts_t * ts_p);
void fbe_rdgen_ts_gen_catepillar_increasing(fbe_rdgen_ts_t * ts_p);
void fbe_rdgen_ts_gen_catepillar_decreasing(fbe_rdgen_ts_t * ts_p);
void fbe_rdgen_ts_get_breakup_count(fbe_rdgen_ts_t * ts_p);
fbe_bool_t fbe_rdgen_ts_resolve_conflict(fbe_rdgen_ts_t * ts_p);
fbe_bool_t fbe_rdgen_ts_is_time_to_trace(fbe_rdgen_ts_t *ts_p);
fbe_bool_t fbe_rden_ts_fixed_lba_conflict_check(fbe_rdgen_ts_t *ts_p);
void fbe_rdgen_ts_initialize_blocks_remaining(fbe_rdgen_ts_t *ts_p);
void fbe_rdgen_ts_get_current_blocks(fbe_rdgen_ts_t *ts_p);
fbe_status_t fbe_rdgen_ts_update_blocks_remaining(fbe_rdgen_ts_t *ts_p);
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_handle_peer_error(fbe_rdgen_ts_t *ts_p);
void fbe_rdgen_ts_lock(fbe_rdgen_ts_t *ts_p);
void fbe_rdgen_ts_unlock(fbe_rdgen_ts_t *ts_p);
fbe_bool_t fbe_rdgen_ts_is_playback(fbe_rdgen_ts_t *ts_p);
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_generate_playback(fbe_rdgen_ts_t *ts_p);
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_set_first_state(fbe_rdgen_ts_t *ts_p,
                                                         fbe_rdgen_ts_state_t state);
void fbe_rdgen_ts_init_random(fbe_rdgen_ts_t *ts_p);
void fbe_rdgen_ts_save_context(fbe_rdgen_ts_t *ts_p);

/* fbe_rdgen_sg.c
 */
fbe_status_t fbe_rdgen_ts_setup_sgs(fbe_rdgen_ts_t *ts_p, fbe_block_count_t bytes);
fbe_bool_t fbe_rdgen_check_sg_list(fbe_rdgen_ts_t *ts_p,
                                   fbe_rdgen_pattern_t pattern,
                                   fbe_bool_t b_panic);
fbe_bool_t fbe_rdgen_fill_sg_list(fbe_rdgen_ts_t *ts_p,
                                  fbe_sg_element_t * sg_ptr,
                                  fbe_rdgen_pattern_t pattern,
                                  fbe_bool_t b_append_checksum,
                                  fbe_bool_t b_read_op);
fbe_status_t fbe_rdgen_sg_count_invalidated_and_bad_crc_blocks(fbe_rdgen_ts_t *ts_p);
fbe_status_t fbe_rdgen_memory_info_apply_offset(fbe_rdgen_memory_info_t *info_p,
                                                fbe_block_count_t dest_bytecnt,
                                                fbe_u32_t block_size,
                                                fbe_u32_t *sgl_count_p);
/* fbe_rdgen_memory.c
 */
void * fbe_rdgen_cmm_memory_allocate(fbe_u32_t allocation_size_in_bytes);
void fbe_rdgen_cmm_memory_release(void * ptr);
fbe_status_t fbe_rdgen_release_memory(void);


/* fbe_rdgen_cmi.c
 */
fbe_status_t fbe_rdgen_cmi_init(void);
fbe_status_t fbe_rdgen_cmi_destroy(void);
fbe_data_pattern_sp_id_t fbe_rdgen_cmi_get_sp_id(void);

/* fbe_rdgen_playback
 */
fbe_status_t fbe_rdgen_playback_get_header(fbe_u32_t *targets_p,
                                           fbe_rdgen_root_file_filter_t **filter_p, fbe_char_t *file_p);
fbe_bool_t fbe_rdgen_playback_generate_wait(fbe_rdgen_ts_t *ts_p);
void fbe_rdgen_playback_handle_objects(fbe_rdgen_thread_t *thread_p,
                                       fbe_time_t *min_msecs_to_wait_p);

/* fbe_rdgen_peer.c
 */
fbe_status_t fbe_rdgen_peer_init_sp_id(void);
void fbe_rdgen_peer_thread_process_queue(fbe_rdgen_thread_t *thread_p);
fbe_bool_t fbe_rdgen_ts_send_to_peer(fbe_rdgen_ts_t *ts_p);
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_should_send_to_peer(fbe_rdgen_ts_t *ts_p);
fbe_status_t fbe_rdgen_peer_died_restart_packets(void);
fbe_status_t fbe_rdgen_peer_thread_process_peer_init(void);
fbe_status_t fbe_rdgen_peer_send_peer_alive_msg(void);

/* fbe_rdgen_read_only.c
 */
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_read_only_state0(fbe_rdgen_ts_t *ts_p);
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_read_only_state1(fbe_rdgen_ts_t *ts_p);
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_read_only_state2(fbe_rdgen_ts_t *ts_p);
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_read_only_state3(fbe_rdgen_ts_t *ts_p);
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_read_only_state4(fbe_rdgen_ts_t *ts_p);

/* fbe_rdgen_read_buffer.c
 */
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_read_buffer_state0(fbe_rdgen_ts_t *ts_p);
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_read_buffer_state1(fbe_rdgen_ts_t *ts_p);
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_read_buffer_state2(fbe_rdgen_ts_t *ts_p);
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_read_buffer_state3(fbe_rdgen_ts_t *ts_p);
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_read_buffer_state4(fbe_rdgen_ts_t *ts_p);
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_read_buffer_state5(fbe_rdgen_ts_t *ts_p);
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_read_buffer_state6(fbe_rdgen_ts_t *ts_p);
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_read_buffer_state7(fbe_rdgen_ts_t *ts_p);

/* fbe_rdgen_write_only.c
 */
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_write_only_state0(fbe_rdgen_ts_t *ts_p);
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_write_only_state1(fbe_rdgen_ts_t *ts_p);
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_write_only_state2(fbe_rdgen_ts_t *ts_p);
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_write_only_state3(fbe_rdgen_ts_t *ts_p);
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_write_only_state4(fbe_rdgen_ts_t *ts_p);
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_write_only_state5(fbe_rdgen_ts_t *ts_p);

/* fbe_rdgen_write_read_compare.c
 */
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_write_read_compare_state0(fbe_rdgen_ts_t *ts_p);
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_write_read_compare_state1(fbe_rdgen_ts_t *ts_p);
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_write_read_compare_state2(fbe_rdgen_ts_t *ts_p);
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_write_read_compare_state3(fbe_rdgen_ts_t *ts_p);
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_write_read_compare_state10(fbe_rdgen_ts_t *ts_p);
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_write_read_compare_state11(fbe_rdgen_ts_t *ts_p);
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_write_read_compare_state12(fbe_rdgen_ts_t *ts_p);
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_write_read_compare_state13(fbe_rdgen_ts_t *ts_p);
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_write_read_compare_state20(fbe_rdgen_ts_t *ts_p);
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_write_read_compare_state21(fbe_rdgen_ts_t *ts_p);
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_write_read_compare_state22(fbe_rdgen_ts_t *ts_p);
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_write_read_compare_state23(fbe_rdgen_ts_t *ts_p);
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_write_read_compare_state24(fbe_rdgen_ts_t *ts_p);

/* fbe_rdgen_read_compare.c
 */
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_read_compare_state0(fbe_rdgen_ts_t *ts_p);
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_read_compare_state1(fbe_rdgen_ts_t *ts_p);
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_read_compare_state2(fbe_rdgen_ts_t *ts_p);
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_read_compare_state3(fbe_rdgen_ts_t *ts_p);
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_read_compare_state10(fbe_rdgen_ts_t *ts_p);
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_read_compare_state11(fbe_rdgen_ts_t *ts_p);
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_read_compare_state12(fbe_rdgen_ts_t *ts_p);
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_read_compare_state13(fbe_rdgen_ts_t *ts_p);

/* fbe_rdgen_verify_only.c
 */
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_verify_only_state0(fbe_rdgen_ts_t *ts_p);
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_verify_only_state1(fbe_rdgen_ts_t *ts_p);
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_verify_only_state2(fbe_rdgen_ts_t *ts_p);
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_verify_only_state3(fbe_rdgen_ts_t *ts_p);
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_verify_only_state4(fbe_rdgen_ts_t *ts_p);

/* fbe_rdgen_read.c
 */
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_read_state0(fbe_rdgen_ts_t *ts_p);
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_read_state1(fbe_rdgen_ts_t *ts_p);
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_read_state2(fbe_rdgen_ts_t *ts_p);
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_read_state3(fbe_rdgen_ts_t *ts_p);
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_read_state4(fbe_rdgen_ts_t *ts_p);

/* fbe_rdgen_write.c
 */
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_write_state0(fbe_rdgen_ts_t *ts_p);
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_write_state1(fbe_rdgen_ts_t *ts_p);
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_write_state2(fbe_rdgen_ts_t *ts_p);
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_write_state3(fbe_rdgen_ts_t *ts_p);
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_write_state4(fbe_rdgen_ts_t *ts_p);
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_write_state5(fbe_rdgen_ts_t *ts_p);

/* fbe_rdgen_verify.c
 */
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_verify_state0(fbe_rdgen_ts_t *ts_p);
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_verify_state1(fbe_rdgen_ts_t *ts_p);
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_verify_state2(fbe_rdgen_ts_t *ts_p);
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_verify_state3(fbe_rdgen_ts_t *ts_p);
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_verify_state4(fbe_rdgen_ts_t *ts_p);

/* fbe_rdgen_zero.c
 */
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_zero_state0(fbe_rdgen_ts_t *ts_p);
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_zero_state1(fbe_rdgen_ts_t *ts_p);
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_zero_state2(fbe_rdgen_ts_t *ts_p);
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_zero_state3(fbe_rdgen_ts_t *ts_p);
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_zero_state4(fbe_rdgen_ts_t *ts_p);

/* fbe_rdgen_dca_rw.c
 */
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_dca_rw_state0(fbe_rdgen_ts_t *ts_p);
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_dca_rw_state1(fbe_rdgen_ts_t *ts_p);
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_dca_rw_state2(fbe_rdgen_ts_t *ts_p);
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_dca_rw_state3(fbe_rdgen_ts_t *ts_p);
fbe_rdgen_ts_state_status_t fbe_rdgen_ts_dca_rw_state4(fbe_rdgen_ts_t *ts_p);

/*fbe_rdgen_cmi_perf.c
*/
fbe_status_t fbe_rdgen_service_test_cmi_performance(fbe_packet_t * packet);
/*************************
 * end file fbe_rdgen_private.h
 *************************/
#endif /* end FBE_RDGEN_PRIVATE_H */
