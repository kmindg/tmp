#ifndef FBE_XOR_PRIVATE_H
#define FBE_XOR_PRIVATE_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2000-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_xor_private.h
 ***************************************************************************
 *
 * @brief
 *   This header file contains private structures and definitions needed by the
 *   XOR library.
 *
 * NOTES:
 *
 * @revision
 *  07/13/2009 - Created. Rob Foley
 * 
 ***************************************************************************/

/*************************
 * INCLUDE FILES
 *************************/
#include "csx_ext.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_sg_element.h"
#include "fbe/fbe_xor_api.h"
#include "fbe/fbe_sector.h"
#include "fbe/fbe_trace_interface.h"
#include "fbe/fbe_sector_trace_interface.h"


/*********************************
 * EXTERNAL REFERENCE DEFINITIONS
 *********************************/

/*******************************
 * LITERAL DEFINITIONS
 *******************************/

/*!*******************************************************************
 * @def XOR_COND
 *********************************************************************
 * @brief We define this as the default condition handler.
 *        We overload this so that in test code we can
 *        test when these conditions are false.
 * 
 *        The naming is intentionally short (omitting FBE)
 *        so that we can keep some conditionals on one line.
 *
 *********************************************************************/
#define XOR_COND(m_cond) (m_cond)

#if DBG
#define XOR_DEBUG_ENABLED 1
#else
#define XOR_DEBUG_ENABLED 0
#endif

#if XOR_DEBUG_ENABLED
#define XOR_DBG_COND(m_cond) (m_cond)
#else
#define XOR_DBG_COND(m_cond) (FBE_FALSE)   
#endif

/*!*******************************************************************
 * @def FBE_XOR_MAX_TRACE_CHARS
 *********************************************************************
 * @brief max number of chars we will put in a formatted trace string.
 *
 *********************************************************************/
#define FBE_XOR_MAX_TRACE_CHARS 128



/* These define some default params for our trace functions. 
 * This makes is much easier to maintain if we need to change 
 * these default params for any reason. 
 */
#define FBE_XOR_LIBRARY_TRACE_PARAMS_CRITICAL FBE_TRACE_LEVEL_CRITICAL_ERROR, __LINE__, __FUNCTION__
#define FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR FBE_TRACE_LEVEL_ERROR, __LINE__, __FUNCTION__
#define FBE_XOR_LIBRARY_TRACE_PARAMS_DEBUG_HIGH FBE_TRACE_LEVEL_DEBUG_HIGH, __LINE__, __FUNCTION__
#define FBE_XOR_LIBRARY_TRACE_PARAMS_INFO FBE_TRACE_LEVEL_INFO, __LINE__, __FUNCTION__
#define FBE_XOR_LIBRARY_TRACE_PARAMS_WARNING FBE_TRACE_LEVEL_INFO, __LINE__, __FUNCTION__

/***************************************************
 * In some cases we may want to enable/disable
 * validation of the number of map-ins, map-outs
 * that the XOR driver does.
 *
 * if XOR_MAPPING_VALIDATION is defined as 1
 *   then we will count the numbers of map ins
 *   and fbe_assert when we fail to unmap appropriately.
 *
 * else if XOR_MAPPING_VALIDATION is not defined
 *        then we will not count or validate the
 *        numbers of map ins.
 *
 ***************************************************/

#if DBG || defined(LAB_DEBUG) || defined(UMODE_ENV) || defined(SIMMODE_ENV)

//#define XOR_MAPPING_VALIDATION 1

#endif

/*! @todo This does nothing for now.
 */
#define FBE_XOR_INC_STATS(m_stats)((void)0)

/*!*******************************************************************
 * fbe_xor_is_map_out_required()
 *********************************************************************
 * @brief This is carried over from hemi's
 *        HEMI_IS_MAP_OUT_REQUIRED().
 *        We currently do not support this mapping,
 *        but have retained this function to keep track of all
 *        the places where it might be needed.
 * 
 * @param mem_p - The memory to prefetch.
 * 
 * @return None.
 *********************************************************************/
