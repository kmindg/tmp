#ifndef FBE_RAID_SIOTS_ACCESSOR_H
#define FBE_RAID_SIOTS_ACCESSOR_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_raid_siots_accessor.h
 ***************************************************************************
 *
 * @brief
 *  This file contains accessor methods for the siots.
 *
 * @version
 *   5/19/2009:  Created. rfoley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "csx_ext.h"
#include "fbe_raid_library_private.h"
#include "fbe_raid_algorithm.h"
#include "fbe_raid_fruts.h"
#include "fbe_raid_library.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/
extern void fbe_raid_siots_trace(fbe_raid_siots_t *const siots_p,
                                 fbe_trace_level_t trace_level,
                                 fbe_u32_t line,
                                 const char *file_p,
                                 fbe_char_t * fail_string_p, ...)
                                 __attribute__((format(__printf_func__,5,6)));
fbe_bool_t fbe_raid_siots_is_startable(fbe_raid_siots_t *const siots_p);

extern fbe_u32_t fbe_transport_get_coarse_time(void);

/*!*******************************************************************
 * @enum fbe_raid_siots_flags_t
 *********************************************************************
 * @brief
 *  This defines all the valid flags for a siots.
 *
 * IMPORTANT !!
 *  When you add or change algorithms,
 *  you need to update fbe_raid_debug_siots_flags_trace_info
 *
 *
 *********************************************************************/
typedef enum fbe_raid_siots_flags_e 
{
    FBE_RAID_SIOTS_FLAG_INVALID                    = 0x00000000,
    FBE_RAID_SIOTS_FLAG_EMBEDDED_SIOTS             = 0x00000001,
    FBE_RAID_SIOTS_FLAG_WAIT_LOCK                  = 0x00000002,
    FBE_RAID_SIOTS_FLAG_WAS_DELAYED                = 0x00000004,
    /*! the upgrade code is not used.
     */
    FBE_RAID_SIOTS_FLAG_WAIT_UPGRADE               = 0x00000008,
    FBE_RAID_SIOTS_FLAG_DONE_GENERATING            = 0x00000010,
    FBE_RAID_SIOTS_FLAG_UNUSED_0                   = 0x00000020,
    FBE_RAID_SIOTS_FLAG_QUIESCED                   = 0x00000040,
    FBE_RAID_SIOTS_FLAG_ERROR_PENDING              = 0x00000080,
    FBE_RAID_SIOTS_FLAG_ONE_SIOTS                  = 0x00000100,
    FBE_RAID_SIOTS_FLAG_SINGLE_STRIP_MODE          = 0x00000200,
    FBE_RAID_SIOTS_FLAG_UNUSED_1                   = 0x00000400,
    FBE_RAID_SIOTS_FLAG_WAITING_SHUTDOWN_CONTINUE  = 0x00000800,
    FBE_RAID_SIOTS_FLAG_RECEIVED_CONTINUE          = 0x00001000,
    /* In this case a "region" is defined as a sequence of 
     * blocks.  In some cases it is the optimum block size. 
     * The region size is defined by fbe_raid_siots_get_region_size() 
     */
    FBE_RAID_SIOTS_FLAG_SINGLE_REGION_MODE         = 0x00002000,
    FBE_RAID_SIOTS_FLAG_SINGLE_ERROR_RECOVERY      = 0x00004000,
    FBE_RAID_SIOTS_FLAG_UNUSED_2                   = 0x00008000,
    FBE_RAID_SIOTS_FLAG_ERROR_INJECTED             = 0x00010000,
    FBE_RAID_SIOTS_FLAG_WROTE_CORRECTIONS          = 0x00020000,
    FBE_RAID_SIOTS_FLAG_WRITE_STARTED              = 0x00040000,
    FBE_RAID_SIOTS_FLAG_UNUSED_3                   = 0x00080000,
    FBE_RAID_SIOTS_FLAG_UNUSED_4                   = 0x00100000,
    FBE_RAID_SIOTS_FLAG_UNUSED_5                   = 0x00200000,
    FBE_RAID_SIOTS_FLAG_WAIT_UPGRADE_LOCK          = 0x00400000,
    FBE_RAID_SIOTS_FLAG_UPGRADE_OWNER              = 0x00800000,
	FBE_RAID_SIOTS_FLAG_JOURNAL_SLOT_ALLOCATED     = 0x01000000,
    FBE_RAID_SIOTS_FLAG_COMPLETE_IMMEDIATE         = 0x02000000,
    FBE_RAID_SIOTS_FLAG_LOGGING_ENABLED            = 0x04000000,
    FBE_RAID_SIOTS_FLAG_WAIT_WRITE_LOG_SLOT        = 0x08000000,
    FBE_RAID_SIOTS_FLAG_WRITE_LOG_HEADER_REQD      = 0x10000000,

     /* IMPORTANT !!
      *  When you add or change algorithms,
      *  you need to update fbe_raid_debug_siots_flags_trace_info
      */
    FBE_RAID_SIOTS_FLAG_LAST                       = 0x20000000
}
fbe_raid_siots_flags_t;

/*!************************************************************
 * @def FBE_RAID_SIOTS_FLAG_STRINGS
 *
 * @brief  Each string represents a different SIOTS flag
 *
 *               FBE_RAID_SIOTS_FLAG_STRINGS[i] represents
 *               bit 1 << i where i = 0..N where N is the max bits used.
 *
 * @notes This structure must be kept in sync with the enum above.
 *
 *************************************************************/
#define FBE_RAID_SIOTS_FLAG_STRINGS\
 "FBE_RAID_SIOTS_FLAG_EMBEDDED_SIOTS",\
 "FBE_RAID_SIOTS_FLAG_WAIT_LOCK",\
 "FBE_RAID_SIOTS_FLAG_WAS_DELAYED",\
 "FBE_RAID_SIOTS_FLAG_WAIT_UPGRADE",\
 "FBE_RAID_SIOTS_FLAG_DONE_GENERATING",\
 "FBE_RAID_SIOTS_FLAG_USE_PARENT_SG",\
 "FBE_RAID_SIOTS_FLAG_QUIESCED",\
 "FBE_RAID_SIOTS_FLAG_ERROR_PENDING",\
 "FBE_RAID_SIOTS_FLAG_DEGRADED",\
 "FBE_RAID_SIOTS_FLAG_SINGLE_STRIP_MODE",\
 "FBE_RAID_SIOTS_FLAG_SHED_FULL",\
 "FBE_RAID_SIOTS_FLAG_WAITING_SHUTDOWN_CONTINUE",\
 "FBE_RAID_SIOTS_FLAG_RECEIVED_CONTINUE",\
 "FBE_RAID_SIOTS_FLAG_SINGLE_REGION_MODE",\
 "FBE_RAID_SIOTS_FLAG_SINGLE_ERROR_RECOVERY",\
 "FBE_RAID_SIOTS_FLAG_SENT_REMAP",\
 "FBE_RAID_SIOTS_FLAG_ERROR_INJECTED",\
 "FBE_RAID_SIOTS_FLAG_WROTE_CORRECTIONS",\
 "FBE_RAID_SIOTS_FLAG_WAITING_REBUILD_LOGGER",\
 "FBE_RAID_SIOTS_FLAG_PROACTIVE_SPARING",\
 "FBE_RAID_SIOTS_FLAG_CHECK_UNALIGNED",\
 "FBE_RAID_SIOTS_FLAG_UNALIGNED_REQUEST",\
 "FBE_RAID_SIOTS_FLAG_WAIT_UPGRADE_LOCK",\
 "FBE_RAID_SIOTS_FLAG_UPGRADE_OWNER",\
 "FBE_RAID_SIOTS_FLAG_JOURNAL_SLOT_ALLOCATED",\
 "FBE_RAID_SIOTS_FLAG_COMPLETE_IMMEDIATE",\
 "FBE_RAID_SIOTS_FLAG_LOGGING_ENABLED",\
 "FBE_RAID_SIOTS_FLAG_WAIT_WRITE_LOG_SLOT",\
 "FBE_RAID_SIOTS_FLAG_WRITE_LOG_HEADER_REQD",\
 "FBE_RAID_SIOTS_FLAG_DEFINE_NEXT_FLAG_HERE"

/*!**************************************************************
 * fbe_raid_siots_get_iots()
 ****************************************************************
 * @brief
 *  Get the parent iots from a siots.
 *
 * @param siots_p - siots to get iots parent for. 
 *
 * @return fbe_raid_iots_t*
 *
 * @author
 *  5/19/2009 - Created. rfoley
 *
 ****************************************************************/
#define fbe_raid_siots_get_iots(m_siots_p)\
(((m_siots_p)->common.flags & FBE_RAID_COMMON_FLAG_TYPE_SIOTS) \
            ? ((fbe_raid_iots_t*)fbe_raid_common_get_parent(&(m_siots_p)->common)) \
            : ((fbe_raid_iots_t*)(((fbe_raid_common_t*)(m_siots_p)->common.parent_p)->parent_p)) )
