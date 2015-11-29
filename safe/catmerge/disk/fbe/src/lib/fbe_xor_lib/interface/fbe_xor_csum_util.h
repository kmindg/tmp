#ifndef FBE_XOR_CSUM_UTIL_H
#define	FBE_XOR_CSUM_UTIL_H

/***************************************************************************
 *	Copyright (C) EMC Corporation 2001-2010
 *	All rights reserved.
 *	Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_xor_csum_util.h
 ***************************************************************************
 *
 *	@brief
 *		This header file defines the interface to the xor library's
 *		low-level checksum library.
 *
 *	@note
 *		Raw checksums preserve the following property:
 *
 *			Given two sectors, A and B, the raw checksum of the
 *			Xor of A and B is equal to the Xor of A's and B's
 *			raw checksums, i.e.
 *
 *			XOR(RAW_CSUM(A),RAW_CSUM(B)) == RAW_CSUM(XOR(A,B))
 *
 *		Cooked checksums have no such guarentee, as they've been
 *		seeded, folded & rotated (spindled, mutilated...).
 *
 *		None of these functions will modify metadata in the fbe_sector_t,
 *		i.e. they will calculate the checksum, but will not store
 *		it in the fbe_sector_t.
 *
 *		In the XP_ARCH architecture, it is the caller's responsibility
 *			1) to migrate processing to the XOR processor, and 
 *			2) to invoke xor_flush_xxx() functions when necessary.
 *
 *
 ***************************************************************************/

/***************************************************************************
 *	INCLUDE FILES
 ***************************************************************************/

#include "fbe/fbe_types.h"

/***************************************************************************
 *	PREPROCESSOR CONSTANT/MACRO DEFINITIONS
 ***************************************************************************/

/* This is the constant seed used for "cooking"
 * checksums.
 *
 * NOTE: for debugging purposes, testproc generates sectors
 * that contain all the same data (ie. 128 words of 0x0d231400).
 * If all the data is the same, it all Xor's to 0, and then is
 * xor'd against the seed, so the result is the seed defined
 * below... but wait, we also shift that result left one bit and
 * fold it into a 16 bit value, so the seed below would become
 * 0x00015EEC (shift), and then 0x00005EED (fold).  Therefore,
 * with fixed seed checksumming, all of our sectors will have
 * the value 0x5EED ("SEED") in the lower 16 bits after
 * we finish checksumming.      - MJH
 */
#define FBE_XOR_CHECKSUM_SEED_CONST  0x0000AF76

#define MMX_DISABLED  0
#define MMX_SUPPORTED 1
#define MMX_UNKNOWN   2

/* As an attempt to reduce processor stalls due to branching,
 * we'll create "blocks" of repeated instructions. To accommodate
 * the Raid3-style checksum, the basic unit of repetition will
 * be eight (8) expressions.
 */
#define FBE_XOR_REPEAT8(m_expr) \
(m_expr, \
 m_expr, \
 m_expr, \
 m_expr, \
 m_expr, \
 m_expr, \
 m_expr, \
 m_expr)

/* Here we define how many times the basic eight-expression
 * block will be scaled. To increase/decrease the scaling
 * factor, just insert/delete invocations of FL_FBE_XOR_REPEAT8(),
 * and adjust the value of XOR_REPEAT_SCALE to be the
 * resulting number of invocations.
 */
#define FBE_XOR_REPEAT(m_expr) \
FBE_XOR_REPEAT8(m_expr), \
FBE_XOR_REPEAT8(m_expr)

#define FBE_XOR_REPEAT_SCALE 2

/* Calculate the number of times the expression
 * is actually repeated.
 */
#define FBE_XOR_REPEAT_CNT ((FBE_XOR_REPEAT_SCALE)*8)

/* This is the repeat count for FBE_XOR_PASSES16, given the WORDS_PER_BLOCK
 * constant it will loop over the whole block in 16 iterations.
 */
#define FBE_XOR_REPEAT16_CNT 8

#define FBE_XOR_PASSES(m_cnt,m_reps) \
for((m_cnt) = ((m_reps) / FBE_XOR_REPEAT_CNT); 0 < (m_cnt)--; )

/* Given the WORDS_PER_BLOCK constant as the m_reps value this macro it will
 * loop over the whole block in 16 iterations.
 */
#define FBE_XOR_PASSES16(m_cnt,m_reps) \
for((m_cnt) = 0; (m_cnt) < ((m_reps) / FBE_XOR_REPEAT16_CNT); (m_cnt)++ )

/* Return true if passed in address is not 16-byte aligned.
 */
