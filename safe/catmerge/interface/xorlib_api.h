/***************************************************************************
 *	Copyright (C) EMC Corporation 2001
 *	All rights reserved.
 *	Licensed material -- property of EMC Corporation
 ***************************************************************************/
#ifndef XORLIB_API_H
#define	XORLIB_API_H

/*! @note Previously we would include `core_config.h' which would include
 *        `environment.h' which unfortunately defined `K10_ENV'.
 */

#include "csx_ext.h"

/*
 * wrapper if called from C++
 */
#if defined(__cplusplus)
extern "C"
{
#endif


/* Literals
 */
#define XORLIB_SEED_UNREFERENCED_VALUE  ((unsigned int)-1)

    /* The number of 32 bit words per symbol.
 */
#define XORLIB_WORDS_PER_SYMBOL (8)

/* The m value for our implementation of the EVENODD algortihm.  Per the
 * algorithm m must be prime.  This value must not be changed.
 */
#define XORLIB_EVENODD_M_VALUE (17)

/* This macro returns value MOD EVENODD_M_VALUE unless that is equal to 
 * EVENODD_M_VALUE - 1 then the default_return is returned.  This is used
 * to determine if the the value is the index of the imaginary row of 
 * zeroes in the EVENODD matrix.
 */
#define XORLIB_MOD_VALUE_M(m_value, m_default_return) \
    ((((m_value) % XORLIB_EVENODD_M_VALUE) == (XORLIB_EVENODD_M_VALUE - 1)) \
    ? (m_default_return) \
    : ((m_value) % XORLIB_EVENODD_M_VALUE))

/* As an attempt to reduce processor stalls due to branching,
 * we'll create "blocks" of repeated instructions. To accommodate
 * the Raid3-style checksum, the basic unit of repetition will
 * be eight (8) expressions.
 */
#define XORLIB_REPEAT8(m_expr) \
(m_expr, \
 m_expr, \
 m_expr, \
 m_expr, \
 m_expr, \
 m_expr, \
 m_expr, \
 m_expr)

#define XORLIB_REPEAT_SCALE 2

/* Calculate the number of times the expression
 * is actually repeated.
 */
#define XORLIB_REPEAT_CNT ((XORLIB_REPEAT_SCALE)*8)

/* This is the repeat count for XORLIB_PASSES16, given the WORDS_PER_BLOCK
 * constant it will loop over the whole block in 16 iterations.
 */
#define XORLIB_REPEAT16_CNT 8

/* Here we define how many times the basic eight-expression
 * block will be scaled. To increase/decrease the scaling
 * factor, just insert/delete invocations of XORLIB_REPEAT8(),
 * and adjust the value of XORLIB_REPEAT_SCALE to be the
 * resulting number of invocations.
 */
#define XORLIB_REPEAT(m_expr) \
XORLIB_REPEAT8(m_expr), \
XORLIB_REPEAT8(m_expr)

#define XORLIB_PASSES(m_cnt,m_reps) \
for((m_cnt) = ((m_reps) / XORLIB_REPEAT_CNT); 0 < (m_cnt)--; )

/* Given the WORDS_PER_BLOCK constant as the m_reps value this macro it will
 * loop over the whole block in 16 iterations.
 */
#define XORLIB_PASSES16(m_cnt,m_reps) \
for((m_cnt) = 0; (m_cnt) < ((m_reps) / XORLIB_REPEAT16_CNT); (m_cnt)++ )

/***************************************************
 * xorlib_sector_invalid_reason_t
 *
 * This enumeration identifies the values
 * for the 'who' field in the invalid sector.
 *
 * In the enum below, there are "reserved"
 * unused values because this structure is historical
 * and these "unused" values were used for other
 * reasons in the past which are no longer in use.
 *
 * Reasons should never be re-used in the structure below.
 * but new reasons can be added at the end of the enum.
 *
 ***************************************************/

typedef enum xorlib_sector_invalid_reason_e
{
    /* First valid invalidate reason
     */
    XORLIB_SECTOR_INVALID_REASON_FIRST  = 0x00,

    /* `Unknown' - Simply means there is no specific reason for the invalidation.
     */
    XORLIB_SECTOR_INVALID_REASON_UNKNOWN = XORLIB_SECTOR_INVALID_REASON_FIRST,

    /*! INV_DH_WR_BAD_CRC is used by the RAID when a block is intentionally 
     *  invalidated due to a `Corrupt CRC' request.
     *  
     *  @note MCR only uses the `Corrupt CRC' for error injection.  This crc
     *        error is treated differently than `Data Lost' since `Corrupt CRC'
     *        is an unexpected CRC error and is thus treated as such.
     */
    XORLIB_SECTOR_INVALID_REASON_RAID_CORRUPT_CRC = 0x01,

    /* XORLIB_SECTOR_INVALID_REASON_DATA_LOST -  The user data for this sector was
     * no longer accessible so it has been marked `data lost'.
     */
    XORLIB_SECTOR_INVALID_REASON_DATA_LOST = 0x02,

    /* This is used by the DH
     * to invalidate a sector after remap. Formerly INV_DH_SOFT_MEDIA. 
     */
    XORLIB_SECTOR_INVALID_REASON_DH_INVALIDATED = 0x03, 

    /*! @todo Need a reason for simulating `data lost'
     */
    XORLIB_SECTOR_INVALID_REASON_UNUSED_2 = 0x04,
    XORLIB_SECTOR_INVALID_REASON_UNUSED_3 = 0x05,
    XORLIB_SECTOR_INVALID_REASON_UNUSED_4 = 0x06,

    /* Used by RAID to invalidate a block when it has been lost due to errors on other
     * drives.  Formerly INV_VERIFY . 
     */
    XORLIB_SECTOR_INVALID_REASON_VERIFY = 0x07,
    
    /* INV_RAID_CORRUPT_DATA is used by RAID to invalidate just a data block
     * due to an IORB_OP_CORRUPT_DATA command. Formerly 
     * FBE_SECTOR_INVALID_REASON_CORRUPT_DATA. 
     * This is treated by raid/cache as a corrupt block. 
     */
    XORLIB_SECTOR_INVALID_REASON_CORRUPT_DATA = 0x08,

    /* INV_RAID_COH is used to corrupt the data but generate correct CRC.
     *      This is only for debug and should not be observed in the field.
     *      Formerly INV_RAID_COH.
     */
    XORLIB_SECTOR_INVALID_REASON_RAID_COH = 0x09,

    /* Used by Provision Drive to invalidate block(s) after remap. 
     */
    XORLIB_SECTOR_INVALID_REASON_PVD_REMAP = 0x0a,

    /* This is used by the COPY
     * to invalidate a sector after media error. 
     */
    XORLIB_SECTOR_INVALID_REASON_COPY_INVALIDATED = 0x0b, 

    /* This is used by PVD to invalidate blocks if metadata is invalid
     */
    XORLIB_SECTOR_INVALID_REASON_PVD_METADATA_INVALID = 0x0c,

    /* This is a block that was invalidated due to an uncorrectable, 
     * which was induced by an intentionally invalidated block. 
     * This pattern should not be treated as a invalidated block, but 
     * should be treated as a correctly invalidated block. 
     */
    XORLIB_SECTOR_INVALID_REASON_INVALIDATED_DATA_LOST = 0x0d,

    /*!**************************************************************************
     * @note Add new values here AND if the sector needs to be reconstructed 
     *       add it to XORLIB_SECTOR_INVALID_REASON_REQUIRES_RECONSTRUCT below!
     ****************************************************************************/
    XORLIB_SECTOR_INVALID_REASON_NEXT,

    /*! Max allowed number of revisions. 
     * Essentially allows twice the number we originally started with. 
     */
    XORLIB_SECTOR_INVALID_REASON_MAX = 0x20,
}
xorlib_sector_invalid_reason_t;

/*!***************************************************************************
 * XORLIB_SECTOR_INVALID_REASON_REQUIRES_RECONSTRUCT
 *****************************************************************************
 *
 * If a sector was invalidated for any of the reason below the data for that
 * sector is lost and it must be reconstructed.  It should be treated as an
 * uncorrectable CRC error as opposed to a purposefully invalidated sector.
 *
 *****************************************************************************/
#define XORLIB_SECTOR_INVALID_REASON_REQUIRES_RECONSTRUCT(reason)   (((reason == XORLIB_SECTOR_INVALID_REASON_CORRUPT_DATA)         ||     \
                                                                      (reason == XORLIB_SECTOR_INVALID_REASON_COPY_INVALIDATED)     ||     \
                                                                      (reason == XORLIB_SECTOR_INVALID_REASON_PVD_METADATA_INVALID) ||     \
                                                                      (reason == XORLIB_SECTOR_INVALID_REASON_DH_INVALIDATED)          ) ? \
                                                                      TRUE : FALSE)

