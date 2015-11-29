#ifndef _FBE_RAID_LIBRARY_API_H_
#define _FBE_RAID_LIBRARY_API_H_

/*******************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 *******************************************************************/

/*******************************************************************
 *  fbe_raid_library_api.h
 *******************************************************************
 *
 * @brief
 *    This file contains the accessor definitions and APIs for accessing 
 *    the raid library routines.
 *
 * @author
 *  10/28/2009  Ron Proulx  - Created from fbe_raid_group_object.h
 *
 *******************************************************************/

/******************************
 * INCLUDES
 ******************************/
#include "csx_ext.h"
#include "fbe/fbe_transport.h"
#include "fbe_block_transport.h"
#include "fbe_raid_geometry.h"
#include "fbe/fbe_api_lun_interface.h"
#include "fbe/fbe_perfstats_interface.h"


/******************************
 * LITERAL DEFINITIONS.
 ******************************/
/*! @note  Notice that there is no dependancy on the raid group objects. 
 *         It is assumed that separate checks will be performed to validate
 *         that any configured parameters are valid for that class.
 */

/*! @def FBE_RAID_LIBRARY_RAID3_MIN_WIDTH
 *  @brief Minimum RAID-3 width (could be less but we maintain previous limits)
 */
#define FBE_RAID_LIBRARY_RAID3_MIN_WIDTH 5
/*! @def FBE_RAID_LIBRARY_RAID3_MAX_WIDTH
 *  @brief Maximum RAID-3 width (could be more but we maintain previous limits)
 */
#define FBE_RAID_LIBRARY_RAID3_MAX_WIDTH 9
/*! @def FBE_RAID_LIBRARY_RAID3_VAULT_WIDTH
 *  @brief Vault RAID-3 width
 */
#define FBE_RAID_LIBRARY_RAID3_VAULT_WIDTH 4

/*! @def FBE_RAID_LIBRARY_RAID5_MIN_WIDTH
 *  @brief Minimum RAID-5 width (2 + 1)
 */
#define FBE_RAID_LIBRARY_RAID5_MIN_WIDTH 3
/*! @def FBE_RAID_LIBRARY_RAID5_MAX_WIDTH
 *  @brief Maximum RAID-5 width (FBE_RAID_MAX_DISK_ARRAY_WIDTH)
 */
#define FBE_RAID_LIBRARY_RAID5_MAX_WIDTH 16

/*! @def FBE_RAID_LIBRARY_RAID0_MIN_WIDTH
 *  @brief Minimum RAID-0 width
 */
#define FBE_RAID_LIBRARY_RAID0_MIN_WIDTH 3
/*! @def FBE_RAID_LIBRARY_RAID0_MAX_WIDTH
 *  @brief Maximum RAID-0 width (FBE_RAID_MAX_DISK_ARRAY_WIDTH)
 */
#define FBE_RAID_LIBRARY_RAID0_MAX_WIDTH 16
/*! @def FBE_RAID_LIBRARY_INDIVIDUAL_DISK_WIDTH
 *  @brief Only width allowed for individual disk is (1)
 */
#define FBE_RAID_LIBRARY_INDIVIDUAL_DISK_WIDTH 1

/*! @def FBE_RAID_LIBRARY_RAID6_MIN_WIDTH
 *  @brief Minimum RAID-6 width
 */
#define FBE_RAID_LIBRARY_RAID6_MIN_WIDTH 4
/*! @def FBE_RAID_LIBRARY_RAID6_MAX_WIDTH
 *  @brief Maximum RAID-6 width (FBE_RAID_MAX_DISK_ARRAY_WIDTH)
 */
#define FBE_RAID_LIBRARY_RAID6_MAX_WIDTH 16

/*! @def FBE_RAID_LIBRARY_RAID10_MIN_WIDTH
 *  @brief Minimum RAID-10 width
 */
#define FBE_RAID_LIBRARY_RAID10_MIN_WIDTH 2
/*! @def FBE_RAID_LIBRARY_RAID10_MAX_WIDTH
 *  @brief Maximum RAID-10 width (FBE_RAID_MAX_DISK_ARRAY_WIDTH)
 */
#define FBE_RAID_LIBRARY_RAID10_MAX_WIDTH 16

/*! @def FBE_RAID_LIBRARY_RAID1_MIN_WIDTH
 *  @brief Minimum RAID-1 width
 */
#define FBE_RAID_LIBRARY_RAID1_MIN_WIDTH 2
/*! @def FBE_RAID_LIBRARY_RAID1_MAX_WIDTH
 *  @brief Maximum RAID-1 width
 */
#define FBE_RAID_LIBRARY_RAID1_MAX_WIDTH 3

/*! @def FBE_RAID_GROUP_MAX_DISK_ARRAY_WIDTH  
 *  @brief Max number of drives we expect. 
 */
#define FBE_RAID_GROUP_MAX_DISK_ARRAY_WIDTH 16

#define FBE_METADATA_IO_NUMBER_OF_COPIES    1
#define FBE_METADATA_IO_WIDTH               (FBE_METADATA_IO_NUMBER_OF_COPIES)

/*! @def FBE_RAID_INVALID_DEGRADED_OFFSET
 *  @brief This is an invalid offset that we use to initialize a checkpoint to a maximum
 *          value meaning completely rebuilt. 
 */
#define FBE_RAID_INVALID_DEGRADED_OFFSET ((fbe_lba_t) -1)

/*! @def FBE_RAID_INVALID_DISK_POSITION
 *
 *  @brief Invalid disk position.  Used when searching for a disk position and no disk is 
 *         found that meets the criteria.
 */
#define FBE_RAID_INVALID_DISK_POSITION ((fbe_u32_t) -1)


/*! @def FBE_RAID_POSITION_BITMASK_ALL
 *
 *  @brief Positions bitmask which has all positions set 
 */
#define FBE_RAID_POSITION_BITMASK_ALL ((fbe_raid_position_bitmask_t) -1)
#define FBE_SEND_IO_TO_RAW_MIRROR_BITMASK        7

/*!*******************************************************************
 * @enum fbe_raid_degraded_factor_t
 *********************************************************************
 * @brief defaults for degraded credits calculations
 *
 *********************************************************************/
typedef enum fbe_raid_degraded_factor_e
{
    FBE_RAID_DEGRADED_PARITY_READ_MULTIPLIER    = 0,
    FBE_RAID_DEGRADED_PARITY_WRITE_MULTIPLIER   = 0,
    FBE_RAID_DEGRADED_MIRROR_READ_MULTIPLIER    = 0,
    FBE_RAID_DEGRADED_MIRROR_WRITE_MULTIPLIER   = 0,
    FBE_RAID_DEGRADED_CREDIT_CEILING_DIVISOR    = 4,
}
fbe_raid_degraded_factor_t;

/*!*******************************************************************
 * @struct fbe_raid_nr_extent_t
 *********************************************************************
 * @brief Defines the elements we expect to get back from a query of
 *        the nr state of each chunk.
 *
 *********************************************************************/
typedef struct fbe_raid_nr_extent_s
{
    fbe_lba_t lba;
    fbe_block_count_t blocks;
    fbe_bool_t b_dirty;
}
fbe_raid_nr_extent_t;

/* This defines the function we use for mocking out calls to fbe_raid_group_get_nr_extent.
 */
