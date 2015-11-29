#ifndef EXT_POOL_LUN_PRIVATE_H
#define EXT_POOL_LUN_PRIVATE_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_ext_pool_lun_private.h
 ***************************************************************************
 *
 * @brief
 *  This file contains private defines for the lun object.
 *  This includes the definitions of the
 *  @ref fbe_ext_pool_lun_t "lun" itself, as well as the associated
 *  structures and defines.
 * 
 * @ingroup lun_class_files
 * 
 * @version
 *   05/22/2009:  Created. RPF
 *
 ***************************************************************************/

#include "fbe/fbe_transport.h"
#include "fbe_block_transport.h"
#include "fbe/fbe_lun.h"
#include "fbe/fbe_perfstats_interface.h"
#include "fbe_base_config_private.h"
#include "fbe_cmi.h"
#include "fbe_perfstats.h"
#include "fbe_database.h"
#include "fbe/fbe_stat_api.h"

/************************************************************
 *  TYPEDEFS, ENUMS, DEFINES, CONSTANTS, MACROS, GLOBALS
 ************************************************************/

/* Lifecycle definitions
 * Define the lun lifecycle's instance.
 */
extern fbe_lifecycle_const_t FBE_LIFECYCLE_CONST_DATA(ext_pool_lun);

#pragma pack(1)
/*!****************************************************************************
 *    
 * @struct fbe_lun_paged_metadata_s
 *  
 * @brief 
 *   This is the paged metadata for the lun object
 *   Currently it has only the dirty flag indicating that the LUN
 *   has outstanding non-cached writes.
 *   
 ******************************************************************************/
typedef union fbe_lun_paged_metadata_u
{
    struct {
        fbe_u32_t unused;
    } unnamed;
}
fbe_lun_paged_metadata_t;
#pragma pack()

#pragma pack(1)
/*!****************************************************************************
 *    
 * @struct fbe_lun_paged_metadata_s
 *  
 * @brief 
 *   This is the paged metadata for the lun object
 *   Currently it has only the dirty flag indicating that the LUN
 *   has outstanding non-cached writes.
 *   
 ******************************************************************************/
typedef struct fbe_lun_dirty_flag_s
{
    fbe_bool_t dirty;
}
fbe_lun_dirty_flag_t;
#pragma pack()


#define FBE_LUN_DIRTY_FLAG_SP_A_BLOCK_OFFSET 0
#define FBE_LUN_DIRTY_FLAG_SP_B_BLOCK_OFFSET 128;

#define FBE_LUN_LAZY_CLEAN_TIME_MS 5000

/* Macro to trace the entry of a function in the LUN object or in one of its derived classes */
#define FBE_LUN_TRACE_FUNC_ENTRY(m_object_p)             \
                                                                \
    fbe_base_object_trace((fbe_base_object_t*) m_object_p,      \
                          FBE_TRACE_LEVEL_DEBUG_HIGH,           \
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,  \
                          "%s entry\n", __FUNCTION__);          \

/* Minimum and maximum zero checkpoing update interval. 1 unit of interval is
 * 1 monitor call to background condition. Based on zeroing rate the update
 * interval is changed.
 */
#define FBE_LUN_MIN_ZEROING_UPDATE_INTERVAL     2
#define FBE_LUN_MAX_ZEROING_UPDATE_INTERVAL     40

/******************************
 * LITERAL DEFINITIONS.
 ******************************/

/******************************
 * LITERAL DEFINITIONS.
 ******************************/

enum fbe_lun_constants_e  {
    FBE_LUN_MAX_NONPAGED_RECORDS = 1,
    FBE_LUN_CHUNK_SIZE = 0x800, /* in blocks ( 2048 blocks ) */
};

/* These are the lifecycle condition ids for a LUN object. */

/*! @enum fbe_lun_lifecycle_cond_id_t  
 *  
 *  @brief These are the lifecycle conditions for the lun object. 
 */
typedef enum fbe_lun_lifecycle_cond_id_e 
{
    /*! See if I/O to LUN is being flushed in preparation for destroying the LUN
     */
    FBE_LUN_LIFECYCLE_COND_LUN_CHECK_FLUSH_FOR_DESTROY = FBE_LIFECYCLE_COND_FIRST_ID(FBE_CLASS_ID_EXTENT_POOL_LUN),

    FBE_LUN_LIFECYCLE_COND_LUN_WAIT_BEFORE_HIBERNATION,

    FBE_LUN_LIFECYCLE_COND_LUN_GET_POWER_SAVE_INFO_FROM_RAID,

    FBE_LUN_LIFECYCLE_COND_VERIFY_NEW_LUN,

    FBE_LUN_LIFECYCLE_COND_LUN_SIGNAL_FIRST_WRITE, /* let upper layers (cache) that the LUN has been written for the first time*/

    FBE_LUN_LIFECYCLE_COND_LUN_MARK_LUN_WRITTEN, /* on startup check if metadata tells us this lun has been written before*/

    FBE_LUN_LIFECYCLE_COND_LAZY_CLEAN,

    FBE_LUN_LIFECYCLE_COND_CHECK_LOCAL_DIRTY_FLAG,

    FBE_LUN_LIFECYCLE_COND_CHECK_PEER_DIRTY_FLAG,

    FBE_LUN_LIFECYCLE_COND_MARK_LOCAL_CLEAN,

    FBE_LUN_LIFECYCLE_COND_MARK_PEER_CLEAN,

    FBE_LUN_LIFECYCLE_COND_UNEXPECTED_ERROR, /*!< Handle unexpected error. */

    FBE_LUN_LIFECYCLE_COND_VERIFY_LUN,

    FBE_LUN_LIFECYCLE_COND_LAST /* must be last */
} 
fbe_lun_lifecycle_cond_id_t;