/*!****************************************************************************
 *     xorlib_sector_invalid_who_t
 * 
 *****************************************************************************
 * 
 * This enumeration identifies the values for the 'who' field in the invalid
 * sector.
 *
 * The `who' field is used for debugging purposes only.  That is there should
 * be no validation done on the `who' field;
 *
 *****************************************************************************/
typedef enum xorlib_sector_invalid_who_e
{
    /* XORLIB_SECTOR_INVALID_WHO_UNKNOWN. Simply means no specific value
     */
    XORLIB_SECTOR_INVALID_WHO_UNKNOWN = -1,

    /* XORLIB_SECTOR_INVALID_WHO_SELF.
     */
    XORLIB_SECTOR_INVALID_WHO_SELF    = 0,

    /* XORLIB_SECTOR_INVALID_WHO_BAD_CRC. For debug purposes only
     */
    XORLIB_SECTOR_INVALID_WHO_BAD_CRC       = (int)0xBADCBADC,   /* Bad CRC*/

    /* XORLIB_SECTOR_INVALID_WHO_CLIENT.  A client of RAID invalidated this sector.
     */
    XORLIB_SECTOR_INVALID_WHO_CLIENT        = 0x424C4945,   /* "CLIE"nt */

    /* XORLIB_SECTOR_INVALID_WHO_RAID. RAID invalidated this request.
     */
    XORLIB_SECTOR_INVALID_WHO_RAID           = 0x52414944,  /* "RAID" */

    /* XORLIB_SECTOR_INVALID_WHO_ERROR_INJECTION. Logical Error Injection Service invalidated this request.
     */
    XORLIB_SECTOR_INVALID_WHO_ERROR_INJECTION = 0x4C454953, /* "LEIS" Logical Error Injection Service*/

    /* XORLIB_SECTOR_INVALID_WHO_PVD. PVD invalidated this request.
     */
    XORLIB_SECTOR_INVALID_WHO_PVD = 0x50564449, /* "PVDI" - PVD Invalidated */

}
xorlib_sector_invalid_who_t;