#if 0
static __forceinline fbe_raid_iots_t* fbe_raid_siots_get_iots(fbe_raid_siots_t *const siots_p)
{
    return ((siots_p->common.flags & FBE_RAID_COMMON_FLAG_TYPE_SIOTS) 
            ? ((fbe_raid_iots_t*)fbe_raid_common_get_parent(&siots_p->common)) 
            : (((fbe_raid_common_t*)siots_p->common.parent_p)->parent_p) );
    if (siots_p->common.flags & FBE_RAID_COMMON_FLAG_TYPE_SIOTS)
    {
        return (fbe_raid_iots_t*)fbe_raid_common_get_parent(&siots_p->common);
    }
    else
    {
        /* Otherwise we're nested.
         */
        fbe_raid_common_t *common_p = NULL;
        common_p = fbe_raid_common_get_parent(&siots_p->common);

        return (fbe_raid_iots_t*)fbe_raid_common_get_parent(common_p);
    }
    
    fbe_raid_common_flags_t flags = fbe_raid_common_get_flags(&siots_p->common);
    fbe_raid_common_t *common_p = NULL;

    common_p = fbe_raid_common_get_parent(&siots_p->common);

    if(RAID_ERROR_COND(!(flags & FBE_RAID_COMMON_FLAG_TYPE_SIOTS) &&
                 !(flags & FBE_RAID_COMMON_FLAG_TYPE_NEST_SIOTS)))
    {
        fbe_raid_siots_trace(siots_p,
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "siots_p 0x%p flags (0x%x) must be set to FBE_RAID_COMMON_FLAG_TYPE_SIOTS or FBE_RAID_COMMON_FLAG_TYPE_NEST_SIOTS\n",
                             siots_p, 
                             flags);
        return NULL;
    }

    if(RAID_ERROR_COND(common_p == NULL))
    {
        fbe_raid_siots_trace(siots_p,
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "siots_p 0x%p parent ptr is NULL\n",
                             siots_p);
        return NULL;
    }

    if (flags & FBE_RAID_COMMON_FLAG_TYPE_SIOTS)
    {
        common_p = fbe_raid_common_get_parent(&siots_p->common);
    }
    else
    {
        /* We are nested, so the parent should be a siots. 
         */
        flags = fbe_raid_common_get_flags(common_p);

        if(RAID_ERROR_COND(!(flags & FBE_RAID_COMMON_FLAG_TYPE_SIOTS)))
        {
            fbe_raid_siots_trace(siots_p,
                                 FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "siots_p 0x%p is nested but parent is not siots."
                                 "Parent 0x%p flag 0x%x is not set to FBE_RAID_COMMON_FLAG_TYPE_SIOTS\n",
                                 siots_p,
                                 common_p,
                                 flags);
            return NULL;
        }

        /* The parent of this is an iots.
         */
        common_p = fbe_raid_common_get_parent(common_p);
    }
    /* This should be an iots.
     */
    flags = fbe_raid_common_get_flags(common_p);

    if(RAID_ERROR_COND(!(flags & FBE_RAID_COMMON_FLAG_TYPE_IOTS)))
    {
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "siots_p 0x%p ancestor 0x%p is not an iots, "
                             "ancestor's flag 0x%x is not set to FBE_RAID_COMMON_FLAG_TYPE_IOTS\n",
                             siots_p,
                             common_p,
                             flags);
        return NULL;
    }

    /* Cast this common back to an iots.
     */
    return (fbe_raid_iots_t *)common_p;
}
#endif
/******************************************
 * end fbe_raid_siots_get_iots()
 ******************************************/
#if 0 /* Lockless siots */
static __forceinline void fbe_raid_siots_lock(fbe_raid_siots_t *siots_p)
{
    //fbe_spinlock_lock(&siots_p->lock);
    return;
}
static __forceinline void fbe_raid_siots_unlock(fbe_raid_siots_t *siots_p)
{
    //fbe_spinlock_unlock(&siots_p->lock);
    return;
}
#endif

static __forceinline void fbe_raid_siots_get_nest_queue_head(fbe_raid_siots_t *siots_p,
                                                      fbe_raid_siots_t **nsiots_p)
{
    *nsiots_p = (fbe_raid_siots_t*)fbe_queue_front(&siots_p->nsiots_q);
    return;
}

static __forceinline fbe_raid_geometry_t *fbe_raid_siots_get_raid_geometry(fbe_raid_siots_t * const siots_p)
{
    return(fbe_raid_iots_get_raid_geometry(fbe_raid_siots_get_iots(siots_p)));
}
static __forceinline void fbe_raid_siots_get_edge(fbe_raid_siots_t *siots_p,
                                           fbe_block_edge_t **edge_pp)
{
    fbe_raid_iots_get_edge(fbe_raid_siots_get_iots(siots_p), edge_pp);
    return;
}

static __inline fbe_bool_t fbe_raid_siots_is_nested(fbe_raid_siots_t *siots_p)
{
    return(fbe_raid_common_is_flag_set(&siots_p->common,
                                       FBE_RAID_COMMON_FLAG_TYPE_NEST_SIOTS));
}
static __inline fbe_bool_t fbe_raid_siots_is_csum_retried(fbe_raid_siots_t *siots_p)
{
    return(fbe_raid_common_is_flag_set(&siots_p->common,
                                       FBE_RAID_COMMON_FLAG_REQ_RETRIED));
}
static __inline fbe_bool_t fbe_raid_siots_is_crc_error_reconstruct(fbe_raid_siots_t *siots_p)
{
    /*! Not supported for now. 
     */
    return(FBE_FALSE);
}

static __inline fbe_raid_siots_t * fbe_raid_siots_get_next(fbe_raid_siots_t *siots_p)
{
    return ((fbe_raid_siots_t*)fbe_raid_common_get_next(&siots_p->common));
}
static __inline fbe_raid_siots_t * fbe_raid_siots_get_prev(fbe_raid_siots_t *siots_p)
{
    return ((fbe_raid_siots_t*)fbe_raid_common_get_prev(&siots_p->common));
}
static __forceinline void fbe_raid_siots_get_priority(fbe_raid_siots_t *const siots_p,
                                               fbe_packet_priority_t *priority_p)
{
    fbe_raid_iots_get_priority(fbe_raid_siots_get_iots(siots_p), priority_p);
    return;
}
static __forceinline void fbe_raid_siots_set_priority(fbe_raid_siots_t *const siots_p,
                                               fbe_packet_priority_t priority)
{
    fbe_raid_iots_set_priority(fbe_raid_siots_get_iots(siots_p), priority);
    return;
}
static __forceinline void fbe_raid_siots_get_opcode(fbe_raid_siots_t *const siots_p,
                                             fbe_payload_block_operation_opcode_t *opcode_p)
{
    fbe_raid_iots_get_current_opcode(fbe_raid_siots_get_iots(siots_p), opcode_p);
    return;
}

static __forceinline void fbe_raid_siots_get_lba(fbe_raid_siots_t *const siots_p,
                                         fbe_lba_t *lba_p)
{
    fbe_raid_iots_get_current_op_lba(fbe_raid_siots_get_iots(siots_p), lba_p);
    return;
}

static __forceinline void fbe_raid_siots_get_blocks(fbe_raid_siots_t *const siots_p,
                                            fbe_block_count_t *blocks_p)
{
    fbe_raid_iots_get_current_op_blocks(fbe_raid_siots_get_iots(siots_p), blocks_p);
    return;
}

static __forceinline void fbe_raid_siots_get_sg_ptr(fbe_raid_siots_t *const siots_p,
                                            fbe_sg_element_t **sg_p)
{
    fbe_raid_iots_get_sg_ptr(fbe_raid_siots_get_iots(siots_p), sg_p);
    return;
}

/* Accessors for flags.
 */
static __forceinline void fbe_raid_siots_init_flags(fbe_raid_siots_t *siots_p,
                                             fbe_raid_siots_flags_t flags)
{
    siots_p->flags = flags;
    return;
}

static __forceinline void fbe_raid_siots_set_flag(fbe_raid_siots_t *siots_p,
                                           fbe_raid_siots_flags_t flag)
{
    siots_p->flags |= flag;
    return;
}

static __forceinline void fbe_raid_siots_clear_flag(fbe_raid_siots_t *siots_p,
                                             fbe_raid_siots_flags_t flag)
{
    siots_p->flags &= ~flag;
    return;
}

static __forceinline fbe_bool_t fbe_raid_siots_is_flag_set(fbe_raid_siots_t *siots_p,
                                                    fbe_raid_siots_flags_t flags)
{
    return ((siots_p->flags & flags) == flags);
}

static __forceinline fbe_bool_t fbe_raid_siots_is_any_flag_set(fbe_raid_siots_t *siots_p,
                                                    fbe_raid_siots_flags_t flags)
{
    return ((siots_p->flags & flags) != 0);
}

/* Accessors for nested siots
 */
static __forceinline void fbe_raid_siots_set_nested_siots(fbe_raid_siots_t *siots_p,
                                                          fbe_raid_siots_t *nested_siots_p)
{
    siots_p->nested_siots_ptr = nested_siots_p;
    return;
}

static __forceinline fbe_raid_siots_t *fbe_raid_siots_get_nested_siots(fbe_raid_siots_t *siots_p)
{
    return(siots_p->nested_siots_ptr);
}

/* Accessors for error_validation_callback.
 */
