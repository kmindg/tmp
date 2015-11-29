#ifndef PARITY_IO_PRIVATE_H
#define PARITY_IO_PRIVATE_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_parity_io_private.h
 ***************************************************************************
 *
 * @brief
 *  This file contains private defines for the parity library.
 * 
 * @ingroup parity_library_files
 * 
 * @version
 *   11/23/2009 - Created. Rob Foley
 *
 ***************************************************************************/

#include "fbe/fbe_transport.h"
#include "fbe_block_transport.h"
#include "fbe/fbe_parity.h"
#include "fbe_raid_geometry.h"
#include "fbe_raid_iots_accessor.h"
#include "fbe_raid_siots_accessor.h"
#include "fbe_raid_library_private.h"
#include "fbe_raid_library.h"
/******************************
 * LITERAL DEFINITIONS.
 ******************************/


/*!*******************************************************************
 * @def FBE_PARITY_MIN_PARITY_SIZE_TO_CHECK
 *********************************************************************
 * @brief This is an arbitrary size, beyond which we want to make sure
 *        that we do not exceed the sg limits.
 *        This currently assumes a single block per sg element.
 *
 *********************************************************************/
#define FBE_PARITY_MIN_PARITY_SIZE_TO_CHECK FBE_RAID_MAX_SG_DATA_ELEMENTS
/* Imports for visibility.
 */


/**********************************************************************
 *
 * Journal Write Log Data Structures.
 *
 **********************************************************************/

/* Value for a metadata journal lba offset when it is not used.
 */
#define FBE_PARITY_JOURNAL_INVALID_LBA_OFFSET 0xFFFF

/* Journal data for representing an entry in the journal that is
 * written out with MDS.
 */
#pragma pack(1)
typedef struct fbe_parity_metadata_journal_data_s
{
    fbe_u16_t lba_offset[FBE_RAID_GROUP_MAX_DISK_ARRAY_WIDTH];
    fbe_u16_t block_count[FBE_RAID_GROUP_MAX_DISK_ARRAY_WIDTH];
}fbe_parity_metadata_journal_data_t;
#pragma pack()

/* fbe_parity_generate.c
 */
fbe_u32_t fbe_parity_check_for_degraded( fbe_raid_siots_t *siots_p );
fbe_status_t fbe_parity_siots_init_dead_position(fbe_raid_siots_t * const siots_p, 
                              fbe_u32_t array_width,
                              fbe_lba_t *degraded_offset_p);
fbe_status_t fbe_raid_extent_is_nr(fbe_lba_t lba, 
                                   fbe_block_count_t blocks,
                                   fbe_raid_nr_extent_t *extent_p,
                                   fbe_u32_t max_extents,
                                   fbe_bool_t *b_is_nr,
                                   fbe_block_count_t *blocks_p);

void fbe_parity_gen_deg_rd(fbe_raid_siots_t * const siots_p);
fbe_bool_t fbe_parity_degraded_read_required(fbe_raid_siots_t * const siots_p);
fbe_status_t fbe_parity_check_degraded_phys( fbe_raid_siots_t *siots_p);
fbe_status_t fbe_parity_generate_validate_siots(fbe_raid_siots_t *siots_p); 

/* fbe_parity_geometry.c
 */
/*fbe_status_t fbe_parity_get_physical_geometry(fbe_raid_geometry_t *raid_geometry_p,
                                            fbe_lba_t pba,
                                            fbe_raid_siots_geometry_t *geo);*/
fbe_status_t fbe_parity_get_small_read_geometry(fbe_raid_geometry_t *raid_geometry_p,
                                                fbe_lba_t lba,
                                                fbe_raid_small_read_geometry_t * geo);
fbe_status_t fbe_parity_get_small_write_geometry(fbe_raid_geometry_t *raid_geometry_p,
                                                 fbe_lba_t lba,
                                                 fbe_raid_siots_geometry_t * geo);
/* fbe_parity_util.c
 */
fbe_raid_fru_error_status_t fbe_parity_get_fruts_error(fbe_raid_fruts_t *fruts_p,
                                                       fbe_raid_fru_eboard_t * eboard);