/* Structures
 */
/*!*******************************************************************
 * @struct  xorlib_system_time_t
 *********************************************************************
 * 
 * @brief   System time that invalidate occurred at.
 *
 *********************************************************************/
typedef struct xorlib_system_time_s
{
  unsigned short year;
  unsigned short month;
  unsigned short weekday;
  unsigned short day;
  unsigned short hour;
  unsigned short minute;
  unsigned short second;
  unsigned short milliseconds;
} xorlib_system_time_t;

/* Structure of size 32 bytes so the pointer arithmetic in xor_raid6_util.c
 * is more readable.
 */
typedef struct XORLIB_SYMBOL_SIZE
{
    unsigned int data_symbol[XORLIB_WORDS_PER_SYMBOL];
}
XORLIB_SYMBOL_SIZE;

typedef enum XORLIB_CSUM_CMP
{
    XORLIB_CSUM_SAME_DATA = 0,
    XORLIB_CSUM_DIFF_DATA = 1
}
XORLIB_CSUM_CMP;

/***************************************************************************
 *	PROTOTYPE DEFINITIONS
 ***************************************************************************/
void xorlib_select_asm(void);

/***************************************************************************
 *	XORLIB Routines available for clients.
 ***************************************************************************/
/* Function pointers used to abstract implementation.
 */
extern void    (*xorlib_copy_data)(const unsigned int *srcptr, unsigned int *tgtptr);
extern unsigned int (*xorlib_calc_csum) (const unsigned int * srcptr);
extern unsigned int (*xorlib_calc_csum_seq) (const unsigned int * srcptr);
extern unsigned short  (*xorlib_cook_csum) (unsigned int rawcsum, unsigned int seed);
extern unsigned short  (*xorlib_compute_chksum) (unsigned int * srcptr, unsigned int seed);
extern unsigned int (*xorlib_calc_csum_and_cpy) (const unsigned int * srcptr, unsigned int * tgtptr);
extern unsigned int (*xorlib_calc_csum_and_cpy_seq) (const unsigned int * srcptr, unsigned int * tgtptr);
extern unsigned int (*xorlib_calc_csum_and_cpy_to_temp) (const unsigned int *srcptr, unsigned int *tempptr);