static __forceinline void fbe_raid_siots_get_error_validation_callback(fbe_raid_siots_t *siots_p, 
                                                                fbe_raid_siots_error_validation_callback_t *error_validation_callback_p) 
{
    *error_validation_callback_p = siots_p->error_validation_callback;
    return;
}
static __forceinline void fbe_raid_siots_set_error_validation_callback(fbe_raid_siots_t *siots_p, 
                                                                fbe_raid_siots_error_validation_callback_t error_validation_callback) 
{
    siots_p->error_validation_callback = error_validation_callback;
    return;
}

static __forceinline fbe_raid_siots_flags_t fbe_raid_siots_get_flags(fbe_raid_siots_t *siots_p)
{
    return siots_p->flags;
}

/* Accessors for fruts.
 */
static __forceinline void fbe_raid_siots_get_read_fruts_head(fbe_raid_siots_t *siots_p,
                                                      fbe_queue_head_t **fruts_pp)
{
    *fruts_pp = &siots_p->read_fruts_head;
    return;
}

/* Accessors for embedded eboard
 */
static __forceinline fbe_xor_error_t *fbe_raid_siots_get_eboard(fbe_raid_siots_t *siots_p) 
{
    return(&siots_p->misc_ts.cxts.eboard);
}
static __forceinline fbe_lba_t fbe_raid_siots_get_eboard_offset(fbe_raid_siots_t *siots_p) 
{
    return(fbe_xor_error_get_offset(fbe_raid_siots_get_eboard(siots_p)));
}

/* Get the head element from the read fruts chain.
 */
static __forceinline void fbe_raid_siots_get_read_fruts(fbe_raid_siots_t *siots_p,
                                                 fbe_raid_fruts_t **fruts_pp)
{
    *fruts_pp = (fbe_raid_fruts_t *)fbe_queue_front(&siots_p->read_fruts_head);
    return;
}
/* Remove the element from the read fruts chain.
 * (just a wrapper for clarity)
 */
static __forceinline fbe_status_t  fbe_raid_siots_remove_read_fruts(fbe_raid_siots_t *siots_p,
                                                    fbe_raid_fruts_t *fruts_p)
{
    fbe_status_t status = FBE_STATUS_OK;

    if(RAID_COND(fbe_raid_common_get_parent((fbe_raid_common_t *)fruts_p) !=
                 (fbe_raid_common_t *)siots_p))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "SIOTS: %p: not matching with FRUTS %p parent\n",
                             siots_p, fruts_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_raid_common_queue_remove(&fruts_p->common);
    return status;
}

static __forceinline fbe_status_t  fbe_raid_siots_remove_journal_fruts(fbe_raid_siots_t *siots_p,
                                                    fbe_raid_fruts_t *fruts_p)
{
    fbe_status_t status = FBE_STATUS_OK;

    if(RAID_COND(fbe_raid_common_get_parent((fbe_raid_common_t *)fruts_p) !=
                 (fbe_raid_common_t *)siots_p))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "SIOTS: %p: not matching with FRUTS %p parent\n",
                             siots_p, fruts_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_raid_common_queue_remove(&fruts_p->common);
    return status;
}


/* Insert to the tail of read fruts chain.
 */
static __forceinline void fbe_raid_siots_enqueue_tail_read_fruts(fbe_raid_siots_t *siots_p,
                                                          fbe_raid_fruts_t *fruts_p)
{
    fbe_raid_common_enqueue_tail(&siots_p->read_fruts_head, 
                                 (fbe_raid_common_t *)fruts_p);
    return;
}

/* Get the head element from the read2 fruts chain.
 */
static __forceinline void fbe_raid_siots_get_read2_fruts(fbe_raid_siots_t *siots_p,
                                                 fbe_raid_fruts_t **fruts_pp)
{
    *fruts_pp = (fbe_raid_fruts_t *)fbe_queue_front(&siots_p->read2_fruts_head);
    return;
}
/* Remove the element from the read fruts chain.
 * (just a wrapper for clarity)
 */
static __forceinline fbe_status_t fbe_raid_siots_remove_read2_fruts(fbe_raid_siots_t *siots_p,
                                                    fbe_raid_fruts_t *fruts_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    if(RAID_COND(fbe_raid_common_get_parent((fbe_raid_common_t *)fruts_p) !=
                 (fbe_raid_common_t *)siots_p))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "SIOTS: %p not matching with FRUTS %p parent\n",
                             siots_p, fruts_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_raid_common_queue_remove(&fruts_p->common);
    return status;
}
/* Insert to the tail of read2 fruts chain.
 */
static __forceinline void fbe_raid_siots_enqueue_tail_read2_fruts(fbe_raid_siots_t *siots_p,
                                                          fbe_raid_fruts_t *fruts_p)
{
    fbe_raid_common_enqueue_tail(&siots_p->read2_fruts_head, 
                                 (fbe_raid_common_t *)fruts_p);
    return;
}

static __forceinline void fbe_raid_siots_get_write_fruts_head(fbe_raid_siots_t *siots_p,
                                                      fbe_queue_head_t **fruts_pp)
{
    *fruts_pp = &siots_p->write_fruts_head;
    return;
}

/* Get the head element from the write fruts chain.
 */
static __forceinline void fbe_raid_siots_get_write_fruts(fbe_raid_siots_t *siots_p,
                                                 fbe_raid_fruts_t **fruts_pp)
{
    *fruts_pp = (fbe_raid_fruts_t *)fbe_queue_front(&siots_p->write_fruts_head);
    return;
}

/* Remove the element from the write fruts chain.
 */
static __forceinline fbe_status_t fbe_raid_siots_remove_write_fruts(fbe_raid_siots_t *siots_p,
                                                     fbe_raid_fruts_t *fruts_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    if(RAID_COND(fbe_raid_common_get_parent((fbe_raid_common_t *)fruts_p) !=
                 (fbe_raid_common_t *)siots_p))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "SIOTS: %p not matching with FRUTS %p parent\n",
                             siots_p, fruts_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_raid_common_queue_remove(&fruts_p->common);
    return status;
}
/* Insert to the tail of write fruts chain.
 */
static __forceinline void fbe_raid_siots_enqueue_tail_write_fruts(fbe_raid_siots_t *siots_p,
                                                           fbe_raid_fruts_t *fruts_p)
{
    fbe_raid_common_enqueue_tail(&siots_p->write_fruts_head, 
                                 (fbe_raid_common_t *)fruts_p);
    return;
}

/* Get the verify tracking structure pointer
 */
static __forceinline void fbe_raid_siots_get_vrts(fbe_raid_siots_t *siots_p,
                                           fbe_raid_vrts_t **vrts_pp)
{
    *vrts_pp = siots_p->vrts_ptr;
    return;
}
/* Accessors for page_size.
 */
static __forceinline void fbe_raid_siots_get_page_size( fbe_raid_siots_t  * siots_p, fbe_u32_t * page_size_p ) 
{
    *page_size_p = siots_p->ctrl_page_size;
    return;
}
static __forceinline void fbe_raid_siots_set_page_size( fbe_raid_siots_t * siots_p, fbe_u32_t page_size ) 
{
    siots_p->ctrl_page_size = page_size;
    return;
}
/* Accessors for page_size.
 */
static __forceinline void fbe_raid_siots_get_data_page_size( fbe_raid_siots_t  * siots_p, fbe_u32_t * page_size_p ) 
{
    *page_size_p = siots_p->data_page_size;
    return;
}
static __forceinline void fbe_raid_siots_set_data_page_size( fbe_raid_siots_t * siots_p, fbe_u32_t page_size ) 
{
    siots_p->data_page_size = page_size;
    return;
}
/* Accessors for total_blocks_to_allocate.
 */
static __forceinline void fbe_raid_siots_get_total_blocks_to_allocate(fbe_raid_siots_t *siots_p, fbe_block_count_t *total_blocks_to_allocate_p) 
{
    *total_blocks_to_allocate_p = siots_p->total_blocks_to_allocate;
    return;
}
static __forceinline void fbe_raid_siots_set_total_blocks_to_allocate(fbe_raid_siots_t *siots_p, fbe_u32_t total_blocks_to_allocate) 
{
    siots_p->total_blocks_to_allocate = total_blocks_to_allocate;
    return;
}
/* Get the ptr to the xor command.
 */
static __forceinline void fbe_raid_siots_get_xor_command_status(fbe_raid_siots_t *siots_p,
                                                  fbe_xor_status_t *status_p)
{
    *status_p = siots_p->misc_ts.xor_status;
    return;
}
static __forceinline void fbe_raid_siots_set_xor_command_status(fbe_raid_siots_t *siots_p,
                                                        fbe_xor_status_t status)
{
    siots_p->misc_ts.xor_status = status;
    return;
}

/* Get the block operation flags.
 */
static __forceinline void fbe_raid_siots_get_block_flags(fbe_raid_siots_t *const siots_p,
                                                 fbe_payload_block_operation_flags_t *block_operation_flags_p)
{
    fbe_raid_iots_t    *iots_p = fbe_raid_siots_get_iots(siots_p);

    fbe_raid_iots_get_block_flags(iots_p, block_operation_flags_p);
    return;
}