static __inline void fbe_xor_prefetch(void *mem_p)
{
#ifdef _AMD64_   
    csx_p_io_prefetch((const char *)mem_p, CSX_P_IO_PREFETCH_HINT_NTA);
#endif
    return;
}

/*!*******************************************************************
 * @def FBE_XOR_MAX_MAP_IN_SIZE
 *********************************************************************
 * @brief Used for validation checks of mapped in memory
 *        Originally this used EMM_BLOCK_SIZE (512*520), but when we converted it
 *        to FBE, we changed it to 0xFFFF.
 *
 *********************************************************************/
#define FBE_XOR_MAX_MAP_IN_SIZE 0xFFFF

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

/*! Mapping is not required at this time.
 */
#define XOR_MAP_REQUIRED 0
/*!*******************************************************************
 * fbe_xor_is_map_in_required()
 *********************************************************************
 * @brief This is carried over from hemi's HEMI_IS_MAP_IN_REQUIRED().
 *        We currently do not support mapping, but have retained this
 *        function to keep track of all the places where it might be needed.
 *********************************************************************/
static __inline fbe_bool_t fbe_xor_is_map_in_required(fbe_sg_element_t *sg_p)
{
    return FBE_FALSE;
}
/*!*******************************************************************
 * fbe_xor_is_map_out_required()
 *********************************************************************
 * @brief This is carried over from hemi's
 *        HEMI_IS_MAP_OUT_REQUIRED().
 *        We currently do not support this mapping,
 *        but have retained this function to keep track of all
 *        the places where it might be needed.
 *********************************************************************/
static __inline fbe_bool_t fbe_xor_is_map_out_required(fbe_u8_t *map_ptr)
{
    return FBE_FALSE;
}

/*!*******************************************************************
 * fbe_xor_sgl_map_memory()
 *********************************************************************
 * @brief This is adopted from HEMI_SGL_MAP_MEMORY.
 *        
 *        We currently do not support this mapping,
 *        but have retained it to keep track of all the places where
 *        mapping might be needed.
 *
 *********************************************************************/
static __inline void* fbe_xor_sgl_map_memory(fbe_sg_element_t *in_sgl, fbe_u32_t offset, fbe_u32_t byte_count)
{
    return in_sgl->address;
}

/*!*******************************************************************
 * fbe_xor_sgl_unmap_memory()
 *********************************************************************
 * @brief This is adopted from HEMI_SGL_UNMAP_MEMORY.
 *        
 *        We currently do not support this un mapping,
 *        but have retained it to keep track of all the places where
 *        unmapping might be needed.
 *
 *********************************************************************/
static __inline void fbe_xor_sgl_unmap_memory(fbe_sg_element_t *in_sgl, fbe_u32_t byte_count)
{
    return;
}

/*********************************
 * STRUCTURE DEFINITIONS
 *********************************/

/*!*******************************************************************
 * @struct fbe_xor_sgl_descriptor_t
 *********************************************************************
 * @brief This structure is used to manipulate SG Lists.
 *
 * @param sgptr - Pointer to current SG List.
 * @param bytptr - Pointer to the physical memory associated with SG List.
 * @param bytcnt - Count assopciated with the current SG List.
 * @param map_ptr - ptr to mapped in memory
 * @param map_cnt - count of memory bytes that we mapped in
 * @param total_mapped_bytes - total memory we've mapped in
 *
 *********************************************************************/
typedef struct fbe_xor_sgl_descriptor_s
{
    fbe_sg_element_t *sgptr;
    fbe_u8_t *bytptr;
    fbe_u32_t bytcnt;

#if XOR_MAP_REQUIRED
    fbe_u8_t *map_ptr;
    fbe_u32_t map_cnt;
#endif
}
fbe_xor_sgl_descriptor_t;

