#ifndef FBE_RDGEN_PRIVATE_INLINES_H
#define FBE_RDGEN_PRIVATE_INLINES_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_rdgen_private_inlines.h
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
#include "fbe/fbe_xor_api.h"
#include "fbe/fbe_random.h"
#include "fbe_rdgen_cmi.h"
#include "flare_ioctls.h"
#include "fbe/fbe_neit.h"
#include "fbe_rdgen_private.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

static __forceinline void fbe_rdgen_peer_request_set_magic(fbe_rdgen_peer_request_t *request_p)
{
    request_p->magic = FBE_MAGIC_NUM_RDGEN_PEER_REQUEST;
    return;
}
static __forceinline fbe_bool_t fbe_rdgen_peer_request_validate_magic(fbe_rdgen_peer_request_t *peer_request_p)
{
    return (peer_request_p->magic == FBE_MAGIC_NUM_RDGEN_PEER_REQUEST);
}
static __inline fbe_rdgen_peer_request_t * 
fbe_rdgen_peer_request_queue_element_to_ptr(fbe_queue_element_t * queue_element_p)
{
    /* We're converting an address to a queue element into an address to the containing
     * rdgen ts. 
     * In order to do this we need to subtract the offset to the queue element from the 
     * address of the queue element. 
     */
    fbe_rdgen_peer_request_t * peer_request_p;
    peer_request_p = (fbe_rdgen_peer_request_t  *)((fbe_u8_t *)queue_element_p - 
                               (fbe_u8_t *)(&((fbe_rdgen_peer_request_t  *)0)->queue_element));
    return peer_request_p;
}

static __inline fbe_rdgen_peer_request_t * 
fbe_rdgen_peer_request_pool_queue_element_to_ptr(fbe_queue_element_t * queue_element_p)
{
    /* We're converting an address to a queue element into an address to the containing
     * rdgen ts. 
     * In order to do this we need to subtract the offset to the queue element from the 
     * address of the queue element. 
     */
    fbe_rdgen_peer_request_t * peer_request_p;
    peer_request_p = (fbe_rdgen_peer_request_t  *)((fbe_u8_t *)queue_element_p - 
                               (fbe_u8_t *)(&((fbe_rdgen_peer_request_t  *)0)->pool_queue_element));
    return peer_request_p;
}

fbe_rdgen_service_t *fbe_get_rdgen_service(void);

/* Rdgen service lock accessors.
 */
static __forceinline void fbe_rdgen_lock(void)
{
    fbe_rdgen_service_t *rdgen_p = fbe_get_rdgen_service();

    fbe_spinlock_lock(&rdgen_p->lock);
    return;
}
static __forceinline void fbe_rdgen_unlock(void)
{
    fbe_rdgen_service_t *rdgen_p = fbe_get_rdgen_service();

    fbe_spinlock_unlock(&rdgen_p->lock);
    return;
}

/* Accessor methods for the flags field.
 */
static __forceinline fbe_bool_t fbe_rdgen_service_is_flag_set(fbe_rdgen_service_flags_t flag)
{
    fbe_rdgen_service_t *rdgen_p = fbe_get_rdgen_service();
    return ((rdgen_p->flags & flag) == flag);
}
static __forceinline void fbe_rdgen_service_init_flags(void)
{
    fbe_rdgen_service_t *rdgen_p = fbe_get_rdgen_service();
    rdgen_p->flags = 0;
    return;
}
static __forceinline void fbe_rdgen_service_set_flag(fbe_rdgen_service_flags_t flag)
{
    fbe_rdgen_service_t *rdgen_p = fbe_get_rdgen_service();
    rdgen_p->flags |= flag;
}

static __forceinline void fbe_rdgen_service_clear_flag(fbe_rdgen_service_flags_t flag)
{
    fbe_rdgen_service_t *rdgen_p = fbe_get_rdgen_service();
    rdgen_p->flags &= ~flag;
}
static __forceinline void fbe_rdgen_get_peer_thread(fbe_rdgen_thread_t **thread_p)
{
    fbe_rdgen_service_t *rdgen_p = fbe_get_rdgen_service();
    *thread_p = &rdgen_p->peer_thread;
    return;
}

static __forceinline fbe_status_t fbe_rdgen_get_io_thread(fbe_rdgen_thread_t **thread_p,
                                      fbe_u32_t thread_num)
{
    fbe_rdgen_service_t *rdgen_p = fbe_get_rdgen_service();
    if (rdgen_p->thread_pool == NULL)
    {
        *thread_p = NULL;
        return FBE_STATUS_GENERIC_FAILURE;
    }
    *thread_p = &rdgen_p->thread_pool[thread_num];
    return FBE_STATUS_OK;
}
static __forceinline fbe_status_t fbe_rdgen_get_object_thread(fbe_rdgen_thread_t **thread_p)
{
    fbe_rdgen_service_t *rdgen_p = fbe_get_rdgen_service();
    *thread_p = &rdgen_p->object_thread;
    return FBE_STATUS_OK;
}
static __forceinline void fbe_rdgen_setup_thread_pool(fbe_rdgen_thread_t *thread_p)
{
    fbe_rdgen_service_t *rdgen_p = fbe_get_rdgen_service();
    rdgen_p->thread_pool = thread_p;
    return;
}
static __forceinline fbe_u32_t fbe_rdgen_get_threads_initializing(void)
{
    fbe_rdgen_service_t *rdgen_p = fbe_get_rdgen_service();
    return rdgen_p->b_threads_initializing;
}
static __forceinline void fbe_rdgen_set_threads_initializing(fbe_bool_t b_initializing)
{
    fbe_rdgen_service_t *rdgen_p = fbe_get_rdgen_service();
    rdgen_p->b_threads_initializing = b_initializing;
    return;
}

static void fbe_rdgen_service_clear_io_statistics(void)
{
    fbe_rdgen_service_t *rdgen_p = fbe_get_rdgen_service();
    fbe_zero_memory(&rdgen_p->historical_stats, sizeof(fbe_rdgen_io_statistics_t));
}

static __forceinline void fbe_rdgen_inc_requests_from_peer(void)
{
    fbe_rdgen_service_t *rdgen_p = fbe_get_rdgen_service();
    fbe_atomic_32_increment(&rdgen_p->historical_stats.requests_from_peer_count);
    return;
}
static __forceinline void fbe_rdgen_inc_requests_to_peer(void)
{
    fbe_rdgen_service_t *rdgen_p = fbe_get_rdgen_service();
    fbe_atomic_32_increment(&rdgen_p->historical_stats.requests_to_peer_count);
    return;
}
static __forceinline void fbe_rdgen_inc_peer_out_of_resource(void)
{
    fbe_rdgen_service_t *rdgen_p = fbe_get_rdgen_service();
    fbe_atomic_32_increment(&rdgen_p->historical_stats.peer_out_of_resource_count);
    return;
}
static __forceinline fbe_rdgen_io_statistics_t* fbe_rdgen_get_historical_stats(void)
{
    fbe_rdgen_service_t *rdgen_p = fbe_get_rdgen_service();
    return &rdgen_p->historical_stats;
}

/* Accessors for the number of outstanding peer requests.
 */
static __inline void fbe_rdgen_inc_num_outstanding_peer_req(void)
{
    fbe_rdgen_service_t *rdgen_p = fbe_get_rdgen_service();
    rdgen_p->num_outstanding_peer_req++;
    return;
}
static __inline void fbe_rdgen_dec_num_outstanding_peer_req(void)
{
    fbe_rdgen_service_t *rdgen_p = fbe_get_rdgen_service();
    rdgen_p->num_outstanding_peer_req--;
    return;
}
static __inline fbe_u32_t fbe_rdgen_get_num_outstanding_peer_req(void)
{
    fbe_rdgen_service_t *rdgen_p = fbe_get_rdgen_service();
    return rdgen_p->num_outstanding_peer_req;
}
/* Accessors for the number of requests.
 */