/* Get the status this siots should return.
 */
static __forceinline void fbe_raid_siots_get_block_status(fbe_raid_siots_t *const siots_p,
                                                  fbe_payload_block_operation_status_t *status_p)
{
    *status_p = siots_p->error;
    return;
}
/* Set the block status this siots should return.
 */
static __forceinline void fbe_raid_siots_set_block_status(fbe_raid_siots_t *const siots_p,
                                                  fbe_payload_block_operation_status_t status)
{
    siots_p->error = status;
    return;
}

/* Get the media error lba this siots should return.
 */
static __forceinline void fbe_raid_siots_get_media_error_lba(fbe_raid_siots_t *const siots_p,
                                                      fbe_lba_t *lba_p)
{
    *lba_p = siots_p->media_error_lba;
    return;
}
/* Set the block status this siots should return.
 */
static __forceinline void fbe_raid_siots_set_media_error_lba(fbe_raid_siots_t *const siots_p,
                                                      fbe_lba_t lba)
{
    siots_p->media_error_lba = lba;
    return;
}
/*!********************************************************
 * fbe_raid_siots_is_retry
 *********************************************************
 * @brief The Raid SIOTS uses the Raid common's RAID_REQ_RETRIED flag
 * to determine if a csum error retry has occurred.
 *********************************************************/
static __inline fbe_bool_t fbe_raid_siots_is_retry(fbe_raid_siots_t *siots_p)
{
    return fbe_raid_common_is_flag_set(&siots_p->common, FBE_RAID_COMMON_FLAG_REQ_RETRIED);
}

/* Get the block qualifier this siots should return.
 */
static __forceinline void fbe_raid_siots_get_block_qualifier(fbe_raid_siots_t *const siots_p,
                                                     fbe_payload_block_operation_qualifier_t *status_p)
{
    *status_p = siots_p->qualifier;
    return;
}
/* Set the qualifier this siots should return.
 */
static __forceinline void fbe_raid_siots_set_block_qualifier(fbe_raid_siots_t *const siots_p,
                                                     fbe_payload_block_operation_qualifier_t status)
{
    siots_p->qualifier = status;
    return;
}
static __forceinline fbe_raid_siots_t *fbe_raid_siots_get_parent(fbe_raid_siots_t *siots_p)
{
    fbe_raid_common_t *common_p = fbe_raid_common_get_parent(&siots_p->common);

    if (!fbe_raid_common_is_flag_set(common_p, FBE_RAID_COMMON_FLAG_TYPE_SIOTS))
    {
        /* Parent is not a siots so do not return it.
         */
        common_p = NULL;
    }
    return (fbe_raid_siots_t*) common_p;
}

/* Set the status this siots should return.
 */
static __forceinline void fbe_raid_siots_set_status(fbe_raid_siots_t *const siots_p,
                                             fbe_payload_block_operation_status_t status,
                                             fbe_payload_block_operation_qualifier_t status_qualifier)
{
    siots_p->error = status;
    siots_p->qualifier = status_qualifier;
    return;
}

/*****************************************
 * fbe_raid_siots_transition
 *
 * Transition this siots to another state.
 * Under debug we will squirrel away
 * the previous states in a stack of
 * states.
 *****************************************/
typedef fbe_raid_state_status_t(*fbe_raid_siots_state_t) (struct fbe_raid_siots_s *);
static __forceinline void fbe_raid_siots_transition(fbe_raid_siots_t *siots_p, fbe_raid_siots_state_t state)
{
    siots_p->prevStateStamp[siots_p->state_index] = fbe_transport_get_coarse_time();
    siots_p->prevState[siots_p->state_index++] = siots_p->common.state;

    /* Although we are casting, we at least have the type check above, that this is a 
     * siots state. 
     */
    fbe_raid_common_set_state(&siots_p->common, (fbe_raid_common_state_t) state);
    if (siots_p->state_index >= FBE_RAID_SIOTS_MAX_STACK)
    {
        siots_p->state_index = 0;
    }
    return;
}
static __inline fbe_raid_siots_state_t fbe_raid_siots_get_state(fbe_raid_siots_t *siots_p)
{
    return (fbe_raid_siots_state_t)fbe_raid_common_get_state(&siots_p->common);
}

static __inline fbe_u32_t fbe_raid_siots_get_width(fbe_raid_siots_t * const siots_p)
{
    fbe_u32_t width;
    fbe_raid_geometry_get_width(fbe_raid_siots_get_raid_geometry(siots_p), &width);

    if(RAID_COND(width > FBE_RAID_MAX_DISK_ARRAY_WIDTH))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "SITOS: 0x%p : width (%d) > FBE_RAID_MAX_DISK_ARRAY_WIDTH \n",
                             siots_p, width);
        width = 0;
    }
    return width;
}

/* Accessor for element size in the siots.
 */
static __forceinline fbe_element_size_t fbe_raid_siots_get_blocks_per_element(fbe_raid_siots_t *siots_p)
{
    fbe_element_size_t sectors_per_element;
    fbe_raid_geometry_t *raid_geometry_p = NULL;
    raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_raid_geometry_get_element_size(raid_geometry_p, 
                                                                    &sectors_per_element);

    if(RAID_ERROR_COND((sectors_per_element == FBE_U32_MAX) || (sectors_per_element == 0)))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "SITOS: 0x%p: sectors_per_element %d is either equal to 0 or -1\n",
                             siots_p,
                             sectors_per_element);
        return 0;
    }

    return sectors_per_element;
}

/*!***********************************************************
 * fbe_raid_siots_is_aborted()
 ************************************************************
 * @brief
 *  A siots is aborted if the iots is aborted or if the iots is
 *  marked in error, meaning that a different siots took an error.
 * 
 * @param siots_p - The siots to check.
 * 
 * @return FBE_TRUE - If this is aborted.
 *        FBE_FALSE - Otherwise, aborted.
 * 
 ************************************************************/
static __forceinline fbe_bool_t fbe_raid_siots_is_aborted(fbe_raid_siots_t *siots_p)
{
    fbe_bool_t b_status = FBE_FALSE;
    fbe_raid_iots_t *iots_p = fbe_raid_siots_get_iots(siots_p);

    if (siots_p->error == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_REQUEST_ABORTED)
    {
        b_status = FBE_TRUE;
    }
    if (fbe_raid_iots_is_marked_aborted(iots_p))
    {
        b_status = FBE_TRUE;
    }
    /* We are aborted if we have a bad status that is not a media error. 
     * Media errors continue to generate since we need to return a full set 
     * of blocks even if they are invalidated. 
     */
    if ((iots_p->error != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID) &&
        (iots_p->error != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS) &&
        (iots_p->error != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR))
    {
        b_status = FBE_TRUE;
    }
    return b_status;
}
/**************************************
 * end fbe_raid_siots_is_aborted()
 **************************************/

/*!***********************************************************
 * fbe_raid_siots_is_aborted_for_shutdown()
 ************************************************************
 * @brief
 *  A siots is aborted if the iots is aborted or if the iots is
 *  marked in error, meaning that a different siots took an error.
 * 
 * @param siots_p - The siots to check.
 * 
 * @return FBE_TRUE - If this is aborted.
 *        FBE_FALSE - Otherwise, aborted.
 * 
 ************************************************************/
static __forceinline fbe_bool_t fbe_raid_siots_is_aborted_for_shutdown(fbe_raid_siots_t *siots_p)
{
    fbe_bool_t b_status = FBE_FALSE;
    fbe_raid_iots_t *iots_p = fbe_raid_siots_get_iots(siots_p);

    if (fbe_raid_iots_is_flag_set(iots_p, FBE_RAID_IOTS_FLAG_ABORT_FOR_SHUTDOWN))
    {
        b_status = FBE_TRUE;
    }
    return b_status;
}
/**************************************
 * end fbe_raid_siots_is_aborted_for_shutdown()
 **************************************/

/*!***********************************************************
 * fbe_raid_siots_is_quiescing()
 ************************************************************
 * @brief
 *  A siots is quiescing if the iots is marked quiescing

 * @param siots_p - The siots to check.
 * 
 * @return FBE_TRUE - If this siots is being quiesced
 *        FBE_FALSE - Otherwise not quiescing
 * 
 ************************************************************/
static __forceinline fbe_bool_t fbe_raid_siots_is_quiescing(fbe_raid_siots_t *siots_p)
{
    if (fbe_raid_iots_is_marked_quiesce(fbe_raid_siots_get_iots(siots_p)))
    {
        return FBE_TRUE;
    }
    return FBE_FALSE;
}
/**************************************
 * end fbe_raid_siots_is_quiescing()
 **************************************/

/*!***********************************************************
 * fbe_raid_siots_should_check_checksums()
 ************************************************************
 * @brief
 *  Determine if we should check checksums or not.
 * 
 * @param siots_p - The siots to check.
 * 
 * @return FBE_TRUE - Check Checksums
 *        FBE_FALSE - Otherwise, do not check checksums.
 * 
 ************************************************************/