typedef fbe_status_t (*fbe_raid_group_get_nr_extent_fn_t)(void *,
                                                          fbe_u32_t position,
                                                          fbe_lba_t lba,
                                                          fbe_block_count_t blocks,
                                                          fbe_raid_nr_extent_t *nr_extent_p,
                                                          fbe_u32_t nr_extents);

/*!*******************************************************************
 * @def FBE_RAID_MAX_NR_EXTENT_COUNT
 *********************************************************************
 * @brief Max number of nr extents we will fill out.
 *
 *********************************************************************/
#define FBE_RAID_MAX_NR_EXTENT_COUNT 4

/*!*******************************************************************
 * @def FBE_RAID_MAX_PARITY_EXTENTS
 *********************************************************************
 * @brief Max number of extents we can create.
 *
 *********************************************************************/
#define FBE_RAID_MAX_PARITY_EXTENTS 2

/*************************
 *   FUNCTION DEFINITIONS
 *************************/
static __inline fbe_u32_t fbe_raid_get_bitcount(fbe_u32_t bitmask)
{
    fbe_u32_t bitcount = 0;

    /* Count total bits set in a bitmask.
     */
    while (bitmask > 0)
    {
        /* Clear off the highest bit.
         */
        bitmask &= bitmask - 1;
        bitcount++;
    }

    return bitcount;
}
static __inline fbe_u32_t fbe_raid_get_first_set_position(fbe_raid_position_bitmask_t position_bitmask)
{
    fbe_u32_t   bit_index = 0;

    /* Find first
     */
    while (bit_index < FBE_RAID_GROUP_MAX_DISK_ARRAY_WIDTH)
    {
        /* Check the current bit_index
         */
        if ((1 << bit_index) & position_bitmask)
        {
            return bit_index;
        }
        bit_index++;
    }

    return FBE_RAID_INVALID_DISK_POSITION;
}
/*!*******************************************************************
 * @struct fbe_raid_memory_stats_t
 *********************************************************************
 * @brief This structure is use to put information about a raid
 *        memory statistics
 * 
 *
 *********************************************************************/
typedef struct fbe_raid_memory_stats_s
{
    /*! This is the number of allocations
     */
    fbe_atomic_t allocations;
    /*! This is the number of frees done
     */
    fbe_atomic_t frees;
    /*! This is the number of bytes allocated
     */
    fbe_atomic_t allocated_bytes;
    /*! This is the number of freed bytes
     */
    fbe_atomic_t freed_bytes;
    /*! This is the number of allocations which are deferred
     */
    fbe_atomic_t deferred_allocations;
    /*! This is the number of allocations which are pending
     */
    fbe_atomic_t pending_allocations;
    /*! This is the number of allocations which got aborted
     */
    fbe_atomic_t aborted_allocations;
    /*! This is the number of allocations which got errors
     */
    fbe_atomic_t allocation_errors;
    /*! This is the number of errors while freeing
     */
    fbe_atomic_t free_errors;           /*! < Indicates a memory leak */
}
fbe_raid_memory_stats_t;

/*************************************** 
 * Imports for visibility.
 ***************************************/


/***************************************
 * fbe_raid_library.c  
 ***************************************/
fbe_status_t fbe_raid_library_initialize(fbe_raid_library_debug_flags_t default_flags);
fbe_status_t fbe_raid_library_destroy(void);
fbe_time_t fbe_raid_library_get_max_io_msecs(void);
void fbe_raid_library_set_max_io_msecs(fbe_time_t msecs);
fbe_bool_t fbe_raid_library_get_encrypted(void);
void fbe_raid_library_set_encrypted(void);
void fbe_raid_iots_get_statistics(fbe_raid_iots_t *iots_p,
                                  fbe_raid_library_statistics_t *stats_p);
/***************************************
 * fbe_raid_util.c  
 ***************************************/
fbe_bool_t fbe_raid_iots_is_quiesced(fbe_raid_iots_t * const iots_p);
fbe_status_t fbe_raid_iots_quiesce_with_lock(fbe_raid_iots_t * const iots_p,
                                             fbe_bool_t *b_quiesced_p);
fbe_status_t fbe_raid_iots_quiesce(fbe_raid_iots_t * const iots_p);
fbe_status_t fbe_raid_siots_continue(fbe_raid_siots_t * const siots_p,
                                     fbe_raid_geometry_t * const raid_geometry_p,
                                     fbe_u16_t continue_bitmask);

fbe_status_t fbe_raid_siots_restart_for_shutdown(fbe_raid_siots_t * const siots_p,
                                                 fbe_queue_head_t *restart_queue_p,
                                                 fbe_u32_t *num_restarted_p);

fbe_status_t fbe_raid_iots_restart_for_shutdown(fbe_raid_iots_t * const iots_p,
                                                fbe_queue_head_t *restart_queue_p,
                                                fbe_u32_t *num_restarted_p);
fbe_status_t fbe_raid_iots_continue(fbe_raid_iots_t * const iots_p,
                                    fbe_u16_t continue_bitmask);

fbe_status_t fbe_raid_iots_get_siots_to_restart(fbe_raid_iots_t * const iots_p,
                                                       fbe_queue_head_t *restart_queue_p,
                                                       fbe_u32_t *num_restarted_p);
fbe_status_t fbe_raid_iots_get_quiesced_ts_to_restart(fbe_raid_iots_t * const iots_p,
                                                      fbe_queue_head_t *restart_queue_p);
fbe_status_t fbe_raid_iots_handle_shutdown(fbe_raid_iots_t *iots_p,
                                           fbe_queue_head_t *restart_queue_p);
fbe_status_t fbe_raid_iots_restart_iots_after_metadata_update(fbe_raid_iots_t *iots_p);
fbe_u32_t fbe_raid_restart_common_queue(fbe_queue_head_t *queue_p);
fbe_status_t fbe_raid_iots_abort_monitor_op(fbe_raid_iots_t * const iots_p);
fbe_status_t fbe_raid_iots_handle_config_change(fbe_raid_iots_t *iots_p,
                                                fbe_queue_head_t *restart_queue_p);
/***************************************
 * fbe_raid_iots.c  
 ***************************************/
fbe_status_t fbe_raid_iots_init(fbe_raid_iots_t *iots_p,
                                fbe_packet_t *packet_p,
                                fbe_payload_block_operation_t *block_operation_p,
                                fbe_raid_geometry_t *raid_geometry_p,
                                fbe_lba_t lba,
                                fbe_block_count_t blocks);
fbe_status_t fbe_raid_iots_fast_init(fbe_raid_iots_t* iots_p);
fbe_status_t fbe_raid_iots_basic_init(fbe_raid_iots_t *iots_p,
									  fbe_packet_t *packet_p,
									  fbe_raid_geometry_t *raid_geometry_p);
fbe_status_t fbe_raid_iots_basic_init_common(fbe_raid_iots_t* iots_p);
fbe_status_t fbe_raid_iots_set_as_not_used(fbe_raid_iots_t * iots_p);
fbe_status_t fbe_raid_iots_start(fbe_raid_iots_t* iots_p);
fbe_bool_t fbe_raid_library_is_opcode_lba_disk_based(fbe_payload_block_operation_opcode_t opcode);
fbe_bool_t fbe_raid_iots_is_request_complete(fbe_raid_iots_t *iots_p);
fbe_bool_t fbe_raid_iots_needs_upgrade(fbe_raid_iots_t *iots_p);
fbe_status_t fbe_raid_iots_lock_upgrade_complete(fbe_raid_iots_t *iots_p);
fbe_status_t fbe_raid_iots_get_next_lba(fbe_raid_iots_t *iots_p,
                                        fbe_lba_t *lba_p,
                                        fbe_block_count_t *blocks_p);