/*!*******************************************************************
 * @struct fbe_xor_scratch_t
 *********************************************************************
 * @brief
 *
 * This structure defines the memory required to carry out eval operations. 
 *
 *  rbld_blk      - Sector needing a rebuild.
 *  fatal_blk     - Sector in strip detected as bad. 
 *  fatal_key     - Position of bad sector(s) in strip. 
 *  fatal_cnt     - Number of bad sectors in strip. 
 *  running_csum  - Used to track the checksum of parity/rebuild data. 
 *  seed          - Used to in the checksum calculations. 
 *  xor_required  - Used to indicate that data must be XOR'ed out. 
 *  initialized   - Indicates that this is a fill operation. 
 *  xors_remaining - Number of xors remaining to complete rebuild  
 *********************************************************************/
typedef struct fbe_xor_scratch_s
{
    fbe_sector_t   *rbld_blk;
    fbe_sector_t   *fatal_blk;
    fbe_xor_option_t option;
    fbe_u16_t       fatal_key;
    fbe_u16_t       fatal_cnt;
    fbe_u32_t       running_csum;
    fbe_lba_t       seed;
    fbe_bool_t      xor_required;
    fbe_bool_t      initialized;
    fbe_bool_t      b_valid_shed;
    fbe_u16_t       xors_remaining;
    fbe_bool_t      copy_required;
    fbe_sector_t    scratch_sector;
}
fbe_xor_scratch_t;

/********************************************************************
 * @struct fbe_xor_strip_range_t
 * 
 *
 * This structure is used to determine where in a strip data falls.  
 *
 *   lo      - Low range
 *   hi     - High range
 * 
 ********************************************************************/
typedef struct fbe_xor_strip_range_s
{
    fbe_lba_t lo;
    fbe_lba_t hi;
}
fbe_xor_strip_range_t;

/*
 * Functions
 */

/************************
 * PROTOTYPE DEFINITIONS
 ************************/

/* fbe_xor_sector_history.c
 */
fbe_status_t fbe_xor_report_error_trace_sector( const fbe_lba_t lba, 
                               const fbe_u32_t pos_bitmask,
                               const fbe_u32_t num_diff_bits_in_crc, 
                               const fbe_object_id_t raid_group_object_id,
                               const fbe_lba_t raid_group_offset, 
                               const fbe_sector_t * const sect_p,
                               const char * const error_string_p,
                               const fbe_sector_trace_error_level_t error_level,
                               const fbe_sector_trace_error_flags_t error_flag );

/*!*************************************************************************
 * @fn fbe_xor_record_invalid_lba_stamp()
 **************************************************************************
 *
 * @brief
 *   Records an invalid lba stamp in the eboard, and saves the fbe_sector_t
 *   in the bad crc fbe_sector_t log.
 *
 * PARAMETERS:
 *   m_lba,       [I] - Lba that took the error.
 *   m_dkey       [I] - Relative position that took the error.
 *   m_eboard_p   [I] - Pointer to the eboard.
 *   m_dblk_p     [I] - Pointer to the fbe_sector_t that took the error.
 **************************************************************************/
static __inline fbe_status_t fbe_xor_record_invalid_lba_stamp(fbe_lba_t m_lba,
                                                       fbe_u16_t m_dkey, 
                                                       fbe_xor_error_t *m_eboard_p, 
                                                       fbe_sector_t *m_dblk_p)
{
    fbe_status_t status;

    (m_eboard_p)->crc_lba_stamp_bitmap |= (m_dkey);
    status = fbe_xor_report_error_trace_sector(m_lba,
                                         m_dkey,
                                         0, /* number of crc bits is 0 */
                                         (m_eboard_p)->raid_group_object_id,
                                         (m_eboard_p)->raid_group_offset,
                                         m_dblk_p,
                                         "LBA Stamp",
                                         FBE_SECTOR_TRACE_ERROR_LEVEL_ERROR,
                                         FBE_SECTOR_TRACE_ERROR_FLAG_LBA);
    if (status != FBE_STATUS_OK) { return status; }
    return FBE_STATUS_OK;
}

static __forceinline fbe_u32_t fbe_xor_init_sg_no_prefetch(fbe_xor_sgl_descriptor_t *sgd_p,
                                                           fbe_sg_element_t *sgl_p)
{
    sgd_p->bytcnt = (sgl_p)->count;
    sgd_p->bytptr = (sgl_p)->address;
    sgd_p->sgptr  = (sgl_p) + (0 < sgd_p->bytcnt);
    return sgd_p->bytcnt;
}

