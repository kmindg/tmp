#ifndef FBE_RAID_LIBRARY_PROTO_H
#define FBE_RAID_LIBRARY_PROTO_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_raid_library_proto.h
 ***************************************************************************
 *
 * @brief
 *  This file contains prototypes shared across the raid group driver.
 *
 * @version
 *   5/14/2009:  Created. rfoley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "csx_ext.h"
#include "fbe/fbe_transport.h"
#include "fbe_raid_fruts.h"
#include "fbe_raid_algorithm.h"
#include "fbe_raid_library_private.h"
#include "fbe/fbe_payload_block_operation.h"
#include "fbe/fbe_perfstats_interface.h"
#include "fbe_raid_geometry.h"
#include "fbe_raid_memory_api.h"
#include "fbe_raid_iots_accessor.h"
#include "fbe_raid_siots_accessor.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/**************************************** 
 * fbe_raid_common.c 
 ****************************************/
void fbe_raid_common_set_memory_allocation_error(fbe_raid_common_t *common_p);
void fbe_raid_common_set_memory_free_error(fbe_raid_common_t *common_p);
fbe_status_t fbe_raid_common_set_memory_request_fields_from_parent(fbe_raid_common_t *common_p);

/**************************************** 
 * fbe_raid_iots.c 
 ****************************************/
fbe_status_t fbe_raid_iots_init_common(fbe_raid_iots_t* iots_p,
                                       fbe_lba_t lba,
                                       fbe_block_count_t blocks);
fbe_status_t fbe_raid_iots_determine_host_buffer_size(fbe_raid_iots_t *const iots_p,
                                                      fbe_u32_t *host_buffer_size_p);
fbe_raid_state_status_t fbe_raid_iots_memory_allocation_error(fbe_raid_iots_t *iots_p);
fbe_bool_t fbe_raid_iots_is_expired(fbe_raid_iots_t *const iots_p);
fbe_status_t fbe_raid_iots_get_expiration_time(fbe_raid_iots_t *const iots_p,
                                               fbe_time_t *expiration_time_p);
fbe_status_t fbe_raid_iots_inc_aborted_shutdown(fbe_raid_iots_t *const iots_p);

fbe_status_t fbe_raid_iots_request_siots_allocation(fbe_raid_iots_t *iots_p);
fbe_status_t fbe_raid_iots_process_allocated_siots(fbe_raid_iots_t *iots_p,
                                                   fbe_raid_siots_t **siots_pp);
fbe_bool_t fbe_raid_iots_is_buffered_op(fbe_raid_iots_t *iots_p);

/**************************************** 
 * fbe_raid_iots_states.c 
 ****************************************/
fbe_raid_state_status_t fbe_raid_iots_memory_allocation_error(fbe_raid_iots_t *iots_p);
fbe_raid_state_status_t fbe_raid_iots_generate_siots(fbe_raid_iots_t * iots_p);
fbe_raid_state_status_t fbe_raid_iots_complete_iots(fbe_raid_iots_t * iots_p);
fbe_raid_state_status_t fbe_raid_iots_freed(fbe_raid_iots_t *iots_p);
fbe_raid_state_status_t fbe_raid_iots_callback(fbe_raid_iots_t * iots_p);

/**************************************** 
 * fbe_raid_state.c 
 ****************************************/
fbe_status_t fbe_raid_siots_start_nested_siots(fbe_raid_siots_t * siots_p,
                                               void *(first_state));
void fbe_raid_get_stripe_range(fbe_lba_t lba,
                               fbe_block_count_t blks_accessed,
                               fbe_element_size_t blks_per_element,
                               fbe_u16_t data_disks,
                               fbe_u16_t max_extents,
                               fbe_raid_extent_t range[]);

void fbe_raid_geometry_load_default_registry_values(void);
/**************************************** 
 * fbe_raid_siots.c 
 ****************************************/
fbe_raid_siots_t * fbe_raid_iots_get_embedded_siots(fbe_raid_iots_t *iots_p);
fbe_status_t fbe_raid_siots_embedded_init(fbe_raid_siots_t *siots_p,
                                  fbe_raid_iots_t *iots_p);
fbe_status_t fbe_raid_siots_allocated_consume_init(fbe_raid_siots_t *siots_p,
                                           fbe_raid_iots_t *iots_p);
fbe_status_t fbe_raid_siots_allocated_activate_init(fbe_raid_siots_t *siots_p,
                                                    fbe_raid_iots_t *iots_p);
fbe_status_t fbe_raid_nest_siots_init(fbe_raid_siots_t *siots_p,
                                      fbe_raid_siots_t *parent_siots_p);
void fbe_raid_siots_init_memory_fields(fbe_raid_siots_t *siots_p);
fbe_status_t fbe_raid_siots_set_optimal_page_size(fbe_raid_siots_t *siots_p,
                                                  fbe_u16_t num_fruts_to_allocate,
                                                  fbe_block_count_t num_of_blocks_to_allocate);
fbe_status_t fbe_raid_siots_destroy(fbe_raid_siots_t *siots_p);
fbe_status_t fbe_raid_siots_reuse(fbe_raid_siots_t *siots_p);
fbe_status_t fbe_raid_siots_get_raid_group_offset(fbe_raid_siots_t *siots_p,
                                                  fbe_lba_t *raid_group_offset_p);
fbe_status_t fbe_raid_siots_common_init(fbe_raid_siots_t *siots_p,
                                fbe_raid_common_t* common_p);
fbe_status_t fbe_raid_siots_parity_rebuild_pos_get(fbe_raid_siots_t *siots_p,
                                           fbe_u16_t *parity_pos_p,
                                           fbe_u16_t *rebuild_pos_p);
fbe_status_t fbe_raid_siots_add_degraded_pos(fbe_raid_siots_t *const siots_p,
                                             fbe_u32_t new_deg_pos );
fbe_status_t fbe_raid_siots_remove_degraded_pos(fbe_raid_siots_t *siots_p,
                                                fbe_u16_t position);
fbe_raid_position_t fbe_raid_siots_dead_pos_get( fbe_raid_siots_t *const siots_p,
                                                 fbe_raid_degraded_position_t position );
fbe_status_t fbe_raid_siots_dead_pos_set( fbe_raid_siots_t *const siots_p,
                                  fbe_raid_degraded_position_t position,
                                  fbe_raid_position_t new_value );
fbe_status_t fbe_raid_siots_update_degraded(fbe_raid_siots_t *siots_p,
                                            fbe_raid_position_bitmask_t returning_bitmask);
fbe_status_t fbe_raid_siots_get_full_access_bitmask(fbe_raid_siots_t *siots_p,
                                            fbe_raid_position_bitmask_t *full_access_bitmask_p);
fbe_u32_t fbe_raid_siots_get_full_access_count(fbe_raid_siots_t *siots_p);
fbe_u32_t fbe_raid_siots_get_read_fruts_count(fbe_raid_siots_t *siots_p);
fbe_u32_t fbe_raid_siots_get_read2_fruts_count(fbe_raid_siots_t *siots_p);
fbe_u32_t fbe_raid_siots_get_write_fruts_count(fbe_raid_siots_t *siots_p);
fbe_status_t fbe_raid_siots_move_fruts(fbe_raid_siots_t *siots_p,
                                       fbe_raid_fruts_t *fruts_to_move_p,
                                       fbe_queue_head_t *destination_chain_p);
void fbe_raid_siots_get_read_fruts_bitmask(fbe_raid_siots_t *siots_p,
                                           fbe_raid_position_bitmask_t *read_fruts_bitmask_p);