fbe_status_t fbe_raid_iots_init_for_next_lba(fbe_raid_iots_t *iots_p);
fbe_status_t fbe_raid_iots_reinit(fbe_raid_iots_t* iots_p,
                                  fbe_lba_t lba,
                                  fbe_block_count_t blocks);
fbe_status_t fbe_raid_iots_set_packet_status(fbe_raid_iots_t * iots_p);
fbe_status_t fbe_raid_iots_destroy(fbe_raid_iots_t* iots_p);
fbe_status_t fbe_raid_iots_fast_destroy(fbe_raid_iots_t* iots_p);
fbe_status_t fbe_raid_iots_basic_destroy(fbe_raid_iots_t* iots_p);
fbe_status_t fbe_raid_iots_get_capacity(fbe_raid_iots_t * iots_p,
                                        fbe_lba_t *capacity_p);
fbe_status_t fbe_raid_iots_get_chunk_range(fbe_raid_iots_t *iots_p,
                                           fbe_lba_t lba,
                                           fbe_block_count_t blocks,
                                           fbe_chunk_size_t chunk_size,
                                           fbe_chunk_count_t *start_chunk_index_p,
                                           fbe_chunk_count_t *num_chunks_p);
fbe_status_t fbe_raid_iots_get_chunk_lba_blocks(fbe_raid_iots_t *iots_p,
                                                fbe_lba_t lba,
                                                fbe_block_count_t blocks,
                                                fbe_lba_t *lba_p,
                                                fbe_block_count_t *blocks_p,
                                                fbe_chunk_size_t chunk_size);
fbe_bool_t fbe_raid_is_iots_aligned_to_chunk(fbe_raid_iots_t *iots_p,
                                             fbe_chunk_size_t chunk_size);
fbe_bool_t fbe_raid_zero_request_aligned_to_chunk(fbe_raid_iots_t * iots_p, fbe_chunk_size_t chunk_size);

fbe_u32_t fbe_raid_iots_determine_num_siots_to_allocate(fbe_raid_iots_t *iots_p);
fbe_status_t fbe_raid_iots_activate_available_siots(fbe_raid_iots_t *iots_p,
                                                    fbe_raid_siots_t **siots_pp);
fbe_status_t fbe_raid_iots_get_upgraded_lock_range(fbe_raid_iots_t *iots_p,
                                                   fbe_u64_t *stripe_number_p,
                                                   fbe_u64_t *stripe_count_p);
fbe_status_t fbe_raid_iots_determine_next_blocks(fbe_raid_iots_t *iots_p,
                                                 fbe_chunk_size_t chunk_size);
fbe_status_t fbe_raid_iots_limit_blocks_for_degraded(fbe_raid_iots_t *iots_p,
                                                     fbe_chunk_size_t chunk_size);
fbe_status_t fbe_raid_iots_remove_nondegraded(fbe_raid_iots_t *iots_p,
                                              fbe_raid_position_bitmask_t positions_to_check,
                                              fbe_chunk_size_t chunk_size);
fbe_status_t fbe_raid_iots_verify_remove_unmarked(fbe_raid_iots_t *iots_p,fbe_chunk_size_t chunk_size);
void fbe_raid_iots_mark_piece_complete(fbe_raid_iots_t *iots_p);
fbe_bool_t fbe_raid_iots_is_piece_complete(fbe_raid_iots_t *iots_p);
fbe_bool_t fbe_raid_iots_is_metadata_operation(fbe_raid_iots_t *iots_p);
fbe_status_t fbe_raid_get_rekey_pos(fbe_raid_geometry_t *raid_geometry_p,
                                    fbe_raid_position_bitmask_t rekey_bitmask,
                                    fbe_raid_position_t *rekey_pos_p);
fbe_status_t fbe_raid_iots_get_block_edge(fbe_raid_iots_t *iots_p,
                                          fbe_block_edge_t **block_edge_p,
                                          fbe_u32_t position);
fbe_status_t fbe_raid_iots_get_extent_offset(fbe_raid_iots_t *iots_p,
                                             fbe_u32_t position,
                                             fbe_lba_t *offset_p);
/* Get the status this iots should return.
 */
static __forceinline void fbe_raid_iots_get_block_status(fbe_raid_iots_t *const iots_p,
                                                  fbe_payload_block_operation_status_t *status_p)
{
    *status_p = iots_p->error;
    return;
}
/* Set the status this iots should return.
 */
static __forceinline void fbe_raid_iots_set_block_status(fbe_raid_iots_t *const iots_p,
                                                  fbe_payload_block_operation_status_t status)
{
    iots_p->error = status;
    return;
}

/* Get the qualifier this iots should return.
 */
static __forceinline void fbe_raid_iots_get_block_qualifier(fbe_raid_iots_t *const iots_p,
                                                     fbe_payload_block_operation_qualifier_t *status_p)
{
    *status_p = iots_p->qualifier;
    return;
}
/* Set the qualifier this iots should return.
 */
static __forceinline void fbe_raid_iots_set_block_qualifier(fbe_raid_iots_t *const iots_p,
                                                     fbe_payload_block_operation_qualifier_t status)
{
    iots_p->qualifier = status;
    return;
}

static __forceinline struct fbe_payload_block_operation_s * fbe_raid_iots_get_block_operation(fbe_raid_iots_t * const iots_p)
{
    return iots_p->block_operation_p;
}
/* Accessors for the active block operation.
 */
static __forceinline void fbe_raid_iots_set_block_operation(fbe_raid_iots_t * const iots_p,
                                                     fbe_payload_block_operation_t *block_operation_p)
{
    iots_p->block_operation_p = block_operation_p;
    return;
}

/* Accessors for packet we are embedded inside of.
 */
static __forceinline void fbe_raid_iots_set_packet(fbe_raid_iots_t * const iots_p,
                                            fbe_packet_t *packet_p)
{
    iots_p->packet_p = packet_p;
    return;
}
static __forceinline struct fbe_packet_s * fbe_raid_iots_get_packet(fbe_raid_iots_t * const iots_p)
{
    return iots_p->packet_p;
}