static __forceinline void fbe_rdgen_inc_num_requests(void)
{
    fbe_rdgen_service_t *rdgen_p = fbe_get_rdgen_service();
    rdgen_p->num_requests++;
    return;
}
static __forceinline void fbe_rdgen_dec_num_requests(void)
{
    fbe_rdgen_service_t *rdgen_p = fbe_get_rdgen_service();
    rdgen_p->num_requests--;
    return;
}
static __forceinline fbe_u32_t fbe_rdgen_get_num_requests(void)
{
    fbe_rdgen_service_t *rdgen_p = fbe_get_rdgen_service();
    return rdgen_p->num_requests;
}

/* Accessors to how many passes to trace after.
 */
static __forceinline void fbe_rdgen_set_trace_per_pass_count(fbe_u32_t passes)
{
    fbe_rdgen_service_t *rdgen_p = fbe_get_rdgen_service();
    rdgen_p->num_passes_per_trace = passes;
    return;
}
static __forceinline fbe_u32_t fbe_rdgen_get_trace_per_pass_count(void)
{
    fbe_rdgen_service_t *rdgen_p = fbe_get_rdgen_service();
    return rdgen_p->num_passes_per_trace;
}

/* Accessor to memory size and type.
 */
static __forceinline void fbe_rdgen_set_memory_size(fbe_u32_t size_in_mb)
{
    fbe_rdgen_service_t *rdgen_p = fbe_get_rdgen_service();
    rdgen_p->mem_size_in_mb = size_in_mb;
    return;
}
static __forceinline fbe_u32_t fbe_rdgen_get_memory_size(void)
{
    fbe_rdgen_service_t *rdgen_p = fbe_get_rdgen_service();
    return rdgen_p->mem_size_in_mb;
}

/* Accessor to memory size and type.
 */
static __forceinline void fbe_rdgen_set_memory_type(fbe_rdgen_memory_type_t memory_type)
{
    fbe_rdgen_service_t *rdgen_p = fbe_get_rdgen_service();
    rdgen_p->memory_type = memory_type;
    return;
}
static __forceinline fbe_rdgen_memory_type_t fbe_rdgen_get_memory_type(void)
{
    fbe_rdgen_service_t *rdgen_p = fbe_get_rdgen_service();
    return rdgen_p->memory_type;
}

/* Accessors to how many ios to trace after.
 */
static __forceinline void fbe_rdgen_set_trace_per_io_count(fbe_u32_t passes)
{
    fbe_rdgen_service_t *rdgen_p = fbe_get_rdgen_service();
    rdgen_p->num_ios_per_trace = passes;
    return;
}
static __forceinline fbe_u32_t fbe_rdgen_get_trace_per_io_count(void)
{
    fbe_rdgen_service_t *rdgen_p = fbe_get_rdgen_service();
    return rdgen_p->num_ios_per_trace;
}

/* Accessors to how much time to trace after.
 */
static __forceinline void fbe_rdgen_set_seconds_per_trace(fbe_u32_t seconds)
{
    fbe_rdgen_service_t *rdgen_p = fbe_get_rdgen_service();
    rdgen_p->num_seconds_per_trace = seconds;
    return;
}
static __forceinline fbe_u32_t fbe_rdgen_get_seconds_per_trace(void)
{
    fbe_rdgen_service_t *rdgen_p = fbe_get_rdgen_service();
    return rdgen_p->num_seconds_per_trace;
}

/* Accessors to last trace time.
 */
static __forceinline void fbe_rdgen_set_last_trace_time(fbe_time_t time)
{
    fbe_rdgen_service_t *rdgen_p = fbe_get_rdgen_service();
    rdgen_p->last_trace_time = time;
    return;
}
static __forceinline fbe_time_t fbe_rdgen_get_last_trace_time(void)
{
    fbe_rdgen_service_t *rdgen_p = fbe_get_rdgen_service();
    return rdgen_p->last_trace_time;
}

/* Accessors for max_io_seconds.
 */
static __forceinline fbe_time_t fbe_rdgen_get_max_io_msecs(void)
{
    fbe_rdgen_service_t *rdgen_p = fbe_get_rdgen_service();
    return rdgen_p->max_io_msec;
}
static __forceinline void fbe_rdgen_set_max_io_msecs(fbe_time_t time)
{
    fbe_rdgen_service_t *rdgen_p = fbe_get_rdgen_service();
    rdgen_p->max_io_msec = time;
    return;
}

/* Accessors for service_options.
 */
static __forceinline fbe_rdgen_svc_options_t fbe_rdgen_get_service_options(void)
{
    fbe_rdgen_service_t *rdgen_p = fbe_get_rdgen_service();
    return rdgen_p->service_options;
}
static __forceinline void fbe_rdgen_set_service_options(fbe_rdgen_svc_options_t options)
{
    fbe_rdgen_service_t *rdgen_p = fbe_get_rdgen_service();
    rdgen_p->service_options = options;
    return;
}
/* Accessors for last_scan_time.
 */
static __forceinline fbe_time_t fbe_rdgen_get_last_scan_time(void)
{
    fbe_rdgen_service_t *rdgen_p = fbe_get_rdgen_service();
    return rdgen_p->last_scan_time;
}
static __forceinline void fbe_rdgen_update_last_scan_time(void)
{
    fbe_rdgen_service_t *rdgen_p = fbe_get_rdgen_service();
    rdgen_p->last_scan_time = fbe_get_time();
    return;
}
static __forceinline fbe_bool_t fbe_rdgen_is_time_to_scan_queues(void)
{
    fbe_bool_t b_status;

    /* It is time to scan if the elapsed time since the last scan 
     * is more than the scan time. 
     */
    if ((fbe_get_elapsed_milliseconds(fbe_rdgen_get_last_scan_time())) > 
        (FBE_RDGEN_THREAD_MAX_SCAN_SECONDS * FBE_TIME_MILLISECONDS_PER_SECOND))
    {
        b_status = FBE_TRUE;
    }
    else
    {
        b_status = FBE_FALSE;
    }
    return b_status;
}

/* Accessors for the number of objects.
 */
static __forceinline void fbe_rdgen_inc_num_objects(void)
{
    fbe_rdgen_service_t *rdgen_p = fbe_get_rdgen_service();
    rdgen_p->num_objects++;
    return;
}
static __forceinline void fbe_rdgen_dec_num_objects(void)
{
    fbe_rdgen_service_t *rdgen_p = fbe_get_rdgen_service();
    rdgen_p->num_objects--;
    return;
}
static __forceinline fbe_u32_t fbe_rdgen_get_num_objects(void)
{
    fbe_rdgen_service_t *rdgen_p = fbe_get_rdgen_service();
    return rdgen_p->num_objects;
}

/* Accessors for the number of threads.
 */
static __forceinline void fbe_rdgen_inc_num_threads(void)
{
    fbe_rdgen_service_t *rdgen_p = fbe_get_rdgen_service();
    rdgen_p->num_threads++;
    return;
}
static __forceinline void fbe_rdgen_dec_num_threads(void)
{
    fbe_rdgen_service_t *rdgen_p = fbe_get_rdgen_service();
    rdgen_p->num_threads--;
    return;
}
static __forceinline fbe_u32_t fbe_rdgen_get_num_threads(void)
{
    fbe_rdgen_service_t *rdgen_p = fbe_get_rdgen_service();
    return rdgen_p->num_threads;
}
static __forceinline void fbe_rdgen_init_num_threads(void)
{
    fbe_rdgen_service_t *rdgen_p = fbe_get_rdgen_service();
    rdgen_p->num_threads = 0;
    return;
}

/* Accessors for the number of ts.
 */
