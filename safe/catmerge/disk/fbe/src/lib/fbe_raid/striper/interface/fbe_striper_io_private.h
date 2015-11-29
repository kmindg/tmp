#ifndef STRIPER_IO_PRIVATE_H
#define STRIPER_IO_PRIVATE_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_striper_io_private.h
 ***************************************************************************
 *
 * @brief
 *  This file contains private defines for the striper library
 *  structures and defines.
 * 
 * @ingroup striper_library_files
 * 
 * @version
 *  11/23/2009 - Created. Rob Foley
 *
 ***************************************************************************/

#include "fbe/fbe_transport.h"
#include "fbe_block_transport.h"
#include "fbe_raid_geometry.h"
#include "fbe_raid_iots_accessor.h"
//#include "fbe_raid_library_proto.h"
#include "fbe_raid_library.h"

/******************************
 * LITERAL DEFINITIONS.
 ******************************/

/*!*******************************************************************
 * @def FBE_STRIPER_MIN_PARITY_SIZE_TO_CHECK
 *********************************************************************
 * @brief This is an arbitrary size, beyond which we want to make sure
 *        that we do not exceed the sg limits.
 *        This currently assumes a single block per sg element.
 *
 *********************************************************************/
#define FBE_STRIPER_MIN_PARITY_SIZE_TO_CHECK FBE_RAID_MAX_SG_DATA_ELEMENTS


/*!*******************************************************************
 * @def FBE_STRIPER_MIN_ZERO_STRIPE_OPTIMIZE
 *********************************************************************
 * @brief Min stripes for which we will try to optimize a zero.
 *
 *********************************************************************/
#define FBE_STRIPER_MIN_ZERO_STRIPE_OPTIMIZE 3

/* fbe_striper_generate.c
 */
fbe_raid_state_status_t fbe_striper_generate_start(fbe_raid_siots_t * siots_p);
fbe_status_t fbe_striper_generate_validate_siots(fbe_raid_siots_t *siots_p);

/* fbe_striper_geometry.c
 */
/*fbe_bool_t fbe_striper_get_physical_geometry(fbe_raid_geometry_t *raid_geometry_p,
                                             fbe_lba_t pba,
                                             fbe_raid_siots_geometry_t *geo,
                                             fbe_u32_t max_window_size,
                                             fbe_u16_t *const unit_width);*/

/* fbe_striper_util.c
 */
fbe_raid_fru_error_status_t fbe_striper_get_fruts_error(fbe_raid_fruts_t *fruts_p,
                                                        fbe_raid_fru_eboard_t * eboard);
fbe_status_t fbe_striper_invalidate_hard_err(fbe_raid_siots_t *siots_p,
                                             fbe_raid_fru_eboard_t * eboard_p);

fbe_raid_state_status_t fbe_striper_add_media_error_region_and_validate(fbe_raid_siots_t *siots_p, fbe_raid_fruts_t* read_fruts_p,
																  fbe_u16_t      media_err_pos_bitmask);

fbe_raid_state_status_t fbe_striper_handle_fruts_error(fbe_raid_siots_t *siots_p,
                                                       fbe_raid_fruts_t *fruts_p,
                                                       fbe_raid_fru_eboard_t * eboard_p,
                                                       fbe_raid_fru_error_status_t fruts_error_status);
/* fbe_striper_read.c 
 */