static __forceinline void fbe_raid_iots_set_geometry(fbe_raid_iots_t *iots_p, fbe_raid_geometry_t *raid_geometry_p)
{
    iots_p->raid_geometry_p = raid_geometry_p;
}
static __forceinline void fbe_raid_iots_set_callback(fbe_raid_iots_t * const iots_p,
                                              fbe_raid_iots_completion_function_t callback_p,
                                              fbe_raid_iots_completion_context_t context)
{
    iots_p->callback = callback_p;
    iots_p->callback_context = context;
    return;
}
static __forceinline void fbe_raid_iots_get_rebuild_logging_bitmask(fbe_raid_iots_t *const iots_p,
                                                             fbe_raid_position_bitmask_t *bitmask_p)
{
    *bitmask_p = iots_p->rebuild_logging_bitmask;
    return;
}
static __forceinline void fbe_raid_iots_set_rebuild_logging_bitmask(fbe_raid_iots_t *const iots_p,
                                                             fbe_raid_position_bitmask_t bitmask)
{
    iots_p->rebuild_logging_bitmask = bitmask;
    return;
}
static __forceinline void fbe_raid_iots_get_status(fbe_raid_iots_t *iots_p, fbe_raid_iots_status_t *status_p) 
{
    *status_p = iots_p->status;
    return;
}
static __forceinline void fbe_raid_iots_set_status(fbe_raid_iots_t *iots_p, fbe_raid_iots_status_t status) 
{
    iots_p->status = status;
    return;
}
static __forceinline void fbe_raid_iots_get_cpu_id(fbe_raid_iots_t *iots_p, fbe_cpu_id_t *cpu_id){
    fbe_transport_get_cpu_id(iots_p->packet_p, cpu_id);
    return;
}
static __forceinline void fbe_raid_iots_set_cpu_id(fbe_raid_iots_t *iots_p, fbe_cpu_id_t cpu_id){
    fbe_transport_set_cpu_id(iots_p->packet_p, cpu_id);
    return;
}

typedef fbe_raid_state_status_t(*fbe_raid_iots_state_t) (struct fbe_raid_iots_s *);
static __forceinline void fbe_raid_iots_set_state(fbe_raid_iots_t *iots_p, fbe_raid_iots_state_t state)
{
    fbe_raid_common_set_state(&iots_p->common, (fbe_raid_common_state_t)state);
}

/* Accessor for the iots lba, which is the start of the iots.
 */
static __forceinline void fbe_raid_iots_get_lba(fbe_raid_iots_t *const iots_p,
                                         fbe_lba_t *lba_p)
{
    *lba_p = iots_p->lba;
    return;
}
static __forceinline void fbe_raid_iots_set_lba(fbe_raid_iots_t *const iots_p,
                                         fbe_lba_t lba)
{
    iots_p->lba = lba;
    return;
}
/* Accessor for the iots blocks, which is the total range of the iots.
 */ 
static __forceinline void fbe_raid_iots_get_blocks(fbe_raid_iots_t *const iots_p,
                                            fbe_block_count_t *blocks_p)
{
    *blocks_p = iots_p->blocks;
    return;
}
static __forceinline void fbe_raid_iots_set_blocks(fbe_raid_iots_t *const iots_p,
                                            fbe_block_count_t blocks)
{
    iots_p->blocks = blocks;
    return;
}
/* Blocks remaining accessor.
 */
static __forceinline void fbe_raid_iots_get_blocks_remaining(fbe_raid_iots_t *const iots_p,
                                                      fbe_block_count_t *blocks_p)
{
    *blocks_p = iots_p->blocks_remaining;
    return;
}
static __forceinline void fbe_raid_iots_set_blocks_remaining(fbe_raid_iots_t *const iots_p,
                                                      fbe_block_count_t blocks)
{
    iots_p->blocks_remaining = blocks;
    return;
}

/* Accessor for the iots current op lba, which is the start of the iots.
 */
static __forceinline void fbe_raid_iots_get_current_op_lba(fbe_raid_iots_t *const iots_p,
                                                    fbe_lba_t *lba_p)
{
    *lba_p = iots_p->current_operation_lba;
    return;
}
static __forceinline void fbe_raid_iots_set_current_op_lba(fbe_raid_iots_t *const iots_p,
                                                    fbe_lba_t lba)
{
    iots_p->current_operation_lba = lba;
    return;
}
/* Accessor for the iots current op blocks, which is the total range of the iots.
 */ 
static __forceinline void fbe_raid_iots_get_current_op_blocks(fbe_raid_iots_t *const iots_p,
                                                       fbe_block_count_t *blocks_p)
{
    *blocks_p = iots_p->current_operation_blocks;
    return;
}
static __forceinline void fbe_raid_iots_set_current_op_blocks(fbe_raid_iots_t *const iots_p,
                                                       fbe_block_count_t blocks)
{
    iots_p->current_operation_blocks = blocks;
    return;
}
/* Current lba accessor.
 */
static __forceinline void fbe_raid_iots_get_current_lba(fbe_raid_iots_t *const iots_p,
                                                 fbe_lba_t *lba_p)
{
    *lba_p = iots_p->current_lba;
    return;
}
static __forceinline void fbe_raid_iots_set_current_lba(fbe_raid_iots_t *const iots_p,
                                                 fbe_lba_t lba)
{
    iots_p->current_lba = lba;
    return;
}
static __forceinline void fbe_raid_iots_get_opcode(fbe_raid_iots_t *const iots_p,
                                            fbe_payload_block_operation_opcode_t *opcode_p)
{
    fbe_payload_block_get_opcode(iots_p->block_operation_p, opcode_p);
    return;
}

/*!*******************************************************************
 * @def FBE_RAID_COMMON_MDS_RESOURCE_PRIORITY_OFFSET
 *********************************************************************
 * @brief Resource priority boost for metadata requests relative 
 * to IOTS resource priority.
 *********************************************************************/
#define FBE_RAID_COMMON_MDS_RESOURCE_PRIORITY_BOOST (2)

static __forceinline void fbe_raid_iots_get_packet_resource_priority(fbe_raid_iots_t *const iots_p,
                                                              fbe_packet_resource_priority_t *packet_resource_priority_p)
{
    if (iots_p->packet_p == NULL)
    {
        *packet_resource_priority_p = FBE_RAID_COMMON_MDS_RESOURCE_PRIORITY_BOOST;
    }
    else
    {
        *packet_resource_priority_p = fbe_transport_get_resource_priority(iots_p->packet_p);
    }
    return;
}
static __forceinline void fbe_raid_iots_adjust_packet_resource_priority(fbe_raid_iots_t *const iots_p)
{
    if ((iots_p->packet_p != NULL) && 
        ((iots_p->packet_p->resource_priority_boost & FBE_TRANSPORT_RESOURCE_PRIORITY_BOOST_FLAG_RG) == 0))
    {
        fbe_packet_resource_priority_t packet_priority;
        /* Packet priority is used by metadata service. We want MDS request to be at
         * a higher priority than iots/siots/fruts. So give packet resource priroity 
         * a boost. Also set the flag to indicate that priority is boosted.  
         */
        iots_p->packet_p->resource_priority_boost |= FBE_TRANSPORT_RESOURCE_PRIORITY_BOOST_FLAG_RG;    
        packet_priority = fbe_transport_get_resource_priority(iots_p->packet_p);
        packet_priority += FBE_RAID_COMMON_MDS_RESOURCE_PRIORITY_BOOST;
        fbe_transport_set_resource_priority(iots_p->packet_p, packet_priority);     
    }
}
static __forceinline void fbe_raid_iots_get_packet_io_stamp(fbe_raid_iots_t *const iots_p,
                                                     fbe_packet_io_stamp_t *packet_io_stamp_p)
{
    if (iots_p->packet_p == NULL)
    {
        *packet_io_stamp_p = FBE_PACKET_IO_STAMP_INVALID;
    }
    else
    {
        *packet_io_stamp_p = fbe_transport_get_io_stamp(iots_p->packet_p);
    }
    return;
}

/* Accessor for the iots block operation's performance stats structure.
 */ 
