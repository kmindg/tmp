#ifndef FBE_LOGICAL_ERROR_INJECTION_PROTO_H
#define FBE_LOGICAL_ERROR_INJECTION_PROTO_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_logical_error_injection_proto.h
 ***************************************************************************
 *
 * @brief
 *  This file contains local prototypes for the logical_error_injection service.
 *
 * @version
 *  4/18/2011 - Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_queue.h"
#include "fbe/fbe_memory.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_logical_error_injection_service.h"
#include "fbe_block_transport.h"
#include "fbe/fbe_sector.h"
#include "fbe/fbe_xor_api.h"
#include "fbe_raid_library_proto.h"

/* fbe_logical_error_injection_main.c
 */
void fbe_logical_error_injection_service_trace(fbe_trace_level_t trace_level,
                         fbe_trace_message_id_t message_id,
                         const char * fmt, ...) __attribute__((format(__printf_func__,3,4)));

void fbe_logical_error_injection_complete_packet(fbe_packet_t *packet_p,
                               fbe_payload_control_status_t control_payload_status,
                               fbe_status_t packet_status);

fbe_status_t fbe_logical_error_injection_get_object(fbe_object_id_t object_id,
                                                    fbe_package_id_t package_id, 
                                                    fbe_logical_error_injection_object_t **object_pp);
fbe_logical_error_injection_object_t *fbe_logical_error_injection_find_object(fbe_object_id_t object_id);
fbe_logical_error_injection_object_t *fbe_logical_error_injection_find_object_no_lock(fbe_object_id_t object_id);
fbe_status_t fbe_logical_error_injection_destroy_object(fbe_logical_error_injection_object_t *object_p);

/* fbe_logical_error_injection_object.c
 */
fbe_status_t fbe_logical_error_injection_object_init(fbe_logical_error_injection_object_t *object_p,
                                                     fbe_object_id_t object_id,
                                                     fbe_package_id_t package_id);
fbe_status_t fbe_logical_error_injection_object_initialize_capacity(fbe_logical_error_injection_object_t *object_p);
void fbe_logical_error_injection_object_destroy(fbe_logical_error_injection_object_t *object_p);
fbe_status_t fbe_logical_error_injection_object_modify(fbe_logical_error_injection_object_t *object_p,
                                                       fbe_u32_t edge_enable_bitmask,
                                                       fbe_lba_t injection_lba_adjustment);
fbe_status_t fbe_logical_error_injection_object_enable(fbe_logical_error_injection_object_t *object_p);
fbe_status_t fbe_logical_error_injection_object_disable(fbe_logical_error_injection_object_t *object_p);
fbe_status_t fbe_logical_error_injection_object_modify_for_class(fbe_logical_error_injection_filter_t *filter_p,
                                                                 fbe_u32_t edge_enable_bitmask,
                                                                 fbe_lba_t injection_lba_adjustment);
fbe_status_t fbe_logical_error_injection_object_enable_class(fbe_logical_error_injection_filter_t *filter_p);
fbe_status_t fbe_logical_error_injection_object_disable_class(fbe_logical_error_injection_filter_t *filter_p);
fbe_status_t fbe_logical_error_injection_object_prepare_destroy(fbe_logical_error_injection_object_t *object_p);

/* fbe_logical_error_injection_control.c
 */
fbe_status_t fbe_logical_error_injection_enumerate_class_exclude_system_objects(fbe_class_id_t class_id,
                                                         fbe_package_id_t package_id,
                                                         fbe_object_id_t *object_list_p,
                                                         fbe_u32_t total_objects,
                                                         fbe_u32_t *actual_num_objects_p);
fbe_status_t fbe_logical_error_injection_get_total_objects_of_class(fbe_class_id_t class_id,
                                                                    fbe_package_id_t package_id,
                                                                    fbe_u32_t *total_objects_p);
fbe_status_t fbe_logical_error_injection_set_block_edge_hook(fbe_object_id_t object_id, 
                                                             fbe_package_id_t package_id,
                                                             fbe_transport_control_set_edge_tap_hook_t *hook_info_p);
fbe_status_t fbe_logical_error_injection_hook_function(fbe_packet_t * packet_p);
fbe_status_t logical_error_injection_get_object_class_id (fbe_object_id_t object_id, fbe_class_id_t *class_id, fbe_package_id_t package_id);

/* fbe_logical_error_injection_record_mgmt.c
 */
fbe_lba_t fbe_logical_error_injection_get_table_lba(fbe_raid_siots_t *siots_p, fbe_lba_t start_lba, fbe_block_count_t blocks);
void fbe_logical_error_injection_adjust_start_lba(fbe_raid_siots_t* const siots_p,
                                                  fbe_lba_t *start_lba_p,
                                                  fbe_bool_t b_adjust_up);
fbe_status_t fbe_logical_error_injection_fruts_inject_all( fbe_raid_fruts_t * const fruts_p );
fbe_status_t fbe_logical_error_injection_remove_poc(void);
fbe_status_t fbe_logical_error_injection_table_validate(void);
fbe_status_t fbe_logical_error_injection_table_randomize(void);
fbe_s32_t fbe_logical_error_injection_bitmap_to_position(const fbe_u16_t bitmap);
fbe_u16_t fbe_logical_error_injection_convert_bitmap_log2phys(fbe_raid_siots_t* const siots_p,
                                                              fbe_u16_t bitmap_log);