void fbe_raid_siots_get_read2_fruts_bitmask(fbe_raid_siots_t *siots_p,
                                           fbe_raid_position_bitmask_t *read2_fruts_bitmask_p);
void fbe_raid_siots_get_write_fruts_bitmask(fbe_raid_siots_t *siots_p,
                                           fbe_raid_position_bitmask_t *write_fruts_bitmask_p);
fbe_status_t fbe_raid_siots_consume_nested_siots(fbe_raid_siots_t *siots_p,
                                                 fbe_raid_memory_info_t *memory_info_p);
fbe_status_t fbe_raid_siots_init_vrts(fbe_raid_siots_t *siots_p,
                                      fbe_raid_memory_info_t *memory_info_p);
fbe_status_t fbe_raid_siots_init_vcts(fbe_raid_siots_t *siots_p,
                                      fbe_raid_memory_info_t *memory_info_p);
void fbe_raid_siots_init_vcts_curr_pass(fbe_raid_siots_t * const siots_p);
void fbe_raid_siots_enqueue_freed_list(fbe_raid_siots_t *siots_p, 
                                       fbe_raid_memory_header_t *memory_p);
fbe_status_t fbe_raid_siots_destroy_resources(fbe_raid_siots_t *siots_p);
fbe_status_t fbe_raid_siots_setup_cached_sgd(fbe_raid_siots_t *siots_p, fbe_sg_element_with_offset_t *sgd_p);
fbe_status_t fbe_raid_siots_setup_sgd(fbe_raid_siots_t *siots_p, 
                              fbe_sg_element_with_offset_t *sgd_p);

fbe_status_t fbe_raid_siots_should_generate_next_siots(fbe_raid_siots_t *siots_p,
                                                       fbe_bool_t *b_generate_p);
fbe_status_t fbe_raid_iots_generate_next_siots_immediate(fbe_raid_iots_t *iots_p);
fbe_status_t fbe_raid_iots_cancel_generate_next_siots(fbe_raid_iots_t *iots_p);

fbe_bool_t  fbe_raid_siots_is_position_degraded(fbe_raid_siots_t *const siots_p,
                                                const fbe_u32_t position);
fbe_u32_t fbe_raid_siots_get_degraded_count(fbe_raid_siots_t *siots_p);
fbe_u32_t fbe_raid_siots_get_disabled_count(fbe_raid_siots_t *siots_p);
void fbe_raid_siots_get_accessible_bitmask(fbe_raid_siots_t *siots_p,
                                           fbe_raid_position_bitmask_t  *accessible_bitmask_p);
fbe_u32_t  fbe_raid_siots_get_parity_count_for_region_mining(fbe_raid_siots_t *const siots_p);
fbe_bool_t fbe_raid_siots_restart_upgrade_waiter(fbe_raid_siots_t *const siots_p,
                                                 fbe_raid_siots_t **restart_siots_p);
fbe_status_t fbe_raid_siots_get_capacity(fbe_raid_siots_t * const siots_p,
                                         fbe_lba_t *capacity_p);
fbe_status_t fbe_raid_siots_convert_hard_media_errors(fbe_raid_siots_t *siots_p,
                                               fbe_raid_position_t read_position);
void fbe_raid_siots_clear_converted_hard_media_errors(fbe_raid_siots_t *siots_p,
                                                      fbe_raid_position_bitmask_t hard_media_err_bitmask);
fbe_bool_t fbe_raid_siots_is_upgradable(fbe_raid_siots_t *const in_siots_p);
fbe_status_t fbe_raid_siots_check_clear_upgrade_waiters(fbe_raid_siots_t *const siots_p);
/* Not in Use 
fbe_raid_state_status_t fbe_raid_siots_upgrade_lock(fbe_raid_siots_t * const siots_p,
                                                    fbe_u32_t aligned_block_size);
*/
fbe_bool_t fbe_raid_siots_is_expired(fbe_raid_siots_t * const siots_p);
fbe_bool_t fbe_raid_siots_is_validation_enabled(fbe_raid_siots_t *siots_p);

fbe_status_t fbe_raid_siots_get_expiration_time(fbe_raid_siots_t *const iots_p,
                                                fbe_time_t *expiration_time_p);
fbe_status_t fbe_raid_siots_abort(fbe_raid_siots_t *const siots_p);
fbe_status_t fbe_raid_siots_reinit_for_next_region(fbe_raid_siots_t *const siots_p);
fbe_bool_t fbe_raid_siots_is_metadata_operation(fbe_raid_siots_t *siots_p);
fbe_bool_t fbe_raid_siots_is_small_request(fbe_raid_siots_t *siots_p);
fbe_status_t fbe_raid_siots_check_fru_request_limit(fbe_raid_siots_t *siots_p,
                                                    fbe_raid_fru_info_t *read_info_p,
                                                    fbe_raid_fru_info_t *write_info_p,
                                                    fbe_raid_fru_info_t *read2_info_p,
                                                    fbe_status_t in_status);
fbe_bool_t fbe_raid_siots_is_monitor_initiated_op(fbe_raid_siots_t *siots_p);
fbe_status_t fbe_raid_siots_get_failed_io_pos_bitmap(fbe_raid_siots_t   *const in_siots_p,
                                                     fbe_u32_t          *out_failed_io_position_bitmap_p);
fbe_raid_fruts_t * fbe_raid_siots_get_fruts_by_position(fbe_raid_fruts_t *read_fruts_p,
                                                        fbe_u16_t position,
                                                        fbe_u32_t read_count);
void fbe_raid_siots_log_traffic(fbe_raid_siots_t *const siots_p, char *const string_p);
void fbe_raid_siots_log_error(fbe_raid_siots_t *const siots_p, char *const string_p);
fbe_raid_state_status_t fbe_raid_siots_wait_previous(fbe_raid_siots_t *const siots_p);
void fbe_raid_siots_get_statistics(fbe_raid_siots_t *siots_p,
                                   fbe_raid_library_statistics_t *stats_p);
void fbe_raid_siots_get_no_data_bitmap(fbe_raid_siots_t *siots_p,
                                       fbe_raid_position_bitmask_t *bitmap_p);
fbe_bool_t fbe_raid_siots_is_able_to_quiesce(fbe_raid_siots_t *siots_p);

/**************************************** 
 * fbe_raid_siots_states.c 
 ****************************************/