/*! @enum fbe_lun_clustered_flags_e
 *
 *  @brief flags to indicate states of the lun object related to dual-SP processing.
 */
enum fbe_lun_clustered_flags_e
{
    /*! This indicates that this object has reached the threshold of errors.
     */
    FBE_LUN_CLUSTERED_FLAG_THRESHOLD_REACHED = 0x0000000000000010,

    FBE_LUN_CLUSTERED_FLAG_EVAL_ERR_REQUEST      = 0x0000000000000100,
    FBE_LUN_CLUSTERED_FLAG_EVAL_ERR_REBOOT_PEER  = 0x0000000000000400,
    FBE_LUN_CLUSTERED_FLAG_EVAL_ERR_MASK         = 0x0000000000000700,

    FBE_LUN_CLUSTERED_FLAG_ALL_MASKS = (FBE_LUN_CLUSTERED_FLAG_THRESHOLD_REACHED |
                                        FBE_LUN_CLUSTERED_FLAG_EVAL_ERR_MASK),

    FBE_LUN_CLUSTERED_FLAG_INVALID = 0xFFFFFFFFFFFFFFFF,
};

typedef struct fbe_lun_clean_dirty_context_s
{
    void *              lun_p;
    fbe_cmi_sp_id_t     sp_id;
    fbe_bool_t          is_read;
    fbe_bool_t          dirty;
    fbe_status_t        status;
    fbe_u8_t *          buff_ptr;
} fbe_lun_clean_dirty_context_t;

/*! @enum fbe_lun_local_state_e
 *  
 *  @brief LUN local state which allows us to know where this LUN is
 *         in its processing of certain conditions.
 */
enum fbe_lun_local_state_e {

    FBE_LUN_LOCAL_STATE_JOIN_REQUEST = 0x0000000000001000,
    FBE_LUN_LOCAL_STATE_JOIN_STARTED = 0x0000000000002000,
    FBE_LUN_LOCAL_STATE_JOIN_DONE    = 0x0000000000004000,
    FBE_LUN_LOCAL_STATE_JOIN_MASK    = 0x0000000000007000,

    FBE_LUN_LOCAL_STATE_EVAL_ERR_REQUEST        = 0x0000000001000000,
    FBE_LUN_LOCAL_STATE_EVAL_ERR_STARTED        = 0x0000000002000000,
    FBE_LUN_LOCAL_STATE_EVAL_ERR_MASK           = 0x0000000007000000,

    FBE_LUN_LOCAL_STATE_ALL_MASKS    = (FBE_LUN_LOCAL_STATE_JOIN_MASK |
                                        FBE_LUN_LOCAL_STATE_EVAL_ERR_MASK),

    FBE_LUN_LOCAL_STATE_INVALID      = 0xFFFFFFFFFFFFFFFF,
};
typedef fbe_u64_t fbe_lun_local_state_t;

enum fbe_lun_flags_e {

    FBE_LUN_FLAGS_DISABLE_FAIL_ON_UNEXPECTED_ERRORS = 0x00000001,
    FBE_LUN_FLAGS_HAS_BEEN_WRITTEN                  = 0x00000002,   /*the lun had the first write done into it, we need to keep track of this for upper layeres*/
    FBE_LUN_FLAGS_DISABLE_CLEAN_DIRTY               = 0x00000004,   /* Clean dirty has been disabled */
};

typedef fbe_u32_t fbe_lun_flags_t;

/*!****************************************************************************
 *    
 * @struct fbe_ext_pool_lun_t
 *  
 * @brief 
 *   This is the definition of the lun object.
 *   This object represents a lun, which is an object which can
 *   perform block size conversions.
 ******************************************************************************/

