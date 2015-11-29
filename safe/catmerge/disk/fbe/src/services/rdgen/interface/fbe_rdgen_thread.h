#ifndef FBE_RDGEN_THREAD_H
#define FBE_RDGEN_THREAD_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_rdgen_thread.h
 ***************************************************************************
 *
 * @brief
 *  This file contains local definitions for the rdgen threads.
 *
 * @version
 *   06/04/2009:  Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_queue.h"
#include "fbe/fbe_winddk.h"

/*!*******************************************************************
 * @enum fbe_rdgen_thread_type_t
 *********************************************************************
 * @brief Defines the kind of thread we are executing in.
 *
 *********************************************************************/
typedef enum fbe_rdgen_thread_type_e
{
    FBE_RDGEN_THREAD_TYPE_INVALID = 0,
    FBE_RDGEN_THREAD_TYPE_IO, /*! This thread gives I/Os context to run in. */

     /*! This thread scans queues periodically and displays errors when 
      *  I/Os appear to be stuck. 
      */
    FBE_RDGEN_THREAD_TYPE_SCAN_QUEUES,
     /*! This thread kicks off requests which have arrived from the peer. 
      */
    FBE_RDGEN_THREAD_TYPE_PEER,

    /*! This thread will abort any I/Os that require it.
     */
    FBE_RDGEN_THREAD_TYPE_ABORT_IOS,

    /*! Thread to coordinate playback.
     */
    FBE_RDGEN_THREAD_TYPE_OBJECT_PLAYBACK,
}
fbe_rdgen_thread_type_t;

/*!*******************************************************************
 * @def FBE_RDGEN_THREAD_MAX_SCAN_SECONDS
 *********************************************************************
 * @brief Max time in between scanning our queues.
 *
 *********************************************************************/
#define FBE_RDGEN_THREAD_MAX_SCAN_SECONDS 15

/*!*******************************************************************
 * @enum fbe_rdgen_thread_flags_t
 *********************************************************************
 * @brief
 *  This is the set of flags used in the thread.
 *
 *********************************************************************/
typedef enum fbe_rdgen_thread_flags_e
{
    FBE_RDGEN_THREAD_FLAGS_INVALID      =   0x00, /*!< No flags are set. */
    FBE_RDGEN_THREAD_FLAGS_HALT         =  0x01,  /*!< Stop the thread.  */
    FBE_RDGEN_THREAD_FLAGS_STOPPED      =  0x02,  /*!< Thread is stopped. */
    FBE_RDGEN_THREAD_FLAGS_INITIALIZED  =  0x04,  /*!< Thread is initialized. */
    FBE_RDGEN_THREAD_FLAGS_LAST
}
fbe_rdgen_thread_flags_t;

/*!*******************************************************************
 * @struct fbe_rdgen_thread_t
 *********************************************************************
 * @brief
 *  This is the definition of the rdgen tracking structure,
 *  which is used to track a single thread of I/O to a given object.
 *
 *********************************************************************/
typedef struct fbe_rdgen_thread_s
{
    /*! Allows a thread to be queued.
     */
    fbe_queue_element_t queue_element;

    /*! This is the handle to our thread object.
     */
    fbe_thread_t thread_handle;

    /*! This is the queue of requests for the thread to process.
     */
    fbe_queue_head_t ts_queue_head;

    /*! This is the lock to protect local data.
     */
    fbe_spinlock_t lock;

    /*! Number of requests on our queue.
     */
    fbe_u32_t queued_requests;

    /*! Used to signal the thread.
     */
    fbe_rendezvous_event_t event;

    /*! Used to communicate with the thread.
     */
    fbe_rdgen_thread_flags_t flags;

    /*! Thread Number, used for affinity.
     */
    fbe_u32_t thread_number; 
}
fbe_rdgen_thread_t;

/* Accessor methods for the flags field.
 */
static __forceinline fbe_bool_t fbe_rdgen_thread_is_flag_set(fbe_rdgen_thread_t *raid_p,
                                                      fbe_rdgen_thread_flags_t flag)
{
    return ((raid_p->flags & flag) == flag);
}
static __forceinline void fbe_rdgen_thread_init_flags(fbe_rdgen_thread_t *raid_p)
{
    raid_p->flags = 0;
    return;
}
static __forceinline void fbe_rdgen_thread_set_flag(fbe_rdgen_thread_t *raid_p,
                                             fbe_rdgen_thread_flags_t flag)
{
    raid_p->flags |= flag;
    return;
}

static __forceinline void fbe_rdgen_thread_clear_flag(fbe_rdgen_thread_t *raid_p,
                                               fbe_rdgen_thread_flags_t flag)
{
    raid_p->flags &= ~flag;
    return;
}

static __inline void fbe_rdgen_thread_lock(fbe_rdgen_thread_t *thread_p)
{
    fbe_spinlock_lock(&thread_p->lock);
    return;
}
static __inline void fbe_rdgen_thread_unlock(fbe_rdgen_thread_t *thread_p)
{
    fbe_spinlock_unlock(&thread_p->lock);
    return;
}

/*************************
 * API Functions
 *************************/
fbe_status_t fbe_rdgen_thread_init(fbe_rdgen_thread_t *thread_p,
                                   const char *name,
                                   fbe_rdgen_thread_type_t thread_type,
                                   fbe_u32_t thread_number);
fbe_status_t fbe_rdgen_thread_destroy(fbe_rdgen_thread_t *thread_p);
void fbe_rdgen_thread_enqueue(fbe_rdgen_thread_t *thread_p,
                              fbe_queue_element_t *queue_element_p);
void fbe_rdgen_peer_thread_enqueue(fbe_queue_element_t *queue_element_p);
fbe_bool_t fbe_rdgen_peer_thread_is_empty(void);
void fbe_rdgen_wakeup_abort_thread(void);
void fbe_rdgen_peer_thread_signal(void);
void fbe_rdgen_object_thread_signal(void);

/*************************
 * end file fbe_rdgen_thread.h
 *************************/
#endif /* end FBE_RDGEN_THREAD_H */