static __forceinline fbe_bool_t fbe_raid_siots_should_check_checksums(fbe_raid_siots_t *siots_p)
{
    fbe_bool_t b_status = FBE_FALSE;

    /* Just use the parent IOTS to determine if checksum checking is needed.
     */
    if (fbe_raid_iots_should_check_checksums(fbe_raid_siots_get_iots(siots_p)))
    {
        b_status = FBE_TRUE;
    }
    else if (siots_p->algorithm == MIRROR_VR_WR ||
             siots_p->algorithm == MIRROR_WR)
    {
        /* We will always do checksum checking on mirror writes.
         * The upper level is not guaranteed to send this down.
         */ 
        b_status = FBE_TRUE;
    }
    return b_status;
}
/**************************************
 * end fbe_raid_siots_should_check_checksums()
 **************************************/
/*!***********************************************************
 * fbe_raid_siots_is_corrupt_crc()
 ************************************************************
 * @brief
 *  Determine if the buffer has invalid blocks
 * 
 * @param siots_p - The siots to check.
 * 
 * @return FBE_TRUE - has invalid blocks
 *        FBE_FALSE - Otherwise, does not 
 * 
 ************************************************************/
static __forceinline fbe_bool_t fbe_raid_siots_is_corrupt_crc(fbe_raid_siots_t *siots_p)
{
    fbe_bool_t b_status = FBE_FALSE;

    /* Just use the parent IOTS to determine if the buffer contains invalid blocks.
     */
    if (fbe_raid_iots_is_corrupt_crc(fbe_raid_siots_get_iots(siots_p)))
    {
        b_status = FBE_TRUE;
    }
    return b_status;
}
/**************************************
 * end fbe_raid_siots_is_corrupt_crc()
 **************************************/

/*!***********************************************************
 * fbe_raid_siots_is_corrupt_operation()
 ************************************************************
 * @brief
 *  Determine if siots operations is corrupt CRC or data
 *  data operation or not.
 * 
 * @param siots_p - The siots to check.
 * 
 * @return FBE_TRUE if siots's operation is corrupt CRC/Data
 *         FBE_FALSE otherwise
 * 
 * @note:
 *  Currently, it is used at the time of reporting event 
 *  log messages to determine whether siots's operation
 *  is corrupt CRC/Data. But its a generic function and
 *  hence can be used at other places too.
 ************************************************************/
static __forceinline fbe_bool_t fbe_raid_siots_is_corrupt_operation(fbe_raid_siots_t *siots_p)
{
    fbe_bool_t b_status = FBE_FALSE;
    fbe_payload_block_operation_opcode_t opcode;

    fbe_raid_siots_get_opcode(siots_p, &opcode);

    /* Just use the parent IOTS to determine if the buffer contains invalid blocks.
     */
    if ((fbe_raid_iots_is_corrupt_crc(fbe_raid_siots_get_iots(siots_p)) == FBE_TRUE) ||
        (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_CORRUPT_DATA))
    {
        b_status = FBE_TRUE;
    }

    return b_status;
}
/**************************************
 * end fbe_raid_siots_is_corrupt_operation()
 **************************************/


/*!***********************************************************
 * fbe_raid_siots_should_invalidate_blocks()
 ************************************************************
 * @brief
 *  Determine if we should invalidate blocks when checking checksums
 * 
 * @param siots_p - The siots to check.
 * 
 * @return FBE_TRUE - Invalidate Blocks
 *        FBE_FALSE - Otherwise, do not Invalidate Blocks
 * 
 ************************************************************/
static __forceinline fbe_bool_t fbe_raid_siots_should_invalidate_blocks(fbe_raid_siots_t *siots_p)
{
    fbe_bool_t b_status = FBE_FALSE;

    /* Raid 0 Reads should invalidate blocks when checking the checksum.
     */
    if (siots_p->algorithm == RAID0_RD)
    {
        b_status = FBE_TRUE;
    }
    return b_status;
}
/**************************************
 * end fbe_raid_siots_should_invalidate_blocks()
 **************************************/
/*!***********************************************************
 * fbe_raid_siots_is_buffered_op()
 ************************************************************
 * @brief
 *  A buffered operation is an operation that the raid group object is 
 * allocating buffers for.  In other words, the client is not sending 
 * an sg list with the operation. 
 * 
 * @param siots_p - The siots to check.
 * 
 * @return FBE_TRUE - Buffered operation, need to allocate buffers.
 *        FBE_FALSE - Non-buffered operation, buffers arrived with op.
 * 
 ************************************************************/
static __forceinline fbe_bool_t fbe_raid_siots_is_buffered_op(fbe_raid_siots_t *siots_p)
{
    fbe_bool_t b_status = FBE_FALSE;
    fbe_payload_block_operation_opcode_t opcode;

    /* There are a few opcodes where we will buffer the transfer like zeros.
     */
    fbe_raid_siots_get_opcode(siots_p, &opcode);

    if ((opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ZERO) ||
        (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_ZEROS) ||
        (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_REKEY_WRITE_ZEROS) || 
        (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_LOG_HDR_RD) ||
        (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_LOG_FLUSH))
    {
        b_status = FBE_TRUE;
    }
    return b_status;
}
/**************************************
 * end fbe_raid_siots_is_buffered_op()
 **************************************/
/*!***********************************************************
 * fbe_raid_siots_is_read_only_verify()
 ************************************************************
 * @brief
 *  Determines if this is a read only background verify
 *  which should not modify the media.  The operation should
 *  only report on issues that it finds.
 * 
 * @param siots_p - The siots to check.
 * 
 * @return FBE_TRUE - Read only Verify operation, no corrections performed.
 *        FBE_FALSE - Regular verify, corrections can be performed..
 * 
 ************************************************************/

static __forceinline fbe_bool_t fbe_raid_siots_is_read_only_verify(fbe_raid_siots_t *siots_p)
{
    fbe_bool_t b_status = FBE_FALSE;
    fbe_payload_block_operation_opcode_t opcode;

    /*! The read only background verify is sent as a different opcode. 
     */
    fbe_raid_siots_get_opcode(siots_p, &opcode);

    if ((opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ_ONLY_VERIFY) ||
        (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ_ONLY_VERIFY_SPECIFIC_AREA))
    {
        b_status = FBE_TRUE;
    }
    return b_status;
}
/******************************************
 * end fbe_raid_siots_is_read_only_verify()
 ******************************************/

/*!***********************************************************
 * fbe_raid_siots_is_bva_verify()
 ************************************************************
 * @brief
 *  Determines if this is a bva verify which doesn't require
 *  any parent sg, buffer syncronization.
 * 
 * @param siots_p - The siots to check.
 * 
 * @return FBE_TRUE - BVA Verify, Not parent buffer requirements
 *        FBE_FALSE - Not a BVA verify, must use parent buffer 
 *                    information
 * 
 ************************************************************/

static __forceinline fbe_bool_t fbe_raid_siots_is_bva_verify(fbe_raid_siots_t *siots_p)
{
    fbe_bool_t b_status = FBE_FALSE;
    fbe_payload_block_operation_opcode_t opcode;

    /*! The background verify avoidance verify is sent as a different opcode. 
     */
    fbe_raid_siots_get_opcode(siots_p, &opcode);

    if (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_WRITE)
    {
        b_status = FBE_TRUE;
    }
    return b_status;
}
/******************************************
 * end fbe_raid_siots_is_bva_verify()
 ******************************************/

/*!***********************************************************
 *          fbe_raid_siots_is_background_request()
 *************************************************************
 * @brief
 *  Determines if this is a background (i.e. request has been
 *  issued by the raid group monitor or not).  This method is 
 *  needed since some error handling (such as newly detected
 *  dead positions) must be different for background requests.
 * 
 * @param siots_p - The siots to check.
 * 
 * @return FBE_TRUE - Is a background request
 *        FBE_FALSE - Normla user request
 * 
 ************************************************************/

static __forceinline fbe_bool_t fbe_raid_siots_is_background_request(fbe_raid_siots_t *siots_p)
{
    fbe_bool_t b_status;
    fbe_payload_block_operation_opcode_t opcode;

    /*!  Currently all `disk based' request are background request
     */
    fbe_raid_siots_get_opcode(siots_p, &opcode);
    b_status = fbe_raid_library_is_opcode_lba_disk_based(opcode);
    
    return b_status;
}
/********************************************
 * end fbe_raid_siots_is_background_request()
 ********************************************/

/* Accessors for the memory request information
 */
static __forceinline fbe_memory_request_t *fbe_raid_siots_get_memory_request(fbe_raid_siots_t *siots_p)
{
    return(&siots_p->memory_request);
}
static __forceinline void fbe_raid_siots_get_memory_request_priority(fbe_raid_siots_t *siots_p,
                                                                    fbe_memory_request_priority_t *priority_p)
{
    fbe_memory_request_t   *memory_request_p = fbe_raid_siots_get_memory_request(siots_p);

    *priority_p = memory_request_p->priority;
    return;
}
static __forceinline void fbe_raid_siots_set_memory_request_priority(fbe_raid_siots_t *siots_p,
                                                                    fbe_memory_request_priority_t new_priority)
{
    fbe_memory_request_t   *memory_request_p = fbe_raid_siots_get_memory_request(siots_p);

    memory_request_p->priority = new_priority;
    return;
}
static __forceinline void fbe_raid_siots_get_memory_request_io_stamp(fbe_raid_siots_t *siots_p,
                                                                    fbe_memory_io_stamp_t *io_stamp_p)
{
    fbe_memory_request_t   *memory_request_p = fbe_raid_siots_get_memory_request(siots_p);

    *io_stamp_p = memory_request_p->io_stamp;
    return;
}
static __forceinline void fbe_raid_siots_set_memory_request_io_stamp(fbe_raid_siots_t *siots_p,
                                                                     fbe_memory_io_stamp_t new_io_stamp)
{
    fbe_memory_request_t   *memory_request_p = fbe_raid_siots_get_memory_request(siots_p);

    memory_request_p->io_stamp = new_io_stamp;
    return;
}