static __forceinline void fbe_rdgen_inc_num_ts(void)
{
    fbe_rdgen_service_t *rdgen_p = fbe_get_rdgen_service();
    fbe_atomic_32_increment(&rdgen_p->num_ts);
    return;
}
static __forceinline void fbe_rdgen_dec_num_ts(void)
{
    fbe_rdgen_service_t *rdgen_p = fbe_get_rdgen_service();
    fbe_atomic_32_decrement(&rdgen_p->num_ts);
    return;
}
static __forceinline fbe_u32_t fbe_rdgen_get_num_ts(void)
{
    fbe_rdgen_service_t *rdgen_p = fbe_get_rdgen_service();
    return rdgen_p->num_ts;
}
static __forceinline void fbe_rdgen_inc_sg_allocated(void)
{
    fbe_rdgen_service_t *rdgen_p = fbe_get_rdgen_service();
    fbe_atomic_increment(&rdgen_p->memory_statistics.sg_allocated_count);
    return;
}
static __forceinline void fbe_rdgen_inc_sg_freed(void)
{
    fbe_rdgen_service_t *rdgen_p = fbe_get_rdgen_service();
    fbe_atomic_increment(&rdgen_p->memory_statistics.sg_freed_count);
    return;
}
static __forceinline void fbe_rdgen_inc_mem_allocated(void)
{
    fbe_rdgen_service_t *rdgen_p = fbe_get_rdgen_service();
    fbe_atomic_increment(&rdgen_p->memory_statistics.mem_allocated_count);
    return;
}
static __forceinline void fbe_rdgen_inc_mem_freed(void)
{
    fbe_rdgen_service_t *rdgen_p = fbe_get_rdgen_service();
    fbe_atomic_increment(&rdgen_p->memory_statistics.mem_freed_count);
    return;
}
static __forceinline void fbe_rdgen_inc_ts_allocated(void)
{
    fbe_rdgen_service_t *rdgen_p = fbe_get_rdgen_service();
    fbe_atomic_increment(&rdgen_p->memory_statistics.ts_allocated_count);
    return;
}
static __forceinline void fbe_rdgen_inc_ts_freed(void)
{
    fbe_rdgen_service_t *rdgen_p = fbe_get_rdgen_service();
    fbe_atomic_increment(&rdgen_p->memory_statistics.ts_freed_count);
    return;
}
static __forceinline void fbe_rdgen_inc_req_allocated(void)
{
    fbe_rdgen_service_t *rdgen_p = fbe_get_rdgen_service();
    fbe_atomic_increment(&rdgen_p->memory_statistics.req_allocated_count);
    return;
}
static __forceinline void fbe_rdgen_inc_req_freed(void)
{
    fbe_rdgen_service_t *rdgen_p = fbe_get_rdgen_service();
    fbe_atomic_increment(&rdgen_p->memory_statistics.req_freed_count);
    return;
}
static __forceinline void fbe_rdgen_inc_obj_allocated(void)
{
    fbe_rdgen_service_t *rdgen_p = fbe_get_rdgen_service();
    fbe_atomic_increment(&rdgen_p->memory_statistics.obj_allocated_count);
    return;
}
static __forceinline void fbe_rdgen_inc_obj_freed(void)
{
    fbe_rdgen_service_t *rdgen_p = fbe_get_rdgen_service();
    fbe_atomic_increment(&rdgen_p->memory_statistics.obj_freed_count);
    return;
}


static __forceinline void fbe_rdgen_object_set_magic(fbe_rdgen_object_t *object_p)
{
    object_p->magic = FBE_MAGIC_NUM_RDGEN_OBJECT;
    return;
}
static __forceinline fbe_u64_t fbe_rdgen_object_get_magic(fbe_rdgen_object_t *object_p)
{
    return object_p->magic;
    
}
static __forceinline fbe_bool_t fbe_rdgen_object_validate_magic(fbe_rdgen_object_t *object_p)
{
    return (object_p->magic == FBE_MAGIC_NUM_RDGEN_OBJECT);
}
static __forceinline fbe_u64_t fbe_rdgen_object_get_object_id(fbe_rdgen_object_t *object_p)
{
    return object_p->object_id;
    
}

static __forceinline void fbe_rdgen_object_set_random_context(fbe_rdgen_object_t *object_p,
                                                              fbe_u64_t context)
{
    object_p->random_context = context;
    return;
}
static __forceinline fbe_u64_t fbe_rdgen_object_get_random_context(fbe_rdgen_object_t *object_p)
{
    return object_p->random_context;
    
}

static __forceinline void fbe_rdgen_enqueue_object(fbe_rdgen_object_t *object_p)
{
    fbe_rdgen_service_t *rdgen_p = fbe_get_rdgen_service();
    fbe_queue_push(&rdgen_p->active_object_head, &object_p->queue_element);
    return;
}
static __forceinline void fbe_rdgen_dequeue_object(fbe_rdgen_object_t *object_p)
{
    fbe_queue_remove(&object_p->queue_element);
    return;
}
static __forceinline void fbe_rdgen_get_active_object_queue(fbe_queue_head_t **queue_p)
{
    fbe_rdgen_service_t *rdgen_p = fbe_get_rdgen_service();
    *queue_p = &rdgen_p->active_object_head;
    return;
}

/* Accessor methods for the flags field.
 */
static __forceinline fbe_bool_t fbe_rdgen_object_is_flag_set(fbe_rdgen_object_t *raid_p,
                                                             fbe_rdgen_object_flags_t flag)
{
    return ((raid_p->flags & flag) == flag);
}
static __forceinline void fbe_rdgen_object_init_flags(fbe_rdgen_object_t *raid_p)
{
    raid_p->flags = 0;
    return;
}
static __forceinline void fbe_rdgen_object_set_flag(fbe_rdgen_object_t *raid_p,
                                              fbe_rdgen_object_flags_t flag)
{
    raid_p->flags |= flag;
}

static __forceinline void fbe_rdgen_object_clear_flag(fbe_rdgen_object_t *raid_p,
                                                fbe_rdgen_object_flags_t flag)
{
    raid_p->flags &= ~flag;
}

/* Accessors for the peer packet queue. 
 * This is the list of packets outstanding to the peer. 
 */
static __forceinline void fbe_rdgen_peer_packet_enqueue(fbe_packet_t *packet_p)
{
    fbe_rdgen_service_t *rdgen_p = fbe_get_rdgen_service();
    fbe_queue_push(&rdgen_p->peer_outstanding_packets, &packet_p->queue_element);
    return;
}

static __forceinline void fbe_rdgen_peer_packet_dequeue(fbe_packet_t *packet_p)
{
    fbe_queue_remove(&packet_p->queue_element);
    return;
}
static __forceinline void fbe_rdgen_get_peer_packet_queue(fbe_queue_head_t **queue_p)
{
    fbe_rdgen_service_t *rdgen_p = fbe_get_rdgen_service();
    *queue_p = &rdgen_p->peer_outstanding_packets;
    return;
}
static __forceinline fbe_u8_t fbe_rdgen_object_get_stack_size(fbe_rdgen_object_t *object_p)
{
    return object_p->stack_size;
}

static __forceinline void fbe_rdgen_object_set_stack_size(fbe_rdgen_object_t *object_p,
                                                     fbe_u8_t stack_size)
{
    object_p->stack_size = stack_size;
}
/* Accessors for the number of threads.
 */
static __forceinline fbe_u32_t fbe_rdgen_object_inc_num_threads(fbe_rdgen_object_t *object_p)
{
	fbe_u32_t old = object_p->active_ts_count;
    object_p->active_ts_count++;
    return old;
}
static __forceinline void fbe_rdgen_object_dec_num_threads(fbe_rdgen_object_t *object_p)
{
    object_p->active_ts_count--;
    return;
}
static __forceinline fbe_u32_t fbe_rdgen_object_get_num_threads(fbe_rdgen_object_t *object_p)
{
    return object_p->active_ts_count;
}


static __forceinline void fbe_rdgen_object_set_max_threads(fbe_rdgen_object_t *object_p,
                                                           fbe_u32_t threads)
{
    object_p->max_threads = threads;
}
static __forceinline fbe_u32_t fbe_rdgen_object_get_max_threads(fbe_rdgen_object_t *object_p)
{
    return object_p->max_threads;
}

static __forceinline void fbe_rdgen_object_set_current_threads(fbe_rdgen_object_t *object_p,
                                                           fbe_u32_t threads)
{
    object_p->current_threads = threads;
}
static __forceinline fbe_u32_t fbe_rdgen_object_get_current_threads(fbe_rdgen_object_t *object_p)
{
    return object_p->current_threads;
}
static __forceinline void fbe_rdgen_object_dec_current_threads(fbe_rdgen_object_t *object_p)
{
    object_p->current_threads--;
}
static __forceinline void fbe_rdgen_object_inc_current_threads(fbe_rdgen_object_t *object_p)
{
    object_p->current_threads++;
}
static __forceinline fbe_u32_t fbe_rdgen_object_get_restart_msec(fbe_rdgen_object_t *object_p)
{
    return object_p->playback_records_p->record_data[object_p->playback_record_index].msec_to_ramp;
}
static __forceinline void fbe_rdgen_object_inc_playback_index(fbe_rdgen_object_t *object_p)
{
    object_p->playback_record_index++;
}

