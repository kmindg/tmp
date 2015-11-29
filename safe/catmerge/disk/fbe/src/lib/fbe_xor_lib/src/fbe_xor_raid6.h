/****************************************************************
 *  Copyright (C)  EMC Corporation 2006
 *  All rights reserved.
 *  Licensed material - property of EMC Corporation.
 ****************************************************************/

/*!**************************************************************************
 * @file fbe_xor_raid6.h
 ***************************************************************************
 *
 * @brief
 *  This file contains support for the functions that implement the EVENODD
 *  algorithm which is used for RAID 6 functionality.
 *
 * @notes
 *
 * @author
 *
 * 03/16/06 - Created. RCL
 *
 **************************************************************************/

#ifndef FBE_XOR_RAID6_H
#define FBE_XOR_RAID6_H

#include "fbe_xor_private.h"

/* RAID 6 defines.
 */
#define FBE_XOR_R6_SYMBOLS_PER_SECTOR 16

/* The m value for our implementation of the EVENODD algortihm.  Per the
 * algorithm m must be prime.  This value must not be changed.
 */
#define FBE_XOR_EVENODD_M_VALUE (XORLIB_EVENODD_M_VALUE)

/* The MSB and LSB of the 16 bit symbols used for the EVENODD algortihm.
 */
#define FBE_XOR_MOST_SIG_BIT 0x10000
#define FBE_XOR_LEAST_SIG_BIT 0x0001

/* The number of 32 bit words per symbol.
 */
#define FBE_XOR_WORDS_PER_SYMBOL (XORLIB_WORDS_PER_SYMBOL)

/* This macro returns value MOD EVENODD_M_VALUE unless that is equal to 
 * EVENODD_M_VALUE - 1 then the default_return is returned.  This is used
 * to determine if the the value is the index of the imaginary row of 
 * zeroes in the EVENODD matrix.
 */
#define FBE_XOR_MOD_VALUE_M(m_value, m_default_return) \
    ((((m_value) % FBE_XOR_EVENODD_M_VALUE) == (FBE_XOR_EVENODD_M_VALUE - 1)) \
    ? (m_default_return) \
    : ((m_value) % FBE_XOR_EVENODD_M_VALUE))

/* Cyclicly shifts a 16 bit symbol value X.  A cyclic shift will shift
 * the bits one space towards the most significant bit and replace the
 * least significant bit with the old MSB.
 */
#define FBE_XOR_CYCLIC_SHIFT(m_X) \
    ((m_X & FBE_XOR_LEAST_SIG_BIT) \
    ? ((m_X = ( (m_X >> 1) ^ FBE_XOR_MOST_SIG_BIT) ) & 0x1fff) \
    : ((m_X >>= 1) & 0x1fff) )

/* Tests if the bit in the value of position is set in the data word.
 */
#define FBE_XOR_IS_BITSET(m_data,m_position) ( (m_data >> (m_position) ) & 0x1)

/* Set the position of m_data to be 1.
 */
#define FBE_XOR_SET_BIT(m_data,m_position) ( (m_data) | (1<<(m_position)) )

/* Cyclically shifts the data symbol based on the position in the parity
 * matrix.  This takes into account a special imaginary row used by the 
 * EVENODD algortihm.  Based on what column is being shifted the position
 * of the S value is known.  The bits above the S value get shifted down
 * to the bottom of the column.  Anything below the S value gets shifted
 * to the top of the column.
 */
#define FBE_XOR_EVENODD_DIAGONAL_MANGLE(m_data,m_position) \
    ((m_position == 0) \
    ? (m_data) \
    : ( ((m_data) >> (m_position)) | ((m_data) << (FBE_XOR_EVENODD_M_VALUE - m_position))) )

/* Cyclically shifts the data symbol based on the position in the parity
 * matrix.  This takes into account a special imaginary row used by the 
 * EVENODD algortihm.  Based on what column is being shifted the position
 * of the S value is known.  The bits above the S value get shifted down
 * to the bottom of the column.  Anything below the S value gets shifted
 * to the top of the column.  This is special for the syndrome calculation
 * since there is one extra bit.
 */