#define FBE_XOR_IS_UNALIGNED_16(addr) \
((ULONG_PTR)addr & 0xF)

/* Return true if passed in address is 16-byte aligned.
 */
#define FBE_XOR_IS_ALIGNED_16(addr) (!FBE_XOR_IS_UNALIGNED_16(addr))

/***************************************************************************
 *	STRUCTURE DEFINITIONS
 ***************************************************************************/

typedef enum fbe_xor_csum_alg_e
{
    FBE_XOR_CSUM_ALG_INVALID = 0,
    FBE_XOR_CSUM_ALG_FULL_FIXED = 1,

    FBE_XOR_CSUM_ALG_LAST
}
fbe_xor_csum_alg_t;

typedef enum fbe_xor_comparison_result_e
{
    /* *** These match the values used in assembly code, do not change. ***
     */
    FBE_XOR_COMPARISON_RESULT_SAME_DATA = 0,
    FBE_XOR_COMPARISON_RESULT_DIFF_DATA = 1
}
fbe_xor_comparison_result_t;


/***************************************************************************
 *	EXTERNAL REFERENCE DEFINITIONS
 ***************************************************************************/

/* Function pointers "translating" functions
 * for low-level checksum functionality.
 */
extern fbe_u16_t
  (*fbe_xor_cook_csum) (fbe_u32_t rawcsum,
  fbe_u32_t seed);
extern fbe_u32_t
  (*fbe_xor_calc_csum) (const fbe_u32_t * srcptr);

extern fbe_u32_t
  (*fbe_xor_calc_csum_and_cmp) (const fbe_u32_t * srcptr,
                           const fbe_u32_t * tgtptr,
                           fbe_xor_comparison_result_t * cmpptr);
extern fbe_u32_t
  (*fbe_xor_calc_csum_and_cpy) (const fbe_u32_t * srcptr,
                           fbe_u32_t * tgtptr);
extern fbe_u32_t
  (*fbe_xor_calc_csum_and_cpy_sequential) (const fbe_u32_t * srcptr,
                                       fbe_u32_t * tgtptr);

extern fbe_u32_t
  (*fbe_xor_calc_csum_and_xor) (const fbe_u32_t * srcptr,
  fbe_u32_t * tgtptr);

extern fbe_u32_t
  (*fbe_xor_calc_csum_and_cpy_to_temp) (const fbe_u32_t *srcptr, fbe_u32_t *tempptr);

extern fbe_u32_t 
(*fbe_xor_calc_csum_and_xor_to_temp) (const fbe_u32_t *srcptr, fbe_u32_t *tempptr);

extern fbe_u32_t 
(*fbe_xor_calc_csum_and_xor_with_temp_and_cpy) (const fbe_u32_t *srcptr, const fbe_u32_t *tempptr, fbe_u32_t *trgptr);

extern void
(*fbe_xor_468_calc_csum_and_update_parity) (
    const fbe_u32_t * old_dblk, const fbe_u32_t * new_dblk, 
    fbe_u32_t * pblk, fbe_u32_t * csum);

extern fbe_u32_t
  (*fbe_xor_calc_csum_and_cpy_vault) (const fbe_u32_t * srcptr,
  fbe_u32_t * tgtptr1, fbe_u32_t * tgtptr2);

extern fbe_u32_t
  (*fbe_xor_calc_csum_and_xor_vault) (const fbe_u32_t * srcptr,
  fbe_u32_t * tgtptr1, fbe_u32_t * tgtptr2);

/***************************************** 
 * LBA Stamp defines  
 *****************************************/
fbe_bool_t fbe_xor_is_valid_lba_stamp(fbe_u16_t *lba_stamp_p, fbe_lba_t lba, fbe_lba_t offset);
fbe_u16_t  fbe_xor_generate_lba_stamp(fbe_lba_t lba, fbe_lba_t offset);


extern fbe_u16_t fbe_xor_compute_chksum(void *srcptr, fbe_u32_t seed);

/***************************************************************************
 *	PROTOTYPE DEFINITIONS
 ***************************************************************************/

/* fbe_xor_csum_util.c
 */
extern fbe_bool_t fbe_xor_is_supported_csum(fbe_xor_csum_alg_t csum_algorithm);

extern fbe_status_t fbe_xor_csum_init(void);

extern void fbe_xor_copy_data(const fbe_u32_t * srcptr, fbe_u32_t * tgtptr);

/* fbe_xor_raid6_csum_util.c 
 */
extern fbe_status_t fbe_xor_raid6_csum_init(void);

#endif /* FBE_XOR_CSUM_UTIL_H */
/*************************
 * end file fbe_xor_csum_util.h
 *************************/