static __forceinline void fbe_rdgen_object_set_restart_time(fbe_rdgen_object_t *object_p)
{
    object_p->time_to_restart = fbe_get_time() + fbe_rdgen_object_get_restart_msec(object_p);
}
static __forceinline void fbe_rdgen_object_reset_restart_time(fbe_rdgen_object_t *object_p)
{
    object_p->time_to_restart = 0;
}

static __forceinline fbe_time_t fbe_rdgen_object_get_restart_time(fbe_rdgen_object_t *object_p)
{
    return object_p->time_to_restart;
}
/* Accessors for the number of passes.
 */
static __forceinline void fbe_rdgen_object_inc_num_passes(fbe_rdgen_object_t *object_p)
{
    object_p->num_passes++;
    return;
}
static __forceinline void fbe_rdgen_object_dec_num_passes(fbe_rdgen_object_t *object_p)
{
    object_p->num_passes--;
    return;
}
static __forceinline fbe_u32_t fbe_rdgen_object_get_num_passes(fbe_rdgen_object_t *object_p)
{
    return object_p->num_passes;
}

/* Accessors for the number of peer_requests.
 */
static __forceinline void fbe_rdgen_object_inc_num_peer_requests(fbe_rdgen_object_t *object_p)
{
    object_p->num_peer_requests++;
    return;
}
static __forceinline void fbe_rdgen_object_dec_num_peer_requests(fbe_rdgen_object_t *object_p)
{
    object_p->num_peer_requests--;
    return;
}
static __forceinline fbe_u32_t fbe_rdgen_object_get_num_peer_requests(fbe_rdgen_object_t *object_p)
{
    return object_p->num_peer_requests;
}


static __forceinline void fbe_rdgen_request_set_magic(fbe_rdgen_request_t *request_p)
{
    request_p->magic = FBE_MAGIC_NUM_RDGEN_REQUEST;
    return;
}
static __forceinline fbe_bool_t fbe_rdgen_request_validate_magic(fbe_rdgen_request_t *request_p)
{
    return (request_p->magic == FBE_MAGIC_NUM_RDGEN_REQUEST);
}
/* Accessors for the number of threads.
 */
static __forceinline void fbe_rdgen_request_inc_num_threads(fbe_rdgen_request_t *request_p)
{
    request_p->active_ts_count++;
    return;
}
static __forceinline void fbe_rdgen_request_dec_num_threads(fbe_rdgen_request_t *request_p)
{
    request_p->active_ts_count--;
    return;
}
static __forceinline fbe_u32_t fbe_rdgen_request_get_num_threads(fbe_rdgen_request_t *request_p)
{
    return request_p->active_ts_count;
}

/* Accessors for the number of passes.
 */
static __forceinline void fbe_rdgen_request_inc_num_passes(fbe_rdgen_request_t *request_p,
                                               fbe_u32_t passes)
{
    request_p->pass_count += passes;
    return;
}
static __forceinline fbe_u32_t fbe_rdgen_request_get_num_passes(fbe_rdgen_request_t *request_p)
{
    return request_p->pass_count;
}

/* Accessors for the number of errors.
 */
static __forceinline void fbe_rdgen_request_inc_num_errors(fbe_rdgen_request_t *request_p,
                                               fbe_u32_t errors)
{
    request_p->err_count += errors;
    return;
}
static __forceinline fbe_u32_t fbe_rdgen_request_get_num_errors(fbe_rdgen_request_t *request_p)
{
    return request_p->err_count;
}

/* Accessors for the number of aborted_errors.
 */
static __forceinline void fbe_rdgen_request_inc_num_aborted_errors(fbe_rdgen_request_t *request_p,
                                               fbe_u32_t aborted_errors)
{
    request_p->aborted_err_count += aborted_errors;
    return;
}
static __forceinline fbe_u32_t fbe_rdgen_request_get_num_aborted_errors(fbe_rdgen_request_t *request_p)
{
    return request_p->aborted_err_count;
}

/* Accessors for the number of media_errors.
 */
static __forceinline void fbe_rdgen_request_inc_num_media_errors(fbe_rdgen_request_t *request_p,
                                               fbe_u32_t media_errors)
{
    request_p->media_err_count += media_errors;
    return;
}
static __forceinline fbe_u32_t fbe_rdgen_request_get_num_media_errors(fbe_rdgen_request_t *request_p)
{
    return request_p->media_err_count;
}

/* Accessors for the number of io_failure_errors.
 */
static __forceinline void fbe_rdgen_request_inc_num_io_failure_errors(fbe_rdgen_request_t *request_p,
                                               fbe_u32_t io_failure_errors)
{
    request_p->io_failure_err_count += io_failure_errors;
    return;
}
static __forceinline fbe_u32_t fbe_rdgen_request_get_num_io_failure_errors(fbe_rdgen_request_t *request_p)
{
    return request_p->io_failure_err_count;
}

/* Accessors for the number of congested_err_count.
 */
static __forceinline void fbe_rdgen_request_inc_num_congested_errors(fbe_rdgen_request_t *request_p,
                                               fbe_u32_t congested_errors)
{
    request_p->congested_err_count += congested_errors;
    return;
}
static __forceinline fbe_u32_t fbe_rdgen_request_get_num_congested_errors(fbe_rdgen_request_t *request_p)
{
    return request_p->congested_err_count;
}

/* Accessors for the number of still_congested_err_count.
 */
static __forceinline void fbe_rdgen_request_inc_num_still_congested_errors(fbe_rdgen_request_t *request_p,
                                               fbe_u32_t still_congested_errors)
{
    request_p->still_congested_err_count += still_congested_errors;
    return;
}
static __forceinline fbe_u32_t fbe_rdgen_request_get_num_still_congested_errors(fbe_rdgen_request_t *request_p)
{
    return request_p->still_congested_err_count;
}
/* Accessors for the number of io_failure_errors.
 */
static __forceinline void fbe_rdgen_request_inc_num_invalid_request_errors(fbe_rdgen_request_t *request_p,
                                                                      fbe_u32_t invalid_request_errors)
{
    request_p->invalid_request_err_count += invalid_request_errors;
    return;
}
static __forceinline fbe_u32_t fbe_rdgen_request_get_num_invalid_request_errors(fbe_rdgen_request_t *request_p)
{
    return request_p->invalid_request_err_count;
}

/* Accessors for the number of inv_blocks_count.
 */
static __forceinline void fbe_rdgen_request_inc_num_inv_blocks(fbe_rdgen_request_t *request_p,
                                                   fbe_u32_t inv_blocks)
{
    request_p->inv_blocks_count += inv_blocks;
    return;
}
static __forceinline fbe_u32_t fbe_rdgen_request_get_num_inv_blocks(fbe_rdgen_request_t *request_p)
{
    return request_p->inv_blocks_count;
}

/* Accessors for the number of bad_crc_blocks_count.
 */
static __forceinline void fbe_rdgen_request_inc_num_bad_crc_blocks(fbe_rdgen_request_t *request_p,
                                                       fbe_u32_t bad_crc_blocks)
{
    request_p->bad_crc_blocks_count += bad_crc_blocks;
    return;
}
static __forceinline fbe_u32_t fbe_rdgen_request_get_num_bad_crc_blocks(fbe_rdgen_request_t *request_p)
{
    return request_p->bad_crc_blocks_count;
}


/* Accessors for the number of ios.
 */
static __forceinline void fbe_rdgen_request_inc_num_ios(fbe_rdgen_request_t *request_p,
                                            fbe_u32_t io_count)
{
    request_p->io_count += io_count;
    return;
}
static __forceinline fbe_u32_t fbe_rdgen_request_get_num_ios(fbe_rdgen_request_t *request_p)
{
    return request_p->io_count;
}