typedef struct fbe_ext_pool_lun_s
{    
    /*! This must be first.  This is the object we inherit from.
     */
    fbe_base_config_t base_config;

    fbe_lun_local_state_t local_state;

    fbe_lun_flags_t flags;

    fbe_path_attr_t prev_path_attr;

    /*! This is the verify report data.
     */
    fbe_lun_verify_report_t verify_report;

    /*! LUN metadata memory.
     */
    fbe_lun_metadata_memory_t lun_metadata_memory;
    fbe_lun_metadata_memory_t lun_metadata_memory_peer;

    /*! @todo: Find out if block edge should be stored here. */
    /*! This is our storage extent protocol edge. 
     */
    fbe_block_edge_t block_edge;

    fbe_lun_index_t object_index; /*!< Set by upper levels like MCC. */

    /*! how many sceonds to wait beofore hibernating after we were idle for a while and notified cache
    this lets IOs in flight get in and prevent us from going to hibernate too soon*/
    fbe_u64_t power_save_io_drain_delay_in_sec;

    /*! how many sceonds the lun user is wiling to wait for the object ot become
    ready after saving power*/
    fbe_u64_t max_lun_latency_time_is_sec;

    /*! In the absence of a way to load a timer with a variable value, we need this counter
    to count until we get to power_save_io_drain_delay_in_sec. we do it in the monitor cycles of 
    FBE_LUN_LIFECYCLE_COND_LUN_WAIT_BEFORE_HIBERNATION   */
    fbe_time_t wait_for_power_save;

    /*! Boolean indicating if NDB was specified by user */
    fbe_bool_t ndb_b;

    /*! Lock to protect the io counter */
    fbe_spinlock_t      io_counter_lock;
 
    /*! io counter to keep track of non cached ios */
    fbe_u32_t   io_counter; 

    /*! Used to indicate that the dirty flag in paged metadata is in the process of being set */
    fbe_bool_t  dirty_pending;
    /*! Used to indicate that the dirty flag in paged metadata is in the process of being cleared */
    fbe_bool_t  clean_pending;
    /*! TRUE if the on-disk metadata is marked dirty for this SP */
    fbe_bool_t  marked_dirty;
    /*! Indicates the time when the LUN last went clean */
    fbe_time_t  clean_time;

    /*! LBA on where the clean/dirty flags start */
    fbe_lba_t   dirty_flags_start_lba;

    /*! Contexts for the clean/dirty callbacks */
    fbe_lun_clean_dirty_context_t clean_dirty_executor_context;
    fbe_lun_clean_dirty_context_t clean_dirty_monitor_context;
    fbe_lun_clean_dirty_context_t clean_dirty_usurper_context;

    /*! Used to indicate that LUN performance statistics is enabled*/
    fbe_bool_t  b_perf_stats_enabled;

    /*! Used to store performance statistics counters manipulated by either LUN object or Raid Group object*/
    fbe_performance_statistics_t performance_stats;

    /*! some attributes about the LUN we need to set*/
    fbe_lun_attributes_t  lun_attributes;

    /*! mark the LU has been written for the first time, stay set forever*/
    fbe_bool_t  lu_has_been_written;
    
    /* whether the LUN should run BV after bind */
    fbe_bool_t  noinitialverify_b;

    /* whether the LUN should run clear_need_zero */
    fbe_bool_t  clear_need_zero_b;

    fbe_bool_t  write_bypass_mode;

    /*track the last rebuild percent we saw so we can tell if it's changing or not*/
    fbe_u32_t last_rebuild_percent; 

    /*keep track of checkpoint for notifications purposes*/
    fbe_lba_t last_zero_checkpoint;

    fbe_atomic_32_t num_unexpected_errors; /*!< Count of invalid request block status received. */

    fbe_u32_t  zero_update_interval_count; /*!< Zero checkpoint update interval. */
    fbe_u32_t  zero_update_count_to_update; /*!< Remaining monitor calls before we update zero checkpoint. */

    fbe_raid_group_get_lun_percent_rebuilt_t    rebuild_status;/*we get this from the RG*/

    fbe_lba_t alignment_boundary; /* LUN will guarantee that I/O will not cross this boundary */

    fbe_lun_initiate_verify_t lun_verify;

    fbe_edge_index_t server_index;

    /* Lifecycle defines.
     */
    FBE_LIFECYCLE_DEF_INST_DATA;
    FBE_LIFECYCLE_DEF_INST_COND(FBE_LIFECYCLE_COND_MAX(FBE_LUN_LIFECYCLE_COND_LAST));
}
fbe_ext_pool_lun_t;

/* Accessors related to local state.
 */
static __forceinline fbe_bool_t fbe_lun_is_local_state_set(fbe_ext_pool_lun_t *lun_p,
                                                                  fbe_lun_local_state_t local_state)
{
    return ((lun_p->local_state & local_state) == local_state);
}
static __forceinline void fbe_lun_init_local_state(fbe_ext_pool_lun_t *lun_p)
{
    lun_p->local_state = 0;
    return;
}
static __forceinline void fbe_lun_set_local_state(fbe_ext_pool_lun_t *lun_p,
                                                         fbe_lun_local_state_t local_state)
{
    lun_p->local_state |= local_state;
}

static __forceinline void fbe_lun_clear_local_state(fbe_ext_pool_lun_t *lun_p,
                                                           fbe_lun_local_state_t local_state)
{
    lun_p->local_state &= ~local_state;
}
static __forceinline void fbe_lun_get_local_state(fbe_ext_pool_lun_t *lun_p,
                                                         fbe_lun_local_state_t *local_state)
{
     *local_state = lun_p->local_state;
}

/*! @fn fbe_ext_pool_lun_lock(fbe_ext_pool_lun_t *const lun_p) 
 *  
 * @brief 
 *   Function which locks the lun via the base object lock.
 */
static __forceinline void
fbe_ext_pool_lun_lock(fbe_ext_pool_lun_t *const lun_p)
{
    fbe_base_object_lock((fbe_base_object_t *)lun_p);
    return;
}

/* @fn fbe_ext_pool_lun_unlock(fbe_ext_pool_lun_t *const lun_p) 
 *  
 * @brief 
 *   Function which unlocks the lun via the base object lock.
 */
static __forceinline void
fbe_ext_pool_lun_unlock(fbe_ext_pool_lun_t *const lun_p)
{
    fbe_base_object_unlock((fbe_base_object_t *)lun_p);
    return;
}

/* Accessor for the block edge.
 */
static __forceinline void fbe_lun_get_block_edge(fbe_ext_pool_lun_t *lun_p,
                                          fbe_block_edge_t **block_edge_p_p)
{
    fbe_base_config_get_block_edge(&lun_p->base_config, 
                                   block_edge_p_p, 
                                   0 /* Edge index since there is only one edge. */);
    return;
}


