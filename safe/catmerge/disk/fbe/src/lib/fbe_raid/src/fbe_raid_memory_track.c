/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_raid_memory_track.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the definitions of the memory tracking
 *
 * @version
 *  10/28/2010  Ron Proulx  - Created
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_types.h"
#include "fbe/fbe_atomic.h"
#include "fbe/fbe_memory.h"
#include "fbe/fbe_time.h"
#include "fbe/fbe_random.h"     
#include "fbe_raid_library_proto.h"

/*************************
 *   LITERAL DEFINITIONS
 *************************/
#define FBE_RAID_MEMORY_TRACKING_SLOTS (sizeof(fbe_u64_t) * 8)

/*************************
 *   STRUCTURE DEFINITIONS
 *************************/
typedef struct fbe_raid_memory_tracking_s
{
    fbe_memory_request_t    memory_request;
    fbe_memory_request_t   *orig_memory_request_p;
    fbe_system_time_t       system_time;

} fbe_raid_memory_tracking_t;


/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/*************************
 *   LOCAL GLOBALS
 *************************/
static fbe_bool_t       fbe_raid_memory_track_initialized = FBE_FALSE;
static fbe_spinlock_t   fbe_raid_memory_track_lock;
static fbe_atomic_t     fbe_raid_memory_track_total_allocations = 0;
static fbe_atomic_t     fbe_raid_memory_track_outstanding_allocations = 0;
static fbe_u64_t        fbe_raid_memory_tracking_outstanding_bitmask = 0;
static fbe_raid_memory_tracking_t fbe_raid_memory_tracking_outstanding_request[FBE_RAID_MEMORY_TRACKING_SLOTS] = { 0 };

/*!**************************************************************
 * fbe_raid_memory_track_initialize()
 ****************************************************************
 * @brief
 *  Initialize the raid memory tracking system
 * 
 * @param   None
 *
 * @return - None.
 *
 *
 ****************************************************************/
void fbe_raid_memory_track_initialize(void)
{
    fbe_spinlock_init(&fbe_raid_memory_track_lock);
    fbe_raid_memory_track_initialized = FBE_TRUE;

    return;
}
/************************************************
 * end of fbe_raid_memory_track_initialize()
 ************************************************/

/*!**************************************************************
 * fbe_raid_memory_track_allocation
 ****************************************************************
 * @brief
 *  Track memory request
 * 
 * @param memory_request_p - Pointer to memory request information
 *
 * @return - None.
 *
 *
 ****************************************************************/
void fbe_raid_memory_track_allocation(fbe_memory_request_t *const memory_request_p)
{
    fbe_u64_t   slot;
    fbe_raid_memory_tracking_t *memory_tracking_p = NULL;

    /* If the lock structure isn't initialize just return
     */
    if (fbe_raid_memory_track_initialized != FBE_TRUE)
    {
        fbe_raid_library_trace_basic(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                     "raid: memory request: 0x%p memory tracking not initialized\n",
                                     memory_request_p);
        return;
    }

    /* Now take the lock that protects the tracking structures
     */
    fbe_spinlock_lock(&fbe_raid_memory_track_lock);

    /* Locate the first available tracking slot.  We are procetected by
     * the memory lock
     */
    for (slot = 0; slot < FBE_RAID_MEMORY_TRACKING_SLOTS; slot++)
    {
        if ((fbe_raid_memory_tracking_outstanding_bitmask & (1ULL << slot)) == 0)
        {
            fbe_raid_memory_tracking_outstanding_bitmask |= (1ULL << slot);
            memory_tracking_p = &fbe_raid_memory_tracking_outstanding_request[slot];
            *(&memory_tracking_p->memory_request) = *memory_request_p;
            memory_tracking_p->orig_memory_request_p = memory_request_p;
            fbe_get_system_time(&memory_tracking_p->system_time);
            break;
        }
    }

    /* Even if there wasn't a tracking structure increment the counters
     */
    fbe_atomic_increment(&fbe_raid_memory_track_total_allocations);
    fbe_atomic_increment(&fbe_raid_memory_track_outstanding_allocations);

    /* Now release the lock that protects the tracking structures
     */
    fbe_spinlock_unlock(&fbe_raid_memory_track_lock);

    return;
}
/************************************************
 * end of fbe_raid_memory_track_allocation()
 ************************************************/

