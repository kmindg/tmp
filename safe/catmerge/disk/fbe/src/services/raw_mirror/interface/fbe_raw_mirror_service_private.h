#ifndef FBE_RAW_MIRROR_SERVICE_PRIVATE_H
#define FBE_RAW_MIRROR_SERVICE_PRIVATE_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_raw_mirror_service_private.h
 ***************************************************************************
 *
 * @brief
 *  This file contains local definitions for the raw mirror service.
 * 
 *  In production code, a raw mirror is used to access the system LUNs non-paged metadata
 *  and their configuration tables.  There is no object interface for this data.  It is
 *  accessed early in boot before any objects have been created.  
 * 
 *  The raw mirror service is used to test fbe_raid_raw_mirror_library
 *  and associated RAID library code.
 *
 * @version
 *   10/2011:  Created. Susan Rundbaken 
 *
 ***************************************************************************/


/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe_raw_mirror_service_thread.h"
#include "fbe/fbe_raw_mirror_service_interface.h"

/*************************
 *    DEFINITIONS
 *************************/

/*!*******************************************************************
 * @struct fbe_raw_mirror_service_ts_t
 *********************************************************************
 * @brief
 *  This is the definition of the raw mirror tracking structure used to track
 *  the I/O request assigned to a raw mirror thread.
 *
 *********************************************************************/
typedef struct fbe_raw_mirror_service_ts_s
{
    /*! This is for enqueuing the ts to the raw mirror thread for execution.
     */
    fbe_queue_element_t thread_queue_element;

    /*! This is for enqueuing the ts to the raw mirror service active request queue.
     */
    fbe_queue_element_t request_queue_element;

    /*! This is the number of times this tracking structure has been processed. 
     */
    fbe_u32_t pass_count;

    /*! This is the start io specification set in the test. 
     *  The specification defines the I/O parameters.
     */
    fbe_raw_mirror_service_io_specification_t  start_io_spec;

    /*! This is used to store the verify counts set by RAID. 
     */
    fbe_raid_verify_raw_mirror_errors_t verify_report;
}
fbe_raw_mirror_service_ts_t;

/*!*******************************************************************
 * @struct fbe_raw_mirror_service_io_stats_t
 *********************************************************************
 * @brief
 *  This is the definition of the raw mirror statistics used to keep track
 *  of the number of failures across tracking structures for a given request.
 *
 *********************************************************************/
typedef struct fbe_raw_mirror_service_io_stats_s
{
    fbe_u32_t total_pass_count;             /* Total number of passes across tracking structures. */
    fbe_u32_t total_err_count;              /* Total number of errors. */
    fbe_u32_t total_dead_err_count;         /* Total number of dead errors. */
    fbe_u32_t total_u_crc_count;            /* Uncorrectable crc errors */
    fbe_u32_t total_c_crc_count;            /* Correctable crc errors */
    fbe_u32_t total_u_crc_multi_count;      /* Uncorrectable crc multi-bit errors */
    fbe_u32_t total_c_crc_multi_count;      /* Correctable crc multi-bit errors */
    fbe_u32_t total_u_crc_single_count;     /* Uncorrectable crc single-bit errors */
    fbe_u32_t total_c_crc_single_count;     /* Correctable crc single-bit errors */
    fbe_u32_t total_u_coh_count;            /* Uncorrectable coherency errors */
    fbe_u32_t total_c_coh_count;            /* Correctable coherency errors */
    fbe_u32_t total_u_ts_count;             /* Uncorrectable time stamp errors */
    fbe_u32_t total_c_ts_count;             /* Correctable time stamp errors */
    fbe_u32_t total_u_ws_count;             /* Uncorrectable write stamp errors */
    fbe_u32_t total_c_ws_count;             /* Correctable write stamp errors */
    fbe_u32_t total_u_ss_count;             /* Uncorrectable shed stamp errors */
    fbe_u32_t total_c_ss_count;             /* Correctable shed stamp errors */
    fbe_u32_t total_u_media_count;          /* Uncorrectable media errors */
    fbe_u32_t total_c_media_count;          /* Correctable media errors */
    fbe_u32_t total_c_soft_media_count;     /* Soft media errors*/
    fbe_u32_t total_retryable_errors;       /* Errors we could retry. */
    fbe_u32_t total_non_retryable_errors;   /* Errors we could not retry */
    fbe_u32_t total_shutdown_errors;        /* Error where we went shutdown. */
    fbe_u32_t total_u_rm_magic_count;       /* Uncorrectable raw mirror magic num errors. */
    fbe_u32_t total_c_rm_magic_count;       /* Correctable raw mirror magic num errors. */
    fbe_u32_t total_c_rm_seq_count;         /* Correctable raw mirror sequence num errors. */
    fbe_u16_t total_rm_magic_bitmap;        /* Bitmask of raw mirror positions with magic num errors.*/
    fbe_u16_t total_rm_seq_bitmap;          /* Bitmask of raw mirror positions with seq num errors. */
}
fbe_raw_mirror_service_io_stats_t;

/*!*******************************************************************
 * @struct fbe_raw_mirror_service_t
 *********************************************************************
 * @brief
 *  This is the definition of the raw mirror service which is used to
 *  generate I/O to the raw mirror.
 *
 *********************************************************************/