static __forceinline void fbe_rdgen_request_inc_num_lock_conflicts(fbe_rdgen_request_t *request_p,
                                                              fbe_u32_t lock_conflict_count)
{
    request_p->lock_conflict_count += lock_conflict_count;
    return;
}
static __forceinline fbe_u32_t fbe_rdgen_request_get_num_lock_conflicts(fbe_rdgen_request_t *request_p)
{
    return request_p->lock_conflict_count;
}
static __forceinline void fbe_rdgen_get_active_request_queue(fbe_queue_head_t **queue_p)
{
    fbe_rdgen_service_t *rdgen_p = fbe_get_rdgen_service();
    *queue_p = &rdgen_p->active_request_head;
    return;
}
static __forceinline void fbe_rdgen_enqueue_request(fbe_rdgen_request_t *request_p)
{
    fbe_rdgen_service_t *rdgen_p = fbe_get_rdgen_service();
    fbe_queue_push(&rdgen_p->active_request_head, &request_p->queue_element);
    return;
}
static __forceinline void fbe_rdgen_dequeue_request(fbe_rdgen_request_t *request_p)
{
    fbe_queue_remove(&request_p->queue_element);
    return;
}

/* Accessors for the peer out of resource count
 */
static __inline void fbe_rdgen_inc_peer_out_of_resource_count(void)
{
    fbe_rdgen_service_t *rdgen_p = fbe_get_rdgen_service();
    rdgen_p->peer_out_of_resource_count++;
    return;
}
static __inline fbe_status_t fbe_rdgen_dec_peer_out_of_resource_count(void)
{
    fbe_rdgen_service_t *rdgen_p = fbe_get_rdgen_service();

    /* Only decrement if it is greater than 0 */
    if ((fbe_s32_t)rdgen_p->peer_out_of_resource_count < 0)
    {
       /* Reset the count and return failure*/
        rdgen_p->peer_out_of_resource_count = 0;
        return FBE_STATUS_GENERIC_FAILURE;
    }
    else if (rdgen_p->peer_out_of_resource_count > 0)
    {
        rdgen_p->peer_out_of_resource_count--;
    }
    return FBE_STATUS_OK;
}
static __inline fbe_u32_t fbe_rdgen_get_peer_out_of_resource_count(void)
{
    fbe_rdgen_service_t *rdgen_p = fbe_get_rdgen_service();
    return rdgen_p->peer_out_of_resource_count;
}

static __forceinline void fbe_rdgen_get_peer_request_queue(fbe_queue_head_t **queue_p)
{
    fbe_rdgen_service_t *rdgen_p = fbe_get_rdgen_service();
    *queue_p = &rdgen_p->peer_request_pool_head;
    return;
}
static __forceinline void fbe_rdgen_get_peer_request_active_queue(fbe_queue_head_t **queue_p)
{
    fbe_rdgen_service_t *rdgen_p = fbe_get_rdgen_service();
    *queue_p = &rdgen_p->peer_request_active_pool_head;
    return;
}
static __forceinline void fbe_rdgen_allocate_peer_request(fbe_rdgen_peer_request_t **request_p)
{
    fbe_rdgen_service_t *rdgen_p = fbe_get_rdgen_service();
    fbe_queue_element_t *queue_element_p = NULL;
    if (fbe_queue_is_empty(&rdgen_p->peer_request_pool_head))
    {
        *request_p = NULL;
        return;
    }
    queue_element_p = fbe_queue_pop(&rdgen_p->peer_request_pool_head);
    fbe_queue_push(&rdgen_p->peer_request_active_pool_head, queue_element_p);
    *request_p = fbe_rdgen_peer_request_pool_queue_element_to_ptr(queue_element_p);
    return;
}
static __forceinline void fbe_rdgen_free_peer_request(fbe_rdgen_peer_request_t *request_p)
{
    fbe_rdgen_service_t *rdgen_p = fbe_get_rdgen_service();
    /* take it off the in use peer_request_queue_head.
     */
    fbe_queue_remove(&request_p->pool_queue_element);
    fbe_queue_push(&rdgen_p->peer_request_pool_head, &request_p->pool_queue_element);
    return;
}
static __forceinline void fbe_rdgen_get_peer_request_pool(fbe_rdgen_peer_request_t **request_p)
{
    fbe_rdgen_service_t *rdgen_p = fbe_get_rdgen_service();
    *request_p = &rdgen_p->peer_request_pool[0];
    return;
}

/* Accessors for the caterpillar lba.
 */
static __forceinline void fbe_rdgen_request_set_caterpillar_lba(fbe_rdgen_request_t *request_p,
                                                    fbe_lba_t lba)
{
    request_p->caterpillar_lba = lba;
    return;
}
static __forceinline fbe_lba_t fbe_rdgen_request_get_caterpillar_lba(fbe_rdgen_request_t *request_p)
{
    return request_p->caterpillar_lba;
}

/* Accessors for the min io per sec object_id.
 */
static __forceinline void fbe_rdgen_request_set_min_rate_object(fbe_rdgen_request_t *request_p,
                                                           fbe_object_id_t object_id)
{
    request_p->min_rate_object_id = object_id;
    return;
}
static __forceinline fbe_object_id_t fbe_rdgen_request_get_min_rate_object(fbe_rdgen_request_t *request_p)
{
    return request_p->min_rate_object_id;
}

/* Accessors for the min rate ios
 */
static __forceinline void fbe_rdgen_request_set_min_rate_ios(fbe_rdgen_request_t *request_p,
                                                        fbe_u32_t io_per_sec)
{
    request_p->min_rate_ios = io_per_sec;
    return;
}
static __forceinline fbe_u32_t fbe_rdgen_request_get_min_rate_ios(fbe_rdgen_request_t *request_p)
{
    return request_p->min_rate_ios;
}
/* Accessors for the min rate milliseconds
 */
static __forceinline void fbe_rdgen_request_set_min_rate_msec(fbe_rdgen_request_t *request_p,
                                                         fbe_time_t msec)
{
    request_p->min_rate_msec = msec;
    return;
}
static __forceinline fbe_time_t fbe_rdgen_request_get_min_rate_msec(fbe_rdgen_request_t *request_p)
{
    return request_p->min_rate_msec;
}


/* Accessors for the max io per sec object_id.
 */
static __forceinline void fbe_rdgen_request_set_max_rate_object(fbe_rdgen_request_t *request_p,
                                                           fbe_object_id_t object_id)
{
    request_p->max_rate_object_id = object_id;
    return;
}
static __forceinline fbe_object_id_t fbe_rdgen_request_get_max_rate_object(fbe_rdgen_request_t *request_p)
{
    return request_p->max_rate_object_id;
}

/* Accessors for the max rate ios
 */
static __forceinline void fbe_rdgen_request_set_max_rate_ios(fbe_rdgen_request_t *request_p,
                                                        fbe_u32_t io_per_sec)
{
    request_p->max_rate_ios = io_per_sec;
    return;
}
static __forceinline fbe_u32_t fbe_rdgen_request_get_max_rate_ios(fbe_rdgen_request_t *request_p)
{
    return request_p->max_rate_ios;
}

/* Accessors for the max rate milliseconds
 */
static __forceinline void fbe_rdgen_request_set_max_rate_msec(fbe_rdgen_request_t *request_p,
                                                         fbe_time_t msec)
{
    request_p->max_rate_msec = msec;
    return;
}
static __forceinline fbe_time_t fbe_rdgen_request_get_max_rate_msec(fbe_rdgen_request_t *request_p)
{
    return request_p->max_rate_msec;
}

/* Determines if this request has been completed by looking at the b_stop value.
 */
static __forceinline fbe_bool_t fbe_rdgen_request_is_complete(fbe_rdgen_request_t *request_p)
{
    return request_p->b_stop;
}
static __forceinline void fbe_rdgen_request_set_complete(fbe_rdgen_request_t *request_p)
{
    request_p->b_stop = FBE_TRUE;
}

static __forceinline void fbe_rdgen_request_update_start_time(fbe_rdgen_request_t *request_p)
{
    request_p->start_time = fbe_get_time();
    return;
}
static __forceinline fbe_time_t fbe_rdgen_request_get_start_time(fbe_rdgen_request_t *request_p)
{
    return request_p->start_time;
}



/* We simply convert from the queue element to the ts ptr. 
 */
