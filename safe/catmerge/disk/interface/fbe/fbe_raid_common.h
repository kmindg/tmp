#ifndef FBE_RAID_COMMON_H
#define FBE_RAID_COMMON_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_raid_common.h
 ***************************************************************************
 *
 * @brief
 *  This file contains .
 *
 * @version
 *   5/15/2009:  Created. rfoley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_queue.h"
#include "fbe_library_interface.h"

/*!*************************************************
 * @typedef fbe_raid_position_t
 ***************************************************
 * @brief Defines a raid position field for a position
 *        within a raid group.
 * 
 ***************************************************/
typedef fbe_u16_t fbe_raid_position_t;

/*!************************************************************
 * @def     FBE_RAID_POSITION_INVALID
 *
 * @brief   A raid group position valid that is invalid
 *
 *************************************************************/
#define FBE_RAID_POSITION_INVALID  FBE_U16_MAX

/*!*************************************************
 * @typedef fbe_raid_position_bitmask_t
 ***************************************************
 * @brief Defines a bitmask for all possible positions
 *        in a raid group
 * 
 ***************************************************/
typedef fbe_u16_t  fbe_raid_position_bitmask_t;

/*!************************************************************
 * @def FBE_RAID_POSITION_BITMASK_ALL_SET
 *
 * @brief  A bitmask of fbe_raid_position_bitmask_t with all bits set
 *
 *************************************************************/
#define FBE_RAID_POSITION_BITMASK_ALL_SET  FBE_U16_MAX


/******************************************************************
 *
 * RAID Driver request status
 *
 * This status is returned by all of the functions that
 * make up the RAID GROUP driver state machines.
 *
 ******************************************************************/
typedef enum fbe_raid_state_status_e
{
    FBE_RAID_STATE_STATUS_INVALID = 0,
    FBE_RAID_STATE_STATUS_EXECUTING,
    FBE_RAID_STATE_STATUS_WAITING,
    FBE_RAID_STATE_STATUS_DONE,
    FBE_RAID_STATE_STATUS_SLEEPING,
    FBE_RAID_STATE_STATUS_LAST,
}
fbe_raid_state_status_t;

struct fbe_raid_common_s;
typedef fbe_raid_state_status_t(*fbe_raid_common_state_t) (struct fbe_raid_common_s *);

/*!*******************************************************************
 * @enum fbe_raid_common_flags_t
 *********************************************************************
 * @brief
 *   These are the set of valid flags for a raid common object.
 *
 * @note    Please add and new/changes to these flags to the 
 *          FBE_RAID_COMMON_FLAG_STRINGS below.
 *
 *********************************************************************/