typedef struct fbe_raw_mirror_service_s
{
    fbe_base_service_t base_service;

    /*! Raw mirror thread pool. 
     *  Used to process I/O requests from user.
     */
    fbe_raw_mirror_service_thread_t  *thread_pool;

    /*! Number of threads. 
     */
    fbe_u32_t num_threads;

    /*! This is the packet that originated this request.
     */
    fbe_packet_t *packet_p;

    /*! List of tracking structures associated with this request. 
     *  For tests with multiple threads, there is one ts per thread.
     */
    fbe_queue_head_t active_ts_head;

    /*! Used to determine if request should be stopped or not. 
     *  There is only 1 active request to the raw mirror service at a time.     
     */
    fbe_bool_t active_stop_b;

    /*! Used to keep track of I/O tracking structure statistics. 
     */
    fbe_raw_mirror_service_io_stats_t   stats;

    /*! This is the lock to protect local data.
     */
    fbe_spinlock_t lock;
}
fbe_raw_mirror_service_t;


/*************************
 *  FUNCTION DEFINITIONS
 *************************/

fbe_raw_mirror_service_t *fbe_get_raw_mirror_service(void);
void fbe_raw_mirror_service_trace(fbe_trace_level_t trace_level,
                                  fbe_trace_message_id_t message_id,
                                  const char * fmt, ...)
                                 __attribute__((format(__printf_func__,3,4)));
fbe_status_t fbe_raw_mirror_service_start_io_request_packet(fbe_raw_mirror_service_ts_t * ts_p);
fbe_raw_mirror_service_ts_t *fbe_raw_mirror_service_ts_thread_queue_element_to_ts_ptr(fbe_queue_element_t * thread_queue_element_p);

fbe_status_t fbe_raw_mirror_service_sg_list_create(fbe_payload_ex_t * sep_payload_p,
                                                   fbe_raw_mirror_service_io_specification_t * start_io_spec_p);
fbe_status_t fbe_raw_mirror_service_sg_list_check(fbe_raw_mirror_service_io_specification_t * start_io_spec_p,
                                                  fbe_sg_element_t * sg_list_p,
                                                  fbe_u32_t num_sg_list_elements);
void fbe_raw_mirror_service_sg_list_free(fbe_sg_element_t * sg_list_p);


static __inline void fbe_raw_mirror_service_set_thread_pool_in_service(fbe_raw_mirror_service_thread_t *thread_pool_p)
{
    fbe_raw_mirror_service_t *raw_mirror_service_p = fbe_get_raw_mirror_service();

    raw_mirror_service_p->thread_pool = thread_pool_p;

    return;
}

static __inline fbe_bool_t fbe_raw_mirror_service_is_initialized(void)
{
    fbe_raw_mirror_service_t *raw_mirror_service_p = fbe_get_raw_mirror_service();

    return (raw_mirror_service_p->base_service.initialized);
};

static __inline fbe_u32_t fbe_raw_mirror_service_get_num_threads(void)
{
    fbe_raw_mirror_service_t *raw_mirror_service_p = fbe_get_raw_mirror_service();

    return raw_mirror_service_p->num_threads;
};

static __inline void fbe_raw_mirror_service_inc_num_threads(void)
{
    fbe_raw_mirror_service_t *raw_mirror_service_p = fbe_get_raw_mirror_service();

    raw_mirror_service_p->num_threads++;

    return;
}

static __inline fbe_status_t fbe_raw_mirror_service_get_thread(fbe_u32_t thread_num,
                                                               fbe_raw_mirror_service_thread_t **thread_p)
{
    fbe_raw_mirror_service_t *raw_mirror_service_p = fbe_get_raw_mirror_service();

    if (raw_mirror_service_p->thread_pool == NULL)
    {
        *thread_p = NULL;
        return FBE_STATUS_GENERIC_FAILURE;
    }

    *thread_p = &raw_mirror_service_p->thread_pool[thread_num];

    return FBE_STATUS_OK;
}

static __inline void fbe_raw_mirror_service_lock(fbe_raw_mirror_service_t *raw_mirror_service_p)
{
    fbe_spinlock_lock(&raw_mirror_service_p->lock);
    return;
}

static __inline void fbe_raw_mirror_service_unlock(fbe_raw_mirror_service_t *raw_mirror_service_p)
{
    fbe_spinlock_unlock(&raw_mirror_service_p->lock);
    return;
}

static __inline fbe_bool_t fbe_raw_mirror_service_request_queue_is_empty(void)
{
    fbe_raw_mirror_service_t *raw_mirror_service_p = fbe_get_raw_mirror_service();

    fbe_raw_mirror_service_lock(raw_mirror_service_p);
    if (fbe_queue_is_empty(&raw_mirror_service_p->active_ts_head))
    {
        fbe_raw_mirror_service_unlock(raw_mirror_service_p);
        return FBE_TRUE;
    }

    fbe_raw_mirror_service_unlock(raw_mirror_service_p);

    return FBE_FALSE;
}

static __inline void fbe_raw_mirror_service_stop_request(void)
{
    fbe_raw_mirror_service_t *raw_mirror_service_p = fbe_get_raw_mirror_service();

    raw_mirror_service_p->active_stop_b = FBE_TRUE;

    return;
}

static __inline fbe_bool_t fbe_raw_mirror_service_is_request_stopped(void)
{
    fbe_raw_mirror_service_t *raw_mirror_service_p = fbe_get_raw_mirror_service();

    return (raw_mirror_service_p->active_stop_b);
}

static __inline void fbe_raw_mirror_service_set_test_packet(fbe_packet_t* test_packet_p)
{
    fbe_raw_mirror_service_t *raw_mirror_service_p = fbe_get_raw_mirror_service();

    raw_mirror_service_p->packet_p = test_packet_p;
}

static __inline fbe_packet_t* fbe_raw_mirror_service_get_test_packet(void)
{
    fbe_raw_mirror_service_t *raw_mirror_service_p = fbe_get_raw_mirror_service();

    return raw_mirror_service_p->packet_p;
}


/*************************
 * end file fbe_raw_mirror_service_private.h
 *************************/

#endif /* end FBE_RAW_MIRROR_SERVICE_PRIVATE_H */

