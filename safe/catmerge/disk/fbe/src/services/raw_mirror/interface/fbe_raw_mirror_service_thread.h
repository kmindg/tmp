#ifndef FBE_RAW_MIRROR_SERVICE_THREAD_H
#define FBE_RAW_MIRROR_SERVICE_THREAD_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_raw_mirror_service_thread.h
 ***************************************************************************
 *
 * @brief
 *  This file contains definitions for the raw mirror service thread support.
 *
 * @version
 *   11/2011:  Created. Susan Rundbaken 
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_queue.h"
#include "fbe/fbe_winddk.h"

/*************************
 *    DEFINITIONS
 *************************/

/*!*******************************************************************
 * @enum fbe_raw_mirror_service_thread_flags_t
 *********************************************************************
 * @brief
 *  This is the set of flags used by the thread.
 *
 *********************************************************************/
typedef enum fbe_raw_mirror_service_thread_flags_e
{
    FBE_RAW_MIRROR_SERVICE_THREAD_FLAGS_INVALID      =  0x00,  /*!< No flags are set. */
    FBE_RAW_MIRROR_SERVICE_THREAD_FLAGS_HALT         =  0x01,  /*!< Stop the thread.  */
    FBE_RAW_MIRROR_SERVICE_THREAD_FLAGS_STOPPED      =  0x02,  /*!< Thread is stopped. */
    FBE_RAW_MIRROR_SERVICE_THREAD_FLAGS_INITIALIZED  =  0x04,  /*!< Thread is initialized. */
    FBE_RAW_MIRROR_SERVICE_THREAD_FLAGS_LAST
}
fbe_raw_mirror_service_thread_flags_t;

/*!*******************************************************************
 * @struct fbe_raw_mirror_service_thread_t
 *********************************************************************
 * @brief
 *  This is the definition of the raw mirror thread.
 *
 *********************************************************************/
typedef struct fbe_raw_mirror_service_thread_s
{
    /*! This is the handle to our thread.
     */
    fbe_thread_t thread_handle;

    /*! This is the queue of requests for the thread to process.
     */
    fbe_queue_head_t ts_queue_head;

    /*! This is the lock to protect local data.
     */
    fbe_spinlock_t lock;

    /*! Used to signal the thread.
     */
    fbe_semaphore_t semaphore;

    /*! Used to communicate with the thread.
     */
    fbe_raw_mirror_service_thread_flags_t flags;

    /*! Thread number used for affinity.
     */
    fbe_u32_t thread_number; 
}
fbe_raw_mirror_service_thread_t;


/*************************
 *  FUNCTION DEFINITIONS
 *************************/

fbe_status_t fbe_raw_mirror_service_init_thread(fbe_raw_mirror_service_thread_t *thread_p,
                                                fbe_u32_t thread_number);
void fbe_raw_mirror_service_thread_enqueue(fbe_raw_mirror_service_thread_t *thread_p,
                                           fbe_queue_element_t *queue_element_p);
fbe_status_t fbe_raw_mirror_service_destroy_thread(fbe_raw_mirror_service_thread_t *thread_p);


static __inline void fbe_raw_mirror_service_thread_set_flag(fbe_raw_mirror_service_thread_t *thread_p,
                                                            fbe_raw_mirror_service_thread_flags_t flag)
{
    thread_p->flags |= flag;
    return;
}

static __inline fbe_bool_t fbe_raw_mirror_service_thread_is_flag_set(fbe_raw_mirror_service_thread_t *thread_p,
                                                                     fbe_raw_mirror_service_thread_flags_t flag)
{
    return ((thread_p->flags & flag) == flag);
}

static __inline void fbe_raw_mirror_service_thread_lock(fbe_raw_mirror_service_thread_t *thread_p)
{
    fbe_spinlock_lock(&thread_p->lock);
    return;
};

static __inline void fbe_raw_mirror_service_thread_unlock(fbe_raw_mirror_service_thread_t *thread_p)
{
    fbe_spinlock_unlock(&thread_p->lock);
    return;
};

/*************************
 * end file fbe_raw_mirror_service_thread.h
 *************************/

#endif /* end FBE_RAW_MIRROR_SERVICE_THREAD_H */