static __forceinline fbe_rdgen_ts_t * 
fbe_rdgen_ts_queue_element_to_ts_ptr(fbe_queue_element_t * queue_element_p)
{
    /* We're converting an address to a queue element into an address to the containing
     * rdgen ts. 
     * In order to do this we need to subtract the offset to the queue element from the 
     * address of the queue element. 
     */
    fbe_rdgen_ts_t * ts_p;
    ts_p = (fbe_rdgen_ts_t  *)((fbe_u8_t *)queue_element_p - 
                               (fbe_u8_t *)(&((fbe_rdgen_ts_t  *)0)->queue_element));
    return ts_p;
}

/* We simply convert from the request queue element to the ts ptr. 
 */
static __forceinline fbe_rdgen_ts_t * 
fbe_rdgen_ts_request_queue_element_to_ts_ptr(fbe_queue_element_t * queue_element_p)
{
    /* We're converting an address to a queue element into an address to the containing
     * rdgen ts. 
     * In order to do this we need to subtract the offset to the queue element from the 
     * address of the queue element. 
     */
    fbe_rdgen_ts_t * ts_p;
    ts_p = (fbe_rdgen_ts_t  *)((fbe_u8_t *)queue_element_p - 
                               (fbe_u8_t *)(&((fbe_rdgen_ts_t  *)0)->request_queue_element));
    return ts_p;
}

/* We simply convert from the request queue element to the ts ptr. 
 */
static __forceinline fbe_rdgen_ts_t * 
fbe_rdgen_ts_packet_to_ts_ptr(fbe_packet_t * packet_p)
{
    /* We're converting an address to a queue element into an address to the containing
     * rdgen ts. 
     * In order to do this we need to subtract the offset to the queue element from the 
     * address of the queue element. 
     */
    fbe_rdgen_ts_t * ts_p;
    ts_p = (fbe_rdgen_ts_t  *)((fbe_u8_t *)packet_p - 
                               (fbe_u8_t *)(&((fbe_rdgen_ts_t  *)0)->read_transport.packet));
    return ts_p;
}

/* Accessors for the wait peer packet queue. 
 * This is the list of packets waiting to get sent to the peer. 
 */
static __inline void fbe_rdgen_wait_peer_ts_enqueue(fbe_rdgen_ts_t *ts_p)
{
    fbe_rdgen_service_t *rdgen_p = fbe_get_rdgen_service();
    fbe_queue_push(&rdgen_p->peer_wait_ts, &ts_p->thread_queue_element);
    return;
}

static __inline void fbe_rdgen_get_wait_peer_ts_queue(fbe_queue_head_t **queue_p)
{
    fbe_rdgen_service_t *rdgen_p = fbe_get_rdgen_service();
    *queue_p = &rdgen_p->peer_wait_ts;
    return;
}


static __forceinline fbe_u32_t fbe_rdgen_get_random_seed(void)
{
    /* This assumes that the caller holds a spin.
     */
    fbe_rdgen_service_t *rdgen_p = fbe_get_rdgen_service();
    return fbe_rtl_random(&rdgen_p->random_context);
}
static __forceinline void fbe_rdgen_set_random_seed(fbe_u32_t seed)
{
    fbe_rdgen_service_t *rdgen_p = fbe_get_rdgen_service();
    rdgen_p->random_context = seed;
}
static __forceinline void fbe_rdgen_ts_set_magic(fbe_rdgen_ts_t *ts_p)
{
    ts_p->magic = FBE_MAGIC_NUM_RDGEN_TS;
    return;
}
static __forceinline fbe_bool_t fbe_rdgen_ts_validate_magic(fbe_rdgen_ts_t *ts_p)
{
    return (ts_p->magic == FBE_MAGIC_NUM_RDGEN_TS);
}

/* object accessors.
 */
static __forceinline void fbe_rdgen_ts_set_object(fbe_rdgen_ts_t *ts_p,
                                     fbe_rdgen_object_t * object_p)
{
    ts_p->object_p = object_p;
    return;
}
static __forceinline fbe_rdgen_object_t * fbe_rdgen_ts_get_object(fbe_rdgen_ts_t *ts_p)
{
    return ts_p->object_p;
}

/* memory_p accessors.
 */
static __forceinline void fbe_rdgen_ts_set_memory_p(fbe_rdgen_ts_t *ts_p,
                                                    void * memory_p)
{
    ts_p->memory_p = memory_p;
    return;
}
static __forceinline void * fbe_rdgen_ts_get_memory_p(fbe_rdgen_ts_t *ts_p)
{
    return ts_p->memory_p;
}

/* sg_p accessors.
 */
static __forceinline void fbe_rdgen_ts_set_sg_ptr(fbe_rdgen_ts_t *ts_p,
                                                  fbe_sg_element_t * sg_ptr)
{
    ts_p->sg_ptr = sg_ptr;
    return;
}
static __forceinline fbe_sg_element_t* fbe_rdgen_ts_get_sg_ptr(fbe_rdgen_ts_t *ts_p)
{
    return ts_p->sg_ptr;
}

/* block_size accessors.
 */
static __forceinline void fbe_rdgen_ts_set_block_size(fbe_rdgen_ts_t *ts_p,
                                                      fbe_block_size_t block_size)
{
    ts_p->block_size = block_size;
    return;
}
static __forceinline fbe_block_size_t fbe_rdgen_ts_get_block_size(fbe_rdgen_ts_t *ts_p)
{
    return ts_p->block_size;
}

/* memory_type accessors.
 */
static __forceinline void fbe_rdgen_ts_set_memory_type(fbe_rdgen_ts_t *ts_p,
                                                       fbe_rdgen_memory_type_t memory_type)
{
    ts_p->memory_type = memory_type;
    return;
}
static __forceinline fbe_rdgen_memory_type_t fbe_rdgen_ts_get_memory_type(fbe_rdgen_ts_t *ts_p)
{
    return ts_p->memory_type;
}
/* State accessors.
 */
static __forceinline void fbe_rdgen_ts_set_state(fbe_rdgen_ts_t *ts_p,
                                     fbe_rdgen_ts_state_t state)
{
    ts_p->state = state;
    return;
}
static __forceinline fbe_rdgen_ts_state_t fbe_rdgen_ts_get_state(fbe_rdgen_ts_t *ts_p)
{
    return ts_p->state;
}
/* Calling state accessors.
 */
static __forceinline void fbe_rdgen_ts_set_calling_state(fbe_rdgen_ts_t *ts_p,
                                             fbe_rdgen_ts_state_t calling_state)
{
    ts_p->calling_state = calling_state;
    return;
}
static __forceinline fbe_rdgen_ts_state_t fbe_rdgen_ts_get_calling_state(fbe_rdgen_ts_t *ts_p)
{
    return ts_p->calling_state;
}

/* Memory request accessor.
 */
static __forceinline fbe_memory_request_t* fbe_rdgen_ts_get_memory_request(fbe_rdgen_ts_t *ts_p)
{
    return &ts_p->memory_request;
}

/* Error count accessor.
 */
static __forceinline void fbe_rdgen_ts_inc_error_count(fbe_rdgen_ts_t *ts_p)
{
    ts_p->err_count++;
    return;
}
static __forceinline fbe_u32_t fbe_rdgen_ts_get_error_count(fbe_rdgen_ts_t *ts_p)
{
    return ts_p->err_count;
}

/* Status accessors.
 */
static __forceinline void fbe_rdgen_ts_set_status(fbe_rdgen_ts_t *ts_p,
                                      fbe_status_t status)
{
    ts_p->status = status;
    return;
}

static __forceinline void fbe_rdgen_ts_set_operation_status(fbe_rdgen_ts_t *ts_p,
                                                fbe_rdgen_operation_status_t operation_status)
{
    ts_p->last_packet_status.rdgen_status = operation_status;
    return;
}