typedef enum fbe_raid_common_flags_e
{
    FBE_RAID_COMMON_FLAG_NONE                       = 0, /*!< No flags are set. */

    /* When we have successfully started a request, 
     * to the lower driver, we the started flag.
     */
    FBE_RAID_COMMON_FLAG_REQ_STARTED                = 0x00000001,
    FBE_RAID_COMMON_FLAG_REQ_BUSIED                 = 0x00000002,

    /* Used for embedded structures.
     */
    FBE_RAID_COMMON_FLAG_EMBED_ALLOC                = 0x00000004,

    /*! Memory allocation for this ts is complete
     */
    FBE_RAID_COMMON_FLAG_MEMORY_ALLOCATION_COMPLETE = 0x00000008,
    /*! Memory for ts wasn't immediately available
     */
    FBE_RAID_COMMON_FLAG_DEFERRED_MEMORY_ALLOCATION = 0x00000010,
    /*! We are waiting for the memory available callback
     */
    FBE_RAID_COMMON_FLAG_WAITING_FOR_MEMORY         = 0x00000020,
    /*! The memory request was aborted and may need to be re-tried
     */
    FBE_RAID_COMMON_FLAG_MEMORY_REQUEST_ABORTED     = 0x00000040,
    /*! A memory allocation error occurred
     */
    FBE_RAID_COMMON_FLAG_MEMORY_ALLOCATION_ERROR    = 0x00000080,
    /*! A memory free error (a.k.a. memory leak) occurred
     */
    FBE_RAID_COMMON_FLAG_MEMORY_FREE_ERROR          = 0x00000100,
    /* This flag indicates that the common structure
     * is undergoing or has undergone a retry.
     * For FRUTS,
     * It is undergoing retry if status is still SYS_VD_ERROR.
     * It has undergone retry if status is not SYS_VD_ERROR.
     *
     * SIOTS also use this flag.
     */
    FBE_RAID_COMMON_FLAG_REQ_RETRIED                = 0x00000200,

    /* This bit is set when the item is on a wait queue.
     */
    FBE_RAID_COMMON_FLAG_ON_WAIT_QUEUE              = 0x00000400,

    /* Restart our state, ignore the memory request abort.
     */
    FBE_RAID_COMMON_FLAG_RESTART                    = 0x00000800,
    /* Currently unused flags */
    FBE_RAID_COMMON_FLAG_UNUSED2                    = 0x00001000,
    FBE_RAID_COMMON_FLAG_UNUSED3                    = 0x00002000,
    FBE_RAID_COMMON_FLAG_UNUSED4                    = 0x00004000,
    FBE_RAID_COMMON_FLAG_UNUSED5                    = 0x00008000,
    FBE_RAID_COMMON_FLAG_UNUSED6                    = 0x00010000,
    FBE_RAID_COMMON_FLAG_UNUSED7                    = 0x00020000,
    FBE_RAID_COMMON_FLAG_UNUSED8                    = 0x00040000,
    FBE_RAID_COMMON_FLAG_UNUSED9                    = 0x00080000,

    /* Currently unused types
     */
    FBE_RAID_COMMON_FLAG_TYPE_UNUSED1               = 0x00100000,
    FBE_RAID_COMMON_FLAG_TYPE_UNUSED2               = 0x00200000,
    FBE_RAID_COMMON_FLAG_TYPE_UNUSED3               = 0x00400000,
    FBE_RAID_COMMON_FLAG_TYPE_UNUSED4               = 0x00800000,
    FBE_RAID_COMMON_FLAG_TYPE_UNUSED5               = 0x01000000,

    /* Currently used common structure type flags
     */
    FBE_RAID_COMMON_FLAG_TYPE_VR_TS                 = 0x02000000,
    FBE_RAID_COMMON_FLAG_TYPE_CX_TS                 = 0x04000000,
    FBE_RAID_COMMON_FLAG_TYPE_FRU_TS                = 0x08000000,
    FBE_RAID_COMMON_FLAG_TYPE_NEST_SIOTS            = 0x10000000,
    FBE_RAID_COMMON_FLAG_TYPE_SIOTS                 = 0x20000000,
    FBE_RAID_COMMON_FLAG_TYPE_IOTS                  = 0x40000000,
    FBE_RAID_COMMON_FLAG_MAX                        = 0x80000000,

    /*! @todo update this once the above has stabilized. 
     */
    FBE_RAID_COMMON_FLAG_TYPE_ALL_STRUCT_TYPE_MASK  = 0x7FF00000,

    FBE_RAID_COMMON_FLAG_STRUCT_TYPE_MASK = (FBE_RAID_COMMON_FLAG_TYPE_IOTS       |
                                             FBE_RAID_COMMON_FLAG_TYPE_SIOTS      |
                                             FBE_RAID_COMMON_FLAG_TYPE_NEST_SIOTS |
                                             FBE_RAID_COMMON_FLAG_TYPE_FRU_TS     |
                                             FBE_RAID_COMMON_FLAG_TYPE_CX_TS      |
                                             FBE_RAID_COMMON_FLAG_TYPE_VR_TS),

    FBE_RAID_COMMON_FLAG_SIOTS_NEST_MASK = (FBE_RAID_COMMON_FLAG_TYPE_NEST_SIOTS),
}
fbe_raid_common_flags_t;

/*!************************************************************
 * @def FBE_RAID_COMMON_FLAG_STRINGS
 *
 * @brief  Each string represents a different IOTS flag
 *
 *         FBE_RAID_IOTS_FLAG_STRINGS[i] represents bit 1 << i
 *         where i = 0..N where N is the max bits used.
 *
 * @notes This structure must be kept in sync with the enum above.
 *
 *************************************************************/