/* Accessors for page size
 */
static __forceinline void fbe_raid_siots_get_page_size_bytes(fbe_raid_siots_t *siots_p,
                                                      fbe_u32_t *size_bytes_p)
{
    *size_bytes_p = siots_p->ctrl_page_size * FBE_BE_BYTES_PER_BLOCK;
    return;
}

static __forceinline void fbe_raid_siots_get_optimal_size(fbe_raid_siots_t *siots_p,
                                                   fbe_u32_t *size_p)
{
    fbe_raid_geometry_t *raid_geometry_p = NULL;
    raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_raid_geometry_get_optimal_size(raid_geometry_p, size_p);
    return;
}

/*!********************************************************
 * fbe_raid_siots_get_region_size()
 *********************************************************
 * @brief Determine the region size this raid group should use.
 *        This takes the optimal block size into account for this
 *        raid group since we want to mine at the optimal block size.
 *********************************************************/
static __forceinline fbe_u32_t fbe_raid_siots_get_region_size( fbe_raid_siots_t *const siots_p)
{
    fbe_u32_t optimal_block_size;
    fbe_raid_siots_get_optimal_size(siots_p, &optimal_block_size);
    return FBE_MAX(FBE_RAID_MINE_REGION_SIZE, optimal_block_size);
}

static __forceinline fbe_raid_position_t fbe_raid_siots_get_start_pos(fbe_raid_siots_t *siots_p)
{
    return (siots_p->geo.position[siots_p->geo.start_index]);
}

static __forceinline fbe_block_count_t fbe_raid_siots_get_parity_stripe_offset(fbe_raid_siots_t *siots_p)
{
    return (siots_p->geo.start_offset_rel_parity_stripe);
}

/* Get the offset for the start of the parity stripe accessed by this I/O.
 * The geo logical parity start is the offset from the unit start 
 * to the beginning of the parity stripe.
 * The siots parity start is the offset from the unit start
 * to the beginning of the stripe accessed by this I/O.
 */
static __inline fbe_lba_t fbe_raid_siots_get_parity_start_offset(fbe_raid_siots_t *siots_p)
{

    if(RAID_COND(siots_p->parity_start < siots_p->geo.logical_parity_start))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
            "SIOTS: %p: siots_p->parity_start(0x%llx) < siots_p->geo.logical_parity_start(0x%llx)",
            siots_p, (unsigned long long)siots_p->parity_start,
	    (unsigned long long)siots_p->geo.logical_parity_start);
        return FBE_RAID_INVALID_LBA;
    }

    return (siots_p->parity_start - siots_p->geo.logical_parity_start);
}

/********************************************************************************
 * START   R G   S I O T S   P A R I T Y     M A C R O S
 ********************************************************************************/
/* These imports are necessary, since the below macros reference these functions.
 */
fbe_status_t fbe_raid_siots_dead_pos_set( fbe_raid_siots_t *const siots_p,
                                  fbe_raid_degraded_position_t position,
                                  fbe_raid_position_t new_value );
static __inline fbe_raid_position_t fbe_raid_siots_dead_pos_get( fbe_raid_siots_t *const siots_p,
                                                          fbe_raid_degraded_position_t position )
{
    /* Make sure that if we are a parity unit, and there is a valid dead positin, then the
     * first and second dead positions are not equivalent. 
     */
   if ( fbe_raid_geometry_is_parity_type(fbe_raid_siots_get_raid_geometry(siots_p)) &&
        (siots_p->dead_pos != FBE_RAID_INVALID_DEAD_POS) &&
        (siots_p->dead_pos == siots_p->dead_pos_2) )
   {
       return FBE_RAID_INVALID_DEAD_POS;
   }
       
    /* Based on the input position, we find the
     * field in the structure to return.
     */
    if (position == FBE_RAID_FIRST_DEAD_POS)
    {
        return siots_p->dead_pos;
    }
    else
    {
        /* Make sure it is second dead pos,
         * as we don't support anything other than 1st and 2nd.
         */
        if(RAID_COND(position != FBE_RAID_SECOND_DEAD_POS))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "SITOS: %p : position(%d)!= FBE_RAID_SECOND_DEAD_POS",
                                 siots_p, position);
            return FBE_RAID_INVALID_DEAD_POS;
        }
        return siots_p->dead_pos_2;
    }
}
static __forceinline void fbe_raid_siots_init_dead_positions(fbe_raid_siots_t *const siots_p)
{
    siots_p->dead_pos = FBE_RAID_INVALID_DEAD_POS;
    siots_p->dead_pos_2 = FBE_RAID_INVALID_DEAD_POS;
}
/**************************************************
 * fbe_raid_siots_pos_is_dead
 * Returns true if either the first or second
 * parity position matches the input position.
 **************************************************/
static __forceinline fbe_bool_t fbe_raid_siots_pos_is_dead(fbe_raid_siots_t *const siots_p, fbe_raid_position_t pos)
{
    fbe_bool_t b_status = FBE_FALSE;

    if ( fbe_raid_siots_dead_pos_get(siots_p, FBE_RAID_FIRST_DEAD_POS) == pos ||
         fbe_raid_siots_dead_pos_get(siots_p, FBE_RAID_SECOND_DEAD_POS) == pos )
    {
        b_status = FBE_TRUE;
    }
    return b_status;
}

/*************************************************************
 * fbe_raid_siots_parity_num_positions
 * Determine the number of parity disks for this parity unit.
 * Raid 6 has 2 and everything else has 1.
 *************************************************************/
static __inline fbe_raid_position_t fbe_raid_siots_parity_num_positions(fbe_raid_siots_t *const siots_p)
{
    return fbe_raid_geometry_is_raid6(fbe_raid_siots_get_raid_geometry(siots_p)) ? 2 : 1;
}

/* We always store the parities in the last position(s)
 * in the extent.
 * Thus to get the "first" parity position, we simply
 * look at width - parity positions.
 */
static __forceinline fbe_raid_position_t fbe_raid_siots_get_parity_pos(fbe_raid_siots_t * const siots_p)
{
    fbe_u16_t array_width = fbe_raid_siots_get_width(siots_p);

    if (array_width == 0)
    {
        return FBE_RAID_INVALID_PARITY_POSITION;
    }

    return (siots_p->geo.position[array_width - fbe_raid_siots_parity_num_positions(siots_p)]);
}

static __forceinline void fbe_raid_siots_geometry_get_parity_pos(fbe_raid_siots_t * const siots_p,
                                                                 fbe_raid_geometry_t *const geometry_p,
                                                                 fbe_raid_position_t *const parity_pos_p)
{
    fbe_u32_t num_parity;
    fbe_raid_geometry_get_num_parity(geometry_p, &num_parity);
    *parity_pos_p = siots_p->geo.position[geometry_p->width - num_parity];
}

/**************************************************
 * fbe_raid_siots_get_row_parity_pos, fbe_raid_siots_dp_pos
 *
 * The Row Parity is the same as fbe_raid_siots_get_parity_pos().
 * The Diagonal Parity is always next to parity_pos,
 * the last item in the array, or width - 1.
 **************************************************/
static __forceinline fbe_raid_position_t fbe_raid_siots_get_row_parity_pos(fbe_raid_siots_t *const siots_p) 
{
    return (fbe_raid_siots_get_parity_pos(siots_p));
}

static __forceinline fbe_raid_position_t fbe_raid_siots_dp_pos(fbe_raid_siots_t *const siots_p) 
{
    return (siots_p->geo.position[fbe_raid_siots_get_width(siots_p) - 1]);
}

/**************************************************
 * fbe_raid_siots_parity_both_dead
 * Returns true if both the first and second
 * parity position matches the input position.
 **************************************************/
static __forceinline fbe_bool_t fbe_raid_siots_parity_both_dead(fbe_raid_siots_t *const siots_p)
{
    fbe_bool_t b_status = FBE_FALSE;

    if ( fbe_raid_siots_pos_is_dead(siots_p, fbe_raid_siots_dp_pos(siots_p)) &&
         fbe_raid_siots_pos_is_dead(siots_p, fbe_raid_siots_get_row_parity_pos(siots_p)) )
    {
        b_status = FBE_TRUE;
    }
    return b_status;
}

/**************************************************
 * fbe_raid_siots_is_parity_pos
 * Returns true if the input position is a
 * parity position.
 *
 * For R5 we only return TRUE if Row parity is a match.
 * For R6 we return TRUE if either Row or Diagonal
 * parity is a match.
 **************************************************/