static __forceinline fbe_u16_t  fbe_xor_get_lba_stamp(fbe_lba_t lba)
{
    return (fbe_u16_t)((lba) ^ (lba >> 16) ^ (lba >> 32) ^ (lba >> 48)); 
}

/* Macros handling SGLs */

fbe_status_t fbe_xor_sgld_init_with_offset(fbe_xor_sgl_descriptor_t *sgd,
                                               fbe_sg_element_t *sgptr,
                                               fbe_u32_t blocks,
                                               fbe_u32_t block_size);
fbe_u32_t fbe_xor_sgld_init(fbe_xor_sgl_descriptor_t *m_sgd,
                                      fbe_sg_element_t *m_sgl,
                                      fbe_u32_t m_blksize);
fbe_u8_t* fbe_xor_sgld_map(fbe_xor_sgl_descriptor_t *sgd_p,
                                   fbe_sg_element_t *sgl);
fbe_status_t fbe_xor_sgld_unmap(fbe_xor_sgl_descriptor_t *sgd_p);
void fbe_xor_sgld_null(fbe_xor_sgl_descriptor_t *sgd);

fbe_status_t fbe_xor_sgld_inc_n(fbe_xor_sgl_descriptor_t *sgd_p,
                                       fbe_u32_t blkcnt,
                                       fbe_u32_t blksize,
                                       fbe_u32_t *sg_bytes_p);
fbe_status_t fbe_xor_init_sg_and_sector_with_offset(fbe_sg_element_t **sg_pp, fbe_sector_t **sector_pp, fbe_u32_t offset);

#define FBE_XOR_SGLD_ADDR(m_sgd) \
((m_sgd).bytptr)

#define FBE_XOR_SGLD_COUNT(m_sgd) \
((m_sgd).bytcnt)

#define FBE_XOR_SGLD_GET_MAP(m_sgd) \
((m_sgd).map_ptr)

#define FBE_XOR_SGLD_GET_CNT(m_sgd) \
((m_sgd).map_cnt)

#define FBE_XOR_SGLD_NULL(m_sgd) \
(fbe_xor_sgld_null(&m_sgd))

#define FBE_XOR_SGLD_INIT(m_sgd,m_sgl,m_blksize)\
(fbe_xor_sgld_init(&m_sgd,m_sgl,m_blksize))

#define FBE_XOR_SGLD_UNMAP(m_sgd)\
(fbe_xor_sgld_unmap(&m_sgd))

#define FBE_XOR_SGLD_MAP(m_sgd,m_sgl)\
(fbe_xor_sgld_unmap(&m_sgd,m_sgl))

#define FBE_XOR_SGLD_INC_N(m_sgd,m_blkcnt,m_blksize,m_sg_bytes)  \
  (fbe_xor_sgld_inc_n(&m_sgd,m_blkcnt,m_blksize,&m_sg_bytes))

#define FBE_XOR_SGLD_INC_BE_N(m_sgd,m_blkcnt,m_sg_bytes) \
(FBE_XOR_SGLD_INC_N(m_sgd,m_blkcnt,FBE_BE_BYTES_PER_BLOCK,m_sg_bytes))

#define FBE_XOR_SGLD_INIT_BE(m_sgd,m_sgl) \
(FBE_XOR_SGLD_INIT(m_sgd,m_sgl, FBE_BE_BYTES_PER_BLOCK))

#define FBE_XOR_SGLD_INC_BE(m_sgd,m_sg_bytes) \
(FBE_XOR_SGLD_INC_BE_N(m_sgd, 1,m_sg_bytes))

#define FBE_XOR_SGLD_INIT_WITH_OFFSET(m_sgd,m_sgptr,m_offset,m_blksize)\
(fbe_xor_sgld_init_with_offset(&m_sgd,m_sgptr,m_offset,m_blksize))