#define FBE_XOR_EVENODD_SYNDROME_DIAGONAL_MANGLE(m_data,m_position) \
    ((m_position == 0) \
    ? ( (m_data) << 1 ) \
    : ( ((m_data) >> (m_position - 1)) | ((m_data) << (FBE_XOR_EVENODD_M_VALUE - m_position + 1))) )

/* Cyclically shifts the data symbol based on the position in the parity
 * matrix and the lost disk.  This takes into account a special imaginary
 * row used by the EVENODD algortihm.  Based on what column is being shifted
 * and the column being rebuilt we know how to line up the bits to reconstruct
 * the old value.  The bits above the last value of the rebuild column get 
 * shifted down to the bottom of the column.  Anything below the last value 
 * gets shifted to the top of the column.
 */
#define FBE_XOR_EVENODD_DIAGONAL_MANGLE_RECONSTRUCT(m_data,m_position,m_rebuild_pos) \
    ((m_position == m_rebuild_pos) \
    ? (m_data) \
    : ( ((m_data) >> ( (m_position - m_rebuild_pos + FBE_XOR_EVENODD_M_VALUE) % FBE_XOR_EVENODD_M_VALUE) ) | \
      ( (m_data) << ( (m_rebuild_pos - m_position + FBE_XOR_EVENODD_M_VALUE) % FBE_XOR_EVENODD_M_VALUE ) ) ) )

/* Cyclically shifts the data symbol based on the position of the lost disk.  
 * This takes into account a special imaginary row used by the EVENODD 
 * algortihm.  Based on what column is being rebuilt the position of the 
 * S value is known.  The bits above the S value get shifted down
 * to the bottom of the column.  Anything below the S value gets shifted
 * to the top of the column.
 */
#define FBE_XOR_EVENODD_DIAGONAL_RECONSTRUCT_PARITY(m_data,m_position) \
    ((m_position == 0) \
    ? (m_data) \
    : ( ((m_data) << (m_position)) | ((m_data) >> (FBE_XOR_EVENODD_M_VALUE - m_position))) )

/* START FBE_XOR_SCRATCH_STATE MACROS
 * The following macros are used to determine the state of the scratch in 
 * the RAID 6 version of eval_parity_unit.  Some macros parameters are not used
 * within the macro.  This was done on purpose to make all macros uniform.
 */

/* This macro determines if the raid6 scratch values are set so that a verify
 * should be performed.
 * Conditions where it should return FBE_TRUE are:
 *   No errors
 */
#define FBE_XOR_IS_SCRATCH_VERIFY_STATE(m_scratch, m_parity_positions, m_ibits) \
    ((m_scratch->fatal_cnt == 0) \
     ? FBE_TRUE \
     : FBE_FALSE)

/* This macro determines if the raid6 scratch values are set so that a one data
 * disk reconstruction should be performed.
 * Conditions where it should return FBE_TRUE are:
 *   One error
 *   Two errors with one of them being the parity position
 */
#define FBE_XOR_IS_SCRATCH_RECONSTRUCT_1_STATE(m_scratch, m_parity_positions, m_ibits) \
    ((((m_scratch->fatal_cnt == 1) && (0 == ((m_parity_positions) & (m_ibits)) )) || \
      ((m_scratch->fatal_cnt == 2) && ((0 != ((m_parity_positions) & (m_ibits))) && ((m_parity_positions) != (m_ibits)) ))) \
      ? FBE_TRUE \
      : FBE_FALSE)

/* This macro determines if the raid6 scratch values are set so that a two data
 * disk reconstruction should be performed.
 * Conditions where it should return FBE_TRUE are:
 *   Two data disk errors 
 */
#define FBE_XOR_IS_SCRATCH_RECONSTRUCT_2_STATE(m_scratch, m_parity_positions, m_ibits) \
    (((m_scratch->fatal_cnt == 2) && ((0 == ((m_parity_positions) & (m_ibits))))) \
      ? FBE_TRUE \
      : FBE_FALSE)