#define FBE_RAID_COMMON_FLAG_STRINGS\
 "FBE_RAID_COMMON_FLAG_REQ_STARTED",\
 "FBE_RAID_COMMON_FLAG_REQ_BUSIED",\
 "FBE_RAID_COMMON_FLAG_EMBED_ALLOC",\
 "FBE_RAID_COMMON_FLAG_MEMORY_ALLOCATION_COMPLETE",\
 "FBE_RAID_COMMON_FLAG_DEFERRED_MEMORY_ALLOCATION",\
 "FBE_RAID_COMMON_FLAG_WAITING_FOR_MEMORY",\
 "FBE_RAID_COMMON_FLAG_MEMORY_REQUEST_ABORTED",\
 "FBE_RAID_COMMON_FLAG_MEMORY_ALLOCATION_ERROR",\
 "FBE_RAID_COMMON_FLAG_MEMORY_FREE_ERROR",\
 "FBE_RAID_COMMON_FLAG_REQ_RETRIED",\
 "FBE_RAID_COMMON_FLAG_ON_WAIT_QUEUE",\
 "FBE_RAID_COMMON_FLAG_UNUSED1",\
 "FBE_RAID_COMMON_FLAG_UNUSED2",\
 "FBE_RAID_COMMON_FLAG_UNUSED3",\
 "FBE_RAID_COMMON_FLAG_UNUSED4",\
 "FBE_RAID_COMMON_FLAG_UNUSED5",\
 "FBE_RAID_COMMON_FLAG_UNUSED6",\
 "FBE_RAID_COMMON_FLAG_UNUSED7",\
 "FBE_RAID_COMMON_FLAG_UNUSED8",\
 "FBE_RAID_COMMON_FLAG_UNUSED9",\
 "FBE_RAID_COMMON_FLAG_TYPE_UNUSED1",\
 "FBE_RAID_COMMON_FLAG_TYPE_UNUSED2",\
 "FBE_RAID_COMMON_FLAG_TYPE_UNUSED3",\
 "FBE_RAID_COMMON_FLAG_TYPE_UNUSED4",\
 "FBE_RAID_COMMON_FLAG_TYPE_UNUSED5",\
 "FBE_RAID_COMMON_FLAG_TYPE_VR_TS",\
 "FBE_RAID_COMMON_FLAG_TYPE_CX_TS",\
 "FBE_RAID_COMMON_FLAG_TYPE_FRU_TS",\
 "FBE_RAID_COMMON_FLAG_TYPE_NEST_SIOTS",\
 "FBE_RAID_COMMON_FLAG_TYPE_SIOTS",\
 "FBE_RAID_COMMON_FLAG_TYPE_IOTS",\
 "FBE_RAID_COMMON_FLAG_MAX"

/******************************************************************
 *
 * Raid Driver Common Request Tracking Structure (RAID_COMMON)
 *
 * When any RAID driver tracking structure is defined, the first
 * record consists of this structure.
 * This allows multiple tracking structure types to be
 * used as request ids.   
 *
 ******************************************************************/


typedef struct fbe_raid_common_s
{
    fbe_queue_element_t queue_element;

    fbe_queue_element_t wait_queue_element; /* wait queue */

    fbe_u64_t magic_number;

    void *parent_p;    /* ptr to parent structure */

    fbe_queue_head_t *queue_p; /* ptr to queue this element is on. */

    /* Next state to execute for this request.
     */
    fbe_raid_common_state_t state;

    fbe_raid_common_flags_t flags; /* Common flags valid across all raid group structures. */
}
fbe_raid_common_t;

void fbe_raid_common_init(fbe_raid_common_t *common_p);

static __forceinline void fbe_raid_common_enqueue_tail(fbe_queue_head_t *head,
                                                fbe_raid_common_t *common_p)
{
    common_p->queue_p = head;
    fbe_queue_push(head, &common_p->queue_element); 
}

static __forceinline void fbe_raid_common_queue_remove(fbe_raid_common_t *common_p)
{
    fbe_queue_remove(&common_p->queue_element);
    return;
}

static __forceinline void fbe_raid_common_queue_next(fbe_queue_head_t *head,
                                              fbe_raid_common_t *common_p)
{
    fbe_queue_next(head, &common_p->queue_element);
    return;
}

static __forceinline fbe_raid_common_t *fbe_raid_queue_element_to_common(fbe_queue_element_t *queue_element_p)
{
    return((fbe_raid_common_t *)((fbe_u8_t *)queue_element_p - 
                                           (fbe_u8_t *)(&((fbe_raid_common_t  *)0)->queue_element)));
}


static __forceinline void fbe_raid_common_init_magic_number(fbe_raid_common_t *common_p,
                                              fbe_u64_t magic_number)
{
    common_p->magic_number = magic_number;
    return;
}