static __forceinline void fbe_raid_iots_get_performance_stats(fbe_raid_iots_t *const iots_p,
                                                       fbe_lun_performance_counters_t **perf_stats_p)
{
	fbe_packet_t * packet = NULL;
	fbe_payload_ex_t * payload = NULL;

	packet = fbe_raid_iots_get_packet(iots_p);
	payload = fbe_transport_get_payload_ex(packet);
    fbe_payload_ex_get_lun_performace_stats_ptr(payload, (void **)perf_stats_p);
    return;
}

static __forceinline void fbe_raid_iots_set_performance_stats(fbe_raid_iots_t *const iots_p,
                                                       fbe_lun_performance_counters_t *perf_stats_p)
{
	fbe_packet_t * packet = NULL;
	fbe_payload_ex_t * payload = NULL;

	packet = fbe_raid_iots_get_packet(iots_p);
	payload = fbe_transport_get_payload_ex(packet);
	fbe_payload_ex_set_lun_performace_stats_ptr(payload, perf_stats_p);
    return;
}

/* Accessor for the iots chunk lba.
 */ 
static __forceinline void fbe_raid_iots_get_chunk_lba(fbe_raid_iots_t *const iots_p,
                                               fbe_lba_t *chunk_lba_p)
{
    *chunk_lba_p = iots_p->start_chunk_lba;
    return;
}
static __forceinline void fbe_raid_iots_set_chunk_lba(fbe_raid_iots_t *const iots_p,
                                               fbe_lba_t chunk_lba)
{
    iots_p->start_chunk_lba = chunk_lba;
    return;
}
/* Accessor for the iots chunk size.
 */ 
static __forceinline void fbe_raid_iots_get_chunk_size(fbe_raid_iots_t *const iots_p,
                                                fbe_chunk_size_t *chunk_size_p)
{
    *chunk_size_p = iots_p->chunk_size;
    return;
}
static __forceinline void fbe_raid_iots_set_chunk_size(fbe_raid_iots_t *const iots_p,
                                                fbe_chunk_size_t chunk_size)
{
    iots_p->chunk_size = chunk_size;
    return;
}

static __forceinline void fbe_raid_iots_init_flags(fbe_raid_iots_t *iots_p,
                                            fbe_raid_iots_flags_t flags)
{
    iots_p->flags = flags;
    return;
}

static __forceinline void fbe_raid_iots_set_flag(fbe_raid_iots_t *iots_p,
                                          fbe_raid_iots_flags_t flag)
{
    iots_p->flags |= flag;
    return;
}

static __forceinline void fbe_raid_iots_clear_flag(fbe_raid_iots_t *iots_p,
                                            fbe_raid_iots_flags_t flag)
{
    iots_p->flags &= ~flag;
    return;
}

static __forceinline fbe_raid_iots_flags_t fbe_raid_iots_get_flags(fbe_raid_iots_t *iots_p)
{
    return(iots_p->flags);
}

static __forceinline fbe_bool_t fbe_raid_iots_is_flag_set(fbe_raid_iots_t *iots_p,
                                                   fbe_raid_iots_flags_t flags)
{
    return ((iots_p->flags & flags) == flags);
}

static __forceinline fbe_bool_t fbe_raid_iots_is_any_flag_set(fbe_raid_iots_t *iots_p,
                                                       fbe_raid_iots_flags_t flags)
{
    return ((iots_p->flags & flags)!=0);
}

static __forceinline void fbe_raid_iots_get_packet_lba(fbe_raid_iots_t *const iots_p,
                                                fbe_lba_t *lba_p)
{
    *lba_p = iots_p->packet_lba;
    return;
}
static __forceinline void fbe_raid_iots_set_packet_lba(fbe_raid_iots_t *const iots_p,
                                                fbe_lba_t lba)
{
    iots_p->packet_lba = lba;
    return;
}
static __forceinline void fbe_raid_iots_get_packet_blocks(fbe_raid_iots_t *const iots_p,
                                                   fbe_block_count_t *blocks_p)
{
    *blocks_p = iots_p->packet_blocks;
    return;
}
static __forceinline void fbe_raid_iots_set_packet_blocks(fbe_raid_iots_t *const iots_p,
                                                   fbe_block_count_t blocks)
{
    iots_p->packet_blocks = blocks;
    return;

}
static __forceinline void fbe_raid_iots_set_current_opcode(fbe_raid_iots_t *const iots_p,
                                                    fbe_payload_block_operation_opcode_t current_opcode)
{
    iots_p->current_opcode = current_opcode;
    return;
}
static __forceinline void fbe_raid_iots_get_current_opcode(fbe_raid_iots_t *const iots_p,
                                                    fbe_payload_block_operation_opcode_t *opcode_p)
{
    *opcode_p = iots_p->current_opcode;
    return;
}

static __forceinline void fbe_raid_iots_set_np_lock(fbe_raid_iots_t *const iots_p,
                                             fbe_bool_t     np_lock)
                               
{
    iots_p->np_lock = np_lock;
    return;
}
static __forceinline void fbe_raid_iots_get_np_lock(fbe_raid_iots_t *const iots_p,
                                             fbe_bool_t *np_lock_p)
{
    *np_lock_p = iots_p->np_lock;
    return;
}

static __forceinline void fbe_raid_iots_clear_np_lock(fbe_raid_iots_t *const iots_p)
{
    iots_p->np_lock = FBE_FALSE;
    return;
}

static __forceinline void fbe_raid_iots_set_host_start_offset(fbe_raid_iots_t *const iots_p,
                                                       fbe_block_count_t offset)
{
    iots_p->host_start_offset = offset;
    return;
}

static __forceinline void fbe_raid_iots_set_extent(fbe_raid_iots_t *const iots_p,
                                                   void *extent_p,
                                                   void *extent_context_p)
{
    iots_p->extent_p = extent_p;
    iots_p->extent_context_p = extent_context_p;
    return;
}


void fbe_raid_iots_lock(fbe_raid_iots_t *iots_p);
void fbe_raid_iots_unlock(fbe_raid_iots_t *iots_p);

fbe_status_t fbe_raid_iots_validate_chunk_info(fbe_raid_iots_t *iots_p,
                                               fbe_lba_t lba,
                                               fbe_block_count_t blocks);
fbe_status_t fbe_raid_iots_mark_unquiesced(fbe_raid_iots_t *iots_p);
fbe_status_t fbe_raid_iots_mark_quiesced(fbe_raid_iots_t *iots_p);
void fbe_raid_iots_mark_complete(fbe_raid_iots_t *iots_p);
fbe_status_t fbe_raid_iots_abort(fbe_raid_iots_t *const iots_p);
fbe_status_t fbe_raid_iots_get_failed_io_pos_bitmap(fbe_raid_iots_t    *const in_iots_p,
                                                    fbe_u32_t          *out_failed_io_position_bitmap_p);
void fbe_raid_iots_set_unexpected_error(fbe_raid_iots_t *const iots_p,
                                        fbe_u32_t line,
                                        const char *function_p,
                                        fbe_char_t * fail_string_p, ...)
                                        __attribute__((format(__printf_func__,4,5)));
fbe_raid_state_status_t fbe_raid_iots_unexpected_error(fbe_raid_iots_t *iots_p);