/* This macro determines if the raid6 scratch values are set so that a two parity
 * disk reconstruction should be performed.
 * Conditions where it should return FBE_TRUE are:
 *   Two parity disk errors 
 *   One parity disk error
 */
#define FBE_XOR_IS_SCRATCH_RECONSTRUCT_P_STATE(m_scratch, m_parity_positions, m_ibits) \
    (((m_scratch->fatal_cnt == 2) && (( (m_parity_positions) == (m_ibits)))) || \
      ((m_scratch->fatal_cnt == 1) && ((0 != ((m_parity_positions) & (m_ibits))))) \
      ? FBE_TRUE \
      : FBE_FALSE)

/* END FBE_XOR_SCRATCH_STATE MACROS */

/* Structure of size 32 bytes so the pointer arithmetic in xor_raid6_util.c
 * is more readable.
 */
typedef struct fbe_xor_raid6_symbol_size_s
{
    fbe_u32_t data_symbol[FBE_XOR_WORDS_PER_SYMBOL];
}
fbe_xor_r6_symbol_size_t;

/* This enumeration holds the possible scratch states for a RAID 6 scratch 
 * buffer.
 */
typedef enum fbe_xor_scratch_r6_state_s
{
    FBE_XOR_SCRATCH_R6_STATE_VERIFY = 0,
    FBE_XOR_SCRATCH_R6_STATE_RECONSTRUCT_1 = 1,
    FBE_XOR_SCRATCH_R6_STATE_RECONSTRUCT_2 = 2,
    FBE_XOR_SCRATCH_R6_STATE_RECONSTRUCT_P = 3,
    FBE_XOR_SCRATCH_R6_STATE_DONE = 4,
    FBE_XOR_SCRATCH_R6_STATE_NOT_SET = 5
}
fbe_xor_scratch_r6_state_t;

/* This buffer holds the 17 symbol syndrome that is used in the evenodd
 * algorithm for RAID 6.
 */
typedef struct fbe_xor_r6_syndrome_s
{
    fbe_xor_r6_symbol_size_t syndrome_symbol[17];
}
fbe_xor_r6_syndrome_t;

/********************************************************************
 *
 * fbe_xor_scratch_r6_t
 *
 * This structure defines the memory required to carry out eval operations. 
 *
 *  row_syndrome  - Pointer to the row syndrome which is a block of memory
 *                  large enough so that all sections of the evenodd 
 *                  algorithm can be performed here.
 *  diag_syndrome - Pointer to the diagonal syndrome which is a block of memory
 *                  large enough so that all sections of the evenodd 
 *                  algorithm can be performed here.
 *  checksum_row_syndrome - Row syndrome calculated for the parity of checksums.
 *  checksum_diag_syndrome - Row syndrome calculated for the parity of checksums.
 *  fatal_blk[2]  - Sectors in strip detected as bad. 
 *  fatal_key     - Position of bad sector(s) in strip. 
 *  fatal_cnt     - Number of bad sectors in strip.
 *  ppos[]        - Array of parity positions. 
 *  running_csum  - Used to track the checksum of parity/rebuild data. 
 *  seed          - Used to in the checksum calculations. 
 *  xor_required  - Used to indicate that data must be XOR'ed out.
 *  initialized   - Indicates that this is a fill operation. 
 *  scratch_state - State of the eval_parity_unit_r6 funciton.
 *  s_value       - Holds the S value for this strip.
 *  sector_p      - Array of pointers to sectors for this strip
 *  pos_bitmask   - Position based array of bitmask associated with position
 *  s_value_for_checksum - Holds the S value for the parity of the checksums
 *                         for this strip.
 *  media_err_bitmap - Bitmap for the media errors in the strip we are working 
 *                     with.
 *  strip_verified - Flag that indicates whether strip has been through the
 *                   verify code path.  Needed to determine error recovery
 *                   for reconstruction.
 *  stamp_mismatch_pos - Used in reconstruct 1 -> reconstruct 2
 *                       case where there is one stamp error and one other error
 *                       If the reconstruct 2 fails we need to exclude
 *                       the stamp error since it might be due to a coherency.
 *                       We need to remove this position from the fatal so
 *                       that we do not invalidate in reconstruct 2.
 *  traced_pos_bitmap  - Bitmap of positions that we have saved data trace for 
 *  width          - The width of this strip
 * 
 ********************************************************************/