fbe_raid_state_status_t fbe_raid_siots_success(fbe_raid_siots_t *siots_p);
fbe_raid_state_status_t fbe_raid_siots_shutdown_error(fbe_raid_siots_t *siots_p);
fbe_raid_state_status_t fbe_raid_siots_aborted(fbe_raid_siots_t *siots_p);
fbe_raid_state_status_t fbe_raid_siots_expired(fbe_raid_siots_t *siots_p);
fbe_raid_state_status_t fbe_raid_siots_write_expired(fbe_raid_siots_t *siots_p);
fbe_raid_state_status_t fbe_raid_siots_unexpected_error(fbe_raid_siots_t *siots_p);
fbe_raid_state_status_t fbe_raid_siots_media_error(fbe_raid_siots_t *siots_p);
fbe_raid_state_status_t fbe_raid_siots_dropped_error(fbe_raid_siots_t *siots_p);
fbe_raid_state_status_t fbe_raid_siots_freed(fbe_raid_siots_t *siots_p);
fbe_raid_state_status_t fbe_raid_siots_dead_error(fbe_raid_siots_t *siots_p);
fbe_raid_state_status_t fbe_raid_siots_invalid_opcode(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_raid_siots_invalid_parameter(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_raid_siots_write_crc_error(fbe_raid_siots_t *siots_p);
fbe_raid_state_status_t fbe_raid_siots_not_preferred_error(fbe_raid_siots_t *siots_p);
fbe_raid_state_status_t fbe_raid_siots_reduce_qdepth_soft(fbe_raid_siots_t *siots_p);
fbe_raid_state_status_t fbe_raid_siots_reduce_qdepth_hard(fbe_raid_siots_t *siots_p);
fbe_raid_state_status_t fbe_raid_siots_zeroed(fbe_raid_siots_t *siots_p);
fbe_raid_state_status_t fbe_raid_siots_write_log_aborted(fbe_raid_siots_t *siots_p);
fbe_raid_state_status_t fbe_raid_siots_memory_allocation_error(fbe_raid_siots_t *siots_p);
fbe_raid_state_status_t fbe_raid_siots_degraded(fbe_raid_siots_t *siots_p);

fbe_raid_state_status_t fbe_raid_siots_transition_finished( fbe_raid_siots_t *siots_p );
void fbe_raid_siots_set_unexpected_error(fbe_raid_siots_t *const siots_p,
                                         fbe_u32_t line,
                                         const char * file_p,
                                         fbe_char_t * fmt,
					 ...) __attribute__((format(__printf_func__,4,5)));
fbe_raid_state_status_t fbe_raid_siots_condition_failure(fbe_raid_siots_t *siots_p,
                                                         const char *const condition_p,
                                                         fbe_u32_t line,
                                                         const char *const file_p);
/**************************************** 
 * fbe_raid_trace.c 
 ****************************************/
void fbe_raid_library_trace(fbe_trace_level_t trace_level,
                            fbe_u32_t line,
                            const char *const function_p,
                            fbe_char_t * fail_string_p,
			    ...) __attribute__((format(__printf_func__,4,5)));

void fbe_raid_library_trace_basic(fbe_trace_level_t trace_level,
                                  fbe_u32_t line,
                                  const char *const function_p,
                                  fbe_char_t * fail_string_p,
				  ...) __attribute__((format(__printf_func__,4,5)));

void fbe_raid_library_object_trace(fbe_object_id_t object_id,
                                   fbe_trace_level_t trace_level,
                                   fbe_u32_t line,
                                   const char *const function_p,
                                   fbe_char_t * fmt,
				   ...) __attribute__((format(__printf_func__,5,6)));

void fbe_raid_siots_object_trace(fbe_raid_siots_t *siots_p,
                                 fbe_trace_level_t trace_level,
                                 fbe_u32_t line,
                                 const char *const function_p,
                                 fbe_char_t * fmt,
				 ...) __attribute__((format(__printf_func__,5,6)));

void fbe_raid_iots_object_trace(fbe_raid_iots_t *iots_p,
                                fbe_trace_level_t trace_level,
                                fbe_u32_t line,
                                const char *const function_p,
                                fbe_char_t * fmt,
				...) __attribute__((format(__printf_func__,5,6)));

void fbe_raid_trace_error(fbe_u32_t line,
                          const char *const function_p,
                          fbe_char_t * fail_string_p,
			  ...) __attribute__((format(__printf_func__,3,4)));

void fbe_raid_siots_trace_failure(fbe_raid_siots_t *siots_p,
                                  const char *const condition_p,
                                  fbe_u32_t line,
                                  const char *const file_p);

void fbe_raid_siots_trace(fbe_raid_siots_t *const siots_p,
                          fbe_trace_level_t trace_level,
                          fbe_u32_t line,
                          const char *file_p,
                          fbe_char_t * fail_string_p,
			  ...) __attribute__((format(__printf_func__,5,6)));

void fbe_raid_iots_trace(fbe_raid_iots_t *const iots_p,
                         fbe_trace_level_t trace_level,
                         fbe_u32_t line,
                         const char *function_p,
                         fbe_char_t * fail_string_p,
			 ...) __attribute__((format(__printf_func__,5,6)));

void fbe_raid_iots_trace_rebuild_chunk_info(fbe_raid_iots_t *const iots_p,
                                            fbe_chunk_count_t chunk_count);

void fbe_raid_iots_trace_verify_chunk_info(fbe_raid_iots_t *const iots_p,
                                           fbe_chunk_count_t chunk_count);

void fbe_raid_write_journal_trace(fbe_raid_siots_t *const siots_p,
								  fbe_trace_level_t trace_level,
								  fbe_u32_t line,
								  const char *function_p,
								  const fbe_char_t * fmt, ...) __attribute__((format(__printf_func__,5,6)));

void fbe_raid_memory_info_trace(fbe_raid_memory_info_t *const memory_info_p,
                          fbe_trace_level_t trace_level,
                          fbe_u32_t line,
                          const char *function_p,
                          fbe_char_t * fail_string_p,
			  ...) __attribute__((format(__printf_func__,5,6)));
/**************************************** 
 * fbe_raid_geometry.c
 ****************************************/
fbe_status_t fbe_raid_geometry_calc_preread_blocks(fbe_lba_t lda,
                                  fbe_raid_siots_geometry_t *geo,
                                  fbe_block_count_t blks_to_write,
                                  fbe_u16_t blks_per_element,
                                  fbe_u16_t data_disks,
                                  fbe_lba_t parity_start,
                                  fbe_block_count_t parity_count,
                                  fbe_block_count_t read1_blkcnt[],
                                  fbe_block_count_t read2_blkcnt[]);
fbe_status_t fbe_raid_map_pba_to_lba_relative(fbe_lba_t fru_lba,
                                                   fbe_u32_t dpos,
                                                   fbe_u32_t ppos,
                                                   fbe_lba_t pstart_lba,
                                                   fbe_u16_t width,
                                                   fbe_lba_t *logical_parity_start_p,
                                                   fbe_lba_t pstripe_offset,
                                                   fbe_u16_t parities);
fbe_status_t fbe_raid_geometry_get_parity_disks(fbe_raid_geometry_t *raid_geometry_p,
                                                fbe_u16_t *parity_disks_p);

/**************************************** 
 * fbe_raid_generate.c 
 ****************************************/
fbe_bool_t fbe_raid_siots_get_lock(fbe_raid_siots_t * siots_p);
fbe_bool_t fbe_raid_siots_get_first_lock_waiter(fbe_raid_siots_t *const siots_p,
                                               fbe_raid_siots_t ** restart_siots_p);
fbe_bool_t fbe_raid_siots_check_quiesced_and_restart_lock_waiters(fbe_raid_siots_t *const siots_p);
void fbe_raid_group_set_nr_extent_mock(fbe_raid_group_get_nr_extent_fn_t mock);
fbe_status_t fbe_raid_iots_get_nr_extent(fbe_raid_iots_t *raid_iots_p,
                                          fbe_u32_t position,
                                          fbe_lba_t lba, 
                                          fbe_block_count_t blocks,
                                          fbe_raid_nr_extent_t *extent_p,
                                          fbe_u32_t max_extents);
fbe_status_t fbe_raid_extent_is_nr(fbe_lba_t lba, 
                                   fbe_block_count_t blocks,
                                   fbe_raid_nr_extent_t *extent_p,
                                   fbe_u32_t max_extents,
                                   fbe_bool_t *b_is_nr,
                                   fbe_block_count_t *blocks_p);
fbe_status_t fbe_raid_extent_find_next_nr(fbe_lba_t lba, 
                                          fbe_u32_t blocks,
                                          fbe_raid_nr_extent_t *extent_p,
                                          fbe_u32_t max_extents,
                                          fbe_bool_t *b_is_nr);
fbe_status_t fbe_raid_extent_get_first_nr(fbe_lba_t lba, 
                                          fbe_block_count_t blocks,
                                          fbe_raid_nr_extent_t *extent_p,
                                          fbe_u32_t max_extents,
                                          fbe_lba_t *nr_start_lba_p,
                                          fbe_block_count_t *nr_blocks_p,
                                          fbe_bool_t *b_is_nr);
fbe_status_t fbe_raid_generate_align_io_end_reduce(fbe_block_size_t alignment_blocks,
                                                   fbe_lba_t start_lba,
                                                   fbe_block_count_t * const blocks_p);
/**************************************** 
 * fbe_raid_fruts.c 
 ****************************************/
fbe_raid_siots_t* fbe_raid_fruts_get_siots(fbe_raid_fruts_t *fruts_p);
void fbe_raid_fruts_common_init(fbe_raid_common_t *common_p);
fbe_raid_state_status_t fbe_raid_fruts_invalid_state(fbe_raid_common_t *common_p);
extern fbe_status_t fbe_raid_fruts_init_fruts(fbe_raid_fruts_t *const fruts_p,
                                        fbe_payload_block_operation_opcode_t opcode,
                                        fbe_lba_t lba,
                                        fbe_block_count_t blocks,
                                        fbe_u32_t position );

fbe_status_t fbe_raid_setup_fruts_from_fru_info(fbe_raid_siots_t * siots_p, 
                                                fbe_raid_fru_info_t *info_p,
                                                fbe_queue_head_t *queue_head_p,
                                                fbe_u32_t opcode,
                                                fbe_raid_memory_info_t *memory_info_p);
void fbe_raid_fruts_chain_set_prepend_header(fbe_queue_head_t *queue_head_p, fbe_bool_t prepend);
fbe_bool_t fbe_raid_fruts_send(fbe_raid_fruts_t *fruts_p, void (*evaluate_state));
fbe_bool_t fbe_raid_fruts_send_chain(fbe_queue_head_t *head_p, void (*evaluate_state));
fbe_status_t fbe_raid_fruts_send_chain_bitmap(fbe_queue_head_t *head_p, 
                                              void (*evaluate_state),
                                              fbe_raid_position_bitmask_t start_bitmap);
fbe_raid_state_status_t fbe_raid_fruts_evaluate(fbe_raid_fruts_t *fruts_p);
fbe_status_t fbe_raid_fruts_destroy(fbe_raid_fruts_t *fruts_p);
fbe_status_t fbe_raid_fruts_packet_completion(fbe_packet_t * packet_p, 
                                             fbe_packet_completion_context_t context);
fbe_raid_position_bitmask_t fbe_raid_fruts_get_media_err_bitmap(fbe_raid_fruts_t * fruts_p,
                                              fbe_u16_t * input_hard_media_err_bitmap_p);
void fbe_raid_fruts_update_media_err_bitmap(fbe_raid_fruts_t *fruts_p);
fbe_status_t fbe_raid_fruts_get_retry_err_bitmap(fbe_raid_fruts_t * fruts_p,
                                                 fbe_raid_position_bitmask_t * input_retryable_err_bitmap_p);
fbe_status_t fbe_raid_fruts_get_retry_timeout_err_bitmap(fbe_raid_fruts_t * fruts_p,
                                                         fbe_raid_position_bitmask_t * input_retryable_err_bitmap_p);
void fbe_raid_fruts_get_bitmap_for_status(fbe_raid_fruts_t * fruts_p,
                                          fbe_raid_position_bitmask_t * output_err_bitmap_p,
                                          fbe_u16_t * output_err_count_p,
                                          fbe_payload_block_operation_status_t status,
                                          fbe_payload_block_operation_qualifier_t qualifier);
fbe_u16_t fbe_raid_fruts_chain_nop_find(fbe_raid_fruts_t * fruts_p,
                                        fbe_u16_t * input_nop_bitmap_p);

fbe_status_t fbe_raid_fruts_for_each(fbe_raid_position_bitmask_t fru_bitmap,
                                     fbe_raid_fruts_t * fruts_p,
                                     fbe_status_t fn(fbe_raid_fruts_t *, fbe_lba_t, fbe_u32_t *),
                                     fbe_u64_t  arg1,
                                     fbe_u64_t  arg2);

fbe_status_t fbe_raid_fruts_for_each_bit(fbe_raid_position_bitmask_t fru_bitmap,
                                     fbe_raid_fruts_t * fruts_p,
                                     fbe_status_t fn(fbe_raid_fruts_t *, fbe_lba_t, fbe_u32_t *),
                                     fbe_u64_t  arg1,
                                     fbe_u64_t  arg2);

fbe_status_t fbe_raid_fruts_for_each_only(fbe_u16_t fru_bitmap,
                                          fbe_raid_fruts_t * fruts_p,
                                          fbe_status_t fn(fbe_raid_fruts_t *, fbe_lba_t, fbe_u32_t *),
                                          fbe_u64_t  arg1,
                                          fbe_u64_t  arg2);
fbe_status_t fbe_raid_fruts_reset_rd(fbe_raid_fruts_t * fruts_p,
                             fbe_lba_t lba_inc,
                             fbe_u32_t * blocks);
fbe_status_t fbe_raid_fruts_reset_wr(fbe_raid_fruts_t * fruts_p,
                             fbe_lba_t lba_inc,
                             fbe_u32_t * blocks);
fbe_status_t fbe_raid_fruts_reset_vr(fbe_raid_fruts_t * fruts_p,
                             fbe_lba_t lba_inc,
                             fbe_u32_t * blocks);
fbe_status_t fbe_raid_fruts_reset(fbe_raid_fruts_t * fruts_p,
                          fbe_lba_t blocks,
                          fbe_u32_t * opcode);
fbe_status_t fbe_raid_fruts_clear_flags(fbe_raid_fruts_t * fruts_p,
                                fbe_lba_t unused_1,
                                fbe_u32_t * unused_2);
fbe_status_t fbe_raid_fruts_set_opcode(fbe_raid_fruts_t * fruts_p,
                                       fbe_lba_t opcode,
                                       fbe_u32_t * unused_2);
fbe_status_t fbe_raid_fruts_set_opcode_crc_check(fbe_raid_fruts_t * fruts_p,
                                       fbe_lba_t opcode,
                                       fbe_u32_t * unused_2);
fbe_status_t fbe_raid_fruts_set_opcode_success(fbe_raid_fruts_t * fruts_p,
                                       fbe_lba_t opcode,
                                       fbe_u32_t * unused_2);
fbe_status_t fbe_raid_fruts_retry(fbe_raid_fruts_t * fruts_p,
                          fbe_lba_t opcode,
                          fbe_u32_t * evaluate_state);
fbe_status_t fbe_raid_fruts_set_opcode_success(fbe_raid_fruts_t * fruts_p,
                                       fbe_lba_t opcode,
                                       fbe_u32_t * unused_2);
fbe_status_t fbe_raid_fruts_validate_sgs(fbe_raid_fruts_t * fruts_p,
                                 fbe_lba_t unused_1,
                                 fbe_u32_t * unused_2);
fbe_status_t fbe_raid_fruts_retry_preserve_flags(fbe_raid_fruts_t * fruts_p,
                                   fbe_lba_t opcode,
                                   fbe_u32_t * evaluate_state);
fbe_u16_t rg_get_fruts_retried_bitmap( fbe_raid_fruts_t * const fruts_p,
                                       fbe_u16_t * input_retried_bitmap_p );
fbe_status_t fbe_raid_fruts_retry_crc_error( fbe_raid_fruts_t * fruts_p,
                                             fbe_lba_t opcode,
                                             fbe_u32_t * evaluate_state );
fbe_bool_t fbe_raid_fruts_chain_get_eboard(fbe_raid_fruts_t * fruts_ptr, 
                                           fbe_raid_fru_eboard_t * eboard_p);
fbe_bool_t fbe_raid_fruts_chain_validate_sg(fbe_raid_fruts_t *fruts_p);
fbe_status_t fbe_raid_fruts_validate_sg(fbe_raid_fruts_t *const fruts_p);
fbe_raid_fruts_t *fbe_raid_fruts_unchain(fbe_raid_fruts_t * fruts_p, fbe_u16_t fru_bitmap);
void fbe_raid_fruts_set_fruts_degraded_nop(fbe_raid_fruts_t *fruts_p);
void fbe_raid_fruts_set_degraded_nop( fbe_raid_siots_t *const siots_p,
                                      fbe_raid_fruts_t * const fruts_p );
void fbe_raid_fruts_set_parity_nop( fbe_raid_siots_t *const siots_p,
                                    fbe_raid_fruts_t * const fruts_p );
fbe_u32_t fbe_raid_fruts_count_active( fbe_raid_siots_t *const siots_p,
                                       fbe_raid_fruts_t * const fruts_p );
void fbe_raid_fruts_chain_set_retryable( fbe_raid_siots_t *const siots_p,
                                         fbe_raid_fruts_t * const fruts_p,
                                         fbe_raid_position_bitmask_t *retryable_bitmap_p );
fbe_raid_fruts_t * fbe_raid_fruts_chain_get_by_position(fbe_queue_head_t *head_p,
                                                  fbe_u16_t position);
fbe_status_t fbe_raid_fruts_get_min_blocks_transferred(fbe_raid_fruts_t *const start_fruts_p,
                                                       const fbe_lba_t start_lba,
                                                       fbe_block_count_t *const min_blocks_p,
                                                       fbe_raid_position_bitmask_t *const errored_bitmask_p);
fbe_u16_t fbe_raid_fruts_get_chain_bitmap( fbe_raid_fruts_t * const fruts_p,
                                           fbe_u16_t * const input_fruts_bitmap_p );

fbe_u16_t fbe_raid_fruts_get_success_chain_bitmap( fbe_raid_fruts_t * fruts_p,
                                                   fbe_raid_position_bitmask_t *read_mask_p,
                                                   fbe_raid_position_bitmask_t *write_mask_p,
                                                   fbe_bool_t b_check_start );

fbe_status_t fbe_raid_siots_retry_fruts_chain(fbe_raid_siots_t * siots_p,
                                              fbe_raid_fru_eboard_t * eboard_p,
                                              fbe_raid_fruts_t *fruts_p);
fbe_status_t fbe_raid_fruts_abort_chain(fbe_queue_head_t *head_p);
fbe_u32_t fbe_raid_siots_get_inherited_error_count(fbe_raid_siots_t *const siots_p);

void fbe_raid_fruts_get_stats_filtered(fbe_raid_fruts_t *fruts_p,
                                       fbe_raid_library_statistics_t *stats_p);
void fbe_raid_fruts_get_stats(fbe_raid_fruts_t *fruts_p,
                              fbe_raid_fru_statistics_t *stats_p);
/**************************************** 
 * fbe_raid_fru_info.c 
 ****************************************/
fbe_status_t fbe_raid_fru_info_init(fbe_raid_fru_info_t *const fru_p,
                                    fbe_lba_t lba,
                                    fbe_block_count_t blocks,
                                    fbe_u32_t position );
fbe_status_t fbe_raid_fru_info_init_arrays(fbe_u32_t    width,
                                           fbe_raid_fru_info_t *read_info_p,
                                           fbe_raid_fru_info_t *write_info_p,
                                           fbe_raid_fru_info_t *read2_info_p);
fbe_status_t fbe_raid_fru_info_set_sg_index(fbe_raid_fru_info_t *fru_info_p,
                                            fbe_u16_t sg_count,
                                            fbe_bool_t b_log);
fbe_status_t fbe_raid_fru_info_populate_sg_index(fbe_raid_fru_info_t *fru_info_p,
                                                 fbe_u32_t page_size,
                                                 fbe_block_count_t *mem_left_p,
                                                 fbe_bool_t b_log);
fbe_status_t fbe_raid_fru_info_get_by_position(fbe_raid_fru_info_t *info_array_p,
                                               fbe_u32_t position,
                                               fbe_u32_t width,
                                               fbe_raid_fru_info_t **found_p);
fbe_status_t fbe_raid_fru_info_array_allocate_sgs(fbe_raid_siots_t *siots_p,
                                                  fbe_raid_memory_info_t *memory_info_p,
                                                  fbe_raid_fru_info_t *read_info_p,
                                                  fbe_raid_fru_info_t *read2_info_p,
                                                  fbe_raid_fru_info_t *write_info_p);
fbe_status_t fbe_raid_fru_info_array_allocate_sgs_sparse(fbe_raid_siots_t *siots_p,
                                                         fbe_raid_memory_info_t *memory_info_p,
                                                         fbe_raid_fru_info_t *read_info_p,
                                                         fbe_raid_fru_info_t *read2_info_p,
                                                         fbe_raid_fru_info_t *write_info_p);
fbe_status_t fbe_raid_fru_info_allocate_sgs(fbe_raid_siots_t *siots_p,
                                            fbe_raid_fruts_t *fruts_p,
                                            fbe_raid_memory_info_t *memory_info_p,
                                            fbe_raid_memory_type_t sg_type);
/**************************************** 
 * fbe_raid_memory.c 
 ****************************************/
fbe_status_t fbe_raid_memory_push_sgid_address(fbe_u32_t sg_size, 
                                       fbe_memory_id_t *const sg_id_addr_p,
                                       fbe_memory_id_t *dst_sg_id_list_p[]);
fbe_raid_sg_index_t fbe_raid_memory_sg_count_index(fbe_u32_t sg_size);
fbe_status_t fbe_raid_siots_free_memory(fbe_raid_siots_t *siots_p);
fbe_bool_t fbe_raid_siots_allocate_vcts(fbe_raid_siots_t *siots_p);
fbe_status_t fbe_raid_siots_allocate_iots_buffer(fbe_raid_siots_t *siots_p);
fbe_status_t fbe_raid_iots_allocate_siots(fbe_raid_iots_t *iots_p);
fbe_status_t fbe_raid_iots_consume_siots(fbe_raid_iots_t *iots_p,
                                         fbe_raid_memory_info_t *memory_info_p);
fbe_status_t fbe_raid_iots_free_memory(fbe_raid_iots_t *iots_p);
fbe_status_t fbe_raid_iots_restart_allocate_siots(fbe_raid_iots_t *iots_p);
fbe_status_t fbe_raid_siots_restart_allocate_memory(fbe_raid_siots_t *siots_p);
void fbe_raid_memory_info_log_request(fbe_raid_memory_info_t *const memory_info_p,
                                      fbe_trace_level_t trace_level,
                                      fbe_u32_t line,
                                      const char *function_p,
                                      fbe_char_t *msg_string_p, ...)
                                      __attribute__((format(__printf_func__,5,6)));
fbe_bool_t fbe_raid_siots_is_allocation_successful(fbe_raid_siots_t *siots_p);
fbe_bool_t fbe_raid_iots_is_allocation_successful(fbe_raid_iots_t *iots_p);
fbe_bool_t fbe_raid_iots_is_deferred_allocation_successful(fbe_raid_iots_t *iots_p);
fbe_bool_t fbe_raid_siots_is_deferred_allocation_successful(fbe_raid_siots_t *siots_p);
fbe_status_t fbe_raid_siots_allocate_memory(fbe_raid_siots_t *siots_p);
fbe_bool_t fbe_raid_memory_validate_page_size(fbe_u32_t page_size);
fbe_bool_t fbe_raid_siots_validate_page_size(fbe_raid_siots_t *siots_p);
fbe_u32_t fbe_raid_memory_chunk_size_to_page_size(fbe_u32_t chunk_size, fbe_bool_t b_control_mem);

fbe_u32_t fbe_raid_memory_calculate_ctrl_page_size(fbe_raid_siots_t * siots_p, fbe_u16_t num_fruts);
fbe_u32_t fbe_raid_memory_calculate_data_page_size(fbe_raid_siots_t * siots_p, fbe_u16_t num_blocks);
fbe_status_t fbe_raid_memory_calculate_num_pages(fbe_u32_t page_size_p,
                                         fbe_u32_t *num_pages_to_allocate_p,
                                         fbe_u16_t num_fruts,
                                         fbe_u16_t *num_sgs_p,
                                         fbe_block_count_t num_blocks,
                                         fbe_bool_t b_allocate_nested_siots);
fbe_status_t fbe_raid_siots_calculate_num_pages(fbe_raid_siots_t *siots_p,
                                                fbe_u16_t num_fruts,
                                                fbe_u16_t *num_sgs_p,
                                                fbe_bool_t b_allocate_nested_siots);
fbe_status_t fbe_raid_memory_calculate_num_data_pages(fbe_u32_t page_size,
                                                      fbe_u32_t *num_pages_to_allocate_p,
                                                      fbe_block_count_t num_blocks);
fbe_status_t fbe_raid_memory_calculate_num_pages_fast(fbe_u32_t ctrl_page_size,
                                                      fbe_u32_t data_page_size,
                                                      fbe_u16_t *num_control_pages_p,
                                                      fbe_u16_t *num_data_pages_p,
                                                      fbe_u16_t num_fruts,
                                                      fbe_u16_t num_sg_lists,
                                                      fbe_u16_t sg_count,
                                                      fbe_block_count_t num_blocks);
fbe_status_t fbe_raid_siots_get_fruts_chain(fbe_raid_siots_t *siots_p, 
                                         fbe_queue_head_t *head_p,
                                         fbe_u32_t count,
                                         fbe_raid_memory_info_t *memory_info_p);
void * fbe_raid_group_memory_allocate_synchronous(void);
fbe_u8_t *fbe_raid_memory_get_first_block_buffer(fbe_sg_element_t *sgl_p);
fbe_u8_t *fbe_raid_memory_get_last_block_buffer(fbe_sg_element_t *sgl_p);
fbe_u32_t fbe_raid_memory_pointer_to_u32(void *ptr);
fbe_bool_t fbe_raid_memory_is_buffer_space_remaining(fbe_raid_memory_info_t *memory_info_p,
                                                     fbe_u32_t bytes_to_plant);
fbe_raid_fruts_t *fbe_raid_siots_get_next_fruts(fbe_raid_siots_t *siots_p, 
                                                fbe_queue_head_t *head_p,
                                                fbe_raid_memory_info_t *memory_info_p);
void * fbe_raid_memory_allocate_contiguous(fbe_raid_memory_info_t *memory_info_p,
                                           fbe_u32_t bytes_to_allocate,
                                           fbe_u32_t *bytes_allocated_p);
void * fbe_raid_memory_allocate_next_buffer(fbe_raid_memory_info_t *memory_info_p,
                                           fbe_u32_t bytes_to_allocate,
                                           fbe_u32_t *bytes_allocated_p);
fbe_u8_t *fbe_raid_memory_get_buffer_end_address(fbe_queue_element_t *buffer_mem_p,
                                                 fbe_u32_t buffer_mem_left,
                                                 fbe_u32_t page_size);
fbe_status_t fbe_raid_memory_init_memory_info(fbe_raid_memory_info_t *memory_info_p,
                                              fbe_memory_request_t *memory_request_p, 
                                              fbe_bool_t b_control_mem);  
fbe_status_t fbe_raid_memory_apply_buffer_offset(fbe_raid_memory_info_t *memory_info_p,
                                                 fbe_u32_t buffer_bytes_to_allocate);
fbe_status_t fbe_raid_memory_apply_allocation_offset(fbe_raid_memory_info_t *memory_info_p,
                                                     fbe_u32_t buffer_bytes_to_allocate,
                                                     fbe_u32_t allocation_size);
fbe_status_t fbe_raid_library_destroy(void);
fbe_status_t fbe_raid_siots_init_data_mem_info(fbe_raid_siots_t *siots_p, 
                                               fbe_raid_memory_info_t *memory_info_p);
fbe_status_t fbe_raid_siots_init_control_mem_info(fbe_raid_siots_t *siots_p, 
                                                  fbe_raid_memory_info_t *memory_info_p);

/**************************************** 
 * fbe_raid_sg_util.c 
 ****************************************/
fbe_block_count_t fbe_raid_sg_count_uniform_blocks(fbe_block_count_t blks_to_add,
                                     fbe_block_count_t blks_per_buffer,
                                     fbe_block_count_t blks_remaining,
                                     fbe_u32_t * sg_totalp);
fbe_status_t fbe_raid_sg_clip_sg_list(fbe_sg_element_with_offset_t * src_sgd_ptr,
                                   fbe_sg_element_t * dest_sgl_ptr,
                                   fbe_u32_t dest_bytecnt,
                                   fbe_u16_t *sg_element_init_ptr);

fbe_status_t fbe_raid_sg_determine_sg_uniform(fbe_block_count_t block_count,
                                  fbe_memory_id_t * sg_id_addr,
                                  fbe_u32_t array_position,
                                  fbe_u32_t buffer_size,
                                  fbe_block_count_t *remaining_count_p,
                                  fbe_memory_id_t * sg_id_list[],
                                  fbe_u32_t sg_count_vec[]);
fbe_status_t fbe_raid_sg_count_sg_blocks(fbe_sg_element_t * sg_ptr, 
                                         fbe_u32_t * sg_count, fbe_block_count_t *block_in_sg_list_p );
fbe_status_t fbe_raid_sg_count_blocks_limit(fbe_sg_element_t * sg_ptr, 
                                            fbe_u32_t * sg_count, 
                                            fbe_block_count_t limit,
                                            fbe_block_count_t *block_in_sg_list_p );
fbe_u32_t fbe_raid_sg_get_sg_total(fbe_memory_id_t * sg_id_list[], fbe_u32_t sg_type);
fbe_status_t fbe_raid_sg_count_nonuniform_blocks(fbe_block_count_t blks_to_count,
                                              fbe_sg_element_with_offset_t * src_sgd_ptr,
                                              fbe_u16_t bytes_per_blk,
                                              fbe_block_count_t *blks_remaining_ptr,
                                              fbe_u32_t * sg_total_ptr);
fbe_block_count_t fbe_raid_sg_scatter_fed_to_bed(fbe_lba_t lda,
                                       fbe_block_count_t blks_to_scatter,
                                       fbe_u16_t data_pos,
                                       fbe_u16_t data_disks,
                                       fbe_lba_t blks_per_element,
                                       fbe_block_count_t blks_per_buffer,
                                       fbe_block_count_t blks_remaining,
                                       fbe_u32_t sg_total[]);
fbe_status_t fbe_raid_scatter_cache_to_bed(fbe_lba_t lda,
                                           fbe_block_count_t blks_to_scatter,
                                           fbe_u16_t data_pos,
                                           fbe_u16_t data_disks,
                                           fbe_lba_t blks_per_element,
                                           fbe_sg_element_with_offset_t * src_sgd_ptr,
                                           fbe_u32_t sg_total[]);
fbe_status_t fbe_raid_scatter_sg_to_bed(fbe_lba_t lda,
                                 fbe_u32_t blks_to_scatter,
                                 fbe_u16_t data_pos,
                                 fbe_u16_t data_disks,
                                 fbe_lba_t blks_per_element,
                                 fbe_sg_element_with_offset_t * src_sgd_ptr,
                                 fbe_sg_element_t * dest_sgl_ptrv[]);
fbe_status_t fbe_raid_scatter_sg_with_memory(fbe_lba_t lda,
                                             fbe_u32_t blks_to_scatter,
                                             fbe_u16_t data_pos,
                                             fbe_u16_t data_disks,
                                             fbe_lba_t blks_per_element,
                                             fbe_raid_memory_info_t *memory_info_p,
                                             fbe_sg_element_t * dest_sgl_ptrv[]);
fbe_sg_element_t *fbe_raid_copy_sg_list(fbe_sg_element_t * src_sgl_ptr,
                                        fbe_sg_element_t * dest_sgl_ptr);
void fbe_raid_adjust_sg_desc(fbe_sg_element_with_offset_t * sgd_p);
fbe_s32_t fbe_raid_get_sg_ptr_offset(fbe_sg_element_t ** sg_ptr_ptr,
                                     fbe_s32_t blocks);
fbe_u16_t fbe_raid_sg_populate_with_memory(fbe_raid_memory_info_t *memory_info_p,
                                           fbe_sg_element_t * dest_sgl_ptr,
                                           fbe_u32_t  dest_bytecnt);
fbe_block_count_t fbe_raid_determine_overlay_sgs(fbe_lba_t lba,
                                             fbe_block_count_t blocks,
                                             fbe_block_count_t buffer_blocks,
                                             fbe_sg_element_t *overlay_sg_ptr,
                                             fbe_lba_t overlay_start,
                                             fbe_block_count_t overlay_count,
                                             fbe_block_count_t mem_left,
                                             fbe_block_count_t * blocks_ptr,
                                             fbe_u32_t * sg_size_ptr);
 fbe_status_t fbe_raid_verify_setup_overlay_sgs(fbe_lba_t base_lba,
                                            fbe_block_count_t base_blocks,
                                            fbe_sg_element_t *base_sg_ptr,
                                            fbe_sg_element_t * sgl_ptr,
                                            fbe_raid_memory_info_t *memory_info_p,
                                            fbe_lba_t verify_start,
                                            fbe_block_count_t verify_count,
                                            fbe_block_count_t * assgined_blocks_p);

/**************************************** 
 * fbe_raid_check_zeroed.c 
 ****************************************/
fbe_raid_state_status_t fbe_raid_check_zeroed_state0(fbe_raid_siots_t * siots_p);

/**************************************** 
 * fbe_raid_xor.c 
 ****************************************/
void fbe_raid_xor_set_debug_options(fbe_raid_siots_t *const siots_p,
                                    fbe_xor_option_t *option_p,
                                    fbe_bool_t b_generate_error_trace);
fbe_status_t fbe_raid_xor_fruts_check_checksum(fbe_raid_fruts_t * fruts_ptr,
                                               fbe_bool_t b_force_crc_check);
fbe_status_t fbe_raid_xor_read_check_checksum(fbe_raid_siots_t * const siots_p,
                                              fbe_bool_t b_force_crc_check,
                                              fbe_bool_t b_should_check_stamps);
fbe_status_t fbe_xor_lib_check_lba_stamp(fbe_xor_execute_stamps_t * const execute_stamps_p);
fbe_status_t fbe_raid_xor_striper_write_check_checksum(fbe_raid_siots_t * const siots_p);
fbe_status_t fbe_raid_xor_mirror_write_check_checksum(fbe_raid_siots_t * const siots_p,
                                                      const fbe_bool_t b_should_generate_stamps);
fbe_status_t fbe_raid_xor_mirror_preread_check_checksum(fbe_raid_siots_t * const siots_p,
                                              fbe_bool_t b_force_crc_check,
                                              fbe_bool_t b_should_check_stamps);
fbe_status_t fbe_raid_xor_write_log_header_set_stamps(fbe_raid_siots_t * const siots_p,
                                                const fbe_bool_t b_first_only);
fbe_status_t fbe_raid_xor_check_checksum(fbe_raid_siots_t * const siots_p,
                                         fbe_xor_option_t option);
fbe_status_t fbe_raid_xor_check_iots_checksum(fbe_raid_siots_t * const siots_p);
fbe_status_t fbe_raid_xor_check_siots_checksum(fbe_raid_siots_t * const siots_p);
fbe_status_t fbe_raid_xor_get_buffered_data(fbe_raid_siots_t * const siots_p);
fbe_status_t fbe_raid_xor_check_lba_stamp(fbe_raid_siots_t * const siots_p);

/**************************************** 
 * fbe_raid_util.c 
 ****************************************/
fbe_bool_t fbe_raid_siots_is_quiesced(fbe_raid_siots_t * const siots_p);
fbe_status_t fbe_raid_util_get_valid_bitmask(fbe_u32_t width, fbe_raid_position_bitmask_t *valid_bitmask_p);
fbe_bool_t fbe_raid_is_option_flag_set(fbe_raid_geometry_t *raid_geometry_p,
                                                       fbe_raid_option_flags_t flags);
fbe_raid_option_flags_t fbe_raid_get_option_flags(fbe_raid_geometry_t *raid_geometry_p);

fbe_u32_t fbe_raid_max_transfer_size(fbe_bool_t host_transfer,
                                     fbe_lba_t lba,
                                     fbe_u32_t blocks_remaining,
                                     fbe_u16_t data_disks,
                                     fbe_lba_t blks_per_element);
fbe_u32_t fbe_raid_get_transfer_limit(fbe_raid_siots_t *siots_p,
                                      fbe_u32_t array_width,
                                      fbe_u32_t blocks,
                                      fbe_lba_t lba,
                                      fbe_bool_t host_transfer,
                                      fbe_u32_t host_io_check_count);

fbe_u32_t fbe_raid_get_max_blocks(fbe_raid_siots_t *const siots_p,
                                  fbe_lba_t lba,
                                  fbe_u32_t blocks,
                                  fbe_u32_t width,
                                  fbe_bool_t align);
fbe_bool_t fbe_raid_siots_mark_quiesced(fbe_raid_siots_t * const siots_p);
fbe_raid_state_status_t fbe_raid_siots_generate_get_lock(fbe_raid_siots_t * siots_p);
fbe_status_t fbe_raid_iots_increment_stripe_crossings(fbe_raid_iots_t* iots_p);
fbe_status_t fbe_raid_fruts_increment_disk_statistics(fbe_raid_fruts_t *fruts_p);
fbe_bool_t fbe_raid_get_checked_byte_count(fbe_u64_t blocks, fbe_u32_t *dest_byte_count);
fbe_bool_t fbe_raid_is_aligned_to_optimal_block_size(fbe_lba_t lba,
                                                     fbe_block_count_t blocks,
                                                     fbe_u32_t optimal_block_size);

/**************************************** 
 * fbe_raid_check_sector.c 
 ****************************************/
fbe_status_t fbe_raid_fruts_check_sectors( fbe_raid_fruts_t *fruts_p );
fbe_status_t fbe_raid_siots_check_data_pattern( fbe_raid_siots_t *siots_p );

/**************************************** 
 * fbe_raid_csum_util.c 
 ****************************************/
fbe_bool_t fbe_raid_csum_handle_csum_error(fbe_raid_siots_t *const siots_p,
                                           fbe_raid_fruts_t *const fruts_p,
                                           fbe_payload_block_operation_opcode_t opcode,
                                           fbe_raid_siots_state_t retry_state);
void fbe_raid_report_retried_crc_errs( fbe_raid_siots_t * const siots_p );

/**************************************** 
 * fbe_raid_record_errors.c 
 ****************************************/
fbe_raid_state_status_t fbe_raid_siots_record_errors(fbe_raid_siots_t * const siots_p,
                                                     fbe_u16_t width,
                                                     fbe_xor_error_t * eboard_p,
                                                     fbe_u16_t record_these_bits,
                                                     fbe_bool_t allow_correctable,
                                                     fbe_bool_t validate);
void fbe_raid_siots_update_iots_vp_eboard( fbe_raid_siots_t *siots_p );
fbe_status_t fbe_raid_report_errors( fbe_raid_siots_t * const siots_p );
fbe_u16_t fbe_raid_csum_get_error_bitmask( fbe_raid_siots_t * const siots_p );
fbe_lba_t fbe_raid_get_region_lba_for_position(fbe_raid_siots_t * const siots_p,
                                         fbe_u16_t pos,
                                         fbe_lba_t default_lba);
fbe_u32_t fbe_raid_get_uncorrectable_status_bits( const fbe_xor_error_t * const eboard_p, fbe_u16_t position_key );
void fbe_raid_region_fix_crc_reason( fbe_u16_t xor_error_type, fbe_u32_t *err_bits );
fbe_u32_t fbe_raid_get_correctable_status_bits( const fbe_raid_vrts_t * const vrts_p, 
                                          fbe_u16_t position_key );
fbe_status_t fbe_raid_send_unsol(fbe_raid_siots_t *siots_p,
                                 fbe_u32_t error_code,
                                 fbe_u32_t fru_pos,
                                 fbe_lba_t lba,
                                 fbe_u32_t blocks,
                                 fbe_u32_t error_info,
                                 fbe_u32_t extra_info );
fbe_status_t fbe_raid_get_error_bits_for_given_region(const fbe_xor_error_region_t * const xor_region_p,
                                                       fbe_bool_t b_errors_retried,
                                                       fbe_u16_t position_key,
                                                       fbe_bool_t b_correctable,
                                                       fbe_u32_t * err_bits_p);
fbe_status_t fbe_raid_record_bad_crc_on_write(fbe_raid_siots_t * siots_p);
fbe_u32_t fbe_raid_extra_info(fbe_raid_siots_t *siots_p);
fbe_u32_t fbe_raid_extra_info_alg(fbe_raid_siots_t *siots_p, fbe_u32_t algorithm);

/**************************************** 
 * fbe_raid_perf_stats.c 
 ****************************************/
fbe_status_t
fbe_raid_perf_stats_inc_mr3_write(fbe_lun_performance_counters_t *lun_perf_stats_p,
                                  fbe_cpu_id_t cpu_id);
fbe_status_t
fbe_raid_perf_stats_get_mr3_write(fbe_lun_performance_counters_t *lun_perf_stats_p, 
                                  fbe_u64_t *mr3_write_p,
                                  fbe_cpu_id_t cpu_id);
fbe_status_t
fbe_raid_perf_stats_inc_stripe_crossings(fbe_lun_performance_counters_t *lun_perf_stats_p,
                                         fbe_cpu_id_t cpu_id);
fbe_status_t
fbe_raid_perf_stats_get_stripe_crossings(fbe_lun_performance_counters_t *lun_perf_stats_p,
                                         fbe_u64_t *stripe_crossings_p,
                                         fbe_cpu_id_t cpu_id);
fbe_status_t
fbe_raid_perf_stats_inc_disk_reads(fbe_lun_performance_counters_t *lun_perf_stats_p,
                                   fbe_u32_t position,
                                   fbe_cpu_id_t cpu_id);
fbe_status_t
fbe_raid_perf_stats_get_disk_reads(fbe_lun_performance_counters_t *lun_perf_stats_p, 
                                   fbe_u32_t position,
                                   fbe_u64_t *disk_reads_p,
                                   fbe_cpu_id_t cpu_id);
fbe_status_t
fbe_raid_perf_stats_inc_disk_writes(fbe_lun_performance_counters_t *lun_perf_stats_p,
                                    fbe_u32_t position,
                                    fbe_cpu_id_t cpu_id);
fbe_status_t
fbe_raid_perf_stats_get_disk_writes(fbe_lun_performance_counters_t *lun_perf_stats_p, 
                                   fbe_u32_t position,
                                   fbe_u64_t *disk_writes_p,
                                   fbe_cpu_id_t cpu_id);
fbe_status_t
fbe_raid_perf_stats_inc_disk_blocks_read(fbe_lun_performance_counters_t *lun_perf_stats_p,
                                         fbe_u32_t position,
                                         fbe_block_count_t blocks,
                                         fbe_cpu_id_t cpu_id);

fbe_status_t
fbe_raid_perf_stats_get_disk_blocks_read(fbe_lun_performance_counters_t *lun_perf_stats_p, 
                                         fbe_u32_t position,
                                         fbe_u64_t *disk_blocks_read_p,
                                         fbe_cpu_id_t cpu_id);
fbe_status_t
fbe_raid_perf_stats_inc_disk_blocks_written(fbe_lun_performance_counters_t *lun_perf_stats_p,
                                            fbe_u32_t position,
                                            fbe_block_count_t blocks,
                                            fbe_cpu_id_t cpu_id);
fbe_status_t
fbe_raid_perf_stats_get_disk_blocks_written(fbe_lun_performance_counters_t *lun_perf_stats_p, 
                                            fbe_u32_t position,
                                            fbe_u64_t *disk_blocks_written_p,
                                            fbe_cpu_id_t cpu_id);

fbe_status_t
fbe_raid_perf_stats_set_timestamp(fbe_lun_performance_counters_t *lun_perf_stats_p);
fbe_status_t
fbe_raid_perf_stats_get_timestamp(fbe_lun_performance_counters_t *lun_perf_stats_p, 
                                  fbe_u64_t *timestamp_p);


fbe_raid_common_error_precedence_t fbe_raid_siots_get_error_precedence(fbe_payload_block_operation_status_t error,
                                                                       fbe_payload_block_operation_qualifier_t qualifier);

/**************************************
 * end of file fbe_raid_library_proto.h
 **************************************/

#endif