static __forceinline void fbe_raid_iots_set_journal_slot_id(fbe_raid_iots_t *iots_p, fbe_u32_t slot_id)
{
	iots_p->journal_slot_id = slot_id;
	return;
}
static __forceinline void fbe_raid_iots_get_journal_slot_id(fbe_raid_iots_t *iots_p, fbe_u32_t *slot_id_p)
{
	*slot_id_p = iots_p->journal_slot_id;
}

void fbe_raid_iots_transition_quiesced(fbe_raid_iots_t *iots_p, 
                                       fbe_raid_iots_state_t state, 
                                       fbe_raid_iots_completion_context_t context);

fbe_bool_t fbe_raid_iots_is_expired(fbe_raid_iots_t *const iots_p);
fbe_bool_t fbe_raid_iots_is_marked_aborted(fbe_raid_iots_t *iots_p);
fbe_bool_t fbe_raid_iots_is_background_request(fbe_raid_iots_t *iots_p);
fbe_bool_t fbe_raid_iots_is_metadata_request(fbe_raid_iots_t *iots_p);
void fbe_raid_iots_reset_generate_state(fbe_raid_iots_t *iots_p);
fbe_status_t fbe_raid_iots_set_block_operation_status(fbe_raid_iots_t * iots_p);

/***************************************
 * fbe_raid_state.c
 ***************************************/
fbe_status_t fbe_raid_common_state(fbe_raid_common_t * common_request_p);


/***************************************
 * fbe_raid_geometry.c                 *
 ***************************************/
void fbe_raid_geometry_get_default_raid_library_debug_flags(fbe_raid_library_debug_flags_t *debug_flags_p);
fbe_status_t fbe_raid_geometry_set_default_raid_library_debug_flags(fbe_raid_library_debug_flags_t new_default_raid_library_debug_flags);
void fbe_raid_geometry_get_default_object_library_flags(fbe_object_id_t object_id,
                                                        fbe_raid_library_debug_flags_t *debug_flags_p);
void fbe_raid_geometry_persist_default_system_library_flags(fbe_raid_library_debug_flags_t debug_flags);
void fbe_raid_geometry_persist_default_user_library_flags(fbe_raid_library_debug_flags_t debug_flags);
void fbe_raid_geometry_set_default_system_library_flags(fbe_raid_library_debug_flags_t debug_flags);
void fbe_raid_geometry_set_default_user_library_flags(fbe_raid_library_debug_flags_t debug_flags);
void fbe_raid_geometry_get_default_user_library_flags(fbe_raid_library_debug_flags_t *debug_flags_p);
void fbe_raid_geometry_get_default_system_library_flags(fbe_raid_library_debug_flags_t *debug_flags_p);
fbe_status_t fbe_raid_geometry_get_data_disks(fbe_raid_geometry_t *raid_geometry_p,
                                              fbe_u16_t *data_disks_p);
fbe_status_t fbe_raid_geometry_get_parity_disks(fbe_raid_geometry_t *raid_geometry_p,
                                                fbe_u16_t *parity_disks_p);
fbe_status_t fbe_raid_geometry_init(fbe_raid_geometry_t *raid_geometry_p,
                                    fbe_object_id_t object_id,
                                    fbe_class_id_t class_id,
                                    fbe_metadata_element_t *metadata_element_p,
                                    fbe_block_edge_t *block_edge_p,
                                    fbe_raid_common_state_t generate_state);
fbe_status_t fbe_raid_type_get_data_disks(fbe_raid_group_type_t raid_type,
                                          fbe_u32_t width,
                                          fbe_u16_t *data_disks_p);
fbe_status_t fbe_raid_geometry_set_configuration(fbe_raid_geometry_t *raid_geometry_p,
                                                 fbe_u32_t width,
                                                 fbe_raid_group_type_t raid_type,
                                                 fbe_element_size_t element_size,
                                                 fbe_elements_per_parity_t elements_per_parity_stripe,
                                                 fbe_lba_t configured_capacity,
                                                 fbe_block_count_t max_blocks_per_drive_request);
fbe_status_t fbe_raid_geometry_init_journal_write_log(fbe_raid_geometry_t *raid_geometry_p);
fbe_status_t fbe_raid_geometry_set_metadata_configuration(fbe_raid_geometry_t *raid_geometry_p,
                                                          fbe_lba_t metadata_start_lba,
                                                          fbe_lba_t metadata_capacity,
                                                          fbe_lba_t metadata_copy_offset,
                                                          fbe_lba_t journal_write_log_start_lba);
fbe_status_t fbe_raid_geometry_set_block_sizes(fbe_raid_geometry_t *raid_geometry_p,
                                               fbe_block_size_t exported_block_size,
                                               fbe_block_size_t imported_block_size,
                                               fbe_block_size_t optimal_block_size);
fbe_bool_t fbe_raid_geometry_is_ready_for_io(fbe_raid_geometry_t *raid_geometry_p);
fbe_status_t fbe_raid_geometry_not_ready_for_io(fbe_raid_geometry_t *raid_geometry_p, 
                                                fbe_packet_t * const packet_p);
fbe_status_t fbe_raid_geometry_validate_width(fbe_raid_group_type_t raid_type, fbe_u32_t width);
fbe_status_t fbe_raid_geometry_determine_element_size(fbe_raid_group_type_t raid_type, 
                                                      fbe_bool_t b_bandwidth,
                                                      fbe_u32_t *element_size_p,
                                                      fbe_u32_t *elements_per_parity_p);
fbe_status_t fbe_raid_geometry_calculate_lock_range(fbe_raid_geometry_t *raid_geometry_p,
                                                    fbe_lba_t lba,
                                                    fbe_block_count_t blocks,
                                                    fbe_u64_t *stripe_number_p,
                                                    fbe_u64_t *stripe_count_p);
fbe_status_t fbe_raid_geometry_calculate_lock_range_physical(fbe_raid_geometry_t *raid_geometry_p,
                                                             fbe_lba_t lba,
                                                             fbe_block_count_t blocks,
                                                             fbe_u64_t *stripe_number_p,
                                                             fbe_u64_t *stripe_count_p);
fbe_status_t fbe_raid_geometry_calculate_zero_lock_range(fbe_raid_geometry_t *raid_geometry_p,
                                                         fbe_lba_t lba,
                                                         fbe_block_count_t blocks,
                                                         fbe_chunk_size_t chunk_size,
                                                         fbe_u64_t *stripe_number_p,
                                                         fbe_u64_t *stripe_count_p);
fbe_status_t fbe_raid_geometry_calculate_chunk_range(fbe_raid_geometry_t *raid_geometry_p,
                                                     fbe_lba_t lba,
                                                     fbe_block_count_t blocks,
                                                     fbe_chunk_size_t chunk_size,
                                                     fbe_lba_t *chunk_index_p,
                                                     fbe_chunk_count_t *chunk_count_p);
fbe_bool_t fbe_raid_geometry_does_request_exceed_extent(fbe_raid_geometry_t *raid_geometry_p,
                                                        fbe_lba_t start_lba,
                                                        fbe_block_count_t blocks,
                                                        fbe_bool_t b_allow_full_journal_access);
fbe_bool_t fbe_raid_geometry_is_journal_io(fbe_raid_geometry_t *raid_geometry_p,
                                           fbe_lba_t io_start_lba);