/* Accessors for the capacity.
 */
static __forceinline void fbe_lun_set_capacity(fbe_ext_pool_lun_t *lun_p, 
                                        fbe_lba_t capacity)
{
    fbe_base_config_set_capacity(&lun_p->base_config, capacity);
    return;
}
static __forceinline void fbe_lun_get_capacity(fbe_ext_pool_lun_t *lun_p,
                                        fbe_lba_t *capacity_p)
{
    fbe_base_config_get_capacity(&lun_p->base_config, capacity_p);
    return;
}

/* Accessors for the NDB boolean.
 */
static __forceinline void fbe_lun_set_ndb_b(fbe_ext_pool_lun_t *lun_p, 
                                     fbe_bool_t ndb_b)
{
    lun_p->ndb_b = ndb_b;
    return;
}
static __forceinline fbe_bool_t fbe_lun_get_ndb_b(fbe_ext_pool_lun_t *lun_p)
{
    return lun_p->ndb_b;
}

/* Accessors for the noinitialverify boolean.
 */
static __forceinline void fbe_lun_set_noinitialverify_b(fbe_ext_pool_lun_t *lun_p, 
                                     fbe_bool_t noinitialverify_b)
{
    lun_p->noinitialverify_b = noinitialverify_b;
    return;
}
static __forceinline fbe_bool_t fbe_lun_get_noinitialverify_b(fbe_ext_pool_lun_t *lun_p)
{
    return lun_p->noinitialverify_b;
}

/* Accessors for the clear_need_zero boolean.
 */
static __forceinline void fbe_lun_set_clear_need_zero_b(fbe_ext_pool_lun_t *lun_p, 
                                     fbe_bool_t clear_need_zero_b)
{
    lun_p->clear_need_zero_b = clear_need_zero_b;
    return;
}
static __forceinline fbe_bool_t fbe_lun_get_clear_need_zero_b(fbe_ext_pool_lun_t *lun_p)
{
    return lun_p->clear_need_zero_b;
}


/* Accessors for the power saving information.
 */
static __forceinline void fbe_lun_set_power_save_info(fbe_ext_pool_lun_t *lun_p,
                                               fbe_u64_t in_power_save_io_drain_delay_in_sec,
                                               fbe_u64_t in_max_lun_latency_time_is_sec)
{
    lun_p->power_save_io_drain_delay_in_sec = in_power_save_io_drain_delay_in_sec;
    lun_p->max_lun_latency_time_is_sec = in_max_lun_latency_time_is_sec;

    return;
}
static __forceinline void fbe_lun_get_power_save_info(fbe_ext_pool_lun_t *lun_p,
                                               fbe_u64_t *out_power_save_io_drain_delay_in_sec,
                                               fbe_u64_t *out_max_lun_latency_time_is_sec)
{
    *out_power_save_io_drain_delay_in_sec = lun_p->power_save_io_drain_delay_in_sec;
    *out_max_lun_latency_time_is_sec = lun_p->max_lun_latency_time_is_sec;

    return;
}


/* Accessor for the verify report.
 */

/* @fn fbe_lun_get_verify_report_ptr(fbe_ext_pool_lun_t *lun_p)
 *  
 * @brief 
 *   Function that gets a pointer to the lun verify report data.
 */
static __forceinline fbe_lun_verify_report_t* fbe_lun_get_verify_report_ptr(fbe_ext_pool_lun_t *lun_p)
{
    return &(lun_p->verify_report);
}
 
/*!****************************************************************************
 * fbe_lun_get_imported_capacity
 ******************************************************************************
 *
 * @brief
 *    This function is used to get the imported capacity of the LUN.
 *
 * @param   lun_p                -  pointer to lun object.
 * @param   imported_capacity_p  -  Pointer to the imported capacity of the LUN.
 *
 * @return  void
 *
 * @version
 *    4/15/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
static __forceinline void fbe_lun_get_imported_capacity(fbe_ext_pool_lun_t *lun_p,
                                                 fbe_lba_t * imported_capacity_p)
{
    fbe_block_edge_t * block_edge_p = NULL;
    fbe_base_config_get_block_edge((fbe_base_config_t *) lun_p, &block_edge_p, 0);
    fbe_block_transport_edge_get_capacity(block_edge_p, imported_capacity_p);
    return;
} // End fbe_lun_get_imported_capacity()

static __forceinline void fbe_lun_get_object_index(fbe_ext_pool_lun_t *lun_p,
                                                   fbe_lun_index_t * object_index_p)
{
    *object_index_p = lun_p->object_index;
    return;
}
static __forceinline void fbe_lun_set_object_index(fbe_ext_pool_lun_t *lun_p,
                                                   fbe_lun_index_t object_index)
{
    lun_p->object_index = object_index;
    return;
}


/*!****************************************************************************
 * fbe_ext_pool_lun_add_to_terminator_queue()
 ******************************************************************************
 * @brief 
 *   Adds the packet to the lun terminator queue. Also sets the cancel
 *   routine.
 *
 * @param in_lun_p              - pointer to the lun object 
 * @param in_packet             - packet to add to the queue.
 *
 * @return void       
 *
 ******************************************************************************/