#define FBE_XOR_INIT_EBOARD_WMASK(m_EBoard)            \
(m_EBoard->w_bitmap |= (m_EBoard->u_bitmap |       \
                        m_EBoard->c_crc_bitmap |   \
                        m_EBoard->c_coh_bitmap |   \
                        m_EBoard->c_ss_bitmap |    \
                        m_EBoard->c_ws_bitmap |    \
                        m_EBoard->c_ts_bitmap |    \
                        m_EBoard->c_n_poc_coh_bitmap |  \
                        m_EBoard->c_poc_coh_bitmap |    \
                        m_EBoard->m_bitmap | \
                        m_EBoard->c_rm_magic_bitmap | \
                        m_EBoard->c_rm_seq_bitmap))

/*******************************
 * IN-LINE FUNCTION DEFINITIONS
 *******************************/
/* This function corrects one error in an uncorrectable bitmap,
 * and sets that error as correctable.
 */
static __forceinline void fbe_xor_eboard_correct_one(fbe_u16_t *u_bitmap_p,
                                          fbe_u16_t *c_bitmap_p,
                                          fbe_u16_t bitkey)
{
    if (*u_bitmap_p & bitkey)
    {
        /* If the bitkey is set in the uncorrectable bitmask,
         * then clear it out from the uncorrectable bitmask,
         * and set the bitkey in the correctable bitmask.
         */
        *c_bitmap_p |= bitkey;
        *u_bitmap_p &= ~bitkey;
    }
    return;
}
/* end fbe_xor_eboard_correct_one */


/************************
 * PROTOTYPE DEFINITIONS
 ************************/

/* fbe_xor_interface.c
 */
fbe_status_t fbe_xor_mem_move(fbe_xor_mem_move_t *mem_move_p,
                              fbe_xor_move_command_t move_cmd,
                              fbe_xor_option_t option,
                              fbe_u16_t src_dst_pairs, 
                              fbe_u16_t ts,
                              fbe_u16_t ss,
                              fbe_u16_t ws);
/* fbe_xor_util.c
 */
fbe_u16_t fbe_xor_get_timestamp(const fbe_u16_t thread_number);
fbe_status_t fbe_xor_generate_positions_from_bitmask(fbe_u16_t bitmask,
                                                     fbe_u16_t *positions_p,
                                                     fbe_u32_t positions_size);
fbe_u32_t fbe_xor_get_bitcount(fbe_u16_t bitmask);
fbe_u32_t fbe_xor_get_first_position_from_bitmask(fbe_u16_t bitmask,
                                                  fbe_u32_t width);
void fbe_xor_copy_data(const fbe_u32_t * srcptr, fbe_u32_t * tgtptr);
fbe_lba_t fbe_xor_convert_lba_to_pba(const fbe_lba_t lba,
                                     const fbe_lba_t raid_group_offset);
fbe_status_t fbe_xor_set_status(fbe_xor_status_t *status_p,
                                fbe_xor_status_t value);
void fbe_xor_vr_increment_errcnt(fbe_raid_verify_error_counts_t * vp_eboard_p,
                                 fbe_xor_error_t * eboard_p,
                                 const fbe_u16_t *parity_positions,
                                 fbe_u16_t parity_drives);
void fbe_xor_correct_all(fbe_xor_error_t * eboard_p);
fbe_status_t fbe_xor_invalidate_sectors(fbe_sector_t * const sector_p[],
                                const fbe_u16_t bitkey[],
                                const fbe_u16_t width,
                                const fbe_lba_t seed,
                                const fbe_u16_t ibitmap,
                                const fbe_xor_error_t *const eboard_p);
void fbe_xor_invalidate_checksum(fbe_sector_t * sector_p);
fbe_xor_status_t fbe_xor_handle_bad_crc_on_write(fbe_sector_t *const data_blk_p,
                                                 const fbe_lba_t lba,
                                                 const fbe_u16_t position_bitmask,
                                                 const fbe_xor_option_t option,
                                                 fbe_xor_error_t *const eboard_p,
                                                 fbe_xor_error_regions_t *const eregions_p,
                                                 const fbe_u16_t width);