fbe_status_t fbe_raid_geometry_get_raid_group_offset(fbe_raid_geometry_t *raid_geometry_p,
                                                     fbe_lba_t *rg_offset_p);
fbe_bool_t fbe_raid_geometry_is_stripe_aligned(fbe_raid_geometry_t *raid_geometry_p,
                                               fbe_lba_t lba, fbe_block_count_t blocks);
fbe_bool_t fbe_raid_geometry_is_single_position(fbe_raid_geometry_t *raid_geometry_p,
                                                fbe_lba_t lba, fbe_block_count_t blocks);

fbe_u32_t fbe_raid_geometry_calc_striper_disk_ios(fbe_raid_geometry_t *raid_geometry_p,
                                                  fbe_payload_block_operation_t *block_op_p,
                                                  fbe_bool_t b_is_zeroing);

fbe_u32_t fbe_raid_geometry_calc_parity_disk_ios(fbe_raid_geometry_t *raid_geometry_p,
                                                 fbe_payload_block_operation_t *block_op_p,
                                                 fbe_bool_t b_is_zeroing,
                                                 fbe_bool_t b_is_degraded,
                                                 fbe_packet_priority_t priority,
                                                 fbe_u32_t max_credits);

fbe_u32_t fbe_raid_geometry_calc_mirror_disk_ios(fbe_raid_geometry_t *raid_geometry_p,
                                                 fbe_payload_block_operation_t *block_op_p,
                                                 fbe_bool_t b_is_zeroing,
                                                 fbe_bool_t b_is_degraded,
                                                 fbe_packet_priority_t priority,
                                                 fbe_u32_t max_credits);

void fbe_raid_geometry_align_lock_request(fbe_raid_geometry_t *raid_geometry_p,
                                          fbe_lba_t * const aligned_start_lba_p,
                                          fbe_lba_t * const aligned_last_lba_p);
void fbe_raid_geometry_align_io(fbe_raid_geometry_t *raid_geometry_p,
                                fbe_lba_t * const lba_p,
                                fbe_block_count_t * const blocks_p);

fbe_bool_t fbe_raid_geometry_io_needs_alignment(fbe_raid_geometry_t *raid_geometry_p,
                                                fbe_lba_t lba,
                                                fbe_block_count_t blocks);
 
void fbe_raid_geometry_set_parity_degraded_read_multiplier(fbe_u32_t multiplier);
void fbe_raid_geometry_set_parity_degraded_write_multiplier(fbe_u32_t multiplier);
void fbe_raid_geometry_set_mirror_degraded_read_multiplier(fbe_u32_t multiplier);
void fbe_raid_geometry_set_mirror_degraded_write_multiplier(fbe_u32_t multiplier);
void fbe_raid_geometry_set_credits_ceiling_divisor(fbe_u32_t divisor);
/********************************* 
 * fbe_parity_geometry.c
 *********************************/
fbe_status_t fbe_parity_get_lun_geometry(fbe_raid_geometry_t *raid_geometry_p,
                                         fbe_lba_t lba,
                                         fbe_raid_siots_geometry_t * geo);

fbe_status_t fbe_parity_get_physical_geometry(fbe_raid_geometry_t *raid_geometry_p,
                                              fbe_lba_t lba,
                                              fbe_raid_siots_geometry_t * geo);

/**************************************** 
 *   fbe_mirror_geometry.c
 ****************************************/
fbe_bool_t fbe_mirror_get_geometry(fbe_raid_geometry_t *raid_geometry_p,
                                   fbe_lba_t lba,
                                   fbe_block_count_t blocks,
                                   fbe_raid_siots_geometry_t *geo);

/**************************************** 
 *   fbe_striper_geometry.c
 ****************************************/
fbe_status_t fbe_striper_get_geometry(fbe_raid_geometry_t *raid_geometry_p,
                                      fbe_lba_t lba,
                                      fbe_raid_siots_geometry_t *geo);

fbe_status_t fbe_striper_get_physical_geometry(fbe_raid_geometry_t *raid_geometry_p,
                                               fbe_lba_t pba,
                                               fbe_raid_siots_geometry_t *geo,
                                               fbe_u16_t *const unit_width);

/********************************* 
 * fbe_raid_memory_api.c 
 *********************************/
fbe_status_t fbe_raid_memory_api_get_raid_memory_statistics(fbe_raid_memory_stats_t *memory_stats_p);
fbe_status_t fbe_raid_memory_api_reset_raid_memory_statistics(void);

/**************************************** 
    fbe_mirror_generate.c
 ****************************************/
fbe_raid_state_status_t fbe_mirror_generate_start(fbe_raid_siots_t * siots_p);

/**************************************** 
 *   fbe_parity_generate.c
 ****************************************/
fbe_raid_state_status_t fbe_parity_generate_start(fbe_raid_siots_t * siots_p);



/**************************************** 
 *   fbe_raid_cond_trace.c
 ****************************************/
fbe_status_t fbe_raid_cond_set_library_error_testing(void);
fbe_status_t fbe_raid_cond_set_library_random_errors(fbe_u32_t percentage, fbe_bool_t b_inject_on_all_rgs);
fbe_status_t fbe_raid_cond_get_library_error_testing_stats(fbe_raid_group_raid_lib_error_testing_stats_t * error_testing_stats_p);
fbe_status_t fbe_raid_cond_reset_library_error_testing_stats(void);
void fbe_raid_cond_init_log(void);
void fbe_raid_cond_destroy_log(void);
fbe_status_t fbe_raid_cond_set_inject_on_user_raid_only(void);

/**************************************** 
 *   fbe_striper_generate.c
 ****************************************/
fbe_raid_state_status_t fbe_striper_generate_start(fbe_raid_siots_t * siots_p);

/**************************************** 
 *   fbe_raid_common.c
 ****************************************/
fbe_status_t fbe_raid_common_set_resource_priority(fbe_packet_t * packet, fbe_raid_geometry_t * geo);

/**************************************** 
 *   fbe_raid_raw_3way_mirror_main.c
 ****************************************/
#define FBE_RAW_MIRROR_WIDTH 3

/*!*******************************************************************
 * @enum fbe_raw_mirror_flags_t
 *********************************************************************
 * @brief Flags we use for controlling the raw mirror interface.
 *
 *********************************************************************/
typedef enum fbe_raw_mirror_flags_e
{
    FBE_RAW_MIRROR_FLAG_NONE     = 0x0000,
    FBE_RAW_MIRROR_FLAG_QUIESCE  = 0x0001,
    FBE_RAW_MIRROR_FLAG_DISABLED = 0x0002,
    FBE_RAW_MIRROR_FLAG_LAST     = 0x0004,
}
fbe_raw_mirror_flags_t;

/*!*************************************************
 * @typedef fbe_raw_mirror_t
 ***************************************************
 * @brief
 *  This is the interface we need in order for
 *  the service to call raid.
 *
 *
 ***************************************************/