static __forceinline void fbe_ext_pool_lun_add_to_terminator_queue(
                                    fbe_ext_pool_lun_t*       in_lun_p,
                                    fbe_packet_t*    in_packet_p)
{
    // Add the packet to the terminator q.
    fbe_base_object_add_to_terminator_queue((fbe_base_object_t*) in_lun_p, in_packet_p);

    // Set the cancel routine.
    fbe_transport_set_cancel_function(in_packet_p, 
                                      fbe_base_object_packet_cancel_function, 
                                      (fbe_base_object_t *)in_lun_p);

} // End fbe_ext_pool_lun_add_to_terminator_queue()


/*!****************************************************************************
 * fbe_lun_get_offset
 ******************************************************************************
 *
 * @brief
 *    This function is used to get the offset of the LUN.
 *
 * @param   lun_p                -  pointer to lun object.
 * @param   offset_p             -  Pointer to the offset of the LUN.
 *
 * @return  void
 *
 * @version
 *    9/17/2010 - Created. Ashwin Tamilarasan
 *
 ******************************************************************************/
static __forceinline void fbe_lun_get_offset(fbe_ext_pool_lun_t *lun_p,
                                      fbe_lba_t * offset_p)
{
    fbe_block_edge_t * block_edge_p = NULL;

    fbe_lun_get_block_edge(lun_p, &block_edge_p);    
    *offset_p = fbe_block_transport_edge_get_offset(block_edge_p);

    return;
} // End fbe_lun_get_offset()

static __inline fbe_bool_t fbe_lun_is_raw(fbe_ext_pool_lun_t *lun_p)
{
    fbe_object_id_t object_id;
    fbe_object_id_t rm_np_object_id;
    fbe_object_id_t rm_config_object_id;

    fbe_base_object_get_object_id((fbe_base_object_t*)lun_p, &object_id);

    fbe_database_get_raw_mirror_config_lun_id(&rm_config_object_id);
    fbe_database_get_raw_mirror_metadata_lun_id(&rm_np_object_id);
    return ((object_id == rm_config_object_id) || (object_id == rm_np_object_id));
}
static __inline void fbe_lun_get_num_unexpected_errors(fbe_ext_pool_lun_t *lun_p,
                                                       fbe_u32_t *num_unexpected_errors_p)
{
    *num_unexpected_errors_p = lun_p->num_unexpected_errors;
}
static __inline void fbe_lun_reset_num_unexpected_errors(fbe_ext_pool_lun_t *lun_p)
{
    lun_p->num_unexpected_errors = 0;
}
static __inline fbe_u32_t fbe_lun_inc_num_unexpected_errors(fbe_ext_pool_lun_t *lun_p)
{
    return fbe_atomic_32_increment(&lun_p->num_unexpected_errors);
}
/* accessors for flags.
 */
static __forceinline fbe_bool_t fbe_lun_is_flag_set(fbe_ext_pool_lun_t *lun_p,
                                                    fbe_lun_flags_t flags)
{
    return((lun_p->flags & flags) == flags);
}
static __forceinline void fbe_lun_init_flags(fbe_ext_pool_lun_t *lun_p)
{
    lun_p->flags = 0;
    return;
}
static __forceinline void fbe_lun_set_flag(fbe_ext_pool_lun_t *lun_p,
                                           fbe_lun_flags_t flags)
{
    lun_p->flags |= flags;
}

static __forceinline void fbe_lun_clear_flag(fbe_ext_pool_lun_t *lun_p,
                                             fbe_lun_flags_t flags)
{
    lun_p->flags &= ~flags;
}
static __forceinline fbe_bool_t fbe_lun_is_clean_dirty_enabled(fbe_ext_pool_lun_t *lun_p)
{
    return((lun_p->flags & FBE_LUN_FLAGS_DISABLE_CLEAN_DIRTY) == 0);
}
static __forceinline fbe_bool_t fbe_lun_is_clean_dirty_disabled(fbe_ext_pool_lun_t *lun_p)
{
    return((lun_p->flags & FBE_LUN_FLAGS_DISABLE_CLEAN_DIRTY) == FBE_LUN_FLAGS_DISABLE_CLEAN_DIRTY);
}
/* Imports for visibility.
 */

/* fbe_lun_main.c */
fbe_base_config_downstream_health_state_t fbe_ext_pool_lun_verify_downstream_health (fbe_ext_pool_lun_t * lun_p);
fbe_status_t fbe_ext_pool_lun_set_condition_based_on_downstream_health (fbe_ext_pool_lun_t *lun_p, 
                                                               fbe_base_config_downstream_health_state_t downstream_health_state);
fbe_status_t fbe_ext_pool_lun_reset_unexpected_errors(fbe_ext_pool_lun_t* lun_p);

/* fbe_lun_monitor.c */
fbe_status_t fbe_ext_pool_lun_monitor_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet_p);
fbe_status_t fbe_ext_pool_lun_monitor_load_verify(void);
fbe_status_t fbe_ext_pool_lun_handle_shutdown(fbe_ext_pool_lun_t * const lun_p);
fbe_lifecycle_status_t 
fbe_ext_pool_lun_pending_func(fbe_base_object_t *base_object_p, fbe_packet_t * packet);
fbe_status_t fbe_ext_pool_lun_check_hook(fbe_ext_pool_lun_t *lun_p,
                                fbe_u32_t state,
                                fbe_u32_t substate,
                                fbe_u64_t val2,
                                fbe_scheduler_hook_status_t *status);