static __forceinline void fbe_raid_common_init_flags(fbe_raid_common_t *common_p,
                                              fbe_raid_common_flags_t flags)
{
    common_p->flags = flags;
    return;
}
static __forceinline void fbe_raid_common_set_flag(fbe_raid_common_t * const common_p,
                                            fbe_raid_common_flags_t flag)
{
    common_p->flags = (fbe_raid_common_flags_t)(common_p->flags | flag);
    return;
}
static __forceinline void fbe_raid_common_clear_flag(fbe_raid_common_t * const common_p,
                                              fbe_raid_common_flags_t flag)
{
    common_p->flags = (fbe_raid_common_flags_t)(common_p->flags &~flag);
    return;
}
static __forceinline fbe_raid_common_flags_t fbe_raid_common_get_flags(fbe_raid_common_t * const common_p)
{
    return common_p->flags;
}
static __forceinline fbe_bool_t fbe_raid_common_is_flag_set(fbe_raid_common_t * const common_p,
                                                     fbe_raid_common_flags_t flags)
{
    return ((common_p->flags & flags) == flags);
}
static __forceinline void fbe_raid_common_set_state(fbe_raid_common_t *common_p,
                                            fbe_raid_common_state_t state)
{
    common_p->state = state;
    return;
}
static __forceinline fbe_raid_common_state_t fbe_raid_common_get_state(fbe_raid_common_t *common_p)
{
    return common_p->state;
}

static __forceinline void fbe_raid_common_set_parent(fbe_raid_common_t *common_p,
                                              fbe_raid_common_t *parent_p)
{
    common_p->parent_p = parent_p;
    return;
}
static __forceinline fbe_raid_common_t *  fbe_raid_common_get_parent(fbe_raid_common_t *common_p)
{
    return (fbe_raid_common_t *)common_p->parent_p;
}
static __forceinline fbe_raid_common_t *  fbe_raid_common_get_next(fbe_raid_common_t *common_p)
{
    if ((common_p == NULL) || (common_p->queue_p == NULL))
    {
        return NULL;
    }
    return (fbe_raid_common_t*)fbe_queue_next(common_p->queue_p, &common_p->queue_element);
}
static __forceinline fbe_raid_common_t *  fbe_raid_common_get_prev(fbe_raid_common_t *common_p)
{
    if ((common_p == NULL) || (common_p->queue_p == NULL))
    {
        return NULL;
    }
    return (fbe_raid_common_t*)fbe_queue_prev(common_p->queue_p, &common_p->queue_element);
}

/*!***************************************************************
 * fbe_raid_common_wait_queue_element_to_common()
 ****************************************************************
 * @brief
 *  Convert from a queue element back to the fbe_logical_drive_io_cmd_t.
 *
 * @param queue_element_p - This is the queue element that we wish to
 *                          get the io cmd for.
 *
 * @return fbe_logical_drive_io_cmd_t - The io cmd object that
 *                                      has the input queue element.
 *
 * @author
 *  11/05/07 - Created. RPF
 *
 ****************************************************************/
static __inline fbe_raid_common_t * 
fbe_raid_common_wait_queue_element_to_common(fbe_queue_element_t * queue_element_p)
{
    /* We're converting an address to a queue element into an address to the containing
     * raid common. In order to do this we need to subtract the offset to the queue 
     * element from the address of the queue element. 
     */
    fbe_raid_common_t * raid_common_p;
    raid_common_p = (fbe_raid_common_t  *)((fbe_u8_t *)queue_element_p - 
                                           (fbe_u8_t *)(&((fbe_raid_common_t  *)0)->wait_queue_element));
    return raid_common_p;
}
/**************************************
 * end fbe_raid_common_wait_queue_element_to_common()
 **************************************/

static __forceinline void fbe_raid_common_wait_enqueue_tail(fbe_queue_head_t *head,
                                                     fbe_raid_common_t *common_p)
{
    if (fbe_queue_is_element_on_queue(&common_p->wait_queue_element))
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_RAID, 
                               FBE_TRACE_LEVEL_CRITICAL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_INFO,
                               (fbe_char_t *)"raid_common_wait_enqueue_tail: common %llx already on the queue\n", 
                               CSX_CAST_PTR_TO_PTRMAX(common_p));
    }
    /* We do not set the queue_p in the common since it is not used by the wait queue 
     * element. 
     */
    fbe_queue_push(head, &common_p->wait_queue_element); 
    return;
}