static __forceinline fbe_bool_t fbe_raid_siots_is_parity_pos(fbe_raid_siots_t *const siots_p, fbe_raid_position_t pos)
{
    fbe_bool_t b_status = FBE_FALSE;
    if ( (pos == fbe_raid_siots_get_row_parity_pos(siots_p)) ||
         ( (fbe_raid_geometry_is_raid6(fbe_raid_siots_get_raid_geometry(siots_p)) == TRUE) &&
           (pos == fbe_raid_siots_dp_pos(siots_p)) ) )
    {
        b_status = FBE_TRUE;
    }
    return b_status;
}

/*!**************************************************************
 * fbe_raid_siots_parity_bitmask()
 ****************************************************************
 * @brief returns a bitmask of all the parity positions.  
 *
 * @param  siots_p - current i/o               
 *
 * @return bitmask of positions in this parity stripe that are parity.
 *
 ****************************************************************/
static __forceinline fbe_raid_position_bitmask_t fbe_raid_siots_parity_bitmask(fbe_raid_siots_t *siots_p)
{
    fbe_raid_position_bitmask_t bitmask = 0;
    bitmask = (1 << fbe_raid_siots_get_row_parity_pos(siots_p));
    
    if ( fbe_raid_siots_parity_num_positions(siots_p) == 2)
    {
        bitmask |= (1 << fbe_raid_siots_dp_pos(siots_p));
    }
    return bitmask;
}
/******************************************
 * end fbe_raid_siots_parity_bitmask()
 ******************************************/
/*******************************************************************
 * fbe_raid_siots_no_dead_data_pos
 * Returns true if all the dead positions are also
 * parity positions.
 * Does not imply that all parities are dead.
 *
 * We calculate this by checking either:
 *  There is no second dead position and the Row parity is dead.
 *  Or for R6 only:
 *    There is no second dead position and DP is dead.
 *    Or Both Parities are dead.
 ********************************************************************/
static __inline fbe_bool_t fbe_raid_siots_no_dead_data_pos(fbe_raid_siots_t *siots_p)
{
    if ( ( ( fbe_raid_siots_dead_pos_get(siots_p, FBE_RAID_SECOND_DEAD_POS) == FBE_RAID_INVALID_DEAD_POS)  &&
           fbe_raid_siots_pos_is_dead(siots_p, fbe_raid_siots_get_row_parity_pos(siots_p)) ) ||
  
         ( fbe_raid_siots_parity_num_positions(siots_p) > 1 &&
           ( ( ( fbe_raid_siots_dead_pos_get(siots_p, FBE_RAID_SECOND_DEAD_POS) == FBE_RAID_INVALID_DEAD_POS)  &&
               ( fbe_raid_siots_pos_is_dead(siots_p, fbe_raid_siots_dp_pos(siots_p)) ) ) ||
             ( fbe_raid_siots_parity_both_dead(siots_p) ) ) ) )
    {
        return FBE_TRUE;
    }
    return FBE_FALSE;
}
/* END fbe_raid_siots_no_dead_data_pos */

/**************************************************
 * fbe_raid_siots_double_degraded
 *  Determine if the SIOTS has 2 dead positions.
 *  Only a raid 6 has this case and only when
 *  the second dead position is set.
 *  The second dead position is only set when
 *  we are double degraded.
 *
 * ASSUMPTIONS:
 *  We do not assume this is a Raid 6, we
 *  will check this below and return false if not Raid 6.
 *
 ***************************************************/
static __forceinline fbe_bool_t fbe_raid_siots_double_degraded(fbe_raid_siots_t *const siots_p)
{
    fbe_bool_t b_status = FBE_FALSE;

    if ( fbe_raid_geometry_is_raid6(fbe_raid_siots_get_raid_geometry(siots_p)) &&
         fbe_raid_siots_dead_pos_get(siots_p, FBE_RAID_SECOND_DEAD_POS) != FBE_RAID_INVALID_DEAD_POS )
    {
        b_status = FBE_TRUE;
    }
    return b_status;
}

/*!*************************************************
 * fbe_raid_siots_get_rebuild_logging_bitmask
 *  The rebuild logging bitmask is a bitmask of positions
 *  which we know are not present.
 *
 * @return
 *   fbe_raid_position_bitmask_t bitmask of positions
 *   which are not present and thus are rebuild logging.
 ***************************************************/
static __forceinline void fbe_raid_siots_get_rebuild_logging_bitmask(fbe_raid_siots_t * const siots_p,
                                                                fbe_raid_position_bitmask_t *rl_bitmask_p)
{
    fbe_raid_iots_get_rebuild_logging_bitmask(fbe_raid_siots_get_iots(siots_p), rl_bitmask_p);
    return;
}
/* end fbe_raid_siots_get_rebuild_logging_bitmask */

/**************************************************
 * fbe_raid_siots_dead_pos_count
 *  Deterimine how many dead positions there are
 *  and return the count.
 *
 ***************************************************/
static __inline fbe_u32_t fbe_raid_siots_dead_pos_count(fbe_raid_siots_t * const siots_p)
{
    fbe_u32_t dead_pos_count = 0;

    if ( fbe_raid_siots_dead_pos_get(siots_p, FBE_RAID_FIRST_DEAD_POS) == FBE_RAID_INVALID_DEAD_POS )
    {
        /* We have no degraded positions, return the default value below.
         */
        fbe_raid_position_t dead_pos2 = fbe_raid_siots_dead_pos_get(siots_p, FBE_RAID_SECOND_DEAD_POS);

        /* If the second position has a value this is illegal. Return that we have 1 dead position.
         * This should not be checked with RAID_COND since forcing this error when the dead positions are
         * correct will cause problems.
         */
        if(dead_pos2 != FBE_RAID_INVALID_DEAD_POS)
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                    "SIOTS: %p : second dead pos (%d) != FBE_RAID_INVALID_DEAD_POS",
                    siots_p,fbe_raid_siots_dead_pos_get(siots_p, FBE_RAID_SECOND_DEAD_POS));
            dead_pos_count = 1;
        }
    }
    else if ( fbe_raid_siots_dead_pos_get(siots_p, FBE_RAID_SECOND_DEAD_POS) != FBE_RAID_INVALID_DEAD_POS )
    {
        /* We have two positions since if the second is set,
         * the first is also set.
         */
        dead_pos_count = 2;
    }
    else
    {
        /* Otherwise we must have 1 dead position.
         */
        dead_pos_count = 1;
    }
    return dead_pos_count;
}
/* end fbe_raid_siots_dead_pos_count() */

/**************************************************
 * fbe_raid_siots_dead_parity_pos_count
 *  Determine how many dead parity positions there are
 *  and return the count.
 *
 ***************************************************/
static __inline fbe_u32_t fbe_raid_siots_dead_parity_pos_count(fbe_raid_siots_t * const siots_p)
{
    fbe_u32_t dead_parity_count = 0;
    fbe_u32_t first_dead_pos;
    fbe_u32_t second_dead_pos;

    /* Get the first dead position, and increment our count if this
     * position is a dead parity position.
     */
    first_dead_pos = fbe_raid_siots_dead_pos_get(siots_p, FBE_RAID_FIRST_DEAD_POS);
    
    if ( first_dead_pos != FBE_RAID_INVALID_DEAD_POS &&
         ( (first_dead_pos == fbe_raid_siots_get_row_parity_pos(siots_p)) ||
           (first_dead_pos == fbe_raid_siots_dp_pos(siots_p)) ) )
    {
        /* The first dead position is parity.
         */
        dead_parity_count++;
    }
    
    /* Get the second dead position, and increment our count if this
     * position is a dead parity position.
     */
    second_dead_pos = fbe_raid_siots_dead_pos_get(siots_p, FBE_RAID_FIRST_DEAD_POS);
    
    if ( second_dead_pos != FBE_RAID_INVALID_DEAD_POS &&
         ( (second_dead_pos == fbe_raid_siots_get_row_parity_pos(siots_p)) ||
           (second_dead_pos == fbe_raid_siots_dp_pos(siots_p)) ) )
    {
        /* The second dead position is parity.
         */
        dead_parity_count++;
    }
    return dead_parity_count;
}
/* end fbe_raid_siots_dead_parity_pos_count() */

/*!****************************************
 * fbe_raid_siots_get_degraded_bitmask 
 * **************************************** 
 * @brief Return a bitmask of dead positions from the siots.
 *  
 *****************************************/