/* fbe_lun_usurper.c */
fbe_status_t fbe_ext_pool_lun_control_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet);
fbe_status_t fbe_ext_pool_lun_usurper_allocate_and_build_control_operation(
                                fbe_ext_pool_lun_t*                              in_lun_p,
                                fbe_packet_t*                           in_packet_p,
                                fbe_payload_control_operation_opcode_t  in_op_code,
                                fbe_payload_control_buffer_t            in_buffer_p,
                                fbe_payload_control_buffer_length_t     in_buffer_length);
void fbe_ext_pool_lun_usurper_add_verify_result_counts(
                                fbe_raid_verify_error_counts_t* in_raid_verify_error_counts_p,
                                fbe_lun_verify_counts_t*        in_lun_verify_counts_p);

fbe_status_t 
fbe_ext_pool_lun_usurper_send_initiate_verify_to_raid(fbe_ext_pool_lun_t* in_lun_p, fbe_packet_t * in_packet, 
                                     fbe_lun_initiate_verify_t* in_initiate_verify_p);

/* fbe_lun_class.c */
fbe_status_t fbe_ext_pool_lun_class_control_entry(fbe_packet_t * packet);
fbe_status_t fbe_ext_pool_lun_class_set_resource_priority(fbe_packet_t * packet_p);
fbe_status_t fbe_ext_pool_lun_calculate_cache_zero_bit_map_size(fbe_block_count_t lun_capacity, fbe_block_count_t *blocks);
fbe_status_t fbe_ext_pool_lun_calculate_cache_zero_bit_map_size_to_remove(fbe_block_count_t lun_capacity, fbe_block_count_t *blocks);

/* fbe_lun_executor.c */
fbe_status_t fbe_ext_pool_lun_io_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet_p);
fbe_status_t fbe_ext_pool_lun_block_transport_entry(fbe_transport_entry_context_t context, fbe_packet_t * packet);
fbe_status_t fbe_ext_pool_lun_send_io_packet(fbe_ext_pool_lun_t * const lun_p, fbe_packet_t *packet_p);
fbe_status_t fbe_ext_pool_lun_io_completion(fbe_packet_t * packet, fbe_packet_completion_context_t in_context);
fbe_status_t fbe_ext_pool_lun_noncached_io_completion(fbe_packet_t* in_packet_p, 
                                   fbe_packet_completion_context_t in_context);
fbe_status_t fbe_ext_pool_lun_set_dirty_flag_metadata(fbe_packet_t* packet_p,
                                             fbe_ext_pool_lun_t * lun_p);

fbe_status_t fbe_ext_pool_lun_set_dirty_flag_completion(fbe_packet_t*  in_packet_p,
                                                        fbe_packet_completion_context_t in_context);
                                                        
fbe_status_t fbe_ext_pool_lun_send_packet( fbe_ext_pool_lun_t*  in_lun_p,
                                  fbe_packet_t*  in_packet_p);

fbe_status_t fbe_ext_pool_lun_update_dirty_flag(fbe_lun_clean_dirty_context_t * context,
                                       fbe_packet_t*   in_packet_p,
                                       fbe_packet_completion_function_t completion_function);    
fbe_status_t fbe_ext_pool_lun_mark_peer_clean(fbe_ext_pool_lun_t * lun_p, 
                                      fbe_packet_t *packet_p, 
                                      fbe_packet_completion_function_t completion_function);
fbe_status_t fbe_ext_pool_lun_mark_local_clean(fbe_ext_pool_lun_t * lun_p, 
                                      fbe_packet_t *packet_p, 
                                      fbe_packet_completion_function_t completion_function);
fbe_status_t fbe_ext_pool_lun_handle_queued_packets_waiting_for_clear_dirty_flag(fbe_ext_pool_lun_t*  lun_p);
fbe_status_t fbe_ext_pool_lun_handle_queued_packets_waiting_for_set_dirty_flag(fbe_ext_pool_lun_t*  lun_p,
                                                                      fbe_packet_t* in_packet_p);

              
fbe_status_t fbe_ext_pool_lun_get_dirty_flag(fbe_lun_clean_dirty_context_t * context,
                                    fbe_packet_t*   in_packet_p,
                                    fbe_packet_completion_function_t completion_function);
static fbe_status_t
fbe_ext_pool_lun_mark_lun_clean_completion(fbe_packet_t * packet_p,
                                  fbe_packet_completion_context_t context);


/* fbe_lun_event.c */
fbe_status_t fbe_ext_pool_lun_event_entry(fbe_object_handle_t object_handle, 
                                           fbe_event_type_t event,
                                           fbe_event_context_t event_context);


/* fbe_lun_metadata.c */
fbe_bool_t fbe_ext_pool_lun_metadata_is_initial_verify_needed(fbe_ext_pool_lun_t *lun_p);
fbe_bool_t fbe_ext_pool_lun_metadata_was_initial_verify_run(fbe_ext_pool_lun_t *lun_p);
fbe_status_t fbe_ext_pool_lun_metadata_initialize_metadata_element(fbe_ext_pool_lun_t * lun_p, fbe_packet_t * packet_p);
fbe_status_t fbe_ext_pool_lun_metadata_write_default_nonpaged_metadata(fbe_ext_pool_lun_t * lun_p,fbe_packet_t *packet_p);
fbe_status_t fbe_ext_pool_lun_nonpaged_metadata_init(fbe_ext_pool_lun_t * lun_p, fbe_packet_t *packet_p);
fbe_status_t fbe_ext_pool_lun_metadata_get_zero_checkpoint(fbe_ext_pool_lun_t * lun_p, fbe_lba_t * zero_checkpoint_p);
fbe_status_t  fbe_ext_pool_lun_metadata_set_zero_checkpoint(fbe_ext_pool_lun_t * lun_p, 
                                                   fbe_packet_t * packet_p,
                                                   fbe_lba_t zero_checkpoint,
                                                   fbe_bool_t persist_checkpoint);