/*!**************************************************************
 * fbe_raid_memory_track_deferred_allocation
 ****************************************************************
 * @brief
 *  Update the associated memory request tracking structure with
 *  the actual master memory header value that was allocated
 * 
 * @param memory_request_p - Pointer to memory request information
 *
 * @return - None.
 *
 *
 ****************************************************************/
void fbe_raid_memory_track_deferred_allocation(fbe_memory_request_t *const memory_request_p)

{
    fbe_u64_t   slot;
    fbe_raid_memory_tracking_t *memory_tracking_p = NULL;

    /* If the lock structure isn't initialize just return
     */
    if (fbe_raid_memory_track_initialized != FBE_TRUE)
    {
        fbe_raid_library_trace_basic(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                     "raid: memory request: 0x%p memory tracking not initialized\n",
                                     memory_request_p);
        return;
    }

    /* Now take the lock that protects the tracking structures
     */
    fbe_spinlock_lock(&fbe_raid_memory_track_lock);

    /* Locate the slot (if any) that was used to `allocate' the
     * request.  Then update the entire memory request information
     * (which includes the updated master header value)
     */
    for (slot = 0; slot < FBE_RAID_MEMORY_TRACKING_SLOTS; slot++)
    {
        memory_tracking_p = &fbe_raid_memory_tracking_outstanding_request[slot];
        if ((fbe_raid_memory_tracking_outstanding_bitmask & (1ULL << slot)) &&
            (memory_tracking_p->orig_memory_request_p == memory_request_p)            )
        {
            /* Just update the entire contents with the updated memory request
             */
            *(&memory_tracking_p->memory_request) = *memory_request_p;
            break;
        }
    }

    /* Now release the lock that protects the tracking structures
     */
    fbe_spinlock_unlock(&fbe_raid_memory_track_lock);

    return;
}
/*********************************************************
 * end of fbe_raid_memory_track_deferred_allocation()
 ********************************************************/

/*!**************************************************************
 * fbe_raid_memory_track_free
 ****************************************************************
 * @brief
 *  Track memory request
 * 
 * @param memory_request_p - Pointer to memory request information
 *
 * @return - None.
 *
 *
 ****************************************************************/
void fbe_raid_memory_track_free(fbe_memory_header_t *const master_memory_header_p)
{
    fbe_u64_t   slot;
    fbe_raid_memory_tracking_t *memory_tracking_p = NULL;

    /* If the lock structure isn't initialize just return
     */
    if (fbe_raid_memory_track_initialized != FBE_TRUE)
    {
        fbe_raid_library_trace_basic(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                     "raid: memory header: 0x%p memory tracking not initialized\n",
                                     master_memory_header_p);
        return;
    }

    /* Now take the lock that protects the tracking structures
     */
    fbe_spinlock_lock(&fbe_raid_memory_track_lock);

    /* Walk thru all the in-use slots and locate the pointer to this master 
     * header.  We are protected by the memory lock.
     */
    for (slot = 0; slot < FBE_RAID_MEMORY_TRACKING_SLOTS; slot++)
    {
        if (fbe_raid_memory_tracking_outstanding_bitmask & (1ULL << slot))
        {
            /* Check if the master header value in the request matches
             */
            memory_tracking_p = &fbe_raid_memory_tracking_outstanding_request[slot];
            if (memory_tracking_p->memory_request.ptr == master_memory_header_p)
            {
                /* Found a matching entry.  Zero the array entry and clear the
                 * in-use bit.
                 */
                fbe_zero_memory(memory_tracking_p, sizeof(*memory_tracking_p));
                fbe_raid_memory_tracking_outstanding_bitmask &= ~(1ULL << slot);
                break;
            }
        }
    }

    /* Always decrement the frees even if a tracking structure wasn' located
     */
    fbe_atomic_decrement(&fbe_raid_memory_track_outstanding_allocations);

    /* Now release the lock that protects the tracking structures
     */
    fbe_spinlock_unlock(&fbe_raid_memory_track_lock);

    return;
}
/************************************************
 * end of fbe_raid_memory_track_free()
 ************************************************/