fbe_bool_t fbe_parity_siots_is_degraded(fbe_raid_siots_t *const siots_p);
fbe_raid_state_status_t fbe_parity_handle_fruts_error(fbe_raid_siots_t *siots_p,
                                                      fbe_raid_fruts_t *fruts_p,
                                                      fbe_raid_fru_eboard_t * eboard_p,
                                                      fbe_raid_fru_error_status_t fruts_error_status);

/* fbe_parity_read.c 
 */
fbe_raid_state_status_t fbe_parity_read_state0(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_read_state1(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_read_state2(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_read_state3(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_read_state4(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_read_state5(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_read_state6(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_read_state7(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_read_state8(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_read_state11(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_read_state12(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_read_state14(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_read_state15(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_read_state20(fbe_raid_siots_t * siots_p);

/* fbe_parity_small_read.c 
 */
fbe_raid_state_status_t fbe_parity_small_read_state0(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_small_read_state1(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_small_read_state2(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_small_read_state3(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_small_read_state4(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_small_read_state5(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_small_read_state6(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_small_read_state7(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_small_read_state8(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_small_read_state20(fbe_raid_siots_t * siots_p);

/* fbe_parity_small_read_util.c
 */
fbe_status_t fbe_parity_small_read_calculate_memory_size(fbe_raid_siots_t *siots_p,
                                                         fbe_u32_t *num_pages_to_allocate_p);
fbe_status_t fbe_parity_small_read_setup_resources(fbe_raid_siots_t * siots_p);

/* fbe_parity_read_util.c 
 */
fbe_status_t fbe_parity_read_get_fruts_info(fbe_raid_siots_t * siots_p,
                                            fbe_raid_fru_info_t *read_p);
fbe_status_t fbe_parity_read_calculate_num_sgs(fbe_raid_siots_t *siots_p,
                                               fbe_u16_t *num_sgs_p,
                                               fbe_raid_fru_info_t *read_p,
                                               fbe_bool_t b_log);
fbe_bool_t fbe_parity_read_fruts_chain_touches_dead_pos( fbe_raid_siots_t * const siots_p, 
                                                    fbe_raid_fruts_t *const fruts_p );
fbe_status_t fbe_parity_read_calculate_memory_size(fbe_raid_siots_t *siots_p,
                                                   fbe_raid_fru_info_t *read_info_p,
                                                   fbe_u32_t *num_pages_to_allocate_p);
fbe_status_t fbe_parity_read_setup_resources(fbe_raid_siots_t * siots_p, 
                                             fbe_raid_fru_info_t *read_p);
/* fbe_parity_degraded_read.c 
 */
fbe_raid_state_status_t fbe_parity_degraded_read_transition(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_degraded_read_state0(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_degraded_read_state1(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_degraded_read_state2(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_degraded_read_state3(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_degraded_read_state4(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_degraded_read_state5(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_degraded_read_state6(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_degraded_read_state7(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_degraded_read_state8(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_degraded_read_state9(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_degraded_read_state11(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_degraded_read_state12(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_degraded_read_state13(fbe_raid_siots_t * siots_p);
static fbe_raid_state_status_t fbe_parity_degraded_read_state16(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_degraded_read_state20(fbe_raid_siots_t * siots_p);

/* fbe_parity_degraded_read_util.c 
 */
fbe_status_t fbe_parity_degraded_read_transition_setup(fbe_raid_siots_t * siots_p);
fbe_status_t fbe_parity_degraded_read_setup_fruts(fbe_raid_siots_t * siots_p,
                                                  fbe_raid_memory_info_t *memory_info_p);
fbe_block_count_t fbe_parity_degraded_read_count_buffers(fbe_raid_siots_t * siots_p);
fbe_status_t fbe_parity_degraded_read_setup_sgs(fbe_raid_siots_t * siots_p,
                                                fbe_raid_memory_info_t *memory_info_p);
fbe_raid_state_status_t fbe_parity_degraded_read_reconstruct(fbe_raid_siots_t * siots_p);
fbe_status_t fbe_parity_degraded_read_calculate_memory_size(fbe_raid_siots_t *siots_p,
                                                            fbe_raid_fru_info_t *read_info_p,
                                                            fbe_raid_fru_info_t *write_info_p,
                                                            fbe_u32_t *num_pages_to_allocate_p);
fbe_status_t fbe_parity_degraded_read_setup_resources(fbe_raid_siots_t * siots_p,
                                                      fbe_raid_fru_info_t *read_info_p,
                                                      fbe_raid_fru_info_t *write_info_p);

fbe_status_t fbe_parity_degraded_read_get_dead_data_positions(fbe_raid_siots_t * siots_p,
                                                              fbe_u16_t *dead_data_pos_1,
                                                              fbe_u16_t *dead_data_pos_2);
/* fbe_parity_468.c     
 */
fbe_raid_state_status_t fbe_parity_small_468_state0(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_small_468_state1(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_468_state0(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_468_state1(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_468_state2(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_468_state3(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_468_state4(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_468_state5(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_468_state6(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_468_state7(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_468_state8(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_468_state9(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_468_state10(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_468_state11(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_468_state12(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_468_state13(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_468_state14(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_468_state15(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_468_state16(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_468_state17(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_468_state18(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_468_state19(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_468_state2(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_468_state20(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_468_state21(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_468_state22(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_468_state23(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_468_state25(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_468_state26(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_468_state27(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_468_state30(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_468_state40(fbe_raid_siots_t * siots_p);

/* fbe_parity_468_util.c 
 */
fbe_status_t fbe_parity_small_468_calculate_memory_size(fbe_raid_siots_t *siots_p,
                                                       fbe_raid_memory_type_t *type_p);
fbe_status_t fbe_parity_small_468_setup_resources(fbe_raid_siots_t * siots_p, 
                                                 fbe_raid_memory_type_t type);
fbe_status_t fbe_parity_small_468_setup_embed_resources(fbe_raid_siots_t * siots_p);
fbe_status_t fbe_parity_468_calculate_memory_size(fbe_raid_siots_t *siots_p,
                                                  fbe_raid_fru_info_t *read_info_p,
                                                  fbe_raid_fru_info_t *write_info_p,
                                                  fbe_u32_t *num_pages_to_allocate_p);
fbe_status_t fbe_parity_468_setup_resources(fbe_raid_siots_t * siots_p, 
                                            fbe_raid_fru_info_t *read_p,
                                            fbe_raid_fru_info_t *write_p);
fbe_status_t fbe_parity_468_xor(fbe_raid_siots_t * const siots_p);
fbe_u32_t fbe_parity_468_count_buffers(fbe_raid_siots_t *siots_p);
fbe_u16_t fbe_parity_468_degraded_count(fbe_raid_siots_t *const siots_p);
fbe_status_t fbe_parity_468_calculate_num_sgs(fbe_raid_siots_t *siots_p,
                                              fbe_u16_t *num_sgs_p,
                                              fbe_raid_fru_info_t *read_p,
                                              fbe_raid_fru_info_t *write_p,
                                              fbe_bool_t b_generate);
fbe_status_t fbe_parity_468_get_fruts_info(fbe_raid_siots_t * siots_p,
                                           fbe_raid_fru_info_t *read_p,
                                           fbe_raid_fru_info_t *write_p,
                                           fbe_block_count_t *read_blocks_p);
void fbe_raid_siots_setup_preread_fru_info(fbe_raid_siots_t *siots_p,
                                           fbe_raid_fru_info_t * const read_fruts_ptr, 
                                           fbe_raid_fru_info_t * const write_fruts_ptr,
                                           fbe_u32_t array_pos);
fbe_status_t fbe_raid_setup_preread_fruts(fbe_raid_siots_t *siots_p,
                                  fbe_raid_fruts_t * const read_fruts_ptr, 
                                  fbe_raid_fruts_t * const write_fruts_ptr,
                                  fbe_u32_t array_pos);
fbe_status_t fbe_parity_468_unaligned_get_pre_read_info(fbe_payload_pre_read_descriptor_t *const pre_read_descriptor_p,
                                        fbe_lba_t write_lba, 
                                        fbe_block_count_t write_blocks,
                                        fbe_lba_t *const output_lba_p, 
                                        fbe_block_count_t *const output_blocks_p);
fbe_status_t fbe_parity_is_preread_degraded(fbe_raid_siots_t *const siots_p,
                                             fbe_bool_t *b_return);
fbe_status_t fbe_parity_get_buffered_data(fbe_raid_siots_t * const siots_p);

/* fbe_parity_mr3.c 
 */
fbe_raid_state_status_t fbe_parity_mr3_state0(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_mr3_state1(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_mr3_state2(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_mr3_state3(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_mr3_state4(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_mr3_state5(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_mr3_state6(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_mr3_state7(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_mr3_state8(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_mr3_state9(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_mr3_state10(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_mr3_state11(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_mr3_state12(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_mr3_state13(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_mr3_state14(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_mr3_state15(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_mr3_state16(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_mr3_state17(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_mr3_state18(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_mr3_state19(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_mr3_state20(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_mr3_state21(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_mr3_state22(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_mr3_state23(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_mr3_state25(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_mr3_state26(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_mr3_state27(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_mr3_state30(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_mr3_state40(fbe_raid_siots_t * siots_p);

/* fbe_parity_mr3_util.c 
 */
fbe_status_t fbe_parity_mr3_get_fruts_info(fbe_raid_siots_t * siots_p,
                                           fbe_raid_fru_info_t *read_p,
                                           fbe_raid_fru_info_t *read2_p,
                                           fbe_raid_fru_info_t *write_info_p,
                                           fbe_block_count_t *read_blocks_p,
                                           fbe_u32_t*read_fruts_p);
fbe_status_t fbe_parity_mr3_calculate_num_sgs(fbe_raid_siots_t *siots_p,
                                              fbe_u16_t *num_sgs_p,
                                              fbe_raid_fru_info_t *read_p,
                                              fbe_raid_fru_info_t *read2_p,
                                              fbe_raid_fru_info_t *write_p,
                                              fbe_bool_t b_generate);
fbe_status_t fbe_parity_mr3_xor(fbe_raid_siots_t * const siots_p);
fbe_block_count_t fbe_parity_mr3_count_buffers(fbe_raid_siots_t *siots_p);
fbe_u16_t fbe_parity_mr3_degraded_count(fbe_raid_siots_t *const siots_p);

fbe_status_t fbe_parity_mr3_calculate_memory_size(fbe_raid_siots_t *siots_p,
                                                  fbe_raid_fru_info_t *read_info_p,
                                                  fbe_raid_fru_info_t *read2_info_p,
                                                  fbe_raid_fru_info_t *write_info_p);
fbe_status_t fbe_parity_mr3_setup_resources(fbe_raid_siots_t * siots_p, 
                                            fbe_raid_fru_info_t *read_p,
                                            fbe_raid_fru_info_t *read2_p,
                                            fbe_raid_fru_info_t *write_p);
fbe_status_t fbe_parity_mr3_get_buffered_data(fbe_raid_siots_t * const siots_p);
fbe_status_t fbe_parity_mr3_handle_timeout_error(fbe_raid_siots_t *siots_p);
fbe_status_t fbe_parity_mr3_handle_retry_error(fbe_raid_siots_t *siots_p,
                                               fbe_raid_fru_eboard_t * eboard_p,
                                               fbe_raid_fru_error_status_t read1_status, 
                                               fbe_raid_fru_error_status_t read2_status);

/* fbe_parity_rcw.c     
 */
fbe_raid_state_status_t fbe_parity_rcw_state0(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_rcw_state1(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_rcw_state2(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_rcw_state3(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_rcw_state4(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_rcw_state5(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_rcw_state6(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_rcw_state7(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_rcw_state8(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_rcw_state9(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_rcw_state10(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_rcw_state11(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_rcw_state12(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_rcw_state13(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_rcw_state14(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_rcw_state15(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_rcw_state16(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_rcw_state17(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_rcw_state18(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_rcw_state19(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_rcw_state2(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_rcw_state20(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_rcw_state21(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_rcw_state22(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_rcw_state23(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_rcw_state25(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_rcw_state26(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_rcw_state27(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_rcw_state30(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_rcw_state40(fbe_raid_siots_t * siots_p);

/* fbe_parity_rcw_util.c 
 */
fbe_status_t fbe_parity_rcw_calculate_memory_size(fbe_raid_siots_t *siots_p,
                                                  fbe_raid_fru_info_t *read_info_p,
                                                  fbe_raid_fru_info_t *read2_info_p,
                                                  fbe_raid_fru_info_t *write_info_p,
                                                  fbe_u32_t *num_pages_to_allocate_p);
fbe_status_t fbe_parity_rcw_setup_resources(fbe_raid_siots_t * siots_p, 
                                            fbe_raid_fru_info_t *read_p,
                                            fbe_raid_fru_info_t *read2_p,
                                            fbe_raid_fru_info_t *write_p);
fbe_status_t fbe_parity_rcw_xor(fbe_raid_siots_t * const siots_p);
fbe_u16_t fbe_parity_rcw_degraded_count(fbe_raid_siots_t *const siots_p);
fbe_status_t fbe_parity_rcw_calculate_num_sgs(fbe_raid_siots_t *siots_p,
                                              fbe_u16_t *num_sgs_p,
                                              fbe_raid_fru_info_t *read_p,
                                              fbe_raid_fru_info_t *read2_p,
                                              fbe_raid_fru_info_t *write_p,
                                              fbe_bool_t b_generate);
fbe_status_t fbe_parity_rcw_get_fruts_info(fbe_raid_siots_t * siots_p,
                                           fbe_raid_fru_info_t *read_p,
                                           fbe_raid_fru_info_t *read2_p,
                                           fbe_raid_fru_info_t *write_p,
                                           fbe_block_count_t *read_blocks_p,
                                           fbe_u32_t *read_fruts_count_p);
fbe_status_t fbe_parity_rcw_unaligned_get_pre_read_info(fbe_payload_pre_read_descriptor_t *const pre_read_descriptor_p,
                                        fbe_lba_t write_lba, 
                                        fbe_block_count_t write_blocks,
                                        fbe_lba_t *const output_lba_p, 
                                        fbe_block_count_t *const output_blocks_p);
fbe_status_t fbe_parity_rcw_handle_retry_error(fbe_raid_siots_t *siots_p,
                                               fbe_raid_fru_eboard_t * eboard_p,
                                               fbe_raid_fru_error_status_t read1_status, 
                                               fbe_raid_fru_error_status_t read2_status);


/* fbe_parity_verify.c
 */
fbe_raid_state_status_t fbe_parity_verify_state0(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_verify_state0a(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_verify_state1(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_verify_state2(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_verify_state3(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_verify_state4(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_verify_state4a(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_verify_state5(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_verify_state6(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_verify_state7(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_verify_state8(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_verify_state9(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_verify_state10(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_verify_state11(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_verify_state19(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_verify_state20(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_verify_state21(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_verify_state22(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_verify_state25(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_degraded_verify_state0(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_recovery_verify_state0(fbe_raid_siots_t * siots_p);
/* fbe_raid_state_status_t fbe_parity_recovery_verify_state1(fbe_raid_siots_t * siots_p); Not in use */
fbe_raid_state_status_t fbe_parity_recovery_verify_state2(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_recovery_verify_state3(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_degraded_recovery_verify_state0(fbe_raid_siots_t * siots_p);

/* fbe_parity_verify_util.c
 */
fbe_bool_t fbe_parity_verify_validate(fbe_raid_siots_t * siots_p);
fbe_status_t fbe_parity_verify_strip(fbe_raid_siots_t * siots_p);
void fbe_parity_rvr_setup_sgs(fbe_raid_siots_t *siots_p, 
                               const fbe_lba_t verify_start, 
                               const fbe_u32_t verify_count);
void fbe_parity_rvr_count_assigned_sgs(fbe_block_count_t blks_to_count, 
                                                   fbe_sg_element_with_offset_t * sg_desc_ptr, 
                                                   fbe_u32_t blks_per_buffer, 
                                                   fbe_u16_t * sg_size_ptr);
fbe_status_t fbe_parity_recovery_verify_setup_geometry(fbe_raid_siots_t * siots_p,
                                               fbe_bool_t b_align);
fbe_status_t fbe_parity_verify_setup_resources(fbe_raid_siots_t * siots_p, fbe_raid_fru_info_t *read_p);
fbe_status_t fbe_parity_verify_get_fruts_info(fbe_raid_siots_t * siots_p,
                                              fbe_raid_fru_info_t *read_p);
fbe_status_t fbe_parity_verify_calculate_memory_size(fbe_raid_siots_t *siots_p,
                                                     fbe_raid_fru_info_t *read_info_p,
                                                     fbe_u32_t *num_pages_to_allocate_p);
fbe_u16_t fbe_parity_verify_get_write_bitmap(fbe_raid_siots_t * const siots_p);
fbe_raid_state_status_t fbe_parity_record_errors(fbe_raid_siots_t * const siots_p, 
                                                 fbe_lba_t verify_address);
fbe_status_t fbe_parity_report_errors(fbe_raid_siots_t * siots_p, fbe_bool_t b_update_eboard);
fbe_status_t fbe_parity_report_media_errors(fbe_raid_siots_t * siots_p,
                                            fbe_raid_fruts_t * const fruts_p,
                                            const fbe_bool_t correctable);
fbe_status_t fbe_parity_verify_get_verify_count(fbe_raid_siots_t *siots_p,
                                                fbe_block_count_t *verify_count_p);
fbe_status_t fbe_parity_recovery_verify_copy_user_data(fbe_raid_siots_t * siots_p);
fbe_status_t fbe_parity_recovery_verify_construct_parity(fbe_raid_siots_t * siots_p);
fbe_status_t fbe_parity_verify_setup_sgs_for_next_pass(fbe_raid_siots_t * siots_p);
fbe_status_t fbe_parity_report_errors_from_eboard(fbe_raid_siots_t * siots_p);
fbe_status_t  fbe_parity_report_errors_from_error_region(fbe_raid_siots_t * siots_p);
fbe_u32_t fbe_raid_get_correctable_status_bits( const fbe_raid_vrts_t * const vrts_p, 
                                          fbe_u16_t position_key );
fbe_status_t fbe_parity_log_correctable_errors_from_error_region(fbe_raid_siots_t * siots_p,
                                                            fbe_u32_t region_idx,
                                                            fbe_u16_t array_pos,
                                                            fbe_u32_t err_bits);
fbe_status_t fbe_parity_log_correct_errors_from_eboard(fbe_raid_siots_t * siots_p,
                                          fbe_u32_t array_pos,
                                          fbe_lba_t parity_start_pba,
                                          fbe_u32_t err_bits,
                                          fbe_bool_t* b_row_reconstructed,
                                          fbe_bool_t* b_diag_reconstructed);
fbe_status_t fbe_parity_log_uncorrectable_errors_from_eboard(fbe_raid_siots_t * siots_p,
                                            fbe_bool_t b_log_uc_pos,
                                            fbe_bool_t b_log_uc_stripe,
                                            fbe_u32_t array_pos,
                                            fbe_lba_t parity_start_pba,
                                            fbe_u32_t err_bits,
                                            fbe_bool_t* data_invalid,
                                            fbe_bool_t* parity_invalid,
                                            fbe_lba_t* data_invalid_pba);
fbe_status_t fbe_parity_log_uncorrectable_errors_from_error_region(fbe_raid_siots_t * siots_p,
                                                                   fbe_u32_t region_index,
                                                                   fbe_u16_t array_pos,
                                                                   fbe_bool_t * b_uc_error_logged_p);
fbe_bool_t fbe_parity_only_invalidated_in_error_regions(fbe_raid_siots_t * siots_p);
void fbe_parity_log_parity_invalidation(fbe_raid_siots_t * siots_p, fbe_u16_t * local_u_bitmask);
fbe_status_t fbe_parity_bva_log_error_in_write(fbe_raid_siots_t *siots_p,
                                fbe_u32_t region_idx,
                                fbe_u32_t array_pos,
                                fbe_u32_t sunburst,
                                fbe_u32_t err_bits);
fbe_status_t fbe_raid_convert_pba_to_lba(fbe_raid_siots_t * siots_p, 
                            fbe_lba_t disk_pba, 
                                         fbe_u32_t array_pos,
                                         fbe_lba_t *lba_for_pba_p);
fbe_status_t fbe_parity_bva_is_err_on_data_overwritten(fbe_raid_siots_t * siots_p,
                                       fbe_lba_t error_pba,
                                       fbe_u32_t array_pos,
                                       fbe_bool_t* b_err_corr);
fbe_status_t fbe_parity_bva_is_err_on_parity_overwritten(fbe_raid_siots_t *siots_p,
                                         fbe_lba_t error_pba,
                                         fbe_u32_t array_pos,
                                         fbe_bool_t* b_err_corr);
void fbe_parity_bva_report_sectors_reconstructed(fbe_raid_siots_t * siots_p, 
                                         fbe_lba_t data_pba,
                                         fbe_u32_t sunburst,
                                         fbe_u32_t err_bits,
                                         fbe_u32_t array_pos,
                                         fbe_bool_t corrected,
                                         fbe_u32_t blocks);

/* fbe_parity_rebuild.c
 */
fbe_raid_state_status_t fbe_parity_rebuild_state0(fbe_raid_siots_t * siots_p);

/* fbe_parity_rebuild_util.c
 */
fbe_status_t fbe_parity_rebuild_get_fru_info(fbe_raid_siots_t * siots_p, 
                                             fbe_raid_fru_info_t *read_p, 
                                             fbe_raid_fru_info_t *write_p);
fbe_status_t fbe_parity_rebuild_setup_resources(fbe_raid_siots_t * siots_p, 
                                                fbe_raid_fru_info_t *read_p, 
                                                fbe_raid_fru_info_t *write_p);
fbe_status_t fbe_parity_rebuild_calculate_memory_size(fbe_raid_siots_t *siots_p,
                                                      fbe_raid_fru_info_t *read_info_p,
                                                      fbe_raid_fru_info_t *write_info_p,
                                                      fbe_u32_t *num_pages_to_allocate_p);
fbe_status_t fbe_parity_rebuild_extent(fbe_raid_siots_t * siots_p);
fbe_status_t fbe_parity_rebuild_multiple_writes(fbe_raid_siots_t * siots_p, fbe_u16_t w_bitmap);
fbe_status_t fbe_parity_rebuild_reinit(fbe_raid_siots_t * siots_p);

/* fbe_parity_zero.c
 */
fbe_raid_state_status_t fbe_parity_zero_state0(fbe_raid_siots_t * siots_p);

/* fbe_parity_journal_flush.c 
 */
fbe_raid_state_status_t fbe_parity_write_log_flush_state0000(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_write_log_flush_state0001(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_write_log_flush_state000(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_write_log_flush_state001(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_write_log_flush_state002(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_write_log_flush_state003(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_write_log_flush_state004(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_write_log_flush_state005(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_write_log_flush_state006(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_write_log_flush_state007(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_write_log_flush_state008(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_journal_flush_state0(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_journal_flush_state1(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_journal_flush_state2(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_journal_flush_state4(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_journal_flush_state5(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_journal_flush_state6(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_journal_flush_state7(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_journal_flush_state8(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_journal_flush_state9(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_journal_flush_state10(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_journal_flush_state11(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_journal_flush_state12(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_journal_flush_state13(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_journal_flush_state14(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_journal_flush_state15(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_write_log_flush_state16(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_write_log_flush_state17(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_write_log_flush_state18(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_write_log_flush_state19(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_write_log_flush_completed(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_parity_write_log_flush_slot_dataloss(fbe_raid_siots_t * siots_p);

/* fbe_parity_write_log util functions */
fbe_status_t fbe_parity_journal_build_data(fbe_raid_siots_t *siots_p);
fbe_status_t fbe_parity_prepare_journal_write(fbe_raid_siots_t *const siots_p);
fbe_status_t fbe_parity_restore_journal_write(fbe_raid_siots_t *const siots_p);

fbe_status_t fbe_parity_journal_flush_restore_dest_lba(fbe_raid_siots_t *siots_p);
fbe_status_t fbe_parity_journal_flush_generate_write_fruts(fbe_raid_siots_t *siots_p);
fbe_status_t fbe_parity_write_log_generate_header_invalidate_fruts(fbe_raid_siots_t *siots_p);

fbe_status_t fbe_parity_journal_flush_calculate_memory_size(fbe_raid_siots_t *siots_p,
															fbe_u32_t fru_info_size,
                                                            fbe_raid_fru_info_t *journal_fru_info_p,
                                                            fbe_raid_fru_info_t *preread_fru_info_p);

fbe_status_t fbe_parity_journal_flush_setup_resources(fbe_raid_siots_t * siots_p, 
													  fbe_raid_fru_info_t * journal_fru_info_p,
													  fbe_raid_fru_info_t * preread_fru_info_p);

fbe_status_t fbe_parity_journal_header_read_setup_resources(fbe_raid_siots_t * siots_p);

fbe_status_t fbe_parity_write_log_allocate_slot(fbe_raid_siots_t *siots_p);
fbe_status_t fbe_parity_write_log_release_slot(fbe_raid_siots_t *siots_p, const char *function, fbe_u32_t line);
fbe_status_t fbe_parity_write_log_set_slot_invalidate_state(fbe_raid_siots_t *siots_p,
                                                            fbe_parity_write_log_slot_invalidate_state_t state,
                                                            const char *function, fbe_u32_t line);
fbe_status_t fbe_parity_write_log_get_slot_invalidate_state(fbe_raid_siots_t *siots_p,
                                                            fbe_parity_write_log_slot_invalidate_state_t *state,
                                                            const char *function, fbe_u32_t line);

fbe_status_t fbe_parity_write_log_calc_csum_of_csums(fbe_raid_siots_t * siots_p,
                                                     fbe_raid_fruts_t * fruts_p,
                                                     fbe_bool_t set_csum,
                                                     fbe_u16_t *chksum_err_bitmap);
fbe_bool_t fbe_parity_is_write_logging_required(fbe_raid_siots_t *siots_p);
void fbe_parity_write_log_remove_failed_read_fruts(fbe_raid_siots_t *const siots_p, fbe_raid_fru_eboard_t * eboard_p);
void fbe_parity_write_log_move_read_fruts_to_write_chain( fbe_raid_siots_t *const siots_p);

fbe_status_t fbe_parity_write_log_get_iots_memory_header(fbe_raid_siots_t *siots_p, fbe_parity_write_log_header_t ** header_pp);
fbe_status_t fbe_parity_write_log_validate_headers(fbe_raid_siots_t *const siots_p, fbe_u32_t * num_drive_ops, fbe_bool_t * b_flush, 
												   fbe_bool_t * b_inv);
fbe_status_t fbe_parity_invalidate_fruts_write_log_header_buffer(fbe_raid_fruts_t * fruts_p,
                                                                 fbe_lba_t unused_1,
                                                                 fbe_u32_t * unused_2);
fbe_status_t fbe_parity_get_fruts_write_log_header(fbe_raid_fruts_t * fruts_p,
                                                   fbe_parity_write_log_header_t ** header_pp);

fbe_status_t fbe_parity_write_log_send_flush_abandoned_event(fbe_raid_siots_t * siots_p,
                                                             fbe_u32_t err_pos_bitmap,
                                                             fbe_u32_t error_info,
                                                             fbe_raid_siots_algorithm_e algorithm);

/* fbe_parity_rekey.c
 */
fbe_raid_state_status_t fbe_parity_rekey_state0(fbe_raid_siots_t * siots_p);

/* fbe_parity_rekey_util.c
 */
fbe_bool_t fbe_parity_rekey_validate(fbe_raid_siots_t * siots_p);
fbe_status_t fbe_parity_rekey_calculate_memory_size(fbe_raid_siots_t *siots_p,
                                                     fbe_raid_fru_info_t *read_info_p,
                                                     fbe_u32_t *num_pages_to_allocate_p);
fbe_status_t fbe_parity_rekey_setup_resources(fbe_raid_siots_t * siots_p, 
                                              fbe_raid_fru_info_t *read_p);
#endif /* PARITY_PRIVATE_H */

/*******************************
 * end fbe_parity_io_private.h
 *******************************/