fbe_xor_status_t fbe_xor_handle_bad_crc_on_read(const fbe_sector_t *const data_blk_p,
                                                const fbe_lba_t lba,
                                                const fbe_u16_t position_bitmask,
                                                const fbe_xor_option_t option,
                                                fbe_xor_error_t *const eboard_p,
                                                fbe_xor_error_regions_t *const eregions_p,
                                                const fbe_u16_t width);
fbe_status_t fbe_xor_validate_data_if_enabled(const fbe_sector_t *const sector_p,
                                              const fbe_lba_t seed,
                                              const fbe_xor_option_t option);
void fbe_xor_correct_all_one_pos(fbe_xor_error_t * const eboard_p,
                             const fbe_u16_t bitkey);

void fbe_xor_correct_all_non_crc_one_pos( fbe_xor_error_t * const eboard_p,
                                      const fbe_u16_t bitkey );

/* fbe_xor_check_sector.c
 */
fbe_status_t fbe_xor_determine_csum_error( fbe_xor_error_t * const eboard_p,
                                           const fbe_sector_t * const blk_p,
                                           const fbe_u16_t key,
                                           const fbe_lba_t seed,
                                           const fbe_u16_t csum_read,
                                           const fbe_u16_t csum_calc,
                                           const fbe_bool_t b_check_invalidated_lba);
fbe_status_t  fbe_xor_determine_media_errors( fbe_xor_error_t *eboard_p,
                                              fbe_u16_t media_err_bitmap );
fbe_status_t fbe_xor_validate_data(const fbe_sector_t *const sector_p,
                                   const fbe_lba_t seed,
                                   const fbe_xor_option_t option);
fbe_bool_t fbe_xor_is_sector_invalidated(const fbe_sector_t *const sector_p,
                                         xorlib_sector_invalid_reason_t *const reason_p);

/* fbe_xor_sector_history.c
 */
fbe_status_t fbe_xor_report_error_trace_sector( const fbe_lba_t lba, 
                                           const fbe_u32_t pos_bitmask, 
                                           const fbe_u32_t num_diff_bits_in_crc, 
                                           const fbe_object_id_t raid_group_object_id,
                                           const fbe_lba_t raid_group_offset, 
                                           const fbe_sector_t * const sect_p,
                                           const char * const error_string_p,
                                           const fbe_sector_trace_error_level_t error_level,
                                           const fbe_sector_trace_error_flags_t error_flag);
fbe_status_t fbe_xor_report_error(const char * const error_string_p,
                                  const fbe_sector_trace_error_level_t error_level,
                                  const fbe_sector_trace_error_flags_t error_flag );
fbe_status_t fbe_xor_trace_sector( const fbe_lba_t lba, 
                               const fbe_u32_t pos_bitmask,
                               const fbe_u32_t num_diff_bits_in_crc, 
                               const fbe_object_id_t raid_group_object_id,
                               const fbe_lba_t raid_group_offset, 
                               const fbe_sector_t * const sect_p,
                               const char * const error_string_p,
                               const fbe_sector_trace_error_level_t error_level,
                               const fbe_sector_trace_error_flags_t error_flag );
fbe_status_t fbe_xor_trace_sector_complete(const fbe_sector_trace_error_level_t error_level,
                                           const fbe_sector_trace_error_flags_t error_flag);
fbe_status_t fbe_xor_sector_history_trace_vector( const fbe_lba_t lba, 
                                      const fbe_sector_t * const sector_p[],
                                      const fbe_u16_t pos_bitmask[],
                                      const fbe_u16_t width,
                                      const fbe_u16_t ppos,
                                      const fbe_object_id_t raid_group_object_id,
                                      const fbe_lba_t raid_group_offset, 
                                      char *error_string_p,
                                      const fbe_sector_trace_error_level_t error_level,
                                      const fbe_sector_trace_error_flags_t error_flag);
fbe_status_t fbe_xor_sector_history_generate_sector_trace_info(const fbe_xor_error_type_t err_type,
                                                               const fbe_bool_t uncorrectable,
                                                               fbe_sector_trace_error_level_t *const sector_trace_level_p,
                                                               fbe_sector_trace_error_flags_t *const sector_trace_flags_p,
                                                               const char **error_string_p);

