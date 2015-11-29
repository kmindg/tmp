/****************************************************************
 *  Copyright (C)  EMC Corporation 2006-2010
 *  All rights reserved.
 *  Licensed material - property of EMC Corporation.
 ****************************************************************/

/*!**************************************************************************
 * @file xor_raid6_util.h
 ***************************************************************************
 *
 * @brief
 *  This file contains support for the eval_parity_unit_r6 function that 
 *  implement the EVENODD algorithm which is used for RAID 6 functionality.
 *
 * @notes
 *
 * @author
 *
 * 04/06/06 - Created. RCL
 *
 **************************************************************************/

#ifndef FBE_XOR_RAID6_UTIL_H
#define FBE_XOR_RAID6_UTIL_H

#include "fbe/fbe_sector.h"
#include "fbe/fbe_sector_trace_interface.h"
#include "fbe/fbe_library_interface.h"

/* This enumeration holds the possible parity indexes into ppos array.
 */
typedef enum fbe_xor_r6_parity_positions_s
{
    FBE_XOR_R6_PARITY_POSITION_ROW = 0,
    FBE_XOR_R6_PARITY_POSITION_DIAG = 1
}
fbe_xor_r6_parity_positions_t;

#define FBE_XOR_LOGICAL_ROW_PARITY_POSITION (FBE_XOR_INV_DRV_POS - 1)
#define FBE_XOR_LOGICAL_DIAG_PARITY_POSITION (FBE_XOR_INV_DRV_POS - 2)

/* This macro adds an error to the fatal_key of a scratch buffer and updates
 * the fatal_cnt.  The value m_scratch should be a pointer to the scratch
 * buffer.
 */
static __forceinline void fbe_xor_scratch_r6_add_error(fbe_xor_scratch_r6_t * m_scratch, 
                                                fbe_u16_t m_bitkey)
{
    if (!(m_scratch->fatal_key & m_bitkey))
    {
        m_scratch->fatal_key |= m_bitkey;
        m_scratch->fatal_cnt++;
    }
    return;
}
/* end fbe_xor_scratch_r6_add_error */

/* This macro removes an error to the fatal_key of a scratch buffer and updates
 * the fatal_cnt.  The value m_scratch should be a pointer to the scratch
 * buffer.
 */
static __forceinline fbe_status_t fbe_xor_scratch_r6_remove_error(fbe_xor_scratch_r6_t * m_scratch, 
                                                   fbe_u16_t m_bitkey)
{
    /* If this is not an old error something is very wrong.
     */                                                           
    if (XOR_COND((m_scratch->fatal_key & m_bitkey) == 0))
    {
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "(m_scratch->fatal_key 0x%x & m_bitkey 0x%x) == 0\n",
                              m_scratch->fatal_key,
                              m_bitkey);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    
    m_scratch->fatal_key &= ~m_bitkey;
    m_scratch->fatal_cnt--;
    return FBE_STATUS_OK;
}
/* end fbe_xor_scratch_r6_remove_error() */

/* This macro will update the fatal_blk pointer of the scratch if a new error
 * has been found. The value m_scratch should be a pointer to the scratch
 * buffer. 
 */
static __forceinline fbe_status_t fbe_xor_scratch_r6_add_fatal_block(fbe_xor_scratch_r6_t * m_scratch, 
                                               fbe_u16_t m_bitkey, 
                                               fbe_sector_t * m_block)
{
    if (m_scratch->fatal_cnt <= 1)                               
    {                                                         
        /* If this is a duplicate of an error we are already aware of 
         * something is very wrong.                                   
         */                                                           
        if (XOR_COND((m_scratch->fatal_key & m_bitkey) != 0))
        {
            fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                  "(m_scratch->fatal_key 0x%x & m_bitkey 0x%x) != 0)\n",
                                  m_scratch->fatal_key,
                                  m_bitkey);

            return FBE_STATUS_GENERIC_FAILURE;
        }
                

        /* This is the first fatality. Save the position of the drive 
         * in the array.                                              
         */                                                           
        m_scratch->fatal_blk[m_scratch->fatal_cnt] = m_block;      
        /* If we can perform a reconstruction, we'll have to back     
         * this data out of the scratch sector.                       
        */                                                           
        m_scratch->xor_required = FBE_TRUE;                           
    }                                                         
    fbe_xor_scratch_r6_add_error(m_scratch, m_bitkey);
    return FBE_STATUS_OK;
}
/* end fbe_xor_scratch_r6_add_fatal_block() */


/* xor_raid6_reconstruct_1.c
 */