/*!**************************************************************
 * fbe_raid_memory_track_remove_aborted_request
 ****************************************************************
 * @brief
 *  Remove the specified memory request from the tracking array
 * 
 * @param memory_request_p - Pointer to memory request information
 *
 * @return - None.
 *
 *
 ****************************************************************/
void fbe_raid_memory_track_remove_aborted_request(fbe_memory_request_t *const memory_request_p)
{
    fbe_u64_t   slot;
    fbe_raid_memory_tracking_t *memory_tracking_p = NULL;

    /* If the lock structure isn't initialize just return
     */
    if (fbe_raid_memory_track_initialized != FBE_TRUE)
    {
        fbe_raid_library_trace_basic(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                     "raid: memory request: 0x%p memory tracking not initialized\n",
                                     memory_request_p);
        return;
    }

    /* Now take the lock that protects the tracking structures
     */
    fbe_spinlock_lock(&fbe_raid_memory_track_lock);

    /* Walk thru all the in-use slots and locate the pointer to this master 
     * header.  We are protected by the memory lock.
     */
    for (slot = 0; slot < FBE_RAID_MEMORY_TRACKING_SLOTS; slot++)
    {
        if (fbe_raid_memory_tracking_outstanding_bitmask & (1ULL << slot))
        {
            /* Check if the master header value in the request matches
             */
            memory_tracking_p = &fbe_raid_memory_tracking_outstanding_request[slot];
            if (memory_tracking_p->orig_memory_request_p == memory_request_p)
            {
                /* Found a matching entry.  Zero the array entry and clear the
                 * in-use bit.
                 */
                fbe_zero_memory(memory_tracking_p, sizeof(*memory_tracking_p));
                fbe_raid_memory_tracking_outstanding_bitmask &= ~(1ULL << slot);
                break;
            }
        }
    }

    /* Always decrement the frees even if a tracking structure wasn' located
     */
    fbe_atomic_decrement(&fbe_raid_memory_track_outstanding_allocations);

    /* Now release the lock that protects the tracking structures
     */
    fbe_spinlock_unlock(&fbe_raid_memory_track_lock);

    return;
}
/*******************************************************
 * end of fbe_raid_memory_track_remove_aborted_request()
 *******************************************************/

/*!**************************************************************
 * fbe_raid_memory_track_get_outstanding_allocations()
 ****************************************************************
 * @brief
 *  Get the outstanding allocation count
 * 
 * @param   None
 * 
 * @return  outstanding allocation count
 *
 ****************************************************************/
fbe_atomic_t fbe_raid_memory_track_get_outstanding_allocations(void)
{
     return(fbe_raid_memory_track_outstanding_allocations);
}
/************************************************************
 * end of fbe_raid_memory_track_get_outstanding_allocations()
 ************************************************************/

/*!**************************************************************
 * fbe_raid_memory_track_destroy()
 ****************************************************************
 * @brief
 *  Destroy the raid memory tracking system
 * 
 * @param   None
 * 
 * @return - None.
 *
 ****************************************************************/
void fbe_raid_memory_track_destroy(void)
{
#if 0 /* SAFEBUG - can't destroy locked spinlock */
    fbe_spinlock_lock(&fbe_raid_memory_track_lock);
#endif
    fbe_spinlock_destroy(&fbe_raid_memory_track_lock);
    fbe_raid_memory_track_initialized = FBE_FALSE;

    return;
}
/************************************************
 * end of fbe_raid_memory_track_destroy()
 ************************************************/

/**********************************
 * end file fbe_raid_memory_track.c
 **********************************/