/* fbe_xor_sector_trace.c
 */
fbe_sector_trace_error_flags_t fbe_xor_sector_trace_generate_trace_flag(const fbe_xor_error_type_t err_type,
                                                                        const char **error_string_p);
void fbe_xor_sector_trace_sector(fbe_sector_trace_error_level_t error_level,
                                 fbe_sector_trace_error_flags_t error_flag,
                                 const fbe_sector_t *const sector_p);

/* fbe_xor_error_region.c 
 */
fbe_status_t fbe_xor_save_crc_error_region( fbe_xor_error_regions_t * const regions_p,
                                            const fbe_xor_error_t * const eboard_p, 
                                            const fbe_lba_t lba, 
                                            const fbe_u16_t pos_bitmask,
                                            const fbe_xor_error_type_t e_err_type,
                                            const fbe_bool_t b_correctable,
                                            const fbe_u32_t parity_drives,
                                            fbe_u16_t width,
                                            fbe_u16_t parity_pos);

fbe_status_t fbe_xor_save_error_region( const fbe_xor_error_t * const eboard_p, 
                                        fbe_xor_error_regions_t * const regions_p, 
                                        const fbe_bool_t b_invalidate,
                                        const fbe_lba_t lba,
                                        const fbe_u32_t parity_drives,
                                        const fbe_u16_t width,
                                        const fbe_u16_t parity_pos);

fbe_status_t fbe_xor_eboard_get_error(const fbe_xor_error_t * const eboard_p,
                                      const fbe_u16_t pos_bitmask,
                                      const fbe_xor_error_type_t e_err_type,
                                      const fbe_bool_t b_correctable,
                                      const fbe_u16_t parity_drives,
                                      fbe_xor_error_type_t * const xor_error_type_p);

fbe_status_t fbe_xor_create_error_region(fbe_xor_error_type_t error,
                                         fbe_u16_t error_bitmask,
                                         fbe_lba_t lba,
                                         fbe_xor_error_regions_t *error_region_p,
                                         fbe_u16_t width,
                                         fbe_u16_t parity_pos);

fbe_bool_t fbe_xor_bitmap_contains_non_lba_stamp_crc_error(fbe_u16_t crc_pos_bitmap, fbe_xor_error_t * eboard_p);

/* fbe_xor_raid5_eval_parity.c
 */
fbe_status_t fbe_xor_eval_parity_unit(fbe_xor_error_t * eboard_p,
                                      fbe_sector_t * sector_p[],
                                      fbe_u16_t bitkey[],
                                      fbe_u16_t width,
                                      fbe_u16_t ppos,
                                      fbe_u16_t rpos,
                                      fbe_lba_t seed,
                                      fbe_bool_t b_final_recovery_attempt,
                                      fbe_bool_t b_preserve_shed_data,
                                      fbe_xor_option_t option,
                                      fbe_u16_t *invalid_bitmask_p);

/* fbe_xor_mirror.c
 */
fbe_status_t fbe_xor_validate_mirror_verify(fbe_xor_mirror_reconstruct_t *verify_p);
fbe_status_t fbe_xor_verify_r1_unit(fbe_xor_error_t * eboard_p,
                                    fbe_sector_t * sector_p[],
                                    fbe_u16_t bitkey[],
                                    fbe_u16_t width,
                                    fbe_u16_t valid_bitmap,
                                    fbe_u16_t needs_rebuild_bitmap,
                                    fbe_lba_t seed,
                                    fbe_xor_option_t option,
                                    fbe_u16_t *invalid_bitmask_p,
                                    fbe_bool_t raw_mirror_io_b);