static __forceinline fbe_raid_common_t* fbe_raid_common_wait_queue_pop(fbe_queue_head_t *head)
{
    fbe_queue_element_t *wait_queue_element_p = NULL;
    fbe_raid_common_t *common_p = NULL;

    /* We do not set the queue_p in the common since it is not used by the wait queue 
     * element. 
     */
    wait_queue_element_p = (fbe_queue_element_t *)fbe_queue_pop(head);
    common_p = fbe_raid_common_wait_queue_element_to_common(wait_queue_element_p);
    return common_p;
}

/*!*******************************************************************
 * @enum fbe_raid_common_error_precedence_t
 *********************************************************************
 * @brief
 *   This is a in-line function to start/restart the IOTS/SIOTS...
 *   We need this as in-line so that it can be accessed from transport
 *   and raid_library. 
 *
 *********************************************************************/
static __forceinline fbe_status_t fbe_raid_common_start(fbe_raid_common_t *common_request_p)
{
    fbe_raid_state_status_t status = FBE_RAID_STATE_STATUS_INVALID;
    fbe_raid_common_state_t prev_state = NULL;
    fbe_u32_t repeat_count = 0;

    while (FBE_RAID_STATE_STATUS_EXECUTING == (status = common_request_p->state(common_request_p)))
    {
        /* Continue to next state.
         * Make sure the status is sane. 
         */
        if ((status == FBE_RAID_STATE_STATUS_INVALID) ||
            (status >= FBE_RAID_STATE_STATUS_LAST))
        {
            fbe_base_library_trace(FBE_LIBRARY_ID_RAID, 
                                   FBE_TRACE_LEVEL_ERROR, 
                                   FBE_TRACE_MESSAGE_ID_INFO,
                                   (fbe_char_t *)"raid_common_start: Status unexpected: 0x%x\n", 
                                   status);

            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* Make sure we are not stuck in an infinite loop. */
        if (prev_state == common_request_p->state)
        {
            repeat_count++;
            if (repeat_count > 0xFFFF)
            {
                fbe_base_library_trace(FBE_LIBRARY_ID_RAID, 
                                       FBE_TRACE_LEVEL_CRITICAL_ERROR, 
                                       FBE_TRACE_MESSAGE_ID_INFO,
                                       (fbe_char_t *)"raid_common_start: repeat_count: 0x%x common_p: 0x%p state: 0x%p\n", 
                                       repeat_count, 
                                       common_request_p, 
                                       common_request_p->state);
            }
        }
        else
        {
            prev_state = common_request_p->state;
            repeat_count = 0;
        }
    }

    return FBE_STATUS_OK;
}


/*!*******************************************************************
 * @enum fbe_raid_common_error_precedence_t
 *********************************************************************
 * @brief
 *   Enumeration of the different error precedence.  This determines
 *   which error is reported for a multi-error I/O.  They are enumerated
 *   from lowest to highest weight.
 *
 *********************************************************************/
typedef enum fbe_raid_common_error_precedence_e
{
    FBE_RAID_COMMON_ERROR_PRECEDENCE_NO_ERROR          =   0,
    FBE_RAID_COMMON_ERROR_PRECEDENCE_STILL_CONGESTED   =   5,
    FBE_RAID_COMMON_ERROR_PRECEDENCE_INVALID_REQUEST   =  10,
    FBE_RAID_COMMON_ERROR_PRECEDENCE_MEDIA_ERROR       =  20,
    FBE_RAID_COMMON_ERROR_PRECEDENCE_NOT_READY         =  30,
    FBE_RAID_COMMON_ERROR_PRECEDENCE_ABORTED           =  40,
    FBE_RAID_COMMON_ERROR_PRECEDENCE_TIMEOUT           =  50,
    FBE_RAID_COMMON_ERROR_PRECEDENCE_DEVICE_DEAD       =  60,
    FBE_RAID_COMMON_ERROR_PRECEDENCE_CONGESTION        =  70,
    FBE_RAID_COMMON_ERROR_PRECEDENCE_UNKNOWN           = 100,
} fbe_raid_common_error_precedence_t;

#endif
/*************************
 * end file fbe_raid_common.h
 *************************/