typedef struct fbe_xor_scratch_r6_s
{
    fbe_xor_r6_syndrome_t *row_syndrome;
    fbe_xor_r6_syndrome_t *diag_syndrome;
    fbe_u32_t checksum_row_syndrome;
    fbe_u32_t checksum_diag_syndrome;
    fbe_sector_t *fatal_blk[2];
    fbe_u16_t fatal_key;
    fbe_u16_t fatal_cnt;
    fbe_u16_t ppos[FBE_XOR_NUM_PARITY_POS];
    fbe_u32_t running_csum;  //may be able to remove this 
    fbe_lba_t seed;
    fbe_lba_t offset;
    fbe_bool_t xor_required;
    fbe_bool_t initialized;
    fbe_xor_scratch_r6_state_t scratch_state;
    fbe_xor_r6_symbol_size_t *s_value;
    fbe_sector_t *sector_p[FBE_XOR_MAX_FRUS];
    fbe_u16_t pos_bitmask[FBE_XOR_MAX_FRUS];
    fbe_u16_t s_value_for_checksum;
    fbe_u16_t media_err_bitmap;
    fbe_u16_t retry_err_bitmap;
    fbe_bool_t strip_verified;
    fbe_u16_t stamp_mismatch_pos;
    fbe_u16_t traced_pos_bitmap;
    fbe_u16_t width;
    struct fbe_xor_r6_scratch_data_s *scratch_data_p;
}
fbe_xor_scratch_r6_t;

typedef struct fbe_xor_r6_scratch_data_s
{
    /* This global variable is the 2 scratch buffer for the syndromes used in the
     * evenodd algortihm for RAID 6.  They are globally defined because we don't
     * want to keep allocating such a large chunk of memory.
     */
    fbe_xor_r6_syndrome_t scratch_syndromes[2];

    /* This global variable is a scratch buffer for the S value used in the
     * evenodd algorithm for RAID 6.
     */
    fbe_xor_r6_symbol_size_t scratch_S_value;

    /* This global variable is to the scratch buffer used in the evenodd algorithm 
     * for RAID 6.
     */
    fbe_xor_scratch_r6_t raid6_scratch;
}
fbe_xor_r6_scratch_data_t;

/* This structure holds the global variables needed for eval_parity_unit_r6
 */

typedef struct fbe_xor_globals_r6_s
{
    /* These globals contain the "initialized" Parity of Checksums for row & diag
     * parity for various numbers of data drives.
     */
    fbe_u16_t xor_R6_init_row_poc[FBE_XOR_MAX_FRUS - 2];
    fbe_u16_t xor_R6_init_diag_poc[FBE_XOR_MAX_FRUS - 2];
}
fbe_xor_globals_r6_t;



/* fbe_xor_raid6_util.c
 */

void fbe_xor_calc_parity_of_csum_init(const fbe_u16_t * const src_ptr,
                                  fbe_u16_t * const tgt_row_ptr,
                                  fbe_u16_t * const tgt_diagonal_ptr,
                                  const fbe_u32_t column_index);

void fbe_xor_calc_parity_of_csum_update(const fbe_u16_t * const src_ptr,
                                  fbe_u16_t * const tgt_row_ptr,
                                  fbe_u16_t * const tgt_diagonal_ptr,
                                  const fbe_u32_t column_index);

void fbe_xor_calc_parity_of_csum_update_and_cpy (const fbe_u16_t * const src_ptr,
                                                 const fbe_u16_t * const tgt_temp_row_ptr,
                                                 const fbe_u16_t * const tgt_temp_diagonal_ptr,
                                                 const fbe_u32_t column_index,
                                                 fbe_u16_t * const tgt_row_ptr,
                                                 fbe_u16_t * const tgt_diagonal_ptr);

#endif /* FBE_XOR_RAID6_H */ 

/* end fbe_xor_raid6.h */