fbe_raid_state_status_t fbe_striper_read_state0(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_striper_read_state1(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_striper_read_state2(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_striper_read_state3(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_striper_read_state4(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_striper_read_state5(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_striper_read_state6(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_striper_read_state7(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_striper_read_state8(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_striper_read_state20(fbe_raid_siots_t * siots_p);

/* fbe_striper_read_util.c 
 */
fbe_status_t fbe_striper_read_get_fruts_info(fbe_raid_siots_t * siots_p,
                                             fbe_raid_fru_info_t *read_p);
fbe_status_t fbe_striper_read_calculate_num_sgs(fbe_raid_siots_t *siots_p,
                                                fbe_u16_t *num_sgs_p,
                                                fbe_raid_fru_info_t *read_p,
                                                fbe_bool_t b_generate);
fbe_status_t fbe_striper_read_calculate_memory_size(fbe_raid_siots_t *siots_p,
                                                    fbe_raid_fru_info_t *read_info_p);
fbe_status_t fbe_striper_read_setup_resources(fbe_raid_siots_t * siots_p, 
                                              fbe_raid_fru_info_t *read_p);

/* fbe_striper_read.c 
 */
fbe_raid_state_status_t fbe_striper_write_state0(fbe_raid_siots_t * siots_p);

/* fbe_striper_write_util.c 
 */
fbe_status_t fbe_striper_write_get_fruts_info(fbe_raid_siots_t * siots_p,
                                              fbe_raid_fru_info_t *read_p,
                                              fbe_raid_fru_info_t *write_p,
                                              fbe_block_count_t *read_blocks_p,
                                              fbe_u32_t *read_fruts_count_p);
fbe_status_t fbe_striper_write_calculate_num_sgs(fbe_raid_siots_t *siots_p,
                                                 fbe_u16_t *num_sgs_p,
                                                 fbe_raid_fru_info_t *read_p,
                                                 fbe_raid_fru_info_t *write_p,
                                                 fbe_bool_t b_generate);
fbe_status_t fbe_striper_write_calculate_memory_size(fbe_raid_siots_t *siots_p,
                                                     fbe_raid_fru_info_t *read_info_p,
                                                     fbe_raid_fru_info_t *write_info_p);
fbe_status_t fbe_striper_write_setup_resources(fbe_raid_siots_t * siots_p, 
                                              fbe_raid_fru_info_t *read_p,
                                              fbe_raid_fru_info_t *write_p);

/* fbe_striper_verify.c
 */
fbe_raid_state_status_t fbe_striper_recovery_verify_state0(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_striper_bva_vr_state0(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_striper_verify_state0(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_striper_verify_state0a(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_striper_verify_state1(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_striper_verify_state2(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_striper_verify_state3(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_striper_verify_state4(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_striper_verify_state5(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_striper_verify_state6(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_striper_verify_state7(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_striper_verify_state8(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_striper_verify_state9(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_striper_verify_state10(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_striper_verify_state11(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_striper_verify_state20(fbe_raid_siots_t * siots_p);
fbe_raid_state_status_t fbe_striper_verify_state21(fbe_raid_siots_t * siots_p);


/* fbe_striper_verify_util.c
 */
fbe_status_t fbe_striper_verify_get_fruts_info(fbe_raid_siots_t * siots_p,
                                               fbe_raid_fru_info_t *read_p);
fbe_status_t fbe_striper_verify_calculate_memory_size(fbe_raid_siots_t *siots_p,
                                                      fbe_raid_fru_info_t *read_info_p);
fbe_status_t fbe_striper_verify_setup_resources(fbe_raid_siots_t * siots_p, 
                                               fbe_raid_fru_info_t *read_p);
fbe_status_t fbe_striper_verify_validate(fbe_raid_siots_t * siots_p);
fbe_status_t fbe_striper_verify_strip(fbe_raid_siots_t * siots_p);
fbe_status_t fbe_striper_verify_report_errors(fbe_raid_siots_t * siots_p);
fbe_status_t fbe_striper_verify_setup_sgs_for_next_pass(fbe_raid_siots_t * siots_p);
fbe_status_t fbe_striper_bva_setup_nsiots(fbe_raid_siots_t * siots_p);
fbe_status_t fbe_striper_generate_expanded_request(fbe_raid_siots_t *const siots_p);
fbe_u32_t fbe_raid_get_correctable_status_bits( const fbe_raid_vrts_t * const vrts_p, 
                                          fbe_u16_t position_key );
fbe_status_t fbe_striper_report_errors(fbe_raid_siots_t * siots_p, fbe_bool_t b_update_eboard);
fbe_u16_t fbe_striper_verify_get_write_bitmap(fbe_raid_siots_t * const siots_p);

/* fbe_striper_zero.c
 */
fbe_raid_state_status_t fbe_striper_zero_state0(fbe_raid_siots_t * siots_p);
#endif /* STRIPER_IO_PRIVATE_H */

/*******************************
 * end fbe_striper_io_private.h
 *******************************/