typedef struct fbe_raw_mirror_s
{
    fbe_block_edge_t *  edge[FBE_RAW_MIRROR_WIDTH];
    fbe_raid_geometry_t geo;
    fbe_bool_t          b_initialized;

    fbe_block_transport_control_get_edge_info_t edge_info;
	/*the bitmap to indicate which disk is unaccessible
           An IO op may occur during the system pvd destroy and creation.
           At this time raw mirror may record the wrong edge info 
           due to memory( which is released by destroying a PVD ) is overwritten by others.
           We use this to record which edge is invalid 
           and check it when send IO to raid.
           The down_disk_bitmask is used internally in the raw_mirror send io packet interface, 
	*/
	fbe_u16_t           down_disk_bitmask;
	/*the flag indicated whether we should quiesce the IO or not
	    if we want to destroy the PVD or edge and 
	    don't want outstanding raw-mirror IO to refrence the wrong edge, 
	    quiesce IO first*/
	fbe_bool_t          quiesce_flag;
	/*the semaphore is used to sync the IO with IO quiesce caller*/
	fbe_semaphore_t     quiesce_io_semaphore;
	/*the count for account the outstanding io*/
	fbe_u32_t           outstanding_io_count;
    fbe_raw_mirror_flags_t flags;
	/*the spinlock to protect all the fields above*/
	fbe_spinlock_t      raw_mirror_lock;
	
}fbe_raw_mirror_t;


/* Accessors for flags.
 */
static __forceinline fbe_bool_t fbe_raw_mirror_is_flag_set(fbe_raw_mirror_t *raid_p,
                                                           fbe_raw_mirror_flags_t flags)
{
    return ((raid_p->flags & flags) == flags);
}
static __forceinline void fbe_raw_mirror_init_flags(fbe_raw_mirror_t *raid_p)
{
    raid_p->flags = 0;
    return;
}
static __forceinline void fbe_raw_mirror_set_flag(fbe_raw_mirror_t *raid_p,
                                                  fbe_raw_mirror_flags_t flags)
{
    raid_p->flags |= flags;
}

static __forceinline void fbe_raw_mirror_clear_flag(fbe_raw_mirror_t *raid_p,
                                                    fbe_raw_mirror_flags_t flags)
{
    raid_p->flags &= ~flags;
}

fbe_status_t fbe_raw_mirror_init(fbe_raw_mirror_t *raw_mirror_p, 
                                 fbe_lba_t offset, 
                                 fbe_lba_t rg_offset, 
                                 fbe_block_count_t capacity);

fbe_status_t fbe_raw_mirror_init_edges(fbe_raw_mirror_t *raw_mirror_p);


fbe_status_t fbe_raw_mirror_send_io_packet_to_raid_library(fbe_raw_mirror_t *raw_mirror_p, fbe_packet_t *packet_p);

fbe_status_t fbe_raw_mirror_get_degraded_bitmask(fbe_raw_mirror_t *raw_mirror_p,fbe_raid_position_bitmask_t *bitmask_p);
fbe_status_t fbe_raw_mirror_ex_send_io_packet_to_raid_library(fbe_raw_mirror_t *raw_mirror_p, fbe_packet_t *packet_p, fbe_u16_t in_disk_bitmap);
fbe_status_t fbe_raw_mirror_mask_down_disk(fbe_raw_mirror_t *raw_mirror_p, fbe_u32_t disk_index);
fbe_status_t fbe_raw_mirror_unmask_down_disk(fbe_raw_mirror_t *raw_mirror_p, fbe_u32_t disk_index);
fbe_status_t fbe_raw_mirror_get_down_disk_bitmask(fbe_raw_mirror_t *raw_mirror_p, fbe_u16_t* down_disk_bitmap);
fbe_status_t fbe_raw_mirror_unset_initialized(fbe_raw_mirror_t *raw_mirror);
fbe_status_t fbe_raw_mirror_mask_down_disk_in_all(fbe_u32_t disk_index);
fbe_status_t fbe_raw_mirror_unmask_down_disk_in_all(fbe_u32_t disk_index);
fbe_status_t fbe_raw_mirror_unset_initialized_in_all(void);
fbe_status_t fbe_raw_mirror_set_quiesce_flags_in_all(void);
fbe_status_t fbe_raw_mirror_unset_quiesce_flags_in_all(void);
fbe_status_t fbe_raw_mirror_wait_all_io_quiesced(void);
fbe_status_t fbe_raw_mirror_set_quiesce_flag(fbe_raw_mirror_t *raw_mirror_p);
fbe_status_t fbe_raw_mirror_unset_quiesce_flag(fbe_raw_mirror_t *raw_mirror_p);
fbe_status_t fbe_raw_mirror_init_edges_in_all(void);
fbe_status_t fbe_raw_mirror_send_io_complete_function(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);
fbe_status_t fbe_raw_mirror_destroy(fbe_raw_mirror_t *raw_mirror_p);
fbe_status_t fbe_raw_mirror_disable(fbe_raw_mirror_t *raw_mirror_p);
fbe_status_t fbe_raw_mirror_is_enabled(fbe_raw_mirror_t *raw_mirror_p);
fbe_status_t fbe_raw_mirror_wait_quiesced(fbe_raw_mirror_t *raw_mirror_p);

/* fbe_parity_write_log_util.c
 */
void fbe_parity_write_log_quiesce(fbe_parity_write_log_info_t * write_log_info_p);
void fbe_parity_write_log_unquiesce(fbe_parity_write_log_info_t * write_log_info_p);
void fbe_parity_write_log_abort(fbe_parity_write_log_info_t * write_log_info_p);

fbe_status_t fbe_parity_write_log_init(fbe_parity_write_log_info_t * write_log_info_p);
fbe_status_t fbe_parity_write_log_destroy(fbe_parity_write_log_info_t * write_log_info_p);
fbe_u32_t fbe_parity_write_log_get_allocated_slot(fbe_parity_write_log_info_t * write_log_info_p);
void fbe_parity_write_log_set_header_invalid(fbe_parity_write_log_header_t * header_p);
fbe_bool_t fbe_parity_write_log_is_header_valid(fbe_parity_write_log_header_t * header_p);
void fbe_parity_write_log_set_all_slots_valid(fbe_parity_write_log_info_t * write_log_info_p, fbe_bool_t both_sp_slots);
void fbe_parity_write_log_set_all_slots_free(fbe_parity_write_log_info_t * write_log_info_p, fbe_bool_t both_sp_slots);
void fbe_parity_write_log_set_slot_allocated(fbe_parity_write_log_info_t * write_log_info_p, fbe_u32_t slot_idx);
void fbe_parity_write_log_set_slot_needs_remap(fbe_parity_write_log_info_t * write_log_info_p, fbe_u32_t slot_idx);
void fbe_parity_write_log_set_slot_free(fbe_parity_write_log_info_t * write_log_info_p, fbe_u32_t slot_idx);
fbe_s32_t fbe_parity_write_log_compare_timestamp(fbe_parity_write_log_header_timestamp_t *timestamp_a,
                                                 fbe_parity_write_log_header_timestamp_t *timestamp_b);
fbe_u32_t fbe_parity_write_log_get_remap_slot(fbe_parity_write_log_info_t * write_log_info_p, fbe_bool_t both_sp_slots);
void fbe_parity_write_log_release_remap_slot(fbe_parity_write_log_info_t * write_log_info_p, fbe_u32_t slot_idx);

fbe_status_t fbe_raid_update_master_block_status(fbe_payload_block_operation_t * mbo, fbe_payload_block_operation_t	* bo);

/*****************************************
 * end of fbe_raid_library_api.h
 *****************************************/

#endif // _FBE_RAID_LIBRARY_API_H_