fbe_status_t fbe_ext_pool_lun_metadata_set_lun_has_been_written(fbe_ext_pool_lun_t * lun_p, fbe_packet_t * packet_p);
fbe_status_t fbe_ext_pool_lun_metadata_nonpaged_set_generation_num(fbe_base_config_t * base_config,
                                                          fbe_packet_t * packet,
                                                          fbe_u64_t   metadata_offset,
                                                          fbe_u8_t *  metadata_record_data,
                                                          fbe_u32_t metadata_record_data_size);
fbe_status_t fbe_ext_pool_lun_metadata_set_nonpaged_metadata(fbe_ext_pool_lun_t * lun_p,
                                                    fbe_packet_t *packet_p);

fbe_bool_t fbe_ext_pool_lun_is_peer_clustered_flag_set(fbe_ext_pool_lun_t * lun_p, fbe_lun_clustered_flags_t flags);
fbe_status_t fbe_ext_pool_lun_set_clustered_flag(fbe_ext_pool_lun_t * lun_p, fbe_lun_clustered_flags_t flags);
fbe_bool_t fbe_ext_pool_lun_is_clustered_flag_set(fbe_ext_pool_lun_t * lun_p, fbe_lun_clustered_flags_t flags);
fbe_status_t fbe_ext_pool_lun_clear_clustered_flag(fbe_ext_pool_lun_t * lun_p, fbe_lun_clustered_flags_t flags);
fbe_status_t lun_ext_pool_process_memory_update_message(fbe_ext_pool_lun_t * lun_p);

/* fbe_lun_zero.c */
fbe_status_t fbe_ext_pool_lun_zero_start_lun_zeroing_packet_allocation(fbe_ext_pool_lun_t * lun_p, fbe_packet_t * packet_p);
fbe_status_t fbe_ext_pool_lun_zero_get_raid_extent_zero_checkpoint(fbe_ext_pool_lun_t * lun_p, fbe_packet_t * packet_p);
fbe_status_t fbe_ext_pool_lun_zero_calculate_zero_percent_for_lu_extent(fbe_ext_pool_lun_t * lun_p,
                                                               fbe_lba_t   lu_zero_checkpoint_p,
                                                               fbe_u16_t * lu_zero_percentage_p);
fbe_status_t fbe_ext_pool_lun_zero_calculate_zero_checkpoint_for_lu_extent(fbe_ext_pool_lun_t * lun_p,
                                                                  fbe_lba_t   rg_zero_checkpoint,
                                                                  fbe_lba_t * lu_zero_checkpoint_p);
/* fbe_lun_event_log.c */
fbe_status_t fbe_ext_pool_lun_event_log_lun_zero_start_or_complete(fbe_ext_pool_lun_t *     lun_p,
                                                          fbe_bool_t      is_lun_zero_start
                                                          );
fbe_status_t fbe_ext_pool_lun_event_log_event(fbe_ext_pool_lun_t *     lun_p,
                                            fbe_event_event_log_request_t*  event_log_data_p);

/* fbe_lun_perf_stats.c */
fbe_status_t
fbe_ext_pool_lun_perf_stats_update_perf_counters_io_completion(fbe_ext_pool_lun_t * lun_p,
                                                      fbe_payload_block_operation_t *block_operation_p,
                                                      fbe_cpu_id_t cpu_id,
                                                      fbe_payload_ex_t *payload_p);
fbe_status_t
fbe_ext_pool_lun_perf_stats_inc_cum_read_time(fbe_ext_pool_lun_t * lun_p,
                                     fbe_u64_t elapsed_time,
                                     fbe_cpu_id_t cpu_id);
fbe_status_t
fbe_ext_pool_lun_perf_stats_get_cum_read_time(fbe_ext_pool_lun_t * lun_p,
                                     fbe_u64_t *cum_read_time_p,
                                     fbe_cpu_id_t cpu_id);
fbe_status_t
fbe_ext_pool_lun_perf_stats_inc_cum_write_time(fbe_ext_pool_lun_t * lun_p,
                                     fbe_u64_t elapsed_time,
                                     fbe_cpu_id_t cpu_id);
fbe_status_t
fbe_ext_pool_lun_perf_stats_get_cum_write_time(fbe_ext_pool_lun_t * lun_p,
                                      fbe_u64_t *cum_write_time_p,
                                      fbe_cpu_id_t cpu_id);
fbe_status_t
fbe_ext_pool_lun_perf_stats_inc_blocks_read(fbe_ext_pool_lun_t * lun_p,
                                   fbe_u64_t blocks,
                                   fbe_cpu_id_t cpu_id);
fbe_status_t
fbe_ext_pool_lun_perf_stats_get_blocks_read(fbe_ext_pool_lun_t * lun_p,
                                   fbe_u64_t *blocks_read_p,
                                   fbe_cpu_id_t cpu_id);
fbe_status_t
fbe_ext_pool_lun_perf_stats_inc_blocks_written(fbe_ext_pool_lun_t * lun_p,
                                      fbe_u64_t blocks,
                                      fbe_cpu_id_t cpu_id);
