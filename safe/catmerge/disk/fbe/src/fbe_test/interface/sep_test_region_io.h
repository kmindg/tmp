/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file sep_test_region_io.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the interface to the sep test library for region I/O.
 *
 * @version
 *   9/19/2011:  Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

void fbe_test_setup_region_test_contexts(fbe_api_rdgen_context_t *context_p,
                                                fbe_rdgen_operation_t rdgen_operation,
                                                fbe_object_id_t lun_object_id, 
                                                fbe_block_count_t max_blocks, 
                                                fbe_u32_t max_regions,
                                                fbe_lba_t capacity,
                                                fbe_api_rdgen_peer_options_t peer_options,
                                                fbe_data_pattern_t pattern);

void fbe_test_get_lu_rand_blocks(fbe_block_count_t max_blocks,
                                          fbe_lba_t capacity,
                                          fbe_api_raid_group_class_get_info_t *rg_info_p,
                                          fbe_block_count_t *current_max_blocks_p);
fbe_u32_t fbe_test_get_random_region_length(void);
fbe_status_t fbe_test_setup_lun_region_contexts(fbe_test_rg_configuration_t *rg_config_p,
                                                       fbe_api_rdgen_context_t *context_p,
                                                       fbe_u32_t num_contexts,
                                                       fbe_rdgen_operation_t rdgen_operation,
                                                       fbe_block_count_t max_blocks,
                                                       fbe_api_rdgen_peer_options_t peer_options,
                                                       fbe_data_pattern_t pattern);
void fbe_test_setup_dual_region_contexts(fbe_test_rg_configuration_t *const rg_config_p, 
                                         fbe_api_rdgen_peer_options_t peer_options, 
                                         fbe_data_pattern_t background_pattern, 
                                         fbe_data_pattern_t pattern, 
                                         fbe_api_rdgen_context_t **read_context_p, 
                                         fbe_api_rdgen_context_t **write_context_p, 
                                         fbe_u32_t background_pass_count,
                                         fbe_u32_t max_io_size);
void fbe_test_run_foreground_region_write(fbe_test_rg_configuration_t *const rg_config_p, 
                                          fbe_data_pattern_t background_pattern, 
                                          fbe_api_rdgen_context_t * read_context_p, 
                                          fbe_api_rdgen_context_t * write_context_p);

fbe_status_t fbe_test_setup_active_region_context(fbe_test_rg_configuration_t *rg_config_p,
                                                  fbe_api_rdgen_context_t *context_p,
                                                  fbe_u32_t num_contexts,
                                                  fbe_bool_t b_active);
/*************************
 * end file sep_region_io.h
 *************************/