fbe_bool_t fbe_logical_error_injection_coherency_adjacent( fbe_raid_siots_t* const siots_p,
                                                           fbe_logical_error_injection_record_t* const rec_p,
                                                           fbe_u16_t width,
                                                           fbe_u16_t deg_bitmask );
fbe_bool_t fbe_logical_error_injection_is_coherency_err_type( const fbe_logical_error_injection_record_t* const rec_p );
fbe_bool_t fbe_logical_error_injection_rec_evaluate(fbe_raid_siots_t* const siots_p,
                                                    fbe_s32_t idx_record,
                                                    fbe_u16_t err_adj_phys);
fbe_bool_t fbe_logical_error_injection_tbl_for_all_types(void);
fbe_bool_t fbe_logical_error_injection_overlap(fbe_lba_t lba,  fbe_block_count_t count,
                                               fbe_lba_t elba, fbe_block_count_t ecount);
fbe_status_t fbe_logical_error_injection_corrupt_sector(fbe_sector_t * sector_p,
                                                        const fbe_logical_error_injection_record_t * const rec_p,
                                                        fbe_u16_t pos, 
                                                        fbe_lba_t seed,
                                                        fbe_raid_group_type_t raid_type,
                                                        fbe_bool_t b_is_proactive_sparing,
                                                        fbe_lba_t start_lba,
                                                        fbe_block_count_t xfer_count,
                                                        fbe_u32_t width,
                                                        fbe_raid_position_bitmask_t parity_bitmask,
                                                        fbe_bool_t b_is_parity_position);
fbe_bool_t fbe_logical_error_injection_has_adjacent_type( fbe_raid_siots_t* const siots_p,
                                                          fbe_logical_error_injection_record_t* const rec_p,
                                                          fbe_bool_t is_match_fn(const fbe_logical_error_injection_record_t *const),
                                                          fbe_u16_t pos_bitmask );
fbe_bool_t fbe_logical_error_injection_is_table_for_all_types(void);
fbe_logical_error_injection_object_t *fbe_logical_error_injection_get_downstream_object(fbe_raid_siots_t *const siots_p,
                                                                                        fbe_u32_t position);
fbe_logical_error_injection_object_t *fbe_logical_error_injection_get_upstream_object(fbe_raid_fruts_t *const fruts_p);
fbe_status_t fbe_logical_error_injection_validate_record_type(fbe_xor_error_type_t err_type);

/* fbe_logical_error_injection_block_operations.c
 */
void fbe_logical_error_injection_block_io_inject_all( fbe_packet_t* in_packet_p );

void fbe_logical_error_injection_get_client_id(fbe_packet_t* packet_p, fbe_object_id_t * object_id);

/* fbe_logical_error_injection_tables.c
 */
fbe_status_t fbe_logical_error_injection_load_table(fbe_u32_t table_number,
                                                    fbe_bool_t b_simulation);
fbe_status_t fbe_logical_error_injection_disable_records(fbe_u32_t start_index, fbe_u32_t num_to_clear);
fbe_status_t fbe_logical_error_injection_validate_and_randomize_table(void);
fbe_status_t fbe_logical_error_injection_get_table_info(fbe_u32_t table_number,
                                                         fbe_bool_t b_simulation,
                                                         fbe_logical_error_injection_table_description_t *table_desc_p);
fbe_status_t fbe_logical_error_injection_get_active_table_info(fbe_bool_t b_simulation,
                                                         fbe_logical_error_injection_table_description_t *table_desc_p);

/* fbe_logical_error_injection_record_validate.c
 */
fbe_status_t fbe_logical_error_injection_correction_validate(fbe_raid_siots_t* const siots_p,
                                                             fbe_xor_error_region_t **unmatched_error_region_p);

/* Delay packet thread 
 */
void fbe_logical_error_injection_wakeup_delay_thread(void);
fbe_status_t fbe_logical_error_injection_add_packet_to_delay_queue(fbe_packet_t* packet_p, 
                                                                   fbe_object_id_t object_id,
                                                                   fbe_logical_error_injection_record_t *rec_p);
fbe_status_t fbe_logical_error_injection_delay_thread_init(fbe_logical_error_injection_thread_t *thread_p);
void fbe_logical_error_injection_delay_thread_func(void * context);
void fbe_logical_error_injection_service_destroy_threads(void);
fbe_status_t fbe_logical_error_injection_validate_fruts(fbe_packet_t * packet_p,
                                                        fbe_raid_fruts_t **fruts_pp);
fbe_u16_t fbe_logical_error_injection_get_valid_bitmask(fbe_raid_siots_t *const siots_p);
fbe_u32_t fbe_logical_error_injection_get_first_set_position(fbe_u16_t position_bitmask);
fbe_u32_t fbe_logical_error_injection_get_bit_count(fbe_u16_t position_bitmask);

/*************************
 * end file fbe_logical_error_injection_proto.h
 *************************/
#endif /* end FBE_LOGICAL_ERROR_INJECTION_PROTO_H */