fbe_status_t fbe_xor_validate_mirror_rebuild(fbe_xor_mirror_reconstruct_t *rebuild_p);
fbe_status_t fbe_xor_rebuild_r1_unit(fbe_xor_error_t *eboard_p,
                                     fbe_sector_t *sector_p[],
                                     fbe_u16_t bitkey[],
                                     fbe_u16_t width,
                                     fbe_bool_t primary_validated,
                                     fbe_u32_t primary,
                                     fbe_u32_t rpos,
                                     fbe_lba_t seed,
                                     fbe_xor_option_t option,
                                     fbe_u16_t *invalid_bitmask_p,
                                     fbe_bool_t raw_mirror_io_b);


/****************************************
 * fbe_xor_raid6_util.c
 ****************************************/
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
                                         fbe_u16_t * const invalid_bitmask_p);
fbe_bool_t fbe_xor_r6_eval_parity_of_checksums(fbe_sector_t * row_parity_blk_ptr,
                                            fbe_sector_t * diag_parity_blk_ptr,
                                            fbe_u16_t array_width,
                                            fbe_u16_t parity_drives);
fbe_status_t fbe_xor_init_raid6_globals(void);

/****************************************
 * fbe_xor_trace.c
 ****************************************/
void fbe_xor_library_trace(fbe_trace_level_t trace_level,
                           fbe_u32_t line,
                           const char *const function_p,
                           fbe_char_t * fail_string_p, ...)
                           __attribute__((format(__printf_func__,4,5)));
fbe_status_t fbe_xor_trace_checksum_errored_sector(fbe_sector_t *const sector_p,
                                                   const fbe_u16_t position_mask, 
                                                   const fbe_lba_t seed,
                                                   const fbe_xor_option_t option,
                                                   const fbe_xor_status_t xor_status);

/**************************************
 * end fbe_xor_interface_striper.c
 **************************************/
fbe_status_t fbe_xor_raid0_verify(fbe_xor_striper_verify_t *xor_verify_p);
fbe_status_t fbe_xor_validate_raid0_stamps(fbe_sector_t *sector_p, fbe_lba_t lba, fbe_lba_t offset);


/* fbe_xor_interface_parity.c
 */
fbe_status_t fbe_xor_parity_rebuild(fbe_xor_parity_reconstruct_t *rebuild_p);
fbe_status_t fbe_xor_parity_reconstruct(fbe_xor_parity_reconstruct_t *rc_p);
fbe_status_t fbe_xor_parity_verify(fbe_xor_parity_reconstruct_t *verify_p);
fbe_status_t fbe_xor_parity_468_write(fbe_xor_468_write_t *const write_p);
fbe_status_t fbe_xor_parity_mr3_write(fbe_xor_mr3_write_t *const write_p);
fbe_status_t fbe_xor_parity_verify_copy_user_data(fbe_xor_mem_move_t *mem_move_p);
fbe_status_t fbe_xor_parity_recovery_verify_const_parity(fbe_xor_recovery_verify_t *xor_recovery_verify_p);

/* fbe_xor_mirror.c
 */
fbe_status_t fbe_xor_mirror_verify(fbe_xor_mirror_reconstruct_t *rebuild_p);
fbe_status_t fbe_xor_mirror_rebuild(fbe_xor_mirror_reconstruct_t *rebuild_p);
fbe_status_t fbe_xor_validate_mirror_stamps(fbe_sector_t *sector_p, fbe_lba_t lba, fbe_lba_t offset);


static __forceinline void fbe_xor_inc_sector_for_sg(fbe_sg_element_t **sg_p, fbe_sector_t **sector_pp)
{
    /* Increment a sector ptr for a given sg element. 
     * If we go beyond the sg element, we increment the sg to the next element. 
     */
    if (((*sector_pp) + 1) < (fbe_sector_t*)((*sg_p)->address + (*sg_p)->count))
    {
        /* We are within the same SG, just increment the sector p.
         */
        (*sector_pp)++;
    }
    else
    {
        /* We exhausted the current sg, move to the next.
         */
        (*sg_p)++;
        *sector_pp = (fbe_sector_t*)(*sg_p)->address;
    }
}
/* end fbe_xor_inc_sector_for_sg() */

/*************************
 * end file fbe_xor_private.h
 *************************/
#endif /* FBE_XOR_PRIVATE_H */