extern unsigned int (*xorlib_calc_csum_and_cmp) (const unsigned int * srcptr, const unsigned int * tgtptr, XORLIB_CSUM_CMP * cmpptr);
extern unsigned int (*xorlib_calc_csum_and_xor) (const unsigned int * srcptr, unsigned int * tgtptr);
extern unsigned int (*xorlib_calc_csum_and_xor_to_temp) (const unsigned int *srcptr, unsigned int *tempptr);
extern unsigned int (*xorlib_calc_csum_and_xor_with_temp_and_cpy) (const unsigned int *srcptr, const unsigned int *tempptr, unsigned int *trgptr);
extern void    (*xorlib_468_calc_csum_and_update_parity) (const unsigned int * old_dblk, const unsigned int * new_dblk, unsigned int * pblk, unsigned int * csum);
extern unsigned int (*xorlib_calc_csum_and_cpy_vault) (const unsigned int * srcptr, unsigned int * tgtptr1, unsigned int * tgtptr2);
extern unsigned int (*xorlib_calc_csum_and_xor_vault) (const unsigned int * srcptr, unsigned int * tgtptr1, unsigned int * tgtptr2);
extern unsigned short  (*xorlib_generate_lba_stamp) (unsigned __int64 lba, unsigned __int64 offset);
extern int    (*xorlib_is_valid_lba_stamp) (unsigned short *lba_stamp, unsigned __int64 lba, unsigned __int64 offset);

extern unsigned int (*xorlib_calc_csum_and_init_r6_parity) (const unsigned int * src_ptr, unsigned int * tgt_row_ptr, unsigned int * tgt_diagonal_ptr, const unsigned int column_index);
extern unsigned int (*xorlib_calc_csum_and_update_r6_parity) (const unsigned int * src_ptr, unsigned int * tgt_row_ptr, unsigned int * tgt_diagonal_ptr, const unsigned int column_index);
extern unsigned int (*xorlib_calc_csum_and_init_r6_parity_to_temp)(const unsigned int * src_ptr, unsigned int * tgt_temp_row_ptr, unsigned int * tgt_temp_diagonal_ptr, 
                                                                 const unsigned int column_index);
extern unsigned int (*xorlib_calc_csum_and_update_r6_parity_to_temp)(const unsigned int * src_ptr, unsigned int * tgt_temp_row_ptr, unsigned int * tgt_temp_diagonal_ptr, 
                                                                   const unsigned int column_index);
extern unsigned int (*xorlib_calc_csum_and_update_r6_parity_to_temp_and_cpy)(const unsigned int * src_ptr, const unsigned int * tgt_temp_row_ptr, const unsigned int * tgt_temp_diagonal_ptr, 
                                                                           const unsigned int column_index, unsigned int * tgt_row_ptr, unsigned int * tgt_diagonal_ptr);  

extern void xorlib_invalidate_checksum(void *const data_p);
extern long xorlib_fill_invalid_sector(void *data_p,
                                       unsigned __int64 invalidate_lba,
                                       xorlib_sector_invalid_reason_t reason,
                                       xorlib_sector_invalid_who_t who,
                                       const xorlib_system_time_t *const system_time_p);
extern long xorlib_fill_update_invalid_sector(void *data_p,
                                              unsigned __int64 invalidate_lba,
                                              xorlib_sector_invalid_reason_t reason,
                                              xorlib_sector_invalid_who_t who,
                                              const xorlib_system_time_t *const system_time_p);
extern int xorlib_is_sector_invalidated( const void * const blk_p,
                                          xorlib_sector_invalid_reason_t * const reason_p,
                                          const unsigned __int64 seed,
                                          const int b_validate_seed);
extern int xorlib_get_seed_from_invalidated_sector( const void *const blk_p,
                                                     unsigned __int64 *const seed_p);
int xorlib_get_who_from_invalidated_sector( const void * const blk_p,
                                            xorlib_sector_invalid_who_t *who_p);
extern int xorlib_get_test_word_from_invalidated_sector( const void *const blk_p,
                                                          unsigned int *const test_word_p);
extern int xorlib_set_test_word_in_invalidated_sector( void *const blk_p,
                                                        unsigned int new_test_word);
extern int xorlib_is_data_invalid_with_good_crc(void *in_sector_p);
extern int xorlib_is_sector_data_invalidated( const void *const blk_p );
extern int xorlib_is_sector_invalidated_for_data_lost( const void *const blk_p );
extern int xorlib_rebuild_invalidated_sector( void * const blk_p );
extern int xorlib_is_sector_with_crc_error_retryable(const void *const blk_p);
extern int xorlib_does_sector_need_to_be_marked_unretryable(const void *const sector_p,
                                                            xorlib_sector_invalid_reason_t *const updated_invalid_reason_p);
extern int xorlib_should_invalidated_sector_be_marked_uncorrectable(xorlib_sector_invalid_reason_t const invalid_reason);

#if defined(__cplusplus)
} /* extern "C" */
#endif

#endif /* XORLIB_API_H */