fbe_status_t fbe_xor_calc_csum_and_reconstruct_1(fbe_xor_scratch_r6_t * const scratch_p,
                                        fbe_sector_t * const data_blk_p,
                                        const fbe_u32_t column_index,
                                        const fbe_u16_t rebuild_pos,
                                        fbe_u32_t *uncooked_csum_p);

fbe_u32_t fbe_xor_calc_csum_and_parity_reconstruct_1(fbe_xor_scratch_r6_t * const scratch_p,
                                               fbe_sector_t * const pblk_p,
                                               const fbe_u32_t parity_index,
                                               const fbe_u16_t rebuild_pos);

 fbe_status_t fbe_xor_eval_reconstruct_1(fbe_xor_scratch_r6_t * const scratch_p,
                                      const fbe_u16_t ppos[],
                                      fbe_xor_error_t * const eboard_p,
                                      const fbe_u16_t bitkey[],
                                      fbe_u16_t rebuild_pos[],
                                      fbe_bool_t *eval_result_p);

 fbe_status_t fbe_xor_eval_reconstruct_2(fbe_xor_scratch_r6_t * const scratch_p,
                                      const fbe_u16_t i,
                                      const fbe_u16_t j,
                                      fbe_xor_error_t * const eboard_p,
                                      const fbe_u16_t bitkey[],
                                      const fbe_u16_t rpos[],
                                      const fbe_u16_t init_rpos[],
                                      const fbe_u16_t ppos[],
                                      fbe_bool_t *reconstruct_result_p);

fbe_u32_t fbe_xor_calc_data_syndrome(fbe_xor_scratch_r6_t * const scratch_p, 
                                     const fbe_u32_t col_index, 
                                     fbe_sector_t * const sector_p );

fbe_u32_t fbe_xor_calc_parity_syndrome(fbe_xor_scratch_r6_t * const scratch_p, 
                                       const fbe_xor_r6_parity_positions_t parity_index,
                                       fbe_sector_t * const sector_p);


void fbe_xor_raid6_correct_all(fbe_xor_error_t * eboard_p);

/* xor_raid6_reconstruct_2.c
 */
 fbe_status_t fbe_xor_rbld_data_stamps_r6(fbe_xor_scratch_r6_t * const scratch_p, 
                                       fbe_xor_error_t * const eboard_p,
                                       fbe_sector_t * const sector_p[],
                                       const fbe_u16_t bitkey[], 
                                       const fbe_u16_t parity_bitmap,
                                       const fbe_u16_t width,
                                       const fbe_u16_t rpos[],
                                       const fbe_u16_t ppos[],
                                       const fbe_bool_t b_reconst_1,
                                       fbe_bool_t * const error_result_p,
                                       fbe_bool_t * const needs_inv_p);

/* xor_raid6_reconstruct_parity.c
 */
fbe_status_t fbe_xor_calc_csum_and_reconstruct_parity(fbe_xor_scratch_r6_t * const scratch_p,
                                             fbe_sector_t * const data_blk_p,
                                             const fbe_u32_t column_index,
                                             const fbe_u16_t * rebuild_pos,
                                             const fbe_u16_t parity_bitmap,
                                             fbe_u32_t *uncooked_csum_p);

fbe_status_t fbe_xor_eval_reconstruct_parity(fbe_xor_scratch_r6_t * const scratch_p,
                                 const fbe_u16_t parity_pos[],
                                 fbe_xor_error_t * const eboard_p,
                                 const fbe_u16_t bitkey[],
                                 const fbe_u16_t rebuild_pos[],
                                 fbe_sector_t * const sector_p[],
                                 fbe_bool_t *reconstruct_result_p);

/* xor_raid6_syndrome_verify.c
 */
 fbe_status_t fbe_xor_eval_verify(fbe_xor_scratch_r6_t * const scratch_p,
                     fbe_xor_error_t * const eboard_p,
                     const fbe_u16_t bitkey[],
                     fbe_u16_t data_pos[],
                     const fbe_u16_t parity_pos[],
                     const fbe_u16_t width,
                     fbe_bool_t *verify_result_p);

/* xor_raid6_special_case.c
 */
 fbe_status_t fbe_xor_raid6_handle_zeroed_state(fbe_xor_scratch_r6_t * const scratch_p,
                                   fbe_sector_t * const sector_p[],
                                   const fbe_u16_t bitkey[],
                                   const fbe_u16_t width,
                                   fbe_xor_error_t *const eboard_p,
                                   const fbe_u16_t parity_pos[],
                                   fbe_u16_t rebuild_pos[],
                                   const fbe_lba_t seed,
                                   fbe_bool_t *zero_result_p);

/* xor_raid6_util.c
 */