static __forceinline void fbe_rdgen_ts_set_miscompare_status(fbe_rdgen_ts_t *ts_p)
{
    fbe_rdgen_ts_set_status(ts_p, FBE_STATUS_GENERIC_FAILURE);
    fbe_rdgen_ts_set_operation_status(ts_p, FBE_RDGEN_OPERATION_STATUS_MISCOMPARE);
    ts_p->last_packet_status.block_status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED;
    ts_p->last_packet_status.block_qualifier = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE;
    return;
}
static __forceinline void fbe_rdgen_ts_set_unexpected_error_status(fbe_rdgen_ts_t *ts_p)
{
    fbe_rdgen_ts_set_status(ts_p, FBE_STATUS_GENERIC_FAILURE);
    fbe_rdgen_ts_set_operation_status(ts_p, FBE_RDGEN_OPERATION_STATUS_UNEXPECTED_ERROR);
    fbe_rdgen_ts_inc_error_count(ts_p);
    ts_p->last_packet_status.block_status = FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST;
    ts_p->last_packet_status.block_qualifier = FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNEXPECTED_ERROR;
    return;
}
static __forceinline fbe_status_t fbe_rdgen_ts_get_status(fbe_rdgen_ts_t *ts_p)
{
    return ts_p->status;
}
static __forceinline fbe_lba_t fbe_rdgen_ts_get_max_lba(fbe_rdgen_ts_t *ts_p)
{
    fbe_lba_t max_lba;
     /* If the user did not specify a max lba, use the object's capacity.
     */
    if (ts_p->request_p->specification.max_lba == FBE_LBA_INVALID)
    {
        max_lba = ts_p->object_p->capacity - 1;
    }
    else if ((ts_p->request_p->specification.block_spec == FBE_RDGEN_BLOCK_SPEC_INCREASING) &&
             (ts_p->request_p->specification.lba_spec == FBE_RDGEN_LBA_SPEC_SEQUENTIAL_INCREASING))
    {
        /* The lbas loop from min_lba to max_lba and the block counts loop from min_blocks
         * to max_blocks.  Thus, the max lba we could get is below.
         */
        max_lba = ts_p->request_p->specification.max_lba + ts_p->request_p->specification.max_blocks - 1;
    }
    else
    {
        /* Otherwise just use the specified max lba.
         */
        max_lba = ts_p->request_p->specification.max_lba;
    }
    return max_lba;
}
static __forceinline fbe_lba_t fbe_rdgen_ts_get_min_lba(fbe_rdgen_ts_t *ts_p)
{
    fbe_lba_t min_lba;

     /* If the user did not specify a min lba, use the object's capacity.
     */
    if (ts_p->request_p->specification.min_lba == FBE_LBA_INVALID)
    {
        if (ts_p->request_p->specification.block_spec == FBE_RDGEN_BLOCK_SPEC_STRIPE_SIZE)
        {
            min_lba = ts_p->object_p->capacity - ts_p->object_p->stripe_size;
        }
        else
        {
        min_lba = ts_p->object_p->capacity - 1;
    }
    }
    else
    {
        /* Otherwise just use the specified min lba.
         */
        min_lba = ts_p->request_p->specification.min_lba;
    }
    return min_lba;
}

/* Sequence Count accessor.
 */
static __forceinline void fbe_rdgen_ts_inc_sequence_count(fbe_rdgen_ts_t *ts_p)
{
    ts_p->sequence_count++;
    return;
}
static __forceinline fbe_u32_t fbe_rdgen_ts_get_sequence_count(fbe_rdgen_ts_t *ts_p)
{
    return ts_p->sequence_count;
}

/* Thread Num accessor.
 */
static __forceinline void fbe_rdgen_ts_set_thread_num(fbe_rdgen_ts_t *ts_p,
                                          fbe_u32_t thread_num)
{
    ts_p->thread_num = thread_num;
    return;
}
static __forceinline fbe_u32_t fbe_rdgen_ts_get_thread_num(fbe_rdgen_ts_t *ts_p)
{
    return ts_p->thread_num;
}
/* Core num accessor.
 */
static __forceinline void fbe_rdgen_ts_set_core_num(fbe_rdgen_ts_t *ts_p,
                                               fbe_u32_t core_num)
{
    ts_p->core_num = core_num;
    return;
}
static __forceinline fbe_u32_t fbe_rdgen_ts_get_core_num(fbe_rdgen_ts_t *ts_p)
{
    return ts_p->core_num;
}

/* Pass Count accessor.
 */
static __forceinline void fbe_rdgen_ts_inc_pass_count(fbe_rdgen_ts_t *ts_p)
{
    /* Increment both the pass_count and the seqeunce count*/
    ts_p->pass_count++;
    if (!fbe_rdgen_specification_options_is_set(&ts_p->request_p->specification,
                                               FBE_RDGEN_DATA_PATTERN_FLAGS_FIXED_SEQUENCE_NUMBER))
    {
        fbe_rdgen_ts_inc_sequence_count(ts_p); 
    }
    return;
}
static __forceinline fbe_u32_t fbe_rdgen_ts_get_pass_count(fbe_rdgen_ts_t *ts_p)
{
    return ts_p->pass_count;
}

/* Io Count accessor.
 */
static __forceinline void fbe_rdgen_ts_inc_io_count(fbe_rdgen_ts_t *ts_p)
{
    ts_p->io_count++;
    return;
}
static __forceinline fbe_u32_t fbe_rdgen_ts_get_io_count(fbe_rdgen_ts_t *ts_p)
{
    return ts_p->io_count;
}
/* Lock conflicts count accessor.
 */
static __forceinline void fbe_rdgen_ts_inc_lock_conflict_count(fbe_rdgen_ts_t *ts_p)
{
    ts_p->lock_conflict_count++;
    return;
}
static __forceinline fbe_u32_t fbe_rdgen_ts_get_lock_conflict_count(fbe_rdgen_ts_t *ts_p)
{
    return ts_p->lock_conflict_count;
}

/* Peer Pass Count accessor.
 */
static __forceinline void fbe_rdgen_ts_inc_peer_pass_count(fbe_rdgen_ts_t *ts_p, fbe_u32_t count)
{
    ts_p->peer_pass_count += count;
    return;
}
static __forceinline fbe_u32_t fbe_rdgen_ts_get_peer_pass_count(fbe_rdgen_ts_t *ts_p)
{
    return ts_p->peer_pass_count;
}

/* Peer Io Count accessor.
 */
static __forceinline void fbe_rdgen_ts_inc_peer_io_count(fbe_rdgen_ts_t *ts_p, fbe_u32_t count)
{
    ts_p->peer_io_count += count;
    return;
}
static __forceinline fbe_u32_t fbe_rdgen_ts_get_peer_io_count(fbe_rdgen_ts_t *ts_p)
{
    return ts_p->peer_io_count;
}

/* Accessor for the packet.
 */
static __forceinline fbe_packet_t * fbe_rdgen_ts_get_packet(fbe_rdgen_ts_t *ts_p)
{
    return &ts_p->read_transport.packet;
}
static __forceinline fbe_packet_t * fbe_rdgen_ts_get_write_packet(fbe_rdgen_ts_t *ts_p)
{
    return &ts_p->write_transport.packet;
}
static __forceinline fbe_packet_t * fbe_rdgen_ts_get_packet_ptr(fbe_rdgen_ts_t *ts_p)
{
    return ts_p->packet_p;
}
static __forceinline void fbe_rdgen_ts_set_packet_ptr(fbe_rdgen_ts_t *ts_p, fbe_packet_t *packet_p)
{
    ts_p->packet_p = packet_p;
    return;
}
/* Accessor methods for the flags field.
 */
static __forceinline fbe_bool_t fbe_rdgen_ts_is_flag_set(fbe_rdgen_ts_t *raid_p,
                                                  fbe_rdgen_ts_flags_t flag)
{
    return ((raid_p->flags & flag) == flag);
}
static __forceinline void fbe_rdgen_ts_init_flags(fbe_rdgen_ts_t *raid_p)
{
    raid_p->flags = 0;
    return;
}
static __forceinline void fbe_rdgen_ts_set_flag(fbe_rdgen_ts_t *raid_p,
                                              fbe_rdgen_ts_flags_t flag)
{
    raid_p->flags |= flag;
}