static __inline void fbe_raid_siots_get_degraded_bitmask(fbe_raid_siots_t *siots_p,
                                                  fbe_u16_t *deg_bitmask_p)
{
    /* Init the dead bitmask to zero.
     * We will OR in any dead positions we find below.
     */
    fbe_u32_t dead_bitmask = 0;

    /* If the 1st dead position is valid, then OR it in.
     */
    if (fbe_raid_siots_dead_pos_get(siots_p, FBE_RAID_FIRST_DEAD_POS) != FBE_RAID_INVALID_DEAD_POS )
    {
        dead_bitmask |= (1 << fbe_raid_siots_dead_pos_get(siots_p, FBE_RAID_FIRST_DEAD_POS));
    }
    /* If the 2nd dead position is valid, then OR it in.
     */
    if (fbe_raid_siots_dead_pos_get(siots_p, FBE_RAID_SECOND_DEAD_POS) != FBE_RAID_INVALID_DEAD_POS )
    {
        dead_bitmask |= (1 << fbe_raid_siots_dead_pos_get(siots_p, FBE_RAID_SECOND_DEAD_POS));
    }
    *deg_bitmask_p = dead_bitmask;
    return;
}
/*!****************************************
 * fbe_raid_siots_get_disabled_bitmask
 * ****************************************
 * 
 * @brief Return a bitmask of known dead positions from the siots.
 * 
 * @param siots_p - Pointer to siots to get disabled bitmask for
 * @param disabled_bitmask_p - Address of disabled bitmask to update
 *
 * @return None (always succeeds) 
 *****************************************/
static __forceinline void fbe_raid_siots_get_disabled_bitmask(fbe_raid_siots_t *siots_p,
                                                       fbe_raid_position_bitmask_t *disabled_bitmask_p)
{
    fbe_raid_iots_t    *iots_p = fbe_raid_siots_get_iots(siots_p);
    fbe_raid_position_bitmask_t disabled_bitmask = 0;
    
    /*! @note Although the `rebuild logging' bitmask represents those positions
     *        that are rebuild logging, for some algorithms like write we still 
     *        process and simply generate a `NR' log.  But for rebuild etc, we 
     *        don't want to write to missing drives!
     */
    fbe_raid_iots_get_rebuild_logging_bitmask(iots_p, &disabled_bitmask);

    /*! @note Previously we ORed in the continue_bitmask since it indicates 
     *        positions that encountered a `dead' error.  But one or more
     *        dead positions may return (remembering that there is a delay
     *        between retries).  Therefore we no longer OR in the continue
     *        bitmask.
     */

    /* Update the bitmask at the address passed.
     */
    *disabled_bitmask_p = disabled_bitmask;

    return;
}
/* end fbe_raid_siots_get_disabled_bitmask() */

/*!*************************************************
 * fbe_raid_siots_get_rebuild_bitmask
 *  The rebuild bitmask is a bitmask of positions
 *  that we should rebuild.
 *
 * @return
 *   fbe_raid_position_bitmask_t 
 ***************************************************/
static __inline void fbe_raid_siots_get_rebuild_bitmask(fbe_raid_siots_t * const siots_p,
                                                        fbe_raid_position_bitmask_t *rb_bitmask_p)
{
    fbe_raid_position_bitmask_t degraded_bitmask;
    fbe_raid_position_bitmask_t rebuild_logging_bitmask;
    fbe_raid_siots_get_degraded_bitmask(siots_p, &degraded_bitmask);
    fbe_raid_siots_get_rebuild_logging_bitmask(siots_p, &rebuild_logging_bitmask);

    /* The positions to rebuild are the positions which are degraded and are also present.
     */
    *rb_bitmask_p = degraded_bitmask & (~rebuild_logging_bitmask);
    return;
}
/* end fbe_raid_siots_get_rebuild_bitmask */

/********************************************************************************
 *      END           R G   S I O T S   P A R I T Y     M A C R O S
 ********************************************************************************/

/*!************************************************************
 * @brief fbe_raid_siots_get_verify_eboard
 *
 * Return the error board for the request.
 *
 *************************************************************/
static __inline fbe_raid_verify_error_counts_t * fbe_raid_siots_get_verify_eboard(fbe_raid_siots_t *const siots_p)
{
    fbe_raid_verify_error_counts_t *eboard_p = NULL;
    fbe_raid_iots_t *iots_p = NULL;

        /* The error board is in the payload.
         */
        iots_p = fbe_raid_siots_get_iots(siots_p);
         fbe_raid_iots_get_verify_eboard(iots_p, &eboard_p);
    
    return eboard_p;
}
/*********************
 * END fbe_raid_siots_get_verify_eboard
 *********************/


/* Accessors for journal_slot_id.
 */
static __forceinline void fbe_raid_siots_get_journal_slot_id( fbe_raid_siots_t  * siots_p, fbe_u32_t *journal_slot_id_p ) 
{
    *journal_slot_id_p = siots_p->journal_slot_id;
    return;
}
static __forceinline void fbe_raid_siots_set_journal_slot_id( fbe_raid_siots_t * siots_p, fbe_u32_t journal_slot_id ) 
{
    siots_p->journal_slot_id = journal_slot_id;
    return;
}

static __forceinline void fbe_raid_siots_get_multi_bit_crc_bitmap(fbe_raid_siots_t *siots_p,
                                                                  fbe_raid_position_bitmask_t *bitmap_p)
{
    fbe_raid_position_bitmask_t multi_bitmap;

    multi_bitmap = siots_p->vrts_ptr->eboard.crc_multi_bitmap;

    if (multi_bitmap &&
        siots_p->vrts_ptr->eboard.crc_multi_and_lba_bitmap && 
        fbe_raid_library_get_encrypted()) {
        /* When encrypted we only fault drives that did not have an lba stamp error in
         * addition to the checksum error. 
         */
        multi_bitmap &= (~siots_p->vrts_ptr->eboard.crc_multi_and_lba_bitmap);
    }
    *bitmap_p = multi_bitmap;
}
/* check pdo usurper needed or not.
 */
static __forceinline fbe_bool_t fbe_raid_siots_pdo_usurper_needed(fbe_raid_siots_t *siots_p)
{
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_u16_t multi_bitmap;
    fbe_payload_block_operation_opcode_t opcode;
    fbe_raid_siots_get_opcode(siots_p, &opcode);
    fbe_raid_siots_get_multi_bit_crc_bitmap(siots_p, &multi_bitmap);

    /* We send the usurper for single and multi-bit CRC errors. 
     * We do not send the usurper if it was injected, if it is a raw mirror or c4mirror, or if it is a 
     * read only verify specific (used in encryption paged error handling).
     */
    if ((siots_p->vrts_ptr->eboard.crc_single_bitmap != 0 || multi_bitmap != 0)                                        &&
        (!fbe_raid_siots_is_flag_set(siots_p, FBE_RAID_SIOTS_FLAG_ERROR_INJECTED) ||
          fbe_raid_geometry_is_debug_flag_set(raid_geometry_p, FBE_RAID_LIBRARY_DEBUG_FLAG_SEND_CRC_ERR_TO_PDO))       &&
        !fbe_raid_geometry_is_debug_flag_set(raid_geometry_p, FBE_RAID_LIBRARY_DEBUG_FLAG_DISABLE_SEND_CRC_ERR_TO_PDO) &&
        !fbe_raid_geometry_is_flag_set(raid_geometry_p, FBE_RAID_GEOMETRY_FLAG_RAW_MIRROR)                             &&
        (opcode != FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ_ONLY_VERIFY_SPECIFIC_AREA)                                  &&
        !fbe_raid_geometry_is_attribute_set(raid_geometry_p, FBE_RAID_ATTRIBUTE_C4_MIRROR)) {
    
        return FBE_TRUE;
    }

    return FBE_FALSE;
}

/*!***********************************************************
 * fbe_raid_siots_is_marked_for_incomplete_write_verify()
 ************************************************************
 * @brief
 *  This function determines if the region for this SIOTS is marked 
 * for Incomplete  write verify or not
 * 
 * @param siots_p - The siots to check.
 * 
 * @return FBE_TRUE - Marked for Incomplete write verify
 *        FBE_FALSE - Otherwise.
 * 
 ************************************************************/
static __forceinline fbe_bool_t fbe_raid_siots_is_marked_for_incomplete_write_verify(fbe_raid_siots_t *siots_p)
{
    fbe_raid_group_paged_metadata_t *chunk_info_p;
    fbe_raid_iots_t *iots_p = NULL;

    iots_p = fbe_raid_siots_get_iots(siots_p);
    fbe_raid_iots_get_chunk_info(iots_p, 0, &chunk_info_p);
    if(chunk_info_p->verify_bits & FBE_RAID_BITMAP_VERIFY_INCOMPLETE_WRITE)
    {
        return FBE_TRUE;
    }
    else
    {
        return FBE_FALSE;
    }
}

void fbe_raid_siots_traffic_trace(fbe_raid_siots_t *siots_p,
                                  fbe_bool_t b_start);
static __forceinline void fbe_raid_siots_check_traffic_trace(fbe_raid_siots_t *siots_p,
                                                        fbe_bool_t b_start)
{
    if (!fbe_raid_siots_is_flag_set(siots_p, FBE_RAID_SIOTS_FLAG_LOGGING_ENABLED) ||
        !fbe_traffic_trace_is_enabled(KTRC_TFBE_RG)){
        /* Normal path.  We are coding for success.
         */
    }
    else{
        /* Rather than tracing at both striper and mirror level, we'll just trace at mirror level. */
        fbe_raid_geometry_t * raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);

        if (!fbe_raid_geometry_is_raid10(raid_geometry_p)) {
            fbe_raid_siots_traffic_trace(siots_p, b_start);
        }
    }
}

#endif /* FBE_RAID_SIOTS_ACCESSOR_H */
/*************************
 * end file fbe_raid_siots_accessor.h
 *************************/