fbe_status_t
fbe_ext_pool_lun_perf_stats_get_blocks_written(fbe_ext_pool_lun_t * lun_p,
                                      fbe_u64_t *blocks_written_p,
                                      fbe_cpu_id_t cpu_id);
fbe_status_t
fbe_ext_pool_lun_perf_stats_inc_read_requests(fbe_ext_pool_lun_t * lun_p,
                                     fbe_cpu_id_t cpu_id);
fbe_status_t
fbe_ext_pool_lun_perf_stats_get_read_requests(fbe_ext_pool_lun_t * lun_p,
                                     fbe_u64_t * read_requests_p,
                                     fbe_cpu_id_t cpu_id);
fbe_status_t
fbe_ext_pool_lun_perf_stats_inc_write_requests(fbe_ext_pool_lun_t * lun_p,
                                     fbe_cpu_id_t cpu_id);
fbe_status_t
fbe_ext_pool_lun_perf_stats_get_write_requests(fbe_ext_pool_lun_t * lun_p,
                                     fbe_u64_t * write_requests_p,
                                     fbe_cpu_id_t cpu_id);
fbe_status_t
fbe_ext_pool_lun_perf_stats_inc_arrivals_to_nonzero_q(fbe_ext_pool_lun_t * lun_p,
                                             fbe_cpu_id_t cpu_id);
fbe_status_t
fbe_ext_pool_lun_perf_stats_get_arrivals_to_nonzero_q(fbe_ext_pool_lun_t * lun_p,
                                             fbe_u64_t *arrivals_to_nonzero_q_p,
                                             fbe_cpu_id_t cpu_id);
fbe_status_t
fbe_ext_pool_lun_perf_stats_inc_sum_q_length_arrival(fbe_ext_pool_lun_t * lun_p,
                                            fbe_u64_t blocks,
                                            fbe_cpu_id_t cpu_id);
fbe_status_t
fbe_ext_pool_lun_perf_stats_get_sum_q_length_arrival(fbe_ext_pool_lun_t * lun_p,
                                            fbe_u64_t *sum_q_length_arrival_p,
                                            fbe_cpu_id_t cpu_id);
fbe_status_t
fbe_ext_pool_lun_perf_stats_inc_read_histogram(fbe_ext_pool_lun_t * lun_p,
                                      fbe_u32_t hist_index,
                                      fbe_cpu_id_t cpu_id);
fbe_status_t
fbe_ext_pool_lun_perf_stats_get_read_histogram(fbe_ext_pool_lun_t * lun_p, 
                                      fbe_u32_t hist_index,
                                      fbe_u64_t *read_histogram_p,
                                      fbe_cpu_id_t cpu_id);
fbe_status_t
fbe_ext_pool_lun_perf_stats_inc_write_histogram(fbe_ext_pool_lun_t * lun_p,
                                       fbe_u32_t hist_index,
                                       fbe_cpu_id_t cpu_id);
fbe_status_t
fbe_ext_pool_lun_perf_stats_get_write_histogram(fbe_ext_pool_lun_t * lun_p, 
                                      fbe_u32_t hist_index,
                                      fbe_u64_t *write_histogram_p,
                                      fbe_cpu_id_t cpu_id);

fbe_status_t 
fbe_ext_pool_lun_metadata_write_default_paged_metadata_completion(fbe_packet_t * packet_p,
                                                         fbe_packet_completion_context_t context);

static fbe_status_t 
fbe_ext_pool_lun_monitor_initiate_lun_dirty_verify_cond_function_completion(fbe_packet_t * in_packet_p,
                                                                   fbe_packet_completion_context_t in_context);

fbe_status_t 
fbe_ext_pool_lun_metadata_write_default_paged_metadata(fbe_ext_pool_lun_t * lun_p,
                                              fbe_packet_t *packet_p);

fbe_bool_t fbe_ext_pool_lun_is_unexpected_error_limit_hit(fbe_ext_pool_lun_t *lun_p);
/* LUN Join */

fbe_lifecycle_status_t 
fbe_ext_pool_lun_join_request(fbe_ext_pool_lun_t * lun_p, fbe_packet_t * packet_p);

fbe_lifecycle_status_t 
fbe_ext_pool_lun_join_passive(fbe_ext_pool_lun_t * lun_p, fbe_packet_t * packet_p);

fbe_lifecycle_status_t 
fbe_ext_pool_lun_join_active(fbe_ext_pool_lun_t * lun_p, fbe_packet_t * packet_p);

fbe_lifecycle_status_t 
fbe_ext_pool_lun_join_done(fbe_ext_pool_lun_t * lun_p, fbe_packet_t * packet_p);

fbe_lifecycle_status_t 
fbe_ext_pool_lun_join_sync_complete(fbe_ext_pool_lun_t * lun_p, fbe_packet_t * packet_p);


fbe_bool_t fbe_ext_pool_lun_is_unexpected_error_limit_hit(fbe_ext_pool_lun_t *lun_p);
fbe_status_t fbe_ext_pool_lun_eval_err_request(fbe_ext_pool_lun_t *lun_p);
fbe_status_t fbe_ext_pool_lun_eval_err_passive(fbe_ext_pool_lun_t *lun_p);
fbe_status_t fbe_ext_pool_lun_eval_err_active(fbe_ext_pool_lun_t *lun_p);

#endif /* EXT_POOL_LUN_PRIVATE_H */

/*******************************
 * end fbe_ext_pool_lun_private.h
 *******************************/