static __forceinline void fbe_rdgen_ts_clear_flag(fbe_rdgen_ts_t *raid_p,
                                                fbe_rdgen_ts_flags_t flag)
{
    raid_p->flags &= ~flag;
}
static __forceinline void fbe_rdgen_ts_update_last_send_time(fbe_rdgen_ts_t *ts_p)
{
    ts_p->last_send_time = fbe_get_time_in_us();
    return;
}
static __forceinline void fbe_rdgen_ts_update_start_time(fbe_rdgen_ts_t *ts_p)
{
    ts_p->start_time = fbe_get_time();
    return;
}

static __forceinline void fbe_rdgen_ts_set_last_response_time(fbe_rdgen_ts_t *ts_p,
                                                              fbe_time_t last_response_time)
{
    ts_p->last_response_time = last_response_time;
    return;
}

static __forceinline void fbe_rdgen_ts_set_max_response_time(fbe_rdgen_ts_t *ts_p,
                                                             fbe_time_t last_response_time)
{
    ts_p->max_response_time = last_response_time;
    return;
}

static __forceinline void fbe_rdgen_ts_update_max_response_time(fbe_rdgen_ts_t *ts_p,
                                                                fbe_time_t last_response_time)
{
    if (last_response_time > ts_p->max_response_time)
    {
        ts_p->max_response_time = last_response_time;
    }
    return;
}
static __forceinline void fbe_rdgen_ts_set_io_interface(fbe_rdgen_ts_t *ts_p,
                                                        fbe_rdgen_interface_type_t io_interface)
{
    ts_p->io_interface = io_interface;
    return;
}
static __forceinline void fbe_rdgen_ts_set_lba(fbe_rdgen_ts_t *ts_p, fbe_lba_t lba)
{
    ts_p->lba = lba;
    return;
}
static __forceinline void fbe_rdgen_ts_set_blocks(fbe_rdgen_ts_t *ts_p, fbe_block_count_t blocks)
{
    ts_p->blocks = blocks;
    return;
}
static __forceinline void fbe_rdgen_ts_set_opcode(fbe_rdgen_ts_t *ts_p, fbe_rdgen_operation_t opcode)
{
    ts_p->operation = opcode;
    return;
}
static __forceinline void fbe_rdgen_ts_set_core(fbe_rdgen_ts_t *ts_p, fbe_u32_t core)
{
    ts_p->core_num = core;
    return;
}
static __forceinline void fbe_rdgen_ts_set_priority(fbe_rdgen_ts_t *ts_p, fbe_packet_priority_t priority)
{
    ts_p->priority = priority;
    return;
}
static __forceinline void fbe_rdgen_ts_set_block_opcode(fbe_rdgen_ts_t *ts_p, fbe_payload_block_operation_opcode_t block_opcode)
{
    ts_p->block_opcode = block_opcode;
    return;
}
static __forceinline void fbe_rdgen_ts_set_corrupt_bitmask(fbe_rdgen_ts_t *ts_p, fbe_u64_t bitmask)
{
    ts_p->corrupted_blocks_bitmap = bitmask;
    return;
}
static __forceinline void fbe_rdgen_ts_reset_stats(fbe_rdgen_ts_t *ts_p)
{
    ts_p->io_count = 0;
    ts_p->pass_count = 0;
    ts_p->cumulative_response_time = 0;
    ts_p->total_responses = 0;
    ts_p->max_response_time = 0;
    ts_p->err_count = 0;
    ts_p->abort_err_count = 0;
    ts_p->abort_retry_count = 0;
    ts_p->media_err_count = 0;
    ts_p->io_failure_err_count = 0;
    ts_p->congested_err_count = 0;
    ts_p->still_congested_err_count = 0;
    ts_p->inv_blocks_count = 0;
    ts_p->invalid_request_err_count = 0;
    ts_p->bad_crc_blocks_count = 0;
    ts_p->lock_conflict_count = 0;
    return;
}
static __forceinline void fbe_rdgen_ts_update_cum_resp_time(fbe_rdgen_ts_t *ts_p,
                                                            fbe_time_t last_response_time)
{
    if ((ts_p->cumulative_response_time > FBE_U32_MAX) &&
        (FBE_U64_MAX - ts_p->cumulative_response_time) >= last_response_time)
    {
        ts_p->cumulative_response_time = 0;
        ts_p->total_responses = 0;
    }
    ts_p->cumulative_response_time += last_response_time;
    ts_p->total_responses++;
    return;
}

static __forceinline void fbe_rdgen_object_lock(fbe_rdgen_object_t *object_p)
{
    fbe_spinlock_lock(&object_p->lock);
    return;
}
static __forceinline void fbe_rdgen_object_unlock(fbe_rdgen_object_t *object_p)
{
    fbe_spinlock_unlock(&object_p->lock);
    return;
}
static __forceinline void fbe_rdgen_object_enqueue_ts(fbe_rdgen_object_t *object_p,
                                          fbe_rdgen_ts_t *ts_p)
{
    fbe_queue_push(&object_p->active_ts_head, &ts_p->queue_element);
    return;
}
static __forceinline void fbe_rdgen_object_dequeue_ts(fbe_rdgen_object_t *object_p,
                                          fbe_rdgen_ts_t *ts_p)
{
    fbe_queue_remove(&ts_p->queue_element);
    return;
}
static __forceinline void fbe_rdgen_object_enqueue_valid_record(fbe_rdgen_object_t *object_p,
                                                                 fbe_rdgen_object_record_t *rec_p)
{
    fbe_queue_push(&object_p->valid_record_queue, &rec_p->queue_element);
    return;
}
static __forceinline void fbe_rdgen_object_enqueue_free_record(fbe_rdgen_object_t *object_p,
                                                                 fbe_rdgen_object_record_t *rec_p)
{
    fbe_queue_push(&object_p->free_record_queue, &rec_p->queue_element);
    return;
}
static __forceinline void fbe_rdgen_object_dequeue_record(fbe_rdgen_object_t *object_p,
                                                          fbe_rdgen_object_record_t *rec_p)
{
    fbe_queue_remove(&rec_p->queue_element);
    return;
}
static __forceinline fbe_bool_t fbe_rdgen_ts_is_aborted(fbe_rdgen_ts_t *ts_p)
{
    if (fbe_rdgen_service_is_flag_set(FBE_RDGEN_SERVICE_FLAGS_DRAIN_IO))
    {
        ts_p->b_aborted = FBE_TRUE;
    }
    return ts_p->b_aborted;
}
static __forceinline void fbe_rdgen_ts_mark_aborted(fbe_rdgen_ts_t *ts_p)
{
    /*! @todo need to check for aborted.
     */
    ts_p->b_aborted = FBE_TRUE;
    return;
}

static __forceinline void fbe_rdgen_request_lock(fbe_rdgen_request_t *request_p)
{
    fbe_spinlock_lock(&request_p->lock);
    return;
}
static __forceinline void fbe_rdgen_request_unlock(fbe_rdgen_request_t *request_p)
{
    fbe_spinlock_unlock(&request_p->lock);
    return;
}
static __forceinline void fbe_rdgen_request_enqueue_ts(fbe_rdgen_request_t *request_p,
                                          fbe_rdgen_ts_t *ts_p)
{
    fbe_queue_push(&request_p->active_ts_head, &ts_p->request_queue_element);
    return;
}
static __forceinline void fbe_rdgen_request_dequeue_ts(fbe_rdgen_request_t *request_p,
                                          fbe_rdgen_ts_t *ts_p)
{
    fbe_queue_remove(&ts_p->request_queue_element);
    return;
}

static __forceinline void fbe_rdgen_request_get_active_ts_queue(fbe_rdgen_request_t *request_p,
                                                    fbe_queue_head_t **queue_p)
{
    *queue_p = &request_p->active_ts_head;
    return;
}

static __forceinline void fbe_rdgen_ts_mark_waiting_lock(fbe_rdgen_ts_t *ts_p)
{
    fbe_rdgen_ts_inc_lock_conflict_count(ts_p);
    fbe_rdgen_ts_set_flag(ts_p, FBE_RDGEN_TS_FLAGS_WAITING_LOCK);
}



fbe_rdgen_ts_t * 
fbe_rdgen_ts_thread_queue_element_to_ts_ptr(fbe_queue_element_t * thread_queue_element_p);


/*************************
 * end file fbe_rdgen_private_inlines.h
 *************************/
#endif /* end FBE_RDGEN_PRIVATE_INLINES_H */