fbe_status_t fbe_xor_rbld_parity_stamps_r6(fbe_xor_scratch_r6_t * const scratch_p, 
                                           fbe_xor_error_t *const eboard_p,
                                           fbe_sector_t *const sector_p[],
                                           const fbe_u16_t bitkey[],
                                           const fbe_u16_t width,
                                           const fbe_u16_t rpos[],
                                           const fbe_u16_t ppos[],
                                           const fbe_lba_t seed,
                                           const fbe_bool_t check_parity_stamps);

fbe_status_t fbe_xor_eval_parity_invalidate_r6(fbe_xor_scratch_r6_t * const scratch_p,
                                               const fbe_u16_t media_err_bitmap,
                                               fbe_xor_error_t * const eboard_p,
                                               fbe_sector_t * const sector_p[],
                                               const fbe_u16_t bitkey[],
                                               const fbe_u16_t width,
                                               const fbe_lba_t seed,
                                               const fbe_u16_t parity_pos[],
                                               const fbe_u16_t rebuild_pos[],
                                               const fbe_u16_t initial_rebuild_pos[],
                                               const fbe_u16_t data_position[],
                                               const fbe_bool_t inv_parity_crc_error,
                                               fbe_u16_t * const bimask_p);

static fbe_status_t fbe_xor_reconstruct_parity_r6(fbe_sector_t * const sector_p[],
                               const fbe_u16_t bitkey[],
                               const fbe_u16_t width,
                               const fbe_lba_t seed,
                               const fbe_u16_t parity_pos[],
                               const fbe_u16_t data_position[] );

fbe_status_t fbe_xor_sector_history_trace_vector_r6(const fbe_lba_t lba, 
                                        const fbe_sector_t * const sector_p[],
                                        const fbe_u16_t pos_bitmask[],
                                        const fbe_u16_t width,
                                        const fbe_u16_t ppos[],
                                        const fbe_object_id_t raid_group_object_id,
                                        const fbe_lba_t raid_group_offset, 
                                        char *error_string_p,
                                        const fbe_sector_trace_error_level_t error_level,
                                        const fbe_sector_trace_error_flags_t error_flag );

fbe_status_t fbe_xor_init_scratch_r6(fbe_xor_scratch_r6_t *scratch_p,
                                     fbe_xor_r6_scratch_data_t *scratch_data_p,
                                     fbe_sector_t * sector_p[],
                                     fbe_u16_t bitkey[],
                                     fbe_u16_t width,
                                     fbe_lba_t seed,
                                     fbe_u16_t parity_bitmap,
                                     fbe_u16_t ppos[],
                                     fbe_u16_t rpos[],
                                     fbe_xor_error_t * eboard_p,
                                     fbe_u16_t initial_rebuild_position[]);

fbe_status_t fbe_xor_reset_rebuild_positions_r6(fbe_xor_scratch_r6_t * scratch_p,
                                    fbe_sector_t * sector_p[],
                                    fbe_u16_t bitkey[],
                                    fbe_u16_t width,
                                    fbe_u16_t parity_pos[],
                                    fbe_u16_t rebuild_pos[],
                                    fbe_u16_t data_position[]);

fbe_status_t fbe_xor_eval_data_sector_r6(fbe_xor_scratch_r6_t * scratch_p,
                             fbe_xor_error_t * eboard_p,
                             fbe_sector_t * dblk_p,
                             fbe_u16_t dkey,
                             fbe_u32_t data_position,
                             fbe_u16_t rpos[],
                             fbe_u16_t parity_bitmap,
                             fbe_bool_t * const error_result_p);

fbe_status_t fbe_xor_eval_parity_sector_r6(fbe_xor_scratch_r6_t * scratch_p,
                               fbe_xor_error_t * eboard_p,
                               fbe_sector_t * pblk_p,
                               fbe_u16_t pkey,
                               fbe_u16_t width,
                               fbe_xor_r6_parity_positions_t pindex,
                               fbe_u16_t parity_bitmap,
                               fbe_u16_t rpos[],
                               fbe_bool_t * const error_result_p);


/* xor_raid6_eval_parity_unit.c
 */
fbe_status_t fbe_xor_eval_parity_unit_r6(fbe_xor_error_t * eboard_p,
                                fbe_sector_t * sector_p[],
                                fbe_u16_t bitkey[],
                                fbe_u16_t width,
                                fbe_u16_t ppos[],
                                fbe_u16_t rpos[],
                                fbe_lba_t seed,
                                fbe_bool_t b_final_recovery_attempt,
                                fbe_u16_t data_position[],
                                fbe_xor_option_t option,
                                fbe_u16_t * invalid_bitmask_p);

#endif /*FBE_XOR_RAID6_UTIL_H*/

/* end xor_raid6_util.h */
